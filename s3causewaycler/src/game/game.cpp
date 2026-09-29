#include "game/game.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace ccler {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kSpeed = 190.f;
constexpr float kReach = 24.f;
constexpr float kSweep = 0.24f;
constexpr int kClock = 36 * 60;
constexpr float kL = 108.f, kR = 212.f, kT = 72.f, kB = 206.f;

struct Lay {
    float x, y;
    int kind;
};

constexpr Lay kLay[6] = {
    {136.f, 188.f, 0}, {188.f, 170.f, 1}, {124.f, 148.f, 2},
    {196.f, 126.f, 3}, {140.f, 104.f, 4}, {180.f, 84.f, 5},
};

float dist(float x0, float y0, float x1, float y1) {
    float dx = x1 - x0, dy = y1 - y0;
    return std::sqrt(dx * dx + dy * dy);
}

const gs::Mipped& bitArt(const Art& a, int kind) {
    if (kind == 1) return a.crate;
    if (kind == 2) return a.net;
    if (kind == 3) return a.barrel;
    if (kind == 4) return a.buoy;
    if (kind == 5) return a.rope;
    return a.timber;
}

int bitPal(int kind) {
    if (kind == 2 || kind == 5) return PAL_NET;
    if (kind == 4) return PAL_BUOY;
    return PAL_WOOD;
}

float bitH(int kind) {
    if (kind == 0) return 16.f;
    if (kind == 1) return 22.f;
    if (kind == 2) return 20.f;
    if (kind == 3) return 26.f;
    if (kind == 4) return 28.f;
    return 16.f;
}

const char* bitName(int kind) {
    if (kind == 1) return "CRATE";
    if (kind == 2) return "NET";
    if (kind == 3) return "BARREL";
    if (kind == 4) return "BUOY";
    if (kind == 5) return "ROPE";
    return "TIMBER";
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Won) return 3;
    if (mode_ == Mode::Lost) return 4;
    if (cleared_ >= 4) return 2;
    return 1;
}

float Game::rnd() {
    rng_ = rng_ * 1664525u + 1013904223u;
    return float((rng_ >> 8) & 0xffffff) / float(0x1000000);
}

void Game::resetDeck() {
    for (int i = 0; i < kBits; i++) {
        bit_[i].x = kLay[i].x;
        bit_[i].y = kLay[i].y;
        bit_[i].kind = kLay[i].kind;
        bit_[i].work = 0;
        bit_[i].gone = false;
    }
    for (auto& m : mote_) m.life = 0;
    px_ = 160.f;
    py_ = 204.f;
    face_ = 1.f;
    step_ = 0;
    cleared_ = 0;
    focus_ = -1;
    sweeping_ = false;
    clock_ = kClock;
    shake_ = 0;
}

void Game::toTitle() {
    over_ = false;
    won_ = false;
    reason_ = "";
    sweeping_ = false;
    blip_ = 0;
    tick_ = 0;
    resetDeck();
    mode_ = Mode::Title;
}

void Game::begin() {
    resetDeck();
    over_ = false;
    won_ = false;
    reason_ = "";
    mode_ = Mode::Play;
    blip(220.f, 0.08f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    t_ = 0;
    toTitle();
    if (bot_) begin();
}

bool Game::startPressed() const {
    const gs::Pad& p = sys_->pad;
    return p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C) || p.pressed(gs::BTN_TURBO);
}

int Game::focusBit() const {
    int best = -1;
    float bd = kReach + 0.01f;
    for (int i = 0; i < kBits; i++) {
        if (bit_[i].gone) continue;
        float d = dist(px_, py_, bit_[i].x, bit_[i].y);
        if (d < bd) {
            bd = d;
            best = i;
        }
    }
    return best;
}

void Game::botInput(float& ix, float& iy, bool& hold) {
    ix = iy = 0;
    hold = false;
    int id = 0;
    while (id < kBits && bit_[id].gone) id++;
    if (id >= kBits) return;
    float dx = bit_[id].x - px_, dy = bit_[id].y - py_;
    float d = std::sqrt(dx * dx + dy * dy);
    hold = d <= kReach;
    if (d < 1.1f) {
        px_ = std::clamp(bit_[id].x, kL, kR);
        py_ = std::clamp(bit_[id].y, kT, kB);
        return;
    }
    ix = dx / d;
    iy = dy / d;
}

void Game::humanInput(float& ix, float& iy, bool& hold) {
    const gs::Pad& p = sys_->pad;
    ix = iy = 0;
    if (p.down(gs::BTN_LEFT)) ix -= 1.f;
    if (p.down(gs::BTN_RIGHT)) ix += 1.f;
    if (p.down(gs::BTN_UP)) iy -= 1.f;
    if (p.down(gs::BTN_DOWN)) iy += 1.f;
    if (std::fabs(p.axisX) > 0.2f || std::fabs(p.axisY) > 0.2f) {
        ix = p.axisX;
        iy = -p.axisY;
    }
    float m = std::sqrt(ix * ix + iy * iy);
    if (m > 1.f) {
        ix /= m;
        iy /= m;
    }
    hold = p.down(gs::BTN_A) || p.down(gs::BTN_B) || p.down(gs::BTN_C) || p.down(gs::BTN_X) || p.down(gs::BTN_Y) ||
           p.down(gs::BTN_Z);
}

void Game::puff(float x, float y) {
    for (int n = 0; n < 4; n++) {
        for (auto& m : mote_) {
            if (m.life > 0) continue;
            m.x = x;
            m.y = y;
            m.vx = (rnd() - 0.5f) * 50.f;
            m.vy = -20.f - rnd() * 30.f;
            m.life = 0.35f + rnd() * 0.2f;
            break;
        }
    }
}

void Game::win() {
    mode_ = Mode::Won;
    won_ = true;
    over_ = true;
    reason_ = "THE GROUND IS CLEAR";
    blip(520.f, 0.18f);
}

void Game::lose() {
    mode_ = Mode::Lost;
    won_ = false;
    over_ = true;
    reason_ = "THE CLOCK DIED";
    blip(90.f, 0.28f);
}

void Game::updatePlay() {
    float ix, iy;
    bool hold = false;
    if (bot_) botInput(ix, iy, hold);
    else humanInput(ix, iy, hold);

    if (ix != 0.f) face_ = ix < 0 ? -1.f : 1.f;
    float sp = std::sqrt(ix * ix + iy * iy);
    px_ = std::clamp(px_ + ix * kSpeed * kDt, kL, kR);
    py_ = std::clamp(py_ + iy * kSpeed * kDt, kT, kB);
    if (sp > 0.15f) step_ += kDt * 8.f;

    focus_ = focusBit();
    sweeping_ = false;
    if (focus_ >= 0 && hold) {
        sweeping_ = true;
        bit_[focus_].work += kDt;
        if (bit_[focus_].work >= kSweep) {
            bit_[focus_].gone = true;
            cleared_++;
            puff(bit_[focus_].x, bit_[focus_].y);
            blip(340.f + float(cleared_) * 40.f, 0.07f);
            focus_ = -1;
            sweeping_ = false;
            if (cleared_ >= kBits) win();
        }
    } else if (focus_ >= 0) {
        bit_[focus_].work = std::max(0.f, bit_[focus_].work - kDt * 0.5f);
    }

    if (mode_ == Mode::Play) {
        clock_--;
        if (clock_ <= 0) {
            clock_ = 0;
            lose();
        }
    }

    for (auto& m : mote_) {
        if (m.life <= 0) continue;
        m.life -= kDt;
        m.x += m.vx * kDt;
        m.y += m.vy * kDt;
    }
    if (shake_ > 0) shake_ -= kDt;
}

void Game::blip(float freq, float hold) {
    blip_ = hold;
    sys_->apu.tone(0, freq, 0.18f);
}

void Game::serviceAudio() {
    if (blip_ > 0) {
        blip_ -= kDt;
        if (blip_ <= 0) sys_->apu.tone(0, 0, 0);
    }
    if (mode_ == Mode::Play && clock_ > 0 && clock_ < 10 * 60) {
        tick_ -= kDt;
        if (tick_ <= 0) {
            tick_ = 0.5f;
            sys_->apu.tone(1, clock_ < 5 * 60 ? 660.f : 440.f, 0.08f);
        }
    } else if (mode_ != Mode::Play) {
        sys_->apu.tone(1, 0, 0);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    if (mode_ == Mode::Title) {
        if (startPressed() || bot_) begin();
        serviceAudio();
        draw();
        return;
    }
    if (mode_ == Mode::Won || mode_ == Mode::Lost) {
        if (!bot_ && startPressed()) toTitle();
        serviceAudio();
        draw();
        return;
    }
    if (!bot_ && sys.pad.pressed(gs::BTN_START)) {
        mode_ = Mode::Pause;
        serviceAudio();
        draw();
        return;
    }
    if (mode_ == Mode::Pause) {
        if (startPressed()) mode_ = Mode::Play;
        serviceAudio();
        draw();
        return;
    }
    updatePlay();
    serviceAudio();
    draw();
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet, bool shadow) {
    if (!(h > 1.5f) || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::lround(cx - s.w * 0.5f + (shake_ > 0 ? std::sin(t_ * 40.f) * 2.f : 0.f)));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::text(const char* s, float x, float y, float scale, int pal) {
    if (!s || !s[0]) return;
    const float gap = std::max(1.f, scale * 2.f);
    float width = 0.f;
    for (const char* p = s; *p; ++p) {
        unsigned char c = static_cast<unsigned char>(*p);
        if (c < 33 || c > 126) width += 10.f * scale + gap;
        else width += float(art_.glyph[c - 32].w) * scale + gap;
    }
    width -= gap;
    x -= width * 0.5f;
    for (const char* p = s; *p; ++p) {
        unsigned char c = static_cast<unsigned char>(*p);
        if (c < 33 || c > 126) {
            x += 10.f * scale + gap;
            continue;
        }
        const gs::Mipped& g = art_.glyph[c - 32];
        float h = float(g.h) * scale;
        float w = float(g.w) * scale;
        spr(g, x + w * 0.5f, y, h, pal, false, 0, false);
        x += w + gap;
    }
}

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    float wave = std::sin(t_ * 1.6f) * 2.f;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t c;
        if (y < 36) c = gs::rgb4(1, 2, 5);
        else if (y < 64) c = gs::rgb4(2, 4, 7);
        else if (y < 92 + int(wave)) c = gs::rgb4(3, 6, 9);
        else if ((y / 6 + int(t_ * 3.f)) & 1) c = gs::rgb4(1, 5, 8);
        else c = gs::rgb4(1, 4, 7);
        v.lineBackdrop[y] = c;
        v.road[y].on = false;
        v.lineFog[y] = 0;
    }
    v.roadTime = int(t_ * 60.f);
}

void Game::world() {
    for (int i = 0; i < 6; i++) {
        float y = 78.f + float(i) * 22.f;
        spr(art_.slab, 160, y, 22, PAL_STONE, false, i > 4 ? 2 : 0, true);
        spr(art_.post, 96, y - 4, 26, PAL_STONE, false, 1, true);
        spr(art_.post, 224, y - 4, 26, PAL_STONE, true, 1, true);
    }
    spr(art_.lamp, 96, 58, 32, PAL_LAMP, false, 0, true);
    spr(art_.lamp, 224, 58, 32, PAL_LAMP, true, 0, true);
    spr(art_.clock, 160, 40, 26, PAL_TEXT, false, 0, true);

    for (int i = 0; i < kBits; i++) {
        if (bit_[i].gone) continue;
        float bob = (i == focus_ && sweeping_) ? std::sin(t_ * 18.f) * 2.f : 0.f;
        spr(art_.shadow, bit_[i].x, bit_[i].y + 2, 8, PAL_STONE, false, 0, true, true);
        spr(bitArt(art_, bit_[i].kind), bit_[i].x + bob, bit_[i].y, bitH(bit_[i].kind), bitPal(bit_[i].kind), false, 0,
            true);
    }

    int fr = int(step_) & 1;
    spr(art_.shadow, px_, py_ + 2, 10, PAL_STONE, false, 0, true, true);
    spr(art_.keeper[fr], px_, py_, 48, PAL_KEEPER, face_ < 0, 0, true);
    float bx = px_ + face_ * 16.f;
    float by = py_ - (sweeping_ ? 6.f : 14.f);
    spr(art_.rake, bx, by, sweeping_ ? 20.f : 28.f, PAL_WOOD, face_ < 0, 0, true);

    for (auto& m : mote_) {
        if (m.life <= 0) continue;
        spr(art_.puff, m.x, m.y, 8.f + (0.4f - m.life) * 10.f, PAL_STONE, false, 0, false);
    }
}

void Game::messages() {
    char buf[48];
    int sec = clock_ / 60;
    std::snprintf(buf, sizeof buf, "%d", sec);
    text(buf, 160, 8, 0.7f, sec < 8 ? PAL_ALERT : PAL_TEXT);

    if (mode_ == Mode::Title) {
        text("S3 CAUSEWAY CLER", 160, 72, 0.7f, PAL_TEXT);
        text("AT THE CAUSEWAY", 160, 94, 0.48f, PAL_LAMP);
        text("CLEAR THE GROUND", 160, 116, 0.48f, PAL_TEXT);
        text("BEFORE THE CLOCK DIES", 160, 134, 0.42f, PAL_ALERT);
        text("HOLD TO SWEEP", 160, 160, 0.4f, PAL_GOOD);
        if (int(t_ * 2.f) & 1) text("START", 160, 184, 0.48f, PAL_TEXT);
        return;
    }
    if (mode_ == Mode::Pause) {
        text("PAUSED", 160, 100, 0.7f, PAL_TEXT);
        return;
    }
    if (mode_ == Mode::Won) {
        text("THE GROUND IS CLEAR", 160, 92, 0.48f, PAL_GOOD);
        text("THE WATCH IS DONE", 160, 114, 0.4f, PAL_TEXT);
        return;
    }
    if (mode_ == Mode::Lost) {
        text("THE CLOCK DIED", 160, 96, 0.52f, PAL_ALERT);
        text("THE WATCH IS OVER", 160, 116, 0.4f, PAL_TEXT);
        std::snprintf(buf, sizeof buf, "LEFT %d", kBits - cleared_);
        text(buf, 160, 136, 0.42f, PAL_TEXT);
        return;
    }
    std::snprintf(buf, sizeof buf, "LEFT %d", kBits - cleared_);
    text(buf, 48, 10, 0.4f, PAL_TEXT);
    if (focus_ >= 0) {
        text(bitName(bit_[focus_].kind), 160, 200, 0.38f, sweeping_ ? PAL_GOOD : PAL_LAMP);
    } else {
        text("SWEEP THE CAUSEWAY", 160, 200, 0.36f, PAL_TEXT);
    }
}

void Game::draw() {
    sys_->vdp.clearSprites();
    sky();
    messages();
    world();
}

}  // namespace ccler

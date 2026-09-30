#include "game/culvert.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace ccler {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kSpeed = 150.f;
constexpr float kReach = 24.f;
constexpr float kDig = 0.28f;
constexpr int kClock = 36 * 60;
constexpr float kL = 108.f, kR = 212.f, kT = 78.f, kB = 196.f;

struct Lay {
    float x, y;
    int kind;
};

constexpr Lay kLay[6] = {
    {132.f, 184.f, 0}, {188.f, 168.f, 1}, {124.f, 146.f, 2},
    {196.f, 128.f, 3}, {140.f, 108.f, 4}, {178.f, 90.f, 5},
};

float dist(float x0, float y0, float x1, float y1) {
    float dx = x1 - x0, dy = y1 - y0;
    return std::sqrt(dx * dx + dy * dy);
}

const gs::Mipped& bitArt(const Art& a, int kind) {
    if (kind == 1) return a.brick;
    if (kind == 2) return a.branch;
    if (kind == 3) return a.tin;
    if (kind == 4) return a.clod;
    if (kind == 5) return a.wheel;
    return a.silt;
}

int bitPal(int kind) {
    if (kind == 1) return PAL_BRICK;
    if (kind == 2) return PAL_MOSS;
    if (kind == 3) return PAL_TIN;
    if (kind == 5) return PAL_TIN;
    return PAL_SILT;
}

float bitH(int kind) {
    if (kind == 1) return 16.f;
    if (kind == 2) return 14.f;
    if (kind == 3) return 18.f;
    if (kind == 5) return 22.f;
    return 18.f;
}

const char* bitName(int kind) {
    if (kind == 1) return "BRICK";
    if (kind == 2) return "BRANCH";
    if (kind == 3) return "TIN";
    if (kind == 4) return "CLOD";
    if (kind == 5) return "WHEEL";
    return "SILT";
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

void Game::resetFloor() {
    for (int i = 0; i < kBits; i++) {
        bit_[i].x = kLay[i].x;
        bit_[i].y = kLay[i].y;
        bit_[i].kind = kLay[i].kind;
        bit_[i].work = 0;
        bit_[i].gone = false;
    }
    for (auto& d : drop_) d.life = 0;
    px_ = 160.f;
    py_ = 198.f;
    face_ = 1.f;
    step_ = 0;
    cleared_ = 0;
    focus_ = -1;
    digging_ = false;
    clock_ = kClock;
}

void Game::toTitle() {
    over_ = false;
    won_ = false;
    reason_ = "";
    digging_ = false;
    blip_ = 0;
    tick_ = 0;
    resetFloor();
    mode_ = Mode::Title;
}

void Game::begin() {
    resetFloor();
    over_ = false;
    won_ = false;
    reason_ = "";
    mode_ = Mode::Play;
    blip(180.f, 0.1f);
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
    if (d < 1.2f) {
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

void Game::splash(float x, float y) {
    for (int n = 0; n < 3; n++) {
        for (auto& d : drop_) {
            if (d.life > 0) continue;
            d.x = x + (rnd() - 0.5f) * 10.f;
            d.y = y;
            d.vy = -18.f - rnd() * 24.f;
            d.life = 0.4f + rnd() * 0.2f;
            break;
        }
    }
}

void Game::win() {
    mode_ = Mode::Won;
    won_ = true;
    over_ = true;
    reason_ = "THE GROUND IS CLEAR";
    blip(480.f, 0.2f);
}

void Game::lose() {
    mode_ = Mode::Lost;
    won_ = false;
    over_ = true;
    reason_ = "THE CLOCK DIED";
    blip(70.f, 0.3f);
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
    if (sp > 0.15f) step_ += kDt * 7.f;

    focus_ = focusBit();
    digging_ = false;
    if (focus_ >= 0 && hold) {
        digging_ = true;
        bit_[focus_].work += kDt;
        if (bit_[focus_].work >= kDig) {
            bit_[focus_].gone = true;
            cleared_++;
            splash(bit_[focus_].x, bit_[focus_].y);
            blip(280.f + float(cleared_) * 36.f, 0.08f);
            focus_ = -1;
            digging_ = false;
            if (cleared_ >= kBits) win();
        }
    } else if (focus_ >= 0) {
        bit_[focus_].work = std::max(0.f, bit_[focus_].work - kDt * 0.4f);
    }

    if (mode_ == Mode::Play) {
        clock_--;
        if (clock_ <= 0) {
            clock_ = 0;
            lose();
        }
    }

    for (auto& d : drop_) {
        if (d.life <= 0) continue;
        d.life -= kDt;
        d.y += d.vy * kDt;
        d.vy += 40.f * kDt;
    }
}

void Game::blip(float freq, float hold) {
    blip_ = hold;
    sys_->apu.tone(0, freq, 0.16f);
}

void Game::serviceAudio() {
    if (blip_ > 0) {
        blip_ -= kDt;
        if (blip_ <= 0) sys_->apu.tone(0, 0, 0);
    }
    if (mode_ == Mode::Play && clock_ > 0 && clock_ < 12 * 60) {
        tick_ -= kDt;
        if (tick_ <= 0) {
            tick_ = 0.5f;
            sys_->apu.tone(1, clock_ < 6 * 60 ? 620.f : 390.f, 0.07f);
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

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet) {
    if (!(h > 1.5f) || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
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

void Game::vault() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int deep = y < 70 ? 1 : (y < 150 ? 2 : 1);
        v.lineBackdrop[y] = gs::rgb4(deep, deep + 1, deep + 1);
        v.lineFog[y] = y < 60 ? uint8_t(4) : 0;
        v.road[y].on = false;
    }
}

void Game::world() {
    spr(art_.mouth, 160, 58, 52, PAL_CLOCK, false, 6, true);
    spr(art_.ring, 78, 168, 150, PAL_PIPE, false, 1, true);
    spr(art_.ring, 242, 168, 150, PAL_PIPE, true, 1, true);
    spr(art_.ring, 96, 86, 90, PAL_PIPE, false, 5, true);
    spr(art_.ring, 224, 86, 90, PAL_PIPE, true, 5, true);

    float dripY = 40.f + std::fmod(t_ * 28.f, 70.f);
    spr(art_.drip, 148, dripY, 10, PAL_WATER, false, 0, false);
    spr(art_.drip, 176, std::fmod(dripY + 30.f, 80.f) + 36.f, 8, PAL_WATER, false, 2, false);

    for (int i = 0; i < kBits; i++) {
        if (bit_[i].gone) continue;
        float bob = (i == focus_ && digging_) ? std::sin(t_ * 16.f) * 1.5f : 0.f;
        spr(bitArt(art_, bit_[i].kind), bit_[i].x + bob, bit_[i].y, bitH(bit_[i].kind), bitPal(bit_[i].kind), false, 0,
            true);
    }

    int fr = int(step_) & 1;
    spr(art_.wader[fr], px_, py_, 46, PAL_WADER, face_ < 0, 0, true);
    float sx = px_ + face_ * 14.f;
    float sy = py_ - (digging_ ? 4.f : 16.f);
    spr(art_.shovel, sx, sy, digging_ ? 20.f : 28.f, PAL_TIN, face_ < 0, 0, true);

    for (auto& d : drop_) {
        if (d.life <= 0) continue;
        spr(art_.puff, d.x, d.y, 6.f + (0.5f - d.life) * 8.f, PAL_WATER, false, 0, false);
    }
}

void Game::messages() {
    char buf[48];
    int sec = clock_ / 60;
    std::snprintf(buf, sizeof buf, "%d", sec);
    text(buf, 160, 8, 0.7f, sec < 8 ? PAL_ALERT : PAL_TEXT);

    if (mode_ == Mode::Title) {
        text("S3 CULVERT CLER", 160, 72, 0.72f, PAL_TEXT);
        text("ONE CULVERT", 160, 96, 0.5f, PAL_WATER);
        text("CLEAR THE GROUND", 160, 118, 0.48f, PAL_TEXT);
        text("BEFORE THE CLOCK DIES", 160, 136, 0.42f, PAL_ALERT);
        text("HOLD TO SHOVEL", 160, 162, 0.4f, PAL_GOOD);
        if (int(t_ * 2.f) & 1) text("START", 160, 186, 0.48f, PAL_TEXT);
        return;
    }
    if (mode_ == Mode::Pause) {
        text("PAUSED", 160, 100, 0.7f, PAL_TEXT);
        return;
    }
    if (mode_ == Mode::Won) {
        text("THE GROUND IS CLEAR", 160, 96, 0.48f, PAL_GOOD);
        text("THE CULVERT IS DONE", 160, 118, 0.42f, PAL_TEXT);
        return;
    }
    if (mode_ == Mode::Lost) {
        text("THE CLOCK DIED", 160, 100, 0.52f, PAL_ALERT);
        std::snprintf(buf, sizeof buf, "LEFT %d", kBits - cleared_);
        text(buf, 160, 122, 0.42f, PAL_TEXT);
        return;
    }
    std::snprintf(buf, sizeof buf, "LEFT %d", kBits - cleared_);
    text(buf, 52, 10, 0.38f, PAL_TEXT);
    if (focus_ >= 0) {
        text(bitName(bit_[focus_].kind), 160, 206, 0.38f, digging_ ? PAL_GOOD : PAL_WATER);
    } else {
        text("CLEAR THE FLOOR", 160, 206, 0.36f, PAL_TEXT);
    }
}

void Game::draw() {
    sys_->vdp.clearSprites();
    vault();
    messages();
    world();
}

}  // namespace ccler

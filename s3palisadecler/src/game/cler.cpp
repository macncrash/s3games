#include "game/cler.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace palcler {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kSpeed = 170.f;
constexpr float kReach = 28.f;
constexpr float kRake = 0.28f;
constexpr int kClock = 36 * 60;
constexpr float kWorld = 980.f;
constexpr float kMinX = 48.f;
constexpr float kMaxX = 940.f;

struct Lay {
    float x, y;
    int kind;
};

constexpr Lay kLay[7] = {
    {160.f, 178.f, 0}, {280.f, 186.f, 1}, {390.f, 172.f, 2}, {510.f, 184.f, 3},
    {640.f, 176.f, 0}, {760.f, 188.f, 1}, {880.f, 174.f, 2},
};

const gs::Mipped& pileArt(const Art& a, int kind) {
    if (kind == 1) return a.branch;
    if (kind == 2) return a.shield;
    if (kind == 3) return a.brush;
    return a.stone;
}

int pilePal(int kind) {
    if (kind == 1 || kind == 3) return PAL_BRUSH;
    if (kind == 2) return PAL_IRON;
    return PAL_STONE;
}

float pileH(int kind) {
    if (kind == 1) return 16.f;
    if (kind == 2) return 26.f;
    if (kind == 3) return 20.f;
    return 16.f;
}

const char* pileName(int kind) {
    if (kind == 1) return "BRANCH";
    if (kind == 2) return "SHIELD";
    if (kind == 3) return "BRUSH";
    return "STONE";
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Won) return 3;
    if (mode_ == Mode::Lost) return 4;
    if (cleared_ >= 5) return 2;
    return 1;
}

float Game::rnd() {
    rng_ = rng_ * 1664525u + 1013904223u;
    return float((rng_ >> 8) & 0xffffff) / float(0x1000000);
}

void Game::resetYard() {
    for (int i = 0; i < kPiles; i++) {
        pile_[i].x = kLay[i].x;
        pile_[i].y = kLay[i].y;
        pile_[i].kind = kLay[i].kind;
        pile_[i].work = 0;
        pile_[i].gone = false;
    }
    for (auto& m : mote_) m.life = 0;
    px_ = 70.f;
    face_ = 1.f;
    step_ = 0;
    cleared_ = 0;
    focus_ = -1;
    raking_ = false;
    clock_ = kClock;
    cam_ = 0;
}

void Game::toTitle() {
    over_ = false;
    won_ = false;
    reason_ = "";
    raking_ = false;
    blip_ = 0;
    tick_ = 0;
    resetYard();
    mode_ = Mode::Title;
}

void Game::begin() {
    resetYard();
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

int Game::focusPile() const {
    int best = -1;
    float bd = kReach + 0.01f;
    for (int i = 0; i < kPiles; i++) {
        if (pile_[i].gone) continue;
        float d = std::fabs(pile_[i].x - px_);
        if (d < bd) {
            bd = d;
            best = i;
        }
    }
    return best;
}

void Game::botInput(float& ix, bool& hold) {
    ix = 0;
    hold = false;
    int id = 0;
    while (id < kPiles && pile_[id].gone) id++;
    if (id >= kPiles) return;
    float dx = pile_[id].x - px_;
    hold = std::fabs(dx) <= kReach;
    if (std::fabs(dx) < 1.2f) {
        px_ = pile_[id].x;
        return;
    }
    ix = dx > 0 ? 1.f : -1.f;
}

void Game::humanInput(float& ix, bool& hold) {
    const gs::Pad& p = sys_->pad;
    ix = 0;
    if (p.down(gs::BTN_LEFT)) ix -= 1.f;
    if (p.down(gs::BTN_RIGHT)) ix += 1.f;
    if (std::fabs(p.axisX) > 0.25f) ix = p.axisX > 0 ? 1.f : -1.f;
    hold = p.down(gs::BTN_A) || p.down(gs::BTN_B) || p.down(gs::BTN_C) || p.down(gs::BTN_X) || p.down(gs::BTN_Y) ||
           p.down(gs::BTN_Z);
}

void Game::puff(float x, float y) {
    for (int n = 0; n < 5; n++) {
        for (auto& m : mote_) {
            if (m.life > 0) continue;
            m.x = x;
            m.y = y;
            m.vx = (rnd() - 0.5f) * 70.f;
            m.vy = -16.f - rnd() * 24.f;
            m.life = 0.3f + rnd() * 0.25f;
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
    blip(80.f, 0.3f);
}

void Game::updatePlay() {
    float ix = 0;
    bool hold = false;
    if (bot_) botInput(ix, hold);
    else humanInput(ix, hold);

    if (ix != 0.f) face_ = ix < 0 ? -1.f : 1.f;
    px_ = std::clamp(px_ + ix * kSpeed * kDt, kMinX, kMaxX);
    if (ix != 0.f) step_ += kDt * 8.f;

    focus_ = focusPile();
    raking_ = false;
    if (focus_ >= 0 && hold) {
        raking_ = true;
        pile_[focus_].work += kDt;
        if (pile_[focus_].work >= kRake) {
            pile_[focus_].gone = true;
            cleared_++;
            puff(pile_[focus_].x, pile_[focus_].y);
            blip(280.f + float(cleared_) * 36.f, 0.07f);
            focus_ = -1;
            raking_ = false;
            if (cleared_ >= kPiles) win();
        }
    } else if (focus_ >= 0) {
        pile_[focus_].work = std::max(0.f, pile_[focus_].work - kDt * 0.4f);
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

    float want = px_ - 150.f;
    cam_ = std::clamp(want, 0.f, kWorld - float(gs::SCREEN_W));
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
            sys_->apu.tone(1, clock_ < 5 * 60 ? 620.f : 390.f, 0.07f);
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

void Game::spr(const gs::Mipped& m, float sx, float sy, float h, int pal, bool flip, bool feet, bool shadow) {
    if (!(h > 1.5f) || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::lround(sx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? sy - s.h : sy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
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
        spr(g, x + w * 0.5f, y, h, pal, false, false);
        x += w + gap;
    }
}

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t c;
        if (y < 40) c = gs::rgb4(2, 1, 5);
        else if (y < 90) c = gs::rgb4(6, 3, 4);
        else if (y < 130) c = gs::rgb4(10, 5, 3);
        else c = gs::rgb4(4, 5, 2);
        v.lineBackdrop[y] = c;
        v.lineFog[y] = 0;
        gs::RoadLine& r = v.road[y];
        r = {};
        if (y >= 150) {
            r.on = true;
            r.cx = 160.f;
            r.hw = 170.f;
            r.v = float(y) * 0.4f + cam_ * 0.02f;
            r.pal = 12;
            r.band = uint8_t((y / 8) & 1);
            r.style = gs::ROAD_MUD;
            r.left = gs::GROUND_LAND;
            r.right = gs::GROUND_LAND;
        }
    }
    v.roadTime = int(t_ * 20.f);
}

void Game::world() {
    float gateX = 40.f - cam_;
    spr(art_.gate, gateX, 168, 96, PAL_TIMBER, false, true);
    for (float wx = 80.f; wx < kWorld; wx += 46.f) {
        float sx = wx - cam_;
        if (sx < -20.f || sx > 340.f) continue;
        float h = 78.f + std::sin(wx * 0.07f) * 6.f;
        spr(art_.stake, sx, 162, h, PAL_TIMBER, false, true);
    }
    for (float wx = 60.f; wx < kWorld; wx += 70.f) {
        float sx = wx - cam_;
        if (sx < -30.f || sx > 350.f) continue;
        spr(art_.ditch, sx, 200, 10, PAL_STONE, false, true);
    }

    for (int i = 0; i < kPiles; i++) {
        if (pile_[i].gone) continue;
        float sx = pile_[i].x - cam_;
        float bob = (i == focus_ && raking_) ? std::sin(t_ * 16.f) * 2.f : 0.f;
        spr(pileArt(art_, pile_[i].kind), sx + bob, pile_[i].y, pileH(pile_[i].kind), pilePal(pile_[i].kind), false,
            true);
    }

    float hx = px_ - cam_;
    int fr = int(step_) & 1;
    spr(art_.ward[fr], hx, 186, 46, PAL_WARD, face_ < 0, true);
    float rx = hx + face_ * (raking_ ? 18.f : 12.f);
    spr(art_.rake, rx, raking_ ? 178.f : 168.f, raking_ ? 14.f : 16.f, PAL_TIMBER, face_ < 0, true);

    for (auto& m : mote_) {
        if (m.life <= 0) continue;
        spr(art_.mote, m.x - cam_, m.y, 6.f + (0.45f - m.life) * 8.f, PAL_STONE, false, false);
    }
}

void Game::messages() {
    char buf[48];
    int sec = clock_ / 60;
    std::snprintf(buf, sizeof buf, "%d", sec);
    text(buf, 160, 6, 0.7f, sec < 8 ? PAL_ALERT : PAL_HUD);

    if (mode_ == Mode::Title) {
        text("S3 PALISADE CLER", 160, 70, 0.72f, PAL_HUD);
        text("ONE PALISADE", 160, 94, 0.48f, PAL_DUSK);
        text("CLEAR THE GROUND", 160, 116, 0.46f, PAL_HUD);
        text("BEFORE THE CLOCK DIES", 160, 134, 0.4f, PAL_ALERT);
        text("HOLD TO RAKE", 160, 160, 0.4f, PAL_BRUSH);
        if (int(t_ * 2.f) & 1) text("START", 160, 184, 0.48f, PAL_HUD);
        return;
    }
    if (mode_ == Mode::Pause) {
        text("PAUSED", 160, 100, 0.7f, PAL_HUD);
        return;
    }
    if (mode_ == Mode::Won) {
        text("THE GROUND IS CLEAR", 160, 92, 0.48f, PAL_BRUSH);
        text("THE PALISADE IS DONE", 160, 114, 0.4f, PAL_HUD);
        return;
    }
    if (mode_ == Mode::Lost) {
        text("THE CLOCK DIED", 160, 96, 0.52f, PAL_ALERT);
        std::snprintf(buf, sizeof buf, "LEFT %d", kPiles - cleared_);
        text(buf, 160, 118, 0.42f, PAL_HUD);
        return;
    }
    std::snprintf(buf, sizeof buf, "LEFT %d", kPiles - cleared_);
    text(buf, 42, 8, 0.38f, PAL_HUD);
    if (focus_ >= 0) text(pileName(pile_[focus_].kind), 160, 200, 0.38f, raking_ ? PAL_BRUSH : PAL_DUSK);
    else text("RAKE THE YARD", 160, 200, 0.36f, PAL_HUD);
}

void Game::draw() {
    sys_->vdp.clearSprites();
    sky();
    messages();
    world();
}

}  // namespace palcler

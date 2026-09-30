#include "game/granary.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace gcler {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kSpeed = 176.f;
constexpr float kReach = 24.f;
constexpr float kScoop = 0.20f;
constexpr int kClock = 36 * 60;
constexpr float kL = 88.f, kR = 232.f, kT = 72.f, kB = 198.f;

struct Lay {
    float x, y;
    int kind;
};

constexpr Lay kLay[6] = {
    {122.f, 176.f, 0}, {196.f, 164.f, 1}, {112.f, 138.f, 2},
    {204.f, 120.f, 3}, {136.f, 98.f, 0},  {184.f, 82.f, 1},
};

float dist(float x0, float y0, float x1, float y1) {
    float dx = x1 - x0, dy = y1 - y0;
    return std::sqrt(dx * dx + dy * dy);
}

const gs::Mipped& pileArt(const Art& a, int kind) {
    if (kind == 1) return a.spill;
    if (kind == 2) return a.bale;
    if (kind == 3) return a.chaff;
    return a.sack;
}

int pilePal(int kind) {
    if (kind == 1) return PAL_GRAIN;
    if (kind == 2) return PAL_STRAW;
    if (kind == 3) return PAL_GRAIN;
    return PAL_SACK;
}

float pileH(int kind) {
    if (kind == 1) return 14.f;
    if (kind == 2) return 20.f;
    if (kind == 3) return 18.f;
    return 26.f;
}

const char* pileName(int kind) {
    if (kind == 1) return "SPILL";
    if (kind == 2) return "BALE";
    if (kind == 3) return "CHAFF";
    return "SACK";
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
    for (int i = 0; i < kPiles; i++) {
        pile_[i].x = kLay[i].x;
        pile_[i].y = kLay[i].y;
        pile_[i].kind = kLay[i].kind;
        pile_[i].work = 0;
        pile_[i].gone = false;
    }
    for (auto& m : mote_) m.life = 0;
    px_ = 160.f;
    py_ = 198.f;
    face_ = 1.f;
    step_ = 0;
    cleared_ = 0;
    focus_ = -1;
    scooping_ = false;
    clock_ = kClock;
    shake_ = 0;
}

void Game::toTitle() {
    over_ = false;
    won_ = false;
    reason_ = "";
    scooping_ = false;
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
    blip(196.f, 0.09f);
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
        float d = dist(px_, py_, pile_[i].x, pile_[i].y);
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
    while (id < kPiles && pile_[id].gone) id++;
    if (id >= kPiles) return;
    float dx = pile_[id].x - px_, dy = pile_[id].y - py_;
    float d = std::sqrt(dx * dx + dy * dy);
    hold = d <= kReach;
    if (d < 1.1f) {
        px_ = std::clamp(pile_[id].x, kL, kR);
        py_ = std::clamp(pile_[id].y, kT, kB);
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
    for (int n = 0; n < 5; n++) {
        for (auto& m : mote_) {
            if (m.life > 0) continue;
            m.x = x;
            m.y = y - 6.f;
            m.vx = (rnd() - 0.5f) * 56.f;
            m.vy = -18.f - rnd() * 36.f;
            m.life = 0.32f + rnd() * 0.22f;
            break;
        }
    }
}

void Game::win() {
    mode_ = Mode::Won;
    won_ = true;
    over_ = true;
    reason_ = "THE GROUND IS CLEAR";
    blip(494.f, 0.2f);
}

void Game::lose() {
    mode_ = Mode::Lost;
    won_ = false;
    over_ = true;
    reason_ = "THE CLOCK DIED";
    blip(82.f, 0.3f);
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

    focus_ = focusPile();
    scooping_ = false;
    if (focus_ >= 0 && hold) {
        scooping_ = true;
        pile_[focus_].work += kDt;
        if (pile_[focus_].work >= kScoop) {
            pile_[focus_].gone = true;
            cleared_++;
            puff(pile_[focus_].x, pile_[focus_].y);
            shake_ = 0.08f;
            blip(300.f + float(cleared_) * 36.f, 0.07f);
            focus_ = -1;
            scooping_ = false;
            if (cleared_ >= kPiles) win();
        }
    } else if (focus_ >= 0) {
        pile_[focus_].work = std::max(0.f, pile_[focus_].work - kDt * 0.45f);
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
        m.vy += 40.f * kDt;
    }
    if (shake_ > 0) shake_ -= kDt;
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
    if (mode_ == Mode::Play && clock_ > 0 && clock_ < 10 * 60) {
        tick_ -= kDt;
        if (tick_ <= 0) {
            tick_ = 0.5f;
            sys_->apu.tone(1, clock_ < 5 * 60 ? 620.f : 392.f, 0.07f);
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
    s.x = int16_t(std::lround(cx - s.w * 0.5f + (shake_ > 0 ? std::sin(t_ * 42.f) * 2.f : 0.f)));
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

void Game::loft() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t c;
        if (y < 36) c = gs::rgb4(3, 2, 1);
        else if (y < 70) c = gs::rgb4(5, 3, 1);
        else if (y < 150) c = gs::rgb4(6, 4, 2);
        else c = gs::rgb4(4, 3, 1);
        v.lineBackdrop[y] = c;
        v.lineFog[y] = y < 40 ? 4 : 0;
        v.road[y].on = false;
    }
}

void Game::world() {
    spr(art_.loft, 80, 58, 26, PAL_BEAM, false, 2, true);
    spr(art_.loft, 160, 54, 28, PAL_BEAM, false, 1, true);
    spr(art_.loft, 240, 58, 26, PAL_BEAM, true, 2, true);
    spr(art_.post, 36, 200, 168, PAL_TIMBER, false, 1, true);
    spr(art_.post, 284, 200, 168, PAL_TIMBER, true, 1, true);
    spr(art_.clock, 160, 42, 24, PAL_TEXT, false, 0, true);

    for (int i = 0; i < kPiles; i++) {
        if (pile_[i].gone) continue;
        float bob = (i == focus_ && scooping_) ? std::sin(t_ * 16.f) * 1.5f : 0.f;
        spr(art_.shadow, pile_[i].x, pile_[i].y + 2, 8, PAL_FLOOR, false, 0, true, true);
        spr(pileArt(art_, pile_[i].kind), pile_[i].x + bob, pile_[i].y, pileH(pile_[i].kind), pilePal(pile_[i].kind),
            false, 0, true);
    }

    int fr = int(step_) & 1;
    spr(art_.shadow, px_, py_ + 2, 10, PAL_FLOOR, false, 0, true, true);
    spr(art_.keeper[fr], px_, py_, 50, PAL_KEEPER, face_ < 0, 0, true);
    float sx = px_ + face_ * 18.f;
    float sy = py_ - (scooping_ ? 4.f : 16.f);
    spr(art_.scoop, sx, sy, scooping_ ? 20.f : 30.f, PAL_TIMBER, face_ < 0, 0, true);

    for (auto& m : mote_) {
        if (m.life <= 0) continue;
        spr(art_.mote, m.x, m.y, 6.f + (0.45f - m.life) * 12.f, PAL_GRAIN, false, 0, false);
    }
}

void Game::messages() {
    char buf[48];
    int sec = clock_ / 60;
    std::snprintf(buf, sizeof buf, "%d", sec);
    text(buf, 160, 6, 0.7f, sec < 8 ? PAL_ALERT : PAL_TEXT);

    if (mode_ == Mode::Title) {
        text("S3 GRANARY CLER", 160, 78, 0.78f, PAL_TEXT);
        text("ONE GRANARY", 160, 100, 0.52f, PAL_GRAIN);
        text("CLEAR THE GROUND", 160, 122, 0.48f, PAL_TEXT);
        text("BEFORE THE CLOCK DIES", 160, 140, 0.42f, PAL_ALERT);
        text("HOLD TO SCOOP", 160, 166, 0.4f, PAL_GOOD);
        if (int(t_ * 2.f) & 1) text("START", 160, 188, 0.48f, PAL_TEXT);
        return;
    }
    if (mode_ == Mode::Pause) {
        text("PAUSED", 160, 100, 0.7f, PAL_TEXT);
        return;
    }
    if (mode_ == Mode::Won) {
        text("THE GROUND IS CLEAR", 160, 96, 0.48f, PAL_GOOD);
        text("THE GRANARY IS DONE", 160, 118, 0.4f, PAL_TEXT);
        return;
    }
    if (mode_ == Mode::Lost) {
        text("THE CLOCK DIED", 160, 100, 0.52f, PAL_ALERT);
        std::snprintf(buf, sizeof buf, "LEFT %d", kPiles - cleared_);
        text(buf, 160, 122, 0.42f, PAL_TEXT);
        return;
    }
    std::snprintf(buf, sizeof buf, "LEFT %d", kPiles - cleared_);
    text(buf, 52, 8, 0.4f, PAL_TEXT);
    if (focus_ >= 0) {
        text(pileName(pile_[focus_].kind), 160, 186, 0.4f, scooping_ ? PAL_GOOD : PAL_GRAIN);
    } else {
        text("CLEAR THE FLOOR", 160, 186, 0.36f, PAL_TEXT);
    }
}

void Game::draw() {
    sys_->vdp.clearSprites();
    loft();
    messages();
    world();
}

}  // namespace gcler

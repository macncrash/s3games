#include "game/lot.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace lotcler {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kSpeed = 150.f;
constexpr float kReach = 26.f;
constexpr float kBrush = 0.26f;
constexpr int kClock = 28 * 60;
constexpr float kL = 48.f, kR = 272.f, kT = 78.f, kB = 204.f;

struct Spot {
    float x, y;
    int kind;
};

constexpr Spot kSpot[7] = {
    {78.f, 168.f, 0}, {132.f, 150.f, 1}, {196.f, 172.f, 2}, {246.f, 140.f, 3},
    {210.f, 104.f, 4}, {140.f, 96.f, 5},  {84.f, 118.f, 6},
};

float dist(float x0, float y0, float x1, float y1) {
    float dx = x1 - x0, dy = y1 - y0;
    return std::sqrt(dx * dx + dy * dy);
}

const gs::Mipped& pileArt(const Art& a, int kind) {
    switch (kind) {
    case 1: return a.drum;
    case 2: return a.pallet;
    case 3: return a.leaves;
    case 4: return a.cone;
    case 5: return a.sack;
    case 6: return a.plank;
    default: return a.tire;
    }
}

int pilePal(int kind) {
    switch (kind) {
    case 1: return PAL_DRUM;
    case 2: return PAL_WOOD;
    case 3: return PAL_LEAF;
    case 4: return PAL_CONE;
    case 5: return PAL_SACK;
    case 6: return PAL_WOOD;
    default: return PAL_TIRE;
    }
}

float pileH(int kind) {
    switch (kind) {
    case 1: return 28.f;
    case 2: return 16.f;
    case 3: return 14.f;
    case 4: return 30.f;
    case 5: return 24.f;
    case 6: return 12.f;
    default: return 20.f;
    }
}

const char* pileName(int kind) {
    switch (kind) {
    case 1: return "DRUM";
    case 2: return "PALLET";
    case 3: return "LEAVES";
    case 4: return "CONE";
    case 5: return "SACK";
    case 6: return "PLANK";
    default: return "TIRE";
    }
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

void Game::layLot() {
    for (int i = 0; i < kPiles; i++) {
        pile_[i].x = kSpot[i].x;
        pile_[i].y = kSpot[i].y;
        pile_[i].kind = kSpot[i].kind;
        pile_[i].work = 0;
        pile_[i].gone = false;
    }
    for (auto& d : dust_) d.life = 0;
    mx_ = 160.f;
    my_ = 198.f;
    face_ = 1.f;
    spin_ = 0;
    cleared_ = 0;
    aim_ = -1;
    brushing_ = false;
    clock_ = kClock;
    shake_ = 0;
}

void Game::toTitle() {
    over_ = false;
    won_ = false;
    reason_ = "";
    brushing_ = false;
    chirp_ = 0;
    tick_ = 0;
    layLot();
    mode_ = Mode::Title;
}

void Game::begin() {
    layLot();
    over_ = false;
    won_ = false;
    reason_ = "";
    mode_ = Mode::Play;
    chirp(180.f, 0.08f);
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

int Game::nearestPile() const {
    int best = -1;
    float bd = 1e9f;
    for (int i = 0; i < kPiles; i++) {
        if (pile_[i].gone) continue;
        float d = dist(mx_, my_, pile_[i].x, pile_[i].y);
        if (d < bd) {
            bd = d;
            best = i;
        }
    }
    return best;
}

void Game::steerBot(float& ix, float& iy, bool& sweep) {
    ix = iy = 0;
    sweep = false;
    int id = nearestPile();
    if (id < 0) return;
    float dx = pile_[id].x - mx_, dy = pile_[id].y - my_;
    float d = std::sqrt(dx * dx + dy * dy);
    sweep = d <= kReach;
    if (d < 1.2f) return;
    ix = dx / d;
    iy = dy / d;
}

void Game::steerHuman(float& ix, float& iy, bool& sweep) {
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
    sweep = p.down(gs::BTN_A) || p.down(gs::BTN_B) || p.down(gs::BTN_C) || p.down(gs::BTN_X) || p.down(gs::BTN_Y) ||
            p.down(gs::BTN_Z);
}

void Game::kickDust(float x, float y) {
    for (int n = 0; n < 5; n++) {
        for (auto& d : dust_) {
            if (d.life > 0) continue;
            d.x = x;
            d.y = y;
            d.vx = (rnd() - 0.5f) * 70.f;
            d.vy = -16.f - rnd() * 36.f;
            d.life = 0.3f + rnd() * 0.25f;
            break;
        }
    }
}

void Game::win() {
    mode_ = Mode::Won;
    won_ = true;
    over_ = true;
    reason_ = "THE GROUND IS CLEAR";
    chirp(560.f, 0.2f);
}

void Game::lose() {
    mode_ = Mode::Lost;
    won_ = false;
    over_ = true;
    reason_ = "THE CLOCK DIED";
    chirp(80.f, 0.3f);
}

void Game::tickPlay() {
    float ix, iy;
    bool sweep = false;
    if (bot_) steerBot(ix, iy, sweep);
    else steerHuman(ix, iy, sweep);

    if (ix != 0.f) face_ = ix < 0 ? -1.f : 1.f;
    float sp = std::sqrt(ix * ix + iy * iy);
    mx_ = std::clamp(mx_ + ix * kSpeed * kDt, kL, kR);
    my_ = std::clamp(my_ + iy * kSpeed * kDt, kT, kB);
    if (sp > 0.12f || sweep) spin_ += kDt * (sweep ? 14.f : 6.f);

    aim_ = -1;
    float near = kReach;
    for (int i = 0; i < kPiles; i++) {
        if (pile_[i].gone) continue;
        float d = dist(mx_, my_, pile_[i].x, pile_[i].y);
        if (d < near) {
            near = d;
            aim_ = i;
        }
    }

    brushing_ = false;
    if (aim_ >= 0 && sweep) {
        brushing_ = true;
        pile_[aim_].work += kDt;
        if (pile_[aim_].work >= kBrush) {
            pile_[aim_].gone = true;
            cleared_++;
            kickDust(pile_[aim_].x, pile_[aim_].y);
            shake_ = 0.08f;
            chirp(300.f + float(cleared_) * 36.f, 0.07f);
            aim_ = -1;
            brushing_ = false;
            if (cleared_ >= kPiles) win();
        }
    } else if (aim_ >= 0) {
        pile_[aim_].work = std::max(0.f, pile_[aim_].work - kDt * 0.4f);
    }

    if (mode_ == Mode::Play) {
        clock_--;
        if (clock_ <= 0) {
            clock_ = 0;
            lose();
        }
    }

    for (auto& d : dust_) {
        if (d.life <= 0) continue;
        d.life -= kDt;
        d.x += d.vx * kDt;
        d.y += d.vy * kDt;
    }
    if (shake_ > 0) shake_ -= kDt;
}

void Game::chirp(float freq, float hold) {
    chirp_ = hold;
    sys_->apu.tone(0, freq, 0.16f);
}

void Game::serviceAudio() {
    if (chirp_ > 0) {
        chirp_ -= kDt;
        if (chirp_ <= 0) sys_->apu.tone(0, 0, 0);
    }
    if (mode_ == Mode::Play && clock_ > 0 && clock_ < 8 * 60) {
        tick_ -= kDt;
        if (tick_ <= 0) {
            tick_ = 0.5f;
            sys_->apu.tone(1, clock_ < 4 * 60 ? 700.f : 420.f, 0.07f);
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
    if (mode_ == Mode::Pause) {
        if (startPressed()) mode_ = Mode::Play;
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
    tickPlay();
    serviceAudio();
    draw();
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet, bool shadow) {
    if (!(h > 1.5f) || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::lround(cx - s.w * 0.5f + (shake_ > 0 ? std::sin(t_ * 48.f) * 2.f : 0.f)));
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
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t c;
        if (y < 36) c = gs::rgb4(2, 3, 6);
        else if (y < 58) c = gs::rgb4(4, 5, 7);
        else if (y < 72) c = gs::rgb4(5, 5, 6);
        else c = gs::rgb4(4, 4, 4);
        v.lineBackdrop[y] = c;
        v.road[y].on = false;
    }
}

void Game::lot() {
    spr(art_.fence, 80, 70, 18, PAL_POLE, false, 2, true);
    spr(art_.fence, 160, 70, 18, PAL_POLE, false, 2, true);
    spr(art_.fence, 240, 70, 18, PAL_POLE, false, 2, true);
    spr(art_.pole, 36, 78, 70, PAL_POLE, false, 1, true);
    spr(art_.pole, 284, 78, 70, PAL_POLE, true, 1, true);
    spr(art_.clock, 160, 58, 22, PAL_INK, false, 0, true);

    const float stalls[] = {70.f, 110.f, 150.f, 190.f, 230.f, 270.f};
    for (float x : stalls) spr(art_.stall, x, 200, 40, PAL_YARD, false, 0, true);

    for (int i = 0; i < kPiles; i++) {
        if (pile_[i].gone) continue;
        float wob = (i == aim_ && brushing_) ? std::sin(t_ * 20.f) * 1.5f : 0.f;
        spr(art_.shadow, pile_[i].x, pile_[i].y + 2, 8, PAL_YARD, false, 0, true, true);
        spr(pileArt(art_, pile_[i].kind), pile_[i].x + wob, pile_[i].y, pileH(pile_[i].kind), pilePal(pile_[i].kind),
            false, 0, true);
    }

    int fr = int(spin_) & 1;
    spr(art_.shadow, mx_, my_ + 2, 12, PAL_YARD, false, 0, true, true);
    spr(art_.rig[fr], mx_, my_, brushing_ ? 30.f : 34.f, PAL_RIG, face_ < 0, 0, true);

    for (auto& d : dust_) {
        if (d.life <= 0) continue;
        spr(art_.dust, d.x, d.y, 7.f + (0.45f - d.life) * 12.f, PAL_DUST, false, 0, false);
    }
}

void Game::messages() {
    char buf[48];
    int sec = clock_ / 60;
    std::snprintf(buf, sizeof buf, "%d", sec);
    text(buf, 160, 6, 0.65f, sec < 8 ? PAL_HOT : PAL_INK);

    if (mode_ == Mode::Title) {
        text("S3 LOT CLER", 160, 84, 0.85f, PAL_INK);
        text("YOU HAVE THE LOT", 160, 106, 0.48f, PAL_RIG);
        text("CLEAR THE GROUND", 160, 128, 0.5f, PAL_INK);
        text("BEFORE THE CLOCK DIES", 160, 146, 0.42f, PAL_HOT);
        text("HOLD TO BRUSH", 160, 168, 0.4f, PAL_GOOD);
        if (int(t_ * 2.f) & 1) text("START", 160, 188, 0.48f, PAL_INK);
        return;
    }
    if (mode_ == Mode::Pause) {
        text("PAUSED", 160, 108, 0.7f, PAL_INK);
        return;
    }
    if (mode_ == Mode::Won) {
        text("THE GROUND IS CLEAR", 160, 100, 0.48f, PAL_GOOD);
        text("THE LOT IS DONE", 160, 122, 0.42f, PAL_INK);
        return;
    }
    if (mode_ == Mode::Lost) {
        text("THE CLOCK DIED", 160, 104, 0.52f, PAL_HOT);
        std::snprintf(buf, sizeof buf, "LEFT %d", kPiles - cleared_);
        text(buf, 160, 126, 0.44f, PAL_INK);
        return;
    }
    std::snprintf(buf, sizeof buf, "LEFT %d", kPiles - cleared_);
    text(buf, 46, 8, 0.38f, PAL_INK);
    if (aim_ >= 0) text(pileName(pile_[aim_].kind), 160, 214, 0.38f, brushing_ ? PAL_GOOD : PAL_RIG);
    else text("CLEAR THE LOT", 160, 214, 0.36f, PAL_INK);
}

void Game::draw() {
    sys_->vdp.clearSprites();
    sky();
    messages();
    lot();
}

}  // namespace lotcler

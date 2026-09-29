#include "game/cler.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace cler {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kSpeed = 168.f;
constexpr float kCarry = 120.f;
constexpr float kReach = 22.f;
constexpr int kClock = 34 * 60;
constexpr float kDeckY = 108.f;
constexpr float kL = 36.f, kR = 284.f;
constexpr float kDropL = 22.f, kDropR = 298.f;

struct Lay {
    float x;
    int kind;
};

constexpr Lay kLay[5] = {
    {78.f, 0}, {128.f, 1}, {168.f, 2}, {214.f, 0}, {252.f, 1},
};

uint16_t mixC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    auto ch = [&](int c0, int c1) { return int(std::lround(c0 + (c1 - c0) * t)); };
    return gs::rgb4(ch(ar, br), ch(ag, bg), ch(ab, bb));
}

const gs::Mipped& loadArt(const Art& a, int kind) {
    if (kind == 1) return a.barrel;
    if (kind == 2) return a.beam;
    return a.crate;
}

int loadPal(int kind) {
    if (kind == 1) return PAL_BARREL;
    if (kind == 2) return PAL_IRON;
    return PAL_CRATE;
}

float loadH(int kind) {
    if (kind == 1) return 22.f;
    if (kind == 2) return 14.f;
    return 20.f;
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Won) return 3;
    if (mode_ == Mode::Lost) return 4;
    if (cleared_ >= 3) return 2;
    return 1;
}

float Game::rnd() {
    rng_ = rng_ * 1664525u + 1013904223u;
    return float((rng_ >> 8) & 0xffffff) / float(0x1000000);
}

void Game::resetSpan() {
    for (int i = 0; i < kLoads; i++) {
        load_[i].x = kLay[i].x;
        load_[i].kind = kLay[i].kind;
        load_[i].gone = false;
    }
    for (auto& m : mote_) m.life = 0;
    px_ = 52.f;
    face_ = 1.f;
    step_ = 0;
    cleared_ = 0;
    carry_ = -1;
    shoving_ = false;
    clock_ = kClock;
    shake_ = 0;
}

void Game::toTitle() {
    over_ = false;
    won_ = false;
    reason_ = "";
    blip_ = 0;
    tick_ = 0;
    resetSpan();
    mode_ = Mode::Title;
}

void Game::begin() {
    resetSpan();
    over_ = false;
    won_ = false;
    reason_ = "";
    mode_ = Mode::Play;
    blip(220.f, 0.1f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.A.clear();
    sys.vdp.B.clear();
    sys.vdp.HUD.clear();
    buildArt(sys.vdp, art_);
    t_ = 0;
    toTitle();
    if (bot_) begin();
}

bool Game::startPressed() const {
    const gs::Pad& p = sys_->pad;
    return p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C) || p.pressed(gs::BTN_TURBO);
}

int Game::nearLoad() const {
    int best = -1;
    float bd = kReach + 0.01f;
    for (int i = 0; i < kLoads; i++) {
        if (load_[i].gone) continue;
        float d = std::fabs(px_ - load_[i].x);
        if (d < bd) {
            bd = d;
            best = i;
        }
    }
    return best;
}

void Game::botInput(float& ix, bool& hold) {
    ix = 0;
    hold = carry_ >= 0;
    int id = carry_;
    if (id < 0) {
        float bd = 1e9f;
        for (int i = 0; i < kLoads; i++) {
            if (load_[i].gone) continue;
            float d = std::fabs(px_ - load_[i].x);
            if (d < bd) {
                bd = d;
                id = i;
            }
        }
    }
    if (id < 0) return;
    float tx = load_[id].x;
    if (carry_ >= 0) {
        tx = load_[id].x < 160.f ? kL : kR;
        hold = true;
    } else if (std::fabs(px_ - load_[id].x) <= kReach) {
        hold = true;
    }
    float dx = tx - px_;
    if (std::fabs(dx) < 1.2f) return;
    ix = dx < 0 ? -1.f : 1.f;
}

void Game::humanInput(float& ix, bool& hold) {
    const gs::Pad& p = sys_->pad;
    ix = 0;
    if (p.down(gs::BTN_LEFT)) ix -= 1.f;
    if (p.down(gs::BTN_RIGHT)) ix += 1.f;
    if (std::fabs(p.axisX) > 0.2f) ix = p.axisX > 0 ? 1.f : -1.f;
    hold = p.down(gs::BTN_A) || p.down(gs::BTN_B) || p.down(gs::BTN_C) || p.down(gs::BTN_X) || p.down(gs::BTN_Y) ||
           p.down(gs::BTN_Z);
}

void Game::puff(float x, float y) {
    for (int n = 0; n < 5; n++) {
        for (auto& m : mote_) {
            if (m.life > 0) continue;
            m.x = x;
            m.y = y;
            m.vx = (rnd() - 0.5f) * 40.f;
            m.vy = 30.f + rnd() * 50.f;
            m.life = 0.35f + rnd() * 0.3f;
            break;
        }
    }
}

void Game::win() {
    mode_ = Mode::Won;
    won_ = true;
    over_ = true;
    reason_ = "THE GROUND IS CLEAR";
    blip(523.f, 0.22f);
}

void Game::lose() {
    mode_ = Mode::Lost;
    won_ = false;
    over_ = true;
    reason_ = "THE CLOCK DIED";
    blip(82.f, 0.3f);
}

void Game::updatePlay() {
    float ix;
    bool hold = false;
    if (bot_) botInput(ix, hold);
    else humanInput(ix, hold);

    if (ix != 0.f) face_ = ix < 0 ? -1.f : 1.f;
    float spd = carry_ >= 0 ? kCarry : kSpeed;
    px_ = std::clamp(px_ + ix * spd * kDt, kL, kR);
    if (std::fabs(ix) > 0.15f) step_ += kDt * 8.f;

    shoving_ = false;
    if (carry_ >= 0 && (load_[carry_].gone || !hold)) {
        if (!hold) carry_ = -1;
    }
    if (carry_ < 0 && hold) {
        int n = nearLoad();
        if (n >= 0) carry_ = n;
    }
    if (carry_ >= 0 && !load_[carry_].gone) {
        shoving_ = true;
        load_[carry_].x = px_ + face_ * 18.f;
        if (load_[carry_].x < kDropL || load_[carry_].x > kDropR) {
            int id = carry_;
            float fx = load_[id].x;
            load_[id].gone = true;
            carry_ = -1;
            shoving_ = false;
            cleared_++;
            shake_ = 0.12f;
            puff(std::clamp(fx, 16.f, 304.f), kDeckY);
            blip(280.f + float(cleared_) * 50.f, 0.07f);
            if (cleared_ >= kLoads) win();
        }
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
    if (mode_ == Mode::Play && clock_ > 0 && clock_ < 12 * 60) {
        tick_ -= kDt;
        if (tick_ <= 0) {
            tick_ = 0.5f;
            sys_->apu.tone(1, clock_ < 6 * 60 ? 720.f : 370.f, 0.07f);
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
    } else if (mode_ == Mode::Play) {
        if (!bot_ && sys.pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else updatePlay();
    } else if (mode_ == Mode::Pause) {
        if (sys.pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
    } else if (!bot_ && startPressed()) {
        toTitle();
    }
    serviceAudio();
    draw();
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet, bool shadow) {
    if (!(h > 1.5f) || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::clamp(int(std::lround(cx - s.w * 0.5f)), -500, 500));
    s.y = int16_t(std::clamp(int(std::lround(feet ? cy - s.h : cy - s.h * 0.5f)), -500, 500));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    float width = 0.f;
    for (unsigned char c : s) {
        if (c < 33 || c > 126) width += 12.f * scale;
        else width += float(art_.glyph[c - 32].w) * scale;
    }
    x -= width * 0.5f;
    for (unsigned char c : s) {
        if (c < 33 || c > 126) {
            x += 12.f * scale;
            continue;
        }
        const gs::Mipped& g = art_.glyph[c - 32];
        float gw = float(g.w) * scale;
        spr(g, x + gw * 0.5f, y, float(g.h) * scale, pal, false, 0, false, false);
        x += gw;
    }
}

void Game::river(float shx) {
    gs::VDP& v = sys_->vdp;
    uint16_t skyTop = gs::rgb4(3, 5, 10);
    uint16_t skyHor = gs::rgb4(10, 11, 12);
    if (mode_ == Mode::Lost) skyHor = gs::rgb4(9, 4, 3);
    else if (mode_ == Mode::Won) skyHor = gs::rgb4(8, 12, 8);
    v.setFogColor(gs::rgb4(2, 5, 8));
    v.roadTime = int(t_ * 60.f);
    constexpr float kHorizon = 118.f;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        gs::RoadLine& rd = v.road[y];
        if (y < int(kHorizon)) {
            rd.on = false;
            float u = float(y) / kHorizon;
            v.lineBackdrop[y] = mixC(skyTop, skyHor, u);
            v.lineFog[y] = 0;
            continue;
        }
        float tt = (float(y) + 0.5f - kHorizon) / (float(gs::SCREEN_H) - kHorizon);
        tt = std::max(tt, 0.04f);
        rd.on = true;
        rd.cx = 160.f + shx;
        rd.hw = 200.f;
        rd.v = t_ * 18.f + float(y) * 0.35f;
        rd.pal = uint8_t(PAL_ROAD);
        rd.style = 2;
        rd.band = (int(std::floor(rd.v * 0.2f)) & 1) ? 1 : 0;
        rd.left = gs::GROUND_WATER;
        rd.right = gs::GROUND_WATER;
        v.lineFog[y] = 0;
        v.lineBackdrop[y] = gs::rgb4(1, 3, 6);
    }
}

void Game::messages() {
    int pal = mode_ == Mode::Lost ? PAL_ALERT : mode_ == Mode::Won ? PAL_GOOD : PAL_GOLD;
    if (mode_ == Mode::Title) {
        text("S3 VIADUCT CLER", 160.f, 22.f, 0.48f, PAL_GOLD);
        text("CLEAR THE GROUND", 160.f, 46.f, 0.36f, PAL_TEXT);
        text("BEFORE THE CLOCK DIES", 160.f, 62.f, 0.3f, PAL_TEXT);
        text("HOLD TO SHOVE IT OFF THE SPAN", 160.f, 208.f, 0.26f, PAL_TEXT);
    } else if (mode_ == Mode::Won) {
        text("THE GROUND IS CLEAR", 160.f, 24.f, 0.38f, PAL_GOOD);
    } else if (mode_ == Mode::Lost) {
        text("THE CLOCK DIED", 160.f, 24.f, 0.4f, PAL_ALERT);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160.f, 24.f, 0.42f, PAL_GOLD);
    } else {
        char buf[16];
        int sec = clock_ / 60;
        std::snprintf(buf, sizeof buf, "%02d", sec);
        text(buf, 280.f, 16.f, 0.48f, clock_ < 10 * 60 ? PAL_ALERT : PAL_CLOCK);
        std::snprintf(buf, sizeof buf, "%d LEFT", kLoads - cleared_);
        text(buf, 48.f, 16.f, 0.34f, pal);
        if (shoving_) text("SHOVE", px_, kDeckY - 52.f, 0.26f, PAL_GOLD);
    }
}

void Game::draw() {
    float shx = 0;
    if (shake_ > 0) shx = std::sin(t_ * 48.f) * 2.f;
    sys_->vdp.clearSprites();
    messages();

    int fr = int(step_) & 1;
    spr(art_.shadow, px_ + shx, kDeckY + 2.f, 10.f, PAL_FX, false, 0, false, true);
    spr(art_.walker[fr], px_ + shx, kDeckY, 40.f, PAL_FIGURE, face_ < 0, 0, true, false);
    spr(art_.pole, px_ + face_ * 16.f + shx, kDeckY - 18.f, 8.f, PAL_IRON, face_ < 0, 0, false, false);
    spr(art_.clock, 280.f, 48.f, 26.f, PAL_CLOCK, false, 0, false, false);

    for (int i = 0; i < kLoads; i++) {
        if (load_[i].gone) continue;
        const Load& p = load_[i];
        spr(art_.shadow, p.x + shx, kDeckY + 2.f, 8.f, PAL_FX, false, 0, false, true);
        spr(loadArt(art_, p.kind), p.x + shx, kDeckY, loadH(p.kind), loadPal(p.kind), false, 0, true, false);
    }
    for (auto& m : mote_) {
        if (m.life <= 0) continue;
        spr(art_.dust, m.x, m.y, 6.f + m.life * 10.f, PAL_FX, false, 0, false, false);
    }

    for (int i = 0; i < 5; i++) {
        float x = 16.f + float(i) * 72.f + shx;
        spr(art_.deck, x, kDeckY + 16.f, 20.f, PAL_DECK, false, 0, false, false);
    }
    const float piers[4] = {40.f, 120.f, 200.f, 280.f};
    for (float x : piers) spr(art_.pier, x + shx, 196.f, 78.f, PAL_PIER, false, 0, true, false);
    spr(art_.lamp, 24.f + shx, kDeckY, 28.f, PAL_LAMP, false, 0, true, false);
    spr(art_.lamp, 296.f + shx, kDeckY, 28.f, PAL_LAMP, true, 0, true, false);

    river(shx);
}

}  // namespace cler

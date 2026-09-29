#include "game/plat.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace craneplat {
namespace {

constexpr float RAIL_Y = 36.f;
constexpr float PLAT_X = 240.f;
constexpr float PLAT_HALF = 36.f;
constexpr float DECK_Y = 168.f;
constexpr float CRATE_H = 20.f;
constexpr float HOOK_DROP = 8.f;
constexpr float TX_MIN = 48.f;
constexpr float TX_MAX = 286.f;
constexpr float LEN_MIN = 36.f;
constexpr float LEN_MAX = 150.f;
constexpr float CREW_TIME = 24.f;
constexpr float G = 320.f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::hookAt(float& x, float& y) const {
    x = tx_ + std::sin(th_) * len_;
    y = RAIL_Y + std::cos(th_) * len_;
}

void Game::crateAt(float& x, float& top, float& bot) const {
    float hx, hy;
    hookAt(hx, hy);
    x = hx;
    top = hy + HOOK_DROP;
    bot = top + CRATE_H;
}

bool Game::overPlatform(float x) const { return std::fabs(x - PLAT_X) <= PLAT_HALF - 8.f; }

bool Game::isLevel() const {
    float x, top, bot;
    crateAt(x, top, bot);
    if (!overPlatform(x)) return false;
    if (std::fabs(bot - DECK_Y) > 3.5f) return false;
    if (std::fabs(th_) > 0.07f) return false;
    if (std::fabs(om_) > 0.35f) return false;
    if (std::fabs(vx_) > 18.f) return false;
    return true;
}

bool Game::startPressed() const {
    const gs::Pad& p = sys_->pad;
    return p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C) || p.pressed(gs::BTN_Z);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.A.clear();
    sys.vdp.HUD.clear();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        sys.vdp.lineBackdrop[y] = gs::rgb4(3, 5, 9);
        sys.vdp.lineFog[y] = 0;
        sys.vdp.road[y].on = false;
    }
    sys.vdp.setFogColor(gs::rgb4(3, 5, 9));
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    if (bot_) begin();
}

void Game::begin() {
    tx_ = 78.f;
    vx_ = 0.f;
    len_ = 52.f;
    th_ = 0.22f;
    om_ = 0.f;
    crew_ = CREW_TIME;
    hold_ = 0.f;
    won_ = false;
    over_ = false;
    why_ = "the other crew took the platform";
    hint_ = "STOP LEVEL WITH THE PLATFORM";
    mode_ = Mode::Shift;
    sys_->apu.tone(0, 220.f, 0.05f);
}

void Game::botPlan(float& ax, float& hoist, bool& act) const {
    float x, top, bot;
    crateAt(x, top, bot);
    const float targetLen = DECK_Y - RAIL_Y - HOOK_DROP - CRATE_H;
    float wantX = PLAT_X - std::sin(th_) * len_;
    ax = clampf((wantX - tx_) * 0.08f - vx_ * 0.04f, -1.f, 1.f);
    bool near = std::fabs(x - PLAT_X) < 18.f && std::fabs(th_) < 0.28f;
    float wantLen = near ? targetLen : std::min(len_, 58.f);
    if (!near && len_ < 50.f) wantLen = 54.f;
    hoist = clampf((wantLen - len_) * 0.35f, -1.f, 1.f);
    if (std::fabs(th_) > 0.2f && len_ > targetLen - 8.f) hoist = std::min(hoist, 0.f);
    if (std::fabs(len_ - targetLen) < 1.2f && std::fabs(tx_ - PLAT_X) < 6.f && std::fabs(th_) < 0.05f) {
        ax = clampf(-vx_ * 0.08f, -1.f, 1.f);
        hoist = 0.f;
    }
    act = isLevel();
}

void Game::update(float dt) {
    float ax = 0.f, hoist = 0.f;
    bool act = false;
    if (bot_) {
        botPlan(ax, hoist, act);
    } else {
        const gs::Pad& p = sys_->pad;
        if (p.down(gs::BTN_LEFT)) ax -= 1.f;
        if (p.down(gs::BTN_RIGHT)) ax += 1.f;
        if (std::fabs(p.axisX) > 0.15f) ax = p.axisX;
        if (p.down(gs::BTN_UP)) hoist -= 1.f;
        if (p.down(gs::BTN_DOWN)) hoist += 1.f;
        if (std::fabs(p.axisY) > 0.15f) hoist = -p.axisY;
        act = p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C) || p.pressed(gs::BTN_Z);
    }

    float drive = ax * 260.f;
    vx_ += drive * dt;
    vx_ *= (std::fabs(ax) < 0.05f) ? 0.86f : 0.94f;
    tx_ += vx_ * dt;
    if (tx_ < TX_MIN) {
        tx_ = TX_MIN;
        vx_ = 0;
    }
    if (tx_ > TX_MAX) {
        tx_ = TX_MAX;
        vx_ = 0;
    }

    float use = std::max(len_, 24.f);
    float dom = -(G / use) * std::sin(th_) - (drive / use) * std::cos(th_);
    om_ += dom * dt;
    float damp = (std::fabs(ax) < 0.05f && std::fabs(hoist) < 0.05f) ? 0.90f : 0.992f;
    om_ *= damp;
    th_ += om_ * dt;

    float prev = len_;
    len_ += hoist * 42.f * dt;
    len_ = clampf(len_, LEN_MIN, LEN_MAX);
    if (len_ > 1.f && prev > 1.f) om_ *= prev / len_;

    float x, top, bot;
    crateAt(x, top, bot);
    bool over = overPlatform(x);
    if (over && bot - DECK_Y > 7.f) {
        why_ = "the load struck the platform";
        mode_ = Mode::Fail;
        over_ = true;
        sys_->apu.tone(0, 90.f, 0.12f);
        return;
    }
    if (!over && bot > 206.f) {
        why_ = "the load dropped off the span";
        mode_ = Mode::Fail;
        over_ = true;
        sys_->apu.tone(0, 90.f, 0.12f);
        return;
    }

    if (isLevel()) {
        hint_ = "LEVEL  SET THE LOAD";
        hold_ += dt;
    } else if (over) {
        hint_ = (bot < DECK_Y - 3.5f) ? "LOWER TO THE YELLOW LINE" : "TOO LOW  LIFT AND STOP";
        hold_ = 0.f;
    } else {
        hint_ = "TAKE THE CRANE TO THE PLATFORM";
        hold_ = 0.f;
    }

    if (act) {
        if (isLevel()) {
            won_ = true;
            over_ = true;
            mode_ = Mode::Win;
            th_ = 0.f;
            om_ = 0.f;
            vx_ = 0.f;
            len_ = DECK_Y - RAIL_Y - HOOK_DROP - CRATE_H;
            tx_ = PLAT_X;
            sys_->apu.tone(0, 523.f, 0.08f);
            return;
        }
        sys_->apu.tone(0, 140.f, 0.04f);
        if (!over) hint_ = "NOT OVER THE PLATFORM";
        else hint_ = "NOT LEVEL  HOLD STILL ON THE LINE";
    }

    crew_ -= dt;
    if (crew_ <= 0.f) {
        crew_ = 0.f;
        why_ = "the other crew took the platform";
        mode_ = Mode::Fail;
        over_ = true;
        sys_->apu.tone(0, 70.f, 0.14f);
    }
}

void Game::draw() {
    sys_->vdp.clearSprites();
    sys_->vdp.HUD.clear();

    float hx, hy;
    hookAt(hx, hy);
    float cx, top, bot;
    crateAt(cx, top, bot);
    float rivalU = 1.f - crew_ / CREW_TIME;
    float ry = 70.f + rivalU * 90.f;

    int links = std::max(1, int(len_ / 6.f));
    for (int i = links; i >= 0; i--) {
        float u = float(i) / float(links);
        spr(art_.link, tx_ + (hx - tx_) * u, RAIL_Y + (hy - RAIL_Y) * u, 5.f, PAL_CABLE);
    }
    spr(art_.rival, 58.f, 78.f, 34.f, PAL_RIVAL);
    spr(art_.hook, 58.f, ry, 12.f, PAL_RIVAL);
    spr(art_.lamp, 196.f, DECK_Y - 2.f, 12.f, PAL_HOOK);
    spr(art_.lamp, 284.f, DECK_Y - 2.f, 12.f, PAL_HOOK);
    spr(art_.trolley, tx_, RAIL_Y - 2.f, 22.f, PAL_CRANE);
    int cratePal = isLevel() || mode_ == Mode::Win ? PAL_GREEN : PAL_WOOD;
    if (mode_ != Mode::Win && isLevel()) cratePal = PAL_GREEN;
    spr(art_.crate, cx, top + CRATE_H * 0.5f, CRATE_H, mode_ == Mode::Win ? PAL_GREEN : (isLevel() ? PAL_GREEN : PAL_WOOD));
    spr(art_.hook, hx, hy + 4.f, 16.f, PAL_HOOK);
    (void)bot;
    (void)cratePal;

    if (mode_ == Mode::Title) {
        hudC(8, "S3 CRANEPLAT", PAL_BANNER);
        hudC(11, "TAKE THE CRANE", PAL_WHITE);
        hudC(12, "STOP LEVEL WITH THE PLATFORM", PAL_AMBER);
        hudC(14, "THE CLOCK IS THE OTHER CREW", PAL_RED);
        hudC(17, "LEFT RIGHT  UP DOWN  Z SETS", PAL_WHITE);
        hudC(20, "PRESS START", PAL_GREEN);
        return;
    }

    int secs = int(std::ceil(crew_));
    if (secs < 0) secs = 0;
    char line[48];
    std::snprintf(line, sizeof line, "OTHER CREW %02d", secs);
    int cpal = secs <= 6 ? PAL_RED : PAL_AMBER;
    hud(1, 0, "S3 CRANEPLAT", PAL_AMBER);
    hud(24, 0, line, cpal);
    int marks = int((crew_ / CREW_TIME) * 16.f + 0.5f);
    std::string bar = "CREW ";
    for (int i = 0; i < 16; i++) bar.push_back(i < marks ? '#' : '.');
    hud(1, 1, bar, cpal);

    if (mode_ == Mode::Win) {
        hudC(6, "LEVEL", PAL_GREEN);
        hudC(8, "STOPPED WITH THE PLATFORM", PAL_WHITE);
        hudC(10, "AHEAD OF THE OTHER CREW", PAL_AMBER);
        hudC(14, "PRESS START", PAL_GREEN);
    } else if (mode_ == Mode::Fail) {
        hudC(6, "THEY TOOK IT", PAL_RED);
        hudC(8, why_, PAL_WHITE);
        hudC(12, "PRESS START", PAL_AMBER);
    } else {
        hud(1, 2, hint_, isLevel() ? PAL_GREEN : PAL_WHITE);
        hud(1, 26, "LEFT RIGHT  UP DOWN  Z SETS", PAL_WHITE);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const float dt = 1.f / 60.f;
    t_ += dt;
    if (mode_ == Mode::Title) {
        th_ = std::sin(t_ * 1.3f) * 0.18f;
        tx_ = 120.f;
        len_ = 64.f;
        crew_ = CREW_TIME;
        if (startPressed() || bot_) begin();
    } else if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        if (startPressed() && !bot_) begin();
    } else {
        update(dt);
    }
    if (t_ > 0.2f) sys.apu.tone(0, 0, 0);
    draw();
}

}  // namespace craneplat

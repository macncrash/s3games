#include "game/mark.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace cranemark {
namespace {

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

void Game::hookAt(float& x, float& y) const {
    x = tx_ + std::sin(th_) * len_;
    y = RAIL_Y + std::cos(th_) * len_;
}

void Game::loadAt(float& x, float& y) const {
    float hx, hy;
    hookAt(hx, hy);
    x = hx;
    y = hy + HOOK_GAP + CRATE_H * 0.5f;
}

bool Game::startPressed() const {
    const gs::Pad& p = sys_->pad;
    return p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C);
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.07f);
    melodyT_ = 0.06f;
    melody_ = -2;
}

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

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool shadow) {
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
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (resting_ && onMark_ && hold_ > 0.12f) return 3;
    if (onMark_ && std::fabs(th_) < 0.2f) return 2;
    return 1;
}

void Game::begin() {
    tx_ = HOME_X;
    vx_ = 0;
    len_ = TRAVEL_L;
    th_ = om_ = 0;
    race_ = hold_ = dropT_ = 0;
    lowering_ = false;
    resting_ = onMark_ = onDeck_ = onPier_ = false;
    won_ = false;
    over_ = false;
    phase_ = 1;
    melody_ = -1;
    why_ = "missed the end";
    banner_ = "";
    loadX_ = HOME_X;
    loadY_ = RAIL_Y + TRAVEL_L + HOOK_GAP + CRATE_H * 0.5f;
    mode_ = Mode::Leg;
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(1, 0, 0);
}

void Game::titlePose() {
    tx_ = 150.f + std::sin(t_ * 0.6f) * 18.f;
    len_ = 48.f;
    th_ = std::sin(t_ * 0.9f) * 0.08f;
    om_ = 0;
    vx_ = 0;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    const uint16_t sky = gs::rgb4(3, 5, 10);
    for (int y = 0; y < gs::SCREEN_H; y++) sys.vdp.lineBackdrop[y] = sky;
    sys.vdp.A.enabled = false;
    sys.vdp.setFogColor(gs::rgb4(8, 8, 12));
    if (bot_) begin();
    else mode_ = Mode::Title;
}

void Game::failLeg(const char* why) {
    if (mode_ != Mode::Leg) return;
    why_ = why;
    banner_ = why;
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    phase_ = 4;
    blip(140.f);
}

void Game::winLeg() {
    if (mode_ != Mode::Leg) return;
    mode_ = Mode::Win;
    over_ = true;
    won_ = true;
    phase_ = 4;
    melody_ = 0;
    melodyT_ = 0;
    why_ = "the leg is made";
    banner_ = "SET DOWN";
}

void Game::pilot(float hx, float& ax, float& hoist) {
    ax = 0;
    hoist = 0;
    float err = MARK_X - hx;
    bool calm = std::fabs(th_) < 0.06f && std::fabs(om_) < 0.18f && std::fabs(vx_) < 8.f;
    bool lined = std::fabs(err) < 3.2f && std::fabs(th_) < 0.08f;
    if (lined && calm && len_ < placeLen() - 6.f) lowering_ = true;
    if (std::fabs(err) > 18.f || std::fabs(th_) > 0.28f) lowering_ = false;

    float goalL = lowering_ ? placeLen() + 1.5f : TRAVEL_L;
    float wantVx = clampf(err * 1.25f, -26.f, 26.f);
    wantVx += clampf(th_ * 58.f, -20.f, 20.f);
    wantVx += clampf(om_ * 18.f, -14.f, 14.f);
    if (lowering_) wantVx *= 0.65f;
    ax = clampf((wantVx - vx_) * 5.f, -48.f, 48.f);
    float downCap = lowering_ ? 14.f : 26.f;
    hoist = clampf((goalL - len_) * 2.3f, lowering_ ? -12.f : -30.f, downCap);
    if (resting_ && onMark_) {
        ax = -vx_ * 6.f;
        hoist = 4.f;
    }
}

void Game::update(float dt) {
    race_ += dt;
    float hx, hy;
    hookAt(hx, hy);
    (void)hy;

    float ax = 0, hoist = 0;
    bool act = false;
    if (mode_ == Mode::Leg && race_ > 0.05f) {
        if (bot_) pilot(hx, ax, hoist);
        else {
            const gs::Pad& pad = sys_->pad;
            float steer = std::fabs(pad.axisX) > 0.12f ? pad.axisX
                                                       : float(pad.down(gs::BTN_RIGHT)) - float(pad.down(gs::BTN_LEFT));
            if (std::fabs(steer) < 0.05f) vx_ *= std::exp(-6.f * dt);
            else ax = steer * 150.f;
            if (pad.down(gs::BTN_UP)) hoist -= 42.f;
            if (pad.down(gs::BTN_DOWN)) hoist += 42.f;
            act = pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_Z) || pad.pressed(gs::BTN_TURBO);
        }
    }

    float prevVx = vx_;
    if (bot_ || ax != 0.f) vx_ += ax * dt;
    vx_ = clampf(vx_, -110.f, 110.f);
    tx_ += vx_ * dt;
    if (tx_ < TX_MIN) {
        tx_ = TX_MIN;
        vx_ = 0;
    }
    if (tx_ > TX_MAX) {
        tx_ = TX_MAX;
        vx_ = 0;
    }
    float usedAx = (vx_ - prevVx) / std::max(dt, 0.0001f);

    len_ = clampf(len_ + hoist * dt, LEN_MIN, LEN_MAX);
    float sth = std::sin(th_);
    float cth = std::cos(th_);
    float L = std::max(len_, 16.f);
    float thAcc = -(usedAx / L) * cth - (GRAV / L) * sth;
    om_ += thAcc * dt;
    om_ *= std::exp(-1.35f * dt);
    th_ += om_ * dt;
    th_ = clampf(th_, -1.15f, 1.15f);
    om_ = clampf(om_, -4.2f, 4.2f);
    cth = std::cos(th_);

    loadAt(loadX_, loadY_);
    float bottom = loadY_ + CRATE_H * 0.5f;
    onMark_ = std::fabs(loadX_ - MARK_X) <= SET_X;
    onDeck_ = loadX_ >= SHIP_L + 4.f && loadX_ <= SHIP_R - 8.f;
    onPier_ = loadX_ >= PIER_L + 4.f && loadX_ <= PIER_R - 4.f;
    resting_ = false;

    float support = -1.f;
    if (onDeck_) support = DECK_Y;
    else if (onPier_) support = PIER_Y;
    if (support > 0.f && cth > 0.4f) {
        float maxHookY = support - CRATE_H - HOOK_GAP;
        float maxL = (maxHookY - RAIL_Y) / cth;
        if (len_ > maxL) {
            if (std::fabs(th_) > 0.72f || std::fabs(om_) > 2.6f) failLeg("the swing hit");
            else {
                len_ = maxL;
                resting_ = std::fabs(th_) < 0.28f && std::fabs(om_) < 0.85f && std::fabs(vx_) < 22.f;
                if (resting_) om_ *= std::exp(-5.f * dt);
            }
        }
    }

    bool overGap = loadX_ > PIER_R + 2.f && loadX_ < SHIP_L - 2.f;
    if (mode_ == Mode::Leg && overGap && bottom >= WATER_Y - 8.f) failLeg("in the drink");
    if (mode_ == Mode::Leg && std::fabs(th_) > 1.02f) failLeg("the load slipped");

    if (mode_ == Mode::Leg && resting_ && onMark_) {
        hold_ += dt;
        phase_ = 3;
        if (hold_ >= HOLD_NEED) winLeg();
    } else {
        hold_ = 0;
        if (mode_ == Mode::Leg) phase_ = (onMark_ && std::fabs(th_) < 0.2f) ? 2 : 1;
    }

    if (act && mode_ == Mode::Leg) {
        if (resting_ && onMark_) winLeg();
        else if (onDeck_ || onPier_) failLeg("missed the mark");
        else failLeg("dropped");
    }

    if (mode_ == Mode::Leg && race_ >= LEG_END) failLeg("missed the end");

    if (mode_ == Mode::Leg) {
        if (std::fabs(hoist) > 6.f) sys_->apu.tone(1, 64.f + len_ * 0.4f, 0.03f);
        else sys_->apu.tone(1, 0, 0);
    }
    if (melody_ == -2) {
        melodyT_ -= dt;
        if (melodyT_ <= 0.f) {
            melody_ = -1;
            sys_->apu.tone(0, 0, 0);
        }
    }
}

void Game::chime(float dt) {
    if (melody_ < 0) return;
    static const float notes[] = {392.f, 523.f, 659.f, 784.f};
    if (melody_ >= 4) return;
    if (melodyT_ <= 0.f) sys_->apu.tone(0, notes[melody_], 0.08f);
    melodyT_ += dt;
    if (melodyT_ > 0.16f) {
        melodyT_ = 0;
        melody_++;
        if (melody_ >= 4) sys_->apu.tone(0, 0, 0);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const float dt = 1.f / 60.f;
    t_ += dt;
    if (mode_ == Mode::Title) {
        if (bot_ || startPressed()) begin();
        else titlePose();
    } else if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        chime(dt);
        if (!bot_ && startPressed()) begin();
    } else {
        update(dt);
    }
    draw();
}

void Game::draw() {
    sys_->vdp.clearSprites();
    sys_->vdp.HUD.clear();

    float hx, hy;
    hookAt(hx, hy);
    float lx, ly;
    if (mode_ == Mode::Title) {
        lx = hx;
        ly = hy + HOOK_GAP + CRATE_H * 0.5f;
    } else {
        lx = loadX_;
        ly = loadY_;
    }
    int hookPal = (resting_ && onMark_) ? PAL_SET : PAL_HOOK;

    spr(art_.gull, 40.f + std::fmod(t_ * 14.f, 220.f), 70.f + std::sin(t_ * 1.4f) * 3.f, 8.f, PAL_WHITE);
    spr(art_.gull, 180.f + std::fmod(t_ * 9.f, 90.f), 86.f, 6.f, PAL_WHITE, true);
    if (mode_ == Mode::Title) spr(art_.banner, 160.f, 62.f, float(art_.banner.h), PAL_BANNER);

    spr(art_.trolley, tx_, RAIL_Y - 2.f, 28.f, (mode_ == Mode::Win) ? PAL_SET : PAL_CRANE);
    spr(art_.pennant, MARK_X - MARK_HALF - 2.f, DECK_Y - 10.f, 16.f, PAL_END);
    spr(art_.pennant, MARK_X + MARK_HALF + 2.f, DECK_Y - 10.f, 16.f, PAL_END, true);
    spr(art_.shadow, lx, ly + CRATE_H * 0.55f, 8.f, PAL_WOOD, false, true);
    spr(art_.crate, lx, ly, CRATE_H, PAL_WOOD);
    spr(art_.hook, hx, hy + 7.f, 16.f, hookPal);

    float dx = hx - tx_;
    float dy = hy - RAIL_Y;
    float dist = std::sqrt(dx * dx + dy * dy);
    int links = std::max(1, int(dist / 5.f));
    for (int i = 0; i <= links; i++) {
        float u = float(i) / float(links);
        spr(art_.link, tx_ + dx * u, RAIL_Y + dy * u, 5.f, PAL_CABLE);
    }

    if (mode_ == Mode::Title) {
        hud(2, 12, "IN THE CRANE", PAL_WHITE);
        hud(2, 13, "SET DOWN ON THE MARK", PAL_AMBER);
        hud(2, 15, "MISSING THE END FAILS THE LEG", PAL_RED);
        hud(2, 17, "LEFT RIGHT MOVES  UP DOWN HOISTS", PAL_WHITE);
        hud(2, 18, "Z SETS THE LOAD", PAL_GREEN);
        hud(24, 18, "PRESS START", PAL_GREEN);
        return;
    }

    float left = std::max(0.f, LEG_END - race_);
    char line[48];
    std::snprintf(line, sizeof line, "END %4.1f", left);
    hud(1, 0, "S3 CRANE MARK", PAL_AMBER);
    hud(26, 0, line, left < 8.f ? PAL_RED : PAL_END);

    if (mode_ == Mode::Leg) {
        int swings = int(std::fabs(th_) / 1.02f * 8.f);
        if (swings > 8) swings = 8;
        std::string bar = "SWING ";
        for (int i = 0; i < 8; i++) bar.push_back(i < swings ? '#' : '.');
        int pal = swings >= 6 ? PAL_RED : swings >= 3 ? PAL_AMBER : PAL_GREEN;
        hud(1, 1, bar, pal);
        if (resting_ && onMark_) hud(1, 2, "HOLD THE SET", PAL_GREEN);
        else if (onMark_) hud(1, 2, "OVER THE MARK", PAL_AMBER);
        else if (resting_ && onDeck_) hud(1, 2, "OFF THE MARK", PAL_RED);
        else if (resting_ && onPier_) hud(1, 2, "STILL ON THE PIER", PAL_AMBER);
        else hud(1, 2, "SET DOWN BEFORE THE END", PAL_WHITE);
        hud(1, 26, "LEFT RIGHT UP DOWN   Z SETS", PAL_WHITE);
    } else if (mode_ == Mode::Win) {
        hudC(6, "SET DOWN", PAL_GREEN);
        hudC(8, "THE LOAD IS ON THE MARK", PAL_WHITE);
        hudC(10, "THE LEG IS MADE", PAL_GREEN);
        if (!bot_) hudC(13, "PRESS START", PAL_AMBER);
    } else {
        hudC(6, "LEG FAILED", PAL_RED);
        std::string up = banner_;
        for (char& c : up)
            if (c >= 'a' && c <= 'z') c = char(c - 32);
        hudC(8, up, PAL_WHITE);
        hudC(10, "MISSING THE END FAILS THE LEG", PAL_RED);
        if (!bot_) hudC(13, "PRESS START", PAL_AMBER);
    }
}

}  // namespace cranemark

#include "game/crane.h"

#include "game/world.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace crane {
namespace {

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

void Game::hookAt(float& x, float& y) const {
    x = tx_ + std::sin(th_) * len_;
    y = RAIL_Y + std::cos(th_) * len_;
}

void Game::carryAt(float& x, float& y) const {
    float hx, hy;
    hookAt(hx, hy);
    x = hx;
    y = hy + HOOK_GAP + CRATE_H * 0.5f;
}

int Game::nextBox() const {
    for (int i = 0; i < 3; i++)
        if (crates_[i].box == Box::Wait) return i;
    return -1;
}

bool Game::startPressed() const {
    const gs::Pad& p = sys_->pad;
    return p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C);
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.07f);
    melodyT_ = 0.06f;
    melody_ = -2;  // a one-shot blip owns channel 0 until melodyT_ expires
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

void Game::begin() {
    for (int i = 0; i < 3; i++) {
        crates_[i].box = Box::Wait;
        crates_[i].x = HOME_X[i];
        crates_[i].y = DOCK_Y - CRATE_H * 0.5f;
        crates_[i].vy = 0;
    }
    carrying_ = -1;
    shipped_ = 0;
    won_ = false;
    over_ = false;
    resting_ = false;
    onMark_ = onDock_ = onShip_ = false;
    tx_ = HOME_X[0];
    vx_ = 0;
    len_ = TRAVEL_L;
    th_ = om_ = 0;
    shiftT_ = phaseT_ = dropT_ = splashT_ = 0;
    job_ = Job::Seek;
    melody_ = -1;
    why_ = "the shift ran out";
    banner_ = "";
    mode_ = Mode::Shift;
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(1, 0, 0);
}

void Game::titlePose() {
    tx_ = MARK_X;
    vx_ = 0;
    len_ = 62.f;
    th_ = std::sin(t_ * 0.9f) * 0.06f;
    om_ = 0;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    const uint16_t sky = gs::rgb4(4, 8, 13);
    for (int y = 0; y < gs::SCREEN_H; y++) sys.vdp.lineBackdrop[y] = sky;
    sys.vdp.A.enabled = false;
    sys.vdp.setFogColor(gs::rgb4(8, 12, 14));
    for (int i = 0; i < 3; i++) {
        crates_[i].box = Box::Wait;
        crates_[i].x = HOME_X[i];
        crates_[i].y = DOCK_Y - CRATE_H * 0.5f;
    }
    if (bot_) begin();
    else mode_ = Mode::Title;
}

void Game::failDrop(const char* banner) {
    if (mode_ != Mode::Shift || carrying_ < 0) return;
    Crate& c = crates_[carrying_];
    carryAt(c.x, c.y);
    c.vy = 30.f;
    c.box = Box::Fall;
    carrying_ = -1;
    resting_ = false;
    mode_ = Mode::Drop;
    dropT_ = 0;
    splashT_ = 0;
    banner_ = banner;
    why_ = "a drop ended the shift";
    sys_->apu.tone(1, 0, 0);
    sys_->apu.noiseBurst(0.5f, 640.f, 0.4f);
    sys_->rumble(0.7f, 0.35f, 200);
}

bool Game::grab() {
    if (carrying_ >= 0) return false;
    float hx, hy;
    hookAt(hx, hy);
    int best = -1;
    float bestD = 1e9f;
    for (int i = 0; i < 3; i++) {
        if (crates_[i].box != Box::Wait) continue;
        float top = crates_[i].y - CRATE_H * 0.5f;
        float dx = hx - crates_[i].x;
        float dy = hy - top;
        if (std::fabs(dx) > 15.f || std::fabs(dy) > 14.f || std::fabs(th_) > 0.45f) continue;
        float d = dx * dx + dy * dy;
        if (d < bestD) {
            bestD = d;
            best = i;
        }
    }
    if (best < 0) return false;
    crates_[best].box = Box::Carry;
    carrying_ = best;
    blip(660.f);
    sys_->rumble(0.15f, 0.25f, 40);
    return true;
}

void Game::release() {
    if (carrying_ < 0) return;
    if (resting_ && onMark_) {
        Crate& c = crates_[carrying_];
        c.box = Box::Ship;
        c.x = MARK_X;
        c.y = DECK_Y - float(shipped_) * CRATE_H - CRATE_H * 0.5f;
        c.vy = 0;
        shipped_++;
        carrying_ = -1;
        resting_ = false;
        blip(520.f);
        sys_->rumble(0.2f, 0.45f, 70);
        if (shipped_ >= 3) {
            won_ = true;
            over_ = true;
            mode_ = Mode::Win;
            melody_ = 0;
            melodyT_ = 0;
            banner_ = "SHIFT CLEAR";
            sys_->apu.tone(1, 0, 0);
        }
        return;
    }
    if (resting_ && onDock_ && !onShip_) {
        Crate& c = crates_[carrying_];
        c.box = Box::Wait;
        c.x = HOME_X[carrying_];
        c.y = DOCK_Y - CRATE_H * 0.5f;
        c.vy = 0;
        carrying_ = -1;
        resting_ = false;
        blip(320.f);
        return;
    }
    failDrop(onShip_ ? "OFF THE MARK" : "DROPPED");
}

void Game::botPlan(float hx, float& ax, float& hoist, bool& act) {
    act = false;
    ax = 0;
    hoist = 0;
    if (mode_ != Mode::Shift) return;
    int next = nextBox();
    if (next < 0 && carrying_ < 0) {
        job_ = Job::Done;
        return;
    }
    if (carrying_ >= 0 && (job_ == Job::Seek || job_ == Job::Lower)) job_ = Job::Lift;
    if (carrying_ < 0 && (job_ == Job::Lift || job_ == Job::Travel || job_ == Job::Descend)) job_ = Job::Seek;

    float goalX = MARK_X;
    float goalL = TRAVEL_L;
    const float thLim = phaseT_ > 5.f ? 0.16f : 0.08f;
    const float omLim = phaseT_ > 5.f ? 0.5f : 0.22f;
    auto calm = [&]() { return std::fabs(th_) < thLim && std::fabs(om_) < omLim && std::fabs(vx_) < 12.f; };

    if (job_ == Job::Seek && next >= 0) {
        goalX = HOME_X[next];
        goalL = TRAVEL_L;
        if (std::fabs(hx - goalX) < 4.f && std::fabs(len_ - goalL) < 3.f && calm()) {
            job_ = Job::Lower;
            phaseT_ = 0;
        }
    } else if (job_ == Job::Lower && next >= 0) {
        goalX = HOME_X[next];
        bool lined = std::fabs(hx - goalX) < 8.f && std::fabs(th_) < 0.14f;
        goalL = lined ? grabLen() + 6.f : TRAVEL_L;
        float top = DOCK_Y - CRATE_H;
        float hy = RAIL_Y + std::cos(th_) * len_;
        if (lined && std::fabs(hy - top) < 12.f && std::fabs(th_) < 0.2f) act = true;
    } else if (job_ == Job::Lift) {
        goalX = hx;
        goalL = TRAVEL_L;
        if (len_ < TRAVEL_L + 4.f && calm()) {
            job_ = Job::Travel;
            phaseT_ = 0;
        }
    } else if (job_ == Job::Travel) {
        goalX = MARK_X;
        goalL = TRAVEL_L;
        if (std::fabs(hx - MARK_X) < 4.f && std::fabs(len_ - TRAVEL_L) < 3.f && calm()) {
            job_ = Job::Descend;
            phaseT_ = 0;
        }
    } else if (job_ == Job::Descend) {
        goalX = MARK_X;
        bool lined = std::fabs(hx - MARK_X) < 7.f && std::fabs(th_) < 0.12f;
        goalL = lined ? placeLen(shipped_) + 10.f : TRAVEL_L;
        if (resting_ && onMark_) act = true;
    } else {
        goalX = tx_;
        goalL = len_;
    }

    float err = goalX - hx;
    float wantVx = clampf(err * 1.15f, -34.f, 34.f);
    wantVx += clampf(th_ * 42.f, -18.f, 18.f);
    wantVx += clampf(om_ * 14.f, -14.f, 14.f);
    ax = clampf((wantVx - vx_) * 4.f, -40.f, 40.f);
    hoist = clampf((goalL - len_) * 2.2f, job_ == Job::Descend || job_ == Job::Lower ? -18.f : -32.f, 28.f);
}

void Game::update(float dt) {
    shiftT_ += dt;
    phaseT_ += dt;
    float hx, hy;
    hookAt(hx, hy);
    (void)hy;

    float ax = 0, hoist = 0;
    bool act = false;
    if (mode_ == Mode::Shift && shiftT_ > 0.08f) {
        if (bot_) botPlan(hx, ax, hoist, act);
        else {
            const gs::Pad& pad = sys_->pad;
            float steer = std::fabs(pad.axisX) > 0.12f ? pad.axisX
                                                       : float(pad.down(gs::BTN_RIGHT)) - float(pad.down(gs::BTN_LEFT));
            if (std::fabs(steer) < 0.05f) vx_ *= std::exp(-7.f * dt);
            else ax = steer * 170.f;
            if (pad.down(gs::BTN_UP)) hoist -= 46.f;
            if (pad.down(gs::BTN_DOWN)) hoist += 46.f;
            act = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_TURBO);
        }
    }

    float prevVx = vx_;
    if (bot_ || ax != 0.f) vx_ += ax * dt;
    vx_ = clampf(vx_, -120.f, 120.f);
    tx_ += vx_ * dt;
    if (tx_ < TX_MIN) {
        tx_ = TX_MIN;
        vx_ = 0;
    }
    if (tx_ > TX_MAX) {
        tx_ = TX_MAX;
        vx_ = 0;
    }
    float usedAx = (vx_ - prevVx) / dt;

    len_ = clampf(len_ + hoist * dt, LEN_MIN, LEN_MAX);
    float sth = std::sin(th_);
    float cth = std::cos(th_);
    float L = std::max(len_, 18.f);
    float thAcc = -(usedAx / L) * cth - (GRAV / L) * sth;
    om_ += thAcc * dt;
    float damp = carrying_ >= 0 ? 1.15f : 5.f;
    om_ *= std::exp(-damp * dt);
    th_ += om_ * dt;
    th_ = clampf(th_, -1.2f, 1.2f);
    om_ = clampf(om_, -4.5f, 4.5f);
    cth = std::cos(th_);

    resting_ = false;
    onMark_ = onDock_ = onShip_ = false;
    if (carrying_ >= 0 && mode_ == Mode::Shift) {
        float cx, cy;
        carryAt(cx, cy);
        onMark_ = std::fabs(cx - MARK_X) <= SET_X;
        onShip_ = cx >= SHIP_L + 6.f && cx <= SHIP_R - 6.f;
        onDock_ = cx >= DOCK_L + CRATE_W * 0.5f - 1.f && cx <= DOCK_R - CRATE_W * 0.5f + 1.f;
        float support = -1.f;
        if (onMark_) support = DECK_Y - float(shipped_) * CRATE_H;
        else if (onShip_) support = DECK_Y;
        else if (onDock_) support = DOCK_Y;
        if (support > 0.f && cth > 0.35f) {
            float maxHookY = support - CRATE_H - HOOK_GAP;
            float maxL = (maxHookY - RAIL_Y) / cth;
            if (len_ > maxL) {
                if (std::fabs(th_) > 0.75f || std::fabs(om_) > 2.8f) failDrop("SWING HIT");
                else {
                    len_ = maxL;
                    resting_ = std::fabs(th_) < 0.30f && std::fabs(om_) < 0.95f && std::fabs(vx_) < 28.f;
                    if (resting_) om_ *= std::exp(-4.f * dt);
                }
            }
        }
        if (carrying_ >= 0 && std::fabs(th_) > 1.02f) failDrop("SLIPPED");
    }

    if (act && mode_ == Mode::Shift) {
        Job was = job_;
        if (carrying_ >= 0) release();
        else if (grab()) {
            job_ = Job::Lift;
            phaseT_ = 0;
        }
        if (mode_ == Mode::Win) job_ = Job::Done;
        else if (mode_ == Mode::Shift && carrying_ < 0 && was == Job::Descend) {
            job_ = Job::Seek;
            phaseT_ = 0;
        }
    }

    for (int i = 0; i < 3; i++) {
        if (crates_[i].box != Box::Fall) continue;
        crates_[i].vy += 760.f * dt;
        crates_[i].y += crates_[i].vy * dt;
        if (crates_[i].y > WATER_Y - 4.f) {
            crates_[i].y = WATER_Y - 4.f;
            crates_[i].vy = 0;
            if (splashT_ <= 0.f) {
                splashT_ = 0.7f;
                splashX_ = crates_[i].x;
            }
        }
    }
    if (splashT_ > 0.f) splashT_ -= dt;

    if (mode_ == Mode::Drop) {
        dropT_ += dt;
        if (dropT_ > 1.05f) {
            mode_ = Mode::Fail;
            over_ = true;
        }
    }

    if (mode_ == Mode::Shift) {
        if (std::fabs(hoist) > 6.f) sys_->apu.tone(1, 70.f + len_ * 0.35f, 0.035f);
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
    static const float notes[] = {523.f, 659.f, 784.f, 1046.f};
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
    if ((mode_ == Mode::Title || mode_ == Mode::Win || mode_ == Mode::Fail) && startPressed()) begin();
    if (mode_ == Mode::Title) titlePose();
    else if (mode_ == Mode::Win || mode_ == Mode::Fail) chime(dt);
    else update(dt);
    draw();
}

void Game::draw() {
    sys_->vdp.clearSprites();
    sys_->vdp.HUD.clear();

    float hx, hy;
    hookAt(hx, hy);
    float bob = std::sin(t_ * 4.f) * 3.f;
    int hookPal = (carrying_ >= 0 && resting_ && onMark_) ? PAL_SET : PAL_HOOK;

    // Earlier sprites sit on top, so the cable is submitted last.
    spr(art_.bird, 30.f + std::fmod(t_ * 16.f, 200.f), 64.f + std::sin(t_ * 1.7f) * 4.f, 9.f, PAL_WHITE);
    spr(art_.bird, 80.f + std::fmod(t_ * 11.f, 180.f), 88.f, 7.f, PAL_WHITE, true);
    if (mode_ == Mode::Title) spr(art_.banner, 100.f, 58.f, float(art_.banner.h), PAL_BANNER);
    spr(art_.trolley, tx_, RAIL_Y - 4.f, 26.f, PAL_CRANE);
    spr(art_.hook, hx, hy + 8.f, 18.f, hookPal);
    spr(art_.lamp, MARK_X - MARK_HALF + 2.f, DECK_Y - 2.f, 14.f, hookPal);
    spr(art_.lamp, MARK_X + MARK_HALF - 2.f, DECK_Y - 2.f, 14.f, hookPal);

    auto drawCrate = [&](int i, float x, float y) {
        spr(art_.shadow, x, y + CRATE_H * 0.55f, 8.f, PAL_WOOD, false, true);
        spr(art_.crate[i], x, y, CRATE_H, PAL_WOOD);
    };
    for (int i = 0; i < 3; i++) {
        if (crates_[i].box == Box::Carry) continue;
        if (crates_[i].box == Box::Wait && mode_ != Mode::Drop && mode_ != Mode::Fail)
            spr(art_.arrow, crates_[i].x, crates_[i].y - CRATE_H * 0.5f - 12.f + bob, 10.f, PAL_AMBER);
        drawCrate(i, crates_[i].x, crates_[i].y);
    }
    if (carrying_ >= 0) {
        float cx, cy;
        carryAt(cx, cy);
        drawCrate(carrying_, cx, cy);
    }
    if (splashT_ > 0.f) spr(art_.splash, splashX_, WATER_Y, 18.f + (0.7f - splashT_) * 10.f, PAL_SPLASH);

    float dx = hx - tx_;
    float dy = hy - RAIL_Y;
    float dist = std::sqrt(dx * dx + dy * dy);
    int links = std::max(1, int(dist / 5.f));
    for (int i = 0; i <= links; i++) {
        float u = float(i) / float(links);
        spr(art_.link, tx_ + dx * u, RAIL_Y + dy * u, 5.f, PAL_CABLE);
    }

    if (mode_ == Mode::Title) {
        hud(2, 11, "THREE CRATES", PAL_WHITE);
        hud(2, 12, "SHIP TO THE MARK", PAL_AMBER);
        hud(2, 14, "A DROP FAILS THE SHIFT", PAL_RED);
        hud(2, 16, "LEFT RIGHT MOVES  UP DOWN HOISTS", PAL_WHITE);
        hud(2, 17, "Z OR C SETS", PAL_GREEN);
        hud(24, 17, "PRESS START", PAL_GREEN);
        return;
    }

    char line[40];
    std::snprintf(line, sizeof line, "SHIPPED %d OF 3", shipped_);
    hud(1, 0, "S3 CRANE", PAL_AMBER);
    hud(22, 0, line, PAL_WHITE);

    if (mode_ == Mode::Shift && carrying_ >= 0) {
        int swings = int(std::fabs(th_) / 1.02f * 8.f);
        if (swings > 8) swings = 8;
        std::string bar = "SWING ";
        for (int i = 0; i < 8; i++) bar.push_back(i < swings ? '#' : '.');
        int pal = swings >= 6 ? PAL_RED : swings >= 3 ? PAL_AMBER : PAL_GREEN;
        hud(1, 1, bar, pal);
        if (resting_ && onMark_) hud(1, 2, "SET DOWN ON THE MARK", PAL_GREEN);
        else if (resting_ && onDock_ && !onShip_) hud(1, 2, "SAFE ON THE PIER", PAL_AMBER);
        else if (resting_ && onShip_) hud(1, 2, "OFF THE MARK", PAL_RED);
        else hud(1, 2, "A DROP FAILS THE SHIFT", PAL_RED);
    } else if (mode_ == Mode::Shift) {
        hud(1, 1, "HOOK THE NEXT CRATE", PAL_WHITE);
    }

    if (mode_ == Mode::Win) {
        hudC(5, "SHIFT CLEAR", PAL_GREEN);
        hudC(7, "THREE CRATES SHIPPED", PAL_WHITE);
        hudC(9, "PRESS START", PAL_AMBER);
    } else if (mode_ == Mode::Fail) {
        hudC(5, "SHIFT FAILED", PAL_RED);
        hudC(7, "A DROP FAILS THE SHIFT", PAL_WHITE);
        hudC(9, "PRESS START", PAL_AMBER);
    } else if (mode_ == Mode::Drop) {
        hudC(8, banner_, PAL_RED);
    } else {
        hud(1, 26, "LEFT RIGHT UP DOWN   Z SETS", PAL_WHITE);
    }
}

}  // namespace crane

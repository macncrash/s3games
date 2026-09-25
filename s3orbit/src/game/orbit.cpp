#include "game/orbit.h"

#include "game/world.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace orbit {
namespace {

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

void clampAccel(float& ax, float& ay, float m) {
    float s = std::hypot(ax, ay);
    if (s > m && s > 0.f) {
        ax *= m / s;
        ay *= m / s;
    }
}

}  // namespace

Game::Arm Game::pose(float t) const {
    constexpr float wS = 0.30f, aS = 18.f;
    constexpr float wB = 0.42f, aB = 12.f, pB = 0.7f;
    constexpr float wR = 0.22f, aR = 8.f;
    float sway = std::sin(t * wS) * aS;
    float bob = std::sin(t * wB + pB) * aB;
    float reach = 56.f + std::sin(t * wR) * aR;
    Arm a;
    a.ex = kElbowX;
    a.ey = kShoulderY + sway;
    a.tx = kElbowX - reach;
    a.ty = a.ey + bob;
    a.tvx = -std::cos(t * wR) * aR * wR;
    a.tvy = std::cos(t * wS) * aS * wS + std::cos(t * wB + pB) * aB * wB;
    return a;
}

void Game::begin() {
    mode_ = Mode::Play;
    t_ = 0;
    x_ = kStartX;
    y_ = kStartY;
    vx_ = vy_ = 0;
    ax_ = ay_ = 0;
    dwell_ = 0;
    stun_ = 0;
    shake_ = 0;
    flash_ = 0;
    phase_ = 0;
    contact_ = 0;
    tries_ = kTries;
    clock_ = kWindow;
    won_ = false;
    over_ = false;
    fine_ = false;
    why_ = "hung";
    chime_ = 0;
    chimeT_ = 0;
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(1, 0, 0);
    sys_->apu.tone(2, 0, 0);
    sys_->apu.noise(0, 0, false);
}

bool Game::startPressed() const { return sys_->pad.pressed(gs::BTN_START); }

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.setFogColor(gs::rgb4(0, 0, 2));
    if (bot_) begin();
    else {
        mode_ = Mode::Title;
        t_ = 0;
        x_ = kShowX;
        y_ = kShowY;
        vx_ = vy_ = 0;
        tries_ = kTries;
        clock_ = kWindow;
        why_ = "hung";
    }
    draw();
}

void Game::human(float& ax, float& ay) {
    const gs::Pad& p = sys_->pad;
    fine_ = p.down(gs::BTN_C) || p.down(gs::BTN_TURBO) || p.down(gs::BTN_A);
    float power = fine_ ? 34.f : 120.f;
    ax = ay = 0;
    if (p.down(gs::BTN_LEFT)) ax -= power;
    if (p.down(gs::BTN_RIGHT)) ax += power;
    if (p.down(gs::BTN_UP)) ay -= power;
    if (p.down(gs::BTN_DOWN)) ay += power;
    if (std::fabs(p.axisX) > 0.12f) ax = p.axisX * power;
    if (p.accel > 0.15f) ax += p.accel * (fine_ ? 34.f : 80.f);
    if (p.brake > 0.15f) ax -= p.brake * (fine_ ? 34.f : 80.f);
    if (p.down(gs::BTN_A)) ax += 34.f;
    clampAccel(ax, ay, 140.f);
}

void Game::pilot(const Arm& a, float& ax, float& ay) {
    float along = (x_ + kNose) - a.tx;
    float dy = y_ - a.ty;
    float rvx = vx_ - a.tvx;
    float rvy = vy_ - a.tvy;
    if (phase_ == 0) {
        if (along > -36.f && std::fabs(dy) < 6.f && std::fabs(rvy) < 8.f && std::fabs(rvx) < 12.f) phase_ = 1;
    } else if (along < -52.f || std::fabs(dy) > 22.f) {
        phase_ = 0;
    }
    if (phase_ == 0) {
        float tx = a.tx - 32.f - kNose;
        ax = (tx - x_) * 2.1f + (a.tvx - vx_) * 3.5f;
        ay = (a.ty - y_) * 3.8f + (a.tvy - vy_) * 4.4f;
    } else {
        float want = clampf((1.5f - along) * 1.3f, -5.f, 8.5f);
        ax = (a.tvx + want - vx_) * 4.4f;
        ay = (a.ty - y_) * 5.2f + (a.tvy - vy_) * 4.8f;
    }
    if (along > -22.f && rvx > 14.f) ax = std::min(ax, -80.f);
    if (std::fabs(dy) < 14.f && std::fabs(rvy) > 16.f) ay = (a.tvy - vy_) * 6.f;
    clampAccel(ax, ay, 105.f);
}

void Game::physics(float dt, float ax, float ay) {
    vx_ += ax * dt;
    vy_ += ay * dt;
    if (fine_) {
        vx_ -= vx_ * 3.4f * dt;
        vy_ -= vy_ * 3.4f * dt;
    }
    vx_ = clampf(vx_, -160.f, 160.f);
    vy_ = clampf(vy_, -160.f, 160.f);
    x_ += vx_ * dt;
    y_ += vy_ * dt;
    if (x_ < 12.f) {
        x_ = 12.f;
        if (vx_ < 0) vx_ = 0;
    }
    if (y_ < 12.f) {
        y_ = 12.f;
        if (vy_ < 0) vy_ = 0;
    }
    if (y_ > 210.f) {
        y_ = 210.f;
        if (vy_ > 0) vy_ = 0;
    }
    if (x_ > 306.f) {
        x_ = 306.f;
        if (vx_ > 0) vx_ = 0;
    }
}

bool Game::ball(float cx, float cy, float r, float L, float T, float W, float H) const {
    float nx = clampf(cx, L, L + W);
    float ny = clampf(cy, T, T + H);
    float dx = cx - nx, dy = cy - ny;
    return dx * dx + dy * dy < r * r;
}

float Game::segDist(float px, float py, float ax, float ay, float bx, float by) const {
    float abx = bx - ax, aby = by - ay;
    float apx = px - ax, apy = py - ay;
    float ab2 = abx * abx + aby * aby;
    float u = ab2 > 1.f ? (apx * abx + apy * aby) / ab2 : 0.f;
    u = clampf(u, 0.f, 1.f);
    return std::hypot(px - (ax + abx * u), py - (ay + aby * u));
}

void Game::miss(const char* why) {
    flash_ = 0.28f;
    shake_ = 0.45f;
    dwell_ = 0;
    phase_ = 0;
    ax_ = ay_ = 0;
    sys_->apu.noiseBurst(0.42f, 1500.f, 9.f);
    sys_->rumble(1.f, 0.35f, 160);
    tries_--;
    if (tries_ <= 0) {
        lose(why);
        return;
    }
    why_ = why;
    stun_ = 1.15f;
    x_ = kStartX;
    y_ = kStartY;
    vx_ = vy_ = 0;
}

void Game::lose(const char* why) {
    if (mode_ != Mode::Play) return;
    why_ = why;
    won_ = false;
    over_ = true;
    mode_ = Mode::Fail;
    ax_ = ay_ = 0;
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(1, 0, 0);
    sys_->apu.tone(2, 0, 0);
    sys_->apu.noiseBurst(0.28f, 420.f, 5.f);
}

void Game::latch(float rel) {
    if (mode_ != Mode::Play) return;
    contact_ = std::max(0, (int)std::lround(rel));
    won_ = true;
    over_ = true;
    mode_ = Mode::Win;
    why_ = "SOFT DOCK";
    chime_ = 0;
    chimeT_ = 0;
    dwell_ = kDwellNeed;
    ax_ = ay_ = 0;
    sys_->apu.tone(1, 0, 0);
    sys_->apu.tone(2, 0, 0);
    sys_->rumble(0.25f, 0.55f, 140);
    stick();
}

void Game::stick() {
    Arm a = pose(t_);
    x_ = a.tx - kNose;
    y_ = a.ty;
    vx_ = a.tvx;
    vy_ = a.tvy;
    ax_ = ay_ = 0;
}

bool Game::hazards(const Arm& a) {
    float probeX = x_ + kNose;
    float probeY = y_;
    if (ball(x_, y_, kShipR, kStationL, kStationT, kStationW, kStationH) ||
        ball(probeX, probeY, 3.f, kStationL, kStationT, kStationW, kStationH)) {
        miss("STRUCK THE HULL");
        return true;
    }
    float seatD = std::hypot(probeX - a.tx, probeY - a.ty);
    if (seatD > 18.f) {
        float backX = a.tx + kCollarBack;
        float dArm = std::min(segDist(x_, y_, kShoulderX, kShoulderY, a.ex, a.ey),
                              segDist(x_, y_, a.ex, a.ey, backX, a.ty));
        float dNose = std::min(segDist(probeX, probeY, kShoulderX, kShoulderY, a.ex, a.ey),
                               segDist(probeX, probeY, a.ex, a.ey, backX, a.ty));
        if (dArm < 8.f || dNose < 4.f) {
            float speed = std::hypot(vx_ - a.tvx, vy_ - a.tvy);
            if (speed > 32.f) {
                miss("STRUCK THE ARM");
                return true;
            }
            float ox = x_ - a.ex, oy = y_ - a.ey;
            float od = std::hypot(ox, oy);
            if (od < 1.f) od = 1.f;
            x_ += ox / od * 4.f;
            y_ += oy / od * 4.f;
            vx_ *= 0.4f;
            vy_ *= 0.4f;
            dwell_ = 0;
            return true;
        }
    }
    return false;
}

void Game::dock(const Arm& a) {
    float dx = (x_ + kNose) - a.tx;
    float dy = y_ - a.ty;
    float rvx = vx_ - a.tvx;
    float rvy = vy_ - a.tvy;
    float speed = std::hypot(rvx, rvy);
    bool jaw = std::fabs(dy) >= kJawIn && std::fabs(dy) < kJawOut && dx > kJawL && dx < kJawR;
    bool back = dx > kSeatX && dx < 12.f && std::fabs(dy) < kJawOut && !jaw;
    bool hardBox = std::fabs(dx) < 6.f && std::fabs(dy) < 7.5f && dx > -4.f;
    if (jaw) {
        if (speed > kHardMetal) {
            miss("HARD CONTACT");
            return;
        }
        float out = dy < 0.f ? -1.f : 1.f;
        y_ += out * 5.f;
        vy_ = a.tvy + out * 26.f;
        vx_ = a.tvx + rvx * 0.35f;
        dwell_ = 0;
        sys_->apu.tone(0, 150.f, 0.05f);
        return;
    }
    if (back && !hardBox) {
        if (speed > kHardMetal) {
            miss("HARD CONTACT");
            return;
        }
        vx_ = a.tvx - 14.f;
        dwell_ = 0;
        sys_->apu.tone(0, 160.f, 0.04f);
        return;
    }
    if (hardBox && (rvx > kHardRvx || std::fabs(rvy) > 40.f || speed > 52.f)) {
        miss("HARD CONTACT");
        return;
    }
    float xw = dwell_ > 0.05f ? kSeatX + 1.2f : kSeatX;
    float yw = dwell_ > 0.05f ? kSeatY + 1.f : kSeatY;
    bool seated = std::fabs(dx) < xw && std::fabs(dy) < yw;
    bool soft = seated && rvx < kSoftRvx && rvx > -8.f && std::fabs(rvy) < 14.f;
    if (soft) {
        dwell_ += 1.f / 60.f;
        if (dwell_ >= kDwellNeed) latch(speed);
        return;
    }
    if (seated && rvx >= kSoftRvx) {
        vx_ = a.tvx - 12.f;
        dwell_ = 0;
        sys_->apu.tone(0, 200.f, 0.05f);
        return;
    }
    dwell_ = 0;
}

void Game::update(float dt) {
    clock_ -= dt;
    if (clock_ <= 0.f) {
        lose("THE WINDOW CLOSED");
        return;
    }
    if (stun_ > 0.f) {
        stun_ -= dt;
        ax_ = ay_ = 0;
        dwell_ = 0;
        sys_->apu.tone(1, 0, 0);
        sys_->apu.tone(2, 0, 0);
        return;
    }
    Arm a = pose(t_);
    float ax = 0, ay = 0;
    fine_ = false;
    if (bot_) pilot(a, ax, ay);
    else human(ax, ay);
    ax_ = ax;
    ay_ = ay;
    physics(dt, ax, ay);
    if (hazards(a)) return;
    dock(pose(t_));
    if (mode_ != Mode::Play) return;
    bool thrust = std::fabs(ax_) > 8.f || std::fabs(ay_) > 8.f;
    sys_->apu.tone(1, thrust ? (fine_ ? 190.f : 88.f) : 0.f, thrust ? 0.034f : 0.f);
    float along = (x_ + kNose) - a.tx;
    float dy = y_ - a.ty;
    if (along > -50.f && std::fabs(dy) < 12.f)
        sys_->apu.tone(2, 260.f + (50.f + along) * 5.f, 0.022f);
    else
        sys_->apu.tone(2, 0, 0);
}

void Game::chime(float dt) {
    static const float notes[] = {523.f, 659.f, 784.f, 1046.f};
    if (chime_ >= 4) return;
    if (chimeT_ <= 0.f) sys_->apu.tone(0, notes[chime_], 0.07f);
    chimeT_ += dt;
    if (chimeT_ > 0.16f) {
        chimeT_ = 0;
        chime_++;
        if (chime_ >= 4) sys_->apu.tone(0, 0, 0);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const float dt = 1.f / 60.f;
    const bool start = startPressed();
    const bool launch = start || sys_->pad.pressed(gs::BTN_A);
    if (mode_ == Mode::Title || mode_ == Mode::Win || mode_ == Mode::Fail) {
        if (launch) begin();
    } else if (mode_ == Mode::Pause) {
        if (start) mode_ = Mode::Play;
    } else if (mode_ == Mode::Play && !bot_ && start) {
        mode_ = Mode::Pause;
    }
    if (mode_ != Mode::Pause) t_ += dt;
    if (shake_ > 0) shake_ = std::max(0.f, shake_ - dt);
    if (flash_ > 0) flash_ = std::max(0.f, flash_ - dt);
    if (mode_ == Mode::Play) update(dt);
    else {
        ax_ = ay_ = 0;
        if (mode_ == Mode::Win) {
            chime(dt);
            stick();
        } else if (mode_ == Mode::Title) {
            x_ = kShowX;
            y_ = kShowY + std::sin(t_ * 1.2f) * 4.f;
            vx_ = vy_ = 0;
        }
    }
    draw();
}

void Game::sky() {
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int b = 1 + y / 90;
        if (flash_ > 0) sys_->vdp.lineBackdrop[y] = gs::rgb4(7, 1, 2);
        else sys_->vdp.lineBackdrop[y] = gs::rgb4(0, 0, b);
    }
}

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

void Game::boom(float x0, float y0, float x1, float y1, float jx, float jy) {
    float dx = x1 - x0, dy = y1 - y0;
    float len = std::hypot(dx, dy);
    int n = std::max(1, int(len / 5.f));
    for (int i = 0; i <= n; i++) {
        float u = float(i) / float(n);
        spr(art_.link, x0 + dx * u + jx, y0 + dy * u + jy, 6.f, PAL_ARM);
    }
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

void Game::draw() {
    sys_->vdp.clearSprites();
    sys_->vdp.HUD.clear();
    sky();
    float jx = 0, jy = 0;
    if (shake_ > 0.f) {
        jx = std::sin(t_ * 90.f) * 2.5f;
        jy = std::cos(t_ * 70.f) * 2.f;
    }
    sys_->vdp.B.scroll(int(jx), int(jy));

    Arm a = pose(t_);
    float dx = (x_ + kNose) - a.tx;
    float dy = y_ - a.ty;
    float rvx = vx_ - a.tvx;
    int collarPal = PAL_ARM;
    if (mode_ == Mode::Win || dwell_ > 0.f) collarPal = PAL_OK;
    else if (dx > -28.f && dx < 8.f && std::fabs(dy) < 14.f && rvx > 32.f) collarPal = PAL_HOT;

    // Earlier sprites draw on top. The ship covers the clamp; stars sit at the back.
    spr(art_.ship, x_ + jx, y_ + jy, float(art_.ship.h), PAL_SHIP);
    if (ax_ > 12.f) spr(art_.plume, x_ - 26.f + jx, y_ + jy, 8.f, PAL_FIRE);
    if (ax_ < -12.f) spr(art_.puff, x_ + 24.f + jx, y_ + jy, 7.f, PAL_FIRE);
    if (ay_ > 12.f) spr(art_.puff, x_ + jx, y_ - 12.f + jy, 7.f, PAL_FIRE);
    if (ay_ < -12.f) spr(art_.puff, x_ + jx, y_ + 12.f + jy, 7.f, PAL_FIRE);

    int lanePal = std::fabs(dy) < 5.f ? PAL_GREEN : PAL_AMBER;
    if (mode_ == Mode::Play || mode_ == Mode::Pause) {
        spr(art_.chevron, a.tx - 22.f + jx, a.ty + jy, 10.f, PAL_AMBER);
        for (int i = 1; i <= 4; i++) spr(art_.dot, a.tx - 8.f - i * 8.f + jx, a.ty + jy, 4.f, lanePal);
    }
    spr(art_.collar, a.tx + jx, a.ty + jy, float(art_.collar.h), collarPal);

    float backX = a.tx + kCollarBack;
    spr(art_.joint, kShoulderX + jx, kShoulderY + jy, 12.f, PAL_ARM);
    spr(art_.joint, a.ex + jx, a.ey + jy, 11.f, PAL_ARM);
    spr(art_.joint, backX + jx, a.ty + jy, 9.f, PAL_ARM);
    boom(kShoulderX, kShoulderY, a.ex, a.ey, jx, jy);
    boom(a.ex, a.ey, backX, a.ty, jx, jy);

    int beacon = (int(t_ * 2.f) & 1) ? PAL_HOT : PAL_ARM;
    spr(art_.lamp, 312 + jx, 36 + jy, 8.f, beacon);
    spr(art_.lamp, 292 + jx, 186 + jy, 7.f, beacon);
    static const float kDrift[10][3] = {{18, 16, 14}, {46, 28, 9},  {80, 48, 18}, {24, 64, 11}, {110, 22, 7},
                                        {140, 58, 15}, {66, 12, 8}, {96, 78, 12}, {36, 96, 16}, {150, 40, 10}};
    for (int i = 0; i < 10; i++) {
        float sx = std::fmod(kDrift[i][0] - t_ * kDrift[i][2], 220.f);
        if (sx < 0) sx += 220.f;
        spr(art_.dot, sx + jx, kDrift[i][1] + jy, 4.f, PAL_TEXT);
    }

    if (mode_ == Mode::Title) {
        bool blink = int(t_ * 2.f) % 2 == 0;
        hud(2, 2, "S3 ORBIT", PAL_AMBER);
        hud(2, 4, "DOCK TO THE ARM", PAL_TEXT);
        hud(2, 6, "TOO HARD A CONTACT IS A FAIL", PAL_RED);
        hud(2, 9, "ARROWS MOVE", PAL_TEXT);
        hud(2, 10, "Z CREEPS    C OR SPACE FINE", PAL_GREEN);
        hud(2, 12, "HOLD THE SEAT UNTIL IT LATCHES", PAL_AMBER);
        if (blink) hud(2, 24, "PRESS START", PAL_GREEN);
        return;
    }

    char line[48];
    int window = std::max(0, (int)std::ceil(clock_ - 0.001f));
    std::snprintf(line, sizeof line, "TRY %d", std::max(0, tries_));
    hud(1, 0, "S3 ORBIT", PAL_AMBER);
    hud(16, 0, line, tries_ == 1 ? PAL_RED : PAL_TEXT);
    std::snprintf(line, sizeof line, "WINDOW %d", window);
    hud(28, 0, line, window <= 10 ? PAL_RED : PAL_TEXT);

    if (mode_ == Mode::Play || mode_ == Mode::Pause) {
        int closing = (int)std::lround(rvx);
        int off = (int)std::lround(std::fabs(dy));
        int pal = (rvx > kHardRvx) ? PAL_RED : (rvx < kSoftRvx && off <= 5) ? PAL_GREEN : PAL_AMBER;
        std::snprintf(line, sizeof line, "CLOSING %d", closing);
        hud(1, 1, line, pal);
        if (off <= 1) hud(16, 1, "LINED UP", PAL_GREEN);
        else {
            std::snprintf(line, sizeof line, dy < 0 ? "HIGH %d" : "LOW %d", off);
            hud(16, 1, line, off >= 9 ? PAL_RED : PAL_AMBER);
        }
        if (dwell_ > 0.f || (std::fabs(dx) < 20.f && std::fabs(dy) < 10.f)) {
            int n = std::min(10, int(dwell_ / kDwellNeed * 10.f + 0.5f));
            std::string bar = "LATCH ";
            for (int i = 0; i < 10; i++) bar.push_back(i < n ? '#' : '.');
            hud(1, 2, bar, dwell_ > 0.f ? PAL_GREEN : PAL_AMBER);
        } else {
            hud(1, 2, "MATCH THE ARM", PAL_TEXT);
        }
        if (stun_ > 0.f) {
            hudC(8, why_, PAL_RED);
            std::snprintf(line, sizeof line, "TRY %d LEFT", tries_);
            hudC(10, line, PAL_AMBER);
        }
    }

    if (mode_ == Mode::Win) {
        hudC(6, "SOFT DOCK", PAL_GREEN);
        hudC(8, "LATCHED ON THE ARM", PAL_TEXT);
        std::snprintf(line, sizeof line, "CONTACT %d", contact_);
        hudC(10, line, PAL_AMBER);
        hudC(13, "PRESS START", PAL_GREEN);
    } else if (mode_ == Mode::Fail) {
        hudC(6, "DOCK FAILED", PAL_RED);
        hudC(8, why_, PAL_TEXT);
        hudC(11, "PRESS START", PAL_AMBER);
    } else if (mode_ == Mode::Pause) {
        hudC(8, "PAUSED", PAL_AMBER);
        hudC(10, "PRESS START", PAL_TEXT);
    } else if (stun_ <= 0.f) {
        hud(1, 26, "ARROWS MOVE   Z CREEPS   C FINE", PAL_TEXT);
        hud(1, 27, "SOFT CONTACT LATCHES", PAL_GREEN);
    }
}

}  // namespace orbit

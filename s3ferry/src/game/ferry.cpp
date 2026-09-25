#include "game/ferry.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

namespace ferry {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;
constexpr float kNorth = 1.5707963f;

constexpr float kHalfLen = 11.f;
constexpr float kHalfBeam = 3.9f;
constexpr float kTide = 96.f;
constexpr float kHoldNeed = 0.50f;
constexpr float kWinSpeed = 0.90f;
constexpr float kAlign = 0.26f;
constexpr float kInL = -13.2f;
constexpr float kInR = 13.2f;
constexpr float kInS = 30.f;
constexpr float kInN = 64.5f;
constexpr float kPocketY = 45.f;
constexpr float kStartX = 44.f;
constexpr float kStartY = -48.f;
constexpr float kPlayZoom = 2.25f;
constexpr float kTitleZoom = 0.92f;
constexpr float kDrawH = 32.f;

float wrap(float a) {
    while (a > kPi) a -= kTau;
    while (a < -kPi) a += kTau;
    return a;
}

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    return gs::rgb4(int(ar + (br - ar) * t + 0.5f), int(ag + (bg - ag) * t + 0.5f), int(ab + (bb - ab) * t + 0.5f));
}

const char* compass(float h) {
    static const char* name[] = {"E", "NE", "N", "NW", "W", "SW", "S", "SE"};
    float u = h;
    while (u < 0.f) u += kTau;
    while (u >= kTau) u -= kTau;
    int i = int((u + kPi / 8.f) / (kPi / 4.f)) & 7;
    return name[i];
}

void bodyToWorld(float h, float surge, float sway, float& vx, float& vy) {
    float c = std::cos(h), s = std::sin(h);
    vx = c * surge + s * sway;
    vy = s * surge - c * sway;
}

void worldToBody(float h, float vx, float vy, float& surge, float& sway) {
    float c = std::cos(h), s = std::sin(h);
    surge = c * vx + s * vy;
    sway = s * vx - c * vy;
}

}  // namespace

float Game::speed() const { return std::hypot(worldVx_, worldVy_); }

float Game::rnd() {
    rng_ = rng_ * 1664525u + 1013904223u;
    return (rng_ >> 8) * (1.f / 16777216.f);
}

void Game::tideFlow(float& cx, float& cy) const {
    float u = 1.f - std::clamp(clock_ / kTide, 0.f, 1.f);
    float ebb = 0.40f + 3.2f * u * u;
    cx = -ebb;
    cy = -0.30f * ebb;
}

void Game::begin() {
    x_ = kStartX;
    y_ = kStartY;
    heading_ = std::atan2(24.f - kStartY, 0.f - kStartX);
    surge_ = 0;
    sway_ = 0;
    yaw_ = 0;
    worldVx_ = 0;
    worldVy_ = 0;
    clock_ = kTide;
    hold_ = 0;
    shake_ = 0;
    foamT_ = 0;
    wakeT_ = 0;
    wasHit_ = false;
    won_ = false;
    over_ = false;
    phase_ = 0;
    chime_ = 0;
    chimeStep_ = 0;
    thrustIn_ = 0;
    foams_.clear();
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(1, 4, 7));
    sys.apu.setMaster(0.72f);
    sys.apu.setEcho(0.10f, 0.18f, 0.08f);
    rng_ = 0xF311u;
    begin();
    if (bot_) {
        mode_ = Mode::Play;
        camX_ = x_;
        camY_ = y_;
        zoom_ = kPlayZoom;
    } else {
        mode_ = Mode::Title;
        camX_ = 8.f;
        camY_ = -4.f;
        zoom_ = kTitleZoom;
    }
}

void Game::controls(float& thrust, float& rudder, float& bow) {
    const gs::Pad& p = sys_->pad;
    thrust = 0;
    if (p.down(gs::BTN_UP)) thrust += 1.f;
    if (p.down(gs::BTN_DOWN)) thrust -= 1.f;
    if (p.accel > 0.15f) thrust = p.accel;
    if (p.brake > 0.15f) thrust = -p.brake;
    thrust = std::clamp(thrust, -1.f, 1.f);
    rudder = 0;
    if (p.down(gs::BTN_LEFT)) rudder += 1.f;
    if (p.down(gs::BTN_RIGHT)) rudder -= 1.f;
    if (std::fabs(p.axisX) > 0.22f) rudder = std::clamp(-p.axisX, -1.f, 1.f);
    bow = 0;
    if (p.down(gs::BTN_A) || p.down(gs::BTN_X)) bow -= 1.f;
    if (p.down(gs::BTN_B) || p.down(gs::BTN_Y)) bow += 1.f;
    bow = std::clamp(bow, -1.f, 1.f);
    if (p.pressed(gs::BTN_C) || p.pressed(gs::BTN_TURBO)) horn();
}

void Game::chase(float tx, float ty, float maxSpd, bool lockNorth, float& thrust, float& rudder, float& bow) {
    float cx, cy;
    tideFlow(cx, cy);
    float dx = tx - x_, dy = ty - y_;
    float dist = std::hypot(dx, dy);
    float wantVx, wantVy;
    if (lockNorth && dist < 1.4f) {
        wantVx = std::clamp(-x_ * 0.65f, -0.55f, 0.55f);
        wantVy = std::clamp((ty - y_) * 0.65f, -0.55f, 0.55f);
        if (std::fabs(x_) < 0.40f && std::fabs(ty - y_) < 0.40f) {
            wantVx = 0;
            wantVy = 0;
        }
    } else if (dist < 0.08f) {
        wantVx = 0;
        wantVy = 0;
    } else {
        float spd = std::min(maxSpd, std::max(0.40f, dist * 0.48f));
        wantVx = dx / dist * spd;
        wantVy = dy / dist * spd;
    }
    float rvx = wantVx - cx;
    float rvy = wantVy - cy;
    float desH = lockNorth ? kNorth : std::atan2(rvy, rvx);
    float hErr = wrap(desH - heading_);
    rudder = std::clamp(hErr * 3.3f - yaw_ * 2.3f, -1.f, 1.f);
    float desSurge, desSway;
    worldToBody(heading_, rvx, rvy, desSurge, desSway);
    if (!lockNorth && std::fabs(hErr) > 0.80f) desSurge = std::clamp(desSurge, -0.7f, 1.5f);
    if (lockNorth) {
        desSurge = std::clamp(desSurge, -1.6f, 1.7f);
        desSway = std::clamp(desSway, -2.6f, 2.6f);
    }
    float bowY = y_ + std::sin(heading_) * kHalfLen;
    if (bowY > 60.2f) desSurge = std::min(desSurge, (60.2f - bowY) * 0.9f);
    thrust = std::clamp((desSurge - surge_) * 1.45f, -1.f, 1.f);
    bow = std::clamp((desSway - sway_) * 0.95f, -1.f, 1.f);
}

void Game::pilot(float& thrust, float& rudder, float& bow) {
    bool beside = y_ > 13.f && std::fabs(x_) > 14.2f && y_ < 78.f;
    if (beside) {
        phase_ = 2;
        chase(x_ > 0.f ? 24.f : -24.f, -10.f, 2.6f, false, thrust, rudder, bow);
        return;
    }
    if (y_ < 20.f || std::fabs(x_) > 11.f) {
        phase_ = 0;
        float gate = std::hypot(x_, y_ - 24.f);
        float maxSpd = 6.1f;
        if (gate < 42.f) maxSpd = 3.5f;
        if (gate < 18.f) maxSpd = 2.35f;
        if (y_ > 6.f) maxSpd = std::min(maxSpd, 2.35f);
        chase(0.f, 24.f, maxSpd, false, thrust, rudder, bow);
        return;
    }
    phase_ = 1;
    float maxSpd = y_ > 34.f ? 1.05f : 2.05f;
    chase(0.f, kPocketY, maxSpd, true, thrust, rudder, bow);
}

void Game::corners(float xs[4], float ys[4]) const {
    float c = std::cos(heading_), s = std::sin(heading_);
    const float fl[4] = {kHalfLen, kHalfLen, -kHalfLen, -kHalfLen};
    const float fb[4] = {kHalfBeam, -kHalfBeam, kHalfBeam, -kHalfBeam};
    for (int i = 0; i < 4; i++) {
        xs[i] = x_ + c * fl[i] + s * fb[i];
        ys[i] = y_ + s * fl[i] - c * fb[i];
    }
}

bool Game::pushOut(float px, float py, float& dx, float& dy) const {
    struct R {
        float x0, y0, x1, y1;
    };
    static const R box[] = {
        {-38.f, 15.5f, -15.f, 82.f},
        {15.f, 15.5f, 38.f, 82.f},
        {-160.f, 66.f, 160.f, 150.f},
    };
    float best = 1e9f;
    bool hit = false;
    dx = dy = 0;
    for (const R& b : box) {
        if (px < b.x0 || px > b.x1 || py < b.y0 || py > b.y1) continue;
        float dl = px - b.x0, dr = b.x1 - px, db = py - b.y0, dt = b.y1 - py;
        float m = dl;
        int side = 0;
        if (dr < m) {
            m = dr;
            side = 1;
        }
        if (db < m) {
            m = db;
            side = 2;
        }
        if (dt < m) {
            m = dt;
            side = 3;
        }
        if (m < best) {
            best = m;
            hit = true;
            dx = dy = 0;
            if (side == 0) dx = -(dl + 0.25f);
            if (side == 1) dx = dr + 0.25f;
            if (side == 2) dy = -(db + 0.25f);
            if (side == 3) dy = dt + 0.25f;
        }
    }
    return hit;
}

void Game::resolveWalls() {
    float c = std::cos(heading_), s = std::sin(heading_);
    const float fl[8] = {kHalfLen, kHalfLen, -kHalfLen, -kHalfLen, kHalfLen, 0.f, -kHalfLen, 0.f};
    const float fb[8] = {kHalfBeam, -kHalfBeam, kHalfBeam, -kHalfBeam, 0.f, kHalfBeam, 0.f, -kHalfBeam};
    bool hit = false;
    for (int pass = 0; pass < 3; pass++) {
        for (int i = 0; i < 8; i++) {
            float px = x_ + c * fl[i] + s * fb[i];
            float py = y_ + s * fl[i] - c * fb[i];
            float dx, dy;
            if (!pushOut(px, py, dx, dy)) continue;
            x_ += dx;
            y_ += dy;
            hit = true;
        }
    }
    if (x_ < -120.f) x_ = -120.f;
    if (x_ > 120.f) x_ = 120.f;
    if (y_ < -140.f) y_ = -140.f;
    if (hit) {
        surge_ *= 0.28f;
        sway_ *= 0.28f;
        yaw_ *= 0.45f;
        shake_ = std::min(1.f, shake_ + 0.45f);
        if (!wasHit_) sys_->apu.noiseBurst(0.16f, 1600.f, 14.f);
    }
    wasHit_ = hit;
}

bool Game::hullInSlip() const {
    float xs[4], ys[4];
    corners(xs, ys);
    for (int i = 0; i < 4; i++) {
        if (xs[i] < kInL || xs[i] > kInR || ys[i] < kInS || ys[i] > kInN) return false;
    }
    return true;
}

bool Game::madeFast() const {
    return hullInSlip() && std::fabs(wrap(kNorth - heading_)) < kAlign && speed() < kWinSpeed;
}

void Game::physics(float dt, float thrust, float rudder, float bow) {
    float cx, cy;
    tideFlow(cx, cy);
    float wash = 0.55f + 0.45f * std::min(1.f, std::fabs(surge_) / 4.f);
    float ahead = thrust >= 0.f ? thrust : thrust * 0.75f;
    surge_ += ahead * 5.5f * dt;
    surge_ -= surge_ * 0.62f * dt;
    sway_ += bow * 3.6f * dt;
    sway_ -= sway_ * 1.30f * dt;
    yaw_ += rudder * wash * 1.45f * dt;
    yaw_ -= bow * 0.95f * dt;
    yaw_ -= yaw_ * 2.35f * dt;
    heading_ = wrap(heading_ + yaw_ * dt);
    float vx, vy;
    bodyToWorld(heading_, surge_, sway_, vx, vy);
    vx += cx;
    vy += cy;
    x_ += vx * dt;
    y_ += vy * dt;
    worldVx_ = vx;
    worldVy_ = vy;
    resolveWalls();
    float rx, ry;
    bodyToWorld(heading_, surge_, sway_, rx, ry);
    worldVx_ = rx + cx;
    worldVy_ = ry + cy;
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.055f);
    tone0_ = 0.12f;
}

void Game::horn() {
    sys_->apu.tone(1, 146.8f, 0.07f);
    hornT_ = 0.50f;
}

void Game::chime() {
    chime_ = 4;
    chimeStep_ = 0;
    chimeT_ = 0.f;
}

void Game::audio(float dt) {
    if (chime_ == 0 && tone0_ > 0.f) {
        tone0_ -= dt;
        if (tone0_ <= 0.f) sys_->apu.tone(0, 0, 0);
    }
    if (hornT_ > 0.f) {
        hornT_ -= dt;
        if (hornT_ <= 0.f) sys_->apu.tone(1, 0, 0);
    }
    if (chime_ > 0) {
        chimeT_ -= dt;
        if (chimeT_ <= 0.f) {
            static const float notes[] = {523.25f, 659.25f, 783.99f, 1046.5f};
            int n = std::min(chimeStep_, 3);
            sys_->apu.tone(0, notes[n], 0.05f);
            tone0_ = 0.18f;
            chimeT_ = 0.14f;
            if (++chimeStep_ >= chime_) chime_ = 0;
        }
    }
    if (mode_ == Mode::Play) {
        float vol = 0.016f + 0.030f * std::fabs(thrustIn_);
        float freq = 62.f + std::fabs(surge_) * 7.f;
        sys_->apu.tone(2, freq, vol);
    } else {
        sys_->apu.tone(2, 0, 0);
    }
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - dt);
}

void Game::updateFoam(float dt) {
    float cx, cy;
    tideFlow(cx, cy);
    for (Foam& f : foams_) {
        f.x += cx * dt;
        f.y += cy * dt;
        f.life -= dt;
    }
    foams_.erase(std::remove_if(foams_.begin(), foams_.end(), [](const Foam& f) { return f.life <= 0.f; }), foams_.end());
    if (mode_ != Mode::Play) return;
    foamT_ -= dt;
    if (foamT_ <= 0.f && foams_.size() < 18) {
        foamT_ = 0.35f;
        float fx = camX_ + (rnd() - 0.5f) * 120.f;
        float fy = camY_ + (rnd() - 0.5f) * 70.f;
        if (fy < 62.f) foams_.push_back({fx, fy, 1.4f + rnd()});
    }
    if (std::hypot(surge_, sway_) > 1.1f) {
        wakeT_ -= dt;
        if (wakeT_ <= 0.f && foams_.size() < 22) {
            wakeT_ = 0.07f;
            float c = std::cos(heading_), s = std::sin(heading_);
            foams_.push_back({x_ - c * (kHalfLen * 0.85f), y_ - s * (kHalfLen * 0.85f), 0.7f});
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) {
            begin();
            mode_ = Mode::Play;
            camX_ = x_;
            camY_ = y_;
            blip(620.f);
        } else if (pad.pressed(gs::BTN_MODE)) {
            sys.quit();
        }
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(320.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            begin();
            mode_ = Mode::Title;
        } else {
            float prev = clock_;
            clock_ -= kDt;
            if (clock_ < 0.f) clock_ = 0.f;
            float thrust = 0, rudder = 0, bow = 0;
            if (bot_) pilot(thrust, rudder, bow);
            else controls(thrust, rudder, bow);
            thrustIn_ = thrust;
            physics(kDt, thrust, rudder, bow);
            if (madeFast()) hold_ += kDt;
            else hold_ = 0;
            if (hold_ >= kHoldNeed && prev > 0.f) {
                mode_ = Mode::Win;
                won_ = true;
                over_ = true;
                chime();
            } else if (clock_ <= 0.f) {
                mode_ = Mode::Fail;
                won_ = false;
                over_ = true;
                blip(98.f);
            } else if (clock_ < 16.f && std::floor(prev) != std::floor(clock_)) {
                blip(clock_ < 8.f ? 960.f : 700.f);
            }
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
        else if (pad.pressed(gs::BTN_MODE)) {
            begin();
            mode_ = Mode::Title;
        }
    } else if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C))) {
            begin();
            mode_ = Mode::Play;
            camX_ = x_;
            camY_ = y_;
            blip(620.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            begin();
            mode_ = Mode::Title;
        }
    }

    if (mode_ == Mode::Title) {
        camX_ = 8.f + std::sin(t_ * 0.12f) * 4.f;
        camY_ = -4.f;
        zoom_ = kTitleZoom;
    } else {
        float lead = mode_ == Mode::Play ? 16.f : 0.f;
        float gx = x_ + std::cos(heading_) * lead;
        float gy = y_ + std::sin(heading_) * lead;
        float k = 1.f - std::exp(-kDt * 4.2f);
        camX_ += (gx - camX_) * k;
        camY_ += (gy - camY_) * k;
        zoom_ += (kPlayZoom - zoom_) * k;
        if (shake_ > 0.f) {
            camX_ += std::sin(t_ * 46.f) * shake_ * 2.5f;
            camY_ += std::cos(t_ * 38.f) * shake_ * 1.8f;
        }
    }
    updateFoam(kDt);
    audio(kDt);
    draw();
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c < 32 || c > 127 || c == ' ') continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = 0;
    if (s)
        while (s[n]) n++;
    hud(20 - n / 2, row, s, pal);
}

int Game::shipFrame() const {
    float u = heading_;
    while (u < 0.f) u += kTau;
    while (u >= kTau) u -= kTau;
    int i = int(std::lround(u / kTau * 16.f)) % 16;
    if (i < 0) i += 16;
    return i;
}

int Game::clockFrame() const {
    float u = 1.f - std::clamp(clock_ / kTide, 0.f, 1.f);
    int i = int(std::lround(u * 7.f));
    return std::clamp(i, 0, 7);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool shadow) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    if (cx + w * 0.5f < -16 || cy + h * 0.5f < -16 || cx - w * 0.5f > gs::SCREEN_W + 16 || cy - h * 0.5f > gs::SCREEN_H + 16) return;
    gs::Sprite s;
    long sw = std::clamp(std::lround(w), 1L, 1800L);
    long sh = std::clamp(std::lround(h), 1L, 1800L);
    s.w = int16_t(sw);
    s.h = int16_t(sh);
    s.x = int16_t(std::clamp(std::lround(cx - sw * 0.5f), -2000L, 2000L));
    s.y = int16_t(std::clamp(std::lround(cy - sh * 0.5f), -2000L, 2000L));
    s.img = m.pick(float(sh));
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::place(const gs::Mipped& m, float wx, float wy, float worldH, int pal, bool flip) {
    float sx, sy;
    worldToScreen(wx, wy, sx, sy);
    spr(m, sx, sy, worldH * zoom_, pal, flip, false);
}

void Game::worldToScreen(float wx, float wy, float& sx, float& sy) const {
    sx = 160.f + (wx - camX_) * zoom_;
    sy = 112.f - (wy - camY_) * zoom_;
}

void Game::drawHud() {
    char buf[64];
    int left = std::max(0, int(clock_));
    int pal = left < 16 ? PAL_ALERT : PAL_BANNER;
    std::snprintf(buf, sizeof buf, "TIDE %02d:%02d", left / 60, left % 60);
    if (mode_ == Mode::Title) {
        hudC(18, "DOCK IN THE SLIP BEFORE THE TIDE CLOCK", PAL_BANNER);
        hudC(20, "THE EBB SETS WEST AND OUT OF THE SLIP", PAL_DIM);
        hudC(22, "ARROWS  ENGINE AND RUDDER", PAL_HUD);
        hudC(23, "Z BOW PORT    X BOW STARBOARD", PAL_HUD);
        hudC(24, "C HORN", PAL_HUD);
        if ((int(t_ * 2.f) & 1) == 0) hudC(26, "RETURN", PAL_WIN);
        hud(1, 0, buf, pal);
        return;
    }
    hud(1, 0, "S3 FERRY", PAL_BANNER);
    hud(28, 0, buf, pal);
    if (mode_ == Mode::Pause) {
        hudC(16, "RETURN CONTINUES", PAL_HUD);
        hudC(17, "ESC TO THE TITLE", PAL_DIM);
        return;
    }
    if (mode_ == Mode::Win) {
        std::snprintf(buf, sizeof buf, "TIDE %02d:%02d LEFT", left / 60, left % 60);
        hudC(16, "LINES ARE FAST", PAL_WIN);
        hudC(17, buf, PAL_HUD);
        if (!bot_) hudC(19, "RETURN SAILS AGAIN", PAL_DIM);
        return;
    }
    if (mode_ == Mode::Fail) {
        hudC(16, "THE SLIP WENT DRY", PAL_ALERT);
        if (!bot_) hudC(18, "RETURN TRIES AGAIN", PAL_HUD);
        return;
    }
    std::snprintf(buf, sizeof buf, "SPD %04.1f  %s", speed(), compass(heading_));
    hud(1, 1, buf, PAL_HUD);
    float u = 1.f - std::clamp(clock_ / kTide, 0.f, 1.f);
    if (madeFast()) hud(1, 26, "HOLD FOR THE LINES", PAL_WIN);
    else if (hullInSlip() && std::fabs(wrap(kNorth - heading_)) >= kAlign) hud(1, 26, "NOSE TO THE BRIDGE", PAL_BANNER);
    else if (hullInSlip()) hud(1, 26, "SLOW FOR THE LINES", PAL_BANNER);
    else if (y_ > 10.f && std::fabs(x_) < 18.f) hud(1, 26, "EASE HER INTO THE SLIP", PAL_HUD);
    else if (u > 0.62f) hud(1, 26, "THE EBB IS STRONG", PAL_ALERT);
    else hud(1, 26, "MAKE THE MOUTH", PAL_HUD);
    hud(1, 27, "Z PORT BOW  X STBD BOW  C HORN", PAL_DIM);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    float tideU = 1.f - std::clamp(clock_ / kTide, 0.f, 1.f);
    const uint16_t deep = lerpC(gs::rgb4(1, 5, 10), gs::rgb4(3, 5, 6), tideU);
    const uint16_t mid = lerpC(gs::rgb4(2, 9, 13), gs::rgb4(6, 8, 6), tideU);
    const uint16_t land = gs::rgb4(8, 7, 4);
    const uint16_t mud = lerpC(gs::rgb4(5, 7, 4), gs::rgb4(8, 6, 3), tideU);
    const uint16_t spark = gs::rgb4(9, 14, 15);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (112.f - y) / std::max(zoom_, 0.05f);
        float shore = std::clamp((wy + 70.f) / 150.f, 0.f, 1.f);
        uint16_t water = lerpC(deep, mid, shore);
        float shimmer = 0.5f + 0.5f * std::sin(y * 0.08f + t_ * 1.5f);
        water = lerpC(water, spark, shimmer * (0.12f - 0.07f * tideU));
        if (wy > 63.f) water = lerpC(mud, land, std::clamp((wy - 63.f) / 16.f, 0.f, 1.f));
        v.lineBackdrop[y] = water;
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }

    // Earlier sprites win the pixel, so banners and the ferry go in before the harbor.
    if (mode_ == Mode::Title) spr(art_.title, 160.f, 40.f, float(art_.title.h), PAL_BANNER);
    else if (mode_ == Mode::Win) spr(art_.docked, 160.f, 78.f, float(art_.docked.h), PAL_WIN);
    else if (mode_ == Mode::Fail) spr(art_.tideOut, 160.f, 78.f, float(art_.tideOut.h), PAL_ALERT);
    else if (mode_ == Mode::Pause) spr(art_.paused, 160.f, 78.f, float(art_.paused.h), PAL_BANNER);
    spr(art_.clock[clockFrame()], 292.f, 22.f, 30.f, PAL_CLOCK);

    for (int i = 0; i < 3; i++) {
        float gx = 50.f + i * 80.f + std::sin(t_ * 0.6f + i * 1.7f) * 26.f;
        float gy = 148.f + i * 14.f + std::cos(t_ * 0.45f + i) * 8.f;
        bool up = std::sin(t_ * 5.f + i) > 0.f;
        spr(art_.gull[up ? 0 : 1], gx, gy, 12.f, PAL_GULL, i == 1);
    }

    float bsx, bsy;
    worldToScreen(x_, y_, bsx, bsy);
    bsy += std::sin(t_ * 2.0f + x_ * 0.05f) * 0.7f;
    float shipH = kDrawH * zoom_;
    if (mode_ == Mode::Title) shipH = std::max(shipH, 26.f);
    const gs::Mipped& hull = art_.hull[shipFrame()];
    spr(hull, bsx, bsy, shipH, PAL_FERRY);
    spr(hull, bsx + 3.f, bsy + 4.f, shipH, PAL_FERRY, false, true);

    for (const Foam& f : foams_) {
        float a = std::clamp(f.life, 0.2f, 1.4f);
        place(art_.foam, f.x, f.y, 1.6f + a, PAL_FOAM);
    }
    auto dashes = [&](float x0, float y0, float x1, float y1) {
        float dx = x1 - x0, dy = y1 - y0;
        float len = std::hypot(dx, dy);
        int n = std::max(1, int(len / 3.6f));
        for (int i = 0; i <= n; i += 2) {
            float u = float(i) / float(n);
            place(art_.guide, x0 + dx * u, y0 + dy * u, 1.7f, PAL_GUIDE);
        }
    };
    dashes(kInL, kInS + 1.f, kInR, kInS + 1.f);
    dashes(kInR, kInS + 1.f, kInR, kInN - 1.f);
    dashes(kInR, kInN - 1.f, kInL, kInN - 1.f);
    dashes(kInL, kInN - 1.f, kInL, kInS + 1.f);
    place(art_.buoy[0], -18.f, 10.f, 8.f, PAL_BUOY);
    place(art_.buoy[1], 18.f, 10.f, 8.f, PAL_BUOY);
    place(art_.bridge, 0.f, 61.f, 7.f, PAL_BANNER);
    place(art_.clock[clockFrame()], 0.f, 96.f, 12.f, PAL_CLOCK);
    place(art_.pier, -26.5f, 48.8f, 66.5f, PAL_PIER);
    place(art_.pier, 26.5f, 48.8f, 66.5f, PAL_PIER);
    place(art_.apron, 0.f, 71.f, 14.f, PAL_PIER);
    place(art_.shed, 0.f, 90.f, 22.f, PAL_SHED);

    drawHud();
}

}  // namespace ferry

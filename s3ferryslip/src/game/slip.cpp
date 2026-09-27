#include "game/slip.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace ferryslip {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;
constexpr float kNorth = 1.5707963f;

constexpr float kHalfLen = 8.4f;
constexpr float kHalfBeam = 2.45f;
constexpr float kCrew = 72.f;
constexpr float kHoldNeed = 0.55f;
constexpr float kWinSpeed = 0.75f;
constexpr float kAlign = 0.30f;
constexpr float kInL = -4.7f;
constexpr float kInR = 4.7f;
constexpr float kInS = 22.f;
constexpr float kInN = 46.f;
constexpr float kPocketY = 33.f;
constexpr float kStartX = 28.f;
constexpr float kStartY = -28.f;
constexpr float kPlayZoom = 2.55f;
constexpr float kTitleZoom = 1.05f;
constexpr float kDrawH = 28.f;

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

void Game::tideFlow(float& cx, float& cy) const {
    float u = 1.f - std::clamp(clock_ / kCrew, 0.f, 1.f);
    float ebb = 0.35f + 1.35f * u * u;
    bool sheltered = y_ > 18.f && std::fabs(x_) < 6.f;
    if (sheltered) ebb *= 0.18f;
    cx = -ebb;
    cy = 0.12f * ebb;
}

void Game::begin() {
    x_ = kStartX;
    y_ = kStartY;
    heading_ = std::atan2(8.f - kStartY, -kStartX);
    surge_ = 0;
    sway_ = 0;
    yaw_ = 0;
    speed_ = 0;
    clock_ = kCrew;
    hold_ = 0;
    shake_ = 0;
    foamT_ = 0;
    wasHit_ = false;
    won_ = false;
    over_ = false;
    chime_ = 0;
    chimeStep_ = 0;
    thrustIn_ = 0;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(1, 4, 7));
    sys.apu.setMaster(0.7f);
    sys.apu.setEcho(0.08f, 0.16f, 0.07f);
    begin();
    if (bot_) {
        mode_ = Mode::Play;
        camX_ = x_;
        camY_ = y_;
        zoom_ = kPlayZoom;
    } else {
        mode_ = Mode::Title;
        camX_ = 0.f;
        camY_ = 28.f;
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

void Game::pilot(float& thrust, float& rudder, float& bow) {
    float cx, cy;
    tideFlow(cx, cy);
    bool lined = y_ > 12.f && std::fabs(x_) < 6.5f;
    float tx = lined ? 0.f : 0.f;
    float ty = lined ? kPocketY : 18.f;
    float dx = tx - x_, dy = ty - y_;
    float dist = std::hypot(dx, dy);
    float desH = lined ? kNorth : std::atan2(dy, dx);
    float hErr = wrap(desH - heading_);
    rudder = std::clamp(hErr * 3.4f - yaw_ * 2.4f, -1.f, 1.f);
    float spd;
    if (lined && dist < 0.7f) spd = 0.f;
    else if (lined) spd = std::min(1.35f, std::max(0.4f, dist * 0.32f));
    else spd = std::min(4.8f, std::max(1.5f, dist * 0.45f));
    float wantVx = 0, wantVy = 0;
    if (dist > 0.15f && spd > 0.f) {
        wantVx = dx / dist * spd;
        wantVy = dy / dist * spd;
    }
    if (lined && dist < 3.f) {
        wantVx = std::clamp(-x_ * 0.9f, -0.45f, 0.45f);
        wantVy = std::clamp((ty - y_) * 0.55f, -0.35f, 0.55f);
    }
    float desSurge, desSway;
    worldToBody(heading_, wantVx - cx, wantVy - cy, desSurge, desSway);
    if (!lined && std::fabs(hErr) > 0.85f) desSurge = std::clamp(desSurge, -0.4f, 0.7f);
    thrust = std::clamp((desSurge - surge_) * 1.6f, -1.f, 1.f);
    bow = std::clamp((desSway - sway_) * 1.15f, -1.f, 1.f);
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
        {-18.f, 16.f, -5.15f, 52.f},
        {5.15f, 16.f, 18.f, 52.f},
        {-18.f, 48.5f, 18.f, 58.f},
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
            if (side == 0) dx = -(dl + 0.2f);
            if (side == 1) dx = dr + 0.2f;
            if (side == 2) dy = -(db + 0.2f);
            if (side == 3) dy = dt + 0.2f;
        }
    }
    return hit;
}

void Game::resolveWalls() {
    float c = std::cos(heading_), s = std::sin(heading_);
    const float fl[6] = {kHalfLen, kHalfLen, -kHalfLen, -kHalfLen, 0.f, 0.f};
    const float fb[6] = {kHalfBeam, -kHalfBeam, kHalfBeam, -kHalfBeam, kHalfBeam, -kHalfBeam};
    bool hit = false;
    for (int pass = 0; pass < 3; pass++) {
        for (int i = 0; i < 6; i++) {
            float px = x_ + c * fl[i] + s * fb[i];
            float py = y_ + s * fl[i] - c * fb[i];
            float dx, dy;
            if (!pushOut(px, py, dx, dy)) continue;
            x_ += dx;
            y_ += dy;
            hit = true;
        }
    }
    if (hit) {
        surge_ *= 0.22f;
        sway_ *= 0.22f;
        yaw_ *= 0.4f;
        shake_ = std::min(1.f, shake_ + 0.4f);
        if (!wasHit_) sys_->apu.noiseBurst(0.14f, 1400.f, 12.f);
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
    return hullInSlip() && std::fabs(wrap(kNorth - heading_)) < kAlign && speed_ < kWinSpeed;
}

void Game::physics(float dt, float thrust, float rudder, float bow) {
    float cx, cy;
    tideFlow(cx, cy);
    float wash = 0.5f + 0.5f * std::min(1.f, std::fabs(surge_) / 3.5f);
    float ahead = thrust >= 0.f ? thrust : thrust * 0.72f;
    surge_ += ahead * 6.2f * dt;
    surge_ -= surge_ * 0.85f * dt;
    sway_ += bow * 4.2f * dt;
    sway_ -= sway_ * 1.55f * dt;
    yaw_ += rudder * wash * 1.7f * dt;
    yaw_ -= yaw_ * 2.5f * dt;
    heading_ = wrap(heading_ + yaw_ * dt);
    float vx, vy;
    bodyToWorld(heading_, surge_, sway_, vx, vy);
    vx += cx;
    vy += cy;
    x_ += vx * dt;
    y_ += vy * dt;
    resolveWalls();
    bodyToWorld(heading_, surge_, sway_, vx, vy);
    speed_ = std::hypot(vx + cx, vy + cy);
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.05f);
    tone0_ = 0.1f;
}

void Game::horn() {
    sys_->apu.tone(1, 130.f, 0.07f);
    hornT_ = 0.45f;
}

void Game::chime() {
    chime_ = 4;
    chimeStep_ = 0;
    chimeT_ = 0;
}

void Game::audio(float dt) {
    if (tone0_ > 0.f) {
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
            static const float notes[] = {523.f, 659.f, 784.f, 1046.f};
            sys_->apu.tone(2, notes[chimeStep_ % 4], 0.06f);
            chimeStep_++;
            chimeT_ = 0.14f;
            if (chimeStep_ >= chime_) {
                chime_ = 0;
                tone0_ = std::max(tone0_, 0.2f);
            }
        }
    } else if (chimeT_ > 0.f) {
        chimeT_ -= dt;
        if (chimeT_ <= 0.f) sys_->apu.tone(2, 0, 0);
    }
    float eng = std::fabs(thrustIn_) * 0.03f + std::min(speed_, 4.f) * 0.008f;
    if (mode_ == Mode::Play && eng > 0.01f) sys_->apu.noise(eng, 700.f + speed_ * 80.f, true);
    else sys_->apu.noise(0, 0, false);
}

void Game::rivalPose(float& rx, float& ry, float& rh) const {
    float u = 1.f - std::clamp(clock_ / kCrew, 0.f, 1.f);
    float s = std::min(1.f, u * 1.15f);
    rx = -46.f + s * 34.f;
    ry = -18.f + s * 28.f;
    rh = std::atan2((12.f - ry), (0.f - rx));
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
                blip(90.f);
            } else if (clock_ < 12.f && std::floor(prev) != std::floor(clock_)) {
                blip(clock_ < 6.f ? 880.f : 640.f);
            }
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
        else if (pad.pressed(gs::BTN_MODE)) {
            begin();
            mode_ = Mode::Title;
        }
    } else if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
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
        camX_ = std::sin(t_ * 0.1f) * 3.f;
        camY_ = 24.f;
        zoom_ = kTitleZoom;
    } else {
        float lead = mode_ == Mode::Play ? 10.f : 0.f;
        float gx = x_ + std::cos(heading_) * lead;
        float gy = y_ + std::sin(heading_) * lead;
        float k = 1.f - std::exp(-kDt * 4.f);
        camX_ += (gx - camX_) * k;
        camY_ += (gy - camY_) * k;
        zoom_ += (kPlayZoom - zoom_) * k;
        if (shake_ > 0.f) {
            camX_ += std::sin(t_ * 40.f) * shake_ * 2.f;
            shake_ *= 0.92f;
        }
    }
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

int Game::shipFrame(float h) const {
    float u = h;
    while (u < 0.f) u += kTau;
    while (u >= kTau) u -= kTau;
    int i = int(std::lround(u / kTau * 12.f)) % 12;
    if (i < 0) i += 12;
    return i;
}

int Game::clockFrame() const {
    float u = 1.f - std::clamp(clock_ / kCrew, 0.f, 1.f);
    return std::clamp(int(std::lround(u * 5.f)), 0, 5);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
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
    sys_->vdp.sprite(s);
}

void Game::place(const gs::Mipped& m, float wx, float wy, float worldH, int pal) {
    float sx, sy;
    worldToScreen(wx, wy, sx, sy);
    spr(m, sx, sy, worldH * zoom_, pal);
}

void Game::worldToScreen(float wx, float wy, float& sx, float& sy) const {
    sx = 160.f + (wx - camX_) * zoom_;
    sy = 112.f - (wy - camY_) * zoom_;
}

void Game::drawHud() {
    char buf[64];
    int left = std::max(0, int(clock_));
    int pal = left < 14 ? PAL_ALERT : PAL_BANNER;
    std::snprintf(buf, sizeof buf, "CREW %02d:%02d", left / 60, left % 60);
    if (mode_ == Mode::Title) {
        hudC(17, "BERTH IN THE SLIP BEFORE THE TIDE TURNS", PAL_BANNER);
        hudC(19, "THE CLOCK IS THE OTHER CREW", PAL_ALERT);
        hudC(21, "ARROWS ENGINE AND RUDDER", PAL_HUD);
        hudC(22, "Z BOW PORT   X BOW STARBOARD", PAL_HUD);
        hudC(23, "C HORN", PAL_HUD);
        if ((int(t_ * 2.f) & 1) == 0) hudC(25, "RETURN", PAL_WIN);
        hud(1, 0, buf, pal);
        return;
    }
    hud(1, 0, "S3 FERRY SLIP", PAL_BANNER);
    hud(28, 0, buf, pal);
    if (mode_ == Mode::Pause) {
        hudC(16, "RETURN CONTINUES", PAL_HUD);
        hudC(17, "ESC TO THE TITLE", PAL_DIM);
        return;
    }
    if (mode_ == Mode::Win) {
        std::snprintf(buf, sizeof buf, "CREW %02d:%02d LEFT", left / 60, left % 60);
        hudC(16, "LINES ARE ON", PAL_WIN);
        hudC(17, buf, PAL_HUD);
        if (!bot_) hudC(19, "RETURN SAILS AGAIN", PAL_DIM);
        return;
    }
    if (mode_ == Mode::Fail) {
        hudC(16, "THE OTHER CREW TOOK THE BERTH", PAL_ALERT);
        if (!bot_) hudC(18, "RETURN TRIES AGAIN", PAL_HUD);
        return;
    }
    std::snprintf(buf, sizeof buf, "SPD %04.1f  %s", speed_, compass(heading_));
    hud(1, 1, buf, PAL_HUD);
    if (madeFast()) hud(1, 26, "HOLD FOR THE LINES", PAL_WIN);
    else if (hullInSlip()) hud(1, 26, "SLOW AND SQUARE", PAL_BANNER);
    else if (y_ > 8.f && std::fabs(x_) < 10.f) hud(1, 26, "EASE INTO THE SLIP", PAL_HUD);
    else hud(1, 26, "BEAT THE OTHER CREW", PAL_HUD);
    hud(1, 27, "Z PORT BOW  X STBD BOW  C HORN", PAL_DIM);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    float tideU = 1.f - std::clamp(clock_ / kCrew, 0.f, 1.f);
    const uint16_t deep = lerpC(gs::rgb4(1, 4, 9), gs::rgb4(3, 5, 6), tideU);
    const uint16_t mid = lerpC(gs::rgb4(2, 8, 12), gs::rgb4(6, 8, 6), tideU);
    const uint16_t spark = gs::rgb4(8, 13, 14);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (112.f - y) / std::max(zoom_, 0.05f);
        float shore = std::clamp((wy + 40.f) / 90.f, 0.f, 1.f);
        uint16_t water = lerpC(deep, mid, shore);
        float shimmer = 0.5f + 0.5f * std::sin(y * 0.09f + t_ * 1.4f);
        water = lerpC(water, spark, shimmer * 0.08f);
        v.lineBackdrop[y] = water;
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }

    if (mode_ == Mode::Title) spr(art_.title, 160.f, 36.f, float(art_.title.h), PAL_BANNER);
    else if (mode_ == Mode::Win) spr(art_.berthed, 160.f, 70.f, float(art_.berthed.h), PAL_WIN);
    else if (mode_ == Mode::Fail) spr(art_.turned, 160.f, 70.f, float(art_.turned.h), PAL_ALERT);
    else if (mode_ == Mode::Pause) spr(art_.paused, 160.f, 70.f, float(art_.paused.h), PAL_BANNER);
    spr(art_.clock[clockFrame()], 292.f, 22.f, 28.f, PAL_CLOCK);

    place(art_.pier, -11.2f, 34.f, 40.f, PAL_PIER);
    place(art_.pier, 11.2f, 34.f, 40.f, PAL_PIER);
    place(art_.shed, 0.f, 62.f, 16.f, PAL_PIER);
    for (int i = 0; i < 4; i++) {
        place(art_.post, -7.2f, 16.f + i * 12.f, 5.f, PAL_POST);
        place(art_.post, 7.2f, 16.f + i * 12.f, 5.f, PAL_POST);
    }

    float rx, ry, rh;
    rivalPose(rx, ry, rh);
    place(art_.rival[shipFrame(rh)], rx, ry, 22.f, PAL_RIVAL);

    float bsx, bsy;
    worldToScreen(x_, y_, bsx, bsy);
    spr(art_.hull[shipFrame(heading_)], bsx, bsy + std::sin(t_ * 2.f) * 0.6f, kDrawH * zoom_, PAL_FERRY);

    if (speed_ > 0.4f && mode_ == Mode::Play) {
        foamT_ += kDt;
        if (foamT_ > 0.12f) foamT_ = 0;
        float fx = x_ - std::cos(heading_) * 7.f;
        float fy = y_ - std::sin(heading_) * 7.f;
        place(art_.foam, fx, fy, 3.5f, PAL_FOAM);
    }
    drawHud();
}

}  // namespace ferryslip

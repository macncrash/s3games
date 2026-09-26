#include "plat.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace tugplat {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;

constexpr float kFace = 18.f;
constexpr float kHalfW = 2.85f;
constexpr float kHalfL = 8.2f;
constexpr float kClear = 2.66f;
constexpr float kBerthX = kFace - kClear - kHalfW;
constexpr float kMarkY = 156.f;
constexpr float kY0 = 128.f;
constexpr float kY1 = 184.f;
constexpr float kHead = 206.f;
constexpr float kSouth = 4.f;
constexpr float kWest = -46.f;
constexpr float kDeckW = 10.f;
constexpr float kLevelTol = 1.70f;
constexpr float kGapTol = 0.90f;
constexpr float kHdgTol = 0.16f;
constexpr float kStopSpd = 0.70f;
constexpr float kHoldNeed = 0.80f;
constexpr float kCrew = 52.f;
constexpr float kTideX = 0.f;
constexpr float kTideY = 0.38f;
constexpr float kCapF = 7.4f;
constexpr float kCapR = 4.8f;
constexpr float kPlayZoom = 2.75f;
constexpr float kCloseZoom = 3.15f;
constexpr float kTitleZoom = 1.08f;
constexpr float kTitleCamX = 9.f;
constexpr float kTitleCamY = 146.f;
constexpr float kRivalY0 = 248.f;

float wrap(float a) {
    while (a > kPi) a -= kTau;
    while (a < -kPi) a += kTau;
    return a;
}

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    return gs::rgb4(int(ar + (br - ar) * t), int(ag + (bg - ag) * t), int(ab + (bb - ab) * t));
}

}  // namespace

float Game::crewLeft() const { return std::max(0.f, kCrew - race_); }

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (hold_ > 0.05f) return 3;
    if (std::fabs(y_ - kMarkY) < 16.f && std::fabs(x_ - kBerthX) < 6.f) return 2;
    return 1;
}

int Game::yawOf(float h) const {
    float u = std::fmod(h, kTau);
    if (u < 0.f) u += kTau;
    int i = int(std::lround(u / kTau * float(kYaws))) % kYaws;
    if (i < 0) i += kYaws;
    return i;
}

bool Game::sampleHull(float& east, const char*& why) const {
    east = -1e9f;
    why = "";
    const float c = std::cos(heading_), s = std::sin(heading_);
    bool hit = false;
    auto consider = [&](float lx, float ly) {
        float wx = x_ + ly * c + lx * s;
        float wy = y_ + ly * s - lx * c;
        if (wx > east) east = wx;
        if (wx < kFace) return;
        if (!hit) why = (wy >= kY0 && wy <= kY1) ? "scraped the platform" : "on the bank";
        hit = true;
    };
    const float lx[3] = {-kHalfW, 0.f, kHalfW};
    for (float x : lx) {
        consider(x, kHalfL);
        consider(x, -kHalfL);
    }
    consider(kHalfW, 0.f);
    consider(-kHalfW, 0.f);
    if (!hit) {
        float north = y_ + std::fabs(s) * kHalfL + std::fabs(c) * kHalfW;
        if (north > kHead) {
            why = "past the head";
            hit = true;
        }
    }
    return hit;
}

bool Game::inSlot(bool& yOk, bool& xOk, bool& hOk, bool& vOk) const {
    yOk = std::fabs(y_ - kMarkY) <= kLevelTol;
    xOk = std::fabs(x_ - kBerthX) <= kGapTol;
    hOk = std::fabs(wrap(heading_ - kPi * 0.5f)) <= kHdgTol;
    vOk = ground_ <= kStopSpd;
    return yOk && xOk && hOk && vOk;
}

void Game::rivalAt(float& x, float& y, float& h) const {
    float u = std::clamp(race_ / kCrew, 0.f, 1.f);
    y = kRivalY0 + (kMarkY - kRivalY0) * u;
    float tuck = std::clamp((u - 0.88f) / 0.12f, 0.f, 1.f);
    x = (kBerthX - 13.f) * (1.f - tuck) + (kBerthX - 4.2f) * tuck;
    h = -kPi * 0.5f;
}

const char* Game::hint() const {
    if (crewLeft() <= 12.f) return "THE OTHER CREW IS CLOSING";
    float xErr = kBerthX - x_;
    float yErr = kMarkY - y_;
    bool yOk, xOk, hOk, vOk;
    if (inSlot(yOk, xOk, hOk, vOk)) return "HOLD HER LEVEL";
    if (std::fabs(yErr) > 24.f) return "BRING HER UP TO THE PLATFORM";
    if (!yOk) return yErr > 0.f ? "SHORT — NOT LEVEL" : "LONG — NOT LEVEL";
    if (!xOk) return xErr > 0.f ? "TOO WIDE OF THE FACE" : "TOO TIGHT ON THE FACE";
    if (!hOk) return "SQUARE HER TO THE FACE";
    if (!vOk) return "KILL THE WAY — FLOOD IS ON";
    return "CLOSE IS NOT LEVEL";
}

void Game::begin() {
    x_ = 3.2f;
    y_ = 72.f;
    heading_ = 1.16f;
    speed_ = 0.f;
    throttle_ = 0.f;
    gvx_ = 0.f;
    gvy_ = 0.f;
    ground_ = 0.f;
    race_ = 0.f;
    hold_ = 0.f;
    wasSlot_ = false;
    won_ = false;
    over_ = false;
    why_ = "";
    hornT_ = 0.f;
    blipT_ = 0.f;
    chimeT_ = 0.f;
    chimeN_ = 0;
    chimeStep_ = 0;
    smokeT_ = 0.f;
    wakeT_ = 0.f;
    smokeN_ = 0;
    wakeN_ = 0;
    for (Puff& p : smokes_) p = {};
    for (Puff& p : wakes_) p = {};
}

void Game::showTitle() {
    begin();
    mode_ = Mode::Title;
    camX_ = kTitleCamX;
    camY_ = kTitleCamY;
    zoom_ = kTitleZoom;
}

void Game::startRun() {
    begin();
    mode_ = Mode::Run;
    camX_ = x_;
    camY_ = y_;
    zoom_ = kPlayZoom;
    horn(0.32f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(1, 4, 7));
    sys.apu.setMaster(0.74f);
    sys.apu.setEcho(0.13f, 0.20f, 0.10f);
    t_ = 0.f;
    if (bot_) startRun();
    else showTitle();
    sys.setLight(24, 70, 110);
}

void Game::controls(float& steer, float& throttle) {
    const gs::Pad& p = sys_->pad;
    steer = 0.f;
    if (p.down(gs::BTN_LEFT)) steer += 1.f;
    if (p.down(gs::BTN_RIGHT)) steer -= 1.f;
    if (std::fabs(p.axisX) > 0.18f) steer = std::clamp(-p.axisX, -1.f, 1.f);
    const bool up = p.down(gs::BTN_UP) || p.down(gs::BTN_C) || p.down(gs::BTN_A);
    const bool down = p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.down(gs::BTN_X);
    if (up) throttle_ = std::min(1.f, throttle_ + kDt * 0.62f);
    if (down) throttle_ = std::max(-1.f, throttle_ - kDt * 0.82f);
    if (p.accel > 0.05f) throttle_ = std::min(1.f, throttle_ + p.accel * kDt * 1.15f);
    if (p.brake > 0.05f) throttle_ = std::max(-1.f, throttle_ - p.brake * kDt * 1.25f);
    if (std::fabs(p.axisY) > 0.22f) throttle_ += (std::clamp(p.axisY, -1.f, 1.f) - throttle_) * std::min(1.f, kDt * 3.f);
    if (p.pressed(gs::BTN_Z) || p.pressed(gs::BTN_Y)) horn(0.42f);
    throttle = throttle_;
}

void Game::pose(float& steer, float& throttle) const {
    const float xErr = kBerthX - x_;
    const float yErr = kMarkY - y_;
    // Inside the slot, face north and balance the flood. A bigger crab swings the stern in.
    if (std::fabs(xErr) < kGapTol && std::fabs(yErr) < kLevelTol + 0.6f) {
        float s = -kTideY + std::clamp(yErr * 0.7f, -0.45f, 0.55f);
        float hdes = kPi * 0.5f + std::clamp(-xErr * 0.05f, -0.06f, 0.06f);
        float err = wrap(hdes - heading_);
        steer = std::clamp(err / 0.12f, -1.f, 1.f);
        float cap = s >= 0.f ? kCapF : kCapR;
        throttle = std::clamp(s / cap, -1.f, 1.f);
        if (std::cos(err) < 0.5f) throttle *= 0.25f;
        return;
    }
    float approach = std::clamp(yErr * 0.42f, -1.4f, 1.35f);
    float s = std::clamp(approach - kTideY, -1.8f, 1.35f);
    float lim = std::fabs(yErr) < 8.f ? 0.10f : 0.16f;
    float a = 0.f;
    if (std::fabs(s) > 0.25f) a = std::asin(std::clamp(-0.7f * xErr / s, -lim, lim));
    float hdes = kPi * 0.5f + a;
    float err = wrap(hdes - heading_);
    steer = std::clamp(err / 0.14f, -1.f, 1.f);
    float cap = s >= 0.f ? kCapF : kCapR;
    throttle = std::clamp(s / cap, -1.f, 1.f);
    if (std::cos(err) < 0.45f) throttle *= 0.2f;
}

void Game::pilot(float& steer, float& throttle) const {
    float east = 0.f;
    const char* hit = "";
    sampleHull(east, hit);
    if (east > kFace - 0.55f) {
        float err = wrap(kPi * 0.5f - heading_);
        steer = std::clamp(err / 0.16f, -1.f, 1.f);
        throttle = -0.7f;
        return;
    }
    if (y_ + std::fabs(std::sin(heading_)) * kHalfL > kHead - 8.f) {
        float err = wrap(kPi * 0.5f - heading_);
        steer = std::clamp(err / 0.16f, -1.f, 1.f);
        throttle = -0.85f;
        return;
    }

    const float xErr = kBerthX - x_;
    const float yErr = kMarkY - y_;
    if (std::fabs(xErr) < 2.4f && yErr < 18.f) {
        pose(steer, throttle);
        return;
    }

    // Aim a point on the berth line just ahead, so the bow is squared before the platform.
    float aimX = kBerthX;
    float aimY = y_ + 22.f;
    if (std::fabs(xErr) < 0.8f) aimY = kMarkY;
    if (aimY > kMarkY) aimY = kMarkY;
    if (x_ > kBerthX + 0.2f) aimX = kBerthX - 1.8f;
    float want = std::fabs(xErr) > 1.6f ? 5.0f : 4.6f;
    if (yErr < 48.f) want = 3.0f;
    if (yErr < 26.f) want = 1.8f;

    float hdes = std::atan2(aimY - y_, aimX - x_);
    if (y_ > kY0 - 24.f) {
        float off = std::clamp(wrap(hdes - kPi * 0.5f), -0.40f, 0.16f);
        hdes = kPi * 0.5f + off;
    }
    float err = wrap(hdes - heading_);
    steer = std::clamp(err / 0.18f, -1.f, 1.f);
    float along = std::max(0.4f, std::sin(heading_));
    float spd = (want - kTideY) / along;
    if (std::fabs(err) > 0.45f) spd = std::min(spd, 1.7f);
    throttle = std::clamp(spd / kCapF, -0.35f, 0.88f);
    if (std::cos(err) < 0.25f) throttle = std::min(throttle, 0.12f);
}

void Game::physics(float steer, float throttle) {
    float rate = std::fabs(speed_) < 1.35f ? 2.15f : 1.62f;
    heading_ = wrap(heading_ + steer * rate * kDt);
    float cap = throttle >= 0.f ? kCapF : kCapR;
    float target = throttle * cap;
    speed_ += (target - speed_) * (1.f - std::exp(-2.5f * kDt));
    speed_ = std::clamp(speed_, -kCapR, kCapF);
    float c = std::cos(heading_), s = std::sin(heading_);
    gvx_ = c * speed_ + kTideX;
    gvy_ = s * speed_ + kTideY;
    x_ += gvx_ * kDt;
    y_ += gvy_ * kDt;
    ground_ = std::hypot(gvx_, gvy_);
    if (x_ < kWest) {
        x_ = kWest;
        if (gvx_ < 0.f) speed_ *= 0.25f;
    }
    if (y_ < kSouth) {
        y_ = kSouth;
        if (gvy_ < 0.f) speed_ *= 0.25f;
    }

    float east = 0.f;
    const char* hit = "";
    if (sampleHull(east, hit)) {
        fail(hit);
        return;
    }

    bool yOk, xOk, hOk, vOk;
    bool slot = inSlot(yOk, xOk, hOk, vOk);
    if (slot) hold_ += kDt;
    else hold_ = 0.f;
    if (hold_ >= kHoldNeed) {
        win();
        return;
    }
    if (slot && !wasSlot_) blip(760.f);
    wasSlot_ = slot;
}

void Game::win() {
    if (won_ || mode_ != Mode::Run) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    why_ = "level";
    chime();
    sys_->rumble(0.32f, 0.16f, 180);
    sys_->setLight(50, 190, 80);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    why_ = why;
    sys_->apu.noiseBurst(0.42f, 80.f, 0.4f);
    sys_->apu.tone(0, 70.f, 0.06f);
    blipT_ = 0.35f;
    sys_->rumble(0.55f, 0.12f, 160);
    sys_->setLight(180, 36, 28);
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.046f);
    blipT_ = 0.09f;
}

void Game::horn(float seconds) {
    sys_->apu.tone(1, 74.f, 0.07f);
    hornT_ = seconds;
}

void Game::chime() {
    chimeN_ = 4;
    chimeStep_ = 0;
    chimeT_ = 0.02f;
}

void Game::audio() {
    float water = (mode_ == Mode::Run ? 0.018f : 0.011f) + std::fabs(speed_) * 0.0007f;
    sys_->apu.noise(water, 460.f, false);
    if (mode_ == Mode::Run && (std::fabs(throttle_) > 0.04f || std::fabs(speed_) > 0.7f)) {
        float wob = 0.74f + 0.26f * std::sin(t_ * (12.f + std::fabs(throttle_) * 18.f));
        float vol = (0.012f + std::fabs(throttle_) * 0.028f) * wob;
        sys_->apu.tone(2, 40.f + std::fabs(throttle_) * 26.f + std::fabs(speed_) * 0.35f, vol);
    } else {
        sys_->apu.tone(2, 0.f, 0.f);
    }
    if (hornT_ > 0.f) {
        hornT_ -= kDt;
        if (hornT_ <= 0.f) sys_->apu.tone(1, 0.f, 0.f);
    }
    if (chimeN_ > 0) {
        chimeT_ -= kDt;
        if (chimeT_ <= 0.f) {
            static const float notes[] = {196.f, 247.f, 294.f, 392.f};
            sys_->apu.tone(0, notes[std::min(chimeStep_, 3)], 0.055f);
            blipT_ = 0.16f;
            chimeT_ = 0.15f;
            if (++chimeStep_ >= chimeN_) chimeN_ = 0;
        }
    } else if (blipT_ > 0.f) {
        blipT_ -= kDt;
        if (blipT_ <= 0.f) sys_->apu.tone(0, 0.f, 0.f);
    }
}

void Game::puffs() {
    for (Puff& p : smokes_)
        if (p.life > 0.f) p.life -= kDt * 0.42f;
    for (Puff& p : wakes_) {
        if (p.life <= 0.f) continue;
        p.life -= kDt * 0.55f;
        p.y += kTideY * kDt * 0.35f;
    }
    bool live = mode_ == Mode::Title || mode_ == Mode::Run || mode_ == Mode::Win;
    if (!live) return;
    smokeT_ -= kDt;
    float push = mode_ == Mode::Title ? 0.4f : std::fabs(throttle_);
    if (smokeT_ <= 0.f && (push > 0.06f || std::fabs(speed_) > 1.2f)) {
        smokeT_ = mode_ == Mode::Title ? 0.16f : 0.10f;
        float c = std::cos(heading_), s = std::sin(heading_);
        Puff p;
        p.x = x_ - c * 6.2f;
        p.y = y_ - s * 6.2f;
        p.life = 1.f;
        smokes_[smokeN_] = p;
        smokeN_ = (smokeN_ + 1) % 8;
    }
    wakeT_ -= kDt;
    if (wakeT_ <= 0.f && (mode_ == Mode::Run || mode_ == Mode::Win) && (std::fabs(speed_) > 1.6f || ground_ > 2.f)) {
        wakeT_ = 0.09f;
        float c = std::cos(heading_), s = std::sin(heading_);
        Puff w;
        w.x = x_ - c * kHalfL * 0.8f;
        w.y = y_ - s * kHalfL * 0.8f;
        w.life = 1.f;
        wakes_[wakeN_] = w;
        wakeN_ = (wakeN_ + 1) % 12;
    }
}

void Game::camera() {
    if (mode_ == Mode::Title) {
        camX_ = kTitleCamX;
        camY_ = kTitleCamY;
        zoom_ = kTitleZoom;
        return;
    }
    float lead = mode_ == Mode::Run ? 5.5f : 0.f;
    float gx = x_ + std::cos(heading_) * lead;
    float gy = y_ + std::sin(heading_) * lead;
    float gz = kPlayZoom;
    if (mode_ == Mode::Run && std::fabs(kMarkY - y_) < 42.f) gz = kCloseZoom;
    if (mode_ == Mode::Win) {
        gx = (x_ + kBerthX) * 0.5f;
        gy = kMarkY;
        gz = 3.25f;
    } else if (mode_ == Mode::Fail) {
        gz = 2.9f;
    }
    float k = 1.f - std::exp(-kDt * (mode_ == Mode::Run ? 3.8f : 2.5f));
    camX_ += (gx - camX_) * k;
    camY_ += (gy - camY_) * k;
    zoom_ += (gz - zoom_) * k;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    if (cx + w * 0.5f < -12.f || cy + h * 0.5f < -12.f || cx - w * 0.5f > gs::SCREEN_W + 12.f ||
        cy - h * 0.5f > gs::SCREEN_H + 12.f)
        return;
    gs::Sprite spt;
    long sw = std::clamp(std::lround(w), 1L, 1800L);
    long sh = std::clamp(std::lround(h), 1L, 1800L);
    spt.w = int16_t(sw);
    spt.h = int16_t(sh);
    spt.x = int16_t(std::clamp(std::lround(cx - sw * 0.5f), -2000L, 2000L));
    spt.y = int16_t(std::clamp(std::lround(cy - sh * 0.5f), -2000L, 2000L));
    spt.img = m.pick(float(sh));
    spt.pal = uint8_t(pal);
    spt.shadow = shadow;
    sys_->vdp.sprite(spt);
}

void Game::place(const gs::Mipped& m, float wx, float wy, float worldH, int pal, bool shadow) {
    float sx = 160.f + (wx - camX_) * zoom_;
    float sy = 112.f - (wy - camY_) * zoom_;
    spr(m, sx, sy, std::max(1.f, worldH * zoom_), pal, shadow);
}

void Game::text(const char* s, float x, float y, float scale, int pal) {
    if (!s || !*s) return;
    const float adv = 18.f * scale;
    const float left = x - float(std::strlen(s)) * adv * 0.5f;
    for (int i = 0; s[i]; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, left + (float(i) + 0.5f) * adv, y, float(g.h) * scale, pal);
    }
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

void Game::hudR(int row, const char* s, int pal) {
    int n = 0;
    if (s)
        while (s[n]) n++;
    hud(40 - n, row, s, pal);
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    v.setFogColor(gs::rgb4(1, 3, 6));
    float zoom = std::max(zoom_, 0.25f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (112.f - float(y)) / zoom;
        float depth = std::clamp((wy - 40.f) / 220.f, 0.f, 1.f);
        uint16_t water = lerpC(gs::rgb4(2, 8, 11), gs::rgb4(1, 3, 6), depth);
        float shim = std::sin(wy * 0.31f + t_ * 1.5f);
        if (shim > 0.93f) water = lerpC(water, gs::rgb4(10, 14, 15), 0.32f);
        v.lineBackdrop[y] = water;
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::drawBoat(float wx, float wy, float hdg, int pal, float bob) {
    const gs::Mipped& m = art_.tug[yawOf(hdg)];
    float dest = (kHalfL * 2.f) * zoom_ * (float(m.h) / kPaintLen);
    float cx = 160.f + (wx - camX_) * zoom_;
    float cy = 112.f - (wy - camY_) * zoom_ + bob;
    spr(m, cx, cy, dest, pal, false);
    spr(art_.shadow, cx + 2.4f, cy + dest * 0.06f, dest * 0.30f, PAL_FX, true);
}

void Game::drawWorld() {
    float deckX = kFace + kDeckW * 0.5f;
    place(art_.platform, deckX, kMarkY, kY1 - kY0, PAL_TIMBER);
    for (float y = 6.f; y < 236.f; y += 12.f) place(art_.plank, deckX, y, 12.f, PAL_TIMBER);
    for (float y = 6.f; y < 236.f; y += 12.f) place(art_.yard, kFace + kDeckW + 8.f, y, 12.f, PAL_YARD);
    place(art_.crane, kFace + 4.2f, kMarkY + 9.f, 13.f, PAL_CRANE);
    place(art_.shed, kFace + kDeckW + 3.2f, kMarkY - 18.f, 8.f, PAL_SHED);
    place(art_.shed, kFace + kDeckW + 2.4f, kMarkY + 24.f, 7.2f, PAL_SHED);
    place(art_.lamp, kFace + 1.3f, kMarkY, 4.2f, PAL_LAMP);
    place(art_.lamp, kFace + 8.2f, kMarkY, 4.6f, PAL_LAMP);
    place(art_.diamond, kBerthX, kMarkY, 3.6f, PAL_MARK);
    for (float y = 16.f; y < 228.f; y += 14.f) {
        if (y > kY0 - 2.f && y < kY1 + 2.f && std::fabs(y - kMarkY) < 6.f) continue;
        place(art_.pile, kFace - 0.15f, y, 2.15f, PAL_TIMBER);
    }
    for (int i = 0; i < 5; i++) place(art_.pile, -8.f + float(i) * 5.2f, kHead, 2.5f, PAL_TIMBER);
    place(art_.buoy, kBerthX - 7.f, kY0 - 10.f, 3.3f, PAL_BUOY);
    place(art_.buoy, -14.f, 96.f, 3.f, PAL_BUOY);
    int flap = int(t_ * 3.4f) & 1;
    place(art_.gull[flap], -6.f + std::sin(t_ * 0.37f) * 10.f, 118.f, 2.4f, PAL_BIRD);
    place(art_.gull[1 - flap], 6.f + std::cos(t_ * 0.29f) * 8.f, 210.f, 2.1f, PAL_BIRD);
    place(art_.gull[flap], -20.f + std::sin(t_ * 0.51f) * 6.f, 54.f, 1.8f, PAL_BIRD);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.hudEnabled = true;
    backdrop();

    bool yOk = false, xOk = false, hOk = false, vOk = false;
    bool slot = mode_ != Mode::Title && inSlot(yOk, xOk, hOk, vOk);
    v.setColor(PAL_LAMP * 16 + 1, (slot || mode_ == Mode::Win) ? gs::rgb4(3, 15, 5) : gs::rgb4(15, 10, 2));

    const char* big = nullptr;
    int bigPal = PAL_AMBER;
    if (mode_ == Mode::Title) {
        big = "S3 TUGBOAT PLAT";
    } else if (mode_ == Mode::Win) {
        big = "LEVEL";
        bigPal = PAL_GREEN;
    } else if (mode_ == Mode::Fail) {
        big = "TOO LATE";
        bigPal = PAL_RED;
        if (why_ && std::strcmp(why_, "scraped the platform") == 0) big = "SCRAPED";
        else if (why_ && std::strcmp(why_, "on the bank") == 0) big = "BANKED";
        else if (why_ && std::strcmp(why_, "past the head") == 0) big = "TOO FAR";
    } else if (mode_ == Mode::Pause) {
        big = "PAUSED";
        bigPal = PAL_HUD;
    }
    if (big) text(big, 160.f, mode_ == Mode::Title ? 16.f : 22.f, mode_ == Mode::Win ? 1.05f : 0.58f, bigPal);
    if (mode_ == Mode::Title) {
        text("STOP LEVEL WITH THE PLATFORM", 160.f, 36.f, 0.36f, PAL_AMBER);
        text("THE CLOCK IS THE OTHER CREW", 160.f, 52.f, 0.32f, PAL_HUD);
    }

    float rx, ry, rh;
    rivalAt(rx, ry, rh);
    float bob = std::sin(t_ * 2.15f) * 0.8f;
    float rbob = std::sin(t_ * 1.7f + 1.2f) * 0.6f;
    drawBoat(x_, y_, heading_, PAL_TUG, bob);
    drawBoat(rx, ry, rh, PAL_RIVAL, rbob);
    float rsx = 160.f + (rx - camX_) * zoom_;
    float rsy = 112.f - (ry - camY_) * zoom_;
    if (rsy > 18.f && rsy < gs::SCREEN_H - 18.f && mode_ != Mode::Title) text("CREW", rsx, rsy - 16.f, 0.34f, PAL_AMBER);

    if ((hold_ > 0.05f || mode_ == Mode::Win) && mode_ != Mode::Title) {
        float mx = (x_ + kFace) * 0.5f;
        place(art_.line, mx, y_, 1.15f, PAL_FX);
    }
    for (const Puff& p : smokes_) {
        if (p.life <= 0.f) continue;
        float h = (2.2f + (1.f - p.life) * 2.4f) * (zoom_ / kPlayZoom);
        float sx = 160.f + (p.x - camX_) * zoom_;
        float sy = 112.f - (p.y - camY_) * zoom_ - (1.f - p.life) * 10.f;
        spr(art_.smoke, sx, sy, std::max(2.f, h), PAL_FX);
    }
    for (const Puff& p : wakes_) {
        if (p.life <= 0.f) continue;
        float h = (2.f + (1.f - p.life) * 2.6f) * (zoom_ / kPlayZoom);
        float sx = 160.f + (p.x - camX_) * zoom_;
        float sy = 112.f - (p.y - camY_) * zoom_;
        spr(art_.wake, sx, sy, std::max(2.f, h), PAL_FX);
    }

    // Shore and the marked platform sit under the hull.
    drawWorld();

    char buf[64];
    int sec = std::max(0, int(std::ceil(crewLeft() - 1e-3f)));
    std::snprintf(buf, sizeof buf, "CREW %d:%02d", sec / 60, sec % 60);
    int crewPal = sec <= 12 ? PAL_RED : PAL_AMBER;

    if (mode_ == Mode::Run || mode_ == Mode::Pause) {
        hud(1, 0, "S3 TUGBOAT PLAT", PAL_AMBER);
        hudR(0, buf, crewPal);
        hud(1, 1, hint(), sec <= 12 ? PAL_RED : PAL_HUD);
        std::snprintf(buf, sizeof buf, "WAY %.1f", ground_);
        hudR(1, buf, PAL_HUD);
        auto lamp = [&](bool ok) {
            if (ok) return PAL_GREEN;
            if (std::fabs(kMarkY - y_) < 18.f) return PAL_RED;
            return PAL_HUD;
        };
        hud(1, 2, "LEVEL", lamp(yOk));
        hud(8, 2, "GAP", lamp(xOk));
        hud(13, 2, "SQUARE", lamp(hOk));
        hud(21, 2, "STOP", lamp(vOk));
        std::snprintf(buf, sizeof buf, "TEL %+.1f", throttle_);
        hudR(2, buf, PAL_HUD);
        if (hold_ > 0.f) {
            int n = std::clamp(int(hold_ / kHoldNeed * 8.f + 0.2f), 1, 8);
            char bar[16] = "HOLD ";
            for (int i = 0; i < n && i < 8; i++) bar[5 + i] = '#';
            bar[5 + n] = 0;
            hudC(24, bar, PAL_GREEN);
        }
        hud(1, 26, "ARROWS DRIVE    Z HORN", PAL_HUD);
    } else if (mode_ == Mode::Title) {
        hudC(23, "CLOSE IS NOT LEVEL", PAL_HUD);
        hudC(26, int(t_ * 2.f) % 2 == 0 ? "ENTER TO TAKE THE TUG" : "ARROWS STEER AND DRIVE", PAL_AMBER);
    } else if (mode_ == Mode::Win) {
        hudC(6, "STOPPED LEVEL", PAL_GREEN);
        hudC(7, "AHEAD OF THE OTHER CREW", PAL_HUD);
        int took = int(std::lround(race_));
        int left = int(std::floor(crewLeft()));
        std::snprintf(buf, sizeof buf, "%d:%02d   CREW HAD %d:%02d", took / 60, took % 60, left / 60, left % 60);
        hudC(9, buf, PAL_AMBER);
        hudC(26, "ENTER TAKES THE NEXT BERTH", PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        hudC(6, why_ && why_[0] ? why_ : "MISSED THE PLATFORM", PAL_RED);
        hudC(26, "ENTER TRIES THE BERTH AGAIN", PAL_HUD);
    }
    hudR(27, S3_VERSION_STRING, PAL_HUD);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) startRun();
        else if (pad.pressed(gs::BTN_MODE)) sys.quit();
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) showTitle();
        else {
            race_ += kDt;
            float steer = 0.f, thr = throttle_;
            if (bot_) pilot(steer, thr);
            else controls(steer, thr);
            throttle_ = thr;
            physics(steer, thr);
            if (mode_ == Mode::Run && race_ >= kCrew) fail("the other crew took the platform");
            if (mode_ == Mode::Run) {
                bool yOk, xOk, hOk, vOk;
                if (inSlot(yOk, xOk, hOk, vOk)) sys.setLight(40, 180, 70);
                else if (crewLeft() < 12.f) sys.setLight(180, 40, 30);
                else if (std::fabs(y_ - kMarkY) < 18.f) sys.setLight(170, 120, 36);
                else sys.setLight(20, 60, 110);
            }
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Run;
        else if (pad.pressed(gs::BTN_MODE)) showTitle();
    } else if (!bot_) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) startRun();
        else if (pad.pressed(gs::BTN_MODE)) showTitle();
    }
    puffs();
    camera();
    audio();
    draw();
}

}  // namespace tugplat

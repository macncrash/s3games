#include "skiff.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace skiffplat {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;

constexpr float kTideX = 0.10f;
constexpr float kTideY = -0.66f;
constexpr float kMarkX = 7.40f;
constexpr float kMarkY = 168.f;
constexpr float kDeckL = 50.f;
constexpr float kDeckW = 22.f;
constexpr float kPlatEdge = 14.f;
constexpr float kDeckCx = kPlatEdge + kDeckW * 0.5f;
constexpr float kPlatY0 = kMarkY - kDeckL * 0.5f;
constexpr float kPlatY1 = kMarkY + kDeckL * 0.5f;
constexpr float kBankX = 30.f;
constexpr float kShoalY = 208.f;
constexpr float kSouth = 16.f;
constexpr float kWest = -48.f;
constexpr float kHalfW = 3.15f;
constexpr float kHalfL = 7.00f;
constexpr float kLevelTol = 2.70f;
constexpr float kGapTol = 1.10f;
constexpr float kHdgTol = 0.24f;
constexpr float kStopSpd = 0.92f;
constexpr float kHoldNeed = 0.72f;
constexpr float kTideClock = 64.f;
constexpr float kCapFwd = 12.5f;
constexpr float kCapRev = 6.5f;
constexpr float kBoatH = 20.5f;
constexpr float kPlayZoom = 2.22f;
constexpr float kTitleZoom = 1.55f;
constexpr float kTitleCamX = 16.f;
constexpr float kTitleCamY = 139.f;

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

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (hold_ > 0.05f) return 3;
    if (std::fabs(y_ - kMarkY) < 12.f && std::fabs(x_ - kMarkX) < 8.f) return 2;
    return 1;
}

int Game::hullFrame() const {
    float u = std::fmod(heading_, kTau);
    if (u < 0) u += kTau;
    int i = int(std::lround(u / kTau * 16.f)) % 16;
    if (i < 0) i += 16;
    return i;
}

bool Game::sampleHull(float& east, const char*& why) const {
    const float c = std::cos(heading_), s = std::sin(heading_);
    east = -1e9f;
    why = "";
    bool hit = false;
    auto check = [&](float lx, float ly) {
        float wx = x_ + ly * c + lx * s;
        float wy = y_ + ly * s - lx * c;
        if (wx > east) east = wx;
        if (hit) return;
        if (wy >= kPlatY0 && wy <= kPlatY1 && wx >= kPlatEdge) {
            why = "scraped the platform";
            hit = true;
        } else if (wx >= kBankX) {
            why = "on the bank";
            hit = true;
        } else if (wy >= kShoalY) {
            why = "past the landing";
            hit = true;
        }
    };
    for (int i = 0; i <= 4; i++) {
        float t = i / 4.f;
        float lx = -kHalfW + (kHalfW * 2.f) * t;
        float ly = -kHalfL + (kHalfL * 2.f) * t;
        check(lx, kHalfL);
        check(lx, -kHalfL);
        check(kHalfW, ly);
        check(-kHalfW, ly);
    }
    return hit;
}

bool Game::inSlot(bool& yOk, bool& xOk, bool& hOk, bool& vOk) const {
    yOk = std::fabs(y_ - kMarkY) <= kLevelTol;
    xOk = std::fabs(x_ - kMarkX) <= kGapTol;
    hOk = std::fabs(wrap(heading_ - kPi * 0.5f)) <= kHdgTol;
    vOk = ground_ <= kStopSpd;
    return yOk && xOk && hOk && vOk;
}

void Game::begin() {
    x_ = 1.2f;
    y_ = 108.f;
    heading_ = 1.32f;
    speed_ = 0.f;
    throttle_ = 0.f;
    gvx_ = kTideX;
    gvy_ = kTideY;
    ground_ = std::hypot(gvx_, gvy_);
    race_ = 0.f;
    tide_ = kTideClock;
    hold_ = 0.f;
    wakeT_ = 0.f;
    wakeN_ = 0;
    won_ = false;
    over_ = false;
    wasSlot_ = false;
    why_ = "";
    chimeN_ = 0;
    chimeStep_ = 0;
    chimeT_ = 0.f;
    tone0_ = 0.f;
    tone1_ = 0.f;
    for (Wake& w : wakes_) w = {};
}

void Game::showTitle() {
    begin();
    mode_ = Mode::Title;
    zoom_ = kTitleZoom;
    camX_ = kTitleCamX;
    camY_ = kTitleCamY;
}

void Game::startRun() {
    begin();
    mode_ = Mode::Run;
    camX_ = x_;
    camY_ = y_;
    zoom_ = kPlayZoom;
    blip(680.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.78f);
    sys.apu.setEcho(0.12f, 0.22f, 0.14f);
    if (bot_) startRun();
    else showTitle();
}

void Game::controls(float& steer, float& throttle) {
    const gs::Pad& p = sys_->pad;
    steer = 0.f;
    if (p.down(gs::BTN_LEFT)) steer += 1.f;
    if (p.down(gs::BTN_RIGHT)) steer -= 1.f;
    if (std::fabs(p.axisX) > 0.18f) steer = std::clamp(-p.axisX, -1.f, 1.f);
    const bool up = p.down(gs::BTN_UP) || p.down(gs::BTN_C) || p.down(gs::BTN_A);
    const bool down = p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.down(gs::BTN_X);
    if (up) throttle_ = std::min(1.f, throttle_ + kDt * 0.75f);
    if (down) throttle_ = std::max(-1.f, throttle_ - kDt * 0.95f);
    if (p.accel > 0.05f) throttle_ = std::min(1.f, throttle_ + p.accel * kDt * 1.1f);
    if (p.brake > 0.05f) throttle_ = std::max(-1.f, throttle_ - p.brake * kDt * 1.1f);
    throttle = throttle_;
}

void Game::pilot(float& steer, float& throttle) {
    const float h0 = std::atan2(-kTideY, -kTideX);
    const float s0 = std::hypot(kTideX, kTideY);
    const float c0 = std::cos(h0);
    const float sn0 = std::sin(h0);

    float east = 0.f;
    const char* hit = "";
    sampleHull(east, hit);
    if (east > kPlatEdge - 0.55f || x_ > kBankX - kHalfW - 1.4f) {
        float err = wrap((h0 + 0.62f) - heading_);
        steer = std::clamp(err / 0.18f, -1.f, 1.f);
        throttle = speed_ > 0.3f ? -0.6f : -0.08f;
        return;
    }
    if (y_ + kHalfL > kShoalY - 4.f) {
        float err = wrap(h0 - heading_);
        steer = std::clamp(err / 0.2f, -1.f, 1.f);
        throttle = -0.75f;
        return;
    }

    const float slide = std::clamp((y_ - (kMarkY - 40.f)) / 40.f, 0.f, 1.f);
    const float laneX = (kMarkX - 6.5f) * (1.f - slide) + kMarkX * slide;
    const float xErr = laneX - x_;
    const float yErr = kMarkY - y_;

    if (yErr > 16.f) {
        float hdes = kPi * 0.5f - std::clamp(xErr * 0.06f, -0.5f, 0.5f);
        if (x_ > laneX + 1.6f) hdes = h0 + 0.34f;
        float err = wrap(hdes - heading_);
        steer = std::clamp(err / 0.26f, -1.f, 1.f);
        float want = yErr > 48.f ? 9.0f : 5.0f;
        if (std::fabs(err) > 0.6f) want = 2.2f;
        if (x_ > kMarkX + 0.4f) want = std::min(want, 3.f);
        throttle = std::clamp((want - speed_) * 0.24f, -0.85f, 0.92f);
        return;
    }

    float wantVx = std::clamp(xErr * 1.25f - gvx_ * 0.4f, -1.6f, 1.6f);
    float wantVy = std::clamp(yErr * 1.15f - gvy_ * 0.4f, -1.6f, 1.6f);
    const float a11 = c0;
    const float a12 = -sn0 * s0;
    const float a21 = sn0;
    const float a22 = c0 * s0;
    const float det = a11 * a22 - a12 * a21;
    float u = 0.f, a = 0.f;
    if (std::fabs(det) > 1e-4f) {
        u = (wantVx * a22 - a12 * wantVy) / det;
        a = (a11 * wantVy - wantVx * a21) / det;
    }
    a = std::clamp(a, -0.30f, 0.30f);
    u = std::clamp(u, -4.0f, 5.0f);
    float err = wrap((h0 + a) - heading_);
    steer = std::clamp(err / 0.22f, -1.f, 1.f);
    float face = std::cos(err);
    if (face < 0.f) face = 0.f;
    throttle = std::clamp((s0 + u) / kCapFwd, -1.f, 1.f);
    if (face < 0.85f) throttle *= face;
}

void Game::physics(float steer, float throttle) {
    float rate = 1.55f + std::min(std::fabs(speed_), 12.f) * 0.04f;
    if (std::fabs(speed_) < 1.2f) rate = 2.15f;
    heading_ = wrap(heading_ + steer * rate * kDt);
    float cap = throttle >= 0.f ? kCapFwd : kCapRev;
    float target = throttle * cap;
    speed_ += (target - speed_) * (1.f - std::exp(-3.2f * kDt));
    speed_ = std::clamp(speed_, -kCapRev, kCapFwd);

    float c = std::cos(heading_), s = std::sin(heading_);
    gvx_ = c * speed_ + kTideX;
    gvy_ = s * speed_ + kTideY;
    x_ += gvx_ * kDt;
    y_ += gvy_ * kDt;
    ground_ = std::hypot(gvx_, gvy_);

    if (y_ < kSouth) {
        y_ = kSouth;
        if (gvy_ < 0.f) speed_ *= 0.4f;
    }
    if (x_ < kWest) {
        x_ = kWest;
        if (gvx_ < 0.f) speed_ *= 0.4f;
    }

    float east = 0.f;
    const char* hit = "";
    if (sampleHull(east, hit)) {
        fail(hit);
        return;
    }

    bool yOk, xOk, hOk, vOk;
    if (inSlot(yOk, xOk, hOk, vOk)) hold_ += kDt;
    else hold_ = 0.f;
    if (hold_ >= kHoldNeed) win();

    wakeT_ -= kDt;
    if (wakeT_ <= 0.f && (std::fabs(speed_) > 2.2f || ground_ > 2.4f)) {
        wakeT_ = 0.08f;
        Wake w;
        w.x = x_ - c * 6.5f;
        w.y = y_ - s * 6.5f;
        w.life = 1.f;
        wakes_[wakeN_] = w;
        wakeN_ = (wakeN_ + 1) % 16;
    }
    for (Wake& w : wakes_)
        if (w.life > 0.f) w.life -= kDt * 0.7f;
}

void Game::win() {
    if (won_) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    why_ = "level";
    chime();
    sys_->rumble(0.35f, 0.18f, 180);
    sys_->setLight(60, 200, 90);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    why_ = why;
    sys_->apu.noiseBurst(0.4f, 90.f, 0.45f);
    sys_->apu.tone(0, 78.f, 0.07f);
    tone0_ = 0.45f;
    sys_->rumble(0.5f, 0.12f, 160);
    sys_->setLight(180, 30, 24);
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.05f);
    tone1_ = 0.09f;
}

void Game::chime() {
    chimeN_ = 4;
    chimeStep_ = 0;
    chimeT_ = 0.02f;
}

void Game::audio() {
    float water = mode_ == Mode::Run ? 0.016f + std::fabs(speed_) * 0.0005f : 0.01f;
    sys_->apu.noise(water, 500.f, false);
    if (mode_ == Mode::Run && (std::fabs(throttle_) > 0.04f || std::fabs(speed_) > 1.4f)) {
        float wob = 0.68f + 0.32f * std::sin(t_ * (13.f + std::fabs(throttle_) * 20.f));
        float vol = (0.012f + std::fabs(throttle_) * 0.028f) * wob;
        sys_->apu.tone(2, 46.f + std::fabs(throttle_) * 34.f + std::fabs(speed_) * 0.35f, vol);
    } else if (tone0_ <= 0.f) {
        sys_->apu.tone(2, 0.f, 0.f);
    }
    if (tone0_ > 0.f) {
        tone0_ -= kDt;
        if (tone0_ <= 0.f) sys_->apu.tone(0, 0.f, 0.f);
    }
    if (tone1_ > 0.f) {
        tone1_ -= kDt;
        if (tone1_ <= 0.f) sys_->apu.tone(1, 0.f, 0.f);
    }
    if (chimeN_ > 0) {
        chimeT_ -= kDt;
        if (chimeT_ <= 0.f) {
            static const float notes[] = {392.f, 523.25f, 659.25f, 784.f};
            sys_->apu.tone(0, notes[std::min(chimeStep_, 3)], 0.06f);
            tone0_ = 0.16f;
            chimeT_ = 0.16f;
            if (++chimeStep_ >= chimeN_) chimeN_ = 0;
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) startRun();
        else if (pad.pressed(gs::BTN_MODE)) sys.quit();
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(420.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        } else {
            race_ += kDt;
            tide_ -= kDt;
            float steer = 0.f, thr = throttle_;
            if (bot_) pilot(steer, thr);
            else controls(steer, thr);
            throttle_ = thr;
            physics(steer, thr);
            if (mode_ == Mode::Run && tide_ <= 0.f) fail("the tide left the platform");
            if (mode_ == Mode::Run) {
                bool yOk, xOk, hOk, vOk;
                bool slot = inSlot(yOk, xOk, hOk, vOk);
                if (slot && !wasSlot_) blip(880.f);
                wasSlot_ = slot;
                if (slot) sys.setLight(40, 170, 80);
                else if (std::fabs(y_ - kMarkY) < 14.f) sys.setLight(170, 130, 40);
                else sys.setLight(20, 50, 90);
            }
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Run;
        else if (pad.pressed(gs::BTN_MODE)) showTitle();
    } else if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
        startRun();
    } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
        showTitle();
    }
    camera();
    audio();
    draw();
}

void Game::camera() {
    if (mode_ == Mode::Title) {
        camX_ = kTitleCamX;
        camY_ = kTitleCamY;
        zoom_ = kTitleZoom;
        return;
    }
    float lead = mode_ == Mode::Run ? 7.f : 0.f;
    float gx = x_ + std::cos(heading_) * lead;
    float gy = y_ + std::sin(heading_) * lead;
    float k = 1.f - std::exp(-kDt * 4.4f);
    camX_ += (gx - camX_) * k;
    camY_ += (gy - camY_) * k;
    zoom_ += (kPlayZoom - zoom_) * k;
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

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    if (cx + w < -8 || cy + h < -8 || cx - w > gs::SCREEN_W + 8 || cy - h > gs::SCREEN_H + 8) return;
    gs::Sprite s;
    long sw = std::clamp(std::lround(w), 1L, 1800L);
    long sh = std::clamp(std::lround(h), 1L, 1800L);
    s.w = int16_t(sw);
    s.h = int16_t(sh);
    s.x = int16_t(std::clamp(std::lround(cx - sw * 0.5f), -2000L, 2000L));
    s.y = int16_t(std::clamp(std::lround(cy - sh * 0.5f), -2000L, 2000L));
    s.img = m.pick(float(sh));
    s.pal = uint8_t(pal);
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::place(const gs::Mipped& m, float wx, float wy, float worldH, int pal, float minPx) {
    float sx = 160.f + (wx - camX_) * zoom_;
    float sy = 112.f - (wy - camY_) * zoom_;
    float h = worldH * zoom_;
    if (h < minPx) h = minPx;
    spr(m, sx, sy, h, pal, false);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = true;
    v.hudEnabled = true;

    const float tideU = std::clamp(tide_ / kTideClock, 0.f, 1.f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (112.f - float(y)) / std::max(zoom_, 0.25f);
        float deep = std::clamp((wy - 10.f) / 220.f, 0.f, 1.f);
        uint16_t water = lerpC(gs::rgb4(3, 12, 13), gs::rgb4(1, 4, 7), deep);
        water = lerpC(water, gs::rgb4(6, 6, 4), (1.f - tideU) * 0.35f);
        float shimmer = 0.5f + 0.5f * std::sin(wy * 0.19f + t_ * 1.4f);
        if (shimmer > 0.93f) water = lerpC(water, gs::rgb4(12, 15, 14), 0.4f);
        v.lineBackdrop[y] = water;
        v.lineFog[y] = 0;

        float edge = wy >= kShoalY ? -80.f : kBankX;
        float edgeSx = 160.f + (edge - camX_) * zoom_;
        gs::RoadLine& r = v.road[y];
        r.on = edgeSx < gs::SCREEN_W + 4.f;
        r.hw = 150.f;
        r.cx = edgeSx + 1.14f * r.hw;
        r.v = wy * 28.f;
        r.pal = uint8_t(wy >= kShoalY ? PAL_SHOAL : PAL_GRASS);
        r.band = (int(std::floor(wy * 0.45f)) & 1) ? 1 : 0;
        r.style = 0;
        r.left = gs::GROUND_DROP;
        r.right = gs::GROUND_DROP;
        v.B.hscroll[y] = int16_t(std::sin(y * 0.04f + t_ * 0.7f) * 3.f + t_ * 6.f);
        v.B.vscroll[y] = int16_t(t_ * 2.5f);
    }

    auto banner = [&](const gs::Mipped& m, float x, float y, int pal) { spr(m, x, y, float(m.h), pal, false); };
    if (mode_ == Mode::Title) banner(art_.title, 160.f, 18.f, PAL_BANNER);
    else if (mode_ == Mode::Pause) banner(art_.paused, 160.f, 96.f, PAL_BANNER);
    else if (mode_ == Mode::Fail) {
        bool tide = why_ && why_[0] == 't';
        banner(tide ? art_.tideOut : art_.missed, 160.f, 86.f, PAL_ALERT);
    } else if (mode_ == Mode::Win) {
        banner(art_.level, 160.f, 72.f, PAL_WIN);
        banner(art_.withPlat, 160.f, 104.f, PAL_WIN);
    }

    float uLev = std::clamp((y_ - kMarkY) / kLevelTol, -1.25f, 1.25f);
    if (mode_ != Mode::Title) {
        spr(art_.track, 160.f, 34.f, 12.f, PAL_LAMP, false);
        int bub = std::fabs(uLev) <= 1.f ? PAL_WIN : PAL_ALERT;
        spr(art_.bubble, 160.f + uLev * 34.f, 34.f, 10.f, bub, false);
    }

    int flap = int(t_ * 3.2f) & 1;
    place(art_.gull[flap], 4.f + std::sin(t_ * 0.35f) * 16.f, 154.f + std::cos(t_ * 0.22f) * 8.f, 4.2f, PAL_GULL,
          mode_ == Mode::Title ? 8.f : 0.f);
    place(art_.gull[1 - flap], 20.f + std::cos(t_ * 0.28f) * 10.f, 96.f, 3.6f, PAL_GULL, 0.f);

    float bob = std::sin(t_ * 2.1f) * 1.3f;
    float bsx = 160.f + (x_ - camX_) * zoom_;
    float bsy = 112.f - (y_ - camY_) * zoom_ + bob;
    float boatH = std::max(kBoatH * zoom_, mode_ == Mode::Title ? 26.f : 0.f);
    const gs::Mipped& hull = art_.hull[hullFrame()];
    spr(hull, bsx, bsy, boatH, PAL_HULL, false);
    spr(hull, bsx + 3.f, bsy + 3.f, boatH, PAL_HULL, true);

    bool near = std::fabs(y_ - kMarkY) < kLevelTol * 1.4f && std::fabs(x_ - kMarkX) < kGapTol * 2.2f;
    int hand = (near || hold_ > 0.f || mode_ == Mode::Win) ? 1 : int(t_ * 1.5f) & 1;
    if (hold_ > 0.f || mode_ == Mode::Win) place(art_.rope, (x_ + kPlatEdge) * 0.5f, y_, 1.6f, PAL_PILE, 0.f);
    place(art_.pile, kPlatEdge + 1.1f, kPlatY0 + 2.f, 7.f, PAL_PILE, mode_ == Mode::Title ? 10.f : 0.f);
    place(art_.pile, kPlatEdge + 1.1f, kPlatY1 - 2.f, 7.f, PAL_PILE, mode_ == Mode::Title ? 10.f : 0.f);
    place(art_.post, kPlatEdge - 0.6f, kMarkY - kLevelTol, 10.f, PAL_LAMP, mode_ == Mode::Title ? 14.f : 0.f);
    place(art_.post, kPlatEdge - 0.6f, kMarkY + kLevelTol, 10.f, PAL_LAMP, mode_ == Mode::Title ? 14.f : 0.f);
    place(art_.pin, kPlatEdge - 0.7f, kMarkY, 2.6f, PAL_ALERT, mode_ == Mode::Title ? 7.f : 0.f);
    place(art_.hand[hand], kDeckCx - 3.f, kMarkY + 1.2f, 8.f, PAL_FOLK, mode_ == Mode::Title ? 14.f : 0.f);
    place(art_.coil, kDeckCx + 1.5f, kMarkY - 3.f, 3.4f, PAL_PILE, 0.f);
    place(art_.shed, 44.f, 178.f, 14.f, PAL_WOOD, mode_ == Mode::Title ? 16.f : 0.f);
    place(art_.lamp, 40.f, 158.f, 12.f, PAL_LAMP, mode_ == Mode::Title ? 12.f : 0.f);
    place(art_.staff, 38.f, 142.f, 11.f, PAL_LAMP, mode_ == Mode::Title ? 11.f : 0.f);
    place(art_.pip, 36.6f, 138.f + tideU * 7.f, 2.f, PAL_ALERT, 0.f);
    place(art_.flag, 46.f, 190.f, 5.5f, PAL_ALERT, mode_ == Mode::Title ? 8.f : 0.f);
    place(art_.deck, kDeckCx, kMarkY, kDeckL, PAL_WOOD, mode_ == Mode::Title ? 70.f : 0.f);
    place(art_.buoy, -8.f, 78.f, 7.f, PAL_BUOY, mode_ == Mode::Title ? 8.f : 0.f);
    place(art_.buoy, -5.f, 118.f, 7.f, PAL_BUOY, 0.f);
    const float reeds[][2] = {{31.f, 118.f}, {31.4f, 128.f}, {31.2f, 204.f}, {31.6f, 214.f}, {30.8f, 108.f}};
    for (const float* r : reeds) place(art_.reed, r[0], r[1], 5.f, PAL_GRASS, 0.f);

    for (const Wake& w : wakes_) {
        if (w.life <= 0.f) continue;
        float h = (2.2f + (1.f - w.life) * 3.5f) * (zoom_ / kPlayZoom);
        float sx = 160.f + (w.x - camX_) * zoom_;
        float sy = 112.f - (w.y - camY_) * zoom_;
        spr(art_.foam, sx, sy, std::max(2.f, h), PAL_FOAM, false);
    }

    if (ground_ > 3.f) {
        float c = std::cos(heading_), s = std::sin(heading_);
        place(art_.foam, x_ + c * 8.f, y_ + s * 8.f, 2.4f, PAL_FOAM, 0.f);
    }

    if (mode_ != Mode::Title) {
        float psx = 160.f + (kMarkX - camX_) * zoom_;
        float psy = 112.f - (kMarkY - camY_) * zoom_;
        if (psx < 18.f || psx > 302.f || psy < 28.f || psy > 196.f) {
            float dx = psx - 160.f, dy = psy - 112.f;
            float k = 1.f;
            if (std::fabs(dx) > 1.f) k = std::min(k, 128.f / std::fabs(dx));
            if (std::fabs(dy) > 1.f) k = std::min(k, 78.f / std::fabs(dy));
            spr(art_.pin, 160.f + dx * k, 112.f + dy * k, 11.f, PAL_ALERT, false);
        }
    }

    char buf[64];
    int left = std::max(0, int(std::ceil(tide_ - 1e-3f)));
    if (mode_ == Mode::Title) {
        hudC(23, "STOP LEVEL WITH THE PLATFORM", PAL_HUD);
        hudC(24, "STRIPE BETWEEN THE POSTS", PAL_BANNER);
        hudC(25, "HOLD. THE TIDE SETS SOUTH", PAL_TAG);
        if ((int(t_ * 2.f) & 1) == 0) hudC(27, "START", PAL_WIN);
        else hudC(27, "ARROWS STEER   UP GO   DOWN BACK", PAL_HUD);
        return;
    }

    hud(1, 0, "S3 SKIFF PLAT", PAL_BANNER);
    std::snprintf(buf, sizeof buf, "TIDE %d", left);
    hud(32, 0, buf, left <= 10 ? PAL_ALERT : PAL_TAG);

    if (mode_ == Mode::Pause) {
        hudC(18, "START CONTINUES", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Win) {
        std::snprintf(buf, sizeof buf, "HELD LEVEL  %.1fS", race_);
        hudC(16, buf, PAL_HUD);
        if (!bot_) hudC(18, "START RUNS IT AGAIN", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Fail) {
        hudC(16, why_, PAL_ALERT);
        if (!bot_) hudC(18, "START TRIES AGAIN", PAL_HUD);
        return;
    }

    bool yOk, xOk, hOk, vOk;
    inSlot(yOk, xOk, hOk, vOk);
    const char* line = "COME UP TO THE PLATFORM";
    int pal = PAL_HUD;
    if (yOk && xOk && hOk && vOk) {
        int n = std::clamp(int(hold_ / kHoldNeed * 5.f) + 1, 1, 5);
        std::snprintf(buf, sizeof buf, "HOLD %d/5", n);
        line = buf;
        pal = PAL_WIN;
    } else if (yOk && xOk && !vOk) {
        line = "LEVEL - EASE OFF";
        pal = PAL_BANNER;
    } else if (yOk && xOk && !hOk) {
        line = "SQUARE THE BOW";
        pal = PAL_BANNER;
    } else if (std::fabs(x_ - kMarkX) < 4.f && y_ < kMarkY - kLevelTol) {
        line = "SHORT OF THE POSTS";
        pal = PAL_TAG;
    } else if (std::fabs(x_ - kMarkX) < 4.f && y_ > kMarkY + kLevelTol) {
        line = "PAST THE POSTS";
        pal = PAL_ALERT;
    } else if (x_ > kMarkX + kGapTol) {
        line = "TOO CLOSE TO THE TIMBER";
        pal = PAL_ALERT;
    } else if (y_ > kPlatY0 - 8.f) {
        line = "TOO WIDE OF THE PLATFORM";
        pal = PAL_TAG;
    }
    hud(1, 1, line, pal);
    std::snprintf(buf, sizeof buf, "OFF %+.1f  IN %+.1f  SPD %.1f", y_ - kMarkY, x_ - kMarkX, ground_);
    hud(1, 2, buf, PAL_HUD);
    if (race_ < 4.f) hud(1, 26, "TIDE SETS YOU SOUTH", PAL_TAG);
    else hud(1, 26, "LINE THE STRIPE UP, THEN HOLD", PAL_HUD);
    hud(1, 27, "ARROWS STEER   UP GO   DOWN BACK", PAL_HUD);
}

}  // namespace skiffplat

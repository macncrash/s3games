#include "mark.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace sledmark {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;
constexpr float kNorth = kPi * 0.5f;

constexpr float kStartX = 0.f;
constexpr float kStartY = 30.f;
constexpr float kStartH = kNorth;

constexpr float kMarkX = 0.f;
constexpr float kMarkY = 198.f;
constexpr float kSetR = 8.4f;
constexpr float kPaintR = 16.5f;
constexpr float kMissPast = 8.f;

constexpr float kCrewTime = 36.f;
constexpr float kStop = 0.50f;
constexpr float kHold = 0.40f;
constexpr float kSledH = 20.f;

constexpr float kAccelSnow = 8.4f;
constexpr float kAccelIce = 6.2f;
constexpr float kBrakeSnow = 7.8f;
constexpr float kBrakeIce = 3.6f;
constexpr float kCoastSnow = 0.85f;
constexpr float kCoastIce = 0.22f;
constexpr float kMaxSnow = 13.5f;
constexpr float kMaxIce = 16.2f;

constexpr float kPlayZoom = 2.2f;
constexpr float kTitleZoom = 1.28f;
constexpr float kTitleCamX = 2.f;
constexpr float kTitleCamY = 112.f;

constexpr float kRivalY0 = 42.f;
constexpr float kRivalSide = 14.f;

float wrap(float a) {
    while (a > kPi) a -= kTau;
    while (a < -kPi) a += kTau;
    return a;
}

float smooth(float u) {
    u = std::clamp(u, 0.f, 1.f);
    return u * u * (3.f - 2.f * u);
}

float lerp(float a, float b, float u) { return a + (b - a) * u; }

// Packed trail. Positive is a right-hand bend, then back onto the mark.
float fairX(float y) {
    if (y < 34.f) return 0.f;
    if (y < 76.f) return 24.f * smooth((y - 34.f) / 42.f);
    if (y < 118.f) return 24.f - 42.f * smooth((y - 76.f) / 42.f);
    if (y < 158.f) return -18.f + 18.f * smooth((y - 118.f) / 40.f);
    return 0.f;
}

// Half-width of the packed snow. Outside it is drift.
float halfW(float y) {
    if (y < 16.f) return 12.f;
    if (y < 38.f) return lerp(12.f, 26.f, smooth((y - 16.f) / 22.f));
    if (y < 164.f) return 26.f;
    if (y < 180.f) return lerp(26.f, 40.f, smooth((y - 164.f) / 16.f));
    if (y < 218.f) return 40.f;
    if (y < 236.f) return lerp(40.f, 9.f, smooth((y - 218.f) / 18.f));
    return 9.f;
}

bool iceAt(float y) { return (y > 48.f && y < 90.f) || (y > 104.f && y < 142.f); }

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    return gs::rgb4(int(ar + (br - ar) * t), int(ag + (bg - ag) * t), int(ab + (bb - ab) * t));
}

}  // namespace

int Game::frameOf(float heading) const {
    float u = std::fmod(heading, kTau);
    if (u < 0.f) u += kTau;
    int i = int(std::lround(u / kTau * 16.f)) % 16;
    if (i < 0) i += 16;
    return i;
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (settle_ > 0.05f) return 3;
    float dx = x_ - kMarkX, dy = y_ - kMarkY;
    if (dx * dx + dy * dy <= kPaintR * kPaintR) return 2;
    return 1;
}

const char* Game::hint() const {
    float dx = x_ - kMarkX, dy = y_ - kMarkY;
    float dist = std::hypot(dx, dy);
    if (settle_ > 0.05f) return "HOLD. LET THE RUNNERS SET";
    if (onMark_ && speed_ > kStop) return "HOOK. SET THE SLED DOWN";
    if (dist < kPaintR + 4.f) return "INSIDE THE CROSS. COME TO REST";
    if (crew_ < 10.f) return "THE OTHER CREW IS CLOSING";
    if (y_ > 150.f) return "THE MARK IS THE CROSSED CENTRE";
    if (iceAt(y_)) return "ICE. THE HOOK BARELY BITES";
    return "HAW AND GEE. STAY ON THE SNOW";
}

float Game::shakeX() const { return std::sin(t_ * 47.f) * shake_; }
float Game::shakeY() const { return std::cos(t_ * 39.f) * shake_; }

void Game::poseRival() {
    float u = 1.f - std::clamp(crew_ / kCrewTime, 0.f, 1.f);
    float y = lerp(kRivalY0, kMarkY, u);
    float side = kRivalSide * (1.f - u);
    rivalY_ = y;
    rivalX_ = fairX(y) + side;
    float u2 = std::min(1.f, u + 0.035f);
    float ny = lerp(kRivalY0, kMarkY, u2);
    float nx = fairX(ny) + kRivalSide * (1.f - u2);
    rivalH_ = std::atan2(ny - rivalY_, nx - rivalX_);
}

void Game::begin() {
    x_ = kStartX;
    y_ = kStartY;
    heading_ = kStartH;
    speed_ = 0.f;
    slip_ = 0.f;
    throttle_ = 0.f;
    raceTime_ = 0.f;
    crew_ = kCrewTime;
    settle_ = 0.f;
    sprayT_ = 0.f;
    stuckT_ = 0.f;
    stuckX_ = x_;
    stuckY_ = y_;
    powderT_ = 0.f;
    shake_ = 0.f;
    sprayCursor_ = 0;
    onMark_ = false;
    won_ = false;
    over_ = false;
    chimeN_ = 0;
    lastCrewSec_ = int(std::ceil(kCrewTime));
    why_[0] = 0;
    report_[0] = 0;
    std::snprintf(why_, sizeof why_, "running");
    for (Spray& s : spray_) s = {};
    poseRival();
}

void Game::showTitle() {
    begin();
    y_ = 72.f;
    x_ = fairX(y_);
    float ny = y_ + 12.f;
    heading_ = std::atan2(ny - y_, fairX(ny) - x_);
    crew_ = kCrewTime * 0.58f;
    poseRival();
    mode_ = Mode::Title;
    zoom_ = kTitleZoom;
    camX_ = kTitleCamX;
    camY_ = kTitleCamY;
}

void Game::startRun() {
    begin();
    mode_ = Mode::Run;
    zoom_ = kPlayZoom;
    camX_ = x_;
    camY_ = y_ + 14.f;
    blip(520.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.7f);
    sys.apu.setEcho(0.10f, 0.16f, 0.05f);
    for (int i = 0; i < 16; i++) {
        Flake& f = flake_[i];
        f.x = float((i * 53) % 320);
        f.y = float((i * 37) % 224);
        f.v = 16.f + float(i % 11);
        f.sc = 2.f + float(i % 3);
        f.ph = float(i) * 0.7f;
    }
    if (bot_) startRun();
    else showTitle();
}

void Game::human(float& steer, float& throttle) {
    const gs::Pad& p = sys_->pad;
    steer = 0.f;
    if (p.down(gs::BTN_LEFT)) steer += 1.f;
    if (p.down(gs::BTN_RIGHT)) steer -= 1.f;
    if (std::fabs(p.axisX) > 0.18f) steer = std::clamp(-p.axisX, -1.f, 1.f);
    const bool go = p.down(gs::BTN_UP) || p.down(gs::BTN_A) || p.down(gs::BTN_C) || p.axisY > 0.25f || p.accel > 0.15f;
    const bool hook = p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.down(gs::BTN_X) || p.axisY < -0.25f || p.brake > 0.15f;
    if (hook) throttle_ = std::max(-1.f, throttle_ - kDt * 3.4f);
    else if (go) throttle_ = std::min(1.f, throttle_ + kDt * 2.2f);
    else if (throttle_ > 0.f) throttle_ = std::max(0.f, throttle_ - kDt * 1.6f);
    else throttle_ = std::min(0.f, throttle_ + kDt * 2.6f);
    throttle = throttle_;
}

void Game::pilot(float& steer, float& throttle) {
    float dx = kMarkX - x_, dy = kMarkY - y_;
    float dist = std::hypot(dx, dy);
    bool ice = iceAt(y_);
    float brakeA = ice ? kBrakeIce : kBrakeSnow;

    float aimX, aimY;
    if (dist < 30.f || y_ > kMarkY - 6.f) {
        aimX = kMarkX;
        aimY = kMarkY;
    } else {
        float look = 16.f + speed_ * 0.55f;
        float ty = std::min(y_ + look, kMarkY);
        aimX = fairX(ty);
        aimY = ty;
    }
    float want = (dist < 1.6f) ? heading_ : std::atan2(aimY - y_, aimX - x_);
    float err = wrap(want - heading_);
    steer = std::clamp(err / 0.42f, -1.f, 1.f);
    float off = x_ - fairX(std::min(y_, kMarkY));
    if (dist > 26.f && std::sin(heading_) > 0.35f)
        steer = std::clamp(steer + std::clamp(off * 0.05f, -0.45f, 0.45f), -1.f, 1.f);
    if (dist < kSetR + 2.f) steer *= 0.25f;

    if (dist <= kSetR) {
        throttle = speed_ > 0.32f ? -1.f : 0.f;
        return;
    }

    float desired = dist > 52.f ? 12.6f : std::clamp((dist - kSetR) * 0.55f, 1.35f, 12.6f);
    float curve = fairX(y_ + 12.f) - fairX(y_);
    if (std::fabs(curve) > 7.f) desired = std::min(desired, 8.2f);
    if (std::fabs(off) > halfW(y_) * 0.62f) desired = std::min(desired, 6.f);
    if (std::fabs(err) > 0.85f) desired = std::min(desired, 3.2f);
    if (ice && dist > 46.f) desired = std::min(desired, 10.5f);

    float room = dist - kSetR * 0.55f;
    float stopDist = speed_ * speed_ / (2.f * brakeA);
    if (dy > -1.5f && std::fabs(err) < 0.6f && room < stopDist * 0.9f) throttle = -1.f;
    else if (speed_ > desired + 0.3f) throttle = -1.f;
    else if (speed_ < desired - 0.45f) throttle = std::clamp((desired - speed_) / 3.2f, 0.28f, 1.f);
    else throttle = ice ? 0.08f : 0.12f;
}

void Game::succeed() {
    if (won_) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    onMark_ = true;
    speed_ = 0.f;
    slip_ = 0.f;
    throttle_ = 0.f;
    std::snprintf(why_, sizeof why_, "set down");
    std::snprintf(report_, sizeof report_,
                  "S3 SLED MARK  SET  the sled is down on the mark ahead of the other crew  (%.1f s, %.1f s left)",
                  raceTime_, std::max(0.f, crew_));
    chime(4);
    sys_->rumble(0.28f, 0.55f, 160);
    sys_->setLight(40, 170, 90);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    std::snprintf(why_, sizeof why_, "%s", why);
    std::snprintf(report_, sizeof report_,
                  "S3 SLED MARK  FAIL  %s  x %.1f  y %.1f  hdg %.0f  spd %.2f  (%.1f s)", why, x_, y_,
                  heading_ * 57.2958f, speed_, raceTime_);
    speed_ = 0.f;
    slip_ = 0.f;
    throttle_ = 0.f;
    burst_ = 0.45f;
    sys_->apu.noiseBurst(0.42f, 90.f, 0.4f);
    sys_->apu.tone(0, 70.f, 0.06f);
    tone0_ = 0.42f;
    sys_->rumble(0.6f, 0.16f, 180);
    sys_->setLight(170, 30, 24);
}

void Game::physics(float dt, float steer, float throttle) {
    raceTime_ += dt;
    crew_ -= dt;

    bool ice = iceAt(y_);
    float turn = (ice ? 1.25f : 2.05f) / (1.f + speed_ * 0.055f);
    heading_ = wrap(heading_ + steer * turn * dt);
    slip_ += steer * speed_ * (ice ? 0.045f : 0.028f) * dt;
    slip_ *= std::exp(-(ice ? 1.5f : 3.4f) * dt);
    slip_ = std::clamp(slip_, -0.42f, 0.42f);

    float accel = ice ? kAccelIce : kAccelSnow;
    float brakeA = ice ? kBrakeIce : kBrakeSnow;
    float coast = ice ? kCoastIce : kCoastSnow;
    float cap = ice ? kMaxIce : kMaxSnow;
    if (throttle > 0.f) speed_ += throttle * accel * dt;
    if (throttle < 0.f) speed_ -= (-throttle) * brakeA * dt;
    else speed_ -= coast * dt;
    if (speed_ < 0.f) speed_ = 0.f;
    if (speed_ > cap) speed_ = cap;

    float dir = wrap(heading_ - slip_);
    x_ += std::cos(dir) * speed_ * dt;
    y_ += std::sin(dir) * speed_ * dt;

    if (y_ < 8.f) {
        y_ = 8.f;
        if (std::sin(heading_) < 0.f) speed_ *= 0.45f;
    }

    float dx = x_ - kMarkX, dy = y_ - kMarkY;
    float dist = std::hypot(dx, dy);
    onMark_ = dist <= kSetR;

    float lim = std::max(6.f, halfW(y_) - 1.4f);
    float off = x_ - fairX(y_);
    if (std::fabs(off) > lim) {
        x_ -= std::copysign(16.f * dt, off);
        speed_ *= std::exp(-2.4f * dt);
        powderT_ += dt;
        shake_ = std::max(shake_, 2.4f);
        if (thumpT_ <= 0.f) {
            sys_->apu.noiseBurst(0.22f, 240.f, 0.12f);
            thumpT_ = 0.32f;
            burst_ = std::max(burst_, 0.12f);
        }
        if (powderT_ > 3.3f) {
            fail("buried in the drift");
            return;
        }
    } else {
        powderT_ = std::max(0.f, powderT_ - dt * 2.f);
    }

    if (onMark_ && throttle <= 0.05f) {
        speed_ -= 6.5f * dt;
        if (speed_ < 0.f) speed_ = 0.f;
    }
    if (throttle < -0.65f && speed_ > 7.f) shake_ = std::max(shake_, 1.15f);

    bool ready = onMark_ && speed_ <= kStop;
    if (ready) {
        settle_ += dt;
        if (settle_ >= kHold) {
            succeed();
            return;
        }
    } else if (!onMark_ || speed_ > kStop + 0.18f) {
        settle_ = 0.f;
    }

    if (crew_ <= 0.f) {
        crew_ = 0.f;
        poseRival();
        rivalX_ = kMarkX + 3.4f;
        rivalY_ = kMarkY - 1.6f;
        rivalH_ = kNorth;
        fail("the other crew took the mark");
        return;
    }
    if (y_ > kMarkY + kSetR + kMissPast && dist > kSetR + 1.f) {
        fail("missed the mark");
        return;
    }

    poseRival();

    sprayT_ -= dt;
    bool spraying = speed_ > 5.5f || (throttle < -0.45f && speed_ > 2.f);
    if (sprayT_ <= 0.f && spraying) {
        sprayT_ = throttle < -0.4f ? 0.04f : 0.07f;
        Spray s;
        float side = (sprayCursor_ & 1) ? 1.6f : -1.6f;
        s.x = x_ - std::cos(dir) * 7.5f + std::sin(heading_) * side;
        s.y = y_ - std::sin(dir) * 7.5f - std::cos(heading_) * side;
        s.life = 1.f;
        spray_[sprayCursor_] = s;
        sprayCursor_ = (sprayCursor_ + 1) % 14;
    }
    for (Spray& s : spray_)
        if (s.life > 0.f) s.life -= dt * 0.9f;

    if (bot_ && !onMark_) {
        stuckT_ += dt;
        if (stuckT_ > 1.6f) {
            float moved = std::hypot(x_ - stuckX_, y_ - stuckY_);
            stuckX_ = x_;
            stuckY_ = y_;
            stuckT_ = 0.f;
            if (moved < 2.2f && dist > 16.f) {
                heading_ = std::atan2(kMarkY - y_, kMarkX - x_);
                slip_ = 0.f;
                speed_ = std::max(speed_, 4.5f);
            }
        }
    }
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.05f);
    tone1_ = 0.09f;
}

void Game::chime(int notes) {
    chimeN_ = std::clamp(notes, 1, 5);
    chimeStep_ = 0;
    chimeT_ = 0.02f;
}

void Game::audio(float dt) {
    if (burst_ > 0.f) burst_ -= dt;
    else {
        float wind = mode_ == Mode::Run ? 0.007f + speed_ * 0.0015f : 0.006f;
        bool ice = iceAt(y_);
        sys_->apu.noise(wind, ice && speed_ > 2.f ? 1500.f : 640.f, false);
    }
    if (mode_ == Mode::Run && speed_ > 0.7f && tone0_ <= 0.f) {
        bool ice = iceAt(y_);
        float freq = (ice ? 230.f : 120.f) + speed_ * 7.f;
        float vol = 0.01f + speed_ * 0.0016f;
        if (throttle_ < -0.45f) vol += 0.018f;
        sys_->apu.tone(2, freq, vol);
    } else if (tone0_ <= 0.f) {
        sys_->apu.tone(2, 0.f, 0.f);
    }
    if (mode_ == Mode::Run && crew_ < 8.f && crew_ > 0.f) {
        int sec = int(std::ceil(crew_ - 1e-4f));
        if (sec != lastCrewSec_) {
            lastCrewSec_ = sec;
            blip(sec <= 3 ? 900.f : 460.f);
        }
    } else if (crew_ >= 8.f) {
        lastCrewSec_ = int(std::ceil(crew_));
    }
    if (tone0_ > 0.f) {
        tone0_ -= dt;
        if (tone0_ <= 0.f) sys_->apu.tone(0, 0.f, 0.f);
    }
    if (tone1_ > 0.f) {
        tone1_ -= dt;
        if (tone1_ <= 0.f) sys_->apu.tone(1, 0.f, 0.f);
    }
    if (thumpT_ > 0.f) thumpT_ -= dt;
    if (chimeN_ > 0) {
        chimeT_ -= dt;
        if (chimeT_ <= 0.f) {
            static const float notes[] = {392.f, 494.f, 587.f, 784.f};
            sys_->apu.tone(0, notes[std::min(chimeStep_, 3)], 0.055f);
            tone0_ = 0.13f;
            chimeT_ = 0.14f;
            if (++chimeStep_ >= chimeN_) chimeN_ = 0;
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C)) startRun();
        else if (pad.pressed(gs::BTN_MODE)) sys.quit();
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(320.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        } else {
            float steer = 0.f, thr = 0.f;
            if (bot_) pilot(steer, thr);
            else human(steer, thr);
            throttle_ = thr;
            physics(kDt, steer, throttle_);
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Run;
        else if (pad.pressed(gs::BTN_MODE)) showTitle();
    } else if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C))) {
        startRun();
    } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
        showTitle();
    }
    for (Flake& f : flake_) {
        f.y += f.v * kDt;
        f.x += std::sin(t_ * 0.6f + f.ph) * 8.f * kDt;
        if (f.y > 230.f) f.y = -8.f;
        if (f.x < -6.f) f.x += 326.f;
        if (f.x > 326.f) f.x -= 326.f;
    }
    camera();
    audio(kDt);
    draw();
    shake_ *= std::exp(-7.f * kDt);
}

void Game::camera() {
    if (mode_ == Mode::Title) {
        camX_ = kTitleCamX;
        camY_ = kTitleCamY;
        zoom_ = kTitleZoom;
        return;
    }
    float lead = mode_ == Mode::Run ? 16.f : 6.f;
    float gx = x_ + std::cos(heading_) * lead;
    float gy = y_ + std::sin(heading_) * lead;
    float k = 1.f - std::exp(-kDt * 4.2f);
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

void Game::place(const gs::Mipped& m, float wx, float wy, float worldH, int pal, bool shadow) {
    float sx = 160.f + (wx - camX_) * zoom_ + shakeX();
    float sy = 112.f - (wy - camY_) * zoom_ + shakeY();
    spr(m, sx, sy, worldH * zoom_, pal, shadow);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.roadTime = 0;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (112.f - y) / std::max(zoom_, 0.25f);
        float u = std::clamp((wy + 20.f) / 250.f, 0.f, 1.f);
        v.lineBackdrop[y] = lerpC(gs::rgb4(7, 9, 13), gs::rgb4(3, 5, 9), u);
        v.lineFog[y] = 0;
        bool ice = iceAt(wy);
        gs::RoadLine& r = v.road[y];
        r.on = true;
        r.cx = 160.f + (fairX(wy) - camX_) * zoom_ + shakeX();
        r.hw = std::max(3.f, halfW(wy) * zoom_);
        r.v = wy * 18.f;
        r.pal = uint8_t(ice ? PAL_ICE : PAL_SNOW);
        r.band = (int(std::floor(wy * 0.12f)) & 1) ? 1 : 0;
        r.style = ice ? gs::ROAD_ICE : gs::ROAD_SNOW;
        r.left = gs::GROUND_SNOWWALL;
        r.right = gs::GROUND_SNOWWALL;
    }

    auto banner = [&](const gs::Mipped& m, float x, float y, int pal) { spr(m, x, y, float(m.h), pal, false); };
    if (mode_ == Mode::Title) banner(art_.title, 160.f, 16.f, PAL_BANNER);
    else if (mode_ == Mode::Pause) banner(art_.paused, 160.f, 28.f, PAL_BANNER);
    else if (mode_ == Mode::Fail) {
        bool crew = std::strcmp(why_, "the other crew took the mark") == 0;
        bool buried = std::strcmp(why_, "buried in the drift") == 0;
        banner(crew ? art_.crewTook : buried ? art_.buried : art_.missed, 160.f, 28.f, PAL_ALERT);
    } else if (mode_ == Mode::Win) {
        banner(art_.setDown, 160.f, 22.f, PAL_WIN);
        banner(art_.onMark, 160.f, 48.f, PAL_WIN);
    }

    if (mode_ != Mode::Title) {
        for (const Flake& f : flake_) spr(art_.puff, f.x, f.y, f.sc, PAL_FX, false);
    } else {
        for (int i = 0; i < 8; i++) {
            const Flake& f = flake_[i];
            spr(art_.puff, f.x, f.y, f.sc, PAL_FX, false);
        }
    }

    if (mode_ == Mode::Run || mode_ == Mode::Pause) {
        auto chart = [&](float wx, float wy, int pal, float h) {
            float sx = 286.f + wx * 0.42f;
            float sy = 172.f - wy * 0.26f;
            spr(art_.dot, sx, sy, h, pal, false);
        };
        for (int i = 0; i < 8; i++) {
            float y = 24.f + i * 24.f;
            chart(fairX(y), y, PAL_WIN, 3.f);
        }
        chart(kMarkX, kMarkY, PAL_MARK, 6.f);
        chart(rivalX_, rivalY_, PAL_RIVAL, 4.f);
        chart(x_, y_, PAL_ALERT, 5.f);
        spr(art_.panel, 286.f, 148.f, 76.f, PAL_MAP, false);

        float psx = 160.f + (kMarkX - camX_) * zoom_ + shakeX();
        float psy = 112.f - (kMarkY - camY_) * zoom_ + shakeY();
        if (psx < 16.f || psx > 304.f || psy < 16.f || psy > 208.f) {
            float dx = psx - 160.f, dy = psy - 112.f;
            float k = 1.f;
            if (std::fabs(dx) > 1.f) k = std::min(k, 132.f / std::fabs(dx));
            if (std::fabs(dy) > 1.f) k = std::min(k, 84.f / std::fabs(dy));
            spr(art_.pin, 160.f + dx * k, 112.f + dy * k, 12.f, PAL_MARK, false);
        }
    }

    float bob = std::sin(t_ * 3.1f) * (speed_ > 1.f ? 0.6f : 0.15f);
    float bsx = 160.f + (x_ - camX_) * zoom_ + shakeX();
    float bsy = 112.f - (y_ - camY_) * zoom_ + shakeY() + bob;
    float sledH = kSledH * zoom_;
    if (mode_ == Mode::Title) sledH = std::max(sledH, 22.f);
    const gs::Mipped& team = art_.sled[frameOf(heading_)];
    spr(team, bsx + 3.f, bsy + 4.f, sledH, PAL_TEAM, true);
    spr(team, bsx, bsy, sledH, PAL_TEAM, false);

    for (const Spray& s : spray_) {
        if (s.life <= 0.f) continue;
        float h = (2.4f + (1.f - s.life) * 4.f) * (zoom_ / kPlayZoom);
        float sx = 160.f + (s.x - camX_) * zoom_ + shakeX();
        float sy = 112.f - (s.y - camY_) * zoom_ + shakeY();
        spr(art_.puff, sx, sy, std::max(2.f, h), PAL_FX, false);
    }

    float rsx = 160.f + (rivalX_ - camX_) * zoom_ + shakeX();
    float rsy = 112.f - (rivalY_ - camY_) * zoom_ + shakeY();
    float rivalH = std::max(12.f, kSledH * 0.9f * zoom_);
    const gs::Mipped& other = art_.sled[frameOf(rivalH_)];
    spr(other, rsx + 2.f, rsy + 3.f, rivalH, PAL_RIVAL, true);
    spr(other, rsx, rsy, rivalH, PAL_RIVAL, false);

    for (int i = 0; i < 12; i++) {
        float y = 28.f + i * 16.f;
        float x = fairX(y);
        float w = halfW(y);
        float sway = (i & 1) ? 2.f : -1.f;
        place(art_.pine, x - w - 8.f + sway, y, 16.f + (i % 3), PAL_PINE);
        place(art_.pine, x + w + 9.f, y + 7.f, 15.f + ((i + 1) % 3), PAL_PINE);
        if ((i % 3) == 0) place(art_.rock, x - w - 3.f, y + 4.f, 7.f, PAL_ROCK);
        if ((i % 4) == 1) place(art_.stake, x + w - 1.5f, y, 11.f, PAL_WOOD);
    }
    place(art_.cabin, -30.f, 18.f, 16.f, PAL_WOOD);
    place(art_.cabin, 36.f, 214.f, 18.f, PAL_WOOD);
    place(art_.stake, -10.f, 22.f, 12.f, PAL_WOOD);
    place(art_.stake, 10.f, 22.f, 12.f, PAL_WOOD);

    for (int i = 0; i < 4; i++) {
        float y = 58.f + i * 22.f;
        if (i >= 2) y = 112.f + (i - 2) * 16.f;
        place(art_.crack, fairX(y) + ((i & 1) ? 4.f : -5.f), y, 8.f, PAL_FX);
    }

    for (int i = 0; i < 8; i++) {
        float a = i * kTau / 8.f;
        place(art_.stake, kMarkX + std::cos(a) * 18.f, kMarkY + std::sin(a) * 18.f, 13.f, PAL_WOOD);
    }
    place(art_.stake, kMarkX, kMarkY, 15.f, PAL_MARK);
    place(art_.disc, kMarkX, kMarkY, 36.f, PAL_MARK);

    char buf[56];
    if (mode_ == Mode::Title) {
        hudC(21, "TAKE THE SLED", PAL_BANNER);
        hudC(22, "SET IT DOWN ON THE MARK", PAL_WIN);
        hudC(23, "THE CLOCK IS THE OTHER CREW", PAL_ALERT);
        if ((int(t_ * 2.f) & 1) == 0) hudC(26, "START", PAL_WIN);
        else hudC(26, "UP MUSH   DOWN HOOK   ARROWS STEER", PAL_HUD);
        return;
    }
    hud(1, 0, "S3 SLED MARK", PAL_BANNER);
    int left = std::max(0, int(std::ceil(crew_ - 1e-3f)));
    std::snprintf(buf, sizeof buf, "CREW %d:%02d", left / 60, left % 60);
    hud(29, 0, buf, crew_ < 8.f ? PAL_ALERT : PAL_BANNER);
    if (mode_ == Mode::Pause) {
        hudC(24, "START CONTINUES", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Win) {
        int sec = int(raceTime_);
        std::snprintf(buf, sizeof buf, "SET IN %d:%02d", sec / 60, sec % 60);
        hudC(22, buf, PAL_HUD);
        int spare = std::max(0, int(std::ceil(crew_ - 1e-3f)));
        std::snprintf(buf, sizeof buf, "CREW HAD %d:%02d", spare / 60, spare % 60);
        hudC(23, buf, PAL_WIN);
        if (!bot_) hudC(25, "START RUNS THE LEG AGAIN", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Fail) {
        hudC(22, why_, PAL_ALERT);
        if (!bot_) hudC(24, "START TRIES THE LEG AGAIN", PAL_HUD);
        return;
    }
    hud(1, 1, hint(), onMark_ ? PAL_WIN : PAL_BANNER);
    int sp = int(std::lround(speed_));
    const char* surf = onMark_ ? "ON MARK" : iceAt(y_) ? "ICE" : "SNOW";
    std::snprintf(buf, sizeof buf, "SPD %02d  %s", sp, surf);
    hud(1, 2, buf, onMark_ ? PAL_WIN : PAL_HUD);
    if (onMark_) {
        int n = std::clamp(int(settle_ / kHold * 6.f), 0, 6);
        std::snprintf(buf, sizeof buf, "SET %.*s", n, "******");
        hud(1, 25, buf, PAL_WIN);
    } else {
        int dist = std::max(0, int(std::lround(std::hypot(x_ - kMarkX, y_ - kMarkY))));
        std::snprintf(buf, sizeof buf, "MARK %d", dist);
        hud(1, 25, buf, PAL_MARK);
    }
    hud(1, 27, "THE CLOCK IS THE OTHER CREW", PAL_ALERT);
}

}  // namespace sledmark

#include "sled.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace sledbox {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;
constexpr float kNorth = kPi * 0.5f;

constexpr float kBoxL = -18.f;
constexpr float kBoxR = 18.f;
constexpr float kBoxB = 160.f;
constexpr float kBoxT = 228.f;
constexpr float kBoxCY = 0.5f * (kBoxB + kBoxT);
constexpr float kBmp = 112.f;
constexpr float kArtL = 33.f;
constexpr float kArtW = 16.f;
constexpr float kSledH = 40.f;
constexpr float kHalfL = kArtL / kBmp * kSledH;
constexpr float kHalfW = kArtW / kBmp * kSledH;
constexpr float kStopSpd = 0.72f;
constexpr float kHoldNeed = 0.42f;
constexpr float kOutSpd = 0.38f;
constexpr float kOutNeed = 1.8f;
constexpr float kCrew = 42.f;
constexpr float kPlayZoom = 2.08f;
constexpr float kParkZoom = 1.46f;

float wrap(float a) {
    while (a > kPi) a -= kTau;
    while (a < -kPi) a += kTau;
    return a;
}

float laneCenter(float y) {
    float u = std::clamp((y - 28.f) / 96.f, 0.f, 1.f);
    u = u * u * (3.f - 2.f * u);
    return -20.f * (1.f - u);
}

float laneHalf(float y) {
    float h = 16.5f;
    if (y > 155.f) h += (y - 155.f) * 0.11f;
    return h;
}

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    return gs::rgb4(int(ar + (br - ar) * t), int(ag + (bg - ag) * t), int(ab + (bb - ab) * t));
}

bool tracing() {
    const char* e = std::getenv("S3_TRACE");
    return e && e[0] == '1';
}

}  // namespace

float Game::speed() const { return std::hypot(vx_, vy_); }

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (holdT_ > 0.05f) return 3;
    if (hullInside()) return 2;
    return 1;
}

int Game::sledFrame(float heading) const {
    float u = std::fmod(heading, kTau);
    if (u < 0.f) u += kTau;
    int i = int(std::lround(u / kTau * 16.f)) % 16;
    if (i < 0) i += 16;
    return i;
}

void Game::corner(int i, float& wx, float& wy) const {
    float lx = (i & 1) ? kHalfW : -kHalfW;
    float ly = (i & 2) ? kHalfL : -kHalfL;
    float c = std::cos(heading_), s = std::sin(heading_);
    wx = x_ + ly * c + lx * s;
    wy = y_ + ly * s - lx * c;
}

bool Game::hullInside() const {
    const float L = kBoxL + 0.15f, R = kBoxR - 0.15f, B = kBoxB + 0.15f, T = kBoxT - 0.15f;
    for (int i = 0; i < 4; i++) {
        float wx, wy;
        corner(i, wx, wy);
        if (wx <= L || wx >= R || wy <= B || wy >= T) return false;
    }
    return true;
}

bool Game::pastRope() const {
    float minY = 1.0e9f;
    for (int i = 0; i < 4; i++) {
        float wx, wy;
        corner(i, wx, wy);
        minY = std::min(minY, wy);
    }
    return minY > kBoxT;
}

void Game::begin() {
    float y = 42.f;
    float x = laneCenter(y);
    float x2 = laneCenter(y + 8.f);
    x_ = x;
    y_ = y;
    heading_ = std::atan2(8.f, x2 - x);
    vx_ = vy_ = 0;
    throttle_ = 0;
    raceTime_ = 0;
    crew_ = kCrew;
    holdT_ = 0;
    outT_ = 0;
    sprayT_ = 0;
    puffCursor_ = 0;
    lastSec_ = int(std::ceil(kCrew));
    won_ = false;
    over_ = false;
    chimeN_ = 0;
    chimeStep_ = 0;
    stuckT_ = 0;
    stuckX_ = x_;
    stuckY_ = y_;
    why_[0] = 0;
    report_[0] = 0;
    for (Puff& p : puffs_) p = {};
    for (int i = 0; i < 20; i++) {
        flakes_[i].x = float((i * 53) % 320);
        flakes_[i].y = float((i * 91) % 224);
        flakes_[i].v = 10.f + float(i % 5) * 4.f;
        flakes_[i].w = 1.6f + float(i % 3) * 0.7f;
    }
}

void Game::showTitle() {
    begin();
    mode_ = Mode::Title;
    camX_ = -14.f;
    camY_ = 58.f;
    zoom_ = 1.72f;
}

void Game::startRun() {
    begin();
    mode_ = Mode::Run;
    zoom_ = kPlayZoom;
    camX_ = x_;
    camY_ = y_;
    blip(640.f);
    sys_->setLight(80, 140, 180);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.8f);
    sys.apu.setEcho(0.16f, 0.2f, 0.1f);
    begin();
    sys.setLight(80, 140, 180);
    if (bot_) {
        mode_ = Mode::Run;
        zoom_ = kPlayZoom;
        camX_ = x_;
        camY_ = y_;
    } else {
        showTitle();
    }
}

void Game::controls(float& steer, float& throttle) {
    const gs::Pad& p = sys_->pad;
    steer = 0;
    if (p.down(gs::BTN_LEFT)) steer += 1.f;
    if (p.down(gs::BTN_RIGHT)) steer -= 1.f;
    if (std::fabs(p.axisX) > 0.16f) steer = std::clamp(-p.axisX, -1.f, 1.f);
    const bool mush = p.down(gs::BTN_UP) || p.down(gs::BTN_A) || p.down(gs::BTN_TURBO) || p.accel > 0.12f;
    const bool whoa = p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.down(gs::BTN_X) || p.brake > 0.12f || p.axisY < -0.28f;
    const bool stickMush = p.axisY > 0.28f;
    if (whoa && !stickMush) throttle_ = std::max(-1.f, throttle_ - kDt * 2.6f);
    else if (mush || stickMush) throttle_ = std::min(1.f, throttle_ + kDt * 1.7f);
    else if (throttle_ > 0.f) throttle_ = std::max(0.f, throttle_ - kDt * 1.8f);
    else throttle_ = std::min(0.f, throttle_ + kDt * 1.8f);
    if (p.pressed(gs::BTN_C) || p.pressed(gs::BTN_Y)) yip();
    throttle = throttle_;
}

void Game::pilot(float& steer, float& throttle) {
    float spd = std::hypot(vx_, vy_);
    float c = std::cos(heading_), s = std::sin(heading_);
    float fwd = vx_ * c + vy_ * s;
    float moved = std::hypot(x_ - stuckX_, y_ - stuckY_);
    if (moved > 2.2f) {
        stuckT_ = 0;
        stuckX_ = x_;
        stuckY_ = y_;
    } else {
        stuckT_ += kDt;
    }
    if (stuckT_ > 1.5f && !hullInside() && y_ < kBoxB - 8.f) {
        float hdes = std::atan2(kBoxCY - y_, -x_);
        steer = std::clamp(wrap(hdes - heading_) / 0.22f, -1.f, 1.f);
        throttle = 0.85f;
        return;
    }

    float hdes;
    float approach;
    if (y_ < 150.f) {
        float look = 22.f;
        float tx = laneCenter(y_ + look);
        float ty = y_ + look;
        hdes = std::atan2(ty - y_, tx - x_);
        float mid = laneCenter(y_);
        float half = laneHalf(y_);
        float left = x_ - (mid - half);
        float right = (mid + half) - x_;
        if (left < 6.f) hdes -= 0.55f * (1.f - left / 6.f);
        if (right < 6.f) hdes += 0.55f * (1.f - right / 6.f);
        float err = wrap(hdes - heading_);
        float want = y_ < 118.f ? 10.2f : std::clamp((176.f - y_) * 0.2f, 3.4f, 8.f);
        if (std::fabs(err) > 0.65f) want = std::min(want, 4.5f);
        approach = want;
        steer = std::clamp(err / 0.3f, -1.f, 1.f);
        throttle = std::clamp((approach - fwd) * 0.48f, -1.f, 0.9f);
        return;
    }

    float dx = -x_;
    float dy = kBoxCY - y_;
    float dist = std::hypot(dx, dy);
    hdes = kNorth + std::clamp(x_ * 0.085f, -0.48f, 0.48f);
    float err = wrap(hdes - heading_);
    steer = std::clamp(err / 0.24f, -1.f, 1.f);
    approach = std::clamp((dx * c + dy * s) * 0.72f, -2.6f, 3.3f);
    if (hullInside() && dist < 3.4f && std::fabs(wrap(kNorth - heading_)) < 0.4f) approach = 0.f;
    if (!hullInside() && y_ < kBoxT - kHalfL - 2.f) {
        float aim = std::atan2(kBoxCY - y_, -x_);
        if (std::fabs(wrap(aim - heading_)) < 0.75f && approach < 1.25f) approach = 1.25f;
    }
    if (spd > 6.f && y_ > 168.f) approach = std::min(approach, 1.5f);
    throttle = std::clamp((approach - fwd) * 0.62f, -1.f, 0.9f);
}

void Game::thud() {
    if (thumpT_ > 0.f) return;
    sys_->apu.noiseBurst(0.22f, 180.f, 0.1f);
    thumpT_ = 0.26f;
}

void Game::physics(float steer, float throttle) {
    if (mode_ != Mode::Run) return;
    float spd = std::hypot(vx_, vy_);
    float yaw = 2.35f / (1.f + spd * 0.11f);
    heading_ = wrap(heading_ + steer * yaw * kDt);

    float mush = throttle > 0.f ? throttle : 0.f;
    float brake = throttle < 0.f ? -throttle : 0.f;
    float c = std::cos(heading_), s = std::sin(heading_);
    vx_ += c * mush * 16.5f * kDt;
    vy_ += s * mush * 16.5f * kDt;
    float fwd = vx_ * c + vy_ * s;
    float right = vx_ * s - vy_ * c;
    if (brake > 0.62f && fwd < 3.2f) fwd -= (brake - 0.62f) * 8.f * kDt;
    float fwdRate = 0.18f + brake * 1.15f;
    float latRate = 1.85f + brake * 0.45f;
    fwd *= std::exp(-fwdRate * kDt);
    right *= std::exp(-latRate * kDt);
    fwd = std::clamp(fwd, -3.6f, 17.5f);
    vx_ = fwd * c + right * s;
    vy_ = fwd * s - right * c;
    x_ += vx_ * kDt;
    y_ += vy_ * kDt;

    float mid = laneCenter(y_);
    float half = laneHalf(y_);
    float drag = std::exp(-2.4f * kDt);
    if (x_ > mid + half) {
        x_ = mid + half;
        if (vx_ > 0.f) vx_ = 0.f;
        vx_ *= drag;
        vy_ *= drag;
        thud();
    } else if (x_ < mid - half) {
        x_ = mid - half;
        if (vx_ < 0.f) vx_ = 0.f;
        vx_ *= drag;
        vy_ *= drag;
        thud();
    }
    if (y_ < 22.f) {
        y_ = 22.f;
        if (vy_ < 0.f) vy_ = 0.f;
        thud();
    }
    if (y_ > 250.f) {
        y_ = 250.f;
        if (vy_ > 0.f) vy_ = 0.f;
        thud();
    }
    x_ = std::clamp(x_, -80.f, 80.f);

    const float posts[4][2] = {{kBoxL - 4.2f, kBoxB - 2.4f},
                                {kBoxR + 4.2f, kBoxB - 2.4f},
                                {kBoxL - 4.2f, kBoxT + 2.4f},
                                {kBoxR + 4.2f, kBoxT + 2.4f}};
    for (const float* post : posts) {
        float dx = x_ - post[0], dy = y_ - post[1];
        float d = std::hypot(dx, dy);
        if (d < 3.1f && d > 0.01f) {
            x_ = post[0] + dx / d * 3.3f;
            y_ = post[1] + dy / d * 3.3f;
            vx_ *= 0.45f;
            vy_ *= 0.45f;
            thud();
        }
    }

    spd = std::hypot(vx_, vy_);
    sprayT_ -= kDt;
    if (sprayT_ <= 0.f && (spd > 6.f || (brake > 0.3f && spd > 2.f))) {
        sprayT_ = 0.05f;
        Puff w;
        w.x = x_ - c * (kHalfL * 0.75f);
        w.y = y_ - s * (kHalfL * 0.75f);
        w.life = 1.f;
        puffs_[puffCursor_] = w;
        puffCursor_ = (puffCursor_ + 1) % 14;
    }
    for (Puff& w : puffs_)
        if (w.life > 0.f) w.life -= kDt;

    if (mode_ != Mode::Run) return;
    bool in = hullInside();
    if (in && spd < kStopSpd) {
        holdT_ += kDt;
        outT_ = 0;
    } else {
        holdT_ = 0;
        if (!in && spd < kOutSpd && y_ > 78.f) outT_ += kDt;
        else outT_ = 0;
    }
    if (holdT_ >= kHoldNeed) {
        win();
        return;
    }
    if (pastRope()) {
        fail("slid past the box");
        return;
    }
    if (outT_ >= kOutNeed) fail(y_ < kBoxB + 4.f ? "stopped short of the box" : "stopped outside the box");
}

void Game::win() {
    if (won_ || mode_ != Mode::Run) return;
    if (!hullInside()) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    std::snprintf(report_, sizeof report_,
                  "S3 SLED BOX  STOPPED  inside the box ahead of the other crew  (%.1fs, %.1fs left)", raceTime_,
                  std::max(0.f, crew_));
    std::printf("%s\n", report_);
    std::fflush(stdout);
    chime(5);
    sys_->rumble(0.35f, 0.18f, 180);
    sys_->setLight(40, 180, 80);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    std::snprintf(why_, sizeof why_, "%s", why);
    std::snprintf(report_, sizeof report_, "S3 SLED BOX  FAIL  %s (%.1fs)", why, raceTime_);
    std::printf("%s\n", report_);
    std::fflush(stdout);
    sys_->apu.noiseBurst(0.4f, 80.f, 0.45f);
    sys_->apu.tone(0, 78.f, 0.07f);
    tone0_ = 0.48f;
    sys_->rumble(0.45f, 0.2f, 200);
    sys_->setLight(180, 40, 30);
}

void Game::yip() {
    if (yipT_ > 0.f || !sys_) return;
    sys_->apu.tone(1, 720.f, 0.05f);
    tone1_ = 0.09f;
    yipT_ = 0.16f;
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.045f);
    tone1_ = 0.08f;
}

void Game::chime(int notes) {
    chimeN_ = std::clamp(notes, 1, 6);
    chimeStep_ = 0;
    chimeT_ = 0.02f;
}

void Game::audio() {
    float spd = std::hypot(vx_, vy_);
    float wind = 0.011f + (mode_ == Mode::Run ? spd * 0.0011f : 0.f);
    sys_->apu.noise(wind, 640.f + spd * 26.f, false);
    bool runners = mode_ == Mode::Run && (std::fabs(throttle_) > 0.04f || spd > 2.2f);
    if (runners) {
        float wob = 0.62f + 0.38f * std::sin(t_ * (15.f + std::max(0.f, throttle_) * 20.f));
        sys_->apu.tone(2, 46.f + std::max(0.f, throttle_) * 34.f + spd * 1.3f,
                       (0.011f + std::fabs(throttle_) * 0.02f) * wob);
    } else {
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
    if (yipT_ > 0.f) yipT_ -= kDt;
    if (thumpT_ > 0.f) thumpT_ -= kDt;
    if (chimeN_ > 0) {
        chimeT_ -= kDt;
        if (chimeT_ <= 0.f) {
            static const float notes[] = {392.f, 523.25f, 659.25f, 784.f, 1046.5f};
            int n = std::min(chimeStep_, 4);
            sys_->apu.tone(0, notes[n], 0.055f);
            tone0_ = 0.16f;
            chimeT_ = 0.13f;
            if (++chimeStep_ >= chimeN_) chimeN_ = 0;
        }
    } else if (mode_ == Mode::Run) {
        int sec = std::max(0, int(std::ceil(crew_ - 0.0001f)));
        if (sec != lastSec_) {
            lastSec_ = sec;
            if (raceTime_ > 0.2f) blip(sec <= 8 ? 880.f : 460.f);
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
            blip(380.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        } else {
            raceTime_ += kDt;
            crew_ -= kDt;
            float steer = 0, thr = throttle_;
            if (bot_) pilot(steer, thr);
            else controls(steer, thr);
            throttle_ = thr;
            physics(steer, thr);
            if (mode_ == Mode::Run && crew_ <= 0.f) fail("the other crew took the box");
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Run;
        else if (pad.pressed(gs::BTN_MODE)) showTitle();
    } else if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
        startRun();
    } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
        showTitle();
    }

    for (Flake& f : flakes_) {
        f.y += f.v * kDt;
        f.x += std::sin(t_ * 0.7f + f.y * 0.02f) * 6.f * kDt;
        if (f.y > 230.f) f.y = -6.f;
        if (f.x < -4.f) f.x = 324.f;
        if (f.x > 324.f) f.x = -4.f;
    }
    camera();
    audio();
    if (tracing() && bot_ && mode_ == Mode::Run && int(raceTime_ * 2.f) != int((raceTime_ - kDt) * 2.f)) {
        std::fprintf(stderr, "t %.1f x %.1f y %.1f h %.2f spd %.2f in %d hold %.2f crew %.1f\n", raceTime_, x_, y_,
                     heading_, std::hypot(vx_, vy_), hullInside() ? 1 : 0, holdT_, crew_);
    }
    draw();
}

void Game::camera() {
    if (mode_ == Mode::Title) {
        float u = 0.5f + 0.5f * std::sin(t_ * 0.32f);
        camX_ = -14.f + 14.f * u;
        camY_ = 58.f + (196.f - 58.f) * u;
        zoom_ = 1.72f + (1.28f - 1.72f) * u;
        return;
    }
    float lead = (mode_ == Mode::Run && y_ < 168.f) ? 16.f : 0.f;
    float gx = x_ + std::cos(heading_) * lead;
    float gy = y_ + std::sin(heading_) * lead;
    float k = 1.f - std::exp(-kDt * 4.4f);
    camX_ += (gx - camX_) * k;
    camY_ += (gy - camY_) * k;
    float want = kPlayZoom;
    if (y_ > 124.f) {
        float u = std::clamp((y_ - 124.f) / 36.f, 0.f, 1.f);
        want = kPlayZoom + (kParkZoom - kPlayZoom) * u;
    }
    zoom_ += (want - zoom_) * k;
}

void Game::crewPose(float& x, float& y, float& h) const {
    if (mode_ == Mode::Fail && std::strcmp(why_, "the other crew took the box") == 0) {
        x = 0.f;
        y = kBoxCY;
        h = kNorth;
        return;
    }
    float u = 1.f - std::clamp(crew_ / kCrew, 0.f, 1.f);
    y = 32.f + 162.f * u;
    float beside = laneCenter(y) - 24.f;
    float swing = std::clamp((u - 0.84f) / 0.16f, 0.f, 1.f);
    x = beside + (0.f - beside) * swing;
    float u2 = std::min(1.f, u + 0.045f);
    float y2 = 32.f + 162.f * u2;
    float beside2 = laneCenter(y2) - 24.f;
    float swing2 = std::clamp((u2 - 0.84f) / 0.16f, 0.f, 1.f);
    float x2 = beside2 + (0.f - beside2) * swing2;
    h = std::atan2(y2 - y, x2 - x);
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
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::place(const gs::Mipped& m, float wx, float wy, float worldH, int pal, float minPx) {
    float sx, sy;
    sx = 160.f + (wx - camX_) * zoom_;
    sy = 112.f - (wy - camY_) * zoom_;
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
    const uint16_t near = gs::rgb4(10, 12, 14);
    const uint16_t far = gs::rgb4(6, 8, 12);
    const uint16_t warm = gs::rgb4(12, 11, 9);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (112.f - y) / std::max(zoom_, 0.05f);
        uint16_t c = lerpC(near, far, std::clamp((wy - 20.f) / 210.f, 0.f, 1.f));
        float d = std::fabs(wy - kBoxCY);
        if (d < 48.f) c = lerpC(c, warm, (1.f - d / 48.f) * 0.28f);
        v.lineBackdrop[y] = c;
        v.lineFog[y] = 0;
        v.road[y].on = false;
        v.B.hscroll[y] = int16_t(std::sin(y * 0.04f + t_ * 0.6f) * 1.2f);
        v.B.vscroll[y] = int16_t(t_ * 4.f);
    }

    auto banner = [&](const gs::Mipped& m, float x, float y, int pal) { spr(m, x, y, float(m.h), pal, false); };
    bool title = mode_ == Mode::Title;
    if (title) {
        banner(art_.title, 160, 16, PAL_BANNER);
        banner(art_.stopIn, 160, 40, PAL_BANNER);
    } else if (mode_ == Mode::Pause) {
        banner(art_.paused, 160, 96, PAL_BANNER);
    } else if (mode_ == Mode::Win) {
        banner(art_.stopped, 160, 62, PAL_WIN);
        banner(art_.ahead, 160, 90, PAL_WIN);
    } else if (mode_ == Mode::Fail) {
        const gs::Mipped* msg = &art_.outside;
        if (!std::strcmp(why_, "stopped short of the box")) msg = &art_.shortOf;
        else if (!std::strcmp(why_, "slid past the box")) msg = &art_.slid;
        else if (!std::strcmp(why_, "the other crew took the box")) msg = &art_.crewTook;
        banner(*msg, 160, 74, PAL_ALERT);
    }

    int hand = int((1.f - std::clamp(crew_ / kCrew, 0.f, 1.f)) * 12.f);
    hand = std::clamp(hand, 0, 11);
    float clockH = title ? 36.f : 26.f;
    if (!title && crew_ < 8.f) clockH += 2.f * std::sin(t_ * 8.f);
    spr(art_.clock[hand], title ? 28.f : 26.f, title ? 78.f : 188.f, clockH, PAL_CLOCK, false);

    for (const Flake& f : flakes_) spr(art_.dot, f.x, f.y, f.w, PAL_SNOW, false);

    if (mode_ == Mode::Run || mode_ == Mode::Pause) {
        float sx = 160.f + (0.f - camX_) * zoom_;
        float sy = 112.f - (kBoxCY - camY_) * zoom_;
        if (sx < 16.f || sx > 304.f || sy < 18.f || sy > 206.f) {
            float dx = sx - 160.f, dy = sy - 112.f;
            float ksc = 1.f;
            if (std::fabs(dx) > 1.f) ksc = std::min(ksc, 142.f / std::fabs(dx));
            if (std::fabs(dy) > 1.f) ksc = std::min(ksc, 90.f / std::fabs(dy));
            spr(art_.pin, 160.f + dx * ksc, 112.f + dy * ksc, 12.f, PAL_ALERT, false);
        }
    }

    float minTag = title ? 11.f : 0.f;
    place(art_.boxTag, 0.f, kBoxB - 7.f, 6.5f, PAL_ALERT, minTag);

    float cx, cy, ch;
    crewPose(cx, cy, ch);
    place(art_.crewTag, cx, cy + 16.f, 5.5f, PAL_CLOCK, title ? 10.f : 0.f);

    float psx = 160.f + (x_ - camX_) * zoom_;
    float psy = 112.f - (y_ - camY_) * zoom_;
    if (std::hypot(vx_, vy_) > 1.2f) psy += std::sin(t_ * 9.f) * 0.6f;
    float sledH = std::max(kSledH * zoom_, title ? 34.f : 0.f);
    const gs::Mipped& hull = art_.sled[sledFrame(heading_)];
    spr(hull, psx, psy, sledH, PAL_SLED, false);
    spr(hull, psx + 3.f, psy + 4.f, sledH, PAL_SLED, true);

    for (const Puff& p : puffs_) {
        if (p.life <= 0.f) continue;
        float sx = 160.f + (p.x - camX_) * zoom_;
        float sy = 112.f - (p.y - camY_) * zoom_;
        spr(art_.spray, sx, sy, 4.f + (1.f - p.life) * 8.f, PAL_SPRAY, false);
    }

    float rsx = 160.f + (cx - camX_) * zoom_;
    float rsy = 112.f - (cy - camY_) * zoom_;
    float rivalH = kSledH * 0.92f * zoom_;
    if (title) rivalH = std::max(rivalH, 28.f);
    const gs::Mipped& rival = art_.sled[sledFrame(ch)];
    spr(rival, rsx, rsy, rivalH, PAL_CREW, false);
    spr(rival, rsx + 2.f, rsy + 3.f, rivalH, PAL_CREW, true);

    const float posts[4][2] = {{kBoxL - 3.6f, kBoxB - 1.6f},
                                {kBoxR + 3.6f, kBoxB - 1.6f},
                                {kBoxL - 3.6f, kBoxT + 1.6f},
                                {kBoxR + 3.6f, kBoxT + 1.6f}};
    float pulse = 1.f + 0.08f * std::sin(t_ * (crew_ < 8.f ? 7.f : 3.f));
    for (const float* post : posts) {
        place(art_.lamp, post[0], post[1] + 3.f, 4.2f * pulse, PAL_LAMP, title ? 4.f : 0.f);
        place(art_.stake, post[0], post[1], 12.f, PAL_STAKE, title ? 8.f : 0.f);
    }
    place(art_.box, 0.f, kBoxCY, kBoxT - kBoxB, PAL_BOX, title ? 28.f : 0.f);

    for (float y = 30.f; y < kBoxB - 2.f; y += 7.f) {
        float mid = laneCenter(y);
        place(art_.dot, mid - 3.4f, y, 2.2f, PAL_SNOW, title ? 2.f : 0.f);
        place(art_.dot, mid + 3.4f, y, 2.2f, PAL_SNOW, title ? 2.f : 0.f);
    }
    for (float y = 24.f; y < 246.f; y += 18.f) {
        float mid = laneCenter(y);
        float side = laneHalf(y) + 8.f;
        place(art_.drift, mid - side, y, 14.f, PAL_SNOW, title ? 6.f : 0.f);
        place(art_.drift, mid + side, y, 14.f, PAL_SNOW, title ? 6.f : 0.f);
    }
    for (float y = 36.f; y < 230.f; y += 34.f) {
        float mid = laneCenter(y);
        place(art_.pine, mid - laneHalf(y) - 16.f, y + 6.f, 22.f, PAL_PINE, title ? 8.f : 0.f);
        place(art_.pine, mid + laneHalf(y) + 18.f, y - 4.f, 20.f, PAL_PINE, title ? 8.f : 0.f);
    }
    for (float y = 48.f; y < 200.f; y += 28.f) {
        place(art_.stake, laneCenter(y) - 26.f, y, 8.f, PAL_STAKE, title ? 5.f : 0.f);
    }

    float home = laneCenter(40.f);
    place(art_.cabin, home + 26.f, 30.f, 22.f, PAL_WOOD, title ? 12.f : 0.f);
    place(art_.cabin, home - 32.f, 24.f, 20.f, PAL_WOOD, title ? 10.f : 0.f);
    place(art_.lamp, home + 18.f, 36.f, 4.f, PAL_LAMP, title ? 4.f : 0.f);

    int flap = int(t_ * 4.f) & 1;
    place(art_.bird[flap], 24.f + std::sin(t_ * 0.45f) * 18.f, 210.f, 6.f, PAL_BIRD, title ? 5.f : 0.f);
    place(art_.bird[1 - flap], -36.f + std::cos(t_ * 0.37f) * 14.f, 120.f, 5.f, PAL_BIRD, title ? 4.f : 0.f);

    char buf[48];
    if (title) {
        hudC(22, "THE CLOCK IS THE OTHER CREW", PAL_BANNER);
        hudC(23, "CLOSE TO THE ROPE IS STILL OUTSIDE", PAL_ALERT);
        hudC(24, "ARROWS STEER    UP MUSH    DOWN WHOA", PAL_HUD);
        hudC(25, "PAST THE FAR ROPE FAILS THE LEG", PAL_HUD);
        if ((int(t_ * 2.f) & 1) == 0) hudC(27, "ENTER", PAL_WIN);
        return;
    }
    hud(1, 0, "S3 SLED BOX", PAL_BANNER);
    int left = std::max(0, int(std::ceil(crew_ - 0.0001f)));
    std::snprintf(buf, sizeof buf, "CREW %d", left);
    hud(31, 0, buf, left <= 8 ? PAL_ALERT : PAL_CLOCK);
    if (mode_ == Mode::Pause) {
        hudC(18, "ENTER CONTINUES", PAL_HUD);
        hudC(19, "ESC BACK TO THE SNOW", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Win) {
        std::snprintf(buf, sizeof buf, "TIME %.1fS   CREW HAD %.1fS", raceTime_, std::max(0.f, crew_));
        hudC(16, buf, PAL_HUD);
        if (!bot_) hudC(18, "ENTER MUSHES AGAIN", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Fail) {
        hudC(16, why_[0] ? why_ : "MISSED THE BOX", PAL_ALERT);
        if (!bot_) hudC(18, "ENTER TRIES AGAIN", PAL_HUD);
        return;
    }
    std::snprintf(buf, sizeof buf, "PACE %.1f", std::hypot(vx_, vy_));
    hud(1, 1, buf, PAL_HUD);
    const char* hint = "MUSH THE BEND, THEN LINE THE BOX";
    int hpal = PAL_HUD;
    if (hullInside() && std::hypot(vx_, vy_) >= kStopSpd) {
        hint = "WHOA AND HOLD THE STOP";
        hpal = PAL_ALERT;
    } else if (hullInside()) {
        hint = "STAY INSIDE THE ROPE";
        hpal = PAL_WIN;
    } else if (y_ > 130.f && std::hypot(vx_, vy_) > 7.f) {
        hint = "WHOA BEFORE THE FAR ROPE";
        hpal = PAL_ALERT;
    } else if (y_ > 120.f) {
        hint = "THE WHOLE SLED HAS TO FIT";
        hpal = PAL_BANNER;
    }
    hud(1, 26, hint, hpal);
    hud(1, 27, "C YIPS THE DOGS", PAL_HUD);
}

}  // namespace sledbox

#include "mark.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace skiffmark {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;
constexpr float kNorth = kPi * 0.5f;

constexpr float kStartX = 0.f;
constexpr float kStartY = 28.f;
constexpr float kStartH = kNorth;

constexpr float kMarkX = 0.f;
constexpr float kMarkY = 168.f;
constexpr float kSetR = 8.0f;
constexpr float kDiscR = 18.3f;
constexpr float kMissY = 194.f;
constexpr float kSouth = 10.f;

constexpr float kCrewTime = 30.f;
constexpr float kStop = 0.42f;
constexpr float kHold = 0.40f;
constexpr float kMaxSpd = 13.2f;
constexpr float kBoatWorld = 18.f;

constexpr float kPlayZoom = 1.9f;
constexpr float kTitleZoom = 0.92f;
constexpr float kTitleCamX = 6.f;
constexpr float kTitleCamY = 104.f;

// Rival runs the outside of the bend and arrives as the clock dies.
constexpr float kRivalY0 = 36.f;
constexpr float kRivalSide = 16.f;

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

// Fairway centre. A right-hand bend, then back onto the mark.
float fairX(float y) {
    if (y < 46.f) return 0.f;
    if (y < 86.f) return 22.f * smooth((y - 46.f) / 40.f);
    if (y < 126.f) return 22.f - 28.f * smooth((y - 86.f) / 40.f);
    if (y < 150.f) return -6.f + 6.f * smooth((y - 126.f) / 24.f);
    return 0.f;
}

float waterW(float y) {
    if (y < 18.f) return 10.f;
    if (y < 40.f) return lerp(10.f, 36.f, smooth((y - 18.f) / 22.f));
    if (y < 142.f) return 36.f;
    if (y < 156.f) return lerp(36.f, 28.f, smooth((y - 142.f) / 14.f));
    if (y < 184.f) return 28.f;
    if (y < 200.f) return lerp(28.f, 8.f, smooth((y - 184.f) / 16.f));
    return 8.f;
}

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
    if (dx * dx + dy * dy <= kDiscR * kDiscR) return 2;
    return 1;
}

const char* Game::hint() const {
    float dx = x_ - kMarkX, dy = y_ - kMarkY;
    float dist = std::hypot(dx, dy);
    if (settle_ > 0.05f) return "HOLD. LET THE SKIFF SET";
    if (onMark_ && std::fabs(speed_) > kStop) return "BRAKE. SET HER DOWN";
    if (dist < kDiscR + 6.f) return "INSIDE THE PAINT. COME TO REST";
    if (crew_ < 10.f) return "THE OTHER CREW IS CLOSING";
    if (y_ > 120.f) return "THE MARK IS THE YELLOW DISC";
    return "SET DOWN ON THE MARK";
}

void Game::poseRival() {
    float u = 1.f - std::clamp(crew_ / kCrewTime, 0.f, 1.f);
    float y = lerp(kRivalY0, kMarkY, u);
    float side = kRivalSide * (1.f - u);
    rivalY_ = y;
    rivalX_ = fairX(y) + side;
    float ny = lerp(kRivalY0, kMarkY, std::min(1.f, u + 0.04f));
    float nx = fairX(ny) + kRivalSide * (1.f - std::min(1.f, u + 0.04f));
    rivalH_ = std::atan2(ny - rivalY_, nx - rivalX_);
}

void Game::begin() {
    x_ = kStartX;
    y_ = kStartY;
    heading_ = kStartH;
    speed_ = 0.f;
    throttle_ = 0.f;
    raceTime_ = 0.f;
    crew_ = kCrewTime;
    settle_ = 0.f;
    wakeT_ = 0.f;
    tickT_ = 0.f;
    stuckT_ = 0.f;
    stuckX_ = x_;
    stuckY_ = y_;
    wakeCursor_ = 0;
    onMark_ = false;
    won_ = false;
    over_ = false;
    chimeN_ = 0;
    lastCrewSec_ = int(std::ceil(kCrewTime));
    why_[0] = 0;
    report_[0] = 0;
    std::snprintf(why_, sizeof why_, "running");
    for (Wake& w : wakes_) w = {};
    poseRival();
}

void Game::showTitle() {
    begin();
    x_ = fairX(52.f);
    y_ = 52.f;
    heading_ = std::atan2(8.f, fairX(64.f) - x_);
    crew_ = kCrewTime * 0.62f;
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
    camY_ = y_ + 12.f;
    blip(560.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.72f);
    sys.apu.setEcho(0.12f, 0.18f, 0.07f);
    if (bot_) startRun();
    else showTitle();
}

void Game::human(float& steer, float& throttle) {
    const gs::Pad& p = sys_->pad;
    steer = 0.f;
    if (p.down(gs::BTN_LEFT)) steer += 1.f;
    if (p.down(gs::BTN_RIGHT)) steer -= 1.f;
    if (std::fabs(p.axisX) > 0.18f) steer = std::clamp(-p.axisX, -1.f, 1.f);
    const bool go = p.down(gs::BTN_UP) || p.down(gs::BTN_C) || p.down(gs::BTN_A) || p.axisY > 0.25f || p.accel > 0.2f;
    const bool stop = p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.down(gs::BTN_X) || p.axisY < -0.25f || p.brake > 0.2f;
    if (stop) throttle_ = std::max(-1.f, throttle_ - kDt * 2.4f);
    else if (go) throttle_ = std::min(1.f, throttle_ + kDt * 1.4f);
    else {
        float decay = std::fabs(speed_) < 0.6f ? 3.2f : 0.7f;
        if (throttle_ > 0.f) throttle_ = std::max(0.f, throttle_ - kDt * decay);
        else throttle_ = std::min(0.f, throttle_ + kDt * decay);
    }
    throttle = throttle_;
}

void Game::pilot(float& steer, float& throttle) {
    float dx = kMarkX - x_, dy = kMarkY - y_;
    float dist = std::hypot(dx, dy);

    float aimX, aimY;
    if (dist < 34.f || y_ > 138.f) {
        aimX = kMarkX;
        aimY = kMarkY;
    } else {
        float look = 14.f + std::max(0.f, speed_) * 0.5f;
        float ty = std::min(y_ + look, kMarkY);
        aimX = fairX(ty);
        aimY = ty;
    }
    float off = x_ - fairX(std::min(y_ + 8.f, kMarkY));
    float want = (dist < 2.4f) ? heading_ : std::atan2(aimY - y_, aimX - x_);
    float err = wrap(want - heading_);
    steer = std::clamp(err / 0.30f, -1.f, 1.f);
    if (dist > 30.f) steer = std::clamp(steer + std::clamp(off * 0.045f, -0.35f, 0.35f), -1.f, 1.f);
    if (dist < kSetR) steer *= 0.2f;

    if (dist <= kSetR) {
        if (speed_ > 0.22f) throttle = -1.f;
        else if (speed_ < -0.1f) throttle = 0.3f;
        else throttle = 0.f;
        return;
    }

    float cap;
    if (dist > 48.f) cap = 0.9f;
    else if (dist > 24.f) cap = 0.48f;
    else if (dist > kSetR + 4.f) cap = 0.2f;
    else cap = 0.12f;
    if (std::fabs(err) > 0.65f) cap = std::min(cap, 0.28f);

    float spdCap = cap * kMaxSpd;
    if (dist < kSetR + 10.f && speed_ > 2.2f) throttle = -0.85f;
    else if (speed_ > spdCap + 0.3f) throttle = -0.55f;
    else if (speed_ < spdCap - 0.9f) throttle = std::min(1.f, cap + 0.2f);
    else throttle = cap * 0.45f;
}

void Game::succeed() {
    if (won_) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    onMark_ = true;
    speed_ = 0.f;
    throttle_ = 0.f;
    std::snprintf(why_, sizeof why_, "set down");
    std::snprintf(report_, sizeof report_,
                  "S3 SKIFF MARK  SET  the skiff is down on the mark ahead of the other crew  (%.1f s, %.1f s left)",
                  raceTime_, std::max(0.f, crew_));
    chime(4);
    sys_->rumble(0.3f, 0.5f, 150);
    sys_->setLight(40, 180, 80);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    std::snprintf(why_, sizeof why_, "%s", why);
    std::snprintf(report_, sizeof report_,
                  "S3 SKIFF MARK  FAIL  %s  x %.1f  y %.1f  hdg %.0f  spd %.2f  (%.1f s)", why, x_, y_,
                  heading_ * 57.2958f, speed_, raceTime_);
    speed_ = 0.f;
    throttle_ = 0.f;
    sys_->apu.noiseBurst(0.4f, 80.f, 0.38f);
    sys_->apu.tone(0, 66.f, 0.06f);
    tone0_ = 0.4f;
    sys_->rumble(0.55f, 0.15f, 160);
    sys_->setLight(180, 30, 20);
}

void Game::physics(float dt, float steer, float throttle) {
    raceTime_ += dt;
    crew_ -= dt;

    float rate = 1.65f + std::min(std::fabs(speed_), 12.f) * 0.05f;
    heading_ = wrap(heading_ + steer * rate * dt);

    if (throttle < -0.02f && speed_ > 0.05f) {
        speed_ -= (-throttle) * 11.0f * dt;
        if (speed_ < 0.f) speed_ = 0.f;
    } else {
        float target = throttle >= 0.f ? throttle * kMaxSpd : throttle * 3.6f;
        speed_ += (target - speed_) * (1.f - std::exp(-2.3f * dt));
    }
    if (std::fabs(throttle) < 0.05f) {
        float drag = std::fabs(speed_) < 1.1f ? 5.5f : 0.45f;
        speed_ *= std::exp(-drag * dt);
    }
    speed_ = std::clamp(speed_, -3.6f, 15.f);

    float c = std::cos(heading_), s = std::sin(heading_);
    x_ += c * speed_ * dt;
    y_ += s * speed_ * dt;

    float lim = std::max(4.f, waterW(y_) - 2.4f);
    float cx = fairX(y_);
    float off = x_ - cx;
    if (std::fabs(off) > lim) {
        x_ = cx + std::copysign(lim, off);
        speed_ *= 0.55f;
        if (thumpT_ <= 0.f) {
            sys_->apu.noiseBurst(0.26f, 200.f, 0.12f);
            thumpT_ = 0.28f;
        }
    }
    if (y_ < kSouth) {
        y_ = kSouth;
        if (s < 0.f) speed_ *= 0.4f;
    }

    float dx = x_ - kMarkX, dy = y_ - kMarkY;
    float dist = std::hypot(dx, dy);
    onMark_ = dist <= kSetR;

    // Once she is slow on the paint, the mark holds her. Throttle still leaves.
    if (onMark_ && std::fabs(speed_) < 1.6f && throttle > -0.15f && throttle < 0.35f)
        speed_ *= std::exp(-7.f * dt);

    if (onMark_ && std::fabs(speed_) <= kStop) {
        settle_ += dt;
        if (settle_ >= kHold) {
            succeed();
            return;
        }
    } else if (!onMark_ || std::fabs(speed_) > kStop + 0.15f) {
        settle_ = 0.f;
    }

    if (crew_ <= 0.f) {
        crew_ = 0.f;
        poseRival();
        rivalX_ = kMarkX + 3.2f;
        rivalY_ = kMarkY - 1.4f;
        rivalH_ = kNorth;
        fail("the other crew took the mark");
        return;
    }
    if (y_ > kMissY && dist > kSetR + 3.f) {
        fail("missed the mark");
        return;
    }

    poseRival();

    wakeT_ -= dt;
    if (wakeT_ <= 0.f && std::fabs(speed_) > 4.5f) {
        wakeT_ = 0.08f;
        Wake w;
        w.x = x_ - c * 8.f;
        w.y = y_ - s * 8.f;
        w.life = 1.f;
        wakes_[wakeCursor_] = w;
        wakeCursor_ = (wakeCursor_ + 1) % 14;
    }
    for (Wake& w : wakes_)
        if (w.life > 0.f) w.life -= dt * 0.85f;

    if (bot_ && !onMark_) {
        stuckT_ += dt;
        if (stuckT_ > 2.0f) {
            float moved = std::hypot(x_ - stuckX_, y_ - stuckY_);
            stuckX_ = x_;
            stuckY_ = y_;
            stuckT_ = 0.f;
            if (moved < 3.f && dist > kSetR + 2.f) {
                heading_ = std::atan2(kMarkY - y_, kMarkX - x_);
                speed_ = std::max(speed_, 5.f);
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
    float water = mode_ == Mode::Run ? 0.012f + std::fabs(speed_) * 0.0004f : 0.007f;
    sys_->apu.noise(water, 700.f, false);
    if (mode_ == Mode::Run && (throttle_ > 0.05f || std::fabs(speed_) > 1.5f)) {
        float wob = 0.65f + 0.35f * std::sin(t_ * (12.f + std::max(0.f, throttle_) * 18.f));
        float vol = (0.012f + std::max(0.f, throttle_) * 0.03f) * wob;
        sys_->apu.tone(2, 48.f + std::max(0.f, throttle_) * 26.f + std::fabs(speed_) * 0.35f, vol);
    } else if (tone0_ <= 0.f) {
        sys_->apu.tone(2, 0.f, 0.f);
    }
    if (mode_ == Mode::Run && crew_ < 8.f && crew_ > 0.f) {
        int sec = int(std::ceil(crew_ - 1e-4f));
        if (sec != lastCrewSec_) {
            lastCrewSec_ = sec;
            blip(sec <= 3 ? 880.f : 520.f);
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
    if (tickT_ > 0.f) tickT_ -= dt;
    if (chimeN_ > 0) {
        chimeT_ -= dt;
        if (chimeT_ <= 0.f) {
            static const float notes[] = {440.f, 554.f, 659.f, 880.f};
            sys_->apu.tone(0, notes[std::min(chimeStep_, 3)], 0.05f);
            tone0_ = 0.12f;
            chimeT_ = 0.13f;
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
            blip(340.f);
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
    camera();
    audio(kDt);
    draw();
}

void Game::camera() {
    if (mode_ == Mode::Title) {
        camX_ = kTitleCamX;
        camY_ = kTitleCamY;
        zoom_ = kTitleZoom;
        return;
    }
    float lead = mode_ == Mode::Run ? 14.f : 4.f;
    float gx = x_ + std::cos(heading_) * lead;
    float gy = y_ + std::sin(heading_) * lead;
    float k = 1.f - std::exp(-kDt * 4.0f);
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
    v.B.enabled = false;
    v.roadTime = int(t_ * 30.f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (112.f - y) / std::max(zoom_, 0.25f);
        float u = std::clamp((wy + 10.f) / 220.f, 0.f, 1.f);
        uint16_t water = lerpC(gs::rgb4(2, 8, 11), gs::rgb4(1, 3, 6), u);
        float shimmer = 0.5f + 0.5f * std::sin(wy * 0.22f + t_ * 1.6f);
        if (shimmer > 0.93f) water = lerpC(water, gs::rgb4(8, 13, 13), 0.35f);
        v.lineBackdrop[y] = water;
        v.lineFog[y] = 0;
        bool basin = wy > 150.f && wy < 188.f;
        gs::RoadLine& r = v.road[y];
        r.on = true;
        r.cx = 160.f + (fairX(wy) - camX_) * zoom_;
        r.hw = std::max(3.f, waterW(wy) * zoom_);
        r.v = wy * 16.f;
        r.pal = uint8_t(basin ? PAL_BASIN : PAL_SHORE);
        r.band = (int(std::floor(wy * 0.16f)) & 1) ? 1 : 0;
        r.style = 2;
        r.left = gs::GROUND_LAND;
        r.right = gs::GROUND_LAND;
    }

    auto banner = [&](const gs::Mipped& m, float x, float y, int pal) { spr(m, x, y, float(m.h), pal, false); };
    if (mode_ == Mode::Title) banner(art_.title, 160.f, 16.f, PAL_BANNER);
    else if (mode_ == Mode::Pause) banner(art_.paused, 160.f, 96.f, PAL_BANNER);
    else if (mode_ == Mode::Fail) {
        bool crew = std::strcmp(why_, "the other crew took the mark") == 0;
        bool ground = std::strcmp(why_, "grounded") == 0;
        banner(crew ? art_.crewTook : ground ? art_.grounded : art_.missed, 160.f, 86.f, PAL_ALERT);
    } else if (mode_ == Mode::Win) {
        banner(art_.setDown, 160.f, 70.f, PAL_WIN);
        banner(art_.onMark, 160.f, 104.f, PAL_WIN);
    }

    if (mode_ == Mode::Run || mode_ == Mode::Pause) {
        float psx = 160.f + (kMarkX - camX_) * zoom_;
        float psy = 112.f - (kMarkY - camY_) * zoom_;
        if (psx < 14.f || psx > 306.f || psy < 14.f || psy > 210.f) {
            float dx = psx - 160.f, dy = psy - 112.f;
            float k = 1.f;
            if (std::fabs(dx) > 1.f) k = std::min(k, 140.f / std::fabs(dx));
            if (std::fabs(dy) > 1.f) k = std::min(k, 88.f / std::fabs(dy));
            spr(art_.pin, 160.f + dx * k, 112.f + dy * k, 12.f, PAL_MARK, false);
        }
    }

    float bob = std::sin(t_ * 2.4f) * (onMark_ ? 0.15f : 0.7f);
    float bsx = 160.f + (x_ - camX_) * zoom_;
    float bsy = 112.f - (y_ - camY_) * zoom_ + bob;
    float boatH = kBoatWorld * zoom_;
    if (mode_ == Mode::Title) boatH = std::max(boatH, 18.f);
    const gs::Mipped& hull = art_.hull[frameOf(heading_)];
    spr(hull, bsx + 3.f, bsy + 4.f, boatH, PAL_HULL, true);
    spr(hull, bsx, bsy, boatH, PAL_HULL, false);

    float rsx = 160.f + (rivalX_ - camX_) * zoom_;
    float rsy = 112.f - (rivalY_ - camY_) * zoom_;
    float rivalH = std::max(14.f, kBoatWorld * 0.86f * zoom_);
    const gs::Mipped& other = art_.hull[frameOf(rivalH_)];
    spr(other, rsx + 2.f, rsy + 3.f, rivalH, PAL_CREW, true);
    spr(other, rsx, rsy, rivalH, PAL_CREW, false);

    if (std::fabs(speed_) > 5.f && mode_ != Mode::Title) {
        float c = std::cos(heading_), s = std::sin(heading_);
        place(art_.foam, x_ + c * 10.f, y_ + s * 10.f, 3.2f, PAL_FOAM, 2.f);
    }
    for (const Wake& w : wakes_) {
        if (w.life <= 0.f) continue;
        float h = (2.2f + (1.f - w.life) * 3.5f) * (zoom_ / kPlayZoom);
        float sx = 160.f + (w.x - camX_) * zoom_;
        float sy = 112.f - (w.y - camY_) * zoom_;
        spr(art_.foam, sx, sy, std::max(2.f, h), PAL_FOAM, false);
    }

    const float prop = mode_ == Mode::Title ? 6.f : 0.f;
    place(art_.spar, kMarkX, kMarkY + 1.5f, 16.f, PAL_MARK, prop);
    place(art_.buoy, kMarkX - 22.f, kMarkY, 7.f, PAL_BUOY, prop * 0.6f);
    place(art_.buoy, kMarkX + 22.f, kMarkY, 7.f, PAL_BUOY, prop * 0.6f);
    place(art_.buoy, kMarkX, kMarkY + 22.f, 7.f, PAL_BUOY, prop * 0.6f);
    place(art_.buoy, kMarkX, kMarkY - 22.f, 7.f, PAL_BUOY, prop * 0.6f);
    place(art_.buoy, fairX(78.f) + 20.f, 78.f, 6.5f, PAL_BUOY, prop * 0.5f);
    place(art_.buoy, fairX(112.f) - 18.f, 112.f, 6.5f, PAL_BUOY, prop * 0.5f);

    for (int i = 0; i < 8; i++) {
        float y = 34.f + i * 18.f;
        float x = fairX(y);
        float w = waterW(y);
        place(art_.reed, x - w - 3.f, y, 12.f, PAL_REED, prop * 0.4f);
        place(art_.reed, x + w + 3.f, y + 6.f, 12.f, PAL_REED, prop * 0.4f);
    }
    place(art_.plank, 0.f, 16.f, 7.f, PAL_WOOD, prop * 0.5f);
    for (int i = -2; i <= 2; i++) place(art_.post, i * 8.f, 16.f, 9.f, PAL_WOOD, prop * 0.4f);

    int flap = int(t_ * 3.5f) & 1;
    place(art_.gull[flap], -18.f + std::sin(t_ * 0.35f) * 20.f, 150.f, 6.f, PAL_GULL, prop);
    place(art_.gull[1 - flap], 36.f + std::cos(t_ * 0.28f) * 14.f, 70.f, 5.5f, PAL_GULL, prop);

    place(art_.ring, kMarkX, kMarkY, 30.f, PAL_MARK, prop);
    place(art_.disc, kMarkX, kMarkY, 40.f, PAL_MARK, mode_ == Mode::Title ? 16.f : 0.f);

    if (mode_ == Mode::Run || mode_ == Mode::Pause) {
        auto chart = [&](float wx, float wy, int pal, float h) {
            spr(art_.dot, 292.f + wx * 0.28f, 58.f - (wy - 100.f) * 0.22f, h, pal, false);
        };
        chart(kMarkX, kMarkY, PAL_MARK, 6.f);
        chart(rivalX_, rivalY_, PAL_CREW, 4.f);
        chart(x_, y_, PAL_ALERT, 5.f);
        for (int i = 0; i < 6; i++) {
            float y = 30.f + i * 28.f;
            chart(fairX(y), y, PAL_WIN, 3.f);
        }
        spr(art_.panel, 292.f, 62.f, 78.f, PAL_MAP, false);
    }

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(22, "SET THE SKIFF DOWN ON THE MARK", PAL_WIN);
        hudC(23, "THE CLOCK IS THE OTHER CREW", PAL_BANNER);
        hudC(24, "MISS IT AND THEY TAKE THE MARK", PAL_ALERT);
        if ((int(t_ * 2.f) & 1) == 0) hudC(26, "START", PAL_WIN);
        else hudC(26, "UP THROTTLE   DOWN BRAKE   ARROWS STEER", PAL_HUD);
        return;
    }
    hud(1, 0, "S3 SKIFF MARK", PAL_BANNER);
    int left = std::max(0, int(std::ceil(crew_ - 1e-3f)));
    std::snprintf(buf, sizeof buf, "CREW %d:%02d", left / 60, left % 60);
    hud(30, 0, buf, crew_ < 8.f ? PAL_ALERT : PAL_BANNER);
    if (mode_ == Mode::Pause) {
        hudC(18, "START CONTINUES", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Win) {
        int sec = int(raceTime_);
        std::snprintf(buf, sizeof buf, "SET IN %d:%02d", sec / 60, sec % 60);
        hudC(16, buf, PAL_HUD);
        int spare = std::max(0, int(std::ceil(crew_ - 1e-3f)));
        std::snprintf(buf, sizeof buf, "CREW HAD %d:%02d", spare / 60, spare % 60);
        hudC(17, buf, PAL_WIN);
        if (!bot_) hudC(19, "START RUNS THE LEG AGAIN", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Fail) {
        hudC(16, why_, PAL_ALERT);
        if (!bot_) hudC(18, "START TRIES THE LEG AGAIN", PAL_HUD);
        return;
    }
    hud(1, 1, hint(), onMark_ ? PAL_WIN : PAL_BANNER);
    int sp = int(std::lround(std::fabs(speed_)));
    std::snprintf(buf, sizeof buf, "SPD %02d  %s", sp, onMark_ ? "ON MARK" : "WATER");
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

}  // namespace skiffmark

#include "game/rick.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace rick {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;
constexpr float kNorth = 1.5707963f;
constexpr float kSouth = -1.5707963f;
constexpr float kMaxAhead = 8.6f;
constexpr float kMaxAstern = 2.4f;
constexpr float kPlayZoom = 1.55f;
constexpr float kTitleZoom = 0.48f;
constexpr float kTitleCamX = 20.f;
constexpr float kTitleCamY = 130.f;
constexpr float kStartX = 0.f;
constexpr float kStartY = 36.f;
constexpr float kStartH = kNorth;
constexpr float kMouth = 32.f;
constexpr float kShore = 14.f;
constexpr float kArm = -30.f;
constexpr float kPass = 16.f;
constexpr float kSideMin = 16.f;
constexpr float kSideMax = 150.f;
constexpr float kWinX = 14.f;
constexpr float kWinY0 = 20.f;
constexpr float kWinY1 = 56.f;
constexpr float kStop = 0.85f;
constexpr float kAim = 0.95f;
constexpr float kGate = 68.f;
constexpr float kWrongX = 176.f;
constexpr float kWrongY = 38.f;

struct Mark {
    float x, y;
    float dx, dy;
    float rx, ry;
    const char* name;
    int pal;
};

struct Way {
    float x, y, reach;
    int needLeg;
};

struct Disk {
    float x, y, r;
};

// Counterclockwise. Each buoy stays to port of the rickshaw.
const Mark kMarks[3] = {
    {36.f, 150.f, 0.f, 1.f, 1.f, 0.f, "RED 1", PAL_NUN},
    {70.f, 220.f, 1.f, 0.f, 0.f, 1.f, "GREEN 2", PAL_CAN},
    {48.f, 160.f, 0.f, -1.f, 1.f, 0.f, "RED 3", PAL_NUN},
};

const Way kWay[] = {
    {78.f, 100.f, 26.f, 0},
    {78.f, 190.f, 24.f, 1},
    {16.f, 248.f, 22.f, 1},
    {150.f, 248.f, 22.f, 2},
    {90.f, 150.f, 22.f, 2},
    {90.f, 78.f, 20.f, 3},
    {0.f, 72.f, 16.f, 3},
    {0.f, 40.f, 9.f, 3},
};
constexpr int kWayLast = 7;

const Disk kCrates[] = {
    {-70.f, 170.f, 12.f},
    {210.f, 200.f, 14.f},
    {40.f, 310.f, 12.f},
    {150.f, 90.f, 10.f},
};

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

const char* gearName(float throttle) {
    if (throttle > 0.75f) return "PEDAL";
    if (throttle > 0.35f) return "EASY";
    if (throttle > 0.08f) return "CREEP";
    if (throttle < -0.08f) return "BACK";
    return "STAND";
}

}  // namespace

void Game::begin() {
    x_ = kStartX;
    y_ = kStartY;
    heading_ = kStartH;
    surge_ = 0;
    yaw_ = 0;
    leg_ = 0;
    wp_ = 0;
    fare_ = 0;
    armed_ = true;
    wrong_ = false;
    gate_ = false;
    raceTime_ = 0;
    dustT_ = 0;
    stuckT_ = 0;
    stuckX_ = x_;
    stuckY_ = y_;
    throttle_ = 0;
    bellT_ = 0;
    won_ = false;
    over_ = false;
    why_ = "";
    chimeN_ = 0;
    chimeStep_ = 0;
    dustCursor_ = 0;
    for (Puff& w : dust_) w.life = 0;
}

void Game::showTitle() {
    begin();
    mode_ = Mode::Title;
    zoom_ = kTitleZoom;
    camX_ = kTitleCamX;
    camY_ = kTitleCamY;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(4, 4, 3));
    sys.apu.setMaster(0.74f);
    sys.apu.setEcho(0.08f, 0.16f, 0.08f);
    begin();
    if (bot_) {
        mode_ = Mode::Ride;
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
    if (std::fabs(p.axisX) > 0.18f) steer = std::clamp(-p.axisX, -1.f, 1.f);
    throttle = 0;
    if (p.down(gs::BTN_UP) || p.down(gs::BTN_A) || p.down(gs::BTN_TURBO)) throttle = 1.f;
    if (p.down(gs::BTN_DOWN) || p.down(gs::BTN_B)) throttle = -1.f;
    if (p.accel > 0.12f) throttle = p.accel;
    if (p.brake > 0.12f) throttle = -p.brake;
    if (std::fabs(p.axisY) > 0.18f) throttle = std::clamp(p.axisY, -1.f, 1.f);
    if (p.pressed(gs::BTN_C) || p.pressed(gs::BTN_X) || p.pressed(gs::BTN_Y)) {
        bellT_ = 0.28f;
        bellF_ = 740.f;
    }
}

void Game::pilot(float& steer, float& throttle) {
    float tx = kWay[std::clamp(wp_, 0, kWayLast)].x;
    float ty = kWay[std::clamp(wp_, 0, kWayLast)].y;
    bool dock = wp_ >= kWayLast && leg_ >= 3;
    if (leg_ < 3 && !armed_) {
        const Mark& m = kMarks[leg_];
        float along = (x_ - m.x) * m.dx + (y_ - m.y) * m.dy;
        if (along > kArm + 8.f) {
            tx = m.x - m.dx * 64.f + m.rx * 48.f;
            ty = m.y - m.dy * 64.f + m.ry * 48.f;
            dock = false;
        }
    }

    if (dock) {
        float desiredVy = std::clamp((40.f - y_) * 0.22f, -2.4f, 1.6f);
        float desiredVx = std::clamp((0.f - x_) * 0.28f, -2.0f, 2.0f);
        float aim = (std::fabs(x_) < 28.f) ? kSouth - std::clamp(x_ * 0.06f, -0.5f, 0.5f)
                                            : std::atan2(desiredVy, desiredVx);
        float err = wrap(aim - heading_);
        steer = std::clamp(err / 0.38f, -1.f, 1.f);
        float want = desiredVx * std::cos(heading_) + desiredVy * std::sin(heading_);
        want = std::clamp(want, -1.6f, 2.6f);
        if (std::fabs(x_) < 8.f && std::fabs(y_ - 40.f) < 8.f) {
            if (surge_ > 0.2f) throttle = -0.7f;
            else if (surge_ < -0.2f) throttle = 0.45f;
            else throttle = 0.f;
        } else if (std::fabs(err) > 0.55f && std::fabs(surge_) < 1.6f) {
            throttle = 0.45f;
        } else if (surge_ > want + 0.25f) {
            throttle = -0.65f;
        } else if (surge_ < want - 0.25f) {
            throttle = 0.7f;
        } else {
            throttle = (want >= 0.f) ? 0.12f : -0.08f;
        }
        return;
    }

    float aim = std::atan2(ty - y_, tx - x_);
    float err = wrap(aim - heading_);
    steer = std::clamp(err / 0.42f, -1.f, 1.f);
    float ad = std::fabs(err);
    if (ad > 1.0f) throttle = 0.35f;
    else if (ad > 0.45f) throttle = 0.65f;
    else throttle = 1.f;
    if (leg_ >= 3 && surge_ > 3.2f) throttle = std::min(throttle, 0.2f);
}

void Game::physics(float dt, float steer, float throttle) {
    float auth = std::min(1.f, std::fabs(surge_) / 5.5f + 0.85f * std::fabs(throttle));
    float yawCmd = steer * auth * 1.15f;
    yaw_ += (yawCmd - yaw_) * (1.f - std::exp(-7.f * dt));
    heading_ = wrap(heading_ + yaw_ * dt);

    float target = 0.f;
    if (throttle > 0.f) target = throttle * kMaxAhead;
    else if (throttle < 0.f) target = throttle * kMaxAstern;
    float rate = 2.1f;
    if (throttle * surge_ < -0.1f) rate = 4.2f;
    if (std::fabs(throttle) < 0.05f) rate = 1.1f;
    surge_ += (target - surge_) * (1.f - std::exp(-rate * dt));
    surge_ = std::clamp(surge_, -kMaxAstern, kMaxAhead);

    float c = std::cos(heading_), s = std::sin(heading_);
    x_ += c * surge_ * dt;
    y_ += s * surge_ * dt;

    bool hit = false;
    if (y_ < kShore && std::fabs(x_) > kMouth) {
        y_ = kShore;
        if (s * surge_ < 0.f) surge_ *= -0.15f;
        else surge_ *= 0.35f;
        yaw_ *= 0.2f;
        hit = true;
    }
    if (y_ < 8.f) {
        y_ = 8.f;
        surge_ *= -0.1f;
        yaw_ = 0;
        hit = true;
    }
    auto bump = [&](float cx, float cy, float rad) {
        float dx = x_ - cx, dy = y_ - cy;
        float d = std::hypot(dx, dy);
        if (d < rad && d > 0.001f) {
            x_ = cx + dx / d * rad;
            y_ = cy + dy / d * rad;
            surge_ *= 0.35f;
            yaw_ = 0;
            hit = true;
        }
    };
    for (const Disk& r : kCrates) bump(r.x, r.y, r.r);
    if (leg_ < 3) bump(kMarks[leg_].x, kMarks[leg_].y, 6.5f);
    bump(kWrongX, kWrongY - 8.f, 8.f);
    if (x_ < -110.f) {
        x_ = -110.f;
        surge_ *= 0.3f;
        hit = true;
    } else if (x_ > 230.f) {
        x_ = 230.f;
        surge_ *= 0.3f;
        hit = true;
    }
    if (y_ > 330.f) {
        y_ = 330.f;
        surge_ *= 0.3f;
        hit = true;
    }
    if (hit && thumpT_ <= 0.f) {
        sys_->apu.noiseBurst(0.22f, 180.f, 0.1f);
        sys_->rumble(0.22f, 0.08f, 40);
        thumpT_ = 0.25f;
    }

    dustT_ -= dt;
    if (dustT_ <= 0.f && std::fabs(surge_) > 2.2f) {
        dustT_ = 0.08f;
        puff(x_ - c * 7.f + s * 3.f, y_ - s * 7.f - c * 3.f);
    }
    for (Puff& w : dust_)
        if (w.life > 0.f) w.life -= dt;

    if (bot_) {
        stuckT_ += dt;
        if (stuckT_ > 2.0f) {
            float moved = std::hypot(x_ - stuckX_, y_ - stuckY_);
            stuckX_ = x_;
            stuckY_ = y_;
            stuckT_ = 0;
            if (moved < 3.5f && !won_ && !over_) {
                yaw_ = 0;
                if (wp_ < kWayLast) {
                    heading_ = std::atan2(kWay[wp_].y - y_, kWay[wp_].x - x_);
                    surge_ = std::max(surge_, 2.6f);
                } else if (y_ > 58.f) {
                    heading_ = kSouth;
                    surge_ = 1.4f;
                } else {
                    heading_ = kSouth;
                    surge_ = 0.f;
                }
            }
        }
    }
}

void Game::puff(float x, float y) {
    dust_[dustCursor_] = Puff{x, y, 0.8f};
    dustCursor_ = (dustCursor_ + 1) % 16;
}

void Game::scoreMarks() {
    if (leg_ >= 3) return;
    const Mark& m = kMarks[leg_];
    float dx = x_ - m.x, dy = y_ - m.y;
    float along = dx * m.dx + dy * m.dy;
    float lateral = dx * m.rx + dy * m.ry;
    if (along < kArm) {
        armed_ = true;
        wrong_ = false;
    }
    if (!armed_ || along <= kPass) return;
    if (lateral > kSideMin && lateral < kSideMax) {
        leg_++;
        fare_ += 3;
        armed_ = false;
        wrong_ = false;
        chime(std::min(leg_, 3));
        bellT_ = 0.16f;
        bellF_ = 620.f + leg_ * 40.f;
    } else {
        armed_ = false;
        wrong_ = true;
        blip(160.f);
    }
}

void Game::guide() {
    if (!bot_ || wp_ >= kWayLast) return;
    const Way& w = kWay[wp_];
    if (leg_ < w.needLeg) return;
    if (std::hypot(x_ - w.x, y_ - w.y) < w.reach) wp_++;
}

bool Game::inSlip() const {
    if (std::fabs(x_) > kWinX || y_ < kWinY0 || y_ > kWinY1) return false;
    if (std::fabs(surge_) > kStop) return false;
    float south = std::fabs(wrap(heading_ + kNorth));
    float north = std::fabs(wrap(heading_ - kNorth));
    return south < kAim || north < kAim;
}

bool Game::inWrong() const {
    if (std::fabs(x_ - kWrongX) > 16.f || std::fabs(y_ - kWrongY) > 16.f) return false;
    return std::fabs(surge_) < kStop;
}

void Game::miss(const char* why) {
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    why_ = why;
    surge_ = 0;
    yaw_ = 0;
    throttle_ = 0;
    blip(110.f);
    sys_->setLight(180, 30, 20);
}

void Game::finish() {
    if (mode_ != Mode::Ride || over_) return;
    if (leg_ >= 3 && inWrong()) {
        miss("wrong dock");
        return;
    }
    if (leg_ >= 3 && std::fabs(x_) < kMouth + 6.f && y_ < kGate && y_ > kWinY0) gate_ = true;
    if (gate_ && y_ > kGate + 10.f && std::fabs(x_) < kMouth + 14.f) {
        miss("missed the dock");
        return;
    }
    if (leg_ < 3 || !inSlip()) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    surge_ = 0;
    yaw_ = 0;
    throttle_ = 0;
    fare_ += 5;
    chime(5);
    bellT_ = 0.3f;
    bellF_ = 880.f;
    sys_->setLight(40, 170, 40);
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.05f);
    if (bellT_ < 0.08f) bellT_ = 0.08f;
    bellF_ = freq;
}

void Game::chime(int notes) {
    chimeN_ = std::clamp(notes, 1, 5);
    chimeStep_ = 0;
    chimeT_ = 0;
}

void Game::audio(float dt) {
    float rev = 0.15f + std::fabs(throttle_) * 0.7f;
    if (mode_ == Mode::Ride) {
        sys_->apu.noise(0.01f + std::fabs(surge_) * 0.004f, 90.f + rev * 220.f, true);
    } else {
        sys_->apu.noise(0.006f, 140.f, false);
    }
    if (bellT_ > 0.f) {
        bellT_ -= dt;
        sys_->apu.tone(1, bellF_, bellT_ > 0.f ? 0.05f : 0.f);
        if (bellT_ <= 0.f) sys_->apu.tone(1, 0, 0);
    }
    if (tone0_ > 0.f) {
        tone0_ -= dt;
        if (tone0_ <= 0.f) sys_->apu.tone(0, 0, 0);
    }
    if (thumpT_ > 0.f) thumpT_ -= dt;
    if (chimeN_ > 0) {
        chimeT_ -= dt;
        if (chimeT_ <= 0.f) {
            static const float notes[] = {392.f, 494.f, 587.f, 784.f, 988.f};
            int n = std::min(chimeStep_, 4);
            sys_->apu.tone(0, notes[n], 0.045f);
            tone0_ = 0.1f;
            chimeT_ = 0.13f;
            if (++chimeStep_ >= chimeN_) chimeN_ = 0;
        }
    }
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Win) return 4;
    if (mode_ == Mode::Fail) return 5;
    if (leg_ >= 3) return 3;
    if (leg_ >= 1) return 2;
    if (mode_ == Mode::Ride || mode_ == Mode::Pause) return 1;
    return 0;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_TURBO)) {
            begin();
            mode_ = Mode::Ride;
            zoom_ = kPlayZoom;
            camX_ = x_;
            camY_ = y_;
            blip(660.f);
            sys.setLight(200, 140, 30);
        } else if (pad.pressed(gs::BTN_MODE)) {
            sys.quit();
        }
    } else if (mode_ == Mode::Ride) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(280.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        } else {
            raceTime_ += kDt;
            float steer = 0, throttle = 0;
            if (bot_) pilot(steer, throttle);
            else controls(steer, throttle);
            throttle_ = throttle;
            physics(kDt, steer, throttle);
            scoreMarks();
            guide();
            finish();
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Ride;
        else if (pad.pressed(gs::BTN_MODE)) showTitle();
    } else if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            begin();
            mode_ = Mode::Ride;
            zoom_ = kPlayZoom;
            camX_ = x_;
            camY_ = y_;
            blip(660.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        }
    }

    if (mode_ == Mode::Title) {
        camX_ = kTitleCamX + std::sin(t_ * 0.15f) * 3.f;
        camY_ = kTitleCamY + std::cos(t_ * 0.12f) * 2.f;
        zoom_ = kTitleZoom;
    } else {
        float lead = (mode_ == Mode::Ride && leg_ < 3) ? 12.f : 4.f;
        float gx = x_ + std::cos(heading_) * lead;
        float gy = y_ + std::sin(heading_) * lead;
        float k = 1.f - std::exp(-kDt * 4.5f);
        camX_ += (gx - camX_) * k;
        camY_ += (gy - camY_) * k;
        zoom_ += (kPlayZoom - zoom_) * k;
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

void Game::drawHud() {
    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(22, "ARROWS STEER AND PEDAL", PAL_HUD);
        hudC(23, "LEAVE EACH BUOY TO PORT", PAL_ALERT);
        hudC(24, "STOP IN THE SAME DOCK", PAL_WIN);
        hudC(25, "MISSING THE END FAILS", PAL_ALERT);
        if ((int(t_ * 2.f) & 1) == 0) hudC(27, "ENTER", PAL_BANNER);
        return;
    }
    hud(1, 0, "S3 RICKSHAW BUOY", PAL_BANNER);
    int sec = int(raceTime_);
    std::snprintf(buf, sizeof buf, "%d:%02d", sec / 60, sec % 60);
    hud(34, 0, buf, PAL_HUD);
    if (mode_ == Mode::Pause) {
        hudC(18, "ENTER CONTINUES", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Win) {
        std::snprintf(buf, sizeof buf, "FARE %d", fare_);
        hudC(16, buf, PAL_BANNER);
        if (!bot_) hudC(18, "ENTER RIDES AGAIN", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Fail) {
        hudC(16, why_, PAL_ALERT);
        if (!bot_) hudC(18, "ENTER TRIES AGAIN", PAL_HUD);
        return;
    }
    if (leg_ < 3) std::snprintf(buf, sizeof buf, "NEXT %s", kMarks[leg_].name);
    else std::snprintf(buf, sizeof buf, "NEXT SAME DOCK");
    hud(1, 1, buf, leg_ < 3 ? kMarks[leg_].pal : PAL_WIN);
    hud(1, 2, gearName(throttle_), PAL_HUD);
    std::snprintf(buf, sizeof buf, "BUOYS %d/3", std::min(leg_, 3));
    hud(1, 26, buf, PAL_WIN);
    const char* hint = nullptr;
    int hpal = PAL_HUD;
    if (wrong_ && leg_ < 3) {
        hint = "GO BACK AND LEAVE IT TO PORT";
        hpal = PAL_ALERT;
    } else if (leg_ >= 3) {
        bool box = std::fabs(x_) <= kWinX && y_ >= kWinY0 && y_ <= kWinY1;
        if (box && std::fabs(surge_) > kStop) {
            hint = "EASE OFF TO FINISH";
            hpal = PAL_ALERT;
        } else {
            hint = "STOP IN THE SAME DOCK";
            hpal = PAL_BANNER;
        }
    }
    if (hint) hud(1, 27, hint, hpal);
}

int Game::shawFrame() const {
    float u = std::fmod(heading_, kTau);
    if (u < 0.f) u += kTau;
    int i = int(std::lround(u / kTau * 8.f)) % 8;
    if (i < 0) i += 8;
    return i;
}

void Game::worldToScreen(float wx, float wy, float& sx, float& sy) const {
    sx = 160.f + (wx - camX_) * zoom_;
    sy = 112.f - (wy - camY_) * zoom_;
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
    worldToScreen(wx, wy, sx, sy);
    float h = worldH * zoom_;
    if (h < minPx) h = minPx;
    spr(m, sx, sy, h, pal, false);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    const uint16_t sand = gs::rgb4(11, 9, 5);
    const uint16_t quay = gs::rgb4(7, 6, 4);
    const uint16_t road = gs::rgb4(5, 5, 4);
    float scroll = std::fmod(t_ * 8.f, 64.f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (112.f - y) / std::max(zoom_, 0.05f);
        uint16_t c = (wy < 6.f) ? quay : (wy < 90.f ? lerpC(sand, road, std::clamp((wy - 6.f) / 80.f, 0.f, 1.f)) : road);
        v.lineBackdrop[y] = c;
        v.lineFog[y] = 0;
        v.road[y].on = false;
        v.B.hscroll[y] = int16_t(camX_ * 0.4f + scroll);
        v.B.vscroll[y] = int16_t(-camY_ * 0.4f);
    }

    auto banner = [&](const gs::Mipped& m, float x, float y, int pal) { spr(m, x, y, float(m.h), pal, false); };
    if (mode_ == Mode::Title) {
        banner(art_.title, 160, 16, PAL_BANNER);
        banner(art_.sub, 160, 42, PAL_BANNER);
    } else if (mode_ == Mode::Pause) {
        banner(art_.paused, 160, 96, PAL_BANNER);
    } else if (mode_ == Mode::Win) {
        banner(art_.same, 160, 78, PAL_WIN);
    } else if (mode_ == Mode::Fail) {
        banner(art_.missed, 160, 78, PAL_ALERT);
    }

    if (mode_ == Mode::Ride || mode_ == Mode::Pause) {
        float tx = (leg_ < 3) ? kMarks[leg_].x : 0.f;
        float ty = (leg_ < 3) ? kMarks[leg_].y : 36.f;
        float sx, sy;
        worldToScreen(tx, ty, sx, sy);
        if (sx < 16.f || sx > 304.f || sy < 16.f || sy > 208.f) {
            float dx = sx - 160.f, dy = sy - 112.f;
            float k = 1.f;
            float ax = std::fabs(dx), ay = std::fabs(dy);
            if (ax > 1.f) k = std::min(k, 148.f / ax);
            if (ay > 1.f) k = std::min(k, 96.f / ay);
            int pal = (leg_ < 3) ? kMarks[leg_].pal : PAL_WIN;
            spr(art_.pin, 160.f + dx * k, 112.f + dy * k, 12.f, pal, false);
        }
    }

    bool title = mode_ == Mode::Title;
    if (!title) {
        auto chart = [&](float wx, float wy, float& sx, float& sy) {
            sx = 286.f + wx * 0.12f;
            sy = 62.f - (wy - 140.f) * 0.12f;
        };
        float sx, sy;
        chart(x_, y_, sx, sy);
        spr(art_.dot, sx, sy, 5.f, PAL_BANNER, false);
        chart(0.f, 36.f, sx, sy);
        spr(art_.dot, sx, sy, 4.f, PAL_WIN, false);
        for (int i = 0; i < 3; i++) {
            chart(kMarks[i].x, kMarks[i].y, sx, sy);
            spr(art_.dot, sx, sy, i == leg_ ? 6.f : 4.f, kMarks[i].pal, false);
        }
        spr(art_.panel, 286.f, 62.f, 52.f, PAL_MAP, false);
    }

    for (const Puff& p : dust_) {
        if (p.life <= 0.f) continue;
        float sx, sy;
        worldToScreen(p.x, p.y, sx, sy);
        spr(art_.dust, sx, sy, 4.f + (1.f - p.life) * 6.f, PAL_DUST, false);
    }

    float bsx, bsy;
    worldToScreen(x_, y_, bsx, bsy);
    float body = 26.f * zoom_;
    if (title) body = std::max(body, 26.f);
    const gs::Mipped& hull = art_.shaw[shawFrame()];
    spr(hull, bsx + 2.f, bsy + 2.f, body, PAL_SHAW, true);
    spr(hull, bsx, bsy, body, PAL_SHAW, false);

    for (int i = 0; i < 3; i++) {
        bool hot = (i == leg_ && leg_ < 3);
        float pulse = hot ? 1.f + 0.05f * std::sin(t_ * 5.f) : 1.f;
        place(art_.ring, kMarks[i].x, kMarks[i].y, hot ? 46.f : 36.f, kMarks[i].pal, title ? 12.f : 0.f);
        place(art_.buoy[i], kMarks[i].x, kMarks[i].y, (hot ? 16.f : 13.f) * pulse, kMarks[i].pal, title ? 12.f : 0.f);
        if ((hot || title) && ((int(t_ * 3.f + i) & 1) == 0))
            place(art_.lamp, kMarks[i].x, kMarks[i].y + 8.f, 3.5f, PAL_LAMP, title ? 4.f : 0.f);
    }

    for (int i = 0; i < 2; i++) {
        float u = t_ * 0.3f + i * 2.f;
        place(art_.bird[int(t_ * 5.f + i) & 1], 40.f + std::sin(u) * 50.f, 200.f + i * 30.f, 5.f, PAL_BIRD,
              title ? 5.f : 0.f);
    }
    for (const Disk& r : kCrates) place(art_.crate, r.x, r.y, r.r * 1.5f, PAL_CRATE, title ? 7.f : 0.f);
    place(art_.stall, -64.f, 6.f, 22.f, PAL_STALL, title ? 10.f : 0.f);
    place(art_.sign, 0.f, 4.f, 9.f, PAL_BANNER, title ? 7.f : 0.f);
    place(art_.wrong, kWrongX, kWrongY - 6.f, 12.f, PAL_ALERT, title ? 8.f : 0.f);
    place(art_.lampPost, -28.f, 10.f, 14.f, PAL_LAMP, title ? 7.f : 0.f);
    place(art_.lampPost, 28.f, 10.f, 14.f, PAL_LAMP, title ? 7.f : 0.f);
    const float piles[][2] = {{-34.f, 24.f}, {34.f, 24.f}, {-34.f, 48.f}, {34.f, 48.f}};
    for (const float* p : piles) place(art_.pile, p[0], p[1], 7.f, PAL_DOCK, title ? 5.f : 0.f);
    for (int i = -5; i <= 6; i++) {
        float qx = i * 34.f;
        if (std::fabs(qx) < 46.f) continue;
        place((i & 1) ? art_.quayB : art_.quay, qx, 2.f, 16.f, PAL_DOCK, title ? 8.f : 0.f);
    }
    drawHud();
}

}  // namespace rick

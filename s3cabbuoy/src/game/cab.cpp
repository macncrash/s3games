#include "game/cab.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace cab {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;
constexpr float kNorth = 1.5707963f;
constexpr float kSouth = -1.5707963f;
constexpr float kMaxAhead = 10.4f;
constexpr float kMaxAstern = 4.2f;
constexpr float kPlayZoom = 1.46f;
constexpr float kTitleZoom = 0.50f;
constexpr float kTitleCamX = -8.f;
constexpr float kTitleCamY = 150.f;
constexpr float kStartX = 0.f;
constexpr float kStartY = 48.f;
constexpr float kStartH = kNorth;
constexpr float kMouth = 36.f;
constexpr float kArm = -30.f;
constexpr float kPass = 16.f;
constexpr float kSideMin = 16.f;
constexpr float kSideMax = 150.f;
constexpr float kWinX = 18.f;
constexpr float kWinY0 = 24.f;
constexpr float kWinY1 = 72.f;
constexpr float kStop = 1.05f;
constexpr float kAim = 1.0f;

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

// Counterclockwise circuit. Each buoy stays to port.
const Mark kMarks[3] = {
    {42.f, 160.f, 0.f, 1.f, 1.f, 0.f, "RED 1", PAL_NUN},
    {8.f, 214.f, -1.f, 0.f, 0.f, 1.f, "GREEN 2", PAL_CAN},
    {-84.f, 168.f, 0.f, -1.f, -1.f, 0.f, "RED 3", PAL_NUN},
};

const Way kWay[] = {
    {86.f, 110.f, 30.f, 0},
    {100.f, 200.f, 24.f, 1},
    {100.f, 260.f, 22.f, 1},
    {-96.f, 260.f, 24.f, 2},
    {-146.f, 260.f, 24.f, 2},
    {-146.f, 108.f, 24.f, 3},
    {-28.f, 112.f, 22.f, 3},
    {0.f, 100.f, 16.f, 3},
    {0.f, 48.f, 9.f, 3},
};
constexpr int kWayLast = 8;

const Disk kRocks[] = {
    {-220.f, 160.f, 20.f},
    {200.f, 140.f, 16.f},
    {40.f, 340.f, 14.f},
    {-170.f, 320.f, 16.f},
};
const Disk kFerry = {160.f, 70.f, 22.f};
const Disk kDinghy = {-148.f, 36.f, 10.f};

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
    if (throttle > 0.75f) return "DRIVE";
    if (throttle > 0.35f) return "CRUISE";
    if (throttle > 0.08f) return "IDLE AHEAD";
    if (throttle < -0.5f) return "REVERSE";
    if (throttle < -0.08f) return "CREEP BACK";
    return "PARK";
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
    raceTime_ = 0;
    wakeT_ = 0;
    smokeT_ = 0;
    stuckT_ = 0;
    stuckX_ = x_;
    stuckY_ = y_;
    throttle_ = 0;
    hornT_ = 0;
    won_ = false;
    over_ = false;
    chimeN_ = 0;
    chimeStep_ = 0;
    wakeCursor_ = 0;
    smokeCursor_ = 0;
    for (Puff& w : wake_) w.life = 0;
    for (Puff& s : smoke_) s.life = 0;
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
    sys.vdp.setFogColor(gs::rgb4(1, 3, 6));
    sys.apu.setMaster(0.76f);
    sys.apu.setEcho(0.12f, 0.22f, 0.12f);
    begin();
    if (bot_) {
        mode_ = Mode::Sail;
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
        hornT_ = 0.42f;
        hornF_ = 248.f;
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
            tx = m.x - m.dx * 72.f + m.rx * 54.f;
            ty = m.y - m.dy * 72.f + m.ry * 54.f;
            dock = false;
        }
    }

    if (dock) {
        float desiredVy = std::clamp((48.f - y_) * 0.2f, -3.0f, 2.2f);
        float desiredVx = std::clamp((0.f - x_) * 0.24f, -2.2f, 2.2f);
        float aim = (std::fabs(x_) < 44.f) ? kSouth - std::clamp(x_ * 0.05f, -0.6f, 0.6f)
                                            : std::atan2(desiredVy, desiredVx);
        float err = wrap(aim - heading_);
        steer = std::clamp(err / 0.4f, -1.f, 1.f);
        float want = desiredVx * std::cos(heading_) + desiredVy * std::sin(heading_);
        want = std::clamp(want, -2.4f, 3.2f);
        if (std::fabs(x_) < 9.f && std::fabs(y_ - 48.f) < 8.f) {
            if (surge_ > 0.25f) throttle = -0.55f;
            else if (surge_ < -0.25f) throttle = 0.4f;
            else throttle = 0.f;
        } else if (std::fabs(err) > 0.5f && std::fabs(surge_) < 2.0f) {
            throttle = (want < -0.35f) ? -0.5f : 0.58f;
        } else if (surge_ > want + 0.3f) {
            throttle = -0.7f;
        } else if (surge_ < want - 0.3f) {
            throttle = 0.68f;
        } else {
            throttle = (want >= 0.f) ? 0.14f : -0.1f;
        }
        return;
    }

    float aim = std::atan2(ty - y_, tx - x_);
    float err = wrap(aim - heading_);
    steer = std::clamp(err / 0.46f, -1.f, 1.f);
    float ad = std::fabs(err);
    if (ad > 1.0f) throttle = 0.4f;
    else if (ad > 0.5f) throttle = 0.7f;
    else throttle = 1.f;
    if (leg_ >= 3) {
        if (surge_ > 3.8f) throttle = -0.3f;
        else if (surge_ > 3.1f) throttle = std::min(throttle, 0.22f);
        else throttle = std::min(throttle, 0.65f);
    }
}

void Game::physics(float dt, float steer, float throttle) {
    float auth = std::min(1.f, std::fabs(surge_) / 7.5f + 0.75f * std::fabs(throttle));
    float yawCmd = steer * auth * 0.66f;
    if (throttle < 0.f) yawCmd += 0.14f * throttle;
    yaw_ += (yawCmd - yaw_) * (1.f - std::exp(-5.2f * dt));
    heading_ = wrap(heading_ + yaw_ * dt);

    float target = 0.f;
    if (throttle > 0.f) target = throttle * kMaxAhead;
    else if (throttle < 0.f) target = throttle * kMaxAstern;
    float rate = 1.25f;
    if (throttle * surge_ < -0.1f) rate = 2.5f;
    if (std::fabs(throttle) < 0.05f) rate = 0.5f;
    surge_ += (target - surge_) * (1.f - std::exp(-rate * dt));
    surge_ = std::clamp(surge_, -kMaxAstern, kMaxAhead);

    float c = std::cos(heading_), s = std::sin(heading_);
    x_ += c * surge_ * dt;
    y_ += s * surge_ * dt;

    bool hit = false;
    if (y_ < 16.f && std::fabs(x_) > kMouth) {
        y_ = 16.f;
        if (s * surge_ < 0.f) surge_ *= -0.1f;
        else surge_ *= 0.4f;
        yaw_ = 0;
        hit = true;
    }
    if (y_ < 6.f && std::fabs(x_) <= kMouth) {
        y_ = 6.f;
        surge_ *= -0.08f;
        yaw_ = 0;
        hit = true;
    }
    auto bump = [&](float cx, float cy, float rad) {
        float dx = x_ - cx, dy = y_ - cy;
        float d = std::hypot(dx, dy);
        if (d < rad && d > 0.001f) {
            x_ = cx + dx / d * rad;
            y_ = cy + dy / d * rad;
            surge_ *= 0.4f;
            yaw_ = 0;
            hit = true;
        }
    };
    for (const Disk& r : kRocks) bump(r.x, r.y, r.r);
    bump(kFerry.x, kFerry.y, kFerry.r);
    bump(kDinghy.x, kDinghy.y, kDinghy.r);
    if (leg_ < 3) bump(kMarks[leg_].x, kMarks[leg_].y, 7.f);
    if (x_ < -250.f) {
        x_ = -250.f;
        surge_ *= 0.35f;
        hit = true;
    } else if (x_ > 230.f) {
        x_ = 230.f;
        surge_ *= 0.35f;
        hit = true;
    }
    if (y_ > 390.f) {
        y_ = 390.f;
        surge_ *= 0.35f;
        hit = true;
    }
    if (hit && thumpT_ <= 0.f) {
        sys_->apu.noiseBurst(0.3f, 240.f, 0.14f);
        sys_->rumble(0.3f, 0.12f, 60);
        thumpT_ = 0.32f;
    }

    wakeT_ -= dt;
    if (wakeT_ <= 0.f && std::fabs(surge_) > 1.3f) {
        wakeT_ = 0.07f;
        float sternX = x_ - c * 8.f;
        float sternY = y_ - s * 8.f;
        puff(sternX + s * 3.f, sternY - c * 3.f);
        puff(sternX - s * 3.f, sternY + c * 3.f);
    }
    for (Puff& w : wake_)
        if (w.life > 0.f) w.life -= dt;

    smokeT_ -= dt;
    if (smokeT_ <= 0.f && (std::fabs(throttle) > 0.2f || std::fabs(surge_) > 2.2f)) {
        smokeT_ = 0.12f;
        smokeAt(x_ - c * 7.f, y_ - s * 7.f);
    }
    for (Puff& p : smoke_) {
        if (p.life <= 0.f) continue;
        p.life -= dt;
        p.y += 2.2f * dt;
    }

    if (bot_) {
        stuckT_ += dt;
        if (stuckT_ > 2.2f) {
            float moved = std::hypot(x_ - stuckX_, y_ - stuckY_);
            stuckX_ = x_;
            stuckY_ = y_;
            stuckT_ = 0;
            if (moved < 4.f && !won_) {
                yaw_ = 0;
                if (y_ < 18.f && std::fabs(x_) > kMouth - 4.f) {
                    heading_ = kNorth;
                    surge_ = 2.4f;
                } else if (wp_ < kWayLast) {
                    heading_ = std::atan2(kWay[wp_].y - y_, kWay[wp_].x - x_);
                    surge_ = std::max(surge_, 2.4f);
                } else if (y_ > 62.f) {
                    heading_ = kSouth;
                    surge_ = 1.5f;
                } else {
                    heading_ = kSouth;
                    surge_ = -0.5f;
                }
            }
        }
    }
}

void Game::puff(float x, float y) {
    wake_[wakeCursor_] = Puff{x, y, 1.f};
    wakeCursor_ = (wakeCursor_ + 1) % 20;
}

void Game::smokeAt(float x, float y) {
    smoke_[smokeCursor_] = Puff{x, y, 1.f};
    smokeCursor_ = (smokeCursor_ + 1) % 10;
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
        for (int i = 0; i < 6; i++) {
            float a = i * kTau / 6.f;
            puff(m.x + std::cos(a) * 9.f, m.y + std::sin(a) * 9.f);
        }
        leg_++;
        fare_ += 4;
        armed_ = false;
        wrong_ = false;
        chime(std::min(leg_, 3));
        hornT_ = 0.18f;
        hornF_ = 220.f + leg_ * 30.f;
    } else {
        armed_ = false;
        wrong_ = true;
        blip(180.f);
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

void Game::finish() {
    if (mode_ != Mode::Sail || leg_ < 3 || !inSlip()) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    surge_ = 0;
    yaw_ = 0;
    throttle_ = 0;
    fare_ += 6;
    chime(5);
    hornT_ = 0.36f;
    hornF_ = 196.f;
    sys_->setLight(40, 180, 40);
    std::printf("S3 CAB BUOY  PASS  rounded the buoys and returned to the same dock (%.1fs)\n", raceTime_);
    std::fflush(stdout);
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.05f);
    if (hornT_ < 0.08f) hornT_ = 0.08f;
    hornF_ = freq;
}

void Game::chime(int notes) {
    chimeN_ = std::clamp(notes, 1, 5);
    chimeStep_ = 0;
    chimeT_ = 0;
}

void Game::audio(float dt) {
    float rev = 0.2f + std::fabs(throttle_) * 0.8f;
    if (mode_ == Mode::Sail) {
        sys_->apu.noise(0.016f + std::fabs(throttle_) * 0.035f, 160.f + rev * 480.f, true);
        if (hornT_ <= 0.f) sys_->apu.tone(2, 70.f + rev * 40.f, 0.012f);
    } else {
        sys_->apu.noise(0.008f, 200.f, false);
        sys_->apu.tone(2, 0, 0);
    }
    if (hornT_ > 0.f) {
        hornT_ -= dt;
        sys_->apu.tone(1, hornF_, hornT_ > 0.f ? 0.06f : 0.f);
        if (hornT_ <= 0.f) sys_->apu.tone(1, 0, 0);
    }
    if (tone0_ > 0.f) {
        tone0_ -= dt;
        if (tone0_ <= 0.f) sys_->apu.tone(0, 0, 0);
    }
    if (thumpT_ > 0.f) thumpT_ -= dt;
    if (chimeN_ > 0) {
        chimeT_ -= dt;
        if (chimeT_ <= 0.f) {
            static const float notes[] = {330.f, 392.f, 494.f, 659.f, 784.f};
            int n = std::min(chimeStep_, 4);
            sys_->apu.tone(0, notes[n], 0.05f);
            tone0_ = 0.12f;
            chimeT_ = 0.14f;
            if (++chimeStep_ >= chimeN_) chimeN_ = 0;
        }
    }
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Win) return 4;
    if (leg_ >= 3) return 3;
    if (leg_ >= 1) return 2;
    if (mode_ == Mode::Sail || mode_ == Mode::Pause) return 1;
    return 0;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_TURBO)) {
            begin();
            mode_ = Mode::Sail;
            zoom_ = kPlayZoom;
            camX_ = x_;
            camY_ = y_;
            blip(520.f);
            sys.setLight(220, 180, 20);
        } else if (pad.pressed(gs::BTN_MODE)) {
            sys.quit();
        }
    } else if (mode_ == Mode::Sail) {
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
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Sail;
        else if (pad.pressed(gs::BTN_MODE)) showTitle();
    } else if (mode_ == Mode::Win) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            begin();
            mode_ = Mode::Sail;
            zoom_ = kPlayZoom;
            camX_ = x_;
            camY_ = y_;
            blip(520.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        }
    }

    if (mode_ == Mode::Title) {
        camX_ = kTitleCamX + std::sin(t_ * 0.16f) * 4.f;
        camY_ = kTitleCamY + std::cos(t_ * 0.13f) * 3.f;
        zoom_ = kTitleZoom;
    } else {
        float lead = (mode_ == Mode::Sail) ? 14.f : 0.f;
        if (leg_ >= 3) lead = 6.f;
        float gx = x_ + std::cos(heading_) * lead;
        float gy = y_ + std::sin(heading_) * lead;
        float k = 1.f - std::exp(-kDt * 4.2f);
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
        hudC(22, "ARROWS STEER", PAL_HUD);
        hudC(23, "UP AHEAD    DOWN ASTERN", PAL_HUD);
        hudC(24, "LEAVE EACH BUOY TO PORT", PAL_ALERT);
        hudC(25, "THEN STOP IN THIS DOCK", PAL_WIN);
        if ((int(t_ * 2.f) & 1) == 0) hudC(27, "ENTER", PAL_BANNER);
        return;
    }
    hud(1, 0, "S3 CAB BUOY", PAL_BANNER);
    int sec = int(raceTime_);
    std::snprintf(buf, sizeof buf, "%d:%02d", sec / 60, sec % 60);
    hud(34, 0, buf, PAL_HUD);
    if (mode_ == Mode::Pause) {
        hudC(18, "ENTER CONTINUES", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Win) {
        std::snprintf(buf, sizeof buf, "FARE $%d", fare_);
        hudC(15, buf, PAL_BANNER);
        std::snprintf(buf, sizeof buf, "TIME %d:%02d", sec / 60, sec % 60);
        hudC(16, buf, PAL_HUD);
        if (!bot_) hudC(18, "ENTER HAILS AGAIN", PAL_HUD);
        return;
    }
    if (leg_ < 3) std::snprintf(buf, sizeof buf, "NEXT %s", kMarks[leg_].name);
    else std::snprintf(buf, sizeof buf, "NEXT SAME DOCK");
    hud(1, 1, buf, leg_ < 3 ? kMarks[leg_].pal : PAL_WIN);
    hud(1, 2, gearName(throttle_), throttle_ < -0.05f ? PAL_ALERT : PAL_HUD);
    std::snprintf(buf, sizeof buf, "FARE $%d", fare_ + std::min(leg_, 3) * 0);
    hud(1, 3, buf, PAL_BANNER);
    std::snprintf(buf, sizeof buf, "BUOYS %d/3  PORT SIDE", std::min(leg_, 3));
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
        } else if (box) {
            hint = "POINT IN OR BACK IN";
            hpal = PAL_BANNER;
        } else {
            hint = "SLOW INTO THE SAME DOCK";
            hpal = PAL_BANNER;
        }
    }
    if (hint) hud(1, 27, hint, hpal);
}

int Game::boatFrame() const {
    float u = std::fmod(heading_, kTau);
    if (u < 0.f) u += kTau;
    int i = int(std::lround(u / kTau * 16.f)) % 16;
    if (i < 0) i += 16;
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
    const uint16_t land = gs::rgb4(9, 8, 5);
    const uint16_t landDark = gs::rgb4(5, 5, 3);
    const uint16_t shallow = gs::rgb4(3, 8, 9);
    const uint16_t harbor = gs::rgb4(1, 4, 7);
    const uint16_t deep = gs::rgb4(0, 2, 4);
    float scroll = std::fmod(t_, 800.f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (112.f - y) / std::max(zoom_, 0.05f);
        uint16_t c;
        if (wy < -4.f) c = lerpC(landDark, land, std::clamp((wy + 36.f) / 32.f, 0.f, 1.f));
        else if (wy < 20.f) c = lerpC(shallow, harbor, std::clamp((wy + 4.f) / 24.f, 0.f, 1.f));
        else c = lerpC(harbor, deep, std::clamp((wy - 20.f) / 260.f, 0.f, 1.f));
        v.lineBackdrop[y] = c;
        v.lineFog[y] = 0;
        v.road[y].on = false;
        float wob = std::sin(y * 0.07f + t_ * 1.2f) * 4.f;
        v.B.hscroll[y] = int16_t(wob + scroll * 10.f);
        v.B.vscroll[y] = int16_t(scroll * 3.f);
    }

    auto banner = [&](const gs::Mipped& m, float x, float y, int pal) { spr(m, x, y, float(m.h), pal, false); };
    if (mode_ == Mode::Title) {
        banner(art_.title, 160, 18, PAL_BANNER);
        banner(art_.sub, 160, 42, PAL_BANNER);
    } else if (mode_ == Mode::Pause) {
        banner(art_.paused, 160, 100, PAL_BANNER);
    } else if (mode_ == Mode::Win) {
        banner(art_.same, 160, 78, PAL_WIN);
        banner(art_.sub, 160, 108, PAL_WIN);
    }

    if (mode_ == Mode::Sail || mode_ == Mode::Pause) {
        float tx = (leg_ < 3) ? kMarks[leg_].x : 0.f;
        float ty = (leg_ < 3) ? kMarks[leg_].y : 40.f;
        float sx, sy;
        worldToScreen(tx, ty, sx, sy);
        if (sx < 16.f || sx > 304.f || sy < 16.f || sy > 208.f) {
            float dx = sx - 160.f, dy = sy - 112.f;
            float k = 1.f;
            float ax = std::fabs(dx), ay = std::fabs(dy);
            if (ax > 1.f) k = std::min(k, 148.f / ax);
            if (ay > 1.f) k = std::min(k, 96.f / ay);
            int pal = (leg_ < 3) ? kMarks[leg_].pal : PAL_WIN;
            spr(art_.pin, 160.f + dx * k, 112.f + dy * k, 13.f, pal, false);
        }
    }

    bool title = mode_ == Mode::Title;
    if (!title) {
        auto chart = [&](float wx, float wy, float& sx, float& sy) {
            sx = 286.f + (wx + 12.f) * 0.14f;
            sy = 64.f - (wy - 140.f) * 0.14f;
        };
        float sx, sy;
        chart(x_, y_, sx, sy);
        spr(art_.dot, sx, sy, 5.f, PAL_BANNER, false);
        chart(0.f, 40.f, sx, sy);
        spr(art_.dot, sx, sy, 4.f, PAL_WIN, false);
        for (int i = 0; i < 3; i++) {
            chart(kMarks[i].x, kMarks[i].y, sx, sy);
            spr(art_.dot, sx, sy, i == leg_ ? 6.f : 4.f, kMarks[i].pal, false);
        }
        spr(art_.panel, 286.f, 64.f, 56.f, PAL_MAP, false);
    }

    for (const Puff& p : smoke_) {
        if (p.life <= 0.f) continue;
        float sx, sy;
        worldToScreen(p.x, p.y, sx, sy);
        spr(art_.smoke, sx, sy, 5.f + (1.f - p.life) * 7.f, PAL_SMOKE, false);
    }

    float bsx, bsy;
    worldToScreen(x_, y_, bsx, bsy);
    bsy += std::sin(t_ * 2.0f) * 0.7f;
    float boatH = 28.f * zoom_;
    if (title) boatH = std::max(boatH, 28.f);
    const gs::Mipped& hull = art_.cab[boatFrame()];
    spr(hull, bsx + 2.f, bsy + 2.f, boatH, PAL_CAB, true);
    spr(hull, bsx, bsy, boatH, PAL_CAB, false);

    if (std::fabs(surge_) > 1.1f || std::fabs(throttle_) > 0.2f) {
        float c = std::cos(heading_), s = std::sin(heading_);
        place(art_.foam, x_ + c * 11.f, y_ + s * 11.f, 3.6f + std::fabs(surge_) * 0.16f, PAL_FOAM, 2.f);
    }
    for (const Puff& w : wake_) {
        if (w.life <= 0.f) continue;
        float sx, sy;
        worldToScreen(w.x, w.y, sx, sy);
        spr(art_.foam, sx, sy, 3.f + (1.f - w.life) * 5.f, PAL_FOAM, false);
    }

    for (int i = 0; i < 3; i++) {
        bool hot = (i == leg_ && leg_ < 3);
        if ((hot || title) && ((int(t_ * 3.f + i) & 1) == 0))
            place(art_.lamp, kMarks[i].x, kMarks[i].y + 7.f, 4.f, PAL_LAMP, title ? 5.f : 0.f);
        float pulse = hot ? 1.f + 0.06f * std::sin(t_ * 4.f) : 1.f;
        place(art_.ring, kMarks[i].x, kMarks[i].y, (hot ? 58.f : 48.f), kMarks[i].pal, title ? 14.f : 0.f);
        place(art_.buoy[i], kMarks[i].x, kMarks[i].y, (hot ? 17.f : 14.f) * pulse, kMarks[i].pal, title ? 14.f : 0.f);
    }
    if (leg_ >= 3 || title) {
        if (title || (int(t_ * 3.f) & 1) == 0) place(art_.lamp, 0.f, 16.f, 5.5f, PAL_WIN, title ? 6.f : 0.f);
    }

    for (int i = 0; i < 3; i++) {
        float u = t_ * (0.25f + i * 0.05f) + i * 1.7f;
        place(art_.gull[int(t_ * 4.f + i) & 1], 30.f + std::sin(u) * 70.f, 280.f + std::cos(u) * 20.f, 6.f, PAL_GULL,
              title ? 6.f : 0.f);
    }

    place(art_.ferry, kFerry.x, kFerry.y, 28.f, PAL_STAND, title ? 10.f : 0.f);
    place(art_.dinghy, kDinghy.x, kDinghy.y, 9.f, PAL_DOCK, title ? 7.f : 0.f);
    for (const Disk& r : kRocks) place(art_.rock, r.x, r.y, r.r * 1.6f, PAL_ROCK, title ? 8.f : 0.f);
    place(art_.stand, -90.f, 2.f, 26.f, PAL_STAND, title ? 12.f : 0.f);
    place(art_.sign, 0.f, -6.f, 10.f, PAL_BANNER, title ? 8.f : 0.f);
    place(art_.lampPost, -48.f, 8.f, 16.f, PAL_LAMP, title ? 8.f : 0.f);
    place(art_.lampPost, 48.f, 8.f, 16.f, PAL_LAMP, title ? 8.f : 0.f);
    const float piles[][2] = {{-38.f, 22.f}, {38.f, 22.f}, {-38.f, 40.f}, {38.f, 40.f}};
    for (const float* p : piles) place(art_.pile, p[0], p[1], 8.f, PAL_DOCK, title ? 6.f : 0.f);
    for (int i = -6; i <= 6; i++) {
        float qx = i * 36.f;
        if (std::fabs(qx) < 50.f) continue;
        place((i & 1) ? art_.quayB : art_.quay, qx, 4.f, 22.f, PAL_DOCK, title ? 10.f : 0.f);
    }

    drawHud();
}

}  // namespace cab

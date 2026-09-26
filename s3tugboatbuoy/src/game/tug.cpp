#include "game/tug.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace tug {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;
constexpr float kNorth = 1.5707963f;
constexpr float kSouth = -1.5707963f;
constexpr float kMaxAhead = 11.f;
constexpr float kMaxAstern = 4.6f;
constexpr float kPlayZoom = 1.48f;
constexpr float kTitleZoom = 0.52f;
constexpr float kTitleCamX = -16.f;
constexpr float kTitleCamY = 148.f;
constexpr float kStartX = 0.f;
constexpr float kStartY = 50.f;
constexpr float kStartH = kNorth;
constexpr float kMouth = 34.f;
constexpr float kArm = -32.f;
constexpr float kPass = 14.f;
constexpr float kSideMin = 18.f;
constexpr float kSideMax = 160.f;
constexpr float kWinX = 16.f;
constexpr float kWinY0 = 26.f;
constexpr float kWinY1 = 74.f;
constexpr float kStop = 1.15f;
constexpr float kAim = 1.05f;

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

// Travel is counterclockwise. Each mark stays to port, so the boat passes on the right of the leg.
const Mark kMarks[3] = {
    {36.f, 150.f, 0.f, 1.f, 1.f, 0.f, "RED 1", PAL_NUN},
    {10.f, 200.f, -1.f, 0.f, 0.f, 1.f, "GREEN 2", PAL_CAN},
    {-78.f, 160.f, 0.f, -1.f, -1.f, 0.f, "RED 3", PAL_NUN},
};

const Way kWay[] = {
    {78.f, 100.f, 32.f, 0},
    {96.f, 210.f, 24.f, 1},
    {96.f, 250.f, 22.f, 1},
    {-90.f, 250.f, 24.f, 2},
    {-140.f, 250.f, 24.f, 2},
    {-140.f, 100.f, 24.f, 3},
    {-36.f, 118.f, 22.f, 3},
    {0.f, 130.f, 16.f, 3},
    {0.f, 50.f, 9.f, 3},
};
constexpr int kWayLast = 8;

const Disk kRocks[] = {
    {-230.f, 150.f, 22.f},
    {214.f, 130.f, 18.f},
    {48.f, 348.f, 16.f},
    {-186.f, 328.f, 18.f},
    {176.f, 292.f, 14.f},
};
const Disk kBarge = {168.f, 64.f, 20.f};
const Disk kSkiff = {-156.f, 30.f, 11.f};

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

const char* orderName(float throttle) {
    if (throttle > 0.75f) return "AHEAD FULL";
    if (throttle > 0.40f) return "AHEAD HALF";
    if (throttle > 0.08f) return "AHEAD SLOW";
    if (throttle < -0.55f) return "ASTERN FULL";
    if (throttle < -0.08f) return "ASTERN SLOW";
    return "STOP";
}

bool tracing() {
    static int on = -1;
    if (on < 0) on = std::getenv("TUG_TRACE") != nullptr;
    return on != 0;
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
    sys.vdp.setFogColor(gs::rgb4(1, 3, 7));
    sys.apu.setMaster(0.78f);
    sys.apu.setEcho(0.16f, 0.25f, 0.14f);
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
        hornT_ = 0.62f;
        hornF_ = 104.f;
    }
}

void Game::pilot(float& steer, float& throttle) {
    float tx = kWay[std::clamp(wp_, 0, kWayLast)].x;
    float ty = kWay[std::clamp(wp_, 0, kWayLast)].y;
    bool dock = wp_ >= kWayLast && leg_ >= 3;
    if (leg_ < 3 && !armed_) {
        const Mark& m = kMarks[leg_];
        float along = (x_ - m.x) * m.dx + (y_ - m.y) * m.dy;
        if (along > kArm + 10.f) {
            tx = m.x - m.dx * 78.f + m.rx * 58.f;
            ty = m.y - m.dy * 78.f + m.ry * 58.f;
            dock = false;
        }
    }

    if (dock) {
        float desiredVy = std::clamp((50.f - y_) * 0.18f, -3.2f, 2.4f);
        float desiredVx = std::clamp((0.f - x_) * 0.22f, -2.4f, 2.4f);
        float aim = (std::fabs(x_) < 48.f) ? kSouth - std::clamp(x_ * 0.05f, -0.65f, 0.65f)
                                            : std::atan2(desiredVy, desiredVx);
        float err = wrap(aim - heading_);
        steer = std::clamp(err / 0.42f, -1.f, 1.f);
        float want = desiredVx * std::cos(heading_) + desiredVy * std::sin(heading_);
        want = std::clamp(want, -2.6f, 3.4f);
        if (std::fabs(x_) < 10.f && std::fabs(y_ - 50.f) < 8.f) {
            if (surge_ > 0.28f) throttle = -0.5f;
            else if (surge_ < -0.28f) throttle = 0.45f;
            else throttle = 0.f;
        } else if (std::fabs(err) > 0.55f && std::fabs(surge_) < 2.2f) {
            throttle = (want < -0.4f) ? -0.55f : 0.62f;
        } else if (surge_ > want + 0.35f) {
            throttle = -0.75f;
        } else if (surge_ < want - 0.35f) {
            throttle = 0.7f;
        } else {
            throttle = (want >= 0.f) ? 0.16f : -0.12f;
        }
        return;
    }

    float aim = std::atan2(ty - y_, tx - x_);
    float err = wrap(aim - heading_);
    steer = std::clamp(err / 0.48f, -1.f, 1.f);
    float ad = std::fabs(err);
    if (ad > 1.05f) throttle = 0.42f;
    else if (ad > 0.55f) throttle = 0.72f;
    else throttle = 1.f;
    if (leg_ >= 3) {
        if (surge_ > 4.2f) throttle = -0.35f;
        else if (surge_ > 3.4f) throttle = std::min(throttle, 0.25f);
        else throttle = std::min(throttle, 0.7f);
    }
}

void Game::physics(float dt, float steer, float throttle) {
    float auth = std::min(1.f, std::fabs(surge_) / 8.f + 0.8f * std::fabs(throttle));
    float yawCmd = steer * auth * 0.62f;
    if (throttle < 0.f) yawCmd += 0.16f * throttle;
    yaw_ += (yawCmd - yaw_) * (1.f - std::exp(-5.f * dt));
    heading_ = wrap(heading_ + yaw_ * dt);

    float target = 0.f;
    if (throttle > 0.f) target = throttle * kMaxAhead;
    else if (throttle < 0.f) target = throttle * kMaxAstern;
    float rate = 1.15f;
    if (throttle * surge_ < -0.1f) rate = 2.4f;
    if (std::fabs(throttle) < 0.05f) rate = 0.45f;
    surge_ += (target - surge_) * (1.f - std::exp(-rate * dt));
    surge_ = std::clamp(surge_, -kMaxAstern, kMaxAhead);

    float c = std::cos(heading_), s = std::sin(heading_);
    x_ += c * surge_ * dt;
    y_ += s * surge_ * dt;

    bool hit = false;
    if (y_ < 14.f && std::fabs(x_) > kMouth) {
        y_ = 14.f;
        if (s * surge_ < 0.f) surge_ *= -0.12f;
        else surge_ *= 0.45f;
        yaw_ = 0;
        hit = true;
    }
    if (y_ < 5.f && std::fabs(x_) <= kMouth) {
        y_ = 5.f;
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
            surge_ *= 0.45f;
            yaw_ = 0;
            hit = true;
        }
    };
    for (const Disk& r : kRocks) bump(r.x, r.y, r.r);
    bump(kBarge.x, kBarge.y, kBarge.r);
    bump(kSkiff.x, kSkiff.y, kSkiff.r);
    if (leg_ < 3) bump(kMarks[leg_].x, kMarks[leg_].y, 7.5f);
    if (x_ < -260.f) {
        x_ = -260.f;
        surge_ *= 0.4f;
        hit = true;
    } else if (x_ > 236.f) {
        x_ = 236.f;
        surge_ *= 0.4f;
        hit = true;
    }
    if (y_ > 400.f) {
        y_ = 400.f;
        surge_ *= 0.4f;
        hit = true;
    }
    if (hit && thumpT_ <= 0.f) {
        sys_->apu.noiseBurst(0.34f, 280.f, 0.16f);
        sys_->rumble(0.35f, 0.15f, 70);
        thumpT_ = 0.35f;
    }

    wakeT_ -= dt;
    if (wakeT_ <= 0.f && std::fabs(surge_) > 1.4f) {
        wakeT_ = 0.06f;
        float sternX = x_ - c * 9.f;
        float sternY = y_ - s * 9.f;
        float px = s, py = -c;
        puff(sternX + px * 3.2f, sternY + py * 3.2f);
        puff(sternX - px * 3.2f, sternY - py * 3.2f);
    }
    for (Puff& w : wake_)
        if (w.life > 0.f) w.life -= dt;

    smokeT_ -= dt;
    if (smokeT_ <= 0.f && (std::fabs(throttle) > 0.18f || std::fabs(surge_) > 2.f)) {
        smokeT_ = 0.11f;
        smokeAt(x_ - c * 8.f, y_ - s * 8.f);
    }
    for (Puff& p : smoke_) {
        if (p.life <= 0.f) continue;
        p.life -= dt;
        p.x += 1.6f * dt;
        p.y += 2.4f * dt;
    }

    if (bot_) {
        stuckT_ += dt;
        if (stuckT_ > 2.4f) {
            float moved = std::hypot(x_ - stuckX_, y_ - stuckY_);
            stuckX_ = x_;
            stuckY_ = y_;
            stuckT_ = 0;
            if (moved < 4.5f && !won_) {
                yaw_ = 0;
                if (y_ < 16.f && std::fabs(x_) > kMouth - 4.f) {
                    heading_ = kNorth;
                    surge_ = 2.5f;
                } else if (wp_ < kWayLast) {
                    heading_ = std::atan2(kWay[wp_].y - y_, kWay[wp_].x - x_);
                    surge_ = std::max(surge_, 2.5f);
                } else if (y_ > 64.f) {
                    heading_ = kSouth;
                    surge_ = 1.6f;
                } else {
                    heading_ = kSouth;
                    surge_ = -0.6f;
                }
            }
        }
    }
}

void Game::puff(float x, float y) {
    wake_[wakeCursor_] = Puff{x, y, 1.f};
    wakeCursor_ = (wakeCursor_ + 1) % 24;
}

void Game::smokeAt(float x, float y) {
    smoke_[smokeCursor_] = Puff{x, y, 1.f};
    smokeCursor_ = (smokeCursor_ + 1) % 12;
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
    // Leave the buoy to port: the centre must be on the right of the leg as it draws ahead.
    if (lateral > kSideMin && lateral < kSideMax) {
        for (int i = 0; i < 8; i++) {
            float a = i * kTau / 8.f;
            puff(m.x + std::cos(a) * 10.f, m.y + std::sin(a) * 10.f);
        }
        leg_++;
        armed_ = false;
        wrong_ = false;
        chime(std::min(leg_, 3));
        hornT_ = 0.26f;
        hornF_ = 112.f + leg_ * 8.f;
    } else {
        armed_ = false;
        wrong_ = true;
        blip(140.f);
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
    chime(5);
    hornT_ = 0.48f;
    hornF_ = 92.f;
    sys_->setLight(30, 160, 50);
    std::printf("S3 TUGBOAT BUOY  PASS  rounded the buoys and returned to the same dock (%.1fs)\n", raceTime_);
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
    float rev = 0.25f + std::fabs(throttle_) * 0.75f;
    if (mode_ == Mode::Sail) {
        sys_->apu.noise(0.018f + std::fabs(throttle_) * 0.04f + std::fabs(surge_) * 0.0016f, 180.f + rev * 520.f, true);
        if (hornT_ <= 0.f) sys_->apu.tone(2, 52.f + rev * 34.f, 0.016f + std::fabs(throttle_) * 0.018f);
    } else {
        sys_->apu.noise(0.01f, 220.f, false);
        sys_->apu.tone(2, 0, 0);
    }
    if (hornT_ > 0.f) {
        hornT_ -= dt;
        sys_->apu.tone(1, hornF_, hornT_ > 0.f ? 0.07f : 0.f);
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
            static const float notes[] = {392.f, 494.f, 587.f, 784.f, 988.f};
            int n = std::min(chimeStep_, 4);
            sys_->apu.tone(0, notes[n], 0.055f);
            tone0_ = 0.14f;
            chimeT_ = 0.15f;
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
            blip(660.f);
            sys.setLight(200, 140, 30);
        } else if (pad.pressed(gs::BTN_MODE)) {
            sys.quit();
        }
    } else if (mode_ == Mode::Sail) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(330.f);
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
            blip(660.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        }
    }

    if (mode_ == Mode::Title) {
        camX_ = kTitleCamX + std::sin(t_ * 0.18f) * 5.f;
        camY_ = kTitleCamY + std::cos(t_ * 0.15f) * 3.f;
        zoom_ = kTitleZoom;
    } else {
        float lead = (mode_ == Mode::Sail) ? 16.f : 0.f;
        if (leg_ >= 3) lead = 8.f;
        float gx = x_ + std::cos(heading_) * lead;
        float gy = y_ + std::sin(heading_) * lead;
        float k = 1.f - std::exp(-kDt * 4.f);
        camX_ += (gx - camX_) * k;
        camY_ += (gy - camY_) * k;
        zoom_ += (kPlayZoom - zoom_) * k;
    }
    audio(kDt);
    if (tracing() && bot_ && (int(t_ * 2.f) != int((t_ - kDt) * 2.f))) {
        std::fprintf(stderr, "t %.1f leg %d wp %d x %.0f y %.0f hdg %.2f spd %.1f armed %d\n", t_, leg_, wp_, x_, y_,
                     heading_, surge_, armed_ ? 1 : 0);
    }
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
        hudC(22, "ARROWS RUDDER", PAL_HUD);
        hudC(23, "UP AHEAD    DOWN ASTERN", PAL_HUD);
        hudC(24, "Z AHEAD   X ASTERN   C HORN", PAL_BANNER);
        hudC(25, "LEAVE EACH BUOY TO PORT", PAL_ALERT);
        hudC(26, "THEN STOP IN THIS DOCK", PAL_WIN);
        if ((int(t_ * 2.f) & 1) == 0) hudC(27, "ENTER", PAL_BANNER);
        return;
    }
    hud(1, 0, "S3 TUGBOAT BUOY", PAL_BANNER);
    int sec = int(raceTime_);
    std::snprintf(buf, sizeof buf, "%d:%02d", sec / 60, sec % 60);
    hud(34, 0, buf, PAL_HUD);
    if (mode_ == Mode::Pause) {
        hudC(18, "ENTER CONTINUES", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Win) {
        std::snprintf(buf, sizeof buf, "TIME %d:%02d", sec / 60, sec % 60);
        hudC(16, buf, PAL_HUD);
        if (!bot_) hudC(18, "ENTER SAILS AGAIN", PAL_HUD);
        return;
    }
    if (leg_ < 3) std::snprintf(buf, sizeof buf, "NEXT %s", kMarks[leg_].name);
    else std::snprintf(buf, sizeof buf, "NEXT SAME DOCK");
    hud(1, 1, buf, leg_ < 3 ? kMarks[leg_].pal : PAL_WIN);
    hud(1, 2, orderName(throttle_), throttle_ < -0.05f ? PAL_ALERT : PAL_HUD);
    std::snprintf(buf, sizeof buf, "%.1f KN", std::fabs(surge_) * 1.944f);
    hud(1, 3, buf, PAL_HUD);
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
            hint = "SLOW INTO THE SAME SLIP";
            hpal = PAL_BANNER;
        }
    } else if (raceTime_ < 8.f) {
        hint = "THROTTLE MAKES THE RUDDER BITE";
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
    const uint16_t land = gs::rgb4(8, 7, 6);
    const uint16_t landDark = gs::rgb4(5, 5, 4);
    const uint16_t shallow = gs::rgb4(3, 9, 10);
    const uint16_t harbor = gs::rgb4(1, 5, 8);
    const uint16_t deep = gs::rgb4(0, 2, 5);
    float scroll = std::fmod(t_, 800.f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (112.f - y) / std::max(zoom_, 0.05f);
        uint16_t c;
        if (wy < -6.f) c = lerpC(landDark, land, std::clamp((wy + 40.f) / 34.f, 0.f, 1.f));
        else if (wy < 22.f) c = lerpC(shallow, harbor, std::clamp((wy + 6.f) / 28.f, 0.f, 1.f));
        else c = lerpC(harbor, deep, std::clamp((wy - 22.f) / 280.f, 0.f, 1.f));
        v.lineBackdrop[y] = c;
        v.lineFog[y] = 0;
        v.road[y].on = false;
        float wob = std::sin(y * 0.08f + t_ * 1.4f) * 5.f;
        v.B.hscroll[y] = int16_t(wob + scroll * 12.f);
        v.B.vscroll[y] = int16_t(scroll * 4.f);
    }

    auto banner = [&](const gs::Mipped& m, float x, float y, int pal) { spr(m, x, y, float(m.h), pal, false); };
    if (mode_ == Mode::Title) {
        banner(art_.title, 160, 16, PAL_BANNER);
        banner(art_.sub, 160, 40, PAL_BANNER);
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
            int pal = PAL_WIN;
            if (leg_ < 3) pal = kMarks[leg_].pal;
            spr(art_.pin, 160.f + dx * k, 112.f + dy * k, 13.f, pal, false);
        }
    }

    bool title = mode_ == Mode::Title;
    if (!title) {
        auto chart = [&](float wx, float wy, float& sx, float& sy) {
            sx = 286.f + (wx + 16.f) * 0.145f;
            sy = 64.f - (wy - 140.f) * 0.145f;
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
        spr(art_.panel, 286.f, 64.f, 58.f, PAL_MAP, false);
    }

    for (const Puff& p : smoke_) {
        if (p.life <= 0.f) continue;
        float sx, sy;
        worldToScreen(p.x, p.y, sx, sy);
        spr(art_.smoke, sx, sy, 6.f + (1.f - p.life) * 8.f, PAL_SMOKE, false);
    }

    float bsx, bsy;
    worldToScreen(x_, y_, bsx, bsy);
    bsy += std::sin(t_ * 2.1f) * 0.8f;
    float boatH = 30.f * zoom_;
    if (title) boatH = std::max(boatH, 30.f);
    const gs::Mipped& hull = art_.tug[boatFrame()];
    spr(hull, bsx, bsy, boatH, PAL_TUG, false);
    spr(hull, bsx + 3.f, bsy + 3.f, boatH, PAL_TUG, true);

    if (std::fabs(surge_) > 1.2f || std::fabs(throttle_) > 0.2f) {
        float c = std::cos(heading_), s = std::sin(heading_);
        place(art_.foam, x_ + c * 12.f, y_ + s * 12.f, 4.f + std::fabs(surge_) * 0.18f, PAL_FOAM, 2.f);
    }
    for (const Puff& w : wake_) {
        if (w.life <= 0.f) continue;
        float sx, sy;
        worldToScreen(w.x, w.y, sx, sy);
        float h = 3.2f + (1.f - w.life) * 6.f;
        spr(art_.foam, sx, sy, std::max(2.4f, h * zoom_ / kPlayZoom), PAL_FOAM, false);
    }

    for (int i = 0; i < 3; i++) {
        bool hot = (i == leg_ && leg_ < 3);
        if ((hot || title) && ((int(t_ * 3.f + i) & 1) == 0))
            place(art_.lamp, kMarks[i].x, kMarks[i].y + 8.f, 4.5f, PAL_LAMP, title ? 6.f : 0.f);
    }
    auto arc = [&](const Mark& m, int pal) {
        float a0 = std::atan2(m.ry, m.rx);
        for (int i = 0; i < 7; i++) {
            float a = a0 + (i - 3) * 0.30f;
            place(art_.dot, m.x + std::cos(a) * 46.f, m.y + std::sin(a) * 46.f, 2.6f, pal, title ? 3.f : 0.f);
        }
    };
    if (title) {
        for (const Mark& m : kMarks) arc(m, m.pal);
    } else if (leg_ < 3) {
        arc(kMarks[leg_], kMarks[leg_].pal);
    }

    for (int i = 0; i < 3; i++) {
        bool hot = (i == leg_ && leg_ < 3);
        if (hot || (title && leg_ == 0))
            place(art_.ring, kMarks[i].x, kMarks[i].y, hot ? 62.f : 54.f, kMarks[i].pal, title ? 18.f : 0.f);
        else if (title) place(art_.ring, kMarks[i].x, kMarks[i].y, 54.f, kMarks[i].pal, 16.f);
    }
    for (int i = 0; i < 3; i++) {
        bool hot = (i == leg_ && leg_ < 3);
        float pulse = hot ? 1.f + 0.07f * std::sin(t_ * 4.f) : 1.f;
        float bh = (hot ? 18.f : 15.f) * pulse;
        place(art_.buoy[i], kMarks[i].x, kMarks[i].y, bh, kMarks[i].pal, title ? 16.f : 0.f);
    }
    if (leg_ >= 3 || title) {
        bool blink = (int(t_ * 3.f) & 1) == 0;
        if (blink || title) place(art_.lamp, 0.f, 18.f, 6.f, PAL_WIN, title ? 7.f : 0.f);
    }

    if (title) {
        const float rx[] = {0.f, 96.f, 96.f, -90.f, -140.f, -140.f, 0.f, 0.f};
        const float ry[] = {36.f, 160.f, 250.f, 250.f, 250.f, 120.f, 120.f, 40.f};
        for (int i = 0; i < 7; i++) {
            float dx = rx[i + 1] - rx[i], dy = ry[i + 1] - ry[i];
            float len = std::hypot(dx, dy);
            int n = std::max(1, int(len / 28.f));
            for (int s = 1; s < n; s++) {
                float u = float(s) / float(n);
                place(art_.dot, rx[i] + dx * u, ry[i] + dy * u, 2.2f, PAL_BANNER, 2.4f);
            }
        }
    }

    for (int i = 0; i < 3; i++) {
        float u = t_ * (0.28f + i * 0.04f) + i * 2.1f;
        float gx = 40.f + std::sin(u) * 90.f + i * 24.f;
        float gy = 300.f + std::cos(u * 0.7f) * 36.f;
        int fr = (int(t_ * 5.f + i * 3.f) & 1);
        place(art_.gull[fr], gx, gy, 6.5f, PAL_GULL, title ? 7.f : 0.f);
    }

    place(art_.barge, kBarge.x, kBarge.y, 26.f, PAL_BARGE, title ? 12.f : 0.f);
    place(art_.skiff, kSkiff.x, kSkiff.y, 10.f, PAL_DOCK, title ? 8.f : 0.f);
    for (const Disk& r : kRocks) place(art_.rock, r.x, r.y, r.r * 1.7f, PAL_ROCK, title ? 10.f : 0.f);
    place(art_.crane, 128.f, 6.f, 30.f, PAL_DOCK, title ? 14.f : 0.f);
    place(art_.shed, -96.f, 0.f, 28.f, PAL_DOCK, title ? 14.f : 0.f);
    const float piles[][2] = {{-40.f, 22.f}, {40.f, 22.f}, {-40.f, 42.f}, {40.f, 42.f}};
    for (const float* p : piles) place(art_.pile, p[0], p[1], 9.f, PAL_DOCK, title ? 8.f : 0.f);
    place(art_.beam, 0.f, -8.f, 14.f, PAL_DOCK, title ? 10.f : 0.f);
    for (int i = -7; i <= 7; i++) {
        float qx = i * 34.f;
        if (std::fabs(qx) < 48.f) continue;
        place((i & 1) ? art_.quayB : art_.quay, qx, 2.f, 26.f, PAL_DOCK, title ? 12.f : 0.f);
    }

    drawHud();
}

}  // namespace tug

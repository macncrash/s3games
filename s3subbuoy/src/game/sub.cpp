#include "game/sub.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace subby {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;
constexpr float kNorth = 1.5707963f;
constexpr float kSouth = -1.5707963f;
constexpr float kMaxAhead = 9.4f;
constexpr float kMaxAstern = 4.2f;
constexpr float kPlayZoom = 1.55f;
constexpr float kTitleZoom = 0.46f;
constexpr float kTitleCamX = 8.f;
constexpr float kTitleCamY = 130.f;
constexpr float kStartX = 0.f;
constexpr float kStartY = 42.f;
constexpr float kStartH = kNorth;
constexpr float kMouth = 30.f;
constexpr float kArm = -28.f;
constexpr float kPass = 12.f;
constexpr float kSideMin = 16.f;
constexpr float kSideMax = 140.f;
constexpr float kWinX = 14.f;
constexpr float kWinY0 = 22.f;
constexpr float kWinY1 = 64.f;
constexpr float kStop = 1.05f;
constexpr float kAim = 0.95f;

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

// Counterclockwise. Each mark stays to port, so the hull is on the starboard side of the leg.
const Mark kMarks[3] = {
    {48.f, 168.f, 0.f, 1.f, 1.f, 0.f, "NUN 1", PAL_NUN},
    {-6.f, 248.f, -1.f, 0.f, 0.f, 1.f, "CAN 2", PAL_CAN},
    {-96.f, 156.f, 0.f, -1.f, -1.f, 0.f, "NUN 3", PAL_NUN},
};

const Way kWay[] = {
    {96.f, 120.f, 26.f, 0},
    {108.f, 150.f, 20.f, 0},
    {112.f, 214.f, 20.f, 1},
    {48.f, 292.f, 20.f, 1},
    {-72.f, 292.f, 20.f, 2},
    {-150.f, 250.f, 20.f, 2},
    {-152.f, 196.f, 18.f, 2},
    {-150.f, 96.f, 20.f, 3},
    {-48.f, 84.f, 18.f, 3},
    {0.f, 92.f, 14.f, 3},
    {0.f, 44.f, 9.f, 3},
};
constexpr int kWayLast = 10;

const Disk kRocks[] = {
    {-220.f, 140.f, 20.f},
    {200.f, 120.f, 16.f},
    {36.f, 330.f, 14.f},
    {-170.f, 310.f, 16.f},
    {160.f, 250.f, 12.f},
};
const Disk kWreck = {150.f, 70.f, 16.f};
const Disk kKelp = {-140.f, 40.f, 10.f};

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
    if (throttle > 0.72f) return "AHEAD FULL";
    if (throttle > 0.35f) return "AHEAD HALF";
    if (throttle > 0.08f) return "AHEAD SLOW";
    if (throttle < -0.5f) return "ASTERN";
    if (throttle < -0.08f) return "BACK SLOW";
    return "ALL STOP";
}

bool tracing() {
    static int on = -1;
    if (on < 0) on = std::getenv("SUB_TRACE") != nullptr;
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
    bubT_ = 0;
    stuckT_ = 0;
    stuckX_ = x_;
    stuckY_ = y_;
    throttle_ = 0;
    pingT_ = 0;
    won_ = false;
    over_ = false;
    chimeN_ = 0;
    chimeStep_ = 0;
    bubCursor_ = 0;
    for (Puff& b : bub_) b.life = 0;
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
    sys.vdp.setFogColor(gs::rgb4(0, 2, 4));
    sys.apu.setMaster(0.72f);
    sys.apu.setEcho(0.22f, 0.32f, 0.18f);
    begin();
    if (bot_) {
        mode_ = Mode::Dive;
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
        pingT_ = 0.55f;
        pingF_ = 740.f;
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
            tx = m.x - m.dx * 70.f + m.rx * 52.f;
            ty = m.y - m.dy * 70.f + m.ry * 52.f;
            dock = false;
        }
    }

    if (dock) {
        float desiredVy = std::clamp((44.f - y_) * 0.2f, -2.8f, 2.2f);
        float desiredVx = std::clamp((0.f - x_) * 0.24f, -2.2f, 2.2f);
        float aim = (std::fabs(x_) < 40.f) ? kSouth - std::clamp(x_ * 0.05f, -0.6f, 0.6f)
                                            : std::atan2(desiredVy, desiredVx);
        float err = wrap(aim - heading_);
        steer = std::clamp(err / 0.4f, -1.f, 1.f);
        float want = desiredVx * std::cos(heading_) + desiredVy * std::sin(heading_);
        want = std::clamp(want, -2.4f, 3.0f);
        if (std::fabs(x_) < 9.f && std::fabs(y_ - 44.f) < 7.f) {
            if (surge_ > 0.26f) throttle = -0.5f;
            else if (surge_ < -0.26f) throttle = 0.42f;
            else throttle = 0.f;
        } else if (std::fabs(err) > 0.5f && std::fabs(surge_) < 2.f) {
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
    steer = std::clamp(err / 0.45f, -1.f, 1.f);
    float ad = std::fabs(err);
    if (ad > 1.0f) throttle = 0.4f;
    else if (ad > 0.5f) throttle = 0.7f;
    else throttle = 1.f;
    if (leg_ >= 3) {
        if (surge_ > 3.8f) throttle = -0.3f;
        else if (surge_ > 3.0f) throttle = std::min(throttle, 0.22f);
        else throttle = std::min(throttle, 0.65f);
    }
}

void Game::physics(float dt, float steer, float throttle) {
    float auth = std::min(1.f, std::fabs(surge_) / 7.f + 0.75f * std::fabs(throttle));
    float yawCmd = steer * auth * 0.7f;
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
    if (y_ < 12.f && std::fabs(x_) > kMouth) {
        y_ = 12.f;
        if (s * surge_ < 0.f) surge_ *= -0.1f;
        else surge_ *= 0.4f;
        yaw_ = 0;
        hit = true;
    }
    if (y_ < 4.f && std::fabs(x_) <= kMouth) {
        y_ = 4.f;
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
            surge_ *= 0.42f;
            yaw_ = 0;
            hit = true;
        }
    };
    for (const Disk& r : kRocks) bump(r.x, r.y, r.r);
    bump(kWreck.x, kWreck.y, kWreck.r);
    bump(kKelp.x, kKelp.y, kKelp.r);
    if (leg_ < 3) bump(kMarks[leg_].x, kMarks[leg_].y, 7.f);
    if (x_ < -250.f) {
        x_ = -250.f;
        surge_ *= 0.35f;
        hit = true;
    } else if (x_ > 220.f) {
        x_ = 220.f;
        surge_ *= 0.35f;
        hit = true;
    }
    if (y_ > 360.f) {
        y_ = 360.f;
        surge_ *= 0.35f;
        hit = true;
    }
    if (hit && thumpT_ <= 0.f) {
        sys_->apu.noiseBurst(0.3f, 220.f, 0.14f);
        sys_->rumble(0.28f, 0.12f, 60);
        thumpT_ = 0.32f;
    }

    bubT_ -= dt;
    if (bubT_ <= 0.f && std::fabs(surge_) > 1.1f) {
        bubT_ = 0.07f;
        bubbleAt(x_ - c * 8.f + s * 2.f, y_ - s * 8.f - c * 2.f);
    }
    for (Puff& b : bub_) {
        if (b.life <= 0.f) continue;
        b.life -= dt;
        b.y += 6.f * dt;
        b.x += std::sin(b.y * 0.4f) * 1.5f * dt;
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
                if (y_ < 16.f && std::fabs(x_) > kMouth - 4.f) {
                    heading_ = kNorth;
                    surge_ = 2.4f;
                } else if (wp_ < kWayLast) {
                    heading_ = std::atan2(kWay[wp_].y - y_, kWay[wp_].x - x_);
                    surge_ = std::max(surge_, 2.4f);
                } else if (y_ > 58.f) {
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

void Game::bubbleAt(float x, float y) {
    bub_[bubCursor_] = Puff{x, y, 1.f};
    bubCursor_ = (bubCursor_ + 1) % 28;
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
            bubbleAt(m.x + std::cos(a) * 8.f, m.y + std::sin(a) * 8.f);
        }
        leg_++;
        armed_ = false;
        wrong_ = false;
        chime(std::min(leg_, 3));
        pingT_ = 0.22f;
        pingF_ = 620.f + leg_ * 40.f;
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

bool Game::inPen() const {
    if (std::fabs(x_) > kWinX || y_ < kWinY0 || y_ > kWinY1) return false;
    if (std::fabs(surge_) > kStop) return false;
    float south = std::fabs(wrap(heading_ + kNorth));
    float north = std::fabs(wrap(heading_ - kNorth));
    return south < kAim || north < kAim;
}

void Game::finish() {
    if (mode_ != Mode::Dive || leg_ < 3 || !inPen()) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    surge_ = 0;
    yaw_ = 0;
    throttle_ = 0;
    chime(5);
    pingT_ = 0.4f;
    pingF_ = 520.f;
    sys_->setLight(20, 140, 80);
    std::printf("S3 SUB BUOY  PASS  rounded the buoys and returned to the same dock (%.1fs)\n", raceTime_);
    std::fflush(stdout);
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.05f);
    if (pingT_ < 0.08f) pingT_ = 0.08f;
    pingF_ = freq;
}

void Game::chime(int notes) {
    chimeN_ = std::clamp(notes, 1, 5);
    chimeStep_ = 0;
    chimeT_ = 0;
}

void Game::audio(float dt) {
    float rev = 0.2f + std::fabs(throttle_) * 0.8f;
    if (mode_ == Mode::Dive) {
        sys_->apu.noise(0.012f + std::fabs(throttle_) * 0.03f + std::fabs(surge_) * 0.0014f, 90.f + rev * 280.f, true);
        if (pingT_ <= 0.f) sys_->apu.tone(2, 48.f + rev * 18.f, 0.012f + std::fabs(throttle_) * 0.012f);
    } else {
        sys_->apu.noise(0.008f, 140.f, false);
        sys_->apu.tone(2, 0, 0);
    }
    if (pingT_ > 0.f) {
        pingT_ -= dt;
        float f = pingF_ * (1.f - pingT_ * 0.35f);
        sys_->apu.tone(1, f, pingT_ > 0.f ? 0.06f : 0.f);
        if (pingT_ <= 0.f) sys_->apu.tone(1, 0, 0);
    }
    if (tone0_ > 0.f) {
        tone0_ -= dt;
        if (tone0_ <= 0.f) sys_->apu.tone(0, 0, 0);
    }
    if (thumpT_ > 0.f) thumpT_ -= dt;
    if (chimeN_ > 0) {
        chimeT_ -= dt;
        if (chimeT_ <= 0.f) {
            static const float notes[] = {330.f, 415.f, 494.f, 659.f, 784.f};
            int n = std::min(chimeStep_, 4);
            sys_->apu.tone(0, notes[n], 0.05f);
            tone0_ = 0.13f;
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
    if (mode_ == Mode::Dive || mode_ == Mode::Pause) return 1;
    return 0;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_TURBO)) {
            begin();
            mode_ = Mode::Dive;
            zoom_ = kPlayZoom;
            camX_ = x_;
            camY_ = y_;
            blip(520.f);
            sys.setLight(20, 80, 140);
        } else if (pad.pressed(gs::BTN_MODE)) {
            sys.quit();
        }
    } else if (mode_ == Mode::Dive) {
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
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Dive;
        else if (pad.pressed(gs::BTN_MODE)) showTitle();
    } else if (mode_ == Mode::Win) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            begin();
            mode_ = Mode::Dive;
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
        float lead = (mode_ == Mode::Dive) ? 14.f : 0.f;
        if (leg_ >= 3) lead = 6.f;
        float gx = x_ + std::cos(heading_) * lead;
        float gy = y_ + std::sin(heading_) * lead;
        float k = 1.f - std::exp(-kDt * 4.2f);
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
        hudC(21, "ARROWS  RUDDER", PAL_HUD);
        hudC(22, "UP AHEAD    DOWN ASTERN", PAL_HUD);
        hudC(23, "Z AHEAD   X BACK   C PING", PAL_BANNER);
        hudC(24, "LEAVE EACH BUOY TO PORT", PAL_ALERT);
        hudC(25, "THEN STOP IN THIS PEN", PAL_WIN);
        if ((int(t_ * 2.f) & 1) == 0) hudC(27, "ENTER", PAL_BANNER);
        return;
    }
    hud(1, 0, "S3 SUB BUOY", PAL_BANNER);
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
        if (!bot_) hudC(18, "ENTER DIVES AGAIN", PAL_HUD);
        return;
    }
    if (leg_ < 3) std::snprintf(buf, sizeof buf, "NEXT %s", kMarks[leg_].name);
    else std::snprintf(buf, sizeof buf, "NEXT SAME PEN");
    hud(1, 1, buf, leg_ < 3 ? kMarks[leg_].pal : PAL_WIN);
    hud(1, 2, orderName(throttle_), throttle_ < -0.05f ? PAL_ALERT : PAL_HUD);
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
        } else if (box) {
            hint = "POINT IN OR BACK IN";
            hpal = PAL_BANNER;
        } else {
            hint = "SLOW INTO THE SAME PEN";
            hpal = PAL_BANNER;
        }
    } else if (raceTime_ < 7.f) {
        hint = "FLOOD AHEAD AND HOLD PORT";
    }
    if (hint) hud(1, 27, hint, hpal);
}

int Game::hullFrame() const {
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
    const uint16_t silt = gs::rgb4(4, 4, 3);
    const uint16_t shore = gs::rgb4(6, 6, 5);
    const uint16_t shall = gs::rgb4(1, 6, 8);
    const uint16_t mid = gs::rgb4(0, 3, 6);
    const uint16_t deep = gs::rgb4(0, 1, 3);
    float scroll = std::fmod(t_, 600.f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (112.f - y) / std::max(zoom_, 0.05f);
        uint16_t c;
        if (wy < -4.f) c = lerpC(silt, shore, std::clamp((wy + 36.f) / 32.f, 0.f, 1.f));
        else if (wy < 18.f) c = lerpC(shall, mid, std::clamp((wy + 4.f) / 22.f, 0.f, 1.f));
        else c = lerpC(mid, deep, std::clamp((wy - 18.f) / 240.f, 0.f, 1.f));
        v.lineBackdrop[y] = c;
        v.lineFog[y] = uint8_t(std::clamp(int((wy - 80.f) / 40.f), 0, 6));
        v.road[y].on = false;
        float wob = std::sin(y * 0.07f + t_ * 1.1f) * 6.f;
        v.B.hscroll[y] = int16_t(wob + scroll * 10.f);
        v.B.vscroll[y] = int16_t(scroll * 3.f);
    }
    v.A.enabled = false;

    auto banner = [&](const gs::Mipped& m, float x, float y, int pal) { spr(m, x, y, float(m.h), pal, false); };
    if (mode_ == Mode::Title) {
        banner(art_.title, 160, 18, PAL_BANNER);
        banner(art_.line, 160, 42, PAL_BANNER);
    } else if (mode_ == Mode::Pause) {
        banner(art_.paused, 160, 96, PAL_BANNER);
    } else if (mode_ == Mode::Win) {
        banner(art_.same, 160, 74, PAL_WIN);
        banner(art_.line, 160, 104, PAL_WIN);
    }

    if (mode_ == Mode::Dive || mode_ == Mode::Pause) {
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
            sx = 286.f + (wx + 8.f) * 0.13f;
            sy = 62.f - (wy - 130.f) * 0.13f;
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
        spr(art_.panel, 286.f, 62.f, 54.f, PAL_MAP, false);
    }

    float bsx, bsy;
    worldToScreen(x_, y_, bsx, bsy);
    bsy += std::sin(t_ * 1.6f) * 0.6f;
    float boatH = 26.f * zoom_;
    if (title) boatH = std::max(boatH, 28.f);
    const gs::Mipped& hull = art_.hull[hullFrame()];
    spr(hull, bsx + 2.f, bsy + 2.f, boatH, PAL_SUB, true);
    spr(hull, bsx, bsy, boatH, PAL_SUB, false);

    for (const Puff& b : bub_) {
        if (b.life <= 0.f) continue;
        float sx, sy;
        worldToScreen(b.x, b.y, sx, sy);
        spr(art_.bubble, sx, sy, 4.f + (1.f - b.life) * 5.f, PAL_BUB, false);
    }

    auto arc = [&](const Mark& m, int pal) {
        float a0 = std::atan2(m.ry, m.rx);
        for (int i = 0; i < 6; i++) {
            float a = a0 + (i - 2.5f) * 0.32f;
            place(art_.dot, m.x + std::cos(a) * 40.f, m.y + std::sin(a) * 40.f, 2.4f, pal, title ? 3.f : 0.f);
        }
    };
    if (title) {
        for (const Mark& m : kMarks) arc(m, m.pal);
    } else if (leg_ < 3) {
        arc(kMarks[leg_], kMarks[leg_].pal);
    }

    for (int i = 0; i < 3; i++) {
        bool hot = (i == leg_ && leg_ < 3);
        if ((hot || title) && ((int(t_ * 3.f + i) & 1) == 0))
            place(art_.lamp, kMarks[i].x, kMarks[i].y + 7.f, 4.f, PAL_LAMP, title ? 5.f : 0.f);
        float pulse = hot ? 1.f + 0.06f * std::sin(t_ * 4.f) : 1.f;
        if (hot || title)
            place(art_.ring, kMarks[i].x, kMarks[i].y, (hot ? 36.f : 30.f) * pulse, kMarks[i].pal, title ? 12.f : 0.f);
        place(art_.buoy[i], kMarks[i].x, kMarks[i].y, (hot ? 16.f : 13.f) * pulse, kMarks[i].pal, title ? 12.f : 0.f);
    }
    if (leg_ >= 3 || title) {
        if (title || (int(t_ * 3.f) & 1) == 0) place(art_.lamp, 0.f, 16.f, 5.f, PAL_WIN, title ? 6.f : 0.f);
    }

    for (int i = 0; i < 4; i++) {
        float u = t_ * (0.35f + i * 0.05f) + i * 1.7f;
        float gx = -20.f + std::sin(u) * 70.f + i * 18.f;
        float gy = 210.f + std::cos(u * 0.6f) * 28.f;
        place(art_.fish[int(t_ * 4.f + i) & 1], gx, gy, 5.f, PAL_FISH, title ? 5.f : 0.f);
    }

    place(art_.wreck, kWreck.x, kWreck.y, 22.f, PAL_WRECK, title ? 10.f : 0.f);
    const float kelp[][2] = {{-140.f, 36.f}, {-128.f, 28.f}, {-152.f, 24.f}, {170.f, 200.f}, {182.f, 188.f}};
    for (const float* k : kelp) place(art_.kelp, k[0], k[1], 16.f, PAL_KELP, title ? 8.f : 0.f);
    for (const Disk& r : kRocks) place(art_.wreck, r.x, r.y, r.r * 1.3f, PAL_WRECK, title ? 8.f : 0.f);
    place(art_.crane, 110.f, 4.f, 26.f, PAL_DOCK, title ? 12.f : 0.f);
    place(art_.shed, -88.f, -2.f, 24.f, PAL_DOCK, title ? 12.f : 0.f);
    const float piles[][2] = {{-34.f, 18.f}, {34.f, 18.f}, {-34.f, 38.f}, {34.f, 38.f}};
    for (const float* p : piles) place(art_.pile, p[0], p[1], 8.f, PAL_DOCK, title ? 7.f : 0.f);
    place(art_.beam, 0.f, -6.f, 12.f, PAL_DOCK, title ? 8.f : 0.f);
    for (int i = -6; i <= 6; i++) {
        float qx = i * 36.f;
        if (std::fabs(qx) < 42.f) continue;
        place((i & 1) ? art_.quayB : art_.quay, qx, 2.f, 22.f, PAL_DOCK, title ? 10.f : 0.f);
    }

    drawHud();
}

}  // namespace subby

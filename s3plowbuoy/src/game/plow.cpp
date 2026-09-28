#include "game/plow.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace plow {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;
constexpr float kNorth = 1.5707963f;
constexpr float kSouth = -1.5707963f;
constexpr float kMaxPace = 16.5f;
constexpr float kPlayZoom = 1.55f;
constexpr float kTitleZoom = 0.46f;
constexpr float kTitleCamX = -6.f;
constexpr float kTitleCamY = 150.f;
constexpr float kStartX = 0.f;
constexpr float kStartY = 52.f;
constexpr float kStartH = kNorth;
constexpr float kMouth = 22.f;
constexpr float kShoreY = 18.f;
constexpr float kBackY = 28.f;
constexpr float kWinX = 14.f;
constexpr float kWinY0 = 30.f;
constexpr float kWinY1 = 78.f;
constexpr float kStop = 1.15f;
constexpr float kAim = 1.2f;
constexpr float kArm = -16.f;
constexpr float kPass = 12.f;
constexpr float kSideMin = 12.f;
constexpr float kSideMax = 110.f;
constexpr float kOtherX = 14.f;
constexpr float kOtherY0 = 300.f;
constexpr float kOtherY1 = 338.f;
constexpr float kOtherHalf = 22.f;
constexpr float kClock = 110.f;

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

const Mark kMarks[3] = {
    {78.f, 148.f, 0.f, 1.f, -1.f, 0.f, "RED 1", PAL_NUN},
    {-6.f, 246.f, -1.f, 0.f, 0.f, -1.f, "GREEN 2", PAL_CAN},
    {-108.f, 156.f, 0.f, -1.f, 1.f, 0.f, "RED 3", PAL_NUN},
};

const Way kWay[] = {
    {0.f, 100.f, 16.f, 0},  {48.f, 120.f, 18.f, 0}, {48.f, 190.f, 20.f, 1}, {90.f, 220.f, 20.f, 1},
    {20.f, 210.f, 18.f, 1}, {-50.f, 210.f, 20.f, 2}, {-70.f, 200.f, 18.f, 2}, {-78.f, 120.f, 20.f, 3},
    {0.f, 150.f, 22.f, 3},  {0.f, 100.f, 16.f, 3},  {0.f, 58.f, 12.f, 3},
};
constexpr int kWayLast = 10;

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

bool tracing() {
    static int on = -1;
    if (on < 0) on = std::getenv("PLOW_TRACE") != nullptr;
    return on != 0;
}

}  // namespace

float Game::speed() const { return std::hypot(vx_, vy_); }

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (leg_ >= 3) return 3;
    if (leg_ >= 1) return 2;
    return 1;
}

void Game::begin() {
    x_ = kStartX;
    y_ = kStartY;
    heading_ = kStartH;
    vx_ = vy_ = yaw_ = 0;
    leg_ = 0;
    wp_ = 0;
    armed_ = true;
    wrong_ = false;
    raceTime_ = 0;
    clock_ = kClock;
    sprayT_ = 0;
    stuckT_ = 0;
    stuckX_ = x_;
    stuckY_ = y_;
    throttle_ = 0;
    won_ = false;
    over_ = false;
    why_[0] = 0;
    chimeN_ = 0;
    chimeStep_ = 0;
    puffCursor_ = 0;
    for (Puff& p : spray_) p.life = 0;
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
    sys.vdp.setFogColor(gs::rgb4(10, 12, 14));
    sys.apu.setMaster(0.7f);
    sys.apu.setEcho(0.18f, 0.22f, 0.1f);
    begin();
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
    if (p.down(gs::BTN_LEFT)) steer -= 1.f;
    if (p.down(gs::BTN_RIGHT)) steer += 1.f;
    if (std::fabs(p.axisX) > 0.18f) steer = std::clamp(p.axisX, -1.f, 1.f);
    throttle = 0;
    if (p.down(gs::BTN_UP) || p.down(gs::BTN_A) || p.down(gs::BTN_TURBO)) throttle = 1.f;
    if (p.down(gs::BTN_DOWN) || p.down(gs::BTN_B)) throttle = -1.f;
    if (p.accel > 0.12f) throttle = p.accel;
    if (p.brake > 0.12f) throttle = -p.brake;
    if (std::fabs(p.axisY) > 0.18f) throttle = std::clamp(p.axisY, -1.f, 1.f);
    if (p.pressed(gs::BTN_C) || p.pressed(gs::BTN_X) || p.pressed(gs::BTN_Y) || p.pressed(gs::BTN_Z)) horn();
}

void Game::pilot(float& steer, float& throttle) {
    steer = 0;
    throttle = 0;
    const Way& w = kWay[std::clamp(wp_, 0, kWayLast)];
    float tx = w.x, ty = w.y;
    bool dock = wp_ >= kWayLast && leg_ >= 3;
    if (leg_ < 3 && !armed_) {
        const Mark& m = kMarks[leg_];
        tx = m.x - m.dx * 70.f + m.rx * 40.f;
        ty = m.y - m.dy * 70.f + m.ry * 40.f;
        dock = false;
    }

    float c = std::cos(heading_), s = std::sin(heading_);
    float lat = vx_ * -s + vy_ * c;
    float fwd = vx_ * c + vy_ * s;
    float sp = std::hypot(vx_, vy_);

    if (dock) {
        float gx = 0.f, gy = 46.f;
        if (std::fabs(x_) > 7.f && y_ > 70.f) gy = y_ - 6.f;
        float dx = gx - x_, dy = gy - y_;
        float dist = std::hypot(dx, dy);
        float want = std::clamp(dist * 0.09f, 0.f, 3.2f);
        if (y_ < 62.f && std::fabs(x_) <= kWinX) want = 0.f;
        float dvx = 0.f, dvy = 0.f;
        if (dist > 0.6f && want > 0.05f) {
            dvx = dx / dist * want;
            dvy = dy / dist * want;
        }
        float aim = (want > 0.25f) ? std::atan2(dvy, dvx) : kSouth;
        float err = wrap(aim - heading_);
        steer = std::clamp(err / 0.32f + lat / 8.f, -1.f, 1.f);
        float des = dvx * c + dvy * s;
        if (std::fabs(err) > 0.85f) throttle = (sp > 2.f) ? -0.8f : 0.3f;
        else if (want < 0.05f) throttle = -1.f;
        else if (fwd > des + 0.25f) throttle = -0.85f;
        else if (fwd < des - 0.2f) throttle = 0.7f;
        else throttle = 0.05f;
        return;
    }

    float aim = std::atan2((ty - y_) - vy_ * 0.25f, (tx - x_) - vx_ * 0.25f);
    float err = wrap(aim - heading_);
    steer = std::clamp(err / 0.38f + lat / 10.f, -1.f, 1.f);
    float ad = std::fabs(err);
    if (ad > 0.55f && sp > 7.f) throttle = -0.45f;
    else if (ad > 1.05f) throttle = 0.35f;
    else if (ad > 0.45f) throttle = 0.65f;
    else throttle = 1.f;

    float cap = kMaxPace;
    if (leg_ >= 3) {
        cap = 8.f;
        if (wp_ >= kWayLast - 1) cap = 3.6f;
        else if (wp_ >= kWayLast - 2) cap = 5.5f;
    }
    if (sp > cap + 0.3f) throttle = -0.75f;
    else if (sp > cap) throttle = std::min(throttle, 0.f);
}

void Game::puffAt(float x, float y) {
    spray_[puffCursor_] = Puff{x, y, 1.f};
    puffCursor_ = (puffCursor_ + 1) % 16;
}

void Game::physics(float dt, float steer, float throttle) {
    float sp = std::hypot(vx_, vy_);
    float auth = std::clamp(0.35f + sp / 14.f, 0.35f, 1.f);
    float yawCmd = steer * 2.15f * auth;
    yaw_ += (yawCmd - yaw_) * (1.f - std::exp(-6.f * dt));
    heading_ = wrap(heading_ + yaw_ * dt);

    float c = std::cos(heading_), s = std::sin(heading_);
    float fwd = vx_ * c + vy_ * s;
    float lat = vx_ * -s + vy_ * c;
    if (throttle > 0.05f) fwd += throttle * 22.f * dt;
    else if (throttle < -0.05f) fwd += throttle * 16.f * dt;
    float drag = (throttle < -0.05f) ? 6.2f : (throttle > 0.05f ? 0.18f : 0.35f);
    fwd *= std::exp(-drag * dt);
    lat *= std::exp(-4.8f * dt);
    fwd = std::clamp(fwd, -4.f, kMaxPace);
    vx_ = fwd * c + lat * -s;
    vy_ = fwd * s + lat * c;
    x_ += vx_ * dt;
    y_ += vy_ * dt;

    auto bump = [&](float cx, float cy, float rad) {
        float dx = x_ - cx, dy = y_ - cy;
        float d = std::hypot(dx, dy);
        if (d < rad && d > 0.001f) {
            x_ = cx + dx / d * rad;
            y_ = cy + dy / d * rad;
            vx_ *= 0.35f;
            vy_ *= 0.35f;
            yaw_ *= 0.2f;
            if (thumpT_ <= 0.f) {
                sys_->apu.noiseBurst(0.32f, 220.f, 0.12f);
                sys_->rumble(0.4f, 0.15f, 70);
                thumpT_ = 0.3f;
            }
        }
    };
    if (leg_ < 3) bump(kMarks[leg_].x, kMarks[leg_].y, 8.f);

    if (y_ < kBackY && std::fabs(x_) <= kMouth) {
        y_ = kBackY;
        if (vy_ < 0.f) vy_ = 0.f;
        vx_ *= 0.4f;
    }
    if (y_ > 26.f && y_ < 72.f) {
        if (x_ > kMouth && x_ < kMouth + 16.f) {
            x_ = kMouth;
            if (vx_ > 0.f) vx_ = 0.f;
        }
        if (x_ < -kMouth && x_ > -kMouth - 16.f) {
            x_ = -kMouth;
            if (vx_ < 0.f) vx_ = 0.f;
        }
    }
    if (y_ < kShoreY && std::fabs(x_) > kMouth + 4.f) {
        y_ = kShoreY;
        if (vy_ < 0.f) vy_ = 0.f;
        vx_ *= 0.4f;
        if (leg_ >= 3 && mode_ == Mode::Run) fail("missed the end");
    }
    if (y_ > kOtherY1 - 8.f && std::fabs(x_ - kOtherX) < kOtherHalf) {
        y_ = kOtherY1 - 8.f;
        if (vy_ > 0.f) vy_ = 0.f;
    }
    if (x_ < -230.f) {
        x_ = -230.f;
        if (vx_ < 0.f) vx_ = 0.f;
    } else if (x_ > 210.f) {
        x_ = 210.f;
        if (vx_ > 0.f) vx_ = 0.f;
    }
    if (y_ > 380.f) {
        y_ = 380.f;
        if (vy_ > 0.f) vy_ = 0.f;
    }

    sp = std::hypot(vx_, vy_);
    sprayT_ -= dt;
    if (sprayT_ <= 0.f && sp > 2.f && mode_ == Mode::Run) {
        sprayT_ = 0.06f;
        puffAt(x_ - c * 10.f + s * 6.f, y_ - s * 10.f - c * 6.f);
        puffAt(x_ - c * 10.f - s * 6.f, y_ - s * 10.f + c * 6.f);
    }
    for (Puff& p : spray_)
        if (p.life > 0.f) p.life -= dt * 0.9f;

    if (bot_ && mode_ == Mode::Run) {
        stuckT_ += dt;
        if (stuckT_ > 2.2f) {
            float moved = std::hypot(x_ - stuckX_, y_ - stuckY_);
            stuckX_ = x_;
            stuckY_ = y_;
            stuckT_ = 0;
            bool docking = leg_ >= 3 && y_ < 130.f && std::fabs(x_) < 40.f;
            if (moved < 2.2f && !won_) {
                if (!docking) {
                    const Way& w = kWay[std::clamp(wp_, 0, kWayLast)];
                    heading_ = std::atan2(w.y - y_, w.x - x_);
                    vx_ = std::cos(heading_) * 3.f;
                    vy_ = std::sin(heading_) * 3.f;
                    yaw_ = 0;
                }
            }
        }
    }
    if (thumpT_ > 0.f) thumpT_ -= dt;
}

void Game::scoreMarks() {
    if (leg_ >= 3 || mode_ != Mode::Run) return;
    const Mark& m = kMarks[leg_];
    float dx = x_ - m.x, dy = y_ - m.y;
    float along = dx * m.dx + dy * m.dy;
    float lateral = dx * m.rx + dy * m.ry;
    if (along < kArm) {
        armed_ = true;
        wrong_ = false;
    }
    if (!armed_ || along <= kPass) return;
    float drive = vx_ * m.dx + vy_ * m.dy;
    if (drive < 0.8f) return;
    if (lateral > kSideMin && lateral < kSideMax) {
        leg_++;
        armed_ = false;
        wrong_ = false;
        chime(std::min(leg_, 3));
        sys_->rumble(0.2f, 0.35f, 70);
        for (int i = 0; i < 6; i++) {
            float a = i * kTau / 6.f;
            puffAt(m.x + std::cos(a) * 12.f, m.y + std::sin(a) * 12.f);
        }
    } else {
        armed_ = false;
        wrong_ = true;
        blip(130.f);
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
    if (std::hypot(vx_, vy_) > kStop) return false;
    float south = std::fabs(wrap(heading_ - kSouth));
    float north = std::fabs(wrap(heading_ - kNorth));
    return south < kAim || north < kAim;
}

bool Game::inOther() const {
    return std::fabs(x_ - kOtherX) <= kOtherHalf && y_ >= kOtherY0 && y_ <= kOtherY1 && std::hypot(vx_, vy_) < 1.4f;
}

void Game::finish() {
    if (mode_ != Mode::Run || leg_ < 3 || !inSlip()) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    vx_ = vy_ = yaw_ = 0;
    throttle_ = 0;
    chime(5);
    sys_->setLight(40, 150, 70);
    sys_->rumble(0.16f, 0.4f, 140);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    std::snprintf(why_, sizeof why_, "%s", why);
    vx_ = vy_ = yaw_ = 0;
    throttle_ = 0;
    blip(70.f);
    sys_->setLight(160, 40, 24);
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.18f);
    tone0_ = 0.12f;
}

void Game::horn() {
    hornT_ = 0.28f;
    sys_->apu.tone(1, 92.f, 0.22f);
}

void Game::chime(int notes) {
    chimeN_ = notes;
    chimeStep_ = 0;
    chimeT_ = 0.01f;
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
    if (chimeN_ > 0) {
        chimeT_ -= dt;
        if (chimeT_ <= 0.f) {
            static const float notes[] = {523.f, 659.f, 784.f, 1046.f, 784.f};
            int i = std::min(chimeStep_, 4);
            sys_->apu.tone(2, notes[i], 0.16f);
            chimeStep_++;
            chimeT_ = 0.12f;
            if (chimeStep_ >= chimeN_) {
                chimeN_ = 0;
                tone0_ = std::max(tone0_, 0.14f);
            }
        }
    } else if (mode_ == Mode::Run) {
        float sp = std::hypot(vx_, vy_);
        float vol = (throttle_ > 0.05f || sp > 1.f) ? 0.05f + sp * 0.008f : 0.f;
        sys_->apu.tone(2, 70.f + sp * 9.f, vol);
    } else {
        sys_->apu.tone(2, 0, 0);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || bot_) {
            begin();
            mode_ = Mode::Run;
            zoom_ = kPlayZoom;
            camX_ = x_;
            camY_ = y_;
            blip(240.f);
        }
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        float steer = 0, throttle = 0;
        if (bot_) pilot(steer, throttle);
        else controls(steer, throttle);
        throttle_ = throttle;
        physics(kDt, steer, throttle);
        scoreMarks();
        guide();
        raceTime_ += kDt;
        clock_ -= kDt;
        if (clock_ <= 0.f) fail("the other crew");
        else if (inOther() && leg_ >= 3) fail("wrong dock");
        else finish();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Run;
        else if (pad.pressed(gs::BTN_MODE)) showTitle();
    } else if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            begin();
            mode_ = Mode::Run;
            zoom_ = kPlayZoom;
            camX_ = x_;
            camY_ = y_;
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        }
    }

    if (mode_ == Mode::Title) {
        camX_ = kTitleCamX + std::sin(t_ * 0.12f) * 4.f;
        camY_ = kTitleCamY;
        zoom_ = kTitleZoom;
    } else {
        float lead = (mode_ == Mode::Run) ? 10.f : 0.f;
        float gx = x_ + std::cos(heading_) * lead;
        float gy = y_ + std::sin(heading_) * lead;
        float k = 1.f - std::exp(-kDt * 4.f);
        camX_ += (gx - camX_) * k;
        camY_ += (gy - camY_) * k;
        zoom_ += (kPlayZoom - zoom_) * k;
    }
    audio(kDt);
    if (tracing() && bot_ && mode_ == Mode::Run && int(t_) != int(t_ - kDt)) {
        std::fprintf(stderr, "t %.0f leg %d wp %d x %.0f y %.0f hdg %.2f spd %.1f armed %d clock %.0f\n", t_, leg_, wp_,
                     x_, y_, heading_, std::hypot(vx_, vy_), armed_ ? 1 : 0, clock_);
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
        hudC(21, "ARROWS STEER THE PLOW", PAL_HUD);
        hudC(22, "UP AHEAD    DOWN BRAKE", PAL_HUD);
        hudC(23, "LEAVE EACH BUOY TO PORT", PAL_WIN);
        hudC(24, "THEN STOP IN THIS DOCK", PAL_BANNER);
        hudC(25, "THE CLOCK IS THE OTHER CREW", PAL_ALERT);
        if ((int(t_ * 2.f) & 1) == 0) hudC(27, "ENTER", PAL_WIN);
        return;
    }
    hud(1, 0, "S3 PLOW BUOY", PAL_BANNER);
    int left = std::max(0, int(std::ceil(clock_)));
    std::snprintf(buf, sizeof buf, "CREW %d:%02d", left / 60, left % 60);
    hud(28, 0, buf, clock_ < 20.f ? PAL_ALERT : PAL_CREW);
    if (mode_ == Mode::Pause) {
        hudC(18, "ENTER CONTINUES", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Win) {
        int sec = int(raceTime_);
        std::snprintf(buf, sizeof buf, "TIME %d:%02d", sec / 60, sec % 60);
        hudC(16, buf, PAL_HUD);
        if (!bot_) hudC(18, "ENTER TAKES ANOTHER LEG", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Fail) {
        hudC(16, why_[0] ? why_ : "MISSED THE END", PAL_ALERT);
        if (!bot_) hudC(18, "ENTER TRIES AGAIN", PAL_HUD);
        return;
    }
    if (leg_ < 3) std::snprintf(buf, sizeof buf, "NEXT %s", kMarks[leg_].name);
    else std::snprintf(buf, sizeof buf, "NEXT SAME DOCK");
    hud(1, 1, buf, leg_ < 3 ? kMarks[leg_].pal : PAL_WIN);
    std::snprintf(buf, sizeof buf, "BUOYS %d/3", std::min(leg_, 3));
    hud(1, 26, buf, PAL_WIN);
    const char* hint = nullptr;
    int hpal = PAL_HUD;
    if (wrong_ && leg_ < 3) {
        hint = "GO BACK AND LEAVE IT TO PORT";
        hpal = PAL_ALERT;
    } else if (leg_ >= 3) {
        hint = "THE SAME DOCK IS THE END";
        hpal = PAL_BANNER;
    } else if (raceTime_ < 6.f) {
        hint = "BLADE UP. THE ICE IS FAST";
    }
    if (hint) hud(1, 27, hint, hpal);
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

int Game::hullFrame() const {
    float u = std::fmod(heading_, kTau);
    if (u < 0.f) u += kTau;
    int i = int(std::lround(u / kTau * 8.f)) % 8;
    if (i < 0) i += 8;
    return i;
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = true;
    v.hudEnabled = true;
    const uint16_t snow = gs::rgb4(13, 14, 15);
    const uint16_t ice = gs::rgb4(9, 12, 14);
    const uint16_t deep = gs::rgb4(5, 8, 12);
    const uint16_t bank = gs::rgb4(12, 11, 9);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (112.f - y) / std::max(zoom_, 0.05f);
        uint16_t c;
        if (wy < 16.f) c = lerpC(bank, snow, std::clamp((wy + 10.f) / 26.f, 0.f, 1.f));
        else if (wy < 90.f) c = lerpC(snow, ice, std::clamp((wy - 16.f) / 74.f, 0.f, 1.f));
        else c = lerpC(ice, deep, std::clamp((wy - 90.f) / 220.f, 0.f, 1.f));
        v.lineBackdrop[y] = c;
        v.lineFog[y] = 0;
        v.road[y].on = false;
        v.B.hscroll[y] = int16_t(camX_ * 0.12f);
        v.B.vscroll[y] = int16_t(-camY_ * 0.08f);
    }

    auto banner = [&](const gs::Mipped& m, float x, float y, int pal) { spr(m, x, y, float(m.h), pal, false); };
    if (mode_ == Mode::Title) {
        banner(art_.title, 160, 18, PAL_BANNER);
        banner(art_.round, 160, 46, PAL_BANNER);
    } else if (mode_ == Mode::Pause) {
        banner(art_.paused, 160, 96, PAL_BANNER);
    } else if (mode_ == Mode::Win) {
        banner(art_.same, 160, 70, PAL_WIN);
        banner(art_.made, 160, 100, PAL_WIN);
    } else if (mode_ == Mode::Fail) {
        if (std::strcmp(why_, "wrong dock") == 0) banner(art_.wrong, 160, 78, PAL_ALERT);
        else if (std::strcmp(why_, "the other crew") == 0) banner(art_.crew, 160, 78, PAL_ALERT);
        else banner(art_.missed, 160, 78, PAL_ALERT);
    }

    place(art_.quay, 0.f, 20.f, 26.f, PAL_DOCK, 6.f);
    place(art_.shed, -24.f, 12.f, 16.f, PAL_DOCK, 4.f);
    place(art_.flag, 18.f, 16.f, 14.f, PAL_ALERT, 3.f);
    place(art_.lamp, -10.f, 36.f, 8.f, PAL_LAMP, 2.f);
    place(art_.lamp, 10.f, 36.f, 8.f, PAL_LAMP, 2.f);
    for (int i = 0; i < 5; i++) {
        place(art_.pile, -kMouth - 3.f, 28.f + i * 8.f, 10.f, PAL_POST, 2.f);
        place(art_.pile, kMouth + 3.f, 28.f + i * 8.f, 10.f, PAL_POST, 2.f);
    }
    place(art_.quay, kOtherX, kOtherY1 - 6.f, 24.f, PAL_OTHER, 5.f);
    place(art_.shed, kOtherX + 16.f, kOtherY1 + 4.f, 14.f, PAL_OTHER, 3.f);
    place(art_.crate, kOtherX - 12.f, kOtherY1 - 4.f, 8.f, PAL_OTHER, 2.f);

    place(art_.nun, kMarks[0].x, kMarks[0].y, 16.f, PAL_NUN, 4.f);
    place(art_.can, kMarks[1].x, kMarks[1].y, 16.f, PAL_CAN, 4.f);
    place(art_.nun3, kMarks[2].x, kMarks[2].y, 16.f, PAL_NUN, 4.f);
    if (leg_ < 3 && mode_ == Mode::Run) {
        float bob = std::sin(t_ * 2.f) * 1.1f;
        place(art_.ring, kMarks[leg_].x, kMarks[leg_].y + bob * 0.2f, 18.f, PAL_WIN, 3.f);
    }

    for (const Puff& p : spray_) {
        if (p.life <= 0.f) continue;
        place(art_.spray, p.x, p.y, 6.f * p.life, PAL_SPRAY, 1.f);
    }
    place(art_.plow[hullFrame()], x_, y_, 22.f, PAL_PLOW, 8.f);

    if (mode_ == Mode::Run && leg_ < 3) {
        const Mark& m = kMarks[leg_];
        float sx, sy;
        worldToScreen(m.x, m.y, sx, sy);
        if (sx < 8 || sx > 312 || sy < 8 || sy > 200) {
            float ax = std::clamp(sx, 16.f, 304.f);
            float ay = std::clamp(sy, 20.f, 190.f);
            spr(art_.pin, ax, ay, 12.f, m.pal, false);
        }
    }
    drawHud();
}

}  // namespace plow

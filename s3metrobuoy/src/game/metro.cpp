#include "game/metro.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace metrobuoy {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;
constexpr float kNorth = 1.5707963f;
constexpr float kSouth = -1.5707963f;
constexpr float kMaxPace = 10.4f;
constexpr float kPlayZoom = 1.22f;
constexpr float kTitleZoom = 0.46f;
constexpr float kTitleCamX = 6.f;
constexpr float kTitleCamY = 150.f;
constexpr float kStartX = 0.f;
constexpr float kStartY = 58.f;
constexpr float kStartH = kNorth;
constexpr float kMouth = 22.f;
constexpr float kShoreY = 22.f;
constexpr float kBackY = 32.f;
constexpr float kWinX = 12.f;
constexpr float kWinY0 = 36.f;
constexpr float kWinY1 = 80.f;
constexpr float kStop = 0.72f;
constexpr float kAim = 1.15f;
constexpr float kArm = -16.f;
constexpr float kPass = 12.f;
constexpr float kSideMin = 14.f;
constexpr float kSideMax = 110.f;
constexpr float kOtherX = 28.f;
constexpr float kOtherY0 = 348.f;
constexpr float kOtherY1 = 388.f;
constexpr float kOtherHalf = 24.f;
constexpr float kLimit = 155.f;

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

// Leave each buoy to port. The metro's starboard side stays on open water.
const Mark kMarks[3] = {
    {96.f, 168.f, 0.f, 1.f, 1.f, 0.f, "AMBER 1", PAL_AMBER},
    {-6.f, 292.f, -1.f, 0.f, 0.f, -1.f, "TEAL 2", PAL_TEAL},
    {-128.f, 176.f, 0.f, -1.f, 1.f, 0.f, "VIOLET 3", PAL_AMBER},
};

const Way kWay[] = {
    {0.f, 118.f, 18.f, 0},
    {150.f, 110.f, 26.f, 0},
    {158.f, 210.f, 26.f, 0},
    {158.f, 230.f, 22.f, 1},
    {-6.f, 220.f, 26.f, 1},
    {-90.f, 220.f, 26.f, 2},
    {-78.f, 100.f, 24.f, 2},
    {-20.f, 150.f, 22.f, 3},
    {0.f, 120.f, 18.f, 3},
    {0.f, 52.f, 12.f, 3},
};
constexpr int kWayLast = 9;

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
    if (throttle > 0.55f) return "DRIVE";
    if (throttle > 0.08f) return "CREEP";
    if (throttle < -0.45f) return "REVERSE";
    if (throttle < -0.05f) return "BRAKE";
    return "IDLE";
}

bool tracing() {
    static int on = -1;
    if (on < 0) on = std::getenv("METRO_TRACE") != nullptr;
    return on != 0;
}

}  // namespace

float Game::speed() const { return std::hypot(vx_, vy_); }

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
    wakeT_ = 0;
    stuckT_ = 0;
    stuckX_ = x_;
    stuckY_ = y_;
    sideT_ = 0;
    otherT_ = 0;
    throttle_ = 0;
    won_ = false;
    over_ = false;
    why_[0] = 0;
    chimeN_ = 0;
    chimeStep_ = 0;
    puffCursor_ = 0;
    for (Puff& p : wake_) p.life = 0;
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
    sys.vdp.setFogColor(gs::rgb4(3, 5, 7));
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
    if (p.down(gs::BTN_LEFT)) steer += 1.f;
    if (p.down(gs::BTN_RIGHT)) steer -= 1.f;
    if (std::fabs(p.axisX) > 0.18f) steer = std::clamp(-p.axisX, -1.f, 1.f);
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
        tx = m.x - m.dx * 72.f + m.rx * 48.f;
        ty = m.y - m.dy * 72.f + m.ry * 48.f;
        dock = false;
    }

    float c = std::cos(heading_), s = std::sin(heading_);
    float lat = vx_ * -s + vy_ * c;
    float fwd = vx_ * c + vy_ * s;
    float sp = std::hypot(vx_, vy_);

    if (dock) {
        float gx = 0.f, gy = 48.f;
        if (std::fabs(x_) > 8.f && y_ > 78.f) gy = y_ - 6.f;
        float dx = gx - x_, dy = gy - y_;
        float dist = std::hypot(dx, dy);
        float want = std::clamp(dist * 0.08f, 0.f, 3.1f);
        if (std::fabs(x_) > 10.f) want = std::min(want, 2.2f);
        if (y_ < 62.f && std::fabs(x_) <= kWinX) want = 0.f;
        float dvx = 0.f, dvy = 0.f;
        if (dist > 0.8f && want > 0.05f) {
            dvx = dx / dist * want;
            dvy = dy / dist * want;
        }
        float aim = (want > 0.3f) ? std::atan2(dvy, dvx) : (kSouth - std::clamp(x_ * 0.07f, -0.35f, 0.35f));
        float err = wrap(aim - heading_);
        steer = std::clamp(err / 0.32f, -1.f, 1.f);
        steer += std::clamp(lat / 5.5f, -0.3f, 0.3f);
        steer = std::clamp(steer, -1.f, 1.f);
        float des = dvx * c + dvy * s;
        if (std::fabs(err) > 0.85f) throttle = (sp > 1.5f) ? -0.65f : 0.3f;
        else if (want < 0.05f) throttle = -1.f;
        else if (fwd > des + 0.2f) throttle = -0.75f;
        else if (fwd < des - 0.16f) throttle = 0.7f;
        else throttle = 0.05f;
        return;
    }

    float aim = std::atan2((ty - y_) - vy_ * 0.35f, (tx - x_) - vx_ * 0.35f);
    float err = wrap(aim - heading_);
    steer = std::clamp(err / 0.38f, -1.f, 1.f);
    steer += std::clamp(lat / 6.5f, -0.28f, 0.28f);
    steer = std::clamp(steer, -1.f, 1.f);
    float ad = std::fabs(err);
    if (ad > 0.48f && sp > 4.6f) throttle = -0.45f;
    else if (ad > 0.95f) throttle = 0.32f;
    else if (ad > 0.38f) throttle = 0.62f;
    else throttle = 1.f;

    if (leg_ >= 3) {
        float cap = 6.8f;
        if (wp_ >= kWayLast - 1) cap = 3.2f;
        else if (wp_ >= kWayLast - 2) cap = 4.6f;
        if (sp > cap + 0.2f) throttle = -0.65f;
        else if (sp > cap) throttle = std::min(throttle, 0.f);
        else throttle = std::min(throttle, 0.8f);
    } else if (sp > kMaxPace - 0.25f) {
        throttle = std::min(throttle, 0.1f);
    }
}

void Game::physics(float dt, float steer, float throttle) {
    float sp = std::hypot(vx_, vy_);
    float auth = std::clamp(0.34f + sp / 11.f, 0.34f, 1.f);
    float yawCmd = steer * 1.85f * auth;
    yaw_ += (yawCmd - yaw_) * (1.f - std::exp(-5.f * dt));
    heading_ = wrap(heading_ + yaw_ * dt);

    float c = std::cos(heading_), s = std::sin(heading_);
    float fwd = vx_ * c + vy_ * s;
    float lat = vx_ * -s + vy_ * c;
    if (throttle > 0.05f) fwd += throttle * 14.f * dt;
    else if (throttle < -0.05f && fwd < 0.5f) fwd += throttle * 5.2f * dt;
    float drag = (throttle < -0.05f) ? 5.1f : (throttle > 0.05f ? 0.28f : 0.2f);
    float grip = (throttle < -0.05f) ? 6.2f : 4.1f;
    fwd *= std::exp(-drag * dt);
    lat *= std::exp(-grip * dt);
    fwd = std::clamp(fwd, -2.8f, kMaxPace);
    vx_ = fwd * c + lat * -s;
    vy_ = fwd * s + lat * c;
    sp = std::hypot(vx_, vy_);
    if (sp > kMaxPace + 0.5f) {
        vx_ *= (kMaxPace + 0.5f) / sp;
        vy_ *= (kMaxPace + 0.5f) / sp;
    }
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
                sys_->apu.noiseBurst(0.28f, 220.f, 0.12f);
                sys_->rumble(0.3f, 0.1f, 70);
                thumpT_ = 0.3f;
            }
        }
    };
    if (leg_ < 3) bump(kMarks[leg_].x, kMarks[leg_].y, 8.f);

    if (y_ < kBackY && std::fabs(x_) <= kMouth) {
        y_ = kBackY;
        if (vy_ < 0.f) vy_ = 0.f;
        vx_ *= 0.45f;
    }
    if (y_ > 30.f && y_ < 64.f) {
        if (x_ > kMouth && x_ < kMouth + 16.f) {
            x_ = kMouth;
            if (vx_ > 0.f) vx_ = 0.f;
        }
        if (x_ < -kMouth && x_ > -kMouth - 16.f) {
            x_ = -kMouth;
            if (vx_ < 0.f) vx_ = 0.f;
        }
    }
    if (y_ < kShoreY && std::fabs(x_) > kMouth + 3.f) {
        y_ = kShoreY;
        if (vy_ < 0.f) vy_ = 0.f;
        vx_ *= 0.4f;
        if (leg_ >= 3 && mode_ == Mode::Run) fail("missed the end");
    }
    if (y_ > kOtherY1 - 8.f && std::fabs(x_ - kOtherX) < kOtherHalf) {
        y_ = kOtherY1 - 8.f;
        if (vy_ > 0.f) vy_ = 0.f;
    }
    if (x_ < -240.f) {
        x_ = -240.f;
        if (vx_ < 0.f) vx_ = 0.f;
    } else if (x_ > 220.f) {
        x_ = 220.f;
        if (vx_ > 0.f) vx_ = 0.f;
    }
    if (y_ > 410.f) {
        y_ = 410.f;
        if (vy_ > 0.f) vy_ = 0.f;
    }

    sp = std::hypot(vx_, vy_);
    wakeT_ -= dt;
    if (wakeT_ <= 0.f && sp > 1.6f && mode_ == Mode::Run) {
        wakeT_ = 0.06f;
        float bx = x_ - c * 16.f, by = y_ - s * 16.f;
        float px = -s, py = c;
        wake_[puffCursor_] = Puff{bx + px * 5.f, by + py * 5.f, 1.f};
        puffCursor_ = (puffCursor_ + 1) % 18;
        wake_[puffCursor_] = Puff{bx - px * 5.f, by - py * 5.f, 1.f};
        puffCursor_ = (puffCursor_ + 1) % 18;
    }
    for (Puff& p : wake_)
        if (p.life > 0.f) p.life -= dt * 0.85f;

    if (bot_ && mode_ == Mode::Run) {
        stuckT_ += dt;
        if (stuckT_ > 2.4f) {
            float moved = std::hypot(x_ - stuckX_, y_ - stuckY_);
            stuckX_ = x_;
            stuckY_ = y_;
            stuckT_ = 0;
            bool docking = leg_ >= 3 && y_ < 150.f && std::fabs(x_) < 50.f;
            if (moved < 2.2f && !won_) {
                if (docking && std::fabs(x_) > kWinX && y_ < 100.f) {
                    vx_ = -std::copysign(2.f, x_);
                    vy_ = -0.3f;
                    heading_ = kSouth;
                    yaw_ = 0;
                } else if (!docking) {
                    const Way& w = kWay[std::clamp(wp_, 0, kWayLast)];
                    heading_ = std::atan2(w.y - y_, w.x - x_);
                    float hc = std::cos(heading_), hs = std::sin(heading_);
                    vx_ = hc * 2.6f;
                    vy_ = hs * 2.6f;
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
        hornT_ = 0.1f;
        sys_->rumble(0.18f, 0.35f, 70);
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
    if (std::hypot(vx_, vy_) > kStop) return false;
    float south = std::fabs(wrap(heading_ - kSouth));
    float north = std::fabs(wrap(heading_ - kNorth));
    return south < kAim || north < kAim;
}

bool Game::inOther() const {
    return std::fabs(x_ - kOtherX) <= kOtherHalf && y_ >= kOtherY0 && y_ <= kOtherY1;
}

void Game::finish() {
    if (mode_ != Mode::Run || leg_ < 3 || !inSlip()) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    vx_ = vy_ = yaw_ = 0;
    throttle_ = 0;
    chime(5);
    sys_->setLight(40, 160, 80);
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
    blip(90.f);
    sys_->setLight(170, 40, 30);
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.05f);
    tone0_ = freq;
}

void Game::horn() {
    hornT_ = 0.26f;
    sys_->apu.tone(2, 110.f, 0.07f);
}

void Game::chime(int notes) {
    chimeN_ = notes;
    chimeStep_ = 0;
    chimeT_ = 0.01f;
}

void Game::audio(float dt) {
    if (hornT_ > 0.f) {
        hornT_ -= dt;
        sys_->apu.tone(2, 110.f, hornT_ > 0.f ? 0.06f : 0.f);
    }
    float eng = (mode_ == Mode::Run) ? std::clamp(std::fabs(throttle_) * 0.035f + speed() * 0.0035f, 0.f, 0.055f) : 0.f;
    float hz = 62.f + std::max(throttle_, 0.f) * 36.f + speed() * 1.8f;
    sys_->apu.tone(0, hz, eng);
    if (chimeN_ > 0) {
        chimeT_ -= dt;
        if (chimeT_ <= 0.f) {
            float notes[] = {440.f, 554.f, 659.f, 880.f, 1046.f};
            int i = std::min(chimeStep_, 4);
            sys_->apu.tone(1, notes[i], 0.055f);
            chimeT_ = 0.14f;
            if (++chimeStep_ >= chimeN_) chimeN_ = 0;
        }
    }
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (leg_ >= 3) return 3;
    if (leg_ >= 1) return 2;
    if (mode_ == Mode::Run || mode_ == Mode::Pause) return 1;
    return 0;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_TURBO)) {
            begin();
            mode_ = Mode::Run;
            zoom_ = kPlayZoom;
            camX_ = x_;
            camY_ = y_;
            blip(240.f);
            sys.setLight(40, 90, 140);
        } else if (pad.pressed(gs::BTN_MODE)) {
            sys.quit();
        }
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(180.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        } else {
            raceTime_ += kDt;
            float steer = 0, throttle = 0;
            if (bot_) pilot(steer, throttle);
            else controls(steer, throttle);
            throttle_ = throttle;
            physics(kDt, steer, throttle);
            if (mode_ == Mode::Run) {
                scoreMarks();
                guide();
                float sp = std::hypot(vx_, vy_);
                if (inOther() && sp < 0.65f) {
                    otherT_ += kDt;
                    if (otherT_ > 0.45f) fail("wrong dock");
                } else {
                    otherT_ = 0;
                }
                if (mode_ == Mode::Run && leg_ >= 3 && y_ < 58.f && y_ > 26.f && std::fabs(x_) > 18.f &&
                    std::fabs(x_) < 60.f && sp < 0.5f) {
                    sideT_ += kDt;
                    if (sideT_ > 1.f) fail("missed the end");
                } else {
                    sideT_ = 0;
                }
                if (mode_ == Mode::Run && raceTime_ > kLimit) fail("the leg ran out");
                if (mode_ == Mode::Run) finish();
            }
        }
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
            blip(240.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        }
    }

    if (mode_ == Mode::Title) {
        camX_ = kTitleCamX + std::sin(t_ * 0.16f) * 6.f;
        camY_ = kTitleCamY + std::cos(t_ * 0.12f) * 4.f;
        zoom_ = kTitleZoom;
    } else {
        float lead = (mode_ == Mode::Run) ? (leg_ >= 3 ? 8.f : 14.f) : 0.f;
        float sp = std::hypot(vx_, vy_);
        float gx = x_, gy = y_;
        if (sp > 0.4f) {
            gx += vx_ / sp * lead;
            gy += vy_ / sp * lead;
        } else {
            gx += std::cos(heading_) * lead;
            gy += std::sin(heading_) * lead;
        }
        float k = 1.f - std::exp(-kDt * 3.4f);
        camX_ += (gx - camX_) * k;
        camY_ += (gy - camY_) * k;
        zoom_ += (kPlayZoom - zoom_) * k;
    }
    audio(kDt);
    if (tracing() && bot_ && mode_ == Mode::Run && int(t_) != int(t_ - kDt)) {
        std::fprintf(stderr, "t %.0f leg %d wp %d x %.0f y %.0f hdg %.2f spd %.1f armed %d\n", t_, leg_, wp_, x_, y_,
                     heading_, std::hypot(vx_, vy_), armed_ ? 1 : 0);
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
        hudC(21, "ARROWS STEER THE METRO", PAL_HUD);
        hudC(22, "UP DRIVE    DOWN REVERSE", PAL_HUD);
        hudC(23, "Z DRIVE   X REVERSE   C HORN", PAL_BANNER);
        hudC(24, "LEAVE EACH BUOY TO PORT", PAL_WIN);
        hudC(25, "THEN STOP IN THIS DOCK", PAL_BANNER);
        hudC(26, "MISSING THE END FAILS THE LEG", PAL_ALERT);
        if ((int(t_ * 2.f) & 1) == 0) hudC(27, "ENTER", PAL_WIN);
        return;
    }
    hud(1, 0, "S3 METRO BUOY", PAL_BANNER);
    int sec = int(raceTime_);
    std::snprintf(buf, sizeof buf, "%d:%02d", sec / 60, sec % 60);
    hud(34, 0, buf, PAL_HUD);
    if (mode_ == Mode::Pause) {
        hudC(18, "ENTER CONTINUES", PAL_HUD);
        hudC(19, "ESC BACK TO THE PIER", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Win) {
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
    hud(1, 2, orderName(throttle_), throttle_ < -0.05f ? PAL_ALERT : PAL_HUD);
    std::snprintf(buf, sizeof buf, "WAY %.1f", std::hypot(vx_, vy_));
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
        if (box && std::hypot(vx_, vy_) > kStop) {
            hint = "BRAKE TO FINISH THE LEG";
            hpal = PAL_ALERT;
        } else if (box) {
            hint = "POINT ALONG THE PIER";
            hpal = PAL_BANNER;
        } else {
            hint = "THE SAME DOCK IS THE END";
            hpal = PAL_BANNER;
        }
    } else if (raceTime_ < 7.f) {
        hint = "THE METRO TURNS WIDE";
    }
    if (hint) hud(1, 27, hint, hpal);
}

void Game::worldToScreen(float wx, float wy, float& sx, float& sy) const {
    sx = 160.f + (wx - camX_) * zoom_;
    sy = 112.f - (wy - camY_) * zoom_;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal) {
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
    sys_->vdp.sprite(s);
}

void Game::place(const gs::Mipped& m, float wx, float wy, float worldH, int pal, float minPx) {
    float sx, sy;
    worldToScreen(wx, wy, sx, sy);
    float h = worldH * zoom_;
    if (h < minPx) h = minPx;
    spr(m, sx, sy, h, pal);
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
    const uint16_t quay = gs::rgb4(5, 5, 6);
    const uint16_t city = gs::rgb4(4, 5, 7);
    const uint16_t harbour = gs::rgb4(2, 7, 9);
    const uint16_t deep = gs::rgb4(1, 4, 7);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (112.f - y) / std::max(zoom_, 0.05f);
        uint16_t c;
        if (wy < 8.f) c = lerpC(city, quay, std::clamp((wy + 30.f) / 38.f, 0.f, 1.f));
        else if (wy < 80.f) c = lerpC(quay, harbour, std::clamp((wy - 8.f) / 70.f, 0.f, 1.f));
        else c = lerpC(harbour, deep, std::clamp((wy - 80.f) / 260.f, 0.f, 1.f));
        v.lineBackdrop[y] = c;
        v.lineFog[y] = 0;
        v.road[y].on = false;
        v.B.hscroll[y] = int16_t(std::sin(y * 0.05f + t_ * 0.9f) * 1.4f + camX_ * 0.12f);
        v.B.vscroll[y] = int16_t(t_ * 4.f);
    }

    auto banner = [&](const gs::Mipped& m, float x, float y, int pal) { spr(m, x, y, float(m.h), pal); };
    if (mode_ == Mode::Title) {
        banner(art_.title, 160, 20, PAL_BANNER);
        banner(art_.round, 160, 48, PAL_BANNER);
    } else if (mode_ == Mode::Pause) {
        banner(art_.paused, 160, 96, PAL_BANNER);
    } else if (mode_ == Mode::Win) {
        banner(art_.home, 160, 68, PAL_WIN);
        banner(art_.made, 160, 100, PAL_WIN);
    } else if (mode_ == Mode::Fail) {
        banner(std::strcmp(why_, "wrong dock") == 0 ? art_.wrong : art_.missed, 160, 78, PAL_ALERT);
    }

    place(art_.pier, 0.f, 18.f, 30.f, PAL_PIER, 6.f);
    place(art_.canopy, -28.f, 8.f, 18.f, PAL_PIER, 4.f);
    place(art_.flag, 18.f, 14.f, 14.f, PAL_ALERT, 3.f);
    place(art_.lamp, -10.f, 34.f, 10.f, PAL_LAMP, 2.f);
    place(art_.lamp, 10.f, 34.f, 10.f, PAL_LAMP, 2.f);
    for (int i = 0; i < 4; i++) {
        place(art_.block, -70.f - i * 22.f, 4.f, 18.f, PAL_CITY, 3.f);
        place(art_.block, 54.f + i * 22.f, 2.f, 16.f, PAL_CITY, 3.f);
    }
    place(art_.canopy, kOtherX, kOtherY1 - 6.f, 26.f, PAL_OTHER, 5.f);
    place(art_.crate, kOtherX - 12.f, kOtherY1 - 4.f, 8.f, PAL_CRATE, 2.f);

    const gs::Mipped* balls[3] = {&art_.amber, &art_.teal, &art_.violet};
    for (int i = 0; i < 3; i++) {
        float bob = std::sin(t_ * 1.6f + i * 1.3f) * 1.4f;
        place(*balls[i], kMarks[i].x, kMarks[i].y + bob, 16.f, kMarks[i].pal, 4.f);
        place(art_.ring, kMarks[i].x, kMarks[i].y, 7.f, kMarks[i].pal, 2.f);
    }

    int flap = (int(t_ * 5.f) & 1);
    place(art_.gull[flap], 50.f + std::sin(t_ * 0.35f) * 24.f, 210.f, 8.f, PAL_GULL, 2.f);
    place(art_.gull[1 - flap], -80.f, 100.f + std::sin(t_ * 0.55f) * 8.f, 7.f, PAL_GULL, 2.f);

    for (const Puff& p : wake_) {
        if (p.life <= 0.f) continue;
        place(art_.wake, p.x, p.y, 4.f + (1.f - p.life) * 7.f, PAL_WAKE, 1.f);
    }

    place(art_.hull[hullFrame()], x_, y_, mode_ == Mode::Title ? 20.f : 28.f, PAL_HULL, 8.f);

    if (mode_ == Mode::Run || mode_ == Mode::Pause) {
        float tx = (leg_ < 3) ? kMarks[leg_].x : 0.f;
        float ty = (leg_ < 3) ? kMarks[leg_].y : 50.f;
        float sx, sy;
        worldToScreen(tx, ty, sx, sy);
        if (sx < 16.f || sx > 304.f || sy < 16.f || sy > 208.f) {
            float dx = std::clamp(sx, 20.f, 300.f);
            float dy = std::clamp(sy, 20.f, 200.f);
            spr(art_.pin, dx, dy, 12.f, leg_ < 3 ? kMarks[leg_].pal : PAL_WIN);
        }
    }

    drawHud();
}

}  // namespace metrobuoy

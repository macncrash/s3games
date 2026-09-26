#include "game/sled.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace sled {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;
constexpr float kNorth = 1.5707963f;
constexpr float kSouth = -1.5707963f;
constexpr float kMaxPace = 15.5f;
constexpr float kPlayZoom = 1.48f;
constexpr float kTitleZoom = 0.56f;
constexpr float kTitleCamX = -10.f;
constexpr float kTitleCamY = 168.f;
constexpr float kStartX = 0.f;
constexpr float kStartY = 54.f;
constexpr float kStartH = kNorth;
constexpr float kMouth = 18.f;
constexpr float kShoreY = 20.f;
constexpr float kBackY = 30.f;
constexpr float kWinX = 12.f;
constexpr float kWinY0 = 30.f;
constexpr float kWinY1 = 72.f;
constexpr float kStop = 1.15f;
constexpr float kAim = 1.15f;
constexpr float kArm = -18.f;
constexpr float kPass = 14.f;
constexpr float kSideMin = 16.f;
constexpr float kSideMax = 120.f;
constexpr float kOtherX = 8.f;
constexpr float kOtherY0 = 300.f;
constexpr float kOtherY1 = 340.f;
constexpr float kOtherHalf = 22.f;
constexpr float kLimit = 115.f;

struct Mark {
    float x, y;
    float dx, dy;
    float rx, ry;
    const char* name;
    int pal;
    bool can;
};

struct Way {
    float x, y, reach;
    int needLeg;
};

// Counterclockwise. Each mark stays to port, so the sled passes on its right.
const Mark kMarks[3] = {
    {70.f, 130.f, 0.f, 1.f, 1.f, 0.f, "RED 1", PAL_NUN, false},
    {10.f, 248.f, -1.f, 0.f, 0.f, -1.f, "GREEN 2", PAL_CAN, true},
    {-96.f, 168.f, 0.f, -1.f, -1.f, 0.f, "RED 3", PAL_NUN, false},
};

const Way kWay[] = {
    {0.f, 100.f, 16.f, 0},
    {124.f, 88.f, 22.f, 0},
    {124.f, 176.f, 22.f, 0},
    {124.f, 186.f, 20.f, 1},
    {-24.f, 186.f, 22.f, 1},
    {-156.f, 214.f, 24.f, 2},
    {-156.f, 112.f, 22.f, 2},
    {0.f, 150.f, 20.f, 3},
    {0.f, 104.f, 14.f, 3},
    {0.f, 48.f, 9.f, 3},
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
    if (throttle > 0.55f) return "MUSH";
    if (throttle > 0.08f) return "EASY";
    if (throttle < -0.45f) return "WHOA";
    if (throttle < -0.05f) return "DRAG";
    return "COAST";
}

bool tracing() {
    static int on = -1;
    if (on < 0) on = std::getenv("SLED_TRACE") != nullptr;
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
    sprayT_ = 0;
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
    for (Puff& p : spray_) p.life = 0;
    for (int i = 0; i < 18; i++) {
        flakes_[i].x = float((i * 47) % 320);
        flakes_[i].y = float((i * 29) % 224);
        flakes_[i].v = 14.f + float(i % 5) * 6.f;
        flakes_[i].w = 2.f + float(i % 3);
    }
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
    sys.apu.setMaster(0.72f);
    sys.apu.setEcho(0.18f, 0.22f, 0.10f);
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
    if (p.pressed(gs::BTN_C) || p.pressed(gs::BTN_X) || p.pressed(gs::BTN_Y) || p.pressed(gs::BTN_Z)) yip();
}

void Game::pilot(float& steer, float& throttle) {
    steer = 0;
    throttle = 0;
    const Way& w = kWay[std::clamp(wp_, 0, kWayLast)];
    float tx = w.x, ty = w.y;
    bool dock = wp_ >= kWayLast && leg_ >= 3;
    if (leg_ < 3 && !armed_) {
        const Mark& m = kMarks[leg_];
        tx = m.x - m.dx * 76.f + m.rx * 48.f;
        ty = m.y - m.dy * 76.f + m.ry * 48.f;
        dock = false;
    }

    float c = std::cos(heading_), s = std::sin(heading_);
    float lat = vx_ * -s + vy_ * c;
    float fwd = vx_ * c + vy_ * s;
    float sp = std::hypot(vx_, vy_);

    if (dock) {
        float gx = 0.f, gy = 42.f;
        if (std::fabs(x_) > 8.f && y_ > 70.f) gy = y_ - 10.f;
        float dx = gx - x_, dy = gy - y_;
        float dist = std::hypot(dx, dy);
        float want = std::clamp(dist * 0.085f, 0.f, 3.6f);
        if (std::fabs(x_) > 10.f) want = std::min(want, 2.4f);
        if (y_ < 58.f && std::fabs(x_) <= kWinX) want = 0.f;
        float dvx = 0.f, dvy = 0.f;
        if (dist > 0.8f && want > 0.05f) {
            dvx = dx / dist * want;
            dvy = dy / dist * want;
        }
        float aim = (want > 0.35f) ? std::atan2(dvy, dvx) : (kSouth - std::clamp(x_ * 0.09f, -0.45f, 0.45f));
        float err = wrap(aim - heading_);
        steer = std::clamp(err / 0.28f, -1.f, 1.f);
        steer += std::clamp(lat / 8.f, -0.35f, 0.35f);
        steer = std::clamp(steer, -1.f, 1.f);
        float des = dvx * c + dvy * s;
        if (std::fabs(err) > 0.85f) throttle = (sp > 2.2f) ? -0.7f : 0.32f;
        else if (want < 0.05f) throttle = -1.f;
        else if (fwd > des + 0.28f) throttle = -0.85f;
        else if (fwd < des - 0.22f) throttle = 0.7f;
        else throttle = 0.08f;
        return;
    }

    float aim = std::atan2((ty - y_) - vy_ * 0.28f, (tx - x_) - vx_ * 0.28f);
    float err = wrap(aim - heading_);
    steer = std::clamp(err / 0.32f, -1.f, 1.f);
    steer += std::clamp(lat / 9.f, -0.3f, 0.3f);
    steer = std::clamp(steer, -1.f, 1.f);
    float ad = std::fabs(err);
    if (ad > 0.55f && sp > 6.5f) throttle = -0.55f;
    else if (ad > 1.0f) throttle = 0.3f;
    else if (ad > 0.45f) throttle = 0.62f;
    else throttle = 1.f;

    if (leg_ >= 3) {
        float cap = 10.f;
        if (wp_ >= kWayLast - 1) cap = 5.2f;
        else if (wp_ >= kWayLast - 2) cap = 7.5f;
        if (sp > cap + 0.25f) throttle = -0.75f;
        else if (sp > cap) throttle = std::min(throttle, 0.f);
        else throttle = std::min(throttle, 0.8f);
    } else if (sp > kMaxPace - 0.4f) {
        throttle = std::min(throttle, 0.15f);
    }
}

void Game::puffAt(float x, float y) {
    spray_[puffCursor_] = Puff{x, y, 1.f};
    puffCursor_ = (puffCursor_ + 1) % 20;
}

void Game::physics(float dt, float steer, float throttle) {
    float sp = std::hypot(vx_, vy_);
    float auth = std::clamp(0.38f + sp / 12.f, 0.38f, 1.1f);
    float yawCmd = steer * 2.55f * auth;
    yaw_ += (yawCmd - yaw_) * (1.f - std::exp(-7.f * dt));
    heading_ = wrap(heading_ + yaw_ * dt);

    float c = std::cos(heading_), s = std::sin(heading_);
    float fwd = vx_ * c + vy_ * s;
    float lat = vx_ * -s + vy_ * c;
    if (throttle > 0.05f) fwd += throttle * 26.f * dt;
    else if (throttle < -0.05f && fwd < 0.45f) fwd += throttle * 5.f * dt;
    float drag = (throttle < -0.05f) ? 5.6f : (throttle > 0.05f ? 0.28f : 0.2f);
    float grip = (throttle < -0.05f) ? 7.5f : (throttle > 0.2f ? 2.4f : 2.15f);
    fwd *= std::exp(-drag * dt);
    lat *= std::exp(-grip * dt);
    fwd = std::clamp(fwd, -3.2f, kMaxPace);
    vx_ = fwd * c + lat * -s;
    vy_ = fwd * s + lat * c;
    sp = std::hypot(vx_, vy_);
    if (sp > kMaxPace + 1.f) {
        vx_ *= (kMaxPace + 1.f) / sp;
        vy_ *= (kMaxPace + 1.f) / sp;
    }
    x_ += vx_ * dt;
    y_ += vy_ * dt;

    auto bump = [&](float cx, float cy, float rad) {
        float dx = x_ - cx, dy = y_ - cy;
        float d = std::hypot(dx, dy);
        if (d < rad && d > 0.001f) {
            x_ = cx + dx / d * rad;
            y_ = cy + dy / d * rad;
            vx_ *= 0.45f;
            vy_ *= 0.45f;
            yaw_ *= 0.3f;
            if (thumpT_ <= 0.f) {
                sys_->apu.noiseBurst(0.28f, 240.f, 0.12f);
                sys_->rumble(0.3f, 0.1f, 60);
                thumpT_ = 0.3f;
            }
        }
    };
    if (leg_ < 3) bump(kMarks[leg_].x, kMarks[leg_].y, 8.f);

    if (y_ < kBackY && std::fabs(x_) <= kMouth) {
        y_ = kBackY;
        if (vy_ < 0.f) vy_ = 0.f;
        vx_ *= 0.55f;
    }
    if (y_ > 28.f && y_ < 58.f) {
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
        vx_ *= 0.5f;
        if (leg_ >= 3 && mode_ == Mode::Run) fail("missed the end");
    }
    if (y_ > kOtherY1 - 6.f && std::fabs(x_ - kOtherX) < kOtherHalf) {
        y_ = kOtherY1 - 6.f;
        if (vy_ > 0.f) vy_ = 0.f;
    }
    if (x_ < -210.f) {
        x_ = -210.f;
        if (vx_ < 0.f) vx_ = 0.f;
    } else if (x_ > 190.f) {
        x_ = 190.f;
        if (vx_ > 0.f) vx_ = 0.f;
    }
    if (y_ > 360.f) {
        y_ = 360.f;
        if (vy_ > 0.f) vy_ = 0.f;
    }

    sp = std::hypot(vx_, vy_);
    sprayT_ -= dt;
    if (sprayT_ <= 0.f && sp > 2.4f && mode_ == Mode::Run) {
        sprayT_ = 0.05f;
        float bx = x_ - c * 7.f, by = y_ - s * 7.f;
        float px = -s, py = c;
        puffAt(bx + px * 3.2f, by + py * 3.2f);
        puffAt(bx - px * 3.2f, by - py * 3.2f);
    }
    for (Puff& p : spray_)
        if (p.life > 0.f) p.life -= dt;

    if (bot_ && mode_ == Mode::Run) {
        stuckT_ += dt;
        if (stuckT_ > 2.2f) {
            float moved = std::hypot(x_ - stuckX_, y_ - stuckY_);
            stuckX_ = x_;
            stuckY_ = y_;
            stuckT_ = 0;
            bool docking = leg_ >= 3 && y_ < 130.f && std::fabs(x_) < 40.f;
            if (moved < 3.2f && !won_) {
                if (docking && std::fabs(x_) > kWinX && y_ < 90.f) {
                    vx_ = -std::copysign(2.4f, x_);
                    vy_ = -0.6f;
                    heading_ = kSouth;
                    yaw_ = 0;
                } else if (!docking) {
                    const Way& w = kWay[std::clamp(wp_, 0, kWayLast)];
                    heading_ = std::atan2(w.y - y_, w.x - x_);
                    float hc = std::cos(heading_), hs = std::sin(heading_);
                    vx_ = hc * 3.2f;
                    vy_ = hs * 3.2f;
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
    if (drive < 1.1f) return;
    if (lateral > kSideMin && lateral < kSideMax) {
        leg_++;
        armed_ = false;
        wrong_ = false;
        chime(std::min(leg_, 3));
        yipT_ = 0.16f;
        sys_->rumble(0.25f, 0.45f, 70);
        for (int i = 0; i < 8; i++) {
            float a = i * kTau / 8.f;
            puffAt(m.x + std::cos(a) * 12.f, m.y + std::sin(a) * 12.f);
        }
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
    sys_->setLight(40, 170, 80);
    sys_->rumble(0.2f, 0.5f, 140);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    std::snprintf(why_, sizeof why_, "%s", why);
    vx_ = vy_ = yaw_ = 0;
    throttle_ = 0;
    blip(96.f);
    sys_->setLight(180, 40, 30);
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.05f);
    tone0_ = 0.08f;
}

void Game::yip() {
    sys_->apu.noiseBurst(0.2f, 1500.f, 0.07f);
    sys_->apu.tone(1, 540.f, 0.04f);
    yipT_ = 0.12f;
}

void Game::chime(int notes) {
    chimeN_ = std::clamp(notes, 1, 5);
    chimeStep_ = 0;
    chimeT_ = 0;
}

void Game::audio(float dt) {
    float sp = std::hypot(vx_, vy_);
    if (mode_ == Mode::Run) {
        sys_->apu.noise(0.012f + sp * 0.0032f + std::fabs(throttle_) * 0.01f, 640.f + sp * 70.f, true);
        if (yipT_ <= 0.f) sys_->apu.tone(2, 46.f + sp * 2.5f, 0.01f + sp * 0.001f);
    } else {
        sys_->apu.noise(0.008f, 380.f, false);
        sys_->apu.tone(2, 0, 0);
    }
    if (yipT_ > 0.f) {
        yipT_ -= dt;
        if (yipT_ <= 0.f) sys_->apu.tone(1, 0, 0);
    }
    if (tone0_ > 0.f) {
        tone0_ -= dt;
        if (tone0_ <= 0.f) sys_->apu.tone(0, 0, 0);
    }
    if (chimeN_ > 0) {
        chimeT_ -= dt;
        if (chimeT_ <= 0.f) {
            static const float notes[] = {349.f, 440.f, 523.f, 698.f, 880.f};
            int n = std::min(chimeStep_, 4);
            sys_->apu.tone(0, notes[n], 0.05f);
            tone0_ = 0.14f;
            chimeT_ = 0.15f;
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
    for (Flake& f : flakes_) {
        f.y += f.v * kDt;
        f.x += std::sin(t_ * 0.7f + f.y * 0.02f) * 8.f * kDt;
        if (f.y > 230.f) f.y = -6.f;
        if (f.x > 330.f) f.x = -8.f;
        if (f.x < -10.f) f.x = 328.f;
    }

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_TURBO)) {
            begin();
            mode_ = Mode::Run;
            zoom_ = kPlayZoom;
            camX_ = x_;
            camY_ = y_;
            blip(620.f);
            sys.setLight(160, 190, 220);
        } else if (pad.pressed(gs::BTN_MODE)) {
            sys.quit();
        }
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(300.f);
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
                if (inOther() && sp < 0.95f) {
                    otherT_ += kDt;
                    if (otherT_ > 0.45f) fail("wrong dock");
                } else {
                    otherT_ = 0;
                }
                if (mode_ == Mode::Run && leg_ >= 3 && y_ < 54.f && y_ > 24.f && std::fabs(x_) > 14.f &&
                    std::fabs(x_) < 50.f && sp < 0.65f) {
                    sideT_ += kDt;
                    if (sideT_ > 1.05f) fail("missed the end");
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
            blip(620.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        }
    }

    if (mode_ == Mode::Title) {
        camX_ = kTitleCamX + std::sin(t_ * 0.16f) * 4.f;
        camY_ = kTitleCamY + std::cos(t_ * 0.13f) * 3.f;
        zoom_ = kTitleZoom;
    } else {
        float lead = (mode_ == Mode::Run) ? (leg_ >= 3 ? 7.f : 13.f) : 0.f;
        float sp = std::hypot(vx_, vy_);
        float gx = x_, gy = y_;
        if (sp > 0.6f) {
            gx += vx_ / sp * lead;
            gy += vy_ / sp * lead;
        } else {
            gx += std::cos(heading_) * lead;
            gy += std::sin(heading_) * lead;
        }
        float k = 1.f - std::exp(-kDt * 4.2f);
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
        hudC(21, "ARROWS STEER", PAL_HUD);
        hudC(22, "UP MUSH    DOWN WHOA", PAL_HUD);
        hudC(23, "Z MUSH   X WHOA   C YIP", PAL_BANNER);
        hudC(24, "LEAVE EACH BUOY TO PORT", PAL_WIN);
        hudC(25, "THEN STOP IN THIS DOCK", PAL_BANNER);
        hudC(26, "MISSING THE END FAILS THE LEG", PAL_ALERT);
        if ((int(t_ * 2.f) & 1) == 0) hudC(27, "ENTER", PAL_WIN);
        return;
    }
    hud(1, 0, "S3 SLED BUOY", PAL_BANNER);
    int sec = int(raceTime_);
    std::snprintf(buf, sizeof buf, "%d:%02d", sec / 60, sec % 60);
    hud(34, 0, buf, PAL_HUD);
    if (mode_ == Mode::Pause) {
        hudC(18, "ENTER CONTINUES", PAL_HUD);
        hudC(19, "ESC BACK TO THE BAY", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Win) {
        std::snprintf(buf, sizeof buf, "TIME %d:%02d", sec / 60, sec % 60);
        hudC(16, buf, PAL_HUD);
        if (!bot_) hudC(18, "ENTER MUSHES AGAIN", PAL_HUD);
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
    std::snprintf(buf, sizeof buf, "PACE %.1f", std::hypot(vx_, vy_));
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
            hint = "EASE OFF TO FINISH";
            hpal = PAL_ALERT;
        } else if (box) {
            hint = "POINT ALONG THE DOCK";
            hpal = PAL_BANNER;
        } else {
            hint = "THE SAME DOCK IS THE END";
            hpal = PAL_BANNER;
        }
    } else if (raceTime_ < 7.f) {
        hint = "MUSH, THEN LET THE RUNNERS BITE";
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

int Game::sledFrame() const {
    float u = std::fmod(heading_, kTau);
    if (u < 0.f) u += kTau;
    int i = int(std::lround(u / kTau * 16.f)) % 16;
    if (i < 0) i += 16;
    return i;
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = true;
    v.hudEnabled = true;
    const uint16_t snow = gs::rgb4(12, 13, 14);
    const uint16_t shore = gs::rgb4(10, 12, 13);
    const uint16_t ice = gs::rgb4(8, 12, 13);
    const uint16_t far = gs::rgb4(5, 8, 12);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (112.f - y) / std::max(zoom_, 0.05f);
        uint16_t c;
        if (wy < 8.f) c = lerpC(snow, shore, std::clamp((wy + 30.f) / 38.f, 0.f, 1.f));
        else if (wy < 80.f) c = lerpC(shore, ice, std::clamp((wy - 8.f) / 72.f, 0.f, 1.f));
        else c = lerpC(ice, far, std::clamp((wy - 80.f) / 260.f, 0.f, 1.f));
        v.lineBackdrop[y] = c;
        v.lineFog[y] = 0;
        v.road[y].on = false;
        float wob = std::sin(y * 0.05f + t_ * 0.7f) * 1.4f;
        v.B.hscroll[y] = int16_t(wob);
        v.B.vscroll[y] = int16_t(t_ * 5.f);
    }

    auto banner = [&](const gs::Mipped& m, float x, float y, int pal) { spr(m, x, y, float(m.h), pal, false); };
    bool title = mode_ == Mode::Title;
    if (title) {
        banner(art_.title, 160, 18, PAL_BANNER);
        banner(art_.round, 160, 42, PAL_BANNER);
    } else if (mode_ == Mode::Pause) {
        banner(art_.paused, 160, 96, PAL_BANNER);
    } else if (mode_ == Mode::Win) {
        banner(art_.same, 160, 70, PAL_WIN);
        banner(art_.made, 160, 100, PAL_WIN);
    } else if (mode_ == Mode::Fail) {
        banner(std::strcmp(why_, "wrong dock") == 0 ? art_.wrong : art_.missed, 160, 78, PAL_ALERT);
    }

    if (mode_ == Mode::Run || mode_ == Mode::Pause) {
        float tx = (leg_ < 3) ? kMarks[leg_].x : 0.f;
        float ty = (leg_ < 3) ? kMarks[leg_].y : 46.f;
        float sx, sy;
        worldToScreen(tx, ty, sx, sy);
        if (sx < 14.f || sx > 306.f || sy < 14.f || sy > 210.f) {
            float dx = sx - 160.f, dy = sy - 112.f;
            float k = 1.f;
            float ax = std::fabs(dx), ay = std::fabs(dy);
            if (ax > 1.f) k = std::min(k, 148.f / ax);
            if (ay > 1.f) k = std::min(k, 96.f / ay);
            int pal = (leg_ < 3) ? kMarks[leg_].pal : PAL_WIN;
            spr(art_.pin, 160.f + dx * k, 112.f + dy * k, 12.f, pal, false);
        }
    }

    if (!title) {
        auto chart = [&](float wx, float wy, float& sx, float& sy) {
            sx = 286.f + (wx + 20.f) * 0.13f;
            sy = 70.f - (wy - 160.f) * 0.13f;
        };
        float sx, sy;
        chart(x_, y_, sx, sy);
        spr(art_.dot, sx, sy, 5.f, PAL_BANNER, false);
        chart(0.f, 46.f, sx, sy);
        spr(art_.dot, sx, sy, 4.f, PAL_WIN, false);
        chart(kOtherX, 320.f, sx, sy);
        spr(art_.dot, sx, sy, 4.f, PAL_ALERT, false);
        for (int i = 0; i < 3; i++) {
            chart(kMarks[i].x, kMarks[i].y, sx, sy);
            spr(art_.dot, sx, sy, i == leg_ ? 6.f : 4.f, kMarks[i].pal, false);
        }
        spr(art_.panel, 286.f, 70.f, 70.f, PAL_MAP, false);
    }

    for (const Flake& f : flakes_) spr(art_.dot, f.x, f.y, f.w, PAL_SNOW, false);

    float bsx, bsy;
    worldToScreen(x_, y_, bsx, bsy);
    if (std::hypot(vx_, vy_) > 1.f) bsy += std::sin(t_ * 8.f) * 0.5f;
    float sledH = 28.f * zoom_;
    if (title) sledH = std::max(sledH, 28.f);
    const gs::Mipped& hull = art_.sled[sledFrame()];
    spr(hull, bsx, bsy, sledH, PAL_SLED, false);
    spr(hull, bsx + 3.f, bsy + 3.f, sledH, PAL_SLED, true);

    for (const Puff& p : spray_) {
        if (p.life <= 0.f) continue;
        float sx, sy;
        worldToScreen(p.x, p.y, sx, sy);
        spr(art_.spray, sx, sy, 3.f + (1.f - p.life) * 7.f, PAL_SPRAY, false);
    }

    auto lane = [&](const Mark& m, int pal) {
        for (int i = -2; i <= 3; i++) {
            float ox = m.dx * (i * 10.f) + m.rx * 40.f;
            float oy = m.dy * (i * 10.f) + m.ry * 40.f;
            place(art_.dot, m.x + ox, m.y + oy, 2.4f, pal, title ? 2.5f : 0.f);
        }
    };
    if (title) {
        for (const Mark& m : kMarks) lane(m, m.pal);
    } else if (leg_ < 3) {
        lane(kMarks[leg_], kMarks[leg_].pal);
    }

    for (int i = 0; i < 3; i++) {
        bool hot = i == leg_ && leg_ < 3;
        const Mark& m = kMarks[i];
        const gs::Mipped& body = m.can ? art_.can : (i == 2 ? art_.nun3 : art_.nun);
        if (hot || title) place(art_.ring, m.x, m.y, hot ? 54.f : 46.f, m.pal, title ? 14.f : 0.f);
        float pulse = hot ? 1.f + 0.06f * std::sin(t_ * 4.f) : 1.f;
        place(body, m.x, m.y, (hot ? 18.f : 15.f) * pulse, m.pal, title ? 12.f : 0.f);
        if ((hot || title) && ((int(t_ * 3.f + i) & 1) == 0))
            place(art_.lamp, m.x, m.y + 10.f, 4.f, PAL_LAMP, title ? 5.f : 0.f);
    }

    if (leg_ >= 3 || title) {
        for (int i = -2; i <= 2; i++) place(art_.dot, i * 5.f, 36.f, 2.6f, PAL_WIN, title ? 3.f : 0.f);
        if ((int(t_ * 3.f) & 1) == 0 || title) place(art_.lamp, 0.f, 24.f, 5.f, PAL_WIN, title ? 6.f : 0.f);
    }
    place(art_.ring, kOtherX, 318.f, 36.f, PAL_ALERT, title ? 10.f : 0.f);

    if (title) {
        const float rx[] = {0.f, 124.f, 124.f, -24.f, -156.f, -156.f, 0.f, 0.f};
        const float ry[] = {54.f, 88.f, 186.f, 186.f, 214.f, 112.f, 150.f, 48.f};
        for (int i = 0; i < 7; i++) {
            float dx = rx[i + 1] - rx[i], dy = ry[i + 1] - ry[i];
            float len = std::hypot(dx, dy);
            int n = std::max(1, int(len / 26.f));
            for (int s = 1; s < n; s++) {
                float u = float(s) / float(n);
                place(art_.dot, rx[i] + dx * u, ry[i] + dy * u, 2.f, PAL_BANNER, 2.2f);
            }
        }
    }

    int flap = int(t_ * 4.f) & 1;
    place(art_.bird[flap], 36.f + std::sin(t_ * 0.4f) * 30.f, 210.f, 6.f, PAL_BIRD, title ? 6.f : 0.f);
    place(art_.bird[1 - flap], -70.f + std::cos(t_ * 0.33f) * 24.f, 120.f, 5.5f, PAL_BIRD, title ? 5.f : 0.f);

    const float cracks[][2] = {{46.f, 110.f}, {-30.f, 200.f}, {90.f, 210.f}, {-70.f, 96.f}};
    for (const float* c : cracks) place(art_.crack, c[0], c[1], 10.f, PAL_MAP, title ? 4.f : 0.f);
    const float drifts[][2] = {{150.f, 48.f}, {-48.f, 64.f}, {48.f, 300.f}, {-170.f, 48.f}};
    for (const float* d : drifts) place(art_.drift, d[0], d[1], 16.f, PAL_DRIFT, title ? 8.f : 0.f);
    const float trees[][2] = {{176.f, 46.f}, {168.f, 230.f}, {-186.f, 70.f}, {-40.f, 292.f}, {70.f, 36.f}};
    for (const float* tr : trees) place(art_.tree, tr[0], tr[1], 22.f, PAL_TREE, title ? 10.f : 0.f);

    place(art_.flag, -10.f, 18.f, 12.f, PAL_DOCK, title ? 8.f : 0.f);
    place(art_.flag, 12.f, 18.f, 12.f, PAL_DOCK, title ? 8.f : 0.f);
    const float piles[][2] = {{-24.f, 34.f}, {24.f, 34.f}, {-24.f, 52.f}, {24.f, 52.f}};
    for (const float* p : piles) place(art_.pile, p[0], p[1], 10.f, PAL_DOCK, title ? 7.f : 0.f);
    place(art_.shed, -78.f, 6.f, 26.f, PAL_DOCK, title ? 12.f : 0.f);
    for (int i = -5; i <= 5; i++) {
        float qx = i * 32.f;
        if (std::fabs(qx) < 40.f) continue;
        place(art_.quay, qx, 4.f, 22.f, PAL_DOCK, title ? 8.f : 0.f);
    }
    place(art_.shed, kOtherX, 328.f, 24.f, PAL_OTHER, title ? 10.f : 0.f);
    for (int i = -1; i <= 1; i++) place(art_.quay, kOtherX + i * 30.f, 304.f, 20.f, PAL_OTHER, title ? 8.f : 0.f);
    place(art_.lamp, kOtherX - 16.f, 312.f, 5.f, PAL_LAMP, title ? 5.f : 0.f);

    drawHud();
}

}  // namespace sled

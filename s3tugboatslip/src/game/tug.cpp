#include "tug.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace tugslip {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;
constexpr float kNorth = kPi * 0.5f;

constexpr float kStartX = -40.f;
constexpr float kStartY = 34.f;
constexpr float kStartH = 1.00f;

constexpr float kPierIn = 18.f;
constexpr float kMouth = 122.f;
constexpr float kHead = 218.f;
constexpr float kBerthY0 = 172.f;
constexpr float kBerthY1 = 196.f;
constexpr float kBerthX = 6.4f;
constexpr float kPark = (kBerthY0 + kBerthY1) * 0.5f;
constexpr float kGateX = 0.f;
constexpr float kGateY = kMouth - 26.f;

constexpr float kBow = 7.0f;
constexpr float kStern = 6.2f;
constexpr float kBeam = 3.55f;

constexpr float kTide = 78.f;
constexpr float kStop = 0.58f;
constexpr float kHold = 0.42f;
constexpr float kShort = 10.f;
constexpr float kScrape = 2.7f;
constexpr float kMaxAhead = 7.2f;
constexpr float kMaxAstern = 3.5f;

constexpr float kPlayZoom = 1.40f;
constexpr float kTitleZoom = 1.02f;
constexpr float kTitleCamX = -4.f;
constexpr float kTitleCamY = 126.f;

constexpr float kNunX = -32.f, kNunY = 84.f, kNunR = 4.3f;
constexpr float kCanX = 30.f, kCanY = 100.f, kCanR = 4.3f;
constexpr float kBargeX = -96.f, kBargeY = 68.f, kBargeR = 16.f;
constexpr float kBoatX = 76.f, kBoatY = 50.f, kBoatR = 9.f;

const float kPierX[2] = {-24.7f, 24.7f};
const float kPierY[5] = {136.f, 160.f, 184.f, 208.f, 232.f};
const float kFenderY[4] = {148.f, 170.f, 192.f, 210.f};
const float kTuft[5][2] = {{-18.f, 246.f}, {6.f, 252.f}, {22.f, 240.f}, {-8.f, 262.f}, {16.f, 258.f}};

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

// East-setting ebb. The finger piers shelter the slip until the tide is almost turned.
float ebbAt(float u, float x, float y) {
    float push = 0.12f + 2.35f * u * u;
    bool slack = std::fabs(x) < kPierIn - 1.f && y > kMouth + 2.f && y < kHead && u < 0.92f;
    return slack ? 0.f : push;
}

bool tracing() {
    static int on = -1;
    if (on < 0) on = std::getenv("TUGSLIP_TRACE") != nullptr;
    return on != 0;
}

}  // namespace

float Game::tideLeft() const { return std::max(0.f, kTide - raceTime_); }

float Game::tideU() const {
    if (mode_ == Mode::Title) return 0.22f;
    return std::clamp(raceTime_ / kTide, 0.f, 1.f);
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (inEnd_) return 3;
    if (inSlip_) return 2;
    return 1;
}

int Game::hullFrame() const {
    float u = std::fmod(heading_, kTau);
    if (u < 0.f) u += kTau;
    int i = int(std::lround(u / kTau * 16.f)) % 16;
    if (i < 0) i += 16;
    return i;
}

const char* Game::orderName() const {
    if (throttle_ > 0.75f) return "AHEAD FULL";
    if (throttle_ > 0.35f) return "AHEAD HALF";
    if (throttle_ > 0.08f) return "AHEAD SLOW";
    if (throttle_ < -0.55f) return "ASTERN FULL";
    if (throttle_ < -0.08f) return "ASTERN SLOW";
    return "STOP";
}

const char* Game::hint() const {
    if (inEnd_ && std::fabs(surge_) > 0.7f) return "ASTERN, THEN HOLD";
    if (inEnd_) return "HOLD THE BERTH";
    if (inSlip_) return "THE BERTH IS AT THE HEAD";
    if (tideU() > 0.55f) return "THE EBB IS SETTING EAST";
    if (y_ > kMouth - 40.f) return "LINE THE BOW ON THE SLIP";
    return "BERTH BEFORE THE TIDE TURNS";
}

void Game::begin() {
    x_ = kStartX;
    y_ = kStartY;
    heading_ = kStartH;
    surge_ = 0.f;
    yaw_ = 0.f;
    throttle_ = 0.f;
    raceTime_ = 0.f;
    settle_ = 0.f;
    short_ = 0.f;
    wakeT_ = 0.f;
    smokeT_ = 0.f;
    tickT_ = 0.f;
    hornT_ = 0.f;
    stuckT_ = 0.f;
    stuckX_ = x_;
    stuckY_ = y_;
    wakeCursor_ = 0;
    smokeCursor_ = 0;
    inSlip_ = false;
    inEnd_ = false;
    committed_ = false;
    entered_ = false;
    won_ = false;
    over_ = false;
    chimeN_ = 0;
    chimeStep_ = 0;
    why_[0] = 0;
    std::snprintf(why_, sizeof why_, "running");
    for (Puff& p : wakes_) p = {};
    for (Puff& p : smoke_) p = {};
}

void Game::showTitle() {
    begin();
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
    camY_ = y_;
    blip(520.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.72f);
    sys.apu.setEcho(0.12f, 0.18f, 0.08f);
    if (bot_) {
        begin();
        mode_ = Mode::Run;
        zoom_ = kPlayZoom;
        camX_ = x_;
        camY_ = y_;
    } else {
        showTitle();
    }
}

void Game::human(float& steer, float& throttle) {
    const gs::Pad& p = sys_->pad;
    steer = 0.f;
    if (p.down(gs::BTN_LEFT)) steer += 1.f;
    if (p.down(gs::BTN_RIGHT)) steer -= 1.f;
    if (std::fabs(p.axisX) > 0.18f) steer = std::clamp(-p.axisX, -1.f, 1.f);
    const bool go = p.down(gs::BTN_UP) || p.down(gs::BTN_A) || p.down(gs::BTN_C) || p.down(gs::BTN_TURBO);
    const bool stop = p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.down(gs::BTN_X);
    float thr = 0.f;
    if (stop) thr = -1.f;
    else if (go) thr = 1.f;
    if (p.accel > 0.15f) thr = p.accel;
    if (p.brake > 0.15f) thr = -p.brake;
    if (std::fabs(p.axisY) > 0.2f) thr = std::clamp(p.axisY, -1.f, 1.f);
    if (p.pressed(gs::BTN_Y) || p.pressed(gs::BTN_Z)) hornT_ = 0.55f;
    throttle = thr;
}

void Game::pilot(float& steer, float& throttle) {
    const float push = ebbAt(std::clamp(raceTime_ / kTide, 0.f, 1.f), x_, y_);
    auto face = [&](float want) {
        float pred = wrap(want - (heading_ + yaw_ * 0.16f));
        steer = std::clamp(pred / 0.28f, -1.f, 1.f);
        return std::fabs(pred);
    };

    if (!committed_) {
        float dx = kGateX - x_, dy = kGateY - y_;
        float dist = std::hypot(dx, dy);
        if (dist > 10.f) {
            float err = face(std::atan2(dy, dx));
            float th = dist > 34.f ? 0.92f : 0.58f;
            if (err > 0.85f) th = 0.12f;
            else if (err > 0.42f) th *= 0.45f;
            if (dist < 28.f && surge_ > 3.4f) th = -0.4f;
            if (dist < 16.f && surge_ > 2.0f) th = -0.85f;
            if (surge_ > 6.2f) th = -0.15f;
            throttle = th;
            return;
        }
        if (y_ > kGateY + 5.f) {
            face(kNorth);
            throttle = surge_ > -0.35f ? -0.75f : 0.f;
            return;
        }
        float err = face(kNorth);
        if (surge_ > 1.3f) {
            throttle = -0.9f;
            return;
        }
        if (err > 0.20f) {
            if (surge_ > 0.45f) throttle = -0.7f;
            else if (surge_ < -0.15f) throttle = 0.35f;
            else throttle = 0.f;
            return;
        }
        if (std::fabs(x_) > 2.5f || std::fabs(y_ - kGateY) > 7.f) {
            face(kNorth + std::clamp(x_ * 0.30f, -0.45f, 0.45f));
            if (surge_ > 1.1f) throttle = -0.45f;
            else if (surge_ < 0.55f) throttle = 0.45f;
            else throttle = 0.f;
            return;
        }
        committed_ = true;
        throttle = 0.55f;
        return;
    }

    float spd = std::max(0.8f, surge_);
    float delta = std::clamp((push + x_ * 0.85f) / spd, -0.48f, 0.48f);
    if (std::fabs(x_) < 1.2f && std::fabs(surge_) < 1.f) delta *= 0.25f;
    float err = face(kNorth + delta);
    if (err > 0.75f && y_ < kMouth + 6.f) {
        throttle = surge_ > 1.2f ? -0.65f : 0.12f;
        return;
    }
    if (y_ > kPark + 3.5f) {
        throttle = surge_ > -0.35f ? -0.8f : 0.f;
        return;
    }

    float cap;
    if (y_ < kMouth + 8.f) cap = std::fabs(x_) > 3.f ? 1.8f : 3.6f;
    else if (y_ < kPark - 16.f) cap = std::fabs(x_) > 2.5f ? 1.6f : 3.2f;
    else if (y_ < kPark - 5.f) cap = 1.35f;
    else cap = 0.45f;
    if (std::fabs(x_) > kBerthX - 0.4f && y_ < kPark - 2.f) cap = std::max(cap, 1.0f);

    if (surge_ > cap + 0.28f) throttle = -0.85f;
    else if (surge_ < cap - 0.22f) throttle = cap < 0.6f ? 0.32f : 0.7f;
    else if (cap < 0.6f && surge_ < 0.55f && surge_ > -0.08f && y_ > kPark - 4.f && std::fabs(x_) <= kBerthX) throttle = 0.f;
    else throttle = 0.06f;
}

void Game::succeed() {
    if (won_) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    surge_ = 0.f;
    yaw_ = 0.f;
    throttle_ = 0.f;
    std::snprintf(why_, sizeof why_, "berthed");
    chime(5);
    sys_->rumble(0.3f, 0.5f, 160);
    sys_->setLight(40, 170, 70);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    surge_ = 0.f;
    yaw_ = 0.f;
    throttle_ = 0.f;
    std::snprintf(why_, sizeof why_, "%s", why);
    sys_->apu.noiseBurst(0.42f, 80.f, 0.4f);
    sys_->apu.tone(0, 70.f, 0.06f);
    tone0_ = 0.4f;
    sys_->rumble(0.55f, 0.15f, 180);
    sys_->setLight(170, 30, 20);
}

void Game::physics(float dt, float steer, float throttle) {
    raceTime_ += dt;
    // Bow thruster yaws even with no way on. The rudder adds to it once the screw is pulling.
    float way = std::min(std::fabs(surge_) / 4.f, 1.f);
    float yawCmd = steer * (1.15f + way * 0.9f);
    yaw_ += (yawCmd - yaw_) * (1.f - std::exp(-4.2f * dt));
    yaw_ = std::clamp(yaw_, -1.7f, 1.7f);
    heading_ = wrap(heading_ + yaw_ * dt);

    float target = throttle >= 0.f ? throttle * kMaxAhead : throttle * kMaxAstern;
    float response = throttle * surge_ < -0.02f ? 2.5f : 0.85f;
    surge_ += (target - surge_) * (1.f - std::exp(-response * dt));
    if (std::fabs(throttle) < 0.05f && std::fabs(surge_) < 1.3f) surge_ *= std::exp(-3.4f * dt);
    surge_ = std::clamp(surge_, -kMaxAstern, kMaxAhead);

    float c = std::cos(heading_), s = std::sin(heading_);
    x_ += c * surge_ * dt;
    y_ += s * surge_ * dt;

    float u = std::clamp(raceTime_ / kTide, 0.f, 1.f);
    x_ += ebbAt(u, x_, y_) * dt;

    auto bump = [&](float bx, float by, float rad) {
        float dx = x_ - bx, dy = y_ - by;
        float d = std::hypot(dx, dy);
        float need = rad + kBeam * 0.85f;
        if (d < need && d > 0.01f) {
            x_ = bx + dx / d * need;
            y_ = by + dy / d * need;
            surge_ *= 0.55f;
            if (thumpT_ <= 0.f) {
                sys_->apu.noiseBurst(0.2f, 180.f, 0.1f);
                thumpT_ = 0.25f;
            }
        }
    };
    bump(kNunX, kNunY, kNunR);
    bump(kCanX, kCanY, kCanR);
    bump(kBargeX, kBargeY, kBargeR);
    bump(kBoatX, kBoatY, kBoatR);

    if (x_ < -130.f) {
        x_ = -130.f;
        surge_ *= 0.5f;
    } else if (x_ > 130.f) {
        x_ = 130.f;
        surge_ *= 0.5f;
    }
    if (y_ < 4.f) {
        y_ = 4.f;
        if (s < 0.f) surge_ *= 0.5f;
    }

    auto sample = [&](float along, float beam, float& px, float& py) {
        px = x_ + c * along - s * beam;
        py = y_ + s * along + c * beam;
    };
    float pts[5][3];
    sample(0.f, 0.f, pts[0][0], pts[0][1]);
    pts[0][2] = kBeam;
    sample(kBow, 0.f, pts[1][0], pts[1][1]);
    pts[1][2] = 1.7f;
    sample(-kStern, 0.f, pts[2][0], pts[2][1]);
    pts[2][2] = 1.55f;
    sample(kBow * 0.4f, kBeam * 0.72f, pts[3][0], pts[3][1]);
    pts[3][2] = 1.25f;
    sample(kBow * 0.4f, -kBeam * 0.72f, pts[4][0], pts[4][1]);
    pts[4][2] = 1.25f;

    float hitSpd = std::fabs(surge_);
    float xBefore = x_, yBefore = y_;
    bool scraped = false;
    auto pushRect = [&](float px, float py, float rad, float x0, float x1, float y0, float y1) {
        float nx = std::clamp(px, x0, x1);
        float ny = std::clamp(py, y0, y1);
        float dx = px - nx, dy = py - ny;
        float d2 = dx * dx + dy * dy;
        if (d2 >= rad * rad) return;
        scraped = true;
        float d = std::sqrt(std::max(d2, 1e-6f));
        if (d < 0.04f) {
            float dl = px - x0, dr = x1 - px, db = py - y0, dtv = y1 - py;
            if (dl <= dr && dl <= db && dl <= dtv) x_ -= dl + rad;
            else if (dr <= db && dr <= dtv) x_ += dr + rad;
            else if (db <= dtv) y_ -= db + rad;
            else y_ += dtv + rad;
        } else {
            float pen = rad - d;
            x_ += dx / d * pen;
            y_ += dy / d * pen;
        }
    };
    for (const auto& p : pts) {
        pushRect(p[0], p[1], p[2], -400.f, -kPierIn, kMouth, 520.f);
        pushRect(p[0], p[1], p[2], kPierIn, 400.f, kMouth, 520.f);
    }
    float shove = std::hypot(x_ - xBefore, y_ - yBefore);
    if (shove > 2.2f) {
        x_ = xBefore + (x_ - xBefore) / shove * 2.2f;
        y_ = yBefore + (y_ - yBefore) / shove * 2.2f;
    }
    if (scraped) {
        if (hitSpd > kScrape) {
            fail("scraped a pile");
            return;
        }
        surge_ *= 0.5f;
        if (thumpT_ <= 0.f) {
            sys_->apu.noiseBurst(0.28f, 140.f, 0.12f);
            thumpT_ = 0.28f;
            sys_->rumble(0.22f, 0.08f, 60);
        }
    }

    float bowX, bowY, sternX, sternY;
    sample(kBow, 0.f, bowX, bowY);
    sample(-kStern, 0.f, sternX, sternY);
    if (std::fabs(x_) < kPierIn && y_ > kBerthY1 + 6.f) {
        fail("past the berth");
        return;
    }
    if (bowY >= kHead - 0.6f || y_ > kHead - 2.f) {
        fail("hit the head");
        return;
    }

    float errH = std::fabs(wrap(kNorth - heading_));
    bool bowIn = std::fabs(bowX) < kPierIn - 1.f && bowY < kHead - 2.f && bowY > kMouth;
    bool sternIn = std::fabs(sternX) < kPierIn - 1.f && sternY > kMouth + 2.f && sternY < kHead;
    inSlip_ = std::fabs(x_) < kPierIn - 0.5f && y_ > kMouth + 1.f && y_ < kHead - 1.f;
    bool posed = bowIn && sternIn && std::fabs(x_) <= kBerthX && y_ >= kBerthY0 && y_ <= kBerthY1 && errH <= 0.58f;
    inEnd_ = posed;
    if (inSlip_ && !entered_) {
        entered_ = true;
        blip(440.f);
    }

    if (posed && std::fabs(surge_) <= kStop) {
        settle_ += dt;
        surge_ *= std::exp(-6.f * dt);
        yaw_ *= std::exp(-8.f * dt);
        if (settle_ >= kHold) {
            succeed();
            return;
        }
    } else {
        settle_ = 0.f;
    }

    bool sitting = inSlip_ && !posed && std::fabs(surge_) < 0.30f && y_ > kMouth + 8.f;
    if (sitting) {
        short_ += dt;
        if (short_ >= kShort) {
            fail(y_ > kBerthY1 ? "past the berth" : "short of the berth");
            return;
        }
    } else {
        short_ = 0.f;
    }

    if (raceTime_ >= kTide) {
        fail("tide turned");
        return;
    }

    wakeT_ -= dt;
    if (std::fabs(surge_) > 2.2f && wakeT_ <= 0.f) {
        wakeT_ = 0.08f;
        Puff w;
        w.x = x_ - c * 7.2f;
        w.y = y_ - s * 7.2f;
        w.life = 1.f;
        wakes_[wakeCursor_] = w;
        wakeCursor_ = (wakeCursor_ + 1) % 12;
    }
    for (Puff& w : wakes_)
        if (w.life > 0.f) w.life -= dt;

    if (bot_) {
        stuckT_ += dt;
        if (stuckT_ > 2.6f) {
            float moved = std::hypot(x_ - stuckX_, y_ - stuckY_);
            stuckX_ = x_;
            stuckY_ = y_;
            stuckT_ = 0.f;
            if (moved < 0.45f && !committed_ && y_ < kMouth) {
                heading_ = std::atan2(kGateY - y_, kGateX - x_);
                yaw_ = 0.f;
                surge_ = std::max(surge_, 3.2f);
            } else if (moved < 0.35f && committed_ && y_ < kPark - 4.f) {
                heading_ = kNorth;
                yaw_ = 0.f;
                surge_ = std::max(surge_, 1.3f);
            }
        }
    }

    if (tracing() && (int(raceTime_ * 60.f) % 30) == 0) {
        std::fprintf(stderr, "t %.1f x %.1f y %.1f h %.2f yaw %.2f spd %.2f line %d slip %d end %d\n", raceTime_, x_, y_,
                     heading_, yaw_, surge_, committed_ ? 1 : 0, inSlip_ ? 1 : 0, inEnd_ ? 1 : 0);
    }
}

void Game::particles(float dt) {
    smokeT_ -= dt;
    if (smokeT_ <= 0.f && mode_ != Mode::Fail) {
        smokeT_ = 0.16f + (mode_ == Mode::Run ? (1.f - std::min(1.f, std::fabs(throttle_))) * 0.1f : 0.22f);
        float c = std::cos(heading_), s = std::sin(heading_);
        Puff p;
        p.x = x_ + c * -6.4f - s * 0.4f;
        p.y = y_ + s * -6.4f + c * 0.4f;
        p.life = 1.f;
        smoke_[smokeCursor_] = p;
        smokeCursor_ = (smokeCursor_ + 1) % 10;
    }
    float wind = 0.35f + tideU() * 1.4f;
    for (Puff& p : smoke_) {
        if (p.life <= 0.f) continue;
        p.life -= dt * 0.42f;
        p.x += wind * dt;
        p.y += 0.25f * dt;
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
    float water = mode_ == Mode::Run ? 0.01f + std::fabs(surge_) * 0.0016f + tideU() * 0.006f : 0.007f;
    sys_->apu.noise(inSlip_ ? water * 0.45f : water, inSlip_ ? 280.f : 620.f, false);
    if ((mode_ == Mode::Run || mode_ == Mode::Title) && (std::fabs(throttle_) > 0.05f || std::fabs(surge_) > 0.8f)) {
        float wob = 0.75f + 0.25f * std::sin(t_ * (17.f + std::fabs(throttle_) * 20.f));
        float vol = (0.011f + std::fabs(throttle_) * 0.028f) * wob;
        sys_->apu.tone(2, 44.f + std::fabs(throttle_) * 26.f + std::fabs(surge_) * 1.4f, vol);
    } else {
        sys_->apu.tone(2, 0.f, 0.f);
    }
    if (hornT_ > 0.f && chimeN_ == 0) {
        hornT_ -= dt;
        sys_->apu.tone(0, 94.f, hornT_ > 0.08f ? 0.07f : 0.03f);
        tone0_ = 0.08f;
    }
    if (tone0_ > 0.f) {
        tone0_ -= dt;
        if (tone0_ <= 0.f && hornT_ <= 0.f && chimeN_ == 0) sys_->apu.tone(0, 0.f, 0.f);
    }
    if (tone1_ > 0.f) {
        tone1_ -= dt;
        if (tone1_ <= 0.f) sys_->apu.tone(1, 0.f, 0.f);
    }
    if (thumpT_ > 0.f) thumpT_ -= dt;
    if (mode_ == Mode::Run && tideLeft() < 12.f && tideLeft() > 0.f) {
        tickT_ -= dt;
        if (tickT_ <= 0.f) {
            blip(tideLeft() < 5.f ? 860.f : 480.f);
            tickT_ = tideLeft() < 5.f ? 0.22f : 0.48f;
        }
    }
    if (chimeN_ > 0) {
        chimeT_ -= dt;
        if (chimeT_ <= 0.f) {
            static const float notes[] = {392.f, 523.f, 659.f, 784.f, 1046.f};
            sys_->apu.tone(0, notes[std::min(chimeStep_, 4)], 0.055f);
            tone0_ = 0.12f;
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
    if (mode_ == Mode::Run) {
        if (tideLeft() < 12.f) sys.setLight(170, 70, 20);
        else sys.setLight(20, 70, 140);
    } else if (mode_ == Mode::Win) {
        sys.setLight(40, 160, 70);
    } else if (mode_ == Mode::Fail) {
        sys.setLight(170, 30, 20);
    }
    particles(kDt);
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
    float lead = mode_ == Mode::Run ? 14.f : 0.f;
    float gx = x_ + std::cos(heading_) * lead;
    float gy = y_ + std::sin(heading_) * lead;
    float gz = kPlayZoom;
    if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        gx = 0.f;
        gy = (kMouth + kHead) * 0.5f;
        gz = 1.08f;
    }
    float k = 1.f - std::exp(-kDt * (mode_ == Mode::Run ? 4.2f : 2.4f));
    camX_ += (gx - camX_) * k;
    camY_ += (gy - camY_) * k;
    zoom_ += (gz - zoom_) * k;
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
    gs::Sprite spt;
    long sw = std::clamp(std::lround(w), 1L, 1800L);
    long sh = std::clamp(std::lround(h), 1L, 1800L);
    spt.w = int16_t(sw);
    spt.h = int16_t(sh);
    spt.x = int16_t(std::clamp(std::lround(cx - sw * 0.5f), -2000L, 2000L));
    spt.y = int16_t(std::clamp(std::lround(cy - sh * 0.5f), -2000L, 2000L));
    spt.img = m.pick(float(sh));
    spt.pal = uint8_t(pal);
    spt.shadow = shadow;
    sys_->vdp.sprite(spt);
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
    v.roadTime = int(t_ * 40.f);
    float tide = tideU();
    float zoom = std::max(zoom_, 0.25f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (112.f - y) / zoom;
        float depth = std::clamp((wy + 10.f) / 260.f, 0.f, 1.f);
        uint16_t water = lerpC(gs::rgb4(3, 8, 12), gs::rgb4(1, 3, 7), depth);
        float shim = 0.5f + 0.5f * std::sin(wy * 0.18f + t_ * 1.6f + tide * 2.f);
        if (shim > 0.94f) water = lerpC(water, gs::rgb4(11, 14, 15), 0.45f);
        water = lerpC(water, gs::rgb4(6, 7, 4), tide * 0.55f);
        v.lineBackdrop[y] = water;
        int fog = int(std::clamp((std::fabs(wy - camY_) - 80.f) * 0.028f, 0.f, 3.f));
        v.lineFog[y] = uint8_t(fog);
        gs::RoadLine& r = v.road[y];
        r = {};
        if (wy >= kMouth && wy <= kHead) {
            r.on = true;
            r.cx = 160.f + (0.f - camX_) * zoom;
            r.hw = std::max(3.f, (kPierIn - tide * 1.2f) * zoom);
            r.v = wy * 16.f;
            r.pal = uint8_t(PAL_SLIP);
            r.band = (int(std::floor(wy / 8.f)) & 1) ? 1 : 0;
            r.style = 2;
            r.left = gs::GROUND_DROP;
            r.right = gs::GROUND_DROP;
        } else if (wy > kHead && wy < kHead + 70.f) {
            r.on = true;
            r.cx = 160.f + (0.f - camX_) * zoom;
            r.hw = 220.f;
            r.v = wy * 12.f;
            r.pal = uint8_t(PAL_SHORE);
            r.band = (int(std::floor(wy * 0.12f)) & 1) ? 1 : 0;
            r.style = 0;
            r.left = 0;
            r.right = 0;
        }
    }

    auto banner = [&](const gs::Mipped& m, float x, float y, int pal) { spr(m, x, y, float(m.h), pal, false); };
    if (mode_ == Mode::Title) banner(art_.title, 160.f, 18.f, PAL_BANNER);
    else if (mode_ == Mode::Pause) banner(art_.paused, 160.f, 100.f, PAL_BANNER);
    else if (mode_ == Mode::Fail) {
        const gs::Mipped* msg = &art_.shortMsg;
        if (std::strcmp(why_, "tide turned") == 0) msg = &art_.tide;
        else if (std::strcmp(why_, "hit the head") == 0) msg = &art_.headMsg;
        else if (std::strcmp(why_, "scraped a pile") == 0) msg = &art_.scraped;
        else if (std::strcmp(why_, "past the berth") == 0) msg = &art_.past;
        banner(*msg, 160.f, 18.f, PAL_ALERT);
    } else if (mode_ == Mode::Win) {
        banner(art_.berthed, 160.f, 16.f, PAL_WIN);
        banner(art_.inSlip, 160.f, 42.f, PAL_WIN);
    }

    if (mode_ == Mode::Run || mode_ == Mode::Pause) {
        float psx = 160.f + (0.f - camX_) * zoom_;
        float psy = 112.f - (kPark - camY_) * zoom_;
        if (psx < 18.f || psx > 302.f || psy < 16.f || psy > 208.f) {
            float dx = psx - 160.f, dy = psy - 112.f;
            float k = 1.f;
            if (std::fabs(dx) > 1.f) k = std::min(k, 136.f / std::fabs(dx));
            if (std::fabs(dy) > 1.f) k = std::min(k, 84.f / std::fabs(dy));
            spr(art_.pin, 160.f + dx * k, 112.f + dy * k, 11.f, PAL_MARK, false);
        }
        auto dot = [&](float wx, float wy, int pal, float h) {
            spr(art_.dot, 278.f + wx * 0.36f, 120.f - (wy - 120.f) * 0.34f, h, pal, false);
        };
        dot(-kPierIn, kMouth, PAL_BANNER, 3.f);
        dot(kPierIn, kMouth, PAL_BANNER, 3.f);
        dot(-kPierIn, kHead, PAL_BANNER, 3.f);
        dot(kPierIn, kHead, PAL_BANNER, 3.f);
        dot(0.f, kPark, PAL_WIN, 5.f);
        dot(x_, y_, PAL_ALERT, 5.f);
        spr(art_.panel, 278.f, 112.f, 92.f, PAL_BANNER, false);
    }

    float bob = (inSlip_ ? 0.2f : 0.7f) * std::sin(t_ * 2.1f);
    float bsx = 160.f + (x_ - camX_) * zoom_;
    float bsy = 112.f - (y_ - camY_) * zoom_ + bob;
    float boatH = 26.f * zoom_;
    if (mode_ == Mode::Title) boatH = std::max(boatH, 28.f);
    const gs::Mipped& hull = art_.tug[hullFrame()];
    spr(hull, bsx + 3.f, bsy + 4.f, boatH, PAL_TUG, true);
    spr(hull, bsx, bsy, boatH, PAL_TUG, false);

    for (const Puff& p : smoke_) {
        if (p.life <= 0.f) continue;
        float h = (3.2f + (1.f - p.life) * 5.f) * (zoom_ / kPlayZoom);
        float sx = 160.f + (p.x - camX_) * zoom_;
        float sy = 112.f - (p.y - camY_) * zoom_;
        spr(art_.smoke, sx, sy, std::max(2.f, h), PAL_SMOKE, false);
    }
    if (!inSlip_ && std::fabs(surge_) > 1.4f) {
        float c = std::cos(heading_), s = std::sin(heading_);
        float dir = surge_ >= 0.f ? -1.f : 1.f;
        place(art_.foam, x_ + c * 8.f * dir, y_ + s * 8.f * dir, 3.4f, PAL_FOAM, 2.f);
    }
    for (const Puff& w : wakes_) {
        if (w.life <= 0.f) continue;
        float h = (2.4f + (1.f - w.life) * 3.2f) * (zoom_ / kPlayZoom);
        float sx = 160.f + (w.x - camX_) * zoom_;
        float sy = 112.f - (w.y - camY_) * zoom_;
        spr(art_.foam, sx, sy, std::max(2.f, h), PAL_FOAM, false);
    }

    int flap = int(t_ * 3.2f) & 1;
    place(art_.gull[flap], -18.f + std::sin(t_ * 0.4f) * 16.f, 58.f + std::cos(t_ * 0.28f) * 6.f, 6.f, PAL_BIRD, 4.f);
    place(art_.gull[1 - flap], 34.f + std::cos(t_ * 0.33f) * 12.f, 150.f, 5.f, PAL_BIRD, 4.f);
    place(art_.gull[flap], 8.f + std::sin(t_ * 0.5f) * 10.f, 230.f, 5.5f, PAL_BIRD, 4.f);

    if (tide > 0.18f) {
        for (int i = 0; i < 4; i++) {
            float span = 130.f;
            float ex = -60.f + std::fmod(t_ * (8.f + tide * 18.f) + i * 34.f, span);
            float ey = 28.f + i * 16.f;
            if (ey > kMouth - 10.f) continue;
            place(art_.ebb, ex, ey, 3.6f, PAL_TIDE, 0.f);
        }
    }

    place(art_.staff, 31.f, 186.f, 24.f, PAL_TIDE, 10.f);
    place(art_.bobber, 31.f, 196.f - tide * 20.f, 3.6f, PAL_TIDE, 3.f);
    for (float fy : kFenderY) {
        place(art_.fender, -17.2f, fy, 6.f, PAL_PILE, 3.f);
        place(art_.fender, 17.2f, fy, 6.f, PAL_PILE, 3.f);
    }
    place(art_.cleat, -21.f, 176.f, 3.4f, PAL_PIER, 0.f);
    place(art_.cleat, -21.f, 196.f, 3.4f, PAL_PIER, 0.f);
    place(art_.cleat, 21.f, 176.f, 3.4f, PAL_PIER, 0.f);
    place(art_.cleat, 21.f, 196.f, 3.4f, PAL_PIER, 0.f);
    place(art_.pile, -20.f, kMouth - 2.f, 9.f, PAL_PILE, 5.f);
    place(art_.pile, 20.f, kMouth - 2.f, 9.f, PAL_PILE, 5.f);
    place(art_.pile, -20.f, kHead - 4.f, 9.f, PAL_PILE, 5.f);
    place(art_.pile, 20.f, kHead - 4.f, 9.f, PAL_PILE, 5.f);
    place(art_.head, 0.f, kHead, 7.f, PAL_PIER, 4.f);
    for (float px : kPierX)
        for (float py : kPierY) place(art_.pier, px, py, 26.f, PAL_PIER, 8.f);

    place(art_.shed, -2.f, 250.f, 20.f, PAL_YARD, 10.f);
    place(art_.crane, 28.f, 246.f, 22.f, PAL_YARD, 10.f);
    place(art_.barge, kBargeX, kBargeY, 16.f, PAL_YARD, 8.f);
    place(art_.boat, kBoatX, kBoatY, 8.f, PAL_YARD, 5.f);
    place(art_.nun, kNunX, kNunY, 8.f, PAL_BUOY, 5.f);
    place(art_.can, kCanX, kCanY, 8.f, PAL_BUOY, 5.f);
    for (const auto& p : kTuft) place(art_.tuft, p[0], p[1], 7.f, PAL_SHORE, 3.f);

    auto dashes = [&](float x0, float y0, float x1, float y1, int n) {
        for (int i = 0; i < n; i++) {
            float u = n == 1 ? 0.5f : float(i) / float(n - 1);
            place(art_.dash, x0 + (x1 - x0) * u, y0 + (y1 - y0) * u, 1.6f, PAL_MARK, 0.f);
        }
    };
    dashes(-kBerthX, kBerthY0, kBerthX, kBerthY0, 5);
    dashes(-kBerthX, kBerthY1, kBerthX, kBerthY1, 5);
    dashes(-kBerthX, kBerthY0, -kBerthX, kBerthY1, 4);
    dashes(kBerthX, kBerthY0, kBerthX, kBerthY1, 4);
    place(art_.mark, 0.f, kPark, 6.f, PAL_MARK, 8.f);

    char buf[64];
    if (mode_ == Mode::Title) {
        hudC(21, "BERTH IN THE SLIP", PAL_WIN);
        hudC(22, "BEFORE THE TIDE TURNS", PAL_BANNER);
        hudC(24, "UP AHEAD   DOWN ASTERN   ARROWS STEER", PAL_HUD);
        if ((int(t_ * 2.f) & 1) == 0) hudC(26, "START", PAL_WIN);
        else hudC(26, "Y BLOWS THE HORN", PAL_HUD);
        return;
    }
    hud(1, 0, "S3 TUGBOAT SLIP", PAL_BANNER);
    int left = int(std::ceil(tideLeft() - 0.001f));
    if (left < 0) left = 0;
    std::snprintf(buf, sizeof buf, "TIDE %d:%02d", left / 60, left % 60);
    hud(29, 0, buf, left <= 12 ? PAL_ALERT : PAL_HUD);
    if (mode_ == Mode::Pause) {
        hudC(18, "START CONTINUES", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Win) {
        int sec = int(raceTime_);
        std::snprintf(buf, sizeof buf, "TIME %d:%02d   TIDE %d:%02d LEFT", sec / 60, sec % 60, left / 60, left % 60);
        hudC(16, buf, PAL_HUD);
        if (!bot_) hudC(18, "START RUNS THE LEG AGAIN", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Fail) {
        hudC(16, why_, PAL_ALERT);
        if (!bot_) hudC(18, "START TRIES THE LEG AGAIN", PAL_HUD);
        return;
    }
    hud(1, 1, hint(), inEnd_ ? PAL_WIN : PAL_BANNER);
    int sp = int(std::lround(std::fabs(surge_)));
    const char* ebb = "SLACK";
    if (!(inSlip_ && tide < 0.92f)) {
        if (tide > 0.66f) ebb = "EBB HARD";
        else if (tide > 0.30f) ebb = "EBB RISE";
        else ebb = "EBB LOW";
    }
    std::snprintf(buf, sizeof buf, "SPD %02d  %s  %s", sp, orderName(), ebb);
    hud(1, 2, buf, tide > 0.66f && !inSlip_ ? PAL_ALERT : PAL_HUD);
    if (inEnd_) {
        int n = std::clamp(int(settle_ / kHold * 6.f), 0, 6);
        std::snprintf(buf, sizeof buf, "HOLD %.*s", n, "******");
        hud(1, 25, buf, PAL_WIN);
    } else if (inSlip_) {
        int dist = std::max(0, int(std::lround(kPark - y_)));
        std::snprintf(buf, sizeof buf, "BERTH %d", dist);
        hud(1, 25, buf, PAL_MARK);
    } else {
        hud(1, 25, "ONE JOB  —  THE SLIP", PAL_WIN);
    }
    if (left <= 12) hud(1, 26, "TIDE IS TURNING", PAL_ALERT);
}

}  // namespace tugslip

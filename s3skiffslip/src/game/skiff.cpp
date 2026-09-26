#include "skiff.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace skiffslip {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;

constexpr float kStartX = -48.f;
constexpr float kStartY = 34.f;
constexpr float kStartH = 1.18f;

constexpr float kPierIn = 14.5f;
constexpr float kPierOut = 34.f;
constexpr float kMouth = 140.f;
constexpr float kHead = 240.f;
constexpr float kPierFar = 252.f;
constexpr float kBerthY0 = 204.f;
constexpr float kBerthY1 = 228.f;
constexpr float kBerthX = 5.6f;
constexpr float kParkY = 214.f;
constexpr float kBow = 7.2f;
constexpr float kStern = 5.8f;
constexpr float kBeam = 3.15f;

constexpr float kTide = 48.f;
constexpr float kStop = 0.45f;
constexpr float kHold = 0.40f;
constexpr float kShort = 6.0f;
constexpr float kScrape = 3.6f;

constexpr float kPlayZoom = 1.75f;
constexpr float kTitleZoom = 0.86f;
constexpr float kTitleCamX = -6.f;
constexpr float kTitleCamY = 152.f;

const float kPierX[2] = {-24.25f, 24.25f};
const float kPierY[5] = {155.f, 178.f, 201.f, 224.f, 247.f};
const float kPileX[4] = {-15.4f, -32.2f, 15.4f, 32.2f};
const float kPileY[4] = {146.f, 188.f, 214.f, 248.f};
const float kTuft[5][2] = {{-28.f, 258.f}, {-6.f, 268.f}, {12.f, 256.f}, {32.f, 266.f}, {0.f, 278.f}};
const float kBuoy[2][3] = {{-36.f, 88.f, 4.2f}, {24.f, 112.f, 4.2f}};

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

}  // namespace

float Game::tideLeft() const { return std::max(0.f, kTide - raceTime_); }

float Game::tideU() const {
    if (mode_ == Mode::Title) return 0.18f;
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

const char* Game::hint() const {
    if (inEnd_ && std::fabs(speed_) > 1.f) return "BRAKE, THEN LET GO";
    if (inEnd_) return "HOLD THE BERTH";
    if (inSlip_) return "THE END IS THE HEAD OF THE SLIP";
    if (tideU() > 0.55f) return "THE EBB IS SETTING EAST";
    if (y_ > kMouth - 30.f) return "LINE THE BOW ON THE END";
    return "BERTH BEFORE THE TIDE TURNS";
}

void Game::begin() {
    x_ = kStartX;
    y_ = kStartY;
    heading_ = kStartH;
    speed_ = 0.f;
    throttle_ = 0.f;
    raceTime_ = 0.f;
    settle_ = 0.f;
    short_ = 0.f;
    wakeT_ = 0.f;
    tickT_ = 0.f;
    stuckT_ = 0.f;
    stuckX_ = x_;
    stuckY_ = y_;
    wakeCursor_ = 0;
    inSlip_ = false;
    inEnd_ = false;
    committed_ = false;
    entered_ = false;
    won_ = false;
    over_ = false;
    chimeN_ = 0;
    why_[0] = 0;
    std::snprintf(why_, sizeof why_, "running");
    for (Wake& w : wakes_) w = {};
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
    blip(620.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.74f);
    sys.apu.setEcho(0.14f, 0.2f, 0.08f);
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
    const bool go = p.down(gs::BTN_UP) || p.down(gs::BTN_C) || p.down(gs::BTN_A) || p.axisY > 0.25f || p.accel > 0.2f;
    const bool stop = p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.down(gs::BTN_X) || p.axisY < -0.25f || p.brake > 0.2f;
    if (stop) throttle_ = std::max(-1.f, throttle_ - kDt * 2.1f);
    else if (go) throttle_ = std::min(1.f, throttle_ + kDt * 1.25f);
    else {
        float decay = std::fabs(speed_) < 0.5f ? 2.8f : 0.6f;
        if (throttle_ > 0.f) throttle_ = std::max(0.f, throttle_ - kDt * decay);
        else throttle_ = std::min(0.f, throttle_ + kDt * decay);
    }
    throttle = throttle_;
}

void Game::pilot(float& steer, float& throttle) {
    const float north = kPi * 0.5f;
    auto drive = [&](float tx, float ty, float th) {
        float dx = tx - x_, dy = ty - y_;
        float dist = std::hypot(dx, dy);
        float want = dist < 6.f ? north : std::atan2(dy, dx);
        float err = wrap(want - heading_);
        steer = std::clamp(err / 0.36f, -1.f, 1.f);
        if (std::fabs(err) > 0.9f) th *= 0.15f;
        else if (std::fabs(err) > 0.4f) th *= 0.45f;
        if (dist < 12.f) th = std::min(th, 0.3f + dist * 0.04f);
        throttle = th;
    };

    if (!committed_) {
        float errN = wrap(north - heading_);
        bool aligned = std::fabs(x_) < 3.2f && std::fabs(errN) < 0.20f && speed_ > 0.2f && speed_ < 2.7f;
        if (aligned && y_ > kMouth - 26.f && y_ < kMouth - 3.f) {
            committed_ = true;
        } else if (tideLeft() < 18.f && y_ > 70.f) {
            committed_ = true;
        } else if (y_ > kMouth - 22.f) {
            drive(0.f, kMouth - 16.f, 0.25f);
            if (y_ > kMouth - 5.f) throttle = -0.75f;
            else if (speed_ > 2.4f) throttle = -1.f;
            else if (speed_ < 0.15f) throttle = 0.35f;
            return;
        } else {
            float tx = y_ < 100.f ? -8.f : 0.f;
            float ty = y_ < 100.f ? 112.f : (kMouth - 16.f);
            drive(tx, ty, y_ < 100.f ? 0.9f : 0.5f);
            return;
        }
    }

    if (y_ < kMouth - 8.f) {
        drive(0.f, kParkY, 0.82f);
        if (std::fabs(x_) > 6.f && speed_ > 7.f) throttle = 0.2f;
        return;
    }

    // Positive bias yaws west of north, which is how a skiff pointed up-slip moves back to port.
    float bias = std::clamp(x_ * 0.16f, -0.35f, 0.35f);
    if (std::fabs(speed_) < 0.7f && std::fabs(x_) < kBerthX && y_ > kBerthY0) bias *= 0.15f;
    if (std::fabs(x_) > 4.2f) {
        bias = std::clamp(x_ * 0.28f, -0.55f, 0.55f);
        steer = std::clamp(wrap(north + bias - heading_) / 0.26f, -1.f, 1.f);
        float cap = y_ < kBerthY0 - 20.f ? 4.5f : 1.8f;
        if (y_ > kParkY) throttle = speed_ > -0.4f ? -0.5f : 0.05f;
        else if (speed_ < cap) throttle = 0.6f;
        else if (speed_ > cap + 0.7f) throttle = -0.4f;
        else throttle = 0.08f;
        return;
    }

    steer = std::clamp(wrap(north + bias - heading_) / 0.32f, -1.f, 1.f);
    if (y_ < kBerthY0 - 18.f) {
        float cap = 5.6f;
        if (speed_ > cap) throttle = -0.5f;
        else if (speed_ < cap - 0.7f) throttle = 0.72f;
        else throttle = 0.12f;
    } else if (y_ < kBerthY0 + 3.f) {
        if (speed_ < 1.3f) throttle = 0.45f;
        else if (speed_ > 2.2f) throttle = -0.7f;
        else throttle = 0.05f;
    } else if (y_ <= kBerthY1 - 3.f) {
        if (speed_ > 0.35f) throttle = -0.85f;
        else if (speed_ < -0.12f) throttle = 0.3f;
        else throttle = 0.f;
    } else {
        throttle = speed_ > -0.3f ? -0.55f : 0.f;
    }
}

void Game::succeed() {
    if (won_) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    speed_ = 0.f;
    throttle_ = 0.f;
    std::snprintf(why_, sizeof why_, "berthed");
    chime(5);
    sys_->rumble(0.35f, 0.55f, 160);
    sys_->setLight(40, 180, 70);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    speed_ = 0.f;
    throttle_ = 0.f;
    std::snprintf(why_, sizeof why_, "%s", why);
    sys_->apu.noiseBurst(0.4f, 90.f, 0.42f);
    sys_->apu.tone(0, 74.f, 0.06f);
    tone0_ = 0.4f;
    sys_->rumble(0.6f, 0.2f, 180);
    sys_->setLight(180, 30, 20);
}

void Game::physics(float dt, float steer, float throttle) {
    raceTime_ += dt;
    float rate = 2.05f + std::min(std::fabs(speed_), 14.f) * 0.025f;
    heading_ = wrap(heading_ + steer * rate * dt);

    const bool slipNow = std::fabs(x_) < kPierIn && y_ > kMouth && y_ < kHead;
    float cap = slipNow ? 6.8f : 14.5f;
    if (throttle < -0.02f && speed_ > 0.f) {
        speed_ -= (-throttle) * 8.0f * dt;
        if (speed_ < 0.f) speed_ = std::max(speed_, throttle * 3.2f);
    } else {
        float target = throttle >= 0.f ? throttle * cap : throttle * 4.5f;
        speed_ += (target - speed_) * (1.f - std::exp(-1.7f * dt));
    }
    if (std::fabs(throttle) < 0.05f && std::fabs(speed_) < 0.9f) speed_ *= std::exp(-5.5f * dt);
    speed_ = std::clamp(speed_, -4.5f, 16.f);

    float c = std::cos(heading_), s = std::sin(heading_);
    x_ += c * speed_ * dt;
    y_ += s * speed_ * dt;

    float u = std::clamp(raceTime_ / kTide, 0.f, 1.f);
    float ebb = 0.30f + 3.4f * u * u;
    // Finger piers shelter the berth until the tide actually turns.
    bool slack = std::fabs(x_) < kPierIn - 0.3f && y_ > kMouth + 3.f && y_ < kHead && u < 0.80f;
    if (!slack) x_ += ebb * dt;

    auto bump = [&](float bx, float by, float rad) {
        float dx = x_ - bx, dy = y_ - by;
        float d = std::hypot(dx, dy);
        if (d < rad && d > 0.01f) {
            x_ = bx + dx / d * rad;
            y_ = by + dy / d * rad;
            speed_ *= 0.62f;
            if (thumpT_ <= 0.f) {
                sys_->apu.noiseBurst(0.22f, 220.f, 0.1f);
                thumpT_ = 0.25f;
            }
        }
    };
    for (const auto& b : kBuoy) bump(b[0], b[1], b[2]);

    if (x_ < -64.f) {
        x_ = -64.f;
        speed_ *= 0.5f;
    } else if (x_ > 64.f) {
        x_ = 64.f;
        speed_ *= 0.5f;
    }
    if (y_ < 14.f) {
        y_ = 14.f;
        if (s < 0.f) speed_ *= 0.5f;
    }

    auto sample = [&](float along, float beam, float& px, float& py) {
        px = x_ + c * along - s * beam;
        py = y_ + s * along + c * beam;
    };
    float bowX, bowY, sternX, sternY, portX, portY, stbdX, stbdY;
    sample(kBow, 0.f, bowX, bowY);
    sample(-kStern, 0.f, sternX, sternY);
    sample(0.f, -kBeam, portX, portY);
    sample(0.f, kBeam, stbdX, stbdY);

    auto inHead = [&](float px, float py) {
        return py >= kHead && py <= kHead + 16.f && px >= -kPierOut - 2.f && px <= kPierOut + 2.f;
    };
    if (inHead(x_, y_) || inHead(bowX, bowY) || inHead(sternX, sternY)) {
        fail("missed the end");
        return;
    }

    float hitSpd = std::fabs(speed_);
    float xBefore = x_, yBefore = y_;
    bool scraped = false;
    auto pushPoint = [&](float px, float py, float rad, float x0, float x1, float y0, float y1) {
        float nx = std::clamp(px, x0, x1);
        float ny = std::clamp(py, y0, y1);
        float dx = px - nx, dy = py - ny;
        float d2 = dx * dx + dy * dy;
        if (d2 >= rad * rad) return;
        scraped = true;
        float d = std::sqrt(std::max(d2, 1e-8f));
        if (d2 < 1e-4f) {
            float dl = px - x0, dr = x1 - px, db = py - y0, dtv = y1 - py;
            if (dl <= dr && dl <= db && dl <= dtv) x_ -= dl + rad;
            else if (dr <= db && dr <= dtv) x_ += dr + rad;
            else if (db <= dtv) y_ -= db + rad;
            else y_ += dtv + rad;
        } else {
            float pen = rad - d + 0.08f;
            x_ += dx / d * pen;
            y_ += dy / d * pen;
        }
    };
    const float pts[][3] = {{x_, y_, kBeam}, {bowX, bowY, 1.7f}, {sternX, sternY, 1.5f}, {portX, portY, 1.2f}, {stbdX, stbdY, 1.2f}};
    for (const auto& p : pts) {
        pushPoint(p[0], p[1], p[2], -kPierOut - 1.f, -kPierIn, kMouth - 6.f, kPierFar);
        pushPoint(p[0], p[1], p[2], kPierIn, kPierOut + 1.f, kMouth - 6.f, kPierFar);
    }
    float shove = std::hypot(x_ - xBefore, y_ - yBefore);
    if (shove > 2.2f) {
        x_ = xBefore + (x_ - xBefore) / shove * 2.2f;
        y_ = yBefore + (y_ - yBefore) / shove * 2.2f;
    }
    if (scraped) {
        if (hitSpd > kScrape) {
            fail("scraped the pier");
            return;
        }
        speed_ *= 0.45f;
        if (thumpT_ <= 0.f) {
            sys_->apu.noiseBurst(0.3f, 160.f, 0.14f);
            thumpT_ = 0.3f;
            sys_->rumble(0.25f, 0.1f, 70);
        }
    }

    c = std::cos(heading_);
    s = std::sin(heading_);
    sample(kBow, 0.f, bowX, bowY);
    sample(-kStern, 0.f, sternX, sternY);
    if (inHead(x_, y_) || inHead(bowX, bowY) || y_ > kHead + 1.5f) {
        fail("missed the end");
        return;
    }

    float errH = std::fabs(wrap(kPi * 0.5f - heading_));
    bool inside = std::fabs(bowX) < kPierIn - 0.8f && std::fabs(sternX) < kPierIn - 0.8f && bowY < kHead - 2.f &&
                  sternY > kMouth + 6.f;
    inSlip_ = std::fabs(x_) < kPierIn - 0.4f && y_ > kMouth + 1.f && y_ < kHead - 1.f;
    bool posed = inside && std::fabs(x_) <= kBerthX && y_ >= kBerthY0 && y_ <= kBerthY1 && errH <= 0.46f;
    inEnd_ = posed;
    if (inSlip_ && !entered_) {
        entered_ = true;
        blip(520.f);
    }

    if (posed && std::fabs(speed_) <= kStop) {
        settle_ += dt;
        speed_ *= std::exp(-6.5f * dt);
        if (settle_ >= kHold) {
            succeed();
            return;
        }
    } else {
        settle_ = 0.f;
    }

    if (raceTime_ >= kTide) {
        fail("tide turned");
        return;
    }
    if (y_ > kBerthY0 - 6.f && y_ < kHead && std::fabs(x_) < kPierIn && !posed && std::fabs(speed_) < 0.35f) {
        short_ += dt;
        if (short_ >= kShort) {
            fail("missed the end");
            return;
        }
    } else {
        short_ = 0.f;
    }

    wakeT_ -= dt;
    if (!inSlip_ && wakeT_ <= 0.f && std::fabs(speed_) > 4.5f) {
        wakeT_ = 0.07f;
        Wake w;
        w.x = x_ - c * 8.f;
        w.y = y_ - s * 8.f;
        w.life = 1.f;
        wakes_[wakeCursor_] = w;
        wakeCursor_ = (wakeCursor_ + 1) % 18;
    }
    for (Wake& w : wakes_)
        if (w.life > 0.f) w.life -= dt;

    if (bot_) {
        stuckT_ += dt;
        if (stuckT_ > 2.4f) {
            float moved = std::hypot(x_ - stuckX_, y_ - stuckY_);
            stuckX_ = x_;
            stuckY_ = y_;
            stuckT_ = 0.f;
            if (moved < 2.2f && y_ < kMouth - 24.f) {
                heading_ = std::atan2(kMouth - y_, -x_ * 0.5f);
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
    float water = mode_ == Mode::Run ? 0.012f + std::fabs(speed_) * 0.0004f + tideU() * 0.008f : 0.008f;
    sys_->apu.noise(inSlip_ ? water * 0.45f : water, inSlip_ ? 320.f : 700.f, false);
    if (mode_ == Mode::Run && (throttle_ > 0.05f || std::fabs(speed_) > 1.5f)) {
        float wob = 0.65f + 0.35f * std::sin(t_ * (16.f + std::max(0.f, throttle_) * 20.f));
        float vol = (0.012f + std::max(0.f, throttle_) * 0.03f) * wob;
        sys_->apu.tone(2, 48.f + std::max(0.f, throttle_) * 36.f + std::fabs(speed_) * 0.35f, vol);
    } else if (tone0_ <= 0.f) {
        sys_->apu.tone(2, 0.f, 0.f);
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
    if (mode_ == Mode::Run && tideLeft() < 10.f && tideLeft() > 0.f) {
        tickT_ -= dt;
        if (tickT_ <= 0.f) {
            blip(tideLeft() < 4.f ? 880.f : 520.f);
            tickT_ = tideLeft() < 4.f ? 0.25f : 0.5f;
        }
    }
    if (chimeN_ > 0) {
        chimeT_ -= dt;
        if (chimeT_ <= 0.f) {
            static const float notes[] = {523.f, 659.f, 784.f, 880.f, 1046.f};
            sys_->apu.tone(0, notes[std::min(chimeStep_, 4)], 0.05f);
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
            blip(360.f);
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
        if (tideLeft() < 10.f) sys.setLight(180, 80, 20);
        else sys.setLight(30, 90, 150);
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
    float lead = mode_ == Mode::Run ? 12.f : 0.f;
    float gx = x_ + std::cos(heading_) * lead;
    float gy = y_ + std::sin(heading_) * lead;
    float gz = kPlayZoom;
    if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        gx = 0.f;
        gy = (kMouth + kHead) * 0.5f;
        gz = 1.15f;
    }
    float k = 1.f - std::exp(-kDt * (mode_ == Mode::Run ? 4.4f : 2.6f));
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

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow, bool hflip) {
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
    spt.hflip = hflip;
    sys_->vdp.sprite(spt);
}

void Game::place(const gs::Mipped& m, float wx, float wy, float worldH, int pal, float minPx, bool hflip) {
    float sx = 160.f + (wx - camX_) * zoom_;
    float sy = 112.f - (wy - camY_) * zoom_;
    float h = worldH * zoom_;
    if (h < minPx) h = minPx;
    spr(m, sx, sy, h, pal, false, hflip);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.roadTime = int(t_ * 48.f);
    float tide = tideU();
    float zoom = std::max(zoom_, 0.2f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (112.f - y) / zoom;
        float u = std::clamp((wy - 20.f) / 280.f, 0.f, 1.f);
        uint16_t water = lerpC(gs::rgb4(4, 9, 13), gs::rgb4(1, 4, 8), u);
        float shim = 0.5f + 0.5f * std::sin(wy * 0.22f + t_ * 1.8f);
        if (shim > 0.93f) water = lerpC(water, gs::rgb4(12, 9, 6), 0.55f);
        water = lerpC(water, gs::rgb4(5, 6, 4), tide * 0.62f);
        v.lineBackdrop[y] = water;
        int fog = int(std::clamp((std::fabs(wy - camY_) - 55.f) * 0.045f, 0.f, 5.f));
        v.lineFog[y] = uint8_t(fog);
        gs::RoadLine& r = v.road[y];
        r = {};
        if (wy >= kMouth && wy <= kHead) {
            r.on = true;
            r.cx = 160.f + (0.f - camX_) * zoom_;
            r.hw = std::max(2.f, kPierIn * (1.f - tide * 0.1f) * zoom_);
            r.v = wy * 18.f;
            r.pal = uint8_t(PAL_SLIP);
            r.band = (int(std::floor(wy / 7.f)) & 1) ? 1 : 0;
            r.style = 2;
            if (tide > 0.25f) {
                r.left = gs::GROUND_LAND;
                r.right = gs::GROUND_LAND;
            } else {
                r.left = gs::GROUND_DROP;
                r.right = gs::GROUND_DROP;
            }
        } else if (wy > kHead && wy < kHead + 80.f) {
            r.on = true;
            r.cx = 160.f + (0.f - camX_) * zoom_;
            r.hw = 180.f;
            r.v = wy * 14.f;
            r.pal = uint8_t(PAL_SHORE);
            r.band = (int(std::floor(wy * 0.15f)) & 1) ? 1 : 0;
            r.style = 0;
            r.left = 0;
            r.right = 0;
        }
    }

    auto banner = [&](const gs::Mipped& m, float x, float y, int pal) { spr(m, x, y, float(m.h), pal, false, false); };
    if (mode_ == Mode::Title) banner(art_.title, 160.f, 16.f, PAL_BANNER);
    else if (mode_ == Mode::Pause) banner(art_.paused, 160.f, 96.f, PAL_BANNER);
    else if (mode_ == Mode::Fail) {
        const gs::Mipped* msg = &art_.missed;
        if (std::strcmp(why_, "tide turned") == 0) msg = &art_.tide;
        else if (std::strcmp(why_, "scraped the pier") == 0) msg = &art_.scraped;
        banner(*msg, 160.f, 20.f, PAL_ALERT);
        banner(art_.leg, 160.f, 46.f, PAL_ALERT);
    } else if (mode_ == Mode::Win) {
        banner(art_.berthed, 160.f, 18.f, PAL_WIN);
        banner(art_.inSlip, 160.f, 46.f, PAL_WIN);
    }

    if (mode_ == Mode::Run || mode_ == Mode::Pause) {
        float psx = 160.f + (0.f - camX_) * zoom_;
        float psy = 112.f - (kParkY - camY_) * zoom_;
        if (psx < 16.f || psx > 304.f || psy < 16.f || psy > 208.f) {
            float dx = psx - 160.f, dy = psy - 112.f;
            float k = 1.f;
            if (std::fabs(dx) > 1.f) k = std::min(k, 140.f / std::fabs(dx));
            if (std::fabs(dy) > 1.f) k = std::min(k, 88.f / std::fabs(dy));
            spr(art_.pin, 160.f + dx * k, 112.f + dy * k, 11.f, PAL_MARK, false, false);
        }
        auto dot = [&](float wx, float wy, int pal, float h) {
            spr(art_.dot, 274.f + wx * 0.42f, 96.f - (wy - 140.f) * 0.32f, h, pal, false, false);
        };
        dot(-kPierIn, kMouth, PAL_BANNER, 3.f);
        dot(kPierIn, kMouth, PAL_BANNER, 3.f);
        dot(-kPierIn, kHead, PAL_BANNER, 3.f);
        dot(kPierIn, kHead, PAL_BANNER, 3.f);
        dot(0.f, kParkY, PAL_WIN, 5.f);
        dot(x_, y_, PAL_ALERT, 5.f);
        spr(art_.panel, 274.f, 96.f, 100.f, PAL_MAP, false, false);
    }

    float bob = (inSlip_ ? 0.25f : 0.85f) * std::sin(t_ * 2.4f);
    float bsx = 160.f + (x_ - camX_) * zoom_;
    float bsy = 112.f - (y_ - camY_) * zoom_ + bob;
    float boatH = 20.f * zoom_;
    if (mode_ == Mode::Title) boatH = std::max(boatH, 22.f);
    const gs::Mipped& hull = art_.hull[hullFrame()];
    spr(hull, bsx + 3.f, bsy + 3.f, boatH, PAL_HULL, true, false);
    spr(hull, bsx, bsy, boatH, PAL_HULL, false, false);
    if (!inSlip_ && std::fabs(speed_) > 3.f) {
        float c = std::cos(heading_), s = std::sin(heading_);
        place(art_.foam, x_ + c * 9.f, y_ + s * 9.f, 3.2f + std::fabs(speed_) * 0.04f, PAL_FOAM, 2.f);
    }

    int flap = int(t_ * 3.5f) & 1;
    place(art_.heron[int(t_ * 0.7f) & 1], -22.f, 176.f, 16.f, PAL_BIRD, 8.f);
    place(art_.gull[flap], -20.f + std::sin(t_ * 0.45f) * 18.f, 70.f + std::cos(t_ * 0.3f) * 8.f, 6.f, PAL_BIRD, 4.f);
    place(art_.gull[1 - flap], 30.f + std::cos(t_ * 0.35f) * 14.f, 180.f, 5.f, PAL_BIRD, 4.f);
    place(art_.flag, -20.f, 208.f, 14.f, PAL_MARK, 8.f, false);
    place(art_.flag, 20.f, 208.f, 14.f, PAL_MARK, 8.f, true);
    place(art_.cleat, -17.f, 202.f, 4.f, PAL_PIER, 0.f);
    place(art_.cleat, -17.f, 218.f, 4.f, PAL_PIER, 0.f);
    place(art_.cleat, 17.f, 202.f, 4.f, PAL_PIER, 0.f);
    place(art_.cleat, 17.f, 218.f, 4.f, PAL_PIER, 0.f);
    place(art_.ladder, 22.f, 208.f, 12.f, PAL_PIER, 6.f);
    for (float px : kPileX)
        for (float py : kPileY) place(art_.pile, px, py, 10.f, PAL_PILE, 4.f);
    place(art_.bobber, 28.f, 214.f - tide * 22.f, 4.f, PAL_TIDE, 3.f);
    place(art_.staff, 28.f, 200.f, 26.f, PAL_TIDE, 10.f);

    place(art_.head, 0.f, kHead, 7.f, PAL_PIER, 4.f);
    for (float px : kPierX)
        for (float py : kPierY) place(art_.pier, px, py, 30.f, PAL_PIER, 8.f);
    place(art_.shed, 2.f, 268.f, 22.f, PAL_SHED, 10.f);
    place(art_.buoyG, kBuoy[0][0], kBuoy[0][1], 8.f, PAL_BUOY, 5.f);
    place(art_.buoyR, kBuoy[1][0], kBuoy[1][1], 8.f, PAL_BUOY, 5.f);
    for (const auto& p : kTuft) place(art_.tuft, p[0], p[1], 8.f, PAL_SHORE, 3.f);

    auto dashes = [&](float x0, float y0, float x1, float y1, int n) {
        for (int i = 0; i < n; i++) {
            float u = n == 1 ? 0.5f : float(i) / float(n - 1);
            place(art_.dash, x0 + (x1 - x0) * u, y0 + (y1 - y0) * u, 2.2f, PAL_MARK, 0.f);
        }
    };
    dashes(-kBerthX, kBerthY0, kBerthX, kBerthY0, 5);
    dashes(-kBerthX, kBerthY1, kBerthX, kBerthY1, 5);
    dashes(-kBerthX, kBerthY0, -kBerthX, kBerthY1, 4);
    dashes(kBerthX, kBerthY0, kBerthX, kBerthY1, 4);
    place(art_.endMark, 0.f, 232.f, 9.f, PAL_MARK, 12.f);

    for (const Wake& w : wakes_) {
        if (w.life <= 0.f) continue;
        float h = (2.2f + (1.f - w.life) * 3.5f) * (zoom_ / kPlayZoom);
        float sx = 160.f + (w.x - camX_) * zoom_;
        float sy = 112.f - (w.y - camY_) * zoom_;
        spr(art_.foam, sx, sy, std::max(2.f, h), PAL_FOAM, false, false);
    }

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(22, "BERTH IN THE SLIP", PAL_WIN);
        hudC(23, "BEFORE THE TIDE TURNS", PAL_BANNER);
        hudC(24, "MISS THE END AND THE LEG FAILS", PAL_ALERT);
        if ((int(t_ * 2.f) & 1) == 0) hudC(26, "START", PAL_WIN);
        else hudC(26, "UP THROTTLE   DOWN BRAKE   ARROWS STEER", PAL_HUD);
        return;
    }
    hud(1, 0, "S3 SKIFF SLIP", PAL_BANNER);
    int left = int(std::ceil(tideLeft() - 0.001f));
    if (left < 0) left = 0;
    std::snprintf(buf, sizeof buf, "TIDE %d:%02d", left / 60, left % 60);
    hud(30, 0, buf, left <= 10 ? PAL_ALERT : PAL_HUD);
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
    int sp = int(std::lround(std::fabs(speed_)));
    const char* ebb = "EBB LOW";
    if (inSlip_ && tide < 0.80f) ebb = "SLACK WATER";
    else if (tide > 0.66f) ebb = "EBB HARD";
    else if (tide > 0.30f) ebb = "EBB RISE";
    std::snprintf(buf, sizeof buf, "SPD %02d  %s", sp, ebb);
    hud(1, 2, buf, tide > 0.66f && !inSlip_ ? PAL_ALERT : PAL_HUD);
    if (inEnd_) {
        int n = std::clamp(int(settle_ / kHold * 6.f), 0, 6);
        std::snprintf(buf, sizeof buf, "HOLD %.*s", n, "******");
        hud(1, 25, buf, PAL_WIN);
    } else if (inSlip_) {
        int dist = std::max(0, int(std::lround(kBerthY0 - y_)));
        std::snprintf(buf, sizeof buf, "END %d", dist);
        hud(1, 25, buf, PAL_MARK);
    } else {
        hud(1, 25, "LEG 1  —  THE SLIP", PAL_WIN);
    }
    if (left <= 10) hud(1, 26, "TIDE IS TURNING", PAL_ALERT);
    hud(1, 27, "MISS THE END AND THE LEG FAILS", PAL_ALERT);
}

}  // namespace skiffslip

#include "tug.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace tuggrass {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;

constexpr float kStartX = 16.f;
constexpr float kStartY = 42.f;
constexpr float kStartH = 1.35f;

constexpr float kChan = 30.f;
constexpr float kSouth = 18.f;
constexpr float kGateL = -18.f;
constexpr float kGateR = 10.f;
constexpr float kGateX = -4.f;
constexpr float kWallY0 = 150.f;
constexpr float kWallY1 = 164.f;

constexpr float kGrassX0 = -34.f;
constexpr float kGrassX1 = 24.f;
constexpr float kGrassY0 = 166.f;
constexpr float kGrassY1 = 218.f;
constexpr float kAimX = -4.f;

constexpr float kHalfL = 6.6f;
constexpr float kHalfW = 2.85f;
constexpr float kArtScale = 4.2f;

constexpr float kFlood = 0.85f;
constexpr float kCurrent = 2.45f;
constexpr float kAheadAcc = 6.2f;
constexpr float kAsternAcc = 5.0f;
constexpr float kSlope = 1.12f;
constexpr float kStop = 0.32f;
constexpr float kHoldNeed = 0.70f;
constexpr float kShortNeed = 2.3f;
constexpr float kNoseNeed = 3.6f;
constexpr float kTimeLimit = 92.f;
constexpr float kReverseCap = -0.18f;

constexpr float kPlayZoom = 2.85f;
constexpr float kTitleZoom = 0.78f;
constexpr float kTitleCamX = -2.f;
constexpr float kTitleCamY = 112.f;

const float kReed[9][2] = {
    {-30.f, 168.f}, {-20.f, 169.f}, {-10.f, 167.5f}, {0.f, 169.f}, {8.f, 168.f},
    {16.f, 170.f},  {-32.f, 176.f}, {18.f, 178.f},   {-28.f, 188.f},
};
const float kTuft[8][2] = {
    {-28.f, 186.f}, {14.f, 190.f}, {-30.f, 204.f}, {16.f, 206.f}, {-22.f, 210.f}, {8.f, 212.f}, {-16.f, 196.f}, {4.f, 200.f},
};

float wrap(float a) {
    while (a > kPi) a -= kTau;
    while (a < -kPi) a += kTau;
    return a;
}

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = clampf(t, 0.f, 1.f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    return gs::rgb4(int(ar + (br - ar) * t), int(ag + (bg - ag) * t), int(ab + (bb - ab) * t));
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (hold_ > 0.08f) return 3;
    if (full_ || cover_ > 0.35f) return 2;
    return 1;
}

int Game::hullFrame() const {
    float u = std::fmod(heading_, kTau);
    if (u < 0.f) u += kTau;
    int i = int(std::lround(u / kTau * 16.f)) % 16;
    if (i < 0) i += 16;
    return i;
}

Game::Ext Game::extents() const {
    const float c = std::cos(heading_), s = std::sin(heading_);
    Ext e{x_, x_, y_, y_};
    const float fs[2] = {-kHalfL, kHalfL};
    const float ws[2] = {-kHalfW, kHalfW};
    for (float f : fs) {
        for (float w : ws) {
            float px = x_ + f * c + w * s;
            float py = y_ + f * s - w * c;
            e.minX = std::min(e.minX, px);
            e.maxX = std::max(e.maxX, px);
            e.minY = std::min(e.minY, py);
            e.maxY = std::max(e.maxY, py);
        }
    }
    return e;
}

bool Game::hullOnGrass(const Ext& e) const {
    return e.minX >= kGrassX0 && e.maxX <= kGrassX1 && e.minY >= kGrassY0 && e.maxY <= kGrassY1;
}

float Game::grassCover(const Ext& e) const {
    if (e.maxX <= kGrassX0 || e.minX >= kGrassX1 || e.maxY <= kGrassY0 || e.minY >= kGrassY1) return 0.f;
    float spanY = std::max(0.5f, e.maxY - e.minY);
    float spanX = std::max(0.5f, e.maxX - e.minX);
    float oy = (std::min(e.maxY, kGrassY1) - std::max(e.minY, kGrassY0)) / spanY;
    float ox = (std::min(e.maxX, kGrassX1) - std::max(e.minX, kGrassX0)) / spanX;
    return clampf(ox, 0.f, 1.f) * clampf(oy, 0.f, 1.f);
}

const char* Game::hint() const {
    if (hold_ > 0.02f) return "HOLD THE FULL STOP";
    if (deep_) return "EASE ASTERN AND HOLD";
    if (full_ || cover_ > 0.2f) return "WHOLE TUG ON, THEN ASTERN";
    if (y_ > kWallY0 - 28.f) return "THREAD THE MOUTH";
    return "THE GRASS IS THROUGH THE MOUTH";
}

void Game::begin() {
    x_ = kStartX;
    y_ = kStartY;
    heading_ = kStartH;
    vx_ = vy_ = 0.f;
    throttle_ = 0.f;
    speed_ = 0.f;
    race_ = 0.f;
    hold_ = 0.f;
    shortT_ = 0.f;
    noseT_ = 0.f;
    cover_ = 0.f;
    wakeT_ = smokeT_ = 0.f;
    stuckT_ = 0.f;
    stuckX_ = x_;
    stuckY_ = y_;
    wakeCursor_ = smokeCursor_ = 0;
    full_ = deep_ = launched_ = false;
    won_ = over_ = false;
    phase_ = 0;
    chimeN_ = chimeStep_ = 0;
    why_[0] = 0;
    for (Puff& p : wake_) p = {};
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
    horn(0.38f);
    blip(520.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.82f);
    sys.apu.setEcho(0.11f, 0.16f, 0.08f);
    t_ = 0.f;
    if (bot_) startRun();
    else showTitle();
}

void Game::controls(float& steer) {
    const gs::Pad& p = sys_->pad;
    steer = 0.f;
    if (p.down(gs::BTN_LEFT)) steer += 1.f;
    if (p.down(gs::BTN_RIGHT)) steer -= 1.f;
    if (std::fabs(p.axisX) > 0.18f) steer = clampf(-p.axisX, -1.f, 1.f);
    const bool ahead = p.down(gs::BTN_UP) || p.down(gs::BTN_C) || p.down(gs::BTN_A) || p.axisY > 0.28f;
    const bool astern = p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.down(gs::BTN_X) || p.axisY < -0.28f;
    if (ahead) throttle_ = std::min(1.f, throttle_ + kDt * 0.70f);
    if (astern) throttle_ = std::max(-1.f, throttle_ - kDt * 0.62f);
    if (p.accel > 0.05f) throttle_ = std::min(1.f, throttle_ + p.accel * kDt * 0.9f);
    if (p.brake > 0.05f) throttle_ = std::max(-1.f, throttle_ - p.brake * kDt * 0.85f);
    if (p.down(gs::BTN_TURBO)) horn(0.12f);
}

void Game::pilot(float& steer) {
    const float north = kPi * 0.5f;
    auto govern = [&](float want) {
        if (speed_ > want + 0.40f) throttle_ = -0.55f;
        else if (speed_ < want - 0.30f) throttle_ = 0.62f;
        else throttle_ = 0.06f;
        if (speed_ < 1.05f && y_ < kGrassY0) throttle_ = std::max(throttle_, 0.55f);
    };

    if (!deep_) {
        phase_ = (y_ > kWallY0 - 18.f && std::fabs(x_ - kGateX) < 7.f) ? 1 : 0;
        if (cover_ > 0.4f) phase_ = 2;
        if (y_ < kWallY0 - 8.f || std::fabs(x_ - kGateX) > 6.5f) {
            float tx = kGateX;
            float ty = kWallY0 - 6.f;
            float hdes = std::atan2(ty - y_, tx - x_);
            steer = clampf(wrap(hdes - heading_) / 0.22f, -1.f, 1.f);
            float want = y_ < 96.f ? 3.7f : 2.35f;
            if (std::fabs(wrap(hdes - heading_)) > 0.65f) want = 1.55f;
            govern(want);
            return;
        }
        float hdes = north + clampf((x_ - kGateX) * 0.20f, -0.30f, 0.30f);
        steer = clampf(wrap(hdes - heading_) / 0.18f, -1.f, 1.f);
        float want = y_ > kGrassY0 ? 1.35f : 1.75f;
        govern(want);
        return;
    }

    phase_ = 3;
    float hdes = north + clampf((x_ - kAimX) * 0.14f, -0.28f, 0.28f);
    steer = clampf(wrap(hdes - heading_) / 0.18f, -1.f, 1.f);
    float snh = std::sin(heading_);
    if (std::fabs(snh) < 0.5f) snh = snh >= 0.f ? 0.5f : -0.5f;
    float ay = -kSlope - 2.5f * vy_ + 0.22f;
    if (extents().maxY > kGrassY1 - 12.f) ay -= 1.4f;
    if (extents().minY < kGrassY0 + 3.5f && vy_ < 0.15f) ay += 1.6f;
    float acc = ay / snh;
    throttle_ = clampf(acc >= 0.f ? acc / kAheadAcc : acc / kAsternAcc, -1.f, 1.f);
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.05f);
    tone1_ = 0.08f;
}

void Game::horn(float seconds) {
    if (hornT_ < seconds) hornT_ = seconds;
}

void Game::chime(int notes) {
    chimeN_ = std::clamp(notes, 1, 6);
    chimeStep_ = 0;
    chimeT_ = 0.02f;
}

void Game::win() {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    vx_ = vy_ = 0.f;
    throttle_ = 0.f;
    speed_ = 0.f;
    std::snprintf(why_, sizeof why_, "full stop");
    chime(5);
    sys_->rumble(0.32f, 0.14f, 160);
    sys_->setLight(40, 180, 70);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    std::snprintf(why_, sizeof why_, "%s", why);
    shake_ = 1.f;
    sys_->rumble(0.55f, 0.28f, 180);
    sys_->setLight(180, 36, 24);
    sys_->apu.noiseBurst(0.44f, 130.f, 0.40f);
    sys_->apu.tone(0, 74.f, 0.06f);
    tone0_ = 0.42f;
}

void Game::puff(Puff* ring, int& cursor, int n, float x, float y) {
    ring[cursor].x = x;
    ring[cursor].y = y;
    ring[cursor].life = 1.f;
    cursor = (cursor + 1) % n;
}

void Game::physics(float steer) {
    race_ += kDt;
    float rate = 0.95f + std::min(speed_, 8.f) * 0.06f;
    heading_ = wrap(heading_ + steer * rate * kDt);

    float eng = throttle_ >= 0.f ? throttle_ * kAheadAcc : throttle_ * kAsternAcc;
    float hc = std::cos(heading_), hs = std::sin(heading_);
    vx_ += hc * eng * kDt;
    vy_ += hs * eng * kDt;

    Ext e = extents();
    cover_ = grassCover(e);
    float water = 1.f - cover_;
    float flood = 1.f - std::exp(-kFlood * water * kDt);
    vx_ += (0.f - vx_) * flood;
    vy_ += (kCurrent - vy_) * flood;

    float along = vx_ * hc + vy_ * hs;
    float lat = -vx_ * hs + vy_ * hc;
    lat *= std::exp(-1.7f * cover_ * kDt);
    along *= std::exp(-0.16f * cover_ * kDt);
    if (cover_ > 0.72f && along < kReverseCap) along = kReverseCap;
    vx_ = along * hc - lat * hs;
    vy_ = along * hs + lat * hc;
    vy_ += kSlope * cover_ * kDt;

    float sp = std::hypot(vx_, vy_);
    if (sp > 9.f) {
        vx_ *= 9.f / sp;
        vy_ *= 9.f / sp;
        sp = 9.f;
    }
    speed_ = sp;
    x_ += vx_ * kDt;
    y_ += vy_ * kDt;

    auto thud = [&]() {
        if (thumpT_ > 0.f) return;
        sys_->apu.noiseBurst(0.24f, 190.f, 0.11f);
        thumpT_ = 0.26f;
        shake_ = std::max(shake_, 0.45f);
    };
    auto pushRect = [&](float x0, float x1, float y0, float y1) {
        Ext b = extents();
        if (b.maxX <= x0 || b.minX >= x1 || b.maxY <= y0 || b.minY >= y1) return;
        float penS = b.maxY - y0;
        float penN = y1 - b.minY;
        float penW = b.maxX - x0;
        float penE = x1 - b.minX;
        float pen = penS;
        int side = 0;
        if (penW < pen) {
            pen = penW;
            side = 1;
        }
        if (penE < pen) {
            pen = penE;
            side = 2;
        }
        if (penN < pen) {
            pen = penN;
            side = 3;
        }
        if (side == 0) {
            y_ -= pen + 0.08f;
            if (vy_ > 0.f) vy_ *= 0.25f;
        } else if (side == 1) {
            x_ -= pen + 0.08f;
            if (vx_ > 0.f) vx_ *= 0.25f;
        } else if (side == 2) {
            x_ += pen + 0.08f;
            if (vx_ < 0.f) vx_ *= 0.25f;
        } else {
            y_ += pen + 0.08f;
            if (vy_ < 0.f) vy_ *= 0.25f;
        }
        thud();
    };

    e = extents();
    if (e.minY < kSouth) {
        y_ += kSouth - e.minY + 0.05f;
        if (vy_ < 0.f) vy_ *= 0.3f;
        thud();
    }
    e = extents();
    if (e.maxY < kWallY1) {
        if (e.minX < -kChan) {
            x_ += -kChan - e.minX + 0.05f;
            if (vx_ < 0.f) vx_ *= 0.25f;
            thud();
        }
        e = extents();
        if (e.maxX > kChan) {
            x_ += kChan - e.maxX - 0.05f;
            if (vx_ > 0.f) vx_ *= 0.25f;
            thud();
        }
    }
    pushRect(-kChan - 2.f, kGateL, kWallY0, kWallY1);
    pushRect(kGateR, kChan + 2.f, kWallY0, kWallY1);

    if (!std::isfinite(x_) || !std::isfinite(y_) || !std::isfinite(vx_) || !std::isfinite(vy_)) {
        fail("off the grass");
        return;
    }

    e = extents();
    cover_ = grassCover(e);
    full_ = hullOnGrass(e);
    deep_ = full_ && e.minY > kGrassY0 + 5.f;
    speed_ = std::hypot(vx_, vy_);
    if (speed_ > 1.15f) launched_ = true;

    if (e.maxY > kGrassY1) {
        fail("ran off the grass");
        return;
    }
    if (e.minY > kWallY1 && (e.minX < kGrassX0 - 0.4f || e.maxX > kGrassX1 + 0.4f)) {
        fail("off the grass");
        return;
    }
    if (race_ > kTimeLimit) {
        fail("timed out");
        return;
    }

    if (deep_ && speed_ <= kStop) {
        shortT_ = 0.f;
        noseT_ = 0.f;
        hold_ += kDt;
        if (hold_ >= kHoldNeed) {
            win();
            return;
        }
    } else if (!launched_) {
        hold_ = 0.f;
        shortT_ = 0.f;
    } else if (speed_ <= kStop && cover_ < 0.04f) {
        hold_ = 0.f;
        shortT_ += kDt;
        if (shortT_ >= kShortNeed) {
            fail("stopped short of the grass");
            return;
        }
    } else if (speed_ <= kStop && !full_) {
        hold_ = 0.f;
        noseT_ += kDt;
        if (noseT_ >= kNoseNeed) {
            fail("not fully on the grass");
            return;
        }
    } else {
        if (!deep_) hold_ = 0.f;
        else if (speed_ > kStop) hold_ = 0.f;
        shortT_ = 0.f;
        noseT_ = 0.f;
    }

    wakeT_ -= kDt;
    smokeT_ -= kDt;
    if (wakeT_ <= 0.f && speed_ > 1.3f && cover_ < 0.45f) {
        wakeT_ = 0.07f;
        puff(wake_, wakeCursor_, 16, x_ - hc * 6.2f, y_ - hs * 6.2f);
    }
    if (smokeT_ <= 0.f && (std::fabs(throttle_) > 0.08f || speed_ > 0.8f)) {
        smokeT_ = 0.10f;
        puff(smoke_, smokeCursor_, 8, x_ - hc * 1.7f, y_ - hs * 1.7f);
    }
    for (Puff& p : wake_)
        if (p.life > 0.f) p.life -= kDt * 0.55f;
    for (Puff& p : smoke_)
        if (p.life > 0.f) {
            p.life -= kDt * 0.42f;
            p.y += kDt * 1.3f;
            p.x += kDt * 0.25f;
        }

    if (bot_ && !deep_) {
        stuckT_ += kDt;
        if (stuckT_ > 2.4f) {
            float moved = std::hypot(x_ - stuckX_, y_ - stuckY_);
            stuckX_ = x_;
            stuckY_ = y_;
            stuckT_ = 0.f;
            if (moved < 1.4f) {
                float dx = kGateX - x_;
                float dy = (kWallY0 + 18.f) - y_;
                heading_ = std::atan2(dy, dx);
                vx_ = std::cos(heading_) * 2.3f;
                vy_ = std::sin(heading_) * 2.3f;
            }
        }
    }
}

void Game::audio() {
    float bed = mode_ == Mode::Run ? 0.015f + speed_ * 0.0006f : 0.009f;
    sys_->apu.noise(cover_ > 0.5f ? bed * 0.45f : bed, cover_ > 0.5f ? 240.f : 520.f, false);
    if (hornT_ > 0.f) {
        hornT_ -= kDt;
        float v = hornT_ > 0.06f ? 0.07f : std::max(0.f, hornT_) * 1.05f;
        sys_->apu.tone(0, 96.f, v);
        if (tone1_ <= 0.f) sys_->apu.tone(1, 144.f, v * 0.45f);
    } else if (tone0_ <= 0.f && chimeN_ == 0) {
        sys_->apu.tone(0, 0.f, 0.f);
    }
    if (mode_ == Mode::Run && (std::fabs(throttle_) > 0.04f || speed_ > 0.5f)) {
        float wob = 0.7f + 0.3f * std::sin(t_ * (9.f + std::fabs(throttle_) * 14.f));
        float vol = (0.012f + std::fabs(throttle_) * 0.026f) * wob;
        float f = 42.f + std::fabs(throttle_) * 30.f + speed_ * 0.8f;
        sys_->apu.tone(2, f, vol);
    } else if (hornT_ <= 0.f) {
        sys_->apu.tone(2, 0.f, 0.f);
    }
    if (tone0_ > 0.f) {
        tone0_ -= kDt;
        if (tone0_ <= 0.f && hornT_ <= 0.f && chimeN_ == 0) sys_->apu.tone(0, 0.f, 0.f);
    }
    if (tone1_ > 0.f) {
        tone1_ -= kDt;
        if (tone1_ <= 0.f && hornT_ <= 0.f) sys_->apu.tone(1, 0.f, 0.f);
    }
    if (thumpT_ > 0.f) thumpT_ -= kDt;
    if (chimeN_ > 0) {
        chimeT_ -= kDt;
        if (chimeT_ <= 0.f) {
            static const float notes[] = {330.f, 415.f, 494.f, 659.f, 784.f};
            sys_->apu.tone(0, notes[std::min(chimeStep_, 4)], 0.05f);
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
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A)) startRun();
        else if (pad.pressed(gs::BTN_MODE)) sys.quit();
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(340.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        } else {
            float steer = 0.f;
            if (bot_) pilot(steer);
            else controls(steer);
            physics(steer);
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Run;
        else if (pad.pressed(gs::BTN_MODE)) showTitle();
    } else if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C))) {
        startRun();
    } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
        showTitle();
    }
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 1.8f);
    camera();
    audio();
    if (mode_ == Mode::Win) sys.setLight(40, 180, 70);
    else if (mode_ == Mode::Fail) sys.setLight(180, 36, 24);
    else if (hold_ > 0.02f || deep_) sys.setLight(50, 170, 60);
    else if (cover_ > 0.3f) sys.setLight(70, 140, 50);
    else sys.setLight(30, 80, 130);
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

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool shadow) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    if (cx + w * 0.5f < -8 || cy + h * 0.5f < -8 || cx - w * 0.5f > gs::SCREEN_W + 8 || cy - h * 0.5f > gs::SCREEN_H + 8)
        return;
    gs::Sprite s;
    long sw = std::clamp(std::lround(w), 1L, 1800L);
    long sh = std::clamp(std::lround(h), 1L, 1800L);
    s.w = int16_t(sw);
    s.h = int16_t(sh);
    s.x = int16_t(std::clamp(std::lround(cx - sw * 0.5f), -2000L, 2000L));
    s.y = int16_t(std::clamp(std::lround(cy - sh * 0.5f), -2000L, 2000L));
    s.img = m.pick(float(sh));
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::sprBox(const gs::Mipped& m, float cx, float cy, float w, float h, int pal) {
    if (w < 1.f || h < 1.f || m.h < 1) return;
    if (cx + w * 0.5f < -4 || cy + h * 0.5f < -4 || cx - w * 0.5f > gs::SCREEN_W + 4 || cy - h * 0.5f > gs::SCREEN_H + 4)
        return;
    gs::Sprite s;
    long sw = std::clamp(std::lround(w), 1L, 1800L);
    long sh = std::clamp(std::lround(h), 1L, 1800L);
    s.w = int16_t(sw);
    s.h = int16_t(sh);
    s.x = int16_t(std::clamp(std::lround(cx - sw * 0.5f), -2000L, 2000L));
    s.y = int16_t(std::clamp(std::lround(cy - sh * 0.5f), -2000L, 2000L));
    s.img = m.pick(std::max(float(sw), float(sh)));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::place(const gs::Mipped& m, float wx, float wy, float worldH, int pal, float minPx, bool flip) {
    float h = worldH * zoom_;
    if (h < minPx) h = minPx;
    float sx = 160.f + (wx - camX_) * zoom_;
    float sy = 112.f - (wy - camY_) * zoom_;
    spr(m, sx, sy, h, pal, flip, false);
}

void Game::worldRect(const gs::Mipped& m, float wx, float wy, float ww, float hh, int pal) {
    float sx = 160.f + (wx - camX_) * zoom_;
    float sy = 112.f - (wy - camY_) * zoom_;
    sprBox(m, sx, sy, ww * zoom_, hh * zoom_, pal);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.hudEnabled = true;

    float jx = 0.f, jy = 0.f;
    if (shake_ > 0.f) {
        jx = std::sin(t_ * 46.f) * shake_ * 3.f;
        jy = std::cos(t_ * 37.f) * shake_ * 2.f;
    }
    float invZ = 1.f / std::max(zoom_, 0.25f);
    camX_ -= jx * invZ;
    camY_ += jy * invZ;

    const float grassCx = (kGrassX0 + kGrassX1) * 0.5f;
    const float grassHw = (kGrassX1 - kGrassX0) * 0.5f;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (112.f - float(y)) * invZ;
        float u = clampf((wy - 16.f) / 230.f, 0.f, 1.f);
        uint16_t water = lerpC(gs::rgb4(2, 8, 12), gs::rgb4(1, 4, 8), u);
        float shimmer = 0.5f + 0.5f * std::sin(wy * 0.17f + t_ * 1.6f);
        if (shimmer > 0.93f && wy < kGrassY0) water = lerpC(water, gs::rgb4(8, 14, 14), 0.4f);
        v.lineBackdrop[y] = water;
        v.lineFog[y] = 0;
        gs::RoadLine& r = v.road[y];
        if (wy >= kGrassY0 && wy <= kGrassY1) {
            bool lip = wy < kGrassY0 + 8.f;
            r.on = true;
            r.cx = 160.f + (grassCx - camX_) * zoom_;
            r.hw = std::max(3.f, grassHw * zoom_);
            r.v = wy * 18.f;
            r.pal = uint8_t(lip ? PAL_LIP : PAL_FIELD);
            r.band = (int(std::floor(wy * 0.12f)) & 1) ? 1 : 0;
            r.style = 0;
            r.left = gs::GROUND_WATER;
            r.right = gs::GROUND_WATER;
        } else {
            r.on = false;
        }
    }

    auto banner = [&](const gs::Mipped& m, float yb, int pal) { spr(m, 160.f, yb, float(m.h), pal, false, false); };
    if (mode_ == Mode::Title) banner(art_.title, 16.f, PAL_BANNER);
    else if (mode_ == Mode::Pause) banner(art_.paused, 96.f, PAL_BANNER);
    else if (mode_ == Mode::Win) {
        banner(art_.fullStop, 30.f, PAL_WIN);
        banner(art_.onGrass, 58.f, PAL_WIN);
    } else if (mode_ == Mode::Fail) {
        if (!std::strcmp(why_, "ran off the grass")) banner(art_.ranOff, 28.f, PAL_ALERT);
        else if (!std::strcmp(why_, "off the grass")) banner(art_.offGrass, 28.f, PAL_ALERT);
        else if (!std::strcmp(why_, "stopped short of the grass")) banner(art_.shortStop, 28.f, PAL_ALERT);
        else if (!std::strcmp(why_, "not fully on the grass")) banner(art_.notFull, 28.f, PAL_ALERT);
        else banner(art_.timed, 28.f, PAL_ALERT);
        banner(art_.legFail, 56.f, PAL_ALERT);
    }

    const float propMin = mode_ == Mode::Title ? 7.f : 0.f;
    if (mode_ == Mode::Run || mode_ == Mode::Pause) {
        float ax = 160.f + (kAimX - camX_) * zoom_;
        float ay = 112.f - ((kGrassY0 + kGrassY1) * 0.5f - camY_) * zoom_;
        if (ax < 14.f || ax > 306.f || ay < 16.f || ay > 208.f) {
            float dx = ax - 160.f, dy = ay - 112.f;
            float k = 1.f;
            if (std::fabs(dx) > 1.f) k = std::min(k, 136.f / std::fabs(dx));
            if (std::fabs(dy) > 1.f) k = std::min(k, 84.f / std::fabs(dy));
            spr(art_.pin, 160.f + dx * k, 112.f + dy * k, 11.f, PAL_WIN, false, false);
        }
        auto chart = [&](float wx, float wy, float h, int pal) {
            spr(art_.dot, 286.f + wx * 0.55f, 62.f - (wy - 140.f) * 0.28f, h, pal, false, false);
        };
        chart(kGrassX0, kGrassY0, 3.f, PAL_WIN);
        chart(kGrassX1, kGrassY0, 3.f, PAL_WIN);
        chart(kGrassX0, kGrassY1, 3.f, PAL_WIN);
        chart(kGrassX1, kGrassY1, 3.f, PAL_WIN);
        chart(kGateL, kWallY0, 3.f, PAL_ALERT);
        chart(kGateR, kWallY0, 3.f, PAL_ALERT);
        chart(x_, y_, 5.f, PAL_BANNER);
        spr(art_.panel, 286.f, 62.f, 58.f, PAL_QUAY, false, false);
    }

    for (const Puff& p : smoke_) {
        if (p.life <= 0.f) continue;
        float h = (2.0f + (1.f - p.life) * 2.8f) * (zoom_ / kPlayZoom);
        float sx = 160.f + (p.x - camX_) * zoom_;
        float sy = 112.f - (p.y - camY_) * zoom_;
        spr(art_.smoke, sx, sy, std::max(2.f, h), PAL_SMOKE, false, false);
    }
    float hc = std::cos(heading_), hs = std::sin(heading_);
    if (speed_ > 1.6f && cover_ < 0.5f && mode_ != Mode::Title)
        place(art_.foam, x_ + hc * 6.4f, y_ + hs * 6.4f, 2.2f, PAL_FOAM, 0.f);
    int fi = hullFrame();
    float bob = cover_ > 0.6f ? 0.f : std::sin(t_ * 2.2f) * 0.35f;
    float bh = float(art_.tug[fi].h) / kArtScale * zoom_;
    if (mode_ == Mode::Title) bh = std::max(bh, 18.f);
    float bsx = 160.f + (x_ - camX_) * zoom_;
    float bsy = 112.f - (y_ - camY_) * zoom_ + bob * zoom_ * 0.12f;
    spr(art_.tug[fi], bsx, bsy, bh, PAL_TUG, false, false);
    spr(art_.shade, bsx + 3.f, bsy + 4.f, bh * 0.7f, PAL_TUG, false, true);

    worldRect(art_.bulk, -24.f, 157.f, 12.f, 14.f, PAL_QUAY);
    worldRect(art_.bulk, 20.f, 157.f, 20.f, 14.f, PAL_QUAY);
    for (float y = 28.f; y <= 146.f; y += 16.f) {
        place(art_.quay, -(kChan + 3.5f), y, 14.f, PAL_QUAY, 0.f, false);
        place(art_.quay, kChan + 3.5f, y, 14.f, PAL_QUAY, 0.f, true);
    }
    place(art_.shed, -28.f, 78.f, 12.f, PAL_QUAY, propMin);
    place(art_.shed, 26.f, 96.f, 11.f, PAL_QUAY, propMin);
    place(art_.shed, 16.f, 200.f, 12.f, PAL_WOOD, propMin);
    place(art_.shed, -28.f, 198.f, 11.f, PAL_WOOD, propMin);
    const float lamps[] = {48.f, 92.f, 136.f};
    for (float y : lamps) {
        place(art_.lamp, -(kChan - 0.4f), y, 6.5f, PAL_MARK, 0.f);
        place(art_.lamp, kChan - 0.4f, y, 6.5f, PAL_MARK, 0.f);
    }
    place(art_.post, kGateL, 157.f, 9.f, PAL_MARK, propMin);
    place(art_.post, kGateR, 157.f, 9.f, PAL_MARK, propMin);
    place(art_.flag, kGateL - 1.5f, 160.f, 8.f, PAL_WOOD, propMin);
    place(art_.flag, kGateR + 1.5f, 160.f, 8.f, PAL_WOOD, propMin);
    const float marks[] = {70.f, 100.f, 130.f};
    for (float y : marks) {
        place(art_.buoyG, kGateX - 8.f, y, 4.4f, PAL_WOOD, 0.f);
        place(art_.buoyR, kGateX + 8.f, y, 4.4f, PAL_WOOD, 0.f);
    }
    for (const float* p : kReed) place(art_.reed, p[0], p[1], 7.5f, PAL_REED, propMin * 0.5f);
    for (const float* p : kTuft) place(art_.tuft, p[0], p[1], 5.5f, PAL_TUFT, propMin * 0.4f);

    auto dashes = [&](float x0, float y0, float x1, float y1, int n) {
        for (int i = 0; i < n; i++) {
            float u = n == 1 ? 0.5f : float(i) / float(n - 1);
            place(art_.dash, x0 + (x1 - x0) * u, y0 + (y1 - y0) * u, 1.6f, PAL_MARK, 0.f);
        }
    };
    dashes(kGrassX0 + 2.f, kGrassY0 + 1.2f, kGrassX1 - 2.f, kGrassY0 + 1.2f, 8);

    int flap = int(t_ * 3.4f) & 1;
    place(art_.gull[flap], -6.f + std::sin(t_ * 0.35f) * 16.f, 118.f, 3.8f, PAL_GULL, propMin);
    place(art_.gull[1 - flap], 12.f + std::cos(t_ * 0.28f) * 10.f, 188.f, 3.4f, PAL_GULL, propMin * 0.6f);

    for (const Puff& p : wake_) {
        if (p.life <= 0.f) continue;
        float h = (1.6f + (1.f - p.life) * 2.2f) * zoom_;
        float sx = 160.f + (p.x - camX_) * zoom_;
        float sy = 112.f - (p.y - camY_) * zoom_;
        spr(art_.foam, sx, sy, std::max(2.f, h), PAL_FOAM, false, false);
    }

    camX_ += jx * invZ;
    camY_ -= jy * invZ;

    char buf[64];
    if (mode_ == Mode::Title) {
        hudC(22, "LAND ON THE GRASS", PAL_WIN);
        hudC(23, "COME TO A FULL STOP", PAL_BANNER);
        hudC(24, "THE FLOOD SETS YOU AT THE BANK", PAL_HUD);
        if ((int(t_ * 2.f) & 1) == 0) hudC(26, "START", PAL_WIN);
        else hudC(26, "ARROWS STEER   UP AHEAD   DOWN ASTERN", PAL_HUD);
        hudC(27, "SPACE HORN", PAL_HUD);
        return;
    }

    hud(1, 0, "S3 TUGBOAT GRASS", PAL_BANNER);
    std::snprintf(buf, sizeof buf, "%4.1fS", race_);
    hud(33, 0, buf, PAL_HUD);
    if (mode_ == Mode::Pause) {
        hudC(18, "START CONTINUES", PAL_HUD);
        hudC(19, "ESC BACK TO THE DOCK", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Win) {
        hudC(16, "THE LEG IS MADE", PAL_BANNER);
        std::snprintf(buf, sizeof buf, "%.1fS", race_);
        hudC(18, buf, PAL_HUD);
        if (!bot_) hudC(20, "START RUNS THE LEG AGAIN", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Fail) {
        hudC(16, why_, PAL_ALERT);
        if (!bot_) hudC(18, "START TRIES THE LEG AGAIN", PAL_HUD);
        return;
    }

    hud(1, 1, hint(), (deep_ || hold_ > 0.02f) ? PAL_WIN : PAL_BANNER);
    std::snprintf(buf, sizeof buf, "ENG %+4d  SPD %3.1f", int(std::lround(throttle_ * 100.f)), speed_);
    hud(20, 1, buf, PAL_HUD);
    const char* where = deep_ ? "ON THE GRASS" : (cover_ > 0.25f ? "BEACHING" : "IN THE WATER");
    hud(1, 2, where, deep_ ? PAL_WIN : PAL_HUD);
    if (deep_ && speed_ <= kStop) {
        int n = std::clamp(int(hold_ / kHoldNeed * 6.f), 0, 6);
        std::snprintf(buf, sizeof buf, "HOLD %.*s", n, "******");
        hud(1, 26, buf, PAL_WIN);
    } else if (deep_) {
        hud(1, 26, "ASTERN HOLDS THE BANK", PAL_BANNER);
    } else if (y_ > 120.f) {
        hud(1, 26, "MOUTH TO PORT, THEN THE GRASS", PAL_MARK);
    } else {
        hud(1, 26, "ONE JOB  —  LAND AND STOP", PAL_WIN);
    }
    hud(1, 27, "ARROWS STEER  UP AHEAD  DOWN ASTERN", PAL_HUD);
}

}  // namespace tuggrass

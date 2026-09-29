#include "game/rick.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace rickslip {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;

constexpr float kStartX = 0.f;
constexpr float kStartY = 18.f;
constexpr float kStartH = kPi * 0.5f;

constexpr float kMouth = 96.f;
constexpr float kHead = 178.f;
constexpr float kPierIn = 7.2f;
constexpr float kPierOut = 15.5f;
constexpr float kPierFar = 196.f;
constexpr float kBerthX = 3.6f;
constexpr float kBerthY0 = 146.f;
constexpr float kBerthY1 = 166.f;
constexpr float kParkY = 156.f;

constexpr float kHalfL = 2.35f;
constexpr float kHalfW = 1.05f;
constexpr float kStop = 0.28f;
constexpr float kHold = 0.55f;
constexpr float kShort = 2.1f;
constexpr float kScrape = 5.4f;
constexpr float kTide = 48.f;

constexpr float kPlayZoom = 7.6f;
constexpr float kTitleZoom = 1.35f;
constexpr float kTitleCamX = 0.f;
constexpr float kTitleCamY = 130.f;

const float kTuft[][2] = {{-22.f, 40.f}, {20.f, 52.f}, {-24.f, 70.f}, {22.f, 28.f}, {-18.f, 12.f}, {18.f, 84.f}};
const float kLamp[][2] = {{-18.f, 36.f}, {18.f, 58.f}, {-18.f, 80.f}, {18.f, 104.f}, {-18.f, 150.f}, {18.f, 168.f}};

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
    return gs::rgb4(int(ar + (br - ar) * t + 0.5f), int(ag + (bg - ag) * t + 0.5f), int(ab + (bb - ab) * t + 0.5f));
}

}  // namespace

float Game::tideLeft() const { return std::max(0.f, kTide - raceTime_); }
float Game::tideU() const { return clampf(raceTime_ / kTide, 0.f, 1.f); }

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (inEnd_ || settle_ > 0.05f) return 3;
    if (inSlip_) return 2;
    return 1;
}

int Game::hullFrame() const {
    float u = heading_;
    if (u < 0.f) u += kTau;
    int i = int(std::lround(u / kTau * 8.f)) % 8;
    if (i < 0) i += 8;
    return i;
}

const char* Game::hint() const {
    if (settle_ > 0.02f) return "HOLD THE BERTH";
    if (inEnd_) return "STOP BEFORE THE END";
    if (inSlip_) return "CREEP TO THE END OF THE SLIP";
    if (y_ > kMouth - 28.f) return "LINE UP THE SLIP";
    return "BERTH BEFORE THE TIDE TURNS";
}

void Game::begin() {
    x_ = kStartX;
    y_ = kStartY;
    heading_ = kStartH;
    speed_ = 0.f;
    raceTime_ = 0.f;
    settle_ = 0.f;
    short_ = 0.f;
    dustT_ = 0.f;
    tickT_ = 0.f;
    stuckT_ = 0.f;
    stuckX_ = x_;
    stuckY_ = y_;
    puffCursor_ = 0;
    inSlip_ = false;
    inEnd_ = false;
    committed_ = false;
    entered_ = false;
    won_ = false;
    over_ = false;
    chimeN_ = 0;
    why_[0] = 0;
    std::snprintf(why_, sizeof why_, "running");
    for (Puff& p : puffs_) p = {};
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
    blip(560.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.76f);
    sys.apu.setEcho(0.12f, 0.18f, 0.07f);
    t_ = 0.f;
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

void Game::human(float& steer, float& gas, float& brake) {
    const gs::Pad& p = sys_->pad;
    steer = 0.f;
    gas = 0.f;
    brake = 0.f;
    if (p.down(gs::BTN_LEFT)) steer += 1.f;
    if (p.down(gs::BTN_RIGHT)) steer -= 1.f;
    if (std::fabs(p.axisX) > 0.18f) steer = clampf(-p.axisX, -1.f, 1.f);
    if (p.down(gs::BTN_UP) || p.down(gs::BTN_C) || p.down(gs::BTN_A) || p.axisY > 0.25f || p.accel > 0.15f) gas = 1.f;
    if (p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.down(gs::BTN_X) || p.axisY < -0.25f || p.brake > 0.15f) brake = 1.f;
    if (p.accel > 0.05f) gas = std::max(gas, p.accel);
    if (p.brake > 0.05f) brake = std::max(brake, p.brake);
}

void Game::pilot(float& steer, float& gas, float& brake) {
    const float north = kPi * 0.5f;
    gas = 0.f;
    brake = 0.f;
    float tx = 0.f;
    float ty = kParkY;
    if (y_ < kMouth - 18.f) {
        tx = 0.f;
        ty = kMouth - 8.f;
    }
    float dx = tx - x_;
    float dy = ty - y_;
    float want = (y_ > kMouth - 4.f) ? north : std::atan2(dy, dx);
    if (y_ > kMouth - 4.f) {
        // Positive bias yaws west of north, back toward the centre when x is east.
        float bias = clampf(x_ * 0.22f, -0.55f, 0.55f);
        if (std::fabs(speed_) < 0.8f && y_ > kBerthY0) bias *= 0.2f;
        want = north + bias;
    }
    float err = wrap(want - heading_);
    steer = clampf(err / 0.28f, -1.f, 1.f);
    if (x_ > 1.4f) steer = std::max(steer, 0.55f);
    if (x_ < -1.4f) steer = std::min(steer, -0.55f);

    if (!committed_) {
        float errN = wrap(north - heading_);
        if (std::fabs(x_) < 2.4f && std::fabs(errN) < 0.22f && y_ > kMouth - 22.f && speed_ > 0.4f && speed_ < 4.2f)
            committed_ = true;
        else if (y_ > kMouth - 6.f) committed_ = true;
    }

    if (y_ < kMouth - 20.f) {
        float cap = 8.2f;
        if (std::fabs(err) > 0.55f) cap = 4.f;
        if (speed_ < cap - 0.4f) gas = 1.f;
        else if (speed_ > cap + 0.3f) brake = 0.6f;
        return;
    }
    if (y_ < kBerthY0 - 10.f) {
        float cap = y_ < kMouth ? 3.4f : 4.6f;
        if (std::fabs(x_) > 2.8f) cap = 2.2f;
        if (speed_ < cap - 0.3f) gas = 0.85f;
        else if (speed_ > cap + 0.25f) brake = 0.7f;
        else gas = 0.15f;
        return;
    }
    if (y_ < kParkY - 1.2f) {
        if (speed_ < 1.15f) gas = 0.45f;
        else if (speed_ > 1.7f) brake = 0.85f;
        return;
    }
    if (speed_ > kStop) brake = 1.f;
    else if (speed_ < -0.08f) gas = 0.35f;
}

void Game::succeed() {
    if (won_) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    speed_ = 0.f;
    std::snprintf(why_, sizeof why_, "berthed");
    chime(5);
    sys_->rumble(0.3f, 0.5f, 150);
    sys_->setLight(40, 170, 70);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    speed_ = 0.f;
    std::snprintf(why_, sizeof why_, "%s", why);
    sys_->apu.noiseBurst(0.38f, 90.f, 0.4f);
    sys_->apu.tone(0, 70.f, 0.06f);
    tone0_ = 0.4f;
    sys_->rumble(0.55f, 0.2f, 170);
    sys_->setLight(180, 30, 20);
}

void Game::physics(float dt, float steer, float gas, float brake) {
    raceTime_ += dt;
    float rate = 2.15f + std::min(std::fabs(speed_), 10.f) * 0.03f;
    heading_ = wrap(heading_ + steer * rate * dt);

    const bool slipNow = std::fabs(x_) < kPierIn && y_ > kMouth && y_ < kHead;
    float cap = slipNow ? 5.6f : 11.5f;
    float drive = gas;
    if (brake > 0.02f) drive = -brake;
    if (drive < -0.02f && speed_ > 0.f) {
        speed_ -= (-drive) * 9.2f * dt;
        if (speed_ < 0.f) speed_ = std::max(speed_, drive * 2.4f);
    } else {
        float target = drive >= 0.f ? drive * cap : drive * 3.2f;
        speed_ += (target - speed_) * (1.f - std::exp(-2.1f * dt));
    }
    if (std::fabs(drive) < 0.04f && std::fabs(speed_) < 1.1f) speed_ *= std::exp(-6.2f * dt);
    speed_ = clampf(speed_, -3.6f, 13.f);

    float c = std::cos(heading_), s = std::sin(heading_);
    x_ += c * speed_ * dt;
    y_ += s * speed_ * dt;

    float u = tideU();
    float ebb = 0.25f + 2.8f * u * u;
    bool slack = std::fabs(x_) < kPierIn - 0.4f && y_ > kMouth + 2.f && y_ < kHead && u < 0.86f;
    if (!slack) x_ += ebb * dt;

    if (x_ < -48.f) {
        x_ = -48.f;
        speed_ *= 0.5f;
    } else if (x_ > 48.f) {
        x_ = 48.f;
        speed_ *= 0.5f;
    }
    if (y_ < 8.f) {
        y_ = 8.f;
        if (s < 0.f) speed_ *= 0.4f;
    }

    auto sample = [&](float along, float beam, float& px, float& py) {
        px = x_ + c * along - s * beam;
        py = y_ + s * along + c * beam;
    };
    float bowX, bowY, sternX, sternY;
    sample(kHalfL, 0.f, bowX, bowY);
    sample(-kHalfL, 0.f, sternX, sternY);

    auto pastEnd = [&](float px, float py) {
        return py >= kHead && px > -kPierOut && px < kPierOut;
    };
    if (pastEnd(x_, y_) || pastEnd(bowX, bowY) || y_ > kHead + 0.4f) {
        fail("missed the end");
        return;
    }

    float hitSpd = std::fabs(speed_);
    float xBefore = x_, yBefore = y_;
    bool scraped = false;
    auto pushPoint = [&](float px, float py, float rad, float x0, float x1, float y0, float y1) {
        float nx = clampf(px, x0, x1);
        float ny = clampf(py, y0, y1);
        float dx = px - nx, dy = py - ny;
        float d2 = dx * dx + dy * dy;
        if (d2 >= rad * rad) return;
        scraped = true;
        float d = std::sqrt(std::max(d2, 1e-6f));
        float pen = rad - d + 0.05f;
        if (d < 0.02f) {
            x_ -= rad;
        } else {
            x_ += dx / d * pen;
            y_ += dy / d * pen;
        }
    };
    float portX, portY, stbdX, stbdY;
    sample(0.f, -kHalfW, portX, portY);
    sample(0.f, kHalfW, stbdX, stbdY);
    const float pts[][3] = {{x_, y_, kHalfW}, {bowX, bowY, 1.1f}, {sternX, sternY, 1.0f}, {portX, portY, 0.7f}, {stbdX, stbdY, 0.7f}};
    for (const auto& p : pts) {
        pushPoint(p[0], p[1], p[2], -kPierOut, -kPierIn, kMouth - 4.f, kPierFar);
        pushPoint(p[0], p[1], p[2], kPierIn, kPierOut, kMouth - 4.f, kPierFar);
        pushPoint(p[0], p[1], p[2], -kPierIn, kPierIn, kHead, kHead + 6.f);
    }
    float shove = std::hypot(x_ - xBefore, y_ - yBefore);
    if (shove > 1.6f) {
        x_ = xBefore + (x_ - xBefore) / shove * 1.6f;
        y_ = yBefore + (y_ - yBefore) / shove * 1.6f;
    }
    if (scraped) {
        if (hitSpd > kScrape) {
            fail("scraped the pier");
            return;
        }
        speed_ *= 0.55f;
        if (thumpT_ <= 0.f) {
            sys_->apu.noiseBurst(0.22f, 180.f, 0.12f);
            thumpT_ = 0.22f;
            sys_->rumble(0.2f, 0.08f, 60);
        }
    }

    c = std::cos(heading_);
    s = std::sin(heading_);
    sample(kHalfL, 0.f, bowX, bowY);
    sample(-kHalfL, 0.f, sternX, sternY);
    if (pastEnd(x_, y_) || pastEnd(bowX, bowY) || y_ > kHead + 0.2f) {
        fail("missed the end");
        return;
    }

    float errH = std::fabs(wrap(kPi * 0.5f - heading_));
    bool inside = std::fabs(bowX) < kPierIn - 0.6f && std::fabs(sternX) < kPierIn - 0.6f && bowY < kHead - 1.5f &&
                  sternY > kMouth + 4.f;
    inSlip_ = std::fabs(x_) < kPierIn - 0.3f && y_ > kMouth + 1.f && y_ < kHead - 1.f;
    bool posed = inside && std::fabs(x_) <= kBerthX && y_ >= kBerthY0 && y_ <= kBerthY1 && errH <= 0.48f;
    inEnd_ = posed;
    if (inSlip_ && !entered_) {
        entered_ = true;
        blip(480.f);
    }

    if (posed && std::fabs(speed_) <= kStop) {
        settle_ += dt;
        speed_ *= std::exp(-7.f * dt);
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
    if (y_ > kMouth + 4.f && y_ < kBerthY0 - 2.f && std::fabs(x_) < kPierIn && !posed && std::fabs(speed_) < 0.32f) {
        short_ += dt;
        if (short_ >= kShort) {
            fail("missed the end");
            return;
        }
    } else {
        short_ = 0.f;
    }

    dustT_ -= dt;
    if (!inSlip_ && dustT_ <= 0.f && std::fabs(speed_) > 3.2f) {
        dustT_ = 0.08f;
        Puff w;
        w.x = x_ - c * 2.4f;
        w.y = y_ - s * 2.4f;
        w.life = 0.7f;
        puffs_[puffCursor_] = w;
        puffCursor_ = (puffCursor_ + 1) % 14;
    }
    for (Puff& w : puffs_)
        if (w.life > 0.f) w.life -= dt;

    if (bot_) {
        stuckT_ += dt;
        if (stuckT_ > 2.2f) {
            float moved = std::hypot(x_ - stuckX_, y_ - stuckY_);
            stuckX_ = x_;
            stuckY_ = y_;
            stuckT_ = 0.f;
            if (moved < 1.4f && y_ < kParkY - 6.f) {
                heading_ = std::atan2(kParkY - y_, -x_);
                speed_ = std::max(speed_, 3.2f);
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
    float hush = mode_ == Mode::Run ? 0.01f + std::fabs(speed_) * 0.0016f : 0.006f;
    sys_->apu.noise(hush, inSlip_ ? 240.f : 520.f, false);
    if (mode_ == Mode::Run && std::fabs(speed_) > 0.4f) {
        float wob = 0.7f + 0.3f * std::sin(t_ * (14.f + std::fabs(speed_)));
        sys_->apu.tone(2, 62.f + std::fabs(speed_) * 6.f, 0.018f * wob);
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
            blip(tideLeft() < 4.f ? 880.f : 440.f);
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
            blip(340.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        } else {
            float steer = 0.f, gas = 0.f, brake = 0.f;
            if (bot_) pilot(steer, gas, brake);
            else human(steer, gas, brake);
            physics(kDt, steer, gas, brake);
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
        if (tideLeft() < 10.f) sys.setLight(170, 70, 20);
        else sys.setLight(40, 90, 140);
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
    float lead = mode_ == Mode::Run ? 8.f : 0.f;
    float gx = x_ + std::cos(heading_) * lead;
    float gy = y_ + std::sin(heading_) * lead;
    float gz = kPlayZoom;
    if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        gx = 0.f;
        gy = (kMouth + kHead) * 0.5f;
        gz = 1.2f;
    }
    float k = 1.f - std::exp(-kDt * (mode_ == Mode::Run ? 4.6f : 2.4f));
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
    v.hudEnabled = true;
    v.roadTime = int(t_ * 40.f);
    float tide = tideU();
    float zoom = std::max(zoom_, 0.25f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (112.f - y) / zoom;
        uint16_t sky = lerpC(gs::rgb4(7, 8, 9), gs::rgb4(3, 4, 6), clampf((wy - 10.f) / 200.f, 0.f, 1.f));
        uint16_t quay = lerpC(gs::rgb4(7, 6, 4), gs::rgb4(4, 4, 3), clampf(wy / 220.f, 0.f, 1.f));
        uint16_t water = lerpC(gs::rgb4(3, 8, 11), gs::rgb4(1, 4, 7), clampf((wy - kMouth) / 120.f, 0.f, 1.f));
        water = lerpC(water, gs::rgb4(6, 6, 4), tide * 0.55f);
        uint16_t col = wy < kMouth - 6.f ? quay : water;
        if (wy < 6.f) col = sky;
        float shim = 0.5f + 0.5f * std::sin(wy * 0.35f + t_ * 2.1f);
        if (wy >= kMouth && shim > 0.92f) col = lerpC(col, gs::rgb4(12, 11, 8), 0.4f);
        v.lineBackdrop[y] = col;
        v.lineFog[y] = uint8_t(clampf((std::fabs(wy - camY_) - 40.f) * 0.04f, 0.f, 4.f));
        gs::RoadLine& r = v.road[y];
        r = {};
        if (wy >= kMouth && wy <= kHead) {
            r.on = true;
            r.cx = 160.f + (0.f - camX_) * zoom;
            r.hw = std::max(3.f, kPierIn * zoom);
            r.v = wy * 16.f;
            r.pal = uint8_t(PAL_SLIP);
            r.band = (int(std::floor(wy / 6.f)) & 1) ? 1 : 0;
            r.style = 2;
            r.left = gs::GROUND_LAND;
            r.right = gs::GROUND_LAND;
        } else if (wy > 8.f && wy < kMouth) {
            r.on = true;
            r.cx = 160.f + (0.f - camX_) * zoom;
            r.hw = std::max(4.f, 5.5f * zoom);
            r.v = wy * 12.f;
            r.pal = uint8_t(PAL_QUAY);
            r.band = (int(std::floor(wy / 8.f)) & 1) ? 1 : 0;
            r.style = 0;
            r.left = gs::GROUND_LAND;
            r.right = gs::GROUND_LAND;
        }
    }

    place(art_.shed, -26.f, 48.f, 10.f, PAL_SHED, 6.f);
    place(art_.shed, 28.f, 72.f, 9.f, PAL_SHED, 6.f);
    for (const auto& p : kLamp) place(art_.lamp, p[0], p[1], 7.f, PAL_LAMP, 3.f);
    for (const auto& p : kTuft) place(art_.tuft, p[0], p[1], 2.4f, PAL_PIER, 2.f);
    for (float py = kMouth; py < kHead; py += 8.f) {
        place(art_.pile, -kPierIn - 1.6f, py, 5.5f, PAL_PIER, 2.f);
        place(art_.pile, kPierIn + 1.6f, py, 5.5f, PAL_PIER, 2.f);
        if (int(py) % 16 == 0) {
            place(art_.cleat, -kPierIn - 3.2f, py + 3.f, 1.6f, PAL_PIER, 1.f);
            place(art_.cleat, kPierIn + 3.2f, py + 3.f, 1.6f, PAL_PIER, 1.f);
        }
    }
    place(art_.endMark, 0.f, kHead - 1.2f, 3.2f, PAL_MARK, 4.f);
    place(art_.flag, -kPierIn - 2.f, kHead - 2.f, 5.f, PAL_CANOPY, 3.f);
    place(art_.flag, kPierIn + 2.f, kHead - 2.f, 5.f, PAL_CANOPY, 3.f, true);

    int flap = int(t_ * 6.f) & 1;
    place(art_.gull[flap], -12.f + std::sin(t_ * 0.7f) * 4.f, 120.f + std::sin(t_) * 2.f, 2.2f, PAL_BIRD, 2.f);
    place(art_.gull[1 - flap], 14.f, 88.f + std::cos(t_ * 0.8f) * 3.f, 1.8f, PAL_BIRD, 2.f, true);

    for (const Puff& w : puffs_) {
        if (w.life <= 0.f) continue;
        place(art_.dust, w.x, w.y, 1.2f + (1.f - w.life) * 1.4f, PAL_DUST, 1.f);
    }

    float sx = 160.f + (x_ - camX_) * zoom_;
    float sy = 112.f - (y_ - camY_) * zoom_;
    float hh = 6.4f * zoom_;
    spr(art_.shaw[hullFrame()], sx + 2.f, sy + 3.f, hh, PAL_SHAW, true);
    spr(art_.shaw[hullFrame()], sx, sy, hh, PAL_SHAW);

    if (mode_ == Mode::Title) {
        spr(art_.title, 160.f, 46.f, 28.f, PAL_BANNER);
        hudC(18, "BERTH IN THE SLIP", PAL_HUD);
        hudC(20, "BEFORE THE TIDE TURNS", PAL_MAP);
        hudC(23, "START TO PEDAL", PAL_HUD);
        hudC(25, "ARROWS STEER  UP PEDALS  DOWN BRAKES", PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        spr(art_.paused, 160.f, 90.f, 26.f, PAL_BANNER);
        hudC(16, "START CONTINUES", PAL_HUD);
    } else if (mode_ == Mode::Win) {
        spr(art_.berthed, 160.f, 40.f, 32.f, PAL_WIN);
        hudC(16, "BERTHED BEFORE THE TIDE", PAL_WIN);
        hudC(18, "THE LEG IS YOURS", PAL_HUD);
        char buf[48];
        std::snprintf(buf, sizeof buf, "TIME %.1f", raceTime_);
        hudC(21, buf, PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        const gs::Mipped* ban = &art_.leg;
        if (std::strcmp(why_, "missed the end") == 0) ban = &art_.missed;
        else if (std::strcmp(why_, "tide turned") == 0) ban = &art_.tide;
        else if (std::strcmp(why_, "scraped the pier") == 0) ban = &art_.scraped;
        spr(*ban, 160.f, 42.f, 24.f, PAL_ALERT);
        hudC(16, "THE LEG FAILS", PAL_ALERT);
        hudC(20, "START TO TRY THE SLIP AGAIN", PAL_HUD);
    } else {
        char buf[48];
        std::snprintf(buf, sizeof buf, "TIDE %.0f", tideLeft());
        hud(1, 1, buf, tideLeft() < 10.f ? PAL_ALERT : PAL_HUD);
        std::snprintf(buf, sizeof buf, "SPD %.1f", speed_);
        hud(30, 1, buf, PAL_HUD);
        if (inEnd_) spr(art_.inSlip, 160.f, 24.f, 16.f, PAL_WIN);
        hudC(26, hint(), inSlip_ ? PAL_WIN : PAL_HUD);
    }
}

}  // namespace rickslip

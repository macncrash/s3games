#include "barge.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace bargeslip {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;

constexpr float kStartX = -46.f;
constexpr float kStartY = 8.f;
constexpr float kStartH = 0.55f;

// Slip opens west. Water sits between the two piers, short of the head wall.
constexpr float kPierW = 10.f;
constexpr float kPierE = 50.f;
constexpr float kSouth0 = 6.f;
constexpr float kSouth1 = 14.f;
constexpr float kNorth0 = 30.f;
constexpr float kNorth1 = 38.f;
constexpr float kHeadX = 42.f;
constexpr float kMouthX = 10.f;
constexpr float kLaneY = 22.f;
constexpr float kParkX = 32.f;
constexpr float kBerthX0 = 28.5f;
constexpr float kBerthX1 = 35.5f;
constexpr float kBerthY0 = 18.2f;
constexpr float kBerthY1 = 25.8f;

constexpr float kBow = 6.6f;
constexpr float kStern = 5.6f;
constexpr float kBeam = 2.05f;

constexpr float kTide = 52.f;
constexpr float kHold = 0.45f;
constexpr float kStop = 0.38f;

constexpr float kPlayZoom = 4.6f;
constexpr float kTitleZoom = 1.85f;
constexpr float kTitleCamX = 12.f;
constexpr float kTitleCamY = 22.f;

struct Box {
    float x0, x1, y0, y1;
};

const Box kSolid[] = {
    {kPierW, kPierE, kSouth0, kSouth1},
    {kPierW, kPierE, kNorth0, kNorth1},
    {kHeadX, kPierE, kSouth0, kNorth1},
};

float wrap(float a) {
    while (a > kPi) a -= kTau;
    while (a < -kPi) a += kTau;
    return a;
}

bool inside(const Box& b, float x, float y) { return x >= b.x0 && x <= b.x1 && y >= b.y0 && y <= b.y1; }

bool solidAt(float x, float y) {
    for (const Box& b : kSolid)
        if (inside(b, x, y)) return true;
    return false;
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
    if (mode_ == Mode::Title) return 0.1f;
    return std::clamp(raceTime_ / kTide, 0.f, 1.f);
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (inBerth_ && std::fabs(speed_) < 1.2f) return 3;
    if (inSlip_) return 2;
    return 1;
}

int Game::hullFrame() const {
    float u = std::fmod(heading_, kTau);
    if (u < 0.f) u += kTau;
    int i = int(std::lround(u / kTau * 8.f)) % 8;
    if (i < 0) i += 8;
    return i;
}

const char* Game::hint() const {
    if (inBerth_ && std::fabs(speed_) > 0.55f) return "EASE THE BARGE AND HOLD";
    if (inBerth_) return "HOLD THE BERTH";
    if (inSlip_) return "THE END OF THE SLIP IS AHEAD";
    if (tideU() > 0.55f) return "THE TIDE IS TURNING";
    if (x_ > -8.f) return "LINE THE BOW ON THE SLIP";
    return "BERTH BEFORE THE TIDE TURNS";
}

void Game::begin() {
    x_ = kStartX;
    y_ = kStartY;
    heading_ = kStartH;
    speed_ = 0.f;
    throttle_ = 0.f;
    raceTime_ = 0.f;
    hold_ = 0.f;
    wakeT_ = 0.f;
    tickT_ = 0.f;
    stuckT_ = 0.f;
    stuckX_ = x_;
    stuckY_ = y_;
    wakeCursor_ = 0;
    inSlip_ = false;
    inBerth_ = false;
    won_ = false;
    over_ = false;
    chimeN_ = 0;
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
    blip(520.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.72f);
    sys.apu.setEcho(0.1f, 0.16f, 0.06f);
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
    if (stop) throttle_ = std::max(-1.f, throttle_ - kDt * 1.8f);
    else if (go) throttle_ = std::min(1.f, throttle_ + kDt * 1.05f);
    else {
        float decay = std::fabs(speed_) < 0.4f ? 2.6f : 0.45f;
        if (throttle_ > 0.f) throttle_ = std::max(0.f, throttle_ - kDt * decay);
        else throttle_ = std::min(0.f, throttle_ + kDt * decay);
    }
    throttle = throttle_;
}

void Game::pilot(float& steer, float& throttle) {
    float yErr = kLaneY - y_;
    float want;
    float th;
    if (x_ < 8.f || std::fabs(yErr) > 5.5f) {
        want = std::atan2(kLaneY - y_, 20.f - x_);
        th = std::fabs(yErr) > 7.f ? 0.78f : 0.62f;
    } else if (x_ < kParkX - 1.2f) {
        want = std::clamp(yErr * 0.18f, -0.35f, 0.35f);
        th = 0.38f;
    } else {
        want = std::clamp(yErr * 0.28f, -0.22f, 0.22f);
        th = 0.f;
    }
    float err = wrap(want - heading_);
    steer = std::clamp(err / 0.32f, -1.f, 1.f);
    if (std::fabs(err) > 0.85f) th *= 0.12f;
    else if (std::fabs(err) > 0.4f) th *= 0.35f;

    if (x_ >= kParkX - 1.6f && x_ < kHeadX && std::fabs(yErr) < 4.2f) {
        if (x_ < kBerthX0 + 0.6f && std::fabs(speed_) < 0.45f) throttle = 0.28f;
        else if (speed_ > 0.18f) throttle = -0.9f;
        else if (speed_ < -0.08f) throttle = 0.22f;
        else throttle = 0.f;
        if (x_ > kBerthX1 - 0.6f) throttle = speed_ > -0.12f ? -0.7f : 0.f;
        return;
    }
    float cap = x_ > 12.f ? 2.6f : 6.4f;
    if (speed_ > cap) th = -0.55f;
    throttle = th;
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
    sys_->rumble(0.25f, 0.45f, 140);
    sys_->setLight(40, 160, 70);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    speed_ = 0.f;
    throttle_ = 0.f;
    std::snprintf(why_, sizeof why_, "%s", why);
    sys_->apu.noiseBurst(0.38f, 80.f, 0.4f);
    sys_->apu.tone(0, 64.f, 0.06f);
    tone0_ = 0.4f;
    sys_->rumble(0.5f, 0.18f, 160);
    sys_->setLight(170, 30, 20);
}

void Game::physics(float dt, float steer, float throttle) {
    raceTime_ += dt;
    if (raceTime_ >= kTide) {
        fail("tide turned");
        return;
    }

    float rate = 1.55f + std::min(std::fabs(speed_), 8.f) * 0.02f;
    heading_ = wrap(heading_ + steer * rate * dt);

    inSlip_ = x_ > kMouthX + 0.5f && x_ < kHeadX - 0.4f && y_ > kSouth1 + 0.4f && y_ < kNorth0 - 0.4f;
    float cap = inSlip_ ? 3.6f : 7.4f;
    if (throttle < -0.02f && speed_ > 0.f) {
        speed_ -= (-throttle) * 6.4f * dt;
        if (speed_ < 0.f) speed_ = std::max(speed_, throttle * 2.2f);
    } else {
        float target = throttle >= 0.f ? throttle * cap : throttle * 2.8f;
        speed_ += (target - speed_) * (1.f - std::exp(-1.7f * dt));
    }
    if (std::fabs(throttle) < 0.04f && std::fabs(speed_) < 0.7f) speed_ *= std::exp(-5.f * dt);
    speed_ = std::clamp(speed_, -2.6f, 8.5f);

    float c = std::cos(heading_), s = std::sin(heading_);
    x_ += c * speed_ * dt;
    y_ += s * speed_ * dt;

    float u = std::clamp(raceTime_ / kTide, 0.f, 1.f);
    float ebb = 0.2f + 2.2f * u * u;
    bool sheltered = inSlip_ && u < 0.9f;
    y_ -= (sheltered ? ebb * 0.06f : ebb) * dt;

    auto sample = [&](float along, float beam, float& px, float& py) {
        px = x_ + c * along - s * beam;
        py = y_ + s * along + c * beam;
    };
    float pts[5][2];
    sample(kBow, 0.f, pts[0][0], pts[0][1]);
    sample(-kStern, 0.f, pts[1][0], pts[1][1]);
    sample(0.4f, -kBeam, pts[2][0], pts[2][1]);
    sample(0.4f, kBeam, pts[3][0], pts[3][1]);
    sample(-1.4f, 0.f, pts[4][0], pts[4][1]);

    float hit = 0.f;
    for (int i = 0; i < 5; i++) {
        float px = pts[i][0], py = pts[i][1];
        if (!solidAt(px, py)) continue;
        float best = 1e9f;
        float ox = 0.f, oy = 0.f;
        for (const Box& b : kSolid) {
            if (!inside(b, px, py)) continue;
            float dl = px - b.x0, dr = b.x1 - px, db = py - b.y0, dtb = b.y1 - py;
            float m = dl;
            float vx = -1.f, vy = 0.f;
            if (dr < m) {
                m = dr;
                vx = 1.f;
                vy = 0.f;
            }
            if (db < m) {
                m = db;
                vx = 0.f;
                vy = -1.f;
            }
            if (dtb < m) {
                m = dtb;
                vx = 0.f;
                vy = 1.f;
            }
            if (m < best) {
                best = m;
                ox = vx * (m + 0.1f);
                oy = vy * (m + 0.1f);
            }
        }
        x_ += ox;
        y_ += oy;
        hit = std::max(hit, std::fabs(speed_));
    }
    if (hit > 2.6f) {
        fail("scraped the pier");
        return;
    }
    if (hit > 0.15f) {
        speed_ *= 0.4f;
        if (thumpT_ <= 0.f) {
            sys_->apu.noiseBurst(0.18f, 180.f, 0.08f);
            thumpT_ = 0.2f;
        }
    }

    if (x_ + c * kBow > kHeadX - 0.2f && speed_ > 0.55f && y_ > kSouth1 && y_ < kNorth0) {
        fail("missed the end");
        return;
    }
    x_ = std::clamp(x_, -80.f, 72.f);
    y_ = std::clamp(y_, -20.f, 96.f);

    inSlip_ = x_ > kMouthX && x_ < kHeadX && y_ > kSouth1 + 0.25f && y_ < kNorth0 - 0.25f;
    inBerth_ = x_ > kBerthX0 && x_ < kBerthX1 && y_ > kBerthY0 && y_ < kBerthY1 && std::fabs(wrap(heading_)) < 0.7f;
    if (inBerth_ && std::fabs(speed_) < kStop) {
        hold_ += dt;
        if (hold_ >= kHold) succeed();
    } else {
        hold_ = 0.f;
    }

    if (std::fabs(speed_) > 1.0f) {
        wakeT_ -= dt;
        if (wakeT_ <= 0.f) {
            Wake& w = wakes_[wakeCursor_++ % 12];
            w.x = x_ - c * 4.4f;
            w.y = y_ - s * 4.4f;
            w.life = 0.75f;
            wakeT_ = 0.1f;
        }
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
            if (moved < 1.2f && x_ < kMouthX) {
                heading_ = std::atan2(kLaneY - y_, 6.f - x_);
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
    float water = mode_ == Mode::Run ? 0.01f + std::fabs(speed_) * 0.0004f + tideU() * 0.007f : 0.006f;
    sys_->apu.noise(inSlip_ ? water * 0.35f : water, inSlip_ ? 240.f : 560.f, false);
    if (mode_ == Mode::Run && (throttle_ > 0.05f || std::fabs(speed_) > 1.0f)) {
        float wob = 0.75f + 0.25f * std::sin(t_ * (10.f + std::max(0.f, throttle_) * 12.f));
        float vol = (0.012f + std::max(0.f, throttle_) * 0.022f) * wob;
        sys_->apu.tone(2, 48.f + std::max(0.f, throttle_) * 28.f, vol);
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
    if (mode_ == Mode::Run && tideLeft() < 12.f && tideLeft() > 0.f) {
        tickT_ -= dt;
        if (tickT_ <= 0.f) {
            blip(tideLeft() < 4.f ? 820.f : 480.f);
            tickT_ = tideLeft() < 4.f ? 0.25f : 0.55f;
        }
    }
    if (chimeN_ > 0) {
        chimeT_ -= dt;
        if (chimeT_ <= 0.f) {
            static const float notes[] = {392.f, 494.f, 587.f, 698.f, 784.f};
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
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C)) startRun();
        else if (pad.pressed(gs::BTN_MODE)) sys.quit();
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(300.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        } else {
            float steer = 0.f, thr = 0.f;
            if (bot_) pilot(steer, thr);
            else human(steer, thr);
            throttle_ = thr;
            physics(kDt, steer, thr);
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
        else sys.setLight(30, 100, 50);
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
    float lead = mode_ == Mode::Run ? 5.f : 0.f;
    float gx = x_ + std::cos(heading_) * lead;
    float gy = y_ + std::sin(heading_) * lead;
    float gz = kPlayZoom;
    if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        gx = (kMouthX + kHeadX) * 0.5f;
        gy = kLaneY;
        gz = 3.6f;
    }
    float k = 1.f - std::exp(-kDt * (mode_ == Mode::Run ? 3.4f : 2.2f));
    camX_ += (gx - camX_) * k;
    camY_ += (gy - camY_) * k;
    zoom_ += (gz - zoom_) * k;
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char ch = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || ch < 32 || ch > 127 || ch == ' ') continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[ch - 32], pal));
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
    v.hudEnabled = true;
    v.roadTime = int(t_ * 30.f);
    float tide = tideU();
    float zoom = std::max(zoom_, 0.2f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (112.f - y) / zoom;
        float u = std::clamp((wy + 12.f) / 100.f, 0.f, 1.f);
        uint16_t water = lerpC(gs::rgb4(2, 7, 10), gs::rgb4(1, 3, 6), 1.f - u);
        float shim = 0.5f + 0.5f * std::sin(wy * 0.3f + t_ * 1.4f);
        if (shim > 0.95f) water = lerpC(water, gs::rgb4(11, 12, 8), 0.4f);
        water = lerpC(water, gs::rgb4(5, 5, 3), tide * 0.5f);
        v.lineBackdrop[y] = water;
        v.lineFog[y] = uint8_t(std::clamp(int((std::fabs(wy - camY_) - 30.f) * 0.07f), 0, 6));
        gs::RoadLine& r = v.road[y];
        r = {};
        if (wy > kSouth1 && wy < kNorth0) {
            float sx0 = 160.f + (kMouthX - camX_) * zoom;
            float sx1 = 160.f + (kHeadX - camX_) * zoom;
            r.on = true;
            r.cx = (sx0 + sx1) * 0.5f;
            r.hw = std::max(2.f, std::fabs(sx1 - sx0) * 0.5f);
            r.v = camX_ * 5.f + t_ * 8.f;
            r.pal = uint8_t(PAL_SLIP);
            r.band = (int(std::floor(wy / 3.f)) & 1) ? 1 : 0;
            r.style = 2;
            r.left = gs::GROUND_LAND;
            r.right = gs::GROUND_LAND;
        } else if (wy >= kSouth0 && wy <= kNorth1) {
            r.on = true;
            r.cx = 160.f + ((kPierW + kPierE) * 0.5f - camX_) * zoom;
            r.hw = std::max(4.f, (kPierE - kPierW) * 0.5f * zoom);
            r.v = wy * 8.f;
            r.pal = uint8_t(PAL_SHORE);
            r.band = (int(std::floor(wy * 0.2f)) & 1) ? 1 : 0;
            r.style = 0;
            r.left = 0;
            r.right = 0;
        }
    }

    auto banner = [&](const gs::Mipped& m, float x, float y, int pal) { spr(m, x, y, float(m.h), pal); };
    if (mode_ == Mode::Title) banner(art_.title, 160.f, 20.f, PAL_BANNER);
    else if (mode_ == Mode::Pause) banner(art_.paused, 160.f, 96.f, PAL_BANNER);
    else if (mode_ == Mode::Fail) {
        const gs::Mipped* msg = &art_.missed;
        if (std::strcmp(why_, "tide turned") == 0) msg = &art_.tide;
        else if (std::strcmp(why_, "scraped the pier") == 0) msg = &art_.scraped;
        banner(*msg, 160.f, 24.f, PAL_ALERT);
        banner(art_.leg, 160.f, 52.f, PAL_ALERT);
    } else if (mode_ == Mode::Win) {
        banner(art_.berthed, 160.f, 18.f, PAL_WIN);
        banner(art_.inSlip, 160.f, 48.f, PAL_WIN);
    }

    place(art_.pierH, (kPierW + kPierE) * 0.5f, (kSouth0 + kSouth1) * 0.5f, kSouth1 - kSouth0, PAL_PIER, 8.f);
    place(art_.pierH, (kPierW + kPierE) * 0.5f, (kNorth0 + kNorth1) * 0.5f, kNorth1 - kNorth0, PAL_PIER, 8.f);
    place(art_.pierV, (kHeadX + kPierE) * 0.5f, kLaneY, kNorth1 - kSouth0, PAL_PIER, 10.f);
    place(art_.lamp, kHeadX - 1.4f, kLaneY + 2.6f, 6.f, PAL_LAMP, 5.f);
    place(art_.flag, kMouthX + 1.2f, kNorth0 + 1.6f, 4.6f, PAL_MARK, 4.f);
    place(art_.flag, kMouthX + 1.2f, kSouth1 - 1.6f, 4.6f, PAL_MARK, 4.f);
    place(art_.buoy, kMouthX - 2.6f, kSouth1 - 0.8f, 3.2f, PAL_ALERT, 3.f);
    place(art_.buoy, kMouthX - 2.6f, kNorth0 + 0.8f, 3.2f, PAL_ALERT, 3.f);
    place(art_.cleat, 33.f, kSouth1 + 1.1f, 1.0f, PAL_PIER, 0.f);
    place(art_.cleat, 33.f, kNorth0 - 1.1f, 1.0f, PAL_PIER, 0.f);
    place(art_.dot, kBerthX0, kBerthY0, 0.65f, PAL_WIN, 3.f);
    place(art_.dot, kBerthX1, kBerthY0, 0.65f, PAL_WIN, 3.f);
    place(art_.dot, kBerthX0, kBerthY1, 0.65f, PAL_WIN, 3.f);
    place(art_.dot, kBerthX1, kBerthY1, 0.65f, PAL_WIN, 3.f);

    for (const Wake& w : wakes_) {
        if (w.life <= 0.f) continue;
        place(art_.foam, w.x, w.y, 1.6f + (1.f - w.life) * 1.8f, PAL_HULL, 2.f);
    }

    float bob = (inSlip_ ? 0.12f : 0.28f) * std::sin(t_ * 1.8f);
    float bsx = 160.f + (x_ - camX_) * zoom;
    float bsy = 112.f - (y_ - camY_) * zoom + bob;
    float hullH = 11.5f * zoom;
    if (mode_ == Mode::Title) hullH = std::max(hullH, 26.f);
    const gs::Mipped& hull = art_.hull[hullFrame()];
    spr(hull, bsx + 2.f, bsy + 3.f, hullH, PAL_HULL, true);
    spr(hull, bsx, bsy, hullH, PAL_HULL, false);

    if (mode_ == Mode::Run || mode_ == Mode::Pause || mode_ == Mode::Title) {
        float psx = 160.f + (kParkX - camX_) * zoom;
        float psy = 112.f - (kLaneY - camY_) * zoom;
        if (mode_ != Mode::Title && (psx < 12.f || psx > 308.f || psy < 12.f || psy > 212.f)) {
            float dx = psx - 160.f, dy = psy - 112.f;
            float k = 1.f;
            if (std::fabs(dx) > 1.f) k = std::min(k, 136.f / std::fabs(dx));
            if (std::fabs(dy) > 1.f) k = std::min(k, 84.f / std::fabs(dy));
            spr(art_.pin, 160.f + dx * k, 112.f + dy * k, 12.f, PAL_MARK, false);
        }
    }

    if (mode_ == Mode::Run || mode_ == Mode::Pause) {
        spr(art_.panel, 276.f, 78.f, 74.f, PAL_MAP, false);
        auto dot = [&](float wx, float wy, int pal, float h) {
            spr(art_.dot, 276.f + (wx - 8.f) * 0.65f, 78.f - (wy - 22.f) * 0.65f, h, pal, false);
        };
        dot(kMouthX, kSouth1, PAL_BANNER, 3.f);
        dot(kMouthX, kNorth0, PAL_BANNER, 3.f);
        dot(kHeadX, kLaneY, PAL_WIN, 4.f);
        dot(x_, y_, PAL_ALERT, 5.f);

        char line[40];
        std::snprintf(line, sizeof line, "TIDE %4.1f", tideLeft());
        hud(1, 1, line, tideLeft() < 12.f ? PAL_ALERT : PAL_HUD);
        std::snprintf(line, sizeof line, "LEG   %02d", int(raceTime_));
        hud(1, 2, line, PAL_HUD);
        hudC(24, hint(), inBerth_ ? PAL_WIN : PAL_HUD);
        hudC(26, "BERTH IN THE SLIP", PAL_BANNER);
    } else if (mode_ == Mode::Title) {
        hudC(23, "BERTH BEFORE THE TIDE TURNS", PAL_HUD);
        hudC(25, "MISSING THE END FAILS THE LEG", PAL_ALERT);
        hudC(26, "START  THROTTLE AND RUDDER", PAL_BANNER);
    }
}

}  // namespace bargeslip

#include "tram.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace tramslip {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;

constexpr float kStartX = -42.f;
constexpr float kStartY = 10.f;
constexpr float kStartH = 0.42f;

// Slip opens west. Rails run between the two quays, short of the head wall.
constexpr float kPierW = 8.f;
constexpr float kPierE = 48.f;
constexpr float kSouth0 = 4.f;
constexpr float kSouth1 = 12.f;
constexpr float kNorth0 = 28.f;
constexpr float kNorth1 = 36.f;
constexpr float kHeadX = 40.f;
constexpr float kMouthX = 8.f;
constexpr float kLaneY = 20.f;
constexpr float kParkX = 30.f;
constexpr float kBerthX0 = 26.5f;
constexpr float kBerthX1 = 33.5f;
constexpr float kBerthY0 = 16.4f;
constexpr float kBerthY1 = 23.6f;

constexpr float kBow = 5.4f;
constexpr float kStern = 4.8f;
constexpr float kBeam = 1.55f;

constexpr float kTide = 50.f;
constexpr float kHold = 0.4f;
constexpr float kStop = 0.36f;

constexpr float kPlayZoom = 5.1f;
constexpr float kTitleZoom = 1.7f;
constexpr float kTitleCamX = 10.f;
constexpr float kTitleCamY = 20.f;

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
    if (mode_ == Mode::Title) return 0.08f;
    return std::clamp(raceTime_ / kTide, 0.f, 1.f);
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (inBerth_ && std::fabs(speed_) < 1.2f) return 3;
    if (inSlip_) return 2;
    return 1;
}

int Game::tramFrame() const {
    float u = std::fmod(heading_, kTau);
    if (u < 0.f) u += kTau;
    int i = int(std::lround(u / kTau * 8.f)) % 8;
    if (i < 0) i += 8;
    return i;
}

const char* Game::hint() const {
    if (inBerth_ && std::fabs(speed_) > 0.5f) return "EASE THE TRAM AND HOLD";
    if (inBerth_) return "HOLD THE BERTH";
    if (inSlip_) return "THE END OF THE SLIP IS AHEAD";
    if (tideU() > 0.55f) return "THE TIDE IS TURNING";
    if (x_ > -6.f) return "LINE THE TRAM ON THE RAILS";
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
    bellT_ = 0.6f;
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
    blip(640.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.7f);
    sys.apu.setEcho(0.08f, 0.14f, 0.05f);
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
    if (stop) throttle_ = std::max(-1.f, throttle_ - kDt * 1.9f);
    else if (go) throttle_ = std::min(1.f, throttle_ + kDt * 1.1f);
    else {
        float decay = std::fabs(speed_) < 0.4f ? 2.8f : 0.5f;
        if (throttle_ > 0.f) throttle_ = std::max(0.f, throttle_ - kDt * decay);
        else throttle_ = std::min(0.f, throttle_ + kDt * decay);
    }
    throttle = throttle_;
}

void Game::pilot(float& steer, float& throttle) {
    float yErr = kLaneY - y_;
    float want;
    float th;
    if (x_ < 6.f || std::fabs(yErr) > 4.8f) {
        want = std::atan2(kLaneY - y_, 18.f - x_);
        th = std::fabs(yErr) > 6.f ? 0.8f : 0.64f;
    } else if (x_ < kParkX - 1.0f) {
        want = std::clamp(yErr * 0.2f, -0.32f, 0.32f);
        th = 0.36f;
    } else {
        want = std::clamp(yErr * 0.3f, -0.2f, 0.2f);
        th = 0.f;
    }
    float err = wrap(want - heading_);
    steer = std::clamp(err / 0.3f, -1.f, 1.f);
    if (std::fabs(err) > 0.8f) th *= 0.12f;
    else if (std::fabs(err) > 0.38f) th *= 0.35f;

    if (x_ >= kParkX - 1.4f && x_ < kHeadX && std::fabs(yErr) < 3.8f) {
        if (x_ < kBerthX0 + 0.5f && std::fabs(speed_) < 0.42f) throttle = 0.26f;
        else if (speed_ > 0.16f) throttle = -0.92f;
        else if (speed_ < -0.08f) throttle = 0.2f;
        else throttle = 0.f;
        if (x_ > kBerthX1 - 0.5f) throttle = speed_ > -0.1f ? -0.7f : 0.f;
        return;
    }
    float cap = x_ > 10.f ? 2.4f : 6.2f;
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
    sys_->rumble(0.2f, 0.4f, 120);
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
    sys_->apu.noiseBurst(0.36f, 90.f, 0.35f);
    sys_->apu.tone(0, 70.f, 0.06f);
    tone0_ = 0.4f;
    sys_->rumble(0.5f, 0.16f, 150);
    sys_->setLight(170, 30, 20);
}

void Game::physics(float dt, float steer, float throttle) {
    raceTime_ += dt;
    if (raceTime_ >= kTide) {
        fail("tide turned");
        return;
    }

    float rate = 1.7f + std::min(std::fabs(speed_), 7.f) * 0.02f;
    heading_ = wrap(heading_ + steer * rate * dt);

    inSlip_ = x_ > kMouthX + 0.4f && x_ < kHeadX - 0.4f && y_ > kSouth1 + 0.35f && y_ < kNorth0 - 0.35f;
    float cap = inSlip_ ? 3.2f : 6.8f;
    if (throttle < -0.02f && speed_ > 0.f) {
        speed_ -= (-throttle) * 6.8f * dt;
        if (speed_ < 0.f) speed_ = std::max(speed_, throttle * 2.0f);
    } else {
        float target = throttle >= 0.f ? throttle * cap : throttle * 2.6f;
        speed_ += (target - speed_) * (1.f - std::exp(-1.9f * dt));
    }
    if (std::fabs(throttle) < 0.04f && std::fabs(speed_) < 0.65f) speed_ *= std::exp(-5.4f * dt);
    speed_ = std::clamp(speed_, -2.4f, 7.8f);

    float c = std::cos(heading_), s = std::sin(heading_);
    x_ += c * speed_ * dt;
    y_ += s * speed_ * dt;

    float u = std::clamp(raceTime_ / kTide, 0.f, 1.f);
    float ebb = 0.18f + 2.05f * u * u;
    bool sheltered = inSlip_ && u < 0.88f;
    y_ -= (sheltered ? ebb * 0.05f : ebb) * dt;

    auto sample = [&](float along, float beam, float& px, float& py) {
        px = x_ + c * along - s * beam;
        py = y_ + s * along + c * beam;
    };
    float pts[5][2];
    sample(kBow, 0.f, pts[0][0], pts[0][1]);
    sample(-kStern, 0.f, pts[1][0], pts[1][1]);
    sample(0.2f, -kBeam, pts[2][0], pts[2][1]);
    sample(0.2f, kBeam, pts[3][0], pts[3][1]);
    sample(-1.2f, 0.f, pts[4][0], pts[4][1]);

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
                ox = vx * (m + 0.08f);
                oy = vy * (m + 0.08f);
            }
        }
        x_ += ox;
        y_ += oy;
        hit = std::max(hit, std::fabs(speed_));
    }
    if (hit > 2.4f) {
        fail("scraped the quay");
        return;
    }
    if (hit > 0.12f) {
        speed_ *= 0.35f;
        if (thumpT_ <= 0.f) {
            sys_->apu.noiseBurst(0.16f, 200.f, 0.07f);
            thumpT_ = 0.18f;
        }
    }

    if (x_ + c * kBow > kHeadX - 0.15f && speed_ > 0.5f && y_ > kSouth1 && y_ < kNorth0) {
        fail("missed the end");
        return;
    }
    x_ = std::clamp(x_, -80.f, 70.f);
    y_ = std::clamp(y_, -16.f, 90.f);

    inSlip_ = x_ > kMouthX && x_ < kHeadX && y_ > kSouth1 + 0.2f && y_ < kNorth0 - 0.2f;
    inBerth_ = x_ > kBerthX0 && x_ < kBerthX1 && y_ > kBerthY0 && y_ < kBerthY1 && std::fabs(wrap(heading_)) < 0.65f;
    if (inBerth_ && std::fabs(speed_) < kStop) {
        hold_ += dt;
        if (hold_ >= kHold) succeed();
    } else {
        hold_ = 0.f;
    }

    if (std::fabs(speed_) > 0.9f) {
        wakeT_ -= dt;
        if (wakeT_ <= 0.f) {
            Wake& w = wakes_[wakeCursor_++ % 12];
            w.x = x_ - c * 3.6f;
            w.y = y_ - s * 3.6f;
            w.life = 0.7f;
            wakeT_ = 0.09f;
        }
    }
    for (Wake& w : wakes_)
        if (w.life > 0.f) w.life -= dt;

    if (bot_) {
        stuckT_ += dt;
        if (stuckT_ > 2.2f) {
            float moved = std::hypot(x_ - stuckX_, y_ - stuckY_);
            stuckX_ = x_;
            stuckY_ = y_;
            stuckT_ = 0.f;
            if (moved < 1.1f && x_ < kMouthX) {
                heading_ = std::atan2(kLaneY - y_, 6.f - x_);
                speed_ = std::max(speed_, 3.0f);
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
    float water = mode_ == Mode::Run ? 0.008f + std::fabs(speed_) * 0.00035f + tideU() * 0.006f : 0.005f;
    sys_->apu.noise(inSlip_ ? water * 0.3f : water, inSlip_ ? 220.f : 500.f, false);
    if (mode_ == Mode::Run && (throttle_ > 0.05f || std::fabs(speed_) > 0.8f)) {
        float wob = 0.8f + 0.2f * std::sin(t_ * (14.f + std::max(0.f, throttle_) * 10.f));
        float vol = (0.01f + std::max(0.f, throttle_) * 0.02f) * wob;
        sys_->apu.tone(2, 62.f + std::max(0.f, throttle_) * 22.f, vol);
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
    if (mode_ == Mode::Run) {
        bellT_ -= dt;
        if (bellT_ <= 0.f) {
            blip(inSlip_ ? 740.f : 520.f);
            bellT_ = inSlip_ ? 1.4f : 2.2f;
        }
        if (tideLeft() < 12.f && tideLeft() > 0.f) {
            tickT_ -= dt;
            if (tickT_ <= 0.f) {
                blip(tideLeft() < 4.f ? 880.f : 460.f);
                tickT_ = tideLeft() < 4.f ? 0.22f : 0.5f;
            }
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
            blip(280.f);
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
        else sys.setLight(40, 90, 40);
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
    float lead = mode_ == Mode::Run ? 4.2f : 0.f;
    float gx = x_ + std::cos(heading_) * lead;
    float gy = y_ + std::sin(heading_) * lead;
    float gz = kPlayZoom;
    if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        gx = (kMouthX + kHeadX) * 0.5f;
        gy = kLaneY;
        gz = 3.8f;
    }
    float k = 1.f - std::exp(-kDt * (mode_ == Mode::Run ? 3.6f : 2.2f));
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

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal) {
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
    sys_->vdp.sprite(spt);
}

void Game::place(const gs::Mipped& m, float wx, float wy, float worldH, int pal, float minPx) {
    float sx = 160.f + (wx - camX_) * zoom_;
    float sy = 112.f - (wy - camY_) * zoom_;
    float h = worldH * zoom_;
    if (h < minPx) h = minPx;
    spr(m, sx, sy, h, pal);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.hudEnabled = true;
    v.roadTime = int(t_ * 24.f);
    float tide = tideU();
    float zoom = std::max(zoom_, 0.2f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (112.f - y) / zoom;
        float u = std::clamp((wy + 8.f) / 90.f, 0.f, 1.f);
        uint16_t water = lerpC(gs::rgb4(2, 6, 11), gs::rgb4(1, 2, 5), 1.f - u);
        float shim = 0.5f + 0.5f * std::sin(wy * 0.35f + t_ * 1.6f);
        if (shim > 0.96f) water = lerpC(water, gs::rgb4(10, 12, 9), 0.35f);
        water = lerpC(water, gs::rgb4(6, 5, 3), tide * 0.45f);
        v.lineBackdrop[y] = water;
        v.lineFog[y] = uint8_t(std::clamp(int((std::fabs(wy - camY_) - 28.f) * 0.06f), 0, 6));
        gs::RoadLine& r = v.road[y];
        r = {};
        if (wy > kSouth1 && wy < kNorth0) {
            float sx0 = 160.f + (kMouthX - camX_) * zoom;
            float sx1 = 160.f + (kHeadX - camX_) * zoom;
            r.on = true;
            r.cx = (sx0 + sx1) * 0.5f;
            r.hw = std::max(2.f, std::fabs(sx1 - sx0) * 0.5f);
            r.v = camX_ * 4.f + t_ * 6.f;
            r.pal = uint8_t(PAL_SLIP);
            r.band = (int(std::floor(wy / 2.5f)) & 1) ? 1 : 0;
            r.style = 2;
            r.left = gs::GROUND_LAND;
            r.right = gs::GROUND_LAND;
        } else if (wy >= kSouth0 && wy <= kNorth1) {
            r.on = true;
            r.cx = 160.f + ((kPierW + kPierE) * 0.5f - camX_) * zoom;
            r.hw = std::max(4.f, (kPierE - kPierW) * 0.5f * zoom);
            r.v = wy * 7.f;
            r.pal = uint8_t(PAL_SHORE);
            r.band = (int(std::floor(wy * 0.25f)) & 1) ? 1 : 0;
            r.style = 0;
            r.left = 0;
            r.right = 0;
        }
    }

    for (float x = kMouthX + 2.f; x < kHeadX - 2.f; x += 4.2f) place(art_.rail, x, kLaneY, 2.4f, PAL_MARK, 3.f);
    for (float x = kPierW + 3.f; x < kPierE - 2.f; x += 7.f) {
        place(art_.quayH, x, (kSouth0 + kSouth1) * 0.5f, 3.2f, PAL_QUAY);
        place(art_.quayH, x, (kNorth0 + kNorth1) * 0.5f, 3.2f, PAL_QUAY);
    }
    for (float y = kSouth0 + 3.f; y < kNorth1 - 2.f; y += 7.f) place(art_.quayV, (kHeadX + kPierE) * 0.5f, y, 6.f, PAL_QUAY);
    place(art_.flag, kMouthX + 1.2f, kNorth0 + 1.4f, 4.2f, PAL_ALERT);
    place(art_.flag, kMouthX + 1.2f, kSouth1 - 1.4f, 4.2f, PAL_ALERT);
    place(art_.lamp, kHeadX - 1.5f, kLaneY + 3.2f, 3.6f, PAL_LAMP);
    place(art_.lamp, kHeadX - 1.5f, kLaneY - 3.2f, 3.6f, PAL_LAMP);
    place(art_.buoy, kMouthX - 3.f, kLaneY + 6.f, 2.8f, PAL_ALERT);
    place(art_.buoy, kMouthX - 3.f, kLaneY - 6.f, 2.8f, PAL_MARK);
    place(art_.pin, (kBerthX0 + kBerthX1) * 0.5f, kLaneY, 2.2f, PAL_WIN, 4.f);
    place(art_.cleat, kBerthX0, kNorth0 - 0.6f, 1.4f, PAL_QUAY);
    place(art_.cleat, kBerthX1, kSouth1 + 0.6f, 1.4f, PAL_QUAY);
    for (const Wake& w : wakes_)
        if (w.life > 0.f) place(art_.foam, w.x, w.y, 1.2f + w.life, PAL_HUD, 2.f);

    float tramH = 7.6f * zoom_;
    float sx = 160.f + (x_ - camX_) * zoom_;
    float sy = 112.f - (y_ - camY_) * zoom_;
    spr(art_.tram[tramFrame()], sx, sy, std::max(10.f, tramH), PAL_TRAM);
    place(art_.bell, x_ - std::cos(heading_) * 1.2f, y_ - std::sin(heading_) * 1.2f, 1.1f, PAL_LAMP, 3.f);

    auto banner = [&](const gs::Mipped& m, float x, float y, int pal) { spr(m, x, y, float(m.h), pal); };
    if (mode_ == Mode::Title) {
        banner(art_.title, 160.f, 22.f, PAL_BANNER);
        hudC(24, "BERTH THE TRAM BEFORE THE TIDE TURNS", PAL_HUD);
        hudC(26, "START  STEER   C GO   B BRAKE", PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        banner(art_.paused, 160.f, 96.f, PAL_BANNER);
    } else if (mode_ == Mode::Fail) {
        const gs::Mipped* msg = &art_.missed;
        if (std::strcmp(why_, "tide turned") == 0) msg = &art_.tide;
        else if (std::strcmp(why_, "scraped the quay") == 0) msg = &art_.scraped;
        banner(*msg, 160.f, 24.f, PAL_ALERT);
        banner(art_.leg, 160.f, 52.f, PAL_ALERT);
    } else if (mode_ == Mode::Win) {
        banner(art_.berthed, 160.f, 18.f, PAL_WIN);
        banner(art_.inSlip, 160.f, 48.f, PAL_WIN);
    }

    if (mode_ == Mode::Run || mode_ == Mode::Pause) {
        char line[48];
        std::snprintf(line, sizeof line, "TIDE %4.1f", tideLeft());
        hud(1, 1, line, tideLeft() < 12.f ? PAL_ALERT : PAL_HUD);
        std::snprintf(line, sizeof line, "SPD %4.1f", speed_);
        hud(30, 1, line, PAL_HUD);
        hudC(26, hint(), PAL_HUD);
    }
}

}  // namespace tramslip

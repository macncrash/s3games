#include "glider.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace gbuoy {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;
constexpr float kDockHalfW = 15.f;
constexpr float kDockSouth = 2.f;
constexpr float kDockNorth = 50.f;
constexpr float kDockY = 26.f;
constexpr float kStartX = 0.f;
constexpr float kStartY = 22.f;
constexpr float kStartH = 1.5707963f;
constexpr float kLandAlt = 10.f;
constexpr float kRound = 4.0f;
constexpr float kCrew = 96.f;
constexpr float kTitleZoom = 0.46f;
constexpr float kTitleCamX = 16.f;
constexpr float kTitleCamY = 230.f;
constexpr float kPlayZoom = 1.52f;

struct Buoy {
    float x, y;
    const char* name;
    int pal;
};

const Buoy kBuoy[3] = {
    {124.f, 170.f, "SPAR", PAL_SPAR},
    {-96.f, 300.f, "CONE", PAL_CONE},
    {72.f, 448.f, "DRUM", PAL_DRUM},
};

// The autopilot flies this polyline. Each buoy is a square, so the bearing wraps.
const float kPath[][2] = {
    {0.f, 110.f},   {124.f, 118.f}, {176.f, 170.f}, {124.f, 222.f}, {72.f, 170.f}, {-96.f, 248.f},
    {-44.f, 300.f}, {-96.f, 352.f}, {-148.f, 300.f}, {72.f, 396.f},  {124.f, 448.f}, {72.f, 500.f},
    {20.f, 448.f},  {0.f, 260.f},   {0.f, 190.f},
};
constexpr int kWpCount = int(sizeof kPath / sizeof kPath[0]);

const float kThermal[3][2] = {{96.f, 118.f}, {-150.f, 236.f}, {150.f, 360.f}};
const float kBeacon[3][2] = {{-168.f, 150.f}, {176.f, 270.f}, {-156.f, 390.f}};

float wrap(float a) {
    while (a > kPi) a -= kTau;
    while (a < -kPi) a += kTau;
    return a;
}

float thermalAt(float x, float y) {
    float lift = 0.f;
    for (const float* t : kThermal) {
        float d = std::hypot(x - t[0], y - t[1]);
        if (d < 34.f) {
            float u = 1.f - d / 34.f;
            lift = std::max(lift, u * u * 1.7f);
        }
    }
    return lift;
}

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    return gs::rgb4(int(ar + (br - ar) * t), int(ag + (bg - ag) * t), int(ab + (bb - ab) * t));
}

}  // namespace

float Game::roundProg() const {
    if (leg_ >= 4) return kRound;
    if (leg_ < 1 || leg_ > 3) return 0.f;
    return mark_[leg_ - 1].accum;
}

float Game::crewLeft() const { return kCrew - raceTime_; }

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (leg_ >= 4) return 3;
    if (leg_ >= 2) return 2;
    return 1;
}

int Game::wingFrame() const {
    float u = std::fmod(heading_, kTau);
    if (u < 0.f) u += kTau;
    int i = int(std::lround(u / kTau * 16.f)) % 16;
    if (i < 0) i += 16;
    return i;
}

void Game::begin() {
    x_ = kStartX;
    y_ = kStartY;
    heading_ = kStartH;
    spd_ = 22.f;
    alt_ = 34.f;
    vs_ = 0.f;
    bank_ = 0.f;
    pitch_ = 0.f;
    leg_ = 0;
    wp_ = 0;
    orbitSign_ = 1;
    legTime_ = 0.f;
    raceTime_ = 0.f;
    final_ = false;
    won_ = false;
    over_ = false;
    crewFail_ = false;
    chimeN_ = 0;
    lastSec_ = -1;
    puffT_ = 0.f;
    puffCursor_ = 0;
    stuckT_ = 0.f;
    stuckX_ = x_;
    stuckY_ = y_;
    report_[0] = 0;
    why_[0] = 0;
    for (Mark& m : mark_) m = {};
    for (Puff& p : puffs_) p = {};
}

void Game::showTitle() {
    begin();
    mode_ = Mode::Title;
    zoom_ = kTitleZoom;
    camX_ = kTitleCamX;
    camY_ = kTitleCamY;
}

void Game::startFly() {
    begin();
    mode_ = Mode::Fly;
    zoom_ = kPlayZoom;
    camX_ = x_;
    camY_ = y_ + 16.f;
    blip(680.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.8f);
    sys.apu.setEcho(0.11f, 0.16f, 0.1f);
    begin();
    if (bot_) {
        mode_ = Mode::Fly;
        zoom_ = kPlayZoom;
        camX_ = x_;
        camY_ = y_ + 12.f;
    } else {
        showTitle();
    }
}

void Game::controls(float& bank, float& pitch) {
    const gs::Pad& p = sys_->pad;
    bank = 0.f;
    pitch = 0.f;
    if (p.down(gs::BTN_LEFT)) bank += 1.f;
    if (p.down(gs::BTN_RIGHT)) bank -= 1.f;
    if (p.down(gs::BTN_UP)) pitch += 1.f;
    if (p.down(gs::BTN_DOWN)) pitch -= 1.f;
    if (std::fabs(p.axisX) > 0.18f) bank = std::clamp(-p.axisX, -1.f, 1.f);
    if (std::fabs(p.axisY) > 0.18f) pitch = std::clamp(p.axisY, -1.f, 1.f);
    if (p.down(gs::BTN_B) || p.down(gs::BTN_X) || p.brake > 0.35f) pitch = -1.f;
    else if (p.down(gs::BTN_A) || p.down(gs::BTN_C) || p.accel > 0.35f) pitch = std::max(pitch, 0.85f);
    bank = std::clamp(bank, -1.f, 1.f);
    pitch = std::clamp(pitch, -1.f, 1.f);
}

void Game::steerToward(float tx, float ty, float& bank) const {
    float err = wrap(std::atan2(ty - y_, tx - x_) - heading_);
    bank = std::clamp(err / 0.36f, -1.f, 1.f);
}

void Game::pilot(float& bank, float& pitch) {
    auto holdAlt = [&](float want) {
        if (alt_ < 12.f) pitch = 1.f;
        else if (alt_ < want - 5.f) pitch = 0.8f;
        else if (alt_ > want + 8.f) pitch = -0.65f;
        else pitch = std::clamp((want - alt_) / 14.f, -0.4f, 0.55f);
    };

    if (wp_ < kWpCount) {
        float tx = kPath[wp_][0];
        float ty = kPath[wp_][1];
        if (std::hypot(x_ - tx, y_ - ty) < 22.f) {
            wp_++;
            if (wp_ < kWpCount) {
                tx = kPath[wp_][0];
                ty = kPath[wp_][1];
            }
        }
        if (wp_ < kWpCount) {
            steerToward(tx, ty, bank);
            holdAlt(31.f);
            return;
        }
    }

    if (final_ && (y_ < -14.f || std::fabs(x_) > 60.f || std::sin(heading_) > 0.3f)) final_ = false;
    float dx = x_;
    float dy = y_ - 200.f;
    float d = std::hypot(dx, dy);
    if (!final_) {
        bool gate = std::fabs(x_) < 24.f && y_ > 150.f && y_ < 250.f && std::sin(heading_) < -0.6f;
        if (gate) final_ = true;
    }
    if (final_) {
        float desired = -kPi * 0.5f - std::clamp(x_ * 0.03f, -0.42f, 0.42f);
        bank = std::clamp(wrap(desired - heading_) / 0.34f, -1.f, 1.f);
        float want = std::clamp((y_ - 4.f) * 0.04f, 3.5f, 8.2f);
        if (alt_ < 3.1f) pitch = 0.75f;
        else if (alt_ > want + 5.f) pitch = -1.f;
        else if (alt_ > want + 1.f) pitch = -0.72f;
        else if (alt_ < want - 1.4f) pitch = 0.4f;
        else pitch = -0.18f;
        return;
    }
    if (d > 42.f) {
        steerToward(0.f, 205.f, bank);
        holdAlt(26.f);
    } else {
        float ang = std::atan2(dy, dx);
        steerToward(std::cos(ang + 1.15f) * 38.f, 200.f + std::sin(ang + 1.15f) * 38.f, bank);
        holdAlt(24.f);
    }
}

void Game::roundBuoy() {
    if (mode_ != Mode::Fly || leg_ < 1 || leg_ > 3) return;
    int i = leg_ - 1;
    Mark& m = mark_[i];
    const Buoy& b = kBuoy[i];
    float dx = x_ - b.x, dy = y_ - b.y;
    float dist = std::hypot(dx, dy);
    if (dist > 190.f) {
        m = {};
        return;
    }
    if (dist < 88.f) m.near = true;
    if (dist > 145.f) {
        m.have = false;
        return;
    }
    float ang = std::atan2(dy, dx);
    if (!m.have) {
        m.prev = ang;
        m.have = true;
        return;
    }
    m.accum += wrap(ang - m.prev);
    m.prev = ang;
    if (m.near && std::fabs(m.accum) >= kRound) {
        leg_++;
        legTime_ = 0.f;
        chime(std::min(leg_, 4));
        blip(520.f + leg_ * 70.f);
    }
}

void Game::tryDock() {
    if (mode_ != Mode::Fly) return;
    bool onDock = std::fabs(x_) <= kDockHalfW && y_ >= kDockSouth && y_ <= kDockNorth;
    bool inbound = std::sin(heading_) < -0.40f;
    if (leg_ >= 4 && onDock && inbound && alt_ <= kLandAlt) {
        win();
        return;
    }
    if (alt_ <= 0.05f) {
        fail(leg_ >= 4 ? "ditched short of the dock" : "ditched before the buoys", false);
        return;
    }
    if (crewLeft() <= 0.f) fail("the other crew took the dock", true);
}

void Game::physics(float bankCmd, float pitchCmd) {
    bank_ += (bankCmd - bank_) * (1.f - std::exp(-8.f * kDt));
    pitch_ += (pitchCmd - pitch_) * (1.f - std::exp(-6.f * kDt));
    float spdTarget = 22.f - pitch_ * 7.f;
    spd_ += (spdTarget - spd_) * (1.f - std::exp(-1.8f * kDt));
    spd_ = std::clamp(spd_, 12.f, 31.f);
    heading_ = wrap(heading_ + bank_ * 1.48f * kDt);

    float sink = 0.68f + std::fabs(bank_) * 0.5f;
    float vert;
    if (pitch_ >= 0.f) vert = (spd_ >= 14.5f) ? pitch_ * 2.7f : -2.0f;
    else vert = pitch_ * 5.2f;
    float prev = alt_;
    alt_ += (0.92f + thermalAt(x_, y_) + vert - sink) * kDt;
    alt_ = std::clamp(alt_, 0.f, 90.f);
    vs_ = (alt_ - prev) / kDt;

    float c = std::cos(heading_), s = std::sin(heading_);
    x_ += c * spd_ * kDt;
    y_ += s * spd_ * kDt;
    x_ = std::clamp(x_, -240.f, 240.f);
    y_ = std::clamp(y_, -36.f, 530.f);

    for (const Buoy& b : kBuoy) {
        float dx = x_ - b.x, dy = y_ - b.y;
        float d = std::hypot(dx, dy);
        if (d < 8.f && d > 0.01f) {
            x_ = b.x + dx / d * 8.5f;
            y_ = b.y + dy / d * 8.5f;
            if (thumpT_ <= 0.f) {
                sys_->apu.noiseBurst(0.22f, 240.f, 0.1f);
                thumpT_ = 0.28f;
            }
        }
    }

    if (leg_ == 0 && y_ > 74.f && std::fabs(x_) < 60.f) {
        leg_ = 1;
        legTime_ = 0.f;
        chime(1);
    }
    legTime_ += kDt;
    roundBuoy();
    tryDock();
    if (mode_ != Mode::Fly) return;

    if (alt_ < 9.f && y_ > 4.f) {
        puffT_ -= kDt;
        if (puffT_ <= 0.f) {
            puffT_ = 0.07f;
            Puff w;
            w.x = x_ - c * 7.f;
            w.y = y_ - s * 7.f;
            w.life = 0.7f;
            puffs_[puffCursor_] = w;
            puffCursor_ = (puffCursor_ + 1) % 12;
        }
    }
    for (Puff& w : puffs_)
        if (w.life > 0.f) w.life -= kDt;

    if (bot_) {
        stuckT_ += kDt;
        if (stuckT_ > 3.2f) {
            float moved = std::hypot(x_ - stuckX_, y_ - stuckY_);
            stuckX_ = x_;
            stuckY_ = y_;
            stuckT_ = 0.f;
            if (moved < 14.f) heading_ = wrap(heading_ + 1.1f);
        }
    }
}

void Game::win() {
    if (won_) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    alt_ = 1.2f;
    spd_ = 0.f;
    bank_ = 0.f;
    pitch_ = 0.f;
    vs_ = 0.f;
    std::snprintf(report_, sizeof report_,
                  "S3 GLIDER BUOY  PASS  rounded the buoys and returned to the same dock ahead of the other crew (%.1fs, %.1fs left)",
                  raceTime_, std::max(0.f, crewLeft()));
    std::printf("%s\n", report_);
    std::fflush(stdout);
    chime(6);
}

void Game::fail(const char* why, bool crew) {
    if (mode_ != Mode::Fly) return;
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    crewFail_ = crew;
    spd_ = 0.f;
    std::snprintf(why_, sizeof why_, "%s", why);
    std::snprintf(report_, sizeof report_, "S3 GLIDER BUOY  FAIL  %s (%.1fs)", why, raceTime_);
    std::printf("%s\n", report_);
    std::fflush(stdout);
    sys_->apu.noiseBurst(0.4f, 90.f, 0.4f);
    sys_->apu.tone(0, 78.f, 0.06f);
    tone0_ = 0.45f;
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.045f);
    tone1_ = 0.07f;
}

void Game::chime(int notes) {
    chimeN_ = std::clamp(notes, 1, 6);
    chimeStep_ = 0;
    chimeT_ = 0.02f;
}

void Game::audio() {
    float wind = mode_ == Mode::Fly ? 0.018f + spd_ * 0.0011f : 0.01f;
    sys_->apu.noise(wind, 800.f + spd_ * 36.f, false);
    if (mode_ == Mode::Fly && vs_ > 0.9f) {
        varioT_ -= kDt;
        if (varioT_ <= 0.f) {
            sys_->apu.tone(1, 500.f + std::min(vs_, 6.f) * 36.f, 0.028f);
            tone1_ = 0.05f;
            varioT_ = 0.24f;
        }
    }
    if (tone0_ > 0.f) {
        tone0_ -= kDt;
        if (tone0_ <= 0.f) sys_->apu.tone(0, 0.f, 0.f);
    }
    if (tone1_ > 0.f) {
        tone1_ -= kDt;
        if (tone1_ <= 0.f && chimeN_ == 0) sys_->apu.tone(1, 0.f, 0.f);
    }
    if (thumpT_ > 0.f) thumpT_ -= kDt;
    if (chimeN_ > 0) {
        chimeT_ -= kDt;
        if (chimeT_ <= 0.f) {
            static const float notes[] = {523.f, 659.f, 784.f, 1046.f, 1318.f, 1568.f};
            sys_->apu.tone(0, notes[std::min(chimeStep_, 5)], 0.05f);
            tone0_ = 0.12f;
            chimeT_ = 0.13f;
            if (++chimeStep_ >= chimeN_) chimeN_ = 0;
        }
    }
    if (mode_ == Mode::Fly) {
        int sec = std::max(0, int(std::ceil(crewLeft())));
        if (sec != lastSec_) {
            if (sec <= 10 && lastSec_ != -1) blip(sec <= 5 ? 920.f : 640.f);
            lastSec_ = sec;
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (mode_ != Mode::Pause) t_ += kDt;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START)) startFly();
        else if (pad.pressed(gs::BTN_MODE)) sys.quit();
    } else if (mode_ == Mode::Fly) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(400.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        } else {
            raceTime_ += kDt;
            float bank = 0.f, pitch = 0.f;
            if (bot_) pilot(bank, pitch);
            else controls(bank, pitch);
            physics(bank, pitch);
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Fly;
        else if (pad.pressed(gs::BTN_MODE)) showTitle();
    } else if (!bot_ && pad.pressed(gs::BTN_START)) {
        startFly();
    } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
        showTitle();
    }
    camera();
    audio();
    draw();
}

void Game::camera() {
    if (mode_ == Mode::Title) {
        camX_ = kTitleCamX;
        camY_ = kTitleCamY;
        zoom_ = kTitleZoom;
        return;
    }
    float lead = mode_ == Mode::Fly ? 20.f : 0.f;
    float gx = x_ + std::cos(heading_) * lead;
    float gy = y_ + std::sin(heading_) * lead;
    float k = 1.f - std::exp(-kDt * 4.2f);
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

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    if (cx + w * 0.5f < -24.f || cy + h * 0.5f < -24.f || cx - w * 0.5f > gs::SCREEN_W + 24.f ||
        cy - h * 0.5f > gs::SCREEN_H + 24.f)
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
    s.shadow = shadow;
    sys_->vdp.sprite(s);
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
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (112.f - y) / std::max(zoom_, 0.2f);
        uint16_t c;
        if (wy < 1.2f) {
            c = gs::rgb4(11, 9, 5);
        } else {
            float u = std::clamp((wy - 1.2f) / 480.f, 0.f, 1.f);
            c = lerpC(gs::rgb4(5, 13, 13), gs::rgb4(1, 4, 8), u);
            float shimmer = 0.5f + 0.5f * std::sin(wy * 0.18f + t_ * 1.6f);
            if (shimmer > 0.93f) c = lerpC(c, gs::rgb4(10, 15, 15), 0.38f);
        }
        v.lineBackdrop[y] = c;
        v.lineFog[y] = 0;
        v.road[y].on = false;
        v.B.hscroll[y] = int16_t(camX_ * zoom_ * 0.45f + t_ * 7.f);
        v.B.vscroll[y] = int16_t(-camY_ * zoom_ * 0.45f + std::sin(t_ * 0.6f) * 2.f);
    }

    const bool title = mode_ == Mode::Title;
    const float minMark = title ? 16.f : 0.f;
    const float minDock = title ? 32.f : 0.f;

    float tx = 0.f, ty = 90.f;
    int tpal = PAL_BANNER;
    if (leg_ >= 1 && leg_ <= 3) {
        tx = kBuoy[leg_ - 1].x;
        ty = kBuoy[leg_ - 1].y;
        tpal = kBuoy[leg_ - 1].pal;
    } else if (leg_ >= 4) {
        tx = 0.f;
        ty = kDockY;
        tpal = PAL_WIN;
    }

    if (!title && (mode_ == Mode::Fly || mode_ == Mode::Pause)) {
        auto chart = [&](float wx, float wy, float& sx, float& sy) {
            sx = 284.f + wx * 0.14f;
            sy = 86.f - wy * 0.105f;
        };
        float sx, sy;
        chart(x_, y_, sx, sy);
        if (sx > 252.f && sx < 316.f && sy > 16.f && sy < 90.f) spr(art_.dot, sx, sy, 5.f, PAL_BANNER);
        chart(0.f, kDockY, sx, sy);
        spr(art_.dot, sx, sy, 4.f, PAL_WIN);
        for (int i = 0; i < 3; i++) {
            chart(kBuoy[i].x, kBuoy[i].y, sx, sy);
            spr(art_.dot, sx, sy, (leg_ == i + 1) ? 6.f : 4.f, kBuoy[i].pal);
        }
        spr(art_.panel, 284.f, 54.f, 78.f, PAL_MAP);
        if (mode_ == Mode::Fly) {
            float bsx = 160.f + (tx - camX_) * zoom_;
            float bsy = 112.f - (ty - camY_) * zoom_;
            if (bsx < 16.f || bsx > 304.f || bsy < 16.f || bsy > 208.f) {
                float dx = bsx - 160.f, dy = bsy - 112.f;
                float ksc = 1.f;
                if (std::fabs(dx) > 1.f) ksc = std::min(ksc, 134.f / std::fabs(dx));
                if (std::fabs(dy) > 1.f) ksc = std::min(ksc, 84.f / std::fabs(dy));
                spr(art_.pin, 160.f + dx * ksc, 112.f + dy * ksc, 13.f, tpal);
            }
        }
    }

    auto banner = [&](const gs::Mipped& m, float y, int pal) { spr(m, 160.f, y, float(m.h), pal); };
    if (title) banner(art_.title, 16.f, PAL_BANNER);
    else if (mode_ == Mode::Pause) banner(art_.paused, 96.f, PAL_BANNER);
    else if (mode_ == Mode::Fail) banner(crewFail_ ? art_.crewTook : art_.ditched, 78.f, PAL_ALERT);
    else if (mode_ == Mode::Win) {
        banner(art_.sameDock, 70.f, PAL_WIN);
        banner(art_.ahead, 102.f, PAL_WIN);
    }

    float gsx = 160.f + (x_ - camX_) * zoom_;
    float gsy = 112.f - (y_ - camY_) * zoom_;
    if (title) gsy += std::sin(t_ * 1.5f) * 1.4f;
    float span = (24.f + std::clamp(1.f - alt_ / 52.f, 0.f, 1.f) * 8.f) * zoom_;
    if (title) span = std::max(span, 22.f);
    float off = 2.f + alt_ * 0.5f;
    const gs::Mipped& wing = art_.wing[wingFrame()];
    spr(wing, gsx, gsy, span, PAL_SAIL);
    spr(art_.shade, gsx + off * 0.6f, gsy + off, std::max(8.f, span * 0.42f), PAL_SHADE);

    for (const Puff& w : puffs_) {
        if (w.life <= 0.f) continue;
        float sx = 160.f + (w.x - camX_) * zoom_;
        float sy = 112.f - (w.y - camY_) * zoom_;
        spr(art_.foam, sx, sy, (3.f + (1.f - w.life) * 6.f) * (zoom_ / kPlayZoom), PAL_FOAM);
    }

    for (int i = 0; i < 3; i++) {
        float pulse = (leg_ == i + 1) ? 1.f + 0.06f * std::sin(t_ * 4.f) : 1.f;
        const gs::Mipped* pic = i == 0 ? &art_.spar : (i == 1 ? &art_.cone : &art_.drum);
        place(*pic, kBuoy[i].x, kBuoy[i].y, 16.f * pulse, kBuoy[i].pal, minMark);
        if (leg_ == i + 1) place(art_.ring, kBuoy[i].x, kBuoy[i].y, 92.f, kBuoy[i].pal, 0.f);
    }
    if (leg_ >= 4 && mode_ == Mode::Fly) place(art_.ring, 0.f, kDockY, 70.f, PAL_WIN, 0.f);

    for (const float* th : kThermal) {
        float bob = 1.f + 0.08f * std::sin(t_ * 1.3f + th[0]);
        place(art_.lift, th[0], th[1], 38.f * bob, PAL_LIFT, title ? 12.f : 0.f);
    }

    int flap = int(t_ * 5.f) & 1;
    place(art_.gull[flap], -36.f + std::sin(t_ * 0.31f) * 80.f, 146.f + std::cos(t_ * 0.22f) * 18.f, 8.f, PAL_GULL,
          title ? 10.f : 0.f);
    place(art_.gull[1 - flap], 70.f + std::cos(t_ * 0.27f) * 60.f, 340.f + std::sin(t_ * 0.19f) * 16.f, 7.f, PAL_GULL,
          title ? 10.f : 0.f);

    place(art_.flag, -18.f, 54.f, 11.f, PAL_DOCK, title ? 10.f : 0.f);
    place(art_.flag, 18.f, 54.f, 11.f, PAL_DOCK, title ? 10.f : 0.f);
    place(art_.sock[int(t_ * 7.f) % 3], -24.f, 8.f, 12.f, PAL_SPAR, title ? 12.f : 0.f);
    place(art_.hut, 30.f, -8.f, 14.f, PAL_DOCK, title ? 14.f : 0.f);
    for (const float* b : kBeacon) place(art_.post, b[0], b[1], 14.f, PAL_DOCK, title ? 8.f : 0.f);
    for (int i = -5; i <= 5; i++) {
        if (i == 0) continue;
        place(art_.foam, i * 26.f, 1.5f, 7.f, PAL_FOAM, title ? 4.f : 0.f);
    }
    place(art_.dock, 0.f, kDockY, 68.f, PAL_DOCK, minDock);

    char buf[64];
    if (title) {
        hudC(23, "ROUND THE BUOYS", PAL_HUD);
        hudC(24, "RETURN TO THE SAME DOCK", PAL_BANNER);
        hudC(25, "THE CLOCK IS THE OTHER CREW", PAL_CREW);
        int crew = int(kCrew);
        std::snprintf(buf, sizeof buf, "THEIR CLOCK %d:%02d", crew / 60, crew % 60);
        if ((int(t_ * 2.f) & 1) == 0) hudC(27, "START", PAL_WIN);
        else hudC(27, buf, PAL_HUD);
        return;
    }

    hud(1, 0, "S3 GLIDER BUOY", PAL_BANNER);
    float left = std::max(0.f, crewLeft());
    int sec = int(left);
    std::snprintf(buf, sizeof buf, "CREW %d:%02d", sec / 60, sec % 60);
    hud(30, 0, buf, (mode_ == Mode::Fly && left < 12.f) ? PAL_ALERT : PAL_CREW);

    if (mode_ == Mode::Pause) {
        hudC(18, "START CONTINUES", PAL_HUD);
        hudC(19, "ESC BACK TO THE BAY", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Win) {
        std::snprintf(buf, sizeof buf, "TIME %d:%02d   CREW LEFT %dS", int(raceTime_) / 60, int(raceTime_) % 60,
                      int(std::max(0.f, crewLeft())));
        hudC(16, buf, PAL_HUD);
        if (!bot_) hudC(18, "START FLIES AGAIN", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Fail) {
        hudC(16, why_, PAL_ALERT);
        if (!bot_) hudC(18, "START TRIES AGAIN", PAL_HUD);
        return;
    }

    int done = leg_ >= 4 ? 3 : (leg_ <= 0 ? 0 : leg_ - 1);
    if (leg_ >= 1 && leg_ <= 3) {
        int pct = std::clamp(int(std::fabs(mark_[leg_ - 1].accum) / kRound * 100.f), 0, 99);
        std::snprintf(buf, sizeof buf, "ROUND %s  %d%%", kBuoy[leg_ - 1].name, pct);
        hud(1, 1, buf, kBuoy[leg_ - 1].pal);
    } else if (leg_ >= 4) {
        hud(1, 1, "THE SAME DOCK", PAL_WIN);
    } else {
        hud(1, 1, "LEAVE THE DOCK", PAL_BANNER);
    }
    std::snprintf(buf, sizeof buf, "ALT %d", int(std::lround(alt_)));
    hud(32, 1, buf, alt_ < 8.f ? PAL_ALERT : PAL_HUD);
    std::snprintf(buf, sizeof buf, "SPD %d  VS %+d", int(std::lround(spd_)), int(std::lround(vs_)));
    hud(1, 2, buf, PAL_HUD);

    std::snprintf(buf, sizeof buf, "%d/3  %s", done, leg_ >= 4 ? "LAND SOUTH" : "ROUND THEM");
    hud(1, 26, buf, PAL_WIN);
    bool onDock = std::fabs(x_) <= kDockHalfW && y_ >= kDockSouth && y_ <= kDockNorth;
    if (leg_ >= 4 && onDock && alt_ > kLandAlt) hud(1, 27, "DIVE ONTO THE DOCK", PAL_ALERT);
    else if (leg_ >= 4 && onDock && std::sin(heading_) >= -0.40f) hud(1, 27, "POINT SOUTH TO LAND", PAL_CREW);
    else if (leg_ >= 4) hud(1, 27, "DESCEND AND TAKE THE DOCK", PAL_HUD);
    else if (raceTime_ < 4.f) hud(1, 27, "ARROWS BANK AND PITCH", PAL_HUD);
    else hud(1, 27, "ROUND THEM, THEN THE SAME DOCK", PAL_HUD);
}

}  // namespace gbuoy

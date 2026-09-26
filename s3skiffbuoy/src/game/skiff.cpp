#include "skiff.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace skiff {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;
constexpr float kEndY = 78.f;
constexpr float kGate = 12.f;
constexpr float kBack = 26.f;
constexpr float kStartX = 0.f;
constexpr float kStartY = 46.f;
constexpr float kStartH = 1.5707963f;
constexpr float kTitleZoom = 0.36f;
constexpr float kTitleCamX = 0.f;
constexpr float kTitleCamY = 286.f;
constexpr float kPlayZoom = 1.58f;
constexpr float kMaxSpeed = 34.f;
constexpr float kRound = 3.2f;

struct Buoy {
    float x, y;
    const char* name;
};

const Buoy kBuoy[3] = {
    {-108.f, 206.f, "CAN"},
    {136.f, 328.f, "NUN"},
    {-18.f, 452.f, "BALL"},
};

const float kPile[8][2] = {
    {-13.4f, 36.f}, {-13.4f, 50.f}, {-13.4f, 64.f}, {-13.4f, 74.f},
    {13.4f, 36.f},  {13.4f, 50.f},  {13.4f, 64.f},  {13.4f, 74.f},
};

const float kRock[3][2] = {{-210.f, 150.f}, {220.f, 260.f}, {-190.f, 390.f}};

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

float Game::roundProg() const {
    if (leg_ < 1 || leg_ > 3) return 0.f;
    return mark_[leg_ - 1].accum;
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (leg_ >= 4) return 3;
    if (leg_ >= 2) return 2;
    return 1;
}

void Game::begin() {
    x_ = kStartX;
    y_ = kStartY;
    heading_ = kStartH;
    speed_ = 0.f;
    throttle_ = 0.f;
    leg_ = 0;
    legTime_ = 0.f;
    raceTime_ = 0.f;
    wakeT_ = 0.f;
    stuckT_ = 0.f;
    stuckX_ = x_;
    stuckY_ = y_;
    wakeCursor_ = 0;
    won_ = false;
    over_ = false;
    chimeN_ = 0;
    report_[0] = 0;
    for (Mark& m : mark_) m = {};
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
    blip(740.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.8f);
    sys.apu.setEcho(0.12f, 0.18f, 0.12f);
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
    steer = 0.f;
    if (p.down(gs::BTN_LEFT)) steer += 1.f;
    if (p.down(gs::BTN_RIGHT)) steer -= 1.f;
    if (std::fabs(p.axisX) > 0.18f) steer = std::clamp(-p.axisX, -1.f, 1.f);
    if (p.down(gs::BTN_UP) || p.down(gs::BTN_C) || p.down(gs::BTN_A) || p.down(gs::BTN_TURBO))
        throttle_ = std::min(1.f, throttle_ + kDt * 0.7f);
    if (p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.down(gs::BTN_X))
        throttle_ = std::max(-0.45f, throttle_ - kDt * 0.85f);
    throttle = throttle_;
}

void Game::pilot(float& steer, float& throttle) {
    auto drive = [&](float tx, float ty, float th) {
        float err = wrap(std::atan2(ty - y_, tx - x_) - heading_);
        steer = std::clamp(err / 0.38f, -1.f, 1.f);
        if (std::fabs(err) > 1.0f) th *= 0.32f;
        throttle = th;
    };
    if (leg_ <= 0) {
        drive(0.f, 150.f, 0.88f);
        return;
    }
    if (leg_ >= 4) {
        if (std::fabs(x_) > 22.f) {
            drive(0.f, y_ - 6.f, 0.8f);
            return;
        }
        float bias = std::clamp(-x_ * 0.05f, -0.5f, 0.5f);
        float err = wrap(-kPi * 0.5f + bias - heading_);
        steer = std::clamp(err / 0.25f, -1.f, 1.f);
        throttle = y_ > 130.f ? 0.7f : 0.5f;
        return;
    }
    const Buoy& b = kBuoy[leg_ - 1];
    float dx = x_ - b.x, dy = y_ - b.y;
    float dist = std::hypot(dx, dy);
    float ang = std::atan2(dy, dx);
    if (dist > 70.f) {
        drive(b.x + std::cos(-0.75f) * 40.f, b.y + std::sin(-0.75f) * 40.f, 0.9f);
    } else if (dist < 28.f) {
        drive(b.x + std::cos(ang + 1.35f) * 48.f, b.y + std::sin(ang + 1.35f) * 48.f, 0.5f);
    } else {
        drive(b.x + std::cos(ang + 1.05f) * 42.f, b.y + std::sin(ang + 1.05f) * 42.f, 0.76f);
    }
}

void Game::roundBuoy() {
    if (mode_ != Mode::Run || leg_ < 1 || leg_ > 3) return;
    int i = leg_ - 1;
    Mark& m = mark_[i];
    const Buoy& b = kBuoy[i];
    float dx = x_ - b.x, dy = y_ - b.y;
    float dist = std::hypot(dx, dy);
    if (dist > 150.f) {
        m = {};
        return;
    }
    if (dist < 60.f) m.near = true;
    if (dist > 86.f) {
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
    if (std::fabs(m.accum) > kRound && m.near) {
        leg_++;
        legTime_ = 0.f;
        chime(std::min(leg_, 5));
    }
}

void Game::resolveEnd() {
    if (mode_ != Mode::Run) return;
    // The dock end is the slot through the sandbar. Off that slot, the leg is over.
    if (leg_ >= 4 && y_ < kEndY) {
        if (std::fabs(x_) <= kGate && std::sin(heading_) < -0.72f) win();
        else fail();
        return;
    }
    if (y_ < kEndY && std::fabs(x_) > kGate) {
        y_ = kEndY + 0.8f;
        if (std::sin(heading_) < 0.f) speed_ *= 0.45f;
        if (thumpT_ <= 0.f) {
            sys_->apu.noiseBurst(0.26f, 280.f, 0.12f);
            thumpT_ = 0.35f;
        }
    }
    if (leg_ == 0 && y_ >= kEndY && std::fabs(x_) <= kGate + 1.f) {
        leg_ = 1;
        legTime_ = 0.f;
        chime(1);
    }
}

void Game::physics(float dt, float steer, float throttle) {
    legTime_ += dt;
    float rate = 1.7f + std::min(std::fabs(speed_), 24.f) * 0.03f;
    heading_ = wrap(heading_ + steer * rate * dt);
    float cap = throttle >= 0.f ? kMaxSpeed : 16.f;
    float target = throttle * cap;
    float ak = target > speed_ ? 1.8f : 2.5f;
    speed_ += (target - speed_) * (1.f - std::exp(-ak * dt));
    speed_ = std::clamp(speed_, -12.f, 38.f);
    float c = std::cos(heading_), s = std::sin(heading_);
    x_ += c * speed_ * dt;
    y_ += s * speed_ * dt;

    if (x_ < -280.f) {
        x_ = -276.f;
        speed_ *= 0.4f;
    } else if (x_ > 280.f) {
        x_ = 276.f;
        speed_ *= 0.4f;
    }
    if (y_ > 540.f) {
        y_ = 534.f;
        speed_ *= 0.4f;
    }
    auto bump = [&](float rx, float ry, float rad) {
        float dx = x_ - rx, dy = y_ - ry;
        float d = std::hypot(dx, dy);
        if (d < rad && d > 0.01f) {
            x_ = rx + dx / d * (rad + 0.4f);
            y_ = ry + dy / d * (rad + 0.4f);
            speed_ *= 0.45f;
            if (thumpT_ <= 0.f) {
                sys_->apu.noiseBurst(0.3f, 360.f, 0.12f);
                thumpT_ = 0.3f;
            }
        }
    };
    for (const Buoy& b : kBuoy) bump(b.x, b.y, 8.f);

    roundBuoy();
    resolveEnd();
    if (mode_ != Mode::Run) return;

    for (const float* p : kPile) bump(p[0], p[1], 3.4f);
    if (y_ < kBack && std::fabs(x_) <= kGate + 2.f) {
        y_ = kBack;
        if (s < 0.f) speed_ *= 0.35f;
    }

    wakeT_ -= dt;
    if (wakeT_ <= 0.f && std::fabs(speed_) > 6.f) {
        wakeT_ = 0.06f;
        Wake w;
        w.x = x_ - c * 8.f;
        w.y = y_ - s * 8.f;
        w.life = 1.f;
        wakes_[wakeCursor_] = w;
        wakeCursor_ = (wakeCursor_ + 1) % 24;
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
            if (moved < 7.f) heading_ = wrap(heading_ + 1.1f);
        }
    }
}

void Game::win() {
    if (won_) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    speed_ = 0.f;
    throttle_ = 0.f;
    std::snprintf(report_, sizeof report_,
                  "S3 SKIFF BUOY  PASS  rounded the buoys and took the same dock end (%.1fs)", raceTime_);
    std::printf("%s\n", report_);
    std::fflush(stdout);
    chime(6);
}

void Game::fail() {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    speed_ = 0.f;
    throttle_ = 0.f;
    sys_->apu.noiseBurst(0.4f, 110.f, 0.45f);
    sys_->apu.tone(0, 90.f, 0.07f);
    tone0_ = 0.45f;
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.05f);
    tone1_ = 0.08f;
}

void Game::chime(int notes) {
    chimeN_ = std::clamp(notes, 1, 6);
    chimeStep_ = 0;
    chimeT_ = 0.02f;
}

void Game::audio(float dt) {
    float water = mode_ == Mode::Run ? 0.016f + std::fabs(speed_) * 0.0005f : 0.01f;
    sys_->apu.noise(water, 620.f, false);
    if (mode_ == Mode::Run && (throttle_ > 0.04f || std::fabs(speed_) > 2.f)) {
        float wob = 0.55f + 0.45f * std::sin(t_ * (16.f + std::max(0.f, throttle_) * 28.f));
        float vol = (0.012f + std::max(0.f, throttle_) * 0.03f) * wob;
        sys_->apu.tone(2, 46.f + std::max(0.f, throttle_) * 34.f + std::fabs(speed_) * 0.35f, vol);
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
    if (chimeN_ > 0) {
        chimeT_ -= dt;
        if (chimeT_ <= 0.f) {
            static const float notes[] = {523.25f, 659.25f, 783.99f, 1046.5f, 1318.5f, 1568.f};
            int n = std::min(chimeStep_, 5);
            sys_->apu.tone(0, notes[n], 0.05f);
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
        if (pad.pressed(gs::BTN_START)) startRun();
        else if (pad.pressed(gs::BTN_MODE)) sys.quit();
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(420.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        } else {
            raceTime_ += kDt;
            float steer = 0.f, thr = throttle_;
            if (bot_) pilot(steer, thr);
            else controls(steer, thr);
            throttle_ = thr;
            physics(kDt, steer, thr);
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Run;
        else if (pad.pressed(gs::BTN_MODE)) showTitle();
    } else if (!bot_ && pad.pressed(gs::BTN_START)) {
        startRun();
    } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
        showTitle();
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
    float lead = mode_ == Mode::Run ? 16.f : 0.f;
    float gx = x_ + std::cos(heading_) * lead;
    float gy = y_ + std::sin(heading_) * lead;
    float k = 1.f - std::exp(-kDt * 4.5f);
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
    if (cx + w < -8 || cy + h < -8 || cx - w > gs::SCREEN_W + 8 || cy - h > gs::SCREEN_H + 8) return;
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

int Game::hullFrame() const {
    float u = std::fmod(heading_, kTau);
    if (u < 0.f) u += kTau;
    int i = int(std::lround(u / kTau * 16.f)) % 16;
    if (i < 0) i += 16;
    return i;
}

void Game::target(float& tx, float& ty) const {
    if (leg_ <= 0) {
        tx = 0.f;
        ty = kEndY + 10.f;
        return;
    }
    if (leg_ >= 4) {
        tx = 0.f;
        ty = kEndY;
        return;
    }
    tx = kBuoy[leg_ - 1].x;
    ty = kBuoy[leg_ - 1].y;
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.roadTime = int(t_ * 48.f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (112.f - y) / std::max(zoom_, 0.2f);
        uint16_t c;
        if (wy < kEndY) {
            c = gs::rgb4(8, 7, 4);
        } else {
            float u = std::clamp((wy - kEndY) / 460.f, 0.f, 1.f);
            c = lerpC(gs::rgb4(4, 12, 12), gs::rgb4(1, 5, 8), u);
            float shimmer = 0.5f + 0.5f * std::sin(wy * 0.16f + t_ * 1.5f);
            if (shimmer > 0.93f) c = lerpC(c, gs::rgb4(9, 14, 14), 0.4f);
        }
        v.lineBackdrop[y] = c;
        v.lineFog[y] = 0;
        gs::RoadLine& r = v.road[y];
        if (wy <= kEndY && wy > -20.f) {
            r.on = true;
            r.cx = 160.f + (0.f - camX_) * zoom_;
            r.hw = std::max(2.f, kGate * zoom_);
            r.v = wy * 28.f + t_ * 16.f;
            r.pal = uint8_t(PAL_ROAD);
            r.band = (int(std::floor(wy * 0.35f)) & 1) ? 1 : 0;
            r.style = 2;
            r.left = 0;
            r.right = 0;
        } else {
            r.on = false;
        }
        v.B.hscroll[y] = int16_t(std::sin(y * 0.07f + t_ * 1.2f) * 5.f + t_ * 10.f);
        v.B.vscroll[y] = int16_t(t_ * 5.f);
    }

    auto banner = [&](const gs::Mipped& m, float x, float y, int pal) { spr(m, x, y, float(m.h), pal, false); };
    if (mode_ == Mode::Title) banner(art_.title, 160.f, 13.f, PAL_BANNER);
    else if (mode_ == Mode::Pause) banner(art_.paused, 160.f, 96.f, PAL_BANNER);
    else if (mode_ == Mode::Fail) {
        banner(art_.missed, 160.f, 78.f, PAL_ALERT);
        banner(art_.legFail, 160.f, 108.f, PAL_ALERT);
    } else if (mode_ == Mode::Win) {
        banner(art_.sameDock, 160.f, 74.f, PAL_WIN);
        banner(art_.endHeld, 160.f, 104.f, PAL_WIN);
    }

    int flap = int(t_ * 4.f) & 1;
    float gx = -30.f + std::sin(t_ * 0.35f) * 70.f;
    float gy = 170.f + std::cos(t_ * 0.22f) * 24.f;
    place(art_.gull[flap], gx, gy, 8.f, PAL_GULL, mode_ == Mode::Title ? 10.f : 0.f);
    place(art_.gull[1 - flap], 80.f + std::cos(t_ * 0.3f) * 50.f, 300.f + std::sin(t_ * 0.27f) * 18.f, 7.f, PAL_GULL,
          mode_ == Mode::Title ? 10.f : 0.f);

    if (mode_ != Mode::Title) {
        auto chart = [&](float wx, float wy, float& sx, float& sy) {
            sx = 278.f + wx * 0.1f;
            sy = 158.f - wy * 0.17f;
        };
        float sx, sy, tx, ty;
        target(tx, ty);
        chart(x_, y_, sx, sy);
        spr(art_.dot, sx, sy, 5.f, PAL_BANNER, false);
        chart(0.f, kEndY, sx, sy);
        spr(art_.dot, sx, sy, 4.f, PAL_END, false);
        for (int i = 0; i < 3; i++) {
            chart(kBuoy[i].x, kBuoy[i].y, sx, sy);
            spr(art_.dot, sx, sy, (leg_ == i + 1) ? 6.f : 4.f, PAL_CAN + i, false);
        }
        spr(art_.panel, 278.f, 116.f, 100.f, PAL_MAP, false);
        chart(tx, ty, sx, sy);
        float bsx = 160.f + (tx - camX_) * zoom_;
        float bsy = 112.f - (ty - camY_) * zoom_;
        if (bsx < 16 || bsx > 304 || bsy < 16 || bsy > 208) {
            float dx = bsx - 160.f, dy = bsy - 112.f;
            float k = 1.f;
            if (std::fabs(dx) > 1.f) k = std::min(k, 142.f / std::fabs(dx));
            if (std::fabs(dy) > 1.f) k = std::min(k, 90.f / std::fabs(dy));
            int pal = leg_ >= 4 || leg_ <= 0 ? PAL_END : PAL_CAN + (leg_ - 1);
            spr(art_.pin, 160.f + dx * k, 112.f + dy * k, 12.f, pal, false);
        }
    }

    float bsx = 160.f + (x_ - camX_) * zoom_;
    float bsy = 112.f - (y_ - camY_) * zoom_ + std::sin(t_ * 2.4f) * 0.8f;
    float boatH = 20.f * zoom_;
    if (mode_ == Mode::Title) boatH = std::max(boatH, 15.f);
    const gs::Mipped& hull = art_.hull[hullFrame()];
    spr(hull, bsx + 3.f, bsy + 2.f, boatH, PAL_HULL, true);
    spr(hull, bsx, bsy, boatH, PAL_HULL, false);
    if (std::fabs(speed_) > 7.f) {
        float c = std::cos(heading_), s = std::sin(heading_);
        place(art_.foam, x_ + c * 11.f, y_ + s * 11.f, 4.f + std::fabs(speed_) * 0.06f, PAL_FOAM, 2.f);
    }

    float minMark = mode_ == Mode::Title ? 14.f : 0.f;
    place(art_.dash, 0.f, kEndY - 2.f, 3.2f, PAL_END, mode_ == Mode::Title ? 6.f : 0.f);
    place(art_.day, -20.f, kEndY - 2.f, 16.f, PAL_END, minMark);
    place(art_.day, 20.f, kEndY - 2.f, 16.f, PAL_END, minMark);
    place(art_.flag, -24.f, 20.f, 10.f, PAL_END, mode_ == Mode::Title ? 8.f : 0.f);

    for (int i = 0; i < 3; i++) {
        float pulse = (leg_ == i + 1) ? 1.f + 0.05f * std::sin(t_ * 4.f) : 1.f;
        const gs::Mipped* pic = i == 0 ? &art_.can : (i == 1 ? &art_.nun : &art_.ball);
        if (leg_ == i + 1) place(art_.ring, kBuoy[i].x, kBuoy[i].y, 78.f, PAL_CAN + i, 0.f);
        place(*pic, kBuoy[i].x, kBuoy[i].y, 13.f * pulse, PAL_CAN + i, mode_ == Mode::Title ? 16.f : 0.f);
    }
    if (leg_ >= 4 && mode_ == Mode::Run) place(art_.ring, 0.f, kEndY, 36.f, PAL_END, 0.f);

    for (const float* p : kPile) place(art_.pile, p[0], p[1], 7.f, PAL_WOOD, 0.f);
    place(art_.finger, -19.f, 52.f, 50.f, PAL_WOOD, mode_ == Mode::Title ? 18.f : 0.f);
    place(art_.finger, 19.f, 52.f, 50.f, PAL_WOOD, mode_ == Mode::Title ? 18.f : 0.f);
    place(art_.shed, 0.f, 16.f, 16.f, PAL_WOOD, mode_ == Mode::Title ? 12.f : 0.f);
    for (const float* rk : kRock) place(art_.rock, rk[0], rk[1], 18.f, PAL_WOOD, mode_ == Mode::Title ? 8.f : 0.f);

    for (const Wake& w : wakes_) {
        if (w.life <= 0.f) continue;
        float h = (3.f + (1.f - w.life) * 5.f) * (zoom_ / kPlayZoom);
        float sx = 160.f + (w.x - camX_) * zoom_;
        float sy = 112.f - (w.y - camY_) * zoom_;
        spr(art_.foam, sx, sy, std::max(2.f, h), PAL_FOAM, false);
    }

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(24, "ROUND THE BUOYS AND RETURN", PAL_HUD);
        hudC(25, "THROUGH THE SAME DOCK END", PAL_BANNER);
        hudC(26, "MISS THE END AND THE LEG FAILS", PAL_ALERT);
        if ((int(t_ * 2.f) & 1) == 0) hudC(27, "START", PAL_WIN);
        else hudC(27, "UP THROTTLE  DOWN EASE  ARROWS STEER", PAL_HUD);
        return;
    }
    hud(1, 0, "S3 SKIFF BUOY", PAL_BANNER);
    int sec = int(raceTime_);
    std::snprintf(buf, sizeof buf, "%d:%02d", sec / 60, sec % 60);
    hud(34, 0, buf, PAL_HUD);
    if (mode_ == Mode::Pause) {
        hudC(18, "START CONTINUES", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Win) {
        std::snprintf(buf, sizeof buf, "TIME %d:%02d", sec / 60, sec % 60);
        hudC(16, buf, PAL_HUD);
        if (!bot_) hudC(18, "START RUNS IT AGAIN", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Fail) {
        hudC(18, "START TRIES AGAIN", PAL_HUD);
        return;
    }
    if (leg_ <= 0) hud(1, 1, "OUT THROUGH THE END", PAL_END);
    else if (leg_ >= 4) hud(1, 1, "THE SAME END", PAL_END);
    else {
        int pct = std::clamp(int(std::fabs(mark_[leg_ - 1].accum) / kRound * 100.f), 0, 99);
        std::snprintf(buf, sizeof buf, "ROUND %s  %d%%", kBuoy[leg_ - 1].name, pct);
        hud(1, 1, buf, PAL_CAN + (leg_ - 1));
    }
    int thr = int(std::lround(throttle_ * 100.f));
    std::snprintf(buf, sizeof buf, "THR %+d   SPD %d", thr, int(std::lround(speed_)));
    hud(1, 2, buf, PAL_HUD);
    if (leg_ >= 4) std::snprintf(buf, sizeof buf, "3/3  TAKE THE END");
    else if (leg_ <= 0) std::snprintf(buf, sizeof buf, "0/3  LEAVE THE DOCK");
    else std::snprintf(buf, sizeof buf, "%d/3  GO AROUND", leg_ - 1);
    hud(1, 26, buf, PAL_WIN);
    if (leg_ >= 4) hud(1, 27, "MISS THE END AND THE LEG FAILS", PAL_ALERT);
    else if (raceTime_ < 5.f) hud(1, 27, "THROTTLE HOLDS   ARROWS STEER", PAL_HUD);
    else hud(1, 27, "GO AROUND, THEN THE SAME DOCK", PAL_HUD);
}

}  // namespace skiff

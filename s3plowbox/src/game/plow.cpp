#include "game/plow.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace plowbox {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;
constexpr float kStorm = 36.f;
constexpr float kBoxX = 0.f;
constexpr float kBoxY = 92.f;
constexpr float kBoxHW = 16.f;
constexpr float kBoxHH = 18.f;
constexpr float kHoldNeed = 0.65f;
constexpr float kStop = 0.28f;

struct V2 {
    float x, y;
};

constexpr V2 kBank[] = {{-34.f, 14.f}, {-38.f, 42.f}, {-36.f, 70.f}, {-32.f, 100.f}, {-28.f, 128.f},
                        {34.f, 12.f},  {38.f, 40.f},  {36.f, 72.f},  {32.f, 104.f}, {30.f, 130.f}};

float wrapPi(float a) {
    while (a > kPi) a -= kTau;
    while (a < -kPi) a += kTau;
    return a;
}

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.08f);
    tone_ = 0.07f;
}

void Game::begin() {
    x_ = 0.f;
    y_ = 8.f;
    heading_ = 0.f;
    speed_ = 0.f;
    blade_ = 0.f;
    you_ = 0.f;
    hold_ = 0.f;
    won_ = false;
    over_ = false;
    fanStep_ = -1;
    flakeN_ = 0;
    camX_ = x_;
    camY_ = y_;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    storm_ = kStorm;
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = true;
    sys.vdp.B.resize(64, 32);
    for (int y = 0; y < sys.vdp.B.h; y++)
        for (int x = 0; x < sys.vdp.B.w; x++) sys.vdp.B.set(x, y, gs::entry(art_.snowTile, PAL_SNOW));
    sys.vdp.setFogColor(gs::rgb4(10, 11, 13));
    sys.apu.setMaster(0.7f);
    sys.apu.setEcho(0.08f, 0.14f, 0.06f);
    begin();
    if (bot_) {
        mode_ = Mode::Drive;
        zoom_ = 2.35f;
    } else {
        mode_ = Mode::Title;
        zoom_ = 1.55f;
        camX_ = 0.f;
        camY_ = 48.f;
    }
}

void Game::controls(float& gas, float& steer) {
    const gs::Pad& p = sys_->pad;
    steer = 0.f;
    gas = 0.f;
    if (p.down(gs::BTN_LEFT)) steer -= 1.f;
    if (p.down(gs::BTN_RIGHT)) steer += 1.f;
    if (p.down(gs::BTN_A) || p.down(gs::BTN_C) || p.down(gs::BTN_UP)) gas += 1.f;
    if (p.down(gs::BTN_B) || p.down(gs::BTN_DOWN)) gas -= 1.f;
}

void Game::pilot(float& gas, float& steer) {
    const float dx = kBoxX - x_;
    const float dy = kBoxY - y_;
    const float dist = std::hypot(dx, dy);
    const float fx = std::sin(heading_);
    const float fy = std::cos(heading_);
    const float along = fx * dx + fy * dy;
    const float side = fy * dx - fx * dy;

    if (bodyInside() && std::fabs(speed_) < kStop + 0.2f) {
        gas = 0.f;
        steer = clampf(wrapPi(0.f - heading_) * 1.6f, -1.f, 1.f) * 0.2f;
        if (std::fabs(speed_) < kStop) steer = 0.f;
        return;
    }

    float err = wrapPi(std::atan2(dx, dy) - heading_);
    float desired = std::min(9.5f, dist * 0.28f);
    if (dist < 10.f) {
        err = wrapPi(0.f - heading_);
        desired = clampf(along * 0.5f, -3.2f, 3.2f);
        if (std::fabs(side) > 1.4f) desired *= 0.4f;
    } else if (std::fabs(err) > 0.7f) {
        desired = std::min(desired, 2.0f);
    }
    steer = clampf(err * 2.2f + (dist < 14.f ? side * 0.07f : 0.f), -1.f, 1.f);
    const float gap = desired - speed_;
    gas = clampf(gap * 1.5f, -1.f, 1.f);
}

void Game::physics(float dt, float gas, float steer) {
    speed_ += gas * 16.f * dt;
    speed_ -= speed_ * 1.55f * dt;
    speed_ = clampf(speed_, -5.f, 11.f);
    const float turn = 1.7f + std::fabs(speed_) * 0.05f;
    heading_ = wrapPi(heading_ + steer * turn * dt);
    x_ += std::sin(heading_) * speed_ * dt;
    y_ += std::cos(heading_) * speed_ * dt;
    if (gas > 0.15f) blade_ += dt * 2.2f;
    if (std::fabs(speed_) > 1.2f) {
        Flake& f = flake_[flakeN_ % 14];
        f.x = x_ - std::sin(heading_) * 5.f + (flakeN_ & 1 ? 3.f : -3.f);
        f.y = y_ - std::cos(heading_) * 4.f;
        f.life = 1.f;
        flakeN_++;
    }
    for (int i = 0; i < 14; i++) flake_[i].life -= dt * 0.9f;
}

bool Game::bodyInside() const {
    const float c = std::cos(heading_);
    const float s = std::sin(heading_);
    const float hx = 6.4f, hy = 7.2f;
    const float lx[4] = {-hx, hx, hx, -hx};
    const float ly[4] = {-hy, -hy, hy, hy};
    for (int i = 0; i < 4; i++) {
        float wx = x_ + lx[i] * c - ly[i] * s;
        float wy = y_ - lx[i] * s - ly[i] * c;
        if (wx < kBoxX - kBoxHW || wx > kBoxX + kBoxHW) return false;
        if (wy < kBoxY - kBoxHH || wy > kBoxY + kBoxHH) return false;
    }
    return true;
}

void Game::audio(float dt, float gas) {
    if (tone_ > 0.f) {
        tone_ -= dt;
        if (tone_ <= 0.f) sys_->apu.tone(0, 0, 0);
    }
    if (mode_ == Mode::Drive && std::fabs(gas) > 0.15f) {
        float phase = blade_ - std::floor(blade_);
        if (phase < dt * 3.f) sys_->apu.tone(1, 90.f + std::fabs(speed_) * 8.f, 0.05f);
        else if (phase > 0.5f && phase < 0.5f + dt * 3.f) sys_->apu.tone(1, 0, 0);
    } else {
        sys_->apu.tone(1, 0, 0);
    }
    if (fanStep_ >= 0) {
        static const float notes[] = {262.f, 330.f, 392.f, 523.f};
        fanStep_++;
        if (fanStep_ % 9 == 1 && fanStep_ < 36) sys_->apu.tone(0, notes[fanStep_ / 9], 0.1f);
        if (fanStep_ > 50) {
            fanStep_ = -1;
            sys_->apu.tone(0, 0, 0);
        }
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal) {
    if (h < 1.5f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 40 || s.x + s.w < -40 || s.y > gs::SCREEN_H + 40 || s.y + s.h < -40) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::worldToScreen(float wx, float wy, float& sx, float& sy) const {
    sx = (wx - camX_) * zoom_ + 160.f;
    sy = 112.f - (wy - camY_) * zoom_;
}

void Game::place(const gs::Mipped& m, float wx, float wy, float worldH, int pal) {
    float sx, sy;
    worldToScreen(wx, wy, sx, sy);
    spr(m, sx, sy, worldH * zoom_, pal);
}

int Game::plowFrame() const {
    float h = heading_;
    if (h < 0.f) h += kTau;
    int face = int(h / (kTau / 8.f) + 0.5f) & 7;
    int phase = int(blade_ * 2.f) & 1;
    return face * 2 + phase;
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = 0;
    while (s[n]) n++;
    hud(20 - n / 2, row, s, pal);
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    int scrollX = int(std::lround(camX_ * zoom_));
    int scrollY = int(std::lround(-camY_ * zoom_));
    vdp.B.scroll(scrollX, scrollY);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int band = 11 + ((y / 40) % 2);
        vdp.lineBackdrop[y] = gs::rgb4(band, band + 1, 15);
        vdp.road[y].on = false;
    }

    float bob = std::sin(t_ * 1.4f) * 0.25f;
    float sx = x_, sy = y_;
    if (mode_ == Mode::Title) {
        sx = 0.f;
        sy = 16.f + bob;
    }
    place(art_.plow[plowFrame()], sx, sy, 16.f, PAL_PLOW);

    const float corners[4][2] = {{kBoxX - kBoxHW, kBoxY - kBoxHH},
                                 {kBoxX + kBoxHW, kBoxY - kBoxHH},
                                 {kBoxX - kBoxHW, kBoxY + kBoxHH},
                                 {kBoxX + kBoxHW, kBoxY + kBoxHH}};
    for (int i = 0; i < 4; i++) place(art_.stake, corners[i][0], corners[i][1], 8.f, PAL_BOX);
    for (int i = 1; i < 4; i++) {
        float u = i / 4.f;
        place(art_.tape, kBoxX - kBoxHW + u * (2.f * kBoxHW), kBoxY - kBoxHH, 1.5f, PAL_BOX);
        place(art_.tape, kBoxX - kBoxHW + u * (2.f * kBoxHW), kBoxY + kBoxHH, 1.5f, PAL_BOX);
    }
    for (int i = 1; i < 5; i++) {
        float u = i / 5.f;
        float yy = kBoxY - kBoxHH + u * (2.f * kBoxHH);
        place(art_.tape, kBoxX - kBoxHW, yy, 1.5f, PAL_BOX);
        place(art_.tape, kBoxX + kBoxHW, yy, 1.5f, PAL_BOX);
    }

    place(art_.barn, 0.f, -6.f, 16.f, PAL_BARN);
    for (const V2& r : kBank) place(art_.bank, r.x, r.y, 9.f, PAL_BANK);
    for (int i = 0; i < 14; i++) {
        if (flake_[i].life > 0.f) place(art_.spray, flake_[i].x, flake_[i].y, 2.2f + flake_[i].life, PAL_SPRAY);
    }

    char line[48];
    if (mode_ == Mode::Title) {
        hudC(3, "S3 PLOW BOX", PAL_HUD);
        hudC(6, "STOP INSIDE THE BOX", 2);
        hudC(8, "BLADE AND CAB BOTH IN", PAL_HUD);
        hudC(10, "THE STORM IS THE CLOCK", 3);
        hudC(18, "A GAS   B BRAKE   ARROWS STEER", PAL_HUD);
        hudC(21, "START", 2);
    } else if (mode_ == Mode::Pause) {
        hudC(12, "PAUSED", 2);
        hudC(15, "START CONTINUES", PAL_HUD);
    } else {
        int sec = int(you_);
        int cs = int(you_ * 100.f) % 100;
        std::snprintf(line, sizeof line, "YOU %d:%02d.%02d", sec / 60, sec % 60, cs);
        hud(1, 1, line, PAL_HUD);
        int csec = int(storm_ - you_);
        if (csec < 0) csec = 0;
        std::snprintf(line, sizeof line, "STORM %d:%02d", csec / 60, csec % 60);
        hud(24, 1, line, 4);
        if (mode_ == Mode::Drive) {
            if (bodyInside()) hudC(25, "INSIDE  HOLD STILL", 3);
            else hudC(25, "THE BOX IS UP THE LANE", 2);
        } else if (mode_ == Mode::Win) {
            hudC(11, "STOPPED", 3);
            hudC(13, "INSIDE THE BOX", 2);
            if (!bot_) hudC(16, "START PLOWS AGAIN", PAL_HUD);
        } else if (mode_ == Mode::Fail) {
            hudC(11, "THE STORM CLOSED", 4);
            hudC(13, "THE LANE", 4);
            if (!bot_) hudC(16, "START PLOWS AGAIN", PAL_HUD);
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    const gs::Pad& pad = sys.pad;
    float gas = 0.f;

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) {
            begin();
            mode_ = Mode::Drive;
            zoom_ = 2.35f;
            blip(520.f);
        }
    } else if (mode_ == Mode::Drive) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(240.f);
        } else {
            float steer = 0.f;
            if (bot_) pilot(gas, steer);
            else controls(gas, steer);
            you_ += kDt;
            physics(kDt, gas, steer);
            if (bodyInside() && std::fabs(speed_) < kStop) hold_ += kDt;
            else hold_ = 0.f;
            if (hold_ >= kHoldNeed) {
                mode_ = Mode::Win;
                won_ = true;
                over_ = true;
                fanStep_ = 0;
                blip(620.f);
            } else if (you_ >= storm_) {
                mode_ = Mode::Fail;
                won_ = false;
                over_ = true;
                blip(90.f);
            }
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) mode_ = Mode::Drive;
    } else if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C))) {
        begin();
        mode_ = Mode::Drive;
        zoom_ = 2.35f;
        blip(520.f);
    }

    if (mode_ == Mode::Title) {
        camX_ = std::sin(t_ * 0.18f) * 3.f;
        camY_ = 50.f + std::sin(t_ * 0.11f) * 2.f;
        zoom_ = 1.5f;
    } else {
        float gx = x_ + std::sin(heading_) * 5.f;
        float gy = y_ + std::cos(heading_) * 5.f;
        float k = 1.f - std::exp(-kDt * 4.f);
        camX_ += (gx - camX_) * k;
        camY_ += (gy - camY_) * k;
        zoom_ = 2.35f;
    }
    audio(kDt, gas);
    draw();
}

}  // namespace plowbox

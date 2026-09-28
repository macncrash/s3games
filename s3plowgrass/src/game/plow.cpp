#include "game/plow.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace plowgrass {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;
constexpr float kCrew = 32.f;
constexpr float kPadX = 0.f;
constexpr float kPadY = 108.f;
constexpr float kPadHW = 30.f;
constexpr float kPadHH = 24.f;
constexpr float kHoldNeed = 0.45f;
constexpr float kStop = 0.16f;

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
    y_ = 6.f;
    heading_ = 0.f;
    speed_ = 0.f;
    wheel_ = 0.f;
    you_ = 0.f;
    hold_ = 0.f;
    won_ = false;
    over_ = false;
    fanStep_ = -1;
    puffN_ = 0;
    camX_ = x_;
    camY_ = y_;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    crew_ = kCrew;
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = true;
    sys.vdp.B.resize(64, 32);
    for (int y = 0; y < sys.vdp.B.h; y++)
        for (int x = 0; x < sys.vdp.B.w; x++) sys.vdp.B.set(x, y, gs::entry(art_.dirtTile, PAL_DIRT));
    sys.vdp.setFogColor(gs::rgb4(6, 8, 4));
    sys.apu.setMaster(0.7f);
    sys.apu.setEcho(0.06f, 0.12f, 0.05f);
    begin();
    if (bot_) {
        mode_ = Mode::Drive;
        zoom_ = 2.2f;
    } else {
        mode_ = Mode::Title;
        zoom_ = 1.35f;
        camX_ = 0.f;
        camY_ = 56.f;
    }
}

void Game::controls(float& gas, float& steer) {
    const gs::Pad& p = sys_->pad;
    steer = 0.f;
    gas = 0.f;
    if (p.down(gs::BTN_LEFT)) steer -= 1.f;
    if (p.down(gs::BTN_RIGHT)) steer += 1.f;
    if (std::fabs(p.axisX) > 0.2f) steer = p.axisX;
    if (p.down(gs::BTN_A) || p.down(gs::BTN_C) || p.down(gs::BTN_UP)) gas += 1.f;
    if (p.down(gs::BTN_B) || p.down(gs::BTN_DOWN)) gas -= 1.f;
    if (p.accel > 0.15f) gas = std::max(gas, p.accel);
    if (p.brake > 0.15f) gas = std::min(gas, -p.brake);
}

void Game::pilot(float& gas, float& steer) {
    const float dx = kPadX - x_;
    const float dy = kPadY - y_;
    const float dist = std::hypot(dx, dy);
    if (onGrass() && std::fabs(speed_) < 1.1f) {
        gas = clampf(-speed_ * 3.2f, -1.f, 0.35f);
        steer = clampf(wrapPi(0.f - heading_) * 1.4f, -1.f, 1.f);
        if (std::fabs(speed_) < kStop) {
            gas = 0.f;
            steer = 0.f;
        }
        return;
    }
    float err = wrapPi(std::atan2(dx, dy) - heading_);
    float desired = std::min(8.2f, 1.4f + dist * 0.16f);
    if (dist < 16.f) {
        err = wrapPi(std::atan2(dx, dy) - heading_);
        desired = clampf(dist * 0.22f, 0.4f, 3.4f);
    }
    if (std::fabs(err) > 0.65f) desired = std::min(desired, 1.8f);
    steer = clampf(err * 2.5f, -1.f, 1.f);
    gas = clampf((desired - speed_) * 1.6f, -1.f, 1.f);
}

void Game::physics(float dt, float gas, float steer) {
    speed_ += gas * 13.5f * dt;
    float drag = 1.15f;
    if (std::fabs(gas) < 0.05f) drag = 2.4f;
    speed_ -= speed_ * drag * dt;
    speed_ = clampf(speed_, -4.5f, 9.5f);
    if (std::fabs(speed_) < 0.04f && std::fabs(gas) < 0.05f) speed_ = 0.f;
    const float turn = (1.45f + std::fabs(speed_) * 0.04f) * (std::fabs(speed_) < 0.2f ? 0.15f : 1.f);
    heading_ = wrapPi(heading_ + steer * turn * dt);
    x_ += std::sin(heading_) * speed_ * dt;
    y_ += std::cos(heading_) * speed_ * dt;
    wheel_ += std::fabs(speed_) * dt * 0.7f;
    if (std::fabs(speed_) > 2.4f) {
        Puff& f = puff_[puffN_ % 12];
        f.x = x_ - std::sin(heading_) * 4.5f;
        f.y = y_ - std::cos(heading_) * 5.f;
        f.life = 0.8f;
        puffN_++;
    }
    for (int i = 0; i < 12; i++) puff_[i].life -= dt * 1.1f;
}

bool Game::onGrass() const {
    const float c = std::cos(heading_);
    const float s = std::sin(heading_);
    const float hx = 5.2f, hy = 6.8f;
    const float lx[4] = {-hx, hx, hx, -hx};
    const float ly[4] = {-hy, -hy, hy, hy};
    for (int i = 0; i < 4; i++) {
        float wx = x_ + lx[i] * c + ly[i] * s;
        float wy = y_ - lx[i] * s + ly[i] * c;
        if (wx < kPadX - kPadHW || wx > kPadX + kPadHW) return false;
        if (wy < kPadY - kPadHH || wy > kPadY + kPadHH) return false;
    }
    return true;
}

void Game::audio(float dt, float gas) {
    if (tone_ > 0.f) {
        tone_ -= dt;
        if (tone_ <= 0.f) sys_->apu.tone(0, 0, 0);
    }
    if (mode_ == Mode::Drive && std::fabs(speed_) > 0.3f) {
        float phase = wheel_ - std::floor(wheel_);
        if (phase < dt * 2.f) sys_->apu.tone(1, 70.f + std::fabs(speed_) * 6.f, 0.04f);
    } else {
        sys_->apu.tone(1, 0, 0);
    }
    (void)gas;
    if (fanStep_ >= 0) {
        static const float notes[] = {392.f, 494.f, 587.f, 784.f};
        fanStep_++;
        if (fanStep_ % 8 == 1 && fanStep_ < 32) sys_->apu.tone(0, notes[fanStep_ / 8], 0.1f);
        if (fanStep_ > 48) {
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
    return int(h / (kTau / 8.f) + 0.5f) & 7;
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
        vdp.lineBackdrop[y] = gs::rgb4(5, 7, 3);
        vdp.road[y].on = false;
    }

    float bob = std::sin(t_ * 1.6f) * 0.2f;
    float sx = x_, sy = y_;
    if (mode_ == Mode::Title) {
        sx = 0.f;
        sy = 14.f + bob;
    }
    place(art_.plow[plowFrame()], sx, sy, 15.f, PAL_PLOW);

    for (int gy = 0; gy < 5; gy++) {
        for (int gx = 0; gx < 6; gx++) {
            float wx = kPadX - kPadHW + 4.f + gx * 10.4f;
            float wy = kPadY - kPadHH + 4.f + gy * 10.f;
            place(art_.sod, wx, wy, 9.2f, PAL_GRASS);
        }
    }
    const float posts[4][2] = {{kPadX - kPadHW, kPadY - kPadHH},
                               {kPadX + kPadHW, kPadY - kPadHH},
                               {kPadX - kPadHW, kPadY + kPadHH},
                               {kPadX + kPadHW, kPadY + kPadHH}};
    for (int i = 0; i < 4; i++) place(art_.post, posts[i][0], posts[i][1], 7.f, PAL_POST);
    for (int i = 0; i < 7; i++) {
        float u = (i + 0.5f) / 7.f;
        place(art_.tuft, kPadX - kPadHW + u * (2.f * kPadHW), kPadY + kPadHH + 3.f, 3.4f, PAL_GRASS);
        place(art_.tuft, kPadX - kPadHW - 3.5f, kPadY - kPadHH + u * (2.f * kPadHH), 3.2f, PAL_GRASS);
    }
    place(art_.crew, 48.f, kPadY, 11.f, PAL_CREW);
    for (int i = 0; i < 12; i++) {
        if (puff_[i].life > 0.f) place(art_.dust, puff_[i].x, puff_[i].y, 1.6f + puff_[i].life, PAL_DUST);
    }

    char line[48];
    if (mode_ == Mode::Title) {
        hudC(3, "S3 PLOW GRASS", PAL_HUD);
        hudC(6, "LAND ON THE GRASS", 3);
        hudC(8, "COME TO A FULL STOP", PAL_HUD);
        hudC(10, "THE CLOCK IS THE OTHER CREW", 4);
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
        int left = int(std::ceil(crew_ - you_));
        if (left < 0) left = 0;
        std::snprintf(line, sizeof line, "CREW %d:%02d", left / 60, left % 60);
        hud(26, 1, line, 4);
        if (mode_ == Mode::Drive) {
            if (onGrass() && std::fabs(speed_) < kStop) hudC(25, "HOLD THE STOP", 3);
            else if (onGrass()) hudC(25, "ON THE GRASS  STOP", 2);
            else hudC(25, "THE GRASS IS AHEAD", 3);
        } else if (mode_ == Mode::Win) {
            hudC(11, "STOPPED", 3);
            hudC(13, "ON THE GRASS", 2);
            if (!bot_) hudC(16, "START PLOWS AGAIN", PAL_HUD);
        } else if (mode_ == Mode::Fail) {
            hudC(11, "THE OTHER CREW", 4);
            hudC(13, "BEAT THE CLOCK", 4);
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
            zoom_ = 2.2f;
            blip(480.f);
        }
    } else if (mode_ == Mode::Drive) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(220.f);
        } else {
            float steer = 0.f;
            if (bot_) pilot(gas, steer);
            else controls(gas, steer);
            you_ += kDt;
            physics(kDt, gas, steer);
            if (onGrass() && std::fabs(speed_) <= kStop) hold_ += kDt;
            else hold_ = 0.f;
            if (hold_ >= kHoldNeed) {
                mode_ = Mode::Win;
                won_ = true;
                over_ = true;
                fanStep_ = 0;
                blip(660.f);
            } else if (you_ >= crew_) {
                mode_ = Mode::Fail;
                won_ = false;
                over_ = true;
                blip(80.f);
            }
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) mode_ = Mode::Drive;
    } else if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C))) {
        begin();
        mode_ = Mode::Drive;
        zoom_ = 2.2f;
        blip(480.f);
    }

    if (mode_ == Mode::Title) {
        camX_ = std::sin(t_ * 0.16f) * 4.f;
        camY_ = 58.f + std::sin(t_ * 0.1f) * 2.f;
        zoom_ = 1.32f;
    } else {
        float gx = x_ + std::sin(heading_) * 6.f;
        float gy = y_ + std::cos(heading_) * 6.f;
        float k = 1.f - std::exp(-kDt * 4.f);
        camX_ += (gx - camX_) * k;
        camY_ += (gy - camY_) * k;
        zoom_ = 2.2f;
    }
    audio(kDt, gas);
    draw();
}

}  // namespace plowgrass

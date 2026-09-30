#include "game/pass.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace railpass {
namespace {

constexpr float CAR_W = 70.f;
constexpr float RAIL_Y = 168.f;
constexpr float EXIT_X = 2480.f;
constexpr float CLOCK0 = 36.f;
constexpr float MAX_SPD = 2.15f;
constexpr float DRIFT_X[5] = {460.f, 920.f, 1420.f, 1880.f, 2260.f};

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.72f);
    sys.apu.setEcho(0.12f, 0.2f, 0.12f);
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    why_ = "";
    clock_ = CLOCK0;
    carX_ = 40.f;
    if (bot_) begin();
}

void Game::begin() {
    mode_ = Mode::Run;
    over_ = false;
    won_ = false;
    why_ = "";
    time_ = 0;
    clock_ = CLOCK0;
    carX_ = 28.f;
    speed_ = 0;
    cam_ = 0;
    shake_ = 0;
    beep_ = 0;
    roll_ = 0;
    plow_ = false;
    cleared_ = 0;
    for (int i = 0; i < 5; i++) {
        drifts_[i].x = DRIFT_X[i];
        drifts_[i].live = true;
    }
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(1, 0, 0);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    why_ = why;
    won_ = false;
    over_ = true;
    mode_ = Mode::Fail;
    speed_ = 0;
    shake_ = 16;
    sys_->apu.noiseBurst(0.55f, 280.f, 0.4f);
    sys_->apu.tone(0, 0, 0);
}

void Game::succeed() {
    if (mode_ != Mode::Run) return;
    why_ = "cleared";
    won_ = true;
    over_ = true;
    mode_ = Mode::Win;
    speed_ = 0;
    sys_->apu.tone(1, 659.f, 0.09f);
    beep_ = 30;
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (won_ || mode_ == Mode::Win) return 4;
    if (carX_ > 1900.f) return 3;
    if (carX_ > 700.f || cleared_ > 0) return 2;
    return 1;
}

void Game::toneTick(float freq, float vol) {
    if (beep_ > 0) return;
    sys_->apu.tone(0, freq, vol);
    beep_ = 6;
}

void Game::pilot(float& accel) {
    plow_ = false;
    accel = 0;
    if (bot_) {
        float nose = carX_ + CAR_W;
        float next = 1.0e9f;
        for (const Drift& d : drifts_) {
            if (!d.live) continue;
            if (d.x + 8.f > nose - 4.f) next = std::min(next, d.x);
        }
        float dist = next - nose;
        accel = speed_ < MAX_SPD ? 1.f : 0.f;
        if (next < 1.0e8f && dist < 100.f) {
            plow_ = dist < 36.f;
            if (speed_ > 0.78f) accel = -1.f;
            else if (speed_ < 0.62f) accel = 0.7f;
            else accel = 0.f;
        }
        return;
    }
    const gs::Pad& p = sys_->pad;
    if (p.down(gs::BTN_DOWN) || p.down(gs::BTN_C) || p.down(gs::BTN_X)) plow_ = true;
    if (p.down(gs::BTN_RIGHT) || p.down(gs::BTN_A) || p.axisX > 0.3f || p.accel > 0.15f) accel += 1.f;
    if (p.down(gs::BTN_LEFT) || p.down(gs::BTN_B) || p.axisX < -0.3f || p.brake > 0.15f) accel -= 1.f;
}

void Game::logic() {
    clock_ -= 1.f / 60.f;
    time_ += 1.f / 60.f;
    if (clock_ <= 0.f) {
        clock_ = 0;
        fail("the storm closed the pass");
        return;
    }

    float accel = 0;
    pilot(accel);
    if (accel > 0.f) speed_ += 0.042f * accel;
    else if (accel < 0.f) speed_ -= 0.085f;
    else speed_ *= 0.986f;
    speed_ = clampf(speed_, 0.f, MAX_SPD);
    if (speed_ < 0.02f) speed_ = 0;
    carX_ += speed_;
    roll_ = (roll_ + int(speed_ * 4.f)) & 1023;

    float nose = carX_ + CAR_W;
    for (Drift& d : drifts_) {
        if (!d.live) continue;
        bool hit = nose > d.x - 6.f && carX_ < d.x + 34.f;
        if (!hit) continue;
        if (plow_ && speed_ <= 0.95f) {
            d.live = false;
            cleared_++;
            speed_ *= 0.5f;
            sys_->apu.noiseBurst(0.28f, 900.f, 0.12f);
            toneTick(220.f, 0.06f);
        } else if (speed_ > 0.5f) {
            fail("buried in a snow drift");
            return;
        } else {
            speed_ = 0;
            carX_ = d.x - 8.f - CAR_W;
        }
    }

    if (mode_ == Mode::Run && carX_ > EXIT_X) succeed();
    if (mode_ == Mode::Run && speed_ > 0.3f && (roll_ % 18) < 2) toneTick(70.f + speed_ * 40.f, 0.04f);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool feet) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 64 || s.x + s.w < -64 || s.y > gs::SCREEN_H + 64 || s.y + s.h < -64) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    float storm = 1.f - clock_ / CLOCK0;
    s.fog = uint8_t(clampf(storm * storm * 10.f, 0.f, 12.f));
    sys_->vdp.sprite(s);
}

void Game::text(const char* s, float x, float y, float scale, int pal, int align) {
    if (!s) return;
    const float adv = 6.f * scale;
    float w = float(std::strlen(s)) * adv;
    if (align == 0) x -= w * 0.5f;
    else if (align > 0) x -= w;
    for (size_t i = 0; s[i]; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < 32 || c >= 96) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        if (g.h < 1) continue;
        spr(g, x + float(i) * adv + g.w * scale * 0.5f, y, g.h * scale, pal, false);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.setFogColor(gs::rgb4(6, 7, 9));
    float shake = shake_ > 0 ? ((int(shake_) & 1) ? 2.f : -2.f) : 0.f;
    cam_ = carX_ - 48.f;
    if (cam_ < 0) cam_ = 0;
    float storm = mode_ == Mode::Title ? 0.15f : 1.f - clock_ / CLOCK0;

    for (int y = 0; y < gs::SCREEN_H; y++) {
        int lift = y < 120 ? y / 30 : 4;
        int grey = int(3 + lift + storm * 5.f);
        v.lineBackdrop[y] = gs::rgb4(std::min(12, grey), std::min(13, grey + 1), std::min(14, 6 + lift));
        v.lineFog[y] = uint8_t(clampf(storm * 6.f + (y > 150 ? 2.f : 0.f), 0.f, 14.f));
        v.road[y].on = false;
    }

    auto sx = [&](float wx) { return wx - cam_ + shake; };

    for (int i = 0; i < 8; i++) {
        float wx = i * 360.f + 40.f;
        spr(art_.peak, sx(wx), 108, 78, PAL_ROCK, true);
        spr(art_.pine, sx(wx + 90.f), RAIL_Y - 6.f, 36, PAL_PINE, true);
    }
    float cloudBase = cam_ * 0.35f + time_ * 18.f;
    for (int i = 0; i < 5; i++) {
        float cx = std::fmod(cloudBase + i * 90.f, 420.f) - 40.f;
        spr(art_.cloud, cx, 28.f + (i % 3) * 10.f, 22.f + storm * 10.f, PAL_STORM, false);
    }

    spr(art_.mouth, sx(EXIT_X + 20.f), RAIL_Y + 6.f, 86, PAL_ROCK, true);

    for (float wx = cam_ - 10.f; wx < cam_ + 360.f; wx += 24.f)
        spr(art_.sleeper, sx(wx), RAIL_Y + 4.f, 9, PAL_RAIL, true);

    for (const Drift& d : drifts_) {
        if (!d.live) continue;
        spr(art_.drift, sx(d.x + 12.f), RAIL_Y + 2.f, 26, PAL_SNOW, true);
    }

    float feet = RAIL_Y + 2.f;
    spr(art_.engine, sx(carX_ + CAR_W * 0.5f), feet, 42, PAL_ENGINE, true);
    if (plow_) spr(art_.plow, sx(carX_ + CAR_W + 6.f), feet - 2.f, 18, PAL_ENGINE, true);

    if (mode_ == Mode::Title) {
        text("S3 RAIL PASS", 160, 36, 2.6f, PAL_HUD, 0);
        text("CLEAR THE PASS", 160, 74, 2.f, PAL_HUD, 0);
        text("BEFORE THE STORM CLOCK", 160, 98, 1.5f, PAL_HUD, 0);
        text("A ROLL   B BRAKE   DOWN PLOW", 160, 140, 1.3f, PAL_HUD, 0);
        text("PRESS START", 160, 176, 2.f, PAL_HUD, 0);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160, 96, 3.f, PAL_HUD, 0);
    } else if (mode_ == Mode::Fail) {
        text("PASS CLOSED", 160, 36, 2.4f, PAL_ALERT, 0);
        text(why_, 160, 70, 1.4f, PAL_HUD, 0);
        text("PRESS START", 160, 120, 2.f, PAL_HUD, 0);
    } else if (mode_ == Mode::Win) {
        text("PASS CLEAR", 160, 34, 2.6f, PAL_HUD, 0);
        text("AHEAD OF THE STORM", 160, 68, 1.5f, PAL_HUD, 0);
        char buf[64];
        std::snprintf(buf, sizeof(buf), "CLOCK %.1f S", clock_);
        text(buf, 160, 104, 2.f, PAL_HUD, 0);
    } else {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "STORM %.1f", clock_);
        text(buf, 8, 10, 1.5f, clock_ < 8.f ? PAL_ALERT : PAL_HUD, -1);
        std::snprintf(buf, sizeof(buf), "DRIFTS %d", 5 - cleared_);
        text(buf, 312, 10, 1.4f, PAL_HUD, 1);
        text(plow_ ? "PLOW DOWN" : "PLOW UP", 8, 206, 1.3f, plow_ ? PAL_HUD : PAL_ALERT, -1);
        const char* hint = carX_ > 1900.f ? "THE MOUTH" : "CLEAR THE DRIFTS";
        text(hint, 312, 206, 1.3f, PAL_HUD, 1);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (beep_ > 0 && --beep_ == 0) sys.apu.tone(0, 0, 0);
    if (shake_ > 0) shake_ -= 1.f;

    const gs::Pad& p = sys.pad;
    bool start = p.pressed(gs::BTN_START) || (mode_ != Mode::Run && p.pressed(gs::BTN_A));
    if (mode_ == Mode::Title) {
        if (start || bot_) begin();
    } else if (mode_ == Mode::Pause) {
        if (p.pressed(gs::BTN_START)) mode_ = Mode::Run;
    } else if (mode_ == Mode::Fail || mode_ == Mode::Win) {
        if (!bot_ && start) begin();
    } else if (mode_ == Mode::Run) {
        if (!bot_ && p.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else logic();
    }
    draw();
}

}  // namespace railpass

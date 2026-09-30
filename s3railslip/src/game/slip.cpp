#include "game/slip.h"
#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace railslip {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float CAR_L = 48.f;
constexpr float ACCEL = 48.f;
constexpr float BRAKE = 78.f;
constexpr float DRAG = 8.f;
constexpr float MAX_V = 72.f;
constexpr float TIDE0 = 18.f;
constexpr float RAIL_Y = 148.f;
constexpr int LEGS = 3;

struct Slip {
    float a, b;
};

// Mouth of each berth, world x. The cradle must sit wholly inside.
constexpr Slip SLIPS[LEGS] = {{220.f, 310.f}, {640.f, 740.f}, {1080.f, 1200.f}};

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.HUD.enabled = false;
    sys.vdp.setFogColor(gs::rgb4(4, 6, 8));
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
}

void Game::begin() {
    mode_ = Mode::Run;
    time_ = 0;
    tide_ = TIDE0;
    carX_ = 24.f;
    speed_ = 0;
    hold_ = 0;
    flash_ = 0;
    berthed_ = 0;
    leg_ = 0;
    why_ = "";
    won_ = false;
    over_ = false;
}

void Game::fail(const char* why) {
    why_ = why;
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    speed_ = 0;
    sys_->apu.noiseBurst(0.45f, 280.f, 0.25f);
    sys_->apu.tone(0, 110.f, 0.12f);
    sys_->apu.tone(1, 0, 0);
}

void Game::berth() {
    berthed_++;
    flash_ = 0.7f;
    sys_->apu.tone(0, 392.f, 0.14f);
    sys_->apu.tone(1, 523.f, 0.1f);
    if (berthed_ >= LEGS) {
        mode_ = Mode::Win;
        over_ = true;
        won_ = true;
        speed_ = 0;
        return;
    }
    leg_ = berthed_;
    tide_ = TIDE0;
    hold_ = 0;
}

void Game::pilot(float& throttle, float& brake) {
    throttle = 0;
    brake = 0;
    if (!bot_) {
        const gs::Pad& p = sys_->pad;
        if (p.down(gs::BTN_A) || p.down(gs::BTN_RIGHT) || p.accel > 0.2f) throttle = 1.f;
        if (p.down(gs::BTN_B) || p.down(gs::BTN_LEFT) || p.brake > 0.2f) brake = 1.f;
        if (p.axisX > 0.25f) throttle = std::max(throttle, p.axisX);
        if (p.axisX < -0.25f) brake = std::max(brake, -p.axisX);
        return;
    }
    const Slip& s = SLIPS[leg_];
    float aim = (s.a + s.b) * 0.5f - CAR_L * 0.5f;
    float dist = aim - carX_;
    float stopNeed = (speed_ * speed_) / (2.f * BRAKE) + 10.f;
    if (dist <= 1.5f) {
        brake = 1.f;
    } else if (dist < stopNeed) {
        brake = speed_ > 6.f ? 1.f : 0.45f;
    } else if (speed_ < MAX_V * 0.85f) {
        throttle = 1.f;
    }
    if (carX_ + CAR_L > s.b - 8.f) brake = 1.f;
}

void Game::logic() {
    float throttle = 0, brake = 0;
    if (mode_ == Mode::Run) pilot(throttle, brake);

    if (throttle > 0 && brake <= 0) speed_ += ACCEL * throttle * DT;
    if (brake > 0) speed_ -= BRAKE * brake * DT;
    if (throttle <= 0 && brake <= 0) speed_ -= DRAG * DT;
    if (speed_ < 0) speed_ = 0;
    if (speed_ > MAX_V) speed_ = MAX_V;
    if (mode_ == Mode::Run) carX_ += speed_ * DT;

    if (mode_ == Mode::Run) {
        tide_ -= DT;
        time_ += DT;
        const Slip& s = SLIPS[leg_];
        bool inside = carX_ >= s.a && carX_ + CAR_L <= s.b;
        if (inside && speed_ < 1.1f) {
            hold_ += DT;
            if (hold_ > 0.25f) berth();
        } else {
            hold_ = 0;
        }
        if (mode_ == Mode::Run && carX_ + CAR_L > s.b + 0.5f) fail("missed the end");
        else if (mode_ == Mode::Run && tide_ <= 0.f) fail("the tide turned");
    }

    if (flash_ > 0) flash_ -= DT;
    if (mode_ == Mode::Run && speed_ > 2.f) {
        sys_->apu.tone(2, 55.f + speed_ * 0.6f, 0.035f);
    } else if (mode_ != Mode::Fail && mode_ != Mode::Win) {
        sys_->apu.tone(2, 0, 0);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& p = sys.pad;
    if (mode_ == Mode::Title) {
        if (bot_ || p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A)) begin();
    } else if (mode_ == Mode::Pause) {
        if (p.pressed(gs::BTN_START)) mode_ = Mode::Run;
    } else if (mode_ == Mode::Run) {
        if (!bot_ && p.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        logic();
    } else if (mode_ == Mode::Fail || mode_ == Mode::Win) {
        if (!bot_ && (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A))) begin();
    }
    draw();
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Win || mode_ == Mode::Fail) return 3;
    if (flash_ > 0.2f) return 2;
    return 1;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool feet, bool flip) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 80 || s.x + s.w < -80 || s.y > gs::SCREEN_H + 40 || s.y + s.h < -40) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
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
        spr(g, x + float(i) * adv + g.w * scale * 0.5f, y, g.h * scale, pal, false, false);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    cam_ = carX_ - 70.f;
    if (cam_ < 0) cam_ = 0;
    float tideU = mode_ == Mode::Title ? 0.2f : clampf(1.f - tide_ / TIDE0, 0.f, 1.f);

    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < 132) {
            int band = y / 22;
            v.lineBackdrop[y] = gs::rgb4(6 + band / 3, 7 + band / 2, 11 + band / 3);
            v.lineFog[y] = 0;
            v.road[y].on = false;
        } else {
            v.lineBackdrop[y] = gs::rgb4(1, 3, 6);
            v.lineFog[y] = uint8_t(y > 190 ? 3 : 1);
            gs::RoadLine& r = v.road[y];
            r.on = true;
            r.cx = 160.f;
            r.hw = 210.f;
            r.v = time_ * 14.f + float(y - 132) * 3.2f;
            r.pal = PAL_WATER;
            r.band = uint8_t((y / 6) & 1);
            r.style = 2;
            r.left = gs::GROUND_WATER;
            r.right = gs::GROUND_WATER;
        }
    }
    v.roadTime = int(time_ * 60.f);

    auto sx = [&](float wx) { return wx - cam_; };

    for (float wx = std::floor(cam_ / 22.f) * 22.f; wx < cam_ + 360.f; wx += 22.f)
        spr(art_.sleeper, sx(wx), RAIL_Y + 6.f, 10, PAL_QUAY, true);

    for (int i = 0; i < 6; i++) {
        float wx = 40.f + i * 230.f;
        spr(art_.shed, sx(wx), RAIL_Y - 8.f, 34, PAL_QUAY, true);
        spr(art_.lamp, sx(wx + 54.f), RAIL_Y - 2.f, 40, PAL_FLAG, true);
    }

    for (int i = 0; i < LEGS; i++) {
        const Slip& s = SLIPS[i];
        float mid = (s.a + s.b) * 0.5f;
        spr(art_.pile, sx(s.a), RAIL_Y + 18.f, 70, PAL_PILE, true);
        spr(art_.pile, sx(s.b), RAIL_Y + 18.f, 70, PAL_PILE, true);
        bool done = i < berthed_;
        bool live = mode_ == Mode::Run && i == leg_;
        spr(art_.flag, sx(mid), RAIL_Y - 28.f, done ? 22.f : (live ? 30.f : 24.f), done ? PAL_FLAG : PAL_ALERT, true);
    }

    float gullX = std::fmod(time_ * 28.f + 40.f, 340.f);
    spr(art_.gull, gullX, 36.f + std::sin(time_ * 3.f) * 6.f, 14, PAL_GULL, false);
    spr(art_.gull, std::fmod(gullX + 150.f, 340.f), 52.f, 11, PAL_GULL, false, true);

    spr(art_.boat, sx(carX_ + CAR_L * 0.5f), RAIL_Y + 2.f, 40, PAL_BOAT, true);

    if (mode_ == Mode::Title) {
        text("S3 RAIL SLIP", 160, 28, 2.4f, PAL_HUD, 0);
        text("BERTH IN THE SLIP", 160, 64, 1.7f, PAL_HUD, 0);
        text("BEFORE THE TIDE TURNS", 160, 86, 1.5f, PAL_HUD, 0);
        text("MISSING THE END FAILS THE LEG", 160, 110, 1.2f, PAL_ALERT, 0);
        text("A THROTTLE   B BRAKE", 160, 150, 1.4f, PAL_HUD, 0);
        text("PRESS START", 160, 184, 2.f, PAL_HUD, 0);
        text(S3_VERSION_STRING, 312, 210, 1.f, PAL_HUD, 1);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160, 90, 3.f, PAL_HUD, 0);
    } else if (mode_ == Mode::Fail) {
        text("LEG LOST", 160, 36, 2.6f, PAL_ALERT, 0);
        text(why_, 160, 74, 1.5f, PAL_HUD, 0);
        text("PRESS START", 160, 130, 2.f, PAL_HUD, 0);
    } else if (mode_ == Mode::Win) {
        text("BERTHED", 160, 32, 3.f, PAL_HUD, 0);
        text("THREE SLIPS BEFORE THE TIDE", 160, 72, 1.3f, PAL_HUD, 0);
        char buf[64];
        std::snprintf(buf, sizeof(buf), "LEG %.1f S", time_);
        text(buf, 160, 108, 2.f, PAL_HUD, 0);
    } else {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "TIDE %.1f", tide_);
        text(buf, 8, 8, 1.6f, tide_ < 5.f ? PAL_ALERT : PAL_HUD, -1);
        std::snprintf(buf, sizeof(buf), "SLIP %d OF %d", leg_ + 1, LEGS);
        text(buf, 312, 8, 1.4f, PAL_HUD, 1);
        text(tideU > 0.72f ? "TIDE TURNING" : "HOLD IN THE SLIP", 160, 206, 1.3f, tideU > 0.72f ? PAL_ALERT : PAL_HUD,
             0);
        if (flash_ > 0) text("IN THE SLIP", 160, 40, 2.f, PAL_HUD, 0);
    }
}

}  // namespace railslip

#include "game/kart.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace kartbox {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kBoxL = 22.f;
constexpr float kBoxR = 30.f;
constexpr float kHalf = 1.25f;
constexpr float kGoal = 26.f;
constexpr float kYardEnd = 38.f;
constexpr float kStopSpd = 0.12f;
constexpr float kHoldNeed = 0.5f;
constexpr float kIdleFail = 1.15f;
constexpr float kLimit = 28.f;
constexpr float kDrive = 8.6f;
constexpr float kBrake = 15.f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

bool bodyInside(float x) { return x - kHalf >= kBoxL && x + kHalf <= kBoxR; }

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (hold_ > 0.04f) return 3;
    if (inside_) return 2;
    return 1;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    showTitle();
}

void Game::showTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    inside_ = false;
    hold_ = 0.f;
    idle_ = 0.f;
    why_ = "";
    banner_ = "";
    chime_ = -1;
    snapCam_ = true;
    x_ = 6.f;
    v_ = 0.f;
    legT_ = 0.f;
    camX_ = 16.f;
    camS_ = 9.f;
}

void Game::startRun() {
    x_ = 4.f;
    v_ = 0.f;
    gas_ = 0.f;
    brake_ = 0.f;
    hold_ = 0.f;
    idle_ = 0.f;
    roll_ = 0.f;
    legT_ = 0.f;
    won_ = false;
    over_ = false;
    inside_ = false;
    why_ = "";
    banner_ = "";
    chime_ = -1;
    shake_ = 0.f;
    mode_ = Mode::Run;
    snapCam_ = true;
    camX_ = x_ + 6.f;
    camS_ = 12.f;
    blip(420.f);
}

void Game::pilot(float& gas, float& brake) const {
    const float err = kGoal - x_;
    const float stopD = (v_ * v_) / (2.f * 13.2f);
    if (err <= 0.04f) {
        gas = 0.f;
        brake = v_ > 0.02f ? 1.f : 0.35f;
        return;
    }
    if (v_ > 0.2f && stopD >= err - 0.12f) {
        gas = 0.f;
        brake = 1.f;
        return;
    }
    if (v_ < 0.35f && err < 2.4f) {
        gas = 0.42f;
        brake = 0.f;
        return;
    }
    if (v_ < 9.2f) {
        gas = 1.f;
        brake = 0.f;
    } else {
        gas = 0.1f;
        brake = 0.f;
    }
}

void Game::win() {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    why_ = "boxed";
    banner_ = "BOXED";
    v_ = 0.f;
    chime_ = 0;
    chimeT_ = 0.f;
    sys_->rumble(0.18f, 0.04f, 110);
    sys_->setLight(40, 160, 60);
}

void Game::fail(const char* why, const char* banner) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    why_ = why;
    banner_ = banner;
    shake_ = 1.f;
    sys_->rumble(0.4f, 0.16f, 140);
    sys_->setLight(160, 30, 20);
    sys_->apu.noiseBurst(0.3f, 180.f, 0.22f);
}

void Game::physics(float gas, float brake) {
    gas_ = clampf(gas, 0.f, 1.f);
    brake_ = clampf(brake, 0.f, 1.f);
    legT_ += kDt;
    anim_ += kDt;

    float a = gas_ * kDrive - brake_ * kBrake;
    a -= v_ * (brake_ > 0.2f ? 0.35f : 0.55f);
    if (gas_ < 0.05f && brake_ < 0.05f) a -= v_ * 0.9f;
    v_ += a * kDt;
    if (v_ < 0.f) v_ = 0.f;
    if (v_ > 14.f) v_ = 14.f;
    if (brake_ > 0.45f && v_ < 0.08f) v_ = 0.f;
    x_ += v_ * kDt;
    roll_ += v_ * kDt * 2.4f;

    inside_ = bodyInside(x_);
    if (inside_ && v_ <= kStopSpd) {
        hold_ += kDt;
        idle_ = 0.f;
        if (hold_ >= kHoldNeed) win();
    } else {
        hold_ = 0.f;
        if (v_ <= kStopSpd && legT_ > 0.4f) idle_ += kDt;
        else idle_ = 0.f;
    }

    if (mode_ != Mode::Run) return;
    if (x_ + kHalf > kYardEnd) fail("overshot the yard", "OVER");
    else if (legT_ > kLimit) fail("the clock ran out", "TIME");
    else if (idle_ > kIdleFail && !inside_) fail("stopped outside the box", "OUT");
}

void Game::blip(float freq) { sys_->apu.tone(0, freq, 0.08f); }

void Game::audio() {
    if (chime_ >= 0) {
        chimeT_ += kDt;
        static const float notes[] = {523.f, 659.f, 784.f, 1046.f};
        int step = int(chimeT_ / 0.14f);
        if (step != chime_ && step < 4) {
            chime_ = step;
            sys_->apu.tone(1, notes[step], 0.12f);
        }
        if (step >= 4) {
            chime_ = -1;
            sys_->apu.tone(1, 0.f, 0.f);
        }
    }
    if (mode_ != Mode::Run) {
        sys_->apu.tone(2, 0.f, 0.f);
        sys_->apu.noise(0.f, 0.f, false);
        return;
    }
    if (gas_ > 0.3f) sys_->apu.tone(2, 70.f + v_ * 11.f, 0.035f);
    else sys_->apu.tone(2, 0.f, 0.f);
    float grit = clampf(v_ / 12.f, 0.f, 1.f) * 0.035f;
    if (brake_ > 0.4f && v_ > 0.4f) grit += 0.04f;
    sys_->apu.noise(grit, 240.f + v_ * 20.f, false);
    if (inside_) sys_->setLight(30, 120, 50);
    else sys_->setLight(20, 40, 30);
}

void Game::sky() {
    uint16_t zen = gs::rgb4(4, 7, 12);
    uint16_t mid = gs::rgb4(8, 11, 14);
    uint16_t hor = gs::rgb4(13, 12, 8);
    if (mode_ == Mode::Fail) hor = gs::rgb4(12, 6, 4);
    if (mode_ == Mode::Win) hor = gs::rgb4(8, 13, 7);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float t = y / float(gs::SCREEN_H - 1);
        auto mix = [&](uint16_t a, uint16_t b, float u) {
            auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
            auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * u)); };
            return gs::rgb4(L(8), L(4), L(0));
        };
        sys_->vdp.lineBackdrop[y] = t < 0.55f ? mix(zen, mid, t / 0.55f) : mix(mid, hor, (t - 0.55f) / 0.45f);
        sys_->vdp.lineFog[y] = 0;
        sys_->vdp.road[y].on = false;
    }
    sys_->vdp.setFogColor(gs::rgb4(6, 7, 6));
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::spr(const gs::Mipped& m, float cx, float cy, float ht, int pal, bool flip) {
    if (ht < 1.2f || m.h < 1) return;
    float w = ht * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(ht)), 1L, 2000L));
    s.x = int16_t(std::clamp(long(std::lround(cx - s.w * 0.5f)), -8000L, 8000L));
    s.y = int16_t(std::clamp(long(std::lround(cy - s.h * 0.5f)), -8000L, 8000L));
    if (s.x > gs::SCREEN_W + 8 || s.x + s.w < -8 || s.y > gs::SCREEN_H + 8 || s.y + s.h < -8) return;
    s.img = m.pick(ht);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::sprBox(const gs::Mipped& m, float cx, float top, float w, float h, int pal) {
    if (w < 1.2f || h < 1.2f || m.h < 1) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::clamp(long(std::lround(cx - s.w * 0.5f)), -8000L, 8000L));
    s.y = int16_t(std::clamp(long(std::lround(top)), -8000L, 8000L));
    if (s.x > gs::SCREEN_W + 4 || s.x + s.w < -4 || s.y > gs::SCREEN_H + 4 || s.y + s.h < -4) return;
    s.img = m.pick(std::max(w, h));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::text(const char* s, float x, float y, float scale, int pal) {
    const float adv = 12.f * scale;
    x -= float(std::strlen(s)) * adv * 0.5f;
    for (int i = 0; s[i]; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + float(i) * adv + g.w * scale * 0.5f, y, std::max(8.f, g.h * scale), pal, false);
    }
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    sky();

    float wantS = 11.5f;
    float wantX = (mode_ == Mode::Title) ? 18.f : x_ + clampf(v_ * 0.25f, 0.f, 4.f);
    if (mode_ == Mode::Win || mode_ == Mode::Fail) wantX = kGoal;
    if (snapCam_) {
        camX_ = wantX;
        camS_ = wantS;
        snapCam_ = false;
    } else {
        camX_ += (wantX - camX_) * 0.12f;
        camS_ += (wantS - camS_) * 0.08f;
    }
    float shx = 0.f;
    if (shake_ > 0.f) {
        shx = std::sin(legT_ * 60.f) * shake_ * 3.f;
        shake_ *= 0.88f;
        if (shake_ < 0.04f) shake_ = 0.f;
    }
    const float scale = camS_;
    const float ax = 150.f + shx;
    const float groundY = 168.f;
    auto sxOf = [&](float wx) { return ax + (wx - camX_) * scale; };

    if (mode_ == Mode::Title) text("KART BOX", 160, 28, 1.6f, PAL_AMBER);
    else if (mode_ == Mode::Pause) text("PAUSE", 160, 28, 1.5f, PAL_HUD);
    else if (mode_ == Mode::Fail) text(banner_, 160, 26, 1.7f, PAL_BAD);
    else if (mode_ == Mode::Win) text("BOXED", 160, 24, 1.8f, PAL_GOOD);

    float kx = sxOf(x_);
    float bodyH = 1.15f * scale;
    spr(art_.body, kx, groundY - 0.55f * scale - bodyH * 0.35f, bodyH, PAL_KART, false);
    float wh = 0.72f * scale;
    bool spoke = std::fmod(roll_, 0.8f) > 0.4f;
    spr(art_.wheel, sxOf(x_ - 0.72f), groundY - wh * 0.35f, wh, PAL_WHEEL, spoke);
    spr(art_.wheel, sxOf(x_ + 0.78f), groundY - wh * 0.35f, wh, PAL_WHEEL, spoke);
    if (v_ > 2.f || brake_ > 0.5f) spr(art_.dust, sxOf(x_ - 1.3f), groundY - 6.f, 10.f + v_, PAL_DUST, false);

    float postH = 2.4f * scale;
    spr(art_.post, sxOf(kBoxL), groundY - postH * 0.45f, postH, PAL_BOX, false);
    spr(art_.post, sxOf(kBoxR), groundY - postH * 0.45f, postH, PAL_BOX, false);
    float tapeY = groundY - 0.15f * scale;
    sprBox(art_.stripe, (sxOf(kBoxL) + sxOf(kBoxR)) * 0.5f, tapeY, (kBoxR - kBoxL) * scale, std::max(4.f, 0.22f * scale),
           PAL_TAPE);

    spr(art_.cone, sxOf(kBoxL - 1.6f), groundY - 0.7f * scale, 1.3f * scale, PAL_CONE, false);
    spr(art_.cone, sxOf(kBoxR + 1.6f), groundY - 0.7f * scale, 1.3f * scale, PAL_CONE, true);

    for (int i = -1; i < 16; i++) {
        float wx = float(i) * 2.6f;
        sprBox(art_.slab, sxOf(wx), groundY, 2.7f * scale, 0.55f * scale, PAL_YARD);
    }
    for (int i = 0; i < 4; i++) {
        float sxc = std::fmod(40.f + float(i) * 110.f - camX_ * 0.4f, 440.f);
        if (sxc < -30.f) sxc += 440.f;
        spr(art_.cloud, sxc, 36.f + float(i % 3) * 12.f, 16.f, PAL_SKY, i & 1);
    }

    char line[48];
    if (mode_ == Mode::Title) {
        hudC(24, "STOP INSIDE THE BOX", PAL_AMBER);
        hudC(26, "A GAS   B BRAKE", PAL_HUD);
    } else if (mode_ == Mode::Run || mode_ == Mode::Pause) {
        std::snprintf(line, sizeof line, "SPD %4.1f", v_);
        hud(1, 1, line, PAL_HUD);
        hud(30, 1, inside_ ? "IN BOX" : "YARD", inside_ ? PAL_GOOD : PAL_AMBER);
        if (hold_ > 0.f) hudC(26, "HOLD STILL", PAL_GOOD);
    } else if (mode_ == Mode::Win) {
        hudC(25, "STOPPED INSIDE THE BOX", PAL_GOOD);
    } else if (mode_ == Mode::Fail) {
        hudC(25, why_, PAL_BAD);
        hudC(26, "A RETRY", PAL_HUD);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    gs::Pad& pad = sys.pad;
    const bool go = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C) || bot_;

    if (mode_ == Mode::Title) {
        if (go) startRun();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_B) || bot_) mode_ = Mode::Run;
    } else if (mode_ == Mode::Fail || mode_ == Mode::Win) {
        if (!bot_ && (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_START))) showTitle();
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
    }

    if (mode_ == Mode::Run) {
        float gas = 0.f, brake = 0.f;
        if (bot_) {
            pilot(gas, brake);
        } else {
            if (pad.down(gs::BTN_RIGHT) || pad.down(gs::BTN_A) || pad.down(gs::BTN_C)) gas = 1.f;
            if (pad.accel > 0.15f) gas = std::max(gas, pad.accel);
            if (pad.down(gs::BTN_LEFT) || pad.down(gs::BTN_B) || pad.down(gs::BTN_DOWN)) brake = 1.f;
            if (pad.brake > 0.15f) brake = std::max(brake, pad.brake);
        }
        physics(gas, brake);
    } else {
        anim_ += kDt;
    }
    audio();
    draw();
    if (mode_ != Mode::Run) {
        sys.apu.tone(0, 0.f, 0.f);
    } else if (legT_ > 0.08f) {
        sys.apu.tone(0, 0.f, 0.f);
    }
}

}  // namespace kartbox

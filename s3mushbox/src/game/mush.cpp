#include "game/mush.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace mushbox {
namespace {

constexpr float BRAKE = 0.062f;
constexpr float STEER = 0.038f;
constexpr float DRAG = 0.996f;
constexpr float STOP_V = 0.07f;
constexpr float RAD = 12.f;
constexpr int STAGES = 3;

struct StageDef {
    float x, y, vx, vy;
    float bx, by, bw, bh;
};

const StageDef kStage[STAGES] = {
    {46.f, 108.f, 2.15f, 0.05f, 168.f, 72.f, 100.f, 78.f},
    {40.f, 48.f, 1.55f, 1.15f, 150.f, 118.f, 108.f, 72.f},
    {36.f, 150.f, 2.55f, -0.85f, 176.f, 64.f, 92.f, 70.f},
};

float len(float x, float y) { return std::sqrt(x * x + y * y); }

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.HUD.clear();
    over_ = false;
    won_ = false;
    cleared_ = 0;
    lives_ = 3;
    stage_ = 0;
    t_ = 0;
    note_ = -1;
    if (bot_) begin(0);
    else mode_ = Mode::Title;
}

void Game::begin(int stage) {
    stage_ = stage;
    const StageDef& s = kStage[stage];
    x_ = s.x;
    y_ = s.y;
    vx_ = s.vx;
    vy_ = s.vy;
    bx_ = s.bx;
    by_ = s.by;
    bw_ = s.bw;
    bh_ = s.bh;
    still_ = 0;
    mode_ = Mode::Slide;
    t_ = 0;
}

bool Game::inBox() const {
    return x_ - RAD >= bx_ && x_ + RAD <= bx_ + bw_ && y_ - RAD >= by_ && y_ + RAD <= by_ + bh_;
}

bool Game::offIce() const { return x_ < 16.f || x_ > 304.f || y_ < 28.f || y_ > 208.f; }

bool Game::settled() const { return len(vx_, vy_) < STOP_V; }

void Game::bot(float& ix, float& iy, bool& brake) const {
    float cx = bx_ + bw_ * 0.5f;
    float cy = by_ + bh_ * 0.5f;
    float dx = cx - x_;
    float dy = cy - y_;
    float dist = std::max(0.001f, len(dx, dy));
    float tx = dx / dist;
    float ty = dy / dist;
    float sp = len(vx_, vy_);
    float want = std::min(2.3f, std::sqrt(std::max(0.f, 2.f * BRAKE * (dist - 4.f))));
    if (dist < 18.f) want = std::min(want, dist * 0.045f);
    float dvx = tx * want - vx_;
    float dvy = ty * want - vy_;
    ix = std::clamp(dvx / STEER, -1.f, 1.f);
    iy = std::clamp(dvy / STEER, -1.f, 1.f);
    float closing = (vx_ * tx + vy_ * ty);
    brake = closing > want + 0.08f || (dist < 22.f && sp > want);
    if (dist < 10.f && sp < 0.35f) {
        brake = true;
        ix *= 0.25f;
        iy *= 0.25f;
    }
}

void Game::physics(float ix, float iy, bool brake) {
    float ax = ix * STEER;
    float ay = iy * STEER;
    float sp = len(vx_, vy_);
    if (brake && sp > 0.001f) {
        ax -= vx_ / sp * BRAKE;
        ay -= vy_ / sp * BRAKE;
    }
    vx_ = (vx_ + ax) * DRAG;
    vy_ = (vy_ + ay) * DRAG;
    x_ += vx_;
    y_ += vy_;
    if (x_ < 10.f) {
        x_ = 10.f;
        vx_ = std::abs(vx_) * 0.2f;
    }
    if (x_ > 310.f) {
        x_ = 310.f;
        vx_ = -std::abs(vx_) * 0.2f;
    }
    if (y_ < 22.f) {
        y_ = 22.f;
        vy_ = std::abs(vy_) * 0.2f;
    }
    if (y_ > 212.f) {
        y_ = 212.f;
        vy_ = -std::abs(vy_) * 0.2f;
    }
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Slide) return 1;
    if (mode_ == Mode::Banner && lastIn_) return 2;
    return 3;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += 1.f;
    gs::Pad& p = sys.pad;
    if (mode_ == Mode::Title) {
        if (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A) || bot_) {
            lives_ = 3;
            cleared_ = 0;
            over_ = false;
            won_ = false;
            begin(0);
        }
    } else if (mode_ == Mode::Slide) {
        float ix = 0, iy = 0;
        bool brake = false;
        if (bot_) bot(ix, iy, brake);
        else {
            if (p.down(gs::BTN_LEFT)) ix -= 1.f;
            if (p.down(gs::BTN_RIGHT)) ix += 1.f;
            if (p.down(gs::BTN_UP)) iy -= 1.f;
            if (p.down(gs::BTN_DOWN)) iy += 1.f;
            ix += p.axisX;
            iy -= p.axisY;
            ix = std::clamp(ix, -1.f, 1.f);
            iy = std::clamp(iy, -1.f, 1.f);
            brake = p.down(gs::BTN_A) || p.down(gs::BTN_B) || p.down(gs::BTN_C) || p.accel > 0.2f;
        }
        physics(ix, iy, brake);
        float sp = len(vx_, vy_);
        if (sp < STOP_V) still_++;
        else still_ = 0;
        sys.apu.tone(0, 70.f + sp * 36.f, sp > 0.25f ? 0.035f : 0.f);
        sys.apu.tone(1, brake ? 46.f : 0.f, brake && sp > 0.12f ? 0.05f : 0.f);
        bool done = still_ > 18 || t_ > 60.f * 12.f || offIce();
        if (done) {
            lastIn_ = inBox() && !offIce() && still_ > 18;
            if (lastIn_) {
                cleared_++;
                sys.apu.tone(0, 0, 0);
                sys.apu.tone(1, 0, 0);
                sys.apu.keyOn(0, 523.f, 0.2f);
                note_ = 0;
            } else {
                lives_--;
                sys.apu.noiseBurst(0.2f, 1800.f, 0.25f);
            }
            banner_ = 70;
            mode_ = Mode::Banner;
        }
    } else if (mode_ == Mode::Banner) {
        if (note_ >= 0) {
            note_++;
            if (note_ == 10) sys.apu.keyOn(0, 659.f, 0.18f);
            if (note_ == 20) sys.apu.keyOn(0, 784.f, 0.16f);
        }
        if (--banner_ <= 0) {
            sys.apu.keyOff(0);
            note_ = -1;
            if (lastIn_) {
                if (stage_ + 1 >= STAGES) {
                    mode_ = Mode::Win;
                    won_ = true;
                    over_ = true;
                    sys.setLight(40, 140, 60);
                    banner_ = 90;
                } else begin(stage_ + 1);
            } else if (lives_ <= 0) {
                mode_ = Mode::Lose;
                over_ = true;
                won_ = false;
                sys.setLight(140, 30, 20);
            } else begin(stage_);
        }
    } else if (mode_ == Mode::Win || mode_ == Mode::Lose) {
        if (!bot_ && (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A))) {
            over_ = false;
            won_ = false;
            mode_ = Mode::Title;
            sys.apu.silence();
        }
    }
    draw();
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 1.1f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, bool shadow) {
    if (w < 1.f || h < 1.f || m.h < 1) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    float adv = 16.f * scale;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        spr(art_.glyph[c - 32], x + float(i) * adv, y, 18.f * scale, pal, false);
    }
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H - 1);
        int r = 6 + int(u * 3);
        int g = 10 + int((1.f - u) * 3);
        int b = 13 + int((1.f - u) * 2);
        v.lineBackdrop[y] = gs::rgb4(r, g, b);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
    stamp(art_.box, bx_ + bw_ * 0.5f, by_ + bh_ * 0.5f, bw_, bh_, PAL_BOX);
    for (int i = 0; i < 5; i++) {
        float sx = std::fmod(40.f + i * 58.f + t_ * 0.15f, 320.f);
        float sy = 30.f + (i * 37 % 150);
        spr(art_.spark, sx, sy, 6.f, PAL_ICE);
    }
    float sp = len(vx_, vy_);
    int pose = sp > 1.4f ? 1 : (sp < 0.2f ? 2 : 0);
    stamp(art_.shadow, x_ + 3.f, y_ + 8.f, 30.f, 12.f, PAL_MUSH, true);
    spr(art_.mush[pose], x_, y_, 30.f + std::min(8.f, sp * 2.f), PAL_MUSH, vx_ < -0.05f);

    if (mode_ == Mode::Title) {
        text("MUSHBOX", 36.f, 64.f, 1.15f, PAL_GOLD);
        hudC(12, "STOP INSIDE THE BOX", PAL_HUD);
        hudC(15, "ARROWS STEER", PAL_HUD);
        hudC(16, "A BRAKES", PAL_HUD);
        hudC(20, "START", PAL_GOLD);
    } else {
        char line[40];
        std::snprintf(line, sizeof(line), "BOX %d/%d", stage_ + 1, STAGES);
        hud(1, 0, line, PAL_HUD);
        std::snprintf(line, sizeof(line), "TRIES %d", lives_);
        hud(30, 0, line, PAL_HUD);
        int spd = int(std::lround(sp * 10.f));
        std::snprintf(line, sizeof(line), "SLIDE %d", spd);
        hud(1, 26, line, inBox() ? PAL_HUD : PAL_HUD);
        hud(28, 26, inBox() ? "IN" : "OUT", inBox() ? PAL_HUD : PAL_HUD);
        if (mode_ == Mode::Banner) hudC(14, lastIn_ ? "IN THE BOX" : "MISSED", lastIn_ ? PAL_GOLD : PAL_HUD);
        if (mode_ == Mode::Win) {
            hudC(12, "THE MUSH STOPPED", PAL_GOLD);
            hudC(14, "INSIDE THE BOX", PAL_GOLD);
            if (!bot_) hudC(18, "START", PAL_HUD);
        }
        if (mode_ == Mode::Lose) {
            hudC(13, "THE MUSH DID NOT", PAL_HUD);
            hudC(15, "STOP IN THE BOX", PAL_HUD);
            if (!bot_) hudC(18, "START RETRIES", PAL_HUD);
        }
    }
}

}  // namespace mushbox

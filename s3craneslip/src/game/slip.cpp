#include "game/slip.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace slip {
namespace {

constexpr float RAIL_Y = 34.f;
constexpr float COPING = 156.f;
constexpr float SLIP_L = 198.f;
constexpr float SLIP_R = 262.f;
constexpr float SLIP_X = 230.f;
constexpr float FLOOR_Y = 190.f;
constexpr float CLOCK_MAX = 42.f;
constexpr float BARGE_X0 = 108.f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

void Game::hookAt(float& x, float& y) const {
    x = tx_;
    y = RAIL_Y + len_;
}

bool Game::inSlip(float x) const { return x > SLIP_L + 16.f && x < SLIP_R - 16.f; }

bool Game::startPressed() const {
    const gs::Pad& p = sys_->pad;
    return p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A);
}

bool Game::actPressed() const {
    const gs::Pad& p = sys_->pad;
    return p.pressed(gs::BTN_C) || p.pressed(gs::BTN_Z) || p.pressed(gs::BTN_B);
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.08f);
    melodyT_ = 0.07f;
    melody_ = -2;
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

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 1.f || m.h < 1) return;
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

void Game::begin() {
    held_ = false;
    won_ = false;
    over_ = false;
    actLatch_ = false;
    tx_ = 72.f;
    vx_ = 0;
    len_ = 28.f;
    bx_ = BARGE_X0;
    by_ = 176.f;
    bvx_ = 0;
    clock_ = CLOCK_MAX;
    water_ = 186.f;
    job_ = Job::ToBarge;
    melody_ = -1;
    mode_ = Mode::Play;
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(1, 0, 0);
    sys_->apu.noise(0, 0, false);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = true;
    sys.vdp.B.scroll(0, 0);
    sys.vdp.setFogColor(gs::rgb4(6, 9, 12));
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int k = y < 100 ? y : 100;
        sys.vdp.lineBackdrop[y] = gs::rgb4(6 + k / 28, 8 + k / 24, 12 + k / 40);
    }
    if (bot_) begin();
    else mode_ = Mode::Title;
}

bool Game::grab() {
    if (held_ || mode_ != Mode::Play) return false;
    float hx, hy;
    hookAt(hx, hy);
    float dx = hx - bx_;
    float dy = hy - (by_ - 8.f);
    if (std::fabs(dx) > 16.f || std::fabs(dy) > 16.f) return false;
    held_ = true;
    bvx_ = 0;
    blip(640.f);
    sys_->rumble(0.2f, 0.35f, 50);
    return true;
}

void Game::release() {
    if (!held_) return;
    held_ = false;
    float hx, hy;
    hookAt(hx, hy);
    bx_ = hx;
    by_ = hy + 10.f;
    bool pocket = inSlip(bx_) && by_ > COPING + 4.f && water_ > COPING + 2.f;
    if (pocket) {
        by_ = std::max(by_, FLOOR_Y - 6.f);
        mode_ = Mode::Win;
        won_ = true;
        over_ = true;
        melody_ = 0;
        melodyT_ = 0;
        blip(880.f);
        sys_->rumble(0.35f, 0.55f, 160);
        sys_->apu.noise(0, 0, false);
    } else {
        blip(210.f);
    }
}

void Game::botPlan(float& ax, float& hoist, bool& act) {
    ax = 0;
    hoist = 0;
    act = false;
    if (mode_ != Mode::Play) return;

    auto go = [&](float x, float wantLen) {
        float dx = x - tx_;
        if (std::fabs(dx) > 2.5f) ax = dx > 0 ? 1.f : -1.f;
        float dl = wantLen - len_;
        if (std::fabs(dl) > 1.5f) hoist = dl > 0 ? 1.f : -1.f;
    };

    if (!held_) {
        if (job_ != Job::LowerGrab) job_ = Job::ToBarge;
        if (job_ == Job::ToBarge) {
            go(bx_, 22.f);
            if (std::fabs(bx_ - tx_) < 3.f && len_ < 30.f) job_ = Job::LowerGrab;
        } else {
            float need = (by_ - 8.f) - RAIL_Y;
            go(bx_, need);
            float hx, hy;
            hookAt(hx, hy);
            bool near = std::fabs(hx - bx_) < 10.f && std::fabs(hy - (by_ - 8.f)) < 10.f;
            if (near && !actLatch_) {
                act = true;
                actLatch_ = true;
            }
        }
    } else {
        actLatch_ = false;
        if (job_ == Job::ToBarge || job_ == Job::LowerGrab) job_ = Job::Raise;
        if (job_ == Job::Raise) {
            go(bx_, 36.f);
            if (len_ < 42.f) job_ = Job::ToSlip;
        } else if (job_ == Job::ToSlip) {
            go(SLIP_X, 40.f);
            if (std::fabs(tx_ - SLIP_X) < 3.f && len_ < 48.f) job_ = Job::LowerDrop;
        } else {
            float need = (FLOOR_Y - 4.f) - RAIL_Y;
            go(SLIP_X, need);
            float hx, hy;
            hookAt(hx, hy);
            if (inSlip(hx) && hy + 10.f > COPING + 18.f && !actLatch_) {
                act = true;
                actLatch_ = true;
            }
        }
    }
}

void Game::update(float dt) {
    float ax = 0, hoist = 0;
    bool act = false;
    if (bot_) botPlan(ax, hoist, act);
    else {
        const gs::Pad& p = sys_->pad;
        if (p.down(gs::BTN_LEFT) || p.axisX < -0.3f) ax -= 1.f;
        if (p.down(gs::BTN_RIGHT) || p.axisX > 0.3f) ax += 1.f;
        if (p.down(gs::BTN_UP)) hoist -= 1.f;
        if (p.down(gs::BTN_DOWN)) hoist += 1.f;
        act = actPressed();
    }

    if (act) {
        if (held_) release();
        else grab();
    }

    vx_ += ax * 540.f * dt;
    vx_ *= std::pow(0.035f, dt);
    tx_ += vx_ * dt;
    tx_ = clampf(tx_, 28.f, 300.f);
    if (tx_ <= 28.f || tx_ >= 300.f) vx_ = 0;
    len_ += hoist * 96.f * dt;
    len_ = clampf(len_, 16.f, 168.f);

    if (held_) {
        float hx, hy;
        hookAt(hx, hy);
        bx_ += (hx - bx_) * std::min(1.f, 10.f * dt);
        by_ += ((hy + 10.f) - by_) * std::min(1.f, 10.f * dt);
        bvx_ = 0;
    } else if (mode_ == Mode::Play) {
        float target = water_ - 12.f;
        bool pocket = inSlip(bx_) && by_ > COPING;
        if (pocket && water_ > COPING) target = FLOOR_Y;
        by_ += (target - by_) * std::min(1.f, 4.f * dt);
        if (!pocket) {
            bvx_ += (BARGE_X0 - bx_) * 0.4f * dt;
            bvx_ *= std::pow(0.2f, dt);
            bx_ += bvx_ * dt;
            bx_ = clampf(bx_, 40.f, 300.f);
        }
        if (pocket && by_ > FLOOR_Y - 8.f && water_ > COPING + 1.f) {
            mode_ = Mode::Win;
            won_ = true;
            over_ = true;
            melody_ = 0;
            melodyT_ = 0;
            sys_->rumble(0.35f, 0.5f, 140);
            sys_->apu.noise(0, 0, false);
        }
    }

    if (mode_ == Mode::Play) {
        clock_ -= dt;
        float rise = 1.f - clampf(clock_ / CLOCK_MAX, 0.f, 1.f);
        water_ = 186.f - rise * 36.f;
        if (std::fabs(hoist) > 0.2f) sys_->apu.tone(1, 70.f + len_ * 0.5f, 0.028f);
        else sys_->apu.tone(1, 0, 0);
        sys_->apu.noise(0.018f + rise * 0.05f, 700.f + rise * 1600.f, false);
        if ((clock_ <= 0.f || water_ <= COPING) && !won_) {
            clock_ = std::max(0.f, clock_);
            mode_ = Mode::Fail;
            over_ = true;
            melody_ = 0;
            melodyT_ = 0;
            sys_->apu.tone(1, 0, 0);
            sys_->apu.noise(0, 0, false);
            sys_->rumble(0.55f, 0.2f, 200);
        }
    }

    if (melody_ == -2) {
        melodyT_ -= dt;
        if (melodyT_ <= 0.f) {
            melody_ = -1;
            sys_->apu.tone(0, 0, 0);
        }
    } else if (melody_ >= 0 && melody_ < 4) {
        static const float winN[] = {494.f, 659.f, 784.f, 988.f};
        static const float loseN[] = {370.f, 311.f, 247.f, 196.f};
        const float* notes = won_ ? winN : loseN;
        if (melodyT_ <= 0.f) sys_->apu.tone(0, notes[melody_], 0.08f);
        melodyT_ += dt;
        if (melodyT_ > 0.16f) {
            melodyT_ = 0;
            melody_++;
            if (melody_ >= 4) sys_->apu.tone(0, 0, 0);
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const float dt = 1.f / 60.f;
    t_ += dt;
    if ((mode_ == Mode::Title || mode_ == Mode::Win || mode_ == Mode::Fail) && startPressed() && !bot_) begin();
    if (mode_ == Mode::Title) {
        tx_ = 150.f + std::sin(t_ * 0.55f) * 40.f;
        len_ = 64.f + std::sin(t_ * 0.7f) * 10.f;
        vx_ = 0;
        bx_ = 108.f + std::sin(t_ * 0.4f) * 6.f;
        by_ = 174.f;
        water_ = 186.f;
        clock_ = CLOCK_MAX;
        held_ = false;
    } else {
        update(dt);
    }
    draw();
}

void Game::draw() {
    sys_->vdp.clearSprites();
    sys_->vdp.HUD.clear();

    float rise = mode_ == Mode::Title ? 0.05f : 1.f - clampf(clock_ / CLOCK_MAX, 0.f, 1.f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int k = y < 100 ? y : 100;
        int r = 6 + k / 28 - int(rise * 2);
        int g = 8 + k / 24 - int(rise * 3);
        int b = 12 + k / 40 - int(rise);
        sys_->vdp.lineBackdrop[y] = gs::rgb4(std::max(2, r), std::max(3, g), std::max(5, b));
    }

    float hx, hy;
    hookAt(hx, hy);
    spr(art_.trolley, tx_, RAIL_Y - 2.f, 22.f, PAL_CRANE);
    spr(art_.hook, hx, hy, 16.f, PAL_HOOK);
    float y0 = RAIL_Y + 8.f;
    int links = std::max(1, int((hy - y0) / 6.f));
    for (int i = 0; i < links; i++) {
        float y = y0 + (hy - y0) * (float(i) + 0.5f) / float(links);
        spr(art_.link, hx, y, 7.f, PAL_CABLE);
    }

    float gullX = std::fmod(t_ * 28.f, 360.f) - 20.f;
    spr(art_.gull, gullX, 58.f + std::sin(t_ * 3.f) * 3.f, 8.f, PAL_GULL);
    spr(art_.barge, bx_, by_, 22.f, PAL_BARGE);
    spr(art_.lamp, SLIP_L - 2.f, 124.f, 16.f, PAL_CRANE);
    spr(art_.lamp, SLIP_R + 2.f, 124.f, 16.f, PAL_CRANE);
    spr(art_.pile, SLIP_L, 168.f, 78.f, PAL_PILE);
    spr(art_.pile, SLIP_R, 168.f, 78.f, PAL_PILE, true);

    float waterH = std::max(8.f, 224.f - water_);
    gs::Sprite sea;
    sea.w = gs::SCREEN_W;
    sea.h = int16_t(std::lround(waterH));
    sea.x = 0;
    sea.y = int16_t(std::lround(water_));
    sea.img = art_.water.pick(waterH);
    sea.pal = PAL_WATER;
    sys_->vdp.sprite(sea);

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(3, "S3 CRANESLIP", PAL_SIGN);
        hudC(6, "BERTH IN THE SLIP", PAL_WHITE);
        hudC(8, "BEFORE THE TIDE TURNS", PAL_AMBER);
        hudC(18, "ARROWS MOVE   C GRABS", PAL_WHITE);
        hudC(21, "START", PAL_GREEN);
    } else {
        std::snprintf(buf, sizeof(buf), "TIDE %04.1f", std::max(0.f, clock_));
        hud(1, 1, buf, clock_ < 10.f ? PAL_RED : PAL_AMBER);
        hud(28, 1, held_ ? "HOLD" : "HOOK", held_ ? PAL_GREEN : PAL_WHITE);
        if (mode_ == Mode::Win) hudC(12, "BERTHED", PAL_GREEN);
        if (mode_ == Mode::Fail) hudC(12, "TIDE TURNED", PAL_RED);
        if (mode_ != Mode::Play) hudC(16, "START", PAL_WHITE);
    }
}

}  // namespace slip

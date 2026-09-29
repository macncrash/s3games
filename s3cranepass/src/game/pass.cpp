#include "game/pass.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace pass {
namespace {

constexpr float RAIL_Y = 36.f;
constexpr float GROUND = 164.f;
constexpr float DUMP_X = 48.f;
constexpr float CLOCK_MAX = 78.f;
constexpr float PIECE_X[4] = {112.f, 164.f, 214.f, 262.f};
constexpr float PIECE_H[4] = {24.f, 16.f, 22.f, 16.f};
constexpr int PIECE_PAL[4] = {PAL_ROCK, PAL_LOG, PAL_JEEP, PAL_SLAB};

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

void Game::hookAt(float& x, float& y) const {
    x = tx_;
    y = RAIL_Y + len_;
}

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

const gs::Mipped& pieceImg(const Art& a, int kind) {
    if (kind == 1) return a.log;
    if (kind == 2) return a.jeep;
    if (kind == 3) return a.slab;
    return a.rock;
}

void Game::begin() {
    for (int i = 0; i < 4; i++) {
        pieces_[i].x = PIECE_X[i];
        pieces_[i].kind = i;
        pieces_[i].gone = false;
    }
    carrying_ = -1;
    cleared_ = 0;
    target_ = 0;
    won_ = false;
    over_ = false;
    actLatch_ = false;
    tx_ = 90.f;
    vx_ = 0;
    len_ = 24.f;
    clock_ = CLOCK_MAX;
    job_ = Job::ToPiece;
    melody_ = -1;
    mode_ = Mode::Play;
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(1, 0, 0);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = true;
    sys.vdp.B.scroll(0, 0);
    sys.vdp.setFogColor(gs::rgb4(8, 9, 11));
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int k = y < 90 ? y : 90;
        sys.vdp.lineBackdrop[y] = gs::rgb4(4 + k / 18, 6 + k / 22, 10 + k / 30);
    }
    if (bot_) begin();
    else mode_ = Mode::Title;
}

bool Game::grab() {
    if (carrying_ >= 0) return false;
    float hx, hy;
    hookAt(hx, hy);
    int best = -1;
    float bestD = 1e9f;
    for (int i = 0; i < 4; i++) {
        if (pieces_[i].gone) continue;
        float top = GROUND - PIECE_H[i];
        float dx = hx - pieces_[i].x;
        float dy = hy - (top + 4.f);
        if (std::fabs(dx) > 18.f || std::fabs(dy) > 16.f) continue;
        float d = dx * dx + dy * dy;
        if (d < bestD) {
            bestD = d;
            best = i;
        }
    }
    if (best < 0) return false;
    carrying_ = best;
    blip(620.f);
    sys_->rumble(0.2f, 0.3f, 50);
    return true;
}

void Game::release() {
    if (carrying_ < 0) return;
    float hx, hy;
    hookAt(hx, hy);
    Piece& p = pieces_[carrying_];
    bool dump = hx > 18.f && hx < 82.f && hy > GROUND - 28.f;
    if (dump) {
        p.gone = true;
        cleared_++;
        blip(880.f);
        if (cleared_ >= 4) {
            mode_ = Mode::Win;
            won_ = true;
            over_ = true;
            melody_ = 0;
            melodyT_ = 0;
            sys_->rumble(0.4f, 0.6f, 180);
        }
    } else {
        p.x = clampf(hx, 96.f, 300.f);
        blip(220.f);
    }
    carrying_ = -1;
}

void Game::botPlan(float& ax, float& hoist, bool& act) {
    ax = 0;
    hoist = 0;
    act = false;
    if (mode_ != Mode::Play) return;

    auto go = [&](float x, float wantLen) {
        float dx = x - tx_;
        if (std::fabs(dx) > 3.f) ax = dx > 0 ? 1.f : -1.f;
        float dl = wantLen - len_;
        if (std::fabs(dl) > 2.f) hoist = dl > 0 ? 1.f : -1.f;
    };

    if (carrying_ < 0) {
        target_ = -1;
        for (int i = 0; i < 4; i++)
            if (!pieces_[i].gone) {
                target_ = i;
                break;
            }
        if (target_ < 0) return;
        if (job_ == Job::Raise || job_ == Job::ToDump || job_ == Job::LowerDrop) job_ = Job::ToPiece;
        if (job_ == Job::ToPiece) {
            go(pieces_[target_].x, 22.f);
            if (std::fabs(pieces_[target_].x - tx_) < 4.f && len_ < 28.f) job_ = Job::LowerGrab;
        } else {
            float top = GROUND - PIECE_H[target_] + 4.f;
            float need = top - RAIL_Y;
            go(pieces_[target_].x, need);
            float hx, hy;
            hookAt(hx, hy);
            bool near = std::fabs(hx - pieces_[target_].x) < 12.f && std::fabs(hy - top) < 12.f;
            if (near && !actLatch_) {
                act = true;
                actLatch_ = true;
            }
            if (!near) actLatch_ = false;
            if (carrying_ >= 0) job_ = Job::Raise;
        }
    } else {
        actLatch_ = false;
        if (job_ == Job::ToPiece || job_ == Job::LowerGrab) job_ = Job::Raise;
        if (job_ == Job::Raise) {
            go(tx_, 18.f);
            if (len_ < 24.f) job_ = Job::ToDump;
        } else if (job_ == Job::ToDump) {
            go(DUMP_X, 18.f);
            if (std::fabs(tx_ - DUMP_X) < 4.f && len_ < 24.f) job_ = Job::LowerDrop;
        } else {
            go(DUMP_X, 120.f);
            if (len_ > 108.f && std::fabs(tx_ - DUMP_X) < 8.f && !actLatch_) {
                act = true;
                actLatch_ = true;
            }
        }
    }
}

void Game::update(float dt) {
    float ax = 0, hoist = 0;
    bool act = false;
    if (bot_) {
        botPlan(ax, hoist, act);
    } else {
        const gs::Pad& p = sys_->pad;
        if (p.down(gs::BTN_LEFT) || p.axisX < -0.3f) ax -= 1.f;
        if (p.down(gs::BTN_RIGHT) || p.axisX > 0.3f) ax += 1.f;
        if (p.down(gs::BTN_UP)) hoist -= 1.f;
        if (p.down(gs::BTN_DOWN)) hoist += 1.f;
        act = actPressed();
    }

    if (act) {
        if (carrying_ >= 0) release();
        else grab();
        if (carrying_ >= 0 && bot_) job_ = Job::Raise;
        if (carrying_ < 0 && bot_ && job_ == Job::LowerDrop) job_ = Job::ToPiece;
    }

    vx_ += ax * 520.f * dt;
    vx_ *= std::pow(0.04f, dt);
    tx_ += vx_ * dt;
    tx_ = clampf(tx_, 24.f, 300.f);
    if (tx_ <= 24.f || tx_ >= 300.f) vx_ = 0;
    len_ += hoist * 92.f * dt;
    len_ = clampf(len_, 14.f, 132.f);

    if (mode_ == Mode::Play) {
        clock_ -= dt;
        if (std::fabs(hoist) > 0.2f) sys_->apu.tone(1, 90.f + len_ * 0.4f, 0.03f);
        else sys_->apu.tone(1, 0, 0);
        float wind = 0.02f + (1.f - clock_ / CLOCK_MAX) * 0.05f;
        sys_->apu.noise(wind, 900.f + (1.f - clock_ / CLOCK_MAX) * 1400.f, false);
        if (clock_ <= 0.f && !won_) {
            clock_ = 0;
            mode_ = Mode::Fail;
            over_ = true;
            melody_ = 0;
            melodyT_ = 0;
            sys_->apu.tone(1, 0, 0);
            sys_->apu.noise(0, 0, false);
            sys_->rumble(0.6f, 0.2f, 220);
        }
    }

    if (melody_ == -2) {
        melodyT_ -= dt;
        if (melodyT_ <= 0.f) {
            melody_ = -1;
            sys_->apu.tone(0, 0, 0);
        }
    } else if (melody_ >= 0 && melody_ < 4) {
        static const float winN[] = {523.f, 659.f, 784.f, 1046.f};
        static const float loseN[] = {392.f, 349.f, 294.f, 220.f};
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
        tx_ = 150.f + std::sin(t_ * 0.6f) * 36.f;
        len_ = 70.f + std::sin(t_ * 0.8f) * 8.f;
        vx_ = 0;
    } else {
        update(dt);
    }
    draw();
}

void Game::draw() {
    sys_->vdp.clearSprites();
    sys_->vdp.HUD.clear();

    float storm = mode_ == Mode::Title ? 0.15f : 1.f - clock_ / CLOCK_MAX;
    storm = clampf(storm, 0.f, 1.f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int k = y < 90 ? y : 90;
        int r = 4 + k / 18 - int(storm * 3);
        int g = 6 + k / 22 - int(storm * 4);
        int b = 10 + k / 30 - int(storm * 2);
        sys_->vdp.lineBackdrop[y] = gs::rgb4(std::max(1, r), std::max(1, g), std::max(2, b));
    }

    float hx, hy;
    hookAt(hx, hy);
    float bob = std::sin(t_ * 3.f) * 2.f;

    spr(art_.hook, hx, hy, 16.f, PAL_HOOK);
    if (carrying_ >= 0) {
        float ph = PIECE_H[pieces_[carrying_].kind];
        spr(pieceImg(art_, pieces_[carrying_].kind), hx, hy + 8.f + ph * 0.5f, ph,
            PIECE_PAL[pieces_[carrying_].kind]);
    }
    float dist = hy - RAIL_Y;
    int links = std::max(1, int(dist / 6.f));
    for (int i = 1; i < links; i++) {
        float u = float(i) / float(links);
        spr(art_.link, tx_, RAIL_Y + dist * u, 6.f, PAL_CABLE);
    }
    spr(art_.trolley, tx_, RAIL_Y - 6.f, 22.f, PAL_CRANE);

    for (int i = 0; i < 4; i++) {
        if (pieces_[i].gone || i == carrying_) continue;
        float ph = PIECE_H[i];
        float py = GROUND - ph * 0.45f;
        if (mode_ == Mode::Play && carrying_ < 0 && i == target_)
            spr(art_.arrow, pieces_[i].x, py - ph * 0.7f + bob, 10.f, PAL_AMBER);
        spr(pieceImg(art_, pieces_[i].kind), pieces_[i].x, py, ph, PIECE_PAL[i]);
    }

    float rivalX = 300.f - storm * 150.f;
    spr(art_.rival, rivalX, 108.f, 18.f, PAL_RIVAL);
    spr(art_.gate, 252.f, 140.f, 28.f, PAL_RIVAL);
    spr(art_.gate, 290.f, 136.f, 32.f, PAL_RIVAL);

    int flakes = 8 + int(storm * 10);
    for (int i = 0; i < flakes; i++) {
        float x = std::fmod(i * 47.f + t_ * (18.f + storm * 40.f) + i * 13.f, 320.f);
        float y = std::fmod(i * 31.f + t_ * (28.f + i * 3.f), 200.f);
        spr(art_.flake, x, y, 4.f + (i & 1), PAL_SNOW);
    }

    if (mode_ == Mode::Title) {
        hudC(3, "S3 CRANEPASS", PAL_AMBER);
        hudC(5, "CLEAR THE PASS", PAL_WHITE);
        hudC(7, "BEFORE THEIR STORM CLOCK", PAL_RED);
        hudC(10, "LEFT RIGHT MOVES THE CRANE", PAL_WHITE);
        hudC(11, "UP DOWN HOISTS", PAL_WHITE);
        hudC(12, "Z DUMPS WRECKAGE ON THE PAD", PAL_GREEN);
        hudC(16, "THE OTHER CREW IS THE CLOCK", PAL_AMBER);
        hudC(20, "PRESS START", PAL_GREEN);
        return;
    }

    int sec = int(clock_ + 0.999f);
    if (sec < 0) sec = 0;
    char line[48];
    std::snprintf(line, sizeof line, "CLEARED %d OF 4", cleared_);
    hud(1, 0, "S3 CRANEPASS", PAL_AMBER);
    hud(16, 0, line, PAL_WHITE);
    std::snprintf(line, sizeof line, "CREW %02d", sec);
    hud(32, 0, line, sec < 15 ? PAL_RED : PAL_AMBER);

    if (mode_ == Mode::Win) {
        hudC(6, "PASS CLEAR", PAL_GREEN);
        hudC(8, "AHEAD OF THE OTHER CREW", PAL_WHITE);
        hudC(11, "PRESS START", PAL_AMBER);
    } else if (mode_ == Mode::Fail) {
        hudC(6, "PASS LOST", PAL_RED);
        hudC(8, "THE OTHER CREW CLOSED IT", PAL_WHITE);
        hudC(11, "PRESS START", PAL_AMBER);
    } else if (carrying_ >= 0) {
        hud(1, 26, "DROP ON THE YELLOW PAD", PAL_GREEN);
    } else {
        hud(1, 26, "HOOK THE WRECKAGE  Z GRABS", PAL_WHITE);
    }
}

}  // namespace pass

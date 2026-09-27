#include "game/boccemark.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace boccemark {
namespace {
constexpr float kDt = 1.f / 60.f;
constexpr float kDecel = 92.f;
constexpr float kLeft = 52.f;
constexpr float kRight = 268.f;
constexpr float kTop = 48.f;
constexpr float kBot = 198.f;
constexpr int kJack = 0;
constexpr int kYou = 1;
constexpr int kThem = 2;

float radius(int side) { return side == kJack ? 4.2f : 7.f; }

float len(float x, float y) { return std::sqrt(x * x + y * y); }
}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.HUD.enabled = true;
    sys.apu.setMaster(0.4f);
    begin();
}

void Game::begin() {
    mode_ = Mode::Title;
    phase_ = Phase::Mark;
    over_ = false;
    won_ = false;
    finished_ = false;
    hold_ = 0;
    aim_ = 0.f;
    meter_ = 0.35f;
    meterDir_ = 1.f;
    t_ = 0.f;
    n_ = 0;
}

void Game::launch(Ball& b, float tx, float ty) {
    float dx = tx - b.x;
    float dy = ty - b.y;
    float d = len(dx, dy);
    if (d < 1.f) {
        b.x = tx;
        b.y = ty;
        b.vx = b.vy = 0.f;
        return;
    }
    float v = std::sqrt(2.f * kDecel * d);
    b.vx = dx / d * v;
    b.vy = dy / d * v;
    if (sys_) sys_->apu.noiseBurst(0.18f, 500.f, 0.05f);
}

void Game::throwNext() {
    if (n_ >= 5) return;
    Ball& b = balls_[n_++];
    b.live = true;
    b.vx = b.vy = 0.f;
    if (phase_ == Phase::Mark) {
        b.side = kJack;
        b.x = 160.f;
        b.y = 186.f;
        if (bot_) launch(b, 160.f, 64.f);
    } else if (phase_ == Phase::You) {
        b.side = kYou;
        b.x = 200.f;
        b.y = 186.f;
        if (bot_) launch(b, 174.f, 78.f);
    } else {
        b.side = kThem;
        b.x = 118.f;
        b.y = 186.f;
        if (bot_) launch(b, 140.f, 112.f);
    }
    if (!bot_) {
        float power = 0.28f + meter_ * 0.72f;
        float reach = (phase_ == Phase::Mark ? 150.f : 130.f) * power;
        float ang = std::clamp(aim_, -0.85f, 0.85f);
        launch(b, b.x + std::sin(ang) * reach, b.y - std::cos(ang) * reach);
    }
    mode_ = Mode::Roll;
}

void Game::stepBalls(float dt) {
    for (int i = 0; i < n_; i++) {
        Ball& b = balls_[i];
        if (!b.live) continue;
        float sp = len(b.vx, b.vy);
        if (sp < 1.f) {
            b.vx = b.vy = 0.f;
            continue;
        }
        float stopIn = sp / kDecel;
        float use = dt;
        bool halt = false;
        if (use >= stopIn) {
            use = stopIn;
            halt = true;
        }
        float dist = sp * use - 0.5f * kDecel * use * use;
        b.x += b.vx / sp * dist;
        b.y += b.vy / sp * dist;
        if (halt) b.vx = b.vy = 0.f;
        else {
            float ns = sp - kDecel * use;
            b.vx = b.vx / sp * ns;
            b.vy = b.vy / sp * ns;
        }
        float r = radius(b.side);
        if (b.x < kLeft + r) {
            b.x = kLeft + r;
            b.vx = std::fabs(b.vx) * 0.55f;
        }
        if (b.x > kRight - r) {
            b.x = kRight - r;
            b.vx = -std::fabs(b.vx) * 0.55f;
        }
        if (b.y < kTop + r) {
            b.live = false;
            b.vx = b.vy = 0.f;
        }
        if (b.y > kBot - r) {
            b.y = kBot - r;
            b.vy = -std::fabs(b.vy) * 0.45f;
        }
    }
}

void Game::separate() {
    for (int i = 0; i < n_; i++) {
        if (!balls_[i].live) continue;
        for (int j = i + 1; j < n_; j++) {
            if (!balls_[j].live) continue;
            float dx = balls_[j].x - balls_[i].x;
            float dy = balls_[j].y - balls_[i].y;
            float rr = radius(balls_[i].side) + radius(balls_[j].side);
            float d = len(dx, dy);
            if (d >= rr || d < 0.01f) continue;
            float nx = dx / d;
            float ny = dy / d;
            float push = (rr - d) * 0.5f + 0.05f;
            balls_[i].x -= nx * push;
            balls_[i].y -= ny * push;
            balls_[j].x += nx * push;
            balls_[j].y += ny * push;
            float rel = (balls_[j].vx - balls_[i].vx) * nx + (balls_[j].vy - balls_[i].vy) * ny;
            if (rel < 0.f) {
                balls_[i].vx += nx * rel;
                balls_[i].vy += ny * rel;
                balls_[j].vx -= nx * rel;
                balls_[j].vy -= ny * rel;
            }
            if (sys_) sys_->apu.tone(1, 180.f, 0.04f);
        }
    }
}

bool Game::settled() const {
    for (int i = 0; i < n_; i++) {
        if (!balls_[i].live) continue;
        if (len(balls_[i].vx, balls_[i].vy) > 1.f) return false;
    }
    return true;
}

void Game::judge() {
    const Ball* jack = nullptr;
    for (int i = 0; i < n_; i++)
        if (balls_[i].live && balls_[i].side == kJack) jack = &balls_[i];
    float bestYou = 1e9f, bestThem = 1e9f;
    if (jack) {
        for (int i = 0; i < n_; i++) {
            if (!balls_[i].live || balls_[i].side == kJack) continue;
            float d = len(balls_[i].x - jack->x, balls_[i].y - jack->y);
            if (balls_[i].side == kYou) bestYou = std::min(bestYou, d);
            else bestThem = std::min(bestThem, d);
        }
    }
    hold_ = 0;
    if (jack && bestYou + 0.5f < bestThem) {
        finished_ = true;
        won_ = true;
        mode_ = Mode::Win;
        if (sys_) {
            sys_->apu.tone(0, 523.f, 0.12f);
            if (!sys_->headless) sys_->rumble(0.35f, 0.55f, 140);
        }
    } else {
        won_ = false;
        finished_ = false;
        mode_ = Mode::Lose;
        if (sys_) sys_->apu.tone(0, 196.f, 0.08f);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (bot_ || pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) {
            phase_ = Phase::Mark;
            n_ = 0;
            aim_ = 0.f;
            meter_ = 0.4f;
            mode_ = Mode::Aim;
            hold_ = 0;
        }
    } else if (mode_ == Mode::Aim) {
        if (bot_) {
            throwNext();
        } else {
            if (pad.down(gs::BTN_LEFT)) aim_ -= 1.4f * kDt;
            if (pad.down(gs::BTN_RIGHT)) aim_ += 1.4f * kDt;
            aim_ = std::clamp(aim_, -0.85f, 0.85f);
            meter_ += meterDir_ * kDt * 0.85f;
            if (meter_ > 1.f) {
                meter_ = 1.f;
                meterDir_ = -1.f;
            }
            if (meter_ < 0.f) {
                meter_ = 0.f;
                meterDir_ = 1.f;
            }
            if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B)) throwNext();
        }
    } else if (mode_ == Mode::Roll) {
        stepBalls(kDt);
        separate();
        if (settled()) {
            hold_++;
            if (hold_ > (bot_ ? 8 : 24)) {
                hold_ = 0;
                if (phase_ == Phase::Mark) {
                    phase_ = Phase::You;
                    mode_ = Mode::Aim;
                } else if (phase_ == Phase::You) {
                    phase_ = Phase::Them;
                    mode_ = Mode::Aim;
                } else {
                    judge();
                }
            }
        } else {
            hold_ = 0;
        }
    } else if (mode_ == Mode::Win) {
        hold_++;
        if (hold_ == 18) sys.apu.tone(0, 659.f, 0.1f);
        if (hold_ == 36) sys.apu.tone(0, 784.f, 0.12f);
        if (hold_ > 70) {
            over_ = true;
            if (!sys.headless) sys.quit();
        }
    } else if (mode_ == Mode::Lose) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) begin();
    }
    draw();
}

void Game::spr(const gs::Image& img, float cx, float cy, float w, float h, int pal) {
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = img;
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!sys_ || !s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        int tile = art_.font[c - 32];
        if (!tile) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(tile, pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = s ? int(std::strlen(s)) : 0;
    hud(20 - n / 2, row, s, pal);
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        v.lineBackdrop[y] = y < 36 ? gs::rgb4(2, 4, 8) : gs::rgb4(2, 5, 2);
    }
    if (mode_ == Mode::Title) spr(art_.title, 160.f, 18.f, float(art_.title.w), float(art_.title.h), PAL_TITLE);
    if (mode_ == Mode::Win) spr(art_.win, 160.f, 18.f, float(art_.win.w), float(art_.win.h), PAL_WIN);

    for (int i = n_ - 1; i >= 0; i--) {
        if (!balls_[i].live && balls_[i].y < kTop) continue;
        int pal = balls_[i].side == kYou ? PAL_YOU : balls_[i].side == kThem ? PAL_THEM : PAL_MARK;
        const gs::Image& img = balls_[i].side == kJack ? art_.mark : art_.ball;
        spr(img, balls_[i].x, balls_[i].y, float(img.w), float(img.h), pal);
    }
    if (mode_ == Mode::Aim && !bot_) {
        float sx = phase_ == Phase::Mark ? 160.f : phase_ == Phase::You ? 200.f : 118.f;
        float sy = 186.f;
        float reach = 36.f + meter_ * 28.f;
        float gx = sx + std::sin(aim_) * reach;
        float gy = sy - std::cos(aim_) * reach;
        spr(art_.mark, gx, gy, 6.f, 6.f, PAL_AIM);
    }
    spr(art_.court, 160.f, 124.f, float(art_.court.w), float(art_.court.h), PAL_COURT);

    hud(1, 1, "S3 BOCCEMARK", PAL_INK);
    if (mode_ == Mode::Title) hudC(26, "START  FINISH THE MARK", PAL_HINT);
    else if (mode_ == Mode::Aim && phase_ == Phase::Mark) hudC(26, "BOWL THE PALLINO", PAL_HINT);
    else if (mode_ == Mode::Aim && phase_ == Phase::You) hudC(26, "YOUR BALL  A THROWS", PAL_HINT);
    else if (mode_ == Mode::Aim) hudC(26, "THEIR BALL", PAL_GOLD);
    else if (mode_ == Mode::Win) hudC(26, "THE MARK IS FINISHED", PAL_WIN);
    else if (mode_ == Mode::Lose) hudC(26, "SHORT  START TO BOWL AGAIN", PAL_GOLD);
    else hudC(26, "LET THEM REST", PAL_HINT);
}

}  // namespace boccemark

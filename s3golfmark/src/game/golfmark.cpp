#include "game/golfmark.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace golfmark {
namespace {
constexpr float kDt = 1.f / 60.f;
constexpr float kG = 520.f;
constexpr float kPi = 3.14159265f;
}

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
    ball_ = {};
    ball_.x = kTeeX;
    ball_.y = groundY(kTeeX) - kBallR;
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    finished_ = false;
    charging_ = false;
    planned_ = false;
    strokes_ = 0;
    hold_ = 0;
    aim_ = 34.f;
    meter_ = 0.2f;
    meterDir_ = 1.f;
    t_ = 0.f;
}

bool Game::stepBall(Ball& b, float dt) const {
    if (!b.live || b.holed) return b.holed;
    b.vy += kG * dt;
    float nx = b.x + b.vx * dt;
    float ny = b.y + b.vy * dt;
    if (nx < 8.f) {
        nx = 8.f;
        b.vx = std::fabs(b.vx) * 0.35f;
    }
    if (nx > 312.f) {
        nx = 312.f;
        b.vx = -std::fabs(b.vx) * 0.35f;
    }
    bool mouth = std::fabs(nx - kCupX) <= kCupHalf - 1.2f;
    float floor = groundY(nx);
    if (!mouth && b.y < kGround && ny >= floor - kBallR && std::fabs(b.x - kCupX) <= kCupHalf + 2.f &&
        b.vy > 0.f) {
        mouth = true;
        floor = kGround + kCupDepth;
        nx = std::clamp(nx, kCupX - (kCupHalf - 2.f), kCupX + (kCupHalf - 2.f));
    }
    if (ny >= floor - kBallR) {
        ny = floor - kBallR;
        if (mouth && ny > kGround - 1.f) {
            b.vx *= 0.72f;
            b.vy = b.vy > 30.f ? -b.vy * 0.12f : 0.f;
            if (std::fabs(b.vx) < 40.f) {
                b.holed = true;
                b.live = false;
                b.vx = 0;
                b.vy = 0;
                b.x = kCupX;
                b.y = kGround + kCupDepth - kBallR;
                return true;
            }
        } else {
            if (b.vy > 50.f) b.vy = -b.vy * 0.22f;
            else b.vy = 0.f;
            float d = kCupX - nx;
            if (!mouth && nx > 190.f && nx < 302.f && std::fabs(d) > kCupHalf)
                b.vx += (d > 0.f ? 1.f : -1.f) * 28.f * dt * 60.f * 0.35f;
            b.vx *= 0.975f;
            if (std::fabs(b.vx) < 10.f && b.vy == 0.f) {
                b.live = false;
                b.vx = 0.f;
            }
        }
    }
    b.x = nx;
    b.y = ny;
    return b.holed;
}

void Game::launch(float angDeg, float spd) {
    float a = angDeg * (kPi / 180.f);
    ball_.vx = std::cos(a) * spd;
    ball_.vy = -std::sin(a) * spd;
    ball_.live = true;
    ball_.holed = false;
    shotSpd_ = spd;
    strokes_++;
    mode_ = Mode::Fly;
    if (sys_) sys_->apu.noiseBurst(0.12f, 900.f, 0.05f);
}

bool Game::flyUntil(Ball b, float ang, float spd, Ball& out) const {
    float a = ang * (kPi / 180.f);
    b.vx = std::cos(a) * spd;
    b.vy = -std::sin(a) * spd;
    b.live = true;
    b.holed = false;
    for (int i = 0; i < 60 * 6; i++) {
        if (stepBall(b, kDt)) {
            out = b;
            return true;
        }
        if (!b.live) {
            out = b;
            return false;
        }
    }
    out = b;
    return false;
}

void Game::pickShot() {
    Ball from = ball_;
    from.y = groundY(from.x) - kBallR;
    Ball best = from;
    float bestD = 1e9f;
    bool found = false;
    for (int ai = 18; ai <= 62; ai += 2) {
        for (int si = 160; si <= 460; si += 10) {
            Ball out;
            bool hole = flyUntil(from, float(ai), float(si), out);
            if (hole) {
                botAng_ = float(ai);
                botSpd_ = float(si);
                planned_ = true;
                return;
            }
            float d = std::fabs(out.x - kCupX);
            if (d < bestD) {
                bestD = d;
                best = out;
                botAng_ = float(ai);
                botSpd_ = float(si);
                found = true;
            }
        }
    }
    planned_ = found;
    (void)best;
}

void Game::settle() {
    if (ball_.holed) {
        finishMark();
        return;
    }
    if (ball_.x < 16.f || ball_.x > 308.f || ball_.y > 210.f) {
        ball_.x = kTeeX;
        ball_.y = groundY(kTeeX) - kBallR;
    } else {
        ball_.y = groundY(ball_.x) - kBallR;
    }
    ball_.vx = ball_.vy = 0;
    ball_.live = false;
    planned_ = false;
    charging_ = false;
    mode_ = Mode::Aim;
}

void Game::finishMark() {
    finished_ = true;
    won_ = true;
    mode_ = Mode::Win;
    hold_ = 0;
    if (sys_) {
        sys_->apu.tone(0, 523.f, 0.12f);
        if (!sys_->headless) sys_->rumble(0.25f, 0.55f, 140);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (bot_ || pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) {
            ball_.x = kTeeX;
            ball_.y = groundY(kTeeX) - kBallR;
            mode_ = Mode::Aim;
        }
    } else if (mode_ == Mode::Aim) {
        if (bot_) {
            if (!planned_) pickShot();
            aim_ = botAng_;
            launch(botAng_, botSpd_);
        } else {
            if (pad.down(gs::BTN_LEFT)) aim_ -= 40.f * kDt;
            if (pad.down(gs::BTN_RIGHT)) aim_ += 40.f * kDt;
            aim_ = std::clamp(aim_, 12.f, 72.f);
            if (pad.pressed(gs::BTN_A)) charging_ = true;
            if (charging_) {
                meter_ += meterDir_ * 1.4f * kDt;
                if (meter_ >= 1.f) {
                    meter_ = 1.f;
                    meterDir_ = -1.f;
                }
                if (meter_ <= 0.05f) {
                    meter_ = 0.05f;
                    meterDir_ = 1.f;
                }
                if (pad.pressed(gs::BTN_B) || (!pad.down(gs::BTN_A) && meter_ > 0.08f && pad.prev[gs::BTN_A])) {
                    charging_ = false;
                    launch(aim_, 140.f + meter_ * 320.f);
                }
            }
        }
    } else if (mode_ == Mode::Fly) {
        for (int s = 0; s < 2; s++) {
            if (stepBall(ball_, kDt)) break;
            if (!ball_.live) break;
        }
        if (ball_.holed || !ball_.live) settle();
    } else if (mode_ == Mode::Win) {
        hold_++;
        if (hold_ == 20 && sys_) sys_->apu.tone(0, 659.f, 0.1f);
        if (hold_ > 70) {
            over_ = true;
            if (!sys.headless) sys.quit();
        }
    }
    if (pad.pressed(gs::BTN_START) && mode_ == Mode::Aim) mode_ = Mode::Title;
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

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        uint16_t c = gs::rgb4(6, 10, 15);
        if (y > 90) c = gs::rgb4(8, 12, 15);
        if (y > 130) c = gs::rgb4(10, 13, 14);
        if (y > 168) c = gs::rgb4(2, 7, 3);
        v.lineBackdrop[y] = c;
    }
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    spr(art_.course, 160.f, 185.f, float(art_.course.w), float(art_.course.h), PAL_COURSE);
    float flagY = groundY(kCupX + kCupHalf + 3.f);
    spr(art_.flag, kCupX + kCupHalf + 4.f, flagY - 14.f, float(art_.flag.w), float(art_.flag.h), PAL_FLAG);
    spr(art_.golfer, kTeeX - 14.f, groundY(kTeeX) - 14.f, float(art_.golfer.w), float(art_.golfer.h), PAL_GOLFER);

    if (mode_ == Mode::Aim || mode_ == Mode::Title) {
        float a = aim_ * (kPi / 180.f);
        float len = 18.f + (mode_ == Mode::Aim && charging_ ? meter_ * 36.f : 10.f);
        for (int i = 1; i <= 6; i++) {
            float u = float(i) / 6.f;
            spr(art_.dot, ball_.x + std::cos(a) * len * u, ball_.y - std::sin(a) * len * u, 3.f, 3.f, PAL_AIM);
        }
    }
    spr(art_.ball, ball_.x, ball_.y, float(art_.ball.w), float(art_.ball.h), PAL_BALL);

    spr(art_.card, 278.f, 28.f, float(art_.card.w), float(art_.card.h), PAL_CARD);
    if (finished_) spr(art_.mark, 292.f, 30.f, float(art_.mark.w), float(art_.mark.h), PAL_MARK);

    if (mode_ == Mode::Title) spr(art_.title, 150.f, 48.f, float(art_.title.w), float(art_.title.h), PAL_TITLE);
    if (mode_ == Mode::Win) spr(art_.win, 150.f, 78.f, float(art_.win.w), float(art_.win.h), PAL_WIN);

    hud(1, 1, "S3 GOLFMARK", PAL_INK);
    char buf[32];
    std::snprintf(buf, sizeof buf, "HOLE 1  STROKES %d", strokes_);
    hud(1, 2, buf, PAL_GOLD);
    if (mode_ == Mode::Title) hudC(24, "START  TEE OFF", PAL_GREEN);
    else if (mode_ == Mode::Aim) hudC(24, charging_ ? "B  STRIKE" : "A  AIM THE MARK", PAL_GREEN);
    else if (mode_ == Mode::Win) hudC(22, "CARD CLOSED", PAL_WIN);
    else hudC(24, "IN THE AIR", PAL_INK);
    if (mode_ == Mode::Aim) {
        std::snprintf(buf, sizeof buf, "LOFT %d", int(std::lround(aim_)));
        hud(1, 24, buf, PAL_AIM);
    }
}

}  // namespace golfmark

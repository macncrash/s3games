#include "game/golfbell.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace golfbell {
namespace {
constexpr float kDt = 1.f / 60.f;
constexpr float kG = 520.f;
constexpr float kPi = 3.14159265f;
}

const char* Game::phase() const {
    switch (mode_) {
    case Mode::Title: return "title";
    case Mode::Aim: return "aim";
    case Mode::Fly: return "fly";
    case Mode::Dead: return "dead";
    case Mode::Ring: return "ring";
    case Mode::Leave: return "leave";
    case Mode::Over: return "over";
    }
    return "?";
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.HUD.enabled = true;
    sys.apu.setMaster(0.4f);
    begin();
    audit();
}

void Game::begin() {
    dead_ = 0;
    tryNo_ = 0;
    won_ = false;
    over_ = false;
    rung_ = false;
    planned_ = false;
    charging_ = false;
    why_ = "";
    hold_ = 0;
    aim_ = 36.f;
    meter_ = 0.2f;
    meterDir_ = 1.f;
    bellAmp_ = 0.15f;
    resetTee();
    mode_ = Mode::Title;
}

void Game::resetTee() {
    ball_ = {};
    ball_.x = kTeeX;
    ball_.y = groundY(kTeeX) - kBallR;
    planned_ = false;
    charging_ = false;
}

void Game::audit() {
    Ball from;
    from.x = kTeeX;
    from.y = groundY(kTeeX) - kBallR;
    Ball weak;
    bool weakHole = flyUntil(from, 20.f, 80.f, weak);
    pickShot();
    Ball holed;
    bool in = planned_ && flyUntil(from, botAng_, botSpd_, holed) && holed.holed;
    bool notOnLip = in && std::fabs(holed.x - kCupX) < 1.f && holed.y > kGround;
    rules_ = in && notOnLip && !weakHole;
    planned_ = false;
    if (!rules_) why_ = "rules";
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
    if (!mouth && b.y < kGround && ny >= floor - kBallR && std::fabs(b.x - kCupX) <= kCupHalf + 2.f && b.vy > 0.f) {
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
    ball_.y = groundY(ball_.x) - kBallR;
    tryNo_ = dead_ + 1;
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
    Ball from;
    from.x = kTeeX;
    from.y = groundY(kTeeX) - kBallR;
    float bestD = 1e9f;
    bool found = false;
    for (int ai = 18; ai <= 62; ai += 2) {
        for (int si = 160; si <= 460; si += 10) {
            Ball out;
            if (flyUntil(from, float(ai), float(si), out)) {
                botAng_ = float(ai);
                botSpd_ = float(si);
                planned_ = true;
                return;
            }
            float d = std::fabs(out.x - kCupX);
            if (d < bestD) {
                bestD = d;
                botAng_ = float(ai);
                botSpd_ = float(si);
                found = true;
            }
        }
    }
    planned_ = found && bestD < 6.f;
}

void Game::ring() {
    if (rung_) return;
    rung_ = true;
    won_ = true;
    why_ = "BELL";
    bellAmp_ = 1.f;
    hold_ = 0;
    mode_ = Mode::Ring;
    if (!sys_) return;
    sys_->apu.tone(0, 740.f, 0.1f);
    sys_->apu.tone(1, 1110.f, 0.07f);
    if (!sys_->headless) sys_->rumble(0.35f, 0.7f, 160);
}

void Game::dieTry(const char* why) {
    if (rung_) return;
    dead_++;
    why_ = why;
    if (sys_) sys_->apu.tone(0, 110.f, 0.08f);
    if (dead_ >= 3) {
        mode_ = Mode::Over;
        won_ = false;
        over_ = true;
        if (sys_) sys_->apu.tone(1, 64.f, 0.06f);
        return;
    }
    hold_ = 0;
    mode_ = Mode::Dead;
}

void Game::finishFly() {
    if (ball_.holed) {
        ring();
        return;
    }
    const char* why = "MISS";
    if (ball_.x < kCupX - 28.f) why = "SHORT";
    else if (ball_.x > kCupX + 18.f) why = "HOT";
    else why = "LIP";
    dieTry(why);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    clock_ += kDt;
    bellPh_ += kDt * (rung_ ? 14.f : 2.1f);
    if (rung_) bellAmp_ = std::max(0.18f, bellAmp_ - kDt * 0.4f);

    const gs::Pad& pad = sys.pad;
    bool start = pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A);
    if (bot_) start = mode_ == Mode::Title && clock_ > 0.35f;

    if (mode_ == Mode::Title) {
        if (start) {
            resetTee();
            mode_ = Mode::Aim;
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        }
    } else if (mode_ == Mode::Aim) {
        if (bot_) {
            if (!planned_) pickShot();
            aim_ = botAng_;
            if (rules_) launch(botAng_, botSpd_);
            else dieTry("rules");
        } else {
            if (pad.down(gs::BTN_LEFT)) aim_ -= 42.f * kDt;
            if (pad.down(gs::BTN_RIGHT)) aim_ += 42.f * kDt;
            aim_ = std::clamp(aim_, 12.f, 72.f);
            if (pad.pressed(gs::BTN_A)) charging_ = true;
            if (charging_) {
                meter_ += meterDir_ * 1.35f * kDt;
                if (meter_ >= 1.f) {
                    meter_ = 1.f;
                    meterDir_ = -1.f;
                }
                if (meter_ <= 0.05f) {
                    meter_ = 0.05f;
                    meterDir_ = 1.f;
                }
                if (pad.pressed(gs::BTN_B) || (!pad.down(gs::BTN_A) && meter_ > 0.08f)) {
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
        if (ball_.holed || !ball_.live) finishFly();
    } else if (mode_ == Mode::Dead) {
        hold_++;
        if (hold_ > 36) {
            resetTee();
            mode_ = Mode::Aim;
        }
    } else if (mode_ == Mode::Ring) {
        hold_++;
        if (hold_ == 16 && sys_) sys_->apu.tone(0, 880.f, 0.09f);
        if (hold_ > 48) {
            mode_ = Mode::Leave;
            hold_ = 0;
        }
    } else if (mode_ == Mode::Leave) {
        hold_++;
        if (hold_ > 36) {
            over_ = true;
            mode_ = Mode::Over;
            if (!sys.headless) sys.quit();
        }
    } else if (mode_ == Mode::Over) {
        if (!bot_ && start) begin();
    }
    draw();
}

void Game::spr(const gs::Image& img, float cx, float cy, float w, float h, int pal) {
    if (!sys_ || img.w == 0) return;
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
        uint16_t c = gs::rgb4(5, 9, 14);
        if (y > 70) c = gs::rgb4(7, 11, 15);
        if (y > 120) c = gs::rgb4(9, 12, 13);
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

    spr(art_.course, 160.f, 184.f, float(art_.course.w), float(art_.course.h), PAL_COURSE);

    float pinX = kCupX + kCupHalf + 3.f;
    float pinBase = groundY(pinX);
    spr(art_.flag, pinX, pinBase - 16.f, float(art_.flag.w), float(art_.flag.h), PAL_FLAG);

    float swing = std::sin(bellPh_) * (rung_ ? 10.f * bellAmp_ : 1.4f);
    float bellCx = pinX + 1.f + swing;
    float bellCy = pinBase - 30.f;
    if (rung_ && bellAmp_ > 0.35f) {
        for (int i = 0; i < 5; i++) {
            float a = bellPh_ * 1.3f + float(i) * 1.256f;
            float rad = 12.f + (1.f - bellAmp_) * 14.f;
            spr(art_.pip, bellCx + std::cos(a) * rad, bellCy + std::sin(a) * rad * 0.4f, 4.f, 4.f, PAL_GOLD);
        }
    }
    spr(art_.clapper, bellCx + swing * 0.2f, bellCy + 5.f, 5.f, 8.f, PAL_BELL);
    spr(art_.bell, bellCx, bellCy, float(art_.bell.w), float(art_.bell.h), PAL_BELL);

    float gx = mode_ == Mode::Fly ? std::min(ball_.x - 10.f, kTeeX - 6.f) : kTeeX - 12.f;
    spr(art_.golfer, gx, groundY(kTeeX) - 16.f, float(art_.golfer.w), float(art_.golfer.h), PAL_GOLFER);

    if (mode_ == Mode::Aim || mode_ == Mode::Title) {
        float a = aim_ * (kPi / 180.f);
        float len = 16.f + (mode_ == Mode::Aim && charging_ ? meter_ * 40.f : 8.f);
        for (int i = 1; i <= 6; i++) {
            float u = float(i) / 6.f;
            spr(art_.dot, ball_.x + std::cos(a) * len * u, ball_.y - std::sin(a) * len * u, 3.f, 3.f, PAL_AIM);
        }
    }
    spr(art_.ball, ball_.x, ball_.y, float(art_.ball.w), float(art_.ball.h), PAL_BALL);

    if (mode_ == Mode::Title) spr(art_.title, 150.f, 42.f, float(art_.title.w), float(art_.title.h), PAL_TITLE);
    if (mode_ == Mode::Ring || mode_ == Mode::Leave || (mode_ == Mode::Over && won_))
        spr(art_.leave, 150.f, 64.f, float(art_.leave.w), float(art_.leave.h), PAL_LEAVE);

    hud(1, 1, "S3 GOLFBELL", PAL_INK);
    char buf[40];
    int shownTry = tryNo_ > 0 ? tryNo_ : 1;
    std::snprintf(buf, sizeof buf, "TRY %d   DEAD %d", shownTry, dead_);
    hud(1, 2, buf, PAL_GOLD);
    hud(28, 2, rung_ ? "BELL" : "QUIET", rung_ ? PAL_LEAVE : PAL_INK);

    if (mode_ == Mode::Title) hudC(24, "START  TEE OFF  BEFORE THE BELL DIES", PAL_GREEN);
    else if (mode_ == Mode::Aim) hudC(24, charging_ ? "RELEASE  STRIKE" : "LEFT RIGHT  HOLD A", PAL_GREEN);
    else if (mode_ == Mode::Fly) hudC(24, "IN THE AIR", PAL_INK);
    else if (mode_ == Mode::Dead) {
        std::snprintf(buf, sizeof buf, "%s  TRY DIED", why_);
        hudC(24, buf, PAL_ALERT);
    } else if (mode_ == Mode::Ring || mode_ == Mode::Leave) hudC(22, "LEAVE  THE BELL RANG", PAL_LEAVE);
    else if (mode_ == Mode::Over && !won_) hudC(24, "THIRD TRY DIED  BELL SILENT", PAL_ALERT);

    if (mode_ == Mode::Aim) {
        std::snprintf(buf, sizeof buf, "LOFT %d", int(std::lround(aim_)));
        hud(1, 24, buf, PAL_AIM);
    }
}

}  // namespace golfbell

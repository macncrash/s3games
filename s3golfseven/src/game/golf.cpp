#include "game/golf.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace golfseven {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kG = 520.f;
constexpr float kAng = 0.66f;
constexpr int kHoles = 7;

int rgbClamp(int v) { return std::max(0, std::min(15, v)); }

}  // namespace

const Game::Hole& Game::hole() const {
    static const Hole kCard[kHoles] = {
        {214.f, -8.f, 96.f, 122.f, 140.f, 164.f},  {236.f, 12.f, 108.f, 132.f, 150.f, 172.f},
        {198.f, 0.f, 90.f, 112.f, 128.f, 150.f},   {248.f, -16.f, 110.f, 138.f, 156.f, 180.f},
        {222.f, 6.f, 100.f, 124.f, 142.f, 166.f},  {230.f, -4.f, 104.f, 128.f, 146.f, 168.f},
        {206.f, 18.f, 94.f, 118.f, 136.f, 158.f},
    };
    return kCard[played_ % kHoles];
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    you_ = them_ = played_ = 0;
    over_ = won_ = rules_ = false;
    mode_ = Mode::Title;
    if (bot_) begin();
}

void Game::begin() {
    you_ = them_ = played_ = 0;
    over_ = won_ = rules_ = false;
    yours_ = true;
    nextTee();
}

void Game::nextTee() {
    const Hole& h = hole();
    ball_ = Ball{};
    ball_.x = kTee;
    ball_.y = kBallR;
    ball_.rest = true;
    win_ = windowFor(h);
    meter_ = 0.08f;
    meterDir_ = 1.f;
    power_ = win_.ok ? (win_.lo + win_.hi) * 0.5f : 0.55f;
    swingT_ = 0;
    flightT_ = 0;
    mode_ = Mode::Aim;
}

bool Game::stepBall(float dt) {
    const Hole& h = hole();
    if (ball_.rest || ball_.holed) return true;
    const float sub = dt / 4.f;
    for (int i = 0; i < 4; i++) {
        if (!ball_.rolling) {
            ball_.vy -= kG * sub;
            ball_.x += ball_.vx * sub;
            ball_.y += ball_.vy * sub;
            if (ball_.x < 2.f || ball_.x > 330.f || ball_.y > 280.f) {
                ball_.ob = true;
                ball_.rest = true;
                ball_.vx = ball_.vy = 0;
                return true;
            }
            if (ball_.y <= kBallR) {
                bool overCup = std::fabs(ball_.x - h.cup) <= 6.5f;
                if (overCup && ball_.vy < 0.f) {
                    ball_.holed = true;
                    ball_.rest = true;
                    ball_.x = h.cup;
                    ball_.y = 0;
                    ball_.vx = ball_.vy = 0;
                    return true;
                }
                bool water = ball_.x >= h.water0 && ball_.x <= h.water1;
                bool sand = ball_.x >= h.sand0 && ball_.x <= h.sand1;
                if (water) {
                    ball_.wet = true;
                    ball_.rest = true;
                    ball_.y = kBallR * 0.4f;
                    ball_.vx = ball_.vy = 0;
                    return true;
                }
                float bounce = sand ? 0.05f : 0.16f;
                ball_.y = kBallR;
                ball_.vy = -ball_.vy * bounce;
                ball_.vx *= sand ? 0.35f : 0.72f;
                if (std::fabs(ball_.vy) < 36.f) {
                    ball_.rolling = true;
                    ball_.vy = 0;
                    ball_.y = kBallR;
                }
            }
        } else {
            bool green = std::fabs(ball_.x - h.cup) < 42.f;
            bool sand = ball_.x >= h.sand0 && ball_.x <= h.sand1;
            float fr = sand ? 260.f : (green ? 78.f : 130.f);
            float s = ball_.vx > 0 ? 1.f : (ball_.vx < 0 ? -1.f : 0.f);
            ball_.vx -= s * fr * sub;
            if (s != 0 && (ball_.vx > 0) != (s > 0)) ball_.vx = 0;
            ball_.x += ball_.vx * sub;
            ball_.y = kBallR;
            if (ball_.x >= h.water0 && ball_.x <= h.water1) {
                ball_.wet = true;
                ball_.rest = true;
                ball_.vx = 0;
                return true;
            }
            if (std::fabs(ball_.x - h.cup) <= 6.2f && std::fabs(ball_.vx) < 150.f) {
                ball_.holed = true;
                ball_.rest = true;
                ball_.x = h.cup;
                ball_.y = 0;
                ball_.vx = 0;
                return true;
            }
            if (ball_.x < 2.f || ball_.x > 330.f) {
                ball_.ob = true;
                ball_.rest = true;
                ball_.vx = 0;
                return true;
            }
            if (std::fabs(ball_.vx) < 6.f) {
                if (++ball_.still > 6) {
                    ball_.rest = true;
                    ball_.vx = 0;
                    return true;
                }
            } else {
                ball_.still = 0;
            }
        }
    }
    return ball_.rest;
}

bool Game::predict(float power, const Hole& h) const {
    Game g;
    g.played_ = played_;
    g.ball_ = Ball{};
    g.ball_.x = kTee;
    g.ball_.y = kBallR;
    g.ball_.rest = false;
    float spd = 80.f + std::clamp(power, 0.f, 1.f) * 280.f;
    g.ball_.vx = std::cos(kAng) * spd + h.wind;
    g.ball_.vy = std::sin(kAng) * spd;
    for (int i = 0; i < 420 && !g.ball_.rest; i++) g.stepBall(kDt);
    return g.ball_.holed;
}

Game::Window Game::windowFor(const Hole& h) const {
    Window w;
    float first = -1.f, last = -1.f;
    for (int i = 0; i <= 48; i++) {
        float p = float(i) / 48.f;
        if (predict(p, h)) {
            if (first < 0) first = p;
            last = p;
        } else if (first >= 0) {
            break;
        }
    }
    if (first < 0) return w;
    w.ok = true;
    w.lo = first;
    w.hi = last;
    if (w.hi < w.lo) w.hi = w.lo;
    return w;
}

void Game::launch(float power) {
    const Hole& h = hole();
    power_ = std::clamp(power, 0.f, 1.f);
    float spd = 80.f + power_ * 280.f;
    ball_ = Ball{};
    ball_.x = kTee;
    ball_.y = kBallR;
    ball_.rest = false;
    ball_.vx = std::cos(kAng) * spd + h.wind;
    ball_.vy = std::sin(kAng) * spd;
    flightT_ = 0;
    swingT_ = 10;
    mode_ = Mode::Flight;
    swingTone();
}

void Game::swingTone() {
    if (!sys_) return;
    sys_->apu.noiseBurst(0.18f, yours_ ? 900.f : 500.f, 0.08f);
    sys_->apu.tone(0, yours_ ? 220.f : 160.f, 0.12f);
    toneT_ = 0.12f;
}

void Game::tickAudio(float dt) {
    if (!sys_) return;
    if (toneT_ > 0) {
        toneT_ -= dt;
        if (toneT_ <= 0) sys_->apu.tone(0, 0, 0);
    }
}

void Game::finishFlight() {
    made_ = ball_.holed;
    if (made_) {
        if (yours_) you_++;
        else them_++;
        if (sys_) {
            sys_->apu.tone(1, 660.f, 0.16f);
            sys_->apu.tone(2, 990.f, 0.1f);
            toneT_ = 0.28f;
            if (!bot_) sys_->rumble(0.25f, 0.55f, 80);
        }
    } else if (sys_) {
        sys_->apu.tone(1, 140.f, 0.1f);
        toneT_ = 0.16f;
    }
    callT_ = bot_ ? 0.12f : 0.55f;
    mode_ = Mode::Call;
}

void Game::afterCall() {
    if (sys_) {
        sys_->apu.tone(1, 0, 0);
        sys_->apu.tone(2, 0, 0);
    }
    if (you_ >= kRace && you_ > them_) {
        won_ = true;
        rules_ = true;
        over_ = true;
        mode_ = Mode::Win;
        return;
    }
    if (them_ >= kRace && them_ > you_) {
        won_ = false;
        rules_ = true;
        over_ = true;
        mode_ = Mode::Lose;
        return;
    }
    if (you_ >= kRace || them_ >= kRace) {
        // Six is not enough, and a tie at seven is not a finish. Play on.
    }
    played_++;
    yours_ = !yours_;
    nextTee();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const float dt = kDt;
    clock_ += dt;
    tickAudio(dt);
    gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = held_;
        draw();
        return;
    }
    if (!bot_ && mode_ != Mode::Title && mode_ != Mode::Win && mode_ != Mode::Lose && pad.pressed(gs::BTN_START)) {
        held_ = mode_;
        mode_ = Mode::Pause;
        draw();
        return;
    }

    if (mode_ == Mode::Title) {
        if (bot_ || pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) begin();
    } else if (mode_ == Mode::Aim) {
        bool scripted = bot_ || !yours_;
        if (!scripted) {
            if (pad.down(gs::BTN_RIGHT) || pad.axisX > 0.4f) meterDir_ = 1.f;
            meter_ += meterDir_ * dt * 0.55f;
            if (meter_ >= 1.f) {
                meter_ = 1.f;
                meterDir_ = -1.f;
            } else if (meter_ <= 0.f) {
                meter_ = 0.f;
                meterDir_ = 1.f;
            }
            if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) launch(meter_);
        } else {
            float aim = power_;
            if (!yours_) {
                uint32_t r = rng_ * 1664525u + 1013904223u;
                rng_ = r;
                float slip = ((r >> 8) & 255) / 255.f;
                float half = std::max(0.02f, (win_.hi - win_.lo) * 0.5f);
                float mid = (win_.lo + win_.hi) * 0.5f;
                // The house finds the cup about one try in five. The rest sail long or short.
                if ((r & 7u) == 0u) aim = mid;
                else aim = mid + (slip < 0.5f ? -1.f : 1.f) * (half + 0.07f + slip * 0.08f);
            }
            meter_ += (aim - meter_) * 0.35f;
            if (std::fabs(meter_ - aim) < 0.02f || flightT_++ > 18) launch(std::clamp(aim, 0.f, 1.f));
        }
    } else if (mode_ == Mode::Flight) {
        if (swingT_ > 0) swingT_--;
        flightT_++;
        bool done = stepBall(dt);
        if (done || flightT_ > 360) finishFlight();
    } else if (mode_ == Mode::Call) {
        callT_ -= dt;
        if (callT_ <= 0.f || (!bot_ && (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_START)))) afterCall();
    } else if (mode_ == Mode::Win || mode_ == Mode::Lose) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) begin();
    }

    draw();
}

void Game::spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool shadow) {
    if (!sys_ || img.w == 0 || w < 1.f || h < 1.f) return;
    gs::Sprite s;
    s.img = img;
    s.x = int16_t(std::lround(cx - w * 0.5f));
    s.y = int16_t(std::lround(cy - h * 0.5f));
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.pal = uint8_t(pal);
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!sys_ || !s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = (unsigned char)s[i];
        if (c >= 'a' && c <= 'z') c = (unsigned char)(c - 32);
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
        if (y < int(kGround) - 8) {
            float u = y / (kGround - 8.f);
            int r = rgbClamp(4 + int((1.f - u) * 6.f));
            int g = rgbClamp(8 + int((1.f - u) * 4.f));
            int b = rgbClamp(12 + int((1.f - u) * 3.f));
            if (mode_ == Mode::Win) g = rgbClamp(g + 2);
            if (mode_ == Mode::Lose) r = rgbClamp(r + 3);
            v.lineBackdrop[y] = gs::rgb4(r, g, b);
        } else {
            v.lineBackdrop[y] = gs::rgb4(3, 9, 3);
        }
    }
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();
    const Hole& h = hole();

    float drift = std::fmod(clock_ * 8.f, 360.f);
    spr(art_.cloud, 40.f + drift, 36.f, 36.f, 16.f, PAL_SKY);
    spr(art_.cloud, 200.f - drift * 0.4f, 52.f, 28.f, 12.f, PAL_SKY);

    float w0 = h.water0, w1 = h.water1;
    spr(art_.water, (w0 + w1) * 0.5f, kGround - 2.f, w1 - w0, 14.f, PAL_HAZ);
    float s0 = h.sand0, s1 = h.sand1;
    spr(art_.sand, (s0 + s1) * 0.5f, kGround - 2.f, s1 - s0, 14.f, PAL_HAZ);

    float greenL = h.cup - 40.f;
    float greenR = h.cup + 28.f;
    for (float x = greenL; x < greenR; x += 36.f)
        spr(art_.turf, x + 16.f, kGround - 1.f, 40.f, 16.f, PAL_TURF);
    spr(art_.cup, h.cup, kGround - 1.f, 16.f, 8.f, PAL_CUP);
    spr(art_.flag, h.cup + 2.f, kGround - 28.f, 24.f, 44.f, PAL_FLAG);

    int pose = (mode_ == Mode::Flight && swingT_ > 0) ? 1 : 0;
    float manX = yours_ ? kTee - 14.f : kTee - 14.f;
    spr(art_.man[pose], manX, kGround - 20.f, 26.f, 36.f, PAL_MAN);

    if (!ball_.holed) {
        float by = kGround - ball_.y;
        spr(art_.ball, ball_.x + 2.f, kGround - 2.f, 10.f, 4.f, PAL_BALL, true);
        float bh = ball_.rolling || ball_.rest ? 12.f : 10.f + std::min(8.f, ball_.y * 0.04f);
        spr(art_.ball, ball_.x, by, bh, bh, PAL_BALL);
    }

    char line[48];
    std::snprintf(line, sizeof line, "YOU %d", you_);
    hud(1, 1, line, PAL_GREEN);
    std::snprintf(line, sizeof line, "HOUSE %d", them_);
    hud(28, 1, line, PAL_ALERT);
    hudC(1, "TO 7", PAL_GOLD);

    if (mode_ == Mode::Title) {
        spr(art_.title, 160.f, 78.f, float(art_.title.w), float(art_.title.h), PAL_TITLE);
        hudC(16, "FIRST TO SEVEN CUPS", PAL_INK);
        hudC(18, "TIME THE PITCH", PAL_INK);
        hudC(22, "START", PAL_GOLD);
    } else if (mode_ == Mode::Pause) {
        hudC(12, "PAUSE", PAL_GOLD);
    } else if (mode_ == Mode::Win) {
        spr(art_.win, 160.f, 78.f, float(art_.win.w), float(art_.win.h), PAL_TITLE);
        hudC(16, "YOU ARE FIRST TO SEVEN", PAL_GREEN);
    } else if (mode_ == Mode::Lose) {
        spr(art_.lose, 160.f, 86.f, float(art_.lose.w), float(art_.lose.h), PAL_TITLE);
        hudC(16, "HOUSE IS FIRST TO SEVEN", PAL_ALERT);
    } else {
        int wind = int(std::lround(h.wind));
        std::snprintf(line, sizeof line, "WIND %+d", wind);
        hud(1, 3, line, PAL_INK);
        std::snprintf(line, sizeof line, "HOLE %d", (played_ % kHoles) + 1);
        hud(30, 3, line, PAL_INK);
        hudC(3, yours_ ? "YOUR PITCH" : "HOUSE PITCH", yours_ ? PAL_GREEN : PAL_ALERT);

        if (mode_ == Mode::Aim || mode_ == Mode::Flight) {
            char bar[34];
            int n = 28;
            int mark = std::clamp(int(meter_ * float(n - 1) + 0.5f), 0, n - 1);
            int a = win_.ok ? std::clamp(int(win_.lo * float(n - 1)), 0, n - 1) : 0;
            int b = win_.ok ? std::clamp(int(win_.hi * float(n - 1)), 0, n - 1) : 0;
            for (int i = 0; i < n; i++) {
                if (i == mark) bar[i] = 'O';
                else if (win_.ok && i >= a && i <= b) bar[i] = '#';
                else bar[i] = '-';
            }
            bar[n] = 0;
            hud(6, 24, bar, PAL_GOLD);
            hudC(26, mode_ == Mode::Aim && yours_ && !bot_ ? "A TO PITCH" : " ", PAL_INK);
        } else if (mode_ == Mode::Call) {
            const char* msg = made_ ? "IN THE CUP" : (ball_.wet ? "IN THE WATER" : (ball_.ob ? "OUT" : "LEFT SHORT"));
            hudC(12, msg, made_ ? PAL_GREEN : PAL_ALERT);
            if (you_ == 6 && yours_ && made_) hudC(14, "SIX DOES NOT FINISH", PAL_GOLD);
        }
    }
}

}  // namespace golfseven

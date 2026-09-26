#include "game/puttseven.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace puttseven {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float OX = 32.f;
constexpr float OY = 44.f;
constexpr float GW = 256.f;
constexpr float GH = 124.f;
constexpr float TEE_X = 128.f;
constexpr float TEE_Y = 102.f;
constexpr float CUP_X = 150.f;
constexpr float CUP_Y = 28.f;
constexpr float CUP_R = 13.f;
constexpr float LIP = 165.f;
constexpr float CORE_R = 8.f;
constexpr float CORE_LIP = 220.f;
constexpr float MU = 58.f;
constexpr float AX = -24.f;  // breaks left
constexpr float AY = 18.f;   // uphill toward the cup
constexpr float BALL_R = 4.2f;
constexpr float BOUNCE_E = 0.28f;
constexpr float STOP_V = 7.5f;
constexpr int ROLL_MAX = 280;
constexpr float PI = 3.14159265f;

struct Box {
    float x, y, w, h;
};

constexpr Box CORNER[4] = {
    {0, 0, 18, 12},
    {GW - 18, 0, 18, 12},
    {0, GH - 12, 18, 12},
    {GW - 18, GH - 12, 18, 12},
};

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

bool inSand(float x, float y) { return x >= 198.f && x <= 248.f && y >= 52.f && y <= 100.f; }

}  // namespace

void Game::bounce(Ball& b, float nx, float ny) const {
    float vn = b.vx * nx + b.vy * ny;
    if (vn < 0.f) {
        b.vx -= (1.f + BOUNCE_E) * vn * nx;
        b.vy -= (1.f + BOUNCE_E) * vn * ny;
        b.bumps++;
    }
}

void Game::collide(Ball& b, float x, float y, float w, float h) const {
    if (b.x >= x && b.x <= x + w && b.y >= y && b.y <= y + h) {
        float dl = b.x - x;
        float dr = x + w - b.x;
        float dt = b.y - y;
        float db = y + h - b.y;
        if (dl <= dr && dl <= dt && dl <= db) {
            b.x = x - BALL_R;
            bounce(b, -1.f, 0.f);
        } else if (dr <= dt && dr <= db) {
            b.x = x + w + BALL_R;
            bounce(b, 1.f, 0.f);
        } else if (dt <= db) {
            b.y = y - BALL_R;
            bounce(b, 0.f, -1.f);
        } else {
            b.y = y + h + BALL_R;
            bounce(b, 0.f, 1.f);
        }
        return;
    }
    float cx = clampf(b.x, x, x + w);
    float cy = clampf(b.y, y, y + h);
    float dx = b.x - cx;
    float dy = b.y - cy;
    float d2 = dx * dx + dy * dy;
    if (d2 >= BALL_R * BALL_R || d2 < 1e-8f) return;
    float d = std::sqrt(d2);
    float nx = dx / d;
    float ny = dy / d;
    b.x += nx * (BALL_R - d);
    b.y += ny * (BALL_R - d);
    bounce(b, nx, ny);
}

void Game::advance(Ball& b) const {
    if (b.rest) return;
    b.vx += AX * DT;
    b.vy += AY * DT;
    float sp = std::hypot(b.vx, b.vy);
    if (sp > 420.f) {
        float s = 420.f / sp;
        b.vx *= s;
        b.vy *= s;
        sp = 420.f;
    }
    float mu = inSand(b.x, b.y) ? MU * 3.2f : MU;
    float drop = mu * DT;
    if (sp <= drop) {
        b.vx = b.vy = 0.f;
    } else {
        float s = (sp - drop) / sp;
        b.vx *= s;
        b.vy *= s;
    }
    b.x += b.vx * DT;
    b.y += b.vy * DT;

    const Box edge[4] = {
        {-80.f, -80.f, 80.f, GH + 160.f},
        {GW, -80.f, 80.f, GH + 160.f},
        {-80.f, -80.f, GW + 160.f, 80.f},
        {-80.f, GH, GW + 160.f, 80.f},
    };
    for (int pass = 0; pass < 3; pass++) {
        for (const Box& e : edge) collide(b, e.x, e.y, e.w, e.h);
        for (const Box& c : CORNER) collide(b, c.x, c.y, c.w, c.h);
    }
    if (!std::isfinite(b.x) || !std::isfinite(b.y) || b.x < -20.f || b.y < -20.f || b.x > GW + 20.f ||
        b.y > GH + 20.f) {
        b.x = TEE_X;
        b.y = TEE_Y;
        b.vx = b.vy = 0.f;
        b.cup = 0;
        b.rest = true;
        return;
    }

    sp = std::hypot(b.vx, b.vy);
    float d = std::hypot(b.x - CUP_X, b.y - CUP_Y);
    if ((d <= CORE_R && sp <= CORE_LIP) || (d <= CUP_R && sp <= LIP) || (sp < STOP_V && d <= CUP_R + 1.5f)) {
        b.x = CUP_X;
        b.y = CUP_Y;
        b.vx = b.vy = 0.f;
        b.cup = 1;
        b.rest = true;
        return;
    }
    if (sp < STOP_V) {
        b.vx = b.vy = 0.f;
        b.rest = true;
    }
}

int Game::trial(float x, float y, float ang, float spd) const {
    Ball b;
    b.x = x;
    b.y = y;
    b.vx = std::cos(ang) * spd;
    b.vy = std::sin(ang) * spd;
    b.rest = false;
    for (int i = 0; i < ROLL_MAX; i++) {
        advance(b);
        if (b.rest) break;
    }
    if (!b.rest) {
        b.vx = b.vy = 0.f;
        b.rest = true;
    }
    return b.cup;
}

bool Game::solve() {
    const float base = std::atan2(CUP_Y - TEE_Y, CUP_X - TEE_X);
    float bestA = base;
    float bestV = 150.f;
    int bestRun = 0;
    auto scan = [&](float a0, float a1, float aStep, float v0, float v1, float vStep) {
        for (float a = a0; a <= a1 + 1e-4f; a += aStep) {
            int run = 0;
            int runBest = 0;
            float runV = v0;
            float mid = v0;
            for (float v = v0; v <= v1 + 1e-4f; v += vStep) {
                if (trial(TEE_X, TEE_Y, a, v) == 1) {
                    if (run == 0) runV = v;
                    run++;
                } else if (run) {
                    if (run > runBest) {
                        runBest = run;
                        mid = runV + 0.5f * float(run - 1) * vStep;
                    }
                    run = 0;
                }
            }
            if (run > runBest) {
                runBest = run;
                mid = runV + 0.5f * float(run - 1) * vStep;
            }
            if (runBest > bestRun) {
                bestRun = runBest;
                bestA = a;
                bestV = mid;
            }
        }
    };

    scan(base - 0.85f, base + 0.85f, 0.035f, 72.f, 228.f, 6.f);
    if (bestRun > 0) {
        float a0 = bestA;
        float v0 = bestV;
        int keep = bestRun;
        bestRun = 0;
        scan(a0 - 0.05f, a0 + 0.05f, 0.012f, std::max(64.f, v0 - 18.f), v0 + 18.f, 2.f);
        if (bestRun == 0) {
            bestA = a0;
            bestV = v0;
            bestRun = keep;
        }
    } else {
        scan(-PI, PI, 0.09f, 70.f, 230.f, 12.f);
    }
    if (trial(TEE_X, TEE_Y, bestA, bestV) != 1) {
        bool found = false;
        for (float dv = -30.f; dv <= 30.f && !found; dv += 2.f) {
            if (trial(TEE_X, TEE_Y, bestA, bestV + dv) == 1) {
                bestV += dv;
                found = true;
            }
        }
        if (!found) bestRun = 0;
    }
    aimShot_ = bestA;
    spdShot_ = bestV;
    aimMiss_ = bestA + 0.9f;
    spdMiss_ = std::max(80.f, bestV * 0.5f);
    if (trial(TEE_X, TEE_Y, aimMiss_, spdMiss_) == 1) {
        aimMiss_ = bestA + 1.7f;
        spdMiss_ = 88.f;
    }
    shotOk_ = bestRun > 0 && trial(TEE_X, TEE_Y, aimShot_, spdShot_) == 1;
    if (!shotOk_) std::fprintf(stderr, "s3puttseven: no putt\n");
    return shotOk_;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.setFogColor(gs::rgb4(2, 3, 2));
    sys.apu.setMaster(0.7f);
    humanAim_ = std::atan2(CUP_Y - TEE_Y, CUP_X - TEE_X);
    place();
    mode_ = Mode::Title;
    sys.setLight(40, 90, 48);
    if (bot_) begin();
}

void Game::place() {
    ball_ = Ball{};
    ball_.x = TEE_X;
    ball_.y = TEE_Y;
    ball_.rest = true;
    swinging_ = false;
    meter_ = 0.f;
    meterDir_ = 1.f;
    rollFrames_ = 0;
    cpuWait_ = 0.f;
}

void Game::begin() {
    you_ = 0;
    them_ = 0;
    strokes_ = 0;
    themPutts_ = 0;
    fate_ = 0;
    won_ = false;
    over_ = false;
    yours_ = true;
    bannerT_ = 0.f;
    holdT_ = 0.f;
    beep_ = 0.f;
    humanAim_ = std::atan2(CUP_Y - TEE_Y, CUP_X - TEE_X);
    solve();
    place();
    aim_ = humanAim_;
    mode_ = Mode::Play;
    if (sys_) sys_->setLight(48, 140, 64);
}

void Game::strike(float ang, float spd) {
    aim_ = ang;
    if (!yours_) themPutts_++;
    ball_.vx = std::cos(ang) * spd;
    ball_.vy = std::sin(ang) * spd;
    ball_.rest = false;
    ball_.cup = 0;
    ball_.bumps = 0;
    swinging_ = false;
    rollFrames_ = 0;
    strokes_++;
    blip(230.f);
    if (sys_) sys_->apu.noiseBurst(0.2f, 1500.f, 0.045f);
}

void Game::settle() {
    bool in = ball_.cup == 1;
    if (in) {
        if (yours_) you_++;
        else them_++;
    }
    if (you_ >= 7 && you_ > them_) {
        fate_ = 1;
        won_ = true;
    } else if (them_ >= 7 && them_ > you_) {
        fate_ = 2;
        won_ = false;
    } else {
        fate_ = 0;
    }
    mode_ = Mode::Hold;
    holdT_ = 0.f;
    if (!sys_) return;
    if (fate_ == 1) {
        beep_ = 0.95f;
        sys_->apu.tone(0, 523.f, 0.12f);
        sys_->apu.tone(1, 659.f, 0.10f);
        sys_->apu.tone(2, 784.f, 0.11f);
        sys_->rumble(0.4f, 0.75f, 180);
        sys_->setLight(255, 210, 48);
    } else if (fate_ == 2) {
        beep_ = 0.5f;
        sys_->apu.tone(0, 98.f, 0.12f);
        sys_->apu.tone(1, 73.f, 0.08f);
        sys_->setLight(160, 32, 28);
    } else if (in) {
        beep_ = 0.4f;
        sys_->apu.tone(0, 494.f, 0.1f);
        sys_->apu.tone(1, 622.f, 0.08f);
        sys_->rumble(0.25f, 0.45f, 90);
        sys_->setLight(yours_ ? 64 : 170, yours_ ? 150 : 50, yours_ ? 70 : 36);
    } else {
        beep_ = 0.22f;
        sys_->apu.tone(0, 110.f, 0.08f);
        sys_->setLight(90, 70, 30);
    }
}

void Game::blip(float freq) {
    if (!sys_) return;
    sys_->apu.tone(0, freq, 0.07f);
    beep_ = 0.1f;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool shadow) {
    if (!sys_ || h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::min(2000, std::max(1, int(std::lround(w)))));
    s.h = int16_t(std::min(2000, std::max(1, int(std::lround(h)))));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(float(s.h));
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::patch(const gs::Mipped& m, float x, float y, float w, float h, int pal) {
    const float ts = 16.f;
    for (float yy = 0.f; yy < h - 0.1f; yy += ts) {
        for (float xx = 0.f; xx < w - 0.1f; xx += ts) spr(m, x + xx + ts * 0.5f, y + yy + ts * 0.5f, ts, pal);
    }
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
        if (y < 36) {
            float u = y / 35.f;
            v.lineBackdrop[y] = gs::rgb4(5 + int(u * 7), 3 + int(u * 3), 10 - int(u * 4));
        } else if (y < 48) {
            v.lineBackdrop[y] = gs::rgb4(6, 4, 2);
        } else if (y < 168) {
            bool band = ((y / 3) & 1) == 0;
            v.lineBackdrop[y] = band ? gs::rgb4(2, 8, 3) : gs::rgb4(3, 11, 4);
        } else if (y < 184) {
            v.lineBackdrop[y] = gs::rgb4(6, 4, 2);
        } else {
            v.lineBackdrop[y] = gs::rgb4(2, 3, 2);
        }
    }
}

void Game::lamps() {
    for (int i = 0; i < 7; i++) {
        bool onY = i < you_;
        bool onT = i < them_;
        spr(art_.dot, 18.f + float(i) * 12.f, 22.f, onY ? 8.f : 4.5f, onY ? PAL_YOU : PAL_INK);
        spr(art_.dot, 230.f + float(i) * 12.f, 22.f, onT ? 8.f : 4.5f, onT ? PAL_THEM : PAL_INK);
    }
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    backdrop();

    const bool showAim = ball_.rest && ball_.cup == 0 &&
                          (mode_ == Mode::Title || mode_ == Mode::Play || mode_ == Mode::Pause);
    if (mode_ == Mode::Title) spr(art_.logo, 160.f, 16.f, float(art_.logo.h), PAL_LOGO);
    else lamps();
    if (mode_ == Mode::Result && won_) spr(art_.win, 160.f, 100.f, float(art_.win.h), PAL_LOGO);
    if (mode_ == Mode::Result && !won_) spr(art_.lose, 160.f, 100.f, float(art_.lose.h), PAL_ALARM);

    if (showAim) {
        float dx = std::cos(aim_);
        float dy = std::sin(aim_);
        int n = swinging_ ? 3 + int(meter_ * 6.f) : 6;
        for (int i = 1; i <= n; i++) {
            float d = 12.f + float(i) * 8.f;
            spr(art_.dot, OX + ball_.x + dx * d, OY + ball_.y + dy * d, 4.5f, PAL_AIM);
        }
    }

    float bx = OX + ball_.x;
    float by = OY + ball_.y;
    bool youUp = yours_;
    float gx = youUp ? bx - 18.f : 64.f;
    float gy = youUp ? by + 6.f : 198.f;
    float tx = youUp ? 256.f : bx + 18.f;
    float ty = youUp ? 198.f : by + 6.f;
    spr(art_.golfer, gx, gy, 36.f, PAL_YOU, false);
    spr(art_.golfer, tx, ty, 36.f, PAL_THEM, true);

    float ballH = ball_.cup ? 6.f : 11.f;
    spr(art_.ball, bx, by, ballH, PAL_BALL);
    spr(art_.shadow, bx + 3.f, by + 4.f, 7.f, PAL_BALL, false, true);

    const gs::Mipped& flag = art_.flag[(int(t_ * 6.f) & 1)];
    spr(flag, OX + CUP_X + 2.f, OY + CUP_Y - 18.f, 36.f, PAL_FLAG);
    float pulse = 26.f + std::sin(t_ * 3.f) * 0.8f;
    spr(art_.cup, OX + CUP_X, OY + CUP_Y, pulse, PAL_CUP);
    if (ball_.cup == 0) spr(art_.peg, OX + TEE_X, OY + TEE_Y + 5.f, 8.f, PAL_WOOD);

    const float tuft[6][2] = {{40, 36}, {36, 88}, {72, 34}, {214, 108}, {236, 40}, {48, 108}};
    for (int i = 0; i < 6; i++) {
        float x = tuft[i][0] + std::sin(t_ * 1.3f + float(i)) * 0.8f;
        spr(art_.tuft, OX + x, OY + tuft[i][1], 10.f, PAL_HEDGE, i & 1);
    }
    for (const Box& c : CORNER) spr(art_.hedge, OX + c.x + c.w * 0.5f, OY + c.y + c.h * 0.5f, c.h + 2.f, PAL_HEDGE);
    patch(art_.sand, OX + 198.f, OY + 52.f, 50.f, 48.f, PAL_SAND);

    const float trees[4][2] = {{28, 30}, {96, 26}, {214, 28}, {286, 30}};
    for (int i = 0; i < 4; i++) spr(art_.tree, trees[i][0], trees[i][1], 40.f, PAL_TREE, i & 1);
    spr(art_.sun, 300.f, 14.f, 16.f, PAL_SUN);

    patch(art_.wood, 16, 32, 288, 16, PAL_WOOD);
    patch(art_.wood, 16, 168, 288, 16, PAL_WOOD);
    patch(art_.wood, 16, 48, 16, 120, PAL_WOOD);
    patch(art_.wood, 288, 48, 16, 120, PAL_WOOD);

    char buf[64];
    if (mode_ == Mode::Title) {
        hudC(21, "FIRST TO SEVEN", PAL_LOGO);
        hudC(22, "HOLE IT BEFORE THEY DO", PAL_INK);
        hudC(23, "ONE CUP ON ONE GREEN", PAL_YOU);
        hud(40 - int(std::strlen(S3_VERSION_STRING)), 24, S3_VERSION_STRING, PAL_INK);
        if ((int(t_ * 2.f) & 1) == 0) hudC(25, "PRESS START", PAL_LOGO);
        hudC(26, "ARROWS AIM   HOLD Z PUTT", PAL_INK);
        hudC(27, "ESC QUITS", PAL_INK);
    } else if (mode_ == Mode::Result) {
        std::snprintf(buf, sizeof buf, "YOU %d   THEM %d", you_, them_);
        if (won_) {
            hudC(16, "FIRST TO SEVEN", PAL_LOGO);
            hudC(18, buf, PAL_YOU);
            hudC(20, "LEAVE WITH IT", PAL_YOU);
        } else {
            hudC(16, "THEY GOT TO SEVEN", PAL_ALARM);
            hudC(18, buf, PAL_THEM);
            hudC(20, "YOU ARE SHORT", PAL_ALARM);
        }
        if (!bot_) hudC(26, "START AGAIN   ESC TITLE", PAL_INK);
    } else {
        std::snprintf(buf, sizeof buf, "YOU %d", you_);
        hud(1, 0, buf, PAL_YOU);
        std::snprintf(buf, sizeof buf, "THEM %d", them_);
        int n = int(std::strlen(buf));
        hud(39 - n, 0, buf, PAL_THEM);
        hudC(0, "TO 7", PAL_LOGO);
        hudC(24, "BREAKS LEFT", PAL_LOGO);
        if (mode_ == Mode::Pause) {
            hudC(12, "PAUSED", PAL_LOGO);
            hudC(27, "START RESUME   ESC TITLE", PAL_INK);
        } else if (mode_ == Mode::Hold) {
            hudC(12, ball_.cup ? "IN THE CUP" : "LEFT IT SHORT", ball_.cup ? PAL_YOU : PAL_ALARM);
            if (fate_ == 1) hudC(14, "FIRST TO SEVEN", PAL_LOGO);
            else if (fate_ == 2) hudC(14, "THEY REACHED SEVEN", PAL_ALARM);
        } else if (!ball_.rest) {
            hudC(26, "ROLLING", PAL_LOGO);
        } else if (!yours_) {
            hudC(26, "THEIR PUTT", PAL_THEM);
        } else if (swinging_) {
            int filled = int(std::lround(meter_ * 16.f));
            int pct = int(std::lround(meter_ * 100.f));
            char bar[40];
            int p = std::snprintf(bar, sizeof bar, "POWER ");
            for (int i = 0; i < 16 && p + 1 < int(sizeof bar); i++) bar[p++] = (i < filled) ? '=' : '.';
            std::snprintf(bar + p, sizeof bar - size_t(p), " %d", pct);
            hud(1, 26, bar, PAL_LOGO);
            hud(1, 27, "RELEASE TO PUTT", PAL_INK);
        } else {
            hudC(26, "YOUR PUTT", PAL_YOU);
            hud(1, 27, "ARROWS AIM    HOLD Z", PAL_INK);
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    if (beep_ > 0.f) {
        beep_ -= DT;
        if (beep_ <= 0.f) {
            sys.apu.tone(0, 0, 0);
            sys.apu.tone(1, 0, 0);
            sys.apu.tone(2, 0, 0);
        }
    }
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        float base = std::atan2(CUP_Y - TEE_Y, CUP_X - TEE_X);
        aim_ = base + std::sin(t_ * 0.8f) * 0.16f;
        ball_.x = TEE_X;
        ball_.y = TEE_Y;
        ball_.vx = ball_.vy = 0.f;
        ball_.rest = true;
        ball_.cup = 0;
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) begin();
        else if (pad.pressed(gs::BTN_MODE)) sys.quit();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = held_;
        else if (pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            you_ = them_ = strokes_ = themPutts_ = fate_ = 0;
            won_ = false;
            over_ = false;
            yours_ = true;
            place();
        }
    } else if (mode_ == Mode::Hold) {
        holdT_ += DT;
        bool skip = !bot_ && pad.pressed(gs::BTN_START);
        float need = bot_ ? 0.02f : 0.5f;
        if (skip || holdT_ >= need) {
            if (fate_) {
                mode_ = Mode::Result;
                bannerT_ = 0.f;
            } else {
                yours_ = !yours_;
                place();
                mode_ = Mode::Play;
                if (yours_) sys.setLight(48, 140, 64);
                else sys.setLight(160, 48, 36);
            }
        }
    } else if (mode_ == Mode::Result) {
        bannerT_ += DT;
        if (!over_ && bannerT_ > (bot_ ? 0.25f : 0.45f)) over_ = true;
        if (!bot_ && pad.pressed(gs::BTN_START)) begin();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            you_ = them_ = strokes_ = themPutts_ = fate_ = 0;
            won_ = false;
            over_ = false;
            yours_ = true;
            place();
        }
    } else if (mode_ == Mode::Play) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_MODE))) {
            held_ = Mode::Play;
            mode_ = Mode::Pause;
        } else if (!ball_.rest) {
            int bumps = ball_.bumps;
            advance(ball_);
            rollFrames_++;
            if (ball_.bumps > bumps && sys_) sys.apu.noiseBurst(0.05f, 380.f, 0.02f);
            if (!ball_.rest && rollFrames_ >= ROLL_MAX) {
                ball_.vx = ball_.vy = 0.f;
                ball_.rest = true;
            }
            if (ball_.rest) settle();
        } else if (bot_ || !yours_) {
            bool waiting = !bot_ && !yours_ && cpuWait_ < 0.4f;
            bool miss = !bot_ && !yours_ && (themPutts_ % 3 == 2);
            aim_ = miss ? aimMiss_ : aimShot_;
            if (waiting) {
                cpuWait_ += DT;
            } else {
                float a = aim_;
                float v = miss ? spdMiss_ : spdShot_;
                if (!shotOk_) {
                    a = std::atan2(CUP_Y - ball_.y, CUP_X - ball_.x);
                    v = 150.f;
                }
                strike(a, v);
            }
        } else {
            float rate = 1.8f;
            if (std::fabs(pad.axisX) > 0.18f) humanAim_ += pad.axisX * rate * 1.35f * DT;
            else {
                if (pad.down(gs::BTN_LEFT)) humanAim_ -= rate * DT;
                if (pad.down(gs::BTN_RIGHT)) humanAim_ += rate * DT;
            }
            if (humanAim_ > PI) humanAim_ -= PI * 2.f;
            if (humanAim_ < -PI) humanAim_ += PI * 2.f;
            aim_ = humanAim_;
            bool swing = pad.down(gs::BTN_A) || pad.down(gs::BTN_B) || pad.down(gs::BTN_C) || pad.down(gs::BTN_TURBO);
            if (swing && !swinging_) {
                swinging_ = true;
                meter_ = 0.f;
                meterDir_ = 1.f;
            }
            if (swinging_ && swing) {
                meter_ += meterDir_ * DT / 0.85f;
                if (meter_ >= 1.f) {
                    meter_ = 1.f;
                    meterDir_ = -1.f;
                }
                if (meter_ <= 0.f) {
                    meter_ = 0.f;
                    meterDir_ = 1.f;
                }
            }
            if (swinging_ && !swing) strike(humanAim_, 64.f + meter_ * 166.f);
        }
    }

    draw();
}

}  // namespace puttseven

#include "game/puttgold.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace puttgold {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float OX = 16.f;
constexpr float OY = 48.f;
constexpr float GW = 288.f;
constexpr float GH = 120.f;
constexpr float TEE_X = 144.f;
constexpr float TEE_Y = 96.f;
constexpr float GOLD_X = 176.f;
constexpr float GOLD_Y = 28.f;
constexpr float CREAM_X = 96.f;
constexpr float CREAM_Y = 78.f;
constexpr float GOLD_R = 12.f;
constexpr float CREAM_R = 14.f;
constexpr float GOLD_LIP = 165.f;
constexpr float CREAM_LIP = 190.f;
constexpr float MU = 68.f;
constexpr float AX = -32.f;
constexpr float AY = 14.f;
constexpr float BALL_R = 4.2f;
constexpr float BOUNCE_E = 0.32f;
constexpr float STOP_V = 6.5f;
constexpr int TRIES = 3;
constexpr int ROLL_MAX = 260;
constexpr float PI = 3.14159265f;

struct Box {
    float x, y, w, h;
};

constexpr Box CORNER[4] = {
    {0, 0, 22, 14},
    {GW - 22, 0, 22, 14},
    {0, GH - 14, 22, 14},
    {GW - 22, GH - 14, 22, 14},
};

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

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
    float drop = MU * DT;
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
    if (!std::isfinite(b.x) || !std::isfinite(b.y)) {
        b.x = TEE_X;
        b.y = TEE_Y;
        b.vx = b.vy = 0.f;
        b.cup = 0;
        b.rest = true;
        return;
    }

    sp = std::hypot(b.vx, b.vy);
    float dg = std::hypot(b.x - GOLD_X, b.y - GOLD_Y);
    float dc = std::hypot(b.x - CREAM_X, b.y - CREAM_Y);
    bool g = dg <= GOLD_R && sp <= GOLD_LIP;
    bool c = dc <= CREAM_R && sp <= CREAM_LIP;
    if (g || c) {
        bool gold = g && (!c || dg <= dc);
        b.x = gold ? GOLD_X : CREAM_X;
        b.y = gold ? GOLD_Y : CREAM_Y;
        b.vx = b.vy = 0.f;
        b.cup = gold ? 2 : 1;
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

bool Game::solve(float x, float y, float& ang, float& spd) const {
    const float base = std::atan2(GOLD_Y - y, GOLD_X - x);
    auto scan = [&](float a0, float a1, float aStep, float v0, float v1, float vStep, float& oa, float& ov) {
        int best = 0;
        oa = base;
        ov = 120.f;
        float bestDelta = 1e9f;
        for (float a = a0; a <= a1 + 1e-4f; a += aStep) {
            int run = 0;
            int bestRun = 0;
            float runStart = v0;
            float mid = v0;
            for (float v = v0; v <= v1 + 1e-4f; v += vStep) {
                if (trial(x, y, a, v) == 2) {
                    if (run == 0) runStart = v;
                    run++;
                } else if (run) {
                    if (run > bestRun) {
                        bestRun = run;
                        mid = runStart + float((run - 1) / 2) * vStep;
                    }
                    run = 0;
                }
            }
            if (run > bestRun) {
                bestRun = run;
                mid = runStart + float((run - 1) / 2) * vStep;
            }
            float delta = std::fabs(a - base);
            if (bestRun > best || (bestRun == best && bestRun > 0 && delta < bestDelta)) {
                best = bestRun;
                bestDelta = delta;
                oa = a;
                ov = mid;
            }
        }
        return best;
    };

    float a = base, v = 120.f;
    int n = scan(base - 0.6f, base + 0.6f, 0.04f, 64.f, 210.f, 6.f, a, v);
    if (n <= 0) n = scan(base - 1.2f, base + 1.2f, 0.08f, 50.f, 230.f, 10.f, a, v);
    if (n > 0) {
        float a2 = a, v2 = v;
        int n2 = scan(a - 0.05f, a + 0.05f, 0.012f, std::max(48.f, v - 18.f), v + 18.f, 2.f, a2, v2);
        if (n2 > 0 && trial(x, y, a2, v2) == 2) {
            a = a2;
            v = v2;
        }
    }
    if (trial(x, y, a, v) != 2) {
        bool found = false;
        for (float dv = -16.f; dv <= 16.f && !found; dv += 2.f) {
            if (trial(x, y, a, v + dv) == 2) {
                v += dv;
                found = true;
            }
        }
        if (!found) n = 0;
    }
    ang = a;
    spd = v;
    return n > 0 && trial(x, y, ang, spd) == 2;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.setFogColor(gs::rgb4(2, 3, 2));
    sys.apu.setMaster(0.7f);
    place();
    mode_ = Mode::Title;
    if (bot_) begin();
}

void Game::place() {
    ball_ = Ball{};
    ball_.x = TEE_X;
    ball_.y = TEE_Y;
    ball_.rest = true;
    aim_ = std::atan2(GOLD_Y - TEE_Y, GOLD_X - TEE_X);
    swinging_ = false;
    meter_ = 0.f;
    meterDir_ = 1.f;
    rollFrames_ = 0;
}

void Game::spot() {
    place();
    mode_ = Mode::Play;
}

void Game::begin() {
    gold_ = 0;
    cream_ = 0;
    strokes_ = 0;
    won_ = false;
    over_ = false;
    bannerT_ = 0.f;
    holdT_ = 0.f;
    beep_ = 0.f;
    place();
    mode_ = Mode::Play;
    if (sys_) sys_->setLight(255, 196, 48);
}

void Game::strike(float ang, float spd) {
    aim_ = ang;
    ball_.vx = std::cos(ang) * spd;
    ball_.vy = std::sin(ang) * spd;
    ball_.rest = false;
    ball_.cup = 0;
    ball_.bumps = 0;
    swinging_ = false;
    rollFrames_ = 0;
    strokes_++;
    blip(210.f);
    if (sys_) sys_->apu.noiseBurst(0.22f, 1600.f, 0.05f);
}

void Game::win() {
    gold_ = 2;
    ball_.x = GOLD_X;
    ball_.y = GOLD_Y;
    ball_.vx = ball_.vy = 0.f;
    ball_.cup = 2;
    ball_.rest = true;
    won_ = true;
    bannerT_ = 0.f;
    mode_ = Mode::Result;
    beep_ = 0.85f;
    if (sys_) {
        sys_->apu.tone(0, 523.f, 0.12f);
        sys_->apu.tone(1, 659.f, 0.10f);
        sys_->apu.tone(2, 784.f, 0.11f);
        sys_->rumble(0.35f, 0.7f, 160);
        sys_->setLight(255, 210, 40);
    }
}

void Game::lose() {
    won_ = false;
    bannerT_ = 0.f;
    mode_ = Mode::Result;
    beep_ = 0.4f;
    if (sys_) {
        sys_->apu.tone(0, 110.f, 0.1f);
        sys_->setLight(170, 36, 28);
    }
}

void Game::finishPutt() {
    if (ball_.cup == 1) cream_++;
    if (bot_) {
        if (strokes_ >= TRIES) lose();
        else spot();
        return;
    }
    holdKind_ = ball_.cup;
    holdT_ = 0.f;
    mode_ = Mode::Hold;
    if (ball_.cup == 1) blip(180.f);
    else blip(90.f);
}

void Game::blip(float freq) {
    if (!sys_) return;
    sys_->apu.tone(0, freq, 0.08f);
    beep_ = 0.12f;
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
        if (y < 32) {
            float u = y / 31.f;
            v.lineBackdrop[y] = gs::rgb4(4 + int(u * 8), 5 + int(u * 4), 11 - int(u * 5));
        } else if (y < 48) {
            v.lineBackdrop[y] = gs::rgb4(6, 4, 2);
        } else if (y < 168) {
            bool band = ((y / 4) & 1) == 0;
            v.lineBackdrop[y] = band ? gs::rgb4(2, 8, 3) : gs::rgb4(3, 11, 4);
        } else if (y < 184) {
            v.lineBackdrop[y] = gs::rgb4(6, 4, 2);
        } else {
            v.lineBackdrop[y] = gs::rgb4(1, 2, 2);
        }
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
    if (mode_ == Mode::Result && won_) spr(art_.doubled, 160.f, 100.f, float(art_.doubled.h), PAL_LOGO);
    if (mode_ == Mode::Result && !won_) spr(art_.fell, 160.f, 100.f, float(art_.fell.h), PAL_RED);

    if (showAim) {
        float dx = std::cos(aim_);
        float dy = std::sin(aim_);
        int n = swinging_ ? 3 + int(meter_ * 5.f) : 5;
        for (int i = 1; i <= n; i++) {
            float d = 12.f + float(i) * 7.f;
            spr(art_.dot, OX + ball_.x + dx * d, OY + ball_.y + dy * d, 4.5f, PAL_AIM);
        }
    }

    spr(art_.num2, OX + GOLD_X + 16.f, OY + GOLD_Y - 8.f, 14.f, PAL_GOLD);
    spr(art_.num1, OX + CREAM_X + 16.f, OY + CREAM_Y - 8.f, 14.f, PAL_CREAM);

    float ballH = ball_.cup ? 7.f : 10.f;
    spr(art_.ball, OX + ball_.x, OY + ball_.y, ballH, PAL_BALL);
    spr(art_.shadow, OX + ball_.x + 2.f, OY + ball_.y + 3.f, 7.f, PAL_BALL, false, true);

    const gs::Mipped& flag = art_.flag[(int(t_ * 8.f) & 1)];
    spr(flag, OX + GOLD_X, OY + GOLD_Y - 16.f, 30.f, PAL_GOLD);
    spr(flag, OX + CREAM_X, OY + CREAM_Y - 14.f, 26.f, PAL_CREAM);
    float pulse = 20.f + std::sin(t_ * 3.f) * 1.2f;
    spr(art_.cup, OX + GOLD_X, OY + GOLD_Y, pulse, PAL_GOLD);
    spr(art_.cup, OX + CREAM_X, OY + CREAM_Y, 22.f, PAL_CREAM);

    const float tuft[6][2] = {{36, 36}, {250, 40}, {28, 78}, {252, 86}, {124, 40}, {214, 86}};
    for (int i = 0; i < 6; i++) {
        float x = tuft[i][0] + std::sin(t_ * 1.4f + float(i)) * 1.2f;
        float y = tuft[i][1];
        spr(art_.tuft, OX + x, OY + y, 10.f, PAL_HEDGE, i & 1);
    }
    for (const Box& c : CORNER) spr(art_.hedge, OX + c.x + c.w * 0.5f, OY + c.y + c.h * 0.5f, c.h, PAL_HEDGE);

    const float trees[4][2] = {{48, 30}, {118, 28}, {214, 30}, {286, 28}};
    for (int i = 0; i < 4; i++) spr(art_.tree, trees[i][0], trees[i][1], 34.f, PAL_TREE, i & 1);
    spr(art_.sun, 300.f, 14.f, 16.f, PAL_SUN);

    patch(art_.wood, 0, 32, 320, 16, PAL_WOOD);
    patch(art_.wood, 0, 168, 320, 16, PAL_WOOD);
    patch(art_.wood, 0, 48, 16, 120, PAL_WOOD);
    patch(art_.wood, 304, 48, 16, 120, PAL_WOOD);

    char buf[64];
    if (mode_ == Mode::Title) {
        hudC(24, "A SHORT PUTT", PAL_GOLD);
        hudC(25, "ONLY THE GOLD COUNTS DOUBLE", PAL_INK);
        if ((int(t_ * 2.f) & 1) == 0) hudC(26, "PRESS START", PAL_GOLD);
        hudC(27, "ARROWS AIM   HOLD Z PUTT", PAL_INK);
    } else if (mode_ == Mode::Result) {
        if (won_) {
            hudC(16, "GOLD COUNTS 2", PAL_GOLD);
            std::snprintf(buf, sizeof buf, "CREAM %d", cream_);
            hudC(18, buf, PAL_CREAM);
            std::snprintf(buf, sizeof buf, "STROKES %d", strokes_);
            hudC(20, buf, PAL_INK);
            hudC(26, "ONLY THE GOLD COUNTS DOUBLE", PAL_GREEN);
        } else {
            hudC(16, "CREAM DOES NOT DOUBLE", PAL_RED);
            hudC(18, "GOLD COUNTS 2", PAL_GOLD);
            std::snprintf(buf, sizeof buf, "CREAM %d   STROKES %d", cream_, strokes_);
            hudC(20, buf, PAL_INK);
            hudC(26, "NO DOUBLE", PAL_RED);
        }
        if (!bot_) hudC(27, "START AGAIN", PAL_GOLD);
    } else {
        std::snprintf(buf, sizeof buf, "STROKES %d/%d", strokes_, TRIES);
        hud(1, 0, buf, PAL_INK);
        int score = gold_ + cream_;
        std::snprintf(buf, sizeof buf, "SCORE %d", score);
        int n = int(std::strlen(buf));
        hud(39 - n, 0, buf, score > 0 ? PAL_GOLD : PAL_INK);
        hud(1, 1, "GOLD X2", PAL_GOLD);
        hud(30, 1, "CREAM X1", PAL_CREAM);
        hudC(24, "BREAKS LEFT", PAL_GOLD);
        if (mode_ == Mode::Pause) {
            hudC(12, "PAUSED", PAL_GOLD);
            hudC(27, "START RESUME   ESC TITLE", PAL_INK);
        } else if (mode_ == Mode::Hold) {
            if (holdKind_ == 1) {
                hudC(12, "ONE", PAL_CREAM);
                hudC(14, "NOT A DOUBLE", PAL_INK);
            } else {
                hudC(12, "NO CUP", PAL_RED);
            }
            hudC(27, strokes_ >= TRIES ? "NO DOUBLE" : "BACK TO THE TEE", PAL_INK);
        } else if (swinging_) {
            int filled = int(std::lround(meter_ * 16.f));
            int pct = int(std::lround(meter_ * 100.f));
            char bar[40];
            int p = std::snprintf(bar, sizeof bar, "POWER ");
            for (int i = 0; i < 16 && p + 1 < int(sizeof bar); i++) bar[p++] = (i < filled) ? '=' : '.';
            std::snprintf(bar + p, sizeof bar - size_t(p), " %d", pct);
            hud(1, 26, bar, PAL_GOLD);
            hud(1, 27, "RELEASE TO PUTT", PAL_INK);
        } else if (ball_.rest && ball_.cup == 0) {
            hud(1, 26, "HOLD Z TO PUTT", PAL_INK);
            hud(1, 27, "ARROWS AIM    Z PUTT", PAL_INK);
        } else {
            hud(1, 26, "ROLLING", PAL_GOLD);
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
        float base = std::atan2(GOLD_Y - TEE_Y, GOLD_X - TEE_X);
        aim_ = base + std::sin(t_ * 0.7f) * 0.12f;
        ball_.x = TEE_X;
        ball_.y = TEE_Y;
        ball_.rest = true;
        ball_.cup = 0;
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) begin();
        else if (pad.pressed(gs::BTN_MODE)) sys.quit();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = held_;
        else if (pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            place();
            won_ = false;
            over_ = false;
        }
    } else if (mode_ == Mode::Hold) {
        holdT_ += DT;
        bool skip = pad.pressed(gs::BTN_START);
        if (skip || holdT_ > 0.7f) {
            if (strokes_ >= TRIES) lose();
            else spot();
        }
    } else if (mode_ == Mode::Result) {
        bannerT_ += DT;
        if (!over_ && bannerT_ > (bot_ ? 0.35f : 0.45f)) over_ = true;
        if (!bot_ && pad.pressed(gs::BTN_START)) begin();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            place();
            won_ = false;
            over_ = false;
            gold_ = 0;
            cream_ = 0;
            strokes_ = 0;
        }
    } else if (mode_ == Mode::Play) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_MODE))) {
            held_ = Mode::Play;
            mode_ = Mode::Pause;
        } else if (!ball_.rest) {
            int bumps = ball_.bumps;
            advance(ball_);
            rollFrames_++;
            if (ball_.bumps > bumps) sys.apu.noiseBurst(0.08f, 420.f, 0.03f);
            if (!ball_.rest && rollFrames_ >= ROLL_MAX) {
                ball_.vx = ball_.vy = 0.f;
                ball_.rest = true;
            }
            if (ball_.rest) {
                if (ball_.cup == 2) win();
                else finishPutt();
            }
        } else if (bot_) {
            float a = aim_;
            float v = 120.f;
            if (!solve(ball_.x, ball_.y, a, v)) a = std::atan2(GOLD_Y - ball_.y, GOLD_X - ball_.x);
            strike(a, v);
        } else {
            float rate = 1.15f;
            if (std::fabs(pad.axisX) > 0.15f) aim_ += pad.axisX * rate * 1.35f * DT;
            else {
                if (pad.down(gs::BTN_LEFT)) aim_ -= rate * DT;
                if (pad.down(gs::BTN_RIGHT)) aim_ += rate * DT;
            }
            if (aim_ > PI) aim_ -= PI * 2.f;
            if (aim_ < -PI) aim_ += PI * 2.f;
            bool swing = pad.down(gs::BTN_A) || pad.down(gs::BTN_B) || pad.down(gs::BTN_C) || pad.down(gs::BTN_TURBO);
            if (swing && !swinging_) {
                swinging_ = true;
                meter_ = 0.f;
                meterDir_ = 1.f;
            }
            if (swinging_ && swing) {
                meter_ += meterDir_ * DT / 0.9f;
                if (meter_ >= 1.f) {
                    meter_ = 1.f;
                    meterDir_ = -1.f;
                }
                if (meter_ <= 0.f) {
                    meter_ = 0.f;
                    meterDir_ = 1.f;
                }
            }
            if (swinging_ && !swing) strike(aim_, 60.f + meter_ * 160.f);
        }
    }

    draw();
}

}  // namespace puttgold

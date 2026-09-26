#include "game/puttbell.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace puttbell {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float OX = 28.f;
constexpr float OY = 34.f;
constexpr float GW = 264.f;
constexpr float GH = 156.f;
constexpr float BALL_R = 5.f;
constexpr float MU = 70.f;
constexpr float AX = 30.f;
constexpr float AY = 16.f;
constexpr float HIT = 12.f;
constexpr float RING_MIN = 26.f;
constexpr float STOP_V = 7.f;
constexpr float BOUNCE = 0.45f;
constexpr float TEE_X = 120.f;
constexpr float TEE_Y = 126.f;
constexpr float BELL_X = 168.f;
constexpr float BELL_Y = 26.f;
constexpr float PI = 3.14159265f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

float wrapPi(float a) {
    while (a > PI) a -= PI * 2.f;
    while (a < -PI) a += PI * 2.f;
    return a;
}

}  // namespace

Game::Halt Game::stepLie(Lie& b, float dt) {
    if (b.rest) return Halt::Rest;
    b.vx += AX * dt;
    b.vy += AY * dt;
    float sp = std::hypot(b.vx, b.vy);
    float drop = MU * dt;
    if (sp <= drop) {
        b.vx = b.vy = 0;
    } else {
        float s = (sp - drop) / sp;
        b.vx *= s;
        b.vy *= s;
    }
    b.x += b.vx * dt;
    b.y += b.vy * dt;

    float dx = b.x - BELL_X;
    float dy = b.y - BELL_Y;
    float d = std::hypot(dx, dy);
    if (d < HIT && d > 1e-4f) {
        sp = std::hypot(b.vx, b.vy);
        if (sp >= RING_MIN) {
            b.x = BELL_X;
            b.y = BELL_Y;
            b.vx = b.vy = 0;
            b.rest = true;
            return Halt::Bell;
        }
        float nx = dx / d;
        float ny = dy / d;
        b.x += nx * (HIT - d);
        b.y += ny * (HIT - d);
        float vn = b.vx * nx + b.vy * ny;
        if (vn < 0) {
            b.vx -= 1.3f * vn * nx;
            b.vy -= 1.3f * vn * ny;
        }
    }

    if (b.x < BALL_R) {
        b.x = BALL_R;
        if (b.vx < 0) b.vx = -b.vx * BOUNCE;
    }
    if (b.x > GW - BALL_R) {
        b.x = GW - BALL_R;
        if (b.vx > 0) b.vx = -b.vx * BOUNCE;
    }
    if (b.y < BALL_R) {
        b.y = BALL_R;
        if (b.vy < 0) b.vy = -b.vy * BOUNCE;
    }
    if (b.y > GH - BALL_R) {
        b.y = GH - BALL_R;
        if (b.vy > 0) b.vy = -b.vy * BOUNCE;
    }

    sp = std::hypot(b.vx, b.vy);
    if (sp < STOP_V) {
        b.vx = b.vy = 0;
        b.rest = true;
        return Halt::Rest;
    }
    return Halt::Fly;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.setFogColor(gs::rgb4(2, 6, 3));
    sys.apu.setMaster(0.72f);
    toTitle();
}

void Game::toTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    rung_ = false;
    dead_ = 0;
    tryNo_ = 0;
    swinging_ = false;
    botReady_ = false;
    bellAmp_ = 0.22f;
    bellPh_ = 0;
    clock_ = 0;
    ball_ = Lie{};
    ball_.x = TEE_X;
    ball_.y = TEE_Y;
    ball_.rest = true;
    aim_ = std::atan2(BELL_Y - TEE_Y, BELL_X - TEE_X);
}

void Game::newGame() {
    dead_ = 0;
    tryNo_ = 0;
    won_ = false;
    over_ = false;
    rung_ = false;
    bellAmp_ = 0.22f;
    beginAim();
}

void Game::beginAim() {
    ball_ = Lie{};
    ball_.x = TEE_X;
    ball_.y = TEE_Y;
    ball_.rest = true;
    aim_ = std::atan2(BELL_Y - TEE_Y, BELL_X - TEE_X);
    meter_ = 0;
    meterDir_ = 1.f;
    swinging_ = false;
    aimT_ = 0;
    rollT_ = 0;
    botReady_ = false;
    mode_ = Mode::Aim;
    if (bot_) {
        float a = aim_;
        float v = 190.f;
        if (solve(a, v)) {
            aim_ = a;
            botSpd_ = v;
        } else {
            botSpd_ = 190.f;
        }
        botReady_ = true;
    }
}

void Game::putt(float ang, float spd) {
    aim_ = ang;
    ball_.vx = std::cos(ang) * spd;
    ball_.vy = std::sin(ang) * spd;
    ball_.rest = false;
    swinging_ = false;
    rollT_ = 0;
    mode_ = Mode::Roll;
    if (sys_) {
        sys_->apu.noiseBurst(0.22f, 1700.f, 0.05f);
        blip(210.f, 0, 0.06f);
    }
}

void Game::ring() {
    if (rung_) return;
    if (mode_ != Mode::Roll) return;
    rung_ = true;
    won_ = true;
    tryNo_ = dead_ + 1;
    ball_.x = BELL_X;
    ball_.y = BELL_Y;
    ball_.vx = ball_.vy = 0;
    ball_.rest = true;
    bellAmp_ = 1.f;
    bellTick_ = 0.22f;
    ringT_ = 0;
    mode_ = Mode::Ring;
    if (!sys_) return;
    sys_->apu.noise(0, 1000);
    blip(784.f, 1174.f, 0.28f);
    sys_->rumble(0.4f, 0.8f, 180);
    sys_->setLight(255, 196, 48);
}

void Game::dieTry() {
    if (rung_) return;
    if (mode_ != Mode::Roll) return;
    ball_.vx = ball_.vy = 0;
    ball_.rest = true;
    dead_++;
    if (sys_) sys_->apu.noise(0, 1000);
    if (dead_ >= 3) {
        mode_ = Mode::Over;
        won_ = false;
        over_ = true;
        if (sys_) {
            blip(90.f, 64.f, 0.45f);
            sys_->setLight(150, 28, 28);
        }
    } else {
        mode_ = Mode::Dead;
        deadT_ = 0;
        if (sys_) {
            blip(140.f, 0, 0.28f);
            sys_->setLight(120, 48, 28);
        }
    }
}

bool Game::reaches(float ang, float spd) const {
    Lie b;
    b.x = TEE_X;
    b.y = TEE_Y;
    b.vx = std::cos(ang) * spd;
    b.vy = std::sin(ang) * spd;
    b.rest = false;
    for (int i = 0; i < 300; i++) {
        Halt h = stepLie(b, DT);
        if (h == Halt::Bell) return true;
        if (h == Halt::Rest) return false;
    }
    return false;
}

bool Game::solve(float& ang, float& spd) const {
    float base = std::atan2(BELL_Y - TEE_Y, BELL_X - TEE_X);
    bool any = false;
    float best = 1e9f;
    float bestA = base;
    float bestV = 190.f;
    for (float da = -0.55f; da <= 0.35f; da += 0.025f) {
        float a = base + da;
        for (float v = 90.f; v <= 250.f; v += 5.f) {
            if (!reaches(a, v)) continue;
            bool wide = reaches(a, v - 8.f) && reaches(a, v + 8.f);
            float score = std::fabs(da) * 6.f + std::fabs(v - 190.f) * 0.02f + (wide ? 0.f : 3.f);
            if (score < best) {
                best = score;
                bestA = a;
                bestV = v;
                any = true;
            }
        }
    }
    ang = bestA;
    spd = bestV;
    return any;
}

void Game::blip(float a, float b, float hold) {
    if (!sys_) return;
    sys_->apu.tone(0, a, 0.11f);
    if (b > 0) sys_->apu.tone(1, b, 0.08f);
    toneHold_ = hold;
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
    if (w < 1.f || h < 1.f) return;
    for (float yy = 0; yy < h - 0.1f; yy += ts) {
        float rh = std::min(ts, h - yy);
        for (float xx = 0; xx < w - 0.1f; xx += ts) {
            float rw = std::min(ts, w - xx);
            spr(m, x + xx + rw * 0.5f, y + yy + rh * 0.5f, std::min(rw, rh), pal);
        }
    }
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (!sys_ || row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        int tile = art_.font[c - 32];
        if (!tile) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(tile, pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        if (y < 30) {
            float u = y / 29.f;
            v.lineBackdrop[y] = gs::rgb4(4 + int(u * 6), 7 + int(u * 4), 14 - int(u * 4));
        } else if (y < int(OY)) {
            v.lineBackdrop[y] = gs::rgb4(2, 6, 2);
        } else if (y < int(OY + GH)) {
            float u = (y - OY) / GH;
            int g = 6 + int(u * 6);
            int band = (y / 3) & 1;
            v.lineBackdrop[y] = gs::rgb4(1 + band, g - band, 2);
        } else {
            v.lineBackdrop[y] = gs::rgb4(5, 3, 1);
        }
    }

    bool sunk = rung_ && (mode_ == Mode::Ring || mode_ == Mode::Leave || (mode_ == Mode::Over && won_));
    float bx = OX + ball_.x;
    float by = OY + ball_.y;
    float hx = OX + BELL_X;
    float hy = OY + BELL_Y;
    float amp = rung_ ? bellAmp_ : 0.22f;
    float swing = std::sin(bellPh_) * 8.f * amp;
    int pose = (mode_ == Mode::Roll && rollT_ < 0.32f) ? 1 : 0;
    float walk = (mode_ == Mode::Leave || (mode_ == Mode::Over && won_)) ? leaveT_ * 52.f : 0.f;

    bool showAim = mode_ == Mode::Title || mode_ == Mode::Aim || mode_ == Mode::Pause;
    if (mode_ == Mode::Title) spr(art_.logo, 160, 11, float(art_.logo.h), PAL_GOLD);

    if (showAim) {
        float dx = std::cos(aim_);
        float dy = std::sin(aim_);
        int n = swinging_ ? 3 + int(meter_ * 5.f) : 6;
        for (int i = 1; i <= n; i++) {
            float d = 12.f + i * 8.f;
            spr(art_.dot, bx + dx * d, by + dy * d, swinging_ ? 5.f : 4.f, PAL_AIM);
        }
    }
    if (rung_ && bellAmp_ > 0.45f) {
        for (int i = 0; i < 6; i++) {
            float a = bellPh_ * 1.4f + i * 1.047f;
            float rad = 16.f + (1.f - bellAmp_) * 18.f;
            spr(art_.dot, hx + std::cos(a) * rad, hy - 10.f + std::sin(a) * rad * 0.45f, 4.f, PAL_GOLD);
        }
    }

    float ballH = sunk ? 8.f : 11.f;
    spr(art_.ball, bx, by, ballH, PAL_BALL);
    spr(art_.shadow, bx + 3.f, by + 4.f, 7.f, PAL_BALL, false, true);
    if (mode_ != Mode::Leave || walk < 70.f) {
        const gs::Mipped& g = art_.golfer[pose];
        spr(g, bx - 2.f + walk, by + 20.f, 36.f, PAL_PLAYER, false);
    }

    spr(art_.clapper, hx + swing * 1.45f, hy - 2.f, 7.f, PAL_BELL);
    spr(art_.bell, hx + swing, hy - 16.f, 32.f, PAL_BELL);
    spr(art_.yoke, hx, hy - 36.f, 16.f, PAL_WOOD);
    spr(art_.cup, hx, hy + 2.f, 18.f, PAL_CUP);

    const float flowers[][2] = {{36, 78}, {46, 108}, {232, 96}};
    for (int i = 0; i < 3; i++) spr(art_.flower, OX + flowers[i][0], OY + flowers[i][1], 14.f, PAL_ALERT);

    float slide = std::fmod(clock_ * 16.f, 52.f);
    for (int i = 0; i < 4; i++) {
        float x = 28.f + i * 58.f + slide;
        if (x > GW - 24.f) x -= 232.f;
        float y = 78.f + (i & 1) * 16.f;
        spr(art_.chevron, OX + x, OY + y, 8.f, PAL_AIM);
    }
    const float tuftX[6] = {58, 92, 150, 210, 70, 200};
    const float tuftY[6] = {48, 64, 58, 52, 100, 112};
    for (int i = 0; i < 6; i++) {
        float x = tuftX[i];
        float y = tuftY[i];
        if (std::hypot(x - BELL_X, y - BELL_Y) < 26.f) continue;
        if (std::hypot(x - ball_.x, y - ball_.y) < 16.f) continue;
        float sway = std::sin(clock_ * 1.7f + i) * 1.2f;
        spr(art_.tuft, OX + x + sway, OY + y, 11.f, PAL_GRASS);
    }

    spr(art_.tree, 48, 28, 40.f, PAL_TREE, false);
    spr(art_.tree, 108, 24, 34.f, PAL_TREE, true);
    spr(art_.tree, 268, 26, 38.f, PAL_TREE, false);
    spr(art_.cloud, 70 + std::sin(clock_ * 0.25f) * 4.f, 14, 16.f, PAL_CLOUD);
    spr(art_.cloud, 230, 12, 12.f, PAL_CLOUD);
    spr(art_.sun, 292, 14, 16.f, PAL_SUN);

    patch(art_.rail, OX - 16.f, OY - 16.f, GW + 32.f, 16.f, PAL_WOOD);
    patch(art_.rail, OX - 16.f, OY + GH, GW + 32.f, 16.f, PAL_WOOD);
    patch(art_.rail, OX - 16.f, OY, 16.f, GH, PAL_WOOD);
    patch(art_.rail, OX + GW, OY, 16.f, GH, PAL_WOOD);

    char buf[64];
    if (mode_ == Mode::Title) {
        hudC(24, "RING THE BELL", PAL_GOLD);
        hudC(25, "BEFORE THE THIRD TRY DIES", PAL_HUD);
        if ((int(clock_ * 2.f) & 1) == 0) hudC(27, "PRESS START", PAL_GOLD);
        else hudC(27, "ARROWS AIM   HOLD Z", PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        hudC(11, "PAUSED", PAL_GOLD);
        hudC(27, "START RESUME   ESC TITLE", PAL_HUD);
    } else if (mode_ == Mode::Over && won_) {
        hudC(11, "BELL", PAL_GOLD);
        hudC(13, "LEFT BEFORE THE THIRD TRY", PAL_HUD);
        if (!bot_) hudC(27, "START AGAIN", PAL_GOLD);
    } else if (mode_ == Mode::Over) {
        hudC(11, "THIRD TRY DEAD", PAL_ALERT);
        hudC(13, "BELL SILENT", PAL_HUD);
        if (!bot_) hudC(27, "START AGAIN", PAL_GOLD);
    } else if (mode_ == Mode::Ring) {
        hudC(11, "BELL", PAL_GOLD);
    } else if (mode_ == Mode::Leave) {
        hudC(11, "LEAVE", PAL_GREEN);
        hudC(13, "THE BELL RANG", PAL_GOLD);
    } else if (mode_ == Mode::Dead) {
        hudC(11, "TRY DIED", PAL_ALERT);
        hudC(13, dead_ == 1 ? "TWO LEFT" : "ONE LEFT", PAL_GOLD);
    } else {
        std::snprintf(buf, sizeof buf, "TRY %d OF 3", dead_ + 1);
        hud(1, 0, buf, dead_ == 2 ? PAL_ALERT : PAL_GOLD);
        std::snprintf(buf, sizeof buf, "DEAD %d", dead_);
        hud(32, 0, buf, PAL_HUD);
        hud(1, 1, "S3 PUTTBELL", PAL_GOLD);
        if (mode_ == Mode::Aim || mode_ == Mode::Roll) {
            hudC(24, "FIRM AT THE BELL", PAL_GOLD);
            hudC(25, "SOFT DIES RIGHT", PAL_HUD);
        }
        if (mode_ == Mode::Roll) {
            hud(1, 27, "ROLLING", PAL_GOLD);
        } else if (swinging_) {
            int n = int(std::lround(clampf(meter_, 0.f, 1.f) * 16.f));
            std::string bar = "POWER ";
            for (int i = 0; i < 16; i++) {
                if (i < n) bar += (i >= 8) ? '#' : '=';
                else bar += (i == 8) ? '|' : '.';
            }
            hud(1, 26, bar, PAL_GOLD);
            hud(1, 27, "RELEASE PAST HALF", PAL_HUD);
        } else if (mode_ == Mode::Aim) {
            hud(1, 26, "HOLD Z TO PUTT", PAL_HUD);
            hud(1, 27, "ARROWS AIM", PAL_HUD);
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    clock_ += DT;
    bellPh_ += DT * (rung_ ? 13.f : 2.2f);
    if (rung_) bellAmp_ = std::max(0.4f, bellAmp_ - DT * 0.18f);
    if (toneHold_ > 0) {
        toneHold_ -= DT;
        if (toneHold_ <= 0) {
            sys.apu.tone(0, 0, 0);
            sys.apu.tone(1, 0, 0);
            sys.apu.tone(2, 0, 0);
        }
    }
    if (rung_ && (mode_ == Mode::Ring || mode_ == Mode::Leave || mode_ == Mode::Over)) {
        bellTick_ -= DT;
        if (bellTick_ <= 0 && bellAmp_ > 0.5f) {
            float vol = 0.05f + 0.1f * bellAmp_;
            sys.apu.tone(0, 784.f, vol);
            sys.apu.tone(1, 1174.f, vol * 0.65f);
            toneHold_ = 0.16f;
            bellTick_ = 0.22f;
        }
    }

    const gs::Pad& pad = sys.pad;
    bool start = pad.pressed(gs::BTN_START);
    bool back = pad.pressed(gs::BTN_MODE);
    if (bot_) {
        start = false;
        back = false;
        if (mode_ == Mode::Title && clock_ > 0.4f) start = true;
    }

    if (mode_ == Mode::Pause) {
        if (start) mode_ = held_;
        else if (back) toTitle();
    } else if (mode_ == Mode::Title) {
        if (back && !bot_) sys.quit();
        else if (start) newGame();
    } else if (mode_ == Mode::Over) {
        if (!bot_ && start) newGame();
        else if (!bot_ && back) toTitle();
    } else if (start && !bot_) {
        held_ = mode_;
        mode_ = Mode::Pause;
    } else if (mode_ == Mode::Aim) {
        aimT_ += DT;
        if (bot_) {
            if (botReady_ && aimT_ > 0.35f) putt(aim_, botSpd_);
        } else {
            float rate = 1.15f;
            if (std::fabs(pad.axisX) > 0.18f) aim_ += pad.axisX * rate * DT;
            else {
                if (pad.down(gs::BTN_LEFT)) aim_ -= rate * DT;
                if (pad.down(gs::BTN_RIGHT)) aim_ += rate * DT;
            }
            float base = std::atan2(BELL_Y - TEE_Y, BELL_X - TEE_X);
            float da = wrapPi(aim_ - base);
            aim_ = base + clampf(da, -1.15f, 1.15f);
            bool hold = pad.down(gs::BTN_A) || pad.down(gs::BTN_C) || pad.down(gs::BTN_TURBO);
            if (hold && !swinging_) {
                swinging_ = true;
                meter_ = 0;
                meterDir_ = 1.f;
            }
            if (swinging_ && hold) {
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
            if (swinging_ && !hold) putt(aim_, 70.f + meter_ * 180.f);
        }
    } else if (mode_ == Mode::Roll) {
        rollT_ += DT;
        Halt h = stepLie(ball_, DT);
        if (h == Halt::Bell) ring();
        else if (h == Halt::Rest || rollT_ > 6.f) dieTry();
    } else if (mode_ == Mode::Dead) {
        deadT_ += DT;
        if (deadT_ > 0.75f) beginAim();
    } else if (mode_ == Mode::Ring) {
        ringT_ += DT;
        if (ringT_ > 0.7f) {
            mode_ = Mode::Leave;
            leaveT_ = 0;
        }
    } else if (mode_ == Mode::Leave) {
        leaveT_ += DT;
        if (leaveT_ > 0.95f) {
            mode_ = Mode::Over;
            won_ = true;
            over_ = true;
        }
    }

    if (mode_ == Mode::Roll) sys.apu.noise(0.03f, 2600.f, true);
    else sys.apu.noise(0, 1000);

    draw();
}

}  // namespace puttbell

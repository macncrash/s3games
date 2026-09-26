#include "game/puttmark.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

#include "version.h"

namespace puttmark {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float GW = 256.f;
constexpr float GH = 152.f;
constexpr float INSET = 16.f;
constexpr float OX = 32.f;
constexpr float OY = 32.f;
constexpr float TEE_X = 128.f;
constexpr float TEE_Y = 124.f;
constexpr float CUP_X = 128.f;
constexpr float CUP_Y = 40.f;
constexpr float COIN_X = 76.f;
constexpr float COIN_Y = 104.f;
constexpr float GX = 18.f;
constexpr float GY = 12.f;
constexpr float MU = 74.f;
constexpr float CUP_R = 11.f;
constexpr float LIP = 130.f;
constexpr float STOP_V = 8.f;
constexpr float SPD_MIN = 60.f;
constexpr float SPD_MAX = 190.f;
constexpr float MARK_R = 8.f;
constexpr float LIFT_R = 12.f;
constexpr float COIN_V = 92.f;
constexpr float HAND_V = 110.f;
constexpr int MAX_PUTTS = 4;
constexpr float PI = 3.14159265f;
constexpr float TAU = 6.2831853f;

float wrapAng(float a) {
    while (a > PI) a -= TAU;
    while (a < -PI) a += TAU;
    return a;
}

void clampGreen(float& x, float& y) {
    x = std::clamp(x, INSET, GW - INSET);
    y = std::clamp(y, INSET, GH - INSET);
}

}  // namespace

void Game::place() {
    ball_ = Lie{};
    ball_.x = TEE_X;
    ball_.y = TEE_Y;
    ball_.rest = true;
    coinX_ = COIN_X;
    coinY_ = COIN_Y;
    handX_ = TEE_X - 18.f;
    handY_ = TEE_Y;
    aim_ = std::atan2(CUP_Y - TEE_Y, CUP_X - TEE_X);
    spd_ = 0;
    meter_ = 0;
    meterDir_ = 1.f;
    holdT_ = holeT_ = rollT_ = sayT_ = 0;
    say_ = "";
    strokes_ = 0;
    marked_ = holed_ = lifted_ = solved_ = charging_ = onBall_ = false;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.setFogColor(gs::rgb4(2, 6, 3));
    sys.apu.setMaster(0.65f);
    place();
    won_ = over_ = false;
    mode_ = Mode::Title;
    clock_ = 0;
}

void Game::begin() {
    place();
    won_ = false;
    over_ = false;
    mode_ = Mode::Mark;
}

void Game::say(const char* s, float time) {
    say_ = s;
    sayT_ = time;
}

void Game::blip(float freq) {
    if (!sys_) return;
    sys_->apu.tone(0, freq, 0.08f);
    beep_ = std::max(beep_, 0.09f);
}

void Game::chord(float a, float b, float c, float hold) {
    if (!sys_) return;
    sys_->apu.tone(0, a, 0.10f);
    sys_->apu.tone(1, b, 0.08f);
    sys_->apu.tone(2, c, 0.07f);
    beep_ = hold;
}

Game::Stop Game::roll(Lie& b, float dt) const {
    if (b.sunk) return Stop::Sunk;
    if (b.off) return Stop::Off;
    if (b.rest) return Stop::Rest;
    const int sub = 4;
    const float h = dt / float(sub);
    for (int i = 0; i < sub; i++) {
        b.vx += GX * h;
        b.vy += GY * h;
        float sp = std::hypot(b.vx, b.vy);
        float drop = MU * h;
        if (sp <= drop) {
            b.vx = b.vy = 0;
        } else {
            float s = (sp - drop) / sp;
            b.vx *= s;
            b.vy *= s;
        }
        b.x += b.vx * h;
        b.y += b.vy * h;
        if (b.x < INSET || b.y < INSET || b.x > GW - INSET || b.y > GH - INSET) {
            b.off = true;
            b.vx = b.vy = 0;
            return Stop::Off;
        }
        float d = std::hypot(b.x - CUP_X, b.y - CUP_Y);
        sp = std::hypot(b.vx, b.vy);
        if (d <= CUP_R && sp <= LIP) {
            b.sunk = true;
            b.rest = true;
            b.x = CUP_X;
            b.y = CUP_Y;
            b.vx = b.vy = 0;
            return Stop::Sunk;
        }
        if (sp < STOP_V) {
            b.vx = b.vy = 0;
            b.rest = true;
            return Stop::Rest;
        }
    }
    return Stop::Moving;
}

bool Game::sinks(float x, float y, float ang, float spd) const {
    Lie b;
    b.x = x;
    b.y = y;
    b.vx = std::cos(ang) * spd;
    b.vy = std::sin(ang) * spd;
    b.rest = false;
    for (int i = 0; i < 240; i++) {
        Stop s = roll(b, DT);
        if (s == Stop::Sunk) return true;
        if (s != Stop::Moving) return false;
    }
    return false;
}

bool Game::solve(float x, float y, float& ang, float& spd) const {
    float base = std::atan2(CUP_Y - y, CUP_X - x);
    bool any = false;
    float bestA = base;
    float bestV = 120.f;
    float bestScore = 1e9f;
    for (int ia = -28; ia <= 28; ia++) {
        float a = base + ia * 0.025f;
        float found[40];
        int n = 0;
        for (int iv = 0; iv <= 32 && n < 40; iv++) {
            float v = SPD_MIN + iv * 4.f;
            if (v > SPD_MAX) break;
            if (sinks(x, y, a, v)) found[n++] = v;
        }
        if (!n) continue;
        float width = found[n - 1] - found[0];
        float score = std::fabs(float(ia)) * 1.2f - width;
        if (!any || score < bestScore) {
            any = true;
            bestScore = score;
            bestA = a;
            bestV = found[n / 2];
        }
    }
    if (!any) return false;
    ang = bestA;
    spd = bestV;
    return true;
}

void Game::tryMark() {
    float d = std::hypot(coinX_ - ball_.x, coinY_ - ball_.y);
    if (d > MARK_R) {
        say("NOT ON THE BALL", 0.55f);
        blip(110.f);
        return;
    }
    coinX_ = ball_.x;
    coinY_ = ball_.y;
    marked_ = true;
    aim_ = std::atan2(CUP_Y - ball_.y, CUP_X - ball_.x);
    solved_ = false;
    spd_ = 120.f;
    if (bot_) {
        float a = aim_;
        float v = spd_;
        if (solve(ball_.x, ball_.y, a, v)) {
            aim_ = a;
            spd_ = v;
            solved_ = true;
        }
    }
    charging_ = false;
    meter_ = 0;
    meterDir_ = 1.f;
    mode_ = Mode::Hold;
    holdT_ = 0.40f;
    blip(523.f);
    say("MARK OPEN", 0.40f);
}

void Game::putt(float ang, float spd) {
    aim_ = ang;
    ball_.vx = std::cos(ang) * spd;
    ball_.vy = std::sin(ang) * spd;
    ball_.rest = false;
    ball_.sunk = false;
    ball_.off = false;
    charging_ = false;
    strokes_++;
    rollT_ = 0;
    mode_ = Mode::Roll;
    if (sys_) {
        sys_->apu.noiseBurst(0.22f, 900.f, 0.05f);
        sys_->rumble(0.15f, 0.25f, 40);
    }
}

void Game::openHole() {
    holed_ = true;
    ball_.sunk = true;
    ball_.rest = true;
    ball_.off = false;
    ball_.vx = ball_.vy = 0;
    ball_.x = CUP_X;
    ball_.y = CUP_Y;
    mode_ = Mode::Hole;
    holeT_ = 0.48f;
    chord(392.f, 523.25f, 659.25f, 0.40f);
    if (sys_) {
        sys_->rumble(0.30f, 0.55f, 120);
        sys_->setLight(80, 180, 90);
    }
    say("IN THE CUP", 0.48f);
}

void Game::beginLift() {
    handX_ = CUP_X;
    handY_ = std::min(GH - INSET, CUP_Y + 20.f);
    mode_ = Mode::Lift;
    say("LIFT THE COIN", 0.7f);
}

void Game::tryLift() {
    float d = std::hypot(handX_ - coinX_, handY_ - coinY_);
    if (d > LIFT_R) {
        say("WALK TO THE COIN", 0.45f);
        blip(120.f);
        return;
    }
    handX_ = coinX_;
    handY_ = coinY_;
    finish();
}

void Game::finish() {
    lifted_ = true;
    holed_ = true;
    won_ = true;
    over_ = true;
    mode_ = Mode::Over;
    chord(659.25f, 783.99f, 1046.5f, 0.85f);
    if (sys_) {
        sys_->rumble(0.40f, 0.80f, 200);
        sys_->setLight(255, 210, 80);
    }
}

void Game::fail() {
    won_ = false;
    over_ = true;
    lifted_ = false;
    mode_ = Mode::Over;
    if (sys_) {
        sys_->apu.tone(0, 98.f, 0.10f);
        sys_->setLight(140, 30, 24);
    }
    beep_ = 0.45f;
}

void Game::resolve(Stop s) {
    if (s == Stop::Sunk || ball_.sunk) {
        openHole();
        return;
    }
    if (strokes_ >= MAX_PUTTS) {
        fail();
        return;
    }
    if (s == Stop::Off || ball_.off) {
        ball_.x = coinX_;
        ball_.y = coinY_;
        ball_.vx = ball_.vy = 0;
        ball_.off = false;
        ball_.sunk = false;
        ball_.rest = true;
        solved_ = false;
        mode_ = Mode::Aim;
        say("BACK ON THE MARK", 0.7f);
        blip(90.f);
        return;
    }
    ball_.vx = ball_.vy = 0;
    ball_.rest = true;
    ball_.off = false;
    marked_ = false;
    solved_ = false;
    onBall_ = false;
    mode_ = Mode::Mark;
    say("MARK THE NEW LIE", 0.8f);
}

Game::Input Game::readPad(const gs::Pad& pad) const {
    Input in;
    in.action = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_TURBO);
    in.start = pad.pressed(gs::BTN_START);
    in.back = pad.pressed(gs::BTN_MODE);
    if (pad.down(gs::BTN_LEFT)) in.x -= 1.f;
    if (pad.down(gs::BTN_RIGHT)) in.x += 1.f;
    if (pad.down(gs::BTN_UP)) in.y -= 1.f;
    if (pad.down(gs::BTN_DOWN)) in.y += 1.f;
    if (std::fabs(pad.axisX) > 0.20f) in.x = pad.axisX;
    if (std::fabs(pad.axisY) > 0.20f) in.y = -pad.axisY;
    float len = std::hypot(in.x, in.y);
    if (len > 1.f) {
        in.x /= len;
        in.y /= len;
    }
    return in;
}

Game::Input Game::botInput() const {
    Input in;
    if (mode_ == Mode::Title && clock_ > 0.55f) {
        in.start = true;
        return in;
    }
    if (mode_ == Mode::Mark) {
        float dx = ball_.x - coinX_;
        float dy = ball_.y - coinY_;
        float d = std::hypot(dx, dy);
        if (d < 5.f) in.action = true;
        else if (d > 1e-3f) {
            in.x = dx / d;
            in.y = dy / d;
        }
        return in;
    }
    if (mode_ == Mode::Lift) {
        float dx = coinX_ - handX_;
        float dy = coinY_ - handY_;
        float d = std::hypot(dx, dy);
        if (d < 8.f) in.action = true;
        else if (d > 1e-3f) {
            in.x = dx / d;
            in.y = dy / d;
        }
    }
    return in;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    clock_ += DT;
    if (sayT_ > 0) sayT_ = std::max(0.f, sayT_ - DT);
    if (beep_ > 0) {
        beep_ -= DT;
        if (beep_ <= 0) {
            sys.apu.tone(0, 0, 0);
            sys.apu.tone(1, 0, 0);
            sys.apu.tone(2, 0, 0);
        }
    }

    Input in = readPad(sys.pad);
    if (bot_) in = botInput();

    if (mode_ == Mode::Title) {
        if (in.back && !bot_) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        } else if (in.start || in.action) begin();
    } else if (mode_ == Mode::Pause) {
        if (in.start) mode_ = held_;
        else if (in.back) {
            place();
            won_ = over_ = false;
            mode_ = Mode::Title;
        }
    } else if (mode_ == Mode::Over) {
        if (!bot_ && (in.start || in.action)) begin();
        else if (!bot_ && in.back) {
            place();
            won_ = over_ = false;
            mode_ = Mode::Title;
        }
    } else if ((in.start || in.back) && !bot_) {
        held_ = mode_;
        mode_ = Mode::Pause;
    } else if (mode_ == Mode::Mark) {
        coinX_ += in.x * COIN_V * DT;
        coinY_ += in.y * COIN_V * DT;
        clampGreen(coinX_, coinY_);
        bool on = std::hypot(coinX_ - ball_.x, coinY_ - ball_.y) <= MARK_R;
        if (on && !onBall_) blip(740.f);
        onBall_ = on;
        if (in.action) tryMark();
    } else if (mode_ == Mode::Hold) {
        holdT_ -= DT;
        if (holdT_ <= 0) mode_ = Mode::Aim;
    } else if (mode_ == Mode::Aim) {
        if (bot_) {
            if (!solved_) {
                float a = aim_;
                float v = 140.f;
                if (solve(ball_.x, ball_.y, a, v)) {
                    aim_ = a;
                    spd_ = v;
                    solved_ = true;
                } else {
                    aim_ = std::atan2(CUP_Y - ball_.y, CUP_X - ball_.x);
                    spd_ = 140.f;
                }
            }
            putt(aim_, spd_);
        } else {
            aim_ = wrapAng(aim_ + in.x * 1.15f * DT);
            bool hold = sys.pad.down(gs::BTN_A) || sys.pad.down(gs::BTN_C) || sys.pad.down(gs::BTN_TURBO);
            if (hold && !charging_) {
                charging_ = true;
                meter_ = 0;
                meterDir_ = 1.f;
            }
            if (charging_ && hold) {
                meter_ += meterDir_ * DT / 1.15f;
                if (meter_ >= 1.f) {
                    meter_ = 1.f;
                    meterDir_ = -1.f;
                }
                if (meter_ <= 0.f) {
                    meter_ = 0.f;
                    meterDir_ = 1.f;
                }
            }
            if (charging_ && !hold) putt(aim_, SPD_MIN + meter_ * (SPD_MAX - SPD_MIN));
        }
    } else if (mode_ == Mode::Roll) {
        Stop s = roll(ball_, DT);
        rollT_ += DT;
        if (s != Stop::Moving || rollT_ > 4.f) resolve(s == Stop::Moving ? Stop::Rest : s);
    } else if (mode_ == Mode::Hole) {
        holeT_ -= DT;
        if (holeT_ <= 0) beginLift();
    } else if (mode_ == Mode::Lift) {
        handX_ += in.x * HAND_V * DT;
        handY_ += in.y * HAND_V * DT;
        clampGreen(handX_, handY_);
        if (in.action) tryLift();
    }

    draw();
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool shadow, bool feet) {
    if (!sys_ || h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 40 || s.x + s.w < -40 || s.y > gs::SCREEN_H + 40 || s.y + s.h < -40) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
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
            v.lineBackdrop[y] = gs::rgb4(4 + int(u * 6), 6 + int(u * 4), 12 - int(u * 4));
        } else if (y < 36) {
            v.lineBackdrop[y] = gs::rgb4(2, 5, 2);
        } else if (y < 184) {
            float u = (y - 36) / 148.f;
            int g = 6 + int(u * 6);
            int r = 2 + int(u * 2);
            if (((y / 5) & 1) == 0) g -= 1;
            if (y < 44 || y > 176) g -= 2;
            v.lineBackdrop[y] = gs::rgb4(r, std::clamp(g, 1, 14), 2);
        } else {
            v.lineBackdrop[y] = gs::rgb4(2, 2, 3);
        }
    }

    auto word = [&](const gs::Image& img, float cx, float cy, int pal) {
        gs::Sprite s;
        s.w = int16_t(img.w);
        s.h = int16_t(img.h);
        s.x = int16_t(std::lround(cx - s.w * 0.5f));
        s.y = int16_t(std::lround(cy - s.h * 0.5f));
        s.img = img;
        s.pal = uint8_t(pal);
        v.sprite(s);
    };

    const bool win = mode_ == Mode::Over && won_;
    if (mode_ == Mode::Title) word(art_.logo, 160, 14, PAL_LOGO);
    else if (win) word(art_.finished, 160, 16, PAL_LOGO);
    else if (mode_ == Mode::Over) word(art_.open, 160, 16, PAL_BAD);

    spr(art_.sun, 22, 14, 16, PAL_SUN, false, false);
    const float trees[] = {22.f, 58.f, 262.f, 298.f};
    for (int i = 0; i < 4; i++) spr(art_.tree, trees[i], 40, 26, PAL_TREE, i & 1, false, true);

    if (mode_ == Mode::Title || mode_ == Mode::Mark) {
        bool on = std::hypot(coinX_ - ball_.x, coinY_ - ball_.y) <= MARK_R;
        int pal = on ? PAL_GREEN : PAL_BRASS;
        for (int i = 0; i < 8; i++) {
            float a = clock_ * 0.5f + i * TAU / 8.f;
            spr(art_.dot, OX + ball_.x + std::cos(a) * 14.f, OY + ball_.y + std::sin(a) * 10.f, 4.f, pal, false, false);
        }
    }
    if (mode_ == Mode::Title || mode_ == Mode::Hold || mode_ == Mode::Aim) {
        float dx = std::cos(aim_);
        float dy = std::sin(aim_);
        int n = (mode_ == Mode::Aim && charging_) ? 3 + int(meter_ * 6.f) : 6;
        for (int i = 1; i <= n; i++) spr(art_.dot, OX + ball_.x + dx * i * 11.f, OY + ball_.y + dy * i * 11.f, 4.5f, PAL_AIM, false, false);
    }
    if (mode_ == Mode::Lift || win) spr(art_.glove, OX + handX_, OY + handY_, 16, PAL_HAND, false, false);

    const float tuft[][2] = {{36, 56}, {58, 92}, {214, 52}, {228, 96}, {44, 118}, {206, 128}, {96, 48}, {176, 136}};
    for (int i = 0; i < 8; i++) {
        float x = tuft[i][0];
        float y = tuft[i][1];
        if (std::hypot(x - ball_.x, y - ball_.y) < 16.f) continue;
        if (std::hypot(x - CUP_X, y - CUP_Y) < 18.f) continue;
        if (std::hypot(x - coinX_, y - coinY_) < 14.f) continue;
        spr(art_.tuft, OX + x, OY + y, 11, PAL_GRASS, false, false);
    }
    const float chev[][2] = {{168, 64}, {186, 88}, {204, 112}};
    for (int i = 0; i < 3; i++) spr(art_.chevron, OX + chev[i][0], OY + chev[i][1], 8, PAL_AIM, false, false);

    float ballH = holed_ ? 7.f : 12.f;
    float ballY = OY + ball_.y;
    if (mode_ == Mode::Hold) ballY -= 14.f;
    spr(art_.ball, OX + ball_.x, ballY, ballH, PAL_BALL, false, false);
    if (mode_ == Mode::Title || mode_ == Mode::Mark || mode_ == Mode::Hold || mode_ == Mode::Aim)
        spr(art_.putter, OX + ball_.x - 16.f, OY + ball_.y + 2.f, 8, PAL_PUTTER, false, false);

    float coinDrawX = OX + (lifted_ ? handX_ : coinX_);
    float coinDrawY = OY + (lifted_ ? handY_ - 8.f : coinY_);
    bool coinOn = mode_ == Mode::Mark && std::hypot(coinX_ - ball_.x, coinY_ - ball_.y) <= MARK_R;
    float coinH = coinOn ? 13.f + std::sin(clock_ * 10.f) * 1.4f : 11.f;
    spr(art_.coin, coinDrawX, coinDrawY, coinH, PAL_COIN, false, false);

    const gs::Mipped& flag = holed_ ? art_.flagDown : art_.flag[int(clock_ * 6.f) & 1];
    spr(flag, OX + CUP_X - 2.f, OY + CUP_Y + 2.f, holed_ ? 30.f : 34.f, PAL_FLAG, false, false, true);
    spr(art_.cup, OX + CUP_X, OY + CUP_Y, 16, PAL_CUP, false, false);

    spr(art_.shadow, OX + ball_.x + 2.f, OY + ball_.y + 5.f, 7, PAL_INK, false, true);
    if (!lifted_) spr(art_.shadow, OX + coinX_ + 1.f, OY + coinY_ + 4.f, 6, PAL_INK, false, true);

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(24, "MARK THE BALL, THEN PUTT", PAL_BRASS);
        hudC(25, "THE HOLE OPENS THE MARK", PAL_INK);
        hudC(26, "LIFT THE COIN TO FINISH IT", PAL_GREEN);
        if ((int(clock_ * 2.f) & 1) == 0) {
            hudC(27, "PRESS START", PAL_BRASS);
            hud(1, 27, std::string("V") + S3_VERSION, PAL_INK);
        } else hudC(27, "ARROWS MOVE   Z OR C ACTS", PAL_INK);
    } else if (mode_ == Mode::Pause) {
        hudC(12, "PAUSED", PAL_BRASS);
        hudC(13, "ENTER RESUMES", PAL_INK);
        hudC(14, "ESC TO THE TITLE", PAL_INK);
    } else if (win) {
        hudC(24, "FINISHED MARK", PAL_BRASS);
        hudC(25, "THE COIN IS UP", PAL_GREEN);
        std::snprintf(buf, sizeof buf, "STROKES %d", strokes_);
        hudC(26, buf, PAL_INK);
        if (!bot_) hudC(27, "START PUTTS AGAIN", PAL_BRASS);
        else hudC(27, "LEAVE", PAL_GREEN);
    } else if (mode_ == Mode::Over) {
        hudC(24, "THE MARK STAYS OPEN", PAL_BAD);
        hudC(25, "NO COIN WAS LIFTED", PAL_INK);
        std::snprintf(buf, sizeof buf, "STROKES %d", strokes_);
        hudC(26, buf, PAL_INK);
        hudC(27, "START PUTTS AGAIN", PAL_BRASS);
    } else {
        hud(1, 24, "S3 PUTTMARK", PAL_BRASS);
        const char* state = "NO MARK";
        if (lifted_) state = "FINISHED";
        else if (holed_ || marked_) state = "MARK OPEN";
        hud(31, 24, state, (holed_ || marked_) ? PAL_BRASS : PAL_INK);
        if (sayT_ > 0 && say_ && say_[0]) hudC(25, say_, PAL_GREEN);
        else if (mode_ == Mode::Mark) {
            bool on = std::hypot(coinX_ - ball_.x, coinY_ - ball_.y) <= MARK_R;
            hudC(25, on ? "ON THE BALL" : "SLIDE THE COIN ONTO THE BALL", on ? PAL_GREEN : PAL_INK);
        } else if (mode_ == Mode::Hold) hudC(25, "MARK OPEN", PAL_BRASS);
        else if (mode_ == Mode::Aim) hudC(25, "ARROWS AIM", PAL_INK);
        else if (mode_ == Mode::Roll) hudC(25, "ROLLING", PAL_BRASS);
        else if (mode_ == Mode::Hole) hudC(25, "IN THE CUP", PAL_GREEN);
        else if (mode_ == Mode::Lift) hudC(25, "WALK THE GLOVE TO THE COIN", PAL_INK);

        if (mode_ == Mode::Aim && charging_) {
            int n = int(std::lround(meter_ * 16.f));
            int pct = int(std::lround(meter_ * 100.f));
            std::string bar = "PACE ";
            for (int i = 0; i < 16; i++) bar += (i < n) ? '=' : '.';
            std::snprintf(buf, sizeof buf, " %d", pct);
            bar += buf;
            hud(1, 26, bar, PAL_BRASS);
        } else if (mode_ == Mode::Mark) hudC(26, "Z OR C SETS THE MARK", PAL_INK);
        else if (mode_ == Mode::Aim) hudC(26, "HOLD Z OR C, RELEASE TO PUTT", PAL_INK);
        else if (mode_ == Mode::Lift) hudC(26, "Z OR C LIFTS THE COIN", PAL_GREEN);
        else hudC(26, "UPHILL TO THE CUP, BREAKS RIGHT", PAL_BRASS);

        std::snprintf(buf, sizeof buf, "STROKE %d/%d", strokes_, MAX_PUTTS);
        hud(1, 27, buf, PAL_INK);
        hud(26, 27, "BREAKS RIGHT", PAL_BRASS);
    }
}

}  // namespace puttmark

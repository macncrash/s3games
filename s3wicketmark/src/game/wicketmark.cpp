#include "game/wicketmark.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace wicketmark {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float FEET = 172.f;
constexpr float KEEP_X = 18.f;
constexpr float STUMP_X = 54.f;
constexpr float BAT_X = 102.f;
constexpr float POP_X = 204.f;
constexpr float LEGAL = 16.f;
constexpr float MARK_X = 288.f;
constexpr float MARK_Y = 168.f;
constexpr float COIN0_X = 236.f;
constexpr float COIN0_Y = 180.f;
constexpr float WAIT_X = 204.f;
constexpr float MARK_R = 12.f;
constexpr float LIFT_R = 18.f;
constexpr float RUN_V = 86.f;
constexpr float COIN_V = 100.f;
constexpr float WALK_V = 112.f;
constexpr int BALLS = 3;
constexpr float PI = 3.14159265f;
constexpr float TAU = 6.2831853f;
constexpr float PITCH_L = 8.f;
constexpr float PITCH_T = 124.f;
constexpr float FLIGHT_T = 0.70f;

float clampf(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }

}  // namespace

const char* Game::callName() const {
    switch (call_) {
    case Call::Wicket: return "wicket";
    case Call::Full: return "full";
    case Call::Off: return "off";
    case Call::Leg: return "leg";
    case Call::NoBall: return "no ball";
    default: return "none";
    }
}

const char* Game::banner() const {
    switch (call_) {
    case Call::Wicket: return "WICKET";
    case Call::Full: return "FULL TOSS";
    case Call::Off: return "OUTSIDE OFF";
    case Call::Leg: return "DOWN LEG";
    case Call::NoBall: return "NO BALL";
    default: return "";
    }
}

void Game::place() {
    coinX_ = COIN0_X;
    coinY_ = COIN0_Y;
    bowlerX_ = WAIT_X;
    bowlerY_ = FEET;
    handX0_ = handY0_ = 0;
    line_ = 1;
    balls_ = 0;
    call_ = Call::None;
    onMark_ = opened_ = lifted_ = finished_ = loosed_ = broke_ = ticked_ = nicked_ = false;
    won_ = over_ = false;
    flight_ = setT_ = callT_ = sayT_ = beep_ = lineCool_ = brokeAt_ = 0;
    say_ = "";
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(5, 8, 12));
    sys.apu.setMaster(0.68f);
    place();
    mode_ = Mode::Title;
    clock_ = 0;
}

void Game::say(const char* s, float time) {
    say_ = s ? s : "";
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

void Game::begin() {
    place();
    mode_ = Mode::Mark;
    say("SET THE MARK", 0.7f);
    blip(440.f);
}

void Game::trySet() {
    float d = std::hypot(coinX_ - MARK_X, coinY_ - MARK_Y);
    if (d > MARK_R) {
        say("OFF THE SCRATCH", 0.45f);
        blip(110.f);
        return;
    }
    coinX_ = MARK_X;
    coinY_ = MARK_Y;
    onMark_ = true;
    bowlerX_ = MARK_X;
    bowlerY_ = FEET;
    line_ = 1;
    loosed_ = false;
    ticked_ = false;
    mode_ = Mode::Set;
    setT_ = 0.36f;
    say("MARK SET", 0.36f);
    blip(660.f);
}

bool Game::inWindow() const { return bowlerX_ >= POP_X && bowlerX_ <= POP_X + LEGAL; }

void Game::release() {
    if (loosed_) return;
    loosed_ = true;
    nicked_ = false;
    balls_++;
    handX0_ = bowlerX_ - 6.f;
    handY0_ = FEET - 40.f;
    if (!onMark_ || bowlerX_ < POP_X) call_ = Call::NoBall;
    else if (bowlerX_ > POP_X + LEGAL) call_ = Call::Full;
    else if (line_ == 0) call_ = Call::Off;
    else if (line_ == 2) call_ = Call::Leg;
    else call_ = Call::Wicket;
    flight_ = 0;
    broke_ = false;
    mode_ = Mode::Flight;
    if (sys_) sys_->apu.noiseBurst(0.18f, 700.f, 0.05f);
}

void Game::ballAt(float u, float& x, float& y) const {
    u = clampf(u, 0.f, 1.f);
    float x0 = handX0_;
    float y0 = handY0_;
    auto mix = [](float a, float b, float t) { return a + (b - a) * t; };
    if (call_ == Call::Wicket) {
        float px = 138.f;
        float py = FEET - 4.f;
        if (u < 0.58f) {
            float w = u / 0.58f;
            x = mix(x0, px, w);
            y = mix(y0, py, w) - std::sin(w * PI) * 12.f;
        } else {
            float w = (u - 0.58f) / 0.42f;
            x = mix(px, STUMP_X + 2.f, w);
            y = mix(py, FEET - 28.f, w) - std::sin(w * PI) * 3.f;
        }
        return;
    }
    if (call_ == Call::NoBall) {
        x = mix(x0, STUMP_X - 8.f, u);
        y = mix(y0, FEET - 52.f, u) - std::sin(u * PI) * 16.f;
        return;
    }
    float tx = BAT_X + 6.f;
    float ty = FEET - 32.f;
    if (u < 0.70f) {
        float w = u / 0.70f;
        float arc = call_ == Call::Full ? 24.f : 10.f;
        x = mix(x0, tx, w);
        y = mix(y0, ty, w) - std::sin(w * PI) * arc;
    } else {
        float w = (u - 0.70f) / 0.30f;
        float dx = call_ == Call::Leg ? 48.f : -54.f;
        float dy = call_ == Call::Full ? -26.f : -8.f;
        x = tx + dx * w;
        y = ty + dy * w - std::sin(w * PI) * 8.f;
    }
}

void Game::enterCall() {
    mode_ = Mode::Call;
    callT_ = 0.75f;
    if (call_ == Call::Wicket) {
        opened_ = true;
        say("MARK OPEN", 0.75f);
        chord(196.f, 247.f, 330.f, 0.45f);
        if (sys_) {
            sys_->rumble(0.35f, 0.7f, 140);
            sys_->setLight(220, 170, 40);
        }
    } else {
        say(banner(), 0.7f);
        blip(call_ == Call::NoBall ? 98.f : 180.f);
    }
}

void Game::beginLift() {
    bowlerX_ = POP_X;
    bowlerY_ = FEET;
    mode_ = Mode::Lift;
    say("LIFT THE COIN", 0.8f);
    blip(523.f);
}

void Game::tryLift() {
    if (!opened_ || !onMark_) return;
    float d = std::hypot(bowlerX_ - coinX_, bowlerY_ - coinY_);
    if (d > LIFT_R) {
        say("WALK BACK TO THE MARK", 0.4f);
        blip(120.f);
        return;
    }
    finish();
}

void Game::finish() {
    bowlerX_ = coinX_;
    bowlerY_ = FEET;
    lifted_ = true;
    finished_ = true;
    won_ = true;
    over_ = true;
    mode_ = Mode::Over;
    chord(523.25f, 659.25f, 783.99f, 0.9f);
    if (sys_) {
        sys_->rumble(0.4f, 0.85f, 200);
        sys_->setLight(255, 210, 70);
    }
}

void Game::fail() {
    won_ = false;
    finished_ = false;
    lifted_ = false;
    over_ = true;
    mode_ = Mode::Over;
    if (sys_) {
        sys_->apu.tone(0, 98.f, 0.10f);
        sys_->setLight(140, 36, 28);
    }
    beep_ = 0.45f;
    say("NOT FINISHED", 1.f);
}

void Game::stepRun(const Input& in) {
    if (bot_) line_ = 1;
    else if (lineCool_ > 0) lineCool_ -= DT;
    else {
        int dir = 0;
        if (in.up || in.stick > 0.5f) dir = -1;
        if (in.down || in.stick < -0.5f) dir = 1;
        if (dir) {
            line_ = std::clamp(line_ + dir, 0, 2);
            lineCool_ = 0.16f;
            blip(320.f);
        }
    }
    bowlerX_ -= RUN_V * DT;
    if (loosed_) return;
    if (inWindow() && !ticked_) {
        blip(880.f);
        ticked_ = true;
    }
    if (bot_) {
        if (bowlerX_ <= POP_X + LEGAL * 0.5f && bowlerX_ >= POP_X) release();
        else if (bowlerX_ < POP_X) release();
    } else if (in.action || bowlerX_ < POP_X) release();
}

void Game::stepFlight() {
    flight_ += DT;
    float u = std::min(1.f, flight_ / FLIGHT_T);
    if (call_ == Call::Wicket && !broke_ && u >= 0.93f) {
        broke_ = true;
        brokeAt_ = clock_;
        if (sys_) {
            sys_->apu.noiseBurst(0.42f, 160.f, 0.18f);
            sys_->rumble(0.5f, 0.8f, 90);
        }
        blip(140.f);
    } else if (call_ != Call::Wicket && call_ != Call::NoBall && call_ != Call::None && !nicked_ && u >= 0.68f) {
        nicked_ = true;
        if (sys_) sys_->apu.noiseBurst(0.16f, 1200.f, 0.05f);
    }
    if (flight_ >= FLIGHT_T) enterCall();
}

void Game::stepCall() {
    callT_ -= DT;
    if (callT_ > 0) return;
    if (opened_) beginLift();
    else if (balls_ >= BALLS) fail();
    else {
        bowlerX_ = MARK_X;
        bowlerY_ = FEET;
        loosed_ = false;
        ticked_ = false;
        nicked_ = false;
        broke_ = false;
        line_ = 1;
        mode_ = Mode::Run;
        say("AGAIN", 0.4f);
        blip(180.f);
    }
}

void Game::stepLift(const Input& in) {
    bowlerX_ += in.x * WALK_V * DT;
    bowlerY_ += in.y * WALK_V * DT;
    bowlerX_ = clampf(bowlerX_, 40.f, 308.f);
    bowlerY_ = clampf(bowlerY_, 150.f, 184.f);
    if (in.action) tryLift();
}

Game::Input Game::readPad(const gs::Pad& pad) const {
    Input in;
    in.action = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_TURBO);
    in.start = pad.pressed(gs::BTN_START);
    in.back = pad.pressed(gs::BTN_MODE);
    in.up = pad.pressed(gs::BTN_UP);
    in.down = pad.pressed(gs::BTN_DOWN);
    if (std::fabs(pad.axisX) > 0.20f) in.x = pad.axisX;
    else {
        if (pad.down(gs::BTN_LEFT)) in.x -= 1.f;
        if (pad.down(gs::BTN_RIGHT)) in.x += 1.f;
    }
    if (std::fabs(pad.axisY) > 0.20f) in.y = -pad.axisY;
    else {
        if (pad.down(gs::BTN_UP)) in.y -= 1.f;
        if (pad.down(gs::BTN_DOWN)) in.y += 1.f;
    }
    if (std::fabs(pad.axisY) > 0.55f) in.stick = pad.axisY > 0 ? 1.f : -1.f;
    float len = std::hypot(in.x, in.y);
    if (len > 1.f) {
        in.x /= len;
        in.y /= len;
    }
    return in;
}

Game::Input Game::botInput() const {
    Input in;
    if (mode_ == Mode::Title) {
        if (clock_ > 0.40f) in.start = true;
        return in;
    }
    if (mode_ == Mode::Mark) {
        float dx = MARK_X - coinX_;
        float dy = MARK_Y - coinY_;
        float d = std::hypot(dx, dy);
        if (d < 5.f) in.action = true;
        else if (d > 1e-3f) {
            in.x = dx / d;
            in.y = dy / d;
        }
        return in;
    }
    if (mode_ == Mode::Lift) {
        float dx = coinX_ - bowlerX_;
        float dy = coinY_ - bowlerY_;
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

    Input in = bot_ ? botInput() : readPad(sys.pad);
    if (mode_ == Mode::Title) {
        if (in.back && !bot_) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        } else if (in.start || in.action) begin();
    } else if (mode_ == Mode::Pause) {
        if (in.start) mode_ = held_;
        else if (in.back) {
            place();
            mode_ = Mode::Title;
        }
    } else if (mode_ == Mode::Over) {
        if (!bot_ && (in.start || in.action)) begin();
        else if (!bot_ && in.back) {
            place();
            mode_ = Mode::Title;
        }
    } else if (!bot_ && (in.start || in.back)) {
        held_ = mode_;
        mode_ = Mode::Pause;
    } else if (mode_ == Mode::Mark) {
        coinX_ = clampf(coinX_ + in.x * COIN_V * DT, 24.f, 308.f);
        coinY_ = clampf(coinY_ + in.y * COIN_V * DT, 148.f, 180.f);
        if (in.action) trySet();
    } else if (mode_ == Mode::Set) {
        setT_ -= DT;
        if (setT_ <= 0) {
            mode_ = Mode::Run;
            loosed_ = false;
            ticked_ = false;
            line_ = 1;
            bowlerX_ = MARK_X;
            bowlerY_ = FEET;
        }
    } else if (mode_ == Mode::Run) stepRun(in);
    else if (mode_ == Mode::Flight) stepFlight();
    else if (mode_ == Mode::Call) stepCall();
    else if (mode_ == Mode::Lift) stepLift(in);

    draw();
}

void Game::spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool flip, bool shade, bool feet) {
    if (!sys_ || img.w == 0 || w < 1.f || h < 1.f) return;
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - h : cy - h * 0.5f));
    s.img = img;
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shade;
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
    if (!s) return;
    int n = int(std::strlen(s));
    hud(20 - n / 2, row, s, pal);
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        if (y < 86) {
            float u = y / 85.f;
            int r = 3 + int(u * 6);
            int g = 5 + int(u * 6);
            int b = 10 + int(u * 3);
            v.lineBackdrop[y] = gs::rgb4(std::min(r, 15), std::min(g, 15), std::min(b, 15));
        } else if (y < 124) {
            int s = (y / 4) & 1;
            v.lineBackdrop[y] = gs::rgb4(2, 6 + s, 2);
        } else if (y < 180) {
            int s = (y / 5) & 1;
            v.lineBackdrop[y] = gs::rgb4(2, 8 + s, 2);
        } else {
            v.lineBackdrop[y] = gs::rgb4(1, 6, 2);
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

    const Mode view = mode_ == Mode::Pause ? held_ : mode_;
    const bool title = view == Mode::Title;
    const bool liveBall = view == Mode::Run || view == Mode::Set || title;
    float bowX = bowlerX_;
    float bowY = bowlerY_;
    if (title || view == Mode::Mark) {
        bowX = WAIT_X;
        bowY = FEET;
    }

    auto word = [&](const gs::Image& img, float y, int pal) {
        spr(img, 160.f, y, float(img.w), float(img.h), pal);
    };
    if (title) word(art_.logo, 20.f, PAL_TITLE);
    else if (mode_ == Mode::Over && won_) word(art_.finished, 24.f, PAL_GOLD);
    else if (view == Mode::Call && call_ == Call::Wicket) word(art_.wicketWord, 28.f, PAL_GOLD);

    bool showBall = false;
    float bx = 0, by = 0;
    if (liveBall || view == Mode::Mark) {
        showBall = true;
        bx = bowX - 8.f;
        by = FEET - 40.f;
        if (title) by += std::sin(clock_ * 4.f) * 2.f;
    } else if (view == Mode::Flight || view == Mode::Call) {
        showBall = true;
        float u = view == Mode::Flight ? std::min(1.f, flight_ / FLIGHT_T) : 1.f;
        ballAt(u, bx, by);
    } else if ((view == Mode::Lift || (mode_ == Mode::Over && won_)) && opened_) {
        showBall = true;
        bx = STUMP_X + 8.f;
        by = FEET - 6.f;
    }
    if (showBall) {
        int seam = (int(clock_ * 12.f) & 1);
        spr(art_.ball[seam], bx, by, 13.f, 13.f, PAL_BALL);
    }

    if (broke_) {
        float w = std::min(1.f, std::max(0.f, clock_ - brokeAt_) / 0.45f);
        float rise = std::sin(w * PI) * 18.f;
        float fall = (FEET - 8.f - 128.f) * w;
        float byb = 128.f + fall - rise;
        spr(art_.bail, STUMP_X - 8.f - w * 14.f, byb, 10.f, 4.f, PAL_WOOD);
        spr(art_.bail, STUMP_X + 8.f + w * 16.f, byb - 2.f, 10.f, 4.f, PAL_WOOD, true);
    }

    float cx = coinX_;
    float cy = coinY_;
    if (title) cy += std::sin(clock_ * 3.f) * 1.6f;
    if (lifted_) {
        cx = bowlerX_;
        cy = bowlerY_ - 46.f + std::sin(clock_ * 6.f) * 1.2f;
    }
    bool nearCoin = std::hypot(coinX_ - MARK_X, coinY_ - MARK_Y) <= MARK_R;
    float coinH = (view == Mode::Mark && nearCoin) ? 15.f + std::sin(clock_ * 10.f) * 1.4f : 13.f;
    spr(art_.coin, cx, cy, coinH, coinH, PAL_COIN);

    int pose = 0;
    if (view == Mode::Run || view == Mode::Lift || title) pose = (int(clock_ * (title ? 4.f : 9.f)) & 1) ? 1 : 0;
    if (view == Mode::Flight || view == Mode::Call) pose = 2;
    float bowH = 52.f;
    spr(art_.bowler[pose], bowX, bowY, bowH * 34.f / 54.f, bowH, PAL_BOWL, false, false, true);

    int batPose = 0;
    if ((view == Mode::Flight || view == Mode::Call) && call_ != Call::Wicket && call_ != Call::NoBall &&
        call_ != Call::None) {
        if (view != Mode::Flight || flight_ > 0.48f) batPose = 1;
    }
    spr(art_.batsman[batPose], BAT_X, FEET, 36.f, 54.f, PAL_WHITE, false, false, true);
    spr(art_.keeper, KEEP_X, FEET + 2.f, 26.f, 40.f, PAL_KEEP, false, false, true);
    spr(art_.stump[broke_ ? 1 : 0], STUMP_X, FEET, 24.f, 44.f, PAL_WOOD, false, false, true);

    int creasePal = (view == Mode::Run && inWindow()) ? PAL_GREEN : PAL_INK;
    spr(art_.crease, POP_X, FEET + 2.f, 3.f, 46.f, creasePal, false, false, true);
    spr(art_.crease, STUMP_X + 16.f, FEET + 2.f, 3.f, 46.f, PAL_INK, false, false, true);
    spr(art_.scratch, MARK_X, MARK_Y + 4.f, 18.f, 10.f, PAL_COIN);

    bool ring = title || view == Mode::Mark || view == Mode::Lift;
    if (ring) {
        float rdx = view == Mode::Lift ? coinX_ : MARK_X;
        float rdy = view == Mode::Lift ? coinY_ : MARK_Y;
        bool hot = view == Mode::Lift ? std::hypot(bowlerX_ - coinX_, bowlerY_ - coinY_) <= LIFT_R : nearCoin;
        int pal = hot ? PAL_GREEN : PAL_GOLD;
        for (int i = 0; i < 8; i++) {
            float a = clock_ * 1.7f + i * TAU / 8.f;
            float rad = hot ? 14.f : 18.f;
            spr(art_.dot, rdx + std::cos(a) * rad, rdy + std::sin(a) * 7.f, 4.f, 4.f, pal);
        }
    }

    spr(art_.shadow, bowX, bowY + 3.f, 22.f, 7.f, PAL_SHADE, false, true);
    spr(art_.shadow, BAT_X, FEET + 3.f, 20.f, 7.f, PAL_SHADE, false, true);
    if (!lifted_) spr(art_.shadow, coinX_, coinY_ + 6.f, 12.f, 5.f, PAL_SHADE, false, true);

    spr(art_.pitch, PITCH_L + art_.pitch.w * 0.5f, PITCH_T + art_.pitch.h * 0.5f, float(art_.pitch.w),
        float(art_.pitch.h), PAL_PITCH);
    spr(art_.screen, 304.f, 118.f, 18.f, 32.f, PAL_STAND, false, false, true);
    spr(art_.stand, 160.f, 118.f, 78.f, 40.f, PAL_STAND, false, false, true);
    spr(art_.crowd, 78.f, 116.f, 64.f, 16.f, PAL_STAND, false, false, true);
    spr(art_.crowd, 236.f, 116.f, 60.f, 16.f, PAL_STAND, true, false, true);
    const float trees[] = {24.f, 64.f, 196.f, 302.f};
    for (int i = 0; i < 4; i++) spr(art_.tree, trees[i], 122.f, 28.f, 42.f, PAL_GRASS, i & 1, false, true);
    const float tufts[][2] = {{40.f, 112.f}, {120.f, 108.f}, {210.f, 110.f}, {270.f, 108.f}};
    for (int i = 0; i < 4; i++) spr(art_.tuft, tufts[i][0], tufts[i][1], 12.f, 12.f, PAL_GRASS, false, false, true);
    float drift = std::fmod(clock_ * 6.f, 160.f);
    spr(art_.cloud, 36.f + drift, 18.f, 34.f, 14.f, PAL_SKY);
    spr(art_.cloud, 210.f - drift * 0.35f, 30.f, 28.f, 12.f, PAL_SKY);
    spr(art_.sun, 28.f, 18.f, 16.f, 16.f, PAL_SKY);

    char buf[48];
    if (mode_ == Mode::Pause) {
        hud(1, 0, "S3 WICKETMARK", PAL_TITLE);
        hudC(11, "PAUSED", PAL_GOLD);
        hudC(13, "ENTER RESUMES", PAL_INK);
        hudC(14, "ESC TO THE TITLE", PAL_INK);
        return;
    }
    if (title) {
        hudC(24, "SET THE COIN ON THE SCRATCH", PAL_GOLD);
        hudC(25, "A LEGAL WICKET OPENS THE MARK", PAL_INK);
        hudC(26, "LIFT THE COIN TO FINISH IT", PAL_GREEN);
        if ((int(clock_ * 2.f) & 1) == 0) hudC(27, "PRESS START", PAL_GOLD);
        else hudC(27, "ARROWS MOVE   Z OR C ACTS", PAL_INK);
        return;
    }

    hud(1, 0, "S3 WICKETMARK", PAL_TITLE);
    std::snprintf(buf, sizeof buf, "BALL %d/%d", std::min(std::max(balls_, view == Mode::Run && !loosed_ ? balls_ + 1 : balls_), BALLS),
                  BALLS);
    hud(40 - int(std::strlen(buf)) - 1, 0, buf, PAL_INK);
    const char* state = "NO MARK";
    int statePal = PAL_INK;
    if (lifted_ || (mode_ == Mode::Over && won_)) {
        state = "FINISHED";
        statePal = PAL_GOLD;
    } else if (opened_) {
        state = "MARK OPEN";
        statePal = PAL_GREEN;
    } else if (onMark_) {
        state = "MARK SET";
        statePal = PAL_GOLD;
    }
    hud(40 - int(std::strlen(state)) - 1, 1, state, statePal);

    if (mode_ == Mode::Over && won_) {
        hudC(24, "FINISHED MARK", PAL_GOLD);
        hudC(25, "THE COIN IS UP", PAL_GREEN);
        std::snprintf(buf, sizeof buf, "BALLS %d", balls_);
        hudC(26, buf, PAL_INK);
        if (!bot_) hudC(27, "START BOWLS AGAIN", PAL_GOLD);
        else hudC(27, "LEAVE", PAL_GREEN);
        return;
    }
    if (mode_ == Mode::Over) {
        hudC(24, "NOT FINISHED", PAL_BAD);
        hudC(25, "THE COIN STAYS DOWN", PAL_INK);
        std::snprintf(buf, sizeof buf, "BALLS %d  %s", balls_, banner());
        hudC(26, buf, PAL_BAD);
        hudC(27, "START TRIES AGAIN", PAL_GOLD);
        return;
    }

    if (sayT_ > 0 && say_ && say_[0]) hudC(2, say_, call_ == Call::Wicket || opened_ ? PAL_GOLD : PAL_INK);

    if (view == Mode::Mark) {
        hudC(24, nearCoin ? "ON THE SCRATCH" : "SLIDE THE COIN ONTO THE SCRATCH", nearCoin ? PAL_GREEN : PAL_INK);
        hudC(25, "Z OR C SETS THE MARK", PAL_GOLD);
        hudC(26, "ARROWS MOVE THE COIN", PAL_INK);
        hudC(27, "THREE BALLS  MIDDLE IS THE WICKET", PAL_INK);
    } else if (view == Mode::Set) {
        hudC(24, "MARK SET", PAL_GOLD);
        hudC(25, "RUN IN", PAL_GREEN);
        hudC(26, "RELEASE AS THE CREASE TURNS GREEN", PAL_INK);
    } else if (view == Mode::Run) {
        hud(1, 24, "OFF", line_ == 0 ? PAL_BAD : PAL_INK);
        hud(8, 24, "MIDDLE", line_ == 1 ? PAL_GREEN : PAL_INK);
        hud(18, 24, "LEG", line_ == 2 ? PAL_BAD : PAL_INK);
        const int N = 18;
        float span1 = POP_X - 20.f;
        float denom = MARK_X - span1;
        auto slot = [&](float x) {
            float f = (MARK_X - x) / denom;
            return std::clamp(int(f * N), 0, N - 1);
        };
        int i0 = slot(POP_X + LEGAL);
        int i1 = slot(POP_X);
        int ip = slot(bowlerX_);
        if (i0 > i1) std::swap(i0, i1);
        char bar[24];
        int n = 0;
        bar[n++] = 'R';
        bar[n++] = 'U';
        bar[n++] = 'N';
        bar[n++] = ' ';
        for (int i = 0; i < N; i++) {
            char c = '.';
            if (i >= i0 && i <= i1) c = '#';
            if (i == ip) c = 'O';
            bar[n++] = c;
        }
        bar[n] = 0;
        hud(1, 25, bar, inWindow() ? PAL_GREEN : PAL_INK);
        if (inWindow()) hudC(26, (int(clock_ * 8.f) & 1) ? "RELEASE" : "Z AT THE CREASE", PAL_GREEN);
        else hudC(26, "Z AT THE CREASE", PAL_INK);
        hudC(27, "UP OFF   DOWN LEG   MIDDLE HITS", PAL_INK);
    } else if (view == Mode::Flight || view == Mode::Call) {
        if (call_ != Call::Wicket) hudC(24, banner(), PAL_BAD);
        else hudC(24, "BOWLED", PAL_GOLD);
        if (opened_) hudC(26, "WALK BACK AND LIFT THE COIN", PAL_GREEN);
    } else if (view == Mode::Lift) {
        bool hot = std::hypot(bowlerX_ - coinX_, bowlerY_ - coinY_) <= LIFT_R;
        hudC(24, hot ? "ON THE COIN" : "WALK BACK TO THE MARK", hot ? PAL_GREEN : PAL_INK);
        hudC(25, "Z OR C LIFTS THE COIN", PAL_GOLD);
        hudC(26, "ARROWS MOVE", PAL_INK);
        hudC(27, "THAT FINISHES THE MARK", PAL_GREEN);
    }
}

}  // namespace wicketmark

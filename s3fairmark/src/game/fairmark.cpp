#include "game/fairmark.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

#include "version.h"

namespace fairmark {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float PI = 3.14159265f;
constexpr float GOLD_X = 160.f;
constexpr float BOTTLE_X[3] = {96.f, 160.f, 224.f};
constexpr int GOLD = 1;
constexpr float FEET = 164.f;
constexpr float SEAT_Y = 102.f;
constexpr float HANG_Y = 90.f;
constexpr float NECK_Y = 120.f;
constexpr float PIVOT_Y = 70.f;
constexpr float AMP = 64.f;
constexpr float OMEGA = 1.85f;
constexpr float HIT = 13.f;
constexpr float NUDGE = 18.f;
constexpr float MARK_R = 16.f;
constexpr float LIFT_R = 14.f;
constexpr float COIN_V = 112.f;
constexpr float HAND_V = 132.f;
constexpr float FLIGHT = 0.34f;
constexpr float OPEN_T = 0.48f;
constexpr int RINGS = 3;
constexpr int BOTTLE_PAL[3] = {PAL_RED, PAL_GOLD, PAL_BLUE};

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

const char* Game::phase() const {
    switch (mode_) {
        case Mode::Title: return "title";
        case Mode::Mark: return "mark";
        case Mode::Aim: return "aim";
        case Mode::Flight: return "flight";
        case Mode::Open: return "open";
        case Mode::Lift: return "lift";
        case Mode::Pause: return "pause";
        case Mode::Over: return won_ ? "finished" : "open";
    }
    return "?";
}

void Game::place() {
    coinX_ = 124.f;
    coinY_ = 172.f;
    handX_ = 236.f;
    handY_ = 176.f;
    swingT_ = 0;
    nudge_ = 0;
    landX_ = GOLD_X;
    flight_ = 0;
    openT_ = 0;
    sayT_ = 0;
    say_ = "";
    thrown_ = 0;
    landBottle_ = -1;
    marked_ = opened_ = lifted_ = finished_ = false;
    onGold_ = wasSweet_ = false;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.setFogColor(gs::rgb4(3, 1, 6));
    sys.apu.setMaster(0.65f);
    place();
    won_ = over_ = false;
    mode_ = Mode::Title;
    clock_ = 0;
}

void Game::begin() {
    place();
    won_ = over_ = false;
    mode_ = Mode::Mark;
    blip(340.f);
}

void Game::toTitle() {
    place();
    won_ = over_ = false;
    mode_ = Mode::Title;
}

void Game::say(const char* s, float time) {
    say_ = s;
    sayT_ = time;
}

void Game::blip(float freq) {
    if (!sys_) return;
    sys_->apu.tone(0, freq, 0.07f);
    beep_ = std::max(beep_, 0.08f);
}

void Game::chord(float a, float b, float c, float hold) {
    if (!sys_) return;
    sys_->apu.tone(0, a, 0.09f);
    sys_->apu.tone(1, b, 0.07f);
    sys_->apu.tone(2, c, 0.06f);
    beep_ = hold;
}

float Game::hangX() const {
    float t = (mode_ == Mode::Aim) ? swingT_ : clock_;
    float n = (mode_ == Mode::Aim) ? nudge_ : 0.f;
    return GOLD_X + AMP * std::sin(t * OMEGA) + n;
}

int Game::bottleAt(float x) const {
    int best = -1;
    float bestD = HIT;
    for (int i = 0; i < 3; i++) {
        float d = std::fabs(x - BOTTLE_X[i]);
        if (d <= bestD) {
            bestD = d;
            best = i;
        }
    }
    return best;
}

void Game::tryMark() {
    float gx = coinX_ - GOLD_X;
    float gy = coinY_ - SEAT_Y;
    float dg = std::hypot(gx, gy);
    if (dg <= MARK_R) {
        coinX_ = GOLD_X;
        coinY_ = SEAT_Y;
        marked_ = true;
        swingT_ = 0;
        nudge_ = 0;
        wasSweet_ = true;
        mode_ = Mode::Aim;
        chord(392.f, 523.f, 659.f, 0.28f);
        say("MARK SET", 0.7f);
        return;
    }
    for (int i = 0; i < 3; i++) {
        if (i == GOLD) continue;
        if (std::hypot(coinX_ - BOTTLE_X[i], coinY_ - SEAT_Y) < 22.f &&
            std::hypot(coinX_ - BOTTLE_X[i], coinY_ - SEAT_Y) < dg) {
            say("ONLY THE GOLD BOTTLE", 0.7f);
            blip(110.f);
            return;
        }
    }
    say("SET IT ON THE GOLD", 0.7f);
    blip(110.f);
}

void Game::release() {
    if (thrown_ >= RINGS || opened_) return;
    landX_ = hangX();
    flight_ = FLIGHT;
    thrown_++;
    mode_ = Mode::Flight;
    blip(440.f);
}

void Game::resolve() {
    landBottle_ = bottleAt(landX_);
    if (landBottle_ == GOLD) {
        opened_ = true;
        openT_ = OPEN_T;
        mode_ = Mode::Open;
        chord(523.f, 784.f, 1046.f, 0.4f);
        if (sys_) {
            sys_->rumble(0.35f, 0.55f, 90);
            sys_->setLight(255, 190, 60);
        }
        say("MARK OPEN", 0.6f);
        return;
    }
    if (landBottle_ >= 0) say("SIDE BOTTLE", 0.7f);
    else say("MISS", 0.55f);
    if (sys_) sys_->apu.noiseBurst(0.18f, 240.f, 0.12f);
    if (thrown_ >= RINGS) fail();
    else {
        swingT_ = 0.85f;
        mode_ = Mode::Aim;
    }
}

void Game::beginLift() {
    handX_ = 248.f;
    handY_ = 178.f;
    mode_ = Mode::Lift;
    say("LIFT THE COIN", 0.8f);
}

void Game::tryLift() {
    if (!opened_ || lifted_) return;
    float d = std::hypot(handX_ - coinX_, handY_ - coinY_);
    if (d > LIFT_R) {
        say("REACH THE COIN", 0.55f);
        blip(120.f);
        return;
    }
    lifted_ = true;
    finished_ = true;
    won_ = true;
    over_ = true;
    mode_ = Mode::Over;
    chord(392.f, 523.f, 784.f, 0.7f);
    if (sys_) {
        sys_->rumble(0.4f, 0.75f, 180);
        sys_->setLight(255, 210, 80);
    }
    say("FINISHED MARK", 1.2f);
}

void Game::fail() {
    if (over_) return;
    won_ = false;
    finished_ = false;
    over_ = true;
    mode_ = Mode::Over;
    blip(90.f);
    say("STILL OPEN", 1.f);
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
    if (std::fabs(pad.axisX) > 0.2f) in.x = pad.axisX;
    if (std::fabs(pad.axisY) > 0.2f) in.y = -pad.axisY;
    float len = std::hypot(in.x, in.y);
    if (len > 1.f) {
        in.x /= len;
        in.y /= len;
    }
    return in;
}

Game::Input Game::botInput() const {
    Input in;
    if (mode_ == Mode::Title && clock_ > 0.4f) {
        in.start = true;
        return in;
    }
    if (mode_ == Mode::Mark) {
        float dx = GOLD_X - coinX_;
        float dy = SEAT_Y - coinY_;
        float d = std::hypot(dx, dy);
        if (d <= MARK_R) in.action = true;
        else if (d > 1e-3f) {
            in.x = dx / d;
            in.y = dy / d;
        }
        return in;
    }
    if (mode_ == Mode::Aim) {
        if (std::fabs(hangX() - GOLD_X) <= 8.f) in.action = true;
        return in;
    }
    if (mode_ == Mode::Lift) {
        float dx = coinX_ - handX_;
        float dy = coinY_ - handY_;
        float d = std::hypot(dx, dy);
        if (d <= LIFT_R - 2.f) in.action = true;
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
        else if (in.back) toTitle();
    } else if (mode_ == Mode::Over) {
        if (!bot_ && (in.start || in.action)) begin();
        else if (!bot_ && in.back) toTitle();
    } else if ((in.start || in.back) && !bot_) {
        held_ = mode_;
        mode_ = Mode::Pause;
    } else if (mode_ == Mode::Mark) {
        coinX_ = clampf(coinX_ + in.x * COIN_V * DT, 16.f, 304.f);
        coinY_ = clampf(coinY_ + in.y * COIN_V * DT, 86.f, 186.f);
        bool on = std::hypot(coinX_ - GOLD_X, coinY_ - SEAT_Y) <= MARK_R;
        if (on && !onGold_) blip(720.f);
        onGold_ = on;
        if (in.action) tryMark();
    } else if (mode_ == Mode::Aim) {
        nudge_ = in.x * NUDGE;
        bool sweet = std::fabs(hangX() - GOLD_X) <= 10.f;
        if (sweet && !wasSweet_) blip(860.f);
        wasSweet_ = sweet;
        if (in.action) release();
        else swingT_ += DT;
    } else if (mode_ == Mode::Flight) {
        flight_ -= DT;
        if (flight_ <= 0) resolve();
    } else if (mode_ == Mode::Open) {
        openT_ -= DT;
        if (openT_ <= 0) beginLift();
    } else if (mode_ == Mode::Lift) {
        handX_ = clampf(handX_ + in.x * HAND_V * DT, 16.f, 304.f);
        handY_ = clampf(handY_ + in.y * HAND_V * DT, 80.f, 190.f);
        if (in.action) tryLift();
    }

    draw();
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool shadow, bool feet, int fog) {
    if (!sys_ || h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 48 || s.x + s.w < -48 || s.y > gs::SCREEN_H + 48 || s.y + s.h < -48) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, int fog, bool shadow) {
    if (!sys_ || w < 1.f || h < 1.f || m.h < 1) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(std::max(w, h));
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::word(const gs::Image& img, float cx, float cy, int pal) {
    if (!sys_ || img.w < 1) return;
    gs::Sprite s;
    s.w = int16_t(img.w);
    s.h = int16_t(img.h);
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = img;
    s.pal = uint8_t(pal);
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

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        if (y < 52) {
            float u = y / 52.f;
            v.lineBackdrop[y] = gs::rgb4(2 + int(u * 2), 1, 6 + int((1.f - u) * 3));
        } else if (y < 96) {
            float u = (y - 52) / 44.f;
            v.lineBackdrop[y] = gs::rgb4(4 + int(u * 6), 2 + int(u * 3), 7 - int(u * 3));
        } else if (y < 168) {
            v.lineBackdrop[y] = gs::rgb4(5, 2, 3);
        } else {
            int g = 4 + ((y / 4) & 1);
            v.lineBackdrop[y] = gs::rgb4(g + 2, g, 2);
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

    const bool win = mode_ == Mode::Over && won_;
    const bool hanging = mode_ == Mode::Title || mode_ == Mode::Mark || mode_ == Mode::Aim || mode_ == Mode::Pause;
    float hx = hangX();
    float hy = HANG_Y;
    if (mode_ == Mode::Flight) {
        float u = 1.f - clampf(flight_ / FLIGHT, 0.f, 1.f);
        hx = landX_;
        hy = HANG_Y + (NECK_Y - HANG_Y) * u * u;
    }

    if (mode_ == Mode::Title) word(art_.logo, 160, 16, PAL_BRASS);
    else if (win) word(art_.finished, 160, 16, PAL_GOOD);
    else if (mode_ == Mode::Over) word(art_.open, 160, 16, PAL_BAD);

    if (mode_ == Mode::Lift || win) {
        float hyy = handY_ - (lifted_ ? 12.f : 0.f);
        spr(art_.hand, handX_, hyy, 18, PAL_HAND);
    }

    float coinX = lifted_ ? handX_ : coinX_;
    float coinY = lifted_ ? handY_ - 22.f : coinY_;
    bool coinHot = mode_ == Mode::Mark && onGold_;
    float coinH = coinHot ? 15.f + std::sin(clock_ * 10.f) * 1.6f : 13.f;
    spr(art_.coin, coinX, coinY, coinH, PAL_GOLD);
    if (!lifted_) spr(art_.shadow, coinX + 1.f, coinY + 7.f, 7, PAL_INK, false, true);

    if (opened_) {
        float bob = 1.f + (mode_ == Mode::Open ? std::sin(openT_ * 18.f) * 0.08f : 0.f);
        spr(art_.ring, GOLD_X, NECK_Y, 20.f * bob, PAL_RING);
    } else if (mode_ == Mode::Over && landBottle_ >= 0) {
        spr(art_.ring, BOTTLE_X[landBottle_], NECK_Y, 20, PAL_RING);
    } else if (mode_ == Mode::Over) {
        spr(art_.ring, landX_, 176.f, 16, PAL_RING);
    } else if (hanging || mode_ == Mode::Flight) {
        spr(art_.ring, hx, hy, mode_ == Mode::Flight ? 18.f : 16.f, PAL_RING);
    }

    if (hanging) {
        float x0 = GOLD_X, y0 = PIVOT_Y, x1 = hx, y1 = hy;
        for (int i = 1; i <= 6; i++) {
            float u = i / 7.f;
            spr(art_.dot, x0 + (x1 - x0) * u, y0 + (y1 - y0) * u, 3.f, PAL_RING);
        }
    }

    if (mode_ == Mode::Mark || mode_ == Mode::Title) {
        int pal = onGold_ ? PAL_GOOD : PAL_BRASS;
        for (int i = 0; i < 6; i++) {
            float a = clock_ * 1.4f + i * PI / 3.f;
            spr(art_.dot, GOLD_X + std::cos(a) * 16.f, SEAT_Y + std::sin(a) * 10.f, 4.f, pal);
        }
    }

    float starY = 78.f + std::sin(clock_ * 3.f) * 2.f;
    spr(art_.star, GOLD_X, starY, 11, PAL_BRASS);

    for (int i = 0; i < 3; i++) {
        spr(art_.shadow, BOTTLE_X[i], FEET - 2.f, 8, PAL_INK, false, true);
        spr(art_.bottle, BOTTLE_X[i], FEET, i == GOLD ? 60.f : 56.f, BOTTLE_PAL[i], false, false, true);
    }

    for (int i = 0; i < 9; i++) {
        float x = 72.f + i * 22.f;
        bool on = ((i + int(clock_ * 7.f)) % 3) != 0;
        spr(art_.bulb, x, 82.f, on ? 9.f : 6.f, PAL_BULB);
    }

    spr(art_.kid, 36.f, 186.f, 36, PAL_KID, false, false, true);

    stamp(art_.shelf, 160, 170, 196, 14, PAL_WOOD);
    stamp(art_.post, 54, 124, 18, 112, PAL_WOOD);
    stamp(art_.post, 266, 124, 18, 112, PAL_WOOD);
    word(art_.sign, 160, 58, PAL_PAPER);
    stamp(art_.awning, 160, 64, 236, 34, PAL_AWN);
    stamp(art_.cloth, 160, 126, 200, 78, PAL_RED);

    const int buntPal[3] = {PAL_RED, PAL_GOLD, PAL_BLUE};
    for (int i = 0; i < 13; i++) {
        float x = 14.f + i * 24.f;
        float y = 30.f + ((i & 1) ? 3.f : 0.f);
        spr(art_.pennant, x, y, 14, buntPal[i % 3]);
        spr(art_.dot, x, 22.f, 3.f, PAL_BRASS);
    }

    spr(art_.wheel, 36, 86, 62, PAL_NIGHT, false, false, false, 7);
    spr(art_.moon, 292, 26, 16, PAL_BULB, false, false, false, 2);
    const float stars[][2] = {{18, 12}, {70, 18}, {120, 10}, {188, 14}, {240, 8}, {268, 20}, {140, 22}, {210, 16}};
    for (int i = 0; i < 8; i++) {
        if ((int(clock_ * 2.f + i) & 3) == 0) continue;
        spr(art_.dot, stars[i][0], stars[i][1], 3.f, PAL_BULB, false, false, false, 1);
    }
    stamp(art_.awning, 28, 108, 40, 12, PAL_AWN, 9);
    stamp(art_.awning, 300, 112, 34, 10, PAL_AWN, 10);

    if (win) {
        for (int i = 0; i < 8; i++) {
            float a = clock_ * 2.2f + i * PI / 4.f;
            float rad = 28.f + (i & 1) * 10.f;
            int pal = buntPal[i % 3];
            spr(art_.dot, 160 + std::cos(a) * rad, 48 + std::sin(a) * 8.f, 4.f, pal);
        }
    }

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(23, "THE COIN IS THE MARK", PAL_BRASS);
        hudC(24, "SET IT ON THE GOLD BOTTLE", PAL_INK);
        hudC(25, "A RING THERE OPENS THE MARK", PAL_INK);
        hudC(26, "LIFT THE COIN TO FINISH IT", PAL_GOOD);
        if ((int(clock_ * 2.f) & 1) == 0) {
            hudC(27, "PRESS START", PAL_BRASS);
            hud(1, 27, std::string("V") + S3_VERSION, PAL_INK);
        } else {
            hudC(27, "ARROWS MOVE   Z OR C ACTS", PAL_INK);
        }
    } else if (mode_ == Mode::Pause) {
        hudC(12, "PAUSED", PAL_BRASS);
        hudC(13, "ENTER RESUMES", PAL_INK);
        hudC(14, "ESC TO THE TITLE", PAL_INK);
    } else if (win) {
        hudC(23, "FINISHED MARK", PAL_BRASS);
        hudC(24, "RING ON THE GOLD BOTTLE", PAL_GOOD);
        hudC(25, "THE COIN IS UP", PAL_GOOD);
        std::snprintf(buf, sizeof buf, "RINGS %d", thrown_);
        hudC(26, buf, PAL_INK);
        hudC(27, bot_ ? "LEAVE" : "START PLAYS AGAIN", bot_ ? PAL_GOOD : PAL_BRASS);
    } else if (mode_ == Mode::Over) {
        hudC(23, "THE MARK STAYS OPEN", PAL_BAD);
        hudC(24, "NO COIN WAS LIFTED", PAL_INK);
        std::snprintf(buf, sizeof buf, "RINGS %d", thrown_);
        hudC(25, buf, PAL_INK);
        hudC(26, "THE REST OF THE MIDWAY IS NOT THE JOB", PAL_BRASS);
        hudC(27, "START TRIES AGAIN", PAL_BRASS);
    } else {
        hud(1, 23, "S3 FAIRMARK", PAL_BRASS);
        const char* state = "NO MARK";
        int statePal = PAL_INK;
        if (lifted_) {
            state = "FINISHED";
            statePal = PAL_GOOD;
        } else if (opened_) {
            state = "MARK OPEN";
            statePal = PAL_BRASS;
        } else if (marked_) {
            state = "MARK SET";
            statePal = PAL_BRASS;
        }
        hud(30, 23, state, statePal);
        if (sayT_ > 0 && say_ && say_[0]) hudC(24, say_, opened_ ? PAL_GOOD : PAL_BRASS);
        else if (mode_ == Mode::Mark) hudC(24, onGold_ ? "ON THE GOLD BOTTLE" : "SLIDE THE COIN ONTO THE GOLD", onGold_ ? PAL_GOOD : PAL_INK);
        else if (mode_ == Mode::Aim) hudC(24, "RELEASE WHEN THE RING COVERS GOLD", PAL_INK);
        else if (mode_ == Mode::Flight) hudC(24, "RING AWAY", PAL_BRASS);
        else if (mode_ == Mode::Open) hudC(24, "MARK OPEN", PAL_GOOD);
        else if (mode_ == Mode::Lift) hudC(24, "WALK THE HAND TO THE COIN", PAL_INK);

        if (mode_ == Mode::Mark) hudC(25, "Z OR C SETS THE MARK", PAL_INK);
        else if (mode_ == Mode::Aim) hudC(25, "Z OR C DROPS THE RING", PAL_INK);
        else if (mode_ == Mode::Lift) hudC(25, "Z OR C LIFTS THE COIN", PAL_GOOD);
        else hudC(25, "ONLY THE GOLD OPENS IT", PAL_BRASS);

        if (mode_ == Mode::Aim) hudC(26, "LEFT AND RIGHT NUDGE THE SWING", PAL_BRASS);
        else if (mode_ == Mode::Mark) hudC(26, "SIDES ARE NOT THE MARK", PAL_BRASS);
        else hudC(26, "SIDES STEAL A RING", PAL_INK);

        std::snprintf(buf, sizeof buf, "RING %d OF %d", std::min(thrown_ + (mode_ == Mode::Aim || mode_ == Mode::Mark ? 1 : 0), RINGS), RINGS);
        hud(1, 27, buf, PAL_INK);
        hud(28, 27, opened_ ? "OPEN" : "ONE BOOTH", opened_ ? PAL_GOOD : PAL_BRASS);
    }
}

}  // namespace fairmark

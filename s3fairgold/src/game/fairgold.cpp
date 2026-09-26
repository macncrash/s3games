#include "game/fairgold.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace fairgold {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float PI = 3.14159265f;
constexpr int RINGS = 3;
constexpr int NB = 5;
constexpr float CX = 160.f;
constexpr float AMP = 116.f;
constexpr float OMEGA = 1.05f;
constexpr float HIT = 18.f;
constexpr float BOT_HIT = 12.f;
constexpr float NUDGE_MAX = 46.f;
constexpr float FLIGHT_T = 0.42f;
constexpr float SHOW_T = 0.55f;
constexpr float PIVOT_Y = 74.f;
constexpr float HANG_Y = 98.f;
constexpr float NECK_Y = 128.f;
constexpr float MISS_Y = 174.f;
constexpr float FEET_Y = 168.f;
constexpr float NUM_Y = 80.f;
constexpr float BULB_Y = 64.f;
constexpr float SHELF_Y = 172.f;
constexpr float AWN_Y = 46.f;
constexpr float BX[NB] = {52.f, 106.f, 160.f, 214.f, 268.f};
constexpr int GOLDEN[NB] = {0, 1, 0, 1, 0};
constexpr float GATE_X = 264.f;
constexpr float POST_L = 228.f;
constexpr float POST_R = 300.f;
constexpr float BAR_CLOSED = 154.f;
constexpr float BAR_OPEN = 96.f;
constexpr float WALK_OPEN = 274.f;
constexpr float WALK_SHUT = 200.f;
constexpr float WALK_V = 96.f;
constexpr int BUNT_PAL[3] = {PAL_RED, PAL_GOLD, PAL_BLUE};

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

const char* Game::phase() const {
    switch (mode_) {
        case Mode::Title: return "title";
        case Mode::Aim: return "aim";
        case Mode::Flight: return "flight";
        case Mode::Show: return "show";
        case Mode::Walk: return "walk";
        case Mode::End: return won_ ? "leave" : "stay";
        case Mode::Pause: return "pause";
    }
    return "?";
}

Game::Mode Game::view() const { return mode_ == Mode::Pause ? held_ : mode_; }

bool Game::canLeave() const {
    const int bare = gold_ + cream_;
    return gold_ >= 1 && score() >= fare() && bare < fare();
}

float Game::ringX() const {
    const Mode m = view();
    const float t = (m == Mode::Title) ? clock_ : swingT_;
    const float n = (m == Mode::Aim) ? nudge_ : 0.f;
    return CX + AMP * std::sin(t * OMEGA) + n;
}

int Game::bottleAt(float x) const {
    int best = -1;
    float bestD = HIT;
    for (int i = 0; i < NB; i++) {
        if (bottle_[i].rung) continue;
        float d = std::fabs(x - bottle_[i].x);
        if (d <= bestD) {
            bestD = d;
            best = i;
        }
    }
    return best;
}

int Game::botTarget() const {
    const float x = ringX();
    int best = -1;
    float bestD = BOT_HIT;
    for (int i = 0; i < NB; i++) {
        if (!bottle_[i].gold || bottle_[i].rung) continue;
        float d = std::fabs(x - bottle_[i].x);
        if (d <= bestD) {
            bestD = d;
            best = i;
        }
    }
    return best;
}

void Game::place() {
    for (int i = 0; i < NB; i++) {
        bottle_[i].x = BX[i];
        bottle_[i].gold = GOLDEN[i] != 0;
        bottle_[i].rung = false;
    }
    gold_ = cream_ = thrown_ = last_ = 0;
    won_ = left_ = over_ = wasHot_ = false;
    nudge_ = flight_ = showT_ = 0.f;
    landX_ = fromX_ = CX;
    walker_ = 24.f;
    say_ = "";
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(4, 1, 6));
    sys.apu.setMaster(0.7f);
    sys.apu.setEcho(0.16f, 0.22f, 0.14f);
    place();
    mode_ = Mode::Title;
    clock_ = swingT_ = 0.f;
}

void Game::begin() {
    place();
    swingT_ = clock_;
    mode_ = Mode::Aim;
    blip(340.f);
}

void Game::toTitle() {
    place();
    mode_ = Mode::Title;
}

void Game::release() {
    if (mode_ != Mode::Aim || thrown_ >= RINGS) return;
    fromX_ = landX_ = ringX();
    flight_ = FLIGHT_T;
    thrown_++;
    mode_ = Mode::Flight;
    blip(480.f);
}

void Game::resolve() {
    const int hit = bottleAt(landX_);
    if (hit >= 0 && bottle_[hit].gold) {
        bottle_[hit].rung = true;
        gold_++;
        last_ = 2;
        say_ = canLeave() ? "FARE MET" : "GOLD COUNTS 2";
        chord(523.f, 659.f, 784.f, 0.32f);
    } else if (hit >= 0) {
        bottle_[hit].rung = true;
        cream_++;
        last_ = 1;
        say_ = canLeave() ? "FARE MET" : "CREAM COUNTS 1";
        blip(392.f);
    } else {
        last_ = 0;
        say_ = "MISS";
        blip(130.f);
        if (sys_) sys_->apu.noiseBurst(0.1f, 700.f, 0.04f);
    }
    showT_ = SHOW_T;
    mode_ = Mode::Show;
}

void Game::startWalk() {
    mode_ = Mode::Walk;
    walker_ = 24.f;
    if (canLeave()) blip(660.f);
    else blip(110.f);
}

void Game::finishWalk() {
    if (canLeave()) {
        won_ = true;
        left_ = true;
        say_ = "LEAVE";
        chord(392.f, 523.f, 784.f, 1.1f);
        if (sys_) {
            sys_->rumble(0.35f, 0.7f, 180);
            sys_->setLight(255, 200, 60);
        }
    } else {
        won_ = false;
        left_ = false;
        say_ = "STAY";
        blip(90.f);
        if (sys_) sys_->setLight(160, 32, 28);
    }
    mode_ = Mode::End;
    over_ = true;
}

void Game::blip(float freq) {
    if (!sys_) return;
    sys_->apu.tone(0, freq, 0.07f);
    beep_ = std::max(beep_, 0.09f);
}

void Game::chord(float a, float b, float c, float hold) {
    if (!sys_) return;
    sys_->apu.tone(0, a, 0.1f);
    sys_->apu.tone(1, b, 0.08f);
    sys_->apu.tone(2, c, 0.07f);
    beep_ = std::max(beep_, hold);
}

void Game::hush() {
    if (!sys_) return;
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(1, 0, 0);
    sys_->apu.tone(2, 0, 0);
}

Game::Input Game::readPad(const gs::Pad& pad) const {
    Input in;
    in.action = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_TURBO);
    in.start = pad.pressed(gs::BTN_START);
    in.back = pad.pressed(gs::BTN_MODE);
    if (pad.down(gs::BTN_LEFT)) in.x -= 1.f;
    if (pad.down(gs::BTN_RIGHT)) in.x += 1.f;
    if (std::fabs(pad.axisX) > 0.25f) in.x = pad.axisX;
    in.x = clampf(in.x, -1.f, 1.f);
    return in;
}

Game::Input Game::botInput() const {
    Input in;
    if (mode_ == Mode::Title && clock_ > 0.55f) {
        in.start = true;
        return in;
    }
    if (mode_ == Mode::Aim && botTarget() >= 0) in.action = true;
    return in;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (mode_ != Mode::Pause) clock_ += DT;
    if (beep_ > 0.f) {
        beep_ -= DT;
        if (beep_ <= 0.f) hush();
    }

    if (mode_ == Mode::Aim) swingT_ += DT;

    const Input in = bot_ ? botInput() : readPad(sys.pad);
    if (mode_ == Mode::Aim) {
        const float want = in.x * NUDGE_MAX;
        nudge_ += (want - nudge_) * 0.45f;
    }

    if (mode_ == Mode::Title) {
        if (in.back && !bot_) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        } else if (in.start || in.action) begin();
    } else if (mode_ == Mode::Pause) {
        if (in.start) mode_ = held_;
        else if (in.back) toTitle();
    } else if (mode_ == Mode::End) {
        if (!bot_ && (in.start || in.action)) begin();
        else if (!bot_ && in.back) toTitle();
    } else if ((in.start || in.back) && !bot_) {
        held_ = mode_;
        mode_ = Mode::Pause;
    } else if (mode_ == Mode::Aim) {
        const int hot = bottleAt(ringX());
        if (hot >= 0 && !wasHot_) blip(860.f);
        wasHot_ = hot >= 0;
        if (in.action) release();
    } else if (mode_ == Mode::Flight) {
        flight_ -= DT;
        if (flight_ <= 0.f) resolve();
    } else if (mode_ == Mode::Show) {
        showT_ -= DT;
        if (showT_ <= 0.f) {
            if (canLeave() || thrown_ >= RINGS) startWalk();
            else mode_ = Mode::Aim;
        }
    } else if (mode_ == Mode::Walk) {
        const float goal = canLeave() ? WALK_OPEN : WALK_SHUT;
        walker_ = std::min(goal, walker_ + WALK_V * DT);
        if (walker_ >= goal - 0.4f) finishWalk();
    }

    draw();
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog) {
    if (!sys_ || h < 1.f || m.h < 1) return;
    const float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 40 || s.x + s.w < -40 || s.y > gs::SCREEN_H + 40 || s.y + s.h < -40) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, int fog) {
    if (!sys_ || w < 1.f || h < 1.f || m.h < 1) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(std::max(w, h));
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
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
    const int n = s ? int(std::strlen(s)) : 0;
    hud(20 - n / 2, row, s, pal);
}

void Game::backdrop(bool gate) {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        if (!gate) {
            if (y < 36) {
                float u = y / 35.f;
                v.lineBackdrop[y] = gs::rgb4(1 + int(u * 2), 1, 5 + int((1.f - u) * 3));
            } else if (y < 78) {
                float u = (y - 36) / 42.f;
                v.lineBackdrop[y] = gs::rgb4(3 + int(u * 6), 1 + int(u * 2), 6 - int(u * 2));
            } else if (y < 158) {
                v.lineBackdrop[y] = gs::rgb4(5, 2, 3);
            } else {
                int g = 3 + ((y / 4) & 1);
                v.lineBackdrop[y] = gs::rgb4(g + 1, g - 1, 1);
            }
        } else if (y < 48) {
            float u = y / 47.f;
            v.lineBackdrop[y] = gs::rgb4(1 + int(u), 1, 6 + int((1.f - u) * 2));
        } else if (y < 96) {
            float u = (y - 48) / 48.f;
            v.lineBackdrop[y] = gs::rgb4(2 + int(u * 5), 1 + int(u * 2), 7 - int(u * 3));
        } else if (y < 168) {
            int g = 4 + ((y / 6) & 1);
            v.lineBackdrop[y] = gs::rgb4(g + 2, g, 2);
        } else {
            v.lineBackdrop[y] = gs::rgb4(4, 3, 2);
        }
    }
}

void Game::drawBooth() {
    const Mode m = view();
    const bool hang = (m == Mode::Title || m == Mode::Aim) && thrown_ < RINGS;
    const int hot = (m == Mode::Aim || m == Mode::Title) ? bottleAt(ringX()) : -1;

    if (m == Mode::Title) word(art_.logo, 160.f, 18.f, PAL_LOGO);

    if (m == Mode::Flight) {
        float u = 1.f - clampf(flight_ / FLIGHT_T, 0.f, 1.f);
        int pred = bottleAt(landX_);
        float destY = pred >= 0 ? NECK_Y : MISS_Y;
        float x = fromX_ + (landX_ - fromX_) * u;
        float y = HANG_Y + (destY - HANG_Y) * u - std::sin(u * PI) * 34.f;
        spr(art_.ring, x, y, 18.f, PAL_RING);
    } else if (m == Mode::Show && last_ == 0) {
        spr(art_.ring, landX_, MISS_Y, 16.f, PAL_RING);
    } else if (hang) {
        int pal = PAL_RING;
        if (hot >= 0) pal = bottle_[hot].gold ? PAL_GOLD : PAL_CREAM;
        float bob = hot >= 0 ? 1.6f * std::sin(clock_ * 14.f) : 0.f;
        spr(art_.ring, ringX(), HANG_Y + bob, hot >= 0 ? 20.f : 16.f, pal);
    }

    for (int i = 0; i < NB; i++) {
        const gs::Image& num = bottle_[i].gold ? art_.two : art_.one;
        int pal = bottle_[i].gold ? PAL_GOLD : PAL_CREAM;
        float y = NUM_Y + (bottle_[i].gold ? std::sin(clock_ * 3.f + i) * 1.4f : 0.f);
        word(num, bottle_[i].x, y, pal);
        if (bottle_[i].gold) spr(art_.star, bottle_[i].x, y - 14.f, 9.f, PAL_GOLD);
    }

    if (hang) {
        float x0 = CX, y0 = PIVOT_Y, x1 = ringX(), y1 = HANG_Y;
        for (int i = 1; i <= 5; i++) {
            float u = i / 6.f;
            spr(art_.dot, x0 + (x1 - x0) * u, y0 + (y1 - y0) * u, 3.f, PAL_RING);
        }
    }

    const int step = int(clock_ * 2.f) & 1;
    spr(art_.shade, 22.f, FEET_Y - 2.f, 8.f, PAL_WOOD);
    spr(art_.kid[step], 22.f, FEET_Y - 20.f, 40.f, PAL_KID);

    for (int i = 0; i < NB; i++) {
        if (!bottle_[i].rung) continue;
        float pulse = (m == Mode::Show && bottleAt(landX_) == i) ? 1.f + std::sin(showT_ * 16.f) * 0.08f : 1.f;
        spr(art_.ring, bottle_[i].x, NECK_Y, 18.f * pulse, PAL_RING);
    }

    for (int i = 0; i < NB; i++) {
        spr(art_.shade, bottle_[i].x, FEET_Y - 1.f, 8.f, PAL_WOOD);
        int pal = bottle_[i].gold ? PAL_GOLD : PAL_CREAM;
        spr(art_.bottle, bottle_[i].x, FEET_Y - 24.f, 48.f, pal);
    }

    for (int i = 0; i < 11; i++) {
        float x = 48.f + i * 22.f;
        bool on = ((i + int(clock_ * 7.f)) % 4) != 0;
        spr(art_.bulb, x, BULB_Y + std::sin(clock_ * 2.f + i) * 1.5f, on ? 9.f : 6.f, PAL_BULB);
    }

    word(art_.sign, 160.f, AWN_Y, PAL_PAPER);
    stamp(art_.shelf, 160.f, SHELF_Y, 236.f, 14.f, PAL_WOOD);
    stamp(art_.awning, 160.f, AWN_Y, 248.f, 30.f, PAL_AWN);
    stamp(art_.post, 30.f, 112.f, 16.f, 120.f, PAL_WOOD);
    stamp(art_.post, 292.f, 112.f, 16.f, 120.f, PAL_WOOD);
    stamp(art_.cloth, 160.f, 124.f, 220.f, 78.f, PAL_RED);

    for (int i = 0; i < 13; i++) {
        float x = 16.f + i * 24.f;
        float y = 16.f + ((i & 1) ? 3.f : 0.f) + std::sin(clock_ * 1.6f + i) * 1.2f;
        spr(art_.pennant, x, y, 13.f, BUNT_PAL[i % 3]);
    }

    const int balloonPal[3] = {PAL_RED, PAL_GOLD, PAL_BLUE};
    for (int i = 0; i < 3; i++) {
        float y = 96.f + i * 16.f + std::sin(clock_ * 1.8f + i) * 2.f;
        spr(art_.balloon, 304.f, y, 18.f, balloonPal[i]);
    }

    spr(art_.wheel, 36.f, 40.f, 62.f, PAL_NIGHT, false, 7);
    for (int i = 0; i < 6; i++) {
        float a = clock_ * 0.6f + i * PI / 3.f;
        float x = 36.f + std::cos(a) * 22.f;
        float y = 40.f + std::sin(a) * 22.f;
        spr(art_.gondola, x, y, 8.f, PAL_NIGHT, false, 6);
    }
    spr(art_.moon, 292.f, 18.f, 14.f, PAL_BULB, false, 1);
    const float stars[][2] = {{70, 10}, {120, 16}, {188, 8}, {230, 14}, {260, 8}};
    for (int i = 0; i < 5; i++) {
        if ((int(clock_ * 2.f + i) & 3) == 0) continue;
        spr(art_.dot, stars[i][0], stars[i][1], 3.f, PAL_BULB, false, 1);
    }
}

void Game::drawGate() {
    const bool open = canLeave();
    float u = open ? clampf((walker_ - 30.f) / 150.f, 0.f, 1.f) : 0.f;
    float barY = BAR_CLOSED + (BAR_OPEN - BAR_CLOSED) * u;
    const int step = int(walker_ * 0.18f) & 1;
    float bob = std::sin(walker_ * 0.35f) * 1.6f;

    if (mode_ == Mode::End && won_) word(art_.doubled, 160.f, 28.f, PAL_GOOD);
    if (mode_ == Mode::End && !won_) word(art_.stay, 160.f, 28.f, PAL_BAD);

    spr(art_.shade, walker_, FEET_Y, 8.f, PAL_WOOD);
    spr(art_.kid[step], walker_, FEET_Y - 20.f + bob, 40.f, PAL_KID);

    int chips = 0;
    for (int i = 0; i < gold_; i++, chips++) word(art_.two, 18.f + chips * 16.f, 70.f, PAL_GOLD);
    for (int i = 0; i < cream_; i++, chips++) word(art_.one, 18.f + chips * 16.f, 70.f, PAL_CREAM);

    stamp(art_.bar, GATE_X, barY, 78.f, 12.f, PAL_GATE);
    stamp(art_.post, POST_L, 132.f, 16.f, 108.f, PAL_WOOD);
    stamp(art_.post, POST_R, 132.f, 16.f, 108.f, PAL_WOOD);
    word(art_.sign, GATE_X, AWN_Y, PAL_PAPER);
    stamp(art_.awning, 160.f, AWN_Y, 300.f, 26.f, PAL_AWN);

    for (int i = 0; i < 13; i++) {
        float x = 16.f + i * 24.f;
        float y = 16.f + ((i & 1) ? 2.f : 0.f);
        spr(art_.pennant, x, y, 12.f, BUNT_PAL[i % 3]);
    }
    for (int i = 0; i < 8; i++) {
        float x = 70.f + i * 28.f;
        bool on = ((i + int(clock_ * 6.f)) % 3) != 0;
        spr(art_.bulb, x, 62.f, on ? 8.f : 5.f, PAL_BULB);
    }

    const int balloonPal[3] = {PAL_GOLD, PAL_RED, PAL_BLUE};
    for (int i = 0; i < 3; i++) spr(art_.balloon, 16.f + i * 14.f, 108.f + std::sin(clock_ + i) * 2.f, 16.f, balloonPal[i]);

    spr(art_.wheel, 72.f, 86.f, 78.f, PAL_NIGHT, false, 3);
    for (int i = 0; i < 6; i++) {
        float a = clock_ * 0.55f + i * PI / 3.f;
        spr(art_.gondola, 72.f + std::cos(a) * 30.f, 86.f + std::sin(a) * 30.f, 9.f, PAL_NIGHT, false, 2);
    }
    spr(art_.moon, 292.f, 22.f, 16.f, PAL_BULB);

    if (won_) {
        for (int i = 0; i < 8; i++) {
            float a = clock_ * 2.4f + i * PI / 4.f;
            float rad = 26.f + (i & 1) * 12.f;
            spr(art_.dot, 160.f + std::cos(a) * rad, 48.f + std::sin(a) * 8.f, 4.f, BUNT_PAL[i % 3]);
        }
    }
}

void Game::drawHud() {
    const Mode m = view();
    char buf[64];
    if (m == Mode::Title) {
        hud(1, 0, "V" S3_VERSION, PAL_INK);
        hudC(23, "A SHORT FAIR", PAL_GOLD);
        hudC(24, "ONLY THE GOLD COUNTS DOUBLE", PAL_INK);
        hudC(25, "LEAVE WHEN THAT IS TRUE", PAL_GOOD);
        hudC(26, "CREAM COUNTS 1    FARE 4", PAL_CREAM);
        if ((int(clock_ * 2.f) & 1) == 0) hudC(27, "PRESS START", PAL_GOLD);
        else hudC(27, "ARROWS NUDGE   Z OR C THROWS", PAL_INK);
        return;
    }
    if (mode_ == Mode::Pause) {
        hudC(12, "PAUSED", PAL_GOLD);
        hudC(13, "START RESUMES", PAL_INK);
        hudC(14, "ESC TO THE TITLE", PAL_INK);
    }

    std::snprintf(buf, sizeof buf, "GOLD %d  CREAM %d  SCORE %d", gold_, cream_, score());
    hud(1, 0, buf, score() > 0 ? PAL_GOLD : PAL_INK);
    std::snprintf(buf, sizeof buf, "%d/%d", std::min(thrown_, RINGS), RINGS);
    hud(36, 0, buf, PAL_INK);
    hud(1, 1, "FARE 4", canLeave() ? PAL_GOOD : PAL_INK);
    hud(12, 1, "GOLD X2", PAL_GOLD);
    hud(28, 1, "CREAM X1", PAL_CREAM);

    if (m == Mode::End && won_) {
        hudC(23, "ONLY THE GOLD COUNTS DOUBLE", PAL_GOOD);
        std::snprintf(buf, sizeof buf, "GOLD %d  CREAM %d  SCORE %d", gold_, cream_, score());
        hudC(24, buf, PAL_GOLD);
        hudC(25, "THE GATE IS OPEN", PAL_GOOD);
        hudC(26, "LEAVE", PAL_GOLD);
        if (!bot_) hudC(27, "START PLAYS AGAIN", PAL_INK);
    } else if (m == Mode::End) {
        hudC(23, "CREAM DOES NOT PAY THE FARE", PAL_BAD);
        std::snprintf(buf, sizeof buf, "GOLD %d  CREAM %d  SCORE %d", gold_, cream_, score());
        hudC(24, buf, PAL_INK);
        hudC(25, "THE GATE STAYS SHUT", PAL_BAD);
        hudC(26, "STAY", PAL_BAD);
        if (!bot_) hudC(27, "START TRIES AGAIN", PAL_INK);
    } else if (m == Mode::Walk) {
        hudC(24, canLeave() ? "THE GATE IS OPEN" : "THE GATE STAYS SHUT", canLeave() ? PAL_GOOD : PAL_BAD);
        hudC(25, "ONLY THE GOLD COUNTS DOUBLE", PAL_INK);
        std::snprintf(buf, sizeof buf, "SCORE %d   FARE %d", score(), fare());
        hudC(26, buf, canLeave() ? PAL_GOOD : PAL_BAD);
    } else if (m == Mode::Show) {
        int pal = last_ == 2 ? PAL_GOLD : (last_ == 1 ? PAL_CREAM : PAL_BAD);
        hudC(24, say_, pal);
        hudC(25, "ONLY THE GOLD COUNTS DOUBLE", PAL_INK);
    } else if (m == Mode::Flight) {
        hudC(24, "RING AWAY", PAL_GOLD);
    } else if (m == Mode::Aim) {
        int hot = bottleAt(ringX());
        if (hot >= 0 && bottle_[hot].gold) hudC(24, "GOLD COUNTS 2", PAL_GOLD);
        else if (hot >= 0) hudC(24, "CREAM COUNTS 1", PAL_CREAM);
        else hudC(24, "ONLY THE GOLD COUNTS DOUBLE", PAL_INK);
        hudC(26, "Z OR C THROWS", PAL_INK);
        hudC(27, "ARROWS NUDGE THE SWING", PAL_INK);
    }
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    const bool gate = view() == Mode::Walk || view() == Mode::End;
    backdrop(gate);
    if (gate) drawGate();
    else drawBooth();
    drawHud();
}

}  // namespace fairgold

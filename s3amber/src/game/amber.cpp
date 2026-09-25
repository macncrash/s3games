#include "game/amber.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace amber {
namespace {

constexpr double kDt = 1.0 / 60.0;
constexpr double kCruise = 24.0;
constexpr double kCommit = 56.0;
constexpr double kStop = 30.0;
constexpr double kExit = -68.0;
constexpr double kBrake = 40.0;
constexpr double kHaltA = 220.0;
constexpr double kHold = 0.35;
constexpr double kSee = 70.0;
constexpr double kBrakeFrom = 76.0;
constexpr int kCycle = 480;
constexpr int kOrdersN = 12;
constexpr int kLives = 3;

constexpr int kBoxL = 136;
constexpr int kBoxR = 184;
constexpr int kBoxT = 88;
constexpr int kBoxB = 136;
constexpr float kCx = 160.f;
constexpr float kCy = 112.f;
constexpr float kHalf = 8.f;

// Shift-clock spawns. The autopilot is timed to this table and to stepCar's order.
struct Order {
    int arm;
    int runner;
    int frame;
    int d0;
};
constexpr Order kOrders[kOrdersN] = {
    {0, 0, 0, 80},    {0, 1, 398, 80},  {1, 0, 457, 80},   {0, 1, 550, 112},
    {1, 1, 695, 120}, {0, 0, 1192, 72}, {0, 1, 1275, 80},  {0, 0, 1347, 104},
    {1, 1, 1812, 88}, {1, 0, 1894, 80}, {1, 1, 1995, 136}, {1, 1, 2575, 136},
};

char phaseOf(int arm, int frame) {
    int f = frame % kCycle;
    if (f < 0) f += kCycle;
    bool ns = arm == 0 || arm == 2;
    if (ns) {
        if (f < 180) return 'G';
        if (f < 240) return 'A';
        return 'R';
    }
    if (f < 240) return 'R';
    if (f < 420) return 'G';
    return 'A';
}

void bumper(int arm, double d, float& x, float& y) {
    if (arm == 0) {
        x = kCx;
        y = float(kBoxT) - float(d);
    } else if (arm == 1) {
        x = float(kBoxR) + float(d);
        y = kCy;
    } else if (arm == 2) {
        x = kCx;
        y = float(kBoxB) + float(d);
    } else {
        x = float(kBoxL) - float(d);
        y = kCy;
    }
}

void carCenter(int arm, double d, float& x, float& y) {
    bumper(arm, d, x, y);
    if (arm == 0) y -= kHalf;
    else if (arm == 1) x += kHalf;
    else if (arm == 2) y += kHalf;
    else x -= kHalf;
}

int faceOf(int arm) {
    // Art is 0 north, 1 east, 2 south, 3 west. Arm 0 approaches from the north, heading south.
    static const int kFace[] = {2, 3, 0, 1};
    return kFace[arm & 3];
}

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.setFogColor(gs::rgb4(1, 1, 2));
    mode_ = Mode::Title;
    titleT_ = 0;
    anim_ = 0;
    over_ = false;
    won_ = false;
    lives_ = kLives;
    car_ = {};
    quiet();
}

int Game::marker() const {
    if (won_ || mode_ == Mode::Won) return 2;
    if (mode_ == Mode::Lost) return 3;
    if (mode_ == Mode::Title) return 0;
    return 1;
}

int Game::clock() const {
    if (mode_ == Mode::Title) return int(anim_ * 60.f);
    return shiftFrame_;
}

void Game::quiet() {
    if (!sys_) return;
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(1, 0, 0);
    sys_->apu.tone(2, 0, 0);
    sys_->apu.noise(0, 0, false);
    fanOn_ = false;
}

void Game::blip(float freq, float vol) {
    sys_->apu.tone(2, freq, vol);
    beep_ = std::max(beep_, 0.08f);
}

void Game::note(const char* s, int pal) {
    note_ = s;
    notePal_ = pal;
    noteT_ = 1.1f;
}

void Game::puffAt(float x, float y) {
    puffs_[puffi_].x = x;
    puffs_[puffi_].y = y;
    puffs_[puffi_].t = 0.38f;
    puffi_ = (puffi_ + 1) % 8;
}

void Game::fanfare() {
    static const float notes[] = {440.f, 554.f, 659.f, 880.f};
    fanT_ += float(kDt);
    int step = int(fanT_ / 0.16f);
    if (step != fanStep_) {
        fanStep_ = step;
        if (step < 4) {
            sys_->apu.tone(0, notes[step], 0.12f);
            beep_ = 0.18f;
        } else if (step > 5) {
            sys_->apu.tone(0, 0, 0);
            fanOn_ = false;
        }
    }
}

void Game::toTitle() {
    mode_ = Mode::Title;
    titleT_ = 0;
    over_ = false;
    won_ = false;
    car_ = {};
    quiet();
}

void Game::startShift() {
    mode_ = Mode::Shift;
    shiftFrame_ = 0;
    next_ = 0;
    lives_ = kLives;
    score_ = 0;
    stopped_ = 0;
    spared_ = 0;
    car_ = {};
    over_ = false;
    won_ = false;
    bannerT_ = 0;
    shake_ = 0;
    pose_ = 0;
    noteT_ = 0;
    note_ = "";
    fanOn_ = false;
    fanStep_ = -1;
    nsPh_ = phaseOf(0, 0);
    ewPh_ = phaseOf(1, 0);
    quiet();
}

void Game::applyHalt() {
    if (!car_.alive || car_.resolved || car_.halted) return;
    bool correct = car_.runner && car_.owed && !car_.released && car_.d > kStop;
    bool near = car_.d > kStop && car_.d <= kSee + 8.0;
    if (!correct && !near) return;
    car_.halted = true;
    car_.good = correct;
    pose_ = 0.45f;
    float x, y;
    bumper(car_.arm, car_.d, x, y);
    puffAt(x, y);
    sys_->apu.noiseBurst(0.18f, 5400.f, 0.07f);
    blip(correct ? 880.f : 140.f, 0.12f);
    sys_->rumble(correct ? 0.25f : 0.55f, 0.4f, 70);
}

void Game::resolve(Result r) {
    if (car_.result != Result::None) return;
    car_.result = r;
    car_.resolved = true;
    if (r == Result::Stopped) {
        stopped_++;
        score_ += 150;
        note("HELD THE RUNNER", PAL_AMBER);
        blip(660.f, 0.1f);
    } else if (r == Result::SpareStop) {
        spared_++;
        score_ += 80;
        note("THEY STOPPED", PAL_GREEN);
        blip(392.f, 0.08f);
    } else if (r == Result::SpareGo) {
        spared_++;
        score_ += 60;
        note("LET THEM GO", PAL_GREEN);
        blip(494.f, 0.08f);
    } else if (r == Result::Miss) {
        lives_--;
        car_.d = kStop - 10.0;
        note("RAN THE LAMP", PAL_RED);
        blip(90.f, 0.14f);
        shake_ = 0.45f;
        sys_->rumble(0.8f, 0.9f, 140);
    } else {
        lives_--;
        note("SPARE THAT ONE", PAL_RED);
        blip(110.f, 0.14f);
        shake_ = 0.35f;
        sys_->rumble(0.7f, 0.5f, 120);
    }
}

void Game::stepCar(bool pressed) {
    Car& c = car_;
    char ph = phaseOf(c.arm, shiftFrame_);
    double prev = c.d;
    if (ph == 'G') {
        c.owed = false;
        if (c.d <= kCommit) c.released = true;
    } else if (ph == 'A') {
        if (c.released) c.owed = false;
        else if (c.d > kCommit) c.owed = true;
        else if (!c.owed) c.released = true;
    } else {
        if (c.released) c.owed = false;
        else if (c.d > kStop) c.owed = true;
    }
    if (bot_ && c.runner && c.owed && !c.halted && c.d > kStop + 10.0 && c.d <= kSee) pressed = true;
    if (pressed) applyHalt();

    if (c.halted) c.speed = std::max(0.0, c.speed - kHaltA * kDt);
    else if (!c.runner && c.owed && c.speed <= 0.0) c.speed = 0.0;
    else if (!c.runner && c.owed && c.d <= kBrakeFrom) c.speed = std::max(0.0, c.speed - kBrake * kDt);
    else c.speed = kCruise;

    c.d -= c.speed * kDt;
    if (!c.runner && c.owed && c.d < kStop) {
        c.d = kStop;
        c.speed = 0.0;
    }
    if (c.halted && c.speed < 0.3) c.speed = 0.0;
    if (c.halted && c.d < kStop) c.d = kStop;

    if (c.runner && !c.halted && c.owed && !c.released && prev > kStop && c.d <= kStop) {
        resolve(Result::Miss);
        return;
    }
    if (c.halted && c.speed <= 0.0) {
        resolve(c.good ? Result::Stopped : Result::FalseStop);
        return;
    }
    if (!c.runner && c.owed && c.speed <= 0.0 && c.d >= kStop - 0.01) {
        c.held += kDt;
        if (c.held >= kHold) resolve(Result::SpareStop);
    } else {
        c.held = 0;
    }
    if (c.result == Result::None && c.d <= kExit) resolve((c.released || !c.owed) ? Result::SpareGo : Result::Miss);
}

void Game::tickShift(bool pressed) {
    if ((!car_.alive || car_.resolved) && next_ < kOrdersN && shiftFrame_ >= kOrders[next_].frame) {
        const Order& o = kOrders[next_++];
        car_ = {};
        car_.alive = true;
        car_.runner = o.runner != 0;
        car_.arm = o.arm;
        car_.d = o.d0;
        car_.speed = kCruise;
    }
    if (car_.alive && !car_.resolved) stepCar(pressed);

    if (mode_ == Mode::Shift && car_.resolved && lives_ <= 0) {
        mode_ = Mode::Lost;
        won_ = false;
        bannerT_ = 0;
        fanOn_ = false;
        quiet();
        blip(82.f, 0.16f);
        return;
    }
    if (mode_ == Mode::Shift && next_ >= kOrdersN && car_.resolved && lives_ > 0) {
        mode_ = Mode::Won;
        won_ = true;
        bannerT_ = 0;
        fanOn_ = true;
        fanT_ = 0;
        fanStep_ = -1;
        note("THE BOX IS CLEAR", PAL_GREEN);
        return;
    }

    char ns = phaseOf(0, shiftFrame_);
    char ew = phaseOf(1, shiftFrame_);
    if (shiftFrame_ > 0 && (ns != nsPh_ || ew != ewPh_)) blip(220.f, 0.04f);
    nsPh_ = ns;
    ewPh_ = ew;
    shiftFrame_++;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    anim_ += float(kDt);
    if (beep_ > 0) {
        beep_ -= float(kDt);
        if (beep_ <= 0) {
            sys.apu.tone(2, 0, 0);
            sys.apu.tone(1, 0, 0);
            if (!fanOn_) sys.apu.tone(0, 0, 0);
        }
    }
    if (pose_ > 0) pose_ -= float(kDt);
    if (shake_ > 0) shake_ = std::max(0.f, shake_ - float(kDt));
    if (noteT_ > 0) noteT_ -= float(kDt);
    for (Puff& p : puffs_)
        if (p.t > 0) p.t -= float(kDt);

    bool start = sys.pad.pressed(gs::BTN_START);
    bool whistle = sys.pad.pressed(gs::BTN_A) || sys.pad.pressed(gs::BTN_B) || sys.pad.pressed(gs::BTN_C) ||
                   sys.pad.pressed(gs::BTN_TURBO);
    bool back = sys.pad.pressed(gs::BTN_MODE);

    if (mode_ == Mode::Title) {
        titleT_ += float(kDt);
        if ((bot_ && titleT_ > 0.45f) || start || whistle) startShift();
    } else if (mode_ == Mode::Shift) {
        if (!bot_ && start) mode_ = Mode::Pause;
        else tickShift(whistle);
    } else if (mode_ == Mode::Pause) {
        if (start || whistle) mode_ = Mode::Shift;
        else if (back) toTitle();
    } else if (mode_ == Mode::Won || mode_ == Mode::Lost) {
        bannerT_ += float(kDt);
        if (bannerT_ > 1.0f) over_ = true;
        if ((start || whistle) && bannerT_ > 0.4f) startShift();
        else if (back) toTitle();
        if (fanOn_) fanfare();
    }
    draw();
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(std::max(1, m.h));
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx + shx_ - s.w * 0.5f));
    s.y = int16_t(std::lround(cy + shy_ - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, bool shadow) {
    if (w < 1.f || h < 1.f || m.h < 1) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx + shx_ - s.w * 0.5f));
    s.y = int16_t(std::lround(cy + shy_ - s.h * 0.5f));
    s.img = m.pick(std::max(w, h));
    s.pal = uint8_t(pal);
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    bool hold = mode_ == Mode::Won;
    bool fell = mode_ == Mode::Lost;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        int r = 1, g = 1, b = 2;
        if ((y / 8 + (y > 120)) & 1) b = 3;
        if (hold) g = std::min(4, g + 1);
        if (fell) r = std::min(5, r + 2);
        if (shake_ > 0 && fell) r = std::min(8, r + int(shake_ * 4));
        v.lineBackdrop[y] = gs::rgb4(r, g, b);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    shx_ = shy_ = 0;
    if (shake_ > 0) {
        shx_ = std::sin(anim_ * 90.f) * 3.5f * shake_;
        shy_ = std::cos(anim_ * 70.f) * 2.f * shake_;
    }
    backdrop();
    if (mode_ == Mode::Won) sys_->setLight(40, 160, 70);
    else if (mode_ == Mode::Lost) sys_->setLight(180, 30, 20);
    else if (pose_ > 0) sys_->setLight(220, 120, 20);
    else sys_->setLight(160, 80, 16);

    int clk = clock();
    char ns = phaseOf(0, clk);
    char ew = phaseOf(1, clk);

    if (mode_ == Mode::Title) spr(art_.title, 160, 30, float(art_.title.h), PAL_AMBER);
    else if (mode_ == Mode::Won) spr(art_.clear, 160, 112, float(art_.clear.h), PAL_GREEN);
    else if (mode_ == Mode::Lost) spr(art_.over, 160, 112, float(art_.over.h), PAL_RED);

    for (const Puff& p : puffs_) {
        if (p.t <= 0) continue;
        float k = 1.3f - p.t * 2.2f;
        spr(art_.ring, p.x, p.y, 10.f + k * 16.f, PAL_FX);
    }

    float copX = 198.f, copY = 150.f;
    if (pose_ > 0) {
        copX = 188.f;
        copY = 142.f;
    }
    spr(art_.cop[pose_ > 0 ? 1 : 0], copX, copY + std::sin(anim_ * 3.f), float(art_.cop[0].h), PAL_COP);
    if (mode_ == Mode::Title) {
        spr(art_.car[1], 250, 164, float(art_.car[1].h), PAL_RUN);
        spr(art_.car[3], 70, 164, float(art_.car[3].h), PAL_BRAKE);
    }

    if (car_.alive) {
        float cx, cy;
        carCenter(car_.arm, car_.d, cx, cy);
        bool side = car_.arm == 1 || car_.arm == 3;
        const gs::Mipped& body = art_.car[faceOf(car_.arm)];
        int pal = car_.runner ? PAL_RUN : PAL_LAW;
        if (!car_.runner && (car_.speed < kCruise - 1.0 || (car_.owed && car_.speed <= 0.0))) pal = PAL_BRAKE;
        if (!car_.resolved && car_.d <= kSee + 4.0 && car_.d > 0.0) spr(art_.bracket, cx, cy, side ? 26.f : 28.f, PAL_FX);
        spr(body, cx, cy, float(body.h), pal);
        stamp(art_.shadow, cx, cy + (side ? 5.f : 7.f), side ? 16.f : 12.f, 5.f, PAL_ROAD, true);
    }

    auto lampPal = [](char ph) { return ph == 'G' ? PAL_LAMP_G : ph == 'A' ? PAL_LAMP_A : PAL_LAMP_R; };
    auto drawHead = [&](float x, float y, char ph) {
        // Lamp first so it sits on top of the housing. Holes are at bitmap y 4, 10, 16.
        int lit = ph == 'R' ? 0 : ph == 'A' ? 1 : 2;
        float oy = lit == 0 ? -9.f : lit == 1 ? -3.f : 3.f;
        spr(art_.lamp, x, y + oy, 7.f, lampPal(ph));
        spr(art_.signal, x, y, float(art_.signal.h), PAL_SIGNAL);
    };
    // Housing holes sit at bitmap y 4, 10, 16 of a 26-tall head, sprite-centered.
    drawHead(122, 58, ns);
    drawHead(198, 166, ns);
    drawHead(214, 78, ew);
    drawHead(106, 150, ew);

    auto lineAt = [&](int arm, double d, bool amber, float across) {
        float x, y;
        bumper(arm, d, x, y);
        bool nsArm = arm == 0 || arm == 2;
        const gs::Mipped& img = amber ? art_.amberLine : art_.white;
        if (nsArm) stamp(img, x, y, across, amber ? 3.f : 3.f, PAL_ROAD);
        else stamp(img, x, y, amber ? 3.f : 3.f, across, PAL_ROAD);
    };
    for (int arm = 0; arm < 4; arm++) {
        lineAt(arm, kStop, false, 40.f);
        lineAt(arm, kCommit, true, 28.f);
        bool nsArm = arm == 0 || arm == 2;
        for (int i = 0; i < 4; i++) {
            float x, y;
            bumper(arm, 12.0 + i * 4.0, x, y);
            if (nsArm) stamp(art_.white, x, y, 34.f, 2.f, PAL_ROAD);
            else stamp(art_.white, x, y, 2.f, 30.f, PAL_ROAD);
        }
        for (int i = 0; i < 3; i++) {
            float x, y;
            bumper(arm, 64.0 + i * 10.0, x, y);
            if (nsArm) stamp(art_.white, x, y, 2.f, 6.f, PAL_ROAD);
            else stamp(art_.white, x, y, 6.f, 2.f, PAL_ROAD);
        }
    }

    stamp(art_.pad, kCx, kCy, float(kBoxR - kBoxL), float(kBoxB - kBoxT), PAL_ROAD);
    stamp(art_.asphalt, kCx, 112, float(kBoxR - kBoxL), float(gs::SCREEN_H), PAL_ROAD);
    stamp(art_.asphalt, 160, kCy, float(gs::SCREEN_W), float(kBoxB - kBoxT), PAL_ROAD);

    stamp(art_.block[0], 68, 44, 120, 76, PAL_BLOCK);
    stamp(art_.block[1], 252, 44, 120, 76, PAL_BLOCK);
    stamp(art_.block[2], 68, 180, 120, 76, PAL_BLOCK);
    stamp(art_.block[3], 252, 180, 120, 76, PAL_BLOCK);

    char top[48];
    std::snprintf(top, sizeof top, "STOP %d  SPARE %d  LIFE %d", stopped_, spared_, lives_);
    if (mode_ == Mode::Title) {
        hudC(21, "ONE INTERSECTION", PAL_AMBER);
        hudC(22, "STOP WHO RUNS THE LAMP", PAL_HUD);
        hudC(23, "SPARE WHO DOES NOT", PAL_HUD);
        hudC(24, "AMBER CARS NEVER BRAKE", PAL_AMBER);
        hudC(25, "PAST THE AMBER TICK THEY FINISH", PAL_AMBER);
        hudC(26, "WHITE BAR IS THE STOP", PAL_HUD);
        hudC(27, "Z WHISTLE      ENTER START", PAL_GREEN);
    } else if (mode_ == Mode::Pause) {
        hud(1, 0, top, PAL_HUD);
        hudC(1, "PAUSED", PAL_AMBER);
        hudC(26, "ENTER RESUME", PAL_HUD);
        hudC(27, "ESC TITLE", PAL_DIM);
    } else {
        hud(1, 0, top, PAL_HUD);
        hud(1, 1, "NS", PAL_HUD);
        const char* nsW = ns == 'G' ? "GREEN" : ns == 'A' ? "AMBER" : "RED";
        const char* ewW = ew == 'G' ? "GREEN" : ew == 'A' ? "AMBER" : "RED";
        int nsPal = ns == 'G' ? PAL_GREEN : ns == 'A' ? PAL_AMBER : PAL_RED;
        int ewPal = ew == 'G' ? PAL_GREEN : ew == 'A' ? PAL_AMBER : PAL_RED;
        hud(4, 1, nsW, nsPal);
        hud(11, 1, "EW", PAL_HUD);
        hud(14, 1, ewW, ewPal);
        hud(21, 1, "Z WHISTLE", PAL_HUD);
        if (noteT_ > 0 && note_[0]) hudC(26, note_, notePal_);
        if (mode_ == Mode::Won) hudC(27, "ENTER AGAIN", PAL_GREEN);
        else if (mode_ == Mode::Lost) hudC(27, "ENTER AGAIN", PAL_RED);
        else {
            char sub[40];
            int done = stopped_ + spared_;
            std::snprintf(sub, sizeof sub, "%d/%d   SCORE %d", done, kOrdersN, score_);
            hud(1, 2, sub, PAL_DIM);
        }
    }
}

}  // namespace amber

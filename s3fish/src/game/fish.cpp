#include "game/fish.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace fish {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float DAY = 56.f;
constexpr float FISH_MIN = 86.f;
constexpr float FISH_MAX = 236.f;
constexpr float PI = 3.1415926f;

struct Spec {
    const char* name;
    float bob;
    float runSpeed;
    float runInt;
    float reelTax;
    float reelRate;
    float startTen;
    float height;
    float nose;
    int pal;
};

constexpr Spec SPEC[5] = {
    {"BASS", 4.2f, 46.f, 0.95f, 18.f, 24.f, 30.f, 32.f, 0.42f, PAL_BASS},
    {"TROUT", 5.0f, 62.f, 0.58f, 16.f, 22.f, 26.f, 28.f, 0.42f, PAL_TROUT},
    {"PIKE", 3.4f, 68.f, 1.15f, 20.f, 19.f, 32.f, 24.f, 0.44f, PAL_PIKE},
    {"WALLEYE", 4.0f, 50.f, 0.78f, 18.f, 22.f, 28.f, 30.f, 0.41f, PAL_WALLEYE},
    {"CATFISH", 2.2f, 36.f, 1.35f, 24.f, 17.f, 40.f, 34.f, 0.40f, PAL_CAT},
};

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

void skyAt(float day, uint16_t& top, uint16_t& hor) {
    const uint16_t dTop = gs::rgb4(6, 4, 11), dHor = gs::rgb4(15, 8, 5);
    const uint16_t nTop = gs::rgb4(3, 8, 14), nHor = gs::rgb4(8, 14, 15);
    const uint16_t uTop = gs::rgb4(9, 4, 8), uHor = gs::rgb4(15, 7, 2);
    const uint16_t kTop = gs::rgb4(1, 1, 3), kHor = gs::rgb4(2, 2, 5);
    if (day < 0.45f) {
        float t = day / 0.45f;
        top = lerpC(dTop, nTop, t);
        hor = lerpC(dHor, nHor, t);
    } else if (day < 0.78f) {
        float t = (day - 0.45f) / 0.33f;
        top = lerpC(nTop, uTop, t);
        hor = lerpC(nHor, uHor, t);
    } else {
        float t = (day - 0.78f) / 0.42f;
        top = lerpC(uTop, kTop, t);
        hor = lerpC(uHor, kHor, t);
    }
}

gs::FMPatch waterPad() {
    gs::FMPatch p;
    p.alg = 4;
    p.fb = 0.2f;
    p.op[0] = {1.f, 0.55f, 0.35f, 0.8f, 0.85f, 0.4f};
    p.op[1] = {2.f, 0.35f, 0.25f, 0.6f, 0.7f, 0.35f};
    p.op[2] = {1.f, 0.2f, 0.2f, 0.5f, 0.55f, 0.3f};
    p.op[3] = {3.f, 0.12f, 0.15f, 0.4f, 0.4f, 0.25f};
    p.vol = 0.08f;
    p.tone = 700.f;
    p.echo = 0.25f;
    return p;
}

gs::FMPatch bell() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.25f;
    p.op[0] = {1.f, 1.f, 0.008f, 0.35f, 0.15f, 0.28f};
    p.op[1] = {2.76f, 0.5f, 0.008f, 0.3f, 0.1f, 0.22f};
    p.op[2] = {5.1f, 0.28f, 0.01f, 0.22f, 0.05f, 0.18f};
    p.op[3] = {1.f, 0.35f, 0.01f, 0.25f, 0.12f, 0.2f};
    p.vol = 0.2f;
    p.echo = 0.35f;
    return p;
}

}  // namespace

float Game::day() const { return DAY; }

bool Game::creelFull() const {
    for (int i = 0; i < 5; i++)
        if (!have_[i]) return false;
    return true;
}

int Game::faceOf(int i) const {
    const Fish& f = fish_[i];
    if (i == hooked_) return f.runDir >= 0 ? 1 : -1;
    return f.vx >= 0.f ? 1 : -1;
}

void Game::mouthOf(int i, float& mx, float& my) const {
    const Fish& f = fish_[i];
    const Spec& sp = SPEC[f.sp];
    float h = f.keeper ? sp.height : 15.f;
    const gs::Mipped& im = art_.fish[f.sp][f.keeper ? 1 : 0];
    float w = h * float(im.w) / float(std::max(1, int(im.h)));
    mx = f.x + faceOf(i) * w * sp.nose;
    my = f.y + h * 0.02f;
}

int Game::goal() const {
    for (int s = 0; s < 5; s++) {
        if (have_[s]) continue;
        for (int i = 0; i < NFISH; i++)
            if (fish_[i].keeper && fish_[i].sp == s && !fish_[i].out) return i;
    }
    return -1;
}

float Game::skyDay() const {
    if (mode_ == Mode::Title || mode_ == Mode::Help) return 0.12f;
    if (mode_ == Mode::Lose) return 1.12f;
    return std::clamp(clock_ / DAY, 0.f, 1.f);
}

void Game::say(const char* s) {
    std::snprintf(msg_, sizeof msg_, "%s", s);
    msgT_ = 1.6f;
}

void Game::blip(bool high) {
    sys_->apu.tone(1, high ? 880.f : 440.f, 0.06f);
    blipT_ = 0.05f;
}

void Game::chime(bool big) {
    chimeBig_ = big;
    chimeStep_ = 0;
    chimeT_ = 1.f;
}

void Game::puffAt(float x, float y) {
    puff_[puffN_ % 6] = Puff{x, y, 0.4f};
    puffN_++;
}

void Game::seed() {
    struct S {
        int sp;
        bool keep;
        float x, vx, y, phase;
    };
    static const S s[NFISH] = {
        {0, true, 100, 24, 148, 0.4f},  {1, true, 220, -30, 126, 1.7f}, {2, true, 180, 20, 110, 2.8f},
        {3, true, 140, -22, 162, 0.9f}, {4, true, 200, 16, 176, 3.6f},  {1, false, 92, 34, 136, 4.2f},
        {0, false, 210, -34, 154, 5.1f}, {2, false, 120, 28, 118, 2.2f}, {4, false, 96, -24, 170, 1.1f},
    };
    for (int i = 0; i < NFISH; i++) {
        Fish& f = fish_[i];
        f = Fish{};
        f.sp = s[i].sp;
        f.keeper = s[i].keep;
        f.x = s[i].x;
        f.vx = s[i].vx;
        f.yBase = s[i].y;
        f.y = s[i].y;
        f.phase = s[i].phase;
        f.runDir = f.vx >= 0.f ? 1 : -1;
    }
}

void Game::startDay() {
    seed();
    age_ = 0;
    clock_ = 0;
    keepers_ = 0;
    over_ = false;
    won_ = false;
    stung_ = false;
    hooked_ = -1;
    lock_ = bite_ = tension_ = progress_ = hookBuf_ = shake_ = showT_ = msgT_ = 0;
    msg_[0] = 0;
    for (int i = 0; i < 5; i++) have_[i] = false;
    for (auto& p : puff_) p.t = 0;
    boatX_ = lureX_ = 160;
    lureY_ = 140;
    mode_ = Mode::Play;
}

void Game::swim() {
    for (int i = 0; i < NFISH; i++) {
        Fish& f = fish_[i];
        if (f.out || i == hooked_) continue;
        f.x += f.vx * DT;
        if (f.x < FISH_MIN) {
            f.x = FISH_MIN;
            f.vx = std::fabs(f.vx);
        }
        if (f.x > FISH_MAX) {
            f.x = FISH_MAX;
            f.vx = -std::fabs(f.vx);
        }
        float bob = SPEC[f.sp].bob * (f.keeper ? 1.f : 0.65f);
        f.y = f.yBase + std::sin(age_ * 1.35f + f.phase) * bob;
        if (f.shy > 0) f.shy -= DT;
    }
}

void Game::showcase() {
    boatX_ = 170.f + std::sin(age_ * 0.35f) * 48.f;
    lureY_ = 142.f + std::sin(age_ * 0.7f) * 16.f;
    lureX_ = boatX_;
}

bool Game::tryHook(bool allowShort) {
    int prefer = bot_ ? goal() : -1;
    if (prefer >= 0) {
        const Fish& g = fish_[prefer];
        float mx, my;
        mouthOf(prefer, mx, my);
        if (!g.out && g.shy <= 0.f && std::hypot(mx - lureX_, my - lureY_) <= 28.f) {
            hook(prefer);
            return true;
        }
    }
    int best = -1;
    float bestD = 1e9f;
    bool bestK = false;
    for (int i = 0; i < NFISH; i++) {
        const Fish& f = fish_[i];
        if (f.out || f.shy > 0.f) continue;
        if (!f.keeper && !allowShort) continue;
        float mx, my;
        mouthOf(i, mx, my);
        float d = std::hypot(mx - lureX_, my - lureY_);
        float r = f.keeper ? 28.f : 15.f;
        if (d > r) continue;
        bool k = f.keeper;
        if (best < 0 || (k && !bestK) || (k == bestK && d < bestD)) {
            best = i;
            bestD = d;
            bestK = k;
        }
    }
    if (best < 0) return false;
    hook(best);
    return true;
}

void Game::hook(int i) {
    hooked_ = i;
    Fish& f = fish_[i];
    f.runDir = f.vx >= 0.f ? 1 : -1;
    f.runT = SPEC[f.sp].runInt;
    tension_ = f.keeper ? SPEC[f.sp].startTen : 18.f;
    progress_ = 0;
    bite_ = 0.16f;
    char b[40];
    std::snprintf(b, sizeof b, f.keeper ? "%s  SET" : "%s  SHORT", SPEC[f.sp].name);
    say(b);
    sys_->apu.noiseBurst(0.22f, 1600.f, 0.06f);
    sys_->rumble(0.35f, 0.15f, 70);
    blip(true);
    float mx, my;
    mouthOf(i, mx, my);
    puffAt(mx, my);
}

void Game::land() {
    int i = hooked_;
    Fish& f = fish_[i];
    int sp = f.sp;
    bool keep = f.keeper;
    float mx, my;
    mouthOf(i, mx, my);
    puffAt(mx, my);
    hooked_ = -1;
    bite_ = progress_ = tension_ = 0;
    lock_ = 0.28f;
    lureX_ = boatX_;
    showSp_ = sp;
    showKeep_ = keep;
    showT_ = keep ? 0.85f : 0.45f;
    if (keep) {
        if (!have_[sp]) {
            have_[sp] = true;
            keepers_++;
        }
        f.out = true;
        sys_->rumble(0.25f, 0.55f, 120);
        if (keepers_ >= 5 && creelFull()) {
            mode_ = Mode::Win;
            won_ = true;
            over_ = true;
            lureY_ = 118.f;
            say("FIVE KEEPERS");
            chime(true);
        } else {
            char b[40];
            std::snprintf(b, sizeof b, "%s KEEPER", SPEC[sp].name);
            say(b);
            chime(false);
        }
    } else {
        say("SHORT");
        f.shy = 1.1f;
        f.x = std::clamp(f.x + (f.vx >= 0.f ? 64.f : -64.f), FISH_MIN, FISH_MAX);
        sys_->apu.tone(1, 220.f, 0.07f);
        blipT_ = 0.12f;
        sys_->rumble(0.15f, 0.05f, 60);
    }
}

void Game::snap() {
    int i = hooked_;
    fish_[i].shy = 0.85f;
    hooked_ = -1;
    bite_ = progress_ = tension_ = 0;
    lock_ = 0.4f;
    lureX_ = boatX_;
    shake_ = 0.45f;
    say("LINE BROKE");
    sys_->apu.noiseBurst(0.55f, 500.f, 0.22f);
    sys_->rumble(0.8f, 0.3f, 160);
    blip(false);
}

void Game::cutLine() {
    if (hooked_ < 0) return;
    fish_[hooked_].shy = 0.55f;
    hooked_ = -1;
    bite_ = progress_ = tension_ = 0;
    lock_ = 0.16f;
    lureX_ = boatX_;
    say("CUT");
    sys_->apu.noiseBurst(0.18f, 900.f, 0.05f);
}

void Game::approach() {
    float ax = 0, ay = 0;
    if (bot_) {
        int t = goal();
        if (t >= 0) {
            float mx, my;
            mouthOf(t, mx, my);
            float dx = mx - boatX_;
            float dy = my - lureY_;
            if (dx < -3.f) ax = -1.f;
            else if (dx > 3.f) ax = 1.f;
            if (dy < -3.f) ay = -1.f;
            else if (dy > 3.f) ay = 1.f;
        }
    } else {
        const gs::Pad& p = sys_->pad;
        if (p.down(gs::BTN_LEFT)) ax = -1.f;
        else if (p.down(gs::BTN_RIGHT)) ax = 1.f;
        else if (std::fabs(p.axisX) > 0.28f) ax = std::clamp(p.axisX, -1.f, 1.f);
        if (p.down(gs::BTN_UP)) ay = -1.f;
        else if (p.down(gs::BTN_DOWN)) ay = 1.f;
    }
    boatX_ = std::clamp(boatX_ + ax * 165.f * DT, 34.f, 286.f);
    lureY_ = std::clamp(lureY_ + ay * 120.f * DT, 100.f, 186.f);
    lureX_ = boatX_;

    if (bot_) {
        int t = goal();
        if (t >= 0) {
            float mx, my;
            mouthOf(t, mx, my);
            if (fish_[t].shy <= 0.f && std::hypot(mx - lureX_, my - lureY_) < 15.f) tryHook(false);
        }
    } else {
        const gs::Pad& p = sys_->pad;
        if (p.pressed(gs::BTN_C)) hookBuf_ = 0.18f;
        if (hookBuf_ > 0.f) {
            if (tryHook(true)) hookBuf_ = 0;
            else hookBuf_ -= DT;
        } else if (p.down(gs::BTN_C)) {
            tryHook(false);
        }
    }
}

void Game::fightFish() {
    Fish& f = fish_[hooked_];
    const Spec& sp = SPEC[f.sp];
    auto place = [&]() { mouthOf(hooked_, lureX_, lureY_); };

    if (bite_ > 0.f) {
        bite_ -= DT;
        f.x = std::clamp(f.x + f.runDir * 14.f * DT, FISH_MIN, FISH_MAX);
        f.y = f.yBase + std::sin(age_ * 2.4f + f.phase) * 1.6f;
        place();
        bool cut = bot_ ? !f.keeper : false;
        if (!bot_) {
            const gs::Pad& p = sys_->pad;
            cut = p.pressed(gs::BTN_B) || p.pressed(gs::BTN_X) || p.pressed(gs::BTN_Z);
        }
        if (cut) cutLine();
        return;
    }

    f.runT -= DT;
    if (f.runT <= 0.f) {
        f.runDir = -f.runDir;
        f.runT = sp.runInt * (f.keeper ? 1.f : 0.7f);
    }
    float rspd = sp.runSpeed * (f.keeper ? 1.f : 0.5f);
    f.x += f.runDir * rspd * DT;
    if (f.x <= FISH_MIN) {
        f.x = FISH_MIN;
        f.runDir = 1;
        f.runT = sp.runInt;
    }
    if (f.x >= FISH_MAX) {
        f.x = FISH_MAX;
        f.runDir = -1;
        f.runT = sp.runInt;
    }
    f.y = f.yBase + std::sin(age_ * 3.1f + f.phase) * 2.4f;
    place();

    int rod = 0;
    bool reel = false;
    bool cut = false;
    if (bot_) {
        rod = f.runDir;
        reel = tension_ < 78.f;
        cut = !f.keeper;
    } else {
        const gs::Pad& p = sys_->pad;
        if (p.down(gs::BTN_LEFT)) rod = -1;
        else if (p.down(gs::BTN_RIGHT)) rod = 1;
        reel = p.down(gs::BTN_C) || p.down(gs::BTN_A) || p.down(gs::BTN_TURBO) || p.down(gs::BTN_Y);
        cut = p.pressed(gs::BTN_B) || p.pressed(gs::BTN_X) || p.pressed(gs::BTN_Z);
    }
    if (cut) {
        cutLine();
        return;
    }

    float tax = sp.reelTax * (f.keeper ? 1.f : 0.35f);
    float rate = sp.reelRate * (f.keeper ? 1.f : 1.8f);
    float d = 8.f;
    if (rod && rod == f.runDir) d -= 50.f;
    else if (rod && rod == -f.runDir) d += 34.f;
    if (reel) {
        d += tax;
        progress_ += rate * DT;
        reelAcc_ -= DT;
        if (reelAcc_ <= 0.f) {
            sys_->apu.tone(0, 200.f + progress_ * 3.2f, 0.045f);
            reelAcc_ = 0.09f;
            reelHold_ = 0.035f;
        }
    }
    tension_ = std::clamp(tension_ + d * DT, 0.f, 100.f);
    if (progress_ >= 100.f) land();
    else if (tension_ >= 100.f) snap();
}

void Game::updatePlay() {
    swim();
    if (hooked_ >= 0) fightFish();
    else if (lock_ > 0.f) {
        lock_ -= DT;
        lureX_ = boatX_;
    } else {
        approach();
    }
    if (mode_ == Mode::Win) return;
    clock_ += DT;
    if (clock_ >= DAY && keepers_ < 5) {
        mode_ = Mode::Lose;
        over_ = true;
        won_ = false;
        say("DARK");
        if (!stung_) {
            stung_ = true;
            sys_->apu.noiseBurst(0.3f, 200.f, 0.4f);
            sys_->apu.tone(1, 110.f, 0.08f);
            blipT_ = 0.4f;
        }
    }
}

void Game::tickFx() {
    for (auto& p : puff_)
        if (p.t > 0) p.t -= DT;
    if (msgT_ > 0) msgT_ -= DT;
    if (shake_ > 0) shake_ -= DT;
    if (showT_ > 0) showT_ -= DT;
}

void Game::audio() {
    if (blipT_ > 0) {
        blipT_ -= DT;
        if (blipT_ <= 0) sys_->apu.tone(1, 0, 0);
    }
    if (reelHold_ > 0) {
        reelHold_ -= DT;
        if (reelHold_ <= 0) sys_->apu.tone(0, 0, 0);
    }
    if (padOn_) {
        float f = 130.f + 8.f * std::sin(age_ * 0.6f);
        if (mode_ == Mode::Lose) f = 90.f;
        if (hooked_ >= 0) f += 14.f;
        sys_->apu.setFreq(0, f);
        float v = (mode_ == Mode::Play || mode_ == Mode::Win) ? 0.07f : 0.05f;
        if (mode_ == Mode::Lose) v = 0.035f;
        sys_->apu.setVol(0, v);
    }
    if (chimeStep_ >= 0) {
        static const float smallN[] = {523.f, 659.f, 784.f};
        static const float bigN[] = {392.f, 494.f, 587.f, 784.f, 988.f};
        const float* n = chimeBig_ ? bigN : smallN;
        int count = chimeBig_ ? 5 : 3;
        chimeT_ += DT;
        if (chimeT_ >= 0.13f) {
            chimeT_ = 0;
            if (chimeStep_ < count) sys_->apu.keyOn(1, n[chimeStep_], chimeBig_ ? 0.22f : 0.16f);
            else sys_->apu.keyOff(1);
            chimeStep_++;
            if (chimeStep_ > count + 2) chimeStep_ = -1;
        }
    }
    float day = skyDay();
    if (day < 0.4f) sys_->setLight(90, 50, 16);
    else if (day < 0.7f) sys_->setLight(30, 60, 110);
    else sys_->setLight(120, 30, 8);
}

void Game::hudText(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudCenter(int row, const char* s, int pal) { hudText(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::pipRow(int col, int row, int on, int total, int onPal, int offPal) {
    on = std::clamp(on, 0, total);
    for (int i = 0; i < total; i++) hudText(col + i, row, i < on ? "=" : "-", i < on ? onPal : offPal);
}

void Game::text(const char* s, float x, float y, float scale, int pal, int align) {
    int n = int(std::strlen(s));
    float adv = 16.f * scale;
    float w = float(n) * adv;
    if (align == 0) x -= w * 0.5f;
    else if (align > 0) x -= w;
    for (int i = 0; i < n; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + i * adv + g.w * scale * 0.5f, y, g.h * scale, pal, false);
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet, bool shadow) {
    if (h < 1.2f || m.h < 1) return;
    cx += shakeX_;
    cy += shakeY_;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 80 || s.x + s.w < -80 || s.y > gs::SCREEN_H + 40 || s.y + s.h < -40) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::line(float x0, float y0, float x1, float y1, int pal) {
    float dx = x1 - x0, dy = y1 - y0;
    float len = std::sqrt(dx * dx + dy * dy);
    int n = std::clamp(int(len / 6.f), 1, 22);
    for (int i = 0; i <= n; i++) {
        float t = float(i) / float(n);
        spr(art_.line, x0 + dx * t, y0 + dy * t, 4.f, pal, false);
    }
}

void Game::drawOneFish(int i, int fogBias) {
    const Fish& f = fish_[i];
    if (f.out) return;
    const Spec& sp = SPEC[f.sp];
    float h = f.keeper ? sp.height : 15.f;
    int fog = std::clamp(int((f.y - 108.f) / 16.f), 0, 5) + fogBias;
    spr(art_.fish[f.sp][f.keeper ? 1 : 0], f.x, f.y, h, sp.pal, faceOf(i) < 0, fog);
    if (f.keeper) {
        float s = 9.f + 2.f * std::sin(age_ * 7.f + f.phase);
        spr(art_.spark, f.x, f.y - h * 0.62f, s, PAL_SUN, false, 0);
    }
}

void Game::backdrop() {
    float day = skyDay();
    uint16_t top, hor;
    skyAt(day, top, hor);
    float dusk = std::clamp((day - 0.55f) / 0.6f, 0.f, 1.f);
    sys_->vdp.setFogColor(lerpC(hor, gs::rgb4(1, 1, 3), dusk));
    uint16_t deepA = lerpC(gs::rgb4(2, 8, 11), gs::rgb4(2, 3, 6), dusk);
    uint16_t deepB = lerpC(gs::rgb4(1, 3, 6), gs::rgb4(1, 1, 3), dusk);
    uint16_t shore = lerpC(gs::rgb4(3, 6, 3), gs::rgb4(3, 3, 3), dusk);
    gs::VDP& v = sys_->vdp;
    const int shore0 = 76, shore1 = 84, rip1 = 102;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        gs::RoadLine& r = v.road[y];
        r.on = false;
        if (y < shore0) {
            float u = float(y) / float(shore0 - 1);
            u = u * u * (3.f - 2.f * u);
            v.lineBackdrop[y] = lerpC(top, hor, u);
            v.lineFog[y] = 0;
        } else if (y < shore1) {
            v.lineBackdrop[y] = shore;
            v.lineFog[y] = 0;
        } else if (y < rip1) {
            r.on = true;
            r.cx = 160;
            r.hw = 420;
            r.v = age_ * 36.f;
            r.pal = PAL_WATER;
            r.band = (int(age_ * 3.f) & 1) ? 1 : 0;
            r.style = 2;
            r.left = r.right = 0;
            v.lineBackdrop[y] = deepA;
            v.lineFog[y] = 0;
        } else {
            float u = float(y - rip1) / float(gs::SCREEN_H - 1 - rip1);
            v.lineBackdrop[y] = lerpC(deepA, deepB, u);
            v.lineFog[y] = uint8_t(std::clamp(int(u * (4.f + dusk * 6.f)), 0, 12));
        }
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    float mag = std::max(0.f, shake_);
    shakeX_ = std::sin(age_ * 73.f) * 4.f * mag;
    shakeY_ = std::cos(age_ * 91.f) * 2.5f * mag;
    backdrop();

    float day = skyDay();
    int lateFog = day > 0.75f ? 2 : 0;
    bool faceLeft = hooked_ >= 0 && fish_[hooked_].x < boatX_ - 4.f;
    float tipX = boatX_ + (faceLeft ? -16.f : 16.f);
    float tipY = 64.f;
    float sag = hooked_ >= 0 ? (1.f - tension_ / 100.f) * 14.f : 8.f;
    float mx = (tipX + lureX_) * 0.5f;
    float my = (tipY + lureY_) * 0.5f + sag;
    int linePal = PAL_INK;
    if (hooked_ >= 0) {
        if (tension_ > 72.f) linePal = PAL_ALERT;
        else if (tension_ < 28.f) linePal = PAL_GOOD;
        else linePal = PAL_AMBER;
    }

    // Earlier sprites sit on top.
    if (mode_ == Mode::Title || mode_ == Mode::Help) text("S3 FISH", 160, 16, 1.5f, PAL_INK);
    if (mode_ == Mode::Win) text("FIVE KEEPERS", 160, 16, 1.15f, PAL_GOOD);
    if (mode_ == Mode::Lose) text("DARK", 160, 16, 1.7f, PAL_ALERT);

    spr(art_.lure, lureX_, lureY_, 16, PAL_FX, false);
    line(tipX, tipY, mx, my, linePal);
    line(mx, my, lureX_, lureY_, linePal);
    spr(art_.angler, boatX_, 88, 46, PAL_BOAT, faceLeft, 0, true);
    spr(art_.boat, boatX_, 100, 18, PAL_BOAT, false, 0, true);
    spr(art_.shadow, boatX_, 104, 8, PAL_BOAT, false, 0, false, true);

    if (showT_ > 0)
        spr(art_.fish[showSp_][showKeep_ ? 1 : 0], boatX_ - 22, 70, showKeep_ ? 18.f : 12.f, SPEC[showSp_].pal, false);
    if (mode_ == Mode::Win) {
        for (int s = 0; s < 5; s++) spr(art_.fish[s][1], 36.f + s * 62.f, 58.f, 20, SPEC[s].pal, s & 1, 0);
    }

    if (hooked_ >= 0) drawOneFish(hooked_, 0);
    for (const auto& p : puff_) {
        if (p.t <= 0) continue;
        float u = 1.f - p.t / 0.4f;
        spr(art_.splash, p.x, p.y, 8.f + u * 22.f, PAL_FX, false, int(u * 6));
    }
    for (int k = 0; k < 3 && hooked_ >= 0; k++) {
        float by = std::fmod(age_ * 28.f + k * 0.33f, 1.f);
        spr(art_.bubble, lureX_ + (k - 1) * 5.f, lureY_ - by * 22.f, 3.f + k, PAL_FX, false);
    }

    int order[NFISH];
    int n = 0;
    for (int i = 0; i < NFISH; i++)
        if (i != hooked_) order[n++] = i;
    std::sort(order, order + n, [&](int a, int b) { return fish_[a].y > fish_[b].y; });
    for (int k = n - 1; k >= 0; --k) drawOneFish(order[k], lateFog);

    const float lilies[] = {46, 156, 268};
    for (float x : lilies) spr(art_.lily, x, 94, 8, PAL_TREE, false);
    for (int i = 0; i < 8; i++) {
        float by = std::fmod(age_ * (10.f + i * 2.f) + i * 37.f, 120.f);
        float bx = 28.f + i * 36.f + std::sin(age_ * 0.3f + i) * 6.f;
        spr(art_.bubble, bx, 214.f - by, 3.f + (i % 3), PAL_FX, false, 2);
    }
    for (int i = 0; i < 6; i++) {
        float x = 18.f + i * 52.f + (i % 2) * 6.f;
        spr(art_.weed, x, 222, 26.f + (i % 3) * 8.f, PAL_TREE, i & 1, 2, true);
    }
    const float rocks[] = {52, 148, 214, 286};
    for (float x : rocks) spr(art_.rock, x, 214, 12, PAL_TREE, false, 3, true);

    struct Tree {
        float x, h;
        int pine;
    };
    const Tree trees[] = {{18, 32, 1}, {64, 38, 0}, {122, 28, 1}, {176, 34, 0}, {236, 40, 1}, {298, 30, 0}};
    for (const Tree& t : trees) spr(t.pine ? art_.pine : art_.tree, t.x, 90, t.h, PAL_TREE, false, 1, true);
    for (int i = 0; i < 2; i++) {
        float x = std::fmod(age_ * (22.f + i * 8.f) + i * 160.f, 360.f) - 20.f;
        int fr = int(age_ * 6.f + i) & 1;
        spr(art_.bird[fr], x, 24.f + i * 12.f, 10, PAL_INK, false, 2);
    }

    if (day < 1.02f) {
        float d = std::clamp(day, 0.f, 1.f);
        float sx = 22.f + d * 276.f;
        float sy = 80.f - std::sin(d * PI) * 58.f;
        float sh = 18.f + (1.f - std::sin(d * PI)) * 8.f;
        int pal = day < 0.62f ? PAL_SUN : PAL_SUNSET;
        spr(art_.sun, sx, sy, sh, pal, false);
        spr(art_.gleam, sx, 96, 7, pal, false, 8);
    } else {
        spr(art_.moon, 228, 26, 16, PAL_INK, false);
        for (int i = 0; i < 7; i++) {
            float tw = 0.5f + 0.5f * std::sin(age_ * 3.f + i);
            if (tw > 0.35f) spr(art_.star, 18.f + i * 44.f, 12.f + (i % 3) * 11.f, 4, PAL_INK, false);
        }
    }
    for (int i = 0; i < 3; i++) {
        float x = std::fmod(24.f + i * 120.f + age_ * (6.f + i), 400.f) - 40.f;
        spr(art_.cloud, x, 16.f + (i % 3) * 8.f, 14.f + (i % 3) * 3.f, PAL_INK, i == 1, 6);
    }

    if (mode_ == Mode::Title) {
        hudCenter(5, "DAYLIGHT IS THE CLOCK", PAL_AMBER);
        hudCenter(6, "FIVE KEEPERS", PAL_GOOD);
        if ((int(age_ * 2.f) & 1) == 0) hudCenter(25, "PRESS START", PAL_INK);
        hudCenter(26, "C  HOW TO FISH", PAL_DIM);
    } else if (mode_ == Mode::Help) {
        const char* lines[] = {"ARROWS MOVE THE BOAT AND LURE", "C HOOKS  HOLD C TO REEL", "FOLLOW THE RUN OR IT BREAKS",
                               "X CUTS THE LINE", "GOLD MARKS A KEEPER", "FIVE KEEPERS BEFORE DARK"};
        for (int i = 0; i < 6; i++) hudCenter(6 + i * 2, lines[i], PAL_INK);
        hudCenter(19, "START TO FISH", PAL_AMBER);
    } else if (mode_ == Mode::Win) {
        hudCenter(5, "DAYLIGHT HELD", PAL_AMBER);
        if ((int(age_ * 2.f) & 1) == 0) hudCenter(26, "PRESS START", PAL_INK);
    } else if (mode_ == Mode::Lose) {
        hudCenter(6, "THE LAKE KEEPS THE REST", PAL_INK);
        char b[24];
        std::snprintf(b, sizeof b, "KEEPERS %d/5", keepers_);
        hudCenter(8, b, PAL_AMBER);
        if ((int(age_ * 2.f) & 1) == 0) hudCenter(26, "PRESS START", PAL_DIM);
    } else {
        if (mode_ == Mode::Pause) {
            hudCenter(10, "PAUSED", PAL_AMBER);
            hudCenter(13, "START  RESUME", PAL_INK);
            hudCenter(15, "ESC  TITLE", PAL_DIM);
        }
        if (msgT_ > 0 && mode_ == Mode::Play) hudCenter(22, msg_, PAL_AMBER);
        if (hooked_ >= 0 && mode_ == Mode::Play) {
            const Fish& f = fish_[hooked_];
            int face = f.runDir >= 0 ? 1 : -1;
            bool match = true;
            if (!bot_) {
                int r = 0;
                if (sys_->pad.down(gs::BTN_LEFT)) r = -1;
                else if (sys_->pad.down(gs::BTN_RIGHT)) r = 1;
                match = r == face;
            }
            if (bite_ > 0) hudCenter(23, face < 0 ? "READY  <" : "READY  >", PAL_AMBER);
            else hudCenter(23, face < 0 ? "FOLLOW  <" : "FOLLOW  >", match ? PAL_GOOD : PAL_ALERT);
            char b[32];
            std::snprintf(b, sizeof b, "%s %s", SPEC[f.sp].name, f.keeper ? "KEEPER" : "SHORT");
            hudCenter(24, b, f.keeper ? PAL_AMBER : PAL_DIM);
            int tn = int(std::lround(tension_ / 10.f));
            int pn = int(std::lround(progress_ / 10.f));
            int tpal = tension_ > 72.f ? PAL_ALERT : tension_ > 40.f ? PAL_AMBER : PAL_GOOD;
            hudText(4, 25, "LINE", tpal);
            pipRow(9, 25, tn, 10, tpal, PAL_DIM);
            hudText(21, 25, "NET", PAL_INK);
            pipRow(25, 25, pn, 10, PAL_GOOD, PAL_DIM);
        } else if (mode_ == Mode::Play) {
            hudCenter(24, "ARROWS MOVE  C HOOKS AND REELS  X CUTS", PAL_DIM);
            int col = 4;
            for (int s = 0; s < 5; s++) {
                int pal = have_[s] ? PAL_GOOD : PAL_DIM;
                hudText(col, 25, SPEC[s].name, pal);
                col += int(std::strlen(SPEC[s].name)) + 1;
            }
        }
        const char* letters = "BTPWC";
        for (int i = 0; i < 5; i++) {
            char s[2] = {letters[i], 0};
            int pal = have_[i] ? PAL_GOOD : PAL_DIM;
            if (hooked_ >= 0 && fish_[hooked_].keeper && fish_[hooked_].sp == i) pal = PAL_AMBER;
            hudText(i * 2, 26, s, pal);
        }
        char b[16];
        std::snprintf(b, sizeof b, "KEEP %d/5", keepers_);
        hudText(11, 26, b, PAL_INK);
        float left = 1.f - std::clamp(clock_ / DAY, 0.f, 1.f);
        int lp = int(std::lround(left * 12.f));
        int lpal = left < 0.22f ? PAL_ALERT : PAL_AMBER;
        hudText(21, 26, "DAY", lpal);
        pipRow(25, 26, lp, 12, lpal, PAL_DIM);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.setFogColor(gs::rgb4(8, 10, 12));
    sys.apu.setMaster(0.85f);
    sys.apu.setEcho(0.2f, 0.28f, 0.16f);
    sys.apu.setPatch(0, waterPad());
    sys.apu.setPatch(1, bell());
    sys.apu.keyOn(0, 146.f, 0.06f);
    padOn_ = true;
    seed();
    boatX_ = lureX_ = 160;
    lureY_ = 140;
    if (bot_) startDay();
    else mode_ = Mode::Title;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (mode_ != Mode::Pause) {
        age_ += DT;
        tickFx();
    }
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        swim();
        showcase();
        if (pad.pressed(gs::BTN_START)) {
            blip(true);
            startDay();
        } else if (pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A)) {
            blip(false);
            mode_ = Mode::Help;
        } else if (pad.pressed(gs::BTN_MODE)) {
            sys.quit();
        }
    } else if (mode_ == Mode::Help) {
        swim();
        showcase();
        if (pad.pressed(gs::BTN_START)) {
            blip(true);
            startDay();
        } else if (pad.pressed(gs::BTN_MODE) || pad.pressed(gs::BTN_C)) {
            mode_ = Mode::Title;
        }
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else updatePlay();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
        else if (pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            over_ = false;
            seed();
        }
    } else if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_MODE))) {
        mode_ = Mode::Title;
        over_ = false;
        won_ = false;
        seed();
        boatX_ = lureX_ = 160;
        lureY_ = 140;
    } else if (mode_ == Mode::Win || mode_ == Mode::Lose) {
        swim();
    }

    audio();
    draw();
}

}  // namespace fish

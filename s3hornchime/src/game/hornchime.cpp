#include "game/hornchime.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace hornchime {

int Game::clockSec() const {
    int t = kStartSec + playFrames_ / kFpc;
    return t < 0 ? 0 : t;
}

int Game::framesUntilHour() const {
    int sec = clockSec();
    int sub = playFrames_ % kFpc;
    if (sec > kHourSec) return -((sec - kHourSec) * kFpc + sub);
    if (sec == kHourSec) return -sub;
    int secLeft = kHourSec - sec;
    return (secLeft - 1) * kFpc + (kFpc - sub);
}

bool Game::onHour() const {
    int sec = clockSec();
    return sec >= kHourSec && sec < kHourSec + kGraceSec;
}

bool Game::pastHour() const { return clockSec() >= kHourSec + kGraceSec; }

void Game::split(int& h, int& m, int& s) const {
    if (froze_) {
        h = struckH_;
        m = struckM_;
        s = struckS_;
        return;
    }
    int t = (mode_ == Mode::Title) ? (11 * 3600 + 59 * 60 + (titleFrames_ / kFpc) % 60) : clockSec();
    h = (t / 3600) % 12;
    if (h == 0) h = 12;
    m = (t / 60) % 60;
    s = t % 60;
}

int Game::hour() const {
    int h, m, s;
    split(h, m, s);
    return h;
}

int Game::minute() const {
    int h, m, s;
    split(h, m, s);
    return m;
}

int Game::second() const {
    int h, m, s;
    split(h, m, s);
    return s;
}

void Game::begin() {
    over_ = won_ = froze_ = false;
    playFrames_ = titleFrames_ = noteLeft_ = anim_ = calls_ = misses_ = 0;
    lastSec_ = -1;
    tickLeft_ = 0;
    bellSwing_ = 0;
    reason_ = "";
    mode_ = Mode::Wait;
    sys_->apu.silence();
}

void Game::beginChime() {
    int h, m, s;
    froze_ = false;
    split(h, m, s);
    struckH_ = h;
    struckM_ = m;
    struckS_ = s;
    froze_ = true;
    won_ = false;
    reason_ = "CHIME";
    anim_ = 0;
    bellSwing_ = 0;
    mode_ = Mode::Chime;
    sys_->apu.keyOn(0, 392.f, 0.28f);
    sys_->apu.keyOn(1, 523.f, 0.22f);
    sys_->apu.keyOn(2, 784.f, 0.16f);
}

void Game::beginFail(const char* why) {
    reason_ = why;
    won_ = false;
    over_ = true;
    anim_ = 0;
    mode_ = Mode::Over;
    sys_->apu.noiseBurst(0.16f, 70.f, 0.14f);
}

void Game::sound() {
    calls_++;
    if (pastHour()) {
        beginFail("the hour passed in silence");
        return;
    }
    if (onHour()) {
        beginChime();
        return;
    }
    noteLeft_ = kNote;
    mode_ = Mode::Note;
    sys_->apu.keyOn(0, 349.f, 0.22f);
    sys_->apu.keyOn(1, 523.f, 0.1f);
}

void Game::tickClock() {
    if (mode_ == Mode::Title || mode_ == Mode::Pause || mode_ == Mode::Over || froze_) return;
    int sec = clockSec();
    if (sec != lastSec_) {
        lastSec_ = sec;
        tickLeft_ = 4;
        float f = (sec == kHourSec) ? 880.f : 220.f;
        sys_->apu.tone(0, f, 0.04f);
    } else if (tickLeft_ > 0) {
        if (--tickLeft_ == 0) sys_->apu.tone(0, 0, 0);
    }
}

void Game::spr(const gs::Image& img, float cx, float cy, int pal, int dw, int dh) {
    if (img.w == 0) return;
    gs::Sprite s;
    s.img = img;
    s.w = dw > 0 ? int16_t(dw) : img.w;
    s.h = dh > 0 ? int16_t(dh) : img.h;
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::hand(float cx, float cy, float ang, float len, int pal) {
    int n = int(len / 5.f);
    if (n < 2) n = 2;
    for (int i = 1; i <= n; i++) {
        float t = i / float(n);
        float x = cx + std::cos(ang) * len * t;
        float y = cy + std::sin(ang) * len * t;
        spr(art_.dot, x, y, pal, i == n ? 5 : 3, i == n ? 5 : 3);
    }
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    uint16_t top = gs::rgb4(1, 1, 3);
    uint16_t mid = gs::rgb4(4, 3, 6);
    uint16_t bot = gs::rgb4(3, 3, 2);
    if (mode_ == Mode::Chime || (mode_ == Mode::Over && won_)) {
        top = gs::rgb4(6, 4, 2);
        mid = gs::rgb4(10, 7, 2);
    }
    if (mode_ == Mode::Over && !won_) mid = gs::rgb4(5, 1, 2);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H - 1);
        auto mix = [](uint16_t a, uint16_t b, float t) {
            int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
            int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
            auto L = [&](int p, int q) { return int(p + (q - p) * t + 0.5f); };
            return gs::rgb4(L(ar, br), L(ag, bg), L(ab, bb));
        };
        v.lineBackdrop[y] = u < 0.55f ? mix(top, mid, u / 0.55f) : mix(mid, bot, (u - 0.55f) / 0.45f);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    sky();

    float sway = (mode_ == Mode::Chime) ? std::sin(anim_ * 0.45f) * 8.f : 0.f;
    spr(art_.tower, 250.f, 128.f, PAL_STONE);
    spr(art_.face, 250.f, 78.f, PAL_FACE);
    spr(art_.bell, 250.f + sway, 118.f, (mode_ == Mode::Chime || won_) ? PAL_GOLD : PAL_STONE);

    int h, m, s;
    split(h, m, s);
    const float kPi = 3.1415926f;
    float ha = ((h % 12) + m / 60.f) / 12.f * kPi * 2.f - kPi * 0.5f;
    float ma = m / 60.f * kPi * 2.f - kPi * 0.5f;
    float sa = s / 60.f * kPi * 2.f - kPi * 0.5f;
    hand(250.f, 78.f, ha, 10.f, PAL_FACE);
    hand(250.f, 78.f, ma, 14.f, PAL_GOLD);
    hand(250.f, 78.f, sa, 16.f, PAL_BAD);

    float puff = 0.f;
    if (mode_ == Mode::Note) puff = 4.f + (kNote - noteLeft_) * 0.15f;
    if (mode_ == Mode::Chime) puff = 3.f;
    spr(art_.player, 62.f, 150.f, PAL_COAT);
    spr(art_.horn, 118.f + puff, 142.f, PAL_BRASS);

    if (mode_ == Mode::Wait || mode_ == Mode::Note) {
        int until = framesUntilHour();
        int span = 56;
        int filled = until <= 0 ? span : span - until;
        if (filled < 2) filled = 2;
        if (filled > span) filled = span;
        bool hot = onHour() || (until > 0 && until <= kFpc * 2);
        spr(art_.bar, 28.f, 168.f - filled * 0.5f, hot ? PAL_GOLD : PAL_BRASS, 8, filled);
    }

    char buf[80];
    int f = int(sys_->frame);
    if (mode_ == Mode::Title) {
        hudC(2, "S3 HORNCHIME", PAL_GOLD);
        hudC(19, "A SHORT HORN", PAL_HUD);
        hudC(21, "SOUND IT ON THE HOUR", PAL_GOLD);
        hudC(22, "OR THE NOTE DIES EARLY", PAL_HUD);
        if ((f / 16) % 2 == 0) hudC(26, "PRESS START", PAL_GOLD);
        const char* ver = S3_VERSION_STRING;
        hud(40 - int(std::strlen(ver)), 0, ver, PAL_DIM);
    } else if (mode_ == Mode::Over && won_) {
        hudC(2, "THE HOUR CHIMES", PAL_GOLD);
        std::snprintf(buf, sizeof buf, "%d:%02d:%02d", hour(), minute(), second());
        hudC(21, buf, PAL_HUD);
        hudC(23, "THE SHORT HORN HELD", PAL_GOLD);
    } else if (mode_ == Mode::Over) {
        hudC(2, "NO CHIME", PAL_BAD);
        hudC(22, reason_, PAL_HUD);
        if ((f / 16) % 2 == 0) hudC(26, "START", PAL_GOLD);
    } else if (mode_ == Mode::Chime) {
        hudC(2, "THE HOUR CHIMES", PAL_GOLD);
        std::snprintf(buf, sizeof buf, "%d:%02d:%02d", hour(), minute(), second());
        hudC(22, buf, PAL_HUD);
    } else {
        std::snprintf(buf, sizeof buf, "%d:%02d:%02d", hour(), minute(), second());
        hud(1, 1, buf, onHour() ? PAL_GOLD : PAL_HUD);
        std::snprintf(buf, sizeof buf, "BREATH %d", kBreaths - misses_);
        hud(28, 1, buf, misses_ ? PAL_BAD : PAL_DIM);
        if (mode_ == Mode::Pause) hudC(24, "PAUSED", PAL_GOLD);
        else if (mode_ == Mode::Note) {
            hudC(24, "THE HORN IS SHORT", PAL_GOLD);
            hudC(26, "HOLD IT TO THE HOUR", PAL_DIM);
        } else if (onHour()) {
            hudC(24, "THE HOUR IS HERE", PAL_GOLD);
            hudC(26, "C SOUNDS THE HORN", PAL_DIM);
        } else {
            int until = framesUntilHour();
            if (until > 0) {
                std::snprintf(buf, sizeof buf, "HOUR IN %d", (until + kFpc - 1) / kFpc);
                hudC(24, buf, PAL_HUD);
            }
            hudC(26, "C SOUNDS THE HORN", PAL_DIM);
        }
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.8f);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(2, 2, 3));
    mode_ = Mode::Title;
    if (bot_) begin();
    draw();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        titleFrames_++;
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C)) begin();
    } else if (mode_ == Mode::Wait) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            held_ = Mode::Wait;
            mode_ = Mode::Pause;
        } else {
            playFrames_++;
            tickClock();
            bool tap = false;
            if (bot_) tap = framesUntilHour() == 8;
            else tap = pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A);
            if (tap) sound();
            else if (pastHour()) beginFail("the hour passed in silence");
        }
    } else if (mode_ == Mode::Note) {
        playFrames_++;
        tickClock();
        noteLeft_--;
        if (onHour()) beginChime();
        else if (pastHour()) beginFail("the hour passed in silence");
        else if (noteLeft_ <= 0) {
            sys.apu.keyOff(0);
            sys.apu.keyOff(1);
            misses_++;
            if (misses_ >= kBreaths) beginFail("the horn died short of the hour");
            else mode_ = Mode::Wait;
        }
    } else if (mode_ == Mode::Chime) {
        anim_++;
        bellSwing_++;
        if (anim_ == 18) sys.apu.keyOn(2, 659.f, 0.18f);
        if (anim_ == 36) sys.apu.keyOn(1, 523.f, 0.16f);
        if (anim_ >= 70) {
            won_ = froze_ && std::strcmp(reason_, "CHIME") == 0 && calls_ >= 1;
            over_ = true;
            mode_ = Mode::Over;
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = held_;
    } else if (mode_ == Mode::Over) {
        anim_++;
        if (!bot_ && !won_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C))) begin();
    }
    draw();
}

}  // namespace hornchime

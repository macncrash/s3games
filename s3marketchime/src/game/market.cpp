#include "game/market.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace marketchime {
namespace {

constexpr int kCoinV[4] = {1, 5, 10, 25};
constexpr float kCoinX[4] = {52.f, 108.f, 168.f, 228.f};
constexpr float kCoinY = 196.f;
constexpr float kPi = 3.14159265f;

struct Sale {
    const char* name;
    int price;
    int pay;
};
constexpr Sale kSales[3] = {
    {"APPLE", 4, 10},
    {"LOAF", 7, 10},
    {"MILK", 18, 25},
};

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    rules_ = audit();
    if (!rules_) std::fprintf(stderr, "s3marketchime rules failed\n");
    toTitle();
}

void Game::toTitle() {
    mode_ = Mode::Title;
    reason_ = "";
    over_ = false;
    won_ = false;
    locked_ = false;
    age_ = 0;
    playFrames_ = 0;
    lockedSec_ = 0;
    served_ = 0;
    faults_ = 0;
    sale_ = 0;
    dish_ = 0;
    stackN_ = 0;
    cursor_ = 0;
    wait_ = 0;
    repL_ = 0;
    repR_ = 0;
    chimeT_ = 0;
    strikes_ = 0;
    flashT_ = 0;
    beepN_ = 0;
}

void Game::openStall() {
    toTitle();
    mode_ = Mode::Play;
    playFrames_ = 0;
    wait_ = 6;
    sale_ = 0;
}

void Game::nextSale() {
    dish_ = 0;
    stackN_ = 0;
    cursor_ = 0;
    sale_ = (sale_ + 1) % 3;
    wait_ = 4;
    flashT_ = 18;
}

int Game::dueOf() const {
    const Sale& s = kSales[sale_ % 3];
    return s.pay - s.price;
}

int Game::clockOf(int frames) const {
    if (frames < 0) frames = 0;
    return (kHourSec - kLeadSec) + frames / kFpc;
}

bool Game::onHour() const {
    if (mode_ == Mode::Title) return false;
    int sec = locked_ ? lockedSec_ : clockOf(playFrames_);
    return sec >= kHourSec && sec < kHourSec + kGraceSec;
}

bool Game::onTheHour() const { return onHour(); }

bool Game::pastHour() const {
    if (locked_) return false;
    return clockOf(playFrames_) >= kHourSec + kGraceSec;
}

void Game::face(int sec, int& h, int& m, int& s) const {
    if (sec < 0) sec = 0;
    h = (sec / 3600) % 24;
    m = (sec / 60) % 60;
    s = sec % 60;
}

int Game::hour() const {
    int h, m, s;
    face(locked_ ? lockedSec_ : clockOf(mode_ == Mode::Title ? 0 : playFrames_), h, m, s);
    return h;
}

int Game::minute() const {
    int h, m, s;
    face(locked_ ? lockedSec_ : clockOf(mode_ == Mode::Title ? 0 : playFrames_), h, m, s);
    return m;
}

int Game::second() const {
    int h, m, s;
    face(locked_ ? lockedSec_ : clockOf(mode_ == Mode::Title ? 0 : playFrames_), h, m, s);
    return s;
}

bool Game::audit() {
    auto bad = [](const char* w) {
        std::fprintf(stderr, "s3marketchime %s\n", w);
        return false;
    };
    for (int i = 0; i < 3; i++) {
        int due = kSales[i].pay - kSales[i].price;
        if (due <= 0 || due > 24) return bad("due out of range");
        int left = due;
        for (int c = 3; c >= 0 && left > 0; c--)
            while (left >= kCoinV[c]) left -= kCoinV[c];
        if (left != 0) return bad("change cannot be made");
    }
    if (clockOf(kLeadSec * kFpc) != kHourSec) return bad("noon is not on the lead");
    if (clockOf(kLeadSec * kFpc - 1) >= kHourSec) return bad("hour arrived early");
    int graceEnd = (kLeadSec + kGraceSec) * kFpc;
    if (clockOf(graceEnd - 1) >= kHourSec + kGraceSec) return bad("grace ended early");
    if (clockOf(graceEnd) < kHourSec + kGraceSec) return bad("grace ran long");
    return true;
}

void Game::blip(float freq, int frames) {
    beepF_ = freq;
    beepN_ = frames;
}

void Game::nudge(int dir) {
    cursor_ = (cursor_ + dir + 4) % 4;
    blip(420.f + float(cursor_) * 50.f, 3);
}

void Game::dropCoin() {
    if (mode_ != Mode::Play || stackN_ >= 8) return;
    int v = kCoinV[cursor_];
    if (dish_ + v > 40) return;
    stack_[stackN_++] = v;
    dish_ += v;
    blip(360.f + float(v) * 18.f, 4);
}

void Game::undoCoin() {
    if (mode_ != Mode::Play || stackN_ <= 0) return;
    dish_ -= stack_[--stackN_];
    blip(240.f, 4);
}

void Game::beginChime() {
    locked_ = true;
    lockedSec_ = clockOf(playFrames_);
    won_ = true;
    reason_ = "CHIME";
    mode_ = Mode::Chime;
    chimeT_ = 0;
    strikes_ = 0;
    flashT_ = 30;
    served_++;
    blip(880.f, 8);
    sys_->rumble(0.25f, 0.6f, 120);
    sys_->setLight(255, 210, 80);
}

void Game::beginFail(const char* why) {
    reason_ = why;
    won_ = false;
    over_ = true;
    mode_ = Mode::Fail;
    blip(110.f, 14);
    sys_->apu.noiseBurst(0.12f, 400.f, 0.22f);
    sys_->rumble(0.45f, 0.15f, 80);
    sys_->setLight(140, 30, 30);
}

void Game::hand() {
    if (mode_ != Mode::Play) return;
    const int due = dueOf();
    if (dish_ == due && due > 0 && onHour()) {
        beginChime();
        return;
    }
    if (dish_ == due && due > 0 && !pastHour()) {
        served_++;
        blip(620.f, 6);
        if (served_ >= 5) {
            beginFail("EARLY");
            return;
        }
        nextSale();
        return;
    }
    faults_++;
    dish_ = 0;
    stackN_ = 0;
    blip(140.f, 8);
    sys_->rumble(0.3f, 0.1f, 50);
    if (faults_ >= 2 || pastHour()) beginFail(pastHour() ? "GONE" : (dish_ > due ? "HEAVY" : "SHORT"));
}

void Game::readInput() {
    const gs::Pad& pad = sys_->pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) openStall();
        return;
    }
    if (mode_ == Mode::Fail || mode_ == Mode::Leave) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) toTitle();
        return;
    }
    if (mode_ != Mode::Play) return;
    if (pad.pressed(gs::BTN_LEFT)) {
        nudge(-1);
        repL_ = 12;
    } else if (pad.down(gs::BTN_LEFT)) {
        if (--repL_ <= 0) {
            nudge(-1);
            repL_ = 6;
        }
    } else repL_ = 0;
    if (pad.pressed(gs::BTN_RIGHT)) {
        nudge(1);
        repR_ = 12;
    } else if (pad.down(gs::BTN_RIGHT)) {
        if (--repR_ <= 0) {
            nudge(1);
            repR_ = 6;
        }
    } else repR_ = 0;
    if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_TURBO)) dropCoin();
    else if (pad.pressed(gs::BTN_B)) undoCoin();
    if (pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_Z)) hand();
}

void Game::driveBot() {
    for (int i = 0; i < gs::BTN_COUNT; i++) sys_->pad.keys[i] = false;
    if (mode_ == Mode::Title) {
        if (age_ > 8) sys_->pad.keys[gs::BTN_START] = true;
        return;
    }
    if (mode_ != Mode::Play) return;
    if (wait_ > 0) {
        wait_--;
        return;
    }
    if (served_ >= 1 && !onHour()) return;
    if ((age_ & 1) != 0) return;
    const int due = dueOf();
    if (dish_ == due) {
        sys_->pad.keys[gs::BTN_C] = true;
        wait_ = 2;
        return;
    }
    int need = due - dish_;
    int want = 0;
    if (need >= 25) want = 3;
    else if (need >= 10) want = 2;
    else if (need >= 5) want = 1;
    if (need <= 0) {
        sys_->pad.keys[gs::BTN_B] = true;
        return;
    }
    if (cursor_ == want) sys_->pad.keys[gs::BTN_A] = true;
    else if (cursor_ < want) sys_->pad.keys[gs::BTN_RIGHT] = true;
    else sys_->pad.keys[gs::BTN_LEFT] = true;
}

void Game::logic() {
    age_++;
    if (flashT_ > 0) flashT_--;
    if (mode_ == Mode::Play) {
        if (pastHour()) {
            beginFail("GONE");
            return;
        }
        playFrames_++;
    } else if (mode_ == Mode::Chime) {
        chimeT_++;
        if (chimeT_ % 6 == 0 && strikes_ < 12) {
            strikes_++;
            blip(660.f + float(strikes_) * 18.f, 4);
            sys_->apu.tone(1, 990.f, 0.05f);
        }
        if (strikes_ >= 12 && chimeT_ > 12 * 6 + 18) {
            mode_ = Mode::Leave;
            chimeT_ = 0;
        }
    } else if (mode_ == Mode::Leave) {
        chimeT_++;
        if (chimeT_ > 24) over_ = true;
    }
}

void Game::audio() {
    if (beepN_ > 0) {
        sys_->apu.tone(0, beepF_, 0.07f);
        beepN_--;
    } else sys_->apu.tone(0, 0.f, 0.f);
    if (mode_ != Mode::Chime) sys_->apu.tone(1, 0.f, 0.f);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    if (!s) return;
    int n = int(std::strlen(s));
    hud(20 - n / 2, row, s, pal);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 2.f || m.h < 1 || m.w < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 400));
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 400));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::sprI(const gs::Image& img, float cx, float cy, float h, int pal) {
    if (h < 1.f || img.h < 1 || img.w < 1) return;
    float w = h * float(img.w) / float(img.h);
    gs::Sprite s;
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 400));
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 400));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = img;
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::solid(float x, float y, float w, float h, int pal) {
    if (w < 1.f || h < 1.f) return;
    gs::Sprite s;
    s.img = art_.solid;
    s.x = int16_t(std::lround(x));
    s.y = int16_t(std::lround(y));
    s.w = int16_t(std::max(1, int(std::lround(w))));
    s.h = int16_t(std::max(1, int(std::lround(h))));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    const bool gone = mode_ == Mode::Fail;
    const bool gold = mode_ == Mode::Chime || mode_ == Mode::Leave;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int r, g, b;
        if (y < 96) {
            r = 4 + y / 24;
            g = 4 + y / 30;
            b = 10 - y / 20;
        } else {
            r = 6;
            g = 4;
            b = 3;
        }
        if (gone) {
            r = std::min(15, r + 4);
            b = std::max(1, b - 4);
        } else if (gold) {
            r = std::min(15, r + 3);
            g = std::min(15, g + 3);
        }
        v.lineBackdrop[y] = gs::rgb4(std::clamp(r, 0, 15), std::clamp(g, 0, 15), std::clamp(b, 0, 15));
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::clockAt(float cx, float cy) {
    spr(art_.clock, cx, cy, 52.f, PAL_CLOCK);
    int sec = locked_ ? lockedSec_ : clockOf(mode_ == Mode::Title ? 0 : playFrames_);
    int h, m, s;
    face(sec, h, m, s);
    float ha = ((float(h % 12) + float(m) / 60.f) / 12.f) * 2.f * kPi - kPi * 0.5f;
    float ma = (float(m) / 60.f) * 2.f * kPi - kPi * 0.5f;
    auto ray = [&](float ang, float len, int pal) {
        for (int i = 4; i < int(len); i += 3) {
            float x = cx + std::cos(ang) * float(i);
            float y = cy + std::sin(ang) * float(i);
            sprI(art_.solid, x, y, 3.f, pal);
        }
    };
    ray(ha, 14.f, PAL_HUD);
    ray(ma, 18.f, PAL_WARN);
    (void)s;
    float swing = (mode_ == Mode::Chime) ? std::sin(float(chimeT_) * 0.7f) * 8.f : 0.f;
    spr(art_.bell, cx, cy + 34.f + swing * 0.15f, 22.f, PAL_CLOCK);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    spr(art_.post, 36.f, 108.f, 90.f, PAL_STALL);
    spr(art_.post, 284.f, 108.f, 90.f, PAL_STALL);
    spr(art_.awning, 160.f, 58.f, 34.f, PAL_STALL);
    spr(art_.counter, 150.f, 128.f, 34.f, PAL_STALL);
    spr(art_.crate, 78.f, 112.f, 26.f, PAL_GOODS);
    spr(art_.clerk, 118.f, 100.f, 58.f, PAL_CLERK);
    if (mode_ == Mode::Play || mode_ == Mode::Chime || mode_ == Mode::Title) {
        spr(art_.buyer, 210.f, 102.f, 56.f, PAL_CUST);
        int g = sale_ % 3;
        spr(art_.good[g], 168.f, 108.f, g == 2 ? 26.f : 20.f, PAL_GOODS);
    }
    clockAt(286.f, 36.f);

    for (int i = 0; i < 4; i++) {
        float lift = (i == cursor_ && mode_ == Mode::Play) ? -6.f : 0.f;
        spr(art_.coin[i], kCoinX[i], kCoinY + lift, 14.f + float(i == 3 ? 0 : i) * 2.f, PAL_COIN);
    }
    for (int i = 0; i < stackN_ && i < 8; i++) {
        int which = 0;
        for (int c = 0; c < 4; c++)
            if (kCoinV[c] == stack_[i]) which = c;
        spr(art_.coin[which], 150.f + float((i % 4) * 14 - 21), 150.f - float(i / 4) * 8.f, 12.f, PAL_COIN);
    }

    if (mode_ == Mode::Title) {
        sprI(art_.title, 150.f, 24.f, float(art_.title.h), PAL_HUD);
        sprI(art_.sub, 150.f, 46.f, float(art_.sub.h), PAL_WARN);
        hudC(26, "START", PAL_HUD);
    } else if (mode_ == Mode::Chime) {
        sprI(art_.chime, 140.f, 22.f, float(art_.chime.h), PAL_OK);
    } else if (mode_ == Mode::Leave) {
        sprI(art_.leave, 150.f, 28.f, float(art_.leave.h), PAL_OK);
    } else if (mode_ == Mode::Fail) {
        sprI(art_.gone, 150.f, 22.f, float(art_.gone.h), PAL_BAD);
    }
    if (flashT_ > 0 && (mode_ == Mode::Play || mode_ == Mode::Chime))
        sprI(art_.exact, 150.f, 78.f, float(art_.exact.h), PAL_OK);

    char line[48];
    int hh = hour(), mm = minute(), ss = second();
    if (hh > 12) hh -= 12;
    if (hh == 0) hh = 12;
    std::snprintf(line, sizeof line, "%d:%02d:%02d", hh, mm, ss);
    hud(1, 1, line, onHour() ? PAL_OK : PAL_HUD);

    if (mode_ == Mode::Play || mode_ == Mode::Chime || mode_ == Mode::Leave) {
        const Sale& sale = kSales[sale_ % 3];
        std::snprintf(line, sizeof line, "%s  DUE %d  DISH %d", sale.name, dueOf(), dish_);
        hud(1, 3, line, dish_ == dueOf() ? PAL_OK : PAL_HUD);
        std::snprintf(line, sizeof line, "SERVED %d  FAULT %d", served_, faults_);
        hud(1, 24, line, PAL_DIM);
        hud(1, 26, "A COIN  B BACK  C HAND", PAL_HUD);
        const char* tag = "1";
        if (cursor_ == 1) tag = "5";
        else if (cursor_ == 2) tag = "10";
        else if (cursor_ == 3) tag = "25";
        std::snprintf(line, sizeof line, "COIN %s", tag);
        hud(28, 26, line, PAL_WARN);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (bot_) driveBot();
    readInput();
    logic();
    audio();
    draw();
    sys.vdp.render(sys.fb);
}

}  // namespace marketchime

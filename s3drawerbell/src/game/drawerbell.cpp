#include "game/drawerbell.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace drawerbell {
namespace {

const int kCents[kCoins] = {1, 5, 10, 1, 25, 5};
const int kKind[kCoins] = {0, 1, 2, 0, 3, 1};
const int kWant[kCoins] = {0, 1, 1, 0, 1, 0};

float homeX(int i) { return 48.f + float(i) * 42.f; }

}  // namespace

int Game::drawerCents() const {
    int n = 0;
    for (int i = 0; i < kCoins; i++)
        if (coin_[i].in) n += coin_[i].cents;
    return n;
}

bool Game::matched() const {
    for (int i = 0; i < kCoins; i++)
        if (coin_[i].in != coin_[i].want) return false;
    return drawerCents() == kTarget;
}

void Game::blip(float freq) { sys_->apu.tone(1, freq, 0.04f); }

void Game::spr(const gs::Image& img, float cx, float cy, int pal) {
    if (img.w == 0 || img.h == 0) return;
    gs::Sprite s;
    s.img = img;
    s.w = img.w;
    s.h = img.h;
    s.x = int16_t(std::lround(cx - img.w * 0.5f));
    s.y = int16_t(std::lround(cy - img.h * 0.5f));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::resetTill() {
    for (int i = 0; i < kCoins; i++) {
        coin_[i].cents = kCents[i];
        coin_[i].kind = kKind[i];
        coin_[i].want = kWant[i] != 0;
        coin_[i].in = false;
        coin_[i].x = homeX(i);
    }
    sel_ = 1;
    foul_ = 0;
}

void Game::begin() {
    rung_ = false;
    won_ = false;
    over_ = false;
    left_ = false;
    pause_ = false;
    dead_ = 0;
    tryNo_ = 1;
    swing_ = 0;
    hold_ = 0;
    t_ = 0;
    clerkX_ = 36.f;
    resetTill();
    mode_ = Mode::Play;
    blip(392.f);
}

void Game::file(int i) {
    if (mode_ != Mode::Play || i < 0 || i >= kCoins) return;
    coin_[i].in = !coin_[i].in;
    blip(coin_[i].in ? 620.f : 440.f);
}

void Game::dieTry() {
    if (mode_ != Mode::Play || rung_) return;
    dead_++;
    swing_ = 2;
    foul_ = 40;
    sys_->apu.tone(0, 146.f, 0.08f);
    sys_->apu.noiseBurst(0.16f, 380.f, 0.08f);
    if (!bot_) sys_->rumble(0.35f, 0.1f, 70);
    if (dead_ >= kMaxDead) {
        mode_ = Mode::Lose;
        won_ = false;
        over_ = true;
        if (!bot_) sys_->setLight(140, 24, 24);
        return;
    }
    tryNo_ = dead_ + 1;
    resetTill();
    foul_ = 40;
    if (!bot_) sys_->setLight(120, 48, 28);
}

void Game::ring() {
    if (rung_ || dead_ >= kMaxDead || !matched()) return;
    rung_ = true;
    tryNo_ = dead_ + 1;
    mode_ = Mode::Ring;
    hold_ = 0;
    swing_ = 18;
    sys_->apu.tone(0, 523.f, 0.1f);
    sys_->apu.tone(2, 784.f, 0.07f);
    if (!bot_) {
        sys_->rumble(0.2f, 0.5f, 140);
        sys_->setLight(255, 196, 64);
    }
}

void Game::shut() {
    if (mode_ != Mode::Play || pause_ || rung_) return;
    if (!matched()) {
        dieTry();
        return;
    }
    ring();
}

void Game::botAct() {
    for (int i = 0; i < kCoins; i++) {
        if (coin_[i].in != coin_[i].want) {
            sel_ = i;
            file(i);
            return;
        }
    }
    shut();
}

void Game::human(const gs::Pad& pad) {
    if (pad.pressed(gs::BTN_LEFT)) {
        sel_ = (sel_ + kCoins - 1) % kCoins;
        blip(360.f);
    }
    if (pad.pressed(gs::BTN_RIGHT)) {
        sel_ = (sel_ + 1) % kCoins;
        blip(360.f);
    }
    if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B)) file(sel_);
    if (pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_TURBO)) shut();
}

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    uint16_t top = gs::rgb4(2, 2, 4);
    uint16_t mid = gs::rgb4(6, 4, 3);
    uint16_t bot = gs::rgb4(3, 2, 1);
    if (mode_ == Mode::Ring || mode_ == Mode::Leave) mid = gs::rgb4(12, 8, 3);
    if (mode_ == Mode::Lose) {
        top = gs::rgb4(2, 0, 1);
        mid = gs::rgb4(5, 1, 1);
        bot = gs::rgb4(1, 0, 1);
    }
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H - 1);
        auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
        auto mix = [&](uint16_t a, uint16_t b, float t) {
            t = std::clamp(t, 0.f, 1.f);
            auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
            return gs::rgb4(L(8), L(4), L(0));
        };
        v.lineBackdrop[y] = u < 0.45f ? mix(top, mid, u / 0.45f) : mix(mid, bot, (u - 0.45f) / 0.55f);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    sky();

    int f = int(sys_->frame);
    float ox = std::sin(f * 0.85f) * float(swing_) * 0.55f;
    int bp = (mode_ == Mode::Ring || mode_ == Mode::Leave) ? PAL_LIT : PAL_BELL;
    spr(art_.bell, 250.f + ox, 42.f, bp);
    spr(art_.clapper, 250.f + ox * 1.3f, 58.f, PAL_BELL);
    spr(art_.slip, 46.f, 78.f, PAL_SLIP);
    spr(art_.drawer, 168.f, 176.f, PAL_WOOD);

    if (mode_ != Mode::Title && mode_ != Mode::Lose) spr(art_.clerk, clerkX_, 132.f, PAL_CLERK);

    for (int i = 0; i < kCoins; i++) {
        float x = coin_[i].in ? 78.f + float(i) * 28.f : coin_[i].x;
        float y = coin_[i].in ? 168.f : 128.f;
        if (mode_ == Mode::Title) {
            x = homeX(i);
            y = 128.f + std::sin((f + i * 9) * 0.08f) * 3.f;
        }
        int pal = PAL_COIN;
        spr(art_.coin[coin_[i].kind], x, y, pal);
        if (mode_ == Mode::Play && i == sel_ && !coin_[i].in) spr(art_.mark, x, y - 16.f, PAL_GOLD);
        if (coin_[i].in && mode_ != Mode::Title) spr(art_.mark, x, 154.f, coin_[i].want ? PAL_GOLD : PAL_BAD);
    }

    char buf[56];
    if (mode_ == Mode::Title) {
        hudC(1, "S3 DRAWERBELL", PAL_GOLD);
        hudC(24, "A SHORT DRAWER", PAL_GOLD);
        hudC(25, "BELL BEFORE THE THIRD TRY DIES", PAL_HUD);
        if ((f & 16) == 0) hudC(27, "PRESS START", PAL_GOLD);
        const char* ver = S3_VERSION_STRING;
        hud(40 - int(std::strlen(ver)), 0, ver, PAL_DIM);
    } else if (mode_ == Mode::Lose) {
        hudC(1, "THIRD TRY DIED", PAL_BAD);
        hudC(25, "THE BELL STAYED QUIET", PAL_HUD);
        if ((f & 16) == 0) hudC(27, "START", PAL_GOLD);
    } else if (mode_ == Mode::Ring || mode_ == Mode::Leave) {
        hudC(1, mode_ == Mode::Leave ? "LEAVE THE TILL" : "THE BELL RINGS", PAL_GOLD);
        std::snprintf(buf, sizeof buf, "TRY %d OF %d", tryNo_, kMaxDead);
        hudC(24, buf, PAL_GOLD);
        std::snprintf(buf, sizeof buf, "TILL %d", drawerCents());
        hudC(25, buf, PAL_HUD);
    } else {
        std::snprintf(buf, sizeof buf, "SLIP %d", kTarget);
        hud(1, 1, buf, PAL_GOLD);
        std::snprintf(buf, sizeof buf, "TRY %d", tryNo_);
        hud(32, 1, buf, dead_ ? PAL_BAD : PAL_HUD);
        std::snprintf(buf, sizeof buf, "TILL %d", drawerCents());
        hudC(24, buf, matched() ? PAL_GOLD : PAL_HUD);
        if (foul_ > 0) hudC(25, "NOT THE SLIP", PAL_BAD);
        else if (pause_) hudC(25, "PAUSED", PAL_GOLD);
        else if (matched()) hudC(25, "SHUT THE DRAWER", PAL_GOLD);
        else hudC(25, "FILE THE SLIP", PAL_HUD);
        hudC(27, "LR COIN  A FILE  C SHUT", PAL_DIM);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.hudEnabled = true;
    sys.apu.setMaster(0.7f);
    int wantSum = 0;
    for (int i = 0; i < kCoins; i++)
        if (kWant[i]) wantSum += kCents[i];
    rules_ = kMaxDead == 3 && kTarget == 40 && wantSum == kTarget;
    for (int i = 0; i < kCoins; i++) {
        coin_[i].cents = kCents[i];
        coin_[i].kind = kKind[i];
        coin_[i].want = kWant[i] != 0;
        coin_[i].x = homeX(i);
    }
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    rung_ = false;
    left_ = false;
    if (bot_) begin();
    draw();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    t_++;

    if (mode_ == Mode::Title) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C))) begin();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) sys.quit();
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) pause_ = !pause_;
        if (!pause_) {
            if (bot_) botAct();
            else human(pad);
        }
        if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            pause_ = false;
            sys.apu.silence();
        }
    } else if (mode_ == Mode::Ring) {
        if (++hold_ == 8 || hold_ == 20) {
            swing_ = 14;
            sys_->apu.tone(0, hold_ == 8 ? 659.f : 880.f, 0.08f);
        }
        if (hold_ > 36) {
            mode_ = Mode::Leave;
            hold_ = 0;
        }
    } else if (mode_ == Mode::Leave) {
        clerkX_ += 3.2f;
        if (swing_ > 0 && (hold_ % 2) == 0) swing_--;
        hold_++;
        if (clerkX_ > 340.f) {
            left_ = true;
            won_ = rung_ && dead_ < kMaxDead && matched();
            over_ = true;
        }
    } else if (mode_ == Mode::Lose) {
        if (!bot_ && pad.pressed(gs::BTN_START)) begin();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
            sys.apu.silence();
        }
    }

    if (swing_ > 0 && mode_ == Mode::Play) swing_--;
    if (mode_ == Mode::Ring && swing_ > 0 && (hold_ % 2) == 0) swing_--;
    if (foul_ > 0 && mode_ == Mode::Play) foul_--;
    draw();
}

}  // namespace drawerbell

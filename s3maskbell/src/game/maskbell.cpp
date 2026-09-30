#include "game/maskbell.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace maskbell {
namespace {

struct Groove {
    int lo, hi;
    const char* name;
    float hx, hy;
};

// One shared punch. Each cut owns a slice of the 48-step rail.
const Groove kGroove[kCuts] = {
    {4, 10, "BROW", 160.f, 92.f},
    {16, 22, "LEFT", 146.f, 112.f},
    {28, 34, "RIGHT", 174.f, 112.f},
    {40, 46, "MOUTH", 160.f, 142.f},
};

}  // namespace

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

bool Game::hot(int i) const {
    if (i < 0 || i >= kCuts || cutOn_[i]) return false;
    int p = phase_;
    return p >= kGroove[i].lo && p <= kGroove[i].hi;
}

void Game::resetBlank(int which) {
    for (int i = 0; i < kCuts; i++) cutOn_[i] = false;
    locked_ = 0;
    phase_ = (which * 7) % kPhase;
    foul_ = 0;
    skip_ = 1;
}

void Game::begin() {
    rung_ = false;
    won_ = false;
    over_ = false;
    pause_ = false;
    dead_ = 0;
    tryNo_ = 1;
    swing_ = 0;
    hold_ = 0;
    resetBlank(0);
    mode_ = Mode::Play;
    blip(392.f);
}

void Game::dieTry() {
    if (mode_ != Mode::Play || rung_) return;
    dead_++;
    swing_ = 4;
    foul_ = 36;
    sys_->apu.tone(0, 146.f, 0.08f);
    sys_->apu.noiseBurst(0.18f, 420.f, 0.08f);
    if (!bot_) sys_->rumble(0.4f, 0.12f, 70);
    if (dead_ >= kMaxDead) {
        mode_ = Mode::Lose;
        won_ = false;
        over_ = true;
        hold_ = 0;
        if (!bot_) sys_->setLight(140, 24, 24);
        return;
    }
    tryNo_ = dead_ + 1;
    resetBlank(dead_);
    if (!bot_) sys_->setLight(120, 48, 28);
}

void Game::strike() {
    if (mode_ != Mode::Play || pause_) return;
    int hit = -1;
    for (int i = 0; i < kCuts; i++) {
        if (hot(i)) {
            hit = i;
            break;
        }
    }
    if (hit < 0) {
        dieTry();
        return;
    }
    cutOn_[hit] = true;
    locked_++;
    blip(440.f + float(locked_) * 70.f);
    if (!bot_) sys_->setLight(40, 90, 40);
}

void Game::ring() {
    if (mode_ != Mode::Play || pause_ || rung_) return;
    if (!allLocked()) {
        dieTry();
        return;
    }
    mode_ = Mode::Ring;
    rung_ = true;
    swing_ = 12;
    hold_ = 0;
    sys_->apu.tone(0, 659.f, 0.1f);
    if (!bot_) sys_->setLight(180, 140, 40);
}

void Game::botAct() {
    if (allLocked()) {
        ring();
        return;
    }
    for (int i = 0; i < kCuts; i++) {
        if (hot(i)) {
            strike();
            return;
        }
    }
}

void Game::human(const gs::Pad& pad) {
    if (pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_B)) strike();
    if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_Z)) ring();
}

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    uint16_t top = gs::rgb4(2, 2, 4);
    uint16_t mid = gs::rgb4(7, 4, 3);
    uint16_t bot = gs::rgb4(3, 2, 1);
    if (mode_ == Mode::Ring) mid = gs::rgb4(12, 8, 3);
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

    int f = int(sys_->frame);
    spr(art_.rail, 160.f, 176.f, PAL_WOOD);
    float tx = 60.f + (phase_ / float(kPhase - 1)) * 200.f;
    spr(art_.tick, tx, 168.f, mode_ == Mode::Play ? PAL_TICK : PAL_DIM);
    spr(art_.mask, 160.f, 118.f, mode_ == Mode::Lose ? PAL_DIM : PAL_FACE);
    for (int i = 0; i < kCuts; i++) {
        if (!cutOn_[i] && mode_ != Mode::Ring) continue;
        spr(art_.hole, kGroove[i].hx, kGroove[i].hy, mode_ == Mode::Ring ? PAL_LIT : PAL_HOLE);
    }
    float ox = std::sin(f * 0.7f) * float(swing_) * 0.5f;
    int bp = PAL_BELL;
    if (mode_ == Mode::Ring) bp = PAL_LIT;
    else if (mode_ == Mode::Play && allLocked()) bp = PAL_GOLD;
    spr(art_.bell, 268.f + ox, 48.f, bp);

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(1, "S3 MASKBELL", PAL_GOLD);
        hudC(22, "A SHORT MASK", PAL_GOLD);
        hudC(23, "BELL BEFORE THE THIRD TRY DIES", PAL_HUD);
        hudC(25, "C CUTS THE GROOVE", PAL_DIM);
        hudC(26, "A RINGS WHEN THE MASK IS CUT", PAL_DIM);
        if ((f & 16) == 0) hudC(27, "PRESS START", PAL_GOLD);
        const char* ver = S3_VERSION_STRING;
        hud(40 - int(std::strlen(ver)), 0, ver, PAL_DIM);
    } else if (mode_ == Mode::Lose) {
        hudC(1, "THIRD TRY DIED", PAL_BAD);
        hudC(25, "THE BELL STAYED QUIET", PAL_HUD);
        if ((f & 16) == 0) hudC(27, "START", PAL_GOLD);
    } else if (mode_ == Mode::Ring) {
        hudC(1, "THE BELL RINGS", PAL_GOLD);
        std::snprintf(buf, sizeof buf, "TRY %d OF %d", tryNo_, kMaxDead);
        hudC(25, buf, PAL_GOLD);
        hudC(26, "MASK CUT", PAL_HUD);
    } else {
        int open = -1;
        for (int i = 0; i < kCuts; i++)
            if (hot(i)) open = i;
        std::snprintf(buf, sizeof buf, "TRY %d", tryNo_);
        hud(1, 24, buf, dead_ ? PAL_BAD : PAL_HUD);
        std::snprintf(buf, sizeof buf, "CUTS %d", locked_);
        hud(30, 24, buf, allLocked() ? PAL_GOLD : PAL_HUD);
        if (foul_ > 0) hudC(25, "THE TRY DIED", PAL_BAD);
        else if (pause_) hudC(25, "PAUSED", PAL_GOLD);
        else if (allLocked()) hudC(25, "RING THE BELL", PAL_GOLD);
        else if (open >= 0) {
            std::snprintf(buf, sizeof buf, "CUT %s", kGroove[open].name);
            hudC(25, buf, PAL_GOLD);
        } else {
            const char* nxt = "BROW";
            for (int i = 0; i < kCuts; i++)
                if (!cutOn_[i]) {
                    nxt = kGroove[i].name;
                    break;
                }
            std::snprintf(buf, sizeof buf, "WAIT %s", nxt);
            hudC(25, buf, PAL_DIM);
        }
        hudC(27, "C STRIKE   A BELL", PAL_DIM);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.hudEnabled = true;
    sys.apu.setMaster(0.7f);
    rules_ = kMaxDead == 3 && kCuts == 4 && kPhase == 48;
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    rung_ = false;
    if (bot_) begin();
    draw();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;

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
        if (mode_ == Mode::Play && !pause_) {
            if (skip_ > 0) skip_--;
            else phase_ = (phase_ + 1) % kPhase;
        }
    } else if (mode_ == Mode::Ring) {
        if (++hold_ == 8 || hold_ == 22) {
            swing_ = 14;
            sys_->apu.tone(0, hold_ == 8 ? 659.f : 784.f, 0.08f);
        }
        if (hold_ > 48) {
            over_ = true;
            won_ = true;
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

    if (swing_ > 0 && mode_ != Mode::Ring) swing_--;
    if (mode_ == Mode::Ring && swing_ > 0 && (hold_ % 2) == 0) swing_--;
    if (foul_ > 0 && mode_ == Mode::Play) foul_--;
    draw();
}

}  // namespace maskbell

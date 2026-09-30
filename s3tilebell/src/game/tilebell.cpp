#include "game/tilebell.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace tilebell {

void Game::blip(float freq) { sys_->apu.keyOn(0, freq, 0.18f); }

void Game::resetTry() {
    for (int i = 0; i < kSockets; i++) placed_[i] = -1;
    cursor_ = 0;
    hand_ = 0;
    age_ = 0;
    anim_ = 0;
    botStep_ = 0;
}

void Game::begin() {
    dead_ = attempt_ = 0;
    over_ = won_ = rung_ = false;
    why_ = "";
    resetTry();
    mode_ = Mode::Try;
    sys_->apu.silence();
}

bool Game::rowFull() const {
    for (int i = 0; i < kSockets; i++)
        if (placed_[i] != kPattern[i]) return false;
    return true;
}

void Game::ring() {
    rung_ = true;
    why_ = "the bell rings before the third try dies";
    sys_->apu.keyOn(0, 523.f, 0.4f);
    sys_->apu.keyOn(1, 784.f, 0.28f);
    sys_->apu.noiseBurst(0.12f, 220.f, 0.06f);
    anim_ = 0;
    mode_ = Mode::Rise;
}

void Game::killTry() {
    dead_++;
    anim_ = 0;
    mode_ = Mode::Gap;
    sys_->apu.noiseBurst(0.2f, 80.f, 0.14f);
    why_ = dead_ >= kTries ? "the third try died" : "that try died";
}

void Game::lay() {
    if (mode_ != Mode::Try) return;
    if (hand_ != kPattern[cursor_]) {
        killTry();
        return;
    }
    placed_[cursor_] = hand_;
    blip(440.f + cursor_ * 40.f);
    if (rowFull()) {
        ring();
        return;
    }
    if (cursor_ < kSockets - 1) cursor_++;
}

void Game::spr(const gs::Image& img, float cx, float cy, float w, float h, int pal) {
    if (img.w == 0 || w < 1.f || h < 1.f) return;
    gs::Sprite s;
    s.img = img;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - w * 0.5f));
    s.y = int16_t(std::lround(cy - h * 0.5f));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
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
    uint16_t top = gs::rgb4(2, 3, 5);
    uint16_t mid = gs::rgb4(6, 5, 4);
    uint16_t bot = gs::rgb4(3, 3, 3);
    if (mode_ == Mode::Over && won_) mid = gs::rgb4(8, 7, 3);
    if (mode_ == Mode::Over && !won_) mid = gs::rgb4(5, 2, 2);
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

    float swing = 0.f;
    if (rung_ && (mode_ == Mode::Rise || (mode_ == Mode::Over && won_))) swing = std::sin(anim_ * 0.42f) * 10.f;
    spr(art_.yoke, 160.f, 28.f, 70.f, 10.f, PAL_WOOD);
    spr(art_.bell, 160.f + swing, 46.f, 36.f, 28.f, PAL_BELL);

    const float x0 = 52.f;
    const float step = 72.f;
    const float sy = 128.f;
    for (int i = 0; i < kSockets; i++) {
        float x = x0 + i * step;
        spr(art_.socket, x, sy, 40.f, 40.f, PAL_WOOD);
        spr(art_.tile[kPattern[i]], x, sy - 28.f, 16.f, 16.f, PAL_GHOST);
        if (placed_[i] >= 0) spr(art_.tile[placed_[i]], x, sy, 30.f, 30.f, PAL_TILE);
        if (mode_ == Mode::Try && i == cursor_) spr(art_.cursor, x, sy, 44.f, 44.f, PAL_GOLD);
    }

    if (mode_ == Mode::Try || mode_ == Mode::Title) {
        spr(art_.tile[hand_], 160.f, 188.f, 28.f, 28.f, PAL_TILE);
    }

    for (int i = 0; i < kTries; i++) {
        int pal = PAL_LAMP;
        gs::Sprite s;
        s.img = art_.lamp;
        s.w = s.h = 10;
        s.x = int16_t(18 + i * 14);
        s.y = 14;
        s.pal = PAL_LAMP;
        if (i < dead_) {
            // cracked lamp: same image, bad read through a recolor is not available; shift palette index via fog-less
            // second draw is the lamp; tint by choosing a palette that maps index 1 to red.
            s.pal = PAL_BAD;
        } else if (i == attempt_ && mode_ != Mode::Over) {
            s.pal = PAL_GOLD;
        } else {
            s.pal = PAL_DIM;
        }
        (void)pal;
        v.sprite(s);
    }

    char buf[80];
    int f = int(sys_->frame);
    if (mode_ == Mode::Title) {
        hudC(3, "S3 TILE BELL", PAL_GOLD);
        hudC(18, "LAY THE MARKED TILES", PAL_HUD);
        hudC(20, "THE BELL RINGS", PAL_GOLD);
        hudC(21, "BEFORE THE THIRD TRY DIES", PAL_HUD);
        if ((f & 16) == 0) hudC(26, "PRESS START", PAL_GOLD);
        const char* ver = S3_VERSION_STRING;
        hud(40 - int(std::strlen(ver)), 0, ver, PAL_DIM);
    } else if (mode_ == Mode::Over && won_) {
        hudC(3, "THE BELL RINGS", PAL_GOLD);
        hudC(20, "BEFORE THE THIRD TRY DIED", PAL_HUD);
        std::snprintf(buf, sizeof buf, "DEAD %d  TRY %d", dead_, attempt_ + 1);
        hudC(22, buf, PAL_GOLD);
    } else if (mode_ == Mode::Over) {
        hudC(3, "THE THIRD TRY DIED", PAL_BAD);
        hudC(21, why_, PAL_HUD);
        if ((f & 16) == 0) hudC(26, "START", PAL_GOLD);
    } else {
        std::snprintf(buf, sizeof buf, "TRY %d/%d", attempt_ + 1, kTries);
        hud(1, 1, buf, PAL_GOLD);
        std::snprintf(buf, sizeof buf, "DEAD %d", dead_);
        hud(30, 1, buf, dead_ ? PAL_BAD : PAL_DIM);
        if (mode_ == Mode::Try) {
            int left = kDieAt - age_;
            if (left < 0) left = 0;
            std::snprintf(buf, sizeof buf, "HAND %d   CLOCK %d", hand_ + 1, left / 6);
            hudC(24, buf, PAL_HUD);
            hudC(26, "ARROWS TURN   C LAYS", PAL_DIM);
        } else if (mode_ == Mode::Rise) {
            hudC(24, "THE BELL RINGS", PAL_GOLD);
        } else if (dead_ < kTries) {
            hudC(24, "THAT TRY DIED", PAL_BAD);
        }
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.85f);
    mode_ = Mode::Title;
    hand_ = 0;
    for (int i = 0; i < kSockets; i++) placed_[i] = -1;
    if (bot_) begin();
    draw();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C)) begin();
    } else if (mode_ == Mode::Try) {
        if (bot_) {
            if (dead_ < kTries - 1) {
                if (age_ == 8) lay();
            } else {
                // Match the painted mark, one stamp every few frames.
                const int want = kPattern[cursor_];
                if ((age_ % 6) == 2) {
                    if (hand_ != want) hand_ = (hand_ + 1) % kFaces;
                    else lay();
                }
            }
        } else {
            if (pad.pressed(gs::BTN_LEFT)) {
                cursor_ = (cursor_ + kSockets - 1) % kSockets;
                blip(300.f);
            }
            if (pad.pressed(gs::BTN_RIGHT)) {
                cursor_ = (cursor_ + 1) % kSockets;
                blip(320.f);
            }
            if (pad.pressed(gs::BTN_UP) || pad.pressed(gs::BTN_DOWN)) {
                int dir = pad.pressed(gs::BTN_DOWN) ? kFaces - 1 : 1;
                hand_ = (hand_ + dir) % kFaces;
                blip(360.f);
            }
            if (pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A)) lay();
        }
        if (mode_ == Mode::Try && ++age_ >= kDieAt) killTry();
    } else if (mode_ == Mode::Rise) {
        anim_++;
        if (anim_ >= 40) {
            won_ = rung_ && dead_ < kTries;
            over_ = true;
            mode_ = Mode::Over;
            if (won_) {
                why_ = "the bell rings before the third try dies";
                sys.apu.keyOn(0, 659.f, 0.32f);
                sys.apu.keyOn(1, 880.f, 0.22f);
            }
        }
    } else if (mode_ == Mode::Gap) {
        anim_++;
        if (anim_ >= 24) {
            if (dead_ >= kTries) {
                won_ = false;
                over_ = true;
                mode_ = Mode::Over;
                why_ = "the third try died";
            } else {
                attempt_++;
                resetTry();
                mode_ = Mode::Try;
            }
        }
    } else if (mode_ == Mode::Over) {
        anim_++;
        if (!bot_ && !won_ && pad.pressed(gs::BTN_START)) begin();
    }

    draw();
}

}  // namespace tilebell

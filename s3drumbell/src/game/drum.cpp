#include "game/drum.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace drumbell {
namespace {

constexpr float kLeft = 52.f;
constexpr float kRight = 276.f;
constexpr float kSpeed = 2.05f;
constexpr float kWindow = 10.f;
constexpr float kMarkX[kMarks] = {86.f, 140.f, 194.f, 248.f};

float clampf(float v, float a, float b) {
    if (v < a) return a;
    if (v > b) return b;
    return v;
}

}  // namespace

int Game::hits() const {
    int n = 0;
    for (int i = 0; i < kMarks; i++)
        if (did_[i]) n++;
    return n;
}

float Game::beaterX() const { return phase_; }

void Game::blip(float freq, float vol, int frames) {
    if (!sys_ || fanT_ > 0) return;
    sys_->apu.tone(0, freq, vol);
    blip_ = frames;
}

void Game::begin() {
    dead_ = 0;
    attempt_ = 0;
    over_ = false;
    won_ = false;
    rung_ = false;
    why_ = "";
    fanT_ = 0;
    resetTry();
    std::snprintf(say_, sizeof say_, "COUNT IN");
    blip(220.f, 0.06f, 4);
}

void Game::resetTry() {
    phase_ = kLeft;
    swing_ = 0;
    shake_ = 0;
    anim_ = 0;
    for (int i = 0; i < kMarks; i++) did_[i] = false;
    mode_ = Mode::Play;
}

void Game::strike() {
    int n = hits();
    if (n >= kMarks) return;
    did_[n] = true;
    swing_ = 8;
    shake_ = 3;
    static const char* name[kMarks] = {"ONE", "TWO", "THREE", "FOUR"};
    std::snprintf(say_, sizeof say_, "%s", name[n]);
    if (sys_) {
        sys_->apu.noiseBurst(0.4f, 1600.f, 0.07f);
        sys_->apu.tone(1, 120.f + float(n) * 18.f, 0.07f);
        sys_->rumble(0.12f, 0.28f, 40);
    }
    blip(196.f + float(n) * 36.f, 0.05f, 3);
    if (n + 1 == kMarks) ring();
}

void Game::killTry(const char* why) {
    dead_++;
    anim_ = 0;
    swing_ = 0;
    shake_ = 6;
    mode_ = Mode::Gap;
    why_ = why;
    std::snprintf(say_, sizeof say_, "TRY DIED");
    for (int i = 0; i < kMarks; i++) did_[i] = false;
    if (sys_) {
        sys_->apu.noiseBurst(0.2f, 380.f, 0.18f);
        sys_->rumble(0.35f, 0.08f, 80);
        sys_->setLight(40, 16, 70);
    }
}

void Game::ring() {
    rung_ = true;
    why_ = "the bell rings before the third try dies";
    mode_ = Mode::Rise;
    anim_ = 0;
    fanT_ = 1;
    std::snprintf(say_, sizeof say_, "BELL");
    if (sys_) {
        sys_->apu.noiseBurst(0.12f, 240.f, 0.05f);
        sys_->rumble(0.25f, 0.55f, 200);
        sys_->setLight(220, 180, 40);
    }
}

bool Game::botTap() const {
    if (mode_ != Mode::Play) return false;
    if (attempt_ < kTries - 1) return phase_ <= kLeft + 0.01f;
    int n = hits();
    if (n >= kMarks) return false;
    float d = beaterX() - kMarkX[n];
    return d >= -2.4f && d <= 2.4f;
}

void Game::audio() {
    if (!sys_) return;
    gs::APU& a = sys_->apu;
    if (fanT_ > 0) {
        fanT_++;
        if (fanT_ == 2 || fanT_ == 14 || fanT_ == 26 || fanT_ == 40) {
            static const float n[4] = {392.f, 494.f, 587.f, 784.f};
            int i = fanT_ < 14 ? 0 : fanT_ < 26 ? 1 : fanT_ < 40 ? 2 : 3;
            a.tone(0, n[i], 0.14f);
            a.tone(2, n[i] * 1.5f, 0.05f);
        }
        if (fanT_ > 78) {
            a.tone(0, 0, 0);
            a.tone(2, 0, 0);
            fanT_ = 0;
        }
        return;
    }
    if (blip_ > 0) {
        blip_--;
        if (blip_ == 0) a.tone(0, 0, 0);
    }
    if (swing_ <= 0) a.tone(1, 0, 0);
}

void Game::image(const gs::Mipped& m, float left, float top, float h, int pal, bool flip) {
    if (!sys_ || h < 1.5f || h > 420.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    if (left > 340.f || top > 250.f || left + w < -30.f || top + h < -30.f) return;
    gs::Sprite s;
    auto q = [](float v) { return int16_t(std::lround(clampf(v, -400.f, 800.f))); };
    s.x = q(left);
    s.y = q(top);
    s.w = int16_t(std::max(1, int(std::lround(w))));
    s.h = int16_t(std::max(1, int(std::lround(h))));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::spr(const gs::Mipped& m, float cx, float bottom, float h, int pal, bool flip) {
    if (m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    image(m, cx - w * 0.5f, bottom - h, h, pal, flip);
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    uint16_t top = gs::rgb4(2, 0, 4);
    uint16_t mid = gs::rgb4(4, 1, 6);
    uint16_t floor = gs::rgb4(3, 2, 2);
    if (mode_ == Mode::Over && won_) mid = gs::rgb4(8, 6, 2);
    if (mode_ == Mode::Over && !won_) mid = gs::rgb4(5, 1, 2);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.road[y].on = false;
        v.lineFog[y] = y > 180 ? uint8_t((y - 180) / 8) : 0;
        float u = float(y) / float(gs::SCREEN_H - 1);
        auto mix = [](uint16_t a, uint16_t b, float t) {
            int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
            int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
            auto L = [&](int p, int q) { return int(p + (q - p) * t + 0.5f); };
            return gs::rgb4(L(ar, br), L(ag, bg), L(ab, bb));
        };
        v.lineBackdrop[y] = u < 0.62f ? mix(top, mid, u / 0.62f) : mix(mid, floor, (u - 0.62f) / 0.38f);
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

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    float swing = 0.f;
    if (rung_ && (mode_ == Mode::Rise || (mode_ == Mode::Over && won_))) swing = std::sin(anim_ * 0.42f) * 10.f;

    image(art_.curtain, 4.f, 16.f, 128.f, PAL_WOOD, false);
    image(art_.curtain, 292.f, 16.f, 128.f, PAL_WOOD, true);

    float jig = shake_ ? ((anim_ & 1) ? 1.4f : -1.4f) : 0.f;
    float bounce = swing_ > 5 ? 2.5f : 0.f;
    spr(art_.stool, 164.f, 178.f, 16.f, PAL_FX, false);
    spr(art_.player, 164.f + jig, 170.f, 64.f, PAL_PLAYER, false);
    spr(art_.drum, 164.f + jig, 156.f + bounce, 40.f, PAL_DRUM, false);
    const gs::Mipped& stick = swing_ > 0 ? art_.stickDown : art_.stickUp;
    image(stick, 178.f, swing_ > 0 ? 102.f : 90.f, 26.f, PAL_FX, false);

    spr(art_.bell, 164.f + swing, 58.f, 36.f, PAL_BELL, false);
    spr(art_.clapper, 164.f + swing * 1.35f, 52.f, 10.f, PAL_FX, false);

    float barY = 196.f;
    for (int i = 0; i < kMarks; i++) {
        const gs::Mipped& head = did_[i] ? art_.headOn : art_.head;
        int pal = did_[i] ? PAL_GREEN : PAL_GOLD;
        spr(head, kMarkX[i], barY, 14.f, pal, false);
    }
    if (mode_ == Mode::Play || mode_ == Mode::Title) {
        float bx = (mode_ == Mode::Title) ? (kLeft + std::fmod(float(sys_->frame) * 1.6f, kRight - kLeft)) : beaterX();
        spr(art_.beater, bx, barY - 2.f, 16.f, PAL_RED, false);
    }

    for (int i = 0; i < kTries; i++) {
        int pal = PAL_DIM;
        if (i < dead_) pal = PAL_RED;
        else if (mode_ != Mode::Title && mode_ != Mode::Over && i == attempt_) pal = PAL_GOLD;
        if (rung_ && i == attempt_) pal = PAL_GREEN;
        spr(art_.lamp, 24.f + float(i) * 16.f, 22.f, 12.f, pal, false);
    }

    char buf[64];
    int f = int(sys_->frame);
    if (mode_ == Mode::Title) {
        hudC(3, "S3 DRUMBELL", PAL_GOLD);
        hudC(18, "A SHORT DRUM", PAL_HUD);
        hudC(20, "FOUR STROKES IN TIME", PAL_HUD);
        hudC(22, "THE BELL RINGS", PAL_GOLD);
        hudC(23, "BEFORE THE THIRD TRY DIES", PAL_HUD);
        if ((f & 16) == 0) hudC(26, "PRESS START", PAL_GOLD);
        const char* ver = S3_VERSION_STRING;
        hud(40 - int(std::strlen(ver)), 0, ver, PAL_DIM);
    } else if (mode_ == Mode::Over && won_) {
        hudC(3, "THE BELL RINGS", PAL_GOLD);
        hudC(20, "BEFORE THE THIRD TRY DIED", PAL_HUD);
        std::snprintf(buf, sizeof buf, "DEAD %d  TRY %d", dead_, attempt_ + 1);
        hudC(22, buf, PAL_GOLD);
    } else if (mode_ == Mode::Over) {
        hudC(3, "THE THIRD TRY DIED", PAL_RED);
        hudC(22, "START", PAL_GOLD);
    } else {
        std::snprintf(buf, sizeof buf, "TRY %d/%d", attempt_ + 1, kTries);
        hud(1, 1, buf, PAL_GOLD);
        std::snprintf(buf, sizeof buf, "DEAD %d", dead_);
        hud(32, 1, buf, dead_ ? PAL_RED : PAL_DIM);
        if (say_[0]) hudC(24, say_, mode_ == Mode::Gap ? PAL_RED : PAL_GOLD);
        if (mode_ == Mode::Play) hudC(26, "C STRIKES THE DRUM", PAL_DIM);
        else if (mode_ == Mode::Rise) hudC(26, "THE BELL RINGS", PAL_GOLD);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(1, 0, 2));
    sys.apu.setMaster(0.85f);
    mode_ = Mode::Title;
    say_[0] = 0;
    if (bot_) begin();
    draw();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C)) begin();
    } else if (mode_ == Mode::Play) {
        bool tap = false;
        if (bot_) tap = botTap();
        else tap = pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B);
        int n = hits();
        if (tap) {
            if (n < kMarks && std::fabs(beaterX() - kMarkX[n]) <= kWindow) strike();
            else killTry("that try died");
        } else if (n < kMarks) {
            phase_ += kSpeed;
            if (beaterX() > kMarkX[n] + kWindow) killTry("that try died");
            else if (phase_ > kRight) killTry("that try died");
        }
        if (swing_ > 0) swing_--;
        if (shake_ > 0) shake_--;
    } else if (mode_ == Mode::Gap) {
        anim_++;
        if (shake_ > 0) shake_--;
        if (anim_ >= 24) {
            if (dead_ >= kTries) {
                won_ = false;
                rung_ = false;
                over_ = true;
                mode_ = Mode::Over;
                why_ = "the third try died";
                std::snprintf(say_, sizeof say_, "SILENT");
            } else {
                attempt_++;
                resetTry();
                std::snprintf(say_, sizeof say_, "AGAIN");
            }
        }
    } else if (mode_ == Mode::Rise) {
        anim_++;
        if (swing_ > 0) swing_--;
        if (anim_ >= 48) {
            won_ = rung_ && dead_ < kTries;
            over_ = true;
            mode_ = Mode::Over;
            why_ = won_ ? "the bell rings before the third try dies" : "the third try died";
        }
    } else if (mode_ == Mode::Over) {
        anim_++;
        if (!bot_ && !won_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C))) begin();
    }

    audio();
    draw();
}

}  // namespace drumbell

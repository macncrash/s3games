#include "game/lantern.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace lanternseven {
namespace {
constexpr float kAmp = 0.9f;
constexpr float kArm = 34.f;
constexpr float kPivotY = 28.f;
constexpr float kCatch = 0.30f;
}  // namespace

float Game::theta(int i) const { return std::sin(phase_[i]) * kAmp; }

float Game::hangX(int i) const {
    float x0 = 28.f + i * 44.f;
    return x0 + std::sin(theta(i)) * kArm;
}

float Game::hangY(int i) const { return kPivotY + std::cos(theta(i)) * kArm; }

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    mode_ = Mode::Title;
    you_ = 0;
    them_ = 0;
    won_ = false;
    over_ = false;
    t_ = 0;
}

void Game::newMatch() {
    you_ = 0;
    them_ = 0;
    won_ = false;
    over_ = false;
    night_ = 0;
    raise_ = 0;
    youX_ = 160;
    for (int i = 0; i < 7; i++) {
        lit_[i] = false;
        phase_[i] = i * 0.85f;
    }
    mode_ = Mode::Play;
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.06f);
    beep_ = 5;
}

void Game::light(int i) {
    if (i < 0 || i > 6 || lit_[i] || mode_ != Mode::Play) return;
    lit_[i] = true;
    you_++;
    raise_ = 12;
    blip(520.f + you_ * 48.f);
    sys_->rumble(0.12f, 0.28f, 70);
    if (you_ >= 7) {
        mode_ = Mode::Win;
        won_ = true;
        over_ = true;
    }
}

int Game::nearest() const {
    int pick = -1;
    float best = 1e9f;
    for (int i = 0; i < 7; i++) {
        if (lit_[i]) continue;
        float th = std::fabs(theta(i));
        float dx = std::fabs(hangX(i) - youX_);
        float score = dx + th * 90.f;
        if (score < best) {
            best = score;
            pick = i;
        }
    }
    return pick;
}

void Game::nightTick() {
    if (mode_ != Mode::Play || over_) return;
    if (++night_ < 72) return;
    night_ = 0;
    them_++;
    sys_->apu.tone(2, 140, 0.05f);
    if (them_ >= 7 && you_ < 7) {
        mode_ = Mode::Lose;
        won_ = false;
        over_ = true;
    }
}

void Game::botAct() {
    if (mode_ == Mode::Title) {
        if (t_ > 18) newMatch();
        return;
    }
    if (mode_ != Mode::Play || over_) return;
    int i = nearest();
    if (i < 0) return;
    float tx = hangX(i);
    if (youX_ < tx - 5) youX_ += 6;
    else if (youX_ > tx + 5) youX_ -= 6;
    else if (std::fabs(theta(i)) < kCatch) light(i);
}

void Game::human() {
    if (mode_ == Mode::Title || mode_ == Mode::Win || mode_ == Mode::Lose) {
        if (sys_->pad.pressed(gs::BTN_START) || sys_->pad.pressed(gs::BTN_A)) newMatch();
        return;
    }
    if (mode_ != Mode::Play) return;
    auto hold = [&](gs::Button b, int& h, float dir) {
        if (sys_->pad.pressed(b)) {
            h = 0;
            youX_ += dir * 4;
        } else if (sys_->pad.down(b)) {
            youX_ += dir * 3.2f;
            h++;
        } else {
            h = 0;
        }
    };
    hold(gs::BTN_LEFT, holdL_, -1);
    hold(gs::BTN_RIGHT, holdR_, 1);
    if (sys_->pad.axisX > 0.2f || sys_->pad.axisX < -0.2f) youX_ += sys_->pad.axisX * 3.4f;
    youX_ = std::clamp(youX_, 16.f, 304.f);
    if (sys_->pad.pressed(gs::BTN_A) || sys_->pad.pressed(gs::BTN_C)) {
        raise_ = 10;
        int hit = -1;
        float best = 22.f;
        for (int i = 0; i < 7; i++) {
            if (lit_[i] || std::fabs(theta(i)) >= kCatch) continue;
            float d = std::fabs(hangX(i) - youX_);
            if (d < best) {
                best = d;
                hit = i;
            }
        }
        if (hit >= 0) light(hit);
        else {
            sys_->apu.noiseBurst(0.2f, 180, 8);
            sys_->apu.tone(1, 90, 0.04f);
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_++;
    if (beep_ > 0 && --beep_ == 0) sys.apu.tone(0, 0, 0);
    if (raise_ > 0) raise_--;
    if (mode_ == Mode::Play && !over_) {
        for (int i = 0; i < 7; i++) phase_[i] += 0.085f + i * 0.004f;
        nightTick();
    }
    if (bot_) botAct();
    else human();
    draw();
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s{};
    s.w = int16_t(std::clamp(std::lround(w), 1L, 2000L));
    s.h = int16_t(std::clamp(std::lround(h), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::box(float x, float y, float w, float h, int pal) {
    if (w < 1 || h < 1) return;
    gs::Sprite s{};
    s.x = int16_t(std::lround(x));
    s.y = int16_t(std::lround(y));
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.img = art_.solid.pick(std::max(h, 4.f));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < 168) {
            float t = y / 168.f;
            int b = 6 + int((1.f - t) * 5);
            v.lineBackdrop[y] = gs::rgb4(1, 1 + int(t * 2), std::min(b, 15));
        } else {
            v.lineBackdrop[y] = gs::rgb4(2, 2, 3);
        }
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }

    spr(art_.moon, 262, 36, 28, PAL_MOON);

    for (int i = 0; i < 7; i++) {
        float x0 = 28.f + i * 44.f;
        float hx = hangX(i);
        float hy = hangY(i);
        spr(art_.post, x0 - 10, 78, 70, PAL_POST);
        for (int s = 1; s <= 4; s++) {
            float u = s / 4.f;
            box(x0 + (hx - x0) * u - 1, kPivotY + (hy - 10 - kPivotY) * u, 2, 2, PAL_SOLID);
        }
        int pal = lit_[i] ? PAL_LAMP : PAL_POST;
        spr(art_.lamp, hx, hy, lit_[i] ? 30 : 26, pal);
        if (lit_[i] || (std::fabs(theta(i)) < kCatch && ((t_ / 6) & 1)))
            spr(art_.flame, hx, hy - 8, lit_[i] ? 12 : 7, PAL_FLAME);
    }

    float hand = 168.f - (raise_ > 0 ? 16.f : 0.f);
    spr(art_.you, youX_, 186, 52, PAL_YOU, youX_ > 200);
    spr(art_.lamp, youX_ + 10, hand, 18, PAL_LAMP);
    spr(art_.flame, youX_ + 10, hand - 8, 8 + (t_ & 1), PAL_FLAME);

    for (int i = 0; i < 7; i++) {
        box(8.f + i * 12, 6, 8, 6, PAL_SOLID);
        if (i >= you_) box(9.f + i * 12, 7, 6, 4, PAL_SOLID);
    }

    if (mode_ == Mode::Title) {
        hudC(10, "S3 LANTERN SEVEN", PAL_GOLD);
        hudC(12, "A SHORT LANTERN", PAL_HUD);
        hudC(14, "FIRST TO SEVEN", PAL_GOLD);
        hudC(16, "WALK UNDER THE DIP", PAL_HUD);
        hudC(18, "A RAISES THE LIGHT", PAL_HUD);
        if ((t_ / 30) % 2 == 0) hudC(21, "PRESS START", PAL_GOLD);
        hud(40 - int(std::strlen(S3_VERSION_STRING)), 26, S3_VERSION_STRING, PAL_DIM);
    } else if (mode_ == Mode::Win) {
        hudC(24, "FIRST TO SEVEN", PAL_GREEN);
    } else if (mode_ == Mode::Lose) {
        hudC(24, "THE NIGHT GOT THERE", PAL_ALERT);
    } else {
        char line[40];
        std::snprintf(line, sizeof(line), "YOU %d   NIGHT %d", you_, them_);
        hudC(24, line, PAL_HUD);
        hudC(26, "UNDER SEVEN IS STILL SHORT", PAL_DIM);
    }
}

}  // namespace lanternseven

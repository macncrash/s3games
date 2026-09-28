#include "game/seven.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace juggleseven {
namespace {

constexpr int kFlight = 28;
constexpr int kEvery = 14;
constexpr int kThemEvery = 18;
constexpr float kLeftX = 118.f;
constexpr float kRightX = 210.f;
constexpr float kHandY = 158.f;
constexpr float kPi = 3.14159265f;

float handX(int h) { return h == 0 ? kLeftX : kRightX; }

static_assert(kSeven == 7, "the line is seven");
static_assert(kSeven - 1 == 6, "six is still short");

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    mode_ = Mode::Title;
    over_ = won_ = rules_ = false;
    you_ = them_ = catches_ = 0;
    titleWait_ = 0;
    beep_ = 0;
    rivalPhase_ = 0;
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    for (int y = 0; y < gs::SCREEN_H; y++) sys.vdp.road[y].on = false;
}

void Game::begin() {
    rules_ = true;
    won_ = over_ = false;
    you_ = them_ = catches_ = 0;
    clock_ = 0;
    pattern();
    blip(392.f);
}

void Game::pattern() {
    mode_ = Mode::Play;
    tick_ = 0;
    wait_ = 0;
    latch_[0] = latch_[1] = 0;
    hn_[0] = 2;
    hold_[0][0] = 0;
    hold_[0][1] = 2;
    hn_[1] = 1;
    hold_[1][0] = 1;
    hold_[1][1] = 0;
    hold_[1][2] = 0;
    for (auto& a : air_) a = {};
}

void Game::miss() {
    mode_ = Mode::Drop;
    wait_ = 36;
    for (auto& a : air_) a.on = false;
    blip(90.f);
    if (sys_) sys_->setLight(160, 28, 28);
}

void Game::finish(bool youWon) {
    won_ = youWon;
    over_ = true;
    mode_ = Mode::End;
    if (youWon) {
        chord(523.25f, 659.25f, 1046.5f);
        if (sys_) {
            sys_->rumble(0.3f, 0.6f, 140);
            sys_->setLight(80, 220, 90);
        }
    } else {
        blip(70.f);
        if (sys_) sys_->setLight(180, 30, 40);
    }
}

void Game::noteCatch() {
    if (you_ >= kSeven || them_ >= kSeven) return;
    catches_++;
    you_++;
    blip(you_ >= kSeven ? 880.f : 520.f);
    if (you_ >= kSeven && them_ < kSeven) finish(true);
}

void Game::noteThem() {
    if (over_ || them_ >= kSeven || you_ >= kSeven) return;
    them_++;
    blip(240.f);
    if (them_ >= kSeven && you_ < kSeven) finish(false);
}

void Game::blip(float freq) {
    if (!sys_) return;
    sys_->apu.tone(0, freq, 0.06f);
    beep_ = 0.07f;
}

void Game::chord(float a, float b, float c) {
    if (!sys_) return;
    sys_->apu.tone(0, a, 0.08f);
    sys_->apu.tone(1, b, 0.06f);
    sys_->apu.tone(2, c, 0.05f);
    beep_ = 0.4f;
}

void Game::ageTone() {
    if (!sys_) return;
    if (beep_ > 0.f) {
        beep_ -= 1.f / 60.f;
        if (beep_ <= 0.f) {
            sys_->apu.tone(0, 0, 0);
            sys_->apu.tone(1, 0, 0);
            sys_->apu.tone(2, 0, 0);
        }
    }
}

void Game::stepPlay() {
    if (over_) return;

    clock_++;
    if (clock_ % kThemEvery == 0) noteThem();
    if (over_) return;

    if (bot_) {
        for (const auto& a : air_) {
            if (!a.on) continue;
            if (a.age >= kFlight - 6) latch_[a.to] = 10;
        }
    } else if (sys_) {
        const gs::Pad& p = sys_->pad;
        if (p.pressed(gs::BTN_LEFT) || p.pressed(gs::BTN_A) || p.pressed(gs::BTN_X)) latch_[0] = 12;
        if (p.pressed(gs::BTN_RIGHT) || p.pressed(gs::BTN_B) || p.pressed(gs::BTN_Y)) latch_[1] = 12;
    }

    if (tick_ % kEvery == 0) {
        int h = (tick_ / kEvery) & 1;
        if (hn_[h] <= 0) {
            miss();
            return;
        }
        int b = hold_[h][0];
        hold_[h][0] = hold_[h][1];
        hold_[h][1] = hold_[h][2];
        hn_[h]--;
        bool placed = false;
        for (auto& a : air_) {
            if (a.on) continue;
            a.on = true;
            a.ball = b;
            a.to = 1 - h;
            a.age = 0;
            placed = true;
            break;
        }
        if (!placed) {
            miss();
            return;
        }
    }

    for (auto& a : air_) {
        if (!a.on || a.age != kFlight) continue;
        int h = a.to;
        a.on = false;
        if (latch_[h] <= 0 || hn_[h] >= 3) {
            miss();
            return;
        }
        hold_[h][hn_[h]++] = a.ball;
        latch_[h] = 0;
        noteCatch();
        if (over_) return;
    }

    for (auto& a : air_)
        if (a.on && a.age < kFlight) a.age++;
    for (int i = 0; i < 2; i++)
        if (latch_[i] > 0) latch_[i]--;
    tick_++;
    rivalPhase_ += 0.18f;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    ageTone();
    if (mode_ == Mode::Title) {
        titleWait_++;
        bool go = sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A);
        if (bot_ && titleWait_ > 16) go = true;
        if (go) begin();
    } else if (mode_ == Mode::Play) {
        if (!bot_ && sys.pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Title;
            titleWait_ = 0;
        } else {
            stepPlay();
        }
    } else if (mode_ == Mode::Drop) {
        rivalPhase_ += 0.08f;
        if (--wait_ <= 0) pattern();
    } else if (mode_ == Mode::End) {
        rivalPhase_ += 0.05f;
        if (!bot_ && (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A))) begin();
    }
    if (mode_ == Mode::Play) sys.setLight(40, 90, 180);
    draw();
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        uint16_t c = gs::rgb4(1, 1, 4);
        if (y > 28) c = gs::rgb4(2, 2, 6);
        if (y > 80) c = gs::rgb4(3, 2, 7);
        if (y > 140) c = gs::rgb4(2, 2, 4);
        if (y > 188) c = gs::rgb4(1, 1, 2);
        v.lineBackdrop[y] = c;
    }
}

void Game::spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool shadow) {
    if (!sys_ || img.w == 0 || w < 1.f || h < 1.f) return;
    gs::Sprite s;
    s.img = img;
    s.x = int16_t(std::lround(cx - w * 0.5f));
    s.y = int16_t(std::lround(cy - h * 0.5f));
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.pal = uint8_t(pal);
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::ballAt(int ball, float x, float y, float scale) {
    const bool blue = ball == 1;
    const gs::Image& img = blue ? art_.blue : art_.red;
    int pal = blue ? PAL_BLUE : PAL_RED;
    float s = 16.f * scale;
    spr(img, x, y + 10.f, s * 0.85f, s * 0.3f, pal, true);
    spr(img, x, y, s, s, pal, false);
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
    int n = s ? int(std::strlen(s)) : 0;
    hud(20 - n / 2, row, s, pal);
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    spr(art_.booth, 52.f, 128.f, float(art_.booth.w), float(art_.booth.h), PAL_RIVAL);
    spr(art_.rival, 52.f, 132.f, float(art_.rival.w), float(art_.rival.h), PAL_RIVAL);
    for (int i = 0; i < 3; i++) {
        float u = rivalPhase_ + i * 2.1f;
        float x = 40.f + (i == 1 ? 24.f : (i == 0 ? 0.f : 12.f));
        float y = 118.f - std::sin(u) * 22.f;
        ballAt(i == 1 ? 1 : 0, x, y, 0.7f);
    }

    auto glove = [&](int h, float x) {
        bool hot = latch_[h] > 0 && mode_ == Mode::Play && !over_;
        spr(art_.glove, x, kHandY, hot ? 22.f : 18.f, hot ? 16.f : 14.f, PAL_GLOVE);
    };

    const bool showAir = mode_ == Mode::Play || mode_ == Mode::Drop || mode_ == Mode::End;
    if (showAir && mode_ != Mode::Title) {
        glove(0, kLeftX);
        glove(1, kRightX);
        for (const auto& a : air_) {
            if (!a.on) continue;
            float u = float(a.age) / float(kFlight);
            if (u < 0.f) u = 0.f;
            if (u > 1.f) u = 1.f;
            float x0 = handX(1 - a.to);
            float x = x0 + (handX(a.to) - x0) * u;
            float y = kHandY - 6.f - std::sin(u * kPi) * 86.f;
            ballAt(a.ball, x, y, 1.f);
        }
        for (int h = 0; h < 2; h++) {
            for (int i = 0; i < hn_[h]; i++) ballAt(hold_[h][i], handX(h), kHandY - 8.f - i * 11.f, 1.f);
        }
    } else {
        ballAt(0, kLeftX, kHandY - 20.f, 1.f);
        ballAt(1, 164.f, 78.f, 1.2f);
        ballAt(2, kRightX, kHandY - 20.f, 1.f);
        glove(0, kLeftX);
        glove(1, kRightX);
    }

    spr(art_.juggler, 164.f, 132.f, float(art_.juggler.w), float(art_.juggler.h), PAL_BODY);
    spr(art_.lamp, 92.f, 58.f, float(art_.lamp.w), float(art_.lamp.h), PAL_MARK);
    spr(art_.lamp, 250.f, 58.f, float(art_.lamp.w), float(art_.lamp.h), PAL_MARK);
    spr(art_.floor, 180.f, 206.f, float(art_.floor.w), float(art_.floor.h), PAL_STAGE);

    hud(1, 1, "S3 JUGGLE SEVEN", PAL_TITLE);
    char buf[48];
    std::snprintf(buf, sizeof buf, "YOU %d", you_);
    hud(1, 3, buf, you_ >= kSeven ? PAL_WIN : PAL_MARK);
    std::snprintf(buf, sizeof buf, "THEM %d", them_);
    hud(30, 3, buf, them_ >= kSeven ? PAL_ALERT : PAL_INK);
    hud(28, 1, "FIRST 7", PAL_INK);

    if (mode_ == Mode::Title) {
        hudC(22, "FIRST TO SEVEN", PAL_MARK);
        hudC(24, "LEFT AND RIGHT CATCH", PAL_INK);
        hudC(26, "SIX IS STILL SHORT", PAL_INK);
    } else if (mode_ == Mode::Play && you_ == kSeven - 1) {
        hudC(26, "SIX IS STILL SHORT", PAL_MARK);
    } else if (mode_ == Mode::Play) {
        hudC(26, "CATCH BEFORE THEY DO", PAL_INK);
    } else if (mode_ == Mode::Drop) {
        hudC(25, "DROP  TALLY HOLDS", PAL_ALERT);
    } else if (mode_ == Mode::End && won_) {
        hudC(23, "FIRST TO SEVEN", PAL_WIN);
        std::snprintf(buf, sizeof buf, "YOU %d  THEM %d", you_, them_);
        hudC(25, buf, PAL_MARK);
    } else if (mode_ == Mode::End) {
        hudC(24, "THEY TOOK SEVEN", PAL_ALERT);
        hudC(26, "SIX WAS NOT ENOUGH", PAL_INK);
    }
}

}  // namespace juggleseven

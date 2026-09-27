#include "game/span.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace spandoor {
namespace {

constexpr float DT = 1.0f / 60.0f;

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Won) return 2;
    if (mode_ == Mode::Lost) return 3;
    if (mode_ == Mode::Play || mode_ == Mode::Pause) return 1;
    return 0;
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.07f);
    beep_ = 0.05f;
}

void Game::schedule() {
    markCount_ = 0;
    markNext_ = 0;
    auto add = [&](float sec, int kind) {
        if (markCount_ >= int(sizeof marks_ / sizeof marks_[0])) return;
        marks_[markCount_++] = {int(std::lround(sec * 60.f)), kind};
    };
    for (int i = 0; i < 14; i++) add(7.f + i * 12.f, SHEAR);
    for (int i = 0; i < 11; i++) add(14.f + i * 15.f, WAGON);
    for (int i = 1; i < 22; i++) add(i * 8.f, FLIP);
    std::sort(marks_, marks_ + markCount_, [](const Mark& a, const Mark& b) {
        if (a.frame != b.frame) return a.frame < b.frame;
        return a.kind < b.kind;
    });
}

void Game::begin() {
    age_ = 0;
    score_ = 0;
    pins_ = 0;
    wagons_ = 0;
    wind_ = 1;
    gap_ = 0;
    lean_ = 1;
    stam_ = 1;
    shake_ = 0;
    beep_ = 0;
    shear_ = 0;
    wagon_ = 0;
    wagonPress_ = 0;
    pinned_ = false;
    wagonHeld_ = false;
    won_ = false;
    over_ = false;
    fanStep_ = -1;
    fanT_ = 0;
    gate_ = 12;
    schedule();
    mode_ = Mode::Play;
    sys_->setLight(30, 70, 110);
    sys_->apu.tone(1, 70.f, 0.03f);
}

void Game::finish(bool kept) {
    mode_ = kept ? Mode::Won : Mode::Lost;
    won_ = kept;
    over_ = true;
    gate_ = 20;
    fanStep_ = kept ? 0 : -1;
    fanT_ = 0;
    sys_->apu.tone(1, 0, 0);
    sys_->apu.tone(0, 0, 0);
    if (kept) {
        sys_->rumble(0.12f, 0.4f, 200);
        sys_->setLight(40, 110, 70);
    } else {
        gap_ = 1;
        sys_->apu.noiseBurst(0.6f, 220.f, 0.4f);
        sys_->rumble(0.7f, 0.2f, 240);
        sys_->setLight(110, 20, 16);
    }
}

Game::Intent Game::intent() {
    Intent in;
    if (bot_) {
        in.lean = float(wind_);
        in.pin = shear_ > 0 && shear_ <= 28 && !pinned_;
        bool push = wagon_ > 0;
        in.shoulder = push && stam_ > 0.08f;
        return in;
    }
    const gs::Pad& p = sys_->pad;
    float lean = p.axisX;
    if (p.down(gs::BTN_LEFT)) lean -= 1.f;
    if (p.down(gs::BTN_RIGHT)) lean += 1.f;
    in.lean = std::clamp(lean, -1.f, 1.f);
    in.pin = p.pressed(gs::BTN_B) || p.pressed(gs::BTN_A);
    in.shoulder = p.down(gs::BTN_C) || p.down(gs::BTN_TURBO) || p.accel > 0.35f;
    return in;
}

void Game::update() {
    while (markNext_ < markCount_ && age_ >= marks_[markNext_].frame) {
        int kind = marks_[markNext_].kind;
        if (kind == SHEAR && shear_ <= 0) {
            shear_ = 54;
            pinned_ = false;
            blip(240.f);
        } else if (kind == WAGON && wagon_ <= 0) {
            wagon_ = 70;
            wagonPress_ = 0;
            wagonHeld_ = false;
            blip(140.f);
        } else if (kind == FLIP) {
            wind_ = -wind_;
            blip(wind_ > 0 ? 330.f : 280.f);
        }
        markNext_++;
    }

    Intent in = intent();
    lean_ += (in.lean - lean_) * (bot_ ? 1.f : 0.22f);
    lean_ = std::clamp(lean_, -1.f, 1.f);

    if (in.pin && shear_ > 0 && shear_ <= 30 && !pinned_) {
        pinned_ = true;
        pins_++;
        gap_ = std::max(0.f, gap_ - 0.04f);
        shake_ = std::max(shake_, 0.35f);
        blip(520.f);
        sys_->apu.noiseBurst(0.18f, 640.f, 0.05f);
    }

    if (shear_ > 0) {
        shear_--;
        if (shear_ == 0 && !pinned_) {
            gap_ += 0.22f;
            shake_ = 0.8f;
            sys_->apu.noiseBurst(0.4f, 180.f, 0.12f);
        }
    }

    if (wagon_ > 0) {
        wagon_--;
        if (in.shoulder) wagonPress_++;
        if (wagon_ == 0) {
            if (wagonPress_ >= 48) {
                wagons_++;
                wagonHeld_ = true;
                gap_ = std::max(0.f, gap_ - 0.03f);
                blip(420.f);
            } else {
                wagonHeld_ = false;
                gap_ += 0.2f;
                shake_ = 0.9f;
                sys_->apu.noiseBurst(0.45f, 140.f, 0.14f);
            }
        }
    }

    bool withWind = lean_ * float(wind_) > 0.45f;
    if (withWind) gap_ -= 0.0035f;
    else gap_ += 0.011f;

    if (in.shoulder) stam_ = std::max(0.f, stam_ - 0.0045f);
    else stam_ = std::min(1.f, stam_ + 0.006f);

    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - 0.03f);
    gap_ = std::clamp(gap_, 0.f, 1.f);

    float drone = 62.f + gap_ * 90.f + (withWind ? 0.f : 18.f);
    sys_->apu.tone(1, drone, 0.028f + gap_ * 0.04f);

    age_++;
    score_ = age_ / 6 + pins_ * 30 + wagons_ * 45;
    if (beep_ > 0.f) {
        beep_ -= DT;
        if (beep_ <= 0.f) sys_->apu.tone(0, 0, 0);
    } else if (age_ % 60 == 0 && age_ < HOLD) {
        blip(age_ > HOLD - 600 ? 740.f : 392.f);
    }

    if (gap_ >= 1.f) finish(false);
    else if (age_ >= HOLD) finish(true);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(1, 2, 4));
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    if (bot_) begin();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (gate_ > 0) gate_--;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        bool go = pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) ||
                  pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_TURBO);
        if (go) begin();
    } else if (mode_ == Mode::Play) {
        if (gate_ == 0 && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else update();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Play;
            gate_ = 8;
        }
    } else if (fanStep_ >= 0) {
        fanT_ += DT;
        if (fanT_ >= 0.16f && fanStep_ < 4) {
            fanT_ = 0;
            static const float notes[] = {294.f, 370.f, 440.f, 587.f};
            sys.apu.tone(0, notes[fanStep_], 0.07f);
            fanStep_++;
            beep_ = 0.12f;
        } else if (fanStep_ >= 4 && beep_ > 0.f) {
            beep_ -= DT;
            if (beep_ <= 0.f) sys.apu.tone(0, 0, 0);
        }
        if (gate_ == 0 && pad.pressed(gs::BTN_START)) {
            sys.apu.tone(0, 0, 0);
            mode_ = Mode::Title;
            gate_ = 10;
        }
    } else if (gate_ == 0 && pad.pressed(gs::BTN_START)) {
        mode_ = Mode::Title;
        gate_ = 10;
    }
    draw();
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

void Game::text(const char* s, float x, float y, float scale, int pal) {
    if (!s) return;
    int n = int(std::strlen(s));
    const float adv = 18.0f * scale;
    x -= n * adv * 0.5f;
    for (int i = 0; i < n; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + i * adv + g.w * scale * 0.5f, y, g.h * scale, pal, false);
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 1.2f || m.h < 1) return;
    stamp(m, cx, cy, h * float(m.w) / float(m.h), h, pal, flip);
}

void Game::stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, bool flip) {
    if (w < 1.f || h < 1.f || m.h < 1) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 40 || s.x + s.w < -40 || s.y > gs::SCREEN_H + 40 || s.y + s.h < -40) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::backdrop() {
    float cold = (mode_ == Mode::Lost) ? 0.7f : gap_ * 0.45f;
    uint16_t top = lerpC(gs::rgb4(2, 3, 8), gs::rgb4(1, 1, 3), cold);
    uint16_t mid = lerpC(gs::rgb4(4, 7, 11), gs::rgb4(2, 3, 6), cold);
    uint16_t bot = lerpC(gs::rgb4(1, 3, 6), gs::rgb4(1, 1, 2), cold);
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float t = y / float(gs::SCREEN_H - 1);
        uint16_t c = t < 0.62f ? lerpC(top, mid, t / 0.62f) : lerpC(mid, bot, (t - 0.62f) / 0.38f);
        v.lineBackdrop[y] = c;
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    float shx = 0, shy = 0;
    if (shake_ > 0.f) {
        shx = std::sin(age_ * 1.8f) * shake_ * 6.f;
        shy = std::cos(age_ * 2.2f) * shake_ * 2.5f;
    }

    if (mode_ == Mode::Title) {
        text("S3 SPANDOOR", 160, 18, 0.72f, PAL_GOLD);
        text("AT THE SPAN", 160, 42, 0.5f, PAL_HUD);
        text("HOLD THE DOOR", 160, 188, 0.48f, PAL_WARN);
        hud(4, 24, "ARROWS LEAN   B PIN   C HOLD", PAL_HUD);
    } else if (mode_ != Mode::Pause) {
        int remain = std::max(0, HOLD - age_);
        int sec = remain == 0 ? 0 : (remain + 59) / 60;
        char clock[16];
        std::snprintf(clock, sizeof clock, "%d:%02d", sec / 60, sec % 60);
        text(clock, 160, 16, 1.05f, (remain <= 600 && mode_ == Mode::Play) ? PAL_RED : PAL_GOLD);
    }
    if (mode_ == Mode::Play && shear_ > 0) text(pinned_ ? "PINNED" : "SHEAR", 160, 40, 0.62f, pinned_ ? PAL_GOLD : PAL_WARN);
    if (mode_ == Mode::Play && wagon_ > 0) text("WAGON", 160, shear_ > 0 ? 62.f : 40.f, 0.62f, PAL_RED);
    if (mode_ == Mode::Play) {
        bool with = lean_ * float(wind_) > 0.45f;
        hud(1, 26, wind_ > 0 ? "WIND >" : "< WIND", with ? PAL_HUD : PAL_RED);
    }
    if (mode_ == Mode::Pause) text("PAUSED", 160, 100, 0.9f, PAL_HUD);
    if (mode_ == Mode::Won) {
        text("THE DOOR HELD", 160, 70, 0.58f, PAL_GOLD);
        text("THREE MINUTES", 160, 94, 0.5f, PAL_HUD);
    }
    if (mode_ == Mode::Lost) text("WATCH IS OVER", 160, 82, 0.58f, PAL_RED);

    if (mode_ == Mode::Play || mode_ == Mode::Pause) {
        float fw = 64.f * stam_;
        stamp(art_.chip, 250, 210, 68, 6, PAL_WATER, false);
        if (fw > 1.f) stamp(art_.chip, 218 + fw * 0.5f, 210, fw, 4, stam_ < 0.25f ? PAL_RED : PAL_GOLD, false);
        float gw = 64.f * (1.f - gap_);
        stamp(art_.chip, 70, 210, 68, 6, PAL_STEEL, false);
        if (gw > 1.f) stamp(art_.chip, 38 + gw * 0.5f, 210, gw, 4, gap_ > 0.55f ? PAL_RED : PAL_HUD, false);
    }

    float slide = (mode_ == Mode::Lost ? 1.f : gap_) * 78.f;
    float leafX = 118.f + slide + shx;
    bool leanArt = lean_ * float(wind_) > 0.2f && std::fabs(lean_) > 0.35f;
    int gust = int(sys_->frame / 5);

    if (mode_ != Mode::Title) {
        spr(leanArt ? art_.watchLean : art_.watch, 92 + lean_ * 10.f + shx, 132 + shy, 58, PAL_COAT, lean_ < 0);
        if (wagon_ > 0) {
            float wx = 250.f - (70 - wagon_) * 1.4f;
            spr(art_.wagon, wx + shx, 150, 40, PAL_WAGON, wind_ < 0);
        }
        if (pinned_ && shear_ > 0) spr(art_.pin, leafX - 8, 78, 28, PAL_LAMP, false);
    } else {
        spr(art_.watch, 96, 130, 58, PAL_COAT, false);
    }

    spr(art_.gate, leafX, 112 + shy, 118, PAL_STEEL, false);
    spr(art_.lamp, 46, 48 + float(gust & 1), 22, PAL_LAMP, false);
    spr(art_.lamp, 274, 48 + float((gust + 1) & 1), 22, PAL_LAMP, false);

    for (int i = 0; i < 5; i++) {
        float y = 36.f + i * 14.f;
        float x = std::fmod(float((age_ * (3 + (i & 1)) + i * 40) * (wind_ >= 0 ? 1 : -1)), 340.f);
        if (x < 0) x += 340.f;
        spr(art_.streak, x - 10.f, y, 3.5f, PAL_CABLE, wind_ < 0);
    }

    spr(art_.cable, 78, 58, 36, PAL_CABLE, false);
    spr(art_.cable, 242, 58, 36, PAL_CABLE, true);
    spr(art_.tower, 28, 108, 168, PAL_STEEL, false);
    spr(art_.tower, 292, 108, 168, PAL_STEEL, true);
    stamp(art_.deck, 160 + shx * 0.3f, 184, 300, 32, PAL_WATER, false);
    stamp(art_.chip, 160, 204, 300, 16, PAL_WATER, false);
}

}  // namespace spandoor

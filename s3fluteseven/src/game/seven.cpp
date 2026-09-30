#include "game/seven.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace fluteseven {
namespace {

constexpr float kLeft = 36.f;
constexpr float kSpeed = 1.8f;
constexpr float kWindow = 12.f;
constexpr int kRivalEvery = 100;
constexpr float kMarkX[kSeven] = {74.f, 106.f, 138.f, 170.f, 202.f, 234.f, 266.f};
// C D E F G A B — seven fingers. Phrase walks up, then settles.
constexpr int kPhrase[kSeven] = {0, 2, 4, 5, 4, 2, 6};
constexpr float kHz[kSeven] = {261.6f, 293.7f, 329.6f, 349.2f, 392.0f, 440.0f, 493.9f};
constexpr char kName[kSeven][2] = {{'C', 0}, {'D', 0}, {'E', 0}, {'F', 0}, {'G', 0}, {'A', 0}, {'B', 0}};

float clampf(float v, float a, float b) {
    if (v < a) return a;
    if (v > b) return b;
    return v;
}

}  // namespace

float Game::headX() const { return kLeft + phase_; }

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    you_ = 0;
    them_ = 0;
    won_ = false;
    over_ = false;
    left_ = false;
    why_ = "";
    mode_ = Mode::Title;
    finger_ = 0;
    hold_ = 0;
    tick_ = 0;
    t_ = 0;
    phase_ = 0;
    breath_ = 0;
    rivalTick_ = 0;
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(1, 1, 3));
    sys.apu.setMaster(0.8f);
}

void Game::begin() {
    you_ = them_ = 0;
    won_ = false;
    over_ = false;
    left_ = false;
    why_ = "";
    phase_ = 0;
    breath_ = 0;
    shake_ = 0;
    finger_ = 0;
    rivalTick_ = 0;
    hold_ = 0;
    mode_ = Mode::Play;
    tone(220.f, 0.04f, 3);
}

void Game::tone(float freq, float vol, int frames) {
    if (!sys_ || fanT_ > 0) return;
    sys_->apu.tone(0, freq, vol);
    sys_->apu.tone(1, freq * 2.f, vol * 0.22f);
    tone_ = frames;
}

void Game::blow() {
    if (you_ >= kSeven) return;
    int n = you_;
    you_++;
    breath_ = 12;
    shake_ = 3;
    phase_ = 0;
    finger_ = 0;
    if (sys_) {
        sys_->rumble(0.05f, 0.12f, 40);
        sys_->setLight(40, 160, 180);
    }
    tone(kHz[kPhrase[n]], 0.11f, 12);
    if (you_ >= kSeven && them_ < kSeven) winLeave();
}

void Game::rivalPoint() {
    if (them_ >= kSeven || you_ >= kSeven) return;
    them_++;
    if (sys_) {
        sys_->apu.tone(2, 196.f, 0.06f);
        tone_ = std::max(tone_, 6);
    }
    if (them_ >= kSeven && you_ < kSeven) lose();
}

void Game::miss() {
    phase_ = 0;
    breath_ = 0;
    shake_ = 6;
    missT_ = 22;
    mode_ = Mode::Miss;
    if (sys_) {
        sys_->apu.noiseBurst(0.12f, 700.f, 0.14f);
        sys_->apu.tone(0, 90.f, 0.05f);
        tone_ = 8;
        sys_->rumble(0.2f, 0.04f, 50);
        sys_->setLight(30, 20, 80);
    }
}

void Game::winLeave() {
    mode_ = Mode::Leave;
    hold_ = 0;
    fanT_ = 1;
    why_ = "first to seven";
}

void Game::lose() {
    won_ = false;
    left_ = false;
    over_ = true;
    why_ = "the other flute was first to seven";
    mode_ = Mode::Over;
    if (sys_) sys_->apu.noiseBurst(0.22f, 80.f, 0.18f);
}

void Game::stepPlay() {
    if (you_ >= kSeven) return;
    rivalTick_++;
    if (rivalTick_ >= kRivalEvery) {
        rivalTick_ = 0;
        rivalPoint();
        if (mode_ != Mode::Play) return;
    }

    int n = you_;
    if (bot_) finger_ = kPhrase[n];
    else if (sys_) {
        const gs::Pad& pad = sys_->pad;
        if (pad.pressed(gs::BTN_UP) || pad.pressed(gs::BTN_RIGHT)) finger_ = (finger_ + 1) % kSeven;
        if (pad.pressed(gs::BTN_DOWN) || pad.pressed(gs::BTN_LEFT)) finger_ = (finger_ + kSeven - 1) % kSeven;
    }

    float d = headX() - kMarkX[n];
    bool tap = false;
    if (bot_) tap = d >= -kSpeed && d <= 2.4f;
    else if (sys_) {
        const gs::Pad& pad = sys_->pad;
        tap = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_C);
    }

    if (tap) {
        if (finger_ == kPhrase[n] && std::fabs(d) <= kWindow) blow();
        else miss();
        return;
    }

    phase_ += kSpeed;
    if (headX() > kMarkX[n] + kWindow) miss();
}

void Game::audio() {
    if (!sys_) return;
    gs::APU& a = sys_->apu;
    if (fanT_ > 0) {
        fanT_++;
        if (fanT_ == 2 || fanT_ == 10 || fanT_ == 18 || fanT_ == 28) {
            int i = (fanT_ / 8) % kSeven;
            a.tone(0, kHz[kPhrase[i]], 0.1f);
            a.tone(1, kHz[kPhrase[i]] * 2.f, 0.03f);
        }
        if (fanT_ > 64) {
            a.tone(0, 0, 0);
            a.tone(1, 0, 0);
            a.tone(2, 0, 0);
            fanT_ = 0;
        }
        return;
    }
    if (tone_ > 0) {
        tone_--;
        if (tone_ == 0) {
            a.tone(0, 0, 0);
            a.tone(1, 0, 0);
            a.tone(2, 0, 0);
        }
    }
}

void Game::image(const gs::Mipped& m, float left, float top, float h, int pal, bool flip, int fog) {
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
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::spr(const gs::Mipped& m, float cx, float bottom, float h, int pal, bool flip, int fog) {
    if (m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    image(m, cx - w * 0.5f, bottom - h, h, pal, flip, fog);
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.road[y].on = false;
        v.lineFog[y] = y > 186 ? uint8_t((y - 186) / 8) : 0;
        if (y < 132) {
            float u = float(y) / 132.f;
            v.lineBackdrop[y] = gs::rgb4(1, 1 + int(u * 2.f), 5 + int((1.f - u) * 5.f));
        } else {
            float u = float(y - 132) / 92.f;
            v.lineBackdrop[y] = gs::rgb4(2 + int(u * 2.f), 2, 3);
        }
    }
}

void Game::stage() {
    spr(art_.arch, 160.f, 128.f, 78.f, PAL_HALL, false, 1);
    spr(art_.lamp, 28.f, 150.f, 42.f, PAL_GOLD, false, 0);
    spr(art_.lamp, 292.f, 150.f, 42.f, PAL_GOLD, false, 0);

    float jig = shake_ ? ((tick_ & 1) ? 1.f : -1.f) : 0.f;
    spr(art_.you, 78.f + jig, 168.f, 62.f, PAL_YOU, false, 0);
    spr(art_.them, 250.f, 168.f, 62.f, PAL_THEM, true, 0);
    float lift = breath_ > 0 ? -2.f : 0.f;
    image(art_.flute, 62.f, 112.f + lift, 11.f, PAL_WOOD, false, 0);
    image(art_.flute, 210.f, 116.f, 11.f, PAL_WOOD, true, 1);

    image(art_.staff, 28.f, 176.f, 24.f, PAL_NOTE, false, 0);
    for (int i = 0; i < kSeven; i++) {
        bool got = i < you_;
        const gs::Mipped& nimg = got ? art_.noteOn : art_.note;
        int pal = got ? PAL_GREEN : PAL_NOTE;
        float ny = 198.f - float(kPhrase[i]) * 2.2f;
        spr(nimg, kMarkX[i], ny, 14.f, pal, false, 0);
    }
    if (mode_ == Mode::Play || mode_ == Mode::Title || mode_ == Mode::Miss) {
        float span = kMarkX[kSeven - 1] - kLeft;
        float bx = (mode_ == Mode::Title) ? (kLeft + std::fmod(t_ * 40.f, span)) : headX();
        spr(art_.breath, bx, 188.f, 12.f, PAL_GOLD, false, 0);
    }

    for (int i = 0; i < kSeven; i++) {
        int pal = i < them_ ? PAL_RED : PAL_HALL;
        spr(art_.pip, 100.f + float(i) * 16.f, 214.f, 10.f, pal, false, 0);
    }
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (!sys_ || row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        int tile = art_.font[c - 32];
        if (!tile) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(tile, pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();
    stage();

    if (mode_ == Mode::Title) {
        hudC(1, "S3 FLUTE SEVEN", PAL_GOLD);
        hudC(3, "A SHORT FLUTE", PAL_HUD);
        hudC(5, "FIRST TO SEVEN", PAL_GREEN);
        hudC(7, "FINGER C-B  BLOW ON THE NOTE", PAL_HUD);
        hudC(9, "ENTER STARTS", PAL_GOLD);
        return;
    }

    hud(1, 0, "S3 FLUTE SEVEN", PAL_GOLD);
    char buf[48];
    std::snprintf(buf, sizeof buf, "YOU %d", you_);
    hud(1, 2, buf, you_ >= kSeven ? PAL_GREEN : PAL_GOLD);
    std::snprintf(buf, sizeof buf, "THEM %d", them_);
    hud(30, 2, buf, them_ >= kSeven ? PAL_RED : PAL_HUD);
    if (mode_ == Mode::Play || mode_ == Mode::Miss) {
        std::snprintf(buf, sizeof buf, "FINGER %s", kName[finger_ % kSeven]);
        hud(16, 0, buf, PAL_GOLD);
    }

    if (mode_ == Mode::Leave || (mode_ == Mode::Over && won_)) {
        hudC(4, "FIRST TO SEVEN", PAL_GREEN);
        hudC(6, "LEAVE", PAL_GOLD);
        return;
    }
    if (mode_ == Mode::Over) {
        hudC(4, "SHORT", PAL_RED);
        hudC(6, why_ ? why_ : "NOT FIRST", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Miss) {
        hudC(4, "MISS", PAL_RED);
        hudC(6, "SIX IS STILL SHORT", PAL_GOLD);
        return;
    }
    hudC(4, "BLOW THE NEXT NOTE", PAL_HUD);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += 1.f / 60.f;
    tick_++;
    if (shake_ > 0) shake_--;
    if (breath_ > 0) breath_--;

    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        hold_++;
        bool go = bot_ ? hold_ >= 24 : (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C));
        if (!bot_ && pad.pressed(gs::BTN_MODE)) sys.quit();
        if (go) begin();
        draw();
        audio();
        return;
    }
    if (mode_ == Mode::Miss) {
        missT_--;
        rivalTick_++;
        if (rivalTick_ >= kRivalEvery) {
            rivalTick_ = 0;
            rivalPoint();
        }
        if (mode_ == Mode::Miss && (bot_ ? missT_ <= 0 : (missT_ <= 0 || pad.pressed(gs::BTN_C)))) mode_ = Mode::Play;
        draw();
        audio();
        return;
    }
    if (mode_ == Mode::Leave) {
        hold_++;
        if (hold_ > 50) {
            won_ = true;
            left_ = true;
            over_ = true;
            mode_ = Mode::Over;
            if (!sys.headless) sys.quit();
        }
        draw();
        audio();
        return;
    }
    if (mode_ == Mode::Over) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) begin();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) sys.quit();
        draw();
        audio();
        return;
    }

    if (!bot_ && pad.pressed(gs::BTN_MODE)) {
        sys.quit();
        return;
    }

    stepPlay();
    draw();
    audio();
}

}  // namespace fluteseven

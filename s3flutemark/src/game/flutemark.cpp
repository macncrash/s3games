#include "game/flutemark.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace flutemark {
namespace {

constexpr float kLeft = 52.f;
constexpr float kRight = 268.f;
constexpr float kSpeed = 1.35f;
constexpr float kWindow = 10.f;
constexpr float kMarkX[Game::kMarks] = {92.f, 146.f, 200.f, 254.f};
// Phrase: C E D G. Finger 0..3 selects those pitches.
constexpr int kPhrase[Game::kMarks] = {0, 2, 1, 3};
constexpr float kHz[4] = {261.6f, 293.7f, 329.6f, 392.0f};
constexpr char kName[4][2] = {{'C', 0}, {'D', 0}, {'E', 0}, {'G', 0}};

float clampf(float v, float a, float b) {
    if (v < a) return a;
    if (v > b) return b;
    return v;
}

}  // namespace

int Game::notes() const {
    int n = 0;
    for (int i = 0; i < kMarks; i++)
        if (did_[i]) n++;
    return n;
}

float Game::headX() const { return kLeft + phase_; }

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    misses_ = 0;
    won_ = false;
    over_ = false;
    finished_ = false;
    mode_ = Mode::Title;
    finger_ = 0;
    hold_ = 0;
    tick_ = 0;
    t_ = 0;
    phase_ = 0;
    breath_ = 0;
    for (int i = 0; i < kMarks; i++) did_[i] = false;
    say_[0] = 0;
    sayT_ = 0;
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(1, 1, 3));
    sys.apu.setMaster(0.8f);
}

void Game::beginPhrase(bool keepMisses) {
    if (!keepMisses) misses_ = 0;
    won_ = false;
    over_ = false;
    finished_ = false;
    phase_ = 0;
    breath_ = 0;
    shake_ = 0;
    finger_ = 0;
    for (int i = 0; i < kMarks; i++) did_[i] = false;
    mode_ = Mode::Play;
    std::snprintf(say_, sizeof say_, "BREATHE");
    sayT_ = 0.5f;
    tone(220.f, 0.04f, 3);
}

void Game::tone(float freq, float vol, int frames) {
    if (!sys_ || fanT_ > 0) return;
    sys_->apu.tone(0, freq, vol);
    sys_->apu.tone(1, freq * 2.f, vol * 0.25f);
    tone_ = frames;
}

void Game::blow() {
    int n = notes();
    if (n >= kMarks) return;
    did_[n] = true;
    breath_ = 12;
    shake_ = 3;
    std::snprintf(say_, sizeof say_, "%s", kName[kPhrase[n]]);
    sayT_ = 0.4f;
    if (sys_) {
        sys_->rumble(0.05f, 0.12f, 40);
        sys_->setLight(40, 160, 180);
    }
    tone(kHz[kPhrase[n]], 0.1f, 14);
    if (n + 1 == kMarks) finishMark();
}

void Game::drop() {
    for (int i = 0; i < kMarks; i++) did_[i] = false;
    misses_++;
    phase_ = 0;
    breath_ = 0;
    shake_ = 6;
    missT_ = 36;
    mode_ = Mode::Miss;
    won_ = false;
    finished_ = false;
    std::snprintf(say_, sizeof say_, "MISS");
    sayT_ = 0.7f;
    if (sys_) {
        sys_->apu.noiseBurst(0.12f, 700.f, 0.16f);
        sys_->apu.tone(0, 90.f, 0.05f);
        tone_ = 8;
        sys_->rumble(0.25f, 0.05f, 70);
        sys_->setLight(30, 20, 80);
    }
}

void Game::finishMark() {
    finished_ = true;
    won_ = true;
    mode_ = Mode::Win;
    hold_ = 0;
    fanT_ = 1;
    std::snprintf(say_, sizeof say_, "FINISHED MARK");
    sayT_ = 3.f;
    if (sys_) {
        sys_->rumble(0.2f, 0.45f, 200);
        sys_->setLight(30, 180, 140);
    }
}

bool Game::botBlow() const {
    if (mode_ != Mode::Play) return false;
    int n = notes();
    if (n >= kMarks) return false;
    float d = headX() - kMarkX[n];
    return d >= -2.4f && d <= 2.4f && finger_ == kPhrase[n];
}

void Game::stepPlay() {
    int n = notes();
    if (bot_) {
        if (n < kMarks) finger_ = kPhrase[n];
    } else if (sys_) {
        const gs::Pad& pad = sys_->pad;
        if (pad.pressed(gs::BTN_UP) || pad.pressed(gs::BTN_RIGHT)) finger_ = (finger_ + 1) & 3;
        if (pad.pressed(gs::BTN_DOWN) || pad.pressed(gs::BTN_LEFT)) finger_ = (finger_ + 3) & 3;
    }

    bool tap = false;
    if (bot_) tap = botBlow();
    else if (sys_) {
        const gs::Pad& pad = sys_->pad;
        tap = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_C);
    }

    if (tap) {
        if (n < kMarks && finger_ == kPhrase[n] && std::fabs(headX() - kMarkX[n]) <= kWindow) blow();
        else drop();
        return;
    }

    phase_ += kSpeed;
    float x = headX();
    if (n < kMarks && x > kMarkX[n] + kWindow) {
        drop();
        return;
    }
    if (phase_ > (kRight - kLeft)) phase_ = 0;
}

void Game::audio() {
    if (!sys_) return;
    gs::APU& a = sys_->apu;
    if (fanT_ > 0) {
        fanT_++;
        if (fanT_ == 2 || fanT_ == 14 || fanT_ == 26 || fanT_ == 40) {
            int i = fanT_ < 14 ? 0 : fanT_ < 26 ? 1 : fanT_ < 40 ? 2 : 3;
            a.tone(0, kHz[kPhrase[i]], 0.11f);
            a.tone(1, kHz[kPhrase[i]] * 2.f, 0.03f);
        }
        if (fanT_ > 72) {
            a.tone(0, 0, 0);
            a.tone(1, 0, 0);
            fanT_ = 0;
        }
        return;
    }
    if (tone_ > 0) {
        tone_--;
        if (tone_ == 0) {
            a.tone(0, 0, 0);
            a.tone(1, 0, 0);
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
        v.lineFog[y] = y > 180 ? uint8_t((y - 180) / 8) : 0;
        if (y < 140) {
            float u = float(y) / 140.f;
            v.lineBackdrop[y] = gs::rgb4(1 + int(u), 1 + int(u * 2.f), 6 + int((1.f - u) * 4.f));
        } else {
            float u = float(y - 140) / 84.f;
            v.lineBackdrop[y] = gs::rgb4(3 + int(u * 2.f), 2, 3);
        }
    }
}

void Game::stage() {
    image(art_.window, 18.f, 16.f, 92.f, PAL_NIGHT, false, 1);
    image(art_.window, 266.f, 16.f, 92.f, PAL_NIGHT, true, 1);
    spr(art_.lamp, 48.f, 148.f, 46.f, PAL_LAMP, false, 0);
    spr(art_.lamp, 276.f, 148.f, 46.f, PAL_LAMP, false, 0);
    spr(art_.stand, 108.f, 168.f, 52.f, PAL_WOOD, false, 0);

    float jig = shake_ ? ((tick_ & 1) ? 1.f : -1.f) : 0.f;
    spr(art_.player, 168.f + jig, 176.f, 70.f, PAL_PLAYER, false, 0);
    float lift = breath_ > 0 ? -2.f : 0.f;
    image(art_.flute, 150.f, 118.f + lift, 12.f, PAL_WOOD, false, 0);

    image(art_.staff, 50.f, 184.f, 28.f, PAL_NOTE, false, 0);
    for (int i = 0; i < kMarks; i++) {
        const gs::Mipped& nimg = did_[i] ? art_.noteOn : art_.note;
        int pal = did_[i] ? PAL_GREEN : PAL_NOTE;
        float ny = 208.f - float(kPhrase[i]) * 4.f;
        spr(nimg, kMarkX[i], ny, 16.f, pal, false, 0);
    }
    if (mode_ == Mode::Play || mode_ == Mode::Title) {
        float bx = (mode_ == Mode::Title) ? (kLeft + std::fmod(t_ * 36.f, kRight - kLeft)) : headX();
        spr(art_.breath, bx, 196.f, 14.f, PAL_GOLD, false, 0);
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

    if (mode_ == Mode::Win) {
        spr(art_.card, 160.f, 78.f, 34.f, PAL_GOLD, false, 0);
        spr(art_.stamp, 160.f, 70.f, 14.f, PAL_RED, false, 0);
    }

    if (mode_ == Mode::Title) {
        hudC(2, "S3 FLUTEMARK", PAL_GOLD);
        hudC(4, "PLAY THE FLUTE", PAL_HUD);
        hudC(6, "A FINISHED MARK ENDS IT", PAL_HUD);
        hudC(8, "FINGER C D E G  BLOW ON THE NOTE", PAL_HUD);
        hudC(21, "ENTER STARTS", PAL_GREEN);
        return;
    }

    hud(1, 0, "S3 FLUTEMARK", PAL_GOLD);
    char pips[8];
    for (int i = 0; i < kMarks; i++) pips[i] = did_[i] ? '#' : '-';
    pips[kMarks] = 0;
    char buf[40];
    std::snprintf(buf, sizeof buf, "MARK %s", pips);
    hud(28, 0, buf, notes() == kMarks ? PAL_GREEN : PAL_HUD);
    std::snprintf(buf, sizeof buf, "FINGER %s", kName[finger_ & 3]);
    hud(1, 2, buf, PAL_GOLD);

    if (mode_ == Mode::Pause) {
        hudC(4, "PAUSED", PAL_GOLD);
        hudC(6, "ENTER RESUMES", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Win) {
        hudC(10, "FINISHED MARK", PAL_GREEN);
        hudC(12, "THE CARD IS CLOSED", PAL_GOLD);
        hudC(14, "LEAVE", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Miss) {
        hudC(4, "MISS", PAL_RED);
        hudC(6, "THE MARK IS OPEN", PAL_GOLD);
        return;
    }
    if (sayT_ > 0 && say_[0]) hudC(3, say_, PAL_GOLD);
    else hudC(3, "BLOW THE MARK", PAL_HUD);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += 1.f / 60.f;
    tick_++;
    if (sayT_ > 0) sayT_ = std::max(0.f, sayT_ - 1.f / 60.f);
    if (shake_ > 0) shake_--;
    if (breath_ > 0) breath_--;

    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        hold_++;
        bool go = bot_ ? hold_ >= 24 : (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C));
        if (!bot_ && pad.pressed(gs::BTN_MODE)) sys.quit();
        if (go) beginPhrase(false);
        draw();
        audio();
        return;
    }
    if (mode_ == Mode::Pause) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) mode_ = Mode::Play;
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) sys.quit();
        draw();
        audio();
        return;
    }
    if (mode_ == Mode::Miss) {
        missT_--;
        if (bot_) {
            if (missT_ <= 0) beginPhrase(true);
        } else if (missT_ <= 0 || pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C)) {
            beginPhrase(true);
        }
        draw();
        audio();
        return;
    }
    if (mode_ == Mode::Win) {
        hold_++;
        if (hold_ > 70) {
            over_ = true;
            if (!sys.headless) sys.quit();
        }
        draw();
        audio();
        return;
    }

    if (!bot_ && pad.pressed(gs::BTN_START)) {
        mode_ = Mode::Pause;
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

}  // namespace flutemark

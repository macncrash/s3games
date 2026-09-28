#include "game/keysmark.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace keysmark {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr int kCount = 8;
constexpr int kSlips = 3;
constexpr int kLane[kCount] = {0, 1, 2, 3, 2, 1, 0, 3};
constexpr int kMidi[kCount] = {60, 62, 64, 67, 64, 62, 60, 67};
constexpr int kKeyX[4] = {56, 124, 192, 260};
constexpr gs::Button kBtn[4] = {gs::BTN_LEFT, gs::BTN_DOWN, gs::BTN_UP, gs::BTN_RIGHT};
constexpr gs::Button kAlt[4] = {gs::BTN_A, gs::BTN_B, gs::BTN_C, gs::BTN_Y};

float hz(int midi) { return float(440.0 * std::pow(2.0, (midi - 69) / 12.0)); }

float noteX(int i) { return 46.f + float(i) * 32.f; }

float noteY(int lane) { return 108.f - float(lane) * 8.f; }

}  // namespace

void Game::blip(float freq, float vol) {
    sys_->apu.tone(0, freq, vol);
    toneT_ = 0.16f;
}

void Game::toneOff() {
    if (toneT_ <= 0) return;
    toneT_ = std::max(0.f, toneT_ - kDt);
    if (toneT_ <= 0) {
        sys_->apu.tone(0, 0, 0);
        sys_->apu.tone(1, 0, 0);
        sys_->apu.tone(2, 0, 0);
    }
}

void Game::begin() {
    index_ = 0;
    slips_ = 0;
    won_ = false;
    over_ = false;
    finished_ = false;
    flash_ = 0;
    botWait_ = 0.35f;
    mode_ = Mode::Play;
    sys_->apu.silence();
    if (!bot_) sys_->setLight(40, 30, 10);
}

void Game::finish() {
    finished_ = true;
    won_ = true;
    over_ = true;
    mode_ = Mode::Over;
    flash_ = 1.f;
    sys_->apu.tone(0, hz(60), 0.12f);
    sys_->apu.tone(1, hz(64), 0.10f);
    sys_->apu.tone(2, hz(67), 0.10f);
    toneT_ = 0.45f;
    if (!bot_) sys_->setLight(40, 160, 50);
}

void Game::fail() {
    won_ = false;
    finished_ = false;
    over_ = true;
    mode_ = Mode::Over;
    sys_->apu.silence();
    sys_->apu.tone(0, 110.f, 0.16f);
    toneT_ = 0.35f;
    if (!bot_) sys_->setLight(160, 30, 20);
}

void Game::strike(int lane) {
    if (mode_ != Mode::Play || index_ >= kCount) return;
    if (lane == kLane[index_]) {
        blip(hz(kMidi[index_]), 0.16f);
        index_++;
        flash_ = 0.35f;
        if (index_ >= kCount) finish();
    } else {
        slips_++;
        sys_->apu.noiseBurst(0.12f, 240.f, 0.08f);
        if (!bot_) sys_->rumble(0.4f, 0.15f, 50);
        if (slips_ >= kSlips) fail();
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(1, 1, 2));
    for (int y = 0; y < gs::SCREEN_H; y++) sys.vdp.road[y].on = false;
    mode_ = Mode::Title;
    clock_ = 0;
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::spr(const gs::Mipped& m, float cx, float cy, float w, float h, int pal) {
    if (w < 1.f || h < 1.f || m.h < 1) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::word(const gs::Image& img, float cx, float y, int pal) {
    if (img.w < 1) return;
    gs::Sprite s;
    s.img = img;
    s.w = int16_t(img.w);
    s.h = int16_t(img.h);
    s.x = int16_t(std::lround(cx - img.w * 0.5f));
    s.y = int16_t(std::lround(y));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int g = 1 + y / 48;
        int b = 3 + (gs::SCREEN_H - y) / 40;
        v.lineBackdrop[y] = gs::rgb4(1, std::min(g, 4), std::min(b, 8));
        v.lineFog[y] = 0;
    }

    const bool closed = finished_;
    const int markPal = closed ? PAL_GOLD : PAL_MARK;
    const float left = noteX(0) - 14.f;
    const float right = noteX(kCount - 1) + 14.f;

    if (mode_ == Mode::Title) word(art_.logo, 160.f, 8.f, PAL_GOLD);
    else if (closed) word(art_.finished, 160.f, 8.f, PAL_GOLD);
    else if (mode_ == Mode::Over) word(art_.open, 160.f, 8.f, PAL_BAD);
    else hudC(0, "S3 KEYSMARK", PAL_GOLD);

    for (int i = 0; i < kSlips; i++) {
        int pal = i < slips_ ? PAL_BAD : PAL_LAMP;
        spr(art_.lamp, 250.f + float(i) * 16.f, 28.f, 10.f, 10.f, pal);
    }
    if (mode_ == Mode::Play && index_ < kCount)
        spr(art_.hand, float(kKeyX[kLane[index_]]), 142.f, 16.f, 12.f, PAL_HAND);

    for (int i = 0; i < kCount; i++) {
        int pal = i < index_ ? PAL_PLAYED : PAL_NOTE;
        if (mode_ == Mode::Play && i == index_) pal = (int(clock_ * 6.f) & 1) ? PAL_GOLD : PAL_NOTE;
        spr(art_.note, noteX(i), noteY(kLane[i]), 14.f, 11.f, pal);
    }
    for (int lane = 0; lane < 4; lane++) {
        bool lit = mode_ == Mode::Play && index_ < kCount && kLane[index_] == lane;
        float lift = lit ? 4.f : 0.f;
        spr(art_.key, float(kKeyX[lane]), 176.f - lift, 48.f, lit ? 52.f : 46.f, lit ? PAL_GOLD : PAL_IVORY);
    }

    spr(art_.bar, left, 78.f, 4.f, 56.f, markPal);
    spr(art_.bar, right, 78.f, 4.f, 56.f, markPal);
    spr(art_.staff, (left + right) * 0.5f, 50.f, right - left, 3.f, markPal);
    for (int s = 0; s < 5; s++) spr(art_.staff, 160.f, 78.f + float(s) * 8.f, 250.f, 2.f, PAL_INK);
    spr(art_.staff, 160.f, 168.f, 280.f, 18.f, PAL_WOOD);

    if (mode_ == Mode::Title) {
        hudC(23, "ONE PHRASE UNDER THE MARK", PAL_INK);
        hudC(24, "THE LAST NOTE CLOSES IT", PAL_INK);
        hudC(26, "ENTER STARTS", PAL_GOLD);
    } else if (mode_ == Mode::Pause) {
        hudC(23, "PAUSED", PAL_GOLD);
        hudC(25, "ENTER CONTINUES", PAL_INK);
    } else if (mode_ == Mode::Over && closed) {
        hudC(23, "FINISHED MARK", PAL_GOLD);
        char buf[40];
        std::snprintf(buf, sizeof buf, "NOTES %d  SLIPS %d", index_, slips_);
        hudC(25, buf, PAL_INK);
    } else if (mode_ == Mode::Over) {
        hudC(23, "THE MARK STAYS OPEN", PAL_BAD);
        char buf[40];
        std::snprintf(buf, sizeof buf, "NOTES %d  SLIPS %d", index_, slips_);
        hudC(25, buf, PAL_INK);
    } else {
        hudC(23, "LEFT DOWN UP RIGHT", PAL_INK);
        hudC(24, "OR Z X C W", PAL_INK);
        char buf[40];
        std::snprintf(buf, sizeof buf, "NOTE %d/%d", std::min(index_ + 1, kCount), kCount);
        hudC(26, buf, PAL_GOLD);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    clock_ += kDt;
    if (flash_ > 0) flash_ = std::max(0.f, flash_ - kDt);
    toneOff();

    const gs::Pad& pad = sys.pad;
    const bool start = pad.pressed(gs::BTN_START);
    const bool back = pad.pressed(gs::BTN_MODE);
    const bool action = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C);

    if (bot_) {
        if (mode_ == Mode::Title && clock_ > 0.45f) begin();
        else if (mode_ == Mode::Play) {
            botWait_ -= kDt;
            if (botWait_ <= 0.f && index_ < kCount) {
                botWait_ = 0.16f;
                strike(kLane[index_]);
            }
        }
    } else if (mode_ == Mode::Title) {
        if (back) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        } else if (start || action) begin();
    } else if (mode_ == Mode::Pause) {
        if (start) mode_ = held_;
        else if (back) {
            won_ = over_ = finished_ = false;
            mode_ = Mode::Title;
        }
    } else if (mode_ == Mode::Over) {
        if (start || action) begin();
        else if (back) {
            won_ = over_ = finished_ = false;
            mode_ = Mode::Title;
        }
    } else if (start) {
        held_ = mode_;
        mode_ = Mode::Pause;
    } else if (back) {
        won_ = over_ = finished_ = false;
        mode_ = Mode::Title;
    } else if (mode_ == Mode::Play) {
        int pressed = -1;
        for (int lane = 0; lane < 4; lane++) {
            if (!pad.pressed(kBtn[lane]) && !pad.pressed(kAlt[lane])) continue;
            if (index_ < kCount && lane == kLane[index_]) pressed = lane;
            else if (pressed < 0) pressed = lane;
        }
        if (pressed >= 0) strike(pressed);
    }

    draw();
}

}  // namespace keysmark

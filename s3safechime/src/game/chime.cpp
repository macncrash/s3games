#include "game/chime.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace safechime {
namespace {

constexpr float kPi = 3.14159265f;
struct Stop {
    int dir;
    int num;
};
constexpr Stop kCombo[kStops] = {{-1, 36}, {1, 4}, {-1, 0}};

float midiHz(int midi) { return 440.f * std::pow(2.f, (midi - 69) / 12.f); }

const char* kTag[kStops] = {"L36", "R04", "L00"};

}  // namespace

int Game::clockSec() const { return kStartSec + playFrames_ / kFpc; }

bool Game::onHour() const {
    int sec = clockSec();
    return sec >= kHourSec && sec < kHourSec + kGraceSec;
}

bool Game::pastHour() const { return clockSec() >= kHourSec + kGraceSec; }

void Game::face(int& h, int& m, int& s) const {
    int t = clockSec();
    if (t < 0) t = 0;
    s = t % 60;
    m = (t / 60) % 60;
    h = (t / 3600) % 12;
    if (h == 0) h = 12;
}

int Game::hour() const {
    int h, m, s;
    face(h, m, s);
    return h;
}
int Game::minute() const {
    int h, m, s;
    face(h, m, s);
    return m;
}
int Game::second() const {
    int h, m, s;
    face(h, m, s);
    return s;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    mode_ = Mode::Title;
    over_ = won_ = chimed_ = frozen_ = moved_ = false;
    dial_ = stage_ = lastDir_ = spinCool_ = lock_ = hold_ = playFrames_ = 0;
    clock_ = toneT_ = bellAmp_ = 0;
    reason_ = "hour silent";
    if (bot_) begin();
    draw();
}

void Game::begin() {
    over_ = won_ = chimed_ = frozen_ = false;
    dial_ = 0;
    stage_ = 0;
    lastDir_ = 0;
    spinCool_ = 0;
    lock_ = 0;
    playFrames_ = 0;
    reason_ = "hour silent";
    mode_ = Mode::Dial;
    sys_->apu.silence();
}

void Game::tone(float freq, float vol) {
    sys_->apu.tone(0, freq, vol);
    toneT_ = 0.08f;
}

void Game::decay() {
    if (toneT_ > 0) {
        toneT_ -= 1.f / 60.f;
        if (toneT_ <= 0) sys_->apu.tone(0, 0, 0);
    }
    if (lock_ > 0) lock_--;
    if (mode_ == Mode::Chime) {
        bellAmp_ = std::min(1.f, bellAmp_ + 0.05f);
        int stroke = (hold_ / 8) % 4;
        float f = midiHz(67 + stroke * 5);
        sys_->apu.tone(1, f, 0.22f * bellAmp_);
        sys_->apu.tone(2, f * 2.f, 0.06f * bellAmp_);
    } else if (bellAmp_ > 0) {
        bellAmp_ = std::max(0.f, bellAmp_ - 0.03f);
        if (bellAmp_ == 0) {
            sys_->apu.tone(1, 0, 0);
            sys_->apu.tone(2, 0, 0);
        }
    }
}

void Game::nudge(int dir) {
    if (dir == 0 || mode_ != Mode::Dial) return;
    dial_ = (dial_ + dir + kNotches) % kNotches;
    lastDir_ = dir;
    moved_ = true;
    spinCool_ = 2;
    tone(midiHz(72 + (dial_ % 5)), 0.12f);
}

void Game::latch() {
    if (mode_ != Mode::Dial || moved_ || lock_ > 0 || stage_ >= kStops) return;
    const Stop& s = kCombo[stage_];
    if (dial_ == s.num && lastDir_ == s.dir) {
        stage_++;
        lock_ = 10;
        tone(midiHz(84), 0.28f);
        if (stage_ == kStops) mode_ = Mode::Wait;
    } else {
        stage_ = 0;
        lock_ = 8;
        reason_ = "tumbler dropped";
        sys_->apu.noiseBurst(0.18f, 600.f, 0.08f);
        tone(midiHz(40), 0.2f);
    }
}

void Game::botDial() {
    if (stage_ >= kStops) return;
    const Stop& s = kCombo[stage_];
    if (dial_ != s.num || lastDir_ != s.dir) {
        if (spinCool_ <= 0) nudge(s.dir);
        else spinCool_--;
    } else {
        latch();
    }
}

void Game::humanDial() {
    const gs::Pad& pad = sys_->pad;
    int dir = 0;
    if (pad.down(gs::BTN_LEFT)) dir = -1;
    if (pad.down(gs::BTN_RIGHT)) dir = dir == 0 ? 1 : 0;
    if (dir != 0) {
        if (spinCool_ <= 0) nudge(dir);
        else spinCool_--;
    } else {
        spinCool_ = 0;
    }
    if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_Y)) latch();
}

void Game::beginChime() {
    chimed_ = true;
    won_ = true;
    frozen_ = true;
    reason_ = "CHIME";
    mode_ = Mode::Chime;
    hold_ = 48;
    bellAmp_ = 0.35f;
    tone(midiHz(84), 0.4f);
}

void Game::beginFail(const char* why) {
    frozen_ = true;
    won_ = false;
    chimed_ = false;
    reason_ = why;
    mode_ = Mode::Fail;
    hold_ = 40;
    tone(midiHz(36), 0.24f);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    clock_ += 1.f / 60.f;
    moved_ = false;
    decay();
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) begin();
        else if (pad.pressed(gs::BTN_MODE) && !bot_) sys.quit();
    } else if (mode_ == Mode::Dial || mode_ == Mode::Wait) {
        if (!frozen_) playFrames_++;
        if (mode_ == Mode::Dial) {
            if (bot_) botDial();
            else humanDial();
        }
        if (mode_ == Mode::Wait && onHour()) beginChime();
        else if (pastHour()) beginFail("HOUR");
        else if (pad.pressed(gs::BTN_MODE) && !bot_) sys.quit();
    } else if (mode_ == Mode::Chime || mode_ == Mode::Fail) {
        if (--hold_ <= 0) {
            over_ = true;
            sys.apu.tone(1, 0, 0);
            sys.apu.tone(2, 0, 0);
            if (!sys.headless) sys.quit();
        }
    }
    draw();
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

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal) {
    if (h < 1.f || m.h < 1) return;
    float sc = h / float(m.h);
    gs::Sprite s;
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 400));
    s.w = int16_t(std::clamp(int(std::lround(m.w * sc)), 1, 400));
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
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    vdp.A.clear();
    vdp.B.clear();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int shade = 1 + y / 70;
        vdp.lineBackdrop[y] = gs::rgb4(1, 1, std::min(5, shade));
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }

    const float ccx = 56.f, ccy = 48.f;
    int h = hour(), m = minute(), s = second();
    float secA = (s / 60.f) * 6.28318f - 1.5708f;
    float minA = ((m + s / 60.f) / 60.f) * 6.28318f - 1.5708f;
    float hourA = ((h % 12 + m / 60.f) / 12.f) * 6.28318f - 1.5708f;
    for (int i = 1; i <= 4; i++) {
        float t = i / 4.f;
        spr(art_.pip, ccx + std::cos(hourA) * 12.f * t, ccy + std::sin(hourA) * 12.f * t, 3.5f, PAL_INK);
    }
    for (int i = 1; i <= 5; i++) {
        float t = i / 5.f;
        spr(art_.pip, ccx + std::cos(minA) * 16.f * t, ccy + std::sin(minA) * 16.f * t, 3.5f, PAL_INK);
    }
    int handPal = chimed_ ? PAL_GOLD : PAL_GOLD;
    if (mode_ == Mode::Fail) handPal = PAL_DEAD;
    for (int i = 1; i <= 6; i++) {
        float t = i / 6.f;
        spr(art_.pip, ccx + std::cos(secA) * 20.f * t, ccy + std::sin(secA) * 20.f * t, 2.5f, handPal);
    }
    for (int i = 0; i < 12; i++) {
        float a = (i / 12.f) * 6.28318f - 1.5708f;
        spr(art_.pip, ccx + std::cos(a) * 22.f, ccy + std::sin(a) * 22.f, i % 3 == 0 ? 5.f : 3.f,
            i == 0 ? PAL_GOLD : PAL_INK);
    }
    spr(art_.face, ccx, ccy, 58.f, PAL_FACE);

    float sway = 0;
    if (mode_ == Mode::Chime) sway = std::sin(clock_ * 10.f) * (6.f + 8.f * bellAmp_);
    spr(art_.bell, 270.f + sway, 46.f, chimed_ ? 34.f : 24.f, PAL_BELL);

    bool open = stage_ == kStops;
    float doorX = open ? 188.f : 160.f;
    float dialX = doorX - 6.f;
    float dialY = 142.f;
    float ang = (dial_ / float(kNotches)) * 2.f * kPi - kPi * 0.5f;
    spr(art_.needle, dialX + std::cos(ang) * 10.f, dialY + std::sin(ang) * 10.f, 16.f, PAL_GOLD);
    spr(art_.dial, dialX, dialY, 46.f, PAL_DIAL);
    for (int i = 0; i < 8; i++) {
        float a = (i / 8.f) * 2.f * kPi - kPi * 0.5f;
        spr(art_.tick, dialX + std::cos(a) * 20.f, dialY + std::sin(a) * 20.f, 7.f, PAL_INK);
    }
    spr(art_.handle, doorX + 28.f, 150.f, open ? 40.f : 34.f, open ? PAL_GOLD : PAL_STEEL);
    spr(art_.door, doorX, 146.f, 100.f, PAL_DOOR);
    spr(art_.body, 150.f, 150.f, 128.f, PAL_STEEL);

    for (int i = 0; i < kStops; i++) {
        int pal = i < stage_ ? PAL_LAMP : PAL_DEAD;
        spr(art_.lamp, 118.f + i * 16.f, 196.f, 10.f, pal);
    }

    char line[48];
    std::snprintf(line, sizeof(line), "%d:%02d:%02d", h, m, s);
    hud(16, 1, line, chimed_ ? PAL_GOLD : PAL_INK);
    std::snprintf(line, sizeof(line), "DIAL %02d", dial_);
    hud(1, 3, line, PAL_INK);

    if (mode_ == Mode::Title) {
        word(art_.title, 200, 78, PAL_GOLD);
        word(art_.rule, 200, 108, PAL_INK);
        hudC(22, "L36  R04  L00", PAL_GOLD);
        hudC(24, "THE HOUR HAS TO CHIME", PAL_INK);
        hudC(26, "START", PAL_INK);
    } else if (mode_ == Mode::Wait) {
        hudC(24, "HOLD FOR THE HOUR", PAL_INK);
    } else if (mode_ == Mode::Chime) {
        hudC(24, "THE HOUR CHIMES", PAL_GOLD);
    } else if (mode_ == Mode::Fail) {
        hudC(24, "HOUR PASSED", PAL_DEAD);
    } else {
        int x = 12;
        for (int i = 0; i < kStops; i++) {
            hud(x, 24, kTag[i], i == stage_ ? PAL_GOLD : PAL_INK);
            x += 6;
        }
        hudC(26, "LEFT RIGHT  A LATCH", PAL_INK);
    }
}

}  // namespace safechime

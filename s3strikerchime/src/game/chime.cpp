#include "game/chime.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace strikerchime {
namespace {

constexpr float kPi = 3.14159265f;
constexpr float kOmega = 2.f * kPi / 90.f;
constexpr float kPhase = kPi * 0.5f - float(kRelease) * kOmega;
constexpr float kBaseY = 178.f;
constexpr float kBellY = 46.f;
constexpr float kRailTop = 78.f;
constexpr float kRailBot = 168.f;

float midiHz(int midi) { return 440.f * std::pow(2.f, (midi - 69) / 12.f); }

}  // namespace

void Game::splitLive(int& h, int& m, int& s) const {
    int t = kOpenSec + playFrames_ / 60;
    if (t < 0) t = 0;
    s = t % 60;
    m = (t / 60) % 60;
    h = (t / 3600) % 12;
    if (h == 0) h = 12;
}

int Game::liveHour() const {
    int h, m, s;
    splitLive(h, m, s);
    return h;
}
int Game::liveMinute() const {
    int h, m, s;
    splitLive(h, m, s);
    return m;
}
int Game::liveSecond() const {
    int h, m, s;
    splitLive(h, m, s);
    return s;
}

float Game::meter() const {
    float s = std::sin(float(playFrames_) * kOmega + kPhase);
    return 0.5f + 0.5f * s;
}

bool Game::onNotch() const { return meter() >= 0.993f; }

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    mode_ = Mode::Title;
    over_ = won_ = chimed_ = frozen_ = swung_ = clean_ = false;
    playFrames_ = 0;
    releaseFrame_ = 0;
    arrive_ = -1;
    hold_ = 0;
    bellAmp_ = toneT_ = 0;
    reason_ = "hour silent";
    if (bot_) begin();
    draw();
}

void Game::begin() {
    over_ = won_ = chimed_ = frozen_ = swung_ = clean_ = false;
    playFrames_ = 0;
    releaseFrame_ = 0;
    arrive_ = -1;
    reason_ = "hour silent";
    mode_ = Mode::Ready;
    sys_->apu.silence();
}

void Game::swing() {
    if (swung_ || mode_ != Mode::Ready) return;
    swung_ = true;
    releaseFrame_ = playFrames_;
    clean_ = onNotch();
    if (clean_) {
        arrive_ = playFrames_ + kFlight;
        mode_ = Mode::Rise;
    } else {
        arrive_ = -1;
        mode_ = Mode::Fall;
    }
    sys_->apu.noiseBurst(0.28f, 900.f, 0.08f);
    sys_->apu.tone(0, midiHz(48), 0.3f);
    toneT_ = 0.08f;
}

void Game::strikeHour() {
    chimed_ = true;
    won_ = true;
    frozen_ = true;
    fh_ = 12;
    fm_ = 0;
    fs_ = 0;
    reason_ = "the hour chimes";
    mode_ = Mode::Chime;
    hold_ = 50;
    bellAmp_ = 0.5f;
    sys_->apu.tone(1, midiHz(84), 0.42f);
    sys_->apu.tone(2, midiHz(96), 0.16f);
}

void Game::passHour(const char* why) {
    frozen_ = true;
    int h, m, s;
    splitLive(h, m, s);
    fh_ = h;
    fm_ = m;
    fs_ = s;
    won_ = false;
    chimed_ = false;
    reason_ = why;
    mode_ = Mode::Fail;
    hold_ = 42;
    sys_->apu.tone(0, midiHz(36), 0.24f);
    toneT_ = 0.2f;
}

void Game::decay() {
    if (toneT_ > 0) {
        toneT_ -= 1.f / 60.f;
        if (toneT_ <= 0) sys_->apu.tone(0, 0, 0);
    }
    if (mode_ == Mode::Chime) {
        bellAmp_ = std::min(1.f, bellAmp_ + 0.04f);
        int stroke = (hold_ / 8) % 3;
        float f = midiHz(84 - stroke * 5);
        sys_->apu.tone(1, f, 0.28f * bellAmp_);
        sys_->apu.tone(2, f * 2.01f, 0.07f * bellAmp_);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    decay();
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) begin();
        else if (pad.pressed(gs::BTN_MODE) && !bot_) sys.quit();
    } else if (mode_ == Mode::Ready || mode_ == Mode::Rise || mode_ == Mode::Fall) {
        playFrames_++;
        if (mode_ == Mode::Ready) {
            if (bot_ && playFrames_ == kRelease) swing();
            else if (!bot_ && (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B))) swing();
        }
        if (mode_ == Mode::Rise && arrive_ >= 0 && playFrames_ >= arrive_) {
            if (arrive_ == kHourFrame) strikeHour();
            else passHour("ring before the hour");
        } else if (mode_ != Mode::Chime && mode_ != Mode::Fail && playFrames_ >= kHourFrame) {
            passHour("hour silent");
        } else if (pad.pressed(gs::BTN_MODE) && !bot_) {
            sys.quit();
        }
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

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 1.f || m.h < 1) return;
    float sc = h / float(m.h);
    gs::Sprite s;
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 400));
    s.w = int16_t(std::clamp(int(std::lround(m.w * sc)), 1, 400));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
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
        vdp.road[y].on = false;
        float t = y / float(gs::SCREEN_H);
        int r = 1 + int(2 * (1.f - t));
        int g = 1 + int(t * 2);
        int b = 3 + int((1.f - t) * 6);
        vdp.lineBackdrop[y] = gs::rgb4(r, g, b);
        vdp.lineFog[y] = 0;
    }

    char clock[16];
    std::snprintf(clock, sizeof(clock), "%d:%02d:%02d", hour(), minute(), second());

    float puckY = kBaseY;
    if (swung_ && clean_ && releaseFrame_ > 0) {
        float u = std::clamp((playFrames_ - releaseFrame_) / float(kFlight), 0.f, 1.f);
        float ease = 1.f - (1.f - u) * (1.f - u);
        puckY = kBaseY + (kBellY - kBaseY) * ease;
    } else if (swung_ && !clean_) {
        float u = std::clamp((playFrames_ - releaseFrame_) / 36.f, 0.f, 1.f);
        float arc = std::sin(u * kPi);
        puckY = kBaseY + (kBellY - kBaseY) * meter() * 0.55f * arc;
    }

    float m = (mode_ == Mode::Title) ? 0.15f : meter();
    float pipY = kRailBot + (kRailTop - kRailBot) * m;
    bool lit = mode_ == Mode::Ready && onNotch();

    spr(art_.hand, 248, 58, mode_ == Mode::Chime ? 20 : 16, PAL_FACE);
    float secAng = (kOpenSec + playFrames_ / 60.f);
    float sweep = std::fmod(secAng, 60.f) / 60.f;
    spr(art_.hand, 248 + std::sin(sweep * 2.f * kPi) * 10.f, 58 - std::cos(sweep * 2.f * kPi) * 10.f, 12, PAL_GOLD);
    spr(art_.pip, 118, pipY, lit || mode_ == Mode::Chime ? 12 : 8, PAL_GOLD);
    spr(art_.pip, 118, kRailTop, 7, PAL_BRASS);
    spr(art_.puck, 168, puckY, 14, PAL_PUCK);
    float bellH = 36.f + bellAmp_ * 6.f;
    spr(art_.bell, 168, 28, bellH, PAL_BRASS);
    float malletY = (mode_ == Mode::Rise || mode_ == Mode::Chime) ? 150.f : 132.f + (1.f - m) * 16.f;
    spr(art_.mallet, 96, malletY, 16, PAL_WOOD, mode_ == Mode::Rise);
    spr(art_.man, 78, 158, 78, PAL_MAN);
    spr(art_.tower, 168, 112, 168, PAL_WOOD);
    spr(art_.face, 248, 58, 52, PAL_FACE);
    spr(art_.moon, 36, 36, 22, PAL_NIGHT);
    spr(art_.ground, 160, 210, 28, PAL_NIGHT);

    if (mode_ == Mode::Title) {
        word(art_.title, 160, 78, PAL_GOLD);
        word(art_.rule, 160, 118, PAL_HUD);
        hudC(24, "A  SWING ON THE NOTCH", PAL_HUD);
        hudC(26, "START", PAL_HUD);
    } else {
        hud(1, 1, clock, PAL_HUD);
        hudC(1, mode_ == Mode::Chime ? "CHIME" : "STRIKER", PAL_HUD);
        if (mode_ == Mode::Ready) hudC(26, lit ? "NOTCH  SWING" : "WAIT FOR THE NOTCH", PAL_HUD);
        else if (mode_ == Mode::Rise) hudC(26, "PUCK RISING", PAL_HUD);
        else if (mode_ == Mode::Fall) hudC(26, "SHORT", PAL_HUD);
        else if (mode_ == Mode::Chime) hudC(25, "THE HOUR CHIMES", PAL_GOLD);
        else hudC(25, reason_, PAL_HUD);
    }
}

}  // namespace strikerchime

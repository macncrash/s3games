#include "game/chime.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace keyschime {
namespace {

const gs::Button kBtn[kLanes] = {gs::BTN_LEFT, gs::BTN_DOWN, gs::BTN_UP, gs::BTN_RIGHT};
const gs::Button kAlt[kLanes] = {gs::BTN_A, gs::BTN_B, gs::BTN_C, gs::BTN_Y};
const int kChart[kPhrase] = {0, 2, 1, 3, 0};
const float kHz[kLanes] = {261.6f, 329.6f, 392.0f, 523.3f};

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

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    mode_ = Mode::Title;
    over_ = won_ = chimed_ = silent_ = frozen_ = false;
    hits_ = spawned_ = 0;
    gap_ = hold_ = playFrames_ = 0;
    clock_ = toneT_ = bellAmp_ = 0;
    reason_ = "hour silent";
    for (int i = 0; i < kLanes; i++) keyLit_[i] = 0;
    if (bot_) begin();
    draw();
}

void Game::begin() {
    over_ = won_ = chimed_ = silent_ = frozen_ = false;
    playFrames_ = 0;
    reason_ = "hour silent";
    arm();
}

void Game::arm() {
    mode_ = Mode::Play;
    hits_ = spawned_ = 0;
    gap_ = 8;
    for (int i = 0; i < kPhrase; i++) notes_[i] = {};
    for (int i = 0; i < kLanes; i++) keyLit_[i] = 0;
    sys_->apu.silence();
}

void Game::tone(float freq, float vol) {
    sys_->apu.tone(0, freq, vol);
    toneT_ = 0.14f;
}

void Game::decay() {
    if (toneT_ > 0) {
        toneT_ -= 1.f / 60.f;
        if (toneT_ <= 0) sys_->apu.tone(0, 0, 0);
    }
    for (int i = 0; i < kLanes; i++) keyLit_[i] = std::max(0.f, keyLit_[i] - 0.07f);
    if (mode_ == Mode::Chime) {
        bellAmp_ = std::min(1.f, bellAmp_ + 0.05f);
        int stroke = (hold_ / 10) % 4;
        float f = midiHz(72 + stroke * 4);
        sys_->apu.tone(1, f, 0.2f * bellAmp_);
        sys_->apu.tone(2, f * 2.f, 0.06f * bellAmp_);
    } else if (bellAmp_ > 0) {
        bellAmp_ = std::max(0.f, bellAmp_ - 0.03f);
        if (bellAmp_ == 0) {
            sys_->apu.tone(1, 0, 0);
            sys_->apu.tone(2, 0, 0);
        }
    }
}

void Game::strike(int lane) {
    if (lane < 0 || lane >= kLanes || mode_ != Mode::Play || silent_) return;
    keyLit_[lane] = 1.f;
    int best = -1;
    float bestD = 14.f;
    for (int i = 0; i < spawned_; i++) {
        Note& n = notes_[i];
        if (n.gone || n.lane != lane) continue;
        float d = std::fabs(n.y - float(kHitY));
        if (d < bestD) {
            bestD = d;
            best = i;
        }
    }
    if (best < 0) return;
    notes_[best].gone = true;
    notes_[best].hit = true;
    hits_++;
    tone(kHz[lane], 0.32f);
}

void Game::miss() {
    if (silent_ || mode_ != Mode::Play) return;
    silent_ = true;
    reason_ = "note missed";
    sys_->apu.noiseBurst(0.22f, 700.f, 0.1f);
    tone(midiHz(40), 0.22f);
}

void Game::strikeHour() {
    if (chimed_ || silent_ || hits_ != kPhrase) {
        passHour();
        return;
    }
    chimed_ = true;
    won_ = true;
    frozen_ = true;
    fh_ = 12;
    fm_ = 0;
    fs_ = 0;
    reason_ = "the hour chimes";
    mode_ = Mode::Chime;
    hold_ = 48;
    bellAmp_ = 0.4f;
    tone(midiHz(84), 0.4f);
}

void Game::passHour() {
    frozen_ = true;
    int h, m, s;
    splitLive(h, m, s);
    fh_ = h;
    fm_ = m;
    fs_ = s;
    won_ = false;
    chimed_ = false;
    reason_ = silent_ ? "hour passed" : "phrase open";
    mode_ = Mode::Fail;
    hold_ = 40;
    tone(midiHz(38), 0.26f);
}

void Game::updatePlay() {
    if (spawned_ < kPhrase && !silent_) {
        if (--gap_ <= 0) {
            Note& n = notes_[spawned_];
            n.lane = kChart[spawned_];
            n.y = -18.f;
            n.gone = false;
            n.hit = false;
            spawned_++;
            gap_ = 26;
        }
    }
    for (int i = 0; i < spawned_; i++) {
        Note& n = notes_[i];
        if (n.gone) continue;
        n.y += 3.5f;
        if (bot_ && !silent_ && std::fabs(n.y - float(kHitY)) <= 7.f) strike(n.lane);
        if (!n.gone && n.y > float(kHitY) + 16.f) {
            n.gone = true;
            miss();
        }
    }
    if (!bot_ && !silent_) {
        const gs::Pad& pad = sys_->pad;
        for (int lane = 0; lane < kLanes; lane++) {
            if (pad.pressed(kBtn[lane]) || pad.pressed(kAlt[lane])) strike(lane);
        }
    }
    if (!silent_ && hits_ == kPhrase && spawned_ == kPhrase) mode_ = Mode::Wait;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    clock_ += 1.f / 60.f;
    decay();
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) begin();
        else if (pad.pressed(gs::BTN_MODE) && !bot_) sys.quit();
    } else if (mode_ == Mode::Play || mode_ == Mode::Wait) {
        playFrames_++;
        if (mode_ == Mode::Play) updatePlay();
        if (playFrames_ >= kHourFrame) strikeHour();
        else if (pad.pressed(gs::BTN_MODE) && !bot_) sys.quit();
    } else if (mode_ == Mode::Chime || mode_ == Mode::Fail) {
        if (!frozen_) playFrames_++;
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
        int sky = y < 120 ? 2 + y / 40 : 1;
        vdp.lineBackdrop[y] = gs::rgb4(1, 1, std::min(6, sky));
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }

    const float cx = 160.f, cy = 58.f;
    spr(art_.face, cx, cy, 70.f, PAL_FACE);
    for (int i = 0; i < 12; i++) {
        float a = (i / 12.f) * 6.28318f - 1.5708f;
        spr(art_.pip, cx + std::cos(a) * 26.f, cy + std::sin(a) * 26.f, i % 3 == 0 ? 6.f : 4.f,
            i == 0 ? PAL_GOLD : PAL_INK);
    }
    int h = hour(), m = minute(), s = second();
    float secA = (s / 60.f) * 6.28318f - 1.5708f;
    float minA = ((m + s / 60.f) / 60.f) * 6.28318f - 1.5708f;
    for (int i = 1; i <= 5; i++) {
        float t = i / 5.f;
        spr(art_.pip, cx + std::cos(minA) * 18.f * t, cy + std::sin(minA) * 18.f * t, 4.f, PAL_INK);
    }
    for (int i = 1; i <= 6; i++) {
        float t = i / 6.f;
        int pal = chimed_ ? PAL_GOLD : PAL_DEAD;
        if (mode_ == Mode::Play || mode_ == Mode::Wait || mode_ == Mode::Title) pal = PAL_GOLD;
        spr(art_.pip, cx + std::cos(secA) * 24.f * t, cy + std::sin(secA) * 24.f * t, 3.f, pal);
    }
    (void)h;

    float sway = 0;
    if (mode_ == Mode::Chime || (mode_ == Mode::Fail && won_)) sway = std::sin(clock_ * 10.f) * (8.f + 10.f * bellAmp_);
    else if (chimed_) sway = std::sin(clock_ * 8.f) * 6.f;
    spr(art_.bell, 160.f + sway, 118.f, chimed_ ? 36.f : 26.f, PAL_BELL);

    for (int i = 0; i < kLanes; i++) {
        float x = float(kLaneX[i]);
        int pal = keyLit_[i] > 0.15f ? PAL_GOLD : PAL_IVORY;
        spr(art_.key, x, 200.f, 52.f, pal);
        spr(art_.glow, x, float(kHitY), 7.f, keyLit_[i] > 0.15f ? PAL_GOLD : PAL_NIGHT);
    }
    spr(art_.black, 88.f, 178.f, 28.f, PAL_EBONY);
    spr(art_.black, 160.f, 178.f, 28.f, PAL_EBONY);
    spr(art_.black, 232.f, 178.f, 28.f, PAL_EBONY);

    if (mode_ == Mode::Play || mode_ == Mode::Wait || mode_ == Mode::Chime) {
        for (int i = 0; i < spawned_; i++) {
            const Note& n = notes_[i];
            if (n.gone) continue;
            spr(art_.note, float(kLaneX[n.lane]), n.y, 16.f, i == kPhrase - 1 ? PAL_GOLD : PAL_NOTE);
        }
    }

    char line[48];
    std::snprintf(line, sizeof(line), "%d:%02d:%02d", hour(), minute(), second());
    hud(16, 1, line, chimed_ ? PAL_GOLD : PAL_INK);

    if (mode_ == Mode::Title) {
        word(art_.title, 160, 96, PAL_GOLD);
        word(art_.rule, 160, 128, PAL_INK);
        hudC(20, "PLAY THE KEYS", PAL_INK);
        hudC(22, "THE HOUR HAS TO CHIME", PAL_GOLD);
        hudC(25, "START", PAL_INK);
    } else if (mode_ == Mode::Wait) {
        hudC(15, "HOLD", PAL_INK);
    } else if (mode_ == Mode::Chime) {
        hudC(15, "THE HOUR CHIMES", PAL_GOLD);
    } else if (mode_ == Mode::Fail) {
        hudC(15, "HOUR PASSED", PAL_DEAD);
    } else {
        hudC(15, "KEYS", PAL_INK);
        std::snprintf(line, sizeof(line), "%d/%d", hits_, kPhrase);
        hud(1, 3, line, PAL_INK);
    }
}

}  // namespace keyschime

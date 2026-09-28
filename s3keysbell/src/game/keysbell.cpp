#include "game/keysbell.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace keysbell {
namespace {

const gs::Button kBtn[kLanes] = {gs::BTN_LEFT, gs::BTN_DOWN, gs::BTN_UP, gs::BTN_RIGHT};
const gs::Button kAlt[kLanes] = {gs::BTN_A, gs::BTN_B, gs::BTN_C, gs::BTN_Y};
const int kChart[kPhrase] = {0, 1, 2, 3, 2, 0, 3, 1};

float midiHz(int midi) { return 440.f * std::pow(2.f, (midi - 69) / 12.f); }

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    mode_ = Mode::Title;
    over_ = won_ = rules_ = rung_ = false;
    chartOk_ = true;
    tryNo_ = 0;
    dead_ = 0;
    spawned_ = hits_ = 0;
    gap_ = hold_ = 0;
    clock_ = toneT_ = 0;
    bellAmp_ = 0.2f;
    reason_ = "bell silent";
    for (int i = 0; i < kLanes; i++) keyLit_[i] = 0;
    if (bot_) begin();
}

void Game::begin() {
    tryNo_ = 1;
    dead_ = 0;
    over_ = won_ = rules_ = rung_ = false;
    chartOk_ = true;
    reason_ = "bell silent";
    armPhrase();
}

void Game::armPhrase() {
    mode_ = Mode::Play;
    spawned_ = hits_ = 0;
    gap_ = 8;
    for (int i = 0; i < kPhrase; i++) notes_[i] = {};
    for (int i = 0; i < kLanes; i++) keyLit_[i] = 0;
    sys_->apu.silence();
}

void Game::tone(float freq, float vol) {
    sys_->apu.tone(0, freq, vol);
    toneT_ = 0.12f;
}

void Game::decay() {
    if (toneT_ > 0) {
        toneT_ -= 1.f / 60.f;
        if (toneT_ <= 0) sys_->apu.tone(0, 0, 0);
    }
    for (int i = 0; i < kLanes; i++) keyLit_[i] = std::max(0.f, keyLit_[i] - 0.08f);
    if (mode_ == Mode::Ring || mode_ == Mode::Leave) {
        bellAmp_ = std::min(1.f, bellAmp_ + 0.04f);
        float wob = 0.55f + 0.45f * std::sin(clock_ * 28.f);
        sys_->apu.tone(1, 392.f * wob, 0.22f * bellAmp_);
    } else if (bellAmp_ > 0.2f) {
        bellAmp_ -= 0.02f;
        sys_->apu.tone(1, 0, 0);
    }
}

void Game::strike(int lane) {
    if (lane < 0 || lane >= kLanes || mode_ != Mode::Play) return;
    keyLit_[lane] = 1.f;
    int best = -1;
    float bestD = 16.f;
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
    tone(midiHz(60 + lane * 3 + (best % 2) * 7), 0.34f);
}

void Game::killTry() {
    if (mode_ != Mode::Play || rung_) return;
    dead_++;
    sys_->apu.noiseBurst(0.25f, 800.f, 0.12f);
    if (dead_ >= kTries) {
        mode_ = Mode::Dead;
        hold_ = 50;
        won_ = false;
        rules_ = false;
        reason_ = "third try died";
        tone(midiHz(42), 0.28f);
    } else {
        mode_ = Mode::Gap;
        hold_ = 36;
        reason_ = "try died";
    }
}

void Game::ring() {
    if (rung_ || mode_ != Mode::Play) return;
    if (hits_ != kPhrase || dead_ >= kTries || tryNo_ < 1 || tryNo_ > kTries) {
        chartOk_ = false;
        killTry();
        return;
    }
    rung_ = true;
    won_ = true;
    rules_ = chartOk_ && dead_ < kTries;
    reason_ = "bell";
    mode_ = Mode::Ring;
    hold_ = 40;
    bellAmp_ = 0.35f;
    tone(midiHz(79), 0.4f);
}

void Game::updatePlay() {
    if (spawned_ < kPhrase) {
        if (--gap_ <= 0) {
            Note& n = notes_[spawned_];
            n.lane = kChart[spawned_];
            n.y = -16.f;
            n.gone = false;
            n.hit = false;
            spawned_++;
            gap_ = 16;
        }
    }
    bool open = spawned_ < kPhrase;
    for (int i = 0; i < spawned_; i++) {
        Note& n = notes_[i];
        if (n.gone) continue;
        open = true;
        n.y += 3.4f;
        if (bot_ && std::fabs(n.y - float(kHitY)) <= 7.f) strike(n.lane);
        if (!n.gone && n.y > float(kHitY) + 14.f) {
            n.gone = true;
            killTry();
            return;
        }
    }
    if (!bot_) {
        const gs::Pad& pad = sys_->pad;
        for (int lane = 0; lane < kLanes; lane++) {
            if (pad.pressed(kBtn[lane]) || pad.pressed(kAlt[lane])) strike(lane);
        }
    }
    if (!open && mode_ == Mode::Play) {
        if (hits_ == kPhrase && spawned_ == kPhrase) ring();
        else {
            chartOk_ = false;
            killTry();
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    clock_ += 1.f / 60.f;
    decay();
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) begin();
        else if (pad.pressed(gs::BTN_MODE) && !bot_) sys.quit();
    } else if (mode_ == Mode::Play) {
        updatePlay();
        if (pad.pressed(gs::BTN_MODE) && !bot_) sys.quit();
    } else if (mode_ == Mode::Gap) {
        if (--hold_ <= 0) {
            tryNo_++;
            if (tryNo_ < 1 || tryNo_ > kTries) chartOk_ = false;
            armPhrase();
        }
    } else if (mode_ == Mode::Ring) {
        if (--hold_ <= 0) {
            mode_ = Mode::Leave;
            hold_ = 36;
        }
    } else {
        if (--hold_ <= 0) {
            over_ = true;
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
        int g = 1 + y / 48;
        vdp.lineBackdrop[y] = gs::rgb4(1, 1, std::min(5, g));
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }
    float sway = 0;
    if (rung_) sway = std::sin(clock_ * 9.f) * (10.f + 8.f * bellAmp_);
    spr(art_.bell, 160.f + sway, 36.f, rung_ ? 40.f : 28.f, PAL_BELL);

    for (int i = 0; i < kLanes; i++) {
        float x = float(kLaneX[i]);
        int pal = keyLit_[i] > 0.2f ? PAL_GOLD : PAL_IVORY;
        spr(art_.key, x, 198.f, 48.f, pal);
        spr(art_.glow, x, float(kHitY), 8.f, keyLit_[i] > 0.2f ? PAL_GOLD : PAL_WOOD);
    }
    spr(art_.black, 96.f, 176.f, 26.f, PAL_EBONY);
    spr(art_.black, 160.f, 176.f, 26.f, PAL_EBONY);
    spr(art_.black, 224.f, 176.f, 26.f, PAL_EBONY);

    if (mode_ == Mode::Play || mode_ == Mode::Ring || mode_ == Mode::Leave) {
        for (int i = 0; i < spawned_; i++) {
            const Note& n = notes_[i];
            if (n.gone) continue;
            spr(art_.note, float(kLaneX[n.lane]), n.y, 16.f, i % 2 ? PAL_NOTE : PAL_GOLD);
        }
    }
    for (int i = 0; i < kTries; i++) {
        int pal = PAL_WOOD;
        if (i < dead_) pal = PAL_DEAD;
        else if (rung_ && i == tryNo_ - 1) pal = PAL_GOLD;
        else if (mode_ == Mode::Play && i == tryNo_ - 1) pal = PAL_CLAP;
        spr(art_.mark, 24.f + i * 16.f, 16.f, 10.f, pal);
    }

    char line[48];
    int show = tryNo_ < 1 ? 1 : tryNo_;
    std::snprintf(line, sizeof(line), "TRY %d", show);
    hud(1, 3, line, dead_ == 2 && !rung_ ? PAL_DEAD : PAL_INK);
    if (mode_ == Mode::Title) {
        word(art_.title, 160, 58, PAL_GOLD);
        word(art_.rule, 160, 96, PAL_INK);
        hudC(16, "CLEAN PHRASE RINGS THE BELL", PAL_INK);
        hudC(18, "A MISS KILLS THE TRY", PAL_INK);
        hudC(22, "START", PAL_GOLD);
    } else if (mode_ == Mode::Ring || mode_ == Mode::Leave) {
        hudC(8, "THE BELL", PAL_GOLD);
        hudC(10, "LEAVE", PAL_INK);
    } else if (mode_ == Mode::Dead) {
        hudC(8, "THIRD TRY DIED", PAL_DEAD);
        hudC(10, "BELL SILENT", PAL_INK);
    } else if (mode_ == Mode::Gap) {
        hudC(8, "TRY DIED", PAL_DEAD);
    } else {
        hudC(1, "KEYS", PAL_INK);
    }
}

}  // namespace keysbell

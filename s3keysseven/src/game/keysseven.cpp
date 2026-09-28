#include "game/keysseven.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace keysseven {
namespace {

const gs::Button kBtn[kLanes] = {gs::BTN_LEFT, gs::BTN_DOWN, gs::BTN_UP, gs::BTN_RIGHT};
const gs::Button kAlt[kLanes] = {gs::BTN_A, gs::BTN_B, gs::BTN_C, gs::BTN_Y};

float midiHz(int midi) {
    return 440.f * std::pow(2.f, (midi - 69) / 12.f);
}

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    rules_ = false;
    you_ = them_ = bars_ = clean_ = spawned_ = hits_ = 0;
    gap_ = 0;
    hold_ = 0;
    clock_ = 0;
    toneT_ = 0;
    chartOk_ = true;
    for (int i = 0; i < kLanes; i++) keyLit_[i] = 0;
    if (bot_) begin();
}

void Game::begin() {
    mode_ = Mode::Play;
    you_ = them_ = bars_ = clean_ = spawned_ = hits_ = 0;
    gap_ = 10;
    hold_ = 0;
    over_ = false;
    won_ = false;
    rules_ = false;
    for (int i = 0; i < kNotes; i++) notes_[i] = {};
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
}

void Game::strike(int lane) {
    if (lane < 0 || lane >= kLanes) return;
    keyLit_[lane] = 1.f;
    int best = -1;
    float bestD = 18.f;
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
    Note& n = notes_[best];
    n.gone = true;
    n.hit = true;
    hits_++;
    tone(midiHz(60 + n.lane * 2 + (n.bar % 3) * 5), 0.35f);
}

void Game::settle(int bar) {
    if (bar != bars_ || bar < 0 || bar >= kRace) {
        chartOk_ = false;
        return;
    }
    int got = 0;
    for (int i = 0; i < kBar; i++) {
        const Note& n = notes_[bar * kBar + i];
        if (!n.gone) {
            chartOk_ = false;
            return;
        }
        if (n.hit) got++;
    }
    if (got == kBar) {
        you_++;
        clean_++;
    } else {
        them_++;
    }
    bars_++;
    if (you_ >= kRace && them_ < kRace) win();
    else if (them_ >= kRace && you_ < kRace) lose();
    else if (you_ >= kRace && them_ >= kRace) {
        chartOk_ = false;
        lose();
    }
}

void Game::win() {
    mode_ = Mode::Win;
    hold_ = 48;
    won_ = true;
    rules_ = chartOk_ && you_ == kRace && them_ < kRace && clean_ == kRace && bars_ == kRace && hits_ == kNotes;
    tone(midiHz(76), 0.4f);
}

void Game::lose() {
    mode_ = Mode::Lose;
    hold_ = 48;
    won_ = false;
    rules_ = false;
    tone(midiHz(48), 0.3f);
}

void Game::updatePlay() {
    if (spawned_ < kNotes) {
        if (--gap_ <= 0) {
            Note& n = notes_[spawned_];
            n.lane = spawned_ % kLanes;
            n.bar = spawned_ / kBar;
            n.y = -18.f;
            n.gone = false;
            n.hit = false;
            spawned_++;
            gap_ = 22;
        }
    }
    bool pending = spawned_ < kNotes;
    for (int i = 0; i < spawned_; i++) {
        Note& n = notes_[i];
        if (n.gone) continue;
        pending = true;
        n.y += 3.05f;
        if (bot_ && !n.gone && std::fabs(n.y - float(kHitY)) <= 8.f) strike(n.lane);
        if (!n.gone && n.y > float(kHitY) + 16.f) n.gone = true;
    }
    if (!bot_) {
        const gs::Pad& pad = sys_->pad;
        for (int lane = 0; lane < kLanes; lane++) {
            if (pad.pressed(kBtn[lane]) || pad.pressed(kAlt[lane])) strike(lane);
        }
    }
    if (mode_ == Mode::Play && bars_ < kRace && spawned_ >= (bars_ + 1) * kBar) {
        bool open = false;
        for (int i = 0; i < kBar; i++) {
            if (!notes_[bars_ * kBar + i].gone) open = true;
        }
        if (!open) settle(bars_);
    }
    if (!pending && mode_ == Mode::Play && bars_ < kRace) {
        chartOk_ = false;
        lose();
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
        int g = 1 + y / 40;
        vdp.lineBackdrop[y] = gs::rgb4(1, 1, std::min(4, g));
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }
    for (int i = 0; i < kLanes; i++) {
        float x = float(kLaneX[i]);
        int pal = keyLit_[i] > 0.2f ? PAL_GOLD : PAL_IVORY;
        spr(art_.key, x, 196, 52, pal);
        if (i < kLanes - 1) spr(art_.black, (x + kLaneX[i + 1]) * 0.5f, 176, 28, PAL_EBONY);
        spr(art_.glow, x, float(kHitY), 8, keyLit_[i] > 0.2f ? PAL_GOLD : PAL_WOOD);
    }
    if (mode_ == Mode::Play || mode_ == Mode::Win || mode_ == Mode::Lose) {
        for (int i = 0; i < spawned_; i++) {
            const Note& n = notes_[i];
            if (n.gone) continue;
            spr(art_.note, float(kLaneX[n.lane]), n.y, 18, n.bar % 2 ? PAL_NOTE : PAL_GOLD);
        }
    }
    for (int i = 0; i < kRace; i++) {
        spr(art_.lamp, 18.f, 28.f + i * 16.f, 12, i < you_ ? PAL_LAMP : PAL_WOOD);
        spr(art_.lamp, 302.f, 28.f + i * 16.f, 12, i < them_ ? PAL_RIVAL : PAL_WOOD);
    }
    char line[40];
    std::snprintf(line, sizeof(line), "YOU %d", you_);
    hud(1, 1, line, PAL_INK);
    std::snprintf(line, sizeof(line), "THEM %d", them_);
    hud(31, 1, line, PAL_INK);
    if (mode_ == Mode::Title) {
        word(art_.title, 160, 48, PAL_GOLD);
        word(art_.leave, 160, 92, PAL_INK);
        hudC(16, "FOUR NOTES A BAR", PAL_INK);
        hudC(18, "CLEAN BAR IS YOURS", PAL_INK);
        hudC(22, "START", PAL_GOLD);
    } else if (mode_ == Mode::Win) {
        word(art_.leave, 160, 64, PAL_GOLD);
        hudC(12, "LEAVE", PAL_INK);
    } else if (mode_ == Mode::Lose) {
        hudC(10, "THEY GOT THERE", PAL_MISS);
    } else {
        hudC(1, "KEYS", PAL_INK);
    }
}

}  // namespace keysseven

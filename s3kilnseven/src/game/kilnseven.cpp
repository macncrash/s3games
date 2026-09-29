#include "game/kilnseven.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace kilnseven {
namespace {

constexpr int kWin = 7;
constexpr int kBandLo = 14;
constexpr int kBandHi = 19;
constexpr int kMissAt = 36;
constexpr int kFireAt = 16;

}  // namespace

const char* Game::phase() const {
    switch (mode_) {
    case Mode::Title: return "title";
    case Mode::Heat: return "heat";
    case Mode::Glow: return "glow";
    case Mode::Rival: return "rival";
    case Mode::Win: return "win";
    case Mode::Lose: return "lose";
    case Mode::Pause: return "pause";
    }
    return "?";
}

bool Game::rules() const {
    if (!won_ || you_ < kWin || them_ >= kWin || !shortSix_ || logN_ < 2) return false;
    int y = 0, t = 0;
    bool saw = false;
    for (int i = 0; i < logN_; i++) {
        if (log_[i].pts != 1) return false;
        if (log_[i].yours) {
            if (y >= kWin) return false;
            y += 1;
            if (y == 6) saw = true;
            if (y >= kWin && t >= kWin) return false;
        } else {
            if (t >= kWin) return false;
            t += 1;
            if (t >= kWin && y < kWin) return false;
        }
    }
    return saw && y == you_ && t == them_ && y >= kWin && t < kWin;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    sys.vdp.A.clear();
    sys.vdp.B.clear();
    sys.vdp.HUD.clear();
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    buildArt(sys.vdp, art_);
    toTitle();
}

void Game::toTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    why_ = "";
    wait_ = 0;
    wind_ = 0;
    anim_ = 0;
}

void Game::resetMatch() {
    you_ = them_ = faults_ = logN_ = wind_ = anim_ = wait_ = 0;
    shortSix_ = won_ = over_ = false;
    why_ = "";
    mode_ = Mode::Heat;
    sys_->apu.silence();
}

void Game::blip(float freq) { sys_->apu.keyOn(0, freq, 0.22f); }

void Game::winShelf() {
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    why_ = "first to seven";
    sys_->apu.keyOn(0, 440.f, 0.28f);
    sys_->apu.keyOn(1, 554.f, 0.2f);
    sys_->apu.keyOn(2, 659.f, 0.16f);
}

void Game::loseShelf(const char* why) {
    mode_ = Mode::Lose;
    won_ = false;
    over_ = true;
    why_ = why;
    sys_->apu.noiseBurst(0.28f, 70.f, 0.22f);
}

void Game::scoreYou() {
    if (logN_ < 16) {
        log_[logN_].yours = true;
        log_[logN_].pts = 1;
        logN_++;
    }
    you_++;
    blip(620.f);
    sys_->apu.noiseBurst(0.12f, 180.f, 0.06f);
    if (you_ == 6) shortSix_ = true;
    anim_ = 0;
    if (you_ >= kWin) {
        winShelf();
        return;
    }
    mode_ = Mode::Glow;
}

void Game::crack() {
    faults_++;
    blip(140.f);
    sys_->apu.noiseBurst(0.24f, 60.f, 0.16f);
    anim_ = 0;
    if (faults_ >= 4) {
        loseShelf("the clay kept cracking");
        return;
    }
    mode_ = Mode::Glow;
}

void Game::scoreThem() {
    if (logN_ < 16) {
        log_[logN_].yours = false;
        log_[logN_].pts = 1;
        logN_++;
    }
    them_++;
    blip(330.f);
    if (them_ >= kWin && you_ < kWin) {
        loseShelf("the other bench reached seven");
        return;
    }
    wind_ = 0;
    mode_ = Mode::Heat;
}

void Game::botAct(bool& fire, bool& start) {
    if (wait_ > 0) {
        wait_--;
        return;
    }
    if (mode_ == Mode::Title) {
        if (sys_ && sys_->frame >= 24) start = true;
        return;
    }
    if (mode_ == Mode::Heat && wind_ == kFireAt) fire = true;
}

void Game::logic(bool fire, bool start) {
    if (mode_ == Mode::Title) {
        if (start) resetMatch();
        return;
    }
    if (mode_ == Mode::Win || mode_ == Mode::Lose) {
        if (start && !bot_) toTitle();
        return;
    }
    if (mode_ == Mode::Pause) {
        if (start) mode_ = held_;
        return;
    }
    if (start && !bot_ && mode_ == Mode::Heat) {
        held_ = mode_;
        mode_ = Mode::Pause;
        return;
    }
    if (mode_ == Mode::Heat) {
        wind_++;
        const bool in = wind_ >= kBandLo && wind_ <= kBandHi;
        if (fire) {
            if (in) scoreYou();
            else crack();
        } else if (wind_ >= kMissAt) {
            crack();
        }
        return;
    }
    if (mode_ == Mode::Glow) {
        if (++anim_ >= 16) {
            anim_ = 0;
            mode_ = Mode::Rival;
        }
        return;
    }
    if (mode_ == Mode::Rival) {
        if (++anim_ >= 18) scoreThem();
    }
}

void Game::spr(const gs::Image& img, float cx, float cy, int pal) {
    if (img.w == 0) return;
    gs::Sprite s;
    s.img = img;
    s.w = img.w;
    s.h = img.h;
    s.x = int16_t(std::lround(cx - img.w * 0.5f));
    s.y = int16_t(std::lround(cy - img.h * 0.5f));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    uint16_t top = gs::rgb4(2, 1, 3);
    uint16_t mid = gs::rgb4(9, 3, 1);
    uint16_t bot = gs::rgb4(2, 2, 2);
    if (mode_ == Mode::Win) mid = gs::rgb4(12, 8, 2);
    if (mode_ == Mode::Lose) mid = gs::rgb4(5, 1, 1);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H - 1);
        auto mix = [](uint16_t a, uint16_t b, float t) {
            int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
            int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
            auto L = [&](int p, int q) { return int(p + (q - p) * t + 0.5f); };
            return gs::rgb4(L(ar, br), L(ag, bg), L(ab, bb));
        };
        v.lineBackdrop[y] = u < 0.45f ? mix(top, mid, u / 0.45f) : mix(mid, bot, (u - 0.45f) / 0.55f);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    sky();

    const float kilnX = 118.f;
    const float kilnY = 118.f;
    spr(art_.kiln, kilnX, kilnY, PAL_BRICK);
    spr(art_.kilnFar, 274.f, 132.f, PAL_ASH);

    const bool hot = mode_ == Mode::Heat || mode_ == Mode::Glow || mode_ == Mode::Rival;
    if (hot || mode_ == Mode::Title || mode_ == Mode::Win) {
        float fy = 150.f + 3.f * std::sin((sys_ ? sys_->frame : 0) * 0.2f);
        spr(art_.flame, kilnX, fy, PAL_FIRE);
        if (mode_ == Mode::Rival || mode_ == Mode::Win) spr(art_.flame, 274.f, 150.f, PAL_FIRE);
    }

    for (int i = 0; i < kWin; i++) {
        spr(art_.pot, 28.f + i * 22.f, 196.f, i < you_ ? PAL_CLAY : PAL_ASH);
        spr(i < them_ ? art_.pot : art_.pot, 176.f + (i % 7) * 18.f, 196.f, i < them_ ? PAL_RIVAL : PAL_ASH);
    }

    spr(art_.bar, 160.f, 46.f, mode_ == Mode::Heat ? PAL_GOOD : PAL_ASH);
    float tick = 100.f;
    if (mode_ == Mode::Heat) {
        float u = std::min(wind_, kMissAt) / float(kMissAt);
        tick = 100.f + u * 120.f;
    } else if (mode_ == Mode::Title) {
        tick = 148.f + 18.f * std::sin((sys_ ? sys_->frame : 0) * 0.1f);
    }
    spr(art_.cone, tick, 34.f, PAL_INK);

    char line[40];
    std::snprintf(line, sizeof(line), "YOU %d", you_);
    hud(1, 1, line, PAL_HUD);
    std::snprintf(line, sizeof(line), "THEM %d", them_);
    hud(30, 1, line, PAL_HUD);
    hudC(3, "FIRST TO SEVEN", PAL_HUD);

    if (mode_ == Mode::Title) {
        hudC(12, "S3 KILN SEVEN", PAL_HUD);
        hudC(14, "FIRE IN THE GREEN BAND", PAL_INK);
        hudC(16, "A SHORT KILN", PAL_HUD);
        hudC(22, "START", PAL_HUD);
    } else if (mode_ == Mode::Heat) {
        hudC(24, "A WHEN THE CONE IS GREEN", PAL_HUD);
    } else if (mode_ == Mode::Glow) {
        hudC(24, you_ > 0 && anim_ < 8 ? "SOUND POT" : "SHELF", PAL_HUD);
    } else if (mode_ == Mode::Rival) {
        hudC(24, "OTHER BENCH", PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        hudC(14, "PAUSED", PAL_HUD);
    } else if (mode_ == Mode::Win) {
        hudC(12, "FIRST TO SEVEN", PAL_HUD);
        hudC(14, "YOUR SHELF", PAL_GOOD);
    } else if (mode_ == Mode::Lose) {
        hudC(12, why_, PAL_BAD);
        hudC(14, "SHORT", PAL_HUD);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    bool fire = false, start = false;
    if (bot_) {
        botAct(fire, start);
    } else {
        const gs::Pad& p = sys.pad;
        fire = p.pressed(gs::BTN_A) || p.pressed(gs::BTN_Z);
        start = p.pressed(gs::BTN_START);
    }
    logic(fire, start);
    draw();
}

}  // namespace kilnseven

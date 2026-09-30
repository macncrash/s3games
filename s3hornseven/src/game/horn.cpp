#include "game/horn.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace hornseven {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr int kBar = 48;
constexpr int kSweet0 = 18;
constexpr int kSweet1 = 28;

int clampi(int v, int a, int b) { return std::max(a, std::min(b, v)); }

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.setFogColor(gs::rgb4(2, 1, 2));
    toTitle();
}

void Game::toTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    reason_ = "";
    you_ = 0;
    rival_ = 0;
    phase_ = 0;
    barHit_ = false;
    showT_ = 0;
    titleT_ = 0;
    walk_ = 78.f;
    puff_ = 0.f;
    toneT_ = 0.f;
}

void Game::beginPlay() {
    mode_ = Mode::Play;
    you_ = 0;
    rival_ = 0;
    phase_ = 0;
    barHit_ = false;
    showT_ = 0;
    walk_ = 78.f;
    reason_ = "";
    won_ = false;
    over_ = false;
}

void Game::blow(bool sweet) {
    if (mode_ != Mode::Play || barHit_) return;
    barHit_ = true;
    puff_ = 0.45f;
    noteN_ = (noteN_ + 1) % 5;
    toneF_ = sweet ? (196.f + float(noteN_) * 32.f) : 110.f;
    toneT_ = sweet ? 0.28f : 0.12f;
    if (sweet) {
        you_++;
        if (you_ >= kGoal && you_ > rival_) beginLead();
    } else {
        rival_++;
        if (rival_ >= kGoal && rival_ >= you_) beginLost("LATE");
    }
}

void Game::endBar() {
    if (mode_ != Mode::Play) return;
    if (!barHit_) {
        rival_++;
        toneF_ = 146.f;
        toneT_ = 0.16f;
        if (rival_ >= kGoal && rival_ >= you_) beginLost("BEHIND");
    }
    barHit_ = false;
    phase_ = 0;
}

void Game::beginLead() {
    mode_ = Mode::Lead;
    showT_ = 0;
    reason_ = "";
}

void Game::beginLeave() {
    if (mode_ != Mode::Lead) return;
    mode_ = Mode::Leave;
    showT_ = 0;
    reason_ = "LEAVE";
    toneF_ = 392.f;
    toneT_ = 0.45f;
}

void Game::beginLost(const char* why) {
    if (mode_ == Mode::Lost || mode_ == Mode::Over || mode_ == Mode::Leave) return;
    mode_ = Mode::Lost;
    showT_ = 0;
    reason_ = why;
    won_ = false;
}

void Game::tickAudio(float dt) {
    if (!sys_) return;
    if (toneT_ > 0.f) {
        toneT_ -= dt;
        float v = std::max(0.f, toneT_) * 0.22f;
        sys_->apu.tone(0, toneF_, v);
        sys_->apu.tone(1, toneF_ * 1.5f, v * 0.35f);
    } else {
        sys_->apu.tone(0, 0.f, 0.f);
        sys_->apu.tone(1, 0.f, 0.f);
    }
    if (puff_ > 0.f) puff_ -= dt;
}

void Game::bot() {
    if (!bot_) return;
    if (mode_ == Mode::Title && titleT_ > 20) beginPlay();
    else if (mode_ == Mode::Play && phase_ == 22) blow(true);
    else if (mode_ == Mode::Lead && showT_ > 18) beginLeave();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& p = sys.pad;
    if (p.pressed(gs::BTN_START) && mode_ != Mode::Title && mode_ != Mode::Over && mode_ != Mode::Leave) {
        if (mode_ == Mode::Pause) mode_ = held_;
        else if (mode_ == Mode::Play || mode_ == Mode::Lead || mode_ == Mode::Lost) {
            held_ = mode_;
            mode_ = Mode::Pause;
        }
    }
    if (mode_ != Mode::Pause) {
        if (mode_ == Mode::Title) {
            titleT_++;
            if (!bot_ && (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A))) beginPlay();
        } else if (mode_ == Mode::Play) {
            bool sweet = phase_ >= kSweet0 && phase_ <= kSweet1;
            if (!bot_ && p.pressed(gs::BTN_A)) blow(sweet);
            if (!bot_ && (p.pressed(gs::BTN_B) || p.pressed(gs::BTN_C))) beginLost("EARLY");
            if (mode_ == Mode::Play) {
                phase_++;
                if (phase_ >= kBar) endBar();
            }
        } else if (mode_ == Mode::Lead) {
            showT_++;
            if (!bot_ && (p.pressed(gs::BTN_B) || p.pressed(gs::BTN_C) || p.pressed(gs::BTN_A))) beginLeave();
        } else if (mode_ == Mode::Leave) {
            showT_++;
            step_++;
            walk_ += 2.4f;
            if (showT_ > 70 || walk_ > 340.f) {
                won_ = you_ >= kGoal && you_ > rival_;
                over_ = true;
                mode_ = Mode::Over;
            }
        } else if (mode_ == Mode::Lost) {
            showT_++;
            if (showT_ > 50) {
                won_ = false;
                over_ = true;
                mode_ = Mode::Over;
            }
        }
        bot();
    }
    tickAudio(kDt);
    if (mode_ == Mode::Play || mode_ == Mode::Lead) sys.setLight(180, 120, 40);
    else if (mode_ == Mode::Leave || (mode_ == Mode::Over && won_)) sys.setLight(220, 170, 60);
    else if (mode_ == Mode::Lost || (mode_ == Mode::Over && !won_ && reason_[0])) sys.setLight(80, 30, 30);
    else sys.setLight(40, 24, 36);
    draw();
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet) {
    if (!sys_ || h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    if (s.w < 1 || s.h < 1) return;
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!sys_ || !s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        int tile = art_.font[c - 32];
        if (!tile) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(tile, pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = s ? int(std::strlen(s)) : 0;
    hud(20 - n / 2, row, s, pal);
}

void Game::lamps(float x0, int n, int pal) {
    n = clampi(n, 0, kGoal);
    for (int i = 0; i < kGoal; i++) {
        float x = x0 + float(i) * 14.f;
        spr(art_.lamp, x, 78.f, 12.f, i < n ? PAL_LAMP : pal);
    }
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    bool win = mode_ == Mode::Leave || mode_ == Mode::Lead || (mode_ == Mode::Over && won_);
    bool dead = mode_ == Mode::Lost || (mode_ == Mode::Over && !won_ && reason_[0]);
    uint16_t top = win ? gs::rgb4(6, 2, 4) : dead ? gs::rgb4(3, 1, 2) : gs::rgb4(1, 1, 3);
    uint16_t mid = win ? gs::rgb4(10, 4, 3) : dead ? gs::rgb4(5, 2, 2) : gs::rgb4(4, 2, 5);
    uint16_t floor = gs::rgb4(5, 3, 1);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.road[y].on = false;
        v.lineFog[y] = 0;
        float u = y / float(gs::SCREEN_H - 1);
        uint16_t c = u < 0.55f ? top : (u < 0.72f ? mid : floor);
        if (y > 168 && ((y / 4) & 1)) c = gs::rgb4(3, 2, 1);
        v.lineBackdrop[y] = c;
    }
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    spr(art_.curtain, 18.f, 90.f, 150.f, PAL_YOU);
    spr(art_.curtain, 302.f, 90.f, 150.f, PAL_THEM, true);
    spr(art_.stand, 160.f, 176.f, 28.f, PAL_WOOD);

    int fr = (step_ / 8) & 1;
    bool leaving = mode_ == Mode::Leave || (mode_ == Mode::Over && won_);
    float px = leaving ? walk_ : 96.f;
    spr(art_.player[leaving ? fr : 0], px, 168.f, 78.f, PAL_YOU, false, true);
    spr(art_.rival[(mode_ == Mode::Play && (phase_ / 12) & 1) ? 1 : 0], 230.f, 168.f, 64.f, PAL_THEM, true, true);

    if (puff_ > 0.f) {
        float ny = 118.f - (0.45f - puff_) * 40.f;
        spr(art_.note, px + 28.f, ny, 18.f + puff_ * 10.f, PAL_NOTE);
    }

    lamps(36.f, you_, PAL_INK);
    lamps(196.f, rival_, PAL_INK);

    bool sweet = mode_ == Mode::Play && phase_ >= kSweet0 && phase_ <= kSweet1;
    if (mode_ == Mode::Play) {
        int marks = 9;
        for (int i = 0; i < marks; i++) {
            char bit[2] = {sweet && i >= 3 && i <= 5 ? '|' : '.', 0};
            hud(15 + i, 16, bit, sweet ? PAL_GOLD : PAL_INK);
        }
        int cursor = clampi(phase_ * 9 / kBar, 0, 8);
        hud(15 + cursor, 15, "V", sweet ? PAL_GOLD : PAL_ALERT);
    }

    char line[40];
    std::snprintf(line, sizeof(line), "YOU %d", you_);
    hud(2, 2, line, PAL_GOLD);
    std::snprintf(line, sizeof(line), "RIVAL %d", rival_);
    hud(28, 2, line, PAL_INK);

    if (mode_ == Mode::Title || (mode_ == Mode::Pause && held_ == Mode::Title)) {
        hudC(6, "S3 HORNSEVEN", PAL_GOLD);
        hudC(9, "PLAY THE HORN", PAL_INK);
        hudC(11, "FIRST TO SEVEN", PAL_GOLD);
        hudC(18, "A ON THE BEAT", PAL_INK);
        hudC(20, "START", PAL_GOLD);
    } else if (mode_ == Mode::Play) {
        hudC(4, sweet ? "BLOW" : "WAIT", sweet ? PAL_GOLD : PAL_INK);
    } else if (mode_ == Mode::Lead) {
        hudC(4, "FIRST TO SEVEN", PAL_GOLD);
        hudC(6, "B LEAVE", PAL_INK);
    } else if (mode_ == Mode::Leave || (mode_ == Mode::Over && won_)) {
        hudC(4, "FIRST TO SEVEN", PAL_GOLD);
        hudC(6, "LEAVE", PAL_INK);
    } else if (mode_ == Mode::Lost || (mode_ == Mode::Over && !won_)) {
        hudC(4, "TOO LATE", PAL_ALERT);
        hudC(6, reason_, PAL_INK);
    } else if (mode_ == Mode::Pause) {
        hudC(4, "PAUSE", PAL_GOLD);
    }
}

}  // namespace hornseven

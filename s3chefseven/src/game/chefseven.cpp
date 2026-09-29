#include "game/chefseven.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace chefseven {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float READY = 0.58f;
constexpr float TARGET = 0.70f;
constexpr float COOK[PANS] = {2.15f, 2.45f, 2.30f};
constexpr float PX[PANS] = {58.f, 140.f, 222.f};
constexpr float RIVAL_GAP = 1.72f;

const char* NAMES[] = {"BURGER", "TROUT", "STEAK", "OMELET"};

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(2, 1, 1));
    sys.apu.setMaster(0.85f);
    sys.apu.setEcho(0.12f, 0.22f, 0.14f);
    chefX_ = PX[1];
    if (bot_) beginMatch();
    else toTitle();
}

void Game::toTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    you_ = 0;
    them_ = 0;
    sel_ = 1;
    shake_ = 0;
    for (int i = 0; i < PANS; i++) pans_[i] = {};
}

void Game::beginMatch() {
    you_ = 0;
    them_ = 0;
    sel_ = 1;
    holdDir_ = 0;
    hold_ = 0;
    dropCd_ = 0.15f;
    rival_ = 0.4f;
    reach_ = 0;
    shake_ = 0;
    fanT_ = 0;
    nextDish_ = 0;
    won_ = false;
    over_ = false;
    chefX_ = PX[1];
    for (int i = 0; i < PANS; i++) pans_[i] = {};
    mode_ = Mode::Play;
}

void Game::nudge(int dir) {
    int n = std::clamp(sel_ + dir, 0, PANS - 1);
    if (n == sel_) return;
    sel_ = n;
    chime(0, 720.f, 0.035f, 0.03f);
}

void Game::humanAct(float dt) {
    const gs::Pad& pad = sys_->pad;
    int dir = 0;
    if (pad.down(gs::BTN_LEFT)) dir = -1;
    else if (pad.down(gs::BTN_RIGHT)) dir = 1;
    if (dir == 0) {
        if (pad.axisX < -0.45f) dir = -1;
        else if (pad.axisX > 0.45f) dir = 1;
    }
    if (dir == 0) {
        hold_ = 0;
        holdDir_ = 0;
    } else if (dir != holdDir_) {
        nudge(dir);
        holdDir_ = dir;
        hold_ = 0;
    } else {
        hold_ += dt;
        if (hold_ > 0.22f) {
            nudge(dir);
            hold_ = 0.12f;
        }
    }
    if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_TURBO)) plate(sel_);
}

void Game::botAct() {
    int urgent = -1;
    float worst = 1e9f;
    for (int i = 0; i < PANS; i++) {
        const Pan& p = pans_[i];
        if (p.dish < 0 || p.heat < READY) continue;
        float ttl = (1.f - p.heat) * COOK[i];
        if (ttl < worst) {
            worst = ttl;
            urgent = i;
        }
    }
    if (urgent < 0) return;
    if (pans_[urgent].heat < TARGET && worst > 0.55f) return;
    if (sel_ < urgent) sel_++;
    else if (sel_ > urgent) sel_--;
    else plate(urgent);
}

bool Game::plate(int pan) {
    if (mode_ != Mode::Play || pan < 0 || pan >= PANS) return false;
    Pan& p = pans_[pan];
    if (p.dish < 0) {
        if (!bot_) chime(0, 180.f, 0.03f, 0.03f);
        return false;
    }
    if (p.heat < READY) {
        p.heat = std::min(READY + 0.02f, p.heat + 0.04f);
        chime(0, 150.f, 0.07f, 0.08f);
        return false;
    }
    you_++;
    p = {};
    reach_ = 0.16f;
    chime(1, 988.f, 0.07f, 0.08f);
    sys_->apu.noiseBurst(0.16f, 2800.f, 0.035f);
    sys_->rumble(0.12f, 0.28f, 60);
    finishIf();
    return true;
}

void Game::tryDrop() {
    if (dropCd_ > 0 || over_) return;
    int slot = -1;
    for (int i = 0; i < PANS; i++)
        if (pans_[i].dish < 0) {
            slot = i;
            break;
        }
    if (slot < 0) return;
    pans_[slot].dish = nextDish_ % 4;
    nextDish_++;
    pans_[slot].heat = 0;
    pans_[slot].dinged = false;
    dropCd_ = 0.28f;
}

void Game::rivalStep(float dt) {
    if (over_) return;
    rival_ += dt;
    if (rival_ < RIVAL_GAP) return;
    rival_ = 0;
    them_++;
    chime(2, 392.f, 0.04f, 0.06f);
    finishIf();
}

void Game::finishIf() {
    if (over_) return;
    if (you_ >= GOAL && you_ > them_) {
        mode_ = Mode::Win;
        won_ = true;
        over_ = true;
        fanT_ = 0;
    } else if (them_ >= GOAL && them_ >= you_) {
        mode_ = Mode::Lose;
        won_ = false;
        over_ = true;
        shake_ = 0.4f;
    }
}

void Game::advance(float dt) {
    if (dropCd_ > 0) dropCd_ -= dt;
    for (int i = 0; i < PANS; i++) {
        Pan& p = pans_[i];
        if (p.dish < 0) continue;
        p.heat += dt / COOK[i];
        if (!p.dinged && p.heat >= READY) {
            p.dinged = true;
            chime(2, 1174.f, 0.05f, 0.08f);
        }
        if (p.heat >= 1.f) {
            p.heat = 1.f;
            p = {};
            shake_ = 0.35f;
            chime(0, 70.f, 0.1f, 0.3f);
            sys_->apu.noiseBurst(0.5f, 420.f, 0.3f);
        }
    }
    tryDrop();
    rivalStep(dt);
}

void Game::chime(int ch, float freq, float vol, float hold) {
    if (ch < 0 || ch > 2) return;
    sys_->apu.tone(ch, freq, vol);
    chime_[ch] = hold;
}

void Game::audio(float dt) {
    for (int i = 0; i < 3; i++) {
        if (i == 2 && mode_ == Mode::Win) continue;
        if (chime_[i] > 0) {
            chime_[i] -= dt;
            if (chime_[i] <= 0) sys_->apu.tone(i, 0, 0);
        }
    }
    if (mode_ == Mode::Win) {
        static const float notes[] = {523.f, 659.f, 784.f, 1046.f, 784.f, 1318.f};
        int step = int(fanT_ / 0.11f);
        if (step < 6) sys_->apu.tone(2, notes[step], 0.075f);
        else sys_->apu.tone(2, 0, 0);
        fanT_ += dt;
    }
    bool hot = mode_ == Mode::Play || mode_ == Mode::Title;
    sys_->apu.noise(hot ? 0.03f : 0.f, 1600.f, false);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    anim_ += DT;
    if (reach_ > 0) reach_ -= DT;
    if (shake_ > 0) shake_ = std::max(0.f, shake_ - DT);
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_C) ||
                      pad.pressed(gs::BTN_TURBO))) {
            chime(1, 880.f, 0.06f, 0.06f);
            beginMatch();
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            sys.quit();
        }
    } else if (mode_ == Mode::Play) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_MODE))) mode_ = Mode::Pause;
        else {
            if (bot_) botAct();
            else humanAct(DT);
            if (mode_ == Mode::Play) advance(DT);
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
        else if (pad.pressed(gs::BTN_MODE)) toTitle();
    } else if (mode_ == Mode::Win || mode_ == Mode::Lose) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) beginMatch();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) toTitle();
    }

    float goal = PX[std::clamp(sel_, 0, PANS - 1)];
    chefX_ += (goal - chefX_) * std::min(1.f, DT * 10.f);
    audio(DT);
    if (mode_ == Mode::Win) sys.setLight(40, 180, 70);
    else if (mode_ == Mode::Lose) sys.setLight(180, 30, 20);
    else sys.setLight(170, 90, 40);
    draw();
}

float Game::demoHeat(int i) const {
    float s = 0.5f + 0.5f * std::sin(anim_ * 0.85f + float(i) * 1.7f);
    return 0.28f + 0.55f * s;
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    if (!s) return;
    hud(20 - int(std::strlen(s)) / 2, row, s, pal);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool shadow) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::solid(float x, float y, float w, float h, int pal) {
    if (w < 1.f || h < 1.f) return;
    gs::Sprite s;
    s.img = art_.solid.lv[0];
    s.x = int16_t(std::lround(x));
    s.y = int16_t(std::lround(y));
    s.w = int16_t(std::max(1, int(std::lround(w))));
    s.h = int16_t(std::max(1, int(std::lround(h))));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::bar(float x, float y, float w, float heat) {
    float fw = w * std::clamp(heat, 0.f, 1.f);
    if (fw >= 1.f) {
        int pal = PAL_RAW;
        if (heat >= 0.90f) pal = ((int(anim_ * 12.f) & 1) ? PAL_RED : PAL_HOT);
        else if (heat >= READY) pal = PAL_OK;
        solid(x, y, fw, 6, pal);
    }
    solid(x + w * READY, y, w * (1.f - READY), 6, PAL_ZONE);
    solid(x, y, w, 6, PAL_TRACK);
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    bool lose = mode_ == Mode::Lose;
    v.setFogColor(lose ? gs::rgb4(10, 1, 0) : gs::rgb4(2, 1, 1));
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t c;
        if (y < 18) c = gs::rgb4(4, 1, 1);
        else if (y < 128) c = gs::rgb4(14, 11, 9);
        else if (y < 176) c = gs::rgb4(9, 10, 11);
        else c = gs::rgb4(5, 3, 2);
        v.lineBackdrop[y] = c;
        v.lineFog[y] = lose ? 3 : 0;
        v.road[y].on = false;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    float ox = 0;
    if (shake_ > 0) ox = std::sin(anim_ * 90.f) * 3.f * shake_;
    const bool service = mode_ != Mode::Title;
    const int flame = int(anim_ * 9.f) % 3;

    if (mode_ == Mode::Title) {
        spr(art_.title, 150, 22, float(art_.title.h), PAL_HUD);
        spr(art_.sub, 150, 46, float(art_.sub.h), PAL_GOLD);
    } else if (mode_ == Mode::Win) {
        spr(art_.win, 150, 28, float(art_.win.h), PAL_GREEN);
    } else if (mode_ == Mode::Lose) {
        spr(art_.lose, 150, 28, float(art_.lose.h), PAL_RED);
    }

    for (int i = 0; i < GOAL; i++) {
        solid(8.f + float(i) * 12.f, 4, 9, 6, i < you_ ? PAL_GREEN : PAL_ZONE);
        solid(228.f + float(i) * 12.f, 4, 9, 6, i < them_ ? PAL_RED : PAL_TRACK);
    }

    float bob = std::sin(anim_ * 6.f) * 1.3f;
    int pose = (reach_ > 0 || (mode_ == Mode::Win && (int(anim_ * 3.f) & 1))) ? 1 : 0;
    spr(art_.chef[pose], chefX_ + ox, 188 + bob, 46, PAL_CHEF);
    spr(art_.shade, chefX_ + ox, 208, 10, PAL_HUD, false, true);

    float ry = 168 + std::sin(anim_ * 4.f) * 1.5f;
    spr(art_.rival, 292, ry, 36, PAL_RIVAL, true);
    bar(268, 96, 40, service ? std::fmod(rival_ / RIVAL_GAP, 1.f) : demoHeat(2));

    for (int i = 0; i < PANS; i++) {
        bool show = !service;
        int dish = i % 4;
        float heat = demoHeat(i);
        if (service && pans_[i].dish >= 0) {
            show = true;
            dish = pans_[i].dish;
            heat = pans_[i].heat;
        }
        spr(art_.pan, PX[i] + ox, 150, 18, PAL_STEEL);
        spr(art_.flame[flame], PX[i] + ox, 166, 16, PAL_FIRE);
        if (!show) continue;
        float lift = (service && i == sel_) ? -3.f : 0.f;
        if (heat >= READY) {
            lift -= 2.f;
            spr(art_.bell, PX[i] + 18 + ox, 112, 12, PAL_GOLD);
        }
        spr(art_.dish[dish], PX[i] + ox, 128 + lift, 28, PAL_FOOD);
        bar(PX[i] - 24 + ox, 100, 48, heat);
        if (service) hud(int(PX[i] / 8.f) - 2, 8, NAMES[dish], heat >= READY ? PAL_GREEN : PAL_PAPER);
    }

    char buf[40];
    if (mode_ == Mode::Title) {
        hudC(24, "SIX IS STILL SHORT", PAL_HUD);
        hudC(25, "ARROWS MOVE   Z PLATES", PAL_GOLD);
        if ((int(anim_ * 2.f) & 1) == 0) hudC(26, "PRESS START", PAL_GOLD);
    } else {
        hud(1, 1, "YOU", PAL_GREEN);
        hud(34, 1, "THEM", PAL_RED);
        std::snprintf(buf, sizeof buf, "%d", you_);
        hud(5, 1, buf, you_ >= GOAL ? PAL_GREEN : PAL_HUD);
        std::snprintf(buf, sizeof buf, "%d", them_);
        hud(31, 1, buf, them_ >= GOAL ? PAL_RED : PAL_HUD);
        if (mode_ == Mode::Win) {
            hudC(16, "FIRST TO SEVEN", PAL_GREEN);
            hudC(26, "START SERVES AGAIN", PAL_HUD);
        } else if (mode_ == Mode::Lose) {
            hudC(16, "THEY HIT SEVEN", PAL_RED);
            hudC(26, "START RETRIES", PAL_HUD);
        } else if (mode_ == Mode::Pause) {
            hudC(14, "PAUSED", PAL_GOLD);
        } else if (you_ == 6) {
            hudC(26, "SIX IS STILL SHORT", PAL_GOLD);
        }
    }
}

}  // namespace chefseven

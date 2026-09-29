#include "game/mark.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace chefmark {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float COOK = 4.8f;
constexpr float MARK_LO = 0.68f;
constexpr float MARK_HI = 0.82f;
constexpr float AIM = 0.74f;

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(2, 1, 1));
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.apu.setMaster(0.85f);
    sys.apu.setEcho(0.1f, 0.18f, 0.12f);
    if (bot_) begin();
    else toTitle();
}

void Game::toTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    finished_ = false;
    score_ = 0;
    heat_ = 0;
    dinged_ = false;
    fan_ = 0;
    fail_[0] = 0;
    sys_->setLight(160, 70, 30);
}

void Game::begin() {
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
    finished_ = false;
    score_ = 0;
    heat_ = 0;
    t_ = 0;
    fan_ = 0;
    dinged_ = false;
    fail_[0] = 0;
}

void Game::chime(int ch, float freq, float vol, float hold) {
    if (ch < 0 || ch > 2) return;
    sys_->apu.tone(ch, freq, vol);
    chime_[ch] = hold;
}

void Game::plate() {
    if (mode_ != Mode::Play) return;
    if (heat_ >= MARK_LO && heat_ <= MARK_HI) {
        float d = std::fabs(heat_ - AIM);
        float acc = 1.f - std::min(1.f, d / 0.10f);
        score_ = 700 + int(std::lround(300.f * acc));
        finished_ = true;
        won_ = true;
        over_ = true;
        mode_ = Mode::Win;
        fan_ = 0;
        chime(1, 988.f, 0.08f, 0.12f);
        sys_->apu.noiseBurst(0.14f, 2400.f, 0.04f);
        sys_->rumble(0.2f, 0.4f, 80);
        return;
    }
    won_ = false;
    finished_ = false;
    over_ = true;
    mode_ = Mode::Lose;
    if (heat_ < MARK_LO) std::snprintf(fail_, sizeof fail_, "plated raw");
    else std::snprintf(fail_, sizeof fail_, "plated past the mark");
    chime(0, 140.f, 0.09f, 0.2f);
    sys_->rumble(0.5f, 0.6f, 120);
}

void Game::advance(float dt) {
    heat_ += dt / COOK;
    if (!dinged_ && heat_ >= MARK_LO) {
        dinged_ = true;
        chime(2, 1174.f, 0.06f, 0.1f);
    }
    if (heat_ >= 1.f) {
        heat_ = 1.f;
        won_ = false;
        finished_ = false;
        over_ = true;
        mode_ = Mode::Lose;
        std::snprintf(fail_, sizeof fail_, "burned the steak");
        sys_->apu.noiseBurst(0.5f, 380.f, 0.35f);
        chime(0, 70.f, 0.12f, 0.4f);
        sys_->rumble(0.8f, 1.f, 180);
    }
}

void Game::botAct() {
    if (heat_ >= AIM && heat_ <= MARK_HI) plate();
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
        static const float notes[] = {523.f, 659.f, 784.f, 1046.f};
        int step = int(fan_ / 0.12f);
        if (step < 4) sys_->apu.tone(2, notes[step], 0.07f);
        else sys_->apu.tone(2, 0, 0);
        fan_ += dt;
    }
    float vol = (mode_ == Mode::Play && heat_ > 0.05f) ? 0.035f : (mode_ == Mode::Title ? 0.015f : 0.f);
    sys_->apu.noise(vol, mode_ == Mode::Play ? 1700.f : 800.f, false);
}

void Game::lights() {
    if (mode_ == Mode::Win) sys_->setLight(40, 180, 60);
    else if (mode_ == Mode::Lose || heat_ > MARK_HI) sys_->setLight(190, 30, 20);
    else if (heat_ >= MARK_LO) sys_->setLight(220, 160, 30);
    else sys_->setLight(160, 70, 30);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_C)) begin();
        else if (pad.pressed(gs::BTN_MODE)) sys.quit();
    } else if (mode_ == Mode::Play) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_MODE))) mode_ = Mode::Pause;
        else {
            if (bot_) botAct();
            else if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_C)) plate();
            if (mode_ == Mode::Play) advance(DT);
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
        else if (pad.pressed(gs::BTN_MODE)) toTitle();
    } else if (!bot_) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) begin();
        else if (pad.pressed(gs::BTN_MODE)) toTitle();
    }
    audio(DT);
    lights();
    draw();
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

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 400));
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 400));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
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

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    bool lose = mode_ == Mode::Lose;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t c;
        if (y < 28) c = gs::rgb4(6, 2, 2);
        else if (y < 128) c = gs::rgb4(13, 10, 8);
        else if (y < 176) c = gs::rgb4(8, 9, 10);
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

    const float bx = 70.f, by = 78.f, bw = 180.f;
    if (mode_ == Mode::Title) {
        spr(art_.title, 160, 36, float(art_.title.h), PAL_GOLD);
        hudC(8, "ONE STEAK", PAL_HUD);
        hudC(10, "PLATE IT ON THE MARK", PAL_HUD);
        hudC(24, "A PLATES   START", PAL_HUD);
    } else if (mode_ == Mode::Win) {
        spr(art_.done, 160, 28, float(art_.done.h), PAL_GREEN);
        spr(art_.mark, 160, 58, 22, PAL_GOLD);
        char buf[24];
        std::snprintf(buf, sizeof buf, "SCORE %d", score_);
        hudC(22, buf, PAL_HUD);
        hudC(24, "THE MARK IS CLOSED", PAL_GREEN);
    } else if (mode_ == Mode::Lose) {
        spr(heat_ >= 1.f || heat_ > MARK_HI ? art_.burn : art_.raw, 160, 30, 16, PAL_RED);
        hudC(24, fail_, PAL_RED);
    } else if (mode_ == Mode::Pause) {
        hudC(3, "PAUSED", PAL_GOLD);
    } else {
        hudC(1, "HIT THE GOLD MARK", PAL_HUD);
    }

    if (mode_ != Mode::Title) {
        spr(art_.ticket, 160, 58, 22, PAL_PAPER);
        hudC(6, "STEAK", PAL_HUD);
    }

    float fw = bw * std::clamp(heat_, 0.f, 1.f);
    int fill = PAL_RED;
    if (heat_ >= MARK_LO && heat_ <= MARK_HI) fill = PAL_GOLD;
    else if (heat_ < MARK_LO) fill = PAL_GREEN;
    solid(bx + bw * MARK_LO, by - 3, 2, 14, PAL_GOLD);
    solid(bx + bw * MARK_HI, by - 3, 2, 14, PAL_GOLD);
    if (fw >= 1.f) solid(bx, by, fw, 8, fill);
    solid(bx, by, bw, 8, PAL_INK);

    int flame = int(t_ * 9.f) % 3;
    bool burnt = mode_ == Mode::Lose && (heat_ >= 1.f || heat_ > MARK_HI);
    spr(art_.flame[flame], 150, 168, 22, PAL_FIRE);
    spr(art_.pan, 150, 152, 28, PAL_STEEL);
    if (mode_ != Mode::Win) spr(burnt ? art_.charred : art_.steak, 140, 128, 40, PAL_FOOD);
    else spr(art_.steak, 140, 128, 40, PAL_FOOD);
    spr(art_.chef, 230, 168, 72, PAL_CHEF);

    if (mode_ == Mode::Play && heat_ >= MARK_LO && heat_ <= MARK_HI) hudC(12, "MARK", PAL_GOLD);
}

}  // namespace chefmark

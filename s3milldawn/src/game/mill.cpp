#include "game/mill.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace mill {
namespace {
constexpr float DT = 1.0f / 60.0f;
constexpr float NIGHT = 46.0f;
constexpr int NFL = 4;
constexpr float REACH = 34.0f;

const float kX[NFL] = {52.0f, 118.0f, 206.0f, 274.0f};
const float kY[NFL] = {158.0f, 192.0f, 150.0f, 186.0f};
const float kDrain[NFL] = {0.058f, 0.072f, 0.064f, 0.078f};
}

bool Game::allLit() const {
    for (int i = 0; i < NFL; i++)
        if (flares_[i].fuel <= 0.001f) return false;
    return true;
}

int Game::flaresLit() const {
    int n = 0;
    for (int i = 0; i < NFL; i++)
        if (flares_[i].fuel > 0.001f) n++;
    return n;
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Watch) return clock_ > NIGHT * 0.5f ? 1 : 2;
    return 3;
}

void Game::tone(float f, float v, float hold) {
    sys_->apu.tone(0, f, v);
    beep_ = hold;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.apu.setMaster(0.42f);
    mode_ = Mode::Title;
    t_ = 0;
}

void Game::begin() {
    score_ = 0;
    stokes_ = 0;
    over_ = false;
    won_ = false;
    px_ = 160;
    py_ = 170;
    face_ = 1;
    stokeT_ = 0;
    cool_ = 0;
    clock_ = NIGHT;
    gustEvery_ = 1.1f;
    gustAt_ = 0;
    fan_ = false;
    noteI_ = 0;
    noteT_ = 0;
    for (int i = 0; i < NFL; i++) {
        flares_[i].x = kX[i];
        flares_[i].y = kY[i];
        flares_[i].fuel = 0.86f;
        flares_[i].gust = 0;
    }
    mode_ = Mode::Watch;
    t_ = 0;
    tone(330, 0.08f, 0.12f);
}

void Game::tryStoke() {
    if (cool_ > 0) return;
    int best = -1;
    float bestD = REACH;
    for (int i = 0; i < NFL; i++) {
        float dx = flares_[i].x - px_;
        float dy = flares_[i].y - py_;
        float d = std::sqrt(dx * dx + dy * dy);
        if (d < bestD) {
            bestD = d;
            best = i;
        }
    }
    if (best < 0) return;
    stokeT_ = 0.18f;
    if (flares_[best].x < px_ - 4) face_ = -1;
    else if (flares_[best].x > px_ + 4) face_ = 1;
    if (flares_[best].fuel >= 0.97f) return;
    flares_[best].fuel = 1.0f;
    cool_ = 0.16f;
    stokes_++;
    score_ += 15;
    tone(520 + best * 40.0f, 0.07f, 0.05f);
}

void Game::botThink(float& ax, float& ay, bool& stoke) {
    ax = 0;
    ay = 0;
    stoke = false;
    int best = 0;
    float low = 2.0f;
    for (int i = 0; i < NFL; i++) {
        float urgency = flares_[i].fuel - (flares_[i].gust > 0 ? 0.12f : 0);
        if (urgency < low) {
            low = urgency;
            best = i;
        }
    }
    float dx = flares_[best].x - px_;
    float dy = flares_[best].y - py_;
    float d = std::sqrt(dx * dx + dy * dy);
    if (d > 10.0f) {
        ax = dx / d;
        ay = dy / d;
        if (std::fabs(dx) > 4) face_ = dx < 0 ? -1 : 1;
    }
    if (d < REACH) stoke = true;
}

void Game::update(float dt) {
    t_ += dt;
    if (beep_ > 0) {
        beep_ -= dt;
        if (beep_ <= 0) sys_->apu.tone(0, 0, 0);
    }
    if (fan_) {
        static const float notes[] = {392, 494, 587, 784, 988};
        noteT_ -= dt;
        if (noteT_ <= 0 && noteI_ < 5) {
            sys_->apu.tone(1, notes[noteI_], 0.11f);
            noteI_++;
            noteT_ = 0.14f;
        }
        if (noteI_ >= 5 && noteT_ <= 0) {
            sys_->apu.tone(1, 0, 0);
            fan_ = false;
        }
    }
    if (mode_ != Mode::Watch) return;

    if (cool_ > 0) cool_ -= dt;
    if (stokeT_ > 0) stokeT_ -= dt;

    float ax = 0, ay = 0;
    bool want = false;
    if (bot_) {
        botThink(ax, ay, want);
        if (want) tryStoke();
    } else {
        const gs::Pad& p = sys_->pad;
        if (p.down(gs::BTN_LEFT) || p.axisX < -0.3f) {
            ax = -1;
            face_ = -1;
        }
        if (p.down(gs::BTN_RIGHT) || p.axisX > 0.3f) {
            ax = 1;
            face_ = 1;
        }
        if (p.down(gs::BTN_UP) || p.axisY > 0.3f) ay = -1;
        if (p.down(gs::BTN_DOWN) || p.axisY < -0.3f) ay = 1;
        if (ax != 0 && ay != 0) {
            ax *= 0.707f;
            ay *= 0.707f;
        }
        if (p.down(gs::BTN_A) || p.pressed(gs::BTN_B)) tryStoke();
    }
    px_ = std::clamp(px_ + ax * 210.0f * dt, 22.0f, 300.0f);
    py_ = std::clamp(py_ + ay * 150.0f * dt, 136.0f, 208.0f);

    gustEvery_ -= dt;
    if (gustEvery_ <= 0) {
        gustEvery_ = 2.6f;
        flares_[gustAt_ % NFL].gust = 0.85f;
        gustAt_++;
        tone(140, 0.04f, 0.06f);
    }

    for (int i = 0; i < NFL; i++) {
        float drain = kDrain[i];
        if (flares_[i].gust > 0) {
            drain += 0.26f;
            flares_[i].gust -= dt;
        }
        flares_[i].fuel -= drain * dt;
        if (flares_[i].fuel <= 0) {
            flares_[i].fuel = 0;
            mode_ = Mode::Defeat;
            over_ = true;
            won_ = false;
            tone(64, 0.18f, 0.45f);
            return;
        }
    }

    clock_ -= dt;
    if (clock_ <= 0) {
        clock_ = 0;
        if (allLit()) {
            mode_ = Mode::Victory;
            over_ = true;
            won_ = true;
            score_ += 400 + stokes_ * 5;
            fan_ = true;
            noteI_ = 0;
            noteT_ = 0;
        } else {
            mode_ = Mode::Defeat;
            over_ = true;
            won_ = false;
        }
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    const float adv = 16.0f * scale;
    float left = x - float(s.size()) * adv * 0.5f;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, left + i * adv + g.w * scale * 0.5f, y, g.h * scale, pal, false);
    }
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || row < 0 || row > 27 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.roadTime = int(t_ * 60);
    for (int y = 0; y < gs::SCREEN_H; y++) v.road[y].on = false;

    const float dawn = mode_ == Mode::Title ? 0.05f : 1.0f - std::clamp(clock_ / NIGHT, 0.0f, 1.0f);
    const int horizon = 108;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        if (y < horizon) {
            float u = y / float(horizon);
            int r = int(1 + dawn * 13 + u * 2);
            int g = int(1 + dawn * 7 + u * 1);
            int b = int(7 - dawn * 4 + (1.0f - u) * 3);
            r = std::clamp(r, 0, 15);
            g = std::clamp(g, 0, 15);
            b = std::clamp(b, 0, 15);
            v.lineBackdrop[y] = gs::rgb4(r, g, b);
        } else {
            float u = float(y - horizon) / float(gs::SCREEN_H - horizon);
            int r = int(2 + dawn * 6 + u * 2);
            int g = int(3 + dawn * 4 + (1.0f - u) * 2);
            int b = int(2 + dawn * 1);
            v.lineBackdrop[y] = gs::rgb4(std::clamp(r, 0, 15), std::clamp(g, 0, 15), std::clamp(b, 0, 15));
        }
    }

    // Earlier sprites sit in front. Letters, the miller, flames, then the mill and sky.
    if (mode_ == Mode::Title) {
        text("MILLDAWN", 160, 40, 1.05f, PAL_HUD);
        text("KEEP THE FLARES LIT", 160, 68, 0.42f, PAL_FLAME);
        text("UNTIL DAWN", 160, 86, 0.5f, PAL_HUD);
        text("ARROWS MOVE   A STOKE", 160, 196, 0.38f, PAL_MAN);
        float pulse = 0.45f + 0.08f * std::sin(t_ * 4.0f);
        text("PRESS START", 160, 214, pulse, PAL_HUD);
    } else if (mode_ == Mode::Victory) {
        text("DAWN", 160, 36, 1.1f, PAL_HUD);
        text("THE MILL HELD", 160, 62, 0.48f, PAL_FLAME);
    } else if (mode_ == Mode::Defeat) {
        text("GONE DARK", 160, 36, 0.9f, PAL_HUD);
        text("A FLARE DIED", 160, 60, 0.42f, PAL_FLAME);
    }

    const bool walking = mode_ == Mode::Watch;
    if (walking || mode_ == Mode::Victory || mode_ == Mode::Defeat) {
        bool arm = stokeT_ > 0;
        spr(art_.man[arm ? 1 : 0], px_, py_ - 16, 40, PAL_MAN, face_ < 0);
    } else {
        spr(art_.man[0], 160, 150, 44, PAL_MAN, false);
        for (int i = 0; i < NFL; i++) {
            float fy = kY[i];
            float fx = kX[i];
            float bob = 4.0f * std::sin(t_ * 7.0f + i);
            spr(art_.flame[0], fx, fy - 28 + bob * 0.2f, 26 + bob, PAL_FLAME, false);
            spr(art_.stake, fx, fy - 6, 22, PAL_YARD, false);
        }
    }

    if (mode_ != Mode::Title) {
        for (int i = NFL - 1; i >= 0; i--) {
            const Flare& f = flares_[i];
            float flick = std::sin(t_ * 18.0f + i * 1.7f) * (f.fuel < 0.35f ? 5.0f : 2.0f);
            int low = f.fuel < 0.28f ? 1 : 0;
            float fh = 10.0f + f.fuel * 26.0f + flick;
            if (f.fuel > 0.02f) spr(art_.flame[low], f.x, f.y - 22 - fh * 0.25f, std::max(8.0f, fh), PAL_FLAME, false);
            spr(art_.stake, f.x, f.y - 4, 24, PAL_YARD, false);
            if (f.gust > 0) {
                float gx = f.x - 18 + std::fmod(t_ * 40.0f, 10.0f);
                spr(art_.gust, gx, f.y - 30, 8, PAL_YARD, false);
            }
        }
    }

    int sail = int(t_ * 3.0f) & 1;
    spr(art_.mill[sail], 160, 86, 108, PAL_MILL, false);

    if (dawn < 0.72f) {
        float my = 28 + dawn * 40;
        spr(art_.moon, 250, my, 22.0f * (1.0f - dawn * 0.8f), PAL_MOON, false);
    }
    static const int sx[12] = {18, 40, 70, 96, 130, 188, 214, 236, 280, 300, 54, 160};
    static const int sy[12] = {16, 34, 12, 48, 22, 18, 40, 14, 28, 46, 58, 8};
    for (int i = 0; i < 12; i++) {
        if (dawn > 0.55f && (i & 1)) continue;
        float tw = 4.0f + ((i + int(t_ * 3)) & 1);
        spr(art_.star, float(sx[i]), float(sy[i]), tw, PAL_MOON, false);
    }

    if (mode_ != Mode::Title) {
        int left = int(std::ceil(clock_));
        char buf[48];
        std::snprintf(buf, sizeof buf, "DAWN %d", left);
        hud(1, 1, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "LIT %d", flaresLit());
        hudC(1, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "%d", score_);
        hud(34, 1, buf, PAL_HUD);
        hud(1, 26, "A STOKE THE NEAREST FLARE", PAL_HUD);
        for (int i = 0; i < NFL; i++) {
            int bars = int(std::lround(flares_[i].fuel * 4));
            std::string mark(4, '.');
            for (int b = 0; b < bars && b < 4; b++) mark[b] = '#';
            hud(30, 3 + i, mark, flares_[i].fuel < 0.3f ? PAL_FLAME : PAL_HUD);
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (mode_ == Mode::Title) {
        bool go = bot_ && t_ > 0.4f;
        if (!bot_ && (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A))) go = true;
        if (go) begin();
    }
    update(DT);
    draw();
}

}  // namespace mill

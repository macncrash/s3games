#include "game/dawn.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace culvertdawn {

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.apu.setMaster(0.5f);
    if (bot_) begin();
}

void Game::begin() {
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
    t_ = 0;
    px_ = kX[0];
    face_ = 1;
    step_ = 0;
    cool_ = 0;
    strike_ = 0;
    beat_ = 0;
    dead_ = -1;
    for (float& f : flare_) f = 1.f;
}

void Game::botAct(bool& left, bool& right, bool& light) {
    int needy = 0;
    for (int i = 1; i < 4; i++)
        if (flare_[i] < flare_[needy]) needy = i;
    int goal = beat_;
    if (flare_[needy] < 0.72f) goal = needy;
    float dx = kX[goal] - px_;
    if (std::fabs(dx) <= 12.f) {
        if (flare_[goal] < 0.98f) light = true;
        else if (goal == beat_ && flare_[needy] >= 0.72f) beat_ = (beat_ + 1) % 4;
    } else if (dx > 0) {
        right = true;
    } else {
        left = true;
    }
}

void Game::update(float dt) {
    bool left = false, right = false, light = false;
    if (bot_) botAct(left, right, light);
    else if (sys_) {
        const gs::Pad& p = sys_->pad;
        left = p.down(gs::BTN_LEFT);
        right = p.down(gs::BTN_RIGHT);
        light = p.down(gs::BTN_A) || p.down(gs::BTN_B) || p.down(gs::BTN_TURBO);
    }
    float vx = (right ? 1.f : 0.f) - (left ? 1.f : 0.f);
    if (vx != 0) face_ = vx;
    px_ = std::clamp(px_ + vx * 168.f * dt, 28.f, 292.f);
    step_ += std::fabs(vx) * dt * 8.f;
    if (cool_ > 0) cool_ -= dt;
    if (strike_ > 0) strike_ -= dt;

    int gust = int(t_ / 2.5f) % 4;
    for (int i = 0; i < 4; i++) {
        float rate = 0.048f + (i == gust ? 0.055f : 0.f);
        flare_[i] -= rate * dt;
    }
    if (light && cool_ <= 0) {
        int pick = -1;
        float best = 1.f;
        for (int i = 0; i < 4; i++) {
            if (std::fabs(px_ - kX[i]) > 18.f) continue;
            if (flare_[i] < best) {
                best = flare_[i];
                pick = i;
            }
        }
        if (pick >= 0 && flare_[pick] < 0.995f) {
            flare_[pick] = 1.f;
            cool_ = 0.18f;
            strike_ = 0.16f;
            if (sys_) sys_->apu.noiseBurst(0.18f, 5200.f, 14.f);
        }
    }
    for (int i = 0; i < 4; i++) {
        if (flare_[i] <= 0.f) {
            flare_[i] = 0.f;
            dead_ = i;
            mode_ = Mode::Out;
            over_ = true;
            won_ = false;
            if (sys_) sys_->apu.tone(0, 90.f, 0.12f);
            return;
        }
    }
    t_ += dt;
    if (t_ >= kNight) {
        mode_ = Mode::Dawn;
        over_ = true;
        won_ = true;
        if (sys_) {
            sys_->apu.tone(0, 0, 0);
            sys_->apu.tone(1, 523.f, 0.08f);
            sys_->apu.tone(2, 659.f, 0.06f);
        }
    }
}

void Game::spr(const gs::Image& img, float x, float y, float w, float h, int pal, int fog, bool flip) {
    gs::Sprite s;
    s.img = img;
    s.x = int16_t(x);
    s.y = int16_t(y);
    s.w = int16_t(std::max(1.f, w));
    s.h = int16_t(std::max(1.f, h));
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    for (int i = 0; i < int(s.size()); i++) {
        unsigned char c = static_cast<unsigned char>(s[size_t(i)]);
        if (c < 32 || c > 127) continue;
        sys_->vdp.HUD.set(col + i, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    float dawn = 0.f;
    if (mode_ == Mode::Dawn) dawn = 1.f;
    else if (mode_ == Mode::Play) dawn = std::clamp((t_ - (kNight - 12.f)) / 12.f, 0.f, 1.f);
    else if (mode_ == Mode::Out) dawn = 0.f;

    for (int y = 0; y < gs::SCREEN_H; y++) {
        float ny = float(y) / float(gs::SCREEN_H - 1);
        int nr = 1;
        int ng = 1;
        int nb = 2 + int((1.f - ny) * 4.f);
        int dr = 13 - int(ny * 5.f);
        int dg = 8 - int(ny * 3.f);
        int db = 5 - int(ny * 2.f);
        if (mode_ == Mode::Out) {
            dr = 6;
            dg = 1;
            db = 1;
        }
        auto mix = [&](int a, int bcol) {
            return int(a + (bcol - a) * dawn + 0.5f);
        };
        v.lineBackdrop[y] = gs::rgb4(mix(nr, dr), mix(ng, dg), mix(nb, db));
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
    v.setFogColor(gs::rgb4(int(2 + 10 * dawn), int(2 + 6 * dawn), int(4 + 2 * dawn)));

    v.clearSprites();
    v.HUD.clear();

    // Characters and flares first: earlier sprites sit in front of the pipe.
    int wf = int(step_) & 1;
    float wy = 148.f + (strike_ > 0 ? -2.f : 0.f);
    spr(art_.watch[wf], px_ - 10.f, wy, 20, 36, PAL_WATCH, 0, face_ < 0);

    for (int i = 0; i < 4; i++) {
        float life = flare_[i];
        float fx = kX[i];
        spr(art_.pot, fx - 8.f, 166.f, 16, 18, 6);
        if (life <= 0.02f) {
            spr(art_.ash, fx - 6.f, 154.f, 12, 16, PAL_ASH);
        } else {
            float flick = 0.85f + 0.15f * std::sin(t_ * 17.f + i * 2.1f);
            float h = (10.f + 22.f * life) * flick;
            spr(art_.flame, fx - 6.f, 166.f - h, 12, h, PAL_FLARE);
        }
    }

    spr(art_.culvert, 0, 0, 320, 224, PAL_STONE);

    int moonFog = int(dawn * 14.f);
    float moonX = 230.f - t_ * 1.2f;
    if (mode_ != Mode::Title) spr(art_.moon, moonX, 58.f + dawn * 10.f, 28, 28, PAL_NIGHT, moonFog);
    else spr(art_.moon, 236, 64, 28, 28, PAL_NIGHT, 0);

    const int stars[][2] = {{48, 62}, {80, 78}, {120, 58}, {160, 74}, {200, 60}, {250, 80}, {280, 66}};
    int starFog = 4 + int(dawn * 12.f);
    for (auto s : stars) spr(art_.star, float(s[0]), float(s[1]), 5, 5, PAL_NIGHT, mode_ == Mode::Title ? 2 : starFog);

    if (mode_ == Mode::Title) {
        spr(art_.title, 78, 96, float(art_.title.w), float(art_.title.h), PAL_HUD);
        hud(6, 18, "KEEP THE FLARES LIT", PAL_HUD);
        hud(7, 20, "ARROWS MOVE   Z LIGHTS", PAL_HUD);
        hud(10, 22, "START TO TAKE THE WATCH", PAL_HUD);
    } else if (mode_ == Mode::Play || mode_ == Mode::Dawn || mode_ == Mode::Out) {
        int left = mode_ == Mode::Play ? int(std::ceil(kNight - t_)) : 0;
        char line[40];
        std::snprintf(line, sizeof(line), "DAWN %02d", left);
        hud(1, 1, line, PAL_HUD);
        hud(16, 1, "FLARES", PAL_HUD);
        for (int i = 0; i < 4; i++) {
            int bars = int(flare_[i] * 5.f + 0.05f);
            if (bars < 0) bars = 0;
            if (bars > 5) bars = 5;
            std::string mark(size_t(bars), '#');
            mark.append(size_t(5 - bars), '.');
            hud(23 + i * 4, 1, mark, PAL_HUD);
        }
        if (mode_ == Mode::Dawn) hud(8, 12, "THE FLARES HELD", PAL_HUD);
        if (mode_ == Mode::Out) hud(9, 12, "A FLARE DIED", PAL_HUD);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const float dt = 1.f / 60.f;
    if (mode_ == Mode::Title) {
        if (bot_ || sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A)) begin();
    } else if (mode_ == Mode::Play) {
        update(dt);
    } else if (mode_ == Mode::Out || mode_ == Mode::Dawn) {
        if (!bot_ && (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A))) begin();
    }
    draw();
}

}  // namespace culvertdawn

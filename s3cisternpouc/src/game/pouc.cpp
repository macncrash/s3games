#include "game/pouc.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace cisternpouc {
namespace {

constexpr float GRAV = 0.24f;
constexpr float JUMP = -6.15f;
constexpr float MOVE = 1.65f;
constexpr float AIR = 1.85f;
constexpr int WATCH_FRAMES = 60 * 42;

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    // near rim, pouch shelf, stair, far lip
    platBox_[0] = {20, 52, 96};
    platBox_[1] = {118, 176, 124};
    platBox_[2] = {186, 118, 118};
    platBox_[3] = {198, 56, 106};
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int d = y < 70 ? 1 : y < 140 ? 2 : 3;
        sys.vdp.lineBackdrop[y] = gs::rgb4(0, d, d + 1);
        sys.vdp.lineFog[y] = 0;
        sys.vdp.road[y].on = false;
    }
    sys.vdp.setFogColor(gs::rgb4(0, 2, 3));
    mode_ = 0;
    titleHold_ = 0;
}

void Game::begin() {
    mode_ = 1;
    over_ = false;
    won_ = false;
    held_ = false;
    phase_ = 0;
    px_ = 58;
    py_ = 52;
    vx_ = 0;
    vy_ = 0;
    plat_ = 0;
    face_ = 1;
    pouchX_ = 168;
    pouchY_ = 176;
    bobY_ = 96;
    bobV_ = 1.15f;
    watch_ = WATCH_FRAMES;
    t_ = 0;
    step_ = 0;
    reason_ = "";
}

void Game::win() {
    mode_ = 2;
    won_ = true;
    over_ = true;
    vx_ = 0;
    reason_ = "across";
    sys_->apu.tone(0, 660, 0.2f);
}

void Game::lose(const char* why) {
    mode_ = 3;
    won_ = false;
    over_ = true;
    vx_ = 0;
    reason_ = why;
    sys_->apu.tone(1, 70, 0.22f);
}

int Game::marker() const {
    if (mode_ == 0) return 0;
    if (over_) return 4;
    if (held_ && py_ < 150.f) return 3;
    if (held_) return 2;
    return 1;
}

void Game::bot(bool& left, bool& right, bool& jump) {
    left = right = jump = false;
    auto go = [&](float x) {
        if (px_ < x - 4) right = true;
        else if (px_ > x + 4) left = true;
    };
    if (!held_) {
        if (plat_ == 1 || py_ > 160.f) {
            go(pouchX_);
        } else {
            right = true;
        }
        return;
    }
    if (plat_ == 3) {
        go(250);
        return;
    }
    if (plat_ == 2 || (py_ < 150.f && py_ > 90.f)) {
        if (px_ < 230) right = true;
        else jump = plat_ == 2;
        return;
    }
    if (px_ < 230) right = true;
    else if (plat_ == 1) jump = true;
    else right = true;
}

void Game::stepPlay(bool left, bool right, bool jump) {
    t_ += 1.f;
    if (watch_ > 0) watch_--;
    if (watch_ == 0) {
        lose("watch ran out");
        return;
    }

    float spd = plat_ >= 0 ? MOVE : AIR;
    if (left) {
        vx_ = -spd;
        face_ = -1;
    } else if (right) {
        vx_ = spd;
        face_ = 1;
    } else {
        vx_ *= (plat_ >= 0) ? 0.45f : 0.92f;
    }
    if (plat_ >= 0 && jump) {
        vy_ = JUMP;
        plat_ = -1;
    }
    px_ += vx_;
    if (px_ < 22) px_ = 22;
    if (px_ > 298) px_ = 298;

    float oldY = py_;
    py_ += vy_;
    vy_ += GRAV;
    if (vy_ > 6.2f) vy_ = 6.2f;
    plat_ = -1;
    float landY = 1e9f;
    int landI = -1;
    for (int i = 0; i < 4; i++) {
        const Plat& p = platBox_[i];
        if (px_ + 8 < p.x || px_ - 8 > p.x + p.w) continue;
        if (oldY <= p.y + 0.6f && py_ >= p.y && vy_ >= 0 && p.y < landY) {
            landY = p.y;
            landI = i;
        }
    }
    if (landI >= 0) {
        py_ = landY;
        vy_ = 0;
        plat_ = landI;
    }

    bobY_ += bobV_;
    if (bobY_ > 168) bobV_ = -1.15f;
    if (bobY_ < 78) bobV_ = 1.15f;
    bool struck = std::fabs(px_ - 104.f) < 16.f && std::fabs(py_ - bobY_) < 16.f;
    if (struck) {
        if (held_) {
            held_ = false;
            pouchX_ = 168;
            pouchY_ = 176;
        }
        vx_ = (px_ < 104.f) ? -2.8f : 2.8f;
        vy_ = -2.2f;
        plat_ = -1;
        sys_->apu.tone(1, 110, 0.16f);
    }

    if (!held_) {
        if (std::fabs(px_ - pouchX_) < 14.f && std::fabs(py_ - pouchY_) < 16.f && plat_ == 1) {
            held_ = true;
            sys_->apu.tone(0, 480, 0.16f);
        }
    } else {
        pouchX_ = px_ + face_ * 8.f;
        pouchY_ = py_ - 22.f;
    }

    if (py_ > 204.f) {
        lose("missed the water");
        return;
    }

    if (plat_ == 3) {
        if (held_ && px_ > 214 && px_ < 290) win();
        else if (!held_) lose("the lip without the pouch");
    }

    if (std::fabs(vx_) > 0.35f && plat_ >= 0) step_ += 1.f;
    else if ((int(t_) % 40) == 0 && !struck) {
        sys_->apu.tone(0, 0, 0);
    }
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 40 || s.y > gs::SCREEN_H + 40 || s.x + s.w < -40 || s.y + s.h < -40) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal, int align) {
    float width = 0;
    for (unsigned char c : s) {
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (c == ' ') width += 8.0f * scale;
        else if (c > 32 && c < 128) width += art_.glyph[c - 32].w * scale + scale;
    }
    if (align == 0) x -= width * 0.5f;
    else if (align > 0) x -= width;
    for (unsigned char c : s) {
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (c == ' ') {
            x += 8.0f * scale;
            continue;
        }
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + g.w * scale * 0.5f, y, float(g.h) * scale, pal, false, false);
        x += g.w * scale + scale;
    }
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    vdp.A.scroll(0, 0);
    vdp.B.scroll(0, int(std::sin(t_ * 0.07f) * 2));

    for (int i = 0; i < 4; i++) {
        const Plat& p = platBox_[i];
        float x = p.x;
        while (x < p.x + p.w) {
            spr(art_.lip, x + 16, p.y + 4, 10, Pal::PAL_STONE, false, false);
            x += 32;
        }
    }

    spr(art_.bucket, 104, bobY_, 16, Pal::PAL_ROPE, false, true);
    // rope up to the crown
    for (float y = 18; y < bobY_ - 8; y += 8) spr(art_.drip, 104, y, 6, Pal::PAL_ROPE, false, false);

    float dripY = std::fmod(t_ * 1.3f, 160.f);
    spr(art_.drip, 48, 18 + dripY, 9, Pal::PAL_WATER, false, false);
    spr(art_.drip, 250, 18 + std::fmod(dripY + 70, 160.f), 9, Pal::PAL_WATER, false, false);

    const gs::Mipped& body = (plat_ < 0) ? art_.stand : (int(step_ / 7) & 1) ? art_.walkA : art_.walkB;
    float hy = (mode_ == 0) ? 52.f : py_;
    float hx = (mode_ == 0) ? 58.f : px_;
    if (mode_ != 0 && std::fabs(vx_) > 0.3f) spr(body, hx, hy, 40, Pal::PAL_KEEPER, face_ < 0, true);
    else spr(art_.stand, hx, hy, 40, Pal::PAL_KEEPER, face_ < 0, true);

    float pouchDrawY = (mode_ == 0) ? 176.f : (held_ ? py_ - 24.f : pouchY_);
    float pouchDrawX = (mode_ == 0) ? 168.f : pouchX_;
    spr(art_.pouch, pouchDrawX, pouchDrawY, held_ ? 16 : 18, Pal::PAL_POUCH, false, true);
    spr(art_.watch, 292, 28, 16, Pal::PAL_HUD, false, true);

    if (mode_ == 0) {
        text("CISTERN", 160, 78, 1.35f, Pal::PAL_HUD, 0);
        text("CARRY THE POUCH ACROSS", 160, 104, 0.62f, Pal::PAL_POUCH, 0);
        hudC(16, "MISS IT AND THE WATCH IS OVER", Pal::PAL_HUD);
        hudC(18, "PRESS START", Pal::PAL_HUD);
    } else if (mode_ == 2) {
        hudC(3, "THE POUCH CROSSED", Pal::PAL_HUD);
        hudC(5, "THE CISTERN", Pal::PAL_POUCH);
    } else if (mode_ == 3) {
        hudC(3, "THE WATCH IS OVER", Pal::PAL_HUD);
        hudC(5, reason_, Pal::PAL_POUCH);
    } else {
        int sec = (watch_ + 59) / 60;
        char buf[24];
        std::snprintf(buf, sizeof(buf), "WATCH %d", sec);
        hud(1, 1, held_ ? "POUCH IN HAND" : "POUCH ON THE SHELF", Pal::PAL_HUD);
        hud(28, 1, buf, sec < 10 ? Pal::PAL_POUCH : Pal::PAL_HUD);
        hud(1, 26, "CROSS TO THE FAR LIP", Pal::PAL_HUD);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    gs::Pad& pad = sys.pad;
    bool left = pad.down(gs::BTN_LEFT) || pad.axisX < -0.35f;
    bool right = pad.down(gs::BTN_RIGHT) || pad.axisX > 0.35f;
    bool jump = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_C);
    bool start = pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A);
    if (bot_ && mode_ == 0) {
        if (++titleHold_ > 8) begin();
    }
    if (mode_ == 0) {
        if (start && !bot_) begin();
    } else if (mode_ == 1) {
        if (bot_) bot(left, right, jump);
        stepPlay(left, right, jump);
    }
    draw();
}

}  // namespace cisternpouc

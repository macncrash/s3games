#include "game/bann.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace cisternbann {
namespace {

constexpr float GRAV = 0.24f;
constexpr float JUMP = -6.2f;
constexpr float MOVE = 1.7f;
constexpr float AIR = 2.05f;

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    // floor, right step, left step, home rim on the right
    // floor, low right ledge, mid left ledge, home rim on the left
    platBox_[0] = {16, 208, 288};
    platBox_[1] = {150, 166, 150};
    platBox_[2] = {20, 112, 150};
    platBox_[3] = {20, 48, 130};
    platBox_[4] = {0, 0, 0};
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int d = y < 80 ? 1 : y < 140 ? 2 : 3;
        sys.vdp.lineBackdrop[y] = gs::rgb4(0, d, d + 2);
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
    has_ = false;
    phase_ = 0;
    px_ = 70;
    py_ = 48;
    vx_ = 0;
    vy_ = 0;
    plat_ = 3;
    face_ = 1;
    bannerX_ = 64;
    bannerY_ = 208;
    eelX_ = 240;
    eelDir_ = 1;
    t_ = 0;
    step_ = 0;
}

void Game::win() {
    mode_ = 2;
    won_ = true;
    over_ = true;
    vx_ = 0;
}

int Game::marker() const {
    if (mode_ == 0) return 0;
    if (mode_ == 2 || over_) return 4;
    if (has_ && py_ < 130) return 3;
    if (has_) return 2;
    return 1;
}

void Game::bot(bool& left, bool& right, bool& jump) {
    left = right = jump = false;
    auto go = [&](float x) {
        if (px_ < x - 3) right = true;
        else if (px_ > x + 3) left = true;
    };
    if (phase_ == 0) {
        // off the left rim, past the mid ledge, down the right shaft, to the floor
        if (plat_ == 0 || py_ > 190.f) phase_ = 1;
        else if (py_ > 140.f) left = true;
        else right = true;
    } else if (phase_ == 1) {
        go(62);
        if (has_) phase_ = 2;
    } else if (phase_ == 2) {
        if (plat_ == 1) phase_ = 3;
        else {
            go(188);
            if (px_ > 168 && py_ > 190.f) jump = true;
        }
    } else if (phase_ == 3) {
        if (plat_ == 2) phase_ = 4;
        else if (px_ > 160) left = true;
        else jump = true;
    } else if (phase_ == 4) {
        if (plat_ == 3) phase_ = 5;
        else if (px_ < 70) right = true;
        else jump = true;
    } else {
        go(90);
    }
}

void Game::stepPlay(bool left, bool right, bool jump) {
    t_ += 1.f;
    float spd = (py_ > 176.f) ? MOVE * 0.62f : (plat_ >= 0 ? MOVE : AIR);
    if (left) {
        vx_ = -spd;
        face_ = -1;
    } else if (right) {
        vx_ = spd;
        face_ = 1;
    } else {
        vx_ *= (plat_ >= 0) ? 0.5f : 0.92f;
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
    if (vy_ > 6.4f) vy_ = 6.4f;
    plat_ = -1;
    float landY = 1e9f;
    int landI = -1;
    for (int i = 0; i < 4; i++) {
        const Plat& p = platBox_[i];
        if (px_ + 9 < p.x || px_ - 9 > p.x + p.w) continue;
        if (oldY <= p.y + 0.5f && py_ >= p.y && vy_ >= 0 && p.y < landY) {
            landY = p.y;
            landI = i;
        }
    }
    if (landI >= 0) {
        py_ = landY;
        vy_ = 0;
        plat_ = landI;
    }
    if (py_ > 208) {
        py_ = 208;
        vy_ = 0;
        plat_ = 0;
    }

    eelX_ += eelDir_ * 0.85f;
    if (eelX_ > 292) eelDir_ = -1;
    if (eelX_ < 210) eelDir_ = 1;

    if (!has_) {
        if (std::fabs(px_ - bannerX_) < 16.f && std::fabs(py_ - bannerY_) < 16.f) {
            has_ = true;
            sys_->apu.tone(0, 520, 0.18f);
        }
    } else {
        bannerX_ = px_ + face_ * 6.f;
        bannerY_ = py_ - 28.f;
    }

    bool eelHit = py_ > 190 && std::fabs(px_ - eelX_) < 18.f;
    if (eelHit) {
        if (has_) {
            has_ = false;
            bannerX_ = 270;
            bannerY_ = 208;
        }
        vx_ = (px_ < eelX_) ? -3.2f : 3.2f;
        vy_ = -2.4f;
        plat_ = -1;
        sys_->apu.tone(1, 90, 0.2f);
    } else if (int(t_) % 30 == 0) {
        sys_->apu.tone(0, 0, 0);
        sys_->apu.tone(1, 0, 0);
    }

    if (std::fabs(vx_) > 0.4f && plat_ >= 0) step_ += 1.f;
    if (has_ && plat_ == 3 && px_ > 36 && px_ < 140) win();
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
    vdp.B.scroll(0, int(std::sin(t_ * 0.08f) * 2));

    for (int i = 1; i < 4; i++) {
        const Plat& p = platBox_[i];
        float x = p.x;
        while (x < p.x + p.w) {
            spr(art_.lip, x + 20, p.y + 2, 10, Pal::PAL_STONE, false, false);
            x += 36;
        }
    }
    spr(art_.eel, eelX_, 204, 12, Pal::PAL_EEL, eelDir_ < 0, true);

    float dripY = std::fmod(t_ * 1.4f, 150.f);
    spr(art_.drip, 70, 30 + dripY, 10, Pal::PAL_WATER, false, false);
    spr(art_.drip, 190, 20 + std::fmod(dripY + 60, 150.f), 10, Pal::PAL_WATER, false, false);

    const gs::Mipped& body = (plat_ < 0) ? art_.stand : (int(step_ / 7) & 1) ? art_.walkA : art_.walkB;
    if (std::fabs(vx_) < 0.3f) {
        spr(art_.stand, px_, py_, 42, Pal::PAL_KEEPER, face_ < 0, true);
    } else {
        spr(body, px_, py_, 42, Pal::PAL_KEEPER, face_ < 0, true);
    }
    spr(art_.banner, bannerX_, has_ ? py_ - 30 : bannerY_ - 4, has_ ? 28 : 32, Pal::PAL_BANNER, false, true);

    if (mode_ == 0) {
        text("CISTERN", 160, 70, 1.4f, Pal::PAL_HUD, 0);
        text("BRING THE BANNER BACK", 160, 100, 0.7f, Pal::PAL_BANNER, 0);
        hudC(16, "ONE CISTERN", Pal::PAL_HUD);
        hudC(18, "PRESS START", Pal::PAL_HUD);
    } else if (mode_ == 2) {
        hudC(2, "THE BANNER IS BACK", Pal::PAL_HUD);
    } else {
        hud(1, 1, has_ ? "BANNER IN HAND" : "BANNER BELOW", Pal::PAL_HUD);
        hud(1, 26, "BRING IT TO THE RIM", Pal::PAL_HUD);
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
        if (start) begin();
    } else if (mode_ == 1) {
        if (bot_) bot(left, right, jump);
        stepPlay(left, right, jump);
    }
    draw();
}

}  // namespace cisternbann

#include "game/harbor.h"

#include <cmath>

namespace harborladd {

namespace {
constexpr float GRAV = 0.30f;
constexpr float JUMP = -5.7f;
constexpr float RUN = 1.65f;
constexpr float CLIMB = 1.15f;
constexpr float FLOOR = 176.f;
constexpr float LADDER_X = 668.f;
constexpr float LADDER_TOP = 40.f;
constexpr float LADDER_BOT = FLOOR;
constexpr float WORLD_R = 820.f;
}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    plat_[0] = {8, FLOOR, 170};
    plat_[1] = {198, FLOOR, 140};
    plat_[2] = {358, FLOOR, 140};
    plat_[3] = {518, FLOOR, 250};
    beginRun();
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    titleT_ = 0;
    for (int y = 0; y < gs::SCREEN_H; y++) sys.vdp.road[y].on = false;
}

void Game::beginRun() {
    px_ = 36;
    py_ = FLOOR;
    vx_ = 0;
    vy_ = 0;
    faceR_ = true;
    climbing_ = false;
    grounded_ = true;
    anim_ = 0;
    cam_ = 0;
}

bool Game::onFloor(float x, float y, float* top) const {
    for (int i = 0; i < NPLAT; i++) {
        const Plat& p = plat_[i];
        if (x >= p.x + 4 && x <= p.x + p.w - 4 && y >= p.y - 3 && y <= p.y + 12) {
            if (top) *top = p.y;
            return true;
        }
    }
    return false;
}

bool Game::hitLadder(float x, float y) const {
    return x > LADDER_X - 8 && x < LADDER_X + 20 && y > LADDER_TOP + 6 && y < LADDER_BOT + 6;
}

void Game::botIntent(float& mx, bool& jump, bool& up) {
    mx = 0;
    jump = false;
    up = false;
    if (hitLadder(px_, py_) && px_ > LADDER_X - 4) {
        up = true;
        return;
    }
    mx = 1;
    bool here = onFloor(px_, py_ + 2, nullptr);
    bool there = onFloor(px_ + 20, py_ + 8, nullptr);
    if (here && !there && grounded_) jump = true;
}

void Game::miss() {
    mode_ = Mode::Over;
    over_ = true;
    won_ = false;
    climbing_ = false;
    sys_->apu.noiseBurst(0.12f, 700, 0.28f);
}

void Game::tickPlay() {
    gs::Pad& pad = sys_->pad;
    float mx = 0;
    bool jump = false;
    bool up = false;
    bool down = false;
    if (bot_) {
        botIntent(mx, jump, up);
    } else {
        if (pad.down(gs::BTN_LEFT)) mx -= 1;
        if (pad.down(gs::BTN_RIGHT)) mx += 1;
        if (std::fabs(pad.axisX) > 0.3f) mx = pad.axisX > 0 ? 1.f : -1.f;
        jump = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_C);
        up = pad.down(gs::BTN_UP) || pad.axisY > 0.4f;
        down = pad.down(gs::BTN_DOWN) || pad.axisY < -0.4f;
    }

    const bool atLadder = hitLadder(px_, py_);
    if (atLadder && (up || (down && py_ < FLOOR - 2) || climbing_)) {
        if (!up && !down && climbing_) {
            vx_ = 0;
            vy_ = 0;
        }
        if (up || down) {
            climbing_ = true;
            grounded_ = false;
            vx_ = 0;
            vy_ = up ? -CLIMB : CLIMB * 0.85f;
            px_ += (LADDER_X + 6 - px_) * 0.4f;
            py_ += vy_;
            if (py_ > FLOOR) {
                py_ = FLOOR;
                climbing_ = false;
                grounded_ = true;
            }
            if (py_ < LADDER_TOP + 18) {
                py_ = LADDER_TOP + 18;
                won_ = true;
                over_ = true;
                mode_ = Mode::Held;
                climbing_ = true;
                sys_->apu.tone(0, 392, 0.08f);
                sys_->apu.tone(1, 523, 0.07f);
                sys_->apu.tone(2, 659, 0.06f);
                return;
            }
            anim_++;
            if ((anim_ % 10) == 0) sys_->apu.tone(0, 160.f + float(anim_ % 3) * 28.f, 0.03f);
            return;
        }
    }
    if (climbing_ && !atLadder) climbing_ = false;
    if (!atLadder) climbing_ = false;

    if (mx > 0.2f) faceR_ = true;
    if (mx < -0.2f) faceR_ = false;
    vx_ = mx * RUN;
    vy_ += GRAV;
    if (vy_ > 6.4f) vy_ = 6.4f;

    px_ += vx_;
    if (px_ < 14) px_ = 14;
    if (px_ > WORLD_R) px_ = WORLD_R;

    float land = 0;
    py_ += vy_;
    grounded_ = false;
    if (vy_ >= 0 && onFloor(px_, py_, &land)) {
        py_ = land;
        vy_ = 0;
        grounded_ = true;
        if (jump) {
            vy_ = JUMP;
            grounded_ = false;
            sys_->apu.tone(0, 196, 0.05f);
        }
    }

    if (py_ > 214.f) {
        miss();
        return;
    }

    if (grounded_ && std::fabs(vx_) > 0.2f) {
        anim_++;
        if ((anim_ % 12) == 0) {
            stepSnd_ ^= 1;
            sys_->apu.tone(1, stepSnd_ ? 84.f : 64.f, 0.025f);
        }
    } else if (!grounded_) {
        anim_++;
    }
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Held || won_) return 3;
    if (mode_ == Mode::Over) return 4;
    if (px_ >= LADDER_X - 16) return 2;
    if (px_ >= 180.f) return 1;
    return 0;
}

void Game::blit(const gs::Image& img, float x, float y, int pal, bool flip) {
    if (img.w == 0) return;
    gs::Sprite s;
    s.img = img;
    s.x = int16_t(x);
    s.y = int16_t(y);
    s.w = img.w;
    s.h = img.h;
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::blitM(const gs::Mipped& m, float x, float y, float h, int pal, bool flip) {
    if (m.h <= 0 || h < 1.f) return;
    const gs::Image& img = m.pick(h);
    float sc = h / float(m.h);
    gs::Sprite s;
    s.img = img;
    s.w = int16_t(std::max(1.f, m.w * sc));
    s.h = int16_t(h);
    s.x = int16_t(x);
    s.y = int16_t(y);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::drawWorld() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    int t = int(sys_->frame);
    float focus = px_ - 130.f;
    if (focus < 0) focus = 0;
    if (focus > WORLD_R - 300.f) focus = WORLD_R - 300.f;
    cam_ = focus;

    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t c;
        if (y < 118) {
            int k = y / 18;
            c = gs::rgb4(3 + k / 3, 5 + k / 2, 9 + k / 3);
        } else if (y < 168) {
            c = gs::rgb4(4, 7, 10);
        } else {
            int wob = ((y + t / 3) / 4) & 1;
            c = wob ? gs::rgb4(2, 6, 10) : gs::rgb4(1, 5, 8);
        }
        vdp.lineBackdrop[y] = c;
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }

    auto sx = [&](float wx) { return wx - cam_; };

    if (mode_ == Mode::Title) {
        blit(art_.wordHarbor, 96, 28, PAL_HUD);
        blit(art_.wordFar, 78, 58, PAL_HUD);
        if ((t / 30) & 1) blit(art_.wordGo, 136, 92, PAL_HUD);
    } else if (mode_ == Mode::Held) {
        blit(art_.wordHeld, 78, 36, PAL_HUD);
        blit(art_.wordFar, 78, 70, PAL_HUD);
    } else if (mode_ == Mode::Over) {
        blit(art_.wordOver, 78, 48, PAL_HUD);
    }

    const gs::Mipped& body = climbing_ ? art_.climb : art_.hero[(anim_ / 8) & 1];
    blitM(body, sx(px_) - 10, py_ - 30, 32, PAL_HERO, !faceR_ && !climbing_);

    blitM(art_.lamp, sx(LADDER_X) + 2, LADDER_TOP - 16, 16, PAL_LAMP, false);
    for (float y = LADDER_TOP; y < LADDER_BOT; y += 12)
        blitM(art_.rung, sx(LADDER_X), y, 12, PAL_IRON, false);

    for (int i = 0; i < NPLAT; i++) {
        const Plat& p = plat_[i];
        for (float x = p.x; x < p.x + p.w - 8; x += 22)
            blitM(art_.plank, sx(x), p.y - 2, 12, PAL_WOOD, false);
        blitM(art_.pile, sx(p.x + 6), p.y - 2, 46, PAL_PILE, false);
        blitM(art_.pile, sx(p.x + p.w - 22), p.y - 2, 46, PAL_PILE, true);
    }

    blitM(art_.crate, sx(70), FLOOR - 16, 16, PAL_WOOD, false);
    blitM(art_.crate, sx(88), FLOOR - 16, 16, PAL_WOOD, true);
    blitM(art_.crate, sx(600), FLOOR - 16, 16, PAL_WOOD, false);

    const float buoys[3] = {182.f, 346.f, 506.f};
    for (int i = 0; i < 3; i++) {
        float by = 168.f + std::sin((t + i * 20) * 0.08f) * 3.f;
        blitM(art_.buoy, sx(buoys[i]), by, 16, PAL_BUOY, false);
    }

    for (int i = 0; i < 4; i++) {
        float gx = std::fmod(40.f + i * 180.f + t * 0.35f, WORLD_R) - cam_;
        float gy = 36.f + std::sin(t * 0.04f + i) * 6.f;
        blitM(art_.gull, gx, gy, 8, PAL_WATER, (t / 16 + i) & 1);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    sys.apu.tone(0, 0, 0);
    sys.apu.tone(1, 0, 0);
    sys.apu.tone(2, 0, 0);

    if (mode_ == Mode::Title) {
        titleT_++;
        bool go = sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A) || sys.pad.anyPressed();
        if (bot_ && titleT_ > 24) go = true;
        if (go) {
            beginRun();
            mode_ = Mode::Play;
            over_ = false;
            won_ = false;
        }
    } else if (mode_ == Mode::Play) {
        tickPlay();
    } else if (mode_ == Mode::Held) {
        if ((sys.frame % 24) == 0) sys.apu.tone(2, 523, 0.04f);
    } else if (mode_ == Mode::Over) {
        if (!bot_ && (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A))) {
            beginRun();
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
        }
    }
    drawWorld();
}

}  // namespace harborladd

#include "game/span.h"

#include <cmath>

namespace spanladd {

namespace {
constexpr float GRAV = 0.32f;
constexpr float JUMP = -5.6f;
constexpr float RUN = 1.55f;
constexpr float CLIMB = 1.15f;
constexpr float FLOOR_L = 168.f;
// The single span sits between the near quay and the far pier.
constexpr float SPAN_X = 96.f;
constexpr float SPAN_W = 128.f;
constexpr float LADDER_X = 268.f;
constexpr float LADDER_TOP = 36.f;
constexpr float LADDER_BOT = FLOOR_L;
}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    plat_[0] = {8, FLOOR_L, 72};
    plat_[1] = {SPAN_X, FLOOR_L, SPAN_W};
    plat_[2] = {240, FLOOR_L, 72};
    beginRun();
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    titleT_ = 0;
}

void Game::beginRun() {
    px_ = 28;
    py_ = FLOOR_L;
    vx_ = 0;
    vy_ = 0;
    faceR_ = true;
    climbing_ = false;
    grounded_ = true;
    anim_ = 0;
}

bool Game::onFloor(float x, float y, float* top) const {
    for (int i = 0; i < NPLAT; i++) {
        const Plat& p = plat_[i];
        if (x >= p.x + 4 && x <= p.x + p.w - 4 && y >= p.y - 2 && y <= p.y + 10) {
            if (top) *top = p.y;
            return true;
        }
    }
    return false;
}

bool Game::hitLadder(float x, float y) const {
    return x > LADDER_X - 6 && x < LADDER_X + 18 && y > LADDER_TOP + 8 && y < LADDER_BOT + 4;
}

void Game::botIntent(float& mx, bool& jump, bool& up) {
    mx = 0;
    jump = false;
    up = false;
    if (hitLadder(px_, py_) && px_ > LADDER_X - 2) {
        up = true;
        return;
    }
    mx = 1;
    float ahead = 0;
    bool here = onFloor(px_, py_ + 2, nullptr);
    bool there = onFloor(px_ + 18, py_ + 6, &ahead);
    if (here && !there && grounded_) jump = true;
    if (here && there && ahead + 1 < py_ && grounded_) jump = true;
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
        if (std::fabs(pad.axisX) > 0.3f) mx = pad.axisX > 0 ? 1 : -1;
        jump = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_C);
        up = pad.down(gs::BTN_UP) || pad.axisY > 0.4f;
        down = pad.down(gs::BTN_DOWN) || pad.axisY < -0.4f;
        if (up && grounded_ && !hitLadder(px_, py_)) jump = true;
    }

    const bool atLadder = hitLadder(px_, py_);
    if (atLadder && (up || (down && py_ < FLOOR_L - 2))) {
        climbing_ = true;
        grounded_ = false;
        vx_ = 0;
        vy_ = up ? -CLIMB : (down ? CLIMB * 0.8f : 0);
        px_ += (LADDER_X + 6 - px_) * 0.35f;
        py_ += vy_;
        if (py_ > FLOOR_L) {
            py_ = FLOOR_L;
            climbing_ = false;
            grounded_ = true;
        }
        if (py_ < LADDER_TOP + 16) {
            py_ = LADDER_TOP + 16;
            won_ = true;
            over_ = true;
            mode_ = Mode::Done;
            sys_->apu.tone(0, 523, 0.08f);
            sys_->apu.tone(1, 659, 0.06f);
            sys_->apu.tone(2, 784, 0.05f);
            return;
        }
        anim_++;
        if ((anim_ % 10) == 0) sys_->apu.tone(0, 180 + (anim_ % 3) * 30, 0.03f);
        return;
    }
    climbing_ = false;

    if (mx > 0.2f) faceR_ = true;
    if (mx < -0.2f) faceR_ = false;
    vx_ = mx * RUN;
    vy_ += GRAV;
    if (vy_ > 6.5f) vy_ = 6.5f;

    px_ += vx_;
    if (px_ < 10) px_ = 10;
    if (px_ > 308) px_ = 308;

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
            sys_->apu.tone(0, 220, 0.05f);
        }
    }

    if (py_ > 230) {
        beginRun();
        sys_->apu.noiseBurst(0.08f, 800, 0.2f);
        return;
    }

    if (grounded_ && std::fabs(vx_) > 0.2f) {
        anim_++;
        if ((anim_ % 12) == 0) {
            stepSnd_ ^= 1;
            sys_->apu.tone(1, stepSnd_ ? 90.f : 70.f, 0.025f);
        }
    } else if (!grounded_) {
        anim_++;
    }
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Done || won_) return 3;
    if (px_ >= LADDER_X - 8) return 2;
    if (px_ >= SPAN_X) return 1;
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
    if (m.h <= 0) return;
    const gs::Image& img = m.pick(h);
    float sc = h / float(m.h);
    gs::Sprite s;
    s.img = img;
    s.w = int16_t(m.w * sc);
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

    for (int i = 0; i < 8; i++) {
        float wx = float((i * 48 + t / 2) % 360) - 20;
        float wy = 176 + (i % 3) * 6;
        blitM(art_.gull, wx, 150, 6, PAL_WATER, false);
        (void)wy;
        gs::Sprite rip;
        rip.img = art_.gull.pick(6);
        rip.x = int16_t(wx);
        rip.y = int16_t(wy);
        rip.w = 18;
        rip.h = 3;
        rip.pal = PAL_WATER;
        rip.fog = 4;
        vdp.sprite(rip);
    }

    blitM(art_.pier, plat_[0].x - 4, plat_[0].y - 8, 56, PAL_STONE, false);
    blitM(art_.pier, plat_[2].x - 2, plat_[2].y - 20, 72, PAL_STONE, true);

    for (float x = SPAN_X; x < SPAN_X + SPAN_W - 4; x += 26) {
        blitM(art_.plank, x, FLOOR_L - 2, 14, PAL_WOOD, false);
        blitM(art_.rail, x + 4, FLOOR_L - 28, 22, PAL_IRON, false);
        blitM(art_.rail, x + 18, FLOOR_L - 28, 22, PAL_IRON, false);
    }

    for (float y = LADDER_TOP; y < LADDER_BOT; y += 12)
        blitM(art_.rung, LADDER_X, y, 12, PAL_IRON, false);
    blitM(art_.flag, LADDER_X + 10, LADDER_TOP - 8, 16, PAL_FLAG, (t / 12) & 1);

    float gullx = 40 + std::sin(t * 0.02f) * 30;
    blitM(art_.gull, gullx, 28 + std::sin(t * 0.05f) * 4, 10, PAL_SKY, (t / 20) & 1);

    const gs::Mipped& body = climbing_ ? art_.climb : art_.hero[(anim_ / 8) & 1];
    blitM(body, px_ - 10, py_ - 28, 30, PAL_HERO, !faceR_ && !climbing_);

    if (mode_ == Mode::Title) {
        blit(art_.wordSpan, 78, 36, PAL_HUD);
        blit(art_.wordFar, 70, 64, PAL_HUD);
        blit(art_.wordHint, 96, 96, PAL_HUD);
        if ((t / 30) & 1) blit(art_.wordGo, 136, 120, PAL_HUD);
    } else if (mode_ == Mode::Done) {
        blit(art_.wordDone, 108, 48, PAL_HUD);
        blit(art_.wordFar, 70, 90, PAL_HUD);
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
    } else if (mode_ == Mode::Done) {
        if ((sys.frame % 20) == 0) sys.apu.tone(2, 660, 0.04f);
        if (!bot_ && (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A))) {
            beginRun();
            mode_ = Mode::Play;
            over_ = false;
            won_ = false;
        }
    }
    drawWorld();
}

}  // namespace spanladd

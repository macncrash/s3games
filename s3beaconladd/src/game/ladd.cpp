#include "game/ladd.h"

#include <cmath>

namespace beaconladd {

namespace {
constexpr float GRAV = 0.28f;
constexpr float JUMP = -6.2f;
constexpr float RUN = 1.6f;
constexpr float CLIMB = 1.25f;
constexpr float LOW = 180.f;
constexpr float HIGH = 118.f;
constexpr float LADDER_X = 292.f;
constexpr float LADDER_TOP = 28.f;
constexpr float LADDER_BOT = HIGH;
}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    plat_[0] = {0, LOW, 78};
    plat_[1] = {96, LOW, 74};
    plat_[2] = {146, HIGH, 100};
    plat_[3] = {262, HIGH, 58};
    beginRun();
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    titleT_ = 0;
}

void Game::beginRun() {
    px_ = 28;
    py_ = LOW;
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
    return x > LADDER_X - 8 && x < LADDER_X + 20 && y > LADDER_TOP + 8 && y < LADDER_BOT + 6;
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
    if (!grounded_) return;
    float aheadTop = 0;
    bool ahead = onFloor(px_ + 16, py_ + 2, &aheadTop);
    if (!ahead) jump = true;
    else if (aheadTop + 8 < py_) jump = true;
    if (py_ > 150 && px_ > 128) jump = true;
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
        if (up && grounded_ && !hitLadder(px_, py_)) jump = true;
    }

    const bool atLadder = hitLadder(px_, py_);
    if (atLadder && (up || (down && py_ < LADDER_BOT - 2))) {
        climbing_ = true;
        grounded_ = false;
        vx_ = 0;
        vy_ = up ? -CLIMB : (down ? CLIMB * 0.8f : 0);
        px_ += (LADDER_X + 6 - px_) * 0.35f;
        py_ += vy_;
        if (py_ > LADDER_BOT) {
            py_ = LADDER_BOT;
            climbing_ = false;
            grounded_ = true;
        }
        if (py_ < LADDER_TOP + 16) {
            py_ = LADDER_TOP + 16;
            won_ = true;
            over_ = true;
            mode_ = Mode::Done;
            sys_->apu.tone(0, 392, 0.08f);
            sys_->apu.tone(1, 523, 0.06f);
            sys_->apu.tone(2, 784, 0.05f);
            return;
        }
        anim_++;
        if ((anim_ % 10) == 0) sys_->apu.tone(0, 160 + (anim_ % 3) * 24, 0.03f);
        return;
    }
    climbing_ = false;

    if (mx > 0.2f) faceR_ = true;
    if (mx < -0.2f) faceR_ = false;
    vx_ = mx * RUN;
    vy_ += GRAV;
    if (vy_ > 6.8f) vy_ = 6.8f;

    px_ += vx_;
    if (px_ < 8) px_ = 8;
    if (px_ > 310) px_ = 310;

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

    if (py_ > 230) {
        beginRun();
        sys_->apu.noiseBurst(0.08f, 700, 0.18f);
        return;
    }

    if (grounded_ && std::fabs(vx_) > 0.2f) {
        anim_++;
        if ((anim_ % 12) == 0) {
            stepSnd_ ^= 1;
            sys_->apu.tone(1, stepSnd_ ? 84.f : 62.f, 0.025f);
        }
    } else if (!grounded_) {
        anim_++;
    }
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Done || won_) return 3;
    if (px_ >= LADDER_X - 10) return 2;
    if (px_ >= 90) return 1;
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

    if (mode_ == Mode::Title) {
        blit(art_.wordOne, 72, 28, PAL_HUD);
        blit(art_.wordFar, 78, 56, PAL_HUD);
        blit(art_.wordHint, 86, 86, PAL_HUD);
        if ((t / 30) & 1) blit(art_.wordGo, 140, 108, PAL_HUD);
    } else if (mode_ == Mode::Done) {
        blit(art_.wordDone, 112, 36, PAL_HUD);
        blit(art_.wordFar, 78, 84, PAL_HUD);
    }

    const gs::Mipped& body = climbing_ ? art_.climb : art_.keeper[(anim_ / 8) & 1];
    blitM(body, px_ - 10, py_ - 30, 32, PAL_KEEPER, !faceR_ && !climbing_);

    blitM(art_.beacon, 18, LOW - 96, 96, PAL_STONE, false);
    int beam = (t / 18) % 3;
    blitM(art_.beam[beam], 36, LOW - 84, 16, PAL_LAMP, false);
    blitM(art_.lamp, 28, LOW - 78, 10, PAL_LAMP, false);

    for (int i = 0; i < NPLAT; i++) {
        const Plat& p = plat_[i];
        float h = (i < 2) ? 36.f : 48.f;
        for (float x = p.x; x < p.x + p.w - 2; x += 28)
            blitM(art_.cliff, x, p.y - 4, h, PAL_STONE, (i & 1) != 0);
    }

    for (float y = LADDER_TOP; y < LADDER_BOT; y += 12)
        blitM(art_.rung, LADDER_X, y, 12, PAL_IRON, false);
    blitM(art_.lamp, LADDER_X + 8, LADDER_TOP - 12, 12, PAL_LAMP, (t / 10) & 1);

    static const int sx[8] = {20, 70, 120, 180, 230, 280, 40, 300};
    static const int sy[8] = {16, 34, 22, 48, 18, 40, 60, 28};
    for (int i = 0; i < 8; i++) {
        int tw = 4 + ((t / 12 + i) & 1);
        blitM(art_.star, float(sx[i]), float(sy[i]), float(tw), PAL_NIGHT, false);
    }

    for (int i = 0; i < 6; i++) {
        float wx = float((i * 58 + t / 3) % 360) - 16;
        blitM(art_.star, wx, 196, 3, PAL_SEA, false);
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
        if ((sys.frame % 20) == 0) sys.apu.tone(2, 523, 0.04f);
        if (!bot_ && (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A))) {
            beginRun();
            mode_ = Mode::Play;
            over_ = false;
            won_ = false;
        }
    }
    drawWorld();
}

}  // namespace beaconladd

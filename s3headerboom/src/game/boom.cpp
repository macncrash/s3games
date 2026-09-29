#include "game/boom.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace headerboom {

namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr float G = 14.0f;
constexpr float BOW0 = 16.0f;
constexpr float HOLD = 78.4f;     // bow x that noses the plank without striking it
constexpr float STRIKE = 78.72f;  // any further and the stem meets the boom
constexpr float DECK_Y = 1.62f;
constexpr float DRIVE_HW = 0.42f;
constexpr float DRIVE_HH = 0.28f;
constexpr float PLANK0 = 78.85f;
constexpr float PLANK1 = 90.4f;
constexpr float BED0 = 80.15f;
constexpr float BED1 = 88.7f;
constexpr float GAP_OK = 0.38f;
constexpr float SHOVE = 3.55f;
constexpr float SLIDE = 1.15f;  // friction while the drive is supported
constexpr float VMAX = 5.4f;
constexpr float ACCEL = 3.1f;
constexpr float BRAKE = 4.8f;
constexpr float WATER = 150;  // screen row where the cut begins

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

void Game::blip(float freq) { sys_->apu.tone(1, freq, 0.1f); }

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.HUD.enabled = false;
    sys.apu.setMaster(0.55f);
    mode_ = Mode::Title;
    banner_ = "DELIVER THE DRIVE";
    if (bot_) begin();
}

void Game::begin() {
    over_ = false;
    won_ = false;
    released_ = false;
    supported_ = true;
    t_ = 0;
    bow_ = BOW0;
    bv_ = 0;
    dx_ = bow_ - 1.35f;
    dy_ = DECK_Y;
    dvx_ = 0;
    dvy_ = 0;
    still_ = 0;
    flash_ = 0;
    why_ = "";
    banner_ = "";
    mode_ = Mode::Run;
    sys_->setLight(40, 90, 140);
    blip(220);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    why_ = why;
    banner_ = why;
    won_ = false;
    over_ = true;
    mode_ = Mode::Fail;
    flash_ = 0.45f;
    sys_->rumble(0.45f, 0.2f, 160);
    sys_->setLight(140, 30, 20);
    sys_->apu.noiseBurst(0.4f, 120.f, 0.28f);
}

void Game::succeed() {
    if (mode_ != Mode::Run) return;
    why_ = "the drive is on the boom";
    banner_ = "ON THE BOOM";
    won_ = true;
    over_ = true;
    mode_ = Mode::Win;
    sys_->rumble(0.08f, 0.04f, 120);
    sys_->setLight(40, 140, 70);
    blip(660);
}

void Game::botInput(float& thrust, bool& release) {
    thrust = 0;
    release = false;
    if (released_) return;
    float dist = HOLD - bow_;
    float stop = (bv_ * bv_) / (2.f * BRAKE);
    if (dist > stop + 0.42f) thrust = 1.f;
    else if (dist > 0.06f) thrust = bv_ > 0.35f ? -1.f : 0.35f;
    else thrust = bv_ > 0.05f ? -1.f : 0.f;
    if (std::fabs(bow_ - HOLD) < 0.07f && std::fabs(bv_) < 0.1f) release = true;
}

void Game::stepRun() {
    float thrust = 0;
    bool release = false;
    if (bot_) {
        botInput(thrust, release);
    } else {
        const gs::Pad& pad = sys_->pad;
        if (pad.down(gs::BTN_RIGHT) || pad.accel > 0.2f) thrust += 1.f;
        if (pad.down(gs::BTN_LEFT) || pad.brake > 0.2f) thrust -= 1.f;
        if (std::fabs(pad.axisX) > 0.2f) thrust = pad.axisX;
        thrust = clampf(thrust, -1.f, 1.f);
        if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_Y))
            release = true;
    }

    float rate = thrust >= 0 ? ACCEL : BRAKE;
    float want = thrust > 0 ? thrust * VMAX : 0.f;
    if (thrust < 0) want = 0.f;
    if (thrust < 0) bv_ = std::max(0.f, bv_ - BRAKE * DT);
    else bv_ += (want - bv_) * std::min(1.f, rate * DT * 0.55f);
    if (thrust == 0) bv_ = std::max(0.f, bv_ - 0.8f * DT);
    bow_ += bv_ * DT;
    if (bow_ >= STRIKE) {
        bow_ = STRIKE;
        bv_ = 0;
        fail("struck the boom");
        return;
    }

    if (!released_) {
        dx_ = bow_ - 1.35f;
        dy_ = DECK_Y;
        dvx_ = bv_;
        dvy_ = 0;
        if (release && t_ > 0.3f) {
            released_ = true;
            dvx_ = bv_ + SHOVE;
            blip(180);
        }
    } else {
        float gap = PLANK0 - bow_;
        bool onDeck = dx_ >= bow_ - 6.4f && dx_ <= bow_ + 0.02f;
        bool onPlank = dx_ >= PLANK0 && dx_ <= PLANK1;
        bool onGap = dx_ > bow_ && dx_ < PLANK0 && gap <= GAP_OK && gap >= -0.02f;
        supported_ = onDeck || onPlank || onGap;
        if (supported_) {
            dvy_ = 0;
            dy_ = DECK_Y;
            float s = dvx_ > 0 ? 1.f : dvx_ < 0 ? -1.f : 0.f;
            dvx_ -= s * SLIDE * DT;
            if (std::fabs(dvx_) < SLIDE * DT) dvx_ = 0;
            // The header's deck carries the crate until the bow is behind it.
            if (onDeck && !onPlank) dvx_ = std::max(dvx_, bv_);
        } else {
            dvy_ -= G * DT;
            dy_ += dvy_ * DT;
        }
        dx_ += dvx_ * DT;

        if (dy_ - DRIVE_HH < 0.05f) {
            if (dx_ < PLANK0) fail("dropped short");
            else fail("dropped long");
            return;
        }
        if (dx_ - DRIVE_HW > PLANK1) {
            fail("carried off the boom");
            return;
        }
        if (supported_ && onPlank && std::fabs(dvx_) < 0.12f && std::fabs(bv_) < 1.2f) {
            still_ += DT;
            bool inBed = dx_ - DRIVE_HW >= BED0 && dx_ + DRIVE_HW <= BED1;
            bool edge = dx_ + DRIVE_HW > PLANK0 && dx_ - DRIVE_HW < PLANK1 && !inBed;
            if (still_ > 0.35f && inBed) {
                succeed();
                return;
            }
            if (still_ > 0.35f && edge) {
                fail("on the edge of the boom");
                return;
            }
        } else if (!(supported_ && onPlank)) {
            still_ = 0;
        }
    }

    if (t_ > 32.f) fail("too late");
}

void Game::paintSky() {
    gs::VDP& vdp = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < WATER) {
            int band = y * 3 / WATER;
            uint16_t c = band == 0 ? gs::rgb4(6, 10, 14) : band == 1 ? gs::rgb4(8, 12, 15) : gs::rgb4(11, 14, 15);
            vdp.lineBackdrop[y] = c;
        } else {
            int shimmer = ((y + int(t_ * 18) + (y / 3)) & 3);
            vdp.lineBackdrop[y] = shimmer == 0 ? gs::rgb4(2, 6, 10) : gs::rgb4(2, 7, 11);
        }
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }
}

void Game::spr(const gs::Mipped& m, float wx, float wy, float destH, int pal, bool flip) {
    if (destH < 2.f || m.h < 1) return;
    float cam = bow_ - 6.5f;
    float ppm = 9.1f;
    float sx = 36.f + (wx - cam) * ppm;
    float sy = float(WATER) - wy * ppm;
    gs::Sprite s;
    s.img = m.pick(destH);
    s.h = std::max(1, int(destH));
    s.w = std::max(1, int(m.w * destH / float(m.h)));
    s.x = int(sx - s.w * 0.5f);
    s.y = int(sy - s.h * 0.5f);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    if (s.x > gs::SCREEN_W || s.y > gs::SCREEN_H || s.x + s.w < 0 || s.y + s.h < 0) return;
    sys_->vdp.sprite(s);
}

void Game::text(const char* s, float x, float y, float scale, int pal) {
    float cx = x;
    for (; *s; ++s) {
        unsigned ch = unsigned(*s);
        int i = (ch < 32 || ch > 127) ? 0 : int(ch) - 32;
        gs::Sprite sp;
        sp.img = art_.glyph[i];
        sp.w = std::max(1, int(5 * scale));
        sp.h = std::max(1, int(7 * scale));
        sp.x = int(cx);
        sp.y = int(y);
        sp.pal = uint8_t(pal);
        sys_->vdp.sprite(sp);
        cx += 6.f * scale;
    }
}

void Game::paintWorld() {
    sys_->vdp.clearSprites();
    if (mode_ == Mode::Title) {
        text("S3 HEADER BOOM", 62, 28, 2, PAL_INK);
        text("DELIVER THE DRIVE", 70, 52, 1, PAL_GOLD);
        text("NOSE UP   A LETS GO", 58, 70, 1, PAL_INK);
        text("PRESS START", 100, 92, 1, PAL_GOOD);
    } else {
        char buf[48];
        std::snprintf(buf, sizeof buf, released_ ? "DRIVE AWAY" : "DRIVE ABOARD");
        text(buf, 8, 8, 1, PAL_INK);
        std::snprintf(buf, sizeof buf, "BOW %d", int(bow_));
        text(buf, 230, 8, 1, PAL_GOLD);
        if (banner_ && banner_[0]) {
            int pal = mode_ == Mode::Win ? PAL_GOOD : mode_ == Mode::Fail ? PAL_BAD : PAL_INK;
            text(banner_, 70, 28, 1, pal);
        }
    }

    float cam = (mode_ == Mode::Title) ? BOW0 - 6.5f : bow_ - 6.5f;
    (void)cam;
    spr(art_.cloud, bow_ - 2.f, 8.4f, 18, PAL_SKY);
    spr(art_.cloud, bow_ + 10.f, 9.2f, 14, PAL_SKY);
    spr(art_.mill, 8.f, 3.6f, 40, PAL_MILL);
    spr(art_.willow, 70.f, 3.2f, 46, PAL_BANK);
    spr(art_.willow, 96.f, 3.0f, 42, PAL_BANK, true);
    for (int i = 0; i < 7; i++) spr(art_.reed, 4.f + i * 3.1f, 1.15f, 22, PAL_BANK);
    for (int i = 0; i < 4; i++) spr(art_.reed, 92.f + i * 2.4f, 1.1f, 20, PAL_BANK);

    float flap = std::sin(t_ * 6.f) * 4.f;
    spr(art_.bird, bow_ + 4.f + flap, 7.2f, 8, PAL_INK);

    // Timber boom: two posts and the receiving plank. The bed is the pale run.
    spr(art_.post, PLANK0 + 0.35f, 1.15f, 36, PAL_BOOM);
    spr(art_.post, PLANK1 - 0.35f, 1.15f, 36, PAL_BOOM);
    spr(art_.plank, (PLANK0 + PLANK1) * 0.5f, DECK_Y + 0.02f, 12, PAL_BOOM);

    float hullX = bow_ - 3.5f;
    spr(art_.hull, hullX, 0.95f, 30, PAL_HULL);
    spr(art_.cabin, bow_ - 4.6f, 1.85f, 24, PAL_HULL);
    spr(art_.sail, bow_ - 5.8f, 2.7f, 40, PAL_HULL);
    spr(art_.drive, dx_, dy_ + 0.05f, 16, PAL_DRIVE);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C))) begin();
    } else if (mode_ == Mode::Run) {
        t_ += DT;
        stepRun();
    } else {
        flash_ = std::max(0.f, flash_ - DT);
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) begin();
    }
    if (mode_ == Mode::Title) {
        bow_ = BOW0;
        dx_ = bow_ - 1.35f;
        dy_ = DECK_Y;
    }
    paintSky();
    paintWorld();
}

}  // namespace headerboom

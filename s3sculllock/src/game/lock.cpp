#include "game/lock.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace scull {

namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr float HULL = 0.18f;
constexpr float BANK = 1.15f;
constexpr int HORIZON = 84;
constexpr int NLEGS = 4;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

void Game::begin() {
    over_ = false;
    won_ = false;
    lives_ = 3;
    legsDone_ = 0;
    leg_ = 0;
    note_ = "";
    flash_ = 0;
    // Four locks. The gate is not on the end, so holding the gate line fails the leg.
    const float gc[NLEGS] = {0.10f, -0.55f, 0.60f, -0.30f};
    const float ec[NLEGS] = {-0.42f, 0.38f, -0.22f, 0.50f};
    for (int i = 0; i < NLEGS; i++) {
        float base = i * 64.0f;
        legs_[i].gateZ = base + 28.0f;
        legs_[i].gateC = gc[i];
        legs_[i].gateH = 0.62f;
        legs_[i].endZ = base + 48.0f;
        legs_[i].endC = ec[i];
        legs_[i].endH = 0.46f;
        legs_[i].doneZ = base + 58.0f;
    }
    startLeg();
    mode_ = Mode::Row;
}

void Game::startLeg() {
    const Leg& L = legs_[leg_];
    pz_ = L.gateZ - 28.0f;
    x_ = 0;
    vx_ = 0;
    phase_ = 0;
    gateTaken_ = false;
    endTaken_ = false;
    hold_ = 0;
}

void Game::blip(float freq) { sys_->apu.tone(1, freq, 0.12f); }

void Game::miss(const char* why) {
    note_ = why;
    flash_ = 0.45f;
    lives_--;
    blip(90);
    if (lives_ <= 0) {
        mode_ = Mode::Lose;
        over_ = true;
        won_ = false;
        hold_ = 0;
    } else {
        mode_ = Mode::Miss;
        hold_ = 1.1f;
    }
}

void Game::botStick(float& steer, float& power) const {
    const Leg& L = legs_[leg_];
    float aim = (!gateTaken_ && pz_ < L.gateZ) ? L.gateC : L.endC;
    float err = aim - x_;
    steer = clampf(err * 3.2f - vx_ * 0.45f, -1.0f, 1.0f);
    power = 1.0f;
}

void Game::row(float dt) {
    const Leg& L = legs_[leg_];
    float steer = 0, power = 0.35f;
    if (bot_) {
        botStick(steer, power);
    } else {
        const gs::Pad& pad = sys_->pad;
        if (pad.down(gs::BTN_LEFT)) steer -= 1;
        if (pad.down(gs::BTN_RIGHT)) steer += 1;
        if (std::fabs(pad.axisX) > 0.15f) steer = pad.axisX;
        steer = clampf(steer, -1.0f, 1.0f);
        if (pad.down(gs::BTN_A) || pad.down(gs::BTN_UP) || pad.accel > 0.2f) power = 1.0f;
        if (pad.down(gs::BTN_DOWN) || pad.brake > 0.2f) power = 0.05f;
    }
    float des = steer * 1.55f;
    vx_ += (des - vx_) * std::min(1.0f, dt * 7.0f);
    x_ += vx_ * dt;
    if (x_ > BANK) {
        x_ = BANK;
        vx_ = 0;
    }
    if (x_ < -BANK) {
        x_ = -BANK;
        vx_ = 0;
    }
    float spd = 6.4f + power * 3.4f;
    float z0 = pz_;
    pz_ += spd * dt;
    phase_ += dt * (1.6f + power * 1.4f);

    if (!gateTaken_ && z0 < L.gateZ && pz_ >= L.gateZ) {
        gateTaken_ = true;
        if (std::fabs(x_ - L.gateC) + HULL > L.gateH) {
            miss("SCRAPED THE GATE");
            return;
        }
        blip(330);
    }
    if (!endTaken_ && z0 < L.endZ && pz_ >= L.endZ) {
        endTaken_ = true;
        if (std::fabs(x_ - L.endC) > L.endH) {
            miss("MISSED THE END");
            return;
        }
        blip(440);
    }
    if (pz_ >= L.doneZ) {
        legsDone_ = leg_ + 1;
        if (leg_ + 1 >= NLEGS) {
            mode_ = Mode::Win;
            over_ = true;
            won_ = true;
            note_ = "LOCK CLEAR";
            blip(660);
        } else {
            leg_++;
            startLeg();
            note_ = "LEG MADE";
            hold_ = 0.6f;
        }
    }
}

void Game::project(float wx, float wz, float& sx, float& sy, float& ppm) const {
    float d = wz - pz_;
    if (d < 0.8f) d = 0.8f;
    float n = 1.0f - std::sqrt(clampf((d - 1.5f) / 40.0f, 0.0f, 1.0f));
    sy = float(HORIZON) + n * float(gs::SCREEN_H - 8 - HORIZON);
    ppm = 168.0f / d;
    sx = 160.0f + (wx - x_) * ppm;
}

void Game::spr(const gs::Mipped& m, float cx, float footY, float destH, int pal, bool flip) {
    if (destH < 2.0f) return;
    gs::Sprite s;
    s.img = m.pick(destH);
    s.h = std::max(1, int(destH));
    s.w = std::max(1, int(m.w * destH / float(m.h)));
    s.x = int(cx - s.w * 0.5f);
    s.y = int(footY - s.h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::glyphText(const char* s, int x, int y, int scale, int pal) {
    int cw = 6 * scale;
    int ch = 8 * scale;
    for (const char* p = s; *p; p++) {
        unsigned c = unsigned(*p);
        if (c < 32 || c > 127) c = '?';
        gs::Sprite sp;
        sp.img = art_.glyph[c - 32];
        sp.x = int16_t(x);
        sp.y = int16_t(y);
        sp.w = int16_t(5 * scale);
        sp.h = int16_t(7 * scale);
        sp.pal = uint8_t(pal);
        sys_->vdp.sprite(sp);
        x += cw;
        (void)ch;
    }
}

void Game::paintSky() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(HORIZON);
        if (y < HORIZON) {
            int r = int(5 + 6 * u);
            int g = int(7 + 5 * u);
            int b = int(11 + 3 * u);
            v.lineBackdrop[y] = gs::rgb4(r, g, b);
            v.lineFog[y] = 0;
            v.road[y].on = false;
        } else {
            v.lineBackdrop[y] = gs::rgb4(3, 6, 3);
            float n = (y - HORIZON) / float(gs::SCREEN_H - HORIZON);
            v.lineFog[y] = uint8_t(clampf((1.0f - n) * 7.0f, 0.0f, 8.0f));
        }
    }
}

void Game::paintWater() {
    gs::VDP& v = sys_->vdp;
    v.roadTime = int(t_ * 40);
    for (int y = HORIZON; y < gs::SCREEN_H; y++) {
        float n = (y - HORIZON) / float(gs::SCREEN_H - 1 - HORIZON);
        float d = 1.5f + 40.0f * (1.0f - n) * (1.0f - n);
        float ppm = 168.0f / d;
        gs::RoadLine& r = v.road[y];
        r.on = true;
        r.cx = 160.0f - x_ * ppm;
        r.hw = 1.42f * ppm;
        r.v = pz_ * 18.0f + d * 22.0f;
        r.pal = PAL_WATER;
        r.band = (int(pz_ * 2 + d) & 3) == 0 ? 1 : 0;
        r.style = 2;
        r.left = gs::GROUND_LAND;
        r.right = gs::GROUND_LAND;
    }
}

void Game::paintWorld() {
    // Far bank dressing, then the lock, then the end posts. Nearer sprites go on later
    // only after the far ones: earlier sprites sit on top, so the call order is near first.
    struct Item {
        float z;
        int kind;
        float wx;
        bool flip;
    };
    Item items[48];
    int n = 0;
    auto push = [&](float z, int kind, float wx, bool flip) {
        if (n < 48 && z > pz_ + 0.9f && z < pz_ + 42.0f) items[n++] = {z, kind, wx, flip};
    };
    for (int i = 0; i < 10; i++) {
        float z = std::floor(pz_ / 8.0f) * 8.0f + i * 8.0f;
        push(z, 3, -2.35f, false);
        push(z + 3.0f, 4, 2.25f, false);
    }
    for (int i = 0; i < NLEGS; i++) {
        const Leg& L = legs_[i];
        push(L.gateZ, 2, -1.55f, false);
        push(L.gateZ, 2, 1.55f, true);
        float left = ( -1.42f + (L.gateC - L.gateH)) * 0.5f;
        float right = ((L.gateC + L.gateH) + 1.42f) * 0.5f;
        push(L.gateZ - 0.2f, 1, left, false);
        push(L.gateZ - 0.2f, 1, right, true);
        push(L.endZ, 0, L.endC - L.endH, false);
        push(L.endZ, 0, L.endC + L.endH, false);
    }
    std::sort(items, items + n, [](const Item& a, const Item& b) { return a.z < b.z; });
    for (int i = 0; i < n; i++) {
        float sx, sy, ppm;
        project(items[i].wx, items[i].z, sx, sy, ppm);
        if (sx < -40 || sx > 360) continue;
        if (items[i].kind == 0) spr(art_.post, sx, sy, 2.3f * ppm, PAL_BUOY, false);
        else if (items[i].kind == 1) {
            float span = std::fabs(items[i].wx) < 0.2f ? 0.7f : std::min(1.3f, 0.5f + std::fabs(items[i].wx));
            // Width follows the leaf's reach across the lock mouth.
            (void)span;
            spr(art_.leaf, sx, sy + 0.15f * ppm, 3.5f * ppm, PAL_WOOD, items[i].flip);
        } else if (items[i].kind == 2)
            spr(art_.pier, sx, sy, 3.1f * ppm, PAL_STONE, items[i].flip);
        else if (items[i].kind == 3)
            spr(art_.tree, sx, sy, 3.4f * ppm, PAL_BANK, false);
        else
            spr(art_.reed, sx, sy, 1.3f * ppm, PAL_BANK, false);
    }
}

void Game::paintHull() {
    float sway = std::sin(phase_ * 6.2832f) * 3.0f;
    float foot = 198.0f;
    float bx = 160.0f + sway * 0.15f;
    int fr = int(std::fmod(phase_, 1.0f) * 5.0f);
    if (fr < 0) fr = 0;
    if (fr > 4) fr = 4;
    float lift = (fr - 2) * 3.0f;
    spr(art_.oar[fr], bx - 36, 176 + lift, 16, PAL_OAR, false);
    spr(art_.oar[4 - fr], bx + 36, 176 - lift, 16, PAL_OAR, true);
    spr(art_.hull, bx, foot, 52, PAL_HULL, false);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = false;
    sys.apu.setMaster(0.8f);
    sys.apu.silence();
    if (bot_) begin();
    else mode_ = Mode::Title;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        pz_ = 8.0f + std::sin(t_ * 0.2f) * 0.2f;
        x_ = std::sin(t_ * 0.35f) * 0.25f;
        phase_ += DT;
        if (pad.pressed(gs::BTN_START)) {
            begin();
            blip(520);
        } else if (pad.pressed(gs::BTN_MODE)) {
            sys.quit();
        }
    } else if (mode_ == Mode::Row) {
        if (hold_ > 0) hold_ -= DT;
        else row(DT);
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Title;
            blip(200);
        }
    } else if (mode_ == Mode::Miss) {
        hold_ -= DT;
        phase_ += DT * 0.4f;
        if (hold_ <= 0) {
            startLeg();
            mode_ = Mode::Row;
        }
    } else if (mode_ == Mode::Win || mode_ == Mode::Lose) {
        phase_ += DT * 0.5f;
        if (!bot_ && pad.pressed(gs::BTN_START)) begin();
    }

    if (flash_ > 0) flash_ -= DT;
    sys.vdp.clearSprites();
    paintSky();
    if (flash_ > 0) {
        for (int y = 0; y < gs::SCREEN_H; y++) sys.vdp.lineBackdrop[y] = gs::rgb4(8, 2, 2);
    }
    paintWater();
    paintWorld();
    if (mode_ != Mode::Title) paintHull();
    else paintHull();

    // Text is issued first so it sits above the scull. clearSprites already ran,
    // so these are the earliest sprites.
    // Re-add is not possible after the world. Draw text by adding now: later sprites
    // are underneath. That would hide the HUD. Add HUD before the world instead.
    // The calls above already filled the list. HUD is drawn into a second pass by
    // prepending: we rebuild, HUD first.
    sys.vdp.clearSprites();
    char buf[48];
    if (mode_ == Mode::Title) {
        glyphText("SCULL LOCK", 78, 28, 2, PAL_GOLD);
        glyphText("PASS THE GATE", 100, 52, 1, PAL_INK);
        glyphText("MAKE THE END OF THE LEG", 70, 66, 1, PAL_INK);
        glyphText("LEFT RIGHT STEER", 94, 150, 1, PAL_INK);
        glyphText("A STROKE    START ROW", 76, 164, 1, PAL_GOLD);
    } else {
        std::snprintf(buf, sizeof(buf), "LEG %d/%d", std::min(leg_ + 1, NLEGS), NLEGS);
        glyphText(buf, 8, 6, 1, PAL_INK);
        std::snprintf(buf, sizeof(buf), "LIVES %d", lives_);
        glyphText(buf, 230, 6, 1, PAL_INK);
        if (mode_ == Mode::Win) {
            glyphText("LOCK CLEAR", 86, 40, 2, PAL_GOLD);
            glyphText("EVERY LEG MADE", 94, 64, 1, PAL_INK);
        } else if (mode_ == Mode::Lose) {
            glyphText("LEG FAILED", 88, 40, 2, PAL_GOLD);
            glyphText(note_, 100, 64, 1, PAL_INK);
        } else if (note_[0] && (mode_ == Mode::Miss || hold_ > 0)) {
            glyphText(note_, 96, 40, 1, PAL_GOLD);
        } else if (mode_ == Mode::Row) {
            if (!gateTaken_) glyphText("GATE AHEAD", 112, 20, 1, PAL_GOLD);
            else if (!endTaken_) glyphText("MAKE THE END", 104, 20, 1, PAL_GOLD);
        }
    }
    paintWorld();
    paintHull();

    if (mode_ == Mode::Row || mode_ == Mode::Title) {
        float drip = std::sin(t_ * 3.0f);
        if (drip > 0.95f) sys.apu.tone(0, 140.0f + x_ * 10.0f, 0.03f);
    }
}

}  // namespace scull

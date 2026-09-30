#include "game/lock.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace kartlock {

namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr float BODY = 0.18f;
constexpr float BANK = 1.18f;
constexpr int HORIZON = 74;
constexpr int NLEGS = 3;

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
    // The gate mouth and the end of the leg are not the same line.
    const float gc[NLEGS] = {0.22f, -0.52f, 0.36f};
    const float ec[NLEGS] = {-0.28f, 0.34f, -0.12f};
    for (int i = 0; i < NLEGS; i++) {
        float base = i * 78.0f;
        legs_[i].gateZ = base + 28.0f;
        legs_[i].gateC = gc[i];
        legs_[i].gateH = 0.62f;
        legs_[i].endZ = base + 50.0f;
        legs_[i].endC = ec[i];
        legs_[i].endH = 0.46f;
        legs_[i].doneZ = base + 60.0f;
    }
    startLeg();
    mode_ = Mode::Run;
}

void Game::startLeg() {
    const Leg& L = legs_[leg_];
    pz_ = L.gateZ - 22.0f;
    x_ = 0;
    vx_ = 0;
    spin_ = 0;
    gateTaken_ = false;
    endTaken_ = false;
    hold_ = 0;
}

void Game::blip(float freq) { sys_->apu.tone(1, freq, 0.12f); }

void Game::miss(const char* why) {
    note_ = why;
    flash_ = 0.35f;
    lives_--;
    blip(90);
    if (lives_ <= 0) {
        mode_ = Mode::Lose;
        over_ = true;
        won_ = false;
        hold_ = 0;
    } else {
        mode_ = Mode::Miss;
        hold_ = 1.0f;
    }
}

void Game::botStick(float& steer, float& power) const {
    const Leg& L = legs_[leg_];
    float aim = (!gateTaken_ && pz_ < L.gateZ + 0.5f) ? L.gateC : L.endC;
    float err = aim - x_;
    steer = clampf(err * 3.2f - vx_ * 0.55f, -1.0f, 1.0f);
    power = 1.0f;
}

void Game::drive(float dt) {
    const Leg& L = legs_[leg_];
    float steer = 0, power = 0.22f;
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
    vx_ += (des - vx_) * std::min(1.0f, dt * 6.0f);
    x_ += vx_ * dt;
    x_ = clampf(x_, -BANK, BANK);
    if (std::fabs(x_) >= BANK) vx_ *= 0.2f;
    float spd = 6.4f + power * 4.2f;
    float z0 = pz_;
    pz_ += spd * dt;
    spin_ += dt * (3.0f + power * 4.0f);

    if (!gateTaken_ && z0 < L.gateZ && pz_ >= L.gateZ) {
        gateTaken_ = true;
        if (std::fabs(x_ - L.gateC) + BODY > L.gateH) {
            miss("SCRAPED THE GATE");
            return;
        }
        blip(340);
    }
    if (!endTaken_ && z0 < L.endZ && pz_ >= L.endZ) {
        endTaken_ = true;
        if (std::fabs(x_ - L.endC) + 0.08f > L.endH) {
            miss("MISSED THE END");
            return;
        }
        blip(460);
    }
    if (pz_ >= L.doneZ) {
        legsDone_ = leg_ + 1;
        if (leg_ + 1 >= NLEGS) {
            mode_ = Mode::Win;
            over_ = true;
            won_ = true;
            note_ = "LOCK CLEAR";
            blip(640);
        } else {
            leg_++;
            startLeg();
            note_ = "LEG MADE";
            hold_ = 0.5f;
        }
    }
}

void Game::project(float wx, float wz, float& sx, float& sy, float& ppm) const {
    float d = wz - pz_;
    if (d < 0.9f) d = 0.9f;
    float n = 1.0f - std::sqrt(clampf((d - 1.5f) / 36.0f, 0.0f, 1.0f));
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
    }
}

void Game::paintSky() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < HORIZON) {
            float u = y / float(HORIZON);
            v.lineBackdrop[y] = gs::rgb4(int(2 + 6 * u), int(3 + 5 * u), int(8 + 4 * u));
            v.lineFog[y] = 0;
            v.road[y].on = false;
        } else {
            v.lineBackdrop[y] = gs::rgb4(2, 5, 2);
            float n = (y - HORIZON) / float(gs::SCREEN_H - HORIZON);
            v.lineFog[y] = uint8_t(clampf((1.0f - n) * 7.0f, 0.0f, 8.0f));
        }
    }
}

void Game::paintRoad() {
    gs::VDP& v = sys_->vdp;
    v.roadTime = int(t_ * 24);
    for (int y = HORIZON; y < gs::SCREEN_H; y++) {
        float n = (y - HORIZON) / float(gs::SCREEN_H - 1 - HORIZON);
        float d = 1.5f + 36.0f * (1.0f - n) * (1.0f - n);
        float ppm = 168.0f / d;
        gs::RoadLine& r = v.road[y];
        r.on = true;
        r.cx = 160.0f - x_ * ppm;
        r.hw = 1.32f * ppm;
        r.v = pz_ * 18.0f + d * 22.0f;
        r.pal = PAL_ROAD;
        r.band = (int(pz_ + d) & 1);
        r.style = 1;
        r.left = gs::GROUND_LAND;
        r.right = gs::GROUND_LAND;
    }
}

void Game::paintWorld() {
    struct Item {
        float z;
        int kind;
        float wx;
        bool flip;
    };
    Item items[48];
    int n = 0;
    auto push = [&](float z, int kind, float wx, bool flip) {
        if (n < 48 && z > pz_ + 0.8f && z < pz_ + 38.0f) items[n++] = {z, kind, wx, flip};
    };
    for (int i = 0; i < 7; i++) {
        float z = std::floor(pz_ / 10.0f) * 10.0f + i * 10.0f;
        push(z, 3, -2.55f, false);
        push(z + 5.0f, 3, 2.5f, true);
    }
    for (int i = 0; i < NLEGS; i++) {
        const Leg& L = legs_[i];
        float left = (L.gateC - L.gateH);
        float right = (L.gateC + L.gateH);
        push(L.gateZ, 1, left - 0.35f, false);
        push(L.gateZ, 1, right + 0.35f, true);
        push(L.endZ, 0, L.endC - L.endH, false);
        push(L.endZ, 0, L.endC + L.endH, true);
    }
    std::sort(items, items + n, [](const Item& a, const Item& b) { return a.z < b.z; });
    for (int i = 0; i < n; i++) {
        float sx, sy, ppm;
        project(items[i].wx, items[i].z, sx, sy, ppm);
        if (sx < -60 || sx > 380) continue;
        if (items[i].kind == 0) spr(art_.post, sx, sy, 2.4f * ppm, PAL_FLAG, items[i].flip);
        else if (items[i].kind == 1) spr(art_.gate, sx, sy, 4.2f * ppm, PAL_STEEL, items[i].flip);
        else spr(art_.tree, sx, sy, 3.4f * ppm, PAL_TREE, items[i].flip);
    }
}

void Game::paintKart() {
    float bob = std::sin(spin_ * 6.2832f) * 1.4f;
    float lean = vx_ * 6.0f;
    spr(art_.kart, 160.0f + lean, 198.0f + bob, 52, PAL_KART, vx_ < -0.2f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = false;
    sys.apu.setMaster(0.7f);
    sys.apu.silence();
    if (bot_) begin();
    else mode_ = Mode::Title;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        pz_ = 8.0f + std::sin(t_ * 0.3f) * 0.2f;
        x_ = std::sin(t_ * 0.45f) * 0.18f;
        spin_ += DT * 2.0f;
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) {
            begin();
            blip(500);
        }
    } else if (mode_ == Mode::Run) {
        if (hold_ > 0) hold_ -= DT;
        else drive(DT);
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Title;
            blip(160);
        }
    } else if (mode_ == Mode::Miss) {
        hold_ -= DT;
        spin_ += DT * 0.4f;
        if (hold_ <= 0) {
            startLeg();
            mode_ = Mode::Run;
        }
    } else {
        spin_ += DT * 0.5f;
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) begin();
    }

    if (flash_ > 0) flash_ -= DT;
    paintSky();
    if (flash_ > 0) {
        for (int y = 0; y < gs::SCREEN_H; y++) sys.vdp.lineBackdrop[y] = gs::rgb4(10, 2, 2);
    }
    paintRoad();
    sys.vdp.clearSprites();

    char buf[48];
    if (mode_ == Mode::Title) {
        glyphText("KART LOCK", 92, 22, 2, PAL_GOLD);
        glyphText("PASS THE GATE", 104, 48, 1, PAL_INK);
        glyphText("MAKE THE END OF THE LEG", 70, 62, 1, PAL_INK);
        glyphText("LEFT RIGHT STEER", 94, 150, 1, PAL_INK);
        glyphText("A THROTTLE   START", 88, 166, 1, PAL_GOLD);
    } else {
        std::snprintf(buf, sizeof(buf), "LEG %d/%d", std::min(leg_ + 1, NLEGS), NLEGS);
        glyphText(buf, 8, 6, 1, PAL_INK);
        std::snprintf(buf, sizeof(buf), "LIVES %d", lives_);
        glyphText(buf, 230, 6, 1, PAL_INK);
        if (mode_ == Mode::Win) {
            glyphText("LOCK CLEAR", 86, 34, 2, PAL_GOLD);
            glyphText("EVERY LEG MADE", 94, 58, 1, PAL_INK);
        } else if (mode_ == Mode::Lose) {
            glyphText("LEG FAILED", 88, 34, 2, PAL_GOLD);
            glyphText(note_, 90, 58, 1, PAL_INK);
        } else if (note_[0] && (mode_ == Mode::Miss || hold_ > 0)) {
            glyphText(note_, 100, 34, 1, PAL_GOLD);
        } else if (mode_ == Mode::Run) {
            if (!gateTaken_) glyphText("GATE AHEAD", 112, 18, 1, PAL_GOLD);
            else if (!endTaken_) glyphText("MAKE THE END", 104, 18, 1, PAL_GOLD);
        }
    }
    paintWorld();
    paintKart();
}

}  // namespace kartlock

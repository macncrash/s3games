#include "game/kilo.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace kilo {

namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr float FINISH = 1000.0f;
constexpr float BANK = 1.18f;
constexpr float HULL = 0.20f;
constexpr int HORIZON = 78;
constexpr int NW = 6;

struct Wheel {
    float z, x, r;
    int kind;  // 0 mill, 1 cart, 2 paddle
};

// Wheels sit in the channel. The open side is wide enough for the shell.
constexpr Wheel kWheels[NW] = {
    {160.f, 0.72f, 0.30f, 0},  {330.f, -0.68f, 0.32f, 1}, {490.f, 0.22f, 0.26f, 2},
    {650.f, -0.78f, 0.30f, 0}, {800.f, 0.58f, 0.34f, 1},  {930.f, -0.18f, 0.24f, 2},
};

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

const char* wheelWord(int kind) {
    if (kind == 0) return "a mill wheel";
    if (kind == 1) return "a cart wheel";
    return "a paddle wheel";
}

}  // namespace

void Game::blip(float freq) { sys_->apu.tone(1, freq, 0.1f); }

void Game::begin() {
    over_ = false;
    won_ = false;
    meters_ = 0;
    x_ = 0;
    vx_ = 0;
    pz_ = 0;
    phase_ = 0;
    flash_ = 0;
    why_ = "";
    mode_ = Mode::Row;
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Row) return;
    why_ = why;
    won_ = false;
    over_ = true;
    mode_ = Mode::Fail;
    flash_ = 0.4f;
    meters_ = int(clampf(pz_, 0.f, FINISH));
    sys_->rumble(0.45f, 0.2f, 160);
    sys_->setLight(160, 40, 24);
    sys_->apu.noiseBurst(0.4f, 180.f, 0.3f);
    std::printf("S3 SCULL KILO  FAIL  %s at %d m\n", why, meters_);
    std::fflush(stdout);
}

void Game::botStick(float& steer, float& power) const {
    int next = -1;
    float best = 1e9f;
    for (int i = 0; i < NW; i++) {
        float ahead = kWheels[i].z - pz_;
        if (ahead < -1.5f || ahead > 70.f) continue;
        if (ahead < best) {
            best = ahead;
            next = i;
        }
    }
    float aim = 0;
    if (next >= 0) {
        const Wheel& w = kWheels[next];
        constexpr float margin = 0.10f;
        float blockL = w.x - w.r - margin;
        float blockR = w.x + w.r + margin;
        float leftAim = (-BANK + blockL) * 0.5f;
        float rightAim = (blockR + BANK) * 0.5f;
        float gapL = blockL - (-BANK);
        float gapR = BANK - blockR;
        aim = gapL >= gapR ? leftAim : rightAim;
        if (best < 18.f) {
            // Commit early to the open side rather than splitting the wheel.
            aim = gapL >= gapR ? leftAim : rightAim;
        }
    }
    aim = clampf(aim, -0.85f, 0.85f);
    float err = aim - x_;
    steer = clampf(err * 3.4f - vx_ * 0.55f, -1.f, 1.f);
    power = 1.f;
}

void Game::row(float dt) {
    float steer = 0, power = 0.35f;
    if (bot_) {
        botStick(steer, power);
    } else {
        const gs::Pad& pad = sys_->pad;
        if (pad.down(gs::BTN_LEFT)) steer -= 1;
        if (pad.down(gs::BTN_RIGHT)) steer += 1;
        if (std::fabs(pad.axisX) > 0.18f) steer = pad.axisX;
        steer = clampf(steer, -1.f, 1.f);
        if (pad.down(gs::BTN_A) || pad.down(gs::BTN_C) || pad.down(gs::BTN_UP) || pad.accel > 0.2f) power = 1.f;
        if (pad.down(gs::BTN_DOWN) || pad.brake > 0.2f) power = 0.08f;
    }
    float des = steer * 1.45f;
    vx_ += (des - vx_) * std::min(1.f, dt * 8.f);
    x_ += vx_ * dt;
    x_ = clampf(x_, -BANK, BANK);
    if (std::fabs(x_) >= BANK - 0.001f) vx_ = 0;

    float spd = 12.5f + power * 7.5f;
    pz_ += spd * dt;
    phase_ += dt * (1.5f + power * 1.6f);
    meters_ = int(clampf(pz_, 0.f, FINISH));

    for (int i = 0; i < NW; i++) {
        const Wheel& w = kWheels[i];
        if (std::fabs(pz_ - w.z) > 1.15f) continue;
        if (std::fabs(x_ - w.x) < w.r + HULL) {
            char buf[48];
            std::snprintf(buf, sizeof buf, "touched %s", wheelWord(w.kind));
            fail(buf);
            return;
        }
    }
    if (pz_ >= FINISH) {
        mode_ = Mode::Win;
        over_ = true;
        won_ = true;
        meters_ = 1000;
        why_ = "wheels untouched";
        sys_->rumble(0.12f, 0.05f, 140);
        sys_->setLight(40, 150, 80);
        blip(660);
        std::printf("S3 SCULL KILO  CLEAR  finished the kilometer  1000 m  wheels untouched  (%.1f s)\n", t_);
        std::fflush(stdout);
    }
}

void Game::project(float wx, float wz, float& sx, float& sy, float& ppm) const {
    float d = wz - pz_;
    if (d < 0.85f) d = 0.85f;
    float n = 1.f - std::sqrt(clampf((d - 1.4f) / 46.f, 0.f, 1.f));
    sy = float(HORIZON) + n * float(gs::SCREEN_H - 10 - HORIZON);
    ppm = 150.f / d;
    sx = 160.f + (wx - x_) * ppm;
}

void Game::spr(const gs::Mipped& m, float cx, float footY, float destH, int pal, bool flip) {
    if (destH < 2.f || m.h < 1) return;
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
            v.lineBackdrop[y] = gs::rgb4(int(4 + 7 * u), int(6 + 6 * u), int(11 + 2 * u));
            v.lineFog[y] = 0;
            v.road[y].on = false;
        } else {
            v.lineBackdrop[y] = gs::rgb4(2, 6, 3);
            float n = (y - HORIZON) / float(gs::SCREEN_H - HORIZON);
            v.lineFog[y] = uint8_t(clampf((1.f - n) * 6.f, 0.f, 7.f));
        }
    }
}

void Game::paintWater() {
    gs::VDP& v = sys_->vdp;
    v.roadTime = int(t_ * 36);
    for (int y = HORIZON; y < gs::SCREEN_H; y++) {
        float n = (y - HORIZON) / float(gs::SCREEN_H - 1 - HORIZON);
        float d = 1.4f + 46.f * (1.f - n) * (1.f - n);
        float ppm = 150.f / d;
        gs::RoadLine& r = v.road[y];
        r.on = true;
        r.cx = 160.f - x_ * ppm;
        r.hw = 1.35f * ppm;
        r.v = pz_ * 16.f + d * 20.f;
        r.pal = PAL_WATER;
        r.band = (int(pz_ * 2.f + d) & 3) == 0 ? 1 : 0;
        r.style = 2;
        r.left = gs::GROUND_LAND;
        r.right = gs::GROUND_LAND;
    }
}

void Game::paintWorld() {
    struct Item {
        float z;
        int kind;
        float wx;
        int extra;
    };
    Item items[64];
    int n = 0;
    auto push = [&](float z, int kind, float wx, int extra) {
        if (n < 64 && z > pz_ + 0.8f && z < pz_ + 48.f) items[n++] = {z, kind, wx, extra};
    };
    float base = std::floor(pz_ / 10.f) * 10.f;
    for (int i = 0; i < 8; i++) {
        float z = base + i * 10.f;
        push(z, 1, -2.15f, 0);
        push(z + 4.f, 2, 2.05f, 0);
    }
    for (int m = 0; m <= 10; m++) {
        float z = m * 100.f;
        push(z, 3, -1.55f, m);
        push(z, 3, 1.55f, m);
        if (m == 10) push(z, 4, 0.f, 0);
    }
    int spin = int(t_ * 8.f) & 3;
    for (int i = 0; i < NW; i++) {
        const Wheel& w = kWheels[i];
        push(w.z, 10 + w.kind, w.x, (spin + i) % 3);
        if (w.kind == 0) push(w.z, 5, w.x, 0);
    }
    std::sort(items, items + n, [](const Item& a, const Item& b) { return a.z < b.z; });
    for (int i = 0; i < n; i++) {
        float sx, sy, ppm;
        project(items[i].wx, items[i].z, sx, sy, ppm);
        if (sx < -50 || sx > 370) continue;
        int k = items[i].kind;
        if (k == 1) spr(art_.tree, sx, sy, 3.2f * ppm, PAL_BANK, false);
        else if (k == 2) spr(art_.reed, sx, sy, 1.15f * ppm, PAL_BANK, false);
        else if (k == 3) spr(art_.post, sx, sy, 2.1f * ppm, PAL_MARK, false);
        else if (k == 4) spr(art_.flag, sx, sy - 1.6f * ppm, 1.1f * ppm, PAL_MARK, false);
        else if (k == 5) spr(art_.mill, sx, sy, 2.6f * ppm, PAL_MILL, items[i].wx < 0);
        else if (k >= 10) {
            int pal = k == 10 ? PAL_MILL : (k == 11 ? PAL_CART : PAL_WHEEL);
            float ht = (k == 11 ? 1.7f : 2.2f) * ppm;
            spr(art_.wheel[items[i].extra % 3], sx, sy - 0.15f * ppm, std::max(8.f, ht), pal, false);
        }
    }
}

void Game::paintHull() {
    float sway = std::sin(phase_ * 6.2832f) * 2.5f;
    float bx = 160.f + sway * 0.2f;
    int fr = int(std::fmod(phase_, 1.f) * 4.f);
    if (fr < 0) fr = 0;
    if (fr > 3) fr = 3;
    float lift = (fr - 1.5f) * 4.f;
    spr(art_.oar[fr], bx - 34, 172 + lift, 15, PAL_OAR, false);
    spr(art_.oar[3 - fr], bx + 34, 172 - lift, 15, PAL_OAR, true);
    spr(art_.hull, bx, 200, 56, PAL_HULL, false);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = false;
    sys.apu.setMaster(0.75f);
    sys.apu.silence();
    if (bot_) begin();
    else mode_ = Mode::Title;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        pz_ = 12.f;
        x_ = std::sin(t_ * 0.4f) * 0.2f;
        phase_ += DT * 1.2f;
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) {
            begin();
            blip(480);
        }
    } else if (mode_ == Mode::Row) {
        row(DT);
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Title;
    } else {
        phase_ += DT * 0.4f;
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) begin();
    }
    if (flash_ > 0) flash_ -= DT;

    sys.vdp.clearSprites();
    paintSky();
    if (flash_ > 0.15f) {
        for (int y = 0; y < gs::SCREEN_H; y++) sys.vdp.lineBackdrop[y] = gs::rgb4(9, 2, 2);
    }
    paintWater();

    char buf[64];
    if (mode_ == Mode::Title) {
        glyphText("SCULL KILO", 86, 24, 2, PAL_GOLD);
        glyphText("FINISH THE KILOMETER", 82, 48, 1, PAL_INK);
        glyphText("DO NOT TOUCH A WHEEL", 82, 62, 1, PAL_INK);
        glyphText("LEFT RIGHT STEER", 100, 148, 1, PAL_INK);
        glyphText("A STROKE   START ROW", 82, 162, 1, PAL_GOLD);
    } else {
        std::snprintf(buf, sizeof buf, "%d M", meters_);
        glyphText(buf, 8, 6, 1, PAL_INK);
        glyphText("1000 M", 250, 6, 1, PAL_GOLD);
        if (mode_ == Mode::Win) {
            glyphText("KILO CLEAR", 86, 36, 2, PAL_GOLD);
            glyphText("WHEELS UNTOUCHED", 88, 58, 1, PAL_INK);
        } else if (mode_ == Mode::Fail) {
            glyphText("SCRAPE", 116, 36, 2, PAL_GOLD);
            glyphText(why_, 70, 58, 1, PAL_INK);
            glyphText("START RETRIES", 100, 78, 1, PAL_INK);
        }
    }
    paintWorld();
    paintHull();
}

}  // namespace kilo

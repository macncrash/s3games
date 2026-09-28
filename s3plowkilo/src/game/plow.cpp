#include "game/plow.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace plow {

namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr float FINISH = 1000.0f;
constexpr float BANK = 1.90f;
constexpr float SHARE = 0.22f;
constexpr float SPEED = 17.6f;
constexpr int HORIZON = 86;
constexpr int NW = 8;

struct Wheel {
    float z, cx, amp, omega, phase, r;
    int kind;  // 0 cart, 1 wagon, 2 tractor
};

// Each wheel rolls across the furrow. One side of the lane stays open.
constexpr Wheel kWheels[NW] = {
    {160.f, -0.55f, 0.48f, 0.72f, 0.3f, 0.40f, 0},  {290.f, 0.70f, 0.42f, 0.90f, 1.2f, 0.46f, 1},
    {420.f, -0.15f, 0.78f, 0.55f, 0.5f, 0.38f, 2},  {545.f, 0.35f, 0.62f, 0.80f, 2.1f, 0.44f, 0},
    {670.f, -0.80f, 0.36f, 1.05f, 0.8f, 0.36f, 1},  {790.f, 0.20f, 0.84f, 0.60f, 1.6f, 0.42f, 2},
    {890.f, -0.40f, 0.55f, 0.88f, 0.4f, 0.40f, 0},  {965.f, 0.55f, 0.30f, 0.50f, 2.4f, 0.34f, 1},
};

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

float wheelX(const Wheel& w, float time) {
    return w.cx + std::sin(time * w.omega + w.phase) * w.amp;
}

const char* wheelWord(int kind) {
    if (kind == 0) return "a cart wheel";
    if (kind == 1) return "a wagon wheel";
    return "a tractor tyre";
}

}  // namespace

void Game::blip(float freq) { sys_->apu.tone(1, freq, 0.1f); }

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.HUD.enabled = false;
    mode_ = Mode::Title;
    if (bot_) begin();
}

void Game::begin() {
    over_ = false;
    won_ = false;
    meters_ = 0;
    t_ = 0;
    x_ = 0;
    vx_ = 0;
    pz_ = 0;
    flash_ = 0;
    why_ = "";
    mode_ = Mode::Run;
    sys_->setLight(180, 120, 40);
    blip(220);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    why_ = why;
    won_ = false;
    over_ = true;
    mode_ = Mode::Fail;
    flash_ = 0.45f;
    meters_ = int(clampf(pz_, 0.f, FINISH));
    sys_->rumble(0.5f, 0.25f, 180);
    sys_->setLight(160, 40, 20);
    sys_->apu.noiseBurst(0.45f, 140.f, 0.32f);
    std::printf("S3 PLOW KILO  FAIL  %s at %d m\n", why, meters_);
    std::fflush(stdout);
}

void Game::botStick(float& steer) const {
    int next = -1;
    float best = 1e9f;
    for (int i = 0; i < NW; i++) {
        float ahead = kWheels[i].z - pz_;
        if (ahead < -1.2f || ahead > 78.f) continue;
        if (ahead < best) {
            best = ahead;
            next = i;
        }
    }
    float aim = 0;
    if (next >= 0) {
        const Wheel& w = kWheels[next];
        float tHit = t_ + std::max(0.f, best) / SPEED;
        float wx = wheelX(w, tHit);
        float pad = w.r + SHARE + 0.16f;
        float leftEdge = wx - pad;
        float rightEdge = wx + pad;
        float leftRoom = leftEdge - (-BANK);
        float rightRoom = BANK - rightEdge;
        float leftAim = (-BANK + leftEdge) * 0.5f;
        float rightAim = (rightEdge + BANK) * 0.5f;
        aim = leftRoom >= rightRoom ? leftAim : rightAim;
        if (best < 14.f) {
            // Commit to the open dirt rather than splitting the iron.
            float here = wheelX(w, t_);
            if (std::fabs(x_ - here) < pad && leftRoom > 0.2f && rightRoom > 0.2f)
                aim = (x_ < here) ? leftAim : rightAim;
        }
    }
    aim = clampf(aim, -BANK + SHARE + 0.08f, BANK - SHARE - 0.08f);
    float err = aim - x_;
    steer = clampf(err * 3.6f - vx_ * 0.62f, -1.f, 1.f);
}

void Game::run(float dt) {
    float steer = 0;
    if (bot_) {
        botStick(steer);
    } else {
        const gs::Pad& pad = sys_->pad;
        if (pad.down(gs::BTN_LEFT)) steer -= 1;
        if (pad.down(gs::BTN_RIGHT)) steer += 1;
        if (std::fabs(pad.axisX) > 0.16f) steer = pad.axisX;
        steer = clampf(steer, -1.f, 1.f);
    }
    float des = steer * 2.35f;
    vx_ += (des - vx_) * std::min(1.f, dt * 7.5f);
    x_ += vx_ * dt;
    float lim = BANK - SHARE;
    x_ = clampf(x_, -lim, lim);
    if (std::fabs(x_) >= lim - 0.001f) vx_ *= 0.2f;

    pz_ += SPEED * dt;
    meters_ = int(clampf(pz_, 0.f, FINISH));

    for (int i = 0; i < NW; i++) {
        const Wheel& w = kWheels[i];
        if (std::fabs(pz_ - w.z) > 1.05f) continue;
        if (std::fabs(x_ - wheelX(w, t_)) < w.r + SHARE) {
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
        sys_->rumble(0.1f, 0.04f, 140);
        sys_->setLight(40, 140, 50);
        blip(620);
        std::printf("S3 PLOW KILO  CLEAR  finished the kilometer  1000 m  wheels untouched  (%.1f s)\n", t_);
        std::fflush(stdout);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) begin();
    } else if (mode_ == Mode::Run) {
        t_ += DT;
        run(DT);
    } else {
        flash_ = std::max(0.f, flash_ - DT);
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) begin();
    }
    paintSky();
    paintField();
    paintWorld();
}

void Game::project(float wx, float wz, float& sx, float& sy, float& ppm) const {
    float d = wz - pz_;
    if (d < 0.9f) d = 0.9f;
    float n = 1.f - std::sqrt(clampf((d - 1.5f) / 42.f, 0.f, 1.f));
    sy = float(HORIZON) + n * float(gs::SCREEN_H - 18 - HORIZON);
    ppm = 168.f / d;
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
            int wash = flash_ > 0 && mode_ == Mode::Fail ? int(flash_ * 6) : 0;
            v.lineBackdrop[y] = gs::rgb4(int(6 + 8 * u) + wash, int(7 + 4 * u), int(10 - 4 * u));
            v.lineFog[y] = 0;
            v.road[y].on = false;
        } else {
            v.lineBackdrop[y] = gs::rgb4(3, 5, 1);
            float n = (y - HORIZON) / float(gs::SCREEN_H - HORIZON);
            v.lineFog[y] = uint8_t(clampf((1.f - n) * 5.f, 0.f, 6.f));
        }
    }
}

void Game::paintField() {
    gs::VDP& v = sys_->vdp;
    v.roadTime = int(t_ * 20);
    for (int y = HORIZON; y < gs::SCREEN_H; y++) {
        float n = (y - HORIZON) / float(gs::SCREEN_H - 1 - HORIZON);
        float d = 1.5f + 42.f * (1.f - n) * (1.f - n);
        float ppm = 168.f / d;
        gs::RoadLine& r = v.road[y];
        r.on = true;
        r.cx = 160.f - x_ * ppm;
        r.hw = (BANK + 0.15f) * ppm;
        r.v = pz_ * 18.f + d * 22.f;
        r.pal = PAL_DIRT;
        r.band = (int(pz_ * 1.5f + d) & 3) == 0 ? 1 : 0;
        r.style = 0;
        r.left = gs::GROUND_LAND;
        r.right = gs::GROUND_LAND;
    }
}

void Game::paintWorld() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();

    char hud[48];
    if (mode_ == Mode::Title) {
        glyphText("S3 PLOW KILO", 58, 28, 2, PAL_GOLD);
        glyphText("FINISH THE KILOMETER", 46, 56, 1, PAL_INK);
        glyphText("DO NOT TOUCH A WHEEL", 46, 70, 1, PAL_INK);
        glyphText("ARROWS STEER   A START", 40, 168, 1, PAL_GOLD);
    } else {
        std::snprintf(hud, sizeof hud, "%d M", meters_);
        glyphText(hud, 8, 6, 2, PAL_INK);
        if (mode_ == Mode::Fail) {
            glyphText("SHARE HIT IRON", 52, 78, 2, PAL_MARK);
            glyphText(why_, 40, 104, 1, PAL_INK);
        } else if (mode_ == Mode::Win) {
            glyphText("KILOMETER CLEAR", 40, 70, 2, PAL_GOLD);
            glyphText("WHEELS UNTOUCHED", 52, 96, 1, PAL_INK);
        }
    }

    // The share stays under the camera. Horses walk just ahead of it.
    if (mode_ != Mode::Title) {
        spr(art_.plow, 160.f, 214.f, 78.f, PAL_PLOW, false);
        float hs, hy, hp;
        project(x_ - 0.34f, pz_ + 3.4f, hs, hy, hp);
        spr(art_.horse, hs, hy, hp * 1.15f, PAL_HORSE, false);
        project(x_ + 0.34f, pz_ + 3.6f, hs, hy, hp);
        spr(art_.horse, hs, hy, hp * 1.15f, PAL_HORSE, true);
    } else {
        spr(art_.plow, 160.f, 200.f, 64.f, PAL_PLOW, false);
    }

    struct Item {
        float z, wx;
        int kind, extra;
    };
    Item items[48];
    int n = 0;
    auto push = [&](float z, float wx, int kind, int extra) {
        if (n < 48 && z > pz_ + 0.6f && z < pz_ + 52.f) items[n++] = {z, wx, kind, extra};
    };
    float base = std::floor(pz_ / 16.f) * 16.f;
    for (int i = 0; i < 6; i++) {
        float z = base + i * 16.f;
        push(z, -2.55f, 1, 0);
        push(z + 8.f, 2.55f, 2, i & 1);
    }
    for (int m = 0; m <= 10; m++) {
        float z = m * 100.f;
        push(z, -2.15f, 3, m);
        if (m == 10) push(z, 0.f, 4, 0);
        if (m == 5) push(z, 2.7f, 5, 0);
    }
    int spin = int(t_ * 7.f) & 3;
    if (spin > 2) spin = 2;
    for (int i = 0; i < NW; i++) {
        const Wheel& w = kWheels[i];
        push(w.z, wheelX(w, t_), 10 + w.kind, spin);
    }
    std::sort(items, items + n, [](const Item& a, const Item& b) { return a.z < b.z; });
    for (int i = 0; i < n; i++) {
        float sx, sy, ppm;
        project(items[i].wx, items[i].z, sx, sy, ppm);
        if (sx < -40 || sx > 360) continue;
        int k = items[i].kind;
        if (k == 1) spr(art_.tree, sx, sy, ppm * 2.4f, PAL_FIELD, false);
        else if (k == 2) spr(art_.stake, sx, sy, ppm * 1.3f, PAL_MARK, items[i].extra != 0);
        else if (k == 3) spr(art_.stake, sx, sy, ppm * 1.5f, PAL_GOLD, false);
        else if (k == 4) spr(art_.flag, sx, sy, ppm * 2.2f, PAL_MARK, false);
        else if (k == 5) spr(art_.barn, sx, sy, ppm * 3.2f, PAL_WAGON, false);
        else if (k == 10) spr(art_.wheel[items[i].extra], sx, sy, ppm * 1.7f, PAL_WHEEL, false);
        else if (k == 11) {
            spr(art_.wagon, sx, sy - ppm * 0.55f, ppm * 1.15f, PAL_WAGON, false);
            spr(art_.wheel[items[i].extra], sx, sy, ppm * 1.5f, PAL_WHEEL, false);
        } else if (k == 12) spr(art_.tyre, sx, sy, ppm * 1.9f, PAL_TYRE, false);
    }
}

}  // namespace plow

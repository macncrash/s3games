#include "game/kilo.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace headerkilo {

namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr float FINISH = 1000.0f;
constexpr float BANK = 1.85f;
constexpr float BEAM = 0.24f;
constexpr float SPEED = 18.4f;
constexpr int HORIZON = 78;
constexpr int NW = 8;

struct Wheel {
    float z, cx, amp, omega, phase, r;
    int kind;  // 0 mill, 1 paddle
};

// Mill wheels and a steamer's paddles sweep the cut. One bank stays open.
constexpr Wheel kWheels[NW] = {
    {150.f, -0.48f, 0.46f, 0.70f, 0.2f, 0.38f, 0}, {285.f, 0.62f, 0.40f, 0.92f, 1.4f, 0.42f, 1},
    {410.f, -0.10f, 0.72f, 0.58f, 0.6f, 0.36f, 0}, {540.f, 0.38f, 0.58f, 0.84f, 2.0f, 0.40f, 1},
    {665.f, -0.78f, 0.34f, 1.02f, 0.9f, 0.34f, 0}, {785.f, 0.18f, 0.80f, 0.62f, 1.5f, 0.40f, 1},
    {890.f, -0.36f, 0.52f, 0.86f, 0.4f, 0.38f, 0}, {960.f, 0.50f, 0.28f, 0.54f, 2.2f, 0.32f, 1},
};

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

float wheelX(const Wheel& w, float time) { return w.cx + std::sin(time * w.omega + w.phase) * w.amp; }

const char* wheelWord(int kind) { return kind == 0 ? "a mill wheel" : "a paddle wheel"; }

}  // namespace

void Game::blip(float freq) { sys_->apu.tone(1, freq, 0.1f); }

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.HUD.enabled = false;
    sys.apu.setMaster(0.6f);
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
    sys_->setLight(40, 90, 160);
    blip(240);
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
    sys_->setLight(140, 30, 20);
    sys_->apu.noiseBurst(0.45f, 140.f, 0.32f);
    std::printf("S3 HEADER KILO  FAIL  %s at %d m\n", why, meters_);
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
        float pad = w.r + BEAM + 0.18f;
        float leftEdge = wx - pad;
        float rightEdge = wx + pad;
        float leftRoom = leftEdge - (-BANK);
        float rightRoom = BANK - rightEdge;
        float leftAim = (-BANK + leftEdge) * 0.5f;
        float rightAim = (rightEdge + BANK) * 0.5f;
        aim = leftRoom >= rightRoom ? leftAim : rightAim;
        if (best < 14.f) {
            float here = wheelX(w, t_);
            if (std::fabs(x_ - here) < pad && leftRoom > 0.2f && rightRoom > 0.2f)
                aim = (x_ < here) ? leftAim : rightAim;
        }
    }
    aim = clampf(aim, -BANK + BEAM + 0.08f, BANK - BEAM - 0.08f);
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
    float des = steer * 2.4f;
    vx_ += (des - vx_) * std::min(1.f, dt * 7.2f);
    x_ += vx_ * dt;
    float lim = BANK - BEAM;
    x_ = clampf(x_, -lim, lim);
    if (std::fabs(x_) >= lim - 0.001f) vx_ *= 0.2f;

    pz_ += SPEED * dt;
    meters_ = int(clampf(pz_, 0.f, FINISH));

    for (int i = 0; i < NW; i++) {
        const Wheel& w = kWheels[i];
        if (std::fabs(pz_ - w.z) > 1.1f) continue;
        if (std::fabs(x_ - wheelX(w, t_)) < w.r + BEAM) {
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
        sys_->setLight(40, 140, 80);
        blip(640);
        std::printf("S3 HEADER KILO  WIN  finished the kilometer  1000 m  wheels untouched  (%.1f s)\n", t_);
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
    paintCanal();
    paintWorld();
}

void Game::project(float wx, float wz, float& sx, float& sy, float& ppm) const {
    float d = wz - pz_;
    if (d < 0.9f) d = 0.9f;
    float n = 1.f - std::sqrt(clampf((d - 1.5f) / 44.f, 0.f, 1.f));
    sy = float(HORIZON) + n * float(gs::SCREEN_H - 16 - HORIZON);
    ppm = 172.f / d;
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
            v.lineBackdrop[y] = gs::rgb4(int(4 + 6 * u) + wash, int(7 + 5 * u), int(12 - 2 * u));
            v.lineFog[y] = 0;
            v.road[y].on = false;
        } else {
            v.lineBackdrop[y] = gs::rgb4(1, 4, 7);
            float n = (y - HORIZON) / float(gs::SCREEN_H - HORIZON);
            v.lineFog[y] = uint8_t(clampf((1.f - n) * 6.f, 0.f, 7.f));
        }
    }
}

void Game::paintCanal() {
    gs::VDP& v = sys_->vdp;
    v.roadTime = int(t_ * 24);
    for (int y = HORIZON; y < gs::SCREEN_H; y++) {
        float n = (y - HORIZON) / float(gs::SCREEN_H - 1 - HORIZON);
        float d = 1.5f + 44.f * (1.f - n) * (1.f - n);
        float ppm = 172.f / d;
        gs::RoadLine& r = v.road[y];
        r.on = true;
        r.cx = 160.f - x_ * ppm;
        r.hw = (BANK + 0.22f) * ppm;
        r.v = pz_ * 20.f + d * 24.f;
        r.pal = PAL_WATER;
        r.band = (int(pz_ * 1.2f + d) & 3) == 0 ? 1 : 0;
        r.style = 2;
        r.left = gs::GROUND_LAND;
        r.right = gs::GROUND_LAND;
    }
}

void Game::paintWorld() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();

    char hud[48];
    if (mode_ == Mode::Title) {
        glyphText("S3 HEADER KILO", 52, 24, 2, PAL_GOLD);
        glyphText("FINISH THE KILOMETER", 46, 52, 1, PAL_INK);
        glyphText("DO NOT TOUCH A WHEEL", 46, 66, 1, PAL_INK);
        glyphText("ARROWS STEER   A START", 40, 168, 1, PAL_GOLD);
    } else {
        std::snprintf(hud, sizeof hud, "%d M", meters_);
        glyphText(hud, 8, 6, 2, PAL_INK);
        if (mode_ == Mode::Fail) {
            glyphText("WHEEL TOUCHED", 58, 74, 2, PAL_MARK);
            glyphText(why_, 28, 100, 1, PAL_INK);
        } else if (mode_ == Mode::Win) {
            glyphText("KILOMETER CLEAR", 40, 66, 2, PAL_GOLD);
            glyphText("WHEELS UNTOUCHED", 52, 92, 1, PAL_INK);
        }
    }

    if (mode_ != Mode::Title) {
        spr(art_.hull, 160.f, 216.f, 72.f, PAL_HULL, false);
        spr(art_.sail, 160.f, 168.f, 46.f, PAL_SAIL, vx_ < -0.15f);
    } else {
        spr(art_.hull, 160.f, 196.f, 58.f, PAL_HULL, false);
        spr(art_.sail, 160.f, 154.f, 38.f, PAL_SAIL, false);
    }

    struct Item {
        float z, wx;
        int kind, extra;
    };
    Item items[48];
    int n = 0;
    auto push = [&](float z, float wx, int kind, int extra) {
        if (n < 48 && z > pz_ + 0.6f && z < pz_ + 54.f) items[n++] = {z, wx, kind, extra};
    };
    float base = std::floor(pz_ / 14.f) * 14.f;
    for (int i = 0; i < 6; i++) {
        float z = base + i * 14.f;
        push(z, -2.45f, 1, i & 1);
        push(z + 7.f, 2.45f, 1, (i + 1) & 1);
    }
    for (int m = 0; m <= 10; m++) {
        float z = m * 100.f;
        push(z, -2.05f, 2, 0);
        if (m == 10) push(z, 0.f, 4, 0);
        if (m == 3 || m == 7) push(z, 2.7f, 3, 0);
    }
    int spin = int(t_ * 8.f) & 3;
    if (spin > 2) spin = 2;
    for (int i = 0; i < NW; i++) {
        const Wheel& w = kWheels[i];
        push(w.z, wheelX(w, t_), 10 + w.kind, spin);
    }
    std::sort(items, items + n, [](const Item& a, const Item& b) { return a.z < b.z; });
    for (int i = 0; i < n; i++) {
        float sx, sy, ppm;
        project(items[i].wx, items[i].z, sx, sy, ppm);
        if (sx < -48 || sx > 368) continue;
        int k = items[i].kind;
        if (k == 1) spr(art_.reed, sx, sy, ppm * 1.6f, PAL_REED, items[i].extra != 0);
        else if (k == 2) spr(art_.buoy, sx, sy, ppm * 1.15f, PAL_MARK, false);
        else if (k == 3) spr(art_.mill, sx, sy, ppm * 2.6f, PAL_MILL, false);
        else if (k == 4) spr(art_.flag, sx, sy, ppm * 2.0f, PAL_MARK, false);
        else if (k == 10) spr(art_.wheel[items[i].extra], sx, sy, ppm * 1.65f, PAL_WHEEL, false);
        else if (k == 11) spr(art_.paddle, sx, sy, ppm * 1.35f, PAL_WHEEL, false);
    }
}

}  // namespace headerkilo

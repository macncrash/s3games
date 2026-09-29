#include "game/pass.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace headerpass {

namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr float LEN = 480.0f;
constexpr float SPEED = 22.0f;
constexpr float CLOCK = 28.0f;
constexpr float BANK = 2.15f;
constexpr float BEAM = 0.22f;
constexpr int HORIZON = 72;
constexpr int NG = 8;

struct Gate {
    float z, cx, hw;
};

// Open water between the jaws. Outside the half-width the rock takes the boat.
constexpr Gate kGates[NG] = {
    {62.f, -0.62f, 0.72f}, {118.f, 0.78f, 0.60f}, {176.f, -0.90f, 0.55f}, {234.f, 0.28f, 0.52f},
    {292.f, 0.95f, 0.54f}, {348.f, -0.48f, 0.50f}, {404.f, 0.22f, 0.58f}, {458.f, 0.00f, 0.78f},
};

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
    storm_ = CLOCK;
    if (bot_) begin();
}

void Game::begin() {
    over_ = false;
    won_ = false;
    marker_ = 0;
    t_ = 0;
    storm_ = CLOCK;
    x_ = 0;
    vx_ = 0;
    pz_ = 0;
    flash_ = 0;
    why_ = "";
    mode_ = Mode::Run;
    sys_->setLight(30, 50, 90);
    blip(220);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    why_ = why;
    won_ = false;
    over_ = true;
    mode_ = Mode::Fail;
    flash_ = 0.5f;
    sys_->rumble(0.55f, 0.3f, 200);
    sys_->setLight(90, 20, 16);
    sys_->apu.noiseBurst(0.5f, 160.f, 0.35f);
    std::printf("S3 HEADER PASS  FAIL  %s\n", why);
    std::fflush(stdout);
}

void Game::botStick(float& steer) const {
    int next = -1;
    float best = 1e9f;
    for (int i = 0; i < NG; i++) {
        float ahead = kGates[i].z - pz_;
        if (ahead < -2.f || ahead > 70.f) continue;
        if (ahead < best) {
            best = ahead;
            next = i;
        }
    }
    float aim = 0;
    if (next >= 0) aim = kGates[next].cx;
    if (pz_ > LEN - 36.f) aim = 0;
    aim = clampf(aim, -BANK + BEAM + 0.06f, BANK - BEAM - 0.06f);
    float err = aim - x_;
    steer = clampf(err * 3.4f - vx_ * 0.7f, -1.f, 1.f);
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
    float des = steer * 2.6f;
    vx_ += (des - vx_) * std::min(1.f, dt * 8.f);
    x_ += vx_ * dt;
    float lim = BANK - BEAM;
    if (x_ < -lim || x_ > lim) {
        fail("the wall took the header");
        return;
    }

    pz_ += SPEED * dt;
    storm_ -= dt;
    if (pz_ > 90.f) marker_ = std::max(marker_, 1);
    if (pz_ > 230.f) marker_ = std::max(marker_, 2);
    if (pz_ > 400.f) marker_ = std::max(marker_, 3);

    for (int i = 0; i < NG; i++) {
        const Gate& g = kGates[i];
        if (std::fabs(pz_ - g.z) > 1.15f) continue;
        if (std::fabs(x_ - g.cx) + BEAM > g.hw) {
            fail("a jaw closed the gap");
            return;
        }
    }

    if (pz_ >= LEN) {
        if (storm_ <= 0.f) {
            fail("the storm closed the pass");
            return;
        }
        mode_ = Mode::Win;
        over_ = true;
        won_ = true;
        marker_ = 4;
        why_ = "pass clear";
        sys_->rumble(0.12f, 0.05f, 160);
        sys_->setLight(40, 120, 70);
        blip(620);
        std::printf("S3 HEADER PASS  WIN  cleared the pass before the storm clock  (%.1f s)\n", t_);
        std::fflush(stdout);
        return;
    }
    if (storm_ <= 0.f) fail("missed the end");
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
    if (mode_ == Mode::Run && storm_ < 8.f) sys.apu.noise(0.04f, 240.f + (8.f - storm_) * 24.f);
    else if (mode_ != Mode::Run) sys.apu.noise(0, 0);
    paintSky();
    paintWater();
    paintWorld();
}

void Game::project(float wx, float wz, float& sx, float& sy, float& ppm) const {
    float d = wz - pz_;
    if (d < 0.85f) d = 0.85f;
    float n = 1.f - std::sqrt(clampf((d - 1.4f) / 48.f, 0.f, 1.f));
    sy = float(HORIZON) + n * float(gs::SCREEN_H - 14 - HORIZON);
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
    float gloom = clampf(1.f - storm_ / CLOCK, 0.f, 1.f);
    if (mode_ == Mode::Title) gloom = 0.15f;
    if (mode_ == Mode::Win) gloom = 0.05f;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < HORIZON) {
            float u = y / float(HORIZON);
            int wash = flash_ > 0 && mode_ == Mode::Fail ? int(flash_ * 7) : 0;
            int r = int((3 + 5 * u) * (1.f - gloom) + 4 * gloom) + wash;
            int g = int((6 + 4 * u) * (1.f - gloom) + 4 * gloom);
            int b = int((11 - 3 * u) * (1.f - gloom) + 5 * gloom);
            v.lineBackdrop[y] = gs::rgb4(std::min(15, r), std::min(15, g), std::min(15, b));
            v.lineFog[y] = uint8_t(clampf(gloom * 5.f, 0.f, 6.f));
            v.road[y].on = false;
        } else {
            v.lineBackdrop[y] = gs::rgb4(1, 3, 5);
            float n = (y - HORIZON) / float(gs::SCREEN_H - HORIZON);
            v.lineFog[y] = uint8_t(clampf((1.f - n) * 5.f + gloom * 4.f, 0.f, 10.f));
        }
    }
    v.setFogColor(gs::rgb4(int(6 + gloom * 4), int(7 + gloom * 2), int(9 - gloom * 2)));
}

void Game::paintWater() {
    gs::VDP& v = sys_->vdp;
    v.roadTime = int(t_ * 28);
    for (int y = HORIZON; y < gs::SCREEN_H; y++) {
        float n = (y - HORIZON) / float(gs::SCREEN_H - 1 - HORIZON);
        float d = 1.4f + 48.f * (1.f - n) * (1.f - n);
        float ppm = 168.f / d;
        gs::RoadLine& r = v.road[y];
        r.on = true;
        r.cx = 160.f - x_ * ppm;
        r.hw = (BANK + 0.15f) * ppm;
        r.v = pz_ * 18.f + d * 22.f;
        r.pal = PAL_WATER;
        r.band = (int(pz_ + d) & 3) == 0 ? 1 : 0;
        r.style = 2;
        r.left = gs::GROUND_DROP;
        r.right = gs::GROUND_DROP;
    }
}

void Game::paintWorld() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();

    char hud[64];
    if (mode_ == Mode::Title) {
        glyphText("S3 HEADER PASS", 58, 18, 2, PAL_GOLD);
        glyphText("CLEAR THE PASS", 70, 48, 1, PAL_INK);
        glyphText("BEFORE THE STORM CLOCK", 40, 62, 1, PAL_INK);
        glyphText("MISS THE END AND THE LEG FAILS", 16, 80, 1, PAL_BAD);
        glyphText("ARROWS STEER    A STARTS", 34, 188, 1, PAL_GOLD);
    } else {
        std::snprintf(hud, sizeof hud, "STORM %4.1f", std::max(0.f, storm_));
        glyphText(hud, 8, 6, 2, storm_ < 8.f ? PAL_BAD : PAL_INK);
        int pct = int(clampf(pz_ / LEN, 0.f, 1.f) * 100.f);
        std::snprintf(hud, sizeof hud, "LEG %d", pct);
        glyphText(hud, 210, 8, 1, PAL_GOLD);
        if (mode_ == Mode::Fail) {
            glyphText("LEG FAILED", 70, 70, 2, PAL_BAD);
            glyphText(why_, 40, 96, 1, PAL_INK);
            glyphText("A RETRIES", 100, 120, 1, PAL_GOLD);
        } else if (mode_ == Mode::Win) {
            glyphText("PASS CLEAR", 70, 64, 2, PAL_GOOD);
            glyphText("BEFORE THE STORM", 58, 90, 1, PAL_INK);
        }
    }

    if (mode_ != Mode::Title) {
        spr(art_.hull, 160.f, 214.f, 70.f, PAL_HULL, false);
        spr(art_.sail, 160.f + vx_ * 2.f, 164.f, 44.f, PAL_SAIL, vx_ < -0.2f);
    } else {
        spr(art_.hull, 160.f, 176.f, 52.f, PAL_HULL, false);
        spr(art_.sail, 160.f, 138.f, 34.f, PAL_SAIL, false);
    }

    struct Item {
        float z, wx, h;
        int kind;
        bool flip;
    };
    Item items[80];
    int n = 0;
    auto push = [&](float z, float wx, float h, int kind, bool flip) {
        if (n < 80 && z > pz_ + 0.5f && z < pz_ + 56.f) items[n++] = {z, wx, h, kind, flip};
    };

    float gloomBase = std::floor(pz_ / 18.f) * 18.f;
    for (int i = 0; i < 5; i++) {
        float z = gloomBase + i * 18.f + 6.f;
        push(z, -2.55f, 2.4f, 1, false);
        push(z + 9.f, 2.55f, 2.8f, 2, true);
    }
    for (int i = 0; i < NG; i++) {
        const Gate& g = kGates[i];
        float left = g.cx - g.hw - 0.35f;
        float right = g.cx + g.hw + 0.35f;
        push(g.z, left, 2.2f, 1, false);
        push(g.z, right, 2.2f, 1, true);
        push(g.z - 4.f, left - 0.35f, 3.1f, 2, false);
        push(g.z - 4.f, right + 0.35f, 3.1f, 2, true);
    }
    push(LEN, 0.f, 2.6f, 3, false);
    if (storm_ < 18.f || mode_ == Mode::Title) {
        float drift = t_ * 6.f;
        push(pz_ + 22.f, -1.2f + std::sin(drift) * 0.3f, 1.1f, 4, false);
        push(pz_ + 34.f, 1.4f, 1.4f, 4, true);
        push(pz_ + 16.f, 0.2f, 0.9f, 4, false);
    }

    std::sort(items, items + n, [](const Item& a, const Item& b) { return a.z < b.z; });
    for (int i = n - 1; i >= 0; i--) {
        float sx, sy, ppm;
        project(items[i].wx, items[i].z, sx, sy, ppm);
        if (sx < -60 || sx > 380) continue;
        float h = ppm * items[i].h;
        int k = items[i].kind;
        if (k == 1) spr(art_.rock, sx, sy, h, PAL_ROCK, items[i].flip);
        else if (k == 2) spr(art_.spire, sx, sy, h, PAL_ROCK, items[i].flip);
        else if (k == 3) spr(art_.crest, sx, sy, h, PAL_CREST, false);
        else if (k == 4) spr(art_.cloud, sx, sy - h * 0.4f, h, PAL_INK, items[i].flip);
    }
}

}  // namespace headerpass

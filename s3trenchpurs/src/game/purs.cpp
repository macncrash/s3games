#include "game/purs.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace purs {
namespace {
constexpr float WATCH_LEN = 58.f;
constexpr float DT = 1.f / 60.f;
}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    if (bot_) beginWatch();
}

void Game::beginWatch() {
    mode_ = Mode::Watch;
    kills_ = 0;
    breaches_ = 0;
    aim_ = 160;
    heat_ = 0;
    run_ = 1;
    idle_ = 0;
    watch_ = 0;
    spawn_ = 0.4f;
    shotCd_ = 0;
    muzzle_ = 0;
    t_ = 0;
    jammed_ = false;
    why_ = "";
    foes_.clear();
    won_ = false;
    over_ = false;
}

void Game::fail(const char* why) {
    why_ = why;
    mode_ = Mode::Lose;
    won_ = false;
    over_ = true;
}

float Game::rnd() {
    rng_ = rng_ * 1664525u + 1013904223u;
    return float((rng_ >> 8) & 0xffff) / 65536.f;
}

void Game::botThink(bool& fire, bool& clear) {
    fire = false;
    clear = false;
    const Foe* best = nullptr;
    for (const Foe& f : foes_) {
        if (!f.alive) continue;
        if (!best || f.z < best->z) best = &f;
    }
    if (best) aim_ = best->x;
    if (jammed_) {
        clear = true;
        return;
    }
    if (best && (heat_ < 0.58f || best->z < 0.30f)) fire = true;
    else if (!best && idle_ > 0.35f) fire = true;
}

void Game::update(float dt) {
    gs::Pad& pad = sys_->pad;
    bool fire = false, clear = false;
    if (bot_) {
        botThink(fire, clear);
    } else {
        float ax = pad.axisX;
        if (pad.down(gs::BTN_LEFT)) ax -= 1;
        if (pad.down(gs::BTN_RIGHT)) ax += 1;
        ax = std::max(-1.f, std::min(1.f, ax));
        aim_ += ax * 240.f * dt;
        fire = pad.down(gs::BTN_A) || pad.down(gs::BTN_C) || pad.accel > 0.3f;
        clear = pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_X);
    }
    aim_ = std::max(36.f, std::min(284.f, aim_));

    if (clear && jammed_) {
        jammed_ = false;
        heat_ = 0.40f;
        sys_->apu.tone(1, 180, 0.08f);
    }

    shotCd_ -= dt;
    if (fire && !jammed_ && shotCd_ <= 0) {
        shotCd_ = 0.10f;
        idle_ = 0;
        heat_ += 0.052f;
        muzzle_ = 0.08f;
        sys_->apu.noiseBurst(0.18f, 14000, 0.04f);
        Foe* hit = nullptr;
        for (Foe& f : foes_) {
            if (!f.alive) continue;
            float tol = 16.f + (1.f - f.z) * 8.f;
            if (std::fabs(f.x - aim_) <= tol && (!hit || f.z < hit->z)) hit = &f;
        }
        if (hit) {
            hit->alive = false;
            hit->flash = 0.12f;
            kills_++;
            sys_->apu.tone(2, 90, 0.06f);
        }
    } else {
        idle_ += dt;
        if (!jammed_) heat_ -= 0.50f * dt;
    }
    heat_ = std::max(0.f, std::min(1.f, heat_));
    if (heat_ >= 1.f) jammed_ = true;
    muzzle_ = std::max(0.f, muzzle_ - dt);

    if (jammed_) run_ -= 0.22f * dt;
    else if (idle_ > 0.70f) run_ -= 0.10f * dt;
    else run_ += 0.14f * dt;
    run_ = std::max(0.f, std::min(1.f, run_));
    if (run_ <= 0.f) {
        fail("THE MACHINE STOPPED");
        return;
    }

    float phase = watch_ / WATCH_LEN;
    float every = phase < 0.33f ? 0.85f : phase < 0.66f ? 0.68f : 0.55f;
    float speed = phase < 0.33f ? 0.15f : phase < 0.66f ? 0.18f : 0.21f;
    spawn_ -= dt;
    if (spawn_ <= 0 && watch_ < WATCH_LEN - 1.2f) {
        spawn_ = every;
        Foe f;
        f.x = 48.f + rnd() * 224.f;
        f.z = 1.f;
        f.flash = 0;
        f.alive = true;
        foes_.push_back(f);
    }
    for (Foe& f : foes_) {
        if (f.flash > 0) f.flash -= dt;
        if (!f.alive) continue;
        f.z -= speed * dt;
        if (f.z <= 0.f) {
            f.alive = false;
            breaches_++;
            sys_->apu.noiseBurst(0.22f, 4000, 0.12f);
        }
    }
    if (breaches_ >= 3) {
        fail("THEY ARE IN THE TRENCH");
        return;
    }
    foes_.erase(std::remove_if(foes_.begin(), foes_.end(),
                               [](const Foe& f) { return !f.alive && f.flash <= 0; }),
                foes_.end());

    watch_ += dt;
    if (watch_ >= WATCH_LEN) {
        mode_ = Mode::Win;
        won_ = true;
        over_ = true;
        why_ = "THE MACHINE HELD";
        sys_->apu.tone(0, 330, 0.12f);
    }
}

void Game::spr(const gs::Mipped& m, float cx, float footY, float h, int pal, int fog, bool flip) {
    if (h < 1.f || m.h <= 0) return;
    float s = h / float(m.h);
    gs::Sprite sp;
    sp.img = m.pick(h);
    sp.w = std::max(1, int(std::lround(m.w * s)));
    sp.h = std::max(1, int(std::lround(h)));
    sp.x = int(std::lround(cx - sp.w * 0.5f));
    sp.y = int(std::lround(footY - sp.h));
    sp.pal = uint8_t(pal);
    sp.fog = uint8_t(std::max(0, std::min(16, fog)));
    sp.hflip = flip;
    sys_->vdp.sprite(sp);
}

void Game::text(const std::string& s, float x, float y, int pal) {
    float cx = x;
    for (char ch : s) {
        int gi = int(static_cast<unsigned char>(ch));
        if (gi >= 32 && gi < 128) spr(art_.glyph[gi - 32], cx + 3, y + 8, 8, pal);
        cx += 6;
    }
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    const int hor = 86;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        gs::RoadLine& r = vdp.road[y];
        r = {};
        if (y < hor) {
            int n = y * 8 / hor;
            vdp.lineBackdrop[y] = gs::rgb4(1, 1, 3 + n / 3);
            vdp.lineFog[y] = 0;
        } else {
            float t = float(y - hor) / float(gs::SCREEN_H - 1 - hor);
            vdp.lineBackdrop[y] = gs::rgb4(2, 3, 2);
            vdp.lineFog[y] = uint8_t((1.f - t) * 10);
            r.on = true;
            r.cx = 160;
            r.hw = 18.f + t * t * 168.f;
            r.v = (1.f - t) * 1800.f + t_ * 40.f;
            r.pal = PAL_ROAD;
            r.band = (int(r.v / 80.f) & 1);
            r.style = gs::ROAD_MUD;
            r.left = gs::GROUND_LAND;
            r.right = gs::GROUND_LAND;
        }
    }

    if (mode_ == Mode::Title) {
        text("S3 TRENCH PURS", 112, 48, PAL_AMBER);
        text("THE LAST MACHINE", 108, 78, PAL_INK);
        text("STILL RUNNING", 120, 92, PAL_INK);
        text("KEEP THE GUN CYCLING", 96, 120, PAL_INK);
        text("MISS THAT AND THE", 108, 136, PAL_ALERT);
        text("WATCH IS OVER", 120, 148, PAL_ALERT);
        if (int(t_ * 2) & 1) text("PRESS START", 124, 180, PAL_AMBER);
        text("ARROWS AIM   A FIRE   B CLEAR", 70, 204, PAL_INK);
        return;
    }

    // Far posts, then raiders, then the lip. Earlier sprites sit on top.
    for (int i = 0; i < 7; i++) {
        float z = 0.25f + (i % 3) * 0.22f;
        float y = 86.f + (1.f - z) * 108.f;
        float x = 30.f + i * 44.f;
        int fog = int(z * 12);
        spr(art_.post, x, y, 8.f + (1.f - z) * 16.f, PAL_DEAD, fog);
    }
    std::vector<int> order;
    order.reserve(foes_.size());
    for (int i = 0; i < int(foes_.size()); i++) order.push_back(i);
    std::sort(order.begin(), order.end(), [&](int a, int b) { return foes_[a].z < foes_[b].z; });
    for (int i = int(order.size()) - 1; i >= 0; i--) {
        const Foe& f = foes_[order[i]];
        float y = 90.f + (1.f - f.z) * 112.f;
        float h = 10.f + (1.f - f.z) * 36.f;
        int fog = int(f.z * 14);
        if (f.alive) spr(art_.foe, f.x, y, h, PAL_FOE, fog, f.x < aim_);
        if (f.flash > 0) spr(art_.flash, f.x, y - h * 0.4f, h * 0.45f, PAL_FX, fog);
    }

    for (int i = 0; i < 8; i++) spr(art_.bag, 28.f + i * 38.f, 214, 16, PAL_BAG);
    spr(art_.dead, 48, 206, 28, PAL_DEAD, 0, false);
    spr(art_.dead, 276, 206, 28, PAL_DEAD, 0, true);
    float gunX = aim_;
    spr(art_.gun, gunX, 222, 52, PAL_GUN);
    if (muzzle_ > 0) spr(art_.flash, gunX + 28, 196, 18, PAL_FX);

    for (int i = 0; i < 12; i++) {
        float sx = float((i * 47 + 13) % 300) + 8;
        float sy = float(10 + (i * 17) % 60);
        spr(art_.star, sx, sy, 3, (i % 4 == 0) ? PAL_FX : PAL_INK);
    }
    if ((int(t_ * 3) % 5) == 0) spr(art_.flare, 250, 40, 14, PAL_ALERT);

    char buf[64];
    int sec = std::max(0, int(std::ceil(WATCH_LEN - watch_)));
    std::snprintf(buf, sizeof(buf), "WATCH %d:%02d", sec / 60, sec % 60);
    text(buf, 8, 6, PAL_INK);
    std::snprintf(buf, sizeof(buf), "KILLS %d", kills_);
    text(buf, 230, 6, PAL_AMBER);
    std::snprintf(buf, sizeof(buf), "BREACH %d/3", breaches_);
    text(buf, 8, 18, breaches_ ? PAL_ALERT : PAL_INK);

    text("RUN", 8, 198, PAL_INK);
    int runCells = int(run_ * 10 + 0.001f);
    for (int i = 0; i < 10; i++) text(i < runCells ? "=" : "-", 32 + i * 6, 198, i < runCells ? PAL_AMBER : PAL_INK);
    text(jammed_ ? "JAM" : "HEAT", 110, 198, jammed_ ? PAL_ALERT : PAL_INK);
    int heatCells = int(heat_ * 8 + 0.001f);
    for (int i = 0; i < 8; i++) text(i < heatCells ? "=" : "-", 140 + i * 6, 198, i < heatCells ? PAL_ALERT : PAL_INK);

    if (mode_ == Mode::Win) {
        text("THE MACHINE HELD", 108, 78, PAL_AMBER);
        text("WATCH COMPLETE", 114, 96, PAL_INK);
    } else if (mode_ == Mode::Lose) {
        text(why_, 320 / 2.f - float(std::strlen(why_) * 6) / 2.f, 78, PAL_ALERT);
        text("THE WATCH IS OVER", 108, 96, PAL_INK);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    if (mode_ == Mode::Title) {
        if (bot_ || sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A)) beginWatch();
    } else if (mode_ == Mode::Watch) {
        update(DT);
    } else if (!bot_ && (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A))) {
        over_ = false;
        won_ = false;
        mode_ = Mode::Title;
    }
    draw();
}

}  // namespace purs

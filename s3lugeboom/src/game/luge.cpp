#include "game/luge.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace luge {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float HORIZON = 90.f;
constexpr float PROJ_K = 220.f;
constexpr float ROAD_P = 268.f;
constexpr float FINISH = 640.f;
constexpr float CLOCK0 = 36.f;
constexpr float RIVAL0 = 46.f;
constexpr float BEND_DZ = 4.f;
constexpr int BEND_N = 64;
constexpr float CURVE_K = 0.016f;
constexpr float PULL_K = 0.011f;
constexpr float STEER_K = 1.85f;
constexpr float PI = 3.14159265f;

}  // namespace

float Game::curvature(float s) const {
    const float u = s / FINISH;
    float c = 0.42f * std::sin(u * PI * 3.1f);
    c += 0.22f * std::sin(u * PI * 6.4f + 0.7f);
    c *= std::sin(std::clamp(u, 0.f, 1.f) * PI);
    if (s < 70.f) c *= s / 70.f;
    if (s > FINISH - 140.f) c *= std::max(0.f, (FINISH - s) / 140.f);
    return std::clamp(c, -1.f, 1.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    propN_ = 0;
    auto add = [&](float s, float x, int kind) {
        if (propN_ < 28) props_[propN_++] = {s, x, kind};
    };
    for (int i = 0; i < 10; i++) {
        add(50.f + float(i) * 58.f, (i & 1) ? -1.85f : 1.95f, 0);
        add(78.f + float(i) * 58.f, (i & 1) ? 2.15f : -2.05f, 0);
    }
    const float rocks[][2] = {{140, 0.72f}, {210, -0.78f}, {300, 0.7f}, {390, -0.74f}, {470, 0.76f}, {540, -0.7f}};
    for (auto& r : rocks) add(r[0], r[1], 1);
    won_ = false;
    over_ = false;
    std::snprintf(result_, sizeof result_, "S3 LUGE BOOM  FAIL  unfinished");
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.apu.setMaster(0.75f);
    if (bot_) resetRun();
    else mode_ = Mode::Title;
}

void Game::resetRun() {
    mode_ = Mode::Run;
    s_ = 0;
    x_ = 0;
    speed_ = 18.f;
    clock_ = CLOCK0;
    elapsed_ = 0;
    shake_ = 0;
    scrapeCd_ = 0;
    lean_ = 0;
    drive_ = 3;
    rival0_ = RIVAL0;
    won_ = false;
    over_ = false;
    if (!windOn_) {
        gs::FMPatch p;
        p.alg = 4;
        p.vol = 0.2f;
        p.op[0].level = 1;
        p.op[0].ar = 0.4f;
        p.op[0].dr = 0.4f;
        p.op[0].sl = 1;
        p.op[0].rr = 0.3f;
        sys_->apu.setPatch(0, p);
        sys_->apu.keyOn(0, 70.f, 0.05f);
        windOn_ = true;
    }
}

void Game::steerOf(float& steer, bool& tuck, bool& brake) const {
    steer = 0;
    tuck = false;
    brake = false;
    if (bot_) {
        tuck = true;
        const float pull = curvature(s_) * speed_ * PULL_K;
        steer = std::clamp(-x_ * 2.4f - pull / STEER_K, -1.f, 1.f);
        return;
    }
    const gs::Pad& pad = sys_->pad;
    float axis = pad.axisX;
    if (pad.down(gs::BTN_LEFT)) axis -= 1.f;
    if (pad.down(gs::BTN_RIGHT)) axis += 1.f;
    steer = std::clamp(axis, -1.f, 1.f);
    tuck = pad.down(gs::BTN_A) || pad.accel > 0.4f;
    brake = pad.down(gs::BTN_B) || pad.brake > 0.4f;
}

void Game::update(float dt) {
    t_ += dt;
    float steer = 0;
    bool tuck = false, brake = false;
    steerOf(steer, tuck, brake);
    lean_ += (steer - lean_) * std::min(1.f, dt * 8.f);
    float target = tuck ? 30.f : 23.f;
    if (brake) target = 14.f;
    speed_ += (target - speed_) * std::min(1.f, dt * 1.6f);
    const float pull = curvature(s_) * speed_ * PULL_K;
    x_ += (steer * STEER_K + pull) * dt;
    s_ += speed_ * dt;
    elapsed_ += dt;
    clock_ -= dt;
    if (shake_ > 0) shake_ -= dt;
    if (scrapeCd_ > 0) scrapeCd_ -= dt;

    bool hit = std::fabs(x_) > 1.05f;
    if (!hit) {
        for (int i = 0; i < propN_; i++) {
            if (props_[i].kind != 1) continue;
            if (std::fabs(props_[i].s - s_) < 2.4f && std::fabs(props_[i].x - x_) < 0.2f) hit = true;
        }
    }
    if (hit && scrapeCd_ <= 0.f && drive_ > 0) {
        scrapeCd_ = 0.55f;
        drive_--;
        speed_ *= 0.62f;
        x_ = std::copysign(std::min(std::fabs(x_), 0.86f), x_);
        shake_ = 0.35f;
        sys_->apu.noiseBurst(0.35f, 240.f, 0.2f);
        sys_->rumble(0.45f, 0.2f, 120);
    }

    if (drive_ <= 0) {
        mode_ = Mode::Fail;
        std::snprintf(result_, sizeof result_, "S3 LUGE BOOM  FAIL  drive lost on the ice");
        over_ = true;
        sys_->setLight(180, 30, 24);
        return;
    }
    if (clock_ <= 0.f && s_ < FINISH) {
        mode_ = Mode::Fail;
        clock_ = 0;
        std::snprintf(result_, sizeof result_, "S3 LUGE BOOM  FAIL  the other crew took the boom");
        over_ = true;
        sys_->setLight(180, 30, 24);
        return;
    }
    if (s_ >= FINISH) {
        s_ = FINISH;
        mode_ = Mode::Victory;
        won_ = true;
        over_ = true;
        std::snprintf(result_, sizeof result_,
                      "S3 LUGE BOOM  PASS  drive on the boom  crew clock %.1fs", clock_);
        sys_->apu.tone(1, 523.f, 0.12f);
        sys_->setLight(40, 180, 90);
    }
}

void Game::cacheBend() {
    for (int i = 0; i < BEND_N; i++) {
        const float dist = float(i) * BEND_DZ;
        if (dist <= 0.05f) {
            bend_[i] = 0;
            continue;
        }
        const int n = 10;
        const float du = dist / float(n);
        float head = 0, bend = 0;
        for (int k = 0; k < n; k++) {
            const float c = curvature(s_ + (float(k) + 0.5f) * du);
            head += c * CURVE_K * du;
            bend += head * du;
        }
        bend_[i] = bend;
    }
}

float Game::bendTo(float dist) const {
    if (dist <= 0.f) return 0.f;
    const float u = dist / BEND_DZ;
    const int i = std::min(BEND_N - 2, std::max(0, int(u)));
    const float f = std::clamp(u - float(i), 0.f, 1.f);
    return bend_[i] * (1.f - f) + bend_[i + 1] * f;
}

bool Game::project(float wx, float wz, float& sx, float& sy, float& scale, int& fog) const {
    const float z = wz - s_;
    if (z < 0.5f) return false;
    sy = HORIZON + PROJ_K / z;
    scale = ROAD_P / z;
    const float cam = (shake_ > 0.f) ? std::sin(t_ * 48.f) * 4.f * shake_ : 0.f;
    sx = 160.f + (bendTo(z) + wx - x_) * scale + cam;
    fog = std::clamp(int((z - 14.f) / 8.f), 0, 12);
    return sy > -40.f && sy < gs::SCREEN_H + 40.f;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet, bool shadow) {
    if (h < 1.5f || m.h <= 0) return;
    const float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 80 || s.x + s.w < -80 || s.y > gs::SCREEN_H + 40 || s.y + s.h < -40) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    for (size_t i = 0; i < s.size(); i++) {
        const int x = col + int(i);
        const unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    const float adv = 18.f * scale;
    x -= float(s.size()) * adv * 0.5f;
    for (size_t i = 0; i < s.size(); i++) {
        const unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + float(i) * adv + g.w * scale * 0.5f, y, g.h * scale, pal, false, 0, false, false);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.roadTime++;
    cacheBend();
    const uint16_t skyTop = gs::rgb4(2, 3, 8);
    const uint16_t skyMid = gs::rgb4(6, 8, 13);
    const uint16_t horC = gs::rgb4(12, 13, 14);
    const float cam = (shake_ > 0.f) ? std::sin(t_ * 48.f) * 3.f * shake_ : 0.f;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (float(y) < HORIZON) {
            const float u = float(y) / HORIZON;
            v.lineBackdrop[y] = u < 0.55f ? skyTop : (u < 0.85f ? skyMid : horC);
            v.lineFog[y] = 0;
            v.road[y].on = false;
            continue;
        }
        const float row = std::max(0.8f, float(y) - HORIZON);
        const float z = PROJ_K / row;
        gs::RoadLine& r = v.road[y];
        r.on = true;
        r.hw = ROAD_P / z;
        r.cx = 160.f + (bendTo(z) - x_) * r.hw + cam;
        r.v = (s_ + z) * 4.f;
        r.pal = PAL_ROAD;
        r.style = gs::ROAD_ICE;
        r.band = (int(std::floor((s_ + z) / 22.f)) & 1) ? 1 : 0;
        r.left = r.right = gs::GROUND_SNOWWALL;
        v.lineFog[y] = uint8_t(std::clamp(int((z - 18.f) / 10.f), 0, 10));
        v.lineBackdrop[y] = horC;
    }

    if (mode_ == Mode::Title) {
        text("S3 LUGE BOOM", 160, 36, 0.72f, PAL_INK);
        text("DELIVER THE DRIVE", 160, 58, 0.42f, PAL_AMBER);
    } else if (mode_ == Mode::Victory) {
        text("DRIVE ON THE BOOM", 160, 40, 0.5f, PAL_INK);
    } else if (mode_ == Mode::Fail) {
        text("BOOM LOST", 160, 40, 0.62f, PAL_AMBER);
    }

    const int frame = lean_ > 0.28f ? 2 : lean_ < -0.28f ? 0 : 1;
    const float bob = std::sin(t_ * speed_ * 0.35f) * 1.4f;
    spr(art_.luge[frame], 160.f + lean_ * 18.f + cam, 208.f + bob, 86.f, PAL_RIDER, false, 0, true, false);
    spr(art_.shadow, 164.f + cam, 210.f, 18.f, PAL_FX, false, 0, true, true);
    for (int i = 0; i < 18; i++) {
        const float fx = std::fmod(float(i * 47) + t_ * (18.f + float(i % 5) * 6.f), 320.f);
        const float fy = std::fmod(float(i * 29) + t_ * (22.f + float(i % 3) * 9.f), 200.f);
        spr(art_.flake, fx, fy, 3.f + float(i % 3), PAL_FX, false, 0, false, false);
    }

    struct Item {
        float z;
        int kind;
        int index;
    };
    Item items[40];
    int n = 0;
    auto push = [&](float z, int kind, int index) {
        if (n < 40 && z > 0.6f && z < 110.f) items[n++] = {z, kind, index};
    };
    for (int i = 0; i < propN_; i++) push(props_[i].s - s_, props_[i].kind, i);
    const float rivalS = rival0_ + elapsed_ * ((FINISH - rival0_) / CLOCK0);
    push(rivalS - s_, 2, 0);
    push(FINISH - s_, 3, 0);
    std::sort(items, items + n, [](const Item& a, const Item& b) { return a.z < b.z; });
    for (int i = 0; i < n; i++) {
        const Item& it = items[i];
        if (it.kind == 3) {
            float sx, sy, scale;
            int fog;
            if (!project(0.f, FINISH, sx, sy, scale, fog)) continue;
            spr(art_.boom, sx, sy, std::min(150.f, 3.1f * scale), PAL_BOOM, false, fog, true, false);
            if (it.z < 40.f) text("BOOM", sx, sy - std::min(150.f, 3.1f * scale) - 6.f, 0.45f, PAL_AMBER);
            continue;
        }
        if (it.kind == 2) {
            float sx, sy, scale;
            int fog;
            const float rx = 0.28f * std::sin(elapsed_ * 1.3f);
            if (!project(rx, rivalS, sx, sy, scale, fog)) continue;
            spr(art_.rival, sx, sy, std::min(120.f, 1.15f * scale), PAL_RIVAL, false, fog, true, false);
            continue;
        }
        const Prop& p = props_[it.index];
        float sx, sy, scale;
        int fog;
        if (!project(p.x, p.s, sx, sy, scale, fog)) continue;
        if (p.kind == 0) spr(art_.tree, sx, sy, std::min(130.f, 2.4f * scale), PAL_TREE, p.x < 0, fog, true, false);
        else spr(art_.rock, sx, sy, std::min(48.f, 0.55f * scale), PAL_ROCK, false, fog, true, false);
    }

    if (mode_ == Mode::Title) {
        hudC(18, "THE CLOCK IS THE OTHER CREW", PAL_HUD);
        hudC(21, "LEFT RIGHT STEER", PAL_HUD);
        hudC(22, "A TUCK   B BRAKE   START", PAL_HUD);
    } else if (mode_ == Mode::Run || mode_ == Mode::Pause) {
        char line[48];
        std::snprintf(line, sizeof line, "DRIVE %d/3", drive_);
        hud(1, 1, line, PAL_HUD);
        std::snprintf(line, sizeof line, "CREW %4.1f", std::max(0.f, clock_));
        hud(28, 1, line, clock_ < 8.f ? PAL_HUD : PAL_HUD);
        const int boomM = int(std::max(0.f, FINISH - s_));
        std::snprintf(line, sizeof line, "BOOM %d", boomM);
        hud(1, 2, line, PAL_HUD);
        if (mode_ == Mode::Pause) hudC(12, "PAUSED", PAL_HUD);
    } else if (mode_ == Mode::Victory) {
        hudC(8, "THE OTHER CREW IS STILL OUT", PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        hudC(8, result_ + 14, PAL_HUD);
    }
    if (windOn_ && (mode_ == Mode::Run || mode_ == Mode::Title)) {
        sys_->apu.setFreq(0, 64.f + speed_ * 1.6f);
        sys_->apu.setVol(0, 0.04f + speed_ * 0.002f);
        sys_->apu.noise(0.02f + speed_ * 0.0012f, 900.f + speed_ * 30.f, false);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        draw();
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) resetRun();
        return;
    }
    if (mode_ == Mode::Pause) {
        draw();
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Run;
        return;
    }
    if (mode_ == Mode::Victory || mode_ == Mode::Fail) {
        draw();
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
            over_ = false;
            won_ = false;
            mode_ = Mode::Title;
        }
        return;
    }
    if (!bot_ && pad.pressed(gs::BTN_START)) {
        mode_ = Mode::Pause;
        draw();
        return;
    }
    update(DT);
    draw();
    if (mode_ == Mode::Run) sys.setLight(50, 120, 190);
}

}  // namespace luge

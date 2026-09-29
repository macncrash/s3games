#include "game/mush.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

#include "version.h"

namespace mush {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr float GOAL = 1000.0f;
constexpr int HORIZON = 86;
constexpr float FOCAL = 210.0f;
constexpr float LANE = 1.35f;

int lerpC(int a, int b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    return gs::rgb4(int(ar + (br - ar) * t), int(ag + (bg - ag) * t), int(ab + (bb - ab) * t));
}

}  // namespace

float Game::rnd() {
    rng_ = rng_ * 1664525u + 1013904223u;
    return (rng_ >> 8) * (1.0f / 16777216.0f);
}

void Game::layCourse() {
    wheels_.clear();
    float z = 42.0f;
    while (z < GOAL - 24.0f) {
        bool edges = rnd() < 0.28f;
        if (edges) {
            int kind = int(rnd() * 3.0f);
            wheels_.push_back({0, z, kind % 3});
            wheels_.push_back({2, z, (kind + 1) % 3});
        } else {
            int lane = int(rnd() * 3.0f);
            if (lane > 2) lane = 2;
            wheels_.push_back({lane, z, int(rnd() * 3.0f) % 3});
        }
        z += 18.0f + rnd() * 8.0f;
    }
}

void Game::beginRun() {
    dist_ = 0;
    meters_ = 0;
    lane_ = 1;
    want_ = 1;
    over_ = false;
    won_ = false;
    t_ = 0;
    speed_ = 14.0f;
    layCourse();
    mode_ = Mode::Run;
}

void Game::steerBot() {
    float nearZ = 1.0e9f;
    for (const Wheel& w : wheels_) {
        if (w.z > 1.2f && w.z < nearZ) nearZ = w.z;
    }
    bool blocked[3] = {};
    if (nearZ < 16.0f) {
        for (const Wheel& w : wheels_) {
            if (std::fabs(w.z - nearZ) < 3.0f && w.lane >= 0 && w.lane <= 2) blocked[w.lane] = true;
        }
    }
    int cur = want_;
    if (cur < 0 || cur > 2 || !blocked[cur]) return;
    if (cur > 0 && !blocked[cur - 1]) want_ = cur - 1;
    else if (cur < 2 && !blocked[cur + 1]) want_ = cur + 1;
}

void Game::update(float dt) {
    if (mode_ != Mode::Run) return;
    if (bot_) steerBot();
    float dl = float(want_) - lane_;
    float step = 7.5f * dt;
    if (std::fabs(dl) <= step) lane_ = float(want_);
    else lane_ += (dl > 0 ? step : -step);

    dist_ += speed_ * dt;
    if (dist_ > GOAL) dist_ = GOAL;
    meters_ = int(dist_);
    for (Wheel& w : wheels_) w.z -= speed_ * dt;

    for (const Wheel& w : wheels_) {
        if (w.z < 2.55f && w.z > 1.15f && std::fabs(float(w.lane) - lane_) < 0.42f) {
            mode_ = Mode::Fail;
            over_ = true;
            won_ = false;
            sys_->apu.noiseBurst(0.5f, 700.0f, 0.25f);
            return;
        }
    }
    if (dist_ >= GOAL) {
        mode_ = Mode::Victory;
        over_ = true;
        won_ = true;
        meters_ = 1000;
        sys_->apu.tone(0, 660.0f, 0.08f);
        sys_->apu.tone(1, 880.0f, 0.07f);
    }
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    const float adv = 16.0f * scale;
    float left = x - float(s.size()) * adv * 0.5f;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, left + i * adv + g.w * scale * 0.5f, y, g.h * scale, pal, false, 0, false);
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 80 || s.x + s.w < -80 || s.y > gs::SCREEN_H + 40 || s.y + s.h < -80) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.setFogColor(skyHor_);
    const float shift = (lane_ - 1.0f);

    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < HORIZON) {
            v.lineBackdrop[y] = lerpC(skyTop_, skyHor_, y / float(HORIZON));
            v.lineFog[y] = 0;
            v.road[y].on = false;
            continue;
        }
        float row = float(y - HORIZON) + 1.0f;
        gs::RoadLine& r = v.road[y];
        r.on = true;
        r.hw = 26.0f + row * 1.55f;
        r.cx = 160.0f - shift * (10.0f + row * 0.22f);
        r.v = dist_ * 4.0f + 2800.0f / row;
        r.pal = PAL_FIELD;
        r.band = (int(std::floor(r.v / 28.0f)) & 1) ? 1 : 0;
        r.style = gs::ROAD_RUTS;
        r.left = r.right = gs::GROUND_LAND;
        v.lineFog[y] = uint8_t(std::clamp(int(12 - row / 8.0f), 0, 12));
        v.lineBackdrop[y] = skyHor_;
    }

    auto project = [&](float laneX, float z, float& sx, float& sy, int& fog) {
        float zz = std::max(0.9f, z);
        float s = FOCAL / zz;
        sx = 160.0f + (laneX - shift * LANE) * s;
        sy = float(HORIZON) + 340.0f / zz;
        fog = std::clamp(int((zz - 10.0f) / 3.0f), 0, 14);
    };

    struct Spr {
        float z;
        int kind;
        float sx, sy, h;
        int pal, fog;
        bool feet;
    };
    std::vector<Spr> far;
    for (int side = -1; side <= 1; side += 2) {
        for (int i = 0; i < 8; i++) {
            float base = float(i) * 22.0f + (side < 0 ? 6.0f : 14.0f);
            float z = std::fmod(base - dist_, 176.0f);
            if (z < 0) z += 176.0f;
            if (z < 2.0f || z > 70.0f) continue;
            float sx, sy;
            int fog;
            project(side * 4.4f, z, sx, sy, fog);
            far.push_back({z, 3, sx, sy, 3.2f * FOCAL / z, PAL_POST, fog, true});
        }
    }
    for (int m = 100; m <= 1000; m += 100) {
        float z = float(m) - dist_;
        if (z < 1.5f || z > 80.0f) continue;
        float sx, sy;
        int fog;
        project(2.4f, z, sx, sy, fog);
        far.push_back({z, 4, sx, sy, 2.4f * FOCAL / z, PAL_POST, fog, true});
    }
    for (const Wheel& w : wheels_) {
        if (w.z < 1.15f || w.z > 55.0f) continue;
        float sx, sy;
        int fog;
        float lx = (float(w.lane) - 1.0f) * LANE;
        project(lx, w.z, sx, sy, fog);
        float h = (w.kind == 0 ? 1.15f : w.kind == 1 ? 1.7f : 1.45f) * FOCAL / std::max(0.9f, w.z);
        int pal = w.kind == 0 ? PAL_WHEEL : w.kind == 1 ? PAL_CART : PAL_BIKE;
        far.push_back({w.z, w.kind, sx, sy, h, pal, fog, true});
    }
    std::sort(far.begin(), far.end(), [](const Spr& a, const Spr& b) { return a.z > b.z; });
    for (const Spr& s : far) {
        const gs::Mipped* img = &art_.wheel;
        if (s.kind == 1) img = &art_.cart;
        else if (s.kind == 2) img = &art_.bike;
        else if (s.kind == 3) img = &art_.tree;
        else if (s.kind == 4) img = &art_.post;
        spr(*img, s.sx, s.sy, s.h, s.pal, false, s.fog, true);
    }

    if (mode_ != Mode::Title) {
        int frame = int(t_ * 8.0f) & 1;
        float h = mode_ == Mode::Fail ? 36.0f : 54.0f;
        spr(art_.mush[frame], 160, 208, h, PAL_MUSH, false, 0, true);
    } else {
        float bob = std::sin(t_ * 3.0f) * 4.0f;
        spr(art_.mush[int(t_ * 6.0f) & 1], 160, 168 + bob, 70, PAL_MUSH, false, 0, true);
        spr(art_.cart, 64, 176, 48, PAL_CART, false, 0, true);
        spr(art_.bike, 256, 176, 44, PAL_BIKE, false, 0, true);
    }

    if (mode_ == Mode::Title) {
        text("MUSHKILO", 160, 36, 1.35f, PAL_RED);
        text("ONE KILOMETER", 160, 68, 0.7f, PAL_AMBER);
        text("DO NOT TOUCH A WHEEL", 160, 92, 0.55f, PAL_HUD);
        hudC(24, "START  RUN", PAL_GREEN);
    } else if (mode_ == Mode::Run) {
        char buf[40];
        std::snprintf(buf, sizeof buf, "%d M", meters_);
        hud(1, 1, buf, PAL_AMBER);
        std::snprintf(buf, sizeof buf, "%d TO GO", std::max(0, 1000 - meters_));
        hud(39 - int(std::strlen(buf)), 1, buf, PAL_HUD);
        hud(1, 26, "LANE", PAL_HUD);
        hud(6, 26, want_ == 0 ? "LEFT" : want_ == 2 ? "RIGHT" : "MID", PAL_GREEN);
    } else if (mode_ == Mode::Fail) {
        text("WHEEL", 160, 64, 1.3f, PAL_RED);
        char buf[40];
        std::snprintf(buf, sizeof buf, "%d M", meters_);
        hudC(12, buf, PAL_HUD);
        hudC(24, "START  AGAIN", PAL_AMBER);
    } else if (mode_ == Mode::Victory) {
        text("KILOMETER", 160, 58, 1.05f, PAL_GREEN);
        text("CLEAN", 160, 88, 1.1f, PAL_AMBER);
        hudC(24, "NO WHEEL TOUCHED", PAL_HUD);
    }
    hud(39 - int(std::strlen(S3_VERSION_STRING)), 26, S3_VERSION_STRING, PAL_HUD);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    skyTop_ = gs::rgb4(3, 6, 12);
    skyHor_ = gs::rgb4(12, 14, 15);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.apu.setMaster(0.8f);
    if (bot_) beginRun();
    else mode_ = Mode::Title;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) {
            beginRun();
            sys.apu.tone(0, 520.0f, 0.06f);
        } else if (pad.pressed(gs::BTN_MODE)) {
            sys.quit();
        }
    } else if (mode_ == Mode::Run) {
        if (!bot_) {
            if (pad.pressed(gs::BTN_LEFT) && want_ > 0) {
                want_--;
                sys.apu.tone(0, 340.0f, 0.04f);
            }
            if (pad.pressed(gs::BTN_RIGHT) && want_ < 2) {
                want_++;
                sys.apu.tone(0, 420.0f, 0.04f);
            }
            if (std::fabs(pad.axisX) > 0.45f) {
                int w = pad.axisX < 0 ? 0 : 2;
                if (w != want_) {
                    want_ = w > want_ ? want_ + 1 : want_ - 1;
                }
            }
            if (pad.pressed(gs::BTN_START)) {
                mode_ = Mode::Title;
            }
        }
        update(DT);
    } else if (mode_ == Mode::Fail) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) beginRun();
    }
    draw();
}

}  // namespace mush

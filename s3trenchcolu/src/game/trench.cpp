#include "game/trench.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace trench {
namespace {
constexpr float kHorizon = 78.f;
constexpr float kDepth = 140.f;
constexpr float kNear = 3.4f;
}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Play) return 1;
    if (mode_ == Mode::Victory) return 2;
    return 3;
}

void Game::spawn(bool scenic) {
    trucks_.clear();
    const float lead = scenic ? 36.f : 54.f;
    for (int i = 0; i < kColumn; i++) {
        Truck r;
        r.slot = i;
        r.z = lead + float(i) * 11.f;
        r.speed = scenic ? 1.6f : kCruise;
        trucks_.push_back(r);
    }
}

void Game::bootTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    planted_ = false;
    sag_ = false;
    stopped_ = 0;
    through_ = 0;
    t_ = 0;
    hold_ = 0;
    settle_ = 0;
    shake_ = 0;
    fanStep_ = -1;
    reason_ = "THE TRENCH IS QUIET";
    spawn(true);
}

void Game::begin() {
    spawn(false);
    mode_ = Mode::Play;
    won_ = false;
    over_ = false;
    planted_ = false;
    sag_ = false;
    stopped_ = 0;
    through_ = 0;
    t_ = 0;
    hold_ = 0;
    settle_ = 0;
    shake_ = 0;
    fanStep_ = -1;
    reason_ = "THE TRENCH IS QUIET";
    blip(420.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    bootTitle();
    if (bot_) begin();
}

void Game::blip(float freq) {
    sys_->apu.tone(2, freq, 0.06f);
    blip_ = 0.08f;
}

void Game::lose(const char* why) {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Fail;
    won_ = false;
    reason_ = why;
    hold_ = 1.5f;
    shake_ = 1.f;
    fanStep_ = 0;
    fanT_ = 0;
    fanGood_ = false;
    sys_->apu.noiseBurst(0.18f, 640.f, 0.18f);
    sys_->rumble(0.4f, 0.15f, 140);
}

void Game::win() {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Victory;
    won_ = true;
    stopped_ = kColumn;
    reason_ = "THE COLUMN STOPS ON THE ROAD";
    hold_ = 1.3f;
    fanStep_ = 0;
    fanT_ = 0;
    fanGood_ = true;
    sys_->rumble(0.15f, 0.4f, 120);
}

void Game::plant() {
    if (mode_ != Mode::Play || planted_) return;
    const float z = leadZ();
    planted_ = true;
    blip(180.f);
    sys_->apu.noiseBurst(0.12f, 300.f, 0.1f);
    if (z > 30.f) {
        sag_ = true;
        lose("THE CABLE SAGGED");
        return;
    }
    if (z < kCableZ - 1.5f) {
        through_ = 1;
        lose("TOO LATE");
        return;
    }
}

void Game::leave() {
    if (mode_ != Mode::Play) return;
    lose("YOU LEFT THE TRENCH");
}

float Game::leadZ() const {
    float z = 1e9f;
    for (const Truck& r : trucks_) z = std::min(z, r.z);
    return z;
}

void Game::update() {
    const float dt = 1.f / 60.f;
    t_ += dt;
    if (shake_ > 0) shake_ = std::max(0.f, shake_ - dt);
    int held = 0;
    for (Truck& r : trucks_) {
        float target = kCableZ + float(r.slot) * kSpace;
        if (planted_ && !sag_ && r.z <= target + 1.2f) r.held = true;
        if (r.held) {
            r.speed = std::max(0.f, r.speed - 14.f * dt);
            r.z -= r.speed * dt;
            if (r.z < target) r.z = target;
            if (r.speed < 0.08f) {
                r.speed = 0;
                held++;
            }
        } else {
            r.z -= r.speed * dt;
            if (r.z < kLipZ) {
                through_++;
                r.z = kLipZ;
                r.speed = 0;
                lose("A TRUCK GOT THROUGH");
                return;
            }
        }
    }
    stopped_ = held;
    if (planted_ && !sag_ && held == kColumn && through_ == 0) {
        settle_ += dt;
        if (settle_ > 0.45f) win();
    }
}

void Game::serviceAudio() {
    if (blip_ > 0) {
        blip_ -= 1.f / 60.f;
        if (blip_ <= 0) sys_->apu.tone(2, 0, 0);
    }
    if (fanStep_ < 0) return;
    fanT_ += 1.f / 60.f;
    const float step = 0.16f;
    if (fanT_ < step) return;
    fanT_ = 0;
    static const float good[4] = {392.f, 523.f, 659.f, 784.f};
    static const float bad[3] = {220.f, 174.f, 130.f};
    if (fanGood_) {
        if (fanStep_ < 4) sys_->apu.tone(0, good[fanStep_], 0.07f);
        if (++fanStep_ >= 6) {
            fanStep_ = -1;
            sys_->apu.tone(0, 0, 0);
        }
    } else {
        if (fanStep_ < 3) sys_->apu.tone(0, bad[fanStep_], 0.07f);
        if (++fanStep_ >= 4) {
            fanStep_ = -1;
            sys_->apu.tone(0, 0, 0);
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (bot_ || pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) begin();
    } else if (mode_ == Mode::Play) {
        if (bot_) {
            const float z = leadZ();
            if (!planted_ && z <= 22.f && z >= kCableZ) plant();
        } else {
            if (pad.pressed(gs::BTN_A)) plant();
            if (pad.pressed(gs::BTN_B)) leave();
        }
        if (mode_ == Mode::Play) update();
    } else {
        hold_ -= 1.f / 60.f;
        if (hold_ <= 0 && !over_) over_ = true;
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) bootTitle();
    }
    serviceAudio();
    draw();
    sys.vdp.roadTime++;
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

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
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
    sys_->vdp.sprite(s);
}

static void project(float z, float lat, float& x, float& y, float& scale) {
    float zz = std::max(z, kNear);
    float t = std::min(1.f, kNear / zz);
    y = kHorizon + t * kDepth;
    float hw = 16.f + t * t * 148.f;
    x = 160.f + lat * hw;
    scale = t;
}

void Game::layRoad() {
    gs::VDP& vdp = sys_->vdp;
    const uint16_t skyTop = gs::rgb4(3, 4, 6);
    const uint16_t skyHor = gs::rgb4(9, 8, 6);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        vdp.road[y].on = false;
        float u = y < int(kHorizon) ? float(y) / kHorizon : 1.f;
        int r = int(((skyTop >> 8) & 15) * (1 - u) + ((skyHor >> 8) & 15) * u);
        int g = int(((skyTop >> 4) & 15) * (1 - u) + ((skyHor >> 4) & 15) * u);
        int b = int((skyTop & 15) * (1 - u) + (skyHor & 15) * u);
        vdp.lineBackdrop[y] = gs::rgb4(r, g, b);
        vdp.lineFog[y] = 0;
        if (y < int(kHorizon)) continue;
        float t = std::min(1.f, (float(y) - kHorizon) / kDepth);
        float z = kNear / std::max(t, 0.02f);
        gs::RoadLine& ln = vdp.road[y];
        ln.on = true;
        ln.cx = 160.f;
        ln.hw = 16.f + t * t * 148.f;
        ln.v = z * 40.f;
        ln.pal = PAL_ROAD;
        ln.band = (int(z) & 1) ? 1 : 0;
        ln.style = 1;
        ln.left = gs::GROUND_LAND;
        ln.right = gs::GROUND_LAND;
        int fog = int(std::clamp((1.f - t) * 14.f - 2.f, 0.f, 14.f));
        vdp.lineFog[y] = uint8_t(fog);
    }
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    layRoad();
    float shx = std::sin(t_ * 40.f) * shake_ * 3.f;

    std::vector<int> order(trucks_.size());
    for (size_t i = 0; i < trucks_.size(); i++) order[i] = int(i);
    std::sort(order.begin(), order.end(), [&](int a, int b) { return trucks_[a].z < trucks_[b].z; });

    if (planted_ && !sag_) {
        float x0, y0, s0, x1, y1, s1;
        project(kCableZ, -0.95f, x0, y0, s0);
        project(kCableZ, 0.95f, x1, y1, s1);
        float h = std::max(4.f, 10.f * s0);
        for (int i = 0; i <= 6; i++) {
            float u = float(i) / 6.f;
            spr(art_.cable, shx + x0 + (x1 - x0) * u, y0 - 2.f, h, PAL_CABLE, false, int((1.f - s0) * 10), false);
        }
        spr(art_.flare, shx + (x0 + x1) * 0.5f, y0 - 6.f, 18.f * s0 + 6.f, PAL_FLARE, false, 0, false);
        spr(art_.stake, shx + x0, y0, 28.f * s0 + 8.f, PAL_STAKE, false, int((1.f - s0) * 8), true);
        spr(art_.stake, shx + x1, y1, 28.f * s1 + 8.f, PAL_STAKE, true, int((1.f - s1) * 8), true);
    }

    for (int idx : order) {
        const Truck& r = trucks_[idx];
        float x, y, sc;
        project(r.z, (r.slot & 1) ? 0.12f : -0.08f, x, y, sc);
        int fog = int(std::clamp((1.f - sc) * 14.f, 0.f, 14.f));
        float h = std::clamp(78.f * sc, 6.f, 150.f);
        spr(art_.truck, shx + x, y, h, PAL_TRUCK, false, fog, true);
    }

    spr(art_.you, 78 + shx, 206, 46, PAL_YOU, false, 0, true);
    spr(art_.bag, 46, 214, 22, PAL_BAG, false, 0, true);
    spr(art_.bag, 118, 216, 26, PAL_BAG, true, 0, true);
    spr(art_.bag, 200, 214, 24, PAL_BAG, false, 0, true);
    spr(art_.bag, 268, 216, 22, PAL_BAG, true, 0, true);
    spr(art_.wire, 150, 188, 10, PAL_STAKE, false, 0, false);
    spr(art_.wire, 210, 192, 8, PAL_STAKE, false, 0, false);

    if (mode_ == Mode::Title) {
        hudC(8, "S3 TRENCH COLUMN", PAL_HUD);
        hudC(11, "STOP THE COLUMN", PAL_HUD);
        hudC(12, "ON THE ROAD", PAL_HUD);
        hudC(16, "A PLANTS THE CABLE", PAL_HUD);
        hudC(17, "B LEAVES THE TRENCH", PAL_ALERT);
        hudC(24, "START", PAL_GOOD);
    } else if (mode_ == Mode::Play) {
        char line[40];
        std::snprintf(line, sizeof(line), "HELD %d/%d", stopped_, kColumn);
        hud(1, 1, line, PAL_HUD);
        if (!planted_) hudC(25, "WAIT FOR THE LEAD", leadZ() < 26.f ? PAL_GOOD : PAL_HUD);
        else hudC(25, "CABLE ACROSS", PAL_GOOD);
        hud(1, 26, "A CABLE  B OUT", PAL_HUD);
    } else if (mode_ == Mode::Victory) {
        hudC(12, "THE COLUMN STOPS", PAL_GOOD);
        hudC(13, "ON THE ROAD", PAL_GOOD);
    } else {
        hudC(12, reason_, PAL_ALERT);
    }
}

}  // namespace trench

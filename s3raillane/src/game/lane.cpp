#include "game/lane.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace lane {
namespace {

constexpr int LEGS = 3;
constexpr float LENS[LEGS] = {240.f, 300.f, 360.f};
constexpr float LANE = 1.f;
constexpr float END_HALF = 0.46f;
constexpr float GRACE = 0.30f;
constexpr int HORIZON = 92;
constexpr float SPAN = 78.f;
constexpr const char* NAMES[LEGS] = {"YARD", "CUTTING", "TERMINUS"};

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

uint16_t mix4(uint16_t a, uint16_t b, float t) {
    t = clampf(t, 0.f, 1.f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    int r = int(ar + (br - ar) * t);
    int g = int(ag + (bg - ag) * t);
    int bl = int(ab + (bb - ab) * t);
    return gs::rgb4(r, g, bl);
}

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.setFogColor(gs::rgb4(4, 5, 7));
    mode_ = Mode::Title;
    t_ = 0;
}

int Game::marker() const {
    if (mode_ == Mode::Title || mode_ == Mode::Pause) return 0;
    if (mode_ == Mode::Roll) return 1;
    if (mode_ == Mode::Banner) return 2;
    return 3;
}

float Game::curveAt(int leg, float dist) const {
    float len = LENS[std::clamp(leg, 0, LEGS - 1)];
    float ease = 1.f;
    if (dist > len - 42.f) ease = std::max(0.f, (len - dist) / 42.f);
    float s = 0.f;
    if (leg <= 0) s = 0.016f * std::sin(dist * 0.045f);
    else if (leg == 1) s = 0.024f * std::sin(dist * 0.062f) + 0.010f * std::sin(dist * 0.15f);
    else s = 0.030f * std::sin(dist * 0.055f) + 0.014f * std::sin(dist * 0.11f + 1.2f);
    return s * ease;
}

void Game::startRun() {
    leg_ = 0;
    held_ = 0;
    won_ = false;
    over_ = false;
    note_ = "";
    beginLeg();
}

void Game::beginLeg() {
    mode_ = Mode::Roll;
    x_ = 0;
    speed_ = 14.f;
    dist_ = 0;
    legTime_ = 0;
    out_ = 0;
    banner_ = 0;
}

void Game::fail(const char* why) {
    note_ = why;
    mode_ = Mode::Lost;
    won_ = false;
    over_ = true;
    speed_ = 0;
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(1, 110.f, 0.12f);
    blip_ = 0.4f;
}

void Game::holdLeg() {
    held_++;
    sys_->apu.tone(1, 660.f, 0.1f);
    blip_ = 0.18f;
    if (held_ >= LEGS) {
        mode_ = Mode::Won;
        won_ = true;
        over_ = true;
        note_ = "LANE HELD";
        return;
    }
    leg_ = held_;
    mode_ = Mode::Banner;
    banner_ = 1.35f;
}

void Game::botSteer(float& steer, float& thr) const {
    float push = curveAt(leg_, dist_) * speed_ * 0.9f;
    float cancel = -push / 1.75f;
    steer = clampf(cancel - x_ * 4.2f, -1.f, 1.f);
    thr = 0.5f;
}

void Game::updateTitle(float dt) {
    t_ += dt;
    dist_ += 8.f * dt;
    x_ = 0.12f * std::sin(t_ * 0.7f);
    speed_ = 8.f;
    bool go = bot_ && t_ > 0.35f;
    if (!bot_) go = sys_->pad.pressed(gs::BTN_START) || sys_->pad.pressed(gs::BTN_A);
    if (go) startRun();
}

void Game::updateRoll(float dt) {
    float steer = 0, thr = 0, brk = 0;
    if (bot_) {
        botSteer(steer, thr);
    } else {
        if (sys_->pad.pressed(gs::BTN_START)) {
            heldMode_ = Mode::Roll;
            mode_ = Mode::Pause;
            return;
        }
        steer = sys_->pad.axisX;
        if (sys_->pad.down(gs::BTN_LEFT)) steer -= 1.f;
        if (sys_->pad.down(gs::BTN_RIGHT)) steer += 1.f;
        steer = clampf(steer, -1.f, 1.f);
        if (sys_->pad.down(gs::BTN_UP) || sys_->pad.down(gs::BTN_A) || sys_->pad.accel > 0.2f) thr = 1.f;
        if (sys_->pad.down(gs::BTN_DOWN) || sys_->pad.down(gs::BTN_B) || sys_->pad.brake > 0.2f) brk = 1.f;
    }

    float goal = brk > 0.2f ? 3.f : 16.f + thr * 12.f;
    speed_ += (goal - speed_) * dt * (brk > 0.2f ? 2.4f : 1.4f);
    speed_ = clampf(speed_, 0.f, 30.f);

    float push = curveAt(leg_, dist_) * speed_ * 0.9f;
    x_ += (push + steer * 1.75f) * dt;
    dist_ += speed_ * dt;
    legTime_ += dt;
    t_ += dt;

    if (std::fabs(x_) > LANE) out_ += dt;
    else out_ = std::max(0.f, out_ - dt * 1.5f);

    float limit = LENS[leg_] / 8.f + 4.f;
    if (out_ > GRACE) fail("LEFT THE LANE");
    else if (legTime_ > limit) fail("MISSED THE END");
    else if (dist_ >= LENS[leg_]) {
        if (std::fabs(x_) > END_HALF) fail("MISSED THE END");
        else holdLeg();
    }
}

void Game::updateBanner(float dt) {
    t_ += dt;
    banner_ -= dt;
    speed_ = std::max(0.f, speed_ - 10.f * dt);
    if (banner_ <= 0.f) beginLeg();
}

void Game::updateEnd(float dt) {
    t_ += dt;
    banner_ += dt;
    if (mode_ == Mode::Lost && !bot_ && (sys_->pad.pressed(gs::BTN_START) || sys_->pad.pressed(gs::BTN_A))) startRun();
    if (mode_ == Mode::Won && !bot_ && sys_->pad.pressed(gs::BTN_START)) {
        mode_ = Mode::Title;
        t_ = 0;
        over_ = false;
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const float dt = 1.f / 60.f;
    if (mode_ == Mode::Title) updateTitle(dt);
    else if (mode_ == Mode::Roll) updateRoll(dt);
    else if (mode_ == Mode::Banner) updateBanner(dt);
    else if (mode_ == Mode::Pause) {
        if (sys.pad.pressed(gs::BTN_START)) mode_ = heldMode_;
    } else updateEnd(dt);
    audio(dt);
    draw();
}

void Game::audio(float dt) {
    if (blip_ > 0) {
        blip_ -= dt;
        if (blip_ <= 0) sys_->apu.tone(1, 0, 0);
    }
    if (mode_ == Mode::Roll) {
        float hz = 48.f + speed_ * 3.2f;
        sys_->apu.tone(0, hz, 0.04f + speed_ * 0.002f);
        engineOn_ = true;
    } else if (engineOn_ && mode_ != Mode::Banner) {
        sys_->apu.tone(0, 0, 0);
        engineOn_ = false;
    } else if (mode_ == Mode::Banner) {
        sys_->apu.tone(0, 40.f, 0.03f);
    }
}

void Game::hudText(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c < 32 || c > 95) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudCenter(int row, const char* s, int pal) { hudText(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::blit(const gs::Image& img, float x, float y, int pal, int w, int h) {
    gs::Sprite s;
    s.img = img;
    s.w = int16_t(w > 0 ? w : img.w);
    s.h = int16_t(h > 0 ? h : img.h);
    if (s.w < 1 || s.h < 1) return;
    s.x = int16_t(std::lround(x));
    s.y = int16_t(std::lround(y));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::skyAndRoad() {
    gs::VDP& v = sys_->vdp;
    const uint16_t top = gs::rgb4(2, 2, 6);
    const uint16_t hor = gs::rgb4(9, 5, 4);
    const bool warn = mode_ == Mode::Roll && std::fabs(x_) > 0.78f;
    int showLeg = std::clamp(leg_, 0, LEGS - 1);

    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < HORIZON) {
            v.lineBackdrop[y] = mix4(top, hor, y / float(HORIZON));
            v.lineFog[y] = 0;
            v.road[y].on = false;
            continue;
        }
        float n = (y - HORIZON) / float(gs::SCREEN_H - 1 - HORIZON);
        float z = dist_ + (1.f - n) * (1.f - n) * SPAN;
        float heading = 0, lat = 0, a = dist_;
        while (a + 0.01f < z) {
            float step = std::min(2.f, z - a);
            heading += curveAt(showLeg, a) * step;
            lat += heading * step;
            a += step;
        }
        float px = 8.f + n * n * 128.f;
        gs::RoadLine& r = v.road[y];
        r.on = true;
        r.hw = LANE * px;
        r.cx = 160.f + (lat - x_) * px;
        r.v = z * 18.f;
        r.pal = PAL_ROAD;
        r.band = (int(std::floor(z / 8.f)) & 1) ? 1 : 0;
        r.style = 1;
        r.left = r.right = gs::GROUND_LAND;
        v.lineFog[y] = uint8_t(std::clamp(int((1.f - n) * 12.f), 0, 12));
        v.lineBackdrop[y] = warn ? gs::rgb4(6, 2, 2) : hor;
    }
    v.roadTime = int(t_ * 60.f);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    skyAndRoad();

    int showLeg = std::clamp(leg_, 0, LEGS - 1);
    float len = LENS[showLeg];

    // Earlier sprites sit on top. The cab covers the gate, the gate covers the poles.
    int cabW = 78, cabH = 60;
    blit(art_.cab, 160 - cabW / 2, 158, PAL_CAB, cabW, cabH);

    // The end pocket. It is tighter than the lane, and missing it fails the leg.
    if (mode_ == Mode::Roll || mode_ == Mode::Banner || mode_ == Mode::Title) {
        float ahead = (mode_ == Mode::Title) ? 40.f + 10.f * std::sin(t_ * 0.4f) : len - dist_;
        if (ahead > 0.5f && ahead < SPAN) {
            float n = 1.f - std::sqrt(ahead / SPAN);
            n = clampf(n, 0.f, 1.f);
            int y = HORIZON + int((gs::SCREEN_H - 1 - HORIZON) * n);
            const gs::RoadLine& r = v.road[std::clamp(y, 0, gs::SCREEN_H - 1)];
            float px = r.hw / LANE;
            float gw = std::max(12.f, END_HALF * 2.f * px);
            float gh = gw * (art_.gate.h / float(art_.gate.w));
            int pal = (mode_ == Mode::Roll && std::fabs(x_) > END_HALF) ? PAL_RED : PAL_GATE;
            blit(art_.gate, r.cx - gw * 0.5f, y - gh * 0.72f, pal, int(gw), int(gh));
        }
    }

    // Telegraph poles beside the lane, repeating down the leg.
    for (int k = 0; k < 8; k++) {
        float base = std::floor(dist_ / 22.f) * 22.f + k * 22.f;
        if (base < dist_ - 1.f || base > dist_ + SPAN) continue;
        float ahead = base - dist_;
        float n = 1.f - std::sqrt(std::max(0.f, ahead / SPAN));
        n = clampf(n, 0.f, 1.f);
        int y = HORIZON + int((gs::SCREEN_H - 1 - HORIZON) * n);
        if (y < HORIZON || y >= gs::SCREEN_H) continue;
        const gs::RoadLine& r = v.road[y];
        float px = r.hw / LANE;
        float h = 18.f + n * 46.f;
        float w = h * (art_.pole.w / float(art_.pole.h));
        for (int side = -1; side <= 1; side += 2) {
            float sx = r.cx + side * 1.55f * px - w * 0.5f;
            blit(art_.pole, sx, y - h, PAL_POLE, int(w), int(h));
        }
    }

    if (mode_ == Mode::Title) {
        blit(art_.logo, 160 - art_.logo.w / 2, 28, PAL_TEXT);
        hudCenter(12, "STAY IN THE LANE", PAL_AMBER);
        hudCenter(13, "THE WHOLE LEG", PAL_TEXT);
        hudCenter(15, "MISSING THE END FAILS", PAL_RED);
        hudCenter(18, "LEFT AND RIGHT STEER", PAL_DIM);
        hudCenter(22, "START", PAL_GREEN);
    } else if (mode_ == Mode::Pause) {
        hudCenter(12, "PAUSE", PAL_AMBER);
        hudCenter(14, "START TO ROLL", PAL_DIM);
    } else if (mode_ == Mode::Banner) {
        char buf[40];
        std::snprintf(buf, sizeof buf, "%s HELD", NAMES[std::clamp(held_ - 1, 0, LEGS - 1)]);
        hudCenter(12, buf, PAL_GREEN);
        std::snprintf(buf, sizeof buf, "NEXT  %s", NAMES[std::clamp(leg_, 0, LEGS - 1)]);
        hudCenter(14, buf, PAL_AMBER);
    } else if (mode_ == Mode::Won) {
        hudCenter(11, "LANE HELD", PAL_GREEN);
        hudCenter(13, "EVERY LEG", PAL_AMBER);
        hudCenter(16, "START", PAL_DIM);
    } else if (mode_ == Mode::Lost) {
        hudCenter(11, note_, PAL_RED);
        char buf[40];
        std::snprintf(buf, sizeof buf, "LEG %d  %s", showLeg + 1, NAMES[showLeg]);
        hudCenter(13, buf, PAL_AMBER);
        hudCenter(16, "START TO RETRY", PAL_DIM);
    } else {
        char buf[48];
        std::snprintf(buf, sizeof buf, "LEG %d  %s", showLeg + 1, NAMES[showLeg]);
        hudText(1, 1, buf, PAL_AMBER);
        int pct = int(clampf(dist_ / len, 0.f, 1.f) * 100.f);
        std::snprintf(buf, sizeof buf, "%d%%", pct);
        hudText(34, 1, buf, PAL_TEXT);
        const char* warn = std::fabs(x_) > END_HALF ? "OFF THE END" : "IN THE LANE";
        int pal = std::fabs(x_) > LANE ? PAL_RED : (std::fabs(x_) > END_HALF ? PAL_AMBER : PAL_GREEN);
        hudText(1, 26, warn, pal);
        hudText(26, 26, "STEER", PAL_DIM);
    }
}

}  // namespace lane

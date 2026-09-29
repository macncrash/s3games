#include "dawn.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "../version.h"

namespace cdawn {
namespace {

constexpr int HORIZON = 78;
constexpr float kDawn = 36.f;
constexpr float kReach = 0.075f;
constexpr float kSpeed = 1.15f;
constexpr float kDrain = 0.048f;
constexpr float kFlareZ[Game::kFlares] = {0.22f, 0.42f, 0.62f, 0.82f};
constexpr int kGustOrder[Game::kFlares] = {0, 2, 1, 3};

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

int Game::lit() const {
    int n = 0;
    for (int i = 0; i < kFlares; i++)
        if (fuel_[i] > 0.f) n++;
    return n;
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Won) return 2;
    if (mode_ == Mode::Lost) return 3;
    return 1;
}

void Game::blip(int ch, float freq) {
    if (!sys_) return;
    sys_->apu.tone(ch, freq, 0.06f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    mode_ = Mode::Title;
    age_ = 0;
    bob_ = 0;
    for (int i = 0; i < kFlares; i++) fuel_[i] = 0.86f;
}

void Game::bootWatch() {
    mode_ = Mode::Watch;
    over_ = false;
    won_ = false;
    fed_ = 0;
    braced_ = 0;
    dead_ = -1;
    clock_ = 0;
    pz_ = 0.22f;
    pourCd_ = 0.15f;
    gustIx_ = -1;
    gustSeq_ = 0;
    gustT_ = 0;
    nextGust_ = 1.8f;
    endAge_ = 0;
    face_ = 1;
    for (int i = 0; i < kFlares; i++) fuel_[i] = 0.86f;
    blip(0, 330.f);
}

int Game::choose() const {
    int best = 0;
    float bestS = 1e9f;
    for (int i = 0; i < kFlares; i++) {
        float s = fuel_[i];
        if (gustIx_ == i) s -= 0.28f + (0.7f - gustT_) * 0.15f;
        s += std::fabs(kFlareZ[i] - pz_) * 0.04f;
        if (s < bestS) {
            bestS = s;
            best = i;
        }
    }
    return best;
}

void Game::think(float& dir, bool& pour, bool& brace) {
    int i = choose();
    float dz = kFlareZ[i] - pz_;
    if (std::fabs(dz) > kReach * 0.85f) dir = dz > 0.f ? 1.f : -1.f;
    else dir = 0.f;
    bool here = std::fabs(dz) <= kReach;
    pour = here && fuel_[i] < 0.93f && pourCd_ <= 0.f;
    brace = here && gustIx_ == i && gustT_ < 0.42f;
}

void Game::readMove(float& dir, bool& pour, bool& brace) const {
    dir = 0.f;
    pour = false;
    brace = false;
    if (bot_) return;
    const gs::Pad& p = sys_->pad;
    if (p.down(gs::BTN_UP)) dir -= 1.f;
    if (p.down(gs::BTN_DOWN)) dir += 1.f;
    if (std::fabs(p.axisY) > 0.35f) dir = -p.axisY;
    pour = p.down(gs::BTN_Z) || p.down(gs::BTN_A) || p.down(gs::BTN_C);
    brace = p.down(gs::BTN_X) || p.down(gs::BTN_B);
}

void Game::tickWatch(float dt) {
    float dir = 0.f;
    bool pour = false, brace = false;
    if (bot_) think(dir, pour, brace);
    else readMove(dir, pour, brace);

    if (dir > 0.2f) face_ = 1;
    else if (dir < -0.2f) face_ = -1;
    pz_ = clampf(pz_ + dir * kSpeed * dt, 0.12f, 0.92f);
    if (pourCd_ > 0.f) pourCd_ -= dt;
    bob_ += dt;

    for (int i = 0; i < kFlares; i++) {
        float d = kDrain;
        if (gustIx_ == i && gustT_ < 0.35f) d += 0.04f;
        fuel_[i] -= d * dt;
    }

    if (gustIx_ < 0) {
        nextGust_ -= dt;
        if (nextGust_ <= 0.f) {
            gustIx_ = kGustOrder[gustSeq_ % kFlares];
            gustSeq_++;
            gustT_ = 0.85f;
            blip(1, 180.f + float(gustIx_) * 30.f);
        }
    } else {
        float prev = gustT_;
        gustT_ -= dt;
        if (prev > 0.f && gustT_ <= 0.f) {
            bool held = brace && std::fabs(pz_ - kFlareZ[gustIx_]) <= kReach;
            float hit = held ? 0.035f : 0.20f;
            fuel_[gustIx_] -= hit;
            if (held) {
                braced_++;
                blip(0, 220.f);
            } else {
                blip(2, 90.f);
            }
            gustIx_ = -1;
            nextGust_ = 1.35f;
        }
    }

    if (pour) {
        int near = -1;
        float best = kReach;
        for (int i = 0; i < kFlares; i++) {
            float dz = std::fabs(pz_ - kFlareZ[i]);
            if (dz <= best) {
                best = dz;
                near = i;
            }
        }
        if (near >= 0 && fuel_[near] < 1.f) {
            fuel_[near] = std::min(1.f, fuel_[near] + 0.36f);
            pourCd_ = 0.26f;
            fed_++;
            blip(0, 440.f + float(near) * 40.f);
        }
    }

    for (int i = 0; i < kFlares; i++) {
        if (fuel_[i] <= 0.f) {
            fuel_[i] = 0.f;
            dead_ = i;
            mode_ = Mode::Lost;
            won_ = false;
            blip(0, 70.f);
            return;
        }
    }

    clock_ += dt;
    if (clock_ >= kDawn) {
        mode_ = Mode::Won;
        won_ = true;
        blip(0, 523.f);
    }
}

void Game::placeZ(float z, float& x, float& y, float& depth) const {
    float t = clampf((1.05f - z) / 0.95f, 0.f, 1.f);
    depth = t * t;
    float sway = std::sin(bob_ * 0.7f) * 8.f * (1.f - depth);
    x = 160.f + sway;
    y = float(HORIZON) + 10.f + depth * 112.f;
}

void Game::skyAndRoad() {
    gs::VDP& v = sys_->vdp;
    v.roadTime = age_;
    float dawn = clampf(clock_ / kDawn, 0.f, 1.f);
    if (mode_ == Mode::Title) dawn = 0.05f;
    float sway = std::sin(bob_ * 0.7f) * 10.f;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < HORIZON) {
            float sky = float(y) / float(HORIZON);
            int r = int(1 + sky * 2 + dawn * 10.f);
            int g = int(2 + sky * 2 + dawn * 6.f);
            int b = int(6 + (1.f - sky) * 3 - dawn * 2.f);
            v.lineBackdrop[y] = gs::rgb4(std::min(r, 15), std::min(g, 15), std::clamp(b, 2, 15));
            v.lineFog[y] = uint8_t(std::clamp(8 - int(sky * 6) - int(dawn * 3), 0, 12));
            v.road[y].on = false;
            continue;
        }
        float depth = float(y - HORIZON) / float(gs::SCREEN_H - HORIZON);
        gs::RoadLine& rl = v.road[y];
        rl = {};
        rl.on = true;
        rl.cx = 160.f + sway * (1.f - depth);
        rl.hw = 12.f + depth * depth * 148.f;
        rl.v = 1800.f / (depth + 0.16f);
        rl.pal = 12;
        rl.band = (int(std::floor(rl.v / 90.f)) & 1) ? 1 : 0;
        rl.style = gs::ROAD_ROCKY;
        rl.left = gs::GROUND_WATER;
        rl.right = gs::GROUND_WATER;
        int tide = ((age_ / 12) + int(depth * 4)) & 1;
        v.lineBackdrop[y] = tide ? gs::rgb4(1, 2, 5) : gs::rgb4(1, 3, 6);
        v.lineFog[y] = uint8_t(std::clamp(int((1.f - depth) * 10.f - dawn * 2.f), 0, 12));
    }
    v.setFogColor(gs::rgb4(2 + int(dawn * 6), 3 + int(dawn * 4), 6));
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        int x = col + i;
        if (x < 0 || x > 39 || c < 33 || c > 126) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::text(const char* s, float x, float y, float scale, int pal) {
    int n = int(std::strlen(s));
    float adv = 18.f * scale;
    float left = x - n * adv * 0.5f;
    for (int i = 0; i < n; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < 33 || c > 126) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, left + float(i) * adv + adv * 0.5f, y, g.h * scale, pal, false, 0);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.hudEnabled = true;
    skyAndRoad();

    if (mode_ == Mode::Title) text("S3 CAUSEWAY DAWN", 160, 34, 0.62f, PAL_HUD);
    else if (mode_ == Mode::Won) text("DAWN", 160, 32, 0.9f, PAL_OK);
    else if (mode_ == Mode::Lost) text("GONE DARK", 160, 32, 0.72f, PAL_ALERT);
    else if (mode_ == Mode::Pause) text("HOLD", 160, 32, 0.8f, PAL_HUD);

    float gx = 40.f + std::fmod(bob_ * 18.f, 260.f);
    spr(art_.gull, gx, 58.f + std::sin(bob_ * 2.f) * 3.f, 8.f, PAL_SEA, false, 4);

    for (int i = kFlares - 1; i >= 0; i--) {
        float x, y, depth;
        placeZ(kFlareZ[i], x, y, depth);
        float h = 14.f + depth * 22.f;
        int fog = int((1.f - depth) * 9.f);
        int side = (i & 1) ? 1 : -1;
        float spread = 18.f + depth * 70.f;
        spr(art_.post, x + side * spread, y, h * 0.85f, PAL_POST, false, fog);
        bool warn = gustIx_ == i && (age_ / 6) & 1;
        spr(art_.pot, x, y, h * 0.55f, warn ? PAL_ALERT : PAL_POT, false, fog);
        if (fuel_[i] > 0.02f) {
            float fh = h * (0.35f + fuel_[i] * 0.85f);
            float flick = ((age_ / 4 + i) & 1) ? 1.5f : 0.f;
            spr(art_.flame, x, y - h * 0.42f - flick, fh, PAL_FLAME, false, fog);
        } else {
            spr(art_.wick, x, y - h * 0.4f, h * 0.22f, PAL_ALERT, false, fog);
        }
    }

    float px, py, pd;
    placeZ(pz_, px, py, pd);
    int fr = (age_ / 8) & 1;
    spr(art_.keeper[fr], px - 16.f, py - 2.f, 36.f + pd * 10.f, PAL_YOU, face_ < 0, 0);

    char line[48];
    if (mode_ == Mode::Title) {
        hudC(21, "ONE CAUSEWAY.", PAL_HUD);
        hudC(22, "KEEP THE FLARES LIT UNTIL DAWN.", PAL_OK);
        hudC(24, "UP DOWN WALK   Z POUR   X BRACE", PAL_HUD);
        hudC(25, "ENTER STARTS", PAL_HUD);
        int n = int(std::strlen(S3_VERSION_STRING));
        hud(39 - n, 0, S3_VERSION_STRING, PAL_HUD);
    } else {
        int secs = std::max(0, int(std::ceil(kDawn - clock_)));
        if (mode_ == Mode::Won) secs = 0;
        std::snprintf(line, sizeof(line), "DAWN %d", secs);
        hud(1, 26, line, secs < 8 ? PAL_OK : PAL_HUD);
        std::snprintf(line, sizeof(line), "LIT %d/%d", lit(), kFlares);
        hud(14, 26, line, lit() < kFlares ? PAL_ALERT : PAL_OK);
        std::snprintf(line, sizeof(line), "FED %d", fed_);
        hud(28, 26, line, PAL_HUD);
        if (gustIx_ >= 0) hudC(24, "SPRAY — BRACE THE POT", PAL_ALERT);
        if (mode_ == Mode::Lost) hudC(23, "A FLARE WENT OUT.", PAL_ALERT);
        if (mode_ == Mode::Won) hudC(23, "THE FLARES HELD.", PAL_OK);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    age_++;
    const float dt = 1.f / 60.f;
    const gs::Pad& p = sys.pad;
    bool go = p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A) || p.pressed(gs::BTN_Z);

    if (mode_ == Mode::Title) {
        bob_ += dt;
        if (bot_ || go) bootWatch();
    } else if (mode_ == Mode::Pause) {
        if (bot_ || go) mode_ = Mode::Watch;
    } else if (mode_ == Mode::Watch) {
        if (!bot_ && p.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else tickWatch(dt);
    } else if (mode_ == Mode::Won || mode_ == Mode::Lost) {
        endAge_++;
        if (endAge_ == 8) blip(0, mode_ == Mode::Won ? 659.f : 55.f);
        if (endAge_ > 20) sys.apu.tone(0, 0, 0);
        if (endAge_ > 30) {
            over_ = true;
            if (!bot_ && go) {
                mode_ = Mode::Title;
                over_ = false;
                won_ = false;
            }
        }
    }
    draw();
}

}  // namespace cdawn

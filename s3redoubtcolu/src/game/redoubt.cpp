#include "game/redoubt.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace redoubt {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr int kColumn = 6;
constexpr float kHorizon = 70.f;
constexpr float kSpan = 146.f;
constexpr float kZNear = 6.2f;
constexpr float kPpm = 26.f;
constexpr float kRoadHalf = 4.4f;
constexpr float kLine = 12.f;
constexpr float kSpeed = 5.6f;
constexpr float kSpace = 8.4f;
constexpr float kLeadZ = 50.f;
constexpr float kCool = 0.36f;
constexpr float kHitZ = 2.3f;
constexpr float kHitX = 1.45f;
constexpr float kZRate = 48.f;
constexpr float kXRate = 7.5f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

int Game::column() const { return kColumn; }

int Game::marker() const {
    if (mode_ == Mode::Victory) return 2;
    if (mode_ == Mode::Fail) return 3;
    if (mode_ == Mode::Play || mode_ == Mode::Pause) return 1;
    return 0;
}

void Game::blip(float freq, float vol) {
    sys_->apu.tone(0, freq, vol);
    beep_ = 0.09f;
}

void Game::lay() {
    trucks_.clear();
    puffs_.clear();
    halted_ = 0;
    through_ = 0;
    for (int i = 0; i < kColumn; ++i) {
        Truck t;
        t.z = kLeadZ + float(i) * kSpace;
        t.lat = (i % 2 == 0) ? -1.15f : 1.05f;
        t.lead = i == 0;
        trucks_.push_back(t);
    }
    aimZ_ = 28.f;
    aimX_ = 0.f;
    cool_ = 0.2f;
    shake_ = 0;
    time_ = 0;
}

void Game::bootTitle() {
    lay();
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    reason_ = "";
}

void Game::begin() {
    lay();
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
    reason_ = "";
}

void Game::win() {
    mode_ = Mode::Victory;
    won_ = true;
    over_ = true;
    reason_ = "THE COLUMN STOPS ON THE ROAD";
    halted_ = 0;
    for (const Truck& t : trucks_)
        if (t.stopped) ++halted_;
    blip(520.f, 0.18f);
    sys_->apu.tone(1, 780.f, 0.12f);
}

void Game::lose(const char* why) {
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    reason_ = why;
    halted_ = 0;
    for (const Truck& t : trucks_)
        if (t.stopped) ++halted_;
    blip(90.f, 0.2f);
}

void Game::tryFire() {
    if (cool_ > 0.f) return;
    cool_ = kCool;
    shake_ = 1.f;
    blip(140.f, 0.16f);
    bool hit = false;
    for (Truck& t : trucks_) {
        if (t.stopped) continue;
        if (std::fabs(t.z - aimZ_) < kHitZ && std::fabs(t.lat - aimX_) < kHitX) {
            t.stopped = true;
            hit = true;
            Puff p;
            p.z = t.z;
            p.lat = t.lat;
            puffs_.push_back(p);
        }
    }
    if (!hit) {
        Puff p;
        p.z = aimZ_;
        p.lat = aimX_;
        puffs_.push_back(p);
    }
}

void Game::botAim() {
    float tz = 1e9f, tx = 0;
    bool any = false;
    for (const Truck& t : trucks_) {
        if (t.stopped) continue;
        if (t.z < tz) {
            tz = t.z;
            tx = t.lat;
            any = true;
        }
    }
    if (!any) return;
    aimZ_ += clampf(tz - aimZ_, -kZRate * kDt, kZRate * kDt);
    aimX_ += clampf(tx - aimX_, -kXRate * kDt, kXRate * kDt);
    if (std::fabs(tz - aimZ_) < 0.7f && std::fabs(tx - aimX_) < 0.35f) tryFire();
}

void Game::update() {
    time_ += kDt;
    if (cool_ > 0.f) cool_ -= kDt;
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 3.f);
    for (Puff& p : puffs_) p.age += kDt;
    puffs_.erase(std::remove_if(puffs_.begin(), puffs_.end(), [](const Puff& p) { return p.age > 0.45f; }),
                 puffs_.end());

    if (bot_) botAim();
    else {
        const gs::Pad& pad = sys_->pad;
        float dz = 0, dx = 0;
        if (pad.down(gs::BTN_UP)) dz += 1.f;
        if (pad.down(gs::BTN_DOWN)) dz -= 1.f;
        if (pad.down(gs::BTN_RIGHT)) dx += 1.f;
        if (pad.down(gs::BTN_LEFT)) dx -= 1.f;
        aimZ_ += dz * kZRate * kDt;
        aimX_ += dx * kXRate * kDt;
        if (pad.pressed(gs::BTN_A) || pad.down(gs::BTN_B)) tryFire();
    }
    aimZ_ = clampf(aimZ_, kLine + 0.4f, 96.f);
    aimX_ = clampf(aimX_, -3.2f, 3.2f);

    bool all = true;
    int past = 0;
    for (Truck& t : trucks_) {
        if (!t.stopped) {
            t.z -= kSpeed * kDt;
            all = false;
            if (t.z <= kLine) ++past;
        }
    }
    if (past) {
        through_ = past;
        lose("THE COLUMN REACHED THE REDOUBT");
        return;
    }
    if (all) win();
}

bool Game::project(float wx, float wz, float& sx, float& sy, float& ppm) const {
    if (wz < kZNear + 0.15f) return false;
    float n = kZNear / wz;
    ppm = kPpm * n;
    sy = kHorizon + n * kSpan;
    sx = 160.f + wx * ppm;
    return sy > -20.f && sy < gs::SCREEN_H + 40.f;
}

int Game::fogFor(float z) const {
    float n = kZNear / std::max(z, 1.f);
    return int(clampf((1.f - n * 3.2f) * 8.f, 0.f, 11.f));
}

void Game::spr(const gs::Mipped& m, float cx, float feet, float ht, int pal, bool flip, int fog) {
    if (ht < 1.5f || m.h < 1) return;
    float w = ht * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::lround(clampf(w, 1.f, 400.f)));
    s.h = int16_t(std::lround(clampf(ht, 1.f, 300.f)));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet - s.h));
    s.img = m.pick(ht);
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::layRoad() {
    uint16_t zen = gs::rgb4(3, 4, 7);
    uint16_t mid = gs::rgb4(7, 8, 10);
    uint16_t hor = gs::rgb4(11, 10, 8);
    if (mode_ == Mode::Fail) hor = gs::rgb4(12, 5, 4);
    if (mode_ == Mode::Victory) hor = gs::rgb4(8, 12, 7);
    for (int y = 0; y < gs::SCREEN_H; ++y) {
        float t = y / float(gs::SCREEN_H - 1);
        sys_->vdp.lineBackdrop[y] = t < 0.28f ? gs::rgb4(int(3 + t / 0.28f * 4), int(4 + t / 0.28f * 4), int(7 + t / 0.28f * 3))
                                              : (y < int(kHorizon) ? mid : hor);
        (void)zen;
        sys_->vdp.lineFog[y] = 0;
        gs::RoadLine& r = sys_->vdp.road[y];
        r.on = false;
        if (y <= int(kHorizon)) continue;
        float n = (y - kHorizon) / kSpan;
        if (n < 0.02f) continue;
        float z = kZNear / n;
        r.on = true;
        r.cx = 160.f;
        r.hw = kRoadHalf * kPpm * n;
        r.v = z * 20.f;
        r.pal = PAL_ROAD;
        r.style = 1;
        r.band = (int(z * 1.6f) & 1) ? 1 : 0;
        r.left = gs::GROUND_LAND;
        r.right = gs::GROUND_LAND;
        sys_->vdp.lineFog[y] = uint8_t(clampf((1.f - n) * 8.f, 0.f, 9.f));
    }
    sys_->vdp.A.enabled = false;
    sys_->vdp.B.enabled = false;
    sys_->vdp.hudEnabled = true;
    sys_->vdp.HUD.clear();
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s) return;
    for (int i = 0; s[i]; ++i) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || row < 0 || row > 27 || c < 32 || c > 127) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::draw() {
    layRoad();
    sys_->vdp.clearSprites();
    float jx = std::sin(time_ * 40.f) * shake_ * 3.f;

    float sx, sy, ppm;
    if (project(aimX_, aimZ_, sx, sy, ppm) && (mode_ == Mode::Play || mode_ == Mode::Pause))
        spr(art_.reticle, sx + jx, sy + 6.f, std::max(8.f, ppm * 1.1f), PAL_BURST, false, 0);

    for (const Puff& p : puffs_) {
        if (!project(p.lat, p.z, sx, sy, ppm)) continue;
        float ht = ppm * (1.4f + p.age * 2.f);
        spr(art_.burst, sx, sy + ht * 0.2f, ht, PAL_BURST, false, fogFor(p.z));
    }

    std::vector<int> order(trucks_.size());
    for (int i = 0; i < int(trucks_.size()); ++i) order[i] = i;
    std::sort(order.begin(), order.end(), [&](int a, int b) { return trucks_[a].z < trucks_[b].z; });
    for (int idx : order) {
        const Truck& t = trucks_[idx];
        if (!project(t.lat, t.z, sx, sy, ppm)) continue;
        const gs::Mipped& m = t.lead ? art_.lead : art_.lorry;
        spr(m, sx, sy, ppm * (t.lead ? 2.15f : 2.35f), t.lead ? PAL_LEAD : PAL_LORRY, false, fogFor(t.z));
    }

    for (int i = 0; i < 7; ++i) {
        float z = 22.f + float(i) * 11.f;
        float lat = (i % 2 == 0) ? -(kRoadHalf + 2.4f) : (kRoadHalf + 2.6f);
        if (project(lat, z, sx, sy, ppm))
            spr(art_.tree, sx, sy, ppm * 4.8f, PAL_TREE, i & 1, fogFor(z));
    }

    if (project(-kRoadHalf - 2.1f, 15.5f, sx, sy, ppm))
        spr(art_.earth, sx + jx, sy + 4.f, ppm * 3.4f, PAL_EARTH, false, 0);
    if (project(-kRoadHalf - 0.35f, 14.2f, sx, sy, ppm))
        spr(art_.gun, sx + aimX_ * 4.f + jx, sy - 6.f, ppm * 2.6f, PAL_GUN, false, 0);

    char buf[64];
    if (mode_ == Mode::Title) {
        hudC(3, "S3 REDOUBT", PAL_AMBER);
        hudC(6, "ONE REDOUBT", PAL_TEXT);
        hudC(8, "STOP THE COLUMN ON THE ROAD", PAL_GOOD);
        hudC(15, "UP DOWN  RANGE", PAL_TEXT);
        hudC(16, "LEFT RIGHT  TRAVERSE", PAL_TEXT);
        hudC(17, "A FIRE", PAL_AMBER);
        hudC(22, "PRESS START", PAL_AMBER);
    } else if (mode_ == Mode::Pause) {
        hudC(10, "PAUSED", PAL_AMBER);
    } else if (mode_ == Mode::Victory) {
        hudC(3, "THE COLUMN STOPS", PAL_GOOD);
        hudC(5, "ON THE ROAD", PAL_GOOD);
        std::snprintf(buf, sizeof buf, "HALTED %d", kColumn);
        hudC(8, buf, PAL_TEXT);
    } else if (mode_ == Mode::Fail) {
        hudC(3, "REDOUBT LOST", PAL_BAD);
        hudC(5, reason_, PAL_BAD);
    } else {
        int live = 0;
        for (const Truck& t : trucks_)
            if (!t.stopped) ++live;
        std::snprintf(buf, sizeof buf, "COLUMN %d", live);
        hud(1, 1, buf, PAL_TEXT);
        std::snprintf(buf, sizeof buf, "RANGE %3.0f", aimZ_);
        hud(28, 1, buf, PAL_AMBER);
        hudC(26, "STOP THEM BEFORE THE WORKS", PAL_TEXT);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    bootTitle();
    if (bot_) begin();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (beep_ > 0.f) beep_ -= kDt;
    else sys.apu.tone(0, 0, 0);

    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (bot_ || pad.pressed(gs::BTN_START)) begin();
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else update();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
    } else if (!bot_ && pad.pressed(gs::BTN_START)) {
        begin();
    }
    draw();
}

}  // namespace redoubt

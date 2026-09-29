#include "game/causeway.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace cway {

namespace {
constexpr int kHorizon = 72;
constexpr float kDt = 1.f / 60.f;
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Victory) return 2;
    if (mode_ == Mode::Fail) return 3;
    return 1;
}

bool Game::onDeck(float z) const { return z >= kDeck0 && z <= kDeck1; }

bool Game::gateHolds() const {
    if (!planted_) return false;
    // The whole column has to fit on the paved span, not the water approaches.
    const float tail = gateZ_ - (kColumn - 1) * kGap;
    return onDeck(gateZ_) && onDeck(tail);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    t_ = 0;
}

void Game::begin() {
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
    planted_ = false;
    stopped_ = 0;
    through_ = 0;
    gateZ_ = 0.66f;
    hold_ = 0;
    flash_ = 0;
    reason_ = "THE CAUSEWAY IS OPEN";
    trucks_.clear();
    for (int i = 0; i < kColumn; i++) {
        Truck tr;
        tr.z = 0.02f - float(i) * 0.13f;
        tr.speed = 0.115f;
        trucks_.push_back(tr);
    }
}

void Game::plant() {
    planted_ = true;
    flash_ = 0.25f;
    sys_->apu.tone(0, gateHolds() ? 220.f : 90.f, 0.35f);
    sys_->apu.noiseBurst(0.2f, 1800.f, 8.f);
}

void Game::raise() {
    if (stopped_ > 0) return;
    planted_ = false;
    sys_->apu.tone(0, 140.f, 0.2f);
}

void Game::win() {
    mode_ = Mode::Victory;
    won_ = true;
    over_ = true;
    reason_ = "THE COLUMN STOPS ON THE ROAD";
    sys_->apu.tone(0, 330.f, 0.4f);
    sys_->apu.tone(1, 440.f, 0.35f);
    sys_->apu.tone(2, 554.f, 0.3f);
}

void Game::lose(const char* why) {
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    reason_ = why;
    sys_->apu.tone(0, 70.f, 0.45f);
    sys_->apu.noiseBurst(0.45f, 600.f, 3.f);
}

void Game::update() {
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(1, 0, 0);
    const gs::Pad& pad = sys_->pad;
    float axis = 0;
    if (pad.down(gs::BTN_UP)) axis -= 1.f;
    if (pad.down(gs::BTN_DOWN)) axis += 1.f;
    if (std::fabs(pad.axisY) > 0.3f) axis -= pad.axisY;
    axis = std::clamp(axis, -1.f, 1.f);

    if (bot_) {
        const float want = 0.58f;
        if (gateZ_ > want + 0.01f) axis = -1.f;
        else if (gateZ_ < want - 0.01f) axis = 1.f;
        else axis = 0;
    }

    if (stopped_ == 0) gateZ_ = std::clamp(gateZ_ + axis * 0.28f * kDt, 0.08f, 0.92f);

    bool tap = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B);
    if (bot_ && !planted_) {
        float lead = trucks_.empty() ? 0 : trucks_[0].z;
        if (std::fabs(gateZ_ - 0.58f) < 0.02f && lead > gateZ_ - 0.03f) tap = true;
    }
    if (tap) {
        if (!planted_) plant();
        else raise();
    }

    stopped_ = 0;
    for (int i = 0; i < int(trucks_.size()); i++) {
        Truck& tr = trucks_[i];
        if (tr.stopped) {
            ++stopped_;
            continue;
        }
        tr.z += tr.speed * kDt;
        bool halt = false;
        float at = tr.z;
        if (i == 0) {
            if (gateHolds() && tr.z >= gateZ_) {
                halt = true;
                at = gateZ_;
            }
        } else if (trucks_[i - 1].stopped && tr.z >= trucks_[i - 1].z - kGap) {
            halt = true;
            at = trucks_[i - 1].z - kGap;
        }
        if (halt) {
            tr.z = at;
            tr.speed = 0;
            tr.stopped = true;
            ++stopped_;
            sys_->apu.noiseBurst(0.15f, 900.f, 10.f);
            sys_->apu.tone(1, 160.f - float(i) * 12.f, 0.25f);
        } else if (tr.z > kDeck1 + 0.08f) {
            ++through_;
            lose("THE COLUMN LEFT THE CAUSEWAY");
            return;
        }
    }

    if (stopped_ == kColumn) {
        bool deck = true;
        for (const Truck& tr : trucks_)
            if (!onDeck(tr.z)) deck = false;
        if (!deck) {
            lose("THE COLUMN IS NOT ON THE ROAD");
            return;
        }
        hold_ += kDt;
        if (hold_ > 0.45f) win();
    } else {
        hold_ = 0;
    }
    if (flash_ > 0) flash_ -= kDt;
}

void Game::project(float z, float lat, float& x, float& y, float& h) const {
    float d = std::clamp(z, 0.f, 1.f);
    float depth = d * d;
    y = float(kHorizon) + depth * float(gs::SCREEN_H - kHorizon - 6);
    h = 8.f + depth * 78.f;
    x = 160.f + lat * (18.f + depth * 150.f);
}

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

void Game::text(const char* s, float x, float y, float scale, int pal) {
    int n = int(std::strlen(s));
    float adv = 16.f * scale;
    float left = x - n * adv * 0.5f;
    for (int i = 0; i < n; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < 33 || c > 126) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, left + float(i) * adv + adv * 0.5f, y, std::max(8.f, float(g.h) * scale), pal, false, 0, false);
    }
}

void Game::hudC(int row, const char* s, int pal) { text(s, 160.f, 10.f + float(row) * 16.f, 0.55f, pal); }

void Game::layRoad() {
    gs::VDP& v = sys_->vdp;
    v.roadTime = int(t_ * 60.f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < kHorizon) {
            float sky = float(y) / float(kHorizon);
            v.lineBackdrop[y] = gs::rgb4(3 + int(sky * 4), 4 + int(sky * 3), 8 + int((1.f - sky) * 4));
            v.lineFog[y] = uint8_t(6 - int(sky * 4));
            v.road[y].on = false;
            continue;
        }
        float depth = float(y - kHorizon) / float(gs::SCREEN_H - kHorizon);
        gs::RoadLine& rl = v.road[y];
        rl = {};
        rl.on = true;
        rl.cx = 160.f;
        rl.hw = 10.f + depth * depth * 150.f;
        rl.v = 2200.f / (depth + 0.15f);
        rl.pal = PAL_ROAD;
        rl.band = (int(std::floor(rl.v / 80.f)) & 1) ? 1 : 0;
        rl.style = 1;
        rl.left = gs::GROUND_WATER;
        rl.right = gs::GROUND_WATER;
        v.lineBackdrop[y] = gs::rgb4(1, 3, 6);
        v.lineFog[y] = uint8_t(std::clamp(int((1.f - depth) * 9.f), 0, 9));
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.hudEnabled = true;
    layRoad();

    for (int i = 3; i >= 0; --i) {
        float z = 0.22f + float(i) * 0.14f;
        float x, y, h;
        project(z, -1.15f, x, y, h);
        spr(art_.lamp, x, y, h * 0.55f, PAL_LAMP, false, int((1.f - z) * 8), true);
        project(z, 1.15f, x, y, h);
        spr(art_.lamp, x, y, h * 0.55f, PAL_LAMP, true, int((1.f - z) * 8), true);
    }

    if (mode_ != Mode::Title) {
        std::vector<int> order;
        for (int i = 0; i < int(trucks_.size()); i++) order.push_back(i);
        std::sort(order.begin(), order.end(), [&](int a, int b) { return trucks_[a].z < trucks_[b].z; });
        for (int i = int(order.size()) - 1; i >= 0; --i) {
            const Truck& tr = trucks_[order[i]];
            if (tr.z < 0.02f) continue;
            float x, y, h;
            project(tr.z, 0, x, y, h);
            int fog = int(std::clamp((0.55f - tr.z) * 14.f, 0.f, 10.f));
            spr(art_.truck, x, y - 2.f, h * 0.72f, PAL_TRUCK, false, fog, true);
        }

        float lx, ly, lh, rx, ry, rh;
        project(gateZ_, -0.92f, lx, ly, lh);
        project(gateZ_, 0.92f, rx, ry, rh);
        spr(art_.post, lx, ly, lh * 0.7f, PAL_STONE, false, 1, true);
        spr(art_.post, rx, ry, rh * 0.7f, PAL_STONE, false, 1, true);
        if (planted_) {
            float cx = (lx + rx) * 0.5f;
            float cy = (ly + ry) * 0.5f - lh * 0.28f;
            float span = std::fabs(rx - lx);
            spr(art_.chain, cx, cy, std::max(6.f, span * 0.16f), gateHolds() ? PAL_OK : PAL_ALERT, false, 0, false);
        }
    } else {
        float x, y, h;
        project(0.55f, 0, x, y, h);
        spr(art_.truck, x, y, h * 0.8f, PAL_TRUCK, false, 2, true);
        project(0.42f, 0, x, y, h);
        spr(art_.truck, x, y, h * 0.7f, PAL_TRUCK, false, 4, true);
    }

    spr(art_.keeper, 160.f, 210.f, 46.f, PAL_KEEPER, false, 0, true);

    if (mode_ == Mode::Title) {
        hudC(1, "CAUSEWAY COLUMN", PAL_HUD);
        hudC(3, "STOP THE COLUMN ON THE ROAD", PAL_HUD);
        hudC(6, "A TO TAKE THE CHAIN", PAL_HUD);
    } else if (mode_ == Mode::Play) {
        char buf[48];
        std::snprintf(buf, sizeof buf, "STOPPED %d/%d", stopped_, kColumn);
        hudC(0, buf, PAL_HUD);
        hudC(1, planted_ ? (gateHolds() ? "CHAIN ON THE ROAD" : "CHAIN MISSES THE SPAN") : "UP DOWN  WALK THE CHAIN",
             planted_ && !gateHolds() ? PAL_ALERT : PAL_HUD);
    } else if (mode_ == Mode::Victory) {
        hudC(2, "THE COLUMN STOPS ON THE ROAD", PAL_OK);
    } else {
        hudC(2, reason_, PAL_ALERT);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    if (mode_ == Mode::Title) {
        if (bot_ || sys.pad.pressed(gs::BTN_A) || sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_B)) begin();
    } else if (mode_ == Mode::Play) {
        update();
    } else if (mode_ == Mode::Fail || mode_ == Mode::Victory) {
        if (!bot_ && (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A))) begin();
    }
    if (mode_ != Mode::Title && mode_ != Mode::Fail && mode_ != Mode::Victory) {
        sys.apu.tone(2, 0, 0);
    }
    draw();
}

}  // namespace cway

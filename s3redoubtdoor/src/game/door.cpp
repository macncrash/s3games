#include "door.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace door {

namespace {
constexpr float DT = 1.f / 60.f;
constexpr float HOLD = 180.f;
constexpr float STRIKE_REACH = 0.30f;
}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Dead || mode_ == Mode::Victory) return 3;
    if (mode_ == Mode::Hold && door_ < 40) return 2;
    if (mode_ == Mode::Hold) return 1;
    return 0;
}

float Game::rnd() {
    rng_ = rng_ * 1664525u + 1013904223u;
    return float(rng_ >> 8) / float(1 << 24);
}

int Game::laneX(int lane) const { return 92 + lane * 68; }

void Game::begin() {
    mode_ = Mode::Hold;
    door_ = 100;
    score_ = 0;
    clock_ = 0;
    lane_ = 1;
    want_ = 1;
    slide_ = 0;
    strike_ = 0;
    cool_ = 0;
    spawn_ = 0.6f;
    shake_ = 0;
    lastLane_ = -1;
    foes_.clear();
    won_ = false;
    over_ = false;
    reason_ = "THE DOOR GAVE WAY";
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.apu.setMaster(0.45f);
    mode_ = Mode::Title;
}

void Game::botThink() {
    if (mode_ == Mode::Title) {
        if (titleT_ > 0.4f) begin();
        return;
    }
    if (mode_ != Mode::Hold) return;
    int best = -1;
    float bestD = 9.f;
    for (int i = 0; i < int(foes_.size()); i++) {
        if (foes_[i].dist < bestD) {
            bestD = foes_[i].dist;
            best = i;
        }
    }
    if (best >= 0) want_ = foes_[best].lane;
    if (best >= 0 && foes_[best].lane == lane_ && slide_ <= 0 && foes_[best].dist < STRIKE_REACH && cool_ <= 0)
        strike_ = 0.12f;
}

void Game::update(float dt) {
    if (mode_ == Mode::Title) {
        titleT_ += dt;
        return;
    }
    if (mode_ != Mode::Hold) return;

    if (want_ != lane_ && slide_ <= 0) slide_ = 0.12f;
    if (slide_ > 0) {
        slide_ -= dt;
        if (slide_ <= 0) {
            lane_ = want_;
            slide_ = 0;
            sys_->apu.tone(0, 180, 0.04f);
        }
    }
    if (cool_ > 0) cool_ -= dt;
    if (strike_ > 0) strike_ -= dt;
    if (shake_ > 0) shake_ = std::max(0.f, shake_ - dt);

    clock_ += dt;
    float press = std::min(1.f, clock_ / HOLD);
    spawn_ -= dt;
    if (spawn_ <= 0 && int(foes_.size()) < 2) {
        Foe f;
        int lane = int(rnd() * 3.f);
        if (lane == lastLane_) lane = (lane + 1 + int(rnd() * 1.5f)) % 3;
        f.lane = lane;
        lastLane_ = lane;
        f.kind = (rnd() < 0.22f + press * 0.2f) ? 1 : 0;
        f.dist = 0.78f;
        f.speed = (f.kind ? 0.16f : 0.22f) + press * 0.10f;
        f.flash = 0;
        bool close = false;
        for (const Foe& o : foes_) {
            float gap = std::fabs(o.dist / std::max(0.05f, o.speed) - f.dist / f.speed);
            if (gap < 0.85f) close = true;
        }
        if (!close) foes_.push_back(f);
        spawn_ = 1.15f - press * 0.45f + rnd() * 0.25f;
    }

    for (int i = int(foes_.size()) - 1; i >= 0; i--) {
        Foe& f = foes_[i];
        if (f.flash > 0) f.flash -= dt;
        f.dist -= f.speed * dt;
        bool mine = f.lane == lane_ && slide_ <= 0;
        if (mine && strike_ > 0 && f.dist < STRIKE_REACH && f.dist > -0.02f) {
            score_ += f.kind ? 25 : 10;
            sys_->apu.noiseBurst(0.35f, 1800.f, 0.08f);
            sys_->apu.tone(1, 320, 0.08f);
            foes_.erase(foes_.begin() + i);
            cool_ = 0.22f;
            strike_ = 0;
            continue;
        }
        if (f.dist <= 0) {
            int dmg = f.kind ? 18 : 12;
            door_ -= dmg;
            shake_ = 0.25f;
            sys_->apu.noiseBurst(0.6f, 400.f, 0.2f);
            sys_->apu.tone(2, 90, 0.12f);
            foes_.erase(foes_.begin() + i);
        }
    }

    if (door_ <= 0) {
        door_ = 0;
        mode_ = Mode::Dead;
        won_ = false;
        over_ = true;
        reason_ = "THE DOOR GAVE WAY";
        sys_->apu.tone(0, 70, 0.2f);
        return;
    }
    if (clock_ >= HOLD) {
        clock_ = HOLD;
        mode_ = Mode::Victory;
        won_ = true;
        over_ = true;
        reason_ = "THE DOOR HELD FOR THREE MINUTES";
        score_ += 300;
        sys_->apu.tone(0, 440, 0.16f);
        sys_->apu.tone(1, 554, 0.14f);
        sys_->apu.tone(2, 659, 0.12f);
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    float jx = (shake_ > 0) ? std::sin(shake_ * 80.f) * 3.f : 0;
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::lround(cx - s.w * 0.5f + jx));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        int x = col + i;
        if (x < 0 || x > 39 || c < 32 || c > 126) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = 0;
    while (s[n]) n++;
    hud(20 - n / 2, row, s, pal);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.road[y].on = false;
        v.lineFog[y] = 0;
        int r, g, b;
        if (y < 96) {
            r = 2 + y / 40;
            g = 2 + y / 48;
            b = 5 + y / 30;
        } else {
            r = 3;
            g = 3;
            b = 2;
        }
        v.lineBackdrop[y] = gs::rgb4(std::clamp(r, 0, 15), std::clamp(g, 0, 15), std::clamp(b, 0, 15));
    }

    spr(art_.wall, 160, 118, 168, PAL_STONE);
    const gs::Mipped& leaf = door_ < 45 ? art_.doorhurt : art_.door;
    spr(leaf, 160, 128, 118, PAL_WOOD);
    float barX = float(laneX(lane_));
    spr(art_.bar, barX, 150, 12, PAL_IRON);

    std::vector<int> order(foes_.size());
    for (int i = 0; i < int(order.size()); i++) order[i] = i;
    std::sort(order.begin(), order.end(), [&](int a, int b) { return foes_[a].dist > foes_[b].dist; });
    for (int id : order) {
        const Foe& f = foes_[id];
        float near = 1.f - std::clamp(f.dist, 0.f, 1.f);
        float h = 18.f + near * 36.f;
        float y = 168.f - near * 28.f;
        float x = float(laneX(f.lane));
        int step = int(clock_ * 6.f + f.lane) & 1;
        if (f.kind) spr(art_.ram, x, y - h * 0.15f, h * 0.45f, PAL_IRON);
        spr(f.kind ? art_.foe[step] : art_.foe[step], x, y, h, PAL_FOE, f.lane == 0);
    }

    if (mode_ != Mode::Dead) {
        int step = (strike_ > 0) ? 1 : 0;
        float px = float(laneX(lane_));
        if (slide_ > 0) {
            float u = slide_ / 0.12f;
            px = float(laneX(want_)) * (1.f - u) + float(laneX(lane_)) * u;
        }
        spr(art_.you[step], px, 156, 52, PAL_YOU, false);
        if (strike_ > 0) spr(art_.spark, px + 16, 148, 16, PAL_ALERT);
    }

    if (mode_ == Mode::Title) {
        hudC(3, "S3 REDOUBT DOOR", PAL_HUD);
        hudC(6, "HOLD THE DOOR", PAL_OK);
        hudC(8, "THREE MINUTES", PAL_HUD);
        hudC(12, "LEFT AND RIGHT  THE BAR", PAL_HUD);
        hudC(14, "A  STRIKE THE RAM", PAL_HUD);
        hudC(18, "MISS THE DOOR AND", PAL_ALERT);
        hudC(20, "THE WATCH IS OVER", PAL_ALERT);
        hudC(24, "START", PAL_OK);
    } else {
        int sec = int(clock_);
        if (sec > 180) sec = 180;
        char line[48];
        std::snprintf(line, sizeof(line), "TIME %d:%02d / 3:00", sec / 60, sec % 60);
        hud(1, 1, line, PAL_HUD);
        std::snprintf(line, sizeof(line), "DOOR %d", door_);
        hud(28, 1, line, door_ < 40 ? PAL_ALERT : PAL_OK);
        std::snprintf(line, sizeof(line), "SCORE %d", score_);
        hud(1, 26, line, PAL_HUD);
        const char* leaf = lane_ == 0 ? "LEFT LEAF" : lane_ == 2 ? "RIGHT LEAF" : "CENTRE LEAF";
        hud(26, 26, leaf, PAL_HUD);
        if (mode_ == Mode::Hold && door_ < 40) hudC(4, "THE DOOR IS FAILING", PAL_ALERT);
        if (mode_ == Mode::Victory) {
            hudC(8, "THE DOOR HELD", PAL_OK);
            hudC(10, "FOR THREE MINUTES", PAL_OK);
        }
        if (mode_ == Mode::Dead) {
            hudC(8, "THE DOOR GAVE WAY", PAL_ALERT);
            hudC(10, "THE WATCH IS OVER", PAL_ALERT);
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    gs::Pad& p = sys.pad;
    if (mode_ == Mode::Title) {
        if (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C)) begin();
    } else if (mode_ == Mode::Hold) {
        if (p.pressed(gs::BTN_LEFT) || p.down(gs::BTN_LEFT)) {
            if (p.pressed(gs::BTN_LEFT) || want_ == lane_) want_ = std::max(0, lane_ - 1);
        }
        if (p.pressed(gs::BTN_RIGHT)) want_ = std::min(2, lane_ + 1);
        if (p.axisX < -0.4f && p.pressed(gs::BTN_LEFT) == false) {
            /* stick is continuous; step once per press via keys */
        }
        if (p.axisX < -0.5f && slide_ <= 0 && want_ == lane_) want_ = std::max(0, lane_ - 1);
        if (p.axisX > 0.5f && slide_ <= 0 && want_ == lane_) want_ = std::min(2, lane_ + 1);
        if ((p.pressed(gs::BTN_A) || p.pressed(gs::BTN_B) || p.pressed(gs::BTN_C)) && cool_ <= 0 && slide_ <= 0) {
            strike_ = 0.12f;
            cool_ = 0.22f;
        }
    } else if (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A)) {
        over_ = false;
        mode_ = Mode::Title;
        titleT_ = 0;
    }
    if (bot_) botThink();
    update(DT);
    draw();
    if (sys.frame % 8 == 0) {
        sys.apu.tone(0, 0, 0);
        sys.apu.tone(1, 0, 0);
    }
}

}  // namespace door

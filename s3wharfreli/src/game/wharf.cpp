#include "wharf.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace wharf {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float Z0 = 1.55f;
constexpr float BELL_AT = 22.f;
constexpr float RELIEF_AT = 28.f;
constexpr int HORIZON = 78;

float speedOf(int kind) {
    if (kind == 2) return 0.13f;
    if (kind == 1) return 0.24f;
    return 0.19f;
}
int hpOf(int kind) { return kind == 2 ? 3 : 1; }
int ptsOf(int kind) { return kind == 2 ? 300 : kind == 1 ? 140 : 100; }

struct Arr {
    float t;
    int kind;
    int lane;
};
// One lane at a time, with room for the tug's three shots.
const Arr kArr[] = {
    {1.1f, 0, -1}, {3.6f, 0, 1},  {6.0f, 1, 0},   {8.2f, 2, -1}, {12.4f, 0, 1},
    {14.6f, 1, 0}, {16.8f, 0, -1}, {19.0f, 1, 1}, {21.6f, 0, 0}, {24.0f, 0, -1},
    {25.8f, 1, 1},
};

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Victory || mode_ == Mode::Over) return 3;
    if (bell_) return 2;
    return 1;
}

float Game::eta(const Foe& f) const { return f.alive ? f.z / f.speed : 1.0e9f; }

void Game::place(const Foe& f, float& x, float& y, float& h) const {
    float near = 1.f - std::clamp(f.z / Z0, 0.f, 1.f);
    x = 160.f + float(f.lane) * (16.f + near * 86.f);
    float feet = 96.f + near * 104.f;
    float tall = f.kind == TUG ? 30.f : f.kind == SKIFF ? 22.f : 48.f;
    h = 10.f + near * tall;
    y = feet - h + (f.kind != BOARDER ? std::sin(tide_ * 2.4f + f.z) * 2.f : 0.f);
}

void Game::winWatch() {
    if (won_ || mode_ == Mode::Over) return;
    won_ = true;
    over_ = true;
    mode_ = Mode::Victory;
    modeT_ = 0;
    reason_ = "THE WATCH HELD UNTIL THE RELIEF BELL";
    sys_->apu.keyOff(1);
    sys_->apu.noiseBurst(0.1f, 1600.f, 0.05f);
    fanStep_ = 0;
    fanT_ = 0;
    sys_->setLight(40, 180, 90);
}

void Game::loseWatch(const char* why) {
    if (won_ || mode_ == Mode::Over || mode_ == Mode::Victory) return;
    reason_ = why;
    won_ = false;
    over_ = true;
    mode_ = Mode::Over;
    modeT_ = 0;
    sys_->apu.keyOff(1);
    sys_->apu.noiseBurst(0.4f, 140.f, 0.3f);
    sys_->setLight(180, 20, 30);
    shake_ = 0.5f;
}

void Game::kill(Foe& f) {
    f.alive = false;
    score_ += f.points;
    sys_->apu.noiseBurst(0.14f, 700.f, 0.05f);
    sys_->apu.tone(0, 480.f, 0.04f);
}

void Game::shoot() {
    if (cool_ > 0) return;
    cool_ = 0.18f;
    flash_ = 4;
    sys_->apu.noiseBurst(0.16f, 1400.f, 0.03f);
    int best = -1;
    float bestZ = 1.0e9f;
    for (int i = 0; i < int(foes_.size()); i++) {
        if (!foes_[i].alive || foes_[i].lane != lane_) continue;
        if (foes_[i].z < bestZ) {
            bestZ = foes_[i].z;
            best = i;
        }
    }
    if (best < 0 || bestZ > 1.3f) return;
    Foe& f = foes_[best];
    f.hp -= 1;
    if (f.hp <= 0) kill(f);
    else sys_->apu.tone(2, 190.f, 0.04f);
}

void Game::beginWatch() {
    mode_ = Mode::Watch;
    over_ = false;
    won_ = false;
    bell_ = false;
    reason_ = "THE WHARF WAS TAKEN";
    score_ = 0;
    lane_ = 0;
    spawnAt_ = 0;
    flash_ = 0;
    fanStep_ = -1;
    watch_ = 0;
    cool_ = 0;
    shake_ = 0;
    bellTick_ = 0;
    modeT_ = 0;
    tide_ = 0;
    foes_.clear();
    script_.clear();
    for (const Arr& a : kArr) {
        Spawn s;
        s.t = a.t;
        s.kind = a.kind;
        s.lane = a.lane;
        script_.push_back(s);
    }
    if (sys_) sys_->setLight(20, 40, 70);
}

void Game::update(float dt) {
    watch_ += dt;
    tide_ += dt;
    if (cool_ > 0) cool_ -= dt;
    if (flash_ > 0) flash_--;
    if (shake_ > 0) shake_ -= dt;

    while (spawnAt_ < int(script_.size()) && script_[spawnAt_].t <= watch_) {
        const Spawn& s = script_[spawnAt_++];
        Foe f;
        f.kind = s.kind;
        f.lane = s.lane;
        f.z = Z0;
        f.speed = speedOf(s.kind);
        f.hp = hpOf(s.kind);
        f.points = ptsOf(s.kind);
        f.alive = true;
        foes_.push_back(f);
    }

    bool wasBell = bell_;
    bell_ = watch_ >= BELL_AT && mode_ == Mode::Watch;
    if (bell_ && !wasBell) {
        bellTick_ = 0;
        sys_->apu.keyOn(1, 392.f, 0.22f);
        sys_->rumble(0.25f, 0.45f, 160);
    } else if (bell_ && !won_) {
        bellTick_ += dt;
        if (bellTick_ >= 0.9f) {
            bellTick_ = 0;
            sys_->apu.keyOn(1, 392.f, 0.18f);
        }
    }

    for (Foe& f : foes_) {
        if (!f.alive) continue;
        f.age++;
        f.z -= f.speed * dt;
        if (f.z <= 0.02f) {
            loseWatch("THE WHARF WAS TAKEN");
            return;
        }
    }

    if (watch_ >= RELIEF_AT) {
        winWatch();
        return;
    }

    int best = -1;
    float mind = 1.0e9f;
    for (int i = 0; i < int(foes_.size()); i++) {
        if (!foes_[i].alive) continue;
        float e = eta(foes_[i]);
        if (e < mind) {
            mind = e;
            best = i;
        }
    }

    const gs::Pad& pad = sys_->pad;
    if (bot_) {
        if (best >= 0) lane_ = foes_[best].lane;
        if (best >= 0 && foes_[best].lane == lane_ && eta(foes_[best]) < 7.f) shoot();
    } else {
        if (pad.pressed(gs::BTN_LEFT)) lane_ = std::max(-1, lane_ - 1);
        if (pad.pressed(gs::BTN_RIGHT)) lane_ = std::min(1, lane_ + 1);
        bool fire = pad.down(gs::BTN_A) || pad.down(gs::BTN_B) || pad.down(gs::BTN_C) || pad.accel > 0.4f;
        if (fire) shoot();
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog) {
    if (h < 1.f || m.h < 1) return;
    float w = h * (float(m.w) / float(m.h));
    gs::Sprite s;
    s.x = int16_t(std::lround(cx - w * 0.5f));
    s.y = int16_t(std::lround(cy));
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
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
        if (x < 0 || x > 39 || c < 32 || c > 126) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = int(std::strlen(s));
    hud(20 - n / 2, row, s, pal);
}

void Game::text(const char* s, float x, float y, float scale, int pal) {
    int n = int(std::strlen(s));
    float adv = 16.f * scale;
    float left = x - n * adv * 0.5f;
    for (int i = 0; i < n; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < 33 || c > 126) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, left + float(i) * adv + adv * 0.5f, y, std::max(8.f, float(g.h) * scale), pal, false, 0);
    }
}

void Game::road() {
    gs::VDP& v = sys_->vdp;
    v.roadTime = int(tide_ * 60.f);
    float sway = std::sin(tide_ * 0.4f) * 10.f;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < HORIZON) {
            float sky = float(y) / float(HORIZON);
            v.lineBackdrop[y] = gs::rgb4(1 + int(sky * 3), 2 + int(sky * 3), 6 + int((1.f - sky) * 5));
            v.lineFog[y] = uint8_t(7 - int(sky * 5));
            v.road[y].on = false;
            continue;
        }
        float depth = float(y - HORIZON) / float(gs::SCREEN_H - HORIZON);
        gs::RoadLine& rl = v.road[y];
        rl = {};
        rl.on = true;
        rl.cx = 160.f + sway * (1.f - depth);
        rl.hw = 6.f + depth * depth * 150.f;
        rl.v = 2400.f / (depth + 0.14f) - tide_ * 40.f;
        rl.pal = 12;
        rl.band = (int(std::floor(rl.v / 70.f)) & 1) ? 1 : 0;
        rl.style = 0;
        rl.left = gs::GROUND_WATER;
        rl.right = gs::GROUND_WATER;
        v.lineBackdrop[y] = gs::rgb4(1, 2, 5);
        v.lineFog[y] = uint8_t(std::clamp(int((1.f - depth) * 11.f), 0, 11));
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.hudEnabled = true;
    int jx = shake_ > 0 ? int(std::sin(tide_ * 40.f) * 3.f) : 0;
    road();

    float swing = std::sin((modeT_ + watch_) * (bell_ ? 11.f : 1.6f)) * (bell_ ? 7.f : 1.2f);
    spr(art_.bell, 292.f + swing + jx, 16.f, bell_ ? 30.f : 22.f, PAL_BELL, false, 0);
    spr(art_.lantern, 36.f + jx, 20.f, 16.f, PAL_FX, false, 0);

    for (int i = 0; i < 5; i++) {
        float near = 0.18f + float(i) * 0.18f;
        float h = 8.f + near * 26.f;
        float y = 86.f + near * 100.f - h;
        float spread = 22.f + near * 130.f;
        int fog = int((1.f - near) * 9);
        spr(art_.piling, 160.f - spread + jx, y, h, PAL_WOOD, false, fog);
        spr(art_.piling, 160.f + spread + jx, y, h, PAL_WOOD, false, fog);
    }
    spr(art_.crate, 118.f + jx, 150.f, 18.f, PAL_WOOD, false, 0);
    spr(art_.crate, 206.f + jx, 158.f, 16.f, PAL_WOOD, false, 0);

    if (mode_ == Mode::Title) {
        int fr = int(modeT_ * 5.f) & 1;
        spr(art_.skiff, 70, 108, 22, PAL_SKIFF, false, 2);
        spr(art_.tug, 250, 100, 26, PAL_TUG, true, 3);
        spr(art_.boarder[fr], 128, 120, 36, PAL_FOE, false, 1);
        spr(art_.sentry[fr], 168, 148, 54, PAL_YOU, false, 0);
    } else {
        std::vector<int> order;
        for (int i = 0; i < int(foes_.size()); i++)
            if (foes_[i].alive) order.push_back(i);
        // Earlier sprites stay on top, so the near end of the wharf is submitted first.
        std::sort(order.begin(), order.end(), [&](int a, int b) { return foes_[a].z < foes_[b].z; });
        for (int id : order) {
            const Foe& f = foes_[id];
            float x, y, h;
            place(f, x, y, h);
            x += jx;
            int fog = int(std::clamp(f.z * 7.f, 0.f, 12.f));
            if (f.kind == TUG) spr(art_.tug, x, y, h, PAL_TUG, f.lane > 0, fog);
            else if (f.kind == SKIFF) spr(art_.skiff, x, y, h, PAL_SKIFF, f.lane < 0, fog);
            else spr(art_.boarder[(f.age / 7) & 1], x, y, h, PAL_FOE, f.lane < 0, fog);
        }
        float px = 160.f + float(lane_) * 52.f + jx;
        int fr = (cool_ > 0.08f) ? 1 : (int(watch_ * 6.f) & 1);
        if (flash_ > 0) spr(art_.flash, px + (lane_ < 0 ? -14.f : 14.f), 166.f, 12.f, PAL_FX, false, 0);
        spr(art_.sentry[fr], px, 158.f, 58.f, PAL_YOU, lane_ < 0, 0);
    }

    char line[48];
    if (mode_ == Mode::Title) {
        hudC(22, "YOU HAVE THE WHARF.", PAL_HUD);
        hudC(23, "HOLD UNTIL THE RELIEF BELL.", PAL_OK);
        hudC(24, "LEFT RIGHT THE PILES    A FIRES", PAL_HUD);
        hudC(25, "ANYTHING ELSE IS A LOSS.", PAL_ALERT);
        hudC(27, "ENTER", PAL_HUD);
        int n = int(std::strlen(S3_VERSION_STRING));
        hud(39 - n, 0, S3_VERSION_STRING, PAL_HUD);
    } else {
        int left = std::max(0, int(std::ceil(RELIEF_AT - watch_)));
        if (!bell_) std::snprintf(line, sizeof line, "BELL %02d", std::max(0, int(std::ceil(BELL_AT - watch_))));
        else std::snprintf(line, sizeof line, "RELIEF %02d", left);
        hud(1, 0, line, bell_ ? PAL_OK : PAL_HUD);
        std::snprintf(line, sizeof line, "SCORE %d", score_);
        hud(28, 0, line, PAL_HUD);
        const char* pile = lane_ < 0 ? "PORT" : lane_ > 0 ? "STARBOARD" : "PLANK";
        std::snprintf(line, sizeof line, "PILE %s", pile);
        hud(1, 27, line, PAL_HUD);
        if (mode_ == Mode::Pause) hudC(25, "PAUSED", PAL_HUD);
        else if (mode_ == Mode::Victory) hudC(25, "THE WATCH HELD", PAL_OK);
        else if (mode_ == Mode::Over) hudC(25, reason_, PAL_ALERT);
        else if (bell_) hudC(25, "THE BELL. HOLD THE WHARF.", PAL_BELL);
        else hudC(25, "HOLD THE WHARF", PAL_HUD);
    }

    if (mode_ == Mode::Title) text("S3 WHARF", 160, 18, 0.85f, PAL_HUD);
    else if (mode_ == Mode::Victory) text("RELIEF", 160, 30, 1.1f, PAL_OK);
    else if (mode_ == Mode::Over) text("LOST", 160, 30, 1.1f, PAL_ALERT);
    else if (bell_) text("THE BELL", 160, 30, 0.8f, PAL_BELL);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    mode_ = Mode::Title;
    modeT_ = 0;
    if (bot_) beginWatch();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    modeT_ += DT;
    const gs::Pad& pad = sys.pad;
    bool start = !bot_ && pad.pressed(gs::BTN_START);
    bool back = !bot_ && pad.pressed(gs::BTN_MODE);
    if (back && mode_ == Mode::Title) {
        if (sys.hasHome()) sys.eject();
        else sys.quit();
    } else if (back && mode_ == Mode::Watch) mode_ = Mode::Pause;
    else if (back && mode_ == Mode::Pause) mode_ = Mode::Title;
    else if (start && (mode_ == Mode::Title || mode_ == Mode::Victory || mode_ == Mode::Over)) beginWatch();
    else if (start && mode_ == Mode::Watch) mode_ = Mode::Pause;
    else if (start && mode_ == Mode::Pause) mode_ = Mode::Watch;

    if (mode_ == Mode::Watch) update(DT);
    else if (mode_ == Mode::Victory && fanStep_ >= 0) {
        fanT_ += DT;
        if (fanT_ > 0.16f) {
            fanT_ = 0;
            static const float notes[] = {330.f, 392.f, 494.f, 659.f};
            if (fanStep_ < 4) sys.apu.tone(0, notes[fanStep_], 0.06f);
            fanStep_++;
            if (fanStep_ > 6) {
                fanStep_ = -1;
                sys.apu.tone(0, 0, 0);
            }
        }
    }
    draw();
}

}  // namespace wharf

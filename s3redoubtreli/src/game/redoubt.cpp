#include "redoubt.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace redoubt {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float BELL_AT = 20.f;
constexpr float BELL_END = 24.5f;
constexpr float ROPE_NEED = 1.05f;
constexpr float Z0 = 1.22f;
constexpr float LOSE_Z = 0.03f;

struct Arr {
    float t;
    int kind;
    int lane;
};

// The approach is scripted so a steady gun on the berm can hold it.
const Arr kArr[] = {
    {1.2f, 0, 0},  {3.4f, 0, -1}, {5.2f, 0, 1},  {7.6f, 1, 0},  {10.0f, 0, -1},
    {12.2f, 0, 1}, {14.0f, 0, 0}, {16.2f, 1, 1}, {18.0f, 0, -1},
};

float speedOf(int kind) { return kind == 1 ? 0.105f : 0.175f; }
int hpOf(int kind) { return kind == 1 ? 2 : 1; }
int ptsOf(int kind) { return kind == 1 ? 250 : 100; }

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Victory || mode_ == Mode::Over) return 3;
    if (bell_) return 2;
    return 1;
}

float Game::eta(const Foe& f) const { return f.alive ? std::max(0.f, f.z - LOSE_Z) / std::max(0.02f, f.speed) : 99.f; }

void Game::place(const Foe& f, float& x, float& y, float& h) const {
    float t = std::clamp(1.f - f.z / Z0, 0.f, 1.f);
    float spread = 16.f + t * 78.f;
    x = 160.f + float(f.lane) * spread;
    y = 86.f + t * 92.f;
    h = 12.f + t * 40.f;
}

void Game::kill(Foe& f) {
    f.alive = false;
    score_ += f.points;
    shake_ = 0.12f;
    if (sys_) sys_->apu.tone(3, 140.f, 0.08f);
}

void Game::winWatch() {
    mode_ = Mode::Victory;
    won_ = true;
    over_ = true;
    reason_ = "THE REDOUBT HELD UNTIL THE RELIEF BELL";
    score_ += 1000;
    if (sys_) {
        sys_->apu.keyOn(1, 660.f, 0.3f);
        sys_->setLight(40, 170, 70);
    }
}

void Game::loseWatch(const char* why) {
    mode_ = Mode::Over;
    won_ = false;
    over_ = true;
    reason_ = why;
    if (sys_) {
        sys_->apu.tone(2, 70.f, 0.2f);
        sys_->setLight(170, 30, 20);
    }
}

void Game::shoot() {
    if (cool_ > 0 || mode_ != Mode::Watch) return;
    cool_ = 0.26f;
    flash_ = 4;
    if (sys_) sys_->apu.noiseBurst(0.35f, 0.55f, 0.08f);
    int best = -1;
    float bestZ = 1.0e9f;
    for (int i = 0; i < int(foes_.size()); i++) {
        if (!foes_[i].alive || foes_[i].lane != lane_) continue;
        if (foes_[i].z < bestZ) {
            bestZ = foes_[i].z;
            best = i;
        }
    }
    if (best < 0 || bestZ > 1.15f) return;
    Foe& f = foes_[best];
    f.hp -= 1;
    if (f.hp <= 0) kill(f);
    else if (sys_) sys_->apu.tone(2, 220.f, 0.05f);
}

void Game::beginWatch() {
    mode_ = Mode::Watch;
    over_ = false;
    won_ = false;
    bell_ = false;
    reason_ = "THE WATCH FAILED";
    score_ = 0;
    lane_ = 0;
    aim_ = 0;
    spawnAt_ = 0;
    flash_ = 0;
    watch_ = 0;
    rope_ = 0;
    cool_ = 0;
    shake_ = 0;
    bellTick_ = 0;
    modeT_ = 0;
    axisLatch_ = 0;
    foes_.clear();
    script_.clear();
    for (const Arr& a : kArr) {
        Spawn s;
        s.t = a.t;
        s.kind = a.kind;
        s.lane = a.lane;
        script_.push_back(s);
    }
    if (sys_) sys_->setLight(50, 36, 22);
}

void Game::update(float dt) {
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
        sys_->apu.keyOn(1, 480.f, 0.24f);
        sys_->rumble(0.25f, 0.4f, 140);
    } else if (bell_ && !won_) {
        bellTick_ += dt;
        if (bellTick_ >= 0.85f) {
            bellTick_ = 0;
            sys_->apu.keyOn(1, 480.f, 0.18f);
        }
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

    bool pull = false;
    int want = lane_;
    if (bot_) {
        bool closing = bell_ && (BELL_END - watch_) < ROPE_NEED + 1.4f;
        bool safe = mind > (closing ? 0.85f : 1.5f);
        if (bell_ && safe && watch_ < BELL_END) pull = true;
        else if (best >= 0) want = foes_[best].lane;
    } else {
        const gs::Pad& pad = sys_->pad;
        if (pad.pressed(gs::BTN_LEFT)) want = std::max(-1, lane_ - 1);
        if (pad.pressed(gs::BTN_RIGHT)) want = std::min(1, lane_ + 1);
        float ax = pad.axisX;
        if (ax < -0.45f && axisLatch_ >= -0.45f) want = std::max(-1, lane_ - 1);
        if (ax > 0.45f && axisLatch_ <= 0.45f) want = std::min(1, lane_ + 1);
        axisLatch_ = ax;
        bool fire = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_Z) || pad.down(gs::BTN_A);
        bool up = pad.down(gs::BTN_UP) || pad.down(gs::BTN_B);
        if (bell_ && up) pull = true;
        else if (fire) shoot();
    }
    if (!pull) lane_ = want;
    aim_ += (float(lane_) - aim_) * std::min(1.f, dt * 9.f);

    if (bot_ && !pull && best >= 0 && foes_[best].lane == lane_ && eta(foes_[best]) < 5.5f) shoot();

    if (pull && bell_) {
        rope_ += dt;
        if (rope_ >= ROPE_NEED) winWatch();
    } else if (rope_ > 0 && !won_) {
        rope_ = std::max(0.f, rope_ - dt * 0.65f);
    }

    if (won_ || mode_ != Mode::Watch) return;

    for (Foe& f : foes_) {
        if (!f.alive) continue;
        f.age++;
        f.z -= f.speed * dt;
        if (f.z <= LOSE_Z) {
            loseWatch("THE BERM IS LOST");
            return;
        }
    }
    if (bell_ && watch_ >= BELL_END && !won_) loseWatch("THE BELL WENT UNANSWERED");
    watch_ += dt;
    if (cool_ > 0) cool_ -= dt;
    if (flash_ > 0) flash_--;
    if (shake_ > 0) shake_ = std::max(0.f, shake_ - dt);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::lround(cx - s.w * 0.5f + shx_));
    s.y = int16_t(std::lround(cy - s.h * 0.5f + shy_));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::stamp(const gs::Mipped& m, float x, float y, float w, float h, int pal) {
    if (w < 1.f || h < 1.f || m.h < 1) return;
    gs::Sprite s;
    s.x = int16_t(std::lround(x + shx_));
    s.y = int16_t(std::lround(y + shy_));
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
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

void Game::road() {
    gs::VDP& v = sys_->vdp;
    const float horizon = 84.f;
    const float sway = std::sin(watch_ * 0.35f) * 18.f;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float sky = float(y) / horizon;
        int r = 4 + int(sky * 6.f);
        int g = 3 + int(sky * 3.f);
        int b = 6 - int(sky * 2.f);
        if (y >= int(horizon)) {
            r = 3;
            g = 3;
            b = 2;
        }
        v.lineBackdrop[y] = gs::rgb4(std::clamp(r, 0, 15), std::clamp(g, 0, 15), std::clamp(b, 0, 15));
        v.lineFog[y] = y < 70 ? uint8_t(4) : 0;
        gs::RoadLine& rl = v.road[y];
        if (y < int(horizon) || y > 200) {
            rl.on = false;
            continue;
        }
        float t = (float(y) - horizon) / (200.f - horizon);
        t = std::clamp(t, 0.02f, 1.f);
        rl.on = true;
        rl.cx = 160.f + sway * (1.f - t);
        rl.hw = 8.f + t * t * 150.f;
        rl.v = watch_ * 420.f + 80.f / t;
        rl.pal = PAL_ROAD;
        rl.band = (int(rl.v / 180.f) & 1) ? 1 : 0;
        rl.style = gs::ROAD_RUTS;
        rl.left = gs::GROUND_LAND;
        rl.right = gs::GROUND_LAND;
    }
    v.roadTime = int(watch_ * 60.f);
}

void Game::draw() {
    shx_ = shy_ = 0;
    if (shake_ > 0.05f) {
        shx_ = std::sin(watch_ * 48.f) * 2.5f;
        shy_ = std::cos(watch_ * 37.f) * 1.2f;
    }
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.hudEnabled = true;
    road();

    float swing = std::sin((modeT_ + watch_) * (bell_ ? 10.f : 1.6f)) * (bell_ ? 6.f : 1.f);
    stamp(art_.frame, 8, 18, 36, 78, PAL_EARTH);
    spr(art_.bell, 26.f + swing, 36.f, bell_ ? 28.f : 22.f, PAL_BELL, false, 0);
    spr(art_.rope, 26.f + swing * 0.25f, 68.f, 34.f + rope_ * 8.f, PAL_BELL, false, 0);

    if (mode_ == Mode::Title) {
        int fr = int(modeT_ * 6.f) & 1;
        spr(art_.walker[fr], 168, 130, 36, PAL_FOE, false, 2);
        spr(art_.shield[fr ^ 1], 118, 124, 28, PAL_SHIELD, true, 6);
        spr(art_.stake, 70, 150, 22, PAL_EARTH, false, 0);
        spr(art_.stake, 250, 148, 24, PAL_EARTH, false, 0);
    } else {
        std::vector<int> order;
        for (int i = 0; i < int(foes_.size()); i++)
            if (foes_[i].alive) order.push_back(i);
        std::sort(order.begin(), order.end(), [&](int a, int b) { return foes_[a].z > foes_[b].z; });
        for (int i : order) {
            const Foe& f = foes_[i];
            float x, y, h;
            place(f, x, y, h);
            int fog = int(std::clamp(f.z * 8.f, 0.f, 11.f));
            const gs::Mipped* img = &art_.walker[(f.age / 7) & 1];
            int pal = PAL_FOE;
            if (f.kind == SHIELD) {
                img = &art_.shield[(f.age / 8) & 1];
                pal = PAL_SHIELD;
            }
            spr(*img, x, y, h, pal, f.lane > 0, fog);
        }
        for (int s = -2; s <= 2; s++) {
            if (s == 0) continue;
            float t = 0.55f + 0.08f * float(s);
            spr(art_.stake, 160.f + float(s) * 62.f, 100.f + t * 40.f, 16.f + t * 8.f, PAL_EARTH, false, 4);
        }
    }

    float gx = 160.f + aim_ * 54.f;
    gx = std::clamp(gx, 70.f, 250.f);
    if (flash_ > 0) spr(art_.flash, gx + 16.f, 176.f, 18.f, PAL_FX, false, 0);
    spr(art_.gun, gx, 188.f, 36.f, PAL_YOU, aim_ < 0, 0);
    stamp(art_.berm, 0, 168, 320, 56, PAL_EARTH);

    char line[64];
    if (mode_ == Mode::Title) {
        hudC(22, "YOU HAVE THE REDOUBT.", PAL_HUD);
        hudC(23, "HOLD UNTIL THE RELIEF BELL.", PAL_OK);
        hudC(24, "LEFT RIGHT THE BERM    A FIRE", PAL_HUD);
        hudC(25, "UP ANSWERS THE BELL", PAL_BELL);
        hudC(26, "START", PAL_HUD);
        int n = int(std::strlen(S3_VERSION_STRING));
        hud(39 - n, 0, S3_VERSION_STRING, PAL_HUD);
    } else {
        int left = std::max(0, int(std::ceil(BELL_AT - watch_)));
        if (!bell_ && mode_ == Mode::Watch) std::snprintf(line, sizeof line, "BELL %02d", left);
        else if (bell_ && mode_ == Mode::Watch)
            std::snprintf(line, sizeof line, "ROPE %d", int(std::clamp(rope_ / ROPE_NEED, 0.f, 1.f) * 10.f));
        else
            std::snprintf(line, sizeof line, "SCORE %d", score_);
        hud(1, 0, line, bell_ ? PAL_OK : PAL_HUD);
        std::snprintf(line, sizeof line, "LANE %+d", lane_);
        hud(32, 0, line, PAL_HUD);
        std::snprintf(line, sizeof line, "SCORE %d", score_);
        hud(1, 27, line, PAL_HUD);
        if (mode_ == Mode::Pause) hudC(25, "PAUSED", PAL_HUD);
        else if (mode_ == Mode::Victory) hudC(25, "THE REDOUBT HELD", PAL_OK);
        else if (mode_ == Mode::Over) hudC(25, reason_, PAL_ALERT);
        else if (bell_) hudC(25, "ANSWER THE BELL", PAL_BELL);
        else hudC(25, "HOLD THE BERM", PAL_HUD);
    }
    if (mode_ == Mode::Victory) sys_->setLight(40, 170, 80);
    else if (mode_ == Mode::Over) sys_->setLight(170, 20, 30);
    else if (bell_) sys_->setLight(160, 120, 30);
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
    const float dt = DT;
    modeT_ += dt;
    const gs::Pad& pad = sys.pad;
    bool start = !bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A));
    bool back = !bot_ && pad.pressed(gs::BTN_MODE);
    if (back && mode_ == Mode::Title) {
        if (sys.hasHome()) sys.eject();
        return;
    }
    if (mode_ == Mode::Title) {
        if (start) beginWatch();
        draw();
        return;
    }
    if (mode_ == Mode::Pause) {
        if (start) mode_ = Mode::Watch;
        draw();
        return;
    }
    if (mode_ == Mode::Victory || mode_ == Mode::Over) {
        if (start) beginWatch();
        draw();
        return;
    }
    if (start) {
        mode_ = Mode::Pause;
        draw();
        return;
    }
    update(dt);
    draw();
}

}  // namespace redoubt

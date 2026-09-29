#include "causeway.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace causeway {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float Z0 = 1.45f;
constexpr float BELL_AT = 20.f;
constexpr float RELIEF_AT = 26.f;
constexpr int HORIZON = 72;

float speedOf(int kind) { return kind == 1 ? 0.15f : 0.22f; }
int hpOf(int kind) { return kind == 1 ? 3 : 1; }
int ptsOf(int kind) { return kind == 1 ? 250 : 100; }

struct Arr {
    float t;
    int kind;
    int lane;
};
// Spaced so one lane is always clear before the next threat is close.
const Arr kArr[] = {
    {1.2f, 0, 0},  {3.4f, 0, -1}, {5.6f, 0, 1},   {7.6f, 1, 0},  {10.2f, 0, 1},
    {12.2f, 0, -1}, {14.4f, 0, 0}, {16.4f, 1, -1}, {19.0f, 0, 1}, {21.4f, 0, 0},
    {23.2f, 0, -1},
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
    x = 160.f + float(f.lane) * (18.f + near * 78.f);
    float feet = 92.f + near * 108.f;
    h = 12.f + near * (f.kind == TRUCK ? 36.f : 46.f);
    y = feet - h;
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
    sys_->apu.noiseBurst(0.4f, 160.f, 0.3f);
    sys_->setLight(180, 20, 30);
    shake_ = 0.5f;
}

void Game::kill(Foe& f) {
    f.alive = false;
    score_ += f.points;
    sys_->apu.noiseBurst(0.14f, 800.f, 0.05f);
    sys_->apu.tone(0, 540.f, 0.04f);
}

void Game::shoot() {
    if (cool_ > 0) return;
    cool_ = 0.16f;
    flash_ = 4;
    sys_->apu.noiseBurst(0.18f, 1500.f, 0.035f);
    int best = -1;
    float bestZ = 1.0e9f;
    for (int i = 0; i < int(foes_.size()); i++) {
        if (!foes_[i].alive || foes_[i].lane != lane_) continue;
        if (foes_[i].z < bestZ) {
            bestZ = foes_[i].z;
            best = i;
        }
    }
    if (best < 0 || bestZ > 1.25f) return;
    Foe& f = foes_[best];
    f.hp -= 1;
    if (f.hp <= 0) kill(f);
    else sys_->apu.tone(2, 220.f, 0.04f);
}

void Game::beginWatch() {
    mode_ = Mode::Watch;
    over_ = false;
    won_ = false;
    bell_ = false;
    reason_ = "THE CAUSEWAY WAS TAKEN";
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
    scroll_ = 0;
    foes_.clear();
    script_.clear();
    for (const Arr& a : kArr) {
        Spawn s;
        s.t = a.t;
        s.kind = a.kind;
        s.lane = a.lane;
        script_.push_back(s);
    }
    if (sys_) sys_->setLight(30, 40, 70);
}

void Game::update(float dt) {
    watch_ += dt;
    scroll_ += dt * 90.f;
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
        sys_->apu.keyOn(1, 440.f, 0.22f);
        sys_->rumble(0.2f, 0.4f, 140);
    } else if (bell_ && !won_) {
        bellTick_ += dt;
        if (bellTick_ >= 0.85f) {
            bellTick_ = 0;
            sys_->apu.keyOn(1, 440.f, 0.18f);
        }
    }

    for (Foe& f : foes_) {
        if (!f.alive) continue;
        f.age++;
        f.z -= f.speed * dt;
        if (f.z <= 0.02f) {
            loseWatch("THE CAUSEWAY WAS TAKEN");
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
        if (best >= 0 && foes_[best].lane == lane_ && eta(foes_[best]) < 6.f) shoot();
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
    v.roadTime = int(watch_ * 60.f);
    float sway = std::sin(watch_ * 0.35f) * 14.f;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float sky = float(y) / float(HORIZON);
        if (y < HORIZON) {
            int r = 2 + int(sky * 6);
            int g = 2 + int(sky * 3);
            int b = 6 + int((1.f - sky) * 4);
            v.lineBackdrop[y] = gs::rgb4(r, g, b);
            v.lineFog[y] = uint8_t(8 - int(sky * 6));
            v.road[y].on = false;
            continue;
        }
        float depth = float(y - HORIZON) / float(gs::SCREEN_H - HORIZON);
        gs::RoadLine& rl = v.road[y];
        rl = {};
        rl.on = true;
        rl.cx = 160.f + sway * (1.f - depth);
        rl.hw = 8.f + depth * depth * 168.f;
        rl.v = 2800.f / (depth + 0.12f) - scroll_ * 4.f;
        rl.pal = 12;
        rl.band = (int(std::floor(rl.v / 90.f)) & 1) ? 1 : 0;
        rl.style = gs::ROAD_ROCKY;
        rl.left = gs::GROUND_WATER;
        rl.right = gs::GROUND_WATER;
        v.lineBackdrop[y] = gs::rgb4(1, 3, 6);
        v.lineFog[y] = uint8_t(std::clamp(int((1.f - depth) * 10.f), 0, 10));
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.hudEnabled = true;
    road();

    float swing = std::sin((modeT_ + watch_) * (bell_ ? 10.f : 2.2f)) * (bell_ ? 6.f : 1.4f);
    spr(art_.bell, 28.f + swing, 18.f, bell_ ? 28.f : 20.f, PAL_BELL, false, 0);

    // Stone posts and a boat on each side of the water, fixed in the view.
    for (int i = 0; i < 4; i++) {
        float near = 0.25f + float(i) * 0.22f;
        float h = 10.f + near * 22.f;
        float y = 88.f + near * 90.f - h;
        float spread = 28.f + near * 120.f;
        spr(art_.post, 160.f - spread, y, h, PAL_STONE, false, int((1.f - near) * 8));
        spr(art_.post, 160.f + spread, y, h, PAL_STONE, false, int((1.f - near) * 8));
    }
    float bob = std::sin(watch_ * 2.2f) * 2.f;
    spr(art_.boat, 46.f, 118.f + bob, 18.f, PAL_BOAT, false, 2);
    spr(art_.boat, 274.f, 132.f - bob, 22.f, PAL_BOAT, true, 1);

    if (mode_ == Mode::Title) {
        int fr = int(modeT_ * 6.f) & 1;
        spr(art_.runner[fr], 118, 96, 36, PAL_FOE, false, 2);
        spr(art_.truck, 200, 108, 28, PAL_TRUCK, false, 1);
        spr(art_.gunner[fr], 160, 150, 52, PAL_YOU, false, 0);
    } else {
        std::vector<int> order;
        for (int i = 0; i < int(foes_.size()); i++)
            if (foes_[i].alive) order.push_back(i);
        std::sort(order.begin(), order.end(), [&](int a, int b) { return foes_[a].z < foes_[b].z; });
        // Earlier sprites are on top, so draw the far ones first.
        for (int n = int(order.size()) - 1; n >= 0; n--) {
            const Foe& f = foes_[order[n]];
            float x, y, h;
            place(f, x, y, h);
            int fog = int(std::clamp(f.z * 8.f, 0.f, 12.f));
            if (f.kind == TRUCK) spr(art_.truck, x, y, h, PAL_TRUCK, f.lane > 0, fog);
            else spr(art_.runner[(f.age / 7) & 1], x, y, h, PAL_FOE, f.lane < 0, fog);
        }
        float px = 160.f + float(lane_) * 54.f;
        int fr = int(watch_ * 8.f) & 1;
        if (flash_ > 0) spr(art_.flash, px + 10.f, 168.f, 14.f, PAL_FX, false, 0);
        spr(art_.gunner[fr], px, 164.f, 56.f, PAL_YOU, lane_ < 0, 0);
    }

    char line[48];
    if (mode_ == Mode::Title) {
        hudC(23, "YOU HAVE THE CAUSEWAY.", PAL_HUD);
        hudC(24, "HOLD UNTIL THE RELIEF BELL.", PAL_OK);
        hudC(25, "LEFT RIGHT THE LANES    A FIRES", PAL_HUD);
        hudC(26, "ANYTHING ELSE IS A LOSS.", PAL_ALERT);
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
        const char* laneName = lane_ < 0 ? "WEST" : lane_ > 0 ? "EAST" : "CROWN";
        std::snprintf(line, sizeof line, "LANE %s", laneName);
        hud(1, 27, line, PAL_HUD);
        if (mode_ == Mode::Pause) hudC(25, "PAUSED", PAL_HUD);
        else if (mode_ == Mode::Victory) hudC(25, "THE WATCH HELD", PAL_OK);
        else if (mode_ == Mode::Over) hudC(25, reason_, PAL_ALERT);
        else if (bell_) hudC(25, "THE BELL. HOLD.", PAL_BELL);
        else hudC(25, "HOLD THE CAUSEWAY", PAL_HUD);
    }

    if (mode_ == Mode::Title) text("S3 CAUSEWAY", 160, 16, 0.7f, PAL_HUD);
    else if (mode_ == Mode::Victory) text("RELIEF", 160, 28, 1.1f, PAL_OK);
    else if (mode_ == Mode::Over) text("LOST", 160, 28, 1.1f, PAL_ALERT);
    else if (bell_) text("THE BELL", 160, 28, 0.8f, PAL_BELL);
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
            static const float notes[] = {392.f, 494.f, 587.f, 784.f};
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

}  // namespace causeway

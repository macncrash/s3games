#include "game/harbor.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace harbor {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float kBellAt = 36.f;
constexpr float kRing = 3.4f;
constexpr float kSteer = 1.55f;
constexpr float kCatch = 0.20f;

enum { Cutter = 0, Barge = 1, Launch = 2 };

struct Row {
    float arrive;
    int kind;
    float lat;
};

const Row kRows[] = {
    {5.2f, Cutter, -0.62f}, {8.0f, Barge, 0.55f},  {10.8f, Launch, -0.15f}, {13.6f, Cutter, 0.72f},
    {16.4f, Barge, -0.58f}, {19.2f, Launch, 0.28f}, {22.0f, Cutter, -0.78f}, {24.6f, Barge, 0.12f},
    {27.2f, Launch, 0.70f}, {29.8f, Cutter, -0.32f}, {32.4f, Barge, 0.48f}, {34.6f, Launch, -0.45f},
};

float speedOf(int kind) { return kind == Barge ? 0.115f : kind == Launch ? 0.175f : 0.255f; }
int ptsOf(int kind) { return kind == Barge ? 250 : kind == Launch ? 180 : 120; }
int palOf(int kind) { return kind == Barge ? PAL_BARGE : kind == Launch ? PAL_LAUNCH : PAL_CUTTER; }
float tallOf(int kind, float z) {
    float base = kind == Barge ? 22.f : kind == Launch ? 16.f : 18.f;
    return base * (0.35f + 0.85f * z);
}

uint16_t mix(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

gs::FMPatch dronePatch() {
    gs::FMPatch p;
    p.alg = 4;
    p.fb = 0.18f;
    p.op[0] = {1.f, 0.5f, 0.5f, 1.4f, 0.75f, 0.55f};
    p.op[1] = {0.5f, 0.28f, 0.4f, 1.0f, 0.55f, 0.4f};
    p.op[2] = {2.f, 0.1f, 0.2f, 0.5f, 0.25f, 0.25f};
    p.op[3] = {3.2f, 0.06f, 0.15f, 0.35f, 0.15f, 0.2f};
    p.vol = 0.06f;
    p.tone = 240.f;
    return p;
}

gs::FMPatch bellPatch() {
    gs::FMPatch p;
    p.alg = 7;
    p.fb = 0.08f;
    p.op[0] = {1.f, 1.f, 0.004f, 0.55f, 0.12f, 0.9f};
    p.op[1] = {2.76f, 0.4f, 0.005f, 0.4f, 0.06f, 0.7f};
    p.op[2] = {5.2f, 0.16f, 0.008f, 0.3f, 0.03f, 0.5f};
    p.op[3] = {8.1f, 0.07f, 0.01f, 0.22f, 0.02f, 0.4f};
    p.vol = 0.22f;
    p.echo = 0.42f;
    return p;
}

}  // namespace

int Game::marker() const {
    if (over_ || mode_ == Mode::Victory || mode_ == Mode::Over) return 3;
    if (bell_ && (mode_ == Mode::Watch || mode_ == Mode::Pause)) return 2;
    if (mode_ == Mode::Watch || mode_ == Mode::Pause) return 1;
    return 0;
}

void Game::project(float lat, float z, float& x, float& y) const {
    float zz = std::clamp(z, 0.f, 1.15f);
    y = kHorizon + zz * 128.f;
    float spread = 28.f + zz * 118.f;
    x = 160.f + lat * spread;
}

int Game::soonest() const {
    int best = -1;
    float z = -1.f;
    for (int i = 0; i < int(boats_.size()); i++) {
        const Boat& b = boats_[size_t(i)];
        if (!b.on || b.turned) continue;
        if (b.z > z) {
            z = b.z;
            best = i;
        }
    }
    return best;
}

void Game::beginWatch() {
    mode_ = Mode::Watch;
    over_ = false;
    won_ = false;
    bell_ = false;
    holding_ = false;
    reason_ = "THE WATCH RAN OUT";
    score_ = 0;
    turned_ = 0;
    piers_ = 3;
    lat_ = 0;
    watch_ = 0;
    modeT_ = 0;
    bellTick_ = shake_ = answerLeft_ = 0;
    spawnAt_ = 0;
    boats_.clear();
    splashes_.clear();
    script_.clear();
    for (const Row& r : kRows) {
        Spawn s;
        s.t = r.arrive - kBoomZ / speedOf(r.kind);
        s.kind = r.kind;
        s.lat = r.lat;
        script_.push_back(s);
    }
    std::sort(script_.begin(), script_.end(), [](const Spawn& a, const Spawn& b) { return a.t < b.t; });
    if (!sys_) return;
    sys_->apu.setPatch(0, dronePatch());
    sys_->apu.setPatch(1, bellPatch());
    sys_->apu.setPatch(2, bellPatch());
    sys_->apu.keyOn(0, 49.f, 0.05f);
    sys_->setLight(20, 60, 80);
}

void Game::stopBoat(Boat& b) {
    if (!b.on || b.turned) return;
    b.turned = true;
    b.on = false;
    score_ += b.points;
    turned_++;
    splashes_.push_back({b.lat, b.z, 0.45f});
    if (splashes_.size() > 8) splashes_.erase(splashes_.begin());
    sys_->apu.tone(1, 520.f, 0.05f);
    sys_->apu.noiseBurst(0.16f, 900.f, 0.06f);
}

void Game::enterHarbor() {
    piers_--;
    shake_ = 0.45f;
    sys_->apu.noiseBurst(0.28f, 180.f, 0.12f);
    sys_->rumble(0.5f, 0.2f, 90);
    if (piers_ <= 0) loseWatch("THE HARBOR WAS OPEN");
}

void Game::winWatch() {
    if (won_ || mode_ == Mode::Over) return;
    won_ = true;
    over_ = true;
    mode_ = Mode::Victory;
    reason_ = "THE HARBOR HELD UNTIL THE RELIEF BELL";
    score_ += 800 + piers_ * 200;
    sys_->apu.keyOn(2, 523.f, 0.2f);
    sys_->setLight(40, 140, 70);
}

void Game::loseWatch(const char* why) {
    if (won_ || mode_ == Mode::Over) return;
    reason_ = why;
    won_ = false;
    over_ = true;
    bell_ = false;
    mode_ = Mode::Over;
    shake_ = 0.7f;
    sys_->apu.keyOff(0);
    sys_->apu.noiseBurst(0.4f, 90.f, 0.28f);
    sys_->setLight(150, 30, 20);
}

void Game::steer(float& axis, bool& hold, bool& answer) {
    axis = 0;
    hold = false;
    answer = false;
    if (!bot_) {
        const gs::Pad& p = sys_->pad;
        if (p.down(gs::BTN_LEFT)) axis -= 1.f;
        if (p.down(gs::BTN_RIGHT)) axis += 1.f;
        if (std::fabs(p.axisX) > 0.2f) axis = p.axisX;
        hold = p.down(gs::BTN_A) || p.down(gs::BTN_C) || p.down(gs::BTN_Z) || p.accel > 0.4f;
        answer = p.pressed(gs::BTN_B) || (bell_ && p.pressed(gs::BTN_START));
        return;
    }
    int i = soonest();
    if (i >= 0) {
        const Boat& b = boats_[size_t(i)];
        float d = b.lat - lat_;
        axis = std::clamp(d * 4.f, -1.f, 1.f);
        float gap = kBoomZ - b.z;
        if (gap < 0.16f && gap > -0.03f && std::fabs(d) < kCatch) hold = true;
    }
    if (bell_ && watch_ > kBellAt + 0.45f) {
        bool hot = false;
        for (const Boat& b : boats_) {
            if (!b.on || b.turned) continue;
            if (b.z > kBoomZ - 0.12f && b.z < kBoomZ + 0.02f) hot = true;
        }
        if (!hot || watch_ > kBellAt + kRing - 0.35f) answer = true;
    }
}

void Game::update(float dt) {
    float axis = 0;
    bool hold = false, answer = false;
    steer(axis, hold, answer);
    holding_ = hold && mode_ == Mode::Watch;
    lat_ = std::clamp(lat_ + axis * kSteer * dt, -0.92f, 0.92f);

    while (spawnAt_ < int(script_.size()) && script_[size_t(spawnAt_)].t <= watch_) {
        const Spawn& s = script_[size_t(spawnAt_++)];
        Boat b;
        b.kind = s.kind;
        b.lat = s.lat;
        b.z = 0.02f;
        b.prev = b.z;
        b.points = ptsOf(s.kind);
        boats_.push_back(b);
    }

    for (Boat& b : boats_) {
        if (!b.on) continue;
        b.age += dt;
        b.prev = b.z;
        if (b.turned) {
            b.z -= speedOf(b.kind) * 0.4f * dt;
            if (b.z < 0) b.on = false;
            continue;
        }
        b.z += speedOf(b.kind) * dt;
        bool crossed = b.prev < kBoomZ && b.z >= kBoomZ;
        if (crossed && holding_ && std::fabs(b.lat - lat_) <= kCatch) stopBoat(b);
        else if (b.z >= kEnterZ) {
            b.on = false;
            splashes_.push_back({b.lat, 1.f, 0.35f});
            enterHarbor();
            if (mode_ != Mode::Watch) return;
        }
    }
    boats_.erase(std::remove_if(boats_.begin(), boats_.end(), [](const Boat& b) { return !b.on; }), boats_.end());
    for (Splash& s : splashes_) s.t -= dt;
    splashes_.erase(std::remove_if(splashes_.begin(), splashes_.end(), [](const Splash& s) { return s.t <= 0; }),
                    splashes_.end());

    if (!bell_ && watch_ >= kBellAt) {
        bell_ = true;
        answerLeft_ = kRing;
        bellTick_ = 0.15f;
        sys_->apu.keyOn(1, 392.f, 0.22f);
        sys_->setLight(180, 150, 60);
    }
    if (bell_) {
        answerLeft_ -= dt;
        bellTick_ -= dt;
        if (bellTick_ <= 0) {
            bellTick_ = 0.55f;
            sys_->apu.keyOn(1, 392.f, 0.2f);
        }
        if (answer) winWatch();
        else if (answerLeft_ <= 0) loseWatch("THE BELL WENT UNANSWERED");
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog) {
    if (h < 2.f || m.h < 1 || m.w < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 40 || s.y > gs::SCREEN_H + 40 || s.x + s.w < -40 || s.y + s.h < -40) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::shadow(float cx, float cy, float w) {
    if (w < 4.f) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 4L, 400L));
    s.h = int16_t(std::max(3L, std::lround(double(w) * 0.16)));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy));
    s.img = art_.shadow.pick(float(s.h));
    s.shadow = true;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    const float adv = 16.f * scale;
    float left = x - float(s.size()) * adv * 0.5f;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, left + float(i) * adv + adv * 0.5f, y, float(g.h) * scale, pal, false, 0);
    }
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || row < 0 || row > 27 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    float warm = std::clamp(watch_ / kBellAt, 0.f, 1.f);
    if (bell_ || mode_ == Mode::Victory) warm = 1.f;
    uint16_t skyTop = mix(gs::rgb4(1, 2, 5), gs::rgb4(4, 3, 6), warm * 0.4f);
    uint16_t skyHor = mix(gs::rgb4(3, 6, 8), gs::rgb4(14, 9, 4), warm);
    v.setFogColor(gs::rgb4(2, 4, 6));
    v.roadTime = int(modeT_ * 60.f);
    float shx = shake_ > 0 ? std::sin(modeT_ * 70.f) * 3.f * shake_ : 0.f;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float t = std::clamp((float(y) - 10.f) / 80.f, 0.f, 1.f);
        v.lineBackdrop[y] = mix(skyTop, skyHor, t);
        v.lineFog[y] = 0;
        gs::RoadLine& r = v.road[y];
        r = {};
        if (y >= int(kHorizon)) {
            r.on = true;
            r.style = 2;
            r.cx = 160.f + shx;
            r.hw = 800.f;
            r.v = modeT_ * 40.f + float(y) * 2.f;
            r.pal = PAL_WATER;
            r.band = ((y / 6) & 1) ? 1 : 0;
            r.left = gs::GROUND_WATER;
            r.right = gs::GROUND_WATER;
        }
    }

    if (mode_ == Mode::Title) text("S3 HARBOR RELIEF", 160, 28, 0.55f, PAL_AMBER);
    else if (bell_ && mode_ == Mode::Watch) text("RELIEF", 160, 22, 0.9f, PAL_AMBER);
    else if (mode_ == Mode::Victory) text("HELD", 160, 22, 1.0f, PAL_GREEN);
    else if (mode_ == Mode::Over) text("OPEN", 160, 22, 1.0f, PAL_RED);
    else if (mode_ == Mode::Pause) text("PAUSE", 160, 22, 0.9f, PAL_HUD);

    std::vector<int> order(boats_.size());
    for (int i = 0; i < int(boats_.size()); i++) order[size_t(i)] = i;
    std::sort(order.begin(), order.end(), [&](int a, int b) { return boats_[size_t(a)].z < boats_[size_t(b)].z; });
    for (int idx : order) {
        const Boat& b = boats_[size_t(idx)];
        float x, y;
        project(b.lat, b.z, x, y);
        int fog = int((1.f - std::clamp(b.z, 0.f, 1.f)) * 10.f);
        int fr = int(b.age * 8.f) & 1;
        const gs::Mipped& img = b.kind == Barge ? art_.barge : b.kind == Launch ? art_.launch[fr] : art_.cutter[fr];
        shadow(x, y + 4.f, tallOf(b.kind, b.z) * 1.4f);
        spr(img, x, y, tallOf(b.kind, b.z), palOf(b.kind), b.lat < 0, fog);
    }
    for (const Splash& s : splashes_) {
        float x, y;
        project(s.lat, s.z, x, y);
        spr(art_.splash, x, y, 14.f + (0.45f - s.t) * 20.f, PAL_FX, false, 0);
    }

    const float buoys[] = {-0.7f, -0.3f, 0.3f, 0.7f};
    for (float lat : buoys) {
        float x, y;
        project(lat, 0.22f, x, y);
        spr(art_.buoy, x, y, 10.f, PAL_RED, false, 6);
    }

    float px, py;
    float plat = (mode_ == Mode::Title) ? std::sin(modeT_ * 0.8f) * 0.45f : lat_;
    project(plat, kBoomZ, px, py);
    px += shx;
    if (holding_) spr(art_.chain, 160.f + shx, py, 10.f, PAL_BELL, false, 0);
    int fr = int(modeT_ * (std::fabs(lat_) > 0 ? 8.f : 3.f)) & 1;
    shadow(px, py + 6.f, 28.f);
    spr(art_.keeper[fr], px, py - 6.f, 28.f, PAL_KEEPER, plat < 0.f, 0);

    float swing = std::sin(modeT_ * (bell_ ? 9.f : 1.8f)) * (bell_ ? 5.f : 1.2f);
    spr(art_.bell, 28.f + swing, 36.f, bell_ ? 26.f : 20.f, PAL_BELL, false, 0);
    spr(art_.rope, 28.f + swing * 0.3f, 56.f, 24.f, PAL_WOOD, false, 0);
    spr(art_.light, 292.f, 70.f, 48.f, PAL_LIGHT, false, 0);
    float gx = 70.f + std::sin(modeT_ * 0.6f) * 40.f;
    spr(art_.gull, gx, 48.f + std::sin(modeT_ * 2.f) * 4.f, 8.f, PAL_NIGHT, false, 0);

    char buf[48];
    if (mode_ == Mode::Watch || mode_ == Mode::Pause) {
        if (bell_) std::snprintf(buf, sizeof buf, "BELL");
        else std::snprintf(buf, sizeof buf, "BELL %d", int(std::ceil(std::max(0.f, kBellAt - watch_))));
        hud(2, 1, buf, bell_ ? PAL_AMBER : PAL_HUD);
        std::snprintf(buf, sizeof buf, "%d", score_);
        hud(36, 1, buf, PAL_AMBER);
        std::snprintf(buf, sizeof buf, "PIERS %d", piers_);
        hud(2, 26, buf, piers_ < 2 ? PAL_RED : PAL_HUD);
        hud(28, 26, holding_ ? "CHAIN DOWN" : "CHAIN UP", holding_ ? PAL_GREEN : PAL_HUD);
    } else if (mode_ == Mode::Title) {
        hud(8, 24, "ARROWS STEER  A HOLDS THE CHAIN", PAL_HUD);
        hud(10, 25, "WHEN THE BELL RINGS  PRESS B", PAL_AMBER);
    } else if (mode_ == Mode::Victory) {
        hud(6, 25, "RELIEF ANSWERED", PAL_GREEN);
    } else if (mode_ == Mode::Over) {
        hud(4, 25, reason_, PAL_RED);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.85f);
    sys.apu.setEcho(0.18f, 0.35f, 0.2f);
    if (bot_) beginWatch();
    else {
        mode_ = Mode::Title;
        sys.setLight(30, 70, 90);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (bot_ && mode_ == Mode::Title) beginWatch();
    modeT_ += DT;
    if (shake_ > 0) shake_ = std::max(0.f, shake_ - DT);
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) beginWatch();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) sys.quit();
    } else if (mode_ == Mode::Watch) {
        bool pause = !bot_ && !bell_ && pad.pressed(gs::BTN_START);
        if (pause) mode_ = Mode::Pause;
        else {
            watch_ += DT;
            update(DT);
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Watch;
        else if (pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            bell_ = false;
            sys.apu.keyOff(0);
        }
    } else if (mode_ == Mode::Victory || mode_ == Mode::Over) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) beginWatch();
    }
    draw();
}

}  // namespace harbor

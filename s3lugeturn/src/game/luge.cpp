#include "game/luge.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace luge {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float kFinish = 1080.f;
constexpr float kHorizon = 78.f;
constexpr float kProj = 108.f;
constexpr float kRoad = 150.f;
constexpr float kTip = 1.02f;

struct Bend {
    float a;
    float b;
    float sign;
    const char* name;
};

const Bend kBends[] = {
    {160.f, 340.f, -1.f, "LEFT BANK"},
    {460.f, 660.f, 1.f, "RIGHT BANK"},
    {780.f, 980.f, -1.f, "LAST BANK"},
};

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (held_ >= 2 || s_ >= kBends[2].a) return 3;
    if (held_ >= 1 || s_ >= kBends[1].a) return 2;
    if (s_ >= kBends[0].a - 20.f) return 1;
    return 1;
}

float Game::demandAt(float worldS) const {
    float d = 0.f;
    for (const Bend& b : kBends) {
        if (worldS < b.a || worldS > b.b) continue;
        float u = (worldS - b.a) / (b.b - b.a);
        d += b.sign * std::sin(u * 3.1415926f);
    }
    return d;
}

void Game::showTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    s_ = 0;
    speed_ = 0;
    time_ = 0;
    steer_ = 0;
    lean_ = 0;
    shake_ = 0;
    held_ = 0;
    chime_ = -1;
    why_[0] = 0;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.HUD.enabled = true;
    sys.apu.setMaster(0.7f);
    showTitle();
}

void Game::startRun() {
    s_ = 0;
    speed_ = 18.f;
    time_ = 0;
    steer_ = 0;
    lean_ = 0;
    shake_ = 0;
    held_ = 0;
    over_ = false;
    won_ = false;
    chime_ = -1;
    why_[0] = 0;
    mode_ = Mode::Run;
    blip(392.f);
}

void Game::blip(float freq) {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.06f;
    p.op[0] = {1.f, 1.f, 0.01f, 0.14f, 0.35f, 0.2f};
    p.op[1] = {2.f, 0.18f, 0.01f, 0.18f, 0.15f, 0.16f};
    p.op[2] = {3.f, 0.04f, 0.01f, 0.2f, 0.08f, 0.2f};
    p.op[3] = {1.f, 0.f, 0.01f, 0.2f, 0.08f, 0.2f};
    p.vol = 0.22f;
    p.tone = 1800.f;
    sys_->apu.setPatch(0, p);
    sys_->apu.keyOn(0, freq, 0.24f);
}

void Game::win() {
    if (mode_ != Mode::Run) return;
    won_ = true;
    over_ = true;
    mode_ = Mode::Win;
    s_ = std::max(s_, kFinish);
    chime_ = 0;
    chimeT_ = 0;
    sys_->rumble(0.12f, 0.04f, 120);
    sys_->setLight(40, 150, 120);
    std::printf("S3 LUGE TURN  CLEAR  three turns held  no tip  (%.1f s)\n", time_);
    std::fflush(stdout);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    won_ = false;
    over_ = true;
    mode_ = Mode::Fail;
    std::snprintf(why_, sizeof why_, "%s", why);
    shake_ = 0.55f;
    sys_->rumble(0.5f, 0.25f, 180);
    sys_->setLight(180, 30, 30);
    sys_->apu.noiseBurst(0.45f, 160.f, 0.32f);
    std::printf("S3 LUGE TURN  FAIL  %s  turns %d/3\n", why, held_);
    std::fflush(stdout);
}

void Game::pilot(float& steer) const {
    float look = demandAt(s_ + 22.f + speed_ * 0.35f);
    float harsh = 0.42f + speed_ * 0.018f;
    steer = std::clamp(look * harsh / 0.92f, -1.f, 1.f);
}

void Game::physics(float steer) {
    steer = std::clamp(steer, -1.f, 1.f);
    time_ += DT;
    float want = 22.f;
    speed_ += (want - speed_) * 0.8f * DT;
    speed_ = std::clamp(speed_, 12.f, 30.f);
    s_ += speed_ * DT;
    steer_ = steer;

    float ideal = demandAt(s_) * (0.42f + speed_ * 0.018f);
    float err = ideal - steer * 0.92f;
    lean_ += (err - lean_) * 0.22f;
    if (std::fabs(lean_) > kTip) {
        fail(lean_ > 0.f ? "tipped to the right" : "tipped to the left");
        return;
    }

    int cleared = 0;
    for (const Bend& b : kBends)
        if (s_ > b.b - 8.f) cleared++;
    if (cleared > held_) {
        held_ = cleared;
        blip(520.f + float(held_) * 90.f);
        if (held_ >= 3) {
            // still have to stay upright off the last bank
        }
    }
    if (held_ >= 3 && s_ >= kFinish) win();
}

void Game::audio() {
    float vol = (mode_ == Mode::Run) ? 0.035f + speed_ * 0.003f : 0.f;
    float scrape = 90.f + speed_ * 4.f + std::fabs(lean_) * 40.f;
    sys_->apu.tone(1, scrape, vol);
    if (chime_ < 0) return;
    chimeT_ -= DT;
    if (chimeT_ > 0) return;
    static const float notes[] = {523.f, 659.f, 784.f, 1046.f};
    if (chime_ < 4) {
        blip(notes[chime_]);
        chime_++;
        chimeT_ = 0.11f;
    }
}

bool Game::project(float lane, float z, float& sx, float& sy, float& scale, int& fog) const {
    if (z < 0.6f || z > 140.f) return false;
    float row = kProj / z;
    sy = kHorizon + row;
    if (sy < 6.f || sy > gs::SCREEN_H + 24.f) return false;
    float hw = kRoad / z;
    float bend = demandAt(s_ + z) - demandAt(s_);
    sx = 160.f + (bend * 0.85f + lane) * hw;
    scale = hw;
    fog = std::clamp(int((z - 16.f) / 9.f), 0, 12);
    return true;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, int fog) {
    if (h < 1.f || m.h < 1) return;
    gs::Sprite s;
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 400));
    s.w = int16_t(std::max(1, int(std::lround(h * (float(m.w) / float(m.h))))));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::drawRoad() {
    gs::VDP& v = sys_->vdp;
    const uint16_t top = gs::rgb4(3, 4, 9);
    const uint16_t mid = gs::rgb4(7, 9, 13);
    const uint16_t hor = gs::rgb4(12, 14, 15);
    float jx = 0;
    if (shake_ > 0) {
        jx = std::sin(time_ * 80.f) * shake_ * 8.f;
        shake_ = std::max(0.f, shake_ - DT);
    }
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (float(y) < kHorizon) {
            float u = float(y) / kHorizon;
            v.lineBackdrop[y] = u < 0.5f ? lerpC(top, mid, u / 0.5f) : lerpC(mid, hor, (u - 0.5f) / 0.5f);
            v.lineFog[y] = 0;
            v.road[y].on = false;
            continue;
        }
        float row = std::max(0.8f, float(y) - kHorizon);
        float z = kProj / row;
        gs::RoadLine& r = v.road[y];
        r.on = true;
        r.hw = kRoad / z;
        float bend = demandAt(s_ + z) - demandAt(s_);
        r.cx = 160.f + jx + bend * 0.9f * r.hw;
        r.v = (s_ + z) * 3.5f;
        r.pal = PAL_ROAD;
        r.style = gs::ROAD_ICE;
        r.band = (int(std::floor((s_ + z) / 16.f)) & 1) ? 1 : 0;
        r.left = r.right = gs::GROUND_SNOWWALL;
        v.lineFog[y] = uint8_t(std::clamp(int((z - 24.f) / 10.f), 0, 10));
        v.lineBackdrop[y] = hor;
    }
    v.roadTime = int(s_ * 2.f);
}

void Game::drawWorld() {
    struct Item {
        float z;
        int kind;
        float lane;
    };
    Item items[24];
    int n = 0;
    auto push = [&](float z, int kind, float lane) {
        if (n < 24 && z > 0.8f && z < 130.f) items[n++] = {z, kind, lane};
    };
    for (int i = 0; i < 14; i++) {
        float ps = 40.f + float(i) * 78.f;
        float side = (i & 1) ? 1.15f : -1.15f;
        push(ps - s_, 0, side);
    }
    for (const Bend& b : kBends) {
        push(b.a - s_, 1, b.sign * -0.72f);
        push(b.b - s_, 1, b.sign * 0.72f);
    }
    std::sort(items, items + n, [](const Item& a, const Item& c) { return a.z > c.z; });
    for (int i = 0; i < n; i++) {
        float sx, sy, sc;
        int fog;
        if (!project(items[i].lane, items[i].z, sx, sy, sc, fog)) continue;
        if (items[i].kind == 0) spr(art_.pine, sx, sy, std::max(10.f, sc * 0.55f), PAL_PINE, fog);
        else spr(art_.gate, sx, sy, std::max(12.f, sc * 0.7f), PAL_GATE, fog);
    }
}

void Game::drawPod() {
    if (mode_ == Mode::Title) {
        spr(art_.title, 160.f, 70.f, 28.f, PAL_HUD, 0);
        spr(art_.pod[1], 160.f, 150.f, 52.f, PAL_POD, 0);
        return;
    }
    int fr = lean_ > 0.22f ? 2 : lean_ < -0.22f ? 0 : 1;
    float sx = 160.f + lean_ * 36.f;
    float sy = 198.f + std::sin(s_ * 0.45f) * 1.2f;
    if (mode_ == Mode::Run && std::fabs(lean_) > 0.15f) {
        spr(art_.spray, sx - lean_ * 20.f, sy - 4.f, 10.f + std::fabs(lean_) * 8.f, PAL_FX, 0);
    }
    float roll = 1.f + std::fabs(lean_) * 0.08f;
    spr(art_.pod[fr], sx, sy, 64.f * roll, PAL_POD, 0);
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

void Game::drawHud() {
    sys_->vdp.HUD.clear();
    char buf[64];
    if (mode_ == Mode::Title) {
        hudC(16, "THREE TURNS  DO NOT TIP", PAL_ICE);
        hudC(18, "STEER INTO THE BANK", PAL_HUD);
        hudC(24, "ARROWS STEER    ENTER DROPS", PAL_HUD);
        return;
    }
    const char* name = "STRAIGHT";
    for (const Bend& b : kBends)
        if (s_ >= b.a && s_ <= b.b) name = b.name;
    std::snprintf(buf, sizeof buf, "TURNS %d/3", held_);
    hud(1, 0, buf, PAL_HUD);
    hud(14, 0, name, PAL_GATE);
    int bar = int(std::clamp(std::fabs(lean_) / kTip, 0.f, 1.f) * 10.f);
    std::snprintf(buf, sizeof buf, "LEAN %.*s", bar, "||||||||||");
    hud(1, 26, buf, bar > 7 ? PAL_POD : PAL_ICE);
    std::snprintf(buf, sizeof buf, "%d M", int(s_));
    hud(30, 26, buf, PAL_HUD);
    if (mode_ == Mode::Win) {
        hudC(12, "THREE TURNS HELD", PAL_HUD);
        hudC(14, "NO TIP", PAL_ICE);
        hudC(24, "ENTER", PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        hudC(12, "TIPPED", PAL_POD);
        hudC(14, why_, PAL_HUD);
        hudC(24, "ENTER", PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        hudC(13, "HELD", PAL_HUD);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    sys.vdp.clearSprites();
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (bot_ || pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) startRun();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Run;
    } else if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) showTitle();
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        float steer = 0.f;
        if (bot_) pilot(steer);
        else {
            if (pad.down(gs::BTN_LEFT)) steer -= 1.f;
            if (pad.down(gs::BTN_RIGHT)) steer += 1.f;
            steer += pad.axisX;
            steer = std::clamp(steer, -1.f, 1.f);
        }
        if (mode_ == Mode::Run) physics(steer);
    }
    audio();
    drawRoad();
    drawWorld();
    drawPod();
    drawHud();
}

}  // namespace luge

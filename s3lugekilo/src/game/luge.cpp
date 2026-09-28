#include "game/luge.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace luge {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float kFinish = 1000.f;
constexpr float kCrew = 46.5f;
constexpr float kHorizon = 86.f;
constexpr float kProj = 118.f;
constexpr float kRoad = 168.f;
constexpr float kBody = 0.16f;

struct Wheel {
    float s;
    float lane;
    float half;
    int kind;
};

const Wheel kWheels[] = {
    {90.f, -0.62f, 0.20f, 0},  {175.f, 0.58f, 0.22f, 1}, {265.f, -0.08f, 0.20f, 2},
    {360.f, 0.66f, 0.20f, 0},  {455.f, -0.58f, 0.24f, 1}, {545.f, 0.22f, 0.20f, 2},
    {640.f, -0.70f, 0.20f, 0}, {735.f, 0.48f, 0.22f, 1}, {830.f, -0.28f, 0.22f, 2},
    {925.f, 0.62f, 0.20f, 0},
};
constexpr int kWheelN = int(sizeof kWheels / sizeof kWheels[0]);

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

const char* wheelWord(int kind) {
    if (kind == 0) return "a clock wheel";
    if (kind == 1) return "a cart wheel";
    return "a sheave wheel";
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (s_ >= 760.f) return 3;
    for (const Wheel& w : kWheels) {
        float a = w.s - s_;
        if (a > 4.f && a < 48.f) return 2;
    }
    return 1;
}

float Game::bendAt(float worldS) const { return std::sin(worldS * 0.011f) * 0.42f + std::sin(worldS * 0.004f) * 0.18f; }

void Game::showTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    s_ = 0;
    x_ = 0;
    speed_ = 0;
    time_ = 0;
    steer_ = 0;
    lean_ = 0;
    shake_ = 0;
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
    x_ = 0;
    speed_ = 20.f;
    time_ = 0;
    steer_ = 0;
    lean_ = 0;
    shake_ = 0;
    over_ = false;
    won_ = false;
    chime_ = -1;
    why_[0] = 0;
    puffN_ = 0;
    mode_ = Mode::Run;
    blip(440.f);
}

void Game::blip(float freq) {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.08f;
    p.op[0] = {1.f, 1.f, 0.01f, 0.12f, 0.4f, 0.18f};
    p.op[1] = {2.f, 0.2f, 0.01f, 0.16f, 0.2f, 0.16f};
    p.op[2] = {3.f, 0.05f, 0.01f, 0.2f, 0.1f, 0.2f};
    p.op[3] = {1.f, 0.f, 0.01f, 0.2f, 0.1f, 0.2f};
    p.vol = 0.2f;
    p.tone = 1600.f;
    sys_->apu.setPatch(0, p);
    sys_->apu.keyOn(0, freq, 0.22f);
}

void Game::win() {
    if (mode_ != Mode::Run) return;
    won_ = true;
    over_ = true;
    mode_ = Mode::Win;
    s_ = std::max(s_, kFinish);
    chime_ = 0;
    chimeT_ = 0;
    sys_->rumble(0.15f, 0.05f, 140);
    sys_->setLight(40, 160, 110);
    std::printf("S3 LUGE KILO  CLEAR  finished the kilometer  1000 m  wheels untouched  beat the other crew  (%.1f s)\n",
                time_);
    std::fflush(stdout);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    won_ = false;
    over_ = true;
    mode_ = Mode::Fail;
    std::snprintf(why_, sizeof why_, "%s", why);
    shake_ = 0.45f;
    sys_->rumble(0.45f, 0.2f, 160);
    sys_->setLight(170, 40, 30);
    sys_->apu.noiseBurst(0.42f, 180.f, 0.3f);
    std::printf("S3 LUGE KILO  FAIL  %s at %d m\n", why, int(std::clamp(s_, 0.f, kFinish)));
    std::fflush(stdout);
}

void Game::pilot(float& steer, float& tuck) const {
    float target = 0.f;
    float nearest = 1e9f;
    bool threat = false;
    for (const Wheel& w : kWheels) {
        float ahead = w.s - s_;
        if (ahead < -1.5f || ahead > 62.f) continue;
        float gap = w.half + kBody + 0.12f;
        if (std::fabs(x_ - w.lane) < gap + 0.22f || ahead < 28.f) {
            float away = (x_ >= w.lane) ? 1.f : -1.f;
            if (std::fabs(x_ - w.lane) < 0.04f) away = (w.lane >= 0.f) ? -1.f : 1.f;
            float want = std::clamp(w.lane + away * (w.half + kBody + 0.34f), -0.72f, 0.72f);
            if (ahead < nearest) {
                nearest = ahead;
                target = want;
                threat = true;
            }
        }
    }
    if (!threat) target = x_ * 0.35f;
    steer = std::clamp((target - x_) * 3.4f, -1.f, 1.f);
    tuck = 1.f;
}

void Game::physics(float steer, float tuck) {
    steer = std::clamp(steer, -1.f, 1.f);
    tuck = std::clamp(tuck, 0.f, 1.f);
    time_ += DT;
    float rate = 1.35f + speed_ * 0.012f;
    x_ += steer * rate * DT;
    if (x_ > 0.90f) {
        x_ = 0.90f;
        speed_ -= 6.f * DT;
    }
    if (x_ < -0.90f) {
        x_ = -0.90f;
        speed_ -= 6.f * DT;
    }
    float want = 22.f + tuck * 12.f;
    speed_ += (want - speed_) * 0.55f * DT;
    speed_ = std::clamp(speed_, 8.f, 36.f);
    s_ += speed_ * DT;
    steer_ = steer;
    lean_ += (steer - lean_) * 0.18f;

    for (const Wheel& w : kWheels) {
        if (std::fabs(s_ - w.s) > 1.85f) continue;
        if (std::fabs(x_ - w.lane) < w.half + kBody) {
            char buf[64];
            std::snprintf(buf, sizeof buf, "touched %s", wheelWord(w.kind));
            fail(buf);
            return;
        }
    }
    if (time_ > kCrew && s_ < kFinish) {
        fail("the other crew's clock ran out");
        return;
    }
    if (s_ >= kFinish) win();
}

void Game::audio() {
    wind_ += DT;
    float vol = (mode_ == Mode::Run) ? 0.04f + speed_ * 0.004f : 0.f;
    sys_->apu.tone(0, 70.f + speed_ * 3.f, vol);
    if (chime_ < 0) return;
    chimeT_ -= DT;
    if (chimeT_ > 0) return;
    static const float notes[] = {523.f, 659.f, 784.f, 1046.f};
    if (chime_ < 4) {
        blip(notes[chime_]);
        chime_++;
        chimeT_ = 0.12f;
    }
}

bool Game::project(float lane, float z, float& sx, float& sy, float& scale, int& fog) const {
    if (z < 0.55f || z > 130.f) return false;
    float row = kProj / z;
    sy = kHorizon + row;
    if (sy < 8.f || sy > gs::SCREEN_H + 20.f) return false;
    float hw = kRoad / z;
    float bend = bendAt(s_ + z) - bendAt(s_);
    sx = 160.f + (bend + lane - x_) * hw;
    scale = hw;
    fog = std::clamp(int((z - 18.f) / 8.f), 0, 12);
    return true;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow, int fog) {
    if (h < 1.f) return;
    gs::Sprite s;
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 400));
    s.w = int16_t(std::max(1, int(std::lround(h * (m.w / std::max(1.f, float(m.h)))))));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::drawRoad() {
    gs::VDP& v = sys_->vdp;
    const uint16_t top = gs::rgb4(2, 3, 8);
    const uint16_t mid = gs::rgb4(6, 8, 12);
    const uint16_t hor = gs::rgb4(12, 13, 14);
    float jx = 0, jy = 0;
    if (shake_ > 0) {
        jx = std::sin(time_ * 90.f) * shake_ * 6.f;
        jy = std::cos(time_ * 70.f) * shake_ * 3.f;
        shake_ = std::max(0.f, shake_ - DT);
    }
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float yy = float(y) - jy;
        if (yy < kHorizon) {
            float u = yy / kHorizon;
            v.lineBackdrop[y] = u < 0.55f ? lerpC(top, mid, u / 0.55f) : lerpC(mid, hor, (u - 0.55f) / 0.45f);
            v.lineFog[y] = 0;
            v.road[y].on = false;
            continue;
        }
        float row = std::max(0.8f, yy - kHorizon);
        float z = kProj / row;
        gs::RoadLine& r = v.road[y];
        r.on = true;
        r.hw = kRoad / z;
        r.cx = 160.f + jx + (bendAt(s_ + z) - bendAt(s_) - x_) * r.hw;
        r.v = (s_ + z) * 4.f;
        r.pal = PAL_ROAD;
        r.style = gs::ROAD_ICE;
        r.band = (int(std::floor((s_ + z) / 18.f)) & 1) ? 1 : 0;
        r.left = r.right = gs::GROUND_SNOWWALL;
        v.lineFog[y] = uint8_t(std::clamp(int((z - 22.f) / 9.f), 0, 10));
        v.lineBackdrop[y] = hor;
    }
    v.roadTime = int(s_ * 3.f);
}

void Game::drawWorld() {
    struct Item {
        float z;
        int kind;
        int index;
    };
    Item items[16];
    int n = 0;
    float ghostS = time_ * (kFinish / kCrew);
    float gz = ghostS - s_;
    if (mode_ != Mode::Title && gz > 0.8f && gz < 120.f && n < 16) items[n++] = {gz, 1, 0};
    for (int i = 0; i < kWheelN; i++) {
        float z = kWheels[i].s - s_;
        if (z > 0.7f && z < 120.f && n < 16) items[n++] = {z, 0, i};
    }
    std::sort(items, items + n, [](const Item& a, const Item& b) { return a.z > b.z; });
    for (int i = 0; i < n; i++) {
        const Item& it = items[i];
        if (it.kind == 1) {
            float sx, sy, sc;
            int fog;
            if (!project(0.f, it.z, sx, sy, sc, fog)) continue;
            spr(art_.ghost, sx, sy, sc * 0.55f, PAL_ICE, false, fog + 2);
            continue;
        }
        const Wheel& w = kWheels[it.index];
        float sx, sy, sc;
        int fog;
        if (!project(w.lane, it.z, sx, sy, sc, fog)) continue;
        float h = sc * (w.half * 2.3f);
        const gs::Mipped& img = w.kind == 0 ? art_.clock : w.kind == 2 ? art_.sheave : art_.wheel;
        int pal = w.kind == 0 ? PAL_CLOCK : w.kind == 2 ? PAL_SHEAVE : PAL_CART;
        spr(img, sx, sy + h * 0.15f, std::max(8.f, h), pal, false, fog);
    }
}

void Game::drawLuge() {
    if (mode_ == Mode::Title) return;
    int fr = lean_ > 0.28f ? 2 : lean_ < -0.28f ? 0 : 1;
    float bob = std::sin(s_ * 0.7f) * 1.4f;
    float sx = 160.f + lean_ * 18.f;
    float sy = 214.f + bob;
    if (speed_ > 16.f && mode_ == Mode::Run && (int(s_) & 3) == 0) {
        Puff& p = puffs_[puffN_ % 12];
        p.x = sx + (puffN_ & 1 ? -16.f : 16.f);
        p.y = sy - 6.f;
        p.t = 0.35f;
        puffN_++;
    }
    for (Puff& p : puffs_) {
        if (p.t <= 0) continue;
        p.t -= DT;
        p.y -= 10.f * DT;
        spr(art_.puff, p.x, p.y, 8.f + (0.35f - p.t) * 18.f, PAL_FX, false, 0);
    }
    spr(art_.luge[fr], sx, sy, 86.f, PAL_LUGE, false, 0);
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

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    float adv = 18.f * scale;
    x -= float(s.size()) * adv * 0.5f;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + float(i) * adv + g.w * scale * 0.5f, y + g.h * scale, g.h * scale, pal, false, 0);
    }
}

void Game::drawHud() {
    sys_->vdp.HUD.clear();
    char buf[64];
    if (mode_ == Mode::Title) {
        text("LUGE KILO", 160, 48, 1.05f, PAL_HUD);
        text("FINISH THE KILOMETER", 160, 78, 0.48f, PAL_CLOCK);
        text("DO NOT TOUCH A WHEEL", 160, 98, 0.42f, PAL_LUGE);
        text("THE CLOCK IS THE OTHER CREW", 160, 116, 0.38f, PAL_ICE);
        hudC(22, "ARROWS STEER   UP TUCKS", PAL_HUD);
        hudC(24, "ENTER TO DROP", PAL_HUD);
        return;
    }
    int meters = int(std::clamp(s_, 0.f, kFinish));
    std::snprintf(buf, sizeof buf, "%d/1000 M", meters);
    hud(1, 0, buf, PAL_HUD);
    std::snprintf(buf, sizeof buf, "YOU %.1f", time_);
    hud(16, 0, buf, PAL_HUD);
    std::snprintf(buf, sizeof buf, "CREW %.1f", kCrew);
    hud(29, 0, buf, PAL_CLOCK);
    if (mode_ == Mode::Win) {
        hudC(12, "KILOMETER CLEAR", PAL_HUD);
        hudC(14, "WHEELS UNTOUCHED", PAL_ICE);
        std::snprintf(buf, sizeof buf, "BEAT THE CREW BY %.1f S", kCrew - time_);
        hudC(16, buf, PAL_CLOCK);
        hudC(24, "ENTER", PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        hudC(12, "RUN OVER", PAL_LUGE);
        hudC(14, why_, PAL_HUD);
        hudC(24, "ENTER", PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        hudC(13, "HELD", PAL_HUD);
    } else {
        float left = kCrew - time_;
        std::snprintf(buf, sizeof buf, "CLOCK %.1f", std::max(0.f, left));
        hud(1, 26, buf, left < 8.f ? PAL_LUGE : PAL_ICE);
        hud(28, 26, "WHEELS CLEAR", PAL_HUD);
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
        float steer = 0, tuck = 0.35f;
        if (bot_) {
            pilot(steer, tuck);
        } else {
            if (pad.down(gs::BTN_LEFT)) steer -= 1.f;
            if (pad.down(gs::BTN_RIGHT)) steer += 1.f;
            steer += pad.axisX;
            if (pad.down(gs::BTN_UP) || pad.down(gs::BTN_A)) tuck = 1.f;
            if (pad.down(gs::BTN_DOWN)) tuck = 0.f;
            steer = std::clamp(steer, -1.f, 1.f);
        }
        if (mode_ == Mode::Run) physics(steer, tuck);
    }
    audio();
    drawRoad();
    drawWorld();
    drawLuge();
    drawHud();
}

}  // namespace luge

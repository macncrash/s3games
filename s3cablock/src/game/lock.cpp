#include "game/lock.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace cablock {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float kG0 = 34.f;
constexpr float kG1 = 72.f;
constexpr float kFinish = 82.f;
constexpr float kChamber0 = 28.f;
constexpr float kChamber1 = 78.f;
constexpr float kRear = 1.15f;
constexpr float kNose = 2.55f;
constexpr float kHalf = 0.46f;
constexpr float kNeed = 0.64f;
constexpr float kRoad = 2.55f;
constexpr float kHorizon = 78.f;
constexpr float kPpm = 46.f;
constexpr float kNear = 0.85f;
constexpr float kRival0 = -6.f;
constexpr float kRivalV = 5.55f;
constexpr float kOmega = 0.82f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = clampf(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

const float kGate[2] = {kG0, kG1};
const float kPhase[2] = {0.55f, 2.85f};

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (s_ > kG1) return 3;
    if (s_ > kChamber0) return 2;
    return 1;
}

float Game::gateGap(int i, float t) const {
    float s = std::sin(t * kOmega + kPhase[i]);
    return 0.22f + 1.85f * std::max(0.f, s);
}

int Game::nextGate() const {
    for (int i = 0; i < 2; i++)
        if (s_ + kNose < kGate[i] + 0.35f) return i;
    return -1;
}

bool Game::window(int i, float t) const {
    for (float dt = 0.f; dt <= 0.7f; dt += 0.1f)
        if (gateGap(i, t + dt) < kNeed) return false;
    return true;
}

void Game::showTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    why_ = "";
    dart_ = false;
    chime_ = -1;
    time_ = 0;
    s_ = 12.f;
    y_ = 0.f;
    v_ = 0;
    rival_ = 8.f;
}

void Game::startRun() {
    s_ = 0;
    y_ = 0.15f;
    v_ = 0;
    rival_ = kRival0;
    time_ = 0;
    gas_ = brake_ = steer_ = 0;
    won_ = false;
    over_ = false;
    dart_ = false;
    why_ = "";
    chime_ = -1;
    mode_ = Mode::Run;
    blip(480.f);
}

void Game::pilot(float& gas, float& brake, float& steer) {
    steer = clampf(-y_ * 4.2f, -1.f, 1.f);
    int g = nextGate();
    if (g < 0) {
        gas = 1.f;
        brake = 0;
        dart_ = false;
        return;
    }
    float dist = kGate[g] - (s_ + kNose);
    float arrive = time_ + std::max(0.f, dist) / std::max(v_, 3.2f);
    bool open = window(g, time_) && dist < 9.f;
    bool soon = window(g, arrive) && dist < 14.f;
    if (dart_) {
        if (dist < -0.6f) dart_ = false;
        else if (dist > 1.2f && !window(g, time_ + 0.35f)) dart_ = false;
    } else if (open || (soon && dist < 4.5f && v_ > 2.f)) {
        dart_ = true;
    }
    if (dart_) {
        gas = 1.f;
        brake = 0;
        return;
    }
    float hold = 1.35f;
    if (dist > hold + 0.4f) {
        float vWant = clampf(dist - hold, 2.f, 9.f);
        gas = v_ < vWant ? 1.f : 0.f;
        brake = v_ > vWant + 0.4f ? 1.f : 0.f;
    } else {
        gas = 0;
        brake = v_ > 0.25f ? 1.f : 0.f;
        if (v_ < 0.2f && dist > hold + 0.15f) gas = 0.35f;
    }
}

void Game::win() {
    if (mode_ != Mode::Run) return;
    won_ = true;
    over_ = true;
    mode_ = Mode::Win;
    why_ = "clear of both gates";
    v_ = 0;
    chime_ = 0;
    chimeT_ = 0;
    sys_->rumble(0.16f, 0.05f, 90);
    sys_->setLight(40, 180, 70);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    won_ = false;
    over_ = true;
    mode_ = Mode::Fail;
    why_ = why;
    sys_->rumble(0.55f, 0.3f, 160);
    sys_->setLight(180, 30, 20);
    sys_->apu.noiseBurst(0.45f, 520.f, 0.28f);
}

void Game::physics(float gas, float brake, float steer) {
    gas_ = clampf(gas, 0.f, 1.f);
    brake_ = clampf(brake, 0.f, 1.f);
    steer_ = clampf(steer, -1.f, 1.f);
    time_ += DT;
    float a = gas_ * 7.4f - brake_ * 13.5f - 0.38f * v_;
    v_ = std::max(0.f, v_ + a * DT);
    if (v_ > 13.5f) v_ = 13.5f;
    float prev = s_;
    s_ += v_ * DT;
    float yaw = steer_ * (0.55f + 0.06f * v_);
    y_ = clampf(y_ + yaw * DT, -1.55f, 1.55f);
    shake_ = std::max(0.f, shake_ - DT);
    if (brake_ > 0.7f && v_ > 3.f) shake_ = 0.07f;

    rival_ += kRivalV * DT;
    if (rival_ >= kFinish) {
        fail("the other crew");
        return;
    }

    for (int i = 0; i < 2; i++) {
        float g = kGate[i];
        bool was = (prev - kRear) < g && (prev + kNose) > g - 0.05f;
        bool now = (s_ - kRear) < g && (s_ + kNose) > g;
        if ((was || now) && std::fabs(y_) + kHalf > gateGap(i, time_) - 0.02f) {
            fail("scraped a gate");
            return;
        }
    }

    if (s_ - kRear > kG1 + 0.4f) win();
    else if (time_ > 26.f) fail("the other crew");
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.07f);
    beep_ = 0.08f;
}

bool Game::project(float wz, float wy, float& sx, float& sy, float& ppm) const {
    float dz = wz - s_;
    if (dz < 0.4f || dz > 52.f) return false;
    float n = kNear / dz;
    sy = kHorizon + n * (gs::SCREEN_H - kHorizon);
    ppm = kPpm * n;
    sx = 160.f + (wy - y_) * ppm;
    return sy > -30.f && sy < gs::SCREEN_H + 30.f;
}

void Game::spr(const gs::Mipped& m, float cx, float feet, float ht, int pal, bool flip, int fog) {
    if (ht < 1.4f || m.h < 1) return;
    float w = ht * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::lround(clampf(w, 1.f, 420.f)));
    s.h = int16_t(std::lround(clampf(ht, 1.f, 320.f)));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet - s.h));
    s.img = m.pick(ht);
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::skyRoad() {
    uint16_t zen = gs::rgb4(2, 4, 8);
    uint16_t mid = gs::rgb4(6, 8, 11);
    uint16_t hor = gs::rgb4(11, 10, 8);
    if (mode_ == Mode::Fail) hor = lerpC(hor, gs::rgb4(12, 4, 3), 0.45f);
    if (mode_ == Mode::Win) hor = lerpC(hor, gs::rgb4(8, 13, 8), 0.4f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float t = y / float(gs::SCREEN_H - 1);
        sys_->vdp.lineBackdrop[y] = t < 0.34f ? lerpC(zen, mid, t / 0.34f) : lerpC(mid, hor, (t - 0.34f) / 0.66f);
        sys_->vdp.lineFog[y] = 0;
        gs::RoadLine& r = sys_->vdp.road[y];
        r.on = false;
        if (y <= int(kHorizon)) continue;
        float n = (y - kHorizon) / (gs::SCREEN_H - kHorizon);
        if (n < 0.012f) continue;
        float dz = kNear / n;
        float world = s_ + dz;
        bool lock = world >= kChamber0 && world <= kChamber1;
        r.on = true;
        r.cx = 160.f - y_ * kPpm * n;
        r.hw = kRoad * kPpm * n;
        r.v = world * 22.f;
        r.pal = lock ? PAL_LOCK : PAL_ROAD;
        r.style = 1;
        r.band = (int(world * 2.f) & 1) ? 1 : 0;
        r.left = lock ? gs::GROUND_WATER : gs::GROUND_LAND;
        r.right = lock ? gs::GROUND_WATER : gs::GROUND_LAND;
        sys_->vdp.lineFog[y] = uint8_t(clampf((1.f - n) * 9.f, 0.f, 8.f));
    }
    sys_->vdp.A.enabled = false;
    sys_->vdp.B.enabled = false;
    sys_->vdp.hudEnabled = true;
    sys_->vdp.HUD.clear();
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::draw() {
    skyRoad();
    sys_->vdp.clearSprites();

    float jx = (mode_ == Mode::Run) ? std::sin(time_ * 28.f) * shake_ * 6.f : 0.f;
    spr(art_.dash, 160.f + jx, 224.f, 86.f, PAL_CAB);
    int wi = (mode_ == Mode::Run) ? (int(s_ * 1.8f) & 3) : 0;
    spr(art_.wheel[wi], 160.f + steer_ * 12.f + jx, 214.f, 54.f, PAL_CAB);

    auto fogOf = [](float ppm) { return int(clampf(11.f - ppm * 0.18f, 0.f, 12.f)); };

    for (int i = 8; i >= 0; i--) {
        float base = std::floor(s_ / 12.f) * 12.f + i * 12.f;
        float sx, sy, ppm;
        if (project(base, -kRoad - 0.4f, sx, sy, ppm))
            spr(art_.wall, sx, sy, ppm * 3.2f, PAL_STONE, false, fogOf(ppm));
        if (project(base + 5.f, kRoad + 0.45f, sx, sy, ppm))
            spr(art_.wall, sx, sy, ppm * 3.0f, PAL_STONE, true, fogOf(ppm));
        if (project(base + 2.f, -kRoad - 0.15f, sx, sy, ppm))
            spr(art_.lamp, sx, sy, ppm * 2.2f, PAL_SIGN, false, fogOf(ppm));
    }

    float hx, hy, hp;
    if (project(kChamber0 - 4.f, -kRoad - 0.2f, hx, hy, hp))
        spr(art_.house, hx, hy, hp * 5.2f, PAL_STONE, false, fogOf(hp));
    if (project(kG0 - 6.f, 0.f, hx, hy, hp)) spr(art_.sign, hx, hy - hp * 2.4f, hp * 0.9f, PAL_SIGN, false, fogOf(hp));

    for (int i = 0; i < 2; i++) {
        float gap = gateGap(i, time_);
        float leaf = std::max(0.15f, (kRoad - gap) * 0.5f);
        float leftY = -(gap + leaf);
        float rightY = gap + leaf;
        float sx, sy, ppm;
        float htScale = 3.6f;
        if (project(kGate[i], leftY, sx, sy, ppm))
            spr(art_.gate, sx, sy, ppm * htScale * (leaf / 0.9f), PAL_GATE, false, fogOf(ppm));
        if (project(kGate[i], rightY, sx, sy, ppm))
            spr(art_.gate, sx, sy, ppm * htScale * (leaf / 0.9f), PAL_GATE, true, fogOf(ppm));
    }

    float rx, ry, rp;
    if (project(rival_, 1.15f, rx, ry, rp)) spr(art_.rival, rx, ry, rp * 1.35f, PAL_RIVAL, false, fogOf(rp));

    char buf[64];
    if (mode_ == Mode::Title) {
        hudC(3, "S3 CAB LOCK", PAL_AMBER);
        hudC(6, "PASS THE LOCK", PAL_HUD);
        hudC(8, "DO NOT SCRAPE A GATE", PAL_BAD);
        hudC(10, "THE OTHER CREW IS THE CLOCK", PAL_AMBER);
        hudC(16, "A GAS   B BRAKE", PAL_HUD);
        hudC(17, "LEFT RIGHT  HOLD THE CENTRE", PAL_HUD);
        hudC(22, "PRESS START", PAL_AMBER);
    } else if (mode_ == Mode::Pause) {
        hudC(10, "PAUSED", PAL_AMBER);
    } else if (mode_ == Mode::Win) {
        hudC(3, "PASSED", PAL_GOOD);
        hudC(5, "CLEAR OF BOTH GATES", PAL_GOOD);
        std::snprintf(buf, sizeof buf, "RUN %.1f S", time_);
        hudC(8, buf, PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        hudC(3, "LOCK FAILED", PAL_BAD);
        hudC(5, why_, PAL_BAD);
    } else {
        std::snprintf(buf, sizeof buf, "SPD %4.1f", v_);
        hud(1, 1, buf, PAL_HUD);
        float lead = rival_ - s_;
        if (lead > 0.4f) {
            std::snprintf(buf, sizeof buf, "CREW AHEAD %3.0f M", lead);
            hud(22, 1, buf, PAL_BAD);
        } else {
            std::snprintf(buf, sizeof buf, "CREW BACK %3.0f M", -lead);
            hud(22, 1, buf, PAL_GOOD);
        }
        int g = nextGate();
        if (g < 0) {
            hudC(2, "PIER AHEAD", PAL_GOOD);
        } else {
            float gap = gateGap(g, time_);
            float dist = kGate[g] - (s_ + kNose);
            const char* which = g == 0 ? "UPPER GATE" : "LOWER GATE";
            if (dist > 0.4f) {
                std::snprintf(buf, sizeof buf, "%s  %3.0f M", which, dist);
                hudC(2, buf, PAL_AMBER);
            } else {
                hudC(2, which, PAL_AMBER);
            }
            if (gap < kNeed) hudC(3, "GATE SHUT  WAIT", PAL_BAD);
            else hudC(3, "GATE OPEN  TAKE IT", PAL_GOOD);
        }
        hudC(26, "A GAS  B BRAKE  ARROWS STEER", PAL_HUD);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.75f);
    sys.apu.setEcho(0.08f, 0.16f, 0.08f);
    if (bot_) startRun();
    else showTitle();
    draw();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;

    if (beep_ > 0.f) {
        beep_ -= DT;
        if (beep_ <= 0.f) sys.apu.tone(1, 0.f, 0.f);
    }
    if (chime_ >= 0) {
        static const float notes[] = {392.f, 494.f, 587.f, 784.f};
        chimeT_ += DT;
        if (chimeT_ > 0.13f) {
            if (chime_ < 4) sys.apu.keyOn(0, notes[chime_], 0.2f);
            else sys.apu.keyOff(0);
            chime_++;
            chimeT_ = 0;
            if (chime_ > 8) chime_ = -1;
        }
    }

    if (mode_ == Mode::Title) {
        time_ += DT;
        s_ = 12.f + std::sin(time_ * 0.35f) * 0.3f;
        rival_ = 18.f + time_ * 1.4f;
        draw();
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) startRun();
        else if (pad.pressed(gs::BTN_MODE)) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        }
        return;
    }

    if (mode_ == Mode::Pause) {
        draw();
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Run;
        else if (pad.pressed(gs::BTN_MODE)) showTitle();
        return;
    }

    if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        draw();
        sys.apu.noise(0.f, 400.f, false);
        sys.apu.tone(2, 0.f, 0.f);
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
            if (mode_ == Mode::Fail) startRun();
            else showTitle();
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) showTitle();
        return;
    }

    float gas = 0, brake = 0, steer = 0;
    if (bot_) {
        pilot(gas, brake, steer);
    } else {
        if (pad.down(gs::BTN_A) || pad.down(gs::BTN_C) || pad.down(gs::BTN_UP) || pad.accel > 0.15f) gas = 1.f;
        if (pad.down(gs::BTN_B) || pad.down(gs::BTN_DOWN) || pad.brake > 0.15f) brake = 1.f;
        if (pad.down(gs::BTN_LEFT)) steer -= 1.f;
        if (pad.down(gs::BTN_RIGHT)) steer += 1.f;
        if (std::fabs(pad.axisX) > 0.2f) steer = pad.axisX;
        if (pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(280.f);
            draw();
            return;
        }
    }

    physics(gas, brake, steer);
    if (mode_ == Mode::Run) {
        float rpm = 64.f + v_ * 16.f + gas_ * 36.f;
        sys.apu.tone(2, rpm, 0.028f + gas_ * 0.028f);
        sys.apu.noise(brake_ > 0.5f && v_ > 1.f ? 0.08f : 0.012f, 640.f + v_ * 24.f, false);
        int g = nextGate();
        bool shut = g >= 0 && gateGap(g, time_) < kNeed;
        sys.setLight(shut ? 170 : 30, shut ? 40 : 90, shut ? 20 : 160);
    }
    draw();
}

}  // namespace cablock

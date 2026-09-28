#include "game/tram.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace tramlock {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float kRear = 4.6f;
constexpr float kNose = 5.8f;
constexpr float kCrew0 = 26.f;
constexpr float kHorizon = 78.f;
constexpr float kPpm = 48.f;
constexpr float kNear = 0.85f;
constexpr float kClear = 0.90f;
constexpr float kSlab = 0.42f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = clampf(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (cleared_ >= 2) return 3;
    if (cleared_ == 1) return 2;
    return 1;
}

float Game::aperture(int g, float t) const {
    const Gate& gt = gates_[g];
    const float ramp = 0.7f;
    if (t < gt.openAt) return 0.f;
    if (t < gt.openAt + ramp) return (t - gt.openAt) / ramp;
    if (t < gt.shutAt - ramp) return 1.f;
    if (t < gt.shutAt) return (gt.shutAt - t) / ramp;
    return 0.f;
}

int Game::nextGate() const {
    for (int i = 0; i < 2; i++) {
        if (s_ - kRear <= gates_[i].z + kSlab) return i;
    }
    return -1;
}

void Game::showTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    why_ = "";
    chime_ = -1;
    time_ = 0;
    crew_ = kCrew0;
    s_ = 18.f;
    v_ = 0;
    cleared_ = 0;
    scraped_ = false;
    gates_[0] = {36.f, 4.2f, 8.4f};
    gates_[1] = {74.f, 10.2f, 15.6f};
}

void Game::startRun() {
    s_ = 0;
    v_ = 0;
    time_ = 0;
    crew_ = kCrew0;
    gas_ = brake_ = 0;
    won_ = false;
    over_ = false;
    why_ = "";
    chime_ = -1;
    cleared_ = 0;
    scraped_ = false;
    gates_[0] = {36.f, 4.2f, 8.4f};
    gates_[1] = {74.f, 10.2f, 15.6f};
    mode_ = Mode::Run;
    blip(392.f);
}

void Game::pilot(float& gas, float& brake) const {
    gas = 0;
    brake = 0;
    int g = nextGate();
    if (g < 0) {
        gas = 0.4f;
        return;
    }
    float z = gates_[g].z;
    float dist = z - (s_ + kNose);
    float open = aperture(g, time_);
    const float ramp = 0.7f;
    float until = 0.f;
    if (time_ < gates_[g].openAt + ramp * kClear) until = (gates_[g].openAt + ramp * kClear) - time_;
    if (open >= kClear) {
        float left = (gates_[g].shutAt - ramp * (1.f - kClear)) - time_;
        float need = std::max(0.f, dist) / std::max(v_, 5.f);
        if (dist < -0.2f) gas = 1.f;
        else if (need + 0.45f < left) gas = 1.f;
        else brake = 1.f;
        return;
    }
    float vWant = dist < 1.4f ? 0.f : clampf(dist / std::max(until, 0.25f), 0.f, 9.f);
    if (dist < 1.6f || v_ > vWant + 0.2f) brake = 1.f;
    else if (v_ < vWant - 0.45f) gas = 1.f;
    else gas = 0.22f;
}

void Game::win() {
    if (mode_ != Mode::Run) return;
    won_ = true;
    over_ = true;
    mode_ = Mode::Win;
    why_ = "both gates clear";
    chime_ = 0;
    chimeT_ = 0;
    sys_->rumble(0.12f, 0.04f, 80);
    sys_->setLight(30, 170, 70);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    won_ = false;
    over_ = true;
    mode_ = Mode::Fail;
    why_ = why;
    sys_->rumble(0.5f, 0.22f, 150);
    sys_->setLight(170, 28, 18);
    sys_->apu.noiseBurst(0.38f, 280.f, 0.24f);
}

void Game::physics(float gas, float brake) {
    gas_ = clampf(gas, 0.f, 1.f);
    brake_ = clampf(brake, 0.f, 1.f);
    time_ += DT;
    crew_ -= DT;
    float a = gas_ * 6.2f - brake_ * 12.f - 0.32f * v_ + 0.08f;
    v_ = std::max(0.f, v_ + a * DT);
    if (v_ > 13.f) v_ = 13.f;
    s_ += v_ * DT;
    spark_ = std::max(0.f, spark_ - DT);
    if (brake_ > 0.65f && v_ > 2.f) spark_ = 0.1f;

    int passed = 0;
    for (int i = 0; i < 2; i++) {
        float z = gates_[i].z;
        float rear = s_ - kRear;
        float nose = s_ + kNose;
        bool inSlab = nose > z - kSlab && rear < z + kSlab;
        if (inSlab && aperture(i, time_) < kClear) {
            scraped_ = true;
            fail("scraped a gate");
            return;
        }
        if (rear > z + kSlab) passed++;
    }
    cleared_ = passed;
    if (passed >= 2) {
        win();
        return;
    }
    if (crew_ <= 0.f) {
        crew_ = 0;
        fail("the other crew took the lock");
        return;
    }
    if (time_ > 1.2f && v_ < 0.04f && gas_ < 0.05f && brake_ > 0.5f && nextGate() >= 0) {
        float dist = gates_[nextGate()].z - (s_ + kNose);
        if (dist > 6.f) {
            fail("stopped short of the lock");
            return;
        }
    }
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.06f);
    beep_ = 0.08f;
}

bool Game::project(float wz, float wy, float& sx, float& sy, float& ppm) const {
    float dz = wz - s_;
    if (dz < 0.45f || dz > 56.f) return false;
    float n = kNear / dz;
    sy = kHorizon + n * (gs::SCREEN_H - kHorizon);
    ppm = kPpm * n;
    sx = 160.f + wy * ppm;
    return sy > -50.f && sy < gs::SCREEN_H + 24.f;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float ht, int pal, bool flip, int fog) {
    if (ht < 1.5f || m.h < 1) return;
    float w = ht * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::lround(clampf(w, 1.f, 420.f)));
    s.h = int16_t(std::lround(clampf(ht, 1.f, 300.f)));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(ht);
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::skyRoad() {
    uint16_t zen = gs::rgb4(2, 4, 8);
    uint16_t mid = gs::rgb4(5, 8, 11);
    uint16_t hor = gs::rgb4(10, 10, 8);
    if (mode_ == Mode::Fail) hor = lerpC(hor, gs::rgb4(11, 4, 3), 0.45f);
    if (mode_ == Mode::Win) hor = lerpC(hor, gs::rgb4(6, 12, 7), 0.4f);
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
        bool chamber = world >= gates_[0].z - 1.f && world <= gates_[1].z + 1.f;
        r.on = true;
        r.cx = 160.f;
        r.hw = 2.15f * kPpm * n;
        r.v = world * 20.f;
        r.pal = chamber ? PAL_CHAMBER : PAL_ROAD;
        r.style = 1;
        r.band = (int(world * 2.f) & 1) ? 1 : 0;
        r.left = chamber ? gs::GROUND_DROP : 0;
        r.right = chamber ? gs::GROUND_DROP : 0;
        int fog = int(clampf((1.f - n) * 12.f, 0.f, 10.f));
        sys_->vdp.lineFog[y] = uint8_t(fog);
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

    for (int i = 8; i >= 0; i--) {
        float wz = std::floor(s_ / 14.f) * 14.f + i * 14.f;
        float sx, sy, ppm;
        if (project(wz, -4.6f, sx, sy, ppm)) {
            int fog = int(clampf(10.f - ppm * 0.12f, 0.f, 12.f));
            spr(art_.wall, sx, sy - ppm * 2.2f, ppm * 4.2f, PAL_STONE, false, fog);
        }
        if (project(wz + 6.f, 4.6f, sx, sy, ppm)) {
            int fog = int(clampf(10.f - ppm * 0.12f, 0.f, 12.f));
            spr(art_.wall, sx, sy - ppm * 2.0f, ppm * 3.8f, PAL_STONE, true, fog);
        }
        if (project(wz + 3.f, -2.4f, sx, sy, ppm)) spr(art_.lamp, sx, sy - ppm * 1.7f, ppm * 2.4f, PAL_IRON);
        if (project(wz + 3.f, 2.4f, sx, sy, ppm)) spr(art_.lamp, sx, sy - ppm * 1.7f, ppm * 2.4f, PAL_IRON, true);
        if (project(wz + 3.f, 0.f, sx, sy, ppm)) spr(art_.wire, sx, sy - ppm * 2.9f, ppm * 0.4f, PAL_IRON, false, 2);
    }

    for (int g = 1; g >= 0; g--) {
        float open = (mode_ == Mode::Title) ? 0.35f + 0.15f * std::sin(time_ * 1.3f + g) : aperture(g, time_);
        float swing = (1.f - open) * 1.55f + open * 3.35f;
        float sx, sy, ppm;
        if (project(gates_[g].z, -swing, sx, sy, ppm))
            spr(art_.leaf, sx, sy - ppm * 1.6f, ppm * 3.4f, PAL_GATE, false, open > 0.9f ? 0 : 1);
        if (project(gates_[g].z, swing, sx, sy, ppm))
            spr(art_.leaf, sx, sy - ppm * 1.6f, ppm * 3.4f, PAL_GATE, true, open > 0.9f ? 0 : 1);
        if (project(gates_[g].z - 1.4f, 3.4f, sx, sy, ppm)) spr(art_.capstan, sx, sy - ppm * 0.4f, ppm * 0.9f, PAL_IRON);
        if (project(gates_[g].z - 1.4f, -3.4f, sx, sy, ppm)) spr(art_.capstan, sx, sy - ppm * 0.4f, ppm * 0.9f, PAL_IRON);
    }
    float sx, sy, ppm;
    if (project(gates_[0].z - 4.f, 0.f, sx, sy, ppm)) spr(art_.board, sx, sy - ppm * 2.4f, ppm * 0.65f, PAL_SIGN);

    int notch = int(clampf(gas_ * 3.f, 0.f, 3.f));
    if (brake_ > 0.4f) notch = 0;
    spr(art_.dash, 160.f, 200.f, 78.f, PAL_CAB);
    spr(art_.lever[notch], 112.f, 170.f, 42.f, PAL_IRON);

    char buf[56];
    if (mode_ == Mode::Title) {
        hudC(3, "S3 TRAM LOCK", PAL_AMBER);
        hudC(6, "PASS THE LOCK", PAL_HUD);
        hudC(8, "DO NOT SCRAPE A GATE", PAL_BAD);
        hudC(10, "THE CLOCK IS THE OTHER CREW", PAL_AMBER);
        hudC(16, "A POWER   B BRAKE", PAL_HUD);
        hudC(22, "PRESS START", PAL_AMBER);
    } else if (mode_ == Mode::Pause) {
        hudC(10, "PAUSED", PAL_AMBER);
    } else if (mode_ == Mode::Win) {
        hudC(3, "PASSED", PAL_GOOD);
        hudC(5, "BOTH GATES CLEAR", PAL_GOOD);
        int left = int(std::max(0.f, crew_) + 0.5f);
        std::snprintf(buf, sizeof buf, "CREW %02d:%02d LEFT", left / 60, left % 60);
        hudC(8, buf, PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        hudC(3, "LEG FAILED", PAL_BAD);
        hudC(5, why_, PAL_BAD);
    } else {
        int left = int(std::max(0.f, crew_));
        std::snprintf(buf, sizeof buf, "CREW %02d:%02d", left / 60, left % 60);
        hud(1, 1, buf, crew_ < 8.f ? PAL_BAD : PAL_AMBER);
        std::snprintf(buf, sizeof buf, "SPD %4.1f", v_);
        hud(30, 1, buf, PAL_HUD);
        int g = nextGate();
        if (g < 0) hudC(2, "CLEAR OF THE LOCK", PAL_GOOD);
        else {
            float dist = gates_[g].z - (s_ + kNose);
            float open = aperture(g, time_);
            std::snprintf(buf, sizeof buf, "GATE %d  %3.0f M  %s", g + 1, std::max(0.f, dist),
                          open >= kClear ? "OPEN" : "SHUT");
            hudC(2, buf, open >= kClear ? PAL_GOOD : PAL_BAD);
        }
        std::snprintf(buf, sizeof buf, "GATES %d/2", cleared_);
        hudC(3, buf, PAL_HUD);
        hudC(26, "A POWER  B BRAKE", PAL_HUD);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.7f);
    sys.apu.setEcho(0.12f, 0.16f, 0.06f);
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
        if (chimeT_ > 0.16f) {
            if (chime_ < 4) sys.apu.keyOn(0, notes[chime_], 0.18f);
            else sys.apu.keyOff(0);
            chime_++;
            chimeT_ = 0;
            if (chime_ > 8) chime_ = -1;
        }
    }

    if (mode_ == Mode::Title) {
        time_ += DT;
        s_ = 16.f + std::sin(time_ * 0.4f) * 0.4f;
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
        sys.apu.noise(0.f, 300.f, false);
        sys.apu.tone(2, 0.f, 0.f);
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
            if (mode_ == Mode::Fail) startRun();
            else showTitle();
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) showTitle();
        return;
    }

    float gas = 0, brake = 0;
    if (bot_) {
        pilot(gas, brake);
    } else {
        if (pad.down(gs::BTN_A) || pad.down(gs::BTN_C) || pad.down(gs::BTN_UP) || pad.accel > 0.15f) gas = 1.f;
        if (pad.down(gs::BTN_B) || pad.down(gs::BTN_DOWN) || pad.brake > 0.15f) brake = 1.f;
        if (pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(280.f);
            draw();
            return;
        }
    }

    physics(gas, brake);
    if (mode_ == Mode::Run) {
        float hum = 62.f + v_ * 12.f + gas_ * 22.f;
        sys.apu.tone(2, hum, 0.022f + gas_ * 0.02f);
        sys.apu.noise(spark_ > 0.f ? 0.08f : 0.01f, 420.f + v_ * 18.f, false);
        sys.setLight(cleared_ ? 40 : 20, cleared_ ? 140 : 70, 90);
    }
    draw();
}

}  // namespace tramlock

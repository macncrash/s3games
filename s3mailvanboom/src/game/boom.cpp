#include "boom.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace boom {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float kBoom = 500.f;
constexpr float kSlot0 = kBoom - 11.f;
constexpr float kSlot1 = kBoom - 3.4f;
constexpr float kStop = (kSlot0 + kSlot1) * 0.5f;
constexpr float kClock = 28.f;
constexpr float kHalfVan = 1.05f;
constexpr float kHorizon = 78.f;
constexpr float kNear = 7.4f;
constexpr float kPpm = 40.f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    auto ch = [](uint16_t c, int sh) { return (c >> sh) & 15; };
    int r = int(ch(a, 8) + (ch(b, 8) - ch(a, 8)) * t);
    int g = int(ch(a, 4) + (ch(b, 4) - ch(a, 4)) * t);
    int bl = int(ch(a, 0) + (ch(b, 0) - ch(a, 0)) * t);
    return gs::rgb4(r, g, bl);
}

}  // namespace

float Game::centerAt(float s) const {
    return 3.2f * std::sin(s * 0.013f) + 1.4f * std::sin(s * 0.029f + 1.1f);
}

float Game::halfAt(float s) const {
    float n = s / kBoom;
    float gate = std::exp(-std::pow((n - 0.97f) / 0.06f, 2.f));
    return 6.2f - gate * 0.7f;
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (s_ > kBoom * 0.78f) return 3;
    if (s_ > kBoom * 0.42f) return 2;
    return 1;
}

void Game::showTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    why_ = "";
    time_ = 0;
    clock_ = kClock;
    s_ = 36.f;
    y_ = centerAt(s_);
    v_ = 0;
    vy_ = 0;
    dwell_ = 0;
    shake_ = 0;
    chime_ = -1;
}

void Game::startRun() {
    s_ = 16.f;
    y_ = centerAt(s_);
    v_ = 9.f;
    vy_ = 0;
    time_ = 0;
    clock_ = kClock;
    dwell_ = 0;
    shake_ = 0;
    why_ = "";
    over_ = false;
    won_ = false;
    chime_ = -1;
    mode_ = Mode::Run;
    blip(240.f);
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.08f);
    beep_ = 0.08f;
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    why_ = why;
    over_ = true;
    won_ = false;
    mode_ = Mode::Fail;
    shake_ = 1.f;
    v_ *= 0.15f;
    sys_->apu.noiseBurst(0.42f, 640.f, 0.22f);
    sys_->apu.tone(0, 64.f, 0.18f);
    beep_ = 0.26f;
}

void Game::win() {
    if (mode_ != Mode::Run) return;
    over_ = true;
    won_ = true;
    mode_ = Mode::Win;
    chime_ = 0;
    chimeT_ = 0;
    why_ = "delivered";
    v_ = 0;
}

void Game::pilot(float& gas, float& steer) const {
    const gs::Pad& pad = sys_->pad;
    gas = 0;
    steer = 0;
    if (pad.down(gs::BTN_UP) || pad.down(gs::BTN_A) || pad.accel > 0.2f) gas = 1.f;
    if (pad.down(gs::BTN_DOWN) || pad.down(gs::BTN_B) || pad.brake > 0.2f) gas = -0.75f;
    if (pad.down(gs::BTN_RIGHT)) steer += 1.f;
    if (pad.down(gs::BTN_LEFT)) steer -= 1.f;
    if (std::fabs(pad.axisX) > 0.18f) steer = pad.axisX;
    if (!bot_) return;

    float look = 22.f + std::max(0.f, v_) * 0.45f;
    float aim = centerAt(s_ + look);
    float err = aim - y_;
    float slope = (centerAt(s_ + 8.f) - centerAt(s_)) / 8.f;
    steer = clampf(err * 0.48f + (slope * v_ - vy_) * 0.1f, -1.f, 1.f);

    float dist = kStop - s_;
    if (dist > 55.f) {
        gas = 1.f;
    } else if (dist > 22.f) {
        gas = v_ > 16.f ? -0.35f : 0.45f;
    } else if (v_ > 3.2f) {
        gas = -0.9f;
    } else if (dist > 0.6f) {
        gas = 0.28f;
    } else if (dist < -0.45f) {
        gas = -0.45f;
    } else {
        gas = v_ > 0.6f ? -0.5f : 0.f;
    }
}

void Game::physics(float gas, float steer) {
    float accel = gas > 0.f ? 22.f : 30.f;
    v_ += gas * accel * DT;
    float drag = gas > 0.25f ? 0.28f : 1.35f;
    v_ *= std::exp(-DT * drag);
    v_ = clampf(v_, -5.f, 32.f);
    float grip = 8.8f + std::fabs(v_) * 0.32f;
    vy_ += (steer * grip - vy_ * 4.4f) * DT;
    s_ += v_ * DT;
    y_ += vy_ * DT;
    clock_ -= DT;
    time_ += DT;
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - DT * 1.4f);

    float worst = 99.f;
    for (int i = -1; i <= 1; i++) {
        float ps = s_ + float(i) * 2.0f;
        float room = halfAt(ps) - std::fabs(y_ - centerAt(ps)) - kHalfVan;
        worst = std::min(worst, room);
    }
    if (worst < 0.f) {
        fail("the van left the drive");
        return;
    }
    if (clock_ <= 0.f && s_ < kSlot1) {
        fail("the other crew took the boom");
        return;
    }
    if (s_ > kBoom + 1.5f) {
        fail("missed the boom");
        return;
    }
    if (s_ >= kBoom - 1.1f && v_ > 6.5f) {
        fail("smashed the boom");
        return;
    }

    bool inSlot = s_ >= kSlot0 && s_ <= kSlot1 && std::fabs(y_ - centerAt(s_)) < 1.55f && std::fabs(v_) < 2.1f;
    if (inSlot) dwell_ += DT;
    else dwell_ = 0.f;
    if (dwell_ > 0.35f) win();
}

bool Game::project(float wz, float wy, float& sx, float& sy, float& ppm) const {
    float dz = wz - s_;
    if (dz < 0.7f || dz > 96.f) return false;
    float n = kNear / dz;
    sy = kHorizon + n * (gs::SCREEN_H - kHorizon);
    ppm = kPpm * n;
    sx = 160.f + (wy - y_) * ppm;
    sx += (centerAt(wz) - centerAt(s_)) * ppm * 0.32f;
    return sy > -40.f && sy < gs::SCREEN_H + 24.f;
}

void Game::spr(const gs::Mipped& m, float cx, float feet, float ht, int pal, bool flip, int fog) {
    if (ht < 1.4f || m.h < 1) return;
    float w = ht * float(m.w) / float(m.h);
    gs::Sprite sp;
    sp.w = int16_t(std::lround(clampf(w, 1.f, 420.f)));
    sp.h = int16_t(std::lround(clampf(ht, 1.f, 320.f)));
    sp.x = int16_t(std::lround(cx - sp.w * 0.5f));
    sp.y = int16_t(std::lround(feet - sp.h));
    sp.img = m.pick(ht);
    sp.pal = uint8_t(pal);
    sp.fog = uint8_t(std::clamp(fog, 0, 16));
    sp.hflip = flip;
    sys_->vdp.sprite(sp);
}

void Game::skyRoad() {
    float rush = clampf(1.f - clock_ / kClock, 0.f, 1.f);
    if (mode_ == Mode::Title) rush = 0.08f;
    uint16_t zen = lerpC(gs::rgb4(3, 5, 10), gs::rgb4(6, 4, 5), rush);
    uint16_t mid = lerpC(gs::rgb4(7, 9, 12), gs::rgb4(9, 7, 6), rush);
    uint16_t hor = lerpC(gs::rgb4(12, 12, 10), gs::rgb4(11, 8, 6), rush);
    if (mode_ == Mode::Fail) hor = lerpC(hor, gs::rgb4(10, 3, 3), 0.4f);
    if (mode_ == Mode::Win) hor = lerpC(hor, gs::rgb4(5, 11, 6), 0.45f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float t = y / float(gs::SCREEN_H - 1);
        sys_->vdp.lineBackdrop[y] = t < 0.34f ? lerpC(zen, mid, t / 0.34f) : lerpC(mid, hor, (t - 0.34f) / 0.66f);
        gs::RoadLine& r = sys_->vdp.road[y];
        r.on = false;
        sys_->vdp.lineFog[y] = 0;
        if (y <= int(kHorizon)) continue;
        float n = (y - kHorizon) / (gs::SCREEN_H - kHorizon);
        if (n < 0.02f) continue;
        float dz = kNear / n;
        float world = s_ + dz;
        float c = centerAt(world);
        r.on = true;
        r.cx = 160.f + (c - y_) * kPpm * n;
        r.hw = halfAt(world) * kPpm * n;
        r.v = world * 5.5f;
        r.pal = PAL_ROAD;
        r.style = 1;
        r.band = (int(world) & 7) < 2 ? 1 : 0;
        bool yard = world > kBoom - 18.f;
        r.left = yard ? gs::GROUND_LAND : gs::GROUND_LAND;
        r.right = r.left;
        if (world > kBoom + 2.f) r.style = 0;
        int fog = int(clampf((1.f - n) * 8.f + rush * 3.f, 0.f, 13.f));
        sys_->vdp.lineFog[y] = uint8_t(fog);
    }
    sys_->vdp.A.enabled = false;
    sys_->vdp.B.enabled = false;
    sys_->vdp.hudEnabled = true;
    sys_->vdp.HUD.clear();
    sys_->vdp.roadTime = int(time_ * 40.f);
    sys_->vdp.setFogColor(mode_ == Mode::Win ? gs::rgb4(7, 10, 8) : gs::rgb4(8, 9, 10));
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
    auto fogOf = [](float ppm) { return int(clampf(12.f - ppm * 0.2f, 0.f, 14.f)); };

    float jx = (mode_ == Mode::Run) ? std::sin(time_ * 28.f) * (0.35f + shake_ * 6.f) : 0.f;
    spr(art_.hood, 160.f + jx, 224.f, 86.f, PAL_VAN);

    float rivalS = 8.f + kBoom * clampf((kClock - clock_) / kClock, 0.f, 1.05f);
    float sx, sy, ppm;
    if (mode_ != Mode::Win && project(rivalS, centerAt(rivalS) + 1.7f, sx, sy, ppm))
        spr(art_.rival, sx, sy, ppm * 1.7f, PAL_RIVAL, false, fogOf(ppm));

    bool raised = mode_ == Mode::Win;
    float armS = kBoom;
    if (project(armS, centerAt(armS), sx, sy, ppm)) {
        if (raised) spr(art_.armUp, sx + ppm * 2.4f, sy, ppm * 4.2f, PAL_BOOM, false, fogOf(ppm));
        else spr(art_.arm, sx, sy - ppm * 0.15f, ppm * 1.15f, PAL_BOOM, false, fogOf(ppm));
    }
    float postY = centerAt(kBoom) + halfAt(kBoom) + 0.35f;
    if (project(kBoom, postY, sx, sy, ppm)) spr(art_.post, sx, sy, ppm * 3.1f, PAL_POST, false, fogOf(ppm));

    for (int i = 0; i < 8; i++) {
        float base = std::floor(s_ / 28.f) * 28.f + float(i) * 28.f;
        if (base > kBoom - 6.f) continue;
        float left = centerAt(base) - halfAt(base) - 0.55f;
        float right = centerAt(base + 14.f) + halfAt(base + 14.f) + 0.55f;
        if (project(base, left, sx, sy, ppm)) spr(art_.bollard, sx, sy, ppm * 1.35f, PAL_POST, false, fogOf(ppm));
        if (project(base + 14.f, right, sx, sy, ppm))
            spr(art_.bollard, sx, sy, ppm * 1.35f, PAL_POST, true, fogOf(ppm));
    }

    char buf[72];
    if (mode_ == Mode::Title) {
        hudC(3, "S3 MAILVAN BOOM", PAL_CREAM);
        hudC(6, "DELIVER THE DRIVE", PAL_HUD);
        hudC(8, "THE CLOCK IS THE OTHER CREW", PAL_BAD);
        hudC(10, "STOP IN THE SLOT AT THE BOOM", PAL_HUD);
        hudC(16, "A GAS    LEFT RIGHT    B BRAKE", PAL_HUD);
        hudC(22, "PRESS START", PAL_CREAM);
    } else if (mode_ == Mode::Pause) {
        hudC(10, "PAUSED", PAL_CREAM);
    } else if (mode_ == Mode::Win) {
        hudC(3, "DRIVE DELIVERED", PAL_GOOD);
        hudC(5, "THE BOOM IS YOURS", PAL_GOOD);
        std::snprintf(buf, sizeof buf, "CREW STILL HAD  %.1f S", std::max(0.f, clock_));
        hudC(8, buf, PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        hudC(3, "DELIVERY LOST", PAL_BAD);
        hudC(5, why_, PAL_BAD);
        std::snprintf(buf, sizeof buf, "%3.0f OF %3.0f", clampf(s_, 0.f, kBoom), kBoom);
        hudC(8, buf, PAL_HUD);
    } else {
        std::snprintf(buf, sizeof buf, "CREW  %4.1f", std::max(0.f, clock_));
        hud(1, 1, buf, clock_ < 8.f ? PAL_BAD : PAL_CREAM);
        std::snprintf(buf, sizeof buf, "DRIVE %3.0f", std::max(0.f, kStop - s_));
        hud(28, 1, buf, PAL_HUD);
        if (s_ > kSlot0 - 30.f && s_ < kSlot1) hudC(24, "SLOT", PAL_GOOD);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    sys.vdp.reset();
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.7f);
    sys.apu.setEcho(0.04f, 0.08f, 0.04f);
    gs::FMPatch patch;
    patch.alg = 4;
    patch.vol = 0.28f;
    patch.op[0].mul = 1.f;
    patch.op[0].level = 1.f;
    patch.op[0].ar = 0.01f;
    patch.op[0].dr = 0.2f;
    patch.op[0].sl = 0.4f;
    patch.op[0].rr = 0.2f;
    sys.apu.setPatch(0, patch);
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
        static const float notes[] = {349.f, 440.f, 523.f, 698.f};
        chimeT_ += DT;
        if (chimeT_ > 0.13f) {
            if (chime_ < 4) sys.apu.keyOn(0, notes[chime_], 0.16f);
            else sys.apu.keyOff(0);
            chime_++;
            chimeT_ = 0;
            if (chime_ > 8) chime_ = -1;
        }
    }

    if (mode_ == Mode::Title) {
        time_ += DT;
        s_ = 42.f + std::sin(time_ * 0.35f) * 3.f;
        y_ = centerAt(s_);
        clock_ = kClock;
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

    if (mode_ == Mode::Fail || mode_ == Mode::Win) {
        draw();
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) showTitle();
        return;
    }

    if (!bot_ && pad.pressed(gs::BTN_START)) {
        mode_ = Mode::Pause;
        draw();
        return;
    }
    float gas = 0, steer = 0;
    pilot(gas, steer);
    physics(gas, steer);
    if (mode_ == Mode::Run && clock_ < 8.f) sys.apu.noise(0.03f, 240.f + (8.f - clock_) * 40.f);
    else if (mode_ != Mode::Fail) sys.apu.noise(0, 0);
    draw();
}

}  // namespace boom

#include "game/ferry.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace ferrylane {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kEnd = 300.f;
constexpr float kCrew0 = 26.f;
constexpr float kCruise = 14.2f;
constexpr float kRate = 7.2f;
constexpr float kBeam = 1.15f;
constexpr float kHor = 78.f;
constexpr float kFocus = 1680.f;
constexpr float kCamBack = 14.f;
constexpr float kPpm = 10.4f;
constexpr float kBoatY = 186.f;
constexpr float kSlip = 36.f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }
float sqr(float v) { return v * v; }

float laneMid(float z) {
    float u = clampf(z, 0.f, kEnd);
    return 2.55f * std::sin(u * 0.029f + 0.4f) + 1.45f * std::sin(u * 0.016f + 1.7f);
}

float laneHalf(float z) {
    float t = clampf(z / kEnd, 0.f, 1.f);
    float a = std::exp(-sqr((t - 0.36f) / 0.07f));
    float b = std::exp(-sqr((t - 0.70f) / 0.055f));
    return 5.35f - 1.25f * a - 1.15f * b;
}

bool tightAt(float z) { return laneHalf(z) < 4.45f; }

float currentOf(float z, float t) { return 0.42f * std::sin(z * 0.041f + t * 0.55f); }

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = clampf(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

float denomAt(float y) { return std::max(6.f, y - kHor + 8.f); }

float ppmAt(float denom) { return kPpm * denom / 80.f; }

}  // namespace

float Game::margin() const {
    float z = clampf(z_, 0.f, kEnd);
    return laneHalf(z) - std::fabs(lat_ - laneMid(z)) - kBeam;
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (z_ > kEnd - 42.f) return 3;
    if (z_ > 90.f) return 2;
    return 1;
}

void Game::showTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    why_ = "";
    banner_ = "";
    chime_ = -1;
    legT_ = 0.f;
    crew_ = kCrew0;
    z_ = 28.f;
    lat_ = laneMid(z_);
    spd_ = kCruise;
    steer_ = 0.f;
    crewZ_ = 8.f;
    shake_ = 0.f;
    near_ = tight_ = slip_ = false;
}

void Game::startRun() {
    z_ = 4.f;
    lat_ = laneMid(z_);
    spd_ = kCruise;
    steer_ = 0.f;
    legT_ = 0.f;
    crew_ = kCrew0;
    crewZ_ = 0.f;
    won_ = false;
    over_ = false;
    why_ = "";
    banner_ = "";
    chime_ = -1;
    near_ = tight_ = slip_ = false;
    crewSec_ = 99;
    shake_ = 0.f;
    mode_ = Mode::Run;
    blip(520.f);
}

float Game::steerInput() const {
    float s = 0.f;
    if (sys_->pad.down(gs::BTN_LEFT)) s -= 1.f;
    if (sys_->pad.down(gs::BTN_RIGHT)) s += 1.f;
    s += sys_->pad.axisX;
    return clampf(s, -1.f, 1.f);
}

void Game::pilotSteer(float& steer) const {
    float lead = clampf(spd_ * 0.85f, 10.f, 16.f);
    float aim = laneMid(z_ + lead);
    float here = laneMid(z_);
    float err = aim - lat_;
    float hold = here - lat_;
    float edge = laneHalf(z_) - std::fabs(hold);
    if (edge < 1.8f) {
        float push = (1.8f - edge) * 1.4f;
        if (hold >= 0.f) err += push;
        else err -= push;
    }
    float cur = currentOf(z_, legT_);
    steer = clampf(err * 0.85f - cur / kRate, -1.f, 1.f);
}

void Game::win() {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    why_ = "in the lane";
    banner_ = "IN THE LANE";
    z_ = kEnd;
    spd_ = 0.f;
    chime_ = 0;
    chimeT_ = 0.f;
    sys_->rumble(0.25f, 0.08f, 140);
    sys_->setLight(40, 170, 110);
}

void Game::fail(const char* why, const char* banner) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    why_ = why;
    banner_ = banner;
    spd_ = 0.f;
    shake_ = 1.f;
    sys_->rumble(0.5f, 0.25f, 160);
    sys_->setLight(180, 40, 28);
    sys_->apu.noiseBurst(0.35f, 220.f, 0.25f);
}

void Game::physics(float steer) {
    if (mode_ != Mode::Run) return;
    steer_ = clampf(steer, -1.f, 1.f);
    float cur = currentOf(z_, legT_);
    lat_ += (steer_ * kRate + cur) * kDt;
    z_ += spd_ * kDt;
    legT_ += kDt;
    crew_ = std::max(0.f, kCrew0 - legT_);
    crewZ_ = kEnd * (1.f - crew_ / kCrew0);

    if (!std::isfinite(z_) || !std::isfinite(lat_)) {
        fail("lost the channel", "LOST");
        return;
    }
    float at = std::min(z_, kEnd);
    float room = laneHalf(at) - std::fabs(lat_ - laneMid(at));
    if (room < kBeam) {
        fail("left the lane", "LEFT THE LANE");
        return;
    }
    if (!slip_ && z_ >= kEnd - kSlip) {
        slip_ = true;
        blip(640.f);
    }
    if (z_ >= kEnd) {
        win();
        return;
    }
    if (crew_ <= 0.f) {
        fail("the other crew", "OTHER CREW");
        return;
    }

    bool tight = tightAt(z_);
    if (tight && !tight_) {
        tight_ = true;
        blip(420.f);
        sys_->rumble(0.1f, 0.03f, 40);
    } else if (!tight && tight_) {
        tight_ = false;
    }
    float edge = room - kBeam;
    if (!near_ && edge < 0.7f) {
        near_ = true;
        blip(160.f);
    } else if (near_ && edge > 1.3f) {
        near_ = false;
    }
    if (crew_ < 7.f) {
        int sec = int(std::floor(crew_));
        if (sec != crewSec_) {
            crewSec_ = sec;
            blip(crew_ < 3.f ? 200.f : 300.f);
        }
    }
}

void Game::blip(float freq) { sys_->apu.tone(1, freq, 0.05f); }

void Game::audio() {
    if (mode_ != Mode::Run) {
        sys_->apu.noise(0.f, 400.f, false);
        sys_->apu.tone(2, 0.f, 0.f);
        if (chime_ >= 0 && mode_ == Mode::Win) {
            chimeT_ += kDt;
            if (chimeT_ > 0.16f) {
                chimeT_ = 0.f;
                static const float notes[] = {523.f, 659.f, 784.f, 1046.f};
                if (chime_ < 4) sys_->apu.tone(0, notes[chime_], 0.12f);
                chime_++;
                if (chime_ > 6) chime_ = -1;
            }
        }
        return;
    }
    float wash = 0.03f + std::fabs(steer_) * 0.02f;
    sys_->apu.noise(wash, 320.f + spd_ * 8.f, false);
    sys_->apu.tone(2, steer_ == 0.f ? 0.f : 180.f + std::fabs(steer_) * 40.f, std::fabs(steer_) * 0.02f);
    float edge = margin();
    if (crew_ < 6.f) sys_->setLight(170, 48, 32);
    else if (edge < 0.8f) sys_->setLight(180, 120, 30);
    else if (tight_) sys_->setLight(40, 90, 160);
    else sys_->setLight(20, 110, 140);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float ht, int pal, bool flip, int fog) {
    if (ht < 1.2f || m.h < 1) return;
    float w = ht * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(ht)), 1L, 2000L));
    s.x = int16_t(std::clamp(long(std::lround(cx - s.w * 0.5f + shx_)), -4000L, 4000L));
    s.y = int16_t(std::clamp(long(std::lround(cy - s.h * 0.5f + shy_)), -4000L, 4000L));
    if (s.x > gs::SCREEN_W + 8 || s.x + s.w < -8 || s.y > gs::SCREEN_H + 8 || s.y + s.h < -8) return;
    s.img = m.pick(ht);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::backdrop() {
    uint16_t zen = gs::rgb4(3, 6, 12);
    uint16_t mid = gs::rgb4(6, 10, 14);
    uint16_t sea = gs::rgb4(2, 6, 10);
    if (mode_ == Mode::Fail) sea = lerpC(sea, gs::rgb4(8, 3, 3), 0.35f);
    if (mode_ == Mode::Win) mid = lerpC(mid, gs::rgb4(10, 14, 10), 0.25f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float t = y / float(kHor);
        if (y < int(kHor)) sys_->vdp.lineBackdrop[y] = lerpC(zen, mid, clampf(t, 0.f, 1.f));
        else sys_->vdp.lineBackdrop[y] = sea;
        sys_->vdp.lineFog[y] = 0;
        sys_->vdp.road[y].on = false;
    }
    sys_->vdp.A.enabled = false;
    sys_->vdp.B.enabled = false;
    sys_->vdp.setFogColor(gs::rgb4(4, 7, 10));
}

void Game::channel() {
    float boatDenom = denomAt(kBoatY);
    for (int y = int(kHor); y < gs::SCREEN_H; y++) {
        float denom = denomAt(float(y));
        float ahead = kFocus / denom - kCamBack;
        float wz = z_ + ahead;
        float ppm = ppmAt(denom);
        gs::RoadLine& r = sys_->vdp.road[y];
        r.on = true;
        r.style = 2;
        r.pal = PAL_ROAD;
        r.left = gs::GROUND_WATER;
        r.right = gs::GROUND_WATER;
        r.v = wz * 48.f + anim_ * 18.f;
        r.band = (int(std::floor(wz * 0.35f)) & 1) ? 1 : 0;
        float mid = laneMid(wz);
        float half = laneHalf(wz);
        r.cx = 160.f + (mid - lat_) * ppm;
        r.hw = std::max(2.f, half * ppm);
        (void)boatDenom;
    }
    sys_->vdp.roadTime = int(anim_ * 60.f);
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    backdrop();
    if (mode_ == Mode::Title) {
        z_ = 36.f + std::sin(anim_ * 0.25f) * 6.f;
        lat_ = laneMid(z_);
        crewZ_ = z_ + 22.f;
    }
    if (shake_ > 0.f) {
        shx_ = std::sin(anim_ * 80.f) * shake_ * 4.f;
        shy_ = std::cos(anim_ * 60.f) * shake_ * 2.f;
        shake_ *= 0.9f;
        if (shake_ < 0.04f) shake_ = 0.f;
    } else {
        shx_ = shy_ = 0.f;
    }
    channel();

    auto place = [&](float wz, float worldX, float& sx, float& sy, float& ppm) {
        float ahead = wz - z_;
        float denom = kFocus / std::max(4.f, ahead + kCamBack);
        sy = denom - 8.f + kHor;
        ppm = ppmAt(denom);
        sx = 160.f + (worldX - lat_) * ppm;
    };

    float ppmBoat = ppmAt(denomAt(kBoatY));
    float ferryH = 78.f * (ppmBoat / 16.f);
    ferryH = clampf(ferryH, 48.f, 96.f);

    // Far scenery first (drawn underneath; earlier sprites sit on top).
    for (int i = 0; i < 3; i++) {
        float cx = std::fmod(40.f + float(i) * 130.f + anim_ * 6.f, 420.f);
        if (cx < -30.f) cx += 420.f;
        spr(art_.cloud, cx, 28.f + float(i) * 12.f, 14.f + float(i % 2) * 4.f, PAL_SKY, i & 1, 0);
    }
    for (int i = 0; i < 3; i++) {
        float gx = 50.f + float(i) * 90.f + std::sin(anim_ * 0.7f + float(i)) * 12.f;
        int fr = int(anim_ * 3.f + float(i)) & 1;
        spr(art_.gull[fr], gx, 58.f + float(i % 2) * 8.f, 10.f, PAL_GULL, i & 1, 1);
    }

    float psx, psy, pp;
    place(kEnd, laneMid(kEnd), psx, psy, pp);
    if (psy > kHor - 10.f && psy < gs::SCREEN_H) {
        float ph = clampf(28.f * pp / 8.f, 8.f, 40.f);
        spr(art_.pier, psx, psy, ph, PAL_PIER, false, psy < kHor + 20.f ? 6 : 1);
    }

    float z0 = std::floor((z_ - 2.f) / 10.f) * 10.f;
    for (float bz = z0; bz < z_ + 78.f; bz += 10.f) {
        if (bz < 2.f || bz > kEnd + 2.f) continue;
        for (int side = -1; side <= 1; side += 2) {
            float wx = laneMid(bz) + float(side) * (laneHalf(bz) + 0.15f);
            float sx, sy, ppm;
            place(bz, wx, sx, sy, ppm);
            if (sy < kHor || sy > gs::SCREEN_H - 8.f) continue;
            float bh = clampf(22.f * ppm / 10.f, 6.f, 28.f);
            spr(art_.buoy, sx, sy, bh, PAL_BUOY, side < 0, sy < kHor + 24.f ? 5 : 0);
        }
    }

    if (crewZ_ > z_ + 1.f && crewZ_ < z_ + 70.f) {
        float sx, sy, ppm;
        place(crewZ_, laneMid(crewZ_), sx, sy, ppm);
        float h = clampf(ferryH * ppm / std::max(1.f, ppmBoat), 10.f, ferryH);
        spr(art_.ferry, sx, sy, h, PAL_CREW, false, 4);
    }

    for (int i = 0; i < 3; i++) {
        float fz = z_ - 1.5f - float(i) * 1.2f;
        float fx = lat_ + steer_ * float(i) * 0.15f;
        float sx, sy, ppm;
        place(fz, fx, sx, sy, ppm);
        if (sy > gs::SCREEN_H + 8.f) continue;
        spr(art_.foam, 160.f + (fx - lat_) * ppmBoat, kBoatY + 34.f + float(i) * 8.f, 8.f + float(i) * 3.f, PAL_FOAM,
            false, i * 3);
    }

    bool flip = steer_ < -0.05f;
    spr(art_.ferry, 160.f + steer_ * 4.f, kBoatY, ferryH, PAL_SHIP, flip, 0);

    if (mode_ == Mode::Title) {
        hudC(2, "FERRY LANE", PAL_HUD);
        hudC(4, "STAY IN THE LANE", PAL_AMBER);
        int cs = int(kCrew0);
        char buf[48];
        std::snprintf(buf, sizeof buf, "CREW CLOCK  %d:%02d", cs / 60, cs % 60);
        hudC(16, "LEFT AND RIGHT STEER", PAL_HUD);
        hudC(18, "THE CHANNEL IS THE LANE", PAL_AMBER);
        hudC(20, buf, PAL_GOOD);
        hudC(22, "THE CLOCK IS THE OTHER CREW", PAL_HUD);
        if ((sys_->frame / 30) % 2 == 0) hudC(25, "PRESS START", PAL_GOOD);
        hud(39 - int(std::strlen(S3_VERSION_STRING)), 27, S3_VERSION_STRING, PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        hudC(3, "PAUSE", PAL_HUD);
        hudC(24, "START RUNS    ESC TITLE", PAL_AMBER);
    } else if (mode_ == Mode::Fail) {
        hudC(2, banner_, PAL_BAD);
        hudC(23, why_, PAL_BAD);
        hudC(25, "START TRIES AGAIN", PAL_HUD);
    } else if (mode_ == Mode::Win) {
        hudC(2, "IN THE LANE", PAL_GOOD);
        hudC(22, "THE WHOLE LEG, STILL IN THE LANE", PAL_GOOD);
        char buf[48];
        std::snprintf(buf, sizeof buf, "%.1f S", legT_);
        hudC(24, buf, PAL_HUD);
        int leftS = int(std::floor(crew_ + 1e-3f));
        std::snprintf(buf, sizeof buf, "CREW HAD %d S", leftS);
        hudC(25, buf, PAL_AMBER);
    } else {
        char buf[48];
        float err = lat_ - laneMid(z_);
        std::snprintf(buf, sizeof buf, "LANE %+4.1f", err);
        int lp = std::fabs(err) < 1.f ? PAL_GOOD : (margin() < 0.9f ? PAL_AMBER : PAL_HUD);
        hud(1, 1, buf, lp);
        std::snprintf(buf, sizeof buf, "SPD %4.1f", spd_);
        hud(14, 1, buf, PAL_HUD);
        int csec = int(std::ceil(crew_ - 1e-4f));
        if (csec < 0) csec = 0;
        std::snprintf(buf, sizeof buf, "CREW %d:%02d", csec / 60, csec % 60);
        hud(25, 1, buf, crew_ < 7.f ? PAL_BAD : (crew_ < 12.f ? PAL_AMBER : PAL_HUD));
        const char* line = "IN THE LANE";
        int pal = PAL_GOOD;
        float leftM = kEnd - z_;
        if (near_) {
            line = "NEAR THE BUOY";
            pal = PAL_AMBER;
        } else if (crewZ_ > z_ + 4.f) {
            line = "CREW AHEAD";
            pal = PAL_BAD;
        } else if (tight_) {
            line = "TIGHT CHANNEL";
            pal = PAL_AMBER;
        } else if (leftM < 50.f) {
            std::snprintf(buf, sizeof buf, "SLIP %3.0f M", std::max(0.f, leftM));
            line = buf;
            pal = PAL_GOOD;
        } else {
            std::snprintf(buf, sizeof buf, "END %3.0f M", std::max(0.f, leftM));
            line = buf;
            pal = PAL_HUD;
        }
        hudC(2, line, pal);
        hudC(26, "LEFT RIGHT  HOLD THE CHANNEL", PAL_HUD);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(4, 7, 10));
    sys.vdp.hudEnabled = true;
    sys.apu.setMaster(0.7f);
    if (bot_) startRun();
    else showTitle();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    anim_ += kDt;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) startRun();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Run;
        else if (pad.pressed(gs::BTN_B)) showTitle();
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else {
            float steer = bot_ ? 0.f : steerInput();
            if (bot_) pilotSteer(steer);
            physics(steer);
        }
    } else if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
        startRun();
    }
    audio();
    draw();
}

}  // namespace ferrylane

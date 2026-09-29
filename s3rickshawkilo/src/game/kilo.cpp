#include "game/kilo.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace kilo {
namespace {

constexpr double DT = 1.0 / 60.0;
constexpr double kFinish = 1000.0;
constexpr double kClock = 70.0;
constexpr double kCruise = 18.2;
constexpr double kHitS = 2.15;
constexpr double kHitLat = 0.34;
constexpr int kHorizon = 86;
constexpr int kWheelN = 10;

struct WheelDef {
    double s;
    double lat;
};

constexpr WheelDef kWheels[kWheelN] = {
    {130, -0.62}, {230, 0.62}, {330, 0.00}, {430, -0.62}, {530, 0.62},
    {620, 0.00},  {720, -0.62}, {810, 0.62}, {900, 0.00}, {960, -0.62},
};

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }
double clampd(double v, double a, double b) { return std::max(a, std::min(b, v)); }

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

bool laneBlocked(double lat, double wlat) { return std::fabs(lat - wlat) < kHitLat + 0.08; }

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (s_ >= 780.0) return 3;
    if (s_ >= 120.0) return 2;
    return 1;
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.06f);
    beep_ = 0.08f;
}

void Game::showTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    whyBuf_[0] = 0;
    time_ = 0;
    s_ = 0;
    v_ = 0;
    lat_ = 0;
    latV_ = 0;
    shake_ = 0;
    chime_ = -1;
    passed_ = 0;
}

void Game::startRun() {
    mode_ = Mode::Run;
    won_ = false;
    over_ = false;
    whyBuf_[0] = 0;
    time_ = 0;
    s_ = 0;
    v_ = 12.0;
    lat_ = 0;
    latV_ = 0;
    shake_ = 0;
    chime_ = -1;
    passed_ = 0;
    blip(520.f);
}

void Game::pilot(float& steer, bool& pedal) const {
    pedal = true;
    const double lanes[3] = {-0.62, 0.0, 0.62};
    double best = lat_;
    double bestScore = -1e9;
    for (double lane : lanes) {
        bool bad = false;
        double soon = 1e9;
        for (int i = 0; i < kWheelN; i++) {
            double ahead = kWheels[i].s - s_;
            if (ahead < -kHitS || ahead > 36.0) continue;
            if (!laneBlocked(lane, kWheels[i].lat)) continue;
            bad = true;
            soon = std::min(soon, ahead);
        }
        if (bad) continue;
        double score = 40.0 - std::fabs(lane - lat_) * 8.0;
        if (std::fabs(lane) < 0.1) score += 1.5;
        if (score > bestScore) {
            bestScore = score;
            best = lane;
        }
    }
    if (bestScore < -1e8) {
        double far = 0;
        double gap = -1;
        for (double lane : lanes) {
            double g = 1e9;
            for (int i = 0; i < kWheelN; i++) {
                double ahead = kWheels[i].s - s_;
                if (ahead < -kHitS) continue;
                if (laneBlocked(lane, kWheels[i].lat)) g = std::min(g, std::max(0.0, ahead));
            }
            if (g > gap) {
                gap = g;
                far = lane;
            }
        }
        best = far;
    }
    steer = float(clampd((best - lat_) * 4.2, -1.0, 1.0));
}

void Game::win() {
    if (mode_ != Mode::Run) return;
    won_ = true;
    over_ = true;
    mode_ = Mode::Win;
    chime_ = 0;
    chimeT_ = 0;
    s_ = std::max(s_, kFinish);
    sys_->rumble(0.12f, 0.04f, 120);
    sys_->setLight(40, 140, 70);
    std::printf(
        "S3 RICKSHAW KILO  CLEAR  finished the kilometer  wheels untouched  the other crew still on the clock  (%.1f s)\n",
        time_);
    std::fflush(stdout);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    won_ = false;
    over_ = true;
    mode_ = Mode::Fail;
    std::snprintf(whyBuf_, sizeof whyBuf_, "%s", why);
    shake_ = 0.5f;
    sys_->rumble(0.4f, 0.2f, 160);
    sys_->setLight(160, 36, 20);
    sys_->apu.noiseBurst(0.4f, 160.f, 0.28f);
    std::printf("S3 RICKSHAW KILO  FAIL  %s at %d m\n", why, int(clampd(s_, 0.0, kFinish)));
    std::fflush(stdout);
}

void Game::physics(float steer, bool pedal) {
    steer = clampf(steer, -1.f, 1.f);
    time_ += DT;
    double want = pedal ? kCruise : 8.5;
    v_ += (want - v_) * std::min(1.0, 2.2 * DT);
    v_ = clampd(v_, 4.0, 22.0);
    s_ += v_ * DT;
    latV_ += (steer * 1.35 - latV_) * std::min(1.0, 8.0 * DT);
    lat_ += latV_ * DT;
    if (lat_ < -0.92) {
        lat_ = -0.92;
        latV_ = 0;
    }
    if (lat_ > 0.92) {
        lat_ = 0.92;
        latV_ = 0;
    }
    for (int i = 0; i < kWheelN; i++) {
        if (std::fabs(s_ - kWheels[i].s) > kHitS) continue;
        if (std::fabs(lat_ - kWheels[i].lat) < kHitLat) {
            fail("touched a wheel");
            return;
        }
    }
    int clear = 0;
    for (int i = 0; i < kWheelN; i++)
        if (s_ > kWheels[i].s + kHitS) clear++;
    if (clear > passed_) {
        passed_ = clear;
        blip(660.f + float(clear) * 18.f);
    }
    if (s_ >= kFinish) {
        win();
        return;
    }
    if (time_ >= kClock) fail("the other crew took the kilo");
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

void Game::hudC(int row, const char* s, int pal) {
    if (!s) return;
    hud(20 - int(std::strlen(s)) / 2, row, s, pal);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float ht, int pal, bool flip, int fog) {
    if (ht < 1.5f || m.h < 1) return;
    float w = ht * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 400L));
    s.h = int16_t(std::clamp(long(std::lround(ht)), 1L, 400L));
    s.x = int16_t(std::clamp(long(std::lround(cx - s.w * 0.5f)), -400L, 800L));
    s.y = int16_t(std::clamp(long(std::lround(cy - s.h)), -400L, 800L));
    if (s.x > gs::SCREEN_W + 4 || s.x + s.w < -4 || s.y > gs::SCREEN_H + 4 || s.y + s.h < -8) return;
    s.img = m.pick(ht);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.hudEnabled = true;
    vdp.setFogColor(gs::rgb4(8, 10, 12));

    const bool title = mode_ == Mode::Title;
    double showS = title ? t_ * 6.0 : s_;
    double showLat = title ? std::sin(t_ * 0.7) * 0.25 : lat_;
    float shakeX = 0;
    if (shake_ > 0) shakeX = std::sin(float(t_) * 40.f) * shake_ * 6.f;

    uint16_t zen = gs::rgb4(4, 7, 12);
    uint16_t hor = gs::rgb4(14, 12, 9);
    if (mode_ == Mode::Fail) hor = gs::rgb4(12, 6, 5);
    if (mode_ == Mode::Win) hor = gs::rgb4(8, 13, 9);

    for (int y = 0; y < gs::SCREEN_H; y++) {
        float skyT = y / float(kHorizon);
        vdp.lineBackdrop[y] = y < kHorizon ? lerpC(zen, hor, skyT) : hor;
        vdp.lineFog[y] = 0;
        gs::RoadLine& r = vdp.road[y];
        if (y <= kHorizon) {
            r.on = false;
            continue;
        }
        float t = (y - kHorizon) / float(gs::SCREEN_H - 1 - kHorizon);
        t = std::max(t, 0.018f);
        float z = 2.35f / t;
        float bend = std::sin(float(showS + z) * 0.012f) * 28.f * (1.f - t * 0.35f);
        r.on = true;
        r.cx = 160.f + bend + shakeX;
        r.hw = 300.f / z;
        r.v = float(showS + z) * 48.f;
        r.pal = PAL_ROAD;
        r.band = (int(std::floor(showS + z)) & 1) ? 1 : 0;
        r.style = 1;
        r.left = 0;
        r.right = 0;
        if (z > 28.f) vdp.lineFog[y] = uint8_t(std::min(10.f, (z - 28.f) * 0.35f));
    }

    auto project = [&](double zs, double lat, float& sx, float& sy, float& fog) {
        float z = float(std::max(zs, 1.6));
        float t = 2.35f / z;
        int y = kHorizon + int(t * float(gs::SCREEN_H - 1 - kHorizon));
        y = std::clamp(y, 0, gs::SCREEN_H - 1);
        sx = vdp.road[y].cx + float(lat) * vdp.road[y].hw;
        sy = float(y);
        fog = z > 24.f ? std::min(12.f, (z - 24.f) * 0.4f) : 0.f;
    };

    struct Spr {
        float z, sx, sy, h, fog;
        int kind;
        bool flip;
    };
    Spr list[24];
    int n = 0;
    auto push = [&](float z, double lat, float h, int kind, bool flip) {
        if (n >= 24 || z < 1.5f || z > 70.f) return;
        float sx, sy, fog;
        project(z, lat, sx, sy, fog);
        list[n++] = {z, sx, sy, h * (18.f / std::max(z, 2.f)), fog, kind, flip};
    };

    if (!title) {
        double rivalS = time_ * (kFinish / kClock);
        double dz = rivalS - s_;
        if (dz > 2.0 && dz < 60.0) push(float(dz), 0.15, 34.f, 1, false);
    }
    for (int i = 0; i < kWheelN; i++) {
        double dz = kWheels[i].s - showS;
        push(float(dz), kWheels[i].lat, 22.f, 2, (i & 1) != 0);
        push(float(dz + 0.4), kWheels[i].lat, 16.f, 3, false);
    }
    for (int i = 0; i < 18; i++) {
        double ps = std::floor(showS / 40.0) * 40.0 + i * 40.0;
        double dz = ps - showS;
        double side = (i & 1) ? 1.35 : -1.35;
        push(float(dz), side, (i % 3 == 0) ? 30.f : 26.f, (i % 3 == 0) ? 4 : 5, side < 0);
    }

    std::sort(list, list + n, [](const Spr& a, const Spr& b) { return a.z < b.z; });
    for (int i = n - 1; i >= 0; i--) {
        const Spr& p = list[i];
        const gs::Mipped* m = &art_.wheel;
        int pal = PAL_WHEEL;
        if (p.kind == 1) {
            m = &art_.rival;
            pal = PAL_RIVAL;
        } else if (p.kind == 3) {
            m = &art_.cart;
            pal = PAL_CART;
        } else if (p.kind == 4) {
            m = &art_.stall;
            pal = PAL_STALL;
        } else if (p.kind == 5) {
            m = &art_.tree;
            pal = PAL_TREE;
        }
        spr(*m, p.sx, p.sy, p.h, pal, p.flip, int(p.fog));
    }

    float psx, psy, pfog;
    project(2.5, showLat, psx, psy, pfog);
    spr(art_.cab, psx, psy + 6.f, 52.f, PAL_CAB, false, 0);
    spr(art_.puller, psx, psy - 8.f, 28.f, PAL_CAB, false, 0);

    char buf[48];
    if (title) {
        hudC(2, "RICKSHAW KILO", PAL_AMBER);
        hudC(4, "FINISH THE KILOMETER", PAL_HUD);
        hudC(5, "DO NOT TOUCH A WHEEL", PAL_GOOD);
        hudC(7, "THE CLOCK IS THE OTHER CREW", PAL_HUD);
        hudC(10, "A START    ARROWS STEER", PAL_AMBER);
    } else if (mode_ == Mode::Pause) {
        hudC(3, "PAUSE", PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        hudC(2, "TOUCHED", PAL_BAD);
        hudC(4, whyBuf_, PAL_HUD);
    } else if (mode_ == Mode::Win) {
        hudC(2, "KILO CLEAR", PAL_GOOD);
        hudC(4, "WHEELS UNTOUCHED", PAL_HUD);
    }
    if (!title) {
        int meters = int(clampd(s_, 0.0, kFinish));
        double left = std::max(0.0, kClock - time_);
        std::snprintf(buf, sizeof buf, "%d M", meters);
        hud(1, 26, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "CREW %d", int(left));
        hud(28, 26, buf, left < 12.0 ? PAL_BAD : PAL_AMBER);
        hudC(24, "1000 M", PAL_GOOD);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.silence();
    sys.apu.setMaster(0.8f);
    showTitle();
    if (bot_) startRun();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    if (beep_ > 0) {
        beep_ -= float(DT);
        if (beep_ <= 0) sys.apu.tone(0, 0, 0);
    }
    if (shake_ > 0) shake_ = std::max(0.f, shake_ - float(DT));
    if (chime_ >= 0) {
        static const float notes[] = {523.f, 659.f, 784.f, 1046.f};
        chimeT_ -= float(DT);
        if (chimeT_ <= 0 && chime_ < 4) {
            sys.apu.tone(1, notes[chime_], 0.07f);
            chimeT_ = 0.12f;
            chime_++;
        }
        if (chime_ >= 4 && chimeT_ <= 0) sys.apu.tone(1, 0, 0);
    }

    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) startRun();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Run;
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        float steer = 0;
        bool pedal = false;
        if (bot_) {
            pilot(steer, pedal);
        } else {
            if (pad.down(gs::BTN_LEFT)) steer -= 1.f;
            if (pad.down(gs::BTN_RIGHT)) steer += 1.f;
            if (std::fabs(pad.axisX) > 0.2f) steer = pad.axisX;
            pedal = pad.down(gs::BTN_A) || pad.down(gs::BTN_C) || pad.accel > 0.2f;
            if (!pad.down(gs::BTN_A) && !pad.down(gs::BTN_B) && !pad.down(gs::BTN_C) && !pad.down(gs::BTN_LEFT) &&
                !pad.down(gs::BTN_RIGHT) && std::fabs(pad.axisX) < 0.15f) {
                // A fresh run still rolls; the pedal holds the crew's pace.
            }
            if (pad.down(gs::BTN_B)) pedal = false;
        }
        physics(steer, pedal);
    } else if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
        showTitle();
    }

    if (mode_ == Mode::Run) sys.apu.tone(2, 70.f + float(v_) * 3.f, 0.03f);
    else sys.apu.tone(2, 0, 0);

    draw();
    sys.vdp.roadTime = int(t_ * 60.0);
}

}  // namespace kilo

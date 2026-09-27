#include "game/kilo.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace bkilo {
namespace {

constexpr double DT = 1.0 / 60.0;
constexpr double kFinish = 1000.0;
constexpr double kClock = 72.0;
constexpr double kScale = 4.15;
constexpr double kHull = 7.2;
constexpr double kBeam = 0.105;
constexpr int kWheelN = 7;

struct WheelDef {
    double x;
    double half;
    double lat0, lat1;
    int kind;  // 0 mill, 1 crane sheave, 2 towpath cart
};

constexpr WheelDef kWheels[kWheelN] = {
    {150, 5.2, 0.00, 0.56, 0}, {290, 3.6, 0.48, 1.00, 1}, {430, 5.4, 0.00, 0.52, 0},
    {560, 3.4, 0.55, 1.00, 2}, {690, 6.2, 0.00, 0.60, 0}, {820, 3.8, 0.42, 1.00, 1},
    {930, 4.4, 0.00, 0.50, 2},
};

double clampd(double v, double a, double b) { return std::max(a, std::min(b, v)); }

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

const char* wheelWord(int kind) {
    if (kind == 0) return "a mill wheel";
    if (kind == 1) return "a crane wheel";
    return "a cart wheel";
}

gs::FMPatch bellPatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.1f;
    p.op[0] = {1.f, 1.f, 0.01f, 0.16f, 0.5f, 0.2f};
    p.op[1] = {2.f, 0.24f, 0.02f, 0.2f, 0.3f, 0.16f};
    p.op[2] = {3.f, 0.06f, 0.02f, 0.22f, 0.16f, 0.18f};
    p.op[3] = {1.f, 0.f, 0.02f, 0.2f, 0.16f, 0.18f};
    p.vol = 0.2f;
    p.tone = 1200.f;
    return p;
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (x_ >= 760.0) return 3;
    if (x_ >= 140.0) return 2;
    return 1;
}

int Game::nextWheel() const {
    int best = -1;
    double bestX = 1e9;
    for (int i = 0; i < kWheelN; i++) {
        if (kWheels[i].x + kWheels[i].half < x_ - 2.0) continue;
        if (kWheels[i].x < bestX) {
            bestX = kWheels[i].x;
            best = i;
        }
    }
    return best;
}

void Game::showTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    whyBuf_[0] = 0;
    cue_ = -2;
    chime_ = -1;
    time_ = 0;
    x_ = 18;
    v_ = 0;
    lat_ = 0.42;
    latV_ = 0;
    camX_ = 40;
    shake_ = 0;
}

void Game::startRun() {
    x_ = 0;
    v_ = 14.2;
    lat_ = 0.50;
    latV_ = 0;
    time_ = 0;
    won_ = false;
    over_ = false;
    whyBuf_[0] = 0;
    cue_ = -2;
    chime_ = -1;
    shake_ = 0;
    mode_ = Mode::Run;
    camX_ = 8;
    blip(480.f);
}

void Game::pilot(double& stick, bool& pole) const {
    pole = true;
    double target = 0.50;
    int focus = -1;
    double focusD = 1e9;
    for (int i = 0; i < kWheelN; i++) {
        double ahead = kWheels[i].x - x_;
        if (ahead < -kWheels[i].half - 4.0 || ahead > 46.0) continue;
        if (ahead < focusD) {
            focusD = ahead;
            focus = i;
        }
    }
    if (focus >= 0) {
        const WheelDef& w = kWheels[focus];
        double below = w.lat0 - (kBeam + 0.07);
        double above = w.lat1 + (kBeam + 0.07);
        bool canBelow = below >= 0.16;
        bool canAbove = above <= 0.84;
        if (canBelow && canAbove) target = (std::fabs(lat_ - below) <= std::fabs(lat_ - above)) ? below : above;
        else if (canAbove) target = above;
        else target = std::min(0.84, std::max(0.16, below));
    }
    stick = clampd((target - lat_) * 3.4, -1.0, 1.0);
}

void Game::win() {
    if (mode_ != Mode::Run) return;
    won_ = true;
    over_ = true;
    mode_ = Mode::Win;
    chime_ = 0;
    chimeT_ = 0;
    x_ = std::max(x_, kFinish);
    sys_->rumble(0.14f, 0.05f, 140);
    sys_->setLight(30, 150, 80);
    std::printf("S3 BARGE KILO  CLEAR  finished the kilometer  1000 m  wheels untouched  (%.1f s)\n", time_);
    std::fflush(stdout);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    won_ = false;
    over_ = true;
    mode_ = Mode::Fail;
    std::snprintf(whyBuf_, sizeof whyBuf_, "%s", why);
    shake_ = 0.45f;
    int meters = int(std::clamp(x_, 0.0, kFinish));
    sys_->rumble(0.45f, 0.22f, 160);
    sys_->setLight(160, 40, 24);
    sys_->apu.noiseBurst(0.42f, 180.f, 0.3f);
    std::printf("S3 BARGE KILO  FAIL  %s at %d m\n", why, meters);
    std::fflush(stdout);
}

void Game::physics(double stick, bool pole) {
    stick = clampd(stick, -1.0, 1.0);
    time_ += DT;
    double want = pole ? 16.8 : 13.1;
    v_ += (want - v_) * std::min(1.0, 1.6 * DT);
    v_ = clampd(v_, 6.0, 18.0);
    x_ += v_ * DT;
    latV_ += (stick * 0.95 - latV_) * std::min(1.0, 6.0 * DT);
    lat_ += latV_ * DT;
    if (lat_ < 0.12) {
        lat_ = 0.12;
        latV_ = 0;
    }
    if (lat_ > 0.88) {
        lat_ = 0.88;
        latV_ = 0;
    }
    if (!std::isfinite(x_) || !std::isfinite(lat_)) {
        fail("lost the cut");
        return;
    }
    for (int i = 0; i < kWheelN; i++) {
        const WheelDef& w = kWheels[i];
        if (std::fabs(x_ - w.x) > w.half + kHull) continue;
        double a0 = lat_ - kBeam, a1 = lat_ + kBeam;
        if (a1 > w.lat0 && a0 < w.lat1) {
            char buf[48];
            std::snprintf(buf, sizeof buf, "touched %s", wheelWord(w.kind));
            fail(buf);
            return;
        }
    }
    if (x_ >= kFinish) {
        win();
        return;
    }
    if (time_ > kClock) fail("the other crew made the kilo");
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.05f);
    beep_ = 0.07f;
}

void Game::sky() {
    uint16_t zen = gs::rgb4(4, 7, 12);
    uint16_t mid = gs::rgb4(7, 11, 14);
    uint16_t hor = gs::rgb4(12, 13, 10);
    if (mode_ == Mode::Fail) hor = lerpC(hor, gs::rgb4(12, 5, 4), 0.4f);
    if (mode_ == Mode::Win) hor = lerpC(hor, gs::rgb4(8, 13, 9), 0.35f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float t = y / float(gs::SCREEN_H - 1);
        sys_->vdp.lineBackdrop[y] = t < 0.5f ? lerpC(zen, mid, t / 0.5f) : lerpC(mid, hor, (t - 0.5f) / 0.5f);
        sys_->vdp.lineFog[y] = 0;
        sys_->vdp.road[y].on = false;
    }
    sys_->vdp.A.enabled = false;
    sys_->vdp.B.enabled = false;
    sys_->vdp.setFogColor(gs::rgb4(6, 9, 11));
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
    if (ht < 1.2f || m.h < 1) return;
    float w = ht * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(ht)), 1L, 2000L));
    s.x = int16_t(std::clamp(long(std::lround(cx - s.w * 0.5f)), -8000L, 8000L));
    s.y = int16_t(std::clamp(long(std::lround(cy - s.h * 0.5f)), -8000L, 8000L));
    if (s.x > gs::SCREEN_W + 8 || s.x + s.w < -8 || s.y > gs::SCREEN_H + 8 || s.y + s.h < -40) return;
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
    sky();

    const bool title = mode_ == Mode::Title;
    double shipX = title ? 36.0 + std::sin(t_ * 0.6) * 2.0 : x_;
    double shipLat = title ? 0.42 + std::sin(t_ * 0.8) * 0.04 : lat_;

    if (title) {
        hudC(2, "BARGE KILO", PAL_HUD);
        hudC(4, "FINISH THE KILOMETER", PAL_AMBER);
        hudC(5, "DO NOT TOUCH A WHEEL", PAL_GOOD);
        hudC(7, "THE CLOCK IS THE OTHER CREW", PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        hudC(3, "PAUSE", PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        hudC(3, "TOUCHED", PAL_BAD);
    } else if (mode_ == Mode::Win) {
        hudC(2, "CLEAR", PAL_GOOD);
        hudC(4, "WHEELS UNTOUCHED", PAL_HUD);
    }

    double lead = title ? 0.0 : 22.0;
    double want = shipX + lead;
    if (title) camX_ = 20;
    else camX_ += (want - camX_) * 0.12;
    if (shake_ > 0) camX_ += std::sin(t_ * 40.0) * double(shake_) * 1.4;

    auto sxOf = [&](double wx) { return float(78.0 + (wx - camX_) * kScale); };
    auto syLat = [&](double lat) { return float(128.0 + lat * 46.0); };

    spr(art_.sun, 28.f, 18.f, 16.f, PAL_SKY);
    for (int i = 0; i < 3; i++) {
        float cx = std::fmod(30.f + float(i) * 120.f - float(camX_) * 0.15f, 400.f);
        if (cx < -30.f) cx += 400.f;
        spr(art_.cloud, cx, 28.f + float(i) * 10.f, 14.f, PAL_SKY, i & 1, 3);
    }

    float left = float(camX_) - 40.f;
    float right = float(camX_) + 90.f;
    for (float wx = std::floor(left / 10.f) * 10.f; wx < right; wx += 10.f) {
        spr(art_.water, sxOf(wx), 168.f, 22.f, PAL_WATER);
        spr(art_.water, sxOf(wx), 188.f, 22.f, PAL_WATER, false, 2);
        spr(art_.bank, sxOf(wx), 112.f, 18.f, PAL_BANK, false, 1);
    }

    const double trees[] = {40, 210, 380, 610, 780, 960};
    for (int i = 0; i < 6; i++) {
        if (i % 2 == 0) spr(art_.tree, sxOf(trees[i]), 96.f, 36.f, PAL_TREE, false, 2);
        else spr(art_.house, sxOf(trees[i]), 100.f, 28.f, PAL_TREE, false, 3);
        spr(art_.reed, sxOf(trees[i] + 18), syLat(0.08), 18.f, PAL_BANK);
    }

    for (int i = 0; i < kWheelN; i++) {
        const WheelDef& w = kWheels[i];
        float sx = sxOf(w.x);
        int fr = int(t_ * 5.0 + i) & 3;
        float rim = std::max(16.f, float((w.lat1 - w.lat0) * 46.0 + 10.0));
        float cy = syLat((w.lat0 + w.lat1) * 0.5);
        if (w.kind == 0) {
            spr(art_.mill, sx - 8.f, 96.f, 48.f, PAL_MILL, false, 1);
            spr(art_.wheel[fr], sx + 10.f, cy, rim, PAL_MILL);
        } else if (w.kind == 1) {
            spr(art_.crane, sx, 108.f, 56.f, PAL_CRANE, false, 1);
            spr(art_.wheel[fr], sx + 6.f, cy, rim, PAL_CRANE);
        } else {
            spr(art_.cart, sx, syLat(0.92) - 8.f, 18.f, PAL_CART);
            spr(art_.wheel[fr], sx - 8.f, cy, rim * 0.72f, PAL_CART);
            spr(art_.wheel[fr], sx + 10.f, cy, rim * 0.72f, PAL_CART);
        }
    }

    {
        float sx = sxOf(1000);
        spr(art_.banner, sx, 96.f, 22.f, PAL_BANNER);
    }

    double crewX = (time_ / kClock) * kFinish;
    if (!title && mode_ != Mode::Win) spr(art_.rival, sxOf(crewX), syLat(0.78), 22.f, PAL_RIVAL, false, 2);

    float by = syLat(shipLat);
    spr(art_.barge, sxOf(shipX), by, title ? 34.f : 32.f, PAL_BOAT);

    int gf = int(t_ * 4.0) & 1;
    spr(art_.gull[gf], sxOf(shipX + 30), 70.f, 10.f, PAL_HUD, false, 2);

    if (title) {
        hudC(20, "LEFT FAR BANK    RIGHT NEAR BANK", PAL_HUD);
        hudC(21, "A POLES    THE CLOCK DOES NOT WAIT", PAL_AMBER);
        if ((sys_->frame / 30) % 2 == 0) hudC(24, "PRESS START", PAL_GOOD);
        hud(39 - int(std::strlen(S3_VERSION_STRING)), 27, S3_VERSION_STRING, PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        hudC(24, "START POLES    ESC TITLE", PAL_AMBER);
    } else if (mode_ == Mode::Fail) {
        hudC(22, whyBuf_, PAL_BAD);
        hudC(25, "START TRIES AGAIN", PAL_HUD);
    } else if (mode_ == Mode::Win) {
        char buf[40];
        std::snprintf(buf, sizeof buf, "1000 M   %.1f S", time_);
        hudC(22, buf, PAL_GOOD);
        hudC(24, "THE OTHER CREW IS STILL OUT", PAL_HUD);
    } else {
        char buf[64];
        int meters = int(std::clamp(std::floor(x_), 0.0, kFinish));
        std::snprintf(buf, sizeof buf, "%d/1000 M", meters);
        hud(1, 0, buf, PAL_HUD);
        double leftT = std::max(0.0, kClock - time_);
        int sec = int(leftT);
        std::snprintf(buf, sizeof buf, "CREW %d:%02d", sec / 60, sec % 60);
        hud(28, 0, buf, leftT < 12.0 ? PAL_BAD : PAL_AMBER);
        std::snprintf(buf, sizeof buf, "SPD %4.1f", v_);
        hud(1, 1, buf, PAL_HUD);
        int nw = nextWheel();
        const char* line = "HOLD THE CUT";
        int pal = PAL_GOOD;
        if (nw >= 0) {
            const WheelDef& w = kWheels[nw];
            int dist = int(std::max(0.0, w.x - x_));
            const char* side = (w.lat0 < 0.05) ? "NEAR BANK" : "FAR BANK";
            std::snprintf(buf, sizeof buf, "%s   %s   %d M", wheelWord(w.kind), side, dist);
            line = buf;
            pal = dist < 28 ? PAL_AMBER : PAL_HUD;
        } else if (x_ > 860.0) {
            line = "THE KILO IS AHEAD";
            pal = PAL_GOOD;
        }
        hudC(3, line, pal);
        hud(1, 26, "LEFT RIGHT   A POLE", PAL_HUD);
        hud(28, 26, "START PAUSE", PAL_HUD);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(6, 9, 11));
    sys.apu.setMaster(0.75f);
    sys.apu.setEcho(0.14f, 0.18f, 0.07f);
    sys.apu.setPatch(0, bellPatch());
    if (bot_) startRun();
    else showTitle();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    t_ += DT;
    if (shake_ > 0) shake_ = std::max(0.f, shake_ - float(DT));
    if (beep_ > 0.f) {
        beep_ -= float(DT);
        if (beep_ <= 0.f) sys.apu.tone(1, 0.f, 0.f);
    }
    if (chime_ >= 0) {
        static const float notes[] = {392.f, 494.f, 587.f, 784.f};
        chimeT_ += float(DT);
        if (chimeT_ > 0.16f) {
            if (chime_ < 4) sys.apu.keyOn(0, notes[chime_], 0.16f);
            else sys.apu.keyOff(0);
            chime_++;
            chimeT_ = 0;
            if (chime_ > 8) chime_ = -1;
        }
    }

    if (!bot_ && mode_ == Mode::Title) {
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
        if (pad.pressed(gs::BTN_START)) {
            blip(440.f);
            mode_ = Mode::Run;
        } else if (pad.pressed(gs::BTN_MODE)) showTitle();
        return;
    }
    if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        draw();
        sys.apu.noise(0.f, 500.f, false);
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
            if (mode_ == Mode::Fail) startRun();
            else showTitle();
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) showTitle();
        return;
    }

    double stick = 0;
    bool pole = false;
    if (bot_) pilot(stick, pole);
    else {
        if (pad.down(gs::BTN_LEFT)) stick -= 1;
        if (pad.down(gs::BTN_RIGHT)) stick += 1;
        if (std::fabs(pad.axisX) > 0.2f) stick = pad.axisX;
        stick = clampd(stick, -1.0, 1.0);
        pole = pad.down(gs::BTN_A) || pad.down(gs::BTN_C) || pad.accel > 0.2f;
        if (pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(300.f);
            draw();
            return;
        }
    }

    int nw = nextWheel();
    physics(stick, pole);
    if (mode_ == Mode::Run && nw != cue_) {
        cue_ = nw;
        if (nw >= 0) blip(kWheels[nw].kind == 0 ? 330.f : 554.f);
    }
    if (mode_ == Mode::Run) {
        float wash = float(std::clamp(v_ / 18.0, 0.1, 1.0)) * 0.035f;
        sys.apu.noise(wash, 320.f + float(v_) * 12.f, false);
        bool near = false;
        for (int i = 0; i < kWheelN; i++)
            if (std::fabs(kWheels[i].x - x_) < kWheels[i].half + 14.0) near = true;
        if (near) sys.setLight(40, 90, 140);
        else sys.setLight(20, 40, 70);
    }
    draw();
}

}  // namespace bkilo

#include "game/tram.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace tramkilo {
namespace {

constexpr double DT = 1.0 / 60.0;
constexpr double kFinish = 1000.0;
constexpr double kClock = 74.0;
constexpr double kScale = 3.6;
constexpr double kHull = 7.0;
constexpr double kBeam = 0.10;
constexpr double kGap = 0.12;
constexpr int kWheelN = 6;

struct WheelDef {
    double x;
    double half;
    double lat0, lat1;
    int kind;  // 0 lorry, 1 cab, 2 other tram
};

constexpr WheelDef kWheels[kWheelN] = {
    {160, 5.5, 0.00, 0.46, 0}, {310, 4.0, 0.55, 1.00, 1}, {450, 6.0, 0.00, 0.44, 2},
    {590, 3.8, 0.58, 1.00, 0}, {730, 5.8, 0.00, 0.48, 1}, {860, 4.2, 0.54, 1.00, 2},
};

double clampd(double v, double a, double b) { return std::max(a, std::min(b, v)); }

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

const char* wheelWord(int kind) {
    if (kind == 0) return "a lorry wheel";
    if (kind == 1) return "a cab wheel";
    return "a tram wheel";
}

int wheelPal(int kind) {
    if (kind == 0) return PAL_LORRY;
    if (kind == 1) return PAL_CAB;
    return PAL_OTHER;
}

gs::FMPatch bellPatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.08f;
    p.op[0] = {1.f, 1.f, 0.01f, 0.18f, 0.45f, 0.22f};
    p.op[1] = {2.01f, 0.2f, 0.02f, 0.22f, 0.25f, 0.18f};
    p.op[2] = {3.f, 0.05f, 0.02f, 0.24f, 0.12f, 0.2f};
    p.op[3] = {1.f, 0.f, 0.02f, 0.2f, 0.1f, 0.2f};
    p.vol = 0.22f;
    p.tone = 1400.f;
    return p;
}

float streetY(double lat) { return float(86.0 + lat * 112.0); }

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (x_ >= 780.0) return 3;
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
    x_ = 24;
    v_ = 0;
    lat_ = 0.46;
    latV_ = 0;
    camX_ = 0;
    shake_ = 0;
}

void Game::startRun() {
    x_ = 0;
    v_ = 13.4;
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
    camX_ = 0;
    blip(520.f);
}

void Game::pilot(double& stick, bool& pole) const {
    pole = true;
    double target = 0.50;
    int focus = -1;
    double focusD = 1e9;
    for (int i = 0; i < kWheelN; i++) {
        double ahead = kWheels[i].x - x_;
        if (ahead < -kWheels[i].half - 6.0 || ahead > 52.0) continue;
        if (ahead < focusD) {
            focusD = ahead;
            focus = i;
        }
    }
    if (focus >= 0) {
        const WheelDef& w = kWheels[focus];
        double below = w.lat0 - (kBeam + kGap);
        double above = w.lat1 + (kBeam + kGap);
        bool canBelow = below >= 0.18;
        bool canAbove = above <= 0.82;
        if (canBelow && canAbove) target = (std::fabs(lat_ - below) <= std::fabs(lat_ - above)) ? below : above;
        else if (canAbove) target = above;
        else target = std::min(0.82, std::max(0.18, below));
    }
    stick = clampd((target - lat_) * 2.6, -1.0, 1.0);
}

void Game::win() {
    if (mode_ != Mode::Run) return;
    won_ = true;
    over_ = true;
    mode_ = Mode::Win;
    chime_ = 0;
    chimeT_ = 0;
    x_ = std::max(x_, kFinish);
    sys_->rumble(0.12f, 0.04f, 120);
    sys_->setLight(40, 140, 70);
    std::printf("S3 TRAM KILO  CLEAR  finished the kilometer  1000 m  wheels untouched  (%.1f s)\n", time_);
    std::fflush(stdout);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    won_ = false;
    over_ = true;
    mode_ = Mode::Fail;
    std::snprintf(whyBuf_, sizeof whyBuf_, "%s", why);
    shake_ = 0.4f;
    int meters = int(std::clamp(x_, 0.0, kFinish));
    sys_->rumble(0.4f, 0.2f, 150);
    sys_->setLight(150, 36, 20);
    sys_->apu.noiseBurst(0.4f, 160.f, 0.28f);
    std::printf("S3 TRAM KILO  FAIL  %s at %d m\n", why, meters);
    std::fflush(stdout);
}

void Game::physics(double stick, bool pole) {
    stick = clampd(stick, -1.0, 1.0);
    time_ += DT;
    double want = pole ? 16.6 : 11.4;
    v_ += (want - v_) * std::min(1.0, 1.5 * DT);
    v_ = clampd(v_, 5.0, 18.0);
    x_ += v_ * DT;
    latV_ += (stick * 0.85 - latV_) * std::min(1.0, 7.5 * DT);
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
        fail("missed the end");
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
        if (lat_ < 0.28 || lat_ > 0.72) {
            fail("missed the end");
            return;
        }
        win();
        return;
    }
    if (time_ > kClock) fail("missed the end");
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.05f);
    beep_ = 0.07f;
}

void Game::sky() {
    uint16_t zen = gs::rgb4(3, 5, 10);
    uint16_t mid = gs::rgb4(6, 8, 12);
    uint16_t hor = gs::rgb4(11, 10, 8);
    if (mode_ == Mode::Fail) hor = lerpC(hor, gs::rgb4(12, 4, 3), 0.45f);
    if (mode_ == Mode::Win) hor = lerpC(hor, gs::rgb4(6, 12, 8), 0.4f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float t = y / float(gs::SCREEN_H - 1);
        sys_->vdp.lineBackdrop[y] = t < 0.42f ? lerpC(zen, mid, t / 0.42f) : lerpC(mid, hor, (t - 0.42f) / 0.58f);
        sys_->vdp.lineFog[y] = 0;
        sys_->vdp.road[y].on = false;
    }
    sys_->vdp.A.enabled = false;
    sys_->vdp.B.enabled = false;
    sys_->vdp.setFogColor(gs::rgb4(5, 6, 8));
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
    double tramX = title ? 40.0 + std::sin(t_ * 0.7) * 1.5 : x_;
    double tramLat = title ? 0.46 + std::sin(t_ * 0.9) * 0.03 : lat_;
    double cam = title ? 0.0 : camX_;
    float bob = title ? 0.f : float(std::sin(t_ * 18.0) * (v_ > 8.0 ? 1.0 : 0.2));
    if (shake_ > 0) bob += float(std::sin(t_ * 40.0) * shake_ * 6.0);

    auto worldX = [&](double wx) { return float((wx - cam) * kScale + 36.0); };

    for (int i = 0; i < 5; i++) {
        double bx = std::fmod(cam * 0.35 + i * 70.0, 360.0);
        spr(art_.cloud, float(bx), 28.f + float(i % 3) * 8.f, 16.f, PAL_WIRE, false, 2);
    }

    double base = std::floor(cam / 48.0) * 48.0;
    for (int i = -1; i < 9; i++) {
        double px = base + i * 48.0;
        spr(art_.pole, worldX(px), 118.f, 78.f, PAL_POLE);
        spr(art_.lamp, worldX(px) + 6.f, 86.f, 18.f, PAL_WIRE);
    }
    for (int i = 0; i < 6; i++) {
        double hx = 80.0 + i * 160.0;
        spr(art_.brick, worldX(hx), 100.f, 52.f, PAL_BRICK, (i & 1) != 0, 3);
    }

    for (int i = kWheelN - 1; i >= 0; i--) {
        const WheelDef& w = kWheels[i];
        float sx = worldX(w.x);
        float sy = streetY((w.lat0 + w.lat1) * 0.5);
        float ht = 36.f + float(w.half) * 1.4f;
        spr(art_.wheel[w.kind], sx, sy + bob * 0.2f, ht, wheelPal(w.kind));
    }

    float stopX = worldX(kFinish);
    spr(art_.stop, stopX, streetY(0.50) - 28.f, 46.f, PAL_STOP);

    spr(art_.tram, worldX(tramX), streetY(tramLat) + bob, title ? 52.f : 58.f, PAL_TRAM);

    if (title) {
        hudC(2, "TRAM KILO", PAL_HUD);
        hudC(4, "FINISH THE KILOMETER", PAL_AMBER);
        hudC(5, "DO NOT TOUCH A WHEEL", PAL_GOOD);
        hudC(7, "MISS THE END AND THE LEG FAILS", PAL_HUD);
        hudC(10, "LEFT RIGHT STEERS THE CAR", PAL_HUD);
        hudC(11, "HOLD A TO KEEP THE BELL", PAL_AMBER);
        hudC(20, "START", PAL_GOOD);
    } else if (mode_ == Mode::Pause) {
        hudC(10, "PAUSED", PAL_AMBER);
        hudC(12, "START ROLLS AGAIN", PAL_HUD);
    } else {
        int meters = int(std::clamp(title ? 0.0 : x_, 0.0, kFinish));
        char line[48];
        std::snprintf(line, sizeof line, "%d M", meters);
        hud(1, 1, line, PAL_HUD);
        int left = int(std::ceil(std::max(0.0, kClock - time_)));
        std::snprintf(line, sizeof line, "%d S", left);
        hud(34, 1, line, left < 12 ? PAL_BAD : PAL_AMBER);
        if (mode_ == Mode::Win) {
            hudC(4, "KILO CLEAR", PAL_GOOD);
            hudC(6, "WHEELS UNTOUCHED", PAL_HUD);
        } else if (mode_ == Mode::Fail) {
            hudC(4, "LEG FAILED", PAL_BAD);
            hudC(6, whyBuf_[0] ? whyBuf_ : "MISSED THE END", PAL_AMBER);
        } else {
            const char* hint = "THE TERMINUS IS AHEAD";
            int pal = PAL_GOOD;
            int nw = nextWheel();
            if (nw >= 0 && kWheels[nw].x - x_ < 40.0) {
                hint = "WHEEL ON THE LINE";
                pal = PAL_BAD;
            } else if (x_ > 780.0) {
                hint = "HOLD THE CENTRE FOR THE END";
                pal = PAL_AMBER;
            }
            hudC(3, hint, pal);
        }
        hud(1, 26, "LEFT RIGHT   A BELL", PAL_HUD);
        hud(28, 26, "START PAUSE", PAL_HUD);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(5, 6, 8));
    sys.apu.setMaster(0.75f);
    sys.apu.setEcho(0.12f, 0.16f, 0.06f);
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
        static const float notes[] = {523.f, 659.f, 784.f, 1046.f};
        chimeT_ += float(DT);
        if (chimeT_ > 0.16f) {
            if (chime_ < 4) sys.apu.keyOn(0, notes[chime_], 0.15f);
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
        sys.apu.noise(0.f, 400.f, false);
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
        if (pad.down(gs::BTN_LEFT) || pad.down(gs::BTN_UP)) stick -= 1;
        if (pad.down(gs::BTN_RIGHT) || pad.down(gs::BTN_DOWN)) stick += 1;
        if (std::fabs(pad.axisX) > 0.2f) stick = pad.axisX;
        if (std::fabs(pad.axisY) > 0.2f) stick = -pad.axisY;
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
    double prev = x_;
    physics(stick, pole);
    camX_ += (x_ - 28.0 - camX_) * std::min(1.0, 4.0 * DT);
    if (mode_ == Mode::Run && x_ > prev && nw != cue_) {
        cue_ = nw;
        if (nw >= 0) blip(kWheels[nw].kind == 2 ? 392.f : 294.f);
    }
    if (mode_ == Mode::Run) {
        float wash = float(std::clamp(v_ / 18.0, 0.08, 1.0)) * 0.03f;
        sys.apu.noise(wash, 240.f + float(v_) * 10.f, false);
        bool near = false;
        for (int i = 0; i < kWheelN; i++)
            if (std::fabs(kWheels[i].x - x_) < kWheels[i].half + 12.0) near = true;
        if (near) sys.setLight(90, 40, 30);
        else sys.setLight(30, 50, 80);
    }
    draw();
}

}  // namespace tramkilo

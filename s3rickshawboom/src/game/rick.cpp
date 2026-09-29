#include "rick.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace rickboom {
namespace {

constexpr double kDt = 1.0 / 60.0;
constexpr double kBoomL = 54.0;
constexpr double kBoomR = 66.5;
constexpr double kDriveOff = 3.15;
constexpr double kDriveHalf = 1.45;
constexpr double kMargin = 0.12;
constexpr double kEnd = 72.0;
constexpr double kStop = 0.30;
constexpr double kHoldNeed = 0.50;
constexpr double kOutNeed = 0.90;
constexpr double kBreak = 6.2;
constexpr double kClock = 26.0;
constexpr double kStartX = 9.0;
constexpr double kPedal = 11.2;
constexpr double kBrake = 17.5;
constexpr double kDrag = 2.3;
constexpr double kMax = 13.0;
constexpr float kZoom = 7.4f;
constexpr float kCabH = 58.f;

double clampd(double v, double a, double b) { return std::max(a, std::min(b, v)); }

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    return gs::rgb4(int(ar + (br - ar) * t), int(ag + (bg - ag) * t), int(ab + (bb - ab) * t));
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (hold_ > 0.08) return 3;
    if (driveOnBoom()) return 2;
    return 1;
}

double Game::driveC() const { return x_ + kDriveOff; }

bool Game::driveOnBoom() const {
    return driveC() - kDriveHalf >= kBoomL + kMargin && driveC() + kDriveHalf <= kBoomR - kMargin;
}

void Game::begin() {
    x_ = kStartX;
    speed_ = 0;
    pedalIn_ = 0;
    brakeIn_ = 0;
    hold_ = 0;
    outT_ = 0;
    race_ = 0;
    clock_ = kClock;
    roll_ = 0;
    phase_ = 0;
    pedal_ = 0;
    won_ = false;
    over_ = false;
    chimeN_ = 0;
    chimeStep_ = 0;
    why_[0] = 0;
    shake_ = 0;
}

void Game::showTitle() {
    begin();
    mode_ = Mode::Title;
    camX_ = 40.f;
}

void Game::startRun() {
    begin();
    mode_ = Mode::Run;
    camX_ = float(x_);
    blip(480.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.85f);
    sys.apu.setEcho(0.10f, 0.16f, 0.08f);
    t_ = 0;
    if (bot_) startRun();
    else showTitle();
}

void Game::controls() {
    const gs::Pad& p = sys_->pad;
    const bool go = p.down(gs::BTN_UP) || p.down(gs::BTN_RIGHT) || p.down(gs::BTN_C) || p.down(gs::BTN_A);
    const bool stop = p.down(gs::BTN_DOWN) || p.down(gs::BTN_LEFT) || p.down(gs::BTN_B) || p.down(gs::BTN_X);
    pedalIn_ = go ? 1.0 : 0.0;
    brakeIn_ = stop ? 1.0 : 0.0;
    if (p.accel > 0.05f) pedalIn_ = std::max(pedalIn_, double(p.accel));
    if (p.brake > 0.05f) brakeIn_ = std::max(brakeIn_, double(p.brake));
}

void Game::pilot() {
    const double aim = (kBoomL + kBoomR) * 0.5 - kDriveOff;
    const double err = aim - x_;
    const bool on = driveOnBoom();
    double want;
    if (!on && driveC() + kDriveHalf < kBoomR - 0.6) {
        if (err > 18.0) want = 8.6;
        else if (err > 8.0) want = 4.0;
        else want = std::max(0.55, err * 0.40);
    } else if (on) {
        want = clampd(err * 0.9, -0.35, 0.8);
        if (std::fabs(err) < 0.45) want = 0;
    } else {
        want = -1.3;
    }
    const double dv = want - speed_;
    pedalIn_ = dv > 0.12 ? clampd(dv / 2.2, 0.15, 1.0) : 0;
    brakeIn_ = dv < -0.10 ? clampd(-dv / 1.5, 0.2, 1.0) : 0;
    phase_ = on ? 1 : 0;
}

void Game::physics() {
    double a = pedalIn_ * kPedal - brakeIn_ * kBrake;
    if (std::fabs(speed_) > 0.02) a -= kDrag * (speed_ > 0 ? 1.0 : -1.0);
    else if (pedalIn_ < 0.05 && brakeIn_ < 0.05) speed_ = 0;
    speed_ += a * kDt;
    speed_ = clampd(speed_, -4.2, kMax);
    const double prev = driveC();
    x_ += speed_ * kDt;
    if (x_ < 3.5) {
        x_ = 3.5;
        if (speed_ < 0) speed_ = 0;
    }
    if (pedalIn_ > 0.2 && speed_ > 0.4) roll_ += speed_ * kDt;
    pedal_ = int(roll_ * 1.6) & 1;

    if (mode_ != Mode::Run) return;

    const bool crossed = prev + kDriveHalf < kBoomL && driveC() + kDriveHalf >= kBoomL;
    if (crossed && speed_ > kBreak) {
        fail("broke the boom");
        return;
    }
    if (driveC() > kEnd) {
        fail("rolled past the boom");
        return;
    }

    const bool on = driveOnBoom();
    const bool slow = std::fabs(speed_) <= kStop;
    if (on && slow) {
        hold_ += kDt;
        outT_ = 0;
        if (hold_ >= kHoldNeed) win();
    } else {
        hold_ = 0;
        if (slow && !on) {
            outT_ += kDt;
            if (outT_ >= kOutNeed)
                fail(driveC() < kBoomL ? "stopped short of the boom" : "stopped off the boom");
        } else {
            outT_ = 0;
        }
    }
    clock_ -= kDt;
    if (mode_ == Mode::Run && clock_ <= 0) {
        clock_ = 0;
        fail("the other crew took the drive");
    }
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.05f);
    tone1_ = 0.08f;
}

void Game::chime(int notes) {
    chimeN_ = std::clamp(notes, 1, 6);
    chimeStep_ = 0;
    chimeT_ = 0.02f;
}

void Game::win() {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    speed_ = 0;
    std::snprintf(why_, sizeof why_, "the drive is on the boom");
    chime(5);
    sys_->rumble(0.28f, 0.12f, 160);
    sys_->setLight(40, 170, 60);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    std::snprintf(why_, sizeof why_, "%s", why);
    shake_ = 1.f;
    sys_->rumble(0.55f, 0.25f, 180);
    sys_->setLight(170, 30, 20);
    sys_->apu.noiseBurst(0.4f, 120.f, 0.35f);
    sys_->apu.tone(0, 70.f, 0.05f);
    tone0_ = 0.4f;
}

void Game::audio() {
    if (tone0_ > 0) {
        tone0_ -= float(kDt);
        if (tone0_ <= 0) sys_->apu.tone(0, 0, 0);
    }
    if (tone1_ > 0) {
        tone1_ -= float(kDt);
        if (tone1_ <= 0) sys_->apu.tone(1, 0, 0);
    }
    if (chimeN_ > 0) {
        chimeT_ -= float(kDt);
        if (chimeT_ <= 0) {
            static const float notes[] = {523.f, 659.f, 784.f, 1046.f};
            sys_->apu.tone(1, notes[chimeStep_ % 4], 0.06f);
            tone1_ = 0.16f;
            chimeStep_++;
            if (chimeStep_ >= chimeN_) chimeN_ = 0;
            else chimeT_ = 0.14f;
        }
    }
    if (mode_ == Mode::Run && pedalIn_ > 0.4 && speed_ > 0.5) {
        float f = 78.f + float(speed_) * 7.f;
        sys_->apu.tone(0, f, 0.03f);
        tone0_ = 0.06f;
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    const gs::Pad& p = sys.pad;
    if (mode_ == Mode::Title) {
        if (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C) || bot_) startRun();
    } else if (mode_ == Mode::Pause) {
        if (p.pressed(gs::BTN_START)) mode_ = Mode::Run;
        if (p.pressed(gs::BTN_B)) showTitle();
    } else if (mode_ == Mode::Run) {
        if (!bot_ && p.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else {
            if (bot_) pilot();
            else controls();
            physics();
            race_ += kDt;
        }
    } else if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        if (!bot_ && (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A))) showTitle();
    }
    if (shake_ > 0) shake_ = std::max(0.f, shake_ - float(kDt) * 0.8f);
    float follow = float(mode_ == Mode::Title ? 42.0 : x_);
    camX_ += (follow - camX_) * (mode_ == Mode::Title ? 0.04f : 0.12f);
    audio();
    draw();
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool shadow) {
    if (m.h < 1 || h < 1.f) return;
    float sc = h / float(m.h);
    float w = float(m.w) * sc;
    if (cx + w * 0.5f < -8 || cx - w * 0.5f > gs::SCREEN_W + 8 || cy + h * 0.5f < -8 || cy - h * 0.5f > gs::SCREEN_H + 8)
        return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(std::lround(w), 1L, 400L));
    s.h = int16_t(std::clamp(std::lround(h), 1L, 300L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::sprBox(const gs::Mipped& m, float cx, float cy, float w, float h, int pal) {
    if (w < 1.f || h < 1.f) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(std::lround(w), 1L, 800L));
    s.h = int16_t(std::clamp(std::lround(h), 1L, 400L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(std::max(w, h));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::text(const char* s, float x, float y, float scale, int pal) {
    if (!s) return;
    const float adv = 6.f * scale;
    for (int i = 0; s[i]; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < 32 || c > 127) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        if (c != ' ') spr(g, x + i * adv + g.w * scale * 0.5f, y, std::max(7.f, g.h * scale), pal, false, false);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.A.enabled = false;
    v.B.enabled = false;
    v.HUD.clear();
    v.hudEnabled = true;

    float jx = 0;
    if (shake_ > 0) jx = std::sin(float(t_) * 50.f) * shake_ * 3.f;
    float cam = camX_ + jx / kZoom;

    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = float(y) / float(gs::SCREEN_H);
        uint16_t sky = lerpC(gs::rgb4(4, 3, 9), gs::rgb4(15, 9, 4), std::clamp((u - 0.12f) * 1.3f, 0.f, 1.f));
        if (y > 148) sky = lerpC(gs::rgb4(5, 4, 4), gs::rgb4(3, 3, 3), (y - 148) / 76.f);
        v.lineBackdrop[y] = sky;
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }

    auto wx = [&](double x) { return 160.f + float(x - cam) * kZoom; };

    char line[64];
    const int boomPal = hold_ > 0.05 ? PAL_WIN : PAL_BOOM;
    if (mode_ == Mode::Title) {
        spr(art_.title, 160.f, 22.f, float(art_.title.h), PAL_BANNER, false, false);
        spr(art_.clock, 160.f, 92.f, 34.f, PAL_CREW, false, false);
        text("THE CLOCK IS THE OTHER CREW", 28.f, 50.f, 1.f, PAL_HUD);
        text("PEDAL  A     BRAKE  B", 70.f, 186.f, 1.f, PAL_HUD);
        text("SET THE DRIVE ON THE BOOM", 40.f, 202.f, 1.f, PAL_BANNER);
    } else if (mode_ == Mode::Pause) {
        spr(art_.paused, 160.f, 86.f, float(art_.paused.h), PAL_BANNER, false, false);
    } else if (mode_ == Mode::Win) {
        spr(art_.delivered, 160.f, 22.f, float(art_.delivered.h), PAL_WIN, false, false);
        text("THE LEG IS MADE", 100.f, 46.f, 1.f, PAL_WIN);
    } else if (mode_ == Mode::Fail) {
        const gs::Mipped* b = &art_.offBoom;
        if (!std::strcmp(why_, "rolled past the boom")) b = &art_.missed;
        else if (!std::strcmp(why_, "stopped short of the boom")) b = &art_.shortB;
        else if (!std::strcmp(why_, "broke the boom")) b = &art_.broke;
        else if (!std::strcmp(why_, "the other crew took the drive")) b = &art_.crew;
        spr(*b, 160.f, 22.f, float(b->h), PAL_ALERT, false, false);
    }
    if (mode_ != Mode::Title) {
        std::snprintf(line, sizeof line, "CREW %4.1f", std::max(0.0, clock_));
        text(line, 46.f, 14.f, 1.f, clock_ < 6.0 ? PAL_ALERT : PAL_HUD);
        spr(art_.clock, 24.f, 16.f, 18.f, clock_ < 6.0 ? PAL_ALERT : PAL_CREW, false, false);
        std::snprintf(line, sizeof line, "SPD %4.1f", std::fabs(speed_));
        text(line, 210.f, 14.f, 1.f, PAL_HUD);
        if (driveOnBoom()) text("ON THE BOOM", 112.f, 32.f, 1.f, PAL_WIN);
        float frac = float(std::clamp(clock_ / kClock, 0.0, 1.0));
        sprBox(art_.stripe, 160.f, 28.f, 180.f, 5.f, PAL_HUD);
        sprBox(art_.hatch, 70.f + frac * 90.f, 28.f, std::max(2.f, frac * 180.f), 3.f, clock_ < 6.0 ? PAL_ALERT : PAL_WIN);
    }

    float cabY = 150.f + std::sin(float(t_) * 9.f) * (std::fabs(float(speed_)) > 0.4f ? 1.1f : 0.f);
    spr(art_.shade, wx(x_) + 8.f, cabY + 18.f, 14.f, PAL_CAB, false, true);
    spr(art_.cab[pedal_], wx(x_), cabY, kCabH, PAL_CAB, false, false);
    float bob = std::sin(float(t_) * 11.f) * (std::fabs(float(speed_)) > 0.3f ? 1.4f : 0.f);
    spr(art_.drive, wx(driveC()), cabY - 6.f + bob, 22.f, PAL_DRIVE, false, false);

    float boomCx = wx((kBoomL + kBoomR) * 0.5);
    float boomW = float(kBoomR - kBoomL) * kZoom;
    spr(art_.post, wx(kBoomL), 118.f, 48.f, boomPal, false, false);
    spr(art_.post, wx(kBoomR), 118.f, 48.f, boomPal, false, false);
    sprBox(art_.boom, boomCx, 96.f, boomW, 16.f, boomPal);
    spr(art_.hook, wx(driveOnBoom() ? driveC() : (kBoomL + kBoomR) * 0.5), 112.f, 20.f, boomPal, false, false);
    sprBox(art_.hatch, boomCx, 168.f, boomW * 0.72f, 18.f, boomPal);
    sprBox(art_.stripe, wx(kEnd), 164.f, 3.f, 48.f, PAL_ALERT);
    spr(art_.crate, wx(kBoomL - 3.2), 156.f, 16.f, PAL_STREET, false, false);
    spr(art_.crate, wx(kBoomR + 3.0), 158.f, 18.f, PAL_STREET, false, false);

    for (int i = 0; i < 14; i++) {
        double sx = i * 8.0;
        sprBox(art_.stripe, wx(sx), 198.f, 16.f, 4.f, PAL_HUD);
    }
    sprBox(art_.stripe, 160.f, 172.f, 360.f, 52.f, PAL_STREET);

    for (int i = -1; i < 9; i++) {
        double bx = i * 13.5;
        spr(art_.stall, wx(bx), 70.f, 64.f, PAL_STREET, (i & 1) != 0, false);
        spr(art_.awning, wx(bx), 44.f, 16.f, PAL_DRIVE, false, false);
        if ((i & 1) == 0) spr(art_.lamp, wx(bx + 5.8), 88.f, 32.f, PAL_LAMP, false, false);
    }
}

}  // namespace rickboom

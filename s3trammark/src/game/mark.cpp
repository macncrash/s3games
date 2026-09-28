#include "mark.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace trammark {
namespace {

constexpr double kDt = 1.0 / 60.0;
constexpr double kTau = 6.283185307179586;
constexpr double kNose = 2.45;
constexpr double kSetY = 40.0;
constexpr double kMarkR = 0.62;
constexpr double kPaintR = 1.85;
constexpr double kSouth = 1.2;
constexpr double kMaxSpd = 8.6;
constexpr double kRev = 2.6;
constexpr double kStop = 0.28;
constexpr double kHoldNeed = 0.55;
constexpr double kOutNeed = 0.9;
constexpr double kStartY = 6.0;
constexpr float kPlayZoom = 14.0f;

double clampd(double v, double a, double b) { return std::max(a, std::min(b, v)); }

double railX(double y) { return 1.35 * std::sin(y * 0.085); }

double railSlope(double y) { return 1.35 * 0.085 * std::cos(y * 0.085); }

double headAt(double y) { return std::atan2(railSlope(y), 1.0); }

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (hold_ > 0.05) return 3;
    if (onMark_) return 2;
    return 1;
}

int Game::tramFrame() const {
    double u = std::fmod(heading_, kTau);
    if (u < 0) u += kTau;
    int i = int(std::lround(u / kTau * 16.0)) % 16;
    if (i < 0) i += 16;
    return i;
}

void Game::measure() {
    heading_ = headAt(y_);
    x_ = railX(y_);
    nx_ = x_ + std::sin(heading_) * kNose;
    ny_ = y_ + std::cos(heading_) * kNose;
    noseDist_ = std::hypot(nx_ - markX_, ny_ - markY_);
    onMark_ = noseDist_ <= kMarkR;
    if (hold_ > 0.05) phase_ = 3;
    else if (onMark_) phase_ = 2;
    else if (noseDist_ < 14.0) phase_ = 1;
    else phase_ = 0;
}

const char* Game::stopWhy() const {
    if (noseDist_ <= kPaintR) return "close to the mark is still off it";
    if (ny_ < markY_) return "stopped short of the mark";
    if (ny_ > markY_) return "stopped long of the mark";
    return "stopped wide of the mark";
}

const char* Game::hint() const {
    if (hold_ > 0.05) return "HOLD THE SET";
    if (onMark_) return "COUPLER IS ON IT  STOP AND HOLD";
    if (noseDist_ <= kPaintR) return "CLOSE IS STILL OFF THE MARK";
    if (ny_ > markY_ + 0.5) return "LONG  BACK BEFORE THE BUFFER";
    if (noseDist_ < 16.0) return "SET THE COUPLER ON THE MARK";
    return "THE MARK IS UP THE RAILS";
}

void Game::begin() {
    double h = headAt(kSetY);
    markX_ = railX(kSetY) + std::sin(h) * kNose;
    markY_ = kSetY + std::cos(h) * kNose;
    endY_ = markY_ + 6.4;
    y_ = kStartY;
    heading_ = headAt(y_);
    x_ = railX(y_);
    speed_ = 0;
    throttle_ = 0;
    hold_ = 0;
    outT_ = 0;
    stuckT_ = 0;
    race_ = 0;
    phase_ = 0;
    won_ = false;
    over_ = false;
    announced_ = false;
    chimeN_ = 0;
    chimeStep_ = 0;
    shake_ = 0;
    why_[0] = 0;
    prevNose_ = 1.0e9;
    stuckY_ = y_;
    measure();
}

void Game::showTitle() {
    begin();
    mode_ = Mode::Title;
    camX_ = float(markX_);
    camY_ = float(markY_ - 2.0);
    zoom_ = 11.4f;
}

void Game::startRun() {
    begin();
    mode_ = Mode::Run;
    camX_ = float(x_);
    camY_ = float(y_);
    zoom_ = kPlayZoom;
    blip(520.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.82f);
    sys.apu.setEcho(0.12f, 0.14f, 0.06f);
    t_ = 0;
    if (bot_) startRun();
    else showTitle();
}

void Game::controls() {
    const gs::Pad& p = sys_->pad;
    const bool gas = p.down(gs::BTN_UP) || p.down(gs::BTN_C) || p.down(gs::BTN_A) || p.axisY > 0.25f;
    const bool brake = p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.down(gs::BTN_X) || p.axisY < -0.25f;
    if (gas && !brake) throttle_ = std::min(1.0, throttle_ + kDt * 1.5);
    else if (brake && !gas) throttle_ = std::max(-1.0, throttle_ - kDt * 1.9);
    else throttle_ *= 0.9;
    if (p.accel > 0.08f) throttle_ = std::min(1.0, throttle_ + p.accel * kDt * 2.0);
    if (p.brake > 0.08f) throttle_ = std::max(-1.0, throttle_ - p.brake * kDt * 2.2);
}

void Game::pilot() {
    double dx = markX_ - nx_;
    double dy = markY_ - ny_;
    double dist = std::hypot(dx, dy);
    double along = dx * std::sin(heading_) + dy * std::cos(heading_);

    if (ny_ > endY_ - 3.2 && speed_ > 0.12) {
        throttle_ = -1.0;
        return;
    }

    double want;
    if (dist <= kMarkR) {
        want = clampd(along * 2.4, -0.65, 0.65);
        if (std::fabs(along) < 0.07) want = 0;
    } else if (dist < 10.0) {
        want = clampd(along * 0.62, -1.7, 2.5);
    } else if (along > 0) {
        want = dist > 20.0 ? 7.4 : 4.2;
    } else {
        want = -1.5;
    }

    if (want > 0.05) {
        if (speed_ < want - 0.12) throttle_ = 1.0;
        else if (speed_ > want + 0.2) throttle_ = -0.9;
        else throttle_ = want / kMaxSpd;
    } else if (want < -0.05) {
        if (speed_ > want + 0.08) throttle_ = -1.0;
        else throttle_ = -0.3;
    } else {
        throttle_ = speed_ > 0.1 ? -0.75 : (speed_ < -0.1 ? 0.4 : 0.0);
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
    std::snprintf(why_, sizeof why_, "set down on the mark");
    chime(5);
    sys_->rumble(0.3f, 0.12f, 160);
    sys_->setLight(40, 180, 70);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    std::snprintf(why_, sizeof why_, "%s", why);
    shake_ = 1.f;
    sys_->rumble(0.55f, 0.25f, 180);
    sys_->setLight(180, 36, 24);
    sys_->apu.noiseBurst(0.42f, 140.f, 0.36f);
    sys_->apu.tone(0, 72.f, 0.06f);
    tone0_ = 0.4f;
}

void Game::physics() {
    double accel = throttle_ >= 0 ? throttle_ * 7.2 : throttle_ * 12.4;
    speed_ += accel * kDt;
    speed_ -= speed_ * 0.62 * kDt;
    speed_ = clampd(speed_, -kRev, kMaxSpd);
    if (std::fabs(throttle_) < 0.04 && std::fabs(speed_) < 0.16) speed_ *= 0.78;

    double h = headAt(y_);
    y_ += std::cos(h) * speed_ * kDt;
    if (!std::isfinite(y_) || !std::isfinite(speed_)) {
        fail("missed the end");
        return;
    }
    if (y_ < kSouth) {
        y_ = kSouth;
        if (speed_ < 0) speed_ = 0;
    }

    measure();
    if (ny_ > endY_) {
        fail("missed the end");
        return;
    }

    if (onMark_ && std::fabs(speed_) < 0.85 && std::fabs(throttle_) < 0.25) {
        speed_ *= 0.82;
        measure();
    }

    bool closing = noseDist_ < prevNose_ - 0.002;
    prevNose_ = noseDist_;
    if (onMark_ && std::fabs(speed_) < kStop) {
        outT_ = 0;
        hold_ += kDt;
        if (hold_ >= kHoldNeed) {
            win();
            return;
        }
    } else if (std::fabs(speed_) < kStop && !closing) {
        hold_ = 0;
        outT_ += kDt;
        if (outT_ >= kOutNeed) {
            fail(stopWhy());
            return;
        }
    } else {
        hold_ = 0;
        if (std::fabs(speed_) >= kStop || closing) outT_ = std::max(0.0, outT_ - kDt);
    }
    measure();

    if (onMark_ && !announced_) {
        announced_ = true;
        blip(740.f);
    }

    if (bot_ && !onMark_) {
        stuckT_ += kDt;
        if (stuckT_ > 2.0) {
            double moved = std::fabs(y_ - stuckY_);
            stuckY_ = y_;
            stuckT_ = 0;
            if (moved < 0.35 && ny_ < endY_ - 4.0) speed_ = ny_ < markY_ ? 1.6 : -1.2;
        }
    }
}

void Game::audio() {
    if (mode_ == Mode::Run && std::fabs(speed_) > 0.35) {
        float wob = 0.8f + 0.2f * std::sin(float(t_) * (6.f + float(std::fabs(speed_)) * 0.55f));
        float vol = (0.014f + float(std::fabs(throttle_)) * 0.03f) * wob;
        float f = 55.f + float(std::fabs(speed_)) * 5.5f;
        sys_->apu.tone(2, f, vol);
        sys_->apu.noise(0.01f + float(std::fabs(speed_)) * 0.0016f, 1400.f, true);
    } else {
        sys_->apu.tone(2, 0.f, 0.f);
        sys_->apu.noise(0.f, 400.f, false);
    }
    if (tone0_ > 0) {
        tone0_ -= float(kDt);
        if (tone0_ <= 0) sys_->apu.tone(0, 0.f, 0.f);
    }
    if (tone1_ > 0) {
        tone1_ -= float(kDt);
        if (tone1_ <= 0) sys_->apu.tone(1, 0.f, 0.f);
    }
    if (chimeN_ > 0) {
        chimeT_ -= float(kDt);
        if (chimeT_ <= 0) {
            static const float notes[] = {330.f, 440.f, 554.f, 659.f, 880.f};
            int n = std::min(chimeStep_, 4);
            sys_->apu.tone(0, notes[n], 0.05f);
            tone0_ = 0.16f;
            chimeT_ = 0.13f;
            if (++chimeStep_ >= chimeN_) chimeN_ = 0;
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    if (shake_ > 0) shake_ = std::max(0.f, shake_ - float(kDt) * 1.6f);
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) startRun();
        else if (pad.pressed(gs::BTN_MODE)) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        }
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(400.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        } else {
            race_ += kDt;
            if (bot_) pilot();
            else controls();
            physics();
            if (mode_ == Mode::Run && race_ > 52.0) fail("the leg ran out");
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) mode_ = Mode::Run;
        else if (pad.pressed(gs::BTN_MODE)) showTitle();
    } else if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C))) {
        startRun();
    } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
        showTitle();
    }
    if (mode_ == Mode::Win) sys.setLight(40, 180, 70);
    else if (mode_ == Mode::Fail) sys.setLight(180, 36, 24);
    else if (hold_ > 0.02) sys.setLight(200, 170, 40);
    else if (mode_ == Mode::Run) sys.setLight(90, 30, 28);
    camera();
    audio();
    draw();
}

void Game::camera() {
    float tx = float(x_);
    float ty = float(y_ + 2.6);
    if (mode_ == Mode::Title) {
        tx = float(markX_);
        ty = float(markY_ - 1.2);
    }
    float z = mode_ == Mode::Title ? 11.4f : kPlayZoom;
    camX_ += (tx - camX_) * 0.12f;
    camY_ += (ty - camY_) * 0.12f;
    zoom_ += (z - zoom_) * 0.08f;
    if (shake_ > 0) {
        camX_ += std::sin(float(t_) * 47.f) * shake_ * 2.2f;
        camY_ += std::cos(float(t_) * 39.f) * shake_ * 1.4f;
    }
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c < 32 || c > 127 || c == ' ') continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = 0;
    if (s)
        while (s[n]) n++;
    hud(20 - n / 2, row, s, pal);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    if (cx + w * 0.5f < -8 || cy + h * 0.5f < -8 || cx - w * 0.5f > gs::SCREEN_W + 8 || cy - h * 0.5f > gs::SCREEN_H + 8)
        return;
    gs::Sprite s;
    long sw = std::clamp(std::lround(w), 1L, 1800L);
    long sh = std::clamp(std::lround(h), 1L, 1800L);
    s.w = int16_t(sw);
    s.h = int16_t(sh);
    s.x = int16_t(std::clamp(std::lround(cx - sw * 0.5f), -2000L, 2000L));
    s.y = int16_t(std::clamp(std::lround(cy - sh * 0.5f), -2000L, 2000L));
    s.img = m.pick(float(sh));
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::sprBox(const gs::Mipped& m, float cx, float cy, float w, float h, int pal) {
    if (w < 1.f || h < 1.f || m.h < 1) return;
    if (cx + w * 0.5f < -4 || cy + h * 0.5f < -4 || cx - w * 0.5f > gs::SCREEN_W + 4 || cy - h * 0.5f > gs::SCREEN_H + 4)
        return;
    gs::Sprite s;
    long sw = std::clamp(std::lround(w), 1L, 1800L);
    long sh = std::clamp(std::lround(h), 1L, 1800L);
    s.w = int16_t(sw);
    s.h = int16_t(sh);
    s.x = int16_t(std::clamp(std::lround(cx - sw * 0.5f), -2000L, 2000L));
    s.y = int16_t(std::clamp(std::lround(cy - sh * 0.5f), -2000L, 2000L));
    s.img = m.pick(std::max(float(sw), float(sh)));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::place(const gs::Mipped& m, double wx, double wy, float worldH, int pal, bool flip) {
    float h = worldH * zoom_;
    float sx = 160.f + float(wx - camX_) * zoom_;
    float sy = 120.f - float(wy - camY_) * zoom_;
    spr(m, sx, sy, h, pal, flip);
}

void Game::worldRect(const gs::Mipped& m, double wx, double wy, double ww, double hh, int pal) {
    float sx = 160.f + float(wx - camX_) * zoom_;
    float sy = 120.f - float(wy - camY_) * zoom_;
    sprBox(m, sx, sy, float(ww) * zoom_, float(hh) * zoom_, pal);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.hudEnabled = true;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineBackdrop[y] = gs::rgb4(4, 6, 4);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }

    worldRect(art_.ballast, 0.2, 28, 7.2, 62, PAL_BALLAST);
    for (int i = 0; i < 40; i++) {
        double yy = 0.6 + i * 1.45;
        double xx = railX(yy);
        worldRect(art_.sleeper, xx, yy, 2.15, 0.28, PAL_RAIL);
        worldRect(art_.rail, xx - 0.62, yy, 0.14, 1.5, PAL_RAIL);
        worldRect(art_.rail, xx + 0.62, yy, 0.14, 1.5, PAL_RAIL);
    }
    for (int i = 0; i < 8; i++) {
        double yy = 34.0 + i * 1.6;
        double xx = railX(yy) + 2.55;
        worldRect(art_.plat, xx, yy, 2.2, 1.55, PAL_TOWN);
    }
    worldRect(art_.buffer, railX(endY_), endY_ + 0.35, 2.6, 0.85, PAL_END);
    worldRect(art_.ring, markX_, markY_, kPaintR * 2.0, kPaintR * 2.0, PAL_MARK);
    place(art_.mark, markX_, markY_, 1.55f, PAL_MARK);

    for (int i = 0; i < 7; i++) {
        double yy = i * 8.2;
        int k = i % 3;
        place(art_.block[k], railX(yy) - 6.6, yy, 5.6f, PAL_TOWN, i & 1);
        place(art_.block[(k + 1) % 3], railX(yy) + 6.8, yy + 3.0, 5.8f, PAL_TOWN, !(i & 1));
        place(art_.pole, railX(yy) - 2.4, yy + 1.4, 3.4f, PAL_WIRE);
        if ((i % 2) == 0) place(art_.tree, railX(yy) + 4.6, yy + 5.0, 2.4f, PAL_WIRE);
    }

    place(art_.shade, x_, y_ - 0.4, 2.3f, PAL_BALLAST);
    place(art_.tram[tramFrame()], x_, y_, 5.2f, PAL_TRAM);

    char line[48];
    if (mode_ == Mode::Title) {
        spr(art_.title, 160, 36, 28, PAL_MARK);
        hudC(16, "SET DOWN ON THE MARK", PAL_HUD);
        hudC(18, "MISSING THE END FAILS THE LEG", PAL_HUD);
        hudC(22, "START", PAL_HUD);
        spr(art_.start, 160, 188, 16, PAL_WIN);
    } else if (mode_ == Mode::Pause) {
        hudC(12, "PAUSED", PAL_HUD);
    } else if (mode_ == Mode::Win) {
        spr(art_.set, 160, 28, 18, PAL_WIN);
        hudC(8, "THE LEG IS MADE", PAL_WIN);
    } else if (mode_ == Mode::Fail) {
        bool missed = why_[0] == 'm';
        spr(missed ? art_.missed : art_.set, 160, 26, 16, PAL_ALERT);
        hudC(8, why_, PAL_ALERT);
    } else {
        std::snprintf(line, sizeof line, "NOSE %4.1f M", noseDist_);
        hud(1, 1, line, onMark_ ? PAL_WIN : PAL_HUD);
        std::snprintf(line, sizeof line, "SPD %4.1f", std::fabs(speed_));
        hud(28, 1, line, PAL_HUD);
        hudC(25, hint(), hold_ > 0.02 ? PAL_WIN : PAL_HUD);
        if (hold_ > 0.05) spr(art_.hold, 160, 40, 16, PAL_WIN);
        std::snprintf(line, sizeof line, "LEG %4.1f", race_);
        hud(1, 26, line, PAL_HUD);
    }
}

}  // namespace trammark

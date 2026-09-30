#include "bus.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace busmark {
namespace {

constexpr double kDt = 1.0 / 60.0;
constexpr double kTau = 6.283185307179586;
constexpr double kNose = 3.55;
constexpr double kMarkX = 1.05;
constexpr double kMarkY = 38.0;
constexpr double kMarkR = 0.92;
constexpr double kPaintR = 2.45;
constexpr double kEnd = 48.2;
constexpr double kLaneL = -2.7;
constexpr double kLaneR = 3.15;
constexpr double kSouth = 0.55;
constexpr double kMaxSpd = 8.2;
constexpr double kRev = 2.5;
constexpr double kStop = 0.30;
constexpr double kHoldNeed = 0.50;
constexpr double kOutNeed = 1.05;
constexpr double kNoseTol = 0.34;
constexpr double kStartX = -0.15;
constexpr double kStartY = 5.2;
constexpr float kPlayZoom = 15.5f;

double wrap(double a) {
    while (a > 3.141592653589793) a -= kTau;
    while (a < -3.141592653589793) a += kTau;
    return a;
}

double clampd(double v, double a, double b) { return std::max(a, std::min(b, v)); }

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (hold_ > 0.05) return 3;
    if (onMark_) return 2;
    return 1;
}

int Game::busFrame() const {
    double u = std::fmod(heading_, kTau);
    if (u < 0) u += kTau;
    int i = int(std::lround(u / kTau * 16.0)) % 16;
    if (i < 0) i += 16;
    return i;
}

void Game::noseAt(double& nx, double& ny) const {
    nx = x_ + std::sin(heading_) * kNose;
    ny = y_ + std::cos(heading_) * kNose;
}

void Game::measure() {
    noseAt(nx_, ny_);
    noseDist_ = std::hypot(nx_ - kMarkX, ny_ - kMarkY);
    onMark_ = noseDist_ <= kMarkR;
    noseOk_ = std::fabs(wrap(heading_)) <= kNoseTol;
    if (hold_ > 0.05) phase_ = 3;
    else if (onMark_) phase_ = 2;
    else if (noseDist_ < 14.0) phase_ = 1;
    else phase_ = 0;
}

const char* Game::stopWhy() const {
    if (onMark_ && !noseOk_) return "nose is not set on the mark";
    if (noseDist_ <= kPaintR) return "close to the mark is still off it";
    double dx = nx_ - kMarkX;
    double dy = ny_ - kMarkY;
    if (std::fabs(dx) > std::fabs(dy) && std::fabs(dx) > 1.2) return "stopped wide of the mark";
    if (dy < 0) return "stopped short of the mark";
    return "stopped long of the mark";
}

const char* Game::hint() const {
    if (hold_ > 0.05) return "HOLD THE SET";
    if (onMark_ && !noseOk_) return "SQUARE THE BUS ON THE MARK";
    if (onMark_) return "FRONT IS ON IT  STOP AND HOLD";
    if (noseDist_ <= kPaintR) return "CLOSE IS STILL OFF THE MARK";
    if (ny_ > kMarkY + 0.5) return "LONG  BACK THE BUS BEFORE THE END";
    if (noseDist_ < 16.0) return "SET THE FRONT ON THE MARK";
    return "THE STOP MARK IS UP THE BLOCK";
}

void Game::begin() {
    x_ = kStartX;
    y_ = kStartY;
    heading_ = 0;
    speed_ = 0;
    throttle_ = 0;
    steer_ = 0;
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
    stuckX_ = x_;
    stuckY_ = y_;
    measure();
}

void Game::showTitle() {
    begin();
    mode_ = Mode::Title;
    camX_ = float(kMarkX);
    camY_ = float(kMarkY - 1.5);
    zoom_ = 12.4f;
}

void Game::startRun() {
    begin();
    mode_ = Mode::Run;
    camX_ = float(x_);
    camY_ = float(y_);
    zoom_ = kPlayZoom;
    blip(360.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.78f);
    sys.apu.setEcho(0.10f, 0.12f, 0.05f);
    t_ = 0;
    if (bot_) startRun();
    else showTitle();
}

void Game::controls() {
    const gs::Pad& p = sys_->pad;
    steer_ = 0;
    if (p.down(gs::BTN_LEFT)) steer_ -= 1;
    if (p.down(gs::BTN_RIGHT)) steer_ += 1;
    if (std::fabs(p.axisX) > 0.18f) steer_ = clampd(double(p.axisX), -1.0, 1.0);
    const bool gas = p.down(gs::BTN_UP) || p.down(gs::BTN_C) || p.down(gs::BTN_A) || p.axisY > 0.25f;
    const bool brake = p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.down(gs::BTN_X) || p.axisY < -0.25f;
    if (gas && !brake) throttle_ = std::min(1.0, throttle_ + kDt * 1.15);
    else if (brake && !gas) throttle_ = std::max(-1.0, throttle_ - kDt * 1.45);
    else throttle_ *= 0.90;
    if (p.accel > 0.08f) throttle_ = std::min(1.0, throttle_ + p.accel * kDt * 1.6);
    if (p.brake > 0.08f) throttle_ = std::max(-1.0, throttle_ - p.brake * kDt * 1.8);
}

void Game::pilot() {
    double dx = kMarkX - nx_;
    double dy = kMarkY - ny_;
    double dist = std::hypot(dx, dy);

    if (ny_ > kEnd - 6.5 && speed_ > 0.12) {
        steer_ = clampd(wrap(-heading_) / 0.32, -1.0, 1.0);
        throttle_ = -1.0;
        return;
    }

    double wantH;
    double wantSpd;
    if (dist <= kMarkR + 0.05) {
        wantH = 0;
        double along = dx * std::sin(heading_) + dy * std::cos(heading_);
        wantSpd = clampd(along * 1.4, -0.7, 0.7);
        if (std::fabs(wrap(heading_)) < kNoseTol && std::fabs(along) < 0.10) wantSpd = 0;
    } else if (dist < 8.0) {
        wantH = std::atan2(dx, dy);
        wantSpd = clampd(dist * 0.38, 0.7, 2.2);
        if (ny_ > kMarkY + 0.35) {
            wantH = 0;
            wantSpd = -1.15;
        }
    } else {
        double ax = y_ > kMarkY - 14.0 ? kMarkX : kMarkX * 0.2;
        double ay = y_ > kMarkY - 14.0 ? kMarkY : kMarkY - 7.0;
        double adx = ax - x_;
        double ady = ay - y_;
        wantH = std::atan2(adx, ady);
        double d = std::hypot(adx, ady);
        wantSpd = d > 18.0 ? 6.6 : 3.6;
    }

    double err = wrap(wantH - heading_);
    steer_ = clampd(err / 0.38, -1.0, 1.0);
    if (dist < 3.5) steer_ *= 0.65;

    if (wantSpd > 0.05) {
        if (speed_ < wantSpd - 0.18) throttle_ = 1.0;
        else if (speed_ > wantSpd + 0.22) throttle_ = -0.8;
        else throttle_ = wantSpd / kMaxSpd;
    } else if (wantSpd < -0.05) {
        if (speed_ > wantSpd + 0.08) throttle_ = -1.0;
        else throttle_ = -0.3;
    } else {
        throttle_ = speed_ > 0.10 ? -0.75 : (speed_ < -0.10 ? 0.4 : 0.0);
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
    sys_->rumble(0.28f, 0.10f, 180);
    sys_->setLight(40, 170, 60);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    std::snprintf(why_, sizeof why_, "%s", why);
    shake_ = 1.f;
    sys_->rumble(0.5f, 0.22f, 180);
    sys_->setLight(170, 30, 20);
    sys_->apu.noiseBurst(0.4f, 120.f, 0.34f);
    sys_->apu.tone(0, 64.f, 0.06f);
    tone0_ = 0.4f;
}

void Game::physics() {
    double turn = steer_ * 1.35 * kDt * std::min(1.0, 0.22 + std::fabs(speed_) * 0.16);
    if (speed_ < -0.05) turn = -turn;
    heading_ = wrap(heading_ + turn);

    double accel = throttle_ >= 0 ? throttle_ * 5.4 : throttle_ * 9.2;
    speed_ += accel * kDt;
    speed_ -= speed_ * 0.55 * kDt;
    speed_ = clampd(speed_, -kRev, kMaxSpd);
    if (std::fabs(throttle_) < 0.04 && std::fabs(speed_) < 0.16) speed_ *= 0.78;

    x_ += std::sin(heading_) * speed_ * kDt;
    y_ += std::cos(heading_) * speed_ * kDt;
    if (!std::isfinite(x_) || !std::isfinite(y_) || !std::isfinite(speed_)) {
        fail("missed the end");
        return;
    }

    if (x_ < kLaneL) {
        x_ = kLaneL;
        speed_ *= 0.55;
    }
    if (x_ > kLaneR) {
        x_ = kLaneR;
        speed_ *= 0.55;
    }
    if (y_ < kSouth) {
        y_ = kSouth;
        if (std::cos(heading_) * speed_ < 0) speed_ *= 0.35;
    }

    measure();
    if (ny_ > kEnd) {
        fail("missed the end");
        return;
    }

    if (onMark_ && noseOk_ && std::fabs(speed_) < 0.85 && std::fabs(throttle_) < 0.25) {
        speed_ *= 0.82;
        heading_ = wrap(heading_ * (1.0 - std::min(1.0, 1.8 * kDt)));
        measure();
    }

    bool closing = noseDist_ < prevNose_ - 0.002;
    prevNose_ = noseDist_;
    bool set = onMark_ && noseOk_;
    if (set && std::fabs(speed_) < kStop) {
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
        blip(520.f);
    }

    if (bot_ && !onMark_) {
        stuckT_ += kDt;
        if (stuckT_ > 2.0) {
            double moved = std::hypot(x_ - stuckX_, y_ - stuckY_);
            stuckX_ = x_;
            stuckY_ = y_;
            stuckT_ = 0;
            if (moved < 0.55 && ny_ < kEnd - 6.0) heading_ = std::atan2(kMarkX - nx_, kMarkY - ny_);
        }
    }
}

void Game::audio() {
    if (mode_ == Mode::Run && std::fabs(speed_) > 0.35) {
        float wob = 0.8f + 0.2f * std::sin(float(t_) * (5.5f + float(std::fabs(speed_)) * 0.55f));
        float vol = (0.016f + float(std::fabs(throttle_)) * 0.03f) * wob;
        float f = 36.f + float(std::fabs(speed_)) * 3.4f;
        sys_->apu.tone(2, f, vol);
        sys_->apu.noise(0.01f + float(std::fabs(speed_)) * 0.0014f, 700.f, false);
    } else {
        sys_->apu.tone(2, 0.f, 0.f);
        sys_->apu.noise(0.f, 360.f, false);
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
            static const float notes[] = {349.f, 440.f, 523.3f, 698.f, 880.f};
            int n = std::min(chimeStep_, 4);
            sys_->apu.tone(0, notes[n], 0.05f);
            tone0_ = 0.16f;
            chimeT_ = 0.14f;
            if (++chimeStep_ >= chimeN_) chimeN_ = 0;
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    if (shake_ > 0) shake_ = std::max(0.f, shake_ - float(kDt) * 1.5f);
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
            blip(280.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        } else {
            race_ += kDt;
            if (bot_) pilot();
            else controls();
            physics();
            if (mode_ == Mode::Run && race_ > 62.0) fail("the leg ran out");
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) mode_ = Mode::Run;
        else if (pad.pressed(gs::BTN_MODE)) showTitle();
    } else if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C))) {
        startRun();
    } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
        showTitle();
    }
    if (mode_ == Mode::Win) sys.setLight(40, 170, 60);
    else if (mode_ == Mode::Fail) sys.setLight(170, 30, 20);
    else if (hold_ > 0.02) sys.setLight(190, 160, 36);
    else if (mode_ == Mode::Run) sys.setLight(30, 60, 130);
    camera();
    audio();
    draw();
}

void Game::camera() {
    float tx = float(x_);
    float ty = float(y_ + 2.4);
    if (mode_ == Mode::Title) {
        tx = float(kMarkX);
        ty = float(kMarkY - 0.6);
    }
    float z = mode_ == Mode::Title ? 12.4f : kPlayZoom;
    camX_ += (tx - camX_) * 0.10f;
    camY_ += (ty - camY_) * 0.10f;
    zoom_ += (z - zoom_) * 0.08f;
    if (shake_ > 0) {
        camX_ += std::sin(float(t_) * 41.f) * shake_ * 2.2f;
        camY_ += std::cos(float(t_) * 33.f) * shake_ * 1.4f;
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
        v.lineBackdrop[y] = gs::rgb4(2, 4, 3);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }

    worldRect(art_.walk, 0.2, 26, 18, 62, PAL_WALK);
    worldRect(art_.asphalt, 0.15, 26, 7.2, 58, PAL_ROAD);
    for (int i = 0; i < 7; i++) {
        double yy = 3.0 + i * 7.0;
        worldRect(art_.asphalt, 0.15, yy, 0.16, 2.2, PAL_HUD);
    }
    worldRect(art_.endbar, 0.15, kEnd + 0.4, 7.2, 0.65, PAL_END);
    worldRect(art_.ring, kMarkX, kMarkY, 2.2, 3.6, PAL_MARK);
    place(art_.bar, kMarkX, kMarkY, 2.4f, PAL_MARK);
    place(art_.shelter, 4.55, kMarkY + 0.4, 3.2f, PAL_STOP);
    place(art_.bench, 4.15, kMarkY - 1.6, 1.1f, PAL_STOP);

    for (int i = 0; i < 7; i++) {
        double yy = 0.5 + i * 7.4;
        int k = i % 3;
        place(art_.block[k], -6.6, yy, 5.4f, PAL_BLOCK, i & 1);
        place(art_.block[(k + 1) % 3], 7.2, yy + 1.6, 5.6f, PAL_BLOCK, !(i & 1));
        if ((i % 2) == 0) place(art_.tree, -4.4, yy + 3.2, 2.4f, PAL_STOP);
    }

    place(art_.shade, x_, y_ - 0.4, 2.6f, PAL_ROAD);
    place(art_.bus[busFrame()], x_, y_, 7.4f, PAL_BUS);

    char line[48];
    if (mode_ == Mode::Title) {
        spr(art_.title, 160, 34, 26, PAL_MARK);
        hudC(16, "SET DOWN ON THE MARK", PAL_HUD);
        hudC(18, "MISSING THE END FAILS THE LEG", PAL_HUD);
        hudC(22, "START", PAL_HUD);
        spr(art_.start, 160, 190, 16, PAL_WIN);
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
        if (hold_ > 0.05) spr(art_.hold, 160, 38, 16, PAL_WIN);
        std::snprintf(line, sizeof line, "LEG %4.1f", race_);
        hud(1, 26, line, PAL_HUD);
    }
}

}  // namespace busmark

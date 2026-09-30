#include "keel.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace keelmark {
namespace {

constexpr double kDt = 1.0 / 60.0;
constexpr double kTau = 6.283185307179586;
constexpr double kBow = 2.35;
constexpr double kMarkX = 0.85;
constexpr double kMarkY = 38.0;
constexpr double kMarkR = 0.82;
constexpr double kPaintR = 2.4;
constexpr double kEnd = 50.5;
constexpr double kLane = 4.4;
constexpr double kSouth = 0.6;
constexpr double kMaxSpd = 8.4;
constexpr double kRev = 2.6;
constexpr double kStop = 0.34;
constexpr double kHoldNeed = 0.62;
constexpr double kOutNeed = 0.9;
constexpr double kBowTol = 0.36;
constexpr double kStartX = -0.2;
constexpr double kStartY = 6.0;
constexpr float kPlayZoom = 12.6f;

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

int Game::hullFrame() const {
    double u = std::fmod(heading_, kTau);
    if (u < 0) u += kTau;
    int i = int(std::lround(u / kTau * 16.0)) % 16;
    if (i < 0) i += 16;
    return i;
}

void Game::bowAt(double& bx, double& by) const {
    bx = x_ + std::sin(heading_) * kBow;
    by = y_ + std::cos(heading_) * kBow;
}

void Game::measure() {
    bowAt(bx_, by_);
    bowDist_ = std::hypot(bx_ - kMarkX, by_ - kMarkY);
    onMark_ = bowDist_ <= kMarkR;
    bowOk_ = std::fabs(wrap(heading_)) <= kBowTol;
    if (hold_ > 0.05) phase_ = 3;
    else if (onMark_) phase_ = 2;
    else if (bowDist_ < 16.0) phase_ = 1;
    else phase_ = 0;
}

const char* Game::stopWhy() const {
    if (onMark_ && !bowOk_) return "bow is not set on the mark";
    if (bowDist_ <= kPaintR) return "close to the mark is still off it";
    double dx = bx_ - kMarkX;
    double dy = by_ - kMarkY;
    if (std::fabs(dx) > std::fabs(dy) && std::fabs(dx) > 1.4) return "stopped wide of the mark";
    if (dy < 0) return "stopped short of the mark";
    return "stopped long of the mark";
}

const char* Game::hint() const {
    if (hold_ > 0.05) return "HOLD THE SET";
    if (onMark_ && !bowOk_) return "SQUARE THE BOW ON THE MARK";
    if (onMark_) return "BOW IS ON IT  SPILL AND HOLD";
    if (bowDist_ <= kPaintR) return "CLOSE IS STILL OFF THE MARK";
    if (by_ > kMarkY + 0.6) return "LONG  BACK DOWN BEFORE THE END";
    if (bowDist_ < 18.0) return "SET THE BOW ON THE MARK";
    return "THE MARK IS UP THE REACH";
}

void Game::begin() {
    x_ = kStartX;
    y_ = kStartY;
    heading_ = 0.04;
    speed_ = 0;
    sheet_ = 0;
    rudder_ = 0;
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
    prevBow_ = 1.0e9;
    stuckX_ = x_;
    stuckY_ = y_;
    measure();
}

void Game::showTitle() {
    begin();
    mode_ = Mode::Title;
    camX_ = float(kMarkX);
    camY_ = float(kMarkY - 2.0);
    zoom_ = 10.6f;
}

void Game::startRun() {
    begin();
    mode_ = Mode::Run;
    camX_ = float(x_);
    camY_ = float(y_);
    zoom_ = kPlayZoom;
    blip(420.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.8f);
    sys.apu.setEcho(0.18f, 0.16f, 0.08f);
    t_ = 0;
    if (bot_) startRun();
    else showTitle();
}

void Game::controls() {
    const gs::Pad& p = sys_->pad;
    rudder_ = 0;
    if (p.down(gs::BTN_LEFT)) rudder_ -= 1;
    if (p.down(gs::BTN_RIGHT)) rudder_ += 1;
    if (std::fabs(p.axisX) > 0.18f) rudder_ = clampd(double(p.axisX), -1.0, 1.0);
    const bool fill = p.down(gs::BTN_UP) || p.down(gs::BTN_C) || p.down(gs::BTN_A) || p.axisY > 0.25f;
    const bool spill = p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.down(gs::BTN_X) || p.axisY < -0.25f;
    if (fill && !spill) sheet_ = std::min(1.0, sheet_ + kDt * 1.5);
    else if (spill && !fill) sheet_ = std::max(-1.0, sheet_ - kDt * 1.7);
    else sheet_ *= 0.9;
    if (p.accel > 0.08f) sheet_ = std::min(1.0, sheet_ + p.accel * kDt * 1.8);
    if (p.brake > 0.08f) sheet_ = std::max(-1.0, sheet_ - p.brake * kDt * 2.0);
}

void Game::pilot() {
    double dx = kMarkX - bx_;
    double dy = kMarkY - by_;
    double dist = std::hypot(dx, dy);

    if (by_ > kEnd - 6.5 && speed_ > 0.12) {
        rudder_ = clampd(wrap(-heading_) / 0.3, -1.0, 1.0);
        sheet_ = -1.0;
        return;
    }

    double wantH;
    double wantSpd;
    if (dist <= kMarkR) {
        wantH = 0;
        double along = dx * std::sin(heading_) + dy * std::cos(heading_);
        wantSpd = clampd(along * 1.5, -0.85, 0.85);
        if (std::fabs(wrap(heading_)) < kBowTol && std::fabs(along) < 0.12) wantSpd = 0;
    } else if (dist < 10.0) {
        wantH = std::atan2(dx, dy);
        wantSpd = clampd(dist * 0.38, 0.7, 2.3);
        if (by_ > kMarkY + 0.45) {
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
        wantSpd = d > 20.0 ? 6.4 : 3.6;
    }

    double err = wrap(wantH - heading_);
    rudder_ = clampd(err / 0.32, -1.0, 1.0);
    if (dist < 4.0) rudder_ *= 0.72;

    if (wantSpd > 0.05) {
        if (speed_ < wantSpd - 0.15) sheet_ = 1.0;
        else if (speed_ > wantSpd + 0.22) sheet_ = -0.8;
        else sheet_ = wantSpd / kMaxSpd;
    } else if (wantSpd < -0.05) {
        if (speed_ > wantSpd + 0.1) sheet_ = -1.0;
        else sheet_ = -0.32;
    } else {
        sheet_ = speed_ > 0.12 ? -0.65 : (speed_ < -0.12 ? 0.4 : 0.0);
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
    sys_->rumble(0.25f, 0.1f, 150);
    sys_->setLight(30, 160, 90);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    std::snprintf(why_, sizeof why_, "%s", why);
    shake_ = 1.f;
    sys_->rumble(0.5f, 0.2f, 170);
    sys_->setLight(160, 40, 28);
    sys_->apu.noiseBurst(0.38f, 120.f, 0.34f);
    sys_->apu.tone(0, 70.f, 0.06f);
    tone0_ = 0.4f;
}

void Game::physics() {
    double turn = rudder_ * 1.85 * kDt * std::min(1.0, 0.32 + std::fabs(speed_) * 0.2);
    if (speed_ < -0.05) turn = -turn;
    heading_ = wrap(heading_ + turn);

    double drive = sheet_ >= 0 ? sheet_ * 6.6 : sheet_ * 10.5;
    speed_ += drive * kDt;
    speed_ -= speed_ * 0.55 * kDt;
    speed_ = clampd(speed_, -kRev, kMaxSpd);
    if (std::fabs(sheet_) < 0.04 && std::fabs(speed_) < 0.2) speed_ *= 0.8;

    x_ += std::sin(heading_) * speed_ * kDt;
    y_ += std::cos(heading_) * speed_ * kDt;
    if (!std::isfinite(x_) || !std::isfinite(y_) || !std::isfinite(speed_)) {
        fail("missed the end");
        return;
    }

    if (x_ < -kLane) {
        x_ = -kLane;
        speed_ *= 0.55;
        shake_ = std::max(shake_, 0.25f);
    }
    if (x_ > kLane) {
        x_ = kLane;
        speed_ *= 0.55;
        shake_ = std::max(shake_, 0.25f);
    }
    if (y_ < kSouth) {
        y_ = kSouth;
        if (std::cos(heading_) * speed_ < 0) speed_ *= 0.35;
    }

    measure();
    if (by_ > kEnd) {
        fail("missed the end");
        return;
    }

    if (onMark_ && bowOk_ && std::fabs(speed_) < 0.95 && std::fabs(sheet_) < 0.24) {
        speed_ *= 0.84;
        heading_ = wrap(heading_ * (1.0 - std::min(1.0, 1.5 * kDt)));
        measure();
    }

    bool closing = bowDist_ < prevBow_ - 0.003;
    prevBow_ = bowDist_;
    bool set = onMark_ && bowOk_;
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
        blip(620.f);
    }

    if (bot_ && !onMark_) {
        stuckT_ += kDt;
        if (stuckT_ > 2.2) {
            double moved = std::hypot(x_ - stuckX_, y_ - stuckY_);
            stuckX_ = x_;
            stuckY_ = y_;
            stuckT_ = 0;
            if (moved < 0.7 && by_ < kEnd - 8.0) heading_ = std::atan2(kMarkX - bx_, kMarkY - by_);
        }
    }
}

void Game::audio() {
    if (mode_ == Mode::Run && std::fabs(speed_) > 0.35) {
        float wob = 0.82f + 0.18f * std::sin(float(t_) * 5.5f);
        float vol = (0.01f + float(std::fabs(sheet_)) * 0.02f) * wob;
        sys_->apu.tone(2, 90.f + float(std::fabs(speed_)) * 6.f, vol * 0.35f);
        sys_->apu.noise(0.006f + float(std::fabs(speed_)) * 0.002f, 1400.f, false);
    } else {
        sys_->apu.tone(2, 0.f, 0.f);
        sys_->apu.noise(0.f, 500.f, false);
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
            static const float notes[] = {349.f, 440.f, 523.3f, 698.5f, 880.f};
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
            blip(360.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        } else {
            race_ += kDt;
            if (bot_) pilot();
            else controls();
            physics();
            if (mode_ == Mode::Run && race_ > 55.0) fail("the leg ran out");
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) mode_ = Mode::Run;
        else if (pad.pressed(gs::BTN_MODE)) showTitle();
    } else if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C))) {
        startRun();
    } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
        showTitle();
    }
    if (mode_ == Mode::Win) sys.setLight(30, 160, 90);
    else if (mode_ == Mode::Fail) sys.setLight(160, 40, 28);
    else if (hold_ > 0.02) sys.setLight(200, 160, 40);
    else if (mode_ == Mode::Run) sys.setLight(20, 60, 140);
    camera();
    audio();
    draw();
}

void Game::camera() {
    float tx = float(x_);
    float ty = float(y_ + 3.4);
    if (mode_ == Mode::Title) {
        tx = float(kMarkX);
        ty = float(kMarkY - 1.0);
    }
    float z = mode_ == Mode::Title ? 10.6f : kPlayZoom;
    camX_ += (tx - camX_) * 0.1f;
    camY_ += (ty - camY_) * 0.1f;
    zoom_ += (z - zoom_) * 0.08f;
    if (shake_ > 0) {
        camX_ += std::sin(float(t_) * 41.f) * shake_ * 2.0f;
        camY_ += std::cos(float(t_) * 33.f) * shake_ * 1.3f;
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
        float t = float(y) / float(gs::SCREEN_H);
        v.lineBackdrop[y] = gs::rgb4(1, 4 + int(t * 3), 8 + int(t * 3));
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }

    worldRect(art_.bank, 0, 28, 28, 72, PAL_BANK);
    worldRect(art_.water, 0, 26, 11.2, 64, PAL_WATER);
    for (int i = 0; i < 7; i++) {
        double yy = 1.5 + i * 8.0;
        worldRect(art_.water, -1.6, yy, 0.35, 3.2, PAL_WAKE);
        worldRect(art_.water, 1.7, yy + 3.0, 0.28, 2.4, PAL_WAKE);
    }
    worldRect(art_.endbar, 0, kEnd + 0.4, 11.2, 0.85, PAL_END);
    place(art_.committee, -4.6, kEnd + 0.2, 3.4f, PAL_REED);
    place(art_.committee, 5.2, kEnd - 0.4, 3.2f, PAL_REED, true);
    worldRect(art_.ring, kMarkX, kMarkY, kPaintR * 2.0, kPaintR * 2.0, PAL_MARK);
    place(art_.buoy, kMarkX, kMarkY, 2.2f, PAL_MARK);
    place(art_.flag, kMarkX + 0.15, kMarkY + 0.9, 1.3f, PAL_END);

    for (int i = 0; i < 9; i++) {
        double yy = -1.0 + i * 6.4;
        place(art_.reed, -6.4, yy, 2.1f, PAL_REED, i & 1);
        place(art_.reed, 6.6, yy + 2.2, 2.3f, PAL_REED, !(i & 1));
    }

    place(art_.shade, x_, y_ - 0.4, 2.4f, PAL_WAKE);
    place(art_.hull[hullFrame()], x_, y_, 5.1f, PAL_HULL);

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
        std::snprintf(line, sizeof line, "BOW %4.1f M", bowDist_);
        hud(1, 1, line, onMark_ ? PAL_WIN : PAL_HUD);
        std::snprintf(line, sizeof line, "SPD %4.1f", std::fabs(speed_));
        hud(28, 1, line, PAL_HUD);
        hudC(25, hint(), hold_ > 0.02 ? PAL_WIN : PAL_HUD);
        if (hold_ > 0.05) spr(art_.hold, 160, 40, 16, PAL_WIN);
        std::snprintf(line, sizeof line, "LEG %4.1f", race_);
        hud(1, 26, line, PAL_HUD);
    }
}

}  // namespace keelmark

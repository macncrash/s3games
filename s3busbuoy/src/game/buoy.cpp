#include "game/buoy.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace buoy {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float PI = 3.14159265f;
constexpr float TAU = 6.2831853f;
constexpr float WORLD_L = 36, WORLD_R = 764, WORLD_T = 48, WORLD_B = 700;
constexpr float MAX_SPD = 168;

struct Mark {
    float x, y;
    int pal;
    const char* name;
};

const Mark kMarks[3] = {
    {250, 450, PAL_RED, "RED"},
    {560, 300, PAL_GREEN, "GREEN"},
    {280, 180, PAL_GOLD, "GOLD"},
};

const float kRocks[][2] = {
    {90, 200}, {140, 120}, {620, 150}, {700, 240}, {680, 480}, {120, 560}, {80, 400}, {640, 600},
};

float wrap(float a) {
    while (a > PI) a -= TAU;
    while (a < -PI) a += TAU;
    return a;
}

float len(float x, float y) { return std::sqrt(x * x + y * y); }

}  // namespace

void Game::course() {
    // Each buoy is rounded to port: east gate, north gate, west gate (CCW).
    // Then the same dock the bus left.
    const Wp seq[] = {
        {340, 520, 0}, {340, 430, 0}, {250, 370, 0}, {160, 450, 0},
        {430, 360, 1}, {640, 300, 1}, {560, 210, 1}, {470, 280, 1},
        {400, 220, 2}, {280, 100, 2}, {180, 180, 2}, {250, 270, 2},
        {440, 480, -1}, {440, 610, -1},
    };
    nwp_ = int(sizeof(seq) / sizeof(seq[0]));
    for (int i = 0; i < nwp_; i++) wps_[i] = seq[i];
    for (int m = 0; m < 3; m++) {
        markWp_[m] = 0;
        for (int i = 0; i < nwp_; i++)
            if (wps_[i].mark == m) {
                markWp_[m] = i;
                break;
            }
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    course();
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    std::snprintf(line_, sizeof(line_), "");
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int d = y < 40 ? 0 : (y / 18);
        sys.vdp.lineBackdrop[y] = gs::rgb4(1, 4 + d / 4, 8 + d / 3);
        sys.vdp.lineFog[y] = 0;
        sys.vdp.road[y].on = false;
    }
    sys.vdp.A.enabled = false;
    sys.vdp.HUD.enabled = false;
    sys.vdp.hudEnabled = false;
    sys.apu.setMaster(0.5f);
}

void Game::begin() {
    mode_ = Mode::Run;
    x_ = 440;
    y_ = 620;
    heading_ = 0;
    speed_ = 0;
    camX_ = 440;
    camY_ = 540;
    rounded_ = 0;
    wp_ = 0;
    hold_ = 0;
    time_ = 0;
    bump_ = 0;
    flash_ = 0;
    over_ = false;
    won_ = false;
}

Game::In Game::human() {
    In in;
    const gs::Pad& p = sys_->pad;
    if (p.down(gs::BTN_LEFT)) in.steer -= 1;
    if (p.down(gs::BTN_RIGHT)) in.steer += 1;
    if (p.axisX) in.steer = p.axisX;
    if (in.steer > 1) in.steer = 1;
    if (in.steer < -1) in.steer = -1;
    if (p.down(gs::BTN_A) || p.down(gs::BTN_UP) || p.accel > 0.2f) in.throttle = p.accel > 0.2f ? p.accel : 1;
    if (p.down(gs::BTN_B) || p.down(gs::BTN_DOWN) || p.brake > 0.2f) in.brake = 1;
    return in;
}

Game::In Game::pilot() const {
    In in;
    if (wp_ >= nwp_) {
        in.brake = 1;
        return in;
    }
    float dx = wps_[wp_].x - x_;
    float dy = wps_[wp_].y - y_;
    float d = len(dx, dy);
    float want = std::atan2(dx, -dy);
    float err = wrap(want - heading_);
    in.steer = err * 1.7f;
    if (in.steer > 1) in.steer = 1;
    if (in.steer < -1) in.steer = -1;
    bool last = wp_ == nwp_ - 1;
    if (last) {
        if (d > 36) {
            in.throttle = std::fabs(err) < 0.7f ? 0.42f : 0.15f;
            in.brake = (std::fabs(err) > 0.9f && speed_ > 40) ? 1 : 0;
        } else {
            in.throttle = 0;
            in.brake = 1;
        }
    } else if (std::fabs(err) > 1.15f) {
        in.throttle = 0.25f;
        in.brake = speed_ > 70 ? 0.6f : 0;
    } else if (std::fabs(err) > 0.55f) {
        in.throttle = 0.55f;
    } else {
        in.throttle = 1;
    }
    return in;
}

Game::In Game::readIn() { return bot_ ? pilot() : human(); }

void Game::hitBuoy(int i) {
    float dx = x_ - kMarks[i].x;
    float dy = y_ - kMarks[i].y;
    float d = len(dx, dy);
    if (d < 1) d = 1;
    x_ += dx / d * 6;
    y_ += dy / d * 6;
    speed_ *= 0.35f;
    bump_ = 0.35f;
    // Cutting the mark undoes that rounding if it is the one in hand.
    if (wp_ < nwp_ && wps_[wp_].mark == i) wp_ = markWp_[i];
    if (rounded_ > i && wp_ < nwp_ && wps_[wp_].mark >= 0) {
        // already passed; a tap still shoves the bus but does not reopen the course
    }
    sys_->apu.noiseBurst(0.25f, 1400, 0.12f);
}

void Game::gates() {
    for (int i = 0; i < 3; i++) {
        float dx = x_ - kMarks[i].x;
        float dy = y_ - kMarks[i].y;
        if (dx * dx + dy * dy < 22 * 22) hitBuoy(i);
    }
    if (wp_ >= nwp_) return;
    float dx = x_ - wps_[wp_].x;
    float dy = y_ - wps_[wp_].y;
    if (dx * dx + dy * dy > 46 * 46) return;
    int mark = wps_[wp_].mark;
    wp_++;
    if (mark >= 0 && (wp_ >= nwp_ || wps_[wp_].mark != mark)) {
        rounded_++;
        flash_ = 0.8f;
        sys_->apu.tone(1, 520, 0.18f);
        sys_->apu.tone(2, 780, 0.12f);
    }
}

void Game::physics(const In& in) {
    float turn = 2.15f * in.steer * (0.35f + 0.65f * std::fabs(speed_) / MAX_SPD);
    heading_ = wrap(heading_ + turn * DT);
    if (in.throttle > 0) speed_ += 130 * in.throttle * DT;
    if (in.brake > 0) speed_ -= 220 * in.brake * DT;
    if (speed_ > MAX_SPD) speed_ = MAX_SPD;
    if (speed_ < -36) speed_ = -36;
    if (in.throttle == 0 && in.brake == 0) speed_ *= (1 - 0.55f * DT);
    if (std::fabs(speed_) < 1) speed_ = 0;
    x_ += std::sin(heading_) * speed_ * DT;
    y_ += -std::cos(heading_) * speed_ * DT;

    if (x_ < WORLD_L) {
        x_ = WORLD_L;
        speed_ *= 0.4f;
        bump_ = 0.2f;
    }
    if (x_ > WORLD_R) {
        x_ = WORLD_R;
        speed_ *= 0.4f;
        bump_ = 0.2f;
    }
    if (y_ < WORLD_T) {
        y_ = WORLD_T;
        speed_ *= 0.4f;
        bump_ = 0.2f;
    }
    if (y_ > WORLD_B) {
        y_ = WORLD_B;
        speed_ *= 0.4f;
        bump_ = 0.2f;
    }
    gates();

    bool docked = rounded_ >= 3 && x_ > 320 && x_ < 560 && y_ > 540 && y_ < 690 && std::fabs(speed_) < 40;
    if (docked) {
        if (++hold_ > 18) finish(true);
    } else {
        hold_ = 0;
    }
    time_ += DT;
    if (mode_ == Mode::Run && time_ > limit_) finish(false);
}

void Game::finish(bool win) {
    won_ = win;
    over_ = true;
    mode_ = win ? Mode::Win : Mode::Fail;
    if (win) {
        int score = int((limit_ - time_) * 10) + 300;
        if (score < 300) score = 300;
        std::snprintf(line_, sizeof(line_),
                      "S3 BUSBUOY  PASS  rounded %d  docked  score %d  (%.1f s)", rounded_, score, time_);
        sys_->apu.tone(0, 392, 0.2f);
        sys_->apu.tone(1, 523, 0.2f);
        sys_->apu.tone(2, 659, 0.2f);
    } else {
        std::snprintf(line_, sizeof(line_),
                      "S3 BUSBUOY  FAIL  rounded %d/3  still out  (%.1f s)", rounded_, time_);
        sys_->apu.tone(0, 110, 0.22f);
    }
}

void Game::audio() {
    if (mode_ != Mode::Run) {
        if (mode_ == Mode::Title) sys_->apu.tone(0, 0, 0);
        return;
    }
    float f = 48 + std::fabs(speed_) * 0.55f;
    sys_->apu.tone(0, f, speed_ > 4 ? 0.05f : 0);
    song_++;
    if ((song_ % 90) == 0) sys_->apu.tone(2, (song_ / 90) & 1 ? 196 : 247, 0.04f);
}

void Game::sprite(const gs::Image& img, float wx, float wy, int w, int h, int pal, bool flip) {
    float sx = wx - camX_ + 160;
    float sy = wy - camY_ + 118;
    if (sx < -w || sy < -h || sx > gs::SCREEN_W + 8 || sy > gs::SCREEN_H + 8) return;
    gs::Sprite s;
    s.img = img;
    s.x = int16_t(sx - w / 2);
    s.y = int16_t(sy - h / 2);
    s.w = int16_t(w);
    s.h = int16_t(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::text(int x, int y, const char* s, int pal) {
    for (int i = 0; s[i]; i++) {
        int c = s[i];
        if (c < 32 || c > 127) c = 32;
        gs::Sprite sp;
        sp.img = art_.font[c - 32];
        sp.x = int16_t(x);
        sp.y = int16_t(y);
        sp.w = 8;
        sp.h = 8;
        sp.pal = uint8_t(pal);
        sys_->vdp.sprite(sp);
        x += 6;
    }
}

void Game::center(int y, const char* s, int pal) {
    int n = int(std::strlen(s));
    text((gs::SCREEN_W - n * 6) / 2, y, s, pal);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    float scroll = time_ * 12 + (mode_ == Mode::Title ? sys_->frame * 0.4f : 0);
    for (int y = 0; y < gs::SCREEN_H; y++) v.B.hscroll[y] = int16_t(scroll + y * 0.2f);
    v.clearSprites();

    sprite(art_.dock, 440, 650, 168, 56, PAL_DOCK, false);
    sprite(art_.shed, 390, 628, 40, 28, PAL_SHED, false);
    for (auto& r : kRocks) sprite(art_.rock, r[0], r[1], 24, 16, PAL_ROCK, false);

    for (int i = 0; i < 3; i++) {
        int pal = kMarks[i].pal;
        bool done = i < rounded_;
        float bob = std::sin(time_ * 3 + i) * 2;
        sprite(art_.mark, kMarks[i].x, kMarks[i].y + bob, 16, 28, done ? PAL_OK : pal, false);
        if (!done && i == rounded_) sprite(art_.lamp, kMarks[i].x + 10, kMarks[i].y - 16, 8, 8, PAL_GOLD, false);
    }

    // Gates still ahead, so the rounding is visible.
    for (int i = wp_; i < nwp_ && i < wp_ + 1; i++) {
        if (wps_[i].mark >= 0) sprite(art_.lamp, wps_[i].x, wps_[i].y, 8, 8, PAL_WAKE, false);
    }

    float wx = x_ - std::sin(heading_) * 18;
    float wy = y_ + std::cos(heading_) * 18;
    sprite(art_.wake, wx, wy, 20, 10, PAL_WAKE, false);

    int face = int(std::floor((heading_ + PI / 8) / (PI / 4))) & 7;
    bool flip = face >= 5;
    int idx = face;
    if (face == 5) idx = 3;
    else if (face == 6) idx = 2;
    else if (face == 7) idx = 1;
    int bw = (idx == 0 || idx == 4) ? 32 : (idx == 2 ? 48 : 36);
    int bh = (idx == 2) ? 32 : (idx == 0 || idx == 4 ? 48 : 36);
    sprite(art_.bus[idx], x_, y_, bw, bh, PAL_BUS, flip);

    if (mode_ == Mode::Title) {
        center(48, "S3 BUSBUOY", PAL_HUD);
        center(68, "ROUND THE BUOYS", PAL_GOLD);
        center(84, "PORT SIDE  THEN THE SAME DOCK", PAL_HUD);
        center(150, "ARROWS STEER   A DRIVE   B BRAKE", PAL_HUD);
        center(168, bot_ ? "CASTING OFF" : "START TO LEAVE THE DOCK", PAL_OK);
    } else if (mode_ == Mode::Pause) {
        center(96, "HELD AT THE DOCK", PAL_HUD);
    } else if (mode_ == Mode::Win) {
        center(36, "DOCKED", PAL_OK);
        center(50, "SAME PIER  BUOYS ROUNDED", PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        center(36, "STILL OUT", PAL_RED);
        center(50, "THE DOCK CLOSED", PAL_HUD);
    } else {
        std::snprintf(hud_, sizeof(hud_), "BUOY %d/3", rounded_);
        text(8, 8, hud_, PAL_HUD);
        std::snprintf(hud_, sizeof(hud_), "TIME %d", int(limit_ - time_));
        text(230, 8, hud_, time_ > limit_ - 15 ? PAL_RED : PAL_HUD);
        if (rounded_ < 3)
            center(200, "LEAVE THE MARK TO PORT", PAL_GOLD);
        else
            center(200, "BACK TO THE SAME DOCK", PAL_OK);
        if (flash_ > 0) {
            center(96, "ROUNDED", PAL_OK);
            flash_ -= DT;
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (bump_ > 0) bump_ -= DT;
    if (mode_ == Mode::Title) {
        camX_ = 440;
        camY_ = 540;
        x_ = 440;
        y_ = 620;
        heading_ = std::sin(sys.frame * 0.02f) * 0.15f;
        if (bot_ || sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A)) begin();
    } else if (mode_ == Mode::Pause) {
        if (sys.pad.pressed(gs::BTN_START)) mode_ = Mode::Run;
    } else if (mode_ == Mode::Run) {
        if (!bot_ && sys.pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else physics(readIn());
        float tx = x_;
        float ty = y_ - 20;
        camX_ += (tx - camX_) * 0.08f;
        camY_ += (ty - camY_) * 0.08f;
        if (camX_ < 170) camX_ = 170;
        if (camX_ > 630) camX_ = 630;
        if (camY_ < 130) camY_ = 130;
        if (camY_ > 590) camY_ = 590;
    } else if (!bot_ && (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A))) {
        begin();
    }
    audio();
    draw();
}

}  // namespace buoy

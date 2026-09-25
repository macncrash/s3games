#include "game/bus.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace bus {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kBusHW = 14.f;
constexpr float kBusHH = 28.f;
constexpr float kAccel = 52.f;
constexpr float kBrake = 96.f;
constexpr float kRev = 40.f;
constexpr float kDrag = 8.f;
constexpr float kMaxFwd = 88.f;
constexpr float kMaxRev = 28.f;
constexpr float kStop = 6.f;
constexpr float kLimit = 150.f;
constexpr float kDwell = 1.15f;
constexpr float kCurbL = 56.f;
constexpr float kCurbR = 264.f;

struct Stop {
    const char* name;
    float cx, cy, hw, hh;
    int side;
};

// Legal door box is the painted rectangle. The bus is inside only when its
// whole 28x56 body fits, so slack is hw-14 by hh-28.
constexpr Stop kStops[6] = {
    {"OAK", 88.f, 540.f, 36.f, 48.f, -1},
    {"PIER", 232.f, 1080.f, 28.f, 44.f, 1},
    {"HILL", 78.f, 1640.f, 30.f, 46.f, -1},
    {"MILL", 236.f, 2200.f, 20.f, 42.f, 1},
    {"YARD", 98.f, 2760.f, 32.f, 48.f, -1},
    {"BARN", 222.f, 3320.f, 24.f, 44.f, 1},
};

float guideX(int stop, float y) {
    if (stop < 0) return 160.f;
    if (stop > 5) stop = 5;
    const Stop& s = kStops[stop];
    float begin = s.cy - 400.f;
    float end = s.cy - 180.f;
    if (y <= begin) return 160.f;
    if (y >= end) return s.cx;
    float u = (y - begin) / (end - begin);
    u = u * u * (3.f - 2.f * u);
    return 160.f + (s.cx - 160.f) * u;
}

bool hitsGuide(float cx, float cy, float hw, float hh) {
    float y0 = cy - hh - kBusHH - 4.f;
    float y1 = cy + hh + kBusHH + 4.f;
    for (float y = y0; y <= y1; y += 3.f) {
        for (int s = 0; s < 6; s++) {
            float x = guideX(s, y);
            if (std::abs(x - cx) < hw + kBusHW + 8.f) return true;
        }
    }
    return false;
}

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    for (int y = 0; y < gs::SCREEN_H; y++) sys.vdp.lineBackdrop[y] = gs::rgb4(3, 3, 4);
    layout();
    showTitle();
}

void Game::layout() {
    obs_.clear();
    for (int i = 0; i < 6; i++) {
        const Stop& s = kStops[i];
        Obs block{160.f, s.cy - s.hh - 30.f, 10.f, 14.f, 0};
        Obs curb{s.side < 0 ? 250.f : 70.f, s.cy + 8.f, 10.f, 18.f, 1};
        Obs truck{s.side < 0 ? 252.f : 68.f, s.cy - 170.f, 11.f, 22.f, 2};
        float sgn = s.cx < 160.f ? 1.f : -1.f;
        Obs coneA{s.cx + sgn * (s.hw + 20.f), s.cy - s.hh + 8.f, 4.f, 4.f, 3};
        Obs coneB{s.cx + sgn * (s.hw + 20.f), s.cy + s.hh - 8.f, 4.f, 4.f, 3};
        for (const Obs& o : {block, curb, truck, coneA, coneB}) {
            if (!hitsGuide(o.cx, o.cy, o.hw, o.hh)) obs_.push_back(o);
        }
    }
}

void Game::showTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    next_ = 0;
    score_ = 0;
    boarded_ = 0;
    hits_ = 0;
    time_ = 0;
    dwell_ = 0;
    hurt_ = 0;
    speed_ = 0;
    steer_ = 0;
    msgT_ = 0;
    msg_[0] = 0;
    line_[0] = 0;
    why_ = nullptr;
    song_ = -1;
    buzzT_ = 0;
    const Stop& s = kStops[0];
    x_ = s.cx;
    y_ = s.cy;
    doorSide_ = s.side;
    camY_ = y_;
    chime(false);
}

void Game::begin() {
    mode_ = Mode::Drive;
    over_ = false;
    won_ = false;
    next_ = 0;
    score_ = 0;
    boarded_ = 0;
    hits_ = 0;
    time_ = 0;
    dwell_ = 0;
    hurt_ = 0;
    speed_ = 0;
    steer_ = 0;
    msgT_ = 0;
    msg_[0] = 0;
    line_[0] = 0;
    why_ = nullptr;
    song_ = -1;
    buzzT_ = 0;
    doorSide_ = -1;
    x_ = 160.f;
    y_ = 130.f;
    camY_ = y_;
    if (sys_) sys_->apu.tone(1, 0, 0);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    anim_ += kDt;
    if (bot_ && mode_ == Mode::Title) begin();

    if (mode_ == Mode::Title) {
        if (!bot_ && sys.pad.pressed(gs::BTN_START)) begin();
        audio(kDt);
        lights();
        draw();
        return;
    }
    if (mode_ == Mode::Pause) {
        if (!bot_ && sys.pad.pressed(gs::BTN_START)) mode_ = Mode::Drive;
        else if (!bot_ && sys.pad.pressed(gs::BTN_MODE)) showTitle();
        audio(kDt);
        lights();
        draw();
        return;
    }
    if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        if (!bot_ && sys.pad.pressed(gs::BTN_START)) begin();
        audio(kDt);
        lights();
        draw();
        return;
    }

    if (!bot_ && sys.pad.pressed(gs::BTN_MODE)) {
        showTitle();
        audio(kDt);
        lights();
        draw();
        return;
    }
    if (!bot_ && dwell_ <= 0.f && sys.pad.pressed(gs::BTN_START)) {
        mode_ = Mode::Pause;
        audio(kDt);
        lights();
        draw();
        return;
    }

    In in = readInput();
    if (dwell_ > 0.f) {
        dwell_ -= kDt;
        speed_ = 0.f;
        steer_ = 0.f;
        time_ += kDt;
        if (dwell_ <= 0.f) {
            dwell_ = 0.f;
            boarded_ += 3;
            next_++;
            if (next_ >= 6) finish(true, nullptr);
        }
    } else if (!over_) {
        physics(in, kDt);
        if (!over_) collide();
        if (!over_) tryDoors(in);
        if (!over_ && dwell_ <= 0.f && time_ >= kLimit) finish(false, "TOO LATE");
        if (!over_) time_ += kDt;
    }
    if (hurt_ > 0.f) hurt_ -= kDt;
    if (msgT_ > 0.f) msgT_ -= kDt;
    follow(kDt);
    audio(kDt);
    lights();
    draw();
}

Game::In Game::readInput() {
    if (bot_) return pilot();
    return human();
}

Game::In Game::human() {
    In in;
    const gs::Pad& p = sys_->pad;
    float ax = p.axisX;
    if (ax < -0.2f || ax > 0.2f) in.steer = std::clamp(ax, -1.f, 1.f);
    else in.steer = (p.down(gs::BTN_RIGHT) ? 1.f : 0.f) - (p.down(gs::BTN_LEFT) ? 1.f : 0.f);
    bool gas = p.down(gs::BTN_UP) || p.accel > 0.18f;
    bool brk = p.down(gs::BTN_DOWN) || p.brake > 0.18f;
    if (brk && !gas) in.brake = p.brake > 0.18f ? std::max(p.brake, 0.4f) : 1.f;
    else if (gas) in.throttle = p.accel > 0.18f ? std::max(p.accel, 0.4f) : 1.f;
    in.doors = p.down(gs::BTN_A) || p.down(gs::BTN_C) || p.down(gs::BTN_TURBO);
    in.doorsEdge = p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C) || p.pressed(gs::BTN_TURBO);
    if (p.pressed(gs::BTN_B)) sys_->apu.noiseBurst(0.05f, 220.f, 5.f);
    return in;
}

Game::In Game::pilot() const {
    In in;
    if (next_ >= 6 || dwell_ > 0.f) return in;
    const Stop& s = kStops[next_];
    float dx = s.cx - x_;
    float dy = s.cy - y_;
    float slackX = s.hw - kBusHW;
    float slackY = s.hh - kBusHH;
    bool xok = std::abs(dx) <= slackX - 0.8f;
    bool yok = std::abs(dy) <= slackY - 0.8f;

    float tx = s.cx;
    float target = 0.f;
    // Leave the stop just served in a straight line so the nose doesn't cut a cone.
    if (next_ > 0) {
        const Stop& prev = kStops[next_ - 1];
        if (y_ < prev.cy + prev.hh + 36.f) {
            tx = prev.cx;
            target = 34.f;
            float err = tx - x_;
            in.steer = std::clamp(err / 6.f, -1.f, 1.f);
            if (std::abs(err) < 0.25f) in.steer = 0.f;
            float se = target - speed_;
            if (se > 0.8f) in.throttle = std::clamp(se / 12.f, 0.f, 1.f);
            else if (se < -0.8f) in.brake = std::clamp(-se / 10.f, 0.f, 1.f);
            return in;
        }
    }
    if (dy > 120.f) {
        tx = guideX(next_, y_);
        target = 82.f;
        if (std::abs(tx - x_) > 8.f) target = 32.f;
    } else if (!xok) {
        tx = s.cx;
        if (dy > slackY + 10.f) target = 0.f;
        else if (dy > 0.f) target = 8.f;
        else target = -8.f;
    } else if (!yok) {
        tx = s.cx;
        target = std::clamp(dy * 0.75f, -18.f, 18.f);
        if (std::abs(target) < 7.f && std::abs(dy) > 1.2f) target = dy > 0.f ? 7.f : -7.f;
    }

    float err = tx - x_;
    in.steer = std::clamp(err / 6.f, -1.f, 1.f);
    if (std::abs(err) < 0.25f) in.steer = 0.f;
    float se = target - speed_;
    if (se > 0.8f) in.throttle = std::clamp(se / 12.f, 0.f, 1.f);
    else if (se < -0.8f) in.brake = std::clamp(-se / 10.f, 0.f, 1.f);
    if (xok && yok && std::abs(speed_) < 4.f) in.doors = true;
    return in;
}

void Game::physics(const In& in, float dt) {
    steer_ = in.steer;
    if (in.brake > 0.f && in.throttle <= 0.f) {
        if (speed_ > 1.f) speed_ -= kBrake * in.brake * dt;
        else speed_ -= kRev * in.brake * dt;
    } else if (in.throttle > 0.f) {
        if (speed_ < 0.f) speed_ += kBrake * in.throttle * dt;
        else speed_ += kAccel * in.throttle * dt;
    } else {
        float d = kDrag + std::abs(speed_) * 0.35f;
        if (speed_ > 0.f) speed_ = std::max(0.f, speed_ - d * dt);
        else if (speed_ < 0.f) speed_ = std::min(0.f, speed_ + d * dt);
    }
    speed_ = std::clamp(speed_, -kMaxRev, kMaxFwd);
    float lat = 16.f + std::abs(speed_) * 0.34f;
    if (lat > 46.f) lat = 46.f;
    x_ += in.steer * lat * dt;
    y_ += speed_ * dt;
    if (y_ < 50.f) {
        y_ = 50.f;
        if (speed_ < 0.f) speed_ = 0.f;
    }
    float far = kStops[5].cy + 180.f;
    if (y_ > far) {
        y_ = far;
        if (speed_ > 0.f) speed_ = 0.f;
    }
}

void Game::collide() {
    for (int pass = 0; pass < 2 && !over_; pass++) {
        for (const Obs& o : obs_) separate(o.cx, o.cy, o.hw, o.hh);
        if (over_) return;
        if (x_ < kCurbL) {
            x_ = kCurbL;
            bump();
        }
        if (!over_ && x_ > kCurbR) {
            x_ = kCurbR;
            bump();
        }
    }
}

void Game::separate(float cx, float cy, float hw, float hh) {
    float dx = x_ - cx;
    float dy = y_ - cy;
    float px = kBusHW + hw - std::abs(dx);
    float py = kBusHH + hh - std::abs(dy);
    if (px <= 0.f || py <= 0.f) return;
    if (px < py) x_ += (dx < 0.f ? -px : px);
    else {
        y_ += (dy < 0.f ? -py : py);
        if (dy > 0.f && speed_ < 0.f) speed_ = 0.f;
        if (dy < 0.f && speed_ > 0.f) speed_ *= 0.25f;
    }
    bump();
}

void Game::bump() {
    if (hurt_ > 0.f || over_) return;
    hits_++;
    hurt_ = 0.9f;
    speed_ *= -0.2f;
    sys_->apu.noiseBurst(0.16f, 2100.f, 7.f);
    sys_->rumble(0.35f, 0.7f, 110);
    if (hits_ >= 3) {
        finish(false, "WRECKED");
        return;
    }
    char b[32];
    std::snprintf(b, sizeof b, "BUMP %d/3", hits_);
    say(b);
}

void Game::tryDoors(const In& in) {
    if (next_ >= 6 || dwell_ > 0.f) return;
    const Stop& s = kStops[next_];
    bool legal = contained(s.cx, s.cy, s.hw, s.hh) && std::abs(speed_) < kStop;
    if (legal && in.doors) openDoors();
    else if (!legal && in.doorsEdge) deny();
}

void Game::openDoors() {
    if (dwell_ > 0.f || next_ >= 6 || over_) return;
    const Stop& s = kStops[next_];
    float sx = std::max(0.01f, s.hw - kBusHW);
    float sy = std::max(0.01f, s.hh - kBusHH);
    float ax = 1.f - std::abs(x_ - s.cx) / sx;
    float ay = 1.f - std::abs(y_ - s.cy) / sy;
    int bonus = int(std::lround(200.f * std::clamp(ax, 0.f, 1.f) * std::clamp(ay, 0.f, 1.f)));
    score_ += 400 + bonus;
    dwell_ = kDwell;
    doorSide_ = s.side;
    char b[40];
    std::snprintf(b, sizeof b, "%s  +%d", s.name, 400 + bonus);
    say(b);
    msgT_ = kDwell;
    chime(false);
    sys_->rumble(0.08f, 0.22f, 140);
}

void Game::deny() {
    const char* m = "DOORS STAY SHUT";
    if (next_ < 6) {
        const Stop& s = kStops[next_];
        if (contained(s.cx, s.cy, s.hw, s.hh) && std::abs(speed_) >= kStop) m = "STILL ROLLING";
        else if (boxOverlap(s.cx, s.cy, s.hw, s.hh)) m = "NOT IN THE BOX";
        else {
            for (int i = 0; i < 6; i++) {
                if (i != next_ && contained(kStops[i].cx, kStops[i].cy, kStops[i].hw, kStops[i].hh)) m = "WRONG STOP";
            }
        }
    }
    say(m);
    buzzT_ = 0.22f;
    sys_->apu.tone(2, 64.f, 0.05f);
    sys_->apu.noiseBurst(0.04f, 640.f, 10.f);
}

void Game::say(const char* s) {
    std::snprintf(msg_, sizeof msg_, "%s", s ? s : "");
    msgT_ = 1.25f;
}

void Game::finish(bool win, const char* why) {
    if (over_) return;
    over_ = true;
    won_ = win;
    why_ = why;
    mode_ = win ? Mode::Win : Mode::Fail;
    speed_ = 0.f;
    if (win) {
        std::snprintf(line_, sizeof line_,
                      "S3 BUS  WIN  six stops  doors opened in the box  score %d  (%.1f s)", score_, time_);
        chime(true);
    } else {
        std::snprintf(line_, sizeof line_,
                      "S3 BUS  FAIL  %s  stop %d/6  x %.0f y %.0f spd %.0f bumps %d  score %d  (%.1f s)",
                      why ? why : "STOPPED", std::min(next_, 5) + 1, x_, y_, speed_, hits_, score_, time_);
    }
}

void Game::chime(bool fanfare) {
    song_ = fanfare ? 1 : 0;
    songN_ = 0;
    songT_ = 0.f;
}

void Game::audio(float dt) {
    float spd = std::abs(speed_);
    float vol = (mode_ == Mode::Drive && dwell_ <= 0.f) ? 0.018f + spd * 0.00038f : 0.f;
    float freq = speed_ < -1.f ? 42.f + spd : 48.f + spd * 2.05f;
    sys_->apu.tone(0, freq, vol);

    if (song_ >= 0) {
        const float doorN[2] = {698.f, 932.f};
        const float fanN[4] = {523.f, 659.f, 784.f, 1046.f};
        const float* notes = song_ == 0 ? doorN : fanN;
        int count = song_ == 0 ? 2 : 4;
        songT_ -= dt;
        if (songT_ <= 0.f) {
            if (songN_ >= count) {
                song_ = -1;
                sys_->apu.tone(1, 0, 0);
            } else {
                sys_->apu.tone(1, notes[songN_], song_ == 0 ? 0.055f : 0.062f);
                songN_++;
                songT_ = 0.15f;
            }
        }
    }
    if (buzzT_ > 0.f) {
        buzzT_ -= dt;
        if (buzzT_ <= 0.f) sys_->apu.tone(2, 0, 0);
    }
}

void Game::follow(float dt) {
    float look = mode_ == Mode::Drive ? std::clamp(speed_, 0.f, 40.f) * 0.3f : 0.f;
    float goal = y_ + look;
    float k = 1.f - std::exp(-dt * 5.f);
    camY_ += (goal - camY_) * k;
}

void Game::lights() {
    if (!sys_) return;
    if (mode_ == Mode::Win) sys_->setLight(40, 220, 70);
    else if (mode_ == Mode::Fail) sys_->setLight(220, 40, 30);
    else if (mode_ == Mode::Drive && next_ < 6 && contained(kStops[next_].cx, kStops[next_].cy, kStops[next_].hw, kStops[next_].hh) &&
             std::abs(speed_) < kStop)
        sys_->setLight(30, 200, 60);
    else if (hurt_ > 0.f) sys_->setLight(220, 50, 20);
    else sys_->setLight(230, 170, 40);
}

bool Game::contained(float cx, float cy, float hw, float hh) const {
    return std::abs(x_ - cx) <= (hw - kBusHW) + 0.02f && std::abs(y_ - cy) <= (hh - kBusHH) + 0.02f;
}

bool Game::boxOverlap(float cx, float cy, float hw, float hh) const {
    return std::abs(x_ - cx) < hw + kBusHW && std::abs(y_ - cy) < hh + kBusHH;
}

void Game::toScreen(float wx, float wy, float& sx, float& sy) const {
    sx = wx;
    sy = 118.f - (wy - camY_);
}

void Game::blit(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, bool flip, bool shadow) {
    if (w < 1.f || h < 1.f || m.h < 1) return;
    if (cx + w * 0.5f < -32.f || cy + h * 0.5f < -32.f || cx - w * 0.5f > gs::SCREEN_W + 32.f ||
        cy - h * 0.5f > gs::SCREEN_H + 32.f)
        return;
    gs::Sprite s;
    long sw = std::clamp(std::lround(w), 1L, 2000L);
    long sh = std::clamp(std::lround(h), 1L, 2000L);
    s.w = int16_t(sw);
    s.h = int16_t(sh);
    s.x = int16_t(std::clamp(std::lround(cx - sw * 0.5f), -2000L, 2000L));
    s.y = int16_t(std::clamp(std::lround(cy - sh * 0.5f), -2000L, 2000L));
    s.img = m.pick(float(sh));
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::world(const gs::Mipped& m, float wx, float wy, float h, int pal, bool flip) {
    float sx, sy;
    toScreen(wx, wy, sx, sy);
    float w = h * float(m.w) / float(std::max(1, m.h));
    blit(m, sx, sy, w, h, pal, flip, false);
}

void Game::hudText(int col, int row, const char* s, int pal) {
    if (!s) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || row < 0 || row > 27 || c < 32 || c > 127 || c == ' ') continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = 0;
    if (s)
        while (s[n]) n++;
    hudText(20 - n / 2, row, s, pal);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.B.scroll(0, int(std::lround(-camY_)));

    blit(art_.panel, 160.f, 200.f, 320.f, 48.f, PAL_BUS, false, false);

    bool open = mode_ == Mode::Title || mode_ == Mode::Win || dwell_ > 0.f;
    bool blink = hurt_ > 0.f && (int(anim_ * 18.f) & 1);
    if (!blink) {
        float sx, sy;
        toScreen(x_ + 2.f, y_ - 4.f, sx, sy);
        blit(art_.shade, sx, sy, 40.f, 16.f, PAL_BUS, false, true);
        if (open) {
            int side = doorSide_ >= 0 ? doorSide_ : -1;
            world(art_.busDoor[side < 0 ? 0 : 1], x_, y_, 72.f, PAL_BUS, false);
        } else if (steer_ < -0.35f) {
            world(art_.busYawL, x_, y_, 72.f, PAL_BUS, false);
        } else if (steer_ > 0.35f) {
            world(art_.busYawR, x_, y_, 72.f, PAL_BUS, false);
        } else {
            world(art_.bus, x_, y_, 72.f, PAL_BUS, false);
        }
    }

    for (int i = 0; i < 6; i++) {
        if (i < next_) continue;
        const Stop& s = kStops[i];
        float shelf = s.side < 0 ? 30.f : 290.f;
        float u = 0.f;
        if (mode_ == Mode::Title && i == 0) u = 0.62f;
        else if (i == next_ && dwell_ > 0.f) u = 1.f - std::clamp(dwell_ / kDwell, 0.f, 1.f);
        bool faceLeft = shelf > x_;
        int fr = (u > 0.08f && (int(anim_ * 8.f) & 1)) ? 1 : 0;
        for (int p = 0; p < 3; p++) {
            float py = s.cy + float(p - 1) * 10.f;
            float px = shelf + (x_ - shelf) * u * 0.82f;
            if (u < 0.05f) py += std::sin(anim_ * 3.f + float(p + i)) * 0.7f;
            world(art_.person[fr], px, py, 16.f, PAL_PERSON, faceLeft);
        }
    }

    for (const Obs& o : obs_) {
        if (o.kind == 3) world(art_.cone, o.cx, o.cy, 14.f, PAL_CONE, false);
        else if (o.kind == 2) world(art_.truck, o.cx, o.cy, 56.f, PAL_TRUCK, false);
        else if (o.kind == 1) world(art_.car[1], o.cx, o.cy, 42.f, PAL_CAR2, false);
        else world(art_.car[0], o.cx, o.cy, 42.f, PAL_CAR, false);
    }

    for (int i = 0; i < 6; i++) {
        float shelf = kStops[i].side < 0 ? 26.f : 294.f;
        world(art_.shelter[i], shelf, kStops[i].cy, 46.f, PAL_SHELTER, false);
    }

    for (int i = 0; i < 26; i++) {
        float ty = 60.f + float(i) * 140.f;
        world(art_.tree, 12.f, ty, (i & 1) ? 30.f : 22.f, PAL_TREE, false);
        world(art_.tree, 308.f, ty + 40.f, (i & 1) ? 22.f : 28.f, PAL_TREE, true);
    }

    for (int i = 0; i < 6; i++) {
        const Stop& s = kStops[i];
        world(art_.zebra, s.cx, s.cy - s.hh - 8.f, 14.f, PAL_ROAD, false);
    }
    for (int i = 0; i < 6; i++) {
        const Stop& s = kStops[i];
        bool ready = false;
        if (mode_ == Mode::Title && i == 0) ready = true;
        else if (mode_ == Mode::Win && i == 5) ready = true;
        else if (i == next_ && (dwell_ > 0.f || (mode_ == Mode::Drive && contained(s.cx, s.cy, s.hw, s.hh) && std::abs(speed_) < kStop)))
            ready = true;
        float sx, sy;
        toScreen(s.cx, s.cy, sx, sy);
        blit(art_.box, sx, sy, s.hw * 2.f, s.hh * 2.f, ready ? PAL_READY : PAL_BOX, false, false);
    }

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(23, "S3 BUS", PAL_BANNER);
        hudC(24, "SIX STOPS", PAL_HUD);
        hudC(25, "DOORS OPEN ONLY IN THE BOX", PAL_BANNER);
        hudC(26, "ARROWS DRIVE   Z OPENS DOORS", PAL_DIM);
        if ((int(anim_ * 2.f) & 1) == 0) hudC(27, "ENTER", PAL_OK);
        return;
    }
    if (mode_ == Mode::Pause) {
        hudC(23, "S3 BUS", PAL_BANNER);
        hudC(25, "PAUSED", PAL_HUD);
        hudC(27, "ENTER CONTINUES", PAL_DIM);
        return;
    }
    if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        hudC(23, "S3 BUS", PAL_BANNER);
        if (won_) {
            hudC(24, "SIX STOPS", PAL_OK);
            hudC(25, "DOORS OPENED IN THE BOX", PAL_BANNER);
        } else {
            hudC(24, "ROUTE CUT", PAL_ALERT);
            hudC(25, why_ ? why_ : "STOPPED", PAL_ALERT);
        }
        int sec = int(time_);
        std::snprintf(buf, sizeof buf, "SCORE %d    %d:%02d", score_, sec / 60, sec % 60);
        hudC(26, buf, PAL_HUD);
        hudC(27, "ENTER RIDES AGAIN", PAL_DIM);
        return;
    }

    const Stop& s = kStops[std::min(next_, 5)];
    hudText(1, 23, "S3 BUS", PAL_BANNER);
    if (next_ < 6) std::snprintf(buf, sizeof buf, "%d/6 %s", next_ + 1, s.name);
    else std::snprintf(buf, sizeof buf, "6/6 BARN");
    hudText(10, 23, buf, PAL_HUD);
    int sec = int(time_);
    std::snprintf(buf, sizeof buf, "%d:%02d", sec / 60, sec % 60);
    hudText(sec >= 600 ? 33 : 34, 23, buf, time_ > kLimit - 20.f ? PAL_ALERT : PAL_HUD);

    int spd = int(std::lround(std::abs(speed_)));
    std::snprintf(buf, sizeof buf, speed_ < -1.f ? "SPD %02d REV" : "SPD %02d", spd);
    hudText(1, 24, buf, PAL_HUD);
    std::snprintf(buf, sizeof buf, "BUMP %d/3", hits_);
    hudText(14, 24, buf, hits_ ? PAL_ALERT : PAL_DIM);
    std::snprintf(buf, sizeof buf, "RIDE %d", boarded_);
    hudText(24, 24, buf, PAL_OK);
    std::snprintf(buf, sizeof buf, "%d", score_);
    hudText(33, 24, buf, PAL_BANNER);

    const char* hint = nullptr;
    int hintPal = PAL_HUD;
    if (msgT_ > 0.f && msg_[0]) {
        hint = msg_;
        hintPal = PAL_BANNER;
    } else if (next_ >= 6) {
        hint = "ROUTE DONE";
        hintPal = PAL_OK;
    } else if (dwell_ > 0.f) {
        hint = "DOORS OPEN";
        hintPal = PAL_OK;
    } else if (contained(s.cx, s.cy, s.hw, s.hh) && std::abs(speed_) < kStop) {
        hint = "OPEN THE DOORS";
        hintPal = (int(anim_ * 4.f) & 1) ? PAL_OK : PAL_BANNER;
    } else if ((contained(s.cx, s.cy, s.hw, s.hh) || boxOverlap(s.cx, s.cy, s.hw, s.hh)) && std::abs(speed_) >= kStop) {
        hint = "STILL ROLLING";
        hintPal = PAL_ALERT;
    } else if (boxOverlap(s.cx, s.cy, s.hw, s.hh)) {
        hint = "NOT IN THE BOX";
        hintPal = PAL_ALERT;
    } else if (std::abs(s.cx - x_) > 18.f && std::abs(s.cy - y_) < 460.f) {
        hint = s.side < 0 ? "PULL LEFT INTO THE BOX" : "PULL RIGHT INTO THE BOX";
        hintPal = PAL_BANNER;
    } else {
        int meters = int(std::lround(std::abs(s.cy - y_) * 0.15f));
        std::snprintf(buf, sizeof buf, "%d M TO %s", meters, s.name);
        hint = buf;
        hintPal = PAL_HUD;
    }
    hudC(25, hint, hintPal);

    for (int i = 0; i < 6; i++) {
        int pal = i < next_ ? PAL_OK : (i == next_ ? PAL_BANNER : PAL_DIM);
        hudText(2 + i * 6, 26, kStops[i].name, pal);
    }
    hudC(27, "ARROWS DRIVE   Z OPENS DOORS", PAL_DIM);
}

}  // namespace bus

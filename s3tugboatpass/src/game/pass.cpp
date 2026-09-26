#include "pass.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace tugpass {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float PI = 3.14159265f;
constexpr float TAU = 6.2831853f;
constexpr float LINE = 120.f;
constexpr float RIVAL0 = 1.2f;
constexpr float RIVAL_HL = 3.35f;
constexpr float CREW_SPEED = 1.40f;
constexpr float START_Y = 13.5f;
constexpr float HALF_L = 3.62f;
constexpr float HALF_B = 1.40f;
constexpr float MAX_SPD = 2.05f;
constexpr float DRIFT_DRAG = 2.2f;
constexpr float PLAY_ZOOM = 8.6f;
constexpr float TITLE_ZOOM = 5.2f;
constexpr float TITLE_CAM_Y = 22.f;

float clampf(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }

float wrapPi(float a) {
    while (a > PI) a -= TAU;
    while (a < -PI) a += TAU;
    return a;
}

float gauss(float y, float c, float s) {
    float d = (y - c) / s;
    return std::exp(-d * d);
}

uint16_t lerpColor(uint16_t a, uint16_t b, float t) {
    t = clampf(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

}  // namespace

float Game::centerAt(float y) const {
    return 9.5f * std::sin(y * 0.030f) + 3.6f * std::sin(y * 0.055f + 1.7f);
}

float Game::halfAt(float y) const {
    float w = 8.2f;
    w -= 2.2f * gauss(y, 42.f, 10.f);
    w -= 3.05f * gauss(y, 80.f, 7.5f);
    w -= 2.6f * gauss(y, 106.f, 6.5f);
    if (y < 22.f) w += (22.f - std::max(y, 0.f)) / 22.f * 4.2f;
    if (y > 108.f) {
        float u = clampf((y - 108.f) / 9.f, 0.f, 1.f);
        u = u * u * (3.f - 2.f * u);
        w += u * 14.f;
    }
    return w;
}

float Game::currentAt(float y) const {
    return 1.15f * gauss(y, 80.f, 11.f) - 0.85f * gauss(y, 104.f, 8.f);
}

float Game::wallGap(float x, float y) const { return halfAt(y) - std::fabs(x - centerAt(y)); }

float Game::crewMax() const { return (LINE - (RIVAL0 + RIVAL_HL)) / CREW_SPEED; }

float Game::crewLeft() const {
    float bow = rivalY() + RIVAL_HL;
    return std::max(0.f, (LINE - bow) / CREW_SPEED);
}

float Game::urgency() const {
    float m = crewMax();
    if (m <= 0.01f) return 1.f;
    return clampf(1.f - crewLeft() / m, 0.f, 1.f);
}

float Game::rivalY() const { return RIVAL0 + CREW_SPEED * playT_; }

float Game::rivalHeading() const {
    float y = rivalY();
    float dx = centerAt(y + 3.f) - centerAt(y - 3.f);
    return std::atan2(dx, 6.f);
}

float Game::rivalX() const {
    float y = rivalY();
    float x = centerAt(y);
    float dy = std::fabs(y - y_);
    if (dy < 9.f) {
        float room = halfAt(y) - 2.3f;
        float push = std::min(room, 1.8f);
        if (push > 0.3f) x += push * (1.f - dy / 9.f);
    }
    return x;
}

float Game::sx(float wx) const { return 160.f + (wx - camX_) * zoom_ + shx_; }
float Game::sy(float wy) const { return 112.f - (wy - camY_) * zoom_ + shy_; }

int Game::yawFrameOf(float h) const {
    float u = std::fmod(h, TAU);
    if (u < 0.f) u += TAU;
    int i = int(std::lround(u / TAU * float(YAWS))) % YAWS;
    if (i < 0) i += YAWS;
    return i;
}

int Game::fogAt(float wy) const {
    float ry = rivalY();
    if (wy > ry - 0.5f) return 0;
    return int(clampf((ry - wy) / 16.f, 0.f, 1.f) * 12.f);
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Win || mode_ == Mode::Fail || over_) return 4;
    if (y_ >= 100.f) return 3;
    if (y_ >= 62.f) return 2;
    return 1;
}

bool Game::flashOn() const {
    if (urgency() < 0.58f || mode_ == Mode::Title || mode_ == Mode::Win) return false;
    return std::fmod(t_, 1.7f) < 0.07f;
}

void Game::sampleHull(float* xs, float* ys, int& n) const {
    n = 0;
    const float fx = std::sin(hdg_), fy = std::cos(hdg_);
    const float rx = std::cos(hdg_), ry = -std::sin(hdg_);
    const float along[] = {1.f, 0.55f, 0.f, -0.55f, -1.f};
    const float side[] = {-1.f, 0.f, 1.f};
    for (float a : along) {
        float beam = std::fabs(a) > 0.9f ? 0.42f : 1.f;
        for (float b : side) {
            xs[n] = x_ + fx * a * HALF_L + rx * b * HALF_B * beam;
            ys[n] = y_ + fy * a * HALF_L + ry * b * HALF_B * beam;
            n++;
        }
    }
}

const char* Game::struck() const {
    float xs[16], ys[16];
    int n = 0;
    sampleHull(xs, ys, n);
    for (int i = 0; i < n; i++)
        if (wallGap(xs[i], ys[i]) < -0.06f) return "struck the wall";
    return nullptr;
}

bool Game::hullClear() const {
    float xs[16], ys[16];
    int n = 0;
    sampleHull(xs, ys, n);
    if (n <= 0) return false;
    for (int i = 0; i < n; i++)
        if (ys[i] < LINE) return false;
    return true;
}

void Game::resetPose() {
    over_ = false;
    won_ = false;
    why_ = "";
    playT_ = 0;
    y_ = START_Y;
    x_ = centerAt(y_);
    float dx = centerAt(y_ + 6.f) - x_;
    hdg_ = std::atan2(dx, 6.f);
    speed_ = 0;
    yawV_ = 0;
    driftX_ = 0;
    thrust_ = 0;
    steer_ = 0;
    throttle_ = 0;
    shake_ = 0;
    wakeT_ = 0;
    smokeT_ = 0;
    hornT_ = 0;
    minGap_ = 99.f;
    beepHold_ = 0;
    beepSec_ = -1;
    wakeN_ = 0;
    smokeN_ = 0;
    chimeStep_ = -1;
    flashWas_ = false;
    for (Puff& w : wakes_) w = {};
    for (Puff& s : smokes_) s = {};
}

void Game::snapCamera() {
    if (mode_ == Mode::Title) {
        camX_ = centerAt(TITLE_CAM_Y);
        camY_ = TITLE_CAM_Y;
        zoom_ = TITLE_ZOOM;
    } else if (mode_ == Mode::Win) {
        camX_ = x_;
        camY_ = y_ - 1.2f;
        zoom_ = 6.4f;
    } else if (mode_ == Mode::Fail) {
        camX_ = x_;
        camY_ = y_ + 0.4f;
        zoom_ = 7.2f;
    } else {
        camX_ = x_ * 0.8f + centerAt(y_) * 0.2f;
        camY_ = y_ - 1.4f;
        zoom_ = PLAY_ZOOM;
    }
}

void Game::followCamera() {
    float tx = x_ * 0.8f + centerAt(y_) * 0.2f;
    float ty = y_ - 1.4f;
    float tz = PLAY_ZOOM;
    if (mode_ == Mode::Title) {
        tx = centerAt(TITLE_CAM_Y);
        ty = TITLE_CAM_Y;
        tz = TITLE_ZOOM;
    } else if (mode_ == Mode::Win) {
        tx = x_;
        ty = y_ - 1.2f;
        tz = 6.4f;
    } else if (mode_ == Mode::Fail) {
        tx = x_;
        ty = y_ + 0.2f;
        tz = 7.2f;
    }
    float k = 1.f - std::exp(-DT * 5.2f);
    camX_ += (tx - camX_) * k;
    camY_ += (ty - camY_) * k;
    zoom_ += (tz - zoom_) * k;
}

void Game::begin() {
    resetPose();
    mode_ = Mode::Play;
    snapCamera();
    horn(0.42f);
}

void Game::showTitle() {
    resetPose();
    mode_ = Mode::Title;
    snapCamera();
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    why_ = why;
    shake_ = 1.f;
    sys_->apu.noiseBurst(0.55f, 380.f, 0.32f);
    sys_->apu.tone(1, 0, 0);
    sys_->apu.tone(2, 0, 0);
    sys_->rumble(0.8f, 0.45f, 220);
    sys_->setLight(170, 30, 24);
}

void Game::win() {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Win;
    over_ = true;
    won_ = true;
    why_ = "clear";
    chime();
    sys_->rumble(0.32f, 0.14f, 180);
    sys_->setLight(40, 140, 70);
}

void Game::chime() {
    chimeStep_ = 0;
    chimeT_ = 0.02f;
}

void Game::horn(float seconds) {
    if (hornT_ < 0.05f) hornT_ = seconds;
}

void Game::pilot(float& thrust, float& steer) const {
    const float look = 5.6f;
    float bowX = x_ + std::sin(hdg_) * HALF_L;
    float bowY = y_ + std::cos(hdg_) * HALF_L;
    float aimY = bowY + look;
    float tx = centerAt(aimY);
    float crab = currentAt(bowY + 2.f) / DRIFT_DRAG;
    tx -= crab * (look / std::max(speed_, 1.15f));
    float cross = centerAt(y_) - x_;
    float wantH = std::atan2(tx - bowX, std::max(2.4f, aimY - bowY));
    wantH += clampf(cross * 0.22f, -0.28f, 0.28f);
    wantH = clampf(wantH, -0.7f, 0.7f);
    float hErr = wrapPi(wantH - hdg_);
    steer = hErr * 2.4f - yawV_ * 1.15f;
    steer -= clampf(driftX_ * 0.85f, -0.45f, 0.45f);
    steer += clampf(cross * 0.5f, -0.55f, 0.55f);

    float pred = bowX + std::sin(hdg_) * 2.4f;
    float c = centerAt(bowY + 2.2f);
    float h = halfAt(bowY + 2.2f);
    float leftGap = pred - (c - h);
    float rightGap = (c + h) - pred;
    if (leftGap < 2.5f) steer += (2.5f - leftGap) * 1.15f;
    if (rightGap < 2.5f) steer -= (2.5f - rightGap) * 1.15f;

    float sternX = x_ - std::sin(hdg_) * HALF_L;
    float sternY = y_ - std::cos(hdg_) * HALF_L;
    float sg = wallGap(sternX, sternY);
    float soff = sternX - centerAt(sternY);
    if (sg < 1.8f) steer += (soff > 0.f ? 1.f : -1.f) * (1.8f - sg) * 1.05f;

    float bowGap = wallGap(bowX, bowY);
    if (bowGap < 1.9f) {
        float off = bowX - centerAt(bowY);
        steer += (off > 0.f ? -1.f : 1.f) * (1.9f - bowGap) * 1.3f;
    }
    steer = clampf(steer, -1.f, 1.f);

    float want = MAX_SPD;
    if (std::fabs(hErr) > 0.38f) want = 1.45f;
    if (std::fabs(cross) > 1.15f) want = std::min(want, 1.4f);
    if (bowGap < 2.0f || sg < 1.7f) want = std::min(want, 1.2f);
    float dist = (LINE + HALF_L) - y_;
    float need = dist / std::max(0.5f, crewLeft() - 6.f);
    if (need > 1.25f && wallGap(x_, y_) > 2.3f && std::fabs(cross) < 1.3f) want = MAX_SPD;
    thrust = clampf((want - speed_) * 2.5f, -1.f, 1.f);
}

void Game::controls() {
    const gs::Pad& p = sys_->pad;
    steer_ = 0.f;
    if (p.down(gs::BTN_LEFT)) steer_ -= 1.f;
    if (p.down(gs::BTN_RIGHT)) steer_ += 1.f;
    if (std::fabs(p.axisX) > 0.15f) steer_ = clampf(p.axisX, -1.f, 1.f);

    float want = 0.f;
    const bool ahead = p.down(gs::BTN_UP) || p.down(gs::BTN_C);
    const bool astern = p.down(gs::BTN_DOWN) || p.down(gs::BTN_B);
    if (ahead) want = 1.f;
    if (astern) want = -1.f;
    if (p.axisY > 0.22f) want = std::max(want, p.axisY);
    if (p.axisY < -0.22f) want = std::min(want, p.axisY);
    if (p.accel > 0.08f) want = std::max(want, p.accel);
    if (p.brake > 0.08f) want = std::min(want, -p.brake);
    float k = 1.f - std::exp(-3.2f * DT);
    throttle_ += (want - throttle_) * k;
    thrust_ = throttle_;
    if (p.pressed(gs::BTN_A) || p.pressed(gs::BTN_Z) || p.pressed(gs::BTN_TURBO)) horn(0.28f);
}

void Game::integrate(float dt) {
    yawV_ += steer_ * (0.70f + std::fabs(speed_) * 0.22f) * dt;
    yawV_ -= yawV_ * 3.4f * dt;
    yawV_ = clampf(yawV_, -0.85f, 0.85f);
    float prev = hdg_;
    float dH = yawV_ * dt;
    hdg_ = wrapPi(prev + dH);
    float pivot = 0.28f * HALF_L;
    float px = x_ + std::sin(prev) * pivot;
    float py = y_ + std::cos(prev) * pivot;
    x_ = px - std::sin(hdg_) * pivot;
    y_ = py - std::cos(hdg_) * pivot;

    float drive = thrust_ * (thrust_ >= 0.f ? 1.72f : 1.15f);
    speed_ += drive * dt;
    speed_ -= speed_ * 0.48f * dt;
    speed_ = clampf(speed_, -0.85f, MAX_SPD);

    float slide = 0.f;
    if (std::fabs(speed_) < 0.7f) slide = steer_ * 0.42f * (0.7f - std::fabs(speed_)) / 0.7f;
    x_ += (std::sin(hdg_) * speed_ + std::cos(hdg_) * slide) * dt;
    y_ += (std::cos(hdg_) * speed_ - std::sin(hdg_) * slide) * dt;

    driftX_ += currentAt(y_) * dt;
    driftX_ -= driftX_ * DRIFT_DRAG * dt;
    driftX_ = clampf(driftX_, -2.f, 2.f);
    x_ += driftX_ * dt;

    if (y_ < 1.6f) {
        y_ = 1.6f;
        if (speed_ < 0.f) speed_ = 0.f;
        driftX_ *= 0.4f;
    }
    x_ = clampf(x_, -40.f, 40.f);
}

void Game::ambience() {
    wakeT_ -= DT;
    if (mode_ == Mode::Play && wakeT_ <= 0.f && std::fabs(speed_) > 0.35f) {
        wakeT_ = 0.08f;
        Puff& w = wakes_[wakeN_++ % 12];
        w.x = x_ - std::sin(hdg_) * HALF_L * 0.82f;
        w.y = y_ - std::cos(hdg_) * HALF_L * 0.82f;
        w.life = 1.f;
        w.seed = t_;
    }
    for (Puff& w : wakes_)
        if (w.life > 0.f) w.life -= DT * 0.65f;

    smokeT_ -= DT;
    bool puff = mode_ == Mode::Title || mode_ == Mode::Play || mode_ == Mode::Win;
    if (puff && smokeT_ <= 0.f) {
        smokeT_ = mode_ == Mode::Play && std::fabs(thrust_) > 0.2f ? 0.12f : 0.28f;
        Puff& s = smokes_[smokeN_++ % 10];
        s.x = x_ - std::sin(hdg_) * 0.85f;
        s.y = y_ - std::cos(hdg_) * 0.85f;
        s.life = 1.f;
        s.seed = float(smokeN_);
    }
    for (Puff& s : smokes_)
        if (s.life > 0.f) s.life -= DT * 0.45f;
}

void Game::step() {
    playT_ += DT;
    if (bot_) {
        float wantT = 0.f, wantS = 0.f;
        pilot(wantT, wantS);
        steer_ = wantS;
        float k = 1.f - std::exp(-2.8f * DT);
        throttle_ += (wantT - throttle_) * k;
        thrust_ = throttle_;
    } else {
        controls();
    }

    auto sub = [&](float dt) -> bool {
        integrate(dt);
        float xs[16], ys[16];
        int n = 0;
        sampleHull(xs, ys, n);
        float gap = 99.f;
        for (int i = 0; i < n; i++) gap = std::min(gap, wallGap(xs[i], ys[i]));
        minGap_ = std::min(minGap_, gap);
        if (const char* why = struck()) {
            fail(why);
            return true;
        }
        if (hullClear()) {
            if (crewLeft() <= 0.f) fail("the other crew took the pass");
            else win();
            return true;
        }
        return false;
    };
    if (sub(DT * 0.5f)) {
        ambience();
        return;
    }
    if (sub(DT * 0.5f)) {
        ambience();
        return;
    }
    if (crewLeft() <= 0.f) fail("the other crew took the pass");
    ambience();
}

const char* Game::hint() const {
    if (mode_ == Mode::Pause) return "PAUSED";
    if (crewLeft() < 12.f) return "THE OTHER CREW IS CLOSING";
    float ahead = wallGap(x_ + std::sin(hdg_) * 2.f, y_ + std::cos(hdg_) * 4.f);
    if (ahead < 1.6f) return "TOO CLOSE TO THE WALL";
    float set = currentAt(y_ + 6.f);
    if (set > 0.45f) return "CURRENT SETS TO STARBOARD";
    if (set < -0.45f) return "CURRENT SETS TO PORT";
    if (halfAt(y_ + 6.f) < 5.7f) return "THE CUT. HOLD THE MIDDLE";
    if (y_ > LINE - 18.f) return "THE LEE LINE. TAKE HER THROUGH";
    return "FULL AHEAD. BEAT THE CREW";
}

const char* Game::telegraph() const {
    if (speed_ < -0.12f) return "ASTERN";
    if (speed_ < 0.25f) return "STOPPED";
    if (speed_ < 0.7f) return "DEAD SLOW";
    if (speed_ < 1.15f) return "SLOW";
    if (speed_ < 1.6f) return "HALF";
    if (speed_ < 1.9f) return "FULL";
    return "FULL AHEAD";
}

void Game::audio() {
    if (chimeStep_ >= 0) {
        chimeT_ -= DT;
        if (chimeT_ <= 0.f) {
            static const float notes[] = {349.2f, 440.f, 523.25f, 698.5f};
            if (chimeStep_ >= 4) {
                sys_->apu.tone(0, 0, 0);
                chimeStep_ = -1;
            } else {
                sys_->apu.tone(0, notes[chimeStep_], 0.07f);
                chimeStep_++;
                chimeT_ = 0.16f;
            }
        }
    } else if (mode_ != Mode::Play) {
        sys_->apu.tone(0, 0, 0);
    }

    if (hornT_ > 0.f) {
        hornT_ -= DT;
        sys_->apu.tone(1, 110.f, 0.06f);
    } else if (beepHold_ > 0.f) {
        beepHold_ -= DT;
        if (beepHold_ <= 0.f) sys_->apu.tone(1, 0, 0);
    } else if (mode_ == Mode::Play) {
        sys_->apu.tone(1, 0, 0);
        int whole = int(crewLeft());
        if (whole <= 10 && whole >= 0 && whole != beepSec_ && crewLeft() > 0.05f) {
            beepSec_ = whole;
            beepHold_ = 0.07f;
            sys_->apu.tone(1, whole <= 5 ? 880.f : 660.f, 0.045f);
        }
    } else {
        sys_->apu.tone(1, 0, 0);
    }

    if (mode_ == Mode::Play && (std::fabs(thrust_) > 0.04f || std::fabs(speed_) > 0.2f)) {
        float wob = 0.78f + 0.22f * std::sin(t_ * (14.f + std::fabs(thrust_) * 8.f));
        float vol = (0.018f + std::fabs(thrust_) * 0.042f) * wob;
        sys_->apu.tone(2, 48.f + std::fabs(thrust_) * 22.f + std::fabs(speed_) * 8.f, vol);
    } else {
        sys_->apu.tone(2, 0, 0);
    }

    float u = mode_ == Mode::Win ? 0.15f : urgency();
    float water = (mode_ == Mode::Play ? 0.012f + std::fabs(speed_) * 0.01f : 0.008f) + u * 0.045f;
    sys_->apu.noise(water, 120.f + std::fabs(speed_) * 40.f + u * 720.f, false);
    bool flash = flashOn();
    if (flash && !flashWas_ && mode_ == Mode::Play) sys_->apu.noiseBurst(0.28f, 160.f, 0.35f);
    flashWas_ = flash;
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

void Game::hudR(int row, const char* s, int pal) {
    int n = 0;
    if (s)
        while (s[n]) n++;
    hud(40 - n, row, s, pal);
}

void Game::hudC(int row, const char* s, int pal) {
    int n = 0;
    if (s)
        while (s[n]) n++;
    hud(20 - n / 2, row, s, pal);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow, bool flip, int fog) {
    if (h < 0.8f || m.h < 1) return;
    float w = h * float(m.w) / float(std::max(m.h, 1));
    if (cx + w * 0.5f < -8.f || cx - w * 0.5f > gs::SCREEN_W + 8.f) return;
    if (cy + h * 0.5f < -8.f || cy - h * 0.5f > gs::SCREEN_H + 8.f) return;
    gs::Sprite s;
    long sw = std::clamp(std::lround(w), 1L, 1800L);
    long sh = std::clamp(std::lround(h), 1L, 1800L);
    s.w = int16_t(sw);
    s.h = int16_t(sh);
    s.x = int16_t(std::clamp(std::lround(cx - sw * 0.5f), -2000L, 2000L));
    s.y = int16_t(std::clamp(std::lround(cy - sh * 0.5f), -2000L, 2000L));
    s.img = m.pick(float(sh));
    s.pal = uint8_t(pal);
    s.shadow = shadow;
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::place(const gs::Mipped& m, float wx, float wy, float worldH, int pal, bool flip, int fog) {
    spr(m, sx(wx), sy(wy), worldH * zoom_, pal, false, flip, fog);
}

void Game::text(const char* s, float x, float y, float scale, int pal) {
    if (!s || !*s) return;
    const float adv = 18.f * scale;
    const float left = x - float(std::strlen(s)) * adv * 0.5f;
    for (int i = 0; s[i]; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, left + (float(i) + 0.5f) * adv, y, float(g.h) * scale, pal);
    }
}

void Game::drawBoat(float wx, float wy, float hdg, int pal, float bob, int fog) {
    const gs::Mipped& hull = art_.tug[yawFrameOf(hdg)];
    float h = float(hull.h) * ((HALF_L * 2.f) * zoom_ / float(TUG_PX));
    float cx = sx(wx), cy = sy(wy) + bob;
    spr(art_.shadow, cx + 2.f, cy + h * 0.06f, h * 0.28f, PAL_FX, true, false, fog);
    spr(hull, cx, cy, h, pal, false, false, fog);
}

void Game::drawWorld() {
    const float top = camY_ + 150.f / std::max(zoom_, 0.4f);
    const float bot = camY_ - 160.f / std::max(zoom_, 0.4f);
    const float faceH = 3.6f;
    const float faceW = faceH * float(art_.face.w) / float(std::max(art_.face.h, 1));
    const float step = std::max(2.35f, (250.f / std::max(zoom_, 0.4f)) / 14.f);
    float reach = 176.f / std::max(zoom_, 0.4f);
    int layers = int(std::ceil((reach - 3.f) / std::max(faceW * 0.86f, 1.f)));
    layers = std::clamp(layers, 2, 5);

    int idx = 0;
    for (float wy = std::floor(bot / step) * step; wy < top + step; wy += step, idx++) {
        float c = centerAt(wy);
        float h = halfAt(wy);
        int fog = fogAt(wy);
        float jig = std::sin(wy * 1.7f) * 0.12f;
        for (int side = -1; side <= 1; side += 2) {
            for (int layer = 0; layer < layers; layer++) {
                bool ridge = layer >= 2;
                const gs::Mipped& m = ridge ? art_.ridge : art_.face;
                float mw = faceH * float(m.w) / float(std::max(m.h, 1));
                float inset = h + mw * 0.5f + float(layer) * mw * 0.82f - 0.25f;
                float wx = c + float(side) * inset;
                bool flip = side > 0;
                place(m, wx, wy + jig, faceH * (ridge ? 1.15f : 1.f), PAL_CLIFF, flip, fog);
            }
            if ((idx % 2) == 0) {
                float px = c + float(side) * (h + faceW * 1.15f);
                place(art_.pine, px, wy + 0.4f, 2.5f + float(idx % 3) * 0.25f, PAL_PINE, side > 0, fog);
            }
        }
        if ((idx % 3) == 0 && wy < 116.f) {
            place(art_.foam, c - h + 0.15f, wy, 0.85f, PAL_FX, false, fog);
            place(art_.foam, c + h - 0.15f, wy, 0.85f, PAL_FX, true, fog);
        }
    }

    auto bank = [&](float y, float side, const gs::Mipped& m, float worldH, int pal) {
        float x = centerAt(y) + side * (halfAt(y) + 0.35f);
        place(m, x, y, worldH, pal, side > 0.f, fogAt(y));
    };
    for (float y = 8.f; y < 112.f; y += 14.f) {
        bank(y, -1.f, art_.buoy, 1.55f, PAL_RED);
        bank(y + 7.f, 1.f, art_.buoy, 1.55f, PAL_GREEN);
    }
    bank(18.f, -1.f, art_.daymark, 2.4f, PAL_RED);
    bank(18.f, 1.f, art_.daymark, 2.4f, PAL_GREEN);
    bank(114.f, -1.f, art_.beacon, 3.3f, PAL_RED);
    bank(114.f, 1.f, art_.beacon, 3.3f, PAL_GREEN);
    bank(76.f, 1.f, art_.hut, 2.8f, PAL_HUT);

    float g1 = 26.f + std::fmod(t_ * 1.6f, 36.f);
    float g2 = 48.f + std::fmod(t_ * 1.1f + 8.f, 30.f);
    spr(art_.gull[int(t_ * 4.f) & 1], sx(centerAt(g1) + std::sin(t_ * 0.8f) * 2.4f), sy(g1), 14.f, PAL_BIRD, false,
        std::sin(t_) > 0.f);
    spr(art_.gull[int(t_ * 3.f) & 1], sx(centerAt(g2) - 1.8f), sy(g2 + std::sin(t_) * 0.4f), 12.f, PAL_BIRD);
}

void Game::drawStorm() {
    float u = mode_ == Mode::Win ? 0.12f : urgency();
    float ry = rivalY();
    if (u > 0.2f || mode_ != Mode::Title) {
        for (int row = 0; row < 3; row++) {
            float wy = ry - 1.2f - float(row) * 3.2f;
            for (int i = -3; i <= 3; i++) {
                float wx = camX_ + float(i) * 5.4f + ((row & 1) ? 2.4f : 0.f);
                spr(art_.cloud, sx(wx), sy(wy), (4.6f + float(row) * 0.6f) * zoom_ * 0.55f, PAL_STORM, false, i < 0,
                    2 + row * 3);
            }
        }
    }
    if (u > 0.34f) {
        int n = 5 + int(u * 12.f);
        for (int i = 0; i < n; i++) {
            float x = std::fmod(float(i) * 47.f + t_ * (30.f + u * 40.f), 340.f) - 10.f;
            float y = std::fmod(t_ * (90.f + float(i) * 11.f) + float(i) * 29.f, 260.f) - 16.f;
            spr(art_.rain, x, y, 14.f + float(i % 3) * 5.f, PAL_STORM);
        }
    }
    if (flashOn()) spr(art_.bolt, 48.f + std::fmod(t_ * 13.f, 180.f), 36.f, 92.f, PAL_STORM);
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    const bool flash = flashOn();
    float u = mode_ == Mode::Win ? 0.12f : (mode_ == Mode::Fail && why_ && std::strstr(why_, "crew") ? 1.f : urgency());
    const uint16_t clearLo = gs::rgb4(1, 5, 7);
    const uint16_t clearHi = gs::rgb4(2, 8, 10);
    const uint16_t stormLo = gs::rgb4(2, 2, 3);
    const uint16_t stormHi = gs::rgb4(4, 4, 6);
    const uint16_t lee = gs::rgb4(2, 9, 11);
    const uint16_t bolt = gs::rgb4(13, 13, 12);
    v.setFogColor(flash ? gs::rgb4(8, 8, 9) : gs::rgb4(3, 3, 4));
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (112.f - float(y)) / std::max(zoom_, 0.2f);
        float band = std::fmod(wy * 0.42f - t_ * 0.55f, 1.f);
        if (band < 0.f) band += 1.f;
        bool streak = band < 0.16f;
        uint16_t lo = lerpColor(streak ? clearHi : clearLo, streak ? stormHi : stormLo, u);
        if (wy > LINE - 4.f) {
            float k = clampf((wy - (LINE - 4.f)) / 10.f, 0.f, 1.f);
            lo = lerpColor(lo, lee, k * (1.f - u * 0.55f));
        }
        float behind = 0.f;
        if (wy < rivalY()) behind = clampf((rivalY() - wy) / 14.f, 0.f, 1.f);
        lo = lerpColor(lo, stormLo, std::max(behind * 0.85f, u * 0.25f));
        if (flash) lo = lerpColor(lo, bolt, 0.55f);
        v.lineBackdrop[y] = lo;
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::draw() {
    shx_ = shy_ = 0.f;
    if (shake_ > 0.f) {
        shx_ = std::sin(t_ * 90.f) * 3.4f * shake_;
        shy_ = std::cos(t_ * 70.f) * 2.2f * shake_;
        shake_ = std::max(0.f, shake_ - DT * 1.6f);
    }
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.hudEnabled = true;
    backdrop();

    bool late = why_ && std::strstr(why_, "crew");
    if (mode_ == Mode::Title) text("S3 TUGBOAT PASS", 160.f, 18.f, 0.62f, PAL_AMBER);
    else if (mode_ == Mode::Win) text("CLEAR", 160.f, 28.f, 1.15f, PAL_GREEN);
    else if (mode_ == Mode::Fail) text(late ? "TOO LATE" : "STRUCK", 160.f, 30.f, 1.05f, PAL_RED);
    else if (mode_ == Mode::Pause) text("PAUSED", 160.f, 96.f, 1.f, PAL_HUD);

    drawStorm();
    float bob = std::sin(t_ * 2.1f) * 0.7f;
    float rBob = std::sin(t_ * 1.7f + 1.1f) * 0.55f;
    drawBoat(x_, y_, hdg_, PAL_TUG, bob, 0);
    if (std::fabs(speed_) > 0.3f && mode_ != Mode::Title) {
        float fx = std::sin(hdg_), fy = std::cos(hdg_);
        spr(art_.foam, sx(x_ + fx * HALF_L * 0.92f), sy(y_ + fy * HALF_L * 0.92f) + bob, 8.f + std::fabs(speed_) * 3.f,
            PAL_FX);
    }
    drawBoat(rivalX(), rivalY(), rivalHeading(), PAL_RIVAL, rBob, 3);
    float crewSy = sy(rivalY());
    if (crewSy > 8.f && crewSy < gs::SCREEN_H - 8.f) text("CREW", sx(rivalX()), crewSy - 28.f, 0.38f, PAL_AMBER);

    for (const Puff& s : smokes_) {
        if (s.life <= 0.f) continue;
        float rise = (1.f - s.life) * 18.f;
        float wind = urgency() * 14.f * (1.f - s.life);
        spr(art_.smoke, sx(s.x) + wind + std::sin(t_ + s.seed) * 3.f, sy(s.y) - rise, 7.f + (1.f - s.life) * 10.f,
            PAL_FX, false, false, int((1.f - s.life) * 8.f));
    }
    for (const Puff& w : wakes_) {
        if (w.life <= 0.f) continue;
        float z = (7.f + (1.f - w.life) * 12.f) * (zoom_ / PLAY_ZOOM);
        spr(art_.wake, sx(w.x), sy(w.y), z, PAL_FX, false, false, int((1.f - w.life) * 10.f));
    }
    drawWorld();

    char buf[64];
    int sec = std::max(0, int(std::ceil(crewLeft() - 1e-3f)));
    std::snprintf(buf, sizeof buf, "CREW %d:%02d", sec / 60, sec % 60);
    const int crewPal = sec <= 15 ? PAL_RED : PAL_AMBER;
    float stern = y_ - std::cos(hdg_) * HALF_L;
    int lee = std::max(0, int(std::lround(LINE - stern)));

    if (mode_ == Mode::Play || mode_ == Mode::Pause) {
        hud(1, 0, "S3 TUGBOAT PASS", PAL_AMBER);
        hudR(0, buf, crewPal);
        hud(1, 1, hint(), sec <= 12 ? PAL_RED : PAL_HUD);
        hud(1, 2, telegraph(), PAL_GREEN);
        std::snprintf(buf, sizeof buf, "LEE %dM", lee);
        hudR(2, buf, PAL_GREEN);
        hud(1, 26, "ARROWS DRIVE    Z HORN", PAL_HUD);
    } else if (mode_ == Mode::Title) {
        hudC(8, "TAKE THE TUG AND CLEAR THE PASS", PAL_AMBER);
        hudC(10, "BEFORE THE STORM CLOCK", PAL_HUD);
        hudC(11, "THE CLOCK IS THE OTHER CREW", PAL_HUD);
        hudC(13, "A TOUCH ON THE WALL FAILS IT", PAL_HUD);
        std::snprintf(buf, sizeof buf, "THEIR CLOCK %d:%02d", sec / 60, sec % 60);
        hudC(15, buf, PAL_AMBER);
        hudC(17, int(t_ * 2.f) % 2 == 0 ? "ENTER TO TAKE THE TUG" : "ARROWS STEER AND DRIVE", PAL_HUD);
    } else if (mode_ == Mode::Win) {
        hudC(8, "CLEARED THE PASS", PAL_GREEN);
        hudC(9, "AHEAD OF THE OTHER CREW", PAL_HUD);
        int took = int(std::lround(playT_));
        int left = int(std::floor(crewLeft()));
        std::snprintf(buf, sizeof buf, "%d:%02d   CREW HAD %d:%02d", took / 60, took % 60, left / 60, left % 60);
        hudC(11, buf, PAL_AMBER);
        hudC(13, "ENTER TAKES THE NEXT PASS", PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        hudC(9, why_ && why_[0] ? why_ : "THE PASS CLOSED", PAL_RED);
        hudC(11, "ENTER TRIES THE PASS AGAIN", PAL_HUD);
    }
    hudR(27, S3_VERSION_STRING, PAL_HUD);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.apu.setMaster(0.8f);
    sys.apu.setEcho(0.16f, 0.22f, 0.12f);
    t_ = 0.f;
    if (bot_) begin();
    else showTitle();
    sys.setLight(30, 100, 120);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || bot_) begin();
        else if (pad.pressed(gs::BTN_MODE) && !bot_) sys.quit();
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else step();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
        else if (pad.pressed(gs::BTN_MODE)) showTitle();
    } else if (!bot_) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) begin();
        else if (pad.pressed(gs::BTN_MODE)) showTitle();
    }

    if (mode_ != Mode::Play) ambience();
    followCamera();
    draw();
    audio();

    if (mode_ == Mode::Play) {
        float u = urgency();
        sys.setLight(int(24 + u * 90), int(100 - u * 60), int(130 - u * 50));
    } else if (mode_ == Mode::Title) {
        sys.setLight(30, 100, 120);
    }
}

}  // namespace tugpass

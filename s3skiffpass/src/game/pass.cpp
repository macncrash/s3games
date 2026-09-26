#include "pass.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace skiffpass {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float PI = 3.14159265f;
constexpr float TAU = 6.2831853f;
constexpr float MOUTH = 122.f;
constexpr float STORM0 = -8.f;
constexpr float CLOCK_MAX = 63.f;
constexpr float STORM_V = (MOUTH - STORM0) / CLOCK_MAX;
constexpr float START_Y = 8.f;
constexpr float BOAT_M = 4.35f;
constexpr float HALF_L = 2.05f;
constexpr float HALF_B = 0.70f;
constexpr float MAX_SPD = 2.65f;
constexpr float DRAG_D = 3.1f;
constexpr float PLAY_ZOOM = 10.2f;
constexpr float TITLE_ZOOM = 4.05f;

struct SkerryDef {
    float y, bias, r;
};

constexpr SkerryDef kSkerries[] = {
    {24.f, -0.50f, 1.18f},
    {46.f, 0.52f, 1.26f},
    {68.f, -0.48f, 1.14f},
    {90.f, 0.55f, 1.30f},
    {108.f, -0.42f, 1.12f},
};

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
    return 4.8f * std::sin(y * 0.046f) + 2.1f * std::sin(y * 0.097f + 0.9f);
}

float Game::halfAt(float y) const {
    float w = 6.7f;
    w -= 1.45f * gauss(y, 34.f, 5.2f);
    w -= 1.55f * gauss(y, 88.f, 5.4f);
    w -= 1.50f * gauss(y, 112.f, 4.2f);
    if (y < 14.f) w += (14.f - std::max(y, 0.f)) / 14.f * 2.4f;
    if (y > 114.f) {
        float u = clampf((y - 114.f) / 8.f, 0.f, 1.f);
        u = u * u * (3.f - 2.f * u);
        w += u * 7.5f;
    }
    return w;
}

float Game::clearance(float x, float y) const {
    float gap = halfAt(y) - std::fabs(x - centerAt(y));
    for (int i = 0; i < rockN_; i++) {
        float dx = x - rock_[i].x;
        float dy = y - rock_[i].y;
        float d = std::sqrt(dx * dx + dy * dy) - (rock_[i].r + 1.45f);
        gap = std::min(gap, d);
    }
    return gap;
}

float Game::pathX(float y) const {
    if (y <= 0.f) return path_[0];
    if (y >= float(PATH_N - 1)) return path_[PATH_N - 1];
    int i = int(y);
    float f = y - float(i);
    return path_[i] * (1.f - f) + path_[i + 1] * f;
}

float Game::clockLeft() const { return std::max(0.f, (MOUTH - stormY_) / STORM_V); }

float Game::sx(float wx) const { return 160.f + (wx - camX_) * zoom_ + shx_; }
float Game::sy(float wy) const { return 118.f - (wy - camY_) * zoom_ + shy_; }

int Game::yawFrame() const {
    float u = std::fmod(hdg_, TAU);
    if (u < 0.f) u += TAU;
    int i = int(std::lround(u / TAU * float(YAWS))) % YAWS;
    if (i < 0) i += YAWS;
    return i;
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Win || mode_ == Mode::Fail || over_) return 4;
    if (y_ >= 108.f) return 3;
    if (y_ >= 58.f) return 2;
    return 1;
}

float Game::urgency() const {
    if (mode_ == Mode::Title) return 0.55f;
    float u = 1.f - clampf(clockLeft() / CLOCK_MAX, 0.f, 1.f);
    float near = clampf(1.f - (y_ - stormY_) / 30.f, 0.f, 1.f);
    return clampf(u * 0.85f + near * 0.5f, 0.f, 1.f);
}

bool Game::flashOn() const {
    float u = urgency();
    if (u < 0.42f && mode_ != Mode::Title) return false;
    float phase = std::fmod(t_ * 0.37f, 1.f);
    if (phase < 0.04f) return true;
    return u > 0.72f && std::fmod(t_ * 0.21f, 1.f) < 0.03f;
}

void Game::sampleHull(float* xs, float* ys, int& n) const {
    n = 0;
    const float fx = std::sin(hdg_), fy = std::cos(hdg_);
    const float rx = std::cos(hdg_), ry = -std::sin(hdg_);
    const float along[] = {1.f, 0.5f, 0.f, -0.5f, -1.f};
    const float side[] = {-1.f, 0.f, 1.f};
    for (float a : along) {
        for (float b : side) {
            xs[n] = x_ + fx * a * HALF_L + rx * b * HALF_B;
            ys[n] = y_ + fy * a * HALF_L + ry * b * HALF_B;
            n++;
        }
    }
}

void Game::bake() {
    rockN_ = 0;
    for (const SkerryDef& s : kSkerries) {
        if (rockN_ >= 8) break;
        float h = halfAt(s.y);
        float reach = std::max(1.1f, h - s.r - 0.5f);
        Rock& r = rock_[rockN_++];
        r.y = s.y;
        r.r = s.r;
        r.bias = s.bias;
        r.x = centerAt(s.y) + s.bias * reach;
    }
    for (int i = 0; i < PATH_N; i++) {
        float y = float(i);
        float c = centerAt(y);
        float h = halfAt(y);
        float lo = c - h + 1.f;
        float hi = c + h - 1.f;
        float best = c;
        float bestScore = -1e9f;
        if (hi <= lo) {
            path_[i] = c;
            continue;
        }
        for (float x = lo; x <= hi; x += 0.25f) {
            float cl = clearance(x, y);
            float score = cl - 0.01f * std::fabs(x - c);
            if (score > bestScore) {
                bestScore = score;
                best = x;
            }
        }
        path_[i] = best;
    }
    for (int pass = 0; pass < 3; pass++) {
        float nxt[PATH_N];
        for (int i = 0; i < PATH_N; i++) {
            float acc = 0.f;
            int cnt = 0;
            for (int k = -3; k <= 3; k++) {
                int j = std::clamp(i + k, 0, PATH_N - 1);
                acc += path_[j];
                cnt++;
            }
            float a = acc / float(cnt);
            nxt[i] = clearance(a, float(i)) >= 1.05f ? a : path_[i];
        }
        for (int i = 0; i < PATH_N; i++) path_[i] = nxt[i];
    }
}

void Game::resetPose() {
    over_ = false;
    won_ = false;
    stormFail_ = false;
    why_ = "";
    playT_ = 0;
    speed_ = 0;
    yawV_ = 0;
    driftX_ = 0;
    driftY_ = 0;
    thrust_ = 0;
    steer_ = 0;
    throttle_ = 0;
    gust_ = 0;
    stormY_ = STORM0;
    shake_ = 0;
    wakeT_ = 0;
    scrapeT_ = -1.f;
    beepHold_ = 0;
    beepSec_ = -1;
    wakeN_ = 0;
    chimeStep_ = -1;
    flashWas_ = false;
    for (Wake& w : wakes_) w = {};
    bake();
    x_ = pathX(START_Y);
    y_ = START_Y;
    float dx = pathX(START_Y + 7.f) - x_;
    hdg_ = std::atan2(dx, 7.f);
}

void Game::snapCamera() {
    if (mode_ == Mode::Title) {
        camX_ = centerAt(24.f);
        camY_ = 18.f;
        zoom_ = TITLE_ZOOM;
    } else if (mode_ == Mode::Win) {
        camX_ = x_;
        camY_ = y_ + 1.2f;
        zoom_ = 7.4f;
    } else {
        camX_ = x_;
        camY_ = y_ + 2.6f;
        zoom_ = PLAY_ZOOM;
    }
}

void Game::followCamera() {
    float tx = x_, ty = y_ + 2.6f + std::max(0.f, speed_) * 0.35f, tz = PLAY_ZOOM;
    if (mode_ == Mode::Title) {
        tx = centerAt(24.f);
        ty = 18.f;
        tz = TITLE_ZOOM;
    } else if (mode_ == Mode::Win) {
        tx = x_;
        ty = y_ + 1.1f;
        tz = 7.4f;
    } else if (mode_ == Mode::Fail) {
        tx = x_;
        ty = y_ + 2.2f;
        tz = PLAY_ZOOM * 0.92f;
    }
    float k = 1.f - std::exp(-DT * 6.f);
    camX_ += (tx - camX_) * k;
    camY_ += (ty - camY_) * k;
    zoom_ += (tz - zoom_) * k;
    if (zoom_ < 1.f) zoom_ = 1.f;
}

void Game::begin() {
    resetPose();
    mode_ = Mode::Play;
    snapCamera();
    chime(false);
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
    stormFail_ = why && std::strncmp(why, "the storm", 9) == 0;
    shake_ = 1.f;
    sys_->apu.noiseBurst(0.5f, 420.f, 0.32f);
    sys_->apu.tone(2, 0, 0);
    sys_->rumble(0.7f, 0.35f, 180);
    sys_->setLight(170, 28, 18);
}

void Game::win() {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Win;
    over_ = true;
    won_ = true;
    why_ = "clear";
    stormFail_ = false;
    chime(true);
    sys_->rumble(0.3f, 0.15f, 160);
    sys_->setLight(40, 150, 80);
}

void Game::chime(bool big) {
    chimeBig_ = big;
    chimeStep_ = 0;
    chimeT_ = 0.02f;
}

void Game::scrape() {
    if (t_ - scrapeT_ < 0.14f) return;
    scrapeT_ = t_;
    sys_->apu.noiseBurst(0.16f, 860.f, 0.1f);
    sys_->rumble(0.22f, 0.08f, 40);
}

void Game::updateGust() {
    float stormF = clampf((stormY_ - STORM0) / (MOUTH - STORM0), 0.f, 1.f);
    float amp = 0.22f + stormF * stormF * 0.85f;
    float wave = std::sin(playT_ * 0.77f) * amp + std::sin(playT_ * 0.29f + 1.4f) * amp * 0.4f;
    float funnel = 0.f;
    if (y_ > 72.f && y_ < 116.f) funnel = std::sin((y_ - 72.f) * 0.11f) * 0.45f;
    gust_ = wave + funnel;
}

void Game::pilot(float& thrust, float& steer) const {
    // Steer the bow onto the baked line. A long look cuts the skerry; a short one tracks it.
    float bowX = x_ + std::sin(hdg_) * HALF_L;
    float bowY = y_ + std::cos(hdg_) * HALF_L;
    float look = 5.2f;
    float tx = pathX(bowY + look);
    float crab = gust_ / DRAG_D;
    tx -= crab * (look / std::max(speed_, 1.3f)) * 0.65f;
    float cc = centerAt(bowY + 3.f);
    float hh = halfAt(bowY + 3.f);
    tx = clampf(tx, cc - hh + 1.2f, cc + hh - 1.2f);

    float cross = pathX(y_ + 1.6f) - x_;
    float wantH = std::atan2(tx - bowX, look) + clampf(cross * 0.28f, -0.4f, 0.4f);
    wantH = clampf(wantH, -0.9f, 0.9f);
    float hErr = wrapPi(wantH - hdg_);
    steer = hErr * 3.3f - yawV_ * 0.85f;

    float aheadY = y_ + 2.4f;
    float c = centerAt(aheadY);
    float h = halfAt(aheadY);
    float predX = x_ + std::sin(hdg_) * 2.6f + crab * 0.35f;
    float lg = predX - (c - h);
    float rg = (c + h) - predX;
    if (lg < 1.8f) steer += (1.8f - lg) * 1.25f;
    if (rg < 1.8f) steer -= (1.8f - rg) * 1.25f;

    float rockNear = 99.f;
    for (int i = 0; i < rockN_; i++) {
        float rdx = bowX - rock_[i].x;
        float rdy = rock_[i].y - bowY;
        if (rdy < -1.5f || rdy > 12.f) continue;
        float dist = std::sqrt(rdx * rdx + rdy * rdy);
        rockNear = std::min(rockNear, dist - rock_[i].r);
        float lim = rock_[i].r + 2.7f;
        if (dist < lim) {
            float away = rdx >= 0.f ? 1.f : -1.f;
            steer += away * (lim - dist) * 0.85f;
        }
    }
    steer = clampf(steer, -1.f, 1.f);

    float wantSpd = MAX_SPD;
    if (std::fabs(cross) > 1.35f) wantSpd = 1.85f;
    if (std::fabs(hErr) > 0.5f) wantSpd = std::min(wantSpd, 1.7f);
    if (rockNear < 2.2f) wantSpd = std::min(wantSpd, 1.75f);
    float need = (MOUTH - y_) / std::max(0.4f, clockLeft());
    if (need > 2.15f && clearance(x_, y_) > 1.4f && std::fabs(cross) < 1.1f) wantSpd = MAX_SPD;
    thrust = clampf((wantSpd - speed_) * 2.2f, -1.f, 1.f);
}

void Game::controls() {
    const gs::Pad& p = sys_->pad;
    steer_ = 0.f;
    if (p.down(gs::BTN_LEFT)) steer_ -= 1.f;
    if (p.down(gs::BTN_RIGHT)) steer_ += 1.f;
    if (std::fabs(p.axisX) > 0.15f) steer_ = clampf(p.axisX, -1.f, 1.f);

    float want = 0.f;
    const bool ahead = p.down(gs::BTN_UP) || p.down(gs::BTN_C) || p.down(gs::BTN_TURBO);
    const bool astern = p.down(gs::BTN_DOWN) || p.down(gs::BTN_B);
    if (ahead) want = 1.f;
    if (astern) want = -1.f;
    if (p.accel > 0.08f) want = std::max(want, p.accel);
    if (p.brake > 0.08f) want = std::min(want, -p.brake);
    float k = 1.f - std::exp(-5.5f * DT);
    throttle_ += (want - throttle_) * k;
    thrust_ = throttle_;
}

void Game::integrate(float dt) {
    yawV_ += steer_ * (1.65f + std::fabs(speed_) * 0.42f) * dt;
    yawV_ -= yawV_ * 3.5f * dt;
    yawV_ = clampf(yawV_, -1.5f, 1.5f);
    hdg_ = wrapPi(hdg_ + yawV_ * dt);

    float drive = thrust_ * (thrust_ >= 0.f ? 4.5f : 2.7f);
    speed_ += drive * dt;
    speed_ -= speed_ * 0.92f * dt;
    speed_ = clampf(speed_, -1.15f, MAX_SPD);

    driftX_ += gust_ * dt;
    driftX_ += std::cos(hdg_) * steer_ * 0.55f * dt;
    driftY_ -= std::sin(hdg_) * steer_ * 0.55f * dt;
    driftX_ -= driftX_ * DRAG_D * dt;
    driftY_ -= driftY_ * DRAG_D * dt;

    x_ += (std::sin(hdg_) * speed_ + driftX_) * dt;
    y_ += (std::cos(hdg_) * speed_ + driftY_) * dt;
    if (y_ < 3.5f) {
        y_ = 3.5f;
        if (speed_ < 0.f) speed_ = 0.f;
        driftY_ = std::max(0.f, driftY_);
    }
}

const char* Game::contacts() {
    float xs[16], ys[16];
    int n = 0;
    sampleHull(xs, ys, n);
    for (int i = 0; i < n; i++) {
        for (int r = 0; r < rockN_; r++) {
            float dx = xs[i] - rock_[r].x;
            float dy = ys[i] - rock_[r].y;
            if (dx * dx + dy * dy < rock_[r].r * rock_[r].r) return "grounded on a skerry";
        }
    }
    float maxR = 0.f, maxL = 0.f, impact = 0.f;
    for (int i = 0; i < n; i++) {
        float c = centerAt(ys[i]);
        float h = halfAt(ys[i]);
        float right = xs[i] - (c + h);
        float left = (c - h) - xs[i];
        if (right > 0.f) {
            maxR = std::max(maxR, right);
            impact = std::max(impact, std::sin(hdg_) * speed_ + driftX_);
        }
        if (left > 0.f) {
            maxL = std::max(maxL, left);
            impact = std::max(impact, -(std::sin(hdg_) * speed_ + driftX_));
        }
    }
    if (maxR > 0.35f && maxL > 0.35f) return "grounded on the cliff";
    if ((maxR > 0.01f || maxL > 0.01f) && impact > 1.35f) return "grounded on the cliff";
    if (maxR > 0.5f || maxL > 0.5f) return "grounded on the cliff";
    if (maxR > 0.f || maxL > 0.f) {
        if (maxR > maxL) x_ -= maxR;
        else x_ += maxL;
        if (maxR > 0.f && driftX_ > 0.f) driftX_ *= 0.35f;
        if (maxL > 0.f && driftX_ < 0.f) driftX_ *= 0.35f;
        speed_ *= 0.985f;
        yawV_ *= 0.85f;
        shake_ = std::max(shake_, 0.35f);
        scrape();
    }
    return nullptr;
}

bool Game::boatClear() const {
    float xs[16], ys[16];
    int n = 0;
    sampleHull(xs, ys, n);
    for (int i = 0; i < n; i++)
        if (ys[i] < MOUTH) return false;
    return n > 0;
}

bool Game::stormTouches() const {
    float xs[16], ys[16];
    int n = 0;
    sampleHull(xs, ys, n);
    for (int i = 0; i < n; i++)
        if (ys[i] <= stormY_) return true;
    return false;
}

void Game::wakes() {
    wakeT_ -= DT;
    if (wakeT_ <= 0.f && std::fabs(speed_) > 0.4f) {
        wakeT_ = 0.07f;
        Wake& w = wakes_[wakeN_++ % 16];
        w.x = x_ - std::sin(hdg_) * HALF_L * 0.85f;
        w.y = y_ - std::cos(hdg_) * HALF_L * 0.85f;
        w.life = 1.f;
    }
    for (Wake& w : wakes_)
        if (w.life > 0.f) w.life -= DT * 0.7f;
}

void Game::step() {
    playT_ += DT;
    stormY_ = STORM0 + STORM_V * playT_;
    updateGust();
    if (bot_) {
        pilot(thrust_, steer_);
        throttle_ = thrust_;
    } else {
        controls();
    }
    auto advance = [&](float dt) -> bool {
        integrate(dt);
        if (const char* why = contacts()) {
            fail(why);
            return true;
        }
        if (boatClear() && stormY_ < MOUTH) {
            win();
            return true;
        }
        if (stormTouches()) {
            fail("the storm took the skiff");
            return true;
        }
        if (stormY_ >= MOUTH) {
            fail("the storm closed the pass");
            return true;
        }
        return false;
    };
    if (advance(DT * 0.5f)) return;
    if (advance(DT * 0.5f)) return;
    wakes();
}

const char* Game::hint() const {
    if (mode_ == Mode::Pause) return "PAUSED";
    if (mode_ == Mode::Win) return "THE SKIFF IS THROUGH";
    if (mode_ == Mode::Fail) return why_ ? why_ : "THE PASS CLOSED";
    if (clockLeft() < 12.f) return "THE STORM IS CLOSING THE PASS";
    for (int i = 0; i < rockN_; i++) {
        float dy = rock_[i].y - y_;
        if (dy > 0.f && dy < 14.f) return rock_[i].bias >= 0.f ? "SKERRY TO STARBOARD" : "SKERRY TO PORT";
    }
    if (halfAt(y_ + 6.f) < 5.6f) return "NARROWS. HOLD THE MIDDLE";
    if (y_ > 108.f) return "THE MOUTH. TAKE HER THROUGH";
    return "OUTRUN THE STORM";
}

void Game::audio() {
    if (chimeStep_ >= 0) {
        chimeT_ -= DT;
        if (chimeT_ <= 0.f) {
            static const float winN[] = {392.f, 523.25f, 659.25f, 784.f};
            static const float castN[] = {330.f, 440.f};
            const float* notes = chimeBig_ ? winN : castN;
            const int count = chimeBig_ ? 4 : 2;
            if (chimeStep_ >= count) {
                sys_->apu.tone(0, 0, 0);
                chimeStep_ = -1;
            } else {
                sys_->apu.tone(0, notes[chimeStep_], chimeBig_ ? 0.07f : 0.045f);
                chimeStep_++;
                chimeT_ = 0.16f;
            }
        }
    } else if (mode_ != Mode::Play) {
        sys_->apu.tone(0, 0, 0);
    }

    if (beepHold_ > 0.f) {
        beepHold_ -= DT;
        if (beepHold_ <= 0.f) sys_->apu.tone(1, 0, 0);
    } else if (mode_ == Mode::Play) {
        int whole = int(clockLeft());
        if (whole <= 10 && whole >= 0 && whole != beepSec_ && clockLeft() > 0.05f) {
            beepSec_ = whole;
            beepHold_ = 0.07f;
            sys_->apu.tone(1, whole <= 5 ? 880.f : 660.f, 0.045f);
        }
    }

    if (mode_ == Mode::Play && (std::fabs(thrust_) > 0.05f || std::fabs(speed_) > 0.25f)) {
        float wob = 0.75f + 0.25f * std::sin(t_ * 31.f);
        float vol = (0.016f + std::fabs(thrust_) * 0.03f) * wob;
        sys_->apu.tone(2, 48.f + std::fabs(thrust_) * 36.f + std::fabs(speed_) * 7.f, vol);
    } else {
        sys_->apu.tone(2, 0, 0);
    }

    float u = urgency();
    float water = (mode_ == Mode::Play ? 0.012f + std::fabs(speed_) * 0.008f : 0.01f) + u * 0.04f;
    float rate = 140.f + std::fabs(speed_) * 30.f + u * 780.f;
    sys_->apu.noise(water, rate, false);

    bool flash = flashOn();
    if (flash && !flashWas_ && mode_ == Mode::Play) sys_->apu.noiseBurst(0.22f + u * 0.2f, 180.f, 0.4f);
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

void Game::place(const gs::Mipped& m, float wx, float wy, float worldH, int pal, bool flip) {
    spr(m, sx(wx), sy(wy), worldH * zoom_, pal, false, flip);
}

void Game::text(const char* s, float x, float y, float scale, int pal) {
    if (!s || !*s) return;
    const float adv = 18.f * scale;
    const float left = x - float(std::strlen(s)) * adv * 0.5f;
    for (int i = 0; s[i]; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        float h = float(g.h) * scale;
        spr(g, left + (float(i) + 0.5f) * adv, y, h, pal);
    }
}

void Game::drawBoat() {
    const gs::Mipped& hull = art_.skiff[yawFrame()];
    float h = float(hull.h) * (BOAT_M * zoom_ / HULL_PX);
    float bob = std::sin(t_ * 1.8f + x_) * 1.5f;
    float cx = sx(x_), cy = sy(y_) + bob;
    spr(hull, cx, cy, h, PAL_BOAT);
    if (std::fabs(thrust_) > 0.2f || std::fabs(speed_) > 0.8f) {
        float fx = std::sin(hdg_), fy = std::cos(hdg_);
        spr(art_.foam, sx(x_ - fx * HALF_L * 0.92f), sy(y_ - fy * HALF_L * 0.92f) + bob, 8.f + std::fabs(speed_) * 3.f,
            PAL_FX);
    }
}

void Game::drawWorld() {
    const float top = camY_ + 150.f / zoom_;
    const float bot = camY_ - 160.f / zoom_;

    float g1 = centerAt(y_ + 8.f) + std::sin(t_ * 0.6f) * 3.4f;
    float g2 = centerAt(y_ + 14.f) + std::cos(t_ * 0.45f) * 2.6f;
    spr(art_.gull[int(t_ * 4.f) & 1], sx(g1), sy(y_ + 7.f + std::sin(t_ * 0.8f)), 15.f, PAL_BIRD, false, std::sin(t_) > 0);
    spr(art_.gull[int(t_ * 3.f) & 1], sx(g2), sy(y_ + 13.f), 12.f, PAL_BIRD, false, std::cos(t_) > 0);

    if (flashOn()) {
        float bx = 36.f + std::fmod(t_ * 17.f, 250.f);
        spr(art_.bolt, bx, 40.f, 78.f, PAL_STORM);
    }

    for (int i = 0; i < rockN_; i++) {
        const Rock& r = rock_[i];
        if (r.y < bot - 3.f || r.y > top + 3.f) continue;
        float side = r.bias >= 0.f ? -1.f : 1.f;
        float bx = r.x + side * (r.r + 1.15f);
        float bc = centerAt(r.y);
        float bh = halfAt(r.y);
        bx = clampf(bx, bc - bh + 0.8f, bc + bh - 0.8f);
        place(art_.buoy, bx, r.y + 0.12f * std::sin(t_ * 1.6f + r.y), 2.15f, PAL_BUOY);
        place(art_.skerry, r.x, r.y, r.r * 2.45f, PAL_ROCK);
        place(art_.foam, r.x, r.y + r.r * 0.35f, r.r * 1.35f, PAL_FX);
    }

    float by = 115.f;
    place(art_.beacon, centerAt(by) - halfAt(by) - 0.15f, by, 3.3f, PAL_GREEN);
    place(art_.beacon, centerAt(by) + halfAt(by) + 0.15f, by, 3.3f, PAL_RED);

    float sySign = 12.f;
    place(art_.sign, centerAt(sySign) - halfAt(sySign) - 1.7f, sySign, 3.05f, PAL_SIGN);
    float hy = 58.f;
    place(art_.hut, centerAt(hy) + halfAt(hy) + 2.2f, hy, 3.15f, PAL_HUT);

    const float step = 2.35f;
    const float cliffH = 3.7f;
    const float aspect = float(art_.cliff.w) / float(std::max(art_.cliff.h, 1));
    const float cliffW = cliffH * aspect;
    int idx = 0;
    for (float wy = std::floor(bot / step) * step; wy < top; wy += step, idx++) {
        if (wy > 117.5f) continue;
        float c = centerAt(wy);
        float h = halfAt(wy);
        if ((idx % 2) == 0) {
            place(art_.foam, c - h + 0.25f, wy, 1.05f, PAL_FX);
            place(art_.foam, c + h - 0.25f, wy, 1.05f, PAL_FX);
        }
        if ((idx % 2) == 0) {
            place(art_.pine, c - h - cliffW - 0.35f, wy + 0.3f, 2.7f, PAL_PINE, false);
            place(art_.pine, c + h + cliffW + 0.35f, wy - 0.2f, 2.5f, PAL_PINE, true);
        }
        place(art_.cliff, c - h - cliffW * 0.5f + 0.4f, wy, cliffH, PAL_CLIFF, false);
        place(art_.cliff, c + h + cliffW * 0.5f - 0.4f, wy, cliffH, PAL_CLIFF, true);
        if ((idx % 4) == 0) {
            place(art_.cliff, c - h - cliffW - 1.4f, wy, cliffH * 1.65f, PAL_CLIFF, false);
            place(art_.cliff, c + h + cliffW + 1.4f, wy, cliffH * 1.55f, PAL_CLIFF, true);
        }
    }
}

void Game::drawStorm() {
    const float top = camY_ + 140.f / zoom_;
    const float bot = camY_ - 150.f / zoom_;
    if (stormY_ >= bot - 6.f && stormY_ <= top + 2.f) {
        for (int row = 0; row < 3; row++) {
            float wy = stormY_ - float(row) * 2.5f;
            if (wy > top || wy < bot - 2.f) continue;
            for (int i = -3; i <= 3; i++) {
                float wx = camX_ + float(i) * 4.8f + ((row & 1) ? 2.2f : 0.f);
                spr(art_.cloud, sx(wx), sy(wy), (5.2f + float(row) * 0.45f) * zoom_, PAL_STORM, false, i < 0,
                    3 + row * 3);
            }
        }
    }
    float u = urgency();
    if (u > 0.22f) {
        int n = 6 + int(u * 10.f);
        for (int i = 0; i < n; i++) {
            float x = std::fmod(float(i) * 47.f + t_ * 22.f, 340.f) - 10.f;
            float y = std::fmod(t_ * (80.f + float(i) * 9.f) + float(i) * 37.f, 250.f) - 14.f;
            spr(art_.rain, x, y, 12.f + float(i % 3) * 4.f, PAL_STORM);
        }
    }

    spr(art_.shadow, sx(x_), sy(y_) + 3.f, 16.f * (zoom_ / PLAY_ZOOM), PAL_FX, true);
    for (const Wake& w : wakes_) {
        if (w.life <= 0.f) continue;
        float z = (6.f + (1.f - w.life) * 11.f) * (zoom_ / PLAY_ZOOM);
        spr(art_.ripple, sx(w.x), sy(w.y), z, PAL_FX, false, false, int((1.f - w.life) * 12.f));
    }
    for (int i = 0; i < 5; i++) {
        float wy = y_ - 1.5f + float(i) * 2.8f;
        float wx = centerAt(wy) + std::sin(t_ * 0.6f + float(i) * 1.4f) * 1.6f;
        spr(art_.ripple, sx(wx), sy(wy), 11.f, PAL_FX, false, false, 2);
    }
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    const bool flash = flashOn();
    float u = urgency();
    const uint16_t clearLo = gs::rgb4(1, 5, 7);
    const uint16_t clearHi = gs::rgb4(2, 8, 10);
    const uint16_t stormLo = gs::rgb4(1, 2, 3);
    const uint16_t stormHi = gs::rgb4(2, 3, 5);
    const uint16_t sea = gs::rgb4(2, 8, 9);
    const uint16_t bolt = gs::rgb4(12, 13, 14);
    v.setFogColor(flash ? gs::rgb4(8, 9, 10) : gs::rgb4(2, 3, 4));
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (118.f - float(y)) / zoom_;
        float band = std::fmod(wy * 0.55f - t_ * (0.7f + std::fabs(speed_) * 0.12f), 1.f);
        if (band < 0.f) band += 1.f;
        bool streak = band < 0.14f;
        uint16_t lo = lerpColor(streak ? clearHi : clearLo, streak ? stormHi : stormLo, u);
        if (wy > MOUTH - 2.f) {
            float k = clampf((wy - (MOUTH - 2.f)) / 8.f, 0.f, 1.f);
            lo = lerpColor(lo, sea, k * (1.f - u * 0.45f));
        }
        if (flash) lo = lerpColor(lo, bolt, 0.5f);
        float rain = std::fmod(wy * 1.6f - t_ * 7.f, 1.f);
        if (rain < 0.f) rain += 1.f;
        if (u > 0.3f && rain < u * 0.18f) lo = lerpColor(lo, gs::rgb4(6, 7, 8), 0.35f);
        v.lineBackdrop[y] = lo;
        float fog = 0.f;
        if (wy < stormY_) fog = clampf((stormY_ - wy) / 7.f, 0.f, 1.f);
        float ys = float(y) / float(gs::SCREEN_H - 1);
        float hem = std::max(0.f, ys - 0.7f) / 0.3f;
        fog = std::max(fog, u * hem * 0.8f);
        if (flash) fog *= 0.25f;
        v.lineFog[y] = uint8_t(std::lround(clampf(fog, 0.f, 1.f) * 14.f));
        v.road[y].on = false;
    }
}

void Game::draw() {
    shx_ = shy_ = 0.f;
    if (shake_ > 0.f) {
        shx_ = std::sin(t_ * 90.f) * 3.2f * shake_;
        shy_ = std::cos(t_ * 73.f) * 2.2f * shake_;
        shake_ = std::max(0.f, shake_ - DT * 1.5f);
    }
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.HUD.enabled = true;
    backdrop();

    if (mode_ == Mode::Title) text("S3 SKIFF PASS", 160.f, 26.f, 0.72f, PAL_AMBER);
    else if (mode_ == Mode::Win) text("CLEAR", 160.f, 48.f, 1.15f, PAL_GREEN);
    else if (mode_ == Mode::Fail) text(stormFail_ ? "STORM" : "GROUNDED", 160.f, 48.f, 1.0f, PAL_RED);
    else if (mode_ == Mode::Pause) text("PAUSED", 160.f, 96.f, 1.f, PAL_HUD);

    drawBoat();
    drawWorld();
    drawStorm();

    char buf[64];
    if (mode_ == Mode::Play || mode_ == Mode::Pause) {
        hud(1, 0, "S3 SKIFF PASS", PAL_AMBER);
        float left = clockLeft();
        int cpal = left < 12.f ? PAL_RED : PAL_HUD;
        if (left >= 10.f) {
            int s = int(std::ceil(left - 1e-4f));
            std::snprintf(buf, sizeof buf, "STORM %d:%02d", s / 60, s % 60);
        } else {
            std::snprintf(buf, sizeof buf, "STORM %4.1f", left);
        }
        hud(40 - int(std::strlen(buf)), 0, buf, cpal);
        hud(1, 2, hint(), left < 12.f ? PAL_RED : PAL_HUD);
        std::snprintf(buf, sizeof buf, "SPD %3.1f", std::fabs(speed_));
        hud(1, 3, buf, PAL_HUD);
        int sea = std::max(0, int(std::lround(MOUTH - y_)));
        std::snprintf(buf, sizeof buf, "TO SEA %dM", sea);
        hud(40 - int(std::strlen(buf)), 3, buf, PAL_GREEN);

        hudC(26, "ARROWS DRIVE    DOWN BRAKES", PAL_HUD);
    } else if (mode_ == Mode::Title) {
        hudC(7, "THE SKIFF HAS ONE JOB", PAL_AMBER);
        hudC(9, "CLEAR THE PASS", PAL_HUD);
        hudC(11, "BEFORE THE STORM CLOCK", PAL_HUD);
        int s = int(std::lround(CLOCK_MAX));
        std::snprintf(buf, sizeof buf, "YOU HAVE %d:%02d", s / 60, s % 60);
        hudC(14, buf, PAL_RED);
        hudC(17, int(t_ * 2.f) % 2 == 0 ? "ENTER TO CAST OFF" : "ARROWS STEER AND DRIVE", PAL_HUD);
    } else if (mode_ == Mode::Win) {
        hudC(10, "CLEARED THE PASS", PAL_GREEN);
        std::snprintf(buf, sizeof buf, "CLOCK HAD %.1f S LEFT", clockLeft());
        hudC(11, buf, PAL_HUD);
        hudC(13, "ENTER SAILS IT AGAIN", PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        hudC(10, why_ && why_[0] ? why_ : "THE PASS CLOSED", PAL_RED);
        hudC(12, "ENTER TRIES THE PASS AGAIN", PAL_HUD);
    }
    hud(40 - int(std::strlen(S3_VERSION_STRING)), 27, S3_VERSION_STRING, PAL_HUD);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.apu.setMaster(0.75f);
    sys.apu.setEcho(0.18f, 0.22f, 0.12f);
    t_ = 0.f;
    if (bot_) begin();
    else showTitle();
    sys.setLight(20, 55, 85);
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

    followCamera();
    draw();
    audio();

    if (mode_ == Mode::Play) {
        float u = urgency();
        if (flashOn()) sys.setLight(180, 190, 210);
        else if (u > 0.75f) sys.setLight(18, 18, 36);
        else sys.setLight(20, 50, 80);
    }
}

}  // namespace skiffpass

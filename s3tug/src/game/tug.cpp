#include "game/tug.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

namespace tug {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;
constexpr float kNorth = 1.5707963f;

constexpr float kHalfLen = 16.f;
constexpr float kPort = 9.2f;
constexpr float kStbd = 5.6f;
constexpr float kQuayX = 184.f;
constexpr float kPileR = 3.6f;
constexpr float kBoxX0 = 152.f;
constexpr float kBoxX1 = 176.f;
constexpr float kBoxY0 = -21.f;
constexpr float kBoxY1 = 21.f;
constexpr float kAimX = 166.f;
constexpr float kAimY = 0.f;
constexpr float kLaneX = 84.f;
constexpr float kStartX = 84.f;
constexpr float kStartY = -100.f;
constexpr float kStartH = kNorth;
constexpr float kPlayZoom = 2.15f;
constexpr float kTitleZoom = 0.70f;
constexpr float kTitleCamX = 112.f;
constexpr float kTitleCamY = -28.f;
constexpr float kShipDraw = 45.4f;
constexpr float kHoldNeed = 0.50f;
constexpr float kWinSpeed = 0.62f;

constexpr int kPileN = 7;
const float kPile[kPileN][2] = {
    {130.f, -26.f}, {130.f, 26.f}, {178.f, -40.f}, {178.f, 40.f}, {38.f, -108.f}, {124.f, -62.f}, {46.f, 72.f},
};

float wrap(float a) {
    while (a > kPi) a -= kTau;
    while (a < -kPi) a += kTau;
    return a;
}

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    return gs::rgb4(int(ar + (br - ar) * t), int(ag + (bg - ag) * t), int(ab + (bb - ab) * t));
}

const char* compass(float h) {
    static const char* name[] = {"E", "NE", "N", "NW", "W", "SW", "S", "SE"};
    float u = h;
    while (u < 0.f) u += kTau;
    while (u >= kTau) u -= kTau;
    int i = int((u + kPi / 8.f) / (kPi / 4.f)) & 7;
    return name[i];
}

void bodyToWorld(float h, float surge, float sway, float& vx, float& vy) {
    float c = std::cos(h), s = std::sin(h);
    vx = c * surge + s * sway;
    vy = s * surge - c * sway;
}

void worldToBody(float h, float vx, float vy, float& surge, float& sway) {
    float c = std::cos(h), s = std::sin(h);
    surge = c * vx + s * vy;
    sway = s * vx - c * vy;
}

}  // namespace

float Game::speed() const { return std::hypot(surge_, sway_); }

void Game::begin() {
    x_ = kStartX;
    y_ = kStartY;
    heading_ = kStartH;
    surge_ = 0;
    sway_ = 0;
    yaw_ = 0;
    job_ = 0;
    hold_ = 0;
    shake_ = 0;
    wakeT_ = 0;
    piled_ = false;
    pile_ = -1;
    won_ = false;
    over_ = false;
    chime_ = 0;
    thrustIn_ = 0;
    tugIn_ = 0;
    wakes_.clear();
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(1, 4, 8));
    sys.apu.setMaster(0.75f);
    sys.apu.setEcho(0.12f, 0.18f, 0.10f);
    begin();
    if (bot_) {
        mode_ = Mode::Play;
        camX_ = x_;
        camY_ = y_;
        zoom_ = kPlayZoom;
    } else {
        mode_ = Mode::Title;
        camX_ = kTitleCamX;
        camY_ = kTitleCamY;
        zoom_ = kTitleZoom;
    }
}

void Game::controls(float& thrust, float& rudder, float& tug) {
    const gs::Pad& p = sys_->pad;
    thrust = 0;
    if (p.down(gs::BTN_UP)) thrust += 1.f;
    if (p.down(gs::BTN_DOWN)) thrust -= 1.f;
    if (p.accel > 0.15f) thrust = p.accel;
    if (p.brake > 0.15f) thrust = -p.brake;
    thrust = std::clamp(thrust, -1.f, 1.f);
    rudder = 0;
    if (p.down(gs::BTN_LEFT)) rudder += 1.f;
    if (p.down(gs::BTN_RIGHT)) rudder -= 1.f;
    if (std::fabs(p.axisX) > 0.22f) rudder = std::clamp(-p.axisX, -1.f, 1.f);
    tug = 0;
    if (p.down(gs::BTN_A) || p.down(gs::BTN_X)) tug -= 1.f;
    if (p.down(gs::BTN_B) || p.down(gs::BTN_Y)) tug += 1.f;
    tug = std::clamp(tug, -1.f, 1.f);
    if (p.pressed(gs::BTN_C) || p.pressed(gs::BTN_TURBO)) horn();
}

void Game::pilot(float& thrust, float& rudder, float& tug) {
    float hErr = wrap(kNorth - heading_);
    if (std::fabs(hErr) < 0.03f && std::fabs(yaw_) < 0.05f) rudder = 0;
    else rudder = std::clamp(hErr * 3.6f - yaw_ * 2.5f, -1.f, 1.f);

    bool lined = std::fabs(y_ - kAimY) < 1.8f && std::fabs(hErr) < 0.07f && std::fabs(yaw_) < 0.22f;
    float xGoal = kLaneX;
    float yGoal = kAimY;
    float maxV = 6.5f;
    if (!lined && x_ < 156.f) {
        xGoal = x_ < 102.f ? kLaneX : std::min(x_, 108.f);
        maxV = x_ > 112.f ? 1.5f : 6.5f;
    } else if (std::fabs(x_ - kAimX) < 2.4f && std::fabs(y_) < 1.8f && x_ > 158.f) {
        xGoal = kAimX;
        yGoal = kAimY;
        maxV = 0.7f;
    } else {
        xGoal = kAimX;
        yGoal = kAimY;
        maxV = x_ > 150.f ? 1.6f : 2.8f;
    }

    float c = std::cos(heading_), s = std::sin(heading_);
    float desVx = std::clamp((xGoal - x_) * 0.55f, -maxV, maxV);
    float desVy = std::clamp((yGoal - y_) * 0.55f, -maxV, maxV);
    if (std::fabs(x_ - kAimX) < 0.9f && std::fabs(y_) < 0.7f && x_ > 158.f && std::fabs(hErr) < 0.08f) {
        desVx = 0;
        desVy = 0;
    }
    float desSurge = c * desVx + s * desVy;
    float desSway = s * desVx - c * desVy;
    thrust = std::clamp((desSurge - surge_) * 1.4f, -1.f, 1.f);
    tug = std::clamp((desSway - sway_) * 1.45f, -1.f, 1.f);
}

void Game::corners(float xs[4], float ys[4]) const {
    float c = std::cos(heading_), s = std::sin(heading_);
    const float fl[4] = {kHalfLen, kHalfLen, -kHalfLen, -kHalfLen};
    const float fb[4] = {kStbd, -kPort, kStbd, -kPort};
    for (int i = 0; i < 4; i++) {
        xs[i] = x_ + c * fl[i] + s * fb[i];
        ys[i] = y_ + s * fl[i] - c * fb[i];
    }
}

bool Game::hitPile(int& which) const {
    float c = std::cos(heading_), s = std::sin(heading_);
    for (int i = 0; i < kPileN; i++) {
        float dx = kPile[i][0] - x_, dy = kPile[i][1] - y_;
        float localF = c * dx + s * dy;
        float localB = s * dx - c * dy;
        float cf = std::clamp(localF, -kHalfLen, kHalfLen);
        float cb = std::clamp(localB, -kPort, kStbd);
        if (std::hypot(localF - cf, localB - cb) < kPileR) {
            which = i;
            return true;
        }
    }
    which = -1;
    return false;
}

bool Game::boxed() const {
    float xs[4], ys[4];
    corners(xs, ys);
    for (int i = 0; i < 4; i++) {
        if (xs[i] < kBoxX0 || xs[i] > kBoxX1 || ys[i] < kBoxY0 || ys[i] > kBoxY1) return false;
    }
    return true;
}

bool Game::aligned() const {
    float north = std::fabs(wrap(heading_ - kNorth));
    float south = std::fabs(wrap(heading_ + kNorth));
    return north < 0.20f || south < 0.20f;
}

void Game::resolveWalls() {
    float xs[4], ys[4];
    corners(xs, ys);
    float maxX = xs[0];
    for (int i = 1; i < 4; i++) maxX = std::max(maxX, xs[i]);
    if (maxX > kQuayX) x_ -= maxX - kQuayX;
    if (x_ < 16.f) x_ = 16.f;
    if (y_ < -168.f) y_ = -168.f;
    if (y_ > 118.f) y_ = 118.f;

    float vx, vy;
    bodyToWorld(heading_, surge_, sway_, vx, vy);
    bool clip = false;
    if (maxX > kQuayX && vx > 0.f) {
        vx = 0;
        clip = true;
    }
    if (x_ <= 16.f && vx < 0.f) {
        vx = 0;
        clip = true;
    }
    if (y_ <= -168.f && vy < 0.f) {
        vy = 0;
        clip = true;
    }
    if (y_ >= 118.f && vy > 0.f) {
        vy = 0;
        clip = true;
    }
    if (clip) {
        worldToBody(heading_, vx, vy, surge_, sway_);
        yaw_ *= 0.55f;
    }
}

void Game::physics(float dt, float thrust, float rudder, float tug) {
    thrustIn_ = thrust;
    tugIn_ = tug;
    surge_ += thrust * 10.f * dt;
    sway_ += tug * 8.5f * dt;
    float auth = 1.05f + 0.75f * std::min(std::fabs(surge_), 8.f) / 8.f;
    yaw_ += rudder * auth * dt;
    yaw_ += tug * 0.04f * dt;
    surge_ *= std::exp(-1.05f * dt);
    sway_ *= std::exp(-1.45f * dt);
    yaw_ *= std::exp(-3.1f * dt);
    surge_ = std::clamp(surge_, -9.f, 9.f);
    sway_ = std::clamp(sway_, -7.f, 7.f);
    yaw_ = std::clamp(yaw_, -1.6f, 1.6f);
    heading_ = wrap(heading_ + yaw_ * dt);

    float vx, vy;
    bodyToWorld(heading_, surge_, sway_, vx, vy);
    x_ += vx * dt;
    y_ += vy * dt;
    resolveWalls();

    wakeT_ -= dt;
    if (wakeT_ <= 0.f && speed() > 1.4f) {
        wakeT_ = 0.06f;
        float c = std::cos(heading_), s = std::sin(heading_);
        Wake w;
        w.x = x_ - c * (kHalfLen * 0.85f);
        w.y = y_ - s * (kHalfLen * 0.85f);
        w.life = 1.f;
        wakes_.push_back(w);
        if (wakes_.size() > 18) wakes_.erase(wakes_.begin());
    }
    for (Wake& w : wakes_) w.life -= dt;
    wakes_.erase(std::remove_if(wakes_.begin(), wakes_.end(), [](const Wake& w) { return w.life <= 0.f; }), wakes_.end());

    int which = -1;
    if (hitPile(which)) {
        piled_ = true;
        pile_ = which;
        won_ = false;
        over_ = true;
        mode_ = Mode::Fail;
        surge_ = 0;
        sway_ = 0;
        yaw_ = 0;
        shake_ = 1.f;
        hold_ = 0;
        sys_->apu.noiseBurst(0.45f, 180.f, 0.18f);
        sys_->rumble(0.7f, 0.4f, 220);
        sys_->setLight(220, 30, 20);
        blip(140.f);
        return;
    }

    bool slow = speed() < kWinSpeed && std::fabs(yaw_) < 0.12f;
    if (boxed() && aligned() && slow) hold_ += dt;
    else hold_ = 0;
    if (hold_ >= kHoldNeed) {
        mode_ = Mode::Win;
        won_ = true;
        over_ = true;
        surge_ = 0;
        sway_ = 0;
        yaw_ = 0;
        chime();
        sys_->setLight(40, 180, 60);
    }
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.05f);
    tone1_ = 0.09f;
}

void Game::horn() {
    sys_->apu.noiseBurst(0.22f, 140.f, 0.28f);
    sys_->apu.tone(0, 196.f, 0.06f);
    tone0_ = 0.28f;
}

void Game::chime() {
    chime_ = 4;
    chimeStep_ = 0;
    chimeT_ = 0;
}

void Game::audio(float dt, float thrust, float tug) {
    float drive = mode_ == Mode::Play ? 0.018f + 0.03f * std::fabs(thrust) + 0.02f * std::fabs(tug) : 0.012f;
    sys_->apu.noise(drive, 520.f + speed() * 40.f, false);
    float eng = mode_ == Mode::Play ? 0.012f + 0.02f * std::fabs(thrust) : 0.f;
    sys_->apu.tone(2, 62.f + std::fabs(surge_) * 4.f, eng);
    if (tone0_ > 0.f) {
        tone0_ -= dt;
        if (tone0_ <= 0.f) sys_->apu.tone(0, 0, 0);
    }
    if (tone1_ > 0.f) {
        tone1_ -= dt;
        if (tone1_ <= 0.f) sys_->apu.tone(1, 0, 0);
    }
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - dt);
    if (chime_ > 0) {
        chimeT_ -= dt;
        if (chimeT_ <= 0.f) {
            static const float notes[] = {392.f, 523.25f, 659.25f, 784.f};
            int n = std::min(chimeStep_, 3);
            sys_->apu.tone(0, notes[n], 0.05f);
            tone0_ = 0.14f;
            chimeT_ = 0.13f;
            if (++chimeStep_ >= chime_) chime_ = 0;
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START)) {
            begin();
            mode_ = Mode::Play;
            camX_ = x_;
            camY_ = y_;
            blip(660.f);
        } else if (pad.pressed(gs::BTN_MODE)) {
            sys.quit();
        }
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(330.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            begin();
            mode_ = Mode::Title;
        } else {
            job_ += kDt;
            float thrust = 0, rudder = 0, tug = 0;
            if (bot_) pilot(thrust, rudder, tug);
            else controls(thrust, rudder, tug);
            physics(kDt, thrust, rudder, tug);
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
        else if (pad.pressed(gs::BTN_MODE)) {
            begin();
            mode_ = Mode::Title;
        }
    } else if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            begin();
            mode_ = Mode::Play;
            camX_ = x_;
            camY_ = y_;
            blip(660.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            begin();
            mode_ = Mode::Title;
        }
    }

    if (mode_ == Mode::Title) {
        camX_ = kTitleCamX + std::sin(t_ * 0.15f) * 6.f;
        camY_ = kTitleCamY;
        zoom_ = kTitleZoom;
    } else {
        float dx = kAimX - x_, dy = kAimY - y_;
        float dist = std::hypot(dx, dy);
        float lead = mode_ == Mode::Play ? 16.f : 0.f;
        if (dist > 1.f) {
            dx *= lead / dist;
            dy *= lead / dist;
        } else {
            dx = dy = 0;
        }
        float gx = x_ + dx;
        float gy = y_ + dy;
        float k = 1.f - std::exp(-kDt * 4.f);
        camX_ += (gx - camX_) * k;
        camY_ += (gy - camY_) * k;
        zoom_ += (kPlayZoom - zoom_) * k;
        if (shake_ > 0.f) {
            camX_ += std::sin(t_ * 47.f) * shake_ * 3.5f;
            camY_ += std::cos(t_ * 39.f) * shake_ * 2.5f;
        }
    }
    float thrust = thrustIn_, tug = tugIn_;
    if (mode_ != Mode::Play) {
        thrust = 0;
        tug = 0;
    }
    audio(kDt, thrust, tug);
    draw();
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

void Game::drawHud() {
    char buf[64];
    if (mode_ == Mode::Title) {
        hudC(21, "BERTH THE SHIP IN THE GREEN BOX", PAL_BANNER);
        hudC(22, "A PILING FAILS THE JOB", PAL_ALERT);
        hudC(23, "ARROWS  ENGINE AND RUDDER", PAL_HUD);
        hudC(24, "Z PORT TUG    X STARBOARD TUG", PAL_HUD);
        hudC(25, "C HORN", PAL_HUD);
        if ((int(t_ * 2.f) & 1) == 0) hudC(27, "START", PAL_WIN);
        return;
    }
    hud(1, 0, "S3 TUG", PAL_BANNER);
    int sec = int(job_);
    std::snprintf(buf, sizeof buf, "%d:%02d", sec / 60, sec % 60);
    hud(34, 0, buf, PAL_HUD);
    if (mode_ == Mode::Pause) {
        hudC(13, "PAUSED", PAL_BANNER);
        hudC(15, "START CONTINUES", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Win) {
        int s = int(job_);
        std::snprintf(buf, sizeof buf, "TIME %d:%02d", s / 60, s % 60);
        hudC(16, buf, PAL_HUD);
        hudC(17, "PILINGS CLEAR", PAL_WIN);
        if (!bot_) hudC(19, "START BERTHS ANOTHER", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Fail) {
        hudC(16, "THE JOB IS LOST", PAL_ALERT);
        if (!bot_) hudC(18, "START TRIES AGAIN", PAL_HUD);
        return;
    }
    std::snprintf(buf, sizeof buf, "SPD %04.1f  %s", speed(), compass(heading_));
    hud(1, 1, buf, PAL_HUD);
    if (boxed() && speed() < 1.4f) hud(1, 26, "HOLD HER IN THE BOX", PAL_WIN);
    else if (x_ < 120.f) hud(1, 26, "NORTH ALONG THE LANE", PAL_HUD);
    else hud(1, 26, "WALK HER INTO THE SLIP", PAL_BANNER);
    hud(1, 27, "PILINGS ARE A FAIL", PAL_ALERT);
}

int Game::shipFrame() const {
    float u = heading_;
    while (u < 0.f) u += kTau;
    while (u >= kTau) u -= kTau;
    int i = int(std::lround(u / kTau * 16.f)) % 16;
    if (i < 0) i += 16;
    return i;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    if (cx + w < -8 || cy + h < -8 || cx - w > gs::SCREEN_W + 8 || cy - h > gs::SCREEN_H + 8) return;
    gs::Sprite s;
    long sw = std::clamp(std::lround(w), 1L, 1800L);
    long sh = std::clamp(std::lround(h), 1L, 1800L);
    s.w = int16_t(sw);
    s.h = int16_t(sh);
    s.x = int16_t(std::clamp(std::lround(cx - sw * 0.5f), -2000L, 2000L));
    s.y = int16_t(std::clamp(std::lround(cy - sh * 0.5f), -2000L, 2000L));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::place(const gs::Mipped& m, float wx, float wy, float worldH, int pal) {
    float sx, sy;
    worldToScreen(wx, wy, sx, sy);
    spr(m, sx, sy, worldH * zoom_, pal, false);
}

void Game::worldToScreen(float wx, float wy, float& sx, float& sy) const {
    sx = 160.f + (wx - camX_) * zoom_;
    sy = 112.f - (wy - camY_) * zoom_;
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    const uint16_t deep = gs::rgb4(1, 4, 8);
    const uint16_t mid = gs::rgb4(2, 8, 11);
    const uint16_t shallow = gs::rgb4(4, 12, 13);
    const uint16_t quay = gs::rgb4(7, 7, 8);
    const uint16_t land = gs::rgb4(5, 6, 5);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (112.f - y) / std::max(zoom_, 0.05f);
        float shore = std::clamp((wy + 140.f) / 220.f, 0.f, 1.f);
        uint16_t water = lerpC(deep, mid, shore);
        float shimmer = 0.5f + 0.5f * std::sin(y * 0.07f + t_ * 1.3f);
        water = lerpC(water, shallow, shimmer * 0.18f);
        float camRight = camX_ + (160.f) / std::max(zoom_, 0.05f);
        uint16_t c = water;
        if (camRight > kQuayX + 30.f && camX_ > kQuayX) c = land;
        else if (camX_ > kQuayX - 8.f) c = lerpC(water, quay, std::clamp((camX_ - (kQuayX - 40.f)) / 80.f, 0.f, 1.f));
        v.lineBackdrop[y] = c;
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }

    auto banner = [&](const gs::Mipped& m, float x, float y, int pal) { spr(m, x, y, float(m.h), pal, false); };
    if (mode_ == Mode::Title) banner(art_.title, 160, 28, PAL_BANNER);
    else if (mode_ == Mode::Win) banner(art_.berthed, 160, 78, PAL_WIN);
    else if (mode_ == Mode::Fail) banner(art_.piling, 160, 78, PAL_ALERT);

    if (mode_ == Mode::Play || mode_ == Mode::Pause) {
        float sx, sy;
        worldToScreen(kAimX, kAimY, sx, sy);
        if (sx < 18 || sx > 302 || sy < 18 || sy > 206) {
            float dx = sx - 160.f, dy = sy - 112.f;
            float k = 1.f;
            if (std::fabs(dx) > 1.f) k = std::min(k, 146.f / std::fabs(dx));
            if (std::fabs(dy) > 1.f) k = std::min(k, 94.f / std::fabs(dy));
            spr(art_.dot, 160.f + dx * k, 112.f + dy * k, 11.f, PAL_WIN, false);
        }
        spr(art_.panel, 286, 78, 80, PAL_MAP, false);
        auto plot = [&](float wx, float wy, float h, int pal) {
            float px = 252.f + (wx - 70.f) * 0.30f;
            float py = 78.f - (wy + 20.f) * 0.28f;
            spr(art_.dot, px, py, h, pal, false);
        };
        plot(kBoxX0, kBoxY0, 3.f, PAL_BOX);
        plot(kBoxX1, kBoxY0, 3.f, PAL_BOX);
        plot(kBoxX1, kBoxY1, 3.f, PAL_BOX);
        plot(kBoxX0, kBoxY1, 3.f, PAL_BOX);
        for (int i = 0; i < kPileN; i++) plot(kPile[i][0], kPile[i][1], 3.5f, PAL_PILE);
        plot(x_, y_, 5.f, PAL_BANNER);
    }

    for (int i = 0; i < kPileN; i++) place(art_.pile, kPile[i][0], kPile[i][1], 9.f, PAL_PILE);

    float bsx, bsy;
    worldToScreen(x_, y_, bsx, bsy);
    bsy += std::sin(t_ * 2.1f) * 0.8f;
    float shipH = kShipDraw * zoom_;
    if (mode_ == Mode::Title) shipH = std::max(shipH, 28.f);
    const gs::Mipped& hull = art_.ship[shipFrame()];
    spr(hull, bsx, bsy, shipH, PAL_SHIP, false);
    spr(hull, bsx + 3.f, bsy + 3.f, shipH, PAL_SHIP, true);

    if (speed() > 1.2f && mode_ == Mode::Play) {
        float c = std::cos(heading_), s = std::sin(heading_);
        place(art_.foam, x_ + c * (kHalfLen + 1.5f), y_ + s * (kHalfLen + 1.5f), 3.2f + speed() * 0.15f, PAL_FOAM);
    }
    for (const Wake& w : wakes_) {
        float sx, sy;
        worldToScreen(w.x, w.y, sx, sy);
        spr(art_.foam, sx, sy, 2.5f + (1.f - w.life) * 4.f, PAL_FOAM, false);
    }

    auto dashes = [&](float x0, float y0, float x1, float y1, float step, int pal, float h) {
        float dx = x1 - x0, dy = y1 - y0;
        float len = std::hypot(dx, dy);
        int n = std::max(1, int(len / step));
        for (int i = 0; i <= n; i++) {
            float u = float(i) / float(n);
            place(art_.dash, x0 + dx * u, y0 + dy * u, h, pal);
        }
    };
    dashes(kBoxX0, kBoxY0, kBoxX1, kBoxY0, 4.f, PAL_BOX, 2.4f);
    dashes(kBoxX1, kBoxY0, kBoxX1, kBoxY1, 4.f, PAL_BOX, 2.4f);
    dashes(kBoxX1, kBoxY1, kBoxX0, kBoxY1, 4.f, PAL_BOX, 2.4f);
    dashes(kBoxX0, kBoxY1, kBoxX0, kBoxY0, 4.f, PAL_BOX, 2.4f);
    dashes(kLaneX, -112.f, kLaneX, -4.f, 10.f, PAL_LANE, 1.8f);
    dashes(kLaneX, 0.f, kAimX - 6.f, 0.f, 10.f, PAL_LANE, 1.8f);

    place(art_.crane, kQuayX + 14.f, 8.f, 28.f, PAL_CRANE);
    for (int i = -3; i <= 3; i++) {
        float yy = i * 28.f;
        place(art_.dock, kQuayX + 12.f, yy, 22.f, PAL_DOCK);
    }

    for (int i = 0; i < 3; i++) {
        float u = t_ * (0.28f + i * 0.04f) + i * 2.1f;
        float gx = 20.f + std::sin(u) * 36.f + i * 18.f;
        float gy = 36.f + std::cos(u * 0.7f) * 24.f;
        int fr = (int(t_ * 4.f + i * 3.f) & 1);
        place(art_.gull[fr], gx, gy, 6.f, PAL_GULL);
    }

    drawHud();
}

}  // namespace tug

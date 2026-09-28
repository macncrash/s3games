#include "tram.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace tramgrass {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;

constexpr float kStartX = 0.f;
constexpr float kStartY = 14.f;
constexpr float kStartH = kPi * 0.5f;

constexpr float kGauge = 4.6f;
constexpr float kSouth = 2.f;
constexpr float kGrassX0 = -34.f;
constexpr float kGrassX1 = 34.f;
constexpr float kGrassY0 = 96.f;
constexpr float kGrassY1 = 188.f;
constexpr float kAimX = 0.f;
constexpr float kAimY = 136.f;

constexpr float kHalfL = 4.6f;
constexpr float kHalfW = 1.15f;

constexpr float kAccel = 7.5f;
constexpr float kBrake = 12.5f;
constexpr float kStop = 0.18f;
constexpr float kHoldNeed = 0.75f;
constexpr float kShortNeed = 2.2f;
constexpr float kNoseNeed = 2.6f;
constexpr float kTimeLimit = 52.f;

constexpr float kPlayZoom = 6.4f;
constexpr float kTitleZoom = 1.35f;
constexpr float kTitleCamX = 0.f;
constexpr float kTitleCamY = 92.f;

const float kTuft[][2] = {
    {-26.f, 108.f}, {-14.f, 122.f}, {16.f, 114.f}, {24.f, 132.f}, {-20.f, 148.f},
    {10.f, 156.f},  {-8.f, 168.f},  {20.f, 104.f}, {-28.f, 160.f}, {6.f, 140.f},
};
const float kPole[] = {18.f, 36.f, 54.f, 72.f};

float wrap(float a) {
    while (a > kPi) a -= kTau;
    while (a < -kPi) a += kTau;
    return a;
}

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = clampf(t, 0.f, 1.f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    return gs::rgb4(int(ar + (br - ar) * t), int(ag + (bg - ag) * t), int(ab + (bb - ab) * t));
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (hold_ > 0.08f) return 3;
    if (full_ || cover_ > 0.45f) return 2;
    return 1;
}

int Game::hullFrame() const {
    float u = heading_;
    if (u < 0.f) u += kTau;
    int i = int(std::lround(u / kTau * 8.f)) % 8;
    if (i < 0) i += 8;
    return i;
}

Game::Ext Game::extents() const {
    const float c = std::cos(heading_), s = std::sin(heading_);
    Ext e{x_, x_, y_, y_};
    const float fs[2] = {-kHalfL, kHalfL};
    const float ws[2] = {-kHalfW, kHalfW};
    for (float f : fs) {
        for (float w : ws) {
            float px = x_ + f * c - w * s;
            float py = y_ + f * s + w * c;
            e.minX = std::min(e.minX, px);
            e.maxX = std::max(e.maxX, px);
            e.minY = std::min(e.minY, py);
            e.maxY = std::max(e.maxY, py);
        }
    }
    return e;
}

bool Game::hullOnGrass(const Ext& e) const {
    return e.minX >= kGrassX0 && e.maxX <= kGrassX1 && e.minY >= kGrassY0 && e.maxY <= kGrassY1;
}

float Game::grassCover(const Ext& e) const {
    if (e.maxX <= kGrassX0 || e.minX >= kGrassX1 || e.maxY <= kGrassY0 || e.minY >= kGrassY1) return 0.f;
    float spanY = std::max(0.4f, e.maxY - e.minY);
    float spanX = std::max(0.4f, e.maxX - e.minX);
    float oy = (std::min(e.maxY, kGrassY1) - std::max(e.minY, kGrassY0)) / spanY;
    float ox = (std::min(e.maxX, kGrassX1) - std::max(e.minX, kGrassX0)) / spanX;
    return clampf(ox, 0.f, 1.f) * clampf(oy, 0.f, 1.f);
}

const char* Game::hint() const {
    if (hold_ > 0.02f) return "HOLD THE FULL STOP";
    if (full_) return "BRAKE AND HOLD";
    if (cover_ > 0.15f) return "EVERY WHEEL ON THE GRASS";
    if (y_ > kGrassY0 - 28.f) return "THE MEADOW IS AHEAD";
    return "LAND ON THE GRASS";
}

void Game::begin() {
    x_ = kStartX;
    y_ = kStartY;
    heading_ = kStartH;
    speed_ = 0.f;
    race_ = 0.f;
    hold_ = shortT_ = noseT_ = cover_ = 0.f;
    sparkT_ = 0.f;
    sparkCursor_ = 0;
    full_ = deep_ = launched_ = false;
    won_ = over_ = false;
    phase_ = 0;
    chimeN_ = chimeStep_ = 0;
    why_[0] = 0;
    for (Spark& p : sparks_) p = {};
}

void Game::showTitle() {
    begin();
    mode_ = Mode::Title;
    zoom_ = kTitleZoom;
    camX_ = kTitleCamX;
    camY_ = kTitleCamY;
}

void Game::startRun() {
    begin();
    mode_ = Mode::Run;
    zoom_ = kPlayZoom;
    camX_ = x_;
    camY_ = y_;
    bell(0.28f);
    blip(520.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.82f);
    sys.apu.setEcho(0.08f, 0.14f, 0.06f);
    t_ = 0.f;
    if (bot_) startRun();
    else showTitle();
}

void Game::controls(float& steer, float& gas, float& brake) {
    const gs::Pad& p = sys_->pad;
    steer = 0.f;
    gas = 0.f;
    brake = 0.f;
    if (p.down(gs::BTN_LEFT)) steer += 1.f;
    if (p.down(gs::BTN_RIGHT)) steer -= 1.f;
    if (std::fabs(p.axisX) > 0.18f) steer = clampf(-p.axisX, -1.f, 1.f);
    if (p.down(gs::BTN_UP) || p.down(gs::BTN_C) || p.down(gs::BTN_A) || p.axisY > 0.28f || p.accel > 0.12f) gas = 1.f;
    if (p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.down(gs::BTN_X) || p.axisY < -0.28f || p.brake > 0.12f) brake = 1.f;
    if (p.accel > 0.05f) gas = std::max(gas, p.accel);
    if (p.brake > 0.05f) brake = std::max(brake, p.brake);
    if (p.down(gs::BTN_TURBO)) bell(0.16f);
}

void Game::pilot(float& steer, float& gas, float& brake) {
    const float north = kPi * 0.5f;
    float hdes = north;
    if (y_ < kGrassY0 - 8.f) hdes = std::atan2(kAimY - y_, kAimX - x_);
    else hdes = north + clampf((x_ - kAimX) * -0.06f, -0.18f, 0.18f);
    float err = wrap(hdes - heading_);
    steer = clampf(err / 0.35f, -1.f, 1.f);
    if (x_ > 1.6f) steer = std::max(steer, 0.25f);
    if (x_ < -1.6f) steer = std::min(steer, -0.25f);

    gas = 0.f;
    brake = 0.f;
    if (y_ < kAimY - 10.f) {
        gas = speed_ < 5.2f ? 1.f : 0.15f;
        if (std::fabs(err) > 0.55f) gas = 0.35f;
        phase_ = y_ > kGrassY0 - 12.f ? 1 : 0;
    } else {
        phase_ = 3;
        brake = 1.f;
    }
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.05f);
    tone1_ = 0.08f;
}

void Game::bell(float seconds) {
    if (bellT_ < seconds) bellT_ = seconds;
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
    speed_ = 0.f;
    std::snprintf(why_, sizeof why_, "full stop");
    chime(5);
    sys_->rumble(0.22f, 0.1f, 140);
    sys_->setLight(40, 180, 70);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    std::snprintf(why_, sizeof why_, "%s", why);
    shake_ = 1.f;
    sys_->rumble(0.45f, 0.2f, 160);
    sys_->setLight(180, 36, 24);
    sys_->apu.noiseBurst(0.4f, 90.f, 0.32f);
    sys_->apu.tone(0, 64.f, 0.06f);
    tone0_ = 0.4f;
}

void Game::physics(float steer, float gas, float brake) {
    race_ += kDt;
    float rate = 0.85f + std::min(std::fabs(speed_), 6.f) * 0.05f;
    if (std::fabs(speed_) < 0.35f) rate *= 0.4f;
    heading_ = wrap(heading_ + steer * rate * kDt * (speed_ < 0.f ? -1.f : 1.f));

    if (brake > 0.05f) {
        float drop = brake * kBrake * kDt;
        if (speed_ > 0.f) speed_ = std::max(0.f, speed_ - drop);
        else if (speed_ < 0.f) speed_ = std::min(0.f, speed_ + drop);
    } else {
        speed_ += gas * kAccel * kDt;
        if (gas < 0.05f) speed_ -= speed_ * 0.35f * kDt;
    }

    Ext e = extents();
    cover_ = grassCover(e);
    float drag = 0.18f + cover_ * 1.35f;
    speed_ *= std::exp(-drag * kDt);
    speed_ = clampf(speed_, -1.8f, 6.6f);

    float hc = std::cos(heading_), hs = std::sin(heading_);
    x_ += hc * speed_ * kDt;
    y_ += hs * speed_ * kDt;

    auto thud = [&]() {
        if (thumpT_ > 0.f) return;
        sys_->apu.noiseBurst(0.18f, 140.f, 0.08f);
        thumpT_ = 0.2f;
        shake_ = std::max(shake_, 0.35f);
    };

    e = extents();
    if (e.minY < kSouth) {
        y_ += kSouth - e.minY + 0.04f;
        if (speed_ < 0.f) speed_ *= 0.2f;
        thud();
    }
    e = extents();
    if (e.maxY < kGrassY0 + 1.5f) {
        if (e.minX < -kGauge) {
            x_ += -kGauge - e.minX + 0.04f;
            speed_ *= 0.9f;
            thud();
        }
        e = extents();
        if (e.maxX > kGauge) {
            x_ += kGauge - e.maxX - 0.04f;
            speed_ *= 0.9f;
            thud();
        }
    }

    e = extents();
    cover_ = grassCover(e);
    full_ = hullOnGrass(e);
    deep_ = full_ && e.minY > kGrassY0 + 4.5f && e.maxY < kGrassY1 - 4.5f;
    if (std::fabs(speed_) > 1.1f || y_ > 40.f) launched_ = true;

    if (e.maxY > kGrassY1 + 0.2f) {
        fail("rolled past the meadow");
        return;
    }
    if (e.minY > kGrassY0 && (e.minX < kGrassX0 || e.maxX > kGrassX1)) {
        fail("left the meadow");
        return;
    }
    if (race_ > kTimeLimit) {
        fail("the clock ran out");
        return;
    }

    float sp = std::fabs(speed_);
    if (deep_ && sp <= kStop) {
        shortT_ = 0.f;
        noseT_ = 0.f;
        hold_ += kDt;
        phase_ = 3;
        if (hold_ >= kHoldNeed) {
            win();
            return;
        }
    } else if (!launched_) {
        hold_ = 0.f;
        shortT_ = 0.f;
    } else if (sp <= kStop && cover_ < 0.04f) {
        hold_ = 0.f;
        shortT_ += kDt;
        if (shortT_ >= kShortNeed) {
            fail("stopped on the rails");
            return;
        }
    } else if (sp <= kStop && !full_) {
        hold_ = 0.f;
        noseT_ += kDt;
        if (noseT_ >= kNoseNeed) {
            fail("wheels still off the grass");
            return;
        }
    } else {
        if (!deep_ || sp > kStop) hold_ = 0.f;
        shortT_ = 0.f;
        noseT_ = 0.f;
    }

    sparkT_ -= kDt;
    if (sparkT_ <= 0.f && (gas > 0.2f || sp > 1.0f)) {
        sparkT_ = 0.1f;
        Spark& p = sparks_[sparkCursor_];
        p.x = x_ - hc * (kHalfL - 0.4f);
        p.y = y_ - hs * (kHalfL - 0.4f);
        p.life = 1.f;
        sparkCursor_ = (sparkCursor_ + 1) % 8;
    }
    for (Spark& p : sparks_)
        if (p.life > 0.f) p.life -= kDt * 0.85f;
}

void Game::audio() {
    float bed = mode_ == Mode::Run ? 0.01f + std::fabs(speed_) * 0.0012f : 0.006f;
    sys_->apu.noise(bed, cover_ > 0.4f ? 160.f : 380.f, false);
    if (bellT_ > 0.f) {
        bellT_ -= kDt;
        float v = bellT_ > 0.04f ? 0.055f : std::max(0.f, bellT_) * 1.2f;
        float ding = std::fmod(t_ * 8.f, 1.f) < 0.5f ? 740.f : 620.f;
        sys_->apu.tone(0, ding, v);
    } else if (tone0_ <= 0.f && chimeN_ == 0) {
        sys_->apu.tone(0, 0.f, 0.f);
    }
    if (mode_ == Mode::Run) {
        float wob = 0.8f + 0.2f * std::sin(t_ * (9.f + std::fabs(speed_)));
        float vol = (0.012f + std::min(std::fabs(speed_), 6.f) * 0.005f) * wob;
        sys_->apu.tone(2, 42.f + std::fabs(speed_) * 8.f, vol);
    } else if (bellT_ <= 0.f) {
        sys_->apu.tone(2, 0.f, 0.f);
    }
    if (tone0_ > 0.f) {
        tone0_ -= kDt;
        if (tone0_ <= 0.f && bellT_ <= 0.f && chimeN_ == 0) sys_->apu.tone(0, 0.f, 0.f);
    }
    if (tone1_ > 0.f) {
        tone1_ -= kDt;
        if (tone1_ <= 0.f && bellT_ <= 0.f) sys_->apu.tone(1, 0.f, 0.f);
    }
    if (thumpT_ > 0.f) thumpT_ -= kDt;
    if (chimeN_ > 0) {
        chimeT_ -= kDt;
        if (chimeT_ <= 0.f) {
            static const float notes[] = {392.f, 494.f, 587.f, 784.f, 988.f};
            sys_->apu.tone(0, notes[std::min(chimeStep_, 4)], 0.05f);
            tone0_ = 0.12f;
            chimeT_ = 0.13f;
            if (++chimeStep_ >= chimeN_) chimeN_ = 0;
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A)) startRun();
        else if (pad.pressed(gs::BTN_MODE)) sys.quit();
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(300.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        } else {
            float steer = 0.f, gas = 0.f, brake = 0.f;
            if (bot_) pilot(steer, gas, brake);
            else controls(steer, gas, brake);
            physics(steer, gas, brake);
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Run;
        else if (pad.pressed(gs::BTN_MODE)) showTitle();
    } else if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C))) {
        startRun();
    } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
        showTitle();
    }
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 1.8f);
    camera();
    audio();
    if (mode_ == Mode::Win) sys.setLight(40, 180, 70);
    else if (mode_ == Mode::Fail) sys.setLight(180, 36, 24);
    else if (hold_ > 0.02f || deep_) sys.setLight(40, 160, 50);
    else if (cover_ > 0.2f) sys.setLight(70, 140, 40);
    else sys.setLight(50, 60, 80);
    draw();
}

void Game::camera() {
    if (mode_ == Mode::Title) {
        camX_ = kTitleCamX;
        camY_ = kTitleCamY;
        zoom_ = kTitleZoom;
        return;
    }
    float lead = mode_ == Mode::Run ? 7.f : 0.f;
    float gx = x_ + std::cos(heading_) * lead;
    float gy = y_ + std::sin(heading_) * lead;
    float k = 1.f - std::exp(-kDt * 4.2f);
    camX_ += (gx - camX_) * k;
    camY_ += (gy - camY_) * k;
    zoom_ += (kPlayZoom - zoom_) * k;
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

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow) {
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
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::place(const gs::Mipped& m, float wx, float wy, float worldH, int pal) {
    float sx = 160.f + (wx - camX_) * zoom_;
    float sy = 112.f - (wy - camY_) * zoom_;
    spr(m, sx, sy, worldH * zoom_, pal, false);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.hudEnabled = true;

    float jx = 0.f, jy = 0.f;
    if (shake_ > 0.f) {
        jx = std::sin(t_ * 48.f) * shake_ * 2.5f;
        jy = std::cos(t_ * 39.f) * shake_ * 1.8f;
    }
    camX_ -= jx / std::max(zoom_, 0.4f);
    camY_ += jy / std::max(zoom_, 0.4f);

    const float grassCx = (kGrassX0 + kGrassX1) * 0.5f;
    const float grassHw = (kGrassX1 - kGrassX0) * 0.5f;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (112.f - float(y)) / zoom_;
        uint16_t lot = lerpC(gs::rgb4(2, 5, 2), gs::rgb4(1, 3, 2), clampf(wy / 200.f, 0.f, 1.f));
        v.lineBackdrop[y] = lot;
        v.lineFog[y] = 0;
        gs::RoadLine& r = v.road[y];
        r.on = true;
        r.v = wy * 18.f;
        r.band = (int(std::floor(wy * 0.28f)) & 1) ? 1 : 0;
        r.left = gs::GROUND_LAND;
        r.right = gs::GROUND_LAND;
        if (wy >= kGrassY0 && wy <= kGrassY1) {
            r.cx = 160.f + (grassCx - camX_) * zoom_;
            r.hw = std::max(8.f, grassHw * zoom_);
            r.pal = PAL_FIELD;
            r.style = 0;
        } else if (wy > kSouth && wy < kGrassY0) {
            r.cx = 160.f + (0.f - camX_) * zoom_;
            r.hw = std::max(6.f, kGauge * zoom_);
            r.pal = PAL_RAIL;
            r.style = 1;
        } else {
            r.on = false;
        }
    }

    auto banner = [&](const gs::Mipped& m, float yb, int pal) { spr(m, 160.f, yb, float(m.h), pal, false); };
    if (mode_ == Mode::Title) banner(art_.title, 18.f, PAL_BANNER);
    else if (mode_ == Mode::Pause) banner(art_.paused, 96.f, PAL_BANNER);
    else if (mode_ == Mode::Win) {
        banner(art_.fullStop, 28.f, PAL_WIN);
        banner(art_.onGrass, 52.f, PAL_WIN);
    } else if (mode_ == Mode::Fail) {
        if (!std::strcmp(why_, "rolled past the meadow")) banner(art_.past, 28.f, PAL_ALERT);
        else if (!std::strcmp(why_, "left the meadow")) banner(art_.leftMeadow, 28.f, PAL_ALERT);
        else if (!std::strcmp(why_, "stopped on the rails")) banner(art_.onRails, 28.f, PAL_ALERT);
        else if (!std::strcmp(why_, "wheels still off the grass")) banner(art_.wheels, 28.f, PAL_ALERT);
        else banner(art_.clocked, 28.f, PAL_ALERT);
        banner(art_.failed, 54.f, PAL_ALERT);
    }

    for (const float* p : kTuft) place(art_.tuft, p[0], p[1], 2.4f, PAL_WIRE);
    for (float y : kPole) {
        place(art_.pole, -kGauge - 0.4f, y, 6.4f, PAL_POLE);
        place(art_.pole, kGauge + 0.4f, y, 6.4f, PAL_POLE);
    }
    place(art_.board, kGrassX0 + 3.f, kGrassY0 + 2.f, 3.6f, PAL_SIGN);
    place(art_.board, kGrassX1 - 3.f, kGrassY0 + 2.f, 3.6f, PAL_SIGN);
    place(art_.board, kAimX, kAimY, 2.2f, PAL_SIGN);

    for (const Spark& p : sparks_) {
        if (p.life <= 0.f) continue;
        float sx = 160.f + (p.x - camX_) * zoom_;
        float sy = 112.f - (p.y - camY_) * zoom_;
        spr(art_.spark, sx, sy, 5.f + (1.f - p.life) * 7.f, PAL_SPARK, false);
    }

    int fi = hullFrame();
    float bh = (kHalfL * 2.f + 0.8f) * zoom_;
    if (mode_ == Mode::Title) bh = std::max(bh, 28.f);
    float bsx = 160.f + (x_ - camX_) * zoom_;
    float bsy = 112.f - (y_ - camY_) * zoom_;
    spr(art_.shade, bsx + 6.f, bsy + 7.f, bh * 0.42f, PAL_TRAM, true);
    spr(art_.tram[fi], bsx, bsy, bh, PAL_TRAM, false);

    char line[48];
    if (mode_ == Mode::Title) {
        hudC(22, "LAND ON THE GRASS", PAL_HUD);
        hudC(24, "FULL STOP TO PASS", PAL_BANNER);
        hudC(26, "ENTER TO START", PAL_HUD);
    } else if (mode_ == Mode::Run) {
        std::snprintf(line, sizeof line, "SPD %4.1f", std::fabs(speed_));
        hud(1, 1, line, PAL_HUD);
        std::snprintf(line, sizeof line, "%4.0fS", std::max(0.f, kTimeLimit - race_));
        hud(33, 1, line, race_ > kTimeLimit - 10.f ? PAL_ALERT : PAL_HUD);
        hudC(26, hint(), hold_ > 0.02f ? PAL_WIN : PAL_BANNER);
    } else if (mode_ == Mode::Pause) {
        hudC(24, "ENTER TO ROLL", PAL_HUD);
    } else if (mode_ == Mode::Win) {
        hudC(24, "ENTER FOR ANOTHER RUN", PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        hudC(24, "ENTER TO TRY AGAIN", PAL_HUD);
    }

    camX_ += jx / std::max(zoom_, 0.4f);
    camY_ -= jy / std::max(zoom_, 0.4f);
}

}  // namespace tramgrass

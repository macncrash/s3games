#include "skiff.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace skiffgrass {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;

constexpr float kStartX = 46.f;
constexpr float kStartY = 38.f;
constexpr float kStartH = 1.65f;

constexpr float kBarY = 122.f;
constexpr float kBarX0 = 28.f;
constexpr float kBarX1 = 136.f;
constexpr float kBarHalf = 8.5f;

constexpr float kGrassX = 34.f;
constexpr float kGrassY0 = 184.f;
constexpr float kGrassY1 = 336.f;
constexpr float kEndX = 13.f;
constexpr float kEndY0 = 258.f;
constexpr float kEndY1 = 302.f;
constexpr float kEndMid = (kEndY0 + kEndY1) * 0.5f;

constexpr float kStop = 0.34f;
constexpr float kSettle = 0.50f;
constexpr float kShort = 3.5f;
constexpr float kTitleZoom = 0.85f;
constexpr float kTitleCamX = 16.f;
constexpr float kTitleCamY = 156.f;
constexpr float kPlayZoom = 1.65f;

const float kReed[8][2] = {
    {40.f, 122.f}, {58.f, 124.f}, {76.f, 120.f}, {94.f, 125.f}, {112.f, 121.f}, {128.f, 123.f}, {36.f, 132.f}, {70.f, 112.f},
};
const float kTuft[10][2] = {
    {-26.f, 198.f}, {26.f, 206.f}, {-28.f, 228.f}, {28.f, 242.f}, {-24.f, 258.f},
    {22.f, 272.f},  {-26.f, 294.f}, {24.f, 308.f}, {-20.f, 324.f}, {20.f, 330.f},
};
const float kPost[8][2] = {
    {-34.f, 188.f}, {34.f, 188.f}, {-34.f, 230.f}, {34.f, 230.f}, {-34.f, 290.f}, {34.f, 290.f}, {-20.f, 336.f}, {20.f, 336.f},
};
const float kIsle[2][3] = {{-64.f, 150.f, 12.f}, {86.f, 176.f, 14.f}};

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

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (inEnd_) return 3;
    if (onGrass_) return 2;
    return 1;
}

int Game::hullFrame() const {
    float u = std::fmod(heading_, kTau);
    if (u < 0.f) u += kTau;
    int i = int(std::lround(u / kTau * 16.f)) % 16;
    if (i < 0) i += 16;
    return i;
}

const char* Game::hint() const {
    if (inEnd_) return std::fabs(speed_) > 1.f ? "BRAKE, THEN LET GO" : "HOLD THE FULL STOP";
    if (onGrass_) return "THE END IS THE PALE BAND";
    if (y_ < kBarY) return "LEAVE THE REEDS TO PORT";
    return "LAND ON THE GRASS";
}

void Game::begin() {
    x_ = kStartX;
    y_ = kStartY;
    heading_ = kStartH;
    speed_ = 0.f;
    throttle_ = 0.f;
    raceTime_ = 0.f;
    settle_ = 0.f;
    short_ = 0.f;
    wakeT_ = 0.f;
    stuckT_ = 0.f;
    stuckX_ = x_;
    stuckY_ = y_;
    wakeCursor_ = 0;
    onGrass_ = false;
    inEnd_ = false;
    landed_ = false;
    won_ = false;
    over_ = false;
    chimeN_ = 0;
    why_[0] = 0;
    report_[0] = 0;
    std::snprintf(why_, sizeof why_, "running");
    for (Wake& w : wakes_) w = {};
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
    blip(620.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.78f);
    if (bot_) {
        begin();
        mode_ = Mode::Run;
        zoom_ = kPlayZoom;
        camX_ = x_;
        camY_ = y_;
    } else {
        showTitle();
    }
}

void Game::human(float& steer, float& throttle) {
    const gs::Pad& p = sys_->pad;
    steer = 0.f;
    if (p.down(gs::BTN_LEFT)) steer += 1.f;
    if (p.down(gs::BTN_RIGHT)) steer -= 1.f;
    if (std::fabs(p.axisX) > 0.18f) steer = std::clamp(-p.axisX, -1.f, 1.f);
    const bool go = p.down(gs::BTN_UP) || p.down(gs::BTN_C) || p.down(gs::BTN_A) || p.axisY > 0.28f || p.accel > 0.2f;
    const bool stop = p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.down(gs::BTN_X) || p.down(gs::BTN_TURBO) ||
                      p.axisY < -0.28f || p.brake > 0.2f;
    if (stop) throttle_ = std::max(-1.f, throttle_ - kDt * 1.8f);
    else if (go) throttle_ = std::min(1.f, throttle_ + kDt * 1.15f);
    else {
        float decay = std::fabs(speed_) < 0.5f ? 2.6f : 0.55f;
        if (throttle_ > 0.f) throttle_ = std::max(0.f, throttle_ - kDt * decay);
        else throttle_ = std::min(0.f, throttle_ + kDt * decay);
    }
    throttle = throttle_;
}

void Game::pilot(float& steer, float& throttle) {
    auto drive = [&](float tx, float ty, float th) {
        float dx = tx - x_, dy = ty - y_;
        float err = std::hypot(dx, dy) < 7.f ? wrap(kPi * 0.5f - heading_) : wrap(std::atan2(dy, dx) - heading_);
        steer = std::clamp(err / 0.30f, -1.f, 1.f);
        if (std::fabs(err) > 1.05f) th *= 0.28f;
        throttle = th;
    };
    if (y_ < kGrassY0 - 8.f && x_ > 12.f && y_ > kBarY - 36.f && y_ < kBarY + 22.f) {
        drive(-6.f, std::min(y_, kBarY - 18.f), 0.8f);
        return;
    }
    if (y_ < kBarY - 6.f && x_ > 4.f) {
        drive(-4.f, 104.f, 0.84f);
        return;
    }
    if (!onGrass_) {
        drive(0.f, 236.f, y_ < 160.f ? 0.72f : 0.46f);
        return;
    }
    if (y_ < kEndY0 + 6.f) {
        drive(0.f, kEndMid, speed_ > 7.5f ? 0.02f : 0.36f);
        return;
    }
    float bias = std::clamp(-x_ * 0.07f, -0.4f, 0.4f);
    steer = std::clamp(wrap(kPi * 0.5f + bias - heading_) / 0.22f, -1.f, 1.f);
    if (speed_ > 0.7f) throttle = -1.f;
    else if (speed_ > 0.22f) throttle = -0.35f;
    else if (speed_ < -0.22f) throttle = 0.3f;
    else throttle = 0.f;
}

void Game::succeed() {
    if (won_) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    speed_ = 0.f;
    throttle_ = 0.f;
    std::snprintf(why_, sizeof why_, "full stop");
    std::snprintf(report_, sizeof report_,
                  "S3 SKIFF GRASS  PASS  landed on the grass and came to a full stop  (%.1f s)", raceTime_);
    std::printf("%s\n", report_);
    std::fflush(stdout);
    chime(5);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    speed_ = 0.f;
    throttle_ = 0.f;
    std::snprintf(why_, sizeof why_, "%s", why);
    sys_->apu.noiseBurst(0.42f, 90.f, 0.4f);
    sys_->apu.tone(0, 78.f, 0.06f);
    tone0_ = 0.4f;
}

void Game::physics(float dt, float steer, float throttle) {
    raceTime_ += dt;
    float rate = 1.35f + std::min(std::fabs(speed_), 18.f) * 0.045f;
    heading_ = wrap(heading_ + steer * rate * dt);

    const bool grassNow = std::fabs(x_) <= kGrassX && y_ >= kGrassY0 && y_ <= kGrassY1;
    if (grassNow && throttle < -0.02f) {
        float decel = (-throttle) * 3.4f;
        if (speed_ > 0.02f) speed_ = std::max(0.f, speed_ - decel * dt);
        else speed_ = std::max(-4.5f, speed_ - decel * 0.55f * dt);
    } else {
        float cap = grassNow ? 11.f : 24.f;
        if (!grassNow && throttle < 0.f) cap = 10.f;
        float target = std::max(0.f, throttle) * cap;
        if (!grassNow && throttle < 0.f) target = throttle * cap;
        float ak = grassNow ? 1.25f : 1.5f;
        speed_ += (target - speed_) * (1.f - std::exp(-ak * dt));
        if (grassNow && throttle < 0.08f && speed_ > 0.f) speed_ = std::max(0.f, speed_ - 1.3f * dt);
    }
    speed_ = std::clamp(speed_, -8.f, 28.f);

    float c = std::cos(heading_), s = std::sin(heading_);
    x_ += c * speed_ * dt;
    y_ += s * speed_ * dt;

    auto bump = [&](float rx, float ry, float rad) {
        float dx = x_ - rx, dy = y_ - ry;
        float d = std::hypot(dx, dy);
        if (d < rad && d > 0.01f) {
            x_ = rx + dx / d * rad;
            y_ = ry + dy / d * rad;
            speed_ *= 0.55f;
            if (thumpT_ <= 0.f) {
                sys_->apu.noiseBurst(0.28f, 240.f, 0.1f);
                thumpT_ = 0.28f;
            }
        }
    };
    if (x_ > kBarX0 && x_ < kBarX1 && std::fabs(y_ - kBarY) < kBarHalf) {
        y_ = y_ < kBarY ? kBarY - kBarHalf - 0.4f : kBarY + kBarHalf + 0.4f;
        speed_ *= 0.35f;
        if (thumpT_ <= 0.f) {
            sys_->apu.noiseBurst(0.34f, 180.f, 0.16f);
            thumpT_ = 0.35f;
        }
    }
    for (const float* isle : kIsle) bump(isle[0], isle[1], isle[2]);
    bump(52.f, 26.f, 6.f);
    bump(74.f, 28.f, 5.f);
    if (x_ < -88.f) {
        x_ = -88.f;
        speed_ *= 0.4f;
    } else if (x_ > 150.f) {
        x_ = 150.f;
        speed_ *= 0.4f;
    }
    if (y_ < 18.f) {
        y_ = 18.f;
        if (s < 0.f) speed_ *= 0.4f;
    }

    onGrass_ = std::fabs(x_) <= kGrassX && y_ >= kGrassY0 && y_ <= kGrassY1;
    inEnd_ = onGrass_ && std::fabs(x_) <= kEndX && y_ >= kEndY0 && y_ <= kEndY1;
    if (onGrass_ && !landed_) {
        landed_ = true;
        blip(480.f);
        sys_->rumble(0.35f, 0.15f, 90);
    }

    if (y_ > kGrassY1 + 4.f) {
        fail("missed the end");
        return;
    }
    if (y_ >= kGrassY0 && y_ <= kGrassY1 + 8.f && std::fabs(x_) > kGrassX + 16.f) {
        fail("off the grass");
        return;
    }
    if (y_ > kEndY0 && std::fabs(x_) > kGrassX + 18.f) {
        fail("missed the end");
        return;
    }
    if (raceTime_ > 90.f) {
        fail("timed out");
        return;
    }

    if (inEnd_ && std::fabs(speed_) <= kStop) {
        settle_ += dt;
        speed_ *= std::exp(-5.f * dt);
        if (settle_ >= kSettle) {
            succeed();
            return;
        }
    } else {
        settle_ = 0.f;
    }

    if (mode_ != Mode::Run) return;
    if (onGrass_ && !inEnd_ && std::fabs(speed_) <= kStop) {
        short_ += dt;
        if (short_ >= kShort) {
            fail("missed the end");
            return;
        }
    } else {
        short_ = 0.f;
    }

    wakeT_ -= dt;
    if (!onGrass_ && wakeT_ <= 0.f && std::fabs(speed_) > 5.f) {
        wakeT_ = 0.07f;
        Wake w;
        w.x = x_ - c * 9.f;
        w.y = y_ - s * 9.f;
        w.life = 1.f;
        wakes_[wakeCursor_] = w;
        wakeCursor_ = (wakeCursor_ + 1) % 20;
    }
    for (Wake& w : wakes_)
        if (w.life > 0.f) w.life -= dt;

    if (bot_) {
        stuckT_ += dt;
        if (stuckT_ > 1.8f) {
            float moved = std::hypot(x_ - stuckX_, y_ - stuckY_);
            stuckX_ = x_;
            stuckY_ = y_;
            stuckT_ = 0.f;
            if (moved < 4.f) {
                heading_ = wrap(heading_ + 2.1f);
                speed_ = std::max(speed_, 6.f);
            }
        }
    }
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.05f);
    tone1_ = 0.09f;
}

void Game::chime(int notes) {
    chimeN_ = std::clamp(notes, 1, 5);
    chimeStep_ = 0;
    chimeT_ = 0.02f;
}

void Game::audio(float dt) {
    float water = mode_ == Mode::Run ? 0.014f + std::fabs(speed_) * 0.00045f : 0.008f;
    sys_->apu.noise(onGrass_ ? water * 0.35f : water, onGrass_ ? 280.f : 640.f, false);
    if (mode_ == Mode::Run && (throttle_ > 0.05f || std::fabs(speed_) > 2.f)) {
        float wob = 0.6f + 0.4f * std::sin(t_ * (14.f + std::max(0.f, throttle_) * 22.f));
        float base = onGrass_ ? 34.f : 52.f;
        float vol = (0.012f + std::max(0.f, throttle_) * 0.028f) * wob;
        sys_->apu.tone(2, base + std::max(0.f, throttle_) * 28.f + std::fabs(speed_) * 0.4f, vol);
    } else if (tone0_ <= 0.f) {
        sys_->apu.tone(2, 0.f, 0.f);
    }
    if (tone0_ > 0.f) {
        tone0_ -= dt;
        if (tone0_ <= 0.f) sys_->apu.tone(0, 0.f, 0.f);
    }
    if (tone1_ > 0.f) {
        tone1_ -= dt;
        if (tone1_ <= 0.f) sys_->apu.tone(1, 0.f, 0.f);
    }
    if (thumpT_ > 0.f) thumpT_ -= dt;
    if (chimeN_ > 0) {
        chimeT_ -= dt;
        if (chimeT_ <= 0.f) {
            static const float notes[] = {392.f, 494.f, 587.f, 784.f, 988.f};
            sys_->apu.tone(0, notes[std::min(chimeStep_, 4)], 0.05f);
            tone0_ = 0.12f;
            chimeT_ = 0.14f;
            if (++chimeStep_ >= chimeN_) chimeN_ = 0;
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C)) startRun();
        else if (pad.pressed(gs::BTN_MODE)) sys.quit();
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(360.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        } else {
            float steer = 0.f, thr = 0.f;
            if (bot_) pilot(steer, thr);
            else human(steer, thr);
            throttle_ = thr;
            physics(kDt, steer, throttle_);
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Run;
        else if (pad.pressed(gs::BTN_MODE)) showTitle();
    } else if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C))) {
        startRun();
    } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
        showTitle();
    }
    camera();
    audio(kDt);
    draw();
}

void Game::camera() {
    if (mode_ == Mode::Title) {
        camX_ = kTitleCamX;
        camY_ = kTitleCamY;
        zoom_ = kTitleZoom;
        return;
    }
    float lead = mode_ == Mode::Run ? 18.f : 0.f;
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
    if (cx + w < -8 || cy + h < -8 || cx - w > gs::SCREEN_W + 8 || cy - h > gs::SCREEN_H + 8) return;
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

void Game::place(const gs::Mipped& m, float wx, float wy, float worldH, int pal, float minPx) {
    float sx = 160.f + (wx - camX_) * zoom_;
    float sy = 112.f - (wy - camY_) * zoom_;
    float h = worldH * zoom_;
    if (h < minPx) h = minPx;
    spr(m, sx, sy, h, pal, false);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.roadTime = int(t_ * 36.f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (112.f - y) / std::max(zoom_, 0.2f);
        float u = std::clamp((wy - 20.f) / 340.f, 0.f, 1.f);
        uint16_t water = lerpC(gs::rgb4(3, 10, 13), gs::rgb4(1, 4, 7), u);
        float shimmer = 0.5f + 0.5f * std::sin(wy * 0.18f + t_ * 1.7f);
        if (shimmer > 0.92f) water = lerpC(water, gs::rgb4(9, 14, 14), 0.45f);
        v.lineBackdrop[y] = water;
        v.lineFog[y] = 0;
        gs::RoadLine& r = v.road[y];
        if (wy >= kGrassY0 && wy <= kGrassY1) {
            bool endBand = wy >= kEndY0 && wy <= kEndY1;
            r.on = true;
            r.cx = 160.f + (0.f - camX_) * zoom_;
            r.hw = std::max(2.f, kGrassX * zoom_);
            r.v = wy * 22.f;
            r.pal = uint8_t(endBand ? PAL_ENDF : PAL_FIELD);
            r.band = (int(std::floor(wy * 0.18f)) & 1) ? 1 : 0;
            r.style = 0;
            r.left = 1;
            r.right = 1;
        } else {
            r.on = false;
        }
    }

    auto banner = [&](const gs::Mipped& m, float x, float y, int pal) { spr(m, x, y, float(m.h), pal, false); };
    if (mode_ == Mode::Title) banner(art_.title, 160.f, 18.f, PAL_BANNER);
    else if (mode_ == Mode::Pause) banner(art_.paused, 160.f, 96.f, PAL_BANNER);
    else if (mode_ == Mode::Fail) {
        banner(std::strcmp(why_, "off the grass") == 0 ? art_.offGrass : art_.missed, 160.f, 78.f, PAL_ALERT);
        banner(art_.legFail, 160.f, 108.f, PAL_ALERT);
    } else if (mode_ == Mode::Win) {
        banner(art_.fullStop, 160.f, 74.f, PAL_WIN);
        banner(art_.onGrass, 160.f, 108.f, PAL_WIN);
    }

    // Earlier sprites sit on top. Hull and chart marks go in before the field.
    if (mode_ == Mode::Run || mode_ == Mode::Pause) {
        float ex = 0.f, ey = kEndMid;
        float psx = 160.f + (ex - camX_) * zoom_;
        float psy = 112.f - (ey - camY_) * zoom_;
        if (psx < 16.f || psx > 304.f || psy < 16.f || psy > 208.f) {
            float dx = psx - 160.f, dy = psy - 112.f;
            float k = 1.f;
            if (std::fabs(dx) > 1.f) k = std::min(k, 142.f / std::fabs(dx));
            if (std::fabs(dy) > 1.f) k = std::min(k, 90.f / std::fabs(dy));
            spr(art_.pin, 160.f + dx * k, 112.f + dy * k, 11.f, PAL_MARK, false);
        }
    }

    float bob = onGrass_ ? 0.f : std::sin(t_ * 2.6f) * 0.7f;
    float bsx = 160.f + (x_ - camX_) * zoom_;
    float bsy = 112.f - (y_ - camY_) * zoom_ + bob;
    float boatH = 20.f * zoom_;
    if (mode_ == Mode::Title) boatH = std::max(boatH, 16.f);
    const gs::Mipped& hull = art_.hull[hullFrame()];
    spr(hull, bsx + 3.f, bsy + 3.f, boatH, PAL_HULL, true);
    spr(hull, bsx, bsy, boatH, PAL_HULL, false);
    if (!onGrass_ && std::fabs(speed_) > 6.f) {
        float c = std::cos(heading_), s = std::sin(heading_);
        place(art_.foam, x_ + c * 12.f, y_ + s * 12.f, 3.5f + std::fabs(speed_) * 0.05f, PAL_FOAM, 2.f);
    }

    for (const Wake& w : wakes_) {
        if (w.life <= 0.f) continue;
        float h = (2.5f + (1.f - w.life) * 4.f) * (zoom_ / kPlayZoom);
        float sx = 160.f + (w.x - camX_) * zoom_;
        float sy = 112.f - (w.y - camY_) * zoom_;
        spr(art_.foam, sx, sy, std::max(2.f, h), PAL_FOAM, false);
    }

    const float markPx = mode_ == Mode::Title ? 8.f : 0.f;
    for (const float* p : kTuft) place(art_.tuft, p[0], p[1], 8.f, PAL_TUFT, markPx * 0.6f);
    for (const float* p : kReed) place(art_.reed, p[0], p[1], 16.f, PAL_REED, markPx);
    for (const float* p : kPost) place(art_.post, p[0], p[1], 8.f, PAL_WOOD, markPx * 0.7f);
    place(art_.shed, -22.f, 214.f, 22.f, PAL_WOOD, markPx);
    place(art_.shed, 58.f, 18.f, 18.f, PAL_WOOD, markPx);
    place(art_.buoy, -16.f, 78.f, 7.f, PAL_BUOY, markPx * 0.6f);
    place(art_.buoy, 22.f, 86.f, 7.f, PAL_BUOY, markPx * 0.6f);
    place(art_.flag, -15.f, kEndMid, 14.f, PAL_MARK, markPx);
    place(art_.flag, 15.f, kEndMid, 14.f, PAL_MARK, markPx);

    auto dashes = [&](float x0, float y0, float x1, float y1, int n) {
        for (int i = 0; i < n; i++) {
            float u = n == 1 ? 0.5f : float(i) / float(n - 1);
            place(art_.dash, x0 + (x1 - x0) * u, y0 + (y1 - y0) * u, 2.4f, PAL_MARK, 0.f);
        }
    };
    dashes(-kEndX, kEndY0, kEndX, kEndY0, 5);
    dashes(-kEndX, kEndY1, kEndX, kEndY1, 5);
    dashes(-kEndX, kEndY0, -kEndX, kEndY1, 4);
    dashes(kEndX, kEndY0, kEndX, kEndY1, 4);
    if ((onGrass_ || mode_ == Mode::Title) && mode_ != Mode::Win)
        place(art_.ring, 0.f, kEndMid, 28.f, PAL_MARK, mode_ == Mode::Title ? 10.f : 0.f);

    int flap = int(t_ * 4.f) & 1;
    place(art_.gull[flap], -30.f + std::sin(t_ * 0.4f) * 24.f, 250.f + std::cos(t_ * 0.25f) * 10.f, 7.f, PAL_GULL, markPx);
    place(art_.gull[1 - flap], 70.f + std::cos(t_ * 0.3f) * 18.f, 90.f, 6.f, PAL_GULL, markPx);

    if (mode_ == Mode::Run || mode_ == Mode::Pause) {
        auto chart = [&](float wx, float wy, int pal, float h) {
            spr(art_.dot, 286.f + wx * 0.22f, 56.f - (wy - 180.f) * 0.17f, h, pal, false);
        };
        chart(-kGrassX, kGrassY0, PAL_WIN, 3.f);
        chart(kGrassX, kGrassY0, PAL_WIN, 3.f);
        chart(-kGrassX, kGrassY1, PAL_WIN, 3.f);
        chart(kGrassX, kGrassY1, PAL_WIN, 3.f);
        chart(0.f, kEndMid, PAL_MARK, 5.f);
        chart(x_, y_, PAL_ALERT, 5.f);
        spr(art_.panel, 286.f, 56.f, 70.f, PAL_MAP, false);
    }

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(23, "LAND ON THE GRASS", PAL_WIN);
        hudC(24, "COME TO A FULL STOP", PAL_BANNER);
        hudC(25, "MISS THE END AND THE LEG FAILS", PAL_ALERT);
        if ((int(t_ * 2.f) & 1) == 0) hudC(27, "START", PAL_WIN);
        else hudC(27, "UP THROTTLE   DOWN BRAKE   ARROWS STEER", PAL_HUD);
        return;
    }
    hud(1, 0, "S3 SKIFF GRASS", PAL_BANNER);
    int sec = int(raceTime_);
    std::snprintf(buf, sizeof buf, "%d:%02d", sec / 60, sec % 60);
    hud(33, 0, buf, PAL_HUD);
    if (mode_ == Mode::Pause) {
        hudC(18, "START CONTINUES", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Win) {
        std::snprintf(buf, sizeof buf, "TIME %d:%02d", sec / 60, sec % 60);
        hudC(16, buf, PAL_HUD);
        if (!bot_) hudC(18, "START RUNS THE LEG AGAIN", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Fail) {
        hudC(16, why_, PAL_ALERT);
        if (!bot_) hudC(18, "START TRIES THE LEG AGAIN", PAL_HUD);
        return;
    }
    hud(1, 1, hint(), inEnd_ ? PAL_WIN : PAL_BANNER);
    int sp = int(std::lround(std::fabs(speed_)));
    std::snprintf(buf, sizeof buf, "SPD %02d  %s", sp, onGrass_ ? "GRASS" : "WATER");
    hud(1, 2, buf, onGrass_ ? PAL_WIN : PAL_HUD);
    if (inEnd_) {
        int n = std::clamp(int(settle_ / kSettle * 6.f), 0, 6);
        std::snprintf(buf, sizeof buf, "HOLD %.*s", n, "******");
        hud(1, 24, buf, PAL_WIN);
    } else if (onGrass_) {
        int dist = std::max(0, int(std::lround(kEndY0 - y_)));
        std::snprintf(buf, sizeof buf, "END %d", dist);
        hud(1, 24, buf, PAL_MARK);
    } else {
        hud(1, 24, "LEG 1  —  THE GRASS", PAL_WIN);
    }
    hud(1, 27, "MISS THE END AND THE LEG FAILS", PAL_ALERT);
}

}  // namespace skiffgrass

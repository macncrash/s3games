#include "luge.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace luge {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;

constexpr float kStartX = -40.f;
constexpr float kStartY = 28.f;
constexpr float kStartH = 0.95f;

constexpr float kWallIn = 12.4f;
constexpr float kWallOut = 22.f;
constexpr float kMouth = 158.f;
constexpr float kHead = 246.f;
constexpr float kWallFar = 258.f;
constexpr float kBerthY0 = 214.f;
constexpr float kBerthY1 = 232.f;
constexpr float kParkY = 222.f;
constexpr float kNose = 4.6f;
constexpr float kTail = 3.4f;
constexpr float kHalf = 2.15f;

constexpr float kTide = 46.f;
constexpr float kStop = 0.42f;
constexpr float kHold = 0.32f;
constexpr float kShort = 5.5f;
constexpr float kHit = 6.2f;

constexpr float kPlayZoom = 2.05f;
constexpr float kTitleZoom = 0.78f;
constexpr float kTitleCamX = 0.f;
constexpr float kTitleCamY = 188.f;

const float kPostX[6] = {-16.f, -20.f, 16.f, 20.f, -14.f, 14.f};
const float kPostY[6] = {164.f, 200.f, 176.f, 228.f, 248.f, 248.f};
const float kCrack[5][2] = {{-30.f, 70.f}, {18.f, 96.f}, {-8.f, 128.f}, {26.f, 48.f}, {-22.f, 150.f}};

float wrap(float a) {
    while (a > kPi) a -= kTau;
    while (a < -kPi) a += kTau;
    return a;
}

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    return gs::rgb4(int(ar + (br - ar) * t + 0.5f), int(ag + (bg - ag) * t + 0.5f), int(ab + (bb - ab) * t + 0.5f));
}

}  // namespace

float Game::tideLeft() const { return std::max(0.f, kTide - raceTime_); }
float Game::tideU() const {
    if (mode_ == Mode::Title) return 0.12f;
    return std::clamp(raceTime_ / kTide, 0.f, 1.f);
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (inEnd_) return 3;
    if (inSlip_) return 2;
    return 1;
}

int Game::podFrame() const {
    float u = std::fmod(heading_, kTau);
    if (u < 0.f) u += kTau;
    int i = int(std::lround(u / kTau * 8.f)) % 8;
    if (i < 0) i += 8;
    return i;
}

const char* Game::hint() const {
    if (inEnd_ && std::fabs(speed_) > 0.8f) return "DIG THE HEELS";
    if (inEnd_) return "HOLD THE BERTH";
    if (inSlip_) return "THE END IS THE HEAD OF THE SLIP";
    if (tideU() > 0.55f) return "THE TIDE IS TURNING";
    if (y_ > kMouth - 36.f) return "LINE THE NOSE ON THE END";
    return "BERTH BEFORE THE TIDE TURNS";
}

void Game::begin() {
    x_ = kStartX;
    y_ = kStartY;
    heading_ = kStartH;
    speed_ = 0.f;
    raceTime_ = 0.f;
    settle_ = 0.f;
    short_ = 0.f;
    sprayT_ = 0.f;
    tickT_ = 0.f;
    stuckT_ = 0.f;
    stuckX_ = x_;
    stuckY_ = y_;
    sprayCursor_ = 0;
    inSlip_ = false;
    inEnd_ = false;
    entered_ = false;
    won_ = false;
    over_ = false;
    chimeN_ = 0;
    why_[0] = 0;
    std::snprintf(why_, sizeof why_, "running");
    for (Spray& s : spray_) s = {};
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
    blip(540.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.7f);
    sys.apu.setEcho(0.12f, 0.18f, 0.06f);
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

void Game::human(float& steer, float& push) {
    const gs::Pad& p = sys_->pad;
    steer = 0.f;
    if (p.down(gs::BTN_LEFT)) steer += 1.f;
    if (p.down(gs::BTN_RIGHT)) steer -= 1.f;
    if (std::fabs(p.axisX) > 0.18f) steer = std::clamp(-p.axisX, -1.f, 1.f);
    const bool go = p.down(gs::BTN_UP) || p.down(gs::BTN_C) || p.down(gs::BTN_A) || p.axisY > 0.25f || p.accel > 0.2f;
    const bool stop = p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.down(gs::BTN_X) || p.axisY < -0.25f || p.brake > 0.2f;
    if (stop) push = -1.f;
    else if (go) push = 1.f;
    else push = 0.f;
}

void Game::pilot(float& steer, float& push) {
    const float north = kPi * 0.5f;
    auto hold = [&](float want, float cap) {
        float err = wrap(want - heading_);
        steer = std::clamp(err / 0.22f, -1.f, 1.f);
        if (std::fabs(err) > 0.6f) cap = std::min(cap, 1.f);
        if (speed_ > cap + 0.35f) push = -1.f;
        else if (speed_ < cap - 0.25f) push = 1.f;
        else push = cap < 0.2f ? 0.f : 0.12f;
    };

    if (y_ < 130.f) {
        hold(std::atan2(130.f - y_, -4.f - x_), 12.f);
        return;
    }
    if (y_ < kMouth - 4.f || std::fabs(x_) > 2.4f) {
        float tx = std::clamp(-x_ * 0.8f, -6.f, 6.f);
        if (y_ >= kMouth - 4.f) tx = 0.f;
        float ty = std::max(y_ + 12.f, kMouth + 8.f);
        if (y_ < kMouth - 4.f) ty = kMouth + 2.f;
        hold(std::atan2(ty - y_, tx - x_), y_ < kMouth ? 3.2f : 2.2f);
        return;
    }
    float bias = std::clamp(x_ * 0.08f, -0.22f, 0.22f);
    if (y_ >= kBerthY0) bias *= 0.15f;
    float cap = 4.2f;
    if (y_ > kBerthY0 - 10.f) cap = 1.8f;
    if (y_ >= kParkY - 2.f) cap = 0.f;
    if (y_ > kBerthY1 - 1.f) cap = 0.f;
    hold(north + bias, cap);
}

void Game::succeed() {
    if (won_) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    speed_ = 0.f;
    std::snprintf(why_, sizeof why_, "berthed");
    chime(5);
    sys_->rumble(0.3f, 0.5f, 150);
    sys_->setLight(40, 170, 80);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    speed_ = 0.f;
    std::snprintf(why_, sizeof why_, "%s", why);
    sys_->apu.noiseBurst(0.42f, 80.f, 0.4f);
    sys_->apu.tone(0, 70.f, 0.06f);
    tone0_ = 0.4f;
    sys_->rumble(0.55f, 0.2f, 170);
    sys_->setLight(180, 30, 20);
}

void Game::physics(float dt, float steer, float push) {
    raceTime_ += dt;
    float rate = 1.15f + std::min(std::fabs(speed_), 16.f) * 0.09f;
    if (std::fabs(speed_) < 0.8f) rate *= 0.45f;
    heading_ = wrap(heading_ + steer * rate * dt);

    const bool slipNow = std::fabs(x_) < kWallIn && y_ > kMouth && y_ < kHead;
    float cap = slipNow ? 6.4f : 16.5f;
    if (push < -0.05f) {
        speed_ -= 11.f * dt;
        if (speed_ < -1.6f) speed_ = -1.6f;
    } else if (push > 0.05f) {
        speed_ += (push * cap - speed_) * (1.f - std::exp(-2.4f * dt));
    } else {
        float drag = slipNow ? 1.8f : 0.55f;
        speed_ *= std::exp(-drag * dt);
        if (std::fabs(speed_) < 0.08f) speed_ = 0.f;
    }
    speed_ = std::clamp(speed_, -1.6f, 18.f);

    float c = std::cos(heading_), s = std::sin(heading_);
    x_ += c * speed_ * dt;
    y_ += s * speed_ * dt;

    float u = std::clamp(raceTime_ / kTide, 0.f, 1.f);
    float ebb = 0.08f + 1.15f * u * u;
    bool sheltered = std::fabs(x_) < kWallIn - 0.4f && y_ > kMouth + 4.f && y_ < kHead && u < 0.82f;
    if (!sheltered) x_ += ebb * dt;

    if (x_ < -72.f) {
        x_ = -72.f;
        speed_ *= 0.4f;
    } else if (x_ > 72.f) {
        x_ = 72.f;
        speed_ *= 0.4f;
    }
    if (y_ < 10.f) {
        y_ = 10.f;
        if (s < 0.f) speed_ *= 0.4f;
    }

    auto sample = [&](float along, float beam, float& px, float& py) {
        px = x_ + c * along - s * beam;
        py = y_ + s * along + c * beam;
    };
    float noseX, noseY, tailX, tailY;
    sample(kNose, 0.f, noseX, noseY);
    sample(-kTail, 0.f, tailX, tailY);

    auto pastHead = [&](float px, float py) {
        return py >= kHead - 0.4f && py <= kHead + 18.f && px >= -kWallOut - 1.f && px <= kWallOut + 1.f;
    };
    if (pastHead(x_, y_) || pastHead(noseX, noseY)) {
        fail("missed the end");
        return;
    }

    float hitSpd = std::fabs(speed_);
    bool scraped = false;
    auto pushOut = [&](float px, float py, float rad, float x0, float x1, float y0, float y1) {
        float nx = std::clamp(px, x0, x1);
        float ny = std::clamp(py, y0, y1);
        float dx = px - nx, dy = py - ny;
        float d2 = dx * dx + dy * dy;
        if (d2 >= rad * rad) return;
        scraped = true;
        float d = std::sqrt(std::max(d2, 1e-6f));
        float pen = rad - d + 0.05f;
        if (d < 0.05f) {
            if (px < (x0 + x1) * 0.5f) x_ -= rad;
            else x_ += rad;
        } else {
            x_ += dx / d * pen;
            y_ += dy / d * pen;
        }
    };
    const float pts[][3] = {{x_, y_, kHalf + 0.3f}, {noseX, noseY, 1.15f}, {tailX, tailY, 1.05f}};
    for (const auto& p : pts) {
        pushOut(p[0], p[1], p[2], -kWallOut, -kWallIn, kMouth - 4.f, kWallFar);
        pushOut(p[0], p[1], p[2], kWallIn, kWallOut, kMouth - 4.f, kWallFar);
    }
    if (scraped) {
        if (hitSpd > kHit) {
            fail("hit the wall");
            return;
        }
        speed_ *= 0.42f;
        if (thumpT_ <= 0.f) {
            sys_->apu.noiseBurst(0.28f, 180.f, 0.12f);
            thumpT_ = 0.28f;
            sys_->rumble(0.2f, 0.08f, 60);
        }
    }

    c = std::cos(heading_);
    s = std::sin(heading_);
    sample(kNose, 0.f, noseX, noseY);
    if (pastHead(x_, y_) || pastHead(noseX, noseY) || y_ > kHead + 0.5f) {
        fail("missed the end");
        return;
    }

    float errH = std::fabs(wrap(kPi * 0.5f - heading_));
    inSlip_ = std::fabs(x_) < kWallIn - 0.5f && y_ > kMouth + 2.f && y_ < kHead - 2.f;
    bool posed = inSlip_ && std::fabs(x_) <= 5.6f && y_ >= kBerthY0 && y_ <= kBerthY1 && errH <= 0.62f &&
                 std::fabs(noseX) < kWallIn - 0.2f;
    inEnd_ = posed;
    if (inSlip_ && !entered_) {
        entered_ = true;
        blip(480.f);
    }

    if (posed && std::fabs(speed_) <= kStop) {
        settle_ += dt;
        speed_ *= std::exp(-7.f * dt);
        if (settle_ >= kHold) {
            succeed();
            return;
        }
    } else {
        settle_ = 0.f;
    }

    if (raceTime_ >= kTide) {
        fail("tide turned");
        return;
    }
    if (y_ > kBerthY0 - 4.f && y_ < kHead && std::fabs(x_) < kWallIn && !posed && std::fabs(speed_) < 0.3f) {
        short_ += dt;
        if (short_ >= kShort) {
            fail("missed the end");
            return;
        }
    } else {
        short_ = 0.f;
    }

    sprayT_ -= dt;
    if (!inSlip_ && sprayT_ <= 0.f && std::fabs(speed_) > 6.f) {
        sprayT_ = 0.06f;
        Spray w;
        w.x = x_ - c * 5.f;
        w.y = y_ - s * 5.f;
        w.life = 0.7f;
        spray_[sprayCursor_] = w;
        sprayCursor_ = (sprayCursor_ + 1) % 16;
    }
    for (Spray& w : spray_)
        if (w.life > 0.f) w.life -= dt;

    if (bot_) {
        stuckT_ += dt;
        if (stuckT_ > 2.2f) {
            float moved = std::hypot(x_ - stuckX_, y_ - stuckY_);
            stuckX_ = x_;
            stuckY_ = y_;
            stuckT_ = 0.f;
            if (moved < 1.6f && y_ < kMouth) {
                heading_ = std::atan2(kMouth - y_, -x_);
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
    float ice = mode_ == Mode::Run ? 0.01f + std::fabs(speed_) * 0.0011f : 0.006f;
    sys_->apu.noise(ice, 900.f + std::fabs(speed_) * 40.f, false);
    if (mode_ == Mode::Run && std::fabs(speed_) > 2.f) {
        float wob = 0.7f + 0.3f * std::sin(t_ * 22.f);
        sys_->apu.tone(2, 90.f + std::fabs(speed_) * 4.f, 0.012f * wob);
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
    if (mode_ == Mode::Run && tideLeft() < 10.f && tideLeft() > 0.f) {
        tickT_ -= dt;
        if (tickT_ <= 0.f) {
            blip(tideLeft() < 4.f ? 920.f : 480.f);
            tickT_ = tideLeft() < 4.f ? 0.22f : 0.5f;
        }
    }
    if (chimeN_ > 0) {
        chimeT_ -= dt;
        if (chimeT_ <= 0.f) {
            static const float notes[] = {392.f, 494.f, 587.f, 784.f, 988.f};
            sys_->apu.tone(0, notes[std::min(chimeStep_, 4)], 0.05f);
            tone0_ = 0.12f;
            chimeT_ = 0.12f;
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
            blip(340.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        } else {
            float steer = 0.f, push = 0.f;
            if (bot_) pilot(steer, push);
            else human(steer, push);
            physics(kDt, steer, push);
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Run;
        else if (pad.pressed(gs::BTN_MODE)) showTitle();
    } else if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C))) {
        startRun();
    } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
        showTitle();
    }
    if (mode_ == Mode::Run) {
        if (tideLeft() < 10.f) sys.setLight(160, 70, 30);
        else sys.setLight(40, 90, 160);
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
    float lead = mode_ == Mode::Run ? 10.f : 0.f;
    float gx = x_ + std::cos(heading_) * lead;
    float gy = y_ + std::sin(heading_) * lead;
    float gz = kPlayZoom;
    if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        gx = 0.f;
        gy = (kBerthY0 + kBerthY1) * 0.5f;
        gz = 1.35f;
    }
    float k = 1.f - std::exp(-kDt * (mode_ == Mode::Run ? 5.f : 2.8f));
    camX_ += (gx - camX_) * k;
    camY_ += (gy - camY_) * k;
    zoom_ += (gz - zoom_) * k;
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

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow, bool hflip) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    if (cx + w < -8 || cy + h < -8 || cx - w > gs::SCREEN_W + 8 || cy - h > gs::SCREEN_H + 8) return;
    gs::Sprite spt;
    long sw = std::clamp(std::lround(w), 1L, 1800L);
    long sh = std::clamp(std::lround(h), 1L, 1800L);
    spt.w = int16_t(sw);
    spt.h = int16_t(sh);
    spt.x = int16_t(std::clamp(std::lround(cx - sw * 0.5f), -2000L, 2000L));
    spt.y = int16_t(std::clamp(std::lround(cy - sh * 0.5f), -2000L, 2000L));
    spt.img = m.pick(float(sh));
    spt.pal = uint8_t(pal);
    spt.shadow = shadow;
    spt.hflip = hflip;
    sys_->vdp.sprite(spt);
}

void Game::place(const gs::Mipped& m, float wx, float wy, float worldH, int pal, float minPx, bool hflip) {
    float sx = 160.f + (wx - camX_) * zoom_;
    float sy = 112.f - (wy - camY_) * zoom_;
    float h = worldH * zoom_;
    if (h < minPx) h = minPx;
    spr(m, sx, sy, h, pal, false, hflip);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.hudEnabled = true;
    float tide = tideU();
    float zoom = std::max(zoom_, 0.2f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (112.f - y) / zoom;
        float u = std::clamp((wy + 10.f) / 280.f, 0.f, 1.f);
        uint16_t ice = lerpC(gs::rgb4(10, 13, 14), gs::rgb4(4, 7, 11), u);
        float sheen = 0.5f + 0.5f * std::sin(wy * 0.31f + t_ * 1.4f);
        if (sheen > 0.92f) ice = lerpC(ice, gs::rgb4(14, 15, 15), 0.45f);
        ice = lerpC(ice, gs::rgb4(3, 5, 6), tide * 0.45f);
        bool inCut = wy > kMouth - 2.f && wy < kHead && std::fabs(0.f) < 1.f;
        (void)inCut;
        v.lineBackdrop[y] = ice;
        v.lineFog[y] = uint8_t(std::clamp(int((std::fabs(wy - camY_) - 70.f) * 0.04f), 0, 6));
        v.road[y] = {};
    }

    auto sxOf = [&](float wx) { return 160.f + (wx - camX_) * zoom_; };
    auto syOf = [&](float wy) { return 112.f - (wy - camY_) * zoom_; };

    if (mode_ == Mode::Title) {
        spr(art_.title, 160.f, 36.f, 28.f, PAL_BANNER);
    } else if (mode_ == Mode::Win) {
        spr(art_.berthed, 160.f, 28.f, 22.f, PAL_WIN);
    } else if (mode_ == Mode::Fail) {
        const gs::Mipped& banner = std::strstr(why_, "wall") ? art_.wall : (std::strstr(why_, "tide") ? art_.tide : art_.missed);
        spr(banner, 160.f, 28.f, 22.f, PAL_ALERT);
    } else if (mode_ == Mode::Pause) {
        spr(art_.paused, 160.f, 28.f, 22.f, PAL_BANNER);
    }

    place(art_.pod[podFrame()], x_, y_, 9.2f, PAL_POD);
    float shx = sxOf(x_ + 1.2f), shy = syOf(y_ - 1.4f);
    spr(art_.pod[podFrame()], shx, shy, 9.2f * 0.92f, PAL_POD, true);

    for (const Spray& w : spray_) {
        if (w.life <= 0.f) continue;
        place(art_.spray, w.x, w.y, 2.2f + (1.f - w.life) * 2.f, PAL_SPRAY);
    }

    for (int i = 0; i < 5; i++) place(art_.crack, kCrack[i][0], kCrack[i][1], 3.2f, PAL_ICE);
    for (int i = 0; i < 6; i++) {
        float y = kMouth + 6.f + i * 14.f;
        place(art_.plank, -17.2f, y, 16.f, PAL_TIMBER);
        place(art_.plank, 17.2f, y, 16.f, PAL_TIMBER);
    }
    place(art_.cap, 0.f, kHead + 2.f, 6.5f, PAL_TIMBER);
    place(art_.stripe, 0.f, (kBerthY0 + kBerthY1) * 0.5f, 2.4f, PAL_END);
    place(art_.endMark, 0.f, kHead - 6.f, 3.6f, PAL_MARK);
    for (int i = 0; i < 6; i++) place(art_.post, kPostX[i], kPostY[i], 6.f, PAL_POST);
    place(art_.cleat, -13.2f, kParkY, 1.8f, PAL_POST);
    place(art_.cleat, 13.2f, kParkY, 1.8f, PAL_POST);
    place(art_.shed, -30.f, kHead + 8.f, 12.f, PAL_SHED);
    place(art_.lamp, -24.f, kMouth + 2.f, 7.f, PAL_LAMP);
    place(art_.lamp, 24.f, kMouth + 2.f, 7.f, PAL_LAMP);
    place(art_.flag, -kWallOut - 2.f, kHead + 4.f, 6.f, PAL_END);
    int flap = int(t_ * 6.f) & 1;
    place(art_.bird[flap], 34.f + std::sin(t_ * 0.7f) * 8.f, 120.f + std::sin(t_ * 0.4f) * 6.f, 3.4f, PAL_BIRD, 0, flap);

    for (int side = -1; side <= 1; side += 2) {
        for (int i = 0; i < 4; i++) {
            place(art_.crack, side * (36.f + i * 6.f), kHead + 14.f + (i & 1) * 4.f, 4.f, PAL_SNOW);
        }
    }

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(8, "S3 LUGE SLIP", PAL_BANNER);
        if (int(t_ * 2) % 2 == 0) hudC(16, "PRESS START", PAL_HUD);
        hudC(20, "ARROWS STEER   UP PUSH   DOWN BRAKE", PAL_HUD);
        hudC(22, "BERTH THE END BEFORE THE TIDE TURNS", PAL_HUD);
        hud(39 - int(std::strlen(S3_VERSION_STRING)), 26, S3_VERSION_STRING, PAL_HUD);
    } else if (mode_ == Mode::Run || mode_ == Mode::Pause) {
        hud(1, 1, hint(), inEnd_ ? PAL_WIN : PAL_BANNER);
        std::snprintf(buf, sizeof buf, "SPD %d", int(std::fabs(speed_) * 10.f));
        hud(1, 26, buf, PAL_HUD);
        int left = int(std::ceil(tideLeft()));
        std::snprintf(buf, sizeof buf, "TIDE %d", left);
        hud(32, 26, buf, left < 10 ? PAL_ALERT : PAL_HUD);
        int pips = std::clamp(int((1.f - tideU()) * 10.f + 0.5f), 0, 10);
        for (int i = 0; i < 10; i++) hud(21 + i, 26, i < pips ? "=" : "-", i < pips ? PAL_WIN : PAL_HUD);
        if (mode_ == Mode::Pause) {
            hudC(14, "START  RESUME", PAL_HUD);
            hudC(16, "ESC    TITLE", PAL_HUD);
        }
    } else if (mode_ == Mode::Win) {
        hudC(12, "BERTHED IN THE SLIP", PAL_WIN);
        std::snprintf(buf, sizeof buf, "TIDE LEFT %d", int(tideLeft()));
        hudC(14, buf, PAL_HUD);
        hudC(18, "START  AGAIN", PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        hudC(12, why_, PAL_ALERT);
        hudC(14, "THE LEG IS MISSED", PAL_HUD);
        hudC(18, "START  AGAIN", PAL_HUD);
    }
}

}  // namespace luge

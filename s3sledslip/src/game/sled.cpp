#include "sled.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace sledslip {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;

constexpr float kTide = 52.f;
constexpr float kStartX = -16.f;
constexpr float kStartY = 26.f;
constexpr float kStartH = 1.22f;

constexpr float kMouth = 142.f;
constexpr float kLine = 190.f;
constexpr float kLineFar = 214.f;
constexpr float kHead = 230.f;
constexpr float kWallIn = 11.2f;
constexpr float kWallOut = 22.f;
constexpr float kBerthX = 4.2f;

constexpr float kDraw = 24.f;
constexpr float kNose = kDraw * 46.f / 120.f;
constexpr float kTail = kDraw * 38.f / 120.f;

constexpr float kStop = 0.48f;
constexpr float kHold = 0.36f;
constexpr float kShort = 4.8f;
constexpr float kScrape = 4.4f;

constexpr float kPlayZoom = 3.55f;
constexpr float kTitleZoom = 2.55f;
constexpr float kTitleCamX = -16.f;
constexpr float kTitleCamY = 28.f;

const float kCribY[4] = {154.f, 174.f, 194.f, 214.f};
const float kMound[6][2] = {{-30.f, 48.f}, {28.f, 62.f}, {-26.f, 86.f}, {32.f, 96.f}, {-34.f, 118.f}, {30.f, 36.f}};

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

float Game::tideLeft() const { return std::max(0.f, kTide - raceTime_); }

float Game::tideU() const {
    if (mode_ == Mode::Title) return 0.16f;
    return std::clamp(raceTime_ / kTide, 0.f, 1.f);
}

float Game::iceHalf(float wy) const {
    float wide = 52.f - tideU() * 18.f;
    if (wide < 34.f) wide = 34.f;
    if (wy >= kMouth) return kWallIn;
    const float gate = kMouth - 32.f;
    if (wy <= gate) return wide;
    float t = (wy - gate) / (kMouth - gate);
    t = t * t * (3.f - 2.f * t);
    return wide + (kWallIn - wide) * t;
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (inEnd_) return 3;
    if (inSlip_) return 2;
    return 1;
}

int Game::teamFrame() const {
    float u = std::fmod(heading_, kTau);
    if (u < 0.f) u += kTau;
    int i = int(std::lround(u / kTau * 16.f)) % 16;
    if (i < 0) i += 16;
    return i;
}

void Game::bowAt(float& bx, float& by) const {
    bx = x_ + std::cos(heading_) * kNose;
    by = y_ + std::sin(heading_) * kNose;
}

void Game::sternAt(float& sx, float& sy) const {
    sx = x_ - std::cos(heading_) * kTail;
    sy = y_ - std::sin(heading_) * kTail;
}

const char* Game::hint() const {
    float bx, by;
    bowAt(bx, by);
    if (inEnd_ && speed_ > 0.7f) return "BRAKE ON THE END";
    if (inEnd_) return "HOLD THE BERTH";
    if (inSlip_) return "THE END IS THE HEAD OF THE SLIP";
    if (tideU() > 0.55f) return "THE TIDE IS TURNING";
    if (y_ > kMouth - 36.f) return "LINE THE RUNNERS ON THE END";
    return "BERTH BEFORE THE TIDE TURNS";
}

void Game::begin() {
    x_ = kStartX;
    y_ = kStartY;
    heading_ = kStartH;
    vx_ = 0.f;
    vy_ = 0.f;
    speed_ = 0.f;
    raceTime_ = 0.f;
    settle_ = 0.f;
    short_ = 0.f;
    tickT_ = 0.f;
    sprayT_ = 0.f;
    stuckT_ = 0.f;
    stuckX_ = x_;
    stuckY_ = y_;
    puffCursor_ = 0;
    inSlip_ = false;
    inEnd_ = false;
    won_ = false;
    over_ = false;
    chimeN_ = 0;
    std::snprintf(why_, sizeof why_, "running");
    for (Puff& p : puffs_) p = {};
    for (int i = 0; i < 26; i++) {
        flakes_[i].x = float((i * 47) % gs::SCREEN_W);
        flakes_[i].y = float((i * 83) % gs::SCREEN_H);
        flakes_[i].s = 2.f + float(i % 3);
        flakes_[i].v = 16.f + float((i * 3) % 7) * 3.f;
    }
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
    camX_ = x_;
    camY_ = y_ + 14.f;
    zoom_ = kPlayZoom;
    blip(540.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.72f);
    sys.apu.setEcho(0.12f, 0.18f, 0.06f);
    if (bot_) startRun();
    else showTitle();
}

void Game::human(float& steer, float& throttle) const {
    const gs::Pad& p = sys_->pad;
    steer = 0.f;
    if (p.down(gs::BTN_LEFT)) steer += 1.f;
    if (p.down(gs::BTN_RIGHT)) steer -= 1.f;
    if (std::fabs(p.axisX) > 0.18f) steer = std::clamp(-p.axisX, -1.f, 1.f);
    const bool push = p.down(gs::BTN_UP) || p.down(gs::BTN_C) || p.down(gs::BTN_A) || p.axisY > 0.25f || p.accel > 0.2f;
    const bool brake = p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.down(gs::BTN_X) || p.axisY < -0.25f || p.brake > 0.2f;
    if (brake) throttle = -1.f;
    else if (push) throttle = p.accel > 0.2f ? std::clamp(p.accel, 0.35f, 1.f) : 1.f;
    else throttle = 0.f;
}

void Game::pilot(float& steer, float& throttle) const {
    const float north = kPi * 0.5f;
    float bx, by;
    bowAt(bx, by);
    float spd = std::hypot(vx_, vy_);
    float want;
    float cap;
    if (y_ < kMouth - 4.f) {
        float tx = 0.f;
        float ty = kMouth + 4.f;
        want = std::atan2(ty - y_, tx - x_);
        float err = wrap(want - heading_);
        cap = std::fabs(err) > 0.55f ? 3.2f : (std::fabs(x_) > 6.f ? 5.0f : 7.2f);
        if (y_ > kMouth - 30.f) cap = std::min(cap, 4.4f);
        if (y_ > kMouth - 24.f && std::fabs(x_) > 3.5f) cap = std::min(cap, 3.1f);
    } else {
        float bias = std::clamp(x_ * 0.22f, -0.48f, 0.48f);
        if (std::fabs(x_) < 1.15f) bias *= 0.3f;
        want = north + bias;
        if (by >= kLine + 3.f && std::fabs(x_) < 2.4f && std::fabs(wrap(north - heading_)) < 0.4f) cap = 0.f;
        else if (by >= kLine - 5.f) cap = 1.25f;
        else if (by >= kLine - 16.f) cap = 2.6f;
        else cap = 4.4f;
        if (std::fabs(x_) > 3.2f && by < kLine + 8.f) cap = std::max(cap, 1.35f);
    }
    float err = wrap(want - heading_);
    steer = std::clamp(err / 0.30f, -1.f, 1.f);
    if (by > kLine && by < kLineFar && std::fabs(x_) < kBerthX && std::fabs(err) < 0.22f && spd < 0.55f) steer = 0.f;
    if (spd > cap + 0.32f) throttle = -1.f;
    else if (spd < cap - 0.22f) throttle = 0.86f;
    else throttle = 0.f;
    if (by > kLine + 7.f) throttle = -1.f;
}

void Game::succeed() {
    if (won_) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    vx_ = vy_ = 0.f;
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
    vx_ = vy_ = 0.f;
    speed_ = 0.f;
    std::snprintf(why_, sizeof why_, "%s", why);
    sys_->apu.noiseBurst(0.42f, 80.f, 0.4f);
    sys_->apu.tone(0, 70.f, 0.06f);
    tone0_ = 0.42f;
    sys_->rumble(0.55f, 0.15f, 170);
    sys_->setLight(170, 30, 20);
}

void Game::physics(float dt, float steer, float throttle) {
    raceTime_ += dt;
    float spd = std::hypot(vx_, vy_);
    float turn = (0.62f + std::min(spd, 7.f) * 0.14f) * steer;
    heading_ = wrap(heading_ + turn * dt);
    float hx = std::cos(heading_), hy = std::sin(heading_);
    float along = vx_ * hx + vy_ * hy;
    float lat = -vx_ * hy + vy_ * hx;

    bool channel = std::fabs(x_) < kWallIn + 0.4f && y_ > kMouth && y_ < kHead;
    float cap = channel ? 6.0f : 9.2f;
    if (throttle > 0.05f) {
        along += throttle * 7.1f * dt;
        if (along > cap) along = cap;
    } else if (throttle < -0.05f) {
        along -= (-throttle) * 16.f * dt;
        if (along < 0.f) along = 0.f;
        lat *= std::exp(-12.f * dt);
    }
    float drag = (std::fabs(throttle) < 0.05f && along < 1.6f) ? 5.2f : 0.22f;
    along *= std::exp(-drag * dt);
    float grip = 3.1f + (throttle < 0.f ? 6.f : 0.f) - std::min(spd, 8.f) * 0.1f;
    if (grip < 1.3f) grip = 1.3f;
    if (spd < 0.9f && throttle <= 0.f) lat = 0.f;
    else lat *= std::exp(-grip * dt);

    vx_ = hx * along - hy * lat;
    vy_ = hy * along + hx * lat;

    float u = std::clamp(raceTime_ / kTide, 0.f, 1.f);
    float ebb = 0.22f + 2.15f * u * u;
    bool sheltered = std::fabs(x_) < kWallIn - 0.5f && y_ > kMouth + 5.f && y_ < kHead && u < 0.86f;
    if (!sheltered) vx_ += ebb * dt;

    float sp = std::hypot(vx_, vy_);
    if (sp > 10.f) {
        vx_ *= 10.f / sp;
        vy_ *= 10.f / sp;
    }
    x_ += vx_ * dt;
    y_ += vy_ * dt;

    if (y_ < 12.f) {
        y_ = 12.f;
        if (vy_ < 0.f) vy_ = 0.f;
    }
    if (x_ < -68.f) {
        x_ = -68.f;
        vx_ *= 0.4f;
    } else if (x_ > 68.f) {
        x_ = 68.f;
        vx_ *= 0.4f;
    }

    float hitSpd = std::hypot(vx_, vy_);
    bool scraped = false;
    auto push = [&](float px, float py, float rad, float x0, float x1, float y0, float y1) {
        float nx = std::clamp(px, x0, x1);
        float ny = std::clamp(py, y0, y1);
        float dx = px - nx, dy = py - ny;
        float d2 = dx * dx + dy * dy;
        if (d2 >= rad * rad) return;
        scraped = true;
        if (d2 < 1e-5f) {
            float dl = px - x0, dr = x1 - px, db = py - y0, dtv = y1 - py;
            float pen = rad + 0.05f;
            if (dl <= dr && dl <= db && dl <= dtv) x_ -= pen;
            else if (dr <= db && dr <= dtv) x_ += pen;
            else if (db <= dtv) y_ -= pen;
            else y_ += pen;
        } else {
            float d = std::sqrt(d2);
            float pen = std::min(rad - d + 0.04f, 2.4f);
            x_ += dx / d * pen;
            y_ += dy / d * pen;
        }
    };
    const float boxes[3][4] = {
        {-kWallOut, -kWallIn, kMouth - 1.f, kHead + 2.f},
        {kWallIn, kWallOut, kMouth - 1.f, kHead + 2.f},
        {-kWallOut, kWallOut, kHead - 0.4f, kHead + 8.f},
    };
    float c = std::cos(heading_), s = std::sin(heading_);
    float pts[4][3] = {{x_, y_, 2.15f},
                       {x_ + c * kNose, y_ + s * kNose, 1.55f},
                       {x_ - c * kTail, y_ - s * kTail, 1.4f},
                       {0.f, 0.f, 0.9f}};
    pts[3][0] = x_ - s * 2.05f;
    pts[3][1] = y_ + c * 2.05f;
    float stbdX = x_ + s * 2.05f;
    float stbdY = y_ - c * 2.05f;
    for (const auto& box : boxes) {
        for (const auto& p : pts) push(p[0], p[1], p[2], box[0], box[1], box[2], box[3]);
        push(stbdX, stbdY, 0.9f, box[0], box[1], box[2], box[3]);
    }
    if (scraped) {
        vx_ *= 0.5f;
        vy_ *= 0.5f;
        if (thumpT_ <= 0.f) {
            sys_->apu.noiseBurst(0.28f, 180.f, 0.12f);
            thumpT_ = 0.22f;
            sys_->rumble(0.2f, 0.08f, 60);
        }
    }

    float bx, by, sx, sy;
    bowAt(bx, by);
    sternAt(sx, sy);
    float errH = std::fabs(wrap(kPi * 0.5f - heading_));
    bool inChannel = std::fabs(x_) < kWallIn - 0.2f && y_ > kMouth + 1.f && y_ < kHead - 1.f;
    if (inChannel && (by > kLineFar + 2.5f || y_ > kHead - 3.2f)) {
        fail("missed the end");
        return;
    }
    if (scraped && hitSpd > kScrape) {
        fail("scraped the crib");
        return;
    }
    if (y_ < kMouth - 1.f && std::fabs(x_) > iceHalf(y_) + 3.5f) {
        fail("off the ice");
        return;
    }

    bool pocket = by >= kLine && by <= kLineFar && std::fabs(x_) <= kBerthX && errH <= 0.55f &&
                  std::fabs(bx) < kWallIn - 1.1f;
    bool sternIn = std::fabs(sx) < kWallIn - 0.3f && sy > kMouth + 2.f && sy < kHead;
    bool posed = pocket && sternIn;
    inSlip_ = inChannel && errH < 1.05f;
    inEnd_ = posed || (inChannel && by >= kLine && by <= kLineFar && std::fabs(x_) <= kBerthX + 0.8f && errH < 0.75f);

    speed_ = std::hypot(vx_, vy_);
    if (posed && speed_ <= kStop) {
        settle_ += dt;
        vx_ *= std::exp(-7.f * dt);
        vy_ *= std::exp(-7.f * dt);
        speed_ = std::hypot(vx_, vy_);
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
    if (inChannel && by < kLine - 0.4f && speed_ < 0.4f) {
        short_ += dt;
        if (short_ >= kShort) {
            fail("missed the end");
            return;
        }
    } else {
        short_ = 0.f;
    }

    sprayT_ -= dt;
    if (sprayT_ <= 0.f && speed_ > 3.2f && mode_ == Mode::Run) {
        sprayT_ = 0.05f;
        Puff w;
        w.x = x_ - hx * (kTail * 0.7f);
        w.y = y_ - hy * (kTail * 0.7f);
        w.vx = -hx * 0.4f + lat * 0.02f;
        w.vy = -hy * 0.4f;
        w.life = 1.f;
        puffs_[puffCursor_] = w;
        puffCursor_ = (puffCursor_ + 1) % 14;
    }
    for (Puff& w : puffs_) {
        if (w.life <= 0.f) continue;
        w.life -= dt;
        w.x += w.vx * dt * 8.f;
        w.y += w.vy * dt * 8.f;
    }

    if (bot_) {
        stuckT_ += dt;
        if (stuckT_ > 1.6f) {
            float moved = std::hypot(x_ - stuckX_, y_ - stuckY_);
            stuckX_ = x_;
            stuckY_ = y_;
            stuckT_ = 0.f;
            if (moved < 1.05f && by < kLine + 2.f) {
                heading_ = y_ < kMouth ? std::atan2(kMouth + 6.f - y_, -x_) : kPi * 0.5f;
                float kick = 2.8f;
                vx_ = std::cos(heading_) * kick;
                vy_ = std::sin(heading_) * kick;
                if (std::fabs(x_) > 1.2f && y_ > kMouth) x_ *= 0.86f;
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
    float hiss = mode_ == Mode::Run ? 0.01f + speed_ * 0.0065f : 0.008f;
    if (inSlip_) hiss *= 0.55f;
    sys_->apu.noise(hiss, 280.f + speed_ * 70.f, false);
    if (mode_ == Mode::Run && speed_ > 1.2f) {
        float wob = 0.7f + 0.3f * std::sin(t_ * (10.f + speed_));
        sys_->apu.tone(2, 62.f + speed_ * 4.f, 0.012f * wob);
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
    if (mode_ == Mode::Run && tideLeft() < 12.f && tideLeft() > 0.f) {
        tickT_ -= dt;
        if (tickT_ <= 0.f) {
            blip(tideLeft() < 5.f ? 860.f : 500.f);
            tickT_ = tideLeft() < 5.f ? 0.24f : 0.5f;
        }
    }
    if (chimeN_ > 0) {
        chimeT_ -= dt;
        if (chimeT_ <= 0.f) {
            static const float notes[] = {523.f, 659.f, 784.f, 880.f, 1046.f};
            sys_->apu.tone(0, notes[std::min(chimeStep_, 4)], 0.055f);
            tone0_ = 0.14f;
            chimeT_ = 0.13f;
            if (++chimeStep_ >= chimeN_) chimeN_ = 0;
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    for (Flake& f : flakes_) {
        f.y += f.v * kDt;
        f.x += 6.f * kDt;
        if (f.y > gs::SCREEN_H + 2) {
            f.y = -2.f;
            f.x = std::fmod(f.x + 37.f, float(gs::SCREEN_W));
        }
        if (f.x > gs::SCREEN_W) f.x -= gs::SCREEN_W;
    }
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
            float steer = 0.f, thr = 0.f;
            if (bot_) pilot(steer, thr);
            else human(steer, thr);
            physics(kDt, steer, thr);
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
        if (tideLeft() < 12.f) sys.setLight(170, 70, 24);
        else sys.setLight(40, 90, 140);
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
    if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        camX_ = x_;
        camY_ = y_ + 8.f;
        zoom_ = 2.45f;
        return;
    }
    float lead = 22.f;
    float gx = x_ + std::cos(heading_) * lead * 0.25f;
    float gy = y_ + std::sin(heading_) * lead;
    float gz = kPlayZoom;
    float k = 1.f - std::exp(-kDt * 4.6f);
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

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow) {
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
    sys_->vdp.sprite(spt);
}

void Game::place(const gs::Mipped& m, float wx, float wy, float worldH, int pal, bool shadow) {
    float zoom = std::max(zoom_, 0.2f);
    float sx = 160.f + (wx - camX_) * zoom;
    float sy = 112.f - (wy - camY_) * zoom;
    spr(m, sx, sy, worldH * zoom, pal, shadow);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.roadTime = int(t_ * 40.f);
    float tide = tideU();
    float zoom = std::max(zoom_, 0.2f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (112.f - y) / zoom;
        uint16_t water = lerpC(gs::rgb4(2, 5, 9), gs::rgb4(1, 2, 5), std::clamp((wy - 10.f) / 240.f, 0.f, 1.f));
        water = lerpC(water, gs::rgb4(1, 3, 5), tide * 0.45f);
        float shim = 0.5f + 0.5f * std::sin(wy * 0.17f + t_ * 1.4f);
        if (shim > 0.94f) water = lerpC(water, gs::rgb4(8, 10, 12), 0.4f);
        v.lineBackdrop[y] = water;
        int fog = int(std::clamp((std::fabs(wy - camY_) - 28.f) * 0.05f, 0.f, 5.f));
        v.lineFog[y] = uint8_t(fog);
        gs::RoadLine& r = v.road[y];
        r = {};
        r.cx = 160.f + (0.f - camX_) * zoom;
        r.v = wy * 16.f;
        r.band = (int(std::floor(wy / 8.f)) & 1) ? 1 : 0;
        if (wy >= kMouth - 0.5f && wy <= kHead + 1.f) {
            r.on = true;
            r.hw = std::max(3.f, kWallIn * zoom);
            r.style = gs::ROAD_ICE;
            r.pal = (wy >= kLine - 1.f && wy <= kLineFar + 1.f) ? uint8_t(PAL_BERTH) : uint8_t(PAL_ICE);
            r.left = r.right = tide > 0.72f ? 1 : gs::GROUND_SNOWWALL;
        } else if (wy > kHead + 1.f && wy < kHead + 46.f) {
            r.on = true;
            r.hw = 34.f * zoom;
            r.style = gs::ROAD_SNOW;
            r.pal = uint8_t(PAL_BANK);
            r.left = r.right = gs::GROUND_SNOWWALL;
        } else if (wy > 4.f && wy < kMouth) {
            r.on = true;
            r.hw = std::max(4.f, iceHalf(wy) * zoom);
            r.style = gs::ROAD_ICE;
            r.pal = uint8_t(PAL_ICE);
            r.left = r.right = 1;
        }
    }

    auto banner = [&](const gs::Mipped& m, float x, float y, int pal) { spr(m, x, y, float(m.h), pal); };
    if (mode_ == Mode::Title) banner(art_.title, 160.f, 18.f, PAL_BANNER);
    else if (mode_ == Mode::Pause) banner(art_.paused, 160.f, 28.f, PAL_BANNER);
    else if (mode_ == Mode::Fail) {
        const gs::Mipped* msg = &art_.missed;
        if (std::strcmp(why_, "tide turned") == 0) msg = &art_.tide;
        else if (std::strcmp(why_, "scraped the crib") == 0) msg = &art_.scraped;
        else if (std::strcmp(why_, "off the ice") == 0) msg = &art_.offIce;
        banner(*msg, 160.f, 18.f, PAL_ALERT);
        banner(art_.leg, 160.f, 42.f, PAL_ALERT);
    } else if (mode_ == Mode::Win) {
        banner(art_.berthed, 160.f, 16.f, PAL_WIN);
        banner(art_.inSlip, 160.f, 44.f, PAL_WIN);
    }
    if (mode_ == Mode::Run || mode_ == Mode::Pause) {
        auto dot = [&](float wx, float wy, int pal, float h) {
            spr(art_.flake, 280.f + wx * 1.15f, 198.f - wy * 0.32f, h, pal);
        };
        dot(-kWallIn, kMouth, PAL_MAP, 3.f);
        dot(kWallIn, kMouth, PAL_MAP, 3.f);
        dot(-kWallIn, kHead, PAL_MAP, 3.f);
        dot(kWallIn, kHead, PAL_MAP, 3.f);
        dot(0.f, kLine, PAL_MAP, 4.f);
        dot(x_, y_, PAL_SLED, 4.f);
        spr(art_.panel, 280.f, 162.f, 96.f, PAL_MAP);
    }

    float bsx = 160.f + (x_ - camX_) * zoom;
    float bsy = 112.f - (y_ - camY_) * zoom;
    const gs::Mipped& team = art_.team[teamFrame()];
    spr(team, bsx + 5.f, bsy + 6.f, kDraw * zoom, PAL_SLED, true);
    spr(team, bsx, bsy, kDraw * zoom, PAL_SLED, false);

    for (const Puff& w : puffs_) {
        if (w.life <= 0.f) continue;
        place(art_.puff, w.x, w.y, 2.2f + (1.f - w.life) * 1.4f, PAL_SNOW);
    }
    int flap = int(t_ * 4.f) & 1;
    place(art_.raven[flap], 2.f, kHead + 16.f, 5.5f, PAL_BIRD);
    place(art_.hut, 0.f, kHead + 16.f, 18.f, PAL_HUT);
    place(art_.smoke[int(t_ * 2.f) & 1], 4.5f, kHead + 26.f + std::sin(t_ * 1.5f), 4.f, PAL_SNOW);
    place(art_.head, 0.f, kHead, 6.5f, PAL_CRIB);
    place(art_.bar, 0.f, kLine + 1.5f, 3.2f, PAL_MARK);
    place(art_.endMark, 0.f, kLine + 8.f, 5.f, PAL_MARK);
    place(art_.stake, -kWallIn + 0.4f, kLine, 8.f, PAL_MARK);
    place(art_.stake, kWallIn - 0.4f, kLine, 8.f, PAL_MARK);
    place(art_.stake, -kWallIn + 0.2f, kMouth + 1.f, 9.f, PAL_MARK);
    place(art_.stake, kWallIn - 0.2f, kMouth + 1.f, 9.f, PAL_MARK);
    for (float py : kCribY) {
        place(art_.crib, -kWallIn - 4.6f, py, 18.f, PAL_CRIB);
        place(art_.crib, kWallIn + 4.6f, py, 18.f, PAL_CRIB);
    }
    place(art_.lamp, -kWallIn - 2.f, kMouth + 2.f, 10.f, PAL_LAMP);
    place(art_.lamp, kWallIn + 2.f, kMouth + 2.f, 10.f, PAL_LAMP);
    place(art_.lamp, -kWallIn - 2.f, kHead - 4.f, 10.f, PAL_LAMP);
    place(art_.lamp, kWallIn + 2.f, kHead - 4.f, 10.f, PAL_LAMP);
    place(art_.staff, kWallIn + 7.f, kMouth - 8.f, 16.f, PAL_TIDE);
    place(art_.bead, kWallIn + 7.f, kMouth - 16.f + tide * 14.f, 3.2f, PAL_TIDE);
    for (const auto& m : kMound) place(art_.mound, m[0], m[1], 6.f, PAL_SNOW);

    if (mode_ == Mode::Run || mode_ == Mode::Pause) {
        float psx = 160.f + (0.f - camX_) * zoom;
        float psy = 112.f - ((kLine + 4.f) - camY_) * zoom;
        if (psx < 18.f || psx > 302.f || psy < 18.f || psy > 206.f) {
            float dx = psx - 160.f, dy = psy - 112.f;
            float k = 1.f;
            if (std::fabs(dx) > 1.f) k = std::min(k, 136.f / std::fabs(dx));
            if (std::fabs(dy) > 1.f) k = std::min(k, 84.f / std::fabs(dy));
            spr(art_.pin, 160.f + dx * k, 112.f + dy * k, 12.f, PAL_MARK);
        }
    }

    for (const Flake& f : flakes_) spr(art_.flake, f.x, f.y, f.s, PAL_SNOW);

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(21, "BERTH BEFORE THE TIDE TURNS", PAL_BANNER);
        hudC(22, "MISSING THE END FAILS THE LEG", PAL_HUD);
        hudC(24, "ARROWS STEER   C PUSH   X BRAKE", PAL_HUD);
        hudC(25, "ENTER START", PAL_BANNER);
    } else if (mode_ == Mode::Run || mode_ == Mode::Pause) {
        std::snprintf(buf, sizeof buf, "TIDE %4.1f", tideLeft());
        hud(1, 1, buf, tideLeft() < 12.f ? PAL_ALERT : PAL_HUD);
        std::snprintf(buf, sizeof buf, "SPD %4.1f", speed_);
        hud(1, 2, buf, PAL_HUD);
        if (inEnd_) hud(1, 3, "AT THE END", PAL_WIN);
        else if (inSlip_) hud(1, 3, "IN THE SLIP", PAL_BANNER);
        hudC(25, hint(), tideLeft() < 12.f ? PAL_ALERT : PAL_HUD);
        if (mode_ == Mode::Pause) hudC(26, "ENTER RESUME", PAL_BANNER);
    } else if (mode_ == Mode::Win) {
        std::snprintf(buf, sizeof buf, "TIDE LEFT %4.1f", tideLeft());
        hudC(24, buf, PAL_WIN);
        hudC(25, "ENTER RUNS THE LEG AGAIN", PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        hudC(25, "ENTER TRIES THE LEG AGAIN", PAL_ALERT);
    }
}

}  // namespace sledslip

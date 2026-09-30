#include "game/keel.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace keelbuoy {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;
constexpr float kNorth = 1.5707963f;
constexpr float kMaxSpeed = 26.f;
constexpr float kZoom = 1.7f;
constexpr float kMarkR = 38.f;

struct Mark {
    float x, y;
    const char* name;
};

struct Way {
    float x, y;
};

const Mark kMarks[3] = {
    {0.f, 360.f, "1 WINDWARD"},
    {220.f, 180.f, "2 WING"},
    {-180.f, 130.f, "3 LEEWARD"},
};

const Way kWay[] = {
    {170.f, 52.f}, {210.f, 230.f}, {0.f, 360.f}, {220.f, 180.f}, {-180.f, 130.f}, {0.f, 78.f}, {0.f, 16.f},
};
constexpr int kWayLast = 6;

float wrap(float a) {
    while (a > kPi) a -= kTau;
    while (a < -kPi) a += kTau;
    return a;
}

float windOff(float heading) { return std::fabs(wrap(heading - kNorth)); }

float polar(float ang) {
    if (ang < 0.48f) return 0.05f;
    if (ang < 1.10f) return 0.55f + 0.35f * ((ang - 0.48f) / 0.62f);
    if (ang < 2.05f) return 1.f;
    if (ang < 2.60f) return 0.86f;
    return 0.64f;
}

const char* sailName(float heading) {
    float a = windOff(heading);
    if (a < 0.48f) return "IN IRONS";
    if (a < 1.05f) return "CLOSE HAULED";
    if (a < 1.75f) return "BEAM REACH";
    if (a < 2.40f) return "BROAD REACH";
    return "RUNNING";
}

uint16_t waterAt(float worldY, float t) {
    float band = 0.5f + 0.5f * std::sin(worldY * 0.08f + t * 1.3f);
    int g = 5 + int(band * 3.f);
    int b = 9 + int(band * 4.f);
    return gs::rgb4(1, g, std::min(b, 15));
}

}  // namespace

void Game::begin() {
    x_ = 0.f;
    y_ = 36.f;
    heading_ = 0.35f;
    speed_ = 10.f;
    yaw_ = 0;
    buoys_ = 0;
    tack_ = -1;
    tackT_ = 10.f;
    wp_ = 0;
    race_ = 0;
    leftDock_ = false;
    brake_ = false;
    won_ = false;
    over_ = false;
    stuckT_ = 0;
    stuckX_ = x_;
    stuckY_ = y_;
    wakes_.clear();
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(1, 5, 10));
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.apu.setMaster(0.75f);
    begin();
    if (bot_) {
        mode_ = Mode::Sail;
        zoom_ = kZoom;
        camX_ = x_;
        camY_ = y_;
    } else {
        mode_ = Mode::Title;
        zoom_ = 0.42f;
        camX_ = 20.f;
        camY_ = 180.f;
    }
}

void Game::controls(float& steer, float& trim) {
    const gs::Pad& p = sys_->pad;
    steer = 0;
    if (p.down(gs::BTN_LEFT)) steer += 1.f;
    if (p.down(gs::BTN_RIGHT)) steer -= 1.f;
    if (std::fabs(p.axisX) > 0.2f) steer = std::clamp(-p.axisX, -1.f, 1.f);
    trim = 0.78f;
    if (p.down(gs::BTN_UP) || p.down(gs::BTN_A) || p.down(gs::BTN_C)) trim = 1.f;
    if (p.down(gs::BTN_DOWN) || p.down(gs::BTN_B)) trim = 0.2f;
    brake_ = p.down(gs::BTN_B);
}

void Game::pilot(float& steer, float& trim) {
    int wp = std::clamp(wp_, 0, kWayLast);
    float tx = kWay[wp].x;
    float ty = kWay[wp].y;
    float dist = std::hypot(tx - x_, ty - y_);
    float desired = std::atan2(ty - y_, tx - x_);
    float aim = desired;
    if (windOff(desired) < 0.58f) {
        int want = (tx < x_) ? 1 : -1;
        if (tack_ == 0) tack_ = want;
        bool crossed = (tack_ > 0 && x_ < tx - 36.f) || (tack_ < 0 && x_ > tx + 36.f);
        if (crossed && tackT_ > 1.6f) {
            tack_ = want;
            tackT_ = 0;
        }
        aim = kNorth + float(tack_) * 0.98f;
    }
    trim = 1.f;
    brake_ = false;
    if (wp == kWayLast) {
        trim = 0.35f;
        if (y_ < 70.f) {
            trim = 0.05f;
            brake_ = true;
        }
        if (dist < 10.f) aim = heading_;
    }
    if (stuckT_ > 4.5f && wp != kWayLast) {
        tack_ = -tack_;
        if (tack_ == 0) tack_ = 1;
        tackT_ = 0;
        stuckT_ = 0;
        aim = kNorth + float(tack_) * 0.98f;
    }
    float err = wrap(aim - heading_);
    steer = std::clamp(err / 0.38f, -1.f, 1.f);
    if (dist < (wp == kWayLast ? 8.f : 30.f) && wp < kWayLast) wp_++;
}

void Game::physics(float dt, float steer, float trim) {
    tackT_ += dt;
    float rate = 1.45f + std::min(speed_, 16.f) * 0.025f;
    yaw_ += (steer * rate - yaw_) * std::min(1.f, dt * 6.f);
    yaw_ = std::clamp(yaw_, -2.4f, 2.4f);
    heading_ = wrap(heading_ + yaw_ * dt);

    float gust = 1.f + 0.05f * std::sin(t_ * 0.63f);
    float target = kMaxSpeed * polar(windOff(heading_)) * std::clamp(trim, 0.f, 1.f) * gust;
    float ak = target > speed_ ? 1.5f : 2.6f;
    speed_ += (target - speed_) * (1.f - std::exp(-ak * dt));
    if (brake_) speed_ *= std::exp(-3.4f * dt);
    speed_ = std::clamp(speed_, 0.f, 32.f);

    float c = std::cos(heading_), s = std::sin(heading_);
    x_ += c * speed_ * dt;
    y_ += s * speed_ * dt;
    y_ -= 0.9f * dt;
    float lee = c * speed_ * 0.07f;
    x_ += -s * lee * dt;
    y_ += c * lee * dt;

    if (std::hypot(x_ - stuckX_, y_ - stuckY_) > 18.f) {
        stuckX_ = x_;
        stuckY_ = y_;
        stuckT_ = 0;
    } else {
        stuckT_ += dt;
    }
}

void Game::marks() {
    if (!leftDock_) {
        if (y_ > 70.f || std::fabs(x_) > 50.f) leftDock_ = true;
        return;
    }
    if (buoys_ < 3) {
        float d = std::hypot(kMarks[buoys_].x - x_, kMarks[buoys_].y - y_);
        if (d < kMarkR) {
            buoys_++;
            sys_->apu.tone(0, 520.f + buoys_ * 80.f, 0.18f);
        }
        return;
    }
    if (std::fabs(x_) < 26.f && y_ > -4.f && y_ < 34.f && speed_ < 9.f) {
        won_ = true;
        over_ = true;
        mode_ = Mode::Victory;
        sys_->apu.tone(0, 784.f, 0.22f);
        sys_->apu.tone(1, 988.f, 0.16f);
        std::printf("S3 KEELBUOY  PASS  rounded the buoys and returned to the same dock (%.1fs)\n", race_);
        std::fflush(stdout);
    }
}

void Game::update(float dt) {
    float steer = 0, trim = 0.8f;
    if (bot_) pilot(steer, trim);
    else controls(steer, trim);
    physics(dt, steer, trim);
    marks();
    if (mode_ == Mode::Sail) race_ += dt;

    if ((int(t_ * 8.f) & 7) == 0 && speed_ > 4.f) {
        Wake w;
        w.x = x_ - std::cos(heading_) * 10.f;
        w.y = y_ - std::sin(heading_) * 10.f;
        w.life = 1.f;
        wakes_.push_back(w);
    }
    for (auto& w : wakes_) w.life -= dt * 0.55f;
    wakes_.erase(std::remove_if(wakes_.begin(), wakes_.end(), [](const Wake& w) { return w.life <= 0; }), wakes_.end());
    if (wakes_.size() > 24) wakes_.erase(wakes_.begin(), wakes_.begin() + int(wakes_.size()) - 24);

    float look = mode_ == Mode::Title ? 0.f : 18.f;
    float tx = x_ + std::cos(heading_) * look;
    float ty = y_ + std::sin(heading_) * look;
    camX_ += (tx - camX_) * std::min(1.f, dt * 2.2f);
    camY_ += (ty - camY_) * std::min(1.f, dt * 2.2f);
}

bool Game::worldToScreen(float wx, float wy, float& sx, float& sy) const {
    sx = (wx - camX_) * zoom_ + 160.f;
    sy = 118.f - (wy - camY_) * zoom_;
    return sx > -80 && sx < 400 && sy > -80 && sy < 300;
}

void Game::spr(const gs::Mipped& m, float sx, float sy, float h, int pal) {
    if (h < 2.f || m.h <= 0) return;
    const gs::Image& im = m.pick(h);
    float sc = h / float(m.h);
    gs::Sprite s;
    s.img = im;
    s.w = std::max(1, int(m.w * sc));
    s.h = std::max(1, int(h));
    s.x = int(std::lround(sx - s.w * 0.5f));
    s.y = int(std::lround(sy - s.h * 0.5f));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::place(const gs::Mipped& m, float wx, float wy, float worldH, int pal) {
    float sx, sy;
    if (!worldToScreen(wx, wy, sx, sy)) return;
    spr(m, sx, sy, worldH * zoom_, pal);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = 0;
    while (s[n]) n++;
    hud(20 - n / 2, row, s, pal);
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (118.f - y) / zoom_;
        vdp.lineBackdrop[y] = waterAt(wy, t_);
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }

    place(art_.dock, 0.f, 10.f, 34.f, PAL_DOCK);
    for (int i = 2; i >= 0; i--) {
        int pal = PAL_MARK0 + i;
        float bob = std::sin(t_ * 2.1f + i) * 1.4f;
        place(art_.buoy[i], kMarks[i].x, kMarks[i].y + bob, i < buoys_ ? 16.f : 22.f, pal);
    }
    for (const Wake& w : wakes_) {
        float sx, sy;
        if (!worldToScreen(w.x, w.y, sx, sy)) continue;
        spr(art_.foam, sx, sy, 8.f + 6.f * (1.f - w.life), PAL_FOAM);
    }

    int face = int(std::floor((heading_ + kPi / 8.f) / (kPi / 4.f))) & 7;
    float bsx, bsy;
    worldToScreen(x_, y_, bsx, bsy);
    if (mode_ != Mode::Title) spr(art_.boat[face], bsx, bsy, 36.f, PAL_BOAT);
    else spr(art_.boat[face], bsx, bsy, 28.f, PAL_BOAT);

    float gx = std::fmod(t_ * 18.f, 240.f) - 40.f;
    spr(art_.gull[int(t_ * 3.f) & 1], 40.f + gx, 36.f + std::sin(t_ * 2.f) * 6.f, 12.f, PAL_GULL);
    spr(art_.gull[(int(t_ * 3.f) + 1) & 1], 200.f - gx * 0.4f, 52.f, 10.f, PAL_GULL);

    char buf[64];
    if (mode_ == Mode::Title) {
        spr(art_.title, 160, 78, 28, PAL_HUD);
        hudC(16, "ROUND THE BUOYS", PAL_HUD);
        hudC(18, "RETURN TO THE SAME DOCK", PAL_WIN);
        hudC(22, "ARROWS STEER   UP SHEETS IN", PAL_HUD);
        if ((int(t_ * 2.f) & 1) == 0) hudC(25, "START", PAL_ALERT);
        return;
    }
    if (mode_ == Mode::Pause) {
        hudC(14, "PAUSED", PAL_ALERT);
        return;
    }
    if (mode_ == Mode::Victory) {
        spr(art_.done, 160, 78, 32, PAL_WIN);
        hudC(16, "SAME DOCK", PAL_WIN);
    }

    hud(1, 0, "WIND N", PAL_HUD);
    if (buoys_ < 3) std::snprintf(buf, sizeof buf, "NEXT %s", kMarks[buoys_].name);
    else std::snprintf(buf, sizeof buf, "NEXT THE DOCK");
    hud(1, 1, buf, buoys_ < 3 ? PAL_MARK0 + buoys_ : PAL_WIN);
    hud(28, 1, sailName(heading_), PAL_ALERT);

    int sec = int(race_);
    std::snprintf(buf, sizeof buf, "SPD %02d  %d:%02d", int(std::lround(speed_)), sec / 60, sec % 60);
    hud(1, 26, buf, PAL_HUD);
    std::snprintf(buf, sizeof buf, "%d/3 BUOYS", buoys_);
    hud(28, 26, buf, PAL_WIN);
    if (windOff(heading_) < 0.48f && mode_ == Mode::Sail) hudC(24, "NO SAILING INTO THE WIND", PAL_ALERT);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    const gs::Pad& p = sys.pad;
    if (mode_ == Mode::Title) {
        begin();
        x_ = 30.f + std::sin(t_ * 0.4f) * 16.f;
        y_ = 200.f;
        heading_ = 0.9f + std::sin(t_ * 0.5f) * 0.4f;
        speed_ = 12.f;
        camX_ = 20.f;
        camY_ = 180.f;
        zoom_ = 0.42f;
        if (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A)) {
            begin();
            mode_ = Mode::Sail;
            zoom_ = kZoom;
            camX_ = x_;
            camY_ = y_;
        }
    } else if (mode_ == Mode::Sail) {
        if (!bot_ && p.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else update(kDt);
    } else if (mode_ == Mode::Pause) {
        if (p.pressed(gs::BTN_START)) mode_ = Mode::Sail;
    } else if (mode_ == Mode::Victory) {
        if (p.pressed(gs::BTN_START)) {
            begin();
            mode_ = Mode::Title;
            zoom_ = 0.42f;
        }
    }
    if ((int(t_ * 30.f) % 40) == 0) {
        sys.apu.tone(2, 90.f + 20.f * std::sin(t_), 0.03f);
    }
    draw();
}

}  // namespace keelbuoy

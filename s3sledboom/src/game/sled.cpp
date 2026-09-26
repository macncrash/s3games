#include "sled.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace sledboom {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;
constexpr float kNorth = kPi * 0.5f;

constexpr float kDraw = 22.f;
constexpr float kNose = 7.6f;
constexpr float kTail = 6.8f;
constexpr float kBeam = 2.35f;
constexpr float kDriveFwd = 1.15f;
constexpr float kDriveH = 5.4f;
constexpr float kHookX = 0.f;
constexpr float kHookY = 160.f;
constexpr float kTargetY = 158.85f;
constexpr float kMouth = 145.f;
constexpr float kDock = 173.f;
constexpr float kPost = 8.6f;
constexpr float kStartX = 0.f;
constexpr float kStartY = 18.f;
constexpr float kCrewTime = 46.f;
constexpr float kStop = 0.6f;
constexpr float kHold = 0.38f;
constexpr float kBoomSpd = 5.6f;
constexpr float kTreeSpd = 8.8f;
constexpr float kPlayZoom = 3.15f;
constexpr float kTitleZoom = 2.2f;
constexpr float kFailY = 166.6f;

constexpr float kTitleCamX = 0.f;
constexpr float kTitleCamY = 152.f;
constexpr float kPortraitY = 132.f;

float wrap(float a) {
    while (a > kPi) a -= kTau;
    while (a < -kPi) a += kTau;
    return a;
}

float smooth(float u) {
    u = std::clamp(u, 0.f, 1.f);
    return u * u * (3.f - 2.f * u);
}

float lerp(float a, float b, float u) { return a + (b - a) * u; }

float fairX(float y) {
    if (y < 36.f) return 0.f;
    if (y < 78.f) return 20.f * smooth((y - 36.f) / 42.f);
    if (y < 120.f) return 20.f * (1.f - smooth((y - 78.f) / 42.f));
    return 0.f;
}

float iceHalf(float y) {
    if (y < 132.f) return 22.f;
    if (y < 148.f) return lerp(22.f, 13.f, smooth((y - 132.f) / 16.f));
    if (y < 180.f) return 13.f;
    return 40.f;
}

struct Haz {
    float x, y, r;
};

Haz spruceAt() {
    float y = 64.f;
    return {fairX(y) - 12.2f, y, 3.55f};
}

Haz corniceAt() {
    float y = 104.f;
    return {fairX(y) + 11.4f, y, 3.45f};
}

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    return gs::rgb4(int(ar + (br - ar) * t), int(ag + (bg - ag) * t), int(ab + (bb - ab) * t));
}

void rivalPose(float u, float& x, float& y, float& h) {
    u = std::clamp(u, 0.f, 1.f);
    float y0 = 28.f, y1 = 156.f;
    y = lerp(y0, y1, u);
    float yb = std::min(y1, y + 5.f);
    auto bank = [](float yy) { return fairX(yy) - iceHalf(yy) - 5.2f; };
    x = bank(y);
    float xb = bank(yb);
    h = std::atan2(yb - y, xb - x);
}

}  // namespace

int Game::frameOf(float heading) const {
    float u = std::fmod(heading, kTau);
    if (u < 0.f) u += kTau;
    int i = int(std::lround(u / kTau * 16.f)) % 16;
    if (i < 0) i += 16;
    return i;
}

void Game::driveAt(float& dx, float& dy) const {
    if (driveOnBoom_) {
        dx = kHookX;
        dy = kHookY;
        return;
    }
    if (!driveOnSled_) {
        dx = lostX_;
        dy = lostY_;
        return;
    }
    float c = std::cos(heading_), s = std::sin(heading_);
    float bx = x_ + c * kDriveFwd;
    float by = y_ + s * kDriveFwd;
    float u = 0.f;
    if (mode_ == Mode::Win) u = 1.f;
    else if (settle_ > 0.f) u = smooth(std::clamp(settle_ / kHold, 0.f, 1.f));
    dx = lerp(bx, kHookX, u);
    dy = lerp(by, kHookY, u);
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (settle_ > 0.05f) return 3;
    if (inNotch_) return 2;
    return 1;
}

const char* Game::hint() const {
    if (driveOnBoom_) return "THE DRIVE IS ON THE BOOM";
    if (settle_ > 0.05f) return "HOLD. THE HOOK IS CLOSING";
    if (inNotch_ && speed_ > 1.1f) return "BRAKE. SET THE DRIVE ON THE RING";
    if (inNotch_) return "PUT THE DRIVE ON THE RING AND STOP";
    if (y_ > kMouth - 22.f) return "BETWEEN THE POSTS. UNDER THE HOOK";
    if (crew_ < 12.f) return "THE OTHER CREW IS CLOSE";
    return "DELIVER THE DRIVE TO THE BOOM";
}

void Game::begin() {
    x_ = kStartX;
    y_ = kStartY;
    heading_ = kNorth;
    vx_ = vy_ = speed_ = throttle_ = 0.f;
    raceTime_ = 0.f;
    crew_ = kCrewTime;
    settle_ = pastT_ = 0.f;
    stuckT_ = 0.f;
    stuckX_ = x_;
    stuckY_ = y_;
    puffCursor_ = 0;
    inNotch_ = false;
    driveOnSled_ = true;
    driveOnBoom_ = false;
    won_ = false;
    over_ = false;
    chimeN_ = 0;
    why_[0] = 0;
    report_[0] = 0;
    lostX_ = lostY_ = 0.f;
    for (Puff& p : puffs_) p = {};
    for (int i = 0; i < 22; i++) {
        flakes_[i].x = float((i * 53) % 320);
        flakes_[i].y = float((i * 37) % 224);
        flakes_[i].v = 16.f + float(i % 7) * 5.f;
        flakes_[i].s = (i % 4 == 0) ? 4.f : 3.f;
    }
}

void Game::showTitle() {
    begin();
    x_ = 0.f;
    y_ = kPortraitY;
    heading_ = kNorth;
    mode_ = Mode::Title;
    camX_ = kTitleCamX;
    camY_ = kTitleCamY;
    zoom_ = kTitleZoom;
}

void Game::startRun() {
    begin();
    mode_ = Mode::Run;
    camX_ = x_;
    camY_ = y_;
    zoom_ = kPlayZoom;
    blip(520.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.72f);
    sys.apu.setEcho(0.16f, 0.22f, 0.08f);
    if (bot_) startRun();
    else showTitle();
}

void Game::human(float& steer, float& throttle) {
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

void Game::pilot(float& steer, float& throttle) {
    float spd = std::hypot(vx_, vy_);
    float hx = std::cos(heading_), hy = std::sin(heading_);
    float along = vx_ * hx + vy_ * hy;
    auto steerTo = [&](float want) { return std::clamp(wrap(want - heading_) / 0.30f, -1.f, 1.f); };

    if (y_ > 134.f) {
        float dy = kTargetY - y_;
        float bias = std::clamp(x_ * 0.28f, -0.6f, 0.6f);
        if (std::fabs(x_) < 0.7f) bias *= 0.22f;
        float want = kNorth + bias;
        if (dy < 1.5f && std::fabs(x_) < 1.7f) want = kNorth;
        steer = steerTo(want);

        float cap;
        if (std::fabs(x_) > 3.8f && dy < 16.f) cap = 1.15f;
        else if (dy > 16.f) cap = 4.6f;
        else if (dy > 8.f) cap = 2.4f;
        else if (dy > 3.2f) cap = 1.25f;
        else if (dy > 0.8f) cap = 0.62f;
        else if (dy > -0.2f) cap = 0.f;
        else cap = -1.f;

        if (cap < 0.f) throttle = -1.f;
        else if (along > cap + 0.16f) throttle = -1.f;
        else if (along < cap - 0.14f) throttle = 0.8f;
        else throttle = 0.1f;
        return;
    }

    float look = 14.f + std::max(0.f, along) * 0.45f;
    float ty = y_ + look;
    float cx = fairX(y_);
    float tx = std::fabs(x_ - cx) > 2.8f ? cx : fairX(ty);
    if (y_ > 42.f && y_ < 82.f) tx += 1.4f;
    if (y_ > 86.f && y_ < 118.f) tx -= 1.3f;
    float want = std::atan2(ty - y_, tx - x_);
    steer = steerTo(want);

    Haz hs[2] = {spruceAt(), corniceAt()};
    for (const Haz& h : hs) {
        float ox = h.x - x_, oy = h.y - y_;
        float d = std::hypot(ox, oy);
        if (d < 12.5f && d > 0.2f) {
            float forward = hx * ox + hy * oy;
            if (forward > -1.2f) {
                float left = -hy * ox + hx * oy;
                float push = (12.5f - d) / 12.5f;
                steer += (left > 0.f ? -1.f : 1.f) * push * 1.2f;
            }
        }
    }
    steer = std::clamp(steer, -1.f, 1.f);

    float cap = 8.6f;
    float err = std::fabs(wrap(want - heading_));
    if (err > 0.5f) cap = 4.8f;
    if (std::fabs(x_ - cx) > 5.5f) cap = 4.0f;
    if (y_ > 120.f) cap = std::min(cap, 6.2f);
    if (along > cap + 0.22f) throttle = -0.8f;
    else if (along < cap - 0.65f) throttle = 0.95f;
    else throttle = 0.42f;
    (void)spd;
}

void Game::succeed() {
    if (won_) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    driveOnBoom_ = true;
    driveOnSled_ = false;
    vx_ = vy_ = speed_ = 0.f;
    std::snprintf(why_, sizeof why_, "delivered");
    std::snprintf(report_, sizeof report_,
                  "S3 SLED BOOM  DELIVERED  the drive is on the boom ahead of the other crew  (%.1f s, %.1f s left)",
                  raceTime_, std::max(0.f, crew_));
    chime(4);
    sys_->rumble(0.3f, 0.55f, 160);
    sys_->setLight(40, 170, 80);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    float c = std::cos(heading_), s = std::sin(heading_);
    lostX_ = x_ + c * kDriveFwd;
    lostY_ = y_ + s * kDriveFwd;
    if (std::strcmp(why, "the drive went over") == 0) lostY_ += 3.2f;
    if (std::strcmp(why, "the drive spilled") == 0 || std::strcmp(why, "the drive left the ice") == 0)
        driveOnSled_ = false;
    if (std::strcmp(why, "the drive went over") == 0) driveOnSled_ = false;
    float spd = speed_;
    vx_ = vy_ = speed_ = 0.f;
    std::snprintf(why_, sizeof why_, "%s", why);
    std::snprintf(report_, sizeof report_,
                  "S3 SLED BOOM  FAIL  %s  x %.1f  y %.1f  hdg %.0f  spd %.2f  crew %.1f  (%.1f s)", why, x_, y_,
                  heading_ * 57.2958f, spd, std::max(0.f, crew_), raceTime_);
    sys_->apu.noiseBurst(0.4f, 90.f, 0.38f);
    sys_->apu.tone(0, 68.f, 0.06f);
    tone0_ = 0.4f;
    sys_->rumble(0.55f, 0.16f, 170);
    sys_->setLight(170, 30, 20);
}

void Game::physics(float dt, float steer, float throttle) {
    raceTime_ += dt;
    crew_ -= dt;

    float spd = std::hypot(vx_, vy_);
    float turn = steer * (1.2f + std::min(spd, 8.f) * 0.11f);
    heading_ = wrap(heading_ + turn * dt);
    float hx = std::cos(heading_), hy = std::sin(heading_);
    float along = vx_ * hx + vy_ * hy;
    float lat = -vx_ * hy + vy_ * hx;

    float cap = y_ > kMouth ? 6.4f : 9.4f;
    if (throttle > 0.05f) {
        along += throttle * 8.4f * dt;
        if (along > cap) along = cap;
    } else if (throttle < -0.05f) {
        along -= (-throttle) * 15.f * dt;
        if (along < 0.08f && throttle < -0.85f && y_ > kMouth) along -= 1.7f * dt;
        if (along < -0.62f) along = -0.62f;
        lat *= std::exp(-11.f * dt);
    }
    float drag = std::fabs(throttle) < 0.05f ? (along < 1.7f ? 3.6f : 0.25f) : 0.16f;
    along *= std::exp(-drag * dt);
    float grip = 3.1f + (throttle < 0.f ? 6.5f : 0.f);
    if (y_ > kMouth - 1.f) grip += 2.4f;
    if (spd < 0.8f && throttle <= 0.05f) lat = 0.f;
    else lat *= std::exp(-grip * dt);

    vx_ = hx * along - hy * lat;
    vy_ = hy * along + hx * lat;
    float sp = std::hypot(vx_, vy_);
    if (sp > 11.f) {
        vx_ *= 11.f / sp;
        vy_ *= 11.f / sp;
    }
    float x0 = x_, y0 = y_;
    x_ += vx_ * dt;
    y_ += vy_ * dt;

    if (y_ < 8.f) {
        y_ = 8.f;
        if (vy_ < 0.f) vy_ = 0.f;
    }

    float hitSpd = std::hypot(vx_, vy_);
    bool treeHit = false, postHit = false, headHit = false;

    auto sample = [&](float alongB, float beam, float& px, float& py) {
        px = x_ + hx * alongB - hy * beam;
        py = y_ + hy * alongB + hx * beam;
    };
    float pts[5][3];
    sample(0.f, 0.f, pts[0][0], pts[0][1]);
    pts[0][2] = kBeam;
    sample(kNose, 0.f, pts[1][0], pts[1][1]);
    pts[1][2] = 1.7f;
    sample(-kTail, 0.f, pts[2][0], pts[2][1]);
    pts[2][2] = 1.55f;
    sample(1.f, -kBeam, pts[3][0], pts[3][1]);
    pts[3][2] = 1.15f;
    sample(1.f, kBeam, pts[4][0], pts[4][1]);
    pts[4][2] = 1.15f;

    auto pushCircle = [&](float cx, float cy, float rad, float px, float py) {
        float dx = px - cx, dy = py - cy;
        float reach = rad + 0.2f;
        float d2 = dx * dx + dy * dy;
        if (d2 >= reach * reach) return false;
        float d = std::sqrt(std::max(d2, 1e-6f));
        float pen = std::min(reach - d + 0.05f, 1.6f);
        x_ += dx / d * pen;
        y_ += dy / d * pen;
        return true;
    };
    auto pushBox = [&](float px, float py, float rad, float xa, float xb, float ya, float yb) {
        float nx = std::clamp(px, xa, xb);
        float ny = std::clamp(py, ya, yb);
        float dx = px - nx, dy = py - ny;
        float d2 = dx * dx + dy * dy;
        if (d2 >= rad * rad) return false;
        if (d2 < 1e-4f) {
            float dl = px - xa, dr = xb - px, db = py - ya, dtv = yb - py;
            float pen = rad + 0.08f;
            if (dl <= dr && dl <= db && dl <= dtv) x_ -= pen;
            else if (dr <= db && dr <= dtv) x_ += pen;
            else if (db <= dtv) y_ -= pen;
            else y_ += pen;
        } else {
            float d = std::sqrt(d2);
            float pen = std::min(rad - d + 0.06f, 1.8f);
            x_ += dx / d * pen;
            y_ += dy / d * pen;
        }
        return true;
    };

    Haz trees[2] = {spruceAt(), corniceAt()};
    for (int n = 0; n < 2; n++) {
        hx = std::cos(heading_);
        hy = std::sin(heading_);
        sample(0.f, 0.f, pts[0][0], pts[0][1]);
        sample(kNose, 0.f, pts[1][0], pts[1][1]);
        sample(-kTail, 0.f, pts[2][0], pts[2][1]);
        sample(1.f, -kBeam, pts[3][0], pts[3][1]);
        sample(1.f, kBeam, pts[4][0], pts[4][1]);
        for (const auto& p : pts) {
            for (const Haz& h : trees)
                if (pushCircle(h.x, h.y, h.r, p[0], p[1])) treeHit = true;
        }
    }
    hx = std::cos(heading_);
    hy = std::sin(heading_);
    sample(0.f, 0.f, pts[0][0], pts[0][1]);
    sample(kNose, 0.f, pts[1][0], pts[1][1]);
    sample(-kTail, 0.f, pts[2][0], pts[2][1]);
    sample(1.f, -kBeam, pts[3][0], pts[3][1]);
    sample(1.f, kBeam, pts[4][0], pts[4][1]);
    for (const auto& p : pts) {
        if (pushBox(p[0], p[1], p[2], -18.f, -kPost, kMouth + 1.f, kDock + 6.f)) postHit = true;
        if (pushBox(p[0], p[1], p[2], kPost, 18.f, kMouth + 1.f, kDock + 6.f)) postHit = true;
        if (pushBox(p[0], p[1], p[2], -18.f, 18.f, kDock, kDock + 10.f)) headHit = true;
    }

    float shove = std::hypot(x_ - x0, y_ - y0);
    if (shove > 2.3f) {
        x_ = x0 + (x_ - x0) / shove * 2.3f;
        y_ = y0 + (y_ - y0) / shove * 2.3f;
    }

    float cx = fairX(y_);
    float lim = iceHalf(y_) - 0.35f;
    float off = x_ - cx;
    bool banked = std::fabs(off) > lim;
    if (banked) {
        x_ = cx + std::copysign(lim, off);
        vx_ *= 0.5f;
        vy_ *= 0.72f;
    }

    if (treeHit || postHit || headHit || banked) {
        vx_ *= treeHit || postHit || headHit ? 0.5f : 0.85f;
        vy_ *= treeHit || postHit || headHit ? 0.5f : 0.85f;
        if (thumpT_ <= 0.f && (treeHit || postHit || headHit)) {
            sys_->apu.noiseBurst(0.26f, 170.f, 0.12f);
            thumpT_ = 0.24f;
            sys_->rumble(0.22f, 0.08f, 60);
        }
    }

    if (treeHit && hitSpd > kTreeSpd) {
        fail("the drive spilled");
        return;
    }
    if (postHit && hitSpd > kBoomSpd) {
        fail("broke the boom");
        return;
    }
    if (headHit && hitSpd > 4.8f) {
        fail("the drive went over");
        return;
    }
    if (banked && hitSpd > 10.4f && y_ < kMouth) {
        fail("the drive left the ice");
        return;
    }

    hx = std::cos(heading_);
    hy = std::sin(heading_);
    float driveX = x_ + hx * kDriveFwd;
    float driveY = y_ + hy * kDriveFwd;
    float errH = std::fabs(wrap(kNorth - heading_));
    inNotch_ = std::fabs(x_) < kPost - 0.4f && y_ > kMouth + 1.f && y_ < kDock - 1.f;

    if (driveY > kFailY) {
        fail("the drive went over");
        return;
    }
    if (driveY > 164.2f && hitSpd < 0.85f) {
        pastT_ += dt;
        if (pastT_ > 0.45f) {
            fail("the drive went over");
            return;
        }
    } else pastT_ = 0.f;

    bool onRing = std::fabs(driveX - kHookX) <= 3.85f && driveY >= 157.2f && driveY <= 163.6f;
    bool body = std::fabs(x_) <= 6.2f && y_ >= 150.f && y_ <= 167.f;
    bool posed = onRing && body && errH <= 0.85f && driveOnSled_;
    speed_ = std::hypot(vx_, vy_);
    if (posed && speed_ <= kStop + 0.2f) {
        vx_ *= std::exp(-9.f * dt);
        vy_ *= std::exp(-9.f * dt);
        speed_ = std::hypot(vx_, vy_);
        if (speed_ <= kStop) {
            bool was = settle_ > 0.02f;
            settle_ += dt;
            if (!was) {
                blip(196.f);
                sys_->apu.noiseBurst(0.16f, 90.f, 0.08f);
            }
            if (settle_ >= kHold) {
                succeed();
                return;
            }
        }
    } else settle_ = 0.f;

    if (crew_ <= 0.f) {
        fail("the other crew took the boom");
        return;
    }

    sprayT_ -= dt;
    if (sprayT_ <= 0.f && speed_ > 3.4f && y_ < kMouth) {
        sprayT_ = 0.06f;
        Puff w;
        w.x = x_ - hx * (kTail * 0.55f);
        w.y = y_ - hy * (kTail * 0.55f);
        w.vx = -hx * 0.6f;
        w.vy = -hy * 0.6f;
        w.life = 1.f;
        puffs_[puffCursor_] = w;
        puffCursor_ = (puffCursor_ + 1) % 12;
    }
    for (Puff& w : puffs_) {
        if (w.life <= 0.f) continue;
        w.life -= dt * 1.15f;
        w.x += w.vx * dt * 10.f;
        w.y += w.vy * dt * 10.f;
    }

    if (bot_) {
        stuckT_ += dt;
        if (stuckT_ > 1.5f) {
            float moved = std::hypot(x_ - stuckX_, y_ - stuckY_);
            stuckX_ = x_;
            stuckY_ = y_;
            stuckT_ = 0.f;
            if (moved < 0.85f && driveY < 157.f) {
                float tx = y_ > kMouth ? 0.f : fairX(y_ + 10.f);
                heading_ = std::atan2(8.f, tx - x_);
                vx_ = std::cos(heading_) * 3.4f;
                vy_ = std::sin(heading_) * 3.4f;
                if (std::fabs(x_ - fairX(y_)) > 3.f && y_ < kMouth) x_ = lerp(x_, fairX(y_), 0.4f);
            }
        }
    }
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.045f);
    tone1_ = 0.08f;
}

void Game::chime(int notes) {
    chimeN_ = std::clamp(notes, 1, 5);
    chimeStep_ = 0;
    chimeT_ = 0.02f;
}

void Game::audio(float dt) {
    float hiss = mode_ == Mode::Run ? 0.01f + speed_ * 0.006f : 0.012f;
    if (inNotch_ || mode_ == Mode::Win) hiss *= 0.55f;
    sys_->apu.noise(hiss, 240.f + speed_ * 60.f, false);
    if (mode_ == Mode::Run && speed_ > 1.4f) {
        float wob = 0.72f + 0.28f * std::sin(t_ * (9.f + speed_));
        sys_->apu.tone(2, 58.f + speed_ * 3.5f, 0.012f * wob);
    } else if (tone0_ <= 0.f) sys_->apu.tone(2, 0.f, 0.f);

    if (tone0_ > 0.f) {
        tone0_ -= dt;
        if (tone0_ <= 0.f) sys_->apu.tone(0, 0.f, 0.f);
    }
    if (tone1_ > 0.f) {
        tone1_ -= dt;
        if (tone1_ <= 0.f) sys_->apu.tone(1, 0.f, 0.f);
    }
    if (thumpT_ > 0.f) thumpT_ -= dt;

    if (mode_ == Mode::Run && throttle_ > 0.45f && speed_ > 2.f) {
        yipT_ -= dt;
        if (yipT_ <= 0.f) {
            yipFlip_ ^= 1;
            blip(yipFlip_ ? 740.f : 620.f);
            yipT_ = 0.62f;
        }
    }
    if (mode_ == Mode::Run && crew_ < 10.f && crew_ > 0.f) {
        tickT_ -= dt;
        if (tickT_ <= 0.f) {
            blip(crew_ < 4.f ? 880.f : 480.f);
            tickT_ = crew_ < 4.f ? 0.22f : 0.48f;
        }
    }
    if (chimeN_ > 0) {
        chimeT_ -= dt;
        if (chimeT_ <= 0.f) {
            static const float notes[] = {349.f, 440.f, 523.f, 698.f, 880.f};
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
            blip(330.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) showTitle();
        else {
            float steer = 0.f, thr = 0.f;
            if (bot_) pilot(steer, thr);
            else human(steer, thr);
            throttle_ = thr;
            physics(kDt, steer, thr);
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C)) mode_ = Mode::Run;
        else if (pad.pressed(gs::BTN_MODE)) showTitle();
    } else if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C))) startRun();
    else if (!bot_ && pad.pressed(gs::BTN_MODE)) showTitle();

    if (mode_ == Mode::Run) {
        if (crew_ < 10.f) sys.setLight(180, 90, 30);
        else sys.setLight(40, 90, 140);
    } else if (mode_ == Mode::Win) sys.setLight(40, 170, 80);
    else if (mode_ == Mode::Fail) sys.setLight(170, 30, 20);
    else sys.setLight(30, 50, 90);

    for (Flake& f : flakes_) {
        f.y += f.v * kDt;
        f.x += std::sin(t_ * 0.8f + f.y * 0.03f) * 10.f * kDt;
        if (f.y > 232.f) {
            f.y = -6.f;
            f.x = std::fmod(f.x + 41.f, 320.f);
            if (f.x < 0.f) f.x += 320.f;
        }
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
    float lead = mode_ == Mode::Run ? 12.f : 0.f;
    float gx = x_ + std::cos(heading_) * lead * 0.35f;
    float gy = y_ + std::sin(heading_) * lead;
    float gz = kPlayZoom;
    if (mode_ == Mode::Win) {
        gx = kHookX;
        gy = kHookY;
        gz = 2.7f;
    } else if (mode_ == Mode::Fail) {
        gx = x_;
        gy = y_;
        gz = 2.85f;
    }
    float k = 1.f - std::exp(-kDt * (mode_ == Mode::Run ? 4.4f : 3.f));
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
    v.roadTime = int(t_ * 30.f);
    float zoom = std::max(zoom_, 0.2f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (112.f - y) / zoom;
        v.lineBackdrop[y] = lerpC(gs::rgb4(7, 9, 12), gs::rgb4(3, 5, 8), std::clamp(y / 224.f, 0.f, 1.f));
        int fog = int(std::clamp((std::fabs(wy - camY_) - 34.f) * 0.035f, 0.f, 4.f));
        v.lineFog[y] = uint8_t(fog);
        gs::RoadLine& r = v.road[y];
        r = {};
        r.on = true;
        r.v = wy * 18.f;
        r.band = (int(std::floor(wy / 7.f)) & 1) ? 1 : 0;
        r.left = r.right = gs::GROUND_SNOWWALL;
        if (wy >= kMouth) {
            r.cx = 160.f + (0.f - camX_) * zoom;
            r.hw = std::max(8.f, (wy > 180.f ? 48.f : iceHalf(wy)) * zoom);
            r.style = gs::ROAD_SNOW;
            r.pal = uint8_t(PAL_PAD);
        } else {
            r.cx = 160.f + (fairX(wy) - camX_) * zoom;
            r.hw = std::max(6.f, iceHalf(wy) * zoom);
            r.style = gs::ROAD_ICE;
            r.pal = uint8_t(PAL_ICE);
        }
    }

    auto banner = [&](const gs::Mipped& m, float x, float y, int pal) { spr(m, x, y, float(m.h), pal); };
    if (mode_ == Mode::Title) banner(art_.title, 160.f, 16.f, PAL_BANNER);
    else if (mode_ == Mode::Pause) banner(art_.paused, 160.f, 28.f, PAL_BANNER);
    else if (mode_ == Mode::Fail) {
        const gs::Mipped* msg = &art_.leg;
        if (std::strcmp(why_, "the other crew took the boom") == 0) msg = &art_.crewTook;
        else if (std::strcmp(why_, "the drive went over") == 0) msg = &art_.wentOver;
        else if (std::strcmp(why_, "broke the boom") == 0) msg = &art_.broke;
        else if (std::strcmp(why_, "the drive spilled") == 0) msg = &art_.spilled;
        else if (std::strcmp(why_, "the drive left the ice") == 0) msg = &art_.leftIce;
        banner(*msg, 160.f, 18.f, PAL_ALERT);
        banner(art_.leg, 160.f, 42.f, PAL_ALERT);
    } else if (mode_ == Mode::Win) {
        banner(art_.delivered, 160.f, 16.f, PAL_WIN);
        banner(art_.onBoom, 160.f, 44.f, PAL_WIN);
    }

    for (const Flake& f : flakes_) spr(art_.flake, f.x, f.y, f.s, PAL_SNOW);

    float rx, ry, rh;
    float ru = mode_ == Mode::Title ? 0.86f : 1.f - std::clamp(crew_ / kCrewTime, 0.f, 1.f);
    rivalPose(ru, rx, ry, rh);
    const gs::Mipped& rival = art_.team[frameOf(rh)];
    place(rival, rx + 0.8f, ry - 0.6f, kDraw * 0.92f, PAL_RIVAL, true);
    place(rival, rx, ry, kDraw * 0.92f, PAL_RIVAL, false);

    float dx, dy;
    driveAt(dx, dy);
    bool showDrive = driveOnBoom_ || !driveOnSled_ || mode_ != Mode::Fail || std::strcmp(why_, "broke the boom") != 0;
    if (mode_ == Mode::Fail && std::strcmp(why_, "broke the boom") == 0) showDrive = true;
    if (mode_ == Mode::Fail && std::strcmp(why_, "the other crew took the boom") == 0) showDrive = true;
    if (showDrive) {
        place(art_.drive, dx + 0.35f, dy - 0.3f, kDriveH, PAL_DRIVE, true);
        place(art_.drive, dx, dy, kDriveH, PAL_DRIVE, false);
    }

    const gs::Mipped& team = art_.team[frameOf(heading_)];
    place(team, x_ + 0.7f, y_ - 0.5f, kDraw, PAL_SLED, true);
    place(team, x_, y_, kDraw, PAL_SLED, false);

    place(art_.ring, kHookX, kHookY, 6.2f, PAL_MARK);
    place(art_.hook, kHookX, kHookY + 1.6f, 5.5f, PAL_BOOM);
    place(art_.chain, kHookX, kHookY + 7.2f, 10.f, PAL_BOOM);
    place(art_.beam, 0.f, 176.f, 4.2f, PAL_BOOM);
    place(art_.beam, 0.f, kDock + 0.4f, 3.4f, PAL_BOOM);
    place(art_.tower, -13.4f, 160.f, 16.f, PAL_BOOM);
    place(art_.tower, 13.4f, 160.f, 16.f, PAL_BOOM);
    place(art_.mill, 2.f, 190.f, 22.f, PAL_MILL);
    place(art_.smoke[int(t_ * 2.f) & 1], 8.5f, 204.f + std::sin(t_ * 1.4f) * 0.6f, 5.f, PAL_SNOW);
    place(art_.lamp, -11.2f, kMouth + 2.f, 9.f, PAL_MARK);
    place(art_.lamp, 11.2f, kMouth + 2.f, 9.f, PAL_MARK);
    place(art_.stake, -kPost - 0.4f, kMouth + 1.5f, 7.f, PAL_MARK);
    place(art_.stake, kPost + 0.4f, kMouth + 1.5f, 7.f, PAL_MARK);

    Haz sp = spruceAt();
    Haz co = corniceAt();
    place(art_.spruce, sp.x, sp.y, 13.f, PAL_TREE);
    place(art_.cornice, co.x, co.y, 7.5f, PAL_SNOW);
    const float decoY[6] = {32.f, 50.f, 74.f, 92.f, 112.f, 126.f};
    for (int i = 0; i < 6; i++) {
        float yy = decoY[i];
        float half = iceHalf(yy);
        float fx = fairX(yy);
        place(art_.spruce, fx - half - 6.5f, yy, 10.f + float(i % 3), PAL_TREE);
        place(art_.spruce, fx + half + 7.f, yy + 3.f, 9.f + float((i + 1) % 3), PAL_TREE);
    }

    for (const Puff& w : puffs_) {
        if (w.life <= 0.f) continue;
        place(art_.puff, w.x, w.y, 2.4f + (1.f - std::max(0.f, w.life)) * 1.6f, PAL_SNOW);
    }

    if (mode_ == Mode::Run || mode_ == Mode::Pause) {
        float psx = 160.f + (kHookX - camX_) * zoom;
        float psy = 112.f - (kHookY - camY_) * zoom;
        if (psx < 16.f || psx > 304.f || psy < 16.f || psy > 208.f) {
            float ddx = psx - 160.f, ddy = psy - 112.f;
            float k = 1.f;
            if (std::fabs(ddx) > 1.f) k = std::min(k, 132.f / std::fabs(ddx));
            if (std::fabs(ddy) > 1.f) k = std::min(k, 80.f / std::fabs(ddy));
            spr(art_.pin, 160.f + ddx * k, 112.f + ddy * k, 12.f, PAL_MARK);
        }
    }

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(22, "DELIVER THE DRIVE TO THE BOOM", PAL_BANNER);
        hudC(23, "THE CLOCK IS THE OTHER CREW", PAL_HUD);
        hudC(25, "ARROWS STEER   C MUSH   X BRAKE", PAL_HUD);
        hudC(26, "ENTER STARTS THE LEG", PAL_BANNER);
    } else if (mode_ == Mode::Run || mode_ == Mode::Pause) {
        std::snprintf(buf, sizeof buf, "CREW %4.1f", std::max(0.f, crew_));
        hud(1, 1, buf, crew_ < 12.f ? PAL_ALERT : PAL_HUD);
        std::snprintf(buf, sizeof buf, "SPD %4.1f", speed_);
        hud(1, 2, buf, PAL_HUD);
        if (settle_ > 0.05f) hud(1, 3, "HOOK", PAL_WIN);
        else if (inNotch_) hud(1, 3, "NOTCH", PAL_BANNER);
        hudC(25, hint(), crew_ < 12.f ? PAL_ALERT : PAL_HUD);
        if (mode_ == Mode::Pause) hudC(26, "ENTER RESUME", PAL_BANNER);
    } else if (mode_ == Mode::Win) {
        std::snprintf(buf, sizeof buf, "CREW LEFT %4.1f", std::max(0.f, crew_));
        hudC(24, buf, PAL_WIN);
        hudC(26, "ENTER RUNS THE LEG AGAIN", PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        hudC(26, "ENTER TRIES THE LEG AGAIN", PAL_ALERT);
    }
}

}  // namespace sledboom

#include "dune.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace dune {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float LENGTH = 1320.f;
constexpr float WATER_Z = 560.f;
constexpr float BOX = 24.f;
constexpr float HALF_W = 5.4f;
constexpr float FIRM = 4.15f;
constexpr float ACCEL = 12.4f;
constexpr float DRAG = 0.365f;
constexpr float BRAKE_ACC = 22.f;
constexpr float VMAX = 28.f;
constexpr float STEER_S = 2.35f;
constexpr float YAW_DAMP = 3.4f;
constexpr float CRUISE = 0.72f;
constexpr float SUN = 0.43f;
constexpr float THROTTLE_HEAT = 2.75f;
constexpr float WASTE = 4.2f;
constexpr float WASTE_AT = 0.80f;
constexpr float SAND_HEAT = 2.3f;
constexpr float START_HEAT = 18.f;
constexpr float COOLED = 8.f;
constexpr float BOIL = 100.f;
constexpr int FILL_FRAMES = 96;
constexpr float FOCAL = 236.f;
constexpr float CAM_H = 1.42f;
constexpr int HORIZON = 104;
constexpr float SHIFT_DZ = 3.f;
constexpr int SHIFT_N = 110;

// Straight through the cistern and the camp so a stop is a stop, not a bend.
float kappa(float z) {
    auto smooth = [](float e0, float e1, float x) {
        float t = std::clamp((x - e0) / (e1 - e0), 0.f, 1.f);
        return t * t * (3.f - 2.f * t);
    };
    auto open = [&](float a, float b) {
        const float r = 40.f;
        if (z >= a && z <= b) return 0.f;
        if (z < a) return 1.f - smooth(a - r, a, z);
        return smooth(b, b + r, z);
    };
    float env = open(-40.f, 90.f);
    env *= open(WATER_Z - 80.f, WATER_Z + 80.f);
    env *= open(LENGTH - 70.f, LENGTH + 200.f);
    return env * (0.0072f * std::sin(z * 0.011f) + 0.0034f * std::sin(z * 0.019f + 0.8f));
}

gs::FMPatch enginePatch() {
    gs::FMPatch p;
    p.alg = 4;
    p.fb = 0.38f;
    p.op[0] = {1.f, 0.85f, 0.03f, 0.2f, 0.7f, 0.22f, 0.f};
    p.op[1] = {2.f, 0.4f, 0.04f, 0.24f, 0.45f, 0.25f, 1.4f};
    p.op[2] = {0.5f, 0.55f, 0.05f, 0.3f, 0.6f, 0.3f, 0.f};
    p.op[3] = {1.f, 0.28f, 0.05f, 0.28f, 0.5f, 0.3f, 0.f};
    p.vol = 0.16f;
    p.drive = 0.42f;
    p.tone = 820.f;
    p.vibRate = 5.5f;
    p.vibDepth = 0.01f;
    return p;
}

gs::FMPatch chimePatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.12f;
    p.op[0] = {1.f, 1.f, 0.01f, 0.16f, 0.2f, 0.4f, 0.f};
    p.op[1] = {2.f, 0.32f, 0.01f, 0.2f, 0.15f, 0.4f, 0.f};
    p.op[2] = {3.f, 0.2f, 0.01f, 0.18f, 0.1f, 0.35f, 0.f};
    p.op[3] = {1.f, 0.45f, 0.02f, 0.28f, 0.3f, 0.45f, 0.f};
    p.vol = 0.18f;
    return p;
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Count) return 1;
    if (mode_ == Mode::Result) return 5;
    if (!watered_ && fillFrames_ > 8) return 3;
    if (watered_) return 4;
    return 2;
}

void Game::poseTitle() {
    z_ = WATER_Z - 16.f;
    x_ = yaw_ = speed_ = 0;
    steer_ = brake_ = 0;
    gas_ = 0.15f;
    reverse_ = false;
    heat_ = 42.f;
    watered_ = false;
    fillFrames_ = 0;
    shake_ = flash_ = 0;
    puffs_.clear();
    tRun_ = 0;
}

void Game::layout() {
    things_.clear();
    auto add = [&](float z, float x, float h, Kind k) {
        Thing t;
        t.z = z;
        t.x = x;
        t.h = h;
        t.kind = k;
        things_.push_back(t);
    };
    for (int i = 0; i < 46; i++) {
        float z = 48.f + i * 27.f;
        if (z > LENGTH - 28.f) break;
        if (std::fabs(z - WATER_Z) < BOX + 36.f) continue;
        float side = (i & 1) ? 1.f : -1.f;
        float x = side * (7.8f + float((i * 13) % 5) * 0.28f);
        bool rock = (i % 3) == 0;
        add(z, x, rock ? 1.35f : 0.72f, rock ? ROCK : SCRUB);
    }
    add(WATER_Z, -6.5f, 2.6f, TANK);
    add(WATER_Z + 7.f, 6.9f, 3.4f, PALM);
    add(WATER_Z - 1.5f, 0.f, 1.15f, BANNER_W);
    add(WATER_Z - 1.5f, -(HALF_W + 0.15f), 2.5f, POST);
    add(WATER_Z - 1.5f, HALF_W + 0.15f, 2.5f, POST);
    add(LENGTH - 10.f, 0.f, 1.15f, BANNER_C);
    add(LENGTH - 10.f, -(HALF_W + 0.15f), 2.5f, POST);
    add(LENGTH - 10.f, HALF_W + 0.15f, 2.5f, POST);
    add(LENGTH - 2.f, 6.6f, 2.15f, TENT);
    add(LENGTH + 4.f, -6.8f, 3.2f, PALM);
    std::sort(things_.begin(), things_.end(), [](const Thing& a, const Thing& b) { return a.z < b.z; });
}

void Game::begin() {
    z_ = x_ = yaw_ = speed_ = steer_ = brake_ = 0;
    gas_ = 0;
    reverse_ = false;
    heat_ = START_HEAT;
    watered_ = false;
    fillFrames_ = 0;
    why_ = 0;
    won_ = false;
    over_ = false;
    shake_ = flash_ = 0;
    tRun_ = 0;
    modeT_ = 0;
    beep_ = -1;
    fanStep_ = -1;
    puffs_.clear();
    for (Thing& t : things_) t.hit = false;
    mode_ = bot_ ? Mode::Run : Mode::Count;
    std::snprintf(report_, sizeof report_, "S3 DUNE  FAIL  no run");
    sys_->apu.keyOn(0, 40.f, 0.05f);
}

void Game::finish(bool win, int why) {
    if (mode_ == Mode::Result) return;
    won_ = win;
    why_ = why;
    over_ = true;
    mode_ = Mode::Result;
    modeT_ = 0;
    int needle = heat_ >= BOIL ? 100 : std::clamp(int(heat_), 0, 99);
    if (win) {
        std::snprintf(report_, sizeof report_,
                      "S3 DUNE  PASS  one water stop  needle %d  camp reached  (%.1f s)", needle, tRun_);
        fanStep_ = 0;
        fanT_ = 0;
        flash_ = 0.15f;
    } else if (why == 2) {
        heat_ = BOIL;
        std::snprintf(report_, sizeof report_,
                      "S3 DUNE  FAIL  the engine boiled at %d m  needle 100  (%.1f s)", int(z_), tRun_);
        flash_ = 0.7f;
        shake_ = 7.f;
        sys_->apu.noiseBurst(0.7f, 140.f, 0.35f);
        sys_->apu.setVol(0, 0.02f);
    } else {
        std::snprintf(report_, sizeof report_,
                      "S3 DUNE  FAIL  skipped the water  needle %d  (%.1f s)", needle, tRun_);
        flash_ = 0.35f;
    }
}

bool Game::inBox() const {
    // The cistern serves the whole apron, not only the packed ruts.
    return std::fabs(z_ - WATER_Z) <= BOX && std::fabs(x_) <= 12.f;
}

float Game::sandAmt() const {
    float off = std::fabs(x_) - FIRM;
    if (off <= 0.f) return 0.f;
    return std::min(1.f, off / 3.3f);
}

Game::In Game::human(const gs::Pad& pad) const {
    In in;
    in.steer = 0;
    if (pad.down(gs::BTN_LEFT)) in.steer -= 1.f;
    if (pad.down(gs::BTN_RIGHT)) in.steer += 1.f;
    if (std::fabs(pad.axisX) > 0.12f) in.steer = pad.axisX;
    bool push = pad.down(gs::BTN_TURBO) || pad.accel > 0.72f;
    bool go = pad.down(gs::BTN_UP) || pad.down(gs::BTN_C) || pad.down(gs::BTN_A) || pad.accel > 0.12f;
    if (push) in.gas = 1.f;
    else if (go) in.gas = CRUISE;
    in.brake = (pad.down(gs::BTN_B) || pad.down(gs::BTN_X) || pad.down(gs::BTN_DOWN) || pad.brake > 0.18f) ? 1.f : 0.f;
    if (pad.down(gs::BTN_DOWN) && in.gas < 0.1f && speed_ <= 0.08f) {
        in.reverse = true;
        in.brake = 0;
        in.gas = 0;
    }
    return in;
}

float Game::autoSteer(bool reverse) const {
    float look = 14.f + std::max(0.f, speed_) * 0.38f;
    float ahead = z_ + (reverse ? -look : look);
    float k = kappa(ahead);
    float ff = k * speed_ / STEER_S;
    float dir = (reverse || speed_ < -0.3f) ? -1.f : 1.f;
    float cmd = ff - x_ * 0.16f * dir - yaw_ * 2.0f;
    return std::clamp(cmd, -1.f, 1.f);
}

Game::In Game::botDrive() const {
    In in;
    float dist = WATER_Z - z_;
    bool box = inBox();
    if (!watered_) {
        if (box && std::fabs(speed_) < 2.6f) {
            in.brake = 1.f;
        } else if (z_ > WATER_Z + BOX) {
            in.reverse = true;
        } else {
            float stopFor = std::max(0.f, speed_) * speed_ / 32.f + 6.f;
            if (dist < stopFor) in.brake = 1.f;
            else if (dist < 90.f) in.gas = 0.42f;
            else in.gas = CRUISE;
        }
    } else {
        in.gas = heat_ > 90.f ? 0.62f : CRUISE;
    }
    in.steer = autoSteer(in.reverse);
    return in;
}

void Game::integrate(const In& in) {
    float follow = std::min(1.f, 12.f * DT);
    steer_ += (in.steer - steer_) * follow;
    gas_ = in.gas;
    brake_ = in.brake;
    reverse_ = in.reverse;

    float sand = sandAmt();
    float acc = reverse_ ? -9.5f : ACCEL * gas_;
    acc -= DRAG * speed_;
    if (!reverse_ && brake_ > 0.f && speed_ > 0.f) acc -= BRAKE_ACC * brake_;
    if (sand > 0.f) {
        float bog = sand * (6.5f + 0.55f * std::fabs(speed_));
        acc -= bog * (speed_ >= 0.f ? 1.f : -1.f);
    }
    speed_ += acc * DT;
    speed_ = std::clamp(speed_, -6.5f, VMAX);
    if (!reverse_ && brake_ > 0.5f && gas_ < 0.05f && std::fabs(speed_) < 0.22f) speed_ = 0.f;

    float k = kappa(z_);
    yaw_ += (steer_ * STEER_S - YAW_DAMP * yaw_ - k * speed_) * DT;
    yaw_ = std::clamp(yaw_, -1.05f, 1.05f);
    x_ += speed_ * std::sin(yaw_) * DT;
    z_ += speed_ * std::cos(yaw_) * DT;
    if (z_ < 0.f) z_ = 0.f;

    for (Thing& th : things_) {
        if (th.kind != ROCK || th.hit) continue;
        if (std::fabs(th.z - z_) > 2.1f || std::fabs(th.x - x_) > 1.55f) continue;
        th.hit = true;
        speed_ *= 0.58f;
        heat_ = std::min(BOIL, heat_ + 6.f);
        shake_ = 6.f;
        x_ += x_ > 0.f ? -0.4f : 0.4f;
        sys_->apu.noiseBurst(0.4f, 110.f, 0.12f);
    }

    bool slow = std::fabs(speed_) < (fillFrames_ > 10 ? 2.8f : 1.2f);
    bool pouring = !watered_ && inBox() && slow;
    if (pouring) {
        if (fillFrames_ < FILL_FRAMES) fillFrames_++;
        if (fillFrames_ >= FILL_FRAMES) {
            watered_ = true;
            heat_ = COOLED;
            sys_->apu.noiseBurst(0.35f, 2400.f, 0.18f);
            sys_->apu.keyOn(1, 523.f, 0.16f);
        }
    } else if (!watered_) {
        int drop = inBox() ? 1 : 3;
        fillFrames_ = std::max(0, fillFrames_ - drop);
        float waste = std::max(0.f, gas_ - WASTE_AT) * WASTE;
        float rate = SUN + THROTTLE_HEAT * gas_ + waste + SAND_HEAT * sand * (0.55f + gas_);
        if (std::fabs(speed_) < 0.8f) rate += 0.25f;
        heat_ += rate * DT;
        if (heat_ > BOIL) heat_ = BOIL;
    } else {
        float waste = std::max(0.f, gas_ - WASTE_AT) * WASTE;
        float rate = SUN + THROTTLE_HEAT * gas_ + waste + SAND_HEAT * sand * (0.55f + gas_);
        if (std::fabs(speed_) < 0.8f) rate += 0.25f;
        heat_ += rate * DT;
        if (heat_ > BOIL) heat_ = BOIL;
    }

    if (speed_ > 7.f && puffs_.size() < 22 && (int(t_ * 60.f) & 3) == 0) {
        for (int side = 0; side < 2; side++) {
            Puff p;
            p.x = 160.f + yaw_ * 30.f + (side ? 34.f : -34.f);
            p.y = 192.f;
            p.vx = (side ? 18.f : -18.f);
            p.vy = -10.f - speed_ * 0.15f;
            p.life = 0.38f;
            p.s = 8.f + sand * 6.f;
            p.steam = false;
            puffs_.push_back(p);
        }
    }
    if (heat_ > 72.f && puffs_.size() < 26 && (int(t_ * 60.f) % 5) == 0) {
        Puff p;
        p.x = 160.f + yaw_ * 20.f;
        p.y = 150.f;
        p.vx = yaw_ * -8.f;
        p.vy = -22.f - (heat_ - 72.f) * 0.15f;
        p.life = 0.5f;
        p.s = 7.f + (heat_ - 72.f) * 0.08f;
        p.steam = true;
        puffs_.push_back(p);
    }
    for (Puff& p : puffs_) {
        p.x += p.vx * DT;
        p.y += p.vy * DT;
        p.life -= DT;
        p.s += 10.f * DT;
    }
    while (!puffs_.empty() && puffs_.front().life <= 0.f) puffs_.erase(puffs_.begin());

    if (heat_ >= BOIL) finish(false, 2);
    else if (z_ >= LENGTH && std::fabs(x_) <= 8.f) finish(watered_, watered_ ? 1 : 3);
}

void Game::buildShift() {
    shiftH_[0] = -yaw_ * 0.55f;
    shiftX_[0] = 0;
    for (int i = 1; i <= SHIFT_N; i++) {
        float zz = z_ + (float(i) - 0.5f) * SHIFT_DZ;
        shiftH_[i] = shiftH_[i - 1] + kappa(zz) * SHIFT_DZ;
        shiftX_[i] = shiftX_[i - 1] + shiftH_[i - 1] * SHIFT_DZ;
    }
}

float Game::shiftAt(float dz) const {
    float u = dz / SHIFT_DZ;
    if (u < 0.f) u = 0.f;
    if (u > float(SHIFT_N - 1)) u = float(SHIFT_N - 1);
    int i = int(u);
    float t = u - float(i);
    return shiftX_[i] * (1.f - t) + shiftX_[i + 1] * t;
}

Game::Proj Game::project(float wz, float wx) const {
    Proj p;
    float dz = wz - z_;
    if (dz < 3.4f || dz > 190.f) return p;
    p.y = float(hor_) + FOCAL * CAM_H / dz;
    if (p.y > gs::SCREEN_H + 40.f) return p;
    p.z = dz;
    p.hw = FOCAL * HALF_W / dz;
    float lat = shiftAt(dz) + wx - x_;
    p.x = 160.f + lat * p.hw / HALF_W + shake_ * (dz < 28.f ? 1.f : 0.25f);
    p.fog = std::clamp((dz - 28.f) / 16.f, 0.f, 10.f);
    p.ok = true;
    return p;
}

void Game::blit(const gs::Mipped& m, float cx, float foot, float h, int pal, bool flip, int fog, bool shade) {
    if (h < 1.6f || m.h <= 0) return;
    const gs::Image& im = m.pick(h);
    float s = h / float(std::max(1, int(im.h)));
    float w = float(im.w) * s;
    gs::Sprite sp;
    sp.x = int(std::lround(cx - w * 0.5f));
    sp.y = int(std::lround(foot - h));
    sp.w = std::max(1, int(std::lround(w)));
    sp.h = std::max(1, int(std::lround(h)));
    sp.img = im;
    sp.pal = uint8_t(pal);
    sp.fog = uint8_t(std::clamp(fog, 0, 16));
    sp.hflip = flip;
    sp.shadow = shade;
    sys_->vdp.sprite(sp);
}

void Game::ui(const gs::Image& im, float x, float y, int pal) {
    if (im.w == 0) return;
    gs::Sprite sp;
    sp.img = im;
    sp.x = int(std::lround(x));
    sp.y = int(std::lround(y));
    sp.w = im.w;
    sp.h = im.h;
    sp.pal = uint8_t(pal);
    sys_->vdp.sprite(sp);
}

void Game::hudText(int col, int row, const char* s, int pal) {
    gs::Plane& h = sys_->vdp.HUD;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        if (x < 0 || x > 39 || row < 0 || row > 27) continue;
        unsigned char ch = (unsigned char)s[i];
        if (ch < 32 || ch > 126) ch = '?';
        int t = art_.font[ch];
        h.set(x, row, t ? gs::entry(t, pal) : 0);
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = int(std::strlen(s));
    hudText(std::max(0, (40 - n) / 2), row, s, pal);
}

void Game::sky() {
    float bob = std::sin(t_ * 1.6f) * 1.1f;
    if (heat_ > 80.f) bob += std::sin(t_ * 22.f) * (heat_ - 80.f) * 0.05f;
    hor_ = HORIZON + int(std::lround(bob));
    if (hor_ < 98) hor_ = 98;
    if (hor_ > 112) hor_ = 112;
    gs::VDP& v = sys_->vdp;
    float hot = std::clamp((heat_ - 55.f) / 45.f, 0.f, 1.f);
    float flash = std::clamp(flash_, 0.f, 1.f);
    int hs = int(std::lround(x_ * 5.f + yaw_ * 28.f));
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.B.hscroll[y] = int16_t(hs);
        v.B.vscroll[y] = 0;
        if (y <= hor_) {
            float u = float(y) / float(std::max(hor_, 1));
            int r = int(5 + u * 10.f + hot * 3.f + flash * 8.f);
            int g = int(7 + u * 4.f);
            int b = int(12 - u * 6.f);
            v.lineBackdrop[y] = gs::rgb4(std::clamp(r, 0, 15), std::clamp(g, 0, 15), std::clamp(b, 0, 15));
            v.lineFog[y] = uint8_t(u * 3.f);
        } else {
            float dz = FOCAL * CAM_H / float(y - hor_);
            int r = 12 + int(hot * 2.f);
            int g = 8;
            int b = 4;
            v.lineBackdrop[y] = gs::rgb4(std::clamp(r, 0, 15), g, b);
            v.lineFog[y] = uint8_t(std::clamp((dz - 30.f) / 18.f, 0.f, 8.f));
        }
    }
    v.A.enabled = false;
    v.B.enabled = true;
}

void Game::road() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        gs::RoadLine& L = v.road[y];
        L.on = false;
        if (y <= hor_) continue;
        float dz = FOCAL * CAM_H / float(y - hor_);
        if (dz > 220.f || dz < 1.3f) continue;
        float hw = FOCAL * HALF_W / dz;
        float wz = z_ + dz;
        bool damp = std::fabs(wz - WATER_Z) <= BOX + 6.f;
        L.on = true;
        L.cx = 160.f + (shiftAt(dz) - x_) * hw / HALF_W + shake_ * (dz < 24.f ? 1.f : 0.2f);
        L.hw = hw * (1.f + 0.045f * std::sin(wz * 0.07f));
        L.v = wz * 10.f;
        L.pal = uint8_t(damp ? PAL_DAMP : PAL_ROAD);
        L.band = (int(std::floor(wz * 0.22f)) & 1) ? 1 : 0;
        L.style = 0;
        L.left = L.right = gs::GROUND_LAND;
    }
}

void Game::world() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();

    if (mode_ == Mode::Title) {
        ui(art_.title, (320 - art_.title.w) * 0.5f, 6, PAL_TITLE);
        ui(art_.sub, (320 - art_.sub.w) * 0.5f, 10.f + art_.title.h, PAL_TITLE);
        ui(art_.tag, (320 - art_.tag.w) * 0.5f, 16.f + art_.title.h + art_.sub.h, PAL_TITLE);
    } else if (mode_ == Mode::Count) {
        int n = modeT_ < 0.4f ? 0 : modeT_ < 0.8f ? 1 : modeT_ < 1.2f ? 2 : 3;
        const gs::Image& im = art_.count[n];
        ui(im, (320 - im.w) * 0.5f, 8, PAL_TITLE);
    } else if (mode_ == Mode::Pause) {
        ui(art_.paused, (320 - art_.paused.w) * 0.5f, 18, PAL_TITLE);
    } else if (mode_ == Mode::Result) {
        const gs::Image& im = why_ == 1 ? art_.camp : why_ == 2 ? art_.boiled : art_.dry;
        ui(im, (320 - im.w) * 0.5f, 16, PAL_TITLE);
    }

    for (const Puff& p : puffs_) {
        if (!p.steam || p.life <= 0.f) continue;
        blit(art_.steam, p.x, p.y, p.s, PAL_DUST, false, int((1.f - p.life / 0.5f) * 5.f));
    }

    float wob = heat_ > 78.f ? std::sin(t_ * 26.f) * (heat_ - 78.f) * 0.05f : 0.f;
    float carX = 160.f + yaw_ * 34.f + wob;
    float foot = 186.f + std::sin(z_ * 0.85f) * 1.2f;
    int frame = yaw_ > 0.08f ? 2 : yaw_ < -0.08f ? 0 : 1;
    blit(art_.buggy[frame], carX, foot, 62.f, PAL_BUGGY, false, 0);
    blit(art_.shadow, carX + yaw_ * 8.f, foot + 4.f, 14.f, PAL_DUST, false, 0, true);

    for (const Puff& p : puffs_) {
        if (p.steam || p.life <= 0.f) continue;
        blit(art_.dust, p.x, p.y, p.s, PAL_DUST, false, int((1.f - p.life / 0.38f) * 6.f));
    }

    for (const Thing& th : things_) {
        if (th.z < z_ + 3.2f) continue;
        if (th.z > z_ + 175.f) break;
        Proj p = project(th.z, th.x);
        if (!p.ok) continue;
        int fog = int(p.fog);
        if (th.kind == BANNER_W || th.kind == BANNER_C) {
            float lift = FOCAL * 1.65f / p.z;
            float h = FOCAL * th.h / p.z;
            const gs::Mipped& im = th.kind == BANNER_W ? art_.bannerWater : art_.bannerCamp;
            blit(im, p.x, p.y - lift, h, PAL_SIGN, false, fog);
            continue;
        }
        const gs::Mipped* im = &art_.rock;
        int pal = PAL_ROCK;
        bool flip = th.x > 0.f;
        if (th.kind == SCRUB) {
            im = &art_.scrub;
            pal = PAL_PALM;
        } else if (th.kind == TANK) {
            im = &art_.tank;
            pal = PAL_TANK;
            flip = false;
        } else if (th.kind == PALM) {
            im = &art_.palm;
            pal = PAL_PALM;
        } else if (th.kind == TENT) {
            im = &art_.tent;
            pal = PAL_TENT;
            flip = false;
        } else if (th.kind == POST) {
            im = &art_.post;
            pal = PAL_ROCK;
        }
        blit(*im, p.x, p.y, FOCAL * th.h / p.z, pal, flip, fog);
    }
}

void Game::hud() {
    gs::Plane& h = sys_->vdp.HUD;
    for (int y = 0; y < 28; y++)
        for (int x = 0; x < 40; x++) h.set(x, y, 0);

    char line[48];
    if (mode_ == Mode::Title) {
        hudC(25, "UP CRUISE    SPACE PUSHES", PAL_INK);
        hudC(26, "ARROWS STEER    X BRAKES", PAL_INK);
        hudC(27, "ENTER ROLLS", PAL_GOOD);
        return;
    }

    int needle = std::clamp(int(std::lround(heat_)), 0, 100);
    int hpal = needle >= 90 ? (((int(t_ * 8.f) & 1) ? PAL_BAD : PAL_HOT)) : needle >= 68 ? PAL_HOT : PAL_GOOD;
    int slots = std::clamp(needle / 10, 0, 10);
    char bar[16];
    bar[0] = '[';
    for (int i = 0; i < 10; i++) bar[1 + i] = i < slots ? '#' : '-';
    bar[11] = ']';
    bar[12] = 0;
    int kmh = int(std::fabs(speed_) * 3.6f + 0.5f);
    std::snprintf(line, sizeof line, "HEAT %3d %s %3d", needle, bar, kmh);
    hudText(1, 0, line, hpal);
    hudText(34, 0, "KM/H", PAL_INK);

    if (!watered_) {
        int dist = std::max(0, int(std::lround(WATER_Z - z_)));
        std::snprintf(line, sizeof line, "WATER %4d M", dist);
        hudText(1, 1, line, PAL_HOT);
        hudText(28, 1, "STOP ONCE", PAL_HOT);
    } else {
        int dist = std::max(0, int(std::lround(LENGTH - z_)));
        std::snprintf(line, sizeof line, "CAMP  %4d M", dist);
        hudText(1, 1, line, PAL_GOOD);
        hudText(28, 1, "TANK TAKEN", PAL_GOOD);
    }

    char lane[] = "LANE [-----*-----]";
    int slot = std::clamp(int(std::lround((x_ / 6.5f) * 5.f)) + 5, 0, 10);
    lane[6 + 5] = '-';
    lane[6 + slot] = '*';
    int lanePal = std::fabs(x_) > FIRM ? PAL_BAD : PAL_INK;
    hudText(11, 26, lane, lanePal);

    const char* hint = "HOLD UP  EASE OFF SPACE";
    int ipal = PAL_INK;
    if (mode_ == Mode::Count) {
        hint = "ONE STOP. THEN THE CAMP.";
        ipal = PAL_HOT;
    } else if (mode_ == Mode::Pause) {
        hint = "ENTER RESUMES";
    } else if (mode_ == Mode::Result) {
        if (why_ == 1) {
            hint = "ENTER RUNS THE STAGE AGAIN";
            ipal = PAL_GOOD;
        } else if (why_ == 2) {
            hint = "BOILED. ENTER TRIES AGAIN";
            ipal = PAL_BAD;
        } else {
            hint = "NO WATER. ENTER TRIES AGAIN";
            ipal = PAL_BAD;
        }
    } else if (!watered_ && fillFrames_ > 0) {
        int pct = std::clamp(fillFrames_ * 100 / FILL_FRAMES, 0, 100);
        std::snprintf(line, sizeof line, "FILLING  %d%%", pct);
        hint = line;
        ipal = PAL_GOOD;
    } else if (!watered_ && z_ > WATER_Z + BOX) {
        hint = "TOO FAR   HOLD DOWN";
        ipal = PAL_BAD;
    } else if (!watered_ && std::fabs(WATER_Z - z_) < 80.f) {
        hint = speed_ > 16.f ? "BRAKE FOR THE WATER" : "STOP IN THE DAMP SAND";
        ipal = PAL_HOT;
    } else if (watered_ && heat_ > 88.f) {
        hint = "NEEDLE HIGH  LIFT OFF";
        ipal = PAL_BAD;
    } else if (std::fabs(x_) > FIRM) {
        hint = x_ > 0.f ? "SOFT SAND  STEER LEFT" : "SOFT SAND  STEER RIGHT";
        ipal = PAL_HOT;
    }
    hudC(27, hint, ipal);
}

void Game::audio() {
    if (toneT_ > 0.f) {
        toneT_ -= DT;
        if (toneT_ <= 0.f) sys_->apu.tone(0, 0, 0);
    }
    if (warnHold_ > 0.f) {
        warnHold_ -= DT;
        if (warnHold_ <= 0.f) sys_->apu.tone(1, 0, 0);
    }
    bool alive = mode_ != Mode::Result || won_;
    if (alive) {
        float bur = 1.f + 0.02f * std::sin(t_ * 40.f);
        float freq = (36.f + std::fabs(speed_) * 2.6f + gas_ * 34.f) * bur;
        if (heat_ > 88.f) freq += 6.f;
        sys_->apu.setFreq(0, freq);
        float vol = 0.045f + gas_ * 0.05f + std::fabs(speed_) * 0.0016f;
        if (mode_ == Mode::Title || mode_ == Mode::Count) vol = 0.04f;
        sys_->apu.setVol(0, vol);
    }
    if (mode_ == Mode::Run && !watered_ && fillFrames_ > 0 && (int(t_ * 60.f) % 10) == 0)
        sys_->apu.noise(0.04f, 1800.f, false);
    else if (fillFrames_ == 0) sys_->apu.noise(0, 0, false);

    if (mode_ == Mode::Run && heat_ > 86.f) {
        warnT_ -= DT;
        if (warnT_ <= 0.f) {
            sys_->apu.tone(1, heat_ > 95.f ? 920.f : 640.f, 0.045f);
            warnHold_ = 0.07f;
            warnT_ = heat_ > 95.f ? 0.16f : 0.4f;
        }
    }
    if (fanStep_ >= 0) {
        static const float notes[] = {392.f, 494.f, 587.f, 784.f};
        fanT_ += DT;
        if (fanT_ > 0.13f) {
            if (fanStep_ < 4) {
                sys_->apu.keyOn(1, notes[fanStep_], 0.18f);
                fanStep_++;
            } else fanStep_ = -1;
            fanT_ = 0;
        }
    }
    if (heat_ >= 95.f) sys_->setLight(255, 40, 16);
    else if (heat_ >= 70.f) sys_->setLight(255, 120, 24);
    else if (watered_) sys_->setLight(40, 140, 255);
    else sys_->setLight(220, 120, 40);
    if (mode_ == Mode::Run && heat_ > 90.f) sys_->rumble(0.15f, 0.35f, 40);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    layout();
    sys.vdp.A.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.apu.setMaster(0.85f);
    sys.apu.setEcho(0.16f, 0.22f, 0.1f);
    sys.apu.setPatch(0, enginePatch());
    sys.apu.setPatch(1, chimePatch());
    std::snprintf(report_, sizeof report_, "S3 DUNE  FAIL  no run");
    if (bot_) {
        begin();
        return;
    }
    mode_ = Mode::Title;
    poseTitle();
    sys.apu.keyOn(0, 42.f, 0.04f);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    t_ += DT;
    flash_ = std::max(0.f, flash_ - DT * 0.7f);
    shake_ *= std::max(0.f, 1.f - 3.2f * DT);

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_MODE)) sys.eject();
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C)) begin();
    } else if (mode_ == Mode::Count) {
        modeT_ += DT;
        int step = int(modeT_ / 0.4f);
        if (step != beep_ && step < 4) {
            beep_ = step;
            sys.apu.tone(0, step == 3 ? 880.f : 440.f + step * 50.f, 0.06f);
            toneT_ = 0.08f;
        }
        if (modeT_ >= 1.6f) {
            mode_ = Mode::Run;
            modeT_ = 0;
            tRun_ = 0;
        }
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Run;
    } else if (mode_ == Mode::Result) {
        modeT_ += DT;
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C))) begin();
    }

    if (mode_ == Mode::Run) {
        In in = bot_ ? botDrive() : human(pad);
        integrate(in);
        tRun_ += DT;
    }

    buildShift();
    sky();
    road();
    world();
    hud();
    audio();
}

}  // namespace dune

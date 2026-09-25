#include "game/drift.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace drift {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float LENGTH = 720.f;
constexpr float K_PEAK = 0.0115f;
constexpr float HALF_W = 6.0f;
constexpr float WALL = 5.05f;
constexpr float G_LO = 0.56f;
constexpr float G_HI = 5.2f;
constexpr float STEER_S = 2.25f;
constexpr float YAW_DAMP = 0.55f;
constexpr float FOCAL = 250.f;
constexpr float CAM_H = 1.38f;
constexpr int HORIZON = 96;
constexpr float SHIFT_DZ = 3.f;
constexpr int SHIFT_N = 100;
constexpr int QUOTA = 5000;
constexpr float SLIP_MIN = 0.17f;

// Road-frame curvature. Positive bends to the right. Windowed so the pass
// eases in and out; the middle lobes are the long slide.
float kappa(float z) {
    constexpr float A = 48.f;
    constexpr float B = LENGTH - 56.f;
    if (z < A || z > B) return 0.f;
    float u = (z - A) / (B - A);
    return K_PEAK * std::sin(u * 6.2831853f * 2.f) * std::sin(u * 3.14159265f);
}

gs::FMPatch enginePatch() {
    gs::FMPatch p;
    p.alg = 4;
    p.fb = 0.42f;
    p.op[0] = {0.5f, 0.8f, 0.04f, 0.35f, 0.7f, 0.25f};
    p.op[1] = {1.f, 0.55f, 0.03f, 0.4f, 0.6f, 0.22f, 2.f};
    p.op[2] = {2.f, 0.22f, 0.02f, 0.3f, 0.35f, 0.2f};
    p.op[3] = {0.25f, 0.35f, 0.06f, 0.5f, 0.7f, 0.28f};
    p.vol = 0.12f;
    p.drive = 0.48f;
    p.tone = 680.f;
    p.vibRate = 5.5f;
    p.vibDepth = 0.012f;
    return p;
}

gs::FMPatch chimePatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.15f;
    p.op[0] = {1.f, 1.f, 0.01f, 0.18f, 0.2f, 0.35f};
    p.op[1] = {2.f, 0.35f, 0.01f, 0.22f, 0.15f, 0.4f};
    p.op[2] = {3.f, 0.22f, 0.01f, 0.2f, 0.1f, 0.35f};
    p.op[3] = {1.f, 0.5f, 0.02f, 0.3f, 0.35f, 0.45f};
    p.vol = 0.18f;
    return p;
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Count) return 1;
    if (mode_ == Mode::Result) return 3;
    return 2;
}

void Game::resetLine() {
    x_ = yaw_ = beta_ = 0;
    steer_ = gas_ = brake_ = 0;
    puffs_.clear();
}

void Game::beginPass() {
    resetLine();
    z_ = 0;
    speed_ = 18.f;
    score_ = 0;
    hold_ = 0;
    chain_ = 1;
    tRun_ = 0;
    walled_ = false;
    won_ = false;
    over_ = false;
    shake_ = 0;
    flash_ = 0;
    mode_ = Mode::Count;
    modeT_ = 0;
    beep_ = -1;
    fanStep_ = -1;
    std::snprintf(report_, sizeof report_, "S3 DRIFT  FAIL  no pass");
}

void Game::botDrive(float& steer, float& gas, float& brake) const {
    float kNow = kappa(z_);
    float kLead = kappa(z_ + speed_ * 0.28f);
    float k = kNow * 0.62f + kLead * 0.38f;
    float kx = 0.05f + 0.14f * std::min(1.f, std::fabs(x_) / WALL);
    float betaRef = std::clamp(-kx * x_ - 0.9f * beta_, -0.42f, 0.42f);
    float yawRef = std::clamp(betaRef + k * speed_ / G_LO, -1.05f, 1.05f);
    float ff = (YAW_DAMP * yaw_ + kNow * speed_) / STEER_S;
    steer = std::clamp(ff + (yawRef - yaw_) * 3.5f, -1.f, 1.f);
    brake = std::fabs(x_) > WALL * 0.78f ? 0.f : 1.f;
    gas = speed_ < 24.f ? 1.f : 0.12f;
}

void Game::humanDrive(const gs::Pad& pad, float& steer, float& gas, float& brake) const {
    steer = 0;
    if (pad.down(gs::BTN_LEFT)) steer -= 1.f;
    if (pad.down(gs::BTN_RIGHT)) steer += 1.f;
    if (std::fabs(pad.axisX) > 0.12f) steer = pad.axisX;
    // Full lock is a shove, not a spin. The slide is held with a small angle.
    steer = std::clamp(steer * 0.62f, -1.f, 1.f);
    gas = (pad.down(gs::BTN_A) || pad.down(gs::BTN_UP) || pad.accel > 0.18f) ? 1.f : 0.f;
    brake = (pad.down(gs::BTN_B) || pad.down(gs::BTN_DOWN) || pad.down(gs::BTN_C) || pad.brake > 0.18f) ? 1.f : 0.f;
}

void Game::integrate(float steer, float gas, float brake) {
    float follow = std::min(1.f, (bot_ || mode_ == Mode::Title ? 16.f : 12.f) * DT);
    steer_ += (steer - steer_) * follow;
    gas_ = gas;
    brake_ = brake;

    float engine = gas_ * 22.f;
    float loose = brake_ * 4.f;
    speed_ += (engine - loose - 0.78f * speed_) * DT;
    speed_ = std::clamp(speed_, 0.f, 32.f);

    float kNow = kappa(z_);
    float G = brake_ > 0.5f ? G_LO : G_HI;
    yaw_ += (steer_ * STEER_S - YAW_DAMP * yaw_ - kNow * speed_) * DT;
    yaw_ = std::clamp(yaw_, -1.15f, 1.15f);
    beta_ += ((yaw_ - beta_) * G - kNow * speed_) * DT;
    beta_ = std::clamp(beta_, -0.95f, 0.95f);

    x_ += speed_ * std::sin(beta_) * DT;
    z_ += speed_ * std::cos(beta_) * DT;
    if (z_ < 0) z_ = 0;

    float slip = std::fabs(yaw_ - beta_);
    bool hot = slip > SLIP_MIN && speed_ > 14.f && mode_ == Mode::Run;
    if (hot) {
        hold_ += DT;
        chain_ = std::min(3.5f, 1.f + hold_ * 0.18f);
        float edge = std::clamp(std::fabs(x_) / WALL, 0.f, 1.f);
        score_ += slip * speed_ * 42.f * chain_ * (1.f + 0.55f * edge) * DT;
    } else if (mode_ == Mode::Run) {
        hold_ = std::max(0.f, hold_ - DT * 0.8f);
        chain_ = std::min(3.5f, 1.f + hold_ * 0.18f);
    }

    if (slip > SLIP_MIN && brake_ > 0.4f && puffs_.size() < 28 && (int(t_ * 60.f) % 2) == 0) {
        float cx = 160.f + yaw_ * 22.f;
        for (int side = 0; side < 2; side++) {
            Puff p;
            p.x = cx + (side ? 28.f : -28.f);
            p.y = 184.f;
            p.vx = (side ? 16.f : -16.f) - yaw_ * 20.f;
            p.vy = -26.f - speed_ * 0.25f;
            p.life = 0.42f;
            p.s = 9.f;
            puffs_.push_back(p);
        }
    }
    for (Puff& p : puffs_) {
        p.x += p.vx * DT;
        p.y += p.vy * DT;
        p.life -= DT;
        p.s += 18.f * DT;
    }
    while (!puffs_.empty() && puffs_.front().life <= 0) puffs_.erase(puffs_.begin());
}

void Game::endPass(bool walled) {
    walled_ = walled;
    won_ = !walled_ && z_ >= LENGTH - 0.05f && score_ >= float(QUOTA);
    mode_ = Mode::Result;
    modeT_ = 0;
    over_ = true;
    flash_ = walled_ ? 0.55f : 0.f;
    shake_ = walled_ ? 8.f : 1.2f;
    int sc = int(score_ + 0.5f);
    if (won_) {
        std::snprintf(report_, sizeof report_, "S3 DRIFT  PASS  slide %d  one pass clean  (%.1f s)", sc, tRun_);
        fanStep_ = 0;
        fanT_ = 0;
    } else if (walled_) {
        std::snprintf(report_, sizeof report_, "S3 DRIFT  FAIL  the wall ends it  slide %d  (%.1f s)", sc, tRun_);
        sys_->apu.noiseBurst(0.72f, 160.f, 0.32f);
        sys_->apu.setVol(0, 0.02f);
    } else {
        std::snprintf(report_, sizeof report_, "S3 DRIFT  FAIL  slide %d  need %d  (%.1f s)", sc, QUOTA, tRun_);
    }
    sys_->rumble(walled_ ? 0.9f : 0.25f, walled_ ? 1.f : 0.45f, walled_ ? 240 : 90);
}

void Game::buildShift() {
    shiftH_[0] = -yaw_ * 0.62f;
    shiftX_[0] = 0;
    for (int i = 1; i <= SHIFT_N; i++) {
        float zz = z_ + (float(i) - 0.5f) * SHIFT_DZ;
        shiftH_[i] = shiftH_[i - 1] + kappa(zz) * SHIFT_DZ;
        shiftX_[i] = shiftX_[i - 1] + shiftH_[i - 1] * SHIFT_DZ;
    }
}

float Game::shiftAt(float dz) const {
    float u = dz / SHIFT_DZ;
    if (u < 0) u = 0;
    if (u > float(SHIFT_N - 1)) u = float(SHIFT_N - 1);
    int i = int(u);
    float t = u - float(i);
    return shiftX_[i] * (1.f - t) + shiftX_[i + 1] * t;
}

Game::Proj Game::project(float wz, float wx) const {
    Proj p;
    float dz = wz - z_;
    if (dz < 3.2f || dz > 185.f) return p;
    p.y = float(hor_) + FOCAL * CAM_H / dz;
    if (p.y > gs::SCREEN_H + 30.f) return p;
    p.z = dz;
    p.hw = FOCAL * HALF_W / dz;
    float lat = shiftAt(dz) + wx - x_;
    p.x = 160.f + lat * p.hw / HALF_W + shake_ * (dz < 30.f ? 1.f : 0.3f);
    p.fog = std::clamp((dz - 22.f) / 12.f, 0.f, 13.f);
    p.ok = true;
    return p;
}

void Game::blit(const gs::Mipped& m, float cx, float foot, float h, int pal, bool flip, int fog, bool shade) {
    if (h < 1.5f) return;
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

void Game::sky() {
    int bob = int(std::sin(t_ * 1.7f) * 1.2f);
    hor_ = HORIZON + bob;
    if (hor_ < 90) hor_ = 90;
    if (hor_ > 104) hor_ = 104;
    gs::VDP& v = sys_->vdp;
    float flash = std::clamp(flash_, 0.f, 1.f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = std::clamp(float(y) / float(std::max(hor_, 1)), 0.f, 1.f);
        int r = int(1 + 7 * u * u);
        int g = int(1 + 2 * u);
        int b = int(3 + 2 * (1.f - u));
        if (y > hor_) {
            r = 2;
            g = 2;
            b = 3;
        }
        r = std::clamp(r + int(flash * 10), 0, 15);
        g = std::clamp(g, 0, 15);
        b = std::clamp(b, 0, 15);
        v.lineBackdrop[y] = gs::rgb4(r, g, b);
        if (y <= hor_) v.lineFog[y] = uint8_t((1.f - u) * 5.f);
        else {
            float dz = FOCAL * CAM_H / float(std::max(1, y - hor_));
            v.lineFog[y] = uint8_t(std::clamp(dz / 16.f, 0.f, 14.f));
        }
        float depth = y < hor_ ? 0.25f + 0.75f * float(y) / float(std::max(hor_, 1)) : 1.f;
        v.B.hscroll[y] = int16_t(shiftH_[std::min(SHIFT_N, 14)] * 70.f * depth + shake_);
        v.B.vscroll[y] = 0;
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
        if (dz > 190.f || dz < 1.4f) continue;
        float hw = FOCAL * HALF_W / dz;
        float z = z_ + dz;
        L.on = true;
        L.cx = 160.f + (shiftAt(dz) - x_) * hw / HALF_W + shake_ * (dz < 24.f ? 1.f : 0.25f);
        L.hw = hw;
        L.v = z * 18.f;
        L.pal = PAL_ROAD;
        L.band = (int(std::floor(z * 0.22f)) & 1) ? 1 : 0;
        L.style = 1;
        L.left = L.right = gs::GROUND_LAND;
    }
}

void Game::world() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();

    if (mode_ == Mode::Title) {
        ui(art_.title, (320 - art_.title.w) * 0.5f, 4, PAL_TITLE);
        ui(art_.sub, (320 - art_.sub.w) * 0.5f, 8.f + art_.title.h, PAL_TITLE);
        ui(art_.tag, (320 - art_.tag.w) * 0.5f, 12.f + art_.title.h + art_.sub.h, PAL_TITLE);
    } else if (mode_ == Mode::Count) {
        int n = modeT_ < 0.4f ? 0 : modeT_ < 0.8f ? 1 : modeT_ < 1.2f ? 2 : 3;
        const gs::Image& im = art_.count[n];
        ui(im, (320 - im.w) * 0.5f, 6, PAL_TITLE);
    } else if (mode_ == Mode::Pause) {
        ui(art_.paused, (320 - art_.paused.w) * 0.5f, 28, PAL_TITLE);
    } else if (mode_ == Mode::Result) {
        const gs::Image& im = walled_ ? art_.walled : won_ ? art_.scored : art_.shortSlide;
        ui(im, (320 - im.w) * 0.5f, 26, PAL_TITLE);
    }

    // Earlier sprites win the pixel, so the car is submitted before the road props.
    float slip = std::fabs(yaw_ - beta_);
    float carX = 160.f + yaw_ * 36.f + std::sin(t_ * 31.f) * slip * 2.f;
    float foot = 174.f + std::sin(z_ * 0.35f) * 1.1f;
    int frame = std::clamp(int(std::lround(yaw_ * 6.5f)) + 3, 0, 6);
    blit(art_.car[frame], carX, foot, 64.f, PAL_CAR, false, 0);
    blit(art_.shadow, carX + yaw_ * 6.f, foot + 6.f, 16.f, PAL_SMOKE, false, 0, true);
    for (const Puff& p : puffs_) {
        if (p.life <= 0) continue;
        blit(art_.smoke, p.x, p.y, p.s, PAL_SMOKE, false, int((1.f - p.life / 0.42f) * 6.f));
    }

    auto gantry = [&](const gs::Mipped& img, float zGate) {
        Proj p = project(zGate, 0);
        if (!p.ok) return;
        float wantW = p.hw * 1.65f;
        float h = wantW * float(img.h) / float(std::max(1, img.w));
        float bottom = p.y - FOCAL * 1.6f / p.z;
        blit(img, p.x, bottom, h, PAL_BANNER, false, int(p.fog));
        float pole = FOCAL * 3.3f / p.z;
        blit(art_.lamp, p.x - p.hw * 0.92f, p.y, pole, PAL_LAMP, false, int(p.fog));
        blit(art_.lamp, p.x + p.hw * 0.92f, p.y, pole, PAL_LAMP, true, int(p.fog));
    };
    gantry(art_.bannerSlide, 46.f);
    gantry(art_.bannerFinish, LENGTH - 10.f);

    float near = std::ceil((z_ + 5.f) / 3.6f) * 3.6f;
    for (float z = near; z < z_ + 110.f; z += 3.6f) {
        float h = 1.85f;
        Proj left = project(z, -(HALF_W + 0.15f));
        Proj right = project(z, HALF_W + 0.15f);
        if (left.ok) blit(art_.barrier, left.x, left.y, FOCAL * h / left.z, PAL_WALL, false, int(left.fog));
        if (right.ok) blit(art_.barrier, right.x, right.y, FOCAL * h / right.z, PAL_WALL, true, int(right.fog));
    }
    float lampZ = std::ceil((z_ + 8.f) / 24.f) * 24.f;
    for (float z = lampZ; z < z_ + 160.f; z += 24.f) {
        Proj left = project(z, -(HALF_W + 1.55f));
        Proj right = project(z, HALF_W + 1.55f);
        if (left.ok) blit(art_.lamp, left.x, left.y, FOCAL * 3.4f / left.z, PAL_LAMP, false, int(left.fog));
        if (right.ok) blit(art_.lamp, right.x, right.y, FOCAL * 3.4f / right.z, PAL_LAMP, true, int(right.fog));
    }
}

void Game::hud() {
    gs::Plane& h = sys_->vdp.HUD;
    for (int y = 0; y < 28; y++)
        for (int x = 0; x < 40; x++) h.set(x, y, 0);

    char line[48];
    if (mode_ == Mode::Title) {
        std::snprintf(line, sizeof line, "NEED %d   WALL ENDS IT", QUOTA);
        hudText(5, 25, line, PAL_HOT);
        hudText(1, 26, "ARROWS STEER   Z GAS   X SLIDE", PAL_INK);
        hudText(6, 27, "ENTER STARTS ONE PASS", PAL_GOOD);
        return;
    }

    int sc = int(score_ + 0.5f);
    int pal = sc >= QUOTA ? PAL_GOOD : PAL_HOT;
    std::snprintf(line, sizeof line, "SLIDE %5d/%d", sc, QUOTA);
    hudText(1, 0, line, pal);
    std::snprintf(line, sizeof line, "x%.1f", chain_);
    hudText(22, 0, line, hold_ > 0.4f ? PAL_HOT : PAL_INK);
    int kmh = int(speed_ * 3.6f + 0.5f);
    std::snprintf(line, sizeof line, "%3d KM/H", kmh);
    hudText(30, 0, line, PAL_INK);

    int travelled = int(std::clamp(z_, 0.f, LENGTH));
    std::snprintf(line, sizeof line, "PASS %3d/%d M", travelled, int(LENGTH));
    hudText(1, 1, line, PAL_INK);
    float slip = std::fabs(yaw_ - beta_);
    std::snprintf(line, sizeof line, "%2d DEG", int(slip * 57.3f + 0.5f));
    hudText(30, 1, line, slip > SLIP_MIN ? PAL_HOT : PAL_INK);

    char lane[] = "POS (---------------)";
    int slot = std::clamp(int(std::lround((x_ / WALL) * 7.f)) + 7, 0, 14);
    lane[5 + slot] = '*';
    int lanePal = std::fabs(x_) > WALL * 0.72f ? PAL_BAD : PAL_INK;
    hudText(8, 26, lane, lanePal);

    if (mode_ == Mode::Count) {
        hudText(8, 27, "ONE PASS. HOLD THE SLIDE.", PAL_HOT);
    } else if (mode_ == Mode::Run) {
        const char* hint = "X/DOWN HOLDS THE SLIDE";
        if (std::fabs(x_) > WALL * 0.62f && x_ * beta_ > 0) hint = x_ > 0 ? "STEER LEFT" : "STEER RIGHT";
        else if (brake_ > 0.5f && slip > SLIP_MIN) hint = "SLIDING";
        else if (brake_ < 0.5f) hint = "X/DOWN TO KICK THE TAIL";
        hudText(6, 27, hint, hint[0] == 'S' && hint[1] == 'T' ? PAL_BAD : PAL_INK);
    } else if (mode_ == Mode::Pause) {
        hudText(10, 27, "ENTER RESUMES", PAL_INK);
    } else if (walled_) {
        hudText(6, 27, "ENTER TRIES ONE MORE PASS", PAL_BAD);
    } else if (won_) {
        hudText(7, 27, "CLEAN. ENTER RUNS IT AGAIN", PAL_GOOD);
    } else {
        hudText(5, 27, "SHORT. ENTER RUNS IT AGAIN", PAL_HOT);
    }
}

void Game::audio() {
    if (toneT_ > 0) {
        toneT_ -= DT;
        if (toneT_ <= 0) sys_->apu.tone(0, 0, 0);
    }
    bool rolling = mode_ == Mode::Run || mode_ == Mode::Title || mode_ == Mode::Count;
    if (rolling && !walled_) {
        float bur = 1.f + 0.03f * std::sin(t_ * 46.f);
        float freq = (42.f + speed_ * 3.4f + (gas_ > 0.5f ? 10.f : 0.f) + (brake_ > 0.5f ? 6.f : 0.f)) * bur;
        sys_->apu.setFreq(0, freq);
        sys_->apu.setVol(0, mode_ == Mode::Count ? 0.05f : 0.07f + speed_ * 0.003f);
    }
    if (fanStep_ >= 0) {
        static const float notes[] = {392.f, 494.f, 587.f, 784.f};
        fanT_ += DT;
        if (fanT_ > 0.12f) {
            if (fanStep_ < 4) sys_->apu.keyOn(1, notes[fanStep_], 0.2f);
            else {
                sys_->apu.keyOff(1);
                fanStep_ = -1;
            }
            fanStep_++;
            fanT_ = 0;
        }
    }
    if (mode_ == Mode::Run && brake_ > 0.5f && (int(t_ * 60.f) % 8) == 0) {
        float slip = std::fabs(yaw_ - beta_);
        if (slip > SLIP_MIN) sys_->rumble(0.04f, 0.1f + slip * 0.15f, 50);
    }
    if (walled_) sys_->setLight(255, 24, 18);
    else if (brake_ > 0.5f && mode_ == Mode::Run) sys_->setLight(255, 96, 24);
    else sys_->setLight(170, 36, 28);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.apu.setMaster(0.9f);
    sys.apu.setPatch(0, enginePatch());
    sys.apu.setPatch(1, chimePatch());
    sys.apu.keyOn(0, 52.f, 0.05f);
    std::snprintf(report_, sizeof report_, "S3 DRIFT  FAIL  no pass");
    if (bot_) {
        beginPass();
        return;
    }
    mode_ = Mode::Title;
    resetLine();
    z_ = 280.f;
    speed_ = 23.f;
    float k = kappa(z_);
    yaw_ = k * speed_ / G_LO;
    beta_ = 0;
    brake_ = 1.f;
    gas_ = 1.f;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    t_ += DT;
    flash_ = std::max(0.f, flash_ - DT * 0.8f);
    shake_ *= std::max(0.f, 1.f - 3.5f * DT);

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_MODE)) sys.eject();
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) beginPass();
    } else if (mode_ == Mode::Count) {
        modeT_ += DT;
        int step = int(modeT_ / 0.4f);
        if (step != beep_ && step < 4) {
            beep_ = step;
            sys.apu.tone(0, step == 3 ? 880.f : 480.f + step * 40.f, 0.07f);
            toneT_ = 0.08f;
        }
        if (modeT_ >= 1.55f) {
            mode_ = Mode::Run;
            modeT_ = 0;
            tRun_ = 0;
        }
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            held_ = Mode::Run;
            mode_ = Mode::Pause;
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = held_;
    } else if (mode_ == Mode::Result) {
        modeT_ += DT;
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) beginPass();
    }

    if (mode_ == Mode::Title) {
        float steer, gas, brake;
        botDrive(steer, gas, brake);
        integrate(steer, gas, brake);
        if (std::fabs(x_) > WALL || z_ >= LENGTH) {
            resetLine();
            z_ = 40.f;
            speed_ = 22.f;
            score_ = 0;
        }
    } else if (mode_ == Mode::Run) {
        float steer, gas, brake;
        if (bot_) botDrive(steer, gas, brake);
        else humanDrive(pad, steer, gas, brake);
        integrate(steer, gas, brake);
        tRun_ += DT;
        if (std::fabs(x_) > WALL) endPass(true);
        else if (z_ >= LENGTH) endPass(false);
    }

    buildShift();
    sky();
    road();
    world();
    hud();
    audio();
}

}  // namespace drift

#include "game/luge.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace luge {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float LENGTH = 720.f;
constexpr float CLOCK0 = 34.f;
constexpr float HALF_W = 5.4f;
constexpr float WALL = 4.55f;
constexpr float STEER_S = 2.15f;
constexpr float YAW_DAMP = 2.6f;
constexpr float FOCAL = 270.f;
constexpr float CAM_H = 0.92f;
constexpr int HORIZON = 108;
constexpr float SHIFT_DZ = 2.6f;
constexpr int SHIFT_N = 120;

float kappa(float z) {
    constexpr float A = 36.f;
    constexpr float B = LENGTH - 48.f;
    if (z < A || z > B) return 0.f;
    float u = (z - A) / (B - A);
    return 0.0054f * std::sin(u * 6.2831853f * 1.5f) * std::sin(u * 3.14159265f);
}

gs::FMPatch windPatch() {
    gs::FMPatch p;
    p.alg = 4;
    p.fb = 0.2f;
    p.op[0] = {0.5f, 0.4f, 0.08f, 0.5f, 0.7f, 0.3f};
    p.op[1] = {1.f, 0.25f, 0.1f, 0.4f, 0.55f, 0.25f};
    p.op[2] = {2.01f, 0.15f, 0.12f, 0.4f, 0.4f, 0.3f};
    p.op[3] = {0.25f, 0.2f, 0.2f, 0.5f, 0.6f, 0.3f};
    p.vol = 0.08f;
    p.tone = 900.f;
    return p;
}

gs::FMPatch chimePatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.12f;
    p.op[0] = {1.f, 1.f, 0.01f, 0.16f, 0.2f, 0.3f};
    p.op[1] = {2.f, 0.3f, 0.01f, 0.2f, 0.12f, 0.35f};
    p.op[2] = {3.f, 0.18f, 0.01f, 0.18f, 0.08f, 0.3f};
    p.op[3] = {1.f, 0.4f, 0.02f, 0.28f, 0.3f, 0.4f};
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

void Game::resetRun() {
    x_ = yaw_ = 0;
    steer_ = tuck_ = scrub_ = 0;
    spray_.clear();
    rocks_ = {{96.f, -2.1f, false},  {168.f, 2.3f, false}, {248.f, -1.6f, false},
              {330.f, 2.0f, false}, {420.f, -2.4f, false}, {510.f, 1.7f, false},
              {600.f, -1.9f, false}};
}

void Game::beginRun() {
    resetRun();
    z_ = 0;
    speed_ = 24.f;
    clock_ = CLOCK0;
    won_ = false;
    over_ = false;
    why_ = 0;
    shake_ = 0;
    flash_ = 0;
    mode_ = Mode::Count;
    modeT_ = 0;
    beep_ = -1;
    fanStep_ = -1;
    tRun_ = 0;
    std::snprintf(report_, sizeof report_, "S3 LUGE PASS  FAIL  the storm still holds the pass");
}

void Game::botDrive(float& steer, float& tuck, float& scrub) const {
    float look = speed_ * 0.42f;
    float k = kappa(z_) * 0.35f + kappa(z_ + look) * 0.65f;
    float target = 0.f;
    for (const Rock& r : rocks_) {
        float dz = r.z - z_;
        if (dz < 4.f || dz > look + 8.f || r.hit) continue;
        if (std::fabs(x_ - r.x) < 2.35f) target = r.x > 0.f ? -2.15f : 2.15f;
    }
    float ff = (YAW_DAMP * yaw_ + k * speed_) / STEER_S;
    steer = std::clamp(ff + (target - x_) * 0.72f - yaw_ * 1.15f, -1.f, 1.f);
    tuck = speed_ < 31.f ? 1.f : 0.35f;
    scrub = std::fabs(x_) > WALL * 0.82f ? 1.f : 0.f;
}

void Game::humanDrive(const gs::Pad& pad, float& steer, float& tuck, float& scrub) const {
    steer = 0;
    if (pad.down(gs::BTN_LEFT)) steer -= 1.f;
    if (pad.down(gs::BTN_RIGHT)) steer += 1.f;
    if (std::fabs(pad.axisX) > 0.15f) steer = pad.axisX;
    steer = std::clamp(steer, -1.f, 1.f);
    tuck = (pad.down(gs::BTN_A) || pad.down(gs::BTN_UP) || pad.accel > 0.2f) ? 1.f : 0.f;
    scrub = (pad.down(gs::BTN_B) || pad.down(gs::BTN_DOWN) || pad.brake > 0.2f) ? 1.f : 0.f;
}

void Game::integrate(float steer, float tuck, float scrub) {
    float follow = std::min(1.f, 14.f * DT);
    steer_ += (steer - steer_) * follow;
    tuck_ = tuck;
    scrub_ = scrub;

    float push = 7.5f + tuck_ * 6.f - scrub_ * 14.f;
    speed_ += (push - 0.22f * speed_) * DT;
    speed_ = std::clamp(speed_, 14.f, 36.f);

    float kNow = kappa(z_);
    yaw_ += (steer_ * STEER_S - YAW_DAMP * yaw_ - kNow * speed_) * DT;
    yaw_ = std::clamp(yaw_, -0.85f, 0.85f);
    x_ += speed_ * std::sin(yaw_) * DT;
    z_ += speed_ * std::cos(yaw_) * DT;
    if (z_ < 0) z_ = 0;

    if (mode_ == Mode::Run && (int(t_ * 60.f) % 3) == 0 && spray_.size() < 16) {
        Spray s;
        s.x = 160.f + yaw_ * 18.f + (spray_.size() & 1 ? 10.f : -10.f);
        s.y = 188.f;
        s.vx = (spray_.size() & 1 ? 22.f : -22.f);
        s.vy = -8.f;
        s.life = 0.28f;
        s.s = 8.f;
        spray_.push_back(s);
    }
    for (Spray& s : spray_) {
        s.x += s.vx * DT;
        s.y += s.vy * DT;
        s.life -= DT;
        s.s += 10.f * DT;
    }
    while (!spray_.empty() && spray_.front().life <= 0) spray_.erase(spray_.begin());

    if (mode_ != Mode::Run) return;
    for (Rock& r : rocks_) {
        if (r.hit) continue;
        if (std::fabs(z_ - r.z) < 1.5f && std::fabs(x_ - r.x) < 0.78f) {
            r.hit = true;
            clock_ = std::max(0.f, clock_ - 2.4f);
            speed_ *= 0.72f;
            yaw_ += x_ < r.x ? -0.35f : 0.35f;
            shake_ = 6.f;
            flash_ = 0.35f;
            sys_->apu.noiseBurst(0.4f, 220.f, 0.16f);
        }
    }
}

void Game::endRun(int why) {
    why_ = why;
    won_ = why == 1;
    mode_ = Mode::Result;
    modeT_ = 0;
    over_ = true;
    if (why == 1) {
        std::snprintf(report_, sizeof report_,
                      "S3 LUGE PASS  PASS  cleared the pass  storm %.1f s left  (%.1f s)", clock_, tRun_);
        fanStep_ = 0;
        fanT_ = 0;
        flash_ = 0.15f;
    } else if (why == 2) {
        std::snprintf(report_, sizeof report_,
                      "S3 LUGE PASS  FAIL  the wall took the sled  (%.1f s)", tRun_);
        shake_ = 9.f;
        flash_ = 0.6f;
        sys_->apu.noiseBurst(0.7f, 140.f, 0.3f);
    } else {
        std::snprintf(report_, sizeof report_,
                      "S3 LUGE PASS  FAIL  the storm closed the pass at %d m  (%.1f s)", int(z_), tRun_);
        flash_ = 0.4f;
    }
    sys_->rumble(won_ ? 0.2f : 0.85f, won_ ? 0.4f : 1.f, won_ ? 80 : 220);
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
    if (u < 0) u = 0;
    if (u > float(SHIFT_N - 1)) u = float(SHIFT_N - 1);
    int i = int(u);
    float t = u - float(i);
    return shiftX_[i] * (1.f - t) + shiftX_[i + 1] * t;
}

Game::Proj Game::project(float wz, float wx) const {
    Proj p;
    float dz = wz - z_;
    if (dz < 2.8f || dz > 170.f) return p;
    p.y = float(hor_) + FOCAL * CAM_H / dz;
    if (p.y > gs::SCREEN_H + 40.f) return p;
    p.z = dz;
    p.hw = FOCAL * HALF_W / dz;
    float lat = shiftAt(dz) + wx - x_;
    p.x = 160.f + lat * p.hw / HALF_W + shake_ * (dz < 26.f ? 1.f : 0.25f);
    p.fog = std::clamp((dz - 18.f) / 11.f, 0.f, 14.f);
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
    float storm = 0.f;
    if (mode_ == Mode::Run || mode_ == Mode::Result) storm = 1.f - std::clamp(clock_ / CLOCK0, 0.f, 1.f);
    int bob = int(std::sin(t_ * 1.4f) * 1.0f);
    hor_ = HORIZON + bob;
    gs::VDP& v = sys_->vdp;
    float flash = std::clamp(flash_, 0.f, 1.f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = std::clamp(float(y) / float(std::max(hor_, 1)), 0.f, 1.f);
        int r = int(4 + 4 * u);
        int g = int(5 + 4 * u);
        int b = int(8 + 3 * u);
        r = int(r * (1.f - storm * 0.55f) + 3 * storm);
        g = int(g * (1.f - storm * 0.5f) + 4 * storm);
        b = int(b * (1.f - storm * 0.35f) + 6 * storm);
        if (y > hor_) {
            r = 6;
            g = 7;
            b = 9;
        }
        r = std::clamp(r + int(flash * 8), 0, 15);
        g = std::clamp(g + int(flash * 8), 0, 15);
        b = std::clamp(b + int(flash * 6), 0, 15);
        v.lineBackdrop[y] = gs::rgb4(r, g, b);
        v.lineFog[y] = y <= hor_ ? uint8_t((1.f - u) * 4.f + storm * 4.f) : uint8_t(std::min(14.f, 2.f + storm * 3.f));
        v.B.hscroll[y] = int16_t(-yaw_ * 40.f + shake_);
        v.B.vscroll[y] = 0;
    }
    v.A.enabled = false;
    v.B.enabled = true;
}

void Game::road() {
    gs::VDP& v = sys_->vdp;
    v.roadTime = int(t_ * 60.f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        gs::RoadLine& L = v.road[y];
        L.on = false;
        if (y <= hor_) continue;
        float dz = FOCAL * CAM_H / float(y - hor_);
        if (dz > 180.f || dz < 1.2f) continue;
        float hw = FOCAL * HALF_W / dz;
        L.on = true;
        L.cx = 160.f + (shiftAt(dz) - x_) * hw / HALF_W + shake_ * (dz < 20.f ? 1.f : 0.2f);
        L.hw = hw;
        L.v = (z_ + dz) * 16.f;
        L.pal = PAL_ROAD;
        L.band = (int(std::floor((z_ + dz) * 0.18f)) & 1) ? 1 : 0;
        L.style = gs::ROAD_ICE;
        L.left = L.right = gs::GROUND_SNOWWALL;
    }
}

void Game::world() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();

    if (mode_ == Mode::Title) {
        ui(art_.title, (320 - art_.title.w) * 0.5f, 8, PAL_TITLE);
        ui(art_.sub, (320 - art_.sub.w) * 0.5f, 16.f + art_.title.h, PAL_TITLE);
    } else if (mode_ == Mode::Count) {
        int n = modeT_ < 0.35f ? 0 : modeT_ < 0.7f ? 1 : modeT_ < 1.05f ? 2 : 3;
        const gs::Image& im = art_.count[n];
        ui(im, (320 - im.w) * 0.5f, 10, PAL_TITLE);
    } else if (mode_ == Mode::Pause) {
        ui(art_.paused, (320 - art_.paused.w) * 0.5f, 28, PAL_TITLE);
    } else if (mode_ == Mode::Result) {
        const gs::Image& im = why_ == 1 ? art_.cleared : why_ == 2 ? art_.buried : art_.late;
        ui(im, (320 - im.w) * 0.5f, 22, PAL_TITLE);
    }

    int frame = std::clamp(int(std::lround(yaw_ * 5.f)) + 2, 0, 4);
    float foot = 186.f;
    float carX = 160.f + yaw_ * 28.f;
    blit(art_.rider[frame], carX, foot, 58.f, PAL_RIDER, false, 0);
    blit(art_.shadow, carX, foot + 4.f, 12.f, PAL_SPRAY, false, 0, true);
    for (const Spray& s : spray_) {
        if (s.life <= 0) continue;
        blit(art_.spray, s.x, s.y, s.s, PAL_SPRAY, false, int((1.f - s.life / 0.28f) * 8.f));
    }

    auto gate = [&](const gs::Mipped& img, float zGate) {
        Proj p = project(zGate, 0);
        if (!p.ok) return;
        float h = p.hw * 0.55f;
        blit(img, p.x, p.y - FOCAL * 2.2f / p.z, h, PAL_BANNER, false, int(p.fog));
    };
    gate(art_.bannerOpen, 28.f);
    gate(art_.bannerShut, LENGTH - 12.f);

    for (const Rock& r : rocks_) {
        Proj p = project(r.z, r.x);
        if (!p.ok) continue;
        blit(art_.rock, p.x, p.y, FOCAL * 0.7f / p.z, PAL_ROCK, false, int(p.fog));
    }

    float pineZ = std::ceil((z_ + 6.f) / 14.f) * 14.f;
    for (float z = pineZ; z < z_ + 140.f; z += 14.f) {
        Proj left = project(z, -(HALF_W + 1.7f));
        Proj right = project(z, HALF_W + 1.7f);
        float side = (int(z) / 14) & 1 ? 0.4f : 0.f;
        if (left.ok) blit(art_.pine, left.x, left.y, FOCAL * (2.6f + side) / left.z, PAL_PINE, false, int(left.fog));
        if (right.ok) blit(art_.pine, right.x, right.y, FOCAL * (2.4f + side) / right.z, PAL_PINE, true, int(right.fog));
    }
}

void Game::hud() {
    gs::Plane& h = sys_->vdp.HUD;
    for (int y = 0; y < 28; y++)
        for (int x = 0; x < 40; x++) h.set(x, y, 0);

    char line[48];
    if (mode_ == Mode::Title) {
        hudText(4, 25, "CLEAR THE PASS BEFORE THE STORM", PAL_HOT);
        hudText(2, 26, "ARROWS LEAN   Z TUCK   X SCRUB", PAL_INK);
        hudText(8, 27, "ENTER DROPS IN", PAL_GOOD);
        return;
    }

    int cs = std::max(0, int(std::ceil(clock_)));
    int pal = cs < 8 ? PAL_BAD : cs < 16 ? PAL_HOT : PAL_GOOD;
    std::snprintf(line, sizeof line, "STORM %02d", cs);
    hudText(1, 0, line, pal);
    std::snprintf(line, sizeof line, "%3d M/S", int(speed_ + 0.5f));
    hudText(31, 0, line, PAL_INK);
    int travelled = int(std::clamp(z_, 0.f, LENGTH));
    std::snprintf(line, sizeof line, "PASS %3d/%d", travelled, int(LENGTH));
    hudText(1, 1, line, PAL_INK);

    char lane[] = "ICE (---------------)";
    int slot = std::clamp(int(std::lround((x_ / WALL) * 7.f)) + 7, 0, 14);
    lane[5 + slot] = '*';
    hudText(9, 26, lane, std::fabs(x_) > WALL * 0.75f ? PAL_BAD : PAL_INK);

    if (mode_ == Mode::Count) hudText(8, 27, "THE CHUTE IS OPEN", PAL_HOT);
    else if (mode_ == Mode::Pause) hudText(11, 27, "ENTER RESUMES", PAL_INK);
    else if (mode_ == Mode::Run) {
        const char* hint = tuck_ > 0.5f ? "TUCKED" : "Z TUCKS FOR SPEED";
        if (std::fabs(x_) > WALL * 0.62f) hint = x_ > 0 ? "LEAN LEFT" : "LEAN RIGHT";
        hudText(11, 27, hint, hint[0] == 'L' ? PAL_BAD : PAL_INK);
    } else if (won_) {
        hudText(6, 27, "CLEAR. ENTER RUNS IT AGAIN", PAL_GOOD);
    } else {
        hudText(6, 27, "BURIED. ENTER TRIES AGAIN", PAL_BAD);
    }
}

void Game::audio() {
    if (toneT_ > 0) {
        toneT_ -= DT;
        if (toneT_ <= 0) sys_->apu.tone(0, 0, 0);
    }
    bool rolling = mode_ == Mode::Run || mode_ == Mode::Title || mode_ == Mode::Count;
    if (rolling && why_ == 0) {
        float freq = 70.f + speed_ * 4.2f + tuck_ * 12.f;
        sys_->apu.setFreq(0, freq);
        sys_->apu.setVol(0, 0.06f + speed_ * 0.002f);
        sys_->apu.noise(0.03f + speed_ * 0.0015f, 400.f + speed_ * 30.f, false);
    } else {
        sys_->apu.setVol(0, 0.02f);
        sys_->apu.noise(0, 0, false);
    }
    if (fanStep_ >= 0) {
        static const float notes[] = {523.f, 659.f, 784.f, 1046.f};
        fanT_ += DT;
        if (fanT_ > 0.11f) {
            if (fanStep_ < 4) sys_->apu.keyOn(1, notes[fanStep_], 0.22f);
            else {
                sys_->apu.keyOff(1);
                fanStep_ = -1;
            }
            fanStep_++;
            fanT_ = 0;
        }
    }
    if (won_) sys_->setLight(80, 180, 220);
    else if (clock_ < 8.f && mode_ == Mode::Run) sys_->setLight(40, 60, 120);
    else sys_->setLight(140, 170, 200);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.apu.setMaster(0.85f);
    sys.apu.setPatch(0, windPatch());
    sys.apu.setPatch(1, chimePatch());
    sys.apu.keyOn(0, 90.f, 0.05f);
    std::snprintf(report_, sizeof report_, "S3 LUGE PASS  FAIL  the storm still holds the pass");
    if (bot_) {
        beginRun();
        return;
    }
    mode_ = Mode::Title;
    resetRun();
    z_ = 80.f;
    speed_ = 28.f;
    clock_ = CLOCK0;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    t_ += DT;
    flash_ = std::max(0.f, flash_ - DT);
    shake_ *= std::max(0.f, 1.f - 4.f * DT);

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_MODE)) sys.eject();
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) beginRun();
    } else if (mode_ == Mode::Count) {
        modeT_ += DT;
        int step = int(modeT_ / 0.35f);
        if (step != beep_ && step < 4) {
            beep_ = step;
            sys.apu.tone(0, step == 3 ? 880.f : 440.f + step * 60.f, 0.08f);
            toneT_ = 0.07f;
        }
        if (modeT_ >= 1.35f) {
            mode_ = Mode::Run;
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
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) beginRun();
    }

    if (mode_ == Mode::Title) {
        float steer, tuck, scrub;
        botDrive(steer, tuck, scrub);
        integrate(steer, tuck, scrub);
        if (std::fabs(x_) > WALL || z_ > 260.f) {
            resetRun();
            z_ = 40.f;
            speed_ = 28.f;
        }
    } else if (mode_ == Mode::Run) {
        float steer, tuck, scrub;
        if (bot_) botDrive(steer, tuck, scrub);
        else humanDrive(pad, steer, tuck, scrub);
        integrate(steer, tuck, scrub);
        tRun_ += DT;
        clock_ -= DT;
        if (std::fabs(x_) > WALL) endRun(2);
        else if (z_ >= LENGTH) endRun(1);
        else if (clock_ <= 0.f) endRun(3);
    } else if (mode_ == Mode::Count) {
        float steer, tuck, scrub;
        botDrive(steer, tuck, scrub);
        integrate(steer, tuck * 0.2f, 0);
    }

    buildShift();
    sky();
    road();
    world();
    hud();
    audio();
}

}  // namespace luge

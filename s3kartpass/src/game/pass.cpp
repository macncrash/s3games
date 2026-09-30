#include "game/pass.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace kartpass {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float LENGTH = 520.f;
constexpr float CLOCK0 = 30.f;
constexpr float HALF = 4.1f;
constexpr float FOCAL = 240.f;
constexpr float CAM_H = 1.15f;
constexpr int HORIZON = 90;

float laneAt(float z) {
    return 2.4f * std::sin(z * 0.014f) + 1.15f * std::sin(z * 0.031f + 1.1f);
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Result) return 2;
    return 1;
}

bool Game::onIce(float z) const {
    float u = std::fmod(z, 140.f);
    if (u < 0) u += 140.f;
    return u > 96.f && u < 122.f;
}

void Game::resetRide() {
    x_ = laneAt(0);
    latV_ = 0;
    steer_ = 0;
    bales_ = {{70.f, 1.7f, false},   {128.f, -1.85f, false}, {186.f, 1.55f, false}, {248.f, -1.6f, false},
              {312.f, 1.75f, false}, {368.f, -1.45f, false}, {430.f, 1.5f, false},  {478.f, -1.7f, false}};
    puffs_.clear();
}

void Game::beginRun() {
    resetRide();
    z_ = 0;
    speed_ = 18.f;
    clock_ = CLOCK0;
    won_ = false;
    over_ = false;
    why_ = 0;
    shake_ = 0;
    tRun_ = 0;
    mode_ = Mode::Run;
}

void Game::botRide(float& steer, float& gas, float& brake) const {
    float look = 14.f + speed_ * 0.32f;
    float want = laneAt(z_ + look);
    for (const Bale& s : bales_) {
        if (s.hit) continue;
        float dz = s.z - z_;
        if (dz < 3.f || dz > look) continue;
        float sx = laneAt(s.z) + s.off;
        if (std::fabs(x_ - sx) < 2.4f) want = sx + (s.off > 0.f ? -2.35f : 2.35f);
    }
    float err = want - x_;
    steer = std::clamp(err * 0.85f - latV_ * 0.32f, -1.f, 1.f);
    float bend = std::fabs(laneAt(z_ + 18.f) - laneAt(z_));
    bool ice = onIce(z_ + 10.f);
    gas = (speed_ < 32.f && bend < 1.7f && !ice) ? 1.f : 0.4f;
    brake = (std::fabs(err) > 2.6f || bend > 2.05f || (ice && speed_ > 24.f)) ? 1.f : 0.f;
}

void Game::humanRide(const gs::Pad& pad, float& steer, float& gas, float& brake) const {
    steer = 0;
    if (pad.down(gs::BTN_LEFT)) steer -= 1.f;
    if (pad.down(gs::BTN_RIGHT)) steer += 1.f;
    if (std::fabs(pad.axisX) > 0.18f) steer = pad.axisX;
    steer = std::clamp(steer, -1.f, 1.f);
    gas = (pad.down(gs::BTN_A) || pad.down(gs::BTN_UP) || pad.accel > 0.2f) ? 1.f : 0.f;
    brake = (pad.down(gs::BTN_B) || pad.down(gs::BTN_DOWN) || pad.brake > 0.2f) ? 1.f : 0.f;
}

void Game::integrate(float steer, float gas, float brake) {
    float follow = std::min(1.f, 14.f * DT);
    steer_ += (steer - steer_) * follow;
    bool ice = onIce(z_);
    float grip = ice ? 1.6f : 3.6f;
    float auth = (ice ? 3.2f : 6.2f) + speed_ * 0.22f;
    latV_ += (steer_ * auth - latV_ * grip) * DT;
    x_ += latV_ * DT;
    float drag = ice ? 1.8f : 0.35f;
    float push = gas * 11.f - brake * 16.f - drag * speed_ * 0.08f - 0.4f;
    speed_ += push * DT;
    speed_ = std::clamp(speed_, 10.f, 36.f);
    z_ += speed_ * DT;

    if ((mode_ == Mode::Run || mode_ == Mode::Title) && (int(t_ * 60.f) % 3) == 0 && puffs_.size() < 12) {
        Puff p;
        p.x = 160.f + steer_ * 18.f + (puffs_.size() & 1 ? -16.f : 16.f);
        p.y = 200.f;
        p.life = 0.28f;
        p.s = 5.f;
        puffs_.push_back(p);
    }
    for (Puff& p : puffs_) {
        p.y -= 12.f * DT;
        p.life -= DT;
        p.s += 10.f * DT;
    }
    while (!puffs_.empty() && puffs_.front().life <= 0) puffs_.erase(puffs_.begin());

    if (mode_ != Mode::Run) return;
    for (Bale& s : bales_) {
        if (s.hit) continue;
        float sx = laneAt(s.z) + s.off;
        if (std::fabs(z_ - s.z) < 1.5f && std::fabs(x_ - sx) < 0.85f) {
            s.hit = true;
            speed_ *= 0.58f;
            clock_ = std::max(0.f, clock_ - 1.8f);
            shake_ = 6.f;
            sys_->apu.noiseBurst(0.4f, 200.f, 0.16f);
        }
    }
}

void Game::endRun(int why) {
    why_ = why;
    won_ = why == 1;
    over_ = true;
    mode_ = Mode::Result;
    if (won_) sys_->apu.tone(1, 740.f, 0.14f);
    else sys_->apu.noiseBurst(0.55f, 80.f, 0.3f);
    sys_->rumble(won_ ? 0.15f : 0.8f, won_ ? 0.4f : 1.f, won_ ? 100 : 220);
}

Game::Proj Game::project(float wz, float wx) const {
    Proj p;
    float dz = wz - z_;
    if (dz < 2.2f || dz > 170.f) return p;
    p.y = float(hor_) + FOCAL * CAM_H / dz;
    if (p.y > gs::SCREEN_H + 40.f) return p;
    p.z = dz;
    p.hw = FOCAL * HALF / dz;
    p.x = 160.f + (wx - x_) * p.hw / HALF + shake_ * (dz < 22.f ? 0.55f : 0.12f);
    p.fog = std::clamp((dz - 18.f) / 11.f, 0.f, 14.f);
    p.ok = true;
    return p;
}

void Game::blit(const gs::Mipped& m, float cx, float foot, float h, int pal, bool flip, int fog) {
    if (h < 2.f) return;
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
    hor_ = HORIZON + int(std::sin(t_ * 1.4f));
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = std::clamp(float(y) / float(std::max(hor_, 1)), 0.f, 1.f);
        int r = int((2 + 4 * u) * (1.f - storm * 0.4f) + 3 * storm);
        int g = int((3 + 4 * u) * (1.f - storm * 0.35f) + 4 * storm);
        int b = int((7 + 4 * u) * (1.f - storm * 0.2f) + 6 * storm);
        if (y > hor_) {
            r = 3;
            g = 4;
            b = 5;
        }
        v.lineBackdrop[y] = gs::rgb4(std::clamp(r, 0, 15), std::clamp(g, 0, 15), std::clamp(b, 0, 15));
        v.lineFog[y] = y <= hor_ ? uint8_t((1.f - u) * 2.f + storm * 6.f) : uint8_t(2 + storm * 4.f);
        v.B.hscroll[y] = int16_t(-steer_ * 14.f - int(z_ * 0.4f));
        v.B.vscroll[y] = 0;
    }
    v.A.enabled = false;
    v.B.enabled = true;
}

void Game::road() {
    gs::VDP& v = sys_->vdp;
    v.roadTime = int(t_ * 40.f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        gs::RoadLine& L = v.road[y];
        L.on = false;
        if (y <= hor_) continue;
        float dz = FOCAL * CAM_H / float(y - hor_);
        if (dz > 180.f || dz < 1.2f) continue;
        float wz = z_ + dz;
        float hw = FOCAL * HALF / dz;
        L.on = true;
        L.cx = 160.f + (laneAt(wz) - x_) * hw / HALF + shake_ * (dz < 16.f ? 0.7f : 0.1f);
        L.hw = hw;
        L.v = wz * 16.f;
        L.pal = PAL_ROAD;
        L.band = (int(std::floor(wz * 0.2f)) & 1) ? 1 : 0;
        L.style = onIce(wz) ? gs::ROAD_ICE : 1;
        L.left = L.right = gs::GROUND_SNOWWALL;
    }
}

void Game::world() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    if (mode_ == Mode::Title) {
        ui(art_.title, (320 - art_.title.w) * 0.5f, 8, PAL_TITLE);
        ui(art_.sub, (320 - art_.sub.w) * 0.5f, 16.f + art_.title.h, PAL_TITLE);
    } else if (mode_ == Mode::Pause) {
        ui(art_.paused, (320 - art_.paused.w) * 0.5f, 22, PAL_TITLE);
    } else if (mode_ == Mode::Result) {
        const gs::Image& im = why_ == 1 ? art_.cleared : why_ == 2 ? art_.missed : art_.late;
        ui(im, (320 - im.w) * 0.5f, 18, PAL_TITLE);
    }

    int frame = std::clamp(int(std::lround(steer_ * 2.f)) + 2, 0, 4);
    blit(art_.kart[frame], 160.f + steer_ * 14.f, 204.f, 58.f, PAL_KART, false, 0);
    for (const Puff& p : puffs_) {
        if (p.life <= 0) continue;
        blit(art_.spray, p.x, p.y, p.s, PAL_SPRAY, false, int((1.f - p.life / 0.28f) * 8.f));
    }

    auto gate = [&](float gz) {
        Proj p = project(gz, laneAt(gz));
        if (!p.ok) return;
        blit(art_.arch, p.x, p.y, p.hw * 1.35f, PAL_GATE, false, int(p.fog));
    };
    gate(8.f);
    gate(LENGTH - 2.f);

    for (const Bale& s : bales_) {
        if (s.hit) continue;
        Proj p = project(s.z, laneAt(s.z) + s.off);
        if (!p.ok) continue;
        blit(art_.bale, p.x, p.y, FOCAL * 0.7f / p.z, PAL_BALE, false, int(p.fog));
    }

    float pineZ = std::ceil((z_ + 6.f) / 18.f) * 18.f;
    for (float zz = pineZ; zz < z_ + 140.f; zz += 18.f) {
        float c = laneAt(zz);
        Proj left = project(zz, c - (HALF + 2.8f));
        Proj right = project(zz, c + HALF + 2.8f);
        if (left.ok) blit(art_.peak, left.x, left.y, FOCAL * 3.4f / left.z, PAL_PEAK, false, int(left.fog));
        if (right.ok) blit(art_.peak, right.x, right.y, FOCAL * 3.1f / right.z, PAL_PEAK, true, int(right.fog));
    }
}

void Game::hud() {
    gs::Plane& h = sys_->vdp.HUD;
    for (int y = 0; y < 28; y++)
        for (int x = 0; x < 40; x++) h.set(x, y, 0);

    char line[48];
    if (mode_ == Mode::Title) {
        hudText(2, 25, "CLEAR THE PASS BEFORE THE STORM", PAL_HOT);
        hudText(1, 26, "ARROWS STEER  A GAS  B BRAKE", PAL_INK);
        hudText(8, 27, "START DROPS THE FLAG", PAL_GOOD);
        return;
    }
    int cs = std::max(0, int(std::ceil(clock_)));
    int pal = cs < 8 ? PAL_BAD : cs < 16 ? PAL_HOT : PAL_GOOD;
    std::snprintf(line, sizeof line, "STORM %02d", cs);
    hudText(1, 0, line, pal);
    std::snprintf(line, sizeof line, "%2d M/S", int(speed_ + 0.5f));
    hudText(32, 0, line, PAL_INK);
    int travelled = int(std::clamp(z_, 0.f, LENGTH));
    std::snprintf(line, sizeof line, "LEG %3d/%d", travelled, int(LENGTH));
    hudText(1, 1, line, PAL_INK);

    if (mode_ == Mode::Pause) hudText(12, 27, "START RESUMES", PAL_INK);
    else if (mode_ == Mode::Run) {
        float err = x_ - laneAt(z_);
        if (onIce(z_)) hudText(12, 27, "ICE  EASE OFF", PAL_HOT);
        else if (std::fabs(err) > HALF * 0.72f)
            hudText(err > 0 ? 9 : 11, 27, err > 0 ? "CUT LEFT" : "CUT RIGHT", PAL_BAD);
        else hudText(9, 27, "HOLD THE KART IN", PAL_INK);
    } else if (won_) {
        hudText(4, 27, "LEG CLEAR. START AGAIN", PAL_GOOD);
    } else if (why_ == 2) {
        hudText(3, 27, "OFF THE PASS. LEG FAILED", PAL_BAD);
    } else {
        hudText(2, 27, "CLOCK GONE. MISSED THE END", PAL_BAD);
    }
}

void Game::audio() {
    if (mode_ == Mode::Run || mode_ == Mode::Title) {
        sys_->apu.tone(0, 70.f + speed_ * 8.f, 0.05f + speed_ * 0.0012f);
        sys_->apu.noise(0.025f + speed_ * 0.001f, 700.f + speed_ * 18.f, false);
    } else {
        sys_->apu.tone(0, 0, 0);
        sys_->apu.noise(0, 0, false);
    }
    if (won_) sys_->setLight(30, 170, 80);
    else if (mode_ == Mode::Run && clock_ < 8.f) sys_->setLight(30, 40, 120);
    else sys_->setLight(80, 100, 140);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.apu.setMaster(0.8f);
    if (bot_) {
        beginRun();
        return;
    }
    mode_ = Mode::Title;
    resetRide();
    z_ = 20.f;
    speed_ = 20.f;
    clock_ = CLOCK0;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    t_ += DT;
    shake_ *= std::max(0.f, 1.f - 6.f * DT);

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_MODE)) sys.eject();
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) beginRun();
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Run;
    } else if (mode_ == Mode::Result) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) beginRun();
    }

    if (mode_ == Mode::Title) {
        float steer, gas, brake;
        botRide(steer, gas, brake);
        integrate(steer, gas, 0);
        if (std::fabs(x_ - laneAt(z_)) > HALF || z_ > 200.f) {
            resetRide();
            z_ = 10.f;
            speed_ = 20.f;
        }
    } else if (mode_ == Mode::Run) {
        float steer, gas, brake;
        if (bot_) botRide(steer, gas, brake);
        else humanRide(pad, steer, gas, brake);
        integrate(steer, gas, brake);
        tRun_ += DT;
        if (std::fabs(x_ - laneAt(z_)) > HALF) endRun(2);
        else if (z_ >= LENGTH && clock_ > 0.f) endRun(1);
        else {
            clock_ -= DT;
            if (clock_ <= 0.f) endRun(3);
        }
    }

    sky();
    road();
    world();
    hud();
    audio();
}

}  // namespace kartpass

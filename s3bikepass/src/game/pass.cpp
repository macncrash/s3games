#include "game/pass.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace bikepass {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float LENGTH = 460.f;
constexpr float CLOCK0 = 32.f;
constexpr float HALF = 3.7f;
constexpr float FOCAL = 250.f;
constexpr float CAM_H = 1.05f;
constexpr int HORIZON = 98;

float laneAt(float z) {
    return 2.15f * std::sin(z * 0.018f) + 1.05f * std::sin(z * 0.043f + 0.8f);
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Result) return 2;
    return 1;
}

void Game::resetRide() {
    x_ = laneAt(0);
    latV_ = 0;
    steer_ = 0;
    slabs_ = {{78.f, 1.45f, false},  {150.f, -1.5f, false}, {224.f, 1.35f, false},
              {300.f, -1.55f, false}, {372.f, 1.4f, false},  {430.f, -1.2f, false}};
    puffs_.clear();
}

void Game::beginRun() {
    resetRide();
    z_ = 0;
    speed_ = 16.f;
    clock_ = CLOCK0;
    won_ = false;
    over_ = false;
    why_ = 0;
    shake_ = 0;
    tRun_ = 0;
    mode_ = Mode::Run;
}

void Game::botRide(float& steer, float& pedal, float& brake) const {
    float look = 12.f + speed_ * 0.38f;
    float want = laneAt(z_ + look);
    for (const Slab& s : slabs_) {
        if (s.hit) continue;
        float dz = s.z - z_;
        if (dz < 4.f || dz > look) continue;
        float sx = laneAt(s.z) + s.off;
        if (std::fabs(x_ - sx) < 2.1f) want = sx + (s.off > 0.f ? -2.15f : 2.15f);
    }
    float err = want - x_;
    steer = std::clamp(err * 0.9f - latV_ * 0.28f, -1.f, 1.f);
    float bend = std::fabs(laneAt(z_ + 16.f) - laneAt(z_));
    pedal = (speed_ < 25.f && bend < 1.6f) ? 1.f : 0.35f;
    brake = (std::fabs(err) > 2.4f || bend > 1.85f) ? 1.f : 0.f;
}

void Game::humanRide(const gs::Pad& pad, float& steer, float& pedal, float& brake) const {
    steer = 0;
    if (pad.down(gs::BTN_LEFT)) steer -= 1.f;
    if (pad.down(gs::BTN_RIGHT)) steer += 1.f;
    if (std::fabs(pad.axisX) > 0.18f) steer = pad.axisX;
    steer = std::clamp(steer, -1.f, 1.f);
    pedal = (pad.down(gs::BTN_A) || pad.down(gs::BTN_UP) || pad.accel > 0.2f) ? 1.f : 0.f;
    brake = (pad.down(gs::BTN_B) || pad.down(gs::BTN_DOWN) || pad.brake > 0.2f) ? 1.f : 0.f;
}

void Game::integrate(float steer, float pedal, float brake) {
    float follow = std::min(1.f, 12.f * DT);
    steer_ += (steer - steer_) * follow;
    float auth = 5.5f + speed_ * 0.28f;
    latV_ += (steer_ * auth - latV_ * 3.4f) * DT;
    x_ += latV_ * DT;
    float push = pedal * 8.5f - brake * 14.f - 0.28f * speed_ - 0.6f;
    speed_ += push * DT;
    speed_ = std::clamp(speed_, 8.f, 27.f);
    z_ += speed_ * DT;

    if ((mode_ == Mode::Run || mode_ == Mode::Title) && (int(t_ * 60.f) % 4) == 0 && puffs_.size() < 10) {
        Puff p;
        p.x = 156.f + steer_ * 10.f;
        p.y = 196.f;
        p.life = 0.32f;
        p.s = 6.f;
        puffs_.push_back(p);
    }
    for (Puff& p : puffs_) {
        p.y -= 10.f * DT;
        p.life -= DT;
        p.s += 8.f * DT;
    }
    while (!puffs_.empty() && puffs_.front().life <= 0) puffs_.erase(puffs_.begin());

    if (mode_ != Mode::Run) return;
    for (Slab& s : slabs_) {
        if (s.hit) continue;
        float sx = laneAt(s.z) + s.off;
        if (std::fabs(z_ - s.z) < 1.4f && std::fabs(x_ - sx) < 0.72f) {
            s.hit = true;
            speed_ *= 0.62f;
            clock_ = std::max(0.f, clock_ - 1.6f);
            shake_ = 5.f;
            sys_->apu.noiseBurst(0.35f, 180.f, 0.14f);
        }
    }
}

void Game::endRun(int why) {
    why_ = why;
    won_ = why == 1;
    over_ = true;
    mode_ = Mode::Result;
    if (won_) sys_->apu.tone(1, 880.f, 0.12f);
    else sys_->apu.noiseBurst(0.55f, 90.f, 0.28f);
    sys_->rumble(won_ ? 0.15f : 0.8f, won_ ? 0.35f : 1.f, won_ ? 90 : 200);
}

Game::Proj Game::project(float wz, float wx) const {
    Proj p;
    float dz = wz - z_;
    if (dz < 2.4f || dz > 160.f) return p;
    p.y = float(hor_) + FOCAL * CAM_H / dz;
    if (p.y > gs::SCREEN_H + 30.f) return p;
    p.z = dz;
    p.hw = FOCAL * HALF / dz;
    p.x = 160.f + (wx - x_) * p.hw / HALF + shake_ * (dz < 24.f ? 0.6f : 0.15f);
    p.fog = std::clamp((dz - 16.f) / 12.f, 0.f, 14.f);
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
    hor_ = HORIZON + int(std::sin(t_ * 1.2f));
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = std::clamp(float(y) / float(std::max(hor_, 1)), 0.f, 1.f);
        int r = int((3 + 5 * u) * (1.f - storm * 0.45f) + 2 * storm);
        int g = int((4 + 4 * u) * (1.f - storm * 0.4f) + 3 * storm);
        int b = int((8 + 3 * u) * (1.f - storm * 0.25f) + 5 * storm);
        if (y > hor_) {
            r = 2;
            g = 4;
            b = 2;
        }
        v.lineBackdrop[y] = gs::rgb4(std::clamp(r, 0, 15), std::clamp(g, 0, 15), std::clamp(b, 0, 15));
        v.lineFog[y] = y <= hor_ ? uint8_t((1.f - u) * 3.f + storm * 5.f) : uint8_t(2 + storm * 3.f);
        v.B.hscroll[y] = int16_t(-steer_ * 18.f);
        v.B.vscroll[y] = 0;
    }
    v.A.enabled = false;
    v.B.enabled = true;
}

void Game::road() {
    gs::VDP& v = sys_->vdp;
    v.roadTime = int(t_ * 50.f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        gs::RoadLine& L = v.road[y];
        L.on = false;
        if (y <= hor_) continue;
        float dz = FOCAL * CAM_H / float(y - hor_);
        if (dz > 170.f || dz < 1.3f) continue;
        float hw = FOCAL * HALF / dz;
        L.on = true;
        L.cx = 160.f + (laneAt(z_ + dz) - x_) * hw / HALF + shake_ * (dz < 18.f ? 0.8f : 0.1f);
        L.hw = hw;
        L.v = (z_ + dz) * 18.f;
        L.pal = PAL_ROAD;
        L.band = (int(std::floor((z_ + dz) * 0.16f)) & 1) ? 1 : 0;
        L.style = 1;
        L.left = L.right = gs::GROUND_LAND;
    }
}

void Game::world() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    if (mode_ == Mode::Title) {
        ui(art_.title, (320 - art_.title.w) * 0.5f, 6, PAL_TITLE);
        ui(art_.sub, (320 - art_.sub.w) * 0.5f, 14.f + art_.title.h, PAL_TITLE);
    } else if (mode_ == Mode::Pause) {
        ui(art_.paused, (320 - art_.paused.w) * 0.5f, 24, PAL_TITLE);
    } else if (mode_ == Mode::Result) {
        const gs::Image& im = why_ == 1 ? art_.cleared : why_ == 2 ? art_.ditch : art_.late;
        ui(im, (320 - im.w) * 0.5f, 20, PAL_TITLE);
    }

    int frame = std::clamp(int(std::lround(steer_ * 2.f)) + 2, 0, 4);
    blit(art_.bike[frame], 160.f + steer_ * 16.f, 198.f, 62.f, PAL_BIKE, false, 0);
    for (const Puff& p : puffs_) {
        if (p.life <= 0) continue;
        blit(art_.dust, p.x, p.y, p.s, PAL_DUST, false, int((1.f - p.life / 0.32f) * 8.f));
    }

    auto gate = [&](const gs::Mipped& img, float gz) {
        Proj p = project(gz, laneAt(gz));
        if (!p.ok) return;
        blit(img, p.x, p.y, p.hw * 1.15f, PAL_GATE, false, int(p.fog));
    };
    gate(art_.gateOpen, 6.f);
    gate(art_.gateShut, LENGTH - 4.f);

    for (const Slab& s : slabs_) {
        Proj p = project(s.z, laneAt(s.z) + s.off);
        if (!p.ok) continue;
        blit(art_.log, p.x, p.y, FOCAL * 0.55f / p.z, PAL_LOG, false, int(p.fog));
    }

    float pineZ = std::ceil((z_ + 8.f) / 16.f) * 16.f;
    for (float zz = pineZ; zz < z_ + 130.f; zz += 16.f) {
        float c = laneAt(zz);
        Proj left = project(zz, c - (HALF + 2.4f));
        Proj right = project(zz, c + HALF + 2.4f);
        if (left.ok) blit(art_.peak, left.x, left.y, FOCAL * 3.1f / left.z, PAL_PEAK, false, int(left.fog));
        if (right.ok) blit(art_.peak, right.x, right.y, FOCAL * 2.8f / right.z, PAL_PEAK, true, int(right.fog));
    }
}

void Game::hud() {
    gs::Plane& h = sys_->vdp.HUD;
    for (int y = 0; y < 28; y++)
        for (int x = 0; x < 40; x++) h.set(x, y, 0);

    char line[48];
    if (mode_ == Mode::Title) {
        hudText(3, 25, "CLEAR THE PASS BEFORE THE STORM", PAL_HOT);
        hudText(2, 26, "ARROWS LEAN   A PEDAL   B BRAKE", PAL_INK);
        hudText(10, 27, "START ROLLS", PAL_GOOD);
        return;
    }
    int cs = std::max(0, int(std::ceil(clock_)));
    int pal = cs < 8 ? PAL_BAD : cs < 16 ? PAL_HOT : PAL_GOOD;
    std::snprintf(line, sizeof line, "STORM %02d", cs);
    hudText(1, 0, line, pal);
    std::snprintf(line, sizeof line, "%2d M/S", int(speed_ + 0.5f));
    hudText(32, 0, line, PAL_INK);
    int travelled = int(std::clamp(z_, 0.f, LENGTH));
    std::snprintf(line, sizeof line, "PASS %3d/%d", travelled, int(LENGTH));
    hudText(1, 1, line, PAL_INK);

    if (mode_ == Mode::Pause) hudText(12, 27, "START RESUMES", PAL_INK);
    else if (mode_ == Mode::Run) {
        float err = x_ - laneAt(z_);
        if (std::fabs(err) > HALF * 0.72f) hudText(err > 0 ? 8 : 10, 27, err > 0 ? "LEAN LEFT" : "LEAN RIGHT", PAL_BAD);
        else hudText(8, 27, "A PEDALS THE CLIMB", PAL_INK);
    } else if (won_) {
        hudText(5, 27, "CLEAR. START RIDES AGAIN", PAL_GOOD);
    } else if (why_ == 2) {
        hudText(4, 27, "DITCHED. START TRIES AGAIN", PAL_BAD);
    } else {
        hudText(3, 27, "STORM TOOK THE PASS. START", PAL_BAD);
    }
}

void Game::audio() {
    if (mode_ == Mode::Run || mode_ == Mode::Title) {
        sys_->apu.tone(0, 90.f + speed_ * 7.f, 0.045f + speed_ * 0.001f);
        sys_->apu.noise(0.02f + speed_ * 0.001f, 500.f + speed_ * 20.f, false);
    } else {
        sys_->apu.tone(0, 0, 0);
        sys_->apu.noise(0, 0, false);
    }
    if (won_) sys_->setLight(40, 180, 90);
    else if (mode_ == Mode::Run && clock_ < 8.f) sys_->setLight(40, 50, 110);
    else sys_->setLight(90, 110, 140);
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
    z_ = 24.f;
    speed_ = 18.f;
    clock_ = CLOCK0;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    t_ += DT;
    shake_ *= std::max(0.f, 1.f - 5.f * DT);

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
        float steer, pedal, brake;
        botRide(steer, pedal, brake);
        integrate(steer, pedal, 0);
        if (std::fabs(x_ - laneAt(z_)) > HALF || z_ > 180.f) {
            resetRide();
            z_ = 12.f;
            speed_ = 18.f;
        }
    } else if (mode_ == Mode::Run) {
        float steer, pedal, brake;
        if (bot_) botRide(steer, pedal, brake);
        else humanRide(pad, steer, pedal, brake);
        integrate(steer, pedal, brake);
        tRun_ += DT;
        clock_ -= DT;
        if (std::fabs(x_ - laneAt(z_)) > HALF) endRun(2);
        else if (z_ >= LENGTH) endRun(1);
        else if (clock_ <= 0.f) endRun(3);
    }

    sky();
    road();
    world();
    hud();
    audio();
}

}  // namespace bikepass

#include "game/pass.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace metropass {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float LENGTH = 500.f;
constexpr float CLOCK0 = 34.f;
constexpr float HALF = 3.6f;
constexpr float RAIL = 1.15f;
constexpr float FOCAL = 250.f;
constexpr float CAM_H = 1.15f;
constexpr int HORIZON = 96;

float laneAt(float z) {
    return 1.7f * std::sin(z * 0.016f) + 0.85f * std::sin(z * 0.037f + 1.1f);
}

float railX(float z, int side) { return laneAt(z) + float(side) * RAIL; }

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Result) return 2;
    return 1;
}

void Game::resetRide() {
    rail_ = -1;
    x_ = railX(0, rail_);
    speed_ = 18.f;
    power_ = 0;
    brake_ = 0;
    blocks_ = {{64.f, 1, false},   {118.f, -1, false}, {176.f, 1, false},  {232.f, -1, false},
               {292.f, 1, false},  {348.f, -1, false}, {404.f, 1, false},  {456.f, -1, false}};
    flakes_.clear();
}

void Game::beginRun() {
    resetRide();
    z_ = 0;
    clock_ = CLOCK0;
    won_ = false;
    over_ = false;
    why_ = 0;
    shake_ = 0;
    tRun_ = 0;
    mode_ = Mode::Run;
}

void Game::botDrive(int& rail, float& power, float& brake) const {
    rail = rail_;
    float nearest = 1e9f;
    int threat = 0;
    for (const Block& b : blocks_) {
        if (b.hit) continue;
        float dz = b.z - z_;
        if (dz < 2.f || dz > 48.f) continue;
        if (b.side == rail_ && dz < nearest) {
            nearest = dz;
            threat = b.side;
        }
    }
    if (threat) rail = -threat;
    float hx = railX(z_ + std::min(nearest, 20.f), threat ? threat : rail_);
    bool stillOn = threat && std::fabs(x_ - hx) < 0.95f && nearest < 16.f;
    power = speed_ < 29.f ? 1.f : 0.25f;
    brake = stillOn ? 1.f : 0.f;
    if (brake > 0.f) power = 0.f;
}

void Game::humanDrive(const gs::Pad& pad, int& rail, float& power, float& brake) const {
    rail = rail_;
    if (pad.pressed(gs::BTN_LEFT) || pad.axisX < -0.45f) rail = -1;
    if (pad.pressed(gs::BTN_RIGHT) || pad.axisX > 0.45f) rail = 1;
    power = (pad.down(gs::BTN_A) || pad.down(gs::BTN_UP) || pad.accel > 0.2f) ? 1.f : 0.f;
    brake = (pad.down(gs::BTN_B) || pad.down(gs::BTN_DOWN) || pad.brake > 0.2f) ? 1.f : 0.f;
}

void Game::integrate(int rail, float power, float brake) {
    rail_ = rail;
    power_ = power;
    brake_ = brake;
    float goal = railX(z_, rail_);
    float step = 7.5f * DT;
    if (x_ < goal) x_ = std::min(goal, x_ + step);
    else x_ = std::max(goal, x_ - step);
    float push = power * 11.f - brake * 18.f - 0.34f * speed_;
    speed_ += push * DT;
    speed_ = std::clamp(speed_, 11.f, 32.f);
    z_ += speed_ * DT;

    if ((mode_ == Mode::Run || mode_ == Mode::Title) && (int(t_ * 60.f) % 3) == 0 && flakes_.size() < 12) {
        Flake f;
        f.x = 150.f + (x_ - laneAt(z_)) * 18.f;
        f.y = 200.f;
        f.life = 0.4f;
        f.s = 5.f;
        flakes_.push_back(f);
    }
    for (Flake& f : flakes_) {
        f.y -= 14.f * DT;
        f.life -= DT;
        f.s += 6.f * DT;
    }
    while (!flakes_.empty() && flakes_.front().life <= 0) flakes_.erase(flakes_.begin());

    if (mode_ != Mode::Run) return;
    for (Block& b : blocks_) {
        if (b.hit) continue;
        float hx = railX(b.z, b.side);
        if (std::fabs(z_ - b.z) < 1.7f && std::fabs(x_ - hx) < 0.78f) {
            b.hit = true;
            shake_ = 6.f;
            if (speed_ > 23.f && brake < 0.5f) {
                endRun(2);
                return;
            }
            speed_ *= 0.58f;
            clock_ = std::max(0.f, clock_ - 1.8f);
            sys_->apu.noiseBurst(0.4f, 160.f, 0.16f);
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
    sys_->rumble(won_ ? 0.12f : 0.7f, won_ ? 0.3f : 1.f, won_ ? 80 : 180);
}

Game::Proj Game::project(float wz, float wx) const {
    Proj p;
    float dz = wz - z_;
    if (dz < 2.2f || dz > 160.f) return p;
    p.y = float(hor_) + FOCAL * CAM_H / dz;
    if (p.y > gs::SCREEN_H + 24.f) return p;
    p.z = dz;
    p.hw = FOCAL * HALF / dz;
    p.x = 160.f + (wx - x_) * p.hw / HALF + shake_ * (dz < 22.f ? 0.5f : 0.1f);
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
    hor_ = HORIZON + int(std::sin(t_ * 0.8f));
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = std::clamp(float(y) / float(std::max(hor_, 1)), 0.f, 1.f);
        int r = int((2 + 4 * u) * (1.f - storm * 0.4f) + 3 * storm);
        int g = int((3 + 4 * u) * (1.f - storm * 0.35f) + 4 * storm);
        int b = int((7 + 4 * u) * (1.f - storm * 0.2f) + 6 * storm);
        if (y > hor_) {
            r = 3;
            g = 4;
            b = 6;
        }
        v.lineBackdrop[y] = gs::rgb4(std::clamp(r, 0, 15), std::clamp(g, 0, 15), std::clamp(b, 0, 15));
        v.lineFog[y] = y <= hor_ ? uint8_t((1.f - u) * 2.f + storm * 6.f) : uint8_t(2 + storm * 4.f);
        v.B.hscroll[y] = int16_t((laneAt(z_) - x_) * 10.f);
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
        float hw = FOCAL * HALF / dz;
        L.on = true;
        L.cx = 160.f + (laneAt(z_ + dz) - x_) * hw / HALF + shake_ * (dz < 16.f ? 0.7f : 0.1f);
        L.hw = hw;
        L.v = (z_ + dz) * 16.f;
        L.pal = PAL_ROAD;
        L.band = (int(std::floor((z_ + dz) * 0.12f)) & 1) ? 1 : 0;
        L.style = 1;
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
        ui(art_.held, (320 - art_.held.w) * 0.5f, 22, PAL_TITLE);
    } else if (mode_ == Mode::Result) {
        const gs::Image& im = why_ == 1 ? art_.cleared : why_ == 2 ? art_.derail : art_.late;
        ui(im, (320 - im.w) * 0.5f, 18, PAL_TITLE);
    }

    int frame = rail_ < 0 ? 0 : rail_ > 0 ? 2 : 1;
    float rel = x_ - laneAt(z_);
    float footDz = FOCAL * CAM_H / float(std::max(8, 206 - hor_));
    float footHw = FOCAL * HALF / footDz;
    float carX = 160.f + rel * footHw / HALF;
    blit(art_.car[frame], carX, 206.f, 68.f, PAL_CAR, false, 0);
    for (const Flake& f : flakes_) {
        if (f.life <= 0) continue;
        blit(art_.snow, f.x, f.y, f.s, PAL_SNOW, false, int((1.f - f.life / 0.4f) * 8.f));
    }

    auto mouth = [&](float gz) {
        Proj p = project(gz, laneAt(gz));
        if (!p.ok) return;
        blit(art_.mouth, p.x, p.y, p.hw * 1.25f, PAL_MOUTH, false, int(p.fog));
    };
    mouth(8.f);
    mouth(LENGTH - 6.f);

    for (const Block& b : blocks_) {
        if (b.hit) continue;
        Proj p = project(b.z, railX(b.z, b.side));
        if (!p.ok) continue;
        blit(art_.rock, p.x, p.y, FOCAL * 0.7f / p.z, PAL_ROCK, b.side < 0, int(p.fog));
    }

    float poleZ = std::ceil((z_ + 10.f) / 22.f) * 22.f;
    for (float zz = poleZ; zz < z_ + 140.f; zz += 22.f) {
        float c = laneAt(zz);
        Proj left = project(zz, c - (HALF + 1.6f));
        Proj right = project(zz, c + HALF + 1.6f);
        if (left.ok) blit(art_.pole, left.x, left.y, FOCAL * 2.4f / left.z, PAL_POLE, false, int(left.fog));
        if (right.ok) blit(art_.pole, right.x, right.y, FOCAL * 2.2f / right.z, PAL_POLE, true, int(right.fog));
        Proj pk = project(zz + 8.f, c - (HALF + 3.2f));
        if (pk.ok) blit(art_.peak, pk.x, pk.y, FOCAL * 3.4f / pk.z, PAL_PEAK, false, int(pk.fog));
    }
}

void Game::hud() {
    gs::Plane& h = sys_->vdp.HUD;
    for (int y = 0; y < 28; y++)
        for (int x = 0; x < 40; x++) h.set(x, y, 0);

    char line[48];
    if (mode_ == Mode::Title) {
        hudText(2, 25, "CLEAR THE PASS BEFORE THE STORM", PAL_HOT);
        hudText(1, 26, "LEFT RIGHT RAILS  A POWER  B BRAKE", PAL_INK);
        hudText(11, 27, "START DEPARTS", PAL_GOOD);
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
    hudText(30, 1, rail_ < 0 ? "RAIL L" : "RAIL R", PAL_INK);

    if (mode_ == Mode::Pause) hudText(12, 27, "START RESUMES", PAL_INK);
    else if (mode_ == Mode::Run) hudText(6, 27, "SWITCH BEFORE THE SLIDE", PAL_INK);
    else if (won_) hudText(4, 27, "CLEAR. START RUNS AGAIN", PAL_GOOD);
    else if (why_ == 2) hudText(3, 27, "OFF THE RAIL. START AGAIN", PAL_BAD);
    else hudText(2, 27, "STORM SHUT THE LINE. START", PAL_BAD);
}

void Game::audio() {
    if (mode_ == Mode::Run || mode_ == Mode::Title) {
        sys_->apu.tone(0, 70.f + speed_ * 6.f, 0.04f + speed_ * 0.0012f);
        sys_->apu.noise(0.015f + speed_ * 0.0008f, 280.f + speed_ * 12.f, false);
    } else {
        sys_->apu.tone(0, 0, 0);
        sys_->apu.noise(0, 0, false);
    }
    if (won_) sys_->setLight(40, 170, 110);
    else if (mode_ == Mode::Run && clock_ < 8.f) sys_->setLight(30, 40, 120);
    else sys_->setLight(80, 90, 140);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.apu.setMaster(0.75f);
    if (bot_) {
        beginRun();
        return;
    }
    mode_ = Mode::Title;
    resetRide();
    z_ = 18.f;
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
        int rail;
        float power, brake;
        botDrive(rail, power, brake);
        integrate(rail, power, 0);
        if (z_ > 160.f) {
            resetRide();
            z_ = 12.f;
            speed_ = 20.f;
        }
    } else if (mode_ == Mode::Run) {
        int rail;
        float power, brake;
        if (bot_) botDrive(rail, power, brake);
        else humanDrive(pad, rail, power, brake);
        integrate(rail, power, brake);
        if (mode_ != Mode::Run) {
            sky();
            road();
            world();
            hud();
            audio();
            return;
        }
        tRun_ += DT;
        clock_ -= DT;
        if (z_ >= LENGTH && clock_ > 0.f) endRun(1);
        else if (clock_ <= 0.f) endRun(3);
    }

    sky();
    road();
    world();
    hud();
    audio();
}

}  // namespace metropass

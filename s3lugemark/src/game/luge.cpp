#include "game/luge.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace luge {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float MARK_Z = 340.f;
constexpr float MARK_LEN = 18.f;
constexpr float MARK_X = 0.72f;
constexpr float MARK_HW = 0.9f;
constexpr float ROAD_HW = 3.05f;
constexpr float CAM_H = 2.75f;
constexpr float FOCAL = 250.f;
constexpr float HORIZON = 78.f;
constexpr float COAST = 14.f;
constexpr float DRAG = 0.45f;
constexpr float BRAKE = 42.f;
float bend(float z) { return std::sin(z * 0.011f) * 1.35f; }

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

int Game::marker() const { return phase_; }

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.85f);
    sys.apu.setEcho(0.18f, 0.25f, 0.12f);
    showTitle();
}

void Game::showTitle() {
    mode_ = Mode::Title;
    phase_ = 0;
    over_ = false;
    won_ = false;
    t_ = 0;
    race_ = 0;
    x_ = -0.35f;
    vx_ = 0;
    z_ = 0;
    vel_ = 0;
    hold_ = 0;
    lean_ = 0;
    why_[0] = 0;
    chimeStep_ = -1;
}

void Game::startRun() {
    mode_ = Mode::Run;
    phase_ = 1;
    race_ = 0;
    x_ = -0.35f;
    vx_ = 0;
    z_ = 8.f;
    vel_ = 6.f;
    hold_ = 0;
    lean_ = 0;
    brakeIn_ = 0;
    steerIn_ = 0;
    won_ = false;
    over_ = false;
    why_[0] = 0;
    blip(660.f);
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.08f);
    toneT_ = 0.08f;
}

void Game::chime() { chimeStep_ = 0; chimeT_ = 0; }

void Game::fail(const char* why) {
    std::snprintf(why_, sizeof why_, "%s", why);
    mode_ = Mode::Fail;
    phase_ = 4;
    over_ = true;
    won_ = false;
    sys_->apu.noiseBurst(0.4f, 180.f, 0.35f);
}

void Game::win() {
    mode_ = Mode::Win;
    phase_ = 4;
    over_ = true;
    won_ = true;
    std::snprintf(why_, sizeof why_, "SET DOWN");
    chime();
}

void Game::controls(float& steer, float& brake) {
    const gs::Pad& p = sys_->pad;
    float ax = p.axisX;
    if (p.down(gs::BTN_LEFT)) ax -= 1;
    if (p.down(gs::BTN_RIGHT)) ax += 1;
    steer = clampf(ax, -1.f, 1.f);
    brake = (p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.down(gs::BTN_A) || p.brake > 0.2f) ? 1.f : 0.f;
    if (p.down(gs::BTN_UP)) brake = 0;
}

void Game::pilot(float& steer, float& brake) {
    const float z0 = MARK_Z - MARK_LEN * 0.5f;
    const float aim = MARK_X;
    steer = clampf((aim - x_) * 3.4f - vx_ * 0.55f, -1.f, 1.f);
    float desired = 30.f;
    if (z_ > z0 - 70.f) desired = clampf((MARK_Z - z_) * 0.38f, 0.f, 30.f);
    if (z_ > z0 && std::fabs(x_ - aim) < MARK_HW && vel_ < 4.f) desired = 0.f;
    if (vel_ > desired + 0.35f) brake = 1.f;
    else if (vel_ < desired - 0.6f) brake = 0.f;
    else brake = 0.35f;
    if (z_ > MARK_Z + MARK_LEN * 0.5f) brake = 1.f;
}

void Game::physics(float steer, float brake) {
    steerIn_ += (steer - steerIn_) * 0.35f;
    brakeIn_ += (brake - brakeIn_) * 0.4f;
    float accel = COAST - DRAG * vel_ - brakeIn_ * BRAKE;
    vel_ += accel * DT;
    if (vel_ < 0) vel_ = 0;
    if (vel_ > 36.f) vel_ = 36.f;
    z_ += vel_ * DT;
    float lat = steerIn_ * (2.1f + vel_ * 0.045f);
    vx_ += (lat - vx_) * 0.18f;
    x_ += vx_ * DT;
    lean_ += (steerIn_ - lean_) * 0.2f;

    const float wall = ROAD_HW - 0.42f;
    if (x_ > wall) {
        x_ = wall;
        vx_ = -std::fabs(vx_) * 0.2f;
        vel_ *= 0.96f;
        shake_ = 1.f;
    } else if (x_ < -wall) {
        x_ = -wall;
        vx_ = std::fabs(vx_) * 0.2f;
        vel_ *= 0.96f;
        shake_ = 1.f;
    }
    if (shake_ > 0) shake_ -= DT;
}

void Game::judge() {
    const float z0 = MARK_Z - MARK_LEN * 0.5f;
    const float z1 = MARK_Z + MARK_LEN * 0.5f;
    const bool over = z_ >= z0 && z_ <= z1 && std::fabs(x_ - MARK_X) <= MARK_HW;
    if (over && vel_ < 1.15f) {
        hold_ += DT;
        phase_ = hold_ > 0.05f ? 3 : 2;
    } else if (over) {
        hold_ = 0;
        phase_ = 2;
    } else {
        hold_ = 0;
        phase_ = 1;
    }
    if (hold_ >= 0.45f) {
        win();
        return;
    }
    if (race_ >= limit_) {
        fail("THE OTHER CREW HAS THE MARK");
        return;
    }
    if (z_ > z1 + 6.f) {
        fail("PAST THE MARK");
        return;
    }
}

void Game::audio() {
    if (toneT_ > 0) {
        toneT_ -= DT;
        if (toneT_ <= 0) sys_->apu.tone(0, 0, 0);
    }
    if (mode_ == Mode::Run) {
        float wind = 0.02f + vel_ * 0.004f;
        sys_->apu.noise(wind, 1400.f + vel_ * 30.f, false);
        if (brakeIn_ > 0.5f && vel_ > 2.f) sys_->apu.tone(1, 180.f + vel_ * 4.f, 0.04f);
        else sys_->apu.tone(1, 0, 0);
    } else {
        sys_->apu.noise(0, 0, false);
        sys_->apu.tone(1, 0, 0);
    }
    if (chimeStep_ >= 0) {
        chimeT_ -= DT;
        if (chimeT_ <= 0) {
            static const float notes[] = {523.f, 659.f, 784.f, 1046.f};
            if (chimeStep_ < 4) {
                sys_->apu.tone(2, notes[chimeStep_], 0.12f);
                chimeT_ = 0.12f;
                chimeStep_++;
            } else {
                sys_->apu.tone(2, 0, 0);
                chimeStep_ = -1;
            }
        }
    }
}

bool Game::project(float wz, float wx, float& sx, float& sy, float& scale) const {
    float depth = wz - z_;
    if (depth < 1.4f) return false;
    float py = CAM_H * FOCAL / depth;
    sy = HORIZON + py;
    scale = FOCAL / depth;
    float dx = bend(wz) - bend(z_);
    sx = 160.f + (wx - x_ + dx) * scale;
    return sy > HORIZON - 2.f && sy < 230.f;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow, bool hflip) {
    if (h < 1.5f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.x = int16_t(std::lround(cx - w * 0.5f));
    s.y = int16_t(std::lround(cy - h * 0.5f));
    s.w = int16_t(std::max(1, int(std::lround(w))));
    s.h = int16_t(std::max(1, int(std::lround(h))));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = hflip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::drawRoad() {
    gs::VDP& v = sys_->vdp;
    const float camZ = z_;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float skyT = float(y) / HORIZON;
        if (y < int(HORIZON)) {
            int r = int(4 + 7 * skyT);
            int g = int(6 + 7 * skyT);
            int b = int(10 + 5 * skyT);
            v.lineBackdrop[y] = gs::rgb4(r, g, b);
            v.lineFog[y] = uint8_t(y < 40 ? 8 : 3);
            v.road[y].on = false;
            continue;
        }
        float py = float(y) - HORIZON + 0.5f;
        float depth = CAM_H * FOCAL / py;
        float wz = camZ + depth;
        float scale = FOCAL / depth;
        float dx = bend(wz) - bend(camZ);
        gs::RoadLine line;
        line.on = true;
        line.cx = 160.f + (dx - x_) * scale + (shake_ > 0 ? std::sin(t_ * 40.f) * 2.f : 0.f);
        line.hw = ROAD_HW * scale;
        line.v = wz * 28.f;
        line.pal = PAL_FIELD;
        line.band = (int(wz) / 8) & 1;
        line.style = gs::ROAD_ICE;
        line.left = gs::GROUND_SNOWWALL;
        line.right = gs::GROUND_SNOWWALL;
        v.road[y] = line;
        v.lineBackdrop[y] = gs::rgb4(7, 9, 12);
        float fog = clampf((depth - 40.f) / 50.f, 0.f, 1.f);
        v.lineFog[y] = uint8_t(fog * 10.f);
    }
    v.roadTime = int(t_ * 60.f);
}

void Game::drawWorld() {
    // Sprites: earlier entries sit on top, so the sled is submitted first.
    float sledH = 118.f;
    float leanPx = lean_ * 18.f;
    spr(art_.sled, 160.f + leanPx, 196.f, sledH * 0.55f, PAL_RIDER, true);
    spr(art_.sled, 160.f + leanPx, 188.f, sledH, PAL_RIDER, false, lean_ < -0.15f);
    spr(art_.rider, 160.f + leanPx * 0.4f, 156.f, 36.f, PAL_RIDER, false);

    if (brakeIn_ > 0.25f && vel_ > 1.f) {
        float puff = 10.f + brakeIn_ * 16.f;
        spr(art_.spray, 132.f, 200.f, puff, PAL_ICE);
        spr(art_.spray, 188.f, 198.f, puff * 0.8f, PAL_ICE, false, true);
    }

    const float z0 = MARK_Z - MARK_LEN * 0.5f;
    const float z1 = MARK_Z + MARK_LEN * 0.5f;
    for (float wz = z1; wz >= z0; wz -= 1.6f) {
        float sx0, sy0, sc0, sx1, sy1, sc1;
        if (!project(wz, MARK_X - MARK_HW, sx0, sy0, sc0)) continue;
        if (!project(wz, MARK_X + MARK_HW, sx1, sy1, sc1)) continue;
        float sxA, syA, scA;
        project(wz + 1.6f, MARK_X, sxA, syA, scA);
        float h = std::fabs(sy0 - syA);
        if (h < 1.f) h = 2.f;
        float cx = (sx0 + sx1) * 0.5f;
        float w = std::fabs(sx1 - sx0);
        gs::Sprite s;
        s.x = int16_t(std::lround(cx - w * 0.5f));
        s.y = int16_t(std::lround(sy0 - h * 0.5f));
        s.w = int16_t(std::max(1, int(std::lround(w))));
        s.h = int16_t(std::max(1, int(std::lround(h + 1.f))));
        s.img = art_.paint.pick(h);
        s.pal = PAL_MARK;
        sys_->vdp.sprite(s);
    }

    for (int i = 0; i < 14; i++) {
        float wz = 30.f + i * 26.f;
        float side = (i & 1) ? 1.f : -1.f;
        float sx, sy, sc;
        if (!project(wz, side * (ROAD_HW + 0.55f), sx, sy, sc)) continue;
        float h = 70.f * sc * 0.55f;
        spr(art_.tree, sx, sy - h * 0.35f, h, PAL_TREE);
    }
    float fsx, fsy, fsc;
    if (project(MARK_Z, MARK_X + MARK_HW + 0.35f, fsx, fsy, fsc)) {
        float h = 72.f * fsc * 0.42f;
        spr(art_.flag, fsx, fsy - h * 0.4f, h, PAL_CREW);
    }
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

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::drawHud() {
    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(4, "S3 LUGE MARK", PAL_HUD);
        hudC(8, "SET DOWN ON THE MARK", PAL_HUD);
        hudC(10, "THE CLOCK IS THE OTHER CREW", PAL_HUD);
        hudC(16, "LEFT RIGHT STEER", PAL_HUD);
        hudC(17, "DOWN OR B  BRAKE", PAL_HUD);
        hudC(22, "ENTER TO DROP", PAL_HUD);
        return;
    }
    float left = std::max(0.f, limit_ - race_);
    hud(1, 1, "CREW", left < 4.f ? PAL_MARK : PAL_ICE);
    std::snprintf(buf, sizeof buf, "%4.1f", left);
    hud(7, 1, buf, left < 4.f ? 2 : 1);
    std::snprintf(buf, sizeof buf, "YOU %4.1f", race_);
    hud(28, 1, buf, 3);
    if (mode_ == Mode::Run && phase_ >= 2) hudC(25, "ON THE MARK", 3);
    if (mode_ == Mode::Pause) hudC(14, "PAUSED", PAL_HUD);
    if (mode_ == Mode::Win) {
        hudC(12, "SET DOWN", 3);
        std::snprintf(buf, sizeof buf, "BEAT THE CREW BY %.1f", left);
        hudC(14, buf, 1);
        hudC(22, "ENTER  AGAIN", PAL_HUD);
    }
    if (mode_ == Mode::Fail) {
        hudC(12, why_, 2);
        hudC(22, "ENTER  AGAIN", PAL_HUD);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.A.clear();
    v.B.clear();
    v.HUD.clear();
    v.clearSprites();
    v.A.enabled = false;
    v.B.enabled = false;
    drawRoad();
    if (mode_ != Mode::Title) drawWorld();
    else {
        spr(art_.sled, 160.f, 150.f, 120.f, PAL_RIDER, true);
        spr(art_.sled, 160.f, 142.f, 130.f, PAL_RIDER);
        spr(art_.rider, 160.f, 108.f, 40.f, PAL_RIDER);
        spr(art_.flag, 250.f, 120.f, 70.f, PAL_CREW);
        spr(art_.paint, 160.f, 196.f, 22.f, PAL_MARK);
    }
    drawHud();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    const gs::Pad& p = sys.pad;
    if (mode_ == Mode::Title) {
        if (bot_ || p.pressed(gs::BTN_START) || p.pressed(gs::BTN_C)) startRun();
    } else if (mode_ == Mode::Pause) {
        if (p.pressed(gs::BTN_START)) mode_ = Mode::Run;
    } else if (mode_ == Mode::Run) {
        if (!bot_ && p.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else {
            float steer = 0, brake = 0;
            if (bot_) pilot(steer, brake);
            else controls(steer, brake);
            physics(steer, brake);
            race_ += DT;
            judge();
        }
    } else if (!bot_ && (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_C))) {
        showTitle();
        startRun();
    }
    if (hornT_ > 0) hornT_ -= DT;
    audio();
    draw();
}

}  // namespace luge

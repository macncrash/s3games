#include "game/slip.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace slip {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr float TIDE = 42.0f;
constexpr float SURF = 52.0f;
constexpr float HALF_L = 36.0f;
constexpr float HALF_H = 12.0f;
constexpr float MOUTH = 760.0f;
constexpr float BACK = 940.0f;
constexpr float BERTH_X = 850.0f;
constexpr float BERTH_Y = 86.0f;
constexpr float ROOF = 46.0f;
constexpr float FLOOR = 118.0f;
constexpr float BED = 156.0f;

}  // namespace

int Game::tideLeft() const {
    int s = int(std::ceil(TIDE - t_));
    return std::max(0, s);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    for (int y = 0; y < gs::SCREEN_H; y++) sys.vdp.road[y].on = false;
    if (bot_) beginRun();
    else mode_ = Mode::Title;
}

void Game::beginRun() {
    mode_ = Mode::Run;
    over_ = false;
    won_ = false;
    hull_ = 3;
    hold_ = 0;
    flash_ = 0;
    t_ = 0;
    x_ = 70;
    y_ = 102;
    vx_ = vy_ = 0;
    thrust_ = plane_ = 0;
    cam_ = 0;
    spin_ = 0;
    ping_ = 1.2f;
}

float Game::currentAt(float depth) const {
    float rip = std::clamp((78.0f - depth) / 78.0f, 0.0f, 1.0f);
    float u = std::clamp(t_ / TIDE, 0.0f, 1.35f);
    float flow = std::cos(std::min(u, 1.0f) * 3.14159265f);
    if (u > 1.0f) flow = -1.0f - (u - 1.0f) * 1.4f;
    return flow * 1.35f * rip;
}

void Game::readHelm(float& thrust, float& plane) {
    const gs::Pad& pad = sys_->pad;
    if (bot_) {
        float aimY = (x_ > MOUTH - 140.0f) ? BERTH_Y : 108.0f;
        float dx = BERTH_X - x_;
        float dy = aimY - y_;
        float want = std::clamp(dx * 0.025f, -0.55f, 1.35f);
        if (std::fabs(dx) < 48.0f) want = std::clamp(dx * 0.05f, -0.45f, 0.45f);
        thrust = std::clamp((want - vx_) * 1.4f, -1.0f, 1.0f);
        plane = std::clamp(dy * 0.15f - vy_ * 1.6f, -1.0f, 1.0f);
        return;
    }
    thrust = 0;
    plane = 0;
    if (pad.down(gs::BTN_RIGHT)) thrust += 1;
    if (pad.down(gs::BTN_LEFT)) thrust -= 1;
    thrust += pad.axisX;
    if (pad.accel > 0.15f) thrust += pad.accel;
    if (pad.brake > 0.15f) thrust -= pad.brake;
    if (pad.down(gs::BTN_DOWN)) plane += 1;
    if (pad.down(gs::BTN_UP)) plane -= 1;
    plane -= pad.axisY;
    thrust = std::clamp(thrust, -1.0f, 1.0f);
    plane = std::clamp(plane, -1.0f, 1.0f);
}

void Game::collide() {
    auto bump = [&](float speed) {
        if (speed > 0.55f) {
            hull_ -= 1;
            flash_ = 18;
            sys_->apu.noiseBurst(0.4f, 240.0f, 0.2f);
            if (hull_ <= 0) {
                over_ = true;
                won_ = false;
                mode_ = Mode::Over;
            }
        }
    };

    if (y_ + HALF_H > BED) {
        bump(std::fabs(vy_) + 0.2f);
        y_ = BED - HALF_H;
        vy_ = std::min(vy_, 0.0f) * -0.2f;
    }
    bool inPocket = x_ + HALF_L > MOUTH;
    if (inPocket && y_ - HALF_H < ROOF) {
        bump(std::fabs(vy_));
        y_ = ROOF + HALF_H;
        vy_ = std::max(vy_, 0.0f);
    }
    if (inPocket && y_ + HALF_H > FLOOR) {
        bump(std::fabs(vy_));
        y_ = FLOOR - HALF_H;
        vy_ = std::min(vy_, 0.0f);
    }
    if (x_ + HALF_L > BACK) {
        bump(std::fabs(vx_));
        x_ = BACK - HALF_L;
        vx_ = std::min(vx_, 0.0f) * -0.15f;
    }
    if (x_ - HALF_L < 8) {
        x_ = 8 + HALF_L;
        vx_ = std::max(vx_, 0.0f);
    }
}

void Game::stepRun() {
    float wantT = 0, wantP = 0;
    readHelm(wantT, wantP);
    thrust_ += (wantT - thrust_) * 0.2f;
    plane_ += (wantP - plane_) * 0.25f;

    vx_ += thrust_ * 0.05f;
    vy_ += plane_ * 0.045f;
    vx_ += currentAt(y_) * 0.045f;
    vy_ += (100.0f - y_) * 0.00035f;
    vx_ *= 0.986f;
    vy_ *= 0.90f;
    x_ += vx_;
    y_ += vy_;
    spin_ += std::fabs(vx_) * 0.35f + 0.04f;
    collide();

    bool inSlip = x_ > MOUTH + HALF_L + 6 && x_ < BACK - HALF_L - 10 && y_ > ROOF + HALF_H + 6 &&
                  y_ < FLOOR - HALF_H - 6 && std::fabs(vx_) < 0.32f && std::fabs(vy_) < 0.28f;
    if (inSlip) {
        if (++hold_ == 1) sys_->apu.tone(1, 660.0f, 0.08f);
        if (hold_ > 36 && !over_) {
            won_ = true;
            over_ = true;
            mode_ = Mode::Over;
            sys_->apu.tone(0, 523.0f, 0.12f);
            sys_->apu.tone(1, 784.0f, 0.1f);
        }
    } else {
        hold_ = 0;
    }

    t_ += DT;
    if (!over_ && t_ >= TIDE) {
        over_ = true;
        won_ = false;
        mode_ = Mode::Over;
        sys_->apu.noiseBurst(0.35f, 120.0f, 0.4f);
    }

    ping_ -= DT;
    if (ping_ <= 0 && !over_) {
        ping_ = 1.35f;
        sys_->apu.tone(2, 420.0f, 0.04f);
    }
    if (mode_ == Mode::Run) {
        float eng = 70.0f + std::fabs(thrust_) * 40.0f;
        sys_->apu.tone(0, eng, 0.03f + std::fabs(thrust_) * 0.03f);
    }
}

void Game::sprite(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 1.0f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 40 || s.y > gs::SCREEN_H + 20 || s.x + s.w < -40 || s.y + s.h < -20) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || row < 0 || row > 27 || c < 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();

    float tideU = std::clamp(t_ / TIDE, 0.0f, 1.0f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < int(SURF)) {
            float k = y / SURF;
            int r = int(4 + (10 - 4) * (1 - k) * (1 - tideU) + tideU * 2);
            int g = int(6 + 4 * (1 - k));
            int b = int(10 + 4 * (1 - k) - tideU * 3);
            vdp.lineBackdrop[y] = gs::rgb4(std::clamp(r, 0, 15), std::clamp(g, 0, 15), std::clamp(b, 0, 15));
        } else {
            float k = (y - SURF) / (gs::SCREEN_H - SURF);
            vdp.lineBackdrop[y] = gs::rgb4(1, int(6 - 4 * k), int(10 - 6 * k));
        }
        vdp.lineFog[y] = 0;
    }

    auto world = [&](float wx, float depth) {
        return std::pair<float, float>(wx - cam_, SURF + depth);
    };

    // Skyline sits on the horizon, scrolling slower than the boat.
    {
        gs::Sprite s;
        s.img = art_.skyline;
        s.w = 256;
        s.h = 48;
        s.x = int16_t(-int(cam_ * 0.25f) % 256);
        s.y = int16_t(SURF - 48);
        s.pal = PAL_SEA;
        vdp.sprite(s);
        s.x = int16_t(s.x + 256);
        vdp.sprite(s);
        s.x = int16_t(s.x + 256);
        vdp.sprite(s);
    }

    for (int i = 0; i < 5; i++) {
        float wx = 40.0f + i * 180.0f;
        auto p = world(wx, 150);
        sprite(art_.kelp, p.first, p.second - 8, 36 + (i % 3) * 6, PAL_KELP);
    }

    // Quay deck and the pocket it roofs.
    for (float wx = MOUTH - 20; wx < BACK + 80; wx += 90) {
        auto p = world(wx + 48, ROOF - 8);
        sprite(art_.quay, p.first, p.second, 32, PAL_QUAY);
    }
    for (int i = 0; i < 4; i++) {
        float wx = MOUTH + 20 + i * 48.0f;
        auto p = world(wx, -6);
        sprite(art_.piling, p.first, p.second + 20, 64, PAL_QUAY);
    }
    {
        auto p = world(MOUTH - 30, -4);
        sprite(art_.buoy, p.first, p.second + 6 + std::sin(t_ * 2.0f) * 2.0f, 26, PAL_BUOY);
        auto q = world(BACK + 24, -18);
        sprite(art_.lamp, q.first, q.second, 30, PAL_LAMP);
    }

    // Gulls over the flood.
    for (int i = 0; i < 3; i++) {
        float gx = std::fmod(20.0f + i * 110.0f + t_ * (18.0f + i * 6), 380.0f) - 20.0f;
        sprite(art_.gull, gx, 16 + (i % 2) * 8 + std::sin(t_ * 3 + i) * 2, 8, PAL_FX);
    }

    auto boat = world(x_, y_);
    bool flipProp = int(spin_) % 2 == 0;
    sprite(art_.prop, boat.first - 40, boat.second + 2, 12, PAL_SUB, flipProp);
    int subPal = (flash_ > 0 && (flash_ / 3) % 2 == 0) ? PAL_BUOY : PAL_SUB;
    sprite(art_.sub, boat.first, boat.second, 36, subPal, vx_ < -0.15f);

    for (int i = 0; i < 6; i++) {
        float phase = std::fmod(t_ * 28.0f + i * 17.0f, 48.0f);
        float bx = boat.first - 34 - phase;
        float by = boat.second + 6 + std::sin(i + t_ * 4) * 3;
        sprite(art_.bubble, bx, by, 4 + (i % 3), PAL_FX);
    }

    // Foam line.
    for (int x = 0; x < gs::SCREEN_W; x += 18) {
        float wob = std::sin(t_ * 2.2f + x * 0.08f) * 2.0f;
        sprite(art_.bubble, float(x), SURF + wob, 5, PAL_SEA);
    }

    char buf[64];
    if (mode_ == Mode::Title) {
        hudC(3, "S3 SUBSLIP", PAL_HUD);
        hudC(6, "BERTH IN THE SLIP", PAL_HUD);
        hudC(8, "BEFORE THE TIDE TURNS", PAL_HUD);
        hudC(12, "RIGHT AHEAD   LEFT ASTERN", PAL_HUD);
        hudC(14, "UP SHALLOW   DOWN DEEP", PAL_HUD);
        hudC(18, "SLACK WATER RUNS DEEP", PAL_HUD);
        hudC(22, "PRESS START", PAL_HUD);
    } else {
        int left = tideLeft();
        std::snprintf(buf, sizeof buf, "TIDE %02d", left);
        hud(1, 1, buf, left <= 8 ? PAL_HUD : PAL_HUD);
        // palette index is the nametable pal; color 3 is red via set? HUD uses one pal.
        // Use pal PAL_HUD always; flash the row by blanking on odd frames when late.
        if (left <= 8 && (int(t_ * 4) % 2 == 0)) hud(1, 1, "TIDE    ", PAL_HUD);
        std::snprintf(buf, sizeof buf, "HULL %d", std::max(0, hull_));
        hud(30, 1, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "SPEED %+4.1f", vx_ * 10.0f);
        hud(1, 26, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "DEPTH %3.0f", y_);
        hud(28, 26, buf, PAL_HUD);
        if (hold_ > 0 && !over_) hudC(12, "HOLD HER", PAL_HUD);
        if (mode_ == Mode::Over && won_) {
            hudC(10, "BERTHED", PAL_HUD);
            std::snprintf(buf, sizeof buf, "%dS BEFORE THE TIDE", left);
            hudC(12, buf, PAL_HUD);
        } else if (mode_ == Mode::Over) {
            hudC(10, hull_ <= 0 ? "HULL GONE" : "TIDE TURNED", PAL_HUD);
            hudC(12, "SHE MISSED THE SLIP", PAL_HUD);
        }
    }
    if (flash_ > 0) flash_--;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (mode_ == Mode::Title) {
        t_ += DT;
        x_ = 120 + std::sin(t_ * 0.7f) * 10;
        y_ = 90 + std::sin(t_ * 0.5f) * 4;
        vx_ = 0.2f;
        cam_ = 40;
        spin_ += 0.15f;
        if (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A) || sys.pad.pressed(gs::BTN_C)) beginRun();
        draw();
        return;
    }
    if (mode_ == Mode::Run) stepRun();
    else {
        vx_ *= 0.96f;
        vy_ *= 0.9f;
        x_ += vx_;
        y_ += vy_;
        t_ += DT;
    }
    float wantCam = x_ - 120.0f;
    if (wantCam < 0) wantCam = 0;
    if (wantCam > BACK + 40 - gs::SCREEN_W) wantCam = BACK + 40 - gs::SCREEN_W;
    cam_ += (wantCam - cam_) * 0.08f;
    draw();
}

}  // namespace slip

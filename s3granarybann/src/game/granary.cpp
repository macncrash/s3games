#include "game/granary.h"

#include <algorithm>
#include <cmath>

namespace granary {
namespace {
constexpr float BANNER_X = 276;
constexpr float BANNER_Y = 158;
constexpr float DOOR_X = 86;
constexpr float DOOR_Y = 156;
}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (won_ || mode_ == Mode::Victory) return 3;
    if (have_) return 2;
    return 1;
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

bool Game::barnBlocks(float x, float y) const {
    // The loft and walls. The door gap along the ground is open.
    if (x < 18 || x > 132 || y < 78 || y > 128) return false;
    return true;
}

void Game::layYard() {
    gs::Plane& b = sys_->vdp.B;
    b.clear();
    for (int cy = 0; cy < 32; cy++) {
        for (int cx = 0; cx < 64; cx++) {
            int y = cy * 8;
            if (y < 96) continue;
            bool path = (cy >= 17 && cy <= 22);
            b.set(cx, cy, gs::entry(path ? art_.dirt : art_.wheat, PAL_GROUND));
        }
    }
    sys_->vdp.A.clear();
    sys_->vdp.A.enabled = false;
    sys_->vdp.HUD.clear();
}

void Game::begin() {
    mode_ = Mode::Yard;
    have_ = false;
    won_ = false;
    over_ = false;
    px_ = 168;
    py_ = 156;
    face_ = 1;
    t_ = 0;
    hold_ = 0;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    layYard();
    mode_ = Mode::Title;
    have_ = false;
    won_ = false;
    over_ = false;
    px_ = 168;
    py_ = 156;
    sys.vdp.hudEnabled = true;
}

void Game::update() {
    const gs::Pad& pad = sys_->pad;
    float ax = 0, ay = 0;
    bool act = false;
    if (bot_) {
        if (mode_ == Mode::Title) {
            if (t_ > 0.35f) begin();
            return;
        }
        if (mode_ == Mode::Victory) return;
        float tx = have_ ? DOOR_X : BANNER_X;
        float ty = have_ ? DOOR_Y : BANNER_Y;
        if (px_ < tx - 1.5f) ax = 1;
        else if (px_ > tx + 1.5f) ax = -1;
        if (py_ < ty - 1.5f) ay = 1;
        else if (py_ > ty + 1.5f) ay = -1;
        float dx = tx - px_, dy = ty - py_;
        if (!have_ && dx * dx + dy * dy < 16 * 16) act = true;
    } else {
        if (mode_ == Mode::Title) {
            if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.anyPressed()) begin();
            return;
        }
        if (mode_ == Mode::Victory) return;
        if (pad.down(gs::BTN_LEFT)) ax -= 1;
        if (pad.down(gs::BTN_RIGHT)) ax += 1;
        if (pad.down(gs::BTN_UP)) ay -= 1;
        if (pad.down(gs::BTN_DOWN)) ay += 1;
        if (std::fabs(pad.axisX) > 0.2f) ax = pad.axisX;
        if (std::fabs(pad.axisY) > 0.2f) ay = -pad.axisY;
        act = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B);
    }

    float mag = std::sqrt(ax * ax + ay * ay);
    if (mag > 1) {
        ax /= mag;
        ay /= mag;
    }
    const float speed = 78.0f / 60.0f;
    float nx = px_ + ax * speed;
    float ny = py_ + ay * speed;
    nx = std::clamp(nx, 16.0f, 304.0f);
    ny = std::clamp(ny, 118.0f, 210.0f);
    if (!barnBlocks(nx, py_)) px_ = nx;
    if (!barnBlocks(px_, ny)) py_ = ny;
    if (ax < -0.2f) face_ = -1;
    else if (ax > 0.2f) face_ = 1;
    if (mag > 0.2f) step_ += 0.18f;

    auto near = [&](float x, float y) {
        float dx = px_ - x, dy = py_ - y;
        return dx * dx + dy * dy < 18.0f * 18.0f;
    };
    if (!have_ && act && near(BANNER_X, BANNER_Y)) {
        have_ = true;
        sys_->apu.tone(0, 660.0f, 0.08f);
        sys_->apu.tone(1, 880.0f, 0.06f);
    }
    if (have_ && near(DOOR_X, DOOR_Y) && (act || bot_)) {
        have_ = false;
        won_ = true;
        mode_ = Mode::Victory;
        hold_ = 0;
        sys_->apu.tone(0, 523.0f, 0.1f);
        sys_->apu.tone(1, 659.0f, 0.1f);
        sys_->apu.tone(2, 784.0f, 0.08f);
    }
}

void Game::draw() {
    sys_->vdp.clearSprites();
    sys_->vdp.HUD.clear();

    spr(art_.barn, 74, 168, 132, PAL_BARN, false, true);
    spr(art_.sack, 150, 176, 22, PAL_YARD, false, true);

    float crowX = std::fmod(t_ * 36.0f, 380.0f) - 30.0f;
    spr(art_.crow, crowX, 48 + std::sin(t_ * 3.0f) * 6.0f, 16, PAL_CROW, crowX > 160, false);

    if (!have_ && mode_ != Mode::Victory) spr(art_.banner, BANNER_X, BANNER_Y, 58, PAL_BANN, false, true);
    if (mode_ == Mode::Victory) spr(art_.banner, 58, 132, 52, PAL_BANN, false, true);

    int fr = int(step_) & 1;
    bool showFarmer = mode_ != Mode::Title;
    if (showFarmer) {
        spr(art_.farmer[fr], px_, py_, 52, PAL_FARM, face_ < 0, true);
        if (have_) spr(art_.banner, px_ + face_ * 10.0f, py_ - 36.0f, 40, PAL_BANN, face_ < 0, false);
    } else {
        spr(art_.farmer[0], 168, 168, 52, PAL_FARM, false, true);
        spr(art_.banner, BANNER_X, BANNER_Y, 58, PAL_BANN, false, true);
    }

    if (mode_ == Mode::Title) {
        hudC(2, "S3 GRANARY", PAL_HUD);
        hudC(4, "ONE GRANARY", PAL_HUD);
        hudC(6, "BRING THE BANNER BACK", PAL_HUD);
        hudC(24, "ARROWS MOVE   A TAKES IT", PAL_HUD);
        if (int(t_ * 2) & 1) hudC(26, "PRESS START", PAL_HUD);
    } else if (mode_ == Mode::Victory) {
        hudC(2, "THE BANNER IS HOME", PAL_HUD);
        hudC(4, "IT IS DONE", PAL_HUD);
    } else if (have_) {
        hudC(2, "BRING IT TO THE DOOR", PAL_HUD);
    } else {
        hudC(2, "THE BANNER IS IN THE YARD", PAL_HUD);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += 1.0f / 60.0f;
    frameN_++;
    if (mode_ != Mode::Victory) update();
    else {
        hold_ += 1.0f / 60.0f;
        if (hold_ > 0.7f) over_ = true;
        if (int(hold_ * 4) == 1) {
            sys.apu.tone(0, 523.0f, 0.05f);
            sys.apu.tone(1, 784.0f, 0.05f);
        }
    }
    // Quiet the square waves after a short attack so they do not drone.
    if (mode_ != Mode::Victory && std::fmod(t_, 0.12f) > 0.08f) {
        sys.apu.tone(0, 0, 0);
        sys.apu.tone(1, 0, 0);
        sys.apu.tone(2, 0, 0);
    }
    draw();
}

}  // namespace granary

#include "game/keel.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

#include "version.h"

namespace keel {

constexpr float GRASS0 = 460.f;
constexpr float GRASS1 = 980.f;
constexpr float WALL = 1000.f;
constexpr float SURF = 132.f;

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(6, 10, 8));
    sys.apu.setMaster(0.5f);
    resetRun();
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
}

void Game::resetRun() {
    x_ = 70.f;
    vx_ = 1.6f;
    keel_ = 0.12f;
    bob_ = 0;
    t_ = 0;
    stillG_ = 0;
    stillW_ = 0;
    touched_ = false;
    dug_ = false;
    why_.clear();
    hold_ = 0;
}

bool Game::onGrass() const { return x_ >= GRASS0 && x_ <= GRASS1; }
bool Game::inWater() const { return x_ < GRASS0; }

void Game::bot(bool& thrustF, bool& thrustB, bool& keelDn, bool& keelUp, bool& start) {
    thrustF = thrustB = keelDn = keelUp = start = false;
    if (mode_ == Mode::Title) {
        start = true;
        return;
    }
    if (mode_ != Mode::Sail) return;
    if (x_ < GRASS0 + 50.f) {
        keelUp = keel_ > 0.16f;
        if (vx_ < 2.55f) thrustF = true;
        else if (vx_ > 2.9f) thrustB = true;
        return;
    }
    keelDn = keel_ < 0.58f;
    keelUp = keel_ > 0.66f;
    if (vx_ > 0.55f) thrustB = true;
    else if (vx_ > 0.12f) thrustB = (int(t_ * 60) & 1) == 0;
    else if (vx_ < -0.08f) thrustF = true;
}

void Game::physics() {
    bool thrustF = false, thrustB = false, keelDn = false, keelUp = false, start = false;
    const gs::Pad& p = sys_->pad;
    if (bot_) bot(thrustF, thrustB, keelDn, keelUp, start);
    else {
        thrustF = p.down(gs::BTN_RIGHT) || p.accel > 0.2f;
        thrustB = p.down(gs::BTN_LEFT) || p.brake > 0.2f;
        keelDn = p.down(gs::BTN_DOWN) || p.down(gs::BTN_B);
        keelUp = p.down(gs::BTN_UP) || p.down(gs::BTN_A);
        start = p.pressed(gs::BTN_START) || p.pressed(gs::BTN_C);
    }

    if (mode_ == Mode::Title) {
        bob_ += 0.04f;
        if (start) {
            resetRun();
            mode_ = Mode::Sail;
            sys_->apu.tone(0, 440.f, 0.08f);
            beep_ = 0.06f;
        }
        return;
    }
    if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        bob_ += 0.02f;
        if (++hold_ > 90) {
            over_ = true;
            if (!bot_ && start) {
                over_ = false;
                won_ = false;
                resetRun();
                mode_ = Mode::Sail;
            }
        }
        return;
    }

    float target = keel_;
    if (keelDn) target = 1.f;
    else if (keelUp) target = 0.08f;
    keel_ += (target - keel_) * 0.12f;

    float thrust = 0;
    if (thrustF) thrust += 0.045f;
    if (thrustB) thrust -= 0.055f;
    float drag = inWater() ? 0.006f : (0.010f + 0.042f * keel_);
    if (x_ > GRASS1) drag = 0.02f;
    float ax = thrust;
    if (std::fabs(vx_) > 0.0001f) ax -= std::copysign(drag, vx_);
    else vx_ = 0;
    vx_ += ax;
    vx_ = std::clamp(vx_, -2.4f, 4.4f);
    if (std::fabs(vx_) < drag) vx_ = 0;
    x_ += vx_;
    t_ += 1.f / 60.f;
    bob_ += inWater() ? 0.08f : 0.03f;

    if (onGrass()) touched_ = true;

    if (onGrass() && keel_ > 0.86f && vx_ > 3.55f) {
        dug_ = true;
        mode_ = Mode::Fail;
        why_ = "THE KEEL DUG IN";
        won_ = false;
        sys_->apu.noiseBurst(0.5f, 400.f, 0.3f);
        return;
    }
    if (x_ >= WALL) {
        x_ = WALL;
        vx_ = 0;
        mode_ = Mode::Fail;
        why_ = "HIT THE STONE BANK";
        won_ = false;
        sys_->apu.noiseBurst(0.45f, 220.f, 0.35f);
        return;
    }
    if (x_ < 8.f) {
        x_ = 8.f;
        vx_ = 0;
        mode_ = Mode::Fail;
        why_ = "SLID BACK TO SEA";
        won_ = false;
        return;
    }
    if (t_ > 28.f) {
        mode_ = Mode::Fail;
        why_ = "THE TIDE RAN OUT";
        won_ = false;
        return;
    }

    if (std::fabs(vx_) < 0.035f && onGrass() && touched_) {
        stillG_ += 1.f / 60.f;
        if (stillG_ > 0.45f) {
            vx_ = 0;
            mode_ = Mode::Win;
            won_ = true;
            why_ = "FULL STOP";
            sys_->apu.tone(1, 523.f, 0.1f);
            beep_ = 0.12f;
        }
    } else {
        stillG_ = 0;
    }
    if (std::fabs(vx_) < 0.035f && inWater() && t_ > 4.f) {
        stillW_ += 1.f / 60.f;
        if (stillW_ > 0.7f) {
            mode_ = Mode::Fail;
            why_ = "STILL AFLOAT";
            won_ = false;
        }
    } else {
        stillW_ = 0;
    }
}

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t c;
        if (y < 118) {
            float u = y / 118.f;
            int r = int(4 + u * 7);
            int g = int(7 + u * 5);
            int b = int(13 - u * 2);
            c = gs::rgb4(r, g, b);
        } else if (y < 132) {
            c = gs::rgb4(8, 12, 6);
        } else {
            c = gs::rgb4(1, 5, 10);
        }
        v.lineBackdrop[y] = c;
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 400));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 400));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
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

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    sky();

    float cam = x_ - 120.f;
    int col0 = int(std::floor(cam / 48.f)) - 1;
    for (int i = 0; i < 10; i++) {
        float wx = float(col0 + i) * 48.f;
        float sx = wx - cam + 24.f;
        if (wx >= WALL - 10.f) spr(art_.stone, sx, SURF + 10.f, 80.f, PAL_STONE);
        else if (wx + 48.f > GRASS0 && wx < GRASS1 + 20.f) spr(art_.grass, sx, SURF + 6.f, 72.f, PAL_GRASS);
        else spr(art_.water, sx, SURF + 16.f, 64.f, PAL_WATER);
    }

    float by = SURF - 18.f + std::sin(bob_) * (inWater() || mode_ == Mode::Title ? 2.4f : 0.4f);
    float keelDrop = 10.f + keel_ * 16.f;
    if (mode_ == Mode::Title) {
        by = SURF - 18.f + std::sin(bob_) * 2.4f;
        keelDrop = 14.f;
    }
    float bx = 120.f;
    if (std::fabs(vx_) > 0.4f) {
        float sprayX = bx - 36.f * (vx_ > 0 ? 1.f : -1.f);
        spr(art_.spray, sprayX, SURF - 2.f, 14.f + std::fabs(vx_) * 2.f, PAL_SPRAY);
    }
    spr(art_.keel, bx - 6.f, by + keelDrop, 28.f + keel_ * 8.f, PAL_BOAT);
    spr(art_.boat, bx, by, 52.f, PAL_BOAT);
    spr(art_.flag, bx + 8.f, by - 28.f, 12.f, PAL_BOAT);

    hud(1, 1, "S3 KEELGRASS", PAL_HUD);
    if (mode_ == Mode::Title) {
        hudC(8, "LAND THE KEEL", PAL_HUD);
        hudC(10, "ON THE GRASS", PAL_HUD);
        hudC(12, "AND STOP DEAD", PAL_HUD);
        hudC(16, "RIGHT DRIVE   LEFT REVERSE", PAL_HUD);
        hudC(17, "DOWN DROP KEEL   UP LIFT", PAL_HUD);
        hudC(20, "START TO LAUNCH", PAL_HUD);
        hudC(26, S3_VERSION_STRING, PAL_HUD);
        return;
    }

    char buf[48];
    std::snprintf(buf, sizeof(buf), "SPEED %4.2f", std::fabs(vx_));
    hud(1, 3, buf, std::fabs(vx_) < 0.035f ? PAL_HUD : PAL_HUD);
    std::snprintf(buf, sizeof(buf), "KEEL %s", keel_ > 0.7f ? "DEEP" : keel_ > 0.35f ? "SET" : "UP");
    hud(22, 3, buf, PAL_HUD);
    if (onGrass()) hud(1, 5, "ON THE GRASS", PAL_HUD);
    else if (inWater()) hud(1, 5, "IN THE WATER", PAL_HUD);
    else hud(1, 5, "OFF THE BANK", PAL_HUD);

    int bar = int(std::clamp(std::fabs(vx_) / 4.4f, 0.f, 1.f) * 18);
    std::string marks(18, '.');
    for (int i = 0; i < bar; i++) marks[i] = '#';
    hud(1, 24, marks, PAL_HUD);
    hud(1, 25, "STOP WHEN THE BAR IS EMPTY", PAL_HUD);

    if (mode_ == Mode::Win) {
        hudC(10, "FULL STOP", PAL_HUD);
        hudC(12, "THE KEEL HOLDS THE GRASS", PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        hudC(10, why_.c_str(), PAL_HUD);
        hudC(12, "THE KEEL DID NOT STOP", PAL_HUD);
        if (!bot_) hudC(16, "START TO TRY AGAIN", PAL_HUD);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (beep_ > 0) {
        beep_ -= 1.f / 60.f;
        if (beep_ <= 0) {
            sys.apu.tone(0, 0, 0);
            sys.apu.tone(1, 0, 0);
        }
    }
    physics();
    draw();
}

}  // namespace keel

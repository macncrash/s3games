#include "game/ladd.h"

#include <cmath>
#include <cstdio>
#include <string>

namespace towerladd {
namespace {

struct Plat {
    float x0, x1, y;
};

struct Lad {
    float x, y0, y1;
};

// The near keep, a mid wall, then the far tower. Two open drops.
// The far ladder is the only way the watch ends.
const Plat kPlat[] = {
    {0, 260, 158},
    {316, 620, 158},
    {676, 1120, 158},
};
const Lad kLad[] = {
    {980, 28, 158},
};
constexpr int kFar = 0;
constexpr float kWorld = 1180;

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    paintKeep();
    sys.vdp.A.enabled = false;
    sys.vdp.hudEnabled = false;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int r = 2 + (90 - y) / 18;
        int b = 6 + (40 - y) / 22;
        if (r < 1) r = 1;
        if (r > 6) r = 6;
        if (b < 3) b = 3;
        if (b > 10) b = 10;
        sys.vdp.lineBackdrop[y] = gs::rgb4(r, r, b);
        sys.vdp.road[y].on = false;
        sys.vdp.lineFog[y] = 0;
    }
    sys.vdp.setFogColor(gs::rgb4(2, 2, 4));
    if (bot_) begin();
}

void Game::paintKeep() {
    gs::VDP& v = sys_->vdp;
    v.B.resize(256, 32);
    v.B.clear();
    for (int cy = 0; cy < 28; cy++) {
        for (int cx = 0; cx < 160; cx++) {
            if (cy < 18) continue;
            int tile = 1 + ((cx * 5 + cy * 3) & 3);
            v.B.set(cx, cy, gs::entry(tile, PAL_STONE));
        }
    }
}

void Game::begin() {
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
    reason_ = "";
    px_ = 48;
    py_ = 158;
    vx_ = vy_ = 0;
    face_ = 1;
    grounded_ = true;
    onLadder_ = false;
    t_ = 0;
    coyote_ = 0;
    jumpBuf_ = 0;
    step_ = foot_ = climbSnd_ = beep_ = 0;
    fan_ = -1;
    botPhase_ = 0;
    cam_ = 0;
}

void Game::win() {
    if (won_) return;
    won_ = true;
    over_ = true;
    mode_ = Mode::Won;
    reason_ = "FAR LADDER";
    fan_ = 0;
    fanT_ = 0;
    vx_ = vy_ = 0;
    onLadder_ = true;
}

void Game::lose(const char* why) {
    if (mode_ == Mode::Won || mode_ == Mode::Lost) return;
    won_ = false;
    over_ = true;
    mode_ = Mode::Lost;
    reason_ = why;
    fan_ = 0;
    fanT_ = 0;
    vx_ = 0;
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Won || mode_ == Mode::Lost) return 4;
    if (px_ > 760) return 3;
    if (px_ > 300) return 2;
    return 1;
}

int Game::nearLadder() const {
    const Lad& L = kLad[kFar];
    if (std::fabs(px_ - L.x) < 18.0f && py_ >= L.y0 - 8.0f && py_ <= L.y1 + 10.0f) return kFar;
    return -1;
}

const gs::Mipped& Game::heroSprite() const {
    if (onLadder_) return (int(t_ * 6) & 1) ? art_.climbA : art_.climbB;
    if (!grounded_) return art_.jump;
    if (std::fabs(vx_) > 8) return (int(foot_) & 1) ? art_.walkA : art_.walkB;
    return art_.stand;
}

void Game::bot(bool& left, bool& right, bool& up, bool& down, bool& jump) const {
    left = right = up = down = jump = false;
    if (mode_ != Mode::Play) return;
    switch (botPhase_) {
    case 0:
        right = true;
        if (grounded_ && px_ >= 236.0f && px_ < 258.0f) jump = true;
        break;
    case 1:
        right = true;
        if (grounded_ && px_ >= 592.0f && px_ < 616.0f) jump = true;
        break;
    case 2:
        if (px_ < 968.0f) right = true;
        else up = true;
        break;
    default:
        up = true;
        break;
    }
}

void Game::play(float dt, bool left, bool right, bool up, bool down, bool jumpPressed) {
    const float move = 112.0f;
    int want = -1;
    if (up || down || onLadder_) want = nearLadder();
    if (onLadder_ && want < 0) onLadder_ = false;

    if (!onLadder_ && (up || down) && want >= 0) {
        onLadder_ = true;
        vy_ = 0;
        vx_ = 0;
    }

    if (onLadder_) {
        const Lad& L = kLad[kFar];
        px_ += (L.x - px_) * std::min(1.0f, dt * 12.0f);
        float dir = (up ? -1.0f : 0.0f) + (down ? 1.0f : 0.0f);
        py_ += dir * 70.0f * dt;
        if (py_ > L.y1) {
            py_ = L.y1;
            onLadder_ = false;
            grounded_ = true;
        }
        if (py_ < L.y0) py_ = L.y0;
        if (py_ <= 46.0f) {
            win();
            return;
        }
        vx_ = 0;
        vy_ = 0;
        grounded_ = false;
        if (dir != 0) {
            climbSnd_ -= dt;
            if (climbSnd_ <= 0) {
                sys_->apu.tone(0, 210, 0.04f);
                beep_ = 0.05f;
                climbSnd_ = 0.16f;
            }
        }
    } else {
        float ax = (right ? 1.0f : 0.0f) - (left ? 1.0f : 0.0f);
        if (ax != 0) face_ = ax > 0 ? 1 : -1;
        vx_ = ax * move;
        vy_ += 740.0f * dt;
        if (vy_ > 380) vy_ = 380;
        px_ += vx_ * dt;
        py_ += vy_ * dt;
        if (px_ < 12) px_ = 12;
        if (px_ > kWorld - 20) px_ = kWorld - 20;

        bool land = false;
        if (vy_ >= 0) {
            for (const Plat& p : kPlat) {
                if (px_ >= p.x0 && px_ <= p.x1 && py_ >= p.y && py_ <= p.y + 16.0f) {
                    py_ = p.y;
                    vy_ = 0;
                    land = true;
                    break;
                }
            }
        }
        grounded_ = land;
        if (land) coyote_ = 0.12f;
        else coyote_ = std::max(0.0f, coyote_ - dt);
        if (jumpPressed) jumpBuf_ = 0.12f;
        else jumpBuf_ = std::max(0.0f, jumpBuf_ - dt);
        if (jumpBuf_ > 0 && coyote_ > 0) {
            vy_ = -310;
            grounded_ = false;
            coyote_ = 0;
            jumpBuf_ = 0;
            sys_->apu.tone(1, 280, 0.06f);
            beep_ = 0.06f;
        }
        if (land && std::fabs(vx_) > 8) {
            step_ -= dt;
            foot_ += dt * 8;
            if (step_ <= 0) {
                sys_->apu.noiseBurst(0.04f, 900, 0.04f);
                step_ = 0.24f;
            }
        }
    }

    if (py_ > 214.0f) {
        lose("OFF THE TOWER");
        return;
    }

    if (bot_) {
        if (botPhase_ == 0 && grounded_ && px_ > 330.0f) botPhase_ = 1;
        else if (botPhase_ == 1 && grounded_ && px_ > 690.0f) botPhase_ = 2;
        else if (botPhase_ == 2 && onLadder_) botPhase_ = 3;
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    float dt = 1.0f / 60.0f;
    gs::Pad& pad = sys.pad;
    bool left = pad.down(gs::BTN_LEFT) || pad.axisX < -0.3f;
    bool right = pad.down(gs::BTN_RIGHT) || pad.axisX > 0.3f;
    bool up = pad.down(gs::BTN_UP) || pad.axisY > 0.3f;
    bool down = pad.down(gs::BTN_DOWN) || pad.axisY < -0.3f;
    bool jump = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_B);
    bool start = pad.pressed(gs::BTN_START);

    if (mode_ == Mode::Title) {
        t_ += dt;
        if (bot_ || start || pad.pressed(gs::BTN_A) || pad.anyPressed()) begin();
    } else if (mode_ == Mode::Play) {
        if (start && !bot_) mode_ = Mode::Pause;
        else {
            if (bot_) bot(left, right, up, down, jump);
            t_ += dt;
            play(dt, left, right, up, down, jump);
        }
    } else if (mode_ == Mode::Pause) {
        if (start || pad.pressed(gs::BTN_A)) mode_ = Mode::Play;
    } else if (mode_ == Mode::Won || mode_ == Mode::Lost) {
        t_ += dt;
        fanT_ += dt;
        if (fan_ >= 0 && fanT_ > 0.18f) {
            fan_++;
            fanT_ = 0;
        }
        if (!bot_ && (start || pad.pressed(gs::BTN_A))) begin();
    }

    float want = px_ - 120.0f;
    if (want < 0) want = 0;
    if (want > kWorld - gs::SCREEN_W) want = kWorld - gs::SCREEN_W;
    cam_ += (want - cam_) * std::min(1.0f, dt * 7.0f);
    sys.vdp.B.scroll(-int(cam_), 0);

    if (beep_ > 0) {
        beep_ -= dt;
        if (beep_ <= 0) sys.apu.tone(0, 0, 0);
    }
    audio();
    draw();
}

void Game::audio() {
    if (mode_ == Mode::Won || mode_ == Mode::Lost) {
        const float good[] = {392, 494, 587, 784};
        const float bad[] = {196, 155, 123, 98};
        const float* n = won_ ? good : bad;
        if (fan_ >= 0 && fan_ < 4) sys_->apu.tone(2, n[fan_], won_ ? 0.08f : 0.04f);
        else sys_->apu.tone(2, 0, 0);
    } else if (mode_ == Mode::Play) {
        float bell = (std::fmod(t_, 2.4f) < 0.08f) ? 220.0f : 0.0f;
        sys_->apu.tone(2, bell, bell > 0 ? 0.03f : 0.0f);
    } else {
        sys_->apu.tone(2, 0, 0);
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet) {
    if (h < 2) return;
    const gs::Image& im = m.pick(h);
    gs::Sprite s;
    s.img = im;
    s.h = int(h);
    s.w = int(h * (float(im.w) / float(im.h > 0 ? im.h : 1)));
    if (s.w < 1) s.w = 1;
    s.x = int(cx - s.w * 0.5f);
    s.y = int(feet ? cy - s.h : cy - s.h * 0.5f);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::world(const gs::Mipped& m, float wx, float wy, float h, int pal, bool flip, bool feet) {
    spr(m, wx - cam_, wy, h, pal, flip, feet);
}

void Game::text(const char* s, float x, float y, float scale, int pal) {
    float cx = x;
    for (const char* p = s; *p; p++) {
        int gi = int(uint8_t(*p)) - 32;
        if (gi < 0 || gi >= 96) gi = 0;
        const gs::Image& im = art_.glyph[gi];
        gs::Sprite sp;
        sp.img = im;
        sp.h = std::max(1, im.h * int(scale));
        sp.w = std::max(1, im.w * int(scale));
        sp.x = int(cx);
        sp.y = int(y);
        sp.pal = uint8_t(pal);
        sys_->vdp.sprite(sp);
        cx += sp.w + scale;
    }
}

void Game::hud(int col, int row, const char* s, int pal) { text(s, 8.0f + col * 8.0f, 6.0f + row * 10.0f, 1, pal); }

void Game::draw() {
    sys_->vdp.clearSprites();
    if (mode_ == Mode::Title) {
        world(art_.moon, 250, 36, 22, PAL_DUSK, false, false);
        text("S3 TOWER LADD", 78, 64, 2, PAL_HUD);
        text("YOU HAVE THE TOWER", 86, 98, 1, PAL_TORCH);
        text("REACH THE FAR LADDER", 74, 116, 1, PAL_HUD);
        text("ARROWS MOVE   UP CLIMBS   A JUMPS", 32, 150, 1, PAL_STONE);
        text("START", 140, 176, 1, PAL_OK);
        return;
    }

    world(art_.moon, 180, 34, 20, PAL_DUSK, false, false);
    const Lad& L = kLad[kFar];
    float h = L.y1 - L.y0;
    world(art_.ladder, L.x, (L.y0 + L.y1) * 0.5f, h, PAL_OK, false, false);
    world(art_.hatch, L.x, L.y0 - 2, 14, PAL_OK, false, true);

    auto merlons = [&](float x0, float x1) {
        for (float x = x0 + 18; x < x1 - 8; x += 36) world(art_.merlon, x, 158, 20, PAL_STONE, false, true);
    };
    merlons(0, 260);
    merlons(316, 620);
    merlons(676, 1100);
    world(art_.banner, 70, 150, 28, PAL_CLOTH, std::sin(t_ * 2.2f) > 0, true);
    world(art_.banner, 1040, 148, 30, PAL_CLOTH, std::sin(t_ * 1.7f) < 0, true);
    world(art_.torch, 248, 150, 16, PAL_TORCH, false, true);
    world(art_.torch, 600, 150, 16, PAL_TORCH, false, true);
    world(art_.torch, 690, 150, 16, PAL_TORCH, false, true);

    auto pit = [&](float x0, float x1) {
        for (float x = x0 + 8; x < x1; x += 16) world(art_.merlon, x, 214, 8, PAL_PIT, false, false);
    };
    pit(260, 316);
    pit(620, 676);

    spr(heroSprite(), px_ - cam_, py_, 28, PAL_KEEP, face_ < 0, true);

    if (mode_ == Mode::Won) hud(0, 0, "FAR LADDER", PAL_OK);
    else if (mode_ == Mode::Lost) hud(0, 0, reason_, PAL_ALERT);
    else if (mode_ == Mode::Pause) hud(0, 0, "HOLD", PAL_HUD);
    else hud(0, 0, "THE FAR LADDER", PAL_HUD);
}

}  // namespace towerladd

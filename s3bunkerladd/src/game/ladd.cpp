#include "game/ladd.h"

#include <cmath>
#include <cstdio>
#include <string>

namespace bunkerladd {
namespace {

struct Plat {
    float x0, x1, y;
};

struct Lad {
    float x, y0, y1;
};

// One bunker. Floors are broken by two sumps. The catwalk clears the trolley.
// The far ladder is the only way out.
const Plat kPlat[] = {
    {0, 248, 188},
    {308, 512, 188},
    {548, 980, 188},
    {36, 420, 108},
};
const Lad kLad[] = {
    {156, 108, 188},
    {812, 36, 188},
};
constexpr int kFar = 1;
constexpr float kWorld = 1024;

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    paintWall();
    sys.vdp.A.enabled = false;
    sys.vdp.hudEnabled = false;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int k = y < 28 ? 2 : y < 150 ? 3 : 2;
        sys.vdp.lineBackdrop[y] = gs::rgb4(k, k + 1, k);
        sys.vdp.road[y].on = false;
    }
    sys.vdp.setFogColor(gs::rgb4(2, 2, 2));
    if (bot_) begin();
}

void Game::paintWall() {
    gs::VDP& v = sys_->vdp;
    v.B.resize(128, 32);
    v.B.clear();
    for (int cy = 0; cy < 28; cy++) {
        for (int cx = 0; cx < 128; cx++) {
            int tile = 1 + ((cx * 3 + cy * 5) & 3);
            if (cy >= 22 && cy <= 24) tile = 5;
            v.B.set(cx, cy, gs::entry(tile, PAL_CONC));
        }
    }
}

void Game::begin() {
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
    reason_ = "";
    px_ = 48;
    py_ = 188;
    vx_ = vy_ = 0;
    face_ = 1;
    grounded_ = true;
    onLadder_ = false;
    lad_ = -1;
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
    mode_ = Mode::Won;
    reason_ = "FAR LADDER";
    fan_ = 0;
    fanT_ = 0;
    vx_ = vy_ = 0;
    onLadder_ = true;
    lad_ = kFar;
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
    if (px_ > 720) return 3;
    if (px_ > 300 && py_ > 140) return 2;
    if (t_ > 0.3f) return 1;
    return 1;
}

float Game::trolleyX() const {
    float u = 0.5f + 0.5f * std::sin(t_ * 1.15f);
    return 328.0f + u * 100.0f;
}

bool Game::onCatwalk() const { return py_ < 140.0f; }

int Game::nearLadder() const {
    for (int i = 0; i < 2; i++) {
        const Lad& L = kLad[i];
        if (std::fabs(px_ - L.x) < 16.0f && py_ >= L.y0 - 6.0f && py_ <= L.y1 + 8.0f) return i;
    }
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
        if (px_ < 148) right = true;
        else up = true;
        break;
    case 1:
        up = true;
        break;
    case 2:
        if (px_ < 400.0f) right = true;
        break;
    case 3:
        right = true;
        break;
    case 4:
        right = true;
        if (px_ >= 492.0f && grounded_) jump = true;
        break;
    case 5:
        right = true;
        break;
    case 6:
        if (px_ < 804) right = true;
        else up = true;
        break;
    default:
        up = true;
        break;
    }
}

void Game::play(float dt, bool left, bool right, bool up, bool down, bool jumpPressed) {
    const float move = 102.0f;
    int want = -1;
    if (up || down || onLadder_) want = nearLadder();
    if (onLadder_ && want < 0) onLadder_ = false;

    if (!onLadder_ && (up || down) && want >= 0) {
        onLadder_ = true;
        lad_ = want;
        vy_ = 0;
        vx_ = 0;
    }

    if (onLadder_) {
        const Lad& L = kLad[lad_];
        px_ += (L.x - px_) * std::min(1.0f, dt * 10.0f);
        float dir = (up ? -1.0f : 0.0f) + (down ? 1.0f : 0.0f);
        if (dir == 0) dir = 0;
        py_ += dir * 62.0f * dt;
        if (py_ < L.y0) py_ = L.y0;
        if (py_ > L.y1) {
            py_ = L.y1;
            onLadder_ = false;
        }
        if (lad_ == kFar && py_ <= 52.0f) {
            win();
            return;
        }
        vx_ = 0;
        vy_ = dir * 62.0f;
        if (py_ <= L.y0 + 1.0f && up && lad_ != kFar) {
            onLadder_ = false;
            py_ = L.y0;
            vy_ = 0;
            grounded_ = true;
        } else {
            grounded_ = false;
        }
        if (dir != 0) {
            climbSnd_ -= dt;
            if (climbSnd_ <= 0) {
                sys_->apu.tone(0, 180, 0.04f);
                beep_ = 0.05f;
                climbSnd_ = 0.18f;
            }
        }
    } else {
        float ax = (right ? 1.0f : 0.0f) - (left ? 1.0f : 0.0f);
        if (ax != 0) face_ = ax > 0 ? 1 : -1;
        vx_ = ax * move;
        vy_ += 720.0f * dt;
        if (vy_ > 360) vy_ = 360;
        px_ += vx_ * dt;
        py_ += vy_ * dt;
        if (px_ < 10) px_ = 10;
        if (px_ > 960) px_ = 960;

        bool land = false;
        if (vy_ >= 0) {
            for (const Plat& p : kPlat) {
                if (px_ >= p.x0 && px_ <= p.x1 && py_ >= p.y && py_ <= p.y + 14.0f) {
                    py_ = p.y;
                    vy_ = 0;
                    land = true;
                    break;
                }
            }
        }
        grounded_ = land;
        if (land) coyote_ = 0.1f;
        else coyote_ = std::max(0.0f, coyote_ - dt);
        if (jumpPressed) jumpBuf_ = 0.1f;
        else jumpBuf_ = std::max(0.0f, jumpBuf_ - dt);
        if (jumpBuf_ > 0 && coyote_ > 0) {
            vy_ = -292;
            grounded_ = false;
            coyote_ = 0;
            jumpBuf_ = 0;
            sys_->apu.tone(1, 320, 0.06f);
            beep_ = 0.06f;
        }
        if (land && std::fabs(vx_) > 8) {
            step_ -= dt;
            foot_ += dt * 8;
            if (step_ <= 0) {
                sys_->apu.noiseBurst(0.05f, 1400, 0.04f);
                step_ = 0.22f;
            }
        }
    }

    bool inSump = ((px_ > 248 && px_ < 308) || (px_ > 512 && px_ < 548)) && py_ > 204;
    if (inSump || py_ > 230) {
        lose("INTO THE SUMP");
        return;
    }
    float tx = trolleyX();
    if (!onCatwalk() && !onLadder_ && py_ > 160 && std::fabs(px_ - tx) < 20.0f && px_ < 500) {
        lose("THE TROLLEY");
        return;
    }

    if (bot_) {
        if (botPhase_ == 0 && onLadder_) botPhase_ = 1;
        else if (botPhase_ == 1 && !onLadder_ && py_ <= 112) botPhase_ = 2;
        else if (botPhase_ == 2 && px_ >= 400.0f) botPhase_ = 3;
        else if (botPhase_ == 3 && grounded_ && py_ > 160) botPhase_ = 4;
        else if (botPhase_ == 4 && !grounded_ && px_ > 500) botPhase_ = 5;
        else if (botPhase_ == 5 && grounded_ && px_ > 548) botPhase_ = 6;
        else if (botPhase_ == 6 && onLadder_ && lad_ == kFar) botPhase_ = 7;
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
        if (fan_ >= 0 && fanT_ > 0.22f) {
            fan_++;
            fanT_ = 0;
        }
        if (fan_ > 6) {
            over_ = true;
            fan_ = -2;
        }
        if (!bot_ && (start || pad.pressed(gs::BTN_A)) && fan_ < 0) begin();
    }

    float want = px_ - 140.0f;
    if (want < 0) want = 0;
    if (want > kWorld - gs::SCREEN_W) want = kWorld - gs::SCREEN_W;
    cam_ += (want - cam_) * std::min(1.0f, dt * 6.0f);
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
        const float good[] = {523, 659, 784, 1046};
        const float bad[] = {220, 174, 130, 98};
        const float* n = won_ ? good : bad;
        if (fan_ >= 0 && fan_ < 4) sys_->apu.tone(2, n[fan_], won_ ? 0.07f : 0.04f);
        else sys_->apu.tone(2, 0, 0);
    } else if (mode_ == Mode::Play) {
        float hum = 46.0f + 2.0f * std::sin(t_ * 3.0f);
        sys_->apu.tone(2, hum, 0.015f);
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
        text("S3 BUNKER LADD", 78, 70, 2, PAL_HUD);
        text("ONE BUNKER", 108, 100, 1, PAL_OK);
        text("REACH THE FAR LADDER", 78, 116, 1, PAL_HUD);
        text("ARROWS MOVE   UP CLIMBS   A JUMPS", 36, 150, 1, PAL_CONC);
        text("START", 140, 176, 1, PAL_LAMP);
        return;
    }

    // Earlier sprites sit on top.
    if (mode_ == Mode::Won) hud(0, 0, "FAR LADDER", PAL_OK);
    else if (mode_ == Mode::Lost) hud(0, 0, reason_, PAL_ALERT);
    else if (mode_ == Mode::Pause) hud(0, 0, "HOLD", PAL_HUD);
    else hud(0, 0, "THE FAR LADDER", PAL_HUD);

    spr(heroSprite(), px_ - cam_, py_, 26, PAL_HERO, face_ < 0, true);

    for (int i = 0; i < 2; i++) {
        const Lad& L = kLad[i];
        float h = L.y1 - L.y0;
        float mid = (L.y0 + L.y1) * 0.5f;
        world(art_.ladder, L.x, mid, h, i == kFar ? PAL_OK : PAL_IRON, false, false);
    }
    world(art_.hatch, kLad[kFar].x, kLad[kFar].y0 - 2, 16, PAL_OK, false, true);
    world(art_.trolley, trolleyX(), 188, 22, PAL_RUST, std::sin(t_ * 1.15f) > 0, true);
    world(art_.bag, 90, 188, 14, PAL_SAND, false, true);
    world(art_.bag, 112, 188, 14, PAL_SAND, true, true);
    world(art_.bag, 620, 188, 14, PAL_SAND, false, true);
    world(art_.slit, 70, 70, 10, PAL_LAMP, false, false);
    world(art_.slit, 400, 48, 10, PAL_LAMP, false, false);
    world(art_.lamp, 210, 28, 18, PAL_LAMP, false, false);
    world(art_.lamp, 640, 28, 18, PAL_LAMP, false, false);

    auto slabs = [&](float x0, float x1, float y, int pal) {
        for (float x = x0 + 8; x < x1; x += 16) world(art_.slab, x, y, 12, pal, false, false);
    };
    for (const Plat& p : kPlat) slabs(p.x0, p.x1, p.y, PAL_CONC);
    slabs(248, 308, 206, PAL_PIT);
    slabs(512, 548, 206, PAL_PIT);
}

}  // namespace bunkerladd

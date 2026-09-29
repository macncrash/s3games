#include "game/ladd.h"

#include <cmath>
#include <string>

namespace redoubtladd {
namespace {

struct Plat {
    float x0, x1, y;
};

struct Lad {
    float x, y0, y1;
};

// One redoubt. Banquette, a ditch broken by a wet cut, a rolling gabion,
// and the far ladder on the counterscarp. That ladder is the only way out.
const Plat kPlat[] = {
    {0, 320, 128},
    {200, 540, 196},
    {604, 940, 196},
};
const Lad kLad[] = {
    {248, 128, 196},
    {840, 72, 196},
};
constexpr int kFar = 1;
constexpr float kWorld = 960;
constexpr float kMove = 130.0f;

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    paintEarth();
    sys.vdp.A.enabled = false;
    sys.vdp.hudEnabled = false;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int sky = y < 90 ? 2 : y < 150 ? 3 : 2;
        int g = y < 90 ? 3 : 4;
        sys.vdp.lineBackdrop[y] = gs::rgb4(sky, g, sky + 2);
        sys.vdp.road[y].on = false;
    }
    sys.vdp.setFogColor(gs::rgb4(2, 2, 3));
    if (bot_) begin();
}

void Game::paintEarth() {
    gs::VDP& v = sys_->vdp;
    v.B.resize(128, 32);
    v.B.clear();
    for (int cy = 14; cy < 28; cy++) {
        for (int cx = 0; cx < 120; cx++) {
            int tile = 1 + ((cx * 5 + cy) & 3);
            v.B.set(cx, cy, gs::entry(tile, PAL_EARTH));
        }
    }
}

void Game::begin() {
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
    reason_ = "";
    px_ = 48;
    py_ = 128;
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
    over_ = false;
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
    if (py_ > 160) return 2;
    if (t_ > 0.2f) return 1;
    return 1;
}

float Game::gabionX() const { return 430.0f + 25.0f * std::sin(t_ * 0.85f); }

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
        if (px_ < 240) right = true;
        else down = true;
        break;
    case 1:
        down = true;
        break;
    case 2:
        right = true;
        if (px_ >= 352.0f && px_ <= 372.0f && grounded_) jump = true;
        break;
    case 3:
        right = true;
        if (px_ >= 500.0f && px_ <= 524.0f && grounded_) jump = true;
        break;
    case 4:
        if (px_ < 828) right = true;
        else up = true;
        break;
    default:
        up = true;
        break;
    }
}

void Game::play(float dt, bool left, bool right, bool up, bool down, bool jumpPressed) {
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
        py_ += dir * 62.0f * dt;
        if (py_ < L.y0) py_ = L.y0;
        if (py_ > L.y1) {
            py_ = L.y1;
            onLadder_ = false;
            grounded_ = true;
            vy_ = 0;
        }
        if (lad_ == kFar && py_ <= 90.0f) {
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
        }
        if (dir != 0) {
            climbSnd_ -= dt;
            if (climbSnd_ <= 0) {
                sys_->apu.tone(0, 160, 0.04f);
                beep_ = 0.05f;
                climbSnd_ = 0.18f;
            }
        }
    } else {
        float ax = (right ? 1.0f : 0.0f) - (left ? 1.0f : 0.0f);
        if (ax != 0) face_ = ax > 0 ? 1 : -1;
        vx_ = ax * kMove;
        vy_ += 720.0f * dt;
        if (vy_ > 360) vy_ = 360;
        px_ += vx_ * dt;
        py_ += vy_ * dt;
        if (px_ < 12) px_ = 12;
        if (px_ > 920) px_ = 920;

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
            vy_ = -340;
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
                sys_->apu.noiseBurst(0.04f, 900, 0.03f);
                step_ = 0.24f;
            }
        }
    }

    bool inCut = px_ > 544.0f && px_ < 600.0f && py_ > 208.0f;
    if (inCut || py_ > 232.0f) {
        lose("INTO THE CUT");
        return;
    }
    float gx = gabionX();
    if (!onLadder_ && py_ > 168.0f && std::fabs(px_ - gx) < 14.0f && px_ < 560.0f) {
        lose("THE GABION");
        return;
    }

    if (bot_) {
        if (botPhase_ == 0 && onLadder_) botPhase_ = 1;
        else if (botPhase_ == 1 && !onLadder_ && py_ > 180.0f) botPhase_ = 2;
        else if (botPhase_ == 2 && grounded_ && px_ > 470.0f) botPhase_ = 3;
        else if (botPhase_ == 3 && grounded_ && px_ > 610.0f) botPhase_ = 4;
        else if (botPhase_ == 4 && onLadder_ && lad_ == kFar) botPhase_ = 5;
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

    float want = px_ - 120.0f;
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
        const float good[] = {392, 494, 587, 784};
        const float bad[] = {196, 155, 123, 98};
        const float* n = won_ ? good : bad;
        if (fan_ >= 0 && fan_ < 4) sys_->apu.tone(2, n[fan_], won_ ? 0.07f : 0.04f);
        else sys_->apu.tone(2, 0, 0);
    } else if (mode_ == Mode::Play) {
        float hum = 55.0f + 3.0f * std::sin(t_ * 0.6f);
        sys_->apu.tone(2, hum, 0.012f);
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
    if (mode_ == Mode::Won) hud(0, 0, "FAR LADDER", PAL_OK);
    else if (mode_ == Mode::Lost) hud(0, 0, reason_, PAL_ALERT);
    else if (mode_ == Mode::Pause) hud(0, 0, "HOLD", PAL_HUD);
    else if (mode_ == Mode::Play) hud(0, 0, "THE FAR LADDER", PAL_HUD);

    if (mode_ == Mode::Title) {
        text("S3 REDOUBT LADD", 72, 64, 2, PAL_HUD);
        text("ONE REDOUBT", 112, 96, 1, PAL_OK);
        text("REACH THE FAR LADDER", 72, 114, 1, PAL_HUD);
        text("ARROWS MOVE   UP CLIMBS   A JUMPS", 32, 148, 1, PAL_EARTH);
        text("START", 140, 176, 1, PAL_FLAG);
        return;
    }

    spr(heroSprite(), px_ - cam_, py_, 26, PAL_HERO, face_ < 0, true);

    for (int i = 0; i < 2; i++) {
        const Lad& L = kLad[i];
        float h = L.y1 - L.y0;
        float mid = (L.y0 + L.y1) * 0.5f;
        world(art_.ladder, L.x, mid, h, i == kFar ? PAL_OK : PAL_TIMBER, false, false);
    }
    world(art_.flag, 72, 128, 22, PAL_FLAG, std::sin(t_ * 2.2f) > 0, true);
    world(art_.flag, kLad[kFar].x + 18, 72, 22, PAL_FLAG, false, true);
    world(art_.gabion, gabionX(), 196, 28, PAL_GABION, false, true);
    world(art_.lamp, 160, 40, 16, PAL_FLAG, false, false);
    world(art_.lamp, 700, 36, 16, PAL_FLAG, false, false);
    world(art_.stake, 300, 128, 16, PAL_TIMBER, false, true);
    world(art_.stake, 620, 196, 16, PAL_TIMBER, false, true);
    world(art_.stake, 900, 196, 16, PAL_TIMBER, false, true);

    auto sods = [&](float x0, float x1, float y, int pal) {
        for (float x = x0 + 8; x < x1; x += 16) world(art_.sod, x, y, 10, pal, false, false);
    };
    for (const Plat& p : kPlat) sods(p.x0, p.x1, p.y, PAL_SOD);
    sods(544, 604, 214, PAL_WATER);

}

}  // namespace redoubtladd

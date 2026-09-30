#include "game/foundry.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace foundrypouc {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kGround = 188.f;
constexpr float kGrav = 0.30f;
constexpr float kJump = -6.55f;
constexpr float kRun = 2.45f;
constexpr float kHalf = 9.f;
constexpr float kWorld = 1480.f;
constexpr float kDoor = 1248.f;

constexpr float kPipeX0 = 508.f;
constexpr float kPipeX1 = 636.f;
constexpr float kPipeY1 = 156.f;
constexpr float kLadleX = 1008.f;

struct Plat {
    float x0, x1, y;
};

constexpr Plat kPlats[] = {
    {0.f, 336.f, kGround},
    {400.f, 708.f, kGround},
    {768.f, kWorld, kGround},
};

}  // namespace

int Game::marker() const {
    if (over_ && won_) return 3;
    if (over_) return 4;
    if (mode_ == Mode::Won) return 3;
    if (mode_ == Mode::Lost) return 4;
    if (mode_ == Mode::Title) return 0;
    if (held_) return 2;
    return 1;
}

bool Game::ladleDown() const {
    int phase = tick_ % 200;
    return phase < 90;
}

bool Game::pipeHits(float x, float top, float feet) const {
    (void)feet;
    return x + kHalf > kPipeX0 && x - kHalf < kPipeX1 && top < kPipeY1;
}

const gs::Mipped& Game::heroSprite() const {
    if (!onGround_) return art_.leap;
    if (duck_) return art_.duck;
    if (std::fabs(vx_) > 0.4f) return (stepN_ / 8) & 1 ? art_.runA : art_.runB;
    return art_.stand;
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.12f);
    beep_ = 0.09f;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.hudEnabled = true;
    sys.vdp.HUD.clear();
    sys.apu.setMaster(0.32f);
    sys.apu.setEcho(0.04f, 0.08f, 0.04f);
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    reason_ = "";
    tick_ = 0;
}

void Game::begin() {
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
    held_ = false;
    onGround_ = true;
    duck_ = false;
    face_ = 1;
    lives_ = 3;
    tick_ = 0;
    stepN_ = 0;
    inv_ = 0;
    reason_ = "";
    px_ = 56.f;
    py_ = kGround;
    vx_ = 0.f;
    vy_ = 0.f;
    pouchX_ = 156.f;
    cam_ = 0.f;
    coyote_ = 0.f;
    hold_ = 0.f;
    ladleY_ = 62.f;
}

void Game::finish(bool crossed, const char* why) {
    if (mode_ != Mode::Play && mode_ != Mode::Pause) return;
    mode_ = crossed ? Mode::Won : Mode::Lost;
    won_ = crossed;
    reason_ = why;
    hold_ = 0.9f;
    if (crossed) blip(523.f);
    else sys_->apu.noiseBurst(0.4f, 180.f, 0.16f);
}

void Game::bot(bool& left, bool& right, bool& jump, bool& duck) {
    left = right = jump = duck = false;
    if (mode_ != Mode::Play) return;
    int phase = tick_ % 200;
    bool window = phase >= 108 && phase <= 140;
    if (held_ && px_ > 900.f && px_ < 948.f && !window) return;
    if (held_ && px_ > 470.f && px_ < 660.f && onGround_) duck = true;
    right = true;
    if (onGround_) {
        if (px_ > 300.f && px_ < 336.f) jump = true;
        if (px_ > 668.f && px_ < 708.f) jump = true;
    }
}

void Game::stepPlay(bool left, bool right, bool jump, bool duck) {
    tick_++;
    if (inv_ > 0) inv_--;
    const bool wantDuck = duck && onGround_;
    duck_ = wantDuck;
    float speed = wantDuck ? 1.15f : kRun;
    vx_ = 0.f;
    if (left) {
        vx_ = -speed;
        face_ = -1;
    }
    if (right) {
        vx_ = speed;
        face_ = 1;
    }
    if (std::fabs(vx_) > 0.2f) stepN_++;

    if (jump && (onGround_ || coyote_ > 0.f) && !wantDuck) {
        vy_ = kJump;
        onGround_ = false;
        coyote_ = 0.f;
        duck_ = false;
        blip(340.f);
    }

    px_ += vx_;
    if (px_ < 16.f) px_ = 16.f;
    if (px_ > kWorld - 20.f) px_ = kWorld - 20.f;

    float body = duck_ ? 30.f : 52.f;
    float top = py_ - body;
    if (pipeHits(px_, top, py_) && !duck_) {
        px_ = kPipeX0 - kHalf - 1.f;
        vx_ = 0.f;
    }

    float prev = py_;
    py_ += vy_;
    vy_ += kGrav;
    if (vy_ > 8.f) vy_ = 8.f;

    body = duck_ ? 30.f : 52.f;
    top = py_ - body;
    if (vy_ < 0.f && px_ + kHalf > kPipeX0 && px_ - kHalf < kPipeX1 && top < kPipeY1 && prev - body >= kPipeY1 - 2.f) {
        py_ = kPipeY1 + body;
        vy_ = 0.4f;
        top = py_ - body;
    }

    bool landed = false;
    if (vy_ >= 0.f) {
        for (const Plat& p : kPlats) {
            if (px_ >= p.x0 - kHalf && px_ <= p.x1 + kHalf && prev <= p.y + 0.5f && py_ >= p.y) {
                py_ = p.y;
                vy_ = 0.f;
                landed = true;
                onGround_ = true;
                break;
            }
        }
    }
    if (!landed) {
        onGround_ = false;
        coyote_ = std::max(0.f, coyote_ - kDt);
    } else {
        coyote_ = 0.10f;
    }

    if (!held_ && std::fabs(px_ - pouchX_) < 22.f && std::fabs(py_ - kGround) < 8.f) {
        held_ = true;
        blip(440.f);
    }

    bool down = ladleDown();
    ladleY_ = down ? 172.f : 58.f;
    if (down && inv_ == 0 && px_ > kLadleX - 56.f && px_ < kLadleX + 56.f && py_ > 120.f) {
        lives_--;
        inv_ = 50;
        vy_ = -3.2f;
        px_ -= 28.f;
        sys_->apu.noiseBurst(0.28f, 500.f, 0.08f);
        if (lives_ <= 0) {
            finish(false, "THE LADLE TOOK THE RUN");
            return;
        }
    }

    if (py_ > 246.f) {
        lives_--;
        if (lives_ <= 0) {
            finish(false, "FELL INTO THE SLAG");
            return;
        }
        vy_ = 0.f;
        onGround_ = true;
        py_ = kGround;
        if (px_ < 400.f) px_ = 70.f;
        else if (px_ < 770.f) px_ = 460.f;
        else px_ = 840.f;
        inv_ = 40;
    }

    if (held_ && onGround_ && px_ >= kDoor && py_ <= kGround + 1.f) finish(true, "THE POUCH CROSSED THE FOUNDRY");
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    gs::Pad& pad = sys.pad;
    if (beep_ > 0.f) beep_ -= kDt;
    else sys.apu.tone(0, 0, 0);

    if (mode_ == Mode::Title) {
        tick_++;
        bool go = pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C);
        if (bot_ && tick_ > 24) go = true;
        if (go) begin();
    } else if (mode_ == Mode::Play) {
        bool left = false, right = false, jump = false, duck = false;
        if (bot_) bot(left, right, jump, duck);
        else {
            left = pad.down(gs::BTN_LEFT);
            right = pad.down(gs::BTN_RIGHT);
            duck = pad.down(gs::BTN_DOWN);
            jump = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_UP);
            if (pad.axisX < -0.4f) left = true;
            if (pad.axisX > 0.4f) right = true;
            if (pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        }
        if (mode_ == Mode::Play) stepPlay(left, right, jump, duck);
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) mode_ = Mode::Play;
    } else {
        hold_ -= kDt;
        tick_++;
        if (hold_ <= 0.f) {
            over_ = true;
            if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
                mode_ = Mode::Title;
                over_ = false;
                won_ = false;
                tick_ = 0;
            }
        }
    }
    draw();
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    int glow = (tick_ / 8) & 1;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.road[y].on = false;
        v.lineFog[y] = 0;
        if (y < 36) v.lineBackdrop[y] = gs::rgb4(2, 1, 1);
        else if (y < 150) v.lineBackdrop[y] = gs::rgb4(3 + (y / 40), 1, 1);
        else v.lineBackdrop[y] = gs::rgb4(6 + glow, 2 + glow, 1);
    }
    v.B.enabled = true;
    v.B.scroll(int(cam_), 0);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog) {
    if (!(h > 1.5f) || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::text(const char* s, float x, float y, float scale, int pal) {
    float width = 0.f;
    for (const char* p = s; *p; ++p) {
        unsigned char c = static_cast<unsigned char>(*p);
        if (c < 33 || c > 126) width += 10.f * scale;
        else width += float(art_.glyph[c - 32].w) * scale;
    }
    x -= width * 0.5f;
    for (const char* p = s; *p; ++p) {
        unsigned char c = static_cast<unsigned char>(*p);
        if (c < 33 || c > 126) {
            x += 10.f * scale;
            continue;
        }
        const gs::Mipped& g = art_.glyph[c - 32];
        float gw = float(g.w) * scale;
        spr(g, x + gw * 0.5f, y, float(g.h) * scale, pal, false, 0);
        x += gw;
    }
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (row < 0 || row > 27) return;
    for (int i = 0; s[i]; ++i) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c < 33 || c > 126) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    float look = px_ - 110.f;
    if (look < 0.f) look = 0.f;
    if (look > kWorld - gs::SCREEN_W) look = kWorld - gs::SCREEN_W;
    cam_ = look;
    backdrop();

    auto wx = [&](float x) { return x - cam_; };

    for (int i = 0; i < 6; i++) {
        float x = 40.f + float(i) * 220.f;
        spr(art_.stack, wx(x), 48.f, 26.f, PAL_SOOT, false, 2);
    }
    spr(art_.furnace, wx(78.f), 150.f, 78.f, PAL_FURNACE, false, 0);
    float mouth = ((tick_ / 7) & 1) ? 12.f : 9.f;
    spr(art_.spark, wx(86.f), 158.f, mouth, PAL_SPARK, false, 0);
    spr(art_.spark, wx(70.f), 168.f, 7.f, PAL_SLAG, false, 0);

    for (int i = 0; i < 4; i++) spr(art_.pipe, wx(kPipeX0 + 16.f + float(i) * 30.f), kPipeY1 - 6.f, 14.f, PAL_IRON, false, 0);

    auto slag = [&](float x0, float x1) {
        float mid = (x0 + x1) * 0.5f;
        float w = x1 - x0;
        for (float x = x0 + 16.f; x < x1; x += 28.f) {
            float bob = ((tick_ / 6 + int(x)) & 1) ? 2.f : 0.f;
            spr(art_.spark, wx(x), 206.f + bob, 10.f, PAL_SLAG, false, 0);
        }
        (void)mid;
        (void)w;
    };
    slag(330.f, 406.f);
    slag(700.f, 778.f);

    spr(art_.ladle, wx(kLadleX), ladleY_, ladleDown() ? 40.f : 28.f, PAL_LADLE, false, 0);
    spr(art_.door, wx(kDoor + 28.f), kGround - 32.f, 70.f, PAL_DOOR, false, 0);

    if (!(inv_ > 0 && (tick_ & 2))) {
        float hh = duck_ && onGround_ ? 34.f : 56.f;
        if (!onGround_) hh = 56.f;
        const gs::Mipped& hs = heroSprite();
        spr(hs, wx(px_), py_ - hh * 0.5f, hh, PAL_WORKER, face_ < 0, 0);
    }
    int glint = (tick_ / 10) & 1;
    if (held_) spr(art_.pouch[glint], wx(px_ + face_ * 12.f), py_ - 36.f, 20.f, PAL_POUCH, face_ < 0, 0);
    else spr(art_.pouch[glint], wx(pouchX_), kGround - 12.f, 22.f, PAL_POUCH, false, 0);

    if (mode_ == Mode::Title) {
        text("FOUNDRY POUC", 160.f, 28.f, 0.7f, PAL_SLAG);
        text("CARRY THE POUCH ACROSS", 160.f, 52.f, 0.36f, PAL_HUD);
        hudC(24, "START", PAL_SLAG);
        hudC(26, "ONE FOUNDRY", PAL_HUD);
    } else if (mode_ == Mode::Play || mode_ == Mode::Pause) {
        char line[40];
        std::snprintf(line, sizeof line, "LIVES %d", lives_);
        hud(1, 1, line, PAL_HUD);
        hud(28, 1, held_ ? "POUCH" : "EMPTY", held_ ? PAL_DOOR : PAL_SLAG);
        if (!held_) hudC(26, "TAKE THE POUCH", PAL_HUD);
        else hudC(26, "LEFT RIGHT  A JUMP  DOWN DUCK", PAL_HUD);
        if (mode_ == Mode::Pause) text("PAUSED", 160.f, 40.f, 0.8f, PAL_SLAG);
    } else if (mode_ == Mode::Won) {
        text("CROSSED", 160.f, 28.f, 0.8f, PAL_DOOR);
        hudC(25, "THE POUCH CROSSED THE FOUNDRY", PAL_DOOR);
    } else if (mode_ == Mode::Lost) {
        text(reason_, 160.f, 26.f, std::strlen(reason_) > 18 ? 0.36f : 0.5f, PAL_SLAG);
        hudC(25, reason_, PAL_SLAG);
    }
}

}  // namespace foundrypouc

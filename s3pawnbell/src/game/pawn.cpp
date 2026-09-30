#include "game/pawn.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace pawnbell {
namespace {
constexpr float kFloor = 176.f;
constexpr float kGrav = 0.32f;
constexpr float kJump = -5.15f;
constexpr float kWalk = 1.62f;
constexpr float kPw = 14.f;
constexpr float kPh = 20.f;
constexpr float kPitA0 = 96.f, kPitA1 = 120.f;
constexpr float kPitB0 = 192.f, kPitB1 = 216.f;
constexpr float kLeft = 16.f, kRight = 304.f;
}  // namespace

bool Game::onFloor(float x) const {
    if (x < kLeft || x > kRight) return false;
    if (x >= kPitA0 && x < kPitA1) return false;
    if (x >= kPitB0 && x < kPitB1) return false;
    return true;
}

bool Game::grounded() const {
    if (py_ < kFloor - 0.6f || vy_ < -0.05f) return false;
    return onFloor(px_ + 3.f) || onFloor(px_ + kPw - 3.f);
}

void Game::resetTry() {
    px_ = 28.f;
    py_ = kFloor;
    vx_ = 0.f;
    vy_ = 0.f;
    faceR_ = true;
    mode_ = Mode::Play;
    wait_ = 0;
}

void Game::kill(const char* why) {
    if (mode_ != Mode::Play) return;
    dead_++;
    why_ = why;
    mode_ = Mode::Dead;
    wait_ = 0;
    vy_ = -2.2f;
    if (sys_) sys_->apu.noiseBurst(0.18f, 240.f, 0.08f);
    if (dead_ >= 3) {
        why_ = "third try died";
    }
}

void Game::ring() {
    if (rung_) return;
    rung_ = true;
    mode_ = Mode::Ring;
    wait_ = 0;
    why_ = "bell";
    if (sys_) {
        sys_->apu.tone(0, 523.f, 0.18f);
        sys_->apu.tone(1, 784.f, 0.12f);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    layHall();
    mode_ = Mode::Title;
    dead_ = 0;
    rung_ = false;
    won_ = false;
    over_ = false;
    why_ = "";
    t_ = 0;
    bobPh_ = 0.4f;
    px_ = 28.f;
    py_ = kFloor;
}

void Game::layHall() {
    if (!sys_) return;
    gs::Plane& p = sys_->vdp.B;
    p.clear();
    for (int cy = 0; cy < 22; cy++) {
        for (int cx = 0; cx < 40; cx++) {
            int tile = (cy > 16) ? art_.tileWall : 0;
            if (cy == 21) tile = art_.tileTrim;
            if (tile) p.set(cx, cy, gs::entry(tile, PAL_WOOD));
        }
    }
    for (int cx = 2; cx < 38; cx++) {
        float x = float(cx * 8);
        bool pit = (x >= kPitA0 && x < kPitA1) || (x >= kPitB0 && x < kPitB1);
        if (pit) continue;
        int tile = ((cx / 2) & 1) ? art_.tileLight : art_.tileDark;
        int pal = ((cx / 2) & 1) ? PAL_IVORY : PAL_WOOD;
        p.set(cx, 22, gs::entry(tile, pal));
        p.set(cx, 23, gs::entry(art_.tileDark, PAL_WOOD));
    }
}

void Game::backdrop() {
    if (!sys_) return;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int shade = 4 + (y * 6) / gs::SCREEN_H;
        if (y > 150) shade = 3;
        sys_->vdp.lineBackdrop[y] = gs::rgb4(shade, shade - 1, shade + 2);
        sys_->vdp.lineFog[y] = 0;
        sys_->vdp.road[y].on = false;
    }
    sys_->vdp.setFogColor(gs::rgb4(2, 2, 4));
}

void Game::botThink() {
    float bobX = 156.f + std::sin(bobPh_) * 34.f;
    float bobVx = std::cos(bobPh_) * 34.f * 0.045f;
    bool ground = grounded();
    auto hop = [&]() {
        if (ground) vy_ = kJump;
    };
    if (px_ < 78.f) {
        vx_ = kWalk;
        faceR_ = true;
        if (px_ >= 74.f) hop();
        return;
    }
    if (px_ < 168.f) {
        faceR_ = true;
        float gap = bobX - (px_ + kPw);
        if (gap > 18.f && (bobVx > 0.f || gap > 36.f)) vx_ = kWalk;
        else if (gap < 16.f && gap > -8.f && bobVx < 0.f) hop();
        else if (bobX + 10.f < px_) vx_ = kWalk;
        else vx_ = 0.f;
        if (px_ >= 164.f && ground) hop();
        return;
    }
    vx_ = kWalk;
    faceR_ = true;
    if (px_ >= 248.f && ground) hop();
}

void Game::physics() {
    bobPh_ += 0.045f;
    if (!bot_) {
        vx_ = 0.f;
        if (sys_->pad.down(gs::BTN_LEFT)) {
            vx_ = -kWalk;
            faceR_ = false;
        }
        if (sys_->pad.down(gs::BTN_RIGHT)) {
            vx_ = kWalk;
            faceR_ = true;
        }
        bool hop = sys_->pad.pressed(gs::BTN_C) || sys_->pad.pressed(gs::BTN_UP) || sys_->pad.pressed(gs::BTN_A);
        if (hop && grounded()) vy_ = kJump;
    } else {
        botThink();
    }

    vy_ += kGrav;
    if (vy_ > 6.f) vy_ = 6.f;
    px_ += vx_;
    py_ += vy_;
    if (px_ < 8.f) px_ = 8.f;
    if (px_ > 300.f) px_ = 300.f;

    if (vy_ >= 0.f && py_ >= kFloor && (onFloor(px_ + 3.f) || onFloor(px_ + kPw - 3.f))) {
        py_ = kFloor;
        vy_ = 0.f;
    }
    if (py_ > 210.f) kill("fell");

    float bobX = 156.f + std::sin(bobPh_) * 34.f;
    float bobY = 158.f;
    float cx = px_ + kPw * 0.5f;
    float cy = py_ - kPh * 0.55f;
    float dx = cx - bobX;
    float dy = cy - bobY;
    if (dx * dx + dy * dy < 15.f * 15.f) kill("weight");

    float top = py_ - kPh;
    bool inBell = cx > 250.f && cx < 294.f && top < 142.f && top > 96.f && py_ < kFloor - 2.f;
    if (inBell) ring();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_++;
    backdrop();
    if (mode_ == Mode::Title) {
        if (bot_ && t_ > 6) resetTry();
        else if (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_C)) resetTry();
    } else if (mode_ == Mode::Play) {
        physics();
    } else if (mode_ == Mode::Dead) {
        vy_ += kGrav;
        py_ += vy_;
        wait_++;
        if (wait_ > 48) {
            if (dead_ >= 3) {
                mode_ = Mode::Over;
                over_ = true;
                won_ = false;
            } else {
                resetTry();
            }
        }
    } else if (mode_ == Mode::Ring) {
        wait_++;
        if (wait_ == 20 && sys_) sys_->apu.tone(0, 659.f, 0.16f);
        if (wait_ == 40 && sys_) sys_->apu.tone(0, 784.f, 0.16f);
        if (wait_ > 70) {
            mode_ = Mode::Over;
            over_ = true;
            won_ = dead_ < 3 && rung_;
        }
    }
    if (mode_ != Mode::Ring && mode_ != Mode::Play) {
        sys.apu.tone(0, 0.f, 0.f);
        sys.apu.tone(1, 0.f, 0.f);
    }
    draw();
}

void Game::stamp(const gs::Image& img, float x, float y, float w, float h, int pal) {
    if (!sys_ || img.w == 0) return;
    gs::Sprite s;
    s.x = int16_t(std::lround(x));
    s.y = int16_t(std::lround(y));
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.img = img;
    s.pal = uint8_t(pal);
    s.hflip = false;
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!sys_ || !s) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        int tile = art_.font[c - 32];
        if (!tile) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(tile, pal));
    }
}

void Game::draw() {
    if (!sys_) return;
    sys_->vdp.clearSprites();
    sys_->vdp.HUD.clear();
    float bobX = 156.f + std::sin(bobPh_) * 34.f;
    stamp(art_.bob, bobX - 8.f, 150.f, 16.f, 16.f, PAL_BOB);
    float swing = std::sin(t_ * 0.08f) * (rung_ ? 10.f : 1.4f);
    stamp(art_.bell, 262.f + swing, 58.f, 28.f, 36.f, PAL_BELL);
    float pawnX = faceR_ ? px_ : px_;
    stamp(art_.pawn, pawnX, py_ - kPh, kPw + 2.f, kPh + 2.f, PAL_PAWN);
    if (!faceR_) {
        // redraw flipped by setting hflip on last sprite is easier: stamp again
    }
    if (mode_ == Mode::Title) {
        hud(12, 6, "SHORT PAWN", PAL_GOLD);
        hud(8, 9, "JUMP THE BELL", PAL_INK);
        hud(7, 12, "THREE TRIES", PAL_ALERT);
        hud(10, 18, "START", PAL_INK);
    } else {
        char line[24];
        std::snprintf(line, sizeof(line), "DEAD %d", dead_);
        hud(1, 1, line, dead_ >= 2 ? PAL_ALERT : PAL_INK);
        hud(28, 1, rung_ ? "BELL" : "QUIET", rung_ ? PAL_GOLD : PAL_INK);
        if (mode_ == Mode::Over && won_) hud(12, 8, "RING", PAL_GOLD);
        if (mode_ == Mode::Over && !won_) hud(8, 8, "THIRD TRY", PAL_ALERT);
    }
    if (!faceR_) {
        // The last pawn sprite should face left. Re-issue with hflip.
        sys_->vdp.clearSprites();
        stamp(art_.bob, bobX - 8.f, 150.f, 16.f, 16.f, PAL_BOB);
        stamp(art_.bell, 262.f + swing, 58.f, 28.f, 36.f, PAL_BELL);
        gs::Sprite s;
        s.x = int16_t(std::lround(px_));
        s.y = int16_t(std::lround(py_ - kPh));
        s.w = int16_t(kPw + 2.f);
        s.h = int16_t(kPh + 2.f);
        s.img = art_.pawn;
        s.pal = PAL_PAWN;
        s.hflip = true;
        sys_->vdp.sprite(s);
    }
}

}  // namespace pawnbell

#include "game/clockmark.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace clockmark {
namespace {
constexpr float kTau = 6.2831853f;
}

void Game::blip(float freq) {
    sys_->apu.keyOn(0, freq, 0.22f);
}

void Game::chord() {
    sys_->apu.keyOn(0, 392.f, 0.3f);
    sys_->apu.keyOn(1, 523.25f, 0.28f);
    sys_->apu.keyOn(2, 659.25f, 0.22f);
}

void Game::begin() {
    hour_ = 9;
    tries_ = 3;
    rep_ = 0;
    liftT_ = 0;
    botWait_ = 8;
    coinY_ = 0;
    over_ = false;
    won_ = false;
    finished_ = false;
    lifted_ = false;
    marked_ = false;
    rules_ = true;
    mode_ = Mode::Play;
    sys_->apu.silence();
}

void Game::turn(int dir) {
    if (dir == 0 || mode_ != Mode::Play) return;
    hour_ = (hour_ + dir + kHours) % kHours;
    blip(onMark() ? 660.f : 330.f);
}

void Game::lift() {
    if (mode_ != Mode::Play) return;
    if (!onMark()) {
        tries_--;
        blip(90.f);
        sys_->apu.noiseBurst(0.25f, 140.f, 0.12f);
        if (!bot_) sys_->rumble(0.4f, 0.1f, 70);
        if (tries_ <= 0) fail();
        return;
    }
    marked_ = true;
    mode_ = Mode::Lift;
    liftT_ = 0;
    coinY_ = 0;
    blip(520.f);
}

void Game::finish() {
    lifted_ = true;
    finished_ = true;
    won_ = true;
    over_ = true;
    mode_ = Mode::Over;
    chord();
}

void Game::fail() {
    won_ = false;
    finished_ = false;
    lifted_ = false;
    over_ = true;
    mode_ = Mode::Over;
    sys_->apu.noiseBurst(0.35f, 80.f, 0.3f);
}

void Game::spr(const gs::Image& img, float cx, float cy, int pal) {
    if (img.w == 0) return;
    gs::Sprite s;
    s.img = img;
    s.w = img.w;
    s.h = img.h;
    s.x = int16_t(std::lround(cx - img.w * 0.5f));
    s.y = int16_t(std::lround(cy - img.h * 0.5f));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    uint16_t top = gs::rgb4(1, 1, 5);
    uint16_t mid = gs::rgb4(8, 4, 3);
    uint16_t bot = gs::rgb4(2, 1, 3);
    if (mode_ == Mode::Over && won_) mid = gs::rgb4(12, 8, 3);
    if (mode_ == Mode::Over && !won_) {
        top = gs::rgb4(2, 0, 1);
        mid = gs::rgb4(5, 1, 1);
    }
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H - 1);
        auto mix = [](uint16_t a, uint16_t b, float t) {
            int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
            int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
            auto L = [&](int p, int q) { return int(p + (q - p) * t + 0.5f); };
            return gs::rgb4(L(ar, br), L(ag, bg), L(ab, bb));
        };
        v.lineBackdrop[y] = u < 0.55f ? mix(top, mid, u / 0.55f) : mix(mid, bot, (u - 0.55f) / 0.45f);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    sky();

    int shown = hour_;
    if (mode_ == Mode::Title) shown = int(sys_->frame / 18) % kHours;
    bool hot = mode_ != Mode::Title && (onMark() || marked_);
    spr(art_.bell, float(kCx), 46.f, PAL_GOLD);
    spr(art_.dial, float(kCx), float(kCy), PAL_DIAL);

    float ma = kMark * (kTau / 12.f);
    float mx = kCx + std::sin(ma) * 34.f;
    float my = kCy - std::cos(ma) * 34.f - coinY_;
    if (!(lifted_ && coinY_ > 70.f)) spr(art_.coin, mx, my, PAL_COIN);

    // Hands paint over the dial. Pivot sits at the bitmap centre.
    spr(art_.hand[shown], float(kCx), float(kCy), hot ? PAL_LIT : PAL_HAND);

    char buf[48];
    int f = int(sys_->frame);
    if (mode_ == Mode::Title) {
        hudC(1, "S3 CLOCKMARK", PAL_GOLD);
        hudC(24, "A SHORT CLOCK", PAL_GOLD);
        hudC(25, "A FINISHED MARK ENDS IT", PAL_HUD);
        if ((f & 16) == 0) hudC(27, "PRESS START", PAL_GOLD);
        const char* ver = S3_VERSION_STRING;
        hud(40 - int(std::strlen(ver)), 0, ver, PAL_DIM);
    } else if (mode_ == Mode::Over && won_) {
        hudC(1, "FINISHED MARK", PAL_GOLD);
        hudC(25, "THE COIN IS LIFTED", PAL_GOLD);
        hudC(26, "THE HOUR SITS ON THE GOLD", PAL_HUD);
    } else if (mode_ == Mode::Over) {
        hudC(1, "MARK STILL OPEN", PAL_BAD);
        hudC(25, "THREE LIFTS", PAL_BAD);
        hudC(26, "THE HAND NEVER HELD THE GOLD", PAL_HUD);
        if ((f & 16) == 0) hudC(27, "START", PAL_GOLD);
    } else {
        std::snprintf(buf, sizeof buf, "MARK %d", kMark == 0 ? 12 : kMark);
        hud(1, 25, buf, PAL_GOLD);
        std::snprintf(buf, sizeof buf, "HAND %d", hour_ == 0 ? 12 : hour_);
        hudC(25, buf, onMark() ? PAL_GOLD : PAL_HUD);
        std::snprintf(buf, sizeof buf, "TRIES %d", tries_);
        hud(31, 25, buf, tries_ < 3 ? PAL_BAD : PAL_HUD);
        if (mode_ == Mode::Lift) hudC(26, "LIFTING THE MARK", PAL_GOLD);
        else if (mode_ == Mode::Pause) hudC(26, "PAUSED", PAL_GOLD);
        else if (onMark()) hudC(26, "LIFT THE MARK", PAL_GOLD);
        else hudC(26, "SET THE HAND ON THE GOLD", PAL_HUD);
        hudC(27, "LR TURN   C LIFTS", PAL_DIM);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.85f);
    mode_ = Mode::Title;
    if (bot_) begin();
    draw();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C)) begin();
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
        } else if (bot_) {
            if (botWait_ > 0) botWait_--;
            else if (!onMark()) {
                int cw = (kMark - hour_ + kHours) % kHours;
                int ccw = (hour_ - kMark + kHours) % kHours;
                turn(cw <= ccw ? 1 : -1);
            } else {
                lift();
            }
        } else {
            int dir = 0;
            if (pad.down(gs::BTN_RIGHT)) dir += 1;
            if (pad.down(gs::BTN_LEFT)) dir -= 1;
            if (dir == 0) rep_ = 0;
            else if (pad.pressed(gs::BTN_RIGHT) || pad.pressed(gs::BTN_LEFT)) {
                rep_ = 0;
                turn(dir);
            } else if (++rep_ > 8 && (rep_ % 3) == 0) {
                turn(dir);
            }
            if (pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A)) lift();
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
    } else if (mode_ == Mode::Lift) {
        liftT_++;
        coinY_ = float(liftT_) * 1.6f;
        if (liftT_ == 28) finish();
    } else if (mode_ == Mode::Over) {
        if (!bot_ && !won_ && pad.pressed(gs::BTN_START)) begin();
    }

    if (!bot_ && mode_ != Mode::Title && pad.pressed(gs::BTN_MODE)) {
        mode_ = Mode::Title;
        sys.apu.silence();
    }
    draw();
}

}  // namespace clockmark

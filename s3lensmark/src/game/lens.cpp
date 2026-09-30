#include "game/lens.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace lens {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap bodyArt() {
    Bitmap b(220, 78);
    b.rect(8, 10, 204, 58, 2);
    b.rect(16, 16, 188, 46, 3);
    b.rect(28, 22, 92, 34, 4);
    b.ellipse(168, 38, 28, 22, 6);
    b.ellipse(168, 38, 16, 12, 4);
    b.rect(148, 34, 10, 8, 1);
    b.rect(12, 62, 196, 8, 5);
    b.rect(96, 4, 28, 10, 3);
    b.rect(104, 0, 12, 8, 2);
    b.outline(4, false);
    return b;
}

Bitmap barrelArt() {
    Bitmap b(120, 36);
    b.rect(4, 6, 112, 24, 2);
    b.rect(4, 6, 112, 6, 1);
    b.rect(4, 24, 112, 6, 3);
    for (int i = 0; i < 9; i++) b.rect(12 + i * 12, 10, 2, 16, i == 6 ? 7 : 4);
    b.ellipse(60, 18, 8, 8, 6);
    b.outline(4, false);
    return b;
}

Bitmap ringArt() {
    Bitmap b(48, 48);
    b.ellipse(24, 24, 22, 22, 2);
    b.ellipse(24, 24, 16, 16, 4);
    b.ellipse(24, 24, 10, 10, 6);
    b.rect(22, 2, 4, 8, 7);
    return b;
}

Bitmap glassArt() {
    Bitmap b(100, 72);
    b.rect(2, 2, 96, 68, 1);
    b.rect(8, 8, 84, 56, 2);
    b.line(8, 60, 90, 10, 3, 2);
    return b;
}

Bitmap hazeArt() {
    Bitmap b(90, 60);
    b.ellipse(45, 30, 40, 26, 1);
    b.ellipse(45, 30, 22, 14, 2);
    return b;
}

Bitmap finderArt() {
    Bitmap b(132, 96);
    b.rect(0, 0, 132, 8, 1);
    b.rect(0, 88, 132, 8, 1);
    b.rect(0, 0, 8, 96, 1);
    b.rect(124, 0, 8, 96, 1);
    b.rect(10, 10, 18, 3, 2);
    b.rect(10, 10, 3, 18, 2);
    b.rect(104, 10, 18, 3, 2);
    b.rect(119, 10, 3, 18, 2);
    b.rect(10, 83, 18, 3, 2);
    b.rect(10, 68, 3, 18, 2);
    b.rect(104, 83, 18, 3, 2);
    b.rect(119, 68, 3, 18, 2);
    b.rect(62, 44, 8, 2, 2);
    b.rect(65, 41, 2, 8, 2);
    return b;
}

Bitmap figureArt() {
    Bitmap b(36, 72);
    b.ellipse(18, 10, 7, 7, 1);
    b.rect(14, 8, 8, 4, 3);
    b.rect(12, 18, 12, 26, 2);
    b.rect(8, 20, 6, 16, 2);
    b.rect(22, 20, 6, 16, 2);
    b.rect(12, 28, 12, 6, 4);
    b.rect(13, 44, 4, 20, 3);
    b.rect(19, 44, 4, 20, 3);
    b.rect(11, 62, 7, 4, 4);
    b.rect(18, 62, 7, 4, 4);
    b.outline(3, false);
    return b;
}

Bitmap lampArt() {
    Bitmap b(28, 80);
    b.rect(12, 18, 4, 58, 2);
    b.poly({{4, 20}, {14, 4}, {24, 20}}, 1);
    b.rect(6, 18, 16, 6, 3);
    b.ellipse(14, 14, 4, 3, 1);
    return b;
}

Bitmap pierArt() {
    Bitmap b(200, 28);
    b.rect(0, 8, 200, 10, 1);
    b.rect(0, 18, 200, 6, 2);
    for (int x = 8; x < 200; x += 22) b.rect(x, 20, 4, 8, 3);
    b.rect(0, 6, 200, 3, 4);
    return b;
}

Bitmap scaleArt() {
    Bitmap b(160, 16);
    b.rect(0, 7, 160, 2, 1);
    for (int i = 0; i < 17; i++) b.rect(i * 10, i % 4 == 0 ? 2 : 5, 2, i % 4 == 0 ? 12 : 8, 2);
    b.rect(112, 0, 3, 16, 3);
    return b;
}

Bitmap caretArt() {
    Bitmap b(10, 14);
    b.poly({{5, 12}, {1, 2}, {9, 2}}, 1);
    return b;
}

Bitmap stampArt() {
    Bitmap b(32, 32);
    b.ellipse(16, 16, 14, 14, 1);
    b.ellipse(16, 16, 10, 10, 2);
    b.line(11, 8, 11, 24, 3, 2);
    b.line(11, 8, 21, 8, 3, 2);
    b.line(11, 16, 18, 16, 3, 2);
    return b;
}

Bitmap printArt() {
    Bitmap b(92, 70);
    b.rect(0, 0, 92, 70, 1);
    b.rect(6, 6, 80, 50, 3);
    b.rect(6, 40, 80, 16, 4);
    b.rect(40, 18, 3, 28, 2);
    b.ellipse(28, 22, 6, 10, 2);
    b.rect(4, 58, 84, 6, 1);
    b.outline(2, false);
    return b;
}

Bitmap cornerArt() {
    Bitmap b(18, 18);
    b.rect(0, 0, 14, 3, 1);
    b.rect(0, 0, 3, 14, 1);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    gs::TextStyle big{3, 1, 0, 15, 1};
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) {
                    px[y * 8 + x + 1] = 1;
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
                }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

}  // namespace

void Game::buildArt() {
    gs::VDP& vdp = sys_->vdp;
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 14, 12), gs::rgb4(8, 9, 11), gs::rgb4(15, 12, 4), gs::rgb4(14, 5, 3),
                          gs::rgb4(6, 13, 9), gs::rgb4(8, 12, 15), shadow, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BRASS, {0, gs::rgb4(15, 13, 7), gs::rgb4(11, 8, 4), gs::rgb4(6, 4, 2), gs::rgb4(2, 2, 2),
                            gs::rgb4(7, 2, 2), gs::rgb4(8, 12, 14), gs::rgb4(15, 12, 3), shadow, 0, 0, 0, 0, 0, 0,
                            shadow});
    setPal(vdp, PAL_SCENE, {0, gs::rgb4(7, 6, 5), gs::rgb4(4, 3, 3), gs::rgb4(3, 3, 4), gs::rgb4(12, 11, 9),
                            gs::rgb4(15, 13, 6), shadow, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FIGURE, {0, gs::rgb4(13, 9, 7), gs::rgb4(3, 4, 8), gs::rgb4(2, 1, 2), gs::rgb4(12, 3, 4),
                             shadow, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_PAPER, {0, gs::rgb4(14, 13, 11), gs::rgb4(7, 6, 6), gs::rgb4(4, 4, 6), gs::rgb4(8, 8, 10),
                            shadow, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GLASS, {0, gs::rgb4(6, 10, 13), gs::rgb4(10, 14, 15), gs::rgb4(14, 15, 15), shadow, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_INK, {0, gs::rgb4(14, 11, 3), gs::rgb4(14, 13, 11), gs::rgb4(2, 1, 1), shadow, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, shadow});
    vdp.setFogColor(gs::rgb4(3, 4, 8));
    loadFont(vdp, art_);
    art_.body = gs::uploadMipped(vdp, bodyArt());
    art_.barrel = gs::uploadMipped(vdp, barrelArt());
    art_.ring = gs::uploadMipped(vdp, ringArt());
    art_.glass = gs::uploadMipped(vdp, glassArt());
    art_.haze = gs::uploadMipped(vdp, hazeArt());
    art_.finder = gs::uploadMipped(vdp, finderArt());
    art_.figure = gs::uploadMipped(vdp, figureArt());
    art_.lamp = gs::uploadMipped(vdp, lampArt());
    art_.pier = gs::uploadMipped(vdp, pierArt());
    art_.scale = gs::uploadMipped(vdp, scaleArt());
    art_.caret = gs::uploadMipped(vdp, caretArt());
    art_.stamp = gs::uploadMipped(vdp, stampArt());
    art_.print = gs::uploadMipped(vdp, printArt());
    art_.corner = gs::uploadMipped(vdp, cornerArt());
    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.HUD.clear();
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt();
    sys.apu.setMaster(0.45f);
    gs::FMPatch tone;
    tone.alg = 0;
    tone.vol = 0.28f;
    tone.op[0] = {1, 1, 0.01f, 0.18f, 0.35f, 0.25f, 0};
    tone.op[1] = {2, 0.35f, 0.01f, 0.2f, 0.2f, 0.3f, 0};
    tone.op[2] = {1, 0.15f, 0.02f, 0.3f, 0.25f, 0.35f, 0};
    tone.op[3] = {1, 0.4f, 0.01f, 0.2f, 0.45f, 0.2f, 0};
    sys.apu.setPatch(0, tone);
    sys.apu.setPatch(1, tone);
    msg_ = "FINISH THE MARK";
    msgT_ = 4;
    draw();
}

void Game::blip(float freq) { sys_->apu.keyOn(0, freq, 0.2f); }

void Game::fanfare() {
    sys_->apu.keyOn(0, 440, 0.26f);
    sys_->apu.keyOn(1, 659, 0.2f);
}

void Game::hud(int col, int row, const std::string& s) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], PAL_HUD));
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

bool Game::press(gs::Button b) const { return sys_->pad.pressed(b); }

bool Game::held(gs::Button b) const { return sys_->pad.down(b); }

void Game::backdrop() {
    gs::VDP& vdp = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H - 1);
        int r = 2 + int((1.0f - u) * 3);
        int g = 3 + int((1.0f - u) * 2);
        int b = 6 + int((1.0f - u) * 5);
        if (y > 150) {
            r = 2;
            g = 3;
            b = 5;
        }
        vdp.lineBackdrop[y] = gs::rgb4(r, g, b);
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }
}

void Game::update(float dt) {
    const gs::Pad& pad = sys_->pad;
    t_ += dt;
    phase_ = std::fmod(phase_ + dt * 0.42f, 1.0f);
    if (msgT_ > 0) msgT_ -= dt;

    if (mode_ == Mode::Title) {
        if (bot_ ? t_ > 0.45f : (press(gs::BTN_START) || press(gs::BTN_A))) {
            mode_ = Mode::Focus;
            t_ = 0;
            msg_ = "SET THE LENS ON THE GOLD";
            msgT_ = 2.4f;
            blip(392);
        }
        return;
    }

    if (mode_ == Mode::Focus) {
        float steer = 0;
        if (bot_) steer = trueFocus_ > focus_ ? 1.f : -1.f;
        else {
            if (held(gs::BTN_LEFT) || pad.axisX < -0.25f) steer -= 1;
            if (held(gs::BTN_RIGHT) || pad.axisX > 0.25f) steer += 1;
        }
        if (bot_ && std::fabs(focus_ - trueFocus_) < 0.012f) steer = 0;
        focus_ = std::clamp(focus_ + steer * 0.42f * dt, 0.02f, 0.98f);
        sharp_ = std::fabs(focus_ - trueFocus_) < 0.035f;
        bool go = bot_ ? (sharp_ && t_ > 0.35f) : press(gs::BTN_A);
        if (go) {
            if (sharp_) {
                mode_ = Mode::Frame;
                t_ = 0;
                msg_ = "PUT THE FIGURE ON THE MARKS";
                msgT_ = 2.4f;
                blip(494);
            } else {
                msg_ = "SOFT  TURN THE RING";
                msgT_ = 1.2f;
                blip(130);
            }
        }
        return;
    }

    if (mode_ == Mode::Frame) {
        float ix = 0, iy = 0;
        if (bot_) {
            ix = camX_ > 1.5f ? -1.f : (camX_ < -1.5f ? 1.f : 0.f);
            iy = camY_ > 1.5f ? -1.f : (camY_ < -1.5f ? 1.f : 0.f);
        } else {
            if (held(gs::BTN_LEFT) || pad.axisX < -0.25f) ix -= 1;
            if (held(gs::BTN_RIGHT) || pad.axisX > 0.25f) ix += 1;
            if (held(gs::BTN_UP) || pad.axisY > 0.25f) iy -= 1;
            if (held(gs::BTN_DOWN) || pad.axisY < -0.25f) iy += 1;
        }
        camX_ = std::clamp(camX_ + ix * 46.0f * dt, -70.0f, 70.0f);
        camY_ = std::clamp(camY_ + iy * 40.0f * dt, -40.0f, 40.0f);
        framed_ = std::fabs(camX_) < 6.0f && std::fabs(camY_) < 5.0f;
        bool go = bot_ ? (framed_ && t_ > 0.3f) : press(gs::BTN_A);
        if (go) {
            if (framed_) {
                mode_ = Mode::Expose;
                t_ = 0;
                msg_ = "TRIP ON THE GOLD";
                msgT_ = 2.2f;
                blip(523);
            } else {
                msg_ = "OFF THE MARKS";
                msgT_ = 1.1f;
                blip(130);
            }
        }
        return;
    }

    if (mode_ == Mode::Expose) {
        float needle = phase_ < 0.5f ? phase_ * 2.0f : (1.0f - phase_) * 2.0f;
        bool in = needle > 0.46f && needle < 0.62f;
        bool go = bot_ ? (in && t_ > 0.2f) : press(gs::BTN_A);
        if (go) {
            if (in && sharp_ && framed_) {
                mode_ = Mode::Develop;
                t_ = 0;
                slide_ = 0;
                msg_ = "FINISHED MARK";
                msgT_ = 6;
                fanfare();
            } else {
                plates_--;
                msg_ = plates_ > 0 ? "OPEN PLATE" : "NO PLATES";
                msgT_ = 1.3f;
                blip(90);
                if (plates_ <= 0) {
                    mode_ = Mode::Over;
                    won_ = false;
                    over_ = true;
                }
            }
        }
        return;
    }

    if (mode_ == Mode::Develop) {
        slide_ = std::min(1.0f, slide_ + dt * 0.7f);
        if (t_ > 2.1f) {
            mode_ = Mode::Over;
            won_ = true;
            over_ = true;
        }
    }
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    backdrop();

    const float fx = 160.0f - camX_ * 0.15f;
    const float fy = 78.0f - camY_ * 0.1f;
    const bool showScene = mode_ != Mode::Develop && mode_ != Mode::Over;

    if (mode_ == Mode::Develop || mode_ == Mode::Over) {
        float py = 40.0f + (1.0f - slide_) * 90.0f;
        if (mode_ == Mode::Over && won_) py = 78;
        if (mode_ == Mode::Over && !won_) py = 200;
        spr(art_.stamp, 196, py + 16, 26, PAL_INK);
        spr(art_.print, 150, py, 78, PAL_PAPER);
    }

    spr(art_.body, 160, 196, 70, PAL_BRASS);
    float ringX = 78.0f + focus_ * 150.0f;
    spr(art_.caret, ringX, 168, 12, PAL_BRASS);
    spr(art_.scale, 156, 156, 14, PAL_BRASS);
    spr(art_.barrel, 156, 178, 28, PAL_BRASS);
    spr(art_.ring, 250, 176, sharp_ ? 34 : 30, PAL_BRASS);

    if (showScene || mode_ == Mode::Title) {
        spr(art_.finder, fx, fy, 108, PAL_BRASS);
        spr(art_.corner, fx - 46, fy - 32, 14, PAL_HUD);
        spr(art_.corner, fx + 46, fy - 32, 14, PAL_HUD, true);
        if (!sharp_ && mode_ != Mode::Title) spr(art_.haze, fx, fy, 52, PAL_GLASS);
        float sx = fx - camX_;
        float sy = fy - 6 - camY_;
        float sh = sharp_ || mode_ == Mode::Title ? 58.0f : 46.0f;
        spr(art_.figure, sx, sy, sh, PAL_FIGURE);
        spr(art_.lamp, sx + 46, sy + 4, 62, PAL_SCENE);
        spr(art_.glass, fx, fy, 64, PAL_GLASS);
        spr(art_.pier, fx, fy + 36, 22, PAL_SCENE);
    }

    if (mode_ == Mode::Expose) {
        float needle = phase_ < 0.5f ? phase_ * 2.0f : (1.0f - phase_) * 2.0f;
        spr(art_.caret, 70 + needle * 180.0f, 132, 10, PAL_HUD);
    }

    hud(12, 1, "S3 LENSMARK");
    if (mode_ == Mode::Title) {
        hud(11, 3, "A SHORT LENS");
        hud(8, 24, "START  FINISH THE MARK");
    } else {
        char plates[16];
        std::snprintf(plates, sizeof(plates), "PLATES %d", plates_);
        hud(30, 1, plates);
        if (msgT_ > 0) {
            int col = std::max(0, 20 - int(msg_.size()) / 2);
            hud(col, 25, msg_);
        }
        if (mode_ == Mode::Focus) hud(1, 23, sharp_ ? "SHARP" : "SOFT");
        if (mode_ == Mode::Frame) hud(1, 23, framed_ ? "ON MARKS" : "FRAME");
        if (mode_ == Mode::Expose) hud(1, 23, "SHUTTER");
        if (won_) hud(12, 23, "MARK CLOSED");
        if (over_ && !won_) hud(13, 23, "MARK OPEN");
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    update(1.0f / 60.0f);
    draw();
}

}  // namespace lens

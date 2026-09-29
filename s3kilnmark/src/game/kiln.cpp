#include "game/kiln.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace kiln {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap kilnBody() {
    Bitmap b(160, 176);
    b.rect(18, 36, 124, 128, 2);
    b.rect(10, 48, 140, 108, 3);
    b.poly({{28, 48}, {80, 10}, {132, 48}}, 2);
    b.poly({{40, 48}, {80, 22}, {120, 48}}, 4);
    for (int y = 56; y < 150; y += 10) {
        for (int x = 22; x < 138; x += 16) {
            int shift = ((y / 10) & 1) ? 8 : 0;
            b.rect(x + shift, y, 14, 8, 1);
            b.rect(x + shift + 1, y + 1, 12, 6, (x / 16 + y / 10) & 1 ? 3 : 2);
        }
    }
    b.rect(58, 78, 44, 70, 5);
    b.rect(64, 88, 32, 48, 6);
    b.rect(148, 90, 8, 36, 4);
    b.rect(150, 70, 4, 24, 1);
    b.outline(5, false);
    return b;
}

Bitmap doorArt() {
    Bitmap b(48, 72);
    b.rect(2, 2, 44, 68, 3);
    b.rect(6, 8, 36, 56, 2);
    b.rect(20, 34, 8, 8, 1);
    b.outline(5, false);
    return b;
}

Bitmap glowArt() {
    Bitmap b(36, 52);
    b.ellipse(18, 28, 16, 22, 2);
    b.ellipse(18, 28, 10, 14, 1);
    b.ellipse(18, 30, 5, 7, 3);
    return b;
}

Bitmap wheelArt() {
    Bitmap b(120, 28);
    b.ellipse(60, 14, 56, 10, 2);
    b.ellipse(60, 14, 40, 6, 3);
    b.ellipse(60, 13, 8, 3, 1);
    b.rect(54, 20, 12, 8, 4);
    return b;
}

Bitmap potArt(bool wide) {
    Bitmap b(72, 88);
    float neck = wide ? 22.f : 14.f;
    b.poly({{36 - neck, 10}, {36 + neck, 10}, {36 + neck + 6, 28}, {50, 70}, {22, 70}, {36 - neck - 6, 28}}, 2);
    b.poly({{36 - neck + 4, 14}, {36 + neck - 4, 14}, {36 + neck, 30}, {46, 64}, {26, 64}, {36 - neck, 30}}, 1);
    b.ellipse(36, 12, neck, 6, 3);
    b.ellipse(36, 12, neck - 6, 3, 4);
    b.rect(20, 68, 32, 8, 2);
    b.outline(5, false);
    return b;
}

Bitmap markArt() {
    Bitmap b(28, 28);
    b.ellipse(14, 14, 12, 12, 1);
    b.ellipse(14, 14, 9, 9, 2);
    // A small kiln stamp: a K inside the ring.
    b.line(10, 8, 10, 20, 3, 2);
    b.line(10, 14, 18, 8, 3, 2);
    b.line(10, 14, 18, 20, 3, 2);
    return b;
}

Bitmap coneArt(bool bent) {
    Bitmap b(20, 36);
    if (!bent) {
        b.poly({{10, 2}, {16, 32}, {4, 32}}, 1);
        b.rect(3, 30, 14, 4, 2);
    } else {
        b.poly({{4, 10}, {16, 18}, {8, 32}}, 1);
        b.rect(3, 30, 14, 4, 2);
    }
    b.outline(3, false);
    return b;
}

Bitmap shelfArt() {
    Bitmap b(200, 24);
    b.rect(0, 8, 200, 8, 2);
    b.rect(8, 16, 8, 8, 3);
    b.rect(184, 16, 8, 8, 3);
    b.rect(0, 6, 200, 3, 1);
    return b;
}

Bitmap figureArt() {
    Bitmap b(40, 72);
    b.ellipse(20, 12, 8, 8, 1);
    b.rect(14, 20, 12, 22, 2);
    b.rect(6, 22, 8, 6, 1);
    b.rect(26, 22, 8, 6, 1);
    b.rect(14, 42, 5, 22, 3);
    b.rect(21, 42, 5, 22, 3);
    b.rect(12, 62, 8, 4, 4);
    b.rect(20, 62, 8, 4, 4);
    b.outline(5, false);
    return b;
}

Bitmap stampTool() {
    Bitmap b(24, 40);
    b.rect(8, 2, 8, 22, 2);
    b.ellipse(12, 30, 10, 8, 1);
    b.ellipse(12, 30, 6, 4, 3);
    return b;
}

Bitmap smokeArt() {
    Bitmap b(32, 32);
    b.ellipse(16, 18, 12, 8, 1);
    b.ellipse(12, 14, 7, 6, 2);
    b.ellipse(20, 12, 6, 5, 1);
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
    const uint16_t shadow = gs::rgb4(2, 1, 1);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 14, 12), gs::rgb4(10, 8, 6), gs::rgb4(15, 12, 4), gs::rgb4(15, 6, 2),
                          gs::rgb4(6, 14, 8), gs::rgb4(4, 8, 14), shadow, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_CLAY, {0, gs::rgb4(14, 10, 7), gs::rgb4(11, 7, 4), gs::rgb4(15, 13, 10), gs::rgb4(8, 5, 3),
                           gs::rgb4(3, 2, 2), shadow, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GLAZE, {0, gs::rgb4(6, 12, 11), gs::rgb4(3, 8, 8), gs::rgb4(12, 15, 13), gs::rgb4(2, 5, 5),
                            gs::rgb4(15, 14, 8), shadow, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BRICK, {0, gs::rgb4(12, 5, 3), gs::rgb4(9, 3, 2), gs::rgb4(14, 7, 4), gs::rgb4(6, 3, 2),
                            gs::rgb4(2, 1, 1), gs::rgb4(1, 1, 1), gs::rgb4(15, 10, 3), 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FIRE, {0, gs::rgb4(15, 12, 3), gs::rgb4(15, 6, 1), gs::rgb4(15, 15, 12), gs::rgb4(10, 2, 1),
                           shadow, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(12, 8, 4), gs::rgb4(8, 5, 2), gs::rgb4(5, 3, 1), gs::rgb4(14, 11, 6),
                           shadow, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_INK, {0, gs::rgb4(14, 12, 8), gs::rgb4(4, 3, 2), gs::rgb4(1, 1, 1), gs::rgb4(15, 14, 10),
                          shadow, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FIGURE, {0, gs::rgb4(13, 9, 6), gs::rgb4(4, 6, 10), gs::rgb4(3, 3, 5), gs::rgb4(2, 2, 2),
                             gs::rgb4(1, 1, 1), shadow, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    vdp.setFogColor(gs::rgb4(8, 3, 1));
    loadFont(vdp, art_);
    art_.kiln = gs::uploadMipped(vdp, kilnBody());
    art_.door = gs::uploadMipped(vdp, doorArt());
    art_.glow = gs::uploadMipped(vdp, glowArt());
    art_.wheel = gs::uploadMipped(vdp, wheelArt());
    art_.pot = gs::uploadMipped(vdp, potArt(false));
    art_.potWide = gs::uploadMipped(vdp, potArt(true));
    art_.mark = gs::uploadMipped(vdp, markArt());
    art_.cone = gs::uploadMipped(vdp, coneArt(false));
    art_.coneBent = gs::uploadMipped(vdp, coneArt(true));
    art_.shelf = gs::uploadMipped(vdp, shelfArt());
    art_.figure = gs::uploadMipped(vdp, figureArt());
    art_.stamp = gs::uploadMipped(vdp, stampTool());
    art_.smoke = gs::uploadMipped(vdp, smokeArt());
    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.HUD.clear();
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt();
    sys.apu.setMaster(0.5f);
    gs::FMPatch tone;
    tone.alg = 0;
    tone.vol = 0.3f;
    tone.op[0] = {1, 1, 0.01f, 0.2f, 0.4f, 0.3f, 0};
    tone.op[1] = {2, 0.4f, 0.01f, 0.25f, 0.2f, 0.3f, 0};
    tone.op[2] = {1, 0.2f, 0.02f, 0.3f, 0.3f, 0.4f, 0};
    tone.op[3] = {1, 0.5f, 0.01f, 0.2f, 0.5f, 0.25f, 0};
    sys.apu.setPatch(0, tone);
    sys.apu.setPatch(1, tone);
}

float Game::rnd() {
    rng_ = rng_ * 1664525u + 1013904223u;
    return (rng_ >> 8) * (1.0f / 16777216.0f);
}

void Game::blip(float freq) { sys_->apu.keyOn(0, freq, 0.22f); }

void Game::fanfare() {
    sys_->apu.keyOn(0, 392, 0.28f);
    sys_->apu.keyOn(1, 523, 0.22f);
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

void Game::backdrop() {
    gs::VDP& vdp = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H - 1);
        int r = 5 + int(4 * (1.0f - u));
        int g = 3 + int(u * 2);
        int b = 2;
        if (mode_ == Mode::Fire || mode_ == Mode::Leave) {
            float glow = std::clamp((heat_ - 600.0f) / 800.0f, 0.0f, 1.0f);
            r = std::min(15, r + int(glow * 6 * (1.0f - u)));
            g = std::min(15, g + int(glow * 2));
        }
        vdp.lineBackdrop[y] = gs::rgb4(r, g, b);
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }
}

void Game::update(float dt) {
    const gs::Pad& pad = sys_->pad;
    t_ += dt;
    spin_ += dt * (mode_ == Mode::Throw ? 4.0f : 0.4f);
    if (msgT_ > 0) msgT_ -= dt;

    auto press = [&](gs::Button b) { return pad.pressed(b); };
    auto held = [&](gs::Button b) { return pad.down(b); };

    if (mode_ == Mode::Title) {
        if (bot_ ? t_ > 0.4f : (press(gs::BTN_START) || press(gs::BTN_A))) {
            mode_ = Mode::Throw;
            t_ = 0;
            height_ = 8;
            wobble_ = 0;
            msg_ = "CENTER THE CLAY";
            msgT_ = 2;
            blip(330);
        }
        return;
    }

    if (mode_ == Mode::Throw) {
        float steer = 0;
        if (bot_) steer = wobble_ > 0 ? -1.f : 1.f;
        else {
            if (held(gs::BTN_LEFT) || pad.axisX < -0.25f) steer -= 1;
            if (held(gs::BTN_RIGHT) || pad.axisX > 0.25f) steer += 1;
        }
        wobble_ += std::sin(t_ * 2.4f) * 28.0f * dt;
        wobble_ += steer * 90.0f * dt;
        wobble_ *= std::exp(-1.2f * dt);
        bool pull = bot_ ? std::fabs(wobble_) < 14.0f : (held(gs::BTN_A) || pad.accel > 0.4f);
        if (pull && std::fabs(wobble_) < 18.0f) height_ += 38.0f * dt;
        else if (std::fabs(wobble_) > 36.0f) height_ = std::max(8.0f, height_ - 20.0f * dt);
        if (height_ >= 100.0f) {
            height_ = 100;
            mode_ = Mode::Stamp;
            cx_ = 70;
            cy_ = 60;
            tx_ = 188;
            ty_ = 126;
            t_ = 0;
            msg_ = "STAMP THE MARK";
            msgT_ = 2.5f;
            blip(440);
        }
        return;
    }

    if (mode_ == Mode::Stamp) {
        float ix = 0, iy = 0;
        if (bot_) {
            ix = tx_ > cx_ ? 1.f : -1.f;
            iy = ty_ > cy_ ? 1.f : -1.f;
            if (std::fabs(tx_ - cx_) < 4) ix = 0;
            if (std::fabs(ty_ - cy_) < 4) iy = 0;
        } else {
            if (held(gs::BTN_LEFT) || pad.axisX < -0.25f) ix -= 1;
            if (held(gs::BTN_RIGHT) || pad.axisX > 0.25f) ix += 1;
            if (held(gs::BTN_UP) || pad.axisY > 0.25f) iy -= 1;
            if (held(gs::BTN_DOWN) || pad.axisY < -0.25f) iy += 1;
        }
        cx_ = std::clamp(cx_ + ix * 120.0f * dt, 20.0f, 300.0f);
        cy_ = std::clamp(cy_ + iy * 120.0f * dt, 30.0f, 190.0f);
        bool hit = bot_ ? (std::fabs(cx_ - tx_) < 5 && std::fabs(cy_ - ty_) < 5 && t_ > 0.3f) : press(gs::BTN_A);
        if (hit) {
            float dx = cx_ - tx_, dy = cy_ - ty_;
            if (dx * dx + dy * dy < 14.0f * 14.0f) {
                marked_ = true;
                mode_ = Mode::Fire;
                heat_ = 640;
                damper_ = 0.45f;
                cone_ = 0;
                t_ = 0;
                msg_ = "FIRE THE CONE";
                msgT_ = 2.5f;
                blip(523);
            } else {
                smudge_++;
                msg_ = "SMUDGED  AIM THE SHOULDER";
                msgT_ = 1.4f;
                blip(140);
            }
        }
        return;
    }

    if (mode_ == Mode::Fire) {
        if (bot_) {
            if (heat_ < 1188) damper_ = 1;
            else if (heat_ > 1232) damper_ = 0;
            else damper_ = 0.36f;
        } else {
            if (held(gs::BTN_UP) || pad.axisY > 0.3f) damper_ += dt * 0.7f;
            if (held(gs::BTN_DOWN) || pad.axisY < -0.3f) damper_ -= dt * 0.7f;
            damper_ = std::clamp(damper_, 0.0f, 1.0f);
        }
        heat_ += (damper_ * 42.0f - 15.0f) * dt * 8.0f;
        heat_ = std::clamp(heat_, 200.0f, 1500.0f);
        if (heat_ >= 1180.0f && heat_ <= 1240.0f) cone_ += 28.0f * dt;
        else if (heat_ > 1240.0f) cone_ += 8.0f * dt;
        if (heat_ > 1360.0f) {
            cracks_++;
            cone_ = 0;
            heat_ = 700;
            msg_ = "OVERFIRED  AGAIN";
            msgT_ = 1.6f;
            blip(90);
        }
        if (cone_ >= 100.0f && marked_) {
            cone_ = 100;
            mode_ = Mode::Leave;
            leave_ = 0;
            msg_ = "FINISHED MARK";
            msgT_ = 6;
            fanfare();
        }
        return;
    }

    if (mode_ == Mode::Leave) {
        leave_ += dt;
        if (leave_ > 2.4f) {
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

    spr(art_.shelf, 160, 188, 28, PAL_WOOD);
    spr(art_.kiln, 210, 112, 168, PAL_BRICK);

    bool open = mode_ == Mode::Leave || mode_ == Mode::Over || mode_ == Mode::Throw || mode_ == Mode::Stamp ||
                mode_ == Mode::Title;
    if (!open) {
        float pulse = 0.75f + 0.25f * std::sin(t_ * 10.0f);
        spr(art_.glow, 210, 128, 46 * pulse + heat_ / 80.0f, PAL_FIRE);
        spr(art_.door, 210, 130, 78, PAL_BRICK);
        spr(cone_ >= 100 ? art_.coneBent : art_.cone, 248, 150, 28, PAL_CLAY);
    } else if (mode_ == Mode::Leave || mode_ == Mode::Over) {
        spr(art_.glow, 210, 132, 30, PAL_FIRE);
    }

    int potPal = (mode_ == Mode::Leave || mode_ == Mode::Over) ? PAL_GLAZE : PAL_CLAY;
    if (mode_ == Mode::Throw || mode_ == Mode::Title) {
        spr(art_.wheel, 108, 176, 26, PAL_WOOD);
        float h = mode_ == Mode::Title ? 70.0f : 28.0f + height_ * 0.55f;
        float lean = mode_ == Mode::Throw ? wobble_ * 0.15f : 0;
        spr(height_ > 55 || mode_ == Mode::Title ? art_.pot : art_.potWide, 108 + lean, 168 - h * 0.35f, h, potPal);
    } else if (mode_ == Mode::Stamp) {
        spr(art_.pot, 160, 140, 92, PAL_CLAY);
        spr(art_.stamp, cx_, cy_ - 16, 36, PAL_WOOD);
        spr(art_.mark, tx_, ty_, 16, PAL_INK);
    } else if (mode_ == Mode::Leave || mode_ == Mode::Over) {
        spr(art_.pot, 118, 132, 86, PAL_GLAZE);
        spr(art_.mark, 136, 128, 18, PAL_INK);
    }

    float figX = 46;
    if (mode_ == Mode::Leave || mode_ == Mode::Over) figX = 46 + leave_ * 90.0f;
    if (mode_ != Mode::Over) spr(art_.figure, figX, 158, 70, PAL_FIGURE, leave_ > 0.2f);

    if ((mode_ == Mode::Fire || mode_ == Mode::Leave) && heat_ > 900) {
        float s = 10 + std::fmod(t_ * 18.0f, 22.0f);
        spr(art_.smoke, 250, 28 - std::fmod(t_ * 12.0f, 20.0f), s, PAL_HUD);
    }

    if (mode_ == Mode::Title) {
        hud(12, 3, "KILN MARK");
        hud(8, 6, "THROW  STAMP  FIRE");
        hud(7, 22, "START  TO OPEN THE DOOR");
    } else if (mode_ == Mode::Throw) {
        hud(2, 2, "WHEEL");
        int bars = std::clamp(int(height_ / 5), 0, 20);
        std::string bar(size_t(bars), '#');
        hud(2, 4, bar);
        hud(2, 24, "LEFT RIGHT CENTER");
        hud(2, 26, "HOLD A TO PULL");
    } else if (mode_ == Mode::Stamp) {
        hud(2, 2, "MARK");
        hud(2, 26, "ARROWS MOVE   A STAMP");
    } else if (mode_ == Mode::Fire) {
        hud(2, 2, "KILN");
        char buf[32];
        std::snprintf(buf, sizeof(buf), "HEAT %d", int(heat_));
        hud(2, 4, buf);
        std::snprintf(buf, sizeof(buf), "CONE %d", int(cone_));
        hud(2, 5, buf);
        int d = int(damper_ * 10);
        std::string lever = "DAMPER ";
        for (int i = 0; i < 10; i++) lever += i < d ? '#' : '.';
        hud(2, 7, lever);
        hud(2, 26, "UP OPEN   DOWN CLOSE");
        if (heat_ >= 1180 && heat_ <= 1240) hud(28, 4, "BAND");
        else if (heat_ > 1240) hud(28, 4, "HOT");
    } else if (mode_ == Mode::Leave || mode_ == Mode::Over) {
        hud(12, 4, "FINISHED MARK");
        hud(10, 6, "THE CONE HAS BENT");
        hud(9, 24, "LEAVE THE KILN");
    }
    if (msgT_ > 0 && mode_ != Mode::Title) {
        int col = std::max(0, 20 - int(msg_.size()) / 2);
        hud(col, 20, msg_);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    update(1.0f / 60.0f);
    draw();
}

}  // namespace kiln

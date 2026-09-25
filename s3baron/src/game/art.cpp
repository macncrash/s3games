#include "game/art.h"

#include <cmath>
#include <string>

namespace baron {
namespace {

constexpr float TAU = 6.2831853f;

using gs::Bitmap;
using gs::Pt;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Pt rot(float cx, float cy, float x, float y, float bank) {
    float dx = x - cx, dy = y - cy;
    float c = std::cos(bank), s = std::sin(bank);
    return {cx + dx * c - dy * s, cy + dx * s + dy * c};
}

void quad(Bitmap& b, float x, float y, float w, float h, int c, float bank, float cx, float cy) {
    b.poly({rot(cx, cy, x, y, bank), rot(cx, cy, x + w, y, bank), rot(cx, cy, x + w, y + h, bank),
            rot(cx, cy, x, y + h, bank)},
           c);
}

void blob(Bitmap& b, float x, float y, float rx, float ry, int c, float bank, float cx, float cy) {
    Pt p = rot(cx, cy, x, y, bank);
    b.ellipse(p.first, p.second, rx, ry, c);
}

void strut(Bitmap& b, float x0, float y0, float x1, float y1, int c, float bank, float cx, float cy) {
    Pt a = rot(cx, cy, x0, y0, bank);
    Pt d = rot(cx, cy, x1, y1, bank);
    b.line(a.first, a.second, d.first, d.second, c, 2.0f);
}

// Chase view. Nose is up-screen, tail is closer to the camera.
Bitmap biplaneRear(float bank) {
    Bitmap b(128, 104);
    const float cx = 64, cy = 52;
    quad(b, 34, 78, 60, 6, 3, bank, cx, cy);
    quad(b, 38, 76, 52, 6, 2, bank, cx, cy);
    quad(b, 58, 74, 12, 22, 3, bank, cx, cy);
    quad(b, 60, 72, 8, 20, 4, bank, cx, cy);
    quad(b, 14, 50, 100, 10, 3, bank, cx, cy);
    quad(b, 16, 48, 96, 8, 2, bank, cx, cy);
    quad(b, 4, 26, 120, 12, 3, bank, cx, cy);
    quad(b, 6, 24, 116, 10, 1, bank, cx, cy);
    b.poly({rot(cx, cy, 54, 16, bank), rot(cx, cy, 74, 16, bank), rot(cx, cy, 70, 86, bank), rot(cx, cy, 58, 86, bank)}, 3);
    b.poly({rot(cx, cy, 56, 14, bank), rot(cx, cy, 72, 14, bank), rot(cx, cy, 68, 84, bank), rot(cx, cy, 60, 84, bank)}, 2);
    blob(b, 64, 18, 10, 8, 5, bank, cx, cy);
    blob(b, 64, 18, 6, 5, 8, bank, cx, cy);
    strut(b, 28, 30, 30, 50, 5, bank, cx, cy);
    strut(b, 100, 30, 98, 50, 5, bank, cx, cy);
    strut(b, 46, 32, 46, 50, 5, bank, cx, cy);
    strut(b, 82, 32, 82, 50, 5, bank, cx, cy);
    // Roundels on the upper wing.
    for (float x : {28.f, 100.f}) {
        blob(b, x, 29, 8, 6, 6, bank, cx, cy);
        blob(b, x, 29, 5, 4, 7, bank, cx, cy);
        blob(b, x, 29, 2.4f, 2.0f, 8, bank, cx, cy);
    }
    b.outline(5, false);
    return b;
}

// Head-on view for anything coming at the gunsight.
Bitmap biplaneFront(float bank) {
    Bitmap b(120, 80);
    const float cx = 60, cy = 40;
    quad(b, 8, 22, 104, 12, 3, bank, cx, cy);
    quad(b, 10, 20, 100, 10, 2, bank, cx, cy);
    quad(b, 22, 46, 76, 9, 3, bank, cx, cy);
    quad(b, 24, 44, 72, 8, 1, bank, cx, cy);
    blob(b, 60, 36, 14, 16, 3, bank, cx, cy);
    blob(b, 60, 34, 11, 13, 2, bank, cx, cy);
    blob(b, 60, 30, 7, 7, 5, bank, cx, cy);
    blob(b, 60, 30, 3, 3, 8, bank, cx, cy);
    strut(b, 34, 26, 36, 46, 5, bank, cx, cy);
    strut(b, 86, 26, 84, 46, 5, bank, cx, cy);
    // Iron crosses. A white field with a black cross reads at mip 1/4.
    for (float x : {28.f, 92.f}) {
        blob(b, x, 25, 9, 7, 6, bank, cx, cy);
        quad(b, x - 7, 23, 14, 4, 5, bank, cx, cy);
        quad(b, x - 2, 18, 4, 14, 5, bank, cx, cy);
    }
    quad(b, 54, 8, 12, 6, 4, bank, cx, cy);
    b.outline(5, false);
    return b;
}

Bitmap balloonArt() {
    Bitmap b(72, 96);
    b.ellipse(36, 34, 28, 30, 3);
    b.ellipse(36, 32, 26, 28, 2);
    b.ellipse(28, 24, 10, 14, 1);
    b.line(36, 60, 36, 74, 5, 2);
    b.rect(28, 74, 16, 10, 4);
    b.rect(30, 76, 12, 6, 3);
    b.line(36, 84, 36, 95, 5, 1.5f);
    b.outline(5, false);
    return b;
}

Bitmap sightArt() {
    Bitmap b(64, 64);
    for (int i = 0; i < 64; i++) {
        float a = i * TAU / 64.0f;
        // Broken ring: leave the cardinal gaps so the target stays visible.
        if (i % 16 < 3) continue;
        b.set(int(std::lround(32 + std::cos(a) * 22)), int(std::lround(32 + std::sin(a) * 22)), 3);
        b.set(int(std::lround(32 + std::cos(a) * 21)), int(std::lround(32 + std::sin(a) * 21)), 5);
    }
    b.rect(31, 8, 2, 8, 3);
    b.rect(31, 48, 2, 8, 3);
    b.rect(8, 31, 8, 2, 3);
    b.rect(48, 31, 8, 2, 3);
    b.set(32, 32, 3);
    return b;
}

Bitmap puffArt() {
    Bitmap b(48, 48);
    b.ellipse(26, 26, 18, 16, 7);
    b.ellipse(22, 22, 14, 12, 6);
    b.ellipse(20, 20, 7, 6, 3);
    for (int y = 0; y < 48; y++)
        for (int x = 0; x < 48; x++)
            if (b.get(x, y) && ((x + y) & 1) && std::hypot(x - 24.0f, y - 24.0f) > 14) b.set(x, y, 0);
    return b;
}

Bitmap propTree() {
    Bitmap b(48, 64);
    b.rect(22, 40, 6, 22, 4);
    b.ellipse(24, 28, 18, 16, 2);
    b.ellipse(24, 22, 12, 10, 1);
    b.outline(5, false);
    return b;
}

Bitmap propRuin() {
    Bitmap b(56, 72);
    b.rect(8, 28, 40, 42, 3);
    b.rect(10, 30, 36, 38, 2);
    b.poly({{8, 28}, {28, 6}, {48, 28}}, 3);
    b.poly({{14, 26}, {28, 12}, {42, 26}}, 4);
    b.rect(24, 48, 10, 20, 5);
    b.rect(14, 36, 8, 8, 1);
    b.rect(34, 36, 8, 10, 1);
    b.outline(5, false);
    return b;
}

Bitmap propCrater() {
    Bitmap b(64, 32);
    b.ellipse(32, 16, 28, 10, 3);
    b.ellipse(32, 16, 16, 6, 5);
    b.ellipse(32, 15, 6, 2, 2);
    return b;
}

Bitmap propBarn() {
    Bitmap b(64, 48);
    b.rect(6, 20, 52, 26, 4);
    b.poly({{4, 22}, {32, 4}, {60, 22}}, 8);
    b.rect(28, 28, 10, 18, 5);
    b.rect(12, 26, 8, 8, 1);
    b.rect(44, 26, 8, 8, 1);
    b.outline(5, false);
    return b;
}

Bitmap cloudArt() {
    Bitmap b(80, 32);
    b.ellipse(24, 18, 18, 10, 6);
    b.ellipse(42, 16, 22, 12, 6);
    b.ellipse(58, 18, 14, 8, 6);
    b.ellipse(36, 14, 12, 7, 3);
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

void skyFor(int wave, uint16_t& top, uint16_t& horizon, uint16_t& fog) {
    struct Sky { uint16_t top, hor, fog; };
    static const Sky k[5] = {
        {gs::rgb4(2, 5, 12), gs::rgb4(15, 10, 6), gs::rgb4(14, 10, 7)},
        {gs::rgb4(3, 7, 14), gs::rgb4(12, 14, 15), gs::rgb4(11, 13, 14)},
        {gs::rgb4(6, 7, 9), gs::rgb4(11, 11, 12), gs::rgb4(10, 10, 11)},
        {gs::rgb4(2, 4, 10), gs::rgb4(14, 8, 4), gs::rgb4(13, 8, 5)},
        {gs::rgb4(4, 2, 8), gs::rgb4(15, 6, 2), gs::rgb4(14, 6, 3)},
    };
    const Sky& s = k[wave < 0 ? 0 : wave > 4 ? 4 : wave];
    top = s.top;
    horizon = s.hor;
    fog = s.fog;
}

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(15, 15, 15);
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(10, 10, 12), gs::rgb4(15, 15, 15), gs::rgb4(15, 4, 3), gs::rgb4(3, 13, 5),
                          gs::rgb4(15, 12, 3), gs::rgb4(4, 8, 15), gs::rgb4(5, 5, 6), 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 12, 2), gs::rgb4(15, 8, 1), gs::rgb4(15, 15, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 3, 2), gs::rgb4(10, 1, 1), gs::rgb4(15, 12, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(8, 15, 6), gs::rgb4(3, 10, 3), gs::rgb4(14, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    auto plane = [&](int pal, uint16_t hi, uint16_t mid, uint16_t lo, uint16_t linen) {
        setPal(vdp, pal, {0, hi, mid, lo, linen, gs::rgb4(1, 1, 1), gs::rgb4(15, 15, 15), gs::rgb4(2, 4, 13),
                          gs::rgb4(13, 2, 2), gs::rgb4(8, 6, 3), 0, 0, 0, 0, 0, shadow});
    };
    plane(PAL_PLAYER, gs::rgb4(9, 11, 6), gs::rgb4(6, 8, 4), gs::rgb4(3, 4, 2), gs::rgb4(12, 10, 7));
    plane(PAL_ENEMY, gs::rgb4(13, 12, 8), gs::rgb4(10, 9, 5), gs::rgb4(6, 5, 3), gs::rgb4(12, 10, 7));
    plane(PAL_ACE, gs::rgb4(15, 6, 4), gs::rgb4(13, 1, 1), gs::rgb4(7, 0, 0), gs::rgb4(5, 1, 1));

    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 13, 3), gs::rgb4(15, 7, 1), gs::rgb4(15, 15, 15), gs::rgb4(14, 3, 2),
                         gs::rgb4(1, 1, 1), gs::rgb4(12, 11, 9), gs::rgb4(7, 6, 5), gs::rgb4(15, 15, 8), 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_PROP, {0, gs::rgb4(14, 14, 10), gs::rgb4(4, 8, 3), gs::rgb4(7, 6, 4), gs::rgb4(9, 6, 3), gs::rgb4(2, 2, 2),
                           gs::rgb4(12, 12, 12), gs::rgb4(5, 7, 3), gs::rgb4(12, 3, 2), gs::rgb4(8, 8, 7), 0, 0, 0, 0, 0, shadow});

    // Field palette consumed by the road chip. Indices are the chip's, not ours.
    const uint16_t field[16] = {
        0,
        gs::rgb4(8, 10, 4), gs::rgb4(6, 8, 3), gs::rgb4(5, 4, 2),
        gs::rgb4(7, 8, 3), gs::rgb4(5, 6, 2),
        gs::rgb4(11, 9, 5), gs::rgb4(8, 6, 3),
        gs::rgb4(5, 4, 3), gs::rgb4(6, 5, 2), gs::rgb4(4, 3, 2),
        gs::rgb4(4, 7, 10), gs::rgb4(3, 6, 9), gs::rgb4(10, 12, 13),
        gs::rgb4(14, 12, 6), gs::rgb4(12, 10, 6),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_FIELD * 16 + i, field[i]);

    loadFont(vdp, art);
    for (int i = 0; i < 3; i++) {
        float bank = i * 0.42f;
        art.rear[i] = gs::uploadMipped(vdp, biplaneRear(bank));
        art.front[i] = gs::uploadMipped(vdp, biplaneFront(bank));
    }
    art.balloon = gs::uploadMipped(vdp, balloonArt());
    art.sight = gs::uploadMipped(vdp, sightArt());
    art.puff = gs::uploadMipped(vdp, puffArt());
    Bitmap flash(16, 16);
    flash.ellipse(8, 8, 7, 5, 1);
    flash.ellipse(8, 8, 3, 2, 3);
    art.flash = gs::uploadMipped(vdp, flash);
    Bitmap bullet(6, 10);
    bullet.rect(2, 0, 2, 10, 1);
    art.bullet = gs::uploadMipped(vdp, bullet);
    Bitmap tracer(6, 14);
    tracer.rect(2, 0, 2, 14, 2);
    tracer.rect(2, 8, 2, 6, 4);
    art.tracer = gs::uploadMipped(vdp, tracer);
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.prop[0] = gs::uploadMipped(vdp, propTree());
    art.prop[1] = gs::uploadMipped(vdp, propRuin());
    art.prop[2] = gs::uploadMipped(vdp, propCrater());
    art.prop[3] = gs::uploadMipped(vdp, propBarn());

    vdp.A.enabled = false;
    vdp.B.enabled = false;
}

}  // namespace baron

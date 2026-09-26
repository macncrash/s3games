#include "game/art.h"

#include <cmath>

namespace maga {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap manStand() {
    Bitmap b(60, 96);
    b.rect(18, 34, 24, 36, 1);
    b.rect(18, 34, 8, 36, 2);
    b.rect(20, 58, 20, 4, 7);
    b.poly({{22, 30}, {30, 22}, {38, 30}, {36, 40}, {24, 40}}, 7);
    b.rect(22, 66, 7, 18, 8);
    b.rect(31, 66, 7, 18, 8);
    b.rect(20, 82, 10, 8, 9);
    b.rect(31, 82, 10, 8, 9);
    b.ellipse(30, 20, 8, 9, 4);
    b.ellipse(30, 22, 6, 7, 3);
    b.rect(22, 16, 16, 4, 4);
    b.set(27, 21, 10);
    b.set(33, 21, 10);
    b.rect(16, 40, 6, 14, 3);
    b.rect(38, 40, 6, 14, 2);
    b.line(14, 58, 48, 36, 5, 3.0f);
    b.line(16, 56, 46, 38, 6, 1.5f);
    b.rect(44, 32, 6, 4, 6);
    b.outline(15, false);
    return b;
}

Bitmap manCrouch() {
    Bitmap b(56, 64);
    b.ellipse(28, 16, 8, 8, 4);
    b.ellipse(28, 18, 6, 6, 3);
    b.rect(20, 12, 16, 4, 4);
    b.set(25, 17, 10);
    b.set(31, 17, 10);
    b.poly({{12, 36}, {28, 22}, {46, 34}, {44, 48}, {14, 50}}, 1);
    b.poly({{12, 36}, {22, 28}, {24, 48}, {14, 50}}, 2);
    b.rect(14, 46, 14, 8, 8);
    b.rect(30, 46, 12, 8, 8);
    b.rect(12, 52, 12, 6, 9);
    b.rect(32, 52, 12, 6, 9);
    b.line(10, 40, 46, 30, 5, 3.0f);
    b.line(12, 38, 44, 32, 6, 1.5f);
    b.outline(15, false);
    return b;
}

Bitmap manFallen() {
    Bitmap b(92, 34);
    b.ellipse(16, 16, 8, 7, 4);
    b.ellipse(17, 17, 5, 5, 3);
    b.rect(22, 12, 40, 12, 1);
    b.rect(22, 12, 40, 4, 2);
    b.rect(58, 14, 14, 8, 8);
    b.rect(70, 16, 10, 6, 9);
    b.rect(24, 22, 28, 5, 2);
    b.line(30, 26, 62, 30, 5, 2.0f);
    b.outline(15, false);
    return b;
}

Bitmap wallArt() {
    Bitmap b(52, 40);
    b.rect(4, 16, 18, 20, 2);
    b.rect(18, 10, 20, 26, 1);
    b.rect(32, 18, 16, 18, 3);
    b.rect(8, 20, 8, 6, 5);
    b.rect(22, 16, 10, 6, 4);
    b.rect(36, 24, 8, 5, 2);
    b.rect(6, 32, 40, 5, 3);
    b.outline(15, false);
    return b;
}

Bitmap poleArt() {
    Bitmap b(16, 72);
    b.rect(6, 4, 4, 64, 3);
    b.rect(5, 4, 2, 64, 2);
    b.rect(4, 2, 8, 6, 4);
    b.rect(3, 62, 10, 6, 3);
    return b;
}

Bitmap postArt() {
    Bitmap b(48, 168);
    b.rect(4, 8, 22, 152, 2);
    b.rect(6, 8, 8, 152, 1);
    b.rect(8, 8, 3, 152, 9);
    for (int y = 20; y < 150; y += 26) b.rect(4, y, 22, 4, 4);
    b.rect(4, 8, 22, 6, 4);
    b.rect(4, 154, 22, 8, 3);
    for (int i = 0; i < 5; i++) b.rect(28 + i * 4, 18, 2, 132, i & 1 ? 5 : 4);
    b.rect(26, 18, 20, 4, 4);
    b.rect(26, 146, 20, 4, 4);
    b.rect(26, 78, 20, 3, 5);
    return b;
}

Bitmap beamArt() {
    Bitmap b(320, 26);
    b.rect(0, 0, 320, 26, 3);
    b.rect(0, 2, 320, 6, 2);
    b.rect(0, 8, 320, 6, 1);
    b.rect(0, 14, 320, 5, 2);
    b.rect(0, 19, 320, 5, 3);
    for (int x = 14; x < 320; x += 26) {
        b.rect(x, 5, 3, 3, 4);
        b.rect(x, 15, 3, 3, 4);
    }
    for (int x = 40; x < 300; x += 48) b.rect(x, 9, 8, 3, 9);
    return b;
}

Bitmap bagArt() {
    Bitmap b(80, 36);
    b.ellipse(22, 22, 18, 10, 2);
    b.ellipse(42, 20, 20, 11, 1);
    b.ellipse(60, 22, 16, 9, 2);
    b.ellipse(32, 14, 16, 8, 1);
    b.ellipse(52, 13, 14, 7, 3);
    b.line(28, 14, 40, 12, 5, 1.0f);
    b.line(46, 13, 58, 12, 5, 1.0f);
    return b;
}

Bitmap rifleArt() {
    Bitmap b(36, 78);
    b.rect(14, 28, 8, 44, 2);
    b.rect(16, 30, 4, 28, 1);
    b.rect(15, 6, 6, 28, 4);
    b.rect(17, 4, 2, 8, 5);
    b.rect(10, 26, 16, 10, 3);
    b.rect(12, 28, 12, 4, 5);
    b.rect(8, 48, 6, 16, 1);
    b.poly({{14, 68}, {22, 68}, {26, 76}, {10, 76}}, 2);
    return b;
}

Bitmap sightArt() {
    Bitmap b(40, 40);
    const float cx = 20, cy = 20;
    for (int i = 0; i < 64; i++) {
        if (i % 16 < 3) continue;
        float a = i * 6.2831853f / 64.f;
        b.set(int(std::lround(cx + std::cos(a) * 14)), int(std::lround(cy + std::sin(a) * 14)), 4);
        b.set(int(std::lround(cx + std::cos(a) * 13)), int(std::lround(cy + std::sin(a) * 13)), 4);
    }
    b.rect(19, 2, 2, 6, 4);
    b.rect(19, 32, 2, 6, 4);
    b.rect(2, 19, 6, 2, 4);
    b.rect(32, 19, 6, 2, 4);
    return b;
}

Bitmap roundArt() {
    Bitmap b(12, 28);
    b.rect(2, 2, 8, 22, 2);
    b.rect(3, 3, 6, 14, 1);
    b.rect(3, 3, 6, 5, 3);
    b.rect(4, 8, 2, 8, 1);
    b.rect(3, 20, 6, 3, 10);
    return b;
}

Bitmap spentArt() {
    Bitmap b(12, 28);
    b.rect(3, 4, 6, 16, 9);
    b.rect(4, 6, 4, 12, 10);
    b.rect(3, 18, 6, 3, 9);
    return b;
}

Bitmap flashArt() {
    Bitmap b(24, 24);
    b.ellipse(12, 12, 10, 8, 6);
    b.ellipse(12, 12, 5, 4, 5);
    b.rect(11, 2, 2, 20, 5);
    b.rect(2, 11, 20, 2, 5);
    return b;
}

Bitmap sparkArt() {
    Bitmap b(20, 20);
    b.ellipse(10, 10, 8, 6, 7);
    b.ellipse(10, 10, 4, 3, 8);
    b.set(4, 4, 8);
    b.set(15, 6, 7);
    b.set(6, 14, 8);
    return b;
}

Bitmap moonArt() {
    Bitmap b(32, 32);
    b.ellipse(16, 16, 13, 13, 1);
    b.ellipse(12, 12, 7, 8, 2);
    b.ellipse(18, 18, 3, 3, 2);
    b.ellipse(11, 20, 2, 2, 2);
    return b;
}

Bitmap lanternArt() {
    Bitmap b(16, 22);
    b.rect(7, 0, 2, 4, 5);
    b.rect(4, 4, 8, 12, 6);
    b.rect(6, 6, 4, 8, 7);
    b.rect(3, 16, 10, 3, 4);
    return b;
}

Bitmap glowArt() {
    Bitmap b(32, 32);
    b.ellipse(16, 16, 14, 12, 3);
    b.ellipse(16, 16, 7, 6, 4);
    return b;
}

Bitmap starArt() {
    Bitmap b(3, 3);
    b.set(1, 0, 1);
    b.set(0, 1, 1);
    b.set(1, 1, 1);
    b.set(2, 1, 1);
    b.set(1, 2, 1);
    return b;
}

Bitmap barArt() {
    Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 1);
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

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    const uint16_t ink = gs::rgb4(15, 14, 12);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(8, 7, 6), gs::rgb4(15, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 12, 4), gs::rgb4(12, 8, 2), gs::rgb4(15, 15, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 4, 3), gs::rgb4(10, 2, 2), gs::rgb4(15, 10, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(8, 15, 7), gs::rgb4(3, 9, 4), gs::rgb4(14, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    auto coat = [&](int pal, uint16_t hi, uint16_t mid) {
        setPal(vdp, pal,
               {0, hi, mid, gs::rgb4(12, 8, 6), gs::rgb4(3, 3, 4), gs::rgb4(8, 5, 2), gs::rgb4(10, 10, 11),
                gs::rgb4(6, 4, 2), gs::rgb4(4, 4, 6), gs::rgb4(2, 2, 2), gs::rgb4(1, 1, 1), 0, 0, 0, 0, shadow});
    };
    coat(PAL_MAN, gs::rgb4(7, 8, 10), gs::rgb4(4, 5, 7));
    coat(PAL_MANB, gs::rgb4(12, 5, 3), gs::rgb4(8, 3, 2));

    setPal(vdp, PAL_WOOD,
           {0, gs::rgb4(8, 6, 3), gs::rgb4(5, 4, 2), gs::rgb4(3, 2, 1), gs::rgb4(7, 7, 8), gs::rgb4(3, 3, 4),
            gs::rgb4(14, 10, 3), gs::rgb4(15, 14, 8), gs::rgb4(6, 5, 3), gs::rgb4(10, 8, 5), 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_STONE,
           {0, gs::rgb4(8, 8, 7), gs::rgb4(5, 5, 4), gs::rgb4(3, 3, 3), gs::rgb4(6, 7, 5), gs::rgb4(4, 4, 3),
            0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FX,
           {0, gs::rgb4(14, 11, 4), gs::rgb4(9, 7, 3), gs::rgb4(13, 8, 4), gs::rgb4(15, 13, 6), gs::rgb4(15, 15, 13),
            gs::rgb4(15, 8, 2), gs::rgb4(9, 8, 6), gs::rgb4(14, 12, 8), gs::rgb4(4, 4, 4), gs::rgb4(2, 2, 2), 0, 0, 0, 0,
            shadow});
    setPal(vdp, PAL_NIGHT,
           {0, gs::rgb4(14, 14, 12), gs::rgb4(10, 10, 8), gs::rgb4(12, 8, 3), gs::rgb4(14, 6, 2), 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, shadow});

    const uint16_t field[16] = {
        0,
        gs::rgb4(6, 5, 3), gs::rgb4(4, 4, 2), gs::rgb4(7, 6, 4),
        gs::rgb4(8, 7, 4), gs::rgb4(5, 4, 3),
        gs::rgb4(9, 7, 5), gs::rgb4(6, 5, 3),
        gs::rgb4(5, 4, 3), gs::rgb4(7, 6, 4), gs::rgb4(8, 6, 4),
        gs::rgb4(3, 3, 4), gs::rgb4(3, 3, 4), gs::rgb4(4, 4, 5),
        gs::rgb4(10, 8, 5), gs::rgb4(4, 3, 2),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_ROAD * 16 + i, field[i]);

    loadFont(vdp, art);
    art.stand = gs::uploadMipped(vdp, manStand());
    art.crouch = gs::uploadMipped(vdp, manCrouch());
    art.fallen = gs::uploadMipped(vdp, manFallen());
    art.wall = gs::uploadMipped(vdp, wallArt());
    art.pole = gs::uploadMipped(vdp, poleArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.beam = gs::uploadMipped(vdp, beamArt());
    art.bag = gs::uploadMipped(vdp, bagArt());
    art.rifle = gs::uploadMipped(vdp, rifleArt());
    art.sight = gs::uploadMipped(vdp, sightArt());
    art.round = gs::uploadMipped(vdp, roundArt());
    art.spent = gs::uploadMipped(vdp, spentArt());
    art.flash = gs::uploadMipped(vdp, flashArt());
    art.spark = gs::uploadMipped(vdp, sparkArt());
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.lantern = gs::uploadMipped(vdp, lanternArt());
    art.glow = gs::uploadMipped(vdp, glowArt());
    art.star = gs::uploadMipped(vdp, starArt());
    art.bar = gs::uploadMipped(vdp, barArt());

    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.setFogColor(gs::rgb4(8, 5, 4));
}

}  // namespace maga

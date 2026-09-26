#include "game/art.h"

#include <cmath>
#include <string>

namespace grass {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink, uint16_t alt = 0) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, ink);
    vdp.setColor(pal * 16 + 2, alt ? alt : ink);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

// Rear three-quarter of a yellow high-wing taildragger. Prop frame 0..2.
Bitmap cubArt(int prop) {
    Bitmap b(160, 124);
    b.poly({{6, 36}, {154, 36}, {146, 52}, {14, 52}}, 2);
    b.poly({{18, 38}, {142, 38}, {136, 46}, {24, 46}}, 1);
    b.line(38, 50, 50, 76, 10, 2.2f);
    b.line(122, 50, 110, 76, 10, 2.2f);
    b.poly({{66, 44}, {94, 44}, {100, 86}, {60, 86}}, 2);
    b.poly({{72, 48}, {88, 48}, {90, 68}, {70, 68}}, 1);
    b.ellipse(80, 44, 16, 13, 3);
    b.ellipse(80, 42, 8, 7, 11);
    if (prop == 0) {
        b.line(80, 22, 80, 64, 6, 3.2f);
        b.ellipse(80, 22, 3, 3, 9);
        b.ellipse(80, 64, 3, 3, 9);
    } else if (prop == 1) {
        b.line(62, 28, 98, 58, 6, 3.f);
        b.line(98, 28, 62, 58, 6, 3.f);
    } else {
        b.ellipse(80, 42, 7, 20, 6);
        b.ellipse(80, 42, 3, 8, 9);
    }
    b.poly({{74, 52}, {88, 52}, {90, 63}, {72, 63}}, 8);
    b.line(78, 52, 78, 63, 5, 1.2f);
    b.line(68, 74, 48, 90, 10, 2.4f);
    b.line(92, 74, 112, 90, 10, 2.4f);
    b.ellipse(46, 94, 10, 10, 4);
    b.ellipse(114, 94, 10, 10, 4);
    b.ellipse(46, 94, 4, 4, 9);
    b.ellipse(114, 94, 4, 4, 9);
    b.poly({{78, 64}, {96, 74}, {94, 116}, {70, 116}, {66, 82}}, 2);
    b.poly({{82, 88}, {90, 92}, {89, 110}, {80, 110}}, 7);
    b.poly({{26, 98}, {134, 98}, {128, 112}, {32, 112}}, 2);
    b.poly({{36, 101}, {124, 101}, {120, 108}, {40, 108}}, 1);
    b.rect(74, 100, 12, 4, 4);
    b.ellipse(80, 116, 4, 4, 4);
    b.outline(4, false);
    return b;
}

Bitmap treeArt(int kind) {
    Bitmap b(52, 76);
    b.rect(23, 44, 7, 28, 4);
    b.rect(25, 46, 2, 22, 5);
    b.ellipse(26, 30, 20, 18, 1);
    b.ellipse(16, 34, 12, 11, 2);
    b.ellipse(34, 26, 11, 10, kind ? 5 : 3);
    b.ellipse(24, 22, 7, 6, 3);
    b.outline(6, false);
    return b;
}

Bitmap postArt() {
    Bitmap b(16, 40);
    b.rect(7, 14, 2, 24, 3);
    b.rect(1, 2, 14, 14, 1);
    b.rect(1, 2, 14, 3, 2);
    b.rect(1, 13, 14, 2, 2);
    return b;
}

Bitmap sockArt(int frame) {
    Bitmap b(56, 72);
    b.rect(8, 18, 3, 50, 4);
    b.ellipse(9, 16, 4, 4, 3);
    float tip = 22.f + frame * 8.f;
    float droop = 6.f + frame * 5.f;
    b.poly({{11, 20}, {tip, 16 + droop}, {tip - 2, 28 + droop}, {11, 32}}, 1);
    b.poly({{11, 22}, {tip - 8, 18 + droop}, {tip - 10, 26 + droop}, {11, 30}}, 2);
    b.poly({{11, 24}, {18, 22 + droop * 0.3f}, {18, 28 + droop * 0.3f}, {11, 29}}, 5);
    b.outline(4, false);
    return b;
}

Bitmap hangarArt() {
    Bitmap b(104, 72);
    b.rect(8, 28, 88, 40, 1);
    b.poly({{4, 30}, {52, 8}, {100, 30}}, 3);
    b.poly({{14, 28}, {52, 12}, {90, 28}}, 4);
    b.rect(40, 40, 24, 28, 5);
    b.rect(16, 36, 14, 10, 6);
    b.rect(74, 36, 14, 10, 6);
    b.rect(18, 38, 10, 6, 2);
    b.rect(76, 38, 10, 6, 2);
    b.rect(8, 26, 88, 3, 7);
    b.outline(5, false);
    return b;
}

Bitmap coneArt() {
    Bitmap b(18, 28);
    b.poly({{9, 2}, {16, 22}, {2, 22}}, 1);
    b.poly({{9, 8}, {13, 22}, {5, 22}}, 3);
    b.rect(3, 20, 12, 3, 2);
    b.rect(7, 23, 4, 4, 4);
    return b;
}

Bitmap barArt() {
    Bitmap b(72, 14);
    for (int x = 0; x < 72; x++) {
        int c = ((x / 9) & 1) ? 1 : 2;
        for (int y = 2; y < 12; y++) b.set(x, y, c);
    }
    return b;
}

Bitmap cloudArt() {
    Bitmap b(96, 40);
    b.ellipse(30, 22, 24, 13, 1);
    b.ellipse(52, 16, 28, 15, 1);
    b.ellipse(74, 22, 18, 11, 2);
    b.ellipse(44, 14, 14, 8, 2);
    return b;
}

Bitmap sunArt() {
    Bitmap b(44, 44);
    b.ellipse(22, 22, 11, 11, 3);
    b.ellipse(19, 19, 5, 5, 5);
    for (int i = 0; i < 8; i++) {
        float a = i * 6.2831853f / 8.f;
        b.line(22 + std::cos(a) * 14, 22 + std::sin(a) * 14, 22 + std::cos(a) * 20, 22 + std::sin(a) * 20, 4, 2.f);
    }
    return b;
}

Bitmap hillArt() {
    Bitmap b(128, 52);
    b.poly({{0, 52}, {0, 30}, {24, 18}, {48, 28}, {78, 8}, {104, 22}, {128, 16}, {128, 52}}, 1);
    b.poly({{10, 52}, {28, 32}, {52, 36}, {80, 20}, {110, 30}, {120, 52}}, 2);
    return b;
}

Bitmap birdArt(bool up) {
    Bitmap b(40, 18);
    if (up) {
        b.line(2, 14, 20, 4, 1, 2.2f);
        b.line(20, 4, 38, 14, 1, 2.2f);
    } else {
        b.line(2, 4, 20, 12, 1, 2.2f);
        b.line(20, 12, 38, 4, 1, 2.2f);
    }
    b.ellipse(20, 9, 3, 2, 2);
    return b;
}

Bitmap dustArt() {
    Bitmap b(28, 20);
    b.ellipse(14, 11, 12, 7, 2);
    b.ellipse(9, 10, 5, 4, 1);
    b.ellipse(18, 9, 4, 3, 3);
    return b;
}

Bitmap shadowArt() {
    Bitmap b(48, 16);
    b.ellipse(24, 8, 22, 6, 1);
    return b;
}

Bitmap chevronArt() {
    Bitmap b(28, 24);
    b.line(4, 18, 14, 4, 1, 3.f);
    b.line(14, 4, 24, 18, 1, 3.f);
    b.line(8, 18, 14, 10, 2, 2.f);
    b.line(14, 10, 20, 18, 2, 2.f);
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
    textPal(vdp, PAL_HUD, gs::rgb4(15, 15, 15), gs::rgb4(10, 12, 12));
    textPal(vdp, PAL_AMBER, gs::rgb4(15, 12, 3), gs::rgb4(15, 14, 8));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 4, 3), gs::rgb4(12, 6, 4));
    textPal(vdp, PAL_GOOD, gs::rgb4(6, 15, 6), gs::rgb4(12, 15, 10));

    setPal(vdp, PAL_SHIP, {0, gs::rgb4(15, 14, 8), gs::rgb4(15, 11, 2), gs::rgb4(12, 7, 1), gs::rgb4(1, 1, 1),
                           gs::rgb4(15, 15, 14), gs::rgb4(5, 5, 6), gs::rgb4(13, 2, 1), gs::rgb4(5, 9, 12),
                           gs::rgb4(9, 9, 10), gs::rgb4(4, 3, 2), gs::rgb4(9, 6, 1)});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(2, 7, 2), gs::rgb4(3, 10, 3), gs::rgb4(5, 13, 4), gs::rgb4(7, 4, 2),
                           gs::rgb4(8, 5, 3), gs::rgb4(8, 14, 6), gs::rgb4(1, 2, 1)});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(15, 15, 14), gs::rgb4(12, 12, 11), gs::rgb4(6, 4, 2)});
    setPal(vdp, PAL_BARN, {0, gs::rgb4(14, 13, 10), gs::rgb4(11, 12, 13), gs::rgb4(3, 8, 3), gs::rgb4(2, 5, 2),
                           gs::rgb4(2, 2, 2), gs::rgb4(8, 10, 12), gs::rgb4(12, 3, 2)});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(15, 15, 15), gs::rgb4(12, 13, 15), gs::rgb4(15, 13, 4), gs::rgb4(15, 10, 3),
                          gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_DUST, {0, gs::rgb4(12, 11, 7), gs::rgb4(9, 8, 4), gs::rgb4(6, 5, 3)});
    setPal(vdp, PAL_BIRD, {0, gs::rgb4(2, 2, 3), gs::rgb4(6, 4, 2)});
    setPal(vdp, PAL_CONE, {0, gs::rgb4(15, 8, 1), gs::rgb4(15, 15, 14), gs::rgb4(12, 5, 1), gs::rgb4(4, 3, 2)});

    // Road bank: 1-3 meadow, 4-5 verge, 6-7 mowed strip, 14 white paint.
    setPal(vdp, PAL_FIELD,
           {0, gs::rgb4(5, 11, 3), gs::rgb4(3, 8, 2), gs::rgb4(7, 12, 4), gs::rgb4(4, 12, 3), gs::rgb4(2, 8, 2),
            gs::rgb4(8, 14, 4), gs::rgb4(4, 11, 3), gs::rgb4(13, 12, 5), gs::rgb4(6, 9, 3), gs::rgb4(9, 14, 5),
            gs::rgb4(3, 7, 2), gs::rgb4(2, 6, 2), gs::rgb4(1, 4, 1), gs::rgb4(15, 15, 14), gs::rgb4(2, 7, 2)});

    loadFont(vdp, art);
    for (int i = 0; i < 3; i++) art.cub[i] = gs::uploadMipped(vdp, cubArt(i));
    art.tree[0] = gs::uploadMipped(vdp, treeArt(0));
    art.tree[1] = gs::uploadMipped(vdp, treeArt(1));
    art.post = gs::uploadMipped(vdp, postArt());
    for (int i = 0; i < 3; i++) art.sock[i] = gs::uploadMipped(vdp, sockArt(i));
    art.hangar = gs::uploadMipped(vdp, hangarArt());
    art.cone = gs::uploadMipped(vdp, coneArt());
    art.bar = gs::uploadMipped(vdp, barArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.hill = gs::uploadMipped(vdp, hillArt());
    art.bird[0] = gs::uploadMipped(vdp, birdArt(false));
    art.bird[1] = gs::uploadMipped(vdp, birdArt(true));
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.chevron = gs::uploadMipped(vdp, chevronArt());
}

}  // namespace grass

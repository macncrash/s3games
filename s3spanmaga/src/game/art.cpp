#include "game/art.h"

namespace spanmaga {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap manStand() {
    Bitmap b(48, 88);
    b.rect(16, 8, 16, 6, 5);
    b.ellipse(24, 18, 8, 8, 4);
    b.ellipse(24, 19, 6, 6, 3);
    b.set(21, 18, 10);
    b.set(27, 18, 10);
    b.rect(14, 26, 20, 32, 1);
    b.rect(14, 26, 7, 32, 2);
    b.rect(16, 48, 16, 4, 6);
    b.rect(18, 56, 6, 20, 8);
    b.rect(26, 56, 6, 20, 8);
    b.rect(16, 74, 9, 6, 9);
    b.rect(26, 74, 9, 6, 9);
    b.rect(8, 30, 6, 16, 2);
    b.rect(34, 30, 6, 14, 1);
    b.line(10, 46, 40, 28, 7, 2.5f);
    b.rect(38, 24, 6, 5, 6);
    b.outline(15, false);
    return b;
}

Bitmap manBrace() {
    Bitmap b(52, 56);
    b.rect(16, 4, 16, 5, 5);
    b.ellipse(26, 14, 8, 7, 4);
    b.ellipse(26, 15, 6, 5, 3);
    b.set(23, 14, 10);
    b.set(29, 14, 10);
    b.poly({{8, 30}, {26, 20}, {46, 28}, {44, 42}, {10, 44}}, 1);
    b.poly({{8, 30}, {18, 24}, {16, 42}, {10, 44}}, 2);
    b.rect(10, 40, 14, 8, 8);
    b.rect(28, 40, 14, 8, 8);
    b.rect(8, 46, 12, 5, 9);
    b.rect(30, 46, 12, 5, 9);
    b.line(6, 34, 44, 26, 7, 2.5f);
    b.outline(15, false);
    return b;
}

Bitmap manFallen() {
    Bitmap b(88, 30);
    b.ellipse(14, 14, 8, 7, 4);
    b.rect(8, 6, 12, 4, 5);
    b.rect(20, 10, 40, 12, 1);
    b.rect(20, 10, 40, 4, 2);
    b.rect(56, 12, 16, 8, 8);
    b.rect(70, 14, 10, 5, 9);
    b.line(28, 22, 64, 26, 7, 2.0f);
    b.outline(15, false);
    return b;
}

Bitmap trussArt() {
    Bitmap b(28, 96);
    b.rect(2, 4, 6, 88, 1);
    b.rect(20, 4, 6, 88, 2);
    b.rect(2, 4, 24, 6, 3);
    b.rect(2, 86, 24, 6, 3);
    b.rect(2, 44, 24, 5, 4);
    b.line(6, 10, 22, 44, 5, 2.0f);
    b.line(22, 10, 6, 44, 5, 2.0f);
    b.line(6, 50, 22, 84, 5, 2.0f);
    b.line(22, 50, 6, 84, 5, 2.0f);
    for (int y = 12; y < 84; y += 14) {
        b.rect(3, y, 3, 3, 6);
        b.rect(22, y, 3, 3, 6);
    }
    return b;
}

Bitmap pierArt() {
    Bitmap b(36, 72);
    b.poly({{6, 8}, {30, 8}, {34, 68}, {2, 68}}, 2);
    b.poly({{8, 10}, {18, 10}, {16, 66}, {4, 66}}, 1);
    b.rect(4, 4, 28, 8, 3);
    b.rect(2, 64, 32, 6, 4);
    for (int y = 18; y < 60; y += 12) b.rect(8, y, 20, 3, 5);
    return b;
}

Bitmap lampArt() {
    Bitmap b(14, 28);
    b.rect(6, 0, 2, 10, 2);
    b.rect(3, 10, 8, 10, 4);
    b.rect(5, 12, 4, 6, 5);
    b.rect(2, 20, 10, 3, 3);
    b.rect(5, 23, 4, 4, 1);
    return b;
}

Bitmap cableArt() {
    Bitmap b(64, 10);
    b.line(0, 2, 64, 8, 1, 2.0f);
    b.line(0, 4, 64, 9, 2, 1.0f);
    return b;
}

Bitmap abutmentArt() {
    Bitmap b(40, 140);
    b.rect(6, 8, 22, 124, 2);
    b.rect(8, 8, 8, 124, 1);
    b.rect(10, 8, 3, 124, 6);
    for (int y = 16; y < 124; y += 18) b.rect(6, y, 22, 4, 4);
    b.rect(4, 4, 26, 8, 3);
    b.rect(2, 128, 30, 8, 4);
    b.rect(28, 20, 8, 100, 5);
    return b;
}

Bitmap rifleArt() {
    Bitmap b(30, 72);
    b.rect(12, 8, 6, 40, 2);
    b.rect(14, 6, 2, 10, 4);
    b.rect(10, 36, 12, 8, 1);
    b.rect(13, 44, 5, 18, 3);
    b.poly({{12, 60}, {19, 60}, {23, 70}, {8, 70}}, 2);
    b.rect(6, 48, 5, 12, 5);
    return b;
}

Bitmap sightArt() {
    Bitmap b(18, 18);
    b.rect(8, 0, 2, 5, 1);
    b.rect(8, 13, 2, 5, 1);
    b.rect(0, 8, 5, 2, 1);
    b.rect(13, 8, 5, 2, 1);
    b.set(9, 9, 5);
    return b;
}

Bitmap roundArt() {
    Bitmap b(8, 16);
    b.rect(2, 4, 4, 10, 1);
    b.rect(2, 2, 4, 3, 4);
    b.rect(3, 12, 2, 2, 2);
    return b;
}

Bitmap spentArt() {
    Bitmap b(8, 16);
    b.rect(2, 5, 4, 8, 3);
    b.rect(3, 12, 2, 2, 2);
    return b;
}

Bitmap flashArt() {
    Bitmap b(16, 16);
    b.poly({{8, 0}, {10, 6}, {16, 8}, {10, 10}, {8, 16}, {6, 10}, {0, 8}, {6, 6}}, 5);
    b.ellipse(8, 8, 3, 3, 4);
    return b;
}

Bitmap sparkArt() {
    Bitmap b(8, 8);
    b.ellipse(4, 4, 3, 3, 4);
    b.set(4, 4, 5);
    return b;
}

Bitmap barArt() {
    Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 1);
    return b;
}

Bitmap moonArt() {
    Bitmap b(28, 28);
    b.ellipse(14, 14, 12, 12, 1);
    b.ellipse(10, 12, 6, 7, 2);
    b.ellipse(16, 16, 2, 2, 2);
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
    const uint16_t ink = gs::rgb4(14, 15, 15);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(6, 8, 9), gs::rgb4(15, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 11, 3), gs::rgb4(10, 7, 2), gs::rgb4(15, 14, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 4, 3), gs::rgb4(9, 2, 2), gs::rgb4(15, 10, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(6, 15, 9), gs::rgb4(2, 8, 6), gs::rgb4(13, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    auto coat = [&](int pal, uint16_t hi, uint16_t mid) {
        setPal(vdp, pal,
               {0, hi, mid, gs::rgb4(12, 9, 7), gs::rgb4(4, 3, 3), gs::rgb4(2, 2, 3), gs::rgb4(9, 8, 6),
                gs::rgb4(5, 4, 3), gs::rgb4(3, 3, 5), gs::rgb4(2, 2, 2), gs::rgb4(1, 1, 1), 0, 0, 0, 0, shadow});
    };
    coat(PAL_COAT, gs::rgb4(5, 8, 9), gs::rgb4(3, 5, 6));
    coat(PAL_COATB, gs::rgb4(10, 6, 3), gs::rgb4(7, 4, 2));

    setPal(vdp, PAL_STEEL,
           {0, gs::rgb4(9, 10, 11), gs::rgb4(5, 6, 7), gs::rgb4(3, 4, 5), gs::rgb4(7, 8, 8), gs::rgb4(12, 8, 3),
            gs::rgb4(14, 12, 6), gs::rgb4(4, 3, 2), gs::rgb4(6, 6, 7), 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_PIER,
           {0, gs::rgb4(8, 8, 7), gs::rgb4(5, 5, 5), gs::rgb4(3, 3, 3), gs::rgb4(6, 6, 5), gs::rgb4(4, 4, 4),
            gs::rgb4(11, 11, 10), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FX,
           {0, gs::rgb4(14, 12, 5), gs::rgb4(8, 7, 4), gs::rgb4(6, 6, 6), gs::rgb4(15, 14, 8), gs::rgb4(15, 15, 12),
            gs::rgb4(15, 9, 3), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_DUSK,
           {0, gs::rgb4(14, 14, 12), gs::rgb4(9, 10, 12), gs::rgb4(12, 8, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    const uint16_t deck[16] = {
        0,
        gs::rgb4(2, 4, 7), gs::rgb4(1, 3, 6), gs::rgb4(4, 7, 10),
        gs::rgb4(6, 4, 3), gs::rgb4(4, 3, 2),
        gs::rgb4(7, 8, 8), gs::rgb4(5, 6, 6),
        gs::rgb4(4, 4, 4), gs::rgb4(6, 6, 6), gs::rgb4(8, 8, 7),
        gs::rgb4(2, 5, 9), gs::rgb4(1, 3, 7), gs::rgb4(5, 9, 12),
        gs::rgb4(13, 11, 4), gs::rgb4(9, 9, 8),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_ROAD * 16 + i, deck[i]);

    loadFont(vdp, art);
    art.stand = gs::uploadMipped(vdp, manStand());
    art.brace = gs::uploadMipped(vdp, manBrace());
    art.fallen = gs::uploadMipped(vdp, manFallen());
    art.truss = gs::uploadMipped(vdp, trussArt());
    art.pier = gs::uploadMipped(vdp, pierArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.cable = gs::uploadMipped(vdp, cableArt());
    art.abutment = gs::uploadMipped(vdp, abutmentArt());
    art.rifle = gs::uploadMipped(vdp, rifleArt());
    art.sight = gs::uploadMipped(vdp, sightArt());
    art.round = gs::uploadMipped(vdp, roundArt());
    art.spent = gs::uploadMipped(vdp, spentArt());
    art.flash = gs::uploadMipped(vdp, flashArt());
    art.spark = gs::uploadMipped(vdp, sparkArt());
    art.bar = gs::uploadMipped(vdp, barArt());
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.star = gs::uploadMipped(vdp, starArt());
}

}  // namespace spanmaga

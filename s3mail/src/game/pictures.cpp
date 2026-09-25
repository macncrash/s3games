#include "game/pictures.h"

#include <algorithm>
#include <initializer_list>
#include <string>

namespace mail {
namespace {

using gs::Bitmap;
using gs::Pt;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap bikeArt(int lean) {
    Bitmap b(80, 96);
    const float L = float(lean) * 7.f;
    b.ellipse(40, 76, 15, 15, 1);
    b.ellipse(40, 76, 9, 9, 2);
    b.ellipse(40, 76, 3, 3, 3);
    b.line(40, 76, 40, 63, 2, 1.2f);
    b.line(40, 76, 28, 76, 2, 1.2f);
    b.line(40, 76, 52, 76, 2, 1.2f);
    b.line(40, 76, 31, 66, 2, 1.1f);
    b.line(40, 76, 49, 66, 2, 1.1f);
    b.ellipse(40, 52, 8, 10, 1);
    b.ellipse(40, 52, 4.5f, 5.5f, 2);
    b.line(40, 64, 40, 48, 3, 3.2f);
    b.line(40, 64, 27, 58, 3, 2.4f);
    b.line(40, 64, 53, 58, 3, 2.4f);
    b.line(22, 48, 58, 48, 11, 2.8f);
    b.ellipse(22, 48, 3.2f, 3.2f, 11);
    b.ellipse(58, 48, 3.2f, 3.2f, 11);
    b.rect(33 + L * 0.3f, 50, 14, 7, 12);
    b.rect(34 + L, 32, 14, 18, 5);
    b.rect(36 + L, 34, 4, 14, 6);
    b.rect(30 + L, 36, 20, 14, 9);
    b.rect(32 + L, 40, 16, 3, 14);
    b.line(34 + L, 36, 24, 48, 7, 3.2f);
    b.line(48 + L, 36, 56, 48, 7, 3.2f);
    b.ellipse(40 + L, 24, 7, 8, 7);
    b.ellipse(40 + L, 17, 8, 4, 8);
    b.rect(32 + L, 15, 16, 4, 8);
    b.rect(46 + L, 17, 8, 3, 8);
    b.outline(15, false);
    return b;
}

Bitmap boxArt(bool full) {
    Bitmap b(44, 58);
    b.rect(19, 30, 6, 24, 1);
    b.rect(14, 50, 16, 4, 1);
    b.ellipse(22, 18, 16, 7, 3);
    b.rect(6, 16, 32, 16, 2);
    b.rect(6, 16, 32, 5, 3);
    b.rect(10, 24, 22, 4, 4);
    if (full) b.rect(13, 25, 14, 2, 6);
    if (full) {
        b.rect(34, 4, 3, 16, 8);
        b.rect(34, 4, 8, 7, 5);
    } else {
        b.rect(34, 20, 3, 8, 8);
        b.rect(34, 20, 9, 4, 5);
    }
    b.outline(15, false);
    return b;
}

Bitmap houseArt(int variant) {
    Bitmap b(88, 80);
    const float doorX = variant == 1 ? 22.f : variant == 2 ? 52.f : 37.f;
    const float chimX = variant == 1 ? 18.f : 58.f;
    b.ellipse(44, 74, 30, 5, 9);
    b.poly({{8, 36}, {44, 8}, {80, 36}}, variant == 2 ? 4 : 3);
    b.poly({{18, 32}, {44, 14}, {70, 32}}, variant == 2 ? 3 : 4);
    b.rect(14, 34, 60, 36, 1);
    b.rect(14, 34, 8, 36, 2);
    b.rect(12, 32, 64, 4, 8);
    b.rect(chimX, 12, 10, 22, 11);
    b.rect(chimX - 1, 10, 12, 4, 11);
    b.rect(doorX, 48, 14, 22, 5);
    b.ellipse(doorX + 11, 60, 1.4f, 1.4f, 8);
    b.rect(20, 42, 12, 11, variant == 1 ? 7 : 6);
    b.rect(56, 42, 12, 11, variant == 0 ? 7 : 6);
    b.rect(25, 42, 2, 11, 8);
    b.rect(20, 46, 12, 2, 8);
    b.rect(61, 42, 2, 11, 8);
    b.rect(56, 46, 12, 2, 8);
    b.rect(doorX - 2, 68, 18, 4, 10);
    b.ellipse(18, 70, 8, 5, 9);
    b.ellipse(70, 70, 8, 5, 9);
    b.outline(15, false);
    return b;
}

Bitmap depotArt() {
    Bitmap b(104, 88);
    b.ellipse(52, 82, 36, 5, 10);
    b.rect(10, 36, 84, 42, 1);
    b.rect(10, 36, 10, 42, 2);
    b.rect(8, 32, 88, 8, 12);
    b.rect(8, 40, 88, 4, 8);
    b.poly({{8, 34}, {52, 10}, {96, 34}}, 4);
    b.rect(70, 14, 8, 22, 11);
    b.rect(68, 12, 12, 4, 11);
    b.rect(78, 8, 2, 28, 8);
    b.rect(74, 6, 10, 6, 13);
    b.rect(40, 52, 24, 26, 5);
    b.rect(18, 46, 14, 12, 7);
    b.rect(72, 46, 14, 12, 6);
    b.rect(24, 46, 2, 12, 8);
    b.rect(18, 51, 14, 2, 8);
    b.rect(36, 68, 32, 5, 10);
    b.ellipse(16, 76, 8, 4, 9);
    b.ellipse(88, 76, 8, 4, 9);
    b.outline(15, false);
    return b;
}

Bitmap treeArt() {
    Bitmap b(52, 84);
    b.rect(23, 52, 8, 26, 4);
    b.rect(23, 52, 3, 26, 5);
    b.ellipse(26, 48, 20, 14, 1);
    b.ellipse(26, 34, 16, 12, 2);
    b.ellipse(26, 22, 11, 9, 1);
    b.ellipse(18, 40, 6, 4, 3);
    b.ellipse(34, 30, 5, 3, 2);
    b.ellipse(26, 78, 10, 3, 5);
    b.outline(15, false);
    return b;
}

Bitmap lampArt() {
    Bitmap b(28, 72);
    b.rect(12, 28, 4, 40, 1);
    b.rect(8, 64, 12, 4, 1);
    b.rect(12, 24, 10, 3, 8);
    b.ellipse(20, 18, 7, 6, 3);
    b.ellipse(20, 18, 3, 3, 4);
    b.outline(15, false);
    return b;
}

Bitmap signArt() {
    Bitmap b(46, 52);
    b.rect(20, 26, 5, 22, 1);
    b.rect(4, 6, 38, 22, 2);
    b.rect(4, 6, 38, 4, 4);
    b.poly({{32, 11}, {14, 17}, {32, 23}}, 3);
    b.outline(15, false);
    return b;
}

Bitmap paperArt() {
    Bitmap b(22, 14);
    b.rect(1, 1, 20, 12, 1);
    b.rect(1, 1, 20, 3, 2);
    b.rect(1, 8, 20, 4, 3);
    b.rect(2, 9, 18, 1, 1);
    return b;
}

Bitmap puffArt() {
    Bitmap b(16, 16);
    b.ellipse(8, 9, 6, 4, 5);
    b.ellipse(5, 8, 3, 2, 1);
    b.ellipse(11, 8, 3, 2, 2);
    return b;
}

Bitmap cloudArt() {
    Bitmap b(56, 24);
    b.ellipse(20, 14, 14, 7, 1);
    b.ellipse(34, 12, 16, 8, 1);
    b.ellipse(28, 14, 10, 5, 2);
    return b;
}

Bitmap sunArt() {
    Bitmap b(32, 32);
    b.ellipse(16, 16, 12, 12, 3);
    b.ellipse(16, 16, 7, 7, 4);
    return b;
}

Bitmap shadowArt() {
    Bitmap b(64, 16);
    b.ellipse(32, 8, 26, 5, 1);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    gs::TextStyle big{3, 1, 0, 15, 1};
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (!g[y * 5 + x]) continue;
                px[y * 8 + x + 1] = 1;
                if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
            }
        }
        const int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 15), gs::rgb4(10, 11, 13), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 3), gs::rgb4(12, 8, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 4, 3), gs::rgb4(10, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(8, 15, 7), gs::rgb4(3, 9, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});

    setPal(vdp, PAL_BIKE,
           {0, gs::rgb4(1, 1, 1), gs::rgb4(9, 10, 12), gs::rgb4(3, 4, 6), gs::rgb4(6, 7, 9), gs::rgb4(13, 2, 2),
            gs::rgb4(8, 1, 1), gs::rgb4(13, 9, 6), gs::rgb4(2, 5, 9), gs::rgb4(12, 9, 4), gs::rgb4(15, 15, 13),
            gs::rgb4(11, 12, 13), gs::rgb4(2, 2, 5), gs::rgb4(4, 3, 2), gs::rgb4(14, 3, 2), ink});
    setPal(vdp, PAL_HOUSE,
           {0, gs::rgb4(14, 12, 9), gs::rgb4(10, 8, 5), gs::rgb4(12, 4, 3), gs::rgb4(8, 2, 2), gs::rgb4(6, 4, 2),
            gs::rgb4(7, 11, 13), gs::rgb4(15, 13, 6), gs::rgb4(15, 15, 14), gs::rgb4(2, 8, 3), gs::rgb4(7, 7, 6),
            gs::rgb4(5, 5, 6), gs::rgb4(3, 6, 11), gs::rgb4(13, 2, 2), gs::rgb4(15, 15, 14), ink});
    setPal(vdp, PAL_TREE,
           {0, gs::rgb4(1, 6, 2), gs::rgb4(3, 10, 3), gs::rgb4(1, 4, 2), gs::rgb4(6, 4, 2), gs::rgb4(4, 2, 1), 0, 0,
            0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BOX,
           {0, gs::rgb4(6, 4, 2), gs::rgb4(2, 5, 9), gs::rgb4(5, 8, 12), gs::rgb4(1, 1, 2), gs::rgb4(14, 2, 2),
            gs::rgb4(15, 15, 13), 0, gs::rgb4(8, 8, 9), 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_SIGN,
           {0, gs::rgb4(5, 5, 6), gs::rgb4(15, 12, 2), gs::rgb4(1, 1, 2), gs::rgb4(15, 15, 14), 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, ink});
    setPal(vdp, PAL_PAPER, {0, gs::rgb4(15, 15, 14), gs::rgb4(12, 12, 13), gs::rgb4(13, 2, 2), gs::rgb4(7, 7, 8), 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 15, 15), gs::rgb4(12, 13, 15), gs::rgb4(15, 12, 3), gs::rgb4(15, 15, 11),
                         gs::rgb4(14, 14, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});

    // Road bank: 1-3 grass, 4-5 sidewalk, 6-7 tarmac, 14 the yellow paint.
    const uint16_t road[16] = {
        0,
        gs::rgb4(5, 12, 4), gs::rgb4(2, 8, 2), gs::rgb4(8, 13, 5),
        gs::rgb4(12, 12, 11), gs::rgb4(8, 8, 7),
        gs::rgb4(6, 6, 7), gs::rgb4(4, 4, 5),
        gs::rgb4(9, 8, 7), gs::rgb4(5, 5, 6), gs::rgb4(7, 7, 8),
        gs::rgb4(3, 5, 8), gs::rgb4(4, 6, 9), gs::rgb4(5, 7, 10),
        gs::rgb4(15, 13, 2), gs::rgb4(8, 8, 9),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_ROAD * 16 + i, road[i]);
    vdp.setFogColor(gs::rgb4(13, 11, 8));
    vdp.A.enabled = false;
    vdp.B.enabled = false;

    loadFont(vdp, art);
    for (int i = 0; i < 3; i++) {
        art.bike[i] = gs::uploadMipped(vdp, bikeArt(i - 1));
        art.house[i] = gs::uploadMipped(vdp, houseArt(i));
    }
    art.boxShut = gs::uploadMipped(vdp, boxArt(false));
    art.boxOpen = gs::uploadMipped(vdp, boxArt(true));
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.sign = gs::uploadMipped(vdp, signArt());
    art.depot = gs::uploadMipped(vdp, depotArt());
    art.paper = gs::uploadMipped(vdp, paperArt());
    art.puff = gs::uploadMipped(vdp, puffArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
}

}  // namespace mail

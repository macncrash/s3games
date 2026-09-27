#include "game/art.h"

#include <algorithm>
#include <string>

namespace ferryturn {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, ink);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 2, 3));
}

// list -1..1 shears the deck in the picture. Bow stays to the right.
Hull drawHull(gs::VDP& vdp, float list) {
    Bitmap b(220, 120);
    const float cx = 110.f, cy = 60.f;
    const float shear = list * 22.f;
    b.poly({{cx - 86.f, cy - 16.f + shear * 0.15f},
            {cx + 78.f, cy - 12.f},
            {cx + 92.f, cy + 2.f},
            {cx + 70.f, cy + 18.f},
            {cx - 88.f, cy + 20.f - shear * 0.15f}},
           2);
    b.poly({{cx - 78.f, cy - 10.f + shear},
            {cx + 62.f, cy - 8.f + shear * 0.35f},
            {cx + 58.f, cy + 12.f + shear * 0.35f},
            {cx - 80.f, cy + 14.f + shear}},
           3);
    b.rect(cx - 70.f, cy - 2.f + shear * 0.6f, 18.f, 8.f, 5);
    b.rect(cx - 48.f, cy - 2.f + shear * 0.5f, 18.f, 8.f, 5);
    b.rect(cx - 26.f, cy - 2.f + shear * 0.35f, 18.f, 8.f, 6);
    b.rect(cx - 4.f, cy - 2.f + shear * 0.2f, 18.f, 8.f, 6);
    b.ellipse(cx + 28.f, cy + shear * 0.15f, 7.f, 9.f, 4);
    b.rect(cx + 24.f, cy - 16.f + shear * 0.1f, 8.f, 8.f, 7);
    b.rect(cx - 90.f, cy - 4.f, 8.f, 10.f, 8);
    b.line(cx - 86.f, cy - 18.f + shear * 0.2f, cx + 84.f, cy - 14.f, 1, 1.4f);
    Hull h;
    h.img = gs::uploadMipped(vdp, b.cropToContent(1));
    return h;
}

Bitmap buoyArt() {
    Bitmap b(28, 48);
    b.rect(12, 20, 4, 24, 3);
    b.ellipse(14, 14, 10, 12, 1);
    b.ellipse(14, 12, 4, 4, 2);
    b.rect(10, 22, 8, 4, 4);
    return b;
}

Bitmap postArt() {
    Bitmap b(16, 40);
    b.rect(6, 4, 4, 34, 1);
    b.rect(3, 2, 10, 5, 2);
    return b;
}

Bitmap dockArt() {
    Bitmap b(70, 28);
    b.rect(0, 6, 70, 16, 1);
    b.rect(0, 6, 70, 3, 2);
    for (int i = 0; i < 5; i++) b.rect(6 + i * 13, 12, 6, 6, 3);
    return b;
}

Bitmap carArt() {
    Bitmap b(22, 12);
    b.rect(2, 4, 18, 6, 1);
    b.rect(5, 2, 8, 3, 2);
    b.rect(3, 8, 4, 3, 3);
    b.rect(15, 8, 4, 3, 3);
    return b;
}

Bitmap wakeArt() {
    Bitmap b(18, 10);
    b.ellipse(9, 5, 8, 3.5f, 1);
    b.ellipse(9, 5, 4, 1.6f, 2);
    return b;
}

Bitmap gullArt() {
    Bitmap b(24, 12);
    b.poly({{2, 8}, {12, 4}, {22, 8}, {12, 6}}, 1);
    return b;
}

Bitmap slipArt() {
    Bitmap b(36, 64);
    b.rect(0, 0, 6, 64, 1);
    b.rect(30, 0, 6, 64, 1);
    b.rect(6, 28, 24, 8, 2);
    b.rect(10, 8, 16, 6, 3);
    return b;
}

Bitmap chevArt() {
    Bitmap b(20, 14);
    b.poly({{2, 2}, {16, 7}, {2, 12}, {6, 7}}, 1);
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
    textPal(vdp, PAL_HUD, gs::rgb4(15, 15, 14));
    textPal(vdp, PAL_AMBER, gs::rgb4(15, 12, 4));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 4, 3));
    textPal(vdp, PAL_GOOD, gs::rgb4(5, 15, 9));

    setPal(vdp, PAL_HULL, {0, gs::rgb4(14, 15, 15), gs::rgb4(4, 6, 8), gs::rgb4(12, 13, 14), gs::rgb4(12, 3, 2),
                           gs::rgb4(14, 12, 3), gs::rgb4(2, 6, 12), gs::rgb4(3, 3, 3), gs::rgb4(8, 5, 3)});
    setPal(vdp, PAL_BUOY, {0, gs::rgb4(15, 5, 2), gs::rgb4(15, 15, 13), gs::rgb4(6, 5, 4), gs::rgb4(15, 10, 2)});
    setPal(vdp, PAL_DOCK, {0, gs::rgb4(8, 8, 7), gs::rgb4(12, 12, 10), gs::rgb4(4, 4, 4)});
    setPal(vdp, PAL_WAKE, {0, gs::rgb4(12, 14, 15), gs::rgb4(15, 15, 15)});
    setPal(vdp, PAL_CAR, {0, gs::rgb4(13, 11, 3), gs::rgb4(6, 8, 10), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_SHORE, {0, gs::rgb4(6, 9, 4), gs::rgb4(9, 8, 5), gs::rgb4(4, 6, 3)});
    setPal(vdp, PAL_GULL, {0, gs::rgb4(15, 15, 14)});
    setPal(vdp, PAL_FUNNEL, {0, gs::rgb4(12, 3, 2)});
    setPal(vdp, PAL_DECK, {0, gs::rgb4(13, 14, 15)});
    setPal(vdp, PAL_POST, {0, gs::rgb4(10, 10, 9), gs::rgb4(14, 6, 2)});
    setPal(vdp, PAL_SLIP, {0, gs::rgb4(7, 7, 8), gs::rgb4(14, 12, 4), gs::rgb4(3, 8, 6)});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(15, 14, 6)});

    for (int i = 0; i < 7; i++) {
        float list = -0.9f + float(i) * (1.8f / 6.f);
        art.hull[i] = drawHull(vdp, list);
    }
    art.buoy = gs::uploadMipped(vdp, buoyArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.dock = gs::uploadMipped(vdp, dockArt());
    art.car = gs::uploadMipped(vdp, carArt());
    art.wake = gs::uploadMipped(vdp, wakeArt());
    art.gull = gs::uploadMipped(vdp, gullArt());
    art.slip = gs::uploadMipped(vdp, slipArt());
    art.chev = gs::uploadMipped(vdp, chevArt());
    loadFont(vdp, art);
}

}  // namespace ferryturn

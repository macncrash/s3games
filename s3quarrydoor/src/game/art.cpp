#include "art.h"

#include <initializer_list>
#include <string>

namespace quarry {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

uint32_t hash2(int x, int y) {
    uint32_t h = uint32_t(x) * 374761393u + uint32_t(y) * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) {
                    px[y * 8 + x + 1] = 1;
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 2;
                }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

void paintPit(gs::Bitmap& b) {
    b.rect(0, 0, 320, 224, 2);
    // Dust sky and a low sun over the cut.
    b.rect(0, 0, 320, 48, 11);
    b.rect(0, 40, 320, 16, 12);
    b.ellipse(250, 28, 14, 14, 13);
    b.ellipse(246, 26, 6, 6, 14);
    // Benches of the pit, left and right of the gate mouth.
    for (int step = 0; step < 5; step++) {
        int y = 52 + step * 22;
        int inset = step * 6;
        b.rect(0, y, 78 - inset, 22, (step % 2) ? 3 : 4);
        b.rect(242 + inset, y, 78 - inset, 22, (step % 2) ? 4 : 3);
        b.rect(0, y, 78 - inset, 2, 5);
        b.rect(242 + inset, y, 78 - inset, 2, 5);
    }
    // Gate mouth: dark haul beyond the timber.
    b.rect(72, 58, 176, 128, 1);
    b.rect(68, 54, 184, 6, 6);
    b.rect(64, 54, 8, 136, 6);
    b.rect(248, 54, 8, 136, 6);
    // Spoil heaps and the road the trucks use.
    b.rect(0, 168, 320, 56, 7);
    b.rect(0, 168, 320, 4, 8);
    b.rect(40, 188, 240, 18, 9);
    b.rect(40, 196, 240, 2, 10);
    for (int i = 0; i < 8; i++) {
        b.ellipse(18 + (i % 3) * 16, 156 - (i % 2) * 6, 16, 10, 4);
        b.ellipse(270 + (i % 2) * 14, 158, 14, 9, 3);
    }
    // Conveyor legs on the far bench.
    b.rect(16, 70, 4, 48, 6);
    b.rect(48, 78, 4, 40, 6);
    b.line(12, 72, 56, 86, 8, 2.f);
    for (int y = 60; y < 170; y += 5)
        for (int x = 4; x < 316; x += 7) {
            if (x > 64 && x < 256 && y > 54 && y < 186) continue;
            uint32_t h = hash2(x, y);
            if ((h % 17) == 0) b.set(x, y, (h & 1) ? 5 : 8);
        }
}

gs::Bitmap gateBitmap() {
    gs::Bitmap b(150, 140);
    b.rect(0, 0, 150, 140, 2);
    for (int x = 4; x < 146; x += 14) {
        b.rect(x, 4, 10, 132, 1);
        b.rect(x, 4, 2, 132, 3);
    }
    for (int y = 18; y < 130; y += 28) b.rect(2, y, 146, 6, 4);
    b.rect(0, 0, 150, 6, 5);
    b.rect(0, 134, 150, 6, 5);
    b.rect(0, 0, 6, 140, 5);
    b.rect(144, 0, 6, 140, 3);
    // Latch bar and quarry stencil.
    b.rect(62, 48, 26, 44, 6);
    b.rect(58, 64, 34, 8, 4);
    b.rect(18, 100, 40, 14, 7);
    return b;
}

gs::Bitmap shoreBitmap() {
    gs::Bitmap b(40, 18);
    b.rect(0, 6, 36, 6, 1);
    b.rect(0, 6, 36, 2, 2);
    b.rect(28, 2, 10, 14, 3);
    b.rect(30, 4, 6, 4, 4);
    return b;
}

gs::Bitmap crewBitmap() {
    gs::Bitmap b(28, 48);
    b.ellipse(14, 10, 7, 6, 1);
    b.rect(8, 4, 12, 4, 4);  // hard hat
    b.rect(7, 7, 14, 3, 5);
    b.rect(9, 16, 10, 16, 2);
    b.rect(4, 18, 6, 12, 3);
    b.rect(18, 18, 6, 12, 3);
    b.rect(10, 32, 3, 14, 1);
    b.rect(15, 32, 3, 14, 1);
    b.rect(8, 20, 12, 3, 6);
    return b;
}

gs::Bitmap truckBitmap() {
    gs::Bitmap b(56, 32);
    b.rect(4, 10, 28, 12, 1);
    b.poly({{30, 10}, {48, 14}, {48, 22}, {30, 22}}, 2);
    b.rect(8, 6, 18, 6, 3);
    b.rect(32, 14, 8, 4, 4);
    b.ellipse(14, 24, 6, 6, 5);
    b.ellipse(40, 24, 6, 6, 5);
    b.ellipse(14, 24, 2, 2, 6);
    b.ellipse(40, 24, 2, 2, 6);
    b.rect(6, 12, 8, 4, 4);
    return b;
}

gs::Bitmap dustBitmap() {
    gs::Bitmap b(24, 16);
    b.ellipse(8, 10, 7, 4, 1);
    b.ellipse(16, 8, 6, 4, 2);
    b.ellipse(12, 6, 3, 2, 3);
    return b;
}

gs::Bitmap chevBitmap() {
    gs::Bitmap b(24, 20);
    b.poly({{2, 10}, {12, 2}, {12, 6}, {22, 6}, {22, 14}, {12, 14}, {12, 18}}, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {gs::rgb4(0, 0, 0), gs::rgb4(15, 13, 6), gs::rgb4(6, 4, 2), gs::rgb4(15, 5, 1),
                          gs::rgb4(6, 12, 4), gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_PIT,
           {gs::rgb4(0, 0, 0), gs::rgb4(2, 1, 1), gs::rgb4(10, 7, 3), gs::rgb4(8, 5, 2), gs::rgb4(12, 8, 4),
            gs::rgb4(6, 4, 2), gs::rgb4(4, 3, 2), gs::rgb4(7, 6, 3), gs::rgb4(5, 4, 2), gs::rgb4(9, 8, 5),
            gs::rgb4(14, 12, 6), gs::rgb4(11, 9, 6), gs::rgb4(13, 8, 4), gs::rgb4(15, 14, 6), gs::rgb4(15, 12, 4),
            gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_GATE, {gs::rgb4(0, 0, 0), gs::rgb4(9, 6, 2), gs::rgb4(12, 8, 3), gs::rgb4(6, 4, 2),
                           gs::rgb4(4, 4, 4), gs::rgb4(8, 8, 7), gs::rgb4(3, 3, 3), gs::rgb4(14, 12, 4)});
    setPal(vdp, PAL_TRUCK, {gs::rgb4(0, 0, 0), gs::rgb4(14, 11, 2), gs::rgb4(10, 8, 2), gs::rgb4(6, 5, 2),
                            gs::rgb4(15, 14, 8), gs::rgb4(2, 2, 2), gs::rgb4(8, 8, 7)});
    setPal(vdp, PAL_CREW, {gs::rgb4(0, 0, 0), gs::rgb4(12, 8, 5), gs::rgb4(6, 8, 10), gs::rgb4(4, 5, 6),
                           gs::rgb4(14, 11, 2), gs::rgb4(8, 6, 1), gs::rgb4(15, 13, 4)});
    setPal(vdp, PAL_WARN, {gs::rgb4(0, 0, 0), gs::rgb4(15, 4, 1), gs::rgb4(15, 12, 3)});
    setPal(vdp, PAL_DUST, {gs::rgb4(0, 0, 0), gs::rgb4(12, 9, 5), gs::rgb4(10, 8, 4), gs::rgb4(14, 12, 8)});

    vdp.setFogColor(gs::rgb4(10, 7, 3));
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int dust = y < 48 ? 8 : (y < 90 ? 6 : 4);
        vdp.lineBackdrop[y] = gs::rgb4(dust, dust / 2 + 2, 1);
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    gs::Bitmap pit(320, 224);
    paintPit(pit);
    gs::bitmapToPlane(tiles, vdp.B, 0, 0, pit, PAL_PIT);
    vdp.A.enabled = false;
    vdp.B.enabled = true;

    art.gate = gs::uploadMipped(vdp, gateBitmap());
    art.shore = gs::uploadMipped(vdp, shoreBitmap());
    art.crew = gs::uploadMipped(vdp, crewBitmap());
    art.truck = gs::uploadMipped(vdp, truckBitmap());
    art.dust = gs::uploadMipped(vdp, dustBitmap());
    art.chev = gs::uploadMipped(vdp, chevBitmap());
}

}  // namespace quarry

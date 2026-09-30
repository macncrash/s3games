#include "art.h"

#include <initializer_list>
#include <string>

namespace wharf {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
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

void paintQuay(gs::Bitmap& b) {
    for (int y = 0; y < 78; y++) {
        int c = y < 28 ? 1 : y < 52 ? 2 : 3;
        b.rect(0, y, 320, 1, c);
    }
    // Low cloud and a lamp-haze over the slip.
    b.ellipse(46, 22, 28, 8, 4);
    b.ellipse(250, 16, 36, 7, 4);
    b.ellipse(168, 30, 50, 6, 4);
    // Warehouse shed, tarred boards, one high window row.
    b.rect(0, 46, 320, 122, 8);
    for (int y = 50; y < 164; y += 6) b.rect(0, y, 320, 1, 9);
    for (int x = 8; x < 312; x += 22) {
        b.rect(x, 48, 2, 116, 5);
        if (x > 70 && x < 250) continue;
        b.rect(x + 6, 58, 10, 14, 15);
        b.rect(x + 8, 60, 6, 8, 3);
    }
    // Freight opening. The door sprite covers it until the tide walks it.
    b.rect(108, 58, 112, 110, 12);
    b.rect(100, 50, 128, 8, 10);
    b.rect(100, 164, 128, 6, 10);
    b.rect(100, 50, 8, 120, 10);
    b.rect(220, 50, 8, 120, 10);
    // Name board and a coil of rope on the quay.
    b.rect(18, 70, 64, 16, 6);
    b.rect(22, 74, 56, 3, 14);
    b.rect(22, 80, 40, 2, 14);
    b.ellipse(40, 148, 16, 8, 14);
    b.ellipse(40, 148, 8, 4, 6);
    // Deck planks above the waterline. Rows under 168 stay for the road.
    b.rect(0, 156, 320, 28, 6);
    for (int x = 0; x < 320; x += 10) b.rect(x, 156, 1, 28, 5);
    b.rect(0, 156, 320, 3, 7);
    for (int i = 0; i < 6; i++) b.ellipse(24 + i * 52, 170, 6, 3, 13);
}

gs::Bitmap doorBitmap() {
    gs::Bitmap b(96, 128);
    b.rect(2, 2, 92, 124, 2);
    for (int y = 6; y < 122; y += 16) {
        b.rect(6, y, 84, 12, (y / 16) & 1 ? 1 : 2);
        b.rect(6, y, 84, 2, 4);
    }
    b.rect(0, 0, 96, 4, 3);
    b.rect(0, 124, 96, 4, 1);
    b.rect(0, 0, 4, 128, 3);
    b.rect(92, 0, 4, 128, 1);
    b.rect(8, 56, 80, 8, 6);
    b.rect(8, 64, 80, 4, 7);
    for (int row = 0; row < 4; row++)
        for (int col = 0; col < 3; col++) {
            int x = 22 + col * 26;
            int y = 22 + row * 28;
            b.ellipse(x, y, 3, 3, 5);
        }
    b.rect(70, 40, 8, 48, 5);
    b.rect(66, 48, 16, 6, 3);
    return b;
}

gs::Bitmap crewBitmap() {
    gs::Bitmap b(40, 64);
    b.ellipse(20, 10, 7, 7, 5);
    b.rect(14, 4, 12, 4, 6);
    b.rect(12, 8, 16, 3, 6);
    b.rect(14, 16, 12, 16, 2);
    b.rect(16, 18, 8, 8, 1);
    b.rect(8, 20, 8, 6, 2);
    b.rect(24, 20, 12, 6, 2);
    b.rect(4, 22, 8, 5, 3);
    b.rect(32, 18, 6, 16, 2);
    b.rect(16, 32, 8, 18, 1);
    b.rect(12, 48, 6, 12, 4);
    b.rect(22, 48, 6, 12, 4);
    b.rect(10, 58, 8, 4, 7);
    b.rect(22, 58, 8, 4, 7);
    return b;
}

gs::Bitmap lampBitmap() {
    gs::Bitmap b(16, 56);
    b.rect(7, 0, 2, 28, 1);
    b.rect(4, 26, 8, 10, 1);
    b.rect(5, 28, 6, 6, 3);
    b.ellipse(8, 30, 2, 2, 2);
    b.rect(6, 36, 4, 20, 1);
    return b;
}

gs::Bitmap crateBitmap() {
    gs::Bitmap b(32, 28);
    b.rect(1, 2, 30, 24, 2);
    b.rect(1, 2, 30, 4, 3);
    b.rect(1, 22, 30, 4, 1);
    b.line(2, 4, 14, 24, 1, 1.4f);
    b.line(30, 4, 18, 24, 1, 1.4f);
    b.rect(10, 10, 12, 8, 4);
    b.rect(12, 12, 8, 2, 5);
    return b;
}

gs::Bitmap gullBitmap() {
    gs::Bitmap b(28, 12);
    b.line(2, 8, 12, 4, 1, 1.6f);
    b.line(12, 4, 26, 7, 1, 1.6f);
    b.line(12, 5, 16, 10, 2, 1.2f);
    b.set(24, 6, 2);
    return b;
}

gs::Bitmap pileBitmap() {
    gs::Bitmap b(12, 72);
    b.rect(2, 0, 8, 72, 2);
    b.rect(2, 0, 3, 72, 3);
    b.rect(8, 0, 2, 72, 1);
    for (int y = 8; y < 70; y += 14) b.rect(1, y, 10, 2, 4);
    b.rect(1, 0, 10, 4, 5);
    return b;
}

gs::Bitmap chevBitmap() {
    gs::Bitmap b(16, 16);
    b.poly({{2, 2}, {14, 8}, {2, 14}}, 1);
    b.poly({{5, 5}, {10, 8}, {5, 11}}, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {gs::rgb4(0, 0, 0), gs::rgb4(14, 13, 9), gs::rgb4(4, 3, 2), gs::rgb4(15, 10, 3),
                          gs::rgb4(13, 3, 2)});
    setPal(vdp, PAL_QUAY,
           {0, gs::rgb4(1, 2, 6), gs::rgb4(2, 4, 9), gs::rgb4(4, 6, 11), gs::rgb4(10, 11, 12), gs::rgb4(3, 2, 2),
            gs::rgb4(6, 4, 2), gs::rgb4(9, 7, 4), gs::rgb4(5, 2, 2), gs::rgb4(3, 1, 1), gs::rgb4(7, 7, 8),
            gs::rgb4(12, 9, 3), gs::rgb4(1, 1, 3), gs::rgb4(8, 8, 6), gs::rgb4(10, 8, 4), gs::rgb4(13, 12, 6)});
    setPal(vdp, PAL_DOOR, {0, gs::rgb4(1, 3, 2), gs::rgb4(2, 6, 4), gs::rgb4(5, 9, 6), gs::rgb4(8, 4, 2),
                           gs::rgb4(10, 10, 8), gs::rgb4(13, 11, 2), gs::rgb4(2, 2, 1)});
    setPal(vdp, PAL_CREW, {0, gs::rgb4(6, 4, 1), gs::rgb4(12, 9, 2), gs::rgb4(14, 12, 5), gs::rgb4(2, 2, 2),
                           gs::rgb4(12, 8, 6), gs::rgb4(2, 2, 4), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(3, 3, 4), gs::rgb4(14, 13, 8), gs::rgb4(15, 12, 3)});
    setPal(vdp, PAL_CRATE, {0, gs::rgb4(4, 2, 1), gs::rgb4(8, 5, 2), gs::rgb4(11, 8, 4), gs::rgb4(2, 2, 5),
                            gs::rgb4(12, 12, 10)});
    setPal(vdp, PAL_WARN, {0, gs::rgb4(14, 3, 2), gs::rgb4(15, 14, 10)});
    setPal(vdp, PAL_BIRD, {0, gs::rgb4(13, 13, 12), gs::rgb4(6, 6, 7)});
    setPal(vdp, PAL_WATER, {0, gs::rgb4(1, 3, 6), gs::rgb4(1, 4, 7), gs::rgb4(2, 5, 8), gs::rgb4(2, 6, 6),
                            gs::rgb4(1, 5, 5), gs::rgb4(2, 6, 7), gs::rgb4(3, 7, 8), gs::rgb4(4, 5, 4),
                            gs::rgb4(6, 6, 5), gs::rgb4(7, 7, 6), gs::rgb4(1, 5, 9), gs::rgb4(2, 7, 11),
                            gs::rgb4(8, 12, 14), gs::rgb4(12, 12, 8), gs::rgb4(5, 8, 8)});
    vdp.setFogColor(gs::rgb4(2, 3, 6));

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, tiles, art);
    gs::Bitmap quay(320, 184);
    paintQuay(quay);
    gs::bitmapToPlane(tiles, vdp.B, 0, 0, quay, PAL_QUAY);
    vdp.A.enabled = false;

    art.door = gs::uploadMipped(vdp, doorBitmap());
    art.crew = gs::uploadMipped(vdp, crewBitmap());
    art.lamp = gs::uploadMipped(vdp, lampBitmap());
    art.crate = gs::uploadMipped(vdp, crateBitmap());
    art.gull = gs::uploadMipped(vdp, gullBitmap());
    art.pile = gs::uploadMipped(vdp, pileBitmap());
    art.chev = gs::uploadMipped(vdp, chevBitmap());
}

}  // namespace wharf

#include "art.h"

#include <initializer_list>
#include <string>

namespace trench {
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

void paintTrench(gs::Bitmap& b) {
    for (int y = 0; y < 92; y++) {
        int c = y < 40 ? 1 : (y < 70 ? 2 : 3);
        b.rect(0, y, 320, 1, c);
    }
    for (int i = 0; i < 40; i++) {
        uint32_t h = hash2(i, 3);
        b.set(int(h % 310u) + 4, int(h % 36u) + 4, 12);
    }
    // Parapet and fire-step. The gate gap sits in the middle of the revetment.
    b.rect(0, 86, 320, 10, 8);
    b.rect(0, 96, 320, 18, 6);
    b.rect(0, 114, 320, 110, 4);
    b.rect(78, 96, 164, 96, 1);
    b.rect(70, 88, 12, 110, 8);
    b.rect(238, 88, 12, 110, 8);
    for (int x = 0; x < 320; x += 16) {
        if (x > 64 && x < 248) continue;
        b.rect(x, 100, 4, 70, 5);
    }
    for (int y = 120; y < 200; y += 6) {
        int wob = (y / 6) & 1;
        b.rect(8 + wob, y, 52, 3, 5);
        b.rect(258 + wob, y, 52, 3, 5);
    }
    // Duckboards in the sump.
    b.rect(0, 196, 320, 28, 9);
    for (int x = 0; x < 320; x += 10) b.rect(x, 200, 7, 18, 5);
    b.rect(0, 208, 320, 2, 10);
    // Sandbag lips and a coil of wire on the left berm.
    for (int i = 0; i < 6; i++) {
        b.ellipse(18 + (i % 3) * 16, 84 - (i / 3) * 8, 12, 6, 6);
        b.ellipse(270 + (i % 3) * 14, 86 - (i / 3) * 8, 11, 6, 6);
    }
    for (int i = 0; i < 5; i++) b.ellipse(22 + i * 8, 70, 7, 3, 7);
    b.line(16, 68, 58, 74, 7, 1.4f);
    b.line(20, 74, 54, 66, 7, 1.2f);
    // A puddle and chalk mark on the fire-step.
    b.ellipse(40, 178, 16, 5, 11);
    b.rect(250, 160, 28, 3, 13);
    b.rect(254, 166, 18, 2, 13);
    for (int y = 96; y < 196; y += 3)
        for (int x = 4; x < 316; x += 5) {
            if (x > 74 && x < 246 && y < 192) continue;
            uint32_t h = hash2(x, y);
            if ((h % 23) == 0) b.set(x, y, (h & 1) ? 14 : 15);
        }
}

gs::Bitmap gateBitmap() {
    gs::Bitmap b(148, 128);
    b.rect(0, 0, 148, 128, 2);
    for (int plank = 0; plank < 7; plank++) {
        int x = 6 + plank * 20;
        b.rect(x, 4, 16, 120, plank & 1 ? 1 : 3);
        b.rect(x, 4, 2, 120, 4);
        for (int y = 10; y < 118; y += 14) b.set(x + 8, y, 5);
    }
    b.rect(4, 28, 140, 8, 6);
    b.rect(4, 88, 140, 8, 6);
    b.rect(8, 30, 132, 2, 4);
    b.rect(8, 90, 132, 2, 4);
    // Cross brace and a rope latch.
    b.line(12, 16, 136, 112, 6, 3.f);
    b.ellipse(74, 64, 8, 8, 7);
    b.ellipse(74, 64, 3, 3, 5);
    b.rect(0, 0, 148, 4, 4);
    b.rect(0, 124, 148, 4, 4);
    return b;
}

gs::Bitmap shoulderBitmap() {
    gs::Bitmap b(48, 28);
    b.ellipse(24, 16, 20, 10, 2);
    b.ellipse(16, 14, 8, 6, 1);
    b.rect(28, 10, 16, 6, 3);
    b.rect(36, 8, 8, 4, 4);
    return b;
}

gs::Bitmap foeBitmap() {
    gs::Bitmap b(28, 48);
    b.ellipse(14, 10, 8, 8, 2);
    b.rect(10, 8, 8, 4, 3);
    b.rect(8, 18, 12, 18, 1);
    b.rect(6, 20, 4, 14, 4);
    b.rect(18, 22, 4, 12, 4);
    b.rect(9, 36, 4, 10, 5);
    b.rect(15, 36, 4, 10, 5);
    b.rect(8, 22, 3, 8, 6);
    return b;
}

gs::Bitmap flareBitmap() {
    gs::Bitmap b(20, 20);
    b.ellipse(10, 10, 9, 9, 1);
    b.ellipse(10, 10, 5, 5, 2);
    b.ellipse(10, 10, 2, 2, 3);
    return b;
}

gs::Bitmap chevBitmap() {
    gs::Bitmap b(22, 18);
    b.poly({{2, 2}, {18, 9}, {2, 16}, {6, 9}}, 1);
    b.poly({{4, 5}, {14, 9}, {4, 13}}, 2);
    return b;
}

gs::Bitmap postBitmap() {
    gs::Bitmap b(10, 90);
    b.rect(2, 0, 6, 90, 1);
    b.rect(1, 0, 2, 90, 2);
    for (int y = 8; y < 84; y += 12) b.rect(1, y, 8, 2, 3);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {gs::rgb4(0, 0, 0), gs::rgb4(14, 13, 8), gs::rgb4(4, 3, 2), gs::rgb4(8, 2, 1)});
    setPal(vdp, PAL_SCENE,
           {gs::rgb4(0, 0, 0), gs::rgb4(1, 1, 3), gs::rgb4(2, 2, 5), gs::rgb4(3, 3, 6), gs::rgb4(4, 3, 2),
            gs::rgb4(6, 4, 2), gs::rgb4(5, 5, 3), gs::rgb4(2, 2, 2), gs::rgb4(3, 3, 2), gs::rgb4(2, 2, 1),
            gs::rgb4(7, 6, 3), gs::rgb4(2, 3, 4), gs::rgb4(12, 12, 10), gs::rgb4(10, 9, 6), gs::rgb4(6, 5, 3),
            gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_GATE,
           {gs::rgb4(0, 0, 0), gs::rgb4(8, 5, 2), gs::rgb4(5, 3, 1), gs::rgb4(10, 7, 3), gs::rgb4(3, 2, 1),
            gs::rgb4(12, 9, 4), gs::rgb4(6, 4, 2), gs::rgb4(9, 8, 5)});
    setPal(vdp, PAL_SKY,
           {gs::rgb4(0, 0, 0), gs::rgb4(14, 12, 6), gs::rgb4(12, 6, 2), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_SOLDIER,
           {gs::rgb4(0, 0, 0), gs::rgb4(6, 7, 4), gs::rgb4(4, 5, 3), gs::rgb4(8, 7, 4), gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_WARN, {gs::rgb4(0, 0, 0), gs::rgb4(14, 10, 2), gs::rgb4(15, 14, 8)});
    setPal(vdp, PAL_FOE,
           {gs::rgb4(0, 0, 0), gs::rgb4(3, 4, 3), gs::rgb4(5, 5, 4), gs::rgb4(2, 2, 1), gs::rgb4(4, 3, 2),
            gs::rgb4(2, 2, 2), gs::rgb4(8, 2, 1)});
    setPal(vdp, PAL_FLARE, {gs::rgb4(0, 0, 0), gs::rgb4(14, 8, 2), gs::rgb4(15, 14, 6), gs::rgb4(15, 15, 13)});

    vdp.setFogColor(gs::rgb4(1, 1, 2));

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, tiles, art);

    gs::Bitmap scene(320, 224);
    paintTrench(scene);
    gs::bitmapToPlane(tiles, vdp.B, 0, 0, scene, PAL_SCENE);
    vdp.A.clear();
    vdp.A.enabled = false;
    vdp.B.enabled = true;

    art.gate = gs::uploadMipped(vdp, gateBitmap());
    art.shoulder = gs::uploadMipped(vdp, shoulderBitmap());
    art.foe = gs::uploadMipped(vdp, foeBitmap());
    art.flare = gs::uploadMipped(vdp, flareBitmap());
    art.chev = gs::uploadMipped(vdp, chevBitmap());
    art.post = gs::uploadMipped(vdp, postBitmap());
}

}  // namespace trench

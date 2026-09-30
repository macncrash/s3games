#include "art.h"

#include <initializer_list>
#include <string>

namespace bdoor {
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
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), gs::TextStyle{2, 1, 0, 15, 1}));
    }
}

void paintHeadland(gs::Bitmap& b) {
    // Sky stays clear so the per-line night colour shows through.
    b.rect(0, 150, 320, 74, 3);
    b.rect(0, 148, 320, 4, 4);
    for (int y = 156; y < 224; y += 3) {
        int c = ((y / 3) & 1) ? 2 : 3;
        b.rect(0, y, 320, 1, c);
    }
    for (int i = 0; i < 18; i++) {
        uint32_t h = hash2(i, 9);
        int x = int(h % 300u);
        int y = 168 + int((h >> 8) % 40u);
        b.ellipse(x, y, 6 + int(h % 8u), 2, (h & 1) ? 5 : 6);
    }
    // White tower, red bands, lamp gallery. The iron door is a sprite.
    b.rect(36, 48, 46, 118, 8);
    for (int y = 56; y < 150; y += 16) b.rect(36, y, 46, 6, 9);
    b.rect(30, 40, 58, 12, 7);
    b.rect(40, 28, 38, 16, 10);
    b.rect(52, 16, 14, 14, 11);
    b.ellipse(59, 16, 5, 4, 12);
    b.rect(28, 162, 70, 8, 7);
    // Lamp-house wall and the doorway the slab seals.
    b.rect(108, 56, 150, 112, 7);
    b.rect(128, 68, 96, 96, 1);
    b.rect(122, 62, 108, 6, 10);
    b.rect(122, 158, 108, 8, 10);
    b.rect(118, 62, 8, 104, 8);
    b.rect(226, 62, 10, 104, 8);
    b.rect(108, 164, 150, 6, 13);
    // Rail and steps down to the headland.
    b.line(110, 170, 250, 188, 8, 2.f);
    b.line(110, 176, 250, 194, 7, 1.5f);
    for (int i = 0; i < 6; i++) b.rect(120 + i * 22, 168 + i * 3, 3, 16, 8);
    b.rect(250, 90, 40, 74, 13);
    b.rect(258, 78, 8, 16, 8);
    b.ellipse(262, 76, 6, 4, 12);
    for (int y = 40; y < 140; y += 5)
        for (int x = 8; x < 100; x += 6) {
            if (x > 30 && x < 90 && y > 40 && y < 166) continue;
            if ((hash2(x, y) % 17) == 0) b.set(x, y, 14);
        }
}

gs::Bitmap slabBitmap() {
    gs::Bitmap b(96, 104);
    b.rect(0, 0, 96, 104, 2);
    for (int y = 0; y < 104; y++) {
        if ((y / 13) % 2 == 0) b.rect(4, y, 88, 1, 1);
        if ((y % 13) < 2) b.rect(0, y, 96, 2, 3);
    }
    b.rect(0, 0, 5, 104, 4);
    b.rect(91, 0, 5, 104, 1);
    b.rect(0, 0, 96, 4, 4);
    b.rect(0, 100, 96, 4, 1);
    // Round lamp glass and dogging bar.
    b.ellipse(48, 36, 16, 16, 5);
    b.ellipse(48, 36, 10, 10, 6);
    b.ellipse(44, 32, 3, 3, 7);
    b.rect(44, 52, 8, 36, 3);
    b.ellipse(48, 78, 14, 14, 4);
    b.ellipse(48, 78, 6, 6, 8);
    b.rect(28, 74, 40, 6, 3);
    for (int i = 0; i < 4; i++) {
        b.ellipse(14, 16 + i * 22, 3, 3, 4);
        b.ellipse(82, 16 + i * 22, 3, 3, 4);
    }
    return b;
}

gs::Bitmap armBitmap() {
    gs::Bitmap b(40, 26);
    b.rect(2, 10, 26, 8, 2);
    b.ellipse(8, 14, 7, 7, 3);
    b.rect(22, 6, 16, 14, 1);
    b.rect(26, 8, 8, 5, 4);
    b.rect(0, 12, 6, 5, 2);
    return b;
}

gs::Bitmap galeBitmap() {
    gs::Bitmap b(26, 44);
    b.ellipse(13, 8, 6, 6, 1);
    b.poly({{4, 16}, {22, 16}, {20, 40}, {6, 42}}, 2);
    b.rect(2, 18, 6, 12, 1);
    b.rect(18, 18, 6, 14, 3);
    b.line(8, 22, 18, 28, 4, 1.2f);
    return b;
}

gs::Bitmap lampBitmap() {
    gs::Bitmap b(28, 28);
    b.ellipse(14, 14, 12, 12, 1);
    b.ellipse(14, 14, 7, 7, 2);
    b.ellipse(11, 11, 2, 2, 3);
    b.rect(13, 0, 2, 8, 1);
    b.rect(13, 20, 2, 8, 1);
    b.rect(0, 13, 8, 2, 1);
    b.rect(20, 13, 8, 2, 1);
    return b;
}

gs::Bitmap chevBitmap() {
    gs::Bitmap b(24, 20);
    b.poly({{2, 10}, {12, 2}, {12, 6}, {22, 6}, {22, 14}, {12, 14}, {12, 18}}, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {gs::rgb4(0, 0, 0), gs::rgb4(15, 14, 8), gs::rgb4(4, 5, 8), gs::rgb4(15, 4, 2),
                          gs::rgb4(6, 14, 8), gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_HEAD,
           {gs::rgb4(0, 0, 0), gs::rgb4(1, 1, 2), gs::rgb4(1, 3, 6), gs::rgb4(1, 2, 5), gs::rgb4(3, 5, 7),
            gs::rgb4(8, 9, 10), gs::rgb4(12, 13, 14), gs::rgb4(5, 5, 6), gs::rgb4(14, 14, 13),
            gs::rgb4(12, 2, 2), gs::rgb4(9, 9, 10), gs::rgb4(6, 6, 7), gs::rgb4(15, 13, 4), gs::rgb4(3, 3, 4),
            gs::rgb4(10, 10, 12)});
    setPal(vdp, PAL_DOOR, {gs::rgb4(0, 0, 0), gs::rgb4(2, 2, 3), gs::rgb4(5, 5, 6), gs::rgb4(8, 8, 9),
                           gs::rgb4(11, 11, 12), gs::rgb4(2, 4, 8), gs::rgb4(14, 12, 4), gs::rgb4(15, 15, 10),
                           gs::rgb4(12, 3, 2)});
    setPal(vdp, PAL_LAMP, {gs::rgb4(0, 0, 0), gs::rgb4(15, 12, 3), gs::rgb4(15, 15, 10), gs::rgb4(15, 15, 15)});
    setPal(vdp, PAL_ARM, {gs::rgb4(0, 0, 0), gs::rgb4(3, 3, 4), gs::rgb4(6, 5, 3), gs::rgb4(10, 7, 4),
                          gs::rgb4(13, 11, 7)});
    setPal(vdp, PAL_WARN, {gs::rgb4(0, 0, 0), gs::rgb4(15, 6, 1), gs::rgb4(15, 13, 3)});
    setPal(vdp, PAL_GALE, {gs::rgb4(0, 0, 0), gs::rgb4(1, 1, 2), gs::rgb4(2, 3, 5), gs::rgb4(3, 4, 6),
                           gs::rgb4(8, 9, 10)});

    vdp.setFogColor(gs::rgb4(1, 1, 3));
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int k = y < 150 ? y : 150;
        vdp.lineBackdrop[y] = gs::rgb4(1, 1 + k / 80, 2 + k / 40);
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    gs::Bitmap head(320, 224);
    paintHeadland(head);
    gs::bitmapToPlane(tiles, vdp.B, 0, 0, head, PAL_HEAD);
    vdp.A.enabled = false;
    vdp.B.enabled = true;

    art.slab = gs::uploadMipped(vdp, slabBitmap());
    art.arm = gs::uploadMipped(vdp, armBitmap());
    art.gale = gs::uploadMipped(vdp, galeBitmap());
    art.lamp = gs::uploadMipped(vdp, lampBitmap());
    art.chev = gs::uploadMipped(vdp, chevBitmap());
}

}  // namespace bdoor

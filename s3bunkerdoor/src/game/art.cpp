#include "art.h"

#include <initializer_list>
#include <string>

namespace door {
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
    gs::TextStyle big{2, 1, 0, 15, 1};
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
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

void paintRoom(gs::Bitmap& b) {
    b.rect(0, 0, 320, 224, 2);
    b.rect(0, 0, 320, 22, 13);
    b.rect(0, 20, 320, 3, 1);
    b.rect(0, 186, 320, 38, 7);
    b.rect(0, 186, 320, 3, 8);
    // Doorway void. The slab sprite covers it until a heave walks it open.
    b.rect(78, 32, 164, 154, 1);
    b.rect(74, 28, 172, 6, 5);
    b.rect(74, 182, 172, 8, 5);
    b.rect(70, 28, 8, 162, 4);
    b.rect(238, 28, 10, 162, 4);
    for (int y = 26; y < 190; y += 4)
        for (int x = 6; x < 314; x += 5) {
            if (x > 70 && x < 250 && y > 26 && y < 190) continue;
            uint32_t h = hash2(x, y);
            if ((h % 19) == 0) b.set(x, y, (h & 1) ? 10 : 12);
        }
    // Lamp, pipes, sandbags. The watch is a concrete room, not a range.
    b.rect(18, 36, 3, 70, 9);
    b.rect(28, 40, 3, 64, 9);
    b.ellipse(24, 112, 14, 8, 6);
    b.ellipse(24, 112, 6, 3, 11);
    b.rect(250, 40, 4, 90, 9);
    b.rect(262, 48, 4, 78, 9);
    b.rect(248, 128, 22, 8, 5);
    for (int i = 0; i < 4; i++) {
        b.ellipse(16 + i * 18, 200, 16, 10, 8);
        b.ellipse(250 + (i % 3) * 16, 204, 14, 9, 8);
    }
    b.rect(8, 150, 48, 22, 4);
    b.rect(12, 154, 28, 6, 14);
    b.rect(12, 162, 18, 4, 14);
    b.line(14, 156, 50, 168, 1, 1.2f);
}

gs::Bitmap slabBitmap() {
    gs::Bitmap b(140, 150);
    b.rect(0, 0, 140, 150, 2);
    for (int y = 0; y < 150; y++) {
        int c = (y / 18) % 2 == 0 ? 2 : 1;
        b.rect(0, y, 140, 1, c);
        if ((y % 18) < 2) b.rect(0, y, 140, 2, 6);
    }
    b.rect(0, 0, 6, 150, 3);
    b.rect(134, 0, 6, 150, 1);
    for (int row = 0; row < 5; row++)
        for (int col = 0; col < 3; col++) {
            int x = 22 + col * 40;
            int y = 16 + row * 26;
            b.ellipse(x, y, 4, 4, 4);
            b.ellipse(x - 1, y - 1, 1, 1, 3);
        }
    b.rect(8, 64, 124, 10, 7);
    b.rect(8, 74, 124, 8, 8);
    b.rect(18, 66, 100, 2, 3);
    // Wheel and dogging bar.
    b.rect(62, 40, 16, 70, 6);
    b.ellipse(70, 78, 22, 22, 1);
    b.ellipse(70, 78, 14, 14, 3);
    b.ellipse(70, 78, 5, 5, 4);
    b.rect(48, 74, 44, 6, 6);
    return b;
}

gs::Bitmap armBitmap() {
    gs::Bitmap b(36, 28);
    b.rect(2, 10, 28, 8, 2);
    b.ellipse(8, 14, 7, 7, 3);
    b.rect(22, 6, 12, 16, 1);
    b.rect(24, 8, 8, 5, 4);
    b.rect(0, 12, 8, 6, 2);
    return b;
}

gs::Bitmap figureBitmap() {
    gs::Bitmap b(28, 48);
    b.ellipse(14, 8, 6, 6, 1);
    b.rect(8, 14, 12, 18, 2);
    b.rect(4, 16, 6, 14, 1);
    b.rect(18, 16, 6, 14, 1);
    b.rect(9, 32, 4, 14, 3);
    b.rect(15, 32, 4, 14, 3);
    b.rect(6, 18, 16, 3, 4);
    return b;
}

gs::Bitmap moonBitmap() {
    gs::Bitmap b(20, 20);
    b.ellipse(10, 10, 8, 8, 1);
    b.ellipse(13, 8, 6, 6, 0);
    b.set(7, 8, 2);
    b.set(8, 12, 2);
    return b;
}

gs::Bitmap chevBitmap() {
    gs::Bitmap b(24, 20);
    b.poly({{2, 10}, {12, 2}, {12, 6}, {22, 6}, {22, 14}, {12, 14}, {12, 18}}, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {gs::rgb4(0, 0, 0), gs::rgb4(14, 12, 6), gs::rgb4(6, 5, 3), gs::rgb4(15, 4, 2),
                          gs::rgb4(4, 12, 6), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_ROOM,
           {gs::rgb4(0, 0, 0), gs::rgb4(1, 1, 1), gs::rgb4(6, 6, 5), gs::rgb4(8, 8, 7), gs::rgb4(4, 4, 4),
            gs::rgb4(7, 4, 2), gs::rgb4(13, 11, 4), gs::rgb4(3, 3, 2), gs::rgb4(5, 5, 4), gs::rgb4(5, 5, 6),
            gs::rgb4(3, 3, 3), gs::rgb4(15, 14, 8), gs::rgb4(7, 6, 5), gs::rgb4(5, 5, 4), gs::rgb4(9, 8, 6),
            gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_DOOR, {gs::rgb4(0, 0, 0), gs::rgb4(3, 4, 4), gs::rgb4(6, 7, 7), gs::rgb4(9, 10, 10),
                           gs::rgb4(12, 11, 7), gs::rgb4(8, 4, 2), gs::rgb4(2, 2, 2), gs::rgb4(12, 10, 2),
                           gs::rgb4(10, 2, 1)});
    setPal(vdp, PAL_NIGHT, {gs::rgb4(0, 0, 0), gs::rgb4(13, 13, 10), gs::rgb4(8, 8, 6)});
    setPal(vdp, PAL_ARM, {gs::rgb4(0, 0, 0), gs::rgb4(3, 3, 2), gs::rgb4(6, 5, 3), gs::rgb4(10, 7, 5),
                          gs::rgb4(13, 10, 7)});
    setPal(vdp, PAL_WARN, {gs::rgb4(0, 0, 0), gs::rgb4(15, 3, 1), gs::rgb4(15, 12, 3)});
    setPal(vdp, PAL_FIG, {gs::rgb4(0, 0, 0), gs::rgb4(1, 1, 2), gs::rgb4(2, 2, 3), gs::rgb4(1, 1, 1),
                          gs::rgb4(4, 3, 2)});

    vdp.setFogColor(gs::rgb4(1, 1, 2));
    for (int y = 0; y < gs::SCREEN_H; y++) {
        vdp.lineBackdrop[y] = gs::rgb4(2, 2, 3);
        vdp.lineFog[y] = 0;
    }

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    gs::Bitmap room(320, 224);
    paintRoom(room);
    gs::bitmapToPlane(tiles, vdp.B, 0, 0, room, PAL_ROOM);
    vdp.A.enabled = false;
    vdp.B.enabled = true;

    art.slab = gs::uploadMipped(vdp, slabBitmap());
    art.arm = gs::uploadMipped(vdp, armBitmap());
    art.figure = gs::uploadMipped(vdp, figureBitmap());
    art.moon = gs::uploadMipped(vdp, moonBitmap());
    art.chev = gs::uploadMipped(vdp, chevBitmap());
}

}  // namespace door

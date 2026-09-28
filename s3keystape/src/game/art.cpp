#include "game/art.h"

namespace keys {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
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
    }
}

gs::Bitmap deskArt() {
    gs::Bitmap b(320, 96);
    b.rect(0, 0, 320, 96, 2);
    for (int y = 0; y < 96; y += 8) b.rect(0, y, 320, 1, (y / 8) & 1 ? 1 : 3);
    b.rect(0, 0, 320, 6, 3);
    b.rect(0, 90, 320, 6, 1);
    return b;
}

gs::Bitmap drawerArt() {
    gs::Bitmap b(248, 78);
    b.rect(0, 0, 248, 78, 2);
    b.rect(4, 4, 240, 70, 1);
    b.rect(8, 8, 232, 62, 4);
    b.rect(110, 32, 28, 10, 6);
    b.rect(114, 34, 20, 6, 5);
    b.outline(3, false);
    return b;
}

gs::Bitmap tapeArt() {
    gs::Bitmap b(210, 62);
    b.rect(0, 0, 210, 62, 2);
    b.rect(4, 6, 202, 50, 1);
    for (int x = 10; x < 200; x += 6) b.rect(x, 8, 1, 46, 3);
    b.rect(0, 0, 210, 4, 4);
    b.rect(0, 58, 210, 4, 4);
    return b;
}

gs::Bitmap bowArt() {
    gs::Bitmap b(22, 18);
    b.ellipse(11, 9, 10, 8, 2);
    b.ellipse(11, 9, 4, 3, 1);
    b.rect(8, 16, 6, 2, 3);
    return b;
}

gs::Bitmap bladeArt() {
    gs::Bitmap b(8, 36);
    b.rect(1, 0, 6, 36, 2);
    b.rect(1, 0, 2, 36, 3);
    b.rect(0, 0, 8, 2, 1);
    return b;
}

gs::Bitmap toothArt(int depth) {
    int h = 6 + depth * 5;
    gs::Bitmap b(8, 18);
    b.rect(0, 18 - h, 7, h, 2);
    b.rect(0, 18 - h, 2, h, 3);
    b.rect(5, 18 - h, 2, 2, 1);
    return b;
}

gs::Bitmap pipArt() {
    gs::Bitmap b(8, 8);
    b.ellipse(4, 4, 3, 3, 1);
    return b;
}

gs::Bitmap cursorArt() {
    gs::Bitmap b(26, 8);
    b.rect(0, 0, 26, 2, 1);
    b.rect(0, 0, 2, 8, 1);
    b.rect(24, 0, 2, 8, 1);
    return b;
}

gs::Bitmap doorArt() {
    gs::Bitmap b(52, 120);
    b.rect(0, 0, 52, 120, 2);
    b.rect(4, 4, 44, 112, 3);
    b.rect(8, 10, 36, 28, 4);
    b.rect(10, 48, 14, 22, 1);
    b.rect(28, 48, 14, 22, 1);
    b.ellipse(40, 68, 3, 3, 5);
    b.rect(6, 100, 40, 8, 2);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(28, 16);
    b.poly({{2, 14}, {8, 2}, {20, 2}, {26, 14}}, 1);
    b.rect(12, 14, 4, 2, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t sh = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(8, 8, 9), gs::rgb4(15, 12, 4), gs::rgb4(6, 14, 7),
                          gs::rgb4(14, 5, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, sh});
    setPal(vdp, PAL_DESK, {0, gs::rgb4(4, 2, 1), gs::rgb4(8, 5, 2), gs::rgb4(12, 8, 4), gs::rgb4(5, 4, 3),
                           gs::rgb4(14, 12, 6), gs::rgb4(9, 8, 6), 0, 0, 0, 0, 0, 0, 0, 0, sh});
    setPal(vdp, PAL_KEY, {0, gs::rgb4(6, 4, 2), gs::rgb4(13, 10, 4), gs::rgb4(15, 14, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, sh});
    setPal(vdp, PAL_TAPE, {0, gs::rgb4(14, 13, 10), gs::rgb4(11, 10, 8), gs::rgb4(8, 7, 6), gs::rgb4(3, 3, 3),
                           gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, sh});
    setPal(vdp, PAL_DOOR, {0, gs::rgb4(2, 3, 5), gs::rgb4(4, 5, 7), gs::rgb4(7, 8, 10), gs::rgb4(10, 12, 14),
                           gs::rgb4(14, 12, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, sh});

    loadFont(vdp, art);
    art.desk = gs::uploadMipped(vdp, deskArt());
    art.drawer = gs::uploadMipped(vdp, drawerArt());
    art.tape = gs::uploadMipped(vdp, tapeArt());
    art.bow = gs::uploadMipped(vdp, bowArt());
    art.blade = gs::uploadMipped(vdp, bladeArt());
    for (int i = 0; i < 3; i++) art.tooth[i] = gs::uploadMipped(vdp, toothArt(i));
    art.pip = gs::uploadMipped(vdp, pipArt());
    art.cursor = gs::uploadMipped(vdp, cursorArt());
    art.door = gs::uploadMipped(vdp, doorArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
}

}  // namespace keys

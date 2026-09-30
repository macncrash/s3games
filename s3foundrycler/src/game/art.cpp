#include "game/art.h"

#include <initializer_list>

namespace fcler {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

gs::Bitmap crewArt(int step) {
    gs::Bitmap b(28, 46);
    b.ellipse(14, 8, 7, 7, 4);
    b.rect(8, 3, 12, 4, 8);
    b.rect(10, 8, 8, 3, 9);
    b.poly({{7, 14}, {21, 14}, {23, 32}, {5, 32}}, 2);
    b.rect(11, 16, 6, 10, 6);
    b.rect(6, 18, 4, 8, 3);
    b.rect(18, 18, 4, 8, 3);
    if (step == 0) {
        b.line(8, 22, 2, 30, 5, 2.4f);
        b.rect(0, 28, 5, 4, 7);
        b.rect(7, 32, 5, 12, 1);
        b.rect(16, 32, 5, 10, 3);
        b.rect(6, 42, 7, 3, 10);
        b.rect(15, 40, 7, 3, 11);
    } else {
        b.line(8, 22, 3, 28, 5, 2.4f);
        b.rect(1, 26, 5, 4, 7);
        b.rect(7, 32, 5, 10, 3);
        b.rect(16, 32, 5, 12, 1);
        b.rect(6, 40, 7, 3, 11);
        b.rect(15, 42, 7, 3, 10);
    }
    b.outline(12, false);
    return b;
}

gs::Bitmap rakeArt() {
    gs::Bitmap b(40, 16);
    b.rect(2, 6, 26, 3, 2);
    b.rect(26, 2, 4, 12, 1);
    b.rect(29, 3, 2, 3, 4);
    b.rect(29, 7, 2, 3, 4);
    b.rect(29, 11, 2, 3, 4);
    b.rect(34, 3, 3, 10, 3);
    b.outline(5, false);
    return b;
}

gs::Bitmap slagArt() {
    gs::Bitmap b(30, 18);
    b.ellipse(15, 11, 13, 6, 1);
    b.ellipse(10, 10, 5, 4, 2);
    b.ellipse(19, 9, 6, 4, 3);
    b.rect(13, 8, 3, 3, 5);
    b.rect(8, 11, 2, 2, 4);
    b.outline(6, false);
    return b;
}

gs::Bitmap furnaceArt() {
    gs::Bitmap b(52, 72);
    b.rect(4, 16, 44, 52, 2);
    b.poly({{8, 16}, {44, 16}, {40, 4}, {12, 4}}, 3);
    b.rect(14, 28, 24, 28, 1);
    b.ellipse(26, 42, 8, 10, 6);
    b.ellipse(26, 44, 4, 6, 7);
    b.rect(8, 62, 8, 8, 4);
    b.rect(36, 62, 8, 8, 4);
    b.rect(18, 18, 4, 6, 5);
    b.outline(8, false);
    return b;
}

gs::Bitmap clockArt() {
    gs::Bitmap b(34, 34);
    b.ellipse(17, 17, 15, 15, 2);
    b.ellipse(17, 17, 11, 11, 1);
    b.rect(16, 8, 2, 8, 4);
    b.rect(16, 16, 7, 2, 4);
    b.rect(15, 2, 4, 4, 3);
    b.outline(5, false);
    return b;
}

gs::Bitmap ladleArt() {
    gs::Bitmap b(22, 16);
    b.ellipse(8, 9, 7, 5, 2);
    b.ellipse(8, 8, 4, 2, 4);
    b.rect(14, 7, 7, 3, 1);
    b.outline(3, false);
    return b;
}

gs::Bitmap linkArt() {
    gs::Bitmap b(6, 8);
    b.rect(1, 1, 4, 6, 1);
    b.rect(2, 2, 2, 4, 0);
    b.outline(2, false);
    return b;
}

gs::Bitmap sparkArt() {
    gs::Bitmap b(5, 5);
    b.set(2, 0, 2);
    b.set(2, 4, 1);
    b.set(0, 2, 1);
    b.set(4, 2, 2);
    b.set(2, 2, 3);
    return b;
}

gs::Bitmap binArt() {
    gs::Bitmap b(28, 24);
    b.poly({{2, 4}, {26, 4}, {22, 22}, {6, 22}}, 1);
    b.rect(4, 2, 20, 4, 2);
    b.rect(8, 10, 12, 3, 3);
    b.outline(4, false);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp, 2);
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

void floorTile(gs::VDP& vdp) {
    uint8_t px[64];
    for (int y = 0; y < 8; y++) {
        int band = y < 4 ? 0 : 1;
        for (int x = 0; x < 8; x++) {
            int col = (x + band * 4) % 8;
            bool mortar = y == 0 || y == 4 || col == 0;
            int c = mortar ? 1 : (band ? 3 : 2);
            if (!mortar && ((x + y) % 5 == 0)) c = 4;
            px[y * 8 + x] = uint8_t(c);
        }
    }
    vdp.loadTile(1, px);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_TEXT, {gs::rgb4(0, 0, 0), gs::rgb4(15, 14, 12), gs::rgb4(3, 2, 2), gs::rgb4(15, 12, 6)});
    setPal(vdp, PAL_IRON, {gs::rgb4(0, 0, 0), gs::rgb4(6, 6, 7), gs::rgb4(10, 10, 11), gs::rgb4(4, 3, 3),
                           gs::rgb4(12, 8, 4), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_EMBER, {gs::rgb4(0, 0, 0), gs::rgb4(8, 3, 1), gs::rgb4(13, 6, 1), gs::rgb4(15, 10, 2),
                            gs::rgb4(15, 14, 6), gs::rgb4(6, 2, 1), gs::rgb4(15, 8, 2), gs::rgb4(15, 15, 8),
                            gs::rgb4(4, 3, 3)});
    setPal(vdp, PAL_SLAG, {gs::rgb4(0, 0, 0), gs::rgb4(3, 3, 3), gs::rgb4(5, 5, 4), gs::rgb4(7, 6, 4),
                           gs::rgb4(12, 6, 1), gs::rgb4(15, 11, 3), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_BRICK, {gs::rgb4(0, 0, 0), gs::rgb4(4, 2, 2), gs::rgb4(9, 4, 3), gs::rgb4(7, 3, 2),
                            gs::rgb4(12, 6, 3), gs::rgb4(5, 5, 5)});
    setPal(vdp, PAL_CREW, {gs::rgb4(0, 0, 0), gs::rgb4(3, 3, 5), gs::rgb4(8, 7, 6), gs::rgb4(5, 4, 6),
                           gs::rgb4(13, 9, 6), gs::rgb4(10, 8, 6), gs::rgb4(11, 4, 2), gs::rgb4(14, 12, 8),
                           gs::rgb4(4, 4, 5), gs::rgb4(2, 6, 8), gs::rgb4(2, 2, 2), gs::rgb4(6, 5, 4),
                           gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_CLOCK, {gs::rgb4(0, 0, 0), gs::rgb4(14, 13, 10), gs::rgb4(10, 8, 3), gs::rgb4(6, 5, 2),
                            gs::rgb4(12, 2, 1), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_GOOD, {gs::rgb4(0, 0, 0), gs::rgb4(8, 14, 8), gs::rgb4(2, 5, 2)});
    setPal(vdp, PAL_ALERT, {gs::rgb4(0, 0, 0), gs::rgb4(15, 4, 2), gs::rgb4(6, 1, 1)});
    setPal(vdp, PAL_SOOT, {gs::rgb4(0, 0, 0), gs::rgb4(2, 2, 2), gs::rgb4(4, 3, 3)});

    art.crew[0] = gs::uploadMipped(vdp, crewArt(0));
    art.crew[1] = gs::uploadMipped(vdp, crewArt(1));
    art.rake = gs::uploadMipped(vdp, rakeArt());
    art.slag = gs::uploadMipped(vdp, slagArt());
    art.furnace = gs::uploadMipped(vdp, furnaceArt());
    art.clock = gs::uploadMipped(vdp, clockArt());
    art.ladle = gs::uploadMipped(vdp, ladleArt());
    art.link = gs::uploadMipped(vdp, linkArt());
    art.spark = gs::uploadMipped(vdp, sparkArt());
    art.bin = gs::uploadMipped(vdp, binArt());
    floorTile(vdp);
    art.floor = 1;
    loadFont(vdp, art);
    vdp.setFogColor(gs::rgb4(2, 1, 1));
}

}  // namespace fcler

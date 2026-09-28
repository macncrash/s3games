#include "game/art.h"

#include <initializer_list>

namespace bocceseven {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
}

int uploadTile(gs::VDP& vdp, gs::TileAlloc& tiles, const gs::Bitmap& b) {
    uint8_t px[64] = {};
    for (int y = 0; y < 8 && y < b.h; y++)
        for (int x = 0; x < 8 && x < b.w; x++) px[y * 8 + x] = uint8_t(b.get(x, y) & 15);
    int t = tiles.alloc(1);
    vdp.loadTile(t, px);
    return t;
}

gs::Bitmap bowlArt() {
    gs::Bitmap b(20, 20);
    b.ellipse(10, 10, 9.0f, 9.0f, 4);
    b.ellipse(10, 10, 7.6f, 7.6f, 2);
    b.ellipse(10, 10, 5.4f, 5.4f, 3);
    b.ellipse(7.2f, 7.0f, 2.2f, 1.6f, 1);
    b.line(4, 13, 16, 7, 3, 1.2f);
    return b;
}

gs::Bitmap pallinoArt() {
    gs::Bitmap b(12, 12);
    b.ellipse(6, 6, 5.0f, 5.0f, 2);
    b.ellipse(6, 6, 2.4f, 2.4f, 1);
    b.rect(5, 2, 2, 8, 3);
    b.rect(2, 5, 8, 2, 3);
    return b;
}

gs::Bitmap shadeArt() {
    gs::Bitmap b(18, 8);
    b.ellipse(9, 4, 7.4f, 2.6f, 1);
    return b;
}

gs::Bitmap cypressArt() {
    gs::Bitmap b(14, 48);
    b.rect(6, 36, 2, 12, 4);
    b.poly({{7, 2}, {12, 38}, {2, 38}}, 2);
    b.poly({{7, 8}, {10, 30}, {4, 30}}, 1);
    b.ellipse(7, 14, 2.0f, 6.0f, 3);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(12, 28);
    b.rect(5, 10, 2, 16, 4);
    b.rect(3, 24, 6, 2, 3);
    b.ellipse(6, 7, 4.2f, 4.2f, 1);
    b.ellipse(5, 6, 1.6f, 1.4f, 2);
    return b;
}

gs::Bitmap flagArt() {
    gs::Bitmap b(22, 28);
    b.rect(2, 2, 2, 24, 3);
    b.poly({{4, 3}, {20, 8}, {4, 14}}, 1);
    b.poly({{8, 5}, {16, 8}, {8, 11}}, 2);
    return b;
}

gs::Bitmap markArt() {
    gs::Bitmap b(10, 10);
    b.line(1, 5, 9, 5, 1, 1);
    b.line(5, 1, 5, 9, 1, 1);
    b.ellipse(5, 5, 2.2f, 2.2f, 2);
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 1);
    b.set(1, 1, 2);
    b.set(4, 2, 3);
    b.set(6, 5, 2);
    b.set(2, 6, 3);
    b.set(5, 6, 4);
    return b;
}

gs::Bitmap railArt() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 1);
    b.rect(0, 0, 8, 2, 2);
    b.rect(0, 6, 8, 2, 3);
    return b;
}

gs::Bitmap lawnArt() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 1);
    b.set(1, 3, 2);
    b.set(2, 2, 2);
    b.set(5, 5, 3);
    b.set(6, 1, 2);
    return b;
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) px[y * 8 + x + 1] = 1;
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_INK, {0, gs::rgb4(15, 15, 14)});
    setPal(vdp, PAL_CREAM, {0, gs::rgb4(15, 14, 9), gs::rgb4(8, 5, 2)});
    setPal(vdp, PAL_YOU, {0, gs::rgb4(12, 15, 13), gs::rgb4(4, 12, 8), gs::rgb4(1, 6, 4), gs::rgb4(15, 15, 15)});
    setPal(vdp, PAL_THEM, {0, gs::rgb4(15, 12, 10), gs::rgb4(13, 4, 3), gs::rgb4(6, 1, 2), gs::rgb4(8, 2, 2)});
    setPal(vdp, PAL_PALLINO, {0, gs::rgb4(15, 15, 12), gs::rgb4(14, 11, 3), gs::rgb4(10, 2, 2)});
    setPal(vdp, PAL_RAIL, {0, gs::rgb4(9, 6, 3), gs::rgb4(13, 9, 4), gs::rgb4(4, 2, 1)});
    setPal(vdp, PAL_DUST, {0, gs::rgb4(11, 9, 6), gs::rgb4(7, 5, 3), gs::rgb4(14, 12, 8), gs::rgb4(15, 14, 11)});
    setPal(vdp, PAL_CYPRESS, {0, gs::rgb4(8, 14, 7), gs::rgb4(2, 8, 3), gs::rgb4(4, 12, 6), gs::rgb4(5, 3, 1)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(15, 14, 6), gs::rgb4(15, 15, 13), gs::rgb4(6, 4, 2), gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(15, 15, 12), gs::rgb4(15, 6, 2)});
    setPal(vdp, PAL_LAWN, {0, gs::rgb4(3, 9, 3), gs::rgb4(5, 12, 4), gs::rgb4(2, 6, 2)});
    setPal(vdp, PAL_FLAG, {0, gs::rgb4(12, 2, 3), gs::rgb4(15, 14, 8), gs::rgb4(7, 4, 2)});

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    art.dust = uploadTile(vdp, tiles, dustArt());
    art.rail = uploadTile(vdp, tiles, railArt());
    art.lawn = uploadTile(vdp, tiles, lawnArt());
    art.bowl = gs::uploadMipped(vdp, bowlArt());
    art.pallino = gs::uploadMipped(vdp, pallinoArt());
    art.shade = gs::uploadMipped(vdp, shadeArt());
    art.cypress = gs::uploadMipped(vdp, cypressArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.flag = gs::uploadMipped(vdp, flagArt());
    art.mark = gs::uploadMipped(vdp, markArt());
}

}  // namespace bocceseven

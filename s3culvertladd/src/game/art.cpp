#include "game/art.h"

#include <initializer_list>
#include <string>

namespace culvert {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    gs::TextStyle big{3, 1, 0, 15, 1};
    for (int c = 32; c < 128; ++c) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; ++y)
            for (int x = 0; x < 5; ++x)
                if (g[y * 5 + x]) {
                    px[y * 8 + x + 1] = 1;
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
                }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
    a.wallTile = tiles.used();
}

Bitmap figure(bool stride) {
    Bitmap b(18, 32);
    b.ellipse(9, 5, 4, 4, 2);
    b.rect(7, 9, 5, 8, 1);
    b.rect(5, 11, 3, 5, 3);
    b.rect(11, 10, 4, 3, 4);
    if (!stride) {
        b.rect(6, 17, 3, 10, 1);
        b.rect(10, 17, 3, 10, 1);
    } else {
        b.rect(4, 17, 3, 9, 1);
        b.rect(11, 18, 3, 9, 1);
    }
    b.rect(5, 26, 4, 3, 5);
    b.rect(10, 26, 4, 3, 5);
    b.rect(13, 8, 3, 3, 6);
    return b;
}

Bitmap brickArt() {
    Bitmap b(32, 16);
    b.rect(0, 0, 32, 16, 1);
    b.rect(0, 0, 32, 2, 2);
    b.rect(0, 8, 32, 2, 2);
    b.rect(0, 2, 2, 6, 2);
    b.rect(16, 10, 2, 6, 2);
    b.rect(2, 3, 12, 4, 3);
    b.rect(18, 3, 12, 4, 3);
    b.rect(2, 11, 12, 4, 3);
    b.rect(20, 11, 10, 4, 3);
    return b;
}

Bitmap rungArt() {
    Bitmap b(22, 28);
    b.rect(1, 0, 3, 28, 1);
    b.rect(18, 0, 3, 28, 1);
    b.rect(1, 6, 20, 3, 2);
    b.rect(1, 16, 20, 3, 2);
    b.rect(1, 25, 20, 2, 3);
    return b;
}

Bitmap archArt() {
    Bitmap b(48, 28);
    b.rect(0, 0, 48, 6, 1);
    b.rect(0, 6, 6, 22, 1);
    b.rect(42, 6, 6, 22, 1);
    b.rect(6, 6, 36, 4, 2);
    b.rect(4, 18, 4, 8, 3);
    b.rect(40, 18, 4, 8, 3);
    return b;
}

Bitmap dripArt() {
    Bitmap b(8, 14);
    b.ellipse(4, 8, 3, 5, 1);
    b.rect(3, 1, 2, 5, 2);
    return b;
}

Bitmap lampArt() {
    Bitmap b(10, 10);
    b.ellipse(5, 5, 4, 4, 1);
    b.ellipse(5, 5, 2, 2, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(14, 13, 11), gs::rgb4(6, 6, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 12, 4), gs::rgb4(8, 5, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(8, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(8, 14, 7), gs::rgb4(2, 6, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_MAN, {0, gs::rgb4(4, 6, 8), gs::rgb4(12, 9, 6), gs::rgb4(3, 4, 5), gs::rgb4(15, 13, 6), gs::rgb4(2, 2, 2),
                          gs::rgb4(15, 12, 4), ink});
    setPal(vdp, PAL_BRICK, {0, gs::rgb4(8, 4, 3), gs::rgb4(4, 3, 3), gs::rgb4(11, 6, 4), ink});
    setPal(vdp, PAL_LADDER, {0, gs::rgb4(6, 7, 7), gs::rgb4(11, 11, 9), gs::rgb4(3, 4, 4), ink});
    setPal(vdp, PAL_WATER, {0, gs::rgb4(3, 6, 9), gs::rgb4(6, 10, 12), ink});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(15, 13, 5), gs::rgb4(15, 15, 12), ink});
    setPal(vdp, PAL_IRON, {0, gs::rgb4(5, 5, 6), gs::rgb4(9, 9, 8), gs::rgb4(3, 3, 3), ink});
    setPal(vdp, PAL_WALL, {0, gs::rgb4(5, 3, 3), gs::rgb4(3, 2, 2), gs::rgb4(7, 4, 3), gs::rgb4(2, 3, 4), ink});

    loadFont(vdp, art);

    uint8_t wall[64];
    for (int y = 0; y < 8; ++y)
        for (int x = 0; x < 8; ++x) {
            int c = 1;
            if (y == 0 || y == 4) c = 2;
            else if ((y < 4 && x == 0) || (y >= 4 && x == 4)) c = 2;
            else if ((x + y) % 5 == 0) c = 3;
            wall[y * 8 + x] = uint8_t(c);
        }
    vdp.loadTile(art.wallTile, wall);
    for (int cy = 0; cy < 32; ++cy)
        for (int cx = 0; cx < 64; ++cx) vdp.B.set(cx, cy, gs::entry(art.wallTile, PAL_WALL));

    art.man = gs::uploadMipped(vdp, figure(false));
    art.stride = gs::uploadMipped(vdp, figure(true));
    art.brick = gs::uploadMipped(vdp, brickArt());
    art.rung = gs::uploadMipped(vdp, rungArt());
    art.arch = gs::uploadMipped(vdp, archArt());
    art.drip = gs::uploadMipped(vdp, dripArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
}

}  // namespace culvert

#include "game/art.h"

#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <string>

namespace lotwell {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap watchArt() {
    Bitmap b(40, 28);
    b.rect(4, 8, 28, 12, 2);
    b.rect(6, 6, 16, 6, 3);
    b.rect(8, 7, 10, 4, 8);
    b.rect(28, 10, 8, 6, 4);
    b.rect(30, 8, 4, 4, 6);
    b.ellipse(12, 21, 5, 5, 1);
    b.ellipse(28, 21, 5, 5, 1);
    b.ellipse(12, 21, 2, 2, 7);
    b.ellipse(28, 21, 2, 2, 7);
    b.rect(14, 11, 6, 3, 5);
    b.outline(15, false);
    return b;
}

Bitmap ramArt() {
    Bitmap b(44, 24);
    b.rect(4, 7, 34, 10, 1);
    b.rect(30, 8, 10, 8, 2);
    b.rect(32, 9, 6, 4, 8);
    b.rect(8, 9, 8, 4, 9);
    b.ellipse(12, 18, 5, 5, 3);
    b.ellipse(32, 18, 5, 5, 3);
    b.ellipse(12, 18, 2, 2, 7);
    b.ellipse(32, 18, 2, 2, 7);
    b.rect(6, 12, 4, 2, 6);
    b.rect(18, 6, 8, 3, 4);
    b.outline(15, false);
    return b;
}

Bitmap wellArt() {
    Bitmap b(48, 48);
    b.ellipse(24, 26, 20, 16, 1);
    b.ellipse(24, 24, 16, 12, 2);
    b.ellipse(24, 24, 10, 7, 4);
    b.ellipse(24, 23, 6, 4, 5);
    b.rect(8, 10, 4, 18, 3);
    b.rect(36, 10, 4, 18, 3);
    b.rect(8, 8, 32, 4, 6);
    b.rect(22, 4, 2, 8, 7);
    b.ellipse(23, 4, 2, 2, 8);
    b.rect(14, 30, 6, 2, 9);
    b.rect(26, 18, 5, 2, 9);
    b.outline(15, false);
    return b;
}

Bitmap lampArt() {
    Bitmap b(16, 40);
    b.rect(6, 12, 4, 24, 1);
    b.rect(4, 34, 8, 4, 2);
    b.ellipse(8, 8, 6, 5, 4);
    b.ellipse(8, 8, 3, 2, 5);
    b.outline(15, false);
    return b;
}

Bitmap stallArt() {
    Bitmap b(28, 20);
    b.rect(2, 6, 24, 12, 1);
    b.rect(4, 8, 8, 4, 3);
    b.ellipse(8, 16, 3, 3, 2);
    b.ellipse(20, 16, 3, 3, 2);
    b.outline(15, false);
    return b;
}

Bitmap puffArt() {
    Bitmap b(16, 16);
    b.ellipse(8, 8, 6, 5, 1);
    b.ellipse(8, 8, 3, 2, 2);
    b.outline(15, false);
    return b;
}

Bitmap shadowArt() {
    Bitmap b(24, 10);
    b.ellipse(12, 5, 10, 4, 1);
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
    uint8_t ash[64], stripe[64], curb[64];
    for (int i = 0; i < 64; i++) ash[i] = 1;
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) {
            uint32_t h = uint32_t(x * 19 + y * 7) * 0x9e3779b1u;
            h ^= h >> 13;
            if ((h % 13) == 0) ash[y * 8 + x] = 2;
            else if ((h % 17) == 0) ash[y * 8 + x] = 5;
        }
    for (int i = 0; i < 64; i++) stripe[i] = 1;
    for (int y = 3; y <= 4; y++)
        for (int x = 0; x < 8; x++) stripe[y * 8 + x] = 3;
    for (int i = 0; i < 64; i++) curb[i] = 4;
    for (int y = 0; y < 8; y++) curb[y * 8 + 0] = 6;
    a.asphalt = tiles.alloc(1);
    a.stripe = tiles.alloc(1);
    a.curb = tiles.alloc(1);
    vdp.loadTile(a.asphalt, ash);
    vdp.loadTile(a.stripe, stripe);
    vdp.loadTile(a.curb, curb);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    auto ink = [&](int pal, uint16_t c) {
        setPal(vdp, pal, {0, c, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    };
    ink(PAL_HUD, gs::rgb4(15, 15, 15));
    ink(PAL_DIM, gs::rgb4(8, 8, 11));
    ink(PAL_ALERT, gs::rgb4(15, 4, 3));
    ink(PAL_OK, gs::rgb4(5, 15, 7));
    ink(PAL_GOLD, gs::rgb4(15, 12, 3));

    setPal(vdp, PAL_WATCH,
           {0, gs::rgb4(2, 2, 3), gs::rgb4(4, 8, 12), gs::rgb4(8, 12, 15), gs::rgb4(12, 4, 2), gs::rgb4(15, 10, 2),
            gs::rgb4(15, 14, 6), gs::rgb4(9, 9, 10), gs::rgb4(12, 14, 15), 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_RAM,
           {0, gs::rgb4(12, 3, 2), gs::rgb4(8, 2, 2), gs::rgb4(2, 2, 2), gs::rgb4(14, 12, 4), gs::rgb4(6, 5, 5),
            gs::rgb4(15, 8, 2), gs::rgb4(7, 7, 8), gs::rgb4(10, 13, 15), gs::rgb4(15, 14, 8), 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_WELL,
           {0, gs::rgb4(7, 7, 8), gs::rgb4(10, 10, 11), gs::rgb4(5, 5, 6), gs::rgb4(1, 2, 4), gs::rgb4(2, 4, 8),
            gs::rgb4(9, 8, 6), gs::rgb4(6, 5, 3), gs::rgb4(12, 11, 8), gs::rgb4(4, 4, 5), 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_PROP,
           {0, gs::rgb4(3, 5, 8), gs::rgb4(2, 2, 3), gs::rgb4(8, 10, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_LAMP,
           {0, gs::rgb4(5, 5, 6), gs::rgb4(3, 3, 4), 0, gs::rgb4(14, 12, 4), gs::rgb4(15, 15, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0,
            shadow});
    setPal(vdp, PAL_FX, {0, gs::rgb4(14, 12, 8), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_LOT,
           {0, gs::rgb4(3, 3, 4), gs::rgb4(4, 4, 5), gs::rgb4(12, 10, 2), gs::rgb4(6, 6, 7), gs::rgb4(2, 2, 3),
            gs::rgb4(8, 8, 9), 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    loadFont(vdp, art);
    art.watch = gs::uploadMipped(vdp, watchArt());
    art.ram = gs::uploadMipped(vdp, ramArt());
    art.well = gs::uploadMipped(vdp, wellArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.stall = gs::uploadMipped(vdp, stallArt());
    art.puff = gs::uploadMipped(vdp, puffArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());

    vdp.B.clear();
    vdp.A.clear();
    for (int y = 0; y < 28; y++) {
        for (int x = 0; x < 40; x++) {
            bool edge = x == 0 || x == 39 || y == 0 || y == 27;
            bool stall = (y == 8 || y == 18) && x > 2 && x < 37 && (x % 6) < 4;
            int tile = edge ? art.curb : (stall ? art.stripe : art.asphalt);
            vdp.B.set(x, y, gs::entry(tile, PAL_LOT));
        }
    }
    vdp.setFogColor(gs::rgb4(1, 1, 3));
}

}  // namespace lotwell

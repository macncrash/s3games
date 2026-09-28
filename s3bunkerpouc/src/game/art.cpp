#include "game/art.h"

#include <initializer_list>
#include <string>

namespace bunker {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i >= 16) break;
        vdp.setColor(pal * 16 + i++, c);
    }
    while (i < 15) vdp.setColor(pal * 16 + i++, 0);
    if (i == 15) vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 1));
}

void loadFont(gs::VDP& vdp, Art& a, gs::TileAlloc& tiles) {
    gs::TextStyle big{2, 1, 0, 15, 1};
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
}

using gs::Bitmap;

Bitmap porter(int pose) {
    Bitmap b(36, 56);
    const bool duck = pose == 3;
    const bool leap = pose == 4;
    const int y0 = duck ? 16 : 0;
    b.rect(11, y0 + 2, 14, 5, 5);
    b.rect(10, y0 + 6, 16, 4, 5);
    b.rect(12, y0 + 9, 12, 4, 4);
    b.ellipse(18, y0 + 16, 6, 6, 3);
    b.rect(14, y0 + 15, 8, 3, 4);
    b.set(16, y0 + 16, 8);
    b.set(20, y0 + 16, 8);
    b.rect(10, y0 + 22, 16, 16, 2);
    b.rect(12, y0 + 24, 6, 10, 1);
    b.rect(22, y0 + 28, 3, 8, 7);
    b.rect(8, y0 + 24, 4, 12, 2);
    b.rect(26, y0 + 24, 4, 11, 2);
    if (leap) {
        b.rect(11, y0 + 38, 6, 8, 6);
        b.rect(20, y0 + 38, 6, 8, 6);
        b.rect(9, y0 + 44, 8, 4, 7);
        b.rect(20, y0 + 44, 8, 4, 7);
    } else if (pose == 1) {
        b.rect(12, y0 + 38, 5, 14, 6);
        b.rect(20, y0 + 40, 5, 10, 6);
        b.rect(11, y0 + 50, 7, 4, 7);
        b.rect(19, y0 + 48, 7, 4, 7);
    } else if (pose == 2) {
        b.rect(20, y0 + 38, 5, 14, 6);
        b.rect(12, y0 + 40, 5, 10, 6);
        b.rect(19, y0 + 50, 7, 4, 7);
        b.rect(11, y0 + 48, 7, 4, 7);
    } else if (duck) {
        b.rect(10, y0 + 36, 7, 8, 6);
        b.rect(19, y0 + 36, 7, 8, 6);
        b.rect(8, y0 + 42, 9, 3, 7);
        b.rect(18, y0 + 42, 9, 3, 7);
    } else {
        b.rect(12, y0 + 38, 5, 14, 6);
        b.rect(20, y0 + 38, 5, 14, 6);
        b.rect(11, y0 + 50, 7, 4, 7);
        b.rect(19, y0 + 50, 7, 4, 7);
    }
    return b;
}

Bitmap satchel(int bob) {
    Bitmap b(28, 22);
    b.rect(6, 6, 16, 13, 1);
    b.rect(7, 7, 14, 11, 2);
    b.rect(8, 4, 12, 4, 1);
    b.rect(12, 2, 4, 4, 5);
    b.rect(13, 8, 3, 6, 3);
    b.set(10, 12, 4);
    b.set(18, 12, 4);
    if (bob) b.rect(6, 5, 16, 1, 5);
    return b;
}

Bitmap block(int w, int h, int fill, int edge) {
    Bitmap b(w, h);
    b.rect(0, 0, float(w), float(h), fill);
    b.rect(0, 0, float(w), 2, edge);
    b.rect(0, float(h - 2), float(w), 2, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    vdp.reset();
    gs::TileAlloc tiles(vdp, 1);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 13, 9), gs::rgb4(6, 6, 5)});
    setPal(vdp, PAL_CONC,
           {0, gs::rgb4(3, 4, 3), gs::rgb4(5, 6, 5), gs::rgb4(8, 8, 7), gs::rgb4(4, 5, 3), gs::rgb4(7, 5, 3)});
    setPal(vdp, PAL_STEEL,
           {0, gs::rgb4(2, 3, 3), gs::rgb4(5, 6, 6), gs::rgb4(9, 10, 9), gs::rgb4(12, 10, 3), gs::rgb4(8, 3, 2)});
    setPal(vdp, PAL_POUCH,
           {0, gs::rgb4(6, 4, 2), gs::rgb4(10, 8, 4), gs::rgb4(4, 3, 2), gs::rgb4(13, 10, 3), gs::rgb4(3, 3, 2)});
    setPal(vdp, PAL_PLAYER,
           {0, gs::rgb4(2, 4, 2), gs::rgb4(4, 6, 3), gs::rgb4(11, 8, 6), gs::rgb4(1, 2, 2), gs::rgb4(3, 5, 3),
            gs::rgb4(2, 2, 2), gs::rgb4(6, 5, 3), gs::rgb4(12, 12, 8)});
    setPal(vdp, PAL_STEAM, {0, gs::rgb4(6, 7, 7), gs::rgb4(12, 13, 13), gs::rgb4(8, 12, 12), gs::rgb4(4, 5, 5)});
    setPal(vdp, PAL_SUMP, {0, gs::rgb4(1, 2, 2), gs::rgb4(1, 4, 3), gs::rgb4(3, 7, 5), gs::rgb4(2, 3, 2)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(13, 3, 2), gs::rgb4(8, 2, 2), gs::rgb4(14, 8, 3)});
    setPal(vdp, PAL_GO, {0, gs::rgb4(4, 12, 5), gs::rgb4(2, 7, 3), gs::rgb4(10, 14, 8)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(4, 3, 1), gs::rgb4(13, 9, 2), gs::rgb4(14, 13, 6), gs::rgb4(6, 5, 2)});
    vdp.setFogColor(gs::rgb4(2, 3, 2));
    loadFont(vdp, art, tiles);

    art.stand = gs::uploadMipped(vdp, porter(0));
    art.runA = gs::uploadMipped(vdp, porter(1));
    art.runB = gs::uploadMipped(vdp, porter(2));
    art.duck = gs::uploadMipped(vdp, porter(3));
    art.leap = gs::uploadMipped(vdp, porter(4));
    art.pouch[0] = gs::uploadMipped(vdp, satchel(0));
    art.pouch[1] = gs::uploadMipped(vdp, satchel(1));

    art.crate = gs::uploadMipped(vdp, block(40, 28, 2, 3));
    {
        Bitmap b(28, 96);
        b.rect(4, 0, 20, 96, 1);
        b.rect(8, 0, 12, 96, 2);
        for (int y = 8; y < 90; y += 16) b.rect(6, float(y), 16, 3, 3);
        art.rib = gs::uploadMipped(vdp, b);
    }
    {
        Bitmap b(36, 80);
        b.rect(2, 0, 32, 80, 1);
        b.rect(4, 2, 28, 76, 2);
        for (int y = 8; y < 74; y += 14) {
            b.rect(8, float(y), 20, 2, 3);
            b.set(10, y + 6, 4);
            b.set(24, y + 6, 4);
        }
        b.rect(0, 74, 36, 6, 5);
        art.slab = gs::uploadMipped(vdp, b);
    }
    {
        Bitmap b(70, 100);
        b.rect(4, 8, 62, 92, 1);
        b.rect(10, 14, 50, 78, 2);
        b.rect(18, 28, 34, 10, 5);
        b.rect(22, 48, 26, 36, 1);
        b.rect(28, 20, 14, 6, 4);
        art.door = gs::uploadMipped(vdp, b);
    }
    {
        Bitmap b(16, 28);
        b.rect(6, 0, 4, 10, 1);
        b.rect(2, 8, 12, 12, 1);
        b.rect(4, 10, 8, 8, 2);
        b.rect(6, 20, 4, 8, 1);
        art.lamp = gs::uploadMipped(vdp, b);
    }
    art.floor = gs::uploadMipped(vdp, block(64, 16, 2, 3));
    {
        Bitmap b(48, 16);
        b.ellipse(24, 10, 20, 5, 2);
        b.rect(6, 6, 36, 4, 3);
        b.rect(10, 4, 28, 2, 1);
        art.pipe = gs::uploadMipped(vdp, b);
    }
    {
        Bitmap a(20, 28);
        a.ellipse(10, 16, 7, 10, 1);
        a.ellipse(10, 14, 4, 6, 2);
        Bitmap c(20, 28);
        c.ellipse(10, 12, 8, 8, 2);
        c.ellipse(8, 18, 4, 5, 3);
        art.puff[0] = gs::uploadMipped(vdp, a);
        art.puff[1] = gs::uploadMipped(vdp, c);
    }
    {
        Bitmap b(64, 36);
        b.rect(0, 0, 64, 36, 1);
        b.ellipse(20, 14, 14, 5, 2);
        b.ellipse(44, 22, 12, 4, 3);
        art.sump = gs::uploadMipped(vdp, b);
    }
    {
        Bitmap b(40, 10);
        b.rect(0, 2, 40, 6, 1);
        for (int x = 3; x < 38; x += 6) b.rect(float(x), 2, 2, 6, 3);
        art.grate = gs::uploadMipped(vdp, b);
    }
    {
        Bitmap b(28, 8);
        b.ellipse(14, 4, 12, 3, 1);
        art.shadow = gs::uploadMipped(vdp, b);
    }
}

}  // namespace bunker

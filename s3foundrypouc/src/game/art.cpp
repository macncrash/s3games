#include "game/art.h"

#include <initializer_list>
#include <string>

namespace foundrypouc {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i >= 16) break;
        vdp.setColor(pal * 16 + i++, c);
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

int brickTile(gs::TileAlloc& al, gs::VDP& vdp) {
    uint8_t px[64];
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int v = 2;
            bool bed = (y == 0) || (y == 4);
            int shift = (y >= 4) ? 4 : 0;
            bool joint = ((x + shift) % 8) == 0;
            if (bed || joint) v = 5;
            else if ((x * 3 + y) % 7 == 0) v = 3;
            else if (y > 5) v = 1;
            px[y * 8 + x] = uint8_t(v);
        }
    }
    int t = al.alloc(1);
    vdp.loadTile(t, px);
    return t;
}

void paintWall(gs::VDP& vdp, gs::TileAlloc& tiles) {
    int brick = brickTile(tiles, vdp);
    vdp.B.resize(64, 32);
    vdp.A.resize(64, 32);
    vdp.B.clear();
    vdp.A.clear();
    vdp.A.enabled = false;
    for (int cy = 0; cy < 18; cy++) {
        for (int cx = 0; cx < 64; cx++) vdp.B.set(cx, cy, gs::entry(brick, PAL_BRICK));
    }
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

Bitmap worker(int pose) {
    Bitmap b(36, 58);
    const bool duck = pose == 3;
    const bool leap = pose == 4;
    const int y0 = duck ? 18 : 0;
    b.rect(11, y0 + 1, 14, 5, 8);
    b.rect(12, y0 + 5, 12, 3, 6);
    b.rect(13, y0 + 7, 10, 2, 3);
    b.ellipse(18, y0 + 14, 6, 6, 4);
    b.set(21, y0 + 13, 9);
    b.set(22, y0 + 13, 9);
    b.rect(15, y0 + 18, 6, 2, 5);
    b.rect(10, y0 + 20, 16, 16, 2);
    b.rect(12, y0 + 22, 5, 10, 1);
    b.rect(23, y0 + 22, 4, 9, 7);
    if (pose == 1) b.rect(25, y0 + 24, 6, 8, 2);
    else if (pose == 2) b.rect(4, y0 + 24, 6, 8, 2);
    else b.rect(7, y0 + 22, 4, 10, 2);
    if (leap) {
        b.rect(10, y0 + 36, 6, 8, 5);
        b.rect(20, y0 + 36, 6, 8, 5);
        b.rect(8, y0 + 42, 8, 3, 7);
        b.rect(20, y0 + 42, 8, 3, 7);
    } else if (duck) {
        b.rect(9, y0 + 34, 8, 6, 5);
        b.rect(19, y0 + 34, 8, 6, 5);
        b.rect(8, y0 + 38, 10, 3, 7);
        b.rect(18, y0 + 38, 10, 3, 7);
    } else if (pose == 1) {
        b.rect(11, y0 + 36, 5, 14, 5);
        b.rect(20, y0 + 38, 5, 10, 5);
        b.rect(10, y0 + 48, 7, 4, 7);
        b.rect(19, y0 + 46, 7, 4, 7);
    } else if (pose == 2) {
        b.rect(20, y0 + 36, 5, 14, 5);
        b.rect(11, y0 + 38, 5, 10, 5);
        b.rect(19, y0 + 48, 7, 4, 7);
        b.rect(10, y0 + 46, 7, 4, 7);
    } else {
        b.rect(11, y0 + 36, 5, 14, 5);
        b.rect(20, y0 + 36, 5, 14, 5);
        b.rect(10, y0 + 48, 7, 4, 7);
        b.rect(19, y0 + 48, 7, 4, 7);
    }
    b.outline(15, false);
    return b;
}

Bitmap satchel(int glint) {
    Bitmap b(26, 22);
    b.ellipse(13, 14, 10, 7, 2);
    b.ellipse(13, 13, 8, 5, 1);
    b.rect(6, 8, 14, 3, 4);
    b.rect(11, 5, 4, 6, 5);
    b.rect(12, 10, 2, 3, 6);
    if (glint) b.rect(9, 12, 2, 3, 7);
    b.line(7, 7, 13, 2, 3, 2);
    b.line(19, 7, 13, 2, 3, 2);
    b.outline(15, false);
    return b;
}

Bitmap furnace() {
    Bitmap b(64, 72);
    b.rect(6, 8, 52, 58, 2);
    b.rect(10, 14, 44, 40, 1);
    b.rect(16, 22, 32, 22, 4);
    b.rect(20, 26, 24, 14, 5);
    b.rect(4, 62, 56, 8, 3);
    b.rect(18, 4, 8, 10, 6);
    b.rect(36, 2, 8, 12, 6);
    b.rect(14, 48, 36, 6, 8);
    b.outline(15, false);
    return b;
}

Bitmap ladle() {
    Bitmap b(28, 36);
    b.rect(12, 2, 4, 16, 3);
    b.ellipse(14, 26, 10, 8, 2);
    b.ellipse(14, 25, 7, 5, 5);
    b.rect(10, 1, 8, 3, 4);
    b.outline(15, false);
    return b;
}

Bitmap quenchDoor() {
    Bitmap b(40, 64);
    b.rect(2, 4, 36, 56, 2);
    b.rect(8, 10, 24, 40, 1);
    b.rect(12, 16, 16, 28, 4);
    b.rect(18, 36, 4, 8, 6);
    b.rect(0, 56, 40, 6, 3);
    b.outline(15, false);
    return b;
}

Bitmap stack() {
    Bitmap b(16, 28);
    b.rect(4, 6, 8, 20, 2);
    b.rect(3, 2, 10, 6, 1);
    b.ellipse(8, 4, 3, 2, 4);
    b.outline(15, false);
    return b;
}

Bitmap pipeSeg() {
    Bitmap b(32, 16);
    b.rect(0, 3, 32, 10, 2);
    b.rect(0, 5, 32, 3, 4);
    b.rect(0, 11, 32, 2, 1);
    b.outline(15, false);
    return b;
}

Bitmap spark() {
    Bitmap b(8, 8);
    b.rect(3, 1, 2, 6, 2);
    b.rect(1, 3, 6, 2, 2);
    b.set(3, 3, 3);
    b.set(4, 4, 3);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 14, 10), gs::rgb4(8, 7, 5), gs::rgb4(4, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_BRICK, {0, gs::rgb4(5, 2, 1), gs::rgb4(8, 3, 2), gs::rgb4(11, 5, 3), gs::rgb4(3, 1, 1), gs::rgb4(2, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 0, 0)});
    setPal(vdp, PAL_IRON, {0, gs::rgb4(3, 3, 4), gs::rgb4(7, 7, 8), gs::rgb4(11, 11, 12), gs::rgb4(14, 14, 13), gs::rgb4(5, 5, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_POUCH, {0, gs::rgb4(8, 4, 1), gs::rgb4(12, 7, 2), gs::rgb4(6, 3, 1), gs::rgb4(10, 8, 3), gs::rgb4(4, 2, 1), gs::rgb4(14, 12, 6), gs::rgb4(15, 15, 10), 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 1, 0)});
    setPal(vdp, PAL_WORKER, {0, gs::rgb4(6, 6, 7), gs::rgb4(9, 4, 2), gs::rgb4(2, 2, 3), gs::rgb4(13, 9, 6), gs::rgb4(4, 3, 3), gs::rgb4(5, 5, 6), gs::rgb4(3, 2, 2), gs::rgb4(7, 7, 8), gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, gs::rgb4(1, 0, 0)});
    setPal(vdp, PAL_SLAG, {0, gs::rgb4(8, 2, 0), gs::rgb4(14, 6, 1), gs::rgb4(15, 12, 3), gs::rgb4(6, 1, 0), gs::rgb4(12, 3, 0), 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 0, 0)});
    setPal(vdp, PAL_FURNACE, {0, gs::rgb4(4, 3, 3), gs::rgb4(7, 6, 5), gs::rgb4(3, 2, 2), gs::rgb4(15, 8, 1), gs::rgb4(15, 13, 4), gs::rgb4(5, 5, 5), 0, gs::rgb4(2, 2, 2), 0, 0, 0, 0, 0, 0, gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_LADLE, {0, gs::rgb4(4, 4, 5), gs::rgb4(8, 8, 9), gs::rgb4(12, 12, 13), gs::rgb4(6, 6, 7), gs::rgb4(15, 9, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_SPARK, {0, gs::rgb4(12, 4, 0), gs::rgb4(15, 12, 3), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_DOOR, {0, gs::rgb4(2, 5, 6), gs::rgb4(4, 8, 9), gs::rgb4(2, 3, 4), gs::rgb4(6, 13, 12), gs::rgb4(10, 14, 8), gs::rgb4(14, 15, 10), 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(0, 2, 2)});
    setPal(vdp, PAL_SOOT, {0, gs::rgb4(3, 3, 3), gs::rgb4(6, 6, 6), 0, gs::rgb4(8, 8, 7), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 1, 1)});
    vdp.setFogColor(gs::rgb4(2, 1, 1));

    gs::TileAlloc tiles(vdp, 1);
    paintWall(vdp, tiles);
    loadFont(vdp, art, tiles);

    art.stand = gs::uploadMipped(vdp, worker(0));
    art.runA = gs::uploadMipped(vdp, worker(1));
    art.runB = gs::uploadMipped(vdp, worker(2));
    art.duck = gs::uploadMipped(vdp, worker(3));
    art.leap = gs::uploadMipped(vdp, worker(4));
    art.pouch[0] = gs::uploadMipped(vdp, satchel(0));
    art.pouch[1] = gs::uploadMipped(vdp, satchel(1));
    art.furnace = gs::uploadMipped(vdp, furnace());
    art.ladle = gs::uploadMipped(vdp, ladle());
    art.door = gs::uploadMipped(vdp, quenchDoor());
    art.stack = gs::uploadMipped(vdp, stack());
    art.pipe = gs::uploadMipped(vdp, pipeSeg());
    art.spark = gs::uploadMipped(vdp, spark());
}

}  // namespace foundrypouc

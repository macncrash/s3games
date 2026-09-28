#include "game/art.h"

#include <initializer_list>
#include <string>

namespace alley {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i >= 16) break;
        vdp.setColor(pal * 16 + i++, c);
    }
    while (i < 15) vdp.setColor(pal * 16 + i++, 0);
    if (i == 15) vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

int brickTile(gs::TileAlloc& al, gs::VDP& vdp, int mortar) {
    uint8_t px[64];
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int v = 2;
            bool bed = (y == 0) || (y == 4);
            int shift = (y >= 4) ? 4 : 0;
            bool joint = ((x + shift) % 8) == 0;
            if (bed || joint) v = mortar;
            else if ((x + y) % 5 == 0) v = 3;
            px[y * 8 + x] = uint8_t(v);
        }
    }
    int t = al.alloc(1);
    vdp.loadTile(t, px);
    return t;
}

int stoneTile(gs::TileAlloc& al, gs::VDP& vdp) {
    uint8_t px[64];
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int v = 1;
            if (y == 0 || x == 0) v = 4;
            else if ((x + y * 3) % 7 == 0) v = 3;
            else if (y > 5) v = 2;
            px[y * 8 + x] = uint8_t(v);
        }
    }
    int t = al.alloc(1);
    vdp.loadTile(t, px);
    return t;
}

void paintAlley(gs::VDP& vdp, gs::TileAlloc& tiles) {
    int brick = brickTile(tiles, vdp, 5);
    int dark = brickTile(tiles, vdp, 6);
    int stone = stoneTile(tiles, vdp);
    vdp.B.resize(64, 32);
    vdp.A.resize(64, 32);
    vdp.B.clear();
    vdp.A.clear();
    for (int cy = 0; cy < 32; cy++) {
        for (int cx = 0; cx < 64; cx++) {
            if (cy < 18) vdp.B.set(cx, cy, gs::entry((cx + cy) & 1 ? brick : dark, PAL_BRICK));
            if (cy >= 22 && cy <= 26) vdp.A.set(cx, cy, gs::entry(stone, PAL_STONE));
        }
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

Bitmap runner(int pose) {
    Bitmap b(36, 60);
    const bool duck = pose == 3;
    const bool leap = pose == 4;
    const int y0 = duck ? 16 : 0;
    b.rect(10, y0 + 2, 16, 5, 6);
    b.rect(12, y0 + 6, 12, 3, 6);
    b.ellipse(18, y0 + 14, 6, 6, 4);
    b.set(21, y0 + 13, 3);
    b.set(22, y0 + 13, 3);
    b.rect(15, y0 + 17, 5, 1, 8);
    b.rect(14, y0 + 18, 6, 2, 9);
    b.rect(9, y0 + 20, 18, 16, 2);
    b.rect(11, y0 + 22, 6, 10, 1);
    b.rect(24, y0 + 22, 4, 8, 7);
    if (pose == 1) b.rect(26, y0 + 24, 6, 10, 2);
    else if (pose == 2) b.rect(4, y0 + 24, 6, 10, 2);
    else b.rect(6, y0 + 22, 4, 12, 2);
    if (leap) {
        b.rect(10, y0 + 36, 6, 8, 5);
        b.rect(20, y0 + 36, 6, 8, 5);
        b.rect(8, y0 + 42, 8, 4, 7);
        b.rect(20, y0 + 42, 8, 4, 7);
    } else if (pose == 1) {
        b.rect(11, y0 + 36, 5, 16, 5);
        b.rect(20, y0 + 38, 5, 12, 5);
        b.rect(10, y0 + 50, 7, 4, 7);
        b.rect(19, y0 + 48, 7, 4, 7);
    } else if (pose == 2) {
        b.rect(20, y0 + 36, 5, 16, 5);
        b.rect(11, y0 + 38, 5, 12, 5);
        b.rect(19, y0 + 50, 7, 4, 7);
        b.rect(10, y0 + 48, 7, 4, 7);
    } else if (duck) {
        b.rect(9, y0 + 34, 7, 6, 5);
        b.rect(19, y0 + 34, 7, 6, 5);
        b.rect(8, y0 + 38, 9, 3, 7);
        b.rect(18, y0 + 38, 9, 3, 7);
    } else {
        b.rect(11, y0 + 36, 5, 16, 5);
        b.rect(20, y0 + 36, 5, 16, 5);
        b.rect(10, y0 + 50, 7, 4, 7);
        b.rect(19, y0 + 50, 7, 4, 7);
    }
    b.outline(15, false);
    return b;
}

Bitmap satchel(int glint) {
    Bitmap b(28, 24);
    b.ellipse(14, 15, 11, 8, 2);
    b.ellipse(14, 14, 9, 6, 1);
    b.rect(6, 8, 16, 4, 3);
    b.rect(12, 6, 4, 6, 5);
    if (glint) b.rect(10, 12, 2, 3, 6);
    b.line(8, 7, 14, 2, 4, 2);
    b.line(20, 7, 14, 2, 4, 2);
    b.outline(15, false);
    return b;
}

Bitmap ashbin() {
    Bitmap b(28, 32);
    b.rect(4, 8, 20, 20, 2);
    b.rect(6, 10, 16, 6, 3);
    b.rect(2, 6, 24, 4, 4);
    b.rect(8, 26, 4, 4, 5);
    b.rect(16, 26, 4, 4, 5);
    b.rect(10, 2, 8, 5, 1);
    b.outline(15, false);
    return b;
}

Bitmap lampPost() {
    Bitmap b(16, 48);
    b.rect(6, 10, 4, 36, 2);
    b.rect(4, 44, 8, 3, 3);
    b.rect(3, 4, 10, 8, 4);
    b.rect(5, 6, 6, 4, 1);
    b.outline(15, false);
    return b;
}

Bitmap flameArt(int frame) {
    Bitmap b(10, 14);
    b.ellipse(5, 8, 3, 5, 2);
    b.ellipse(5, frame ? 7 : 6, 2, 3, 1);
    b.set(5, 3, 3);
    return b;
}

Bitmap grateArt() {
    Bitmap b(40, 28);
    b.rect(2, 2, 36, 24, 1);
    for (int x = 6; x < 36; x += 6) b.rect(x, 4, 2, 20, 3);
    b.rect(4, 12, 32, 2, 3);
    b.outline(15, false);
    return b;
}

Bitmap lipArt() {
    Bitmap b(12, 18);
    b.rect(2, 2, 8, 14, 2);
    b.rect(4, 4, 4, 8, 3);
    b.outline(15, false);
    return b;
}

Bitmap lineArt() {
    Bitmap b(64, 6);
    b.rect(0, 2, 64, 2, 1);
    for (int x = 4; x < 60; x += 8) b.set(x, 1, 2);
    return b;
}

Bitmap pegArt() {
    Bitmap b(8, 36);
    b.rect(2, 0, 4, 34, 2);
    b.rect(1, 32, 6, 3, 3);
    b.outline(15, false);
    return b;
}

Bitmap doorArt() {
    Bitmap b(36, 64);
    b.rect(2, 4, 32, 58, 2);
    b.rect(6, 8, 24, 48, 1);
    b.rect(8, 12, 8, 12, 4);
    b.rect(20, 12, 8, 12, 3);
    b.rect(8, 28, 8, 12, 3);
    b.rect(20, 28, 8, 12, 4);
    b.ellipse(24, 36, 2, 2, 5);
    b.rect(0, 0, 36, 6, 6);
    b.outline(15, false);
    return b;
}

Bitmap stairArt() {
    Bitmap b(28, 20);
    b.poly({{2, 18}, {26, 18}, {26, 12}, {16, 12}, {16, 6}, {6, 6}, {6, 18}}, 2);
    b.outline(15, false);
    return b;
}

Bitmap shadowArt() {
    Bitmap b(28, 8);
    b.ellipse(14, 4, 12, 3, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 13, 11), gs::rgb4(8, 8, 9), gs::rgb4(4, 4, 6)});
    setPal(vdp, PAL_BRICK,
           {0, gs::rgb4(6, 3, 3), gs::rgb4(9, 4, 3), gs::rgb4(12, 6, 4), gs::rgb4(4, 2, 2), gs::rgb4(3, 2, 2),
            gs::rgb4(2, 1, 2)});
    setPal(vdp, PAL_STONE,
           {0, gs::rgb4(5, 5, 6), gs::rgb4(3, 3, 4), gs::rgb4(8, 8, 8), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_POUCH,
           {0, gs::rgb4(10, 6, 2), gs::rgb4(7, 4, 1), gs::rgb4(13, 9, 3), gs::rgb4(4, 2, 1), gs::rgb4(14, 12, 6),
            gs::rgb4(15, 15, 10)});
    setPal(vdp, PAL_PLAYER,
           {0, gs::rgb4(3, 5, 9), gs::rgb4(2, 3, 6), gs::rgb4(1, 1, 2), gs::rgb4(13, 9, 7), gs::rgb4(4, 4, 5),
            gs::rgb4(1, 1, 1), gs::rgb4(8, 2, 2), gs::rgb4(12, 3, 3), gs::rgb4(14, 8, 7)});
    setPal(vdp, PAL_IRON, {0, gs::rgb4(12, 12, 10), gs::rgb4(6, 6, 7), gs::rgb4(3, 3, 4), gs::rgb4(9, 9, 8)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(8, 5, 2), gs::rgb4(5, 3, 1), gs::rgb4(10, 7, 3), gs::rgb4(3, 2, 1), gs::rgb4(12, 9, 4)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(15, 14, 6), gs::rgb4(14, 8, 2), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(8, 1, 1), gs::rgb4(15, 10, 6)});
    setPal(vdp, PAL_DRAIN, {0, gs::rgb4(1, 1, 2), gs::rgb4(2, 3, 4), gs::rgb4(4, 5, 6)});
    setPal(vdp, PAL_DOOR,
           {0, gs::rgb4(2, 6, 3), gs::rgb4(1, 3, 2), gs::rgb4(4, 8, 5), gs::rgb4(8, 10, 6), gs::rgb4(14, 12, 4),
            gs::rgb4(6, 5, 3)});
    setPal(vdp, PAL_GO, {0, gs::rgb4(6, 14, 7), gs::rgb4(2, 8, 3), gs::rgb4(12, 15, 10)});

    vdp.setFogColor(gs::rgb4(1, 1, 3));
    gs::TileAlloc tiles(vdp, 1);
    paintAlley(vdp, tiles);
    loadFont(vdp, art, tiles);

    art.stand = gs::uploadMipped(vdp, runner(0));
    art.runA = gs::uploadMipped(vdp, runner(1));
    art.runB = gs::uploadMipped(vdp, runner(2));
    art.duck = gs::uploadMipped(vdp, runner(3));
    art.leap = gs::uploadMipped(vdp, runner(4));
    art.pouch[0] = gs::uploadMipped(vdp, satchel(0));
    art.pouch[1] = gs::uploadMipped(vdp, satchel(1));
    art.bin = gs::uploadMipped(vdp, ashbin());
    art.lamp = gs::uploadMipped(vdp, lampPost());
    art.flame[0] = gs::uploadMipped(vdp, flameArt(0));
    art.flame[1] = gs::uploadMipped(vdp, flameArt(1));
    art.grate = gs::uploadMipped(vdp, grateArt());
    art.lip = gs::uploadMipped(vdp, lipArt());
    art.line = gs::uploadMipped(vdp, lineArt());
    art.peg = gs::uploadMipped(vdp, pegArt());
    art.door = gs::uploadMipped(vdp, doorArt());
    art.stair = gs::uploadMipped(vdp, stairArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
}

}  // namespace alley

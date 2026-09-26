#include "game/art.h"

#include <initializer_list>
#include <string>

namespace pouc {
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

int makeTile(gs::TileAlloc& al, gs::VDP& vdp, std::initializer_list<const char*> rows) {
    uint8_t px[64] = {};
    int y = 0;
    for (const char* row : rows) {
        if (y >= 8) break;
        for (int x = 0; x < 8 && row[x]; x++) {
            char c = row[x];
            int v = 0;
            if (c >= '0' && c <= '9') v = c - '0';
            else if (c >= 'a' && c <= 'f') v = c - 'a' + 10;
            px[y * 8 + x] = uint8_t(v);
        }
        y++;
    }
    int t = al.alloc(1);
    vdp.loadTile(t, px);
    return t;
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
    Bitmap b(40, 64);
    const bool duck = pose == 3;
    const bool leap = pose == 4;
    const int y0 = duck ? 8 : 0;
    b.rect(12, y0 + 4, 16, 6, 5);
    b.rect(10, y0 + 8, 20, 3, 5);
    b.set(26, y0 + 6, 8);
    b.ellipse(20, y0 + 16, 7, 7, 4);
    b.set(23, y0 + 15, 3);
    b.set(24, y0 + 15, 3);
    b.rect(17, y0 + 19, 5, 1, 9);
    b.rect(11, y0 + 22, 18, 18, 2);
    b.rect(13, y0 + 24, 8, 12, 1);
    b.set(20, y0 + 28, 8);
    b.set(20, y0 + 32, 8);
    b.set(20, y0 + 36, 8);
    if (pose == 1) b.rect(28, y0 + 24, 5, 12, 2);
    else if (pose == 2) b.rect(7, y0 + 26, 5, 12, 2);
    else b.rect(8, y0 + 24, 5, 14, 2);
    b.rect(29, y0 + 30, 4, 3, 8);
    if (leap) {
        b.rect(12, y0 + 40, 7, 10, 6);
        b.rect(22, y0 + 40, 7, 10, 6);
        b.rect(10, y0 + 48, 8, 5, 7);
        b.rect(24, y0 + 48, 8, 5, 7);
    } else if (pose == 1) {
        b.rect(13, y0 + 40, 6, 16, 6);
        b.rect(23, y0 + 42, 6, 12, 6);
        b.rect(12, y0 + 54, 8, 5, 7);
        b.rect(22, y0 + 52, 8, 5, 7);
    } else if (pose == 2) {
        b.rect(22, y0 + 40, 6, 16, 6);
        b.rect(13, y0 + 42, 6, 12, 6);
        b.rect(21, y0 + 54, 8, 5, 7);
        b.rect(12, y0 + 52, 8, 5, 7);
    } else if (duck) {
        b.rect(12, y0 + 38, 7, 10, 6);
        b.rect(22, y0 + 38, 7, 10, 6);
        b.rect(10, y0 + 46, 9, 4, 7);
        b.rect(22, y0 + 46, 9, 4, 7);
    } else {
        b.rect(13, y0 + 40, 6, 16, 6);
        b.rect(22, y0 + 40, 6, 16, 6);
        b.rect(12, y0 + 54, 8, 5, 7);
        b.rect(21, y0 + 54, 8, 5, 7);
    }
    b.outline(15, false);
    return b;
}

Bitmap pouchArt(int glint) {
    Bitmap b(32, 28);
    b.ellipse(16, 17, 12, 9, 2);
    b.ellipse(16, 16, 10, 7, 1);
    b.poly({{7, 9}, {25, 9}, {21, 15}, {11, 15}}, 3);
    b.rect(14, 13, 4, 5, 5);
    if (glint) b.set(15, 14, 6);
    b.rect(5, 7, 3, 9, 4);
    b.rect(24, 7, 3, 9, 4);
    b.line(6, 8, 16, 4, 4, 2);
    b.line(26, 8, 16, 4, 4, 2);
    b.outline(15, false);
    return b;
}

Bitmap deskArt() {
    Bitmap b(52, 40);
    b.rect(4, 10, 44, 7, 1);
    b.rect(6, 16, 40, 4, 2);
    b.rect(8, 20, 5, 16, 3);
    b.rect(39, 20, 5, 16, 3);
    b.rect(10, 4, 16, 7, 5);
    b.rect(28, 6, 10, 5, 6);
    b.outline(4, false);
    return b;
}

Bitmap crateArt() {
    Bitmap b(32, 32);
    b.rect(1, 1, 30, 30, 2);
    b.rect(4, 4, 24, 24, 1);
    b.line(5, 5, 26, 26, 3, 2);
    b.line(26, 5, 5, 26, 3, 2);
    b.rect(1, 14, 30, 3, 3);
    b.outline(4, false);
    return b;
}

Bitmap barrelArt() {
    Bitmap b(28, 34);
    b.ellipse(14, 17, 11, 14, 2);
    b.ellipse(14, 16, 9, 11, 1);
    b.rect(4, 8, 20, 3, 5);
    b.rect(4, 22, 20, 3, 5);
    b.outline(4, false);
    return b;
}

Bitmap bayArt() {
    Bitmap b(72, 64);
    b.rect(8, 2, 56, 30, 3);
    b.rect(12, 6, 48, 22, 1);
    b.rect(24, 12, 24, 12, 6);
    b.rect(0, 30, 72, 8, 2);
    b.rect(6, 38, 6, 22, 3);
    b.rect(60, 38, 6, 22, 3);
    b.rect(30, 32, 12, 4, 5);
    b.outline(4, false);
    return b;
}

Bitmap matArt() {
    Bitmap b(48, 14);
    b.rect(0, 3, 48, 8, 2);
    b.rect(3, 5, 42, 4, 1);
    return b;
}

Bitmap lampArt() {
    Bitmap b(18, 52);
    b.rect(7, 12, 4, 32, 3);
    b.rect(3, 8, 12, 5, 2);
    b.rect(1, 44, 16, 5, 1);
    b.outline(15, false);
    return b;
}

Bitmap flameArt(int hot) {
    Bitmap b(12, 14);
    b.ellipse(6, 8, 4, 5, 4);
    b.ellipse(6, 8, 2, 3, hot ? 5 : 2);
    b.set(6, 3, 5);
    return b;
}

Bitmap holeArt() {
    Bitmap b(64, 40);
    b.rect(0, 0, 64, 40, 1);
    b.rect(4, 6, 56, 30, 2);
    b.rect(8, 14, 48, 4, 5);
    b.rect(8, 26, 48, 4, 5);
    b.rect(6, 16, 52, 2, 3);
    b.rect(6, 28, 52, 2, 4);
    return b;
}

Bitmap lipArt() {
    Bitmap b(16, 22);
    b.rect(0, 0, 16, 5, 4);
    b.rect(0, 5, 16, 3, 5);
    b.rect(0, 8, 16, 12, 2);
    b.rect(0, 8, 16, 2, 1);
    return b;
}

Bitmap deckArt() {
    Bitmap b(128, 20);
    b.rect(0, 2, 128, 16, 2);
    b.rect(0, 2, 128, 3, 1);
    b.rect(0, 15, 128, 3, 3);
    for (int x = 2; x < 124; x += 16) {
        b.line(float(x), 6, float(x + 8), 14, 4, 3);
        b.line(float(x + 8), 6, float(x), 14, 4, 3);
    }
    b.outline(6, false);
    return b;
}

Bitmap doorArt() {
    Bitmap b(40, 96);
    b.rect(2, 0, 36, 94, 2);
    b.rect(5, 4, 30, 38, 1);
    b.rect(5, 48, 30, 40, 3);
    b.rect(6, 6, 28, 34, 4);
    b.rect(17, 40, 6, 12, 5);
    for (int y : {8, 40, 52, 86}) {
        b.set(6, y, 5);
        b.set(33, y, 5);
    }
    b.outline(6, false);
    return b;
}

Bitmap hookArt() {
    Bitmap b(28, 36);
    b.rect(11, 0, 5, 16, 2);
    b.rect(11, 14, 12, 5, 2);
    b.rect(19, 8, 5, 11, 1);
    b.rect(12, 2, 3, 6, 1);
    b.outline(6, false);
    return b;
}

Bitmap cableArt() {
    Bitmap b(4, 16);
    b.rect(1, 0, 2, 16, 2);
    return b;
}

Bitmap beamArt() {
    Bitmap b(160, 16);
    b.rect(0, 4, 160, 8, 2);
    b.rect(0, 4, 160, 2, 1);
    for (int x = 0; x < 160; x += 14) b.rect(x, 4, 2, 8, 3);
    return b;
}

Bitmap postArt() {
    Bitmap b(16, 80);
    b.rect(5, 0, 6, 74, 2);
    b.rect(2, 0, 12, 6, 1);
    b.rect(2, 70, 12, 8, 3);
    b.outline(6, false);
    return b;
}

Bitmap signalArt() {
    Bitmap b(10, 10);
    b.ellipse(5, 5, 4, 4, 1);
    b.ellipse(5, 5, 2, 2, 3);
    return b;
}

Bitmap shadowArt() {
    Bitmap b(40, 12);
    b.ellipse(20, 6, 16, 4, 1);
    return b;
}

void paintHall(gs::VDP& vdp, gs::TileAlloc& tiles) {
    int truss = makeTile(tiles, vdp, {"55555555", "50555505", "05500550", "00555500", "00055000", "00555500",
                                      "05500550", "50555505"});
    int rail = makeTile(tiles, vdp, {"22222222", "55555555", "51111115", "55555555", "22222222", "11111111",
                                     "11111111", "11111111"});
    int brick = makeTile(tiles, vdp, {"22212221", "22212221", "44444444", "12221222", "12221222", "44444444",
                                      "22122212", "22122212"});
    int dark = makeTile(tiles, vdp, {"11121112", "11121112", "33333333", "21112111", "21112111", "33333333",
                                     "11211121", "11211121"});
    int window = makeTile(tiles, vdp, {"88888888", "86666668", "86777768", "86777768", "86777768", "86666668",
                                       "88888888", "22222222"});
    int cornice = makeTile(tiles, vdp, {"99999999", "88888888", "55555555", "22222222", "22222222", "11111111",
                                        "11111111", "11111111"});
    int skirt = makeTile(tiles, vdp, {"33333333", "55555555", "11111111", "11111111", "22222222", "22222222",
                                      "11111111", "33333333"});
    int pillar = makeTile(tiles, vdp, {"52222225", "51111115", "52222225", "51111115", "52222225", "51111115",
                                       "52222225", "51111115"});
    int safety = makeTile(tiles, vdp, {"44444444", "11111111", "12111121", "11111111", "11131111", "11111111",
                                       "21111112", "11111111"});
    int floor = makeTile(tiles, vdp, {"11111111", "11211121", "11111111", "13111131", "11111111", "11121111",
                                      "11111111", "12111112"});
    int alt = makeTile(tiles, vdp, {"11111111", "11111111", "11121111", "11111111", "11111131", "11111111",
                                    "21111111", "11111111"});
    for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 64; x++) {
            int tile = brick;
            int pal = PAL_WALL;
            if (y <= 2) tile = truss;
            else if (y == 3) tile = rail;
            else if (y >= 5 && y <= 7 && (x % 9 == 4 || x % 9 == 5)) tile = window;
            else if (y == 9) tile = cornice;
            else if (y >= 10 && y <= 19) tile = (x % 16 == 0) ? pillar : dark;
            else if (y == 20 || y == 21) tile = skirt;
            else if (y == 22) {
                tile = safety;
                pal = PAL_FLOOR;
            } else if (y >= 23) {
                tile = ((x + y) % 5 == 0) ? alt : floor;
                pal = PAL_FLOOR;
            }
            vdp.B.set(x, y, gs::entry(tile, pal));
            vdp.A.set(x, y, 0);
        }
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(15, 15, 14);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(9, 9, 8), gs::rgb4(6, 6, 6)});
    setPal(vdp, PAL_WALL,
           {0, gs::rgb4(6, 2, 1), gs::rgb4(9, 3, 2), gs::rgb4(11, 5, 3), gs::rgb4(5, 4, 3), gs::rgb4(3, 3, 4),
            gs::rgb4(5, 6, 7), gs::rgb4(13, 14, 12), gs::rgb4(4, 3, 2), gs::rgb4(8, 7, 6)});
    setPal(vdp, PAL_FLOOR,
           {0, gs::rgb4(6, 6, 5), gs::rgb4(8, 8, 7), gs::rgb4(4, 4, 3), gs::rgb4(13, 11, 3), gs::rgb4(3, 3, 2)});
    setPal(vdp, PAL_POUCH,
           {0, gs::rgb4(13, 9, 4), gs::rgb4(10, 6, 2), gs::rgb4(6, 3, 1), gs::rgb4(4, 2, 1), gs::rgb4(13, 10, 3),
            gs::rgb4(15, 14, 8)});
    setPal(vdp, PAL_PLAYER,
           {0, gs::rgb4(8, 10, 13), gs::rgb4(4, 6, 10), gs::rgb4(2, 3, 6), gs::rgb4(13, 9, 6), gs::rgb4(1, 1, 2),
            gs::rgb4(3, 3, 4), gs::rgb4(2, 1, 1), gs::rgb4(13, 11, 3), gs::rgb4(8, 4, 3)});
    setPal(vdp, PAL_STEEL,
           {0, gs::rgb4(12, 12, 11), gs::rgb4(7, 7, 8), gs::rgb4(3, 3, 4), gs::rgb4(14, 12, 3), gs::rgb4(10, 8, 2),
            gs::rgb4(1, 1, 1), gs::rgb4(8, 4, 2)});
    setPal(vdp, PAL_WOOD,
           {0, gs::rgb4(12, 8, 4), gs::rgb4(8, 5, 2), gs::rgb4(5, 3, 1), gs::rgb4(3, 2, 1), gs::rgb4(14, 13, 10),
            gs::rgb4(4, 5, 6)});
    setPal(vdp, PAL_FX,
           {0, gs::rgb4(15, 12, 4), gs::rgb4(15, 14, 8), gs::rgb4(15, 9, 2), gs::rgb4(13, 5, 1), gs::rgb4(15, 15, 13)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(9, 2, 1), gs::rgb4(15, 14, 12)});
    setPal(vdp, PAL_PIT,
           {0, gs::rgb4(2, 2, 3), gs::rgb4(1, 1, 2), gs::rgb4(8, 8, 9), gs::rgb4(12, 12, 13), gs::rgb4(5, 3, 2)});
    setPal(vdp, PAL_DOOR,
           {0, gs::rgb4(15, 6, 4), gs::rgb4(12, 2, 2), gs::rgb4(7, 1, 1), gs::rgb4(14, 8, 5), gs::rgb4(10, 10, 11),
            gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_GO, {0, gs::rgb4(8, 15, 6), gs::rgb4(2, 8, 3), gs::rgb4(15, 15, 14)});

    gs::TileAlloc tiles(vdp);
    paintHall(vdp, tiles);
    loadFont(vdp, art, tiles);

    art.stand = gs::uploadMipped(vdp, porter(0));
    art.runA = gs::uploadMipped(vdp, porter(1));
    art.runB = gs::uploadMipped(vdp, porter(2));
    art.duck = gs::uploadMipped(vdp, porter(3));
    art.leap = gs::uploadMipped(vdp, porter(4));
    art.pouch[0] = gs::uploadMipped(vdp, pouchArt(0));
    art.pouch[1] = gs::uploadMipped(vdp, pouchArt(1));
    art.desk = gs::uploadMipped(vdp, deskArt());
    art.crate = gs::uploadMipped(vdp, crateArt());
    art.barrel = gs::uploadMipped(vdp, barrelArt());
    art.bay = gs::uploadMipped(vdp, bayArt());
    art.mat = gs::uploadMipped(vdp, matArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.flame[0] = gs::uploadMipped(vdp, flameArt(0));
    art.flame[1] = gs::uploadMipped(vdp, flameArt(1));
    art.hole = gs::uploadMipped(vdp, holeArt());
    art.lip = gs::uploadMipped(vdp, lipArt());
    art.deck = gs::uploadMipped(vdp, deckArt());
    art.door = gs::uploadMipped(vdp, doorArt());
    art.hook = gs::uploadMipped(vdp, hookArt());
    art.cable = gs::uploadMipped(vdp, cableArt());
    art.beam = gs::uploadMipped(vdp, beamArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.signal = gs::uploadMipped(vdp, signalArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());

    vdp.A.enabled = false;
    vdp.B.enabled = true;
    vdp.B.scroll(0, 0);
    vdp.hudEnabled = true;
    vdp.setFogColor(gs::rgb4(2, 1, 1));
}

}  // namespace pouc

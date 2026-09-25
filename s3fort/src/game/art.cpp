#include "game/art.h"

#include <cstring>
#include <initializer_list>

namespace fort {
namespace {

using gs::Bitmap;

void pal(gs::VDP& vdp, int p, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(p * 16 + i, c);
        i++;
    }
    while (i < 16) vdp.setColor(p * 16 + i++, 0);
}

int solid(gs::VDP& vdp, gs::TileAlloc& tiles, int color) {
    uint8_t px[64];
    std::memset(px, uint8_t(color), sizeof px);
    int t = tiles.alloc(1);
    vdp.loadTile(t, px);
    return t;
}

void loadFont(gs::VDP& vdp, Art& a, gs::TileAlloc& tiles) {
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
    a.pipOk = solid(vdp, tiles, 6);
    a.pipMid = solid(vdp, tiles, 1);
    a.pipLow = solid(vdp, tiles, 4);
    a.pipOff = solid(vdp, tiles, 5);
}

// Index 1 cloak, 2 fold, 3 steel, 4 steel dark, 5 skin, 6 gold, 7 leather, 8 string, 9 outline.
Bitmap wardenArt(int pose) {
    Bitmap b(52, 78);
    b.poly({{26, 24}, {44, 36}, {40, 64}, {12, 64}, {8, 36}}, 1);
    b.poly({{26, 28}, {36, 38}, {34, 58}, {20, 58}, {16, 40}}, 2);
    b.ellipse(26, 16, 10, 11, 3);
    b.rect(16, 14, 20, 5, 4);
    b.rect(24, 6, 4, 10, 6);
    b.rect(25, 18, 2, 7, 4);
    b.rect(21, 20, 10, 7, 5);
    b.rect(22, 22, 2, 2, 4);
    b.rect(29, 22, 2, 2, 4);
    b.rect(18, 62, 7, 10, 7);
    b.rect(28, 62, 7, 10, 7);
    b.ellipse(21, 73, 6, 3, 4);
    b.ellipse(32, 73, 6, 3, 4);
    if (pose == 1) {
        b.poly({{28, 34}, {46, 16}, {48, 20}, {32, 40}}, 7);
        b.rect(44, 12, 3, 12, 3);
        b.line(30, 36, 46, 18, 8, 1);
    } else if (pose == 2) {
        b.ellipse(40, 40, 11, 15, 3);
        b.ellipse(40, 40, 6, 8, 6);
        b.rect(8, 32, 5, 20, 7);
        b.rect(6, 30, 9, 4, 4);
    } else {
        b.rect(34, 38, 14, 4, 7);
        b.rect(45, 34, 3, 12, 3);
        b.line(36, 40, 46, 36, 8, 1);
    }
    b.outline(9, false);
    return b;
}

// 1 tunic, 2 shade, 3 skin, 4 helm, 5 shaft, 6 head, 7 boot, 8 outline.
Bitmap raiderArt(int step) {
    Bitmap b(40, 68);
    int s = step ? 3 : 0;
    b.ellipse(20, 12, 8, 8, 4);
    b.rect(14, 8, 12, 4, 4);
    b.rect(16, 14, 8, 6, 3);
    b.rect(17, 16, 2, 2, 8);
    b.rect(22, 16, 2, 2, 8);
    b.poly({{12, 22}, {30, 22}, {32, 46}, {10, 46}}, 1);
    b.poly({{18, 24}, {26, 24}, {25, 42}, {16, 42}}, 2);
    b.rect(8, 24, 5, 16, 3);
    b.rect(28, 24, 5, 14, 3);
    b.poly({{30, 20}, {36, 8}, {38, 10}, {32, 24}}, 5);
    b.poly({{34, 6}, {39, 5}, {37, 11}}, 6);
    b.rect(13 - s, 46, 6, 14, 2);
    b.rect(22 + s, 46, 6, 14, 2);
    b.ellipse(15 - s, 61, 5, 3, 7);
    b.ellipse(26 + s, 61, 5, 3, 7);
    b.outline(8, false);
    return b;
}

// 1 shield, 2 rim, 3 boss, 4 tunic, 5 skin, 6 helm, 7 emblem, 8 outline.
Bitmap shieldArt(int step) {
    Bitmap b(44, 68);
    int s = step ? 2 : 0;
    b.ellipse(16, 36, 12, 18, 1);
    b.ellipse(16, 36, 9, 14, 2);
    b.ellipse(16, 36, 4, 5, 3);
    b.rect(14, 22, 4, 16, 7);
    b.ellipse(30, 14, 7, 7, 6);
    b.rect(26, 18, 8, 6, 5);
    b.rect(27, 28, 8, 16, 4);
    b.rect(24, 46, 5, 12, 4);
    b.rect(31, 46 + s, 5, 12, 4);
    b.ellipse(26, 60, 4, 3, 8);
    b.ellipse(34, 60 + s, 4, 3, 8);
    b.outline(8, false);
    return b;
}

// 1 wood, 2 grain, 3 iron, 4 iron dark, 5 wheel, 6 cloth, 7 skin, 8 glint, 9 outline.
Bitmap ramArt(int step) {
    Bitmap b(64, 74);
    int w = step ? 2 : 0;
    b.rect(28, 12, 8, 26, 1);
    b.rect(30, 14, 3, 22, 2);
    b.ellipse(32, 44, 16, 15, 3);
    b.ellipse(32, 44, 10, 10, 4);
    b.ellipse(26, 39, 4, 3, 8);
    b.ellipse(12, 60 + w, 8, 8, 5);
    b.ellipse(52, 58 - w, 8, 8, 5);
    b.ellipse(12, 60 + w, 3, 3, 4);
    b.ellipse(52, 58 - w, 3, 3, 4);
    b.rect(6, 28, 8, 18, 6);
    b.rect(50, 28, 8, 18, 6);
    b.ellipse(10, 22, 5, 5, 7);
    b.ellipse(54, 22, 5, 5, 7);
    b.rect(8, 20, 4, 3, 4);
    b.rect(52, 20, 4, 3, 4);
    b.outline(9, false);
    return b;
}

Bitmap boltArt() {
    Bitmap b(10, 22);
    b.poly({{5, 1}, {8, 8}, {2, 8}}, 2);
    b.rect(4, 7, 2, 10, 1);
    b.poly({{2, 16}, {8, 16}, {5, 21}}, 3);
    b.outline(4, false);
    return b;
}

Bitmap towerArt() {
    Bitmap b(46, 118);
    b.rect(8, 22, 30, 92, 1);
    b.rect(4, 12, 12, 16, 1);
    b.rect(17, 6, 12, 22, 1);
    b.rect(30, 12, 12, 16, 1);
    b.rect(6, 20, 34, 4, 2);
    b.rect(14, 40, 6, 22, 2);
    b.rect(16, 42, 2, 18, 9);
    b.rect(10, 70, 26, 3, 3);
    b.rect(10, 92, 26, 3, 3);
    b.rect(28, 78, 8, 16, 8);
    b.rect(12, 104, 8, 6, 7);
    b.outline(9, false);
    return b;
}

Bitmap doorArt(bool broke) {
    Bitmap b(78, 100);
    b.rect(6, 28, 66, 68, 1);
    b.ellipse(39, 30, 32, 24, 1);
    b.rect(8, 24, 62, 6, 2);
    b.rect(16, 36, 46, 54, broke ? 5 : 4);
    if (!broke) {
        for (int i = 0; i < 5; i++) b.rect(20 + i * 8, 30, 3, 58, 6);
        b.rect(18, 46, 42, 3, 6);
        b.rect(18, 62, 42, 3, 6);
        b.ellipse(39, 22, 18, 8, 8);
    } else {
        b.poly({{20, 40}, {36, 36}, {34, 70}, {18, 78}}, 5);
        b.poly({{40, 42}, {58, 34}, {60, 64}, {44, 72}}, 2);
        b.rect(22, 34, 3, 30, 6);
        b.rect(48, 38, 3, 22, 6);
        b.line(30, 48, 50, 70, 6, 2);
        b.rect(18, 80, 40, 5, 5);
    }
    b.outline(9, false);
    return b;
}

Bitmap wallArt() {
    Bitmap b(120, 28);
    b.rect(0, 10, 120, 16, 1);
    for (int i = 0; i < 6; i++) b.rect(4 + i * 20, 2, 12, 12, 1);
    b.rect(0, 18, 120, 4, 2);
    for (int i = 0; i < 4; i++) b.rect(16 + i * 28, 14, 3, 8, 2);
    b.outline(9, false);
    return b;
}

Bitmap bannerArt() {
    Bitmap b(26, 44);
    b.rect(12, 0, 3, 8, 4);
    b.poly({{4, 8}, {22, 8}, {20, 16}, {6, 16}}, 1);
    b.rect(5, 14, 16, 22, 1);
    b.poly({{5, 36}, {13, 30}, {21, 36}, {17, 42}, {9, 42}}, 2);
    b.rect(11, 18, 4, 10, 3);
    b.rect(9, 21, 8, 3, 3);
    b.outline(5, false);
    return b;
}

Bitmap torchArt(int frame) {
    Bitmap b(16, 28);
    b.rect(7, 14, 3, 12, 5);
    b.rect(5, 24, 7, 3, 4);
    if (frame == 0) {
        b.ellipse(8, 10, 5, 7, 2);
        b.ellipse(8, 9, 3, 4, 1);
    } else {
        b.ellipse(9, 9, 6, 6, 3);
        b.ellipse(8, 8, 3, 4, 1);
        b.rect(4, 4, 2, 3, 2);
    }
    return b;
}

Bitmap moonArt() {
    Bitmap b(34, 34);
    b.ellipse(16, 16, 13, 13, 3);
    b.ellipse(17, 17, 11, 11, 1);
    b.ellipse(12, 13, 3, 2, 2);
    b.ellipse(20, 18, 2, 2, 2);
    b.ellipse(15, 22, 2, 1, 2);
    return b;
}

Bitmap starArt() {
    Bitmap b(7, 7);
    b.rect(3, 0, 1, 7, 1);
    b.rect(0, 3, 7, 1, 1);
    b.rect(3, 3, 1, 1, 3);
    return b;
}

Bitmap puffArt() {
    Bitmap b(24, 18);
    b.ellipse(12, 10, 10, 6, 1);
    b.ellipse(8, 8, 4, 3, 2);
    return b;
}

Bitmap sparkArt() {
    Bitmap b(12, 12);
    b.rect(5, 1, 2, 10, 1);
    b.rect(1, 5, 10, 2, 2);
    b.rect(5, 5, 2, 2, 3);
    return b;
}

Bitmap shadowArt() {
    Bitmap b(32, 12);
    b.ellipse(16, 6, 14, 4, 1);
    return b;
}

Bitmap braceArt() {
    Bitmap b(48, 28);
    b.ellipse(24, 16, 20, 8, 2);
    b.ellipse(24, 16, 12, 4, 1);
    return b;
}

// Drawn with PAL_GOLD: 1 rim, 2 face, 3 hand, 4 hub.
Bitmap clockArt(int q) {
    Bitmap b(40, 40);
    b.ellipse(20, 20, 16, 16, 1);
    b.ellipse(20, 20, 13, 13, 2);
    b.rect(19, 6, 2, 3, 1);
    b.rect(19, 31, 2, 3, 1);
    b.rect(6, 19, 3, 2, 1);
    b.rect(31, 19, 3, 2, 1);
    b.ellipse(20, 20, 2, 2, 4);
    if (q == 0) b.rect(19, 10, 2, 10, 3);
    else if (q == 1) b.rect(20, 19, 10, 2, 3);
    else if (q == 2) b.rect(19, 20, 2, 10, 3);
    else b.rect(10, 19, 10, 2, 3);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    pal(vdp, PAL_HUD,
        {0, gs::rgb4(15, 15, 15), gs::rgb4(9, 9, 11), gs::rgb4(15, 13, 6), gs::rgb4(15, 4, 4), gs::rgb4(3, 3, 5),
         gs::rgb4(6, 14, 7), gs::rgb4(12, 12, 14), 0, 0, 0, 0, 0, 0, 0, shadow});
    pal(vdp, PAL_GOLD,
        {0, gs::rgb4(15, 13, 4), gs::rgb4(2, 2, 4), gs::rgb4(15, 5, 3), gs::rgb4(8, 8, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0,
         0, shadow});
    pal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    pal(vdp, PAL_DIM, {0, gs::rgb4(6, 6, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    pal(vdp, PAL_WARDEN,
        {0, gs::rgb4(10, 2, 2), gs::rgb4(6, 1, 1), gs::rgb4(12, 13, 14), gs::rgb4(6, 7, 8), gs::rgb4(13, 9, 6),
         gs::rgb4(14, 11, 4), gs::rgb4(5, 4, 2), gs::rgb4(3, 3, 4), gs::rgb4(1, 1, 2)});
    pal(vdp, PAL_RAIDER,
        {0, gs::rgb4(5, 6, 3), gs::rgb4(3, 4, 2), gs::rgb4(12, 8, 5), gs::rgb4(7, 4, 2), gs::rgb4(6, 4, 2),
         gs::rgb4(13, 13, 14), gs::rgb4(3, 2, 2), gs::rgb4(1, 1, 1)});
    pal(vdp, PAL_SHIELD,
        {0, gs::rgb4(8, 6, 3), gs::rgb4(4, 3, 1), gs::rgb4(11, 11, 12), gs::rgb4(4, 5, 3), gs::rgb4(12, 8, 5),
         gs::rgb4(6, 6, 7), gs::rgb4(8, 2, 2), gs::rgb4(1, 1, 1)});
    pal(vdp, PAL_RAM,
        {0, gs::rgb4(6, 4, 2), gs::rgb4(4, 3, 1), gs::rgb4(9, 9, 10), gs::rgb4(5, 5, 6), gs::rgb4(2, 2, 2),
         gs::rgb4(5, 3, 2), gs::rgb4(12, 8, 5), gs::rgb4(14, 14, 15), gs::rgb4(1, 1, 1)});
    pal(vdp, PAL_GATE,
        {0, gs::rgb4(7, 7, 8), gs::rgb4(4, 4, 5), gs::rgb4(5, 5, 6), gs::rgb4(6, 4, 2), gs::rgb4(3, 2, 1),
         gs::rgb4(10, 10, 11), gs::rgb4(3, 5, 3), gs::rgb4(10, 6, 3), gs::rgb4(1, 1, 2)});
    pal(vdp, PAL_FIRE,
        {0, gs::rgb4(15, 14, 5), gs::rgb4(15, 8, 1), gs::rgb4(12, 3, 1), gs::rgb4(5, 4, 3), gs::rgb4(4, 3, 2),
         gs::rgb4(2, 1, 1)});
    pal(vdp, PAL_BANNER,
        {0, gs::rgb4(8, 1, 2), gs::rgb4(4, 0, 1), gs::rgb4(14, 12, 4), gs::rgb4(5, 4, 3), gs::rgb4(1, 1, 2)});
    pal(vdp, PAL_MOON, {0, gs::rgb4(14, 14, 11), gs::rgb4(9, 9, 8), gs::rgb4(6, 6, 9), gs::rgb4(15, 15, 13)});
    pal(vdp, PAL_BOLT, {0, gs::rgb4(8, 6, 3), gs::rgb4(14, 14, 15), gs::rgb4(10, 2, 2), gs::rgb4(2, 2, 3)});
    pal(vdp, PAL_DUST, {0, gs::rgb4(7, 6, 5), gs::rgb4(12, 11, 8), gs::rgb4(15, 13, 6)});

    const uint16_t road[16] = {
        0,
        gs::rgb4(2, 3, 2), gs::rgb4(1, 2, 1), gs::rgb4(3, 4, 2),
        gs::rgb4(5, 4, 3), gs::rgb4(3, 3, 2),
        gs::rgb4(6, 5, 3), gs::rgb4(4, 3, 2),
        gs::rgb4(7, 6, 4), gs::rgb4(3, 3, 5),
        gs::rgb4(5, 4, 3), gs::rgb4(2, 3, 5),
        gs::rgb4(3, 4, 6), gs::rgb4(6, 7, 8),
        gs::rgb4(8, 7, 5), gs::rgb4(7, 6, 4),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_ROAD * 16 + i, road[i]);
    vdp.setFogColor(gs::rgb4(2, 2, 4));

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, art, tiles);
    for (int i = 0; i < 3; i++) art.warden[i] = gs::uploadMipped(vdp, wardenArt(i));
    for (int i = 0; i < 2; i++) {
        art.raider[i] = gs::uploadMipped(vdp, raiderArt(i));
        art.shield[i] = gs::uploadMipped(vdp, shieldArt(i));
        art.ram[i] = gs::uploadMipped(vdp, ramArt(i));
        art.torch[i] = gs::uploadMipped(vdp, torchArt(i));
    }
    art.bolt = gs::uploadMipped(vdp, boltArt());
    art.tower = gs::uploadMipped(vdp, towerArt());
    art.door = gs::uploadMipped(vdp, doorArt(false));
    art.doorBroke = gs::uploadMipped(vdp, doorArt(true));
    art.wall = gs::uploadMipped(vdp, wallArt());
    art.banner = gs::uploadMipped(vdp, bannerArt());
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.star = gs::uploadMipped(vdp, starArt());
    art.puff = gs::uploadMipped(vdp, puffArt());
    art.spark = gs::uploadMipped(vdp, sparkArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.brace = gs::uploadMipped(vdp, braceArt());
    for (int i = 0; i < 4; i++) art.clock[i] = gs::uploadMipped(vdp, clockArt(i));
}

}  // namespace fort

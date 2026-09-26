#include "game/art.h"

#include <cmath>
#include <initializer_list>
#include <string>

namespace ridgepurs {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

// Indices shared by the machines, seen from behind:
// 1 highlight, 2 body, 3 shade, 4 iron, 5 lamp, 6 glass,
// 7 stripe, 8 outline, 9 tyre, 10 hub, 11 stack, 12 glint.
Bitmap youArt() {
    Bitmap b(48, 56);
    b.poly({{4, 22}, {44, 22}, {46, 40}, {2, 40}}, 2);
    b.poly({{10, 24}, {38, 24}, {36, 34}, {12, 34}}, 1);
    b.rect(14, 10, 20, 16, 3);
    b.rect(16, 12, 16, 8, 6);
    b.rect(18, 13, 5, 3, 12);
    b.rect(12, 6, 3, 16, 4);
    b.rect(33, 6, 3, 16, 4);
    b.rect(12, 4, 24, 4, 4);
    b.rect(14, 5, 20, 2, 1);
    b.ellipse(24, 30, 6, 6, 9);
    b.ellipse(24, 30, 2, 2, 10);
    b.rect(6, 38, 36, 5, 4);
    b.rect(8, 39, 32, 2, 7);
    b.ellipse(10, 46, 6, 6, 9);
    b.ellipse(38, 46, 6, 6, 9);
    b.ellipse(10, 46, 2, 2, 10);
    b.ellipse(38, 46, 2, 2, 10);
    b.rect(8, 34, 6, 4, 5);
    b.rect(34, 34, 6, 4, 5);
    b.rect(8, 16, 3, 10, 11);
    b.rect(37, 16, 3, 10, 11);
    b.outline(8, false);
    return b;
}

Bitmap crawlerArt() {
    Bitmap b(56, 40);
    b.rect(2, 22, 12, 14, 9);
    b.rect(42, 22, 12, 14, 9);
    b.ellipse(8, 24, 5, 5, 9);
    b.ellipse(8, 34, 5, 5, 9);
    b.ellipse(48, 24, 5, 5, 9);
    b.ellipse(48, 34, 5, 5, 9);
    b.rect(5, 27, 6, 6, 10);
    b.rect(45, 27, 6, 6, 10);
    b.poly({{10, 16}, {46, 16}, {50, 30}, {6, 30}}, 2);
    b.poly({{16, 18}, {40, 18}, {42, 26}, {14, 26}}, 1);
    b.rect(18, 6, 20, 12, 3);
    b.rect(20, 8, 16, 6, 6);
    b.rect(16, 4, 24, 3, 4);
    b.rect(18, 4, 4, 3, 5);
    b.rect(34, 4, 4, 3, 5);
    b.rect(26, 30, 4, 4, 11);
    b.outline(8, false);
    return b;
}

Bitmap drayArt() {
    Bitmap b(44, 64);
    b.poly({{8, 18}, {22, 4}, {36, 18}}, 1);
    b.rect(6, 18, 32, 26, 2);
    b.rect(8, 20, 28, 6, 1);
    b.rect(10, 28, 24, 2, 3);
    b.rect(10, 34, 24, 2, 3);
    b.rect(12, 42, 20, 8, 4);
    b.rect(14, 44, 16, 3, 7);
    b.ellipse(8, 56, 6, 6, 9);
    b.ellipse(36, 56, 6, 6, 9);
    b.ellipse(8, 56, 2, 2, 10);
    b.ellipse(36, 56, 2, 2, 10);
    b.rect(10, 40, 5, 3, 5);
    b.rect(29, 40, 5, 3, 5);
    b.rect(21, 8, 2, 10, 4);
    b.outline(8, false);
    return b;
}

Bitmap haulerArt() {
    Bitmap b(64, 48);
    b.rect(8, 8, 4, 20, 4);
    b.rect(52, 8, 4, 20, 4);
    b.rect(8, 6, 48, 4, 3);
    b.rect(14, 14, 36, 16, 2);
    b.rect(16, 16, 32, 4, 1);
    b.rect(18, 22, 28, 2, 7);
    b.rect(6, 30, 52, 6, 4);
    b.rect(8, 31, 48, 2, 3);
    b.ellipse(12, 40, 6, 6, 9);
    b.ellipse(18, 40, 6, 6, 9);
    b.ellipse(46, 40, 6, 6, 9);
    b.ellipse(52, 40, 6, 6, 9);
    b.ellipse(12, 40, 2, 2, 10);
    b.ellipse(18, 40, 2, 2, 10);
    b.ellipse(46, 40, 2, 2, 10);
    b.ellipse(52, 40, 2, 2, 10);
    b.rect(10, 0, 4, 14, 11);
    b.rect(10, 0, 4, 3, 5);
    b.rect(20, 28, 6, 3, 5);
    b.rect(38, 28, 6, 3, 5);
    b.outline(8, false);
    return b;
}

Bitmap cairnArt() {
    Bitmap b(36, 44);
    b.ellipse(18, 36, 16, 6, 3);
    b.ellipse(18, 28, 12, 7, 2);
    b.ellipse(18, 20, 8, 6, 1);
    b.ellipse(18, 13, 5, 4, 6);
    b.outline(5, false);
    return b;
}

Bitmap postArt() {
    Bitmap b(30, 52);
    b.rect(13, 10, 4, 38, 2);
    b.rect(11, 48, 8, 3, 3);
    b.rect(12, 8, 6, 4, 4);
    b.poly({{17, 10}, {28, 14}, {17, 20}}, 7);
    b.outline(5, false);
    return b;
}

Bitmap peakLArt() {
    Bitmap b(100, 64);
    b.poly({{0, 63}, {0, 34}, {16, 38}, {34, 18}, {52, 32}, {70, 8}, {100, 28}, {100, 63}}, 2);
    b.poly({{0, 63}, {12, 44}, {36, 30}, {64, 24}, {100, 40}, {100, 63}}, 3);
    b.poly({{62, 16}, {70, 8}, {80, 20}}, 1);
    return b;
}

Bitmap peakRArt() {
    Bitmap b(110, 56);
    b.poly({{0, 55}, {0, 30}, {24, 16}, {42, 32}, {66, 10}, {88, 26}, {110, 18}, {110, 55}}, 2);
    b.poly({{0, 55}, {18, 36}, {50, 26}, {84, 30}, {110, 24}, {110, 55}}, 3);
    b.poly({{58, 18}, {66, 10}, {76, 20}}, 1);
    return b;
}

Bitmap cloudArt() {
    Bitmap b(72, 28);
    b.ellipse(20, 16, 16, 8, 2);
    b.ellipse(40, 14, 18, 10, 1);
    b.ellipse(56, 16, 12, 7, 3);
    return b;
}

Bitmap sunArt() {
    Bitmap b(22, 22);
    b.ellipse(11, 11, 9, 9, 1);
    b.ellipse(11, 11, 5, 5, 2);
    return b;
}

Bitmap moonArt() {
    Bitmap b(20, 20);
    b.ellipse(10, 10, 8, 8, 1);
    b.ellipse(8, 8, 3, 3, 3);
    return b;
}

Bitmap shotArt() {
    Bitmap b(6, 16);
    b.rect(2, 0, 2, 16, 1);
    b.rect(2, 8, 2, 8, 2);
    return b;
}

Bitmap boltArt() {
    Bitmap b(8, 18);
    b.rect(3, 0, 2, 18, 2);
    b.rect(3, 0, 2, 7, 1);
    b.set(2, 5, 3);
    b.set(5, 5, 3);
    return b;
}

Bitmap puffArt() {
    Bitmap b(28, 28);
    b.ellipse(14, 14, 11, 8, 2);
    b.ellipse(13, 13, 6, 4, 1);
    for (int y = 0; y < 28; y++)
        for (int x = 0; x < 28; x++)
            if (b.get(x, y) && ((x * 5 + y * 3) & 3) == 0 && std::hypot(x - 14.0, y - 14.0) > 6) b.set(x, y, 0);
    return b;
}

Bitmap sparkArt() {
    Bitmap b(16, 16);
    b.poly({{8, 0}, {10, 6}, {15, 8}, {10, 10}, {8, 15}, {6, 10}, {1, 8}, {6, 6}}, 1);
    b.ellipse(8, 8, 2, 2, 2);
    return b;
}

Bitmap flameArt(int frame) {
    Bitmap b(16, 20);
    if (frame == 0) {
        b.poly({{8, 1}, {13, 10}, {11, 18}, {5, 18}, {3, 10}}, 4);
        b.poly({{8, 5}, {11, 12}, {8, 17}, {5, 12}}, 6);
        b.ellipse(8, 14, 2, 3, 1);
    } else {
        b.poly({{8, 3}, {14, 12}, {10, 19}, {4, 18}, {2, 9}}, 5);
        b.poly({{8, 7}, {11, 13}, {8, 18}, {5, 12}}, 6);
        b.ellipse(8, 15, 2, 2, 1);
    }
    return b;
}

Bitmap shadowArt() {
    Bitmap b(64, 16);
    b.ellipse(32, 8, 28, 5, 1);
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
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shade = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_INK, {0, gs::rgb4(15, 15, 14), gs::rgb4(11, 12, 13), gs::rgb4(6, 7, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shade});
    setPal(vdp, PAL_GOLD,
           {0, gs::rgb4(15, 12, 5), gs::rgb4(12, 8, 3), gs::rgb4(8, 5, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_ALERT,
           {0, gs::rgb4(15, 5, 4), gs::rgb4(9, 2, 2), gs::rgb4(5, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 0, 0)});
    setPal(vdp, PAL_GOOD,
           {0, gs::rgb4(8, 15, 7), gs::rgb4(3, 9, 4), gs::rgb4(1, 5, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(0, 3, 1)});

    setPal(vdp, PAL_YOU,
           {0, gs::rgb4(12, 14, 15), gs::rgb4(4, 6, 8), gs::rgb4(2, 3, 5), gs::rgb4(7, 7, 8), gs::rgb4(15, 12, 4),
            gs::rgb4(8, 12, 14), gs::rgb4(14, 8, 2), shade, gs::rgb4(1, 1, 1), gs::rgb4(8, 8, 8), gs::rgb4(5, 5, 6),
            gs::rgb4(15, 15, 13), 0, 0, shade});
    setPal(vdp, PAL_CRAWL,
           {0, gs::rgb4(14, 10, 6), gs::rgb4(10, 4, 2), gs::rgb4(6, 2, 1), gs::rgb4(5, 4, 4), gs::rgb4(15, 12, 5),
            gs::rgb4(6, 8, 10), gs::rgb4(12, 8, 3), shade, gs::rgb4(2, 1, 1), gs::rgb4(7, 6, 5), gs::rgb4(4, 3, 3),
            gs::rgb4(15, 14, 10), 0, 0, shade});
    setPal(vdp, PAL_DRAY,
           {0, gs::rgb4(14, 12, 8), gs::rgb4(10, 8, 5), gs::rgb4(6, 5, 3), gs::rgb4(5, 4, 3), gs::rgb4(15, 10, 4),
            gs::rgb4(5, 7, 9), gs::rgb4(8, 3, 2), shade, gs::rgb4(2, 2, 2), gs::rgb4(8, 7, 6), gs::rgb4(6, 5, 4),
            gs::rgb4(15, 14, 11), 0, 0, shade});
    setPal(vdp, PAL_HAUL,
           {0, gs::rgb4(10, 12, 8), gs::rgb4(4, 6, 3), gs::rgb4(2, 3, 2), gs::rgb4(5, 5, 4), gs::rgb4(15, 13, 6),
            gs::rgb4(7, 10, 12), gs::rgb4(12, 10, 4), shade, gs::rgb4(1, 1, 1), gs::rgb4(7, 7, 6), gs::rgb4(4, 4, 3),
            gs::rgb4(14, 15, 12), 0, 0, shade});
    setPal(vdp, PAL_STONE,
           {0, gs::rgb4(13, 12, 10), gs::rgb4(8, 7, 6), gs::rgb4(4, 4, 3), gs::rgb4(6, 5, 4), shade, gs::rgb4(12, 9, 6),
            gs::rgb4(12, 3, 2), 0, 0, 0, 0, 0, 0, 0, shade});
    setPal(vdp, PAL_FX,
           {0, gs::rgb4(15, 15, 13), gs::rgb4(12, 10, 8), gs::rgb4(7, 6, 5), gs::rgb4(15, 8, 2), gs::rgb4(15, 4, 1),
            gs::rgb4(15, 12, 4), gs::rgb4(8, 8, 8), shade, 0, 0, 0, 0, 0, 0, shade});
    setPal(vdp, PAL_PEAK,
           {0, gs::rgb4(13, 14, 15), gs::rgb4(5, 6, 8), gs::rgb4(3, 3, 5), gs::rgb4(8, 7, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            shade});
    setPal(vdp, PAL_BOLT,
           {0, gs::rgb4(15, 15, 12), gs::rgb4(15, 10, 3), gs::rgb4(12, 6, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shade});

    const uint16_t road[16] = {
        0,
        gs::rgb4(4, 5, 3),
        gs::rgb4(2, 3, 2),
        gs::rgb4(6, 6, 4),
        gs::rgb4(8, 7, 5),
        gs::rgb4(4, 4, 3),
        gs::rgb4(9, 8, 6),
        gs::rgb4(5, 4, 3),
        gs::rgb4(12, 11, 8),
        gs::rgb4(3, 3, 2),
        gs::rgb4(7, 6, 5),
        gs::rgb4(2, 3, 5),
        gs::rgb4(3, 4, 6),
        gs::rgb4(5, 6, 8),
        gs::rgb4(13, 12, 9),
        gs::rgb4(14, 13, 10),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_ROAD * 16 + i, road[i]);

    art.you = gs::uploadMipped(vdp, youArt());
    art.crawler = gs::uploadMipped(vdp, crawlerArt());
    art.dray = gs::uploadMipped(vdp, drayArt());
    art.hauler = gs::uploadMipped(vdp, haulerArt());
    art.cairn = gs::uploadMipped(vdp, cairnArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.peakL = gs::uploadMipped(vdp, peakLArt());
    art.peakR = gs::uploadMipped(vdp, peakRArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.shot = gs::uploadMipped(vdp, shotArt());
    art.bolt = gs::uploadMipped(vdp, boltArt());
    art.puff = gs::uploadMipped(vdp, puffArt());
    art.spark = gs::uploadMipped(vdp, sparkArt());
    art.flame[0] = gs::uploadMipped(vdp, flameArt(0));
    art.flame[1] = gs::uploadMipped(vdp, flameArt(1));
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    loadFont(vdp, art);
}

}  // namespace ridgepurs

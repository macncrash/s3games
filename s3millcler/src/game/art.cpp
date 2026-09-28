#include "game/art.h"

#include <cstdint>
#include <vector>

namespace mill {
namespace {

void setPal(gs::VDP& vdp, int pal, const std::vector<uint16_t>& cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

gs::Bitmap handArt(int fr) {
    gs::Bitmap b(28, 40);
    b.ellipse(14, 7, 5, 5, 4);
    b.rect(12, 2, 4, 3, 8);
    b.rect(10, 12, 8, 12, 1);
    b.rect(11, 14, 6, 6, 7);
    b.rect(8, 13, 3, 8, 2);
    b.rect(17, 13, 3, 8, 2);
    if (fr == 0) {
        b.rect(10, 24, 3, 12, 3);
        b.rect(15, 24, 3, 10, 3);
    } else {
        b.rect(9, 24, 3, 10, 3);
        b.rect(16, 24, 3, 12, 3);
    }
    b.rect(9, 34, 5, 3, 5);
    b.rect(15, 34, 5, 3, 5);
    b.outline(6, false);
    return b;
}

gs::Bitmap rakeArt() {
    gs::Bitmap b(36, 18);
    b.rect(2, 8, 28, 2, 1);
    for (int i = 0; i < 5; i++) b.rect(4 + i * 5, 2, 2, 7, 2);
    b.rect(28, 7, 6, 3, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap millArt() {
    gs::Bitmap b(56, 88);
    b.poly({{4, 86}, {52, 86}, {46, 28}, {28, 8}, {10, 28}}, 1);
    b.poly({{16, 30}, {28, 14}, {40, 30}, {36, 36}, {20, 36}}, 2);
    b.rect(22, 40, 12, 18, 4);
    b.rect(24, 42, 8, 8, 5);
    b.rect(20, 62, 16, 24, 6);
    b.rect(24, 66, 8, 20, 7);
    b.rect(8, 48, 6, 10, 3);
    b.rect(42, 48, 6, 10, 3);
    b.outline(8, false);
    return b;
}

gs::Bitmap sailArt(int fr) {
    gs::Bitmap b(48, 48);
    b.rect(22, 22, 4, 4, 3);
    if (fr == 0) {
        b.poly({{24, 22}, {26, 4}, {32, 8}, {26, 22}}, 1);
        b.poly({{24, 26}, {22, 44}, {16, 40}, {22, 26}}, 1);
        b.poly({{22, 24}, {4, 20}, {6, 14}, {22, 22}}, 2);
        b.poly({{26, 24}, {44, 28}, {42, 34}, {26, 26}}, 2);
    } else {
        b.poly({{24, 22}, {18, 4}, {12, 8}, {22, 22}}, 1);
        b.poly({{24, 26}, {30, 44}, {36, 40}, {26, 26}}, 1);
        b.poly({{22, 24}, {6, 30}, {10, 36}, {22, 26}}, 2);
        b.poly({{26, 24}, {42, 16}, {38, 10}, {26, 22}}, 2);
    }
    return b;
}

gs::Bitmap doorArt() {
    gs::Bitmap b(22, 28);
    b.rect(2, 4, 18, 22, 1);
    b.rect(4, 6, 14, 18, 2);
    b.rect(16, 14, 2, 2, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap sackArt() {
    gs::Bitmap b(22, 18);
    b.ellipse(11, 10, 9, 6, 1);
    b.ellipse(11, 9, 6, 4, 2);
    b.rect(8, 3, 6, 3, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap stoneArt() {
    gs::Bitmap b(20, 14);
    b.poly({{2, 10}, {6, 3}, {14, 2}, {18, 8}, {12, 12}, {4, 12}}, 1);
    b.poly({{6, 6}, {12, 4}, {14, 8}, {8, 10}}, 2);
    b.outline(3, false);
    return b;
}

gs::Bitmap reedArt() {
    gs::Bitmap b(18, 22);
    for (int i = 0; i < 5; i++) b.line(4.f + i * 2.f, 18.f, 6.f + i * 1.5f, 3.f, i & 1 ? 1 : 2, 1.2f);
    b.rect(3, 16, 12, 4, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap chaffArt() {
    gs::Bitmap b(20, 12);
    b.ellipse(10, 7, 8, 3, 1);
    b.ellipse(6, 6, 3, 2, 2);
    b.ellipse(14, 6, 3, 2, 3);
    return b;
}

gs::Bitmap clockArt() {
    gs::Bitmap b(28, 28);
    b.ellipse(14, 14, 12, 12, 1);
    b.ellipse(14, 14, 9, 9, 2);
    b.rect(13, 6, 2, 8, 3);
    b.rect(13, 13, 7, 2, 4);
    b.rect(12, 2, 4, 3, 5);
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(12, 12);
    b.ellipse(6, 6, 5, 3, 1);
    b.ellipse(6, 5, 2, 2, 2);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(24, 8);
    b.ellipse(12, 4, 10, 3, 1);
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
    const uint16_t ink = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(15, 14, 12), gs::rgb4(10, 9, 8), gs::rgb4(3, 3, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, ink});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 4), gs::rgb4(15, 14, 9), gs::rgb4(6, 4, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(15, 10, 7), gs::rgb4(5, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, ink});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(8, 15, 6), gs::rgb4(14, 15, 11), gs::rgb4(1, 4, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, ink});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(11, 10, 8), gs::rgb4(7, 6, 5), gs::rgb4(4, 4, 3), gs::rgb4(14, 13, 11), 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FIGURE, {0, gs::rgb4(12, 8, 4), gs::rgb4(7, 5, 3), gs::rgb4(4, 3, 2), gs::rgb4(14, 11, 8),
                             gs::rgb4(2, 2, 2), gs::rgb4(3, 2, 1), gs::rgb4(15, 12, 6), gs::rgb4(5, 3, 1), 0, 0, 0, 0,
                             0, 0, ink});
    setPal(vdp, PAL_SACK, {0, gs::rgb4(13, 10, 4), gs::rgb4(15, 13, 7), gs::rgb4(8, 6, 2), gs::rgb4(4, 3, 1), 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_REED, {0, gs::rgb4(6, 11, 3), gs::rgb4(10, 13, 5), gs::rgb4(8, 6, 2), gs::rgb4(3, 4, 1), 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FX, {0, gs::rgb4(8, 7, 5), gs::rgb4(14, 13, 10), gs::rgb4(4, 3, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         0, ink});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(14, 14, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(12, 8, 3), gs::rgb4(8, 5, 2), gs::rgb4(14, 11, 6), gs::rgb4(3, 2, 1), 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_MILL, {0, gs::rgb4(12, 11, 9), gs::rgb4(7, 5, 3), gs::rgb4(14, 8, 3), gs::rgb4(4, 6, 8),
                           gs::rgb4(9, 12, 14), gs::rgb4(5, 4, 3), gs::rgb4(2, 2, 2), gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0,
                           0, ink});
    setPal(vdp, PAL_CLOCK, {0, gs::rgb4(10, 8, 5), gs::rgb4(15, 14, 10), gs::rgb4(2, 2, 2), gs::rgb4(12, 3, 2),
                            gs::rgb4(6, 5, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});

    const uint16_t road[16] = {
        0,
        gs::rgb4(6, 8, 3),
        gs::rgb4(4, 6, 2),
        gs::rgb4(8, 9, 4),
        gs::rgb4(9, 8, 4),
        gs::rgb4(5, 5, 2),
        gs::rgb4(11, 9, 5),
        gs::rgb4(8, 6, 3),
        gs::rgb4(12, 10, 6),
        gs::rgb4(7, 6, 3),
        gs::rgb4(10, 8, 4),
        gs::rgb4(3, 6, 8),
        gs::rgb4(2, 5, 7),
        gs::rgb4(5, 8, 10),
        gs::rgb4(13, 11, 6),
        gs::rgb4(9, 7, 4),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_ROAD * 16 + i, road[i]);
    setPal(vdp, PAL_WATER, {0, gs::rgb4(5, 9, 12), gs::rgb4(3, 6, 9), gs::rgb4(8, 12, 13), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, ink});

    loadFont(vdp, art);
    art.hand[0] = gs::uploadMipped(vdp, handArt(0));
    art.hand[1] = gs::uploadMipped(vdp, handArt(1));
    art.rake = gs::uploadMipped(vdp, rakeArt());
    art.mill = gs::uploadMipped(vdp, millArt());
    art.sail[0] = gs::uploadMipped(vdp, sailArt(0));
    art.sail[1] = gs::uploadMipped(vdp, sailArt(1));
    art.door = gs::uploadMipped(vdp, doorArt());
    art.sack = gs::uploadMipped(vdp, sackArt());
    art.stone = gs::uploadMipped(vdp, stoneArt());
    art.reed = gs::uploadMipped(vdp, reedArt());
    art.chaff = gs::uploadMipped(vdp, chaffArt());
    art.clock = gs::uploadMipped(vdp, clockArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
}

}  // namespace mill

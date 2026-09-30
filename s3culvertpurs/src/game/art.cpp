#include "game/art.h"

namespace cpurs {
namespace {

void pal(gs::VDP& vdp, int bank, const uint16_t c[16]) {
    for (int i = 0; i < 16; i++) vdp.setColor(bank * 16 + i, c[i]);
}

gs::Bitmap youArt() {
    gs::Bitmap b(40, 28);
    b.rect(4, 10, 32, 12, 2);
    b.rect(8, 4, 18, 8, 3);
    b.rect(22, 6, 6, 4, 6);
    b.ellipse(10, 22, 5, 4, 1);
    b.ellipse(30, 22, 5, 4, 1);
    b.rect(6, 14, 28, 3, 5);
    b.rect(34, 12, 4, 4, 8);
    return b;
}

gs::Bitmap pumpArt() {
    gs::Bitmap b(36, 26);
    b.rect(2, 8, 28, 12, 2);
    b.rect(24, 2, 6, 10, 4);
    b.ellipse(8, 20, 4, 3, 1);
    b.ellipse(22, 20, 4, 3, 1);
    b.rect(6, 11, 10, 4, 7);
    b.rect(30, 10, 4, 3, 3);
    return b;
}

gs::Bitmap rollerArt() {
    gs::Bitmap b(44, 24);
    b.ellipse(22, 13, 16, 9, 3);
    b.ellipse(22, 13, 8, 5, 2);
    b.rect(4, 8, 8, 10, 4);
    b.rect(32, 8, 8, 10, 4);
    b.rect(18, 4, 8, 4, 6);
    return b;
}

gs::Bitmap drainArt() {
    gs::Bitmap b(34, 36);
    b.rect(6, 10, 22, 18, 2);
    b.ellipse(17, 8, 8, 6, 4);
    b.rect(10, 16, 14, 6, 3);
    b.rect(4, 22, 4, 8, 1);
    b.rect(26, 22, 4, 8, 1);
    b.rect(14, 26, 6, 4, 6);
    return b;
}

gs::Bitmap ribArt() {
    gs::Bitmap b(48, 40);
    b.rect(0, 0, 6, 40, 2);
    b.rect(42, 0, 6, 40, 2);
    b.rect(0, 0, 48, 6, 3);
    b.rect(6, 6, 36, 3, 4);
    for (int y = 10; y < 38; y += 8) {
        b.rect(1, y, 4, 2, 5);
        b.rect(43, y, 4, 2, 5);
    }
    return b;
}

gs::Bitmap grateArt() {
    gs::Bitmap b(28, 16);
    b.rect(0, 0, 28, 16, 2);
    for (int x = 3; x < 26; x += 5) b.rect(x, 2, 2, 12, 4);
    b.rect(0, 7, 28, 2, 3);
    return b;
}

gs::Bitmap puffArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 7, 6, 2);
    b.ellipse(6, 6, 3, 3, 3);
    return b;
}

gs::Bitmap sparkArt() {
    gs::Bitmap b(10, 10);
    b.line(1, 5, 9, 5, 2, 2);
    b.line(5, 1, 5, 9, 3, 2);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(20, 8);
    b.ellipse(10, 4, 9, 3, 1);
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

void buildArt(gs::VDP& vdp, Art& a) {
    const uint16_t hud[16] = {
        gs::rgb4(0, 0, 0), gs::rgb4(14, 15, 13), gs::rgb4(7, 8, 7), gs::rgb4(12, 10, 4),
        gs::rgb4(3, 3, 3), gs::rgb4(12, 4, 3), gs::rgb4(4, 10, 6), gs::rgb4(2, 2, 2),
        gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, gs::rgb4(1, 1, 1)};
    const uint16_t you[16] = {
        0, gs::rgb4(2, 2, 2), gs::rgb4(4, 6, 3), gs::rgb4(7, 9, 4), gs::rgb4(3, 4, 2),
        gs::rgb4(10, 12, 5), gs::rgb4(14, 13, 6), gs::rgb4(5, 6, 4), gs::rgb4(15, 8, 3),
        0, 0, 0, 0, 0, 0, 0};
    const uint16_t pump[16] = {
        0, gs::rgb4(2, 1, 1), gs::rgb4(8, 3, 2), gs::rgb4(12, 6, 3), gs::rgb4(5, 4, 3),
        gs::rgb4(3, 2, 2), gs::rgb4(14, 10, 4), gs::rgb4(10, 8, 5), 0, 0, 0, 0, 0, 0, 0, 0};
    const uint16_t roll[16] = {
        0, gs::rgb4(3, 2, 1), gs::rgb4(10, 7, 2), gs::rgb4(14, 11, 3), gs::rgb4(6, 5, 2),
        gs::rgb4(4, 3, 2), gs::rgb4(15, 14, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0};
    const uint16_t drain[16] = {
        0, gs::rgb4(2, 3, 4), gs::rgb4(4, 6, 8), gs::rgb4(7, 9, 11), gs::rgb4(3, 5, 6),
        gs::rgb4(5, 6, 6), gs::rgb4(12, 12, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0};
    const uint16_t stone[16] = {
        0, gs::rgb4(2, 2, 3), gs::rgb4(5, 5, 6), gs::rgb4(8, 8, 9), gs::rgb4(3, 3, 4),
        gs::rgb4(6, 6, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    const uint16_t spark[16] = {
        0, gs::rgb4(6, 6, 5), gs::rgb4(15, 14, 6), gs::rgb4(15, 8, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    const uint16_t water[16] = {
        0, gs::rgb4(1, 4, 6), gs::rgb4(2, 7, 8), gs::rgb4(5, 10, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    const uint16_t road[16] = {
        0, gs::rgb4(3, 4, 3), gs::rgb4(2, 3, 3), gs::rgb4(5, 5, 4), gs::rgb4(4, 4, 3), gs::rgb4(2, 3, 2),
        gs::rgb4(5, 5, 6), gs::rgb4(3, 3, 4), gs::rgb4(7, 7, 6), gs::rgb4(4, 4, 5), gs::rgb4(6, 6, 5),
        gs::rgb4(1, 3, 5), gs::rgb4(2, 5, 6), gs::rgb4(4, 7, 7), gs::rgb4(8, 8, 5), gs::rgb4(6, 6, 6)};
    pal(vdp, PAL_HUD, hud);
    pal(vdp, PAL_YOU, you);
    pal(vdp, PAL_PUMP, pump);
    pal(vdp, PAL_ROLL, roll);
    pal(vdp, PAL_DRAIN, drain);
    pal(vdp, PAL_STONE, stone);
    pal(vdp, PAL_SPARK, spark);
    pal(vdp, PAL_WATER, water);
    pal(vdp, PAL_ROAD, road);
    vdp.setFogColor(gs::rgb4(1, 2, 2));

    a.you = gs::uploadMipped(vdp, youArt());
    a.pump = gs::uploadMipped(vdp, pumpArt());
    a.roller = gs::uploadMipped(vdp, rollerArt());
    a.drainer = gs::uploadMipped(vdp, drainArt());
    a.rib = gs::uploadMipped(vdp, ribArt());
    a.grate = gs::uploadMipped(vdp, grateArt());
    a.puff = gs::uploadMipped(vdp, puffArt());
    a.spark = gs::uploadMipped(vdp, sparkArt());
    a.shadow = gs::uploadMipped(vdp, shadowArt());
    loadFont(vdp, a);
}

}  // namespace cpurs

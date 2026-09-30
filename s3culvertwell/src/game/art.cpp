#include "game/art.h"

namespace culvertwell {
namespace {

void pal(gs::VDP& vdp, int bank, const uint16_t* c) {
    for (int i = 0; i < 16; i++) vdp.setColor(bank * 16 + i, c[i]);
}

gs::Bitmap keeperArt(bool swing) {
    gs::Bitmap b(48, 56);
    b.ellipse(22, 10, 7, 7, 3);
    b.rect(18, 4, 8, 4, 4);
    b.rect(14, 16, 16, 18, 5);
    b.rect(16, 18, 6, 8, 8);
    b.rect(16, 34, 5, 16, 7);
    b.rect(24, 34, 5, 16, 7);
    b.rect(14, 48, 8, 4, 1);
    b.rect(22, 48, 8, 4, 1);
    b.rect(19, 10, 2, 2, 1);
    if (!swing) {
        b.rect(30, 20, 4, 22, 2);
        b.rect(26, 18, 12, 5, 6);
    } else {
        b.rect(28, 22, 16, 4, 2);
        b.rect(40, 16, 6, 14, 6);
    }
    return b;
}

gs::Bitmap sapperArt() {
    gs::Bitmap b(32, 48);
    b.ellipse(16, 8, 6, 6, 4);
    b.rect(10, 14, 12, 16, 3);
    b.rect(8, 18, 6, 3, 2);
    b.rect(18, 20, 10, 3, 6);
    b.rect(11, 30, 4, 14, 5);
    b.rect(18, 30, 4, 14, 5);
    b.rect(20, 8, 3, 2, 1);
    b.rect(22, 16, 8, 2, 8);
    return b;
}

gs::Bitmap barrelArt() {
    gs::Bitmap b(28, 32);
    b.ellipse(14, 16, 12, 12, 3);
    b.ellipse(14, 16, 7, 7, 5);
    b.rect(4, 8, 20, 3, 2);
    b.rect(4, 21, 20, 3, 2);
    b.rect(12, 6, 4, 20, 4);
    return b;
}

gs::Bitmap ramArt() {
    gs::Bitmap b(52, 36);
    b.rect(2, 14, 36, 10, 3);
    b.poly({{34, 10}, {50, 18}, {34, 28}}, 6);
    b.rect(6, 16, 8, 6, 2);
    b.rect(18, 16, 8, 6, 4);
    b.ellipse(10, 10, 5, 5, 5);
    b.ellipse(24, 10, 5, 5, 5);
    b.rect(8, 24, 4, 8, 7);
    b.rect(22, 24, 4, 8, 7);
    return b;
}

gs::Bitmap wellArt(int cracks) {
    gs::Bitmap b(56, 64);
    b.ellipse(28, 30, 22, 22, 3);
    b.ellipse(28, 30, 14, 14, 5);
    b.ellipse(28, 28, 8, 6, 8);
    b.rect(8, 18, 6, 8, 2);
    b.rect(40, 16, 7, 9, 4);
    b.rect(18, 8, 8, 6, 2);
    b.rect(30, 42, 8, 6, 4);
    b.rect(24, 50, 8, 10, 6);
    if (cracks >= 1) {
        b.line(16, 20, 26, 34, 1, 1);
        b.line(26, 34, 22, 46, 1, 1);
    }
    if (cracks >= 2) {
        b.line(40, 18, 30, 32, 1, 1);
        b.line(30, 32, 38, 48, 1, 1);
        b.rect(12, 36, 5, 4, 1);
    }
    return b;
}

gs::Bitmap archSide(bool right) {
    gs::Bitmap b(72, 200);
    b.rect(0, 0, 72, 200, 2);
    for (int y = 0; y < 200; y++) {
        int inset = 6 + (y * y) / 2200;
        if (!right) b.rect(72 - inset, y, inset, 1, 0);
        else b.rect(0, y, inset, 1, 0);
        if (y % 16 < 2) {
            int x0 = right ? inset : 0;
            int x1 = right ? 72 : 72 - inset;
            for (int x = x0; x < x1; x++)
                if (b.get(x, y)) b.set(x, y, 4);
        }
    }
    b.rect(right ? 8 : 48, 70, 14, 22, 6);
    return b;
}

gs::Bitmap lintelArt() {
    gs::Bitmap b(320, 28);
    b.rect(0, 0, 320, 28, 2);
    b.rect(0, 22, 320, 4, 4);
    for (int x = 12; x < 320; x += 32) b.rect(x, 6, 12, 6, 5);
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
        gs::rgb4(0, 0, 0), gs::rgb4(15, 15, 13), gs::rgb4(8, 8, 7), gs::rgb4(15, 12, 4),
        gs::rgb4(4, 4, 4), gs::rgb4(12, 4, 3), gs::rgb4(6, 10, 6), gs::rgb4(3, 3, 3),
        gs::rgb4(15, 15, 15), 0, 0, 0, 0, 0, 0, gs::rgb4(2, 2, 2)};
    const uint16_t stone[16] = {
        0, gs::rgb4(3, 3, 4), gs::rgb4(6, 6, 7), gs::rgb4(9, 9, 10), gs::rgb4(4, 4, 5),
        gs::rgb4(7, 7, 6), gs::rgb4(12, 10, 4), gs::rgb4(2, 2, 3), gs::rgb4(14, 13, 8),
        0, 0, 0, 0, 0, 0, 0};
    const uint16_t keeper[16] = {
        0, gs::rgb4(2, 2, 2), gs::rgb4(6, 5, 3), gs::rgb4(12, 9, 6), gs::rgb4(4, 5, 3),
        gs::rgb4(5, 7, 4), gs::rgb4(14, 12, 5), gs::rgb4(3, 4, 5), gs::rgb4(8, 10, 6),
        0, 0, 0, 0, 0, 0, 0};
    const uint16_t foe[16] = {
        0, gs::rgb4(2, 2, 2), gs::rgb4(8, 3, 2), gs::rgb4(10, 6, 4), gs::rgb4(5, 4, 3),
        gs::rgb4(4, 4, 5), gs::rgb4(12, 8, 3), gs::rgb4(3, 3, 3), gs::rgb4(14, 12, 6),
        0, 0, 0, 0, 0, 0, 0};
    const uint16_t well[16] = {
        0, gs::rgb4(2, 2, 2), gs::rgb4(5, 5, 6), gs::rgb4(8, 8, 9), gs::rgb4(4, 4, 5),
        gs::rgb4(2, 5, 7), gs::rgb4(7, 6, 4), gs::rgb4(3, 3, 3), gs::rgb4(4, 8, 9),
        0, 0, 0, 0, 0, 0, 0};
    const uint16_t barrel[16] = {
        0, gs::rgb4(3, 2, 1), gs::rgb4(8, 5, 2), gs::rgb4(11, 7, 3), gs::rgb4(6, 4, 2),
        gs::rgb4(4, 3, 2), gs::rgb4(14, 10, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0};
    const uint16_t ram[16] = {
        0, gs::rgb4(2, 2, 2), gs::rgb4(5, 5, 6), gs::rgb4(7, 7, 8), gs::rgb4(9, 6, 3),
        gs::rgb4(12, 9, 6), gs::rgb4(13, 12, 8), gs::rgb4(4, 4, 5), 0, 0, 0, 0, 0, 0, 0, 0};
    const uint16_t road[16] = {
        0,
        gs::rgb4(4, 4, 5), gs::rgb4(3, 3, 4), gs::rgb4(6, 6, 6),
        gs::rgb4(5, 5, 5), gs::rgb4(3, 4, 4),
        gs::rgb4(6, 6, 6), gs::rgb4(4, 4, 5),
        gs::rgb4(8, 8, 7), gs::rgb4(5, 5, 5), gs::rgb4(7, 7, 6),
        gs::rgb4(2, 4, 6), gs::rgb4(3, 6, 7), gs::rgb4(5, 8, 8),
        gs::rgb4(12, 11, 6), gs::rgb4(9, 9, 8)};
    pal(vdp, PAL_HUD, hud);
    pal(vdp, PAL_STONE, stone);
    pal(vdp, PAL_KEEPER, keeper);
    pal(vdp, PAL_FOE, foe);
    pal(vdp, PAL_WELL, well);
    pal(vdp, PAL_BARREL, barrel);
    pal(vdp, PAL_RAM, ram);
    pal(vdp, PAL_ROAD, road);
    vdp.setFogColor(gs::rgb4(2, 3, 4));

    a.keeper = gs::uploadMipped(vdp, keeperArt(false));
    a.swing = gs::uploadMipped(vdp, keeperArt(true));
    a.sapper = gs::uploadMipped(vdp, sapperArt());
    a.barrel = gs::uploadMipped(vdp, barrelArt());
    a.ram = gs::uploadMipped(vdp, ramArt());
    for (int i = 0; i < 3; i++) a.well[i] = gs::uploadMipped(vdp, wellArt(i));
    a.archL = gs::uploadMipped(vdp, archSide(false));
    a.archR = gs::uploadMipped(vdp, archSide(true));
    a.lintel = gs::uploadMipped(vdp, lintelArt());
    loadFont(vdp, a);
}

}  // namespace culvertwell

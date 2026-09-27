#include "game/art.h"

#include <string>

namespace alleywell {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap wellArt() {
    Bitmap b(96, 120);
    b.rect(18, 78, 60, 34, 3);
    b.rect(22, 82, 52, 26, 2);
    for (int row = 0; row < 4; row++) {
        int y = 80 + row * 8;
        b.rect(20, y, 56, 2, 1);
        for (int k = 0; k < 4; k++) b.rect(24 + k * 14 + (row & 1) * 6, y + 2, 2, 6, 4);
    }
    b.ellipse(48, 78, 30, 12, 3);
    b.ellipse(48, 76, 26, 10, 2);
    b.ellipse(48, 76, 16, 6, 5);
    b.ellipse(44, 74, 6, 2, 8);
    b.rect(30, 28, 6, 52, 6);
    b.rect(60, 28, 6, 52, 6);
    b.rect(32, 30, 2, 48, 7);
    b.poly({{22, 30}, {48, 8}, {74, 30}}, 6);
    b.poly({{28, 28}, {48, 14}, {68, 28}}, 7);
    b.line(48, 18, 48, 70, 7, 1.5f);
    b.rect(44, 62, 8, 7, 4);
    b.rect(46, 64, 4, 4, 8);
    b.ellipse(48, 96, 22, 6, 4);
    b.outline(4, false);
    return b;
}

Bitmap crackArt() {
    Bitmap b(96, 120);
    b.line(30, 86, 40, 100, 1, 2);
    b.line(40, 100, 36, 112, 1, 2);
    b.line(58, 84, 52, 98, 1, 2);
    b.line(52, 98, 64, 110, 1, 2);
    b.line(44, 90, 50, 104, 1, 1.5f);
    return b;
}

Bitmap fellArt() {
    Bitmap b(120, 72);
    b.ellipse(60, 40, 46, 16, 3);
    b.ellipse(60, 38, 28, 9, 5);
    b.rect(18, 28, 22, 16, 2);
    b.rect(70, 22, 26, 14, 3);
    b.rect(40, 18, 18, 12, 4);
    b.rect(28, 44, 16, 10, 2);
    b.rect(78, 46, 20, 12, 3);
    b.poly({{8, 20}, {24, 8}, {30, 24}}, 6);
    b.rect(86, 14, 8, 18, 7);
    b.outline(4, false);
    return b;
}

Bitmap keeperArt(bool swinging) {
    Bitmap b(64, 88);
    b.ellipse(32, 14, 8, 8, 1);
    b.rect(26, 8, 12, 5, 4);
    b.rect(24, 22, 16, 22, 2);
    b.rect(26, 24, 12, 16, 3);
    b.rect(22, 24, 6, 16, 2);
    b.rect(36, 24, 6, 16, 2);
    b.rect(24, 44, 7, 22, 6);
    b.rect(33, 44, 7, 22, 6);
    b.rect(22, 64, 10, 6, 6);
    b.rect(33, 64, 10, 6, 6);
    if (!swinging) {
        b.line(18, 30, 50, 48, 5, 3);
        b.rect(46, 44, 8, 6, 5);
    } else {
        b.line(14, 40, 58, 28, 5, 3);
        b.rect(54, 22, 8, 8, 5);
        b.ellipse(56, 20, 6, 4, 7);
    }
    b.outline(4, false);
    return b;
}

Bitmap barrelArt() {
    Bitmap b(48, 56);
    b.ellipse(24, 12, 16, 7, 2);
    b.ellipse(24, 12, 10, 4, 1);
    b.rect(8, 12, 32, 32, 2);
    b.rect(10, 14, 28, 28, 1);
    for (int i = 0; i < 4; i++) b.rect(12 + i * 7, 14, 2, 28, 3);
    b.rect(8, 18, 32, 4, 4);
    b.rect(8, 34, 32, 4, 4);
    b.ellipse(24, 44, 16, 7, 2);
    b.outline(3, false);
    return b;
}

Bitmap cartArt() {
    Bitmap b(72, 56);
    b.poly({{8, 20}, {58, 16}, {62, 36}, {6, 38}}, 2);
    b.poly({{12, 22}, {52, 19}, {54, 32}, {12, 34}}, 1);
    b.rect(4, 34, 58, 5, 3);
    b.ellipse(16, 44, 8, 8, 4);
    b.ellipse(16, 44, 3, 3, 5);
    b.ellipse(48, 44, 8, 8, 4);
    b.ellipse(48, 44, 3, 3, 5);
    b.rect(58, 18, 4, 20, 3);
    b.outline(3, false);
    return b;
}

Bitmap ramArt() {
    Bitmap b(88, 48);
    b.ellipse(18, 24, 14, 14, 1);
    b.ellipse(18, 24, 8, 8, 2);
    b.rect(24, 16, 52, 16, 3);
    b.rect(26, 18, 48, 4, 1);
    for (int i = 0; i < 3; i++) b.rect(30 + i * 14, 18, 3, 12, 4);
    b.ellipse(72, 30, 7, 7, 2);
    b.ellipse(56, 32, 7, 7, 2);
    b.outline(2, false);
    return b;
}

Bitmap pierArt() {
    Bitmap b(36, 96);
    b.rect(4, 8, 28, 88, 1);
    for (int row = 0; row < 10; row++) {
        int y = 10 + row * 8;
        b.rect(6, y, 24, 2, 3);
        int shift = (row & 1) ? 8 : 0;
        b.rect(8 + shift, y + 2, 2, 6, 2);
        b.rect(18 + (shift ? -6 : 0), y + 2, 2, 6, 2);
    }
    b.rect(2, 4, 32, 6, 2);
    b.outline(2, false);
    return b;
}

Bitmap windowArt() {
    Bitmap b(28, 36);
    b.rect(2, 2, 24, 32, 1);
    b.rect(6, 6, 16, 20, 4);
    b.rect(6, 14, 16, 2, 3);
    b.rect(13, 6, 2, 20, 3);
    b.rect(8, 8, 4, 4, 5);
    b.rect(4, 28, 20, 4, 3);
    return b;
}

Bitmap dustArt() {
    Bitmap b(40, 24);
    b.ellipse(12, 14, 8, 5, 2);
    b.ellipse(22, 12, 10, 6, 1);
    b.ellipse(30, 15, 6, 4, 2);
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
    setPal(vdp, PAL_HUD, {gs::rgb4(0, 0, 0), gs::rgb4(15, 15, 14), gs::rgb4(14, 10, 4), gs::rgb4(13, 3, 2),
                          gs::rgb4(6, 13, 6), gs::rgb4(8, 8, 8)});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(12, 12, 10), gs::rgb4(9, 9, 8), gs::rgb4(6, 6, 6), gs::rgb4(3, 3, 4),
                            gs::rgb4(2, 5, 8), gs::rgb4(7, 4, 2), gs::rgb4(10, 7, 3), gs::rgb4(4, 6, 8)});
    setPal(vdp, PAL_KEEPER, {0, gs::rgb4(13, 9, 6), gs::rgb4(2, 3, 8), gs::rgb4(5, 6, 12), gs::rgb4(1, 1, 3),
                             gs::rgb4(10, 8, 3), gs::rgb4(2, 2, 2), gs::rgb4(14, 13, 10)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(11, 8, 3), gs::rgb4(8, 5, 2), gs::rgb4(4, 2, 1), gs::rgb4(10, 10, 9),
                           gs::rgb4(3, 3, 4)});
    setPal(vdp, PAL_IRON, {0, gs::rgb4(9, 9, 10), gs::rgb4(4, 4, 5), gs::rgb4(8, 5, 2), gs::rgb4(14, 12, 5)});
    setPal(vdp, PAL_BRICK, {0, gs::rgb4(9, 3, 2), gs::rgb4(5, 2, 2), gs::rgb4(11, 10, 8), gs::rgb4(2, 3, 6),
                            gs::rgb4(14, 11, 4)});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 15, 14), gs::rgb4(10, 9, 7), gs::rgb4(14, 6, 3)});
    const uint16_t road[16] = {
        0,
        gs::rgb4(8, 3, 2),
        gs::rgb4(5, 2, 1),
        gs::rgb4(9, 8, 7),
        gs::rgb4(4, 4, 4),
        gs::rgb4(3, 3, 3),
        gs::rgb4(6, 6, 5),
        gs::rgb4(8, 8, 7),
        gs::rgb4(3, 3, 3),
        gs::rgb4(5, 5, 4),
        gs::rgb4(7, 7, 6),
        gs::rgb4(3, 4, 6),
        gs::rgb4(4, 5, 7),
        gs::rgb4(6, 8, 10),
        gs::rgb4(4, 2, 2),
        gs::rgb4(12, 11, 9),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_ROAD * 16 + i, road[i]);

    art.well = gs::uploadMipped(vdp, wellArt());
    art.wellCrack = gs::uploadMipped(vdp, crackArt());
    art.wellFell = gs::uploadMipped(vdp, fellArt());
    art.keeper = gs::uploadMipped(vdp, keeperArt(false));
    art.shove = gs::uploadMipped(vdp, keeperArt(true));
    art.barrel = gs::uploadMipped(vdp, barrelArt());
    art.cart = gs::uploadMipped(vdp, cartArt());
    art.ram = gs::uploadMipped(vdp, ramArt());
    art.pier = gs::uploadMipped(vdp, pierArt());
    art.window = gs::uploadMipped(vdp, windowArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    loadFont(vdp, art);
}

}  // namespace alleywell

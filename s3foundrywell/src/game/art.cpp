#include "game/art.h"

namespace foundrywell {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
    while (i < 15) vdp.setColor(pal * 16 + i++, 0);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 1));
}

gs::Mipped up(gs::VDP& vdp, const gs::Bitmap& b) { return gs::uploadMipped(vdp, b.cropToContent(1)); }

void loadFont(gs::VDP& vdp, Art& a, gs::TileAlloc& tiles) {
    gs::TextStyle big{3, 1, 0, 15, 1};
    uint8_t blank[64];
    for (int i = 0; i < 64; i++) blank[i] = 2;
    a.panel = tiles.shared(blank);
    for (int c = 32; c < 128; c++) {
        uint8_t px[64];
        for (int i = 0; i < 64; i++) px[i] = 2;
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) {
                    if (y + 1 < 8 && x + 2 < 8) px[(y + 1) * 8 + x + 2] = 15;
                    px[y * 8 + x + 1] = 1;
                }
        a.font[c - 32] = tiles.shared(px);
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

void floorTiles(gs::VDP& vdp, Art& a, gs::TileAlloc& tiles) {
    uint8_t soot[64], brick[64], plate[64], grate[64];
    for (int i = 0; i < 64; i++) {
        soot[i] = 1;
        brick[i] = 3;
        plate[i] = 4;
        grate[i] = 2;
    }
    for (int y = 0; y < 8; y++) {
        brick[y * 8] = 5;
        brick[y * 8 + 7] = 6;
        if (y == 0 || y == 7)
            for (int x = 0; x < 8; x++) brick[y * 8 + x] = 5;
    }
    soot[3 * 8 + 2] = 2;
    soot[5 * 8 + 6] = 7;
    soot[1 * 8 + 5] = 8;
    for (int x = 0; x < 8; x++) {
        plate[2 * 8 + x] = 5;
        plate[5 * 8 + x] = 6;
    }
    plate[4 * 8 + 3] = 9;
    for (int y = 0; y < 8; y++) {
        grate[y * 8 + 1] = 5;
        grate[y * 8 + 4] = 9;
        grate[y * 8 + 6] = 5;
    }
    a.soot = tiles.shared(soot);
    a.brick = tiles.shared(brick);
    a.plate = tiles.shared(plate);
    a.grate = tiles.shared(grate);
}

void smithBmp(gs::Bitmap& b, int pose) {
    b = gs::Bitmap(32, 40);
    b.ellipse(16, 10, 6, 6, 2);
    b.rect(11, 15, 10, 14, 3);
    b.rect(12, 22, 8, 6, 4);
    b.rect(12, 28, 3, 8, 5);
    b.rect(17, 28, 3, 8, 5);
    b.rect(10, 16, 3, 8, 6);
    if (pose == 2) {
        b.rect(22, 16, 8, 3, 7);
        b.rect(28, 12, 3, 8, 8);
    } else if (pose == 1) {
        b.rect(22, 18, 6, 3, 6);
        b.rect(13, 30, 3, 6, 5);
    } else {
        b.rect(22, 20, 6, 3, 6);
    }
    b.set(14, 9, 1);
    b.set(18, 9, 1);
    b.outline(15, false);
}

void blob(gs::Bitmap& b, int w, int h, int pose, int body, int eye, int drip) {
    b = gs::Bitmap(w, h);
    b.ellipse(w * 0.5f, h * 0.55f, w * 0.38f, h * 0.32f, body);
    if (pose & 1) b.ellipse(w * 0.5f, h * 0.62f, w * 0.34f, h * 0.26f, body);
    b.ellipse(w * 0.38f, h * 0.42f, 2.2f, 2.2f, eye);
    b.ellipse(w * 0.62f, h * 0.42f, 2.2f, 2.2f, eye);
    b.rect(w * 0.46f, h * 0.78f, 3, h * 0.16f, drip);
    b.outline(15, false);
}

void pourBmp(gs::Bitmap& b, int pose) {
    b = gs::Bitmap(44, 36);
    b.ellipse(22, 20, 16, 10, 2);
    b.rect(8, 16, 28, 8, 3);
    b.poly({{6, 18}, {2, 8}, {12, 14}}, 4);
    b.poly({{38, 18}, {42, 8}, {32, 14}}, 4);
    b.ellipse(16, 16, 2.4f, 2.4f, 5);
    b.ellipse(28, 16, 2.4f, 2.4f, 5);
    if (pose & 1) b.rect(18, 24, 8, 6, 6);
    else b.rect(14, 24, 6, 5, 6);
    b.outline(15, false);
}

void wellBmp(gs::Bitmap& b) {
    b = gs::Bitmap(48, 56);
    b.rect(20, 4, 8, 18, 3);
    b.ellipse(24, 22, 6, 4, 4);
    b.rect(14, 24, 20, 22, 2);
    b.ellipse(24, 28, 14, 6, 5);
    b.ellipse(24, 30, 8, 4, 6);
    b.rect(16, 34, 16, 4, 7);
    b.rect(18, 44, 4, 8, 2);
    b.rect(26, 44, 4, 8, 2);
    b.outline(15, false);
}

void rubbleBmp(gs::Bitmap& b) {
    b = gs::Bitmap(48, 28);
    b.ellipse(16, 16, 10, 6, 2);
    b.ellipse(30, 18, 12, 5, 3);
    b.rect(20, 10, 8, 6, 4);
    b.outline(15, false);
}

void propStack(gs::Bitmap& b) {
    b = gs::Bitmap(28, 64);
    b.rect(8, 8, 12, 48, 2);
    b.rect(6, 6, 16, 6, 3);
    b.rect(10, 20, 8, 3, 4);
    b.rect(10, 36, 8, 3, 5);
    b.ellipse(14, 4, 4, 3, 6);
    b.outline(15, false);
}

void propCrucible(gs::Bitmap& b) {
    b = gs::Bitmap(36, 32);
    b.poly({{6, 8}, {30, 8}, {26, 26}, {10, 26}}, 2);
    b.ellipse(18, 10, 12, 4, 3);
    b.ellipse(18, 10, 7, 2, 4);
    b.rect(16, 26, 4, 4, 5);
    b.outline(15, false);
}

void propAnvil(gs::Bitmap& b) {
    b = gs::Bitmap(36, 22);
    b.rect(4, 6, 26, 6, 2);
    b.rect(10, 12, 12, 6, 3);
    b.rect(6, 18, 22, 3, 4);
    b.rect(22, 4, 8, 4, 5);
    b.outline(15, false);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& a) {
    setPal(vdp, PAL_TEXT, {gs::rgb4(0, 0, 0), gs::rgb4(15, 13, 8), gs::rgb4(3, 2, 2), gs::rgb4(8, 6, 4)});
    setPal(vdp, PAL_EMBER, {gs::rgb4(0, 0, 0), gs::rgb4(15, 12, 3), gs::rgb4(4, 2, 1), gs::rgb4(10, 5, 1)});
    setPal(vdp, PAL_ALERT, {gs::rgb4(0, 0, 0), gs::rgb4(15, 4, 2), gs::rgb4(5, 1, 1), gs::rgb4(12, 8, 3)});
    setPal(vdp, PAL_GOOD, {gs::rgb4(0, 0, 0), gs::rgb4(8, 15, 6), gs::rgb4(2, 4, 2), gs::rgb4(12, 14, 8)});
    setPal(vdp, PAL_SMITH, {gs::rgb4(0, 0, 0), gs::rgb4(2, 1, 1), gs::rgb4(12, 8, 5), gs::rgb4(6, 6, 7),
                            gs::rgb4(9, 3, 2), gs::rgb4(4, 3, 3), gs::rgb4(11, 9, 6), gs::rgb4(8, 8, 9),
                            gs::rgb4(14, 12, 4)});
    setPal(vdp, PAL_SPARK, {gs::rgb4(0, 0, 0), gs::rgb4(1, 1, 1), gs::rgb4(15, 8, 2), gs::rgb4(15, 14, 4),
                            gs::rgb4(12, 3, 1)});
    setPal(vdp, PAL_SLAG, {gs::rgb4(0, 0, 0), gs::rgb4(1, 1, 1), gs::rgb4(5, 5, 6), gs::rgb4(9, 8, 7),
                           gs::rgb4(14, 6, 2), gs::rgb4(3, 3, 3)});
    setPal(vdp, PAL_POUR, {gs::rgb4(0, 0, 0), gs::rgb4(1, 1, 1), gs::rgb4(8, 3, 2), gs::rgb4(12, 5, 2),
                           gs::rgb4(6, 4, 4), gs::rgb4(15, 13, 3), gs::rgb4(15, 7, 1)});
    setPal(vdp, PAL_WELL, {gs::rgb4(0, 0, 0), gs::rgb4(1, 1, 1), gs::rgb4(7, 7, 8), gs::rgb4(4, 4, 5),
                           gs::rgb4(10, 9, 8), gs::rgb4(5, 5, 6), gs::rgb4(1, 2, 4), gs::rgb4(11, 8, 4)});
    setPal(vdp, PAL_IRON, {gs::rgb4(0, 0, 0), gs::rgb4(1, 1, 1), gs::rgb4(6, 6, 7), gs::rgb4(9, 4, 2),
                           gs::rgb4(15, 9, 2), gs::rgb4(4, 4, 5), gs::rgb4(12, 10, 6)});
    setPal(vdp, PAL_FX, {gs::rgb4(0, 0, 0), gs::rgb4(15, 15, 12), gs::rgb4(15, 10, 3), gs::rgb4(15, 6, 1),
                         gs::rgb4(8, 8, 8)});
    setPal(vdp, PAL_FLOOR, {gs::rgb4(0, 0, 0), gs::rgb4(3, 2, 2), gs::rgb4(4, 3, 3), gs::rgb4(6, 3, 2),
                            gs::rgb4(5, 5, 6), gs::rgb4(8, 7, 6), gs::rgb4(2, 2, 2), gs::rgb4(7, 4, 2),
                            gs::rgb4(10, 6, 2), gs::rgb4(15, 8, 2)});
    setPal(vdp, PAL_HEAT, {gs::rgb4(0, 0, 0), gs::rgb4(8, 2, 1), gs::rgb4(12, 5, 1), gs::rgb4(15, 10, 2)});

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, a, tiles);
    floorTiles(vdp, a, tiles);

    gs::Bitmap b;
    wellBmp(b);
    a.well = up(vdp, b);
    rubbleBmp(b);
    a.rubble = up(vdp, b);
    b = gs::Bitmap(20, 16);
    b.line(2, 2, 16, 14, 1, 2);
    b.line(4, 12, 14, 4, 1, 1);
    a.crack = up(vdp, b);
    for (int i = 0; i < 3; i++) {
        smithBmp(b, i);
        a.smith[i] = up(vdp, b);
    }
    for (int i = 0; i < 2; i++) {
        blob(b, 24, 20, i, 2, 3, 4);
        a.spark[i] = up(vdp, b);
        blob(b, 30, 24, i, 2, 4, 3);
        a.slag[i] = up(vdp, b);
        pourBmp(b, i);
        a.pour[i] = up(vdp, b);
    }
    b = gs::Bitmap(16, 16);
    b.ellipse(8, 8, 6, 5, 2);
    b.ellipse(8, 8, 3, 2, 1);
    a.puff = up(vdp, b);
    b = gs::Bitmap(22, 12);
    b.ellipse(11, 6, 9, 4, 1);
    b.ellipse(11, 6, 5, 2, 3);
    a.arc = up(vdp, b);
    b = gs::Bitmap(8, 4);
    b.rect(0, 0, 8, 4, 1);
    a.bar = up(vdp, b);
    b = gs::Bitmap(16, 8);
    b.ellipse(8, 4, 7, 3, 1);
    a.shadow = up(vdp, b);
    propStack(b);
    a.stack = up(vdp, b);
    propCrucible(b);
    a.crucible = up(vdp, b);
    propAnvil(b);
    a.anvil = up(vdp, b);
    b = gs::Bitmap(18, 28);
    b.rect(8, 2, 3, 18, 2);
    b.ellipse(9, 22, 6, 4, 3);
    a.ladle = up(vdp, b);
    b = gs::Bitmap(20, 12);
    b.rect(2, 3, 16, 6, 2);
    b.rect(4, 4, 12, 3, 3);
    a.ingot = up(vdp, b);
    b = gs::Bitmap(8, 20);
    for (int y = 0; y < 18; y += 4) b.ellipse(4, 2 + y, 3, 2, 2);
    a.chain = up(vdp, b);
}

}  // namespace foundrywell

#include "game/art.h"

#include <initializer_list>

namespace keep {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap doorArt() {
    Bitmap b(88, 148);
    b.rect(3, 3, 82, 142, 4);
    for (int i = 0; i < 5; i++) {
        int x = 6 + i * 16;
        int tone = (i & 1) ? 2 : 3;
        b.rect(float(x), 6, 14, 136, tone);
        b.rect(float(x), 6, 3, 136, 1);
        for (int y = 14; y < 140; y += 9) b.set(x + 7, y + (i % 3), 8);
    }
    b.rect(5, 28, 78, 9, 5);
    b.rect(5, 28, 78, 2, 6);
    b.rect(5, 104, 78, 9, 5);
    b.rect(5, 104, 78, 2, 6);
    for (int i = 0; i < 5; i++) {
        b.ellipse(12.0f + i * 16, 32, 2.2f, 2.2f, 7);
        b.ellipse(12.0f + i * 16, 108, 2.2f, 2.2f, 7);
    }
    b.rect(4, 18, 7, 16, 5);
    b.rect(4, 70, 7, 18, 5);
    b.rect(4, 120, 7, 16, 5);
    b.rect(5, 20, 2, 12, 6);
    b.ellipse(64, 74, 8, 8, 5);
    b.ellipse(64, 74, 4.5f, 4.5f, 9);
    b.ellipse(66, 72, 1.4f, 1.4f, 6);
    b.outline(8, false);
    return b;
}

Bitmap jambArt() {
    Bitmap b(44, 168);
    b.rect(4, 2, 36, 164, 3);
    for (int row = 0; row < 8; row++) {
        int y = 4 + row * 20;
        int x = (row & 1) ? 18 : 6;
        int tone = (row % 3) == 0 ? 1 : 2;
        b.rect(float(x), float(y), 16, 16, tone);
        b.rect(float(x), float(y), 16, 3, 4);
        if (row == 2 || row == 5) b.rect(float(x + 2), float(y + 6), 8, 3, 7);
    }
    b.rect(18, 46, 7, 28, 5);
    b.rect(20, 48, 3, 24, 6);
    b.outline(6, false);
    return b;
}

Bitmap lintelArt() {
    Bitmap b(168, 22);
    b.rect(2, 4, 164, 14, 2);
    b.rect(2, 4, 164, 3, 4);
    b.rect(2, 14, 164, 4, 3);
    for (int i = 0; i < 6; i++) b.rect(float(14 + i * 24), 7, 4, 8, 5);
    b.poly({{78, 6}, {90, 6}, {84, 14}}, 8);
    b.outline(6, false);
    return b;
}

Bitmap floorArt() {
    Bitmap b(200, 32);
    b.rect(0, 6, 200, 26, 3);
    for (int i = 0; i < 6; i++) {
        int x = 4 + i * 32;
        b.rect(float(x), 8, 28, 12, (i & 1) ? 2 : 1);
        b.rect(float(x), 20, 28, 8, 2);
        b.rect(float(x), 8, 28, 2, 4);
    }
    return b;
}

Bitmap barArt() {
    Bitmap b(124, 20);
    b.rect(8, 4, 108, 12, 2);
    b.rect(8, 4, 108, 3, 1);
    b.rect(8, 13, 108, 3, 3);
    b.rect(2, 3, 8, 14, 4);
    b.rect(114, 3, 8, 14, 4);
    b.rect(4, 5, 3, 10, 5);
    b.rect(117, 5, 3, 10, 5);
    b.outline(6, false);
    return b;
}

Bitmap barTiltArt() {
    Bitmap b(108, 52);
    b.poly({{10, 42}, {92, 10}, {100, 16}, {18, 48}}, 2);
    b.poly({{12, 38}, {90, 8}, {96, 12}, {18, 42}}, 1);
    b.ellipse(12, 42, 7, 7, 4);
    b.ellipse(96, 12, 7, 7, 4);
    b.ellipse(12, 42, 3, 3, 5);
    b.ellipse(96, 12, 3, 3, 5);
    b.outline(6, false);
    return b;
}

Bitmap bracketArt() {
    Bitmap b(18, 22);
    b.rect(2, 2, 4, 18, 4);
    b.rect(12, 2, 4, 18, 4);
    b.rect(2, 14, 14, 5, 4);
    b.rect(3, 3, 2.5f, 8, 5);
    b.outline(6, false);
    return b;
}

Bitmap keeperArt(bool shove) {
    Bitmap b(76, 92);
    b.poly({{22, 34}, {54, 34}, {66, 88}, {10, 88}}, 2);
    b.poly({{26, 38}, {50, 38}, {58, 86}, {18, 86}}, 1);
    b.ellipse(38, 26, 14, 15, 2);
    b.ellipse(38, 24, 9, 11, 3);
    b.ellipse(20, 42, 11, 8, 2);
    b.ellipse(56, 42, 11, 8, 2);
    if (shove) {
        b.poly({{18, 40}, {10, 8}, {20, 6}, {30, 38}}, 2);
        b.poly({{58, 40}, {66, 8}, {56, 6}, {46, 38}}, 2);
        b.ellipse(14, 8, 6, 5, 6);
        b.ellipse(62, 8, 6, 5, 6);
    } else {
        b.poly({{18, 42}, {8, 72}, {18, 76}, {28, 48}}, 2);
        b.poly({{58, 42}, {68, 72}, {58, 76}, {48, 48}}, 2);
        b.ellipse(10, 74, 5, 4, 6);
        b.ellipse(66, 74, 5, 4, 6);
    }
    b.rect(24, 60, 28, 5, 7);
    b.rect(36, 58, 4, 8, 7);
    b.outline(8, false);
    return b;
}

Bitmap handArt() {
    Bitmap b(30, 22);
    b.ellipse(12, 12, 8, 7, 6);
    b.rect(14, 7, 13, 6, 6);
    b.rect(16, 4, 4, 5, 5);
    b.rect(21, 3, 4, 5, 5);
    b.ellipse(8, 14, 3.5f, 4, 5);
    b.outline(8, false);
    return b;
}

Bitmap ramArt() {
    Bitmap b(72, 30);
    b.rect(16, 8, 52, 14, 3);
    b.rect(16, 8, 52, 4, 2);
    b.rect(16, 18, 52, 4, 1);
    b.ellipse(14, 15, 10, 11, 5);
    b.ellipse(14, 15, 6, 7, 6);
    b.rect(28, 6, 4, 18, 5);
    b.rect(48, 6, 4, 18, 5);
    b.outline(8, false);
    return b;
}

Bitmap crowArt() {
    Bitmap b(48, 22);
    b.line(6, 16, 42, 5, 4, 3.5f);
    b.line(6, 16, 4, 8, 5, 2.5f);
    b.line(4, 8, 12, 8, 5, 2.5f);
    b.ellipse(42, 5, 3, 3, 5);
    return b;
}

Bitmap torchArt(int frame) {
    Bitmap b(22, 40);
    b.rect(9, 18, 4, 16, 5);
    b.rect(7, 32, 8, 5, 6);
    float fy = frame ? 3.0f : 0.0f;
    b.ellipse(11, 14 - fy, 6, 9, 3);
    b.ellipse(11, 15 - fy, 3.5f, 6, 2);
    b.ellipse(11, 16 - fy, 2, 3.5f, 1);
    if (frame) b.set(5, 10, 2);
    else b.set(16, 8, 2);
    return b;
}

Bitmap nightArt() {
    Bitmap b(96, 140);
    b.rect(0, 0, 96, 140, 3);
    b.rect(0, 0, 96, 48, 4);
    b.rect(18, 88, 20, 52, 2);
    b.rect(54, 100, 14, 40, 2);
    b.rect(24, 78, 8, 12, 5);
    const int sx[] = {8, 28, 46, 70, 18, 60, 84, 38, 76, 12};
    const int sy[] = {10, 18, 8, 24, 36, 30, 14, 48, 42, 58};
    for (int i = 0; i < 10; i++) b.set(sx[i], sy[i], 1);
    return b;
}

Bitmap crackArt() {
    Bitmap b(8, 120);
    b.rect(3, 0, 2, 120, 1);
    b.rect(2, 10, 4, 8, 2);
    b.rect(2, 50, 4, 14, 2);
    b.rect(2, 90, 4, 8, 2);
    return b;
}

Bitmap moteArt() {
    Bitmap b(5, 5);
    b.ellipse(2.5f, 2.5f, 2, 2, 1);
    return b;
}

Bitmap chipArt() {
    Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 1);
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
    const uint16_t shadow = gs::rgb4(1, 1, 1);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 15), gs::rgb4(10, 10, 12), gs::rgb4(15, 12, 8), gs::rgb4(8, 12, 8),
                          gs::rgb4(15, 13, 5), gs::rgb4(15, 8, 3), shadow, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(10, 9, 8), gs::rgb4(7, 6, 6), gs::rgb4(4, 4, 5), gs::rgb4(13, 11, 8),
                            gs::rgb4(2, 2, 4), gs::rgb4(2, 2, 2), gs::rgb4(4, 6, 4), gs::rgb4(11, 7, 4), 0, 0, 0, 0, 0, 0,
                            shadow});
    setPal(vdp, PAL_OAK, {0, gs::rgb4(13, 9, 5), gs::rgb4(10, 6, 3), gs::rgb4(7, 4, 2), gs::rgb4(3, 2, 1),
                          gs::rgb4(8, 8, 10), gs::rgb4(13, 13, 14), gs::rgb4(15, 14, 9), gs::rgb4(2, 1, 1),
                          gs::rgb4(4, 4, 5), 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_IRON, {0, gs::rgb4(12, 8, 4), gs::rgb4(8, 5, 2), gs::rgb4(4, 3, 2), gs::rgb4(9, 9, 11),
                           gs::rgb4(14, 14, 15), gs::rgb4(2, 2, 3), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_NIGHT, {0, gs::rgb4(14, 14, 9), gs::rgb4(4, 5, 9), gs::rgb4(1, 1, 3), gs::rgb4(2, 2, 6),
                            gs::rgb4(7, 8, 12), gs::rgb4(3, 3, 5), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FIRE, {0, gs::rgb4(15, 15, 11), gs::rgb4(15, 10, 2), gs::rgb4(13, 5, 1), gs::rgb4(8, 2, 1),
                           gs::rgb4(6, 4, 2), gs::rgb4(3, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_CLOAK, {0, gs::rgb4(11, 3, 3), gs::rgb4(7, 2, 2), gs::rgb4(3, 1, 1), gs::rgb4(13, 10, 7),
                            gs::rgb4(8, 6, 4), gs::rgb4(4, 4, 5), gs::rgb4(12, 11, 6), gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, 0,
                            shadow});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 5), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 4, 3), gs::rgb4(8, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_TRACK, {0, gs::rgb4(2, 2, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_WARN, {0, gs::rgb4(15, 10, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    art.door = gs::uploadMipped(vdp, doorArt());
    art.jamb = gs::uploadMipped(vdp, jambArt());
    art.lintel = gs::uploadMipped(vdp, lintelArt());
    art.floor = gs::uploadMipped(vdp, floorArt());
    art.bar = gs::uploadMipped(vdp, barArt());
    art.barTilt = gs::uploadMipped(vdp, barTiltArt());
    art.bracket = gs::uploadMipped(vdp, bracketArt());
    art.keeper = gs::uploadMipped(vdp, keeperArt(false));
    art.keeperShove = gs::uploadMipped(vdp, keeperArt(true));
    art.hand = gs::uploadMipped(vdp, handArt());
    art.ram = gs::uploadMipped(vdp, ramArt());
    art.crow = gs::uploadMipped(vdp, crowArt());
    art.torch[0] = gs::uploadMipped(vdp, torchArt(0));
    art.torch[1] = gs::uploadMipped(vdp, torchArt(1));
    art.night = gs::uploadMipped(vdp, nightArt());
    art.crack = gs::uploadMipped(vdp, crackArt());
    art.mote = gs::uploadMipped(vdp, moteArt());
    art.chip = gs::uploadMipped(vdp, chipArt());
    loadFont(vdp, art);
}

}  // namespace keep

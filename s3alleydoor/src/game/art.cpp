#include "game/art.h"

#include <initializer_list>
#include <string>

namespace alley {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
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

gs::Bitmap leafArt() {
    gs::Bitmap b(44, 108);
    b.rect(2, 2, 40, 104, 2);
    b.rect(2, 2, 8, 104, 1);
    b.rect(34, 2, 8, 104, 3);
    for (int y = 10; y < 100; y += 14) b.rect(6, float(y), 32, 2, 4);
    b.rect(14, 16, 16, 22, 5);
    b.rect(16, 18, 12, 16, 8);
    b.rect(18, 22, 4, 4, 9);
    b.rect(30, 58, 6, 10, 6);
    b.rect(31, 60, 3, 4, 7);
    b.rect(8, 92, 28, 6, 4);
    b.outline(15, false);
    return b;
}

gs::Bitmap jambArt() {
    gs::Bitmap b(16, 120);
    b.rect(2, 2, 12, 116, 2);
    b.rect(2, 2, 4, 116, 1);
    b.rect(10, 2, 4, 116, 3);
    for (int y = 8; y < 112; y += 10) b.rect(3, float(y), 10, 2, 4);
    b.outline(15, false);
    return b;
}

gs::Bitmap lintelArt() {
    gs::Bitmap b(120, 16);
    b.rect(1, 3, 118, 11, 2);
    b.rect(1, 3, 118, 3, 1);
    b.rect(1, 11, 118, 3, 3);
    for (int i = 0; i < 10; i++) b.rect(float(6 + i * 11), 6, 3, 5, 5);
    b.outline(15, false);
    return b;
}

gs::Bitmap keeperArt(int frame) {
    gs::Bitmap b(28, 48);
    b.ellipse(14, 6, 6, 4, 4);
    b.ellipse(14, 12, 5, 5, 3);
    b.set(12, 12, 8);
    b.set(16, 12, 8);
    b.rect(10, 16, 8, 12, 1);
    b.rect(9, 27, 10, 3, 6);
    if (frame == 0) {
        b.line(10, 20, 4, 34, 1, 3.0f);
        b.rect(6, 32, 6, 12, 2);
        b.rect(16, 32, 6, 12, 1);
    } else {
        b.line(10, 20, 2, 24, 1, 3.0f);
        b.line(18, 20, 25, 28, 1, 3.0f);
        b.rect(8, 32, 5, 12, 2);
        b.rect(16, 32, 5, 12, 1);
    }
    b.rect(6, 42, 7, 4, 5);
    b.rect(16, 42, 7, 4, 5);
    b.outline(15, false);
    return b;
}

gs::Bitmap pusherArt(int frame) {
    gs::Bitmap b(26, 52);
    b.ellipse(13, 6, 5, 3, 4);
    b.ellipse(13, 12, 5, 5, 2);
    b.set(11, 12, 8);
    b.set(15, 12, 8);
    b.rect(9, 16, 8, 14, 1);
    b.rect(8, 28, 10, 3, 6);
    float arm = frame ? 30.f : 22.f;
    b.line(9, 18, 2, arm, 1, 2.8f);
    b.line(17, 18, 24, arm, 1, 2.8f);
    b.rect(7, 32, 5, 14, 3);
    b.rect(14, 32, 5, 14, 1);
    b.rect(6, 44, 6, 4, 5);
    b.rect(14, 44, 6, 4, 5);
    b.outline(15, false);
    return b;
}

gs::Bitmap chainArt() {
    gs::Bitmap b(10, 40);
    for (int y = 2; y < 36; y += 6) {
        b.rect(2, float(y), 6, 4, 1);
        b.rect(3, float(y + 1), 4, 2, 2);
    }
    b.rect(1, 34, 8, 4, 3);
    b.outline(15, false);
    return b;
}

gs::Bitmap brickArt() {
    gs::Bitmap b(22, 12);
    b.rect(1, 1, 20, 10, 1);
    b.rect(1, 1, 20, 3, 2);
    b.rect(1, 8, 20, 3, 3);
    b.line(11, 2, 11, 9, 4, 1.2f);
    b.outline(15, false);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(18, 28);
    b.rect(8, 1, 2, 8, 3);
    b.poly({{3, 9}, {15, 9}, {13, 20}, {5, 20}}, 1);
    b.rect(5, 12, 8, 5, 2);
    b.rect(7, 20, 4, 6, 3);
    b.outline(15, false);
    return b;
}

gs::Bitmap neonArt() {
    gs::Bitmap b(40, 16);
    b.rect(1, 2, 38, 12, 3);
    b.rect(3, 4, 34, 8, 1);
    b.rect(6, 6, 4, 4, 2);
    b.rect(14, 6, 4, 4, 2);
    b.rect(22, 6, 8, 4, 2);
    b.rect(32, 6, 3, 4, 2);
    b.outline(15, false);
    return b;
}

gs::Bitmap binArt() {
    gs::Bitmap b(28, 32);
    b.rect(4, 6, 20, 22, 2);
    b.rect(4, 6, 20, 4, 1);
    b.rect(3, 4, 22, 3, 3);
    b.rect(8, 14, 12, 8, 4);
    b.rect(10, 26, 4, 4, 5);
    b.rect(16, 26, 4, 4, 5);
    b.outline(15, false);
    return b;
}

gs::Bitmap catArt() {
    gs::Bitmap b(24, 14);
    b.ellipse(12, 8, 9, 4, 1);
    b.poly({{4, 6}, {6, 1}, {8, 6}}, 1);
    b.poly({{12, 6}, {14, 1}, {16, 6}}, 1);
    b.set(9, 7, 8);
    b.set(14, 7, 8);
    b.rect(20, 7, 3, 2, 2);
    b.outline(15, false);
    return b;
}

gs::Bitmap pipeArt() {
    gs::Bitmap b(64, 10);
    b.rect(0, 2, 64, 6, 2);
    b.rect(0, 2, 64, 2, 1);
    for (int x = 8; x < 60; x += 12) b.rect(float(x), 1, 3, 8, 3);
    b.outline(15, false);
    return b;
}

gs::Bitmap grateArt() {
    gs::Bitmap b(36, 14);
    b.rect(1, 2, 34, 10, 2);
    for (int x = 4; x < 32; x += 5) b.rect(float(x), 3, 2, 8, 1);
    b.rect(2, 6, 32, 2, 3);
    b.outline(15, false);
    return b;
}

gs::Bitmap dropArt() {
    gs::Bitmap b(3, 7);
    b.line(1, 0, 1, 6, 1, 1.4f);
    return b;
}

gs::Bitmap sparkArt() {
    gs::Bitmap b(5, 5);
    b.rect(2, 0, 1, 5, 1);
    b.rect(0, 2, 5, 1, 2);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(8, 4);
    b.ellipse(4, 2, 4, 2, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(14, 13, 11), gs::rgb4(8, 7, 6), gs::rgb4(3, 3, 4)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(15, 12, 5), gs::rgb4(12, 8, 2), gs::rgb4(6, 4, 1), gs::rgb4(15, 15, 10)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(10, 2, 2), gs::rgb4(15, 10, 6)});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(6, 14, 8), gs::rgb4(3, 8, 4), gs::rgb4(12, 15, 10)});
    setPal(vdp, PAL_BRICK, {0, gs::rgb4(9, 4, 3), gs::rgb4(12, 6, 4), gs::rgb4(5, 2, 2), gs::rgb4(7, 6, 5),
                            gs::rgb4(3, 2, 2)});
    setPal(vdp, PAL_COAT, {0, gs::rgb4(3, 4, 7), gs::rgb4(6, 7, 10), gs::rgb4(2, 2, 3), gs::rgb4(12, 9, 6),
                           gs::rgb4(4, 3, 2), gs::rgb4(8, 3, 3), gs::rgb4(1, 1, 1), gs::rgb4(14, 12, 8)});
    setPal(vdp, PAL_IRON, {0, gs::rgb4(5, 6, 7), gs::rgb4(8, 9, 10), gs::rgb4(3, 3, 4), gs::rgb4(6, 5, 4),
                           gs::rgb4(2, 4, 6), gs::rgb4(10, 9, 6), gs::rgb4(14, 12, 6), gs::rgb4(9, 12, 14),
                           gs::rgb4(13, 14, 12)});
    setPal(vdp, PAL_NEON, {0, gs::rgb4(4, 14, 8), gs::rgb4(10, 15, 12), gs::rgb4(1, 4, 3), gs::rgb4(2, 8, 5)});
    setPal(vdp, PAL_PROP, {0, gs::rgb4(4, 4, 4), gs::rgb4(7, 7, 6), gs::rgb4(2, 2, 2), gs::rgb4(8, 5, 3),
                           gs::rgb4(3, 3, 2), gs::rgb4(11, 8, 4)});
    setPal(vdp, PAL_WET, {0, gs::rgb4(6, 8, 10), gs::rgb4(10, 12, 12), gs::rgb4(3, 4, 5)});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(2, 2, 5), gs::rgb4(4, 4, 8), gs::rgb4(8, 7, 6)});
    setPal(vdp, PAL_MIST, {0, gs::rgb4(7, 7, 8), gs::rgb4(4, 4, 5)});
    setPal(vdp, PAL_ROAD, {0, gs::rgb4(3, 3, 3), gs::rgb4(5, 5, 5), gs::rgb4(2, 2, 3), gs::rgb4(6, 6, 5),
                           gs::rgb4(4, 4, 5), gs::rgb4(8, 7, 5), gs::rgb4(1, 1, 2), gs::rgb4(7, 6, 4)});

    art.leaf = gs::uploadMipped(vdp, leafArt());
    art.jamb = gs::uploadMipped(vdp, jambArt());
    art.lintel = gs::uploadMipped(vdp, lintelArt());
    art.keeper[0] = gs::uploadMipped(vdp, keeperArt(0));
    art.keeper[1] = gs::uploadMipped(vdp, keeperArt(1));
    art.pusher[0] = gs::uploadMipped(vdp, pusherArt(0));
    art.pusher[1] = gs::uploadMipped(vdp, pusherArt(1));
    art.chain = gs::uploadMipped(vdp, chainArt());
    art.brick = gs::uploadMipped(vdp, brickArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.neon = gs::uploadMipped(vdp, neonArt());
    art.bin = gs::uploadMipped(vdp, binArt());
    art.cat = gs::uploadMipped(vdp, catArt());
    art.pipe = gs::uploadMipped(vdp, pipeArt());
    art.grate = gs::uploadMipped(vdp, grateArt());
    art.drop = gs::uploadMipped(vdp, dropArt());
    art.spark = gs::uploadMipped(vdp, sparkArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    loadFont(vdp, art);

    uint8_t brick[64];
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int mortar = (y == 0 || y == 4 || ((y < 4) ? (x == 0) : (x == 4)));
            brick[y * 8 + x] = uint8_t(mortar ? 4 : ((x + y) & 1) ? 1 : 2);
        }
    }
    vdp.loadTile(1, brick);
}

}  // namespace alley

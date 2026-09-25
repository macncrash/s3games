#include "art.h"

#include <cmath>
#include <initializer_list>

namespace barge {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
    while (i < 15) vdp.setColor(pal * 16 + i++, 0);
    vdp.setColor(pal * 16 + 0, 0);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

gs::Bitmap boatArt() {
    gs::Bitmap b(160, 40);
    // Bow is to the right. Bottom edge is the keel, top is the stack.
    b.poly({{2.f, 28.f}, {146.f, 24.f}, {156.f, 30.f}, {148.f, 38.f}, {8.f, 38.f}, {2.f, 32.f}}, 2);
    b.poly({{10.f, 32.f}, {144.f, 30.f}, {146.f, 38.f}, {12.f, 38.f}}, 3);
    b.poly({{4.f, 22.f}, {148.f, 20.f}, {150.f, 25.f}, {6.f, 27.f}}, 4);
    b.rect(14, 17, 120, 5, 1);
    b.rect(52, 11, 40, 8, 5);
    b.rect(54, 8, 16, 5, 10);
    b.rect(72, 9, 16, 5, 10);
    b.rect(16, 8, 32, 11, 1);
    b.rect(19, 10, 8, 6, 6);
    b.rect(29, 10, 8, 6, 13);
    b.rect(40, 11, 5, 8, 12);
    b.poly({{14.f, 8.f}, {50.f, 8.f}, {48.f, 4.f}, {16.f, 4.f}}, 8);
    b.rect(26, 0, 5, 5, 7);
    b.rect(25, 0, 7, 2, 4);
    b.rect(6, 6, 2, 10, 7);
    b.poly({{8.f, 6.f}, {20.f, 9.f}, {8.f, 12.f}}, 4);
    b.ellipse(18, 24, 3.2f, 3.2f, 9);
    b.ellipse(142, 23, 3.2f, 3.2f, 9);
    b.rect(58, 28, 44, 2, 11);
    gs::TextStyle st{1, 8, 0, 0, 0};
    b.blit(gs::textBitmap("NELL", st), 62, 26);
    b.outline(15, false);
    return b;
}

gs::Bitmap gateArt() {
    gs::Bitmap b(32, 96);
    b.rect(4, 0, 24, 96, 1);
    for (int y = 0; y < 96; y += 8) b.rect(6, y, 20, 7, ((y / 8) & 1) ? 1 : 3);
    for (int y = 4; y < 92; y += 16) b.rect(3, y, 26, 4, 4);
    b.rect(0, 0, 4, 96, 5);
    b.rect(28, 0, 4, 96, 5);
    for (int y = 12; y < 88; y += 18) {
        b.rect(8, y, 3, 3, 6);
        b.rect(21, y, 3, 3, 6);
    }
    b.rect(12, 0, 8, 6, 5);
    b.outline(5, false);
    return b;
}

gs::Bitmap stoneArt() {
    gs::Bitmap b(48, 36);
    b.rect(0, 0, 48, 36, 2);
    for (int row = 0; row < 36; row += 9) {
        int off = ((row / 9) & 1) ? 12 : 0;
        for (int x = -24; x < 48; x += 24) {
            int shade = ((x + row) & 32) ? 1 : 3;
            b.rect(x + off, row + 1, 22, 7, shade);
        }
        b.rect(0, row, 48, 1, 5);
    }
    for (int x = 2; x < 46; x += 8) b.rect(x, 0, 4, 2, 4);
    return b;
}

gs::Bitmap grassArt() {
    gs::Bitmap b(80, 28);
    b.rect(0, 10, 80, 18, 4);
    b.poly({{0.f, 14.f}, {10.f, 6.f}, {22.f, 12.f}, {36.f, 2.f}, {52.f, 11.f}, {66.f, 4.f}, {80.f, 12.f}, {80.f, 28.f}, {0.f, 28.f}}, 1);
    for (int x = 4; x < 76; x += 9) b.rect(x, 8, 2, 6, 2);
    for (int x = 8; x < 74; x += 14) b.set(x, 7, 5);
    return b;
}

gs::Bitmap houseArt() {
    gs::Bitmap b(64, 48);
    b.rect(8, 22, 48, 24, 1);
    b.poly({{4.f, 22.f}, {32.f, 4.f}, {60.f, 22.f}}, 2);
    b.rect(28, 30, 10, 16, 3);
    b.rect(14, 26, 10, 8, 4);
    b.rect(42, 26, 10, 8, 4);
    b.rect(30, 8, 4, 10, 5);
    b.rect(18, 40, 6, 4, 6);
    b.outline(5, false);
    return b;
}

gs::Bitmap treeArt(bool fat) {
    gs::Bitmap b(40, 52);
    b.rect(17, 32, 6, 18, 3);
    b.ellipse(20, 22, fat ? 16.f : 13.f, fat ? 16.f : 14.f, 1);
    b.ellipse(14, 20, 7, 8, 2);
    b.ellipse(26, 24, 6, 6, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(12, 28);
    b.rect(4, 4, 4, 24, 1);
    b.rect(2, 2, 8, 5, 2);
    b.rect(5, 10, 2, 2, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap gullArt(bool up) {
    gs::Bitmap b(28, 14);
    float tip = up ? 2.f : 11.f;
    b.line(14, 7, 2, tip, 1, 1.6f);
    b.line(14, 7, 26, tip, 1, 1.6f);
    b.ellipse(14, 8, 2.4f, 1.6f, 1);
    b.set(18, 7, 2);
    return b;
}

gs::Bitmap waterArt() {
    gs::Bitmap b(32, 32);
    for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 32; x++) {
            int n = (x * 3 + y * 5) & 7;
            int c = n < 4 ? 1 : n < 6 ? 2 : 3;
            if ((x + y * 2) % 11 == 0) c = 5;
            if (y < 3) c = 4;
            b.set(x, y, c);
        }
    }
    return b;
}

gs::Bitmap foamArt() {
    gs::Bitmap b(16, 10);
    b.ellipse(8, 5, 6, 3, 1);
    b.ellipse(5, 5, 2, 1.4f, 2);
    b.ellipse(11, 4, 2, 1.2f, 2);
    return b;
}

gs::Bitmap sluiceArt() {
    gs::Bitmap b(8, 20);
    b.rect(3, 0, 2, 20, 1);
    b.rect(1, 4, 2, 8, 2);
    b.rect(5, 8, 2, 8, 2);
    return b;
}

gs::Bitmap gaugeArt() {
    gs::Bitmap b(10, 48);
    b.rect(3, 0, 4, 48, 1);
    for (int y = 2; y < 46; y += 6) b.rect(1, y, 8, 2, 2);
    b.rect(0, 2, 10, 2, 3);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(28, 28);
    b.ellipse(14, 14, 8, 8, 1);
    b.ellipse(14, 14, 4, 4, 2);
    for (int i = 0; i < 8; i++) {
        float a = i * 0.785398f;
        float c = std::cos(a), s = std::sin(a);
        b.line(14 + c * 9.f, 14 + s * 9.f, 14 + c * 13.f, 14 + s * 13.f, 1, 1.5f);
    }
    return b;
}

gs::Bitmap cloudArt() {
    gs::Bitmap b(40, 16);
    b.ellipse(14, 9, 10, 5, 1);
    b.ellipse(24, 8, 9, 5, 1);
    b.ellipse(20, 7, 6, 4, 2);
    return b;
}

gs::Bitmap barArt() {
    gs::Bitmap b(16, 10);
    b.rect(0, 2, 16, 6, 1);
    b.rect(0, 1, 16, 1, 2);
    b.rect(0, 8, 16, 1, 2);
    return b;
}

gs::Bitmap puckArt() {
    gs::Bitmap b(22, 12);
    b.poly({{2.f, 3.f}, {18.f, 2.f}, {20.f, 6.f}, {16.f, 10.f}, {4.f, 10.f}}, 3);
    b.rect(8, 4, 6, 4, 4);
    b.outline(2, false);
    return b;
}

void loadFont(gs::VDP& vdp, Art& art) {
    gs::TileAlloc tiles(vdp);
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (!g[y * 5 + x]) continue;
                px[y * 8 + x + 1] = 1;
                if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
            }
        }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        art.font[c - 32] = t;
    }
}

gs::Mipped words(gs::VDP& vdp, const char* text, int scale, int fill, int edge) {
    gs::TextStyle st{scale, fill, edge, 15, 1};
    return gs::uploadMipped(vdp, gs::textBitmap(text, st));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(8, 10, 11), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BOAT, {0, gs::rgb4(14, 13, 10), gs::rgb4(2, 6, 4), gs::rgb4(1, 3, 2), gs::rgb4(12, 2, 2),
                           gs::rgb4(11, 7, 3), gs::rgb4(5, 11, 14), gs::rgb4(2, 2, 2), gs::rgb4(15, 15, 14),
                           gs::rgb4(14, 11, 2), gs::rgb4(8, 5, 2), gs::rgb4(12, 14, 14), gs::rgb4(6, 3, 2),
                           gs::rgb4(2, 5, 8), gs::rgb4(7, 12, 8), ink});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(8, 8, 8), gs::rgb4(5, 5, 6), gs::rgb4(11, 10, 9), gs::rgb4(4, 7, 3),
                            gs::rgb4(12, 11, 9), gs::rgb4(3, 4, 5), gs::rgb4(9, 9, 8), 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_GATE, {0, gs::rgb4(9, 6, 3), gs::rgb4(6, 4, 2), gs::rgb4(12, 9, 5), gs::rgb4(4, 4, 5),
                           gs::rgb4(3, 3, 4), gs::rgb4(13, 12, 8), 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_GRASS, {0, gs::rgb4(3, 8, 3), gs::rgb4(6, 12, 4), gs::rgb4(2, 5, 2), gs::rgb4(7, 5, 3),
                            gs::rgb4(14, 12, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_HOUSE, {0, gs::rgb4(13, 12, 9), gs::rgb4(10, 3, 2), gs::rgb4(4, 3, 2), gs::rgb4(6, 12, 14),
                            gs::rgb4(5, 5, 6), gs::rgb4(8, 6, 3), 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_WATER, {0, gs::rgb4(1, 4, 6), gs::rgb4(2, 6, 8), gs::rgb4(3, 8, 9), gs::rgb4(10, 14, 13),
                            gs::rgb4(6, 12, 11), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(14, 15, 15), gs::rgb4(9, 13, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(2, 7, 2), gs::rgb4(4, 10, 3), gs::rgb4(6, 4, 2), gs::rgb4(3, 8, 4),
                           gs::rgb4(1, 3, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 6), gs::rgb4(15, 15, 14), gs::rgb4(14, 12, 3), gs::rgb4(1, 4, 2),
                          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 5, 3), gs::rgb4(4, 1, 1), gs::rgb4(15, 13, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 13, 7), gs::rgb4(3, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_GULL, {0, gs::rgb4(15, 15, 15), gs::rgb4(15, 8, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FENDER, {0, gs::rgb4(3, 3, 4), gs::rgb4(12, 3, 2), gs::rgb4(14, 12, 4), gs::rgb4(8, 14, 6),
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_SUN, {0, gs::rgb4(15, 13, 4), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});

    loadFont(vdp, art);
    art.boat = gs::uploadMipped(vdp, boatArt());
    art.gate = gs::uploadMipped(vdp, gateArt());
    art.stone = gs::uploadMipped(vdp, stoneArt());
    art.grass = gs::uploadMipped(vdp, grassArt());
    art.house = gs::uploadMipped(vdp, houseArt());
    art.tree[0] = gs::uploadMipped(vdp, treeArt(false));
    art.tree[1] = gs::uploadMipped(vdp, treeArt(true));
    art.post = gs::uploadMipped(vdp, postArt());
    art.gull[0] = gs::uploadMipped(vdp, gullArt(true));
    art.gull[1] = gs::uploadMipped(vdp, gullArt(false));
    art.water = gs::uploadMipped(vdp, waterArt());
    art.foam = gs::uploadMipped(vdp, foamArt());
    art.sluice = gs::uploadMipped(vdp, sluiceArt());
    art.gauge = gs::uploadMipped(vdp, gaugeArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.bar = gs::uploadMipped(vdp, barArt());
    art.puck = gs::uploadMipped(vdp, puckArt());
    art.title = words(vdp, "S3 BARGE", 3, 1, 2);
    art.clear = words(vdp, "CLEAR", 3, 1, 2);
    art.scraped = words(vdp, "SCRAPED", 3, 1, 2);
    art.cill = words(vdp, "THE CILL", 3, 1, 2);
    art.timeUp = words(vdp, "TIME", 3, 1, 2);
    art.paused = words(vdp, "PAUSED", 3, 1, 2);

    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.hudEnabled = true;
}

}  // namespace barge

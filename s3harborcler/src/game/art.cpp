#include "game/art.h"

#include <initializer_list>
#include <string>

namespace hcler {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i >= 16) break;
        vdp.setColor(pal * 16 + i++, c);
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

void courses(gs::Bitmap& b, int x, int y, int w, int h, int brick, int shade, int mortar) {
    b.rect(float(x), float(y), float(w), float(h), brick);
    b.rect(float(x), float(y), 6.f, float(h), shade);
    for (int row = y; row < y + h; row += 6) {
        b.rect(float(x), float(row), float(w), 1.f, mortar);
        int shift = ((row - y) / 6) & 1 ? 8 : 2;
        for (int col = x + shift; col < x + w; col += 16) b.rect(float(col), float(row), 1.f, 6.f, mortar);
    }
}

gs::Bitmap clerkArt(int step) {
    gs::Bitmap b(40, 64);
    b.ellipse(20, 7, 10, 4, 5);
    b.rect(11, 7, 18, 4, 5);
    b.ellipse(20, 16, 7, 7, 4);
    b.rect(15, 13, 10, 3, 8);
    b.set(17, 16, 9);
    b.set(23, 16, 9);
    b.rect(17, 20, 6, 2, 10);
    b.poly({{10, 24}, {30, 24}, {33, 44}, {7, 44}}, 1);
    b.poly({{10, 24}, {20, 24}, {17, 44}, {7, 44}}, 2);
    b.rect(16, 26, 8, 8, 7);
    b.line(29, 30, 36, 42, 6, 2.2f);
    b.rect(33, 40, 5, 3, 6);
    if (step == 0) {
        b.rect(13, 44, 6, 13, 3);
        b.rect(22, 44, 6, 11, 3);
        b.rect(11, 55, 9, 4, 6);
        b.rect(21, 53, 9, 4, 6);
    } else {
        b.rect(13, 44, 6, 11, 3);
        b.rect(22, 44, 6, 13, 3);
        b.rect(12, 53, 9, 4, 6);
        b.rect(20, 55, 9, 4, 6);
    }
    b.outline(9, false);
    return b;
}

gs::Bitmap barrowArt() {
    gs::Bitmap b(36, 36);
    b.line(6, 4, 10, 24, 5, 2.2f);
    b.line(18, 2, 20, 22, 5, 2.2f);
    b.line(6, 8, 18, 6, 3, 2.f);
    b.rect(8, 22, 18, 4, 5);
    b.ellipse(14, 30, 5, 5, 6);
    b.ellipse(26, 29, 5, 5, 6);
    b.ellipse(14, 30, 2, 2, 3);
    b.ellipse(26, 29, 2, 2, 3);
    b.outline(8, false);
    return b;
}

gs::Bitmap crateArt() {
    gs::Bitmap b(36, 32);
    b.poly({{4, 10}, {30, 6}, {33, 24}, {6, 28}}, 1);
    b.poly({{4, 10}, {16, 8}, {16, 26}, {6, 28}}, 2);
    b.line(5, 16, 31, 12, 4, 1.5f);
    b.line(6, 22, 32, 18, 4, 1.5f);
    b.line(16, 8, 17, 26, 3, 1.5f);
    b.rect(20, 3, 10, 4, 6);
    b.outline(5, false);
    return b;
}

gs::Bitmap barrelArt() {
    gs::Bitmap b(28, 36);
    b.ellipse(14, 8, 10, 4, 3);
    b.rect(4, 8, 20, 20, 1);
    b.ellipse(14, 28, 10, 4, 2);
    b.rect(4, 12, 20, 3, 4);
    b.rect(4, 22, 20, 3, 4);
    b.rect(4, 8, 3, 20, 2);
    b.outline(8, false);
    return b;
}

gs::Bitmap netArt() {
    gs::Bitmap b(40, 28);
    b.ellipse(20, 16, 16, 10, 1);
    b.ellipse(16, 14, 8, 5, 2);
    for (int y = 8; y < 24; y += 4)
        for (int x = 8; x < 34; x += 5) b.set(x, y, 3);
    b.rect(17, 4, 6, 4, 5);
    b.outline(4, false);
    return b;
}

gs::Bitmap coilArt() {
    gs::Bitmap b(32, 22);
    b.ellipse(16, 12, 13, 8, 1);
    b.ellipse(16, 12, 7, 4, 2);
    b.ellipse(16, 12, 3, 2, 3);
    b.line(4, 8, 10, 6, 4, 1.4f);
    b.outline(5, false);
    return b;
}

gs::Bitmap boatArt() {
    gs::Bitmap b(110, 52);
    b.poly({{8, 18}, {100, 14}, {96, 36}, {14, 40}}, 1);
    b.poly({{8, 18}, {40, 16}, {36, 38}, {14, 40}}, 2);
    b.rect(22, 8, 8, 14, 4);
    b.rect(48, 10, 36, 10, 3);
    b.rect(50, 12, 14, 6, 7);
    b.rect(86, 20, 10, 8, 6);
    gs::Bitmap label = gs::textBitmap("S3", {2, 2, 0, 0, 1});
    b.blit(label, 52, 22);
    b.outline(9, false);
    return b;
}

gs::Bitmap cargoArt() {
    gs::Bitmap b(20, 14);
    b.rect(2, 3, 12, 9, 1);
    b.rect(3, 4, 10, 3, 2);
    b.ellipse(15, 9, 4, 4, 3);
    return b;
}

gs::Bitmap shedArt() {
    gs::Bitmap b(280, 72);
    b.rect(0, 8, 280, 6, 6);
    courses(b, 0, 14, 280, 46, 1, 2, 3);
    b.rect(0, 58, 280, 12, 7);
    auto bay = [&](int x) {
        b.rect(float(x), 22, 28, 36, 8);
        b.rect(float(x + 2), 24, 24, 16, 4);
        b.rect(float(x + 12), 24, 2, 32, 6);
    };
    bay(16);
    bay(210);
    b.rect(96, 28, 70, 40, 8);
    gs::Bitmap sign = gs::textBitmap("HARBOR", {2, 5, 0, 0, 1});
    b.rect(100, 16, float(sign.w + 8), float(sign.h + 4), 8);
    b.blit(sign, 104, 18);
    return b;
}

gs::Bitmap clockArt() {
    gs::Bitmap b(34, 34);
    b.ellipse(17, 17, 15, 15, 3);
    b.ellipse(17, 17, 12, 12, 1);
    b.rect(16, 7, 2, 10, 3);
    b.rect(16, 16, 8, 2, 4);
    b.rect(14, 1, 6, 4, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(16, 40);
    b.rect(7, 0, 2, 12, 3);
    b.poly({{2, 12}, {14, 12}, {15, 22}, {1, 22}}, 1);
    b.ellipse(8, 16, 4, 3, 2);
    b.rect(6, 22, 4, 14, 3);
    b.rect(3, 34, 10, 3, 4);
    b.outline(4, false);
    return b;
}

gs::Bitmap craneArt() {
    gs::Bitmap b(90, 70);
    b.rect(8, 40, 16, 28, 1);
    b.rect(6, 36, 20, 6, 2);
    b.line(16, 40, 78, 8, 3, 3.f);
    b.line(78, 8, 78, 28, 4, 1.6f);
    b.rect(74, 26, 8, 6, 5);
    b.outline(6, false);
    return b;
}

gs::Bitmap pilingArt() {
    gs::Bitmap b(14, 48);
    b.rect(3, 4, 8, 40, 1);
    b.rect(3, 4, 3, 40, 2);
    b.rect(1, 0, 12, 6, 3);
    b.rect(4, 18, 6, 2, 4);
    b.rect(4, 32, 6, 2, 4);
    return b;
}

gs::Bitmap buoyArt() {
    gs::Bitmap b(22, 30);
    b.ellipse(11, 12, 8, 8, 1);
    b.ellipse(11, 12, 4, 4, 2);
    b.rect(9, 18, 4, 8, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap gullArt() {
    gs::Bitmap b(28, 12);
    b.line(2, 8, 14, 4, 1, 1.6f);
    b.line(14, 4, 26, 8, 1, 1.6f);
    b.ellipse(14, 6, 2, 2, 2);
    return b;
}

gs::Bitmap foamArt() {
    gs::Bitmap b(18, 8);
    b.ellipse(6, 4, 5, 2, 1);
    b.ellipse(12, 4, 4, 2, 2);
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(14, 10);
    b.ellipse(7, 6, 5, 3, 1);
    b.ellipse(5, 5, 2, 2, 2);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(32, 8);
    b.ellipse(16, 4, 13, 2, 1);
    return b;
}

gs::Bitmap markArt() {
    gs::Bitmap b(14, 10);
    b.poly({{1, 1}, {7, 9}, {13, 1}, {10, 1}, {7, 5}, {4, 1}}, 1);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    gs::TextStyle big{3, 1, 0, 0, 1};
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
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(14, 15, 15), gs::rgb4(8, 10, 12), gs::rgb4(3, 4, 6)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 3), gs::rgb4(15, 14, 10), gs::rgb4(5, 4, 2), gs::rgb4(3, 3, 3)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(15, 11, 8), gs::rgb4(6, 2, 2), gs::rgb4(3, 1, 1)});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(7, 15, 8), gs::rgb4(13, 15, 12), gs::rgb4(2, 5, 3), gs::rgb4(1, 2, 1)});
    setPal(vdp, PAL_SHED,
           {0, gs::rgb4(9, 6, 4), gs::rgb4(6, 4, 3), gs::rgb4(12, 10, 8), gs::rgb4(3, 6, 9), gs::rgb4(14, 12, 6),
            gs::rgb4(8, 7, 6), gs::rgb4(3, 3, 4), gs::rgb4(2, 2, 3), gs::rgb4(5, 3, 2)});
    setPal(vdp, PAL_CLERK,
           {0, gs::rgb4(14, 11, 2), gs::rgb4(9, 7, 1), gs::rgb4(2, 3, 5), gs::rgb4(13, 9, 6), gs::rgb4(1, 2, 4),
            gs::rgb4(2, 2, 2), gs::rgb4(14, 13, 10), gs::rgb4(3, 3, 4), gs::rgb4(1, 1, 1), gs::rgb4(8, 2, 2)});
    setPal(vdp, PAL_NET,
           {0, gs::rgb4(8, 10, 6), gs::rgb4(5, 7, 4), gs::rgb4(12, 12, 8), gs::rgb4(3, 4, 3), gs::rgb4(10, 8, 4)});
    setPal(vdp, PAL_WOOD,
           {0, gs::rgb4(12, 8, 3), gs::rgb4(8, 5, 2), gs::rgb4(5, 3, 1), gs::rgb4(11, 11, 12), gs::rgb4(3, 2, 1),
            gs::rgb4(14, 10, 4)});
    setPal(vdp, PAL_STEEL,
           {0, gs::rgb4(6, 8, 9), gs::rgb4(3, 5, 6), gs::rgb4(13, 13, 14), gs::rgb4(14, 10, 2), gs::rgb4(7, 7, 8),
            gs::rgb4(2, 2, 3), gs::rgb4(8, 4, 2), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_FX, {0, gs::rgb4(12, 13, 14), gs::rgb4(9, 11, 13), gs::rgb4(6, 7, 8)});
    setPal(vdp, PAL_NIGHT,
           {0, gs::rgb4(1, 2, 5), gs::rgb4(3, 5, 8), gs::rgb4(8, 9, 12), gs::rgb4(14, 13, 8)});
    setPal(vdp, PAL_BOAT,
           {0, gs::rgb4(11, 4, 2), gs::rgb4(7, 2, 1), gs::rgb4(12, 10, 7), gs::rgb4(2, 2, 3), gs::rgb4(4, 4, 5),
            gs::rgb4(6, 7, 8), gs::rgb4(14, 12, 3), gs::rgb4(4, 6, 9), gs::rgb4(2, 1, 1)});

    const uint16_t quay[16] = {0,
                               gs::rgb4(7, 7, 6), gs::rgb4(5, 5, 5), gs::rgb4(9, 8, 7),
                               gs::rgb4(8, 7, 5), gs::rgb4(6, 6, 5),
                               gs::rgb4(10, 9, 8), gs::rgb4(7, 7, 6),
                               gs::rgb4(11, 10, 8), gs::rgb4(6, 6, 5), gs::rgb4(12, 10, 4),
                               gs::rgb4(4, 5, 6), gs::rgb4(3, 4, 5), gs::rgb4(8, 9, 10),
                               gs::rgb4(13, 11, 3), gs::rgb4(8, 8, 7)};
    const uint16_t water[16] = {0,
                                gs::rgb4(2, 5, 8), gs::rgb4(1, 3, 6), gs::rgb4(3, 6, 9),
                                gs::rgb4(4, 7, 8), gs::rgb4(2, 4, 6),
                                gs::rgb4(3, 6, 9), gs::rgb4(2, 5, 8),
                                gs::rgb4(4, 7, 10), gs::rgb4(2, 4, 7), gs::rgb4(8, 10, 11),
                                gs::rgb4(2, 6, 10), gs::rgb4(1, 4, 8), gs::rgb4(10, 13, 14),
                                gs::rgb4(12, 13, 10), gs::rgb4(3, 5, 7)};
    for (int i = 0; i < 16; i++) {
        vdp.setColor(PAL_QUAY * 16 + i, quay[i]);
        vdp.setColor(PAL_WATER * 16 + i, water[i]);
    }

    loadFont(vdp, art);
    art.clerk[0] = gs::uploadMipped(vdp, clerkArt(0));
    art.clerk[1] = gs::uploadMipped(vdp, clerkArt(1));
    art.barrow = gs::uploadMipped(vdp, barrowArt());
    art.crate = gs::uploadMipped(vdp, crateArt());
    art.barrel = gs::uploadMipped(vdp, barrelArt());
    art.net = gs::uploadMipped(vdp, netArt());
    art.coil = gs::uploadMipped(vdp, coilArt());
    art.boat = gs::uploadMipped(vdp, boatArt());
    art.cargo = gs::uploadMipped(vdp, cargoArt());
    art.shed = gs::uploadMipped(vdp, shedArt());
    art.clock = gs::uploadMipped(vdp, clockArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.crane = gs::uploadMipped(vdp, craneArt());
    art.piling = gs::uploadMipped(vdp, pilingArt());
    art.buoy = gs::uploadMipped(vdp, buoyArt());
    art.gull = gs::uploadMipped(vdp, gullArt());
    art.foam = gs::uploadMipped(vdp, foamArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.mark = gs::uploadMipped(vdp, markArt());

    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.hudEnabled = true;
    vdp.setFogColor(gs::rgb4(3, 5, 8));
}

}  // namespace hcler

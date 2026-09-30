#include "game/art.h"

#include <cmath>
#include <string>

namespace beaconmaga {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap walkerArt() {
    Bitmap b(40, 84);
    b.ellipse(20, 12, 8, 9, 4);
    b.rect(12, 6, 16, 6, 5);
    b.rect(14, 4, 12, 5, 6);
    b.set(16, 13, 10);
    b.set(24, 13, 10);
    b.poly({{8, 28}, {20, 18}, {32, 28}, {30, 52}, {10, 52}}, 1);
    b.poly({{8, 28}, {16, 22}, {18, 50}, {10, 52}}, 2);
    b.rect(6, 30, 6, 16, 3);
    b.rect(28, 30, 6, 16, 2);
    b.rect(12, 50, 7, 22, 8);
    b.rect(22, 50, 7, 22, 8);
    b.rect(11, 70, 9, 6, 9);
    b.rect(21, 70, 9, 6, 9);
    b.rect(30, 36, 6, 4, 7);
    b.outline(15, false);
    return b;
}

Bitmap fallenArt() {
    Bitmap b(80, 28);
    b.ellipse(12, 12, 7, 6, 4);
    b.rect(8, 4, 10, 4, 6);
    b.rect(18, 8, 34, 10, 1);
    b.rect(18, 8, 34, 3, 2);
    b.rect(50, 10, 14, 6, 8);
    b.rect(62, 12, 10, 4, 9);
    b.rect(28, 16, 8, 4, 7);
    b.outline(15, false);
    return b;
}

Bitmap postArt() {
    Bitmap b(16, 72);
    b.rect(6, 8, 4, 64, 2);
    b.rect(7, 8, 2, 64, 1);
    b.rect(2, 4, 12, 6, 4);
    b.rect(4, 0, 8, 5, 6);
    b.rect(6, 20, 4, 3, 3);
    b.rect(6, 40, 4, 3, 3);
    return b;
}

Bitmap rockArt() {
    Bitmap b(48, 28);
    b.poly({{4, 22}, {14, 8}, {28, 4}, {44, 16}, {40, 26}, {8, 26}}, 2);
    b.poly({{14, 8}, {28, 4}, {26, 16}, {12, 18}}, 1);
    b.rect(18, 14, 8, 4, 3);
    b.outline(15, false);
    return b;
}

Bitmap cageArt() {
    Bitmap b(56, 40);
    b.rect(8, 8, 40, 24, 2);
    b.rect(12, 12, 32, 16, 6);
    b.rect(18, 14, 20, 12, 7);
    b.rect(8, 8, 40, 3, 4);
    b.rect(8, 29, 40, 3, 3);
    for (int x = 12; x < 46; x += 8) b.rect(x, 8, 2, 24, 1);
    b.rect(24, 32, 8, 6, 2);
    b.rect(20, 2, 16, 6, 5);
    return b;
}

Bitmap glowArt() {
    Bitmap b(32, 32);
    b.ellipse(16, 16, 14, 12, 3);
    b.ellipse(16, 16, 7, 6, 4);
    b.ellipse(16, 16, 3, 3, 5);
    return b;
}

Bitmap beamArt() {
    Bitmap b(20, 48);
    b.poly({{8, 46}, {12, 46}, {18, 2}, {2, 2}}, 2);
    b.poly({{9, 46}, {11, 46}, {14, 6}, {6, 6}}, 3);
    return b;
}

Bitmap rifleArt() {
    Bitmap b(28, 64);
    b.rect(10, 22, 8, 36, 2);
    b.rect(12, 24, 4, 20, 1);
    b.rect(11, 4, 6, 22, 4);
    b.rect(13, 2, 2, 5, 5);
    b.rect(6, 20, 16, 8, 3);
    b.poly({{10, 56}, {18, 56}, {22, 62}, {6, 62}}, 2);
    return b;
}

Bitmap sightArt() {
    Bitmap b(28, 28);
    const float cx = 14, cy = 14;
    for (int i = 0; i < 40; i++) {
        if (i % 10 < 2) continue;
        float a = i * 6.2831853f / 40.f;
        b.set(int(cx + std::cos(a) * 10), int(cy + std::sin(a) * 10), 4);
    }
    b.rect(13, 1, 2, 4, 4);
    b.rect(13, 23, 2, 4, 4);
    b.rect(1, 13, 4, 2, 4);
    b.rect(23, 13, 4, 2, 4);
    b.set(14, 14, 5);
    return b;
}

Bitmap roundArt() {
    Bitmap b(10, 22);
    b.rect(2, 2, 6, 16, 2);
    b.rect(3, 3, 4, 8, 1);
    b.rect(3, 3, 4, 3, 3);
    b.rect(2, 16, 6, 3, 10);
    return b;
}

Bitmap spentArt() {
    Bitmap b(10, 22);
    b.rect(3, 4, 4, 10, 9);
    b.rect(3, 14, 4, 3, 10);
    return b;
}

Bitmap flashArt() {
    Bitmap b(20, 20);
    b.ellipse(10, 10, 8, 6, 6);
    b.ellipse(10, 10, 3, 2, 5);
    b.rect(9, 1, 2, 18, 5);
    b.rect(1, 9, 18, 2, 5);
    return b;
}

Bitmap sparkArt() {
    Bitmap b(16, 16);
    b.ellipse(8, 8, 6, 5, 7);
    b.ellipse(8, 8, 3, 2, 8);
    return b;
}

Bitmap barArt() {
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
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    const uint16_t ink = gs::rgb4(14, 13, 11);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(7, 7, 8), gs::rgb4(15, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 11, 3), gs::rgb4(10, 6, 2), gs::rgb4(15, 15, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 4, 3), gs::rgb4(8, 2, 2), gs::rgb4(15, 10, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(6, 15, 8), gs::rgb4(2, 8, 4), gs::rgb4(12, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    auto coat = [&](int pal, uint16_t hi, uint16_t mid) {
        setPal(vdp, pal,
               {0, hi, mid, gs::rgb4(8, 7, 6), gs::rgb4(2, 3, 5), gs::rgb4(3, 2, 2), gs::rgb4(12, 10, 6),
                gs::rgb4(6, 5, 3), gs::rgb4(3, 3, 4), gs::rgb4(2, 2, 2), gs::rgb4(1, 1, 1), 0, 0, 0, 0, shadow});
    };
    coat(PAL_COAT, gs::rgb4(4, 6, 8), gs::rgb4(2, 3, 5));
    coat(PAL_COATB, gs::rgb4(8, 4, 3), gs::rgb4(5, 2, 2));

    setPal(vdp, PAL_STONE,
           {0, gs::rgb4(7, 7, 8), gs::rgb4(4, 4, 5), gs::rgb4(5, 5, 6), gs::rgb4(9, 8, 6), gs::rgb4(3, 3, 4),
            gs::rgb4(14, 12, 6), gs::rgb4(2, 2, 3), 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_IRON,
           {0, gs::rgb4(6, 7, 8), gs::rgb4(3, 4, 5), gs::rgb4(8, 8, 7), gs::rgb4(2, 2, 3), gs::rgb4(10, 8, 4),
            0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FX,
           {0, gs::rgb4(14, 10, 3), gs::rgb4(8, 6, 2), gs::rgb4(15, 12, 4), gs::rgb4(15, 14, 7), gs::rgb4(15, 15, 12),
            gs::rgb4(15, 8, 2), gs::rgb4(10, 9, 6), gs::rgb4(14, 12, 8), gs::rgb4(4, 4, 5), gs::rgb4(2, 2, 3), 0, 0, 0, 0,
            shadow});
    setPal(vdp, PAL_LAMP,
           {0, gs::rgb4(5, 6, 7), gs::rgb4(3, 3, 4), gs::rgb4(12, 8, 2), gs::rgb4(15, 12, 4), gs::rgb4(15, 15, 9),
            gs::rgb4(14, 10, 3), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, shadow});

    const uint16_t road[16] = {
        0,
        gs::rgb4(1, 2, 4), gs::rgb4(1, 1, 3), gs::rgb4(2, 3, 5),
        gs::rgb4(3, 3, 4), gs::rgb4(2, 2, 3),
        gs::rgb4(4, 4, 5), gs::rgb4(3, 3, 4),
        gs::rgb4(5, 5, 4), gs::rgb4(2, 2, 3), gs::rgb4(6, 6, 5),
        gs::rgb4(1, 3, 6), gs::rgb4(1, 2, 5), gs::rgb4(3, 6, 9),
        gs::rgb4(12, 10, 4), gs::rgb4(5, 5, 6),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_ROAD * 16 + i, road[i]);

    loadFont(vdp, art);
    art.walker = gs::uploadMipped(vdp, walkerArt());
    art.fallen = gs::uploadMipped(vdp, fallenArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.rock = gs::uploadMipped(vdp, rockArt());
    art.cage = gs::uploadMipped(vdp, cageArt());
    art.glow = gs::uploadMipped(vdp, glowArt());
    art.beam = gs::uploadMipped(vdp, beamArt());
    art.rifle = gs::uploadMipped(vdp, rifleArt());
    art.sight = gs::uploadMipped(vdp, sightArt());
    art.round = gs::uploadMipped(vdp, roundArt());
    art.spent = gs::uploadMipped(vdp, spentArt());
    art.flash = gs::uploadMipped(vdp, flashArt());
    art.spark = gs::uploadMipped(vdp, sparkArt());
    art.bar = gs::uploadMipped(vdp, barArt());
}

}  // namespace beaconmaga

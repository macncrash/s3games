#include "game/art.h"

#include <cmath>
#include <initializer_list>
#include <string>

namespace pace {
namespace {

constexpr float TAU = 6.2831853f;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

gs::Bitmap walker(int step) {
    gs::Bitmap b(48, 84);
    b.ellipse(24, 13, 11, 6, 5);
    b.rect(11, 15, 26, 4, 5);
    b.ellipse(24, 25, 7, 8, 4);
    b.set(21, 25, 8);
    b.set(27, 25, 8);
    b.rect(20, 29, 8, 2, 9);
    b.rect(18, 33, 12, 3, 9);
    b.poly({{12, 36}, {36, 36}, {40, 64}, {8, 64}}, 2);
    b.poly({{12, 36}, {24, 36}, {22, 64}, {8, 64}}, 1);
    b.set(23, 44, 9);
    b.set(23, 50, 9);
    b.set(23, 56, 9);
    b.line(16, 40, 8, 52, 10, 3.f);
    b.ellipse(8, 56, 5, 6, 6);
    b.ellipse(8, 55, 2.5f, 3, 7);
    b.line(34, 40, 39, 54, 10, 3.f);
    if (step == 0) {
        b.rect(14, 62, 7, 14, 3);
        b.rect(26, 62, 7, 11, 11);
        b.rect(12, 74, 11, 5, 8);
        b.rect(25, 71, 9, 5, 8);
    } else {
        b.rect(14, 62, 7, 11, 11);
        b.rect(26, 62, 7, 14, 3);
        b.rect(13, 71, 9, 5, 8);
        b.rect(24, 74, 11, 5, 8);
    }
    b.outline(8, false);
    return b;
}

gs::Bitmap fallenBody() {
    gs::Bitmap b(92, 40);
    b.ellipse(16, 22, 8, 7, 4);
    b.ellipse(16, 14, 10, 5, 5);
    b.poly({{24, 15}, {76, 18}, {80, 30}, {22, 28}}, 2);
    b.poly({{24, 15}, {48, 16}, {50, 28}, {22, 26}}, 1);
    b.rect(68, 24, 14, 6, 8);
    b.ellipse(42, 11, 5, 4, 6);
    b.ellipse(42, 11, 2, 2, 7);
    b.outline(8, false);
    return b;
}

gs::Bitmap pierArt() {
    gs::Bitmap b(40, 128);
    b.rect(4, 10, 32, 118, 2);
    b.rect(22, 12, 10, 112, 1);
    for (int y = 18; y < 118; y += 14) {
        b.rect(4, y, 32, 2, 4);
        b.rect(10, y + 4, 2, 8, 3);
        b.rect(26, y + 5, 2, 6, 3);
    }
    b.rect(1, 4, 38, 8, 1);
    b.rect(0, 0, 40, 5, 2);
    b.rect(15, 46, 8, 20, 6);
    b.rect(17, 48, 4, 16, 7);
    b.rect(6, 96, 7, 12, 5);
    b.outline(6, false);
    return b;
}

gs::Bitmap blockArt() {
    gs::Bitmap b(36, 24);
    b.rect(1, 2, 34, 20, 2);
    b.rect(2, 3, 14, 18, 1);
    b.rect(1, 2, 34, 3, 1);
    b.rect(1, 11, 34, 2, 4);
    b.outline(6, false);
    return b;
}

gs::Bitmap keyArt() {
    gs::Bitmap b(28, 36);
    b.poly({{6, 6}, {22, 6}, {26, 34}, {2, 34}}, 2);
    b.poly({{8, 8}, {15, 8}, {14, 32}, {4, 32}}, 1);
    b.rect(12, 16, 4, 8, 6);
    b.outline(6, false);
    return b;
}

gs::Bitmap bollardArt() {
    gs::Bitmap b(16, 40);
    b.rect(4, 8, 8, 26, 1);
    b.rect(3, 4, 10, 6, 2);
    b.rect(4, 18, 8, 3, 2);
    b.rect(3, 32, 10, 5, 3);
    b.outline(3, false);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(48, 72);
    b.rect(21, 40, 7, 28, 3);
    b.rect(23, 42, 3, 22, 4);
    b.ellipse(24, 28, 18, 16, 2);
    b.ellipse(16, 24, 10, 9, 1);
    b.ellipse(32, 30, 9, 8, 1);
    b.outline(4, false);
    return b;
}

gs::Bitmap beadArt() {
    gs::Bitmap b(24, 24);
    for (int i = 0; i < 24; i++) {
        if ((i % 6) < 2) continue;
        float a = i * TAU / 24.f;
        int x = int(std::lround(12 + std::cos(a) * 8));
        int y = int(std::lround(12 + std::sin(a) * 8));
        b.set(x, y, 2);
        b.set(x, y + 1, 1);
    }
    b.rect(11, 3, 2, 4, 1);
    b.rect(11, 17, 2, 4, 1);
    b.rect(3, 11, 4, 2, 1);
    b.rect(17, 11, 4, 2, 1);
    b.rect(11, 11, 2, 2, 1);
    return b;
}

gs::Bitmap barrelArt() {
    gs::Bitmap b(22, 64);
    b.rect(9, 0, 4, 36, 1);
    b.rect(8, 2, 2, 32, 2);
    b.rect(7, 32, 8, 6, 3);
    b.rect(6, 38, 10, 18, 4);
    b.rect(7, 40, 4, 14, 2);
    b.rect(5, 54, 12, 8, 3);
    b.outline(3, false);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(14, 22);
    b.rect(6, 0, 2, 6, 3);
    b.rect(3, 6, 8, 8, 1);
    b.ellipse(7, 10, 3, 3, 2);
    b.rect(4, 14, 6, 6, 3);
    b.outline(3, false);
    return b;
}

gs::Bitmap pipArt() {
    gs::Bitmap b(14, 14);
    b.ellipse(7, 7, 5, 5, 1);
    b.ellipse(7, 7, 2, 2, 2);
    b.outline(3, false);
    return b;
}

gs::Bitmap flashArt() {
    gs::Bitmap b(20, 20);
    b.ellipse(10, 10, 8, 6, 2);
    b.ellipse(10, 10, 4, 3, 1);
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(28, 14);
    b.ellipse(14, 8, 12, 5, 3);
    b.ellipse(10, 7, 5, 3, 4);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(32, 12);
    b.ellipse(16, 6, 14, 4, 1);
    return b;
}

gs::Bitmap stripeArt() {
    gs::Bitmap b(16, 8);
    b.rect(0, 1, 16, 6, 1);
    b.rect(0, 3, 16, 2, 2);
    return b;
}

gs::Bitmap moonArt() {
    gs::Bitmap b(22, 22);
    b.ellipse(11, 11, 8, 8, 1);
    b.ellipse(14, 10, 6, 6, 2);
    return b;
}

gs::Bitmap cloudArt() {
    gs::Bitmap b(64, 24);
    b.ellipse(18, 14, 14, 7, 3);
    b.ellipse(34, 12, 16, 8, 3);
    b.ellipse(48, 14, 12, 6, 4);
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
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(15, 15, 15), gs::rgb4(11, 11, 13), gs::rgb4(4, 4, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 3), gs::rgb4(15, 15, 12), gs::rgb4(3, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(15, 12, 10), gs::rgb4(4, 0, 0), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(8, 15, 6), gs::rgb4(14, 15, 12), gs::rgb4(1, 4, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(10, 9, 8), gs::rgb4(7, 6, 6), gs::rgb4(4, 4, 5), gs::rgb4(5, 5, 4), gs::rgb4(3, 5, 2),
                            gs::rgb4(2, 2, 3), gs::rgb4(14, 8, 2), gs::rgb4(6, 5, 4), 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FIGURE, {0, gs::rgb4(12, 3, 3), gs::rgb4(8, 1, 2), gs::rgb4(4, 0, 1), gs::rgb4(14, 10, 8), gs::rgb4(2, 2, 3),
                             gs::rgb4(15, 11, 2), gs::rgb4(15, 15, 12), gs::rgb4(1, 1, 1), gs::rgb4(11, 10, 8), gs::rgb4(6, 5, 4),
                             gs::rgb4(6, 0, 1), 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_HOLD, {0, gs::rgb4(15, 10, 2), gs::rgb4(8, 5, 2), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_LIVE, {0, gs::rgb4(13, 15, 10), gs::rgb4(6, 12, 7), gs::rgb4(1, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(5, 9, 3), gs::rgb4(2, 5, 2), gs::rgb4(5, 3, 2), gs::rgb4(3, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 15, 12), gs::rgb4(15, 8, 2), gs::rgb4(8, 7, 6), gs::rgb4(12, 11, 9), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_NIGHT, {0, gs::rgb4(14, 14, 11), gs::rgb4(8, 8, 12), gs::rgb4(7, 7, 10), gs::rgb4(12, 12, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_METAL, {0, gs::rgb4(10, 10, 12), gs::rgb4(6, 6, 8), gs::rgb4(3, 3, 4), gs::rgb4(6, 4, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    const uint16_t road[16] = {
        0,
        gs::rgb4(2, 4, 2), gs::rgb4(1, 3, 1), gs::rgb4(3, 5, 2),
        gs::rgb4(4, 4, 2), gs::rgb4(3, 3, 2),
        gs::rgb4(5, 5, 6), gs::rgb4(3, 3, 4),
        gs::rgb4(6, 6, 5), gs::rgb4(4, 4, 5), gs::rgb4(7, 7, 6),
        gs::rgb4(2, 3, 6), gs::rgb4(1, 2, 5), gs::rgb4(3, 4, 7),
        gs::rgb4(12, 10, 6), gs::rgb4(7, 7, 8),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_ROAD * 16 + i, road[i]);

    loadFont(vdp, art);
    art.walk[0] = gs::uploadMipped(vdp, walker(0));
    art.walk[1] = gs::uploadMipped(vdp, walker(1));
    art.fallen = gs::uploadMipped(vdp, fallenBody());
    art.pier = gs::uploadMipped(vdp, pierArt());
    art.block = gs::uploadMipped(vdp, blockArt());
    art.keystone = gs::uploadMipped(vdp, keyArt());
    art.bollard = gs::uploadMipped(vdp, bollardArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.bead = gs::uploadMipped(vdp, beadArt());
    art.barrel = gs::uploadMipped(vdp, barrelArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.pip = gs::uploadMipped(vdp, pipArt());
    art.flash = gs::uploadMipped(vdp, flashArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.stripe = gs::uploadMipped(vdp, stripeArt());
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());

    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.hudEnabled = true;
    vdp.setFogColor(gs::rgb4(6, 4, 6));
}

}  // namespace pace

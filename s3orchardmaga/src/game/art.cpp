#include "game/art.h"

#include <cmath>
#include <string>

namespace orchardmaga {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap manStand() {
    Bitmap b(48, 88);
    b.rect(16, 30, 16, 32, 1);
    b.rect(16, 30, 6, 32, 2);
    b.ellipse(24, 16, 7, 8, 4);
    b.ellipse(24, 18, 5, 6, 3);
    b.rect(18, 10, 12, 4, 5);
    b.set(21, 17, 10);
    b.set(27, 17, 10);
    b.rect(12, 34, 5, 16, 3);
    b.rect(31, 34, 5, 16, 2);
    b.rect(17, 60, 6, 18, 8);
    b.rect(26, 60, 6, 18, 8);
    b.rect(16, 76, 8, 6, 9);
    b.rect(25, 76, 8, 6, 9);
    b.line(14, 52, 40, 28, 6, 2.4f);
    b.rect(36, 24, 6, 4, 6);
    b.outline(15, false);
    return b;
}

Bitmap manCrouch() {
    Bitmap b(52, 56);
    b.ellipse(26, 14, 7, 7, 4);
    b.ellipse(26, 16, 5, 5, 3);
    b.rect(20, 8, 12, 4, 5);
    b.set(23, 15, 10);
    b.set(29, 15, 10);
    b.poly({{10, 32}, {26, 20}, {42, 32}, {40, 44}, {12, 46}}, 1);
    b.poly({{10, 32}, {20, 24}, {22, 44}, {12, 46}}, 2);
    b.rect(12, 42, 12, 7, 8);
    b.rect(28, 42, 12, 7, 8);
    b.rect(10, 48, 12, 5, 9);
    b.rect(30, 48, 12, 5, 9);
    b.line(8, 36, 42, 26, 6, 2.2f);
    b.outline(15, false);
    return b;
}

Bitmap manFallen() {
    Bitmap b(84, 30);
    b.ellipse(14, 14, 7, 6, 4);
    b.ellipse(15, 15, 4, 4, 3);
    b.rect(20, 10, 36, 10, 1);
    b.rect(20, 10, 36, 3, 2);
    b.rect(54, 12, 12, 7, 8);
    b.rect(64, 14, 10, 5, 9);
    b.line(28, 22, 58, 26, 6, 2.0f);
    b.outline(15, false);
    return b;
}

Bitmap treeArt() {
    Bitmap b(64, 110);
    b.rect(28, 48, 8, 62, 3);
    b.rect(30, 50, 3, 56, 4);
    b.ellipse(32, 36, 26, 28, 1);
    b.ellipse(24, 30, 12, 12, 2);
    b.ellipse(42, 34, 10, 10, 2);
    b.ellipse(18, 40, 4, 4, 6);
    b.ellipse(46, 28, 4, 4, 6);
    b.ellipse(34, 18, 3, 3, 6);
    b.ellipse(22, 22, 3, 3, 5);
    return b;
}

Bitmap crateArt() {
    Bitmap b(40, 32);
    b.rect(2, 6, 36, 22, 2);
    b.rect(4, 8, 32, 4, 1);
    b.rect(2, 6, 36, 3, 4);
    for (int i = 0; i < 3; i++) b.ellipse(10 + i * 10, 16, 4, 4, 6);
    b.rect(6, 22, 28, 3, 3);
    b.outline(15, false);
    return b;
}

Bitmap ladderArt() {
    Bitmap b(24, 78);
    b.rect(2, 0, 3, 78, 2);
    b.rect(18, 0, 3, 78, 1);
    for (int y = 6; y < 74; y += 12) b.rect(2, y, 19, 2, 3);
    return b;
}

Bitmap lampArt() {
    Bitmap b(18, 28);
    b.rect(8, 0, 2, 8, 2);
    b.poly({{4, 10}, {14, 10}, {16, 22}, {2, 22}}, 6);
    b.poly({{6, 12}, {12, 12}, {13, 20}, {5, 20}}, 7);
    b.rect(3, 22, 12, 3, 4);
    return b;
}

Bitmap glowArt() {
    Bitmap b(28, 28);
    b.ellipse(14, 14, 12, 10, 3);
    b.ellipse(14, 14, 6, 5, 4);
    return b;
}

Bitmap rifleArt() {
    Bitmap b(32, 72);
    b.rect(12, 26, 8, 40, 2);
    b.rect(14, 28, 4, 24, 1);
    b.rect(13, 4, 6, 26, 4);
    b.rect(15, 2, 2, 6, 5);
    b.rect(8, 24, 16, 9, 3);
    b.rect(6, 44, 6, 14, 1);
    b.poly({{12, 62}, {20, 62}, {24, 70}, {8, 70}}, 2);
    return b;
}

Bitmap sightArt() {
    Bitmap b(36, 36);
    const float cx = 18, cy = 18;
    for (int i = 0; i < 48; i++) {
        if (i % 12 < 2) continue;
        float a = i * 6.2831853f / 48.f;
        b.set(int(std::lround(cx + std::cos(a) * 12)), int(std::lround(cy + std::sin(a) * 12)), 4);
    }
    b.rect(17, 2, 2, 5, 4);
    b.rect(17, 29, 2, 5, 4);
    b.rect(2, 17, 5, 2, 4);
    b.rect(29, 17, 5, 2, 4);
    b.set(18, 18, 5);
    return b;
}

Bitmap roundArt() {
    Bitmap b(10, 24);
    b.rect(2, 2, 6, 18, 2);
    b.rect(3, 3, 4, 10, 1);
    b.rect(3, 3, 4, 4, 3);
    b.rect(2, 18, 6, 3, 10);
    return b;
}

Bitmap spentArt() {
    Bitmap b(10, 24);
    b.rect(3, 4, 4, 12, 9);
    b.rect(3, 16, 4, 3, 10);
    return b;
}

Bitmap flashArt() {
    Bitmap b(22, 22);
    b.ellipse(11, 11, 9, 7, 6);
    b.ellipse(11, 11, 4, 3, 5);
    b.rect(10, 2, 2, 18, 5);
    b.rect(2, 10, 18, 2, 5);
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

Bitmap appleArt() {
    Bitmap b(16, 16);
    b.ellipse(8, 9, 6, 6, 1);
    b.ellipse(6, 8, 2, 2, 2);
    b.rect(7, 2, 2, 4, 3);
    b.ellipse(11, 3, 3, 2, 4);
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
    const uint16_t shadow = gs::rgb4(1, 2, 1);
    const uint16_t ink = gs::rgb4(15, 14, 10);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(7, 8, 5), gs::rgb4(15, 15, 13), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 3), gs::rgb4(10, 7, 1), gs::rgb4(15, 15, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 3, 2), gs::rgb4(8, 1, 1), gs::rgb4(15, 9, 7), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(6, 14, 4), gs::rgb4(2, 7, 2), gs::rgb4(12, 15, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    auto coat = [&](int pal, uint16_t hi, uint16_t mid) {
        setPal(vdp, pal,
               {0, hi, mid, gs::rgb4(12, 8, 5), gs::rgb4(3, 3, 3), gs::rgb4(5, 3, 2), gs::rgb4(8, 8, 7),
                gs::rgb4(6, 4, 2), gs::rgb4(3, 4, 3), gs::rgb4(2, 2, 1), gs::rgb4(1, 1, 1), 0, 0, 0, 0, shadow});
    };
    coat(PAL_MAN, gs::rgb4(5, 7, 4), gs::rgb4(2, 4, 2));
    coat(PAL_MANB, gs::rgb4(10, 6, 3), gs::rgb4(6, 3, 2));

    setPal(vdp, PAL_TREE,
           {0, gs::rgb4(2, 8, 2), gs::rgb4(4, 11, 3), gs::rgb4(6, 4, 2), gs::rgb4(8, 5, 2), gs::rgb4(12, 3, 2),
            gs::rgb4(14, 4, 2), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_WOOD,
           {0, gs::rgb4(10, 7, 3), gs::rgb4(6, 4, 2), gs::rgb4(8, 6, 3), gs::rgb4(4, 3, 2), gs::rgb4(12, 9, 4),
            gs::rgb4(13, 3, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FX,
           {0, gs::rgb4(14, 12, 4), gs::rgb4(8, 6, 2), gs::rgb4(13, 8, 2), gs::rgb4(15, 14, 6), gs::rgb4(15, 15, 11),
            gs::rgb4(15, 8, 2), gs::rgb4(9, 8, 4), gs::rgb4(14, 11, 6), gs::rgb4(4, 4, 3), gs::rgb4(2, 2, 1), 0, 0, 0, 0,
            shadow});
    setPal(vdp, PAL_APPLE,
           {0, gs::rgb4(5, 5, 4), gs::rgb4(3, 3, 2), gs::rgb4(12, 8, 2), gs::rgb4(15, 13, 4), gs::rgb4(15, 14, 8),
            gs::rgb4(14, 3, 2), gs::rgb4(15, 6, 3), 0, 0, 0, 0, 0, 0, 0, shadow});

    const uint16_t grass[16] = {
        0,
        gs::rgb4(3, 7, 2), gs::rgb4(2, 5, 1), gs::rgb4(4, 8, 2),
        gs::rgb4(5, 9, 3), gs::rgb4(2, 6, 2),
        gs::rgb4(6, 5, 2), gs::rgb4(4, 4, 2),
        gs::rgb4(5, 4, 2), gs::rgb4(7, 6, 3), gs::rgb4(8, 7, 3),
        gs::rgb4(2, 4, 2), gs::rgb4(3, 5, 2), gs::rgb4(4, 6, 2),
        gs::rgb4(9, 8, 3), gs::rgb4(1, 3, 1),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_ROAD * 16 + i, grass[i]);

    loadFont(vdp, art);
    art.stand = gs::uploadMipped(vdp, manStand());
    art.crouch = gs::uploadMipped(vdp, manCrouch());
    art.fallen = gs::uploadMipped(vdp, manFallen());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.crate = gs::uploadMipped(vdp, crateArt());
    art.ladder = gs::uploadMipped(vdp, ladderArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.glow = gs::uploadMipped(vdp, glowArt());
    art.rifle = gs::uploadMipped(vdp, rifleArt());
    art.sight = gs::uploadMipped(vdp, sightArt());
    art.round = gs::uploadMipped(vdp, roundArt());
    art.spent = gs::uploadMipped(vdp, spentArt());
    art.flash = gs::uploadMipped(vdp, flashArt());
    art.spark = gs::uploadMipped(vdp, sparkArt());
    art.bar = gs::uploadMipped(vdp, barArt());
    art.apple = gs::uploadMipped(vdp, appleArt());
}

}  // namespace orchardmaga

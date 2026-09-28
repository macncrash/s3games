#include "game/art.h"

#include <cmath>
#include <string>

namespace towermaga {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap manStand() {
    Bitmap b(44, 84);
    b.rect(14, 28, 16, 30, 1);
    b.rect(14, 28, 5, 30, 2);
    b.ellipse(22, 14, 7, 8, 4);
    b.ellipse(22, 16, 5, 6, 3);
    b.rect(16, 6, 12, 5, 5);
    b.set(19, 15, 10);
    b.set(25, 15, 10);
    b.rect(8, 32, 6, 14, 3);
    b.rect(30, 32, 6, 14, 2);
    b.rect(15, 56, 6, 18, 8);
    b.rect(24, 56, 6, 18, 8);
    b.rect(14, 72, 8, 6, 9);
    b.rect(23, 72, 8, 6, 9);
    b.line(10, 48, 36, 30, 6, 2.2f);
    b.rect(32, 26, 6, 4, 6);
    b.outline(15, false);
    return b;
}

Bitmap manCrouch() {
    Bitmap b(50, 52);
    b.ellipse(24, 12, 7, 7, 4);
    b.ellipse(24, 14, 5, 5, 3);
    b.rect(18, 6, 12, 4, 5);
    b.set(21, 13, 10);
    b.set(27, 13, 10);
    b.poly({{8, 30}, {24, 18}, {40, 30}, {38, 42}, {10, 44}}, 1);
    b.poly({{8, 30}, {18, 22}, {20, 42}, {10, 44}}, 2);
    b.rect(10, 40, 12, 6, 8);
    b.rect(26, 40, 12, 6, 8);
    b.rect(8, 45, 12, 4, 9);
    b.rect(28, 45, 12, 4, 9);
    b.line(6, 34, 40, 24, 6, 2.0f);
    b.outline(15, false);
    return b;
}

Bitmap manFallen() {
    Bitmap b(80, 28);
    b.ellipse(12, 12, 7, 6, 4);
    b.ellipse(13, 13, 4, 4, 3);
    b.rect(18, 8, 34, 10, 1);
    b.rect(18, 8, 34, 3, 2);
    b.rect(50, 10, 12, 7, 8);
    b.rect(60, 12, 10, 5, 9);
    b.line(26, 20, 54, 24, 6, 2.0f);
    b.outline(15, false);
    return b;
}

Bitmap pierArt() {
    Bitmap b(36, 100);
    b.rect(4, 16, 28, 84, 1);
    b.rect(4, 16, 8, 84, 2);
    b.rect(0, 8, 36, 12, 3);
    b.rect(6, 0, 24, 10, 4);
    for (int y = 28; y < 96; y += 16) {
        b.rect(6, y, 24, 2, 5);
        b.rect(10, y + 4, 6, 8, 2);
    }
    b.rect(12, 40, 12, 18, 6);
    b.rect(14, 42, 8, 12, 7);
    return b;
}

Bitmap bannerArt() {
    Bitmap b(22, 70);
    b.rect(10, 0, 2, 70, 2);
    b.poly({{12, 8}, {20, 16}, {12, 28}}, 6);
    b.poly({{12, 28}, {20, 36}, {12, 48}}, 1);
    b.rect(12, 8, 3, 40, 4);
    return b;
}

Bitmap stairArt() {
    Bitmap b(64, 36);
    for (int i = 0; i < 5; i++) {
        int y = 4 + i * 6;
        int x = 4 + i * 4;
        b.rect(x, y, 56 - i * 8, 5, i & 1 ? 2 : 1);
        b.rect(x, y, 56 - i * 8, 1, 3);
    }
    return b;
}

Bitmap lampArt() {
    Bitmap b(16, 30);
    b.rect(7, 0, 2, 10, 2);
    b.poly({{3, 12}, {13, 12}, {15, 22}, {1, 22}}, 6);
    b.poly({{5, 14}, {11, 14}, {12, 20}, {4, 20}}, 7);
    b.rect(2, 22, 12, 3, 4);
    return b;
}

Bitmap glowArt() {
    Bitmap b(26, 26);
    b.ellipse(13, 13, 11, 9, 3);
    b.ellipse(13, 13, 5, 4, 4);
    return b;
}

Bitmap rifleArt() {
    Bitmap b(28, 78);
    b.rect(10, 28, 8, 42, 2);
    b.rect(12, 30, 4, 26, 1);
    b.rect(11, 4, 6, 28, 4);
    b.rect(13, 2, 2, 6, 5);
    b.rect(6, 26, 16, 8, 3);
    b.rect(4, 46, 6, 14, 1);
    b.poly({{10, 66}, {18, 66}, {22, 76}, {6, 76}}, 2);
    return b;
}

Bitmap sightArt() {
    Bitmap b(34, 34);
    const float cx = 17, cy = 17;
    for (int i = 0; i < 40; i++) {
        if (i % 10 < 2) continue;
        float a = i * 6.2831853f / 40.f;
        b.set(int(std::lround(cx + std::cos(a) * 11)), int(std::lround(cy + std::sin(a) * 11)), 4);
    }
    b.rect(16, 2, 2, 5, 4);
    b.rect(16, 27, 2, 5, 4);
    b.rect(2, 16, 5, 2, 4);
    b.rect(27, 16, 5, 2, 4);
    b.set(17, 17, 5);
    return b;
}

Bitmap roundArt() {
    Bitmap b(10, 22);
    b.rect(2, 2, 6, 16, 2);
    b.rect(3, 3, 4, 8, 1);
    b.rect(3, 3, 4, 4, 3);
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
    Bitmap b(14, 14);
    b.ellipse(7, 7, 5, 4, 7);
    b.ellipse(7, 7, 2, 2, 8);
    return b;
}

Bitmap barArt() {
    Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 1);
    return b;
}

Bitmap flagArt() {
    Bitmap b(36, 18);
    b.rect(0, 2, 3, 16, 2);
    b.poly({{3, 2}, {34, 6}, {28, 10}, {34, 14}, {3, 16}}, 1);
    b.poly({{3, 2}, {20, 5}, {18, 9}, {3, 8}}, 6);
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
    const uint16_t ink = gs::rgb4(15, 14, 12);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(8, 7, 6), gs::rgb4(15, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 11, 3), gs::rgb4(11, 7, 2), gs::rgb4(15, 15, 9), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 4, 3), gs::rgb4(9, 2, 2), gs::rgb4(15, 10, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(7, 15, 6), gs::rgb4(2, 8, 3), gs::rgb4(13, 15, 11), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    auto coat = [&](int pal, uint16_t hi, uint16_t mid) {
        setPal(vdp, pal,
               {0, hi, mid, gs::rgb4(11, 8, 6), gs::rgb4(3, 3, 4), gs::rgb4(4, 3, 3), gs::rgb4(9, 9, 10),
                gs::rgb4(5, 4, 2), gs::rgb4(3, 3, 5), gs::rgb4(2, 2, 2), gs::rgb4(1, 1, 1), 0, 0, 0, 0, shadow});
    };
    coat(PAL_MAN, gs::rgb4(5, 6, 8), gs::rgb4(2, 3, 5));
    coat(PAL_MANB, gs::rgb4(10, 5, 3), gs::rgb4(6, 2, 2));

    setPal(vdp, PAL_STONE,
           {0, gs::rgb4(7, 7, 8), gs::rgb4(5, 5, 6), gs::rgb4(10, 10, 11), gs::rgb4(4, 4, 5), gs::rgb4(3, 3, 4),
            gs::rgb4(12, 8, 3), gs::rgb4(2, 2, 3), 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_IRON,
           {0, gs::rgb4(6, 7, 7), gs::rgb4(3, 4, 4), gs::rgb4(9, 8, 7), gs::rgb4(2, 2, 3), gs::rgb4(5, 4, 3),
            0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FX,
           {0, gs::rgb4(14, 11, 4), gs::rgb4(8, 6, 3), gs::rgb4(13, 8, 3), gs::rgb4(15, 13, 6), gs::rgb4(15, 15, 12),
            gs::rgb4(15, 8, 2), gs::rgb4(9, 8, 6), gs::rgb4(14, 12, 8), gs::rgb4(4, 4, 4), gs::rgb4(2, 2, 2), 0, 0, 0, 0,
            shadow});
    setPal(vdp, PAL_LAMP,
           {0, gs::rgb4(6, 6, 7), gs::rgb4(3, 3, 4), gs::rgb4(12, 8, 2), gs::rgb4(15, 12, 4), gs::rgb4(15, 14, 8),
            gs::rgb4(14, 9, 2), gs::rgb4(15, 15, 10), 0, 0, 0, 0, 0, 0, 0, shadow});

    const uint16_t cobble[16] = {
        0,
        gs::rgb4(3, 3, 4), gs::rgb4(2, 2, 3), gs::rgb4(4, 4, 5),
        gs::rgb4(5, 5, 5), gs::rgb4(3, 3, 4),
        gs::rgb4(6, 6, 5), gs::rgb4(4, 4, 5),
        gs::rgb4(2, 2, 3), gs::rgb4(5, 4, 3), gs::rgb4(6, 5, 4),
        gs::rgb4(1, 1, 2), gs::rgb4(3, 3, 3), gs::rgb4(4, 4, 4),
        gs::rgb4(7, 7, 6), gs::rgb4(1, 1, 2),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_ROAD * 16 + i, cobble[i]);

    loadFont(vdp, art);
    art.stand = gs::uploadMipped(vdp, manStand());
    art.crouch = gs::uploadMipped(vdp, manCrouch());
    art.fallen = gs::uploadMipped(vdp, manFallen());
    art.pier = gs::uploadMipped(vdp, pierArt());
    art.banner = gs::uploadMipped(vdp, bannerArt());
    art.stair = gs::uploadMipped(vdp, stairArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.glow = gs::uploadMipped(vdp, glowArt());
    art.rifle = gs::uploadMipped(vdp, rifleArt());
    art.sight = gs::uploadMipped(vdp, sightArt());
    art.round = gs::uploadMipped(vdp, roundArt());
    art.spent = gs::uploadMipped(vdp, spentArt());
    art.flash = gs::uploadMipped(vdp, flashArt());
    art.spark = gs::uploadMipped(vdp, sparkArt());
    art.bar = gs::uploadMipped(vdp, barArt());
    art.flag = gs::uploadMipped(vdp, flagArt());

    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.setFogColor(gs::rgb4(1, 1, 3));
}

}  // namespace towermaga

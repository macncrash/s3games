#include "game/art.h"

#include <cmath>
#include <initializer_list>

namespace skate {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
}

void ink(gs::VDP& vdp, int pal, uint16_t main, uint16_t shadow) {
    setPal(vdp, pal, {0, main});
    vdp.setColor(pal * 16 + 15, shadow);
}

void board(gs::Bitmap& b, float x0, float y0, float x1, float y1) {
    b.line(x0, y0, x1, y1, 7, 4.2f);
    b.line(x0, y0 - 2.f, x1, y1 - 2.f, 8, 2.2f);
    b.rect(x1 - 3.f, y1 - 5.f, 5.f, 4.f, 14);
    b.ellipse(x0 + 4.f, y0 + 4.f, 3.1f, 3.1f, 9);
    b.ellipse(x1 - 4.f, y1 + 4.f, 3.1f, 3.1f, 9);
    b.ellipse(x0 + 4.f, y0 + 4.f, 1.3f, 1.3f, 8);
    b.ellipse(x1 - 4.f, y1 + 4.f, 1.3f, 1.3f, 8);
}

void helmet(gs::Bitmap& b, float cx, float cy) {
    b.ellipse(cx, cy, 7.2f, 7.4f, 5);
    b.ellipse(cx - 1.f, cy - 2.f, 3.2f, 2.4f, 15);
    b.rect(cx - 6.f, cy + 1.f, 12.f, 3.f, 6);
    b.rect(cx - 2.f, cy + 2.f, 5.f, 2.f, 3);
}

void rider(gs::Bitmap& b, float lean, float crouch, bool armsOut) {
    const float hx = 28.f + lean;
    const float hy = 14.f + crouch;
    const float hip = 36.f + crouch * 0.35f;
    helmet(b, hx, hy);
    b.poly({{hx - 6.f, hy + 6.f}, {hx + 7.f, hy + 6.f}, {hx + 8.f, hip}, {hx - 7.f, hip}}, 1);
    b.rect(hx - 5.f, hy + 8.f, 10.f, 4.f, 2);
    if (armsOut) {
        b.line(hx - 4.f, hy + 10.f, hx - 16.f, hy + 8.f, 1, 3.2f);
        b.line(hx + 5.f, hy + 10.f, hx + 16.f, hy + 6.f, 1, 3.2f);
        b.ellipse(hx - 16.f, hy + 8.f, 2.f, 2.f, 3);
        b.ellipse(hx + 16.f, hy + 6.f, 2.f, 2.f, 3);
    } else {
        b.line(hx - 2.f, hy + 10.f, hx - 8.f, hip + 2.f, 1, 3.f);
        b.line(hx + 3.f, hy + 10.f, hx + 9.f, hip - 2.f, 1, 3.f);
    }
    b.line(hx - 3.f, hip, 16.f, 48.f, 11, 4.f);
    b.line(hx + 3.f, hip, 32.f, 48.f, 12, 4.f);
    b.rect(12.f, 46.f, 10.f, 4.f, 10);
    b.rect(28.f, 46.f, 10.f, 4.f, 10);
}

gs::Bitmap rideArt() {
    gs::Bitmap b(52, 66);
    rider(b, 0.f, 0.f, false);
    board(b, 6.f, 52.f, 46.f, 52.f);
    b.outline(13, false);
    return b;
}

gs::Bitmap airArt() {
    gs::Bitmap b(52, 66);
    rider(b, 1.f, 6.f, false);
    board(b, 8.f, 54.f, 46.f, 52.f);
    b.outline(13, false);
    return b;
}

gs::Bitmap noseUpArt() {
    gs::Bitmap b(56, 66);
    rider(b, -6.f, 2.f, true);
    board(b, 6.f, 56.f, 50.f, 44.f);
    b.outline(13, false);
    return b;
}

gs::Bitmap noseDownArt() {
    gs::Bitmap b(56, 66);
    rider(b, 7.f, 2.f, true);
    board(b, 4.f, 44.f, 50.f, 56.f);
    b.outline(13, false);
    return b;
}

gs::Bitmap grindArt() {
    gs::Bitmap b(52, 58);
    rider(b, 0.f, 10.f, false);
    board(b, 6.f, 48.f, 46.f, 48.f);
    b.outline(13, false);
    return b;
}

gs::Bitmap bailArt() {
    gs::Bitmap b(64, 36);
    b.ellipse(16.f, 16.f, 8.f, 7.f, 5);
    b.rect(10.f, 16.f, 12.f, 3.f, 6);
    b.ellipse(30.f, 20.f, 12.f, 6.f, 1);
    b.rect(24.f, 18.f, 6.f, 5.f, 2);
    b.line(18.f, 18.f, 8.f, 10.f, 1, 3.f);
    b.line(36.f, 18.f, 48.f, 12.f, 11, 3.f);
    b.ellipse(8.f, 10.f, 2.f, 2.f, 3);
    board(b, 34.f, 8.f, 60.f, 14.f);
    b.outline(13, false);
    return b;
}

gs::Bitmap deckArt(bool pad) {
    gs::Bitmap b(40, 28);
    b.rect(0, 0, 40, 6, pad ? 6 : 1);
    b.rect(0, 2, 40, 2, pad ? 7 : 2);
    b.rect(0, 6, 40, 20, 3);
    b.rect(0, 22, 40, 6, 4);
    for (int x = 8; x < 40; x += 10) b.rect(x, 6, 2, 20, 5);
    b.line(4, 12, 18, 16, 5, 1.f);
    b.rect(0, 0, 40, 1, pad ? 14 : 15);
    return b;
}

gs::Bitmap railArt() {
    gs::Bitmap b(36, 24);
    b.rect(2, 3, 32, 4, 1);
    b.rect(2, 3, 32, 1, 15);
    b.rect(4, 6, 3, 14, 3);
    b.rect(28, 6, 3, 14, 3);
    b.rect(3, 18, 6, 3, 4);
    b.rect(27, 18, 6, 3, 4);
    b.rect(8, 4, 3, 2, 5);
    b.rect(24, 4, 3, 2, 5);
    b.rect(2, 5, 32, 1, 2);
    return b;
}

gs::Bitmap stairArt() {
    gs::Bitmap b(40, 36);
    for (int i = 0; i < 5; i++) {
        int y = 4 + i * 6;
        int x = 2 + i * 6;
        b.rect(float(x), float(y), float(36 - i * 6), 6.f, (i & 1) ? 6 : 7);
        b.rect(float(x), float(y), float(36 - i * 6), 1.f, 8);
    }
    return b;
}

gs::Bitmap bankArt() {
    gs::Bitmap b(44, 30);
    b.poly({{2, 4}, {40, 26}, {40, 29}, {2, 10}}, 3);
    b.poly({{2, 4}, {40, 26}, {40, 22}, {6, 4}}, 1);
    b.line(4, 6, 38, 24, 6, 1.4f);
    return b;
}

gs::Bitmap blockArt() {
    gs::Bitmap b(28, 34);
    b.rect(2, 6, 24, 26, 3);
    b.rect(2, 6, 24, 4, 1);
    b.rect(2, 28, 24, 4, 4);
    b.rect(6, 12, 6, 8, 5);
    b.rect(16, 18, 6, 6, 2);
    b.outline(5, false);
    return b;
}

gs::Bitmap coneArt() {
    gs::Bitmap b(16, 22);
    b.poly({{8, 2}, {3, 16}, {13, 16}}, 1);
    b.poly({{8, 5}, {5, 16}, {11, 16}}, 2);
    b.rect(3, 15, 10, 3, 3);
    b.rect(6, 18, 4, 2, 8);
    return b;
}

gs::Bitmap canArt() {
    gs::Bitmap b(12, 16);
    b.rect(2, 2, 8, 12, 4);
    b.rect(2, 2, 8, 2, 5);
    b.rect(3, 6, 6, 2, 1);
    b.ellipse(6, 2, 3, 1.4, 5);
    b.outline(3, false);
    return b;
}

gs::Bitmap buildingArt(bool tower) {
    gs::Bitmap b(tower ? 26 : 34, tower ? 78 : 64);
    b.rect(2, 8, b.w - 4, b.h - 8, 1);
    b.rect(2, b.h - 6, b.w - 4, 6, 2);
    b.rect(4, 2, b.w - 8, 8, 5);
    for (int y = 14; y < b.h - 10; y += 8)
        for (int x = 6; x < b.w - 6; x += 8) b.rect(x, y, 4, 5, ((x + y) / 8) & 1 ? 3 : 4);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(28, 28);
    b.ellipse(14, 16, 8, 8, 4);
    b.ellipse(12, 14, 3, 3, 5);
    for (int i = 0; i < 6; i++) {
        float a = -0.4f + float(i) * 0.45f;
        b.line(14 + std::cos(a) * 8, 16 + std::sin(a) * 7, 14 + std::cos(a) * 13, 16 + std::sin(a) * 12, 5, 1.4f);
    }
    return b;
}

gs::Bitmap cloudArt() {
    gs::Bitmap b(40, 16);
    b.ellipse(14, 9, 10, 5, 6);
    b.ellipse(26, 8, 11, 6, 6);
    b.ellipse(20, 10, 8, 4, 1);
    return b;
}

gs::Bitmap sparkArt() {
    gs::Bitmap b(12, 12);
    b.line(1, 6, 11, 6, 2, 1.4f);
    b.line(6, 1, 6, 11, 1, 1.4f);
    b.line(2, 2, 10, 10, 1, 1.2f);
    b.ellipse(6, 6, 1.6f, 1.6f, 2);
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(14, 10);
    b.ellipse(5, 5, 4, 2.4f, 3);
    b.ellipse(10, 4, 3, 2.f, 1);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(36, 10);
    b.ellipse(18, 5, 14, 3, 1);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
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
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    ink(vdp, PAL_HUD, gs::rgb4(15, 15, 14), gs::rgb4(2, 1, 3));
    ink(vdp, PAL_GOLD, gs::rgb4(15, 12, 3), gs::rgb4(4, 2, 0));
    vdp.setColor(PAL_GOLD * 16 + 2, gs::rgb4(6, 3, 0));
    ink(vdp, PAL_RED, gs::rgb4(15, 4, 3), gs::rgb4(4, 0, 1));
    ink(vdp, PAL_GREEN, gs::rgb4(6, 15, 8), gs::rgb4(0, 3, 1));

    setPal(vdp, PAL_SKATER,
           {0, gs::rgb4(3, 13, 12), gs::rgb4(1, 7, 8), gs::rgb4(14, 10, 7), gs::rgb4(10, 6, 4), gs::rgb4(14, 14, 15),
            gs::rgb4(13, 2, 3), gs::rgb4(12, 7, 3), gs::rgb4(2, 2, 2), gs::rgb4(15, 15, 15), gs::rgb4(15, 12, 2),
            gs::rgb4(3, 4, 8), gs::rgb4(2, 2, 5), gs::rgb4(1, 1, 2), gs::rgb4(15, 13, 4), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_DECK,
           {0, gs::rgb4(11, 11, 12), gs::rgb4(8, 8, 9), gs::rgb4(6, 6, 7), gs::rgb4(4, 4, 5), gs::rgb4(3, 3, 4),
            gs::rgb4(14, 11, 3), gs::rgb4(8, 6, 2), gs::rgb4(4, 6, 4), gs::rgb4(2, 2, 3), gs::rgb4(9, 9, 10),
            gs::rgb4(7, 7, 8), gs::rgb4(5, 5, 6), gs::rgb4(2, 2, 3), gs::rgb4(15, 13, 6), gs::rgb4(13, 13, 14)});
    setPal(vdp, PAL_RAIL,
           {0, gs::rgb4(12, 14, 15), gs::rgb4(6, 8, 10), gs::rgb4(4, 4, 6), gs::rgb4(8, 4, 2), gs::rgb4(15, 13, 6),
            gs::rgb4(9, 10, 12), gs::rgb4(3, 3, 4), gs::rgb4(14, 14, 15), 0, 0, 0, 0, 0, 0, 0, gs::rgb4(15, 15, 15)});
    setPal(vdp, PAL_CITY,
           {0, gs::rgb4(5, 3, 7), gs::rgb4(3, 2, 4), gs::rgb4(15, 11, 5), gs::rgb4(8, 5, 3), gs::rgb4(2, 1, 3)});
    setPal(vdp, PAL_PROP,
           {0, gs::rgb4(15, 8, 2), gs::rgb4(15, 12, 4), gs::rgb4(10, 4, 1), gs::rgb4(7, 10, 6), gs::rgb4(12, 14, 9),
            gs::rgb4(7, 7, 8), gs::rgb4(4, 4, 5), gs::rgb4(10, 10, 11), gs::rgb4(3, 3, 4)});
    setPal(vdp, PAL_FX,
           {0, gs::rgb4(12, 8, 6), gs::rgb4(15, 15, 13), gs::rgb4(8, 6, 6), gs::rgb4(15, 9, 3), gs::rgb4(15, 14, 8),
            gs::rgb4(10, 6, 9)});
    setPal(vdp, PAL_PAD,
           {0, gs::rgb4(12, 10, 4), gs::rgb4(8, 6, 2), gs::rgb4(6, 5, 3), gs::rgb4(4, 3, 2), gs::rgb4(3, 2, 2),
            gs::rgb4(15, 12, 3), gs::rgb4(10, 7, 1), gs::rgb4(6, 7, 3), gs::rgb4(2, 2, 1), gs::rgb4(14, 11, 4), 0, 0, 0,
            gs::rgb4(15, 14, 6), gs::rgb4(15, 13, 5)});

    vdp.setFogColor(gs::rgb4(7, 3, 8));
    loadFont(vdp, art);
    art.ride = gs::uploadMipped(vdp, rideArt());
    art.air = gs::uploadMipped(vdp, airArt());
    art.noseUp = gs::uploadMipped(vdp, noseUpArt());
    art.noseDown = gs::uploadMipped(vdp, noseDownArt());
    art.grind = gs::uploadMipped(vdp, grindArt());
    art.bail = gs::uploadMipped(vdp, bailArt());
    art.deck = gs::uploadMipped(vdp, deckArt(false));
    art.pad = gs::uploadMipped(vdp, deckArt(true));
    art.rail = gs::uploadMipped(vdp, railArt());
    art.stair = gs::uploadMipped(vdp, stairArt());
    art.bank = gs::uploadMipped(vdp, bankArt());
    art.block = gs::uploadMipped(vdp, blockArt());
    art.cone = gs::uploadMipped(vdp, coneArt());
    art.can = gs::uploadMipped(vdp, canArt());
    art.building = gs::uploadMipped(vdp, buildingArt(false));
    art.tower = gs::uploadMipped(vdp, buildingArt(true));
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.spark = gs::uploadMipped(vdp, sparkArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.logo = gs::uploadMipped(vdp, gs::textBitmap("S3 SKATE", {3, 1, 2, 0, 1}));
}

}  // namespace skate

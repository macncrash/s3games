#include "game/art.h"

#include <cmath>
#include <initializer_list>

namespace skatemark {
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
    b.line(x0, y0, x1, y1, 7, 3.6f);
    b.line(x0, y0 - 2.f, x1, y1 - 2.f, 8, 1.8f);
    b.ellipse(x0 + 3.f, y0 + 3.f, 2.6f, 2.6f, 9);
    b.ellipse(x1 - 3.f, y1 + 3.f, 2.6f, 2.6f, 9);
}

void rider(gs::Bitmap& b, float crouch, bool arms) {
    const float hx = 22.f;
    const float hy = 10.f + crouch;
    const float hip = 28.f + crouch * 0.4f;
    b.ellipse(hx, hy, 6.2f, 6.4f, 5);
    b.ellipse(hx - 1.f, hy - 1.5f, 2.6f, 2.f, 15);
    b.rect(hx - 5.f, hy + 1.f, 10.f, 2.f, 6);
    b.poly({{hx - 5.f, hy + 5.f}, {hx + 6.f, hy + 5.f}, {hx + 6.f, hip}, {hx - 5.f, hip}}, 1);
    b.rect(hx - 4.f, hy + 7.f, 8.f, 3.f, 2);
    if (arms) {
        b.line(hx - 3.f, hy + 8.f, hx - 12.f, hy + 6.f, 1, 2.6f);
        b.line(hx + 4.f, hy + 8.f, hx + 13.f, hy + 5.f, 1, 2.6f);
    } else {
        b.line(hx - 2.f, hy + 8.f, hx - 6.f, hip, 1, 2.4f);
        b.line(hx + 2.f, hy + 8.f, hx + 7.f, hip - 1.f, 1, 2.4f);
    }
    b.line(hx - 2.f, hip, 14.f, 40.f, 11, 3.2f);
    b.line(hx + 2.f, hip, 26.f, 40.f, 12, 3.2f);
    b.rect(10.f, 38.f, 8.f, 3.f, 10);
    b.rect(22.f, 38.f, 8.f, 3.f, 10);
}

gs::Bitmap rideArt() {
    gs::Bitmap b(44, 54);
    rider(b, 0.f, false);
    board(b, 4.f, 43.f, 40.f, 43.f);
    b.outline(13, false);
    return b;
}

gs::Bitmap airArt() {
    gs::Bitmap b(44, 54);
    rider(b, 5.f, true);
    board(b, 6.f, 45.f, 40.f, 42.f);
    b.outline(13, false);
    return b;
}

gs::Bitmap bailArt() {
    gs::Bitmap b(48, 28);
    b.ellipse(14.f, 10.f, 6.f, 6.f, 5);
    b.rect(8.f, 14.f, 28.f, 7.f, 1);
    b.rect(10.f, 16.f, 10.f, 3.f, 2);
    board(b, 6.f, 22.f, 42.f, 18.f);
    b.outline(13, false);
    return b;
}

gs::Bitmap deckArt() {
    gs::Bitmap b(28, 18);
    b.rect(0, 4, 28, 10, 1);
    b.rect(0, 4, 28, 3, 2);
    b.rect(0, 12, 28, 2, 4);
    b.line(4, 8, 4, 13, 5, 1.f);
    b.line(14, 8, 14, 13, 5, 1.f);
    b.line(22, 8, 22, 13, 5, 1.f);
    return b;
}

gs::Bitmap railArt() {
    gs::Bitmap b(16, 28);
    b.rect(6, 2, 4, 22, 1);
    b.rect(7, 2, 1, 22, 2);
    b.rect(2, 20, 12, 6, 4);
    b.rect(1, 1, 14, 4, 3);
    return b;
}

gs::Bitmap bankArt() {
    gs::Bitmap b(36, 22);
    b.poly({{0, 18}, {36, 4}, {36, 20}, {0, 20}}, 1);
    b.line(2, 16, 34, 5, 6, 2.f);
    b.rect(0, 18, 36, 3, 4);
    return b;
}

gs::Bitmap coneArt() {
    gs::Bitmap b(14, 20);
    b.poly({{7, 1}, {12, 16}, {2, 16}}, 1);
    b.rect(3, 5, 8, 2, 2);
    b.rect(2, 10, 10, 2, 2);
    b.rect(1, 16, 12, 3, 3);
    return b;
}

gs::Bitmap buildingArt() {
    gs::Bitmap b(28, 48);
    b.rect(2, 6, 24, 42, 1);
    b.rect(4, 2, 8, 8, 1);
    for (int y = 12; y < 44; y += 8)
        for (int x = 6; x < 22; x += 8) b.rect(float(x), float(y), 4, 4, 3);
    b.rect(10, 36, 8, 12, 2);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(24, 24);
    b.ellipse(12, 12, 7, 7, 1);
    b.ellipse(10, 10, 3, 3, 2);
    for (int i = 0; i < 8; i++) {
        float a = float(i) * 0.785f;
        float c = std::cos(a), s = std::sin(a);
        b.line(12 + c * 8, 12 + s * 8, 12 + c * 11, 12 + s * 11, 1, 1.4f);
    }
    return b;
}

gs::Bitmap cloudArt() {
    gs::Bitmap b(32, 14);
    b.ellipse(10, 8, 8, 5, 1);
    b.ellipse(20, 7, 9, 5, 1);
    b.ellipse(16, 9, 6, 3, 2);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(32, 8);
    b.ellipse(16, 4, 13, 2.6f, 1);
    return b;
}

gs::Bitmap cardArt() {
    gs::Bitmap b(72, 48);
    b.rect(1, 1, 70, 46, 1);
    b.rect(4, 4, 64, 40, 2);
    b.rect(8, 10, 40, 4, 3);
    b.rect(8, 20, 28, 3, 4);
    b.rect(8, 28, 28, 3, 4);
    b.rect(8, 36, 28, 3, 4);
    return b;
}

gs::Bitmap stampArt() {
    gs::Bitmap b(40, 18);
    b.rect(1, 1, 38, 16, 1);
    b.rect(3, 3, 34, 12, 2);
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
    ink(vdp, PAL_RED, gs::rgb4(15, 4, 3), gs::rgb4(4, 0, 1));
    ink(vdp, PAL_GREEN, gs::rgb4(6, 15, 8), gs::rgb4(0, 3, 1));

    setPal(vdp, PAL_SKATER,
           {0, gs::rgb4(3, 13, 12), gs::rgb4(1, 7, 8), gs::rgb4(14, 10, 7), gs::rgb4(10, 6, 4), gs::rgb4(14, 14, 15),
            gs::rgb4(13, 2, 3), gs::rgb4(12, 7, 3), gs::rgb4(2, 2, 2), gs::rgb4(15, 15, 15), gs::rgb4(15, 12, 2),
            gs::rgb4(3, 4, 8), gs::rgb4(2, 2, 5), gs::rgb4(1, 1, 2), gs::rgb4(15, 13, 4), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_DECK,
           {0, gs::rgb4(11, 11, 12), gs::rgb4(8, 8, 9), gs::rgb4(6, 6, 7), gs::rgb4(4, 4, 5), gs::rgb4(3, 3, 4),
            gs::rgb4(14, 11, 3), gs::rgb4(8, 6, 2), 0, 0, 0, 0, 0, 0, 0, gs::rgb4(13, 13, 14)});
    setPal(vdp, PAL_RAIL,
           {0, gs::rgb4(12, 14, 15), gs::rgb4(7, 9, 11), gs::rgb4(15, 12, 4), gs::rgb4(6, 5, 4), 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, gs::rgb4(15, 15, 15)});
    setPal(vdp, PAL_CITY,
           {0, gs::rgb4(5, 3, 7), gs::rgb4(3, 2, 4), gs::rgb4(15, 11, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_PROP,
           {0, gs::rgb4(15, 8, 2), gs::rgb4(15, 14, 6), gs::rgb4(4, 3, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_FX,
           {0, gs::rgb4(15, 14, 6), gs::rgb4(15, 15, 12), gs::rgb4(8, 8, 9), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            gs::rgb4(2, 1, 2)});

    art.ride = gs::uploadMipped(vdp, rideArt());
    art.air = gs::uploadMipped(vdp, airArt());
    art.bail = gs::uploadMipped(vdp, bailArt());
    art.deck = gs::uploadMipped(vdp, deckArt());
    art.rail = gs::uploadMipped(vdp, railArt());
    art.bank = gs::uploadMipped(vdp, bankArt());
    art.cone = gs::uploadMipped(vdp, coneArt());
    art.building = gs::uploadMipped(vdp, buildingArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.card = gs::uploadMipped(vdp, cardArt());
    art.stamp = gs::uploadMipped(vdp, stampArt());

    gs::TextStyle st;
    st.scale = 2;
    st.color = 1;
    st.outline = 15;
    art.logo = gs::uploadMipped(vdp, gs::textBitmap("SKATEMARK", st));
    loadFont(vdp, art);
}

}  // namespace skatemark

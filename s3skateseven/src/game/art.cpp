#include "game/art.h"

#include <initializer_list>

namespace skateseven {
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
    b.line(x0, y0, x1, y1, 7, 3.4f);
    b.line(x0, y0 - 2.f, x1, y1 - 2.f, 8, 1.6f);
    b.ellipse(x0 + 3.f, y0 + 3.f, 2.4f, 2.4f, 9);
    b.ellipse(x1 - 3.f, y1 + 3.f, 2.4f, 2.4f, 9);
}

void body(gs::Bitmap& b, float crouch, bool arms, int shirt) {
    const float hx = 22.f;
    const float hy = 10.f + crouch;
    const float hip = 28.f + crouch * 0.4f;
    b.ellipse(hx, hy, 6.f, 6.2f, 5);
    b.ellipse(hx - 1.f, hy - 1.4f, 2.4f, 1.8f, 15);
    b.rect(hx - 5.f, hy + 1.f, 10.f, 2.f, 6);
    b.poly({{hx - 5.f, hy + 5.f}, {hx + 6.f, hy + 5.f}, {hx + 6.f, hip}, {hx - 5.f, hip}}, shirt);
    b.rect(hx - 4.f, hy + 7.f, 8.f, 3.f, 2);
    if (arms) {
        b.line(hx - 3.f, hy + 8.f, hx - 12.f, hy + 4.f, shirt, 2.4f);
        b.line(hx + 4.f, hy + 8.f, hx + 13.f, hy + 3.f, shirt, 2.4f);
    } else {
        b.line(hx - 2.f, hy + 8.f, hx - 6.f, hip, shirt, 2.2f);
        b.line(hx + 2.f, hy + 8.f, hx + 7.f, hip - 1.f, shirt, 2.2f);
    }
    b.line(hx - 2.f, hip, 14.f, 40.f, 11, 3.f);
    b.line(hx + 2.f, hip, 26.f, 40.f, 12, 3.f);
    b.rect(10.f, 38.f, 8.f, 3.f, 10);
    b.rect(22.f, 38.f, 8.f, 3.f, 10);
}

gs::Bitmap rideArt() {
    gs::Bitmap b(44, 54);
    body(b, 0.f, false, 1);
    board(b, 4.f, 43.f, 40.f, 43.f);
    b.outline(13, false);
    return b;
}

gs::Bitmap airArt() {
    gs::Bitmap b(44, 54);
    body(b, 6.f, true, 1);
    board(b, 6.f, 46.f, 40.f, 42.f);
    b.outline(13, false);
    return b;
}

gs::Bitmap bailArt() {
    gs::Bitmap b(48, 28);
    b.ellipse(14.f, 10.f, 6.f, 6.f, 5);
    b.rect(8.f, 14.f, 28.f, 7.f, 1);
    board(b, 6.f, 22.f, 42.f, 18.f);
    b.outline(13, false);
    return b;
}

gs::Bitmap deckArt() {
    gs::Bitmap b(28, 16);
    b.rect(0, 2, 28, 10, 1);
    b.rect(0, 2, 28, 3, 2);
    b.rect(0, 10, 28, 2, 4);
    b.line(6, 6, 6, 11, 6, 1.f);
    b.line(14, 6, 14, 11, 6, 1.f);
    b.line(22, 6, 22, 11, 6, 1.f);
    return b;
}

gs::Bitmap coneArt() {
    gs::Bitmap b(14, 18);
    b.poly({{7, 1}, {13, 16}, {1, 16}}, 1);
    b.rect(2, 15, 10, 2, 2);
    b.rect(5, 7, 4, 2, 2);
    return b;
}

gs::Bitmap buildingArt() {
    gs::Bitmap b(28, 48);
    b.rect(2, 6, 24, 42, 1);
    b.rect(4, 2, 8, 6, 3);
    for (int y = 10; y < 42; y += 8)
        for (int x = 5; x < 22; x += 7) b.rect(float(x), float(y), 3, 4, 2);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(20, 20);
    b.ellipse(10, 10, 6, 6, 1);
    b.ellipse(10, 10, 3, 3, 2);
    return b;
}

gs::Bitmap cloudArt() {
    gs::Bitmap b(36, 14);
    b.ellipse(12, 8, 8, 4, 1);
    b.ellipse(22, 7, 10, 5, 2);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(32, 8);
    b.ellipse(16, 4, 13, 2.4f, 1);
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
           {0, gs::rgb4(2, 12, 13), gs::rgb4(1, 6, 8), gs::rgb4(14, 10, 7), gs::rgb4(9, 5, 3), gs::rgb4(14, 14, 15),
            gs::rgb4(13, 2, 4), gs::rgb4(12, 7, 2), gs::rgb4(2, 2, 2), gs::rgb4(15, 15, 15), gs::rgb4(15, 12, 2),
            gs::rgb4(3, 4, 8), gs::rgb4(2, 2, 5), gs::rgb4(1, 1, 2), gs::rgb4(15, 13, 4), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_DECK,
           {0, gs::rgb4(10, 10, 12), gs::rgb4(7, 7, 9), gs::rgb4(5, 5, 6), gs::rgb4(3, 3, 4), gs::rgb4(2, 2, 3),
            gs::rgb4(14, 12, 3), 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(13, 13, 14)});
    setPal(vdp, PAL_RIVAL,
           {0, gs::rgb4(14, 4, 6), gs::rgb4(8, 2, 4), gs::rgb4(14, 10, 7), gs::rgb4(9, 5, 3), gs::rgb4(14, 14, 15),
            gs::rgb4(4, 2, 6), gs::rgb4(12, 7, 2), gs::rgb4(2, 2, 2), gs::rgb4(15, 15, 15), gs::rgb4(15, 12, 2),
            gs::rgb4(4, 3, 7), gs::rgb4(3, 2, 5), gs::rgb4(1, 1, 2), gs::rgb4(15, 13, 4), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_CITY, {0, gs::rgb4(5, 3, 8), gs::rgb4(15, 12, 5), gs::rgb4(3, 2, 5), 0});
    setPal(vdp, PAL_PROP, {0, gs::rgb4(15, 8, 2), gs::rgb4(15, 14, 5), 0});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 14, 6), gs::rgb4(15, 15, 13), gs::rgb4(8, 8, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         0, 0, 0, gs::rgb4(2, 1, 2)});

    art.ride = gs::uploadMipped(vdp, rideArt());
    art.air = gs::uploadMipped(vdp, airArt());
    art.bail = gs::uploadMipped(vdp, bailArt());
    art.deck = gs::uploadMipped(vdp, deckArt());
    art.cone = gs::uploadMipped(vdp, coneArt());
    art.building = gs::uploadMipped(vdp, buildingArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());

    gs::TextStyle st;
    st.scale = 2;
    st.color = 1;
    st.outline = 15;
    art.logo = gs::uploadMipped(vdp, gs::textBitmap("SEVEN", st));
    loadFont(vdp, art);
}

}  // namespace skateseven

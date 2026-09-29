#include "game/art.h"

#include "game/world.h"

#include <cmath>

namespace cliffboom {
namespace {

void textPal(gs::VDP& v, int pal, uint16_t ink) {
    v.setColor(pal * 16 + 1, ink);
    v.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

void paintPals(gs::VDP& vdp) {
    textPal(vdp, PAL_WHITE, gs::rgb4(15, 15, 15));
    textPal(vdp, PAL_AMBER, gs::rgb4(15, 12, 3));
    textPal(vdp, PAL_RED, gs::rgb4(15, 4, 3));
    textPal(vdp, PAL_GREEN, gs::rgb4(6, 15, 7));
    textPal(vdp, PAL_BANNER, gs::rgb4(14, 12, 8));

    const uint16_t cliff[16] = {
        0,
        gs::rgb4(3, 2, 2),
        gs::rgb4(6, 5, 4),
        gs::rgb4(9, 8, 6),
        gs::rgb4(11, 9, 6),
        gs::rgb4(8, 6, 4),
        gs::rgb4(5, 7, 3),
        gs::rgb4(13, 12, 9),
        gs::rgb4(13, 3, 2),
        gs::rgb4(15, 14, 12),
        gs::rgb4(4, 4, 5),
        gs::rgb4(7, 6, 5),
        gs::rgb4(14, 10, 3),
        gs::rgb4(2, 3, 4),
        gs::rgb4(10, 11, 8),
        gs::rgb4(1, 1, 2),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_CLIFF * 16 + i, cliff[i]);

    vdp.setColor(PAL_CAR * 16 + 1, gs::rgb4(13, 10, 4));
    vdp.setColor(PAL_CAR * 16 + 2, gs::rgb4(8, 5, 2));
    vdp.setColor(PAL_CAR * 16 + 3, gs::rgb4(15, 14, 11));
    vdp.setColor(PAL_CAR * 16 + 4, gs::rgb4(2, 2, 3));
    vdp.setColor(PAL_CAR * 16 + 5, gs::rgb4(12, 3, 2));
    vdp.setColor(PAL_CAR * 16 + 6, gs::rgb4(4, 6, 8));
    vdp.setColor(PAL_CAR * 16 + 7, gs::rgb4(15, 12, 2));

    vdp.setColor(PAL_BOOM * 16 + 1, gs::rgb4(14, 3, 2));
    vdp.setColor(PAL_BOOM * 16 + 2, gs::rgb4(15, 15, 13));
    vdp.setColor(PAL_BOOM * 16 + 3, gs::rgb4(3, 3, 4));
    vdp.setColor(PAL_BOOM * 16 + 4, gs::rgb4(14, 11, 3));

    vdp.setColor(PAL_DUST * 16 + 1, gs::rgb4(12, 10, 7));
    vdp.setColor(PAL_DUST * 16 + 2, gs::rgb4(8, 7, 5));
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& art) {
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
        art.font[c - 32] = t;
    }
}

void fillSlab(gs::Bitmap& b, const Slab& s, int c) { b.rect(s.x0, s.y0, s.x1 - s.x0, s.y1 - s.y0, c); }

gs::Bitmap cliffPicture() {
    gs::Bitmap b(512, 256);
    for (int i = 0; i < kShelfN; i++) {
        Slab lip = kShelf[i];
        lip.x0 -= 10.f;
        lip.y0 -= 8.f;
        lip.x1 += 8.f;
        lip.y1 += 8.f;
        if (lip.x0 < 0) lip.x0 = 0;
        if (lip.y0 < 0) lip.y0 = 0;
        fillSlab(b, lip, 2);
    }
    for (int i = 0; i < kShelfN; i++) fillSlab(b, kShelf[i], 4);
    for (int i = 0; i < kShelfN; i++) {
        const Slab& s = kShelf[i];
        b.rect(s.x0, s.y0, s.x1 - s.x0, 3, 5);
        b.rect(s.x0, s.y1 - 3, s.x1 - s.x0, 3, 5);
        b.rect(s.x0, s.y0, 3, s.y1 - s.y0, 11);
    }
    // Centre grit and a pale edge so the shelf reads as a road, not a field.
    b.rect(28, 178, 180, 4, 7);
    b.rect(186, 70, 4, 110, 7);
    b.rect(210, 68, 150, 4, 7);
    b.rect(348, 80, 4, 100, 7);
    b.rect(348, 180, 60, 4, 7);
    for (int i = 0; i < 9; i++) b.ellipse(40.f + i * 22.f, 164.f, 4, 2, 6);
    for (int i = 0; i < 6; i++) b.ellipse(230.f + i * 24.f, 52.f, 5, 2, 6);
    for (int i = 0; i < 5; i++) b.ellipse(360.f, 150.f + i * 12.f, 3, 2, 14);
    // Striped gate across the end of the shelf. Past it the plane stays clear: the drop.
    for (int i = 0; i < 8; i++) b.rect(BOOM_X - 4.f, BOOM_Y0 + i * 8.f, 10, 8, (i & 1) ? 9 : 8);
    b.rect(BOOM_X - 6.f, BOOM_Y0 - 4.f, 14, 4, 10);
    b.rect(BOOM_X + 6.f, 156, 18, 6, 3);
    b.rect(388, 168, 22, 14, 12);
    return b;
}

gs::Bitmap carArt(float ang) {
    gs::Bitmap b(40, 40);
    const float cx = 20.f, cy = 20.f;
    auto P = [&](float x, float y) -> gs::Pt {
        float c = std::cos(ang), s = std::sin(ang);
        return {cx + x * c - y * s, cy + x * s + y * c};
    };
    b.poly({P(14, 0), P(8, -7), P(-11, -7), P(-14, 0), P(-11, 7), P(8, 7)}, 1);
    b.poly({P(11, 0), P(6, -5), P(-8, -5), P(-8, 5), P(6, 5)}, 2);
    b.poly({P(3, -4), P(3, 4), P(-4, 3), P(-4, -3)}, 3);
    b.ellipse(P(6, -6).first, P(6, -6).second, 2.2f, 2.2f, 4);
    b.ellipse(P(6, 6).first, P(6, 6).second, 2.2f, 2.2f, 4);
    b.ellipse(P(-9, -6).first, P(-9, -6).second, 2.2f, 2.2f, 4);
    b.ellipse(P(-9, 6).first, P(-9, 6).second, 2.2f, 2.2f, 4);
    b.ellipse(P(12, -3).first, P(12, -3).second, 1.4f, 1.2f, 7);
    b.ellipse(P(12, 3).first, P(12, 3).second, 1.4f, 1.2f, 7);
    b.rect(P(-12, -1).first, P(-12, -1).second, 2, 2, 5);
    return b;
}

gs::Bitmap boomArt() {
    gs::Bitmap b(16, 72);
    b.rect(2, 0, 12, 6, 3);
    for (int i = 0; i < 8; i++) b.rect(3, 6 + i * 8, 10, 8, (i & 1) ? 2 : 1);
    b.rect(1, 34, 14, 4, 4);
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(18, 10);
    b.ellipse(9, 5, 8, 3, 1);
    b.ellipse(5, 5, 3, 2, 2);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(26, 12);
    b.ellipse(13, 6, 11, 4, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    paintPals(vdp);
    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    vdp.B.resize(64, 32);
    gs::bitmapToPlane(tiles, vdp.B, 0, 0, cliffPicture(), PAL_CLIFF);
    vdp.A.enabled = false;
    for (int y = 0; y < gs::SCREEN_H; y++) vdp.road[y].on = false;

    for (int i = 0; i < 8; i++) art.car[i] = gs::uploadMipped(vdp, carArt(float(i) * 0.78539816f));
    art.boom = gs::uploadMipped(vdp, boomArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    gs::TextStyle banner{2, 1, 15, 0, 1};
    art.banner = gs::uploadMipped(vdp, gs::textBitmap("CLIFF BOOM", banner));
}

}  // namespace cliffboom

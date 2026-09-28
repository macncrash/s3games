#include "game/art.h"

#include <cmath>
#include <string>

namespace cranegrass {
namespace {

void textPal(gs::VDP& v, int pal, uint16_t ink) {
    v.setColor(pal * 16 + 1, ink);
    v.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

void paintPals(gs::VDP& vdp) {
    textPal(vdp, PAL_HUD, gs::rgb4(15, 15, 14));
    textPal(vdp, PAL_AMBER, gs::rgb4(15, 12, 3));
    textPal(vdp, PAL_RED, gs::rgb4(15, 4, 3));
    textPal(vdp, PAL_GREEN, gs::rgb4(6, 15, 7));

    const uint16_t yard[16] = {
        0,
        gs::rgb4(3, 8, 3),
        gs::rgb4(5, 11, 4),
        gs::rgb4(8, 13, 5),
        gs::rgb4(2, 6, 2),
        gs::rgb4(7, 7, 6),
        gs::rgb4(10, 10, 9),
        gs::rgb4(4, 4, 4),
        gs::rgb4(2, 6, 11),
        gs::rgb4(4, 9, 14),
        gs::rgb4(8, 13, 15),
        gs::rgb4(2, 5, 2),
        gs::rgb4(6, 4, 2),
        gs::rgb4(9, 6, 3),
        gs::rgb4(12, 14, 6),
        gs::rgb4(1, 2, 1),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_YARD * 16 + i, yard[i]);

    vdp.setColor(PAL_CRANE * 16 + 1, gs::rgb4(14, 12, 4));
    vdp.setColor(PAL_CRANE * 16 + 2, gs::rgb4(10, 8, 2));
    vdp.setColor(PAL_CRANE * 16 + 3, gs::rgb4(5, 4, 2));
    vdp.setColor(PAL_CRANE * 16 + 4, gs::rgb4(13, 14, 15));
    vdp.setColor(PAL_CRANE * 16 + 5, gs::rgb4(3, 5, 8));
    vdp.setColor(PAL_CRANE * 16 + 6, gs::rgb4(15, 8, 2));
    vdp.setColor(PAL_CRANE * 16 + 7, gs::rgb4(2, 2, 2));
    vdp.setColor(PAL_CRANE * 16 + 8, gs::rgb4(8, 8, 9));

    vdp.setColor(PAL_BOOM * 16 + 1, gs::rgb4(12, 13, 14));
    vdp.setColor(PAL_BOOM * 16 + 2, gs::rgb4(6, 7, 8));
    vdp.setColor(PAL_BOOM * 16 + 3, gs::rgb4(15, 12, 2));
    vdp.setColor(PAL_BOOM * 16 + 4, gs::rgb4(2, 2, 3));

    vdp.setColor(PAL_FX * 16 + 1, gs::rgb4(12, 12, 11));
    vdp.setColor(PAL_FX * 16 + 2, gs::rgb4(7, 7, 6));
    vdp.setColor(PAL_FX * 16 + 3, gs::rgb4(1, 1, 1));
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

gs::Bitmap rotate(const gs::Bitmap& src, float ang) {
    const int w = src.w, h = src.h;
    gs::Bitmap o(w, h);
    const float cx = (w - 1) * 0.5f, cy = (h - 1) * 0.5f;
    const float c = std::cos(ang), s = std::sin(ang);
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            float dx = x - cx, dy = y - cy;
            int sx = int(std::lround(cx + c * dx + s * dy));
            int sy = int(std::lround(cy - s * dx + c * dy));
            int p = src.get(sx, sy);
            if (p) o.set(x, y, p);
        }
    }
    return o;
}

gs::Bitmap craneNorth() {
    gs::Bitmap b(40, 72);
    // counterweight
    b.rect(10, 50, 20, 14, 2);
    b.rect(12, 52, 16, 10, 1);
    b.rect(14, 58, 12, 3, 3);
    // slewing deck
    b.ellipse(20, 40, 12, 11, 2);
    b.ellipse(20, 39, 9, 8, 1);
    b.rect(16, 34, 8, 6, 5);
    b.rect(17, 35, 3, 3, 4);
    // cab glass
    b.rect(22, 33, 5, 5, 4);
    // chassis / carriers
    b.rect(6, 28, 6, 28, 8);
    b.rect(28, 28, 6, 28, 8);
    b.rect(7, 30, 4, 24, 3);
    b.rect(29, 30, 4, 24, 3);
    for (int i = 0; i < 5; i++) {
        b.rect(7, 31 + i * 5, 4, 2, 7);
        b.rect(29, 31 + i * 5, 4, 2, 7);
    }
    // lattice boom, pointing north (up)
    b.rect(17, 4, 6, 32, 2);
    b.rect(18, 5, 4, 30, 1);
    for (int y = 6; y < 34; y += 4) {
        b.line(18, float(y), 22, float(y + 3), 4, 1);
        b.line(22, float(y), 18, float(y + 3), 4, 1);
    }
    b.rect(16, 2, 8, 4, 6);
    b.rect(19, 0, 2, 4, 7);
    // hook line
    b.line(20, 4, 20, 16, 7, 1);
    b.rect(18, 16, 4, 3, 6);
    return b;
}

gs::Bitmap grassPad() {
    gs::Bitmap b(64, 80);
    b.rect(0, 0, 64, 80, 2);
    for (int y = 0; y < 80; y++) {
        for (int x = 0; x < 64; x++) {
            int n = (x * 17 + y * 13) ^ (x * y);
            int c = 2;
            if ((n & 7) == 0) c = 1;
            else if ((n & 7) == 1) c = 3;
            else if ((n & 31) == 2) c = 4;
            if (x < 2 || y < 2 || x > 61 || y > 77) c = 4;
            b.set(x, y, c);
        }
    }
    // pale landing ticks
    b.rect(8, 36, 48, 2, 14);
    b.rect(30, 20, 2, 36, 14);
    return b;
}

gs::Bitmap slab() {
    gs::Bitmap b(40, 48);
    b.rect(0, 0, 40, 48, 6);
    b.rect(1, 0, 2, 48, 5);
    b.rect(37, 0, 2, 48, 7);
    for (int y = 6; y < 48; y += 12) b.rect(4, y, 32, 1, 7);
    b.rect(18, 0, 4, 48, 14);
    return b;
}

gs::Bitmap water() {
    gs::Bitmap b(48, 32);
    for (int y = 0; y < 32; y++)
        for (int x = 0; x < 48; x++) {
            int c = ((x + y * 3) & 4) ? 9 : 8;
            if (((x * 5 + y) & 15) == 0) c = 10;
            b.set(x, y, c);
        }
    return b;
}

gs::Bitmap tree() {
    gs::Bitmap b(18, 22);
    b.rect(8, 12, 3, 10, 12);
    b.ellipse(9, 8, 7, 7, 11);
    b.ellipse(8, 7, 4, 4, 1);
    return b;
}

gs::Bitmap puff() {
    gs::Bitmap b(12, 12);
    b.ellipse(6, 6, 5, 4, 2);
    b.ellipse(5, 5, 3, 2, 1);
    return b;
}

gs::Bitmap shadow() {
    gs::Bitmap b(28, 16);
    b.ellipse(14, 8, 12, 6, 3);
    return b;
}

gs::Bitmap legs() {
    gs::Bitmap b(36, 20);
    b.rect(2, 8, 32, 4, 2);
    b.rect(0, 4, 6, 12, 1);
    b.rect(30, 4, 6, 12, 1);
    b.rect(1, 2, 4, 3, 3);
    b.rect(31, 15, 4, 3, 3);
    b.rect(16, 6, 4, 8, 4);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    paintPals(vdp);
    vdp.setFogColor(gs::rgb4(8, 11, 13));
    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, tiles, art);

    art.grass = gs::uploadMipped(vdp, grassPad());
    art.slab = gs::uploadMipped(vdp, slab());
    art.water = gs::uploadMipped(vdp, water());
    art.tree = gs::uploadMipped(vdp, tree());
    art.puff = gs::uploadMipped(vdp, puff());
    art.shadow = gs::uploadMipped(vdp, shadow());
    art.legs = gs::uploadMipped(vdp, legs());

    gs::Bitmap north = craneNorth();
    // Bitmap y grows south. World heading 0 is east. North is +pi/2, which is
    // a -90 degree turn of a sprite whose nose points up (negative bitmap y).
    const float pi = 3.14159265f;
    for (int i = 0; i < HEADINGS; i++) {
        float heading = (pi * 2.f * float(i)) / float(HEADINGS);
        float ang = heading - pi * 0.5f;
        art.crane[i] = gs::uploadMipped(vdp, rotate(north, ang));
    }
}

}  // namespace cranegrass

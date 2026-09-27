#include "game/art.h"

namespace hbann {
namespace {

void pal(gs::VDP& v, int p, int i, int r, int g, int b) { v.setColor(p * 16 + i, gs::rgb4(r, g, b)); }

gs::Bitmap boatBmp() {
    gs::Bitmap b(40, 56);
    b.poly({{20, 2}, {36, 18}, {33, 50}, {7, 50}, {4, 18}}, 1);
    b.poly({{20, 8}, {30, 20}, {28, 44}, {12, 44}, {10, 20}}, 2);
    b.rect(17, 16, 6, 16, 3);
    b.rect(16, 14, 8, 4, 4);
    b.ellipse(20, 30, 5, 4, 5);
    b.rect(14, 46, 12, 3, 6);
    b.outline(7, false);
    return b;
}

gs::Bitmap cutterBmp() {
    gs::Bitmap b(44, 52);
    b.poly({{22, 2}, {40, 16}, {36, 46}, {8, 46}, {4, 16}}, 1);
    b.poly({{22, 10}, {32, 18}, {30, 38}, {14, 38}, {12, 18}}, 2);
    b.rect(18, 8, 8, 10, 3);
    b.rect(20, 4, 4, 6, 4);
    b.rect(10, 22, 6, 4, 5);
    b.rect(28, 22, 6, 4, 5);
    b.outline(6, false);
    return b;
}

gs::Bitmap raftBmp() {
    gs::Bitmap b(36, 28);
    b.ellipse(18, 16, 16, 8, 1);
    b.rect(8, 12, 20, 6, 2);
    b.rect(16, 4, 4, 14, 3);
    b.rect(14, 2, 8, 3, 4);
    return b;
}

gs::Bitmap pierBmp() {
    gs::Bitmap b(28, 48);
    b.rect(6, 4, 16, 40, 1);
    b.rect(4, 8, 4, 6, 2);
    b.rect(20, 8, 4, 6, 2);
    b.rect(4, 22, 4, 6, 2);
    b.rect(20, 22, 4, 6, 2);
    b.rect(4, 36, 4, 6, 2);
    b.rect(20, 36, 4, 6, 2);
    b.rect(10, 0, 8, 6, 3);
    return b;
}

gs::Bitmap bannerBmp() {
    gs::Bitmap b(28, 32);
    b.rect(13, 2, 3, 28, 3);
    b.poly({{16, 4}, {26, 8}, {24, 18}, {16, 15}}, 1);
    b.poly({{16, 15}, {24, 18}, {22, 26}, {16, 22}}, 2);
    b.rect(12, 28, 5, 3, 4);
    return b;
}

gs::Bitmap wakeBmp() {
    gs::Bitmap b(24, 12);
    b.ellipse(12, 6, 10, 4, 1);
    b.ellipse(12, 6, 5, 2, 2);
    return b;
}

gs::Bitmap splashBmp() {
    gs::Bitmap b(20, 16);
    b.ellipse(10, 10, 8, 4, 1);
    b.line(10, 2, 10, 8, 2, 2);
    b.line(4, 6, 8, 10, 2, 1);
    b.line(16, 6, 12, 10, 2, 1);
    return b;
}

gs::Bitmap buoyBmp() {
    gs::Bitmap b(12, 18);
    b.ellipse(6, 8, 5, 5, 1);
    b.rect(5, 12, 2, 5, 2);
    b.ellipse(6, 6, 2, 2, 3);
    return b;
}

gs::Bitmap lampBmp() {
    gs::Bitmap b(12, 28);
    b.rect(5, 10, 2, 16, 1);
    b.rect(3, 24, 6, 3, 2);
    b.ellipse(6, 7, 4, 4, 3);
    return b;
}

gs::Bitmap sunBmp() {
    gs::Bitmap b(20, 20);
    b.ellipse(10, 10, 7, 7, 1);
    b.ellipse(8, 8, 3, 3, 2);
    return b;
}

gs::Bitmap cloudBmp() {
    gs::Bitmap b(36, 16);
    b.ellipse(12, 9, 8, 5, 1);
    b.ellipse(22, 8, 10, 6, 1);
    return b;
}

gs::Bitmap plateBmp() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 1);
    return b;
}

void glyphs(gs::VDP& vdp, Art& art) {
    for (int c = 32; c < 96; ++c) {
        gs::Bitmap b(14, 16);
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; ++y)
            for (int x = 0; x < 5; ++x)
                if (g[y * 5 + x]) b.rect(float(x * 2 + 2), float(y * 2 + 1), 2, 2, 1);
        b.outline(2, false);
        art.glyph[c] = gs::uploadImage(vdp, b);
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    pal(vdp, PAL_TEXT, 1, 15, 15, 15);
    pal(vdp, PAL_TEXT, 2, 1, 1, 3);
    pal(vdp, PAL_AMBER, 1, 15, 12, 4);
    pal(vdp, PAL_AMBER, 2, 3, 1, 0);
    pal(vdp, PAL_RED, 1, 15, 5, 3);
    pal(vdp, PAL_RED, 2, 3, 0, 0);
    pal(vdp, PAL_DIM, 1, 8, 12, 12);
    pal(vdp, PAL_DIM, 2, 1, 2, 3);

    pal(vdp, PAL_BOAT, 1, 3, 5, 6);
    pal(vdp, PAL_BOAT, 2, 6, 9, 10);
    pal(vdp, PAL_BOAT, 3, 12, 10, 4);
    pal(vdp, PAL_BOAT, 4, 14, 13, 8);
    pal(vdp, PAL_BOAT, 5, 2, 3, 4);
    pal(vdp, PAL_BOAT, 6, 8, 4, 2);
    pal(vdp, PAL_BOAT, 7, 1, 1, 2);

    pal(vdp, PAL_CUTTER, 1, 4, 4, 5);
    pal(vdp, PAL_CUTTER, 2, 7, 7, 8);
    pal(vdp, PAL_CUTTER, 3, 12, 3, 2);
    pal(vdp, PAL_CUTTER, 4, 3, 3, 3);
    pal(vdp, PAL_CUTTER, 5, 10, 9, 6);
    pal(vdp, PAL_CUTTER, 6, 1, 1, 1);

    pal(vdp, PAL_BANNER, 1, 13, 2, 2);
    pal(vdp, PAL_BANNER, 2, 14, 11, 3);
    pal(vdp, PAL_BANNER, 3, 6, 4, 2);
    pal(vdp, PAL_BANNER, 4, 14, 14, 12);

    pal(vdp, PAL_FX, 1, 10, 14, 15);
    pal(vdp, PAL_FX, 2, 15, 15, 15);
    pal(vdp, PAL_FX, 3, 14, 8, 2);

    pal(vdp, PAL_QUAY, 1, 8, 6, 3);
    pal(vdp, PAL_QUAY, 2, 5, 4, 2);
    pal(vdp, PAL_QUAY, 3, 12, 11, 8);

    pal(vdp, PAL_WAKE, 1, 14, 2, 2);
    pal(vdp, PAL_WAKE, 2, 3, 3, 4);
    pal(vdp, PAL_WAKE, 3, 15, 14, 6);

    pal(vdp, PAL_SKY, 1, 15, 15, 10);
    pal(vdp, PAL_SKY, 2, 15, 13, 6);
    pal(vdp, PAL_SKY, 3, 13, 13, 14);

    pal(vdp, PAL_PLATE, 1, 1, 2, 4);

    pal(vdp, PAL_ROAD, 1, 2, 6, 10);
    pal(vdp, PAL_ROAD, 2, 1, 4, 8);
    pal(vdp, PAL_ROAD, 3, 3, 8, 12);
    pal(vdp, PAL_ROAD, 4, 4, 9, 13);
    pal(vdp, PAL_ROAD, 5, 1, 3, 6);
    pal(vdp, PAL_ROAD, 6, 2, 5, 8);
    pal(vdp, PAL_ROAD, 7, 1, 3, 5);
    pal(vdp, PAL_ROAD, 8, 5, 8, 6);
    pal(vdp, PAL_ROAD, 9, 3, 6, 4);
    pal(vdp, PAL_ROAD, 10, 6, 10, 8);
    pal(vdp, PAL_ROAD, 11, 2, 7, 9);
    pal(vdp, PAL_ROAD, 12, 4, 10, 12);
    pal(vdp, PAL_ROAD, 13, 8, 12, 14);
    pal(vdp, PAL_ROAD, 14, 7, 7, 5);
    pal(vdp, PAL_ROAD, 15, 10, 12, 11);

    vdp.setFogColor(gs::rgb4(5, 7, 9));
    art.boat = gs::uploadMipped(vdp, boatBmp());
    art.cutter = gs::uploadMipped(vdp, cutterBmp());
    art.raft = gs::uploadMipped(vdp, raftBmp());
    art.pier = gs::uploadMipped(vdp, pierBmp());
    art.banner = gs::uploadImage(vdp, bannerBmp());
    art.wake = gs::uploadImage(vdp, wakeBmp());
    art.splash = gs::uploadImage(vdp, splashBmp());
    art.buoy = gs::uploadImage(vdp, buoyBmp());
    art.lamp = gs::uploadImage(vdp, lampBmp());
    art.sun = gs::uploadImage(vdp, sunBmp());
    art.cloud = gs::uploadImage(vdp, cloudBmp());
    art.plate = gs::uploadImage(vdp, plateBmp());
    glyphs(vdp, art);
}

}  // namespace hbann

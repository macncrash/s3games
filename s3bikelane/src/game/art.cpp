#include "game/art.h"

#include <cstring>

namespace lane {
namespace {

void pal(gs::VDP& v, int p, int i, int r, int g, int b) { v.setColor(p * 16 + i, gs::rgb4(r, g, b)); }

void palettes(gs::VDP& v) {
    for (int p = 0; p < 16; p++) pal(v, p, 0, 0, 0, 0);

    pal(v, PAL_HUD, 1, 14, 15, 14);
    pal(v, PAL_HUD, 2, 2, 3, 4);
    pal(v, PAL_HUD, 3, 15, 13, 4);
    pal(v, PAL_HUD, 4, 15, 7, 3);

    pal(v, PAL_BIKE, 1, 2, 2, 3);
    pal(v, PAL_BIKE, 2, 15, 12, 2);
    pal(v, PAL_BIKE, 3, 12, 8, 1);
    pal(v, PAL_BIKE, 4, 4, 8, 12);
    pal(v, PAL_BIKE, 5, 14, 6, 3);
    pal(v, PAL_BIKE, 6, 5, 5, 6);
    pal(v, PAL_BIKE, 7, 9, 9, 10);
    pal(v, PAL_BIKE, 8, 8, 5, 3);
    pal(v, PAL_BIKE, 9, 15, 14, 11);

    pal(v, PAL_CONE, 1, 15, 9, 1);
    pal(v, PAL_CONE, 2, 15, 15, 13);
    pal(v, PAL_CONE, 3, 6, 4, 2);
    pal(v, PAL_CONE, 4, 12, 6, 1);

    pal(v, PAL_TREE, 1, 2, 6, 3);
    pal(v, PAL_TREE, 2, 3, 9, 4);
    pal(v, PAL_TREE, 3, 5, 4, 2);
    pal(v, PAL_TREE, 4, 8, 6, 3);

    pal(v, PAL_TITLE, 1, 15, 15, 13);
    pal(v, PAL_TITLE, 2, 2, 4, 6);
    pal(v, PAL_TITLE, 3, 15, 11, 2);
    pal(v, PAL_TITLE, 4, 4, 13, 8);

    pal(v, PAL_ALERT, 1, 15, 7, 3);
    pal(v, PAL_ALERT, 2, 5, 1, 2);

    pal(v, PAL_GATE, 1, 15, 15, 15);
    pal(v, PAL_GATE, 2, 2, 2, 3);
    pal(v, PAL_GATE, 3, 14, 3, 3);
    pal(v, PAL_GATE, 4, 15, 12, 2);

    // Road bank: 1-3 grass, 4-5 verge, 6-7 tarmac, 14 paint, 15 speck.
    pal(v, PAL_ROAD, 1, 3, 8, 3);
    pal(v, PAL_ROAD, 2, 2, 6, 2);
    pal(v, PAL_ROAD, 3, 5, 10, 4);
    pal(v, PAL_ROAD, 4, 6, 7, 4);
    pal(v, PAL_ROAD, 5, 4, 5, 3);
    pal(v, PAL_ROAD, 6, 5, 5, 6);
    pal(v, PAL_ROAD, 7, 3, 3, 4);
    pal(v, PAL_ROAD, 8, 7, 6, 4);
    pal(v, PAL_ROAD, 14, 14, 12, 3);
    pal(v, PAL_ROAD, 15, 7, 7, 8);

    v.setFogColor(gs::rgb4(6, 8, 10));
}

void fontTiles(gs::TileAlloc& tiles, int* out) {
    uint8_t px[64];
    for (int ch = 32; ch < 127; ch++) {
        const uint8_t* g = gs::glyph(char(ch));
        std::memset(px, 0, sizeof px);
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) px[y * 8 + x + 1] = 1;
        out[ch] = tiles.shared(px);
    }
}

gs::Bitmap label(const char* s, int color, int scale) {
    gs::TextStyle st;
    st.scale = scale;
    st.color = color;
    st.outline = 2;
    st.spacing = 1;
    return gs::textBitmap(s, st);
}

void bikeFrame(gs::Bitmap& b, int lean) {
    b.ellipse(16.f, 34.f, 8.f, 8.f, 1);
    b.ellipse(36.f, 34.f, 8.f, 8.f, 1);
    b.ellipse(16.f, 34.f, 3.f, 3.f, 7);
    b.ellipse(36.f, 34.f, 3.f, 3.f, 7);
    b.line(16, 28, 36, 28, 6, 2);
    b.line(20, 28, 28 + lean, 14, 2, 2);
    b.line(34, 28, 28 + lean, 16, 2, 2);
    b.rect(22 + lean, 12, 14, 6, 2);
    b.rect(24 + lean, 8, 8, 6, 4);
    b.ellipse(28.f + lean, 7.f, 5.f, 4.f, 5);
    b.rect(26 + lean, 5, 3, 2, 9);
    b.rect(18 + lean, 18, 16, 3, 3);
    b.line(30 + lean, 14, 40, 18, 8, 2);
}

void coneBmp(gs::Bitmap& b) {
    b.poly({{4.f, 28.f}, {20.f, 28.f}, {14.f, 4.f}, {10.f, 4.f}}, 1);
    b.rect(8, 12, 8, 3, 2);
    b.rect(9, 18, 6, 3, 2);
    b.rect(6, 28, 12, 3, 3);
}

void treeBmp(gs::Bitmap& b) {
    b.ellipse(16.f, 16.f, 12.f, 14.f, 1);
    b.ellipse(12.f, 14.f, 6.f, 7.f, 2);
    b.rect(14, 26, 4, 14, 3);
    b.rect(10, 38, 12, 3, 4);
}

void gateBmp(gs::Bitmap& b) {
    b.rect(2, 4, 6, 44, 2);
    b.rect(56, 4, 6, 44, 2);
    b.rect(2, 2, 60, 8, 4);
    for (int i = 0; i < 6; i++) b.rect(8 + i * 8, 4, 4, 4, (i & 1) ? 1 : 3);
    b.rect(8, 16, 48, 4, 1);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    palettes(vdp);
    gs::TileAlloc tiles(vdp, 1);
    fontTiles(tiles, art.font);

    for (int i = 0; i < 3; i++) {
        gs::Bitmap b(52, 48);
        bikeFrame(b, (i - 1) * 5);
        art.bike[i] = gs::uploadMipped(vdp, b);
    }
    gs::Bitmap sh(44, 12);
    sh.ellipse(22.f, 6.f, 18.f, 4.f, 1);
    art.shadow = gs::uploadMipped(vdp, sh);

    gs::Bitmap cone(24, 36);
    coneBmp(cone);
    art.cone = gs::uploadMipped(vdp, cone);

    gs::Bitmap tree(32, 44);
    treeBmp(tree);
    art.tree = gs::uploadMipped(vdp, tree);

    gs::Bitmap gate(64, 52);
    gateBmp(gate);
    art.gate = gs::uploadMipped(vdp, gate);

    art.title = gs::uploadImage(vdp, label("BIKE LANE", 1, 3));
    art.sub = gs::uploadImage(vdp, label("STAY IN THE LANE", 3, 1));
    art.rule = gs::uploadImage(vdp, label("MISS THE END  THE LEG FAILS", 1, 1));
    art.made = gs::uploadImage(vdp, label("LANE HELD", 4, 3));
    art.out = gs::uploadImage(vdp, label("LEFT THE LANE", 1, 2));
    art.missed = gs::uploadImage(vdp, label("MISSED THE END", 1, 2));
}

}  // namespace lane

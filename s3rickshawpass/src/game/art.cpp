#include "art.h"

#include <initializer_list>

namespace pass {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
    vdp.setColor(pal * 16 + 0, 0);
}

void roadPal(gs::VDP& vdp, int pal, bool snow) {
    auto C = gs::rgb4;
    uint16_t cols[16] = {};
    if (!snow) {
        cols[1] = C(3, 4, 2);
        cols[2] = C(5, 6, 3);
        cols[3] = C(7, 7, 4);
        cols[4] = C(6, 5, 3);
        cols[5] = C(4, 3, 2);
        cols[6] = C(6, 5, 4);
        cols[7] = C(4, 4, 3);
        cols[8] = C(8, 7, 6);
        cols[9] = C(3, 3, 2);
        cols[10] = C(9, 8, 6);
        cols[11] = C(4, 6, 8);
        cols[12] = C(5, 7, 9);
        cols[13] = C(8, 10, 12);
        cols[14] = C(12, 12, 11);
        cols[15] = C(9, 8, 7);
    } else {
        cols[1] = C(10, 11, 12);
        cols[2] = C(13, 13, 14);
        cols[3] = C(8, 9, 10);
        cols[4] = C(11, 11, 12);
        cols[5] = C(8, 9, 11);
        cols[6] = C(12, 12, 13);
        cols[7] = C(9, 10, 11);
        cols[8] = C(14, 14, 15);
        cols[9] = C(7, 8, 10);
        cols[10] = C(15, 15, 15);
        cols[11] = C(6, 8, 10);
        cols[12] = C(8, 10, 12);
        cols[13] = C(11, 12, 14);
        cols[14] = C(15, 15, 15);
        cols[15] = C(13, 14, 15);
    }
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, cols[i]);
}

gs::Bitmap cabArt() {
    gs::Bitmap b(64, 36);
    b.poly({{4.f, 28.f}, {10.f, 16.f}, {22.f, 10.f}, {46.f, 10.f}, {56.f, 18.f}, {60.f, 28.f}, {54.f, 32.f}, {8.f, 32.f}}, 1);
    b.rect(18, 14, 22, 10, 2);
    b.rect(42, 16, 8, 6, 3);
    b.ellipse(14, 30, 6, 6, 4);
    b.ellipse(50, 30, 6, 6, 4);
    b.ellipse(14, 30, 2, 2, 5);
    b.ellipse(50, 30, 2, 2, 5);
    b.rect(24, 8, 14, 4, 6);
    b.outline(7, false);
    return b;
}

gs::Bitmap pullerArt() {
    gs::Bitmap b(22, 36);
    b.ellipse(11, 7, 5, 5, 1);
    b.rect(8, 12, 6, 12, 2);
    b.line(8, 14, 2, 22, 3, 2);
    b.line(14, 14, 20, 22, 3, 2);
    b.line(9, 24, 6, 34, 4, 2);
    b.line(13, 24, 16, 34, 4, 2);
    b.rect(4, 20, 14, 2, 5);
    b.outline(6, false);
    return b;
}

gs::Bitmap rockArt() {
    gs::Bitmap b(40, 28);
    b.poly({{2.f, 24.f}, {8.f, 12.f}, {16.f, 6.f}, {28.f, 4.f}, {38.f, 16.f}, {36.f, 26.f}, {6.f, 26.f}}, 1);
    b.poly({{12.f, 16.f}, {18.f, 10.f}, {26.f, 12.f}, {24.f, 20.f}}, 2);
    b.rect(10, 20, 16, 4, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap pineArt() {
    gs::Bitmap b(28, 48);
    b.poly({{14.f, 2.f}, {26.f, 22.f}, {4.f, 22.f}}, 1);
    b.poly({{14.f, 12.f}, {26.f, 34.f}, {2.f, 34.f}}, 2);
    b.poly({{14.f, 22.f}, {24.f, 42.f}, {4.f, 42.f}}, 1);
    b.rect(12, 40, 4, 8, 3);
    return b;
}

gs::Bitmap gateArt() {
    gs::Bitmap b(48, 40);
    b.rect(2, 10, 4, 28, 1);
    b.rect(42, 10, 4, 28, 1);
    b.poly({{0.f, 12.f}, {24.f, 2.f}, {48.f, 12.f}, {44.f, 16.f}, {24.f, 8.f}, {4.f, 16.f}}, 2);
    b.rect(18, 18, 12, 16, 3);
    b.rect(6, 20, 8, 6, 4);
    b.rect(34, 20, 8, 6, 4);
    b.outline(5, false);
    return b;
}

void loadFont(gs::VDP& vdp, Art& art) {
    gs::TileAlloc tiles(vdp);
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (!g[y * 5 + x]) continue;
                px[y * 8 + x + 1] = 1;
                if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
            }
        }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        art.font[c - 32] = t;
    }
}

gs::Mipped words(gs::VDP& vdp, const char* text, int scale, int fill, int edge) {
    gs::TextStyle st{scale, fill, edge, 15, 1};
    return gs::uploadMipped(vdp, gs::textBitmap(text, st));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(8, 9, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_CAB,
           {0, gs::rgb4(12, 3, 2), gs::rgb4(14, 11, 5), gs::rgb4(5, 7, 9), gs::rgb4(2, 2, 3), gs::rgb4(14, 12, 8),
            gs::rgb4(10, 4, 3), gs::rgb4(2, 1, 1), ink});
    setPal(vdp, PAL_ROCK, {0, gs::rgb4(7, 6, 5), gs::rgb4(10, 9, 8), gs::rgb4(5, 4, 4), gs::rgb4(2, 2, 2), ink});
    setPal(vdp, PAL_PINE, {0, gs::rgb4(2, 6, 3), gs::rgb4(3, 8, 4), gs::rgb4(5, 3, 2), ink});
    setPal(vdp, PAL_GATE, {0, gs::rgb4(6, 3, 2), gs::rgb4(10, 4, 3), gs::rgb4(3, 3, 4), gs::rgb4(14, 12, 6), gs::rgb4(2, 1, 1), ink});
    setPal(vdp, PAL_STORM, {0, gs::rgb4(14, 14, 15), gs::rgb4(6, 7, 10), gs::rgb4(15, 15, 15), ink});
    roadPal(vdp, PAL_ROAD, false);
    roadPal(vdp, PAL_SNOW, true);
    loadFont(vdp, art);
    art.cab = gs::uploadMipped(vdp, cabArt());
    art.puller = gs::uploadMipped(vdp, pullerArt());
    art.rock = gs::uploadMipped(vdp, rockArt());
    art.pine = gs::uploadMipped(vdp, pineArt());
    art.gate = gs::uploadMipped(vdp, gateArt());
    art.banner = words(vdp, "THE PASS", 3, 1, 2);
}

}  // namespace pass

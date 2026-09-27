#include "art.h"

#include <initializer_list>

namespace bargepass {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
    while (i < 15) vdp.setColor(pal * 16 + i++, 0);
    vdp.setColor(pal * 16 + 0, 0);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

gs::Bitmap hullArt() {
    gs::Bitmap b(88, 28);
    b.poly({{6.f, 14.f}, {22.f, 3.f}, {70.f, 3.f}, {82.f, 10.f}, {82.f, 18.f}, {70.f, 25.f}, {22.f, 25.f}}, 1);
    b.poly({{24.f, 7.f}, {66.f, 7.f}, {74.f, 14.f}, {66.f, 21.f}, {24.f, 21.f}}, 2);
    b.rect(30, 9, 22, 10, 3);
    b.rect(34, 11, 6, 5, 6);
    b.rect(42, 11, 6, 5, 6);
    b.rect(56, 8, 8, 12, 4);
    b.rect(10, 11, 8, 6, 7);
    for (int x = 26; x < 64; x += 8) b.rect(x, 4, 2, 3, 8);
    b.outline(9, false);
    return b;
}

gs::Bitmap rivalArt() {
    gs::Bitmap b(72, 22);
    b.poly({{4.f, 11.f}, {16.f, 3.f}, {56.f, 3.f}, {68.f, 9.f}, {68.f, 14.f}, {56.f, 19.f}, {16.f, 19.f}}, 1);
    b.rect(22, 7, 18, 8, 2);
    b.rect(44, 6, 8, 10, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap cliffArt() {
    gs::Bitmap b(36, 48);
    b.rect(0, 8, 36, 40, 1);
    b.poly({{0.f, 14.f}, {8.f, 2.f}, {18.f, 10.f}, {28.f, 0.f}, {36.f, 12.f}, {36.f, 48.f}, {0.f, 48.f}}, 2);
    for (int y = 16; y < 46; y += 10) b.rect(4, y, 12, 3, 3);
    for (int y = 20; y < 44; y += 12) b.rect(20, y, 10, 3, 4);
    b.rect(0, 40, 36, 8, 5);
    return b;
}

gs::Bitmap pineArt() {
    gs::Bitmap b(24, 40);
    b.rect(10, 26, 4, 12, 3);
    b.poly({{12.f, 2.f}, {22.f, 28.f}, {2.f, 28.f}}, 1);
    b.poly({{12.f, 10.f}, {20.f, 30.f}, {4.f, 30.f}}, 2);
    return b;
}

gs::Bitmap archArt() {
    gs::Bitmap b(28, 56);
    b.rect(4, 8, 20, 48, 1);
    b.rect(8, 12, 12, 40, 2);
    b.rect(0, 0, 28, 10, 3);
    b.rect(10, 2, 8, 6, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap foamArt() {
    gs::Bitmap b(20, 8);
    b.ellipse(10, 4, 8, 3, 1);
    b.ellipse(5, 4, 2, 1.2f, 2);
    b.ellipse(15, 3, 2.2f, 1.2f, 2);
    return b;
}

gs::Bitmap flakeArt() {
    gs::Bitmap b(5, 5);
    b.rect(2, 0, 1, 5, 1);
    b.rect(0, 2, 5, 1, 1);
    b.set(1, 1, 2);
    b.set(3, 3, 2);
    return b;
}

gs::Bitmap cabinArt() {
    gs::Bitmap b(40, 28);
    b.rect(6, 12, 28, 14, 1);
    b.poly({{2.f, 12.f}, {20.f, 2.f}, {38.f, 12.f}}, 2);
    b.rect(16, 16, 8, 10, 3);
    b.rect(9, 15, 5, 4, 4);
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(9, 12, 13), ink});
    setPal(vdp, PAL_HULL,
           {0, gs::rgb4(11, 7, 3), gs::rgb4(5, 3, 2), gs::rgb4(14, 13, 11), gs::rgb4(3, 5, 8), gs::rgb4(15, 12, 4),
            gs::rgb4(8, 13, 15), gs::rgb4(12, 3, 2), gs::rgb4(4, 3, 2), gs::rgb4(2, 2, 2), ink});
    setPal(vdp, PAL_CLIFF,
           {0, gs::rgb4(7, 7, 8), gs::rgb4(10, 10, 11), gs::rgb4(5, 5, 6), gs::rgb4(12, 11, 10), gs::rgb4(3, 4, 3),
            ink});
    setPal(vdp, PAL_ARCH,
           {0, gs::rgb4(9, 8, 7), gs::rgb4(6, 5, 5), gs::rgb4(12, 9, 4), gs::rgb4(15, 13, 6), gs::rgb4(2, 2, 3), ink});
    setPal(vdp, PAL_PINE, {0, gs::rgb4(2, 7, 3), gs::rgb4(4, 10, 4), gs::rgb4(6, 4, 2), ink});
    setPal(vdp, PAL_RIVAL, {0, gs::rgb4(8, 4, 3), gs::rgb4(13, 12, 9), gs::rgb4(3, 4, 6), gs::rgb4(2, 1, 1), ink});
    setPal(vdp, PAL_WATER, {0, gs::rgb4(3, 8, 11), gs::rgb4(6, 12, 14), ink});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(14, 15, 15), gs::rgb4(8, 12, 14), ink});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 6), gs::rgb4(15, 15, 13), gs::rgb4(1, 4, 2), ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 5, 3), gs::rgb4(15, 13, 6), gs::rgb4(4, 1, 1), ink});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 5), gs::rgb4(8, 5, 1), gs::rgb4(15, 15, 12), ink});
    setPal(vdp, PAL_STORM, {0, gs::rgb4(14, 14, 15), gs::rgb4(9, 10, 12), ink});

    vdp.setFogColor(gs::rgb4(4, 5, 7));
    loadFont(vdp, art);
    art.hull = gs::uploadMipped(vdp, hullArt());
    art.rival = gs::uploadMipped(vdp, rivalArt());
    art.cliff = gs::uploadMipped(vdp, cliffArt());
    art.pine = gs::uploadMipped(vdp, pineArt());
    art.arch = gs::uploadMipped(vdp, archArt());
    art.foam = gs::uploadMipped(vdp, foamArt());
    art.flake = gs::uploadMipped(vdp, flakeArt());
    art.cabin = gs::uploadMipped(vdp, cabinArt());
    art.title = words(vdp, "BARGE PASS", 3, 1, 2);
    art.clear = words(vdp, "CLEAR", 3, 1, 3);
    art.fail = words(vdp, "FAILED", 3, 1, 2);
}

}  // namespace bargepass

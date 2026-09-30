#include "art.h"

#include <initializer_list>

namespace keelplat {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
    while (i < 15) vdp.setColor(pal * 16 + i++, 0);
    vdp.setColor(pal * 16 + 0, 0);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 2, 3));
}

gs::Bitmap hullArt() {
    gs::Bitmap b(80, 36);
    b.poly({{6.f, 14.f}, {18.f, 10.f}, {62.f, 10.f}, {74.f, 16.f}, {70.f, 24.f}, {14.f, 24.f}}, 1);
    b.poly({{22.f, 12.f}, {58.f, 12.f}, {64.f, 18.f}, {20.f, 18.f}}, 2);
    b.rect(30, 12, 8, 5, 3);
    b.poly({{36.f, 24.f}, {46.f, 24.f}, {44.f, 34.f}, {38.f, 34.f}}, 4);
    b.rect(40, 24, 2, 10, 5);
    b.ellipse(12, 16, 2.2f, 2.2f, 6);
    b.rect(66, 14, 5, 3, 7);
    b.line(8, 22, 72, 22, 8, 1.2f);
    b.outline(9, false);
    return b;
}

gs::Bitmap sailArt() {
    gs::Bitmap b(36, 40);
    b.line(8, 2, 8, 38, 1, 2.f);
    b.poly({{10.f, 6.f}, {32.f, 16.f}, {30.f, 28.f}, {10.f, 32.f}}, 2);
    b.line(10, 18, 30, 20, 3, 1.f);
    b.rect(4, 36, 10, 3, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap quayArt() {
    gs::Bitmap b(72, 32);
    b.rect(0, 8, 72, 18, 1);
    for (int x = 0; x < 72; x += 12) b.rect(x, 8, 11, 8, (x / 12) & 1 ? 2 : 3);
    b.rect(0, 6, 72, 3, 4);
    b.rect(4, 7, 64, 1, 5);
    for (int x = 6; x < 68; x += 16) b.rect(x, 22, 4, 10, 6);
    b.outline(7, false);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(10, 28);
    b.rect(3, 4, 4, 22, 1);
    b.rect(1, 2, 8, 5, 2);
    b.ellipse(5, 4, 2.f, 2.f, 3);
    b.rect(2, 24, 6, 3, 4);
    return b;
}

gs::Bitmap waveArt() {
    gs::Bitmap b(28, 10);
    b.ellipse(8, 6, 6, 3, 1);
    b.ellipse(18, 5, 7, 3, 2);
    b.ellipse(14, 7, 4, 2, 1);
    return b;
}

gs::Bitmap gullArt() {
    gs::Bitmap b(18, 10);
    b.poly({{1.f, 6.f}, {8.f, 3.f}, {9.f, 5.f}}, 1);
    b.poly({{17.f, 6.f}, {10.f, 3.f}, {9.f, 5.f}}, 1);
    b.ellipse(9, 6, 1.4f, 1.2f, 2);
    return b;
}

gs::Bitmap rivalArt() {
    gs::Bitmap b(40, 16);
    b.poly({{2.f, 8.f}, {10.f, 5.f}, {32.f, 5.f}, {38.f, 9.f}, {30.f, 12.f}, {8.f, 12.f}}, 1);
    b.line(16, 5, 16, 1, 2, 1.4f);
    b.poly({{16.f, 2.f}, {26.f, 6.f}, {16.f, 7.f}}, 3);
    b.outline(4, false);
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
    const uint16_t ink = gs::rgb4(1, 2, 3);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(8, 12, 13), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_HULL,
           {0, gs::rgb4(14, 13, 10), gs::rgb4(8, 6, 4), gs::rgb4(4, 8, 10), gs::rgb4(3, 3, 4), gs::rgb4(6, 6, 7),
            gs::rgb4(12, 4, 3), gs::rgb4(15, 12, 6), gs::rgb4(5, 8, 10), gs::rgb4(2, 2, 3), ink});
    setPal(vdp, PAL_SAIL, {0, gs::rgb4(6, 4, 3), gs::rgb4(15, 15, 13), gs::rgb4(10, 12, 13), gs::rgb4(8, 6, 3),
                           gs::rgb4(2, 3, 4), ink});
    setPal(vdp, PAL_QUAY, {0, gs::rgb4(8, 8, 8), gs::rgb4(11, 11, 10), gs::rgb4(6, 6, 7), gs::rgb4(12, 12, 11),
                           gs::rgb4(15, 15, 14), gs::rgb4(5, 4, 4), gs::rgb4(3, 3, 4), ink});
    setPal(vdp, PAL_WATER, {0, gs::rgb4(12, 15, 15), gs::rgb4(7, 12, 14), ink});
    setPal(vdp, PAL_CREW, {0, gs::rgb4(12, 4, 3), gs::rgb4(4, 3, 2), gs::rgb4(15, 14, 10), gs::rgb4(2, 2, 2), ink});
    setPal(vdp, PAL_POST, {0, gs::rgb4(5, 4, 3), gs::rgb4(9, 8, 6), gs::rgb4(15, 13, 4), gs::rgb4(3, 3, 3), ink});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 6), gs::rgb4(15, 15, 12), gs::rgb4(1, 5, 2), ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(15, 12, 5), gs::rgb4(5, 1, 1), ink});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 14, 8), gs::rgb4(6, 8, 12), gs::rgb4(2, 3, 5), ink});
    setPal(vdp, PAL_GULL, {0, gs::rgb4(15, 15, 15), gs::rgb4(15, 10, 3), ink});

    vdp.setFogColor(gs::rgb4(6, 9, 12));
    loadFont(vdp, art);
    art.hull = gs::uploadMipped(vdp, hullArt());
    art.sail = gs::uploadMipped(vdp, sailArt());
    art.quay = gs::uploadMipped(vdp, quayArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.wave = gs::uploadMipped(vdp, waveArt());
    art.gull = gs::uploadMipped(vdp, gullArt());
    art.rival = gs::uploadMipped(vdp, rivalArt());
    art.title = words(vdp, "KEEL PLAT", 3, 1, 2);
    art.level = words(vdp, "LEVEL", 3, 1, 3);
    art.late = words(vdp, "LATE", 3, 1, 2);
    art.paused = words(vdp, "HELD", 3, 1, 2);
}

}  // namespace keelplat

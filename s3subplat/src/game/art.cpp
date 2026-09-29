#include "art.h"

#include <initializer_list>

namespace subplat {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
    while (i < 15) vdp.setColor(pal * 16 + i++, 0);
    vdp.setColor(pal * 16 + 0, 0);
    vdp.setColor(pal * 16 + 15, gs::rgb4(0, 1, 2));
}

gs::Bitmap subArt() {
    gs::Bitmap b(72, 28);
    b.ellipse(36, 16, 30, 9, 2);
    b.poly({{8.f, 16.f}, {22.f, 8.f}, {48.f, 8.f}, {62.f, 16.f}, {48.f, 24.f}, {22.f, 24.f}}, 1);
    b.rect(28, 4, 16, 6, 3);
    b.rect(30, 2, 4, 4, 4);
    b.ellipse(46, 15, 4, 3, 5);
    b.rect(18, 13, 10, 4, 6);
    b.poly({{62.f, 12.f}, {70.f, 16.f}, {62.f, 20.f}}, 3);
    b.rect(10, 18, 8, 3, 7);
    b.outline(8, false);
    return b;
}

gs::Bitmap platArt() {
    gs::Bitmap b(96, 36);
    b.rect(0, 4, 96, 8, 1);
    b.rect(0, 2, 96, 3, 2);
    b.rect(6, 12, 8, 22, 3);
    b.rect(42, 12, 8, 22, 3);
    b.rect(82, 12, 8, 22, 3);
    b.rect(8, 16, 4, 4, 4);
    b.rect(44, 16, 4, 4, 4);
    b.rect(84, 20, 4, 8, 5);
    b.rect(20, 28, 56, 4, 6);
    b.outline(7, false);
    return b;
}

gs::Bitmap kelpArt() {
    gs::Bitmap b(16, 56);
    for (int y = 0; y < 56; y++) {
        int x = 7 + int(3 * ((y / 6) % 2 == 0 ? 1 : -1));
        b.rect(x, y, 3, 1, (y / 8) % 2 ? 1 : 2);
        if (y % 9 == 0) b.rect(x - 3, y, 4, 2, 3);
    }
    return b;
}

gs::Bitmap fishArt() {
    gs::Bitmap b(18, 10);
    b.ellipse(8, 5, 6, 3, 1);
    b.poly({{12.f, 5.f}, {17.f, 1.f}, {17.f, 9.f}}, 2);
    b.set(5, 4, 3);
    return b;
}

gs::Bitmap bubArt() {
    gs::Bitmap b(8, 8);
    b.ellipse(4, 4, 3, 3, 1);
    b.set(3, 3, 2);
    return b;
}

gs::Bitmap rivalArt() {
    gs::Bitmap b(40, 14);
    b.ellipse(18, 8, 14, 5, 1);
    b.rect(14, 2, 8, 4, 2);
    b.poly({{30.f, 5.f}, {38.f, 8.f}, {30.f, 11.f}}, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(10, 10);
    b.ellipse(5, 5, 4, 4, 1);
    b.ellipse(5, 5, 2, 2, 2);
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
    const uint16_t ink = gs::rgb4(0, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 15, 15), gs::rgb4(6, 10, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_SUB, {0, gs::rgb4(12, 13, 11), gs::rgb4(4, 8, 7), gs::rgb4(9, 10, 8), gs::rgb4(3, 3, 4),
                          gs::rgb4(8, 14, 15), gs::rgb4(15, 12, 3), gs::rgb4(6, 4, 2), gs::rgb4(1, 2, 2), 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_PLAT, {0, gs::rgb4(10, 9, 7), gs::rgb4(14, 13, 8), gs::rgb4(6, 6, 7), gs::rgb4(15, 14, 6),
                           gs::rgb4(12, 4, 3), gs::rgb4(5, 7, 8), gs::rgb4(2, 2, 3), 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_KELP, {0, gs::rgb4(2, 8, 4), gs::rgb4(4, 12, 5), gs::rgb4(1, 5, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FISH, {0, gs::rgb4(14, 10, 3), gs::rgb4(12, 5, 2), gs::rgb4(1, 1, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BUB, {0, gs::rgb4(10, 14, 15), gs::rgb4(15, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_RIVAL, {0, gs::rgb4(12, 4, 3), gs::rgb4(8, 3, 2), gs::rgb4(15, 10, 4), gs::rgb4(3, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 7), gs::rgb4(2, 5, 2), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 6, 3), gs::rgb4(4, 1, 1), gs::rgb4(15, 13, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 13, 6), gs::rgb4(2, 3, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_LIGHT, {0, gs::rgb4(15, 15, 8), gs::rgb4(15, 15, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});

    loadFont(vdp, art);
    art.sub = gs::uploadMipped(vdp, subArt());
    art.plat = gs::uploadMipped(vdp, platArt());
    art.kelp = gs::uploadMipped(vdp, kelpArt());
    art.fish = gs::uploadMipped(vdp, fishArt());
    art.bub = gs::uploadMipped(vdp, bubArt());
    art.rival = gs::uploadMipped(vdp, rivalArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.title = words(vdp, "SUBPLAT", 3, 1, 2);
    art.level = words(vdp, "LEVEL", 3, 1, 2);
    art.late = words(vdp, "LATE", 3, 1, 2);
    art.paused = words(vdp, "HOLD", 2, 1, 2);

    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.hudEnabled = true;
    vdp.setFogColor(gs::rgb4(1, 3, 6));
}

}  // namespace subplat

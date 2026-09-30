#include "art.h"

#include <initializer_list>

namespace keellock {
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
    gs::Bitmap b(40, 96);
    b.poly({{20.f, 2.f}, {34.f, 22.f}, {36.f, 70.f}, {28.f, 92.f}, {12.f, 92.f}, {4.f, 70.f}, {6.f, 22.f}}, 1);
    b.poly({{20.f, 10.f}, {28.f, 26.f}, {29.f, 68.f}, {22.f, 84.f}, {18.f, 84.f}, {11.f, 68.f}, {12.f, 26.f}}, 2);
    b.rect(18, 30, 4, 36, 3);
    b.ellipse(20, 48, 3.2f, 5.f, 4);
    b.rect(16, 78, 8, 6, 5);
    b.rect(18, 88, 4, 6, 6);
    b.line(20, 8, 20, 86, 7, 1.4f);
    b.outline(8, false);
    return b;
}

gs::Bitmap sailArt() {
    gs::Bitmap b(56, 64);
    b.poly({{28.f, 2.f}, {50.f, 58.f}, {28.f, 50.f}}, 1);
    b.poly({{28.f, 6.f}, {8.f, 54.f}, {28.f, 46.f}}, 2);
    b.line(28, 0, 28, 62, 3, 2.f);
    b.rect(24, 58, 8, 5, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap gateArt() {
    gs::Bitmap b(72, 28);
    b.rect(0, 4, 72, 20, 1);
    for (int x = 2; x < 70; x += 8) b.rect(x, 6, 6, 16, ((x / 8) & 1) ? 2 : 3);
    b.rect(0, 2, 72, 4, 4);
    b.rect(0, 22, 72, 4, 4);
    for (int x = 10; x < 64; x += 16) b.ellipse(float(x), 14.f, 2.2f, 2.2f, 5);
    b.outline(6, false);
    return b;
}

gs::Bitmap stoneArt() {
    gs::Bitmap b(48, 20);
    b.rect(0, 0, 48, 20, 1);
    for (int x = 0; x < 48; x += 12) {
        int off = ((x / 12) & 1) ? 4 : 0;
        b.rect(x + 1, 1 + (off ? 0 : 0), 10, 8, 2);
        b.rect(x + 1 - off / 2, 10, 10, 8, 3);
    }
    b.rect(0, 9, 48, 1, 4);
    return b;
}

gs::Bitmap reedArt() {
    gs::Bitmap b(24, 32);
    b.rect(0, 22, 24, 10, 1);
    for (int i = 0; i < 6; i++) {
        float x = 3.f + i * 3.4f;
        b.line(x, 28.f, x + ((i & 1) ? 2.f : -2.f), 4.f + (i % 3), (i & 1) ? 2 : 3, 1.4f);
    }
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(12, 36);
    b.rect(5, 10, 2, 24, 1);
    b.rect(2, 2, 8, 10, 2);
    b.rect(3, 4, 6, 6, 3);
    b.rect(3, 32, 6, 3, 4);
    return b;
}

gs::Bitmap foamArt() {
    gs::Bitmap b(28, 12);
    b.ellipse(14, 6, 12, 4, 1);
    b.ellipse(6, 6, 3, 2, 2);
    b.ellipse(20, 5, 4, 2, 2);
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(7, 12, 13), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_HULL,
           {0, gs::rgb4(14, 14, 12), gs::rgb4(6, 8, 10), gs::rgb4(9, 5, 3), gs::rgb4(12, 9, 5), gs::rgb4(4, 3, 2),
            gs::rgb4(8, 3, 2), gs::rgb4(3, 3, 4), gs::rgb4(2, 2, 3), ink});
    setPal(vdp, PAL_SAIL, {0, gs::rgb4(15, 15, 13), gs::rgb4(11, 13, 14), gs::rgb4(5, 4, 3), gs::rgb4(8, 6, 3),
                           gs::rgb4(2, 3, 4), ink});
    setPal(vdp, PAL_GATE, {0, gs::rgb4(9, 6, 3), gs::rgb4(6, 4, 2), gs::rgb4(12, 9, 4), gs::rgb4(4, 4, 5),
                           gs::rgb4(14, 12, 6), gs::rgb4(2, 2, 2), ink});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(7, 7, 8), gs::rgb4(10, 10, 9), gs::rgb4(5, 5, 6), gs::rgb4(3, 3, 4), ink});
    setPal(vdp, PAL_BANK, {0, gs::rgb4(3, 8, 3), gs::rgb4(7, 12, 4), gs::rgb4(2, 6, 2), gs::rgb4(9, 8, 3), ink});
    setPal(vdp, PAL_CREW, {0, gs::rgb4(15, 8, 3), gs::rgb4(8, 3, 2), gs::rgb4(15, 14, 8), ink});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(14, 15, 15), gs::rgb4(8, 12, 14), ink});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 6), gs::rgb4(15, 15, 12), gs::rgb4(1, 5, 2), ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(15, 12, 5), gs::rgb4(5, 1, 1), ink});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 4), gs::rgb4(9, 6, 1), gs::rgb4(15, 15, 12), ink});

    vdp.setFogColor(gs::rgb4(5, 8, 11));
    loadFont(vdp, art);
    art.hull = gs::uploadMipped(vdp, hullArt());
    art.sail = gs::uploadMipped(vdp, sailArt());
    art.gate = gs::uploadMipped(vdp, gateArt());
    art.stone = gs::uploadMipped(vdp, stoneArt());
    art.reed = gs::uploadMipped(vdp, reedArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.foam = gs::uploadMipped(vdp, foamArt());
    art.title = words(vdp, "KEEL LOCK", 3, 1, 2);
    art.clear = words(vdp, "CLEAR", 3, 1, 3);
    art.fail = words(vdp, "SCRAPED", 3, 1, 2);
}

}  // namespace keellock

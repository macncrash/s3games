#include "art.h"

#include <initializer_list>

namespace keelpass {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
    while (i < 15) vdp.setColor(pal * 16 + i++, 0);
    vdp.setColor(pal * 16 + 0, 0);
    vdp.setColor(pal * 16 + 15, gs::rgb4(2, 2, 3));
}

gs::Bitmap hullArt() {
    gs::Bitmap b(36, 88);
    b.poly({{18.f, 3.f}, {30.f, 20.f}, {32.f, 62.f}, {24.f, 84.f}, {12.f, 84.f}, {4.f, 62.f}, {6.f, 20.f}}, 1);
    b.poly({{18.f, 12.f}, {24.f, 24.f}, {25.f, 58.f}, {20.f, 74.f}, {16.f, 74.f}, {11.f, 58.f}, {12.f, 24.f}}, 2);
    b.rect(16, 28, 4, 28, 3);
    b.ellipse(18, 40, 2.6f, 4.2f, 4);
    b.rect(14, 70, 8, 5, 5);
    b.line(18, 8, 18, 78, 6, 1.3f);
    b.outline(7, false);
    return b;
}

gs::Bitmap sailArt() {
    gs::Bitmap b(52, 60);
    b.poly({{26.f, 2.f}, {48.f, 54.f}, {26.f, 46.f}}, 1);
    b.poly({{26.f, 6.f}, {7.f, 50.f}, {26.f, 42.f}}, 2);
    b.line(26, 0, 26, 58, 3, 2.f);
    b.rect(22, 54, 8, 5, 4);
    for (int y = 10; y < 48; y += 8) b.line(12, float(y), 42, float(y - 2), 5, 0.8f);
    b.outline(6, false);
    return b;
}

gs::Bitmap cliffArt() {
    gs::Bitmap b(40, 48);
    b.poly({{2.f, 48.f}, {8.f, 8.f}, {18.f, 0.f}, {34.f, 14.f}, {38.f, 48.f}}, 1);
    b.poly({{10.f, 40.f}, {14.f, 16.f}, {20.f, 8.f}, {28.f, 18.f}, {30.f, 40.f}}, 2);
    b.rect(16, 22, 6, 8, 3);
    b.line(6, 46, 36, 46, 4, 2.f);
    b.outline(5, false);
    return b;
}

gs::Bitmap pineArt() {
    gs::Bitmap b(20, 36);
    b.poly({{10.f, 1.f}, {18.f, 18.f}, {2.f, 18.f}}, 1);
    b.poly({{10.f, 10.f}, {17.f, 28.f}, {3.f, 28.f}}, 2);
    b.rect(8, 28, 4, 7, 3);
    return b;
}

gs::Bitmap foamArt() {
    gs::Bitmap b(28, 12);
    b.ellipse(14, 6, 12, 4, 1);
    b.ellipse(7, 6, 3.2f, 2.f, 2);
    b.ellipse(20, 5, 4, 2, 2);
    return b;
}

gs::Bitmap cloudArt() {
    gs::Bitmap b(48, 20);
    b.ellipse(16, 12, 12, 6, 1);
    b.ellipse(28, 10, 14, 7, 1);
    b.ellipse(36, 13, 8, 5, 2);
    b.ellipse(12, 13, 6, 4, 2);
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(8, 11, 13), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_HULL,
           {0, gs::rgb4(14, 13, 11), gs::rgb4(5, 7, 9), gs::rgb4(10, 5, 3), gs::rgb4(13, 10, 5), gs::rgb4(4, 3, 2),
            gs::rgb4(7, 3, 2), gs::rgb4(2, 2, 3), ink});
    setPal(vdp, PAL_SAIL, {0, gs::rgb4(15, 15, 13), gs::rgb4(12, 13, 14), gs::rgb4(4, 3, 2), gs::rgb4(8, 6, 3),
                           gs::rgb4(9, 8, 6), gs::rgb4(2, 2, 3), ink});
    setPal(vdp, PAL_CLIFF, {0, gs::rgb4(7, 6, 6), gs::rgb4(10, 9, 8), gs::rgb4(4, 4, 5), gs::rgb4(3, 3, 3),
                            gs::rgb4(12, 11, 9), gs::rgb4(2, 2, 2), ink});
    setPal(vdp, PAL_PINE, {0, gs::rgb4(2, 7, 3), gs::rgb4(4, 10, 4), gs::rgb4(5, 4, 2), ink});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(14, 15, 15), gs::rgb4(8, 12, 14), ink});
    setPal(vdp, PAL_CLOUD, {0, gs::rgb4(8, 8, 10), gs::rgb4(12, 12, 13), ink});
    setPal(vdp, PAL_CREW, {0, gs::rgb4(15, 9, 3), gs::rgb4(8, 4, 2), gs::rgb4(15, 14, 8), ink});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 6), gs::rgb4(15, 15, 12), gs::rgb4(1, 5, 2), ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(15, 12, 5), gs::rgb4(6, 1, 1), ink});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 4), gs::rgb4(9, 6, 1), gs::rgb4(15, 15, 12), ink});

    vdp.setFogColor(gs::rgb4(4, 5, 7));
    loadFont(vdp, art);
    art.hull = gs::uploadMipped(vdp, hullArt());
    art.sail = gs::uploadMipped(vdp, sailArt());
    art.cliff = gs::uploadMipped(vdp, cliffArt());
    art.pine = gs::uploadMipped(vdp, pineArt());
    art.foam = gs::uploadMipped(vdp, foamArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.title = words(vdp, "KEEL PASS", 3, 1, 2);
    art.clear = words(vdp, "CLEAR", 3, 1, 3);
    art.rock = words(vdp, "ROCK", 3, 1, 2);
}

}  // namespace keelpass

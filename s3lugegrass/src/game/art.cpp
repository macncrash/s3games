#include "art.h"

#include <cmath>
#include <initializer_list>

namespace lugegrass {
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

gs::Bitmap lugeArt() {
    gs::Bitmap b(92, 32);
    b.poly({{4.f, 20.f}, {80.f, 14.f}, {88.f, 18.f}, {84.f, 24.f}, {6.f, 26.f}}, 2);
    b.rect(8, 22, 70, 3, 3);
    b.line(6, 25, 86, 20, 4, 1.5f);
    b.ellipse(16, 24, 3.f, 3.f, 5);
    b.ellipse(74, 21, 3.f, 3.f, 5);
    b.ellipse(46, 12, 9, 6, 6);
    b.ellipse(56, 10, 6, 5, 7);
    b.rect(38, 14, 14, 3, 8);
    b.rect(60, 8, 9, 3, 1);
    b.ellipse(67, 7, 4, 3, 9);
    b.set(69, 6, 10);
    b.line(34, 16, 26, 12, 8, 1.4f);
    b.outline(11, false);
    return b;
}

gs::Bitmap sprayArt() {
    gs::Bitmap b(16, 8);
    b.ellipse(7, 5, 6, 2.4f, 1);
    b.ellipse(11, 3, 3, 1.6f, 2);
    return b;
}

gs::Bitmap iceArt() {
    gs::Bitmap b(48, 22);
    b.rect(0, 4, 48, 18, 1);
    b.poly({{0.f, 8.f}, {12.f, 2.f}, {24.f, 7.f}, {38.f, 1.f}, {48.f, 8.f}, {48.f, 22.f}, {0.f, 22.f}}, 2);
    for (int x = 2; x < 46; x += 8) b.rect(x, 12, 4, 2, 3);
    b.rect(0, 16, 48, 6, 4);
    return b;
}

gs::Bitmap grassArt() {
    gs::Bitmap b(40, 24);
    b.rect(0, 6, 40, 18, 1);
    for (int x = 1; x < 40; x += 4) {
        b.line(float(x), 16.f, float(x + ((x / 4) & 1 ? 1 : -1)), 4.f, (x / 4) & 1 ? 2 : 3, 1.2f);
    }
    b.rect(0, 18, 40, 6, 4);
    return b;
}

gs::Bitmap dirtArt() {
    gs::Bitmap b(40, 20);
    b.rect(0, 4, 40, 16, 1);
    for (int i = 0; i < 8; i++) b.ellipse(4.f + i * 5.f, 12.f, 1.4f, 1.f, 2);
    b.rect(0, 14, 40, 6, 3);
    return b;
}

gs::Bitmap pineArt() {
    gs::Bitmap b(26, 46);
    b.poly({{13.f, 2.f}, {23.f, 18.f}, {3.f, 18.f}}, 1);
    b.poly({{13.f, 12.f}, {24.f, 30.f}, {2.f, 30.f}}, 2);
    b.poly({{13.f, 22.f}, {25.f, 40.f}, {1.f, 40.f}}, 1);
    b.rect(11, 38, 4, 8, 3);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(10, 48);
    b.rect(4, 6, 3, 42, 1);
    b.rect(1, 2, 8, 6, 2);
    b.rect(3, 20, 4, 3, 3);
    return b;
}

gs::Bitmap tuftArt() {
    gs::Bitmap b(14, 12);
    b.line(2, 11, 4, 2, 1, 1.3f);
    b.line(6, 11, 7, 1, 2, 1.3f);
    b.line(10, 11, 9, 3, 1, 1.3f);
    b.rect(1, 10, 12, 2, 3);
    return b;
}

gs::Bitmap flagArt() {
    gs::Bitmap b(28, 36);
    b.rect(4, 4, 2, 32, 1);
    b.poly({{6.f, 4.f}, {24.f, 8.f}, {22.f, 14.f}, {6.f, 16.f}}, 2);
    b.line(8, 8, 20, 10, 3, 1.f);
    return b;
}

gs::Bitmap hutArt() {
    gs::Bitmap b(48, 36);
    b.rect(8, 14, 32, 20, 1);
    b.poly({{4.f, 14.f}, {24.f, 3.f}, {44.f, 14.f}}, 2);
    b.rect(20, 22, 8, 12, 3);
    b.rect(12, 18, 6, 5, 4);
    b.rect(30, 18, 6, 5, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap birdArt(bool up) {
    gs::Bitmap b(14, 8);
    if (up) {
        b.line(1, 6, 7, 2, 1, 1.2f);
        b.line(7, 2, 13, 6, 1, 1.2f);
    } else {
        b.line(1, 2, 7, 5, 1, 1.2f);
        b.line(7, 5, 13, 2, 1, 1.2f);
    }
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 14, 12), gs::rgb4(8, 9, 10), ink});
    setPal(vdp, PAL_LUGE,
           {0, gs::rgb4(13, 3, 2), gs::rgb4(13, 13, 14), gs::rgb4(5, 6, 7), gs::rgb4(3, 3, 4), gs::rgb4(2, 2, 3),
            gs::rgb4(9, 6, 4), gs::rgb4(12, 9, 7), gs::rgb4(4, 5, 8), gs::rgb4(15, 12, 3), gs::rgb4(2, 2, 2), ink});
    setPal(vdp, PAL_ICE, {0, gs::rgb4(10, 12, 14), gs::rgb4(14, 15, 15), gs::rgb4(7, 9, 12), gs::rgb4(12, 13, 14)});
    setPal(vdp, PAL_GRASS, {0, gs::rgb4(3, 9, 3), gs::rgb4(6, 13, 4), gs::rgb4(2, 7, 2), gs::rgb4(4, 6, 3)});
    setPal(vdp, PAL_DIRT, {0, gs::rgb4(7, 5, 3), gs::rgb4(5, 4, 2), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_PINE, {0, gs::rgb4(2, 7, 3), gs::rgb4(1, 5, 2), gs::rgb4(6, 4, 2)});
    setPal(vdp, PAL_POST, {0, gs::rgb4(8, 8, 9), gs::rgb4(14, 12, 3), gs::rgb4(12, 4, 2)});
    setPal(vdp, PAL_TUFT, {0, gs::rgb4(4, 12, 3), gs::rgb4(8, 14, 5), gs::rgb4(3, 6, 2)});
    setPal(vdp, PAL_FLAG, {0, gs::rgb4(9, 9, 8), gs::rgb4(12, 3, 2), gs::rgb4(15, 14, 8)});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(5, 14, 6), gs::rgb4(12, 15, 10), ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 5, 3), gs::rgb4(15, 12, 6), ink});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(14, 13, 10), gs::rgb4(7, 10, 8), ink});
    setPal(vdp, PAL_HUT, {0, gs::rgb4(8, 5, 3), gs::rgb4(10, 3, 2), gs::rgb4(3, 3, 4), gs::rgb4(12, 13, 14), ink});
    setPal(vdp, PAL_BIRD, {0, gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_SPRAY, {0, gs::rgb4(12, 14, 12), gs::rgb4(8, 11, 6)});

    art.luge = gs::uploadMipped(vdp, lugeArt());
    art.spray = gs::uploadMipped(vdp, sprayArt());
    art.ice = gs::uploadMipped(vdp, iceArt());
    art.grass = gs::uploadMipped(vdp, grassArt());
    art.dirt = gs::uploadMipped(vdp, dirtArt());
    art.pine = gs::uploadMipped(vdp, pineArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.tuft = gs::uploadMipped(vdp, tuftArt());
    art.flag = gs::uploadMipped(vdp, flagArt());
    art.hut = gs::uploadMipped(vdp, hutArt());
    art.bird[0] = gs::uploadMipped(vdp, birdArt(true));
    art.bird[1] = gs::uploadMipped(vdp, birdArt(false));
    art.title = words(vdp, "LUGE", 3, 1, 2);
    art.grassWord = words(vdp, "GRASS", 3, 1, 2);
    art.stopped = words(vdp, "FULL STOP", 2, 1, 2);
    art.missed = words(vdp, "MISSED THE END", 2, 1, 2);
    art.shortStop = words(vdp, "SHORT OF THE GRASS", 2, 1, 2);
    art.paused = words(vdp, "HELD", 2, 1, 2);
    loadFont(vdp, art);
}

}  // namespace lugegrass

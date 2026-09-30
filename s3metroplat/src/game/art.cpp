#include "game/art.h"

#include <cmath>
#include <initializer_list>

namespace metroplat {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, ink);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

void wheels(gs::Bitmap& b, float cx, float phase) {
    b.rect(int(cx) - 16, 46, 32, 3, 2);
    for (int k = 0; k < 2; k++) {
        float wx = cx - 8.f + k * 16.f;
        b.ellipse(wx, 52, 5, 5, 8);
        b.ellipse(wx, 52, 2, 2, 9);
        for (int s = 0; s < 4; s++) {
            float a = phase + s * 1.5708f;
            b.line(wx, 52, wx + std::cos(a) * 4.f, 52 + std::sin(a) * 4.f, 1, 1.f);
        }
    }
}

gs::Bitmap carArt(int phase) {
    gs::Bitmap b(176, 58);
    b.rect(4, 10, 168, 36, 3);
    b.rect(4, 10, 168, 5, 4);
    b.rect(4, 40, 168, 5, 5);
    b.rect(8, 14, 20, 8, 6);
    // double doors the driver puts on the gap line
    b.rect(72, 16, 28, 28, 7);
    b.rect(74, 18, 11, 22, 10);
    b.rect(87, 18, 11, 22, 11);
    b.rect(85, 28, 2, 4, 12);
    b.rect(34, 16, 22, 14, 10);
    b.rect(118, 16, 22, 14, 10);
    b.rect(36, 18, 8, 8, 13);
    b.rect(120, 18, 8, 8, 13);
    b.rect(156, 16, 12, 14, 10);
    b.rect(2, 22, 4, 14, 2);
    b.rect(170, 22, 4, 14, 2);
    b.rect(60, 6, 18, 5, 4);
    wheels(b, 40, phase * 1.2f);
    wheels(b, 136, phase * 1.2f + 0.6f);
    b.outline(1, false);
    return b;
}

gs::Bitmap platArt() {
    gs::Bitmap b(192, 78);
    b.rect(0, 44, 192, 30, 3);
    b.rect(0, 44, 192, 4, 4);
    b.rect(0, 72, 192, 4, 2);
    for (int x = 4; x < 188; x += 14) b.rect(x, 54, 8, 10, 5);
    b.rect(10, 16, 172, 28, 6);
    b.rect(10, 16, 172, 4, 7);
    for (int x = 18; x < 170; x += 22) b.rect(x, 22, 10, 14, 8);
    // yellow gap line under the platform doors
    b.rect(78, 46, 32, 5, 13);
    b.rect(78, 44, 3, 12, 13);
    b.rect(107, 44, 3, 12, 13);
    b.rect(86, 20, 16, 20, 9);
    return b;
}

gs::Bitmap sleeperArt() {
    gs::Bitmap b(28, 16);
    b.rect(0, 2, 28, 12, 3);
    b.rect(0, 3, 28, 2, 4);
    b.rect(0, 11, 28, 2, 4);
    for (int x = 2; x < 26; x += 8) b.rect(x, 5, 4, 6, 2);
    return b;
}

gs::Bitmap signalArt() {
    gs::Bitmap b(16, 40);
    b.rect(6, 14, 4, 24, 2);
    b.rect(2, 2, 12, 16, 3);
    b.ellipse(8, 7, 3, 3, 4);
    b.ellipse(8, 13, 3, 3, 5);
    return b;
}

gs::Bitmap benchArt() {
    gs::Bitmap b(36, 22);
    b.rect(2, 8, 32, 4, 3);
    b.rect(4, 12, 3, 8, 2);
    b.rect(29, 12, 3, 8, 2);
    b.rect(2, 4, 32, 4, 4);
    return b;
}

gs::Bitmap sparkArt() {
    gs::Bitmap b(12, 12);
    b.ellipse(6, 6, 5, 4, 1);
    b.ellipse(6, 6, 2, 2, 2);
    return b;
}

gs::Bitmap archArt() {
    gs::Bitmap b(80, 40);
    b.poly({{0, 39}, {8, 10}, {40, 2}, {72, 10}, {79, 39}}, 2);
    b.poly({{10, 39}, {22, 16}, {40, 8}, {58, 16}, {70, 39}}, 3);
    b.rect(18, 28, 8, 10, 4);
    return b;
}

gs::Bitmap clockArt() {
    gs::Bitmap b(22, 22);
    b.ellipse(11, 11, 10, 10, 3);
    b.ellipse(11, 11, 8, 8, 4);
    b.line(11, 11, 11, 5, 2, 1.f);
    b.line(11, 11, 16, 12, 5, 1.f);
    return b;
}

gs::Image words(gs::VDP& vdp, const char* s, int scale, int color) {
    return gs::uploadImage(vdp, gs::textBitmap(s, {scale, color, 0, 15, 1}));
}

void loadFont(gs::VDP& vdp, Art& art) {
    gs::TileAlloc tiles(vdp);
    for (int c = 32; c < 96; c++) {
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

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_TEXT, gs::rgb4(15, 15, 14));
    textPal(vdp, PAL_DIM, gs::rgb4(8, 10, 12));
    textPal(vdp, PAL_RED, gs::rgb4(15, 4, 3));
    textPal(vdp, PAL_GREEN, gs::rgb4(4, 15, 8));
    textPal(vdp, PAL_AMBER, gs::rgb4(15, 12, 3));

    setPal(vdp, PAL_CAR,
           {0, gs::rgb4(1, 1, 2), gs::rgb4(3, 4, 5), gs::rgb4(11, 12, 13), gs::rgb4(14, 15, 15), gs::rgb4(7, 8, 9),
            gs::rgb4(12, 3, 3), gs::rgb4(2, 3, 5), gs::rgb4(4, 4, 5), gs::rgb4(9, 9, 10), gs::rgb4(8, 13, 15),
            gs::rgb4(4, 8, 11), gs::rgb4(15, 13, 3), gs::rgb4(13, 15, 15), 0, 0});
    setPal(vdp, PAL_PLAT,
           {0, gs::rgb4(1, 1, 2), gs::rgb4(4, 4, 5), gs::rgb4(8, 8, 9), gs::rgb4(12, 12, 11), gs::rgb4(6, 6, 7),
            gs::rgb4(9, 8, 7), gs::rgb4(11, 10, 8), gs::rgb4(5, 6, 8), gs::rgb4(3, 4, 6), gs::rgb4(10, 12, 14),
            gs::rgb4(6, 8, 10), gs::rgb4(14, 14, 12), gs::rgb4(15, 12, 2), 0, 0});
    setPal(vdp, PAL_TUNNEL,
           {0, gs::rgb4(2, 2, 3), gs::rgb4(4, 4, 6), gs::rgb4(3, 3, 5), gs::rgb4(6, 6, 8), gs::rgb4(8, 7, 4), 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_LAMP,
           {0, gs::rgb4(2, 2, 3), gs::rgb4(5, 5, 6), gs::rgb4(6, 6, 5), gs::rgb4(4, 14, 5), gs::rgb4(14, 4, 3),
            gs::rgb4(15, 14, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 14, 8), gs::rgb4(15, 10, 3), gs::rgb4(12, 12, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         0, 0, 0});

    vdp.setFogColor(gs::rgb4(2, 2, 4));
    art.car[0] = gs::uploadImage(vdp, carArt(0));
    art.car[1] = gs::uploadImage(vdp, carArt(1));
    art.plat = gs::uploadImage(vdp, platArt());
    art.sleeper = gs::uploadImage(vdp, sleeperArt());
    art.signal = gs::uploadImage(vdp, signalArt());
    art.bench = gs::uploadImage(vdp, benchArt());
    art.spark = gs::uploadImage(vdp, sparkArt());
    art.arch = gs::uploadImage(vdp, archArt());
    art.clock = gs::uploadImage(vdp, clockArt());
    art.logo = words(vdp, "METRO PLAT", 3, 1);
    art.tag = words(vdp, "STOP LEVEL WITH THE PLATFORM", 1, 1);
    art.banLevel = words(vdp, "LEVEL", 3, 1);
    art.banShort = words(vdp, "SHORT", 3, 1);
    art.banPast = words(vdp, "WALL", 3, 1);
    art.banCrew = words(vdp, "SERVICE OVER", 2, 1);
    loadFont(vdp, art);
}

}  // namespace metroplat

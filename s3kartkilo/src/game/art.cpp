#include "game/art.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>

namespace kartkilo {
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

gs::Bitmap paintKart(bool rival) {
    gs::Bitmap b(48, 72);
    b.rect(10, 8, 28, 56, 1);
    b.rect(14, 12, 20, 22, 2);
    b.rect(16, 36, 16, 18, 3);
    b.ellipse(24, 18, 6, 5, 4);
    b.ellipse(21, 17, 1.2f, 1.2f, 5);
    b.ellipse(27, 17, 1.2f, 1.2f, 5);
    b.rect(8, 14, 6, 10, 6);
    b.rect(34, 14, 6, 10, 6);
    b.rect(6, 48, 8, 12, 7);
    b.rect(34, 48, 8, 12, 7);
    b.rect(18, 6, 12, 6, 8);
    b.rect(20, 58, 8, 8, 8);
    b.ellipse(14, 22, 3.2f, 3.2f, 9);
    b.ellipse(34, 22, 3.2f, 3.2f, 9);
    b.ellipse(14, 54, 3.2f, 3.2f, 9);
    b.ellipse(34, 54, 3.2f, 3.2f, 9);
    if (!rival) {
        b.rect(20, 40, 8, 6, 10);
    } else {
        b.rect(18, 40, 12, 4, 10);
        b.rect(22, 44, 4, 6, 11);
    }
    b.outline(15, false);
    return b;
}

gs::Bitmap paintWheel(int frame) {
    gs::Bitmap b(28, 28);
    const float cx = 13.5f, cy = 13.5f;
    const float ang = frame * 0.785398f;
    b.ellipse(cx, cy, 12.f, 12.f, 1);
    b.ellipse(cx, cy, 9.2f, 9.2f, 2);
    b.ellipse(cx, cy, 7.4f, 7.4f, 0);
    for (int i = 0; i < 4; i++) {
        float a = ang + i * 1.570796f;
        b.line(cx, cy, cx + std::cos(a) * 6.2f, cy + std::sin(a) * 6.2f, 3, 1.8f);
    }
    b.ellipse(cx, cy, 2.4f, 2.4f, 4);
    b.ellipse(cx, cy, 1.1f, 1.1f, 1);
    return b;
}

gs::Bitmap paintRoad() {
    gs::Bitmap b(96, 32);
    b.rect(0, 0, 96, 32, 1);
    b.rect(0, 0, 4, 32, 2);
    b.rect(92, 0, 4, 32, 2);
    b.rect(46, 4, 4, 10, 3);
    b.rect(46, 20, 4, 8, 3);
    for (int i = 0; i < 6; i++) b.rect(8.f + i * 14.f, 14.f, 6.f, 2.f, 4);
    return b;
}

gs::Bitmap paintRibbon() {
    gs::Bitmap b(80, 14);
    for (int i = 0; i < 8; i++) b.rect(float(i * 10), 1, 10, 12, (i & 1) ? 1 : 2);
    b.rect(0, 0, 80, 2, 3);
    b.rect(0, 12, 80, 2, 3);
    return b;
}

gs::Bitmap paintPuff() {
    gs::Bitmap b(16, 12);
    b.ellipse(8, 6, 7, 4, 1);
    b.ellipse(5, 6, 2.2f, 1.6f, 2);
    b.ellipse(11, 5, 2.f, 1.5f, 3);
    return b;
}

void loadFont(gs::VDP& vdp, Art& art) {
    gs::TileAlloc tiles(vdp);
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++) {
                if (!g[y * 5 + x]) continue;
                px[y * 8 + x + 1] = 1;
                if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
            }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        art.font[c - 32] = t;
    }
}

gs::Mipped words(gs::VDP& vdp, const char* text, int scale, int ink) {
    gs::TextStyle st{scale, ink, 2, 15, 1};
    return gs::uploadMipped(vdp, gs::textBitmap(text, st));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 15), gs::rgb4(8, 8, 10), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_KART,
           {0, gs::rgb4(13, 2, 2), gs::rgb4(15, 6, 3), gs::rgb4(8, 1, 1), gs::rgb4(15, 12, 8), gs::rgb4(2, 2, 3),
            gs::rgb4(15, 14, 4), gs::rgb4(4, 4, 5), gs::rgb4(15, 8, 2), gs::rgb4(1, 1, 1), gs::rgb4(15, 15, 12),
            gs::rgb4(6, 6, 8)});
    setPal(vdp, PAL_RIVAL,
           {0, gs::rgb4(2, 4, 12), gs::rgb4(4, 8, 15), gs::rgb4(1, 2, 8), gs::rgb4(12, 14, 15), gs::rgb4(2, 2, 3),
            gs::rgb4(8, 12, 15), gs::rgb4(3, 3, 5), gs::rgb4(15, 12, 3), gs::rgb4(1, 1, 1), gs::rgb4(10, 14, 15),
            gs::rgb4(15, 15, 8)});
    setPal(vdp, PAL_WHEEL,
           {0, gs::rgb4(2, 2, 2), gs::rgb4(6, 6, 7), gs::rgb4(12, 12, 13), gs::rgb4(14, 10, 3)});
    setPal(vdp, PAL_ROAD,
           {0, gs::rgb4(4, 4, 5), gs::rgb4(15, 15, 15), gs::rgb4(14, 14, 12), gs::rgb4(7, 7, 8)});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 12, 2), gs::rgb4(12, 2, 3), gs::rgb4(15, 15, 15)});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 8), gs::rgb4(1, 4, 2), gs::rgb4(15, 15, 15)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(6, 1, 1), gs::rgb4(15, 15, 15)});
    setPal(vdp, PAL_PUFF, {0, gs::rgb4(12, 12, 13), gs::rgb4(8, 8, 9), gs::rgb4(15, 15, 15)});
    setPal(vdp, PAL_POST, {0, gs::rgb4(15, 10, 2), gs::rgb4(8, 5, 1), gs::rgb4(15, 15, 12)});
    for (int p = 0; p < 16; p++) vdp.setColor(p * 16 + 15, gs::rgb4(1, 1, 2));

    art.kart = gs::uploadMipped(vdp, paintKart(false));
    art.rival = gs::uploadMipped(vdp, paintKart(true));
    for (int i = 0; i < 4; i++) art.wheel[i] = gs::uploadMipped(vdp, paintWheel(i));
    art.road = gs::uploadMipped(vdp, paintRoad());
    art.ribbon = gs::uploadMipped(vdp, paintRibbon());
    art.puff = gs::uploadMipped(vdp, paintPuff());
    art.title = words(vdp, "S3 KART KILO", 2, 1);
    art.clean = words(vdp, "WHEELS UNTOUCHED", 2, 1);
    loadFont(vdp, art);
    vdp.setFogColor(gs::rgb4(6, 8, 6));
}

}  // namespace kartkilo

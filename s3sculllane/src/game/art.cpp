#include "art.h"

#include <initializer_list>

namespace sculllane {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap paintShell() {
    Bitmap b(36, 78);
    b.poly({{18, 2}, {28, 22}, {30, 70}, {6, 70}, {8, 22}}, 2);
    b.poly({{18, 8}, {24, 24}, {25, 64}, {11, 64}, {12, 24}}, 3);
    b.rect(15, 48, 6, 14, 4);
    b.ellipse(18, 54, 3.2f, 4.f, 5);
    b.line(18, 10, 18, 46, 6, 1.2f);
    b.outline(1, false);
    return b.cropToContent(1);
}

Bitmap paintOar() {
    Bitmap b(54, 14);
    b.line(4, 7, 40, 7, 2, 2.f);
    b.ellipse(46, 7, 7.f, 4.5f, 3);
    b.ellipse(46, 7, 4.f, 2.2f, 4);
    return b;
}

Bitmap paintBuoy() {
    Bitmap b(16, 28);
    b.rect(7, 14, 2, 12, 4);
    b.ellipse(8, 10, 6.f, 6.5f, 1);
    b.ellipse(8, 8, 3.2f, 2.4f, 2);
    b.rect(6, 4, 4, 3, 3);
    return b;
}

Bitmap paintFlag() {
    Bitmap b(28, 40);
    b.rect(4, 8, 3, 30, 2);
    b.poly({{7, 8}, {24, 14}, {7, 20}}, 1);
    b.poly({{7, 18}, {22, 24}, {7, 30}}, 3);
    return b;
}

Bitmap paintSplash() {
    Bitmap b(36, 14);
    b.ellipse(18, 8, 14.f, 4.f, 1);
    b.ellipse(18, 8, 7.f, 2.f, 2);
    return b;
}

Bitmap paintCox() {
    Bitmap b(18, 22);
    b.ellipse(9, 7, 5.f, 5.f, 2);
    b.rect(5, 12, 8, 8, 3);
    b.rect(3, 13, 3, 6, 4);
    b.rect(12, 13, 3, 6, 4);
    return b;
}

gs::Mipped word(gs::VDP& vdp, const char* s, int scale, int color) {
    gs::TextStyle st{scale, color, 0, 1, 1};
    return gs::uploadMipped(vdp, gs::textBitmap(s, st));
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) px[y * 8 + x + 1] = 1;
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(15, 15, 14);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(7, 9, 11), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_SHELL,
           {0, gs::rgb4(1, 2, 3), gs::rgb4(12, 3, 3), gs::rgb4(14, 6, 5), gs::rgb4(8, 2, 2), gs::rgb4(15, 13, 8),
            gs::rgb4(4, 4, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_OAR,
           {0, gs::rgb4(3, 2, 1), gs::rgb4(10, 7, 3), gs::rgb4(14, 14, 12), gs::rgb4(6, 10, 12), 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0});
    setPal(vdp, PAL_BUOY,
           {0, gs::rgb4(15, 12, 2), gs::rgb4(15, 15, 12), gs::rgb4(14, 3, 2), gs::rgb4(3, 3, 3), 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 6, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_SIGN,
           {0, gs::rgb4(15, 14, 8), gs::rgb4(5, 6, 8), gs::rgb4(12, 4, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_CREW,
           {0, gs::rgb4(2, 3, 5), gs::rgb4(14, 12, 9), gs::rgb4(4, 6, 12), gs::rgb4(8, 9, 11), 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0});
    setPal(vdp, PAL_LANE,
           {0, gs::rgb4(2, 6, 8), gs::rgb4(1, 4, 7), gs::rgb4(3, 7, 6), gs::rgb4(14, 13, 6), gs::rgb4(12, 10, 3),
            gs::rgb4(4, 8, 10), gs::rgb4(3, 6, 9), gs::rgb4(6, 8, 7), gs::rgb4(3, 4, 3), gs::rgb4(8, 9, 6),
            gs::rgb4(2, 7, 11), gs::rgb4(1, 5, 9), gs::rgb4(6, 12, 14), gs::rgb4(15, 14, 8), gs::rgb4(3, 8, 10)});
    vdp.setFogColor(gs::rgb4(7, 10, 12));

    art.shell = gs::uploadMipped(vdp, paintShell());
    art.oar = gs::uploadMipped(vdp, paintOar());
    art.buoy = gs::uploadMipped(vdp, paintBuoy());
    art.flag = gs::uploadMipped(vdp, paintFlag());
    art.splash = gs::uploadMipped(vdp, paintSplash());
    art.cox = gs::uploadMipped(vdp, paintCox());
    art.title = word(vdp, "SCULL LANE", 3, 1);
    art.held = word(vdp, "LANE HELD", 3, 1);
    art.left = word(vdp, "LEFT THE LANE", 2, 1);
    art.crew = word(vdp, "OTHER CREW", 2, 1);
    art.stay = word(vdp, "STAY IN THE LANE", 1, 1);
    art.start = word(vdp, "START", 2, 1);
    loadFont(vdp, art);
}

}  // namespace sculllane

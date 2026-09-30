#include "art.h"

#include <initializer_list>
#include <string>

namespace helilane {
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

gs::Mipped word(gs::VDP& vdp, const char* s, int scale, int color) {
    gs::TextStyle st;
    st.scale = scale;
    st.color = color;
    st.outline = 0;
    st.spacing = 1;
    return gs::uploadMipped(vdp, gs::textBitmap(s, st));
}

Bitmap paintBody() {
    Bitmap b(88, 56);
    b.ellipse(44, 28, 22, 16, 3);
    b.ellipse(44, 26, 18, 12, 2);
    b.ellipse(52, 22, 10, 7, 4);
    b.ellipse(54, 21, 5, 3, 8);
    b.rect(16, 24, 16, 6, 3);
    b.rect(8, 14, 10, 18, 5);
    b.rect(10, 16, 4, 10, 6);
    b.rect(40, 8, 6, 12, 5);
    b.rect(22, 36, 4, 12, 7);
    b.rect(58, 36, 4, 12, 7);
    b.rect(16, 46, 22, 3, 7);
    b.rect(50, 46, 22, 3, 7);
    b.rect(30, 30, 18, 3, 9);
    b.outline(1, false);
    return b.cropToContent(1);
}

Bitmap paintDisc() {
    Bitmap b(96, 18);
    b.ellipse(48, 9, 44, 6, 2);
    b.ellipse(48, 9, 28, 3, 3);
    b.rect(44, 4, 8, 10, 1);
    return b.cropToContent(0);
}

Bitmap paintFin() {
    Bitmap b(28, 36);
    b.poly({{4, 30}, {8, 6}, {18, 8}, {14, 32}}, 2);
    b.poly({{8, 12}, {14, 13}, {12, 26}, {7, 24}}, 3);
    b.rect(6, 28, 10, 4, 1);
    return b.cropToContent(1);
}

Bitmap paintShadow() {
    Bitmap b(70, 16);
    b.ellipse(35, 8, 30, 5, 1);
    return b;
}

Bitmap paintPylon() {
    Bitmap b(18, 48);
    b.poly({{9, 2}, {16, 46}, {2, 46}}, 2);
    b.rect(7, 8, 4, 34, 3);
    b.rect(4, 6, 10, 4, 1);
    b.rect(6, 20, 6, 3, 4);
    return b.cropToContent(1);
}

Bitmap paintGate() {
    Bitmap b(64, 40);
    b.rect(4, 8, 6, 30, 2);
    b.rect(54, 8, 6, 30, 2);
    b.rect(4, 4, 56, 6, 1);
    b.rect(22, 12, 20, 8, 3);
    return b.cropToContent(1);
}

Bitmap paintCloud() {
    Bitmap b(64, 24);
    b.ellipse(20, 14, 14, 7, 1);
    b.ellipse(38, 12, 16, 8, 1);
    b.ellipse(30, 10, 10, 5, 2);
    return b.cropToContent(1);
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 13), gs::rgb4(6, 8, 9)});
    setPal(vdp, PAL_SHIP,
           {0, gs::rgb4(2, 2, 3), gs::rgb4(12, 13, 14), gs::rgb4(7, 8, 9), gs::rgb4(5, 12, 15), gs::rgb4(3, 3, 4),
            gs::rgb4(10, 3, 2), gs::rgb4(4, 4, 5), gs::rgb4(14, 15, 15), gs::rgb4(13, 10, 2)});
    setPal(vdp, PAL_RIVAL, {0, gs::rgb4(8, 2, 2), gs::rgb4(12, 5, 3), gs::rgb4(4, 2, 2), gs::rgb4(14, 12, 8)});
    setPal(vdp, PAL_PYLON, {0, gs::rgb4(15, 14, 6), gs::rgb4(10, 10, 11), gs::rgb4(6, 6, 7), gs::rgb4(14, 4, 2)});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 9)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 6, 3)});
    setPal(vdp, PAL_SIGN, {0, gs::rgb4(15, 13, 6), gs::rgb4(6, 5, 3), gs::rgb4(12, 4, 3)});
    setPal(vdp, PAL_ROTOR, {0, gs::rgb4(2, 2, 2), gs::rgb4(8, 9, 10), gs::rgb4(13, 14, 15)});
    setPal(vdp, PAL_LANE,
           {0, gs::rgb4(4, 8, 3), gs::rgb4(3, 6, 2), gs::rgb4(6, 9, 4), gs::rgb4(8, 8, 6), gs::rgb4(6, 6, 5),
            gs::rgb4(5, 5, 6), gs::rgb4(4, 4, 5), gs::rgb4(7, 7, 6), gs::rgb4(3, 3, 3), gs::rgb4(9, 9, 8),
            gs::rgb4(2, 5, 8), gs::rgb4(3, 6, 9), gs::rgb4(8, 11, 13), gs::rgb4(14, 12, 3), gs::rgb4(8, 8, 7)});
    vdp.setFogColor(gs::rgb4(8, 10, 13));

    art.body = gs::uploadMipped(vdp, paintBody());
    art.disc = gs::uploadMipped(vdp, paintDisc());
    art.fin = gs::uploadMipped(vdp, paintFin());
    art.shadow = gs::uploadMipped(vdp, paintShadow());
    art.pylon = gs::uploadMipped(vdp, paintPylon());
    art.gate = gs::uploadMipped(vdp, paintGate());
    art.cloud = gs::uploadMipped(vdp, paintCloud());
    art.title = word(vdp, "HELI LANE", 3, 1);
    art.held = word(vdp, "LANE HELD", 3, 1);
    art.left = word(vdp, "LEFT THE LANE", 2, 1);
    art.missed = word(vdp, "OTHER CREW", 2, 1);
    art.stay = word(vdp, "STAY IN THE LANE", 1, 1);
    art.start = word(vdp, "START", 2, 1);
    loadFont(vdp, art);
}

}  // namespace helilane

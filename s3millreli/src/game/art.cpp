#include "art.h"

#include <cstdint>
#include <string>

namespace mill {
namespace {

void setPal(gs::VDP& vdp, int pal, const uint16_t* cs, int n) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, i < n ? cs[i] : 0);
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& art) {
    for (int i = 0; i < 96; i++) {
        gs::TextStyle st;
        st.scale = 1;
        st.color = 1;
        gs::Bitmap g = gs::textBitmap(std::string(1, char(32 + i)), st);
        gs::Bitmap tile(8, 8);
        for (int y = 0; y < g.h && y < 8; y++)
            for (int x = 0; x < g.w && x < 8; x++)
                if (g.get(x, y)) tile.set(x, y, 1);
        art.font[i] = tiles.shared(tile.px.data());
        art.glyph[i] = gs::uploadMipped(vdp, g.w > 0 ? g : tile);
    }
}

gs::Bitmap millArt() {
    gs::Bitmap b(96, 120);
    b.rect(18, 36, 60, 78, 3);
    b.rect(22, 40, 52, 70, 4);
    for (int y = 44; y < 104; y += 8) b.rect(22, float(y), 52, 1, 2);
    b.rect(40, 78, 16, 32, 6);
    b.rect(44, 86, 8, 10, 8);
    b.rect(30, 50, 10, 12, 7);
    b.rect(56, 50, 10, 12, 7);
    b.poly({{18, 36}, {48, 8}, {78, 36}}, 5);
    b.rect(44, 4, 8, 16, 9);
    b.ellipse(48, 96, 10, 4, 1);
    b.outline(10, false);
    return b;
}

gs::Bitmap capArt() {
    gs::Bitmap b(28, 16);
    b.ellipse(14, 10, 12, 6, 1);
    b.rect(12, 2, 4, 8, 2);
    return b;
}

gs::Bitmap sailPlus() {
    gs::Bitmap b(72, 72);
    b.rect(33, 4, 6, 64, 1);
    b.rect(4, 33, 64, 6, 1);
    b.rect(34, 8, 4, 18, 3);
    b.rect(34, 46, 4, 18, 3);
    b.rect(8, 34, 18, 4, 3);
    b.rect(46, 34, 18, 4, 3);
    b.ellipse(36, 36, 5, 5, 4);
    return b;
}

gs::Bitmap sailCross() {
    gs::Bitmap b(72, 72);
    b.line(10, 10, 62, 62, 1, 5);
    b.line(62, 10, 10, 62, 1, 5);
    b.line(16, 14, 40, 38, 3, 3);
    b.line(56, 16, 38, 34, 3, 3);
    b.ellipse(36, 36, 5, 5, 4);
    return b;
}

gs::Bitmap millerArt() {
    gs::Bitmap b(24, 36);
    b.ellipse(12, 6, 5, 5, 3);
    b.rect(8, 11, 8, 3, 4);
    b.rect(7, 14, 10, 12, 1);
    b.rect(5, 16, 4, 9, 2);
    b.rect(15, 16, 4, 9, 5);
    b.rect(8, 26, 4, 8, 6);
    b.rect(13, 26, 4, 8, 6);
    b.rect(6, 33, 6, 2, 7);
    b.rect(13, 33, 6, 2, 7);
    b.outline(8, false);
    return b;
}

gs::Bitmap sackArt() {
    gs::Bitmap b(16, 18);
    b.ellipse(8, 10, 6, 7, 1);
    b.rect(5, 2, 6, 4, 2);
    b.line(5, 8, 11, 8, 3, 1);
    return b;
}

gs::Bitmap body(int step, int kind) {
    gs::Bitmap b(26, 36);
    int skin = 3, coat = kind == 2 ? 2 : 1;
    b.ellipse(13, 6, 5, 5, skin);
    b.rect(9, 11, 8, 3, 4);
    b.rect(7, 14, 12, 11, coat);
    if (kind == 2) b.rect(8, 16, 10, 6, 6);
    b.rect(float(step ? 4 : 8), 15, 4, 9, 5);
    b.rect(float(step ? 16 : 14), 15, 4, 9, 5);
    b.rect(float(step ? 8 : 10), 25, 4, 8, 7);
    b.rect(float(step ? 14 : 12), 25, 4, 8, 7);
    b.rect(6, 32, 5, 3, 8);
    b.rect(15, 32, 5, 3, 8);
    if (kind == 0) b.line(18, 8, 24, 18, 9, 2);
    if (kind == 1) b.rect(18, 16, 7, 2, 9);
    b.outline(10, false);
    return b;
}

gs::Bitmap wainBody(int step) {
    gs::Bitmap b(40, 28);
    b.rect(4, 6, 28, 12, 1);
    b.rect(6, 8, 24, 6, 2);
    b.ellipse(10, 20, 5, 5, step ? 4 : 3);
    b.ellipse(28, 20, 5, 5, step ? 3 : 4);
    b.rect(30, 8, 8, 3, 5);
    b.outline(6, false);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(24, 26);
    b.rect(10, 0, 4, 5, 3);
    b.ellipse(12, 14, 10, 8, 1);
    b.ellipse(12, 16, 5, 4, 2);
    b.rect(10, 21, 4, 4, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap ropeArt() {
    gs::Bitmap b(8, 40);
    b.rect(3, 0, 2, 32, 1);
    b.ellipse(4, 35, 3, 4, 2);
    return b;
}

gs::Bitmap sluiceArt() {
    gs::Bitmap b(20, 28);
    b.rect(2, 2, 4, 24, 2);
    b.rect(14, 2, 4, 24, 2);
    b.rect(2, 8, 16, 4, 1);
    b.rect(2, 16, 16, 3, 3);
    return b;
}

gs::Bitmap sweepArt() {
    gs::Bitmap b(48, 8);
    b.rect(0, 3, 40, 2, 1);
    b.rect(36, 1, 10, 6, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t hud[] = {gs::rgb4(0, 0, 0), gs::rgb4(14, 13, 9), gs::rgb4(6, 5, 3), gs::rgb4(15, 15, 12)};
    const uint16_t millp[] = {0, gs::rgb4(6, 5, 3), gs::rgb4(8, 7, 5), gs::rgb4(12, 11, 8), gs::rgb4(14, 13, 10),
                              gs::rgb4(10, 4, 3), gs::rgb4(5, 3, 2), gs::rgb4(9, 12, 14), gs::rgb4(2, 2, 2),
                              gs::rgb4(4, 4, 4), gs::rgb4(3, 2, 1)};
    const uint16_t sail[] = {0, gs::rgb4(13, 13, 11), gs::rgb4(7, 7, 6), gs::rgb4(10, 8, 5), gs::rgb4(4, 3, 2)};
    const uint16_t you[] = {0, gs::rgb4(9, 6, 3), gs::rgb4(6, 4, 2), gs::rgb4(13, 9, 6), gs::rgb4(4, 3, 2),
                            gs::rgb4(12, 10, 6), gs::rgb4(3, 3, 5), gs::rgb4(2, 2, 2), gs::rgb4(1, 1, 1)};
    const uint16_t foe[] = {0, gs::rgb4(8, 3, 2), gs::rgb4(5, 5, 4), gs::rgb4(12, 8, 6), gs::rgb4(3, 2, 2),
                            gs::rgb4(10, 8, 5), gs::rgb4(7, 7, 6), gs::rgb4(4, 3, 2), gs::rgb4(2, 2, 1),
                            gs::rgb4(11, 11, 9), gs::rgb4(1, 1, 1)};
    const uint16_t wain[] = {0, gs::rgb4(8, 5, 2), gs::rgb4(12, 9, 4), gs::rgb4(3, 3, 3), gs::rgb4(5, 5, 5),
                             gs::rgb4(6, 3, 2), gs::rgb4(2, 1, 1)};
    const uint16_t bell[] = {0, gs::rgb4(13, 11, 4), gs::rgb4(8, 6, 2), gs::rgb4(6, 5, 3), gs::rgb4(15, 14, 8),
                             gs::rgb4(4, 3, 1)};
    const uint16_t fx[] = {0, gs::rgb4(14, 12, 6), gs::rgb4(15, 15, 10), gs::rgb4(8, 10, 12)};
    const uint16_t ok[] = {0, gs::rgb4(8, 13, 6), gs::rgb4(4, 8, 3)};
    const uint16_t alert[] = {0, gs::rgb4(14, 5, 3), gs::rgb4(8, 2, 2)};
    const uint16_t water[] = {0, gs::rgb4(4, 8, 10), gs::rgb4(7, 11, 12), gs::rgb4(3, 5, 6)};
    setPal(vdp, PAL_HUD, hud, 4);
    setPal(vdp, PAL_MILL, millp, 11);
    setPal(vdp, PAL_SAIL, sail, 5);
    setPal(vdp, PAL_YOU, you, 9);
    setPal(vdp, PAL_FOE, foe, 11);
    setPal(vdp, PAL_WAIN, wain, 7);
    setPal(vdp, PAL_BELL, bell, 6);
    setPal(vdp, PAL_FX, fx, 4);
    setPal(vdp, PAL_OK, ok, 3);
    setPal(vdp, PAL_ALERT, alert, 3);
    setPal(vdp, PAL_WATER, water, 4);
    vdp.setFogColor(gs::rgb4(6, 7, 9));

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    art.mill = gs::uploadMipped(vdp, millArt());
    art.cap = gs::uploadMipped(vdp, capArt());
    art.sailPlus = gs::uploadMipped(vdp, sailPlus());
    art.sailCross = gs::uploadMipped(vdp, sailCross());
    art.miller = gs::uploadMipped(vdp, millerArt());
    art.sack = gs::uploadMipped(vdp, sackArt());
    art.reaper[0] = gs::uploadMipped(vdp, body(0, 0));
    art.reaper[1] = gs::uploadMipped(vdp, body(1, 0));
    art.runner[0] = gs::uploadMipped(vdp, body(0, 1));
    art.runner[1] = gs::uploadMipped(vdp, body(1, 1));
    art.wain[0] = gs::uploadMipped(vdp, wainBody(0));
    art.wain[1] = gs::uploadMipped(vdp, wainBody(1));
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.rope = gs::uploadMipped(vdp, ropeArt());
    art.sluice = gs::uploadMipped(vdp, sluiceArt());
    art.sweep = gs::uploadMipped(vdp, sweepArt());
}

}  // namespace mill

#include "art.h"

#include <string>

namespace redoubt {
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

gs::Bitmap soldier(int step, bool plated) {
    gs::Bitmap b(24, 36);
    b.ellipse(12, 6, 5, 5, plated ? 4 : 3);
    b.rect(9, 10, 6, 3, 5);
    b.rect(7, 13, 10, 10, plated ? 2 : 1);
    if (plated) b.rect(8, 14, 8, 6, 6);
    b.rect(step ? 4.f : 8.f, 14, 4, 8, 7);
    b.rect(step ? 16.f : 12.f, 14, 4, 8, 7);
    b.rect(step ? 8.f : 6.f, 23, 4, 9, 1);
    b.rect(step ? 13.f : 14.f, 23, 4, 9, 2);
    b.rect(5, 32, 5, 3, 8);
    b.rect(14, 32, 5, 3, 8);
    b.rect(16, 16, 7, 2, 9);
    b.outline(10, false);
    return b;
}

gs::Bitmap bermArt() {
    gs::Bitmap b(320, 56);
    b.poly({{0, 28}, {40, 10}, {90, 18}, {140, 6}, {190, 16}, {250, 8}, {320, 22}, {320, 56}, {0, 56}}, 1);
    b.poly({{0, 34}, {50, 20}, {110, 28}, {170, 16}, {230, 26}, {320, 18}, {320, 56}, {0, 56}}, 2);
    for (int x = 8; x < 312; x += 18) {
        b.rect(float(x), 22, 4, 16, 3);
        b.rect(float(x + 1), 18, 2, 6, 4);
    }
    b.rect(0, 48, 320, 8, 5);
    b.rect(148, 30, 24, 10, 6);
    return b;
}

gs::Bitmap gunArt() {
    gs::Bitmap b(40, 22);
    b.rect(4, 12, 28, 6, 1);
    b.rect(26, 8, 12, 5, 2);
    b.rect(6, 16, 8, 5, 3);
    b.rect(18, 16, 8, 5, 3);
    b.ellipse(10, 10, 5, 5, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap flashArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 7, 5, 1);
    b.ellipse(8, 8, 3, 3, 2);
    return b;
}

gs::Bitmap stakeArt() {
    gs::Bitmap b(8, 20);
    b.poly({{4, 0}, {7, 8}, {5, 20}, {3, 20}, {1, 8}}, 1);
    b.rect(2, 6, 4, 2, 2);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(26, 28);
    b.rect(11, 0, 4, 5, 3);
    b.ellipse(13, 15, 11, 9, 1);
    b.ellipse(13, 17, 6, 5, 2);
    b.rect(11, 22, 4, 4, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap ropeArt() {
    gs::Bitmap b(6, 40);
    b.rect(2, 0, 2, 32, 1);
    b.ellipse(3, 35, 3, 4, 2);
    return b;
}

gs::Bitmap frameArt() {
    gs::Bitmap b(36, 70);
    b.rect(2, 8, 4, 62, 1);
    b.rect(30, 8, 4, 62, 1);
    b.rect(2, 6, 32, 5, 2);
    b.rect(14, 11, 8, 4, 3);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t hud[] = {0, gs::rgb4(14, 13, 10), gs::rgb4(6, 5, 4)};
    const uint16_t earth[] = {0, gs::rgb4(6, 5, 2), gs::rgb4(4, 3, 1), gs::rgb4(8, 7, 3), gs::rgb4(10, 9, 5),
                              gs::rgb4(3, 2, 1), gs::rgb4(9, 8, 4)};
    const uint16_t you[] = {0, gs::rgb4(5, 6, 4), gs::rgb4(8, 8, 6), gs::rgb4(3, 3, 2), gs::rgb4(12, 10, 6),
                            gs::rgb4(2, 2, 2)};
    const uint16_t foe[] = {0, gs::rgb4(7, 3, 2), gs::rgb4(5, 2, 1), gs::rgb4(10, 6, 4), gs::rgb4(4, 3, 2),
                            gs::rgb4(9, 8, 6), gs::rgb4(6, 5, 3), gs::rgb4(8, 4, 2), gs::rgb4(3, 2, 1),
                             gs::rgb4(12, 10, 4), gs::rgb4(1, 1, 1)};
    const uint16_t shield[] = {0, gs::rgb4(5, 5, 6), gs::rgb4(8, 8, 9), gs::rgb4(12, 11, 8), gs::rgb4(4, 4, 5),
                               gs::rgb4(9, 8, 6), gs::rgb4(11, 11, 12), gs::rgb4(7, 6, 5), gs::rgb4(3, 3, 3),
                               gs::rgb4(13, 12, 6), gs::rgb4(1, 1, 1)};
    const uint16_t fx[] = {0, gs::rgb4(15, 14, 6), gs::rgb4(15, 8, 2), gs::rgb4(14, 14, 12)};
    const uint16_t bell[] = {0, gs::rgb4(13, 10, 3), gs::rgb4(8, 6, 2), gs::rgb4(6, 5, 3), gs::rgb4(15, 13, 7),
                             gs::rgb4(4, 3, 2)};
    const uint16_t alert[] = {0, gs::rgb4(15, 4, 3)};
    const uint16_t ok[] = {0, gs::rgb4(6, 14, 7)};
    const uint16_t road[] = {
        0,
        gs::rgb4(3, 5, 2), gs::rgb4(2, 4, 1), gs::rgb4(5, 6, 3),
        gs::rgb4(6, 5, 2), gs::rgb4(4, 3, 1),
        gs::rgb4(7, 6, 3), gs::rgb4(5, 4, 2),
        gs::rgb4(8, 7, 4), gs::rgb4(6, 5, 3), gs::rgb4(9, 8, 5),
        gs::rgb4(2, 3, 6), gs::rgb4(3, 4, 7), gs::rgb4(8, 9, 11),
        gs::rgb4(12, 11, 6), gs::rgb4(10, 9, 5)};
    setPal(vdp, PAL_HUD, hud, 3);
    setPal(vdp, PAL_EARTH, earth, 7);
    setPal(vdp, PAL_YOU, you, 6);
    setPal(vdp, PAL_FOE, foe, 11);
    setPal(vdp, PAL_SHIELD, shield, 11);
    setPal(vdp, PAL_FX, fx, 4);
    setPal(vdp, PAL_BELL, bell, 6);
    setPal(vdp, PAL_ALERT, alert, 2);
    setPal(vdp, PAL_OK, ok, 2);
    setPal(vdp, PAL_ROAD, road, 16);
    vdp.setFogColor(gs::rgb4(8, 6, 5));

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    art.berm = gs::uploadMipped(vdp, bermArt());
    art.gun = gs::uploadMipped(vdp, gunArt());
    art.flash = gs::uploadMipped(vdp, flashArt());
    art.stake = gs::uploadMipped(vdp, stakeArt());
    art.walker[0] = gs::uploadMipped(vdp, soldier(0, false));
    art.walker[1] = gs::uploadMipped(vdp, soldier(1, false));
    art.shield[0] = gs::uploadMipped(vdp, soldier(0, true));
    art.shield[1] = gs::uploadMipped(vdp, soldier(1, true));
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.rope = gs::uploadMipped(vdp, ropeArt());
    art.frame = gs::uploadMipped(vdp, frameArt());
}

}  // namespace redoubt

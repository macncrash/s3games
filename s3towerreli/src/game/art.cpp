#include "art.h"

#include <cstdint>

namespace tower {
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

gs::Bitmap climber(int step, bool hook, bool shield) {
    gs::Bitmap b(24, 36);
    b.ellipse(12, 6, 5, 5, 3);
    b.rect(9, 11, 6, 3, 4);
    b.rect(7, 14, 10, 10, shield ? 2 : 1);
    if (shield) b.ellipse(12, 18, 7, 6, 5);
    int arm = step ? 3 : 15;
    b.rect(float(arm), 14, 5, 3, 6);
    if (hook) b.line(float(arm), 14, float(arm + (step ? -2 : 4)), 8, 7, 1.4f);
    b.rect(step ? 8.f : 11.f, 24, 3, 8, 1);
    b.rect(step ? 13.f : 10.f, 24, 3, 8, 2);
    b.rect(7, 31, 4, 3, 8);
    b.rect(13, 31, 4, 3, 8);
    b.outline(9, false);
    return b;
}

gs::Bitmap towerArt() {
    gs::Bitmap b(110, 190);
    b.rect(28, 18, 54, 168, 2);
    b.rect(32, 22, 46, 160, 3);
    for (int y = 28; y < 176; y += 10) {
        b.rect(32, float(y), 46, 2, 1);
        b.rect(34, float(y + 4), 4, 3, 4);
        b.rect(72, float(y + 4), 4, 3, 4);
    }
    for (int i = 0; i < 6; i++) {
        b.rect(float(22 + i * 12), 8, 8, 14, 5);
        b.rect(float(24 + i * 12), 4, 4, 6, 6);
    }
    b.rect(44, 36, 22, 28, 0);
    b.rect(46, 38, 18, 24, 7);
    b.ellipse(55, 50, 7, 8, 0);
    b.rect(18, 176, 10, 14, 4);
    b.rect(82, 176, 10, 14, 4);
    for (int y = 40; y < 170; y += 8) {
        b.rect(20, float(y), 6, 2, 8);
        b.rect(84, float(y), 6, 2, 8);
        b.rect(52, float(y), 6, 2, 8);
    }
    b.outline(9, false);
    return b;
}

gs::Bitmap flagArt() {
    gs::Bitmap b(28, 16);
    b.rect(0, 0, 2, 16, 3);
    b.poly({{2, 1}, {26, 5}, {2, 10}}, 1);
    b.poly({{4, 3}, {20, 6}, {4, 8}}, 2);
    return b;
}

gs::Bitmap sentinelArt() {
    gs::Bitmap b(28, 40);
    b.ellipse(14, 8, 6, 6, 3);
    b.rect(10, 6, 8, 3, 4);
    b.rect(8, 14, 12, 14, 1);
    b.rect(10, 16, 8, 8, 2);
    b.rect(6, 16, 4, 10, 5);
    b.rect(18, 16, 4, 10, 5);
    b.line(20, 12, 26, 4, 6, 1.5f);
    b.rect(9, 28, 4, 9, 7);
    b.rect(15, 28, 4, 9, 7);
    b.outline(8, false);
    return b;
}

gs::Bitmap stoneArt() {
    gs::Bitmap b(12, 12);
    b.ellipse(6, 6, 5, 4, 1);
    b.ellipse(5, 5, 2, 2, 2);
    b.outline(3, false);
    return b;
}

gs::Bitmap flashArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 6, 6, 1);
    b.ellipse(8, 8, 3, 3, 2);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(26, 28);
    b.rect(11, 0, 4, 5, 3);
    b.ellipse(13, 15, 11, 9, 1);
    b.ellipse(13, 16, 6, 5, 2);
    b.rect(11, 22, 4, 4, 4);
    b.ellipse(13, 26, 2, 2, 5);
    b.outline(6, false);
    return b;
}

gs::Bitmap ropeArt() {
    gs::Bitmap b(8, 40);
    b.rect(3, 0, 2, 32, 1);
    b.ellipse(4, 35, 3, 4, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    vdp.setFogColor(gs::rgb4(2, 2, 4));
    const uint16_t hud[] = {0, gs::rgb4(14, 13, 10), gs::rgb4(6, 6, 8)};
    const uint16_t stone[] = {0,
                              gs::rgb4(5, 5, 6),
                              gs::rgb4(7, 7, 8),
                              gs::rgb4(9, 9, 10),
                              gs::rgb4(4, 4, 5),
                              gs::rgb4(8, 8, 9),
                              gs::rgb4(11, 11, 12),
                              gs::rgb4(3, 3, 5),
                              gs::rgb4(6, 5, 4),
                              gs::rgb4(2, 2, 3)};
    const uint16_t you[] = {0,
                            gs::rgb4(3, 4, 8),
                            gs::rgb4(6, 7, 12),
                            gs::rgb4(12, 9, 6),
                            gs::rgb4(8, 5, 3),
                            gs::rgb4(4, 5, 9),
                            gs::rgb4(10, 10, 8),
                            gs::rgb4(2, 2, 4),
                            gs::rgb4(1, 1, 2)};
    const uint16_t climb[] = {0,
                              gs::rgb4(6, 3, 2),
                              gs::rgb4(8, 4, 3),
                              gs::rgb4(10, 7, 5),
                              gs::rgb4(4, 2, 2),
                              gs::rgb4(9, 8, 6),
                              gs::rgb4(5, 3, 2),
                              gs::rgb4(12, 10, 6),
                              gs::rgb4(3, 2, 2),
                              gs::rgb4(1, 1, 1)};
    const uint16_t shield[] = {0,
                               gs::rgb4(4, 5, 6),
                               gs::rgb4(7, 8, 9),
                               gs::rgb4(10, 8, 6),
                               gs::rgb4(3, 3, 4),
                               gs::rgb4(11, 12, 13),
                               gs::rgb4(5, 6, 7),
                               gs::rgb4(13, 12, 8),
                               gs::rgb4(2, 2, 3),
                               gs::rgb4(1, 1, 2)};
    const uint16_t fx[] = {0, gs::rgb4(14, 13, 8), gs::rgb4(15, 15, 12), gs::rgb4(6, 5, 4)};
    const uint16_t bell[] = {0,
                             gs::rgb4(12, 9, 3),
                             gs::rgb4(8, 6, 2),
                             gs::rgb4(5, 4, 2),
                             gs::rgb4(14, 12, 6),
                             gs::rgb4(15, 14, 8),
                             gs::rgb4(3, 2, 1)};
    const uint16_t alert[] = {0, gs::rgb4(14, 4, 3), gs::rgb4(8, 2, 2)};
    const uint16_t ok[] = {0, gs::rgb4(6, 13, 7), gs::rgb4(3, 7, 4)};
    setPal(vdp, PAL_HUD, hud, 3);
    setPal(vdp, PAL_STONE, stone, 10);
    setPal(vdp, PAL_YOU, you, 9);
    setPal(vdp, PAL_CLIMB, climb, 10);
    setPal(vdp, PAL_SHIELD, shield, 10);
    setPal(vdp, PAL_FX, fx, 4);
    setPal(vdp, PAL_BELL, bell, 7);
    setPal(vdp, PAL_ALERT, alert, 3);
    setPal(vdp, PAL_OK, ok, 3);

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    art.tower = gs::uploadMipped(vdp, towerArt());
    art.flag = gs::uploadMipped(vdp, flagArt());
    art.sentinel = gs::uploadMipped(vdp, sentinelArt());
    art.stone = gs::uploadMipped(vdp, stoneArt());
    art.flash = gs::uploadMipped(vdp, flashArt());
    art.climb[0] = gs::uploadMipped(vdp, climber(0, false, false));
    art.climb[1] = gs::uploadMipped(vdp, climber(1, false, false));
    art.hook[0] = gs::uploadMipped(vdp, climber(0, true, false));
    art.hook[1] = gs::uploadMipped(vdp, climber(1, true, false));
    art.shield[0] = gs::uploadMipped(vdp, climber(0, false, true));
    art.shield[1] = gs::uploadMipped(vdp, climber(1, false, true));
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.rope = gs::uploadMipped(vdp, ropeArt());
}

}  // namespace tower

#include "art.h"

#include <string>

namespace redoubtpurs {
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

gs::Bitmap machine(int step, int body, int trim, int stack) {
    gs::Bitmap b(36, 28);
    b.rect(4, 14, 28, 8, body);
    b.rect(8, 8, 16, 8, trim);
    b.rect(20, 4, 4, 6, stack);
    b.rect(22, 1, 2, 4, 5);
    b.ellipse(10, 22, 5, 5, 4);
    b.ellipse(26, 22, 5, 5, 4);
    if (step) {
        b.rect(6, 20, 3, 3, 6);
        b.rect(22, 20, 3, 3, 6);
    }
    b.rect(14, 11, 8, 3, 7);
    b.outline(8, false);
    return b;
}

gs::Bitmap youArt(int step) {
    gs::Bitmap b = machine(step, 1, 2, 3);
    b.rect(16, 0, 3, 8, 3);
    b.rect(15, 6, 6, 3, 9);
    return b;
}

gs::Bitmap lightArt(int step) { return machine(step, 1, 2, 3); }

gs::Bitmap heavyArt(int step) {
    gs::Bitmap b = machine(step, 1, 2, 4);
    b.rect(6, 10, 24, 4, 6);
    b.rect(4, 13, 28, 3, 7);
    b.outline(8, false);
    return b;
}

gs::Bitmap wreckArt() {
    gs::Bitmap b(36, 22);
    b.poly({{2, 16}, {8, 8}, {18, 12}, {30, 6}, {34, 14}, {28, 20}, {4, 20}}, 1);
    b.rect(10, 14, 8, 4, 2);
    b.ellipse(8, 18, 4, 3, 3);
    b.ellipse(26, 17, 5, 3, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap bermArt() {
    gs::Bitmap b(320, 48);
    b.poly({{0, 16}, {40, 6}, {90, 14}, {150, 4}, {210, 12}, {270, 6}, {320, 14}, {320, 48}, {0, 48}}, 1);
    b.poly({{0, 24}, {60, 16}, {130, 22}, {200, 14}, {320, 20}, {320, 48}, {0, 48}}, 2);
    for (int x = 6; x < 314; x += 22) b.rect(float(x), 10, 3, 14, 3);
    b.rect(0, 40, 320, 8, 4);
    b.rect(140, 22, 40, 8, 5);
    return b;
}

gs::Bitmap boltArt() {
    gs::Bitmap b(6, 12);
    b.rect(2, 0, 2, 10, 1);
    b.rect(1, 8, 4, 3, 2);
    return b;
}

gs::Bitmap puffArt() {
    gs::Bitmap b(12, 12);
    b.ellipse(6, 6, 5, 4, 1);
    b.ellipse(6, 6, 2, 2, 2);
    return b;
}

gs::Bitmap stakeArt() {
    gs::Bitmap b(8, 18);
    b.poly({{4, 0}, {7, 7}, {5, 18}, {3, 18}, {1, 7}}, 1);
    b.rect(2, 5, 4, 2, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t hud[] = {0, gs::rgb4(14, 13, 9), gs::rgb4(5, 4, 3)};
    const uint16_t earth[] = {0, gs::rgb4(6, 5, 2), gs::rgb4(4, 3, 1), gs::rgb4(8, 7, 3), gs::rgb4(3, 2, 1),
                              gs::rgb4(9, 8, 4)};
    const uint16_t you[] = {0, gs::rgb4(4, 6, 3), gs::rgb4(7, 8, 5), gs::rgb4(10, 9, 4), gs::rgb4(2, 2, 2),
                            gs::rgb4(12, 12, 10), gs::rgb4(3, 3, 2), gs::rgb4(8, 10, 6), gs::rgb4(1, 1, 1),
                            gs::rgb4(13, 11, 5)};
    const uint16_t light[] = {0, gs::rgb4(8, 4, 2), gs::rgb4(6, 3, 1), gs::rgb4(10, 7, 3), gs::rgb4(3, 2, 2),
                              gs::rgb4(12, 11, 8), gs::rgb4(4, 3, 2), gs::rgb4(9, 6, 3), gs::rgb4(1, 1, 1)};
    const uint16_t heavy[] = {0, gs::rgb4(4, 4, 5), gs::rgb4(6, 6, 7), gs::rgb4(3, 3, 3), gs::rgb4(8, 7, 4),
                              gs::rgb4(2, 2, 2), gs::rgb4(9, 8, 6), gs::rgb4(5, 4, 3), gs::rgb4(1, 1, 1)};
    const uint16_t bolt[] = {0, gs::rgb4(15, 14, 6), gs::rgb4(15, 8, 2)};
    const uint16_t fx[] = {0, gs::rgb4(12, 11, 9), gs::rgb4(15, 14, 8)};
    const uint16_t wreck[] = {0, gs::rgb4(4, 3, 2), gs::rgb4(6, 3, 1), gs::rgb4(2, 2, 2), gs::rgb4(1, 1, 1)};
    const uint16_t ok[] = {0, gs::rgb4(6, 14, 6)};
    const uint16_t road[] = {
        0,
        gs::rgb4(3, 5, 2), gs::rgb4(2, 4, 1), gs::rgb4(5, 6, 3),
        gs::rgb4(6, 5, 2), gs::rgb4(4, 3, 1),
        gs::rgb4(7, 6, 3), gs::rgb4(5, 4, 2),
        gs::rgb4(8, 7, 4), gs::rgb4(6, 5, 3), gs::rgb4(9, 8, 5),
        gs::rgb4(2, 3, 6), gs::rgb4(3, 4, 7), gs::rgb4(8, 9, 11),
        gs::rgb4(12, 11, 6), gs::rgb4(10, 9, 5)};
    setPal(vdp, PAL_HUD, hud, 3);
    setPal(vdp, PAL_EARTH, earth, 6);
    setPal(vdp, PAL_YOU, you, 10);
    setPal(vdp, PAL_LIGHT, light, 9);
    setPal(vdp, PAL_HEAVY, heavy, 9);
    setPal(vdp, PAL_BOLT, bolt, 3);
    setPal(vdp, PAL_FX, fx, 3);
    setPal(vdp, PAL_WRECK, wreck, 5);
    setPal(vdp, PAL_OK, ok, 2);
    setPal(vdp, PAL_ROAD, road, 16);
    vdp.setFogColor(gs::rgb4(7, 6, 5));

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    art.berm = gs::uploadMipped(vdp, bermArt());
    art.you[0] = gs::uploadMipped(vdp, youArt(0));
    art.you[1] = gs::uploadMipped(vdp, youArt(1));
    art.light[0] = gs::uploadMipped(vdp, lightArt(0));
    art.light[1] = gs::uploadMipped(vdp, lightArt(1));
    art.heavy[0] = gs::uploadMipped(vdp, heavyArt(0));
    art.heavy[1] = gs::uploadMipped(vdp, heavyArt(1));
    art.wreck = gs::uploadMipped(vdp, wreckArt());
    art.bolt = gs::uploadMipped(vdp, boltArt());
    art.puff = gs::uploadMipped(vdp, puffArt());
    art.stake = gs::uploadMipped(vdp, stakeArt());
}

}  // namespace redoubtpurs

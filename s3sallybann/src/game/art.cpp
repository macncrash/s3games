#include "game/art.h"

#include <string>

namespace sallybann {
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

gs::Bitmap runnerArt() {
    gs::Bitmap b(28, 40);
    b.ellipse(14, 7, 5, 5, 1);
    b.rect(11, 4, 6, 3, 4);
    b.rect(10, 11, 8, 3, 4);
    b.rect(9, 14, 10, 12, 2);
    b.rect(10, 16, 8, 6, 3);
    b.rect(8, 14, 3, 8, 6);
    b.rect(19, 15, 3, 7, 6);
    b.rect(11, 26, 3, 9, 5);
    b.rect(16, 26, 3, 9, 5);
    b.rect(10, 34, 4, 3, 5);
    b.rect(16, 34, 4, 3, 5);
    b.rect(20, 8, 2, 22, 4);
    b.poly({{20, 6}, {26, 5}, {21, 10}}, 6);
    b.outline(5, false);
    return b;
}

gs::Bitmap bannerArt() {
    gs::Bitmap b(30, 52);
    b.rect(5, 6, 3, 42, 1);
    b.rect(3, 46, 7, 3, 1);
    b.poly({{8, 8}, {26, 14}, {26, 28}, {8, 24}}, 2);
    b.rect(10, 14, 12, 4, 3);
    b.poly({{8, 26}, {24, 30}, {8, 34}}, 4);
    b.ellipse(6, 5, 2, 2, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap jambArt() {
    gs::Bitmap b(28, 110);
    b.rect(0, 0, 28, 110, 1);
    for (int y = 4; y < 104; y += 12) {
        int off = ((y / 12) & 1) ? 4 : 0;
        for (int x = off; x < 24; x += 12) b.rect(float(x), float(y), 10, 8, ((x + y) & 8) ? 2 : 3);
    }
    b.rect(0, 96, 28, 14, 4);
    b.outline(2, false);
    return b;
}

gs::Bitmap lintelArt() {
    gs::Bitmap b(78, 18);
    b.rect(0, 4, 78, 12, 1);
    b.rect(0, 0, 78, 5, 3);
    for (int x = 4; x < 74; x += 12) b.rect(float(x), 6, 8, 6, 2);
    b.outline(2, false);
    return b;
}

gs::Bitmap boltArt() {
    gs::Bitmap b(18, 6);
    b.rect(0, 2, 12, 2, 2);
    b.poly({{10, 0}, {17, 3}, {10, 6}}, 1);
    b.rect(1, 1, 2, 4, 3);
    return b;
}

gs::Bitmap sodArt() {
    gs::Bitmap b(36, 18);
    b.rect(0, 6, 36, 12, 1);
    b.rect(0, 4, 36, 4, 2);
    for (int x = 2; x < 34; x += 6) b.line(float(x), 4, float(x + ((x / 6) & 1 ? 2 : -1)), 0, 3, 1);
    return b;
}

gs::Bitmap puffArt() {
    gs::Bitmap b(12, 10);
    b.ellipse(6, 5, 5, 3, 1);
    b.ellipse(4, 4, 2, 2, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t hud[] = {0, gs::rgb4(14, 13, 10)};
    const uint16_t stone[] = {0, gs::rgb4(4, 4, 5), gs::rgb4(7, 7, 8), gs::rgb4(11, 11, 12), gs::rgb4(3, 5, 3)};
    const uint16_t coat[] = {0, gs::rgb4(12, 8, 6), gs::rgb4(9, 2, 2), gs::rgb4(5, 1, 1),
                             gs::rgb4(6, 7, 8), gs::rgb4(2, 2, 2), gs::rgb4(13, 10, 3)};
    const uint16_t banner[] = {0, gs::rgb4(6, 4, 2), gs::rgb4(13, 2, 2), gs::rgb4(14, 12, 3), gs::rgb4(7, 1, 2)};
    const uint16_t bolt[] = {0, gs::rgb4(12, 12, 13), gs::rgb4(8, 5, 2), gs::rgb4(14, 14, 11)};
    const uint16_t ground[] = {0, gs::rgb4(5, 4, 2), gs::rgb4(3, 7, 2), gs::rgb4(7, 11, 3)};
    const uint16_t good[] = {0, gs::rgb4(4, 14, 6)};
    const uint16_t alert[] = {0, gs::rgb4(15, 4, 3)};
    const uint16_t gold[] = {0, gs::rgb4(15, 12, 4)};
    setPal(vdp, PAL_HUD, hud, 2);
    setPal(vdp, PAL_STONE, stone, 5);
    setPal(vdp, PAL_COAT, coat, 7);
    setPal(vdp, PAL_BANNER, banner, 5);
    setPal(vdp, PAL_BOLT, bolt, 4);
    setPal(vdp, PAL_GROUND, ground, 4);
    setPal(vdp, PAL_GOOD, good, 2);
    setPal(vdp, PAL_ALERT, alert, 2);
    setPal(vdp, PAL_GOLD, gold, 2);

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    art.runner = gs::uploadMipped(vdp, runnerArt());
    art.banner = gs::uploadMipped(vdp, bannerArt());
    art.jamb = gs::uploadMipped(vdp, jambArt());
    art.lintel = gs::uploadMipped(vdp, lintelArt());
    art.bolt = gs::uploadMipped(vdp, boltArt());
    art.sod = gs::uploadMipped(vdp, sodArt());
    art.puff = gs::uploadMipped(vdp, puffArt());
    vdp.setFogColor(gs::rgb4(2, 2, 4));
}

}  // namespace sallybann

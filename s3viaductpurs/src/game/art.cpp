#include "art.h"

#include <string>

namespace viaductpurs {
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

gs::Bitmap runnerArt(int step) {
    gs::Bitmap b(36, 22);
    b.rect(2, 8, 28, 10, 1);
    b.rect(18, 4, 10, 8, 2);
    b.rect(22, 6, 4, 3, 3);
    b.rect(step ? 4.f : 8.f, 17, 6, 4, 4);
    b.rect(step ? 20.f : 16.f, 17, 6, 4, 4);
    b.rect(28, 10, 6, 3, 5);
    b.rect(6, 10, 4, 3, 6);
    b.outline(7, false);
    return b;
}

gs::Bitmap railArt(int step) {
    gs::Bitmap b(40, 24);
    b.rect(2, 9, 32, 9, 1);
    b.rect(8, 4, 16, 7, 2);
    b.rect(12, 6, 6, 3, 3);
    b.rect(step ? 4.f : 10.f, 17, 7, 5, 4);
    b.rect(step ? 22.f : 16.f, 17, 7, 5, 4);
    b.rect(30, 11, 8, 3, 5);
    b.outline(6, false);
    return b;
}

gs::Bitmap craneArt(int step) {
    gs::Bitmap b(44, 28);
    b.rect(4, 14, 28, 8, 1);
    b.rect(10, 8, 6, 8, 2);
    b.rect(14, 4, 22, 3, 3);
    b.rect(32, 6, 2, 10, 3);
    b.rect(step ? 6.f : 12.f, 21, 6, 5, 4);
    b.rect(step ? 20.f : 14.f, 21, 6, 5, 4);
    b.rect(30, 16, 6, 3, 5);
    b.outline(6, false);
    return b;
}

gs::Bitmap busArt(int step) {
    gs::Bitmap b(42, 22);
    b.rect(2, 6, 36, 11, 1);
    b.rect(6, 8, 6, 4, 2);
    b.rect(16, 8, 6, 4, 2);
    b.rect(26, 8, 6, 4, 2);
    b.rect(step ? 6.f : 10.f, 16, 6, 4, 3);
    b.rect(step ? 26.f : 22.f, 16, 6, 4, 3);
    b.rect(36, 9, 4, 4, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap boltArt() {
    gs::Bitmap b(10, 6);
    b.rect(1, 2, 8, 2, 1);
    b.rect(6, 1, 3, 4, 2);
    return b;
}

gs::Bitmap puffArt() {
    gs::Bitmap b(16, 14);
    b.ellipse(8, 8, 6, 5, 1);
    b.ellipse(5, 6, 3, 3, 2);
    b.ellipse(11, 7, 3, 2, 3);
    return b;
}

gs::Bitmap pierArt() {
    gs::Bitmap b(28, 90);
    b.rect(8, 8, 12, 78, 1);
    b.rect(4, 4, 20, 10, 2);
    b.rect(10, 20, 8, 6, 3);
    b.rect(10, 40, 8, 6, 3);
    b.rect(10, 60, 8, 6, 3);
    b.rect(6, 82, 16, 6, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(8, 16);
    b.rect(3, 0, 2, 8, 2);
    b.ellipse(4, 12, 3, 3, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t hud[] = {0, gs::rgb4(14, 13, 10), gs::rgb4(7, 7, 8), gs::rgb4(15, 12, 5), gs::rgb4(15, 6, 3)};
    const uint16_t you[] = {0, gs::rgb4(4, 7, 5), gs::rgb4(7, 10, 8), gs::rgb4(12, 14, 10), gs::rgb4(2, 2, 2),
                            gs::rgb4(14, 12, 4), gs::rgb4(10, 8, 3), gs::rgb4(1, 1, 1)};
    const uint16_t rail[] = {0, gs::rgb4(6, 3, 2), gs::rgb4(9, 5, 3), gs::rgb4(13, 10, 6), gs::rgb4(2, 2, 2),
                             gs::rgb4(14, 8, 3), gs::rgb4(1, 1, 1)};
    const uint16_t crane[] = {0, gs::rgb4(5, 5, 3), gs::rgb4(8, 7, 4), gs::rgb4(12, 10, 4), gs::rgb4(3, 3, 2),
                              gs::rgb4(14, 6, 2), gs::rgb4(1, 1, 1)};
    const uint16_t bus[] = {0, gs::rgb4(3, 4, 7), gs::rgb4(10, 12, 14), gs::rgb4(2, 2, 3), gs::rgb4(14, 10, 3),
                            gs::rgb4(1, 1, 2)};
    const uint16_t bolt[] = {0, gs::rgb4(15, 14, 6), gs::rgb4(15, 8, 2)};
    const uint16_t stone[] = {0, gs::rgb4(6, 6, 7), gs::rgb4(9, 8, 7), gs::rgb4(4, 5, 6), gs::rgb4(3, 3, 3),
                              gs::rgb4(1, 1, 2)};
    const uint16_t ok[] = {0, gs::rgb4(8, 14, 7), gs::rgb4(3, 6, 3)};
    const uint16_t alert[] = {0, gs::rgb4(15, 5, 3), gs::rgb4(8, 2, 2)};
    setPal(vdp, PAL_HUD, hud, 5);
    setPal(vdp, PAL_YOU, you, 8);
    setPal(vdp, PAL_RAIL, rail, 7);
    setPal(vdp, PAL_CRANE, crane, 7);
    setPal(vdp, PAL_BUS, bus, 6);
    setPal(vdp, PAL_BOLT, bolt, 3);
    setPal(vdp, PAL_STONE, stone, 6);
    setPal(vdp, PAL_OK, ok, 3);
    setPal(vdp, PAL_ALERT, alert, 3);
    vdp.setFogColor(gs::rgb4(3, 3, 5));

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, tiles, art);
    art.runner[0] = gs::uploadMipped(vdp, runnerArt(0));
    art.runner[1] = gs::uploadMipped(vdp, runnerArt(1));
    art.rail[0] = gs::uploadMipped(vdp, railArt(0));
    art.rail[1] = gs::uploadMipped(vdp, railArt(1));
    art.crane[0] = gs::uploadMipped(vdp, craneArt(0));
    art.crane[1] = gs::uploadMipped(vdp, craneArt(1));
    art.bus[0] = gs::uploadMipped(vdp, busArt(0));
    art.bus[1] = gs::uploadMipped(vdp, busArt(1));
    art.bolt = gs::uploadMipped(vdp, boltArt());
    art.puff = gs::uploadMipped(vdp, puffArt());
    art.pier = gs::uploadMipped(vdp, pierArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
}

}  // namespace viaductpurs

#include "art.h"

#include <cstdint>

namespace causeway {
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

gs::Bitmap gunnerArt(int step) {
    gs::Bitmap b(32, 44);
    b.ellipse(16, 7, 6, 6, 3);
    b.rect(12, 12, 8, 4, 4);
    b.rect(9, 16, 14, 12, 1);
    b.rect(11, 18, 10, 6, 2);
    b.rect(step ? 6.f : 10.f, 16, 4, 9, 5);
    b.rect(step ? 22.f : 18.f, 16, 4, 9, 5);
    b.rect(step ? 10.f : 13.f, 28, 4, 11, 1);
    b.rect(step ? 18.f : 15.f, 28, 4, 11, 2);
    b.rect(8, 38, 7, 3, 6);
    b.rect(17, 38, 7, 3, 6);
    b.rect(20, 20, 11, 3, 7);
    b.rect(28, 19, 3, 2, 8);
    b.outline(9, false);
    return b;
}

gs::Bitmap runnerArt(int step) {
    gs::Bitmap b(26, 38);
    b.ellipse(13, 6, 5, 5, 3);
    b.rect(9, 11, 8, 11, 1);
    b.rect(10, 13, 6, 5, 2);
    int arm = step ? 4 : 16;
    b.rect(float(arm), 13, 4, 9, 4);
    b.rect(step ? 8.f : 14.f, 22, 4, 11, 1);
    b.rect(step ? 14.f : 8.f, 22, 4, 11, 5);
    b.rect(6, 32, 6, 3, 6);
    b.rect(14, 32, 6, 3, 6);
    b.rect(16, 14, 8, 2, 7);
    b.outline(8, false);
    return b;
}

gs::Bitmap truckArt() {
    gs::Bitmap b(40, 28);
    b.rect(2, 10, 28, 12, 1);
    b.rect(26, 6, 12, 16, 2);
    b.rect(29, 8, 7, 6, 3);
    b.rect(4, 12, 8, 6, 4);
    b.ellipse(10, 23, 5, 5, 5);
    b.ellipse(30, 23, 5, 5, 5);
    b.ellipse(10, 23, 2, 2, 6);
    b.ellipse(30, 23, 2, 2, 6);
    b.rect(0, 13, 4, 3, 7);
    b.outline(8, false);
    return b;
}

gs::Bitmap boatArt() {
    gs::Bitmap b(36, 16);
    b.poly({{2, 6}, {8, 14}, {30, 14}, {34, 6}, {28, 6}, {8, 6}}, 1);
    b.rect(14, 2, 8, 6, 2);
    b.rect(16, 3, 4, 3, 3);
    b.rect(6, 5, 6, 2, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(24, 28);
    b.rect(10, 0, 4, 5, 3);
    b.ellipse(12, 14, 10, 9, 1);
    b.ellipse(12, 16, 5, 5, 2);
    b.rect(10, 22, 4, 4, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(16, 28);
    b.rect(4, 0, 8, 22, 1);
    b.rect(2, 20, 12, 6, 2);
    b.rect(6, 4, 4, 3, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap flashArt() {
    gs::Bitmap b(14, 14);
    b.ellipse(7, 7, 6, 6, 1);
    b.ellipse(7, 7, 3, 3, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    const uint16_t hud[] = {0, gs::rgb4(14, 14, 12), gs::rgb4(8, 8, 7), gs::rgb4(15, 13, 6), gs::rgb4(15, 5, 3),
                            gs::rgb4(8, 14, 7), shadow};
    const uint16_t you[] = {0, gs::rgb4(4, 5, 4), gs::rgb4(7, 8, 6), gs::rgb4(12, 10, 7), gs::rgb4(6, 5, 3),
                            gs::rgb4(3, 4, 3), gs::rgb4(2, 2, 2), gs::rgb4(8, 8, 7), gs::rgb4(14, 12, 4), gs::rgb4(1, 1, 1)};
    const uint16_t foe[] = {0, gs::rgb4(5, 4, 3), gs::rgb4(8, 6, 4), gs::rgb4(11, 8, 6), gs::rgb4(4, 3, 3),
                            gs::rgb4(6, 5, 4), gs::rgb4(2, 2, 2), gs::rgb4(9, 8, 6), gs::rgb4(1, 1, 1)};
    const uint16_t truck[] = {0, gs::rgb4(5, 6, 4), gs::rgb4(7, 8, 5), gs::rgb4(10, 12, 11), gs::rgb4(3, 4, 3),
                              gs::rgb4(2, 2, 2), gs::rgb4(8, 8, 7), gs::rgb4(12, 4, 2), gs::rgb4(1, 1, 1)};
    const uint16_t boat[] = {0, gs::rgb4(6, 5, 3), gs::rgb4(8, 7, 4), gs::rgb4(3, 5, 6), gs::rgb4(10, 8, 4),
                             gs::rgb4(1, 1, 2)};
    const uint16_t bell[] = {0, gs::rgb4(12, 10, 3), gs::rgb4(8, 6, 2), gs::rgb4(14, 12, 6), gs::rgb4(6, 5, 2),
                             gs::rgb4(3, 2, 1)};
    const uint16_t fx[] = {0, gs::rgb4(15, 14, 8), gs::rgb4(15, 8, 2), gs::rgb4(14, 14, 12)};
    const uint16_t ok[] = {0, gs::rgb4(8, 14, 7), gs::rgb4(4, 8, 4)};
    const uint16_t alert[] = {0, gs::rgb4(15, 5, 3), gs::rgb4(8, 2, 2)};
    const uint16_t stone[] = {0, gs::rgb4(7, 7, 6), gs::rgb4(5, 5, 4), gs::rgb4(10, 9, 6), gs::rgb4(2, 2, 2)};
    setPal(vdp, PAL_HUD, hud, 7);
    setPal(vdp, PAL_YOU, you, 10);
    setPal(vdp, PAL_FOE, foe, 9);
    setPal(vdp, PAL_TRUCK, truck, 9);
    setPal(vdp, PAL_BOAT, boat, 6);
    setPal(vdp, PAL_BELL, bell, 6);
    setPal(vdp, PAL_FX, fx, 4);
    setPal(vdp, PAL_OK, ok, 3);
    setPal(vdp, PAL_ALERT, alert, 3);
    setPal(vdp, PAL_STONE, stone, 5);

    // Road bank 12: stone causeway, water on both sides.
    // 1-3 verge speck, 4-5 edge, 6-7 stone, 8 pebble, 9 track, 10 ridge,
    // 11-13 water, 14 lamp paint, 15 speck.
    const uint16_t roadPal[] = {
        0,
        gs::rgb4(3, 5, 4),
        gs::rgb4(2, 4, 4),
        gs::rgb4(4, 6, 5),
        gs::rgb4(6, 6, 5),
        gs::rgb4(5, 5, 4),
        gs::rgb4(8, 8, 7),
        gs::rgb4(6, 6, 5),
        gs::rgb4(4, 4, 4),
        gs::rgb4(5, 5, 4),
        gs::rgb4(9, 9, 8),
        gs::rgb4(1, 3, 6),
        gs::rgb4(2, 4, 7),
        gs::rgb4(4, 7, 9),
        gs::rgb4(12, 11, 6),
        gs::rgb4(10, 10, 9),
    };
    setPal(vdp, 12, roadPal, 16);

    vdp.setFogColor(gs::rgb4(6, 5, 7));

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, tiles, art);
    art.gunner[0] = gs::uploadMipped(vdp, gunnerArt(0));
    art.gunner[1] = gs::uploadMipped(vdp, gunnerArt(1));
    art.runner[0] = gs::uploadMipped(vdp, runnerArt(0));
    art.runner[1] = gs::uploadMipped(vdp, runnerArt(1));
    art.truck = gs::uploadMipped(vdp, truckArt());
    art.boat = gs::uploadMipped(vdp, boatArt());
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.flash = gs::uploadMipped(vdp, flashArt());
}

}  // namespace causeway

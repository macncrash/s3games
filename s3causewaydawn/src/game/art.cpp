#include "art.h"

#include <string>

namespace cdawn {
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

gs::Bitmap keeperArt(int step) {
    gs::Bitmap b(28, 40);
    b.ellipse(14, 6, 5, 5, 3);
    b.rect(10, 11, 8, 3, 4);
    b.rect(9, 14, 10, 11, 1);
    b.rect(11, 16, 6, 4, 2);
    b.rect(step ? 4.f : 7.f, 15, 4, 9, 5);
    b.rect(19, 15, 6, 3, 6);
    b.ellipse(25, 14, 2, 2, 7);
    b.rect(step ? 10.f : 13.f, 25, 4, 10, 1);
    b.rect(step ? 15.f : 12.f, 25, 4, 10, 2);
    b.rect(8, 34, 6, 3, 8);
    b.rect(15, 34, 6, 3, 8);
    b.outline(9, false);
    return b;
}

gs::Bitmap potArt() {
    gs::Bitmap b(22, 18);
    b.poly({{3, 4}, {19, 4}, {16, 16}, {6, 16}}, 1);
    b.rect(2, 3, 18, 3, 2);
    b.rect(8, 1, 6, 3, 3);
    b.ellipse(11, 10, 3, 2, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap flameArt() {
    gs::Bitmap b(14, 22);
    b.poly({{7, 1}, {12, 10}, {10, 16}, {7, 21}, {4, 16}, {2, 10}}, 1);
    b.poly({{7, 6}, {10, 12}, {7, 18}, {4, 12}}, 2);
    b.ellipse(7, 12, 2, 3, 3);
    return b;
}

gs::Bitmap wickArt() {
    gs::Bitmap b(6, 8);
    b.line(3, 1, 3, 7, 1, 2);
    b.ellipse(3, 2, 2, 2, 2);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(10, 28);
    b.rect(4, 2, 3, 22, 1);
    b.rect(1, 22, 8, 4, 2);
    b.ellipse(5, 3, 3, 2, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap gullArt() {
    gs::Bitmap b(20, 8);
    b.poly({{1, 4}, {8, 2}, {10, 4}, {12, 2}, {19, 4}, {12, 5}, {8, 5}}, 1);
    b.outline(2, false);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t hud[] = {0, gs::rgb4(14, 13, 10), gs::rgb4(7, 7, 8), gs::rgb4(15, 11, 4), gs::rgb4(15, 6, 3),
                            gs::rgb4(8, 13, 9)};
    const uint16_t you[] = {0, gs::rgb4(4, 5, 6), gs::rgb4(7, 8, 8), gs::rgb4(12, 10, 8), gs::rgb4(5, 4, 3),
                            gs::rgb4(3, 4, 5), gs::rgb4(8, 7, 4), gs::rgb4(14, 12, 5), gs::rgb4(2, 2, 3),
                            gs::rgb4(1, 1, 1)};
    const uint16_t pot[] = {0, gs::rgb4(5, 5, 5), gs::rgb4(8, 8, 7), gs::rgb4(3, 3, 3), gs::rgb4(11, 9, 5),
                            gs::rgb4(1, 1, 1)};
    const uint16_t flame[] = {0, gs::rgb4(15, 8, 2), gs::rgb4(15, 13, 4), gs::rgb4(15, 15, 10)};
    const uint16_t sea[] = {0, gs::rgb4(12, 13, 14), gs::rgb4(2, 3, 5)};
    const uint16_t ok[] = {0, gs::rgb4(9, 14, 8), gs::rgb4(4, 8, 5)};
    const uint16_t alert[] = {0, gs::rgb4(15, 5, 3), gs::rgb4(8, 2, 2)};
    const uint16_t post[] = {0, gs::rgb4(6, 6, 6), gs::rgb4(3, 3, 3), gs::rgb4(13, 11, 4), gs::rgb4(1, 1, 1)};
    setPal(vdp, PAL_HUD, hud, 6);
    setPal(vdp, PAL_YOU, you, 10);
    setPal(vdp, PAL_POT, pot, 6);
    setPal(vdp, PAL_FLAME, flame, 4);
    setPal(vdp, PAL_SEA, sea, 3);
    setPal(vdp, PAL_OK, ok, 3);
    setPal(vdp, PAL_ALERT, alert, 3);
    setPal(vdp, PAL_POST, post, 5);

    const uint16_t roadPal[] = {
        0,
        gs::rgb4(2, 3, 4),
        gs::rgb4(1, 2, 3),
        gs::rgb4(4, 5, 5),
        gs::rgb4(6, 6, 6),
        gs::rgb4(4, 4, 4),
        gs::rgb4(8, 8, 7),
        gs::rgb4(5, 5, 5),
        gs::rgb4(3, 3, 3),
        gs::rgb4(7, 6, 4),
        gs::rgb4(10, 9, 7),
        gs::rgb4(1, 2, 5),
        gs::rgb4(1, 3, 7),
        gs::rgb4(2, 5, 9),
        gs::rgb4(12, 10, 5),
        gs::rgb4(9, 9, 8),
    };
    setPal(vdp, 12, roadPal, 16);
    vdp.setFogColor(gs::rgb4(2, 3, 6));

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, tiles, art);
    art.keeper[0] = gs::uploadMipped(vdp, keeperArt(0));
    art.keeper[1] = gs::uploadMipped(vdp, keeperArt(1));
    art.pot = gs::uploadMipped(vdp, potArt());
    art.flame = gs::uploadMipped(vdp, flameArt());
    art.wick = gs::uploadMipped(vdp, wickArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.gull = gs::uploadMipped(vdp, gullArt());
}

}  // namespace cdawn

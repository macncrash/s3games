#include "art.h"

#include <string>

namespace wharfmaga {
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
    gs::Bitmap b(30, 42);
    b.ellipse(14, 6, 5, 5, 3);
    b.rect(4, 3, 14, 3, 4);
    b.rect(9, 11, 10, 13, 1);
    b.rect(11, 13, 6, 4, 2);
    b.rect(step ? 4.f : 7.f, 13, 4, 9, 5);
    b.rect(step ? 20.f : 17.f, 13, 5, 8, 5);
    b.rect(step ? 9.f : 12.f, 24, 4, 12, 1);
    b.rect(step ? 16.f : 13.f, 24, 4, 12, 6);
    b.rect(8, 35, 6, 3, 7);
    b.rect(15, 35, 6, 3, 7);
    b.rect(19, 16, 10, 2, 8);
    b.rect(27, 15, 2, 3, 8);
    b.outline(9, false);
    return b;
}

gs::Bitmap boarderArt(int step) {
    gs::Bitmap b(22, 34);
    b.ellipse(11, 5, 4, 4, 3);
    b.rect(7, 9, 8, 11, 1);
    b.rect(8, 11, 6, 3, 2);
    b.rect(step ? 2.f : 15.f, 10, 4, 8, 4);
    b.rect(step ? 7.f : 11.f, 20, 4, 9, 1);
    b.rect(step ? 12.f : 7.f, 20, 4, 9, 5);
    b.rect(5, 28, 5, 3, 6);
    b.rect(12, 28, 5, 3, 6);
    b.rect(14, 12, 6, 2, 7);
    b.outline(8, false);
    return b;
}

gs::Bitmap swingerArt(int step) {
    gs::Bitmap b(24, 34);
    b.ellipse(10, 5, 4, 4, 3);
    b.rect(6, 9, 8, 10, 1);
    b.ellipse(16, 13, 4, 4, 4);
    b.rect(15, 13, 2, 6, 5);
    b.rect(step ? 2.f : 8.f, 10, 4, 7, 2);
    b.rect(step ? 6.f : 10.f, 19, 4, 10, 1);
    b.rect(step ? 11.f : 6.f, 19, 4, 10, 6);
    b.rect(4, 28, 5, 3, 7);
    b.rect(11, 28, 5, 3, 7);
    b.outline(8, false);
    return b;
}

gs::Bitmap dinghyArt() {
    gs::Bitmap b(32, 14);
    b.poly({{1, 6}, {6, 12}, {26, 12}, {31, 6}, {24, 4}, {8, 4}}, 1);
    b.rect(13, 1, 5, 5, 2);
    b.rect(4, 5, 6, 2, 3);
    b.line(8, 4, 16, 1, 4, 1);
    b.outline(5, false);
    return b;
}

gs::Bitmap pileArt() {
    gs::Bitmap b(12, 36);
    b.rect(4, 2, 4, 28, 1);
    b.rect(2, 28, 8, 6, 2);
    b.rect(3, 8, 6, 2, 3);
    b.rect(3, 18, 6, 2, 3);
    b.ellipse(6, 3, 4, 2, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap crateArt() {
    gs::Bitmap b(18, 16);
    b.rect(1, 3, 16, 12, 1);
    b.line(1, 9, 17, 9, 2, 1);
    b.line(9, 3, 9, 15, 2, 1);
    b.rect(7, 1, 4, 3, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap gullArt(int step) {
    gs::Bitmap b(22, 10);
    if (step) {
        b.poly({{1, 6}, {8, 2}, {11, 5}, {14, 2}, {21, 6}, {14, 5}, {11, 7}, {8, 5}}, 1);
    } else {
        b.poly({{1, 3}, {8, 6}, {11, 5}, {14, 6}, {21, 3}, {14, 7}, {11, 6}, {8, 7}}, 1);
    }
    b.ellipse(11, 6, 2, 2, 2);
    return b;
}

gs::Bitmap brassArt() {
    gs::Bitmap b(6, 14);
    b.rect(1, 2, 4, 10, 1);
    b.rect(1, 1, 4, 2, 2);
    b.rect(2, 5, 2, 5, 3);
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
    const uint16_t hud[] = {0, gs::rgb4(13, 14, 12), gs::rgb4(6, 7, 6), gs::rgb4(15, 13, 6), gs::rgb4(2, 2, 3)};
    const uint16_t keep[] = {0, gs::rgb4(3, 5, 6), gs::rgb4(8, 9, 7), gs::rgb4(13, 11, 8), gs::rgb4(2, 3, 4),
                             gs::rgb4(4, 6, 5), gs::rgb4(2, 2, 3), gs::rgb4(1, 1, 2), gs::rgb4(10, 8, 4),
                             gs::rgb4(1, 1, 1)};
    const uint16_t board[] = {0, gs::rgb4(5, 3, 3), gs::rgb4(9, 6, 4), gs::rgb4(12, 9, 7), gs::rgb4(4, 3, 3),
                              gs::rgb4(6, 4, 3), gs::rgb4(2, 2, 2), gs::rgb4(11, 7, 3), gs::rgb4(1, 1, 1)};
    const uint16_t swing[] = {0, gs::rgb4(4, 5, 4), gs::rgb4(8, 8, 5), gs::rgb4(12, 11, 8), gs::rgb4(9, 7, 3),
                              gs::rgb4(6, 5, 2), gs::rgb4(3, 3, 3), gs::rgb4(2, 2, 2), gs::rgb4(1, 1, 1)};
    const uint16_t dinghy[] = {0, gs::rgb4(7, 5, 3), gs::rgb4(10, 8, 5), gs::rgb4(3, 7, 8), gs::rgb4(12, 11, 8),
                               gs::rgb4(1, 1, 2)};
    const uint16_t brass[] = {0, gs::rgb4(12, 9, 3), gs::rgb4(15, 14, 7), gs::rgb4(7, 5, 2)};
    const uint16_t fx[] = {0, gs::rgb4(15, 14, 9), gs::rgb4(15, 9, 3)};
    const uint16_t ok[] = {0, gs::rgb4(6, 14, 8), gs::rgb4(2, 7, 5)};
    const uint16_t alert[] = {0, gs::rgb4(15, 5, 3), gs::rgb4(8, 2, 2)};
    const uint16_t pile[] = {0, gs::rgb4(6, 5, 3), gs::rgb4(3, 3, 2), gs::rgb4(8, 7, 4), gs::rgb4(11, 10, 6),
                             gs::rgb4(1, 1, 1)};
    const uint16_t gull[] = {0, gs::rgb4(14, 14, 13), gs::rgb4(8, 8, 7)};
    const uint16_t crate[] = {0, gs::rgb4(8, 5, 2), gs::rgb4(5, 3, 1), gs::rgb4(11, 8, 3), gs::rgb4(2, 1, 1)};
    setPal(vdp, PAL_HUD, hud, 5);
    setPal(vdp, PAL_KEEP, keep, 10);
    setPal(vdp, PAL_BOARD, board, 9);
    setPal(vdp, PAL_SWING, swing, 9);
    setPal(vdp, PAL_DINGHY, dinghy, 6);
    setPal(vdp, PAL_BRASS, brass, 4);
    setPal(vdp, PAL_FX, fx, 3);
    setPal(vdp, PAL_OK, ok, 3);
    setPal(vdp, PAL_ALERT, alert, 3);
    setPal(vdp, PAL_PILE, pile, 6);
    setPal(vdp, PAL_GULL, gull, 3);
    setPal(vdp, PAL_CRATE, crate, 5);

    const uint16_t roadPal[] = {
        0,
        gs::rgb4(5, 4, 2),
        gs::rgb4(4, 3, 2),
        gs::rgb4(6, 5, 3),
        gs::rgb4(7, 6, 3),
        gs::rgb4(3, 3, 2),
        gs::rgb4(8, 7, 4),
        gs::rgb4(6, 5, 3),
        gs::rgb4(2, 2, 1),
        gs::rgb4(5, 4, 2),
        gs::rgb4(9, 8, 5),
        gs::rgb4(1, 3, 6),
        gs::rgb4(2, 5, 8),
        gs::rgb4(3, 7, 9),
        gs::rgb4(12, 11, 7),
        gs::rgb4(8, 7, 4),
    };
    setPal(vdp, 12, roadPal, 16);
    vdp.setFogColor(gs::rgb4(3, 5, 8));

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, tiles, art);
    art.keeper[0] = gs::uploadMipped(vdp, keeperArt(0));
    art.keeper[1] = gs::uploadMipped(vdp, keeperArt(1));
    art.boarder[0] = gs::uploadMipped(vdp, boarderArt(0));
    art.boarder[1] = gs::uploadMipped(vdp, boarderArt(1));
    art.swinger[0] = gs::uploadMipped(vdp, swingerArt(0));
    art.swinger[1] = gs::uploadMipped(vdp, swingerArt(1));
    art.dinghy = gs::uploadMipped(vdp, dinghyArt());
    art.pile = gs::uploadMipped(vdp, pileArt());
    art.crate = gs::uploadMipped(vdp, crateArt());
    art.gull[0] = gs::uploadMipped(vdp, gullArt(0));
    art.gull[1] = gs::uploadMipped(vdp, gullArt(1));
    art.brass = gs::uploadMipped(vdp, brassArt());
    art.flash = gs::uploadMipped(vdp, flashArt());
}

}  // namespace wharfmaga

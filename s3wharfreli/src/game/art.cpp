#include "art.h"

#include <string>

namespace wharf {
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

gs::Bitmap sentryArt(int step) {
    gs::Bitmap b(32, 46);
    b.ellipse(16, 8, 6, 6, 3);
    b.rect(11, 2, 10, 4, 4);          // sou'wester brim
    b.rect(13, 0, 6, 3, 4);
    b.rect(10, 14, 12, 14, 1);        // oilskin
    b.rect(12, 16, 8, 6, 2);
    b.rect(step ? 5.f : 9.f, 16, 4, 10, 5);
    b.rect(step ? 23.f : 19.f, 16, 4, 10, 5);
    b.rect(22, 20, 9, 3, 7);          // carbine
    b.rect(29, 19, 3, 2, 8);
    b.rect(step ? 11.f : 14.f, 28, 4, 12, 1);
    b.rect(step ? 17.f : 14.f, 28, 4, 12, 6);
    b.rect(9, 39, 7, 3, 9);
    b.rect(16, 39, 7, 3, 9);
    b.outline(10, false);
    return b;
}

gs::Bitmap boarderArt(int step) {
    gs::Bitmap b(24, 40);
    b.ellipse(12, 6, 5, 5, 3);
    b.rect(8, 4, 8, 3, 4);            // knit cap
    b.rect(8, 11, 8, 12, 1);
    b.rect(9, 13, 6, 5, 2);
    b.rect(float(step ? 3 : 14), 13, 4, 9, 5);
    b.rect(step ? 7.f : 13.f, 23, 4, 11, 1);
    b.rect(step ? 13.f : 7.f, 23, 4, 11, 6);
    b.rect(5, 33, 6, 3, 7);
    b.rect(13, 33, 6, 3, 7);
    b.rect(15, 15, 7, 2, 8);          // hook
    b.outline(9, false);
    return b;
}

gs::Bitmap skiffArt() {
    gs::Bitmap b(40, 18);
    b.poly({{1, 7}, {8, 16}, {32, 16}, {39, 7}, {30, 7}, {10, 7}}, 1);
    b.rect(16, 3, 8, 7, 2);
    b.rect(18, 4, 4, 4, 3);
    b.line(20, 2, 20, 10, 4, 1);
    b.rect(4, 6, 8, 2, 5);
    b.outline(6, false);
    return b;
}

gs::Bitmap tugArt() {
    gs::Bitmap b(48, 26);
    b.poly({{2, 12}, {8, 22}, {42, 22}, {46, 12}, {40, 12}, {8, 12}}, 1);
    b.rect(18, 4, 14, 10, 2);
    b.rect(20, 6, 6, 4, 3);
    b.rect(30, 2, 3, 8, 4);           // stack
    b.ellipse(32, 2, 3, 2, 5);
    b.rect(6, 10, 8, 3, 6);
    b.outline(7, false);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(22, 30);
    b.rect(10, 0, 3, 6, 3);
    b.ellipse(11, 16, 9, 9, 1);
    b.ellipse(11, 18, 4, 4, 2);
    b.rect(9, 24, 4, 4, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap pilingArt() {
    gs::Bitmap b(14, 32);
    b.rect(4, 0, 6, 26, 1);
    b.rect(2, 24, 10, 6, 2);
    b.rect(5, 6, 4, 3, 3);
    b.rect(5, 14, 4, 2, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap crateArt() {
    gs::Bitmap b(22, 18);
    b.rect(1, 2, 20, 14, 1);
    b.line(1, 9, 21, 9, 2, 1);
    b.line(11, 2, 11, 16, 2, 1);
    b.rect(8, 7, 6, 4, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap lanternArt() {
    gs::Bitmap b(12, 18);
    b.rect(5, 0, 2, 3, 3);
    b.rect(3, 3, 6, 10, 1);
    b.rect(4, 5, 4, 6, 2);
    b.rect(4, 13, 4, 3, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap flashArt() {
    gs::Bitmap b(12, 12);
    b.ellipse(6, 6, 5, 5, 1);
    b.ellipse(6, 6, 2, 2, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t hud[] = {0, gs::rgb4(14, 14, 12), gs::rgb4(8, 9, 8), gs::rgb4(15, 13, 6), gs::rgb4(15, 6, 3),
                            gs::rgb4(8, 14, 8), gs::rgb4(1, 2, 3)};
    const uint16_t you[] = {0, gs::rgb4(2, 5, 4), gs::rgb4(4, 8, 6), gs::rgb4(12, 9, 6), gs::rgb4(3, 4, 3),
                            gs::rgb4(1, 4, 3), gs::rgb4(5, 6, 4), gs::rgb4(6, 6, 5), gs::rgb4(14, 12, 5),
                            gs::rgb4(2, 2, 2), gs::rgb4(1, 1, 1)};
    const uint16_t foe[] = {0, gs::rgb4(5, 3, 3), gs::rgb4(8, 5, 4), gs::rgb4(11, 8, 6), gs::rgb4(3, 2, 2),
                            gs::rgb4(6, 4, 3), gs::rgb4(4, 3, 3), gs::rgb4(2, 2, 2), gs::rgb4(9, 8, 5), gs::rgb4(1, 1, 1)};
    const uint16_t tug[] = {0, gs::rgb4(3, 4, 5), gs::rgb4(6, 7, 6), gs::rgb4(10, 12, 12), gs::rgb4(2, 2, 2),
                            gs::rgb4(8, 8, 7), gs::rgb4(12, 5, 2), gs::rgb4(1, 1, 2)};
    const uint16_t skiff[] = {0, gs::rgb4(6, 4, 2), gs::rgb4(8, 6, 3), gs::rgb4(12, 11, 8), gs::rgb4(4, 3, 2),
                              gs::rgb4(10, 8, 4), gs::rgb4(1, 1, 1)};
    const uint16_t bell[] = {0, gs::rgb4(13, 11, 3), gs::rgb4(8, 6, 2), gs::rgb4(5, 4, 2), gs::rgb4(14, 13, 7),
                             gs::rgb4(3, 2, 1)};
    const uint16_t fx[] = {0, gs::rgb4(15, 14, 8), gs::rgb4(15, 9, 3)};
    const uint16_t ok[] = {0, gs::rgb4(8, 14, 7), gs::rgb4(4, 8, 4)};
    const uint16_t alert[] = {0, gs::rgb4(15, 5, 3), gs::rgb4(8, 2, 2)};
    const uint16_t wood[] = {0, gs::rgb4(7, 5, 2), gs::rgb4(5, 3, 1), gs::rgb4(10, 8, 3), gs::rgb4(2, 2, 1)};
    setPal(vdp, PAL_HUD, hud, 7);
    setPal(vdp, PAL_YOU, you, 11);
    setPal(vdp, PAL_FOE, foe, 10);
    setPal(vdp, PAL_TUG, tug, 8);
    setPal(vdp, PAL_SKIFF, skiff, 7);
    setPal(vdp, PAL_BELL, bell, 6);
    setPal(vdp, PAL_FX, fx, 3);
    setPal(vdp, PAL_OK, ok, 3);
    setPal(vdp, PAL_ALERT, alert, 3);
    setPal(vdp, PAL_WOOD, wood, 5);

    // Palette 12: timber deck, dark water either side.
    const uint16_t roadPal[] = {
        0,
        gs::rgb4(2, 4, 3),
        gs::rgb4(1, 3, 3),
        gs::rgb4(3, 5, 4),
        gs::rgb4(6, 4, 2),
        gs::rgb4(4, 3, 1),
        gs::rgb4(8, 6, 3),
        gs::rgb4(5, 4, 2),
        gs::rgb4(3, 2, 1),
        gs::rgb4(9, 7, 3),
        gs::rgb4(7, 5, 2),
        gs::rgb4(1, 2, 5),
        gs::rgb4(2, 4, 7),
        gs::rgb4(3, 6, 8),
        gs::rgb4(12, 10, 4),
        gs::rgb4(10, 8, 5),
    };
    setPal(vdp, 12, roadPal, 16);
    vdp.setFogColor(gs::rgb4(4, 5, 7));

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, tiles, art);
    art.sentry[0] = gs::uploadMipped(vdp, sentryArt(0));
    art.sentry[1] = gs::uploadMipped(vdp, sentryArt(1));
    art.boarder[0] = gs::uploadMipped(vdp, boarderArt(0));
    art.boarder[1] = gs::uploadMipped(vdp, boarderArt(1));
    art.skiff = gs::uploadMipped(vdp, skiffArt());
    art.tug = gs::uploadMipped(vdp, tugArt());
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.piling = gs::uploadMipped(vdp, pilingArt());
    art.crate = gs::uploadMipped(vdp, crateArt());
    art.lantern = gs::uploadMipped(vdp, lanternArt());
    art.flash = gs::uploadMipped(vdp, flashArt());
}

}  // namespace wharf

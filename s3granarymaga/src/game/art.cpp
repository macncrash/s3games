#include "art.h"

#include <string>

namespace grmaga {
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

gs::Bitmap watcherArt(int step) {
    gs::Bitmap b(30, 42);
    b.ellipse(12, 7, 5, 5, 3);
    b.rect(8, 12, 9, 12, 1);
    b.rect(9, 14, 6, 4, 2);
    b.rect(step ? 3.f : 6.f, 13, 5, 7, 4);
    b.rect(16, 16, 12, 3, 5);
    b.rect(step ? 8.f : 10.f, 24, 4, 12, 1);
    b.rect(step ? 14.f : 13.f, 24, 4, 12, 6);
    b.rect(7, 35, 6, 3, 7);
    b.rect(13, 35, 6, 3, 7);
    b.rect(4, 10, 7, 2, 8);
    b.outline(9, false);
    return b;
}

gs::Bitmap raiderArt(int step) {
    gs::Bitmap b(22, 36);
    b.ellipse(11, 5, 4, 4, 3);
    b.rect(7, 9, 8, 11, 1);
    b.rect(8, 11, 6, 3, 2);
    b.rect(step ? 2.f : 15.f, 10, 4, 8, 4);
    b.rect(step ? 6.f : 11.f, 20, 4, 11, 1);
    b.rect(step ? 12.f : 7.f, 20, 4, 11, 5);
    b.rect(5, 30, 5, 3, 6);
    b.rect(12, 30, 5, 3, 6);
    b.rect(14, 13, 6, 2, 7);
    b.outline(8, false);
    return b;
}

gs::Bitmap sackArt() {
    gs::Bitmap b(16, 18);
    b.ellipse(8, 10, 6, 6, 1);
    b.rect(6, 3, 4, 5, 2);
    b.line(5, 8, 11, 8, 3, 1);
    b.outline(4, false);
    return b;
}

gs::Bitmap siloArt() {
    gs::Bitmap b(36, 64);
    b.rect(6, 14, 24, 44, 1);
    b.ellipse(18, 14, 12, 8, 2);
    b.rect(15, 4, 6, 12, 3);
    b.rect(8, 28, 6, 8, 4);
    b.rect(22, 40, 6, 8, 4);
    b.rect(4, 56, 28, 6, 5);
    b.outline(6, false);
    return b;
}

gs::Bitmap doorArt() {
    gs::Bitmap b(28, 40);
    b.rect(2, 8, 24, 30, 1);
    b.poly({{2, 8}, {14, 1}, {26, 8}}, 2);
    b.rect(10, 16, 8, 16, 3);
    b.rect(16, 23, 2, 2, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap roundArt() {
    gs::Bitmap b(6, 14);
    b.rect(1, 3, 4, 9, 1);
    b.rect(1, 1, 4, 3, 2);
    b.rect(2, 5, 2, 5, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap flashArt() {
    gs::Bitmap b(14, 10);
    b.poly({{1, 5}, {6, 1}, {8, 4}, {13, 2}, {9, 5}, {12, 9}, {7, 6}, {4, 9}}, 1);
    b.outline(2, false);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t hud[] = {gs::rgb4(0, 0, 0), gs::rgb4(14, 13, 9), gs::rgb4(6, 5, 3)};
    const uint16_t watch[] = {0, gs::rgb4(3, 4, 6), gs::rgb4(12, 9, 6), gs::rgb4(13, 10, 7), gs::rgb4(4, 5, 3),
                              gs::rgb4(5, 5, 4), gs::rgb4(2, 2, 3), gs::rgb4(1, 1, 1), gs::rgb4(8, 7, 3), gs::rgb4(1, 1, 2)};
    const uint16_t raider[] = {0, gs::rgb4(5, 3, 2), gs::rgb4(10, 7, 5), gs::rgb4(8, 6, 4), gs::rgb4(6, 2, 2),
                               gs::rgb4(3, 2, 2), gs::rgb4(2, 2, 1), gs::rgb4(9, 8, 4), gs::rgb4(1, 1, 1)};
    const uint16_t sack[] = {0, gs::rgb4(11, 9, 4), gs::rgb4(8, 6, 2), gs::rgb4(6, 4, 2), gs::rgb4(3, 2, 1)};
    const uint16_t grain[] = {0, gs::rgb4(12, 9, 3), gs::rgb4(14, 12, 5), gs::rgb4(8, 6, 2), gs::rgb4(4, 3, 1)};
    const uint16_t flash[] = {0, gs::rgb4(15, 14, 8), gs::rgb4(12, 6, 2)};
    const uint16_t held[] = {0, gs::rgb4(8, 14, 6)};
    const uint16_t lost[] = {0, gs::rgb4(14, 5, 3)};
    const uint16_t timber[] = {0, gs::rgb4(9, 7, 4), gs::rgb4(12, 8, 4), gs::rgb4(4, 3, 2), gs::rgb4(14, 12, 6),
                               gs::rgb4(6, 4, 2), gs::rgb4(2, 2, 1)};
    setPal(vdp, PAL_HUD, hud, 3);
    setPal(vdp, PAL_WATCH, watch, 10);
    setPal(vdp, PAL_RAIDER, raider, 9);
    setPal(vdp, PAL_SACK, sack, 5);
    setPal(vdp, PAL_GRAIN, grain, 5);
    setPal(vdp, PAL_FLASH, flash, 3);
    setPal(vdp, PAL_HELD, held, 2);
    setPal(vdp, PAL_LOST, lost, 2);
    setPal(vdp, PAL_TIMBER, timber, 7);

    uint16_t yard[16] = {};
    yard[1] = gs::rgb4(10, 9, 3);
    yard[2] = gs::rgb4(7, 7, 2);
    yard[3] = gs::rgb4(12, 10, 4);
    yard[4] = gs::rgb4(5, 6, 2);
    yard[5] = gs::rgb4(4, 5, 1);
    yard[6] = gs::rgb4(8, 6, 3);
    yard[7] = gs::rgb4(6, 4, 2);
    yard[8] = gs::rgb4(5, 4, 2);
    yard[9] = gs::rgb4(4, 3, 2);
    yard[10] = gs::rgb4(9, 7, 3);
    yard[11] = gs::rgb4(3, 4, 6);
    yard[12] = gs::rgb4(2, 3, 5);
    yard[13] = gs::rgb4(5, 6, 8);
    yard[14] = gs::rgb4(13, 12, 8);
    yard[15] = gs::rgb4(9, 8, 5);
    setPal(vdp, 12, yard, 16);
    vdp.setFogColor(gs::rgb4(6, 5, 7));

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, tiles, art);
    art.watcher[0] = gs::uploadMipped(vdp, watcherArt(0));
    art.watcher[1] = gs::uploadMipped(vdp, watcherArt(1));
    art.raider[0] = gs::uploadMipped(vdp, raiderArt(0));
    art.raider[1] = gs::uploadMipped(vdp, raiderArt(1));
    art.sack = gs::uploadMipped(vdp, sackArt());
    art.silo = gs::uploadMipped(vdp, siloArt());
    art.door = gs::uploadMipped(vdp, doorArt());
    art.round = gs::uploadMipped(vdp, roundArt());
    art.flash = gs::uploadMipped(vdp, flashArt());
}

}  // namespace grmaga

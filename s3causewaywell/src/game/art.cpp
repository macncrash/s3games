#include "art.h"

#include <string>

namespace cwell {
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
    b.ellipse(15, 6, 5, 5, 3);
    b.rect(11, 11, 8, 4, 4);
    b.rect(10, 15, 10, 12, 1);
    b.rect(12, 17, 6, 5, 2);
    b.rect(step ? 5.f : 8.f, 16, 4, 10, 5);
    b.rect(21, 18, 7, 3, 6);
    b.rect(26, 16, 3, 3, 7);
    b.rect(step ? 11.f : 14.f, 27, 4, 10, 1);
    b.rect(step ? 16.f : 13.f, 27, 4, 10, 2);
    b.rect(9, 36, 6, 3, 8);
    b.rect(16, 36, 6, 3, 8);
    b.outline(9, false);
    return b;
}

gs::Bitmap raiderArt(int step) {
    gs::Bitmap b(26, 40);
    b.ellipse(12, 6, 5, 5, 3);
    b.rect(8, 11, 8, 12, 1);
    b.rect(9, 13, 6, 4, 2);
    b.rect(step ? 4.f : 16.f, 13, 4, 10, 4);
    b.rect(18, 12, 3, 16, 5);
    b.rect(step ? 8.f : 13.f, 23, 4, 12, 1);
    b.rect(step ? 13.f : 8.f, 23, 4, 12, 6);
    b.rect(6, 34, 6, 3, 7);
    b.rect(13, 34, 6, 3, 7);
    b.outline(8, false);
    return b;
}

gs::Bitmap haulerArt() {
    gs::Bitmap b(40, 36);
    b.rect(2, 16, 22, 10, 1);
    b.rect(4, 18, 8, 5, 2);
    b.ellipse(8, 28, 5, 5, 3);
    b.ellipse(20, 28, 5, 5, 3);
    b.ellipse(8, 28, 2, 2, 4);
    b.ellipse(20, 28, 2, 2, 4);
    b.ellipse(30, 8, 5, 5, 5);
    b.rect(26, 13, 8, 12, 6);
    b.rect(24, 16, 4, 8, 7);
    b.rect(28, 25, 4, 8, 6);
    b.outline(8, false);
    return b;
}

gs::Bitmap wellArt() {
    gs::Bitmap b(48, 52);
    b.rect(20, 2, 4, 10, 4);
    b.rect(14, 2, 16, 4, 4);
    b.line(22, 6, 22, 22, 5, 1);
    b.ellipse(24, 18, 6, 4, 6);
    b.ellipse(24, 28, 18, 8, 1);
    b.ellipse(24, 28, 12, 5, 2);
    b.ellipse(24, 28, 7, 3, 3);
    b.rect(8, 28, 6, 16, 1);
    b.rect(34, 28, 6, 16, 1);
    b.rect(12, 36, 24, 8, 7);
    b.rect(10, 42, 28, 6, 1);
    b.outline(8, false);
    return b;
}

gs::Bitmap crackArt() {
    gs::Bitmap b(16, 20);
    b.line(8, 0, 4, 8, 1, 1);
    b.line(4, 8, 11, 12, 1, 1);
    b.line(11, 12, 6, 19, 1, 1);
    return b;
}

gs::Bitmap shotArt() {
    gs::Bitmap b(8, 8);
    b.ellipse(4, 4, 3, 3, 1);
    b.ellipse(3, 3, 1, 1, 2);
    return b;
}

gs::Bitmap flashArt() {
    gs::Bitmap b(12, 12);
    b.ellipse(6, 6, 5, 5, 1);
    b.ellipse(6, 6, 2, 2, 2);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(12, 32);
    b.rect(4, 4, 4, 24, 1);
    b.rect(2, 26, 8, 4, 2);
    b.ellipse(6, 4, 3, 3, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap skiffArt() {
    gs::Bitmap b(28, 14);
    b.poly({{2, 6}, {6, 12}, {22, 12}, {26, 6}, {20, 6}, {8, 6}}, 1);
    b.rect(12, 1, 2, 8, 2);
    b.poly({{14, 2}, {22, 6}, {14, 8}}, 3);
    b.outline(4, false);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t hud[] = {0, gs::rgb4(14, 13, 10), gs::rgb4(8, 8, 6), gs::rgb4(15, 12, 5), gs::rgb4(15, 6, 3),
                            gs::rgb4(7, 13, 8), gs::rgb4(2, 2, 2)};
    const uint16_t you[] = {0, gs::rgb4(5, 6, 5), gs::rgb4(8, 9, 7), gs::rgb4(13, 11, 8), gs::rgb4(6, 5, 3),
                            gs::rgb4(4, 5, 4), gs::rgb4(9, 8, 5), gs::rgb4(14, 13, 6), gs::rgb4(3, 3, 2),
                            gs::rgb4(1, 1, 1)};
    const uint16_t foe[] = {0, gs::rgb4(6, 4, 3), gs::rgb4(9, 6, 4), gs::rgb4(12, 9, 6), gs::rgb4(4, 3, 3),
                            gs::rgb4(7, 6, 3), gs::rgb4(5, 4, 3), gs::rgb4(2, 2, 2), gs::rgb4(1, 1, 1)};
    const uint16_t haul[] = {0, gs::rgb4(6, 5, 3), gs::rgb4(9, 7, 4), gs::rgb4(3, 3, 2), gs::rgb4(8, 8, 7),
                             gs::rgb4(11, 8, 6), gs::rgb4(5, 4, 3), gs::rgb4(4, 3, 2), gs::rgb4(1, 1, 1)};
    const uint16_t well[] = {0, gs::rgb4(8, 8, 7), gs::rgb4(5, 5, 4), gs::rgb4(2, 5, 8), gs::rgb4(6, 4, 2),
                             gs::rgb4(4, 3, 2), gs::rgb4(9, 7, 4), gs::rgb4(7, 7, 6), gs::rgb4(1, 1, 1)};
    const uint16_t fx[] = {0, gs::rgb4(15, 14, 8), gs::rgb4(15, 15, 12), gs::rgb4(12, 8, 3)};
    const uint16_t ok[] = {0, gs::rgb4(8, 14, 7), gs::rgb4(4, 8, 4)};
    const uint16_t alert[] = {0, gs::rgb4(15, 5, 3), gs::rgb4(8, 2, 2)};
    const uint16_t post[] = {0, gs::rgb4(7, 7, 6), gs::rgb4(4, 4, 3), gs::rgb4(14, 12, 4), gs::rgb4(1, 1, 1)};
    const uint16_t skiff[] = {0, gs::rgb4(6, 5, 3), gs::rgb4(4, 3, 2), gs::rgb4(11, 12, 10), gs::rgb4(1, 1, 2)};
    setPal(vdp, PAL_HUD, hud, 7);
    setPal(vdp, PAL_YOU, you, 10);
    setPal(vdp, PAL_FOE, foe, 9);
    setPal(vdp, PAL_HAUL, haul, 9);
    setPal(vdp, PAL_WELL, well, 9);
    setPal(vdp, PAL_FX, fx, 4);
    setPal(vdp, PAL_OK, ok, 3);
    setPal(vdp, PAL_ALERT, alert, 3);
    setPal(vdp, PAL_POST, post, 5);
    setPal(vdp, PAL_SKIFF, skiff, 5);

    // Palette 12 is the road generator: stone causeway, water shoulders.
    const uint16_t roadPal[] = {
        0,
        gs::rgb4(3, 4, 4),
        gs::rgb4(2, 3, 4),
        gs::rgb4(4, 5, 5),
        gs::rgb4(7, 7, 6),
        gs::rgb4(5, 5, 4),
        gs::rgb4(9, 9, 8),
        gs::rgb4(6, 6, 5),
        gs::rgb4(4, 4, 3),
        gs::rgb4(8, 7, 5),
        gs::rgb4(11, 10, 8),
        gs::rgb4(1, 3, 6),
        gs::rgb4(2, 5, 8),
        gs::rgb4(3, 7, 10),
        gs::rgb4(12, 11, 6),
        gs::rgb4(10, 10, 9),
    };
    setPal(vdp, 12, roadPal, 16);
    vdp.setFogColor(gs::rgb4(4, 5, 8));

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, tiles, art);
    art.keeper[0] = gs::uploadMipped(vdp, keeperArt(0));
    art.keeper[1] = gs::uploadMipped(vdp, keeperArt(1));
    art.raider[0] = gs::uploadMipped(vdp, raiderArt(0));
    art.raider[1] = gs::uploadMipped(vdp, raiderArt(1));
    art.hauler = gs::uploadMipped(vdp, haulerArt());
    art.well = gs::uploadMipped(vdp, wellArt());
    art.crack = gs::uploadMipped(vdp, crackArt());
    art.shot = gs::uploadMipped(vdp, shotArt());
    art.flash = gs::uploadMipped(vdp, flashArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.skiff = gs::uploadMipped(vdp, skiffArt());
}

}  // namespace cwell

#include "art.h"

#include <string>

namespace well {
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
    gs::Bitmap b(26, 40);
    b.ellipse(13, 6, 5, 5, 3);
    b.rect(9, 3, 8, 3, 4);
    b.rect(8, 11, 10, 12, 1);
    b.rect(10, 13, 6, 4, 2);
    b.rect(step ? 3.f : 6.f, 12, 5, 3, 5);
    b.rect(17, 13, 8, 3, 6);
    b.rect(23, 12, 2, 2, 7);
    b.rect(step ? 9.f : 12.f, 23, 4, 12, 1);
    b.rect(step ? 15.f : 12.f, 23, 4, 12, 2);
    b.rect(7, 34, 5, 3, 8);
    b.rect(14, 34, 5, 3, 8);
    b.outline(9, false);
    return b;
}

gs::Bitmap raiderArt(int step) {
    gs::Bitmap b(22, 32);
    b.ellipse(11, 5, 4, 4, 3);
    b.rect(7, 9, 8, 10, 1);
    b.rect(step ? 2.f : 14.f, 10, 4, 8, 4);
    b.rect(15, 12, 6, 2, 5);
    b.rect(step ? 6.f : 10.f, 19, 3, 9, 1);
    b.rect(step ? 12.f : 8.f, 19, 3, 9, 2);
    b.rect(5, 27, 4, 3, 6);
    b.rect(12, 27, 4, 3, 6);
    b.outline(6, false);
    return b;
}

gs::Bitmap rammerArt(int step) {
    gs::Bitmap b(34, 32);
    b.ellipse(18, 6, 4, 4, 3);
    b.rect(14, 10, 8, 10, 1);
    b.rect(4, 13, 12, 4, 4);
    b.rect(0, 12, 5, 6, 5);
    b.rect(step ? 14.f : 18.f, 20, 4, 8, 2);
    b.rect(step ? 20.f : 16.f, 20, 4, 8, 1);
    b.rect(13, 27, 4, 3, 6);
    b.rect(19, 27, 4, 3, 6);
    b.outline(6, false);
    return b;
}

gs::Bitmap wellArt() {
    gs::Bitmap b(48, 56);
    b.rect(8, 18, 6, 34, 1);
    b.rect(34, 18, 6, 34, 1);
    b.rect(10, 46, 28, 8, 2);
    b.ellipse(24, 22, 18, 8, 3);
    b.ellipse(24, 22, 12, 5, 4);
    b.ellipse(24, 23, 6, 3, 5);
    b.rect(22, 4, 4, 14, 6);
    b.rect(16, 2, 16, 3, 6);
    b.rect(14, 30, 4, 8, 2);
    b.rect(30, 32, 4, 6, 1);
    b.outline(7, false);
    return b;
}

gs::Bitmap pierArt() {
    gs::Bitmap b(36, 80);
    b.rect(4, 8, 8, 68, 1);
    b.rect(24, 8, 8, 68, 1);
    b.rect(6, 0, 24, 12, 2);
    b.ellipse(18, 36, 10, 16, 3);
    b.rect(2, 70, 10, 8, 4);
    b.rect(24, 70, 10, 8, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap boltArt() {
    gs::Bitmap b(10, 4);
    b.rect(0, 1, 8, 2, 1);
    b.rect(7, 0, 3, 4, 2);
    return b;
}

gs::Bitmap crackArt() {
    gs::Bitmap b(16, 20);
    b.line(8, 0, 4, 8, 1, 1.5f);
    b.line(4, 8, 11, 12, 1, 1.5f);
    b.line(11, 12, 6, 19, 1, 1.5f);
    b.line(4, 8, 1, 14, 2, 1.2f);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t hud[] = {0, gs::rgb4(14, 13, 9), gs::rgb4(8, 8, 7), gs::rgb4(15, 12, 4), gs::rgb4(6, 10, 14)};
    const uint16_t you[] = {0, gs::rgb4(2, 4, 8), gs::rgb4(4, 6, 11), gs::rgb4(12, 9, 6), gs::rgb4(3, 2, 2),
                            gs::rgb4(6, 7, 8), gs::rgb4(9, 8, 6), gs::rgb4(14, 12, 5), gs::rgb4(2, 2, 2), gs::rgb4(1, 1, 2)};
    const uint16_t foe[] = {0, gs::rgb4(7, 3, 2), gs::rgb4(9, 5, 3), gs::rgb4(12, 9, 6), gs::rgb4(5, 3, 2),
                            gs::rgb4(8, 7, 4), gs::rgb4(3, 2, 2), gs::rgb4(1, 1, 1)};
    const uint16_t ram[] = {0, gs::rgb4(5, 3, 2), gs::rgb4(8, 4, 3), gs::rgb4(11, 8, 5), gs::rgb4(6, 4, 2),
                            gs::rgb4(9, 6, 3), gs::rgb4(3, 2, 1), gs::rgb4(1, 1, 1)};
    const uint16_t wel[] = {0, gs::rgb4(7, 7, 7), gs::rgb4(9, 9, 8), gs::rgb4(5, 5, 5), gs::rgb4(2, 5, 8),
                            gs::rgb4(1, 3, 6), gs::rgb4(6, 5, 3), gs::rgb4(3, 3, 3)};
    const uint16_t stone[] = {0, gs::rgb4(6, 6, 6), gs::rgb4(8, 8, 7), gs::rgb4(3, 4, 6), gs::rgb4(4, 4, 4),
                              gs::rgb4(2, 2, 2)};
    const uint16_t shot[] = {0, gs::rgb4(15, 14, 8), gs::rgb4(15, 10, 3)};
    const uint16_t ok[] = {0, gs::rgb4(8, 14, 7), gs::rgb4(3, 8, 4)};
    const uint16_t bad[] = {0, gs::rgb4(15, 6, 3), gs::rgb4(10, 8, 6)};
    setPal(vdp, PAL_HUD, hud, 5);
    setPal(vdp, PAL_YOU, you, 10);
    setPal(vdp, PAL_FOE, foe, 8);
    setPal(vdp, PAL_RAM, ram, 8);
    setPal(vdp, PAL_WELL, wel, 8);
    setPal(vdp, PAL_STONE, stone, 6);
    setPal(vdp, PAL_SHOT, shot, 3);
    setPal(vdp, PAL_OK, ok, 3);
    setPal(vdp, PAL_BAD, bad, 3);
    vdp.setFogColor(gs::rgb4(3, 3, 5));

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, tiles, art);
    art.sentry[0] = gs::uploadMipped(vdp, sentryArt(0));
    art.sentry[1] = gs::uploadMipped(vdp, sentryArt(1));
    art.raider[0] = gs::uploadMipped(vdp, raiderArt(0));
    art.raider[1] = gs::uploadMipped(vdp, raiderArt(1));
    art.rammer[0] = gs::uploadMipped(vdp, rammerArt(0));
    art.rammer[1] = gs::uploadMipped(vdp, rammerArt(1));
    art.well = gs::uploadMipped(vdp, wellArt());
    art.pier = gs::uploadMipped(vdp, pierArt());
    art.bolt = gs::uploadMipped(vdp, boltArt());
    art.crack = gs::uploadMipped(vdp, crackArt());
}

}  // namespace well

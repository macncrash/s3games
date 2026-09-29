#include "art.h"

#include <string>

namespace cwmaga {
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
    gs::Bitmap b(28, 40);
    b.ellipse(14, 6, 5, 5, 3);
    b.rect(10, 11, 8, 12, 1);
    b.rect(11, 13, 6, 5, 2);
    b.rect(step ? 5.f : 8.f, 12, 4, 8, 4);
    b.rect(step ? 19.f : 16.f, 12, 4, 8, 4);
    b.rect(step ? 9.f : 12.f, 23, 4, 11, 1);
    b.rect(step ? 15.f : 12.f, 23, 4, 11, 5);
    b.rect(8, 33, 6, 3, 6);
    b.rect(14, 33, 6, 3, 6);
    b.rect(18, 15, 9, 2, 7);
    b.outline(8, false);
    return b;
}

gs::Bitmap runnerArt(int step) {
    gs::Bitmap b(22, 34);
    b.ellipse(11, 5, 4, 4, 3);
    b.rect(7, 9, 8, 10, 1);
    b.rect(8, 11, 6, 4, 2);
    b.rect(step ? 3.f : 14.f, 10, 4, 8, 4);
    b.rect(step ? 7.f : 11.f, 19, 4, 10, 1);
    b.rect(step ? 12.f : 7.f, 19, 4, 10, 5);
    b.rect(5, 28, 5, 3, 6);
    b.rect(12, 28, 5, 3, 6);
    b.rect(13, 12, 7, 2, 7);
    b.outline(8, false);
    return b;
}

gs::Bitmap skiffArt() {
    gs::Bitmap b(30, 14);
    b.poly({{2, 5}, {7, 12}, {24, 12}, {28, 5}, {22, 5}, {8, 5}}, 1);
    b.rect(12, 1, 6, 5, 2);
    b.rect(4, 4, 5, 2, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(10, 28);
    b.rect(4, 6, 2, 18, 1);
    b.rect(2, 22, 6, 4, 2);
    b.ellipse(5, 4, 4, 3, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap brassArt() {
    gs::Bitmap b(6, 14);
    b.rect(1, 2, 4, 10, 1);
    b.rect(1, 1, 4, 2, 2);
    b.rect(2, 4, 2, 6, 3);
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
    const uint16_t hud[] = {0, gs::rgb4(14, 13, 10), gs::rgb4(7, 7, 6), gs::rgb4(15, 12, 5), gs::rgb4(2, 2, 2)};
    const uint16_t you[] = {0, gs::rgb4(4, 5, 4), gs::rgb4(8, 8, 6), gs::rgb4(12, 10, 7), gs::rgb4(5, 6, 4),
                            gs::rgb4(3, 3, 3), gs::rgb4(2, 2, 2), gs::rgb4(9, 8, 4), gs::rgb4(1, 1, 1)};
    const uint16_t foe[] = {0, gs::rgb4(6, 4, 3), gs::rgb4(9, 6, 4), gs::rgb4(12, 9, 7), gs::rgb4(5, 4, 3),
                            gs::rgb4(7, 5, 4), gs::rgb4(2, 2, 2), gs::rgb4(10, 8, 5), gs::rgb4(1, 1, 1)};
    const uint16_t skiff[] = {0, gs::rgb4(6, 5, 3), gs::rgb4(8, 7, 4), gs::rgb4(3, 6, 7), gs::rgb4(1, 1, 2)};
    const uint16_t brass[] = {0, gs::rgb4(12, 9, 3), gs::rgb4(15, 13, 6), gs::rgb4(7, 5, 2)};
    const uint16_t fx[] = {0, gs::rgb4(15, 14, 8), gs::rgb4(15, 8, 2)};
    const uint16_t ok[] = {0, gs::rgb4(7, 14, 7), gs::rgb4(3, 8, 4)};
    const uint16_t alert[] = {0, gs::rgb4(15, 5, 3), gs::rgb4(8, 2, 2)};
    const uint16_t stone[] = {0, gs::rgb4(7, 7, 6), gs::rgb4(4, 4, 4), gs::rgb4(12, 11, 6), gs::rgb4(2, 2, 2)};
    setPal(vdp, PAL_HUD, hud, 5);
    setPal(vdp, PAL_YOU, you, 9);
    setPal(vdp, PAL_FOE, foe, 9);
    setPal(vdp, PAL_SKIFF, skiff, 5);
    setPal(vdp, PAL_BRASS, brass, 4);
    setPal(vdp, PAL_FX, fx, 3);
    setPal(vdp, PAL_OK, ok, 3);
    setPal(vdp, PAL_ALERT, alert, 3);
    setPal(vdp, PAL_STONE, stone, 5);

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
    vdp.setFogColor(gs::rgb4(4, 5, 8));

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, tiles, art);
    art.gunner[0] = gs::uploadMipped(vdp, gunnerArt(0));
    art.gunner[1] = gs::uploadMipped(vdp, gunnerArt(1));
    art.runner[0] = gs::uploadMipped(vdp, runnerArt(0));
    art.runner[1] = gs::uploadMipped(vdp, runnerArt(1));
    art.skiff = gs::uploadMipped(vdp, skiffArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.brass = gs::uploadMipped(vdp, brassArt());
    art.flash = gs::uploadMipped(vdp, flashArt());
}

}  // namespace cwmaga

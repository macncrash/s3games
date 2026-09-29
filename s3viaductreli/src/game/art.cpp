#include "art.h"

#include <string>

namespace viaduct {
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
    gs::Bitmap b(28, 42);
    b.ellipse(14, 6, 5, 5, 3);
    b.rect(10, 4, 8, 3, 4);
    b.rect(9, 11, 10, 12, 1);
    b.rect(11, 13, 6, 5, 2);
    b.rect(step ? 5.f : 8.f, 13, 4, 10, 5);
    b.rect(18, 14, 9, 3, 6);
    b.rect(25, 13, 2, 2, 7);
    b.rect(step ? 10.f : 13.f, 23, 4, 12, 1);
    b.rect(step ? 16.f : 13.f, 23, 4, 12, 2);
    b.rect(8, 34, 6, 3, 8);
    b.rect(15, 34, 6, 3, 8);
    b.outline(9, false);
    return b;
}

gs::Bitmap climberArt(int step) {
    gs::Bitmap b(18, 30);
    b.ellipse(9, 5, 4, 4, 3);
    b.rect(6, 9, 7, 9, 1);
    b.rect(step ? 2.f : 11.f, 10, 3, 8, 4);
    b.rect(step ? 6.f : 10.f, 18, 3, 8, 1);
    b.rect(step ? 10.f : 6.f, 18, 3, 8, 2);
    b.rect(5, 25, 4, 2, 5);
    b.rect(10, 25, 4, 2, 5);
    b.outline(6, false);
    return b;
}

gs::Bitmap archArt() {
    gs::Bitmap b(64, 96);
    b.rect(4, 18, 10, 74, 1);
    b.rect(50, 18, 10, 74, 1);
    b.rect(8, 18, 48, 14, 2);
    b.ellipse(32, 52, 20, 26, 3);
    b.rect(0, 8, 64, 12, 2);
    b.rect(0, 6, 64, 4, 4);
    b.rect(6, 86, 6, 8, 5);
    b.rect(52, 86, 6, 8, 5);
    b.rect(14, 22, 4, 6, 4);
    b.rect(46, 22, 4, 6, 4);
    b.outline(6, false);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(22, 26);
    b.rect(9, 0, 4, 4, 3);
    b.ellipse(11, 13, 9, 8, 1);
    b.ellipse(11, 15, 4, 4, 2);
    b.rect(9, 20, 4, 4, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap sparkArt() {
    gs::Bitmap b(14, 16);
    b.ellipse(7, 10, 5, 4, 1);
    b.rect(6, 2, 2, 8, 2);
    b.rect(4, 4, 2, 3, 3);
    b.rect(8, 5, 2, 2, 3);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(10, 18);
    b.rect(4, 0, 2, 8, 2);
    b.ellipse(5, 12, 4, 4, 1);
    b.outline(3, false);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t hud[] = {0, gs::rgb4(14, 13, 10), gs::rgb4(8, 7, 6), gs::rgb4(15, 12, 5), gs::rgb4(15, 6, 3),
                            gs::rgb4(8, 13, 7)};
    const uint16_t you[] = {0, gs::rgb4(3, 4, 6), gs::rgb4(5, 6, 8), gs::rgb4(12, 9, 6), gs::rgb4(4, 3, 2),
                            gs::rgb4(2, 3, 4), gs::rgb4(6, 6, 5), gs::rgb4(14, 12, 4), gs::rgb4(2, 2, 2), gs::rgb4(1, 1, 1)};
    const uint16_t foe[] = {0, gs::rgb4(6, 3, 2), gs::rgb4(8, 5, 3), gs::rgb4(11, 8, 5), gs::rgb4(4, 2, 2),
                            gs::rgb4(2, 2, 2), gs::rgb4(1, 1, 1)};
    const uint16_t fuse[] = {0, gs::rgb4(12, 4, 1), gs::rgb4(15, 12, 3), gs::rgb4(15, 8, 2)};
    const uint16_t bell[] = {0, gs::rgb4(13, 11, 3), gs::rgb4(8, 6, 2), gs::rgb4(15, 13, 7), gs::rgb4(5, 4, 2),
                             gs::rgb4(2, 2, 1)};
    const uint16_t stone[] = {0, gs::rgb4(6, 6, 6), gs::rgb4(8, 8, 7), gs::rgb4(2, 3, 4), gs::rgb4(10, 9, 7),
                              gs::rgb4(4, 4, 4), gs::rgb4(1, 1, 2)};
    const uint16_t ok[] = {0, gs::rgb4(8, 14, 7), gs::rgb4(4, 8, 4)};
    const uint16_t alert[] = {0, gs::rgb4(15, 5, 3), gs::rgb4(8, 2, 2)};
    const uint16_t gorge[] = {0, gs::rgb4(2, 4, 6), gs::rgb4(3, 6, 7), gs::rgb4(1, 2, 3)};
    setPal(vdp, PAL_HUD, hud, 6);
    setPal(vdp, PAL_YOU, you, 10);
    setPal(vdp, PAL_FOE, foe, 7);
    setPal(vdp, PAL_FUSE, fuse, 4);
    setPal(vdp, PAL_BELL, bell, 6);
    setPal(vdp, PAL_STONE, stone, 7);
    setPal(vdp, PAL_OK, ok, 3);
    setPal(vdp, PAL_ALERT, alert, 3);
    setPal(vdp, PAL_GORGE, gorge, 4);
    vdp.setFogColor(gs::rgb4(4, 3, 5));

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, tiles, art);
    art.sentry[0] = gs::uploadMipped(vdp, sentryArt(0));
    art.sentry[1] = gs::uploadMipped(vdp, sentryArt(1));
    art.climber[0] = gs::uploadMipped(vdp, climberArt(0));
    art.climber[1] = gs::uploadMipped(vdp, climberArt(1));
    art.arch = gs::uploadMipped(vdp, archArt());
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.spark = gs::uploadMipped(vdp, sparkArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
}

}  // namespace viaduct

#include "art.h"

#include <cmath>
#include <cstdint>
#include <string>

namespace cistern {
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

gs::Bitmap figure(int step, int coat, bool helm, bool shield) {
    gs::Bitmap b(26, 36);
    b.ellipse(13, 6, 5, 5, helm ? 4 : 3);
    b.rect(10, 10, 6, 3, 5);
    b.rect(7, 13, 12, 11, coat);
    if (shield) b.rect(8, 14, 8, 8, 6);
    b.rect(step ? 5.f : 8.f, 14, 3, 8, 7);
    b.rect(step ? 18.f : 15.f, 14, 3, 8, 7);
    b.rect(step ? 8.f : 11.f, 24, 3, 8, 2);
    b.rect(step ? 15.f : 12.f, 24, 3, 8, 1);
    b.rect(7, 31, 4, 3, 8);
    b.rect(15, 31, 4, 3, 8);
    b.outline(9, false);
    return b;
}

gs::Bitmap basinArt() {
    gs::Bitmap b(120, 78);
    b.ellipse(60, 40, 56, 34, 1);
    b.ellipse(60, 42, 40, 22, 2);
    b.ellipse(52, 36, 10, 5, 3);
    for (int i = 0; i < 5; i++) b.ellipse(30.f + i * 14.f, 50, 3, 1, 4);
    return b;
}

gs::Bitmap ringArt() {
    gs::Bitmap b(168, 112);
    b.ellipse(84, 56, 80, 50, 1);
    b.ellipse(84, 56, 58, 34, 0);
    for (int i = 0; i < 8; i++) {
        float a = i * 0.785f;
        int x = int(84 + std::cos(a) * 68);
        int y = int(56 + std::sin(a) * 42);
        b.rect(float(x - 4), float(y - 3), 8, 6, (i & 1) ? 2 : 3);
    }
    b.rect(78, 8, 12, 6, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(24, 26);
    b.rect(10, 0, 4, 5, 3);
    b.ellipse(12, 14, 10, 8, 1);
    b.ellipse(12, 16, 5, 4, 2);
    b.rect(10, 21, 4, 3, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap ropeArt() {
    gs::Bitmap b(6, 40);
    b.rect(2, 0, 2, 36, 1);
    b.ellipse(3, 36, 2, 3, 2);
    return b;
}

gs::Bitmap bucketArt() {
    gs::Bitmap b(16, 14);
    b.rect(2, 2, 12, 10, 1);
    b.rect(3, 3, 10, 3, 2);
    b.line(2, 2, 8, 0, 3, 1);
    b.line(14, 2, 8, 0, 3, 1);
    b.outline(4, false);
    return b;
}

gs::Bitmap flashArt() {
    gs::Bitmap b(14, 14);
    b.ellipse(7, 7, 6, 6, 1);
    b.ellipse(7, 7, 3, 3, 2);
    return b;
}

gs::Bitmap reedArt() {
    gs::Bitmap b(10, 22);
    b.line(3, 20, 2, 2, 1, 1);
    b.line(6, 20, 8, 4, 2, 1);
    b.ellipse(2, 3, 2, 2, 3);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    vdp.setFogColor(gs::rgb4(1, 2, 3));
    const uint16_t hud[] = {0, gs::rgb4(14, 13, 9), gs::rgb4(6, 6, 5)};
    const uint16_t stone[] = {0, gs::rgb4(6, 6, 5), gs::rgb4(8, 8, 6), gs::rgb4(4, 4, 3), gs::rgb4(10, 9, 6),
                              gs::rgb4(3, 3, 2)};
    const uint16_t water[] = {0, gs::rgb4(2, 5, 7), gs::rgb4(3, 8, 10), gs::rgb4(6, 11, 12), gs::rgb4(1, 3, 5)};
    const uint16_t you[] = {0, gs::rgb4(4, 5, 3), gs::rgb4(7, 8, 4), gs::rgb4(12, 9, 5), gs::rgb4(14, 12, 6),
                            gs::rgb4(9, 7, 4), gs::rgb4(3, 3, 2), gs::rgb4(8, 8, 7), gs::rgb4(2, 2, 1), gs::rgb4(1, 1, 1)};
    const uint16_t foe[] = {0, gs::rgb4(5, 2, 2), gs::rgb4(8, 3, 2), gs::rgb4(12, 8, 5), gs::rgb4(3, 2, 2),
                            gs::rgb4(9, 6, 4), gs::rgb4(4, 4, 4), gs::rgb4(7, 5, 4), gs::rgb4(2, 1, 1), gs::rgb4(1, 1, 0)};
    const uint16_t plate[] = {0, gs::rgb4(4, 4, 5), gs::rgb4(7, 7, 8), gs::rgb4(11, 10, 8), gs::rgb4(3, 3, 4),
                              gs::rgb4(8, 7, 5), gs::rgb4(12, 12, 13), gs::rgb4(6, 6, 7), gs::rgb4(2, 2, 2), gs::rgb4(1, 1, 1)};
    const uint16_t fx[] = {0, gs::rgb4(15, 14, 8), gs::rgb4(15, 10, 3)};
    const uint16_t bell[] = {0, gs::rgb4(13, 10, 3), gs::rgb4(8, 6, 2), gs::rgb4(5, 4, 2), gs::rgb4(15, 13, 6),
                             gs::rgb4(3, 2, 1)};
    const uint16_t ok[] = {0, gs::rgb4(8, 14, 7), gs::rgb4(3, 6, 3)};
    const uint16_t alert[] = {0, gs::rgb4(15, 5, 4), gs::rgb4(8, 2, 2)};
    setPal(vdp, PAL_HUD, hud, 3);
    setPal(vdp, PAL_STONE, stone, 6);
    setPal(vdp, PAL_WATER, water, 5);
    setPal(vdp, PAL_YOU, you, 10);
    setPal(vdp, PAL_FOE, foe, 10);
    setPal(vdp, PAL_PLATE, plate, 10);
    setPal(vdp, PAL_FX, fx, 3);
    setPal(vdp, PAL_BELL, bell, 6);
    setPal(vdp, PAL_OK, ok, 3);
    setPal(vdp, PAL_ALERT, alert, 3);

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    art.basin = gs::uploadMipped(vdp, basinArt());
    art.ring = gs::uploadMipped(vdp, ringArt());
    art.sentry[0] = gs::uploadMipped(vdp, figure(0, 1, true, false));
    art.sentry[1] = gs::uploadMipped(vdp, figure(1, 1, true, false));
    art.raider[0] = gs::uploadMipped(vdp, figure(0, 1, false, false));
    art.raider[1] = gs::uploadMipped(vdp, figure(1, 1, false, false));
    art.runner[0] = gs::uploadMipped(vdp, figure(0, 2, false, false));
    art.runner[1] = gs::uploadMipped(vdp, figure(1, 2, false, false));
    art.plate[0] = gs::uploadMipped(vdp, figure(0, 1, true, true));
    art.plate[1] = gs::uploadMipped(vdp, figure(1, 1, true, true));
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.rope = gs::uploadMipped(vdp, ropeArt());
    art.bucket = gs::uploadMipped(vdp, bucketArt());
    art.flash = gs::uploadMipped(vdp, flashArt());
    art.reed = gs::uploadMipped(vdp, reedArt());
}

}  // namespace cistern

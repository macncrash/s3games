#include "art.h"

#include <cmath>
#include <cstdint>
#include <string>

namespace purse {
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

// Tracked cart. Indices: 1 body, 2 trim, 3 lamp, 4 wheel, 5 cab, 6 stack.
gs::Bitmap cartArt(int step, bool tall, bool narrow) {
    gs::Bitmap b(narrow ? 22 : 30, tall ? 22 : 16);
    float bodyW = narrow ? 14.f : 20.f;
    float x0 = (b.w - bodyW) * 0.5f;
    b.rect(x0, tall ? 6.f : 4.f, bodyW, tall ? 10.f : 8.f, 1);
    b.rect(x0 + 2.f, tall ? 4.f : 2.f, bodyW * 0.45f, 4, 5);
    b.rect(x0 + bodyW - 5.f, tall ? 7.f : 5.f, 3, 3, 3);
    b.rect(x0 + 1.f, 1, 3, tall ? 5.f : 3.f, 6);
    float wy = tall ? 15.f : 11.f;
    float shift = step ? 2.f : 0.f;
    b.rect(x0 - 1.f + shift, wy, 5, 4, 4);
    b.rect(x0 + bodyW - 5.f - shift, wy, 5, 4, 4);
    b.rect(x0 + 1.f, tall ? 8.f : 6.f, bodyW - 2.f, 2, 2);
    b.outline(2, false);
    return b;
}

gs::Bitmap wreckArt() {
    gs::Bitmap b(28, 14);
    b.rect(3, 5, 18, 6, 1);
    b.rect(16, 3, 8, 5, 2);
    b.rect(2, 10, 6, 3, 4);
    b.rect(18, 9, 5, 3, 4);
    b.ellipse(10, 4, 3, 2, 6);
    b.outline(2, false);
    return b;
}

gs::Bitmap sparkArt() {
    gs::Bitmap b(12, 12);
    b.ellipse(6, 6, 5, 5, 1);
    b.ellipse(6, 6, 2, 2, 2);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(8, 10);
    b.rect(3, 6, 2, 4, 2);
    b.ellipse(4, 4, 3, 3, 1);
    return b;
}

gs::Bitmap basinArt() {
    gs::Bitmap b(140, 86);
    b.ellipse(70, 44, 64, 38, 1);
    b.ellipse(70, 46, 46, 24, 2);
    b.ellipse(58, 38, 12, 6, 3);
    for (int i = 0; i < 6; i++) b.ellipse(28.f + i * 14.f, 58, 2, 1, 4);
    return b;
}

gs::Bitmap ringArt() {
    gs::Bitmap b(196, 120);
    b.ellipse(98, 60, 94, 56, 1);
    b.ellipse(98, 60, 70, 38, 0);
    for (int i = 0; i < 12; i++) {
        float a = i * 0.5236f;
        int x = int(98 + std::cos(a) * 82);
        int y = int(60 + std::sin(a) * 48);
        b.rect(float(x - 3), float(y - 2), 6, 4, (i % 3) ? 2 : 3);
    }
    b.outline(4, false);
    return b;
}

gs::Bitmap lipArt() {
    gs::Bitmap b(20, 8);
    b.rect(0, 2, 20, 4, 1);
    b.rect(2, 3, 6, 2, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    vdp.setFogColor(gs::rgb4(1, 2, 3));
    const uint16_t hud[] = {0, gs::rgb4(14, 13, 10), gs::rgb4(5, 5, 4)};
    const uint16_t stone[] = {0, gs::rgb4(7, 6, 5), gs::rgb4(9, 8, 6), gs::rgb4(4, 4, 3), gs::rgb4(11, 10, 7),
                              gs::rgb4(3, 3, 2)};
    const uint16_t water[] = {0, gs::rgb4(2, 4, 6), gs::rgb4(3, 7, 9), gs::rgb4(6, 11, 12), gs::rgb4(1, 3, 4)};
    const uint16_t you[] = {0, gs::rgb4(3, 8, 4), gs::rgb4(10, 12, 6), gs::rgb4(15, 14, 6), gs::rgb4(2, 2, 2),
                            gs::rgb4(6, 10, 8), gs::rgb4(8, 6, 3)};
    const uint16_t rival[] = {0, gs::rgb4(9, 3, 2), gs::rgb4(12, 8, 4), gs::rgb4(15, 12, 5), gs::rgb4(2, 1, 1),
                              gs::rgb4(6, 4, 3), gs::rgb4(5, 3, 2)};
    const uint16_t heavy[] = {0, gs::rgb4(4, 4, 6), gs::rgb4(8, 8, 9), gs::rgb4(13, 12, 8), gs::rgb4(1, 1, 2),
                              gs::rgb4(6, 6, 7), gs::rgb4(3, 3, 4)};
    const uint16_t skit[] = {0, gs::rgb4(10, 6, 2), gs::rgb4(14, 10, 3), gs::rgb4(15, 14, 7), gs::rgb4(3, 2, 1),
                             gs::rgb4(8, 5, 2), gs::rgb4(6, 3, 1)};
    const uint16_t fx[] = {0, gs::rgb4(15, 14, 8), gs::rgb4(15, 8, 2)};
    const uint16_t ok[] = {0, gs::rgb4(8, 14, 7), gs::rgb4(3, 6, 3)};
    const uint16_t alert[] = {0, gs::rgb4(15, 5, 4), gs::rgb4(8, 2, 2)};
    setPal(vdp, PAL_HUD, hud, 3);
    setPal(vdp, PAL_STONE, stone, 6);
    setPal(vdp, PAL_WATER, water, 5);
    setPal(vdp, PAL_YOU, you, 7);
    setPal(vdp, PAL_RIVAL, rival, 7);
    setPal(vdp, PAL_HEAVY, heavy, 7);
    setPal(vdp, PAL_SKIT, skit, 7);
    setPal(vdp, PAL_FX, fx, 3);
    setPal(vdp, PAL_OK, ok, 3);
    setPal(vdp, PAL_ALERT, alert, 3);

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    art.basin = gs::uploadMipped(vdp, basinArt());
    art.ring = gs::uploadMipped(vdp, ringArt());
    art.lip = gs::uploadMipped(vdp, lipArt());
    art.cart[0] = gs::uploadMipped(vdp, cartArt(0, false, false));
    art.cart[1] = gs::uploadMipped(vdp, cartArt(1, false, false));
    art.heavy[0] = gs::uploadMipped(vdp, cartArt(0, true, false));
    art.heavy[1] = gs::uploadMipped(vdp, cartArt(1, true, false));
    art.skit[0] = gs::uploadMipped(vdp, cartArt(0, false, true));
    art.skit[1] = gs::uploadMipped(vdp, cartArt(1, false, true));
    art.wreck = gs::uploadMipped(vdp, wreckArt());
    art.spark = gs::uploadMipped(vdp, sparkArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
}

}  // namespace purse

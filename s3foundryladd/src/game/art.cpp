#include "art.h"

#include <cstdint>
#include <string>

namespace foundryladd {
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

gs::Bitmap worker(int pose) {
    gs::Bitmap b(20, 32);
    b.ellipse(10, 5, 4, 4, 2);
    b.rect(8, 2, 4, 2, 3);
    b.rect(7, 9, 6, 8, 4);
    b.rect(5, 11, 3, 5, 5);
    b.rect(12, 11, 3, 5, 5);
    b.rect(8, 17, 2, 8, 6);
    b.rect(11, 17, 2, 8, 6);
    if (pose == 1) {
        b.rect(7, 17, 2, 7, 0);
        b.rect(8, 19, 2, 7, 6);
    } else if (pose == 2) {
        b.rect(11, 17, 2, 7, 0);
        b.rect(12, 19, 2, 7, 6);
    } else if (pose == 3) {
        b.rect(4, 8, 3, 4, 5);
        b.rect(13, 8, 3, 4, 5);
        b.rect(8, 18, 4, 5, 6);
    } else if (pose == 4 || pose == 5) {
        b.rect(5, 11, 10, 5, 0);
        b.rect(6, 10, 3, 6, 5);
        b.rect(11, 12, 3, 6, 5);
        int dy = pose == 4 ? 0 : 2;
        b.rect(8, 17 + dy, 2, 7, 6);
        b.rect(11, 15 + (2 - dy), 2, 7, 6);
    }
    b.outline(1, false);
    return b;
}

gs::Bitmap ladderArt() {
    gs::Bitmap b(16, 28);
    b.rect(2, 1, 2, 26, 2);
    b.rect(12, 1, 2, 26, 2);
    for (int y = 3; y < 26; y += 5) b.rect(3, y, 10, 2, 3);
    b.outline(1, false);
    return b;
}

gs::Bitmap ladleArt() {
    gs::Bitmap b(36, 22);
    b.rect(2, 2, 3, 12, 2);
    b.ellipse(16, 12, 12, 6, 3);
    b.ellipse(16, 11, 7, 3, 4);
    b.rect(8, 16, 4, 4, 5);
    b.outline(1, false);
    return b;
}

gs::Bitmap furnaceArt() {
    gs::Bitmap b(40, 36);
    b.rect(4, 10, 32, 20, 2);
    b.rect(8, 4, 8, 8, 3);
    b.rect(24, 4, 8, 8, 3);
    b.ellipse(20, 20, 8, 6, 4);
    b.rect(6, 28, 28, 6, 5);
    b.rect(14, 16, 4, 4, 6);
    b.outline(1, false);
    return b;
}

gs::Bitmap stackArt() {
    gs::Bitmap b(16, 48);
    b.rect(4, 8, 8, 34, 2);
    b.rect(2, 4, 12, 6, 3);
    b.rect(6, 16, 4, 6, 4);
    b.rect(3, 40, 10, 6, 5);
    b.outline(1, false);
    return b;
}

gs::Bitmap ingotArt() {
    gs::Bitmap b(18, 12);
    b.poly({{2, 6}, {9, 2}, {16, 6}, {9, 10}}, 2);
    b.rect(6, 5, 6, 2, 3);
    b.outline(1, false);
    return b;
}

gs::Bitmap sparkArt() {
    gs::Bitmap b(8, 8);
    b.ellipse(4, 4, 3, 3, 2);
    b.set(4, 1, 3);
    b.set(4, 6, 3);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(18, 6);
    b.ellipse(9, 3, 8, 2, 1);
    return b;
}

gs::Bitmap tileBrick() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 2);
    b.rect(0, 3, 8, 1, 3);
    b.rect(3, 0, 1, 3, 3);
    b.rect(6, 4, 1, 4, 3);
    return b;
}

gs::Bitmap tileGrate() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 2);
    for (int i = 1; i < 8; i += 2) b.rect(i, 0, 1, 8, 3);
    return b;
}

gs::Bitmap tileEmber() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 2);
    b.ellipse(4, 4, 2, 2, 4);
    return b;
}

gs::Bitmap tileSoot() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 2);
    b.set(2, 3, 3);
    b.set(5, 5, 3);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t hud[] = {0, gs::rgb4(14, 13, 10), gs::rgb4(6, 5, 4)};
    const uint16_t brick[] = {0, gs::rgb4(1, 1, 1), gs::rgb4(6, 3, 2), gs::rgb4(3, 2, 2), gs::rgb4(12, 5, 1)};
    const uint16_t hand[] = {0, gs::rgb4(1, 1, 1), gs::rgb4(12, 8, 5), gs::rgb4(4, 4, 5), gs::rgb4(8, 4, 2),
                             gs::rgb4(10, 8, 3), gs::rgb4(3, 3, 4)};
    const uint16_t iron[] = {0, gs::rgb4(1, 1, 1), gs::rgb4(8, 8, 9), gs::rgb4(4, 4, 5), gs::rgb4(12, 10, 6)};
    const uint16_t heat[] = {0, gs::rgb4(2, 1, 1), gs::rgb4(8, 3, 1), gs::rgb4(5, 2, 1), gs::rgb4(15, 10, 2),
                             gs::rgb4(4, 3, 3), gs::rgb4(15, 14, 6)};
    const uint16_t ladle[] = {0, gs::rgb4(1, 1, 1), gs::rgb4(6, 6, 7), gs::rgb4(9, 8, 6), gs::rgb4(14, 6, 1),
                              gs::rgb4(3, 3, 3)};
    const uint16_t slag[] = {0, gs::rgb4(2, 1, 0), gs::rgb4(10, 4, 1), gs::rgb4(6, 2, 1)};
    const uint16_t fx[] = {0, gs::rgb4(8, 8, 8), gs::rgb4(15, 12, 4), gs::rgb4(15, 15, 10)};
    const uint16_t alert[] = {0, gs::rgb4(15, 5, 2), gs::rgb4(8, 2, 1)};
    const uint16_t ok[] = {0, gs::rgb4(6, 14, 6), gs::rgb4(2, 6, 2)};
    const uint16_t smoke[] = {0, gs::rgb4(2, 2, 2), gs::rgb4(5, 5, 5), gs::rgb4(8, 7, 6), gs::rgb4(12, 6, 2),
                              gs::rgb4(3, 3, 3)};
    const uint16_t gold[] = {0, gs::rgb4(15, 12, 3), gs::rgb4(8, 6, 1)};
    const uint16_t dim[] = {0, gs::rgb4(9, 8, 6), gs::rgb4(4, 3, 3)};
    setPal(vdp, PAL_HUD, hud, 3);
    setPal(vdp, PAL_BRICK, brick, 5);
    setPal(vdp, PAL_HAND, hand, 7);
    setPal(vdp, PAL_IRON, iron, 5);
    setPal(vdp, PAL_HEAT, heat, 7);
    setPal(vdp, PAL_LADLE, ladle, 6);
    setPal(vdp, PAL_SLAG, slag, 4);
    setPal(vdp, PAL_FX, fx, 4);
    setPal(vdp, PAL_ALERT, alert, 3);
    setPal(vdp, PAL_OK, ok, 3);
    setPal(vdp, PAL_SMOKE, smoke, 6);
    setPal(vdp, PAL_GOLD, gold, 3);
    setPal(vdp, PAL_DIM, dim, 3);
    vdp.setFogColor(gs::rgb4(4, 2, 1));

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, tiles, art);
    art.brick = tiles.shared(tileBrick().px.data());
    art.grate = tiles.shared(tileGrate().px.data());
    art.ember = tiles.shared(tileEmber().px.data());
    art.soot = tiles.shared(tileSoot().px.data());

    art.stand = gs::uploadMipped(vdp, worker(0));
    art.walkA = gs::uploadMipped(vdp, worker(1));
    art.walkB = gs::uploadMipped(vdp, worker(2));
    art.jump = gs::uploadMipped(vdp, worker(3));
    art.climbA = gs::uploadMipped(vdp, worker(4));
    art.climbB = gs::uploadMipped(vdp, worker(5));
    art.ladder = gs::uploadMipped(vdp, ladderArt());
    art.ladle = gs::uploadMipped(vdp, ladleArt());
    art.furnace = gs::uploadMipped(vdp, furnaceArt());
    art.stack = gs::uploadMipped(vdp, stackArt());
    art.ingot = gs::uploadMipped(vdp, ingotArt());
    art.spark = gs::uploadMipped(vdp, sparkArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
}

}  // namespace foundryladd

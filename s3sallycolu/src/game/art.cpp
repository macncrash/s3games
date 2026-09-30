#include "game/art.h"

#include <cstdint>
#include <string>

namespace sally {
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

gs::Bitmap wagonArt() {
    gs::Bitmap b(56, 40);
    b.ellipse(16, 14, 7, 6, 4);
    b.rect(12, 18, 6, 8, 5);
    b.rect(8, 24, 4, 6, 6);
    b.rect(18, 24, 4, 6, 6);
    b.rect(24, 16, 26, 12, 2);
    b.poly({{24, 16}, {30, 8}, {48, 8}, {50, 16}}, 3);
    b.rect(28, 18, 8, 6, 7);
    b.rect(40, 18, 6, 6, 8);
    b.ellipse(32, 32, 6, 6, 1);
    b.ellipse(32, 32, 2, 2, 9);
    b.ellipse(48, 32, 6, 6, 1);
    b.ellipse(48, 32, 2, 2, 9);
    b.rect(22, 28, 30, 3, 6);
    b.outline(10, false);
    return b;
}

gs::Bitmap flagArt() {
    gs::Bitmap b(22, 48);
    b.rect(4, 10, 3, 36, 1);
    b.poly({{7, 10}, {20, 16}, {7, 22}}, 2);
    b.rect(8, 12, 6, 4, 3);
    b.rect(3, 44, 5, 3, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap jambArt() {
    gs::Bitmap b(36, 96);
    b.rect(0, 0, 36, 96, 1);
    for (int y = 6; y < 90; y += 14) {
        int off = ((y / 14) & 1) ? 6 : 0;
        for (int x = off; x < 32; x += 14) b.rect(float(x), float(y), 12, 10, (x + y) & 8 ? 2 : 3);
    }
    b.rect(0, 0, 36, 8, 4);
    for (int x = 3; x < 32; x += 10) b.rect(float(x), 0, 5, 12, 5);
    b.outline(6, false);
    return b;
}

gs::Bitmap torchArt() {
    gs::Bitmap b(12, 28);
    b.rect(5, 12, 2, 14, 1);
    b.ellipse(6, 8, 4, 6, 2);
    b.ellipse(6, 9, 2, 3, 3);
    b.rect(4, 24, 4, 3, 4);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(10, 28);
    b.rect(4, 4, 2, 22, 1);
    b.rect(1, 2, 8, 4, 2);
    b.rect(3, 24, 4, 3, 3);
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(16, 12);
    b.ellipse(8, 7, 6, 3, 1);
    b.ellipse(5, 5, 3, 2, 2);
    b.ellipse(11, 6, 2, 2, 3);
    return b;
}

void roadBank(gs::VDP& vdp, int pal, bool band) {
    auto C = gs::rgb4;
    int r = pal * 16;
    if (!band) {
        vdp.setColor(r + 1, C(3, 6, 2));
        vdp.setColor(r + 2, C(2, 4, 1));
        vdp.setColor(r + 3, C(5, 6, 3));
        vdp.setColor(r + 4, C(5, 5, 3));
        vdp.setColor(r + 5, C(3, 3, 2));
        vdp.setColor(r + 6, C(5, 5, 5));
        vdp.setColor(r + 7, C(3, 3, 4));
        vdp.setColor(r + 8, C(8, 7, 5));
        vdp.setColor(r + 9, C(2, 2, 2));
        vdp.setColor(r + 10, C(7, 7, 6));
        vdp.setColor(r + 14, C(13, 12, 4));
        vdp.setColor(r + 15, C(7, 7, 6));
    } else {
        vdp.setColor(r + 1, C(4, 7, 3));
        vdp.setColor(r + 2, C(3, 5, 2));
        vdp.setColor(r + 3, C(6, 7, 3));
        vdp.setColor(r + 4, C(8, 7, 3));
        vdp.setColor(r + 5, C(6, 5, 2));
        vdp.setColor(r + 6, C(10, 8, 4));
        vdp.setColor(r + 7, C(8, 6, 3));
        vdp.setColor(r + 8, C(12, 10, 5));
        vdp.setColor(r + 9, C(4, 3, 2));
        vdp.setColor(r + 10, C(12, 10, 6));
        vdp.setColor(r + 14, C(15, 13, 3));
        vdp.setColor(r + 15, C(11, 9, 5));
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    using C = uint16_t;
    auto c = gs::rgb4;
    const C hud[] = {0, c(15, 15, 14), c(8, 8, 7)};
    const C stone[] = {0, c(6, 6, 7), c(4, 4, 5), c(8, 8, 8), c(10, 9, 7), c(12, 11, 8), c(2, 2, 3)};
    const C wagon[] = {0, c(2, 2, 2), c(8, 6, 3), c(11, 9, 5), c(6, 4, 3), c(9, 7, 5),
                       c(4, 3, 2), c(13, 12, 8), c(3, 5, 8), c(9, 9, 8), c(1, 1, 1)};
    const C flag[] = {0, c(9, 8, 5), c(13, 2, 2), c(15, 14, 12), c(4, 3, 2), c(1, 1, 1)};
    const C torch[] = {0, c(5, 4, 3), c(15, 8, 2), c(15, 14, 5), c(3, 2, 2)};
    const C dust[] = {0, c(10, 9, 7), c(12, 11, 8), c(8, 7, 6)};
    const C good[] = {0, c(8, 15, 8)};
    const C alert[] = {0, c(15, 6, 4)};
    const C gold[] = {0, c(15, 13, 5)};
    setPal(vdp, PAL_HUD, hud, 3);
    setPal(vdp, PAL_STONE, stone, 7);
    setPal(vdp, PAL_WAGON, wagon, 11);
    setPal(vdp, PAL_FLAG, flag, 6);
    setPal(vdp, PAL_TORCH, torch, 5);
    setPal(vdp, PAL_DUST, dust, 4);
    setPal(vdp, PAL_GOOD, good, 2);
    setPal(vdp, PAL_ALERT, alert, 2);
    setPal(vdp, PAL_GOLD, gold, 2);
    roadBank(vdp, PAL_ROAD, false);
    roadBank(vdp, PAL_BAND, true);
    vdp.setFogColor(c(6, 7, 10));

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    art.wagon = gs::uploadMipped(vdp, wagonArt());
    art.flag = gs::uploadMipped(vdp, flagArt());
    art.jamb = gs::uploadMipped(vdp, jambArt());
    art.torch = gs::uploadMipped(vdp, torchArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
}

}  // namespace sally

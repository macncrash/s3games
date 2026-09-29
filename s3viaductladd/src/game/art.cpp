#include "game/art.h"

#include <string>

namespace viaductladd {
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

gs::Bitmap watchArt(int pose) {
    gs::Bitmap b(22, 36);
    b.ellipse(11, 5, 4, 4, 3);
    b.rect(8, 2, 6, 2, 4);
    b.rect(7, 9, 8, 11, 1);
    b.rect(9, 11, 4, 5, 2);
    if (pose == 0) {
        b.rect(3, 11, 4, 7, 5);
        b.rect(15, 11, 4, 7, 5);
        b.rect(8, 20, 3, 12, 1);
        b.rect(12, 20, 3, 12, 6);
    } else if (pose == 1) {
        b.rect(2, 10, 4, 8, 5);
        b.rect(16, 12, 4, 6, 5);
        b.rect(7, 20, 3, 11, 1);
        b.rect(13, 21, 3, 10, 6);
    } else if (pose == 2) {
        b.rect(3, 12, 4, 6, 5);
        b.rect(15, 10, 4, 8, 5);
        b.rect(9, 20, 3, 10, 6);
        b.rect(13, 20, 3, 11, 1);
    } else if (pose == 3) {
        b.rect(4, 8, 3, 6, 5);
        b.rect(15, 8, 3, 6, 5);
        b.rect(8, 18, 3, 8, 1);
        b.rect(12, 16, 3, 7, 6);
    } else {
        b.rect(4, 6, 3, 10, 5);
        b.rect(15, 6, 3, 10, 5);
        b.rect(8, 18, 3, 12, 1);
        b.rect(12, 18, 3, 12, 6);
    }
    b.rect(7, 31, 4, 3, 8);
    b.rect(12, 31, 4, 3, 8);
    b.outline(7, false);
    return b;
}

gs::Bitmap ladderArt() {
    gs::Bitmap b(14, 28);
    b.rect(1, 0, 2, 28, 1);
    b.rect(11, 0, 2, 28, 1);
    for (int y = 3; y < 26; y += 5) b.rect(2, y, 10, 2, 2);
    b.outline(3, false);
    return b;
}

gs::Bitmap crowArt() {
    gs::Bitmap b(18, 10);
    b.ellipse(8, 5, 6, 3, 1);
    b.poly({{13, 4}, {18, 5}, {13, 7}}, 2);
    b.rect(2, 3, 7, 2, 3);
    b.rect(4, 7, 2, 3, 4);
    b.set(5, 4, 5);
    b.outline(6, false);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(10, 16);
    b.rect(4, 0, 2, 4, 1);
    b.rect(2, 4, 6, 6, 2);
    b.rect(3, 5, 4, 4, 3);
    b.rect(4, 10, 2, 6, 1);
    b.outline(4, false);
    return b;
}

gs::Bitmap signalArt() {
    gs::Bitmap b(16, 22);
    b.rect(7, 6, 2, 16, 1);
    b.ellipse(8, 5, 5, 5, 2);
    b.ellipse(8, 5, 2, 2, 3);
    b.rect(2, 18, 12, 3, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap pierArt() {
    gs::Bitmap b(12, 20);
    b.rect(2, 0, 8, 18, 1);
    b.rect(0, 16, 12, 4, 2);
    b.rect(4, 4, 4, 2, 3);
    b.outline(4, false);
    return b;
}

void tile8(gs::Bitmap& b, int c) {
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) b.set(x, y, c);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t hud[] = {0, gs::rgb4(14, 13, 10), gs::rgb4(6, 5, 4)};
    const uint16_t brick[] = {0, gs::rgb4(9, 4, 3), gs::rgb4(6, 3, 2), gs::rgb4(12, 7, 4), gs::rgb4(4, 2, 2),
                              gs::rgb4(11, 9, 6)};
    const uint16_t dusk[] = {0, gs::rgb4(6, 4, 8), gs::rgb4(10, 6, 8), gs::rgb4(14, 9, 6)};
    const uint16_t coat[] = {0, gs::rgb4(2, 3, 7), gs::rgb4(12, 10, 6), gs::rgb4(13, 9, 6), gs::rgb4(1, 1, 2),
                             gs::rgb4(4, 5, 9), gs::rgb4(1, 2, 5), gs::rgb4(0, 0, 1), gs::rgb4(3, 2, 2)};
    const uint16_t brass[] = {0, gs::rgb4(10, 8, 3), gs::rgb4(14, 12, 5), gs::rgb4(6, 4, 2)};
    const uint16_t gold[] = {0, gs::rgb4(15, 13, 4), gs::rgb4(10, 7, 2)};
    const uint16_t lamp[] = {0, gs::rgb4(5, 4, 3), gs::rgb4(12, 8, 3), gs::rgb4(15, 14, 6), gs::rgb4(3, 2, 2)};
    const uint16_t crow[] = {0, gs::rgb4(2, 2, 3), gs::rgb4(8, 7, 6), gs::rgb4(1, 1, 2), gs::rgb4(5, 4, 3),
                             gs::rgb4(14, 12, 4), gs::rgb4(0, 0, 0)};
    const uint16_t alert[] = {0, gs::rgb4(14, 3, 2)};
    const uint16_t ok[] = {0, gs::rgb4(6, 13, 7)};
    const uint16_t arch[] = {0, gs::rgb4(7, 5, 6), gs::rgb4(4, 3, 5)};
    const uint16_t gorge[] = {0, gs::rgb4(2, 3, 4), gs::rgb4(3, 5, 5), gs::rgb4(5, 6, 6)};
    const uint16_t dim[] = {0, gs::rgb4(8, 7, 6)};
    setPal(vdp, PAL_HUD, hud, 3);
    setPal(vdp, PAL_BRICK, brick, 6);
    setPal(vdp, PAL_DUSK, dusk, 4);
    setPal(vdp, PAL_COAT, coat, 9);
    setPal(vdp, PAL_BRASS, brass, 4);
    setPal(vdp, PAL_GOLD, gold, 3);
    setPal(vdp, PAL_LAMP, lamp, 5);
    setPal(vdp, PAL_CROW, crow, 7);
    setPal(vdp, PAL_ALERT, alert, 2);
    setPal(vdp, PAL_OK, ok, 2);
    setPal(vdp, PAL_ARCH, arch, 3);
    setPal(vdp, PAL_GORGE, gorge, 4);
    setPal(vdp, PAL_DIM, dim, 2);

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);

    auto solid = [&](int c) {
        gs::Bitmap b(8, 8);
        tile8(b, c);
        return tiles.shared(b.px.data());
    };
    art.sky = solid(1);
    art.cloud = solid(2);
    {
        gs::Bitmap b(8, 8);
        tile8(b, 1);
        b.rect(0, 6, 8, 2, 2);
        b.rect(0, 0, 2, 8, 3);
        art.brick = tiles.shared(b.px.data());
        gs::Bitmap c(8, 8);
        tile8(c, 2);
        c.rect(0, 3, 8, 2, 1);
        c.rect(4, 0, 2, 8, 3);
        art.brickB = tiles.shared(c.px.data());
        gs::Bitmap d(8, 8);
        tile8(d, 4);
        d.rect(0, 6, 8, 2, 5);
        art.cap = tiles.shared(d.px.data());
        gs::Bitmap j(8, 8);
        tile8(j, 3);
        j.rect(3, 0, 2, 8, 4);
        art.joint = tiles.shared(j.px.data());
        gs::Bitmap v(8, 8);
        tile8(v, 1);
        v.poly({{0, 8}, {8, 0}, {8, 8}}, 2);
        art.voussoir = tiles.shared(v.px.data());
        gs::Bitmap a(8, 8);
        tile8(a, 0);
        a.rect(3, 0, 2, 8, 1);
        art.arch = tiles.shared(a.px.data());
        gs::Bitmap r(8, 8);
        r.rect(1, 2, 6, 2, 1);
        r.rect(3, 4, 2, 4, 2);
        art.rail = tiles.shared(r.px.data());
        gs::Bitmap g(8, 8);
        tile8(g, 1);
        g.rect(0, 5, 8, 3, 2);
        art.gorge = tiles.shared(g.px.data());
        gs::Bitmap h(8, 8);
        tile8(h, 2);
        h.rect(2, 1, 3, 2, 3);
        art.gorgeB = tiles.shared(h.px.data());
    }

    art.stand = gs::uploadMipped(vdp, watchArt(0));
    art.walkA = gs::uploadMipped(vdp, watchArt(1));
    art.walkB = gs::uploadMipped(vdp, watchArt(2));
    art.jump = gs::uploadMipped(vdp, watchArt(3));
    art.climbA = gs::uploadMipped(vdp, watchArt(4));
    art.climbB = gs::uploadMipped(vdp, watchArt(4));
    {
        gs::Bitmap alt = watchArt(4);
        alt.rect(5, 8, 3, 8, 5);
        alt.rect(14, 10, 3, 8, 5);
        art.climbB = gs::uploadMipped(vdp, alt);
    }
    art.ladder = gs::uploadMipped(vdp, ladderArt());
    art.crow = gs::uploadMipped(vdp, crowArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.signal = gs::uploadMipped(vdp, signalArt());
    art.pier = gs::uploadMipped(vdp, pierArt());
}

}  // namespace viaductladd

#include "art.h"

#include <cstdint>
#include <initializer_list>

namespace twc {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
    for (; i < 16; ++i) vdp.setColor(pal * 16 + i, 0);
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

gs::Bitmap carArt() {
    gs::Bitmap b(36, 18);
    b.rect(2, 8, 32, 7, 1);
    b.rect(8, 3, 16, 6, 2);
    b.rect(10, 4, 6, 4, 3);
    b.rect(18, 4, 5, 4, 3);
    b.ellipse(8, 15, 4, 3, 4);
    b.ellipse(28, 15, 4, 3, 4);
    b.rect(31, 9, 3, 2, 5);
    b.outline(6, false);
    return b;
}

gs::Bitmap truckArt() {
    gs::Bitmap b(40, 26);
    b.rect(2, 8, 16, 12, 1);
    b.rect(4, 4, 12, 6, 2);
    b.rect(6, 5, 7, 4, 3);
    b.rect(18, 6, 20, 14, 4);
    b.rect(20, 8, 16, 8, 5);
    b.ellipse(10, 22, 4, 3, 6);
    b.ellipse(28, 22, 4, 3, 6);
    b.ellipse(34, 22, 4, 3, 6);
    b.rect(17, 10, 2, 6, 7);
    b.outline(8, false);
    return b;
}

gs::Bitmap towerArt() {
    gs::Bitmap b(120, 100);
    b.rect(0, 10, 22, 88, 1);
    b.rect(98, 10, 22, 88, 1);
    b.rect(4, 16, 14, 76, 2);
    b.rect(102, 16, 14, 76, 2);
    for (int y = 22; y < 80; y += 14) {
        b.rect(6, float(y), 6, 7, 4);
        b.rect(108, float(y), 6, 7, 4);
    }
    for (int i = 0; i < 4; i++) {
        b.rect(float(i * 6), 0, 5, 12, 5);
        b.rect(float(98 + i * 6), 0, 5, 12, 5);
    }
    b.rect(0, 78, 120, 20, 1);
    b.rect(8, 74, 104, 6, 3);
    b.rect(54, 82, 12, 12, 8);
    b.outline(9, false);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 6, 6, 1);
    b.ellipse(8, 8, 3, 3, 2);
    b.rect(7, 13, 2, 3, 3);
    return b;
}

gs::Bitmap linkArt() {
    gs::Bitmap b(8, 8);
    b.rect(1, 1, 6, 6, 1);
    b.rect(3, 2, 2, 4, 0);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(28, 40);
    b.rect(12, 24, 4, 14, 3);
    b.ellipse(14, 14, 12, 12, 1);
    b.ellipse(10, 12, 5, 5, 2);
    b.outline(4, false);
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(16, 12);
    b.ellipse(8, 6, 7, 4, 1);
    b.ellipse(6, 5, 3, 2, 2);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(20, 8);
    b.ellipse(10, 4, 9, 3, 1);
    return b;
}

gs::Bitmap cloudArt() {
    gs::Bitmap b(28, 12);
    b.ellipse(10, 7, 8, 4, 1);
    b.ellipse(18, 6, 7, 4, 1);
    return b;
}

gs::Bitmap pennantArt() {
    gs::Bitmap b(18, 12);
    b.rect(0, 0, 2, 12, 2);
    b.poly({{2, 1}, {16, 4}, {2, 8}}, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    vdp.setFogColor(gs::rgb4(3, 4, 6));
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(14, 13, 10), gs::rgb4(6, 6, 8)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(14, 11, 4), gs::rgb4(8, 6, 2)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(14, 3, 2), gs::rgb4(8, 2, 2)});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(6, 13, 6), gs::rgb4(2, 6, 3)});
    setPal(vdp, PAL_CAR,
           {0, gs::rgb4(12, 4, 3), gs::rgb4(8, 3, 2), gs::rgb4(6, 8, 10), gs::rgb4(2, 2, 2), gs::rgb4(14, 12, 4),
            gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_TRUCK,
           {0, gs::rgb4(5, 6, 4), gs::rgb4(3, 4, 3), gs::rgb4(8, 10, 8), gs::rgb4(4, 5, 3), gs::rgb4(6, 7, 4),
            gs::rgb4(2, 2, 2), gs::rgb4(8, 7, 3), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_STONE,
           {0, gs::rgb4(6, 6, 7), gs::rgb4(8, 8, 9), gs::rgb4(4, 4, 5), gs::rgb4(10, 11, 12), gs::rgb4(5, 5, 6),
            gs::rgb4(2, 2, 3), gs::rgb4(14, 12, 6), gs::rgb4(3, 3, 4), gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_TREE,
           {0, gs::rgb4(3, 6, 2), gs::rgb4(5, 8, 3), gs::rgb4(5, 4, 2), gs::rgb4(2, 3, 1)});
    setPal(vdp, PAL_FX, {0, gs::rgb4(10, 10, 9), gs::rgb4(14, 13, 10), gs::rgb4(4, 4, 4)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(15, 14, 6), gs::rgb4(15, 15, 12), gs::rgb4(6, 5, 3)});
    setPal(vdp, PAL_RED, {0, gs::rgb4(13, 2, 1), gs::rgb4(8, 1, 1)});
    setPal(vdp, PAL_WHITE, {0, gs::rgb4(14, 14, 13), gs::rgb4(9, 9, 8)});
    setPal(vdp, PAL_FLAG, {0, gs::rgb4(12, 3, 2), gs::rgb4(6, 5, 3), gs::rgb4(14, 12, 4)});

    int r = PAL_ROAD * 16;
    vdp.setColor(r + 0, 0);
    vdp.setColor(r + 1, gs::rgb4(4, 6, 3));
    vdp.setColor(r + 2, gs::rgb4(3, 4, 2));
    vdp.setColor(r + 3, gs::rgb4(6, 6, 4));
    vdp.setColor(r + 4, gs::rgb4(5, 5, 3));
    vdp.setColor(r + 5, gs::rgb4(3, 3, 2));
    vdp.setColor(r + 6, gs::rgb4(3, 3, 4));
    vdp.setColor(r + 7, gs::rgb4(5, 5, 6));
    vdp.setColor(r + 8, gs::rgb4(6, 5, 4));
    vdp.setColor(r + 9, gs::rgb4(2, 2, 2));
    vdp.setColor(r + 10, gs::rgb4(4, 4, 4));
    vdp.setColor(r + 11, gs::rgb4(3, 4, 6));
    vdp.setColor(r + 12, gs::rgb4(4, 5, 7));
    vdp.setColor(r + 13, gs::rgb4(7, 8, 10));
    vdp.setColor(r + 14, gs::rgb4(12, 11, 6));
    vdp.setColor(r + 15, gs::rgb4(6, 6, 5));

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, tiles, art);
    art.car = gs::uploadMipped(vdp, carArt());
    art.truck = gs::uploadMipped(vdp, truckArt());
    art.tower = gs::uploadMipped(vdp, towerArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.link = gs::uploadMipped(vdp, linkArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.pennant = gs::uploadMipped(vdp, pennantArt());
}

}  // namespace twc

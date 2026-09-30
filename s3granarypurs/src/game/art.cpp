#include "art.h"

#include <string>

namespace gpurs {
namespace {

void setPal(gs::VDP& vdp, int pal, const uint16_t* cs, int n) {
    for (int i = 0; i < 16; ++i) vdp.setColor(pal * 16 + i, i < n ? cs[i] : 0);
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& art) {
    for (int i = 0; i < 96; ++i) {
        gs::TextStyle st;
        st.scale = 1;
        st.color = 1;
        gs::Bitmap g = gs::textBitmap(std::string(1, char(32 + i)), st);
        gs::Bitmap tile(8, 8);
        for (int y = 0; y < g.h && y < 8; ++y)
            for (int x = 0; x < g.w && x < 8; ++x)
                if (g.get(x, y)) tile.set(x, y, 1);
        art.font[i] = tiles.shared(tile.px.data());
        art.glyph[i] = gs::uploadMipped(vdp, g.w > 0 ? g : tile);
    }
}

gs::Bitmap millerArt() {
    gs::Bitmap b(40, 36);
    b.rect(4, 14, 28, 14, 2);
    b.rect(6, 16, 16, 8, 4);
    b.rect(22, 10, 12, 10, 5);
    b.rect(24, 12, 6, 5, 6);
    b.ellipse(10, 28, 6, 6, 1);
    b.ellipse(26, 28, 6, 6, 1);
    b.rect(8, 8, 4, 8, 3);
    b.ellipse(10, 6, 3, 3, 7);
    b.rect(2, 18, 4, 3, 8);
    b.outline(9, false);
    return b;
}

gs::Bitmap augerArt() {
    gs::Bitmap b(28, 32);
    b.rect(6, 8, 16, 14, 2);
    b.rect(8, 10, 12, 6, 4);
    b.rect(18, 4, 6, 10, 3);
    b.ellipse(8, 24, 5, 5, 1);
    b.ellipse(20, 24, 5, 5, 1);
    b.line(4, 6, 22, 2, 5, 2);
    b.outline(6, false);
    return b;
}

gs::Bitmap tipperArt() {
    gs::Bitmap b(36, 30);
    b.poly({{4, 16}, {22, 8}, {32, 12}, {32, 20}, {4, 20}}, 2);
    b.rect(4, 18, 28, 6, 3);
    b.ellipse(10, 24, 5, 5, 1);
    b.ellipse(26, 24, 5, 5, 1);
    b.rect(28, 10, 6, 8, 5);
    b.rect(30, 12, 3, 3, 6);
    b.outline(7, false);
    return b;
}

gs::Bitmap thresherArt() {
    gs::Bitmap b(42, 34);
    b.rect(4, 10, 30, 14, 2);
    b.rect(6, 12, 18, 8, 4);
    b.ellipse(34, 16, 7, 7, 3);
    b.ellipse(34, 16, 3, 3, 5);
    b.ellipse(10, 26, 6, 6, 1);
    b.ellipse(24, 26, 6, 6, 1);
    b.rect(8, 4, 8, 8, 6);
    b.outline(7, false);
    return b;
}

gs::Bitmap doorArt() {
    gs::Bitmap b(48, 40);
    b.poly({{2, 14}, {24, 2}, {46, 14}}, 3);
    b.rect(4, 14, 40, 24, 2);
    b.rect(16, 18, 16, 18, 1);
    b.rect(22, 26, 4, 8, 5);
    b.rect(20, 6, 8, 6, 4);
    b.outline(6, false);
    return b;
}

gs::Bitmap siloArt() {
    gs::Bitmap b(22, 48);
    b.ellipse(11, 8, 9, 6, 3);
    b.rect(2, 8, 18, 36, 2);
    for (int y = 14; y < 40; y += 6) b.rect(2, float(y), 18, 2, 1);
    b.rect(8, 22, 6, 8, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap sackArt() {
    gs::Bitmap b(16, 14);
    b.ellipse(8, 8, 7, 5, 1);
    b.rect(5, 2, 6, 3, 2);
    b.line(3, 8, 13, 7, 3, 1);
    return b;
}

gs::Bitmap chuteArt() {
    gs::Bitmap b(10, 28);
    b.rect(3, 0, 4, 22, 1);
    b.poly({{1, 20}, {9, 20}, {7, 27}, {3, 27}}, 2);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(10, 22);
    b.rect(4, 0, 2, 10, 1);
    b.ellipse(5, 15, 4, 4, 2);
    b.rect(4, 18, 2, 3, 3);
    return b;
}

gs::Bitmap chevArt() {
    gs::Bitmap b(12, 10);
    b.poly({{1, 2}, {6, 8}, {11, 2}, {9, 2}, {6, 6}, {3, 2}}, 1);
    return b;
}

gs::Bitmap puffArt() {
    gs::Bitmap b(14, 12);
    b.ellipse(7, 6, 6, 5, 1);
    b.ellipse(5, 5, 2, 2, 2);
    return b;
}

gs::Bitmap sparkArt() {
    gs::Bitmap b(10, 10);
    b.line(5, 0, 5, 9, 1, 2);
    b.line(0, 5, 9, 5, 1, 2);
    b.ellipse(5, 5, 2, 2, 2);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(16, 6);
    b.ellipse(8, 3, 7, 2, 1);
    return b;
}

gs::Bitmap grainArt() {
    gs::Bitmap b(6, 6);
    b.ellipse(3, 3, 2, 2, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t hud[] = {0, gs::rgb4(15, 14, 9), gs::rgb4(8, 6, 3), gs::rgb4(15, 11, 3), gs::rgb4(14, 5, 2)};
    const uint16_t you[] = {0, gs::rgb4(2, 2, 2), gs::rgb4(6, 8, 3), gs::rgb4(4, 4, 3), gs::rgb4(12, 11, 6),
                            gs::rgb4(8, 6, 3), gs::rgb4(14, 13, 8), gs::rgb4(15, 12, 4), gs::rgb4(3, 3, 2),
                            gs::rgb4(1, 1, 1)};
    const uint16_t auger[] = {0, gs::rgb4(3, 3, 3), gs::rgb4(9, 7, 3), gs::rgb4(5, 4, 2), gs::rgb4(13, 11, 5),
                              gs::rgb4(11, 8, 2), gs::rgb4(1, 1, 1)};
    const uint16_t tip[] = {0, gs::rgb4(2, 2, 3), gs::rgb4(8, 5, 2), gs::rgb4(5, 3, 1), gs::rgb4(12, 8, 3),
                            gs::rgb4(4, 5, 7), gs::rgb4(10, 12, 14), gs::rgb4(1, 1, 1)};
    const uint16_t thresh[] = {0, gs::rgb4(3, 2, 2), gs::rgb4(10, 4, 2), gs::rgb4(6, 3, 2), gs::rgb4(14, 10, 4),
                               gs::rgb4(15, 13, 6), gs::rgb4(8, 6, 3), gs::rgb4(1, 1, 1)};
    const uint16_t mill[] = {0, gs::rgb4(4, 3, 2), gs::rgb4(9, 6, 3), gs::rgb4(12, 5, 2), gs::rgb4(7, 5, 2),
                             gs::rgb4(14, 12, 5), gs::rgb4(2, 2, 1)};
    const uint16_t silo[] = {0, gs::rgb4(7, 7, 6), gs::rgb4(11, 11, 9), gs::rgb4(14, 13, 8), gs::rgb4(6, 5, 4),
                             gs::rgb4(2, 2, 2)};
    const uint16_t fx[] = {0, gs::rgb4(12, 10, 6), gs::rgb4(15, 14, 8), gs::rgb4(8, 6, 3)};
    const uint16_t sack[] = {0, gs::rgb4(11, 9, 4), gs::rgb4(7, 5, 2), gs::rgb4(14, 12, 6), gs::rgb4(4, 3, 1)};
    const uint16_t shock[] = {0, gs::rgb4(15, 15, 12), gs::rgb4(15, 10, 3)};
    const uint16_t road[] = {0,
                             gs::rgb4(7, 8, 2),
                             gs::rgb4(5, 6, 1),
                             gs::rgb4(9, 8, 3),
                             gs::rgb4(8, 6, 2),
                             gs::rgb4(6, 4, 1),
                             gs::rgb4(10, 7, 2),
                             gs::rgb4(7, 5, 1),
                             gs::rgb4(11, 9, 4),
                             gs::rgb4(6, 5, 2),
                             gs::rgb4(8, 6, 3),
                             gs::rgb4(4, 6, 8),
                             gs::rgb4(6, 8, 10),
                             gs::rgb4(9, 11, 12),
                             gs::rgb4(13, 11, 4),
                             gs::rgb4(10, 8, 3)};
    setPal(vdp, PAL_HUD, hud, 5);
    setPal(vdp, PAL_YOU, you, 10);
    setPal(vdp, PAL_AUGER, auger, 7);
    setPal(vdp, PAL_TIP, tip, 8);
    setPal(vdp, PAL_THRESH, thresh, 8);
    setPal(vdp, PAL_MILL, mill, 7);
    setPal(vdp, PAL_SILO, silo, 6);
    setPal(vdp, PAL_FX, fx, 4);
    setPal(vdp, PAL_SACK, sack, 5);
    setPal(vdp, PAL_SHOCK, shock, 3);
    setPal(vdp, PAL_ROAD, road, 16);
    vdp.setFogColor(gs::rgb4(6, 5, 3));

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    art.miller = gs::uploadMipped(vdp, millerArt());
    art.auger = gs::uploadMipped(vdp, augerArt());
    art.tipper = gs::uploadMipped(vdp, tipperArt());
    art.thresher = gs::uploadMipped(vdp, thresherArt());
    art.door = gs::uploadMipped(vdp, doorArt());
    art.silo = gs::uploadMipped(vdp, siloArt());
    art.sack = gs::uploadMipped(vdp, sackArt());
    art.chute = gs::uploadMipped(vdp, chuteArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.chev = gs::uploadMipped(vdp, chevArt());
    art.puff = gs::uploadMipped(vdp, puffArt());
    art.spark = gs::uploadMipped(vdp, sparkArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.grain = gs::uploadMipped(vdp, grainArt());
}

}  // namespace gpurs

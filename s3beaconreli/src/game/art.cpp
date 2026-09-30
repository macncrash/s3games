#include "art.h"

#include <string>

namespace beacon {
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

gs::Bitmap keeperArt() {
    gs::Bitmap b(28, 44);
    b.ellipse(14, 8, 6, 6, 3);
    b.rect(8, 3, 12, 5, 4);
    b.rect(7, 6, 3, 3, 4);
    b.rect(10, 13, 8, 3, 5);
    b.rect(8, 16, 12, 14, 1);
    b.rect(10, 18, 8, 8, 2);
    b.rect(5, 17, 4, 11, 6);
    b.rect(19, 17, 4, 11, 6);
    b.rect(9, 30, 4, 10, 7);
    b.rect(15, 30, 4, 10, 7);
    b.rect(8, 38, 5, 3, 8);
    b.rect(15, 38, 5, 3, 8);
    b.rect(12, 8, 2, 2, 9);
    b.outline(10, false);
    return b;
}

gs::Bitmap wreckArt(int step, bool bucket) {
    gs::Bitmap b(26, 36);
    b.ellipse(13, 6, 5, 5, 3);
    b.rect(9, 4, 8, 3, 4);
    b.rect(8, 11, 10, 12, bucket ? 2 : 1);
    b.rect(10, 13, 6, 6, 5);
    int arm = step ? 2 : 16;
    b.rect(float(arm), 13, 6, 3, 6);
    if (bucket) {
        b.rect(float(step ? 1 : 18), 16, 5, 6, 7);
        b.rect(float(step ? 2 : 19), 17, 3, 3, 8);
    }
    b.rect(step ? 8.f : 12.f, 23, 3, 9, 1);
    b.rect(step ? 14.f : 10.f, 23, 3, 9, 2);
    b.rect(7, 31, 5, 3, 9);
    b.rect(14, 31, 5, 3, 9);
    b.outline(10, false);
    return b;
}

gs::Bitmap houseArt() {
    gs::Bitmap b(72, 110);
    b.poly({{8, 28}, {36, 4}, {64, 28}}, 2);
    b.rect(14, 26, 44, 78, 1);
    b.rect(18, 30, 36, 70, 3);
    b.rect(28, 36, 16, 22, 0);
    b.rect(30, 38, 12, 18, 4);
    for (int y = 64; y < 98; y += 8) b.rect(20, float(y), 32, 2, 5);
    b.rect(32, 78, 8, 22, 6);
    b.rect(8, 100, 56, 8, 2);
    b.rect(4, 106, 64, 4, 7);
    b.rect(34, 8, 4, 10, 8);
    b.outline(9, false);
    return b;
}

gs::Bitmap flameArt() {
    gs::Bitmap b(16, 22);
    b.ellipse(8, 14, 5, 6, 1);
    b.ellipse(8, 10, 3, 6, 2);
    b.ellipse(8, 7, 2, 4, 3);
    return b;
}

gs::Bitmap glassArt() {
    gs::Bitmap b(22, 26);
    b.rect(2, 4, 18, 18, 1);
    b.rect(4, 6, 14, 14, 0);
    b.line(2, 4, 20, 4, 2, 1.2f);
    b.line(2, 21, 20, 21, 2, 1.2f);
    b.line(2, 4, 2, 21, 2, 1.2f);
    b.line(20, 4, 20, 21, 2, 1.2f);
    b.line(11, 4, 11, 21, 3, 1.f);
    return b;
}

gs::Bitmap beamArt() {
    gs::Bitmap b(48, 8);
    b.rect(0, 2, 48, 4, 1);
    b.rect(8, 3, 28, 2, 2);
    return b;
}

gs::Bitmap flareArt() {
    gs::Bitmap b(14, 14);
    b.ellipse(7, 7, 6, 6, 1);
    b.ellipse(7, 7, 3, 3, 2);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(18, 16);
    b.poly({{9, 1}, {2, 10}, {16, 10}}, 1);
    b.rect(2, 10, 14, 3, 2);
    b.ellipse(9, 8, 2, 2, 3);
    b.rect(8, 0, 2, 3, 4);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(8, 48);
    b.rect(3, 0, 2, 48, 1);
    b.rect(1, 44, 6, 4, 2);
    return b;
}

gs::Bitmap rockArt() {
    gs::Bitmap b(40, 22);
    b.poly({{2, 18}, {10, 6}, {22, 10}, {36, 4}, {38, 18}}, 1);
    b.poly({{8, 16}, {14, 10}, {24, 14}, {20, 18}}, 2);
    b.outline(3, false);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    static const uint16_t hud[] = {0, gs::rgb4(14, 13, 10), gs::rgb4(6, 6, 8)};
    static const uint16_t alert[] = {0, gs::rgb4(15, 4, 3)};
    static const uint16_t ok[] = {0, gs::rgb4(6, 15, 8)};
    static const uint16_t bell[] = {0, gs::rgb4(15, 12, 3), gs::rgb4(10, 7, 2), gs::rgb4(15, 15, 8), gs::rgb4(6, 5, 3)};
    static const uint16_t keep[] = {0, gs::rgb4(2, 3, 8), gs::rgb4(4, 6, 12), gs::rgb4(12, 8, 5), gs::rgb4(1, 1, 3),
                                    gs::rgb4(10, 8, 3), gs::rgb4(3, 4, 9), gs::rgb4(1, 2, 4), gs::rgb4(2, 2, 2),
                                    gs::rgb4(8, 12, 15), gs::rgb4(0, 0, 1)};
    static const uint16_t wreck[] = {0, gs::rgb4(3, 5, 3), gs::rgb4(5, 7, 4), gs::rgb4(11, 8, 6), gs::rgb4(2, 2, 2),
                                     gs::rgb4(6, 3, 2), gs::rgb4(8, 7, 5), gs::rgb4(4, 4, 5), gs::rgb4(9, 9, 8),
                                     gs::rgb4(2, 2, 1), gs::rgb4(0, 0, 0)};
    static const uint16_t douse[] = {0, gs::rgb4(4, 4, 6), gs::rgb4(6, 6, 9), gs::rgb4(11, 8, 6), gs::rgb4(1, 1, 2),
                                     gs::rgb4(8, 8, 10), gs::rgb4(7, 6, 5), gs::rgb4(3, 5, 8), gs::rgb4(6, 10, 13),
                                     gs::rgb4(2, 2, 3), gs::rgb4(0, 0, 1)};
    static const uint16_t rock[] = {0, gs::rgb4(4, 4, 5), gs::rgb4(7, 7, 8), gs::rgb4(2, 2, 3)};
    static const uint16_t lamp[] = {0, gs::rgb4(5, 4, 3), gs::rgb4(3, 3, 4), gs::rgb4(6, 5, 4), gs::rgb4(14, 12, 6),
                                    gs::rgb4(3, 3, 3), gs::rgb4(2, 2, 3), gs::rgb4(4, 4, 5), gs::rgb4(8, 7, 4),
                                    gs::rgb4(1, 1, 2)};
    static const uint16_t fx[] = {0, gs::rgb4(15, 8, 2), gs::rgb4(15, 14, 5), gs::rgb4(15, 15, 12), gs::rgb4(10, 12, 14)};

    setPal(vdp, PAL_HUD, hud, 3);
    setPal(vdp, PAL_ALERT, alert, 2);
    setPal(vdp, PAL_OK, ok, 2);
    setPal(vdp, PAL_BELL, bell, 5);
    setPal(vdp, PAL_KEEP, keep, 11);
    setPal(vdp, PAL_WRECK, wreck, 11);
    setPal(vdp, PAL_DOUSE, douse, 11);
    setPal(vdp, PAL_ROCK, rock, 4);
    setPal(vdp, PAL_LAMP, lamp, 9);
    setPal(vdp, PAL_FX, fx, 5);
    vdp.setFogColor(gs::rgb4(1, 2, 4));

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, tiles, art);
    art.keeper = gs::uploadMipped(vdp, keeperArt());
    art.wreck[0] = gs::uploadMipped(vdp, wreckArt(0, false));
    art.wreck[1] = gs::uploadMipped(vdp, wreckArt(1, false));
    art.douse[0] = gs::uploadMipped(vdp, wreckArt(0, true));
    art.douse[1] = gs::uploadMipped(vdp, wreckArt(1, true));
    art.house = gs::uploadMipped(vdp, houseArt());
    art.flame = gs::uploadMipped(vdp, flameArt());
    art.glass = gs::uploadMipped(vdp, glassArt());
    art.beam = gs::uploadMipped(vdp, beamArt());
    art.flare = gs::uploadMipped(vdp, flareArt());
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.rock = gs::uploadMipped(vdp, rockArt());
}

}  // namespace beacon

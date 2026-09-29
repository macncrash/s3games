#include "art.h"

#include <string>

namespace palisade {
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

gs::Bitmap stakeArt() {
    gs::Bitmap b(18, 120);
    b.rect(6, 0, 6, 14, 3);
    b.poly({{9, 0}, {4, 16}, {14, 16}}, 4);
    b.rect(5, 14, 8, 100, 1);
    b.rect(7, 14, 3, 100, 2);
    for (int y = 22; y < 108; y += 14) b.rect(5, float(y), 8, 2, 5);
    b.rect(3, 108, 12, 10, 6);
    b.outline(7, false);
    return b;
}

gs::Bitmap gateArt() {
    gs::Bitmap b(46, 96);
    b.rect(0, 8, 8, 88, 1);
    b.rect(38, 8, 8, 88, 1);
    b.rect(2, 8, 4, 88, 2);
    b.rect(40, 8, 4, 88, 2);
    b.rect(0, 0, 46, 12, 3);
    b.rect(4, 2, 38, 4, 4);
    b.rect(18, 12, 10, 70, 5);
    b.rect(20, 40, 6, 8, 6);
    b.outline(7, false);
    return b;
}

gs::Bitmap hillArt() {
    gs::Bitmap b(64, 28);
    b.poly({{0, 28}, {0, 16}, {18, 8}, {40, 14}, {64, 6}, {64, 28}}, 1);
    b.rect(0, 22, 64, 6, 2);
    return b;
}

gs::Bitmap skybandArt() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 1);
    b.rect(1, 1, 2, 2, 2);
    return b;
}

gs::Bitmap person(int step, int kind) {
    // kind 0 defender, 1 climber, 2 runner, 3 shielded
    gs::Bitmap b(24, 36);
    int skin = 3;
    int cloth = kind == 0 ? 1 : 4;
    b.ellipse(12, 6, 5, 5, skin);
    if (kind == 0) b.rect(8, 2, 8, 3, 5);
    b.rect(8, 11, 8, 10, cloth);
    if (kind == 3) {
        b.ellipse(12, 16, 8, 9, 6);
        b.ellipse(12, 16, 4, 5, 2);
    }
    int arm = step ? 4 : 7;
    b.rect(float(arm), 12, 4, 8, skin);
    b.rect(step ? 15.f : 13.f, 12, 4, 8, cloth);
    b.rect(step ? 8.f : 11.f, 21, 4, 10, 2);
    b.rect(step ? 13.f : 10.f, 21, 4, 10, cloth);
    if (kind == 2) b.rect(16, 14, 7, 2, 7);
    else if (kind != 0) b.rect(15, 18, 7, 2, 7);
    b.outline(8, false);
    return b;
}

gs::Bitmap spearArt() {
    gs::Bitmap b(8, 52);
    b.poly({{4, 0}, {1, 10}, {7, 10}}, 3);
    b.rect(3, 8, 2, 42, 1);
    b.rect(2, 46, 4, 4, 2);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(26, 28);
    b.rect(11, 0, 4, 5, 3);
    b.ellipse(13, 15, 11, 9, 1);
    b.ellipse(13, 17, 6, 5, 2);
    b.rect(11, 22, 4, 4, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap ropeArt() {
    gs::Bitmap b(6, 40);
    b.rect(2, 0, 2, 32, 1);
    b.ellipse(3, 34, 3, 4, 2);
    return b;
}

gs::Bitmap sparkArt() {
    gs::Bitmap b(12, 12);
    b.ellipse(6, 6, 5, 5, 1);
    b.ellipse(6, 6, 2, 2, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shadow = gs::rgb4(1, 1, 1);
    const uint16_t hud[] = {0, gs::rgb4(14, 13, 10), gs::rgb4(7, 6, 5), gs::rgb4(15, 12, 4), gs::rgb4(15, 5, 3),
                            gs::rgb4(6, 14, 7), shadow};
    const uint16_t wood[] = {0,
                             gs::rgb4(6, 4, 2),
                             gs::rgb4(9, 6, 3),
                             gs::rgb4(12, 10, 6),
                             gs::rgb4(14, 13, 8),
                             gs::rgb4(4, 3, 2),
                             gs::rgb4(5, 4, 3),
                             gs::rgb4(2, 1, 1),
                             shadow};
    const uint16_t you[] = {0, gs::rgb4(4, 5, 7), gs::rgb4(3, 3, 4), gs::rgb4(12, 9, 6), gs::rgb4(6, 7, 8),
                            gs::rgb4(10, 9, 4), gs::rgb4(8, 8, 9), gs::rgb4(11, 10, 8), gs::rgb4(1, 1, 1), shadow};
    const uint16_t foe[] = {0, gs::rgb4(5, 3, 2), gs::rgb4(3, 3, 2), gs::rgb4(11, 8, 6), gs::rgb4(7, 3, 2),
                            gs::rgb4(4, 4, 3), gs::rgb4(8, 8, 7), gs::rgb4(9, 8, 4), gs::rgb4(1, 1, 1), shadow};
    const uint16_t shield[] = {0, gs::rgb4(5, 3, 2), gs::rgb4(8, 8, 6), gs::rgb4(11, 8, 6), gs::rgb4(6, 4, 3),
                               gs::rgb4(4, 4, 3), gs::rgb4(10, 10, 8), gs::rgb4(8, 7, 3), gs::rgb4(1, 1, 1), shadow};
    const uint16_t fx[] = {0, gs::rgb4(15, 14, 8), gs::rgb4(15, 15, 13), shadow};
    const uint16_t bell[] = {0, gs::rgb4(13, 10, 3), gs::rgb4(8, 6, 2), gs::rgb4(5, 4, 2), gs::rgb4(15, 13, 7),
                             gs::rgb4(2, 2, 1), shadow};
    const uint16_t alert[] = {0, gs::rgb4(15, 4, 3), gs::rgb4(8, 1, 1), shadow};
    const uint16_t ok[] = {0, gs::rgb4(7, 15, 6), gs::rgb4(2, 6, 2), shadow};
    const uint16_t field[] = {0, gs::rgb4(3, 6, 2), gs::rgb4(5, 8, 3), gs::rgb4(8, 10, 5), shadow};
    setPal(vdp, PAL_HUD, hud, 7);
    setPal(vdp, PAL_WOOD, wood, 9);
    setPal(vdp, PAL_YOU, you, 10);
    setPal(vdp, PAL_FOE, foe, 10);
    setPal(vdp, PAL_SHIELD, shield, 10);
    setPal(vdp, PAL_FX, fx, 4);
    setPal(vdp, PAL_BELL, bell, 7);
    setPal(vdp, PAL_ALERT, alert, 4);
    setPal(vdp, PAL_OK, ok, 4);
    setPal(vdp, PAL_FIELD, field, 5);

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    art.stake = gs::uploadMipped(vdp, stakeArt());
    art.gate = gs::uploadMipped(vdp, gateArt());
    art.hill = gs::uploadMipped(vdp, hillArt());
    art.skyband = gs::uploadMipped(vdp, skybandArt());
    art.spear = gs::uploadMipped(vdp, spearArt());
    art.you[0] = gs::uploadMipped(vdp, person(0, 0));
    art.you[1] = gs::uploadMipped(vdp, person(1, 0));
    art.climber[0] = gs::uploadMipped(vdp, person(0, 1));
    art.climber[1] = gs::uploadMipped(vdp, person(1, 1));
    art.runner[0] = gs::uploadMipped(vdp, person(0, 2));
    art.runner[1] = gs::uploadMipped(vdp, person(1, 2));
    art.shield[0] = gs::uploadMipped(vdp, person(0, 3));
    art.shield[1] = gs::uploadMipped(vdp, person(1, 3));
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.rope = gs::uploadMipped(vdp, ropeArt());
    art.spark = gs::uploadMipped(vdp, sparkArt());
    vdp.setFogColor(gs::rgb4(6, 7, 8));
}

}  // namespace palisade

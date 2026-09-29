#include "art.h"

#include <cstdint>
#include <string>

namespace trench {
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

gs::Bitmap climber(int step, bool helm, bool plate) {
    gs::Bitmap b(24, 36);
    b.ellipse(12, 6, 5, 5, helm ? 4 : 3);
    b.rect(9, 10, 6, 3, 5);
    b.rect(7, 13, 10, 10, plate ? 2 : 1);
    if (plate) b.rect(8, 14, 8, 7, 6);
    b.rect(step ? 4.f : 7.f, 14, 3, 8, 7);
    b.rect(step ? 16.f : 13.f, 14, 3, 8, 7);
    b.rect(step ? 8.f : 10.f, 23, 3, 9, 1);
    b.rect(step ? 13.f : 11.f, 23, 3, 9, 2);
    b.rect(5, 31, 5, 3, 8);
    b.rect(14, 31, 5, 3, 8);
    b.rect(16, 16, 7, 2, 9);
    b.outline(10, false);
    return b;
}

gs::Bitmap soldierArt() {
    gs::Bitmap b(28, 32);
    b.ellipse(12, 7, 6, 5, 1);
    b.rect(7, 11, 10, 3, 2);
    b.rect(6, 14, 12, 10, 3);
    b.rect(8, 24, 4, 7, 4);
    b.rect(13, 24, 4, 7, 4);
    b.rect(16, 16, 10, 3, 5);
    b.rect(22, 15, 4, 2, 6);
    b.outline(7, false);
    return b;
}

gs::Bitmap bagsArt() {
    gs::Bitmap b(96, 28);
    for (int i = 0; i < 4; i++) {
        float x = 4.f + float(i) * 22.f;
        b.ellipse(x + 10, 16, 12, 8, (i & 1) ? 2 : 3);
        b.rect(x + 2, 12, 16, 3, 4);
    }
    b.rect(0, 22, 96, 5, 1);
    b.outline(5, false);
    return b;
}

gs::Bitmap boardsArt() {
    gs::Bitmap b(160, 16);
    b.rect(0, 2, 160, 12, 1);
    for (int x = 4; x < 156; x += 12) b.rect(float(x), 3, 8, 10, 2);
    for (int x = 0; x < 160; x += 32) b.rect(float(x), 0, 3, 16, 3);
    return b;
}

gs::Bitmap wireArt() {
    gs::Bitmap b(40, 22);
    b.rect(18, 4, 3, 16, 1);
    b.line(2, 8, 36, 6, 2, 1);
    b.line(2, 14, 36, 12, 2, 1);
    b.line(6, 6, 12, 16, 3, 1);
    b.line(16, 5, 22, 16, 3, 1);
    b.line(26, 5, 32, 15, 3, 1);
    return b;
}

gs::Bitmap rifleArt() {
    gs::Bitmap b(48, 14);
    b.rect(4, 5, 36, 4, 1);
    b.rect(0, 6, 8, 2, 2);
    b.rect(28, 8, 8, 5, 3);
    b.rect(34, 3, 3, 4, 4);
    return b;
}

gs::Bitmap sightArt() {
    gs::Bitmap b(15, 15);
    b.rect(0, 0, 4, 2, 1);
    b.rect(0, 0, 2, 4, 1);
    b.rect(11, 0, 4, 2, 1);
    b.rect(13, 0, 2, 4, 1);
    b.rect(0, 13, 4, 2, 1);
    b.rect(0, 11, 2, 4, 1);
    b.rect(11, 13, 4, 2, 1);
    b.rect(13, 11, 2, 4, 1);
    b.rect(7, 6, 2, 3, 2);
    return b;
}

gs::Bitmap flashArt() {
    gs::Bitmap b(14, 14);
    b.ellipse(7, 7, 6, 6, 1);
    b.ellipse(7, 7, 3, 3, 2);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(22, 26);
    b.rect(9, 0, 4, 5, 3);
    b.ellipse(11, 14, 9, 8, 1);
    b.ellipse(11, 16, 5, 4, 2);
    b.rect(9, 21, 4, 4, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap stakeArt() {
    gs::Bitmap b(8, 40);
    b.rect(3, 0, 2, 36, 1);
    b.rect(1, 34, 6, 4, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shadow = gs::rgb4(1, 1, 1);
    const uint16_t hud[] = {0, gs::rgb4(14, 13, 10), gs::rgb4(7, 7, 6), gs::rgb4(15, 12, 5), gs::rgb4(15, 6, 3),
                            gs::rgb4(9, 14, 6), shadow};
    const uint16_t bag[] = {0, gs::rgb4(3, 3, 2), gs::rgb4(6, 5, 3), gs::rgb4(8, 7, 4), gs::rgb4(10, 8, 5),
                            gs::rgb4(2, 2, 1), shadow};
    const uint16_t you[] = {0, gs::rgb4(5, 6, 4), gs::rgb4(8, 9, 6), gs::rgb4(4, 5, 3), gs::rgb4(3, 3, 2),
                            gs::rgb4(7, 6, 4), gs::rgb4(12, 11, 8), gs::rgb4(1, 1, 1), shadow};
    const uint16_t foe[] = {0,
                            gs::rgb4(4, 5, 3),
                            gs::rgb4(6, 5, 3),
                            gs::rgb4(9, 8, 6),
                            gs::rgb4(5, 6, 4),
                            gs::rgb4(7, 6, 4),
                            gs::rgb4(3, 3, 2),
                            gs::rgb4(4, 4, 3),
                            gs::rgb4(2, 2, 1),
                            gs::rgb4(8, 6, 3),
                            gs::rgb4(1, 1, 1),
                            shadow};
    const uint16_t plate[] = {0,
                              gs::rgb4(4, 5, 5),
                              gs::rgb4(7, 8, 8),
                              gs::rgb4(9, 8, 6),
                              gs::rgb4(5, 6, 6),
                              gs::rgb4(8, 7, 5),
                              gs::rgb4(10, 11, 11),
                              gs::rgb4(3, 3, 4),
                              gs::rgb4(2, 2, 2),
                              gs::rgb4(12, 4, 3),
                              gs::rgb4(1, 1, 1),
                              shadow};
    const uint16_t fx[] = {0, gs::rgb4(13, 12, 8), gs::rgb4(15, 15, 11), gs::rgb4(15, 11, 3), shadow};
    const uint16_t bell[] = {0, gs::rgb4(13, 10, 3), gs::rgb4(8, 6, 2), gs::rgb4(5, 4, 2), gs::rgb4(15, 13, 6),
                             gs::rgb4(3, 2, 1), shadow};
    const uint16_t alert[] = {0, gs::rgb4(15, 4, 3), gs::rgb4(8, 1, 1), shadow};
    const uint16_t ok[] = {0, gs::rgb4(8, 15, 7), gs::rgb4(2, 6, 3), shadow};
    setPal(vdp, PAL_HUD, hud, 7);
    setPal(vdp, PAL_BAG, bag, 7);
    setPal(vdp, PAL_YOU, you, 9);
    setPal(vdp, PAL_FOE, foe, 12);
    setPal(vdp, PAL_PLATE, plate, 12);
    setPal(vdp, PAL_FX, fx, 5);
    setPal(vdp, PAL_BELL, bell, 7);
    setPal(vdp, PAL_ALERT, alert, 4);
    setPal(vdp, PAL_OK, ok, 4);
    vdp.setFogColor(gs::rgb4(2, 2, 2));

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    art.bags = gs::uploadMipped(vdp, bagsArt());
    art.boards = gs::uploadMipped(vdp, boardsArt());
    art.wire = gs::uploadMipped(vdp, wireArt());
    art.rifle = gs::uploadMipped(vdp, rifleArt());
    art.sight = gs::uploadMipped(vdp, sightArt());
    art.flash = gs::uploadMipped(vdp, flashArt());
    art.soldier = gs::uploadMipped(vdp, soldierArt());
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.stake = gs::uploadMipped(vdp, stakeArt());
    for (int s = 0; s < 2; s++) {
        art.climber[s] = gs::uploadMipped(vdp, climber(s, false, false));
        art.runner[s] = gs::uploadMipped(vdp, climber(s, true, false));
        art.shield[s] = gs::uploadMipped(vdp, climber(s, true, true));
    }
}

}  // namespace trench

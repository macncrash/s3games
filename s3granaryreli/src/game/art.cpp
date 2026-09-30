#include "art.h"

#include <cstdint>
#include <string>

namespace granary {
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
    gs::Bitmap b(26, 40);
    b.ellipse(13, 7, 6, 6, 4);
    b.rect(8, 12, 10, 4, 5);
    b.rect(7, 16, 12, 12, 2);
    b.rect(8, 17, 10, 4, 6);
    b.rect(9, 28, 3, 9, 1);
    b.rect(14, 28, 3, 9, 1);
    b.rect(7, 36, 5, 3, 8);
    b.rect(14, 36, 5, 3, 8);
    b.line(18, 18, 25, 8, 7, 2);
    b.rect(22, 6, 4, 3, 9);
    b.outline(10, false);
    return b;
}

gs::Bitmap person(int step, bool fast, bool wagon) {
    gs::Bitmap b(wagon ? 36 : 24, wagon ? 28 : 36);
    if (wagon) {
        b.rect(2, 10, 28, 12, 2);
        b.rect(4, 12, 24, 6, 6);
        b.ellipse(8, 22, 5, 5, 1);
        b.ellipse(24, 22, 5, 5, 1);
        b.rect(26, 6, 8, 8, 3);
        b.rect(28, 8, 4, 4, 4);
        b.outline(8, false);
        return b;
    }
    b.ellipse(12, 6, 5, 5, fast ? 3 : 4);
    b.rect(8, 11, 8, 3, 5);
    b.rect(6, 14, 12, 10, fast ? 6 : 1);
    int lx = step ? 5 : 8;
    int rx = step ? 13 : 10;
    b.rect(float(lx), 24, 3, 8, 2);
    b.rect(float(rx), 24, 3, 8, 2);
    b.rect(4, 31, 5, 2, 7);
    b.rect(14, 31, 5, 2, 7);
    b.rect(16, 16, 6, 2, 9);
    b.outline(8, false);
    return b;
}

gs::Bitmap barnArt() {
    gs::Bitmap b(200, 110);
    b.poly({{20, 40}, {100, 4}, {180, 40}}, 3);
    b.rect(24, 38, 152, 68, 2);
    b.rect(28, 42, 144, 60, 4);
    for (int i = 0; i < 3; i++) {
        float x = 36.f + float(i) * 46.f;
        b.rect(x, 52, 34, 46, 1);
        b.rect(x + 2, 54, 30, 42, 5);
        b.rect(x + 14, 70, 4, 6, 6);
    }
    b.rect(86, 44, 28, 10, 7);
    b.rect(90, 18, 6, 16, 8);
    b.outline(9, false);
    return b;
}

gs::Bitmap siloArt() {
    gs::Bitmap b(36, 96);
    b.ellipse(18, 14, 14, 10, 3);
    b.rect(4, 14, 28, 74, 2);
    b.rect(6, 18, 24, 66, 4);
    for (int y = 24; y < 80; y += 10) b.rect(6, float(y), 24, 2, 1);
    b.rect(14, 40, 8, 12, 5);
    b.outline(6, false);
    return b;
}

gs::Bitmap sackArt() {
    gs::Bitmap b(18, 16);
    b.ellipse(9, 9, 8, 6, 1);
    b.rect(6, 2, 6, 4, 2);
    b.line(4, 8, 14, 8, 3, 1);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(24, 28);
    b.rect(10, 0, 4, 5, 3);
    b.ellipse(12, 15, 10, 9, 1);
    b.ellipse(12, 17, 5, 5, 2);
    b.rect(10, 22, 4, 4, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap ropeArt() {
    gs::Bitmap b(6, 40);
    b.rect(2, 0, 2, 34, 1);
    b.ellipse(3, 36, 2, 3, 2);
    return b;
}

gs::Bitmap forkArt() {
    gs::Bitmap b(48, 16);
    b.rect(0, 6, 36, 3, 1);
    b.rect(34, 2, 2, 12, 2);
    b.rect(38, 2, 2, 12, 2);
    b.rect(42, 2, 2, 12, 2);
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

gs::Bitmap puffArt() {
    gs::Bitmap b(14, 14);
    b.ellipse(7, 7, 6, 6, 1);
    b.ellipse(7, 7, 3, 3, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shadow = gs::rgb4(2, 1, 0);
    const uint16_t hud[] = {0, gs::rgb4(15, 14, 10), gs::rgb4(8, 6, 3), gs::rgb4(15, 12, 4), gs::rgb4(15, 5, 2),
                            gs::rgb4(10, 14, 5), shadow};
    const uint16_t barn[] = {0,
                             gs::rgb4(3, 2, 1),
                             gs::rgb4(8, 5, 2),
                             gs::rgb4(10, 4, 2),
                             gs::rgb4(12, 8, 3),
                             gs::rgb4(5, 3, 1),
                             gs::rgb4(14, 12, 6),
                             gs::rgb4(6, 7, 8),
                             gs::rgb4(4, 4, 4),
                             gs::rgb4(1, 1, 1),
                             shadow};
    const uint16_t you[] = {0, gs::rgb4(6, 4, 2), gs::rgb4(10, 7, 3), gs::rgb4(4, 3, 2), gs::rgb4(13, 9, 5),
                            gs::rgb4(7, 5, 2), gs::rgb4(14, 12, 7), gs::rgb4(9, 8, 5), gs::rgb4(2, 2, 1),
                            gs::rgb4(12, 11, 8), gs::rgb4(1, 1, 1), shadow};
    const uint16_t foe[] = {0, gs::rgb4(5, 3, 2), gs::rgb4(7, 5, 3), gs::rgb4(3, 2, 2), gs::rgb4(9, 7, 5),
                            gs::rgb4(4, 3, 2), gs::rgb4(8, 3, 2), gs::rgb4(2, 2, 1), gs::rgb4(1, 1, 1),
                            gs::rgb4(11, 10, 8), shadow};
    const uint16_t cart[] = {0, gs::rgb4(3, 3, 3), gs::rgb4(8, 6, 3), gs::rgb4(6, 4, 2), gs::rgb4(12, 10, 6),
                             gs::rgb4(10, 4, 2), gs::rgb4(14, 12, 5), gs::rgb4(2, 2, 1), gs::rgb4(1, 1, 1), shadow};
    const uint16_t fx[] = {0, gs::rgb4(14, 12, 6), gs::rgb4(15, 15, 10), gs::rgb4(15, 10, 3), shadow};
    const uint16_t bell[] = {0, gs::rgb4(13, 10, 3), gs::rgb4(8, 6, 2), gs::rgb4(5, 4, 2), gs::rgb4(15, 13, 6),
                             gs::rgb4(3, 2, 1), shadow};
    const uint16_t alert[] = {0, gs::rgb4(15, 4, 2), gs::rgb4(8, 1, 1), shadow};
    const uint16_t ok[] = {0, gs::rgb4(10, 15, 5), gs::rgb4(3, 6, 1), shadow};
    const uint16_t road[] = {0,
                             gs::rgb4(6, 8, 2),
                             gs::rgb4(4, 6, 1),
                             gs::rgb4(8, 9, 3),
                             gs::rgb4(7, 6, 2),
                             gs::rgb4(5, 4, 1),
                             gs::rgb4(8, 6, 2),
                             gs::rgb4(6, 4, 1),
                             gs::rgb4(9, 8, 4),
                             gs::rgb4(5, 4, 2),
                             gs::rgb4(7, 5, 2),
                             gs::rgb4(3, 5, 8),
                             gs::rgb4(4, 6, 9),
                             gs::rgb4(8, 10, 12),
                             gs::rgb4(12, 10, 4),
                             gs::rgb4(10, 8, 3)};
    setPal(vdp, PAL_HUD, hud, 7);
    setPal(vdp, PAL_BARN, barn, 11);
    setPal(vdp, PAL_YOU, you, 12);
    setPal(vdp, PAL_FOE, foe, 11);
    setPal(vdp, PAL_CART, cart, 10);
    setPal(vdp, PAL_FX, fx, 5);
    setPal(vdp, PAL_BELL, bell, 7);
    setPal(vdp, PAL_ALERT, alert, 4);
    setPal(vdp, PAL_OK, ok, 4);
    setPal(vdp, PAL_ROAD, road, 16);
    vdp.setFogColor(gs::rgb4(6, 5, 3));

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    art.barn = gs::uploadMipped(vdp, barnArt());
    art.silo = gs::uploadMipped(vdp, siloArt());
    art.sack = gs::uploadMipped(vdp, sackArt());
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.rope = gs::uploadMipped(vdp, ropeArt());
    art.fork = gs::uploadMipped(vdp, forkArt());
    art.sight = gs::uploadMipped(vdp, sightArt());
    art.puff = gs::uploadMipped(vdp, puffArt());
    art.keeper = gs::uploadMipped(vdp, keeperArt());
    for (int s = 0; s < 2; s++) {
        art.thief[s] = gs::uploadMipped(vdp, person(s, false, false));
        art.runner[s] = gs::uploadMipped(vdp, person(s, true, false));
        art.wagon[s] = gs::uploadMipped(vdp, person(s, false, true));
    }
}

}  // namespace granary

#include "art.h"

#include <cstdint>

namespace bunker {
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

gs::Bitmap man(int step, bool helm, bool plate) {
    gs::Bitmap b(28, 40);
    b.ellipse(14, 7, 6, 6, helm ? 4 : 3);
    b.rect(11, 12, 6, 4, 5);
    b.rect(8, 16, 12, 12, plate ? 2 : 1);
    if (plate) b.rect(9, 17, 10, 8, 6);
    int lx = step ? 6 : 9;
    int rx = step ? 16 : 13;
    b.rect(float(lx), 16, 4, 10, 7);
    b.rect(float(rx), 16, 4, 10, 7);
    b.rect(step ? 9.f : 12.f, 28, 4, 10, 1);
    b.rect(step ? 15.f : 12.f, 28, 4, 10, 2);
    b.rect(6, 36, 6, 3, 8);
    b.rect(16, 36, 6, 3, 8);
    b.rect(18, 20, 8, 2, 9);
    b.outline(10, false);
    return b;
}

gs::Bitmap doorArt() {
    gs::Bitmap b(180, 168);
    b.rect(0, 0, 180, 168, 2);
    b.rect(6, 6, 168, 156, 3);
    b.rect(10, 10, 160, 148, 4);
    for (int y = 14; y < 154; y += 8) b.rect(12, float(y), 156, 1, 1);
    b.rect(18, 40, 144, 56, 0);
    b.rect(18, 40, 144, 3, 5);
    b.rect(18, 93, 144, 3, 5);
    b.rect(18, 40, 3, 56, 5);
    b.rect(159, 40, 3, 56, 5);
    b.rect(24, 118, 132, 10, 6);
    b.rect(28, 120, 124, 3, 7);
    b.rect(70, 108, 40, 8, 8);
    b.outline(9, false);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(28, 30);
    b.rect(12, 0, 4, 6, 3);
    b.ellipse(14, 16, 12, 10, 1);
    b.ellipse(14, 18, 7, 6, 2);
    b.rect(12, 24, 4, 4, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap ropeArt() {
    gs::Bitmap b(8, 48);
    b.rect(3, 0, 2, 40, 1);
    b.ellipse(4, 42, 3, 4, 2);
    return b;
}

gs::Bitmap rifleArt() {
    gs::Bitmap b(90, 36);
    b.rect(40, 8, 10, 18, 2);
    b.rect(36, 14, 18, 4, 3);
    b.rect(20, 16, 50, 5, 1);
    b.rect(8, 17, 14, 3, 4);
    b.rect(44, 20, 12, 12, 5);
    b.outline(6, false);
    return b;
}

gs::Bitmap sightArt() {
    gs::Bitmap b(17, 17);
    b.rect(0, 0, 5, 2, 1);
    b.rect(0, 0, 2, 5, 1);
    b.rect(12, 0, 5, 2, 1);
    b.rect(15, 0, 2, 5, 1);
    b.rect(0, 15, 5, 2, 1);
    b.rect(0, 12, 2, 5, 1);
    b.rect(12, 15, 5, 2, 1);
    b.rect(15, 12, 2, 5, 1);
    b.rect(8, 7, 2, 3, 2);
    return b;
}

gs::Bitmap flashArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 7, 7, 1);
    b.ellipse(8, 8, 3, 3, 2);
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(10, 10);
    b.ellipse(5, 5, 4, 4, 1);
    return b;
}

gs::Bitmap helmArt() {
    gs::Bitmap b(22, 16);
    b.ellipse(11, 8, 10, 7, 1);
    b.rect(4, 8, 14, 4, 2);
    b.rect(8, 6, 6, 3, 3);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    const uint16_t hud[] = {0, gs::rgb4(14, 14, 12), gs::rgb4(8, 8, 7), gs::rgb4(15, 13, 6), gs::rgb4(15, 5, 3),
                            gs::rgb4(8, 14, 7), shadow};
    const uint16_t door[] = {0,
                             gs::rgb4(2, 2, 3),
                             gs::rgb4(4, 4, 5),
                             gs::rgb4(6, 6, 7),
                             gs::rgb4(8, 8, 9),
                             gs::rgb4(3, 3, 4),
                             gs::rgb4(5, 4, 3),
                             gs::rgb4(10, 8, 4),
                             gs::rgb4(12, 10, 3),
                             gs::rgb4(1, 1, 1),
                             shadow};
    const uint16_t you[] = {0, gs::rgb4(5, 5, 4), gs::rgb4(9, 9, 8), gs::rgb4(3, 3, 3), gs::rgb4(12, 11, 8),
                            gs::rgb4(7, 5, 3), gs::rgb4(1, 1, 1), shadow};
    const uint16_t foe[] = {0,
                            gs::rgb4(3, 4, 3),
                            gs::rgb4(5, 6, 4),
                            gs::rgb4(10, 8, 6),
                            gs::rgb4(6, 7, 5),
                            gs::rgb4(8, 6, 4),
                            gs::rgb4(2, 2, 2),
                            gs::rgb4(4, 4, 3),
                            gs::rgb4(2, 2, 1),
                            gs::rgb4(7, 6, 3),
                            gs::rgb4(1, 1, 1),
                            shadow};
    const uint16_t plate[] = {0,
                              gs::rgb4(4, 5, 6),
                              gs::rgb4(7, 8, 9),
                              gs::rgb4(10, 8, 6),
                              gs::rgb4(5, 6, 7),
                              gs::rgb4(8, 7, 5),
                              gs::rgb4(9, 10, 11),
                              gs::rgb4(3, 3, 4),
                              gs::rgb4(2, 2, 2),
                              gs::rgb4(12, 4, 3),
                              gs::rgb4(1, 1, 1),
                              shadow};
    const uint16_t fx[] = {0, gs::rgb4(12, 11, 8), gs::rgb4(15, 15, 12), gs::rgb4(15, 12, 4), shadow};
    const uint16_t bell[] = {0, gs::rgb4(12, 9, 3), gs::rgb4(8, 6, 2), gs::rgb4(6, 5, 3), gs::rgb4(14, 12, 6),
                             gs::rgb4(3, 2, 1), shadow};
    const uint16_t alert[] = {0, gs::rgb4(15, 4, 3), gs::rgb4(8, 1, 1), shadow};
    const uint16_t ok[] = {0, gs::rgb4(8, 15, 7), gs::rgb4(2, 6, 3), shadow};
    setPal(vdp, PAL_HUD, hud, 7);
    setPal(vdp, PAL_DOOR, door, 11);
    setPal(vdp, PAL_YOU, you, 7);
    setPal(vdp, PAL_FOE, foe, 12);
    setPal(vdp, PAL_PLATE, plate, 12);
    setPal(vdp, PAL_FX, fx, 5);
    setPal(vdp, PAL_BELL, bell, 7);
    setPal(vdp, PAL_ALERT, alert, 4);
    setPal(vdp, PAL_OK, ok, 4);
    vdp.setFogColor(gs::rgb4(1, 1, 2));

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    art.door = gs::uploadMipped(vdp, doorArt());
    art.bar = art.door;
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.rope = gs::uploadMipped(vdp, ropeArt());
    art.rifle = gs::uploadMipped(vdp, rifleArt());
    art.sight = gs::uploadMipped(vdp, sightArt());
    art.flash = gs::uploadMipped(vdp, flashArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.helm = gs::uploadMipped(vdp, helmArt());
    for (int s = 0; s < 2; s++) {
        art.walker[s] = gs::uploadMipped(vdp, man(s, false, false));
        art.sprinter[s] = gs::uploadMipped(vdp, man(s, true, false));
        art.plate[s] = gs::uploadMipped(vdp, man(s, true, true));
    }
}

}  // namespace bunker

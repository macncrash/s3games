#include "art.h"

#include <cstdint>

namespace quarry {
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

gs::Bitmap climberArt(int step) {
    gs::Bitmap b(24, 36);
    b.ellipse(12, 6, 5, 5, 3);
    b.rect(9, 11, 6, 3, 4);
    b.rect(7, 14, 10, 10, 1);
    b.rect(step ? 4.f : 8.f, 16, 3, 8, 2);
    b.rect(step ? 16.f : 13.f, 16, 3, 8, 2);
    b.rect(step ? 8.f : 11.f, 24, 3, 9, 5);
    b.rect(step ? 13.f : 10.f, 24, 3, 9, 6);
    b.rect(6, 32, 5, 3, 7);
    b.rect(13, 32, 5, 3, 7);
    b.outline(8, false);
    return b;
}

gs::Bitmap cartArt(int step) {
    gs::Bitmap b(32, 28);
    b.rect(4, 6, 24, 12, 1);
    b.rect(6, 8, 20, 6, 2);
    b.rect(2, 4, 6, 4, 3);
    b.ellipse(9, 20, 4, 4, 4);
    b.ellipse(23, 20, 4, 4, 4);
    b.ellipse(9, 20, 2, 2, step ? 5 : 6);
    b.ellipse(23, 20, 2, 2, step ? 6 : 5);
    b.outline(7, false);
    return b;
}

gs::Bitmap slabArt(int step) {
    gs::Bitmap b(30, 26);
    b.poly({{4, 18}, {10, 6}, {22, 4}, {27, 12}, {24, 22}, {8, 23}}, 1);
    b.poly({{10, 12}, {16, 8}, {20, 14}, {12, 16}}, step ? 2 : 3);
    b.rect(8, 18, 6, 3, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap faceArt() {
    gs::Bitmap b(200, 150);
    b.rect(0, 0, 200, 150, 2);
    for (int i = 0; i < 6; i++) {
        int y = 8 + i * 22;
        int inset = i * 6;
        b.rect(float(inset), float(y), float(200 - inset * 2), 16, (i & 1) ? 3 : 4);
        b.rect(float(inset), float(y + 14), float(200 - inset * 2), 3, 1);
        b.rect(float(18 + (i * 17) % 40), float(y + 3), 10, 6, 5);
    }
    b.rect(70, 118, 60, 18, 6);
    b.rect(78, 122, 44, 8, 7);
    b.outline(8, false);
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

gs::Bitmap craneArt() {
    gs::Bitmap b(70, 18);
    b.rect(0, 6, 64, 4, 1);
    b.rect(56, 2, 6, 14, 2);
    b.rect(8, 10, 3, 7, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap cageArt() {
    gs::Bitmap b(18, 28);
    b.rect(2, 2, 14, 22, 1);
    b.rect(4, 4, 10, 18, 0);
    b.rect(2, 2, 14, 2, 2);
    b.rect(8, 0, 2, 4, 3);
    b.line(4, 6, 14, 20, 2, 1);
    b.line(14, 6, 4, 20, 2, 1);
    return b;
}

gs::Bitmap workerArt() {
    gs::Bitmap b(22, 28);
    b.ellipse(11, 6, 5, 5, 3);
    b.rect(7, 11, 8, 9, 1);
    b.rect(8, 13, 6, 4, 2);
    b.rect(6, 19, 3, 7, 4);
    b.rect(13, 19, 3, 7, 4);
    b.rect(4, 25, 5, 2, 5);
    b.rect(13, 25, 5, 2, 5);
    return b;
}

gs::Bitmap pickArt() {
    gs::Bitmap b(28, 16);
    b.line(2, 14, 22, 4, 1, 2);
    b.rect(18, 1, 8, 3, 2);
    b.rect(20, 4, 3, 5, 3);
    return b;
}

gs::Bitmap flashArt() {
    gs::Bitmap b(14, 14);
    b.ellipse(7, 7, 6, 6, 1);
    b.ellipse(7, 7, 3, 3, 2);
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(10, 10);
    b.ellipse(5, 5, 4, 3, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shadow = gs::rgb4(1, 1, 1);
    const uint16_t hud[] = {0, gs::rgb4(14, 13, 10), gs::rgb4(8, 7, 5), gs::rgb4(15, 12, 4), gs::rgb4(15, 5, 3),
                            gs::rgb4(8, 14, 6), shadow};
    const uint16_t face[] = {0,
                             gs::rgb4(3, 2, 2),
                             gs::rgb4(6, 5, 4),
                             gs::rgb4(8, 6, 4),
                             gs::rgb4(10, 8, 5),
                             gs::rgb4(5, 4, 3),
                             gs::rgb4(2, 2, 2),
                             gs::rgb4(1, 1, 2),
                             shadow};
    const uint16_t you[] = {0, gs::rgb4(7, 6, 3), gs::rgb4(12, 9, 3), gs::rgb4(11, 8, 6), gs::rgb4(4, 4, 3),
                            gs::rgb4(2, 2, 2), shadow};
    const uint16_t foe[] = {0,         gs::rgb4(4, 5, 3), gs::rgb4(6, 7, 4), gs::rgb4(10, 8, 6), gs::rgb4(3, 3, 2),
                            gs::rgb4(5, 4, 3), gs::rgb4(7, 6, 4), gs::rgb4(2, 2, 1), shadow};
    const uint16_t slab[] = {0, gs::rgb4(7, 7, 6), gs::rgb4(10, 10, 8), gs::rgb4(5, 5, 4), gs::rgb4(3, 3, 3), shadow};
    const uint16_t fx[] = {0, gs::rgb4(15, 14, 6), gs::rgb4(15, 8, 2), gs::rgb4(8, 7, 5)};
    const uint16_t bell[] = {0, gs::rgb4(13, 11, 4), gs::rgb4(8, 6, 2), gs::rgb4(4, 4, 3), gs::rgb4(15, 14, 8),
                             shadow};
    const uint16_t alert[] = {0, gs::rgb4(15, 6, 4), gs::rgb4(8, 2, 2)};
    const uint16_t ok[] = {0, gs::rgb4(8, 14, 6), gs::rgb4(4, 8, 3)};
    const uint16_t road[] = {0,
                             gs::rgb4(4, 5, 2),
                             gs::rgb4(3, 4, 2),
                             gs::rgb4(5, 5, 3),
                             gs::rgb4(6, 5, 3),
                             gs::rgb4(5, 4, 2),
                             gs::rgb4(7, 6, 4),
                             gs::rgb4(5, 4, 3),
                             gs::rgb4(9, 8, 6),
                             gs::rgb4(6, 5, 4),
                             gs::rgb4(8, 7, 5),
                             gs::rgb4(3, 4, 5),
                             gs::rgb4(4, 5, 6),
                             gs::rgb4(2, 3, 4),
                             gs::rgb4(12, 11, 7),
                             gs::rgb4(11, 10, 8)};
    setPal(vdp, PAL_HUD, hud, 7);
    setPal(vdp, PAL_FACE, face, 9);
    setPal(vdp, PAL_YOU, you, 7);
    setPal(vdp, PAL_FOE, foe, 9);
    setPal(vdp, PAL_SLAB, slab, 6);
    setPal(vdp, PAL_FX, fx, 4);
    setPal(vdp, PAL_BELL, bell, 6);
    setPal(vdp, PAL_ALERT, alert, 3);
    setPal(vdp, PAL_OK, ok, 3);
    setPal(vdp, PAL_ROAD, road, 16);

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    art.face = gs::uploadMipped(vdp, faceArt());
    art.crane = gs::uploadMipped(vdp, craneArt());
    art.cage = gs::uploadMipped(vdp, cageArt());
    art.pick = gs::uploadMipped(vdp, pickArt());
    art.flash = gs::uploadMipped(vdp, flashArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.climber[0] = gs::uploadMipped(vdp, climberArt(0));
    art.climber[1] = gs::uploadMipped(vdp, climberArt(1));
    art.cart[0] = gs::uploadMipped(vdp, cartArt(0));
    art.cart[1] = gs::uploadMipped(vdp, cartArt(1));
    art.slab[0] = gs::uploadMipped(vdp, slabArt(0));
    art.slab[1] = gs::uploadMipped(vdp, slabArt(1));
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.worker = gs::uploadMipped(vdp, workerArt());
    vdp.setFogColor(gs::rgb4(6, 6, 7));
}

}  // namespace quarry

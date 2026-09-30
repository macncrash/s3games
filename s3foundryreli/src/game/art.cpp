#include "art.h"

#include <cstdint>

namespace foundry {
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

gs::Bitmap pourerArt(int step) {
    gs::Bitmap b(32, 44);
    b.ellipse(16, 7, 6, 6, 4);
    b.rect(12, 12, 8, 4, 5);
    b.rect(8, 16, 16, 14, 2);
    b.rect(10, 18, 12, 6, 6);
    b.rect(step ? 4.f : 7.f, 18, 5, 10, 3);
    b.rect(step ? 23.f : 20.f, 18, 5, 10, 3);
    b.rect(step ? 10.f : 13.f, 30, 5, 10, 1);
    b.rect(step ? 17.f : 14.f, 30, 5, 10, 1);
    b.rect(8, 39, 6, 3, 8);
    b.rect(18, 39, 6, 3, 8);
    b.rect(22, 22, 8, 3, 7);
    b.outline(9, false);
    return b;
}

gs::Bitmap cinderArt(int step) {
    gs::Bitmap b(26, 32);
    b.ellipse(13, 14, 10, 12, 1);
    b.ellipse(13, 16, 6, 6, 2);
    b.rect(6, 4, 3, 6, 3);
    b.rect(17, 4, 3, 6, 3);
    b.rect(step ? 5.f : 8.f, 24, 5, 7, 4);
    b.rect(step ? 16.f : 13.f, 24, 5, 7, 4);
    b.rect(9, 12, 2, 2, 5);
    b.rect(15, 12, 2, 2, 5);
    b.outline(6, false);
    return b;
}

gs::Bitmap slagArt(int step) {
    gs::Bitmap b(30, 28);
    b.ellipse(15, 12, 13, 9, 1);
    b.rect(4, 10, 6, 5, 2);
    b.rect(20, 10, 6, 5, 2);
    b.rect(step ? 4.f : 7.f, 18, 6, 8, 3);
    b.rect(step ? 20.f : 17.f, 18, 6, 8, 3);
    b.rect(10, 8, 3, 3, 4);
    b.rect(17, 8, 3, 3, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap ingotArt(int step) {
    gs::Bitmap b(36, 28);
    b.poly({{4, 16}, {18, 6}, {32, 16}, {18, 24}}, 1);
    b.rect(8, 14, 20, 6, 2);
    b.rect(step ? 6.f : 10.f, 20, 6, 6, 3);
    b.rect(step ? 24.f : 20.f, 20, 6, 6, 3);
    b.rect(14, 10, 8, 3, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap hallArt() {
    gs::Bitmap b(220, 180);
    b.rect(0, 0, 220, 180, 2);
    b.rect(8, 8, 204, 164, 3);
    for (int x = 12; x < 208; x += 14) b.rect(float(x), 10, 2, 150, 1);
    for (int y = 14; y < 156; y += 12) b.rect(10, float(y), 200, 2, 4);
    b.rect(36, 48, 148, 70, 0);
    b.rect(36, 48, 148, 6, 5);
    b.rect(36, 112, 148, 6, 5);
    b.rect(36, 48, 6, 70, 5);
    b.rect(178, 48, 6, 70, 5);
    b.ellipse(110, 150, 40, 12, 6);
    b.ellipse(110, 148, 22, 6, 7);
    b.rect(96, 128, 28, 16, 8);
    b.outline(9, false);
    return b;
}

gs::Bitmap crucibleArt() {
    gs::Bitmap b(70, 36);
    b.poly({{6, 4}, {64, 4}, {56, 30}, {14, 30}}, 1);
    b.rect(16, 8, 38, 10, 2);
    b.ellipse(35, 14, 12, 4, 3);
    b.rect(30, 26, 10, 8, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap ladleArt() {
    gs::Bitmap b(64, 20);
    b.rect(8, 8, 40, 4, 1);
    b.ellipse(50, 10, 10, 7, 2);
    b.ellipse(50, 10, 5, 3, 3);
    b.rect(2, 6, 8, 8, 4);
    b.outline(5, false);
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

gs::Bitmap flashArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 7, 7, 1);
    b.ellipse(8, 8, 3, 3, 2);
    return b;
}

gs::Bitmap sightArt() {
    gs::Bitmap b(17, 17);
    b.rect(0, 7, 5, 2, 1);
    b.rect(12, 7, 5, 2, 1);
    b.rect(7, 0, 2, 5, 1);
    b.rect(7, 12, 2, 5, 1);
    b.rect(8, 8, 1, 1, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    vdp.setFogColor(gs::rgb4(6, 2, 1));
    const uint16_t hud[] = {0, gs::rgb4(15, 13, 8), gs::rgb4(4, 2, 1)};
    const uint16_t hall[] = {
        0,
        gs::rgb4(3, 2, 2),
        gs::rgb4(5, 3, 2),
        gs::rgb4(7, 4, 3),
        gs::rgb4(4, 3, 3),
        gs::rgb4(12, 6, 2),
        gs::rgb4(8, 3, 1),
        gs::rgb4(15, 9, 2),
        gs::rgb4(10, 8, 6),
        gs::rgb4(1, 1, 1),
    };
    const uint16_t you[] = {
        0,
        gs::rgb4(3, 3, 4),
        gs::rgb4(8, 5, 3),
        gs::rgb4(12, 8, 4),
        gs::rgb4(14, 10, 7),
        gs::rgb4(6, 4, 3),
        gs::rgb4(11, 4, 2),
        gs::rgb4(14, 12, 4),
        gs::rgb4(2, 2, 2),
        gs::rgb4(1, 1, 1),
    };
    const uint16_t cinder[] = {
        0,
        gs::rgb4(12, 4, 1),
        gs::rgb4(15, 8, 1),
        gs::rgb4(15, 12, 3),
        gs::rgb4(6, 2, 1),
        gs::rgb4(15, 15, 6),
        gs::rgb4(2, 1, 1),
    };
    const uint16_t slag[] = {
        0,
        gs::rgb4(6, 6, 6),
        gs::rgb4(10, 8, 6),
        gs::rgb4(4, 4, 4),
        gs::rgb4(14, 6, 2),
        gs::rgb4(1, 1, 1),
    };
    const uint16_t fx[] = {0, gs::rgb4(15, 14, 8), gs::rgb4(15, 8, 2)};
    const uint16_t bell[] = {0, gs::rgb4(12, 10, 4), gs::rgb4(8, 6, 2), gs::rgb4(14, 12, 6), gs::rgb4(4, 3, 2), gs::rgb4(2, 2, 1)};
    const uint16_t alert[] = {0, gs::rgb4(15, 4, 3)};
    const uint16_t ok[] = {0, gs::rgb4(8, 15, 6)};
    const uint16_t glow[] = {0, gs::rgb4(15, 7, 1), gs::rgb4(15, 12, 3), gs::rgb4(14, 14, 6), gs::rgb4(8, 4, 2), gs::rgb4(2, 1, 1)};
    setPal(vdp, PAL_HUD, hud, 3);
    setPal(vdp, PAL_HALL, hall, 10);
    setPal(vdp, PAL_YOU, you, 10);
    setPal(vdp, PAL_CINDER, cinder, 7);
    setPal(vdp, PAL_SLAG, slag, 6);
    setPal(vdp, PAL_FX, fx, 3);
    setPal(vdp, PAL_BELL, bell, 6);
    setPal(vdp, PAL_ALERT, alert, 2);
    setPal(vdp, PAL_OK, ok, 2);
    setPal(vdp, PAL_GLOW, glow, 6);

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    art.hall = gs::uploadMipped(vdp, hallArt());
    art.crucible = gs::uploadMipped(vdp, crucibleArt());
    art.ladle = gs::uploadMipped(vdp, ladleArt());
    art.pourer[0] = gs::uploadMipped(vdp, pourerArt(0));
    art.pourer[1] = gs::uploadMipped(vdp, pourerArt(1));
    art.cinder[0] = gs::uploadMipped(vdp, cinderArt(0));
    art.cinder[1] = gs::uploadMipped(vdp, cinderArt(1));
    art.slag[0] = gs::uploadMipped(vdp, slagArt(0));
    art.slag[1] = gs::uploadMipped(vdp, slagArt(1));
    art.ingot[0] = gs::uploadMipped(vdp, ingotArt(0));
    art.ingot[1] = gs::uploadMipped(vdp, ingotArt(1));
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.rope = gs::uploadMipped(vdp, ropeArt());
    art.flash = gs::uploadMipped(vdp, flashArt());
    art.sight = gs::uploadMipped(vdp, sightArt());
}

}  // namespace foundry

#include "game/art.h"

#include <cstring>
#include <string>

namespace spanpouc {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

gs::Image phrase(gs::VDP& vdp, const std::string& s, int scale, int color) {
    gs::TextStyle st;
    st.scale = scale;
    st.color = color;
    st.spacing = 1;
    return gs::uploadImage(vdp, gs::textBitmap(s, st));
}

gs::Bitmap walkerArt() {
    gs::Bitmap b(48, 72);
    b.poly({{24, 14}, {40, 58}, {8, 58}}, 3);
    b.poly({{24, 18}, {34, 56}, {14, 56}}, 4);
    b.ellipse(24, 12, 8, 9, 5);
    b.ellipse(24, 12, 5, 6, 6);
    b.rect(15, 56, 6, 14, 2);
    b.rect(27, 56, 6, 14, 2);
    b.rect(12, 58, 8, 3, 7);
    b.rect(28, 58, 8, 3, 7);
    b.rect(6, 28, 7, 16, 3);
    b.rect(36, 26, 6, 14, 4);
    b.outline(1, false);
    return b;
}

gs::Bitmap pouchArt() {
    gs::Bitmap b(36, 28);
    b.ellipse(18, 16, 15, 10, 3);
    b.ellipse(18, 16, 11, 7, 4);
    b.rect(6, 5, 24, 6, 5);
    b.rect(8, 7, 20, 2, 2);
    b.ellipse(18, 16, 3, 3, 6);
    b.line(18, 6, 18, 13, 2, 1.5f);
    b.outline(1, false);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(14, 56);
    b.rect(4, 0, 6, 56, 3);
    b.rect(5, 0, 2, 56, 4);
    b.rect(2, 6, 10, 3, 5);
    b.rect(2, 24, 10, 3, 5);
    b.rect(2, 42, 10, 3, 5);
    b.outline(1, false);
    return b;
}

gs::Bitmap gateArt() {
    gs::Bitmap b(96, 64);
    b.rect(4, 8, 12, 56, 3);
    b.rect(80, 8, 12, 56, 3);
    b.rect(4, 8, 88, 14, 4);
    b.rect(8, 12, 80, 6, 5);
    b.rect(28, 22, 40, 10, 2);
    b.outline(1, false);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(32, 32);
    b.ellipse(16, 16, 10, 10, 3);
    b.ellipse(16, 16, 6, 6, 4);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(14, 10, 4), gs::rgb4(6, 8, 10), gs::rgb4(15, 4, 3)});
    setPal(vdp, PAL_WALKER,
           {0, gs::rgb4(2, 2, 3), gs::rgb4(3, 3, 4), gs::rgb4(5, 6, 8), gs::rgb4(8, 9, 11), gs::rgb4(12, 9, 7),
            gs::rgb4(14, 12, 10), gs::rgb4(4, 3, 3)});
    setPal(vdp, PAL_POUCH,
           {0, gs::rgb4(3, 2, 1), gs::rgb4(6, 4, 2), gs::rgb4(10, 6, 3), gs::rgb4(13, 9, 4), gs::rgb4(8, 5, 2),
            gs::rgb4(15, 13, 6)});
    setPal(vdp, PAL_TIMBER,
           {0, gs::rgb4(2, 2, 2), gs::rgb4(4, 3, 2), gs::rgb4(7, 5, 3), gs::rgb4(10, 8, 5), gs::rgb4(13, 11, 7)});
    setPal(vdp, PAL_GATE,
           {0, gs::rgb4(1, 1, 2), gs::rgb4(9, 8, 6), gs::rgb4(6, 6, 6), gs::rgb4(8, 8, 8), gs::rgb4(12, 11, 8)});
    setPal(vdp, PAL_SUN, {0, gs::rgb4(8, 6, 3), gs::rgb4(12, 9, 4), gs::rgb4(15, 13, 7), gs::rgb4(15, 15, 12)});

    auto road = [&](int p, int idx, int r, int g, int b) { vdp.setColor(p * 16 + idx, gs::rgb4(r, g, b)); };
    road(PAL_ROAD, 1, 2, 5, 7);
    road(PAL_ROAD, 2, 3, 7, 9);
    road(PAL_ROAD, 3, 5, 9, 10);
    road(PAL_ROAD, 4, 4, 3, 2);
    road(PAL_ROAD, 5, 6, 5, 3);
    road(PAL_ROAD, 6, 7, 5, 3);
    road(PAL_ROAD, 7, 9, 7, 4);
    road(PAL_ROAD, 8, 12, 10, 6);
    road(PAL_ROAD, 9, 6, 4, 2);
    road(PAL_ROAD, 10, 8, 6, 3);
    road(PAL_ROAD, 11, 2, 6, 8);
    road(PAL_ROAD, 12, 3, 8, 10);
    road(PAL_ROAD, 13, 8, 12, 13);
    road(PAL_ROAD, 14, 11, 9, 5);
    road(PAL_ROAD, 15, 5, 4, 3);
    vdp.setFogColor(gs::rgb4(7, 9, 11));

    art.walker = gs::uploadImage(vdp, walkerArt());
    art.pouch = gs::uploadImage(vdp, pouchArt());
    art.post = gs::uploadImage(vdp, postArt());
    art.gate = gs::uploadImage(vdp, gateArt());
    art.sun = gs::uploadImage(vdp, sunArt());

    art.glyphH = 0;
    for (int ch = 32; ch < 127; ch++) {
        gs::TextStyle st;
        st.scale = 2;
        st.color = 1;
        st.spacing = 0;
        gs::Bitmap g = gs::textBitmap(std::string(1, char(ch)), st);
        art.glyphW[ch - 32] = g.w;
        art.glyphH = g.h;
        art.glyph[ch - 32] = gs::uploadImage(vdp, g);
    }
    art.title = phrase(vdp, "S3 SPAN POUC", 3, 1);
    art.line1 = phrase(vdp, "CARRY THE POUCH", 2, 2);
    art.line2 = phrase(vdp, "ACROSS THE SPAN", 2, 2);
    art.hint = phrase(vdp, "ARROWS KEEP THE DECK", 1, 1);
    art.win = phrase(vdp, "THE POUCH CROSSED", 2, 1);
    art.fell = phrase(vdp, "THE POUCH LEFT THE SPAN", 2, 4);
}

}  // namespace spanpouc

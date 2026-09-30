#include "art.h"

#include <cstdint>
#include <string>

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

gs::Bitmap cokeArt() {
    gs::Bitmap b(34, 26);
    b.rect(2, 8, 26, 10, 2);
    b.rect(6, 4, 16, 6, 3);
    b.rect(4, 18, 6, 6, 4);
    b.rect(20, 18, 6, 6, 4);
    b.rect(10, 10, 4, 3, 5);
    b.ellipse(28, 10, 4, 3, 6);
    b.outline(1, false);
    return b;
}

gs::Bitmap ladleArt() {
    gs::Bitmap b(40, 28);
    b.rect(2, 12, 22, 8, 2);
    b.ellipse(30, 14, 8, 6, 3);
    b.ellipse(30, 14, 4, 3, 4);
    b.rect(6, 20, 6, 6, 5);
    b.rect(16, 20, 6, 6, 5);
    b.rect(8, 6, 3, 8, 6);
    b.outline(1, false);
    return b;
}

gs::Bitmap slagArt() {
    gs::Bitmap b(42, 30);
    b.poly({{4, 16}, {14, 6}, {34, 6}, {38, 16}, {30, 22}, {8, 22}}, 2);
    b.rect(10, 8, 18, 6, 3);
    b.rect(6, 22, 7, 6, 4);
    b.rect(26, 22, 7, 6, 4);
    b.rect(16, 12, 8, 3, 5);
    b.outline(1, false);
    return b;
}

gs::Bitmap stackArt() {
    gs::Bitmap b(22, 48);
    b.rect(4, 28, 14, 16, 2);
    b.rect(6, 16, 10, 12, 3);
    b.rect(8, 6, 6, 10, 4);
    b.rect(3, 42, 16, 4, 5);
    b.outline(1, false);
    return b;
}

gs::Bitmap gateArt() {
    gs::Bitmap b(28, 22);
    b.rect(2, 8, 22, 6, 2);
    b.rect(4, 4, 4, 6, 3);
    b.rect(18, 4, 4, 6, 3);
    b.ellipse(8, 16, 4, 4, 4);
    b.ellipse(20, 16, 4, 4, 4);
    b.rect(10, 9, 6, 3, 5);
    b.outline(1, false);
    return b;
}

gs::Bitmap chimneyArt() {
    gs::Bitmap b(18, 56);
    b.rect(4, 10, 10, 40, 2);
    b.rect(2, 6, 14, 6, 3);
    b.rect(6, 46, 6, 8, 4);
    b.rect(7, 16, 4, 8, 5);
    b.outline(1, false);
    return b;
}

gs::Bitmap crucibleArt() {
    gs::Bitmap b(36, 24);
    b.poly({{4, 6}, {32, 6}, {28, 18}, {8, 18}}, 2);
    b.ellipse(18, 10, 8, 3, 3);
    b.rect(14, 16, 8, 6, 4);
    b.outline(1, false);
    return b;
}

gs::Bitmap ingotArt() {
    gs::Bitmap b(20, 16);
    b.poly({{2, 8}, {10, 3}, {18, 8}, {10, 13}}, 2);
    b.rect(6, 7, 8, 3, 3);
    b.outline(1, false);
    return b;
}

gs::Bitmap sootArt() {
    gs::Bitmap b(14, 12);
    b.ellipse(7, 6, 6, 4, 2);
    b.ellipse(5, 5, 2, 2, 3);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(20, 8);
    b.ellipse(10, 4, 9, 3, 1);
    return b;
}

gs::Bitmap plumeArt() {
    gs::Bitmap b(28, 14);
    b.ellipse(8, 8, 6, 4, 2);
    b.ellipse(16, 6, 7, 4, 3);
    b.ellipse(22, 8, 5, 3, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    vdp.setFogColor(gs::rgb4(6, 2, 1));
    const uint16_t text[] = {0, gs::rgb4(14, 12, 9)};
    const uint16_t gold[] = {0, gs::rgb4(15, 12, 4)};
    const uint16_t alert[] = {0, gs::rgb4(15, 5, 2)};
    const uint16_t good[] = {0, gs::rgb4(6, 14, 7)};
    setPal(vdp, PAL_TEXT, text, 2);
    setPal(vdp, PAL_GOLD, gold, 2);
    setPal(vdp, PAL_ALERT, alert, 2);
    setPal(vdp, PAL_GOOD, good, 2);
    const uint16_t coke[] = {0, gs::rgb4(1, 1, 1), gs::rgb4(4, 4, 4), gs::rgb4(8, 7, 5), gs::rgb4(2, 2, 2),
                             gs::rgb4(12, 10, 6), gs::rgb4(15, 8, 2)};
    const uint16_t ladle[] = {0, gs::rgb4(1, 1, 1), gs::rgb4(7, 7, 8), gs::rgb4(11, 11, 12), gs::rgb4(15, 8, 2),
                              gs::rgb4(3, 3, 4), gs::rgb4(5, 4, 3)};
    const uint16_t slag[] = {0, gs::rgb4(1, 1, 1), gs::rgb4(8, 4, 2), gs::rgb4(15, 7, 1), gs::rgb4(3, 3, 3),
                             gs::rgb4(15, 12, 3)};
    const uint16_t stack[] = {0, gs::rgb4(1, 1, 1), gs::rgb4(6, 5, 4), gs::rgb4(9, 7, 5), gs::rgb4(12, 9, 4),
                              gs::rgb4(3, 2, 2)};
    const uint16_t gate[] = {0, gs::rgb4(1, 1, 1), gs::rgb4(10, 6, 3), gs::rgb4(14, 10, 4), gs::rgb4(3, 3, 3),
                             gs::rgb4(15, 12, 5)};
    const uint16_t chimney[] = {0, gs::rgb4(1, 1, 1), gs::rgb4(5, 3, 3), gs::rgb4(8, 4, 3), gs::rgb4(3, 2, 2),
                                gs::rgb4(12, 5, 2)};
    const uint16_t fx[] = {0, gs::rgb4(2, 1, 1), gs::rgb4(8, 6, 5), gs::rgb4(12, 10, 8)};
    const uint16_t glow[] = {0, gs::rgb4(15, 10, 2), gs::rgb4(15, 6, 1)};
    const uint16_t crucible[] = {0, gs::rgb4(1, 1, 1), gs::rgb4(6, 5, 5), gs::rgb4(15, 8, 1), gs::rgb4(4, 3, 3)};
    const uint16_t ingot[] = {0, gs::rgb4(1, 1, 1), gs::rgb4(12, 8, 3), gs::rgb4(15, 12, 5)};
    setPal(vdp, PAL_COKE, coke, 7);
    setPal(vdp, PAL_LADLE, ladle, 7);
    setPal(vdp, PAL_SLAG, slag, 6);
    setPal(vdp, PAL_STACK, stack, 6);
    setPal(vdp, PAL_GATE, gate, 6);
    setPal(vdp, PAL_CHIMNEY, chimney, 6);
    setPal(vdp, PAL_FX, fx, 4);
    setPal(vdp, PAL_GLOW, glow, 3);
    setPal(vdp, PAL_CRUCIBLE, crucible, 5);
    setPal(vdp, PAL_INGOT, ingot, 4);

    int r = PAL_ROAD * 16;
    vdp.setColor(r + 0, 0);
    vdp.setColor(r + 1, gs::rgb4(5, 2, 1));
    vdp.setColor(r + 2, gs::rgb4(3, 2, 1));
    vdp.setColor(r + 3, gs::rgb4(8, 4, 2));
    vdp.setColor(r + 4, gs::rgb4(6, 3, 1));
    vdp.setColor(r + 5, gs::rgb4(4, 2, 1));
    vdp.setColor(r + 6, gs::rgb4(4, 4, 4));
    vdp.setColor(r + 7, gs::rgb4(6, 6, 6));
    vdp.setColor(r + 8, gs::rgb4(7, 5, 3));
    vdp.setColor(r + 9, gs::rgb4(2, 2, 2));
    vdp.setColor(r + 10, gs::rgb4(5, 5, 5));
    vdp.setColor(r + 11, gs::rgb4(3, 2, 2));
    vdp.setColor(r + 12, gs::rgb4(4, 3, 2));
    vdp.setColor(r + 13, gs::rgb4(9, 5, 2));
    vdp.setColor(r + 14, gs::rgb4(15, 12, 4));
    vdp.setColor(r + 15, gs::rgb4(8, 7, 6));

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, tiles, art);
    art.coke = gs::uploadMipped(vdp, cokeArt());
    art.ladle = gs::uploadMipped(vdp, ladleArt());
    art.slag = gs::uploadMipped(vdp, slagArt());
    art.stack = gs::uploadMipped(vdp, stackArt());
    art.gate = gs::uploadMipped(vdp, gateArt());
    art.chimney = gs::uploadMipped(vdp, chimneyArt());
    art.crucible = gs::uploadMipped(vdp, crucibleArt());
    art.ingot = gs::uploadMipped(vdp, ingotArt());
    art.soot = gs::uploadMipped(vdp, sootArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.plume = gs::uploadMipped(vdp, plumeArt());
}

}  // namespace foundry

#include "art.h"

#include <string>

namespace wharfpurs {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i >= 16) break;
        vdp.setColor(pal * 16 + i, c);
        i++;
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void loadFont(gs::VDP& vdp, Art& art) {
    gs::TileAlloc tiles(vdp);
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

gs::Bitmap youArt() {
    gs::Bitmap b(36, 40);
    b.rect(6, 18, 24, 12, 1);          // deck
    b.rect(8, 10, 14, 10, 2);          // cab
    b.rect(10, 12, 8, 5, 3);
    b.rect(20, 6, 3, 8, 4);            // lamp mast
    b.ellipse(21, 5, 3, 3, 5);
    b.rect(4, 28, 8, 7, 6);
    b.rect(24, 28, 8, 7, 6);
    b.rect(2, 22, 6, 4, 7);            // bumper
    b.outline(8, false);
    return b;
}

gs::Bitmap winchArt() {
    gs::Bitmap b(34, 32);
    b.rect(4, 16, 26, 10, 1);
    b.ellipse(17, 14, 7, 7, 2);        // drum
    b.ellipse(17, 14, 3, 3, 3);
    b.rect(6, 8, 3, 10, 4);
    b.rect(25, 8, 3, 10, 4);
    b.rect(3, 24, 8, 6, 5);
    b.rect(23, 24, 8, 6, 5);
    b.outline(6, false);
    return b;
}

gs::Bitmap dollyArt() {
    gs::Bitmap b(48, 28);
    b.rect(4, 12, 40, 8, 1);           // flat bed
    b.rect(8, 6, 10, 8, 2);
    b.rect(20, 8, 16, 5, 3);           // crates
    b.rect(2, 18, 7, 6, 4);
    b.rect(16, 18, 7, 6, 4);
    b.rect(30, 18, 7, 6, 4);
    b.rect(40, 18, 6, 6, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap stackerArt() {
    gs::Bitmap b(32, 52);
    b.rect(8, 28, 18, 12, 1);
    b.rect(18, 4, 4, 28, 2);           // mast
    b.rect(14, 16, 12, 4, 3);          // fork
    b.rect(6, 18, 8, 6, 4);
    b.rect(4, 38, 8, 8, 5);
    b.rect(18, 38, 8, 8, 5);
    b.rect(10, 30, 6, 4, 6);
    b.outline(7, false);
    return b;
}

gs::Bitmap pilingArt() {
    gs::Bitmap b(12, 40);
    b.rect(3, 0, 6, 34, 1);
    b.rect(1, 30, 10, 8, 2);
    b.rect(4, 8, 4, 3, 3);
    b.rect(4, 18, 4, 3, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap lanternArt() {
    gs::Bitmap b(14, 22);
    b.rect(6, 0, 2, 4, 3);
    b.poly({{2, 6}, {12, 6}, {10, 16}, {4, 16}}, 1);
    b.rect(5, 8, 4, 6, 2);
    b.rect(4, 16, 6, 3, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap craneArt() {
    gs::Bitmap b(70, 56);
    b.poly({{8, 54}, {18, 54}, {28, 8}, {22, 8}}, 1);
    b.poly({{28, 10}, {66, 16}, {64, 20}, {26, 14}}, 2);
    b.line(58, 18, 58, 36, 3, 1);
    b.rect(54, 36, 8, 5, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap shedArt() {
    gs::Bitmap b(56, 40);
    b.poly({{2, 16}, {28, 4}, {54, 16}, {50, 16}, {28, 8}, {6, 16}}, 2);
    b.rect(6, 16, 44, 20, 1);
    b.rect(22, 22, 10, 14, 3);
    b.rect(10, 20, 8, 6, 4);
    b.rect(38, 20, 8, 6, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap gullArt() {
    gs::Bitmap b(18, 8);
    b.line(0, 4, 8, 2, 1, 1);
    b.line(8, 2, 17, 5, 1, 1);
    b.line(2, 6, 8, 3, 2, 1);
    return b;
}

gs::Bitmap moonArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 6, 6, 1);
    b.ellipse(11, 7, 4, 4, 0);
    b.outline(2, false);
    return b;
}

gs::Bitmap shotArt() {
    gs::Bitmap b(8, 12);
    b.ellipse(4, 4, 3, 3, 1);
    b.rect(3, 6, 2, 5, 2);
    return b;
}

gs::Bitmap boltArt() {
    gs::Bitmap b(6, 10);
    b.rect(2, 0, 2, 8, 1);
    b.rect(1, 7, 4, 2, 2);
    return b;
}

gs::Bitmap puffArt() {
    gs::Bitmap b(14, 12);
    b.ellipse(5, 6, 4, 3, 1);
    b.ellipse(9, 5, 4, 4, 2);
    return b;
}

gs::Bitmap sparkArt() {
    gs::Bitmap b(8, 8);
    b.ellipse(4, 4, 3, 3, 1);
    b.set(4, 1, 2);
    b.set(4, 6, 2);
    return b;
}

gs::Bitmap flameArt(int step) {
    gs::Bitmap b(12, 16);
    b.poly({{6, 1}, {11, 14}, {1, 14}}, 1);
    b.poly({{6, float(step ? 5 : 4)}, {9, 13}, {3, 13}}, 2);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(28, 8);
    b.ellipse(14, 4, 12, 3, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shade = gs::rgb4(0, 0, 0);
    setPal(vdp, PAL_INK, {0, gs::rgb4(14, 15, 15), gs::rgb4(8, 11, 13), gs::rgb4(4, 6, 8)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 5), gs::rgb4(12, 8, 2), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(10, 2, 2), gs::rgb4(15, 10, 6)});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(6, 15, 9), gs::rgb4(2, 9, 6), gs::rgb4(12, 15, 13)});
    setPal(vdp, PAL_YOU,
           {0, gs::rgb4(14, 12, 2), gs::rgb4(8, 10, 12), gs::rgb4(4, 8, 12), gs::rgb4(6, 6, 6), gs::rgb4(15, 14, 6),
            gs::rgb4(3, 3, 3), gs::rgb4(11, 8, 3), shade});
    setPal(vdp, PAL_WINCH,
           {0, gs::rgb4(10, 4, 3), gs::rgb4(6, 6, 7), gs::rgb4(3, 3, 4), gs::rgb4(12, 8, 5), gs::rgb4(2, 2, 2), shade});
    setPal(vdp, PAL_DOLLY,
           {0, gs::rgb4(5, 7, 9), gs::rgb4(9, 6, 3), gs::rgb4(12, 9, 5), gs::rgb4(3, 3, 3), shade});
    setPal(vdp, PAL_STACK,
           {0, gs::rgb4(4, 8, 6), gs::rgb4(11, 12, 10), gs::rgb4(14, 10, 3), gs::rgb4(7, 9, 11), gs::rgb4(2, 2, 2),
            gs::rgb4(13, 13, 8), shade});
    setPal(vdp, PAL_WOOD,
           {0, gs::rgb4(9, 6, 3), gs::rgb4(14, 12, 4), gs::rgb4(5, 3, 2), gs::rgb4(12, 10, 8), shade});
    setPal(vdp, PAL_FX, {0, gs::rgb4(10, 10, 11), gs::rgb4(14, 14, 15), gs::rgb4(15, 8, 2), gs::rgb4(12, 3, 1)});
    setPal(vdp, PAL_DUSK, {0, gs::rgb4(13, 14, 15), gs::rgb4(6, 7, 10), gs::rgb4(3, 4, 7)});
    setPal(vdp, PAL_BOLT, {0, gs::rgb4(15, 15, 10), gs::rgb4(15, 8, 2), gs::rgb4(8, 12, 15)});

    const uint16_t road[16] = {
        0,
        gs::rgb4(3, 6, 4),
        gs::rgb4(2, 4, 3),
        gs::rgb4(5, 7, 4),
        gs::rgb4(11, 8, 4),
        gs::rgb4(7, 5, 2),
        gs::rgb4(9, 7, 3),
        gs::rgb4(6, 4, 2),
        gs::rgb4(12, 10, 6),
        gs::rgb4(5, 4, 2),
        gs::rgb4(8, 6, 3),
        gs::rgb4(1, 4, 8),
        gs::rgb4(2, 6, 11),
        gs::rgb4(9, 13, 14),
        gs::rgb4(14, 12, 3),
        gs::rgb4(13, 11, 7),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_ROAD * 16 + i, road[i]);

    art.you = gs::uploadMipped(vdp, youArt());
    art.winch = gs::uploadMipped(vdp, winchArt());
    art.dolly = gs::uploadMipped(vdp, dollyArt());
    art.stacker = gs::uploadMipped(vdp, stackerArt());
    art.piling = gs::uploadMipped(vdp, pilingArt());
    art.lantern = gs::uploadMipped(vdp, lanternArt());
    art.crane = gs::uploadMipped(vdp, craneArt());
    art.shed = gs::uploadMipped(vdp, shedArt());
    art.gull = gs::uploadMipped(vdp, gullArt());
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.shot = gs::uploadMipped(vdp, shotArt());
    art.bolt = gs::uploadMipped(vdp, boltArt());
    art.puff = gs::uploadMipped(vdp, puffArt());
    art.spark = gs::uploadMipped(vdp, sparkArt());
    art.flame[0] = gs::uploadMipped(vdp, flameArt(0));
    art.flame[1] = gs::uploadMipped(vdp, flameArt(1));
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    loadFont(vdp, art);
}

}  // namespace wharfpurs

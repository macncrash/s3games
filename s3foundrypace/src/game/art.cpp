#include "game/art.h"

#include <cmath>
#include <initializer_list>
#include <string>

namespace foundrypace {
namespace {

constexpr float TAU = 6.2831853f;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

gs::Bitmap smithArt(int step) {
    gs::Bitmap b(36, 72);
    b.ellipse(18, 12, 6, 6, 4);
    b.poly({{10, 3}, {26, 3}, {24, 11}, {12, 11}}, 8);
    b.rect(15, 10, 2, 2, 9);
    b.rect(20, 10, 2, 2, 9);
    b.poly({{9, 18}, {27, 18}, {29, 42}, {7, 42}}, 2);
    b.poly({{11, 18}, {20, 18}, {19, 40}, {8, 40}}, 1);
    b.rect(16, 20, 4, 12, 11);
    b.line(10, 22, 4, 36, 6, 3.f);
    b.ellipse(4, 38, 3, 3, 7);
    b.line(26, 22, 32, 34, 6, 3.f);
    b.ellipse(32, 36, 3, 3, 6);
    if (step == 0) {
        b.rect(10, 42, 6, 22, 3);
        b.rect(20, 42, 6, 16, 8);
        b.rect(8, 62, 9, 4, 14);
        b.rect(19, 56, 8, 4, 8);
    } else {
        b.rect(10, 42, 6, 16, 8);
        b.rect(20, 42, 6, 22, 3);
        b.rect(9, 56, 8, 4, 8);
        b.rect(18, 62, 9, 4, 14);
    }
    b.outline(12, false);
    return b;
}

gs::Bitmap fallenArt() {
    gs::Bitmap b(84, 30);
    b.ellipse(12, 16, 6, 5, 4);
    b.poly({{8, 6}, {18, 5}, {20, 14}, {6, 14}}, 8);
    b.poly({{18, 10}, {70, 13}, {74, 24}, {16, 22}}, 2);
    b.poly({{18, 10}, {42, 11}, {44, 22}, {16, 20}}, 1);
    b.rect(62, 18, 10, 4, 8);
    b.outline(12, false);
    return b;
}

gs::Bitmap crucibleArt() {
    gs::Bitmap b(28, 34);
    b.poly({{4, 10}, {24, 10}, {22, 28}, {6, 28}}, 3);
    b.ellipse(14, 10, 10, 4, 5);
    b.ellipse(14, 10, 6, 2, 6);
    b.rect(12, 2, 4, 8, 2);
    b.outline(1, false);
    return b;
}

gs::Bitmap ingotArt() {
    gs::Bitmap b(30, 16);
    b.poly({{2, 12}, {8, 4}, {28, 4}, {22, 12}}, 2);
    b.poly({{4, 11}, {9, 6}, {18, 6}, {13, 11}}, 1);
    b.rect(6, 12, 16, 3, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap furnaceArt() {
    gs::Bitmap b(48, 70);
    b.rect(6, 18, 36, 50, 2);
    b.rect(10, 22, 28, 42, 1);
    b.poly({{4, 18}, {44, 18}, {40, 8}, {8, 8}}, 3);
    b.rect(18, 2, 12, 8, 4);
    b.rect(20, 0, 8, 4, 5);
    b.ellipse(24, 40, 8, 10, 6);
    b.ellipse(24, 42, 4, 6, 7);
    b.rect(8, 60, 32, 8, 3);
    b.outline(8, false);
    return b;
}

gs::Bitmap flameArt(int fr) {
    gs::Bitmap b(20, 28);
    b.poly({{10, 2}, {16, 14}, {12, 26}, {8, 26}, {4, 14}}, fr ? 2 : 1);
    b.poly({{10, 8}, {13, 16}, {10, 24}, {7, 16}}, 3);
    return b;
}

gs::Bitmap chimneyArt() {
    gs::Bitmap b(18, 48);
    b.rect(4, 8, 10, 40, 1);
    b.rect(6, 10, 4, 36, 2);
    b.rect(2, 4, 14, 6, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap stakeArt() {
    gs::Bitmap b(10, 36);
    b.rect(4, 2, 2, 32, 1);
    b.poly({{6, 4}, {10, 8}, {6, 14}}, 5);
    b.rect(3, 32, 4, 3, 2);
    return b;
}

gs::Bitmap beadArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 6, 6, 1);
    b.ellipse(8, 8, 2, 2, 2);
    b.line(8, 1, 8, 4, 3, 1.f);
    b.line(8, 12, 8, 15, 3, 1.f);
    b.line(1, 8, 4, 8, 3, 1.f);
    b.line(12, 8, 15, 8, 3, 1.f);
    return b;
}

gs::Bitmap ironArt() {
    gs::Bitmap b(14, 48);
    b.rect(5, 8, 4, 36, 1);
    b.rect(2, 4, 10, 6, 2);
    b.rect(4, 42, 6, 4, 3);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(12, 64);
    b.rect(4, 0, 4, 60, 1);
    b.rect(2, 56, 8, 6, 2);
    b.rect(5, 8, 2, 20, 3);
    return b;
}

gs::Bitmap bellowsArt(int fr) {
    gs::Bitmap b(36, 18);
    if (fr == 0) b.poly({{2, 9}, {28, 3}, {30, 15}, {2, 12}}, 1);
    else b.poly({{2, 8}, {20, 5}, {22, 13}, {2, 11}}, 1);
    b.rect(28, 6, 6, 6, 2);
    b.outline(3, false);
    return b;
}

gs::Bitmap lipArt() {
    gs::Bitmap b(40, 22);
    b.poly({{2, 4}, {38, 6}, {34, 18}, {4, 16}}, 1);
    b.poly({{8, 8}, {30, 9}, {28, 14}, {10, 13}}, 2);
    return b;
}

gs::Bitmap pipArt() {
    gs::Bitmap b(12, 12);
    b.ellipse(6, 6, 5, 5, 1);
    b.ellipse(6, 6, 2, 2, 2);
    return b;
}

gs::Bitmap flashArt() {
    gs::Bitmap b(24, 24);
    b.poly({{12, 1}, {14, 10}, {23, 12}, {14, 14}, {12, 23}, {10, 14}, {1, 12}, {10, 10}}, 1);
    b.ellipse(12, 12, 3, 3, 2);
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(20, 16);
    b.ellipse(6, 10, 4, 3, 1);
    b.ellipse(12, 7, 5, 3, 2);
    b.ellipse(16, 11, 3, 2, 3);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(28, 8);
    b.ellipse(14, 4, 12, 3, 1);
    return b;
}

gs::Bitmap stripeArt() {
    gs::Bitmap b(16, 6);
    b.rect(0, 1, 16, 4, 1);
    b.rect(0, 2, 16, 2, 2);
    return b;
}

gs::Bitmap hoodArt() {
    gs::Bitmap b(64, 26);
    b.poly({{2, 24}, {32, 4}, {62, 24}}, 1);
    b.poly({{16, 18}, {32, 8}, {46, 18}}, 2);
    b.rect(28, 16, 8, 8, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap sparkArt() {
    gs::Bitmap b(8, 8);
    b.line(1, 7, 6, 1, 1, 1.2f);
    b.set(4, 3, 2);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    gs::TextStyle big{3, 1, 0, 15, 1};
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) {
                    px[y * 8 + x + 1] = 1;
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
                }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(1, 0, 0);
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(14, 12, 10), gs::rgb4(9, 8, 7), gs::rgb4(3, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, ink});
    setPal(vdp, PAL_EMBER, {0, gs::rgb4(15, 8, 1), gs::rgb4(15, 13, 6), gs::rgb4(6, 2, 0), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 3, 2), gs::rgb4(15, 9, 6), gs::rgb4(5, 1, 0), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, ink});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(9, 15, 5), gs::rgb4(14, 15, 10), gs::rgb4(1, 4, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, ink});
    setPal(vdp, PAL_IRON, {0, gs::rgb4(9, 9, 10), gs::rgb4(5, 5, 6), gs::rgb4(3, 3, 4), gs::rgb4(12, 12, 13),
                           gs::rgb4(2, 2, 2), gs::rgb4(7, 6, 5), gs::rgb4(11, 8, 4), 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FIGURE,
           {0, gs::rgb4(12, 6, 2), gs::rgb4(7, 3, 1), gs::rgb4(3, 2, 2), gs::rgb4(13, 10, 7), gs::rgb4(4, 3, 2),
            gs::rgb4(9, 7, 5), gs::rgb4(15, 10, 2), gs::rgb4(2, 2, 2), gs::rgb4(1, 1, 1), gs::rgb4(8, 5, 2),
            gs::rgb4(14, 12, 6), gs::rgb4(1, 0, 0), gs::rgb4(6, 4, 3), gs::rgb4(4, 3, 2), ink});
    setPal(vdp, PAL_HOLD, {0, gs::rgb4(15, 11, 2), gs::rgb4(15, 15, 10), gs::rgb4(6, 3, 0), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, ink});
    setPal(vdp, PAL_LIVE, {0, gs::rgb4(12, 15, 7), gs::rgb4(6, 12, 4), gs::rgb4(1, 3, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, ink});
    setPal(vdp, PAL_SOOT, {0, gs::rgb4(4, 4, 5), gs::rgb4(2, 2, 3), gs::rgb4(6, 5, 4), gs::rgb4(1, 1, 1),
                           gs::rgb4(8, 6, 4), gs::rgb4(3, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 15, 12), gs::rgb4(15, 8, 1), gs::rgb4(8, 6, 4), gs::rgb4(12, 9, 5),
                         gs::rgb4(3, 2, 2), gs::rgb4(7, 5, 3), gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_GLOW, {0, gs::rgb4(15, 10, 2), gs::rgb4(15, 6, 1), gs::rgb4(15, 14, 6), gs::rgb4(8, 3, 0),
                           gs::rgb4(15, 15, 8), gs::rgb4(12, 4, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BRICK, {0, gs::rgb4(10, 4, 2), gs::rgb4(6, 2, 1), gs::rgb4(13, 7, 3), gs::rgb4(3, 1, 1),
                            gs::rgb4(14, 10, 4), gs::rgb4(2, 1, 1), gs::rgb4(8, 5, 3), 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_STACK, {0, gs::rgb4(7, 6, 6), gs::rgb4(4, 3, 3), gs::rgb4(11, 5, 2), gs::rgb4(3, 2, 2),
                            gs::rgb4(2, 2, 2), gs::rgb4(12, 10, 8), gs::rgb4(15, 8, 1), gs::rgb4(15, 12, 3), 0, 0, 0, 0,
                            0, 0, ink});
    setPal(vdp, PAL_SLAG, {0, gs::rgb4(5, 4, 3), gs::rgb4(8, 5, 2), gs::rgb4(3, 2, 2), gs::rgb4(12, 6, 1), 0, 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 0, ink});

    const uint16_t road[16] = {
        0,
        gs::rgb4(6, 3, 1),
        gs::rgb4(3, 2, 1),
        gs::rgb4(9, 4, 1),
        gs::rgb4(12, 6, 1),
        gs::rgb4(4, 3, 2),
        gs::rgb4(14, 8, 2),
        gs::rgb4(5, 4, 3),
        gs::rgb4(15, 11, 3),
        gs::rgb4(2, 2, 2),
        gs::rgb4(8, 5, 2),
        gs::rgb4(1, 1, 1),
        gs::rgb4(10, 4, 1),
        gs::rgb4(7, 3, 1),
        gs::rgb4(13, 9, 3),
        gs::rgb4(4, 2, 1),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_ROAD * 16 + i, road[i]);

    loadFont(vdp, art);
    art.smith[0] = gs::uploadMipped(vdp, smithArt(0));
    art.smith[1] = gs::uploadMipped(vdp, smithArt(1));
    art.fallen = gs::uploadMipped(vdp, fallenArt());
    art.crucible = gs::uploadMipped(vdp, crucibleArt());
    art.ingot = gs::uploadMipped(vdp, ingotArt());
    art.furnace = gs::uploadMipped(vdp, furnaceArt());
    art.flame[0] = gs::uploadMipped(vdp, flameArt(0));
    art.flame[1] = gs::uploadMipped(vdp, flameArt(1));
    art.chimney = gs::uploadMipped(vdp, chimneyArt());
    art.stake = gs::uploadMipped(vdp, stakeArt());
    art.bead = gs::uploadMipped(vdp, beadArt());
    art.iron = gs::uploadMipped(vdp, ironArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.bellows[0] = gs::uploadMipped(vdp, bellowsArt(0));
    art.bellows[1] = gs::uploadMipped(vdp, bellowsArt(1));
    art.lip = gs::uploadMipped(vdp, lipArt());
    art.pip = gs::uploadMipped(vdp, pipArt());
    art.flash = gs::uploadMipped(vdp, flashArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.stripe = gs::uploadMipped(vdp, stripeArt());
    art.hood = gs::uploadMipped(vdp, hoodArt());
    art.spark = gs::uploadMipped(vdp, sparkArt());

    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.hudEnabled = true;
    vdp.setFogColor(gs::rgb4(8, 3, 1));
}

}  // namespace foundrypace

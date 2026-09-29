#include "game/art.h"

#include <initializer_list>
#include <string>

namespace orchardpurs {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap treeArt() {
    Bitmap b(40, 64);
    b.ellipse(20, 18, 16, 14, 2);
    b.ellipse(12, 24, 9, 8, 3);
    b.ellipse(28, 22, 8, 8, 1);
    b.rect(17, 30, 6, 26, 4);
    b.ellipse(12, 14, 3, 3, 5);
    b.ellipse(24, 16, 2, 2, 5);
    b.ellipse(30, 18, 2, 2, 6);
    b.outline(7, false);
    return b;
}

Bitmap tractorArt() {
    Bitmap b(44, 32);
    b.rect(6, 14, 24, 10, 2);
    b.rect(22, 6, 12, 10, 3);
    b.rect(26, 8, 6, 4, 4);
    b.ellipse(12, 24, 6, 6, 1);
    b.ellipse(32, 24, 7, 7, 1);
    b.ellipse(12, 24, 2, 2, 5);
    b.ellipse(32, 24, 3, 3, 5);
    b.rect(4, 16, 4, 3, 6);
    b.outline(7, false);
    return b;
}

Bitmap sprayerArt() {
    Bitmap b(40, 28);
    b.rect(8, 12, 22, 8, 2);
    b.ellipse(14, 10, 5, 5, 3);
    b.ellipse(22, 10, 4, 4, 4);
    b.ellipse(10, 22, 5, 5, 1);
    b.ellipse(28, 22, 5, 5, 1);
    b.rect(28, 14, 8, 2, 5);
    b.outline(7, false);
    return b;
}

Bitmap harvestArt() {
    Bitmap b(52, 36);
    b.rect(4, 16, 32, 10, 2);
    b.rect(28, 6, 16, 14, 3);
    b.rect(32, 8, 8, 5, 4);
    b.ellipse(14, 28, 7, 7, 1);
    b.ellipse(36, 28, 8, 8, 1);
    b.rect(2, 18, 6, 4, 5);
    b.rect(8, 10, 14, 4, 6);
    b.outline(7, false);
    return b;
}

Bitmap appleArt() {
    Bitmap b(10, 10);
    b.ellipse(5, 6, 4, 4, 1);
    b.rect(4, 1, 2, 3, 2);
    b.outline(3, false);
    return b;
}

Bitmap mistArt() {
    Bitmap b(14, 8);
    b.ellipse(4, 4, 3, 3, 1);
    b.ellipse(9, 4, 4, 3, 2);
    return b;
}

Bitmap shedArt() {
    Bitmap b(28, 48);
    b.poly({{2, 16}, {14, 4}, {26, 16}}, 2);
    b.rect(4, 16, 20, 26, 3);
    b.rect(11, 28, 6, 14, 1);
    b.rect(6, 20, 5, 5, 4);
    b.outline(5, false);
    return b;
}

Bitmap leafArt() {
    Bitmap b(8, 8);
    b.ellipse(4, 4, 3, 2, 1);
    return b;
}

Bitmap sparkArt() {
    Bitmap b(8, 8);
    b.ellipse(4, 4, 3, 3, 1);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TextStyle big{2, 1, 0, 15, 1};
    for (int c = 32; c < 128; c++) {
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(15, 15, 13);
    const uint16_t shadow = gs::rgb4(1, 1, 1);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(6, 9, 3), gs::rgb4(14, 12, 4), gs::rgb4(14, 6, 3), gs::rgb4(8, 13, 6),
                          gs::rgb4(12, 10, 6), shadow, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(3, 9, 2), gs::rgb4(2, 7, 2), gs::rgb4(4, 11, 3), gs::rgb4(6, 4, 2),
                           gs::rgb4(13, 3, 2), gs::rgb4(14, 10, 3), shadow, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_TRACTOR, {0, gs::rgb4(2, 2, 2), gs::rgb4(10, 4, 2), gs::rgb4(12, 8, 3), gs::rgb4(8, 12, 14),
                              gs::rgb4(14, 13, 8), gs::rgb4(14, 12, 3), shadow, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_SPRAY, {0, gs::rgb4(2, 2, 3), gs::rgb4(4, 8, 6), gs::rgb4(8, 13, 10), gs::rgb4(12, 14, 8),
                            gs::rgb4(6, 10, 12), shadow, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_HARVEST, {0, gs::rgb4(2, 2, 2), gs::rgb4(8, 7, 3), gs::rgb4(12, 10, 4), gs::rgb4(6, 8, 10),
                              gs::rgb4(13, 6, 2), gs::rgb4(5, 4, 2), shadow, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_APPLE, {0, gs::rgb4(13, 2, 2), gs::rgb4(3, 8, 2), gs::rgb4(14, 12, 4), shadow, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_SHED, {0, gs::rgb4(3, 2, 2), gs::rgb4(10, 4, 3), gs::rgb4(12, 8, 5), gs::rgb4(8, 11, 13),
                           gs::rgb4(4, 3, 2), shadow, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_LEAF, {0, gs::rgb4(6, 10, 3), gs::rgb4(10, 12, 4), shadow, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           shadow});
    setPal(vdp, PAL_WRECK, {0, gs::rgb4(3, 3, 3), gs::rgb4(4, 4, 3), gs::rgb4(5, 4, 3), gs::rgb4(3, 3, 4),
                            gs::rgb4(6, 5, 3), gs::rgb4(2, 2, 2), shadow, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    loadFont(vdp, art);
    gs::TextStyle title{3, 1, 0, 15, 1};
    art.titleA = gs::uploadMipped(vdp, gs::textBitmap("ORCHARD", title));
    art.titleB = gs::uploadMipped(vdp, gs::textBitmap("PURSUIT", title));
    art.sub = gs::uploadMipped(vdp, gs::textBitmap("LAST MACHINE STILL RUNNING", gs::TextStyle{1, 3, 0, 15, 1}));
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.tractor = gs::uploadMipped(vdp, tractorArt());
    art.sprayer = gs::uploadMipped(vdp, sprayerArt());
    art.harvest = gs::uploadMipped(vdp, harvestArt());
    art.apple = gs::uploadMipped(vdp, appleArt());
    art.mist = gs::uploadMipped(vdp, mistArt());
    art.shed = gs::uploadMipped(vdp, shedArt());
    art.leaf = gs::uploadMipped(vdp, leafArt());
    art.spark = gs::uploadMipped(vdp, sparkArt());
    vdp.A.enabled = false;
    vdp.B.enabled = false;
}

}  // namespace orchardpurs

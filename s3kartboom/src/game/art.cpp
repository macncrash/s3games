#include "game/art.h"

namespace kartboom {
namespace {

void pal(gs::VDP& v, int bank, int i, int r, int g, int b) { v.setColor(bank * 16 + i, gs::rgb4(r, g, b)); }

void paintKart(gs::Bitmap& b, bool withDrive) {
    b.rect(6, 28, 36, 22, 1);
    b.rect(10, 18, 28, 16, 1);
    b.rect(14, 10, 20, 12, 2);
    b.rect(16, 12, 7, 6, 3);
    b.rect(25, 12, 7, 6, 3);
    b.rect(2, 30, 8, 16, 4);
    b.rect(38, 30, 8, 16, 4);
    b.rect(4, 34, 4, 8, 5);
    b.rect(40, 34, 4, 8, 5);
    b.rect(18, 40, 12, 6, 6);
    if (withDrive) {
        b.rect(16, 22, 16, 12, 7);
        b.ellipse(24, 28, 5, 5, 8);
        b.ellipse(24, 28, 2, 2, 9);
    }
    b.rect(20, 46, 8, 4, 6);
}

void paintCone(gs::Bitmap& b) {
    b.poly({{8, 2}, {14, 22}, {2, 22}}, 1);
    b.rect(3, 8, 10, 3, 2);
    b.rect(4, 14, 8, 2, 2);
    b.rect(1, 21, 14, 3, 3);
}

void paintDrive(gs::Bitmap& b) {
    b.rect(2, 4, 20, 16, 1);
    b.ellipse(12, 12, 6, 6, 2);
    b.ellipse(12, 12, 2, 2, 3);
    b.rect(4, 2, 16, 3, 4);
}

void paintBoom(gs::Bitmap& b) {
    b.rect(6, 8, 8, 70, 1);
    b.rect(8, 10, 4, 64, 2);
    b.rect(6, 6, 52, 8, 1);
    b.rect(8, 8, 46, 4, 3);
    b.rect(48, 14, 4, 16, 1);
    b.rect(46, 28, 8, 4, 4);
    b.line(50, 32, 50, 48, 4, 2);
    b.rect(46, 48, 8, 6, 5);
    b.rect(2, 72, 16, 6, 6);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    for (int i = 0; i < 16; i++) {
        pal(vdp, PAL_HUD, i, 0, 0, 0);
        pal(vdp, PAL_KART, i, 0, 0, 0);
        pal(vdp, PAL_DRIVE, i, 0, 0, 0);
        pal(vdp, PAL_CONE, i, 0, 0, 0);
        pal(vdp, PAL_BOOM, i, 0, 0, 0);
        pal(vdp, PAL_ROAD, i, 0, 0, 0);
    }
    pal(vdp, PAL_HUD, 1, 15, 14, 8);
    pal(vdp, PAL_HUD, 2, 15, 15, 15);
    pal(vdp, PAL_HUD, 3, 4, 14, 6);
    pal(vdp, PAL_HUD, 4, 15, 4, 3);

    pal(vdp, PAL_KART, 1, 15, 12, 1);
    pal(vdp, PAL_KART, 2, 2, 6, 13);
    pal(vdp, PAL_KART, 3, 10, 14, 15);
    pal(vdp, PAL_KART, 4, 1, 1, 1);
    pal(vdp, PAL_KART, 5, 6, 6, 7);
    pal(vdp, PAL_KART, 6, 12, 3, 2);
    pal(vdp, PAL_KART, 7, 11, 12, 13);
    pal(vdp, PAL_KART, 8, 4, 5, 7);
    pal(vdp, PAL_KART, 9, 15, 14, 6);

    pal(vdp, PAL_DRIVE, 1, 9, 10, 12);
    pal(vdp, PAL_DRIVE, 2, 2, 2, 3);
    pal(vdp, PAL_DRIVE, 3, 14, 12, 4);
    pal(vdp, PAL_DRIVE, 4, 5, 6, 8);

    pal(vdp, PAL_CONE, 1, 15, 7, 1);
    pal(vdp, PAL_CONE, 2, 15, 15, 15);
    pal(vdp, PAL_CONE, 3, 3, 3, 3);

    pal(vdp, PAL_BOOM, 1, 12, 4, 3);
    pal(vdp, PAL_BOOM, 2, 8, 8, 9);
    pal(vdp, PAL_BOOM, 3, 15, 12, 2);
    pal(vdp, PAL_BOOM, 4, 4, 4, 5);
    pal(vdp, PAL_BOOM, 5, 15, 13, 3);
    pal(vdp, PAL_BOOM, 6, 5, 5, 6);

    // Road palette: 1-3 grass, 4-5 verge, 6-7 tarmac, 14 paint, 15 speck.
    pal(vdp, PAL_ROAD, 1, 2, 8, 3);
    pal(vdp, PAL_ROAD, 2, 1, 6, 2);
    pal(vdp, PAL_ROAD, 3, 4, 10, 3);
    pal(vdp, PAL_ROAD, 4, 8, 8, 7);
    pal(vdp, PAL_ROAD, 5, 5, 5, 5);
    pal(vdp, PAL_ROAD, 6, 4, 4, 5);
    pal(vdp, PAL_ROAD, 7, 3, 3, 4);
    pal(vdp, PAL_ROAD, 8, 6, 6, 6);
    pal(vdp, PAL_ROAD, 14, 14, 12, 3);
    pal(vdp, PAL_ROAD, 15, 7, 7, 8);

    gs::Bitmap kart(48, 56);
    paintKart(kart, true);
    art.kart = gs::uploadMipped(vdp, kart);
    gs::Bitmap bare(48, 56);
    paintKart(bare, false);
    art.kartEmpty = gs::uploadMipped(vdp, bare);

    gs::Bitmap drive(24, 22);
    paintDrive(drive);
    art.drive = gs::uploadMipped(vdp, drive);

    gs::Bitmap cone(16, 24);
    paintCone(cone);
    art.cone = gs::uploadMipped(vdp, cone);

    gs::Bitmap boom(64, 80);
    paintBoom(boom);
    art.boom = gs::uploadMipped(vdp, boom);

    gs::TextStyle st;
    st.scale = 1;
    st.color = 1;
    st.spacing = 1;
    for (int i = 0; i < 96; i++) {
        std::string s(1, char(32 + i));
        if (i == 0) {
            gs::Bitmap blank(5, 7);
            art.glyph[i] = gs::uploadImage(vdp, blank);
        } else {
            art.glyph[i] = gs::uploadImage(vdp, gs::textBitmap(s, st));
        }
    }
}

}  // namespace kartboom

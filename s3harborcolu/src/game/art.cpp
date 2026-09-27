#include "game/art.h"

#include <string>

namespace hcol {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink) {
    for (int i = 0; i < 16; ++i) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, ink);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
    gs::TextStyle big{3, 1, 0, 15, 1};
    for (int c = 32; c < 128; ++c) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; ++y)
            for (int x = 0; x < 5; ++x)
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

gs::Bitmap lorryArt() {
    gs::Bitmap b(48, 56);
    b.rect(14, 2, 20, 7, 1);
    b.rect(16, 4, 16, 3, 5);
    b.rect(12, 9, 24, 10, 2);
    b.rect(14, 11, 8, 6, 6);
    b.rect(26, 11, 8, 6, 6);
    b.rect(22, 11, 4, 6, 3);
    b.rect(8, 19, 32, 8, 4);
    b.rect(10, 20, 28, 2, 1);
    b.rect(12, 27, 24, 12, 7);
    for (int y = 29; y < 37; y += 3) b.rect(14, y, 20, 1, 8);
    b.ellipse(14, 34, 4, 4, 9);
    b.ellipse(34, 34, 4, 4, 9);
    b.rect(6, 40, 36, 5, 10);
    b.rect(4, 32, 5, 16, 11);
    b.rect(39, 32, 5, 16, 11);
    b.rect(20, 44, 8, 3, 12);
    b.outline(15, false);
    return b;
}

gs::Bitmap tenderArt() {
    gs::Bitmap b(40, 32);
    b.rect(10, 2, 20, 6, 1);
    b.rect(12, 4, 7, 3, 5);
    b.rect(21, 4, 7, 3, 5);
    b.rect(8, 8, 24, 8, 2);
    b.rect(6, 16, 28, 6, 4);
    b.ellipse(12, 20, 3, 3, 8);
    b.ellipse(28, 20, 3, 3, 8);
    b.rect(4, 22, 32, 4, 6);
    b.rect(14, 14, 12, 3, 7);
    b.outline(15, false);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(14, 48);
    b.rect(5, 2, 4, 40, 2);
    b.rect(6, 2, 2, 40, 1);
    b.rect(3, 4, 8, 5, 4);
    b.rect(2, 42, 10, 4, 3);
    b.outline(15, false);
    return b;
}

gs::Bitmap linkArt() {
    gs::Bitmap b(10, 8);
    b.rect(1, 1, 8, 6, 1);
    b.rect(3, 2, 4, 4, 2);
    b.outline(15, false);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(12, 12);
    b.ellipse(6, 6, 5, 5, 1);
    b.ellipse(6, 6, 2, 2, 2);
    b.outline(15, false);
    return b;
}

gs::Bitmap craneArt() {
    gs::Bitmap b(36, 64);
    b.rect(14, 8, 8, 48, 2);
    b.rect(16, 8, 3, 48, 1);
    b.rect(6, 6, 28, 6, 3);
    b.rect(4, 4, 8, 4, 4);
    b.rect(8, 52, 20, 8, 5);
    b.line(28, 10, 32, 28, 6, 2);
    b.outline(15, false);
    return b;
}

gs::Bitmap shedArt() {
    gs::Bitmap b(40, 36);
    b.rect(4, 8, 32, 6, 3);
    b.rect(6, 14, 28, 16, 1);
    b.rect(10, 16, 8, 8, 4);
    b.rect(22, 16, 8, 8, 5);
    b.rect(8, 30, 24, 4, 2);
    b.outline(15, false);
    return b;
}

gs::Bitmap buoyArt() {
    gs::Bitmap b(16, 28);
    b.rect(6, 2, 4, 6, 1);
    b.ellipse(8, 14, 6, 6, 2);
    b.rect(7, 18, 2, 8, 3);
    b.outline(15, false);
    return b;
}

gs::Bitmap gullArt() {
    gs::Bitmap b(20, 10);
    b.line(1, 6, 9, 3, 1, 1);
    b.line(10, 3, 18, 7, 1, 1);
    b.rect(9, 4, 2, 2, 2);
    return b;
}

gs::Bitmap puffArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 6, 5, 1);
    b.ellipse(6, 7, 3, 2, 2);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(24, 8);
    b.ellipse(12, 4, 10, 3, 1);
    return b;
}

gs::Bitmap cloudArt() {
    gs::Bitmap b(28, 12);
    b.ellipse(10, 7, 8, 4, 1);
    b.ellipse(18, 6, 7, 4, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_TEXT, gs::rgb4(14, 14, 13));
    textPal(vdp, PAL_GOLD, gs::rgb4(14, 11, 3));
    textPal(vdp, PAL_ALERT, gs::rgb4(14, 3, 2));
    textPal(vdp, PAL_GOOD, gs::rgb4(4, 13, 6));

    setPal(vdp, PAL_LORRY,
           {0, gs::rgb4(10, 11, 6), gs::rgb4(6, 7, 3), gs::rgb4(3, 4, 2), gs::rgb4(4, 5, 6), gs::rgb4(8, 10, 12),
            gs::rgb4(3, 4, 5), gs::rgb4(5, 6, 3), gs::rgb4(8, 8, 4), gs::rgb4(2, 2, 2), gs::rgb4(4, 4, 3),
            gs::rgb4(1, 1, 1), gs::rgb4(12, 8, 2), gs::rgb4(9, 10, 5), gs::rgb4(14, 13, 8), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_TENDER,
           {0, gs::rgb4(14, 12, 4), gs::rgb4(11, 8, 2), gs::rgb4(6, 4, 2), gs::rgb4(8, 6, 3), gs::rgb4(6, 8, 10),
            gs::rgb4(4, 4, 3), gs::rgb4(13, 10, 3), gs::rgb4(2, 2, 2), gs::rgb4(9, 7, 3), gs::rgb4(1, 1, 1),
            gs::rgb4(12, 9, 4), gs::rgb4(7, 5, 2), gs::rgb4(15, 13, 6), gs::rgb4(5, 4, 2), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_CHAIN,
           {0, gs::rgb4(8, 8, 8), gs::rgb4(5, 5, 5), gs::rgb4(3, 3, 3), gs::rgb4(12, 3, 2), gs::rgb4(14, 14, 12),
            gs::rgb4(6, 6, 6), gs::rgb4(9, 9, 8), gs::rgb4(4, 4, 4), gs::rgb4(7, 7, 6), gs::rgb4(1, 1, 1),
            gs::rgb4(10, 10, 9), gs::rgb4(2, 2, 2), gs::rgb4(11, 6, 3), gs::rgb4(13, 12, 10), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_CRANE,
           {0, gs::rgb4(9, 9, 8), gs::rgb4(5, 5, 5), gs::rgb4(12, 8, 3), gs::rgb4(14, 4, 2), gs::rgb4(4, 4, 4),
            gs::rgb4(7, 6, 4), gs::rgb4(3, 3, 3), gs::rgb4(8, 8, 7), gs::rgb4(2, 2, 2), gs::rgb4(11, 10, 8),
            gs::rgb4(1, 1, 1), gs::rgb4(6, 5, 4), gs::rgb4(13, 11, 6), gs::rgb4(10, 7, 3), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_SHED,
           {0, gs::rgb4(9, 7, 5), gs::rgb4(6, 4, 3), gs::rgb4(11, 6, 3), gs::rgb4(4, 6, 8), gs::rgb4(8, 9, 10),
            gs::rgb4(3, 3, 3), gs::rgb4(12, 10, 7), gs::rgb4(5, 4, 3), gs::rgb4(2, 2, 2), gs::rgb4(10, 8, 6),
            gs::rgb4(1, 1, 1), gs::rgb4(7, 5, 4), gs::rgb4(13, 11, 8), gs::rgb4(4, 3, 2), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_BUOY,
           {0, gs::rgb4(14, 3, 2), gs::rgb4(12, 10, 2), gs::rgb4(4, 4, 4), gs::rgb4(15, 14, 8), gs::rgb4(8, 2, 1),
            gs::rgb4(3, 3, 3), gs::rgb4(11, 8, 2), gs::rgb4(6, 5, 3), gs::rgb4(2, 2, 2), gs::rgb4(13, 6, 3),
            gs::rgb4(1, 1, 1), gs::rgb4(9, 4, 2), gs::rgb4(15, 12, 4), gs::rgb4(7, 3, 2), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_FX,
           {0, gs::rgb4(14, 14, 13), gs::rgb4(10, 12, 13), gs::rgb4(8, 9, 10), gs::rgb4(6, 7, 8), gs::rgb4(12, 13, 14),
            gs::rgb4(5, 6, 7), gs::rgb4(9, 10, 11), gs::rgb4(4, 5, 6), gs::rgb4(13, 14, 15), gs::rgb4(3, 4, 5),
            gs::rgb4(1, 1, 2), gs::rgb4(11, 12, 13), gs::rgb4(7, 8, 9), gs::rgb4(15, 15, 14), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_LINK,
           {0, gs::rgb4(13, 3, 2), gs::rgb4(15, 15, 13), gs::rgb4(8, 2, 1), gs::rgb4(12, 12, 10), gs::rgb4(6, 1, 1),
            gs::rgb4(10, 10, 9), gs::rgb4(14, 6, 4), gs::rgb4(4, 4, 4), gs::rgb4(9, 3, 2), gs::rgb4(11, 11, 10),
            gs::rgb4(1, 1, 1), gs::rgb4(7, 7, 6), gs::rgb4(15, 8, 5), gs::rgb4(5, 5, 5), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_LAMP,
           {0, gs::rgb4(15, 14, 6), gs::rgb4(12, 6, 2), gs::rgb4(6, 8, 4), gs::rgb4(14, 4, 2), gs::rgb4(8, 10, 4),
            gs::rgb4(4, 4, 2), gs::rgb4(15, 10, 3), gs::rgb4(3, 3, 2), gs::rgb4(10, 12, 5), gs::rgb4(7, 4, 1),
            gs::rgb4(1, 1, 1), gs::rgb4(13, 12, 5), gs::rgb4(9, 5, 2), gs::rgb4(15, 15, 10), gs::rgb4(2, 2, 1)});
    setPal(vdp, PAL_GULL,
           {0, gs::rgb4(15, 15, 14), gs::rgb4(8, 8, 8), gs::rgb4(12, 12, 11), gs::rgb4(4, 4, 4), gs::rgb4(10, 10, 9),
            gs::rgb4(6, 6, 6), gs::rgb4(14, 14, 13), gs::rgb4(3, 3, 3), gs::rgb4(11, 11, 10), gs::rgb4(5, 5, 5),
            gs::rgb4(1, 1, 1), gs::rgb4(9, 9, 8), gs::rgb4(13, 13, 12), gs::rgb4(7, 7, 7), gs::rgb4(2, 2, 2)});

    int r = PAL_ROAD * 16;
    vdp.setColor(r + 0, 0);
    vdp.setColor(r + 1, gs::rgb4(7, 7, 5));
    vdp.setColor(r + 2, gs::rgb4(4, 5, 3));
    vdp.setColor(r + 3, gs::rgb4(9, 8, 5));
    vdp.setColor(r + 4, gs::rgb4(8, 7, 5));
    vdp.setColor(r + 5, gs::rgb4(5, 4, 3));
    vdp.setColor(r + 6, gs::rgb4(3, 3, 4));
    vdp.setColor(r + 7, gs::rgb4(5, 5, 6));
    vdp.setColor(r + 8, gs::rgb4(6, 6, 5));
    vdp.setColor(r + 9, gs::rgb4(2, 2, 3));
    vdp.setColor(r + 10, gs::rgb4(4, 4, 5));
    vdp.setColor(r + 11, gs::rgb4(2, 5, 8));
    vdp.setColor(r + 12, gs::rgb4(3, 6, 9));
    vdp.setColor(r + 13, gs::rgb4(7, 10, 12));
    vdp.setColor(r + 14, gs::rgb4(13, 12, 6));
    vdp.setColor(r + 15, gs::rgb4(6, 6, 7));
    vdp.setFogColor(gs::rgb4(7, 9, 11));

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, tiles, art);
    art.lorry = gs::uploadMipped(vdp, lorryArt());
    art.tender = gs::uploadMipped(vdp, tenderArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.link = gs::uploadMipped(vdp, linkArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.crane = gs::uploadMipped(vdp, craneArt());
    art.shed = gs::uploadMipped(vdp, shedArt());
    art.buoy = gs::uploadMipped(vdp, buoyArt());
    art.gull = gs::uploadMipped(vdp, gullArt());
    art.puff = gs::uploadMipped(vdp, puffArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
}

}  // namespace hcol

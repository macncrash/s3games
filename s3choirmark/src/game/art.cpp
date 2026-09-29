#include "art.h"

#include <initializer_list>

namespace choirmark {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
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
    }
}

gs::Bitmap robe(int kind, bool open) {
    gs::Bitmap b(28, 44);
    const int cx = 14;
    const int sh = kind == 2 ? 11 : kind == 1 ? 9 : 8;
    b.ellipse(float(cx), 42, float(sh), 2, 14);
    b.poly({{float(cx - sh), 18}, {float(cx + sh), 18}, {float(cx + sh - 2), 40}, {float(cx - sh + 2), 40}}, 2);
    b.rect(cx - 2, 16, 4, 4, 4);
    b.ellipse(float(cx), 11, 5.5f, 6, 4);
    if (kind == 0) b.ellipse(float(cx), 6, 6, 3.5f, 5);
    else if (kind == 1) {
        b.rect(cx - 6, 8, 2, 10, 5);
        b.rect(cx + 4, 8, 2, 10, 5);
    } else {
        b.rect(cx - 5, 5, 10, 3, 5);
        b.ellipse(float(cx), 15, 5, 3, 5);
    }
    b.set(cx - 2, 11, 7);
    b.set(cx + 2, 11, 7);
    if (open) b.rect(cx - 2, 14, 4, 2, 6);
    else b.rect(cx - 2, 14, 4, 1, 6);
    b.outline(15, true);
    return b;
}

gs::Bitmap noteHead() {
    gs::Bitmap b(10, 12);
    b.ellipse(4, 8, 3.2f, 2.4f, 1);
    b.rect(6, 1, 2, 8, 1);
    b.outline(15, false);
    return b;
}

gs::Bitmap fermata() {
    gs::Bitmap b(18, 12);
    b.ellipse(9, 6, 7, 4, 1);
    b.ellipse(9, 6, 4, 2, 0);
    b.ellipse(9, 10, 1.4f, 1.2f, 1);
    b.outline(15, false);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_INK, {0, gs::rgb4(14, 13, 11), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 1, 3)});
    setPal(vdp, PAL_NAVE, {0, gs::rgb4(6, 4, 3), gs::rgb4(9, 6, 4), gs::rgb4(12, 9, 6), gs::rgb4(4, 3, 5),
                           gs::rgb4(15, 14, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_TREBLE, {0, gs::rgb4(15, 12, 6), gs::rgb4(12, 8, 14), gs::rgb4(8, 5, 10), gs::rgb4(15, 12, 10),
                             gs::rgb4(14, 10, 4), gs::rgb4(8, 2, 4), gs::rgb4(2, 1, 2), gs::rgb4(15, 15, 12), 0, 0, 0,
                             0, 0, gs::rgb4(3, 2, 2), gs::rgb4(1, 0, 1)});
    setPal(vdp, PAL_ALTO, {0, gs::rgb4(8, 14, 12), gs::rgb4(6, 10, 12), gs::rgb4(4, 7, 9), gs::rgb4(14, 11, 9),
                           gs::rgb4(10, 8, 6), gs::rgb4(6, 2, 3), gs::rgb4(2, 1, 2), gs::rgb4(12, 15, 14), 0, 0, 0, 0,
                           0, gs::rgb4(2, 3, 3), gs::rgb4(0, 1, 1)});
    setPal(vdp, PAL_BASS, {0, gs::rgb4(8, 9, 15), gs::rgb4(5, 6, 12), gs::rgb4(3, 4, 8), gs::rgb4(13, 11, 9),
                           gs::rgb4(8, 6, 5), gs::rgb4(4, 2, 3), gs::rgb4(1, 1, 2), gs::rgb4(11, 12, 15), 0, 0, 0, 0,
                           0, gs::rgb4(2, 2, 3), gs::rgb4(0, 0, 1)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 4), gs::rgb4(12, 8, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           gs::rgb4(4, 2, 0)});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 15, 12), gs::rgb4(10, 12, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         gs::rgb4(2, 2, 4)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 0, 0)});
    vdp.setFogColor(gs::rgb4(2, 1, 4));
    loadFont(vdp, art);
    for (int i = 0; i < 3; i++) {
        art.singer[i][0] = gs::uploadMipped(vdp, robe(i, false));
        art.singer[i][1] = gs::uploadMipped(vdp, robe(i, true));
    }
    art.note = gs::uploadMipped(vdp, noteHead());
    art.mark = gs::uploadMipped(vdp, fermata());
    art.title = gs::uploadMipped(vdp, gs::textBitmap("S3 CHOIRMARK", {2, 1, 2, 0, 1}));
    art.done = gs::uploadMipped(vdp, gs::textBitmap("FINISHED MARK", {2, 1, 2, 0, 1}));
    gs::Bitmap rule(2, 2);
    rule.rect(0, 0, 2, 2, 1);
    art.bar = gs::uploadImage(vdp, rule);
}

}  // namespace choirmark

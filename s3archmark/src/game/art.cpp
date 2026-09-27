#include "game/art.h"

#include <cmath>
#include <initializer_list>

namespace archmark {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink, uint16_t edge) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, ink);
    vdp.setColor(pal * 16 + 2, edge);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
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

int facePix(int x, int y) {
    float dx = float(x) + 0.5f - 75.f;
    float dy = float(y) + 0.5f - 75.f;
    float r = std::sqrt(dx * dx + dy * dy);
    if (r > 72.f) return 0;
    if (r > 68.f) return 1;
    if (std::fabs(dx) < 1.2f && r < 12.f) return 8;
    if (std::fabs(dy) < 1.2f && r < 12.f) return 8;
    if (r <= 14.f) return 6;
    if (r <= 28.f) return 4;
    if (r <= 42.f) return 3;
    if (r <= 56.f) return 2;
    return 7;
}

gs::Bitmap faceArt() {
    gs::Bitmap b(150, 150);
    for (int y = 0; y < b.h; y++)
        for (int x = 0; x < b.w; x++) b.set(x, y, facePix(x, y));
    return b;
}

gs::Bitmap archerArt() {
    gs::Bitmap b(36, 64);
    b.ellipse(16.f, 8.f, 6.f, 6.f, 1);
    b.rect(12.f, 14.f, 8.f, 16.f, 2);
    b.rect(6.f, 18.f, 8.f, 3.f, 3);
    b.rect(18.f, 16.f, 14.f, 2.f, 4);
    b.line(18.f, 16.f, 32.f, 28.f, 4, 2.f);
    b.line(32.f, 28.f, 20.f, 36.f, 4, 2.f);
    b.rect(11.f, 30.f, 4.f, 22.f, 2);
    b.rect(17.f, 30.f, 4.f, 22.f, 2);
    b.rect(9.f, 50.f, 6.f, 4.f, 5);
    b.rect(17.f, 50.f, 6.f, 4.f, 5);
    return b;
}

gs::Bitmap arrowArt() {
    gs::Bitmap b(28, 6);
    b.rect(0, 2, 20, 2, 1);
    b.poly({{20.f, 0.f}, {27.f, 3.f}, {20.f, 6.f}}, 2);
    b.rect(1, 0, 3, 2, 3);
    b.rect(1, 4, 3, 2, 3);
    return b;
}

gs::Bitmap coinArt() {
    gs::Bitmap b(14, 14);
    b.ellipse(7.f, 7.f, 6.f, 6.f, 1);
    b.ellipse(7.f, 7.f, 3.2f, 3.2f, 2);
    return b;
}

gs::Bitmap handArt() {
    gs::Bitmap b(16, 14);
    b.ellipse(8.f, 8.f, 6.f, 5.f, 1);
    b.rect(3.f, 2.f, 2.f, 5.f, 1);
    b.rect(6.f, 1.f, 2.f, 5.f, 1);
    b.rect(9.f, 2.f, 2.f, 5.f, 1);
    return b;
}

gs::Bitmap sightArt() {
    gs::Bitmap b(11, 11);
    b.line(5, 0, 5, 10, 1, 1.f);
    b.line(0, 5, 10, 5, 1, 1.f);
    b.ellipse(5.f, 5.f, 3.f, 3.f, 2);
    return b;
}

gs::Bitmap baleArt() {
    gs::Bitmap b(40, 18);
    b.rect(0, 2, 40, 14, 1);
    b.rect(0, 2, 40, 3, 2);
    b.line(8, 4, 8, 15, 3, 1.f);
    b.line(20, 4, 20, 15, 3, 1.f);
    b.line(32, 4, 32, 15, 3, 1.f);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(22, 40);
    b.rect(9, 22, 4, 16, 1);
    b.ellipse(11.f, 14.f, 10.f, 12.f, 2);
    b.ellipse(8.f, 12.f, 4.f, 4.f, 3);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_FACE,
           {0, gs::rgb4(8, 6, 3), gs::rgb4(2, 2, 2), gs::rgb4(2, 4, 12), gs::rgb4(12, 2, 2), gs::rgb4(14, 12, 3),
            gs::rgb4(15, 13, 4), gs::rgb4(14, 14, 12), gs::rgb4(4, 2, 1)});
    setPal(vdp, PAL_ARCH,
           {0, gs::rgb4(13, 9, 6), gs::rgb4(3, 5, 10), gs::rgb4(12, 8, 5), gs::rgb4(6, 4, 2), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_ARROW, {0, gs::rgb4(10, 7, 3), gs::rgb4(12, 12, 11), gs::rgb4(14, 3, 3)});
    setPal(vdp, PAL_COIN, {0, gs::rgb4(15, 12, 3), gs::rgb4(12, 8, 1)});
    setPal(vdp, PAL_SIGHT, {0, gs::rgb4(15, 15, 15), gs::rgb4(15, 4, 3)});
    setPal(vdp, PAL_HAND, {0, gs::rgb4(13, 9, 6)});
    setPal(vdp, PAL_WORLD, {0, gs::rgb4(10, 8, 3), gs::rgb4(12, 10, 4), gs::rgb4(6, 4, 2), gs::rgb4(2, 8, 3),
                            gs::rgb4(4, 11, 4)});
    textPal(vdp, PAL_INK, gs::rgb4(15, 15, 14), gs::rgb4(1, 1, 2));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 13, 4), gs::rgb4(2, 1, 0));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 6, 4), gs::rgb4(2, 0, 0));
    textPal(vdp, PAL_TITLE, gs::rgb4(15, 14, 8), gs::rgb4(3, 1, 1));
    textPal(vdp, PAL_WIN, gs::rgb4(15, 15, 12), gs::rgb4(1, 4, 1));

    loadFont(vdp, art);
    art.face = gs::uploadImage(vdp, faceArt());
    art.archer = gs::uploadImage(vdp, archerArt());
    art.arrow = gs::uploadImage(vdp, arrowArt());
    art.coin = gs::uploadImage(vdp, coinArt());
    art.hand = gs::uploadImage(vdp, handArt());
    art.sight = gs::uploadImage(vdp, sightArt());
    art.bale = gs::uploadImage(vdp, baleArt());
    art.tree = gs::uploadImage(vdp, treeArt());
    art.title = gs::uploadImage(vdp, gs::textBitmap("S3 ARCHMARK", {2, 1, 2, 0, 1}));
    art.win = gs::uploadImage(vdp, gs::textBitmap("FINISHED MARK", {2, 1, 2, 0, 1}));
    gs::Bitmap dot(3, 3);
    dot.ellipse(1.f, 1.f, 1.2f, 1.2f, 1);
    art.dot = gs::uploadImage(vdp, dot);
}

}  // namespace archmark

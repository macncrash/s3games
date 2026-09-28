#include "game/art.h"

namespace lot {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, ink);
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

gs::Bitmap lotArt() {
    gs::Bitmap b(320, 96);
    for (int y = 0; y < b.h; y++) {
        for (int x = 0; x < b.w; x++) {
            uint32_t h = uint32_t(x) * 374761393u ^ uint32_t(y) * 668265263u;
            h ^= h >> 13;
            int crack = ((h ^ (h >> 7)) & 31u) == 0;
            int seam = (y % 24 == 0) || ((x + (y / 24) * 11) % 40 == 0);
            int c = 1;
            if (seam) c = 2;
            else if (crack) c = 3;
            else if (((x / 8 + y / 6) & 1) == 0) c = 4;
            b.set(x, y, c);
        }
    }
    return b;
}

gs::Bitmap fenceArt() {
    gs::Bitmap b(320, 36);
    for (int x = 0; x < b.w; x += 10) b.rect(float(x), 4, 3, 32, 1);
    b.rect(0, 8, 320, 3, 2);
    b.rect(0, 22, 320, 3, 2);
    for (int x = 4; x < b.w; x += 18) b.rect(float(x), 0, 2, 8, 3);
    return b;
}

void body(gs::Bitmap& b, int ox, bool step) {
    b.ellipse(ox + 10, 8, 6, 7, 1);
    b.rect(ox + 6, 14, 9, 16, 1);
    b.rect(ox + 4, 16, 4, 12, 2);
    b.rect(ox + 13, 16, 4, 12, 2);
    int ly = step ? 0 : 4;
    b.rect(ox + 6, 30, 4, 18 + (step ? 4 : 0), 1);
    b.rect(ox + 12, 30 + ly, 4, 18 - ly, 1);
    b.rect(ox + 5, 46 + (step ? 4 : 0), 6, 3, 3);
    b.rect(ox + 11, 46 + ly, 6, 3, 3);
}

gs::Bitmap youArt() {
    gs::Bitmap b(36, 56);
    body(b, 4, false);
    b.rect(18, 22, 14, 3, 2);
    b.rect(30, 18, 4, 6, 4);
    b.set(33, 17, 5);
    return b;
}

gs::Bitmap rivalArt(bool step) {
    gs::Bitmap b(28, 56);
    body(b, 4, step);
    b.rect(2, 20, 6, 3, 2);
    return b;
}

gs::Bitmap markArt() {
    gs::Bitmap b(14, 8);
    b.line(1, 6, 7, 1, 1, 2);
    b.line(7, 1, 13, 6, 1, 2);
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(16, 10);
    b.ellipse(8, 6, 7, 3, 1);
    b.ellipse(5, 4, 2, 2, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_LOT,
           {0, gs::rgb4(4, 4, 5), gs::rgb4(2, 2, 3), gs::rgb4(6, 6, 6), gs::rgb4(5, 5, 4), gs::rgb4(8, 7, 5), 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_MAN,
           {0, gs::rgb4(2, 2, 3), gs::rgb4(6, 5, 4), gs::rgb4(1, 1, 1), gs::rgb4(8, 7, 5), gs::rgb4(14, 12, 6), 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0});
    textPal(vdp, PAL_INK, gs::rgb4(14, 13, 11));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 13, 4));
    textPal(vdp, PAL_RED, gs::rgb4(15, 4, 3));
    textPal(vdp, PAL_GREEN, gs::rgb4(5, 15, 7));
    setPal(vdp, PAL_FLASH, {0, gs::rgb4(15, 15, 10), gs::rgb4(15, 10, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_DUST, {0, gs::rgb4(8, 7, 6), gs::rgb4(12, 11, 9), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    vdp.setColor(0, 0);

    loadFont(vdp, art);
    art.lot = gs::uploadImage(vdp, lotArt());
    art.fence = gs::uploadImage(vdp, fenceArt());
    art.you = gs::uploadImage(vdp, youArt());
    art.rival[0] = gs::uploadImage(vdp, rivalArt(false));
    art.rival[1] = gs::uploadImage(vdp, rivalArt(true));
    art.mark = gs::uploadImage(vdp, markArt());
    art.dust = gs::uploadImage(vdp, dustArt());
}

}  // namespace lot

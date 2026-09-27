#include "game/art.h"

#include <initializer_list>

namespace golfseven {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 0, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink, uint16_t edge) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, ink);
    vdp.setColor(pal * 16 + 15, edge);
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

gs::Bitmap ballArt() {
    gs::Bitmap b(12, 12);
    b.ellipse(6, 6, 4.6f, 4.6f, 1);
    b.ellipse(4.6f, 4.2f, 1.5f, 1.1f, 2);
    b.set(7, 7, 3);
    b.set(5, 8, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap flagArt() {
    gs::Bitmap b(22, 40);
    b.rect(3, 6, 2, 32, 3);
    b.rect(5, 6, 14, 10, 1);
    b.rect(5, 6, 14, 2, 2);
    b.line(5, 16, 19, 11, 2, 1);
    b.rect(4, 36, 4, 3, 4);
    return b;
}

gs::Bitmap manArt(int swing) {
    gs::Bitmap b(22, 32);
    b.ellipse(11, 5, 3.2f, 3.2f, 4);
    b.rect(9, 2, 5, 2, 2);
    b.rect(8, 9, 7, 9, 1);
    b.rect(8, 17, 7, 3, 3);
    if (swing == 0) {
        b.line(8, 11, 3, 16, 4, 1.4f);
        b.line(15, 11, 19, 18, 5, 1.4f);
        b.line(17, 18, 21, 16, 6, 1.2f);
    } else {
        b.line(8, 11, 4, 6, 4, 1.4f);
        b.line(14, 10, 20, 4, 5, 1.4f);
        b.line(18, 5, 21, 8, 6, 1.2f);
    }
    b.line(10, 20, 8, 28, 3, 1.6f);
    b.line(14, 20, 16, 28, 3, 1.6f);
    b.rect(6, 28, 4, 2, 5);
    b.rect(14, 28, 4, 2, 5);
    return b;
}

gs::Bitmap turfArt() {
    gs::Bitmap b(48, 14);
    b.rect(0, 2, 48, 12, 1);
    for (int x = 0; x < 48; x++) {
        b.set(x, 1, 3);
        if ((x / 4) & 1) b.set(x, 4, 2);
        else b.set(x, 8, 2);
    }
    return b;
}

gs::Bitmap waterArt() {
    gs::Bitmap b(32, 12);
    b.rect(0, 2, 32, 10, 1);
    for (int x = 0; x < 32; x += 4) {
        b.set(x, 4, 2);
        b.set(x + 1, 4, 2);
        b.set(x + 2, 7, 3);
    }
    return b;
}

gs::Bitmap sandArt() {
    gs::Bitmap b(28, 12);
    b.ellipse(14, 8, 13, 5, 4);
    for (int i = 0; i < 6; i++) b.set(4 + i * 4, 6 + (i & 1), 5);
    return b;
}

gs::Bitmap cupArt() {
    gs::Bitmap b(14, 8);
    b.ellipse(7, 4, 6, 2.4f, 1);
    b.ellipse(7, 4, 3.2f, 1.3f, 2);
    return b;
}

gs::Bitmap cloudArt() {
    gs::Bitmap b(28, 12);
    b.ellipse(10, 7, 8, 4, 1);
    b.ellipse(18, 6, 7, 4.2f, 1);
    b.ellipse(14, 5, 5, 3, 2);
    return b;
}

gs::Image phrase(gs::VDP& vdp, const char* s, int scale) {
    return gs::uploadImage(vdp, gs::textBitmap(s, {scale, 1, 2, 0, 1}));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_LINKS, {gs::rgb4(0, 0, 0), gs::rgb4(6, 12, 5), gs::rgb4(3, 8, 3), gs::rgb4(12, 10, 4),
                            gs::rgb4(2, 6, 12), gs::rgb4(14, 14, 15)});
    textPal(vdp, PAL_INK, gs::rgb4(15, 15, 14), gs::rgb4(1, 2, 3));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 13, 4), gs::rgb4(4, 2, 0));
    textPal(vdp, PAL_GREEN, gs::rgb4(8, 15, 7), gs::rgb4(0, 3, 1));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 6, 4), gs::rgb4(3, 0, 0));
    setPal(vdp, PAL_BALL, {gs::rgb4(0, 0, 0), gs::rgb4(15, 15, 15), gs::rgb4(12, 13, 14), gs::rgb4(7, 8, 9),
                           gs::rgb4(1, 2, 4)});
    setPal(vdp, PAL_FLAG, {gs::rgb4(0, 0, 0), gs::rgb4(15, 12, 2), gs::rgb4(10, 6, 1), gs::rgb4(14, 14, 13),
                           gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_MAN, {gs::rgb4(0, 0, 0), gs::rgb4(14, 13, 10), gs::rgb4(12, 2, 2), gs::rgb4(2, 3, 7),
                          gs::rgb4(13, 9, 6), gs::rgb4(3, 2, 2), gs::rgb4(6, 4, 2)});
    setPal(vdp, PAL_TURF, {gs::rgb4(0, 0, 0), gs::rgb4(4, 12, 4), gs::rgb4(2, 8, 2), gs::rgb4(8, 14, 6)});
    setPal(vdp, PAL_HAZ, {gs::rgb4(0, 0, 0), gs::rgb4(3, 8, 14), gs::rgb4(8, 13, 15), gs::rgb4(2, 5, 10),
                          gs::rgb4(14, 12, 6), gs::rgb4(10, 8, 3)});
    setPal(vdp, PAL_SKY, {gs::rgb4(0, 0, 0), gs::rgb4(15, 15, 15), gs::rgb4(13, 14, 15)});
    textPal(vdp, PAL_TITLE, gs::rgb4(15, 15, 14), gs::rgb4(1, 4, 2));
    setPal(vdp, PAL_CUP, {gs::rgb4(0, 0, 0), gs::rgb4(1, 2, 2), gs::rgb4(0, 0, 0)});

    loadFont(vdp, art);
    art.ball = gs::uploadImage(vdp, ballArt());
    art.flag = gs::uploadImage(vdp, flagArt());
    art.man[0] = gs::uploadImage(vdp, manArt(0));
    art.man[1] = gs::uploadImage(vdp, manArt(1));
    art.turf = gs::uploadImage(vdp, turfArt());
    art.water = gs::uploadImage(vdp, waterArt());
    art.sand = gs::uploadImage(vdp, sandArt());
    art.cup = gs::uploadImage(vdp, cupArt());
    art.cloud = gs::uploadImage(vdp, cloudArt());
    art.title = phrase(vdp, "GOLF SEVEN", 3);
    art.win = phrase(vdp, "FIRST TO SEVEN", 2);
    art.lose = phrase(vdp, "HOUSE TO SEVEN", 2);
    vdp.setFogColor(gs::rgb4(6, 10, 14));
}

}  // namespace golfseven

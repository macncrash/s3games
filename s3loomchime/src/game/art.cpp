#include "game/art.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>

namespace loomchime {
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
}

gs::Bitmap frameArt() {
    gs::Bitmap b(156, 170);
    b.rect(0, 0, 12, 170, 2);
    b.rect(144, 0, 12, 170, 2);
    b.rect(0, 0, 6, 170, 1);
    b.rect(150, 0, 6, 170, 3);
    b.rect(6, 6, 144, 10, 2);
    b.rect(6, 6, 144, 3, 3);
    b.rect(6, 154, 144, 12, 2);
    b.rect(6, 162, 144, 4, 1);
    b.rect(64, 0, 28, 8, 4);
    for (int y = 24; y < 150; y += 8) {
        b.set(3, y, 4);
        b.set(152, y, 4);
    }
    return b;
}

gs::Bitmap warpArt() {
    gs::Bitmap b(3, 88);
    b.rect(1, 0, 1, 88, 1);
    b.rect(0, 0, 1, 88, 2);
    for (int y = 3; y < 88; y += 6) b.set(2, y, 3);
    return b;
}

gs::Bitmap shuttleArt() {
    gs::Bitmap b(34, 12);
    b.ellipse(17.f, 6.f, 16.f, 5.f, 1);
    b.ellipse(17.f, 5.4f, 11.f, 2.4f, 2);
    b.rect(10, 4, 14, 3, 3);
    b.ellipse(11.f, 6.f, 2.2f, 2.2f, 4);
    b.ellipse(23.f, 6.f, 2.2f, 2.2f, 4);
    b.rect(16, 2, 2, 6, 5);
    return b;
}

gs::Bitmap reedArt() {
    gs::Bitmap b(112, 10);
    b.rect(0, 0, 112, 2, 1);
    b.rect(0, 8, 112, 2, 1);
    for (int x = 4; x < 108; x += 4) b.rect(float(x), 2, 1, 6, 2);
    b.rect(0, 0, 4, 10, 3);
    b.rect(108, 0, 4, 10, 3);
    return b;
}

gs::Bitmap heddleArt() {
    gs::Bitmap b(7, 18);
    b.rect(3, 0, 1, 18, 1);
    b.rect(1, 7, 5, 5, 2);
    b.set(3, 8, 0);
    b.set(3, 9, 0);
    b.set(3, 10, 0);
    return b;
}

gs::Bitmap pickArt() {
    gs::Bitmap b(14, 6);
    b.rect(0, 0, 14, 6, 1);
    b.rect(0, 0, 14, 2, 2);
    b.rect(0, 4, 14, 2, 3);
    for (int x = 1; x < 14; x += 3) b.set(x, 2, 4);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(20, 24);
    b.rect(8, 0, 4, 4, 1);
    b.ellipse(10.f, 14.f, 9.f, 8.f, 2);
    b.ellipse(10.f, 12.f, 6.f, 5.f, 3);
    b.rect(2, 18, 16, 3, 4);
    b.rect(7, 20, 6, 3, 1);
    return b;
}

gs::Bitmap clapperArt() {
    gs::Bitmap b(5, 8);
    b.rect(2, 0, 1, 4, 1);
    b.ellipse(2.5f, 6.f, 2.f, 2.f, 2);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(8, 10);
    b.rect(3, 0, 2, 3, 1);
    b.ellipse(4.f, 6.5f, 3.2f, 3.f, 2);
    b.ellipse(4.f, 6.f, 1.4f, 1.4f, 3);
    return b;
}

gs::Bitmap beamArt() {
    gs::Bitmap b(128, 8);
    b.rect(0, 2, 128, 4, 1);
    b.rect(0, 2, 128, 1, 2);
    b.ellipse(7.f, 4.f, 5.f, 3.2f, 3);
    b.ellipse(121.f, 4.f, 5.f, 3.2f, 3);
    return b;
}

gs::Bitmap faceArt() {
    gs::Bitmap b(46, 46);
    b.ellipse(23.f, 23.f, 22.f, 22.f, 1);
    b.ellipse(23.f, 23.f, 18.f, 18.f, 2);
    b.ellipse(23.f, 23.f, 16.f, 16.f, 3);
    for (int i = 0; i < 12; i++) {
        float a = float(i) * 3.14159265f / 6.f;
        int x = int(23.f + std::cos(a) * 15.f);
        int y = int(23.f + std::sin(a) * 15.f);
        b.set(x, y, 4);
    }
    b.ellipse(23.f, 23.f, 2.f, 2.f, 4);
    return b;
}

gs::Bitmap dotArt() {
    gs::Bitmap b(3, 3);
    b.ellipse(1.f, 1.f, 1.2f, 1.2f, 1);
    return b;
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
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 2;
                }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(4, 2, 1), gs::rgb4(8, 5, 2), gs::rgb4(12, 8, 4), gs::rgb4(6, 5, 3)});
    setPal(vdp, PAL_WARP, {0, gs::rgb4(12, 11, 8), gs::rgb4(15, 14, 11), gs::rgb4(7, 6, 4)});
    setPal(vdp, PAL_SHUTTLE,
           {0, gs::rgb4(5, 3, 1), gs::rgb4(10, 7, 3), gs::rgb4(14, 12, 7), gs::rgb4(7, 1, 2), gs::rgb4(13, 4, 3)});
    setPal(vdp, PAL_BELL, {0, gs::rgb4(7, 6, 2), gs::rgb4(13, 11, 4), gs::rgb4(15, 14, 7), gs::rgb4(9, 7, 2)});
    setPal(vdp, PAL_CLOTH, {0, gs::rgb4(10, 4, 3), gs::rgb4(14, 8, 5), gs::rgb4(6, 2, 2), gs::rgb4(15, 13, 8)});
    setPal(vdp, PAL_CLOCK, {0, gs::rgb4(3, 3, 4), gs::rgb4(8, 8, 9), gs::rgb4(13, 13, 12), gs::rgb4(15, 12, 5)});
    setPal(vdp, PAL_HAND, {0, gs::rgb4(15, 13, 6)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(4, 4, 5), gs::rgb4(11, 9, 3), gs::rgb4(15, 14, 8)});
    textPal(vdp, PAL_INK, gs::rgb4(15, 14, 12), gs::rgb4(2, 1, 1));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 13, 5), gs::rgb4(3, 2, 0));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 5, 4), gs::rgb4(3, 0, 0));
    textPal(vdp, PAL_DIM, gs::rgb4(7, 6, 5), gs::rgb4(1, 1, 1));

    loadFont(vdp, art);
    art.frame = gs::uploadImage(vdp, frameArt());
    art.warp = gs::uploadImage(vdp, warpArt());
    art.shuttle = gs::uploadImage(vdp, shuttleArt());
    art.reed = gs::uploadImage(vdp, reedArt());
    art.heddle = gs::uploadImage(vdp, heddleArt());
    art.pick = gs::uploadImage(vdp, pickArt());
    art.bell = gs::uploadImage(vdp, bellArt());
    art.clapper = gs::uploadImage(vdp, clapperArt());
    art.lamp = gs::uploadImage(vdp, lampArt());
    art.beam = gs::uploadImage(vdp, beamArt());
    art.face = gs::uploadImage(vdp, faceArt());
    art.dot = gs::uploadImage(vdp, dotArt());
}

}  // namespace loomchime

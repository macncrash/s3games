#include "game/art.h"

#include <cstdint>
#include <initializer_list>

namespace loombell {
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
    gs::Bitmap b(168, 188);
    b.rect(0, 0, 14, 188, 2);
    b.rect(154, 0, 14, 188, 2);
    b.rect(0, 0, 14, 188, 1);
    b.rect(160, 0, 8, 188, 3);
    b.rect(8, 8, 152, 12, 2);
    b.rect(8, 8, 152, 3, 3);
    b.rect(8, 168, 152, 14, 2);
    b.rect(8, 176, 152, 4, 1);
    b.rect(70, 0, 28, 10, 4);
    b.rect(78, 0, 12, 6, 5);
    for (int y = 28; y < 164; y += 6) {
        b.set(4, y, 4);
        b.set(162, y, 4);
    }
    return b;
}

gs::Bitmap warpArt() {
    gs::Bitmap b(3, 96);
    b.rect(1, 0, 1, 96, 1);
    b.rect(0, 0, 1, 96, 2);
    for (int y = 2; y < 96; y += 5) b.set(2, y, 3);
    return b;
}

gs::Bitmap shuttleArt() {
    gs::Bitmap b(28, 12);
    b.ellipse(14.f, 6.f, 13.f, 5.f, 1);
    b.ellipse(14.f, 5.2f, 9.f, 2.6f, 2);
    b.rect(8, 4, 12, 3, 3);
    b.ellipse(9.f, 6.f, 2.4f, 2.4f, 4);
    b.ellipse(19.f, 6.f, 2.4f, 2.4f, 4);
    b.rect(13, 3, 2, 5, 5);
    return b;
}

gs::Bitmap reedArt() {
    gs::Bitmap b(120, 12);
    b.rect(0, 0, 120, 3, 1);
    b.rect(0, 9, 120, 3, 1);
    for (int x = 3; x < 118; x += 5) b.rect(float(x), 2, 1, 8, 2);
    b.rect(0, 0, 4, 12, 3);
    b.rect(116, 0, 4, 12, 3);
    return b;
}

gs::Bitmap heddleArt() {
    gs::Bitmap b(7, 16);
    b.rect(3, 0, 1, 16, 1);
    b.rect(1, 6, 5, 5, 2);
    b.set(3, 7, 0);
    b.set(3, 8, 0);
    b.set(3, 9, 0);
    return b;
}

gs::Bitmap pickArt() {
    gs::Bitmap b(14, 7);
    b.rect(0, 1, 14, 5, 1);
    b.rect(0, 1, 14, 2, 2);
    b.rect(0, 4, 14, 2, 3);
    for (int x = 1; x < 14; x += 3) b.rect(float(x), 2, 1, 3, 4);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(22, 26);
    b.rect(9, 0, 4, 5, 1);
    b.ellipse(11.f, 16.f, 10.f, 9.f, 2);
    b.ellipse(11.f, 14.f, 7.f, 6.f, 3);
    b.rect(3, 20, 16, 3, 4);
    b.rect(8, 22, 6, 3, 1);
    return b;
}

gs::Bitmap clapperArt() {
    gs::Bitmap b(6, 8);
    b.rect(2, 0, 2, 4, 1);
    b.ellipse(3.f, 6.f, 2.4f, 2.2f, 2);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(8, 10);
    b.rect(3, 0, 2, 3, 1);
    b.ellipse(4.f, 6.5f, 3.4f, 3.2f, 2);
    b.ellipse(4.f, 6.f, 1.6f, 1.6f, 3);
    return b;
}

gs::Bitmap beamArt() {
    gs::Bitmap b(140, 8);
    b.rect(0, 2, 140, 4, 1);
    b.rect(0, 2, 140, 1, 2);
    b.ellipse(8.f, 4.f, 6.f, 3.5f, 3);
    b.ellipse(132.f, 4.f, 6.f, 3.5f, 3);
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
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(3, 2, 1), gs::rgb4(7, 4, 2), gs::rgb4(11, 7, 3), gs::rgb4(5, 4, 3),
                           gs::rgb4(13, 11, 8)});
    setPal(vdp, PAL_WARP, {0, gs::rgb4(13, 12, 9), gs::rgb4(15, 14, 11), gs::rgb4(8, 7, 5)});
    setPal(vdp, PAL_SHUTTLE,
           {0, gs::rgb4(4, 2, 1), gs::rgb4(9, 6, 3), gs::rgb4(14, 11, 6), gs::rgb4(6, 1, 3), gs::rgb4(12, 3, 4)});
    setPal(vdp, PAL_BELL, {0, gs::rgb4(8, 6, 2), gs::rgb4(14, 11, 3), gs::rgb4(15, 14, 6), gs::rgb4(10, 8, 2)});
    textPal(vdp, PAL_INK, gs::rgb4(15, 14, 12), gs::rgb4(2, 1, 1));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 13, 5), gs::rgb4(3, 2, 0));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 5, 4), gs::rgb4(3, 0, 0));
    textPal(vdp, PAL_DIM, gs::rgb4(7, 6, 5), gs::rgb4(1, 1, 1));
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(4, 4, 5), gs::rgb4(12, 10, 4), gs::rgb4(15, 14, 8)});
    setPal(vdp, PAL_CLOTH,
           {0, gs::rgb4(9, 3, 4), gs::rgb4(14, 6, 6), gs::rgb4(6, 1, 2), gs::rgb4(15, 12, 8)});

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
}

}  // namespace loombell

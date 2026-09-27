#include "game/art.h"

#include <initializer_list>

namespace skate {
namespace {

void pal(gs::VDP& v, int p, std::initializer_list<uint16_t> cols) {
    int i = 0;
    for (uint16_t c : cols) {
        v.setColor(p * 16 + i, c);
        i++;
        if (i >= 16) break;
    }
}

void font(gs::VDP& v, Art& a) {
    uint8_t px[64];
    for (int c = 32; c < 128; c++) {
        std::memset(px, 0, sizeof px);
        const uint8_t* g = gs::glyph(char(c));
        if (g) {
            for (int y = 0; y < 7; y++) {
                for (int x = 0; x < 5; x++) {
                    if (g[y] & (1 << (4 - x))) px[y * 8 + x] = 1;
                }
            }
        }
        int t = 1 + (c - 32);
        v.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

gs::Bitmap skaterBmp() {
    gs::Bitmap b(20, 36);
    b.rect(7, 1, 6, 6, 2);     // head
    b.rect(8, 6, 4, 2, 3);     // neck
    b.rect(5, 8, 10, 10, 4);   // shirt
    b.rect(3, 10, 3, 7, 3);    // arm
    b.rect(14, 10, 3, 7, 3);
    b.rect(6, 18, 4, 10, 5);   // legs
    b.rect(11, 18, 4, 10, 5);
    b.rect(5, 27, 5, 3, 6);    // shoes
    b.rect(11, 27, 5, 3, 6);
    b.set(8, 3, 1);
    b.set(11, 3, 1);
    return b;
}

gs::Bitmap boardBmp() {
    gs::Bitmap b(26, 8);
    b.rect(1, 2, 24, 3, 2);
    b.rect(0, 3, 2, 2, 3);
    b.rect(24, 3, 2, 2, 3);
    b.ellipse(6, 6, 2.2f, 2.2f, 4);
    b.ellipse(20, 6, 2.2f, 2.2f, 4);
    b.set(6, 6, 1);
    b.set(20, 6, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    using gs::rgb4;
    pal(vdp, PAL_INK, {0, rgb4(1, 1, 2), rgb4(15, 15, 14), rgb4(8, 8, 9)});
    pal(vdp, PAL_GOLD, {0, rgb4(6, 4, 1), rgb4(15, 12, 3), rgb4(10, 7, 2)});
    pal(vdp, PAL_GOOD, {0, rgb4(1, 6, 2), rgb4(8, 15, 6), rgb4(2, 8, 3)});
    pal(vdp, PAL_BAD, {0, rgb4(6, 1, 1), rgb4(15, 5, 4), rgb4(8, 2, 2)});
    pal(vdp, PAL_YOU, {0, rgb4(1, 1, 1), rgb4(14, 11, 8), rgb4(12, 5, 4), rgb4(2, 6, 12), rgb4(3, 3, 8), rgb4(1, 1, 2)});
    pal(vdp, PAL_DECK, {0, rgb4(2, 2, 2), rgb4(12, 8, 3), rgb4(8, 4, 2), rgb4(4, 4, 5)});
    pal(vdp, PAL_TAPE, {0, rgb4(1, 1, 2), rgb4(12, 4, 6), rgb4(3, 3, 4), rgb4(15, 14, 10), rgb4(6, 6, 7)});
    pal(vdp, PAL_STREET, {0, rgb4(3, 3, 4), rgb4(5, 5, 6), rgb4(8, 7, 4), rgb4(2, 5, 3)});
    pal(vdp, PAL_PAPER, {0, rgb4(4, 3, 2), rgb4(15, 14, 11), rgb4(2, 2, 3)});
    pal(vdp, PAL_WOOD, {0, rgb4(4, 2, 1), rgb4(10, 6, 2), rgb4(6, 3, 1)});
    pal(vdp, PAL_WIN, {0, rgb4(1, 4, 3), rgb4(12, 15, 8), rgb4(15, 15, 15)});
    vdp.setFogColor(rgb4(6, 7, 10));
    font(vdp, art);

    art.skater = gs::uploadMipped(vdp, skaterBmp());
    art.board = gs::uploadMipped(vdp, boardBmp());

    gs::Bitmap post(8, 28);
    post.rect(3, 0, 2, 28, 2);
    post.rect(1, 0, 6, 4, 3);
    art.post = gs::uploadImage(vdp, post);

    gs::Bitmap cas(36, 22);
    cas.rect(0, 0, 36, 22, 2);
    cas.rect(2, 2, 32, 18, 3);
    cas.ellipse(12, 11, 5, 5, 1);
    cas.ellipse(24, 11, 5, 5, 1);
    cas.rect(14, 9, 8, 4, 4);
    cas.rect(4, 18, 28, 2, 5);
    art.cassette = gs::uploadImage(vdp, cas);

    gs::Bitmap dr(168, 28);
    dr.rect(0, 0, 168, 28, 2);
    dr.rect(2, 2, 164, 22, 3);
    art.drawer = gs::uploadImage(vdp, dr);

    gs::Bitmap slot(48, 18);
    slot.rect(0, 0, 48, 18, 1);
    slot.rect(1, 1, 46, 16, 2);
    art.slot = gs::uploadImage(vdp, slot);

    for (int i = 0; i < kTapeN; i++) {
        gs::Bitmap slip(44, 14);
        int ink = 2 + (i % 2);
        slip.rect(0, 0, 44, 14, ink);
        slip.rect(2, 2, 40, 10, 4);
        art.slip[i] = gs::uploadImage(vdp, slip);
    }

    gs::Bitmap wheel(6, 6);
    wheel.ellipse(3, 3, 2.5f, 2.5f, 1);
    art.wheel = gs::uploadImage(vdp, wheel);

    gs::Bitmap asphalt(8, 8);
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) asphalt.set(x, y, ((x + y * 3) % 7 == 0) ? 2 : 1);
    gs::TileAlloc tiles(vdp, 128);
    int cell = tiles.shared(asphalt.px.data());
    vdp.B.resize(64, 32);
    vdp.B.clear();
    for (int y = 20; y < 28; y++)
        for (int x = 0; x < 64; x++) vdp.B.set(x, y, gs::entry(cell, PAL_STREET));

    gs::Bitmap curb(8, 8);
    curb.rect(0, 0, 8, 3, 3);
    curb.rect(0, 3, 8, 5, 4);
    int curbT = tiles.shared(curb.px.data());
    for (int x = 0; x < 64; x++) vdp.B.set(x, 19, gs::entry(curbT, PAL_STREET));
}

}  // namespace skate

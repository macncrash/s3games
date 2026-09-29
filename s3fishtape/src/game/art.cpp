#include "game/art.h"

#include <cstdint>
#include <initializer_list>

namespace fishtape {
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

gs::Image phrase(gs::VDP& vdp, const char* s, int scale) {
    return gs::uploadImage(vdp, gs::textBitmap(s, {scale, 1, 2, 0, 1}));
}

void paintAngler(gs::Bitmap& b, int pose) {
    b.ellipse(12, 8, 4.2f, 4.4f, 2);
    b.rect(9, 3, 7, 3, 5);
    b.rect(8, 12, 9, 11, 3);
    b.rect(8, 12, 3, 11, 4);
    b.rect(9, 22, 4, 12, 6);
    b.rect(15, 22, 4, 12, 6);
    b.rect(8, 33, 5, 3, 7);
    b.rect(15, 33, 5, 3, 7);
    if (pose == 1) {
        b.line(16, 14, 26, 6, 8, 2.2f);
        b.ellipse(27, 5, 1.6f, 1.6f, 9);
        b.rect(10, 22, 3, 12, 6);
        b.rect(16, 23, 3, 11, 6);
    } else if (pose == 2) {
        b.line(14, 14, 20, 4, 8, 2.2f);
        b.rect(8, 22, 3, 13, 6);
        b.rect(17, 24, 3, 10, 6);
        b.rect(7, 34, 5, 3, 7);
        b.rect(16, 33, 5, 3, 7);
    } else {
        b.line(15, 13, 22, 2, 8, 2.2f);
        b.ellipse(23, 2, 1.4f, 1.4f, 9);
    }
    b.outline(1, false);
}

void paintFlat(gs::Bitmap& b) {
    b.ellipse(16, 7, 12.f, 4.2f, 2);
    b.ellipse(16, 8, 8.f, 2.2f, 3);
    b.rect(24, 5, 6, 3, 4);
    b.rect(6, 4, 4, 2, 4);
    b.rect(12, 5, 2, 2, 5);
    b.outline(1, false);
}

void paintDeep(gs::Bitmap& b) {
    b.ellipse(16, 8, 12.f, 5.4f, 2);
    b.ellipse(15, 9, 7.f, 2.6f, 3);
    b.poly({{26, 6}, {32, 3}, {32, 13}, {26, 10}}, 4);
    b.rect(8, 3, 5, 3, 4);
    b.rect(14, 6, 2, 2, 5);
    b.outline(1, false);
}

void paintLong(gs::Bitmap& b) {
    b.ellipse(20, 7, 16.f, 4.f, 2);
    b.ellipse(18, 8, 10.f, 2.f, 3);
    b.poly({{34, 5}, {40, 2}, {40, 12}, {34, 9}}, 4);
    b.rect(10, 3, 3, 3, 4);
    b.rect(16, 5, 2, 2, 5);
    b.rect(6, 4, 3, 2, 4);
    b.outline(1, false);
}

void paintTwin(gs::Bitmap& b) {
    b.ellipse(12, 7, 8.f, 4.f, 2);
    b.ellipse(11, 8, 4.f, 1.8f, 3);
    b.rect(18, 5, 4, 3, 4);
    b.rect(8, 5, 2, 2, 5);
    b.outline(1, false);
}

void paintLure(gs::Bitmap& b) {
    b.ellipse(4, 3, 2.4f, 2.4f, 2);
    b.rect(3, 5, 2, 3, 3);
    b.outline(1, false);
}

void paintCreel(gs::Bitmap& b) {
    b.rect(2, 4, 68, 12, 3);
    b.rect(4, 6, 64, 8, 4);
    b.rect(8, 2, 10, 4, 5);
    b.rect(30, 2, 10, 4, 5);
    b.rect(52, 2, 10, 4, 5);
    b.line(2, 10, 70, 10, 2, 1.f);
    b.outline(1, false);
}

void paintTape(gs::Bitmap& b) {
    b.rect(0, 2, 140, 10, 3);
    b.rect(2, 4, 136, 6, 4);
    for (int i = 0; i < 7; i++) b.rect(8 + i * 18, 4, 2, 6, 2);
    b.outline(1, false);
}

void paintSlip(gs::Bitmap& b, int which) {
    if (which == 0) b.ellipse(10, 5, 8.f, 3.f, 3);
    else if (which == 1) b.ellipse(10, 5, 8.f, 3.6f, 4);
    else b.ellipse(11, 5, 9.f, 2.6f, 5);
    b.rect(16, 3, 3, 3, 2);
    b.rect(6, 4, 1, 1, 6);
    b.outline(1, false);
}

void paintJetty(gs::Bitmap& b) {
    b.rect(0, 2, 88, 10, 3);
    for (int i = 0; i < 6; i++) b.line(4 + i * 14, 2, 4 + i * 14, 12, 2, 1.f);
    b.rect(6, 12, 4, 6, 4);
    b.rect(40, 12, 4, 6, 4);
    b.rect(74, 12, 4, 6, 4);
    b.outline(1, false);
}

void paintReed(gs::Bitmap& b) {
    b.line(6, 28, 4, 6, 2, 2.f);
    b.line(6, 28, 10, 2, 3, 2.f);
    b.line(6, 26, 2, 12, 4, 1.4f);
    b.ellipse(4, 5, 2.f, 3.f, 2);
    b.ellipse(10, 2, 2.f, 3.f, 3);
}

void paintSun(gs::Bitmap& b) {
    b.ellipse(8, 8, 5.f, 5.f, 2);
    b.outline(1, false);
}

void paintCloud(gs::Bitmap& b) {
    b.ellipse(10, 8, 7.f, 4.f, 2);
    b.ellipse(18, 7, 6.f, 4.f, 2);
    b.outline(1, false);
}

void paintBlot(gs::Bitmap& b) { b.ellipse(3, 3, 2.6f, 2.6f, 1); }

void fishPal(gs::VDP& vdp, int pal, uint16_t body, uint16_t belly, uint16_t fin, uint16_t eye) {
    setPal(vdp, pal,
           {0, gs::rgb4(1, 1, 2), body, belly, fin, eye, gs::rgb4(15, 15, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0});
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_INK, gs::rgb4(2, 3, 6), gs::rgb4(1, 1, 2));
    textPal(vdp, PAL_GOLD, gs::rgb4(14, 11, 3), gs::rgb4(6, 4, 1));
    textPal(vdp, PAL_GOOD, gs::rgb4(3, 13, 5), gs::rgb4(1, 4, 2));
    textPal(vdp, PAL_BAD, gs::rgb4(13, 3, 3), gs::rgb4(5, 1, 1));
    setPal(vdp, PAL_SKY, {0, gs::rgb4(8, 8, 10), gs::rgb4(15, 15, 15), 0});
    setPal(vdp, PAL_KIT,
           {0, gs::rgb4(2, 2, 3), gs::rgb4(13, 9, 6), gs::rgb4(2, 6, 10), gs::rgb4(1, 3, 6), gs::rgb4(6, 4, 2),
            gs::rgb4(3, 3, 4), gs::rgb4(4, 3, 2), gs::rgb4(5, 3, 1), gs::rgb4(12, 10, 4)});
    fishPal(vdp, PAL_DAB, gs::rgb4(11, 8, 4), gs::rgb4(14, 12, 8), gs::rgb4(8, 6, 3), gs::rgb4(1, 1, 2));
    fishPal(vdp, PAL_BASS, gs::rgb4(3, 9, 4), gs::rgb4(12, 14, 10), gs::rgb4(2, 6, 3), gs::rgb4(1, 1, 1));
    fishPal(vdp, PAL_PIKE, gs::rgb4(5, 8, 3), gs::rgb4(12, 13, 8), gs::rgb4(2, 5, 2), gs::rgb4(14, 12, 2));
    fishPal(vdp, PAL_TWIN, gs::rgb4(7, 5, 3), gs::rgb4(11, 9, 6), gs::rgb4(4, 3, 2), gs::rgb4(2, 1, 1));
    setPal(vdp, PAL_WOOD,
           {0, gs::rgb4(3, 2, 1), gs::rgb4(6, 4, 2), gs::rgb4(10, 7, 3), gs::rgb4(7, 5, 2), gs::rgb4(12, 9, 4)});
    setPal(vdp, PAL_WATER, {0, gs::rgb4(1, 4, 8), gs::rgb4(4, 9, 12), gs::rgb4(8, 13, 14)});
    setPal(vdp, PAL_PAPER,
           {0, gs::rgb4(4, 3, 2), gs::rgb4(8, 6, 3), gs::rgb4(13, 11, 7), gs::rgb4(4, 10, 5), gs::rgb4(6, 8, 3),
            gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_REED, {0, gs::rgb4(1, 3, 1), gs::rgb4(3, 8, 2), gs::rgb4(6, 11, 3), gs::rgb4(2, 6, 2)});
    setPal(vdp, PAL_LURE, {0, gs::rgb4(3, 2, 1), gs::rgb4(14, 4, 3), gs::rgb4(12, 10, 2)});
    setPal(vdp, PAL_SUN, {0, gs::rgb4(12, 8, 2), gs::rgb4(15, 14, 6)});

    loadFont(vdp, art);
    art.logo = phrase(vdp, "FISHTAPE", 2);
    art.matchW = phrase(vdp, "DRAWER MATCHES", 1);
    art.openW = phrase(vdp, "STILL OPEN", 1);

    for (int i = 0; i < 3; i++) {
        gs::Bitmap b(30, 40);
        paintAngler(b, i);
        art.angler[i] = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(32, 14);
        paintFlat(b);
        art.dab = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(34, 16);
        paintDeep(b);
        art.bass = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(42, 14);
        paintLong(b);
        art.pike = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(24, 14);
        paintTwin(b);
        art.twin = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(8, 10);
        paintLure(b);
        art.lure = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(72, 18);
        paintCreel(b);
        art.creel = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(140, 14);
        paintTape(b);
        art.tape = gs::uploadImage(vdp, b);
    }
    for (int i = 0; i < kTapeN; i++) {
        gs::Bitmap b(22, 10);
        paintSlip(b, i);
        art.slip[i] = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(88, 20);
        paintJetty(b);
        art.jetty = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(14, 30);
        paintReed(b);
        art.reed = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(16, 16);
        paintSun(b);
        art.sun = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(28, 14);
        paintCloud(b);
        art.cloud = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(6, 6);
        paintBlot(b);
        art.blot = gs::uploadImage(vdp, b);
    }
}

}  // namespace fishtape

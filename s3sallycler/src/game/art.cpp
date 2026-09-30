#include "game/art.h"

#include <cstring>
#include <initializer_list>

#include "console/gfx.h"

namespace sally {

static void pal(gs::VDP& v, int p, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        v.setColor(p * 16 + i, c);
        i++;
    }
}

static void paintBody(gs::Bitmap& b) {
    b.rect(10, 1, 12, 3, 5);
    b.ellipse(16, 12, 6, 7, 2);
    b.rect(14, 10, 2, 2, 1);
    b.rect(13, 15, 5, 1, 1);
    b.rect(13, 18, 6, 3, 8);
    b.rect(8, 21, 16, 16, 3);
    b.rect(10, 23, 5, 8, 4);
    b.rect(6, 24, 4, 10, 3);
    b.rect(14, 37, 4, 12, 6);
    b.rect(20, 37, 4, 12, 6);
    b.rect(13, 48, 5, 3, 7);
    b.rect(19, 48, 5, 3, 7);
    b.outline(1, false);
}

static void paintBroom(gs::Bitmap& b) {
    b.line(3, 2, 16, 22, 9, 2);
    b.rect(12, 20, 10, 3, 6);
    b.rect(13, 23, 2, 8, 4);
    b.rect(16, 23, 2, 9, 5);
    b.rect(19, 23, 2, 8, 4);
    b.outline(1, false);
}

static void paintHeap(gs::Bitmap& b) {
    b.ellipse(16, 16, 13, 7, 3);
    b.ellipse(11, 12, 6, 6, 4);
    b.ellipse(20, 11, 7, 6, 2);
    b.ellipse(16, 9, 4, 4, 5);
    b.rect(8, 18, 16, 3, 6);
    b.outline(1, false);
}

static void paintClean(gs::Bitmap& b) {
    b.ellipse(8, 8, 3, 2, 2);
    b.ellipse(16, 6, 2, 2, 3);
    b.rect(6, 12, 12, 2, 4);
}

static void paintSun(gs::Bitmap& b) {
    b.ellipse(16, 16, 13, 13, 2);
    b.ellipse(16, 16, 7, 7, 3);
}

static void paintTree(gs::Bitmap& b) {
    b.rect(8, 18, 4, 14, 4);
    b.ellipse(10, 12, 8, 10, 2);
    b.ellipse(10, 8, 4, 5, 3);
}

static void loadFont(gs::VDP& vdp) {
    uint8_t px[64];
    for (int ch = 32; ch < 128; ch++) {
        std::memset(px, 0, sizeof px);
        const uint8_t* g = gs::glyph(char(ch));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) px[y * 8 + x + 1] = 1;
        vdp.loadTile(1 + (ch - 32), px);
    }
}

static void loadGround(gs::VDP& vdp) {
    uint8_t grass[64];
    uint8_t lip[64];
    std::memset(grass, 2, sizeof grass);
    std::memset(lip, 3, sizeof lip);
    for (int i = 0; i < 64; i++) {
        if ((i * 17 + 3) % 11 == 0) grass[i] = 4;
        if ((i * 9 + 5) % 13 == 0) grass[i] = 1;
        if (i < 8) lip[i] = 5;
    }
    vdp.loadTile(200, grass);
    vdp.loadTile(201, lip);
}

void Art::build(gs::VDP& vdp) {
    using gs::rgb4;
    pal(vdp, PAL_INK, {0, rgb4(14, 13, 10)});
    pal(vdp, PAL_GOLD, {0, rgb4(15, 12, 3)});
    pal(vdp, PAL_BAD, {0, rgb4(15, 4, 3)});
    pal(vdp, PAL_GOOD, {0, rgb4(6, 14, 6)});

    pal(vdp, PAL_YOU, {0, rgb4(1, 1, 2), rgb4(13, 9, 6), rgb4(2, 4, 9), rgb4(1, 2, 6), rgb4(2, 2, 3),
                       rgb4(4, 5, 3), rgb4(3, 2, 1), rgb4(14, 14, 12), rgb4(8, 6, 3), rgb4(10, 8, 4)});
    pal(vdp, PAL_HEAP, {0, rgb4(2, 1, 1), rgb4(6, 8, 2), rgb4(4, 5, 1), rgb4(8, 6, 2), rgb4(10, 8, 3),
                        rgb4(5, 3, 1), rgb4(3, 2, 1)});
    pal(vdp, PAL_FX, {0, rgb4(4, 4, 4), rgb4(14, 14, 10), rgb4(15, 15, 15), rgb4(6, 8, 3)});
    pal(vdp, PAL_SUN, {0, rgb4(12, 5, 2), rgb4(15, 10, 3), rgb4(15, 14, 8)});
    pal(vdp, PAL_GND, {0, rgb4(2, 4, 1), rgb4(3, 7, 2), rgb4(5, 4, 2), rgb4(4, 9, 3), rgb4(7, 6, 3)});

    loadFont(vdp);
    loadGround(vdp);
    fontBase = 1;

    gs::Bitmap body(32, 52);
    paintBody(body);
    this->body = gs::uploadImage(vdp, body);

    gs::Bitmap broom(24, 32);
    paintBroom(broom);
    this->broom = gs::uploadImage(vdp, broom);

    gs::Bitmap heap(32, 24);
    paintHeap(heap);
    this->heap = gs::uploadImage(vdp, heap);

    gs::Bitmap clean(24, 16);
    paintClean(clean);
    this->clean = gs::uploadImage(vdp, clean);

    gs::Bitmap sun(32, 32);
    paintSun(sun);
    this->sun = gs::uploadImage(vdp, sun);

    gs::Bitmap tree(20, 36);
    paintTree(tree);
    this->tree = gs::uploadImage(vdp, tree);
}

}  // namespace sally

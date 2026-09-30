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
    b.rect(16, 2, 18, 4, 5);
    b.rect(12, 6, 26, 3, 5);
    b.rect(18, 9, 12, 5, 5);
    b.ellipse(24, 20, 7, 8, 2);
    b.rect(21, 18, 2, 2, 1);
    b.rect(18, 23, 5, 1, 1);
    b.rect(20, 27, 8, 5, 8);
    b.rect(12, 32, 24, 26, 3);
    b.rect(14, 34, 6, 16, 4);
    b.rect(8, 36, 6, 14, 3);
    b.rect(6, 48, 8, 4, 3);
    b.rect(15, 58, 8, 16, 6);
    b.rect(25, 58, 8, 16, 6);
    b.rect(13, 72, 10, 5, 7);
    b.rect(25, 72, 10, 5, 7);
    b.outline(1, false);
}

static void paintGun(gs::Bitmap& b) {
    b.rect(2, 2, 16, 3, 9);
    b.rect(14, 4, 6, 3, 10);
    b.rect(4, 5, 3, 3, 10);
    b.rect(0, 2, 3, 2, 1);
    b.outline(1, false);
}

static void paintSmoke(gs::Bitmap& b) {
    b.ellipse(8, 8, 6, 5, 2);
    b.ellipse(16, 7, 7, 5, 3);
    b.ellipse(12, 11, 5, 4, 1);
}

static void paintSun(gs::Bitmap& b) {
    b.ellipse(16, 16, 14, 14, 2);
    b.ellipse(16, 16, 8, 8, 3);
}

static void paintTree(gs::Bitmap& b) {
    b.rect(8, 22, 4, 16, 4);
    b.ellipse(10, 14, 9, 12, 2);
    b.ellipse(10, 10, 5, 6, 3);
}

static void paintPip(gs::Bitmap& b) {
    b.ellipse(6, 6, 5, 5, 2);
    b.outline(1, false);
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
    uint8_t dirt[64];
    std::memset(grass, 2, sizeof grass);
    std::memset(dirt, 3, sizeof dirt);
    for (int i = 0; i < 64; i++) {
        if ((i * 17 + 3) % 11 == 0) grass[i] = 4;
        if ((i * 9 + 5) % 13 == 0) grass[i] = 1;
        if (i < 8) dirt[i] = 5;
    }
    vdp.loadTile(200, grass);
    vdp.loadTile(201, dirt);
}

void Art::build(gs::VDP& vdp) {
    using gs::rgb4;
    pal(vdp, PAL_INK, {0, rgb4(14, 13, 10)});
    pal(vdp, PAL_GOLD, {0, rgb4(15, 12, 3)});
    pal(vdp, PAL_BAD, {0, rgb4(15, 4, 3)});
    pal(vdp, PAL_GOOD, {0, rgb4(6, 14, 6)});

    pal(vdp, PAL_YOU, {0, rgb4(1, 1, 2), rgb4(13, 9, 6), rgb4(2, 3, 8), rgb4(1, 2, 5), rgb4(2, 2, 3),
                       rgb4(3, 3, 5), rgb4(2, 1, 1), rgb4(14, 14, 12), rgb4(10, 10, 11), rgb4(8, 5, 2)});
    pal(vdp, PAL_FOE, {0, rgb4(1, 1, 2), rgb4(13, 9, 6), rgb4(10, 2, 2), rgb4(6, 1, 1), rgb4(3, 1, 1),
                       rgb4(4, 3, 3), rgb4(2, 1, 1), rgb4(12, 12, 10), rgb4(10, 10, 11), rgb4(8, 5, 2)});
    pal(vdp, PAL_FX, {0, rgb4(4, 4, 5), rgb4(12, 12, 12), rgb4(15, 15, 15), rgb4(6, 5, 4)});
    pal(vdp, PAL_SUN, {0, rgb4(12, 5, 2), rgb4(15, 10, 3), rgb4(15, 14, 8)});
    pal(vdp, PAL_GND, {0, rgb4(2, 4, 1), rgb4(3, 7, 2), rgb4(5, 4, 2), rgb4(4, 9, 3), rgb4(7, 6, 3)});

    loadFont(vdp);
    loadGround(vdp);
    fontBase = 1;

    gs::Bitmap body(48, 80);
    paintBody(body);
    this->body = gs::uploadImage(vdp, body);

    gs::Bitmap gun(24, 10);
    paintGun(gun);
    this->gun = gs::uploadImage(vdp, gun);

    gs::Bitmap smoke(24, 16);
    paintSmoke(smoke);
    this->smoke = gs::uploadImage(vdp, smoke);

    gs::Bitmap sun(32, 32);
    paintSun(sun);
    this->sun = gs::uploadImage(vdp, sun);

    gs::Bitmap tree(20, 40);
    paintTree(tree);
    this->tree = gs::uploadImage(vdp, tree);

    gs::Bitmap pip(12, 12);
    paintPip(pip);
    this->pip = gs::uploadImage(vdp, pip);
}

}  // namespace sally

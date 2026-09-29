#include "game/art.h"

namespace bedstape {
namespace {

void setPal(gs::VDP& vdp, int p, const uint16_t c[16]) {
    for (int i = 0; i < 16; i++) vdp.setColor(p * 16 + i, c[i]);
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) {
                    px[y * 8 + x + 1] = 1;
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
                }
        a.font[c - 32] = tiles.shared(px);
    }
}

gs::Mipped say(gs::VDP& vdp, const char* s, int scale, int color, int outline) {
    gs::TextStyle st;
    st.scale = scale;
    st.color = color;
    st.outline = outline;
    st.shadow = 0;
    return gs::uploadMipped(vdp, gs::textBitmap(s, st));
}

gs::Bitmap bedArt() {
    gs::Bitmap b(64, 28);
    b.rect(1, 4, 62, 6, 2);
    b.rect(1, 4, 62, 2, 7);
    b.rect(1, 10, 5, 14, 3);
    b.rect(58, 10, 5, 14, 4);
    b.rect(2, 22, 60, 4, 3);
    b.rect(6, 11, 52, 11, 5);
    b.outline(1, false);
    return b;
}

gs::Bitmap soilArt() {
    gs::Bitmap b(48, 12);
    b.rect(0, 1, 48, 10, 2);
    b.rect(0, 1, 48, 3, 6);
    for (int i = 0; i < 7; i++) b.set(3 + i * 6, 7, 3);
    b.outline(1, false);
    return b;
}

gs::Bitmap herbArt(int kind) {
    gs::Bitmap b(36, 32);
    if (kind == 0) {
        b.rect(17, 16, 3, 14, 3);
        b.ellipse(10, 16, 8, 4, 2);
        b.ellipse(26, 18, 7, 4, 4);
        b.ellipse(16, 10, 6, 5, 5);
        b.ellipse(22, 12, 4, 3, 6);
    } else if (kind == 1) {
        b.rect(17, 8, 2, 22, 3);
        for (int i = 0; i < 5; i++) {
            b.line(18, 10.f + i * 4, 8, 6.f + i * 4, 2, 1);
            b.line(18, 10.f + i * 4, 28, 6.f + i * 4, 4, 1);
        }
    } else if (kind == 2) {
        b.rect(17, 14, 3, 16, 3);
        b.ellipse(12, 16, 6, 5, 2);
        b.ellipse(24, 15, 6, 5, 5);
        b.ellipse(18, 9, 5, 4, 4);
        b.ellipse(14, 22, 4, 3, 6);
    } else {
        b.rect(17, 12, 3, 18, 3);
        b.ellipse(11, 14, 7, 3, 2);
        b.ellipse(25, 16, 6, 3, 4);
        b.ellipse(14, 22, 6, 3, 5);
        b.ellipse(23, 24, 5, 3, 2);
    }
    b.outline(1, false);
    return b;
}

gs::Bitmap manArt(bool pour) {
    gs::Bitmap b(26, 34);
    b.ellipse(13, 7, 6, 6, 2);
    b.rect(9, 13, 8, 11, 4);
    b.rect(8, 15, 10, 5, 5);
    b.rect(9, 24, 3, 8, 6);
    b.rect(14, 24, 3, 8, 6);
    b.set(11, 7, 1);
    b.set(15, 7, 1);
    if (pour) b.rect(17, 15, 7, 3, 2);
    else b.rect(5, 15, 4, 8, 2);
    b.outline(1, false);
    return b;
}

gs::Bitmap canArt() {
    gs::Bitmap b(16, 12);
    b.rect(1, 3, 10, 8, 2);
    b.rect(2, 4, 8, 3, 3);
    b.rect(10, 5, 4, 2, 4);
    b.rect(11, 1, 2, 5, 4);
    b.outline(1, false);
    return b;
}

gs::Bitmap dropArt() {
    gs::Bitmap b(5, 7);
    b.ellipse(2, 4, 2, 2, 2);
    b.set(2, 1, 3);
    return b;
}

gs::Bitmap slipArt() {
    gs::Bitmap b(28, 16);
    b.rect(0, 0, 28, 16, 2);
    b.rect(2, 2, 24, 3, 3);
    b.rect(2, 7, 16, 2, 4);
    b.rect(2, 11, 10, 2, 4);
    b.outline(1, false);
    return b;
}

gs::Bitmap drawerArt() {
    gs::Bitmap b(96, 22);
    b.rect(0, 0, 96, 22, 2);
    b.rect(2, 2, 92, 4, 3);
    b.rect(40, 12, 16, 4, 4);
    b.outline(1, false);
    return b;
}

void paintYard(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
    uint8_t g[64], p[64];
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) {
            int n = (x * 5 + y * 3) & 7;
            g[y * 8 + x] = n == 0 ? 3 : 2;
            p[y * 8 + x] = ((x + y) & 3) == 0 ? 3 : 2;
        }
    a.grass = tiles.shared(g);
    a.path = tiles.shared(p);
    vdp.B.resize(64, 32);
    vdp.B.clear();
    for (int y = 0; y < 28; y++)
        for (int x = 0; x < 40; x++) {
            bool path = y >= 16;
            vdp.B.set(x, y, gs::entry(path ? a.path : a.grass, path ? PAL_PATH : PAL_YARD));
        }
    vdp.A.clear();
    vdp.A.enabled = false;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    auto pal = [&](int p, const uint16_t* c) { setPal(vdp, p, c); };
    uint16_t hud[16] = {0, gs::rgb4(15, 15, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 3, 4)};
    uint16_t gold[16] = {0, gs::rgb4(15, 13, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(5, 3, 0)};
    uint16_t warn[16] = {0, gs::rgb4(15, 5, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(5, 1, 1)};
    uint16_t good[16] = {0, gs::rgb4(6, 15, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(0, 4, 1)};
    uint16_t dim[16] = {0, gs::rgb4(10, 11, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 2, 3)};
    uint16_t wood[16] = {0, gs::rgb4(2, 1, 1), gs::rgb4(12, 8, 3), gs::rgb4(8, 5, 2), gs::rgb4(5, 3, 1), gs::rgb4(6, 4, 2),
                         gs::rgb4(14, 12, 8), gs::rgb4(15, 13, 8), 0, 0, 0, 0, 0, 0, 0, 0};
    uint16_t soil[16] = {0, gs::rgb4(2, 1, 1), gs::rgb4(10, 7, 3), gs::rgb4(6, 4, 2), gs::rgb4(4, 8, 3), gs::rgb4(3, 5, 2),
                         gs::rgb4(13, 10, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0};
    uint16_t leaf[16] = {0, gs::rgb4(1, 2, 1), gs::rgb4(4, 12, 3), gs::rgb4(2, 8, 2), gs::rgb4(7, 14, 4), gs::rgb4(12, 15, 5),
                         gs::rgb4(14, 6, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0};
    uint16_t sage[16] = {0, gs::rgb4(2, 2, 2), gs::rgb4(8, 10, 7), gs::rgb4(5, 7, 5), gs::rgb4(10, 12, 8), gs::rgb4(12, 13, 9),
                         gs::rgb4(6, 8, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0};
    uint16_t man[16] = {0, gs::rgb4(2, 1, 1), gs::rgb4(14, 11, 7), gs::rgb4(9, 6, 4), gs::rgb4(2, 7, 12), gs::rgb4(1, 4, 8),
                        gs::rgb4(4, 3, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0};
    uint16_t water[16] = {0, gs::rgb4(1, 2, 4), gs::rgb4(8, 14, 15), gs::rgb4(3, 9, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    uint16_t yard[16] = {0, gs::rgb4(1, 3, 1), gs::rgb4(3, 9, 3), gs::rgb4(6, 12, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    uint16_t path[16] = {0, gs::rgb4(3, 2, 2), gs::rgb4(8, 7, 5), gs::rgb4(11, 10, 7), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    uint16_t can[16] = {0, gs::rgb4(1, 2, 2), gs::rgb4(11, 13, 14), gs::rgb4(6, 8, 9), gs::rgb4(4, 5, 6), 0, 0, 0, 0, 0, 0, 0,
                        0, 0, 0, 0};
    uint16_t sun[16] = {0, gs::rgb4(8, 4, 1), gs::rgb4(15, 12, 3), gs::rgb4(15, 8, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    uint16_t drawer[16] = {0, gs::rgb4(2, 1, 1), gs::rgb4(9, 6, 3), gs::rgb4(13, 10, 6), gs::rgb4(6, 4, 2), 0, 0, 0, 0, 0, 0,
                           0, 0, 0, 0, 0};
    pal(PAL_HUD, hud);
    pal(PAL_GOLD, gold);
    pal(PAL_WARN, warn);
    pal(PAL_GOOD, good);
    pal(PAL_DIM, dim);
    pal(PAL_WOOD, wood);
    pal(PAL_SOIL, soil);
    pal(PAL_LEAF, leaf);
    pal(PAL_SAGE, sage);
    pal(PAL_MAN, man);
    pal(PAL_WATER, water);
    pal(PAL_YARD, yard);
    pal(PAL_PATH, path);
    pal(PAL_CAN, can);
    pal(PAL_SUN, sun);
    pal(PAL_DRAWER, drawer);

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    paintYard(vdp, tiles, art);

    art.title = say(vdp, "BEDSTAPE", 3, 1, 15);
    art.sub = say(vdp, "MATCH THE TAPE", 2, 1, 15);
    art.win = say(vdp, "DRAWER MATCHES", 2, 1, 15);
    art.late = say(vdp, "STILL OPEN", 2, 1, 15);
    art.bed = gs::uploadMipped(vdp, bedArt());
    art.soil = gs::uploadMipped(vdp, soilArt());
    for (int i = 0; i < kHerbs; i++) art.herb[i] = gs::uploadMipped(vdp, herbArt(i));
    art.man[0] = gs::uploadMipped(vdp, manArt(false));
    art.man[1] = gs::uploadMipped(vdp, manArt(true));
    art.can = gs::uploadMipped(vdp, canArt());
    art.drop = gs::uploadMipped(vdp, dropArt());
    art.slip = gs::uploadMipped(vdp, slipArt());
    art.drawer = gs::uploadMipped(vdp, drawerArt());
    gs::Bitmap sh(20, 6);
    sh.ellipse(10, 3, 9, 2, 1);
    art.shade = gs::uploadMipped(vdp, sh);
    vdp.HUD.enabled = true;
    vdp.hudEnabled = true;
}

}  // namespace bedstape

#include "game/art.h"

namespace bedsseven {
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
    gs::Bitmap b(72, 40);
    b.rect(2, 6, 68, 8, 2);
    b.rect(2, 6, 68, 3, 7);
    b.rect(2, 14, 6, 22, 3);
    b.rect(64, 14, 6, 22, 4);
    b.rect(4, 34, 64, 4, 3);
    b.rect(8, 16, 56, 18, 5);
    b.outline(1, false);
    return b;
}

gs::Bitmap soilArt() {
    gs::Bitmap b(52, 14);
    b.rect(0, 2, 52, 10, 2);
    b.rect(0, 2, 52, 3, 6);
    for (int i = 0; i < 8; i++) b.set(4 + i * 6, 8, 3);
    b.outline(1, false);
    return b;
}

gs::Bitmap sproutArt(int stage) {
    gs::Bitmap b(40, 36);
    int h = 8 + stage * 8;
    b.rect(18, 36 - h, 4, h, 3);
    b.ellipse(12, 36 - h, 7, 4, 2);
    b.ellipse(28, 36 - h + 2, 6, 3, 4);
    if (stage >= 1) b.ellipse(20, 36 - h - 2, 4, 3, 5);
    if (stage >= 2) {
        b.ellipse(14, 36 - h + 6, 3, 2, 6);
        b.ellipse(26, 36 - h + 8, 3, 2, 6);
    }
    b.outline(1, false);
    return b;
}

gs::Bitmap manArt(bool pour) {
    gs::Bitmap b(28, 36);
    b.ellipse(14, 8, 6, 6, 2);
    b.rect(10, 14, 8, 12, 4);
    b.rect(9, 16, 10, 6, 5);
    b.rect(10, 26, 3, 8, 6);
    b.rect(15, 26, 3, 8, 6);
    b.set(12, 8, 1);
    b.set(16, 8, 1);
    if (pour) b.rect(18, 16, 8, 3, 2);
    else b.rect(6, 16, 4, 8, 2);
    b.outline(1, false);
    return b;
}

gs::Bitmap rivalArt() {
    gs::Bitmap b(24, 32);
    b.ellipse(12, 7, 5, 5, 2);
    b.rect(8, 12, 8, 10, 3);
    b.rect(8, 14, 8, 5, 4);
    b.rect(8, 22, 3, 8, 5);
    b.rect(13, 22, 3, 8, 5);
    b.rect(16, 14, 6, 3, 2);
    b.set(10, 7, 1);
    b.set(14, 7, 1);
    b.outline(1, false);
    return b;
}

gs::Bitmap canArt() {
    gs::Bitmap b(16, 14);
    b.rect(2, 3, 10, 9, 2);
    b.rect(3, 4, 8, 3, 3);
    b.rect(11, 5, 4, 2, 4);
    b.rect(12, 1, 2, 5, 4);
    b.outline(1, false);
    return b;
}

gs::Bitmap dropArt() {
    gs::Bitmap b(6, 8);
    b.ellipse(3, 4, 2, 3, 2);
    b.set(3, 1, 3);
    return b;
}

gs::Bitmap pipArt(bool on) {
    gs::Bitmap b(8, 8);
    b.ellipse(4, 4, 3, 3, on ? 2 : 3);
    b.outline(1, false);
    return b;
}

void paintYard(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
    uint8_t g[64], p[64];
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) {
            int n = (x * 3 + y * 5) & 7;
            g[y * 8 + x] = n == 0 ? 3 : 2;
            p[y * 8 + x] = ((x + y) & 3) == 0 ? 3 : 2;
        }
    a.grass = tiles.shared(g);
    a.path = tiles.shared(p);
    vdp.B.resize(64, 32);
    vdp.B.clear();
    for (int y = 0; y < 28; y++)
        for (int x = 0; x < 40; x++) {
            int tile = y >= 14 ? a.path : a.grass;
            int pal = y >= 14 ? PAL_PATH : PAL_YARD;
            vdp.B.set(x, y, gs::entry(tile, pal));
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
    uint16_t wood[16] = {0, gs::rgb4(2, 1, 1), gs::rgb4(12, 8, 3), gs::rgb4(8, 5, 2), gs::rgb4(5, 3, 1), gs::rgb4(6, 4, 2),
                         gs::rgb4(14, 12, 8), gs::rgb4(15, 13, 8), 0, 0, 0, 0, 0, 0, 0, 0};
    uint16_t soil[16] = {0, gs::rgb4(2, 1, 1), gs::rgb4(10, 7, 3), gs::rgb4(6, 4, 2), gs::rgb4(4, 8, 3), gs::rgb4(3, 5, 2),
                         gs::rgb4(13, 10, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0};
    uint16_t leaf[16] = {0, gs::rgb4(1, 2, 1), gs::rgb4(6, 13, 3), gs::rgb4(3, 9, 2), gs::rgb4(8, 14, 5), gs::rgb4(14, 12, 3),
                         gs::rgb4(14, 4, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0};
    uint16_t man[16] = {0, gs::rgb4(2, 1, 1), gs::rgb4(14, 11, 7), gs::rgb4(9, 6, 4), gs::rgb4(3, 8, 13), gs::rgb4(2, 5, 9),
                        gs::rgb4(4, 3, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0};
    uint16_t rival[16] = {0, gs::rgb4(2, 1, 1), gs::rgb4(13, 9, 6), gs::rgb4(12, 4, 3), gs::rgb4(8, 2, 2), gs::rgb4(4, 3, 5),
                          0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    uint16_t water[16] = {0, gs::rgb4(1, 2, 4), gs::rgb4(8, 14, 15), gs::rgb4(3, 9, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    uint16_t yard[16] = {0, gs::rgb4(1, 3, 1), gs::rgb4(4, 10, 3), gs::rgb4(7, 13, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    uint16_t sky[16] = {};
    uint16_t pip[16] = {0, gs::rgb4(2, 2, 2), gs::rgb4(15, 13, 4), gs::rgb4(5, 5, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    uint16_t can[16] = {0, gs::rgb4(1, 2, 2), gs::rgb4(10, 12, 13), gs::rgb4(6, 8, 9), gs::rgb4(4, 5, 6), 0, 0, 0, 0, 0, 0, 0,
                        0, 0, 0, 0};
    uint16_t dim[16] = {0, gs::rgb4(10, 11, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 2, 3)};
    uint16_t path[16] = {0, gs::rgb4(3, 2, 2), gs::rgb4(9, 8, 6), gs::rgb4(12, 11, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    pal(PAL_HUD, hud);
    pal(PAL_GOLD, gold);
    pal(PAL_WARN, warn);
    pal(PAL_GOOD, good);
    pal(PAL_WOOD, wood);
    pal(PAL_SOIL, soil);
    pal(PAL_LEAF, leaf);
    pal(PAL_MAN, man);
    pal(PAL_RIVAL, rival);
    pal(PAL_WATER, water);
    pal(PAL_YARD, yard);
    pal(PAL_SKY, sky);
    pal(PAL_PIP, pip);
    pal(PAL_CAN, can);
    pal(PAL_DIM, dim);
    pal(PAL_PATH, path);

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    paintYard(vdp, tiles, art);

    art.title = say(vdp, "BEDS", 4, 1, 15);
    art.sub = say(vdp, "FIRST TO SEVEN", 2, 1, 15);
    art.win = say(vdp, "FIRST TO SEVEN", 2, 1, 15);
    art.lose = say(vdp, "STILL SHORT", 2, 1, 15);
    art.bed = gs::uploadMipped(vdp, bedArt());
    art.soil = gs::uploadMipped(vdp, soilArt());
    for (int i = 0; i < 3; i++) art.sprout[i] = gs::uploadMipped(vdp, sproutArt(i));
    art.man[0] = gs::uploadMipped(vdp, manArt(false));
    art.man[1] = gs::uploadMipped(vdp, manArt(true));
    art.rival = gs::uploadMipped(vdp, rivalArt());
    art.can = gs::uploadMipped(vdp, canArt());
    art.drop = gs::uploadMipped(vdp, dropArt());
    art.pipOn = gs::uploadMipped(vdp, pipArt(true));
    art.pipOff = gs::uploadMipped(vdp, pipArt(false));
    gs::Bitmap sh(20, 6);
    sh.ellipse(10, 3, 9, 2, 1);
    art.shade = gs::uploadMipped(vdp, sh);
    vdp.HUD.enabled = true;
    vdp.hudEnabled = true;
}

}  // namespace bedsseven

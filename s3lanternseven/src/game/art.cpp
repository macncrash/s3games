#include "game/art.h"

#include <initializer_list>

namespace lanternseven {
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

void paintYou(gs::Bitmap& b) {
    b.ellipse(16, 8, 6, 6, 1);
    b.rect(12, 14, 8, 12, 2);
    b.rect(10, 16, 4, 8, 3);
    b.rect(18, 16, 4, 10, 3);
    b.rect(12, 26, 4, 12, 4);
    b.rect(17, 26, 4, 12, 4);
    b.rect(11, 37, 5, 3, 5);
    b.rect(16, 37, 5, 3, 5);
    b.rect(20, 18, 8, 4, 6);
}

void paintLamp(gs::Bitmap& b) {
    b.rect(8, 0, 4, 4, 1);
    b.poly({{4, 6}, {16, 6}, {14, 18}, {6, 18}}, 2);
    b.rect(6, 8, 8, 6, 3);
    b.rect(5, 18, 10, 3, 1);
    b.rect(8, 21, 4, 3, 4);
}

void paintFlame(gs::Bitmap& b) {
    b.ellipse(5, 7, 3.2f, 4.5f, 1);
    b.ellipse(5, 8, 1.6f, 2.4f, 2);
    b.set(5, 3, 3);
}

void paintPost(gs::Bitmap& b) {
    b.rect(4, 0, 4, 56, 1);
    b.rect(2, 52, 8, 6, 2);
    b.rect(0, 4, 8, 3, 1);
}

void paintMoon(gs::Bitmap& b) {
    b.ellipse(12, 12, 10, 10, 1);
    b.ellipse(16, 10, 8, 8, 0);
    b.set(7, 10, 2);
    b.set(9, 14, 2);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_HUD, gs::rgb4(14, 14, 12), gs::rgb4(1, 1, 3));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 12, 3), gs::rgb4(3, 2, 0));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 5, 3), gs::rgb4(3, 1, 1));
    textPal(vdp, PAL_GREEN, gs::rgb4(6, 15, 8), gs::rgb4(0, 3, 1));
    textPal(vdp, PAL_DIM, gs::rgb4(8, 8, 10), gs::rgb4(1, 1, 2));

    setPal(vdp, PAL_YOU,
           {gs::rgb4(0, 0, 0), gs::rgb4(12, 8, 5), gs::rgb4(2, 3, 8), gs::rgb4(14, 11, 7), gs::rgb4(1, 2, 5),
            gs::rgb4(3, 2, 2), gs::rgb4(12, 9, 2)});
    setPal(vdp, PAL_THEM,
           {gs::rgb4(0, 0, 0), gs::rgb4(6, 6, 8), gs::rgb4(3, 3, 6), gs::rgb4(10, 10, 12), gs::rgb4(2, 2, 4),
            gs::rgb4(1, 1, 2), gs::rgb4(8, 4, 3)});
    setPal(vdp, PAL_LAMP,
           {gs::rgb4(0, 0, 0), gs::rgb4(10, 8, 3), gs::rgb4(14, 11, 4), gs::rgb4(15, 14, 8), gs::rgb4(6, 5, 2)});
    setPal(vdp, PAL_FLAME, {gs::rgb4(0, 0, 0), gs::rgb4(15, 8, 1), gs::rgb4(15, 14, 4), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_POST, {gs::rgb4(0, 0, 0), gs::rgb4(3, 3, 5), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_MOON, {gs::rgb4(0, 0, 0), gs::rgb4(13, 13, 11), gs::rgb4(8, 8, 9)});
    setPal(vdp, PAL_SOLID, {gs::rgb4(0, 0, 0), gs::rgb4(15, 12, 3), gs::rgb4(8, 3, 3), gs::rgb4(4, 4, 6),
                            gs::rgb4(15, 10, 2)});

    gs::Bitmap you(32, 42);
    paintYou(you);
    art.you = gs::uploadMipped(vdp, you);

    gs::Bitmap lamp(20, 26);
    paintLamp(lamp);
    art.lamp = gs::uploadMipped(vdp, lamp);

    gs::Bitmap flame(10, 12);
    paintFlame(flame);
    art.flame = gs::uploadMipped(vdp, flame);

    gs::Bitmap post(12, 60);
    paintPost(post);
    art.post = gs::uploadMipped(vdp, post);

    gs::Bitmap moon(24, 24);
    paintMoon(moon);
    art.moon = gs::uploadMipped(vdp, moon);

    gs::Bitmap solid(4, 4);
    solid.rect(0, 0, 4, 4, 1);
    art.solid = gs::uploadMipped(vdp, solid);

    loadFont(vdp, art);
    vdp.setFogColor(gs::rgb4(1, 1, 3));
}

}  // namespace lanternseven

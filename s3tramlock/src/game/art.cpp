#include "game/art.h"

namespace tramlock {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, ink);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
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

gs::Bitmap dashArt() {
    gs::Bitmap b(200, 56);
    b.poly({{0, 22}, {36, 8}, {164, 8}, {200, 22}, {200, 56}, {0, 56}}, 1);
    b.rect(0, 28, 200, 28, 2);
    b.rect(14, 32, 54, 12, 3);
    b.rect(132, 32, 54, 12, 3);
    b.rect(84, 14, 32, 16, 4);
    b.rect(90, 18, 20, 8, 5);
    b.rect(0, 48, 200, 8, 6);
    for (int i = 0; i < 8; i++) b.rect(8.f + i * 24.f, 50, 10, 3, i % 2 ? 7 : 8);
    return b;
}

gs::Bitmap leverArt(float pull) {
    gs::Bitmap b(28, 44);
    b.rect(11, 26, 6, 14, 1);
    b.ellipse(14, 38, 8, 4, 2);
    float tip = 8.f + pull * 12.f;
    b.line(14, 28, 14, tip, 1, 2.6f);
    b.ellipse(14, tip, 4, 3, 3);
    return b;
}

gs::Bitmap leafArt() {
    gs::Bitmap b(28, 72);
    b.rect(2, 2, 24, 68, 1);
    b.rect(0, 0, 28, 6, 2);
    b.rect(0, 66, 28, 6, 2);
    for (int i = 0; i < 5; i++) b.rect(4, 12.f + i * 11.f, 20, 3, 3);
    b.rect(11, 8, 6, 56, 4);
    b.ellipse(14, 36, 4, 4, 5);
    return b;
}

gs::Bitmap wallArt() {
    gs::Bitmap b(36, 48);
    b.rect(0, 8, 36, 40, 1);
    b.poly({{0, 10}, {18, 0}, {36, 10}}, 2);
    for (int row = 0; row < 4; row++)
        for (int col = 0; col < 3; col++) b.rect(3.f + col * 11.f, 14.f + row * 8.f, 8, 5, (row + col) & 1 ? 3 : 4);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(14, 36);
    b.rect(6, 10, 2, 26, 1);
    b.rect(3, 6, 8, 6, 2);
    b.ellipse(7, 5, 3, 3, 3);
    return b;
}

gs::Bitmap wireArt() {
    gs::Bitmap b(40, 8);
    b.line(0, 4, 40, 4, 1, 1.4f);
    b.line(0, 6, 40, 2, 2, 1.f);
    return b;
}

gs::Bitmap capstanArt() {
    gs::Bitmap b(22, 22);
    b.ellipse(11, 12, 9, 6, 1);
    b.ellipse(11, 8, 6, 4, 2);
    b.rect(9, 4, 4, 6, 3);
    return b;
}

gs::Bitmap boardArt() {
    gs::TextStyle st{2, 1, 0, 0, 1};
    gs::Bitmap word = gs::textBitmap("LOCK", st);
    gs::Bitmap b(word.w + 10, word.h + 8);
    b.rect(0, 0, float(b.w), float(b.h), 2);
    b.rect(2, 2, float(b.w) - 4, float(b.h) - 4, 3);
    b.blit(word, 5, 4);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_HUD, gs::rgb4(15, 15, 13));
    textPal(vdp, PAL_AMBER, gs::rgb4(15, 12, 3));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 4, 3));
    textPal(vdp, PAL_GOOD, gs::rgb4(5, 15, 7));
    loadFont(vdp, art);

    setPal(vdp, PAL_CAB,
           {0, gs::rgb4(2, 8, 5), gs::rgb4(8, 9, 7), gs::rgb4(1, 4, 3), gs::rgb4(3, 3, 4), gs::rgb4(14, 14, 10),
            gs::rgb4(1, 3, 2), gs::rgb4(15, 13, 4), gs::rgb4(6, 7, 5), gs::rgb4(12, 11, 6)});
    setPal(vdp, PAL_STONE,
           {0, gs::rgb4(6, 6, 6), gs::rgb4(8, 8, 7), gs::rgb4(5, 5, 5), gs::rgb4(9, 8, 7)});
    setPal(vdp, PAL_GATE,
           {0, gs::rgb4(6, 4, 2), gs::rgb4(4, 3, 2), gs::rgb4(9, 7, 3), gs::rgb4(3, 3, 4), gs::rgb4(12, 10, 4)});
    setPal(vdp, PAL_IRON, {0, gs::rgb4(3, 3, 4), gs::rgb4(8, 8, 7), gs::rgb4(14, 12, 3)});
    setPal(vdp, PAL_SIGN, {0, gs::rgb4(1, 1, 1), gs::rgb4(2, 4, 8), gs::rgb4(15, 14, 6)});

    auto roadPal = [&](int pal, uint16_t ground, uint16_t verge, uint16_t asphalt, uint16_t paint) {
        for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, asphalt);
        vdp.setColor(pal * 16 + 0, 0);
        vdp.setColor(pal * 16 + 1, ground);
        vdp.setColor(pal * 16 + 2, gs::rgb4(3, 5, 6));
        vdp.setColor(pal * 16 + 3, gs::rgb4(4, 6, 7));
        vdp.setColor(pal * 16 + 4, verge);
        vdp.setColor(pal * 16 + 5, gs::rgb4(5, 5, 5));
        vdp.setColor(pal * 16 + 6, asphalt);
        vdp.setColor(pal * 16 + 7, gs::rgb4(3, 3, 4));
        vdp.setColor(pal * 16 + 8, gs::rgb4(4, 4, 5));
        vdp.setColor(pal * 16 + 9, gs::rgb4(2, 3, 4));
        vdp.setColor(pal * 16 + 10, gs::rgb4(7, 6, 5));
        vdp.setColor(pal * 16 + 14, paint);
        vdp.setColor(pal * 16 + 15, gs::rgb4(10, 10, 8));
    };
    roadPal(PAL_ROAD, gs::rgb4(3, 5, 4), gs::rgb4(5, 5, 4), gs::rgb4(5, 5, 6), gs::rgb4(12, 12, 10));
    roadPal(PAL_CHAMBER, gs::rgb4(3, 4, 5), gs::rgb4(6, 6, 6), gs::rgb4(4, 5, 6), gs::rgb4(11, 11, 8));

    art.dash = gs::uploadMipped(vdp, dashArt());
    for (int i = 0; i < 4; i++) art.lever[i] = gs::uploadMipped(vdp, leverArt(i / 3.f));
    art.leaf = gs::uploadMipped(vdp, leafArt());
    art.wall = gs::uploadMipped(vdp, wallArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.wire = gs::uploadMipped(vdp, wireArt());
    art.capstan = gs::uploadMipped(vdp, capstanArt());
    art.board = gs::uploadMipped(vdp, boardArt());
    vdp.setFogColor(gs::rgb4(6, 8, 10));
}

}  // namespace tramlock

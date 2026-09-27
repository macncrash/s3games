#include "game/art.h"

#include <cstdint>
#include <initializer_list>

#include "console/gfx.h"

namespace skatechime {
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
    vdp.setColor(pal * 16 + 15, edge);
}

gs::Bitmap riderArt() {
    gs::Bitmap b(22, 30);
    b.ellipse(11.f, 5.f, 4.5f, 4.5f, 1);
    b.rect(8, 3, 6, 3, 2);
    b.rect(7, 10, 8, 9, 3);
    b.rect(3, 11, 5, 7, 4);
    b.rect(14, 11, 5, 7, 4);
    b.rect(8, 18, 3, 8, 5);
    b.rect(12, 18, 3, 8, 5);
    b.rect(7, 25, 5, 3, 6);
    b.rect(12, 25, 5, 3, 6);
    return b;
}

gs::Bitmap deckArt() {
    gs::Bitmap b(32, 8);
    b.rect(3, 2, 26, 3, 1);
    b.ellipse(4.f, 3.5f, 2.4f, 2.2f, 2);
    b.ellipse(28.f, 3.5f, 2.4f, 2.2f, 2);
    b.ellipse(10.f, 5.2f, 2.2f, 2.2f, 3);
    b.ellipse(22.f, 5.2f, 2.2f, 2.2f, 3);
    b.set(10, 5, 4);
    b.set(22, 5, 4);
    return b;
}

gs::Bitmap brickArt() {
    gs::Bitmap b(32, 14);
    b.rect(0, 0, 32, 14, 1);
    b.rect(0, 0, 32, 2, 2);
    b.line(0, 7, 32, 7, 3, 1.f);
    b.line(10, 0, 10, 7, 3, 1.f);
    b.line(22, 7, 22, 14, 3, 1.f);
    return b;
}

gs::Bitmap goldArt() {
    gs::Bitmap b(32, 14);
    b.rect(0, 0, 32, 14, 1);
    b.rect(0, 0, 32, 3, 2);
    b.rect(4, 5, 24, 4, 3);
    return b;
}

gs::Bitmap towerArt() {
    gs::Bitmap b(36, 96);
    b.rect(6, 18, 24, 78, 1);
    b.poly({{18, 1}, {34, 20}, {2, 20}}, 2);
    b.rect(14, 0, 8, 8, 3);
    b.rect(10, 28, 16, 18, 4);
    for (int y = 52; y < 90; y += 12) b.rect(12, y, 5, 8, 5);
    for (int y = 58; y < 90; y += 12) b.rect(20, y, 5, 8, 5);
    return b;
}

gs::Bitmap faceArt() {
    gs::Bitmap b(14, 14);
    b.ellipse(7.f, 7.f, 6.2f, 6.2f, 1);
    b.ellipse(7.f, 7.f, 4.6f, 4.6f, 2);
    b.rect(6, 2, 2, 5, 3);
    b.rect(6, 6, 5, 2, 3);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(14, 16);
    b.rect(6, 0, 2, 3, 3);
    b.ellipse(7.f, 9.f, 6.f, 5.5f, 1);
    b.ellipse(7.f, 8.f, 3.2f, 3.f, 2);
    b.rect(2, 13, 10, 2, 4);
    return b;
}

gs::Bitmap coneArt() {
    gs::Bitmap b(12, 16);
    b.poly({{6, 1}, {11, 14}, {1, 14}}, 1);
    b.rect(2, 13, 8, 2, 2);
    b.rect(4, 6, 4, 2, 3);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (!g[y * 5 + x]) continue;
                px[y * 8 + x + 1] = 1;
                if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
            }
        }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_RIDER,
           {0, gs::rgb4(13, 10, 7), gs::rgb4(2, 2, 3), gs::rgb4(3, 6, 12), gs::rgb4(14, 12, 8), gs::rgb4(2, 3, 6),
            gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_DECK, {0, gs::rgb4(11, 4, 3), gs::rgb4(7, 3, 2), gs::rgb4(8, 8, 9), gs::rgb4(14, 14, 12)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(13, 10, 2), gs::rgb4(15, 14, 6), gs::rgb4(15, 15, 10)});
    setPal(vdp, PAL_STREET, {0, gs::rgb4(5, 5, 6), gs::rgb4(8, 8, 9), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_TOWER,
           {0, gs::rgb4(6, 6, 8), gs::rgb4(4, 4, 6), gs::rgb4(12, 10, 4), gs::rgb4(9, 11, 14), gs::rgb4(3, 4, 6),
            gs::rgb4(8, 9, 11)});
    textPal(vdp, PAL_INK, gs::rgb4(14, 14, 13), gs::rgb4(1, 1, 2));
    textPal(vdp, PAL_HOUR, gs::rgb4(15, 13, 4), gs::rgb4(4, 2, 0));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 4, 3), gs::rgb4(3, 0, 0));
    textPal(vdp, PAL_WORD, gs::rgb4(15, 14, 8), gs::rgb4(3, 2, 1));

    art.rider = gs::uploadImage(vdp, riderArt());
    art.deck = gs::uploadImage(vdp, deckArt());
    art.brick = gs::uploadImage(vdp, brickArt());
    art.gold = gs::uploadImage(vdp, goldArt());
    art.tower = gs::uploadImage(vdp, towerArt());
    art.face = gs::uploadImage(vdp, faceArt());
    art.bell = gs::uploadImage(vdp, bellArt());
    art.cone = gs::uploadImage(vdp, coneArt());

    gs::TextStyle st;
    st.scale = 3;
    st.color = 1;
    st.outline = 15;
    st.spacing = 1;
    art.word = gs::uploadImage(vdp, gs::textBitmap("SKATE", st));

    loadFont(vdp, art);
    vdp.setFogColor(gs::rgb4(5, 7, 11));
}

}  // namespace skatechime

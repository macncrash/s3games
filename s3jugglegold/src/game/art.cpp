#include "game/art.h"

#include <initializer_list>

namespace jugglegold {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
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

gs::Bitmap jugglerArt() {
    gs::Bitmap b(48, 86);
    b.ellipse(24.f, 12.f, 8.f, 8.f, 1);
    b.ellipse(21.f, 11.f, 1.3f, 1.4f, 2);
    b.ellipse(27.f, 11.f, 1.3f, 1.4f, 2);
    b.ellipse(24.f, 15.f, 1.4f, 1.f, 2);
    b.rect(18.f, 19.f, 12.f, 5.f, 1);
    b.rect(14.f, 24.f, 20.f, 26.f, 3);
    b.rect(14.f, 24.f, 20.f, 5.f, 4);
    b.rect(16.f, 32.f, 6.f, 8.f, 4);
    b.rect(26.f, 32.f, 6.f, 8.f, 4);
    b.rect(16.f, 50.f, 6.f, 26.f, 5);
    b.rect(26.f, 50.f, 6.f, 26.f, 5);
    b.rect(13.f, 74.f, 11.f, 5.f, 6);
    b.rect(24.f, 74.f, 11.f, 5.f, 6);
    b.line(14.f, 28.f, 4.f, 46.f, 1, 3.2f);
    b.line(34.f, 28.f, 44.f, 46.f, 1, 3.2f);
    return b;
}

gs::Bitmap gloveArt() {
    gs::Bitmap b(20, 16);
    b.ellipse(10.f, 9.f, 8.f, 5.5f, 1);
    b.rect(3.f, 2.f, 3.f, 7.f, 1);
    b.rect(8.f, 1.f, 3.f, 7.f, 1);
    b.rect(13.f, 2.f, 3.f, 7.f, 1);
    b.rect(3.f, 8.f, 14.f, 2.f, 2);
    return b;
}

gs::Bitmap creamArt() {
    gs::Bitmap b(18, 18);
    b.ellipse(9.f, 9.f, 8.f, 8.f, 3);
    b.ellipse(9.f, 9.f, 6.2f, 6.2f, 1);
    b.ellipse(7.f, 6.5f, 2.2f, 1.6f, 2);
    b.ellipse(11.f, 11.f, 2.f, 1.2f, 4);
    return b;
}

gs::Bitmap goldArt() {
    gs::Bitmap b(18, 18);
    b.ellipse(9.f, 9.f, 8.f, 8.f, 3);
    b.ellipse(9.f, 9.f, 6.4f, 6.4f, 1);
    b.ellipse(7.f, 6.5f, 2.1f, 1.5f, 2);
    b.line(9.f, 5.f, 9.f, 13.f, 4, 1.4f);
    b.line(5.f, 9.f, 13.f, 9.f, 4, 1.4f);
    return b;
}

gs::Bitmap floorArt() {
    gs::Bitmap b(220, 28);
    b.rect(0.f, 0.f, 220.f, 8.f, 1);
    b.rect(0.f, 8.f, 220.f, 20.f, 2);
    for (int i = 0; i < 11; i++) b.rect(float(i * 20), 8.f, 2.f, 20.f, 3);
    return b;
}

gs::Bitmap drapeArt() {
    gs::Bitmap b(36, 120);
    for (int y = 0; y < 120; y++) {
        for (int x = 0; x < 36; x++) {
            int fold = (x / 6) & 1;
            int c = (x % 6 == 0) ? 3 : fold ? 2 : 1;
            if (y > 108) c = 4;
            b.set(x, y, c);
        }
    }
    b.ellipse(30.f, 70.f, 6.f, 10.f, 4);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(22, 36);
    b.rect(10.f, 0.f, 2.f, 10.f, 3);
    b.poly({{2.f, 12.f}, {20.f, 12.f}, {16.f, 28.f}, {6.f, 28.f}}, 1);
    b.ellipse(11.f, 16.f, 3.f, 2.f, 2);
    b.rect(9.f, 28.f, 4.f, 8.f, 3);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_BODY, {0, gs::rgb4(14, 11, 8), gs::rgb4(4, 2, 2), gs::rgb4(9, 2, 4), gs::rgb4(13, 10, 4),
                           gs::rgb4(3, 3, 6), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_GLOVE, {0, gs::rgb4(12, 3, 3), gs::rgb4(6, 1, 1)});
    setPal(vdp, PAL_CREAM, {0, gs::rgb4(15, 14, 11), gs::rgb4(15, 15, 14), gs::rgb4(8, 7, 6), gs::rgb4(12, 10, 8)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 2), gs::rgb4(15, 15, 8), gs::rgb4(8, 5, 0), gs::rgb4(15, 8, 1)});
    setPal(vdp, PAL_STAGE, {0, gs::rgb4(6, 4, 2), gs::rgb4(4, 3, 2), gs::rgb4(8, 6, 3)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(14, 12, 6), gs::rgb4(15, 15, 10), gs::rgb4(5, 4, 3), gs::rgb4(10, 2, 3)});
    textPal(vdp, PAL_INK, gs::rgb4(14, 13, 12));
    textPal(vdp, PAL_MARK, gs::rgb4(15, 12, 3));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 5, 4));
    textPal(vdp, PAL_TITLE, gs::rgb4(15, 14, 6));
    textPal(vdp, PAL_WIN, gs::rgb4(8, 15, 8));
    loadFont(vdp, art);
    art.juggler = gs::uploadImage(vdp, jugglerArt());
    art.glove = gs::uploadImage(vdp, gloveArt());
    art.cream = gs::uploadImage(vdp, creamArt());
    art.gold = gs::uploadImage(vdp, goldArt());
    art.floor = gs::uploadImage(vdp, floorArt());
    art.drape = gs::uploadImage(vdp, drapeArt());
    art.lamp = gs::uploadImage(vdp, lampArt());
}

}  // namespace jugglegold

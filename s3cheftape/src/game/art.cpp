#include "art.h"

#include <initializer_list>

namespace cheftape {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
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
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 3;
                }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

void paintPlate(gs::Bitmap& b, int dish) {
    b.ellipse(16, 22, 14.f, 5.f, 2);
    b.ellipse(16, 20, 11.f, 3.4f, 1);
    if (dish == 0) {
        b.ellipse(16, 16, 7.f, 5.f, 4);
        b.ellipse(16, 15, 4.f, 2.2f, 5);
    } else if (dish == 1) {
        b.ellipse(16, 15, 8.f, 4.5f, 4);
        b.rect(11, 13, 3, 2, 6);
        b.rect(16, 14, 4, 2, 3);
    } else if (dish == 2) {
        b.rect(9, 12, 14, 6, 4);
        b.rect(10, 10, 12, 3, 5);
        b.rect(15, 8, 2, 3, 6);
    } else {
        b.ellipse(16, 16, 7.f, 4.f, 4);
        b.ellipse(13, 15, 2.2f, 1.4f, 5);
        b.ellipse(18, 16, 1.8f, 1.2f, 5);
    }
}

void paintChef(gs::Bitmap& b) {
    b.rect(8, 4, 16, 4, 5);
    b.rect(6, 7, 20, 3, 5);
    b.ellipse(16, 16, 7.f, 6.f, 2);
    b.rect(13, 14, 2, 2, 3);
    b.rect(18, 14, 2, 2, 3);
    b.rect(15, 18, 3, 1, 4);
    b.poly({{6, 22}, {26, 22}, {24, 42}, {8, 42}}, 6);
    b.rect(10, 24, 12, 8, 1);
    b.rect(8, 42, 5, 8, 7);
    b.rect(19, 42, 5, 8, 7);
    b.rect(7, 49, 7, 2, 3);
    b.rect(18, 49, 7, 2, 3);
}

void paintReel(gs::Bitmap& b) {
    b.rect(2, 6, 28, 20, 3);
    b.rect(4, 8, 10, 16, 1);
    b.rect(18, 8, 10, 16, 1);
    b.ellipse(9, 16, 3.f, 3.f, 4);
    b.ellipse(23, 16, 3.f, 3.f, 4);
    b.rect(14, 14, 4, 4, 2);
}

void paintTicket(gs::Bitmap& b) {
    b.rect(0, 0, 28, 16, 1);
    b.rect(2, 3, 16, 2, 2);
    b.rect(2, 7, 10, 2, 3);
    b.rect(2, 11, 12, 2, 4);
}

void paintPan(gs::Bitmap& b) {
    b.ellipse(14, 10, 12.f, 6.f, 2);
    b.ellipse(14, 9, 9.f, 4.f, 1);
    b.rect(24, 8, 8, 3, 3);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(15, 14, 12), gs::rgb4(12, 8, 6), gs::rgb4(2, 1, 1), gs::rgb4(10, 3, 3),
                             gs::rgb4(15, 15, 15), gs::rgb4(14, 13, 12), gs::rgb4(4, 4, 6), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_KITCHEN, {0, gs::rgb4(6, 4, 3), gs::rgb4(3, 2, 2)});
    setPal(vdp, PAL_SOUP, {0, gs::rgb4(15, 14, 12), gs::rgb4(9, 8, 7), gs::rgb4(4, 3, 3), gs::rgb4(14, 10, 3),
                           gs::rgb4(15, 13, 6), gs::rgb4(10, 6, 2), gs::rgb4(8, 2, 1)});
    setPal(vdp, PAL_STEAK, {0, gs::rgb4(15, 14, 12), gs::rgb4(8, 7, 6), gs::rgb4(4, 3, 3), gs::rgb4(10, 4, 2),
                            gs::rgb4(12, 6, 3), gs::rgb4(6, 2, 1), gs::rgb4(14, 12, 6)});
    setPal(vdp, PAL_CAKE, {0, gs::rgb4(15, 14, 12), gs::rgb4(10, 8, 7), gs::rgb4(5, 3, 3), gs::rgb4(15, 12, 14),
                           gs::rgb4(14, 8, 10), gs::rgb4(12, 4, 6), gs::rgb4(15, 15, 15)});
    setPal(vdp, PAL_GRAVY, {0, gs::rgb4(12, 11, 9), gs::rgb4(7, 6, 5), gs::rgb4(4, 3, 2), gs::rgb4(8, 5, 2),
                            gs::rgb4(6, 4, 2), gs::rgb4(3, 2, 1), gs::rgb4(10, 8, 4)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 4), gs::rgb4(8, 5, 1)});
    setPal(vdp, PAL_PAPER, {0, gs::rgb4(14, 12, 8), gs::rgb4(9, 6, 3), gs::rgb4(5, 3, 2), gs::rgb4(12, 4, 2)});
    setPal(vdp, PAL_STEEL, {0, gs::rgb4(10, 11, 12), gs::rgb4(5, 6, 7), gs::rgb4(3, 3, 4)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(6, 1, 1)});
    setPal(vdp, PAL_FIRE, {0, gs::rgb4(15, 8, 1), gs::rgb4(12, 3, 1), gs::rgb4(15, 13, 3)});
    vdp.setFogColor(gs::rgb4(3, 2, 1));

    loadFont(vdp, art);
    for (int i = 0; i < kDishes; i++) {
        gs::Bitmap plate(32, 28);
        paintPlate(plate, i);
        art.plate[i] = gs::uploadMipped(vdp, plate);
    }
    gs::Bitmap chef(32, 52);
    paintChef(chef);
    art.chef = gs::uploadMipped(vdp, chef);
    gs::Bitmap reel(32, 32);
    paintReel(reel);
    art.reel = gs::uploadMipped(vdp, reel);
    gs::Bitmap ticket(28, 16);
    paintTicket(ticket);
    art.ticket = gs::uploadMipped(vdp, ticket);
    gs::Bitmap pan(32, 16);
    paintPan(pan);
    art.pan = gs::uploadMipped(vdp, pan);
    gs::Bitmap solid(4, 4);
    solid.rect(0, 0, 4, 4, 1);
    art.solid = gs::uploadMipped(vdp, solid);
    art.word = gs::uploadMipped(vdp, gs::textBitmap("CHEF", {3, 1, 2, 0, 1}));
}

}  // namespace cheftape

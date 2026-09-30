#include "game/art.h"

namespace presstape {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void loadFont(gs::VDP& vdp, int* font) {
    gs::TileAlloc tiles(vdp, 1);
    for (int c = 0; c < 96; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c + 32));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) px[y * 8 + (x + 1)] = 1;
        font[c] = tiles.shared(px);
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 14, 13)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 4)});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(6, 6, 7)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(14, 3, 2)});
    setPal(vdp, PAL_STEEL, {0, gs::rgb4(3, 3, 4), gs::rgb4(7, 8, 9), gs::rgb4(11, 12, 13), gs::rgb4(14, 15, 15)});
    setPal(vdp, PAL_OIL, {0, gs::rgb4(1, 2, 2), gs::rgb4(3, 5, 4), gs::rgb4(8, 9, 4)});
    setPal(vdp, PAL_RIB, {0, gs::rgb4(5, 3, 2), gs::rgb4(11, 6, 3), gs::rgb4(15, 10, 5)});
    setPal(vdp, PAL_WEB, {0, gs::rgb4(2, 4, 6), gs::rgb4(5, 9, 12), gs::rgb4(10, 14, 15)});
    setPal(vdp, PAL_CAP, {0, gs::rgb4(6, 2, 4), gs::rgb4(12, 4, 7), gs::rgb4(15, 9, 11)});
    setPal(vdp, PAL_SCRAP, {0, gs::rgb4(3, 3, 2), gs::rgb4(7, 6, 4), gs::rgb4(10, 9, 6)});
    setPal(vdp, PAL_DOOR, {0, gs::rgb4(3, 2, 1), gs::rgb4(8, 5, 2), gs::rgb4(13, 9, 4), gs::rgb4(4, 8, 3)});
    vdp.setFogColor(gs::rgb4(2, 2, 3));
    loadFont(vdp, art.font);

    {
        gs::Bitmap b(96, 150);
        b.rect(4, 8, 12, 130, 2);
        b.rect(80, 8, 12, 130, 2);
        b.rect(6, 10, 4, 124, 3);
        b.rect(82, 10, 4, 124, 3);
        b.rect(4, 4, 88, 14, 2);
        b.rect(10, 6, 76, 6, 3);
        b.rect(40, 18, 16, 18, 1);
        b.rect(44, 20, 8, 12, 3);
        b.rect(8, 128, 80, 14, 2);
        b.rect(16, 132, 64, 6, 1);
        art.frame = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(64, 22);
        b.rect(2, 2, 60, 8, 2);
        b.rect(4, 3, 56, 3, 3);
        b.rect(26, 10, 12, 8, 1);
        b.rect(18, 16, 28, 4, 2);
        b.line(8, 6, 56, 6, 3, 1);
        art.ram = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(78, 16);
        b.rect(2, 2, 74, 12, 2);
        b.rect(6, 4, 66, 4, 3);
        b.rect(10, 9, 10, 3, 1);
        b.rect(58, 9, 10, 3, 1);
        art.bed = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(40, 12);
        b.rect(2, 3, 36, 6, 2);
        b.rect(4, 4, 32, 2, 3);
        b.rect(8, 7, 6, 2, 1);
        b.rect(26, 7, 6, 2, 1);
        art.rib = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(28, 20);
        b.rect(2, 2, 24, 16, 2);
        b.rect(4, 4, 20, 12, 3);
        b.ellipse(9, 8, 2, 2, 1);
        b.ellipse(19, 8, 2, 2, 1);
        b.ellipse(14, 13, 2, 2, 1);
        art.web = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(22, 22);
        b.ellipse(11, 8, 8, 5, 2);
        b.rect(4, 8, 14, 10, 2);
        b.ellipse(11, 18, 8, 3, 1);
        b.ellipse(11, 7, 4, 2, 3);
        art.cap = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(26, 18);
        b.poly({{2, 10}, {8, 2}, {16, 6}, {22, 2}, {24, 12}, {14, 16}, {4, 15}}, 2);
        b.line(6, 8, 18, 12, 3, 1.2f);
        b.rect(10, 6, 4, 3, 1);
        art.flash = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(18, 10);
        b.ellipse(9, 5, 8, 4, 2);
        b.ellipse(9, 5, 3, 2, 1);
        art.shim = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(46, 24);
        b.rect(2, 4, 42, 16, 2);
        b.rect(4, 6, 38, 10, 3);
        b.rect(6, 2, 8, 4, 1);
        b.rect(32, 2, 8, 4, 1);
        b.line(8, 12, 38, 12, 1, 1);
        art.drawer = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(36, 64);
        b.rect(2, 2, 32, 58, 2);
        b.rect(6, 6, 24, 22, 3);
        b.rect(6, 32, 24, 20, 1);
        b.rect(26, 30, 4, 4, 3);
        art.door = gs::uploadImage(vdp, b);
    }
}

}  // namespace presstape

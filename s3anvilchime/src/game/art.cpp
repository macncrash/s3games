#include "game/art.h"

namespace anvilchime {
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 14, 12)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 3)});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(6, 6, 8)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(14, 3, 3)});
    setPal(vdp, PAL_IRON, {0, gs::rgb4(2, 2, 3), gs::rgb4(6, 6, 7), gs::rgb4(11, 11, 12), gs::rgb4(14, 14, 15),
                           gs::rgb4(5, 3, 2)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(4, 2, 1), gs::rgb4(8, 4, 2), gs::rgb4(12, 7, 3), gs::rgb4(6, 5, 2)});
    setPal(vdp, PAL_BELL, {0, gs::rgb4(10, 7, 1), gs::rgb4(15, 12, 3), gs::rgb4(15, 15, 8), gs::rgb4(6, 4, 1)});
    setPal(vdp, PAL_NIGHT, {0, gs::rgb4(9, 11, 13), gs::rgb4(3, 6, 4), gs::rgb4(12, 4, 2)});
    setPal(vdp, PAL_FACE, {0, gs::rgb4(15, 13, 4), gs::rgb4(12, 8, 2), gs::rgb4(15, 15, 10)});

    {
        // Long horn, short top. The gold span is drawn as its own sprite.
        gs::Bitmap b(88, 32);
        b.poly({{4, 14}, {8, 8}, {28, 8}, {34, 14}, {30, 18}, {10, 18}}, 3);
        b.rect(30, 12, 18, 8, 2);
        b.rect(28, 18, 48, 7, 1);
        b.rect(34, 24, 36, 6, 5);
        b.poly({{70, 12}, {84, 16}, {78, 20}, {62, 18}}, 4);
        b.line(32, 15, 46, 15, 3, 1.f);
        art.anvil = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(14, 8);
        b.rect(0, 1, 14, 6, 1);
        b.rect(1, 2, 12, 2, 3);
        b.rect(2, 4, 10, 2, 2);
        art.face = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(22, 40);
        b.rect(9, 1, 4, 22, 2);
        b.rect(10, 1, 2, 18, 3);
        b.poly({{1, 22}, {21, 22}, {19, 32}, {3, 32}}, 1);
        b.rect(3, 26, 16, 4, 4);
        art.hammer = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(22, 120);
        b.rect(7, 0, 8, 120, 1);
        b.rect(9, 0, 3, 120, 2);
        b.rect(3, 0, 16, 5, 3);
        b.rect(1, 112, 20, 8, 3);
        for (int y = 14; y < 108; y += 16) b.rect(8, y, 6, 2, 4);
        art.tower = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(24, 22);
        b.poly({{12, 1}, {3, 7}, {21, 7}}, 3);
        b.poly({{4, 7}, {2, 16}, {22, 16}, {20, 7}}, 2);
        b.rect(7, 9, 10, 3, 3);
        b.ellipse(12, 18, 3, 2, 1);
        art.bell = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(36, 36);
        b.ellipse(18, 18, 16, 16, 1);
        b.ellipse(18, 18, 13, 13, 2);
        b.ellipse(18, 18, 2, 2, 3);
        art.clock = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(3, 3);
        b.rect(0, 0, 3, 3, 1);
        art.pip = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(10, 10);
        b.ellipse(5, 5, 4, 4, 1);
        b.ellipse(5, 5, 2, 2, 2);
        art.lamp = gs::uploadImage(vdp, b);
    }
    loadFont(vdp, art.font);
}

}  // namespace anvilchime

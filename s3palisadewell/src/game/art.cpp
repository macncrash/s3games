#include "game/art.h"

#include <cstring>
#include <initializer_list>

namespace palisade {
namespace {

void pal(gs::VDP& v, int p, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) v.setColor(p * 16 + i++, c);
}

void glyphTile(gs::VDP& v, gs::TileAlloc& tiles, int& slot, char ch) {
    const uint8_t* g = gs::glyph(ch);
    uint8_t px[64];
    std::memset(px, 0, sizeof px);
    for (int y = 0; y < 7; y++)
        for (int x = 0; x < 5; x++)
            if (g[y * 5 + x]) px[y * 8 + x + 1] = 1;
    slot = tiles.shared(px);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    using gs::rgb4;
    vdp.setFogColor(rgb4(2, 3, 6));
    pal(vdp, PAL_HUD, {0, rgb4(14, 13, 9), rgb4(8, 6, 3), rgb4(12, 3, 2), rgb4(4, 8, 3)});
    pal(vdp, PAL_WOOD, {0, rgb4(8, 5, 2), rgb4(11, 7, 3), rgb4(5, 3, 1), rgb4(13, 10, 5), rgb4(3, 2, 1)});
    pal(vdp, PAL_STONE, {0, rgb4(8, 8, 8), rgb4(12, 12, 11), rgb4(5, 5, 6), rgb4(4, 7, 12), rgb4(6, 10, 14),
                         rgb4(9, 6, 3), rgb4(3, 3, 4)});
    pal(vdp, PAL_PLAYER, {0, rgb4(12, 8, 5), rgb4(3, 7, 4), rgb4(2, 4, 2), rgb4(4, 3, 2), rgb4(10, 8, 3),
                          rgb4(6, 4, 2), rgb4(14, 12, 8)});
    pal(vdp, PAL_RAIDER, {0, rgb4(11, 7, 4), rgb4(10, 2, 2), rgb4(6, 1, 1), rgb4(3, 2, 2), rgb4(8, 8, 7),
                          rgb4(12, 9, 4), rgb4(2, 1, 1)});
    pal(vdp, PAL_BRUTE, {0, rgb4(6, 6, 7), rgb4(3, 3, 4), rgb4(9, 8, 6), rgb4(5, 3, 2), rgb4(11, 4, 2),
                         rgb4(2, 2, 2), rgb4(8, 7, 5)});
    pal(vdp, PAL_FX, {0, rgb4(14, 12, 6), rgb4(15, 8, 2), rgb4(14, 14, 12), rgb4(8, 10, 4)});
    pal(vdp, PAL_GRASS, {0, rgb4(2, 5, 2), rgb4(3, 7, 2), rgb4(4, 8, 3), rgb4(6, 5, 2), rgb4(3, 4, 2),
                         rgb4(8, 7, 3)});

    auto upload = [&](gs::Bitmap& b) { return gs::uploadImage(vdp, b); };

    {
        gs::Bitmap b(24, 40);
        b.ellipse(12, 7, 5, 5, 1);
        b.rect(10, 4, 4, 3, 7);
        b.rect(9, 11, 6, 4, 2);
        b.rect(7, 14, 10, 14, 2);
        b.rect(8, 16, 3, 10, 3);
        b.rect(14, 22, 4, 8, 6);
        b.rect(9, 28, 3, 8, 4);
        b.rect(14, 28, 3, 8, 4);
        b.line(18, 16, 22, 12, 5, 1);
        b.line(18, 18, 22, 22, 5, 1);
        b.line(22, 12, 22, 22, 5, 1);
        art.player = upload(b);
    }
    {
        gs::Bitmap b(20, 32);
        b.ellipse(10, 6, 4, 4, 1);
        b.rect(7, 10, 7, 10, 2);
        b.rect(6, 12, 3, 7, 3);
        b.rect(14, 14, 3, 8, 6);
        b.rect(7, 20, 3, 9, 4);
        b.rect(11, 20, 3, 9, 4);
        b.line(4, 4, 4, 28, 5, 1);
        b.line(3, 4, 8, 6, 5, 1);
        art.raider = upload(b);
    }
    {
        gs::Bitmap b(28, 36);
        b.rect(8, 4, 12, 8, 1);
        b.rect(6, 12, 16, 12, 2);
        b.rect(8, 14, 12, 6, 3);
        b.rect(7, 24, 5, 9, 4);
        b.rect(16, 24, 5, 9, 4);
        b.rect(2, 16, 8, 6, 5);
        b.rect(20, 15, 6, 5, 6);
        art.brute = upload(b);
    }
    {
        gs::Bitmap b(56, 52);
        b.rect(24, 2, 4, 10, 6);
        b.rect(16, 10, 24, 4, 6);
        b.ellipse(28, 30, 20, 14, 1);
        b.ellipse(28, 30, 14, 9, 2);
        b.ellipse(28, 31, 8, 5, 4);
        b.ellipse(26, 30, 3, 2, 5);
        b.rect(10, 28, 4, 16, 3);
        b.rect(42, 28, 4, 16, 3);
        art.well = upload(b);
    }
    {
        gs::Bitmap b(12, 56);
        b.rect(3, 4, 6, 50, 1);
        b.rect(4, 4, 2, 50, 2);
        b.line(2, 6, 9, 2, 4, 1);
        b.rect(2, 48, 8, 6, 3);
        art.post = upload(b);
    }
    {
        gs::Bitmap b(5, 14);
        b.rect(2, 2, 1, 10, 1);
        b.rect(1, 1, 3, 2, 2);
        art.arrow = upload(b);
    }
    {
        gs::Bitmap b(18, 18);
        b.ellipse(8, 8, 7, 7, 3);
        b.ellipse(11, 7, 6, 6, 0);
        art.moon = upload(b);
    }
    {
        gs::Bitmap b(8, 8);
        b.rect(1, 1, 6, 6, 1);
        art.pip = upload(b);
    }

    gs::TileAlloc tiles(vdp, 1);
    for (int i = 0; i < 96; i++) glyphTile(vdp, tiles, art.font[i], char(32 + i));

    uint8_t grass[64], grass2[64], dirt[64];
    std::memset(grass, 1, sizeof grass);
    std::memset(grass2, 1, sizeof grass2);
    std::memset(dirt, 4, sizeof dirt);
    grass[3] = 2;
    grass[11] = 3;
    grass[27] = 2;
    grass[44] = 3;
    grass[52] = 5;
    grass2[6] = 3;
    grass2[20] = 2;
    grass2[36] = 5;
    grass2[50] = 2;
    dirt[8] = 6;
    dirt[33] = 5;
    dirt[55] = 6;
    int tg = tiles.shared(grass);
    int tg2 = tiles.shared(grass2);
    int td = tiles.shared(dirt);
    vdp.B.clear();
    for (int cy = 0; cy < 32; cy++) {
        for (int cx = 0; cx < 64; cx++) {
            if (cy >= 22) vdp.B.set(cx, cy, gs::entry((cx + cy) & 1 ? tg2 : tg, PAL_GRASS));
            else if (cy >= 19) vdp.B.set(cx, cy, gs::entry(td, PAL_GRASS));
        }
    }
}

}  // namespace palisade

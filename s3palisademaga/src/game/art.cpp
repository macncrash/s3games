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
    vdp.setFogColor(rgb4(3, 2, 2));
    pal(vdp, PAL_HUD, {0, rgb4(15, 14, 10), rgb4(8, 6, 3), rgb4(13, 3, 2), rgb4(12, 10, 4), rgb4(4, 9, 3)});
    pal(vdp, PAL_WOOD, {0, rgb4(7, 4, 2), rgb4(11, 7, 3), rgb4(4, 2, 1), rgb4(13, 11, 6), rgb4(2, 1, 1)});
    pal(vdp, PAL_EARTH, {0, rgb4(6, 5, 3), rgb4(9, 7, 4), rgb4(4, 3, 2), rgb4(8, 6, 4)});
    pal(vdp, PAL_PLAYER, {0, rgb4(12, 9, 6), rgb4(3, 5, 8), rgb4(2, 3, 5), rgb4(5, 3, 2), rgb4(14, 12, 8),
                          rgb4(9, 7, 3), rgb4(1, 1, 1)});
    pal(vdp, PAL_RAIDER, {0, rgb4(10, 6, 4), rgb4(12, 2, 2), rgb4(6, 1, 1), rgb4(3, 2, 2), rgb4(8, 7, 5),
                          rgb4(13, 9, 4), rgb4(2, 1, 1)});
    pal(vdp, PAL_FEINT, {0, rgb4(8, 8, 9), rgb4(5, 6, 8), rgb4(3, 3, 5), rgb4(11, 11, 12), rgb4(6, 6, 7),
                         rgb4(4, 4, 5), rgb4(2, 2, 3)});
    pal(vdp, PAL_FX, {0, rgb4(15, 13, 6), rgb4(15, 8, 2), rgb4(14, 14, 12), rgb4(6, 5, 2)});
    pal(vdp, PAL_GRASS, {0, rgb4(2, 5, 2), rgb4(3, 7, 3), rgb4(5, 8, 3), rgb4(7, 6, 3), rgb4(3, 4, 2),
                         rgb4(8, 7, 4)});

    auto upload = [&](gs::Bitmap& b) { return gs::uploadImage(vdp, b); };

    {
        gs::Bitmap b(22, 36);
        b.ellipse(11, 6, 4, 4, 1);
        b.rect(9, 3, 4, 3, 5);
        b.rect(8, 10, 7, 12, 2);
        b.rect(6, 12, 3, 8, 3);
        b.rect(9, 22, 3, 11, 4);
        b.rect(13, 22, 3, 11, 4);
        b.rect(15, 12, 6, 3, 6);
        b.rect(19, 10, 2, 7, 7);
        art.sentry = upload(b);
    }
    {
        gs::Bitmap b(18, 30);
        b.ellipse(9, 5, 4, 4, 1);
        b.rect(6, 9, 7, 10, 2);
        b.rect(4, 11, 3, 7, 3);
        b.rect(13, 12, 3, 6, 6);
        b.rect(6, 19, 3, 9, 4);
        b.rect(10, 19, 3, 9, 4);
        b.line(3, 2, 3, 22, 5, 1);
        b.line(2, 2, 7, 4, 5, 1);
        art.raider = upload(b);
    }
    {
        gs::Bitmap b(18, 28);
        b.ellipse(9, 6, 5, 4, 1);
        b.rect(5, 4, 8, 3, 4);
        b.rect(6, 10, 6, 9, 2);
        b.rect(4, 12, 3, 6, 3);
        b.rect(12, 12, 3, 6, 5);
        b.rect(6, 19, 3, 7, 6);
        b.rect(10, 19, 3, 7, 6);
        art.feint = upload(b);
    }
    {
        gs::Bitmap b(10, 48);
        b.rect(3, 6, 4, 40, 1);
        b.rect(4, 6, 2, 40, 2);
        b.line(1, 8, 8, 2, 4, 1);
        b.line(1, 14, 8, 8, 4, 1);
        b.rect(2, 42, 6, 5, 3);
        art.stake = upload(b);
    }
    {
        gs::Bitmap b(6, 12);
        b.rect(1, 1, 4, 9, 1);
        b.rect(1, 8, 4, 3, 2);
        art.round = upload(b);
    }
    {
        gs::Bitmap b(4, 8);
        b.rect(1, 0, 2, 8, 1);
        art.slug = upload(b);
    }

    gs::TileAlloc tiles(vdp, 1);
    for (int i = 0; i < 96; i++) glyphTile(vdp, tiles, art.font[i], char(32 + i));

    uint8_t grass[64], dirt[64];
    std::memset(grass, 1, sizeof grass);
    std::memset(dirt, 4, sizeof dirt);
    grass[4] = 2;
    grass[19] = 3;
    grass[37] = 2;
    grass[50] = 5;
    dirt[9] = 2;
    dirt[28] = 3;
    dirt[46] = 1;
    int tg = tiles.shared(grass);
    int td = tiles.shared(dirt);
    vdp.B.clear();
    vdp.A.enabled = false;
    for (int cy = 0; cy < 32; cy++) {
        for (int cx = 0; cx < 64; cx++) {
            if (cy >= 24) vdp.B.set(cx, cy, gs::entry((cx + cy) & 1 ? tg : td, PAL_GRASS));
            else if (cy >= 21) vdp.B.set(cx, cy, gs::entry(td, PAL_EARTH));
        }
    }
}

}  // namespace palisade

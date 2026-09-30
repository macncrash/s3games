#include "game/art.h"

namespace bellchime {
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

// Crown, flared body of height bodyH, lip, and a clapper. bodyH is the voice.
gs::Image makeBell(gs::VDP& vdp, int bodyH) {
    const int w = 36;
    const int h = 10 + bodyH + 10;
    gs::Bitmap b(w, h);
    b.poly({{18, 1}, {12, 8}, {24, 8}}, 3);
    b.rect(15, 6, 6, 4, 2);
    int top = 10;
    int bot = 10 + bodyH;
    for (int y = top; y < bot; y++) {
        float u = float(y - top) / float(bodyH);
        int half = 6 + int(u * 9.f);
        int c = (y & 3) == 0 ? 3 : 2;
        b.rect(18 - half, y, half * 2, 1, c);
        b.set(18 - half, y, 1);
        b.set(18 + half - 1, y, 1);
    }
    b.rect(4, bot, 28, 4, 4);
    b.rect(6, bot + 1, 24, 1, 3);
    b.ellipse(18, bot + 7, 2, 3, 1);
    return gs::uploadImage(vdp, b);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 14, 12)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 3)});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(6, 6, 8)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(14, 3, 3)});
    setPal(vdp, PAL_BELL, {0, gs::rgb4(5, 3, 1), gs::rgb4(10, 7, 2), gs::rgb4(15, 12, 4), gs::rgb4(8, 5, 2)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(4, 2, 1), gs::rgb4(8, 4, 2), gs::rgb4(12, 7, 3)});
    setPal(vdp, PAL_ROPE, {0, gs::rgb4(9, 7, 3), gs::rgb4(13, 11, 6)});
    setPal(vdp, PAL_NIGHT, {0, gs::rgb4(10, 12, 14), gs::rgb4(14, 14, 12), gs::rgb4(4, 5, 8)});
    setPal(vdp, PAL_SHORT, {0, gs::rgb4(6, 4, 1), gs::rgb4(14, 11, 3), gs::rgb4(15, 15, 8), gs::rgb4(9, 6, 2)});

    loadFont(vdp, art.font);

    {
        gs::Bitmap b(220, 12);
        b.rect(0, 2, 220, 8, 1);
        b.rect(0, 3, 220, 3, 2);
        for (int x = 8; x < 220; x += 18) b.rect(x, 0, 3, 12, 3);
        art.beam = gs::uploadImage(vdp, b);
    }
    art.bellH[0] = kLongH;
    art.bellH[1] = kShortH;
    art.bellH[2] = kDeepH;
    art.bell[0] = makeBell(vdp, kLongH);
    art.bell[1] = makeBell(vdp, kShortH);
    art.bell[2] = makeBell(vdp, kDeepH);
    {
        gs::Bitmap b(4, 48);
        b.rect(1, 0, 2, 48, 1);
        for (int y = 4; y < 48; y += 6) b.rect(0, y, 4, 2, 2);
        art.rope = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(28, 28);
        b.ellipse(14, 14, 13, 13, 1);
        b.ellipse(14, 14, 11, 11, 3);
        b.ellipse(14, 14, 2, 2, 2);
        art.clock = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(3, 3);
        b.rect(0, 1, 3, 1, 1);
        b.rect(1, 0, 1, 3, 1);
        art.pip = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(8, 8);
        b.ellipse(4, 4, 3, 3, 1);
        art.lamp = gs::uploadImage(vdp, b);
    }
}

}  // namespace bellchime

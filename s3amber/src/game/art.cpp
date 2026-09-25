#include "game/art.h"

#include <initializer_list>

namespace amber {
namespace {

using gs::Bitmap;

void pal(gs::VDP& vdp, int p, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(p * 16 + i, c);
        i++;
    }
    while (i < 16) vdp.setColor(p * 16 + i++, 0);
}

void textPal(gs::VDP& vdp, int p, uint16_t ink, uint16_t edge, uint16_t shadow) {
    for (int i = 0; i < 16; i++) vdp.setColor(p * 16 + i, 0);
    vdp.setColor(p * 16 + 1, ink);
    vdp.setColor(p * 16 + 2, edge);
    vdp.setColor(p * 16 + 15, shadow);
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

Bitmap word(const char* s, int scale) { return gs::textBitmap(s, {scale, 1, 2, 15, 1}); }

Bitmap rot180(const Bitmap& s) {
    Bitmap d(s.w, s.h);
    for (int y = 0; y < s.h; y++)
        for (int x = 0; x < s.w; x++) d.set(s.w - 1 - x, s.h - 1 - y, s.get(x, y));
    return d;
}

Bitmap rotCW(const Bitmap& s) {
    Bitmap d(s.h, s.w);
    for (int y = 0; y < s.h; y++)
        for (int x = 0; x < s.w; x++) d.set(s.h - 1 - y, x, s.get(x, y));
    return d;
}

Bitmap rotCCW(const Bitmap& s) {
    Bitmap d(s.h, s.w);
    for (int y = 0; y < s.h; y++)
        for (int x = 0; x < s.w; x++) d.set(y, s.w - 1 - x, s.get(x, y));
    return d;
}

// Facing north. Index 6 is the brake lamp so a palette swap can light it.
Bitmap carUp() {
    Bitmap b(14, 16);
    b.rect(0, 2, 3, 4, 1);
    b.rect(11, 2, 3, 4, 1);
    b.rect(0, 10, 3, 4, 1);
    b.rect(11, 10, 3, 4, 1);
    b.rect(2, 1, 10, 14, 2);
    b.rect(2, 1, 3, 14, 3);
    b.rect(4, 3, 6, 4, 4);
    b.rect(4, 9, 6, 3, 4);
    b.set(3, 0, 5);
    b.set(4, 0, 5);
    b.set(9, 0, 5);
    b.set(10, 0, 5);
    b.rect(3, 14, 3, 2, 6);
    b.rect(8, 14, 3, 2, 6);
    b.rect(5, 12, 4, 2, 7);
    return b;
}

Bitmap copArt(bool halt) {
    Bitmap b(20, 26);
    b.ellipse(10, 4, 6, 3, 2);
    b.rect(5, 5, 10, 2, 3);
    b.ellipse(10, 9, 4, 4, 5);
    b.rect(8, 8, 2, 2, 1);
    b.rect(12, 8, 2, 2, 1);
    b.rect(5, 13, 10, 8, 4);
    b.rect(6, 14, 8, 6, 6);
    b.rect(5, 16, 10, 2, 7);
    b.rect(6, 21, 3, 4, 1);
    b.rect(11, 21, 3, 4, 1);
    if (halt) {
        b.rect(14, 6, 3, 8, 4);
        b.rect(14, 4, 4, 3, 8);
        b.set(16, 3, 9);
    } else {
        b.rect(14, 14, 4, 3, 4);
        b.rect(17, 14, 2, 2, 8);
    }
    b.rect(8, 14, 2, 2, 9);
    return b;
}

Bitmap housing() {
    Bitmap b(12, 26);
    b.rect(3, 0, 6, 20, 1);
    b.rect(5, 20, 2, 6, 1);
    b.ellipse(6, 4, 2.4f, 2.4f, 2);
    b.ellipse(6, 10, 2.4f, 2.4f, 2);
    b.ellipse(6, 16, 2.4f, 2.4f, 2);
    return b;
}

Bitmap lampArt() {
    Bitmap b(8, 8);
    b.ellipse(4, 4, 3, 3, 1);
    b.set(3, 2, 2);
    return b;
}

Bitmap solid(int c) {
    Bitmap b(4, 4);
    b.rect(0, 0, 4, 4, c);
    return b;
}

Bitmap shadowArt() {
    Bitmap b(16, 8);
    b.ellipse(8, 4, 7, 3, 1);
    return b;
}

Bitmap ringArt() {
    Bitmap b(16, 16);
    b.ellipse(8, 8, 7, 7, 1);
    b.ellipse(8, 8, 4, 4, 0);
    return b;
}

Bitmap bracketArt() {
    Bitmap b(24, 24);
    b.rect(0, 0, 7, 2, 1);
    b.rect(0, 0, 2, 7, 1);
    b.rect(17, 0, 7, 2, 1);
    b.rect(22, 0, 2, 7, 1);
    b.rect(0, 22, 7, 2, 1);
    b.rect(0, 17, 2, 7, 1);
    b.rect(17, 22, 7, 2, 1);
    b.rect(22, 17, 2, 7, 1);
    return b;
}

Bitmap blockArt(int variant) {
    Bitmap b(64, 48);
    b.rect(0, 0, 64, 48, 1);
    b.rect(0, 0, 64, 4, 7);
    b.rect(0, 44, 64, 4, 2);
    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 4; c++) {
            int x = 6 + c * 15;
            int y = 8 + r * 12;
            bool lit = ((variant * 5 + r * 3 + c) % 4) != 0;
            int col = 3;
            if (lit) col = ((r + c + variant) & 1) ? 4 : 5;
            b.rect(float(x), float(y), 10, 7, col);
            if (lit) b.rect(float(x + 1), float(y + 1), 3, 2, 8);
        }
    }
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_HUD, gs::rgb4(14, 14, 13), gs::rgb4(2, 2, 3), gs::rgb4(1, 1, 2));
    textPal(vdp, PAL_AMBER, gs::rgb4(15, 10, 2), gs::rgb4(6, 3, 0), gs::rgb4(2, 1, 0));
    textPal(vdp, PAL_RED, gs::rgb4(15, 4, 3), gs::rgb4(5, 1, 1), gs::rgb4(2, 0, 0));
    textPal(vdp, PAL_GREEN, gs::rgb4(8, 15, 8), gs::rgb4(1, 5, 2), gs::rgb4(0, 2, 1));
    textPal(vdp, PAL_DIM, gs::rgb4(8, 8, 9), gs::rgb4(2, 2, 3), gs::rgb4(1, 1, 2));

    pal(vdp, PAL_LAW, {0, gs::rgb4(1, 1, 2), gs::rgb4(3, 6, 11), gs::rgb4(2, 4, 7), gs::rgb4(7, 10, 12),
                       gs::rgb4(15, 15, 11), gs::rgb4(3, 1, 1), gs::rgb4(12, 12, 9), gs::rgb4(9, 12, 14)});
    pal(vdp, PAL_RUN, {0, gs::rgb4(1, 1, 2), gs::rgb4(13, 7, 1), gs::rgb4(8, 4, 1), gs::rgb4(6, 8, 9),
                       gs::rgb4(15, 14, 8), gs::rgb4(4, 2, 1), gs::rgb4(8, 3, 1), gs::rgb4(15, 11, 4)});
    pal(vdp, PAL_BRAKE, {0, gs::rgb4(1, 1, 2), gs::rgb4(3, 6, 11), gs::rgb4(2, 4, 7), gs::rgb4(7, 10, 12),
                         gs::rgb4(15, 15, 11), gs::rgb4(15, 2, 2), gs::rgb4(12, 12, 9), gs::rgb4(9, 12, 14)});
    pal(vdp, PAL_COP, {0, gs::rgb4(1, 1, 2), gs::rgb4(2, 3, 6), gs::rgb4(1, 2, 4), gs::rgb4(4, 5, 8),
                       gs::rgb4(12, 8, 6), gs::rgb4(3, 4, 7), gs::rgb4(14, 9, 2), gs::rgb4(15, 15, 13),
                       gs::rgb4(15, 12, 4)});
    pal(vdp, PAL_SIGNAL, {0, gs::rgb4(2, 3, 3), gs::rgb4(1, 1, 1), gs::rgb4(6, 7, 6)});
    pal(vdp, PAL_LAMP_R, {0, gs::rgb4(15, 2, 2), gs::rgb4(15, 12, 10)});
    pal(vdp, PAL_LAMP_A, {0, gs::rgb4(15, 10, 1), gs::rgb4(15, 15, 12)});
    pal(vdp, PAL_LAMP_G, {0, gs::rgb4(4, 14, 4), gs::rgb4(14, 15, 12)});
    pal(vdp, PAL_ROAD, {0, gs::rgb4(3, 3, 4), gs::rgb4(4, 4, 6), gs::rgb4(14, 14, 13), gs::rgb4(13, 8, 1),
                        gs::rgb4(1, 1, 2)});
    pal(vdp, PAL_BLOCK, {0, gs::rgb4(3, 3, 5), gs::rgb4(2, 2, 3), gs::rgb4(1, 1, 2), gs::rgb4(13, 8, 2),
                         gs::rgb4(4, 7, 11), gs::rgb4(5, 3, 2), gs::rgb4(6, 6, 8), gs::rgb4(15, 12, 6)});
    pal(vdp, PAL_FX, {0, gs::rgb4(15, 11, 3), gs::rgb4(15, 15, 13), gs::rgb4(8, 5, 1)});

    loadFont(vdp, art);
    art.title = gs::uploadMipped(vdp, word("AMBER", 4));
    art.clear = gs::uploadMipped(vdp, word("BOX CLEAR", 2));
    art.over = gs::uploadMipped(vdp, word("LIGHT WINS", 2));

    Bitmap up = carUp();
    art.car[0] = gs::uploadMipped(vdp, up);
    art.car[1] = gs::uploadMipped(vdp, rotCW(up));
    art.car[2] = gs::uploadMipped(vdp, rot180(up));
    art.car[3] = gs::uploadMipped(vdp, rotCCW(up));
    art.cop[0] = gs::uploadMipped(vdp, copArt(false));
    art.cop[1] = gs::uploadMipped(vdp, copArt(true));
    art.signal = gs::uploadMipped(vdp, housing());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.white = gs::uploadMipped(vdp, solid(3));
    art.amberLine = gs::uploadMipped(vdp, solid(4));
    art.asphalt = gs::uploadMipped(vdp, solid(1));
    art.pad = gs::uploadMipped(vdp, solid(2));
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.ring = gs::uploadMipped(vdp, ringArt());
    art.bracket = gs::uploadMipped(vdp, bracketArt());
    for (int i = 0; i < 4; i++) art.block[i] = gs::uploadMipped(vdp, blockArt(i));
}

}  // namespace amber

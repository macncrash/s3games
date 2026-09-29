#include "game/art.h"

namespace shelvebell {
namespace {

void setPal(gs::VDP& vdp, int pal, const uint16_t cs[16]) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, cs[i]);
}

void ink(gs::VDP& vdp, int pal, uint16_t c, uint16_t shadow) {
    uint16_t cs[16] = {};
    cs[1] = c;
    cs[15] = shadow;
    setPal(vdp, pal, cs);
}

void stamp(gs::Bitmap& b, char ch, int x, int y, int c, int scale) {
    const uint8_t* g = gs::glyph(ch);
    for (int gy = 0; gy < 7; gy++) {
        for (int gx = 0; gx < 5; gx++) {
            if (!g[gy * 5 + gx]) continue;
            for (int sy = 0; sy < scale; sy++)
                for (int sx = 0; sx < scale; sx++) b.set(x + gx * scale + sx, y + gy * scale + sy, c);
        }
    }
}

void bookPalettes(gs::VDP& vdp) {
    const uint16_t gilt = gs::rgb4(15, 12, 4);
    const uint16_t inkc = gs::rgb4(15, 14, 12);
    const uint16_t page = gs::rgb4(13, 11, 8);
    const uint16_t pageD = gs::rgb4(8, 7, 5);
    struct Tone {
        int pal;
        uint16_t spine, dark, lite;
    };
    const Tone tones[4] = {
        {PAL_A, gs::rgb4(11, 2, 3), gs::rgb4(5, 1, 1), gs::rgb4(15, 8, 6)},
        {PAL_B, gs::rgb4(2, 5, 11), gs::rgb4(1, 2, 5), gs::rgb4(7, 10, 15)},
        {PAL_C, gs::rgb4(2, 8, 5), gs::rgb4(1, 4, 2), gs::rgb4(8, 14, 9)},
        {PAL_D, gs::rgb4(9, 4, 10), gs::rgb4(4, 1, 5), gs::rgb4(14, 9, 14)},
    };
    for (const Tone& t : tones) {
        uint16_t cs[16] = {};
        cs[1] = t.spine;
        cs[2] = t.dark;
        cs[3] = t.lite;
        cs[4] = gilt;
        cs[5] = inkc;
        cs[6] = page;
        cs[7] = pageD;
        setPal(vdp, t.pal, cs);
    }
}

gs::Bitmap bookOf(char letter) {
    gs::Bitmap b(16, 30);
    b.rect(0, 0, 12, 30, 1);
    b.rect(0, 0, 2, 30, 2);
    b.rect(9, 1, 3, 28, 3);
    b.rect(12, 1, 4, 28, 6);
    b.rect(14, 2, 1, 26, 7);
    for (int y = 4; y < 26; y += 2) b.set(13, y, 7);
    b.rect(0, 0, 12, 2, 2);
    b.rect(0, 28, 12, 2, 2);
    b.rect(2, 4, 7, 1, 4);
    b.rect(2, 24, 7, 1, 4);
    stamp(b, letter, 2, 10, 2, 1);
    stamp(b, letter, 3, 11, 5, 1);
    return b;
}

gs::Bitmap plateOf(char letter) {
    gs::Bitmap b(18, 22);
    b.rect(0, 0, 18, 22, 2);
    b.rect(2, 2, 14, 18, 1);
    b.rect(2, 2, 14, 2, 3);
    stamp(b, letter, 6, 8, 5, 1);
    return b;
}

gs::Bitmap cartArt() {
    gs::Bitmap b(58, 42);
    b.rect(2, 1, 16, 3, 1);
    b.rect(2, 1, 16, 1, 5);
    b.rect(3, 4, 3, 12, 2);
    b.rect(14, 12, 4, 16, 2);
    b.rect(14, 12, 1, 16, 4);
    b.rect(46, 12, 4, 16, 1);
    b.rect(49, 12, 1, 16, 5);
    b.rect(14, 24, 36, 6, 1);
    b.rect(14, 24, 36, 1, 5);
    b.rect(14, 28, 36, 2, 3);
    b.ellipse(22, 36, 5, 5, 12);
    b.ellipse(22, 36, 2, 2, 13);
    b.ellipse(42, 36, 5, 5, 12);
    b.ellipse(42, 36, 2, 2, 13);
    b.rect(22, 35, 20, 2, 13);
    return b;
}

gs::Bitmap plankArt() {
    gs::Bitmap b(148, 7);
    for (int y = 0; y < 7; y++) {
        for (int x = 0; x < 148; x++) {
            int c = 1;
            if (((x / 5) + y * 2) % 7 == 0) c = 2;
            if (y >= 5) c = 3;
            if (y == 0) c = 5;
            b.set(x, y, c);
        }
    }
    return b;
}

gs::Bitmap railArt() {
    gs::Bitmap b(140, 3);
    b.rect(0, 0, 140, 3, 2);
    b.rect(0, 0, 140, 1, 4);
    return b;
}

gs::Bitmap caseArt() {
    gs::Bitmap b(168, 176);
    b.rect(0, 0, 168, 176, 8);
    b.rect(4, 4, 160, 168, 9);
    b.rect(0, 0, 6, 176, 3);
    b.rect(162, 0, 6, 176, 1);
    b.rect(0, 0, 168, 8, 2);
    b.rect(0, 168, 168, 8, 3);
    for (int i = 0; i < 4; i++) b.rect(8, 18 + i * 40, 152, 2, 7);
    return b;
}

gs::Bitmap bracketArt() {
    gs::Bitmap b(156, 36);
    b.rect(0, 0, 4, 36, 1);
    b.rect(152, 0, 4, 36, 1);
    b.rect(0, 32, 156, 4, 1);
    b.rect(4, 34, 148, 2, 2);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(28, 32);
    b.rect(12, 0, 4, 5, 2);
    b.rect(13, 0, 2, 3, 4);
    b.poly({{6, 8}, {22, 8}, {26, 22}, {2, 22}}, 1);
    b.poly({{8, 10}, {20, 10}, {22, 20}, {6, 20}}, 3);
    b.rect(1, 22, 26, 4, 1);
    b.rect(0, 25, 28, 3, 2);
    b.ellipse(14, 16, 2.2f, 2.2f, 5);
    b.rect(13, 26, 2, 5, 6);
    b.ellipse(14, 31, 2.4f, 1.6f, 6);
    return b;
}

gs::Bitmap clapperArt() {
    gs::Bitmap b(6, 8);
    b.rect(2, 0, 2, 3, 2);
    b.ellipse(3, 5, 2.2f, 2.2f, 1);
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
    ink(vdp, PAL_INK, gs::rgb4(15, 15, 14), gs::rgb4(3, 2, 2));
    ink(vdp, PAL_GOLD, gs::rgb4(15, 13, 5), gs::rgb4(4, 3, 1));
    ink(vdp, PAL_ALERT, gs::rgb4(15, 5, 3), gs::rgb4(4, 1, 1));
    ink(vdp, PAL_OK, gs::rgb4(6, 15, 7), gs::rgb4(1, 4, 2));
    ink(vdp, PAL_DIM, gs::rgb4(10, 8, 6), gs::rgb4(2, 1, 1));
    ink(vdp, PAL_MARK, gs::rgb4(15, 14, 8), gs::rgb4(5, 4, 1));
    bookPalettes(vdp);

    const uint16_t wood[16] = {
        0,
        gs::rgb4(12, 8, 4),
        gs::rgb4(8, 5, 2),
        gs::rgb4(4, 3, 1),
        gs::rgb4(3, 2, 1),
        gs::rgb4(15, 12, 7),
        gs::rgb4(6, 8, 3),
        gs::rgb4(7, 5, 2),
        gs::rgb4(9, 6, 4),
        gs::rgb4(11, 8, 5),
        gs::rgb4(13, 11, 8),
        gs::rgb4(5, 4, 3),
        gs::rgb4(2, 2, 2),
        gs::rgb4(6, 6, 6),
        gs::rgb4(14, 13, 10),
        gs::rgb4(1, 1, 1),
    };
    setPal(vdp, PAL_WOOD, wood);

    const uint16_t brass[16] = {
        0,
        gs::rgb4(14, 11, 3),
        gs::rgb4(8, 6, 2),
        gs::rgb4(15, 14, 7),
        gs::rgb4(15, 15, 12),
        gs::rgb4(6, 4, 2),
        gs::rgb4(3, 2, 1),
        0, 0, 0, 0, 0, 0, 0, 0,
        gs::rgb4(2, 1, 0),
    };
    setPal(vdp, PAL_BELL, brass);

    const char* letters = "ABCD";
    for (int i = 0; i < 4; i++) {
        art.book[i] = gs::uploadMipped(vdp, bookOf(letters[i]));
        art.plate[i] = gs::uploadMipped(vdp, plateOf(letters[i]));
    }
    art.cart = gs::uploadMipped(vdp, cartArt());
    art.plank = gs::uploadMipped(vdp, plankArt());
    art.rail = gs::uploadMipped(vdp, railArt());
    art.caseBack = gs::uploadMipped(vdp, caseArt());
    art.bracket = gs::uploadMipped(vdp, bracketArt());
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.clapper = gs::uploadMipped(vdp, clapperArt());
    loadFont(vdp, art);
    vdp.setFogColor(gs::rgb4(4, 3, 2));
}

}  // namespace shelvebell

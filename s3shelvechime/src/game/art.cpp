#include "game/art.h"

#include <cmath>

namespace shelvechime {
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
    const uint16_t gold = gs::rgb4(15, 12, 4);
    const uint16_t inkc = gs::rgb4(15, 14, 11);
    const uint16_t page = gs::rgb4(14, 12, 9);
    const uint16_t pageD = gs::rgb4(8, 7, 5);
    struct Tone {
        int pal;
        uint16_t spine, dark, lite;
    };
    const Tone tones[4] = {
        {PAL_A, gs::rgb4(12, 2, 2), gs::rgb4(6, 1, 1), gs::rgb4(15, 8, 6)},
        {PAL_B, gs::rgb4(2, 4, 12), gs::rgb4(1, 2, 6), gs::rgb4(7, 10, 15)},
        {PAL_C, gs::rgb4(2, 9, 4), gs::rgb4(1, 5, 2), gs::rgb4(8, 14, 8)},
        {PAL_D, gs::rgb4(10, 3, 11), gs::rgb4(5, 1, 6), gs::rgb4(14, 8, 15)},
    };
    for (const Tone& t : tones) {
        uint16_t cs[16] = {};
        cs[1] = t.spine;
        cs[2] = t.dark;
        cs[3] = t.lite;
        cs[4] = gold;
        cs[5] = inkc;
        cs[6] = page;
        cs[7] = pageD;
        setPal(vdp, t.pal, cs);
    }
}

gs::Bitmap bookOf(char letter) {
    gs::Bitmap b(BOOK_W, BOOK_H);
    b.rect(0, 0, 12, BOOK_H, 1);
    b.rect(0, 0, 2, BOOK_H, 2);
    b.rect(9, 1, 3, BOOK_H - 2, 3);
    b.rect(12, 1, 3, BOOK_H - 2, 6);
    b.rect(14, 2, 1, BOOK_H - 4, 7);
    for (int y = 3; y < BOOK_H - 3; y += 2) b.set(13, y, 7);
    b.rect(0, 0, 12, 2, 2);
    b.rect(0, BOOK_H - 2, 12, 2, 2);
    b.rect(2, 4, 8, 1, 4);
    b.rect(2, BOOK_H - 6, 8, 1, 4);
    stamp(b, letter, 2, 9, 2, 2);
    stamp(b, letter, 1, 8, 5, 2);
    return b;
}

gs::Bitmap plateOf(char letter) {
    gs::Bitmap b(PLATE_W, PLATE_H);
    b.rect(0, 0, PLATE_W, PLATE_H, 2);
    b.rect(2, 2, PLATE_W - 4, PLATE_H - 4, 1);
    b.rect(2, 2, PLATE_W - 4, 2, 3);
    stamp(b, letter, 6, 8, 2, 2);
    stamp(b, letter, 5, 7, 5, 2);
    return b;
}

gs::Bitmap cartArt() {
    gs::Bitmap b(58, 42);
    b.rect(2, 2, 3, 14, 2);
    b.rect(1, 0, 16, 3, 1);
    b.rect(1, 0, 16, 1, 5);
    b.rect(14, 12, 4, 16, 2);
    b.rect(46, 12, 4, 16, 1);
    b.rect(14, 26, 36, 5, 1);
    b.rect(14, 26, 36, 1, 5);
    b.rect(14, 30, 36, 1, 3);
    b.ellipse(22, 36, 5, 5, 4);
    b.ellipse(22, 36, 2, 2, 7);
    b.ellipse(42, 36, 5, 5, 4);
    b.ellipse(42, 36, 2, 2, 7);
    return b;
}

gs::Bitmap plankArt() {
    gs::Bitmap b(140, 6);
    b.rect(0, 0, 140, 6, 1);
    b.rect(0, 0, 140, 1, 5);
    b.rect(0, 5, 140, 1, 3);
    for (int x = 18; x < 140; x += 28) b.rect(x, 1, 1, 4, 7);
    return b;
}

gs::Bitmap railArt() {
    gs::Bitmap b(128, 3);
    b.rect(0, 0, 128, 3, 1);
    b.rect(0, 0, 128, 1, 3);
    return b;
}

gs::Bitmap residentsArt() {
    gs::Bitmap b(36, 28);
    for (int i = 0; i < 3; i++) {
        int x = 2 + i * 11;
        b.rect(x, 2, 8, 24, 1);
        b.rect(x, 2, 2, 24, 2);
        b.rect(x + 6, 3, 2, 22, 5);
        b.rect(x + 1, 6, 5, 1, 4);
    }
    return b;
}

gs::Bitmap caseArt() {
    gs::Bitmap b(150, 168);
    b.rect(0, 0, 150, 168, 2);
    b.rect(4, 4, 142, 160, 9);
    b.rect(4, 4, 142, 3, 1);
    b.rect(4, 161, 142, 3, 3);
    b.rect(4, 4, 3, 160, 1);
    b.rect(143, 4, 3, 160, 3);
    return b;
}

gs::Bitmap bracketArt() {
    gs::Bitmap b(150, 36);
    b.rect(0, 14, 6, 8, 1);
    b.rect(144, 14, 6, 8, 1);
    b.rect(4, 16, 142, 2, 1);
    return b;
}

gs::Bitmap clockArt() {
    gs::Bitmap b(36, 36);
    b.ellipse(18, 18, 17, 17, 1);
    b.ellipse(18, 18, 14, 14, 6);
    b.ellipse(18, 18, 12, 12, 5);
    for (int i = 0; i < 12; i++) {
        float a = float(i) * 3.1415926f / 6.f;
        int x = int(18 + std::sin(a) * 10.f);
        int y = int(18 - std::cos(a) * 10.f);
        b.set(x, y, i % 3 == 0 ? 4 : 2);
    }
    b.ellipse(18, 18, 2, 2, 4);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(16, 18);
    b.poly({{8, 0}, {10, 3}, {6, 3}}, 4);
    b.poly({{3, 4}, {13, 4}, {15, 13}, {1, 13}}, 1);
    b.poly({{5, 6}, {11, 6}, {12, 12}, {4, 12}}, 3);
    b.ellipse(8, 15, 3, 2, 4);
    b.rect(7, 2, 2, 2, 2);
    return b;
}

gs::Bitmap dotArt() {
    gs::Bitmap b(4, 4);
    b.ellipse(2, 2, 1.6f, 1.6f, 1);
    return b;
}

gs::Bitmap bannerArt() {
    gs::Bitmap b(120, 14);
    b.rect(0, 0, 120, 14, 1);
    b.rect(2, 2, 116, 10, 2);
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

const uint16_t kWood[16] = {
    0,
    gs::rgb4(12, 8, 4),
    gs::rgb4(8, 5, 2),
    gs::rgb4(4, 2, 1),
    gs::rgb4(2, 2, 2),
    gs::rgb4(15, 12, 7),
    gs::rgb4(6, 8, 3),
    gs::rgb4(9, 6, 3),
    gs::rgb4(11, 5, 2),
    gs::rgb4(10, 8, 5),
    gs::rgb4(13, 11, 8),
    gs::rgb4(7, 4, 2),
    gs::rgb4(14, 10, 6),
    gs::rgb4(5, 4, 3),
    gs::rgb4(3, 3, 2),
    gs::rgb4(1, 1, 1),
};

const uint16_t kRoom[16] = {
    0,
    gs::rgb4(14, 12, 8),
    gs::rgb4(6, 5, 4),
    gs::rgb4(15, 14, 8),
    gs::rgb4(15, 12, 3),
    gs::rgb4(12, 11, 9),
    gs::rgb4(15, 15, 13),
    gs::rgb4(8, 7, 5),
    gs::rgb4(4, 3, 2),
    gs::rgb4(9, 7, 4),
    gs::rgb4(11, 9, 6),
    gs::rgb4(3, 3, 4),
    gs::rgb4(13, 10, 5),
    gs::rgb4(7, 6, 4),
    gs::rgb4(2, 2, 2),
    gs::rgb4(1, 1, 1),
};

const uint16_t kMark[16] = {
    0,
    gs::rgb4(15, 14, 6),
    gs::rgb4(10, 8, 2),
    gs::rgb4(15, 15, 10),
    gs::rgb4(8, 7, 2),
    gs::rgb4(14, 12, 4),
    gs::rgb4(6, 5, 1),
    gs::rgb4(12, 11, 5),
    gs::rgb4(4, 3, 1),
    gs::rgb4(15, 13, 7),
    gs::rgb4(9, 8, 3),
    gs::rgb4(7, 6, 2),
    gs::rgb4(13, 12, 6),
    gs::rgb4(5, 4, 1),
    gs::rgb4(11, 10, 4),
    gs::rgb4(2, 2, 1),
};

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    ink(vdp, PAL_INK, gs::rgb4(15, 15, 14), gs::rgb4(3, 2, 2));
    ink(vdp, PAL_GOLD, gs::rgb4(15, 13, 5), gs::rgb4(4, 3, 1));
    ink(vdp, PAL_ALERT, gs::rgb4(15, 5, 3), gs::rgb4(4, 1, 1));
    ink(vdp, PAL_OK, gs::rgb4(6, 15, 7), gs::rgb4(1, 4, 2));
    ink(vdp, PAL_DIM, gs::rgb4(10, 8, 6), gs::rgb4(2, 1, 1));
    bookPalettes(vdp);
    setPal(vdp, PAL_WOOD, kWood);
    setPal(vdp, PAL_ROOM, kRoom);
    setPal(vdp, PAL_MARK, kMark);
    vdp.setFogColor(gs::rgb4(3, 2, 2));

    const char letters[4] = {'A', 'B', 'C', 'D'};
    for (int i = 0; i < 4; i++) {
        art.book[i] = gs::uploadMipped(vdp, bookOf(letters[i]));
        art.plate[i] = gs::uploadMipped(vdp, plateOf(letters[i]));
    }
    art.cart = gs::uploadMipped(vdp, cartArt());
    art.plank = gs::uploadMipped(vdp, plankArt());
    art.rail = gs::uploadMipped(vdp, railArt());
    art.residents = gs::uploadMipped(vdp, residentsArt());
    art.caseBack = gs::uploadMipped(vdp, caseArt());
    art.bracket = gs::uploadMipped(vdp, bracketArt());
    art.clock = gs::uploadMipped(vdp, clockArt());
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.dot = gs::uploadMipped(vdp, dotArt());
    art.banner = gs::uploadMipped(vdp, bannerArt());
    loadFont(vdp, art);
}

}  // namespace shelvechime

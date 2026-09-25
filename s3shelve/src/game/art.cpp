#include "game/art.h"

namespace shelve {
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
    const uint16_t pageD = gs::rgb4(9, 8, 6);
    struct Tone {
        int pal;
        uint16_t spine, dark, lite;
    };
    const Tone tones[4] = {
        {PAL_A, gs::rgb4(12, 2, 2), gs::rgb4(6, 1, 1), gs::rgb4(15, 7, 5)},
        {PAL_B, gs::rgb4(2, 4, 12), gs::rgb4(1, 2, 6), gs::rgb4(6, 9, 15)},
        {PAL_C, gs::rgb4(2, 9, 4), gs::rgb4(1, 5, 2), gs::rgb4(7, 14, 8)},
        {PAL_D, gs::rgb4(10, 3, 11), gs::rgb4(5, 1, 6), gs::rgb4(14, 8, 14)},
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
    b.rect(0, 0, 14, BOOK_H, 1);
    b.rect(0, 0, 2, BOOK_H, 2);
    b.rect(11, 1, 3, BOOK_H - 2, 3);
    b.rect(14, 1, 4, BOOK_H - 2, 6);
    b.rect(16, 2, 1, BOOK_H - 4, 7);
    for (int y = 3; y < BOOK_H - 3; y += 2) b.set(15, y, 7);
    b.rect(0, 0, 14, 2, 2);
    b.rect(0, BOOK_H - 2, 14, 2, 2);
    b.rect(2, 4, 9, 1, 4);
    b.rect(2, BOOK_H - 6, 9, 1, 4);
    stamp(b, letter, 3, 10, 2, 2);
    stamp(b, letter, 2, 9, 5, 2);
    return b;
}

gs::Bitmap plateOf(char letter) {
    gs::Bitmap b(PLATE_W, PLATE_H);
    b.rect(0, 0, PLATE_W, PLATE_H, 2);
    b.rect(2, 2, PLATE_W - 4, PLATE_H - 4, 1);
    b.rect(2, 2, PLATE_W - 4, 2, 3);
    b.rect(3, 5, PLATE_W - 6, 1, 4);
    b.rect(3, PLATE_H - 6, PLATE_W - 6, 1, 4);
    stamp(b, letter, 8, 9, 2, 2);
    stamp(b, letter, 7, 8, 5, 2);
    return b;
}

gs::Bitmap cartArt() {
    gs::Bitmap b(64, 46);
    // Push handle, left of the bed, so the open side faces the stacks.
    b.rect(4, 2, 3, 16, 2);
    b.rect(2, 0, 18, 3, 1);
    b.rect(2, 0, 18, 1, 5);
    b.rect(18, 1, 2, 8, 12);
    // Side posts of the bin. The middle stays open so the books show through.
    b.rect(16, 14, 4, 18, 2);
    b.rect(16, 14, 1, 18, 4);
    b.rect(52, 14, 4, 18, 1);
    b.rect(55, 14, 1, 18, 5);
    // Front lip, toward the bottom of the bitmap, covers the heels of the books.
    b.rect(16, 28, 40, 6, 1);
    b.rect(16, 28, 40, 1, 5);
    b.rect(16, 32, 40, 2, 3);
    // Wheels and axle.
    b.ellipse(26, 39, 6, 6, 12);
    b.ellipse(26, 39, 2.4f, 2.4f, 13);
    b.ellipse(48, 39, 6, 6, 12);
    b.ellipse(48, 39, 2.4f, 2.4f, 13);
    b.rect(26, 38, 22, 2, 13);
    b.set(26, 39, 14);
    b.set(48, 39, 14);
    return b;
}

gs::Bitmap plankArt() {
    gs::Bitmap b(PLANK_W, PLANK_H);
    for (int y = 0; y < PLANK_H; y++) {
        for (int x = 0; x < PLANK_W; x++) {
            int c = 1;
            if (((x / 4) + y * 3) % 7 == 0) c = 2;
            if ((x / 6 + y) % 13 == 0) c = 5;
            if (y >= 6) c = 3;
            if (y == 0) c = 5;
            if (x < 2 || x >= PLANK_W - 2) c = 4;
            b.set(x, y, c);
        }
    }
    return b;
}

gs::Bitmap railArt() {
    gs::Bitmap b(PLANK_W - 8, 3);
    b.rect(0, 0, b.w, 3, 1);
    b.rect(0, 0, b.w, 1, 3);
    return b;
}

gs::Bitmap residentsArt() {
    gs::Bitmap b(56, 32);
    const int spines[][3] = {
        {1, 26, 6}, {9, 30, 7}, {17, 24, 8}, {25, 31, 9}, {34, 27, 6}, {43, 29, 8},
    };
    for (const int* s : spines) {
        int x = s[0], h = s[1], c = s[2];
        int y = 32 - h;
        b.rect(x, y, 6, h, c);
        b.rect(x + 5, y, 2, h, 10);
        b.rect(x, y, 6, 1, 4);
        b.rect(x + 1, y + 4, 4, 1, 5);
        b.rect(x + 1, y + h - 5, 4, 1, 5);
    }
    return b;
}

gs::Bitmap caseArt() {
    gs::Bitmap b(CASE_W, CASE_H);
    b.rect(8, 6, CASE_W - 16, CASE_H - 10, 3);
    b.rect(10, 8, CASE_W - 20, CASE_H - 16, 2);
    // Posts.
    b.rect(0, 0, 8, CASE_H, 1);
    b.rect(0, 0, 2, CASE_H, 4);
    b.rect(6, 0, 2, CASE_H, 5);
    b.rect(CASE_W - 8, 0, 8, CASE_H, 1);
    b.rect(CASE_W - 8, 0, 2, CASE_H, 5);
    b.rect(CASE_W - 2, 0, 2, CASE_H, 4);
    // Cornice and plinth.
    b.rect(0, 0, CASE_W, 6, 1);
    b.rect(0, 0, CASE_W, 2, 5);
    b.rect(0, 4, CASE_W, 2, 3);
    b.rect(0, CASE_H - 6, CASE_W, 6, 2);
    b.rect(0, CASE_H - 2, CASE_W, 2, 4);
    for (int i = 0; i < 4; i++) {
        int y = 28 + i * 44;
        b.ellipse(4, float(y), 1.4f, 1.4f, 14);
        b.ellipse(CASE_W - 5.f, float(y), 1.4f, 1.4f, 14);
    }
    return b;
}

gs::Bitmap bracketArt() {
    gs::Bitmap b(160, 42);
    auto arm = [&](int x, int y, int sx, int sy) {
        for (int i = 0; i < 20; i++) {
            for (int t = 0; t < 3; t++) {
                b.set(x + sx * i, y + sy * t, t == 2 ? 2 : 1);
                b.set(x + sx * t, y + sy * i, t == 2 ? 2 : 1);
            }
        }
    };
    arm(0, 0, 1, 1);
    arm(b.w - 1, 0, -1, 1);
    arm(0, b.h - 1, 1, -1);
    arm(b.w - 1, b.h - 1, -1, -1);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(18, 28);
    for (int y = 0; y < 8; y += 3) b.rect(8, y, 2, 2, 1);
    b.poly({{2, 10}, {16, 10}, {18, 22}, {0, 22}}, 1);
    b.poly({{4, 12}, {14, 12}, {15, 20}, {3, 20}}, 2);
    b.ellipse(9, 18, 2.2f, 2.2f, 3);
    b.rect(7, 22, 4, 2, 1);
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
    bookPalettes(vdp);

    const uint16_t wood[16] = {
        0,
        gs::rgb4(13, 9, 4),
        gs::rgb4(9, 6, 3),
        gs::rgb4(5, 3, 1),
        gs::rgb4(3, 2, 1),
        gs::rgb4(15, 12, 7),
        gs::rgb4(5, 7, 3),
        gs::rgb4(8, 5, 2),
        gs::rgb4(10, 4, 2),
        gs::rgb4(12, 10, 6),
        gs::rgb4(13, 12, 9),
        gs::rgb4(9, 8, 6),
        gs::rgb4(7, 7, 8),
        gs::rgb4(3, 3, 4),
        gs::rgb4(14, 13, 11),
        gs::rgb4(2, 1, 1),
    };
    const uint16_t room[16] = {
        0,
        gs::rgb4(12, 9, 3),
        gs::rgb4(15, 13, 6),
        gs::rgb4(15, 15, 12),
        gs::rgb4(2, 3, 6),
        gs::rgb4(6, 4, 3),
        gs::rgb4(8, 10, 12),
        gs::rgb4(9, 2, 2),
        gs::rgb4(5, 1, 1),
        gs::rgb4(12, 9, 3),
        0,
        0,
        0,
        0,
        0,
        gs::rgb4(2, 1, 1),
    };
    const uint16_t mark[16] = {
        0,
        gs::rgb4(15, 14, 6),
        gs::rgb4(15, 15, 13),
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        gs::rgb4(4, 3, 1),
    };
    setPal(vdp, PAL_WOOD, wood);
    setPal(vdp, PAL_ROOM, room);
    setPal(vdp, PAL_MARK, mark);

    loadFont(vdp, art);
    const char* letters = "ABCD";
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
    art.lamp = gs::uploadMipped(vdp, lampArt());

    gs::TextStyle logo{3, 1, 0, 15, 1};
    gs::TextStyle ban{2, 1, 0, 15, 1};
    art.logo = gs::uploadMipped(vdp, gs::textBitmap("S3 SHELVE", logo));
    art.come = gs::uploadMipped(vdp, gs::textBitmap("COMES BACK", ban));
    art.clear = gs::uploadMipped(vdp, gs::textBitmap("CART CLEAR", ban));
    art.goes = gs::uploadMipped(vdp, gs::textBitmap("CART GOES BACK", ban));
}

}  // namespace shelve

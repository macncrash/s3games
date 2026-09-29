#include "game/art.h"

namespace shelvemark {
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
    struct Tone {
        int pal;
        uint16_t spine, dark, lite;
    };
    const Tone tones[4] = {
        {PAL_A, gs::rgb4(13, 2, 2), gs::rgb4(6, 1, 1), gs::rgb4(15, 8, 5)},
        {PAL_B, gs::rgb4(2, 5, 13), gs::rgb4(1, 2, 7), gs::rgb4(7, 10, 15)},
        {PAL_C, gs::rgb4(2, 10, 4), gs::rgb4(1, 5, 2), gs::rgb4(8, 15, 8)},
        {PAL_D, gs::rgb4(11, 3, 12), gs::rgb4(5, 1, 6), gs::rgb4(15, 8, 14)},
    };
    for (const Tone& t : tones) {
        uint16_t cs[16] = {};
        cs[1] = t.spine;
        cs[2] = t.dark;
        cs[3] = t.lite;
        cs[4] = gold;
        cs[5] = inkc;
        cs[6] = page;
        setPal(vdp, t.pal, cs);
    }
}

gs::Bitmap bookOf(char letter) {
    gs::Bitmap b(BOOK_W, BOOK_H);
    b.rect(0, 0, 12, BOOK_H, 1);
    b.rect(0, 0, 2, BOOK_H, 2);
    b.rect(10, 1, 2, BOOK_H - 2, 3);
    b.rect(12, 2, 4, BOOK_H - 4, 6);
    b.rect(0, 0, 12, 2, 2);
    b.rect(0, BOOK_H - 2, 12, 2, 2);
    b.rect(2, 4, 8, 1, 4);
    b.rect(2, BOOK_H - 6, 8, 1, 4);
    stamp(b, letter, 2, 10, 2, 2);
    stamp(b, letter, 1, 9, 5, 2);
    return b;
}

gs::Bitmap plateOf(char letter) {
    gs::Bitmap b(22, 26);
    b.rect(0, 0, 22, 26, 2);
    b.rect(2, 2, 18, 22, 1);
    b.rect(2, 2, 18, 2, 3);
    stamp(b, letter, 6, 8, 5, 2);
    return b;
}

gs::Bitmap cartArt() {
    gs::Bitmap b(58, 40);
    b.rect(2, 2, 3, 14, 2);
    b.rect(0, 0, 16, 3, 1);
    b.rect(0, 0, 16, 1, 5);
    b.rect(14, 14, 4, 16, 2);
    b.rect(46, 14, 4, 16, 1);
    b.rect(14, 26, 36, 5, 1);
    b.rect(14, 26, 36, 1, 5);
    b.ellipse(24, 35, 5, 5, 12);
    b.ellipse(24, 35, 2, 2, 13);
    b.ellipse(42, 35, 5, 5, 12);
    b.ellipse(42, 35, 2, 2, 13);
    b.rect(24, 34, 18, 2, 13);
    return b;
}

gs::Bitmap shelfArt() {
    gs::Bitmap b(120, 8);
    b.rect(0, 0, 120, 8, 1);
    b.rect(0, 0, 120, 1, 5);
    b.rect(0, 6, 120, 2, 3);
    for (int x = 4; x < 116; x += 9) b.set(x, 3, 2);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(10, 176);
    b.rect(0, 0, 10, 176, 1);
    b.rect(0, 0, 2, 176, 5);
    b.rect(8, 0, 2, 176, 3);
    return b;
}

gs::Bitmap markArt() {
    gs::Bitmap b(20, 20);
    b.rect(2, 2, 16, 16, 1);
    b.rect(5, 5, 10, 10, 2);
    b.rect(8, 8, 4, 4, 1);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(16, 22);
    b.rect(7, 0, 2, 6, 1);
    b.poly({{1, 8}, {15, 8}, {13, 16}, {3, 16}}, 1);
    b.ellipse(8, 13, 2, 2, 2);
    b.rect(6, 16, 4, 2, 1);
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
        gs::rgb4(12, 8, 4),
        gs::rgb4(8, 5, 2),
        gs::rgb4(5, 3, 1),
        gs::rgb4(3, 2, 1),
        gs::rgb4(15, 12, 7),
        gs::rgb4(6, 8, 3),
        gs::rgb4(9, 6, 3),
        gs::rgb4(11, 5, 2),
        gs::rgb4(13, 11, 7),
        gs::rgb4(10, 9, 7),
        0,
        gs::rgb4(6, 6, 7),
        gs::rgb4(2, 2, 3),
        gs::rgb4(14, 13, 10),
        gs::rgb4(2, 1, 1),
    };
    const uint16_t room[16] = {
        0, gs::rgb4(14, 11, 4), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 1, 1),
    };
    const uint16_t mark[16] = {
        0, gs::rgb4(15, 13, 3), gs::rgb4(15, 15, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(5, 3, 1),
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
    art.shelf = gs::uploadMipped(vdp, shelfArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.mark = gs::uploadMipped(vdp, markArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());

    gs::TextStyle logo{2, 1, 0, 15, 1};
    gs::TextStyle ban{2, 1, 0, 15, 1};
    art.logo = gs::uploadMipped(vdp, gs::textBitmap("SHELVE MARK", logo));
    art.backBan = gs::uploadMipped(vdp, gs::textBitmap("COMES BACK", ban));
    art.heldBan = gs::uploadMipped(vdp, gs::textBitmap("MARK HELD", ban));
    art.doneBan = gs::uploadMipped(vdp, gs::textBitmap("MARK FINISHED", ban));
    art.lostBan = gs::uploadMipped(vdp, gs::textBitmap("MARK OPEN", ban));
}

}  // namespace shelvemark

#include "game/art.h"

#include <initializer_list>

namespace hornmark {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink, uint16_t edge) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, ink);
    vdp.setColor(pal * 16 + 2, edge);
    vdp.setColor(pal * 16 + 15, edge);
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

gs::Image up(gs::VDP& vdp, const gs::Bitmap& b) { return gs::uploadImage(vdp, b); }

void paintPlayer(gs::Bitmap& b) {
    b.ellipse(16, 10, 6.2f, 6.4f, 3);
    b.rect(12, 4, 8, 3, 8);
    b.rect(13, 8, 3, 2, 4);
    b.rect(11, 16, 12, 16, 2);
    b.rect(11, 16, 3, 16, 5);
    b.rect(12, 30, 10, 3, 6);
    b.rect(13, 33, 4, 14, 7);
    b.rect(19, 33, 4, 14, 7);
    b.rect(11, 45, 7, 3, 9);
    b.rect(18, 45, 7, 3, 9);
    b.rect(8, 20, 4, 10, 3);
    b.rect(22, 18, 5, 4, 3);
    b.outline(1, false);
}

void paintHorn(gs::Bitmap& b) {
    b.ellipse(10, 18, 8.f, 7.f, 3);
    b.ellipse(10, 18, 4.2f, 3.4f, 1);
    b.rect(16, 14, 22, 6, 4);
    b.rect(16, 16, 22, 2, 6);
    b.rect(36, 12, 8, 10, 5);
    b.ellipse(46, 17, 7.f, 9.f, 5);
    b.ellipse(48, 17, 4.f, 6.f, 1);
    b.rect(8, 20, 6, 3, 7);
    b.outline(1, false);
}

void paintBell(gs::Bitmap& b) {
    b.ellipse(8, 8, 7.f, 7.f, 4);
    b.ellipse(9, 8, 3.4f, 4.2f, 1);
    b.outline(1, false);
}

void paintNote(gs::Bitmap& b) {
    b.ellipse(6, 10, 5.2f, 3.6f, 2);
    b.rect(10, 2, 2, 10, 2);
    b.rect(10, 2, 5, 2, 3);
    b.outline(1, false);
}

void paintStaff(gs::Bitmap& b) {
    b.rect(0, 3, 48, 2, 2);
}

void paintPine(gs::Bitmap& b) {
    b.poly({{14, 2}, {26, 28}, {2, 28}}, 3);
    b.poly({{14, 12}, {28, 40}, {0, 40}}, 4);
    b.rect(12, 38, 5, 10, 5);
    b.outline(1, false);
}

void paintMoon(gs::Bitmap& b) {
    b.ellipse(12, 12, 10.f, 10.f, 3);
    b.ellipse(16, 10, 8.f, 8.f, 1);
    b.outline(1, false);
}

void paintBreath(gs::Bitmap& b) {
    b.ellipse(8, 6, 6.f, 3.f, 2);
    b.ellipse(16, 8, 4.f, 2.2f, 3);
    b.outline(1, false);
}

void paintStamp(gs::Bitmap& b) {
    b.rect(2, 2, 28, 16, 3);
    b.rect(4, 4, 24, 12, 2);
    b.rect(6, 7, 8, 6, 4);
    b.rect(16, 7, 8, 6, 4);
    b.outline(1, false);
}

void paintRock(gs::Bitmap& b) {
    b.ellipse(16, 10, 14.f, 7.f, 3);
    b.ellipse(10, 8, 5.f, 3.f, 4);
    b.outline(1, false);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_INK, gs::rgb4(14, 14, 12), gs::rgb4(2, 2, 3));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 12, 3), gs::rgb4(4, 2, 0));
    textPal(vdp, PAL_TITLE, gs::rgb4(15, 14, 10), gs::rgb4(3, 2, 4));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 5, 4), gs::rgb4(3, 0, 0));
    setPal(vdp, PAL_BRASS, {0, gs::rgb4(2, 1, 0), gs::rgb4(8, 5, 1), gs::rgb4(12, 8, 2), gs::rgb4(15, 12, 4),
                            gs::rgb4(10, 7, 2), gs::rgb4(6, 4, 1), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_COAT, {0, gs::rgb4(1, 1, 2), gs::rgb4(3, 3, 6), gs::rgb4(8, 6, 4), gs::rgb4(12, 9, 6),
                           gs::rgb4(2, 2, 4), gs::rgb4(6, 4, 2), gs::rgb4(9, 7, 3), gs::rgb4(4, 2, 1),
                           gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_PINE, {0, gs::rgb4(0, 1, 0), gs::rgb4(1, 3, 1), gs::rgb4(2, 6, 2), gs::rgb4(3, 8, 3),
                           gs::rgb4(4, 3, 1)});
    setPal(vdp, PAL_NOTE, {0, gs::rgb4(1, 1, 2), gs::rgb4(15, 15, 14), gs::rgb4(15, 12, 3)});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(1, 1, 2), gs::rgb4(6, 6, 8), gs::rgb4(4, 4, 5)});
    setPal(vdp, PAL_FACE, {0, gs::rgb4(2, 1, 1), gs::rgb4(12, 8, 5), gs::rgb4(15, 12, 8), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_MOON, {0, gs::rgb4(2, 2, 4), gs::rgb4(10, 10, 12), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_STAFF, {0, gs::rgb4(1, 1, 2), gs::rgb4(12, 11, 8)});
    setPal(vdp, PAL_BREATH, {0, gs::rgb4(2, 3, 4), gs::rgb4(10, 12, 13), gs::rgb4(14, 15, 15)});
    setPal(vdp, PAL_HILL, {0, gs::rgb4(1, 2, 1), gs::rgb4(3, 5, 2), gs::rgb4(5, 6, 3), gs::rgb4(7, 7, 4)});
    setPal(vdp, PAL_SHADE, {0, gs::rgb4(0, 0, 0), gs::rgb4(0, 0, 0)});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(2, 2, 6)});

    loadFont(vdp, art);
    gs::Bitmap player(32, 52);
    paintPlayer(player);
    art.player = up(vdp, player);
    gs::Bitmap horn(56, 32);
    paintHorn(horn);
    art.horn = up(vdp, horn);
    gs::Bitmap bell(16, 16);
    paintBell(bell);
    art.bell = up(vdp, bell);
    gs::Bitmap note(16, 16);
    paintNote(note);
    art.note = up(vdp, note);
    gs::Bitmap staff(48, 8);
    paintStaff(staff);
    art.staff = up(vdp, staff);
    gs::Bitmap pine(30, 50);
    paintPine(pine);
    art.pine = up(vdp, pine);
    gs::Bitmap moon(24, 24);
    paintMoon(moon);
    art.moon = up(vdp, moon);
    gs::Bitmap breath(24, 14);
    paintBreath(breath);
    art.breath = up(vdp, breath);
    gs::Bitmap stamp(32, 20);
    paintStamp(stamp);
    art.stamp = up(vdp, stamp);
    gs::Bitmap rock(32, 16);
    paintRock(rock);
    art.rock = up(vdp, rock);
    vdp.setFogColor(gs::rgb4(2, 2, 4));
}

}  // namespace hornmark

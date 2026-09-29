#include "game/art.h"

namespace cistern {

static void pal(gs::VDP& vdp, int p, int i, int r, int g, int b) { vdp.setColor(p * 16 + i, gs::rgb4(r, g, b)); }

static gs::Image up(gs::VDP& vdp, const gs::Bitmap& b) { return gs::uploadImage(vdp, b); }

void buildArt(gs::VDP& vdp, Art& a) {
    pal(vdp, PAL_HUD, 1, 14, 13, 9);
    pal(vdp, PAL_HUD, 2, 12, 8, 3);
    pal(vdp, PAL_HUD, 3, 6, 7, 6);
    pal(vdp, PAL_HUD, 4, 14, 4, 3);

    pal(vdp, PAL_STONE, 1, 5, 5, 5);
    pal(vdp, PAL_STONE, 2, 7, 7, 6);
    pal(vdp, PAL_STONE, 3, 4, 4, 4);
    pal(vdp, PAL_STONE, 4, 3, 4, 3);
    pal(vdp, PAL_STONE, 5, 8, 8, 7);
    pal(vdp, PAL_STONE, 6, 2, 3, 2);
    pal(vdp, PAL_STONE, 7, 1, 2, 2);
    pal(vdp, PAL_STONE, 8, 3, 5, 3);
    pal(vdp, PAL_STONE, 9, 9, 8, 6);

    pal(vdp, PAL_WATER, 1, 1, 4, 5);
    pal(vdp, PAL_WATER, 2, 2, 7, 8);
    pal(vdp, PAL_WATER, 3, 4, 10, 10);
    pal(vdp, PAL_WATER, 4, 1, 3, 4);
    pal(vdp, PAL_WATER, 5, 8, 12, 11);

    pal(vdp, PAL_WOOD, 1, 6, 4, 2);
    pal(vdp, PAL_WOOD, 2, 8, 5, 2);
    pal(vdp, PAL_WOOD, 3, 4, 3, 1);
    pal(vdp, PAL_WOOD, 4, 10, 7, 3);
    pal(vdp, PAL_WOOD, 5, 5, 5, 6);
    pal(vdp, PAL_WOOD, 6, 9, 9, 10);
    pal(vdp, PAL_WOOD, 7, 3, 3, 4);
    pal(vdp, PAL_WOOD, 8, 12, 10, 6);

    pal(vdp, PAL_BODY, 1, 2, 2, 3);
    pal(vdp, PAL_BODY, 2, 4, 4, 5);
    pal(vdp, PAL_BODY, 3, 12, 8, 6);
    pal(vdp, PAL_BODY, 4, 8, 5, 4);
    pal(vdp, PAL_BODY, 5, 14, 12, 8);
    pal(vdp, PAL_BODY, 6, 6, 3, 2);

    pal(vdp, PAL_ALARM, 1, 14, 12, 4);
    pal(vdp, PAL_ALARM, 2, 14, 6, 3);
    pal(vdp, PAL_ALARM, 3, 15, 15, 12);

    gs::Bitmap arch(168, 150);
    arch.rect(0, 0, 168, 150, 1);
    for (int y = 0; y < 150; y += 12) {
        int shift = ((y / 12) & 1) ? 10 : 0;
        for (int x = -20; x < 168; x += 22) {
            int c = 2 + ((x / 22 + y / 12) % 3);
            arch.rect(x + shift + 1, y + 1, 20, 10, c);
        }
    }
    for (int i = 0; i < 8; i++) arch.rect(18 + i * 18, 8, 3, 134, 6);
    arch.rect(14, 138, 140, 8, 9);
    arch.ellipse(84, 72, 58, 56, 7);
    arch.ellipse(84, 74, 48, 48, 0);
    arch.ellipse(84, 30, 40, 8, 8);
    a.arch = up(vdp, arch);

    gs::Bitmap water(92, 70);
    water.rect(0, 18, 92, 52, 1);
    water.ellipse(46, 22, 44, 14, 2);
    for (int i = 0; i < 6; i++) water.rect(6 + i * 14, 30 + (i % 3) * 8, 10, 2, 3);
    water.rect(0, 58, 92, 12, 4);
    water.ellipse(30, 26, 8, 3, 5);
    a.water = up(vdp, water);

    gs::Bitmap door(64, 108);
    for (int i = 0; i < 5; i++) {
        door.rect(4 + i * 12, 4, 10, 100, (i % 2) ? 2 : 1);
        door.line(4 + i * 12, 4, 4 + i * 12, 104, 3, 1);
    }
    door.rect(2, 2, 60, 104, 3);
    door.rect(4, 4, 56, 100, 0);
    for (int i = 0; i < 5; i++) door.rect(6 + i * 11, 6, 9, 96, (i % 2) ? 2 : 1);
    door.rect(4, 22, 56, 6, 5);
    door.rect(4, 78, 56, 6, 5);
    door.rect(6, 23, 52, 2, 6);
    door.rect(6, 79, 52, 2, 6);
    door.ellipse(50, 54, 4, 4, 7);
    door.ellipse(50, 54, 2, 2, 8);
    a.door = up(vdp, door);

    gs::Bitmap iron(8, 36);
    iron.rect(2, 0, 4, 36, 6);
    iron.rect(0, 2, 8, 4, 5);
    iron.rect(0, 30, 8, 4, 5);
    a.iron = up(vdp, iron);

    gs::Bitmap body(40, 64);
    body.ellipse(22, 10, 7, 8, 3);
    body.rect(16, 8, 6, 4, 4);
    body.rect(14, 18, 16, 26, 1);
    body.rect(12, 20, 6, 18, 2);
    body.line(14, 24, 2, 40, 2, 4);
    body.line(28, 24, 38, 36, 5, 3);
    body.rect(12, 42, 8, 18, 1);
    body.rect(22, 42, 8, 18, 2);
    body.rect(10, 58, 10, 4, 6);
    body.rect(22, 58, 10, 4, 6);
    a.body = up(vdp, body);

    gs::Bitmap shoulder(44, 58);
    shoulder.ellipse(24, 14, 7, 8, 3);
    shoulder.rect(16, 20, 20, 18, 1);
    shoulder.line(16, 26, 2, 34, 2, 5);
    shoulder.line(34, 24, 42, 20, 5, 3);
    shoulder.rect(14, 36, 9, 16, 1);
    shoulder.rect(24, 36, 9, 16, 2);
    shoulder.rect(12, 50, 11, 4, 6);
    shoulder.rect(24, 50, 11, 4, 6);
    a.shoulder = up(vdp, shoulder);

    gs::Bitmap arrow(18, 14);
    arrow.poly({{2, 7}, {10, 1}, {10, 5}, {16, 5}, {16, 9}, {10, 9}, {10, 13}}, 1);
    a.arrow = up(vdp, arrow);

    gs::Bitmap drip(3, 8);
    drip.ellipse(1, 2, 1.4f, 2.2f, 2);
    drip.rect(1, 4, 1, 4, 3);
    a.drip = up(vdp, drip);

    gs::Bitmap bar(8, 8);
    bar.rect(0, 0, 8, 8, 1);
    a.bar = up(vdp, bar);

    gs::TextStyle big;
    big.scale = 2;
    big.color = 1;
    a.word = up(vdp, gs::textBitmap("HOLD THE DOOR", big));

    gs::TextStyle st;
    st.scale = 1;
    st.color = 1;
    for (int c = 32; c < 128; c++) a.glyph[c - 32] = up(vdp, gs::textBitmap(std::string(1, char(c)), st));
}

}  // namespace cistern

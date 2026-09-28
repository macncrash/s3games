#include "game/art.h"

#include <initializer_list>

namespace safetape {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

constexpr int WALL = 1, WAINSCOT = 2, WOOD = 3, BRASS = 4, STEEL = 5, GAP = 6;
constexpr int PAPER = 7, RULE = 8, LAMP = 9, SHADE = 10, FLOOR = 11, GROUT = 12, PLATE = 13;

void paintRoom(gs::Bitmap& b, Art& art) {
    b.rect(0, 0, 320, 224, WALL);
    b.rect(0, 0, 320, 18, SHADE);
    b.rect(0, 18, 320, 6, WOOD);
    b.rect(0, 150, 320, 10, WAINSCOT);
    b.rect(0, 160, 320, 64, FLOOR);
    for (int x = 0; x < 320; x += 20) b.rect(x, 160, 1, 64, GROUT);

    // Banker's lamp over the tape.
    b.rect(36, 28, 4, 22, BRASS);
    b.ellipse(38, 26, 22, 10, LAMP);
    b.ellipse(38, 30, 16, 6, SHADE);

    // The tape itself: three ruled lines. Digits are sprites, so the paper stays blank.
    b.rect(18, 48, 62, 88, WOOD);
    b.rect(22, 44, 54, 88, PAPER);
    b.rect(22, 44, 54, 8, RULE);
    for (int i = 0; i < 3; i++) {
        int y = 64 + i * 22;
        b.rect(28, y + 12, 42, 1, RULE);
        art.tapeX[i] = 48;
        art.tapeY[i] = float(y);
    }

    // Close plate. Same total, different stops. Numbers are sprites.
    b.rect(248, 52, 58, 46, PLATE);
    b.rect(252, 56, 50, 38, PAPER);
    art.nearY = 74;
    art.nearX[0] = 262;
    art.nearX[1] = 276;
    art.nearX[2] = 290;

    // Safe body, drawer mouth, hinge.
    b.rect(108, 36, 104, 108, STEEL);
    b.rect(114, 42, 92, 70, GAP);
    b.rect(114, 116, 92, 22, WOOD);
    b.rect(120, 122, 18, 12, PAPER);
    b.rect(148, 122, 18, 12, PAPER);
    b.rect(176, 122, 18, 12, PAPER);
    b.rect(104, 48, 6, 70, BRASS);
    for (int i = 0; i < 4; i++) b.rect(106, 56 + i * 16, 3, 3, SHADE);

    art.doorX = 118;
    art.doorY = 46;
    art.doorW = 84;
    art.doorH = 62;
    art.dialX[0] = 140;
    art.dialX[1] = 160;
    art.dialX[2] = 180;
    art.dialY = 78;
    art.drawerX[0] = 129;
    art.drawerX[1] = 157;
    art.drawerX[2] = 185;
    art.drawerY = 128;
}

gs::Bitmap paintDoor() {
    gs::Bitmap b(84, 62);
    b.rect(0, 0, 84, 62, 1);
    b.rect(4, 4, 76, 54, 2);
    b.ellipse(42, 30, 18, 18, 3);
    b.ellipse(42, 30, 12, 12, 4);
    b.ellipse(42, 30, 4, 4, 5);
    b.rect(40, 12, 4, 8, 5);
    b.rect(58, 28, 14, 4, 5);
    for (int i = 0; i < 6; i++) b.rect(8 + i * 12, 6, 3, 3, 6);
    return b;
}

gs::Bitmap paintCaret() {
    gs::Bitmap b(12, 7);
    b.poly({{6, 1}, {1, 6}, {11, 6}}, 1);
    return b;
}

gs::Bitmap paintWalker() {
    gs::Bitmap b(14, 26);
    b.ellipse(7, 4, 3.2f, 3.2f, 1);
    b.rect(5, 8, 4, 9, 1);
    b.rect(2, 9, 3, 7, 2);
    b.rect(9, 9, 3, 7, 2);
    b.rect(5, 17, 2, 8, 3);
    b.rect(8, 17, 2, 8, 3);
    return b;
}

gs::Bitmap digitBitmap(int d) {
    char s[2] = {char('0' + d), 0};
    return gs::textBitmap(s, gs::TextStyle{3, 1, 0, 0, 0});
}

void keepFull(gs::Mipped& m, gs::VDP& vdp, const gs::Bitmap& g) {
    gs::Image img = gs::uploadImage(vdp, g);
    m.w = g.w;
    m.h = g.h;
    m.lv[0] = m.lv[1] = m.lv[2] = img;
}

void loadFont(gs::VDP& vdp, Art& art, gs::TileAlloc& tiles) {
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) px[y * 8 + x + 1] = 1;
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        art.font[c - 32] = t;
    }
    for (int d = 0; d < 10; d++) keepFull(art.digit[d], vdp, digitBitmap(d));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(14, 13, 11)});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 12, 4)});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 4, 3)});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(5, 14, 7)});
    setPal(vdp, PAL_ROOM,
           {0, gs::rgb4(4, 5, 6), gs::rgb4(6, 5, 4), gs::rgb4(7, 4, 2), gs::rgb4(12, 9, 3), gs::rgb4(8, 9, 10),
            gs::rgb4(2, 2, 3), gs::rgb4(14, 13, 10), gs::rgb4(9, 3, 3), gs::rgb4(6, 12, 5), gs::rgb4(2, 3, 3),
            gs::rgb4(5, 4, 3), gs::rgb4(3, 3, 3), gs::rgb4(10, 8, 5)});
    setPal(vdp, PAL_INK, {0, gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_WHEEL, {0, gs::rgb4(15, 14, 11)});
    setPal(vdp, PAL_DOOR,
           {0, gs::rgb4(7, 8, 9), gs::rgb4(11, 12, 13), gs::rgb4(12, 9, 3), gs::rgb4(4, 3, 2), gs::rgb4(15, 13, 6),
            gs::rgb4(5, 6, 7)});
    setPal(vdp, PAL_TAPE, {0, gs::rgb4(3, 2, 2)});
    setPal(vdp, PAL_WALK, {0, gs::rgb4(12, 9, 6), gs::rgb4(4, 6, 9), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_SHADE, {0, gs::rgb4(1, 1, 2)});

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, art, tiles);

    gs::Bitmap room(gs::SCREEN_W, gs::SCREEN_H);
    paintRoom(room, art);
    vdp.B.clear();
    gs::bitmapToPlane(tiles, vdp.B, 0, 0, room, PAL_ROOM);
    vdp.A.enabled = false;
    vdp.A.clear();
    vdp.B.enabled = true;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        vdp.lineBackdrop[y] = gs::rgb4(3, 4, 5);
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }
    vdp.setFogColor(gs::rgb4(2, 2, 3));

    art.door = gs::uploadMipped(vdp, paintDoor());
    art.caret = gs::uploadMipped(vdp, paintCaret());
    art.walker = gs::uploadMipped(vdp, paintWalker());
    gs::Bitmap solid(8, 8);
    solid.rect(0, 0, 8, 8, 1);
    art.solid = gs::uploadMipped(vdp, solid);
}

}  // namespace safetape

#include "pictures.h"

#include <initializer_list>
#include <string>

#include "world.h"

namespace pouch {
namespace {

using gs::Bitmap;
using gs::Pt;

enum C : int {
    C_ASPH = 1,
    C_ASPH2,
    C_WALK,
    C_WALK2,
    C_CURB,
    C_GRASS,
    C_GRASS2,
    C_YELLOW,
    C_WHITE,
    C_BRICK,
    C_AWN,
    C_WIN,
    C_DOOR,
    C_INK,
    C_GLASS
};

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void pals(gs::VDP& vdp) {
    const uint16_t ink = gs::rgb4(15, 14, 11);
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    auto text = [&](int pal, uint16_t face, uint16_t edge) {
        setPal(vdp, pal, {0, face, edge, gs::rgb4(8, 8, 9), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    };
    text(PAL_HUD, ink, gs::rgb4(2, 2, 3));
    text(PAL_TITLE, ink, gs::rgb4(2, 1, 2));
    text(PAL_ALARM, gs::rgb4(15, 4, 3), gs::rgb4(3, 0, 0));
    text(PAL_SAFE, gs::rgb4(8, 15, 6), gs::rgb4(1, 3, 1));
    text(PAL_GOLD, gs::rgb4(15, 12, 3), gs::rgb4(4, 2, 0));

    setPal(vdp, PAL_CITY, {0, gs::rgb4(3, 3, 4), gs::rgb4(5, 5, 6), gs::rgb4(10, 9, 8), gs::rgb4(7, 6, 6),
                           gs::rgb4(12, 11, 9), gs::rgb4(3, 8, 3), gs::rgb4(2, 5, 2), gs::rgb4(14, 12, 2),
                           gs::rgb4(15, 15, 14), gs::rgb4(9, 4, 3), gs::rgb4(13, 2, 3), gs::rgb4(14, 12, 6),
                           gs::rgb4(5, 2, 2), gs::rgb4(2, 1, 1), gs::rgb4(6, 8, 11)});

    setPal(vdp, PAL_HERO, {0, gs::rgb4(3, 4, 10), gs::rgb4(6, 8, 14), gs::rgb4(14, 10, 7), gs::rgb4(13, 2, 2),
                           gs::rgb4(2, 2, 4), gs::rgb4(12, 8, 3), gs::rgb4(14, 11, 6), gs::rgb4(1, 1, 2),
                           gs::rgb4(15, 13, 4), 0, 0, 0, 0, 0, shadow});

    auto car = [&](int pal, uint16_t body, uint16_t body2, uint16_t roof, uint16_t stripe) {
        setPal(vdp, pal, {0, body, body2, gs::rgb4(5, 9, 13), roof, gs::rgb4(15, 15, 12), gs::rgb4(1, 1, 1),
                          gs::rgb4(1, 1, 2), gs::rgb4(15, 6, 2), stripe, 0, 0, 0, 0, 0, shadow});
    };
    car(PAL_RED, gs::rgb4(13, 2, 2), gs::rgb4(8, 1, 1), gs::rgb4(9, 1, 1), gs::rgb4(4, 1, 1));
    car(PAL_BLUE, gs::rgb4(2, 5, 13), gs::rgb4(1, 3, 8), gs::rgb4(1, 2, 7), gs::rgb4(8, 10, 12));
    car(PAL_TAXI, gs::rgb4(15, 12, 2), gs::rgb4(12, 9, 1), gs::rgb4(10, 8, 1), gs::rgb4(1, 1, 1));
    car(PAL_VAN, gs::rgb4(2, 8, 4), gs::rgb4(1, 5, 2), gs::rgb4(1, 4, 2), gs::rgb4(12, 12, 10));
    car(PAL_VAN2, gs::rgb4(8, 5, 2), gs::rgb4(5, 3, 1), gs::rgb4(4, 2, 1), gs::rgb4(12, 10, 6));
    car(PAL_BUS, gs::rgb4(14, 13, 9), gs::rgb4(11, 10, 7), gs::rgb4(9, 8, 6), gs::rgb4(2, 4, 12));
    car(PAL_BUS2, gs::rgb4(12, 2, 2), gs::rgb4(8, 1, 1), gs::rgb4(6, 1, 1), gs::rgb4(14, 12, 4));

    setPal(vdp, PAL_PROP, {0, gs::rgb4(4, 4, 5), gs::rgb4(8, 8, 7), gs::rgb4(15, 14, 8), gs::rgb4(1, 1, 2),
                           gs::rgb4(6, 4, 2), gs::rgb4(2, 7, 3), gs::rgb4(4, 10, 4), 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FX, {0, gs::rgb4(10, 9, 8), gs::rgb4(15, 14, 12), gs::rgb4(14, 12, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
    gs::TextStyle big;
    big.scale = 3;
    big.color = 1;
    big.outline = 2;
    big.spacing = 1;
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
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

void stamp(Bitmap& b, const char* s, int x, int y, int color) {
    gs::TextStyle st;
    st.scale = 1;
    st.color = color;
    st.spacing = 1;
    b.blit(gs::textBitmap(s, st), x, y);
}

void slabs(Bitmap& b, int x0, int x1, int y0, int y1, int fill, int line) {
    b.rect(float(x0), float(y0), float(x1 - x0), float(y1 - y0), fill);
    for (int y = y0; y < y1; y++)
        for (int x = x0; x < x1; x++)
            if (((x - x0) % 16) == 0 || ((y - y0) % 10) == 0) b.set(x, y, line);
}

void street(Bitmap& b, int y0, int y1, bool right, const char* num) {
    const int w = gs::SCREEN_W;
    b.rect(0, float(y0), float(w), float(y1 - y0), C_ASPH);
    for (int y = y0 + 2; y < y1 - 1; y += 3)
        for (int x = 1; x < w; x += 7)
            if ((((x * 13) ^ (y * 7)) & 31) < 3) b.set(x, y, C_ASPH2);
    b.rect(0, float(y0), float(w), 1, C_WHITE);
    b.rect(0, float(y1 - 1), float(w), 1, C_WHITE);
    const int mid = (y0 + y1) / 2;
    for (int x = 0; x < w; x += 18) {
        if (x + 8 >= 112 && x <= 208) continue;
        b.rect(float(x), float(mid - 1), 8, 2, C_YELLOW);
    }
    for (int y = y0 + 3; y + 3 < y1; y += 7) b.rect(112, float(y), 96, 3, C_WHITE);
    if (right)
        b.poly({{18, mid - 5.f}, {32, float(mid)}, {18, mid + 5.f}}, C_YELLOW);
    else
        b.poly({{w - 18.f, mid - 5.f}, {w - 32.f, float(mid)}, {w - 18.f, mid + 5.f}}, C_YELLOW);
    stamp(b, num, right ? 40 : w - 48, y0 + 4, C_YELLOW);
}

void median(Bitmap& b, int y0, int y1) {
    b.rect(0, float(y0), float(gs::SCREEN_W), float(y1 - y0), C_GRASS);
    b.rect(0, float(y0), float(gs::SCREEN_W), 2, C_GRASS2);
    b.rect(0, float(y1 - 2), float(gs::SCREEN_W), 2, C_GRASS2);
    slabs(b, 112, 208, y0, y1, C_WALK, C_WALK2);
}

void city(Bitmap& b) {
    const int w = gs::SCREEN_W;
    b.rect(0, 0, float(w), float(gs::SCREEN_H), C_ASPH);
    for (int i = 0; i < 8; i++) {
        int x = i * 40;
        b.rect(float(x), 0, 40, float(Y_SHOP1), (i & 1) ? C_BRICK : C_DOOR);
        b.rect(float(x + 3), 1, 14, 6, (i % 3) ? C_WIN : C_GLASS);
        if (i != 3 && i != 4) b.rect(float(x + 20), 5, 16, 6, (i & 1) ? C_AWN : C_YELLOW);
    }
    b.rect(146, 0, 28, float(Y_SHOP1), C_DOOR);
    b.rect(156, 1, 8, 6, C_INK);
    stamp(b, "DROP", 149, 1, C_WHITE);

    slabs(b, 0, w, Y_GOAL0, Y_GOAL1, C_WALK, C_WALK2);
    b.rect(136, 14, 48, 16, C_AWN);
    b.rect(140, 17, 40, 10, C_BRICK);
    b.rect(0, float(Y_CURB0), float(w), float(Y_CURB1 - Y_CURB0), C_CURB);

    street(b, Y_ST3_0, Y_ST3_1, true, "3");
    median(b, Y_MED2_0, Y_MED2_1);
    street(b, Y_ST2_0, Y_ST2_1, false, "2");
    median(b, Y_MED1_0, Y_MED1_1);
    street(b, Y_ST1_0, Y_ST1_1, true, "1");

    b.rect(0, float(Y_CURB2_0), float(w), float(Y_CURB2_1 - Y_CURB2_0), C_CURB);
    slabs(b, 0, w, Y_START0, Y_START1, C_WALK, C_WALK2);
    stamp(b, "START", 8, 208, C_INK);
}

Bitmap courier(int step, bool pouch) {
    Bitmap b(20, 20);
    b.rect(5, 1, 10, 3, 4);
    b.rect(4, 3, 12, 2, 4);
    b.rect(6, 5, 8, 4, 3);
    b.set(8, 6, 8);
    b.set(12, 6, 8);
    b.rect(4, 9, 12, 7, 1);
    b.rect(5, 10, 10, 2, 2);
    b.rect(2, 10, 2, 5, 1);
    b.rect(16, 10, 2, 5, 1);
    b.set(2, 15, 3);
    b.set(17, 15, 3);
    if (pouch) {
        b.line(8, 10, 13, 12, 7, 1.2f);
        b.rect(11, 11, 6, 6, 6);
        b.rect(12, 12, 4, 2, 7);
        b.set(13, 15, 9);
    }
    int a = step & 1;
    b.rect(6, 16, 3, 3, 5);
    b.rect(11, 16, 3, 3, 5);
    b.rect(6, 18 - a, 3, 2, 8);
    b.rect(11, 17 + a, 3, 2, 8);
    b.outline(8, false);
    return b;
}

Bitmap satchelArt() {
    Bitmap b(12, 10);
    b.rect(2, 2, 8, 6, 6);
    b.rect(3, 3, 6, 2, 7);
    b.rect(4, 1, 4, 2, 7);
    b.set(6, 5, 9);
    b.outline(8, false);
    return b;
}

void wheels(Bitmap& b, int x, int y, int w, int h) {
    b.rect(float(x), float(y), 5, 4, 6);
    b.rect(float(x + w - 5), float(y), 5, 4, 6);
    b.rect(float(x), float(y + h - 4), 5, 4, 6);
    b.rect(float(x + w - 5), float(y + h - 4), 5, 4, 6);
}

Bitmap sedanArt() {
    Bitmap b(SEDAN_W, SEDAN_H);
    wheels(b, 3, 1, 28, SEDAN_H - 2);
    b.rect(2, 3, 36, 12, 1);
    b.rect(14, 4, 12, 10, 3);
    b.rect(16, 5, 8, 8, 4);
    b.rect(18, 2, 6, 2, 9);
    b.rect(3, 8, 34, 2, 9);
    b.rect(35, 4, 3, 3, 5);
    b.rect(35, 11, 3, 3, 5);
    b.rect(2, 4, 2, 3, 8);
    b.rect(2, 11, 2, 3, 8);
    b.outline(7, false);
    return b;
}

Bitmap vanArt() {
    Bitmap b(VAN_W, VAN_H);
    wheels(b, 4, 1, 40, VAN_H - 2);
    b.rect(2, 3, 48, 14, 1);
    b.rect(30, 4, 14, 12, 3);
    b.rect(32, 5, 8, 10, 4);
    b.rect(6, 5, 20, 10, 2);
    b.line(28, 4, 28, 16, 9, 1.2f);
    b.rect(3, 9, 46, 2, 9);
    b.rect(47, 5, 3, 3, 5);
    b.rect(47, 12, 3, 3, 5);
    b.rect(2, 5, 2, 3, 8);
    b.rect(2, 12, 2, 3, 8);
    b.outline(7, false);
    return b;
}

Bitmap busArt() {
    Bitmap b(BUS_W, BUS_H);
    wheels(b, 6, 1, 28, BUS_H - 2);
    wheels(b, 42, 1, 28, BUS_H - 2);
    b.rect(2, 3, 72, 18, 1);
    b.rect(3, 4, 70, 16, 2);
    for (int i = 0; i < 5; i++) b.rect(float(8 + i * 10), 5, 7, 6, 3);
    b.rect(58, 5, 10, 14, 4);
    b.rect(60, 8, 6, 8, 3);
    b.rect(3, 12, 68, 3, 9);
    b.rect(70, 6, 4, 4, 5);
    b.rect(70, 14, 4, 4, 5);
    b.rect(2, 6, 2, 4, 8);
    b.rect(2, 14, 2, 4, 8);
    b.outline(7, false);
    return b;
}

Bitmap lampArt() {
    Bitmap b(10, 24);
    b.rect(4, 8, 2, 14, 1);
    b.ellipse(5, 5, 4, 3, 2);
    b.rect(2, 6, 6, 2, 3);
    b.outline(4, false);
    return b;
}

Bitmap treeArt() {
    Bitmap b(16, 16);
    b.rect(7, 9, 2, 6, 5);
    b.ellipse(8, 7, 6, 5, 6);
    b.ellipse(6, 6, 3, 3, 7);
    b.outline(4, false);
    return b;
}

Bitmap shadowArt() {
    Bitmap b(18, 8);
    b.ellipse(9, 4, 8, 3, 1);
    return b;
}

Bitmap puffArt() {
    Bitmap b(14, 14);
    b.ellipse(8, 8, 5, 3, 1);
    b.ellipse(6, 7, 3, 2, 2);
    b.ellipse(10, 6, 2, 2, 3);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    pals(vdp);
    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    Bitmap ground(gs::SCREEN_W, gs::SCREEN_H);
    city(ground);
    gs::bitmapToPlane(tiles, vdp.B, 0, 0, ground, PAL_CITY);
    for (int i = 0; i < 2; i++) {
        art.hero[i] = gs::uploadMipped(vdp, courier(i, true));
        art.bare[i] = gs::uploadMipped(vdp, courier(i, false));
    }
    art.satchel = gs::uploadMipped(vdp, satchelArt());
    art.sedan = gs::uploadMipped(vdp, sedanArt());
    art.van = gs::uploadMipped(vdp, vanArt());
    art.bus = gs::uploadMipped(vdp, busArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.puff = gs::uploadMipped(vdp, puffArt());
}

}  // namespace pouch

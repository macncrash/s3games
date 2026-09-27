#include "game/art.h"

#include <initializer_list>

namespace sbann {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap runner(int frame) {
    Bitmap b(30, 42);
    b.ellipse(15, 7, 4.4f, 4.2f, 1);
    b.rect(11, 3, 8, 3, 7);
    b.rect(11, 5, 8, 2, 8);
    b.rect(18, 7, 2, 2, 2);
    b.rect(9, 12, 12, 14, 3);
    b.rect(9, 12, 3, 14, 4);
    b.rect(12, 22, 6, 2, 5);
    b.rect(18, 16, 4, 5, 6);
    b.ellipse(22, 18, 2.2f, 2.2f, 9);
    if (frame == 0) {
        b.rect(10, 26, 4, 11, 4);
        b.rect(16, 26, 4, 9, 3);
        b.rect(9, 36, 6, 3, 6);
        b.rect(15, 34, 6, 3, 6);
    } else {
        b.rect(10, 26, 4, 9, 3);
        b.rect(16, 26, 4, 11, 4);
        b.rect(9, 34, 6, 3, 6);
        b.rect(15, 36, 6, 3, 6);
    }
    b.outline(15, false);
    return b;
}

Bitmap airRunner() {
    Bitmap b(30, 40);
    b.ellipse(16, 8, 4.4f, 4.2f, 1);
    b.rect(12, 4, 8, 3, 7);
    b.rect(12, 6, 8, 2, 8);
    b.rect(11, 13, 12, 12, 3);
    b.rect(11, 13, 3, 12, 4);
    b.rect(14, 22, 6, 2, 5);
    b.rect(20, 16, 4, 5, 6);
    b.ellipse(24, 18, 2.1f, 2.1f, 9);
    b.rect(8, 24, 5, 4, 4);
    b.rect(17, 24, 5, 4, 3);
    b.rect(7, 27, 6, 3, 6);
    b.rect(17, 27, 6, 3, 6);
    b.outline(15, false);
    return b;
}

Bitmap guardArt(int frame) {
    Bitmap b(34, 36);
    b.ellipse(14, 7, 4.6f, 4.4f, 1);
    b.rect(10, 3, 9, 4, 2);
    b.rect(9, 11, 12, 13, 1);
    b.rect(9, 11, 3, 13, 2);
    b.rect(11, 21, 7, 2, 3);
    b.rect(10, 24, 4, 8, 2);
    b.rect(16, 24, 4, 8, 6);
    b.rect(9, 31, 6, 3, 6);
    b.rect(15, 31, 6, 3, 6);
    float py = frame ? 12.f : 14.f;
    b.line(20, py, 32, py - 2.f, 4, 1.6f);
    b.poly({{31, py - 5}, {33, py - 1}, {28, py + 1}}, 5);
    b.rect(18, 8, 2, 2, 8);
    b.outline(15, false);
    return b;
}

Bitmap bannerArt(int frame) {
    Bitmap b(36, 46);
    b.rect(8, 4, 3, 40, 5);
    b.rect(8, 2, 3, 3, 6);
    float fy = frame ? 6.f : 9.f;
    b.poly({{11, fy}, {32, fy + 7}, {11, fy + 18}}, 1);
    b.poly({{11, fy + 2}, {24, fy + 8}, {11, fy + 14}}, 2);
    b.rect(14, fy + 7, 8, 3, 3);
    b.rect(17, fy + 5, 3, 7, 4);
    b.outline(15, false);
    return b;
}

Bitmap postArt() {
    Bitmap b(14, 28);
    b.rect(6, 2, 3, 24, 2);
    b.rect(3, 2, 8, 3, 1);
    b.rect(4, 22, 6, 4, 3);
    b.rect(5, 8, 4, 2, 4);
    b.outline(15, false);
    return b;
}

Bitmap plankArt() {
    Bitmap b(22, 12);
    b.rect(0, 1, 22, 9, 2);
    b.rect(0, 1, 22, 3, 1);
    b.rect(0, 8, 22, 3, 3);
    for (int x = 3; x < 20; x += 6) b.rect(float(x), 4, 1, 5, 4);
    b.rect(2, 6, 2, 2, 5);
    b.rect(17, 5, 2, 2, 5);
    b.outline(15, false);
    return b;
}

Bitmap lipArt() {
    Bitmap b(18, 14);
    b.poly({{0, 4}, {14, 2}, {16, 12}, {0, 10}}, 2);
    b.poly({{0, 4}, {12, 3}, {10, 6}, {0, 7}}, 1);
    b.line(4, 3, 8, 12, 3, 1.2f);
    b.rect(12, 6, 3, 2, 4);
    b.outline(15, false);
    return b;
}

Bitmap linkArt() {
    Bitmap b(8, 8);
    b.ellipse(4, 4, 3.1f, 2.4f, 1);
    b.ellipse(4, 4, 1.5f, 1.0f, 0);
    b.rect(3, 1, 2, 2, 2);
    return b;
}

Bitmap hookArt() {
    Bitmap b(16, 20);
    b.rect(7, 0, 2, 8, 2);
    b.ellipse(8, 12, 5.5f, 5.5f, 1);
    b.ellipse(8, 12, 2.6f, 2.6f, 0);
    b.rect(11, 12, 4, 2, 3);
    b.rect(6, 2, 2, 2, 4);
    b.outline(15, false);
    return b;
}

Bitmap towerArt() {
    Bitmap b(44, 120);
    b.poly({{8, 10}, {36, 10}, {40, 116}, {4, 116}}, 2);
    b.rect(10, 14, 24, 96, 1);
    b.rect(6, 8, 32, 6, 3);
    b.rect(12, 4, 20, 6, 5);
    b.rect(18, 0, 8, 6, 6);
    for (int y = 24; y < 108; y += 16) {
        b.rect(8, float(y), 28, 3, 3);
        b.rect(18, float(y + 4), 6, 8, 4);
    }
    b.rect(19, 28, 4, 7, 7);
    b.rect(19, 52, 4, 7, 4);
    b.rect(19, 78, 4, 7, 4);
    b.poly({{4, 116}, {12, 96}, {16, 116}}, 3);
    b.poly({{40, 116}, {32, 96}, {28, 116}}, 3);
    b.outline(15, false);
    return b;
}

Bitmap rockArt() {
    Bitmap b(80, 90);
    b.poly({{0, 28}, {16, 12}, {40, 22}, {62, 8}, {80, 24}, {80, 89}, {0, 89}}, 2);
    b.poly({{0, 40}, {24, 26}, {52, 34}, {80, 20}, {80, 70}, {0, 78}}, 1);
    b.rect(0, 78, 80, 12, 3);
    for (int y = 36; y < 80; y += 14) b.line(6, float(y), 70, float(y + 6), 4, 1.2f);
    b.rect(18, 18, 3, 10, 5);
    b.rect(48, 14, 3, 12, 5);
    b.outline(15, false);
    return b;
}

Bitmap lampArt() {
    Bitmap b(12, 16);
    b.rect(5, 0, 2, 4, 2);
    b.poly({{2, 6}, {10, 6}, {8, 13}, {4, 13}}, 1);
    b.rect(4, 7, 4, 4, 3);
    b.rect(5, 13, 2, 3, 2);
    return b;
}

Bitmap moonArt() {
    Bitmap b(26, 26);
    b.ellipse(13, 13, 11, 11, 1);
    b.ellipse(16, 11, 8, 8, 0);
    b.ellipse(10, 12, 2, 2, 2);
    b.ellipse(12, 17, 1.2f, 1.2f, 2);
    return b;
}

Bitmap cloudArt() {
    Bitmap b(52, 18);
    b.ellipse(16, 11, 12, 6, 1);
    b.ellipse(30, 9, 14, 7, 1);
    b.ellipse(42, 11, 8, 5, 2);
    return b;
}

Bitmap batArt(int frame) {
    Bitmap b(18, 8);
    if (frame == 0) {
        b.line(1, 6, 9, 3, 1, 1.3f);
        b.line(9, 3, 17, 6, 1, 1.3f);
    } else {
        b.line(1, 2, 9, 4, 1, 1.3f);
        b.line(9, 4, 17, 2, 1, 1.3f);
    }
    b.rect(8, 3, 2, 2, 2);
    return b;
}

Bitmap starArt() {
    Bitmap b(5, 5);
    b.rect(2, 0, 1, 5, 1);
    b.rect(0, 2, 5, 1, 1);
    b.rect(2, 2, 1, 1, 2);
    return b;
}

Bitmap splashArt() {
    Bitmap b(18, 10);
    b.ellipse(9, 6, 8, 3, 1);
    b.ellipse(5, 5, 2, 2, 2);
    b.ellipse(13, 4, 2, 2, 2);
    return b;
}

Bitmap chevArt() {
    Bitmap b(12, 10);
    b.poly({{6, 1}, {11, 9}, {6, 6}, {1, 9}}, 1);
    b.poly({{6, 3}, {8, 8}, {6, 6}, {4, 8}}, 2);
    return b;
}

Bitmap windArt() {
    Bitmap b(22, 4);
    b.rect(0, 1, 22, 2, 1);
    b.rect(4, 0, 8, 1, 2);
    return b;
}

Bitmap dustArt() {
    Bitmap b(10, 8);
    b.ellipse(5, 4, 4, 3, 1);
    b.ellipse(4, 3, 2, 1.4f, 2);
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

gs::Mipped word(gs::VDP& vdp, const char* s) {
    gs::TextStyle st;
    st.scale = 5;
    st.color = 1;
    st.outline = 2;
    st.spacing = 1;
    return gs::uploadMipped(vdp, gs::textBitmap(s, st));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(15, 15, 14);
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    const uint16_t out = gs::rgb4(1, 1, 2);

    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(8, 9, 11), gs::rgb4(14, 11, 6), gs::rgb4(12, 4, 3),
                          gs::rgb4(6, 13, 7), gs::rgb4(6, 8, 12), gs::rgb4(5, 5, 7), gs::rgb4(10, 8, 5),
                          0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(13, 9, 5), gs::rgb4(9, 6, 3), gs::rgb4(4, 3, 2), gs::rgb4(3, 3, 4),
                           gs::rgb4(12, 11, 8), gs::rgb4(6, 5, 4), 0, 0, 0, 0, 0, 0, 0, 0, out});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(9, 10, 12), gs::rgb4(5, 6, 8), gs::rgb4(3, 3, 5), gs::rgb4(2, 2, 3),
                            gs::rgb4(7, 8, 9), gs::rgb4(12, 10, 6), gs::rgb4(15, 13, 6), gs::rgb4(14, 8, 3),
                            0, 0, 0, 0, 0, 0, out});
    setPal(vdp, PAL_YOU, {0, gs::rgb4(14, 11, 8), gs::rgb4(8, 6, 5), gs::rgb4(4, 5, 11), gs::rgb4(2, 3, 7),
                          gs::rgb4(12, 9, 3), gs::rgb4(3, 2, 2), gs::rgb4(2, 2, 4), gs::rgb4(1, 1, 3),
                          gs::rgb4(15, 12, 5), 0, 0, 0, 0, 0, out});
    setPal(vdp, PAL_FOE, {0, gs::rgb4(6, 6, 7), gs::rgb4(3, 3, 4), gs::rgb4(8, 4, 2), gs::rgb4(11, 12, 13),
                          gs::rgb4(14, 13, 10), gs::rgb4(2, 2, 2), gs::rgb4(4, 3, 3), gs::rgb4(12, 3, 2),
                          0, 0, 0, 0, 0, 0, out});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(13, 2, 3), gs::rgb4(8, 1, 2), gs::rgb4(14, 11, 3), gs::rgb4(15, 14, 8),
                             gs::rgb4(12, 10, 6), gs::rgb4(6, 5, 3), 0, 0, 0, 0, 0, 0, 0, 0, out});
    setPal(vdp, PAL_IRON, {0, gs::rgb4(12, 13, 14), gs::rgb4(7, 8, 9), gs::rgb4(8, 5, 3), gs::rgb4(14, 12, 6),
                           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, out});
    setPal(vdp, PAL_FX, {0, gs::rgb4(14, 15, 15), gs::rgb4(10, 12, 14), gs::rgb4(15, 13, 6), gs::rgb4(8, 7, 5),
                         gs::rgb4(12, 8, 4), gs::rgb4(6, 8, 10), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_ROCK, {0, gs::rgb4(7, 8, 9), gs::rgb4(4, 5, 6), gs::rgb4(2, 2, 3), gs::rgb4(3, 4, 4),
                           gs::rgb4(5, 6, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, out});

    auto textPal = [&](int pal, uint16_t c) {
        setPal(vdp, pal, {0, c, gs::rgb4(2, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    };
    textPal(PAL_AMBER, gs::rgb4(15, 12, 4));
    textPal(PAL_ALERT, gs::rgb4(15, 4, 3));
    textPal(PAL_GOOD, gs::rgb4(8, 15, 7));
    textPal(PAL_TITLE, gs::rgb4(15, 12, 5));

    art.runner[0] = gs::uploadMipped(vdp, runner(0));
    art.runner[1] = gs::uploadMipped(vdp, runner(1));
    art.air = gs::uploadMipped(vdp, airRunner());
    art.guard[0] = gs::uploadMipped(vdp, guardArt(0));
    art.guard[1] = gs::uploadMipped(vdp, guardArt(1));
    art.banner[0] = gs::uploadMipped(vdp, bannerArt(0));
    art.banner[1] = gs::uploadMipped(vdp, bannerArt(1));
    art.post = gs::uploadMipped(vdp, postArt());
    art.plank = gs::uploadMipped(vdp, plankArt());
    art.lip = gs::uploadMipped(vdp, lipArt());
    art.link = gs::uploadMipped(vdp, linkArt());
    art.hook = gs::uploadMipped(vdp, hookArt());
    art.tower = gs::uploadMipped(vdp, towerArt());
    art.rock = gs::uploadMipped(vdp, rockArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.bat[0] = gs::uploadMipped(vdp, batArt(0));
    art.bat[1] = gs::uploadMipped(vdp, batArt(1));
    art.star = gs::uploadMipped(vdp, starArt());
    art.splash = gs::uploadMipped(vdp, splashArt());
    art.chev = gs::uploadMipped(vdp, chevArt());
    art.wind = gs::uploadMipped(vdp, windArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.word[0] = word(vdp, "SPAN");
    art.word[1] = word(vdp, "BACK");
    art.word[2] = word(vdp, "OVER");
    art.word[3] = word(vdp, "WAIT");
    loadFont(vdp, art);
}

}  // namespace sbann

#include "game/art.h"

#include <cstring>
#include <initializer_list>

namespace siege {
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

Bitmap word(const char* s, int scale) {
    return gs::textBitmap(s, {scale, 1, 2, 15, 1});
}

// The player: a faced block of curtain wall.
Bitmap youArt(bool brace) {
    const int w = brace ? 78 : 52;
    const int h = brace ? 64 : 76;
    Bitmap b(w, h);
    int x0 = 4, x1 = w - 4;
    int top = brace ? 14 : 16;
    b.rect(float(x0), float(top), float(x1 - x0), float(h - top - 4), 3);
    b.rect(float(x0 + 3), float(top + 3), float(x1 - x0 - 6), float(h - top - 10), 4);
    int courses = brace ? 3 : 4;
    for (int i = 0; i < courses; i++) {
        int y = top + 8 + i * ((h - top - 16) / courses);
        b.rect(float(x0 + 2), float(y), float(x1 - x0 - 4), 2, 2);
        b.rect(float(x0 + 2 + ((i & 1) ? 8 : 16)), float(y - 3), 2, 6, 5);
    }
    // Crenels.
    int tooth = brace ? 16 : 12;
    b.rect(float(x0), 4, float(tooth), float(top), 3);
    b.rect(float(x1 - tooth), 4, float(tooth), float(top), 3);
    if (brace) b.rect(float(w / 2 - 8), 2, 16, float(top + 2), 4);
    b.rect(float(x0 + 2), 6, float(tooth - 4), 5, 4);
    b.rect(float(x1 - tooth + 2), 6, float(tooth - 4), 5, 4);
    // A face in the masonry, so the wall is someone.
    int cx = w / 2;
    int ey = brace ? 28 : 34;
    b.rect(float(cx - 12), float(ey), 5, brace ? 6 : 10, 8);
    b.rect(float(cx + 7), float(ey), 5, brace ? 6 : 10, 8);
    b.rect(float(cx - 11), float(ey + 1), 2, 2, 7);
    b.rect(float(cx + 8), float(ey + 1), 2, 2, 7);
    b.poly({{float(cx), float(ey + 22)},
            {float(cx - 8), float(ey + 12)},
            {float(cx - 4), float(ey + 12)},
            {float(cx), float(ey + 17)},
            {float(cx + 4), float(ey + 12)},
            {float(cx + 8), float(ey + 12)}},
           7);
    // Murder-hole the stone falls from.
    b.rect(float(cx - 3), float(h - 12), 6, 6, 8);
    b.rect(float(cx - 1), float(h - 10), 2, 3, 5);
    if (brace) {
        b.rect(6, float(h / 2), float(w - 12), 4, 7);
        b.rect(8, float(h / 2 + 1), float(w - 16), 2, 4);
    } else {
        b.rect(float(x0 + 4), float(h - 18), 7, 3, 6);
    }
    b.outline(1, false);
    return b;
}

Bitmap gateArt(int kind) {
    Bitmap b(72, 80);
    b.rect(4, 10, 64, 66, 2);
    b.rect(10, 16, 52, 58, 3);
    b.poly({{36, 8}, {14, 28}, {58, 28}}, 2);
    b.rect(18, 28, 36, 42, 4);
    b.rect(20, 30, 32, 38, 5);
    b.rect(18, 36, 36, 3, 6);
    b.rect(18, 48, 36, 3, 6);
    b.rect(34, 30, 4, 38, 7);
    b.ellipse(36, 52, 3, 3, 7);
    b.rect(8, 18, 6, 48, 2);
    b.rect(58, 18, 6, 48, 2);
    b.rect(6, 62, 60, 8, 2);
    if (kind >= 1) {
        b.line(22, 30, 40, 68, 8, 1.4f);
        b.line(48, 26, 30, 58, 8, 1.2f);
        b.line(28, 44, 46, 40, 8, 1.0f);
    }
    if (kind >= 2) {
        b.rect(26, 38, 18, 22, 9);
        b.poly({{30, 36}, {44, 40}, {40, 58}, {24, 54}}, 8);
        b.rect(30, 44, 8, 10, 9);
    }
    b.outline(1, false);
    return b;
}

Bitmap ramArt(bool heavy, int phase) {
    Bitmap b(heavy ? 58 : 48, 52);
    int w = b.w;
    int mid = w / 2;
    int hide = heavy ? 2 : 2;
    int hideD = heavy ? 3 : 3;
    int wood = heavy ? 5 : 4;
    int woodD = heavy ? 3 : 5;
    int head = heavy ? 4 : 6;
    // Canopy.
    b.poly({{float(mid), 16}, {6, 28}, {float(w - 6), 28}}, hide);
    b.poly({{float(mid), 18}, {12, 27}, {float(w - 12), 27}}, hideD);
    b.rect(8, 28, float(w - 16), 12, wood);
    b.rect(10, 30, float(w - 20), 8, woodD);
    // Beam and head toward the wall (top of the sprite).
    b.rect(float(mid - 3), 4, 6, 26, wood);
    if (heavy) {
        b.poly({{float(mid), 0}, {float(mid - 10), 12}, {float(mid + 10), 12}}, head);
        b.poly({{float(mid - 12), 8}, {float(mid - 4), 4}, {float(mid - 6), 12}}, 4);
        b.poly({{float(mid + 12), 8}, {float(mid + 4), 4}, {float(mid + 6), 12}}, 4);
        b.rect(float(mid - 8), 10, 16, 3, 7);
        b.rect(4, 26, float(w - 8), 3, 2);
    } else {
        b.ellipse(float(mid), 8, 8, 7, head);
        b.rect(float(mid - 2), 2, 3, 6, 7);
        b.rect(6, 27, float(w - 12), 2, 7);
    }
    // Crew slit.
    b.rect(float(mid - 6), 32, 12, 5, 9);
    b.rect(float(mid - 4), 33, 2, 2, 1);
    b.rect(float(mid + 2), 33, 2, 2, 1);
    int wx = phase ? 3 : 0;
    b.ellipse(float(14 + wx), 44, 6, 6, 8);
    b.ellipse(float(w - 14 - wx), 44, 6, 6, 8);
    b.ellipse(float(14 + wx), 44, 2, 2, 7);
    b.ellipse(float(w - 14 - wx), 44, 2, 2, 7);
    b.outline(1, false);
    return b;
}

Bitmap merlonArt() {
    Bitmap b(30, 40);
    b.rect(4, 8, 22, 28, 3);
    b.rect(6, 10, 18, 10, 4);
    b.rect(4, 20, 22, 2, 2);
    b.rect(4, 28, 22, 2, 2);
    b.rect(14, 12, 2, 14, 5);
    b.rect(8, 32, 5, 3, 6);
    b.outline(1, false);
    return b;
}

Bitmap courseArt() {
    Bitmap b(68, 22);
    b.rect(0, 2, 68, 18, 3);
    b.rect(2, 4, 30, 6, 4);
    b.rect(36, 4, 28, 6, 4);
    b.rect(2, 12, 20, 6, 5);
    b.rect(26, 12, 18, 6, 4);
    b.rect(48, 12, 16, 6, 5);
    b.rect(0, 10, 68, 2, 2);
    return b;
}

Bitmap towerArt() {
    Bitmap b(40, 96);
    b.rect(6, 20, 28, 72, 3);
    b.rect(8, 24, 24, 64, 4);
    for (int i = 0; i < 5; i++) b.rect(8, float(32 + i * 12), 24, 2, 2);
    b.rect(4, 8, 10, 16, 3);
    b.rect(26, 8, 10, 16, 3);
    b.rect(16, 4, 8, 18, 5);
    b.rect(14, 58, 12, 18, 8);
    b.rect(18, 64, 4, 8, 1);
    b.rect(10, 86, 8, 4, 6);
    b.outline(1, false);
    return b;
}

Bitmap bannerArt() {
    Bitmap b(18, 36);
    b.rect(8, 0, 2, 34, 5);
    b.poly({{10, 4}, {16, 10}, {10, 16}}, 2);
    b.poly({{10, 6}, {14, 10}, {10, 14}}, 4);
    b.rect(4, 14, 8, 12, 2);
    b.rect(5, 20, 6, 2, 4);
    b.outline(1, false);
    return b;
}

Bitmap torchArt(int frame) {
    Bitmap b(14, 28);
    b.rect(6, 12, 3, 14, 5);
    b.rect(5, 24, 5, 3, 1);
    int dy = frame ? 1 : 0;
    b.ellipse(7, float(8 + dy), 4, 6, 4);
    b.ellipse(7, float(9 + dy), 2.4f, 4, 3);
    b.ellipse(7, float(10 + dy), 1.2f, 2, 2);
    b.outline(1, false);
    return b;
}

Bitmap rockArt() {
    Bitmap b(16, 16);
    b.poly({{8, 1}, {14, 6}, {12, 14}, {3, 13}, {2, 6}}, 2);
    b.poly({{8, 4}, {11, 7}, {9, 12}, {5, 10}, {5, 6}}, 3);
    b.rect(4, 8, 3, 2, 4);
    b.outline(1, false);
    return b;
}

Bitmap puffArt() {
    Bitmap b(16, 16);
    b.ellipse(8, 8, 6, 5, 2);
    b.ellipse(5, 8, 3, 2.4f, 3);
    b.ellipse(11, 7, 2.2f, 2, 3);
    return b;
}

Bitmap shadowArt() {
    Bitmap b(20, 8);
    b.ellipse(10, 4, 9, 3, 1);
    return b;
}

Bitmap moonArt() {
    Bitmap b(28, 28);
    b.ellipse(14, 14, 11, 11, 2);
    b.ellipse(17, 12, 8, 8, 0);
    b.ellipse(11, 16, 2, 2, 3);
    b.ellipse(15, 19, 1.2f, 1.2f, 4);
    b.outline(1, false);
    return b;
}

Bitmap starArt() {
    Bitmap b(7, 7);
    b.rect(3, 0, 1, 7, 2);
    b.rect(0, 3, 7, 1, 2);
    b.set(2, 2, 3);
    b.set(4, 4, 3);
    return b;
}

Bitmap tuftArt() {
    Bitmap b(18, 12);
    b.line(3, 10, 5, 2, 2, 1.2f);
    b.line(8, 10, 7, 1, 3, 1.2f);
    b.line(12, 10, 14, 3, 2, 1.2f);
    b.rect(2, 9, 14, 2, 4);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_HUD, gs::rgb4(15, 15, 14), gs::rgb4(3, 3, 4), gs::rgb4(1, 1, 2));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 13, 4), gs::rgb4(6, 3, 1), gs::rgb4(2, 1, 0));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 4, 3), gs::rgb4(6, 1, 1), gs::rgb4(2, 0, 0));
    textPal(vdp, PAL_DIM, gs::rgb4(9, 8, 8), gs::rgb4(3, 3, 4), gs::rgb4(1, 1, 2));

    // 1 outline, 2 mortar, 3 stone, 4 light, 5 dark, 6 moss, 7 gold, 8 void.
    pal(vdp, PAL_WALL,
        {0, gs::rgb4(1, 1, 2), gs::rgb4(6, 6, 6), gs::rgb4(9, 9, 8), gs::rgb4(12, 12, 10), gs::rgb4(4, 4, 5),
         gs::rgb4(3, 6, 3), gs::rgb4(14, 11, 3), gs::rgb4(1, 1, 1), gs::rgb4(10, 8, 6)});
    // Gate: 2 stone, 3 shade, 4 wood, 5 wood light, 6 iron, 7 band, 8 crack, 9 hole.
    pal(vdp, PAL_GATE,
        {0, gs::rgb4(1, 1, 2), gs::rgb4(7, 7, 6), gs::rgb4(4, 4, 4), gs::rgb4(7, 4, 2), gs::rgb4(10, 7, 3),
         gs::rgb4(8, 8, 9), gs::rgb4(12, 11, 6), gs::rgb4(8, 2, 1), gs::rgb4(1, 0, 0)});
    // Timber ram. Iron index 6 is the head; 9 is the crew slit.
    pal(vdp, PAL_RAM,
        {0, gs::rgb4(1, 1, 1), gs::rgb4(8, 5, 3), gs::rgb4(5, 3, 2), gs::rgb4(10, 7, 3), gs::rgb4(6, 4, 2),
         gs::rgb4(11, 11, 12), gs::rgb4(13, 11, 6), gs::rgb4(2, 2, 2), gs::rgb4(12, 8, 6)});
    // Iron ram reuses the same drawing with a colder palette. Index 4 reads as the spiked head.
    pal(vdp, PAL_IRON,
        {0, gs::rgb4(1, 1, 2), gs::rgb4(5, 5, 6), gs::rgb4(3, 3, 4), gs::rgb4(12, 12, 13), gs::rgb4(6, 5, 4),
         gs::rgb4(4, 4, 5), gs::rgb4(14, 13, 8), gs::rgb4(2, 2, 3), gs::rgb4(9, 7, 6)});
    pal(vdp, PAL_ROCK,
        {0, gs::rgb4(1, 1, 1), gs::rgb4(7, 7, 6), gs::rgb4(11, 11, 9), gs::rgb4(4, 4, 4)});
    pal(vdp, PAL_DUST, {0, gs::rgb4(2, 2, 2), gs::rgb4(8, 7, 5), gs::rgb4(12, 11, 8)});
    pal(vdp, PAL_BANNER,
        {0, gs::rgb4(2, 1, 1), gs::rgb4(12, 2, 2), gs::rgb4(7, 1, 1), gs::rgb4(14, 12, 4), gs::rgb4(6, 5, 3)});
    pal(vdp, PAL_FIRE,
        {0, gs::rgb4(2, 1, 1), gs::rgb4(15, 14, 6), gs::rgb4(15, 8, 2), gs::rgb4(12, 3, 1), gs::rgb4(5, 3, 2)});
    pal(vdp, PAL_GRASS, {0, gs::rgb4(1, 2, 1), gs::rgb4(4, 8, 3), gs::rgb4(2, 5, 2), gs::rgb4(5, 4, 2)});
    pal(vdp, PAL_MOON, {0, gs::rgb4(3, 3, 5), gs::rgb4(14, 14, 11), gs::rgb4(10, 10, 8), gs::rgb4(8, 8, 6)});

    loadFont(vdp, art);
    art.title = gs::uploadMipped(vdp, word("S3 SIEGE", 3));
    art.sub = gs::uploadMipped(vdp, word("YOU ARE THE WALL", 2));
    art.hold = gs::uploadMipped(vdp, word("THE GATE HOLDS", 2));
    art.broke = gs::uploadMipped(vdp, word("THE GATE BREAKS", 2));
    art.through = gs::uploadMipped(vdp, word("A RAM GOT THROUGH", 2));
    art.pause = gs::uploadMipped(vdp, word("PAUSE", 3));
    art.you = gs::uploadMipped(vdp, youArt(false));
    art.youBrace = gs::uploadMipped(vdp, youArt(true));
    for (int i = 0; i < 3; i++) art.gate[i] = gs::uploadMipped(vdp, gateArt(i));
    for (int i = 0; i < 2; i++) {
        art.ram[i] = gs::uploadMipped(vdp, ramArt(false, i));
        art.iron[i] = gs::uploadMipped(vdp, ramArt(true, i));
        art.torch[i] = gs::uploadMipped(vdp, torchArt(i));
    }
    art.rock = gs::uploadMipped(vdp, rockArt());
    art.puff = gs::uploadMipped(vdp, puffArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.merlon = gs::uploadMipped(vdp, merlonArt());
    art.course = gs::uploadMipped(vdp, courseArt());
    art.tower = gs::uploadMipped(vdp, towerArt());
    art.banner = gs::uploadMipped(vdp, bannerArt());
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.star = gs::uploadMipped(vdp, starArt());
    art.tuft = gs::uploadMipped(vdp, tuftArt());
}

}  // namespace siege

#include "game/art.h"

#include <cstdint>

namespace sub {
namespace {

void textPal(gs::VDP& vdp, int pal, uint16_t fg) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, fg);
    vdp.setColor(pal * 16 + 2, gs::rgb4(0, 0, 1));
}

void colors(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    vdp.setColor(pal * 16 + 0, 0);
}

uint32_t hash3(int x, int y, int s) {
    uint32_t h = uint32_t(x) * 374761393u + uint32_t(y) * 668265263u + uint32_t(s) * 1440662683u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

void bakeFont(gs::VDP& vdp, int* font) {
    gs::TileAlloc tiles(vdp, 1);
    for (int c = 0; c < 96; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c + 32));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) px[y * 8 + (x + 1)] = 1;
        font[c] = tiles.shared(px);
    }
}

gs::Mipped bakeText(gs::VDP& vdp, const char* s, int scale) {
    gs::TextStyle st;
    st.scale = scale;
    st.color = 1;
    st.outline = 2;
    st.shadow = 0;
    st.spacing = 1;
    return gs::uploadMipped(vdp, gs::textBitmap(s, st));
}

gs::Bitmap rockStrip(int seed, bool ceiling) {
    gs::Bitmap b(8, 48);
    for (int y = 0; y < 48; y++) {
        for (int x = 0; x < 8; x++) {
            uint32_t h = hash3(x, y, seed + 3);
            int c = 1 + int(h % 3);
            if ((h >> 8) % 9 == 0) c = 7;
            if ((h >> 12) % 13 == 0) c = 6;
            if (x == (seed % 5) + 1 && (y & 3) != 0) c = 6;
            b.set(x, y, c);
        }
    }
    if (!ceiling) {
        for (int y = 0; y < 4; y++)
            for (int x = 0; x < 8; x++) b.set(x, y, 4 + ((x + y + seed) & 1));
        for (int x = 0; x < 8; x++) {
            if (hash3(x, seed, 1) % 3 == 0) b.set(x, 0, 0);
            if (hash3(x, seed, 2) % 5 == 0) b.set(x, 1, 0);
        }
        b.set(3, 2, 5);
        b.set(4, 2, 5);
    } else {
        for (int y = 44; y < 48; y++)
            for (int x = 0; x < 8; x++) b.set(x, y, 4 + ((x + y + seed) & 1));
        for (int x = 0; x < 8; x++) {
            if (hash3(x, seed, 4) % 3 == 0) b.set(x, 47, 0);
            if (hash3(x, seed, 5) % 5 == 0) b.set(x, 46, 0);
        }
    }
    return b;
}

gs::Bitmap paintSub() {
    gs::Bitmap b(96, 36);
    b.ellipse(48, 22, 34, 10, 1);
    b.poly({{68, 14}, {92, 21}, {68, 30}}, 1);
    b.poly({{22, 16}, {36, 12}, {36, 30}, {20, 28}}, 2);
    b.ellipse(46, 19, 22, 5, 3);
    b.rect(36, 8, 16, 12, 2);
    b.poly({{36, 8}, {44, 3}, {52, 8}}, 1);
    b.rect(42, 3, 3, 5, 7);
    b.rect(28, 30, 38, 4, 6);
    b.ellipse(40, 22, 4, 4, 9);
    b.ellipse(40, 22, 2, 2, 4);
    b.ellipse(56, 21, 4, 4, 9);
    b.ellipse(56, 21, 2, 2, 4);
    b.ellipse(84, 20, 4, 3, 5);
    b.ellipse(86, 20, 2, 2, 1);
    b.rect(18, 20, 6, 3, 8);
    b.poly({{16, 16}, {22, 14}, {22, 28}, {16, 26}}, 2);
    b.set(48, 10, 5);
    b.set(49, 10, 5);
    b.outline(7, false);
    return b.cropToContent(0);
}

gs::Bitmap paintProp(bool spin) {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 2, 2, 8);
    if (!spin) b.ellipse(8, 8, 7, 2, 2);
    else b.ellipse(8, 8, 2, 7, 2);
    b.ellipse(8, 8, 1, 1, 5);
    return b;
}

gs::Bitmap paintFish(bool fat) {
    gs::Bitmap b(30, 16);
    float ry = fat ? 5.5f : 4.f;
    b.poly({{3, 8}, {16, 8 - ry}, {24, 8}, {16, 8 + ry}}, 1);
    b.poly({{14, 8 - ry * 0.4f}, {28, 2}, {18, 8}}, 4);
    b.poly({{14, 8 + ry * 0.4f}, {28, 14}, {18, 8}}, 4);
    b.ellipse(10, 7, 2, 2, 2);
    b.ellipse(9, 7, 1, 1, 3);
    b.outline(4, false);
    return b.cropToContent(0);
}

gs::Bitmap paintGate() {
    gs::Bitmap b(40, 88);
    b.rect(0, 0, 8, 88, 2);
    b.rect(32, 0, 8, 88, 2);
    b.rect(8, 6, 24, 76, 1);
    b.rect(8, 6, 24, 5, 3);
    b.rect(8, 77, 24, 5, 3);
    for (int i = 0; i < 4; i++) {
        int y = 18 + i * 15;
        b.poly({{14, float(y)}, {26, float(y + 6)}, {14, float(y + 12)}, {18, float(y + 6)}}, 5);
    }
    b.ellipse(4, 14, 3, 3, 3);
    b.ellipse(36, 14, 3, 3, 3);
    b.ellipse(4, 74, 3, 3, 3);
    b.ellipse(36, 74, 3, 3, 3);
    b.rect(2, 0, 36, 3, 4);
    b.rect(2, 85, 36, 3, 4);
    gs::TextStyle st;
    st.scale = 1;
    st.color = 5;
    st.outline = 0;
    st.spacing = 1;
    gs::Bitmap word = gs::textBitmap("OUT", st);
    b.blit(word, 20 - word.w / 2, 40);
    return b;
}

gs::Bitmap paintGlow() {
    gs::Bitmap b(10, 8);
    b.ellipse(4, 4, 4, 3, 2);
    b.ellipse(3, 4, 2, 2, 1);
    return b;
}

gs::Bitmap paintRing() {
    gs::Bitmap b(32, 32);
    b.ellipse(16, 16, 15, 15, 1);
    b.ellipse(16, 16, 11, 11, 0);
    return b;
}

gs::Bitmap paintBulb() {
    gs::Bitmap b(8, 8);
    b.rect(3, 0, 2, 3, 3);
    b.ellipse(4, 5, 3, 3, 1);
    b.ellipse(4, 5, 1, 1, 2);
    return b;
}

gs::Bitmap paintMote() {
    gs::Bitmap b(4, 4);
    b.ellipse(2, 2, 1.6f, 1.6f, 1);
    return b;
}

gs::Bitmap paintPuff() {
    gs::Bitmap b(12, 12);
    b.ellipse(6, 6, 5, 4, 1);
    b.ellipse(4, 5, 2, 2, 2);
    b.ellipse(8, 7, 2, 2, 3);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_HUD, gs::rgb4(15, 15, 15));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 5, 4));
    textPal(vdp, PAL_WARN, gs::rgb4(15, 12, 4));
    textPal(vdp, PAL_WIN, gs::rgb4(8, 15, 12));
    textPal(vdp, PAL_DIM, gs::rgb4(7, 11, 13));

    colors(vdp, PAL_SUB,
           {0, gs::rgb4(3, 10, 11), gs::rgb4(1, 5, 6), gs::rgb4(8, 14, 13), gs::rgb4(12, 15, 13), gs::rgb4(15, 14, 6),
            gs::rgb4(12, 5, 2), gs::rgb4(0, 2, 3), gs::rgb4(7, 8, 8), gs::rgb4(2, 6, 8)});
    colors(vdp, PAL_HIT,
           {0, gs::rgb4(12, 4, 4), gs::rgb4(7, 1, 2), gs::rgb4(15, 8, 6), gs::rgb4(14, 12, 8), gs::rgb4(15, 13, 5),
            gs::rgb4(14, 3, 1), gs::rgb4(5, 0, 1), gs::rgb4(10, 5, 4), gs::rgb4(8, 2, 3)});
    colors(vdp, PAL_ROCK,
           {0, gs::rgb4(3, 2, 2), gs::rgb4(5, 4, 3), gs::rgb4(8, 6, 4), gs::rgb4(11, 9, 6), gs::rgb4(13, 11, 7),
            gs::rgb4(2, 4, 5), gs::rgb4(9, 8, 6)});
    colors(vdp, PAL_CEIL,
           {0, gs::rgb4(1, 2, 3), gs::rgb4(2, 3, 4), gs::rgb4(3, 5, 6), gs::rgb4(6, 8, 8), gs::rgb4(4, 7, 7),
            gs::rgb4(2, 4, 5), gs::rgb4(1, 1, 2)});
    colors(vdp, PAL_FISH,
           {0, gs::rgb4(5, 8, 9), gs::rgb4(9, 12, 11), gs::rgb4(14, 14, 8), gs::rgb4(3, 5, 6)});
    colors(vdp, PAL_LAMP, {0, gs::rgb4(15, 15, 12), gs::rgb4(12, 13, 7), gs::rgb4(8, 10, 6)});
    colors(vdp, PAL_GATE,
           {0, gs::rgb4(13, 15, 15), gs::rgb4(4, 6, 7), gs::rgb4(15, 13, 6), gs::rgb4(1, 2, 3), gs::rgb4(15, 12, 4)});

    art.sub = gs::uploadMipped(vdp, paintSub());
    art.prop[0] = gs::uploadMipped(vdp, paintProp(false));
    art.prop[1] = gs::uploadMipped(vdp, paintProp(true));
    for (int i = 0; i < 3; i++) {
        art.rock[i] = gs::uploadMipped(vdp, rockStrip(i + 1, false));
        art.ceil[i] = gs::uploadMipped(vdp, rockStrip(i + 9, true));
    }
    art.fish[0] = gs::uploadMipped(vdp, paintFish(false));
    art.fish[1] = gs::uploadMipped(vdp, paintFish(true));
    art.mote = gs::uploadMipped(vdp, paintMote());
    art.puff = gs::uploadMipped(vdp, paintPuff());
    art.ring = gs::uploadMipped(vdp, paintRing());
    art.bulb = gs::uploadMipped(vdp, paintBulb());
    art.gate = gs::uploadMipped(vdp, paintGate());
    art.glow = gs::uploadMipped(vdp, paintGlow());
    art.title = bakeText(vdp, "S3 SUB", 3);
    art.tag = bakeText(vdp, "DON'T SCRAPE THE BOTTOM", 2);
    art.clear = bakeText(vdp, "CHANNEL CLEAR", 3);
    art.breach = bakeText(vdp, "HULL OPEN", 3);
    art.paused = bakeText(vdp, "PAUSED", 3);
    bakeFont(vdp, art.font);
}

}  // namespace sub

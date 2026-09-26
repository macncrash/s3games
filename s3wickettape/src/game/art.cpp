#include "game/art.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <initializer_list>

namespace wickettape {
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

gs::Image phrase(gs::VDP& vdp, const char* s, int scale) {
    return gs::uploadImage(vdp, gs::textBitmap(s, {scale, 1, 2, 0, 1}));
}

void paintBatsman(gs::Bitmap& b, int pose) {
    b.ellipse(16, 11, 5.4f, 5.6f, 2);
    b.rect(12, 5, 9, 4, 5);
    b.rect(18, 7, 5, 2, 5);
    b.rect(14, 11, 4, 2, 10);
    b.rect(11, 17, 14, 13, 3);
    b.rect(11, 17, 4, 13, 4);
    b.rect(12, 29, 12, 4, 8);
    b.rect(12, 32, 5, 16, 8);
    b.rect(19, 32, 5, 16, 8);
    b.rect(11, 47, 6, 4, 9);
    b.rect(19, 47, 6, 4, 9);
    if (pose == 0) {
        b.rect(24, 18, 4, 5, 2);
        b.rect(26, 12, 4, 28, 6);
        b.rect(27, 12, 2, 28, 7);
    } else if (pose == 1) {
        b.rect(8, 20, 5, 4, 2);
        b.line(10, 22, 4, 40, 6, 3.6f);
        b.line(12, 22, 8, 38, 7, 1.6f);
    } else if (pose == 2) {
        b.rect(24, 22, 5, 4, 2);
        b.rect(28, 21, 10, 4, 6);
        b.rect(28, 22, 10, 2, 7);
    } else if (pose == 3) {
        b.rect(18, 16, 4, 5, 2);
        b.line(20, 16, 30, 4, 6, 3.8f);
        b.line(22, 18, 28, 6, 7, 1.6f);
    } else {
        b.rect(8, 22, 4, 4, 2);
        b.rect(6, 28, 4, 16, 6);
        b.rect(12, 34, 5, 14, 8);
        b.rect(20, 32, 5, 16, 8);
        b.rect(11, 47, 6, 4, 9);
        b.rect(20, 47, 7, 4, 9);
    }
    b.outline(1, false);
}

void paintBowler(gs::Bitmap& b, int pose) {
    b.ellipse(20, 9, 4.8f, 5.f, 2);
    b.rect(16, 4, 8, 3, 7);
    b.rect(15, 14, 11, 12, 3);
    b.rect(15, 14, 3, 12, 4);
    b.rect(16, 25, 9, 6, 5);
    if (pose == 0) {
        b.line(18, 31, 10, 48, 5, 3.2f);
        b.line(24, 31, 32, 48, 5, 3.2f);
        b.rect(6, 47, 7, 3, 6);
        b.rect(28, 47, 7, 3, 6);
        b.line(16, 16, 8, 28, 2, 3.f);
        b.line(24, 16, 32, 24, 2, 2.6f);
    } else if (pose == 1) {
        b.line(18, 31, 16, 48, 5, 3.2f);
        b.line(24, 31, 26, 48, 5, 3.2f);
        b.rect(12, 47, 7, 3, 6);
        b.rect(23, 47, 7, 3, 6);
        b.line(18, 16, 8, 8, 2, 3.f);
        b.line(24, 16, 30, 26, 2, 2.6f);
    } else {
        b.line(18, 31, 14, 48, 5, 3.2f);
        b.line(24, 31, 28, 46, 5, 3.2f);
        b.rect(10, 47, 7, 3, 6);
        b.rect(25, 45, 7, 3, 6);
        b.line(16, 16, 4, 20, 2, 3.2f);
        b.ellipse(4, 20, 2.2f, 2.2f, 2);
    }
    b.outline(1, false);
}

void paintKeeper(gs::Bitmap& b) {
    b.ellipse(12, 8, 4.2f, 4.2f, 2);
    b.rect(8, 3, 8, 3, 7);
    b.rect(7, 12, 10, 8, 3);
    b.rect(6, 19, 5, 4, 5);
    b.rect(14, 19, 5, 4, 5);
    b.rect(8, 20, 4, 10, 5);
    b.rect(13, 20, 4, 10, 5);
    b.rect(7, 29, 5, 3, 6);
    b.rect(13, 29, 5, 3, 6);
    b.outline(1, false);
}

void paintFielder(gs::Bitmap& b) {
    b.ellipse(8, 5, 3.2f, 3.2f, 2);
    b.rect(5, 2, 6, 2, 6);
    b.rect(5, 8, 7, 7, 3);
    b.rect(5, 8, 2, 7, 4);
    b.rect(5, 15, 2, 6, 5);
    b.rect(9, 15, 2, 6, 5);
    b.rect(4, 20, 4, 2, 7);
    b.rect(8, 20, 4, 2, 7);
    b.outline(1, false);
}

gs::Bitmap ballArt(int seam) {
    gs::Bitmap b(14, 14);
    b.ellipse(7, 7, 5.6f, 5.6f, 1);
    b.ellipse(5, 5, 2.f, 1.4f, 4);
    if (seam == 0) b.line(7, 2, 7, 12, 3, 1.2f);
    else b.line(3, 4, 11, 10, 3, 1.2f);
    return b;
}

gs::Bitmap stumpArt() {
    gs::Bitmap b(22, 40);
    auto stick = [&](float x) {
        b.rect(x, 8, 4, 28, 1);
        b.rect(x, 8, 2, 28, 2);
        b.rect(x, 18, 4, 2, 3);
    };
    stick(2);
    stick(9);
    stick(16);
    b.rect(2, 6, 8, 3, 4);
    b.rect(12, 6, 8, 3, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap pitchArt() {
    gs::Bitmap b(200, 46);
    b.rect(0, 0, 200, 46, 4);
    for (int y = 0; y < 46; y++) {
        for (int x = 0; x < 200; x++) {
            uint32_t h = uint32_t(x) * 374761393u ^ uint32_t(y) * 668265263u;
            if ((h & 31u) == 0) b.set(x, y, 5);
        }
    }
    b.rect(28, 4, 2, 38, 6);
    b.rect(168, 4, 2, 38, 6);
    b.rect(18, 4, 12, 2, 6);
    b.rect(18, 40, 12, 2, 6);
    b.rect(168, 4, 14, 2, 6);
    b.rect(168, 40, 14, 2, 6);
    return b;
}

gs::Bitmap ropeArt() {
    gs::Bitmap b(28, 96);
    b.rect(3, 18, 4, 74, 3);
    b.rect(20, 28, 4, 64, 3);
    b.rect(2, 16, 6, 4, 4);
    b.rect(19, 26, 6, 4, 4);
    for (int i = 0; i <= 16; i++) {
        float t = i / 16.f;
        float x = 5.f + t * 16.f;
        float y = 22.f + std::sin(t * 3.14159265f) * 18.f;
        b.rect(x, y, 2.f, 2.f, 1);
        b.rect(x, y + 10.f, 2.f, 2.f, 2);
    }
    b.rect(2, 14, 5, 4, 5);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(36, 40);
    b.rect(16, 22, 5, 16, 3);
    b.rect(16, 22, 2, 16, 4);
    b.ellipse(18, 16, 12, 10, 1);
    b.ellipse(12, 18, 6, 5, 2);
    b.ellipse(24, 14, 6, 5, 2);
    return b;
}

gs::Bitmap crowdArt() {
    gs::Bitmap b(96, 20);
    for (int i = 0; i < 7; i++) {
        int x = 2 + i * 13;
        int shirt = 2 + (i % 3);
        b.ellipse(x + 5.f, 5.f, 2.8f, 2.8f, 1);
        b.rect(x + 2, 8, 7, 6, shirt);
        b.rect(x + 3, 14, 2, 5, 5);
        b.rect(x + 6, 14, 2, 5, 5);
    }
    return b;
}

gs::Bitmap houseArt() {
    gs::Bitmap b(64, 36);
    b.poly({{2.f, 14.f}, {32.f, 2.f}, {62.f, 14.f}}, 2);
    b.rect(8, 14, 48, 18, 1);
    b.rect(8, 14, 48, 3, 3);
    b.rect(28, 20, 8, 12, 3);
    b.rect(14, 18, 8, 6, 4);
    b.rect(42, 18, 8, 6, 4);
    b.rect(6, 31, 52, 3, 5);
    return b;
}

gs::Bitmap screenArt() {
    gs::Bitmap b(34, 26);
    b.rect(4, 2, 26, 16, 6);
    b.rect(4, 2, 26, 3, 7);
    b.rect(14, 18, 3, 7, 5);
    b.rect(20, 18, 3, 7, 5);
    return b;
}

gs::Bitmap cloudArt() {
    gs::Bitmap b(36, 14);
    b.ellipse(12, 8, 8, 4, 1);
    b.ellipse(22, 7, 10, 5, 1);
    b.ellipse(18, 5, 5, 3, 2);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 5.4f, 5.4f, 3);
    b.ellipse(6, 6, 2.f, 1.5f, 4);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(18, 8);
    b.ellipse(9, 4, 8, 3, 1);
    return b;
}

gs::Bitmap blotArt() {
    gs::Bitmap b(4, 4);
    b.rect(0, 0, 4, 4, 1);
    return b;
}

gs::Bitmap tapeArt() {
    gs::Bitmap b(168, 14);
    b.rect(0, 0, 168, 14, 3);
    b.rect(0, 0, 168, 2, 4);
    b.rect(0, 12, 168, 2, 4);
    for (int x = 4; x < 164; x += 8) b.rect(x, 1, 2, 2, 0);
    return b;
}

gs::Bitmap drawerArt() {
    gs::Bitmap b(176, 18);
    b.rect(0, 0, 176, 18, 7);
    b.rect(0, 0, 176, 3, 8);
    b.rect(4, 6, 168, 8, 9);
    b.rect(80, 1, 16, 3, 2);
    return b;
}

gs::Bitmap slotArt() {
    gs::Bitmap b(44, 12);
    b.rect(0, 0, 44, 12, 8);
    b.rect(2, 2, 40, 8, 9);
    return b;
}

gs::Bitmap slipArt(const char* name) {
    gs::Bitmap word = gs::textBitmap(name, {1, 1, 2, 0, 1});
    gs::Bitmap b(word.w + 6, word.h + 4);
    b.rect(0, 0, b.w, b.h, 3);
    b.rect(0, 0, b.w, 1, 5);
    b.blit(word, 3, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_INK, gs::rgb4(15, 15, 14), gs::rgb4(1, 1, 2));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 12, 3), gs::rgb4(3, 2, 0));
    textPal(vdp, PAL_GOOD, gs::rgb4(6, 15, 7), gs::rgb4(0, 3, 1));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 4, 3), gs::rgb4(3, 0, 0));
    setPal(vdp, PAL_SKY, {0, gs::rgb4(15, 15, 15), gs::rgb4(12, 13, 14), gs::rgb4(15, 13, 4), gs::rgb4(15, 15, 10)});
    setPal(vdp, PAL_KIT,
           {0, gs::rgb4(1, 1, 2), gs::rgb4(13, 9, 6), gs::rgb4(14, 14, 11), gs::rgb4(10, 10, 8), gs::rgb4(1, 7, 3),
            gs::rgb4(8, 5, 2), gs::rgb4(12, 9, 5), gs::rgb4(15, 15, 14), gs::rgb4(2, 2, 3), gs::rgb4(4, 2, 1)});
    setPal(vdp, PAL_BOWL,
           {0, gs::rgb4(1, 1, 2), gs::rgb4(13, 9, 6), gs::rgb4(2, 3, 10), gs::rgb4(1, 2, 6), gs::rgb4(15, 15, 14),
            gs::rgb4(2, 2, 3), gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_BALL, {0, gs::rgb4(13, 2, 2), gs::rgb4(8, 1, 1), gs::rgb4(15, 14, 12), gs::rgb4(15, 8, 6)});
    setPal(vdp, PAL_WOOD,
           {0, gs::rgb4(13, 10, 6), gs::rgb4(8, 5, 2), gs::rgb4(15, 13, 8), gs::rgb4(14, 12, 8), gs::rgb4(11, 8, 4),
            gs::rgb4(15, 15, 13), gs::rgb4(8, 4, 2), gs::rgb4(12, 7, 3), gs::rgb4(4, 2, 1), gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_GRASS, {0, gs::rgb4(2, 8, 2), gs::rgb4(5, 12, 4), gs::rgb4(6, 4, 2), gs::rgb4(4, 2, 1)});
    setPal(vdp, PAL_FIELD,
           {0, gs::rgb4(1, 1, 2), gs::rgb4(13, 9, 6), gs::rgb4(14, 13, 10), gs::rgb4(10, 9, 7), gs::rgb4(15, 15, 14),
            gs::rgb4(1, 6, 3), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_TWIN,
           {0, gs::rgb4(1, 1, 2), gs::rgb4(13, 9, 6), gs::rgb4(12, 3, 2), gs::rgb4(8, 2, 1), gs::rgb4(15, 15, 14),
            gs::rgb4(3, 2, 1), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_PAPER,
           {0, gs::rgb4(2, 2, 3), gs::rgb4(8, 7, 6), gs::rgb4(15, 14, 11), gs::rgb4(12, 10, 7), gs::rgb4(11, 2, 2)});
    setPal(vdp, PAL_ROPE,
           {0, gs::rgb4(15, 15, 13), gs::rgb4(10, 10, 8), gs::rgb4(8, 8, 7), gs::rgb4(5, 4, 3), gs::rgb4(12, 2, 2)});
    setPal(vdp, PAL_CROWD,
           {0, gs::rgb4(13, 9, 6), gs::rgb4(12, 2, 2), gs::rgb4(2, 4, 11), gs::rgb4(13, 11, 3), gs::rgb4(2, 2, 4)});
    setPal(vdp, PAL_HOUSE,
           {0, gs::rgb4(14, 13, 11), gs::rgb4(11, 3, 2), gs::rgb4(6, 3, 2), gs::rgb4(6, 10, 14), gs::rgb4(4, 3, 2),
            gs::rgb4(15, 15, 14), gs::rgb4(12, 12, 11)});

    loadFont(vdp, art);
    for (int i = 0; i < 5; i++) {
        gs::Bitmap b(40, 62);
        paintBatsman(b, i);
        art.batsman[i] = gs::uploadImage(vdp, b);
    }
    for (int i = 0; i < 3; i++) {
        gs::Bitmap b(40, 54);
        paintBowler(b, i);
        art.bowler[i] = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(24, 34);
        paintKeeper(b);
        art.keeper = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(16, 24);
        paintFielder(b);
        art.fielder = gs::uploadImage(vdp, b);
    }
    art.ball[0] = gs::uploadImage(vdp, ballArt(0));
    art.ball[1] = gs::uploadImage(vdp, ballArt(1));
    art.stump = gs::uploadImage(vdp, stumpArt());
    art.pitch = gs::uploadImage(vdp, pitchArt());
    art.rope = gs::uploadImage(vdp, ropeArt());
    art.tree = gs::uploadImage(vdp, treeArt());
    art.crowd = gs::uploadImage(vdp, crowdArt());
    art.house = gs::uploadImage(vdp, houseArt());
    art.screen = gs::uploadImage(vdp, screenArt());
    art.cloud = gs::uploadImage(vdp, cloudArt());
    art.sun = gs::uploadImage(vdp, sunArt());
    art.shadow = gs::uploadImage(vdp, shadowArt());
    art.blot = gs::uploadImage(vdp, blotArt());
    art.tape = gs::uploadImage(vdp, tapeArt());
    art.drawer = gs::uploadImage(vdp, drawerArt());
    art.slot = gs::uploadImage(vdp, slotArt());
    art.logo = phrase(vdp, "S3 WICKETTAPE", 2);
    art.matchW = phrase(vdp, "DRAWER MATCHES", 2);
    art.openW = phrase(vdp, "STILL OPEN", 2);
    for (int i = 0; i < kTapeN; i++) art.slip[i] = gs::uploadImage(vdp, slipArt(tapeName(i)));
}

}  // namespace wickettape

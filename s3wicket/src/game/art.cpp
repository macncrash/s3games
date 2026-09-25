#include "game/art.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>

namespace wicket {
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

void body(gs::Bitmap& b) {
    b.ellipse(16, 12, 6.4f, 6.6f, 9);
    b.ellipse(18, 13, 3.1f, 3.3f, 2);
    b.rect(15, 12, 5, 2, 1);
    b.rect(10, 18, 13, 12, 3);
    b.rect(10, 18, 4, 12, 4);
    b.rect(12, 29, 10, 5, 3);
    b.rect(12, 33, 5, 14, 5);
    b.rect(18, 33, 5, 14, 5);
    b.rect(12, 33, 5, 2, 8);
    b.rect(18, 33, 5, 2, 8);
    b.rect(12, 38, 5, 2, 8);
    b.rect(18, 38, 5, 2, 8);
    b.rect(11, 46, 6, 4, 10);
    b.rect(18, 46, 6, 4, 10);
}

void paintBatsman(gs::Bitmap& b, int pose) {
    body(b);
    if (pose == 0) {
        b.rect(21, 20, 5, 7, 2);
        b.rect(24, 18, 5, 6, 8);
        b.rect(24, 23, 5, 24, 6);
        b.rect(25, 23, 2, 24, 7);
    } else if (pose == 1) {
        b.rect(8, 16, 5, 7, 2);
        b.rect(6, 4, 5, 8, 8);
        b.line(8, 12, 14, 28, 6, 4.2f);
        b.line(10, 12, 16, 26, 7, 2.f);
    } else if (pose == 2) {
        b.rect(20, 22, 6, 5, 2);
        b.rect(24, 20, 16, 5, 6);
        b.rect(24, 21, 16, 2, 7);
        b.rect(22, 20, 4, 5, 8);
    } else {
        b.rect(14, 16, 5, 6, 2);
        b.line(16, 18, 34, 6, 6, 4.4f);
        b.line(18, 20, 32, 8, 7, 2.f);
        b.rect(12, 14, 5, 5, 8);
    }
    b.outline(1, false);
}

void paintBowler(gs::Bitmap& b, int pose) {
    b.ellipse(22, 11, 5.6f, 5.8f, 2);
    b.rect(17, 6, 9, 4, 7);
    b.rect(16, 16, 12, 12, 3);
    b.rect(16, 16, 4, 12, 4);
    b.rect(17, 27, 10, 7, 5);
    if (pose == 0) {
        b.line(20, 34, 12, 48, 5, 3.4f);
        b.line(26, 34, 32, 48, 5, 3.4f);
        b.rect(8, 47, 8, 3, 8);
        b.rect(28, 47, 8, 3, 8);
        b.line(18, 18, 8, 30, 2, 3.2f);
        b.line(24, 18, 32, 26, 2, 3.f);
    } else if (pose == 1) {
        b.line(20, 34, 14, 48, 5, 3.4f);
        b.line(26, 34, 24, 48, 5, 3.4f);
        b.rect(10, 47, 8, 3, 8);
        b.rect(20, 47, 8, 3, 8);
        b.line(18, 18, 10, 10, 2, 3.2f);
        b.line(26, 18, 34, 28, 2, 3.f);
    } else {
        b.line(20, 34, 18, 48, 5, 3.4f);
        b.line(26, 34, 28, 48, 5, 3.4f);
        b.rect(14, 47, 7, 3, 8);
        b.rect(26, 47, 7, 3, 8);
        b.line(20, 18, 5, 6, 2, 3.4f);
        b.ellipse(5, 6, 2.4f, 2.4f, 2);
        b.line(26, 18, 34, 24, 2, 3.f);
    }
    b.outline(1, false);
}

gs::Bitmap ballArt(int seam) {
    gs::Bitmap b(14, 14);
    b.ellipse(7, 7, 6.2f, 6.2f, 1);
    b.ellipse(5, 5, 2.2f, 1.6f, 4);
    if (seam == 0) b.line(7, 2, 7, 12, 3, 1.2f);
    else b.line(3, 4, 11, 10, 3, 1.2f);
    b.ellipse(7, 7, 6.2f, 6.2f, 2);
    b.ellipse(7, 7, 5.2f, 5.2f, 1);
    b.ellipse(5, 5, 2.0f, 1.4f, 4);
    if (seam == 0) b.line(7, 2, 7, 12, 3, 1.1f);
    else b.line(3, 4, 11, 10, 3, 1.1f);
    return b;
}

gs::Bitmap stumpArt(bool broken) {
    gs::Bitmap b(24, 42);
    auto stick = [&](float x, float lean) {
        if (!broken) {
            b.rect(x, 8, 4, 30, 1);
            b.rect(x, 8, 2, 30, 2);
            b.rect(x, 16, 4, 3, 5);
        } else {
            b.line(x + 2, 10, x + 2 + lean, 38, 1, 3.6f);
            b.line(x + 2, 18, x + 2 + lean * 0.6f, 24, 5, 3.2f);
        }
    };
    stick(2, -8);
    stick(10, 1);
    stick(17, 8);
    if (!broken) {
        b.rect(2, 6, 9, 3, 3);
        b.rect(13, 6, 9, 3, 3);
    }
    b.outline(4, false);
    return b;
}

gs::Bitmap pitchArt() {
    gs::Bitmap b(240, 72);
    b.rect(0, 0, 240, 72, 1);
    for (int y = 0; y < 72; y++) {
        for (int x = 0; x < 240; x++) {
            uint32_t h = uint32_t(x) * 374761393u ^ uint32_t(y) * 668265263u;
            if ((h & 31u) == 0) b.set(x, y, 2);
            else if ((h & 63u) == 1) b.set(x, y, 3);
        }
    }
    b.ellipse(120, 36, 34, 10, 2);
    b.rect(40, 6, 2, 60, 4);
    b.rect(196, 6, 2, 60, 4);
    b.rect(28, 6, 2, 18, 4);
    b.rect(28, 48, 2, 18, 4);
    b.rect(208, 6, 2, 18, 4);
    b.rect(208, 48, 2, 18, 4);
    b.rect(28, 6, 14, 2, 4);
    b.rect(28, 64, 14, 2, 4);
    b.rect(196, 6, 14, 2, 4);
    b.rect(196, 64, 14, 2, 4);
    return b;
}

gs::Bitmap ropeArt() {
    gs::Bitmap b(44, 120);
    b.rect(4, 30, 5, 86, 3);
    b.rect(33, 38, 5, 78, 3);
    b.rect(3, 28, 7, 5, 4);
    b.rect(32, 36, 7, 5, 4);
    for (int i = 0; i <= 30; i++) {
        float t = i / 30.f;
        float x = 6.f + t * 28.f;
        float sag = std::sin(t * 3.14159265f);
        float y = 34.f + sag * 36.f;
        b.rect(x, y, 2.2f, 2.2f, 1);
        b.rect(x, y + 3.f, 2.2f, 2.2f, 2);
    }
    b.poly({{7.f, 30.f}, {18.f, 36.f}, {7.f, 42.f}}, 5);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(48, 42);
    b.rect(21, 24, 6, 16, 3);
    b.rect(20, 24, 2, 16, 4);
    b.ellipse(24, 16, 16, 12, 1);
    b.ellipse(16, 18, 8, 7, 2);
    b.ellipse(32, 14, 8, 6, 2);
    return b;
}

gs::Bitmap crowdArt() {
    gs::Bitmap b(112, 22);
    for (int i = 0; i < 8; i++) {
        int x = 2 + i * 14;
        int shirt = 2 + (i % 3);
        b.ellipse(x + 5.f, 6.f, 3.2f, 3.2f, 1);
        b.rect(x + 2, 10, 8, 7, shirt);
        b.rect(x + 3, 17, 2, 5, 5);
        b.rect(x + 7, 17, 2, 5, 5);
    }
    return b;
}

gs::Bitmap houseArt() {
    gs::Bitmap b(72, 40);
    b.poly({{2.f, 16.f}, {36.f, 2.f}, {70.f, 16.f}}, 3);
    b.rect(8, 16, 56, 22, 1);
    b.rect(8, 16, 56, 3, 2);
    b.rect(32, 24, 10, 14, 5);
    b.rect(14, 22, 10, 8, 6);
    b.rect(50, 22, 10, 8, 6);
    b.rect(6, 36, 60, 3, 4);
    return b;
}

gs::Bitmap screenArt() {
    gs::Bitmap b(40, 28);
    b.rect(1, 1, 38, 26, 1);
    b.rect(1, 1, 38, 3, 7);
    b.rect(1, 24, 38, 3, 7);
    b.rect(1, 1, 3, 26, 7);
    b.rect(36, 1, 3, 26, 7);
    gs::Bitmap word = gs::textBitmap("SIX", {1, 3, 0, 0, 1});
    b.blit(word, 8, 10);
    return b;
}

gs::Bitmap cloudArt() {
    gs::Bitmap b(40, 16);
    b.ellipse(14, 9, 10, 5, 1);
    b.ellipse(24, 8, 12, 6, 1);
    b.ellipse(20, 6, 7, 4, 2);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 6.2f, 6.2f, 1);
    b.ellipse(6, 6, 2.f, 1.6f, 2);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(20, 8);
    b.ellipse(10, 4, 9, 3, 1);
    return b;
}

gs::Bitmap blotArt() {
    gs::Bitmap b(4, 4);
    b.rect(0, 0, 4, 4, 1);
    return b;
}

gs::Bitmap bailArt() {
    gs::Bitmap b(8, 4);
    b.rect(0, 1, 8, 2, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_CLOUD, {0, gs::rgb4(15, 15, 15), gs::rgb4(12, 13, 15)});
    setPal(vdp, PAL_PITCH,
           {0, gs::rgb4(10, 7, 3), gs::rgb4(7, 5, 2), gs::rgb4(12, 9, 5), gs::rgb4(15, 15, 13)});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(2, 8, 2), gs::rgb4(5, 12, 3), gs::rgb4(6, 4, 2), gs::rgb4(4, 2, 1)});
    setPal(vdp, PAL_BAT,
           {0, gs::rgb4(1, 1, 2), gs::rgb4(13, 9, 6), gs::rgb4(15, 15, 14), gs::rgb4(12, 12, 13), gs::rgb4(14, 13, 10),
            gs::rgb4(10, 6, 2), gs::rgb4(13, 9, 4), gs::rgb4(3, 3, 4), gs::rgb4(2, 5, 12), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_BOWL,
           {0, gs::rgb4(1, 1, 2), gs::rgb4(13, 9, 6), gs::rgb4(12, 2, 3), gs::rgb4(8, 1, 2), gs::rgb4(15, 15, 14),
            gs::rgb4(2, 1, 1), gs::rgb4(1, 1, 1), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_BALL, {0, gs::rgb4(13, 2, 2), gs::rgb4(8, 1, 1), gs::rgb4(15, 14, 12), gs::rgb4(15, 8, 6)});
    setPal(vdp, PAL_STUMP,
           {0, gs::rgb4(12, 9, 5), gs::rgb4(8, 5, 2), gs::rgb4(15, 14, 10), gs::rgb4(2, 1, 1), gs::rgb4(11, 2, 2)});
    setPal(vdp, PAL_ROPE,
           {0, gs::rgb4(15, 15, 13), gs::rgb4(11, 11, 9), gs::rgb4(12, 12, 11), gs::rgb4(8, 8, 7), gs::rgb4(13, 2, 2)});
    setPal(vdp, PAL_CROWD,
           {0, gs::rgb4(13, 9, 6), gs::rgb4(12, 2, 2), gs::rgb4(2, 4, 12), gs::rgb4(13, 11, 2), gs::rgb4(2, 2, 4)});
    setPal(vdp, PAL_SHADE, {0, gs::rgb4(1, 2, 1)});
    textPal(vdp, PAL_INK, gs::rgb4(15, 15, 14), gs::rgb4(1, 1, 2));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 13, 4), gs::rgb4(3, 2, 1));
    textPal(vdp, PAL_GREEN, gs::rgb4(5, 15, 6), gs::rgb4(1, 3, 1));
    textPal(vdp, PAL_RED, gs::rgb4(15, 4, 3), gs::rgb4(3, 0, 0));
    textPal(vdp, PAL_TITLE, gs::rgb4(15, 13, 4), gs::rgb4(2, 2, 6));
    setPal(vdp, PAL_HOUSE,
           {0, gs::rgb4(14, 13, 11), gs::rgb4(11, 10, 8), gs::rgb4(11, 2, 2), gs::rgb4(7, 1, 1), gs::rgb4(5, 3, 2),
            gs::rgb4(6, 10, 14), gs::rgb4(3, 3, 4)});

    loadFont(vdp, art);
    for (int i = 0; i < 4; i++) {
        gs::Bitmap b(42, 56);
        paintBatsman(b, i);
        art.batsman[i] = gs::uploadImage(vdp, b);
    }
    for (int i = 0; i < 3; i++) {
        gs::Bitmap b(40, 54);
        paintBowler(b, i);
        art.bowler[i] = gs::uploadImage(vdp, b);
    }
    art.ball[0] = gs::uploadImage(vdp, ballArt(0));
    art.ball[1] = gs::uploadImage(vdp, ballArt(1));
    art.stump[0] = gs::uploadImage(vdp, stumpArt(false));
    art.stump[1] = gs::uploadImage(vdp, stumpArt(true));
    art.bail = gs::uploadImage(vdp, bailArt());
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
    art.title = phrase(vdp, "S3 WICKET", 3);
    art.sixBalls = phrase(vdp, "SIX BALLS", 2);
    art.hitWicket = phrase(vdp, "HIT THE WICKET", 2);
    art.clearRope = phrase(vdp, "CLEARS THE ROPE", 2);
    art.bowledWord = phrase(vdp, "BOWLED", 2);
    art.ours = phrase(vdp, "THE OVER IS OURS", 2);
    art.gone = phrase(vdp, "SIX BALLS GONE", 2);
}

}  // namespace wicket

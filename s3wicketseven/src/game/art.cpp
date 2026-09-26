#include "game/art.h"

#include <cstdint>
#include <initializer_list>

namespace wicketseven {
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

gs::Image phrase(gs::VDP& vdp, const char* s, int scale) {
    return gs::uploadImage(vdp, gs::textBitmap(s, {scale, 1, 2, 0, 1}));
}

// Shared index layout for both kits. Shirt, shade, and cap change per palette.
void paintBatsman(gs::Bitmap& b, int pose) {
    b.ellipse(20, 12, 6.4f, 6.2f, 2);
    b.ellipse(20, 9, 7.4f, 6.0f, 9);
    b.rect(13, 13, 14, 3, 13);
    b.rect(14, 14, 12, 2, 2);
    b.rect(13, 18, 14, 12, 3);
    b.rect(13, 18, 4, 12, 4);
    b.rect(16, 19, 6, 3, 4);
    b.rect(13, 30, 7, 16, 5);
    b.rect(21, 30, 7, 16, 5);
    b.rect(13, 30, 7, 3, 6);
    b.rect(21, 30, 7, 3, 6);
    b.rect(13, 38, 7, 2, 6);
    b.rect(21, 38, 7, 2, 6);
    b.rect(13, 46, 8, 4, 11);
    b.rect(21, 46, 8, 4, 11);
    if (pose == 0) {
        b.rect(29, 16, 5, 32, 7);
        b.rect(30, 16, 2, 32, 8);
        b.rect(28, 14, 7, 4, 10);
    } else if (pose == 1) {
        b.line(26, 22, 44, 32, 7, 4.4f);
        b.line(26, 22, 44, 32, 8, 1.6f);
        b.rect(23, 18, 6, 5, 10);
    } else if (pose == 2) {
        b.line(24, 24, 46, 18, 7, 4.6f);
        b.line(24, 24, 46, 18, 8, 1.6f);
        b.rect(22, 20, 6, 5, 10);
    } else {
        b.line(22, 20, 44, 4, 7, 4.6f);
        b.line(22, 20, 44, 4, 8, 1.6f);
        b.rect(20, 16, 6, 5, 10);
    }
    b.outline(1, false);
}

void paintBowler(gs::Bitmap& b, int pose) {
    b.ellipse(26, 11, 5.8f, 5.8f, 2);
    b.rect(20, 4, 13, 4, 9);
    b.rect(16, 5, 6, 3, 9);
    b.rect(19, 16, 13, 12, 3);
    b.rect(19, 16, 4, 12, 4);
    b.rect(20, 28, 6, 12, 5);
    b.rect(27, 28, 6, 12, 5);
    if (pose == 0) {
        b.line(22, 40, 12, 52, 5, 3.6f);
        b.line(30, 40, 38, 52, 5, 3.6f);
        b.rect(8, 51, 9, 3, 11);
        b.rect(33, 51, 9, 3, 11);
        b.line(20, 18, 6, 30, 2, 3.3f);
        b.line(30, 18, 40, 28, 2, 3.f);
    } else if (pose == 1) {
        b.line(23, 40, 16, 52, 5, 3.6f);
        b.line(30, 40, 34, 52, 5, 3.6f);
        b.rect(12, 51, 9, 3, 11);
        b.rect(29, 51, 9, 3, 11);
        b.line(20, 18, 8, 6, 2, 3.3f);
        b.line(30, 18, 40, 26, 2, 3.f);
    } else {
        b.line(24, 40, 18, 52, 5, 3.4f);
        b.line(31, 40, 36, 52, 5, 3.4f);
        b.rect(14, 51, 8, 3, 11);
        b.rect(32, 51, 8, 3, 11);
        b.line(20, 16, 5, 6, 2, 3.5f);
        b.ellipse(5, 6, 2.4f, 2.4f, 10);
        b.line(31, 18, 42, 30, 2, 3.f);
    }
    b.outline(1, false);
}

gs::Bitmap ballArt(int seam) {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 6.4f, 6.4f, 1);
    b.ellipse(6, 6, 2.1f, 1.5f, 4);
    if (seam == 0) b.line(8, 3, 8, 13, 3, 1.3f);
    else b.line(4, 5, 12, 11, 3, 1.3f);
    b.outline(2, false);
    return b;
}

gs::Bitmap stumpArt(bool broken) {
    gs::Bitmap b(28, 46);
    auto stick = [&](float x, float lean) {
        if (!broken) {
            b.rect(x, 8, 4, 32, 1);
            b.rect(x + 1, 8, 2, 32, 2);
            b.rect(x, 16, 4, 3, 5);
        } else {
            b.line(x + 2, 10, x + 2 + lean, 42, 1, 3.5f);
        }
    };
    stick(3, -8);
    stick(11, 1);
    stick(19, 8);
    if (!broken) {
        b.rect(3, 5, 10, 3, 3);
        b.rect(15, 5, 10, 3, 3);
    }
    b.outline(4, false);
    return b;
}

gs::Bitmap pitchArt() {
    gs::Bitmap b(200, 72);
    b.rect(0, 0, 200, 72, 1);
    for (int y = 0; y < 72; y++) {
        for (int x = 0; x < 200; x++) {
            uint32_t h = uint32_t(x) * 2246822519u ^ uint32_t(y) * 3266489917u;
            if ((h & 31u) == 0) b.set(x, y, 2);
            else if ((h & 53u) == 1) b.set(x, y, 5);
        }
    }
    b.ellipse(100, 36, 28, 9, 3);
    b.rect(36, 4, 2, 64, 4);
    b.rect(48, 4, 2, 64, 4);
    b.rect(150, 4, 2, 64, 4);
    b.rect(162, 4, 2, 64, 4);
    b.rect(36, 4, 16, 2, 4);
    b.rect(36, 66, 16, 2, 4);
    b.rect(148, 4, 16, 2, 4);
    b.rect(148, 66, 16, 2, 4);
    return b;
}

gs::Bitmap ropeArt() {
    gs::Bitmap b(36, 120);
    b.rect(4, 8, 5, 104, 3);
    b.rect(27, 16, 5, 96, 3);
    b.rect(2, 6, 9, 5, 4);
    b.rect(25, 14, 9, 5, 4);
    for (int i = 0; i <= 24; i++) {
        float t = i / 24.f;
        float x = 6.f + t * 22.f;
        float sag = 1.f - (t - 0.5f) * (t - 0.5f) * 4.f;
        float y = 18.f + sag * 28.f;
        b.rect(x, y, 2.2f, 2.2f, 1);
        b.rect(x, y + 18.f, 2.2f, 2.2f, 2);
    }
    b.rect(6, 4, 8, 6, 5);
    b.rect(9, 1, 2, 4, 5);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(44, 40);
    b.rect(19, 22, 6, 16, 3);
    b.rect(18, 22, 2, 16, 4);
    b.ellipse(22, 16, 16, 12, 1);
    b.ellipse(14, 18, 8, 7, 2);
    b.ellipse(30, 14, 9, 7, 2);
    return b;
}

gs::Bitmap crowdArt() {
    gs::Bitmap b(112, 24);
    for (int i = 0; i < 8; i++) {
        int x = 2 + i * 14;
        int shirt = 2 + (i % 3);
        b.ellipse(x + 5.f, 6.f, 3.3f, 3.3f, 1);
        b.rect(x + 3, 3, 5, 2, 6);
        b.rect(x + 2, 10, 8, 8, shirt);
        b.rect(x + 3, 18, 2, 5, 5);
        b.rect(x + 7, 18, 2, 5, 5);
    }
    return b;
}

gs::Bitmap houseArt() {
    gs::Bitmap b(88, 48);
    b.poly({{4.f, 18.f}, {44.f, 2.f}, {84.f, 18.f}}, 3);
    b.poly({{14.f, 18.f}, {44.f, 8.f}, {74.f, 18.f}}, 4);
    b.rect(10, 18, 68, 24, 1);
    b.rect(10, 18, 68, 3, 2);
    b.rect(38, 26, 12, 16, 5);
    b.rect(16, 24, 12, 9, 6);
    b.rect(60, 24, 12, 9, 6);
    b.rect(18, 26, 8, 5, 7);
    b.rect(62, 26, 8, 5, 7);
    b.rect(8, 40, 72, 4, 2);
    b.rect(36, 8, 16, 8, 9);
    gs::Bitmap num = gs::textBitmap("7", {1, 8, 0, 0, 0});
    b.blit(num, 41, 9);
    return b;
}

gs::Bitmap screenArt() {
    gs::Bitmap b(42, 32);
    b.rect(1, 1, 40, 30, 9);
    b.rect(1, 1, 40, 3, 7);
    b.rect(1, 28, 40, 3, 7);
    b.rect(1, 1, 3, 30, 7);
    b.rect(38, 1, 3, 30, 7);
    b.rect(8, 7, 26, 4, 8);
    b.line(30, 10, 16, 24, 8, 4.2f);
    return b;
}

gs::Bitmap cloudArt() {
    gs::Bitmap b(42, 16);
    b.ellipse(14, 9, 11, 5, 1);
    b.ellipse(26, 8, 12, 6, 1);
    b.ellipse(20, 6, 7, 4, 2);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(18, 18);
    b.ellipse(9, 9, 7.f, 7.f, 1);
    b.ellipse(7, 7, 2.2f, 1.6f, 2);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(22, 8);
    b.ellipse(11, 4, 10, 3, 1);
    return b;
}

gs::Bitmap blotArt() {
    gs::Bitmap b(4, 4);
    b.rect(0, 0, 4, 4, 1);
    return b;
}

gs::Bitmap bailArt() {
    gs::Bitmap b(10, 4);
    b.rect(0, 1, 10, 2, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_CLOUD, {0, gs::rgb4(15, 15, 15), gs::rgb4(12, 14, 15)});
    setPal(vdp, PAL_PITCH,
           {0, gs::rgb4(12, 9, 5), gs::rgb4(8, 6, 3), gs::rgb4(14, 11, 7), gs::rgb4(15, 15, 13), gs::rgb4(6, 5, 3)});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(2, 7, 2), gs::rgb4(5, 11, 3), gs::rgb4(6, 4, 2), gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_BAT,
           {0, gs::rgb4(1, 1, 2), gs::rgb4(13, 9, 6), gs::rgb4(15, 15, 14), gs::rgb4(12, 12, 13), gs::rgb4(14, 13, 10),
            gs::rgb4(8, 7, 5), gs::rgb4(9, 5, 2), gs::rgb4(13, 8, 3), gs::rgb4(2, 3, 7), gs::rgb4(15, 15, 14),
            gs::rgb4(2, 2, 2), gs::rgb4(10, 6, 4), gs::rgb4(4, 5, 6)});
    setPal(vdp, PAL_BOWL,
           {0, gs::rgb4(1, 1, 2), gs::rgb4(13, 9, 6), gs::rgb4(11, 2, 3), gs::rgb4(6, 1, 2), gs::rgb4(14, 13, 10),
            gs::rgb4(8, 7, 5), gs::rgb4(9, 5, 2), gs::rgb4(13, 8, 3), gs::rgb4(14, 12, 4), gs::rgb4(15, 15, 14),
            gs::rgb4(2, 2, 2), gs::rgb4(10, 6, 4), gs::rgb4(4, 5, 6)});
    setPal(vdp, PAL_BALL, {0, gs::rgb4(12, 2, 2), gs::rgb4(6, 1, 1), gs::rgb4(15, 14, 12), gs::rgb4(15, 9, 7)});
    setPal(vdp, PAL_STUMP,
           {0, gs::rgb4(13, 10, 6), gs::rgb4(8, 6, 3), gs::rgb4(15, 14, 11), gs::rgb4(2, 1, 1), gs::rgb4(12, 3, 3)});
    setPal(vdp, PAL_ROPE,
           {0, gs::rgb4(15, 15, 14), gs::rgb4(10, 10, 8), gs::rgb4(7, 4, 2), gs::rgb4(11, 8, 4), gs::rgb4(13, 2, 2)});
    setPal(vdp, PAL_CROWD,
           {0, gs::rgb4(13, 9, 6), gs::rgb4(12, 2, 2), gs::rgb4(2, 4, 11), gs::rgb4(13, 11, 3), gs::rgb4(2, 2, 3),
            gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_SHADE, {0, gs::rgb4(1, 2, 1)});
    textPal(vdp, PAL_INK, gs::rgb4(15, 15, 14), gs::rgb4(1, 1, 2));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 13, 3), gs::rgb4(3, 2, 0));
    textPal(vdp, PAL_GREEN, gs::rgb4(4, 15, 5), gs::rgb4(0, 3, 1));
    textPal(vdp, PAL_RED, gs::rgb4(15, 4, 3), gs::rgb4(3, 0, 0));
    textPal(vdp, PAL_TITLE, gs::rgb4(15, 13, 4), gs::rgb4(1, 1, 4));
    setPal(vdp, PAL_HOUSE,
           {0, gs::rgb4(14, 12, 10), gs::rgb4(10, 8, 7), gs::rgb4(11, 2, 2), gs::rgb4(6, 1, 1), gs::rgb4(5, 3, 2),
            gs::rgb4(7, 11, 14), gs::rgb4(3, 3, 4), gs::rgb4(15, 12, 3), gs::rgb4(2, 4, 6), gs::rgb4(4, 7, 9)});

    loadFont(vdp, art);
    for (int i = 0; i < 4; i++) {
        gs::Bitmap b(48, 60);
        paintBatsman(b, i);
        art.batsman[i] = gs::uploadImage(vdp, b);
    }
    for (int i = 0; i < 3; i++) {
        gs::Bitmap b(48, 60);
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
    art.wicketWord = phrase(vdp, "WICKET", 3);
    art.sevenWord = phrase(vdp, "SEVEN", 3);
    art.ruleWord = phrase(vdp, "FIRST TO SEVEN", 2);
    art.leaveWord = phrase(vdp, "LEAVE", 3);
    art.oneWord = phrase(vdp, "ONE", 2);
    art.twoWord = phrase(vdp, "TWO", 2);
    art.fourWord = phrase(vdp, "FOUR", 2);
    art.sixWord = phrase(vdp, "SIX", 2);
    art.caughtWord = phrase(vdp, "CAUGHT", 2);
    art.beatenWord = phrase(vdp, "BEATEN", 2);
    art.soonWord = phrase(vdp, "TOO SOON", 2);
    art.lateWord = phrase(vdp, "TOO LATE", 2);
    art.lineWord = phrase(vdp, "WRONG LINE", 2);
    art.misWord = phrase(vdp, "MISREAD", 2);
}

}  // namespace wicketseven

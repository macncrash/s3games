#include "game/art.h"

#include <cstdint>

namespace wicketgold {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    if (pal < PAL_INK) vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
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
    b.ellipse(16, 9, 5.2f, 5.4f, 1);
    b.ellipse(17.5f, 9.5f, 2.2f, 2.4f, 2);
    b.rect(12, 4, 8, 3, 9);
    b.rect(11, 6, 3, 3, 9);
    b.rect(14, 14, 8, 11, 3);
    b.rect(14, 14, 3, 11, 4);
    b.rect(15, 24, 7, 4, 5);
    b.rect(15, 28, 3, 12, 5);
    b.rect(20, 28, 3, 12, 6);
    b.rect(14, 39, 5, 3, 10);
    b.rect(19, 39, 5, 3, 10);
    if (pose == 0) {
        b.rect(22, 16, 4, 5, 2);
        b.rect(24, 18, 3, 22, 7);
        b.rect(25, 18, 1, 22, 8);
    } else if (pose == 1) {
        b.rect(21, 18, 5, 4, 2);
        b.rect(24, 17, 14, 4, 7);
        b.rect(24, 18, 14, 2, 8);
    } else if (pose == 2) {
        b.rect(18, 14, 4, 4, 2);
        b.line(20, 16, 34, 4, 7, 3.4f);
        b.line(21, 17, 32, 6, 8, 1.6f);
    } else {
        b.rect(10, 16, 4, 4, 2);
        b.line(12, 18, 6, 30, 7, 3.2f);
        b.line(13, 18, 8, 28, 8, 1.4f);
    }
    b.outline(15, false);
}

void paintBowler(gs::Bitmap& b, int pose) {
    b.ellipse(22, 8, 4.6f, 4.8f, 1);
    b.rect(18, 3, 8, 3, 5);
    b.rect(17, 13, 10, 10, 3);
    b.rect(17, 13, 3, 10, 4);
    b.rect(18, 22, 8, 5, 6);
    if (pose == 0) {
        b.line(20, 27, 14, 42, 7, 3.f);
        b.line(26, 27, 30, 42, 7, 3.f);
        b.rect(10, 41, 7, 3, 8);
        b.rect(26, 41, 7, 3, 8);
        b.line(18, 15, 8, 24, 2, 2.8f);
        b.line(26, 15, 32, 22, 2, 2.6f);
    } else if (pose == 1) {
        b.line(20, 27, 16, 42, 7, 3.f);
        b.line(26, 27, 24, 42, 7, 3.f);
        b.rect(12, 41, 7, 3, 8);
        b.rect(20, 41, 7, 3, 8);
        b.line(18, 14, 10, 6, 2, 2.8f);
        b.line(26, 16, 34, 24, 2, 2.6f);
    } else {
        b.line(21, 27, 18, 42, 7, 3.f);
        b.line(26, 27, 28, 42, 7, 3.f);
        b.rect(14, 41, 7, 3, 8);
        b.rect(25, 41, 7, 3, 8);
        b.line(18, 15, 4, 18, 2, 3.f);
        b.ellipse(4, 18, 2.2f, 2.2f, 1);
    }
    b.outline(15, false);
}

void paintUmpire(gs::Bitmap& b, int pose) {
    b.ellipse(14, 7, 4.2f, 4.4f, 1);
    b.rect(11, 2, 7, 3, 7);
    b.rect(10, 12, 9, 12, 3);
    b.rect(10, 12, 3, 12, 4);
    b.rect(11, 23, 7, 10, 7);
    b.rect(10, 32, 4, 2, 8);
    b.rect(16, 32, 4, 2, 8);
    if (pose == 1) {
        b.line(10, 14, 3, 4, 3, 2.6f);
        b.line(18, 14, 25, 4, 3, 2.6f);
        b.rect(1, 2, 4, 3, 2);
        b.rect(23, 2, 4, 3, 2);
    } else if (pose == 2) {
        b.line(18, 16, 28, 16, 3, 2.6f);
        b.rect(26, 14, 4, 4, 2);
        b.rect(8, 16, 3, 8, 4);
    } else {
        b.rect(7, 14, 3, 8, 4);
        b.rect(19, 14, 3, 8, 4);
    }
    b.outline(15, false);
}

gs::Bitmap ballArt(int seam) {
    gs::Bitmap b(14, 14);
    b.ellipse(7, 7, 6.f, 6.f, 1);
    b.ellipse(5, 5, 2.f, 1.4f, 4);
    if (seam == 0) b.line(7, 2, 7, 12, 3, 1.2f);
    else b.line(3, 5, 11, 9, 3, 1.2f);
    b.outline(15, false);
    return b;
}

gs::Bitmap stumpArt(bool broken) {
    gs::Bitmap b(22, 40);
    auto stick = [&](float x, float lean) {
        if (!broken) {
            b.rect(x, 8, 3.2f, 28, 1);
            b.rect(x, 8, 1.4f, 28, 2);
        } else {
            b.line(x + 1.5f, 10, x + 1.5f + lean, 36, 1, 3.f);
        }
    };
    stick(2, -7);
    stick(9, 0);
    stick(16, 7);
    if (!broken) {
        b.rect(2, 6, 8, 2.4f, 2);
        b.rect(12, 6, 8, 2.4f, 2);
    }
    b.outline(15, false);
    return b;
}

gs::Bitmap pitchArt() {
    gs::Bitmap b(168, 46);
    b.rect(0, 6, 168, 34, 1);
    for (int y = 6; y < 40; y++) {
        for (int x = 0; x < 168; x++) {
            uint32_t h = uint32_t(x) * 2246822519u ^ uint32_t(y) * 3266489917u;
            if ((h & 31u) == 0) b.set(x, y, 2);
            else if ((h & 47u) == 1) b.set(x, y, 3);
        }
    }
    b.ellipse(84, 23, 26, 7, 3);
    b.rect(22, 6, 2, 34, 4);
    b.rect(144, 6, 2, 34, 4);
    b.rect(14, 6, 12, 2, 4);
    b.rect(14, 38, 12, 2, 4);
    b.rect(142, 6, 14, 2, 4);
    b.rect(142, 38, 14, 2, 4);
    return b;
}

gs::Bitmap ropeArt() {
    gs::Bitmap b(36, 96);
    b.rect(3, 18, 4, 74, 3);
    b.rect(28, 26, 4, 66, 3);
    b.rect(2, 16, 6, 4, 4);
    b.rect(27, 24, 6, 4, 4);
    for (int i = 0; i <= 24; i++) {
        float t = i / 24.f;
        float x = 5.f + t * 22.f;
        float sag = 1.f - (t - 0.5f) * (t - 0.5f) * 4.f;
        float y = 22.f + sag * 28.f;
        b.rect(x, y, 2.f, 2.f, 1);
        b.rect(x, y + 2.f, 2.f, 2.f, 2);
    }
    b.poly({{4.f, 18.f}, {14.f, 24.f}, {4.f, 30.f}}, 4);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(40, 36);
    b.rect(17, 20, 5, 14, 3);
    b.rect(17, 20, 2, 14, 4);
    b.ellipse(20, 14, 14, 10, 1);
    b.ellipse(13, 15, 6, 5, 2);
    b.ellipse(27, 12, 6, 5, 2);
    return b;
}

gs::Bitmap crowdArt() {
    gs::Bitmap b(90, 18);
    for (int i = 0; i < 7; i++) {
        int x = 2 + i * 13;
        int shirt = 2 + (i % 3);
        b.ellipse(x + 4.f, 5.f, 2.8f, 2.8f, 1);
        b.rect(x + 1, 8, 7, 6, shirt);
        b.rect(x + 2, 14, 2, 4, 5);
        b.rect(x + 5, 14, 2, 4, 5);
    }
    return b;
}

gs::Bitmap houseArt() {
    gs::Bitmap b(64, 36);
    b.poly({{2.f, 14.f}, {32.f, 2.f}, {62.f, 14.f}}, 3);
    b.rect(8, 14, 48, 18, 1);
    b.rect(8, 14, 48, 3, 2);
    b.rect(28, 20, 8, 12, 5);
    b.rect(14, 18, 8, 7, 6);
    b.rect(42, 18, 8, 7, 6);
    b.rect(6, 31, 52, 3, 4);
    return b;
}

gs::Bitmap cloudArt() {
    gs::Bitmap b(36, 14);
    b.ellipse(12, 8, 9, 4.5f, 1);
    b.ellipse(22, 7, 10, 5, 1);
    b.ellipse(18, 5, 6, 3.5f, 2);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(14, 14);
    b.ellipse(7, 7, 5.4f, 5.4f, 1);
    b.ellipse(5, 5, 1.8f, 1.4f, 4);
    return b;
}

gs::Bitmap starArt() {
    gs::Bitmap b(9, 9);
    b.rect(4, 0, 1, 9, 1);
    b.rect(0, 4, 9, 1, 1);
    b.set(4, 4, 4);
    b.set(3, 3, 3);
    b.set(5, 3, 3);
    b.set(3, 5, 3);
    b.set(5, 5, 3);
    return b;
}

gs::Bitmap blotArt() {
    gs::Bitmap b(4, 4);
    b.rect(0, 0, 4, 4, 1);
    return b;
}

gs::Bitmap bailArt() {
    gs::Bitmap b(8, 3);
    b.rect(0, 1, 8, 2, 1);
    return b;
}

gs::Bitmap plateArt() {
    gs::Bitmap b(34, 16);
    b.rect(0, 0, 34, 16, 2);
    b.rect(2, 2, 30, 12, 1);
    gs::Bitmap word = gs::textBitmap("X2", {2, 2, 0, 0, 0});
    b.blit(word, 5, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_CLOUD, {0, gs::rgb4(15, 15, 15), gs::rgb4(13, 14, 15)});
    setPal(vdp, PAL_PITCH, {0, gs::rgb4(12, 9, 4), gs::rgb4(8, 6, 2), gs::rgb4(14, 12, 7), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(2, 8, 2), gs::rgb4(6, 12, 3), gs::rgb4(6, 4, 2), gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_BAT,
           {0, gs::rgb4(13, 9, 6), gs::rgb4(9, 6, 4), gs::rgb4(15, 15, 13), gs::rgb4(12, 12, 10), gs::rgb4(14, 14, 12),
            gs::rgb4(11, 11, 9), gs::rgb4(10, 6, 2), gs::rgb4(13, 9, 4), gs::rgb4(1, 2, 8), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_BOWL,
           {0, gs::rgb4(13, 9, 6), gs::rgb4(9, 6, 4), gs::rgb4(15, 15, 14), gs::rgb4(12, 12, 11), gs::rgb4(12, 2, 2),
            gs::rgb4(8, 1, 1), gs::rgb4(2, 2, 3), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_CREAM, {0, gs::rgb4(14, 12, 8), gs::rgb4(9, 7, 4), gs::rgb4(13, 2, 2), gs::rgb4(15, 15, 13)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 3), gs::rgb4(10, 7, 1), gs::rgb4(15, 14, 8), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_STUMP, {0, gs::rgb4(13, 10, 5), gs::rgb4(8, 5, 2), gs::rgb4(12, 2, 2), gs::rgb4(5, 3, 1)});
    setPal(vdp, PAL_ROPE, {0, gs::rgb4(15, 15, 13), gs::rgb4(10, 10, 8), gs::rgb4(8, 5, 2), gs::rgb4(12, 2, 2)});
    setPal(vdp, PAL_ROPEGOLD, {0, gs::rgb4(15, 12, 3), gs::rgb4(10, 7, 1), gs::rgb4(8, 5, 2), gs::rgb4(15, 15, 10)});
    setPal(vdp, PAL_CROWD,
           {0, gs::rgb4(13, 9, 6), gs::rgb4(12, 2, 2), gs::rgb4(2, 4, 11), gs::rgb4(13, 11, 2), gs::rgb4(2, 2, 4)});
    setPal(vdp, PAL_HOUSE,
           {0, gs::rgb4(14, 13, 11), gs::rgb4(10, 9, 7), gs::rgb4(11, 2, 2), gs::rgb4(6, 1, 1), gs::rgb4(4, 3, 2),
            gs::rgb4(6, 10, 14)});
    textPal(vdp, PAL_INK, gs::rgb4(15, 15, 14), gs::rgb4(1, 1, 2));
    textPal(vdp, PAL_GOLDTEXT, gs::rgb4(15, 13, 3), gs::rgb4(3, 2, 0));
    textPal(vdp, PAL_GREEN, gs::rgb4(4, 15, 6), gs::rgb4(1, 3, 1));
    textPal(vdp, PAL_RED, gs::rgb4(15, 4, 3), gs::rgb4(3, 0, 0));

    loadFont(vdp, art);
    for (int i = 0; i < 4; i++) {
        gs::Bitmap b(40, 48);
        paintBatsman(b, i);
        art.batsman[i] = gs::uploadImage(vdp, b);
    }
    for (int i = 0; i < 3; i++) {
        gs::Bitmap b(40, 48);
        paintBowler(b, i);
        art.bowler[i] = gs::uploadImage(vdp, b);
    }
    for (int i = 0; i < 3; i++) {
        gs::Bitmap b(32, 40);
        paintUmpire(b, i);
        art.umpire[i] = gs::uploadImage(vdp, b);
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
    art.cloud = gs::uploadImage(vdp, cloudArt());
    art.sun = gs::uploadImage(vdp, sunArt());
    art.star = gs::uploadImage(vdp, starArt());
    art.blot = gs::uploadImage(vdp, blotArt());
    art.plate = gs::uploadImage(vdp, plateArt());
    art.wordWicket = phrase(vdp, "WICKET", 3);
    art.wordGold = phrase(vdp, "GOLD", 3);
    art.wordDouble = phrase(vdp, "DOUBLE", 2);
    art.wordNot = phrase(vdp, "NOT DOUBLE", 2);
    art.wordRope = phrase(vdp, "ROPE", 2);
    art.wordBowled = phrase(vdp, "BOWLED", 2);
    art.wordCream = phrase(vdp, "CREAM", 2);
}

}  // namespace wicketgold

#include "game/art.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>

namespace wicketbell {
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

void paintBowler(gs::Bitmap& b, int pose) {
    b.ellipse(24, 10, 7.2f, 7.4f, 2);
    b.rect(18, 4, 12, 4, 6);
    b.rect(17, 6, 14, 3, 3);
    b.rect(16, 16, 16, 16, 3);
    b.rect(16, 16, 5, 16, 4);
    b.rect(18, 31, 12, 8, 5);
    b.rect(18, 38, 5, 22, 5);
    b.rect(26, 38, 5, 22, 4);
    b.rect(16, 58, 8, 4, 7);
    b.rect(25, 58, 8, 4, 7);
    if (pose == 0) {
        b.line(18, 20, 8, 32, 2, 3.4f);
        b.line(30, 20, 40, 30, 2, 3.4f);
        b.ellipse(40, 30, 2.4f, 2.2f, 2);
        b.ellipse(8, 32, 2.2f, 2.f, 2);
    } else if (pose == 1) {
        b.line(20, 18, 16, 8, 2, 3.2f);
        b.line(28, 18, 34, 16, 2, 3.2f);
        b.ellipse(34, 16, 2.4f, 2.2f, 2);
        b.ellipse(16, 8, 2.2f, 2.f, 2);
    } else {
        b.line(18, 20, 10, 28, 2, 3.2f);
        b.line(28, 16, 42, 10, 2, 3.4f);
        b.ellipse(42, 10, 2.6f, 2.4f, 2);
        b.ellipse(10, 28, 2.2f, 2.f, 2);
    }
    b.outline(1, false);
}

void paintKeeper(gs::Bitmap& b) {
    b.ellipse(16, 9, 6.4f, 6.2f, 6);
    b.rect(11, 8, 10, 3, 4);
    b.rect(12, 11, 8, 3, 1);
    b.rect(12, 15, 8, 10, 3);
    b.rect(10, 24, 5, 14, 3);
    b.rect(17, 24, 5, 14, 3);
    b.rect(9, 36, 6, 4, 7);
    b.rect(17, 36, 6, 4, 7);
    b.ellipse(6, 20, 3.4f, 3.f, 5);
    b.ellipse(26, 20, 3.4f, 3.f, 5);
    b.rect(4, 18, 4, 3, 2);
    b.rect(24, 18, 4, 3, 2);
    b.outline(1, false);
}

gs::Bitmap ballArt(int seam) {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 6.4f, 6.4f, 1);
    b.ellipse(6, 6, 2.2f, 1.6f, 4);
    if (seam == 0) b.line(8, 2, 8, 14, 3, 1.3f);
    else b.line(3, 5, 13, 11, 3, 1.3f);
    return b;
}

void paintStumps(gs::Bitmap& b) {
    auto stick = [&](float x) {
        b.rect(x, 44, 9, 66, 2);
        b.rect(x + 1, 45, 7, 64, 1);
        b.rect(x + 1, 44, 7, 4, 3);
        b.rect(x + 2, 62, 5, 3, 3);
        b.rect(x + 2, 86, 5, 3, 3);
        b.rect(x + 3, 50, 2, 54, 4);
    };
    stick(kMidX - kStumpOuter);
    stick(kMidX + kStumpOuter - 8.f);
}

// Brass is filled first. The mouth is punched with 0 last, so the center
// pixel the rule samples stays open and the lip around it stays brass.
void paintBell(gs::Bitmap& b) {
    b.line(kMidX, 34.f, kMidX, kBellY - kLipRy + 2.f, 3, 2.4f);
    b.ellipse(kMidX, kBellY - kLipRy - 1.f, 3.2f, 3.f, 4);
    b.ellipse(kMidX, kBellY - 8.f, kLipR * 0.62f, 9.f, 3);
    b.ellipse(kMidX, kBellY + 2.f, kLipR, kLipRy * 0.78f, 3);
    b.ellipse(kMidX - 4.f, kBellY - 4.f, 5.f, 8.f, 4);
    b.ellipse(kMidX, kBellY + 1.f, kMouthR + 1.6f, kMouthRy + 1.4f, 1);
    b.ellipse(kMidX, kBellY, kMouthR, kMouthRy, 0);
    b.ellipse(kMidX, kBellY + 10.f, 2.2f, 2.8f, 5);
}

void paintPitch(gs::Bitmap& b) {
    b.poly({{78.f, 6.f}, {142.f, 6.f}, {208.f, 104.f}, {12.f, 104.f}}, 1);
    for (int y = 0; y < b.h; y++) {
        for (int x = 0; x < b.w; x++) {
            if (!b.get(x, y)) continue;
            uint32_t h = uint32_t(x) * 374761393u ^ uint32_t(y) * 668265263u;
            if ((h & 31u) == 0) b.set(x, y, 2);
            else if ((h & 47u) == 1) b.set(x, y, 4);
        }
    }
    b.line(86, 18, 134, 18, 3, 1.4f);
    b.line(70, 22, 70, 8, 3, 1.2f);
    b.line(150, 22, 150, 8, 3, 1.2f);
    b.line(28, 92, 192, 92, 3, 1.6f);
    b.line(36, 96, 36, 78, 3, 1.3f);
    b.line(184, 96, 184, 78, 3, 1.3f);
    b.ellipse(110, 58, 16, 8, 2);
}

gs::Bitmap screenArt() {
    gs::Bitmap b(52, 44);
    b.rect(2, 2, 48, 30, 1);
    b.rect(0, 0, 52, 4, 3);
    b.rect(0, 30, 52, 4, 3);
    b.rect(0, 0, 4, 34, 3);
    b.rect(48, 0, 4, 34, 3);
    b.rect(1, 1, 50, 2, 2);
    b.rect(10, 34, 4, 10, 4);
    b.rect(38, 34, 4, 10, 4);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(44, 40);
    b.rect(19, 24, 6, 14, 3);
    b.rect(18, 24, 2, 14, 4);
    b.ellipse(22, 16, 16, 12, 1);
    b.ellipse(14, 18, 8, 7, 2);
    b.ellipse(30, 14, 8, 6, 2);
    return b;
}

gs::Bitmap houseArt() {
    gs::Bitmap b(72, 48);
    b.poly({{2.f, 18.f}, {36.f, 3.f}, {70.f, 18.f}}, 2);
    b.rect(8, 18, 56, 24, 1);
    b.rect(8, 18, 56, 3, 3);
    b.rect(32, 26, 10, 16, 4);
    b.rect(14, 24, 10, 8, 5);
    b.rect(50, 24, 10, 8, 5);
    b.rect(6, 40, 60, 4, 3);
    b.rect(34, 4, 2, 8, 6);
    return b;
}

gs::Bitmap crowdArt() {
    gs::Bitmap b(96, 20);
    for (int i = 0; i < 7; i++) {
        int x = 2 + i * 13;
        int shirt = 2 + (i % 4);
        b.ellipse(x + 5.f, 5.f, 3.f, 3.f, 1);
        b.rect(x + 2, 9, 7, 6, shirt);
        b.rect(x + 3, 15, 2, 4, 5);
        b.rect(x + 6, 15, 2, 4, 5);
    }
    return b;
}

gs::Bitmap ropeArt() {
    gs::Bitmap b(28, 56);
    b.rect(4, 16, 4, 38, 3);
    b.rect(20, 20, 4, 34, 3);
    b.rect(3, 14, 6, 4, 2);
    b.rect(19, 18, 6, 4, 2);
    for (int i = 0; i <= 16; i++) {
        float t = i / 16.f;
        float x = 6.f + t * 14.f;
        float y = 20.f + std::sin(t * 3.14159265f) * 14.f;
        b.rect(x, y, 2.f, 2.f, 1);
    }
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
    b.ellipse(8, 8, 6.2f, 6.2f, 2);
    b.ellipse(6, 6, 2.2f, 1.6f, 3);
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
    gs::Bitmap b(18, 6);
    b.rect(1, 2, 16, 3, 1);
    b.rect(1, 1, 16, 2, 3);
    b.ellipse(3, 3, 2.f, 2.f, 2);
    b.ellipse(15, 3, 2.f, 2.f, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_INK, gs::rgb4(15, 15, 14), gs::rgb4(1, 2, 3));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 13, 4), gs::rgb4(3, 2, 1));
    textPal(vdp, PAL_GREEN, gs::rgb4(6, 15, 7), gs::rgb4(1, 3, 1));
    textPal(vdp, PAL_RED, gs::rgb4(15, 4, 3), gs::rgb4(3, 0, 0));
    textPal(vdp, PAL_TITLE, gs::rgb4(15, 13, 5), gs::rgb4(2, 3, 6));
    setPal(vdp, PAL_PITCH, {0, gs::rgb4(12, 9, 5), gs::rgb4(9, 6, 3), gs::rgb4(15, 14, 11), gs::rgb4(13, 10, 6)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(13, 9, 4), gs::rgb4(8, 5, 2), gs::rgb4(15, 13, 8), gs::rgb4(5, 3, 1)});
    setPal(vdp, PAL_BRASS,
           {0, gs::rgb4(6, 3, 1), gs::rgb4(9, 6, 2), gs::rgb4(14, 10, 3), gs::rgb4(15, 14, 7), gs::rgb4(5, 2, 1)});
    setPal(vdp, PAL_BALL, {0, gs::rgb4(13, 2, 2), gs::rgb4(8, 1, 1), gs::rgb4(15, 14, 12), gs::rgb4(15, 8, 6)});
    setPal(vdp, PAL_KIT,
           {0, gs::rgb4(1, 1, 2), gs::rgb4(13, 9, 6), gs::rgb4(15, 15, 13), gs::rgb4(2, 3, 8), gs::rgb4(14, 14, 15),
            gs::rgb4(12, 10, 6), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_KEEP,
           {0, gs::rgb4(1, 1, 2), gs::rgb4(13, 9, 6), gs::rgb4(15, 15, 14), gs::rgb4(1, 2, 6), gs::rgb4(12, 2, 2),
            gs::rgb4(2, 8, 3), gs::rgb4(3, 3, 4)});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(2, 8, 2), gs::rgb4(5, 12, 3), gs::rgb4(6, 4, 2), gs::rgb4(4, 2, 1)});
    setPal(vdp, PAL_HOUSE,
           {0, gs::rgb4(14, 13, 11), gs::rgb4(10, 3, 2), gs::rgb4(6, 3, 2), gs::rgb4(5, 2, 1), gs::rgb4(6, 10, 14),
            gs::rgb4(3, 3, 4)});
    setPal(vdp, PAL_CROWD,
           {0, gs::rgb4(13, 9, 6), gs::rgb4(12, 2, 2), gs::rgb4(2, 4, 11), gs::rgb4(13, 11, 3), gs::rgb4(2, 2, 4)});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(15, 15, 15), gs::rgb4(15, 14, 8), gs::rgb4(15, 12, 3), gs::rgb4(12, 13, 15)});
    setPal(vdp, PAL_WHITE, {0, gs::rgb4(15, 15, 15), gs::rgb4(12, 12, 13), gs::rgb4(7, 7, 8), gs::rgb4(4, 4, 5)});

    loadFont(vdp, art);
    for (int i = 0; i < 3; i++) {
        gs::Bitmap b(48, 68);
        paintBowler(b, i);
        art.bowler[i] = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(32, 44);
        paintKeeper(b);
        art.keeper = gs::uploadImage(vdp, b);
    }
    art.ball[0] = gs::uploadImage(vdp, ballArt(0));
    art.ball[1] = gs::uploadImage(vdp, ballArt(1));
    {
        gs::Bitmap st(kBmpW, kBmpH);
        paintStumps(st);
        art.stumps = gs::uploadImage(vdp, st);
        gs::Bitmap be(kBmpW, kBmpH);
        paintBell(be);
        art.bell = gs::uploadImage(vdp, be);
    }
    art.bail = gs::uploadImage(vdp, bailArt());
    {
        gs::Bitmap b(220, 110);
        paintPitch(b);
        art.pitch = gs::uploadImage(vdp, b);
    }
    art.screen = gs::uploadImage(vdp, screenArt());
    art.tree = gs::uploadImage(vdp, treeArt());
    art.house = gs::uploadImage(vdp, houseArt());
    art.crowd = gs::uploadImage(vdp, crowdArt());
    art.rope = gs::uploadImage(vdp, ropeArt());
    art.cloud = gs::uploadImage(vdp, cloudArt());
    art.sun = gs::uploadImage(vdp, sunArt());
    art.shadow = gs::uploadImage(vdp, shadowArt());
    art.blot = gs::uploadImage(vdp, blotArt());
    art.title = phrase(vdp, "WICKET BELL", 2);
    art.three = phrase(vdp, "THREE TRIES", 2);
    art.theBell = phrase(vdp, "THE BELL", 2);
    art.tryDied = phrase(vdp, "THIRD TRY DIED", 2);
    art.leave = phrase(vdp, "LEAVE", 2);
}

}  // namespace wicketbell

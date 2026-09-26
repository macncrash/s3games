#include "game/art.h"

#include <cmath>
#include <initializer_list>

namespace wicketchime {
namespace {

constexpr float kTau = 6.2831853f;

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

void shaft(gs::Bitmap& b, float theta, float len, float tail, float w, int col) {
    const float cx = kDial * 0.5f, cy = kDial * 0.5f;
    float dx = std::sin(theta), dy = -std::cos(theta);
    float px = -dy, py = dx;
    float x0 = cx - dx * tail, y0 = cy - dy * tail;
    float x1 = cx + dx * len, y1 = cy + dy * len;
    b.poly({{x0 + px * w * 0.4f, y0 + py * w * 0.4f},
            {x1 + px * w, y1 + py * w},
            {x1 - px * w, y1 - py * w},
            {x0 - px * w * 0.4f, y0 - py * w * 0.4f}},
           col);
}

gs::Bitmap handArt(int kind, int step) {
    gs::Bitmap b(kDial, kDial);
    float theta = step * (kTau / 60.f);
    if (kind == 0) {
        shaft(b, theta, 12.f, 4.f, 3.1f, 1);
        shaft(b, theta, 10.5f, 2.2f, 1.3f, 2);
    } else if (kind == 1) {
        shaft(b, theta, 17.5f, 4.5f, 1.35f, 3);
    } else {
        shaft(b, theta, 20.f, 6.f, 0.55f, 4);
        shaft(b, theta + kTau * 0.5f, 5.5f, 0.f, 1.3f, 4);
    }
    return b;
}

gs::Bitmap faceArt() {
    gs::Bitmap b(kDial, kDial);
    const float c = kDial * 0.5f;
    b.ellipse(c, c, c - 1.2f, c - 1.2f, 1);
    b.ellipse(c, c, c - 4.2f, c - 4.2f, 2);
    b.ellipse(c, c, c - 6.4f, c - 6.4f, 3);
    for (int i = 0; i < 60; i += 5) {
        float a = i * (kTau / 60.f);
        float s = std::sin(a), co = std::cos(a);
        int col = (i % 15 == 0) ? 6 : 5;
        float inner = (i % 15 == 0) ? 15.5f : 17.2f;
        b.line(c + s * inner, c - co * inner, c + s * 21.2f, c - co * 21.2f, col, i % 15 == 0 ? 1.8f : 1.f);
    }
    auto stamp = [&](const char* t, int x, int y) {
        gs::Bitmap num = gs::textBitmap(t, gs::TextStyle{1, 7, 0, 0, 0});
        b.blit(num, x - num.w / 2, y);
    };
    stamp("12", int(c), 6);
    stamp("3", int(c) + 14, int(c) - 4);
    stamp("6", int(c), int(kDial) - 14);
    stamp("9", int(c) - 18, int(c) - 4);
    return b;
}

gs::Bitmap ringArt() {
    gs::Bitmap b(kDial, kDial);
    const float c = kDial * 0.5f;
    b.ellipse(c, c, c - 0.8f, c - 0.8f, 1);
    b.ellipse(c, c, c - 4.6f, c - 4.6f, 0);
    return b;
}

gs::Bitmap capArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 6.2f, 6.2f, 1);
    b.ellipse(8, 8, 2.6f, 2.6f, 2);
    return b;
}

gs::Bitmap ballArt(int seam) {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 6.5f, 6.5f, 2);
    b.ellipse(6.2f, 5.8f, 2.1f, 1.5f, 4);
    if (seam == 0) b.line(8, 2.4f, 8, 13.6f, 3, 1.2f);
    else b.line(3.2f, 5.f, 12.8f, 11.f, 3, 1.2f);
    b.outline(1, false);
    return b;
}

gs::Bitmap bailArt() {
    gs::Bitmap b(24, 8);
    b.rect(2, 2, 20, 3, 5);
    b.rect(2, 2, 20, 1, 6);
    b.ellipse(3.5f, 4.f, 2.2f, 2.2f, 3);
    b.ellipse(20.5f, 4.f, 2.2f, 2.2f, 3);
    b.outline(1, false);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(16, 18);
    b.rect(7, 0, 2, 4, 7);
    b.poly({{3.f, 6.f}, {13.f, 6.f}, {15.f, 14.f}, {1.f, 14.f}}, 6);
    b.rect(2, 13, 12, 2, 8);
    b.ellipse(8, 16, 1.6f, 1.6f, 7);
    b.line(5, 8, 6, 12, 8, 1.f);
    b.outline(1, false);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(24, 10);
    b.ellipse(12, 5, 10, 3.2f, 1);
    return b;
}

gs::Bitmap blotArt() {
    gs::Bitmap b(12, 8);
    b.rect(0, 0, 12, 8, 1);
    return b;
}

void paintBowler(gs::Bitmap& b, int pose) {
    b.ellipse(22, 11, 7.2f, 7.4f, 2);
    b.rect(16, 5, 12, 5, 7);
    b.rect(15, 9, 14, 3, 3);
    b.rect(15, 18, 16, 16, 3);
    b.rect(16, 18, 5, 16, 4);
    b.rect(17, 33, 6, 22, 5);
    b.rect(26, 33, 6, 22, 6);
    b.rect(15, 53, 9, 5, 8);
    b.rect(25, 53, 9, 5, 8);
    if (pose == 0) {
        b.line(18, 22, 8, 34, 2, 3.2f);
        b.line(30, 22, 40, 36, 2, 3.4f);
        b.ellipse(40, 40, 2.6f, 2.4f, 2);
        b.ellipse(8, 34, 2.2f, 2.f, 2);
    } else if (pose == 1) {
        b.line(18, 22, 10, 30, 2, 3.2f);
        b.line(28, 20, 34, 14, 2, 3.2f);
        b.ellipse(34, 12, 2.6f, 2.4f, 2);
        b.ellipse(10, 30, 2.2f, 2.f, 2);
    } else {
        b.line(16, 24, 8, 34, 2, 3.2f);
        b.line(28, 20, 44, 18, 2, 3.4f);
        b.ellipse(44, 20, 2.7f, 2.4f, 2);
        b.ellipse(8, 34, 2.2f, 2.f, 2);
    }
    b.outline(1, false);
}

void paintBatsman(gs::Bitmap& b, int pose) {
    b.ellipse(20, 11, 7.f, 7.2f, 2);
    b.rect(13, 4, 14, 5, 4);
    b.rect(12, 8, 16, 3, 5);
    b.rect(13, 18, 16, 15, 3);
    b.rect(13, 18, 4, 15, 4);
    b.rect(12, 33, 8, 20, 6);
    b.rect(22, 33, 8, 20, 6);
    b.rect(13, 36, 6, 14, 7);
    b.rect(23, 36, 6, 14, 7);
    b.rect(11, 51, 10, 5, 8);
    b.rect(21, 51, 10, 5, 8);
    if (pose == 0) {
        b.rect(8, 24, 4, 28, 9);
        b.rect(9, 20, 3, 8, 10);
    } else if (pose == 1) {
        b.line(18, 28, 4, 16, 9, 4.2f);
        b.line(20, 30, 10, 22, 10, 2.f);
    } else {
        b.line(22, 22, 36, 8, 9, 4.f);
        b.line(20, 24, 30, 14, 10, 2.f);
        b.rect(12, 20, 12, 12, 3);
    }
    b.outline(1, false);
}

void paintKeeper(gs::Bitmap& b) {
    b.ellipse(16, 10, 6.f, 6.f, 2);
    b.rect(11, 6, 10, 4, 4);
    b.rect(12, 16, 10, 10, 3);
    b.rect(8, 24, 7, 12, 6);
    b.rect(18, 24, 7, 12, 6);
    b.ellipse(6, 20, 3.4f, 3.f, 9);
    b.ellipse(26, 20, 3.4f, 3.f, 9);
    b.rect(7, 34, 7, 4, 8);
    b.rect(18, 34, 7, 4, 8);
    b.outline(1, false);
}

gs::Bitmap stumpArt() {
    gs::Bitmap b(54, 72);
    auto stick = [&](float x) {
        b.rect(x, 6, 8, 62, 2);
        b.rect(x + 1, 8, 6, 58, 3);
        b.rect(x + 2, 10, 2, 52, 4);
        b.rect(x, 6, 8, 4, 5);
        b.rect(x + 1, 24, 6, 2, 1);
        b.rect(x + 1, 42, 6, 2, 1);
    };
    stick(3);
    stick(23);
    stick(43);
    return b;
}

gs::Bitmap pitchArt() {
    gs::Bitmap b(180, 120);
    b.poly({{58.f, 8.f}, {122.f, 8.f}, {172.f, 112.f}, {8.f, 112.f}}, 4);
    b.poly({{70.f, 16.f}, {110.f, 16.f}, {148.f, 104.f}, {32.f, 104.f}}, 1);
    for (int y = 0; y < b.h; y++) {
        for (int x = 0; x < b.w; x++) {
            if (b.get(x, y) != 1) continue;
            uint32_t h = uint32_t(x) * 374761393u ^ uint32_t(y) * 668265263u;
            if ((h & 23u) == 0) b.set(x, y, 2);
            else if ((h & 41u) == 1) b.set(x, y, 5);
        }
    }
    b.line(74, 22, 106, 22, 3, 1.5f);
    b.line(78, 26, 78, 16, 3, 1.2f);
    b.line(102, 26, 102, 16, 3, 1.2f);
    b.line(64, 48, 116, 48, 3, 1.6f);
    b.line(70, 52, 70, 40, 3, 1.2f);
    b.line(110, 52, 110, 40, 3, 1.2f);
    b.ellipse(90, 64, 10, 5, 2);
    return b;
}

gs::Bitmap towerArt() {
    gs::Bitmap b(72, 112);
    b.rect(10, 18, 52, 86, 1);
    b.rect(10, 18, 52, 6, 3);
    b.rect(8, 100, 56, 8, 2);
    for (int i = 0; i < 5; i++) b.rect(10 + i * 12, 8, 8, 12, 1);
    b.rect(12, 6, 4, 8, 4);
    b.rect(34, 2, 3, 14, 2);
    b.poly({{30.f, 4.f}, {46.f, 8.f}, {30.f, 12.f}}, 4);
    b.ellipse(36, 40, 16, 16, 2);
    b.rect(30, 78, 12, 22, 5);
    b.rect(16, 58, 10, 12, 2);
    b.rect(46, 58, 10, 12, 2);
    b.rect(18, 66, 6, 4, 3);
    b.rect(48, 66, 6, 4, 3);
    b.outline(1, false);
    return b;
}

gs::Bitmap screenArt() {
    gs::Bitmap b(80, 36);
    b.rect(2, 2, 76, 26, 6);
    b.rect(0, 0, 80, 4, 7);
    b.rect(0, 26, 80, 4, 7);
    b.rect(0, 0, 4, 30, 7);
    b.rect(76, 0, 4, 30, 7);
    b.rect(18, 30, 4, 6, 8);
    b.rect(58, 30, 4, 6, 8);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(48, 44);
    b.rect(21, 26, 6, 16, 4);
    b.rect(20, 26, 2, 16, 5);
    b.ellipse(24, 16, 18, 13, 1);
    b.ellipse(14, 18, 9, 8, 2);
    b.ellipse(32, 14, 9, 7, 3);
    return b;
}

gs::Bitmap houseArt() {
    gs::Bitmap b(78, 52);
    b.poly({{2.f, 20.f}, {39.f, 4.f}, {76.f, 20.f}}, 7);
    b.rect(8, 20, 62, 26, 6);
    b.rect(8, 20, 62, 3, 8);
    b.rect(34, 28, 12, 18, 9);
    b.rect(14, 26, 12, 10, 10);
    b.rect(52, 26, 12, 10, 10);
    b.rect(6, 44, 66, 4, 8);
    b.rect(48, 8, 3, 10, 2);
    return b;
}

gs::Bitmap crowdArt() {
    gs::Bitmap b(104, 22);
    for (int i = 0; i < 8; i++) {
        int x = 2 + i * 13;
        int shirt = 2 + (i % 4);
        b.ellipse(x + 5.f, 5.f, 3.2f, 3.f, 1);
        b.rect(x + 2, 9, 7, 7, shirt);
        b.rect(x + 3, 16, 2, 5, 5);
        b.rect(x + 6, 16, 2, 5, 5);
    }
    return b;
}

gs::Bitmap ropeArt() {
    gs::Bitmap b(16, 48);
    b.rect(6, 8, 4, 38, 8);
    b.rect(5, 6, 6, 4, 7);
    b.ellipse(8, 6, 3.f, 2.4f, 5);
    for (int i = 0; i < 4; i++) b.line(1, 14 + i * 8, 14, 12 + i * 8, 5, 1.3f);
    return b;
}

gs::Bitmap cloudArt() {
    gs::Bitmap b(40, 16);
    b.ellipse(14, 9, 10, 6, 1);
    b.ellipse(24, 8, 12, 7, 1);
    b.ellipse(20, 10, 8, 4, 2);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(18, 18);
    b.ellipse(9, 9, 5.5f, 5.5f, 3);
    b.ellipse(9, 9, 3.2f, 3.2f, 4);
    for (int i = 0; i < 8; i++) {
        float a = i * (kTau / 8.f);
        b.line(9 + std::cos(a) * 6.2f, 9 + std::sin(a) * 6.2f, 9 + std::cos(a) * 8.4f, 9 + std::sin(a) * 8.4f, 4, 1.2f);
    }
    return b;
}

gs::Image phrase(gs::VDP& vdp, const char* s, int scale) {
    return gs::uploadImage(vdp, gs::textBitmap(s, {scale, 1, 15, 0, 1}));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_INK, gs::rgb4(15, 15, 14), gs::rgb4(1, 2, 3));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 13, 4), gs::rgb4(3, 2, 1));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 5, 3), gs::rgb4(3, 0, 0));
    textPal(vdp, PAL_GREEN, gs::rgb4(6, 15, 7), gs::rgb4(1, 3, 1));
    setPal(vdp, PAL_BALL, {0, gs::rgb4(4, 1, 1), gs::rgb4(13, 2, 2), gs::rgb4(15, 14, 12), gs::rgb4(15, 9, 7)});
    setPal(vdp, PAL_BAT,
           {0, gs::rgb4(1, 1, 2), gs::rgb4(13, 9, 6), gs::rgb4(15, 15, 13), gs::rgb4(2, 3, 8), gs::rgb4(5, 7, 13),
            gs::rgb4(14, 14, 15), gs::rgb4(9, 10, 12), gs::rgb4(3, 2, 2), gs::rgb4(12, 9, 5), gs::rgb4(6, 4, 2)});
    setPal(vdp, PAL_BOWL,
           {0, gs::rgb4(1, 1, 2), gs::rgb4(13, 9, 6), gs::rgb4(11, 2, 3), gs::rgb4(7, 1, 2), gs::rgb4(15, 15, 14),
            gs::rgb4(12, 12, 13), gs::rgb4(4, 3, 2), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_WOOD,
           {0, gs::rgb4(5, 3, 1), gs::rgb4(8, 5, 2), gs::rgb4(13, 9, 4), gs::rgb4(15, 13, 8), gs::rgb4(14, 12, 7),
            gs::rgb4(15, 14, 10)});
    setPal(vdp, PAL_PITCH,
           {0, gs::rgb4(12, 9, 5), gs::rgb4(9, 6, 3), gs::rgb4(15, 15, 13), gs::rgb4(4, 10, 3), gs::rgb4(13, 11, 7)});
    setPal(vdp, PAL_TOWER,
           {0, gs::rgb4(10, 10, 11), gs::rgb4(6, 6, 7), gs::rgb4(13, 13, 14), gs::rgb4(8, 8, 9), gs::rgb4(4, 3, 3),
            gs::rgb4(13, 10, 3), gs::rgb4(6, 4, 1), gs::rgb4(15, 13, 6)});
    setPal(vdp, PAL_FACE,
           {0, gs::rgb4(4, 3, 2), gs::rgb4(8, 6, 3), gs::rgb4(15, 14, 11), gs::rgb4(12, 10, 7), gs::rgb4(5, 4, 3),
            gs::rgb4(12, 2, 2), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_HAND,
           {0, gs::rgb4(2, 2, 3), gs::rgb4(14, 11, 4), gs::rgb4(15, 15, 13), gs::rgb4(13, 2, 2), gs::rgb4(9, 8, 7)});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(2, 7, 2), gs::rgb4(4, 11, 3), gs::rgb4(7, 13, 4), gs::rgb4(6, 4, 2), gs::rgb4(4, 2, 1)});
    setPal(vdp, PAL_SKY,
           {0, gs::rgb4(15, 15, 15), gs::rgb4(13, 14, 15), gs::rgb4(15, 14, 6), gs::rgb4(15, 12, 3), gs::rgb4(15, 15, 14),
            gs::rgb4(14, 14, 15), gs::rgb4(7, 6, 5), gs::rgb4(5, 4, 3)});
    setPal(vdp, PAL_CROWD,
           {0, gs::rgb4(13, 9, 6), gs::rgb4(12, 2, 2), gs::rgb4(2, 4, 11), gs::rgb4(13, 11, 3), gs::rgb4(3, 3, 4),
            gs::rgb4(14, 13, 11), gs::rgb4(10, 3, 2), gs::rgb4(6, 3, 2), gs::rgb4(4, 2, 1), gs::rgb4(6, 10, 14)});
    setPal(vdp, PAL_SHADE, {0, gs::rgb4(2, 3, 1)});

    loadFont(vdp, art);
    for (int k = 0; k < 3; k++)
        for (int s = 0; s < 60; s++) art.hand[k][s] = gs::uploadImage(vdp, handArt(k, s));
    art.face = gs::uploadImage(vdp, faceArt());
    art.ring = gs::uploadImage(vdp, ringArt());
    art.cap = gs::uploadImage(vdp, capArt());
    art.ball[0] = gs::uploadImage(vdp, ballArt(0));
    art.ball[1] = gs::uploadImage(vdp, ballArt(1));
    art.bail = gs::uploadImage(vdp, bailArt());
    art.bell = gs::uploadImage(vdp, bellArt());
    art.shadow = gs::uploadImage(vdp, shadowArt());
    art.blot = gs::uploadImage(vdp, blotArt());
    art.title = phrase(vdp, "SHORT WICKET", 2);
    for (int i = 0; i < 3; i++) {
        gs::Bitmap bowl(48, 72);
        paintBowler(bowl, i);
        art.bowler[i] = gs::uploadMipped(vdp, bowl);
        gs::Bitmap bat(44, 64);
        paintBatsman(bat, i);
        art.batsman[i] = gs::uploadMipped(vdp, bat);
    }
    {
        gs::Bitmap k(32, 42);
        paintKeeper(k);
        art.keeper = gs::uploadMipped(vdp, k);
    }
    art.stumps = gs::uploadMipped(vdp, stumpArt());
    art.pitch = gs::uploadMipped(vdp, pitchArt());
    art.tower = gs::uploadMipped(vdp, towerArt());
    art.screen = gs::uploadMipped(vdp, screenArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.house = gs::uploadMipped(vdp, houseArt());
    art.crowd = gs::uploadMipped(vdp, crowdArt());
    art.rope = gs::uploadMipped(vdp, ropeArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
}

}  // namespace wicketchime

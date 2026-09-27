#include "game/art.h"

#include <cmath>
#include <initializer_list>

namespace golfbell {
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
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
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

int coursePix(int x, int y) {
    float gy = groundY(float(x) + 0.5f);
    float py = float(y) + 0.5f;
    bool mouth = std::fabs(float(x) + 0.5f - kCupX) <= kCupHalf;
    if (mouth && py >= kGround - 1.f && py < kGround + kCupDepth) return 8;
    if (py < gy - 0.5f) return 0;
    float wx = float(x);
    bool green = wx > 188.f && wx < 300.f;
    bool sand = wx > 150.f && wx < 178.f;
    int cap = green ? 3 : (sand ? 6 : 1);
    int body = green ? 2 : (sand ? 5 : 4);
    if (py < gy + 2.f) return cap;
    if (((x + y) & 7) == 0) return green ? 3 : body;
    return body;
}

gs::Bitmap courseArt() {
    gs::Bitmap b(320, 96);
    for (int y = 0; y < b.h; y++)
        for (int x = 0; x < b.w; x++) b.set(x, y, coursePix(x, y + 136));
    return b;
}

gs::Bitmap ballArt() {
    gs::Bitmap b(9, 9);
    b.ellipse(4.f, 4.f, 3.6f, 3.6f, 1);
    b.ellipse(3.f, 3.f, 1.2f, 1.2f, 2);
    return b;
}

gs::Bitmap golferArt() {
    gs::Bitmap b(20, 30);
    b.ellipse(8.f, 5.f, 4.f, 4.f, 1);
    b.rect(6.f, 9.f, 5.f, 10.f, 2);
    b.rect(2.f, 11.f, 5.f, 3.f, 3);
    b.rect(10.f, 13.f, 8.f, 2.f, 4);
    b.rect(5.f, 18.f, 3.f, 9.f, 2);
    b.rect(9.f, 18.f, 3.f, 9.f, 2);
    b.ellipse(17.f, 14.f, 2.f, 2.f, 5);
    return b;
}

gs::Bitmap flagArt() {
    gs::Bitmap b(18, 36);
    b.rect(1.f, 4.f, 2.f, 30.f, 1);
    b.poly({{3.f, 5.f}, {16.f, 10.f}, {3.f, 15.f}}, 2);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(16, 18);
    b.rect(7.f, 0.f, 2.f, 3.f, 2);
    b.poly({{3.f, 5.f}, {13.f, 5.f}, {15.f, 15.f}, {1.f, 15.f}}, 1);
    b.rect(2.f, 14.f, 12.f, 2.f, 3);
    b.ellipse(8.f, 11.f, 2.f, 2.f, 0);
    return b;
}

gs::Bitmap titleArt() { return gs::textBitmap("S3 GOLFBELL", {2, 1, 2, 0, 1}); }

gs::Bitmap leaveArt() { return gs::textBitmap("BELL", {2, 1, 2, 0, 1}); }

}  // namespace

float groundY(float x) {
    if (std::fabs(x - kCupX) <= kCupHalf) return kGround + kCupDepth;
    if (x > 190.f && x < 302.f) {
        float d = std::fabs(x - kCupX);
        float span = std::min(d, 56.f);
        return kGround - 8.f + (56.f - span) * 0.12f;
    }
    if (x > 150.f && x < 178.f) return kGround + 6.f;
    return kGround;
}

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_COURSE,
           {0, gs::rgb4(2, 9, 3), gs::rgb4(1, 8, 2), gs::rgb4(4, 13, 5), gs::rgb4(3, 6, 2), gs::rgb4(12, 10, 4),
            gs::rgb4(14, 12, 6), gs::rgb4(6, 4, 2), gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_BALL, {0, gs::rgb4(15, 15, 14), gs::rgb4(12, 12, 11)});
    setPal(vdp, PAL_GOLFER,
           {0, gs::rgb4(13, 9, 6), gs::rgb4(2, 4, 9), gs::rgb4(14, 12, 8), gs::rgb4(9, 6, 3), gs::rgb4(15, 15, 14)});
    setPal(vdp, PAL_FLAG, {0, gs::rgb4(14, 14, 13), gs::rgb4(13, 2, 3)});
    setPal(vdp, PAL_BELL, {0, gs::rgb4(14, 12, 4), gs::rgb4(8, 7, 3), gs::rgb4(15, 14, 8)});
    textPal(vdp, PAL_INK, gs::rgb4(15, 15, 14), gs::rgb4(1, 1, 2));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 13, 4), gs::rgb4(2, 1, 0));
    textPal(vdp, PAL_GREEN, gs::rgb4(5, 15, 7), gs::rgb4(1, 2, 1));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 5, 4), gs::rgb4(2, 0, 0));
    textPal(vdp, PAL_AIM, gs::rgb4(15, 12, 3), gs::rgb4(2, 1, 0));
    textPal(vdp, PAL_TITLE, gs::rgb4(15, 14, 6), gs::rgb4(3, 1, 1));
    textPal(vdp, PAL_LEAVE, gs::rgb4(15, 15, 12), gs::rgb4(1, 4, 1));

    loadFont(vdp, art);
    art.course = gs::uploadImage(vdp, courseArt());
    art.ball = gs::uploadImage(vdp, ballArt());
    art.golfer = gs::uploadImage(vdp, golferArt());
    art.flag = gs::uploadImage(vdp, flagArt());
    art.bell = gs::uploadImage(vdp, bellArt());
    gs::Bitmap clap(5, 8);
    clap.rect(2.f, 0.f, 1.f, 6.f, 1);
    clap.ellipse(2.f, 6.f, 2.f, 2.f, 2);
    art.clapper = gs::uploadImage(vdp, clap);
    gs::Bitmap pip(4, 4);
    pip.ellipse(1.5f, 1.5f, 1.6f, 1.6f, 1);
    art.pip = gs::uploadImage(vdp, pip);
    gs::Bitmap dot(3, 3);
    dot.ellipse(1.f, 1.f, 1.2f, 1.2f, 1);
    art.dot = gs::uploadImage(vdp, dot);
    art.title = gs::uploadImage(vdp, titleArt());
    art.leave = gs::uploadImage(vdp, leaveArt());
}

}  // namespace golfbell

#include "art.h"

#include <cmath>
#include <initializer_list>

namespace bikemark {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
    while (i < 15) vdp.setColor(pal * 16 + i++, 0);
    vdp.setColor(pal * 16 + 0, 0);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 1));
}

gs::Bitmap frameArt() {
    gs::Bitmap b(88, 48);
    b.line(16, 34, 44, 18, 1, 2.4f);
    b.line(44, 18, 72, 34, 1, 2.4f);
    b.line(44, 18, 38, 34, 2, 2.0f);
    b.line(24, 34, 58, 34, 3, 2.2f);
    b.rect(40, 12, 4, 8, 4);
    b.rect(36, 10, 14, 3, 4);
    b.line(66, 34, 80, 26, 5, 1.8f);
    b.ellipse(18, 36, 3, 3, 6);
    b.outline(15, false);
    return b;
}

gs::Bitmap wheelArt(int phase) {
    gs::Bitmap b(26, 26);
    b.ellipse(13, 13, 11, 11, 1);
    b.ellipse(13, 13, 8, 8, 2);
    b.ellipse(13, 13, 2.2f, 2.2f, 3);
    float a0 = phase ? 0.55f : 0.1f;
    for (int i = 0; i < 3; i++) {
        float a = a0 + i * 2.0944f;
        b.line(13, 13, 13 + std::cos(a) * 8.f, 13 + std::sin(a) * 8.f, 4, 1.2f);
    }
    return b;
}

gs::Bitmap standArt() {
    gs::Bitmap b(18, 22);
    b.line(4, 2, 14, 18, 1, 2.0f);
    b.rect(11, 16, 6, 3, 2);
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(14, 8);
    b.ellipse(7, 4, 5, 2.4f, 1);
    b.ellipse(4, 4, 1.6f, 1.1f, 2);
    return b;
}

gs::Bitmap roadArt() {
    gs::Bitmap b(48, 24);
    b.rect(0, 0, 48, 24, 1);
    for (int x = 1; x < 46; x += 7) b.rect(x, 4, 3, 2, 2);
    b.rect(0, 0, 48, 2, 3);
    b.rect(0, 21, 48, 3, 4);
    return b;
}

gs::Bitmap paintArt() {
    gs::Bitmap b(40, 14);
    b.rect(0, 3, 40, 8, 1);
    for (int x = 0; x < 40; x += 6) b.rect(x, 4, 3, 6, 2);
    b.rect(0, 1, 40, 2, 3);
    b.rect(0, 11, 40, 2, 3);
    return b;
}

gs::Bitmap crossArt() {
    gs::Bitmap b(22, 22);
    b.line(3, 3, 18, 18, 1, 2.4f);
    b.line(18, 3, 3, 18, 1, 2.4f);
    b.ellipse(11, 11, 3, 3, 2);
    return b;
}

gs::Bitmap pegArt() {
    gs::Bitmap b(8, 40);
    b.rect(3, 6, 2, 32, 1);
    b.poly({{1.f, 8.f}, {4.f, 1.f}, {7.f, 8.f}}, 2);
    b.rect(2, 34, 4, 4, 3);
    return b;
}

gs::Bitmap yardArt() {
    gs::Bitmap b(64, 36);
    b.rect(2, 14, 60, 20, 1);
    b.rect(6, 18, 10, 8, 2);
    b.rect(26, 18, 10, 8, 2);
    b.rect(46, 20, 10, 14, 3);
    b.rect(0, 12, 64, 3, 4);
    b.outline(15, false);
    return b;
}

gs::Bitmap clockArt(int hand) {
    gs::Bitmap b(30, 30);
    b.rect(2, 2, 26, 26, 1);
    b.rect(5, 5, 20, 20, 2);
    b.ellipse(15, 15, 2, 2, 3);
    float a = hand * 0.785398f - 1.5708f;
    b.line(15, 15, 15 + std::cos(a) * 8.f, 15 + std::sin(a) * 8.f, 4, 1.5f);
    b.outline(15, false);
    return b;
}

gs::Bitmap crewArt(bool step) {
    gs::Bitmap b(20, 32);
    b.ellipse(10, 6, 4, 4, 1);
    b.rect(7, 11, 6, 9, 2);
    b.line(7, 13, step ? 2.f : 4.f, 19, 3, 1.4f);
    b.line(13, 13, step ? 18.f : 16.f, 19, 3, 1.4f);
    b.line(8, 20, step ? 5.f : 7.f, 30, 4, 1.5f);
    b.line(12, 20, step ? 15.f : 13.f, 30, 4, 1.5f);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(10, 32);
    b.rect(4, 10, 2, 20, 1);
    b.ellipse(5, 6, 4, 4, 2);
    b.ellipse(5, 6, 2, 2, 3);
    return b;
}

gs::Bitmap hedgeArt() {
    gs::Bitmap b(30, 22);
    b.ellipse(15, 12, 13, 8, 1);
    b.ellipse(8, 11, 6, 5, 2);
    b.rect(12, 16, 4, 5, 3);
    return b;
}

gs::Bitmap wordArt(const char* s, int scale, int color) {
    gs::TextStyle st{scale, color, 0, 0, 1};
    gs::Bitmap t = gs::textBitmap(s, st);
    gs::Bitmap b(t.w + 8, t.h + 8);
    b.blit(t, 4, 4);
    b.outline(15, false);
    return b;
}

void loadFont(gs::VDP& vdp, Art& art) {
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
        art.font[c - 32] = t;
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 13, 10), gs::rgb4(5, 5, 5)});
    setPal(vdp, PAL_BIKE, {0, gs::rgb4(3, 4, 6), gs::rgb4(9, 7, 4), gs::rgb4(6, 6, 7), gs::rgb4(13, 5, 2),
                           gs::rgb4(12, 11, 8), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_RIDER, {0, gs::rgb4(11, 4, 3), gs::rgb4(13, 10, 7)});
    setPal(vdp, PAL_ROAD, {0, gs::rgb4(4, 4, 5), gs::rgb4(6, 6, 6), gs::rgb4(9, 8, 6), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(14, 11, 2), gs::rgb4(15, 14, 6), gs::rgb4(8, 5, 1)});
    setPal(vdp, PAL_STAND, {0, gs::rgb4(7, 7, 8), gs::rgb4(12, 10, 4)});
    setPal(vdp, PAL_CREW, {0, gs::rgb4(12, 8, 5), gs::rgb4(3, 5, 9), gs::rgb4(13, 12, 5), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_CLOCK, {0, gs::rgb4(11, 9, 6), gs::rgb4(15, 14, 11), gs::rgb4(2, 2, 2), gs::rgb4(13, 3, 2)});
    setPal(vdp, PAL_YARD, {0, gs::rgb4(6, 5, 4), gs::rgb4(10, 9, 7), gs::rgb4(4, 3, 3), gs::rgb4(8, 3, 2),
                           gs::rgb4(2, 5, 3)});
    setPal(vdp, PAL_DUST, {0, gs::rgb4(9, 8, 6), gs::rgb4(13, 12, 9)});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(5, 13, 6), gs::rgb4(12, 15, 9)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(15, 11, 5)});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(14, 11, 4), gs::rgb4(7, 9, 13)});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(8, 6, 10)});

    art.frame = gs::uploadMipped(vdp, frameArt());
    art.wheel[0] = gs::uploadMipped(vdp, wheelArt(0));
    art.wheel[1] = gs::uploadMipped(vdp, wheelArt(1));
    art.stand = gs::uploadMipped(vdp, standArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.road = gs::uploadMipped(vdp, roadArt());
    art.paint = gs::uploadMipped(vdp, paintArt());
    art.cross = gs::uploadMipped(vdp, crossArt());
    art.peg = gs::uploadMipped(vdp, pegArt());
    art.yard = gs::uploadMipped(vdp, yardArt());
    for (int i = 0; i < 8; i++) art.clock[i] = gs::uploadMipped(vdp, clockArt(i));
    art.crew[0] = gs::uploadMipped(vdp, crewArt(false));
    art.crew[1] = gs::uploadMipped(vdp, crewArt(true));
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.hedge = gs::uploadMipped(vdp, hedgeArt());
    art.title = gs::uploadMipped(vdp, wordArt("BIKE", 3, 1));
    art.markWord = gs::uploadMipped(vdp, wordArt("MARK", 3, 1));
    art.setWord = gs::uploadMipped(vdp, wordArt("SET DOWN", 2, 1));
    art.offWord = gs::uploadMipped(vdp, wordArt("OFF MARK", 2, 1));
    art.late = gs::uploadMipped(vdp, wordArt("TOO LATE", 2, 1));
    art.paused = gs::uploadMipped(vdp, wordArt("PAUSED", 2, 1));
    loadFont(vdp, art);
}

}  // namespace bikemark

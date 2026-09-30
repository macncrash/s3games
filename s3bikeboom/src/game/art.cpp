#include "art.h"

#include <cmath>
#include <initializer_list>

namespace bikeboom {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
    while (i < 15) vdp.setColor(pal * 16 + i++, 0);
    vdp.setColor(pal * 16 + 0, 0);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

gs::Bitmap bikeArt() {
    gs::Bitmap b(104, 64);
    b.line(16, 46, 46, 24, 1, 2.4f);
    b.line(46, 24, 84, 46, 1, 2.4f);
    b.line(46, 24, 40, 46, 2, 2.0f);
    b.line(28, 46, 70, 46, 2, 2.6f);
    b.rect(44, 16, 4, 12, 3);
    b.rect(38, 13, 16, 4, 3);
    b.rect(62, 34, 16, 7, 6);
    b.rect(64, 36, 12, 3, 7);
    b.ellipse(22, 48, 13, 13, 4);
    b.ellipse(80, 48, 13, 13, 4);
    b.ellipse(22, 48, 3, 3, 5);
    b.ellipse(80, 48, 3, 3, 5);
    b.ellipse(40, 22, 5, 5, 8);
    b.rect(36, 26, 7, 12, 9);
    b.line(36, 28, 28, 36, 9, 2.0f);
    b.line(43, 30, 52, 38, 9, 2.0f);
    b.line(38, 38, 34, 50, 10, 2.0f);
    b.line(42, 38, 48, 50, 10, 2.0f);
    b.set(38, 21, 11);
    b.set(42, 21, 11);
    b.outline(15, false);
    return b;
}

gs::Bitmap wheelArt(int spokes) {
    gs::Bitmap b(30, 30);
    b.ellipse(15, 15, 13, 13, 1);
    b.ellipse(15, 15, 10, 10, 2);
    b.ellipse(15, 15, 2.6f, 2.6f, 3);
    float a0 = spokes ? 0.35f : 0.f;
    for (int i = 0; i < 4; i++) {
        float a = a0 + i * 0.785398f;
        b.line(15, 15, 15 + std::cos(a) * 9.5f, 15 + std::sin(a) * 9.5f, 4, 1.2f);
    }
    return b;
}

gs::Bitmap driveArt() {
    gs::Bitmap b(28, 20);
    b.rect(1, 2, 26, 16, 1);
    b.rect(3, 4, 22, 12, 2);
    b.rect(6, 6, 4, 8, 3);
    b.rect(18, 6, 4, 8, 3);
    b.rect(12, 8, 4, 4, 4);
    b.outline(15, false);
    return b;
}

gs::Bitmap reelArt() {
    gs::Bitmap b(18, 18);
    b.ellipse(9, 9, 7, 7, 1);
    b.ellipse(9, 9, 3, 3, 2);
    b.ellipse(9, 9, 1.4f, 1.4f, 3);
    b.outline(15, false);
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(16, 10);
    b.ellipse(8, 5, 6, 3, 1);
    b.ellipse(5, 6, 2, 1.3f, 2);
    return b;
}

gs::Bitmap roadArt() {
    gs::Bitmap b(64, 28);
    b.rect(0, 0, 64, 28, 1);
    for (int x = 2; x < 62; x += 11) b.rect(x, 8, 5, 2, 2);
    b.rect(0, 0, 64, 3, 3);
    b.rect(0, 25, 64, 3, 4);
    return b;
}

gs::Bitmap kerbArt() {
    gs::Bitmap b(32, 12);
    b.rect(0, 2, 32, 8, 1);
    for (int x = 0; x < 32; x += 8) b.rect(x, 2, 4, 8, 2);
    return b;
}

gs::Bitmap mastArt() {
    gs::Bitmap b(18, 96);
    b.rect(6, 4, 6, 90, 1);
    b.rect(3, 2, 12, 8, 2);
    for (int y = 16; y < 88; y += 12) b.rect(4, y, 10, 2, 3);
    b.rect(7, 88, 4, 6, 4);
    b.outline(15, false);
    return b;
}

gs::Bitmap armArt() {
    gs::Bitmap b(88, 16);
    b.rect(0, 4, 84, 6, 1);
    b.rect(0, 10, 84, 3, 2);
    for (int x = 4; x < 80; x += 10) b.rect(x, 5, 3, 4, 3);
    b.rect(78, 2, 8, 12, 4);
    b.outline(15, false);
    return b;
}

gs::Bitmap hookArt() {
    gs::Bitmap b(14, 28);
    b.rect(6, 0, 2, 14, 1);
    b.line(7, 14, 3, 22, 2, 1.8f);
    b.line(3, 22, 10, 24, 2, 1.8f);
    b.ellipse(8, 22, 3, 3, 3);
    b.outline(15, false);
    return b;
}

gs::Bitmap padArt() {
    gs::Bitmap b(36, 10);
    b.rect(0, 2, 36, 6, 1);
    b.rect(2, 3, 8, 3, 2);
    b.rect(26, 3, 8, 3, 2);
    return b;
}

gs::Bitmap clockArt(int hand) {
    gs::Bitmap b(32, 32);
    b.ellipse(16, 16, 13, 13, 1);
    b.ellipse(16, 16, 11, 11, 2);
    b.ellipse(16, 16, 2.2f, 2.2f, 3);
    for (int i = 0; i < 12; i++) {
        float a = i * 0.523599f - 1.5708f;
        b.set(int(16 + std::cos(a) * 9.f), int(16 + std::sin(a) * 9.f), 4);
    }
    float a = hand * 0.785398f - 1.5708f;
    b.line(16, 16, 16 + std::cos(a) * 9.f, 16 + std::sin(a) * 9.f, 5, 1.6f);
    b.outline(15, false);
    return b;
}

gs::Bitmap crewArt(bool step) {
    gs::Bitmap b(24, 36);
    b.ellipse(12, 6, 4, 4, 1);
    b.rect(8, 11, 8, 11, 2);
    b.rect(9, 12, 6, 4, 5);
    b.line(8, 14, step ? 2.f : 5.f, 22, 3, 1.6f);
    b.line(16, 14, step ? 22.f : 19.f, 22, 3, 1.6f);
    b.line(10, 22, step ? 6.f : 9.f, 34, 4, 1.7f);
    b.line(14, 22, step ? 18.f : 15.f, 34, 4, 1.7f);
    b.set(10, 5, 6);
    b.set(14, 5, 6);
    return b;
}

gs::Bitmap vanArt() {
    gs::Bitmap b(56, 28);
    b.rect(2, 8, 40, 14, 1);
    b.rect(40, 12, 12, 10, 2);
    b.rect(8, 10, 10, 6, 3);
    b.rect(22, 10, 10, 6, 3);
    b.ellipse(14, 22, 4, 4, 4);
    b.ellipse(36, 22, 4, 4, 4);
    b.outline(15, false);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(12, 40);
    b.rect(5, 12, 2, 26, 1);
    b.ellipse(6, 8, 4, 4, 2);
    b.ellipse(6, 8, 2, 2, 3);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(28, 40);
    b.rect(12, 20, 4, 18, 3);
    b.ellipse(14, 14, 11, 11, 1);
    b.ellipse(10, 12, 5, 4, 2);
    b.outline(15, false);
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 14, 13), gs::rgb4(8, 8, 7)});
    setPal(vdp, PAL_BIKE,
           {0, gs::rgb4(2, 3, 5), gs::rgb4(1, 1, 2), gs::rgb4(10, 11, 12), gs::rgb4(1, 1, 1), gs::rgb4(6, 6, 7),
            gs::rgb4(8, 2, 2), gs::rgb4(12, 10, 4), gs::rgb4(12, 8, 6), gs::rgb4(3, 5, 9), gs::rgb4(2, 2, 4),
            gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_DRIVE,
           {0, gs::rgb4(3, 5, 8), gs::rgb4(6, 9, 12), gs::rgb4(12, 10, 4), gs::rgb4(14, 13, 8), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_ROAD, {0, gs::rgb4(3, 3, 4), gs::rgb4(8, 8, 6), gs::rgb4(5, 5, 6), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_BOOM,
           {0, gs::rgb4(12, 9, 2), gs::rgb4(8, 6, 1), gs::rgb4(14, 12, 5), gs::rgb4(4, 3, 2), gs::rgb4(6, 5, 3)});
    setPal(vdp, PAL_HOOK, {0, gs::rgb4(9, 9, 10), gs::rgb4(5, 5, 6), gs::rgb4(13, 11, 4)});
    setPal(vdp, PAL_CREW,
           {0, gs::rgb4(11, 8, 6), gs::rgb4(4, 6, 10), gs::rgb4(8, 7, 5), gs::rgb4(2, 2, 3), gs::rgb4(13, 4, 3),
            gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_CLOCK,
           {0, gs::rgb4(10, 8, 3), gs::rgb4(14, 13, 9), gs::rgb4(2, 2, 3), gs::rgb4(6, 5, 3), gs::rgb4(12, 3, 2),
            gs::rgb4(14, 12, 4)});
    setPal(vdp, PAL_TOWN, {0, gs::rgb4(2, 6, 3), gs::rgb4(4, 9, 4), gs::rgb4(5, 3, 2), gs::rgb4(8, 7, 5)});
    setPal(vdp, PAL_DUST, {0, gs::rgb4(8, 7, 5), gs::rgb4(5, 4, 3)});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(6, 13, 7), gs::rgb4(2, 6, 3)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(14, 4, 3), gs::rgb4(6, 1, 1)});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(14, 11, 4), gs::rgb4(6, 4, 1)});
    setPal(vdp, PAL_DOCK, {0, gs::rgb4(5, 5, 6), gs::rgb4(8, 3, 2), gs::rgb4(10, 11, 12), gs::rgb4(2, 2, 2)});

    art.bike = gs::uploadMipped(vdp, bikeArt());
    art.wheel[0] = gs::uploadMipped(vdp, wheelArt(0));
    art.wheel[1] = gs::uploadMipped(vdp, wheelArt(1));
    art.drive = gs::uploadMipped(vdp, driveArt());
    art.reel = gs::uploadMipped(vdp, reelArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.road = gs::uploadMipped(vdp, roadArt());
    art.kerb = gs::uploadMipped(vdp, kerbArt());
    art.mast = gs::uploadMipped(vdp, mastArt());
    art.arm = gs::uploadMipped(vdp, armArt());
    art.hook = gs::uploadMipped(vdp, hookArt());
    art.pad = gs::uploadMipped(vdp, padArt());
    for (int i = 0; i < 8; i++) art.clock[i] = gs::uploadMipped(vdp, clockArt(i));
    art.crew[0] = gs::uploadMipped(vdp, crewArt(false));
    art.crew[1] = gs::uploadMipped(vdp, crewArt(true));
    art.van = gs::uploadMipped(vdp, vanArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.title = gs::uploadMipped(vdp, wordArt("BIKE", 3, 1));
    art.boomWord = gs::uploadMipped(vdp, wordArt("BOOM", 3, 1));
    art.delivered = gs::uploadMipped(vdp, wordArt("DELIVERED", 2, 1));
    art.shortB = gs::uploadMipped(vdp, wordArt("SHORT", 2, 1));
    art.past = gs::uploadMipped(vdp, wordArt("PAST", 2, 1));
    art.late = gs::uploadMipped(vdp, wordArt("TOO LATE", 2, 1));
    art.paused = gs::uploadMipped(vdp, wordArt("HOLD", 2, 1));
    loadFont(vdp, art);
}

}  // namespace bikeboom

#include "game/art.h"

#include <cstring>

namespace drift {
namespace {

void pal(gs::VDP& v, int p, int i, int r, int g, int b) { v.setColor(p * 16 + i, gs::rgb4(r, g, b)); }

void palettes(gs::VDP& v) {
    for (int p = 0; p < 16; p++) pal(v, p, 0, 0, 0, 0);

    pal(v, PAL_INK, 1, 14, 14, 15);
    pal(v, PAL_INK, 2, 2, 2, 5);
    pal(v, PAL_HOT, 1, 15, 10, 3);
    pal(v, PAL_HOT, 2, 3, 1, 1);
    pal(v, PAL_BAD, 1, 15, 3, 3);
    pal(v, PAL_BAD, 2, 3, 0, 1);
    pal(v, PAL_GOOD, 1, 8, 15, 10);
    pal(v, PAL_GOOD, 2, 1, 3, 2);

    pal(v, PAL_CAR, 1, 1, 1, 2);
    pal(v, PAL_CAR, 2, 4, 4, 5);
    pal(v, PAL_CAR, 3, 10, 10, 11);
    pal(v, PAL_CAR, 4, 8, 0, 1);
    pal(v, PAL_CAR, 5, 14, 1, 2);
    pal(v, PAL_CAR, 6, 2, 1, 2);
    pal(v, PAL_CAR, 7, 3, 6, 9);
    pal(v, PAL_CAR, 8, 14, 14, 12);
    pal(v, PAL_CAR, 9, 1, 1, 2);
    pal(v, PAL_CAR, 10, 9, 1, 1);
    pal(v, PAL_CAR, 11, 15, 7, 2);
    pal(v, PAL_CAR, 12, 1, 0, 1);
    pal(v, PAL_CAR, 13, 15, 9, 7);

    pal(v, PAL_WALL, 1, 8, 8, 9);
    pal(v, PAL_WALL, 2, 12, 12, 13);
    pal(v, PAL_WALL, 3, 15, 11, 2);
    pal(v, PAL_WALL, 4, 2, 2, 3);
    pal(v, PAL_WALL, 5, 14, 9, 2);

    pal(v, PAL_LAMP, 1, 3, 3, 4);
    pal(v, PAL_LAMP, 2, 6, 6, 7);
    pal(v, PAL_LAMP, 3, 15, 9, 2);
    pal(v, PAL_LAMP, 4, 15, 14, 8);
    pal(v, PAL_LAMP, 5, 1, 1, 2);

    pal(v, PAL_SMOKE, 1, 8, 8, 10);
    pal(v, PAL_SMOKE, 2, 14, 14, 15);
    pal(v, PAL_SMOKE, 3, 5, 5, 7);

    pal(v, PAL_TITLE, 1, 15, 8, 2);
    pal(v, PAL_TITLE, 2, 2, 1, 4);
    pal(v, PAL_TITLE, 3, 1, 0, 2);
    pal(v, PAL_TITLE, 4, 15, 14, 12);
    pal(v, PAL_TITLE, 5, 14, 2, 3);

    pal(v, PAL_BANNER, 1, 2, 2, 4);
    pal(v, PAL_BANNER, 2, 15, 15, 15);
    pal(v, PAL_BANNER, 3, 13, 2, 2);
    pal(v, PAL_BANNER, 4, 15, 11, 3);
    pal(v, PAL_BANNER, 5, 1, 1, 2);

    pal(v, PAL_SKY, 1, 2, 2, 5);
    pal(v, PAL_SKY, 2, 3, 3, 7);
    pal(v, PAL_SKY, 3, 4, 4, 8);
    pal(v, PAL_SKY, 4, 15, 12, 5);
    pal(v, PAL_SKY, 5, 8, 6, 3);
    pal(v, PAL_SKY, 6, 14, 14, 11);
    pal(v, PAL_SKY, 7, 5, 5, 7);
    pal(v, PAL_SKY, 8, 7, 2, 3);
    pal(v, PAL_SKY, 9, 8, 4, 3);
    pal(v, PAL_SKY, 10, 12, 12, 14);

    auto road = [&](int p) {
        pal(v, p, 1, 2, 2, 3);
        pal(v, p, 2, 3, 3, 4);
        pal(v, p, 3, 5, 5, 6);
        pal(v, p, 4, 4, 4, 5);
        pal(v, p, 5, 2, 2, 3);
        pal(v, p, 6, 2, 2, 3);
        pal(v, p, 7, 3, 3, 4);
        pal(v, p, 8, 6, 6, 7);
        pal(v, p, 9, 4, 4, 5);
        pal(v, p, 10, 3, 3, 4);
        pal(v, p, 14, 14, 13, 9);
        pal(v, p, 15, 7, 7, 8);
    };
    road(PAL_ROAD);
    v.setFogColor(gs::rgb4(2, 2, 4));
}

void fontTiles(gs::TileAlloc& tiles, int* out) {
    uint8_t px[64];
    for (int ch = 32; ch < 127; ch++) {
        const uint8_t* g = gs::glyph(char(ch));
        std::memset(px, 0, sizeof px);
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x] && x + 1 < 8 && y + 1 < 8) px[(y + 1) * 8 + x + 1] = 2;
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) px[y * 8 + x] = 1;
        out[ch] = tiles.shared(px);
    }
}

void skyline(gs::VDP& v, gs::TileAlloc& tiles) {
    gs::Bitmap b(320, 104);
    for (int i = 0; i < 40; i++) {
        int x = (i * 47) % 310;
        int y = (i * 19) % 36;
        b.set(x, y, 10);
        if (i % 4 == 0) b.set(x + 1, y, 10);
    }
    b.ellipse(268, 22, 11, 11, 6);
    b.ellipse(274, 20, 9, 9, 0);
    b.rect(0, 88, 320, 16, 9);

    struct Blk {
        int x, w, h, c;
    };
    const Blk blocks[] = {
        {8, 28, 38, 1},   {34, 18, 52, 2},  {58, 36, 30, 1},  {96, 22, 64, 2},
        {116, 40, 44, 3}, {160, 16, 70, 2}, {178, 30, 36, 1}, {214, 24, 48, 3},
        {242, 18, 28, 1}, {286, 26, 40, 2}, {40, 14, 22, 8},  {200, 12, 18, 8},
    };
    for (const Blk& t : blocks) b.rect(float(t.x), float(104 - t.h), float(t.w), float(t.h), t.c);
    b.rect(160, 104 - 70, 16, 8, 8);
    b.line(168, 34, 196, 28, 7, 2);
    b.line(196, 28, 196, 70, 7, 2);
    b.rect(108, 18, 6, 22, 7);
    b.ellipse(111, 16, 4, 3, 5);

    for (const Blk& t : blocks) {
        if (t.h < 32) continue;
        for (int wy = 104 - t.h + 6; wy < 96; wy += 8)
            for (int wx = t.x + 3; wx < t.x + t.w - 4; wx += 6)
                if (((wx + wy) / 3) % 3 != 0) b.rect(float(wx), float(wy), 2, 3, (wx + wy) % 5 == 0 ? 4 : 5);
    }
    gs::bitmapToPlane(tiles, v.B, 0, 0, b, PAL_SKY);
}

gs::Bitmap carFrame(float n) {
    gs::Bitmap b(96, 72);
    auto X = [&](float x, float w) { return x + n * w; };
    b.ellipse(X(22, 16), 52, 10, 14, 1);
    b.ellipse(X(22, 16), 52, 4, 8, 2);
    b.ellipse(X(74, -14), 52, 10, 14, 1);
    b.ellipse(X(74, -14), 52, 4, 8, 3);
    b.poly({{X(26, 8), 30}, {X(70, 2), 30}, {X(84, -10), 54}, {X(12, 14), 54}}, 4);
    b.poly({{X(30, 8), 33}, {X(66, 2), 33}, {X(76, -6), 50}, {X(20, 10), 50}}, 5);
    b.poly({{X(28, 14), 20}, {X(68, 8), 20}, {X(64, 6), 32}, {X(30, 10), 32}}, 6);
    b.rect(X(40, 12), 14, 16, 4, 6);
    b.line(X(48, 12), 14, X(48, 14), 8, 2, 2);
    b.poly({{X(36, 12), 34}, {X(60, 6), 34}, {X(56, 4), 44}, {X(34, 8), 44}}, 7);
    b.rect(X(40, 2), 46, 16, 7, 8);
    b.rect(X(42, 2), 48, 4, 3, 9);
    b.rect(X(49, 2), 48, 5, 3, 9);
    b.rect(X(16, 12), 36, 12, 6, 10);
    b.rect(X(68, -8), 36, 12, 6, 10);
    b.rect(X(18, 12), 37, 7, 3, 11);
    b.rect(X(71, -8), 37, 7, 3, 11);
    b.rect(X(28, 8), 56, 6, 3, 2);
    b.rect(X(64, -4), 56, 6, 3, 2);
    b.rect(X(32, 6), 34, 32, 2, 13);
    b.outline(12, false);
    return b;
}

gs::Bitmap barrierBmp() {
    gs::Bitmap b(40, 32);
    b.poly({{2, 28}, {38, 28}, {34, 10}, {6, 10}}, 1);
    b.poly({{6, 10}, {34, 10}, {31, 5}, {9, 5}}, 2);
    b.rect(4, 16, 32, 4, 5);
    b.rect(8, 22, 8, 4, 3);
    b.rect(24, 22, 8, 4, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap lampBmp() {
    gs::Bitmap b(22, 64);
    b.rect(9, 18, 4, 42, 1);
    b.rect(6, 58, 10, 4, 1);
    b.rect(4, 14, 14, 4, 2);
    b.ellipse(11, 12, 8, 6, 3);
    b.ellipse(11, 12, 3, 3, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap smokeBmp() {
    gs::Bitmap b(18, 18);
    b.ellipse(9, 10, 8, 6, 1);
    b.ellipse(8, 8, 4, 3, 2);
    b.ellipse(12, 11, 2, 2, 3);
    return b;
}

gs::Bitmap shadowBmp() {
    gs::Bitmap b(52, 16);
    b.ellipse(26, 8, 22, 6, 1);
    return b;
}

gs::Bitmap bannerBmp(const char* word, int stripe) {
    gs::Bitmap b(104, 32);
    b.rect(0, 4, 104, 24, 1);
    b.rect(0, 4, 104, 4, stripe);
    b.rect(0, 24, 104, 4, stripe);
    for (int i = 0; i < 8; i++) b.rect(float(4 + i * 12), float((i & 1) ? 8 : 20), 6, 3, stripe);
    gs::Bitmap t = gs::textBitmap(word, {2, 2, 0, 0, 1});
    b.blit(t, (104 - t.w) / 2, 9);
    return b;
}

gs::Image words(gs::VDP& v, const char* s, int scale, int color, int outline) {
    return gs::uploadImage(v, gs::textBitmap(s, {scale, color, outline, 3, 1}));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    palettes(vdp);
    gs::TileAlloc tiles(vdp, 1);
    fontTiles(tiles, art.font);
    skyline(vdp, tiles);
    for (int i = 0; i < 7; i++) {
        float n = (i - 3) / 3.f;
        art.car[i] = gs::uploadMipped(vdp, carFrame(n));
    }
    art.barrier = gs::uploadMipped(vdp, barrierBmp());
    art.lamp = gs::uploadMipped(vdp, lampBmp());
    art.smoke = gs::uploadMipped(vdp, smokeBmp());
    art.shadow = gs::uploadMipped(vdp, shadowBmp());
    art.bannerSlide = gs::uploadMipped(vdp, bannerBmp("SLIDE", 4));
    art.bannerFinish = gs::uploadMipped(vdp, bannerBmp("FINISH", 3));
    art.title = words(vdp, "S3 DRIFT", 5, 1, 2);
    art.sub = words(vdp, "ONE PASS", 2, 4, 2);
    art.tag = words(vdp, "SCORE THE SLIDE", 2, 5, 2);
    art.count[0] = words(vdp, "3", 5, 4, 2);
    art.count[1] = words(vdp, "2", 5, 4, 2);
    art.count[2] = words(vdp, "1", 5, 4, 2);
    art.count[3] = words(vdp, "GO", 5, 1, 2);
    art.walled = words(vdp, "THE WALL ENDS IT", 3, 5, 2);
    art.scored = words(vdp, "PASS SCORED", 3, 1, 2);
    art.shortSlide = words(vdp, "NOT ENOUGH SLIDE", 2, 4, 2);
    art.paused = words(vdp, "PAUSED", 3, 4, 2);
}

}  // namespace drift

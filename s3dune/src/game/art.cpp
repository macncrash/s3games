#include "art.h"

#include <cmath>
#include <cstring>

namespace dune {
namespace {

void pal(gs::VDP& v, int p, int i, int r, int g, int b) { v.setColor(p * 16 + i, gs::rgb4(r, g, b)); }

void roadPal(gs::VDP& v, int p, bool damp) {
    if (!damp) {
        pal(v, p, 1, 13, 10, 6);
        pal(v, p, 2, 11, 8, 4);
        pal(v, p, 3, 14, 11, 7);
        pal(v, p, 4, 9, 6, 3);
        pal(v, p, 5, 12, 8, 4);
        pal(v, p, 6, 8, 6, 3);
        pal(v, p, 7, 11, 8, 4);
        pal(v, p, 8, 7, 6, 5);
        pal(v, p, 9, 6, 4, 2);
        pal(v, p, 10, 14, 11, 6);
        pal(v, p, 11, 4, 8, 12);
        pal(v, p, 12, 5, 9, 13);
        pal(v, p, 13, 8, 12, 15);
        pal(v, p, 14, 15, 14, 9);
        pal(v, p, 15, 12, 9, 5);
    } else {
        pal(v, p, 1, 8, 8, 7);
        pal(v, p, 2, 6, 6, 6);
        pal(v, p, 3, 9, 9, 8);
        pal(v, p, 4, 5, 6, 7);
        pal(v, p, 5, 7, 7, 8);
        pal(v, p, 6, 5, 6, 7);
        pal(v, p, 7, 7, 8, 8);
        pal(v, p, 8, 4, 5, 6);
        pal(v, p, 9, 4, 5, 6);
        pal(v, p, 10, 10, 11, 10);
        pal(v, p, 11, 3, 7, 12);
        pal(v, p, 12, 4, 9, 14);
        pal(v, p, 13, 6, 12, 15);
        pal(v, p, 14, 8, 13, 14);
        pal(v, p, 15, 6, 7, 8);
    }
}

void palettes(gs::VDP& v) {
    for (int p = 0; p < 16; p++) pal(v, p, 0, 0, 0, 0);

    pal(v, PAL_INK, 1, 15, 15, 14);
    pal(v, PAL_INK, 2, 2, 1, 1);
    pal(v, PAL_HOT, 1, 15, 12, 3);
    pal(v, PAL_HOT, 2, 3, 1, 0);
    pal(v, PAL_BAD, 1, 15, 3, 2);
    pal(v, PAL_BAD, 2, 3, 0, 0);
    pal(v, PAL_GOOD, 1, 6, 15, 10);
    pal(v, PAL_GOOD, 2, 0, 2, 1);

    pal(v, PAL_BUGGY, 1, 1, 1, 1);
    pal(v, PAL_BUGGY, 2, 3, 3, 3);
    pal(v, PAL_BUGGY, 3, 8, 8, 7);
    pal(v, PAL_BUGGY, 4, 12, 8, 3);
    pal(v, PAL_BUGGY, 5, 9, 3, 1);
    pal(v, PAL_BUGGY, 6, 13, 6, 1);
    pal(v, PAL_BUGGY, 7, 15, 10, 3);
    pal(v, PAL_BUGGY, 8, 10, 10, 9);
    pal(v, PAL_BUGGY, 9, 4, 4, 5);
    pal(v, PAL_BUGGY, 10, 7, 7, 8);
    pal(v, PAL_BUGGY, 11, 15, 14, 8);
    pal(v, PAL_BUGGY, 12, 14, 2, 1);
    pal(v, PAL_BUGGY, 13, 1, 0, 0);
    pal(v, PAL_BUGGY, 14, 5, 3, 2);

    pal(v, PAL_ROCK, 1, 6, 5, 4);
    pal(v, PAL_ROCK, 2, 9, 7, 5);
    pal(v, PAL_ROCK, 3, 12, 10, 7);
    pal(v, PAL_ROCK, 4, 2, 2, 2);
    pal(v, PAL_ROCK, 5, 8, 8, 7);

    pal(v, PAL_TANK, 1, 3, 3, 4);
    pal(v, PAL_TANK, 2, 6, 6, 7);
    pal(v, PAL_TANK, 3, 2, 6, 12);
    pal(v, PAL_TANK, 4, 4, 10, 15);
    pal(v, PAL_TANK, 5, 8, 8, 9);
    pal(v, PAL_TANK, 6, 4, 4, 5);
    pal(v, PAL_TANK, 7, 14, 10, 2);
    pal(v, PAL_TANK, 8, 1, 1, 2);
    pal(v, PAL_TANK, 9, 14, 15, 15);

    pal(v, PAL_DUST, 1, 10, 8, 5);
    pal(v, PAL_DUST, 2, 13, 11, 7);
    pal(v, PAL_DUST, 3, 8, 6, 4);
    pal(v, PAL_DUST, 4, 14, 14, 13);
    pal(v, PAL_DUST, 5, 11, 12, 13);

    pal(v, PAL_TITLE, 1, 15, 12, 4);
    pal(v, PAL_TITLE, 2, 3, 1, 0);
    pal(v, PAL_TITLE, 3, 1, 0, 0);
    pal(v, PAL_TITLE, 4, 15, 15, 13);
    pal(v, PAL_TITLE, 5, 15, 3, 2);
    pal(v, PAL_TITLE, 6, 4, 12, 14);

    pal(v, PAL_SIGN, 1, 2, 4, 8);
    pal(v, PAL_SIGN, 2, 15, 15, 14);
    pal(v, PAL_SIGN, 3, 4, 10, 14);
    pal(v, PAL_SIGN, 4, 6, 4, 2);
    pal(v, PAL_SIGN, 5, 1, 1, 2);
    pal(v, PAL_SIGN, 7, 12, 8, 3);

    pal(v, PAL_SKY, 1, 12, 8, 4);
    pal(v, PAL_SKY, 2, 10, 6, 3);
    pal(v, PAL_SKY, 3, 8, 5, 2);
    pal(v, PAL_SKY, 4, 15, 15, 11);
    pal(v, PAL_SKY, 5, 15, 12, 5);
    pal(v, PAL_SKY, 6, 2, 5, 2);
    pal(v, PAL_SKY, 7, 14, 10, 6);

    pal(v, PAL_PALM, 1, 6, 4, 2);
    pal(v, PAL_PALM, 2, 8, 6, 3);
    pal(v, PAL_PALM, 3, 2, 6, 2);
    pal(v, PAL_PALM, 4, 3, 8, 3);
    pal(v, PAL_PALM, 5, 6, 11, 4);
    pal(v, PAL_PALM, 6, 1, 2, 1);

    pal(v, PAL_TENT, 1, 14, 12, 8);
    pal(v, PAL_TENT, 2, 10, 7, 4);
    pal(v, PAL_TENT, 3, 3, 2, 2);
    pal(v, PAL_TENT, 4, 2, 1, 1);
    pal(v, PAL_TENT, 5, 6, 4, 2);

    roadPal(v, PAL_ROAD, false);
    roadPal(v, PAL_DAMP, true);
    v.setFogColor(gs::rgb4(14, 11, 7));
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
    gs::Bitmap b(512, 112);
    b.ellipse(400, 26, 16, 16, 5);
    b.ellipse(400, 26, 9, 9, 4);
    for (int x = 0; x < 512; x++) {
        float n = x * 0.02f;
        int crest = 74 + int(9.f * std::sin(n) + 5.f * std::sin(n * 2.4f + 1.1f));
        int mid = 90 + int(4.f * std::sin(n * 1.3f + 0.5f));
        int near = 102 + int(3.f * std::sin(n * 2.2f));
        if (crest < 60) crest = 60;
        for (int y = crest; y < 112; y++) {
            int c = 1;
            if (y >= mid) c = 2;
            if (y >= near) c = 3;
            if (y < crest + 2 && ((x * 3) % 7 == 0)) c = 7;
            b.set(x, y, c);
        }
    }
    auto cactus = [&](int x, int base) {
        b.rect(float(x), float(base - 14), 3, 14, 6);
        b.rect(float(x - 5), float(base - 9), 5, 2, 6);
        b.rect(float(x - 5), float(base - 14), 2, 5, 6);
        b.rect(float(x + 3), float(base - 7), 5, 2, 6);
        b.rect(float(x + 6), float(base - 12), 2, 5, 6);
    };
    cactus(70, 88);
    cactus(210, 84);
    cactus(330, 90);
    cactus(470, 86);
    gs::bitmapToPlane(tiles, v.B, 0, 0, b, PAL_SKY);
}

gs::Bitmap buggyFrame(float lean) {
    gs::Bitmap b(96, 68);
    auto X = [&](float x, float w) { return x + lean * w; };
    b.ellipse(X(22, 4), 48, 14, 16, 1);
    b.ellipse(X(74, 4), 48, 14, 16, 1);
    b.ellipse(X(22, 4), 48, 8, 10, 2);
    b.ellipse(X(74, 4), 48, 8, 10, 2);
    b.ellipse(X(22, 4), 48, 4, 5, 3);
    b.ellipse(X(74, 4), 48, 4, 5, 3);
    b.ellipse(X(22, 4), 48, 2, 2, 4);
    b.ellipse(X(74, 4), 48, 2, 2, 4);
    b.rect(X(20, 5), 46, 56, 4, 9);
    b.poly({{X(26, 8), 44}, {X(70, 8), 44}, {X(76, 7), 30}, {X(20, 7), 30}}, 5);
    b.poly({{X(30, 8), 42}, {X(66, 8), 42}, {X(70, 7), 32}, {X(26, 7), 32}}, 6);
    b.rect(X(34, 8), 33, 28, 4, 7);
    b.rect(X(28, 7), 36, 7, 4, 12);
    b.rect(X(61, 7), 36, 7, 4, 12);
    b.line(X(30, 8), 32, X(28, 9), 14, 8, 2.2f);
    b.line(X(66, 8), 32, X(68, 9), 14, 8, 2.2f);
    b.line(X(28, 9), 14, X(68, 9), 14, 8, 2.2f);
    b.line(X(28, 9), 14, X(48, 8), 8, 8, 2.f);
    b.line(X(68, 9), 14, X(48, 8), 8, 8, 2.f);
    b.rect(X(40, 8), 16, 16, 14, 9);
    b.rect(X(42, 8), 18, 12, 8, 10);
    for (int i = 0; i < 4; i++) b.rect(X(43.f + i * 3.f, 8), 18, 2, 8, 8);
    b.ellipse(X(48, 8), 16, 6, 4, 10);
    b.ellipse(X(48, 8), 16, 3, 2, 3);
    b.rect(X(36, 7), 48, 5, 8, 14);
    b.rect(X(55, 7), 48, 5, 8, 14);
    b.outline(13, false);
    return b;
}

gs::Bitmap rockBmp() {
    gs::Bitmap b(40, 28);
    b.ellipse(20, 16, 16, 10, 1);
    b.ellipse(15, 14, 8, 6, 2);
    b.ellipse(24, 17, 6, 4, 3);
    b.rect(12, 18, 4, 3, 5);
    b.outline(4, false);
    return b;
}

gs::Bitmap scrubBmp() {
    gs::Bitmap b(28, 18);
    b.ellipse(14, 11, 12, 6, 1);
    b.ellipse(8, 9, 5, 4, 2);
    b.ellipse(18, 10, 5, 3, 3);
    return b;
}

gs::Bitmap tankBmp() {
    gs::Bitmap b(56, 64);
    b.rect(10, 44, 5, 16, 6);
    b.rect(41, 44, 5, 16, 6);
    b.rect(8, 56, 10, 4, 1);
    b.rect(38, 56, 10, 4, 1);
    b.ellipse(28, 32, 22, 14, 2);
    b.ellipse(28, 30, 16, 9, 3);
    b.ellipse(22, 27, 6, 4, 4);
    b.rect(10, 24, 36, 6, 7);
    b.rect(26, 10, 4, 14, 1);
    b.ellipse(28, 9, 6, 4, 5);
    b.rect(24, 6, 8, 3, 9);
    b.outline(8, false);
    return b;
}

gs::Bitmap palmBmp() {
    gs::Bitmap b(48, 80);
    b.rect(21, 36, 7, 40, 1);
    b.rect(23, 40, 2, 32, 2);
    b.line(24, 40, 6, 22, 4, 2.4f);
    b.line(24, 40, 42, 22, 4, 2.4f);
    b.line(24, 40, 8, 34, 3, 2.2f);
    b.line(24, 40, 40, 34, 3, 2.2f);
    b.line(24, 40, 24, 12, 5, 2.4f);
    b.ellipse(24, 28, 16, 8, 4);
    b.ellipse(12, 26, 8, 4, 3);
    b.ellipse(36, 26, 8, 4, 3);
    b.ellipse(24, 18, 6, 7, 5);
    b.outline(6, false);
    return b;
}

gs::Bitmap postBmp() {
    gs::Bitmap b(10, 48);
    b.rect(3, 0, 4, 44, 1);
    b.rect(1, 42, 8, 5, 2);
    b.outline(4, false);
    return b;
}

gs::Bitmap bannerBmp(const char* word, int stripe) {
    gs::Bitmap b(120, 36);
    b.rect(2, 8, 116, 22, 1);
    b.rect(2, 8, 116, 4, stripe);
    b.rect(2, 26, 116, 4, stripe);
    gs::Bitmap t = gs::textBitmap(word, {2, 2, 0, 0, 1});
    b.blit(t, (120 - t.w) / 2, 12);
    b.outline(5, false);
    return b;
}

gs::Bitmap tentBmp() {
    gs::Bitmap b(48, 36);
    b.poly({{4, 32}, {44, 32}, {24, 6}}, 1);
    b.poly({{24, 8}, {42, 32}, {28, 32}}, 2);
    b.rect(20, 22, 6, 10, 3);
    b.line(6, 32, 24, 6, 5, 1.5f);
    b.outline(4, false);
    return b;
}

gs::Bitmap dustBmp() {
    gs::Bitmap b(18, 16);
    b.ellipse(9, 9, 8, 5, 1);
    b.ellipse(7, 8, 4, 3, 2);
    b.ellipse(12, 10, 3, 2, 3);
    return b;
}

gs::Bitmap steamBmp() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 9, 6, 5, 4);
    b.ellipse(6, 7, 3, 2, 5);
    return b;
}

gs::Bitmap shadowBmp() {
    gs::Bitmap b(64, 16);
    b.ellipse(32, 8, 26, 6, 1);
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
    for (int i = 0; i < 3; i++) art.buggy[i] = gs::uploadMipped(vdp, buggyFrame(float(i - 1)));
    art.shadow = gs::uploadMipped(vdp, shadowBmp());
    art.dust = gs::uploadMipped(vdp, dustBmp());
    art.steam = gs::uploadMipped(vdp, steamBmp());
    art.rock = gs::uploadMipped(vdp, rockBmp());
    art.scrub = gs::uploadMipped(vdp, scrubBmp());
    art.tank = gs::uploadMipped(vdp, tankBmp());
    art.palm = gs::uploadMipped(vdp, palmBmp());
    art.post = gs::uploadMipped(vdp, postBmp());
    art.bannerWater = gs::uploadMipped(vdp, bannerBmp("WATER", 3));
    art.bannerCamp = gs::uploadMipped(vdp, bannerBmp("CAMP", 7));
    art.tent = gs::uploadMipped(vdp, tentBmp());
    art.title = words(vdp, "S3 DUNE", 4, 1, 2);
    art.sub = words(vdp, "ONE WATER STOP", 2, 6, 2);
    art.tag = words(vdp, "DON'T BOIL THE ENGINE", 2, 5, 2);
    art.count[0] = words(vdp, "3", 5, 4, 2);
    art.count[1] = words(vdp, "2", 5, 4, 2);
    art.count[2] = words(vdp, "1", 5, 4, 2);
    art.count[3] = words(vdp, "GO", 4, 1, 2);
    art.camp = words(vdp, "CAMP REACHED", 3, 6, 2);
    art.boiled = words(vdp, "ENGINE BOILED", 3, 5, 2);
    art.dry = words(vdp, "SKIPPED THE WATER", 2, 5, 2);
    art.paused = words(vdp, "PAUSED", 3, 4, 2);
}

}  // namespace dune

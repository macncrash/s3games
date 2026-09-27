#include "art.h"

#include <cstdint>
#include <cstring>

namespace cabpass {
namespace {

void pal(gs::VDP& v, int p, int i, int r, int g, int b) { v.setColor(p * 16 + i, gs::rgb4(r, g, b)); }

void textPal(gs::VDP& v, int p, int r, int g, int b) {
    pal(v, p, 1, r, g, b);
    pal(v, p, 2, 1, 2, 4);
    pal(v, p, 3, 0, 0, 2);
}

void palettes(gs::VDP& v) {
    textPal(v, PAL_HUD, 15, 15, 15);
    textPal(v, PAL_GOLD, 15, 13, 3);
    textPal(v, PAL_ALERT, 15, 4, 3);

    pal(v, PAL_CAB, 1, 15, 13, 2);   // yellow body
    pal(v, PAL_CAB, 2, 12, 9, 1);
    pal(v, PAL_CAB, 3, 2, 2, 3);     // windows / tyres
    pal(v, PAL_CAB, 4, 8, 10, 12);   // glass
    pal(v, PAL_CAB, 5, 14, 14, 15);  // roof sign
    pal(v, PAL_CAB, 6, 13, 2, 2);    // tail lamp
    pal(v, PAL_CAB, 7, 4, 4, 5);     // bumper
    pal(v, PAL_CAB, 8, 1, 1, 1);     // checker
    pal(v, PAL_CAB, 9, 15, 15, 15);
    pal(v, PAL_CAB, 10, 6, 5, 3);
    pal(v, PAL_CAB, 11, 15, 8, 2);
    pal(v, PAL_CAB, 12, 3, 3, 4);
    pal(v, PAL_CAB, 13, 9, 8, 6);
    pal(v, PAL_CAB, 14, 15, 12, 4);
    pal(v, PAL_CAB, 15, 1, 1, 2);

    pal(v, PAL_RIVAL, 1, 3, 5, 9);
    pal(v, PAL_RIVAL, 2, 2, 3, 6);
    pal(v, PAL_RIVAL, 3, 1, 1, 2);
    pal(v, PAL_RIVAL, 4, 6, 8, 11);
    pal(v, PAL_RIVAL, 5, 12, 12, 13);
    pal(v, PAL_RIVAL, 6, 14, 3, 2);
    pal(v, PAL_RIVAL, 7, 5, 5, 6);
    pal(v, PAL_RIVAL, 8, 1, 1, 1);
    pal(v, PAL_RIVAL, 9, 14, 14, 15);
    pal(v, PAL_RIVAL, 10, 4, 4, 5);
    pal(v, PAL_RIVAL, 11, 15, 10, 3);
    pal(v, PAL_RIVAL, 12, 2, 2, 4);
    pal(v, PAL_RIVAL, 13, 7, 7, 8);
    pal(v, PAL_RIVAL, 14, 10, 11, 13);
    pal(v, PAL_RIVAL, 15, 0, 0, 1);

    pal(v, PAL_PINE, 1, 2, 5, 3);
    pal(v, PAL_PINE, 2, 3, 8, 4);
    pal(v, PAL_PINE, 3, 6, 4, 2);
    pal(v, PAL_PINE, 4, 14, 15, 15);
    pal(v, PAL_PINE, 5, 1, 2, 2);

    pal(v, PAL_ROCK, 1, 7, 7, 8);
    pal(v, PAL_ROCK, 2, 4, 4, 5);
    pal(v, PAL_ROCK, 3, 12, 12, 13);
    pal(v, PAL_ROCK, 4, 9, 8, 6);
    pal(v, PAL_ROCK, 5, 2, 2, 3);

    pal(v, PAL_BANNER, 1, 15, 15, 15);
    pal(v, PAL_BANNER, 2, 2, 2, 4);
    pal(v, PAL_BANNER, 3, 12, 2, 2);
    pal(v, PAL_BANNER, 4, 15, 12, 3);
    pal(v, PAL_BANNER, 5, 4, 3, 2);
    pal(v, PAL_BANNER, 6, 8, 8, 9);

    pal(v, PAL_FX, 1, 15, 15, 15);
    pal(v, PAL_FX, 2, 12, 13, 15);
    pal(v, PAL_FX, 3, 7, 8, 10);

    pal(v, PAL_RANGE, 1, 5, 7, 11);
    pal(v, PAL_RANGE, 2, 3, 5, 9);
    pal(v, PAL_RANGE, 3, 2, 4, 7);
    pal(v, PAL_RANGE, 4, 14, 15, 15);
    pal(v, PAL_RANGE, 5, 2, 4, 3);
    pal(v, PAL_RANGE, 6, 7, 8, 10);
    pal(v, PAL_RANGE, 7, 11, 12, 14);
    pal(v, PAL_RANGE, 8, 15, 13, 8);
    pal(v, PAL_RANGE, 9, 4, 5, 7);

    // Tarmac indices used by the road generator (see vdp roadLine).
    pal(v, PAL_ROAD, 1, 4, 6, 4);
    pal(v, PAL_ROAD, 2, 3, 4, 3);
    pal(v, PAL_ROAD, 3, 6, 7, 5);
    pal(v, PAL_ROAD, 4, 8, 8, 7);
    pal(v, PAL_ROAD, 5, 5, 5, 5);
    pal(v, PAL_ROAD, 6, 3, 3, 4);
    pal(v, PAL_ROAD, 7, 4, 4, 5);
    pal(v, PAL_ROAD, 8, 6, 6, 6);
    pal(v, PAL_ROAD, 9, 2, 2, 3);
    pal(v, PAL_ROAD, 10, 5, 5, 6);
    pal(v, PAL_ROAD, 11, 4, 5, 7);
    pal(v, PAL_ROAD, 12, 3, 4, 6);
    pal(v, PAL_ROAD, 13, 6, 7, 9);
    pal(v, PAL_ROAD, 14, 14, 13, 4);
    pal(v, PAL_ROAD, 15, 7, 7, 8);

    v.setFogColor(gs::rgb4(8, 9, 12));
}

void peak(gs::Bitmap& b, float x, float top, float half, int body, int snow) {
    const float base = float(b.h - 1);
    b.poly({{x - half, base}, {x + half, base}, {x + half * 0.14f, top + 9.f}, {x, top}, {x - half * 0.12f, top + 7.f}},
           body);
    b.poly({{x - half * 0.14f, top + 11.f}, {x, top + 1.f}, {x + half * 0.16f, top + 12.f}}, snow);
}

void range(gs::VDP& v, gs::TileAlloc& tiles) {
    gs::Bitmap b(512, 96);
    peak(b, 80, 18, 130, 2, 4);
    peak(b, 210, 40, 64, 1, 4);
    peak(b, 310, 28, 50, 3, 4);
    peak(b, 440, 12, 150, 2, 4);
    peak(b, 160, 26, 36, 9, 4);
    peak(b, 360, 24, 32, 9, 4);
    b.ellipse(40, 20, 30, 12, 6);
    b.ellipse(70, 14, 20, 9, 7);
    b.ellipse(480, 16, 10, 8, 8);
    for (int i = 0; i < 16; i++) {
        float x = 16.f + float(i) * 31.f;
        float h = 8.f + float((i * 13) % 8);
        b.poly({{x, 92.f}, {x + 5.f, 92.f - h}, {x + 11.f, 92.f}}, 5);
    }
    gs::bitmapToPlane(tiles, v.B, 0, 0, b, PAL_RANGE);
}

void font(gs::TileAlloc& tiles, int* out) {
    uint8_t px[64];
    for (int ch = 32; ch < 127; ch++) {
        const uint8_t* g = gs::glyph(char(ch));
        std::memset(px, 0, sizeof px);
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x] && x + 1 < 8 && y + 1 < 8) px[(y + 1) * 8 + (x + 1)] = 2;
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) px[y * 8 + x] = 1;
        out[ch] = tiles.shared(px);
    }
}

constexpr int BODY = 1, SHADE = 2, INK = 3, GLASS = 4, SIGN = 5, LAMP = 6, BUMP = 7, CHECK = 8, WHITE = 9, TRIM = 10,
              AMBER = 11, TYRE = 12, PLATE = 13, ROOF = 14;

void cabRear(gs::Bitmap& b, float lean) {
    const float s = lean * 7.f;
    const float cx = 48.f + s;
    b.ellipse(48, 118, 22, 5, 3);
    b.rect(18, 78, 60, 36, BODY);
    b.rect(22, 46, 52, 34, BODY);
    b.poly({{22, 46}, {26, 34}, {70, 34}, {74, 46}}, ROOF);
    b.rect(30, 22, 36, 14, SIGN);
    b.rect(34, 25, 28, 8, INK);
    b.rect(38, 27, 6, 4, WHITE);
    b.rect(48, 27, 6, 4, WHITE);
    b.rect(26, 50, 18, 16, GLASS);
    b.rect(52, 50, 18, 16, GLASS);
    b.rect(46, 50, 4, 16, SHADE);
    b.rect(20, 86, 12, 10, LAMP);
    b.rect(64, 86, 12, 10, LAMP);
    b.rect(34, 90, 28, 8, PLATE);
    b.rect(38, 92, 20, 4, WHITE);
    b.rect(16, 110, 16, 10, TYRE);
    b.rect(64, 110, 16, 10, TYRE);
    b.rect(14, 108, 68, 4, BUMP);
    for (int i = 0; i < 8; i++) b.rect(20 + i * 7, 72, 4, 6, (i & 1) ? CHECK : WHITE);
    b.line(cx - 16, 40, cx - 22, 78, TRIM, 2.f);
    b.line(cx + 16, 40, cx + 22, 78, TRIM, 2.f);
    b.ellipse(cx, 18, 3, 2, AMBER);
    b.outline(15, false);
}

gs::Bitmap cabBmp(int lean) {
    gs::Bitmap b(96, 128);
    cabRear(b, float(lean));
    return b;
}

gs::Bitmap rivalBmp() {
    gs::Bitmap b(72, 96);
    b.rect(14, 58, 44, 26, 1);
    b.rect(18, 36, 36, 24, 1);
    b.poly({{18, 36}, {22, 26}, {50, 26}, {54, 36}}, 2);
    b.rect(24, 16, 24, 12, 5);
    b.rect(20, 40, 12, 12, 4);
    b.rect(40, 40, 12, 12, 4);
    b.rect(16, 66, 8, 8, 6);
    b.rect(48, 66, 8, 8, 6);
    b.rect(12, 82, 12, 8, 3);
    b.rect(48, 82, 12, 8, 3);
    b.rect(12, 80, 48, 3, 7);
    for (int i = 0; i < 6; i++) b.rect(16 + i * 7, 54, 4, 4, (i & 1) ? 8 : 9);
    b.ellipse(36, 12, 2, 2, 11);
    b.outline(15, false);
    return b;
}

gs::Bitmap pineBmp() {
    gs::Bitmap b(48, 80);
    b.rect(21, 52, 6, 24, 3);
    b.poly({{24, 6}, {44, 58}, {4, 58}}, 1);
    b.poly({{24, 18}, {40, 48}, {8, 48}}, 2);
    b.ellipse(24, 10, 4, 3, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap rockBmp() {
    gs::Bitmap b(40, 32);
    b.poly({{4, 28}, {10, 10}, {22, 4}, {36, 14}, {32, 28}}, 1);
    b.poly({{12, 16}, {20, 8}, {28, 16}, {18, 22}}, 3);
    b.outline(5, false);
    return b;
}

gs::Bitmap lampBmp() {
    gs::Bitmap b(24, 72);
    b.rect(10, 18, 4, 50, 2);
    b.ellipse(12, 12, 8, 6, 1);
    b.ellipse(12, 11, 3, 2, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap postBmp() {
    gs::Bitmap b(16, 80);
    b.rect(5, 4, 6, 74, 1);
    b.rect(3, 2, 10, 6, 2);
    b.outline(3, false);
    return b;
}

gs::Bitmap banner(const char* word, int fill) {
    gs::TextStyle st;
    st.scale = 2;
    st.color = 1;
    st.outline = 2;
    st.shadow = 0;
    gs::Bitmap t = gs::textBitmap(word, st);
    gs::Bitmap b(t.w + 16, t.h + 12);
    b.rect(0, 0, float(b.w), float(b.h), fill);
    b.blit(t, 8, 6);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    palettes(vdp);
    gs::TileAlloc tiles(vdp);
    range(vdp, tiles);
    font(tiles, art.font);
    art.cab[0] = gs::uploadMipped(vdp, cabBmp(0));
    art.cab[1] = gs::uploadMipped(vdp, cabBmp(1));
    art.rival = gs::uploadMipped(vdp, rivalBmp());
    art.pine = gs::uploadMipped(vdp, pineBmp());
    art.rock = gs::uploadMipped(vdp, rockBmp());
    art.lamp = gs::uploadMipped(vdp, lampBmp());
    art.post = gs::uploadMipped(vdp, postBmp());
    art.passBan = gs::uploadMipped(vdp, banner("PASS", 3));
    art.fareBan = gs::uploadMipped(vdp, banner("FARE", 4));
    gs::Bitmap flake(8, 8);
    flake.ellipse(4, 4, 3, 3, 1);
    art.flake = gs::uploadMipped(vdp, flake);
    gs::Bitmap shadow(48, 12);
    shadow.ellipse(24, 6, 20, 4, 1);
    art.shadow = gs::uploadMipped(vdp, shadow);

    gs::TextStyle big;
    big.scale = 3;
    big.color = 1;
    big.outline = 2;
    art.title = gs::uploadImage(vdp, gs::textBitmap("CAB PASS", big));
    gs::TextStyle sub;
    sub.scale = 1;
    sub.color = 1;
    sub.outline = 2;
    art.sub = gs::uploadImage(vdp, gs::textBitmap("BEAT THE OTHER CREW", sub));
    art.go = gs::uploadImage(vdp, gs::textBitmap("TAKE THE CAB", big));
    art.clear = gs::uploadImage(vdp, gs::textBitmap("PASS CLEAR", big));
    art.closed = gs::uploadImage(vdp, gs::textBitmap("CREW CLOSED IT", big));
    art.ditch = gs::uploadImage(vdp, gs::textBitmap("OFF THE ROAD", big));
}

}  // namespace cabpass

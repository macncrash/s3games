#include "art.h"

#include <cstdint>
#include <cstring>

namespace sledpass {
namespace {

void pal(gs::VDP& v, int p, int i, int r, int g, int b) { v.setColor(p * 16 + i, gs::rgb4(r, g, b)); }

void textPal(gs::VDP& v, int p, int r, int g, int b) {
    pal(v, p, 1, r, g, b);
    pal(v, p, 2, 1, 2, 4);
    pal(v, p, 3, 0, 0, 2);
}

void palettes(gs::VDP& v) {
    textPal(v, PAL_HUD, 15, 15, 15);
    textPal(v, PAL_GOLD, 15, 13, 4);
    textPal(v, PAL_ALERT, 15, 4, 3);

    // Dogsled from behind: cream and charcoal dogs, red parka, timber basket.
    pal(v, PAL_TEAM, 1, 14, 12, 8);
    pal(v, PAL_TEAM, 2, 9, 8, 7);
    pal(v, PAL_TEAM, 3, 4, 3, 3);
    pal(v, PAL_TEAM, 4, 13, 2, 2);
    pal(v, PAL_TEAM, 5, 7, 5, 2);
    pal(v, PAL_TEAM, 6, 11, 7, 3);
    pal(v, PAL_TEAM, 7, 12, 13, 14);
    pal(v, PAL_TEAM, 8, 5, 4, 3);
    pal(v, PAL_TEAM, 9, 6, 4, 2);
    pal(v, PAL_TEAM, 10, 13, 2, 3);
    pal(v, PAL_TEAM, 11, 15, 14, 11);
    pal(v, PAL_TEAM, 12, 12, 8, 6);
    pal(v, PAL_TEAM, 13, 15, 15, 15);
    pal(v, PAL_TEAM, 14, 15, 12, 3);
    pal(v, PAL_TEAM, 15, 2, 1, 2);

    pal(v, PAL_PINE, 1, 2, 5, 3);
    pal(v, PAL_PINE, 2, 3, 7, 4);
    pal(v, PAL_PINE, 3, 6, 9, 6);
    pal(v, PAL_PINE, 4, 6, 4, 2);
    pal(v, PAL_PINE, 5, 14, 15, 15);
    pal(v, PAL_PINE, 6, 1, 2, 2);

    pal(v, PAL_TIMBER, 1, 6, 4, 2);
    pal(v, PAL_TIMBER, 2, 10, 7, 4);
    pal(v, PAL_TIMBER, 3, 14, 15, 15);
    pal(v, PAL_TIMBER, 4, 13, 2, 2);
    pal(v, PAL_TIMBER, 5, 15, 15, 14);
    pal(v, PAL_TIMBER, 6, 15, 11, 3);
    pal(v, PAL_TIMBER, 7, 4, 3, 3);
    pal(v, PAL_TIMBER, 8, 8, 9, 10);
    pal(v, PAL_TIMBER, 9, 1, 1, 2);

    pal(v, PAL_ROCK, 1, 8, 8, 9);
    pal(v, PAL_ROCK, 2, 4, 4, 5);
    pal(v, PAL_ROCK, 3, 15, 15, 15);
    pal(v, PAL_ROCK, 4, 6, 7, 5);
    pal(v, PAL_ROCK, 5, 2, 2, 3);

    pal(v, PAL_FX, 1, 15, 15, 15);
    pal(v, PAL_FX, 2, 11, 13, 15);
    pal(v, PAL_FX, 3, 6, 7, 9);

    pal(v, PAL_BANNER, 1, 15, 15, 15);
    pal(v, PAL_BANNER, 2, 2, 1, 3);
    pal(v, PAL_BANNER, 3, 12, 2, 2);
    pal(v, PAL_BANNER, 4, 7, 1, 1);
    pal(v, PAL_BANNER, 5, 15, 12, 4);
    pal(v, PAL_BANNER, 6, 5, 3, 2);

    // Peaks with a notch over the pass, and the storm already piled on the left.
    pal(v, PAL_RANGE, 1, 6, 8, 12);
    pal(v, PAL_RANGE, 2, 4, 6, 10);
    pal(v, PAL_RANGE, 3, 3, 5, 8);
    pal(v, PAL_RANGE, 4, 14, 15, 15);
    pal(v, PAL_RANGE, 5, 2, 4, 3);
    pal(v, PAL_RANGE, 6, 6, 7, 9);
    pal(v, PAL_RANGE, 7, 10, 11, 13);
    pal(v, PAL_RANGE, 8, 15, 14, 9);
    pal(v, PAL_RANGE, 9, 3, 4, 6);

    // Packed snow, runner tracks, and the banks beside the pass.
    pal(v, PAL_SNOW, 1, 13, 14, 15);
    pal(v, PAL_SNOW, 2, 8, 10, 13);
    pal(v, PAL_SNOW, 3, 15, 15, 15);
    pal(v, PAL_SNOW, 4, 12, 13, 15);
    pal(v, PAL_SNOW, 5, 7, 9, 12);
    pal(v, PAL_SNOW, 6, 14, 15, 15);
    pal(v, PAL_SNOW, 7, 11, 13, 15);
    pal(v, PAL_SNOW, 8, 8, 10, 13);
    pal(v, PAL_SNOW, 9, 15, 15, 15);
    pal(v, PAL_SNOW, 10, 13, 14, 15);
    pal(v, PAL_SNOW, 11, 9, 11, 14);
    pal(v, PAL_SNOW, 12, 7, 9, 12);
    pal(v, PAL_SNOW, 13, 10, 12, 14);
    pal(v, PAL_SNOW, 14, 15, 15, 15);
    pal(v, PAL_SNOW, 15, 12, 14, 15);

    v.setFogColor(gs::rgb4(8, 10, 13));
}

void peak(gs::Bitmap& b, float x, float top, float half, int body, int snow) {
    const float base = float(b.h - 1);
    b.poly({{x - half, base}, {x + half, base}, {x + half * 0.16f, top + 10.f}, {x, top}, {x - half * 0.1f, top + 8.f}},
           body);
    b.poly({{x - half * 0.16f, top + 12.f}, {x, top + 1.f}, {x + half * 0.18f, top + 14.f}}, snow);
}

void range(gs::VDP& v, gs::TileAlloc& tiles) {
    gs::Bitmap b(512, 96);
    // Notch in the middle is the pass. The storm cloud sits on the left shoulder.
    peak(b, 70, 22, 120, 2, 4);
    peak(b, 200, 46, 70, 1, 4);
    peak(b, 300, 34, 48, 3, 4);
    peak(b, 430, 16, 140, 2, 4);
    peak(b, 150, 30, 40, 9, 4);
    peak(b, 360, 28, 36, 9, 4);
    b.ellipse(48, 18, 28, 12, 6);
    b.ellipse(78, 14, 22, 10, 7);
    b.ellipse(30, 26, 18, 8, 6);
    b.ellipse(470, 18, 9, 9, 8);
    b.ellipse(476, 16, 4, 4, 4);
    for (int i = 0; i < 18; i++) {
        float x = 18.f + float(i) * 28.f;
        float h = 10.f + float((i * 17) % 9);
        b.poly({{x, 90.f}, {x + 6.f, 90.f - h}, {x + 12.f, 90.f}}, 5);
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

constexpr int FUR = 1, FUR2 = 2, DARK = 3, COLLAR = 4, ROPE = 5, WOOD = 6, STEEL = 7, PAW = 8, BASKET = 9, PARKA = 10,
              HOOD = 11, SNOW = 13, LAMP = 14, INK = 15;

void dog(gs::Bitmap& b, float x, float y, float sc, int fur) {
    b.line(x - 3.1f * sc, y + 5.6f * sc, x - 2.2f * sc, y + 0.4f * sc, DARK, 2.1f * sc);
    b.line(x + 3.1f * sc, y + 5.6f * sc, x + 2.2f * sc, y + 0.4f * sc, DARK, 2.1f * sc);
    b.ellipse(x - 3.3f * sc, y + 6.2f * sc, 1.7f * sc, 1.0f * sc, PAW);
    b.ellipse(x + 3.3f * sc, y + 6.2f * sc, 1.7f * sc, 1.0f * sc, PAW);
    b.ellipse(x, y, 5.4f * sc, 4.3f * sc, fur);
    b.ellipse(x, y + 0.8f * sc, 3.3f * sc, 2.5f * sc, fur == FUR ? FUR2 : DARK);
    b.ellipse(x, y - 5.2f * sc, 3.3f * sc, 2.8f * sc, fur);
    b.poly({{x - 1.6f * sc, y - 6.2f * sc}, {x - 2.8f * sc, y - 8.4f * sc}, {x - 0.4f * sc, y - 6.8f * sc}}, DARK);
    b.poly({{x + 1.6f * sc, y - 6.2f * sc}, {x + 2.8f * sc, y - 8.4f * sc}, {x + 0.4f * sc, y - 6.8f * sc}}, DARK);
    b.line(x + 3.2f * sc, y + 0.6f * sc, x + 7.2f * sc, y - 2.8f * sc, fur, 1.8f * sc);
    b.ellipse(x, y - 3.4f * sc, 2.4f * sc, 0.9f * sc, COLLAR);
}

gs::Bitmap teamBmp(int lean) {
    gs::Bitmap b(120, 132);
    const float s = float(lean) * 8.f;
    b.line(46 + s * 0.3f, 34, 58, 78, ROPE, 1.5f);
    b.line(74 + s * 0.3f, 34, 62, 78, ROPE, 1.5f);
    b.line(40 + s, 58, 58, 84, ROPE, 1.6f);
    b.line(80 + s, 58, 62, 84, ROPE, 1.6f);
    dog(b, 46 + s, 32, 0.92f, FUR);
    dog(b, 74 + s, 32, 0.92f, FUR2);
    dog(b, 38 + s * 0.8f, 58, 1.12f, FUR2);
    dog(b, 82 + s * 0.8f, 58, 1.12f, FUR);
    b.line(36, 86, 30, 124, STEEL, 3.4f);
    b.line(84, 86, 90, 124, STEEL, 3.4f);
    b.line(36, 86, 44, 74, STEEL, 3.0f);
    b.line(84, 86, 76, 74, STEEL, 3.0f);
    b.line(44, 74, 76, 74, WOOD, 2.6f);
    b.poly({{38, 84}, {82, 84}, {88, 112}, {32, 112}}, WOOD);
    b.poly({{44, 90}, {76, 90}, {78, 106}, {42, 106}}, BASKET);
    b.line(40, 88, 36, 108, ROPE, 2.0f);
    b.line(80, 88, 84, 108, ROPE, 2.0f);
    b.line(32, 110, 88, 110, STEEL, 3.2f);
    const float mx = 60 + s;
    b.ellipse(mx, 100, 12, 14, PARKA);
    b.ellipse(mx, 84, 8, 7, PARKA);
    b.ellipse(mx, 82, 8.2f, 3.4f, HOOD);
    b.ellipse(mx, 86, 3.4f, 3.2f, DARK);
    b.line(mx - 10, 98, 34, 110, PARKA, 3.4f);
    b.line(mx + 10, 98, 86, 110, PARKA, 3.4f);
    b.ellipse(34, 110, 2.2f, 1.8f, HOOD);
    b.ellipse(86, 110, 2.2f, 1.8f, HOOD);
    b.ellipse(mx + 8, 78, 1.6f, 1.6f, LAMP);
    b.ellipse(60, 126, 18, 4, SNOW);
    b.outline(INK, false);
    return b;
}

gs::Bitmap pineBmp() {
    gs::Bitmap b(40, 72);
    b.rect(17, 50, 6, 18, 4);
    b.poly({{20, 58}, {4, 58}, {20, 34}, {36, 58}}, 1);
    b.poly({{20, 42}, {7, 42}, {20, 20}, {33, 42}}, 2);
    b.poly({{20, 28}, {10, 28}, {20, 8}, {30, 28}}, 3);
    b.ellipse(14, 40, 3, 1.4f, 5);
    b.ellipse(26, 26, 3, 1.3f, 5);
    b.ellipse(18, 16, 2.4f, 1.2f, 5);
    b.outline(6, false);
    return b;
}

gs::Bitmap stakeBmp() {
    gs::Bitmap b(14, 52);
    b.rect(6, 8, 3, 40, 1);
    b.rect(5, 44, 5, 4, 2);
    b.rect(2, 10, 10, 6, 4);
    b.rect(2, 16, 10, 4, 5);
    b.poly({{7, 4}, {12, 12}, {2, 12}}, 3);
    b.outline(9, false);
    return b;
}

gs::Bitmap cabinBmp() {
    gs::Bitmap b(52, 44);
    b.poly({{2, 18}, {26, 4}, {50, 18}}, 3);
    b.rect(6, 16, 40, 22, 1);
    for (int y = 18; y < 36; y += 4) b.rect(6, y, 40, 2, 2);
    b.rect(22, 24, 8, 14, 7);
    b.rect(10, 20, 8, 7, 6);
    b.rect(34, 20, 8, 7, 6);
    b.rect(12, 22, 4, 3, 5);
    b.rect(36, 22, 4, 3, 5);
    b.rect(24, 2, 4, 8, 7);
    b.outline(9, false);
    return b;
}

gs::Bitmap boulderBmp() {
    gs::Bitmap b(36, 30);
    b.ellipse(18, 18, 14, 10, 1);
    b.ellipse(14, 16, 6, 4, 2);
    b.ellipse(18, 10, 8, 3.2f, 3);
    b.line(12, 18, 22, 14, 4, 1.2f);
    b.outline(5, false);
    return b;
}

gs::Bitmap driftBmp() {
    gs::Bitmap b(44, 26);
    b.ellipse(22, 16, 18, 8, 2);
    b.ellipse(20, 14, 14, 6, 1);
    b.ellipse(16, 12, 4, 2.2f, 1);
    b.ellipse(28, 11, 3, 1.6f, 1);
    return b;
}

gs::Bitmap postBmp() {
    gs::Bitmap b(16, 76);
    b.rect(6, 8, 5, 62, 1);
    for (int y = 10; y < 66; y += 8) b.rect(6, y, 5, 3, 2);
    b.rect(3, 66, 10, 6, 1);
    b.poly({{8, 2}, {14, 12}, {2, 12}}, 3);
    b.outline(9, false);
    return b;
}

gs::Bitmap bannerBmp(const char* word) {
    gs::Bitmap b(116, 40);
    for (int x = 0; x < 116; x += 8) {
        b.rect(x, 0, 4, 6, 5);
        b.rect(x + 4, 0, 4, 6, 6);
    }
    b.rect(4, 8, 108, 26, 3);
    b.rect(4, 8, 108, 4, 4);
    gs::TextStyle st;
    st.scale = 2;
    st.color = 1;
    st.outline = 2;
    st.spacing = 1;
    gs::Bitmap t = gs::textBitmap(word, st);
    b.blit(t, (116 - t.w) / 2, 14);
    b.outline(2, false);
    return b;
}

gs::Bitmap flakeBmp() {
    gs::Bitmap b(7, 7);
    b.line(3, 0, 3, 6, 1, 1.2f);
    b.line(0, 3, 6, 3, 2, 1.2f);
    b.set(1, 1, 1);
    b.set(5, 1, 1);
    b.set(1, 5, 2);
    b.set(5, 5, 2);
    return b;
}

gs::Bitmap shadowBmp() {
    gs::Bitmap b(64, 16);
    b.ellipse(32, 8, 26, 6, 1);
    return b;
}

gs::Image words(gs::VDP& v, const char* s, int scale, int ink) {
    gs::TextStyle st;
    st.scale = scale;
    st.color = 1;
    st.outline = 2;
    st.shadow = 3;
    st.spacing = 1;
    gs::Image img = gs::uploadImage(v, gs::textBitmap(s, st));
    (void)ink;
    return img;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    palettes(vdp);
    gs::TileAlloc tiles(vdp, 1);
    range(vdp, tiles);
    font(tiles, art.font);
    art.team[0] = gs::uploadMipped(vdp, teamBmp(0));
    art.team[1] = gs::uploadMipped(vdp, teamBmp(-1));
    art.pine = gs::uploadMipped(vdp, pineBmp());
    art.stake = gs::uploadMipped(vdp, stakeBmp());
    art.cabin = gs::uploadMipped(vdp, cabinBmp());
    art.boulder = gs::uploadMipped(vdp, boulderBmp());
    art.drift = gs::uploadMipped(vdp, driftBmp());
    art.post = gs::uploadMipped(vdp, postBmp());
    art.mushBan = gs::uploadMipped(vdp, bannerBmp("MUSH"));
    art.passBan = gs::uploadMipped(vdp, bannerBmp("PASS"));
    art.flake = gs::uploadMipped(vdp, flakeBmp());
    art.shadow = gs::uploadMipped(vdp, shadowBmp());
    art.title = words(vdp, "S3 SLED PASS", 3, PAL_GOLD);
    art.sub = words(vdp, "CLEAR THE PASS", 2, PAL_HUD);
    art.go = words(vdp, "MUSH", 4, PAL_GOLD);
    art.clear = words(vdp, "PASS CLEAR", 3, PAL_GOLD);
    art.storm = words(vdp, "STORM CLOSED", 3, PAL_ALERT);
    art.buried = words(vdp, "IN THE BANK", 3, PAL_ALERT);
}

}  // namespace sledpass

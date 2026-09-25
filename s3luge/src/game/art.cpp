#include "game/art.h"

#include <cstdint>
#include <cstring>

namespace luge {
namespace {

void pal(gs::VDP& v, int p, int i, int r, int g, int b) { v.setColor(p * 16 + i, gs::rgb4(r, g, b)); }

void textPal(gs::VDP& v, int p, int r, int g, int b) {
    pal(v, p, 1, r, g, b);
    pal(v, p, 2, 1, 2, 5);
    pal(v, p, 3, 0, 0, 2);
}

void palettes(gs::VDP& v) {
    textPal(v, PAL_HUD, 15, 15, 15);
    textPal(v, PAL_GOLD, 15, 12, 4);
    textPal(v, PAL_ALERT, 15, 4, 3);
    textPal(v, PAL_TITLE, 15, 15, 15);

    // Prone luger: red suit, white helmet, steel runners.
    pal(v, PAL_RIDER, 1, 13, 1, 2);
    pal(v, PAL_RIDER, 2, 7, 0, 1);
    pal(v, PAL_RIDER, 3, 15, 15, 15);
    pal(v, PAL_RIDER, 4, 15, 12, 3);
    pal(v, PAL_RIDER, 5, 2, 2, 4);
    pal(v, PAL_RIDER, 6, 10, 12, 14);
    pal(v, PAL_RIDER, 7, 14, 15, 15);
    pal(v, PAL_RIDER, 8, 4, 4, 7);
    pal(v, PAL_RIDER, 9, 12, 12, 13);
    pal(v, PAL_RIDER, 10, 6, 14, 15);
    pal(v, PAL_RIDER, 11, 1, 1, 2);

    // Course lamps. Indices 1 and 2 stripe the pole.
    pal(v, PAL_POST, 1, 3, 5, 8);
    pal(v, PAL_POST, 2, 8, 10, 13);
    pal(v, PAL_POST, 3, 15, 12, 5);
    pal(v, PAL_POST, 4, 15, 15, 12);
    pal(v, PAL_POST, 5, 1, 1, 2);

    // Finish posts: the same bitmap reads as checks.
    pal(v, PAL_CHECK, 1, 1, 1, 2);
    pal(v, PAL_CHECK, 2, 15, 15, 15);
    pal(v, PAL_CHECK, 3, 15, 12, 3);
    pal(v, PAL_CHECK, 4, 15, 15, 14);
    pal(v, PAL_CHECK, 5, 1, 1, 1);

    pal(v, PAL_FX, 1, 15, 15, 15);
    pal(v, PAL_FX, 2, 10, 14, 15);

    // Dawn canyon behind the chute.
    pal(v, PAL_CANYON, 1, 5, 7, 12);
    pal(v, PAL_CANYON, 2, 7, 10, 14);
    pal(v, PAL_CANYON, 3, 3, 5, 10);
    pal(v, PAL_CANYON, 4, 14, 15, 15);
    pal(v, PAL_CANYON, 5, 2, 3, 7);
    pal(v, PAL_CANYON, 6, 12, 8, 7);
    pal(v, PAL_CANYON, 7, 15, 15, 14);
    pal(v, PAL_CANYON, 8, 14, 14, 11);

    // Gate cloth. Text is baked in as indices 1 and 2.
    pal(v, PAL_BANNER, 1, 15, 15, 15);
    pal(v, PAL_BANNER, 2, 1, 1, 4);
    pal(v, PAL_BANNER, 3, 11, 1, 2);
    pal(v, PAL_BANNER, 4, 6, 0, 1);
    pal(v, PAL_BANNER, 5, 15, 12, 4);
    pal(v, PAL_BANNER, 6, 8, 6, 2);

    // Chute: glassy floor, amber lip, blue ice walls to the screen edge.
    pal(v, PAL_ICE, 1, 5, 8, 14);
    pal(v, PAL_ICE, 2, 2, 5, 11);
    pal(v, PAL_ICE, 3, 8, 12, 15);
    pal(v, PAL_ICE, 4, 15, 11, 4);
    pal(v, PAL_ICE, 5, 12, 7, 2);
    pal(v, PAL_ICE, 6, 11, 15, 15);
    pal(v, PAL_ICE, 7, 7, 12, 15);
    pal(v, PAL_ICE, 8, 6, 9, 13);
    pal(v, PAL_ICE, 9, 14, 15, 15);
    pal(v, PAL_ICE, 10, 15, 15, 15);
    pal(v, PAL_ICE, 11, 4, 8, 13);
    pal(v, PAL_ICE, 12, 3, 7, 12);
    pal(v, PAL_ICE, 13, 6, 10, 14);
    pal(v, PAL_ICE, 14, 13, 15, 15);
    pal(v, PAL_ICE, 15, 15, 15, 15);

    v.setFogColor(gs::rgb4(10, 13, 15));
}

void cliff(gs::Bitmap& b, float x, float w, float top, int c) {
    const float base = float(b.h - 1);
    b.poly({{x, base}, {x + w, base}, {x + w * 0.78f, top + 12.f}, {x + w * 0.22f, top}}, c);
}

void canyon(gs::VDP& v, gs::TileAlloc& tiles) {
    gs::Bitmap b(512, 104);
    const struct {
        float x, w, top;
        int c;
    } faces[] = {
        {0, 150, 18, 3},   {18, 120, 8, 1},    {40, 70, 28, 2},
        {250, 160, 12, 3}, {270, 130, 4, 1},   {300, 80, 26, 2},
        {430, 90, 20, 1},  {448, 64, 10, 2},
    };
    for (auto& f : faces) cliff(b, f.x, f.w, f.top, f.c);
    // Snow caps and a few ice streaks.
    b.poly({{48, 22}, {78, 18}, {96, 36}, {40, 40}}, 4);
    b.poly({{292, 16}, {330, 12}, {348, 34}, {280, 36}}, 4);
    b.poly({{460, 24}, {492, 20}, {508, 40}, {448, 42}}, 4);
    b.rect(86, 30, 3, 28, 4);
    b.rect(334, 28, 3, 34, 4);
    b.rect(24, 48, 2, 22, 2);
    b.rect(390, 40, 2, 30, 2);

    for (int y = b.h - 16; y < b.h; y++)
        for (int x = 0; x < b.w; x++)
            if (!b.get(x, y)) b.set(x, y, 6);

    for (int i = 0; i < 40; i++) {
        uint32_t h = uint32_t(i * 1103515245u + 12345u);
        int x = int(h % 512u);
        int y = int((h >> 8) % 36u);
        if (!b.get(x, y)) b.set(x, y, 7);
    }
    b.ellipse(196, 24, 11, 11, 8);
    b.ellipse(202, 22, 9, 9, 0);
    gs::bitmapToPlane(tiles, v.B, 0, 0, b, PAL_CANYON);
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

gs::Bitmap rider(int pose) {
    gs::Bitmap b(72, 64);
    const int lean = pose == 2 ? -8 : 0;
    const int tuck = pose == 1 ? 5 : 0;
    b.line(16, 60, 24, 34 + tuck, 6, 2.6f);
    b.line(56, 60, 48, 34 + tuck, 6, 2.6f);
    b.line(18, 58, 26, 36 + tuck, 7, 1.2f);
    b.line(54, 58, 46, 36 + tuck, 7, 1.2f);
    b.poly({{20, 58}, {52, 58}, {46, float(36 + tuck)}, {26, float(36 + tuck)}}, 5);
    b.poly({{24, 54}, {48, 54}, {44, float(40 + tuck)}, {28, float(40 + tuck)}}, 8);
    b.ellipse(36 + lean, 30 + tuck, 16, 11, 1);
    b.ellipse(36 + lean, 29 + tuck, 11, 7, 2);
    b.line(22 + lean, 32 + tuck, 14, 44, 1, 3.2f);
    b.line(50 + lean, 32 + tuck, 58, 44, 1, 3.2f);
    b.ellipse(13, 45, 3.2f, 2.4f, 9);
    b.ellipse(59, 45, 3.2f, 2.4f, 9);
    b.line(24 + lean, 34 + tuck, 48 + lean, 34 + tuck, 4, 1.6f);
    b.ellipse(36 + lean * 1.15f, 16 + tuck, 10, 9, 3);
    b.ellipse(36 + lean * 1.15f, 17 + tuck, 7, 5, 10);
    b.rect(28 + lean, 16 + tuck, 16, 3, 4);
    b.outline(11, false);
    b.ellipse(36 + lean * 1.15f, 18 + tuck, 4.5f, 2.6f, 10);
    b.rect(31 + lean, 17 + tuck, 10, 2, 4);
    return b;
}

gs::Bitmap postBmp() {
    gs::Bitmap b(16, 72);
    b.rect(6, 10, 4, 56, 1);
    for (int y = 10; y < 64; y += 8) b.rect(6, y, 4, 4, 2);
    b.rect(4, 64, 8, 5, 1);
    b.ellipse(8, 8, 5, 5, 3);
    b.ellipse(8, 8, 2.2f, 2.2f, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap beamBmp() {
    gs::Bitmap b(40, 8);
    b.rect(0, 2, 40, 4, 1);
    b.rect(0, 3, 40, 2, 2);
    for (int x = 2; x < 40; x += 8) b.rect(x, 1, 3, 6, 3);
    return b;
}

gs::Bitmap bannerBmp(const char* word) {
    gs::Bitmap b(112, 36);
    for (int x = 0; x < 112; x += 8) {
        b.rect(x, 0, 4, 6, 5);
        b.rect(x + 4, 0, 4, 6, 6);
    }
    b.rect(2, 8, 108, 24, 3);
    b.rect(2, 8, 108, 3, 4);
    gs::TextStyle st;
    st.scale = 2;
    st.color = 1;
    st.outline = 2;
    st.spacing = 1;
    gs::Bitmap t = gs::textBitmap(word, st);
    b.blit(t, (112 - t.w) / 2, 12);
    b.outline(2, false);
    return b;
}

gs::Bitmap sparkBmp() {
    gs::Bitmap b(7, 7);
    b.line(3, 0, 3, 6, 1, 1);
    b.line(0, 3, 6, 3, 2, 1);
    b.line(1, 1, 5, 5, 1, 1);
    b.line(5, 1, 1, 5, 2, 1);
    return b;
}

gs::Bitmap shadowBmp() {
    gs::Bitmap b(48, 14);
    b.ellipse(24, 7, 20, 5, 1);
    return b;
}

gs::Image words(gs::VDP& v, const char* s, int scale) {
    gs::TextStyle st;
    st.scale = scale;
    st.color = 1;
    st.outline = 2;
    st.shadow = 3;
    st.spacing = 1;
    return gs::uploadImage(v, gs::textBitmap(s, st));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    palettes(vdp);
    gs::TileAlloc tiles(vdp, 1);
    canyon(vdp, tiles);
    font(tiles, art.font);
    art.flat = gs::uploadMipped(vdp, rider(0));
    art.tuck = gs::uploadMipped(vdp, rider(1));
    art.lean = gs::uploadMipped(vdp, rider(2));
    art.post = gs::uploadMipped(vdp, postBmp());
    art.beam = gs::uploadMipped(vdp, beamBmp());
    art.dropBan = gs::uploadMipped(vdp, bannerBmp("DROP"));
    art.finishBan = gs::uploadMipped(vdp, bannerBmp("FINISH"));
    art.spark = gs::uploadMipped(vdp, sparkBmp());
    art.shadow = gs::uploadMipped(vdp, shadowBmp());
    art.title = words(vdp, "S3 LUGE", 4);
    art.sub = words(vdp, "STAY OFF THE WALLS", 2);
    art.num[0] = words(vdp, "3", 5);
    art.num[1] = words(vdp, "2", 5);
    art.num[2] = words(vdp, "1", 5);
    art.clean = words(vdp, "CLEAN RUN", 3);
    art.hit = words(vdp, "HIT THE WALL", 3);
    art.slow = words(vdp, "TOO SLOW", 3);
}

}  // namespace luge

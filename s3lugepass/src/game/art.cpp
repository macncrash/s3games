#include "game/art.h"

#include <cstring>

namespace luge {
namespace {

void pal(gs::VDP& v, int p, int i, int r, int g, int b) { v.setColor(p * 16 + i, gs::rgb4(r, g, b)); }

void palettes(gs::VDP& v) {
    for (int p = 0; p < 16; p++) pal(v, p, 0, 0, 0, 0);

    pal(v, PAL_INK, 1, 14, 15, 15);
    pal(v, PAL_INK, 2, 1, 2, 4);
    pal(v, PAL_HOT, 1, 15, 12, 4);
    pal(v, PAL_HOT, 2, 3, 2, 1);
    pal(v, PAL_BAD, 1, 15, 4, 4);
    pal(v, PAL_BAD, 2, 3, 0, 1);
    pal(v, PAL_GOOD, 1, 8, 15, 12);
    pal(v, PAL_GOOD, 2, 1, 3, 3);

    pal(v, PAL_RIDER, 1, 12, 1, 2);
    pal(v, PAL_RIDER, 2, 15, 3, 4);
    pal(v, PAL_RIDER, 3, 15, 12, 8);
    pal(v, PAL_RIDER, 4, 2, 2, 3);
    pal(v, PAL_RIDER, 5, 9, 10, 12);
    pal(v, PAL_RIDER, 6, 14, 15, 15);
    pal(v, PAL_RIDER, 7, 4, 5, 7);
    pal(v, PAL_RIDER, 8, 15, 14, 6);
    pal(v, PAL_RIDER, 9, 1, 1, 2);
    pal(v, PAL_RIDER, 10, 6, 7, 9);

    pal(v, PAL_PINE, 1, 1, 4, 2);
    pal(v, PAL_PINE, 2, 2, 7, 3);
    pal(v, PAL_PINE, 3, 14, 15, 15);
    pal(v, PAL_PINE, 4, 6, 7, 8);
    pal(v, PAL_PINE, 5, 3, 3, 4);

    pal(v, PAL_ROCK, 1, 5, 5, 6);
    pal(v, PAL_ROCK, 2, 8, 8, 9);
    pal(v, PAL_ROCK, 3, 13, 14, 15);
    pal(v, PAL_ROCK, 4, 2, 2, 3);
    pal(v, PAL_ROCK, 5, 10, 6, 3);

    pal(v, PAL_SPRAY, 1, 10, 12, 14);
    pal(v, PAL_SPRAY, 2, 14, 15, 15);
    pal(v, PAL_SPRAY, 3, 6, 8, 10);

    pal(v, PAL_TITLE, 1, 15, 14, 12);
    pal(v, PAL_TITLE, 2, 2, 4, 8);
    pal(v, PAL_TITLE, 3, 1, 1, 3);
    pal(v, PAL_TITLE, 4, 12, 14, 15);
    pal(v, PAL_TITLE, 5, 15, 6, 3);

    pal(v, PAL_BANNER, 1, 1, 2, 4);
    pal(v, PAL_BANNER, 2, 15, 15, 15);
    pal(v, PAL_BANNER, 3, 12, 2, 3);
    pal(v, PAL_BANNER, 4, 15, 12, 3);
    pal(v, PAL_BANNER, 5, 4, 6, 8);

    pal(v, PAL_SKY, 1, 6, 8, 11);
    pal(v, PAL_SKY, 2, 4, 6, 9);
    pal(v, PAL_SKY, 3, 10, 11, 13);
    pal(v, PAL_SKY, 4, 14, 15, 15);
    pal(v, PAL_SKY, 5, 8, 9, 11);
    pal(v, PAL_SKY, 6, 3, 4, 6);
    pal(v, PAL_SKY, 7, 12, 13, 14);
    pal(v, PAL_SKY, 8, 2, 3, 5);
    pal(v, PAL_SKY, 9, 7, 8, 9);
    pal(v, PAL_SKY, 10, 13, 14, 15);

    // Ice chute. Indices match the road chip: 1-3 snow ground, 6-7 sheet, 14 glare.
    pal(v, PAL_ROAD, 1, 11, 12, 14);
    pal(v, PAL_ROAD, 2, 8, 10, 13);
    pal(v, PAL_ROAD, 3, 14, 15, 15);
    pal(v, PAL_ROAD, 4, 9, 11, 13);
    pal(v, PAL_ROAD, 5, 7, 9, 12);
    pal(v, PAL_ROAD, 6, 10, 13, 15);
    pal(v, PAL_ROAD, 7, 7, 11, 14);
    pal(v, PAL_ROAD, 8, 13, 14, 15);
    pal(v, PAL_ROAD, 9, 12, 14, 15);
    pal(v, PAL_ROAD, 10, 15, 15, 15);
    pal(v, PAL_ROAD, 14, 15, 15, 15);
    pal(v, PAL_ROAD, 15, 6, 9, 12);
    v.setFogColor(gs::rgb4(7, 8, 10));
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
    gs::Bitmap b(320, 120);
    for (int y = 0; y < 48; y++) {
        int c = y < 16 ? 8 : y < 32 ? 6 : 2;
        b.rect(0, float(y), 320, 1, c);
    }
    for (int i = 0; i < 18; i++) {
        float x = float((i * 53) % 300);
        float y = 6.f + float((i * 11) % 22);
        b.ellipse(x, y, 18 + (i % 5) * 4.f, 7.f, i % 2 ? 5 : 3);
    }
    struct Peak {
        float x, h, w;
    };
    const Peak peaks[] = {{20, 70, 46}, {70, 92, 58}, {130, 64, 40}, {180, 100, 70},
                           {240, 78, 48}, {290, 88, 54}};
    for (const Peak& p : peaks) {
        b.poly({{p.x - p.w, 120}, {p.x, 120 - p.h}, {p.x + p.w, 120}}, 9);
        b.poly({{p.x - p.w * 0.28f, 120 - p.h * 0.62f},
                {p.x, 120 - p.h},
                {p.x + p.w * 0.28f, 120 - p.h * 0.62f}},
               4);
    }
    b.rect(0, 112, 320, 8, 10);
    gs::bitmapToPlane(tiles, v.B, 0, 0, b, PAL_SKY);
}

gs::Bitmap riderFrame(int lean) {
    gs::Bitmap b(48, 64);
    float n = float(lean - 2) * 3.2f;
    auto X = [&](float x) { return x + n; };
    b.poly({{X(8), 40}, {X(40), 40}, {X(44), 52}, {X(4), 52}}, 5);
    b.rect(X(6), 50, 36, 3, 7);
    b.line(X(6), 46, X(42), 46, 10, 2);
    b.ellipse(X(24), 28, 8, 11, 1);
    b.ellipse(X(24), 26, 6, 8, 2);
    b.ellipse(X(24 + n * 0.15f), 16, 6, 6, 3);
    b.rect(X(20), 12, 8, 3, 8);
    b.rect(X(18), 18, 3, 2, 6);
    b.rect(X(27), 18, 3, 2, 6);
    b.line(X(16), 30, X(8), 38, 2, 2);
    b.line(X(32), 30, X(40), 38, 2, 2);
    b.ellipse(X(10), 54, 3, 2, 4);
    b.ellipse(X(38), 54, 3, 2, 4);
    return b;
}

gs::Bitmap pineBmp() {
    gs::Bitmap b(28, 48);
    b.rect(12, 36, 4, 12, 5);
    b.poly({{14, 2}, {26, 22}, {2, 22}}, 1);
    b.poly({{14, 12}, {26, 32}, {2, 32}}, 2);
    b.poly({{14, 22}, {24, 40}, {4, 40}}, 1);
    b.poly({{10, 6}, {14, 2}, {16, 8}}, 3);
    return b;
}

gs::Bitmap rockBmp() {
    gs::Bitmap b(28, 22);
    b.poly({{2, 18}, {6, 8}, {14, 3}, {22, 9}, {26, 18}}, 1);
    b.poly({{8, 16}, {12, 8}, {18, 10}, {20, 16}}, 2);
    b.rect(6, 17, 16, 3, 3);
    b.rect(10, 6, 3, 2, 4);
    return b;
}

gs::Bitmap sprayBmp() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 6, 4, 1);
    b.ellipse(8, 8, 3, 2, 2);
    return b;
}

gs::Bitmap shadowBmp() {
    gs::Bitmap b(40, 10);
    b.ellipse(20, 5, 16, 3, 1);
    return b;
}

gs::Bitmap banner(const char* word, int ink) {
    gs::TextStyle st;
    st.scale = 2;
    st.color = ink;
    st.outline = 1;
    st.spacing = 1;
    gs::Bitmap wordB = gs::textBitmap(word, st);
    gs::Bitmap b(wordB.w + 16, wordB.h + 10);
    b.rect(0, 0, float(b.w), float(b.h), 5);
    b.rect(2, 2, float(b.w - 4), float(b.h - 4), 1);
    b.blit(wordB, 8, 5);
    return b;
}

gs::Image uploadText(gs::VDP& v, const char* s, int scale, int color, int outline) {
    gs::TextStyle st;
    st.scale = scale;
    st.color = color;
    st.outline = outline;
    st.spacing = 1;
    return gs::uploadImage(v, gs::textBitmap(s, st));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    palettes(vdp);
    gs::TileAlloc tiles(vdp);
    fontTiles(tiles, art.font);
    skyline(vdp, tiles);
    for (int i = 0; i < 5; i++) art.rider[i] = gs::uploadMipped(vdp, riderFrame(i));
    art.pine = gs::uploadMipped(vdp, pineBmp());
    art.rock = gs::uploadMipped(vdp, rockBmp());
    art.spray = gs::uploadMipped(vdp, sprayBmp());
    art.shadow = gs::uploadMipped(vdp, shadowBmp());
    art.bannerOpen = gs::uploadMipped(vdp, banner("OPEN", 2));
    art.bannerShut = gs::uploadMipped(vdp, banner("STORM", 4));
    art.title = uploadText(vdp, "LUGE PASS", 3, 1, 3);
    art.sub = uploadText(vdp, "BEAT THE STORM", 1, 5, 2);
    art.count[0] = uploadText(vdp, "3", 4, 1, 3);
    art.count[1] = uploadText(vdp, "2", 4, 1, 3);
    art.count[2] = uploadText(vdp, "1", 4, 1, 3);
    art.count[3] = uploadText(vdp, "DROP", 3, 5, 3);
    art.cleared = uploadText(vdp, "PASS CLEAR", 2, 4, 2);
    art.buried = uploadText(vdp, "IN THE WALL", 2, 5, 3);
    art.late = uploadText(vdp, "STORM CLOSED", 2, 5, 3);
    art.paused = uploadText(vdp, "HELD", 3, 1, 3);
}

}  // namespace luge

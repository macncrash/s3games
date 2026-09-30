#include "game/art.h"

#include <cstring>

namespace kartpass {
namespace {

void pal(gs::VDP& v, int p, int i, int r, int g, int b) { v.setColor(p * 16 + i, gs::rgb4(r, g, b)); }

void palettes(gs::VDP& v) {
    for (int p = 0; p < 16; p++) pal(v, p, 0, 0, 0, 0);

    pal(v, PAL_INK, 1, 14, 15, 15);
    pal(v, PAL_INK, 2, 1, 2, 4);
    pal(v, PAL_HOT, 1, 15, 11, 2);
    pal(v, PAL_HOT, 2, 3, 2, 0);
    pal(v, PAL_BAD, 1, 15, 4, 3);
    pal(v, PAL_BAD, 2, 3, 0, 1);
    pal(v, PAL_GOOD, 1, 6, 15, 11);
    pal(v, PAL_GOOD, 2, 1, 3, 2);

    pal(v, PAL_KART, 1, 12, 2, 2);
    pal(v, PAL_KART, 2, 15, 12, 2);
    pal(v, PAL_KART, 3, 2, 2, 3);
    pal(v, PAL_KART, 4, 1, 1, 2);
    pal(v, PAL_KART, 5, 8, 8, 9);
    pal(v, PAL_KART, 6, 14, 14, 15);
    pal(v, PAL_KART, 7, 4, 6, 12);
    pal(v, PAL_KART, 8, 15, 6, 2);
    pal(v, PAL_KART, 9, 3, 3, 4);

    pal(v, PAL_PEAK, 1, 2, 4, 3);
    pal(v, PAL_PEAK, 2, 4, 6, 4);
    pal(v, PAL_PEAK, 3, 14, 15, 15);
    pal(v, PAL_PEAK, 4, 8, 9, 10);
    pal(v, PAL_PEAK, 5, 3, 3, 4);

    pal(v, PAL_BALE, 1, 10, 7, 2);
    pal(v, PAL_BALE, 2, 13, 10, 3);
    pal(v, PAL_BALE, 3, 6, 4, 1);
    pal(v, PAL_BALE, 4, 14, 12, 6);

    pal(v, PAL_SPRAY, 1, 12, 13, 14);
    pal(v, PAL_SPRAY, 2, 8, 10, 12);

    pal(v, PAL_TITLE, 1, 15, 14, 8);
    pal(v, PAL_TITLE, 2, 2, 3, 6);
    pal(v, PAL_TITLE, 3, 15, 5, 2);
    pal(v, PAL_TITLE, 4, 8, 13, 15);

    pal(v, PAL_GATE, 1, 14, 14, 15);
    pal(v, PAL_GATE, 2, 12, 2, 2);
    pal(v, PAL_GATE, 3, 2, 4, 6);
    pal(v, PAL_GATE, 4, 15, 11, 2);

    pal(v, PAL_SKY, 1, 4, 6, 10);
    pal(v, PAL_SKY, 2, 3, 4, 7);
    pal(v, PAL_SKY, 3, 7, 8, 11);
    pal(v, PAL_SKY, 4, 13, 14, 15);
    pal(v, PAL_SKY, 5, 5, 5, 6);
    pal(v, PAL_SKY, 6, 9, 10, 12);
    pal(v, PAL_SKY, 7, 3, 4, 5);
    pal(v, PAL_SKY, 8, 11, 12, 14);

    pal(v, PAL_ROAD, 1, 4, 4, 5);
    pal(v, PAL_ROAD, 2, 3, 3, 4);
    pal(v, PAL_ROAD, 3, 6, 6, 6);
    pal(v, PAL_ROAD, 4, 12, 13, 14);
    pal(v, PAL_ROAD, 5, 8, 9, 10);
    pal(v, PAL_ROAD, 6, 5, 6, 7);
    pal(v, PAL_ROAD, 7, 2, 3, 4);
    pal(v, PAL_ROAD, 8, 9, 10, 11);
    pal(v, PAL_ROAD, 14, 14, 12, 3);
    pal(v, PAL_ROAD, 15, 7, 7, 8);
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
    gs::Bitmap b(320, 104);
    for (int y = 0; y < 64; y++) {
        int c = y < 18 ? 2 : y < 42 ? 1 : 3;
        b.rect(0, float(y), 320, 1, c);
    }
    for (int i = 0; i < 6; i++) b.ellipse(30.f + float(i) * 52.f, 16.f, 18.f, 6.f, i & 1 ? 6 : 8);
    struct Peak {
        float x, h, w;
    };
    const Peak peaks[] = {{24, 70, 40}, {78, 88, 50}, {140, 54, 32}, {190, 92, 58}, {250, 66, 40}, {300, 80, 46}};
    for (const Peak& p : peaks) {
        b.poly({{p.x - p.w, 104}, {p.x, 104 - p.h}, {p.x + p.w, 104}}, 7);
        b.poly({{p.x - p.w * 0.28f, 104 - p.h * 0.62f}, {p.x, 104 - p.h}, {p.x + p.w * 0.28f, 104 - p.h * 0.62f}}, 4);
    }
    gs::bitmapToPlane(tiles, v.B, 0, 0, b, PAL_SKY);
}

gs::Bitmap rearKart(int lean) {
    gs::Bitmap b(56, 52);
    float dx = float(lean) * 2.2f;
    b.rect(8 + dx, 8, 40, 18, 1);
    b.rect(10 + dx, 10, 36, 6, 2);
    b.rect(18 + dx, 16, 20, 8, 6);
    b.rect(24 + dx, 17, 8, 6, 3);
    b.rect(4, 22, 10, 16, 4);
    b.rect(42, 22, 10, 16, 4);
    b.ellipse(9, 30, 6, 9, 5);
    b.ellipse(47, 30, 6, 9, 5);
    b.rect(14 + dx * 0.4f, 26, 28, 14, 1);
    b.rect(20 + dx * 0.4f, 30, 16, 6, 8);
    b.rect(6 + dx, 4, 44, 4, 3);
    b.rect(2, 6, 8, 3, 9);
    b.rect(46, 6, 8, 3, 9);
    b.rect(26 + dx, 38, 4, 6, 5);
    return b;
}

gs::Bitmap pine() {
    gs::Bitmap b(28, 48);
    b.poly({{14, 2}, {2, 22}, {26, 22}}, 1);
    b.poly({{14, 12}, {0, 32}, {28, 32}}, 2);
    b.poly({{14, 4}, {8, 14}, {20, 14}}, 3);
    b.rect(12, 32, 4, 14, 5);
    return b;
}

gs::Bitmap hay() {
    gs::Bitmap b(28, 22);
    b.ellipse(14, 12, 12, 8, 1);
    b.ellipse(14, 11, 8, 5, 2);
    b.line(4, 8, 24, 8, 3, 1);
    b.line(5, 14, 23, 14, 3, 1);
    b.rect(12, 4, 4, 14, 4);
    return b;
}

gs::Bitmap flake() {
    gs::Bitmap b(12, 12);
    b.ellipse(6, 6, 4, 4, 1);
    b.ellipse(6, 6, 2, 2, 2);
    return b;
}

gs::Bitmap archway() {
    gs::Bitmap b(48, 36);
    b.rect(2, 8, 6, 26, 1);
    b.rect(40, 8, 6, 26, 1);
    b.rect(2, 4, 44, 8, 2);
    b.rect(8, 6, 32, 4, 4);
    b.rect(16, 14, 16, 10, 3);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    palettes(vdp);
    gs::TileAlloc tiles(vdp);
    fontTiles(tiles, art.font);
    skyline(vdp, tiles);
    for (int i = 0; i < 5; i++) art.kart[i] = gs::uploadMipped(vdp, rearKart(i - 2));
    art.peak = gs::uploadMipped(vdp, pine());
    art.bale = gs::uploadMipped(vdp, hay());
    art.spray = gs::uploadMipped(vdp, flake());
    art.arch = gs::uploadMipped(vdp, archway());

    auto stamp = [&](const char* s, int scale, int color) {
        gs::TextStyle st;
        st.scale = scale;
        st.color = color;
        st.outline = 2;
        return gs::uploadImage(vdp, gs::textBitmap(s, st));
    };
    art.title = stamp("KART PASS", 3, 1);
    art.sub = stamp("BEAT THE STORM", 1, 4);
    art.cleared = stamp("PASS CLEAR", 2, 1);
    art.missed = stamp("MISSED THE END", 2, 3);
    art.late = stamp("STORM TOOK IT", 2, 3);
    art.paused = stamp("PAUSED", 2, 4);
}

}  // namespace kartpass

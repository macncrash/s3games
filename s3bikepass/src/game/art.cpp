#include "game/art.h"

#include <cstring>

namespace bikepass {
namespace {

void pal(gs::VDP& v, int p, int i, int r, int g, int b) { v.setColor(p * 16 + i, gs::rgb4(r, g, b)); }

void palettes(gs::VDP& v) {
    for (int p = 0; p < 16; p++) pal(v, p, 0, 0, 0, 0);

    pal(v, PAL_INK, 1, 14, 15, 15);
    pal(v, PAL_INK, 2, 1, 2, 3);
    pal(v, PAL_HOT, 1, 15, 12, 3);
    pal(v, PAL_HOT, 2, 3, 2, 0);
    pal(v, PAL_BAD, 1, 15, 4, 3);
    pal(v, PAL_BAD, 2, 3, 0, 1);
    pal(v, PAL_GOOD, 1, 7, 15, 10);
    pal(v, PAL_GOOD, 2, 1, 3, 2);

    pal(v, PAL_BIKE, 1, 2, 2, 3);
    pal(v, PAL_BIKE, 2, 12, 2, 2);
    pal(v, PAL_BIKE, 3, 14, 10, 7);
    pal(v, PAL_BIKE, 4, 1, 1, 2);
    pal(v, PAL_BIKE, 5, 15, 13, 2);
    pal(v, PAL_BIKE, 6, 4, 8, 14);
    pal(v, PAL_BIKE, 7, 13, 14, 15);
    pal(v, PAL_BIKE, 8, 3, 4, 8);
    pal(v, PAL_BIKE, 9, 8, 8, 9);

    pal(v, PAL_PEAK, 1, 3, 5, 3);
    pal(v, PAL_PEAK, 2, 5, 7, 4);
    pal(v, PAL_PEAK, 3, 7, 7, 8);
    pal(v, PAL_PEAK, 4, 12, 13, 14);
    pal(v, PAL_PEAK, 5, 2, 2, 3);

    pal(v, PAL_LOG, 1, 6, 4, 2);
    pal(v, PAL_LOG, 2, 9, 6, 3);
    pal(v, PAL_LOG, 3, 3, 5, 2);
    pal(v, PAL_LOG, 4, 12, 10, 6);

    pal(v, PAL_DUST, 1, 10, 9, 7);
    pal(v, PAL_DUST, 2, 13, 12, 9);

    pal(v, PAL_TITLE, 1, 15, 14, 10);
    pal(v, PAL_TITLE, 2, 2, 3, 6);
    pal(v, PAL_TITLE, 3, 15, 6, 2);
    pal(v, PAL_TITLE, 4, 8, 14, 15);

    pal(v, PAL_GATE, 1, 14, 14, 15);
    pal(v, PAL_GATE, 2, 12, 2, 2);
    pal(v, PAL_GATE, 3, 2, 3, 5);
    pal(v, PAL_GATE, 4, 15, 12, 2);

    pal(v, PAL_SKY, 1, 5, 7, 11);
    pal(v, PAL_SKY, 2, 3, 5, 8);
    pal(v, PAL_SKY, 3, 8, 9, 12);
    pal(v, PAL_SKY, 4, 14, 15, 15);
    pal(v, PAL_SKY, 5, 6, 6, 7);
    pal(v, PAL_SKY, 6, 9, 10, 11);
    pal(v, PAL_SKY, 7, 4, 4, 5);
    pal(v, PAL_SKY, 8, 11, 12, 13);

    pal(v, PAL_ROAD, 1, 3, 6, 2);
    pal(v, PAL_ROAD, 2, 2, 4, 2);
    pal(v, PAL_ROAD, 3, 5, 7, 3);
    pal(v, PAL_ROAD, 4, 6, 6, 5);
    pal(v, PAL_ROAD, 5, 4, 4, 4);
    pal(v, PAL_ROAD, 6, 4, 4, 5);
    pal(v, PAL_ROAD, 7, 3, 3, 4);
    pal(v, PAL_ROAD, 8, 7, 7, 6);
    pal(v, PAL_ROAD, 14, 14, 12, 3);
    pal(v, PAL_ROAD, 15, 6, 6, 7);
    v.setFogColor(gs::rgb4(6, 7, 9));
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
    gs::Bitmap b(320, 112);
    for (int y = 0; y < 70; y++) {
        int c = y < 22 ? 2 : y < 48 ? 1 : 3;
        b.rect(0, float(y), 320, 1, c);
    }
    for (int i = 0; i < 8; i++) {
        float x = 20.f + float(i) * 40.f;
        b.ellipse(x, 18.f + float(i % 3) * 6.f, 22.f, 8.f, i % 2 ? 6 : 8);
    }
    struct Peak {
        float x, h, w;
    };
    const Peak peaks[] = {{18, 62, 36}, {64, 84, 48}, {120, 58, 34}, {168, 96, 56},
                           {230, 70, 42}, {286, 88, 50}};
    for (const Peak& p : peaks) {
        b.poly({{p.x - p.w, 112}, {p.x, 112 - p.h}, {p.x + p.w, 112}}, 7);
        b.poly({{p.x - p.w * 0.22f, 112 - p.h * 0.7f},
                {p.x, 112 - p.h},
                {p.x + p.w * 0.22f, 112 - p.h * 0.7f}},
               4);
        b.poly({{p.x - 6, 112 - p.h * 0.35f}, {p.x + 10, 112 - 8}, {p.x + 18, 112 - p.h * 0.2f}}, 5);
    }
    gs::bitmapToPlane(tiles, v.B, 0, 0, b, PAL_SKY);
}

gs::Bitmap rearBike(int lean) {
    gs::Bitmap b(48, 64);
    int s = lean * 4;
    b.ellipse(14, 50, 8, 8, 4);
    b.ellipse(34, 50, 8, 8, 4);
    b.ellipse(14, 50, 3, 3, 7);
    b.ellipse(34, 50, 3, 3, 7);
    b.line(16, 46, 24 + s / 2, 28, 9, 2);
    b.line(32, 46, 24 + s / 2, 28, 9, 2);
    b.line(12, 34, 36, 34, 9, 2);
    b.ellipse(24 + s, 22, 7, 9, 2);
    b.ellipse(24 + s, 12, 6, 6, 6);
    b.rect(20.f + s, 18, 8, 8, 5);
    b.line(10, 32, 18 + s, 24, 1, 2);
    b.line(38, 32, 30 + s, 24, 1, 2);
    b.ellipse(18 + s, 26, 2, 2, 3);
    b.ellipse(30 + s, 26, 2, 2, 3);
    b.rect(22.f + s, 30, 4, 8, 8);
    return b;
}

gs::Bitmap peakSprite() {
    gs::Bitmap b(40, 56);
    b.poly({{2, 56}, {20, 4}, {38, 56}}, 1);
    b.poly({{12, 56}, {20, 14}, {28, 56}}, 2);
    b.poly({{14, 18}, {20, 4}, {26, 18}}, 4);
    b.rect(18, 40, 4, 10, 5);
    return b;
}

gs::Bitmap logSprite() {
    gs::Bitmap b(40, 18);
    b.ellipse(6, 9, 6, 7, 3);
    b.rect(6, 3, 28, 12, 1);
    b.rect(6, 3, 28, 4, 2);
    b.ellipse(34, 9, 6, 7, 4);
    b.ellipse(34, 9, 2, 2, 2);
    return b;
}

gs::Bitmap dustSprite() {
    gs::Bitmap b(12, 8);
    b.ellipse(6, 4, 5, 3, 1);
    b.ellipse(6, 4, 2, 1, 2);
    return b;
}

gs::Bitmap arch(bool shut) {
    gs::Bitmap b(56, 48);
    b.rect(2, 8, 6, 40, 3);
    b.rect(48, 8, 6, 40, 3);
    b.rect(2, 4, 52, 8, shut ? 2 : 4);
    b.rect(8, 16, 40, 6, 1);
    return b;
}

gs::Image label(gs::VDP& v, const char* s, int scale, int color, int outline) {
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
    for (int i = 0; i < 5; i++) art.bike[i] = gs::uploadMipped(vdp, rearBike(i - 2));
    art.peak = gs::uploadMipped(vdp, peakSprite());
    art.log = gs::uploadMipped(vdp, logSprite());
    art.dust = gs::uploadMipped(vdp, dustSprite());
    art.gateOpen = gs::uploadMipped(vdp, arch(false));
    art.gateShut = gs::uploadMipped(vdp, arch(true));
    art.title = label(vdp, "BIKE PASS", 3, 1, 2);
    art.sub = label(vdp, "BEFORE THE STORM", 1, 4, 0);
    art.cleared = label(vdp, "PASS CLEAR", 3, 1, 2);
    art.ditch = label(vdp, "IN THE DITCH", 2, 3, 2);
    art.late = label(vdp, "STORM CLOSED", 2, 3, 2);
    art.paused = label(vdp, "PAUSED", 3, 1, 2);
}

}  // namespace bikepass

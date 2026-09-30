#include "game/art.h"

#include <cstring>

namespace metropass {
namespace {

void pal(gs::VDP& v, int p, int i, int r, int g, int b) { v.setColor(p * 16 + i, gs::rgb4(r, g, b)); }

void palettes(gs::VDP& v) {
    for (int p = 0; p < 16; p++) pal(v, p, 0, 0, 0, 0);

    pal(v, PAL_INK, 1, 14, 15, 15);
    pal(v, PAL_INK, 2, 1, 2, 4);
    pal(v, PAL_HOT, 1, 15, 12, 3);
    pal(v, PAL_HOT, 2, 3, 2, 0);
    pal(v, PAL_BAD, 1, 15, 4, 3);
    pal(v, PAL_BAD, 2, 3, 0, 1);
    pal(v, PAL_GOOD, 1, 6, 15, 12);
    pal(v, PAL_GOOD, 2, 1, 3, 3);

    pal(v, PAL_CAR, 1, 12, 3, 3);
    pal(v, PAL_CAR, 2, 15, 8, 4);
    pal(v, PAL_CAR, 3, 4, 8, 13);
    pal(v, PAL_CAR, 4, 15, 14, 8);
    pal(v, PAL_CAR, 5, 2, 2, 3);
    pal(v, PAL_CAR, 6, 8, 9, 11);
    pal(v, PAL_CAR, 7, 14, 14, 15);
    pal(v, PAL_CAR, 8, 3, 3, 4);
    pal(v, PAL_CAR, 9, 6, 6, 7);

    pal(v, PAL_PEAK, 1, 4, 6, 5);
    pal(v, PAL_PEAK, 2, 7, 8, 7);
    pal(v, PAL_PEAK, 3, 12, 13, 14);
    pal(v, PAL_PEAK, 4, 3, 4, 5);
    pal(v, PAL_PEAK, 5, 9, 10, 11);

    pal(v, PAL_ROCK, 1, 5, 5, 6);
    pal(v, PAL_ROCK, 2, 8, 8, 9);
    pal(v, PAL_ROCK, 3, 13, 14, 15);
    pal(v, PAL_ROCK, 4, 3, 3, 4);

    pal(v, PAL_SNOW, 1, 12, 13, 15);
    pal(v, PAL_SNOW, 2, 15, 15, 15);

    pal(v, PAL_TITLE, 1, 15, 14, 11);
    pal(v, PAL_TITLE, 2, 2, 3, 6);
    pal(v, PAL_TITLE, 3, 15, 5, 3);
    pal(v, PAL_TITLE, 4, 8, 13, 15);

    pal(v, PAL_MOUTH, 1, 6, 6, 7);
    pal(v, PAL_MOUTH, 2, 3, 3, 4);
    pal(v, PAL_MOUTH, 3, 14, 12, 4);
    pal(v, PAL_MOUTH, 4, 10, 4, 3);

    pal(v, PAL_POLE, 1, 4, 5, 6);
    pal(v, PAL_POLE, 2, 10, 11, 12);
    pal(v, PAL_POLE, 3, 14, 10, 3);

    pal(v, PAL_SKY, 1, 3, 5, 9);
    pal(v, PAL_SKY, 2, 2, 3, 6);
    pal(v, PAL_SKY, 3, 6, 8, 12);
    pal(v, PAL_SKY, 4, 14, 15, 15);
    pal(v, PAL_SKY, 5, 5, 6, 8);
    pal(v, PAL_SKY, 6, 8, 9, 11);
    pal(v, PAL_SKY, 7, 3, 4, 5);
    pal(v, PAL_SKY, 8, 11, 12, 14);

    pal(v, PAL_ROAD, 1, 12, 13, 14);
    pal(v, PAL_ROAD, 2, 9, 10, 12);
    pal(v, PAL_ROAD, 3, 14, 15, 15);
    pal(v, PAL_ROAD, 4, 6, 6, 7);
    pal(v, PAL_ROAD, 5, 4, 4, 5);
    pal(v, PAL_ROAD, 6, 7, 7, 8);
    pal(v, PAL_ROAD, 7, 5, 5, 6);
    pal(v, PAL_ROAD, 8, 8, 8, 9);
    pal(v, PAL_ROAD, 14, 14, 11, 3);
    pal(v, PAL_ROAD, 15, 9, 9, 10);
    v.setFogColor(gs::rgb4(7, 8, 11));
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
    for (int y = 0; y < 78; y++) {
        int c = y < 18 ? 2 : y < 46 ? 1 : 3;
        b.rect(0, float(y), 320, 1, c);
    }
    for (int i = 0; i < 7; i++) {
        float x = 16.f + float(i) * 46.f;
        b.ellipse(x, 16.f + float(i % 3) * 5.f, 20.f, 7.f, i % 2 ? 6 : 8);
    }
    struct Peak {
        float x, h, w;
    };
    const Peak peaks[] = {{24, 58, 34}, {78, 78, 46}, {140, 52, 32}, {190, 90, 54}, {250, 64, 38}, {300, 80, 44}};
    for (const Peak& p : peaks) {
        b.poly({{p.x - p.w, 112}, {p.x, 112 - p.h}, {p.x + p.w, 112}}, 7);
        b.poly({{p.x - p.w * 0.2f, 112 - p.h * 0.72f}, {p.x, 112 - p.h}, {p.x + p.w * 0.2f, 112 - p.h * 0.72f}}, 4);
    }
    b.rect(148, 70, 18, 42, 5);
    b.rect(152, 62, 10, 10, 4);
    gs::bitmapToPlane(tiles, v.B, 0, 0, b, PAL_SKY);
}

gs::Bitmap metro(int sway) {
    gs::Bitmap b(64, 48);
    int s = sway * 3;
    b.rect(6.f + s, 16, 52, 26, 1);
    b.rect(8.f + s, 18, 48, 10, 3);
    b.rect(10.f + s, 20, 14, 6, 6);
    b.rect(40.f + s, 20, 14, 6, 6);
    b.rect(28.f + s, 18, 8, 22, 2);
    b.ellipse(16.f + s, 34, 3, 3, 4);
    b.ellipse(48.f + s, 34, 3, 3, 4);
    b.rect(4.f + s, 28, 6, 6, 8);
    b.rect(54.f + s, 28, 6, 6, 8);
    b.rect(30.f + s, 6, 4, 12, 9);
    b.line(18, 8, 46, 8, 5, 2);
    b.rect(20.f + s, 40, 24, 4, 5);
    b.rect(28.f + s, 42, 8, 4, 7);
    return b;
}

gs::Bitmap peakSprite() {
    gs::Bitmap b(36, 52);
    b.poly({{2, 52}, {18, 4}, {34, 52}}, 1);
    b.poly({{12, 52}, {18, 16}, {24, 52}}, 2);
    b.poly({{13, 16}, {18, 4}, {23, 16}}, 3);
    return b;
}

gs::Bitmap rockSprite() {
    gs::Bitmap b(36, 22);
    b.poly({{2, 20}, {8, 6}, {18, 2}, {30, 8}, {34, 20}}, 1);
    b.poly({{10, 18}, {16, 8}, {26, 10}, {28, 18}}, 2);
    b.ellipse(12, 8, 5, 3, 3);
    b.rect(6, 16, 24, 4, 4);
    return b;
}

gs::Bitmap snowSprite() {
    gs::Bitmap b(10, 8);
    b.ellipse(5, 4, 4, 3, 1);
    b.ellipse(5, 4, 2, 1, 2);
    return b;
}

gs::Bitmap mouthSprite() {
    gs::Bitmap b(64, 52);
    b.rect(2, 8, 8, 44, 1);
    b.rect(54, 8, 8, 44, 1);
    b.rect(2, 2, 60, 10, 3);
    b.rect(10, 14, 44, 8, 2);
    b.rect(26, 4, 12, 6, 4);
    return b;
}

gs::Bitmap poleSprite() {
    gs::Bitmap b(28, 40);
    b.rect(12, 6, 3, 34, 1);
    b.rect(4, 6, 20, 3, 2);
    b.ellipse(8, 7, 2, 2, 3);
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
    for (int i = 0; i < 3; i++) art.car[i] = gs::uploadMipped(vdp, metro(i - 1));
    art.peak = gs::uploadMipped(vdp, peakSprite());
    art.rock = gs::uploadMipped(vdp, rockSprite());
    art.snow = gs::uploadMipped(vdp, snowSprite());
    art.mouth = gs::uploadMipped(vdp, mouthSprite());
    art.pole = gs::uploadMipped(vdp, poleSprite());
    art.title = label(vdp, "METRO PASS", 3, 1, 2);
    art.sub = label(vdp, "BEFORE THE STORM", 1, 4, 0);
    art.cleared = label(vdp, "LINE CLEAR", 3, 1, 2);
    art.derail = label(vdp, "OFF THE RAIL", 2, 3, 2);
    art.late = label(vdp, "STORM SHUT", 2, 3, 2);
    art.held = label(vdp, "HELD", 3, 1, 2);
}

}  // namespace metropass

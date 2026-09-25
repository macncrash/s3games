#include "game/art.h"

#include <cmath>
#include <cstring>

namespace bike {
namespace {

void pal(gs::VDP& v, int p, int i, int r, int g, int b) { v.setColor(p * 16 + i, gs::rgb4(r, g, b)); }

void palettes(gs::VDP& v) {
    for (int p = 0; p < 16; p++) pal(v, p, 0, 0, 0, 0);

    pal(v, PAL_INK, 1, 15, 15, 14);
    pal(v, PAL_INK, 2, 2, 2, 4);
    pal(v, PAL_ALERT, 1, 15, 6, 4);
    pal(v, PAL_ALERT, 2, 3, 0, 1);

    pal(v, PAL_BIKE, 1, 1, 1, 2);
    pal(v, PAL_BIKE, 2, 3, 3, 4);
    pal(v, PAL_BIKE, 3, 12, 13, 14);
    pal(v, PAL_BIKE, 4, 8, 8, 9);
    pal(v, PAL_BIKE, 5, 13, 2, 3);
    pal(v, PAL_BIKE, 6, 8, 1, 2);
    pal(v, PAL_BIKE, 7, 15, 12, 3);
    pal(v, PAL_BIKE, 8, 11, 8, 2);
    pal(v, PAL_BIKE, 9, 14, 10, 7);
    pal(v, PAL_BIKE, 10, 15, 15, 14);
    pal(v, PAL_BIKE, 11, 2, 3, 8);
    pal(v, PAL_BIKE, 12, 4, 4, 5);
    pal(v, PAL_BIKE, 13, 15, 14, 12);
    pal(v, PAL_BIKE, 14, 6, 6, 7);
    pal(v, PAL_BIKE, 15, 15, 4, 3);

    pal(v, PAL_LOW, 1, 1, 1, 2);
    pal(v, PAL_LOW, 2, 3, 3, 4);
    pal(v, PAL_LOW, 3, 10, 10, 11);
    pal(v, PAL_LOW, 4, 13, 13, 14);
    pal(v, PAL_LOW, 5, 8, 1, 2);
    pal(v, PAL_LOW, 6, 15, 3, 3);
    pal(v, PAL_LOW, 7, 5, 5, 6);

    pal(v, PAL_HIGH, 1, 1, 1, 2);
    pal(v, PAL_HIGH, 2, 3, 3, 5);
    pal(v, PAL_HIGH, 3, 11, 11, 12);
    pal(v, PAL_HIGH, 4, 14, 14, 12);
    pal(v, PAL_HIGH, 5, 8, 5, 1);
    pal(v, PAL_HIGH, 6, 15, 12, 2);
    pal(v, PAL_HIGH, 7, 6, 6, 8);
    pal(v, PAL_HIGH, 8, 9, 9, 11);

    pal(v, PAL_CITY, 1, 4, 5, 8);
    pal(v, PAL_CITY, 2, 6, 7, 10);
    pal(v, PAL_CITY, 3, 3, 3, 6);
    pal(v, PAL_CITY, 4, 15, 12, 5);
    pal(v, PAL_CITY, 5, 2, 2, 4);
    pal(v, PAL_CITY, 6, 8, 4, 4);
    pal(v, PAL_CITY, 7, 12, 3, 4);
    pal(v, PAL_CITY, 8, 15, 13, 6);
    pal(v, PAL_CITY, 9, 15, 15, 12);
    pal(v, PAL_CITY, 10, 10, 10, 12);
    pal(v, PAL_CITY, 11, 12, 13, 15);
    pal(v, PAL_CITY, 12, 5, 6, 9);

    pal(v, PAL_ROAD, 1, 4, 4, 5);
    pal(v, PAL_ROAD, 2, 6, 6, 7);
    pal(v, PAL_ROAD, 3, 8, 8, 8);
    pal(v, PAL_ROAD, 4, 13, 11, 4);
    pal(v, PAL_ROAD, 5, 7, 7, 8);
    pal(v, PAL_ROAD, 6, 5, 5, 6);
    pal(v, PAL_ROAD, 7, 3, 3, 4);
    pal(v, PAL_ROAD, 8, 9, 9, 7);

    pal(v, PAL_SIGN, 1, 3, 3, 4);
    pal(v, PAL_SIGN, 2, 14, 13, 10);
    pal(v, PAL_SIGN, 3, 2, 2, 3);
    pal(v, PAL_SIGN, 4, 15, 10, 2);
    pal(v, PAL_SIGN, 5, 1, 1, 2);
    pal(v, PAL_SIGN, 6, 15, 15, 14);
    pal(v, PAL_SIGN, 7, 12, 2, 3);

    pal(v, PAL_DUST, 1, 8, 8, 9);
    pal(v, PAL_DUST, 2, 12, 12, 13);
    pal(v, PAL_DUST, 3, 5, 5, 6);

    pal(v, PAL_TITLE, 1, 15, 14, 12);
    pal(v, PAL_TITLE, 2, 3, 1, 4);
    pal(v, PAL_TITLE, 3, 12, 2, 3);
    pal(v, PAL_TITLE, 4, 15, 10, 3);

    v.setFogColor(gs::rgb4(3, 3, 6));
}

void fontTiles(gs::TileAlloc& tiles, int* out) {
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

void ring(gs::Bitmap& b, float cx, float cy, float outer, float inner, int c) {
    b.ellipse(cx, cy, outer, outer, c);
    if (inner > 0.5f) b.ellipse(cx, cy, inner, inner, 0);
}

void spokes(gs::Bitmap& b, float cx, float cy, float r, float ang, int c) {
    for (int i = 0; i < 6; i++) {
        float a = ang + float(i) * 3.1415926f / 3.f;
        b.line(cx, cy, cx + std::cos(a) * r, cy + std::sin(a) * r, c, 1.2f);
    }
}

void looseWheel(gs::Bitmap& b, float ang) {
    const float cx = 20, cy = 20, r = 16;
    ring(b, cx, cy, r, r - 4, 1);
    ring(b, cx, cy, r - 4.5f, r - 6.5f, 3);
    spokes(b, cx, cy, r - 7, ang, 4);
    b.ellipse(cx, cy, 4.2f, 4.2f, 5);
    b.ellipse(cx, cy, 2.1f, 2.1f, 6);
    b.ellipse(cx + 7, cy - 6, 2.2f, 1.4f, 6);
    b.outline(7, false);
}

gs::Bitmap bikePose(int pose) {
    gs::Bitmap b(92, 88);
    const float rx = 26, fx = 66, wy = 70;
    auto wheel = [&](float cx) {
        ring(b, cx, wy, 14, 10, 1);
        ring(b, cx, wy, 10, 8, 3);
        b.ellipse(cx, wy, 3.2f, 3.2f, 4);
    };
    wheel(rx);
    wheel(fx);

    if (pose == 3) {
        b.line(30, 62, 58, 48, 6, 3);
        b.line(58, 48, 66, 58, 5, 3);
        b.line(36, 50, 52, 40, 6, 2.5f);
        b.ellipse(40, 36, 7, 6, 9);
        b.ellipse(40, 33, 6, 5, 10);
        b.rect(34, 30, 10, 3, 15);
        b.ellipse(48, 48, 5, 4, 7);
        b.line(44, 46, 34, 58, 11, 3);
        b.outline(1, false);
        spokes(b, rx, wy, 7, 0.4f, 3);
        spokes(b, fx, wy, 7, 1.1f, 3);
        return b;
    }

    const bool duck = pose == 2;
    const float hy = duck ? 48.f : 28.f;
    const float head = duck ? 40.f : 16.f;
    const float hip = duck ? 56.f : 44.f;

    b.line(rx + 4, wy - 2, 46, hip - 6, 6, 3.2f);
    b.line(46, hip - 8, fx - 6, duck ? 50.f : 40.f, 5, 3.2f);
    b.line(fx - 6, duck ? 50.f : 40.f, fx, wy - 12, 5, 3);
    b.line(46, hip - 8, 46, wy - 4, 6, 2.6f);
    b.line(fx - 8, duck ? 46.f : 36.f, fx + 8, duck ? 42.f : 30.f, 2, 2.4f);
    b.rect(fx + 4, (duck ? 40.f : 28.f), 8, 3, 2);

    b.ellipse(44, hip, 5, 4, 11);
    b.poly({{40, hip - 2}, {48, hip - 4}, {50, hy + 6}, {36, hy + 4}}, 7);
    b.poly({{38, hy + 2}, {46, hy}, {44, hy - 6}, {36, hy - 2}}, 8);
    b.ellipse(40, head, 7.5f, 7.f, 9);
    b.ellipse(40, head - 2, 7.2f, 5.5f, 10);
    b.rect(34, head - 6, 12, 3, 15);
    b.line(44, hy, fx - 2, duck ? 44.f : 34.f, 9, 2.4f);
    b.ellipse(fx - 1, duck ? 44.f : 34.f, 2.2f, 2.f, 9);

    const float crank = pose == 1 ? 1.1f : -0.9f;
    float px = 46 + std::cos(crank) * 8.f;
    float py = wy - 2 + std::sin(crank) * 7.f;
    b.line(44, hip + 2, px, py - 6, 11, 3);
    b.line(px, py - 6, px, py, 12, 2.6f);
    b.rect(px - 3, py - 1, 7, 3, 14);
    float qx = 46 + std::cos(crank + 3.14f) * 8.f;
    float qy = wy - 2 + std::sin(crank + 3.14f) * 7.f;
    b.line(42, hip + 3, qx, qy - 5, 11, 3);
    b.line(qx, qy - 5, qx, qy, 12, 2.4f);
    b.rect(qx - 3, qy - 1, 6, 3, 13);

    b.rect(48, hip - 14, 3, 6, 6);
    b.ellipse(49, hip - 15, 4, 2, 2);
    b.outline(1, false);
    spokes(b, rx, wy, 7, pose * 0.6f, 3);
    spokes(b, fx, wy, 7, pose * 0.6f + 0.4f, 3);
    b.ellipse(rx, wy, 2.4f, 2.4f, 4);
    b.ellipse(fx, wy, 2.4f, 2.4f, 4);
    return b;
}

void skyline(gs::VDP& v, gs::TileAlloc& tiles) {
    gs::Bitmap b(512, 160);
    b.ellipse(430, 36, 16, 16, 8);
    b.ellipse(424, 32, 14, 14, 0);
    b.ellipse(438, 36, 8, 8, 9);
    for (int i = 0; i < 18; i++) {
        int x = (i * 53) % 500;
        int y = 8 + (i * 17) % 28;
        b.ellipse(float(x), float(y), 10 + (i % 4) * 3.f, 3.5f, 11);
    }
    int x = 0;
    int n = 0;
    while (x < 512) {
        int w = 26 + (n * 17 % 34);
        int h = 42 + (n * 29 % 78);
        int c = 1 + (n % 3);
        float top = float(160 - h);
        b.rect(float(x), top, float(w - 2), float(h), c);
        b.rect(float(x), top, float(w - 2), 3, 6);
        if (n % 4 == 0) b.line(float(x + w / 2), top, float(x + w / 2), top - 12, 10, 1.4f);
        if (n % 5 == 2) {
            b.rect(float(x), top + 18, float(w - 2), 6, 7);
            b.rect(float(x + 2), top + 19, float(w - 6), 3, 4);
        }
        for (int wy = int(top) + 8; wy < 152; wy += 9)
            for (int wx = x + 4; wx < x + w - 8; wx += 8)
                b.rect(float(wx), float(wy), 3, 4, ((wx + wy + n) % 5 == 0) ? 4 : 5);
        if (n % 3 == 1) b.rect(float(x + w / 2 - 4), 146, 8, 14, 12);
        x += w;
        n++;
    }
    gs::bitmapToPlane(tiles, v.B, 0, 0, b, PAL_CITY);
}

void street(gs::VDP& v, gs::TileAlloc& tiles) {
    gs::Bitmap b(512, 224);
    b.rect(0, 166, 512, 6, 3);
    b.rect(0, 172, 512, 2, 4);
    b.rect(0, 174, 512, 36, 1);
    b.rect(0, 210, 512, 14, 5);
    for (int x = 0; x < 512; x += 16) {
        b.rect(float(x), 210, 1, 14, 6);
        if ((x / 16) % 4 == 0) b.rect(float(x + 4), 186, 2, 10, 2);
        if ((x / 16) % 6 == 0) b.rect(float(x + 8), 196, 10, 2, 2);
        if ((x / 16) % 9 == 3) {
            b.ellipse(float(x + 8), 192, 6, 3, 7);
            b.ellipse(float(x + 8), 192, 2, 1, 8);
        }
    }
    gs::bitmapToPlane(tiles, v.A, 0, 0, b, PAL_ROAD);
}

gs::Bitmap lampBmp() {
    gs::Bitmap b(18, 70);
    b.rect(7, 16, 4, 48, 1);
    b.rect(3, 62, 12, 4, 1);
    b.rect(2, 12, 14, 5, 2);
    b.ellipse(9, 10, 6, 5, 4);
    b.ellipse(9, 10, 2.5f, 2.5f, 6);
    b.outline(5, false);
    return b;
}

gs::Bitmap shadowBmp() {
    gs::Bitmap b(48, 12);
    b.ellipse(24, 6, 22, 4, 1);
    return b;
}

gs::Bitmap puffBmp() {
    gs::Bitmap b(14, 14);
    b.ellipse(7, 8, 6, 4, 1);
    b.ellipse(6, 6, 3, 2, 2);
    return b;
}

gs::Bitmap wireBmp() {
    gs::Bitmap b(32, 3);
    b.rect(0, 1, 32, 1, 8);
    return b;
}

gs::Bitmap signPost(const char* label, bool finish) {
    gs::Bitmap b(finish ? 46 : 40, 72);
    b.rect(finish ? 20.f : 17.f, 22, 4, 48, 1);
    b.rect(4, 8, float(b.w - 8), 18, 2);
    if (finish) {
        for (int y = 0; y < 4; y++)
            for (int x = 0; x < 8; x++) b.rect(6 + x * 4.f, 10 + y * 3.5f, 4, 3, ((x + y) & 1) ? 5 : 6);
    }
    gs::TextStyle st;
    st.scale = 1;
    st.color = finish ? 6 : 3;
    st.shadow = 0;
    st.outline = 0;
    gs::Bitmap word = gs::textBitmap(label, st);
    int dx = (b.w - word.w) / 2;
    if (!finish) b.blit(word, dx, 12);
    b.outline(5, false);
    return b;
}

gs::Image words(gs::VDP& v, const char* s, int scale, int color, int outline, int shadow) {
    gs::TextStyle st;
    st.scale = scale;
    st.color = color;
    st.outline = outline;
    st.shadow = shadow;
    st.spacing = 1;
    return gs::uploadImage(v, gs::textBitmap(s, st));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    palettes(vdp);
    gs::TileAlloc tiles(vdp, 1);
    fontTiles(tiles, art.font);
    skyline(vdp, tiles);
    street(vdp, tiles);
    art.bike[0] = gs::uploadMipped(vdp, bikePose(0));
    art.bike[1] = gs::uploadMipped(vdp, bikePose(1));
    art.duck = gs::uploadMipped(vdp, bikePose(2));
    art.crash = gs::uploadMipped(vdp, bikePose(3));
    for (int i = 0; i < 4; i++) {
        gs::Bitmap w(40, 40);
        looseWheel(w, float(i) * 0.52f);
        art.wheel[i] = gs::uploadMipped(vdp, w);
    }
    art.lamp = gs::uploadMipped(vdp, lampBmp());
    art.shadow = gs::uploadMipped(vdp, shadowBmp());
    art.puff = gs::uploadMipped(vdp, puffBmp());
    art.post[0] = gs::uploadMipped(vdp, signPost("250", false));
    art.post[1] = gs::uploadMipped(vdp, signPost("500", false));
    art.post[2] = gs::uploadMipped(vdp, signPost("750", false));
    art.gantry = gs::uploadMipped(vdp, signPost("FINISH", true));
    art.wire = gs::uploadImage(vdp, wireBmp());
    art.title = words(vdp, "S3 BIKE", 4, 1, 2, 3);
    art.sub = words(vdp, "ONE KILOMETER", 2, 4, 2, 0);
    art.rule = words(vdp, "DON'T TOUCH WHEELS", 2, 1, 2, 3);
}

}  // namespace bike

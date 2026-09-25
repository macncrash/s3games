#include "game/art.h"

#include <cstring>

namespace plow {
namespace {

void pal(gs::VDP& v, int p, int i, int r, int g, int b) { v.setColor(p * 16 + i, gs::rgb4(r, g, b)); }

void palettes(gs::VDP& v) {
    for (int p = 0; p < 16; p++) pal(v, p, 0, 0, 0, 0);

    pal(v, PAL_INK, 1, 15, 15, 15);
    pal(v, PAL_INK, 2, 1, 2, 4);
    pal(v, PAL_AMBER, 1, 15, 12, 2);
    pal(v, PAL_AMBER, 2, 3, 2, 0);
    pal(v, PAL_ALARM, 1, 15, 4, 3);
    pal(v, PAL_ALARM, 2, 3, 0, 1);

    pal(v, PAL_TRUCK, 1, 14, 7, 1);
    pal(v, PAL_TRUCK, 2, 9, 4, 1);
    pal(v, PAL_TRUCK, 3, 5, 9, 12);
    pal(v, PAL_TRUCK, 4, 11, 12, 13);
    pal(v, PAL_TRUCK, 5, 4, 5, 6);
    pal(v, PAL_TRUCK, 6, 1, 1, 2);
    pal(v, PAL_TRUCK, 7, 15, 12, 2);
    pal(v, PAL_TRUCK, 8, 12, 15, 15);
    pal(v, PAL_TRUCK, 9, 15, 2, 2);
    pal(v, PAL_TRUCK, 10, 1, 1, 1);
    pal(v, PAL_TRUCK, 11, 15, 13, 2);
    pal(v, PAL_TRUCK, 12, 15, 15, 15);
    pal(v, PAL_TRUCK, 13, 13, 14, 15);

    pal(v, PAL_SIGN, 1, 15, 15, 14);
    pal(v, PAL_SIGN, 2, 1, 2, 5);
    pal(v, PAL_SIGN, 3, 12, 2, 2);
    pal(v, PAL_SIGN, 4, 2, 3, 8);
    pal(v, PAL_SIGN, 5, 1, 2, 5);
    pal(v, PAL_SIGN, 6, 15, 11, 2);
    pal(v, PAL_SIGN, 7, 0, 0, 1);
    pal(v, PAL_SIGN, 8, 8, 6, 3);

    pal(v, PAL_TREE, 1, 15, 15, 15);
    pal(v, PAL_TREE, 2, 12, 14, 15);
    pal(v, PAL_TREE, 3, 3, 8, 4);
    pal(v, PAL_TREE, 4, 2, 6, 3);
    pal(v, PAL_TREE, 5, 1, 4, 2);
    pal(v, PAL_TREE, 6, 6, 4, 2);
    pal(v, PAL_TREE, 7, 1, 2, 1);
    pal(v, PAL_TREE, 8, 3, 2, 1);

    pal(v, PAL_DRIFT, 1, 15, 15, 15);
    pal(v, PAL_DRIFT, 2, 13, 15, 15);
    pal(v, PAL_DRIFT, 3, 9, 12, 14);
    pal(v, PAL_DRIFT, 4, 6, 9, 12);
    pal(v, PAL_DRIFT, 5, 3, 5, 8);
    pal(v, PAL_DRIFT, 6, 4, 6, 9);

    pal(v, PAL_ROCK, 1, 8, 8, 9);
    pal(v, PAL_ROCK, 2, 12, 13, 14);
    pal(v, PAL_ROCK, 3, 5, 5, 6);
    pal(v, PAL_ROCK, 4, 15, 15, 15);
    pal(v, PAL_ROCK, 5, 2, 2, 3);

    pal(v, PAL_FX, 1, 15, 15, 15);
    pal(v, PAL_FX, 2, 14, 15, 15);

    pal(v, PAL_TITLE, 1, 15, 15, 14);
    pal(v, PAL_TITLE, 2, 1, 2, 6);
    pal(v, PAL_TITLE, 3, 4, 2, 1);
    pal(v, PAL_TITLE, 4, 12, 13, 14);
    pal(v, PAL_TITLE, 5, 15, 12, 2);

    pal(v, PAL_MOUNT, 1, 7, 9, 12);
    pal(v, PAL_MOUNT, 2, 5, 7, 10);
    pal(v, PAL_MOUNT, 3, 3, 5, 8);
    pal(v, PAL_MOUNT, 4, 15, 15, 15);
    pal(v, PAL_MOUNT, 5, 14, 14, 12);
    pal(v, PAL_MOUNT, 6, 1, 3, 2);
    pal(v, PAL_MOUNT, 7, 2, 4, 3);
    pal(v, PAL_MOUNT, 8, 15, 15, 12);
    pal(v, PAL_MOUNT, 9, 12, 13, 14);

    auto road = [&](int p, bool cut) {
        pal(v, p, 1, cut ? 8 : 13, cut ? 8 : 14, cut ? 7 : 15);
        pal(v, p, 2, cut ? 6 : 10, cut ? 6 : 12, cut ? 5 : 14);
        pal(v, p, 3, cut ? 4 : 8, cut ? 5 : 10, cut ? 4 : 12);
        pal(v, p, 4, cut ? 7 : 12, cut ? 7 : 13, cut ? 6 : 14);
        pal(v, p, 5, cut ? 5 : 9, cut ? 5 : 11, cut ? 4 : 13);
        pal(v, p, 6, cut ? 6 : 11, cut ? 6 : 13, cut ? 6 : 14);
        pal(v, p, 7, cut ? 8 : 13, cut ? 8 : 14, cut ? 7 : 15);
        pal(v, p, 8, cut ? 4 : 7, cut ? 4 : 9, cut ? 4 : 11);
        pal(v, p, 9, cut ? 9 : 12, cut ? 9 : 14, cut ? 8 : 15);
        pal(v, p, 10, cut ? 10 : 14, cut ? 10 : 15, cut ? 9 : 15);
        pal(v, p, 11, 9, 12, 14);
        pal(v, p, 12, 8, 11, 13);
        pal(v, p, 13, 6, 9, 12);
        pal(v, p, 14, cut ? 11 : 15, cut ? 11 : 15, cut ? 10 : 15);
        pal(v, p, 15, cut ? 12 : 14, cut ? 12 : 15, cut ? 11 : 15);
    };
    road(PAL_ROAD, false);
    road(PAL_CUT, true);
    v.setFogColor(gs::rgb4(11, 13, 15));
}

void peak(gs::Bitmap& b, float x, float half, float h, int c, int cap) {
    const float base = float(b.h - 1);
    const float apex = x + half * 0.04f;
    b.poly({{x - half, base}, {x + half * 0.9f, base}, {apex, base - h}}, c);
    float snow = h * 0.2f;
    float t = snow / h;
    b.poly({{apex, base - h}, {apex - half * t * 0.5f, base - h + snow}, {apex + half * t * 0.4f, base - h + snow}}, cap);
}

void mountains(gs::VDP& v, gs::TileAlloc& tiles) {
    gs::Bitmap b(320, 120);
    b.ellipse(248, 22, 16, 16, 8);
    b.ellipse(248, 22, 7, 7, 4);
    b.ellipse(40, 28, 26, 6, 9);
    b.ellipse(78, 26, 14, 5, 9);
    b.ellipse(180, 34, 30, 7, 9);
    struct Pk {
        float x, w, h;
        int c;
    };
    const Pk far[] = {{28, 64, 30, 1}, {100, 80, 26, 1}, {190, 74, 34, 1}, {280, 68, 28, 1}};
    const Pk mid[] = {{18, 46, 46, 2}, {86, 54, 58, 2}, {156, 48, 50, 2}, {230, 52, 66, 2}, {304, 50, 44, 2}};
    const Pk near[] = {{40, 34, 40, 3}, {112, 38, 52, 3}, {176, 32, 36, 3}, {246, 40, 48, 3}, {318, 28, 32, 3}};
    for (auto& p : far) peak(b, p.x, p.w, p.h, p.c, 5);
    for (auto& p : mid) peak(b, p.x, p.w, p.h, p.c, 4);
    for (auto& p : near) peak(b, p.x, p.w, p.h, p.c, 4);
    for (int i = 0; i < 36; i++) {
        float x = float(i * 9 + 1);
        float h = 6.f + float((i * 5) % 7);
        b.poly({{x, 119}, {x + 6, 119}, {x + 3, 119 - h}}, (i & 1) ? 6 : 7);
    }
    gs::bitmapToPlane(tiles, v.B, 0, 0, b, PAL_MOUNT);
}

void font(gs::TileAlloc& tiles, int* out) {
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

gs::Bitmap truckBmp(bool down) {
    gs::Bitmap b(80, 72);
    int by = down ? 2 : 8;
    b.poly({{4, float(by + 16)}, {40, float(by + 2)}, {76, float(by + 16)}, {70, float(by + 22)}, {40, float(by + 10)}, {10, float(by + 22)}}, 4);
    b.poly({{10, float(by + 14)}, {40, float(by + 5)}, {70, float(by + 14)}, {66, float(by + 18)}, {40, float(by + 10)}, {14, float(by + 18)}}, 11);
    b.rect(6, by + 18, 8, 4, 5);
    b.rect(66, by + 18, 8, 4, 5);
    b.rect(18, by + 16, 44, 3, 13);
    b.poly({{16, 26}, {64, 26}, {68, 38}, {12, 38}}, 1);
    b.rect(22, 30, 36, 5, 2);
    b.rect(14, 38, 52, 16, 1);
    b.rect(18, 40, 44, 9, 3);
    b.rect(20, 42, 18, 5, 8);
    b.rect(36, 36, 8, 5, 7);
    b.rect(38, 33, 4, 4, 7);
    b.rect(10, 54, 60, 12, 1);
    b.rect(10, 54, 60, 3, 11);
    b.rect(14, 60, 8, 4, 9);
    b.rect(58, 60, 8, 4, 9);
    b.rect(4, 46, 12, 22, 6);
    b.rect(64, 46, 12, 22, 6);
    b.rect(6, 50, 8, 10, 10);
    b.rect(66, 50, 8, 10, 10);
    b.rect(34, 66, 12, 4, 5);
    b.rect(37, 62, 6, 4, 12);
    b.outline(10, false);
    return b;
}

gs::Bitmap driftBmp(int var) {
    gs::Bitmap b(72, 36);
    b.ellipse(36, 24, 32, 12, 3);
    b.ellipse(24, 22, 16, 10, 2);
    b.ellipse(48, 23, 14, 8, 1);
    b.ellipse(34, 16, 10, 5, 1);
    if (var) b.ellipse(16, 20, 8, 4, 2);
    else b.ellipse(54, 18, 7, 3, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap spruceBmp(int var) {
    gs::Bitmap b(36, 72);
    int lean = (var - 1) * 2;
    b.rect(16 + lean / 2, 54, 5, 16, 6);
    b.rect(17 + lean / 2, 56, 2, 12, 8);
    b.poly({{18.f + lean, 2.f}, {6.f, 22.f}, {30.f, 22.f}}, 4);
    b.poly({{18.f + lean * 0.4f, 16.f}, {3.f, 38.f}, {33.f, 38.f}}, var == 2 ? 5 : 3);
    b.poly({{18.f, 30.f}, {1.f, 54.f}, {35.f, 54.f}}, 4);
    b.poly({{18.f + lean, 2.f}, {11.f, 12.f}, {25.f, 12.f}}, 1);
    b.poly({{18.f, 18.f}, {9.f, 26.f}, {27.f, 26.f}}, 2);
    b.outline(7, false);
    return b;
}

gs::Bitmap rockBmp(int var) {
    gs::Bitmap b(var ? 36 : 28, 20);
    float cx = b.w * 0.5f;
    b.ellipse(cx, 12, cx - 2, 7, 1);
    b.ellipse(cx - 3, 10, cx * 0.4f, 4, 2);
    b.ellipse(cx + 4, 13, 4, 2.4f, 3);
    b.ellipse(cx - 2, 8, 5, 2, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap postBmp() {
    gs::Bitmap b(16, 42);
    b.rect(7, 10, 3, 30, 8);
    b.rect(2, 4, 12, 10, 6);
    b.rect(3, 6, 10, 6, 1);
    b.outline(2, false);
    return b;
}

gs::Bitmap gateBmp(const char* word) {
    gs::Bitmap b(128, 40);
    for (int i = 0; i < 8; i++) b.rect(i * 16, 0, 8, 6, (i & 1) ? 6 : 1);
    b.rect(0, 8, 128, 22, 4);
    b.rect(0, 8, 128, 3, 3);
    b.rect(0, 27, 128, 3, 5);
    gs::TextStyle st;
    st.scale = 2;
    st.color = 1;
    st.outline = 2;
    st.spacing = 1;
    gs::Bitmap t = gs::textBitmap(word, st);
    b.blit(t, (128 - t.w) / 2, 12);
    b.outline(7, false);
    return b;
}

gs::Bitmap flakeBmp() {
    gs::Bitmap b(7, 7);
    b.line(3, 0, 3, 6, 1, 1);
    b.line(0, 3, 6, 3, 1, 1);
    b.line(1, 1, 5, 5, 1, 1);
    b.line(5, 1, 1, 5, 1, 1);
    return b;
}

gs::Bitmap shadowBmp() {
    gs::Bitmap b(40, 12);
    b.ellipse(20, 6, 18, 5, 1);
    return b;
}

gs::Bitmap titleCard() {
    gs::TextStyle st;
    st.scale = 4;
    st.color = 1;
    st.outline = 2;
    st.shadow = 3;
    st.spacing = 1;
    gs::Bitmap word = gs::textBitmap("S3 PLOW", st);
    gs::Bitmap b(word.w + 8, word.h + 22);
    b.blit(word, 4, 0);
    float y = float(word.h + 4);
    float mid = b.w * 0.5f;
    b.poly({{8, y + 12}, {mid, y}, {float(b.w - 8), y + 12}, {float(b.w - 16), y + 16}, {mid, y + 6}, {16, y + 16}}, 4);
    b.poly({{16, y + 10}, {mid, y + 3}, {float(b.w - 16), y + 10}, {float(b.w - 22), y + 13}, {mid, y + 6}, {22, y + 13}}, 5);
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
    mountains(vdp, tiles);
    font(tiles, art.font);
    art.truckDown = gs::uploadMipped(vdp, truckBmp(true));
    art.truckUp = gs::uploadMipped(vdp, truckBmp(false));
    art.drift[0] = gs::uploadMipped(vdp, driftBmp(0));
    art.drift[1] = gs::uploadMipped(vdp, driftBmp(1));
    for (int i = 0; i < 3; i++) art.spruce[i] = gs::uploadMipped(vdp, spruceBmp(i));
    art.rock[0] = gs::uploadMipped(vdp, rockBmp(0));
    art.rock[1] = gs::uploadMipped(vdp, rockBmp(1));
    art.post = gs::uploadMipped(vdp, postBmp());
    art.gateRoll = gs::uploadMipped(vdp, gateBmp("ROLL"));
    art.gateSummit = gs::uploadMipped(vdp, gateBmp("SUMMIT"));
    art.flake = gs::uploadMipped(vdp, flakeBmp());
    art.shadow = gs::uploadMipped(vdp, shadowBmp());
    art.titleCard = gs::uploadImage(vdp, titleCard());
    art.titleSub = words(vdp, "CLEAR THE PASS", 2);
    art.count[0] = words(vdp, "3", 5);
    art.count[1] = words(vdp, "2", 5);
    art.count[2] = words(vdp, "1", 5);
    art.count[3] = words(vdp, "GO", 4);
    art.plowed = words(vdp, "PLOWED", 2);
    art.winText = words(vdp, "PASS CLEAR", 3);
    art.loseStorm = words(vdp, "STORM CLOSED", 2);
    art.loseSnow = words(vdp, "SNOW STILL BLOCKS", 2);
    art.loseBuried = words(vdp, "BURIED", 3);
}

}  // namespace plow

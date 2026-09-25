#include "game/art.h"

#include <cmath>
#include <cstdint>
#include <cstring>

namespace sled {
namespace {

void pal(gs::VDP& v, int p, int i, int r, int g, int b) { v.setColor(p * 16 + i, gs::rgb4(r, g, b)); }

void palettes(gs::VDP& v) {
    for (int p = 0; p < 16; p++) pal(v, p, 0, 0, 0, 0);
    pal(v, PAL_WHITE, 1, 15, 15, 15);
    pal(v, PAL_WHITE, 2, 1, 2, 5);
    pal(v, PAL_GOLD, 1, 15, 12, 3);
    pal(v, PAL_GOLD, 2, 3, 2, 1);
    pal(v, PAL_ALERT, 1, 15, 4, 3);
    pal(v, PAL_ALERT, 2, 3, 0, 1);

    pal(v, PAL_PLAYER, 1, 13, 2, 2);
    pal(v, PAL_PLAYER, 2, 8, 1, 1);
    pal(v, PAL_PLAYER, 3, 2, 3, 8);
    pal(v, PAL_PLAYER, 4, 14, 9, 6);
    pal(v, PAL_PLAYER, 5, 10, 6, 3);
    pal(v, PAL_PLAYER, 6, 6, 3, 2);
    pal(v, PAL_PLAYER, 7, 13, 14, 15);
    pal(v, PAL_PLAYER, 8, 15, 15, 15);
    pal(v, PAL_PLAYER, 9, 15, 12, 2);
    pal(v, PAL_PLAYER, 10, 2, 1, 2);
    pal(v, PAL_PLAYER, 11, 3, 12, 14);

    pal(v, PAL_RIVAL, 1, 2, 2, 3);
    pal(v, PAL_RIVAL, 2, 5, 5, 7);
    pal(v, PAL_RIVAL, 3, 14, 11, 3);
    pal(v, PAL_RIVAL, 4, 15, 14, 8);
    pal(v, PAL_RIVAL, 5, 14, 13, 11);
    pal(v, PAL_RIVAL, 6, 1, 1, 2);
    pal(v, PAL_RIVAL, 7, 12, 13, 14);
    pal(v, PAL_RIVAL, 8, 15, 15, 15);
    pal(v, PAL_RIVAL, 9, 8, 5, 3);
    pal(v, PAL_RIVAL, 10, 1, 1, 2);

    pal(v, PAL_TREE, 1, 15, 15, 15);
    pal(v, PAL_TREE, 2, 12, 13, 14);
    pal(v, PAL_TREE, 3, 1, 4, 2);
    pal(v, PAL_TREE, 4, 2, 7, 3);
    pal(v, PAL_TREE, 5, 4, 9, 4);
    pal(v, PAL_TREE, 6, 7, 4, 2);
    pal(v, PAL_TREE, 7, 4, 2, 1);
    pal(v, PAL_TREE, 8, 1, 2, 1);

    pal(v, PAL_ROCK, 1, 8, 8, 9);
    pal(v, PAL_ROCK, 2, 12, 12, 13);
    pal(v, PAL_ROCK, 3, 4, 4, 5);
    pal(v, PAL_ROCK, 4, 14, 15, 15);
    pal(v, PAL_ROCK, 5, 2, 2, 3);

    pal(v, PAL_BANNER, 1, 15, 14, 12);
    pal(v, PAL_BANNER, 2, 2, 2, 6);
    pal(v, PAL_BANNER, 3, 12, 2, 2);
    pal(v, PAL_BANNER, 4, 15, 15, 15);
    pal(v, PAL_BANNER, 5, 6, 1, 1);
    pal(v, PAL_BANNER, 6, 8, 5, 3);
    pal(v, PAL_BANNER, 7, 1, 1, 2);

    pal(v, PAL_FX, 1, 15, 15, 15);
    pal(v, PAL_FX, 2, 14, 15, 15);

    pal(v, PAL_TITLE, 1, 15, 14, 12);
    pal(v, PAL_TITLE, 2, 2, 3, 8);
    pal(v, PAL_TITLE, 3, 5, 3, 2);

    pal(v, PAL_MOUNT, 1, 8, 10, 13);
    pal(v, PAL_MOUNT, 2, 6, 8, 11);
    pal(v, PAL_MOUNT, 3, 4, 6, 8);
    pal(v, PAL_MOUNT, 4, 15, 15, 15);
    pal(v, PAL_MOUNT, 5, 3, 4, 7);
    pal(v, PAL_MOUNT, 6, 2, 5, 3);
    pal(v, PAL_MOUNT, 8, 15, 13, 6);
    pal(v, PAL_MOUNT, 9, 15, 15, 12);

    auto snow = [&](int p, bool icy) {
        pal(v, p, 1, icy ? 11 : 14, icy ? 14 : 15, 15);
        pal(v, p, 2, icy ? 8 : 11, icy ? 12 : 13, icy ? 14 : 15);
        pal(v, p, 3, icy ? 6 : 8, icy ? 9 : 11, icy ? 12 : 13);
        pal(v, p, 4, 13, 14, 15);
        pal(v, p, 5, 9, 12, 14);
        pal(v, p, 6, icy ? 12 : 15, icy ? 15 : 15, 15);
        pal(v, p, 7, icy ? 9 : 13, icy ? 13 : 14, 15);
        pal(v, p, 8, icy ? 6 : 8, icy ? 10 : 10, icy ? 13 : 12);
        pal(v, p, 9, icy ? 7 : 12, icy ? 11 : 14, 15);
        pal(v, p, 10, 14, 15, 15);
        pal(v, p, 11, 8, 12, 15);
        pal(v, p, 12, 6, 11, 15);
        pal(v, p, 13, 5, 9, 14);
        pal(v, p, 14, 15, 15, icy ? 15 : 12);
        pal(v, p, 15, icy ? 13 : 15, 15, 15);
    };
    snow(PAL_SNOW, false);
    snow(PAL_ICE, true);
    v.setFogColor(gs::rgb4(12, 14, 15));
}

void peak(gs::Bitmap& b, float x, float half, float h, int c) {
    const float base = float(b.h - 1);
    const float apex = x + half * 0.06f;
    b.poly({{x - half, base}, {x + half * 0.86f, base}, {apex, base - h}}, c);
    float cap = h * 0.22f;
    float t = cap / h;
    b.poly({{apex, base - h}, {apex - half * t * 0.55f, base - h + cap}, {apex + half * t * 0.42f, base - h + cap}}, 4);
}

void mountains(gs::VDP& v, gs::TileAlloc& tiles) {
    gs::Bitmap b(320, 112);
    b.ellipse(262, 26, 20, 18, 9);
    b.ellipse(262, 26, 11, 10, 8);
    b.ellipse(48, 22, 28, 8, 4);
    b.ellipse(70, 20, 16, 6, 4);
    b.ellipse(168, 30, 22, 6, 4);
    struct Pk {
        float x, w, h;
        int c;
    };
    const Pk far[] = {{30, 70, 34, 1}, {110, 90, 28, 1}, {200, 80, 36, 1}, {290, 70, 30, 1}};
    const Pk mid[] = {{16, 48, 48, 2}, {78, 56, 62, 2}, {150, 50, 54, 2}, {214, 46, 70, 2}, {300, 58, 50, 2}};
    const Pk near[] = {{46, 36, 44, 3}, {124, 40, 58, 3}, {186, 34, 40, 3}, {248, 42, 52, 3}, {330, 30, 36, 3}};
    for (auto& p : far) peak(b, p.x, p.w, p.h, p.c);
    for (auto& p : mid) peak(b, p.x, p.w, p.h, p.c);
    for (auto& p : near) peak(b, p.x, p.w, p.h, p.c);
    for (int i = 0; i < 40; i++) {
        float x = float(i * 8 + 2);
        float h = 5.f + float((i * 3) % 5);
        b.poly({{x, 111}, {x + 5, 111}, {x + 2.5f, 111 - h}}, 6);
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

gs::Bitmap treeBmp(int var) {
    gs::Bitmap b(40, 64);
    int lean = (var - 1) * 3;
    b.rect(17 + lean / 2, 46, 6, 16, 6);
    b.rect(18 + lean / 2, 48, 2, 14, 7);
    b.poly({{20.f + lean, 2.f}, {6.f, 26.f}, {34.f + lean * 0.3f, 26.f}}, 4);
    b.poly({{20.f + lean * 0.6f, 16.f}, {3.f, 40.f}, {37.f, 40.f}}, var == 2 ? 5 : 3);
    b.poly({{20.f, 30.f}, {1.f, 52.f}, {39.f, 52.f}}, 4);
    b.poly({{20.f + lean, 2.f}, {12.f, 14.f}, {28.f + lean * 0.2f, 14.f}}, 1);
    b.poly({{20.f + lean * 0.5f, 16.f}, {10.f, 26.f}, {30.f, 26.f}}, 2);
    b.rect(14, 50, 8, 2, 1);
    b.outline(8, false);
    return b;
}

gs::Bitmap rockBmp(int var) {
    gs::Bitmap b(var ? 34 : 26, 18);
    float cx = b.w * 0.5f;
    b.ellipse(cx, 11, cx - 2, 7, 1);
    b.ellipse(cx - 2, 9, cx * 0.45f, 4, 2);
    b.ellipse(cx + 3, 12, 4, 2.5f, 3);
    b.ellipse(cx - 4, 8, 3, 1.5f, 4);
    b.outline(5, false);
    return b;
}

void sledDeck(gs::Bitmap& b, int y) {
    b.poly({{10, float(y + 8)}, {42, float(y + 8)}, {46, float(y + 14)}, {6, float(y + 14)}}, 5);
    b.rect(8, y + 13, 36, 3, 6);
    b.line(12, y + 18, 16, y + 6, 7, 2);
    b.line(40, y + 18, 36, y + 6, 7, 2);
    b.rect(10, y + 17, 32, 2, 7);
}

gs::Bitmap playerBmp(int pose) {
    // pose 0 sit, 1 tuck, 2 lean left
    gs::Bitmap b(52, 60);
    int ox = pose == 2 ? -6 : 0;
    int body = pose == 1 ? 6 : 0;
    sledDeck(b, 36);
    b.line(14 + ox, 34 + body, 8, 42, 1, 3);
    b.line(38 + ox, 34 + body, 44, 42, 1, 3);
    b.rect(16 + ox, 26 + body, 20, 14 - body / 2, 1);
    b.rect(18 + ox, 30 + body, 16, 6, 2);
    b.ellipse(26 + ox, 20 + body, 8, 8, 1);
    b.ellipse(26 + ox, 21 + body, 5, 4, 4);
    b.rect(20 + ox, 19 + body, 12, 3, 11);
    b.ellipse(26 + ox, 12 + body, 3, 3, 8);
    b.poly({{18.f + ox, 30.f + body}, {4.f, 34.f}, {8.f, 40.f}, {20.f + ox, 34.f}}, 9);
    b.rect(18 + ox, 40, 16, 6, 3);
    b.outline(10, false);
    return b;
}

void clockHands(gs::Bitmap& b, int frame, float cx, float cy) {
    float a = float(frame) * 1.5707963f - 1.5707963f;
    b.line(cx, cy, cx + std::cos(a) * 6.f, cy + std::sin(a) * 6.f, 6, 1);
    float h = a * 0.22f + 0.8f;
    b.line(cx, cy, cx + std::cos(h) * 3.5f, cy + std::sin(h) * 3.5f, 6, 1);
    b.ellipse(cx, cy, 1.4f, 1.4f, 3);
}

gs::Bitmap rivalBmp(int frame) {
    gs::Bitmap b(52, 60);
    sledDeck(b, 36);
    b.line(16, 34, 10, 42, 1, 3);
    b.line(36, 34, 42, 42, 1, 3);
    b.rect(16, 24, 20, 14, 1);
    b.rect(18, 28, 16, 6, 2);
    b.ellipse(26, 16, 7, 7, 1);
    b.ellipse(26, 10, 3, 3, 3);
    b.ellipse(26, 30, 9, 9, 3);
    b.ellipse(26, 30, 7, 7, 5);
    b.set(26, 24, 6);
    b.set(26, 36, 6);
    b.set(20, 30, 6);
    b.set(32, 30, 6);
    clockHands(b, frame, 26, 30);
    b.rect(18, 40, 16, 5, 2);
    b.poly({{34, 26}, {46, 22}, {44, 28}, {34, 30}}, 3);
    b.outline(10, false);
    return b;
}

gs::Bitmap flagBmp() {
    gs::Bitmap b(18, 36);
    b.rect(2, 2, 2, 32, 6);
    b.poly({{4, 3}, {16, 8}, {4, 14}}, 3);
    b.poly({{4, 6}, {12, 9}, {4, 12}}, 4);
    b.outline(7, false);
    return b;
}

gs::Bitmap bannerBmp(const char* word) {
    gs::Bitmap b(112, 40);
    for (int x = 0; x < 112; x += 8) {
        b.rect(x, 2, 4, 8, 4);
        b.rect(x + 4, 2, 4, 8, 3);
        b.rect(x, 10, 4, 8, 3);
        b.rect(x + 4, 10, 4, 8, 4);
    }
    b.rect(0, 20, 112, 16, 5);
    gs::TextStyle st;
    st.scale = 2;
    st.color = 1;
    st.outline = 2;
    st.spacing = 1;
    gs::Bitmap t = gs::textBitmap(word, st);
    b.blit(t, (112 - t.w) / 2, 22);
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
    gs::Bitmap b(32, 12);
    b.ellipse(16, 6, 14, 5, 1);
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
    for (int i = 0; i < 3; i++) art.tree[i] = gs::uploadMipped(vdp, treeBmp(i));
    art.rock[0] = gs::uploadMipped(vdp, rockBmp(0));
    art.rock[1] = gs::uploadMipped(vdp, rockBmp(1));
    art.playerSit = gs::uploadMipped(vdp, playerBmp(0));
    art.playerTuck = gs::uploadMipped(vdp, playerBmp(1));
    art.playerLean = gs::uploadMipped(vdp, playerBmp(2));
    for (int i = 0; i < 4; i++) art.rival[i] = gs::uploadMipped(vdp, rivalBmp(i));
    art.flag = gs::uploadMipped(vdp, flagBmp());
    art.banner = gs::uploadMipped(vdp, bannerBmp("FINISH"));
    art.drop = gs::uploadMipped(vdp, bannerBmp("DROP"));
    art.flake = gs::uploadMipped(vdp, flakeBmp());
    art.shadow = gs::uploadMipped(vdp, shadowBmp());
    art.titleMain = words(vdp, "S3 SLED", 4);
    art.titleA = words(vdp, "ONE SNOW RUN", 2);
    art.titleB = words(vdp, "THE CLOCK IS THE OTHER SLED", 1);
    art.count[0] = words(vdp, "3", 5);
    art.count[1] = words(vdp, "2", 5);
    art.count[2] = words(vdp, "1", 5);
    art.count[3] = words(vdp, "GO", 4);
    art.passed = words(vdp, "PASSED THE CLOCK", 2);
    art.lostLead = words(vdp, "THE CLOCK IS AHEAD", 2);
    art.winText = words(vdp, "BEAT THE CLOCK", 2);
    art.loseText = words(vdp, "THE CLOCK WINS", 2);
}

}  // namespace sled

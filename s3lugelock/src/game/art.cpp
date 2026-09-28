#include "game/art.h"

#include <algorithm>
#include <cstring>

namespace lugelock {
namespace {

void pal(gs::VDP& v, int p, int i, int r, int g, int b) { v.setColor(p * 16 + i, gs::rgb4(r, g, b)); }

void ramp(gs::VDP& v, int p) {
    pal(v, p, 0, 0, 0, 0);
    pal(v, p, 1, 15, 15, 15);
    pal(v, p, 2, 2, 3, 5);
}

gs::Bitmap rider(int pose) {
    gs::Bitmap b(64, 56);
    const int lean = pose == 2 ? -7 : 0;
    const int tuck = pose == 1 ? 6 : 0;
    b.line(14, 52, 22, 30 + tuck, 6, 2.4f);
    b.line(50, 52, 42, 30 + tuck, 6, 2.4f);
    b.line(16, 50, 24, 32 + tuck, 7, 1.1f);
    b.line(48, 50, 40, 32 + tuck, 7, 1.1f);
    b.poly({{18, 50}, {46, 50}, {40, float(32 + tuck)}, {24, float(32 + tuck)}}, 5);
    b.poly({{22, 46}, {42, 46}, {38, float(36 + tuck)}, {26, float(36 + tuck)}}, 8);
    b.ellipse(32 + lean, 26 + tuck, 14, 9, 1);
    b.ellipse(32 + lean, 25 + tuck, 9, 6, 2);
    b.line(20 + lean, 28 + tuck, 12, 38, 1, 2.8f);
    b.line(44 + lean, 28 + tuck, 52, 38, 1, 2.8f);
    b.ellipse(11, 39, 2.8f, 2.1f, 9);
    b.ellipse(53, 39, 2.8f, 2.1f, 9);
    b.ellipse(32 + lean, 14 + tuck, 8, 7, 3);
    b.ellipse(32 + lean, 15 + tuck, 5, 4, 10);
    b.rect(26 + lean, 13 + tuck, 12, 3, 4);
    b.outline(11, false);
    b.rect(28 + lean, 14 + tuck, 8, 2, 4);
    return b;
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

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    ramp(vdp, PAL_HUD);
    pal(vdp, PAL_HUD, 1, 14, 15, 15);
    pal(vdp, PAL_HUD, 2, 2, 4, 6);

    pal(vdp, PAL_RIDER, 0, 0, 0, 0);
    pal(vdp, PAL_RIDER, 1, 12, 13, 14);
    pal(vdp, PAL_RIDER, 2, 7, 8, 10);
    pal(vdp, PAL_RIDER, 3, 15, 6, 3);
    pal(vdp, PAL_RIDER, 4, 4, 5, 7);
    pal(vdp, PAL_RIDER, 5, 9, 4, 3);
    pal(vdp, PAL_RIDER, 6, 3, 3, 4);
    pal(vdp, PAL_RIDER, 7, 13, 13, 14);
    pal(vdp, PAL_RIDER, 8, 11, 8, 5);
    pal(vdp, PAL_RIDER, 9, 14, 12, 6);
    pal(vdp, PAL_RIDER, 10, 15, 10, 8);
    pal(vdp, PAL_RIDER, 11, 1, 1, 2);

    pal(vdp, PAL_STEEL, 0, 0, 0, 0);
    pal(vdp, PAL_STEEL, 1, 8, 10, 11);
    pal(vdp, PAL_STEEL, 2, 13, 14, 14);
    pal(vdp, PAL_STEEL, 3, 4, 6, 7);
    pal(vdp, PAL_STEEL, 4, 15, 12, 4);
    pal(vdp, PAL_STEEL, 5, 2, 3, 4);

    pal(vdp, PAL_POST, 0, 0, 0, 0);
    pal(vdp, PAL_POST, 1, 6, 7, 8);
    pal(vdp, PAL_POST, 2, 12, 13, 13);
    pal(vdp, PAL_POST, 3, 3, 4, 5);
    pal(vdp, PAL_POST, 4, 14, 10, 3);

    pal(vdp, PAL_FX, 0, 0, 0, 0);
    pal(vdp, PAL_FX, 1, 14, 15, 15);
    pal(vdp, PAL_FX, 2, 8, 12, 14);

    ramp(vdp, PAL_TITLE);
    pal(vdp, PAL_TITLE, 1, 15, 15, 15);
    pal(vdp, PAL_TITLE, 2, 2, 5, 8);

    ramp(vdp, PAL_GOLD);
    pal(vdp, PAL_GOLD, 1, 15, 13, 5);
    pal(vdp, PAL_GOLD, 2, 5, 3, 1);

    ramp(vdp, PAL_ALERT);
    pal(vdp, PAL_ALERT, 1, 15, 6, 4);
    pal(vdp, PAL_ALERT, 2, 4, 1, 1);

    pal(vdp, PAL_SIGN, 0, 0, 0, 0);
    pal(vdp, PAL_SIGN, 1, 15, 14, 8);
    pal(vdp, PAL_SIGN, 2, 3, 5, 8);
    pal(vdp, PAL_SIGN, 3, 12, 8, 3);

    for (int i = 0; i < 16; i++) {
        int c = 8 + i / 3;
        pal(vdp, PAL_ICE, i, c - 2, c, std::min(15, c + 3));
    }
    pal(vdp, PAL_ICE, 1, 14, 15, 15);
    pal(vdp, PAL_ICE, 4, 6, 9, 12);

    gs::TileAlloc tiles(vdp, 1);
    font(tiles, art.font);

    art.flat = gs::uploadMipped(vdp, rider(0));
    art.tuck = gs::uploadMipped(vdp, rider(1));
    art.lean = gs::uploadMipped(vdp, rider(2));

    gs::Bitmap post(16, 48);
    post.rect(4, 0, 8, 48, 1);
    post.rect(6, 0, 3, 48, 2);
    post.rect(3, 40, 10, 6, 3);
    post.rect(5, 6, 6, 3, 4);
    art.post = gs::uploadMipped(vdp, post);

    gs::Bitmap leaf(12, 40);
    leaf.rect(0, 0, 12, 40, 1);
    leaf.rect(1, 0, 3, 40, 2);
    for (int y = 4; y < 40; y += 8) leaf.rect(0, y, 12, 1, 3);
    leaf.rect(8, 0, 2, 40, 5);
    leaf.rect(2, 16, 8, 3, 4);
    art.leaf = gs::uploadMipped(vdp, leaf);

    gs::Bitmap beam(32, 8);
    beam.rect(0, 2, 32, 4, 1);
    beam.rect(0, 2, 32, 1, 2);
    art.beam = gs::uploadMipped(vdp, beam);

    gs::TextStyle st;
    st.scale = 1;
    st.color = 1;
    st.outline = 2;
    st.spacing = 1;
    art.sign = gs::uploadMipped(vdp, gs::textBitmap("LOCK", st));

    gs::Bitmap spark(8, 8);
    spark.ellipse(4, 4, 3, 3, 1);
    spark.ellipse(4, 4, 1.4f, 1.4f, 2);
    art.spark = gs::uploadMipped(vdp, spark);

    gs::Bitmap shadow(32, 10);
    shadow.ellipse(16, 5, 14, 3.2f, 1);
    art.shadow = gs::uploadMipped(vdp, shadow);

    st.scale = 3;
    st.spacing = 2;
    art.title = gs::uploadImage(vdp, gs::textBitmap("LUGE LOCK", st));
    st.scale = 1;
    st.spacing = 1;
    art.sub = gs::uploadImage(vdp, gs::textBitmap("PASS CLEAN", st));
}

}  // namespace lugelock

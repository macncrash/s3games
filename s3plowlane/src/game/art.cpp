#include "game/art.h"

#include <cstring>

namespace plow {
namespace {

void pal(gs::VDP& v, int p, int i, int r, int g, int b) { v.setColor(p * 16 + i, gs::rgb4(r, g, b)); }

void palettes(gs::VDP& v) {
    for (int p = 0; p < 16; p++) pal(v, p, 0, 0, 0, 0);

    pal(v, PAL_HUD, 1, 15, 14, 10);
    pal(v, PAL_HUD, 2, 4, 3, 1);
    pal(v, PAL_WARN, 1, 15, 4, 2);
    pal(v, PAL_WARN, 2, 4, 1, 0);
    pal(v, PAL_GOOD, 1, 8, 15, 6);
    pal(v, PAL_GOOD, 2, 1, 3, 1);

    pal(v, PAL_PLOW, 1, 2, 1, 1);
    pal(v, PAL_PLOW, 2, 12, 2, 1);
    pal(v, PAL_PLOW, 3, 15, 4, 2);
    pal(v, PAL_PLOW, 4, 6, 6, 7);
    pal(v, PAL_PLOW, 5, 10, 10, 11);
    pal(v, PAL_PLOW, 6, 3, 3, 3);
    pal(v, PAL_PLOW, 7, 14, 12, 4);
    pal(v, PAL_PLOW, 8, 8, 5, 2);
    pal(v, PAL_PLOW, 9, 1, 1, 1);
    pal(v, PAL_PLOW, 10, 15, 15, 13);

    pal(v, PAL_POST, 1, 5, 4, 2);
    pal(v, PAL_POST, 2, 9, 7, 3);
    pal(v, PAL_POST, 3, 3, 2, 1);
    pal(v, PAL_POST, 4, 12, 10, 4);

    pal(v, PAL_GATE, 1, 14, 14, 12);
    pal(v, PAL_GATE, 2, 12, 2, 1);
    pal(v, PAL_GATE, 3, 3, 2, 1);
    pal(v, PAL_GATE, 4, 8, 12, 3);

    pal(v, PAL_FIELD, 1, 6, 5, 2);
    pal(v, PAL_FIELD, 2, 9, 7, 3);
    pal(v, PAL_FIELD, 3, 4, 6, 2);
    pal(v, PAL_FIELD, 4, 11, 9, 3);

    pal(v, PAL_TITLE, 1, 15, 12, 3);
    pal(v, PAL_TITLE, 2, 3, 2, 1);
    pal(v, PAL_TITLE, 3, 15, 15, 12);
    pal(v, PAL_TITLE, 4, 12, 3, 1);
    pal(v, PAL_TITLE, 5, 6, 10, 3);

    auto road = [&](int p) {
        pal(v, p, 1, 5, 3, 1);
        pal(v, p, 2, 7, 5, 2);
        pal(v, p, 3, 9, 6, 2);
        pal(v, p, 4, 4, 3, 1);
        pal(v, p, 5, 3, 2, 1);
        pal(v, p, 6, 6, 4, 2);
        pal(v, p, 7, 8, 5, 2);
        pal(v, p, 8, 10, 7, 3);
        pal(v, p, 9, 4, 5, 2);
        pal(v, p, 10, 3, 4, 1);
        pal(v, p, 14, 12, 10, 4);
        pal(v, p, 15, 6, 5, 2);
    };
    road(PAL_ROAD);
    v.setFogColor(gs::rgb4(10, 7, 3));
}

void fontTiles(gs::TileAlloc& tiles, int* out) {
    uint8_t px[64];
    for (int ch = 32; ch < 127; ch++) {
        const uint8_t* g = gs::glyph(char(ch));
        std::memset(px, 0, sizeof px);
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) px[(y + 1) * 8 + (x + 1)] = 1;
        out[ch] = tiles.shared(px);
    }
}

gs::Bitmap plowBmp() {
    gs::Bitmap b(72, 56);
    b.rect(22, 8, 28, 18, 2);
    b.rect(24, 10, 24, 10, 5);
    b.rect(26, 12, 8, 6, 10);
    b.rect(40, 12, 6, 6, 10);
    b.rect(18, 24, 36, 14, 3);
    b.rect(16, 26, 4, 8, 6);
    b.rect(52, 26, 4, 8, 6);
    b.ellipse(24, 42, 8, 8, 9);
    b.ellipse(48, 42, 8, 8, 9);
    b.ellipse(24, 42, 4, 4, 4);
    b.ellipse(48, 42, 4, 4, 4);
    b.rect(8, 36, 56, 4, 6);
    b.rect(6, 40, 8, 10, 8);
    b.rect(58, 40, 8, 10, 8);
    b.rect(30, 40, 12, 12, 7);
    b.line(12, 50, 22, 54, 8, 2);
    b.line(60, 50, 50, 54, 8, 2);
    b.rect(10, 52, 52, 3, 4);
    b.rect(30, 4, 12, 6, 1);
    b.rect(34, 0, 4, 6, 6);
    return b;
}

gs::Bitmap shareBmp() {
    gs::Bitmap b(16, 20);
    b.poly({{8, 1}, {14, 18}, {2, 18}}, 4);
    b.poly({{8, 4}, {12, 16}, {4, 16}}, 7);
    return b;
}

gs::Bitmap postBmp() {
    gs::Bitmap b(10, 28);
    b.rect(3, 0, 4, 26, 1);
    b.rect(4, 0, 2, 26, 2);
    b.rect(1, 2, 8, 3, 4);
    return b;
}

gs::Bitmap baleBmp() {
    gs::Bitmap b(28, 18);
    b.rect(2, 4, 24, 12, 1);
    b.rect(4, 2, 20, 14, 2);
    for (int i = 0; i < 5; i++) b.line(6 + i * 4, 3, 6 + i * 4, 15, 4, 1);
    return b;
}

gs::Bitmap gateBmp() {
    gs::Bitmap b(96, 28);
    b.rect(0, 8, 96, 16, 1);
    b.rect(2, 10, 92, 12, 2);
    b.rect(8, 0, 6, 28, 3);
    b.rect(82, 0, 6, 28, 3);
    b.rect(28, 12, 40, 8, 4);
    return b;
}

gs::Image phrase(gs::VDP& vdp, const std::string& s, int color, int scale) {
    gs::TextStyle st;
    st.scale = scale;
    st.color = color;
    st.outline = 2;
    st.shadow = 0;
    st.spacing = 1;
    return gs::uploadImage(vdp, gs::textBitmap(s, st));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    palettes(vdp);
    gs::TileAlloc tiles(vdp);
    fontTiles(tiles, art.font);
    art.plow = gs::uploadMipped(vdp, plowBmp());
    art.share = gs::uploadMipped(vdp, shareBmp());
    art.post = gs::uploadMipped(vdp, postBmp());
    art.bale = gs::uploadMipped(vdp, baleBmp());
    art.gate = gs::uploadMipped(vdp, gateBmp());
    art.title = phrase(vdp, "PLOW LANE", 1, 3);
    art.sub = phrase(vdp, "STAY IN THE LANE", 3, 1);
    art.tag = phrase(vdp, "THE CLOCK IS THE OTHER CREW", 5, 1);
    art.won = phrase(vdp, "LEG CLEAN", 5, 2);
    art.lost = phrase(vdp, "CREW TOOK IT", 4, 2);
}

}  // namespace plow

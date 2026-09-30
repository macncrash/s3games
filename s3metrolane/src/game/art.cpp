#include "game/art.h"

#include <cstring>

namespace metro {
namespace {

void pal(gs::VDP& v, int p, int i, int r, int g, int b) { v.setColor(p * 16 + i, gs::rgb4(r, g, b)); }

void palettes(gs::VDP& v) {
    for (int p = 0; p < 16; p++) pal(v, p, 0, 0, 0, 0);

    pal(v, PAL_HUD, 1, 14, 15, 13);
    pal(v, PAL_HUD, 2, 1, 2, 3);
    pal(v, PAL_HUD, 3, 15, 12, 3);
    pal(v, PAL_HUD, 4, 15, 8, 2);

    pal(v, PAL_CAR, 1, 1, 1, 2);
    pal(v, PAL_CAR, 2, 9, 10, 12);
    pal(v, PAL_CAR, 3, 5, 6, 8);
    pal(v, PAL_CAR, 4, 15, 14, 6);
    pal(v, PAL_CAR, 5, 4, 12, 14);
    pal(v, PAL_CAR, 6, 14, 5, 3);
    pal(v, PAL_CAR, 7, 12, 12, 13);
    pal(v, PAL_CAR, 8, 3, 3, 4);
    pal(v, PAL_CAR, 9, 15, 15, 14);

    pal(v, PAL_LAMP, 1, 15, 12, 3);
    pal(v, PAL_LAMP, 2, 15, 15, 10);
    pal(v, PAL_LAMP, 3, 4, 4, 5);
    pal(v, PAL_LAMP, 4, 8, 6, 3);

    pal(v, PAL_PILLAR, 1, 4, 4, 5);
    pal(v, PAL_PILLAR, 2, 7, 7, 8);
    pal(v, PAL_PILLAR, 3, 2, 2, 3);
    pal(v, PAL_PILLAR, 4, 11, 8, 4);

    pal(v, PAL_TITLE, 1, 15, 15, 13);
    pal(v, PAL_TITLE, 2, 2, 3, 5);
    pal(v, PAL_TITLE, 3, 15, 11, 2);
    pal(v, PAL_TITLE, 4, 3, 14, 10);

    pal(v, PAL_ALERT, 1, 15, 6, 3);
    pal(v, PAL_ALERT, 2, 4, 1, 2);

    pal(v, PAL_STOP, 1, 15, 15, 15);
    pal(v, PAL_STOP, 2, 2, 2, 3);
    pal(v, PAL_STOP, 3, 14, 3, 3);
    pal(v, PAL_STOP, 4, 15, 13, 2);

    // Road bank: 1-3 shoulder, 4-5 verge, 6-7 railbed, 14 paint, 15 speck.
    pal(v, PAL_ROAD, 1, 2, 2, 3);
    pal(v, PAL_ROAD, 2, 1, 1, 2);
    pal(v, PAL_ROAD, 3, 3, 3, 4);
    pal(v, PAL_ROAD, 4, 5, 5, 5);
    pal(v, PAL_ROAD, 5, 3, 3, 3);
    pal(v, PAL_ROAD, 6, 4, 4, 5);
    pal(v, PAL_ROAD, 7, 2, 2, 3);
    pal(v, PAL_ROAD, 8, 6, 5, 4);
    pal(v, PAL_ROAD, 14, 14, 11, 2);
    pal(v, PAL_ROAD, 15, 6, 6, 7);

    v.setFogColor(gs::rgb4(3, 3, 5));
}

void fontTiles(gs::TileAlloc& tiles, int* out) {
    uint8_t px[64];
    for (int ch = 32; ch < 127; ch++) {
        const uint8_t* g = gs::glyph(char(ch));
        std::memset(px, 0, sizeof px);
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) px[y * 8 + x + 1] = 1;
        out[ch] = tiles.shared(px);
    }
}

gs::Bitmap label(const char* s, int color, int scale) {
    gs::TextStyle st;
    st.scale = scale;
    st.color = color;
    st.outline = 2;
    st.spacing = 1;
    return gs::textBitmap(s, st);
}

void carRear(gs::Bitmap& b, int lean) {
    b.rect(6 + lean, 8, 40, 34, 2);
    b.rect(8 + lean, 10, 36, 12, 5);
    b.rect(10 + lean, 12, 14, 8, 7);
    b.rect(28 + lean, 12, 14, 8, 7);
    b.rect(22 + lean, 14, 8, 4, 9);
    b.rect(8 + lean, 26, 36, 10, 3);
    b.rect(10 + lean, 28, 10, 6, 6);
    b.rect(32 + lean, 28, 10, 6, 8);
    b.rect(4, 38, 8, 8, 1);
    b.rect(40, 38, 8, 8, 1);
    b.ellipse(8.f, 42.f, 3.f, 3.f, 4);
    b.ellipse(44.f, 42.f, 3.f, 3.f, 4);
    b.rect(22 + lean, 36, 8, 4, 1);
    b.rect(6 + lean, 6, 6, 4, 4);
    b.rect(40 + lean, 6, 6, 4, 4);
}

void lampBmp(gs::Bitmap& b) {
    b.rect(10, 10, 4, 28, 3);
    b.ellipse(12.f, 8.f, 8.f, 6.f, 1);
    b.ellipse(12.f, 8.f, 4.f, 3.f, 2);
    b.rect(6, 36, 12, 3, 4);
}

void pillarBmp(gs::Bitmap& b) {
    b.rect(8, 2, 12, 40, 1);
    b.rect(10, 4, 4, 36, 2);
    b.rect(4, 0, 20, 6, 3);
    b.rect(4, 38, 20, 4, 4);
}

void stopBmp(gs::Bitmap& b) {
    b.rect(2, 6, 6, 46, 2);
    b.rect(56, 6, 6, 46, 2);
    b.rect(2, 2, 60, 10, 4);
    for (int i = 0; i < 5; i++) b.rect(10 + i * 9, 4, 5, 5, (i & 1) ? 1 : 3);
    b.rect(10, 18, 44, 6, 3);
    b.rect(18, 28, 28, 4, 1);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    palettes(vdp);
    gs::TileAlloc tiles(vdp, 1);
    fontTiles(tiles, art.font);

    for (int i = 0; i < 3; i++) {
        gs::Bitmap b(52, 50);
        carRear(b, (i - 1) * 4);
        art.car[i] = gs::uploadMipped(vdp, b);
    }
    gs::Bitmap sh(48, 12);
    sh.ellipse(24.f, 6.f, 20.f, 4.f, 1);
    art.shadow = gs::uploadMipped(vdp, sh);

    gs::Bitmap lamp(24, 42);
    lampBmp(lamp);
    art.lamp = gs::uploadMipped(vdp, lamp);

    gs::Bitmap pillar(28, 46);
    pillarBmp(pillar);
    art.pillar = gs::uploadMipped(vdp, pillar);

    gs::Bitmap stop(64, 56);
    stopBmp(stop);
    art.stop = gs::uploadMipped(vdp, stop);

    art.title = gs::uploadImage(vdp, label("METRO LANE", 1, 3));
    art.sub = gs::uploadImage(vdp, label("STAY IN THE LANE", 3, 1));
    art.rule = gs::uploadImage(vdp, label("MISS THE END  THE LEG FAILS", 1, 1));
    art.made = gs::uploadImage(vdp, label("LANE HELD", 4, 3));
    art.out = gs::uploadImage(vdp, label("LEFT THE LANE", 1, 2));
    art.missed = gs::uploadImage(vdp, label("MISSED THE END", 1, 2));
}

}  // namespace metro

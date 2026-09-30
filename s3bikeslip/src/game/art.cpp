#include "game/art.h"

#include <cstring>

namespace slip {
namespace {

void pal(gs::VDP& v, int p, int i, int r, int g, int b) { v.setColor(p * 16 + i, gs::rgb4(r, g, b)); }

void palettes(gs::VDP& v) {
    for (int p = 0; p < 16; p++) pal(v, p, 0, 0, 0, 0);

    pal(v, PAL_HUD, 1, 14, 15, 15);
    pal(v, PAL_HUD, 2, 2, 4, 6);
    pal(v, PAL_HUD, 3, 15, 12, 4);
    pal(v, PAL_HUD, 4, 15, 6, 4);

    pal(v, PAL_BIKE, 1, 1, 2, 3);
    pal(v, PAL_BIKE, 2, 12, 13, 14);
    pal(v, PAL_BIKE, 3, 14, 3, 4);
    pal(v, PAL_BIKE, 4, 8, 2, 3);
    pal(v, PAL_BIKE, 5, 15, 12, 3);
    pal(v, PAL_BIKE, 6, 6, 6, 7);
    pal(v, PAL_BIKE, 7, 10, 8, 6);
    pal(v, PAL_BIKE, 8, 3, 5, 8);
    pal(v, PAL_BIKE, 9, 15, 14, 12);

    pal(v, PAL_POST, 1, 5, 4, 3);
    pal(v, PAL_POST, 2, 8, 7, 5);
    pal(v, PAL_POST, 3, 3, 5, 4);
    pal(v, PAL_POST, 4, 12, 11, 8);
    pal(v, PAL_POST, 5, 2, 3, 4);

    pal(v, PAL_BUOY, 1, 14, 2, 3);
    pal(v, PAL_BUOY, 2, 15, 14, 12);
    pal(v, PAL_BUOY, 3, 3, 6, 8);
    pal(v, PAL_BUOY, 4, 15, 10, 2);

    pal(v, PAL_TITLE, 1, 15, 15, 14);
    pal(v, PAL_TITLE, 2, 2, 3, 6);
    pal(v, PAL_TITLE, 3, 12, 8, 3);
    pal(v, PAL_TITLE, 4, 4, 12, 10);

    pal(v, PAL_ALERT, 1, 15, 8, 4);
    pal(v, PAL_ALERT, 2, 4, 1, 2);

    pal(v, PAL_ROAD, 1, 3, 6, 4);
    pal(v, PAL_ROAD, 2, 2, 5, 3);
    pal(v, PAL_ROAD, 3, 5, 8, 5);
    pal(v, PAL_ROAD, 4, 7, 6, 4);
    pal(v, PAL_ROAD, 5, 5, 4, 3);
    pal(v, PAL_ROAD, 6, 6, 6, 7);
    pal(v, PAL_ROAD, 7, 4, 4, 5);
    pal(v, PAL_ROAD, 8, 8, 7, 5);
    pal(v, PAL_ROAD, 11, 2, 5, 8);
    pal(v, PAL_ROAD, 12, 3, 7, 10);
    pal(v, PAL_ROAD, 13, 6, 11, 13);
    pal(v, PAL_ROAD, 14, 14, 12, 4);
    pal(v, PAL_ROAD, 15, 8, 8, 9);

    v.setFogColor(gs::rgb4(4, 6, 8));
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

void bikeFrame(gs::Bitmap& b, int lean) {
    b.rect(14 + lean, 8, 20, 16, 2);
    b.rect(18 + lean, 4, 12, 8, 8);
    b.ellipse(24.f + lean, 8.f, 5.f, 5.f, 5);
    b.rect(22 + lean, 6, 4, 3, 9);
    b.rect(10, 22, 8, 14, 6);
    b.rect(30, 22, 8, 14, 6);
    b.ellipse(14.f, 36.f, 7.f, 7.f, 1);
    b.ellipse(34.f, 36.f, 7.f, 7.f, 1);
    b.ellipse(14.f, 36.f, 3.f, 3.f, 7);
    b.ellipse(34.f, 36.f, 3.f, 3.f, 7);
    b.line(14, 30, 34, 30, 3, 2);
    b.line(22 + lean, 20, 14, 32, 3, 2);
    b.rect(16 + lean * 2, 24, 16, 4, 4);
}

void postBmp(gs::Bitmap& b) {
    b.rect(10, 4, 8, 40, 1);
    b.rect(8, 2, 12, 6, 4);
    b.rect(12, 18, 4, 10, 3);
    b.rect(6, 40, 16, 4, 2);
    b.rect(4, 42, 6, 8, 5);
}

void buoyBmp(gs::Bitmap& b) {
    b.ellipse(12.f, 14.f, 9.f, 10.f, 1);
    b.rect(4, 12, 16, 5, 2);
    b.rect(11, 2, 2, 8, 4);
    b.ellipse(12.f, 24.f, 6.f, 3.f, 3);
}

void mouthBmp(gs::Bitmap& b) {
    b.rect(2, 8, 6, 36, 1);
    b.rect(40, 8, 6, 36, 1);
    b.rect(2, 6, 44, 6, 4);
    b.rect(8, 14, 32, 4, 2);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    palettes(vdp);
    gs::TileAlloc tiles(vdp, 1);
    fontTiles(tiles, art.font);

    for (int i = 0; i < 3; i++) {
        gs::Bitmap b(48, 48);
        bikeFrame(b, (i - 1) * 4);
        art.bike[i] = gs::uploadMipped(vdp, b);
    }
    gs::Bitmap sh(40, 12);
    sh.ellipse(20.f, 6.f, 16.f, 4.f, 1);
    art.shadow = gs::uploadMipped(vdp, sh);

    gs::Bitmap post(28, 52);
    postBmp(post);
    art.post = gs::uploadMipped(vdp, post);

    gs::Bitmap buoy(24, 32);
    buoyBmp(buoy);
    art.buoy = gs::uploadMipped(vdp, buoy);

    gs::Bitmap mouth(48, 48);
    mouthBmp(mouth);
    art.mouth = gs::uploadMipped(vdp, mouth);

    art.title = gs::uploadImage(vdp, label("BIKE SLIP", 1, 3));
    art.sub = gs::uploadImage(vdp, label("BERTH BEFORE THE TIDE", 3, 1));
    art.rule = gs::uploadImage(vdp, label("MISS THE END  THE LEG FAILS", 1, 1));
    art.berthed = gs::uploadImage(vdp, label("BERTHED", 4, 3));
    art.missed = gs::uploadImage(vdp, label("MISSED THE END", 1, 2));
    art.tide = gs::uploadImage(vdp, label("TIDE TURNED", 1, 2));
    art.edged = gs::uploadImage(vdp, label("OFF THE PIER", 1, 2));
}

}  // namespace slip

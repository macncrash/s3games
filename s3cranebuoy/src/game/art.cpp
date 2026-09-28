#include "game/art.h"

#include "game/world.h"

#include <cmath>
#include <string>
#include <vector>

namespace cranebuoy {
namespace {

void textPal(gs::VDP& v, int pal, uint16_t ink) {
    v.setColor(pal * 16 + 1, ink);
    v.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

void paintPals(gs::VDP& vdp) {
    textPal(vdp, PAL_WHITE, gs::rgb4(15, 15, 15));
    textPal(vdp, PAL_AMBER, gs::rgb4(15, 12, 3));
    textPal(vdp, PAL_RED, gs::rgb4(15, 4, 3));
    textPal(vdp, PAL_GREEN, gs::rgb4(7, 15, 6));
    textPal(vdp, PAL_BANNER, gs::rgb4(15, 14, 8));

    const uint16_t harbor[16] = {
        0,
        gs::rgb4(2, 6, 11),
        gs::rgb4(3, 8, 13),
        gs::rgb4(5, 11, 14),
        gs::rgb4(10, 14, 15),
        gs::rgb4(10, 9, 6),
        gs::rgb4(6, 5, 4),
        gs::rgb4(12, 12, 11),
        gs::rgb4(14, 12, 4),
        gs::rgb4(8, 8, 7),
        gs::rgb4(4, 5, 5),
        gs::rgb4(9, 11, 8),
        gs::rgb4(15, 15, 14),
        gs::rgb4(13, 4, 3),
        gs::rgb4(7, 8, 9),
        gs::rgb4(2, 2, 3),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_HARBOR * 16 + i, harbor[i]);

    vdp.setColor(PAL_CRANE * 16 + 1, gs::rgb4(14, 14, 15));
    vdp.setColor(PAL_CRANE * 16 + 2, gs::rgb4(7, 9, 11));
    vdp.setColor(PAL_CRANE * 16 + 3, gs::rgb4(3, 4, 5));
    vdp.setColor(PAL_CRANE * 16 + 4, gs::rgb4(12, 14, 15));
    vdp.setColor(PAL_CRANE * 16 + 5, gs::rgb4(15, 12, 2));
    vdp.setColor(PAL_CRANE * 16 + 6, gs::rgb4(4, 4, 5));
    vdp.setColor(PAL_CRANE * 16 + 7, gs::rgb4(11, 3, 2));

    vdp.setColor(PAL_BUOY * 16 + 1, gs::rgb4(14, 3, 2));
    vdp.setColor(PAL_BUOY * 16 + 2, gs::rgb4(15, 15, 14));
    vdp.setColor(PAL_BUOY * 16 + 3, gs::rgb4(15, 12, 2));
    vdp.setColor(PAL_BUOY * 16 + 4, gs::rgb4(3, 3, 4));
    vdp.setColor(PAL_BUOY * 16 + 5, gs::rgb4(6, 14, 5));

    vdp.setColor(PAL_WAKE * 16 + 1, gs::rgb4(12, 15, 15));
    vdp.setColor(PAL_WAKE * 16 + 2, gs::rgb4(8, 12, 14));

    vdp.setColor(PAL_MARK * 16 + 1, gs::rgb4(15, 14, 4));
    vdp.setColor(PAL_MARK * 16 + 2, gs::rgb4(15, 15, 15));
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& art) {
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) {
                    px[y * 8 + x + 1] = 1;
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
                }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        art.font[c - 32] = t;
    }
}

gs::Bitmap harborPicture() {
    gs::Bitmap b(320, 224);
    for (int y = 0; y < 224; y++) {
        int c = (y / 7) % 2 ? 2 : 3;
        if (y < 18 || y > 214) c = 10;
        b.rect(0, float(y), 320, 1, c);
    }
    b.rect(0, 0, 16, 224, 10);
    b.rect(304, 0, 16, 224, 10);
    b.rect(0, 0, 320, 16, 10);
    for (int i = 0; i < 8; i++) b.ellipse(20.f + i * 38.f, 8.f, 10, 6, 9);
    for (int i = 0; i < 6; i++) b.ellipse(28.f + i * 46.f, 216.f, 12, 5, 11);

    b.rect(DOCK_L - 8, DOCK_T, DOCK_R - DOCK_L + 16, DOCK_B - DOCK_T + 8, 6);
    b.rect(DOCK_L, DOCK_T + 4, DOCK_R - DOCK_L, DOCK_B - DOCK_T, 5);
    b.rect(DOCK_L, DOCK_T + 4, DOCK_R - DOCK_L, 4, 7);
    for (int i = 0; i < 5; i++) b.rect(DOCK_L + 8 + i * 16, DOCK_T + 16, 3, DOCK_B - DOCK_T - 20, 6);
    b.rect((DOCK_L + DOCK_R) * 0.5f - 18, DOCK_T + 10, 36, 10, 8);
    gs::TextStyle ink{1, 15, 0, 0, 1};
    gs::Bitmap plate = gs::textBitmap("DOCK", ink);
    b.blit(plate, int((DOCK_L + DOCK_R) * 0.5f) - plate.w / 2, int(DOCK_T) + 22);

    for (int i = 0; i < 9; i++) {
        float y = 28.f + i * 16.f;
        b.rect(24, y, 10, 2, 4);
        b.rect(286, y + 6, 10, 2, 4);
    }
    return b;
}

gs::Bitmap craneArt(float ang) {
    gs::Bitmap b(48, 48);
    const float cx = 24.f, cy = 24.f;
    auto P = [&](float x, float y) -> gs::Pt {
        float c = std::cos(ang), s = std::sin(ang);
        return {cx + x * c - y * s, cy + x * s + y * c};
    };
    b.poly({P(14, -8), P(16, -4), P(16, 4), P(14, 8), P(-12, 9), P(-16, 0), P(-12, -9)}, 2);
    b.poly({P(14, -8), P(16, 0), P(14, 8), P(8, 6), P(8, -6)}, 1);
    b.poly({P(4, -5), P(4, 5), P(-6, 5), P(-6, -5)}, 4);
    b.poly({P(2, -3), P(2, 1), P(-3, 1), P(-3, -3)}, 3);
    auto tip = P(20, 0);
    b.line(P(6, 0).first, P(6, 0).second, tip.first, tip.second, 5, 2.f);
    b.ellipse(tip.first, tip.second, 2.2f, 2.2f, 6);
    b.ellipse(P(-11, 0).first, P(-11, 0).second, 3.2f, 3.2f, 7);
    return b;
}

gs::Bitmap buoyArt(bool done) {
    gs::Bitmap b(18, 18);
    b.ellipse(9, 10, 7, 6, 4);
    b.ellipse(9, 8, 6, 6, 1);
    b.rect(3, 7, 12, 3, done ? 5 : 2);
    b.ellipse(9, 4, 2, 2, 3);
    return b;
}

gs::Bitmap wakeArt() {
    gs::Bitmap b(22, 10);
    b.ellipse(11, 5, 10, 3, 1);
    b.ellipse(6, 5, 3, 2, 2);
    b.ellipse(16, 5, 3, 2, 2);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(28, 12);
    b.ellipse(14, 6, 12, 4, 1);
    return b;
}

gs::Bitmap markArt() {
    gs::Bitmap b(10, 10);
    b.poly({{5, 1}, {9, 5}, {5, 9}, {1, 5}}, 1);
    b.rect(4, 4, 2, 2, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    paintPals(vdp);
    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    gs::bitmapToPlane(tiles, vdp.B, 0, 0, harborPicture(), PAL_HARBOR);
    vdp.A.enabled = false;

    for (int i = 0; i < 8; i++) art.crane[i] = gs::uploadMipped(vdp, craneArt(float(i) * 0.78539816f));
    art.buoy = gs::uploadMipped(vdp, buoyArt(false));
    art.buoyDone = gs::uploadMipped(vdp, buoyArt(true));
    art.wake = gs::uploadMipped(vdp, wakeArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.mark = gs::uploadMipped(vdp, markArt());
    gs::TextStyle banner{3, 1, 15, 0, 1};
    art.banner = gs::uploadMipped(vdp, gs::textBitmap("CRANEBUOY", banner));
}

}  // namespace cranebuoy

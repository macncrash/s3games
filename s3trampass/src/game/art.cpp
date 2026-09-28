#include "art.h"

#include <cstdint>
#include <cstring>

namespace trampass {
namespace {

void pal(gs::VDP& v, int p, int i, int r, int g, int b) { v.setColor(p * 16 + i, gs::rgb4(r, g, b)); }

void textPal(gs::VDP& v, int p, int r, int g, int b) {
    pal(v, p, 1, r, g, b);
    pal(v, p, 2, 1, 2, 4);
    pal(v, p, 3, 0, 0, 2);
}

void palettes(gs::VDP& v) {
    textPal(v, PAL_HUD, 15, 15, 15);
    textPal(v, PAL_GOLD, 15, 13, 3);
    textPal(v, PAL_ALERT, 15, 4, 3);

    pal(v, PAL_TRAM, 1, 14, 3, 3);    // red body
    pal(v, PAL_TRAM, 2, 10, 2, 2);
    pal(v, PAL_TRAM, 3, 15, 14, 8);   // cream band
    pal(v, PAL_TRAM, 4, 6, 9, 12);    // glass
    pal(v, PAL_TRAM, 5, 15, 15, 14);  // destination
    pal(v, PAL_TRAM, 6, 15, 12, 2);   // headlamp
    pal(v, PAL_TRAM, 7, 3, 3, 4);     // underframe
    pal(v, PAL_TRAM, 8, 2, 2, 2);
    pal(v, PAL_TRAM, 9, 15, 15, 15);
    pal(v, PAL_TRAM, 10, 8, 8, 9);    // pantograph
    pal(v, PAL_TRAM, 11, 12, 6, 2);
    pal(v, PAL_TRAM, 12, 1, 1, 1);
    pal(v, PAL_TRAM, 13, 5, 5, 6);
    pal(v, PAL_TRAM, 14, 13, 13, 12);
    pal(v, PAL_TRAM, 15, 0, 0, 1);

    pal(v, PAL_RIVAL, 1, 2, 6, 4);
    pal(v, PAL_RIVAL, 2, 1, 4, 3);
    pal(v, PAL_RIVAL, 3, 12, 13, 10);
    pal(v, PAL_RIVAL, 4, 5, 8, 10);
    pal(v, PAL_RIVAL, 5, 14, 14, 13);
    pal(v, PAL_RIVAL, 6, 14, 10, 2);
    pal(v, PAL_RIVAL, 7, 2, 3, 3);
    pal(v, PAL_RIVAL, 8, 1, 1, 1);
    pal(v, PAL_RIVAL, 9, 15, 15, 15);
    pal(v, PAL_RIVAL, 10, 7, 8, 8);
    pal(v, PAL_RIVAL, 11, 10, 8, 3);
    pal(v, PAL_RIVAL, 12, 1, 1, 2);
    pal(v, PAL_RIVAL, 13, 4, 5, 5);
    pal(v, PAL_RIVAL, 14, 11, 12, 11);
    pal(v, PAL_RIVAL, 15, 0, 1, 1);

    pal(v, PAL_PINE, 1, 2, 5, 3);
    pal(v, PAL_PINE, 2, 3, 8, 4);
    pal(v, PAL_PINE, 3, 6, 4, 2);
    pal(v, PAL_PINE, 4, 14, 15, 15);
    pal(v, PAL_PINE, 5, 1, 2, 2);

    pal(v, PAL_MAST, 1, 6, 6, 7);
    pal(v, PAL_MAST, 2, 3, 3, 4);
    pal(v, PAL_MAST, 3, 12, 10, 3);
    pal(v, PAL_MAST, 4, 9, 9, 10);
    pal(v, PAL_MAST, 5, 2, 2, 3);

    pal(v, PAL_BANNER, 1, 15, 15, 15);
    pal(v, PAL_BANNER, 2, 2, 2, 4);
    pal(v, PAL_BANNER, 3, 12, 2, 2);
    pal(v, PAL_BANNER, 4, 15, 12, 3);
    pal(v, PAL_BANNER, 5, 4, 3, 2);
    pal(v, PAL_BANNER, 6, 8, 8, 9);

    pal(v, PAL_FX, 1, 15, 15, 15);
    pal(v, PAL_FX, 2, 12, 13, 15);
    pal(v, PAL_FX, 3, 7, 8, 10);

    pal(v, PAL_RANGE, 1, 5, 7, 11);
    pal(v, PAL_RANGE, 2, 3, 5, 9);
    pal(v, PAL_RANGE, 3, 2, 4, 7);
    pal(v, PAL_RANGE, 4, 14, 15, 15);
    pal(v, PAL_RANGE, 5, 2, 4, 3);
    pal(v, PAL_RANGE, 6, 7, 8, 10);
    pal(v, PAL_RANGE, 7, 11, 12, 14);
    pal(v, PAL_RANGE, 8, 15, 13, 8);
    pal(v, PAL_RANGE, 9, 4, 5, 7);

    pal(v, PAL_ROAD, 1, 4, 5, 5);
    pal(v, PAL_ROAD, 2, 3, 3, 4);
    pal(v, PAL_ROAD, 3, 6, 6, 6);
    pal(v, PAL_ROAD, 4, 8, 8, 7);
    pal(v, PAL_ROAD, 5, 5, 5, 5);
    pal(v, PAL_ROAD, 6, 2, 2, 3);
    pal(v, PAL_ROAD, 7, 4, 4, 5);
    pal(v, PAL_ROAD, 8, 7, 7, 6);
    pal(v, PAL_ROAD, 9, 2, 2, 3);
    pal(v, PAL_ROAD, 10, 5, 5, 6);
    pal(v, PAL_ROAD, 11, 14, 12, 3);
    pal(v, PAL_ROAD, 12, 3, 4, 6);
    pal(v, PAL_ROAD, 13, 9, 9, 8);
    pal(v, PAL_ROAD, 14, 12, 3, 2);
    pal(v, PAL_ROAD, 15, 6, 6, 7);

    v.setFogColor(gs::rgb4(8, 9, 12));
}

void peak(gs::Bitmap& b, float x, float top, float half, int body, int snow) {
    const float base = float(b.h - 1);
    b.poly({{x - half, base}, {x + half, base}, {x + half * 0.14f, top + 9.f}, {x, top}, {x - half * 0.12f, top + 7.f}},
           body);
    b.poly({{x - half * 0.14f, top + 11.f}, {x, top + 1.f}, {x + half * 0.16f, top + 12.f}}, snow);
}

void range(gs::VDP& v, gs::TileAlloc& tiles) {
    gs::Bitmap b(512, 96);
    peak(b, 70, 22, 120, 2, 4);
    peak(b, 200, 38, 70, 1, 4);
    peak(b, 320, 16, 150, 3, 4);
    peak(b, 450, 30, 90, 2, 4);
    for (int i = 0; i < 14; i++) {
        float x = 20.f + float(i) * 36.f;
        float h = 10.f + float((i * 7) % 9);
        b.poly({{x, 94.f}, {x + 4.f, 94.f - h}, {x + 9.f, 94.f}}, 5);
    }
    gs::bitmapToPlane(tiles, v.B, 0, 0, b, PAL_RANGE);
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

void tramBody(gs::Bitmap& b) {
    b.rect(10, 22, 44, 36, 1);
    b.rect(12, 24, 40, 8, 3);
    b.rect(14, 34, 12, 10, 4);
    b.rect(34, 34, 12, 10, 4);
    b.rect(18, 18, 28, 6, 5);
    b.rect(28, 19, 8, 4, 1);
    b.line(22, 8, 32, 16, 10, 1.4f);
    b.line(42, 8, 32, 16, 10, 1.4f);
    b.line(18, 6, 46, 6, 10, 1.2f);
    b.rect(30, 4, 4, 4, 10);
    b.rect(8, 52, 8, 6, 7);
    b.rect(44, 52, 8, 6, 7);
    b.rect(16, 54, 8, 8, 12);
    b.rect(36, 54, 8, 8, 12);
    b.rect(26, 48, 12, 4, 6);
    b.rect(6, 30, 4, 8, 2);
    b.rect(54, 30, 4, 8, 2);
}

void pine(gs::Bitmap& b) {
    b.poly({{16, 40}, {8, 22}, {24, 22}}, 1);
    b.poly({{16, 28}, {6, 12}, {26, 12}}, 2);
    b.poly({{16, 16}, {9, 4}, {23, 4}}, 1);
    b.rect(14, 40, 4, 8, 3);
    b.rect(12, 2, 6, 3, 4);
}

void mast(gs::Bitmap& b) {
    b.rect(6, 8, 3, 40, 1);
    b.rect(2, 6, 12, 3, 4);
    b.line(8, 8, 14, 2, 2, 1.2f);
    b.rect(4, 46, 7, 3, 5);
    b.rect(10, 10, 3, 3, 3);
}

void rock(gs::Bitmap& b) {
    b.poly({{4, 22}, {16, 6}, {28, 22}}, 1);
    b.poly({{8, 22}, {16, 10}, {20, 22}}, 2);
    b.rect(6, 20, 18, 4, 4);
}

void banner(gs::Bitmap& b, int band) {
    b.rect(0, 4, float(b.w), 16, 2);
    b.rect(2, 6, float(b.w - 4), 12, band);
    b.rect(0, 0, float(b.w), 3, 6);
}

void chevron(gs::Bitmap& b, int dir) {
    if (dir < 0) {
        b.poly({{20, 2}, {4, 12}, {20, 22}, {14, 12}}, 4);
        b.poly({{28, 6}, {16, 12}, {28, 18}, {22, 12}}, 1);
    } else {
        b.poly({{4, 2}, {20, 12}, {4, 22}, {10, 12}}, 4);
        b.poly({{0, 6}, {12, 12}, {0, 18}, {6, 12}}, 1);
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    palettes(vdp);
    gs::TileAlloc tiles(vdp, 1);
    range(vdp, tiles);
    font(tiles, art.font);

    gs::Bitmap tram(64, 68);
    tramBody(tram);
    art.tram = gs::uploadMipped(vdp, tram);

    gs::Bitmap riv(64, 68);
    tramBody(riv);
    art.rival = gs::uploadMipped(vdp, riv);

    gs::Bitmap pn(32, 48);
    pine(pn);
    art.pine = gs::uploadMipped(vdp, pn);

    gs::Bitmap ms(16, 52);
    mast(ms);
    art.mast = gs::uploadMipped(vdp, ms);

    gs::Bitmap rk(32, 26);
    rock(rk);
    art.rock = gs::uploadMipped(vdp, rk);

    gs::Bitmap passB(72, 24);
    banner(passB, 3);
    art.passBan = gs::uploadMipped(vdp, passB);
    gs::Bitmap yardB(72, 24);
    banner(yardB, 4);
    art.yardBan = gs::uploadMipped(vdp, yardB);

    gs::Bitmap cl(32, 24);
    chevron(cl, -1);
    art.chevL = gs::uploadMipped(vdp, cl);
    gs::Bitmap cr(32, 24);
    chevron(cr, 1);
    art.chevR = gs::uploadMipped(vdp, cr);

    gs::Bitmap fl(4, 4);
    fl.rect(1, 0, 2, 4, 1);
    fl.rect(0, 1, 4, 2, 2);
    art.flake = gs::uploadMipped(vdp, fl);

    gs::Bitmap sh(40, 8);
    sh.ellipse(20, 4, 16, 3, 1);
    art.shadow = gs::uploadMipped(vdp, sh);

    gs::TextStyle big{3, 1, 2, 3, 1};
    gs::TextStyle sub{1, 1, 2, 0, 1};
    art.title = gs::uploadImage(vdp, gs::textBitmap("TRAM PASS", big));
    art.sub = gs::uploadImage(vdp, gs::textBitmap("BEAT THE OTHER CREW", sub));
    art.go = gs::uploadImage(vdp, gs::textBitmap("TAKE THE TRAM", big));
    art.clear = gs::uploadImage(vdp, gs::textBitmap("PASS CLEAR", big));
    art.closed = gs::uploadImage(vdp, gs::textBitmap("CREW CLOSED IT", big));
    art.derail = gs::uploadImage(vdp, gs::textBitmap("LEFT THE RAIL", big));
}

}  // namespace trampass

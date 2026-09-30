#include "game/art.h"

#include <initializer_list>

namespace buoy {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
    vdp.setColor(pal * 16 + 0, 0);
}

void loadFont(gs::VDP& vdp, Art& art) {
    gs::TileAlloc tiles(vdp);
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (!g[y * 5 + x]) continue;
                px[y * 8 + x + 1] = 1;
                if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
            }
        }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        art.font[c - 32] = t;
    }
}

gs::Mipped words(gs::VDP& vdp, const char* text, int scale, int fill, int edge) {
    gs::TextStyle st{scale, fill, edge, 0, 1};
    return gs::uploadMipped(vdp, gs::textBitmap(text, st));
}

void paintHeli(gs::Bitmap& b) {
    b.ellipse(40, 28, 16, 22, 1);
    b.poly({{22, 18}, {58, 18}, {62, 28}, {58, 40}, {22, 40}, {18, 28}}, 2);
    b.ellipse(40, 26, 8, 10, 3);
    b.rect(36, 18, 8, 8, 4);
    b.poly({{36, 44}, {44, 44}, {48, 62}, {32, 62}}, 5);
    b.rect(30, 58, 20, 6, 6);
    b.ellipse(40, 28, 3, 3, 8);
    b.line(40, 8, 40, 48, 7, 1.4f);
    b.line(18, 28, 62, 28, 7, 1.2f);
    b.outline(15, false);
}

void paintRotor(gs::Bitmap& b) {
    b.ellipse(32, 32, 28, 28, 1);
    b.ellipse(32, 32, 22, 8, 2);
    b.ellipse(32, 32, 8, 22, 2);
    b.ellipse(32, 32, 4, 4, 3);
    b.line(6, 10, 58, 54, 4, 1.2f);
    b.line(6, 54, 58, 10, 4, 1.2f);
}

void paintBuoy(gs::Bitmap& b) {
    b.ellipse(16, 28, 12, 5, 4);
    b.ellipse(16, 16, 10, 10, 1);
    b.ellipse(16, 14, 10, 6, 2);
    b.rect(14, 6, 4, 8, 3);
    b.ellipse(16, 6, 2, 2, 5);
    b.line(8, 16, 24, 16, 6, 1.4f);
    b.outline(15, false);
}

void paintFlag(gs::Bitmap& b) {
    b.rect(2, 2, 2, 20, 1);
    b.poly({{4, 3}, {22, 7}, {4, 12}}, 2);
    b.outline(15, false);
}

void paintDock(gs::Bitmap& b) {
    b.rect(0, 8, b.w, b.h - 8, 1);
    for (int x = 2; x < b.w - 4; x += 10) b.rect(x, 10, 7, b.h - 12, (x / 10) % 2 ? 2 : 3);
    b.rect(0, 8, b.w, 5, 4);
    b.rect(8, b.h / 2 - 6, b.w - 16, 12, 5);
    b.ellipse(12, b.h - 8, 3, 3, 6);
    b.ellipse(b.w - 12, b.h - 8, 3, 3, 6);
    b.outline(15, false);
}

void paintWake(gs::Bitmap& b) {
    b.ellipse(16, 10, 14, 6, 1);
    b.ellipse(16, 10, 8, 3, 2);
}

void paintShore(gs::Bitmap& b) {
    b.rect(0, 0, b.w, b.h, 1);
    for (int x = 0; x < b.w; x += 9) b.rect(x, 4, 6, b.h - 4, (x / 9) % 2 ? 2 : 3);
    b.rect(0, 0, b.w, 5, 4);
    b.line(0, float(b.h - 3), float(b.w), float(b.h - 3), 5, 2.f);
}

void paintGull(gs::Bitmap& b) {
    b.line(2, 8, 12, 4, 1, 1.6f);
    b.line(12, 4, 22, 8, 1, 1.6f);
    b.line(8, 7, 12, 6, 2, 1.2f);
}

void paintShadow(gs::Bitmap& b) { b.ellipse(16, 8, 14, 6, 1); }

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 15, 15), gs::rgb4(4, 6, 8), gs::rgb4(8, 10, 12)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 6, 4), gs::rgb4(8, 2, 2), gs::rgb4(15, 12, 6)});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 9), gs::rgb4(2, 6, 3), gs::rgb4(14, 15, 10)});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 13, 6), gs::rgb4(6, 4, 1), gs::rgb4(15, 8, 3)});
    setPal(vdp, PAL_HELI, {0, gs::rgb4(12, 13, 14), gs::rgb4(7, 8, 10), gs::rgb4(3, 8, 12), gs::rgb4(14, 15, 15),
                           gs::rgb4(4, 5, 6), gs::rgb4(2, 2, 3), gs::rgb4(9, 9, 8), gs::rgb4(15, 12, 4)});
    setPal(vdp, PAL_DOCK, {0, gs::rgb4(8, 7, 5), gs::rgb4(11, 9, 6), gs::rgb4(6, 5, 4), gs::rgb4(13, 12, 8),
                           gs::rgb4(14, 13, 6), gs::rgb4(15, 6, 3)});
    setPal(vdp, PAL_BUOY, {0, gs::rgb4(14, 4, 3), gs::rgb4(15, 8, 4), gs::rgb4(6, 6, 7), gs::rgb4(3, 5, 8),
                           gs::rgb4(15, 14, 8), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_BUOY2, {0, gs::rgb4(14, 12, 2), gs::rgb4(15, 15, 6), gs::rgb4(6, 6, 7), gs::rgb4(3, 5, 8),
                            gs::rgb4(15, 14, 8), gs::rgb4(8, 4, 2)});
    setPal(vdp, PAL_BUOY3, {0, gs::rgb4(3, 10, 14), gs::rgb4(8, 14, 15), gs::rgb4(6, 6, 7), gs::rgb4(3, 5, 8),
                            gs::rgb4(15, 14, 8), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_WAKE, {0, gs::rgb4(10, 14, 15), gs::rgb4(14, 15, 15)});
    setPal(vdp, PAL_SHORE, {0, gs::rgb4(4, 8, 3), gs::rgb4(6, 10, 4), gs::rgb4(3, 6, 3), gs::rgb4(8, 11, 5),
                            gs::rgb4(9, 12, 10)});
    setPal(vdp, PAL_FLAG, {0, gs::rgb4(12, 12, 13), gs::rgb4(15, 4, 3)});
    setPal(vdp, PAL_ROTOR, {0, gs::rgb4(6, 8, 9), gs::rgb4(12, 14, 15), gs::rgb4(15, 12, 4), gs::rgb4(9, 11, 12)});
    setPal(vdp, PAL_SUN, {0, gs::rgb4(15, 14, 6), gs::rgb4(15, 10, 3)});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(15, 15, 14), gs::rgb4(4, 8, 10)});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(5, 6, 7), gs::rgb4(2, 3, 4)});
    vdp.setFogColor(gs::rgb4(4, 8, 10));

    loadFont(vdp, art);
    gs::Bitmap heli(80, 72);
    paintHeli(heli);
    art.heli = gs::uploadMipped(vdp, heli);
    gs::Bitmap rotor(64, 64);
    paintRotor(rotor);
    art.rotor = gs::uploadMipped(vdp, rotor);
    gs::Bitmap sh(32, 16);
    paintShadow(sh);
    art.shadow = gs::uploadMipped(vdp, sh);
    gs::Bitmap dock(96, 48);
    paintDock(dock);
    art.dock = gs::uploadMipped(vdp, dock);
    gs::Bitmap by(32, 36);
    paintBuoy(by);
    art.buoy = gs::uploadMipped(vdp, by);
    gs::Bitmap fl(24, 24);
    paintFlag(fl);
    art.flag = gs::uploadMipped(vdp, fl);
    gs::Bitmap wk(32, 20);
    paintWake(wk);
    art.wake = gs::uploadMipped(vdp, wk);
    gs::Bitmap shore(160, 36);
    paintShore(shore);
    art.shore = gs::uploadMipped(vdp, shore);
    gs::Bitmap gull(24, 12);
    paintGull(gull);
    art.gull = gs::uploadMipped(vdp, gull);
    art.title = words(vdp, "HELIBUOY", 3, 1, 2);
    art.docked = words(vdp, "DOCKED", 3, 1, 2);
    art.late = words(vdp, "CREW BEAT YOU", 2, 1, 2);
    art.dipped = words(vdp, "IN THE DRINK", 2, 1, 2);
}

}  // namespace buoy

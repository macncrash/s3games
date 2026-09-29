#include "art.h"

namespace cranelane {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i >= 16) break;
        vdp.setColor(pal * 16 + i++, c);
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap paintCrane() {
    Bitmap b(48, 72);
    b.rect(6, 40, 36, 16, 1);
    b.rect(4, 48, 8, 14, 3);
    b.rect(36, 48, 8, 14, 3);
    b.ellipse(8, 62, 5, 5, 4);
    b.ellipse(40, 62, 5, 5, 4);
    b.ellipse(8, 62, 2, 2, 8);
    b.ellipse(40, 62, 2, 2, 8);
    b.rect(8, 34, 14, 10, 2);
    b.rect(10, 36, 6, 4, 5);
    b.rect(28, 36, 12, 12, 6);
    b.rect(22, 8, 4, 36, 7);
    for (int i = 0; i < 5; i++) b.line(16, 12 + i * 6, 32, 18 + i * 5, 9, 1.2f);
    for (int i = 0; i < 4; i++) b.line(32, 14 + i * 7, 16, 20 + i * 6, 9, 1.2f);
    b.rect(18, 4, 12, 6, 1);
    b.rect(23, 0, 2, 8, 7);
    b.rect(2, 42, 6, 6, 6);
    b.outline(15, false);
    return b;
}

Bitmap paintHook() {
    Bitmap b(10, 18);
    b.line(5, 0, 5, 8, 1, 1.4f);
    b.ellipse(5, 12, 3.2f, 4.2f, 2);
    b.rect(4, 8, 2, 3, 1);
    return b;
}

Bitmap paintShadow() {
    Bitmap b(44, 12);
    b.ellipse(22, 6, 20, 4, 1);
    return b;
}

Bitmap paintStack() {
    Bitmap b(28, 36);
    b.rect(2, 18, 24, 6, 1);
    b.rect(4, 12, 20, 6, 2);
    b.rect(3, 6, 22, 6, 3);
    b.ellipse(6, 28, 4, 4, 4);
    b.ellipse(22, 28, 4, 4, 4);
    b.rect(10, 2, 8, 5, 5);
    return b;
}

Bitmap paintBarrier() {
    Bitmap b(32, 16);
    b.poly({{2, 14}, {8, 2}, {24, 2}, {30, 14}}, 1);
    b.rect(6, 6, 20, 3, 2);
    b.rect(8, 10, 6, 3, 3);
    b.rect(18, 10, 6, 3, 3);
    return b;
}

Bitmap paintCone() {
    Bitmap b(14, 20);
    b.poly({{7, 1}, {2, 16}, {12, 16}}, 1);
    b.rect(4, 8, 6, 2, 2);
    b.rect(3, 12, 8, 2, 2);
    b.rect(1, 16, 12, 3, 3);
    return b;
}

Bitmap paintFlood() {
    Bitmap b(16, 36);
    b.rect(7, 10, 2, 22, 2);
    b.rect(2, 4, 12, 6, 1);
    b.rect(4, 5, 8, 3, 3);
    b.rect(3, 32, 10, 3, 2);
    return b;
}

Bitmap paintGate() {
    Bitmap b(72, 32);
    b.rect(2, 2, 4, 28, 1);
    b.rect(66, 2, 4, 28, 1);
    b.rect(2, 2, 68, 5, 2);
    b.rect(10, 10, 8, 6, 3);
    b.rect(26, 10, 8, 6, 4);
    b.rect(42, 10, 8, 6, 3);
    b.rect(54, 10, 8, 6, 4);
    b.line(6, 6, 36, 28, 1, 1.2f);
    b.line(66, 6, 36, 28, 1, 1.2f);
    return b;
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
                if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 2;
            }
        }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        art.font[c - 32] = t;
    }
}

gs::Mipped words(gs::VDP& vdp, const char* text, int scale, int color) {
    gs::TextStyle st{scale, color, 2, 0, 1};
    return gs::uploadMipped(vdp, gs::textBitmap(text, st));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 12), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_CRANE,
           {0, gs::rgb4(15, 12, 1), gs::rgb4(12, 8, 1), gs::rgb4(2, 2, 2), gs::rgb4(1, 1, 1), gs::rgb4(8, 12, 14),
            gs::rgb4(4, 4, 5), gs::rgb4(9, 9, 8), gs::rgb4(14, 14, 12), gs::rgb4(6, 5, 4), 0, 0, 0, 0, 0,
            gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_YARD,
           {0, gs::rgb4(10, 6, 3), gs::rgb4(7, 7, 8), gs::rgb4(5, 6, 7), gs::rgb4(3, 3, 3), gs::rgb4(13, 10, 4)});
    setPal(vdp, PAL_CONE, {0, gs::rgb4(15, 7, 1), gs::rgb4(15, 15, 12), gs::rgb4(4, 4, 4)});
    setPal(vdp, PAL_LIGHT, {0, gs::rgb4(14, 14, 10), gs::rgb4(5, 5, 6), gs::rgb4(15, 15, 8)});
    setPal(vdp, PAL_GATE, {0, gs::rgb4(13, 11, 3), gs::rgb4(4, 5, 6), gs::rgb4(15, 8, 1), gs::rgb4(2, 3, 4)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 3, 2), gs::rgb4(3, 1, 1)});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(7, 15, 7), gs::rgb4(1, 4, 1)});
    setPal(vdp, PAL_ROAD,
           {0, gs::rgb4(6, 6, 6), gs::rgb4(4, 4, 4), gs::rgb4(8, 8, 7), gs::rgb4(3, 3, 3), gs::rgb4(2, 2, 2),
            gs::rgb4(3, 3, 2), gs::rgb4(4, 4, 3), gs::rgb4(5, 5, 5), gs::rgb4(7, 6, 4), gs::rgb4(4, 3, 2),
            gs::rgb4(2, 3, 4), gs::rgb4(3, 4, 5), gs::rgb4(5, 6, 7), gs::rgb4(14, 11, 2), gs::rgb4(5, 5, 4)});

    art.crane = gs::uploadMipped(vdp, paintCrane());
    art.hook = gs::uploadMipped(vdp, paintHook());
    art.shadow = gs::uploadMipped(vdp, paintShadow());
    art.stack = gs::uploadMipped(vdp, paintStack());
    art.barrier = gs::uploadMipped(vdp, paintBarrier());
    art.cone = gs::uploadMipped(vdp, paintCone());
    art.flood = gs::uploadMipped(vdp, paintFlood());
    art.gate = gs::uploadMipped(vdp, paintGate());
    art.title = words(vdp, "CRANE LANE", 3, 1);
    art.stay = words(vdp, "KEEP THE HOOK IN", 1, 1);
    art.held = words(vdp, "LANE HELD", 2, 1);
    art.left = words(vdp, "LEFT THE LANE", 2, 1);
    art.start = words(vdp, "START", 2, 1);
    loadFont(vdp, art);
    vdp.setFogColor(gs::rgb4(5, 6, 7));
}

}  // namespace cranelane

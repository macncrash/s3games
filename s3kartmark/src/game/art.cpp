#include "game/art.h"

#include <initializer_list>

namespace kartmark {
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

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const auto C = gs::rgb4;
    setPal(vdp, PAL_HUD, {C(0, 0, 0), C(15, 15, 13), C(12, 11, 6), C(6, 14, 8), C(15, 6, 4), C(8, 13, 15), C(4, 5, 6), C(0, 0, 0),
                          C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(2, 2, 3)});
    setPal(vdp, PAL_KART, {C(0, 0, 0), C(14, 3, 2), C(8, 1, 1), C(15, 12, 4), C(3, 3, 4), C(15, 15, 14), C(10, 10, 11), C(6, 2, 2)});
    setPal(vdp, PAL_DRIVER, {C(0, 0, 0), C(15, 10, 6), C(3, 4, 8), C(12, 13, 15), C(2, 2, 3), C(14, 4, 3), C(8, 8, 9)});
    setPal(vdp, PAL_ASPHALT, {C(0, 0, 0), C(5, 5, 6), C(8, 8, 9), C(3, 3, 4), C(12, 12, 11), C(2, 2, 3)});
    setPal(vdp, PAL_MARK, {C(0, 0, 0), C(15, 13, 2), C(15, 15, 12), C(9, 7, 1), C(14, 3, 2), C(15, 15, 15)});
    setPal(vdp, PAL_CONE, {C(0, 0, 0), C(15, 8, 1), C(15, 15, 13), C(10, 4, 1), C(3, 3, 3)});
    setPal(vdp, PAL_STAND, {C(0, 0, 0), C(7, 8, 10), C(4, 5, 7), C(12, 4, 3), C(3, 8, 12), C(14, 13, 10), C(2, 2, 3)});
    setPal(vdp, PAL_WHEEL, {C(0, 0, 0), C(2, 2, 2), C(9, 9, 10), C(14, 12, 3), C(5, 5, 6)});
    setPal(vdp, PAL_BANNER, {C(0, 0, 0), C(15, 14, 8), C(12, 2, 2), C(2, 2, 4), C(15, 15, 15), C(8, 1, 1)});
    vdp.setFogColor(C(8, 10, 13));

    loadFont(vdp, art);

    {
        gs::Bitmap b(96, 28);
        b.rect(10, 10, 72, 10, 1);
        b.rect(18, 6, 40, 6, 7);
        b.poly({{8, 14}, {4, 18}, {14, 18}}, 3);
        b.rect(70, 8, 14, 6, 6);
        b.rect(22, 4, 8, 3, 5);
        b.line(8, 20, 86, 20, 4, 1.4f);
        b.rect(4, 16, 6, 6, 2);
        b.rect(84, 16, 6, 6, 2);
        art.kart = gs::uploadMipped(vdp, b);
    }
    for (int f = 0; f < 2; f++) {
        gs::Bitmap b(18, 18);
        b.ellipse(9, 9, 8, 8, 1);
        b.ellipse(9, 9, 3.2f, 3.2f, 2);
        if (f == 0) {
            b.line(9, 2, 9, 16, 3, 1.2f);
            b.line(2, 9, 16, 9, 4, 1.1f);
        } else {
            b.line(3, 3, 15, 15, 3, 1.2f);
            b.line(15, 3, 3, 15, 4, 1.1f);
        }
        art.wheel[f] = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(22, 28);
        b.ellipse(11, 7, 6, 6, 2);
        b.rect(8, 5, 6, 3, 3);
        b.rect(7, 12, 9, 9, 5);
        b.line(8, 20, 5, 27, 4, 1.6f);
        b.line(14, 20, 17, 27, 4, 1.6f);
        b.ellipse(9, 7, 1.2f, 1.2f, 6);
        b.ellipse(14, 7, 1.2f, 1.2f, 6);
        art.driver = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(72, 20);
        b.rect(2, 4, 68, 12, 1);
        b.rect(2, 4, 68, 3, 2);
        b.rect(2, 13, 68, 3, 2);
        b.line(8, 2, 64, 18, 4, 2.0f);
        b.line(64, 2, 8, 18, 4, 2.0f);
        b.rect(32, 1, 8, 3, 5);
        art.paint = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(16, 28);
        b.poly({{8, 1}, {15, 26}, {1, 26}}, 1);
        b.rect(2, 10, 12, 3, 2);
        b.rect(3, 18, 10, 3, 2);
        b.rect(6, 26, 4, 2, 4);
        art.cone = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(70, 36);
        b.rect(2, 10, 66, 22, 1);
        b.rect(2, 10, 66, 4, 2);
        for (int i = 0; i < 5; i++) b.rect(6 + i * 12, 16, 8, 10, (i & 1) ? 3 : 4);
        b.rect(0, 30, 70, 4, 6);
        b.line(4, 6, 66, 6, 5, 1.5f);
        art.stand = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(8, 40);
        b.rect(3, 8, 2, 30, 2);
        b.ellipse(4, 5, 3, 3, 1);
        art.lamp = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(120, 28);
        b.rect(0, 0, 120, 28, 3);
        b.rect(3, 3, 114, 22, 2);
        gs::TextStyle st;
        st.scale = 2;
        st.color = 1;
        st.outline = 4;
        st.spacing = 1;
        gs::Bitmap word = gs::textBitmap("KART MARK", st);
        b.blit(word, 8, 5);
        art.banner = gs::uploadMipped(vdp, b);
    }
}

}  // namespace kartmark

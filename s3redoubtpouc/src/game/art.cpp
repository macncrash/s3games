#include "game/art.h"

namespace pouc {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

gs::Bitmap label(const char* s, int color, int scale) {
    gs::TextStyle st;
    st.scale = scale;
    st.color = color;
    st.outline = 15;
    st.spacing = 1;
    return gs::textBitmap(s, st);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    using gs::rgb4;
    setPal(vdp, PAL_HUD, {rgb4(0, 0, 0), rgb4(15, 14, 10), rgb4(8, 7, 5), rgb4(15, 12, 4), rgb4(4, 3, 2)});
    setPal(vdp, PAL_STONE,
           {rgb4(0, 0, 0), rgb4(6, 6, 7), rgb4(9, 9, 10), rgb4(4, 4, 5), rgb4(11, 10, 8), rgb4(3, 3, 4),
            rgb4(7, 6, 5)});
    setPal(vdp, PAL_RUNNER,
           {rgb4(0, 0, 0), rgb4(3, 5, 10), rgb4(5, 8, 14), rgb4(12, 8, 5), rgb4(15, 12, 9), rgb4(2, 2, 3),
            rgb4(14, 11, 4), rgb4(8, 2, 2)});
    setPal(vdp, PAL_SENTRY,
           {rgb4(0, 0, 0), rgb4(10, 2, 2), rgb4(14, 4, 3), rgb4(6, 5, 4), rgb4(12, 11, 8), rgb4(3, 3, 4),
            rgb4(15, 14, 10)});
    setPal(vdp, PAL_IRON,
           {rgb4(0, 0, 0), rgb4(5, 6, 7), rgb4(9, 10, 11), rgb4(3, 3, 4), rgb4(12, 9, 4), rgb4(2, 2, 3)});
    setPal(vdp, PAL_WATER,
           {rgb4(0, 0, 0), rgb4(2, 5, 8), rgb4(3, 8, 11), rgb4(6, 12, 14), rgb4(1, 3, 5), rgb4(8, 10, 8)});
    setPal(vdp, PAL_TORCH, {rgb4(0, 0, 0), rgb4(15, 10, 2), rgb4(15, 14, 5), rgb4(10, 4, 1), rgb4(15, 15, 12)});
    setPal(vdp, PAL_POUCH, {rgb4(0, 0, 0), rgb4(10, 6, 2), rgb4(14, 10, 3), rgb4(15, 13, 6), rgb4(6, 3, 1), rgb4(3, 2, 1)});

    vdp.setFogColor(rgb4(2, 2, 4));

    {
        gs::Bitmap b(24, 36);
        b.rect(8, 2, 8, 8, 4);
        b.rect(9, 4, 6, 3, 5);
        b.rect(6, 10, 12, 12, 1);
        b.rect(7, 11, 10, 6, 2);
        b.rect(4, 12, 3, 8, 1);
        b.rect(17, 12, 3, 8, 1);
        b.rect(8, 22, 4, 12, 3);
        b.rect(13, 22, 4, 12, 3);
        b.rect(7, 32, 5, 3, 5);
        b.rect(13, 32, 5, 3, 5);
        b.rect(15, 16, 7, 8, 6);
        b.rect(16, 17, 5, 5, 7);
        b.outline(5, false);
        art.runner = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(22, 34);
        b.rect(7, 1, 8, 8, 4);
        b.rect(6, 0, 10, 3, 5);
        b.rect(5, 9, 12, 13, 1);
        b.rect(6, 10, 10, 7, 2);
        b.rect(16, 8, 3, 14, 3);
        b.rect(18, 4, 2, 12, 6);
        b.rect(7, 22, 4, 10, 3);
        b.rect(12, 22, 4, 10, 3);
        b.rect(6, 31, 5, 2, 5);
        b.rect(12, 31, 5, 2, 5);
        b.outline(5, false);
        art.sentry = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(16, 14);
        b.ellipse(8, 8, 7, 5, 1);
        b.ellipse(8, 7, 5, 3, 2);
        b.rect(4, 2, 8, 3, 4);
        b.rect(6, 1, 4, 2, 3);
        b.line(3, 4, 13, 4, 5, 1);
        art.pouch = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(16, 48);
        for (int x = 1; x < 16; x += 4) b.rect(float(x), 0, 2, 48, 1);
        for (int y = 4; y < 48; y += 8) b.rect(0, float(y), 16, 2, 2);
        b.rect(0, 0, 16, 3, 3);
        art.bar = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(28, 64);
        b.rect(2, 8, 6, 56, 1);
        b.rect(20, 8, 6, 56, 1);
        b.rect(2, 4, 24, 8, 2);
        b.rect(6, 0, 16, 6, 4);
        b.poly({{8, 20}, {20, 20}, {22, 58}, {6, 58}}, 3);
        b.rect(12, 36, 4, 8, 4);
        art.gate = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(32, 24);
        b.rect(0, 0, 32, 24, 1);
        b.rect(1, 1, 14, 10, 2);
        b.rect(17, 1, 14, 10, 2);
        b.rect(1, 13, 14, 10, 3);
        b.rect(17, 13, 14, 10, 6);
        b.rect(0, 11, 32, 2, 5);
        art.block = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(16, 14);
        b.rect(1, 4, 14, 10, 2);
        b.poly({{1, 4}, {8, 0}, {15, 4}}, 4);
        b.rect(6, 8, 4, 6, 5);
        art.merlon = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(32, 16);
        b.rect(0, 0, 32, 16, 4);
        b.rect(0, 0, 32, 3, 6);
        for (int x = 0; x < 32; x += 8) b.line(float(x), 3, float(x), 16, 3, 1);
        art.plank = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(32, 20);
        b.rect(0, 0, 32, 20, 1);
        b.ellipse(8, 8, 6, 3, 2);
        b.ellipse(22, 12, 7, 3, 3);
        b.rect(0, 16, 32, 4, 4);
        art.water = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(10, 16);
        b.rect(4, 8, 2, 8, 3);
        b.ellipse(5, 6, 3, 5, 1);
        b.ellipse(5, 5, 2, 3, 2);
        art.torch = gs::uploadImage(vdp, b);
    }
    art.title = gs::uploadImage(vdp, label("REDOUBT", 1, 3));
    art.hint = gs::uploadImage(vdp, label("CARRY THE POUCH ACROSS", 1, 1));
    art.win = gs::uploadImage(vdp, label("POUCH ACROSS", 3, 2));
    art.lose = gs::uploadImage(vdp, label("POUCH LOST", 1, 2));
    art.mark = gs::uploadImage(vdp, label("A JUMP", 2, 1));
}

}  // namespace pouc

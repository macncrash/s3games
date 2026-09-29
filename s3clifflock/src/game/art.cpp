#include "art.h"

#include <cstdint>
#include <string>

namespace clifflock {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, ink);
}

uint32_t hash2(int x, int y) {
    uint32_t h = uint32_t(x) * 374761393u + uint32_t(y) * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

Bitmap truckArt() {
    Bitmap b(72, 36);
    b.rect(6, 14, 48, 14, 1);
    b.rect(40, 6, 22, 22, 1);
    b.rect(44, 9, 14, 8, 4);
    b.rect(8, 16, 28, 4, 2);
    b.rect(4, 22, 8, 6, 3);
    b.ellipse(18, 28, 7, 7, 5);
    b.ellipse(18, 28, 3, 3, 6);
    b.ellipse(50, 28, 7, 7, 5);
    b.ellipse(50, 28, 3, 3, 6);
    b.rect(62, 16, 6, 4, 7);
    b.rect(10, 12, 10, 3, 3);
    return b;
}

Bitmap slabArt() {
    Bitmap b(32, 14);
    for (int y = 0; y < 14; y++) {
        for (int x = 0; x < 32; x++) {
            int c = 1;
            if (y < 3) c = 2;
            else if (y > 10) c = 3;
            else if ((hash2(x, y) & 7) == 0) c = 4;
            b.set(x, y, c);
        }
    }
    b.rect(0, 0, 32, 1, 5);
    return b;
}

Bitmap cliffArt() {
    Bitmap b(32, 48);
    for (int y = 0; y < 48; y++) {
        for (int x = 0; x < 32; x++) {
            uint32_t h = hash2(x, y);
            int c = 1 + int(h % 3u);
            if ((h & 31u) == 0) c = 4;
            if (y < 3) c = 5;
            b.set(x, y, c);
        }
    }
    return b;
}

Bitmap leafArt() {
    Bitmap b(18, 72);
    b.rect(2, 0, 14, 72, 1);
    for (int y = 4; y < 68; y += 10) b.rect(4, y, 10, 2, 2);
    b.rect(7, 8, 4, 56, 3);
    b.rect(0, 0, 18, 3, 4);
    return b;
}

Bitmap postArt() {
    Bitmap b(16, 80);
    b.rect(2, 0, 12, 80, 1);
    b.rect(4, 0, 3, 80, 2);
    for (int y = 6; y < 76; y += 12) b.rect(2, y, 12, 2, 3);
    b.rect(0, 0, 16, 6, 4);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_ROCK, {0, gs::rgb4(6, 5, 4), gs::rgb4(9, 8, 6), gs::rgb4(4, 3, 3), gs::rgb4(8, 7, 5),
                           gs::rgb4(12, 11, 8)});
    setPal(vdp, PAL_TRUCK, {0, gs::rgb4(12, 6, 2), gs::rgb4(14, 10, 3), gs::rgb4(4, 3, 3), gs::rgb4(8, 12, 14),
                            gs::rgb4(2, 2, 2), gs::rgb4(10, 10, 9), gs::rgb4(15, 14, 6)});
    setPal(vdp, PAL_GATE, {0, gs::rgb4(5, 6, 7), gs::rgb4(9, 10, 11), gs::rgb4(3, 4, 5), gs::rgb4(13, 12, 8)});
    setPal(vdp, PAL_SEA, {0, gs::rgb4(2, 5, 8), gs::rgb4(4, 8, 11), gs::rgb4(8, 12, 13)});
    setPal(vdp, PAL_CREW, {0, gs::rgb4(10, 2, 2), gs::rgb4(14, 6, 3), gs::rgb4(3, 2, 2), gs::rgb4(6, 8, 10),
                           gs::rgb4(1, 1, 1), gs::rgb4(6, 6, 6), gs::rgb4(12, 10, 4)});
    textPal(vdp, PAL_HUD, gs::rgb4(15, 14, 12));
    textPal(vdp, PAL_INK, gs::rgb4(14, 12, 6));
    textPal(vdp, PAL_WARN, gs::rgb4(15, 6, 3));

    art.truck = gs::uploadMipped(vdp, truckArt());
    art.slab = gs::uploadMipped(vdp, slabArt());
    art.cliff = gs::uploadMipped(vdp, cliffArt());
    art.leaf = gs::uploadMipped(vdp, leafArt());
    art.post = gs::uploadMipped(vdp, postArt());

    gs::TextStyle st;
    st.scale = 1;
    st.color = 1;
    st.spacing = 1;
    for (int i = 0; i < 96; i++) {
        std::string s(1, char(32 + i));
        art.glyph[i] = gs::uploadMipped(vdp, gs::textBitmap(s, st));
    }
    vdp.setFogColor(gs::rgb4(6, 7, 9));
}

}  // namespace clifflock

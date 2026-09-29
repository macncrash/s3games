#include "game/art.h"

namespace palisadepurs {
namespace {

void pal(gs::VDP& v, int p, const uint16_t* c, int n) {
    for (int i = 0; i < n; i++) v.setColor(p * 16 + 1 + i, c[i]);
}

gs::Bitmap glyphBmp(char ch) {
    gs::Bitmap b(5, 7);
    const uint8_t* g = gs::glyph(ch);
    for (int y = 0; y < 7; y++)
        for (int x = 0; x < 5; x++)
            if (g[y * 5 + x]) b.set(x, y, 1);
    return b;
}

gs::Bitmap stakeBmp() {
    gs::Bitmap b(16, 48);
    b.poly({{8, 0}, {3, 12}, {13, 12}}, 2);
    b.rect(4, 10, 8, 32, 1);
    b.rect(6, 12, 3, 28, 3);
    for (int y = 16; y < 40; y += 8) b.rect(4, float(y), 8, 2, 4);
    b.rect(2, 40, 12, 6, 5);
    b.outline(6, false);
    return b;
}

gs::Bitmap youBmp() {
    gs::Bitmap b(40, 22);
    b.rect(2, 8, 24, 10, 2);
    b.rect(22, 10, 14, 6, 3);
    b.rect(6, 4, 10, 6, 4);
    b.rect(8, 5, 5, 3, 5);
    b.ellipse(8, 18, 4, 4, 6);
    b.ellipse(20, 18, 4, 4, 6);
    b.ellipse(32, 16, 3, 3, 6);
    b.rect(34, 11, 4, 2, 7);
    b.outline(8, false);
    return b;
}

gs::Bitmap ramBmp() {
    gs::Bitmap b(44, 20);
    b.poly({{2, 10}, {12, 4}, {12, 16}}, 3);
    b.rect(10, 5, 26, 10, 2);
    b.rect(14, 2, 12, 4, 4);
    b.rect(16, 7, 6, 4, 5);
    b.ellipse(16, 16, 4, 4, 6);
    b.ellipse(30, 16, 4, 4, 6);
    b.rect(32, 6, 8, 3, 7);
    b.outline(8, false);
    return b;
}

gs::Bitmap cartBmp() {
    gs::Bitmap b(36, 18);
    b.rect(8, 4, 22, 8, 2);
    b.rect(4, 6, 6, 5, 3);
    b.rect(12, 1, 8, 4, 4);
    b.ellipse(12, 14, 4, 4, 6);
    b.ellipse(26, 14, 4, 4, 6);
    b.rect(26, 6, 6, 3, 5);
    b.outline(8, false);
    return b;
}

gs::Bitmap boltBmp() {
    gs::Bitmap b(10, 4);
    b.rect(0, 1, 8, 2, 1);
    b.rect(6, 0, 4, 4, 2);
    return b;
}

gs::Bitmap shotBmp() {
    gs::Bitmap b(8, 4);
    b.rect(2, 1, 6, 2, 3);
    b.rect(0, 1, 3, 2, 4);
    return b;
}

gs::Bitmap sparkBmp() {
    gs::Bitmap b(6, 6);
    b.rect(2, 0, 2, 6, 1);
    b.rect(0, 2, 6, 2, 2);
    return b;
}

gs::Bitmap lampBmp() {
    gs::Bitmap b(6, 6);
    b.ellipse(3, 3, 3, 3, 1);
    b.rect(2, 2, 2, 2, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    static const uint16_t hud[] = {gs::rgb4(15, 14, 10), gs::rgb4(12, 9, 4), gs::rgb4(8, 7, 5), gs::rgb4(15, 6, 2)};
    static const uint16_t stake[] = {gs::rgb4(6, 4, 2), gs::rgb4(9, 7, 3), gs::rgb4(12, 9, 4), gs::rgb4(4, 3, 1),
                                      gs::rgb4(3, 3, 2), gs::rgb4(1, 1, 0)};
    static const uint16_t you[] = {gs::rgb4(2, 3, 2), gs::rgb4(4, 7, 4), gs::rgb4(7, 10, 6), gs::rgb4(5, 6, 4),
                                    gs::rgb4(14, 13, 6), gs::rgb4(2, 2, 2), gs::rgb4(15, 8, 2), gs::rgb4(0, 0, 0)};
    static const uint16_t ram[] = {gs::rgb4(3, 2, 2), gs::rgb4(8, 3, 2), gs::rgb4(11, 5, 3), gs::rgb4(6, 5, 4),
                                    gs::rgb4(14, 10, 4), gs::rgb4(2, 2, 2), gs::rgb4(15, 12, 5), gs::rgb4(0, 0, 0)};
    static const uint16_t cart[] = {gs::rgb4(2, 2, 3), gs::rgb4(5, 5, 7), gs::rgb4(8, 8, 10), gs::rgb4(4, 4, 5),
                                     gs::rgb4(12, 8, 3), gs::rgb4(1, 1, 1), gs::rgb4(0, 0, 0), gs::rgb4(0, 0, 0)};
    static const uint16_t bolt[] = {gs::rgb4(15, 14, 6), gs::rgb4(15, 8, 2), gs::rgb4(10, 12, 14), gs::rgb4(6, 4, 2)};
    static const uint16_t fx[] = {gs::rgb4(15, 15, 12), gs::rgb4(14, 8, 3), gs::rgb4(8, 8, 7), gs::rgb4(4, 4, 3)};
    static const uint16_t field[] = {gs::rgb4(3, 5, 2), gs::rgb4(5, 7, 3), gs::rgb4(8, 7, 3), gs::rgb4(2, 3, 1)};
    static const uint16_t wreck[] = {gs::rgb4(2, 2, 2), gs::rgb4(4, 4, 3), gs::rgb4(6, 5, 4), gs::rgb4(3, 3, 3),
                                      gs::rgb4(5, 4, 2), gs::rgb4(1, 1, 1), gs::rgb4(3, 3, 3), gs::rgb4(0, 0, 0)};
    pal(vdp, PAL_HUD, hud, 4);
    pal(vdp, PAL_STAKE, stake, 6);
    pal(vdp, PAL_YOU, you, 8);
    pal(vdp, PAL_RAM, ram, 8);
    pal(vdp, PAL_CART, cart, 7);
    pal(vdp, PAL_BOLT, bolt, 4);
    pal(vdp, PAL_FX, fx, 4);
    pal(vdp, PAL_FIELD, field, 4);
    pal(vdp, PAL_WRECK, wreck, 8);

    art.stake = gs::uploadMipped(vdp, stakeBmp());
    art.you = gs::uploadMipped(vdp, youBmp());
    art.ram = gs::uploadMipped(vdp, ramBmp());
    art.cart = gs::uploadMipped(vdp, cartBmp());
    art.bolt = gs::uploadMipped(vdp, boltBmp());
    art.shot = gs::uploadMipped(vdp, shotBmp());
    art.spark = gs::uploadMipped(vdp, sparkBmp());
    art.lamp = gs::uploadMipped(vdp, lampBmp());
    for (int i = 0; i < 96; i++) art.glyph[i] = gs::uploadMipped(vdp, glyphBmp(char(32 + i)));
}

}  // namespace palisadepurs

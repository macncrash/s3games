#include "game/art.h"

#include <initializer_list>
#include <string>

namespace depotwell {
namespace {

void pal(gs::VDP& v, int p, std::initializer_list<uint16_t> cs) {
    int i = 1;
    v.setColor(p * 16, 0);
    for (uint16_t c : cs) {
        if (i < 15) v.setColor(p * 16 + i, c);
        ++i;
    }
    while (i < 15) v.setColor(p * 16 + i++, 0);
    v.setColor(p * 16 + 15, gs::rgb4(1, 1, 2));
}

void textPal(gs::VDP& v, int p, uint16_t ink, uint16_t shade) {
    for (int i = 0; i < 16; ++i) v.setColor(p * 16 + i, 0);
    v.setColor(p * 16 + 1, ink);
    v.setColor(p * 16 + 15, shade);
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
    gs::TextStyle big{3, 1, 0, 15, 1};
    for (int c = 32; c < 128; ++c) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; ++y)
            for (int x = 0; x < 5; ++x)
                if (g[y * 5 + x]) {
                    px[y * 8 + x + 1] = 1;
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
                }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

void box(gs::Bitmap& b, int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); }

gs::Bitmap yardArt() {
    gs::Bitmap b(320, 224);
    for (int x = 16; x < 300; x += 64) {
        box(b, x, 8, 2, 28, 15);
        box(b, x, 14, 48, 1, 15);
    }
    box(b, 0, 34, 176, 10, 12);
    box(b, 0, 40, 168, 6, 7);
    for (int y = 48; y < 82; y += 5)
        for (int x = 2; x < 166; x += 12) box(b, x, y, 10, 3, ((x + y) / 5) & 1 ? 6 : 7);
    box(b, 8, 52, 28, 28, 9);
    box(b, 44, 52, 28, 28, 9);
    box(b, 80, 52, 28, 28, 9);
    box(b, 116, 54, 22, 16, 10);
    box(b, 14, 58, 6, 16, 7);
    box(b, 50, 58, 6, 16, 7);
    box(b, 86, 58, 6, 16, 7);
    box(b, 0, 80, 172, 8, 8);
    box(b, 0, 86, 172, 3, 3);
    for (int y = 92; y < 214; ++y)
        for (int x = 0; x < 320; ++x)
            if (((x * 13 + y * 7) % 19) == 0) b.set(x, y, 3);
            else if (((x * 3 + y) % 11) == 0) b.set(x, y, 1);
            else b.set(x, y, 2);
    for (int i = 0; i < 3; ++i) {
        int y = int(laneFoot(i));
        box(b, 0, y - 14, 250, 22, 3);
        for (int x = 6; x < 246; x += 12) box(b, x, y - 3, 8, 5, 5);
        box(b, 0, y - 6, 250, 2, 4);
        box(b, 0, y + 2, 250, 2, 4);
        box(b, int(kMarkX) - 7, y - 4, 14, 8, 10);
        box(b, int(kMarkX) - 1, y - 18, 2, 14, 10);
    }
    for (int y = 96; y < 206; y += 10)
        for (int x = 252; x < 318; x += 12) box(b, x, y, 10, 8, ((x + y) / 10) & 1 ? 8 : 3);
    box(b, 248, 96, 4, 112, 15);
    for (int i = 0; i < 5; ++i) box(b, 188, 96 + i * 8, 10, 6, i & 1 ? 3 : 12);
    box(b, 186, 48, 3, 58, 15);
    box(b, 186, 52, 22, 2, 15);
    box(b, 204, 48, 4, 4, 13);
    box(b, 204, 56, 4, 4, 14);
    for (int x = 20; x < 150; x += 28) box(b, x, 90, 2, 3, 11);
    return b;
}

gs::Bitmap wellArt() {
    gs::Bitmap b(70, 132);
    b.poly({{10.f, 30.f}, {34.f, 6.f}, {58.f, 30.f}}, 8);
    box(b, 8, 26, 54, 6, 9);
    box(b, 12, 28, 46, 3, 8);
    box(b, 12, 32, 5, 48, 8);
    box(b, 14, 32, 2, 48, 9);
    box(b, 53, 32, 5, 48, 8);
    box(b, 54, 32, 2, 48, 9);
    box(b, 18, 36, 34, 5, 10);
    box(b, 33, 40, 2, 26, 11);
    box(b, 2, 46, 24, 3, 10);
    box(b, 2, 42, 3, 8, 10);
    box(b, 20, 48, 3, 18, 11);
    b.ellipse(35, 82, 24, 13, 2);
    b.ellipse(35, 82, 20, 10, 1);
    b.ellipse(35, 84, 12, 7, 5);
    b.ellipse(35, 85, 8, 5, 6);
    b.ellipse(31, 82, 3, 2, 7);
    for (int i = 0; i < 4; ++i) box(b, 14, 96 + i * 6, 42, 5, i & 1 ? 3 : 2);
    box(b, 8, 118, 54, 8, 3);
    box(b, 4, 124, 62, 5, 4);
    box(b, 18, 120, 8, 3, 12);
    b.outline(15, false);
    return b;
}

gs::Bitmap rubbleArt() {
    gs::Bitmap b(78, 40);
    b.ellipse(40, 26, 32, 10, 3);
    b.ellipse(22, 24, 14, 7, 2);
    b.ellipse(56, 22, 16, 8, 2);
    box(b, 14, 14, 16, 8, 8);
    box(b, 40, 10, 8, 16, 10);
    box(b, 28, 16, 12, 4, 9);
    b.ellipse(20, 18, 5, 3, 5);
    b.outline(15, false);
    return b;
}

gs::Bitmap crackArt() {
    gs::Bitmap b(28, 40);
    b.line(8, 2, 14, 16, 14, 1.2f);
    b.line(14, 16, 8, 36, 14, 1.2f);
    b.line(14, 16, 24, 24, 11, 1.1f);
    b.line(10, 24, 18, 30, 3, 1.f);
    return b;
}

gs::Bitmap bucketArt() {
    gs::Bitmap b(14, 12);
    box(b, 3, 3, 8, 7, 10);
    box(b, 4, 4, 6, 3, 3);
    box(b, 2, 2, 10, 2, 11);
    b.line(3, 2, 7, 0, 11, 1.f);
    b.line(11, 2, 7, 0, 11, 1.f);
    b.outline(15, false);
    return b;
}

gs::Bitmap shunterArt(int frame) {
    gs::Bitmap b(40, 30);
    int bob = frame ? 1 : 0;
    box(b, 2, 16, 6, 4, 11);
    box(b, 6, 14, 4, 7, 7);
    box(b, 10, 12, 16, 10, 1);
    box(b, 10, 12, 16, 3, 2);
    box(b, 24, 5, 13, 17, 1);
    box(b, 26, 7, 8, 6, 8);
    box(b, 27, 8, 3, 3, 6);
    box(b, 8, 10, 4, 4, 6);
    box(b, 9, 11, 2, 2, 12);
    box(b, 8 + bob, 21, 6, 6, 5);
    box(b, 18 - bob, 21, 6, 6, 5);
    box(b, 29, 21, 6, 6, 5);
    box(b, 9, 23, 2, 2, 4);
    box(b, 20, 23, 2, 2, 4);
    box(b, 31, 23, 2, 2, 4);
    b.outline(15, false);
    return b;
}

gs::Bitmap boxArt(int frame) {
    gs::Bitmap b(40, 26);
    int bob = frame ? 1 : 0;
    box(b, 3, 3, 30, 14, 1);
    box(b, 3, 3, 30, 3, 2);
    box(b, 6, 6, 8, 8, 6);
    box(b, 18, 6, 10, 8, 4);
    box(b, 20, 8, 6, 4, 7);
    box(b, 32, 10, 5, 4, 4);
    box(b, 2, 9, 2, 2, 7);
    box(b, 6 + bob, 17, 6, 6, 5);
    box(b, 22 - bob, 17, 6, 6, 5);
    b.outline(15, false);
    return b;
}

gs::Bitmap flatArt(int frame) {
    gs::Bitmap b(40, 24);
    int bob = frame ? 1 : 0;
    box(b, 2, 12, 34, 4, 1);
    box(b, 2, 12, 34, 2, 2);
    b.ellipse(12, 10, 5, 5, 3);
    b.ellipse(12, 10, 3, 3, 4);
    b.ellipse(24, 9, 5, 5, 3);
    b.ellipse(24, 9, 3, 3, 4);
    box(b, 32, 10, 5, 4, 6);
    box(b, 6 + bob, 15, 6, 6, 5);
    box(b, 22 - bob, 15, 6, 6, 5);
    b.outline(15, false);
    return b;
}

gs::Bitmap tankArt(int frame) {
    gs::Bitmap b(46, 24);
    int bob = frame ? 1 : 0;
    box(b, 6, 6, 32, 10, 1);
    box(b, 8, 7, 28, 3, 2);
    box(b, 16, 6, 8, 10, 6);
    box(b, 18, 3, 5, 4, 7);
    box(b, 19, 4, 3, 2, 5);
    box(b, 36, 8, 6, 6, 3);
    box(b, 2, 9, 5, 4, 4);
    box(b, 8 + bob, 15, 6, 6, 4);
    box(b, 20, 15, 6, 6, 4);
    box(b, 30 - bob, 15, 6, 6, 4);
    b.outline(15, false);
    return b;
}

gs::Bitmap bufferArt() {
    gs::Bitmap b(14, 16);
    box(b, 6, 2, 3, 12, 6);
    box(b, 2, 4, 10, 4, 1);
    box(b, 3, 5, 8, 2, 7);
    box(b, 4, 12, 6, 3, 3);
    b.outline(15, false);
    return b;
}

gs::Bitmap puffArt() {
    gs::Bitmap b(14, 14);
    b.ellipse(7, 7, 5, 4, 1);
    b.ellipse(5, 6, 2, 2, 3);
    b.ellipse(9, 8, 2, 2, 2);
    return b;
}

gs::Bitmap sparkArt() {
    gs::Bitmap b(12, 12);
    b.line(6, 1, 6, 10, 2, 1.2f);
    b.line(1, 6, 10, 6, 2, 1.2f);
    b.line(3, 3, 9, 9, 3, 1.f);
    b.line(9, 3, 3, 9, 3, 1.f);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(16, 6);
    b.ellipse(8, 3, 7, 2, 1);
    return b;
}

gs::Bitmap moonArt() {
    gs::Bitmap b(18, 18);
    b.ellipse(9, 9, 7, 7, 1);
    b.ellipse(12, 8, 5, 5, 2);
    b.outline(15, false);
    return b;
}

gs::Bitmap starArt() {
    gs::Bitmap b(7, 7);
    box(b, 3, 0, 1, 7, 3);
    box(b, 0, 3, 7, 1, 3);
    box(b, 2, 2, 3, 3, 1);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(8, 8);
    b.ellipse(4, 4, 3, 3, 1);
    b.ellipse(4, 4, 1, 1, 2);
    return b;
}

gs::Bitmap chainArt() {
    gs::Bitmap b(6, 8);
    box(b, 1, 1, 4, 6, 4);
    box(b, 2, 2, 2, 4, 0);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& a) {
    textPal(vdp, PAL_TEXT, gs::rgb4(13, 12, 9), gs::rgb4(2, 2, 3));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 12, 4), gs::rgb4(4, 2, 1));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 5, 3), gs::rgb4(3, 1, 1));
    textPal(vdp, PAL_GOOD, gs::rgb4(8, 14, 6), gs::rgb4(1, 3, 1));
    pal(vdp, PAL_YARD,
        {gs::rgb4(10, 9, 7), gs::rgb4(7, 6, 5), gs::rgb4(4, 4, 4), gs::rgb4(12, 12, 13), gs::rgb4(6, 4, 2),
         gs::rgb4(9, 4, 3), gs::rgb4(5, 2, 2), gs::rgb4(8, 8, 7), gs::rgb4(2, 2, 3), gs::rgb4(14, 11, 3),
         gs::rgb4(3, 5, 2), gs::rgb4(3, 2, 3), gs::rgb4(13, 3, 2), gs::rgb4(4, 12, 5)});
    pal(vdp, PAL_STONE,
        {gs::rgb4(13, 12, 10), gs::rgb4(9, 8, 7), gs::rgb4(6, 6, 6), gs::rgb4(4, 4, 5), gs::rgb4(2, 4, 7),
         gs::rgb4(4, 8, 11), gs::rgb4(8, 12, 14), gs::rgb4(8, 5, 2), gs::rgb4(5, 3, 1), gs::rgb4(5, 5, 6),
         gs::rgb4(10, 6, 3), gs::rgb4(4, 6, 3), gs::rgb4(14, 13, 8), gs::rgb4(3, 3, 3)});
    pal(vdp, PAL_ENGINE,
        {gs::rgb4(6, 8, 4), gs::rgb4(3, 5, 2), gs::rgb4(2, 2, 3), gs::rgb4(12, 9, 4), gs::rgb4(4, 4, 5),
         gs::rgb4(14, 12, 6), gs::rgb4(12, 3, 2), gs::rgb4(6, 10, 12), gs::rgb4(12, 12, 12), gs::rgb4(8, 8, 7),
         gs::rgb4(3, 3, 2), gs::rgb4(14, 14, 10), gs::rgb4(5, 6, 3), gs::rgb4(2, 2, 2)});
    pal(vdp, PAL_BOX,
        {gs::rgb4(11, 3, 2), gs::rgb4(7, 2, 1), gs::rgb4(5, 5, 6), gs::rgb4(13, 12, 9), gs::rgb4(3, 3, 4),
         gs::rgb4(8, 8, 7), gs::rgb4(14, 12, 4), gs::rgb4(4, 2, 2), gs::rgb4(9, 5, 3), gs::rgb4(2, 2, 2),
         gs::rgb4(6, 4, 3), gs::rgb4(10, 8, 6), gs::rgb4(3, 3, 3), gs::rgb4(1, 1, 1)});
    pal(vdp, PAL_FLAT,
        {gs::rgb4(9, 6, 3), gs::rgb4(6, 4, 2), gs::rgb4(8, 5, 2), gs::rgb4(4, 4, 5), gs::rgb4(3, 3, 4),
         gs::rgb4(7, 7, 8), gs::rgb4(5, 3, 1), gs::rgb4(12, 9, 4), gs::rgb4(2, 2, 2), gs::rgb4(10, 7, 3),
         gs::rgb4(6, 5, 3), gs::rgb4(3, 3, 2), gs::rgb4(8, 6, 4), gs::rgb4(1, 1, 1)});
    pal(vdp, PAL_TANK,
        {gs::rgb4(2, 2, 3), gs::rgb4(6, 6, 7), gs::rgb4(8, 3, 2), gs::rgb4(3, 3, 4), gs::rgb4(10, 8, 4),
         gs::rgb4(14, 11, 2), gs::rgb4(12, 3, 2), gs::rgb4(4, 4, 5), gs::rgb4(8, 8, 8), gs::rgb4(5, 5, 4),
         gs::rgb4(3, 2, 2), gs::rgb4(9, 7, 3), gs::rgb4(1, 1, 2), gs::rgb4(7, 6, 5)});
    pal(vdp, PAL_FX,
        {gs::rgb4(14, 14, 12), gs::rgb4(15, 13, 6), gs::rgb4(15, 15, 14), gs::rgb4(8, 8, 8), gs::rgb4(4, 4, 5),
         gs::rgb4(11, 10, 8), gs::rgb4(6, 6, 6), gs::rgb4(2, 2, 2), gs::rgb4(0, 0, 0), gs::rgb4(0, 0, 0),
         gs::rgb4(0, 0, 0), gs::rgb4(0, 0, 0), gs::rgb4(0, 0, 0), gs::rgb4(0, 0, 0)});
    pal(vdp, PAL_SKY,
        {gs::rgb4(14, 13, 9), gs::rgb4(8, 8, 10), gs::rgb4(15, 15, 13), gs::rgb4(14, 10, 4), gs::rgb4(4, 4, 8),
         gs::rgb4(2, 2, 4), gs::rgb4(0, 0, 0), gs::rgb4(0, 0, 0), gs::rgb4(0, 0, 0), gs::rgb4(0, 0, 0),
         gs::rgb4(0, 0, 0), gs::rgb4(0, 0, 0), gs::rgb4(0, 0, 0), gs::rgb4(0, 0, 0)});

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, tiles, a);
    a.well = gs::uploadMipped(vdp, wellArt());
    a.rubble = gs::uploadMipped(vdp, rubbleArt());
    a.crack = gs::uploadMipped(vdp, crackArt());
    a.bucket = gs::uploadMipped(vdp, bucketArt());
    a.shunter[0] = gs::uploadMipped(vdp, shunterArt(0));
    a.shunter[1] = gs::uploadMipped(vdp, shunterArt(1));
    a.box[0] = gs::uploadMipped(vdp, boxArt(0));
    a.box[1] = gs::uploadMipped(vdp, boxArt(1));
    a.flat[0] = gs::uploadMipped(vdp, flatArt(0));
    a.flat[1] = gs::uploadMipped(vdp, flatArt(1));
    a.tank[0] = gs::uploadMipped(vdp, tankArt(0));
    a.tank[1] = gs::uploadMipped(vdp, tankArt(1));
    a.buffer = gs::uploadMipped(vdp, bufferArt());
    a.puff = gs::uploadMipped(vdp, puffArt());
    a.spark = gs::uploadMipped(vdp, sparkArt());
    a.shadow = gs::uploadMipped(vdp, shadowArt());
    a.moon = gs::uploadMipped(vdp, moonArt());
    a.star = gs::uploadMipped(vdp, starArt());
    a.lamp = gs::uploadMipped(vdp, lampArt());
    a.chain = gs::uploadMipped(vdp, chainArt());

    vdp.A.enabled = false;
    vdp.B.clear();
    gs::bitmapToPlane(tiles, vdp.B, 0, 0, yardArt(), PAL_YARD);
    vdp.B.enabled = true;
    vdp.hudEnabled = true;
    vdp.setFogColor(gs::rgb4(2, 1, 3));
}

}  // namespace depotwell

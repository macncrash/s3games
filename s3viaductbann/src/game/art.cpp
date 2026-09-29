#include "game/art.h"

#include <string>

namespace viaductbann {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void loadFont(gs::VDP& vdp, Art& a, gs::TileAlloc& tiles) {
    gs::TextStyle big{2, 1, 0, 15, 1};
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
        a.font[c - 32] = t;
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

gs::Bitmap runner(int pose) {
    gs::Bitmap b(28, 44);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    int bob = pose == 2 ? 1 : 0;
    R(9, 1 + bob, 10, 4, 7);   // helmet
    R(8, 5 + bob, 12, 6, 4);   // face
    R(16, 6 + bob, 2, 2, 1);
    R(6, 11 + bob, 16, 14, 2); // coat
    R(6, 11 + bob, 3, 14, 3);
    R(11, 16 + bob, 6, 3, 5);  // sash
    if (pose == 1) {
        R(7, 25, 5, 12, 2);
        R(16, 27, 5, 10, 2);
        R(6, 36, 7, 3, 6);
        R(15, 36, 7, 3, 6);
        R(20, 13, 4, 8, 3);
    } else if (pose == 2) {
        R(6, 27, 5, 10, 2);
        R(16, 25, 5, 12, 2);
        R(5, 36, 7, 3, 6);
        R(15, 36, 7, 3, 6);
        R(20, 14, 4, 8, 3);
    } else if (pose == 3) {
        R(9, 22, 4, 8, 2);
        R(15, 20, 5, 7, 2);
        R(8, 30, 5, 3, 6);
        R(16, 27, 6, 3, 6);
        R(18, 11, 6, 3, 3);
    } else {
        R(9, 25, 4, 12, 2);
        R(15, 25, 4, 12, 2);
        R(8, 36, 6, 3, 6);
        R(15, 36, 6, 3, 6);
        R(20, 14, 3, 9, 3);
    }
    return b;
}

gs::Bitmap lookout(int pose) {
    gs::Bitmap b(26, 42);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    int bob = pose ? 1 : 0;
    R(8, 1 + bob, 10, 3, 6);
    R(7, 4 + bob, 12, 6, 4);
    R(5, 10 + bob, 16, 14, 2);
    R(5, 10 + bob, 3, 14, 3);
    R(16, 13 + bob, 6, 2, 5); // pike
    R(20, 8 + bob, 2, 16, 7);
    if (pose) {
        R(7, 24, 4, 12, 2);
        R(15, 25, 4, 11, 2);
        R(6, 35, 6, 3, 1);
        R(14, 35, 6, 3, 1);
    } else {
        R(8, 24, 4, 12, 2);
        R(14, 24, 4, 12, 2);
        R(7, 35, 6, 3, 1);
        R(13, 35, 6, 3, 1);
    }
    return b;
}

gs::Bitmap cloth(int flutter) {
    gs::Bitmap b(28, 38);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    int lean = flutter ? 2 : 0;
    R(13, 0, 2, 34, 6);
    R(3 + lean, 3, 18, 6, 5);
    R(4 + lean, 9, 16, 16, 2);
    R(4 + lean, 9, 5, 16, 3);
    R(12 + lean, 13, 5, 8, 4); // device
    R(5 + lean, 25, 3, 7, 2);
    R(11 + lean, 25, 3, 6, 3);
    R(16 + lean, 25, 3, 5, 2);
    return b;
}

gs::Bitmap ashlar() {
    gs::Bitmap b(28, 14);
    b.rect(0, 1, 28, 12, 2);
    b.rect(0, 1, 28, 3, 4);
    b.rect(0, 10, 28, 3, 1);
    b.rect(8, 4, 2, 6, 3);
    b.rect(18, 4, 2, 6, 3);
    return b;
}

gs::Bitmap pier() {
    gs::Bitmap b(14, 56);
    b.rect(3, 0, 8, 56, 2);
    b.rect(2, 0, 3, 56, 4);
    b.rect(1, 48, 12, 6, 1);
    b.rect(4, 12, 6, 2, 3);
    b.rect(4, 28, 6, 2, 3);
    return b;
}

gs::Bitmap archRing() {
    gs::Bitmap b(36, 28);
    b.rect(0, 0, 6, 28, 2);
    b.rect(30, 0, 6, 28, 2);
    b.rect(0, 0, 36, 6, 4);
    b.ellipse(18, 8, 10, 8, 5);
    b.rect(6, 20, 24, 4, 3);
    return b;
}

gs::Bitmap mist() {
    gs::Bitmap b(40, 12);
    b.ellipse(10, 7, 10, 4, 2);
    b.ellipse(26, 8, 12, 4, 3);
    b.rect(4, 9, 30, 2, 4);
    return b;
}

gs::Bitmap lamp() {
    gs::Bitmap b(10, 30);
    b.rect(4, 10, 2, 20, 2);
    b.rect(2, 1, 6, 9, 3);
    b.rect(3, 2, 4, 6, 5);
    return b;
}

gs::Bitmap abutment() {
    gs::Bitmap b(22, 64);
    b.rect(2, 10, 6, 54, 2);
    b.rect(14, 10, 6, 54, 2);
    b.rect(1, 8, 20, 6, 4);
    b.rect(1, 28, 20, 4, 3);
    b.rect(1, 46, 20, 3, 1);
    b.rect(8, 0, 6, 10, 5);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    gs::TileAlloc tiles(vdp);
    setPal(vdp, PAL_HUD, {gs::rgb4(0, 0, 0), gs::rgb4(14, 14, 12), gs::rgb4(3, 2, 4), gs::rgb4(15, 11, 4),
                          gs::rgb4(12, 3, 3), gs::rgb4(7, 6, 8), gs::rgb4(2, 1, 3)});
    setPal(vdp, PAL_STONE, {gs::rgb4(0, 0, 0), gs::rgb4(3, 3, 4), gs::rgb4(7, 6, 7), gs::rgb4(5, 4, 5),
                            gs::rgb4(10, 9, 8), gs::rgb4(4, 3, 5), gs::rgb4(8, 7, 6)});
    setPal(vdp, PAL_COAT, {gs::rgb4(0, 0, 0), gs::rgb4(1, 1, 2), gs::rgb4(4, 4, 6), gs::rgb4(2, 2, 4),
                           gs::rgb4(12, 9, 7), gs::rgb4(10, 7, 3), gs::rgb4(2, 2, 2), gs::rgb4(6, 6, 7),
                           gs::rgb4(14, 13, 11)});
    setPal(vdp, PAL_BANNER, {gs::rgb4(0, 0, 0), gs::rgb4(3, 1, 1), gs::rgb4(13, 2, 2), gs::rgb4(8, 1, 2),
                             gs::rgb4(14, 12, 4), gs::rgb4(7, 5, 2), gs::rgb4(5, 4, 3), gs::rgb4(15, 14, 8)});
    setPal(vdp, PAL_LOOK, {gs::rgb4(0, 0, 0), gs::rgb4(1, 1, 1), gs::rgb4(3, 3, 4), gs::rgb4(2, 2, 3),
                           gs::rgb4(9, 8, 6), gs::rgb4(12, 4, 3), gs::rgb4(4, 4, 3), gs::rgb4(10, 9, 7)});
    setPal(vdp, PAL_GORGE, {gs::rgb4(0, 0, 0), gs::rgb4(1, 1, 2), gs::rgb4(2, 2, 3), gs::rgb4(3, 3, 4),
                            gs::rgb4(5, 5, 6), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_MIST, {gs::rgb4(0, 0, 0), gs::rgb4(2, 2, 3), gs::rgb4(4, 4, 5), gs::rgb4(6, 6, 7),
                           gs::rgb4(8, 8, 9), gs::rgb4(3, 3, 4)});
    setPal(vdp, PAL_LAMP, {gs::rgb4(0, 0, 0), gs::rgb4(3, 3, 2), gs::rgb4(5, 5, 4), gs::rgb4(9, 7, 3),
                           gs::rgb4(12, 9, 4), gs::rgb4(15, 13, 6)});

    art.stand = gs::uploadMipped(vdp, runner(0));
    art.walkA = gs::uploadMipped(vdp, runner(1));
    art.walkB = gs::uploadMipped(vdp, runner(2));
    art.leap = gs::uploadMipped(vdp, runner(3));
    art.look[0] = gs::uploadMipped(vdp, lookout(0));
    art.look[1] = gs::uploadMipped(vdp, lookout(1));
    art.banner = gs::uploadMipped(vdp, cloth(0));
    art.bannerB = gs::uploadMipped(vdp, cloth(1));
    art.ashlar = gs::uploadMipped(vdp, ashlar());
    art.pier = gs::uploadMipped(vdp, pier());
    art.arch = gs::uploadMipped(vdp, archRing());
    art.mist = gs::uploadMipped(vdp, mist());
    art.lamp = gs::uploadMipped(vdp, lamp());
    art.abutment = gs::uploadMipped(vdp, abutment());
    loadFont(vdp, art, tiles);
    vdp.setFogColor(gs::rgb4(3, 2, 5));
}

}  // namespace viaductbann

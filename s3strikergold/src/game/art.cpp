#include "game/art.h"

#include <cstdint>
#include <initializer_list>

namespace strikergold {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
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
    }
}

void stamp(gs::Bitmap& b, const char* s, int x, int y, int c) {
    gs::Bitmap t = gs::textBitmap(s, {1, c, 0, 0, 1});
    b.blit(t, x, y);
}

gs::Bitmap towerArt() {
    gs::Bitmap b(kTowerW, kTowerPixH);
    for (int y = 0; y < kTowerPixH; y++) {
        for (int x = 0; x < kTowerW; x++) {
            bool post = x < 7 || x >= kTowerW - 7;
            bool cap = y < 12 || y > kTowerPixH - 14;
            bool grain = ((x * 3 + y) & 11) == 0;
            int c = 0;
            if (post || cap) c = grain ? 3 : 2;
            else if (x >= 18 && x <= 36) c = ((y / 4) & 1) ? 1 : 5;
            else c = grain ? 4 : 5;
            if (y > 144 && x > 12 && x < 44) c = 6;
            b.set(x, y, c);
        }
    }
    b.rect(20, 4, 16, 6, 7);
    const char* face[kMarkN] = {"10", "20", "30", "40"};
    for (int i = 0; i < kMarkN; i++) {
        float y = kSlotBot - kMarkH[i] * (kSlotBot - kSlotTop);
        int iy = int(y);
        int ink = i == kGold ? 9 : 8;
        b.rect(7, iy, 11, 2, ink);
        b.rect(kTowerW - 18, iy, 11, 2, ink);
        if (i == kGold) {
            b.rect(16, iy - 8, 24, 12, 9);
            stamp(b, "X2", 20, iy - 7, 2);
        } else {
            stamp(b, face[i], 8, iy - 8, ink);
        }
    }
    return b;
}

gs::Bitmap puckArt() {
    gs::Bitmap b(14, 10);
    b.ellipse(7.f, 5.f, 6.f, 4.f, 2);
    b.ellipse(7.f, 4.2f, 3.4f, 1.8f, 3);
    b.rect(3, 7, 8, 2, 1);
    return b;
}

gs::Bitmap bellArt(bool lit) {
    gs::Bitmap b(24, 22);
    b.ellipse(12.f, 9.f, 10.f, 8.f, lit ? 3 : 2);
    b.ellipse(12.f, 8.f, 5.5f, 4.f, lit ? 4 : 3);
    b.rect(11, 1, 2, 4, 1);
    b.ellipse(12.f, 18.f, 2.4f, 2.4f, 1);
    if (lit) b.rect(4, 6, 3, 2, 4);
    return b;
}

gs::Bitmap manArt(int pose) {
    gs::Bitmap b(30, 50);
    b.ellipse(15.f, 8.f, 5.2f, 5.2f, 3);
    b.rect(12, 13, 7, 15, 2);
    b.rect(9, 15, 4, 9, 4);
    b.rect(18, 15, 4, 9, 4);
    b.rect(11, 27, 4, 16, 1);
    b.rect(16, 27, 4, 16, 1);
    if (pose == 0) b.rect(6, 17, 4, 8, 4);
    else if (pose == 1) b.rect(18, 5, 9, 4, 4);
    else b.rect(3, 22, 9, 4, 4);
    b.rect(9, 43, 5, 5, 5);
    b.rect(16, 43, 5, 5, 5);
    b.rect(13, 6, 4, 2, 6);
    return b;
}

gs::Bitmap malletArt() {
    gs::Bitmap b(10, 30);
    b.rect(4, 8, 2, 20, 1);
    b.ellipse(5.f, 6.f, 4.2f, 4.4f, 2);
    b.rect(2, 4, 6, 2, 3);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(12, 18);
    b.rect(5, 9, 2, 9, 1);
    b.ellipse(6.f, 6.f, 5.f, 4.5f, 2);
    b.ellipse(6.f, 5.4f, 2.4f, 2.f, 3);
    return b;
}

gs::Bitmap flagArt() {
    gs::Bitmap b(22, 16);
    b.rect(1, 1, 2, 14, 1);
    b.poly({{3, 2}, {20, 5}, {3, 10}}, 2);
    b.poly({{5, 3}, {16, 5}, {5, 8}}, 3);
    return b;
}

gs::Bitmap titleArt() {
    gs::Bitmap word = gs::textBitmap("STRIKER GOLD", {2, 1, 2, 0, 1});
    gs::Bitmap b(word.w + 8, word.h + 8);
    b.rect(0, 0, b.w, b.h, 3);
    b.rect(2, 2, b.w - 4, b.h - 4, 2);
    b.blit(word, 4, 4);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 14, 12), gs::rgb4(3, 2, 2)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 3), gs::rgb4(7, 4, 1), gs::rgb4(15, 15, 8), gs::rgb4(4, 2, 0)});
    setPal(vdp, PAL_RED, {0, gs::rgb4(14, 3, 3), gs::rgb4(6, 1, 1)});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(5, 14, 6), gs::rgb4(1, 5, 2)});
    setPal(vdp, PAL_WOOD,
           {0, gs::rgb4(2, 1, 2), gs::rgb4(9, 5, 2), gs::rgb4(12, 7, 3), gs::rgb4(6, 3, 2), gs::rgb4(8, 6, 4),
            gs::rgb4(4, 4, 5), gs::rgb4(13, 11, 7), gs::rgb4(12, 11, 9), gs::rgb4(15, 11, 2)});
    setPal(vdp, PAL_BRASS, {0, gs::rgb4(6, 4, 1), gs::rgb4(13, 9, 2), gs::rgb4(15, 13, 5), gs::rgb4(15, 15, 10)});
    setPal(vdp, PAL_NIGHT, {0, gs::rgb4(3, 2, 6), gs::rgb4(15, 13, 5), gs::rgb4(11, 4, 6)});
    setPal(vdp, PAL_MAN,
           {0, gs::rgb4(3, 2, 4), gs::rgb4(10, 2, 3), gs::rgb4(14, 10, 7), gs::rgb4(12, 7, 4), gs::rgb4(2, 2, 3),
            gs::rgb4(5, 3, 2)});
    setPal(vdp, PAL_CREAM, {0, gs::rgb4(14, 12, 8), gs::rgb4(8, 6, 3), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(15, 14, 6), gs::rgb4(5, 3, 1), gs::rgb4(9, 4, 2)});
    setPal(vdp, PAL_CROWD, {0, gs::rgb4(2, 2, 4), gs::rgb4(5, 3, 8), gs::rgb4(7, 2, 3), gs::rgb4(3, 5, 6)});
    vdp.setFogColor(gs::rgb4(2, 1, 4));
    loadFont(vdp, art);
    art.tower = gs::uploadMipped(vdp, towerArt());
    art.puck = gs::uploadMipped(vdp, puckArt());
    art.bell[0] = gs::uploadMipped(vdp, bellArt(false));
    art.bell[1] = gs::uploadMipped(vdp, bellArt(true));
    for (int i = 0; i < 3; i++) art.man[i] = gs::uploadMipped(vdp, manArt(i));
    art.mallet = gs::uploadMipped(vdp, malletArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.flag = gs::uploadMipped(vdp, flagArt());
    art.title = gs::uploadImage(vdp, titleArt());
}

}  // namespace strikergold

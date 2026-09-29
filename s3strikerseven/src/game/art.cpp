#include "game/art.h"

#include <initializer_list>

namespace strikerseven {
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

gs::Bitmap towerArt() {
    gs::Bitmap b(kTowerW, kTowerPixH);
    for (int y = 0; y < kTowerPixH; y++) {
        for (int x = 0; x < kTowerW; x++) {
            bool post = x < 7 || x >= kTowerW - 7;
            bool cap = y < 12 || y > kTowerPixH - 10;
            bool grain = ((x * 3 + y) & 7) == 0;
            int c = 0;
            if (post || cap) c = grain ? 3 : 2;
            else if (x >= 15 && x <= 32) c = 1;
            else c = grain ? 4 : 5;
            if (y > 144 && x > 8 && x < 40) c = 6;
            b.set(x, y, c);
        }
    }
    const float marks[3] = {kTickH, kPairH, kBellH};
    const int ink[3] = {8, 10, 9};
    for (int i = 0; i < 3; i++) {
        int iy = int(kSlotBot - marks[i] * (kSlotBot - kSlotTop));
        b.rect(7, iy, 9, 2, ink[i]);
        b.rect(kTowerW - 16, iy, 9, 2, ink[i]);
    }
    b.rect(16, 6, 16, 7, 7);
    b.rect(18, 148, 12, 6, 6);
    return b;
}

gs::Bitmap puckArt() {
    gs::Bitmap b(12, 8);
    b.ellipse(6.f, 4.f, 5.f, 3.f, 2);
    b.ellipse(6.f, 3.2f, 3.f, 1.5f, 3);
    b.rect(2, 6, 8, 2, 1);
    return b;
}

gs::Bitmap bellArt(bool lit) {
    gs::Bitmap b(22, 18);
    b.ellipse(11.f, 8.f, 9.f, 6.5f, lit ? 3 : 2);
    b.ellipse(11.f, 7.f, 5.f, 3.f, lit ? 4 : 3);
    b.rect(10, 1, 2, 3, 1);
    b.ellipse(11.f, 15.f, 2.f, 2.f, 1);
    return b;
}

gs::Bitmap manArt(int pose) {
    gs::Bitmap b(28, 46);
    b.ellipse(14.f, 8.f, 5.f, 5.f, 3);
    b.rect(11, 13, 6, 13, 2);
    b.rect(16, 14, 4, 9, 4);
    b.rect(10, 25, 4, 14, 1);
    b.rect(15, 25, 4, 14, 1);
    if (pose == 0) b.rect(6, 16, 5, 8, 4);
    else if (pose == 1) b.rect(16, 5, 8, 4, 4);
    else b.rect(3, 20, 9, 4, 4);
    b.rect(8, 39, 5, 4, 5);
    b.rect(15, 39, 5, 4, 5);
    return b;
}

gs::Bitmap malletArt() {
    gs::Bitmap b(8, 26);
    b.rect(3, 6, 2, 18, 1);
    b.ellipse(4.f, 5.f, 3.2f, 3.6f, 2);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(10, 16);
    b.rect(4, 8, 2, 8, 1);
    b.ellipse(5.f, 5.f, 4.f, 4.f, 2);
    b.ellipse(5.f, 4.4f, 2.f, 2.f, 3);
    return b;
}

gs::Bitmap buntArt() {
    gs::Bitmap b(18, 12);
    b.poly({{0, 0}, {18, 0}, {9, 11}}, 2);
    b.poly({{2, 0}, {16, 0}, {9, 8}}, 3);
    return b;
}

gs::Bitmap crowdArt() {
    gs::Bitmap b(34, 26);
    b.ellipse(7.f, 7.f, 4.5f, 4.5f, 2);
    b.ellipse(17.f, 6.f, 4.5f, 4.5f, 3);
    b.ellipse(27.f, 8.f, 4.5f, 4.5f, 4);
    b.rect(3, 12, 8, 13, 2);
    b.rect(13, 11, 8, 14, 3);
    b.rect(23, 13, 8, 12, 4);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 14, 13), gs::rgb4(3, 2, 2)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 4), gs::rgb4(8, 5, 1), gs::rgb4(15, 15, 8)});
    setPal(vdp, PAL_RED, {0, gs::rgb4(14, 3, 3), gs::rgb4(6, 1, 1)});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(5, 14, 6), gs::rgb4(1, 5, 2)});
    setPal(vdp, PAL_WOOD,
           {0, gs::rgb4(2, 1, 2), gs::rgb4(8, 4, 2), gs::rgb4(11, 6, 3), gs::rgb4(6, 3, 2), gs::rgb4(9, 7, 4),
            gs::rgb4(4, 4, 5), gs::rgb4(12, 10, 6), gs::rgb4(13, 12, 10), gs::rgb4(15, 12, 3), gs::rgb4(4, 12, 5),
            gs::rgb4(12, 3, 3)});
    setPal(vdp, PAL_BRASS, {0, gs::rgb4(6, 4, 1), gs::rgb4(12, 8, 2), gs::rgb4(15, 13, 5), gs::rgb4(15, 15, 10)});
    setPal(vdp, PAL_NIGHT, {0, gs::rgb4(3, 2, 6), gs::rgb4(15, 12, 4), gs::rgb4(12, 4, 6)});
    setPal(vdp, PAL_MAN,
           {0, gs::rgb4(3, 2, 4), gs::rgb4(12, 3, 4), gs::rgb4(14, 10, 7), gs::rgb4(13, 8, 5), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_THEM,
           {0, gs::rgb4(2, 2, 5), gs::rgb4(3, 5, 12), gs::rgb4(12, 9, 6), gs::rgb4(8, 8, 12), gs::rgb4(1, 1, 3)});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(15, 14, 6), gs::rgb4(4, 2, 1), gs::rgb4(8, 3, 2)});
    setPal(vdp, PAL_CROWD, {0, gs::rgb4(2, 2, 4), gs::rgb4(4, 3, 7), gs::rgb4(6, 2, 4), gs::rgb4(3, 4, 6)});
    vdp.setFogColor(gs::rgb4(2, 1, 4));
    loadFont(vdp, art);
    art.tower = gs::uploadMipped(vdp, towerArt());
    art.puck = gs::uploadMipped(vdp, puckArt());
    art.bell[0] = gs::uploadMipped(vdp, bellArt(false));
    art.bell[1] = gs::uploadMipped(vdp, bellArt(true));
    for (int i = 0; i < 3; i++) art.man[i] = gs::uploadMipped(vdp, manArt(i));
    art.mallet = gs::uploadMipped(vdp, malletArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.bunt = gs::uploadMipped(vdp, buntArt());
    art.crowd = gs::uploadMipped(vdp, crowdArt());
    art.title = gs::uploadImage(vdp, gs::textBitmap("STRIKER", {2, 1, 2, 0, 1}));
    art.banner = gs::uploadImage(vdp, gs::textBitmap("TO SEVEN", {1, 1, 0, 0, 1}));
}

}  // namespace strikerseven

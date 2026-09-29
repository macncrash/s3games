#include "game/art.h"

#include <cstdint>
#include <initializer_list>

namespace strikermark {
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
            bool post = x < 8 || x >= kTowerW - 8;
            bool cap = y < 14 || y > kTowerPixH - 12;
            bool grain = ((x + y * 3) & 7) == 0;
            int c = 0;
            if (post || cap) c = grain ? 3 : 2;
            else if (x >= 16 && x <= 34) c = 1;
            else c = grain ? 4 : 5;
            if (y > 150 && x > 10 && x < 42) c = 6;
            b.set(x, y, c);
        }
    }
    b.rect(18, 8, 16, 8, 7);
    for (int i = 0; i < kMarkN; i++) {
        float y = kSlotBot - kMarkH[i] * (kSlotBot - kSlotTop);
        int iy = int(y);
        int ink = i == kGold ? 9 : 8;
        b.rect(8, iy, 10, 2, ink);
        b.rect(kTowerW - 18, iy, 10, 2, ink);
        if (i == kGold) b.rect(17, iy - 3, 18, 7, 9);
    }
    b.rect(20, 154, 12, 8, 6);
    return b;
}

gs::Bitmap puckArt() {
    gs::Bitmap b(12, 8);
    b.ellipse(6.f, 4.f, 5.2f, 3.2f, 2);
    b.ellipse(6.f, 3.4f, 3.2f, 1.6f, 3);
    b.rect(2, 6, 8, 2, 1);
    return b;
}

gs::Bitmap bellArt(bool lit) {
    gs::Bitmap b(22, 20);
    b.ellipse(11.f, 8.f, 9.f, 7.f, lit ? 3 : 2);
    b.ellipse(11.f, 7.f, 5.f, 3.5f, lit ? 4 : 3);
    b.rect(10, 1, 2, 4, 1);
    b.ellipse(11.f, 16.f, 2.2f, 2.2f, 1);
    return b;
}

gs::Bitmap manArt(int pose) {
    gs::Bitmap b(28, 48);
    b.ellipse(14.f, 8.f, 5.f, 5.f, 3);
    b.rect(11, 13, 6, 14, 2);
    b.rect(8, 14, 4, 10, 4);
    b.rect(16, 14, 4, 10, 4);
    b.rect(10, 26, 4, 16, 1);
    b.rect(15, 26, 4, 16, 1);
    if (pose == 0) {
        b.rect(6, 16, 4, 8, 4);
    } else if (pose == 1) {
        b.rect(16, 6, 8, 4, 4);
    } else {
        b.rect(4, 20, 8, 4, 4);
    }
    b.rect(8, 42, 5, 4, 5);
    b.rect(15, 42, 5, 4, 5);
    return b;
}

gs::Bitmap malletArt() {
    gs::Bitmap b(8, 28);
    b.rect(3, 6, 2, 20, 1);
    b.ellipse(4.f, 5.f, 3.4f, 4.f, 2);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(10, 16);
    b.rect(4, 8, 2, 8, 1);
    b.ellipse(5.f, 5.f, 4.f, 4.f, 2);
    b.ellipse(5.f, 4.5f, 2.f, 2.f, 3);
    return b;
}

gs::Bitmap buntArt() {
    gs::Bitmap b(20, 12);
    b.poly({{0, 0}, {20, 0}, {10, 11}}, 2);
    b.poly({{2, 0}, {18, 0}, {10, 8}}, 3);
    return b;
}

gs::Bitmap crowdArt() {
    gs::Bitmap b(36, 28);
    b.ellipse(8.f, 8.f, 5.f, 5.f, 2);
    b.ellipse(18.f, 7.f, 5.f, 5.f, 3);
    b.ellipse(28.f, 9.f, 5.f, 5.f, 4);
    b.rect(4, 13, 8, 14, 2);
    b.rect(14, 12, 8, 15, 3);
    b.rect(24, 14, 8, 13, 4);
    return b;
}

gs::Bitmap titleArt() {
    return gs::textBitmap("STRIKER", {2, 1, 2, 0, 1});
}

gs::Bitmap bannerArt() {
    gs::Bitmap b(120, 16);
    b.rect(0, 0, 120, 16, 2);
    b.rect(2, 2, 116, 12, 1);
    gs::Bitmap t = gs::textBitmap("GOLD MARK", {1, 3, 0, 0, 1});
    b.blit(t, 60 - t.w / 2, 4);
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
            gs::rgb4(4, 4, 5), gs::rgb4(12, 10, 6), gs::rgb4(13, 12, 10), gs::rgb4(15, 12, 3)});
    setPal(vdp, PAL_BRASS, {0, gs::rgb4(6, 4, 1), gs::rgb4(12, 8, 2), gs::rgb4(15, 13, 5)});
    setPal(vdp, PAL_NIGHT, {0, gs::rgb4(3, 2, 6), gs::rgb4(15, 12, 4), gs::rgb4(12, 4, 6), gs::rgb4(8, 8, 12)});
    setPal(vdp, PAL_MAN,
           {0, gs::rgb4(3, 2, 4), gs::rgb4(12, 3, 4), gs::rgb4(14, 10, 7), gs::rgb4(13, 8, 5), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 15, 12), gs::rgb4(15, 8, 2)});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(15, 14, 6), gs::rgb4(4, 2, 1), gs::rgb4(8, 3, 2)});
    setPal(vdp, PAL_CROWD,
           {0, gs::rgb4(2, 2, 4), gs::rgb4(4, 3, 7), gs::rgb4(6, 2, 4), gs::rgb4(3, 4, 6)});
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
    art.title = gs::uploadImage(vdp, titleArt());
    art.banner = gs::uploadImage(vdp, bannerArt());
}

}  // namespace strikermark

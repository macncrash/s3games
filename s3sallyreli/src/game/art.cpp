#include "art.h"

#include <cstdint>

namespace sally {
namespace {

void setPal(gs::VDP& vdp, int pal, const uint16_t* cs, int n) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, i < n ? cs[i] : 0);
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& art) {
    for (int i = 0; i < 96; i++) {
        gs::TextStyle st;
        st.scale = 1;
        st.color = 1;
        gs::Bitmap g = gs::textBitmap(std::string(1, char(32 + i)), st);
        gs::Bitmap tile(8, 8);
        for (int y = 0; y < g.h && y < 8; y++)
            for (int x = 0; x < g.w && x < 8; x++)
                if (g.get(x, y)) tile.set(x, y, 1);
        art.font[i] = tiles.shared(tile.px.data());
        art.glyph[i] = gs::uploadMipped(vdp, g.w > 0 ? g : tile);
    }
}

gs::Bitmap soldier(int step) {
    gs::Bitmap b(32, 44);
    b.ellipse(14, 7, 6, 6, 3);
    b.rect(10, 4, 8, 3, 4);
    b.rect(9, 13, 10, 4, 5);
    b.rect(8, 17, 12, 12, 1);
    b.rect(10, 19, 8, 6, 2);
    b.rect(step ? 5.f : 9.f, 18, 4, 9, 6);
    b.rect(step ? 16.f : 13.f, 18, 4, 9, 6);
    b.rect(step ? 8.f : 11.f, 29, 4, 11, 1);
    b.rect(step ? 15.f : 13.f, 29, 4, 11, 2);
    b.rect(6, 39, 6, 3, 7);
    b.rect(16, 39, 6, 3, 7);
    b.rect(18, 20, 12, 2, 8);
    b.outline(9, false);
    return b;
}

gs::Bitmap raider(int step, bool fast, bool plated) {
    gs::Bitmap b(30, 40);
    b.ellipse(16, 7, 5, 5, fast ? 4 : 3);
    b.rect(12, 12, 8, 3, 5);
    b.rect(10, 15, 10, 11, plated ? 6 : 1);
    if (plated) b.rect(12, 17, 6, 7, 2);
    b.rect(step ? 7.f : 11.f, 16, 3, 8, 7);
    b.rect(step ? 16.f : 14.f, 16, 3, 8, 8);
    b.rect(step ? 10.f : 12.f, 26, 3, 10, 1);
    b.rect(step ? 15.f : 14.f, 26, 3, 10, 2);
    b.rect(8, 35, 5, 3, 9);
    b.rect(15, 35, 5, 3, 9);
    b.rect(2, 18, 10, 2, 10);
    b.outline(11, false);
    return b;
}

gs::Bitmap wallArt() {
    gs::Bitmap b(72, 180);
    b.rect(0, 0, 72, 180, 1);
    for (int y = 8; y < 176; y += 12) {
        int off = ((y / 12) & 1) ? 8 : 0;
        for (int x = off; x < 68; x += 16) b.rect(float(x), float(y), 14, 10, (x + y) & 16 ? 2 : 3);
    }
    b.rect(0, 0, 72, 8, 4);
    for (int x = 4; x < 68; x += 14) b.rect(float(x), 0, 6, 10, 5);
    b.outline(6, false);
    return b;
}

gs::Bitmap archArt() {
    gs::Bitmap b(48, 64);
    b.rect(0, 16, 48, 48, 2);
    b.ellipse(24, 28, 16, 18, 1);
    b.rect(10, 28, 28, 36, 1);
    b.rect(0, 0, 8, 64, 3);
    b.rect(40, 0, 8, 64, 3);
    b.rect(0, 0, 48, 6, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap bannerArt() {
    gs::Bitmap b(18, 28);
    b.rect(8, 0, 2, 28, 3);
    b.rect(10, 2, 7, 12, 1);
    b.rect(12, 5, 3, 6, 2);
    return b;
}

gs::Bitmap pikeArt() {
    gs::Bitmap b(54, 8);
    b.rect(0, 3, 40, 2, 1);
    b.poly({{40, 1}, {52, 4}, {40, 7}}, 2);
    b.rect(8, 2, 3, 4, 3);
    return b;
}

gs::Bitmap pistolArt() {
    gs::Bitmap b(22, 12);
    b.rect(2, 3, 16, 3, 1);
    b.rect(12, 6, 4, 5, 2);
    b.rect(0, 2, 4, 4, 3);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(26, 28);
    b.rect(11, 0, 4, 5, 3);
    b.ellipse(13, 15, 11, 9, 1);
    b.ellipse(13, 17, 6, 5, 2);
    b.rect(11, 22, 4, 4, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap flashArt() {
    gs::Bitmap b(14, 14);
    b.ellipse(7, 7, 6, 6, 1);
    b.ellipse(7, 7, 3, 3, 2);
    return b;
}

gs::Bitmap puffArt() {
    gs::Bitmap b(12, 12);
    b.ellipse(6, 6, 5, 5, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    const uint16_t hud[] = {0, gs::rgb4(14, 13, 11), gs::rgb4(7, 7, 6), gs::rgb4(15, 12, 5), gs::rgb4(15, 6, 3),
                            gs::rgb4(8, 14, 8), shadow};
    const uint16_t wall[] = {0, gs::rgb4(1, 1, 2), gs::rgb4(5, 5, 6), gs::rgb4(7, 7, 8), gs::rgb4(4, 4, 5),
                             gs::rgb4(9, 8, 6), gs::rgb4(2, 2, 3), shadow};
    const uint16_t you[] = {0, gs::rgb4(4, 5, 7), gs::rgb4(8, 9, 12), gs::rgb4(12, 9, 5), gs::rgb4(6, 4, 2),
                            gs::rgb4(9, 6, 4), gs::rgb4(3, 3, 5), gs::rgb4(2, 2, 2), gs::rgb4(11, 10, 6),
                            gs::rgb4(1, 1, 1), shadow};
    const uint16_t raid[] = {0, gs::rgb4(5, 3, 2), gs::rgb4(8, 4, 3), gs::rgb4(10, 7, 4), gs::rgb4(4, 3, 2),
                             gs::rgb4(7, 5, 3), gs::rgb4(6, 6, 5), gs::rgb4(3, 3, 2), gs::rgb4(9, 8, 4),
                             gs::rgb4(2, 2, 1), gs::rgb4(8, 7, 3), gs::rgb4(1, 1, 1), shadow};
    const uint16_t shield[] = {0, gs::rgb4(3, 4, 5), gs::rgb4(8, 9, 10), gs::rgb4(10, 8, 5), gs::rgb4(6, 2, 2),
                               gs::rgb4(4, 4, 5), gs::rgb4(7, 8, 9), gs::rgb4(2, 2, 3), gs::rgb4(5, 5, 4),
                               gs::rgb4(2, 1, 1), gs::rgb4(12, 10, 4), gs::rgb4(1, 1, 1), shadow};
    const uint16_t fx[] = {0, gs::rgb4(12, 11, 8), gs::rgb4(15, 15, 12), gs::rgb4(15, 10, 3), shadow};
    const uint16_t bell[] = {0, gs::rgb4(13, 10, 3), gs::rgb4(8, 6, 2), gs::rgb4(5, 4, 2), gs::rgb4(15, 13, 6),
                             gs::rgb4(3, 2, 1), shadow};
    const uint16_t alert[] = {0, gs::rgb4(15, 4, 3), gs::rgb4(8, 1, 1), shadow};
    const uint16_t ok[] = {0, gs::rgb4(8, 15, 7), gs::rgb4(2, 6, 3), shadow};
    const uint16_t field[] = {0, gs::rgb4(3, 5, 2), gs::rgb4(5, 7, 3), gs::rgb4(8, 7, 3), shadow};
    setPal(vdp, PAL_HUD, hud, 7);
    setPal(vdp, PAL_WALL, wall, 8);
    setPal(vdp, PAL_YOU, you, 11);
    setPal(vdp, PAL_RAID, raid, 13);
    setPal(vdp, PAL_SHIELD, shield, 13);
    setPal(vdp, PAL_FX, fx, 5);
    setPal(vdp, PAL_BELL, bell, 7);
    setPal(vdp, PAL_ALERT, alert, 4);
    setPal(vdp, PAL_OK, ok, 4);
    setPal(vdp, PAL_FIELD, field, 5);
    vdp.setFogColor(gs::rgb4(3, 4, 5));

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    art.wall = gs::uploadMipped(vdp, wallArt());
    art.arch = gs::uploadMipped(vdp, archArt());
    art.banner = gs::uploadMipped(vdp, bannerArt());
    art.pike = gs::uploadMipped(vdp, pikeArt());
    art.pistol = gs::uploadMipped(vdp, pistolArt());
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.flash = gs::uploadMipped(vdp, flashArt());
    art.puff = gs::uploadMipped(vdp, puffArt());
    for (int s = 0; s < 2; s++) {
        art.you[s] = gs::uploadMipped(vdp, soldier(s));
        art.raider[s] = gs::uploadMipped(vdp, raider(s, false, false));
        art.runner[s] = gs::uploadMipped(vdp, raider(s, true, false));
        art.shield[s] = gs::uploadMipped(vdp, raider(s, false, true));
    }
}

}  // namespace sally

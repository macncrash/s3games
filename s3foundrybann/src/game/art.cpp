#include "art.h"

namespace foundry {
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

gs::Bitmap workerArt(int step) {
    gs::Bitmap b(24, 40);
    b.ellipse(12, 7, 5, 5, 4);
    b.rect(9, 12, 6, 3, 5);
    b.rect(7, 15, 10, 12, 2);
    b.rect(8, 17, 8, 5, 6);
    b.rect(step ? 3.f : 5.f, 16, 4, 9, 3);
    b.rect(step ? 17.f : 15.f, 16, 4, 9, 3);
    b.rect(step ? 8.f : 10.f, 27, 4, 10, 1);
    b.rect(step ? 13.f : 11.f, 27, 4, 10, 1);
    b.rect(7, 36, 5, 3, 8);
    b.rect(13, 36, 5, 3, 8);
    b.rect(10, 6, 2, 2, 7);
    b.rect(14, 6, 2, 2, 7);
    b.outline(9, false);
    return b;
}

gs::Bitmap bannerArt(int step) {
    gs::Bitmap b(28, 48);
    b.rect(4, 2, 3, 44, 3);
    b.rect(7, 4, 16, 22, 1);
    b.rect(7, 14, 16, 4, 2);
    int flutter = step ? 2 : 0;
    b.poly({{7, 26}, {23, 26}, {21.f + flutter, 40}, {9.f - flutter, 40}}, 1);
    b.rect(8, 28, 12, 3, 2);
    b.rect(3, 44, 5, 3, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap ingotArt() {
    gs::Bitmap b(36, 18);
    b.poly({{2, 10}, {10, 3}, {28, 3}, {34, 10}, {28, 15}, {8, 15}}, 1);
    b.rect(10, 6, 16, 4, 2);
    b.rect(12, 8, 6, 2, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap pourArt() {
    gs::Bitmap b(14, 80);
    b.rect(4, 0, 6, 80, 1);
    b.rect(5, 0, 3, 80, 2);
    for (int y = 4; y < 78; y += 10) b.rect(3, y, 3, 4, 3);
    b.ellipse(7, 74, 6, 5, 4);
    return b;
}

gs::Bitmap ladleArt() {
    gs::Bitmap b(30, 26);
    b.rect(14, 0, 3, 8, 1);
    b.ellipse(15, 16, 12, 7, 2);
    b.ellipse(15, 15, 8, 4, 3);
    b.rect(24, 14, 5, 3, 1);
    b.outline(5, false);
    return b;
}

gs::Bitmap archArt() {
    gs::Bitmap b(64, 72);
    b.rect(0, 8, 10, 64, 1);
    b.rect(54, 8, 10, 64, 1);
    b.rect(0, 0, 64, 12, 2);
    b.rect(14, 28, 36, 44, 4);
    b.rect(18, 34, 28, 6, 3);
    b.outline(5, false);
    return b;
}

gs::Bitmap stackArt() {
    gs::Bitmap b(40, 56);
    b.rect(6, 16, 28, 40, 1);
    b.rect(10, 4, 14, 16, 2);
    b.rect(13, 0, 8, 6, 3);
    b.rect(8, 24, 8, 6, 4);
    b.rect(22, 34, 8, 6, 4);
    b.rect(12, 44, 10, 5, 6);
    b.outline(5, false);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t hud[] = {gs::rgb4(0, 0, 0), gs::rgb4(15, 13, 8), gs::rgb4(8, 7, 5), gs::rgb4(15, 8, 2)};
    const uint16_t brick[] = {0, gs::rgb4(6, 3, 2), gs::rgb4(9, 5, 3), gs::rgb4(14, 8, 3), gs::rgb4(2, 2, 3), gs::rgb4(1, 1, 1)};
    const uint16_t you[] = {0, gs::rgb4(3, 3, 4), gs::rgb4(5, 6, 8), gs::rgb4(8, 5, 3), gs::rgb4(12, 8, 5),
                            gs::rgb4(4, 3, 2), gs::rgb4(13, 10, 4), gs::rgb4(2, 2, 2), gs::rgb4(1, 1, 1), gs::rgb4(0, 0, 0)};
    const uint16_t iron[] = {0, gs::rgb4(7, 7, 8), gs::rgb4(11, 11, 12), gs::rgb4(4, 4, 5), gs::rgb4(14, 10, 4), gs::rgb4(1, 1, 2)};
    const uint16_t melt[] = {0, gs::rgb4(12, 3, 1), gs::rgb4(15, 9, 2), gs::rgb4(15, 14, 5), gs::rgb4(8, 2, 1), gs::rgb4(3, 2, 2)};
    const uint16_t ban[] = {0, gs::rgb4(12, 2, 2), gs::rgb4(14, 11, 3), gs::rgb4(5, 3, 2), gs::rgb4(3, 2, 1), gs::rgb4(1, 0, 0)};
    const uint16_t soot[] = {0, gs::rgb4(3, 3, 4), gs::rgb4(6, 5, 4), gs::rgb4(10, 6, 3), gs::rgb4(2, 2, 2), gs::rgb4(1, 1, 1)};
    const uint16_t ok[] = {0, gs::rgb4(8, 14, 6), gs::rgb4(2, 6, 3)};
    const uint16_t alert[] = {0, gs::rgb4(15, 6, 3), gs::rgb4(6, 2, 1)};
    setPal(vdp, PAL_HUD, hud, 4);
    setPal(vdp, PAL_BRICK, brick, 6);
    setPal(vdp, PAL_YOU, you, 10);
    setPal(vdp, PAL_IRON, iron, 6);
    setPal(vdp, PAL_MELT, melt, 6);
    setPal(vdp, PAL_BANNER, ban, 6);
    setPal(vdp, PAL_SOOT, soot, 6);
    setPal(vdp, PAL_OK, ok, 3);
    setPal(vdp, PAL_ALERT, alert, 3);

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, tiles, art);
    art.worker[0] = gs::uploadMipped(vdp, workerArt(0));
    art.worker[1] = gs::uploadMipped(vdp, workerArt(1));
    art.banner[0] = gs::uploadMipped(vdp, bannerArt(0));
    art.banner[1] = gs::uploadMipped(vdp, bannerArt(1));
    art.ingot = gs::uploadMipped(vdp, ingotArt());
    art.pour = gs::uploadMipped(vdp, pourArt());
    art.ladle = gs::uploadMipped(vdp, ladleArt());
    art.arch = gs::uploadMipped(vdp, archArt());
    art.stack = gs::uploadMipped(vdp, stackArt());
    vdp.HUD.resize(64, 32);
    vdp.HUD.clear();
}

}  // namespace foundry

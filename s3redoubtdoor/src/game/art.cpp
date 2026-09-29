#include "art.h"

#include <string>

namespace door {
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

gs::Bitmap wallArt() {
    gs::Bitmap b(200, 150);
    b.rect(0, 18, 200, 132, 1);
    b.rect(0, 10, 200, 14, 2);
    for (int y = 32; y < 140; y += 16) {
        int off = ((y / 16) & 1) ? 10 : 0;
        for (int x = -20 + off; x < 200; x += 22) b.rect(float(x), float(y), 20, 14, (x / 22 + y) & 1 ? 3 : 1);
    }
    b.rect(62, 36, 76, 100, 0);
    b.rect(58, 32, 84, 8, 4);
    b.rect(70, 48, 10, 28, 5);
    b.rect(120, 48, 10, 28, 5);
    b.rect(94, 78, 12, 18, 5);
    b.outline(6, false);
    return b;
}

gs::Bitmap doorArt(bool hurt) {
    gs::Bitmap b(68, 108);
    b.rect(4, 4, 60, 100, 1);
    for (int i = 0; i < 5; i++) b.rect(8 + i * 12.f, 8, 8, 92, i & 1 ? 2 : 3);
    b.rect(4, 28, 60, 4, 4);
    b.rect(4, 70, 60, 4, 4);
    b.rect(30, 48, 8, 6, 5);
    b.ellipse(34, 58, 3, 3, 6);
    if (hurt) {
        b.line(10, 12, 40, 96, 7, 2);
        b.line(50, 20, 22, 88, 7, 1.5f);
        b.rect(18, 40, 10, 16, 0);
    }
    b.outline(8, false);
    return b;
}

gs::Bitmap barArt() {
    gs::Bitmap b(78, 10);
    b.rect(0, 3, 78, 4, 1);
    b.rect(4, 1, 8, 8, 2);
    b.rect(66, 1, 8, 8, 2);
    return b;
}

gs::Bitmap manArt(int step, bool foe) {
    gs::Bitmap b(22, 34);
    int skin = foe ? 3 : 4;
    int coat = foe ? 1 : 2;
    b.ellipse(11, 6, 4, 4, skin);
    b.rect(8, 10, 6, 3, 5);
    b.rect(6, 13, 10, 10, coat);
    b.rect(step ? 3.f : 7.f, 14, 4, 8, 6);
    b.rect(step ? 15.f : 11.f, 14, 4, 8, foe ? 7 : 6);
    b.rect(step ? 7.f : 6.f, 22, 4, 8, 8);
    b.rect(step ? 12.f : 12.f, 22, 4, 8, 8);
    b.rect(5, 29, 5, 3, 9);
    b.rect(12, 29, 5, 3, 9);
    if (!foe) b.rect(14, 15, 8, 2, 7);
    else b.rect(0, 16, 8, 2, 7);
    b.outline(10, false);
    return b;
}

gs::Bitmap ramArt() {
    gs::Bitmap b(40, 16);
    b.rect(6, 5, 28, 6, 1);
    b.poly({{6, 5}, {0, 8}, {6, 11}}, 2);
    b.rect(30, 3, 6, 10, 3);
    b.rect(12, 2, 4, 12, 4);
    b.rect(22, 2, 4, 12, 4);
    return b;
}

gs::Bitmap sparkArt() {
    gs::Bitmap b(12, 12);
    b.ellipse(6, 6, 5, 5, 1);
    b.ellipse(6, 6, 2, 2, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t hud[] = {0, gs::rgb4(15, 14, 11), gs::rgb4(6, 5, 3)};
    const uint16_t stone[] = {0, gs::rgb4(6, 6, 6), gs::rgb4(8, 8, 7), gs::rgb4(5, 5, 5), gs::rgb4(9, 8, 6),
                              gs::rgb4(2, 2, 3), gs::rgb4(3, 3, 3)};
    const uint16_t wood[] = {0, gs::rgb4(7, 4, 1), gs::rgb4(9, 6, 2), gs::rgb4(5, 3, 1), gs::rgb4(4, 3, 2),
                             gs::rgb4(11, 9, 4), gs::rgb4(12, 10, 5), gs::rgb4(3, 2, 1), gs::rgb4(2, 1, 1)};
    const uint16_t you[] = {0, gs::rgb4(4, 5, 7), gs::rgb4(6, 7, 9), gs::rgb4(10, 8, 5), gs::rgb4(12, 10, 7),
                            gs::rgb4(3, 3, 3), gs::rgb4(8, 8, 7), gs::rgb4(13, 12, 8), gs::rgb4(3, 3, 4),
                            gs::rgb4(2, 2, 2), gs::rgb4(1, 1, 1)};
    const uint16_t foe[] = {0, gs::rgb4(8, 2, 1), gs::rgb4(6, 4, 2), gs::rgb4(11, 7, 4), gs::rgb4(4, 3, 2),
                            gs::rgb4(9, 8, 6), gs::rgb4(7, 3, 2), gs::rgb4(10, 8, 3), gs::rgb4(3, 2, 2),
                            gs::rgb4(2, 1, 1), gs::rgb4(1, 1, 1)};
    const uint16_t iron[] = {0, gs::rgb4(8, 8, 9), gs::rgb4(5, 5, 6), gs::rgb4(11, 10, 8), gs::rgb4(4, 3, 2)};
    const uint16_t alert[] = {0, gs::rgb4(15, 6, 2), gs::rgb4(15, 13, 4)};
    const uint16_t ok[] = {0, gs::rgb4(8, 14, 6), gs::rgb4(14, 14, 10)};
    const uint16_t night[] = {0, gs::rgb4(8, 8, 10)};

    setPal(vdp, PAL_HUD, hud, 3);
    setPal(vdp, PAL_STONE, stone, 7);
    setPal(vdp, PAL_WOOD, wood, 9);
    setPal(vdp, PAL_YOU, you, 11);
    setPal(vdp, PAL_FOE, foe, 11);
    setPal(vdp, PAL_IRON, iron, 5);
    setPal(vdp, PAL_ALERT, alert, 3);
    setPal(vdp, PAL_OK, ok, 3);
    setPal(vdp, PAL_NIGHT, night, 2);

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    art.wall = gs::uploadMipped(vdp, wallArt());
    art.door = gs::uploadMipped(vdp, doorArt(false));
    art.doorhurt = gs::uploadMipped(vdp, doorArt(true));
    art.bar = gs::uploadMipped(vdp, barArt());
    art.you[0] = gs::uploadMipped(vdp, manArt(0, false));
    art.you[1] = gs::uploadMipped(vdp, manArt(1, false));
    art.foe[0] = gs::uploadMipped(vdp, manArt(0, true));
    art.foe[1] = gs::uploadMipped(vdp, manArt(1, true));
    art.ram = gs::uploadMipped(vdp, ramArt());
    art.spark = gs::uploadMipped(vdp, sparkArt());
    vdp.setFogColor(gs::rgb4(2, 2, 4));
}

}  // namespace door

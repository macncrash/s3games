#include "game/art.h"

namespace scorebell {
namespace {

void ramp(gs::VDP& v, int p, const uint16_t c[16]) {
    for (int i = 0; i < 16; i++) v.setColor(p * 16 + i, c[i]);
}

void palettes(gs::VDP& v) {
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    uint16_t hud[16] = {};
    hud[1] = gs::rgb4(15, 14, 11);
    hud[2] = gs::rgb4(15, 12, 4);
    hud[3] = gs::rgb4(8, 14, 7);
    hud[4] = gs::rgb4(14, 4, 3);
    hud[15] = shadow;
    ramp(v, PAL_HUD, hud);

    uint16_t wood[16] = {};
    wood[1] = gs::rgb4(10, 6, 3);
    wood[2] = gs::rgb4(6, 3, 1);
    wood[3] = gs::rgb4(13, 9, 5);
    wood[4] = gs::rgb4(3, 2, 1);
    ramp(v, PAL_WOOD, wood);

    uint16_t paper[16] = {};
    paper[1] = gs::rgb4(14, 13, 10);
    paper[2] = gs::rgb4(11, 10, 8);
    paper[3] = gs::rgb4(4, 3, 3);
    paper[4] = gs::rgb4(8, 7, 6);
    ramp(v, PAL_PAPER, paper);

    uint16_t ink[16] = {};
    ink[1] = gs::rgb4(2, 2, 4);
    ink[2] = gs::rgb4(5, 5, 8);
    ink[3] = gs::rgb4(1, 1, 2);
    ramp(v, PAL_INK, ink);

    uint16_t brass[16] = {};
    brass[1] = gs::rgb4(14, 11, 3);
    brass[2] = gs::rgb4(15, 14, 7);
    brass[3] = gs::rgb4(8, 5, 1);
    brass[4] = gs::rgb4(4, 3, 1);
    ramp(v, PAL_BRASS, brass);

    uint16_t room[16] = {};
    room[1] = gs::rgb4(12, 9, 6);
    room[2] = gs::rgb4(5, 4, 6);
    room[3] = gs::rgb4(15, 13, 8);
    room[4] = gs::rgb4(3, 3, 5);
    ramp(v, PAL_ROOM, room);

    uint16_t dead[16] = {};
    dead[1] = gs::rgb4(9, 3, 3);
    dead[2] = gs::rgb4(4, 2, 2);
    ramp(v, PAL_DEAD, dead);
}

void font(gs::VDP& v, Art& a) {
    gs::TileAlloc tiles(v);
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) {
                    px[y * 8 + x + 1] = 1;
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
                }
        a.font[c - 32] = tiles.shared(px);
    }
}

gs::Bitmap deskArt() {
    gs::Bitmap b(280, 48);
    b.rect(0, 6, 280, 28, 1);
    b.rect(0, 6, 280, 6, 3);
    b.rect(0, 28, 280, 6, 2);
    b.rect(18, 32, 10, 14, 4);
    b.rect(252, 32, 10, 14, 4);
    return b;
}

gs::Bitmap sheetArt() {
    gs::Bitmap b(230, 88);
    b.rect(0, 0, 230, 88, 1);
    b.rect(4, 4, 222, 80, 2);
    for (int i = 0; i < 5; i++) b.rect(22, 22 + i * 10, 190, 2, 3);
    b.rect(196, 18, 3, 46, 3);
    b.rect(204, 18, 2, 46, 3);
    return b;
}

gs::Bitmap clefArt() {
    gs::Bitmap b(24, 56);
    b.ellipse(12, 36, 7, 9, 1);
    b.rect(14, 8, 3, 36, 1);
    b.ellipse(16, 14, 5, 5, 1);
    b.ellipse(12, 36, 3, 4, 2);
    return b;
}

gs::Bitmap noteArt() {
    gs::Bitmap b(20, 36);
    b.ellipse(7, 28, 6, 4, 1);
    b.rect(12, 3, 3, 26, 1);
    b.rect(12, 3, 6, 3, 1);
    return b;
}

gs::Bitmap barArt() {
    gs::Bitmap b(6, 52);
    b.rect(2, 0, 3, 52, 1);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(40, 36);
    b.ellipse(20, 22, 16, 12, 3);
    b.ellipse(20, 20, 12, 8, 1);
    b.ellipse(16, 16, 4, 3, 2);
    b.rect(17, 4, 6, 8, 3);
    b.rect(14, 2, 12, 4, 4);
    b.rect(8, 30, 24, 3, 4);
    return b;
}

gs::Bitmap clapperArt() {
    gs::Bitmap b(8, 16);
    b.rect(3, 0, 2, 8, 4);
    b.ellipse(4, 12, 3, 3, 3);
    return b;
}

gs::Bitmap standArt() {
    gs::Bitmap b(14, 70);
    b.rect(5, 0, 4, 62, 2);
    b.rect(1, 58, 12, 6, 1);
    b.rect(4, 0, 6, 4, 3);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    palettes(vdp);
    vdp.setFogColor(gs::rgb4(2, 2, 4));
    font(vdp, art);
    art.desk = gs::uploadMipped(vdp, deskArt());
    art.sheet = gs::uploadMipped(vdp, sheetArt());
    art.clef = gs::uploadMipped(vdp, clefArt());
    art.note = gs::uploadMipped(vdp, noteArt());
    art.bar = gs::uploadMipped(vdp, barArt());
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.clapper = gs::uploadMipped(vdp, clapperArt());
    art.stand = gs::uploadMipped(vdp, standArt());
}

}  // namespace scorebell

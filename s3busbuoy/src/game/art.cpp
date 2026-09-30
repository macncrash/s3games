#include "game/art.h"

namespace buoy {
namespace {

void pal(gs::VDP& v, int p, const uint16_t* c, int n) {
    for (int i = 0; i < 16; i++) v.setColor(p * 16 + i, i < n ? c[i] : gs::rgb4(0, 0, 0));
}

void glyphImage(gs::VDP& v, gs::Image& img, char ch) {
    gs::Bitmap b(8, 8);
    const uint8_t* g = gs::glyph(ch);
    for (int y = 0; y < 7; y++)
        for (int x = 0; x < 5; x++)
            if (g[y * 5 + x]) b.set(x + 1, y, 1);
    img = gs::uploadImage(v, b);
}

void busNorth(gs::Bitmap& b) {
    b.rect(10, 4, 12, 36, 2);
    b.rect(11, 6, 10, 8, 4);
    b.rect(11, 18, 10, 8, 4);
    b.rect(10, 30, 12, 4, 3);
    b.rect(8, 10, 3, 8, 5);
    b.rect(21, 10, 3, 8, 5);
    b.rect(8, 26, 3, 8, 5);
    b.rect(21, 26, 3, 8, 5);
    b.rect(12, 2, 3, 3, 6);
    b.rect(17, 2, 3, 3, 6);
    b.outline(1, false);
}

void busDiag(gs::Bitmap& b, bool se) {
    // Nose toward NE (or SE if se): body along the diagonal.
    for (int i = 0; i < 28; i++) {
        int x = se ? 6 + i / 2 : 4 + i / 2;
        int y = se ? 28 - i / 2 : 6 + i / 2;
        b.rect(float(x), float(y), 10, 8, 2);
    }
    b.rect(se ? 8 : 18, se ? 22 : 8, 6, 5, 4);
    b.rect(se ? 16 : 12, se ? 14 : 16, 6, 5, 4);
    b.rect(se ? 20 : 8, se ? 8 : 24, 5, 4, 3);
    b.outline(1, false);
}

void busEast(gs::Bitmap& b) {
    b.rect(6, 10, 36, 12, 2);
    b.rect(28, 12, 8, 8, 4);
    b.rect(16, 12, 8, 8, 4);
    b.rect(8, 10, 4, 12, 3);
    b.rect(12, 8, 8, 3, 5);
    b.rect(26, 8, 8, 3, 5);
    b.rect(12, 21, 8, 3, 5);
    b.rect(26, 21, 8, 3, 5);
    b.rect(40, 12, 3, 3, 6);
    b.rect(40, 17, 3, 3, 6);
    b.outline(1, false);
}

void busSouth(gs::Bitmap& b) {
    b.rect(10, 8, 12, 36, 2);
    b.rect(11, 28, 10, 8, 4);
    b.rect(11, 16, 10, 8, 4);
    b.rect(10, 10, 12, 4, 3);
    b.rect(8, 14, 3, 8, 5);
    b.rect(21, 14, 3, 8, 5);
    b.rect(8, 28, 3, 8, 5);
    b.rect(21, 28, 3, 8, 5);
    b.rect(12, 40, 3, 3, 6);
    b.rect(17, 40, 3, 3, 6);
    b.outline(1, false);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t hud[] = {gs::rgb4(0, 0, 0), gs::rgb4(15, 15, 14), gs::rgb4(15, 12, 3), gs::rgb4(8, 14, 10)};
    const uint16_t bus[] = {gs::rgb4(0, 0, 0), gs::rgb4(1, 1, 2), gs::rgb4(14, 14, 13), gs::rgb4(12, 2, 2),
                            gs::rgb4(6, 10, 13), gs::rgb4(2, 2, 3), gs::rgb4(15, 14, 4)};
    const uint16_t red[] = {gs::rgb4(0, 0, 0), gs::rgb4(13, 2, 2), gs::rgb4(15, 15, 14), gs::rgb4(15, 8, 2),
                            gs::rgb4(4, 4, 5)};
    const uint16_t grn[] = {gs::rgb4(0, 0, 0), gs::rgb4(2, 11, 4), gs::rgb4(15, 15, 14), gs::rgb4(10, 15, 6),
                            gs::rgb4(4, 4, 5)};
    const uint16_t gold[] = {gs::rgb4(0, 0, 0), gs::rgb4(13, 10, 1), gs::rgb4(15, 15, 14), gs::rgb4(15, 13, 4),
                             gs::rgb4(4, 4, 5)};
    const uint16_t dockC[] = {gs::rgb4(0, 0, 0), gs::rgb4(8, 5, 2), gs::rgb4(11, 8, 4), gs::rgb4(5, 3, 1),
                              gs::rgb4(13, 12, 8), gs::rgb4(2, 2, 3)};
    const uint16_t wakeC[] = {gs::rgb4(0, 0, 0), gs::rgb4(12, 14, 15), gs::rgb4(8, 12, 14)};
    const uint16_t rockC[] = {gs::rgb4(0, 0, 0), gs::rgb4(6, 6, 6), gs::rgb4(9, 9, 8), gs::rgb4(3, 5, 3)};
    const uint16_t shedC[] = {gs::rgb4(0, 0, 0), gs::rgb4(10, 3, 2), gs::rgb4(14, 13, 10), gs::rgb4(4, 6, 8),
                              gs::rgb4(2, 2, 2)};
    const uint16_t ok[] = {gs::rgb4(0, 0, 0), gs::rgb4(14, 15, 12), gs::rgb4(4, 14, 8)};
    pal(vdp, PAL_HUD, hud, 4);
    pal(vdp, PAL_BUS, bus, 7);
    pal(vdp, PAL_RED, red, 5);
    pal(vdp, PAL_GREEN, grn, 5);
    pal(vdp, PAL_GOLD, gold, 5);
    pal(vdp, PAL_DOCK, dockC, 6);
    pal(vdp, PAL_WAKE, wakeC, 3);
    pal(vdp, PAL_ROCK, rockC, 4);
    pal(vdp, PAL_SHED, shedC, 5);
    pal(vdp, PAL_OK, ok, 3);
    vdp.setFogColor(gs::rgb4(4, 8, 12));

    for (int i = 0; i < 96; i++) glyphImage(vdp, art.font[i], char(32 + i));

    gs::Bitmap n(32, 48), ne(36, 36), e(48, 32), se(36, 36), s(32, 48);
    busNorth(n);
    busDiag(ne, false);
    busEast(e);
    busDiag(se, true);
    busSouth(s);
    art.bus[0] = gs::uploadImage(vdp, n);
    art.bus[1] = gs::uploadImage(vdp, ne);
    art.bus[2] = gs::uploadImage(vdp, e);
    art.bus[3] = gs::uploadImage(vdp, se);
    art.bus[4] = gs::uploadImage(vdp, s);

    gs::Bitmap mark(16, 28);
    mark.ellipse(8, 16, 6, 6, 1);
    mark.rect(7, 4, 2, 12, 4);
    mark.rect(7, 2, 6, 4, 3);
    mark.ellipse(8, 16, 3, 3, 2);
    art.mark = gs::uploadImage(vdp, mark);

    gs::Bitmap dock(168, 56);
    dock.rect(4, 18, 160, 28, 2);
    for (int i = 0; i < 8; i++) dock.rect(8 + i * 20, 20, 16, 24, i & 1 ? 1 : 2);
    dock.rect(0, 16, 168, 3, 4);
    for (int i = 0; i < 5; i++) dock.rect(16 + i * 34, 46, 6, 8, 5);
    dock.outline(3, false);
    art.dock = gs::uploadImage(vdp, dock);

    gs::Bitmap shed(40, 28);
    shed.poly({{2, 16}, {20, 4}, {38, 16}}, 1);
    shed.rect(6, 16, 28, 10, 2);
    shed.rect(16, 18, 8, 8, 3);
    art.shed = gs::uploadImage(vdp, shed);

    gs::Bitmap wake(20, 10);
    wake.ellipse(10, 5, 9, 3, 1);
    wake.ellipse(8, 5, 4, 2, 2);
    art.wake = gs::uploadImage(vdp, wake);

    gs::Bitmap rock(24, 16);
    rock.ellipse(12, 9, 10, 6, 1);
    rock.ellipse(9, 8, 4, 3, 2);
    rock.rect(4, 13, 16, 2, 3);
    art.rock = gs::uploadImage(vdp, rock);

    gs::Bitmap lamp(8, 8);
    lamp.ellipse(4, 4, 3, 3, 1);
    art.lamp = gs::uploadImage(vdp, lamp);

    // A quiet water tile on plane B, so the picture is in VRAM before the first frame.
    uint8_t tile[64];
    for (int i = 0; i < 64; i++) tile[i] = ((i * 17) % 23) < 2 ? 2 : 1;
    vdp.loadTile(1, tile);
    const uint16_t sea[] = {gs::rgb4(0, 0, 0), gs::rgb4(2, 6, 12), gs::rgb4(5, 10, 14)};
    pal(vdp, 12, sea, 3);
    vdp.B.resize(64, 64);
    vdp.B.clear();
    for (int y = 0; y < 64; y++)
        for (int x = 0; x < 64; x++) vdp.B.set(x, y, gs::entry(1, 12));
}

}  // namespace buoy

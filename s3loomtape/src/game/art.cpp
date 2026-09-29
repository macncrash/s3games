#include "game/art.h"

#include <initializer_list>

namespace loomtape {
namespace {

const Slip kTable[kSlips] = {
    {"YARN", 5, -1, true}, {"WARP", 5, 0, false}, {"FILL", 8, -1, true},
    {"WEFT", 8, 1, false}, {"DENT", 3, -1, true}, {"REED", 3, 2, false},
};

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink, uint16_t edge) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, ink);
    vdp.setColor(pal * 16 + 2, edge);
}

gs::Bitmap frameArt() {
    gs::Bitmap b(156, 150);
    b.rect(0, 0, 12, 150, 2);
    b.rect(144, 0, 12, 150, 2);
    b.rect(0, 0, 156, 10, 3);
    b.rect(0, 140, 156, 10, 1);
    b.rect(64, 0, 28, 8, 4);
    for (int y = 18; y < 138; y += 8) {
        b.set(4, y, 4);
        b.set(150, y, 4);
    }
    return b;
}

gs::Bitmap warpArt() {
    gs::Bitmap b(3, 78);
    b.rect(1, 0, 1, 78, 1);
    b.rect(0, 0, 1, 78, 2);
    for (int y = 2; y < 78; y += 5) b.set(2, y, 3);
    return b;
}

gs::Bitmap shuttleArt() {
    gs::Bitmap b(26, 10);
    b.ellipse(13.f, 5.f, 12.f, 4.2f, 1);
    b.ellipse(13.f, 4.4f, 8.f, 2.2f, 2);
    b.rect(8, 3, 10, 3, 3);
    b.ellipse(8.f, 5.f, 2.f, 2.f, 4);
    b.ellipse(18.f, 5.f, 2.f, 2.f, 4);
    return b;
}

gs::Bitmap reedArt() {
    gs::Bitmap b(108, 10);
    b.rect(0, 0, 108, 2, 1);
    b.rect(0, 8, 108, 2, 1);
    for (int x = 3; x < 106; x += 5) b.rect(float(x), 2, 1, 6, 2);
    b.rect(0, 0, 4, 10, 3);
    b.rect(104, 0, 4, 10, 3);
    return b;
}

gs::Bitmap heddleArt() {
    gs::Bitmap b(7, 14);
    b.rect(3, 0, 1, 14, 1);
    b.rect(1, 5, 5, 5, 2);
    b.set(3, 6, 0);
    b.set(3, 7, 0);
    b.set(3, 8, 0);
    return b;
}

gs::Bitmap pickArt() {
    gs::Bitmap b(12, 6);
    b.rect(0, 1, 12, 4, 1);
    b.rect(0, 0, 12, 1, 2);
    for (int x = 1; x < 12; x += 3) b.set(x, 3, 3);
    return b;
}

gs::Bitmap beamArt() {
    gs::Bitmap b(128, 8);
    b.rect(0, 2, 128, 4, 1);
    b.rect(0, 2, 128, 1, 2);
    b.ellipse(8.f, 4.f, 6.f, 3.2f, 3);
    b.ellipse(120.f, 4.f, 6.f, 3.2f, 3);
    return b;
}

gs::Bitmap drawerArt() {
    gs::Bitmap b(52, 70);
    b.rect(0, 0, 52, 70, 2);
    b.rect(3, 4, 46, 18, 1);
    b.rect(3, 26, 46, 18, 1);
    b.rect(3, 48, 46, 18, 1);
    b.rect(20, 10, 12, 4, 3);
    b.rect(20, 32, 12, 4, 3);
    b.rect(20, 54, 12, 4, 3);
    return b;
}

gs::Bitmap spoolArt() {
    gs::Bitmap b(14, 14);
    b.ellipse(7.f, 7.f, 6.f, 6.f, 1);
    b.ellipse(7.f, 7.f, 2.2f, 2.2f, 3);
    b.rect(6, 1, 2, 12, 2);
    return b;
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
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 2;
                }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

}  // namespace

const Slip& slipAt(int i) { return kTable[i < 0 ? 0 : i % kSlips]; }

const char* tapeName(int i) {
    if (i < 0 || i >= kTapeN) return "";
    for (const Slip& s : kTable)
        if (s.tape == i) return s.name;
    return "";
}

int tapePay(int i) {
    if (i < 0 || i >= kTapeN) return 0;
    for (const Slip& s : kTable)
        if (s.tape == i) return s.pay;
    return 0;
}

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(3, 2, 1), gs::rgb4(7, 4, 2), gs::rgb4(11, 7, 3), gs::rgb4(5, 4, 3),
                           gs::rgb4(13, 11, 8)});
    setPal(vdp, PAL_WARP, {0, gs::rgb4(13, 12, 9), gs::rgb4(15, 14, 11), gs::rgb4(8, 7, 5)});
    setPal(vdp, PAL_SHUTTLE,
           {0, gs::rgb4(4, 2, 1), gs::rgb4(9, 6, 3), gs::rgb4(14, 11, 6), gs::rgb4(6, 1, 3), gs::rgb4(12, 3, 4)});
    setPal(vdp, PAL_CLOTH, {0, gs::rgb4(8, 3, 3), gs::rgb4(12, 8, 3), gs::rgb4(3, 5, 10), gs::rgb4(14, 12, 8)});
    textPal(vdp, PAL_INK, gs::rgb4(15, 14, 12), gs::rgb4(2, 1, 1));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 13, 5), gs::rgb4(3, 2, 0));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 5, 4), gs::rgb4(3, 0, 0));
    textPal(vdp, PAL_DIM, gs::rgb4(7, 6, 5), gs::rgb4(1, 1, 1));
    setPal(vdp, PAL_TAPE, {0, gs::rgb4(12, 10, 6), gs::rgb4(6, 4, 2), gs::rgb4(2, 2, 3), gs::rgb4(14, 12, 4)});
    setPal(vdp, PAL_DRAWER, {0, gs::rgb4(5, 3, 2), gs::rgb4(9, 6, 3), gs::rgb4(14, 11, 6), gs::rgb4(2, 1, 1)});

    loadFont(vdp, art);
    art.frame = gs::uploadImage(vdp, frameArt());
    art.warp = gs::uploadImage(vdp, warpArt());
    art.shuttle = gs::uploadImage(vdp, shuttleArt());
    art.reed = gs::uploadImage(vdp, reedArt());
    art.heddle = gs::uploadImage(vdp, heddleArt());
    art.pick = gs::uploadImage(vdp, pickArt());
    art.beam = gs::uploadImage(vdp, beamArt());
    art.drawer = gs::uploadImage(vdp, drawerArt());
    art.spool = gs::uploadImage(vdp, spoolArt());
}

}  // namespace loomtape

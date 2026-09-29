#include "game/art.h"

namespace chefmark {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

gs::Bitmap chefArt() {
    gs::Bitmap b(40, 56);
    b.rect(12, 1, 16, 6, 4);
    b.ellipse(20, 10, 12, 5, 4);
    b.ellipse(20, 22, 8, 8, 2);
    b.rect(15, 20, 3, 2, 1);
    b.rect(22, 20, 3, 2, 1);
    b.rect(17, 26, 6, 2, 3);
    b.rect(11, 30, 18, 18, 4);
    b.rect(14, 32, 12, 4, 6);
    b.rect(6, 34, 6, 12, 2);
    b.rect(28, 34, 6, 12, 2);
    b.rect(13, 48, 6, 6, 5);
    b.rect(21, 48, 6, 6, 5);
    b.outline(1, false);
    return b;
}

gs::Bitmap steakArt(bool burnt) {
    gs::Bitmap b(48, 36);
    b.ellipse(24, 28, 18, 6, 8);
    b.ellipse(24, 26, 14, 4, 2);
    if (burnt) {
        b.ellipse(24, 16, 14, 8, 5);
        b.ellipse(18, 15, 4, 3, 1);
        b.ellipse(30, 17, 3, 2, 1);
    } else {
        b.ellipse(24, 16, 14, 8, 4);
        b.ellipse(22, 15, 9, 5, 3);
        b.rect(14, 14, 14, 2, 6);
        b.rect(16, 18, 10, 2, 6);
        b.ellipse(32, 12, 3, 2, 7);
    }
    b.outline(1, false);
    return b;
}

gs::Bitmap panArt() {
    gs::Bitmap b(72, 22);
    b.ellipse(30, 12, 26, 8, 3);
    b.ellipse(30, 11, 20, 5, 2);
    b.rect(52, 9, 16, 5, 4);
    b.rect(66, 8, 4, 7, 5);
    b.outline(1, false);
    return b;
}

gs::Bitmap flameArt(int frame) {
    gs::Bitmap b(28, 22);
    float h = frame == 0 ? 12.f : frame == 1 ? 16.f : 10.f;
    b.ellipse(14, 18, 9, 3, 4);
    b.ellipse(14, 16 - h * 0.25f, 6, h * 0.4f, 3);
    b.ellipse(10, 14, 3, h * 0.28f, 2);
    b.ellipse(18, 15, 3, h * 0.25f, 3);
    b.ellipse(14, 9, 2, 3, 1);
    return b;
}

gs::Bitmap ticketArt() {
    gs::Bitmap b(64, 28);
    b.rect(2, 2, 60, 24, 2);
    b.rect(2, 2, 60, 8, 4);
    b.rect(8, 14, 36, 2, 3);
    b.rect(8, 19, 28, 2, 3);
    b.outline(1, false);
    return b;
}

gs::Bitmap markArt() {
    gs::Bitmap b(28, 28);
    b.line(3, 15, 11, 23, 1, 3);
    b.line(11, 23, 25, 5, 1, 3);
    return b;
}

gs::Bitmap solidArt() {
    gs::Bitmap b(4, 4);
    b.rect(0, 0, 4, 4, 1);
    return b;
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
    (void)vdp;
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

gs::Mipped say(gs::VDP& vdp, const char* s) {
    gs::TextStyle st{2, 1, 0, 0, 1};
    return gs::uploadMipped(vdp, gs::textBitmap(s, st));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 2), gs::rgb4(15, 15, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 3, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(4, 14, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_CHEF,
           {0, gs::rgb4(1, 1, 1), gs::rgb4(15, 12, 8), gs::rgb4(10, 4, 3), gs::rgb4(15, 15, 15), gs::rgb4(3, 3, 6),
            gs::rgb4(13, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_FOOD,
           {0, gs::rgb4(2, 1, 1), gs::rgb4(15, 15, 13), gs::rgb4(12, 6, 3), gs::rgb4(9, 3, 2), gs::rgb4(3, 2, 1),
            gs::rgb4(14, 12, 8), gs::rgb4(4, 10, 3), gs::rgb4(12, 13, 14), 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_STEEL, {0, gs::rgb4(1, 1, 2), gs::rgb4(14, 15, 15), gs::rgb4(7, 8, 9), gs::rgb4(4, 4, 5), gs::rgb4(10, 6, 3)});
    setPal(vdp, PAL_FIRE, {0, gs::rgb4(15, 15, 12), gs::rgb4(15, 10, 2), gs::rgb4(15, 4, 1), gs::rgb4(8, 2, 1)});
    setPal(vdp, PAL_PAPER, {0, gs::rgb4(2, 1, 1), gs::rgb4(15, 14, 10), gs::rgb4(8, 6, 4), gs::rgb4(13, 3, 2)});
    setPal(vdp, PAL_INK, {0, gs::rgb4(8, 9, 10)});

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, tiles, art);
    art.title = say(vdp, "S3 CHEFMARK");
    art.done = say(vdp, "FINISHED MARK");
    art.raw = say(vdp, "TOO RAW");
    art.burn = say(vdp, "BURNED");
    art.chef = gs::uploadMipped(vdp, chefArt());
    art.steak = gs::uploadMipped(vdp, steakArt(false));
    art.charred = gs::uploadMipped(vdp, steakArt(true));
    art.pan = gs::uploadMipped(vdp, panArt());
    for (int i = 0; i < 3; i++) art.flame[i] = gs::uploadMipped(vdp, flameArt(i));
    art.ticket = gs::uploadMipped(vdp, ticketArt());
    art.mark = gs::uploadMipped(vdp, markArt());
    art.solid = gs::uploadMipped(vdp, solidArt());
}

}  // namespace chefmark

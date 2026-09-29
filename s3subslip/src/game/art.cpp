#include "game/art.h"

namespace slip {
namespace {

void loadFont(gs::VDP& vdp, Art& art) {
    gs::TileAlloc tiles(vdp, 1);
    for (int i = 0; i < 96; i++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(32 + i));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) px[y * 8 + x + 1] = 1;
        art.font[i] = tiles.shared(px);
    }
}

void paintSub(gs::Bitmap& b) {
    b.ellipse(48, 24, 40, 11, 2);
    b.ellipse(46, 22, 36, 8, 3);
    b.rect(18, 16, 58, 5, 5);
    b.rect(28, 8, 16, 12, 2);
    b.rect(30, 6, 10, 8, 3);
    b.rect(38, 3, 2, 8, 7);
    b.ellipse(34, 12, 2, 2, 4);
    b.ellipse(62, 22, 3, 2, 8);
    b.rect(8, 20, 6, 3, 6);
    b.line(6, 18, 6, 28, 6, 2);
    b.ellipse(14, 23, 2, 2, 9);
    b.outline(7, true);
}

void paintProp(gs::Bitmap& b) {
    b.line(8, 2, 8, 14, 6, 2);
    b.line(2, 8, 14, 8, 6, 2);
    b.ellipse(8, 8, 2, 2, 9);
}

void paintPiling(gs::Bitmap& b) {
    b.rect(4, 0, 8, 78, 2);
    b.rect(4, 0, 3, 78, 3);
    b.rect(3, 0, 10, 6, 4);
    b.rect(2, 70, 12, 6, 1);
    for (int y = 14; y < 70; y += 12) b.rect(4, y, 8, 2, 5);
}

void paintQuay(gs::Bitmap& b) {
    b.rect(0, 8, 96, 24, 2);
    b.rect(0, 8, 96, 6, 3);
    b.rect(0, 26, 96, 6, 1);
    for (int x = 4; x < 96; x += 16) b.rect(x, 14, 8, 4, 4);
    b.rect(0, 0, 96, 8, 5);
    b.rect(8, 1, 10, 4, 6);
}

void paintBuoy(gs::Bitmap& b) {
    b.ellipse(12, 10, 8, 8, 2);
    b.ellipse(12, 9, 5, 5, 3);
    b.rect(10, 16, 4, 8, 1);
    b.rect(4, 22, 16, 3, 4);
}

void paintBubble(gs::Bitmap& b) {
    b.ellipse(4, 4, 3, 3, 3);
    b.set(3, 3, 1);
}

void paintKelp(gs::Bitmap& b) {
    b.line(6, 40, 4, 24, 2, 2);
    b.line(4, 24, 8, 10, 3, 2);
    b.line(8, 10, 5, 0, 2, 2);
    b.ellipse(9, 16, 3, 2, 4);
}

void paintGull(gs::Bitmap& b) {
    b.line(1, 6, 8, 3, 1, 1);
    b.line(8, 3, 15, 6, 1, 1);
    b.set(8, 4, 1);
}

void paintLamp(gs::Bitmap& b) {
    b.rect(6, 8, 4, 22, 2);
    b.ellipse(8, 6, 5, 4, 3);
    b.rect(3, 28, 10, 3, 1);
}

void paintSkyline(gs::Bitmap& b) {
    for (int x = 0; x < b.w; x++) {
        int h = 8 + ((x * 17) % 23) + ((x / 40) % 2) * 10;
        if ((x / 28) % 5 == 0) h += 14;
        for (int y = b.h - h; y < b.h; y++) b.set(x, y, 2);
        if ((x / 28) % 5 == 0) b.rect(float(x + 6), float(b.h - h - 8), 2, 8, 3);
        for (int y = b.h - h + 4; y < b.h - 2; y += 6)
            if (((x + y) / 5) % 3 == 0) b.set(x, y, 4);
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    auto pal = [&](int p, int i, int r, int g, int b) { vdp.setColor(p * 16 + i, gs::rgb4(r, g, b)); };

    pal(PAL_HUD, 1, 15, 15, 14);
    pal(PAL_HUD, 2, 15, 12, 4);
    pal(PAL_HUD, 3, 15, 4, 3);
    pal(PAL_HUD, 4, 6, 15, 8);
    pal(PAL_HUD, 5, 8, 10, 12);

    pal(PAL_SEA, 1, 14, 15, 15);
    pal(PAL_SEA, 2, 4, 8, 12);
    pal(PAL_SEA, 3, 8, 13, 15);
    pal(PAL_SEA, 4, 2, 5, 8);

    pal(PAL_SUB, 1, 2, 4, 3);
    pal(PAL_SUB, 2, 3, 7, 5);
    pal(PAL_SUB, 3, 6, 11, 8);
    pal(PAL_SUB, 4, 12, 15, 14);
    pal(PAL_SUB, 5, 12, 8, 3);
    pal(PAL_SUB, 6, 8, 9, 7);
    pal(PAL_SUB, 7, 1, 2, 1);
    pal(PAL_SUB, 8, 15, 3, 2);
    pal(PAL_SUB, 9, 14, 14, 12);

    pal(PAL_QUAY, 1, 4, 4, 5);
    pal(PAL_QUAY, 2, 8, 8, 9);
    pal(PAL_QUAY, 3, 12, 12, 11);
    pal(PAL_QUAY, 4, 6, 6, 6);
    pal(PAL_QUAY, 5, 5, 6, 6);
    pal(PAL_QUAY, 6, 14, 10, 3);

    pal(PAL_BUOY, 1, 6, 3, 2);
    pal(PAL_BUOY, 2, 15, 5, 2);
    pal(PAL_BUOY, 3, 15, 12, 4);
    pal(PAL_BUOY, 4, 3, 3, 3);

    pal(PAL_LAMP, 1, 5, 5, 4);
    pal(PAL_LAMP, 2, 7, 7, 6);
    pal(PAL_LAMP, 3, 15, 13, 5);

    pal(PAL_KELP, 1, 2, 5, 2);
    pal(PAL_KELP, 2, 3, 8, 3);
    pal(PAL_KELP, 3, 6, 12, 5);
    pal(PAL_KELP, 4, 10, 14, 6);

    pal(PAL_FX, 1, 15, 15, 15);
    pal(PAL_FX, 2, 10, 14, 15);
    pal(PAL_FX, 3, 14, 15, 15);

    vdp.setFogColor(gs::rgb4(2, 4, 7));

    loadFont(vdp, art);

    gs::Bitmap sub(96, 40);
    paintSub(sub);
    art.sub = gs::uploadMipped(vdp, sub);

    gs::Bitmap prop(16, 16);
    paintProp(prop);
    art.prop = gs::uploadMipped(vdp, prop);

    gs::Bitmap pil(16, 80);
    paintPiling(pil);
    art.piling = gs::uploadMipped(vdp, pil);

    gs::Bitmap quay(96, 32);
    paintQuay(quay);
    art.quay = gs::uploadMipped(vdp, quay);

    gs::Bitmap buoy(24, 28);
    paintBuoy(buoy);
    art.buoy = gs::uploadMipped(vdp, buoy);

    gs::Bitmap bub(8, 8);
    paintBubble(bub);
    art.bubble = gs::uploadMipped(vdp, bub);

    gs::Bitmap kelp(12, 42);
    paintKelp(kelp);
    art.kelp = gs::uploadMipped(vdp, kelp);

    gs::Bitmap gull(16, 8);
    paintGull(gull);
    art.gull = gs::uploadMipped(vdp, gull);

    gs::Bitmap lamp(16, 32);
    paintLamp(lamp);
    art.lamp = gs::uploadMipped(vdp, lamp);

    gs::Bitmap sky(256, 48);
    paintSkyline(sky);
    art.skyline = gs::uploadImage(vdp, sky);
}

}  // namespace slip

#include "art.h"

#include <cstdint>

namespace bunker {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

uint32_t hash2(int x, int y) {
    uint32_t h = uint32_t(x) * 374761393u + uint32_t(y) * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
    gs::TextStyle big{3, 1, 0, 15, 1};
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
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

void paintRoom(gs::Bitmap& b) {
    b.rect(0, 0, 320, 224, 2);
    b.rect(0, 0, 320, 18, 4);
    b.rect(0, 16, 320, 3, 1);
    b.rect(0, 198, 320, 26, 5);
    b.rect(0, 198, 320, 3, 1);
    // Lamp side of the wall is warmer than the far jamb.
    b.rect(0, 22, 48, 120, 3);
    b.rect(DOOR_X - 5, DOOR_Y - 5, DOOR_W + 10, DOOR_H + 8, 1);
    b.rect(DOOR_X - 5, DOOR_Y, 5, DOOR_H, 3);
    for (int y = 0; y < DOOR_H; y++) {
        int c = y < int(DOOR_H * 0.78f) ? 13 : 14;
        b.rect(DOOR_X, DOOR_Y + y, DOOR_W, 1, c);
    }
    for (int y = SLIT_Y; y < SLIT_Y + SLIT_H; y += 6) b.rect(SLIT_X + 8, y, SLIT_W - 16, 1, 14);
    b.rect(SLIT_X + SLIT_W / 2 - 1, SLIT_Y + 10, 2, 8, 8);
    for (int y = 24; y < 190; y += 3)
        for (int x = 4; x < 316; x += 5) {
            if (x > DOOR_X - 2 && x < DOOR_X + DOOR_W + 2 && y > DOOR_Y - 2 && y < DOOR_Y + DOOR_H) continue;
            uint32_t h = hash2(x, y);
            if ((h % 17) == 0) b.set(x, y, h % 2 ? 1 : 12);
        }
    b.rect(36, 22, 2, 168, 1);
    b.rect(286, 22, 2, 160, 1);
    b.rect(12, 8, 150, 3, 1);
    b.rect(180, 7, 90, 3, 1);
    b.line(32, 4, 32, 26, 1, 1.2f);
    b.rect(22, 26, 20, 6, 1);
    b.ellipse(32, 38, 9, 7, 8);
    b.ellipse(32, 38, 4, 3, 9);
    b.ellipse(48, 214, 36, 10, 6);
    b.rect(8, 128, 42, 30, 7);
    b.rect(10, 130, 38, 7, 15);
    b.rect(10, 142, 38, 5, 15);
    b.line(12, 132, 46, 154, 1, 1.2f);
    b.line(46, 132, 12, 154, 1, 1.2f);
    b.ellipse(26, 176, 16, 10, 10);
    b.ellipse(46, 188, 14, 9, 10);
    b.ellipse(20, 198, 15, 9, 10);
    b.rect(258, 118, 36, 24, 1);
    b.rect(262, 122, 20, 12, 11);
    b.rect(286, 126, 4, 4, 12);
    b.rect(274, 104, 2, 16, 1);
    b.ellipse(292, 170, 16, 10, 10);
    b.ellipse(268, 184, 14, 9, 10);
    b.ellipse(300, 196, 12, 8, 10);
    gs::Bitmap label = gs::textBitmap("B-1", {2, 15, 0, 0, 1});
    b.blit(label, 12, 52);
    b.ellipse(16, 96, 7, 16, 1);
    b.rect(300, 64, 12, 28, 1);
}

void paintDoor(gs::Bitmap& b) {
    b.rect(0, 0, DOOR_W, DOOR_H, 2);
    b.rect(5, 5, DOOR_W - 10, DOOR_H - 10, 3);
    b.rect(8, 8, DOOR_W - 16, DOOR_H - 16, 2);
    for (int y = 10; y < DOOR_H - 8; y += 7) b.rect(10, y, DOOR_W - 20, 1, 6);
    b.rect(0, 0, 8, DOOR_H, 1);
    b.rect(DOOR_W - 7, 0, 7, DOOR_H, 1);
    b.rect(0, 0, DOOR_W, 6, 10);
    b.rect(0, DOOR_H - 6, DOOR_W, 6, 10);
    // Lamp catches the left stile.
    b.rect(8, 8, 5, DOOR_H - 16, 9);
    for (int i = 0; i < 8; i++) {
        int y = 14 + i * 22;
        b.rect(i % 2 ? 0 : 4, y, 8, 8, i % 2 ? 7 : 8);
    }
    for (int y = 16; y < DOOR_H - 10; y += 18) {
        b.ellipse(16, float(y), 2.4f, 2.4f, 4);
        b.ellipse(DOOR_W - 16, float(y), 2.4f, 2.4f, 4);
        b.set(16, y, 1);
        b.set(DOOR_W - 16, y, 1);
    }
    int lx = SLIT_X - DOOR_X;
    int ly = SLIT_Y - DOOR_Y;
    b.rect(lx - 3, ly - 4, SLIT_W + 6, 4, 1);
    b.rect(lx - 3, ly + SLIT_H, SLIT_W + 6, 4, 1);
    b.rect(lx - 4, ly, 4, SLIT_H, 1);
    b.rect(lx + SLIT_W, ly, 4, SLIT_H, 1);
    // Brackets the locking bar sits in.
    int bx = BAR_X - DOOR_X;
    int by = BAR_Y - DOOR_Y;
    b.rect(bx - 6, by - 3, 8, BAR_H + 6, 1);
    b.rect(bx + BAR_W - 2, by - 3, 8, BAR_H + 6, 1);
    b.ellipse(float(DOOR_W / 2), float(by + 28), 11, 11, 1);
    b.ellipse(float(DOOR_W / 2), float(by + 28), 7, 7, 6);
    b.rect(DOOR_W / 2 - 1, by + 20, 3, 16, 4);
    b.rect(DOOR_W / 2 - 8, by + 26, 16, 3, 4);
    // The slit is a real hole: the trench and anyone in it show through.
    b.rect(float(lx), float(ly), float(SLIT_W), float(SLIT_H), 0);
}

gs::Bitmap barArt() {
    gs::Bitmap b(BAR_W, BAR_H + 4);
    b.rect(0, 2, BAR_W, BAR_H, 1);
    b.rect(2, 3, BAR_W - 4, BAR_H - 4, 2);
    b.rect(2, 3, BAR_W - 4, 2, 3);
    for (int x = 8; x < BAR_W - 6; x += 14) b.rect(x, 4, 3, BAR_H - 4, 4);
    b.rect(BAR_W / 2 - 6, 3, 12, BAR_H - 2, 9);
    return b;
}

gs::Bitmap crackArt() {
    gs::Bitmap b(40, 64);
    b.line(22, 2, 16, 20, 6, 1.5f);
    b.line(16, 20, 26, 36, 6, 1.4f);
    b.line(26, 36, 14, 62, 6, 1.3f);
    b.line(16, 20, 6, 28, 6, 1.1f);
    b.line(26, 34, 36, 46, 6, 1.1f);
    return b;
}

// Shared index roles: 1 body, 2 shade, 3 skin, 4 helmet, 5 visor, 6 weapon, 7 boot, 8 lamp, 9 outline.
gs::Bitmap person(int kind, int step) {
    const bool crawl = kind == 1;
    const bool ram = kind == 2;
    const int W = ram ? 56 : 44;
    const int H = crawl ? 36 : ram ? 70 : 60;
    gs::Bitmap b(W, H);
    const float cx = W * 0.5f;
    if (crawl) {
        b.ellipse(cx, 22, 16, 9, 1);
        b.ellipse(cx - 2, 20, 12, 6, 2);
        b.ellipse(cx + 8, 16, 7, 6, 4);
        b.rect(cx + 4, 15, 8, 3, 5);
        b.rect(cx - 16, 20, 14, 3, 6);
        b.rect(8, 26, 8, 5, 7);
        b.rect(22 + step * 3, 27, 8, 4, 7);
        b.outline(9, false);
        return b;
    }
    b.ellipse(cx, 13, ram ? 13.f : 10.f, ram ? 10.f : 8.f, 4);
    b.rect(cx - 8, 12, ram ? 18.f : 16.f, 4, 5);
    if (ram) b.rect(cx + 6, 6, 5, 4, 8);
    b.rect(cx - (ram ? 14.f : 10.f), 22, ram ? 28.f : 20.f, ram ? 28.f : 22.f, 1);
    b.rect(cx - (ram ? 14.f : 10.f), 22, 6, ram ? 28.f : 22.f, 2);
    if (ram) b.rect(cx - 16, 26, 10, 22, 2);
    b.rect(cx - 4, 24, 6, 5, 3);
    b.rect(cx - (ram ? 20.f : 16.f), 28, 8, 12, 1);
    b.rect(6, 32, ram ? 44.f : 32.f, 3, 6);
    if (ram) b.rect(4, 29, 48, 6, 6);
    int lx = step ? 12 : 16;
    int rx = step ? 30 : 24;
    if (ram) {
        lx = step ? 14 : 20;
        rx = step ? 34 : 28;
    }
    b.rect(float(lx), 46, 7, 12, 1);
    b.rect(float(rx), 46, 7, 12, 2);
    b.rect(float(lx - 1), H - 7, 9, 4, 7);
    b.rect(float(rx - 1), H - 7, 9, 4, 7);
    b.outline(9, false);
    return b;
}

gs::Bitmap rifleArt() {
    gs::Bitmap b(104, 78);
    b.rect(46, 4, 12, 46, 5);
    b.rect(48, 2, 8, 18, 6);
    b.rect(42, 16, 20, 5, 6);
    b.rect(44, 40, 16, 22, 4);
    b.rect(38, 52, 28, 10, 4);
    b.rect(30, 58, 18, 14, 2);
    b.rect(58, 58, 22, 14, 2);
    b.ellipse(34, 62, 9, 7, 1);
    b.ellipse(74, 62, 10, 8, 1);
    b.rect(30, 66, 10, 6, 7);
    b.rect(70, 67, 10, 6, 7);
    b.rect(49, 8, 6, 3, 8);
    b.outline(9, false);
    return b;
}

gs::Bitmap sightArt() {
    gs::Bitmap b(19, 19);
    b.rect(0, 0, 6, 2, 1);
    b.rect(0, 0, 2, 6, 1);
    b.rect(13, 0, 6, 2, 1);
    b.rect(17, 0, 2, 6, 1);
    b.rect(0, 17, 6, 2, 1);
    b.rect(0, 13, 2, 6, 1);
    b.rect(13, 17, 6, 2, 1);
    b.rect(17, 13, 2, 6, 1);
    b.rect(8, 2, 2, 4, 1);
    b.rect(8, 13, 2, 4, 1);
    b.rect(2, 8, 4, 2, 1);
    b.rect(13, 8, 4, 2, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 14, 13), gs::rgb4(8, 8, 9), gs::rgb4(15, 14, 10), gs::rgb4(15, 5, 3),
                          gs::rgb4(8, 13, 7), gs::rgb4(6, 8, 10), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_ROOM,
           {0, gs::rgb4(3, 3, 4), gs::rgb4(5, 5, 6), gs::rgb4(8, 7, 6), gs::rgb4(2, 2, 3), gs::rgb4(3, 2, 2),
            gs::rgb4(6, 4, 3), gs::rgb4(9, 6, 3), gs::rgb4(15, 13, 5), gs::rgb4(15, 15, 11), gs::rgb4(7, 7, 4),
            gs::rgb4(3, 7, 4), gs::rgb4(8, 14, 7), gs::rgb4(1, 1, 2), gs::rgb4(3, 3, 2), gs::rgb4(12, 11, 8)});
    setPal(vdp, PAL_DOOR,
           {0, gs::rgb4(2, 3, 4), gs::rgb4(5, 6, 7), gs::rgb4(8, 9, 10), gs::rgb4(12, 12, 11), gs::rgb4(7, 3, 2),
            gs::rgb4(3, 4, 5), gs::rgb4(13, 11, 2), gs::rgb4(1, 1, 1), gs::rgb4(14, 12, 7), gs::rgb4(4, 4, 5), 0, 0,
            0, 0, shadow});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 12, 4), gs::rgb4(10, 7, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 2, 0)});
    setPal(vdp, PAL_YOU, {0, gs::rgb4(5, 4, 3), gs::rgb4(3, 4, 3), gs::rgb4(2, 2, 2), gs::rgb4(8, 5, 2), gs::rgb4(11, 11, 10),
                          gs::rgb4(4, 4, 5), gs::rgb4(12, 8, 6), gs::rgb4(14, 12, 6), gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FOE, {0, gs::rgb4(4, 5, 3), gs::rgb4(2, 3, 2), gs::rgb4(11, 8, 6), gs::rgb4(7, 8, 6), gs::rgb4(1, 1, 1),
                          gs::rgb4(3, 3, 4), gs::rgb4(2, 2, 2), gs::rgb4(8, 7, 3), gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BRUTE,
           {0, gs::rgb4(6, 7, 8), gs::rgb4(3, 3, 4), gs::rgb4(10, 8, 6), gs::rgb4(8, 9, 10), gs::rgb4(2, 5, 7),
            gs::rgb4(5, 5, 4), gs::rgb4(2, 2, 2), gs::rgb4(15, 3, 2), gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 15, 14), gs::rgb4(15, 13, 4), gs::rgb4(15, 15, 15), gs::rgb4(15, 8, 2),
                         gs::rgb4(9, 8, 6), gs::rgb4(2, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(8, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 0, 0)});
    setPal(vdp, PAL_OK, {0, gs::rgb4(8, 15, 7), gs::rgb4(2, 6, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(0, 2, 1)});

    vdp.setFogColor(gs::rgb4(1, 1, 2));
    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    gs::Bitmap room(gs::SCREEN_W, gs::SCREEN_H);
    paintRoom(room);
    gs::bitmapToPlane(tiles, vdp.B, 0, 0, room, PAL_ROOM);
    vdp.A.enabled = false;
    vdp.B.enabled = true;
    vdp.hudEnabled = true;

    gs::Bitmap door(DOOR_W, DOOR_H);
    paintDoor(door);
    art.door = gs::uploadMipped(vdp, door);
    art.bar = gs::uploadMipped(vdp, barArt());
    art.crack = gs::uploadMipped(vdp, crackArt());
    art.walker[0] = gs::uploadMipped(vdp, person(0, 0));
    art.walker[1] = gs::uploadMipped(vdp, person(0, 1));
    art.ducker[0] = gs::uploadMipped(vdp, person(1, 0));
    art.ducker[1] = gs::uploadMipped(vdp, person(1, 1));
    art.breacher[0] = gs::uploadMipped(vdp, person(2, 0));
    art.breacher[1] = gs::uploadMipped(vdp, person(2, 1));
    art.rifle = gs::uploadMipped(vdp, rifleArt());
    art.sight = gs::uploadMipped(vdp, sightArt());
    gs::Bitmap flash(16, 16);
    flash.ellipse(8, 8, 7, 5, 2);
    flash.ellipse(8, 8, 3, 2, 3);
    art.flash = gs::uploadMipped(vdp, flash);
    gs::Bitmap spark(8, 8);
    spark.line(1, 6, 6, 1, 4, 1.4f);
    spark.line(2, 2, 7, 6, 2, 1.2f);
    art.spark = gs::uploadMipped(vdp, spark);
    gs::Bitmap dust(22, 22);
    dust.ellipse(11, 12, 9, 7, 5);
    dust.ellipse(9, 10, 5, 4, 1);
    art.dust = gs::uploadMipped(vdp, dust);
}

}  // namespace bunker

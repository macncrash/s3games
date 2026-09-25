#include "game/art.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>

namespace bus {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
    while (i < 15) vdp.setColor(pal * 16 + i++, 0);
    vdp.setColor(pal * 16 + 0, 0);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

void solid(uint8_t* px, int c) {
    for (int i = 0; i < 64; i++) px[i] = uint8_t(c);
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& art) {
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (!g[y * 5 + x]) continue;
                px[y * 8 + x + 1] = 1;
                if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
            }
        }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        art.font[c - 32] = t;
    }
}

void paintRoad(gs::VDP& vdp, gs::TileAlloc& tiles) {
    uint8_t wall[64], wallB[64], walk[64], curb[64], asphalt[64], edgeL[64], edgeR[64], dash[64];
    solid(wall, 6);
    solid(wallB, 10);
    for (int y = 1; y <= 2; y++)
        for (int x : {2, 5}) {
            wall[y * 8 + x] = 7;
            wallB[y * 8 + x] = 7;
        }
    for (int y = 4; y <= 5; y++)
        for (int x : {2, 5}) {
            wall[y * 8 + x] = 7;
            wallB[y * 8 + x] = 7;
        }
    wall[0 * 8 + 1] = 11;
    wall[0 * 8 + 2] = 11;
    wall[0 * 8 + 3] = 11;
    solid(walk, 4);
    for (int x = 0; x < 8; x++) walk[7 * 8 + x] = 5;
    solid(curb, 8);
    solid(asphalt, 1);
    asphalt[2 * 8 + 6] = 2;
    asphalt[5 * 8 + 1] = 2;
    asphalt[6 * 8 + 4] = 2;
    solid(edgeL, 1);
    solid(edgeR, 1);
    for (int y = 0; y < 8; y++) {
        edgeL[y * 8 + 0] = 9;
        edgeR[y * 8 + 7] = 9;
    }
    edgeL[3 * 8 + 5] = 2;
    edgeR[6 * 8 + 2] = 2;
    solid(dash, 1);
    for (int y = 1; y < 7; y++) {
        dash[y * 8 + 3] = 3;
        dash[y * 8 + 4] = 3;
    }

    int tWall = tiles.alloc(1);
    int tWallB = tiles.alloc(1);
    int tWalk = tiles.alloc(1);
    int tCurb = tiles.alloc(1);
    int tAsp = tiles.alloc(1);
    int tEdgeL = tiles.alloc(1);
    int tEdgeR = tiles.alloc(1);
    int tDash = tiles.alloc(1);
    vdp.loadTile(tWall, wall);
    vdp.loadTile(tWallB, wallB);
    vdp.loadTile(tWalk, walk);
    vdp.loadTile(tCurb, curb);
    vdp.loadTile(tAsp, asphalt);
    vdp.loadTile(tEdgeL, edgeL);
    vdp.loadTile(tEdgeR, edgeR);
    vdp.loadTile(tDash, dash);

    for (int cy = 0; cy < vdp.B.h; cy++) {
        bool dashed = (cy % 8) < 3;
        for (int cx = 0; cx < vdp.B.w; cx++) {
            int id = ((cx + cy) & 1) ? tWallB : tWall;
            if (cx == 3 || cx == 36) id = tWalk;
            else if (cx == 4 || cx == 35) id = tCurb;
            else if (cx == 5) id = tEdgeL;
            else if (cx == 34) id = tEdgeR;
            else if (cx >= 6 && cx <= 33) id = (dashed && (cx == 19 || cx == 20)) ? tDash : tAsp;
            vdp.B.set(cx, cy, gs::entry(id, PAL_ROAD));
        }
    }
}

// Body is 28x56, centered in a 48x72 sheet, so a sprite drawn at height 72
// matches the collision box exactly. Front of the bus is the top of the sheet.
gs::Bitmap paintBus(int yaw, int door) {
    gs::Bitmap b(48, 72);
    auto plot = [&](int x, int y, int c) {
        float t = (y - 8) / 56.f;
        if (t < 0.f) t = 0.f;
        if (t > 1.f) t = 1.f;
        int sh = int(std::lround((0.5f - t) * float(yaw) * 5.f));
        b.set(x + sh, y, c);
    };
    auto rect = [&](int x, int y, int w, int h, int c) {
        for (int j = y; j < y + h; j++)
            for (int i = x; i < x + w; i++) plot(i, j, c);
    };
    rect(6, 12, 5, 9, 5);
    rect(37, 12, 5, 9, 5);
    rect(6, 50, 5, 9, 5);
    rect(37, 50, 5, 9, 5);
    rect(10, 8, 28, 56, 1);
    rect(12, 10, 24, 6, 11);
    rect(13, 12, 22, 8, 3);
    rect(12, 24, 7, 12, 4);
    rect(29, 24, 7, 12, 4);
    rect(12, 44, 7, 8, 4);
    rect(29, 44, 7, 8, 4);
    rect(10, 40, 28, 4, 2);
    rect(12, 58, 5, 3, 8);
    rect(31, 58, 5, 3, 8);
    rect(13, 8, 4, 2, 7);
    rect(31, 8, 4, 2, 7);
    plot(9, 18, 6);
    plot(38, 18, 6);
    if (door < 0) {
        rect(0, 28, 10, 18, 9);
        rect(1, 30, 7, 5, 3);
    } else if (door > 0) {
        rect(38, 28, 10, 18, 9);
        rect(40, 30, 7, 5, 3);
    } else {
        rect(11, 28, 2, 16, 9);
    }
    b.outline(6, false);
    b.rect(18, 15, 12, 8, 10);
    gs::TextStyle st{1, 6, 0, 0, 0};
    gs::Bitmap num = gs::textBitmap("6", st);
    b.blit(num, 24 - num.w / 2, 16);
    return b;
}

gs::Bitmap paintBox() {
    gs::Bitmap b(32, 32);
    for (int t = 0; t < 3; t++) {
        for (int i = 0; i < 32; i++) {
            b.set(i, t, 1);
            b.set(i, 31 - t, 1);
            b.set(t, i, 1);
            b.set(31 - t, i, 1);
        }
    }
    for (int i = 0; i < 9; i++) {
        b.set(i, 0, 3);
        b.set(i, 1, 3);
        b.set(0, i, 3);
        b.set(1, i, 3);
        b.set(31 - i, 0, 3);
        b.set(31 - i, 1, 3);
        b.set(31, i, 3);
        b.set(30, i, 3);
        b.set(i, 31, 3);
        b.set(i, 30, 3);
        b.set(0, 31 - i, 3);
        b.set(1, 31 - i, 3);
        b.set(31 - i, 31, 3);
        b.set(31 - i, 30, 3);
        b.set(31, 31 - i, 3);
        b.set(30, 31 - i, 3);
    }
    for (int i = 6; i < 26; i += 3) {
        b.set(i, 5, 2);
        b.set(i, 26, 2);
        b.set(5, i, 2);
        b.set(26, i, 2);
    }
    return b;
}

gs::Bitmap paintShelter(const char* name) {
    gs::Bitmap b(44, 46);
    b.rect(2, 12, 40, 5, 1);
    b.rect(6, 17, 3, 22, 2);
    b.rect(34, 17, 3, 22, 2);
    b.rect(9, 31, 26, 4, 3);
    b.rect(20, 2, 4, 12, 2);
    b.ellipse(22, 4, 6, 4, 5);
    b.outline(6, false);
    b.rect(6, 18, 32, 11, 5);
    gs::TextStyle st{1, 4, 0, 0, 1};
    gs::Bitmap t = gs::textBitmap(name, st);
    b.blit(t, 22 - t.w / 2, 20);
    return b;
}

gs::Bitmap paintPerson(int frame) {
    gs::Bitmap b(12, 16);
    b.ellipse(6, 3.2f, 2.4f, 2.3f, 6);
    b.ellipse(6, 3.4f, 2.0f, 1.8f, 1);
    b.set(7, 3, 6);
    b.rect(4, 6, 4, 5, 2);
    b.rect(2, 7, 2, 3, 3);
    b.rect(8, 7, 2, 3, 3);
    if (frame == 0) {
        b.rect(4, 11, 2, 4, 4);
        b.rect(7, 11, 2, 4, 4);
    } else {
        b.rect(3, 11, 2, 4, 4);
        b.rect(8, 11, 2, 4, 4);
    }
    b.set(4, 14, 5);
    b.set(7, 14, 5);
    return b;
}

gs::Bitmap paintCar() {
    gs::Bitmap b(24, 42);
    b.rect(1, 7, 4, 7, 4);
    b.rect(19, 7, 4, 7, 4);
    b.rect(1, 28, 4, 7, 4);
    b.rect(19, 28, 4, 7, 4);
    b.rect(4, 3, 16, 36, 1);
    b.rect(6, 6, 12, 8, 3);
    b.rect(6, 18, 12, 10, 2);
    b.rect(7, 3, 3, 2, 5);
    b.rect(14, 3, 3, 2, 5);
    b.rect(6, 35, 3, 2, 4);
    b.rect(15, 35, 3, 2, 4);
    b.outline(4, false);
    return b;
}

gs::Bitmap paintTruck() {
    gs::Bitmap b(28, 56);
    b.rect(2, 8, 4, 8, 4);
    b.rect(22, 8, 4, 8, 4);
    b.rect(2, 40, 4, 8, 4);
    b.rect(22, 40, 4, 8, 4);
    b.rect(5, 3, 18, 16, 2);
    b.rect(7, 5, 14, 7, 3);
    b.rect(4, 19, 20, 32, 1);
    b.rect(7, 24, 14, 8, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap paintCone() {
    gs::Bitmap b(12, 14);
    b.poly({{6.f, 1.f}, {11.f, 12.f}, {1.f, 12.f}}, 1);
    b.rect(2, 6, 8, 2, 2);
    b.ellipse(6, 12.2f, 4.2f, 1.6f, 3);
    return b;
}

gs::Bitmap paintTree() {
    gs::Bitmap b(22, 26);
    b.rect(10, 16, 3, 9, 3);
    b.ellipse(11, 11, 9, 8, 1);
    b.ellipse(8, 9, 4, 3, 2);
    return b;
}

gs::Bitmap paintZebra() {
    gs::Bitmap b(40, 14);
    for (int i = 1; i < 40; i += 6) b.rect(float(i), 1, 3, 12, 9);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 15), gs::rgb4(10, 12, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BUS, {0, gs::rgb4(15, 12, 2), gs::rgb4(2, 3, 9), gs::rgb4(7, 11, 13), gs::rgb4(1, 2, 5),
                          gs::rgb4(2, 2, 2), gs::rgb4(1, 1, 1), gs::rgb4(15, 15, 10), gs::rgb4(14, 2, 2),
                          gs::rgb4(13, 9, 1), gs::rgb4(14, 14, 12), gs::rgb4(15, 14, 6), 0, 0, 0, ink});
    setPal(vdp, PAL_BOX, {0, gs::rgb4(15, 13, 2), gs::rgb4(12, 8, 1), gs::rgb4(15, 15, 13), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_READY, {0, gs::rgb4(4, 15, 5), gs::rgb4(2, 9, 3), gs::rgb4(14, 15, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_SHELTER, {0, gs::rgb4(12, 3, 3), gs::rgb4(8, 8, 9), gs::rgb4(8, 5, 3), gs::rgb4(15, 15, 14),
                              gs::rgb4(2, 5, 12), gs::rgb4(1, 1, 2), 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_CAR, {0, gs::rgb4(13, 2, 2), gs::rgb4(8, 1, 1), gs::rgb4(6, 10, 12), gs::rgb4(1, 1, 1),
                          gs::rgb4(15, 14, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_CAR2, {0, gs::rgb4(2, 5, 13), gs::rgb4(1, 3, 8), gs::rgb4(6, 10, 12), gs::rgb4(1, 1, 1),
                           gs::rgb4(15, 14, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_TRUCK, {0, gs::rgb4(12, 11, 8), gs::rgb4(4, 5, 7), gs::rgb4(6, 10, 12), gs::rgb4(1, 1, 1),
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_PERSON, {0, gs::rgb4(13, 9, 6), gs::rgb4(2, 4, 10), gs::rgb4(12, 8, 6), gs::rgb4(2, 2, 4),
                             gs::rgb4(4, 3, 2), gs::rgb4(2, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(2, 8, 3), gs::rgb4(5, 12, 4), gs::rgb4(6, 4, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_ROAD, {0, gs::rgb4(3, 3, 4), gs::rgb4(5, 5, 6), gs::rgb4(14, 12, 3), gs::rgb4(8, 8, 7),
                           gs::rgb4(6, 6, 5), gs::rgb4(5, 3, 6), gs::rgb4(13, 10, 5), gs::rgb4(4, 4, 4),
                           gs::rgb4(13, 13, 12), gs::rgb4(3, 2, 4), gs::rgb4(8, 2, 3), 0, 0, 0, 0, ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_OK, {0, gs::rgb4(6, 15, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(8, 8, 9), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 12, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_CONE, {0, gs::rgb4(15, 7, 1), gs::rgb4(15, 15, 14), gs::rgb4(3, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    paintRoad(vdp, tiles);

    art.bus = gs::uploadMipped(vdp, paintBus(0, 0));
    art.busYawL = gs::uploadMipped(vdp, paintBus(-1, 0));
    art.busYawR = gs::uploadMipped(vdp, paintBus(1, 0));
    art.busDoor[0] = gs::uploadMipped(vdp, paintBus(0, -1));
    art.busDoor[1] = gs::uploadMipped(vdp, paintBus(0, 1));
    art.box = gs::uploadMipped(vdp, paintBox());
    const char* names[6] = {"OAK", "PIER", "HILL", "MILL", "YARD", "BARN"};
    for (int i = 0; i < 6; i++) art.shelter[i] = gs::uploadMipped(vdp, paintShelter(names[i]));
    art.person[0] = gs::uploadMipped(vdp, paintPerson(0));
    art.person[1] = gs::uploadMipped(vdp, paintPerson(1));
    art.car[0] = gs::uploadMipped(vdp, paintCar());
    art.car[1] = gs::uploadMipped(vdp, paintCar());
    art.truck = gs::uploadMipped(vdp, paintTruck());
    art.cone = gs::uploadMipped(vdp, paintCone());
    art.tree = gs::uploadMipped(vdp, paintTree());
    art.zebra = gs::uploadMipped(vdp, paintZebra());
    gs::Bitmap shade(28, 14);
    shade.ellipse(14, 7, 12, 5, 1);
    art.shade = gs::uploadMipped(vdp, shade);
    gs::Bitmap panel(16, 16);
    panel.rect(0, 0, 16, 16, 6);
    panel.rect(0, 0, 16, 1, 7);
    art.panel = gs::uploadMipped(vdp, panel);

    vdp.A.enabled = false;
    vdp.B.enabled = true;
    vdp.hudEnabled = true;
    vdp.setFogColor(gs::rgb4(2, 2, 3));
}

}  // namespace bus

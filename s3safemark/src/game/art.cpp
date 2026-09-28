#include "game/art.h"

#include <initializer_list>
#include <string>

namespace safemark {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

constexpr int kDoorX = 108, kDoorY = 98, kDoorW = 104, kDoorH = 92;
constexpr float kDialLx[3] = {22, 52, 82};
constexpr float kDialLy = 34;

constexpr int WALL = 1, BAND = 2, CEIL = 3, TRIM = 4, WOOD = 5, WOODD = 6, FLOOR = 7, SEAM = 8;
constexpr int PAPER = 9, BRASS = 10, RED = 11, HOLE = 12, GOLD = 13, GLOW = 14, SHADOW = 15;

void paintRoom(gs::Bitmap& b, Art& art) {
    b.rect(0, 0, 320, 224, WALL);
    b.rect(0, 0, 320, 22, CEIL);
    b.ellipse(160, 6, 90, 12, GLOW);
    b.rect(0, 22, 320, 5, TRIM);
    for (int x = 6; x < 320; x += 22) b.rect(x, 28, 2, 118, BAND);

    b.rect(0, 146, 320, 42, WOOD);
    b.rect(0, 146, 320, 4, TRIM);
    b.rect(0, 188, 320, 36, FLOOR);
    for (int y = 196; y < 224; y += 9) b.rect(0, y, 320, 1, SEAM);

    // Round clock. Digit is a sprite on the paper face.
    const float ccx = 48, ccy = 72, cr = 26;
    b.ellipse(ccx + 2, ccy + 3, cr + 2, cr + 2, SHADOW);
    b.ellipse(ccx, ccy, cr + 3, cr + 3, WOODD);
    b.ellipse(ccx, ccy, cr, cr, BRASS);
    b.ellipse(ccx, ccy, cr - 5, cr - 5, PAPER);
    b.rect(ccx - 3, ccy - cr - 8, 6, 8, BRASS);
    art.clueX[0] = ccx;
    art.clueY[0] = ccy;

    // Framed notice. Digit sits in the paper.
    const int px = 118, py = 30, pw = 84, ph = 52;
    b.rect(px + 3, py + 3, pw, ph, SHADOW);
    b.rect(px, py, pw, ph, WOOD);
    b.rect(px + 5, py + 5, pw - 10, ph - 10, BRASS);
    b.rect(px + 9, py + 9, pw - 18, ph - 18, PAPER);
    art.clueX[1] = px + pw * 0.5f;
    art.clueY[1] = py + ph * 0.5f;

    // Ledger card. Digit is the page number.
    const int kx = 228, ky = 36, kw = 72, kh = 58;
    b.rect(kx + 3, ky + 3, kw, kh, SHADOW);
    b.rect(kx, ky, kw, kh, WOODD);
    b.rect(kx + 4, ky + 4, kw - 8, 12, RED);
    b.rect(kx + 4, ky + 16, kw - 8, kh - 20, PAPER);
    art.clueX[2] = kx + kw * 0.5f;
    art.clueY[2] = ky + 16 + (kh - 20) * 0.5f;

    // Wall safe cavity and the gold bars the mark stamps onto.
    b.rect(104, 94, 112, 100, BRASS);
    b.rect(106, 96, 108, 96, WOODD);
    b.rect(116, 106, 88, 76, HOLE);
    b.rect(136, 118, 48, 7, GOLD);
    b.rect(136, 132, 48, 7, GOLD);
    b.rect(136, 146, 48, 7, GOLD);

    b.rect(70, 196, 180, 10, RED);
    b.rect(76, 198, 168, 5, WOODD);
}

gs::Bitmap paintDoor() {
    gs::Bitmap b(kDoorW, kDoorH);
    b.rect(0, 0, kDoorW, kDoorH, 1);
    b.rect(0, 0, kDoorW, 3, 2);
    b.rect(0, 0, 3, kDoorH, 2);
    b.rect(0, kDoorH - 3, kDoorW, 3, 3);
    b.rect(kDoorW - 3, 0, 3, kDoorH, 3);
    for (float x : kDialLx) {
        b.ellipse(x, kDialLy, 13, 13, 5);
        b.ellipse(x, kDialLy, 11, 11, 4);
    }
    b.rect(46, 54, 12, 24, 6);
    b.rect(48, 52, 8, 26, 5);
    b.ellipse(52, 52, 5, 4, 5);
    b.ellipse(8, 8, 2.2f, 2.2f, 7);
    b.ellipse(kDoorW - 8, 8, 2.2f, 2.2f, 7);
    b.ellipse(8, kDoorH - 8, 2.2f, 2.2f, 7);
    b.ellipse(kDoorW - 8, kDoorH - 8, 2.2f, 2.2f, 7);
    return b;
}

gs::Bitmap paintCaret() {
    gs::Bitmap b(14, 8);
    b.poly({{7, 7}, {1, 1}, {13, 1}}, 1);
    return b;
}

gs::Bitmap paintRing() {
    gs::Bitmap b(40, 40);
    b.ellipse(20, 20, 17, 17, 1);
    b.ellipse(20, 20, 13, 13, 0);
    return b;
}

gs::Bitmap paintStamp() {
    gs::Bitmap b(36, 36);
    b.ellipse(18, 18, 16, 16, 1);
    b.ellipse(18, 18, 11, 11, 2);
    b.rect(16, 6, 4, 24, 2);
    b.rect(6, 16, 24, 4, 2);
    return b;
}

gs::Bitmap paintWheel() {
    gs::Bitmap b(22, 22);
    b.ellipse(11, 11, 10, 10, 2);
    b.ellipse(11, 11, 8, 8, 1);
    b.rect(10, 2, 2, 5, 3);
    return b;
}

gs::Bitmap digitBitmap(int d) {
    gs::TextStyle st;
    st.scale = 2;
    st.color = 1;
    st.spacing = 0;
    return gs::textBitmap(std::string(1, char('0' + d)), st);
}

void loadFont(gs::VDP& vdp, Art& art, gs::TileAlloc& tiles) {
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) {
                    px[y * 8 + x + 1] = 1;
                    if (y + 1 < 8 && x + 2 < 8) px[(y + 1) * 8 + x + 2] = 15;
                }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        art.font[c - 32] = t;
    }
    for (int d = 0; d < 10; d++) art.digit[d] = gs::uploadMipped(vdp, digitBitmap(d));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(15, 14, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 12, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 4, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(8, 15, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_ROOM,
           {0, gs::rgb4(3, 4, 6), gs::rgb4(4, 6, 8), gs::rgb4(12, 11, 9), gs::rgb4(8, 6, 3), gs::rgb4(6, 4, 2),
            gs::rgb4(3, 2, 1), gs::rgb4(7, 5, 3), gs::rgb4(4, 3, 2), gs::rgb4(14, 13, 11), gs::rgb4(12, 9, 3),
            gs::rgb4(11, 2, 2), gs::rgb4(1, 1, 2), gs::rgb4(15, 12, 3), gs::rgb4(14, 13, 7), gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_INK, {0, gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_WHEEL, {0, gs::rgb4(1, 1, 2), gs::rgb4(13, 10, 4), gs::rgb4(15, 14, 10)});
    setPal(vdp, PAL_DOOR,
           {0, gs::rgb4(7, 8, 9), gs::rgb4(13, 14, 15), gs::rgb4(3, 4, 5), gs::rgb4(1, 1, 2), gs::rgb4(12, 9, 3),
            gs::rgb4(6, 4, 2), gs::rgb4(15, 14, 11)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 2), gs::rgb4(8, 4, 1)});
    setPal(vdp, PAL_SHADE, {0, gs::rgb4(1, 2, 3)});

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, art, tiles);

    gs::Bitmap room(gs::SCREEN_W, gs::SCREEN_H);
    paintRoom(room, art);
    vdp.B.clear();
    gs::bitmapToPlane(tiles, vdp.B, 0, 0, room, PAL_ROOM);
    vdp.A.enabled = false;
    vdp.A.clear();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        vdp.lineBackdrop[y] = gs::rgb4(3, 4, 6);
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }

    art.door = gs::uploadMipped(vdp, paintDoor());
    art.caret = gs::uploadMipped(vdp, paintCaret());
    art.ring = gs::uploadMipped(vdp, paintRing());
    art.stamp = gs::uploadMipped(vdp, paintStamp());
    art.wheel = gs::uploadMipped(vdp, paintWheel());
    gs::Bitmap solid(8, 8);
    solid.rect(0, 0, 8, 8, 1);
    art.solid = gs::uploadMipped(vdp, solid);

    art.doorX = kDoorX;
    art.doorY = kDoorY;
    art.doorW = kDoorW;
    art.doorH = kDoorH;
    for (int i = 0; i < 3; i++) art.dialX[i] = kDoorX + kDialLx[i];
    art.dialY = kDoorY + kDialLy;
    art.slideOpen = 78;
}

}  // namespace safemark

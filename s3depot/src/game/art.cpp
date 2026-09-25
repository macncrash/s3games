#include "game/art.h"

#include <cmath>
#include <initializer_list>

namespace depot {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

int uploadTile(gs::VDP& vdp, gs::TileAlloc& tiles, const gs::Bitmap& b) {
    uint8_t px[64] = {};
    for (int y = 0; y < 8 && y < b.h; y++)
        for (int x = 0; x < 8 && x < b.w; x++) px[y * 8 + x] = uint8_t(b.get(x, y) & 15);
    int t = tiles.alloc(1);
    vdp.loadTile(t, px);
    return t;
}

gs::Bitmap spin(const gs::Bitmap& src, float ang) {
    gs::Bitmap d(src.w, src.h);
    float cx = (src.w - 1) * 0.5f;
    float cy = (src.h - 1) * 0.5f;
    float c = std::cos(ang);
    float s = std::sin(ang);
    for (int y = 0; y < src.h; y++) {
        for (int x = 0; x < src.w; x++) {
            float dx = float(x) - cx;
            float dy = float(y) - cy;
            int sx = int(std::lround(cx + c * dx + s * dy));
            int sy = int(std::lround(cy - s * dx + c * dy));
            d.set(x, y, src.get(sx, sy));
        }
    }
    return d;
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
    for (int c = 32; c < 128; c++) {
        uint8_t px[64];
        for (int i = 0; i < 64; i++) px[i] = 2;
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (!g[y * 5 + x]) continue;
                int sy = y + 1;
                int sx = x + 2;
                if (sy < 7 && sx < 8) px[sy * 8 + sx] = 15;
            }
        }
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (!g[y * 5 + x]) continue;
                px[y * 8 + x + 1] = 1;
            }
        }
        for (int x = 0; x < 8; x++) px[7 * 8 + x] = 4;
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

gs::Mipped words(gs::VDP& vdp, const char* s, int scale) {
    gs::TextStyle st;
    st.scale = scale;
    st.color = 1;
    st.outline = 3;
    st.shadow = 2;
    st.spacing = 1;
    return gs::uploadMipped(vdp, gs::textBitmap(s, st));
}

gs::Bitmap tugEast() {
    gs::Bitmap b(40, 40);
    b.ellipse(15, 11, 4.4f, 4.4f, 1);
    b.ellipse(15, 29, 4.4f, 4.4f, 1);
    b.ellipse(24, 12, 3.5f, 3.5f, 1);
    b.ellipse(24, 28, 3.5f, 3.5f, 1);
    b.ellipse(15, 11, 1.6f, 1.6f, 5);
    b.ellipse(15, 29, 1.6f, 1.6f, 5);
    b.ellipse(24, 12, 1.2f, 1.2f, 5);
    b.ellipse(24, 28, 1.2f, 1.2f, 5);
    b.rect(7, 14, 8, 12, 2);
    b.rect(7, 14, 8, 2, 1);
    b.rect(13, 13, 14, 14, 3);
    b.rect(13, 13, 14, 2, 2);
    b.rect(16, 16, 8, 8, 6);
    b.rect(17, 17, 6, 5, 8);
    b.rect(18, 17, 3, 2, 4);
    b.rect(26, 11, 3, 18, 5);
    b.rect(26, 11, 3, 2, 4);
    b.rect(29, 16, 8, 2, 4);
    b.rect(29, 23, 8, 2, 4);
    b.rect(35, 16, 2, 2, 10);
    b.rect(35, 23, 2, 2, 10);
    b.ellipse(19, 12.2f, 2.1f, 1.7f, 9);
    b.ellipse(19, 11.8f, 0.7f, 0.6f, 7);
    b.outline(1, false);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(28, 12);
    b.ellipse(14, 6, 12.f, 4.2f, 1);
    return b;
}

gs::Bitmap crateArt() {
    gs::Bitmap b(24, 20);
    b.rect(2, 3, 20, 14, 3);
    b.rect(2, 3, 20, 3, 4);
    b.rect(2, 9, 20, 2, 5);
    b.rect(11, 3, 2, 14, 2);
    b.rect(4, 5, 2, 2, 6);
    b.rect(18, 5, 2, 2, 6);
    b.rect(4, 13, 2, 2, 6);
    b.rect(18, 13, 2, 2, 6);
    b.outline(1, false);
    return b;
}

gs::Bitmap drumArt() {
    gs::Bitmap b(18, 24);
    b.ellipse(9, 18, 6.2f, 3.0f, 2);
    b.rect(3, 7, 12, 12, 3);
    b.rect(3, 10, 12, 3, 5);
    b.rect(3, 14, 12, 2, 6);
    b.ellipse(9, 7, 6.2f, 3.0f, 3);
    b.ellipse(9, 6.2f, 3.4f, 1.3f, 4);
    b.outline(1, false);
    return b;
}

gs::Bitmap palletArt() {
    gs::Bitmap b(26, 20);
    b.rect(1, 13, 24, 5, 3);
    b.rect(2, 15, 5, 2, 1);
    b.rect(19, 15, 5, 2, 1);
    b.rect(3, 3, 9, 11, 4);
    b.rect(3, 3, 9, 2, 5);
    b.rect(13, 5, 9, 9, 5);
    b.rect(13, 5, 9, 2, 6);
    b.outline(1, false);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(16, 32);
    b.rect(7, 12, 2, 16, 2);
    b.rect(5, 26, 6, 3, 1);
    b.poly({{2, 12}, {8, 4}, {14, 12}}, 3);
    b.rect(3, 10, 10, 4, 3);
    b.rect(4, 11, 8, 2, 4);
    b.rect(6, 11, 4, 1, 5);
    b.outline(1, false);
    return b;
}

gs::Bitmap bollardArt() {
    gs::Bitmap b(10, 16);
    b.rect(3, 3, 4, 11, 6);
    b.rect(3, 3, 4, 3, 7);
    b.rect(3, 8, 4, 2, 7);
    b.ellipse(5, 3, 3.f, 1.6f, 6);
    b.outline(1, false);
    return b;
}

gs::Bitmap moonArt() {
    gs::Bitmap b(18, 18);
    b.ellipse(8, 9, 7.f, 7.f, 7);
    b.ellipse(12, 7, 6.f, 6.f, 0);
    b.ellipse(6, 10, 1.3f, 1.1f, 8);
    b.ellipse(5, 7, 0.7f, 0.6f, 8);
    return b;
}

gs::Bitmap cloudArt() {
    gs::Bitmap b(32, 14);
    b.ellipse(12, 8, 8.f, 4.f, 5);
    b.ellipse(20, 7, 8.f, 4.2f, 6);
    b.ellipse(16, 6, 5.f, 3.f, 6);
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(6, 6);
    b.ellipse(3, 3, 2.4f, 2.f, 3);
    b.ellipse(3, 3, 1.f, 0.8f, 8);
    return b;
}

gs::Bitmap beaconArt() {
    gs::Bitmap b(6, 6);
    b.ellipse(3, 3, 2.2f, 2.2f, 9);
    b.ellipse(3, 3, 0.8f, 0.8f, 7);
    return b;
}

gs::Bitmap plaqueArt() {
    gs::Bitmap b(176, 30);
    b.rect(0, 0, 176, 30, 2);
    b.rect(0, 0, 176, 2, 4);
    b.rect(0, 28, 176, 2, 4);
    return b;
}

gs::Bitmap skyTile(int variant) {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, (variant & 1) ? 1 : 2);
    if (variant == 0) {
        b.set(1, 2, 4);
        b.set(2, 2, 3);
    } else if (variant == 2) {
        b.set(5, 4, 3);
    } else if (variant == 3) {
        b.set(3, 1, 4);
        b.set(6, 6, 3);
    }
    return b;
}

gs::Bitmap concTile(int variant) {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 2);
    b.set(1, 1, 3);
    b.set(6, 2, 1);
    b.set(3, 5, 4);
    b.set(5, 6, 3);
    if (variant == 1) b.set(4, 3, 7);
    if (variant == 2) {
        b.set(2, 6, 1);
        b.set(7, 4, 3);
    }
    return b;
}

gs::Bitmap brickTile() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 2);
    b.rect(0, 3, 8, 1, 1);
    b.rect(0, 7, 8, 1, 1);
    b.rect(3, 0, 1, 3, 1);
    b.rect(6, 4, 1, 3, 1);
    b.set(1, 1, 3);
    b.set(5, 5, 4);
    return b;
}

gs::Bitmap windowTile() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 2);
    b.rect(0, 7, 8, 1, 1);
    b.rect(1, 2, 6, 4, 7);
    b.rect(2, 3, 2, 2, 8);
    b.set(5, 3, 8);
    b.rect(3, 2, 1, 4, 1);
    return b;
}

gs::Bitmap roofTile() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 4);
    b.set(2, 2, 1);
    b.set(6, 3, 1);
    b.rect(0, 6, 8, 2, 2);
    b.rect(0, 5, 8, 1, 6);
    return b;
}

gs::Bitmap postTile() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 4);
    b.rect(2, 0, 4, 8, 2);
    b.rect(3, 0, 2, 8, 3);
    b.set(3, 2, 6);
    b.set(3, 6, 6);
    return b;
}

gs::Bitmap hazardTile() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 1);
    b.rect(0, 0, 4, 8, 5);
    b.rect(0, 0, 1, 8, 6);
    return b;
}

gs::Bitmap apronTile() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 4);
    b.set(2, 2, 1);
    b.set(5, 4, 2);
    b.set(6, 6, 1);
    b.set(1, 5, 2);
    return b;
}

gs::Bitmap stripeTile() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 2);
    b.rect(0, 3, 8, 2, 5);
    b.rect(0, 3, 8, 1, 6);
    return b;
}

gs::Bitmap stainTile() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 2);
    b.ellipse(4, 4, 3.f, 2.2f, 7);
    b.set(2, 3, 1);
    b.set(1, 1, 3);
    return b;
}

gs::Bitmap railTile() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 2);
    b.set(1, 6, 1);
    b.set(5, 7, 1);
    b.rect(1, 1, 2, 6, 3);
    b.rect(0, 2, 8, 1, 4);
    b.rect(0, 3, 8, 1, 5);
    b.rect(0, 5, 8, 1, 4);
    return b;
}

gs::Bitmap barTile() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 2);
    b.rect(0, 7, 8, 1, 4);
    return b;
}

gs::Bitmap doorTop(bool shut) {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, shut ? 9 : 5);
    b.rect(0, 0, 1, 8, 6);
    b.rect(7, 0, 1, 8, 6);
    b.rect(0, 0, 8, 2, 6);
    if (shut) {
        b.rect(2, 3, 4, 4, 10);
        b.set(3, 4, 9);
    } else {
        b.set(3, 5, 8);
    }
    return b;
}

gs::Bitmap doorBot(bool shut) {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, shut ? 9 : 5);
    b.rect(0, 0, 1, 8, 6);
    b.rect(7, 0, 1, 8, 6);
    b.rect(0, 6, 8, 2, 6);
    if (shut) b.rect(2, 1, 4, 4, 10);
    return b;
}

gs::Bitmap bayOpenTile() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 2);
    b.rect(0, 0, 8, 2, 3);
    b.rect(0, 0, 8, 1, 4);
    b.set(2, 4, 1);
    b.set(5, 6, 5);
    return b;
}

gs::Bitmap bayFullTile() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 1);
    b.rect(1, 1, 6, 6, 5);
    b.set(2, 4, 4);
    b.set(3, 5, 4);
    b.set(4, 4, 4);
    b.set(5, 3, 4);
    b.set(6, 2, 4);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 14, 10), gs::rgb4(2, 2, 3), 0, gs::rgb4(12, 8, 2)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 6, 4), gs::rgb4(3, 1, 1), 0, gs::rgb4(8, 3, 2)});
    setPal(vdp, PAL_YARD,
           {0, gs::rgb4(4, 4, 4), gs::rgb4(7, 7, 6), gs::rgb4(10, 10, 9), gs::rgb4(5, 5, 4), gs::rgb4(14, 12, 3),
            gs::rgb4(9, 7, 1), gs::rgb4(3, 3, 2), gs::rgb4(13, 13, 12)});
    setPal(vdp, PAL_BRICK,
           {0, gs::rgb4(6, 5, 5), gs::rgb4(9, 3, 2), gs::rgb4(12, 5, 3), gs::rgb4(5, 2, 1), gs::rgb4(1, 1, 2),
            gs::rgb4(13, 11, 3), gs::rgb4(8, 12, 13), gs::rgb4(14, 12, 6), gs::rgb4(8, 5, 2), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_NIGHT,
           {0, gs::rgb4(1, 2, 6), gs::rgb4(2, 3, 8), gs::rgb4(14, 14, 11), gs::rgb4(15, 15, 15), gs::rgb4(6, 7, 10),
            gs::rgb4(9, 10, 12), gs::rgb4(14, 14, 10), gs::rgb4(10, 10, 7)});
    setPal(vdp, PAL_TUG,
           {0, gs::rgb4(1, 1, 1), gs::rgb4(9, 4, 1), gs::rgb4(14, 7, 1), gs::rgb4(15, 13, 3), gs::rgb4(7, 7, 8),
            gs::rgb4(2, 2, 3), gs::rgb4(15, 15, 13), gs::rgb4(6, 10, 13), gs::rgb4(14, 2, 2), gs::rgb4(4, 4, 5)});
    setPal(vdp, PAL_CRATE,
           {0, gs::rgb4(2, 1, 1), gs::rgb4(6, 3, 1), gs::rgb4(10, 6, 2), gs::rgb4(13, 9, 4), gs::rgb4(8, 2, 1),
            gs::rgb4(5, 5, 5)});
    setPal(vdp, PAL_DRUM,
           {0, gs::rgb4(1, 1, 2), gs::rgb4(3, 4, 6), gs::rgb4(6, 8, 10), gs::rgb4(11, 13, 14), gs::rgb4(2, 4, 10),
            gs::rgb4(13, 11, 2)});
    setPal(vdp, PAL_PALLET,
           {0, gs::rgb4(2, 1, 1), gs::rgb4(6, 4, 2), gs::rgb4(10, 7, 3), gs::rgb4(2, 8, 3), gs::rgb4(4, 11, 5),
            gs::rgb4(12, 12, 8)});
    setPal(vdp, PAL_LAMP,
           {0, gs::rgb4(2, 2, 2), gs::rgb4(5, 5, 6), gs::rgb4(8, 7, 5), gs::rgb4(14, 11, 3), gs::rgb4(15, 15, 12),
            gs::rgb4(14, 12, 2), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_TITLE, {0, gs::rgb4(15, 13, 6), gs::rgb4(3, 2, 1), gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_DOCK,
           {0, gs::rgb4(1, 5, 2), gs::rgb4(2, 9, 3), gs::rgb4(5, 13, 5), gs::rgb4(13, 15, 12), gs::rgb4(1, 6, 3)});
    setPal(vdp, PAL_RAIL,
           {0, gs::rgb4(3, 3, 3), gs::rgb4(5, 5, 4), gs::rgb4(6, 4, 2), gs::rgb4(8, 8, 9), gs::rgb4(12, 12, 13)});
    vdp.setFogColor(gs::rgb4(2, 2, 5));

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    for (int i = 0; i < 4; i++) art.sky[i] = uploadTile(vdp, tiles, skyTile(i));
    for (int i = 0; i < 3; i++) art.conc[i] = uploadTile(vdp, tiles, concTile(i));
    art.brick = uploadTile(vdp, tiles, brickTile());
    art.window = uploadTile(vdp, tiles, windowTile());
    art.roof = uploadTile(vdp, tiles, roofTile());
    art.post = uploadTile(vdp, tiles, postTile());
    art.hazard = uploadTile(vdp, tiles, hazardTile());
    art.apron = uploadTile(vdp, tiles, apronTile());
    art.stripe = uploadTile(vdp, tiles, stripeTile());
    art.stain = uploadTile(vdp, tiles, stainTile());
    art.rail = uploadTile(vdp, tiles, railTile());
    art.bar = uploadTile(vdp, tiles, barTile());
    art.doorTop = uploadTile(vdp, tiles, doorTop(false));
    art.doorBot = uploadTile(vdp, tiles, doorBot(false));
    art.shutTop = uploadTile(vdp, tiles, doorTop(true));
    art.shutBot = uploadTile(vdp, tiles, doorBot(true));
    art.bayOpen = uploadTile(vdp, tiles, bayOpenTile());
    art.bayFull = uploadTile(vdp, tiles, bayFullTile());

    gs::Bitmap east = tugEast();
    const float step = 6.2831853f / 16.f;
    for (int i = 0; i < 16; i++) art.tug[i] = gs::uploadMipped(vdp, spin(east, step * float(i)));
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.crate = gs::uploadMipped(vdp, crateArt());
    art.drum = gs::uploadMipped(vdp, drumArt());
    art.pallet = gs::uploadMipped(vdp, palletArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.bollard = gs::uploadMipped(vdp, bollardArt());
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.beacon = gs::uploadMipped(vdp, beaconArt());
    art.plaque = gs::uploadMipped(vdp, plaqueArt());
    art.logo = words(vdp, "S3 DEPOT", 3);
    art.tag = words(vdp, "CLEAR THE YARD", 1);
    art.hint = words(vdp, "ARROWS DRIVE", 1);
    art.hint2 = words(vdp, "MATCH THE LOAD TO ITS DOOR", 1);
    art.win = words(vdp, "YARD CLEAR", 2);
    art.lose = words(vdp, "WHISTLE", 3);
    art.stowed = words(vdp, "STOWED", 1);
}

}  // namespace depot

#include "game/art.h"

#include <string>

namespace wharfbann {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

int makeTile(gs::TileAlloc& al, gs::VDP& vdp, std::initializer_list<const char*> rows) {
    uint8_t px[64] = {};
    int y = 0;
    for (const char* row : rows) {
        if (y >= 8) break;
        for (int x = 0; x < 8 && row[x]; x++) {
            char c = row[x];
            int v = 0;
            if (c >= '0' && c <= '9') v = c - '0';
            else if (c >= 'a' && c <= 'f') v = c - 'a' + 10;
            px[y * 8 + x] = uint8_t(v);
        }
        y++;
    }
    int t = al.alloc(1);
    vdp.loadTile(t, px);
    return t;
}

void loadFont(gs::VDP& vdp, Art& a, gs::TileAlloc& tiles) {
    gs::TextStyle big{2, 1, 0, 15, 1};
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

gs::Bitmap sailor(int pose) {
    gs::Bitmap b(28, 44);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    int bob = pose == 2 ? 1 : 0;
    R(8, 2 + bob, 12, 4, 8);
    R(9, 5 + bob, 10, 7, 5);
    R(11, 7 + bob, 2, 2, 1);
    R(16, 7 + bob, 2, 2, 1);
    R(6, 12 + bob, 16, 12, 3);
    R(6, 12 + bob, 4, 12, 4);
    R(10, 20 + bob, 8, 2, 2);
    if (pose == 1) {
        R(7, 24, 5, 13, 6);
        R(15, 26, 5, 11, 6);
        R(6, 36, 7, 3, 7);
        R(14, 36, 7, 3, 7);
        R(20, 14, 4, 8, 5);
    } else if (pose == 2) {
        R(6, 26, 5, 11, 6);
        R(16, 24, 5, 13, 6);
        R(5, 36, 7, 3, 7);
        R(15, 36, 7, 3, 7);
        R(2, 14, 4, 8, 5);
    } else if (pose == 3) {
        R(9, 22, 4, 10, 6);
        R(15, 22, 4, 10, 6);
        R(8, 31, 6, 3, 7);
        R(14, 31, 6, 3, 7);
        R(18, 12, 6, 3, 5);
    } else {
        R(9, 24, 4, 13, 6);
        R(15, 24, 4, 13, 6);
        R(8, 36, 6, 3, 7);
        R(14, 36, 6, 3, 7);
        R(20, 14, 3, 9, 5);
    }
    return b;
}

gs::Bitmap cloth() {
    gs::Bitmap b(26, 34);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    R(4, 2, 16, 3, 6);
    R(6, 5, 14, 20, 2);
    R(6, 5, 3, 20, 3);
    R(12, 9, 3, 12, 4);
    R(8, 25, 3, 6, 2);
    R(14, 25, 3, 5, 3);
    return b;
}

gs::Bitmap cask() {
    gs::Bitmap b(22, 22);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    R(3, 2, 16, 18, 3);
    R(2, 5, 18, 3, 5);
    R(2, 14, 18, 3, 5);
    R(8, 6, 2, 10, 2);
    R(12, 6, 2, 10, 4);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    gs::TileAlloc tiles(vdp);
    setPal(vdp, PAL_HUD, {gs::rgb4(0, 0, 0), gs::rgb4(14, 15, 13), gs::rgb4(2, 4, 6), gs::rgb4(15, 12, 4),
                          gs::rgb4(12, 3, 2), gs::rgb4(6, 8, 9), gs::rgb4(1, 2, 3)});
    setPal(vdp, PAL_WOOD, {gs::rgb4(0, 0, 0), gs::rgb4(4, 2, 1), gs::rgb4(8, 5, 2), gs::rgb4(11, 7, 3),
                           gs::rgb4(6, 4, 2), gs::rgb4(3, 2, 1), gs::rgb4(13, 10, 5), gs::rgb4(2, 2, 2),
                           gs::rgb4(9, 8, 6)});
    setPal(vdp, PAL_SAILOR, {gs::rgb4(0, 0, 0), gs::rgb4(1, 1, 2), gs::rgb4(12, 2, 2), gs::rgb4(2, 4, 8),
                             gs::rgb4(4, 6, 11), gs::rgb4(12, 9, 6), gs::rgb4(1, 2, 4), gs::rgb4(3, 2, 1),
                             gs::rgb4(14, 13, 10)});
    setPal(vdp, PAL_BANNER, {gs::rgb4(0, 0, 0), gs::rgb4(2, 1, 1), gs::rgb4(13, 2, 1), gs::rgb4(8, 1, 1),
                             gs::rgb4(14, 12, 3), gs::rgb4(5, 3, 1), gs::rgb4(10, 8, 4), gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_BARREL, {gs::rgb4(0, 0, 0), gs::rgb4(2, 1, 1), gs::rgb4(6, 3, 1), gs::rgb4(9, 5, 2),
                             gs::rgb4(12, 8, 3), gs::rgb4(4, 4, 4), gs::rgb4(8, 8, 7), gs::rgb4(14, 12, 6)});
    setPal(vdp, PAL_WATER, {gs::rgb4(0, 0, 0), gs::rgb4(1, 3, 6), gs::rgb4(2, 5, 9), gs::rgb4(3, 7, 11),
                            gs::rgb4(6, 10, 13), gs::rgb4(10, 13, 14), gs::rgb4(1, 2, 4), gs::rgb4(8, 9, 6)});
    setPal(vdp, PAL_HARBOR, {gs::rgb4(0, 0, 0), gs::rgb4(3, 4, 6), gs::rgb4(6, 7, 8), gs::rgb4(9, 9, 8),
                             gs::rgb4(4, 5, 4), gs::rgb4(12, 11, 8), gs::rgb4(2, 2, 3), gs::rgb4(14, 13, 10),
                             gs::rgb4(7, 5, 3)});
    setPal(vdp, PAL_ROPE, {gs::rgb4(0, 0, 0), gs::rgb4(5, 3, 1), gs::rgb4(9, 6, 2), gs::rgb4(12, 9, 4),
                           gs::rgb4(3, 2, 1), gs::rgb4(14, 12, 6), gs::rgb4(2, 5, 3), gs::rgb4(8, 10, 8)});

    loadFont(vdp, art, tiles);
    vdp.setFogColor(gs::rgb4(6, 8, 10));

    int sky = makeTile(tiles, vdp, {"00000000", "00010000", "00000000", "00000010", "00000000", "00100000",
                                    "00000000", "00000000"});
    int shed = makeTile(tiles, vdp, {"22222222", "23333332", "23444432", "23444432", "23333332", "22222222",
                                     "25555552", "22222222"});
    int mast = makeTile(tiles, vdp, {"00050000", "00050000", "00555000", "00050000", "00050000", "00050000",
                                     "00050000", "00555000"});
    int wave = makeTile(tiles, vdp, {"00000000", "11221100", "33443322", "22222233", "44444444", "33333333",
                                     "22222222", "33333333"});
    int foam = makeTile(tiles, vdp, {"00055000", "00555500", "11233111", "33333333", "22222222", "33333333",
                                     "44444444", "33333333"});

    vdp.A.clear();
    vdp.B.clear();
    vdp.HUD.clear();
    for (int cy = 0; cy < 32; cy++) {
        for (int cx = 0; cx < 64; cx++) {
            if (cy >= 16 && cy <= 22) {
                int tile = ((cx + cy) & 3) == 0 ? foam : wave;
                vdp.A.set(cx, cy, gs::entry(tile, PAL_WATER, (cx & 1), 0));
            }
            if (cy >= 6 && cy <= 12 && (cx % 11 == 2 || cx % 11 == 3))
                vdp.B.set(cx, cy, gs::entry(shed, PAL_HARBOR));
            if (cy >= 3 && cy <= 12 && cx % 17 == 8) vdp.B.set(cx, cy, gs::entry(mast, PAL_HARBOR));
            if (cy < 6 && ((cx * 3 + cy) % 19 == 0)) vdp.B.set(cx, cy, gs::entry(sky, PAL_HARBOR));
        }
    }

    art.stand = gs::uploadMipped(vdp, sailor(0));
    art.walkA = gs::uploadMipped(vdp, sailor(1));
    art.walkB = gs::uploadMipped(vdp, sailor(2));
    art.leap = gs::uploadMipped(vdp, sailor(3));
    art.banner = gs::uploadMipped(vdp, cloth());
    art.barrel = gs::uploadMipped(vdp, cask());

    gs::Bitmap pole(8, 40);
    pole.rect(3, 0, 3, 40, 3);
    pole.rect(1, 36, 6, 4, 2);
    art.staff = gs::uploadMipped(vdp, pole);

    gs::Bitmap post(16, 28);
    post.rect(6, 0, 4, 24, 2);
    post.rect(2, 20, 12, 6, 3);
    post.ellipse(8, 4, 5, 3, 4);
    art.bollard = gs::uploadMipped(vdp, post);

    gs::Bitmap deck(48, 14);
    deck.rect(0, 2, 48, 8, 3);
    deck.rect(0, 2, 48, 2, 6);
    for (int i = 0; i < 48; i += 8) deck.rect(float(i), 4, 1, 6, 1);
    deck.rect(0, 10, 48, 4, 2);
    art.plank = gs::uploadMipped(vdp, deck);

    gs::Bitmap coil(18, 12);
    coil.ellipse(9, 6, 7, 4, 2);
    coil.ellipse(9, 6, 3, 2, 0);
    coil.rect(2, 5, 14, 1, 3);
    art.rope = gs::uploadMipped(vdp, coil);

    gs::Bitmap floatB(14, 16);
    floatB.ellipse(7, 6, 5, 5, 3);
    floatB.rect(6, 10, 2, 5, 1);
    floatB.rect(3, 5, 8, 2, 5);
    art.buoy = gs::uploadMipped(vdp, floatB);

    gs::Bitmap bird(18, 8);
    bird.line(1, 4, 8, 2, 7, 1);
    bird.line(8, 2, 16, 5, 7, 1);
    bird.rect(7, 2, 3, 2, 5);
    art.gull = gs::uploadMipped(vdp, bird);

    gs::Bitmap box(20, 16);
    box.rect(1, 2, 18, 12, 4);
    box.rect(1, 2, 18, 3, 6);
    box.line(1, 2, 19, 14, 2, 1);
    art.crate = gs::uploadMipped(vdp, box);
}

}  // namespace wharfbann

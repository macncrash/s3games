#include "game/door.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <initializer_list>

namespace granary {
namespace {

constexpr int HOLD = 180 * 60;
constexpr int DOOR_MAX = 12;
constexpr float DOOR_X = 160.f;
constexpr float DOOR_Y = 136.f;

int ord(char ch) {
    if (ch >= 'A' && ch <= 'Z') return ch - 'A' + 1;
    if (ch >= '0' && ch <= '9') return 27 + (ch - '0');
    if (ch == ':') return 37;
    if (ch == '-') return 38;
    if (ch == '!') return 39;
    if (ch == '.') return 40;
    return 0;
}

// 5x7 glyphs, row-major bits in the low 5.
const uint8_t GLY[41][7] = {
    {0, 0, 0, 0, 0, 0, 0},
    {0x0e, 0x11, 0x11, 0x1f, 0x11, 0x11, 0x11},  // A
    {0x1e, 0x11, 0x11, 0x1e, 0x11, 0x11, 0x1e},  // B
    {0x0e, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0e},  // C
    {0x1e, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1e},  // D
    {0x1f, 0x10, 0x10, 0x1e, 0x10, 0x10, 0x1f},  // E
    {0x1f, 0x10, 0x10, 0x1e, 0x10, 0x10, 0x10},  // F
    {0x0e, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0e},  // G
    {0x11, 0x11, 0x11, 0x1f, 0x11, 0x11, 0x11},  // H
    {0x0e, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0e},  // I
    {0x01, 0x01, 0x01, 0x01, 0x11, 0x11, 0x0e},  // J
    {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11},  // K
    {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1f},  // L
    {0x11, 0x1b, 0x15, 0x15, 0x11, 0x11, 0x11},  // M
    {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11},  // N
    {0x0e, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0e},  // O
    {0x1e, 0x11, 0x11, 0x1e, 0x10, 0x10, 0x10},  // P
    {0x0e, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0d},  // Q
    {0x1e, 0x11, 0x11, 0x1e, 0x14, 0x12, 0x11},  // R
    {0x0f, 0x10, 0x10, 0x0e, 0x01, 0x01, 0x1e},  // S
    {0x1f, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04},  // T
    {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0e},  // U
    {0x11, 0x11, 0x11, 0x11, 0x0a, 0x0a, 0x04},  // V
    {0x11, 0x11, 0x11, 0x15, 0x15, 0x1b, 0x11},  // W
    {0x11, 0x11, 0x0a, 0x04, 0x0a, 0x11, 0x11},  // X
    {0x11, 0x11, 0x0a, 0x04, 0x04, 0x04, 0x04},  // Y
    {0x1f, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1f},  // Z
    {0x0e, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0e},  // 0
    {0x04, 0x0c, 0x04, 0x04, 0x04, 0x04, 0x0e},  // 1
    {0x0e, 0x11, 0x01, 0x06, 0x08, 0x10, 0x1f},  // 2
    {0x1f, 0x01, 0x02, 0x06, 0x01, 0x11, 0x0e},  // 3
    {0x02, 0x06, 0x0a, 0x12, 0x1f, 0x02, 0x02},  // 4
    {0x1f, 0x10, 0x1e, 0x01, 0x01, 0x11, 0x0e},  // 5
    {0x06, 0x08, 0x10, 0x1e, 0x11, 0x11, 0x0e},  // 6
    {0x1f, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08},  // 7
    {0x0e, 0x11, 0x11, 0x0e, 0x11, 0x11, 0x0e},  // 8
    {0x0e, 0x11, 0x11, 0x0f, 0x01, 0x02, 0x0c},  // 9
    {0x00, 0x04, 0x04, 0x00, 0x04, 0x04, 0x00},  // :
    {0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00},  // -
    {0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x04},  // !
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04},  // .
};

gs::Image alloc(gs::VDP& vdp, int w, int h, uint8_t*& p) {
    gs::Image im = vdp.allocImage(w, h);
    p = vdp.rom() + im.off;
    return im;
}

void put(gs::VDP& vdp, const gs::Image& im, float x, float y, float w, float h, int pal, int fog = 0, bool flip = false) {
    gs::Sprite s;
    s.x = int16_t(x);
    s.y = int16_t(y);
    s.w = int16_t(w);
    s.h = int16_t(h);
    s.img = im;
    s.pal = uint8_t(pal);
    s.fog = uint8_t(fog);
    s.hflip = flip;
    vdp.sprite(s);
}

}  // namespace

void Game::Img::px(int x, int y, int c) {
    if (x < 0 || y < 0 || x >= w || y >= h || c <= 0) return;
    p[y * w + x] = uint8_t(c);
}
void Game::Img::rect(int x, int y, int rw, int rh, int c) {
    for (int j = 0; j < rh; j++)
        for (int i = 0; i < rw; i++) px(x + i, y + j, c);
}
void Game::Img::frame(int x, int y, int rw, int rh, int c) {
    rect(x, y, rw, 1, c);
    rect(x, y + rh - 1, rw, 1, c);
    rect(x, y, 1, rh, c);
    rect(x + rw - 1, y, 1, rh, c);
}

float Game::rnd() {
    rng_ = rng_ * 1664525u + 1013904223u;
    return (rng_ >> 8) * (1.f / 16777216.f);
}

void Game::glyphTile(gs::VDP& vdp, int index, char ch) {
    uint8_t px[64] = {};
    const uint8_t* g = GLY[ord(ch)];
    for (int y = 0; y < 7; y++)
        for (int x = 0; x < 5; x++)
            if (g[y] & (1 << (4 - x))) px[(y + 1) * 8 + (x + 1)] = 1;
    vdp.loadTile(index, px);
}

void Game::buildArt(gs::VDP& vdp) {
    const int P = 16;
    auto pal = [&](int bank, std::initializer_list<uint16_t> cs) {
        int i = 0;
        for (uint16_t c : cs) vdp.setColor(bank * P + i++, c);
        while (i < 16) vdp.setColor(bank * P + i++, 0);
    };
    using gs::rgb4;
    pal(0, {0, rgb4(15, 14, 10)});                                          // ink
    pal(1, {0, rgb4(4, 2, 1), rgb4(8, 4, 2), rgb4(12, 7, 3), rgb4(6, 3, 2),
            rgb4(14, 11, 5), rgb4(9, 6, 2), rgb4(5, 5, 5), rgb4(2, 1, 1)});  // barn
    pal(2, {0, rgb4(6, 3, 1), rgb4(10, 6, 2), rgb4(13, 9, 4), rgb4(3, 3, 4),
            rgb4(8, 8, 9), rgb4(14, 12, 6)});                                 // door
    pal(3, {0, rgb4(12, 8, 5), rgb4(3, 4, 6), rgb4(6, 8, 10), rgb4(2, 2, 2),
            rgb4(10, 8, 4), rgb4(8, 8, 9), rgb4(4, 2, 1)});                   // guard
    pal(4, {0, rgb4(10, 6, 4), rgb4(8, 2, 2), rgb4(4, 2, 2), rgb4(3, 3, 3),
            rgb4(12, 10, 6), rgb4(6, 5, 4)});                                 // raider
    pal(5, {0, rgb4(15, 12, 4), rgb4(14, 6, 2), rgb4(15, 15, 12)});          // shot
    pal(6, {0, rgb4(7, 6, 2), rgb4(10, 8, 3), rgb4(4, 5, 2), rgb4(6, 4, 2)});  // yard tiles
    pal(7, {0, rgb4(2, 1, 1), rgb4(8, 3, 2)});                               // cracks
    pal(8, {0, rgb4(15, 12, 4), rgb4(15, 8, 2)});                            // lamp
    pal(9, {0, rgb4(15, 13, 8)});                                            // hud warn
    vdp.setFogColor(rgb4(6, 5, 8));

    uint8_t solid[64];
    std::fill(solid, solid + 64, 1);
    vdp.loadTile(tileSolid_, solid);
    uint8_t wheat[64] = {};
    uint8_t dirt[64] = {};
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) {
            bool stalk = ((x * 3 + y * 5) % 7) == 0 || ((x + y) % 5) == 0;
            wheat[y * 8 + x] = stalk ? ((x + y) & 1 ? 2 : 1) : 3;
            dirt[y * 8 + x] = ((x * y + x) & 3) == 0 ? 4 : 1;
        }
    vdp.loadTile(tileWheat_, wheat);
    vdp.loadTile(tileDirt_, dirt);

    const char* letters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:-!.";
    for (int i = 0; letters[i]; i++) glyphTile(vdp, fontBase_ + ord(letters[i]), letters[i]);

    barn_.w = 176;
    barn_.h = 108;
    barn_.im = alloc(vdp, barn_.w, barn_.h, barn_.p);
    barn_.rect(8, 36, 160, 70, 2);
    for (int x = 10; x < 166; x += 6) barn_.rect(x, 38, 2, 66, 1);
    barn_.rect(0, 28, 176, 10, 5);
    for (int i = 0; i < 36; i++) barn_.rect(4 + i * 2, 20 + (i % 3), 8, 8, i & 1 ? 6 : 5);
    barn_.rect(20, 8, 136, 16, 6);
    barn_.rect(48, 0, 80, 10, 5);
    barn_.rect(70, 2, 10, 14, 4);
    barn_.rect(18, 44, 140, 60, 4);
    barn_.rect(58, 48, 60, 58, 8);  // doorway hole
    barn_.rect(0, 100, 176, 8, 7);
    barn_.rect(24, 52, 16, 14, 3);
    barn_.rect(136, 52, 16, 14, 3);
    barn_.rect(28, 56, 8, 6, 8);
    barn_.rect(140, 56, 8, 6, 8);

    doorImg_.w = 52;
    doorImg_.h = 64;
    doorImg_.im = alloc(vdp, doorImg_.w, doorImg_.h, doorImg_.p);
    doorImg_.rect(0, 0, 52, 64, 2);
    for (int x = 2; x < 50; x += 6) doorImg_.rect(x, 2, 2, 60, 1);
    doorImg_.rect(24, 0, 4, 64, 1);
    doorImg_.rect(6, 16, 8, 6, 4);
    doorImg_.rect(38, 16, 8, 6, 4);
    doorImg_.rect(6, 40, 8, 6, 5);
    doorImg_.rect(38, 40, 8, 6, 5);
    doorImg_.rect(20, 30, 4, 4, 6);
    doorImg_.frame(0, 0, 52, 64, 1);

    guard_.w = 24;
    guard_.h = 40;
    guard_.im = alloc(vdp, guard_.w, guard_.h, guard_.p);
    guard_.rect(8, 0, 8, 6, 5);     // hat
    guard_.rect(6, 5, 12, 3, 4);
    guard_.rect(8, 8, 8, 7, 1);     // face
    guard_.rect(7, 15, 10, 14, 3);  // coat
    guard_.rect(4, 16, 4, 12, 2);
    guard_.rect(16, 16, 4, 12, 2);
    guard_.rect(8, 29, 3, 10, 4);
    guard_.rect(13, 29, 3, 10, 4);
    guard_.rect(7, 37, 5, 3, 7);
    guard_.rect(12, 37, 5, 3, 7);
    guard_.rect(17, 18, 2, 16, 7);  // fork haft
    guard_.rect(15, 14, 6, 2, 6);
    guard_.px(15, 12, 6);
    guard_.px(20, 12, 6);
    guard_.px(15, 13, 6);
    guard_.px(20, 13, 6);

    auto body = [&](Img& im, int coat) {
        im.w = 20;
        im.h = 34;
        im.im = alloc(vdp, im.w, im.h, im.p);
        im.rect(6, 0, 8, 6, 1);
        im.rect(5, 6, 10, 4, 5);
        im.rect(4, 10, 12, 12, coat);
        im.rect(2, 11, 3, 9, 4);
        im.rect(15, 11, 3, 9, 4);
        im.rect(6, 22, 3, 10, 3);
        im.rect(11, 22, 3, 10, 3);
        im.rect(5, 31, 5, 3, 6);
        im.rect(11, 31, 5, 3, 6);
        im.rect(9, 12, 8, 2, 4);  // club
    };
    body(foe_[0], 2);
    body(foe_[1], 3);
    body(foe_[2], 4);

    shot_.w = 5;
    shot_.h = 5;
    shot_.im = alloc(vdp, shot_.w, shot_.h, shot_.p);
    shot_.rect(1, 1, 3, 3, 1);
    shot_.px(2, 0, 3);
    shot_.px(2, 4, 2);
    shot_.px(0, 2, 3);
    shot_.px(4, 2, 2);

    crack_.w = 48;
    crack_.h = 60;
    crack_.im = alloc(vdp, crack_.w, crack_.h, crack_.p);
    for (int i = 0; i < 40; i++) crack_.px(10 + i / 3, 4 + i, 1);
    for (int i = 0; i < 24; i++) crack_.px(24 + i / 2, 20 + i, 2);
    for (int i = 0; i < 18; i++) crack_.px(30 - i / 3, 8 + i, 1);

    lamp_.w = 10;
    lamp_.h = 10;
    lamp_.im = alloc(vdp, lamp_.w, lamp_.h, lamp_.p);
    lamp_.rect(3, 3, 4, 4, 1);
    lamp_.frame(1, 1, 8, 8, 2);
}

void Game::paintSky(gs::VDP& vdp) {
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float t = y / 120.f;
        if (t > 1.f) t = 1.f;
        int r = int(2 + 10 * t);
        int g = int(2 + 6 * t);
        int b = int(6 + 2 * (1.f - t));
        vdp.lineBackdrop[y] = gs::rgb4(r, g, b);
        vdp.lineFog[y] = uint8_t(y < 70 ? (70 - y) / 10 : 0);
        vdp.road[y].on = false;
    }
}

void Game::paintYard(gs::VDP& vdp) {
    vdp.B.clear();
    vdp.A.clear();
    for (int y = 13; y < 28; y++)
        for (int x = 0; x < 40; x++) {
            uint32_t e = gs::entry((x + y) % 5 == 0 ? tileDirt_ : tileWheat_, 6);
            vdp.B.set(x, y, e);
        }
    // packed earth in front of the sill
    for (int y = 16; y < 20; y++)
        for (int x = 12; x < 28; x++) vdp.A.set(x, y, gs::entry(tileDirt_, 6));
}

void Game::clearHud(gs::VDP& vdp) { vdp.HUD.clear(); }

void Game::hudText(gs::VDP& vdp, int col, int row, const char* s, int pal) {
    for (int i = 0; s[i]; i++) {
        int o = ord(s[i]);
        if (!o) continue;
        vdp.HUD.set(col + i, row, gs::entry(fontBase_ + o, pal));
    }
}

void Game::blip(gs::APU& apu, float freq) {
    gs::FMPatch p;
    p.alg = 4;
    p.vol = 0.18f;
    p.op[0].mul = 1;
    p.op[0].level = 1;
    p.op[0].ar = 0.01f;
    p.op[0].dr = 0.12f;
    p.op[0].sl = 0.2f;
    p.op[0].rr = 0.08f;
    apu.setPatch(0, p);
    apu.keyOn(0, freq, 0.16f);
    apu.keyOff(0);
}

void Game::spawn(float x, float spd, int kind) {
    for (Foe& f : foes_) {
        if (f.alive) continue;
        f.x = x;
        f.y = 214.f;
        f.spd = spd;
        f.kind = kind;
        f.alive = true;
        return;
    }
}

void Game::drawWorld(gs::VDP& vdp) {
    vdp.clearSprites();
    for (auto& s : shots_) {
        if (!s.alive) continue;
        put(vdp, shot_.im, s.x - 3, s.y - 3, 7, 7, 5);
    }
    if (flash_ > 0) put(vdp, lamp_.im, px_ - 6, 118, 12, 12, 8);
    put(vdp, guard_.im, px_ - 12, 104, 24, 40, 3);

    int order[8];
    int n = 0;
    for (int i = 0; i < 8; i++)
        if (foes_[i].alive) order[n++] = i;
    std::sort(order, order + n, [&](int a, int b) { return foes_[a].y > foes_[b].y; });
    for (int k = 0; k < n; k++) {
        Foe& f = foes_[order[k]];
        float depth = (f.y - DOOR_Y) / 80.f;
        if (depth < 0) depth = 0;
        if (depth > 1) depth = 1;
        float h = 16.f + depth * 22.f;
        float w = h * (20.f / 34.f);
        int fog = int((1.f - depth) * 8);
        bool flip = f.x < DOOR_X;
        put(vdp, foe_[f.kind].im, f.x - w * 0.5f, f.y - h, w, h, 4, fog, flip);
    }
    if (door_ < DOOR_MAX) {
        float a = 1.f - door_ / float(DOOR_MAX);
        put(vdp, crack_.im, 136, 78, 48, 56 * (0.45f + a), 7);
    }
    put(vdp, doorImg_.im, 134, 74, 52, 64, 2);
    put(vdp, barn_.im, 72, 8, 176, 108, 1);
    put(vdp, lamp_.im, 118, 58, 14, 14, 8);
    put(vdp, lamp_.im, 188, 58, 14, 14, 8);
}

void Game::tickPlay(gs::Pad& pad, gs::APU& apu) {
    float ax = 0;
    bool fire = false;
    bool shove = false;
    if (bot_) {
        int best = -1;
        float bestY = 1e9f;
        for (int i = 0; i < 8; i++) {
            if (!foes_[i].alive) continue;
            if (foes_[i].y < bestY) {
                bestY = foes_[i].y;
                best = i;
            }
        }
        if (best >= 0) {
            float tx = foes_[best].x;
            if (px_ < tx - 3) ax = 1;
            else if (px_ > tx + 3) ax = -1;
            if (std::fabs(px_ - tx) < 10.f) fire = true;
            if (bestY < 158.f && std::fabs(px_ - tx) < 22.f) shove = true;
        }
    } else {
        if (pad.down(gs::BTN_LEFT)) ax -= 1;
        if (pad.down(gs::BTN_RIGHT)) ax += 1;
        if (std::fabs(pad.axisX) > 0.25f) ax = pad.axisX;
        fire = pad.down(gs::BTN_A) || pad.down(gs::BTN_TURBO);
        shove = pad.pressed(gs::BTN_B);
    }
    px_ += ax * 2.35f;
    if (px_ < 78) px_ = 78;
    if (px_ > 242) px_ = 242;
    if (cool_ > 0) cool_--;
    if (shove_ > 0) shove_--;
    if (flash_ > 0) flash_--;

    if (fire && cool_ == 0) {
        int best = -1;
        float bestY = 1e9f;
        for (int i = 0; i < 8; i++) {
            if (!foes_[i].alive) continue;
            if (foes_[i].y < bestY) {
                bestY = foes_[i].y;
                best = i;
            }
        }
        for (Pellet& s : shots_) {
            if (s.alive) continue;
            s.alive = true;
            s.x = px_;
            s.y = 128.f;
            s.vx = 0;
            s.vy = 3.7f;
            if (bot_ && best >= 0) {
                float dx = foes_[best].x - DOOR_X;
                float dy = foes_[best].y - DOOR_Y;
                float len = std::sqrt(dx * dx + dy * dy);
                if (len < 1) len = 1;
                float fvx = dx / len * foes_[best].spd;
                float fvy = dy / len * foes_[best].spd;
                float t = (foes_[best].y - s.y) / (s.vy - fvy);
                if (t < 1) t = 1;
                if (t > 50) t = 50;
                s.vx = (foes_[best].x + fvx * t - s.x) / t;
            }
            cool_ = 8;
            flash_ = 3;
            blip(apu, 520.f);
            break;
        }
    }
    if (shove && shove_ == 0) {
        shove_ = 36;
        apu.noiseBurst(0.25f, 1800.f, 0.12f);
        for (Foe& f : foes_) {
            if (!f.alive) continue;
            float dx = f.x - px_;
            float dy = f.y - 140.f;
            if (dx * dx + dy * dy < 38.f * 38.f) {
                f.y += 30.f;
                f.x += dx * 0.4f;
            }
        }
    }

    for (Pellet& s : shots_) {
        if (!s.alive) continue;
        s.x += s.vx;
        s.y += s.vy;
        if (s.y > 230 || s.x < 0 || s.x > 320) s.alive = false;
    }
    for (Foe& f : foes_) {
        if (!f.alive) continue;
        float dx = DOOR_X - f.x;
        float dy = DOOR_Y - f.y;
        float len = std::sqrt(dx * dx + dy * dy);
        if (len < 1.f) len = 1.f;
        f.x += dx / len * f.spd;
        f.y += dy / len * f.spd;
        for (Pellet& s : shots_) {
            if (!s.alive) continue;
            float dx = s.x - f.x;
            float dy = s.y - f.y + 8;
            float hit = 11.f + (f.y - DOOR_Y) * 0.06f;
            if (dx * dx + dy * dy < hit * hit) {
                s.alive = false;
                f.alive = false;
                blip(apu, 180.f);
                break;
            }
        }
        if (!f.alive) continue;
        float bx = f.x - DOOR_X;
        float by = f.y - DOOR_Y;
        if (by < 8.f && bx * bx + by * by < 26.f * 26.f) {
            f.alive = false;
            door_--;
            apu.noiseBurst(0.4f, 700.f, 0.2f);
            if (door_ < 0) door_ = 0;
        }
    }

    held_++;
    int gap = 42 - std::min(18, held_ / 400);
    if (held_ % gap == 1) {
        float x = 28.f + rnd() * 264.f;
        bool rush = (held_ / gap) % 5 == 4;
        spawn(x, rush ? 0.92f : 0.48f + rnd() * 0.12f, int(rnd() * 3.f) % 3);
    }
}

void Game::init(gs::System& sys) {
    sys.vdp.reset();
    sys_ = &sys;
    rng_ = 0x6d2b79f5u;
    mode_ = bot_ ? Mode::Play : Mode::Title;
    over_ = false;
    won_ = false;
    door_ = DOOR_MAX;
    held_ = 0;
    cool_ = 0;
    shove_ = 0;
    flash_ = 0;
    px_ = 160;
    for (Foe& f : foes_) f.alive = false;
    for (Pellet& s : shots_) s.alive = false;
    gs::VDP& vdp = sys.vdp;
    buildArt(vdp);
    paintSky(vdp);
    paintYard(vdp);
    vdp.hudEnabled = true;
    sys.apu.setMaster(0.7f);
    sys.apu.setEcho(0.18f, 0.25f, 0.15f);
}

void Game::frame(gs::System& sys) {
    gs::VDP& vdp = sys.vdp;
    if (mode_ == Mode::Title) {
        if (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A)) {
            mode_ = Mode::Play;
            blip(sys.apu, 330.f);
        }
    } else if (mode_ == Mode::Play) {
        tickPlay(sys.pad, sys.apu);
        if (door_ <= 0) {
            mode_ = Mode::End;
            over_ = true;
            won_ = false;
            sys.apu.noiseBurst(0.5f, 300.f, 0.4f);
        } else if (held_ >= HOLD) {
            mode_ = Mode::End;
            over_ = true;
            won_ = true;
            blip(sys.apu, 440.f);
        }
    } else if (sys.pad.pressed(gs::BTN_START) && !bot_) {
        init(sys);
        mode_ = Mode::Play;
    }

    drawWorld(vdp);
    clearHud(vdp);
    if (mode_ == Mode::Title) {
        hudText(vdp, 12, 2, "GRANARY DOOR", 0);
        hudText(vdp, 11, 4, "HOLD THREE MIN", 0);
        hudText(vdp, 13, 22, "START", 0);
        hudText(vdp, 8, 24, "ARROWS MOVE  Z FIRE", 0);
    } else {
        int left = std::max(0, HOLD - held_);
        int show = (left + 59) / 60;
        if (mode_ == Mode::End && won_) show = 0;
        char clk[16];
        std::snprintf(clk, sizeof(clk), "TIME %d:%02d", show / 60, show % 60);
        hudText(vdp, 1, 1, clk, 0);
        char bar[24];
        int n = door_;
        if (n < 0) n = 0;
        if (n > DOOR_MAX) n = DOOR_MAX;
        std::snprintf(bar, sizeof(bar), "DOOR %02d", n);
        hudText(vdp, 28, 1, bar, door_ <= 3 ? 9 : 0);
        if (mode_ == Mode::End) {
            if (won_) {
                hudText(vdp, 12, 12, "THE DOOR HELD", 0);
                hudText(vdp, 14, 14, "IT IS DONE", 0);
            } else {
                hudText(vdp, 12, 12, "THE DOOR FELL", 9);
            }
        }
    }
}

}  // namespace granary

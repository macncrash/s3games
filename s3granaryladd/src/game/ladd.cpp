#include "game/ladd.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

#include "console/gfx.h"

namespace granaryladd {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr float GRAV = 860.0f;
constexpr float JUMP_V = -378.0f;
constexpr float MAX_RUN = 122.0f;
constexpr float MAX_FALL = 440.0f;
constexpr float CLIMB = 78.0f;
constexpr float LEAD = 20.0f;
constexpr float FORGIVE = 12.0f;
constexpr float kWatch = 42.0f;
constexpr float kWorld = 1480.0f;
constexpr float kLossY = 236.0f;
constexpr float kFarX = 1188.0f;

enum { PLAT_BAY = 0, PLAT_BIN = 1, PLAT_BEAM = 2, PLAT_LOFT = 3, PLAT_N = 4 };
enum Pal {
    PAL_HUD = 0,
    PAL_WOOD = 1,
    PAL_GRAIN = 2,
    PAL_HERO = 3,
    PAL_IRON = 4,
    PAL_ALERT = 5,
    PAL_OK = 6,
    PAL_DUST = 7
};

struct Plat {
    float x, y, w;
};

// Gaps are 64px. A full-speed jump taken just before the lip still lands on the next bay.
const Plat kPlats[PLAT_N] = {
    {16, 192, 228},    // 16..244    threshing floor
    {308, 176, 216},   // 308..524   sack run, barrel patrols the middle
    {588, 164, 196},   // 588..784   tie beam
    {848, 148, 470},   // 848..1318  loft, far ladder
};

struct Lad {
    float x, y0, y1;
};

const Lad kFar = {kFarX, 52, 148};

uint16_t C(int r, int g, int b) { return gs::rgb4(r, g, b); }

float approach(float v, float target, float delta) {
    if (v < target) return std::min(target, v + delta);
    return std::max(target, v - delta);
}

void setPal(gs::VDP& v, int pal, const uint16_t* cols, int n) {
    for (int i = 0; i < n; i++) v.setColor(pal * 16 + i, cols[i]);
}

gs::Bitmap figure(const char* rows, int w, int h) {
    gs::Bitmap b(w, h);
    for (int y = 0; y < h; y++) {
        const char* row = rows + y * w;
        for (int x = 0; x < w; x++) {
            char ch = row[x];
            int c = 0;
            if (ch == 'o') c = 1;
            else if (ch == 's') c = 2;
            else if (ch == 'p') c = 3;
            else if (ch == 'b') c = 4;
            else if (ch == 'h') c = 5;
            else if (ch == 'k') c = 6;
            if (c) b.set(x, y, c);
        }
    }
    return b;
}

int solidTile(gs::TileAlloc& alloc, int c, int fleck, int seed) {
    uint8_t px[64];
    for (int i = 0; i < 64; i++) {
        uint32_t h = uint32_t(i * 17 + seed * 131) * 2246822519u;
        px[i] = uint8_t((h % 11u) == 0 ? fleck : c);
    }
    return alloc.shared(px);
}

int plankTile(gs::TileAlloc& alloc) {
    uint8_t px[64];
    std::memset(px, 3, sizeof px);
    for (int y = 0; y < 8; y++) {
        px[y * 8 + 0] = 1;
        px[y * 8 + 7] = 5;
        if (y == 0 || y == 1) px[y * 8 + 0] = 2, px[y * 8 + 1] = 2, px[y * 8 + 2] = 4;
    }
    for (int x = 0; x < 8; x++) px[x] = 2;
    return alloc.shared(px);
}

}  // namespace

float Game::barrelX() const {
    float period = 8.0f;
    float ph = std::fmod(t_, period);
    if (ph < 0) ph += period;
    float u = ph < 4.0f ? ph / 4.0f : (period - ph) / 4.0f;
    return 392.0f + u * 56.0f;
}

float Game::viewX() const { return camX_; }

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_) return 4;
    if (px_ >= kPlats[PLAT_LOFT].x) return 3;
    if (px_ >= kPlats[PLAT_BIN].x) return 2;
    return 1;
}

int Game::nearLadder() const {
    if (lock_ > 0 || onLadder_) return -1;
    if (std::abs(px_ - kFar.x) > 14.0f) return -1;
    if (py_ >= kFar.y0 - 2.0f && py_ <= kFar.y1 + 6.0f) return 0;
    return -1;
}

void Game::blip(float freq, float vol, float hold) {
    sys_->apu.tone(0, freq, vol);
    beep_ = hold;
}

void Game::textAt(const char* s, float x, float y, int pal) {
    float cx = x;
    for (const char* p = s; *p; p++) {
        unsigned char ch = (unsigned char)*p;
        if (ch >= 'a' && ch <= 'z') ch = (unsigned char)(ch - 32);
        if (ch < 32 || ch >= 128) {
            cx += 6;
            continue;
        }
        int gi = ch - 32;
        if (gi < 0 || gi >= 96 || glyphs_[gi].w == 0) {
            cx += 6;
            continue;
        }
        gs::Sprite sp;
        sp.img = glyphs_[gi];
        sp.x = int(std::lround(cx));
        sp.y = int(std::lround(y));
        sp.w = 6;
        sp.h = 8;
        sp.pal = uint8_t(pal);
        sys_->vdp.sprite(sp);
        cx += 6;
    }
}

void Game::spr(const gs::Mipped& m, float cx, float feetY, float h, int pal, bool flip) {
    if (m.h <= 0 || h < 2.0f) return;
    float sc = h / float(m.h);
    float w = float(m.w) * sc;
    gs::Sprite s;
    s.x = int16_t(std::lround(cx - w * 0.5f));
    s.y = int16_t(std::lround(feetY - h));
    s.w = int16_t(std::max(1, int(std::lround(w))));
    s.h = int16_t(std::lround(h));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::worldSpr(const gs::Mipped& m, float wx, float feetY, float h, int pal, bool flip) {
    float sx = wx - camX_;
    float sy = feetY - camY_;
    if (sx < -80 || sx > gs::SCREEN_W + 80) return;
    spr(m, sx, sy, h, pal, flip);
}

void Game::buildArt() {
    gs::VDP& v = sys_->vdp;
    const uint16_t hud[] = {C(0, 0, 0), C(15, 13, 8), C(8, 5, 2), C(15, 15, 12)};
    const uint16_t wood[] = {C(0, 0, 0), C(4, 2, 1), C(10, 6, 2), C(7, 4, 2), C(12, 8, 3), C(3, 2, 1), C(6, 5, 2)};
    const uint16_t grain[] = {C(0, 0, 0), C(12, 9, 2), C(14, 12, 4), C(8, 6, 1), C(15, 14, 6), C(5, 3, 1)};
    const uint16_t hero[] = {C(0, 0, 0), C(13, 9, 6), C(4, 7, 11), C(6, 4, 2), C(3, 2, 1), C(2, 1, 1), C(9, 7, 3)};
    const uint16_t iron[] = {C(0, 0, 0), C(6, 6, 6), C(10, 10, 9), C(3, 3, 3), C(13, 11, 6), C(8, 5, 2)};
    const uint16_t alert[] = {C(0, 0, 0), C(15, 6, 3), C(8, 2, 1)};
    const uint16_t ok[] = {C(0, 0, 0), C(12, 14, 6), C(4, 8, 3)};
    const uint16_t dust[] = {C(0, 0, 0), C(10, 8, 4), C(14, 12, 7)};
    setPal(v, PAL_HUD, hud, 4);
    setPal(v, PAL_WOOD, wood, 7);
    setPal(v, PAL_GRAIN, grain, 6);
    setPal(v, PAL_HERO, hero, 7);
    setPal(v, PAL_IRON, iron, 6);
    setPal(v, PAL_ALERT, alert, 3);
    setPal(v, PAL_OK, ok, 3);
    setPal(v, PAL_DUST, dust, 3);

    // Rows are width+NUL. 'o' skin, 's' shirt, 'p' pants, 'b' boot, 'h' hair, 'k' cap.
    const char* stand =
        "..kkkk.."
        ".hhhhhh."
        ".hooooh."
        "..oooo.."
        ".ssssss."
        "ssssssss"
        ".pppppp."
        ".pppppp."
        ".pp..pp."
        "bb....bb";
    const char* walkA =
        "..kkkk.."
        ".hhhhhh."
        ".hooooh."
        "..oooo.."
        ".ssssss."
        "ssssssss"
        ".pppppp."
        "..pppp.."
        ".pp...p."
        "bb......";
    const char* walkB =
        "..kkkk.."
        ".hhhhhh."
        ".hooooh."
        "..oooo.."
        ".ssssss."
        "ssssssss"
        ".pppppp."
        ".pppp..."
        "p...pp.."
        ".....bb.";
    const char* jumpR =
        "..kkkk.."
        ".hhhhhh."
        ".hooooh."
        "s.oooo.s"
        "ssssssss"
        ".ssssss."
        ".pppppp."
        "pp....pp"
        "b......b"
        "........";
    const char* climbA =
        ".k....k."
        "hh....hh"
        "ho.oo.oh"
        ".ssooss."
        "..pppp.."
        ".pppppp."
        "pp....pp"
        "b......b"
        "........"
        "........";
    const char* climbB =
        "..k..k.."
        ".hh..hh."
        "ho.oo.oh"
        ".ssssss."
        "..pppp.."
        ".pppppp."
        ".p....p."
        "b......b"
        "........"
        "........";
    stand_ = gs::uploadMipped(v, figure(stand, 8, 10));
    walkA_ = gs::uploadMipped(v, figure(walkA, 8, 10));
    walkB_ = gs::uploadMipped(v, figure(walkB, 8, 10));
    jump_ = gs::uploadMipped(v, figure(jumpR, 8, 10));
    climbA_ = gs::uploadMipped(v, figure(climbA, 8, 10));
    climbB_ = gs::uploadMipped(v, figure(climbB, 8, 10));

    gs::Bitmap lad(16, 96);
    lad.rect(1, 0, 3, 96, 1);
    lad.rect(12, 0, 3, 96, 2);
    for (int y = 4; y < 96; y += 8) lad.rect(2, y, 12, 2, 4);
    ladder_ = gs::uploadMipped(v, lad);

    gs::Bitmap bar(24, 24);
    bar.ellipse(12, 12, 11, 11, 1);
    bar.ellipse(12, 12, 8, 8, 2);
    bar.rect(4, 10, 16, 3, 3);
    bar.rect(6, 6, 12, 2, 4);
    barrel_ = gs::uploadMipped(v, bar);

    gs::Bitmap sack(18, 16);
    sack.ellipse(9, 9, 8, 6, 1);
    sack.ellipse(9, 8, 5, 4, 2);
    sack.rect(7, 2, 4, 3, 3);
    sack_ = gs::uploadMipped(v, sack);

    gs::Bitmap mote(4, 4);
    mote.ellipse(2, 2, 1.4f, 1.4f, 1);
    mote_ = gs::uploadMipped(v, mote);

    for (int i = 0; i < 96; i++) {
        gs::Bitmap g(6, 8);
        const uint8_t* bits = gs::glyph(char(i + 32));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (bits[y * 5 + x]) g.set(x, y, 1);
        glyphs_[i] = gs::uploadImage(v, g);
    }

    gs::TileAlloc alloc(v, 1);
    plank_ = plankTile(alloc);
    post_ = solidTile(alloc, 1, 5, 2);
    wheat_ = solidTile(alloc, 1, 2, 3);
    pit_ = solidTile(alloc, 5, 1, 4);
    beam_ = solidTile(alloc, 3, 2, 5);
    loft_ = solidTile(alloc, 4, 2, 6);
}

void Game::paint() {
    gs::VDP& v = sys_->vdp;
    v.A.resize(256, 32);
    v.B.resize(128, 32);
    v.A.clear();
    v.B.clear();
    v.A.enabled = true;
    v.B.enabled = true;
    v.HUD.clear();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.road[y].on = false;
        int g = 2 + y / 48;
        v.lineBackdrop[y] = C(g + 1, g, 1);
        v.lineFog[y] = 0;
    }

    auto put = [&](gs::Plane& p, int tx, int ty, int tile, int pal) {
        if (tx < 0 || ty < 0 || tx >= p.w || ty >= p.h || tile <= 0) return;
        p.set(tx, ty, gs::entry(tile, pal));
    };

    for (int tx = 0; tx < v.B.w; tx++) {
        put(v.B, tx, 2, beam_, PAL_WOOD);
        if ((tx % 8) == 0) {
            for (int ty = 3; ty < 18; ty++) put(v.B, tx, ty, post_, PAL_WOOD);
        }
        if ((tx % 5) == 2) put(v.B, tx, 8, wheat_, PAL_GRAIN);
    }

    for (int i = 0; i < PLAT_N; i++) {
        const Plat& p = kPlats[i];
        int x0 = int(p.x) / 8;
        int n = int(p.w) / 8;
        int y0 = int(p.y) / 8;
        int depth = (i == PLAT_LOFT) ? 6 : 4;
        for (int k = 0; k < n; k++) {
            put(v.A, x0 + k, y0, plank_, PAL_WOOD);
            for (int c = 1; c <= depth; c++) {
                int tile = (i == PLAT_BIN && (k % 4) == 1) ? wheat_ : ((k + c) & 1) ? post_ : beam_;
                put(v.A, x0 + k, y0 + c, tile, (i == PLAT_BIN) ? PAL_GRAIN : PAL_WOOD);
            }
        }
    }
    for (int i = 0; i < PLAT_N - 1; i++) {
        int x0 = int(kPlats[i].x + kPlats[i].w) / 8;
        int x1 = int(kPlats[i + 1].x) / 8;
        for (int tx = x0; tx < x1; tx++)
            for (int ty = 26; ty < 32; ty++) put(v.A, tx, ty, pit_, PAL_WOOD);
    }
    for (int tx = 0; tx < 40; tx++)
        for (int ty = 28; ty < 32; ty++)
            if (v.A.get(tx, ty) == 0) put(v.A, tx, ty, wheat_, PAL_GRAIN);
}

void Game::win() {
    if (won_) return;
    won_ = true;
    over_ = true;
    mode_ = Mode::Won;
    reason_ = "THE FAR LADDER";
    vx_ = vy_ = 0;
    onLadder_ = true;
    py_ = kFar.y0;
    shake_ = 0.25f;
    sys_->rumble(0.25f, 0.7f, 200);
    sys_->setLight(180, 140, 40);
}

void Game::lose(const char* why) {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Lost;
    over_ = true;
    won_ = false;
    reason_ = why;
    vx_ = vy_ = 0;
    onLadder_ = false;
    shake_ = 0.8f;
    sys_->apu.noiseBurst(0.4f, 380.0f, 0.24f);
    sys_->rumble(0.8f, 0.25f, 160);
    sys_->setLight(140, 40, 16);
}

void Game::begin() {
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
    reason_ = "";
    onLadder_ = false;
    grounded_ = true;
    onPlat_ = PLAT_BAY;
    face_ = 1;
    px_ = 72.0f;
    py_ = kPlats[PLAT_BAY].y;
    vx_ = vy_ = 0;
    coyote_ = 0.12f;
    jumpBuf_ = 0;
    lock_ = 0.08f;
    step_ = 0;
    foot_ = 8;
    climbSnd_ = 0;
    shake_ = 0;
    tickSec_ = -1;
    t_ = 0;
    puffs_.clear();
    camX_ = 0;
    camY_ = 8;
    blip(520.0f, 0.05f, 0.05f);
}

void Game::bot(bool& left, bool& right, bool& up, bool& down, bool& jump) const {
    left = right = up = down = jump = false;
    if (mode_ != Mode::Play) return;
    if (onLadder_) {
        up = true;
        return;
    }
    if (grounded_ && onPlat_ == PLAT_LOFT) {
        float dx = kFar.x - px_;
        if (dx > 4.0f) right = true;
        else if (dx < -4.0f) left = true;
        if (std::abs(dx) < 12.0f) up = true;
        return;
    }
    right = true;
    if (!grounded_ || onPlat_ < 0 || onPlat_ >= PLAT_N) return;
    const Plat& p = kPlats[onPlat_];
    float edge = p.x + p.w;
    if (onPlat_ == PLAT_BIN && px_ < edge - 70.0f && std::abs(px_ - barrelX()) < 26.0f && px_ < barrelX() + 6.0f) {
        jump = true;
        return;
    }
    if (px_ >= edge - LEAD && (vx_ > 100.0f || px_ >= edge - 6.0f)) jump = true;
}

void Game::play(float dt, bool left, bool right, bool up, bool down, bool jumpHeld) {
    if (jumpHeld) jumpBuf_ = 0.12f;
    if (jumpBuf_ > 0) jumpBuf_ -= dt;
    if (lock_ > 0) lock_ -= dt;

    if (onLadder_) {
        px_ = approach(px_, kFar.x, 220.0f * dt);
        vx_ = 0;
        float before = py_;
        if (up) py_ -= CLIMB * dt;
        if (down) py_ += CLIMB * dt;
        if (py_ < kFar.y0) py_ = kFar.y0;
        if (py_ > kFar.y1) py_ = kFar.y1;
        climbSnd_ -= std::abs(py_ - before);
        if (climbSnd_ <= 0 && (up || down)) {
            blip(210.0f + (int(py_) & 7) * 6.0f, 0.03f, 0.04f);
            climbSnd_ = 8.0f;
        }
        if (py_ <= kFar.y0 + 0.5f) {
            py_ = kFar.y0;
            win();
            return;
        }
        if (down && py_ >= kFar.y1 - 0.5f) {
            onLadder_ = false;
            lock_ = 0.14f;
            py_ = kFar.y1;
            px_ = kFar.x - 16.0f;
            grounded_ = true;
            onPlat_ = PLAT_LOFT;
            vy_ = 0;
            return;
        }
        return;
    }

    if (lock_ <= 0 && up && nearLadder() >= 0) {
        onLadder_ = true;
        vx_ = vy_ = 0;
        grounded_ = false;
        if (py_ > kFar.y1) py_ = kFar.y1;
        if (py_ < kFar.y0) py_ = kFar.y0;
        blip(440.0f, 0.04f, 0.04f);
        return;
    }

    float target = 0;
    if (right) target += MAX_RUN;
    if (left) target -= MAX_RUN;
    vx_ = approach(vx_, target, (grounded_ ? 1100.0f : 700.0f) * dt);
    if (left) face_ = -1;
    if (right) face_ = 1;

    if (jumpBuf_ > 0 && (grounded_ || coyote_ > 0)) {
        vy_ = JUMP_V;
        grounded_ = false;
        coyote_ = 0;
        jumpBuf_ = 0;
        blip(640.0f, 0.05f, 0.05f);
        sys_->rumble(0.08f, 0.3f, 40);
        if (puffs_.size() < 10) puffs_.push_back({px_, py_, 0.18f});
    }

    if (!grounded_) {
        vy_ += GRAV * dt;
        if (vy_ > MAX_FALL) vy_ = MAX_FALL;
    } else {
        vy_ = 0;
    }

    if (grounded_ && std::abs(vx_) > 22.0f) {
        foot_ -= std::abs(vx_) * dt;
        step_ += std::abs(vx_) * dt;
        if (foot_ <= 0) {
            blip(120.0f, 0.016f, 0.03f);
            foot_ = 14.0f;
        }
    }

    px_ += vx_ * dt;
    float prevY = py_;
    py_ += vy_ * dt;
    if (px_ < 12.0f) {
        px_ = 12.0f;
        vx_ = 0;
    }

    bool wasGround = grounded_;
    grounded_ = false;
    int landed = -1;
    if (vy_ >= 0) {
        float reach = std::max(16.0f, vy_ * dt + 8.0f);
        for (int i = 0; i < PLAT_N; i++) {
            const Plat& p = kPlats[i];
            if (px_ < p.x - FORGIVE || px_ > p.x + p.w + FORGIVE) continue;
            if (prevY <= p.y + 1.0f && py_ >= p.y && py_ - p.y <= reach) {
                if (landed < 0 || p.y < kPlats[landed].y - 0.5f) landed = i;
            }
        }
    }
    if (landed >= 0) {
        if (!wasGround && prevY < kPlats[landed].y - 6.0f && puffs_.size() < 10) puffs_.push_back({px_, py_, 0.16f});
        py_ = kPlats[landed].y;
        vy_ = 0;
        grounded_ = true;
        onPlat_ = landed;
    }
    if (grounded_) coyote_ = 0.10f;
    else coyote_ = std::max(0.0f, coyote_ - dt);

    if (mode_ != Mode::Play) return;
    if (grounded_ && onPlat_ == PLAT_BIN && std::abs(px_ - barrelX()) < 16.0f && py_ > kPlats[PLAT_BIN].y - 8.0f) {
        lose("THE BARREL");
        return;
    }
    if (t_ >= kWatch) {
        lose("THE HATCH");
        return;
    }
    if (py_ > kLossY) {
        lose("THE PIT");
        return;
    }
}

void Game::audio() {
    if (beep_ > 0) {
        beep_ -= DT;
        if (beep_ <= 0) sys_->apu.tone(0, 0, 0);
    }
    if (mode_ == Mode::Play) {
        float drone = (px_ > kPlats[PLAT_LOFT].x) ? 98.0f : 54.0f;
        sys_->apu.tone(1, drone, 0.015f);
        int sec = int(std::ceil(kWatch - t_));
        if (sec != tickSec_ && sec <= 8 && sec >= 0) blip(880.0f, 0.035f, 0.04f);
        tickSec_ = sec;
    } else if (mode_ == Mode::Title) {
        sys_->apu.tone(1, 62.0f, 0.012f);
    } else if (mode_ == Mode::Won) {
        sys_->apu.tone(1, 196.0f, 0.03f);
    } else {
        sys_->apu.tone(1, 0, 0);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    float sx = 0, sy = 0;
    if (shake_ > 0) {
        sx = std::sin(t_ * 70.0f) * shake_ * 3.0f;
        sy = std::cos(t_ * 53.0f) * shake_ * 2.0f;
    }
    v.A.scroll(int(std::lround(-(camX_ + sx))), int(std::lround(-(camY_ + sy))));
    v.B.scroll(int(std::lround(-camX_ * 0.35f)), 0);
    v.clearSprites();

    const gs::Mipped* hero = &stand_;
    if (onLadder_ || mode_ == Mode::Won) hero = (int(py_ / 6.0f) & 1) ? &climbA_ : &climbB_;
    else if (mode_ == Mode::Play && !grounded_) hero = &jump_;
    else if (mode_ == Mode::Play && std::abs(vx_) > 16.0f) hero = (int(step_ / 7.0f) & 1) ? &walkA_ : &walkB_;

    worldSpr(ladder_, kFar.x, kFar.y1 + 4.0f, kFar.y1 - kFar.y0 + 8.0f, PAL_IRON, false);
    worldSpr(barrel_, barrelX(), kPlats[PLAT_BIN].y - 2.0f, 26.0f, PAL_GRAIN, false);

    const float sacks[][2] = {{48, 192}, {78, 192}, {150, 192}, {330, 176}, {620, 164}, {900, 148}, {980, 148}};
    for (auto& s : sacks) worldSpr(sack_, s[0], s[1] - 2.0f, 16.0f, PAL_GRAIN, false);

    for (int i = 0; i < 7; i++) {
        float mx = std::fmod(40.0f + i * 180.0f + t_ * (6.0f + i), kWorld);
        float my = 40.0f + (i * 37 % 90) + std::sin(t_ * 0.8f + i) * 6.0f;
        worldSpr(mote_, mx, my, 4.0f, PAL_DUST, false);
    }
    for (const Puff& p : puffs_) worldSpr(mote_, p.x, p.y, 6.0f + (0.2f - p.life) * 10.0f, PAL_DUST, false);

    float hx = px_ - camX_;
    float hy = py_ - camY_;
    spr(*hero, hx, hy, 28.0f, PAL_HERO, face_ < 0);

    int left = int(std::ceil(std::max(0.0f, kWatch - t_)));
    char clock[16];
    std::snprintf(clock, sizeof clock, "HATCH %02d", left);
    textAt(clock, 8, 6, (left <= 8 && mode_ == Mode::Play) ? PAL_ALERT : PAL_HUD);
    textAt("FAR LADDER", 220, 6, PAL_OK);

    if (mode_ == Mode::Title) {
        textAt("S3 GRANARY LADD", 100, 70, PAL_HUD);
        textAt("REACH THE FAR LADDER", 88, 88, PAL_OK);
        textAt("ARROWS RUN  C JUMP  UP CLIMB", 64, 112, PAL_HUD);
        textAt("START", 142, 136, (int(t_ * 2) & 1) ? PAL_OK : PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        textAt("PAUSED", 136, 100, PAL_HUD);
    } else if (mode_ == Mode::Won) {
        textAt("THE FAR LADDER", 106, 78, PAL_OK);
    } else if (mode_ == Mode::Lost) {
        textAt(reason_, 120, 78, PAL_ALERT);
        textAt("SHORT OF THE LADDER", 94, 94, PAL_HUD);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt();
    paint();
    sys.vdp.setFogColor(C(4, 3, 1));
    sys.apu.setMaster(0.7f);
    sys.apu.setEcho(0.10f, 0.16f, 0.10f);
    t_ = 0;
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    grounded_ = true;
    onLadder_ = false;
    onPlat_ = PLAT_BAY;
    face_ = 1;
    px_ = 72;
    py_ = kPlats[PLAT_BAY].y;
    camX_ = 0;
    camY_ = 8;
    if (bot_) begin();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (mode_ != Mode::Pause) t_ += DT;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        float span = kWorld - gs::SCREEN_W;
        float pan = 0.5f - 0.5f * std::cos(std::min(t_, 14.0f) * 0.18f);
        camX_ = pan * span;
        camY_ = 4.0f;
        px_ = 72.0f;
        py_ = kPlats[PLAT_BAY].y;
        grounded_ = true;
        onLadder_ = false;
        face_ = 1;
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) begin();
        else if (pad.pressed(gs::BTN_MODE)) sys.quit();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Play;
            blip(400.0f, 0.04f, 0.04f);
        } else if (pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
            t_ = 0;
            sys.apu.tone(1, 0, 0);
        }
    } else if (mode_ == Mode::Won || mode_ == Mode::Lost) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C))) begin();
    } else if (mode_ == Mode::Play) {
        bool left = false, right = false, up = false, down = false, jump = false;
        if (bot_) {
            bot(left, right, up, down, jump);
        } else if (pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(240.0f, 0.04f, 0.04f);
        } else {
            left = pad.down(gs::BTN_LEFT) || pad.axisX <= -0.35f;
            right = pad.down(gs::BTN_RIGHT) || pad.axisX >= 0.35f;
            up = pad.down(gs::BTN_UP) || pad.axisY >= 0.45f;
            down = pad.down(gs::BTN_DOWN) || pad.axisY <= -0.45f;
            jump = pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B);
        }
        if (mode_ == Mode::Play) play(DT, left, right, up, down, jump);
        float maxX = kWorld - gs::SCREEN_W;
        float wantX = std::clamp(px_ - 130.0f, 0.0f, maxX);
        float wantY = std::clamp(py_ - 150.0f, 0.0f, 28.0f);
        float k = std::min(1.0f, DT * 7.0f);
        camX_ += (wantX - camX_) * k;
        camY_ += (wantY - camY_) * k;
    }

    if (shake_ > 0) shake_ = std::max(0.0f, shake_ - DT * 2.0f);
    for (Puff& p : puffs_) p.life -= DT;
    puffs_.erase(std::remove_if(puffs_.begin(), puffs_.end(), [](const Puff& p) { return p.life <= 0; }), puffs_.end());

    if (mode_ == Mode::Play && px_ > kPlats[PLAT_LOFT].x) sys.setLight(160, 120, 40);
    else if (mode_ == Mode::Play) sys.setLight(140, 80, 24);
    else if (mode_ == Mode::Title) sys.setLight(120, 70, 20);
    else if (mode_ == Mode::Won) sys.setLight(180, 150, 40);
    else if (mode_ == Mode::Lost) sys.setLight(140, 36, 16);

    audio();
    draw();
}

}  // namespace granaryladd

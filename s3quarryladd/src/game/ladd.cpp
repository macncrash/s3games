#include "game/ladd.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace quarryladd {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr float GRAV = 900.0f;
constexpr float JUMP_V = -400.0f;
constexpr float MAX_RUN = 130.0f;
constexpr float MAX_FALL = 460.0f;
constexpr float CLIMB = 86.0f;
constexpr float LEAD = 22.0f;
constexpr float FORGIVE = 12.0f;
constexpr float kWatch = 46.0f;
constexpr float kWorld = 1760.0f;
constexpr float kLossY = 320.0f;
constexpr float kDustX = 1000.0f;
constexpr float kDustPeriod = 2.8f;
constexpr float kDustOn = 0.65f;
constexpr float kHookCenter = 1240.0f;
constexpr float kHookAmp = 46.0f;
constexpr float kHookW = 0.85f;
constexpr float kSkipCenter = 442.0f;
constexpr float kSkipAmp = 52.0f;
constexpr float kSkipW = 0.62f;
constexpr float kFarX = 1588.0f;

enum { PLAT_BENCH = 0, PLAT_HAUL = 1, PLAT_SHELF = 2, PLAT_CRUSH = 3, PLAT_CABLE = 4, PLAT_RIM = 5, PLAT_N = 6 };
enum { LAD_UP = 0, LAD_FAR = 1, LAD_N = 2 };

struct Plat {
    float x, y, w;
    bool jump;
};

struct Lad {
    float x, y0, y1;
    int kind;
};

// Gaps a running jump still clears. The cable is only reached by the cut ladder.
const Plat kPlats[PLAT_N] = {
    {32, 208, 220, true},     // near bench, the watch
    {312, 208, 260, true},    // haul road, the skip
    {632, 176, 200, true},    // cut shelf
    {892, 192, 250, true},    // crusher deck
    {1060, 112, 320, true},   // cableway
    {1460, 88, 240, false}    // rim
};

const Lad kLads[LAD_N] = {
    {1100, 112, 192, LAD_UP},
    {kFarX, 36, 88, LAD_FAR}
};

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

float approach(float v, float target, float delta) {
    if (v < target) return std::min(target, v + delta);
    return std::max(target, v - delta);
}

float skipAt(float t) { return kSkipCenter + kSkipAmp * std::sin(t * kSkipW); }
float hookAt(float t) { return kHookCenter + kHookAmp * std::sin(t * kHookW); }

bool dustHot(float t) {
    float ph = std::fmod(t, kDustPeriod);
    if (ph < 0) ph += kDustPeriod;
    return ph < kDustOn;
}

bool dustClear(float t) {
    float ph = std::fmod(t, kDustPeriod);
    if (ph < 0) ph += kDustPeriod;
    if (ph < kDustOn) return false;
    return (kDustPeriod - ph) > 1.15f;
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_) return 4;
    if (onPlat_ >= PLAT_CABLE || px_ >= 1120.0f) return 3;
    if (onPlat_ == PLAT_CRUSH || px_ >= 860.0f) return 2;
    return 1;
}

const gs::Mipped& Game::heroSprite() const {
    if (onLadder_ || mode_ == Mode::Won) return (int(py_ / 6.0f) & 1) ? art_.climbA : art_.climbB;
    if (mode_ == Mode::Play && !grounded_) return art_.jump;
    if (mode_ == Mode::Play && std::abs(vx_) > 16.0f) return (int(step_ / 7.0f) & 1) ? art_.walkA : art_.walkB;
    return art_.stand;
}

int Game::nearLadder(bool down) const {
    if (lock_ > 0) return -1;
    for (int i = 0; i < LAD_N; i++) {
        const Lad& L = kLads[i];
        if (std::abs(px_ - L.x) > 14.0f) continue;
        if (!down && py_ > L.y0 + 6.0f && py_ <= L.y1 + 6.0f) return i;
        if (down && L.kind != LAD_FAR && std::abs(py_ - L.y0) <= 8.0f) return i;
    }
    return -1;
}

void Game::blip(float freq, float vol, float hold) {
    sys_->apu.tone(0, freq, vol);
    beep_ = hold;
}

void Game::mount(int i) {
    onLadder_ = true;
    lad_ = i;
    vx_ = 0;
    vy_ = 0;
    grounded_ = false;
    const Lad& L = kLads[i];
    if (py_ < L.y0 + 2.0f) py_ = L.y0 + 2.0f;
    if (py_ > L.y1) py_ = L.y1;
    blip(420.0f, 0.04f, 0.04f);
    if (i == LAD_FAR) win();
}

void Game::leave(bool top) {
    const Lad& L = kLads[lad_];
    onLadder_ = false;
    lock_ = 0.18f;
    vx_ = 0;
    vy_ = 0;
    if (top) {
        py_ = L.y0;
        px_ = L.x + 18.0f;
    } else {
        py_ = L.y1;
        px_ = L.x + 14.0f;
    }
    grounded_ = false;
    onPlat_ = -1;
    for (int i = 0; i < PLAT_N; i++) {
        const Plat& p = kPlats[i];
        if (px_ < p.x - 8.0f || px_ > p.x + p.w + 8.0f) continue;
        if (std::abs(py_ - p.y) <= 8.0f) {
            py_ = p.y;
            grounded_ = true;
            onPlat_ = i;
            coyote_ = 0.12f;
            break;
        }
    }
}

void Game::win() {
    if (won_) return;
    won_ = true;
    over_ = true;
    mode_ = Mode::Won;
    reason_ = "THE FAR LADDER";
    vx_ = 0;
    vy_ = 0;
    onLadder_ = true;
    lad_ = LAD_FAR;
    fan_ = 0;
    fanT_ = 0;
    shake_ = 0.3f;
    sys_->rumble(0.3f, 0.8f, 220);
    sys_->setLight(40, 180, 70);
}

void Game::lose(const char* why) {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Lost;
    over_ = true;
    won_ = false;
    reason_ = why;
    vx_ = 0;
    vy_ = 0;
    onLadder_ = false;
    fan_ = 0;
    fanT_ = 0;
    shake_ = 0.9f;
    sys_->apu.noiseBurst(0.45f, 420.0f, 0.26f);
    sys_->rumble(0.85f, 0.3f, 180);
    sys_->setLight(180, 30, 20);
}

void Game::begin() {
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
    reason_ = "";
    onLadder_ = false;
    lad_ = -1;
    grounded_ = true;
    onPlat_ = PLAT_BENCH;
    face_ = 1;
    px_ = 90.0f;
    py_ = kPlats[PLAT_BENCH].y;
    vx_ = vy_ = 0;
    coyote_ = 0.12f;
    jumpBuf_ = 0;
    lock_ = 0.12f;
    step_ = 0;
    foot_ = 8;
    climbSnd_ = 0;
    shake_ = 0;
    fan_ = -1;
    tickSec_ = -1;
    t_ = 0;
    puffs_.clear();
    camX_ = 0;
    camY_ = 40;
    blip(480.0f, 0.05f, 0.05f);
}

void Game::paint() {
    gs::VDP& v = sys_->vdp;
    v.A.resize(256, 64);
    v.B.resize(128, 32);
    v.A.enabled = true;
    v.B.enabled = true;
    auto put = [&](gs::Plane& p, int tx, int ty, int tile, int pal, int hf = 0) {
        if (tx < 0 || ty < 0 || tx >= p.w || ty >= p.h || tile <= 0) return;
        p.set(tx, ty, gs::entry(tile, pal, hf));
    };

    for (int ty = 0; ty < 6; ty++) {
        for (int tx = 0; tx < v.B.w; tx++) {
            uint32_t h = uint32_t(tx) * 2246822519u ^ uint32_t(ty) * 3266489917u;
            if ((h % 23u) == 0) put(v.B, tx, ty, art_.star, 10);
        }
    }
    for (int tx = 0; tx < v.B.w; tx++) {
        int top = 8 + int((tx * 3 + 1) % 5);
        for (int ty = top; ty < 18; ty++) {
            int tile = (ty == top) ? art_.farLip : ((tx + ty) & 1) ? art_.farWall : art_.strata;
            put(v.B, tx, ty, tile, PAL_ROCK);
        }
    }

    auto deck = [&](const Plat& p, int body, int bodyTile) {
        int x0 = int(p.x) / 8;
        int n = std::max(1, int(p.w) / 8);
        int y0 = int(p.y) / 8;
        for (int k = 0; k < n; k++) {
            int tx = x0 + k;
            put(v.A, tx, y0, art_.lip, PAL_ROCK);
            for (int c = 1; c <= body; c++) put(v.A, tx, y0 + c, ((k + c) & 1) ? bodyTile : art_.rockB, PAL_ROCK);
        }
    };
    deck(kPlats[PLAT_BENCH], 8, art_.rock);
    deck(kPlats[PLAT_HAUL], 8, art_.rock);
    deck(kPlats[PLAT_SHELF], 10, art_.strata);
    deck(kPlats[PLAT_CRUSH], 8, art_.rock);
    deck(kPlats[PLAT_CABLE], 1, art_.grate);
    deck(kPlats[PLAT_RIM], 6, art_.strata);

    for (int tx = 0; tx < 220; tx++) {
        for (int ty = 36; ty < 42; ty++) put(v.A, tx, ty, (ty > 38) ? art_.water : art_.pit, PAL_ROCK);
    }

    int beam0 = int(kPlats[PLAT_CABLE].x) / 8;
    int beam1 = int(kPlats[PLAT_CABLE].x + kPlats[PLAT_CABLE].w) / 8;
    int row = int(kPlats[PLAT_CABLE].y) / 8;
    for (int ty = row + 2; ty < row + 12; ty += 3) {
        put(v.A, beam0 + 2, ty, art_.grate, PAL_IRON);
        put(v.A, beam1 - 3, ty, art_.grate, PAL_IRON);
    }
}

void Game::bot(bool& left, bool& right, bool& up, bool& down, bool& jump) const {
    left = right = up = down = jump = false;
    if (mode_ != Mode::Play) return;
    if (onLadder_) {
        up = true;
        return;
    }
    if (onPlat_ == PLAT_RIM) {
        float dx = kFarX - px_;
        if (dx > 5.0f) right = true;
        else if (dx < -5.0f) left = true;
        else up = true;
        return;
    }
    if (onPlat_ == PLAT_CABLE) {
        float hx = hookAt(t_);
        float edge = kPlats[PLAT_CABLE].x + kPlats[PLAT_CABLE].w;
        if (px_ > hx + 26.0f) {
            right = true;
            if (grounded_ && px_ >= edge - LEAD && vx_ > 100.0f) jump = true;
            return;
        }
        right = true;
        float gap = hx - px_;
        if (grounded_ && vx_ > 90.0f && gap > 30.0f && gap < 62.0f) jump = true;
        return;
    }
    if (onPlat_ == PLAT_CRUSH) {
        if (px_ < kDustX - 28.0f && !dustClear(t_)) {
            if (px_ < kPlats[PLAT_CRUSH].x + 36.0f) right = true;
            else if (px_ > kPlats[PLAT_CRUSH].x + 70.0f) left = true;
            return;
        }
        float dx = kLads[LAD_UP].x - px_;
        if (dx > 4.0f) right = true;
        else if (dx < -4.0f) left = true;
        else up = true;
        return;
    }
    if (onPlat_ == PLAT_HAUL) {
        float sx = skipAt(t_);
        float gap = sx - px_;
        right = true;
        if (grounded_ && vx_ > 80.0f && gap > 28.0f && gap < 64.0f) jump = true;
        float edge = kPlats[PLAT_HAUL].x + kPlats[PLAT_HAUL].w;
        if (grounded_ && px_ >= edge - LEAD && vx_ > 100.0f) jump = true;
        return;
    }
    right = true;
    if (!grounded_ || onPlat_ < 0 || onPlat_ >= PLAT_N || !kPlats[onPlat_].jump) return;
    float edge = kPlats[onPlat_].x + kPlats[onPlat_].w;
    if (px_ >= edge - LEAD && (vx_ > 105.0f || px_ >= edge - 8.0f)) jump = true;
}

void Game::play(float dt, bool left, bool right, bool up, bool down, bool jumpPressed) {
    if (jumpPressed) jumpBuf_ = 0.12f;
    if (jumpBuf_ > 0) jumpBuf_ -= dt;
    if (lock_ > 0) lock_ -= dt;

    if (onLadder_) {
        const Lad& L = kLads[lad_];
        px_ = approach(px_, L.x, 240.0f * dt);
        vx_ = 0;
        if (up) py_ -= CLIMB * dt;
        if (down) py_ += CLIMB * dt;
        if (py_ <= L.y0) {
            if (L.kind == LAD_FAR) {
                py_ = L.y0;
                win();
                return;
            }
            leave(true);
            return;
        }
        if (py_ >= L.y1 && down) {
            leave(false);
            return;
        }
        if (up || down) {
            climbSnd_ -= dt;
            if (climbSnd_ <= 0) {
                blip(180.0f + std::fmod(py_, 24.0f) * 3.0f, 0.03f, 0.03f);
                climbSnd_ = 0.16f;
            }
        }
        if (t_ > kWatch) lose("THE WATCH");
        return;
    }

    float accel = grounded_ ? 980.0f : 640.0f;
    if (left) {
        vx_ = approach(vx_, -MAX_RUN, accel * dt);
        face_ = -1;
    } else if (right) {
        vx_ = approach(vx_, MAX_RUN, accel * dt);
        face_ = 1;
    } else {
        vx_ = approach(vx_, 0, (grounded_ ? 1400.0f : 280.0f) * dt);
    }

    if (jumpBuf_ > 0 && (grounded_ || coyote_ > 0)) {
        vy_ = JUMP_V;
        grounded_ = false;
        coyote_ = 0;
        jumpBuf_ = 0;
        blip(520.0f, 0.04f, 0.04f);
    }
    vy_ = std::min(MAX_FALL, vy_ + GRAV * dt);

    if (std::abs(vx_) > 40.0f && grounded_) {
        step_ += std::abs(vx_) * dt * 0.45f;
        foot_ -= dt * std::abs(vx_);
        if (foot_ <= 0) {
            blip(110.0f, 0.016f, 0.02f);
            foot_ = 14.0f;
        }
    }

    px_ += vx_ * dt;
    float prevY = py_;
    py_ += vy_ * dt;

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
        if (!wasGround && prevY < kPlats[landed].y - 6.0f) {
            if (puffs_.size() < 12) puffs_.push_back({px_, py_, 0.22f});
            sys_->rumble(0.12f, 0.05f, 28);
        }
        py_ = kPlats[landed].y;
        vy_ = 0;
        grounded_ = true;
        onPlat_ = landed;
    }

    if (grounded_) coyote_ = 0.11f;
    else coyote_ = std::max(0.0f, coyote_ - dt);

    int grab = -1;
    if (up) grab = nearLadder(false);
    else if (down) grab = nearLadder(true);
    if (grab >= 0) {
        mount(grab);
        return;
    }

    if (mode_ != Mode::Play) return;

    if (dustHot(t_) && std::abs(px_ - kDustX) < 16.0f && py_ > kPlats[PLAT_CRUSH].y - 70.0f &&
        py_ < kPlats[PLAT_CRUSH].y + 8.0f) {
        lose("THE DUST");
        return;
    }
    if (grounded_ && onPlat_ == PLAT_HAUL && std::abs(px_ - skipAt(t_)) < 20.0f) {
        lose("THE SKIP");
        return;
    }
    if (grounded_ && onPlat_ == PLAT_CABLE && std::abs(px_ - hookAt(t_)) < 14.0f) {
        lose("THE HOOK");
        return;
    }
    if (py_ > kLossY) {
        lose("THE SUMP");
        return;
    }
    if (t_ > kWatch) lose("THE WATCH");
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet, bool shadow) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::lround(std::clamp(cx - s.w * 0.5f, -2000.0f, 2000.0f)));
    s.y = int16_t(std::lround(std::clamp(feet ? cy - s.h : cy - s.h * 0.5f, -2000.0f, 2000.0f)));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

float Game::viewX() const {
    float view = camX_;
    if (shake_ > 0) view += std::sin(t_ * 78.0f) * shake_ * 3.0f;
    return view;
}

void Game::world(const gs::Mipped& m, float wx, float wy, float h, int pal, bool flip, bool feet, bool shadow) {
    spr(m, wx - viewX(), wy - camY_, h, pal, flip, feet, shadow);
}

void Game::text(const char* s, float x, float y, float scale, int pal) {
    float width = 0;
    for (const char* p = s; *p; p++) {
        unsigned char c = static_cast<unsigned char>(*p);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (c == ' ') width += 8.0f * scale;
        else if (c > 32 && c < 128) width += art_.glyph[c - 32].w * scale + scale;
    }
    x -= width * 0.5f;
    for (const char* p = s; *p; p++) {
        unsigned char c = static_cast<unsigned char>(*p);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (c == ' ') {
            x += 8.0f * scale;
            continue;
        }
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + g.w * scale * 0.5f, y, g.h * scale, pal, false, false, false);
        x += g.w * scale + scale;
    }
}

void Game::label(const char* s, float wx, float wy, float scale, int pal) { text(s, wx - viewX(), wy - camY_, scale, pal); }

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    float pulse = 0.55f + 0.45f * std::sin(t_ * 6.0f);
    bool hot = dustHot(t_);
    v.setColor(PAL_ALERT * 16 + 1, hot ? gs::rgb4(15, 5 + int(6 * pulse), 2) : gs::rgb4(12, 4, 2));
    v.setColor(PAL_GOLD * 16 + 1, gs::rgb4(12 + int(3 * pulse), 9, 2));

    const uint16_t zenith = gs::rgb4(3, 4, 7);
    const uint16_t haze = gs::rgb4(10, 7, 4);
    const uint16_t ochre = gs::rgb4(8, 5, 2);
    const uint16_t pit = gs::rgb4(2, 3, 4);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t c;
        if (y < 70) c = lerpC(zenith, haze, y / 70.0f);
        else if (y < 150) c = lerpC(haze, ochre, (y - 70) / 80.0f);
        else c = lerpC(ochre, pit, (y - 150) / 74.0f);
        v.lineBackdrop[y] = c;
        v.lineFog[y] = uint8_t(y > 200 ? (y - 200) / 6 : 0);
        v.road[y].on = false;
    }
    v.setFogColor(gs::rgb4(6, 4, 2));
    v.A.scroll(int(std::lround(-viewX())), int(std::lround(camY_)));
    v.B.scroll(int(std::lround(-viewX() * 0.32f)), int(std::lround(camY_ * 0.12f)));

    if (mode_ == Mode::Title) {
        text("S3 QUARRY LADD", 160, 14, 1.0f, PAL_GOLD);
        text("AT THE QUARRY", 160, 36, 0.8f, PAL_HUD);
        text("THE FAR LADDER", 160, 54, 0.9f, PAL_GOLD);
        text("MISS IT AND THE WATCH IS OVER", 160, 74, 0.58f, PAL_ALERT);
        if ((int(t_ * 2.0f) & 1) == 0) text("START", 160, 100, 1.0f, PAL_OK);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160, 78, 1.2f, PAL_HUD);
        hudC(18, "START", PAL_DIM);
    } else if (mode_ == Mode::Won) {
        text("THE FAR LADDER", 160, 148, 1.0f, PAL_HUD);
        text("THE WATCH HOLDS", 160, 170, 0.85f, PAL_OK);
    } else if (mode_ == Mode::Lost) {
        text("THE WATCH IS OVER", 160, 148, 0.85f, PAL_ALERT);
        text(reason_, 160, 170, 0.8f, PAL_HUD);
    }

    if (mode_ != Mode::Won && mode_ != Mode::Lost) {
        label("SKIP", kSkipCenter, 176, 0.55f, PAL_ALERT);
        label("DUST", kDustX, 150, 0.55f, PAL_DUST);
        label("FAR", kFarX, 18, 0.8f, PAL_GOLD);
    }

    for (const Puff& p : puffs_) {
        float h = 8.0f + (0.28f - p.life) * 20.0f;
        world(art_.dust, p.x, p.y - 2.0f, h, PAL_DUST, false, true);
    }

    float hy = py_;
    if (mode_ == Mode::Title) hy += std::sin(t_ * 2.2f) * 1.0f;
    world(heroSprite(), px_, hy, 42, PAL_MAN, face_ < 0, true);

    if (hot) {
        for (int i = 0; i < 3; i++) {
            float life = std::fmod(t_ * 1.4f + i * 0.33f, 1.0f);
            float sy = kPlats[PLAT_CRUSH].y - 6.0f - life * 52.0f;
            world(art_.dust, kDustX + (i - 1) * 6.0f, sy, 12.0f + life * 16.0f, PAL_DUST, false, false);
        }
    }

    float sx = skipAt(t_);
    world(art_.skip, sx, kPlats[PLAT_HAUL].y, 22, PAL_SKIP, std::cos(t_ * kSkipW) < 0, true);

    float hx = hookAt(t_);
    float deck = kPlats[PLAT_CABLE].y;
    world(art_.cable, hx, deck - 28.0f, 28, PAL_IRON, false, false);
    world(art_.hook, hx, deck - 8.0f, 16, PAL_IRON, std::cos(t_ * kHookW) < 0, false);

    world(art_.clock, 78, kPlats[PLAT_BENCH].y - 28, 18, PAL_GOLD, false, false);
    world(art_.boulder, 180, kPlats[PLAT_BENCH].y, 16, PAL_ROCK, false, true);
    world(art_.boulder, 700, kPlats[PLAT_SHELF].y, 14, PAL_ROCK, true, true);
    world(art_.boulder, 1540, kPlats[PLAT_RIM].y, 18, PAL_ROCK, false, true);

    const float lamps[][2] = {{60, 208}, {200, 208}, {360, 208}, {520, 208}, {700, 176}, {960, 192}, {1200, 112},
                               {1360, 112}, {1600, 88}};
    for (const float* lamp : lamps) world(art_.lamp, lamp[0], lamp[1] - 2.0f, 22, PAL_GOLD, false, true);

    float ripple = 248.0f + std::sin(t_ * 1.7f) * 6.0f;
    world(art_.ripple, ripple, 300, 8, PAL_WATER, false, false);
    world(art_.ripple, 900, 304, 8, PAL_WATER, true, false);
    world(art_.ripple, 1280, 308, 8, PAL_WATER, false, false);

    for (int i = 0; i < LAD_N; i++) {
        const Lad& L = kLads[i];
        int pal = L.kind == LAD_FAR ? PAL_GOLD : PAL_IRON;
        for (float y = L.y0 + 10.0f; y < L.y1; y += 22.0f) world(art_.ladder, L.x, y, 26, pal, false, false);
    }

    if (mode_ == Mode::Play || mode_ == Mode::Pause) {
        int left = int(std::ceil(kWatch - t_));
        if (left < 0) left = 0;
        char buf[24];
        std::snprintf(buf, sizeof buf, "WATCH %d", left);
        hud(1, 0, "QUARRY", PAL_GOLD);
        hud(30, 0, buf, left <= 10 ? PAL_ALERT : PAL_HUD);
        const char* hint = "THE FAR LADDER";
        if (onPlat_ == PLAT_BENCH) hint = "C AT THE EDGE";
        else if (onPlat_ == PLAT_HAUL) hint = "JUMP THE SKIP";
        else if (onPlat_ == PLAT_CRUSH && px_ < kDustX && !dustClear(t_)) hint = "LET THE DUST FALL";
        else if (onPlat_ == PLAT_CRUSH || (onLadder_ && lad_ == LAD_UP)) hint = "UP THE CUT";
        else if (onPlat_ == PLAT_CABLE) hint = "JUMP THE HOOK";
        else if (onPlat_ == PLAT_RIM || (onLadder_ && lad_ == LAD_FAR)) hint = "THE FAR LADDER";
        hudC(1, hint, PAL_DIM);
        hud(1, 26, "ARROWS  C JUMP  UP CLIMB", PAL_HUD);
    } else if (mode_ == Mode::Title) {
        hudC(26, "ARROWS MOVE   C JUMP   UP CLIMB", PAL_HUD);
    } else if (!bot_) {
        hudC(26, "START", PAL_HUD);
    }
}

void Game::audio() {
    if (beep_ > 0) {
        beep_ -= DT;
        if (beep_ <= 0) sys_->apu.tone(0, 0, 0);
    }
    if (fan_ >= 0) {
        static const float good[] = {392.0f, 494.0f, 587.0f, 784.0f};
        static const float bad[] = {220.0f, 174.0f, 146.0f, 110.0f};
        fanT_ += DT;
        if (fanT_ > 0.13f) {
            const float* notes = won_ ? good : bad;
            if (fan_ < 4) sys_->apu.tone(2, notes[fan_], won_ ? 0.07f : 0.04f);
            else sys_->apu.tone(2, 0, 0);
            fan_++;
            fanT_ = 0;
            if (fan_ > 8) fan_ = -1;
        }
        sys_->apu.tone(1, 0, 0);
        return;
    }
    if (mode_ == Mode::Play) {
        sys_->apu.tone(1, onPlat_ >= PLAT_CABLE ? 92.0f : 49.0f, 0.015f);
        if (dustHot(t_) && std::abs(px_ - kDustX) < 90.0f) sys_->apu.noise(0.035f, 900.0f, false);
        else sys_->apu.noise(0, 0, false);
        int sec = int(std::ceil(kWatch - t_));
        if (sec != tickSec_ && sec <= 8 && sec >= 0) {
            tickSec_ = sec;
            blip(600.0f + float(8 - sec) * 24.0f, 0.04f, 0.04f);
        }
    } else if (mode_ == Mode::Title) {
        sys_->apu.tone(1, 46.0f, 0.012f);
        sys_->apu.noise(0, 0, false);
    } else {
        sys_->apu.tone(1, 0, 0);
        sys_->apu.noise(0, 0, false);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    paint();
    sys.vdp.setFogColor(gs::rgb4(6, 4, 2));
    sys.apu.setMaster(0.7f);
    sys.apu.setEcho(0.12f, 0.18f, 0.1f);
    t_ = 0;
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    camX_ = 0;
    camY_ = 40;
    px_ = 90;
    py_ = kPlats[PLAT_BENCH].y;
    grounded_ = true;
    onPlat_ = PLAT_BENCH;
    onLadder_ = false;
    face_ = 1;
    if (bot_) begin();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (mode_ != Mode::Pause) t_ += DT;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        float pan = 0.5f - 0.5f * std::cos(std::min(t_, 8.0f) * 0.32f);
        camX_ = pan * 1300.0f;
        camY_ = 36.0f;
        px_ = 90.0f;
        py_ = kPlats[PLAT_BENCH].y;
        grounded_ = true;
        onLadder_ = false;
        face_ = 1;
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C))) begin();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Play;
            blip(440.0f, 0.04f, 0.04f);
        } else if (pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
            t_ = 0;
            sys.apu.tone(1, 0, 0);
            sys.apu.tone(2, 0, 0);
        }
    } else if (mode_ == Mode::Won || mode_ == Mode::Lost) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C))) begin();
    } else if (mode_ == Mode::Play) {
        bool left = false, right = false, up = false, down = false, jump = false;
        if (bot_) {
            bot(left, right, up, down, jump);
        } else if (pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(280.0f, 0.04f, 0.04f);
        } else {
            left = pad.down(gs::BTN_LEFT) || pad.axisX <= -0.35f;
            right = pad.down(gs::BTN_RIGHT) || pad.axisX >= 0.35f;
            up = pad.down(gs::BTN_UP) || pad.axisY >= 0.45f;
            down = pad.down(gs::BTN_DOWN) || pad.axisY <= -0.45f;
            jump = pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_TURBO);
            if (nearLadder(false) < 0 && nearLadder(true) < 0 && pad.pressed(gs::BTN_UP)) jump = true;
        }
        if (mode_ == Mode::Play) play(DT, left, right, up, down, jump);
        float maxX = kWorld - float(gs::SCREEN_W);
        float wantX = std::clamp(px_ - 120.0f, 0.0f, maxX);
        float wantY = std::clamp(py_ - 130.0f, 0.0f, 70.0f);
        float k = std::min(1.0f, DT * 7.0f);
        camX_ += (wantX - camX_) * k;
        camY_ += (wantY - camY_) * k;
    }

    if (shake_ > 0) shake_ = std::max(0.0f, shake_ - DT * 2.0f);
    for (Puff& p : puffs_) p.life -= DT;
    puffs_.erase(std::remove_if(puffs_.begin(), puffs_.end(), [](const Puff& p) { return p.life <= 0; }), puffs_.end());

    if (mode_ == Mode::Won) sys.setLight(40, 180, 70);
    else if (mode_ == Mode::Lost) sys.setLight(180, 36, 24);
    else if (mode_ == Mode::Play && kWatch - t_ < 8.0f) sys.setLight(180, 70, 20);
    else if (mode_ == Mode::Play) sys.setLight(160, 100, 40);
    else sys.setLight(140, 90, 40);

    audio();
    draw();
}

}  // namespace quarryladd

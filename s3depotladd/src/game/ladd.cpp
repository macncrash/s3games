#include "game/ladd.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace depotladd {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr float GRAV = 900.0f;
constexpr float JUMP_V = -400.0f;
constexpr float MAX_RUN = 130.0f;
constexpr float MAX_FALL = 460.0f;
constexpr float CLIMB = 86.0f;
constexpr float LEAD = 22.0f;
constexpr float FORGIVE = 12.0f;
constexpr float kWatch = 54.0f;
constexpr float kWorld = 2000.0f;
constexpr float kLossY = 300.0f;
constexpr float kVentX = 1172.0f;
constexpr float kVentPeriod = 2.7f;
constexpr float kVentOn = 0.72f;
constexpr float kHookCenter = 1472.0f;
constexpr float kHookAmp = 36.0f;
constexpr float kHookW = 0.70f;
constexpr float kMidX = 1360.0f;
constexpr float kTrapX = 1408.0f;
constexpr float kFarX = 1800.0f;

enum { PLAT_OFFICE = 0, PLAT_DOCK = 1, PLAT_BOXA = 2, PLAT_BOXB = 3, PLAT_TANK = 4, PLAT_FLAT = 5, PLAT_WALK = 6,
       PLAT_FAR = 7, PLAT_N = 8 };
enum { LAD_MID = 0, LAD_TRAP = 1, LAD_FAR = 2, LAD_N = 3 };

struct Plat {
    float x, y, w;
    bool jump;
};

struct Lad {
    float x, y0, y1;
    int kind;
};

// Gaps are 48px. A full-speed jump still lands with deck to spare.
const Plat kPlats[PLAT_N] = {
    {16, 216, 192, true},    // office porch
    {256, 232, 272, true},   // concrete dock
    {576, 200, 192, true},   // boxcar
    {816, 184, 176, true},   // second boxcar
    {1040, 216, 176, true},  // tank car, the vent
    {1264, 224, 184, true},  // flatcar, ladder only for the bot
    {1320, 120, 312, true},  // gantry
    {1680, 104, 208, false}  // departure sill
};

const Lad kLads[LAD_N] = {
    {kMidX, 120, 224, LAD_MID},
    {kTrapX, 120, 296, LAD_TRAP},
    {kFarX, 56, 104, LAD_FAR}
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

float hookAt(float t) { return kHookCenter + kHookAmp * std::sin(t * kHookW); }

bool ventHot(float t) {
    float ph = std::fmod(t, kVentPeriod);
    if (ph < 0) ph += kVentPeriod;
    return ph < kVentOn;
}

// True when the jet is off and will stay off long enough to cross it.
bool ventClear(float t) {
    float ph = std::fmod(t, kVentPeriod);
    if (ph < 0) ph += kVentPeriod;
    if (ph < kVentOn) return false;
    return (kVentPeriod - ph) > 1.05f;
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_) return 4;
    if (onPlat_ >= PLAT_WALK || px_ >= 1320.0f) return 3;
    if (onPlat_ == PLAT_TANK || px_ >= 1000.0f) return 2;
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
    py_ = kLads[LAD_FAR].y0;
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
    onPlat_ = PLAT_OFFICE;
    face_ = 1;
    px_ = 118.0f;
    py_ = kPlats[PLAT_OFFICE].y;
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
    camY_ = 58;
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

    for (int ty = 0; ty < 8; ty++) {
        for (int tx = 0; tx < v.B.w; tx++) {
            uint32_t h = uint32_t(tx) * 2246822519u ^ uint32_t(ty) * 3266489917u;
            if ((h % 19u) == 0) put(v.B, tx, ty, (h & 8u) ? art_.starB : art_.star, PAL_NIGHT);
        }
    }
    for (int tx = 0; tx < v.B.w; tx++) {
        int top = 10 + int((tx * 5 + 3) % 4);
        for (int ty = top; ty < 16; ty++) {
            int tile = (ty == top) ? art_.shed : ((tx + ty) & 1) ? art_.shedB : art_.skyWin;
            put(v.B, tx, ty, tile, PAL_NIGHT);
        }
    }
    for (int ty = 6; ty < 16; ty++) {
        put(v.B, 78, ty, art_.shed, PAL_NIGHT);
        put(v.B, 79, ty, art_.shedB, PAL_NIGHT);
    }
    put(v.B, 78, 5, art_.shed, PAL_NIGHT);
    put(v.B, 79, 5, art_.shed, PAL_NIGHT);

    for (int tx = 0; tx < 250; tx++) {
        put(v.A, tx, 33, (tx % 3 == 0) ? art_.rail : art_.tie, PAL_IRON);
        for (int ty = 34; ty < 40; ty++) put(v.A, tx, ty, art_.ballast, PAL_CONC);
    }

    auto deck = [&](const Plat& p, int body, int bodyTile, int bodyPal, int topTile, int topPal) {
        int x0 = int(p.x) / 8;
        int n = int(p.w) / 8;
        int y0 = int(p.y) / 8;
        for (int k = 0; k < n; k++) {
            int tx = x0 + k;
            put(v.A, tx, y0, topTile, topPal);
            for (int c = 1; c <= body; c++) {
                int tile = bodyTile;
                if (bodyPal == PAL_CAR && (k % 6) == 3 && c > 1 && c < body) tile = art_.carDoor;
                if (bodyPal == PAL_TANK && (k % 5) == 2) tile = art_.tankBand;
                put(v.A, tx, y0 + c, tile, bodyPal);
            }
        }
    };

    deck(kPlats[PLAT_OFFICE], 6, art_.clap, PAL_CONC, art_.safety, PAL_CONC);
    int officeRow = int(kPlats[PLAT_OFFICE].y) / 8;
    for (int tx = 3; tx < 24; tx += 3) {
        put(v.A, tx, officeRow + 2, (tx / 3) & 1 ? art_.winLit : art_.window, PAL_CONC);
        put(v.A, tx, officeRow + 4, art_.brick, PAL_CONC);
    }
    put(v.A, 12, officeRow + 3, art_.window, PAL_CONC);
    put(v.A, 13, officeRow + 3, art_.window, PAL_CONC);

    deck(kPlats[PLAT_DOCK], 3, art_.conc, PAL_CONC, art_.safety, PAL_CONC);
    deck(kPlats[PLAT_BOXA], 5, art_.car, PAL_CAR, art_.roof, PAL_CAR);
    deck(kPlats[PLAT_BOXB], 5, art_.car, PAL_CAR, art_.roof, PAL_CAR);
    deck(kPlats[PLAT_TANK], 3, art_.tank, PAL_TANK, art_.safety, PAL_CONC);
    deck(kPlats[PLAT_FLAT], 2, art_.wood, PAL_WOOD, art_.wood, PAL_WOOD);
    deck(kPlats[PLAT_WALK], 0, art_.grate, PAL_IRON, art_.grate, PAL_IRON);
    deck(kPlats[PLAT_FAR], 1, art_.grate, PAL_IRON, art_.gold, PAL_AMBER);

    int beam0 = int(kPlats[PLAT_WALK].x) / 8;
    int beam1 = int(kPlats[PLAT_WALK].x + kPlats[PLAT_WALK].w) / 8;
    int walkRow = int(kPlats[PLAT_WALK].y) / 8;
    for (int tx = beam0; tx < beam1; tx++) put(v.A, tx, walkRow - 4, art_.beam, PAL_IRON);
    for (int ty = walkRow - 3; ty < walkRow; ty++) {
        put(v.A, beam0 + 1, ty, art_.post, PAL_IRON);
        put(v.A, beam1 - 2, ty, art_.post, PAL_IRON);
    }
}

void Game::bot(bool& left, bool& right, bool& up, bool& down, bool& jump) {
    left = right = up = down = jump = false;
    if (mode_ != Mode::Play) return;
    if (onLadder_) {
        up = true;
        return;
    }
    if (onPlat_ == PLAT_FAR) {
        float dx = kFarX - px_;
        if (dx > 5.0f) right = true;
        else if (dx < -5.0f) left = true;
        else up = true;
        return;
    }
    if (onPlat_ == PLAT_WALK) {
        float hx = hookAt(t_);
        float edge = kPlats[PLAT_WALK].x + kPlats[PLAT_WALK].w;
        if (px_ > hx + 22.0f) {
            right = true;
            if (grounded_ && px_ >= edge - LEAD && vx_ > 100.0f) jump = true;
            return;
        }
        right = true;
        float gap = hx - px_;
        if (grounded_ && vx_ > 100.0f && gap > 26.0f && gap < 60.0f) jump = true;
        if (grounded_ && px_ >= edge - LEAD && vx_ > 100.0f) jump = true;
        return;
    }
    if (onPlat_ == PLAT_FLAT) {
        float dx = kMidX - px_;
        if (dx > 4.0f) right = true;
        else if (dx < -4.0f) left = true;
        else up = true;
        return;
    }
    if (onPlat_ == PLAT_TANK) {
        float edge = kPlats[PLAT_TANK].x + kPlats[PLAT_TANK].w;
        if (px_ > kVentX + 22.0f) {
            right = true;
            if (grounded_ && px_ >= edge - LEAD && vx_ > 100.0f) jump = true;
            return;
        }
        if (!ventClear(t_)) {
            if (px_ > 1124.0f) left = true;
            else if (px_ < 1104.0f) right = true;
            return;
        }
        right = true;
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
        float before = py_;
        if (up) py_ -= CLIMB * dt;
        if (down) py_ += CLIMB * dt;
        if (py_ < L.y0) py_ = L.y0;
        if (py_ > L.y1) py_ = L.y1;
        climbSnd_ -= std::abs(py_ - before);
        if (climbSnd_ <= 0 && (up || down)) {
            blip(190.0f + float(int(py_) & 7) * 8.0f, 0.03f, 0.035f);
            climbSnd_ = 8.0f;
        }
        if (L.kind == LAD_FAR && py_ <= L.y0 + 0.5f) {
            py_ = L.y0;
            win();
            return;
        }
        if (L.kind == LAD_TRAP && py_ > L.y0 + 28.0f) {
            lose("THE WRONG LADDER");
            return;
        }
        if (jumpPressed && !up) {
            onLadder_ = false;
            lock_ = 0.12f;
            vy_ = JUMP_V * 0.55f;
            grounded_ = false;
            vx_ = face_ * 40.0f;
            blip(620.0f, 0.04f, 0.04f);
            return;
        }
        if (L.kind != LAD_FAR && up && py_ <= L.y0 + 0.5f) {
            leave(true);
            return;
        }
        if (L.kind != LAD_TRAP && down && py_ >= L.y1 - 0.5f) {
            leave(false);
            return;
        }
        return;
    }

    if (lock_ <= 0 && (up || down)) {
        int i = nearLadder(!up && down);
        if (i >= 0) {
            mount(i);
            return;
        }
    }

    float target = 0;
    if (right) target += MAX_RUN;
    if (left) target -= MAX_RUN;
    vx_ = approach(vx_, target, (grounded_ ? 1400.0f : 900.0f) * dt);
    if (left) face_ = -1;
    if (right) face_ = 1;

    if (jumpBuf_ > 0 && (grounded_ || coyote_ > 0)) {
        vy_ = JUMP_V;
        grounded_ = false;
        coyote_ = 0;
        jumpBuf_ = 0;
        blip(680.0f, 0.05f, 0.045f);
        sys_->rumble(0.1f, 0.35f, 50);
        if (puffs_.size() < 12) puffs_.push_back({px_, py_, 0.18f});
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
            blip(120.0f, 0.018f, 0.03f);
            foot_ = 13.0f;
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
            if (puffs_.size() < 12) puffs_.push_back({px_, py_, 0.2f});
            sys_->rumble(0.15f, 0.06f, 30);
        }
        py_ = kPlats[landed].y;
        vy_ = 0;
        grounded_ = true;
        onPlat_ = landed;
    }

    if (grounded_) coyote_ = 0.11f;
    else coyote_ = std::max(0.0f, coyote_ - dt);

    if (mode_ != Mode::Play) return;

    if (ventHot(t_) && std::abs(px_ - kVentX) < 16.0f && py_ > kPlats[PLAT_TANK].y - 78.0f &&
        py_ < kPlats[PLAT_TANK].y + 8.0f && px_ > kPlats[PLAT_TANK].x - 8.0f &&
        px_ < kPlats[PLAT_TANK].x + kPlats[PLAT_TANK].w + 8.0f) {
        lose("THE VENT");
        return;
    }
    if (grounded_ && onPlat_ == PLAT_WALK && std::abs(px_ - hookAt(t_)) < 14.0f) {
        lose("THE HOOK");
        return;
    }
    if (py_ > kLossY || px_ < 4.0f || px_ > 1940.0f) {
        lose("OFF THE YARD");
        return;
    }
    if (t_ > kWatch) lose("OUT OF TIME");
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
    v.setColor(PAL_AMBER * 16 + 4, gs::rgb4(15, 12 + int(3 * pulse), 6 + int(5 * pulse)));
    bool hot = ventHot(t_);
    v.setColor(PAL_ALERT * 16 + 1, hot ? gs::rgb4(15, 4 + int(6 * pulse), 2) : gs::rgb4(15, 4, 3));

    const uint16_t zenith = gs::rgb4(1, 1, 3);
    const uint16_t mid = gs::rgb4(2, 2, 6);
    const uint16_t sodium = gs::rgb4(12, 6, 1);
    const uint16_t yard = gs::rgb4(2, 1, 1);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t c;
        if (y < 78) c = lerpC(zenith, mid, y / 78.0f);
        else if (y < 140) c = lerpC(mid, sodium, (y - 78) / 62.0f);
        else if (y < 186) c = lerpC(sodium, yard, (y - 140) / 46.0f);
        else c = yard;
        v.lineBackdrop[y] = c;
        v.lineFog[y] = uint8_t(y > 200 ? (y - 200) / 8 : 0);
        v.road[y].on = false;
    }
    v.setFogColor(gs::rgb4(2, 1, 1));
    v.A.scroll(int(std::lround(-viewX())), int(std::lround(camY_)));
    v.B.scroll(int(std::lround(-viewX() * 0.38f)), int(std::lround(camY_ * 0.15f)));

    if (mode_ == Mode::Title) {
        text("S3 DEPOT LADD", 160, 12, 1.0f, PAL_AMBER);
        text("AT THE DEPOT", 160, 32, 0.85f, PAL_HUD);
        text("THE FAR LADDER", 160, 50, 0.9f, PAL_AMBER);
        text("MISS IT AND THE WATCH IS OVER", 160, 70, 0.62f, PAL_ALERT);
        if ((int(t_ * 2.0f) & 1) == 0) text("START", 160, 98, 1.0f, PAL_OK);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160, 78, 1.3f, PAL_HUD);
        hudC(18, "START", PAL_DIM);
    } else if (mode_ == Mode::Won) {
        text("THE FAR LADDER", 160, 150, 1.0f, PAL_HUD);
        text("THE WATCH HOLDS", 160, 172, 0.9f, PAL_OK);
    } else if (mode_ == Mode::Lost) {
        text("THE WATCH IS OVER", 160, 150, 0.9f, PAL_ALERT);
        text(reason_, 160, 172, 0.85f, PAL_HUD);
    }

    if (mode_ != Mode::Won && mode_ != Mode::Lost) {
        label("UP", kMidX, 198, 0.7f, PAL_AMBER);
        label("DOWN", kTrapX, 104, 0.7f, PAL_ALERT);
        label("FAR", kFarX, 36, 0.85f, PAL_AMBER);
    }

    float hy = py_;
    if (mode_ == Mode::Title) hy += std::sin(t_ * 2.2f) * 1.0f;
    for (const Puff& p : puffs_) {
        float h = 8.0f + (0.28f - p.life) * 22.0f;
        world(art_.dust, p.x, p.y - 2.0f, h, PAL_CONC, false, true);
    }
    world(heroSprite(), px_, hy, 44, PAL_YARD, face_ < 0, true);
    if (grounded_ && !onLadder_ && mode_ != Mode::Title)
        world(art_.shadow, px_, py_ + 2.0f, 6, PAL_YARD, false, true, true);

    if (hot) {
        int fr = int(t_ * 10.0f);
        for (int i = 0; i < 3; i++) {
            float life = std::fmod(t_ * 1.3f + i * 0.31f, 1.0f);
            float sy = kPlats[PLAT_TANK].y - 8.0f - life * 58.0f;
            world(art_.steam[(fr + i) & 1], kVentX + (i - 1) * 3.0f, sy, 10.0f + life * 14.0f, PAL_STEAM, false, false);
        }
    }
    world(art_.glow, kVentX, kPlats[PLAT_TANK].y - 18.0f, hot ? 12.0f : 8.0f, hot ? PAL_ALERT : PAL_OK, false, false);

    float hx = hookAt(t_);
    float deck = kPlats[PLAT_WALK].y;
    world(art_.cable, hx, deck - 46.0f, 36, PAL_IRON, false, false);
    world(art_.hook, hx, deck - 16.0f, 20, PAL_IRON, std::cos(t_ * kHookW) < 0, false);

    int sigPal = (onPlat_ == PLAT_FAR || (onLadder_ && lad_ == LAD_FAR) || mode_ == Mode::Won) ? PAL_OK : PAL_ALERT;
    world(art_.signal, kFarX + 28.0f, kPlats[PLAT_FAR].y - 16.0f, 18, sigPal, false, false);
    world(art_.whistle, kFarX, kLads[LAD_FAR].y0 - 18.0f, 16, PAL_AMBER, false, false);

    for (int i = 0; i < LAD_N; i++) {
        const Lad& L = kLads[i];
        int pal = L.kind == LAD_FAR ? PAL_AMBER : L.kind == LAD_TRAP ? PAL_ALERT : PAL_IRON;
        for (float y = L.y0 + 8.0f; y < L.y1; y += 20.0f) world(art_.ladder, L.x, y, 24, pal, false, false);
    }

    const float lamps[][2] = {{70, 216}, {170, 216}, {310, 232}, {470, 232}, {640, 200}, {900, 184}, {1100, 216},
                               {1340, 224}, {1500, 120}, {1760, 104}};
    int fi = int(t_ * 8.0f) & 1;
    for (const float* lamp : lamps) {
        world(art_.lamp, lamp[0], lamp[1] - 2.0f, 26, PAL_AMBER, false, true);
        world(art_.glow, lamp[0], lamp[1] - 24.0f, fi ? 9.0f : 7.0f, PAL_AMBER, false, false);
    }

    float drum = 548.0f + 18.0f * std::sin(t_ * 1.4f);
    world(art_.drum, drum, 262, 16, PAL_DRUM, std::cos(t_ * 1.4f) < 0, true);
    world(art_.crate, 340, 232, 16, PAL_WOOD, false, true);
    world(art_.crate, 368, 232, 13, PAL_WOOD, true, true);
    world(art_.crate, 700, 200, 15, PAL_WOOD, false, true);
    world(art_.wheel, 620, 252, 14, PAL_IRON, false, true);
    world(art_.wheel, 730, 252, 14, PAL_IRON, false, true);
    world(art_.wheel, 860, 236, 14, PAL_IRON, false, true);
    world(art_.wheel, 960, 236, 14, PAL_IRON, false, true);
    world(art_.tower, 232, 262, 78, PAL_TANK, false, true);
    world(art_.clock, 78, 190, 20, PAL_AMBER, false, false);
    spr(art_.moon, 268.0f - viewX() * 0.05f, 18.0f, 16, PAL_AMBER, false, false);

    if (mode_ == Mode::Play || mode_ == Mode::Pause) {
        int left = int(std::ceil(kWatch - t_));
        if (left < 0) left = 0;
        char buf[24];
        std::snprintf(buf, sizeof buf, "WATCH %d", left);
        hud(1, 0, "DEPOT", PAL_AMBER);
        hud(31, 0, buf, left <= 10 ? PAL_ALERT : PAL_HUD);
        const char* hint = "THE FAR LADDER";
        if (onPlat_ == PLAT_OFFICE || onPlat_ == PLAT_DOCK) hint = "C AT THE EDGE";
        else if (onPlat_ == PLAT_TANK && px_ < kVentX + 20.0f && !ventClear(t_)) hint = "LET THE VENT DIE";
        else if (onPlat_ == PLAT_TANK) hint = "CROSS WHILE IT SLEEPS";
        else if (onPlat_ == PLAT_FLAT || (onLadder_ && lad_ == LAD_MID)) hint = "UP TO THE GANTRY";
        else if (onPlat_ == PLAT_WALK) hint = "JUMP THE HOOK";
        else if (onPlat_ == PLAT_FAR || (onLadder_ && lad_ == LAD_FAR)) hint = "UP THE FAR LADDER";
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
        float drone = (onPlat_ >= PLAT_WALK) ? 98.0f : 55.0f;
        sys_->apu.tone(1, drone, 0.016f);
        if (ventHot(t_) && std::abs(px_ - kVentX) < 80.0f) sys_->apu.noise(0.04f, 1800.0f, false);
        else sys_->apu.noise(0, 0, false);
        int sec = int(std::ceil(kWatch - t_));
        if (sec != tickSec_ && sec <= 8 && sec >= 0) {
            tickSec_ = sec;
            blip(620.0f + float(8 - sec) * 28.0f, 0.04f, 0.04f);
        }
    } else if (mode_ == Mode::Title) {
        sys_->apu.tone(1, 49.0f, 0.012f);
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
    sys.vdp.setFogColor(gs::rgb4(2, 1, 1));
    sys.apu.setMaster(0.7f);
    sys.apu.setEcho(0.14f, 0.22f, 0.12f);
    t_ = 0;
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    camX_ = 0;
    camY_ = 58;
    px_ = 118;
    py_ = kPlats[PLAT_OFFICE].y;
    grounded_ = true;
    onPlat_ = PLAT_OFFICE;
    onLadder_ = false;
    face_ = 1;
    if (bot_) begin();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (mode_ != Mode::Pause) t_ += DT;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        float pan = 0.5f - 0.5f * std::cos(std::min(t_, 8.0f) * 0.35f);
        camX_ = pan * 1500.0f;
        camY_ = 58.0f;
        px_ = 118.0f;
        py_ = kPlats[PLAT_OFFICE].y;
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
        float wantX = std::clamp(px_ - 130.0f, 0.0f, maxX);
        float wantY = std::clamp(py_ - 128.0f, 0.0f, 80.0f);
        float k = std::min(1.0f, DT * 7.0f);
        camX_ += (wantX - camX_) * k;
        camY_ += (wantY - camY_) * k;
    }

    if (shake_ > 0) shake_ = std::max(0.0f, shake_ - DT * 2.0f);
    for (Puff& p : puffs_) p.life -= DT;
    puffs_.erase(std::remove_if(puffs_.begin(), puffs_.end(), [](const Puff& p) { return p.life <= 0; }), puffs_.end());

    if (mode_ == Mode::Won) sys.setLight(40, 180, 70);
    else if (mode_ == Mode::Lost) sys.setLight(180, 36, 24);
    else if (mode_ == Mode::Play && kWatch - t_ < 8.0f) sys.setLight(180, 60, 24);
    else if (mode_ == Mode::Play) sys.setLight(160, 90, 30);
    else sys.setLight(120, 70, 30);

    audio();
    draw();
}

}  // namespace depotladd

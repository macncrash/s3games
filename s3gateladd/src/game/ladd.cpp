#include "game/ladd.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace gateladd {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr float GRAV = 860.0f;
constexpr float JUMP_V = -378.0f;
constexpr float MAX_RUN = 122.0f;
constexpr float MAX_FALL = 440.0f;
constexpr float CLIMB = 76.0f;
constexpr float LEAD = 20.0f;
constexpr float FORGIVE = 16.0f;
constexpr float kPeriod = 3.4f;
constexpr float kRise = 0.28f;
constexpr float kShut = 2.15f;
constexpr float kGrateX = 378.0f;
constexpr float kHoldX = 326.0f;
constexpr float kNearX = 96.0f;
constexpr float kFarX = 1196.0f;
constexpr float kDitchX = 1332.0f;
constexpr float kWatch = 48.0f;
constexpr float kWorld = 1500.0f;
constexpr float kLossY = 232.0f;
constexpr int kFarPlat = 4;

struct Plat {
    float x, y, w;
    bool jump;
};

struct Lad {
    float x, y0, y1;
    bool goal;
    const char* downWhy;
};

// Gaps are 46px. A full-speed jump still lands with deck to spare.
const Plat kPlats[] = {
    {24, 176, 210, true},    // 24..234
    {280, 176, 214, true},   // 280..494  the gate crown, portcullis
    {540, 160, 196, true},   // 540..736
    {782, 176, 188, true},   // 782..970
    {1016, 144, 380, false}  // 1016..1396 the far tower
};
constexpr int kPlatN = 5;

const Lad kLads[] = {
    {kNearX, 176, 248, false, "THE NEAR LADDER"},
    {kFarX, 64, 144, true, ""},
    {kDitchX, 144, 248, false, "THE DITCH LADDER"},
};
constexpr int kLadN = 3;

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

bool inTower(float x) { return (x >= 8.0f && x < 190.0f) || (x >= 1120.0f && x < 1420.0f); }

}  // namespace

float Game::grateDrop() const {
    float ph = std::fmod(t_, kPeriod);
    if (ph < 0) ph += kPeriod;
    if (ph < kRise) return 1.0f - ph / kRise;
    if (ph < kShut) return 0.0f;
    float u = (ph - kShut) / 0.18f;
    return u > 1.0f ? 1.0f : u;
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_) return 3;
    if (px_ > 720.0f) return 2;
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
    for (int i = 0; i < kLadN; i++) {
        const Lad& L = kLads[i];
        if (std::abs(px_ - L.x) > 15.0f) continue;
        if (!down && py_ >= L.y0 + 4.0f && py_ <= L.y1 + 3.0f) return i;
        if (down && !L.goal && std::abs(py_ - L.y0) <= 8.0f) return i;
    }
    return -1;
}

bool Game::besideRungs() const {
    for (int i = 0; i < kLadN; i++) {
        const Lad& L = kLads[i];
        if (std::abs(px_ - L.x) < 42.0f && py_ > L.y0 - 6.0f && py_ < L.y1 + 6.0f) return true;
    }
    return false;
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
    if (py_ < L.y0 + 6.0f) py_ = L.y0 + 6.0f;
    blip(460.0f, 0.04f, 0.04f);
}

void Game::leave(bool top) {
    const Lad& L = kLads[lad_];
    onLadder_ = false;
    lock_ = 0.16f;
    vx_ = 0;
    vy_ = 0;
    if (top) {
        py_ = L.y0;
        px_ = L.x + (L.x < kFarX ? 18.0f : -18.0f);
    } else {
        py_ = L.y1;
    }
    grounded_ = false;
    onPlat_ = -1;
    for (int i = 0; i < kPlatN; i++) {
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
    py_ = kLads[1].y0;
    fan_ = 0;
    fanT_ = 0;
    shake_ = 0.35f;
    sys_->rumble(0.35f, 0.85f, 240);
    sys_->setLight(40, 190, 80);
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
    shake_ = 1.0f;
    sys_->apu.noiseBurst(0.5f, 480.0f, 0.28f);
    sys_->rumble(0.9f, 0.35f, 200);
    sys_->setLight(200, 30, 20);
}

void Game::begin() {
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
    reason_ = "";
    onLadder_ = false;
    lad_ = -1;
    grounded_ = true;
    onPlat_ = 0;
    face_ = 1;
    px_ = 168.0f;
    py_ = 176.0f;
    vx_ = vy_ = 0;
    coyote_ = 0.12f;
    jumpBuf_ = 0;
    lock_ = 0.1f;
    step_ = 0;
    foot_ = 8;
    climbSnd_ = 0;
    shake_ = 0;
    fan_ = -1;
    t_ = 0;
    puffs_.clear();
    camX_ = 0;
    camY_ = 18;
    slammed_ = true;
    blip(520.0f, 0.05f, 0.05f);
}

void Game::paint() {
    gs::VDP& v = sys_->vdp;
    v.A.resize(256, 32);
    v.B.resize(128, 32);
    v.A.enabled = true;
    v.B.enabled = true;
    auto put = [&](gs::Plane& p, int tx, int ty, int tile, int pal, int hf = 0) {
        if (tx < 0 || ty < 0 || tx >= p.w || ty >= p.h || tile <= 0) return;
        p.set(tx, ty, gs::entry(tile, pal, hf));
    };

    for (int ty = 0; ty < 10; ty++) {
        for (int tx = 0; tx < v.B.w; tx++) {
            uint32_t h = uint32_t(tx) * 2246822519u ^ uint32_t(ty) * 3266489917u;
            if ((h % 17u) == 0) put(v.B, tx, ty, (h & 8u) ? art_.starB : art_.star, PAL_FAR);
        }
    }
    for (int tx = 0; tx < v.B.w; tx++) {
        int top = 12 + int((tx * 3 + 1) % 4);
        for (int ty = top; ty < 20; ty++) put(v.B, tx, ty, ty == top ? art_.farR : art_.farW, PAL_FAR);
        if (tx % 11 == 4) put(v.B, tx, top - 1, art_.farR, PAL_FAR);
    }

    for (int i = 0; i < kPlatN; i++) {
        const Plat& p = kPlats[i];
        int x0 = int(p.x) / 8;
        int n = int(p.w) / 8;
        int y0 = int(p.y) / 8;
        int courses = (i == 1) ? 1 : 6;
        for (int k = 0; k < n; k++) {
            int tx = x0 + k;
            for (int c = 1; c <= courses; c++) {
                int tile = ((tx + c) & 1) ? art_.ashB : art_.ash;
                if (c == 4 && (k % 5) == 2) tile = art_.ivy;
                put(v.A, tx, y0 + c, tile, PAL_STONE);
            }
        }
    }

    auto tower = [&](int x0, int x1, int y0) {
        for (int ty = y0; ty < 32; ty++) {
            for (int tx = x0; tx < x1; tx++) {
                bool slit = (tx == x0 + 3 || tx == x1 - 4) && ((ty - y0) % 4) == 2;
                int tile = slit ? (((tx + ty) & 1) ? art_.slitLit : art_.slit) : (((tx + ty) & 1) ? art_.ashB : art_.ash);
                put(v.A, tx, ty, tile, PAL_STONE);
            }
        }
        for (int tx = x0; tx < x1; tx += 2) put(v.A, tx, y0 - 1, art_.mer, PAL_STONE);
    };
    tower(1, 24, 11);
    tower(140, 177, 5);

    int arch0 = 300 / 8;
    int arch1 = 456 / 8;
    for (int tx = arch0; tx < arch1; tx++) {
        put(v.A, tx, 24, art_.vous, PAL_STONE);
        for (int ty = 25; ty < 31; ty++) put(v.A, tx, ty, art_.voidT, PAL_STONE);
    }
    for (int ty = 25; ty < 32; ty++) {
        put(v.A, arch0, ty, art_.jamb, PAL_STONE, 0);
        put(v.A, arch1 - 1, ty, art_.jamb, PAL_STONE, 1);
    }
    for (int tx = arch0 + 2; tx < arch1 - 2; tx++) {
        for (int ty = 28; ty < 32; ty++) put(v.A, tx, ty, art_.door, PAL_WOOD);
    }

    for (int i = 0; i < kPlatN; i++) {
        const Plat& p = kPlats[i];
        int x0 = int(p.x) / 8;
        int n = int(p.w) / 8;
        int y0 = int(p.y) / 8;
        for (int k = 0; k < n; k++) put(v.A, x0 + k, y0, art_.cope, PAL_STONE);
    }

    for (int i = 0; i < kPlatN; i++) {
        const Plat& p = kPlats[i];
        int x0 = int(p.x) / 8 + 1;
        int x1 = int(p.x + p.w) / 8 - 1;
        int y0 = int(p.y) / 8 - 1;
        for (int tx = x0; tx < x1; tx += 2) {
            float wx = tx * 8.0f + 4.0f;
            if (inTower(wx)) continue;
            if (std::abs(wx - kGrateX) < 22.0f) continue;
            put(v.A, tx, y0, art_.mer, PAL_STONE);
        }
    }

    for (int tx = 0; tx < 190; tx++) {
        if (v.A.get(tx, 31) == 0) put(v.A, tx, 31, (tx & 1) ? art_.yardB : art_.yard, PAL_STONE);
    }
    for (int i = 0; i < kPlatN - 1; i++) {
        int x0 = int(kPlats[i].x + kPlats[i].w) / 8;
        int x1 = int(kPlats[i + 1].x) / 8;
        for (int tx = x0; tx < x1; tx++) {
            for (int ty = 26; ty < 32; ty++) put(v.A, tx, ty, ((tx + ty) & 1) ? art_.yardB : art_.yard, PAL_STONE);
        }
    }
}

void Game::bot(bool& left, bool& right, bool& up, bool& down, bool& jump) const {
    left = right = up = down = jump = false;
    if (mode_ != Mode::Play) return;
    if (onLadder_) {
        if (lad_ >= 0 && kLads[lad_].goal) up = true;
        return;
    }
    if (grounded_ && onPlat_ == kFarPlat) {
        float dx = kFarX - px_;
        if (dx > 4.0f) right = true;
        else if (dx < -4.0f) left = true;
        else up = true;
        return;
    }
    float ph = std::fmod(t_, kPeriod);
    if (ph < 0) ph += kPeriod;
    bool go = ph >= 0.34f && ph < 0.78f && grateDrop() < 0.02f;
    if (grounded_ && px_ > 292.0f && px_ < kGrateX - 14.0f && !go) {
        if (px_ > kHoldX + 1.0f) left = true;
        else if (px_ < kHoldX - 8.0f) right = true;
        return;
    }
    right = true;
    if (!grounded_ || onPlat_ < 0 || onPlat_ >= kPlatN || !kPlats[onPlat_].jump) return;
    float edge = kPlats[onPlat_].x + kPlats[onPlat_].w;
    if (px_ >= edge - LEAD && (vx_ > 100.0f || px_ >= edge - 6.0f)) jump = true;
}

void Game::play(float dt, bool left, bool right, bool up, bool down, bool jumpPressed) {
    if (jumpPressed) jumpBuf_ = 0.12f;
    if (jumpBuf_ > 0) jumpBuf_ -= dt;
    if (lock_ > 0) lock_ -= dt;

    if (onLadder_) {
        const Lad& L = kLads[lad_];
        px_ = approach(px_, L.x, 220.0f * dt);
        vx_ = 0;
        float before = py_;
        if (up) py_ -= CLIMB * dt;
        if (down) py_ += CLIMB * dt;
        if (py_ < L.y0) py_ = L.y0;
        if (py_ > L.y1) py_ = L.y1;
        climbSnd_ -= std::abs(py_ - before);
        if (climbSnd_ <= 0 && (up || down)) {
            blip(200.0f + (int(py_) & 7) * 9.0f, 0.03f, 0.04f);
            climbSnd_ = 8.0f;
        }
        if (L.goal && py_ <= L.y0 + 0.4f) {
            py_ = L.y0;
            win();
            return;
        }
        if (!L.goal && py_ > L.y0 + 30.0f) {
            lose(L.downWhy);
            return;
        }
        if (jumpPressed && !up) {
            onLadder_ = false;
            lock_ = 0.12f;
            vy_ = JUMP_V * 0.55f;
            grounded_ = false;
            vx_ = face_ * 48.0f;
            blip(640.0f, 0.045f, 0.04f);
            return;
        }
        if (!L.goal && up && py_ <= L.y0 + 0.4f) {
            leave(true);
            return;
        }
        if (down && py_ >= L.y1 - 0.4f) {
            leave(false);
            return;
        }
        return;
    }

    if (lock_ <= 0 && (up || down)) {
        int i = nearLadder(!up && down);
        if (i >= 0 && (up || (down && !kLads[i].goal))) {
            mount(i);
            return;
        }
    }

    float target = 0;
    if (right) target += MAX_RUN;
    if (left) target -= MAX_RUN;
    if (grounded_ && besideRungs()) {
        if (target > 48.0f) target = 48.0f;
        if (target < -48.0f) target = -48.0f;
    }
    vx_ = approach(vx_, target, (grounded_ ? 1100.0f : 700.0f) * dt);
    if (left) face_ = -1;
    if (right) face_ = 1;

    if (jumpBuf_ > 0 && (grounded_ || coyote_ > 0)) {
        vy_ = JUMP_V;
        grounded_ = false;
        coyote_ = 0;
        jumpBuf_ = 0;
        blip(700.0f, 0.05f, 0.05f);
        sys_->rumble(0.12f, 0.4f, 60);
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
            blip(140.0f, 0.02f, 0.03f);
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
        for (int i = 0; i < kPlatN; i++) {
            const Plat& p = kPlats[i];
            if (px_ < p.x - FORGIVE || px_ > p.x + p.w + FORGIVE) continue;
            if (prevY <= p.y + 1.0f && py_ >= p.y && py_ - p.y <= reach) {
                if (landed < 0 || p.y < kPlats[landed].y - 0.5f) landed = i;
            }
        }
    }
    if (landed >= 0) {
        if (!wasGround && prevY < kPlats[landed].y - 8.0f) {
            if (puffs_.size() < 10) puffs_.push_back({px_, py_, 0.22f});
            sys_->rumble(0.18f, 0.08f, 36);
        }
        py_ = kPlats[landed].y;
        vy_ = 0;
        grounded_ = true;
        onPlat_ = landed;
    }

    if (grounded_) coyote_ = 0.10f;
    else coyote_ = std::max(0.0f, coyote_ - dt);

    if (mode_ != Mode::Play) return;
    float drop = grateDrop();
    if (drop > 0.55f && std::abs(px_ - kGrateX) < 13.0f && py_ > 150.0f && py_ < 210.0f) {
        lose("THE PORTCULLIS");
        return;
    }
    if (py_ > kLossY || px_ < 4.0f || px_ > 1460.0f) {
        lose("OFF THE WALL");
        return;
    }
    if (t_ > kWatch) lose("THE WATCH ENDED");
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
    if (cx < -160 || cx > gs::SCREEN_W + 160 || cy < -160 || cy > gs::SCREEN_H + 180) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::lround(std::clamp(cx - s.w * 0.5f, -2000.0f, 2000.0f)));
    s.y = int16_t(std::lround(std::clamp(feet ? cy - s.h : cy - s.h * 0.5f, -2000.0f, 2000.0f)));
    if (s.x > gs::SCREEN_W + 80 || s.x + s.w < -80 || s.y > gs::SCREEN_H + 80 || s.y + s.h < -80) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::world(const gs::Mipped& m, float wx, float wy, float h, int pal, bool flip, bool feet, bool shadow) {
    float view = camX_;
    if (shake_ > 0) view += std::sin(t_ * 80.0f) * shake_ * 3.0f;
    spr(m, wx - view, wy - camY_, h, pal, flip, feet, shadow);
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

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    float pulse = 0.55f + 0.45f * std::sin(t_ * 5.0f);
    v.setColor(PAL_BRASS * 16 + 4, gs::rgb4(13 + int(2 * pulse), 11 + int(3 * pulse), 4 + int(3 * pulse)));

    const uint16_t zenith = gs::rgb4(1, 1, 4);
    const uint16_t mid = gs::rgb4(3, 2, 7);
    const uint16_t dusk = gs::rgb4(10, 5, 3);
    const uint16_t yard = gs::rgb4(2, 1, 2);
    float view = camX_;
    if (shake_ > 0) view += std::sin(t_ * 80.0f) * shake_ * 3.0f;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t c;
        if (y < 90) c = lerpC(zenith, mid, y / 90.0f);
        else if (y < 150) c = lerpC(mid, dusk, (y - 90) / 60.0f);
        else if (y < 190) c = lerpC(dusk, yard, (y - 150) / 40.0f);
        else c = yard;
        v.lineBackdrop[y] = c;
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
    v.A.scroll(int(std::lround(-view)), int(std::lround(camY_)));
    v.B.scroll(int(std::lround(-view * 0.35f)), int(std::lround(camY_ * 0.15f)));

    if (mode_ == Mode::Title) {
        text("S3 GATE LADD", 160, 10, 1.25f, PAL_GOLD);
        text("YOU HAVE THE GATE", 160, 32, 0.95f, PAL_HUD);
        text("THE FAR LADDER", 160, 52, 1.05f, PAL_GOLD);
        text("ANYTHING ELSE LOSES", 160, 72, 0.8f, PAL_ALERT);
        if ((int(t_ * 2.0f) & 1) == 0) text("START", 160, 96, 1.0f, PAL_OK);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160, 78, 1.4f, PAL_HUD);
        hudC(18, "START", PAL_DIM);
    } else if (mode_ == Mode::Won) {
        text("THE FAR LADDER", 160, 150, 1.05f, PAL_GOLD);
        text("THE GATE HOLDS", 160, 172, 1.0f, PAL_OK);
    } else if (mode_ == Mode::Lost) {
        text("THE WATCH IS OVER", 160, 36, 1.0f, PAL_ALERT);
        text(reason_, 160, 60, 0.95f, PAL_HUD);
    }

    float drop = grateDrop();
    float bottom = 112.0f + drop * 108.0f;
    const float segH = 52.0f;
    for (int i = 0; i < 3; i++) {
        float cy = bottom - segH * 0.5f - i * (segH - 10.0f);
        world(art_.grate, kGrateX, cy, segH, PAL_IRON, false, false);
    }

    float hy = py_;
    if (mode_ == Mode::Title) hy += std::sin(t_ * 2.0f) * 1.2f;
    if (grounded_ && !onLadder_ && mode_ != Mode::Title)
        world(art_.shadow, px_, py_ + 2.0f, 7, PAL_FX, false, true, true);
    world(heroSprite(), px_, hy, 46, PAL_PLAYER, face_ < 0, true);
    for (const Puff& p : puffs_) {
        float h = 8.0f + (0.35f - p.life) * 26.0f;
        world(art_.dust, p.x, p.y, h, PAL_STONE, false, true);
    }

    for (int i = 0; i < kLadN; i++) {
        const Lad& L = kLads[i];
        int pal = L.goal ? PAL_BRASS : PAL_IRON;
        for (float y = L.y0; y < L.y1 - 1.0f; y += 24.0f)
            world(art_.ladder, L.x, y + 12.0f, 28, pal, false, false);
    }
    float bell = 16.0f + pulse * 2.0f;
    world(art_.bell, kFarX, kLads[1].y0 - 16.0f, bell, PAL_BRASS, false, false);

    world(art_.post, kGrateX - 26.0f, 176, 96, PAL_STONE, false, true);
    world(art_.post, kGrateX + 26.0f, 176, 96, PAL_STONE, true, true);
    world(art_.winch, kGrateX, 78, 22, PAL_WOOD, false, false);
    float ph = std::fmod(t_, kPeriod);
    if (ph < 0) ph += kPeriod;
    bool safe = drop < 0.04f && ph > kRise && ph < 1.15f;
    world(art_.gem, kGrateX, 62, safe ? 10 : 8, safe ? PAL_OK : PAL_ALERT, false, false);
    world(art_.keystone, kGrateX, 198, 16, PAL_STONE, false, false);

    float sway0 = std::sin(t_ * 1.6f) * 3.0f;
    float sway1 = std::sin(t_ * 1.6f + 1.3f) * 3.0f;
    world(art_.banner, 328 + sway0, 214, 40, PAL_WARDEN, false, false);
    world(art_.banner, 428 + sway1, 214, 40, PAL_WARDEN, false, false);

    int fi = int(t_ * 9.0f) & 1;
    const float lamps[][2] = {{86, 176}, {214, 176}, {312, 176}, {444, 176}, {620, 160},
                               {900, 176}, {1160, 144}, {1288, 144}};
    for (const float* lamp : lamps) {
        world(art_.flame[fi], lamp[0], lamp[1] - 34.0f, 11, PAL_FX, false, false);
        world(art_.torch, lamp[0], lamp[1], 28, PAL_WOOD, false, true);
    }
    world(art_.flame[fi], 318, 214, 12, PAL_FX, false, false);
    world(art_.torch, 318, 236, 30, PAL_WOOD, false, true);
    world(art_.flame[fi], 438, 214, 12, PAL_FX, false, false);
    world(art_.torch, 438, 236, 30, PAL_WOOD, false, true);

    float g0 = 356.0f + 36.0f * std::sin(t_ * 0.5f);
    float g1 = 392.0f + 28.0f * std::sin(t_ * 0.5f + 2.0f);
    int gs0 = std::cos(t_ * 0.5f) >= 0 ? 0 : 1;
    int gs1 = std::cos(t_ * 0.5f + 2.0f) >= 0 ? 1 : 0;
    world(art_.guard[gs0], g0, 242, 42, PAL_WARDEN, std::cos(t_ * 0.5f) < 0, true);
    world(art_.guard[gs1], g1, 242, 42, PAL_WARDEN, std::cos(t_ * 0.5f + 2.0f) < 0, true);

    spr(art_.moon, 268 - view * 0.04f, 30, 22, PAL_GOLD, false, false);

    if (mode_ != Mode::Won && mode_ != Mode::Lost) {
        text("DOWN", kNearX - view, 156 - camY_, 0.85f, PAL_ALERT);
        text("DOWN", kDitchX - view, 124 - camY_, 0.85f, PAL_ALERT);
        text("FAR", kFarX - view, 40 - camY_, 1.0f, PAL_GOLD);
    }

    if (mode_ == Mode::Play || mode_ == Mode::Pause) {
        int left = int(std::ceil(kWatch - t_));
        if (left < 0) left = 0;
        char buf[24];
        std::snprintf(buf, sizeof buf, "HORN %d", left);
        hud(1, 0, "GATE", PAL_GOLD);
        hud(32, 0, buf, left <= 10 ? PAL_ALERT : PAL_HUD);
        const char* hint = "THE FAR LADDER";
        if (px_ < 250.0f) hint = "C AT THE EDGE";
        else if (px_ > 290.0f && px_ < 430.0f && !safe) hint = "LET THE GRATE RISE";
        else if (px_ > 1080.0f) hint = "UP THE FAR LADDER";
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
        static const float good[] = {392.0f, 523.0f, 659.0f, 784.0f};
        static const float bad[] = {220.0f, 174.0f, 146.0f, 110.0f};
        fanT_ += DT;
        if (fanT_ > 0.14f) {
            const float* notes = won_ ? good : bad;
            if (fan_ < 4) sys_->apu.tone(2, notes[fan_], won_ ? 0.07f : 0.045f);
            else sys_->apu.tone(2, 0, 0);
            fan_++;
            fanT_ = 0;
            if (fan_ > 8) fan_ = -1;
        }
        sys_->apu.tone(1, 0, 0);
        return;
    }
    if (mode_ == Mode::Play) {
        float drone = (px_ > 1000.0f) ? 82.0f : 55.0f;
        sys_->apu.tone(1, drone, 0.018f);
        if (kWatch - t_ < 10.0f && kWatch - t_ > 0) {
            float frac = (kWatch - t_) - std::floor(kWatch - t_);
            if (frac < DT) blip(880.0f, 0.04f, 0.04f);
        }
    } else if (mode_ == Mode::Title) {
        sys_->apu.tone(1, 49.0f, 0.014f);
    } else {
        sys_->apu.tone(1, 0, 0);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    paint();
    sys.vdp.setFogColor(gs::rgb4(1, 1, 2));
    sys.apu.setMaster(0.7f);
    sys.apu.setEcho(0.12f, 0.2f, 0.12f);
    t_ = 0;
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    camX_ = 72;
    camY_ = 18;
    px_ = 168;
    py_ = 176;
    grounded_ = true;
    onPlat_ = 0;
    if (bot_) begin();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const float dt = DT;
    t_ += dt;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        float pan = 0.5f - 0.5f * std::cos(t_ * 0.22f);
        camX_ = 72.0f + pan * 920.0f;
        camY_ = 18.0f;
        px_ = 168.0f;
        py_ = 176.0f;
        grounded_ = true;
        onLadder_ = false;
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C))) begin();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) sys.quit();
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
        if (mode_ == Mode::Play) play(dt, left, right, up, down, jump);
        float maxX = kWorld - gs::SCREEN_W;
        float wantX = std::clamp(px_ - 150.0f, 0.0f, maxX);
        float wantY = std::clamp(py_ - 148.0f, 0.0f, 32.0f);
        float k = std::min(1.0f, dt * 6.0f);
        camX_ += (wantX - camX_) * k;
        camY_ += (wantY - camY_) * k;
    }

    float drop = grateDrop();
    if (drop > 0.88f) {
        if (!slammed_) {
            slammed_ = true;
            sys.apu.noiseBurst(0.32f, 160.0f, 0.16f);
            shake_ = std::max(shake_, 0.65f);
            if (puffs_.size() < 8) {
                puffs_.push_back({kGrateX - 10.0f, 176.0f, 0.32f});
                puffs_.push_back({kGrateX + 10.0f, 176.0f, 0.32f});
            }
        }
    } else if (drop < 0.08f) {
        slammed_ = false;
    }
    if (shake_ > 0) shake_ = std::max(0.0f, shake_ - dt * 2.2f);
    for (Puff& p : puffs_) p.life -= dt;
    puffs_.erase(std::remove_if(puffs_.begin(), puffs_.end(), [](const Puff& p) { return p.life <= 0; }), puffs_.end());

    if (mode_ == Mode::Play && px_ > 1000.0f) sys.setLight(170, 120, 40);
    else if (mode_ == Mode::Play) sys.setLight(120, 70, 36);
    else if (mode_ == Mode::Title) sys.setLight(90, 60, 40);
    else if (mode_ == Mode::Won) sys.setLight(40, 180, 70);
    else if (mode_ == Mode::Lost) sys.setLight(180, 30, 24);

    audio();
    draw();
}

}  // namespace gateladd

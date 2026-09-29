#include "game/ladd.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace orchardladd {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr float GRAV = 820.0f;
constexpr float JUMP_V = -390.0f;
constexpr float MAX_RUN = 128.0f;
constexpr float MAX_FALL = 460.0f;
constexpr float CLIMB = 72.0f;
constexpr float LEAD = 22.0f;
constexpr float FORGIVE = 14.0f;
constexpr float kWatch = 46.0f;
constexpr float kWorld = 1560.0f;
constexpr float kLossY = 268.0f;
constexpr float kWaspX = 690.0f;
constexpr float kFarX = 1188.0f;
constexpr float kBarrelSpan = 1.75f;

enum { PLAT_ROW = 0, PLAT_CRATE = 1, PLAT_LIMB = 2, PLAT_WAGON = 3, PLAT_LOFT = 4, PLAT_N = 5 };
enum { LAD_DITCH = 0, LAD_FAR = 1, LAD_N = 2 };

struct Plat {
    float x, y, w;
    bool jump;
};

struct Lad {
    float x, y0, y1;
    bool goal;
    const char* downWhy;
};

// A full run and a late jump still clears each gap. The loft sits above the wagon.
const Plat kPlats[PLAT_N] = {
    {20, 200, 210, true},    // 20..230    grass row
    {286, 182, 234, true},   // 286..520   cider crates
    {516, 150, 136, true},   // 516..652   laden limb
    {760, 198, 168, true},   // 760..928   wagon bed
    {996, 146, 380, false}   // 996..1376  barn loft
};

const Lad kLads[LAD_N] = {
    {150, 200, 276, false, "THE DITCH"},
    {kFarX, 64, 146, true, ""}
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

float wrapPhase(float t, float period) {
    float ph = std::fmod(t, period);
    if (ph < 0) ph += period;
    return ph;
}

}  // namespace

float Game::barrelX() const {
    // Sits on the lip of the crates. A full jump clears it; walking into it does not.
    return 502.0f + std::sin(t_ * 1.7f) * 4.0f;
}

bool Game::barrelGoingRight() const {
    return wrapPhase(t_, kBarrelSpan * 2.0f) < kBarrelSpan;
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_) return 4;
    if (px_ >= kPlats[PLAT_LOFT].x) return 3;
    if (px_ >= kPlats[PLAT_CRATE].x) return 2;
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
        if (std::abs(px_ - L.x) > 16.0f) continue;
        if (!down && py_ >= L.y0 + 4.0f && py_ <= L.y1 + 6.0f) return i;
        if (down && !L.goal && std::abs(py_ - L.y0) <= 8.0f) return i;
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
    lock_ = 0.16f;
    vx_ = 0;
    vy_ = 0;
    if (top) {
        py_ = L.y0;
        px_ = L.x + 18.0f;
    } else {
        py_ = L.y1;
        px_ = L.x + (L.goal ? -16.0f : 14.0f);
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
    onPlat_ = PLAT_ROW;
    face_ = 1;
    px_ = 86.0f;
    py_ = kPlats[PLAT_ROW].y;
    vx_ = vy_ = 0;
    coyote_ = 0.12f;
    jumpBuf_ = 0;
    lock_ = 0.08f;
    step_ = 0;
    foot_ = 8;
    climbSnd_ = 0;
    shake_ = 0;
    fan_ = -1;
    tickSec_ = -1;
    t_ = 0;
    puffs_.clear();
    camX_ = 8;
    camY_ = 20;
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

    for (int ty = 0; ty < 14; ty++) {
        for (int tx = 0; tx < v.B.w; tx++) {
            uint32_t h = uint32_t(tx) * 2246822519u ^ uint32_t(ty) * 3266489917u;
            if ((h % 23u) == 0) put(v.B, tx, ty, (h & 4u) ? art_.blossom : art_.sky, PAL_SKY);
        }
    }
    for (int tx = 0; tx < v.B.w; tx++) {
        int top = 12 + int((tx * 3) % 4);
        if ((tx % 7) == 0) top -= 3;
        for (int ty = top; ty < 22; ty++) {
            int tile = (ty == top) ? art_.leaf : (((tx + ty) & 1) ? art_.leafB : art_.leaf);
            put(v.B, tx, ty, tile, PAL_LEAF);
        }
    }

    for (int i = 0; i < PLAT_N; i++) {
        const Plat& p = kPlats[i];
        int x0 = int(p.x) / 8;
        int n = int(p.w) / 8;
        int y0 = int(p.y) / 8;
        int courses = (i == PLAT_LOFT) ? 4 : 3;
        for (int k = 0; k < n; k++) {
            int tx = x0 + k;
            int cap = (i == PLAT_LOFT) ? art_.board : (i == PLAT_ROW ? art_.grass : art_.crateT);
            int pal = (i == PLAT_LOFT) ? PAL_BARN : PAL_ORCH;
            put(v.A, tx, y0, cap, pal);
            for (int c = 1; c <= courses; c++) {
                int tile = (i == PLAT_LOFT) ? (((tx + c) & 1) ? art_.barnB : art_.barn)
                                            : (((tx + c) & 1) ? art_.soilB : art_.soil);
                int body = (i == PLAT_LOFT) ? PAL_BARN : PAL_WOOD;
                put(v.A, tx, y0 + c, tile, body);
            }
        }
    }

    for (int tx = 0; tx < 200; tx++) {
        for (int ty = 28; ty < 32; ty++) {
            if (v.A.get(tx, ty) == 0) put(v.A, tx, ty, art_.grass, PAL_ORCH);
        }
    }
    for (int i = 0; i < PLAT_N - 1; i++) {
        int x0 = int(kPlats[i].x + kPlats[i].w) / 8;
        int x1 = int(kPlats[i + 1].x) / 8;
        for (int tx = x0; tx < x1; tx++) {
            for (int ty = 26; ty < 32; ty++) put(v.A, tx, ty, art_.pit, PAL_DITCH);
        }
    }
}

void Game::bot(bool& left, bool& right, bool& up, bool& down, bool& jump) const {
    left = right = up = down = jump = false;
    if (mode_ != Mode::Play) return;
    if (onLadder_) {
        if (lad_ == LAD_FAR) up = true;
        return;
    }
    if (grounded_ && onPlat_ == PLAT_LOFT) {
        float dx = kFarX - px_;
        if (dx > 3.0f) right = true;
        else if (dx < -3.0f) left = true;
        if (std::abs(dx) < 12.0f) up = true;
        return;
    }
    if (grounded_ && onPlat_ == PLAT_CRATE) {
        right = true;
        // Leave the deck before the cider cask. The next limb is the landing.
        if (px_ >= 466.0f && vx_ > 100.0f) jump = true;
        return;
    }
    right = true;
    if (!grounded_ || onPlat_ < 0 || onPlat_ >= PLAT_N || !kPlats[onPlat_].jump) return;
    float edge = kPlats[onPlat_].x + kPlats[onPlat_].w;
    if (px_ >= edge - LEAD && (vx_ > 100.0f || px_ >= edge - 6.0f)) jump = true;
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
            blip(210.0f + (int(py_) & 7) * 9.0f, 0.03f, 0.04f);
            climbSnd_ = 8.0f;
        }
        if (L.goal && py_ <= L.y0 + 0.4f) {
            py_ = L.y0;
            win();
            return;
        }
        if (!L.goal && py_ > L.y0 + 28.0f) {
            lose(L.downWhy);
            return;
        }
        if (jumpPressed && !up) {
            onLadder_ = false;
            lock_ = 0.12f;
            vy_ = JUMP_V * 0.55f;
            grounded_ = false;
            vx_ = face_ * 40.0f;
            blip(640.0f, 0.04f, 0.04f);
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
    vx_ = approach(vx_, target, (grounded_ ? 1100.0f : 700.0f) * dt);
    if (left) face_ = -1;
    if (right) face_ = 1;

    if (jumpBuf_ > 0 && (grounded_ || coyote_ > 0)) {
        vy_ = JUMP_V;
        grounded_ = false;
        coyote_ = 0;
        jumpBuf_ = 0;
        blip(700.0f, 0.05f, 0.05f);
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
            blip(160.0f, 0.018f, 0.03f);
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
            if (puffs_.size() < 12) puffs_.push_back({px_, py_, 0.2f});
            sys_->rumble(0.16f, 0.06f, 30);
        }
        py_ = kPlats[landed].y;
        vy_ = 0;
        grounded_ = true;
        onPlat_ = landed;
    }

    if (grounded_) coyote_ = 0.10f;
    else coyote_ = std::max(0.0f, coyote_ - dt);

    if (mode_ != Mode::Play) return;
    if (grounded_ && onPlat_ == PLAT_CRATE && std::abs(px_ - barrelX()) < 14.0f) {
        lose("THE BARREL");
        return;
    }
    if (std::abs(px_ - kWaspX) < 22.0f && py_ > 124.0f && py_ < 210.0f) {
        lose("THE WASPS");
        return;
    }
    if (py_ > kLossY || px_ < 4.0f || px_ > 1480.0f) {
        lose("OFF THE ORCHARD");
        return;
    }
    if (t_ > kWatch) lose("THE ORCHARD CLOSED");
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
    if (cx < -180 || cx > gs::SCREEN_W + 180 || cy < -180 || cy > gs::SCREEN_H + 200) return;
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

float Game::viewX() const {
    float view = camX_;
    if (shake_ > 0) view += std::sin(t_ * 76.0f) * shake_ * 3.0f;
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

void Game::worldText(const char* s, float wx, float wy, float scale, int pal) {
    text(s, wx - viewX(), wy - camY_, scale, pal);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    float pulse = 0.55f + 0.45f * std::sin(t_ * 5.0f);
    v.setColor(PAL_WASP * 16 + 2, gs::rgb4(15, 10 + int(4 * pulse), 2));
    v.setColor(PAL_GOLD * 16 + 1, gs::rgb4(14, 11 + int(3 * pulse), 3));

    const uint16_t zenith = gs::rgb4(6, 10, 15);
    const uint16_t mid = gs::rgb4(10, 13, 15);
    const uint16_t gold = gs::rgb4(14, 12, 7);
    const uint16_t grass = gs::rgb4(3, 8, 3);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t c;
        if (y < 80) c = lerpC(zenith, mid, y / 80.0f);
        else if (y < 140) c = lerpC(mid, gold, (y - 80) / 60.0f);
        else if (y < 190) c = lerpC(gold, grass, (y - 140) / 50.0f);
        else c = grass;
        v.lineBackdrop[y] = c;
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
    v.A.scroll(int(std::lround(-viewX())), int(std::lround(camY_)));
    v.B.scroll(int(std::lround(-viewX() * 0.35f)), int(std::lround(camY_ * 0.12f)));

    if (mode_ == Mode::Title) {
        text("S3 ORCHARD LADD", 160, 8, 1.05f, PAL_GOLD);
        text("YOU HAVE THE ORCHARD", 160, 30, 0.75f, PAL_HUD);
        text("THE FAR LADDER", 160, 48, 1.0f, PAL_GOLD);
        text("ANYTHING ELSE IS A LOSS", 160, 68, 0.7f, PAL_ALERT);
        if ((int(t_ * 2.0f) & 1) == 0) text("START", 160, 92, 1.0f, PAL_OK);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160, 78, 1.3f, PAL_HUD);
        hudC(18, "START", PAL_DIM);
    } else if (mode_ == Mode::Won) {
        text("THE FAR LADDER", 160, 148, 1.05f, PAL_GOLD);
        text("THE ORCHARD IS YOURS", 160, 170, 0.8f, PAL_OK);
    } else if (mode_ == Mode::Lost) {
        text("THE ORCHARD IS OVER", 160, 36, 0.9f, PAL_ALERT);
        text(reason_, 160, 58, 0.9f, PAL_HUD);
    }

    const float trees[] = {40, 180, 340, 560, 820, 1040, 1280};
    for (float tx : trees) world(art_.tree, tx, 168, 78, PAL_LEAF, false, true);

    if (mode_ != Mode::Won && mode_ != Mode::Lost) {
        worldText("DITCH", 150, 168, 0.6f, PAL_ALERT);
        worldText("BARREL", 460, 150, 0.6f, PAL_WOOD);
        worldText("WASPS", kWaspX, 96, 0.6f, PAL_WASP);
        worldText("FAR", kFarX, 40, 0.9f, PAL_GOLD);
    }

    float hy = py_;
    if (mode_ == Mode::Title) hy += std::sin(t_ * 2.2f) * 1.1f;
    world(heroSprite(), px_, hy, 42, PAL_PICK, face_ < 0, true);

    for (int i = 0; i < LAD_N; i++) {
        const Lad& L = kLads[i];
        int pal = L.goal ? PAL_GOLD : PAL_WOOD;
        for (float y = L.y0; y < L.y1 - 1.0f; y += 24.0f)
            world(art_.ladder, L.x, y + 12.0f, 28, pal, false, false);
    }

    world(art_.barrel, barrelX(), kPlats[PLAT_CRATE].y, 22, PAL_WOOD, !barrelGoingRight(), true);
    world(art_.hive, kWaspX, 108, 18, PAL_GOLD, false, false);
    float bob = std::sin(t_ * 9.0f) * 6.0f;
    world(art_.wasp, kWaspX + bob, 124, 16, PAL_WASP, bob > 0, false);
    world(art_.wasp, kWaspX - bob * 0.6f, 132, 12, PAL_WASP, bob < 0, false);

    const float apples[] = {70, 210, 400, 600, 880, 1100, 1320};
    for (int i = 0; i < 7; i++) world(art_.apple, apples[i], 176 - (i % 3) * 8.0f, 10, PAL_APPLE, false, false);
    world(art_.crate, 800, kPlats[PLAT_WAGON].y, 16, PAL_WOOD, false, true);
    world(art_.crate, 860, kPlats[PLAT_WAGON].y, 16, PAL_WOOD, true, true);

    for (const Puff& p : puffs_) {
        float h = 6.0f + (0.28f - p.life) * 20.0f;
        world(art_.apple, p.x, p.y - 8.0f, h, PAL_FX, false, true);
    }

    if (mode_ == Mode::Play || mode_ == Mode::Pause) {
        int left = int(std::ceil(kWatch - t_));
        if (left < 0) left = 0;
        char buf[24];
        std::snprintf(buf, sizeof buf, "ROWS %d", left);
        hud(1, 0, "ORCHARD", PAL_GOLD);
        hud(30, 0, buf, left <= 10 ? PAL_ALERT : PAL_HUD);
        const char* hint = "THE FAR LADDER";
        if (onPlat_ == PLAT_CRATE && px_ < 400.0f) hint = "WAIT ON THE BARREL";
        else if (std::abs(px_ - 150.0f) < 24.0f && py_ > 180.0f) hint = "NOT THE DITCH";
        else if (px_ > 600.0f && px_ < 740.0f) hint = "CLEAR THE WASPS";
        else if (px_ > 1050.0f) hint = "UP THE FAR LADDER";
        else if (px_ < 200.0f) hint = "C AT THE EDGE";
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
        float drone = (px_ > 996.0f) ? 98.0f : 54.0f;
        sys_->apu.tone(1, drone, 0.016f);
        int sec = int(std::ceil(kWatch - t_));
        if (sec != tickSec_ && sec <= 10 && sec >= 0) blip(880.0f, 0.04f, 0.04f);
        tickSec_ = sec;
    } else if (mode_ == Mode::Title) {
        sys_->apu.tone(1, 49.0f, 0.012f);
    } else {
        sys_->apu.tone(1, 0, 0);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    paint();
    sys.vdp.setFogColor(gs::rgb4(8, 10, 6));
    sys.apu.setMaster(0.7f);
    sys.apu.setEcho(0.12f, 0.18f, 0.1f);
    t_ = 0;
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    grounded_ = true;
    onLadder_ = false;
    onPlat_ = PLAT_ROW;
    face_ = 1;
    px_ = 86;
    py_ = kPlats[PLAT_ROW].y;
    camX_ = 8;
    camY_ = 16;
    if (bot_) begin();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (mode_ != Mode::Pause) t_ += DT;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        float span = kWorld - gs::SCREEN_W;
        float pan = 0.5f - 0.5f * std::cos(std::min(t_, 16.0f) * 0.16f);
        camX_ = 8.0f + pan * (span - 8.0f);
        camY_ = 12.0f;
        px_ = 86.0f;
        py_ = kPlats[PLAT_ROW].y;
        grounded_ = true;
        onLadder_ = false;
        face_ = 1;
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C))) begin();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) sys.quit();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Play;
            blip(420.0f, 0.04f, 0.04f);
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
            blip(260.0f, 0.04f, 0.04f);
        } else {
            left = pad.down(gs::BTN_LEFT) || pad.axisX <= -0.35f;
            right = pad.down(gs::BTN_RIGHT) || pad.axisX >= 0.35f;
            up = pad.down(gs::BTN_UP) || pad.axisY >= 0.45f;
            down = pad.down(gs::BTN_DOWN) || pad.axisY <= -0.45f;
            jump = pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_TURBO);
            if (nearLadder(false) < 0 && nearLadder(true) < 0 && pad.pressed(gs::BTN_UP)) jump = true;
        }
        if (mode_ == Mode::Play) play(DT, left, right, up, down, jump);
        float maxX = kWorld - gs::SCREEN_W;
        float wantX = std::clamp(px_ - 130.0f, 0.0f, maxX);
        float wantY = std::clamp(py_ - 150.0f, 0.0f, 40.0f);
        float k = std::min(1.0f, DT * 7.0f);
        camX_ += (wantX - camX_) * k;
        camY_ += (wantY - camY_) * k;
    }

    if (shake_ > 0) shake_ = std::max(0.0f, shake_ - DT * 2.1f);
    for (Puff& p : puffs_) p.life -= DT;
    puffs_.erase(std::remove_if(puffs_.begin(), puffs_.end(), [](const Puff& p) { return p.life <= 0; }), puffs_.end());

    if (mode_ == Mode::Play && px_ > 1050.0f) sys.setLight(40, 160, 70);
    else if (mode_ == Mode::Play && px_ > 600.0f && px_ < 760.0f) sys.setLight(180, 160, 40);
    else if (mode_ == Mode::Play) sys.setLight(40, 140, 50);
    else if (mode_ == Mode::Title) sys.setLight(90, 120, 40);
    else if (mode_ == Mode::Won) sys.setLight(40, 180, 70);
    else if (mode_ == Mode::Lost) sys.setLight(180, 30, 24);

    audio();
    draw();
}

}  // namespace orchardladd

#include "game/eaves.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace eaves {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr float GRAV = 940.0f;
constexpr float JUMP_V = -348.0f;
constexpr float MAX_RUN = 108.0f;
constexpr float MAX_FALL = 420.0f;
constexpr float CLIMB = 78.0f;
constexpr float LEAD = 26.0f;
constexpr float FORGIVE = 8.0f;
constexpr int WORLD_W = 2048;
constexpr int WORLD_H = 256;
constexpr int HOW_JUMP = 0;
constexpr int HOW_CLIMB = 1;

struct Plat {
    float x, y, w;
    int link;
    int how;
    int ladder;
    bool wind;
};

struct Ladder {
    float x, y0, y1;
    bool goal;
};

// Walk the eaves left to right. Gaps are 32px. Two ladders, one of them the exit.
const Plat kPlats[] = {
    {24, 184, 216, 1, HOW_JUMP, -1, false},    // 0  right 240
    {272, 184, 216, 2, HOW_CLIMB, 0, false},   // 1  right 488
    {400, 112, 248, 3, HOW_JUMP, -1, false},   // 2  right 648
    {680, 112, 176, 4, HOW_JUMP, -1, false},   // 3  right 856
    {888, 144, 200, 5, HOW_JUMP, -1, true},    // 4  wind, right 1088
    {1120, 144, 160, 6, HOW_JUMP, -1, false},  // 5  right 1280
    {1312, 176, 160, -1, HOW_CLIMB, 1, false}  // 6  right 1472, the far ladder
};
constexpr int kPlatN = 7;

const Ladder kLads[] = {
    {440, 112, 184, false},
    {1456, 64, 176, true}
};
constexpr int kLadN = 2;

constexpr int TOWER_X = 1472;
constexpr int TOWER_Y = 32;

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

}  // namespace

const gs::Mipped& Game::hero() const {
    if (onLadder_) return (int(py_ / 5.0f) & 1) ? art_.climbA : art_.climbB;
    if (!grounded_) return art_.jump;
    if (std::abs(vx_) > 14.0f) return (int(step_ / 8.0f) & 1) ? art_.walkA : art_.walkB;
    return art_.stand;
}

void Game::blip(float freq, float vol, float hold) {
    sys_->apu.tone(0, freq, vol);
    beep_ = hold;
}

void Game::noteOff() {
    if (beep_ > 0) {
        beep_ -= DT;
        if (beep_ <= 0) sys_->apu.tone(0, 0, 0);
    }
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 64 || s.x + s.w < -64 || s.y > gs::SCREEN_H + 48 || s.y + s.h < -48) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::world(const gs::Mipped& m, float wx, float wy, float h, int pal, bool flip, bool feet) {
    spr(m, wx - camX_, wy - camY_, h, pal, flip, feet);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal, int align) {
    float width = 0;
    for (unsigned char c : s) {
        if (c == ' ') width += 8.0f * scale;
        else if (c > 32 && c < 128) width += art_.glyph[c - 32].w * scale + scale;
    }
    if (align == 0) x -= width * 0.5f;
    else if (align > 0) x -= width;
    for (unsigned char c : s) {
        if (c == ' ') {
            x += 8.0f * scale;
            continue;
        }
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        float h = g.h * scale;
        spr(g, x + g.w * scale * 0.5f, y, h, pal, false, false);
        x += g.w * scale + scale;
    }
}

int Game::platAt(float x, float y) const {
    int best = -1;
    float bestD = 6.0f;
    for (int i = 0; i < kPlatN; i++) {
        const Plat& p = kPlats[i];
        if (x < p.x - FORGIVE || x > p.x + p.w + FORGIVE) continue;
        float d = std::abs(y - p.y);
        if (d < bestD) {
            bestD = d;
            best = i;
        }
    }
    return best;
}

void Game::mount(int i) {
    onLadder_ = true;
    lad_ = i;
    vx_ = 0;
    vy_ = 0;
    grounded_ = false;
}

void Game::leave(bool top) {
    const Ladder& L = kLads[lad_];
    onLadder_ = false;
    lockout_ = 0.22f;
    if (top) {
        py_ = L.y0;
        px_ = L.x + 22.0f;
        vy_ = 0;
        vx_ = 36.0f;
        onPlat_ = platAt(px_, py_);
        grounded_ = onPlat_ >= 0;
        if (onPlat_ > checkpoint_) checkpoint_ = onPlat_;
        if (route_ >= 0 && route_ < kPlatN && kPlats[route_].link == onPlat_) route_ = onPlat_;
        coyote_ = 0.12f;
        face_ = 1;
    } else {
        py_ = L.y1;
        vy_ = 0;
        onPlat_ = platAt(px_, py_);
        grounded_ = onPlat_ >= 0;
    }
}

void Game::win() {
    if (won_) return;
    won_ = true;
    over_ = true;
    mode_ = Victory;
    vy_ = 0;
    vx_ = 0;
    fan_ = 0;
    fanT_ = 0;
    sys_->rumble(0.35f, 0.85f, 220);
    sys_->setLight(40, 200, 80);
    if (!announced_) {
        announced_ = true;
        std::printf("the far ladder holds\n");
        std::fflush(stdout);
    }
}

void Game::miss() {
    if (mode_ != Play) return;
    falls_++;
    tries_--;
    onLadder_ = false;
    vx_ = 0;
    vy_ = 0;
    sys_->apu.noiseBurst(0.45f, 640.0f, 0.28f);
    sys_->rumble(0.9f, 0.4f, 180);
    sys_->setLight(200, 30, 20);
    if (tries_ <= 0) {
        mode_ = Over;
        over_ = true;
        won_ = false;
        return;
    }
    mode_ = Drop;
    dropT_ = 0.7f;
}

void Game::respawn() {
    const Plat& p = kPlats[checkpoint_];
    route_ = checkpoint_;
    px_ = p.x + 48.0f;
    py_ = p.y;
    vx_ = 0;
    vy_ = 0;
    grounded_ = true;
    onPlat_ = checkpoint_;
    onLadder_ = false;
    face_ = 1;
    coyote_ = 0.12f;
    jumpBuf_ = 0;
    lockout_ = 0.15f;
    mode_ = Play;
}

void Game::resetRun() {
    tries_ = 5;
    falls_ = 0;
    checkpoint_ = 0;
    route_ = 0;
    onLadder_ = false;
    onPlat_ = 0;
    lad_ = 0;
    face_ = 1;
    px_ = kPlats[0].x + 48.0f;
    py_ = kPlats[0].y;
    vx_ = vy_ = 0;
    grounded_ = true;
    coyote_ = 0.12f;
    jumpBuf_ = 0;
    lockout_ = 0;
    dropT_ = 0;
    step_ = 0;
    foot_ = 0;
    won_ = false;
    over_ = false;
    announced_ = false;
    fan_ = -1;
    puffs_.clear();
    camX_ = 0;
    camY_ = 16;
    mode_ = Play;
}

void Game::paintCity() {
    gs::VDP& v = sys_->vdp;
    v.A.resize(256, 32);
    v.B.resize(64, 32);
    v.A.enabled = true;
    v.B.enabled = true;
    auto putA = [&](int tx, int ty, int tile) {
        if (tx < 0 || ty < 0 || tx >= v.A.w || ty >= v.A.h || tile <= 0) return;
        v.A.set(tx, ty, gs::entry(tile, PAL_CITY));
    };
    auto putB = [&](int tx, int ty, int tile) {
        if (tx < 0 || ty < 0 || tx >= v.B.w || ty >= v.B.h || tile <= 0) return;
        v.B.set(tx, ty, gs::entry(tile, PAL_FAR));
    };

    for (int ty = 0; ty < 8; ty++) {
        for (int tx = 0; tx < v.B.w; tx++) {
            uint32_t h = uint32_t(tx) * 2246822519u ^ uint32_t(ty) * 3266489917u;
            if ((h % 19u) == 0) putB(tx, ty, (h & 8u) ? art_.starB : art_.star);
        }
    }
    for (int tx = 0; tx < v.B.w; tx++) {
        int top = 9 + int((tx * 5 + 3) % 5);
        for (int ty = top; ty < 18; ty++) putB(tx, ty, ty == top ? art_.farRoof : art_.farWall);
        if (tx % 9 == 3) {
            putB(tx, top - 1, art_.farChim);
            putB(tx, top - 2, art_.farChim);
        }
    }

    for (int i = 0; i < kPlatN; i++) {
        const Plat& p = kPlats[i];
        int x0 = int(p.x) / 8;
        int n = int(p.w) / 8;
        int y0 = int(p.y) / 8;
        for (int k = 0; k < n; k++) {
            int tx = x0 + k;
            int slate = (k == 0) ? art_.capL : (k == n - 1) ? art_.capR : ((tx & 1) ? art_.slateB : art_.slate);
            putA(tx, y0, slate);
            putA(tx, y0 + 1, art_.gutter);
            for (int row = 2; row <= 4; row++) {
                int tile = ((tx + row) & 1) ? art_.brickB : art_.brick;
                if (row == 3 && k > 0 && k < n - 1 && (k % 4) == 2) {
                    bool lit = ((tx * 17 + i * 3) % 5) != 0;
                    tile = lit ? art_.window : art_.windowD;
                } else if (row == 4 && (k % 5) == 1) {
                    tile = art_.ivy;
                }
                putA(tx, y0 + row, tile);
            }
        }
    }

    int tx0 = TOWER_X / 8;
    int tx1 = 1680 / 8;
    int ty0 = TOWER_Y / 8;
    for (int ty = ty0; ty < 31; ty++) {
        for (int tx = tx0; tx < tx1; tx++) {
            bool window = (tx == tx0 + 3 || tx == tx0 + 8) && ((ty - ty0) % 3) == 1 && ty < 22;
            putA(tx, ty, window ? art_.window : art_.tower);
        }
    }
}

void Game::bot(bool& left, bool& right, bool& up, bool& down, bool& jump) const {
    left = right = up = down = jump = false;
    if (mode_ != Play || route_ < 0 || route_ >= kPlatN) return;
    const Plat& p = kPlats[route_];
    if (p.how == HOW_CLIMB && p.ladder >= 0) {
        const Ladder& L = kLads[p.ladder];
        if (onLadder_) {
            up = true;
            return;
        }
        if (px_ < L.x - 8.0f) right = true;
        else if (px_ > L.x + 8.0f) left = true;
        else up = true;
        if (px_ > p.x + p.w - 12.0f) {
            left = true;
            right = false;
        }
        return;
    }
    right = true;
    float edge = p.x + p.w;
    if (grounded_ && px_ >= edge - LEAD && (vx_ >= MAX_RUN * 0.9f || px_ >= edge - 8.0f)) jump = true;
}

void Game::play(float dt, bool left, bool right, bool up, bool down, bool jumpPressed) {
    if (jumpPressed) jumpBuf_ = 0.12f;
    if (jumpBuf_ > 0) jumpBuf_ -= dt;
    if (lockout_ > 0) lockout_ -= dt;

    if (onLadder_) {
        const Ladder& L = kLads[lad_];
        px_ = approach(px_, L.x, 240.0f * dt);
        vx_ = 0;
        float before = py_;
        if (up) py_ -= CLIMB * dt;
        if (down) py_ += CLIMB * dt;
        if (py_ < L.y0) py_ = L.y0;
        if (py_ > L.y1) py_ = L.y1;
        climbSnd_ -= std::abs(py_ - before);
        if (climbSnd_ <= 0 && (up || down)) {
            blip(210.0f + (int(py_) & 7) * 8.0f, 0.03f, 0.04f);
            climbSnd_ = 9.0f;
        }
        if (L.goal && py_ <= L.y0 + 0.2f) {
            py_ = L.y0;
            win();
            return;
        }
        if (!L.goal && up && py_ <= L.y0 + 0.2f) {
            leave(true);
            return;
        }
        if (down && py_ >= L.y1 - 0.2f) {
            leave(false);
            return;
        }
        if (jumpPressed) {
            onLadder_ = false;
            lockout_ = 0.12f;
            vy_ = JUMP_V * 0.72f;
            grounded_ = false;
            coyote_ = 0;
            if (right) face_ = 1;
            if (left) face_ = -1;
            vx_ = face_ * MAX_RUN * 0.4f;
            blip(680.0f, 0.05f, 0.05f);
        }
        return;
    }

    if (lockout_ <= 0 && (up || down)) {
        for (int i = 0; i < kLadN; i++) {
            const Ladder& L = kLads[i];
            if (std::abs(px_ - L.x) > 18.0f) continue;
            if (up && py_ >= L.y0 + 4.0f && py_ <= L.y1 + 2.0f) {
                mount(i);
                return;
            }
            if (down && !L.goal && std::abs(py_ - L.y0) <= 6.0f) {
                mount(i);
                py_ = L.y0 + 6.0f;
                return;
            }
        }
    }

    float target = 0;
    if (right) target += MAX_RUN;
    if (left) target -= MAX_RUN;
    if (grounded_ && onPlat_ >= 0 && onPlat_ < kPlatN && kPlats[onPlat_].wind && !right) target = std::min(target, -52.0f);
    vx_ = approach(vx_, target, (grounded_ ? 760.0f : 500.0f) * dt);
    if (left) face_ = -1;
    if (right) face_ = 1;

    if (jumpBuf_ > 0 && (grounded_ || coyote_ > 0)) {
        vy_ = JUMP_V;
        grounded_ = false;
        coyote_ = 0;
        jumpBuf_ = 0;
        blip(720.0f, 0.055f, 0.06f);
        sys_->rumble(0.15f, 0.45f, 70);
    }

    if (!grounded_) {
        vy_ += GRAV * dt;
        if (vy_ > MAX_FALL) vy_ = MAX_FALL;
    } else {
        vy_ = 0;
    }

    if (grounded_ && std::abs(vx_) > 20.0f) {
        foot_ -= std::abs(vx_) * dt;
        step_ += std::abs(vx_) * dt;
        if (foot_ <= 0) {
            blip(150.0f, 0.025f, 0.03f);
            foot_ = 16.0f;
        }
    }

    px_ += vx_ * dt;
    float prevY = py_;
    py_ += vy_ * dt;

    bool wasGround = grounded_;
    grounded_ = false;
    int landed = -1;
    if (vy_ >= 0) {
        float reach = std::max(12.0f, vy_ * dt + 2.0f);
        for (int i = 0; i < kPlatN; i++) {
            const Plat& p = kPlats[i];
            if (px_ < p.x - FORGIVE || px_ > p.x + p.w + FORGIVE) continue;
            if (prevY <= p.y + 0.8f && py_ >= p.y && py_ - p.y <= reach) {
                if (landed < 0 || p.y < kPlats[landed].y) landed = i;
            }
        }
    }
    if (landed >= 0) {
        if (!wasGround && prevY < kPlats[landed].y - 8.0f) {
            if (puffs_.size() < 6) puffs_.push_back({px_, py_, 0.28f});
            sys_->rumble(0.2f, 0.1f, 40);
        }
        py_ = kPlats[landed].y;
        vy_ = 0;
        grounded_ = true;
        onPlat_ = landed;
        if (landed > checkpoint_) checkpoint_ = landed;
        if (route_ >= 0 && route_ < kPlatN && kPlats[route_].link == landed) route_ = landed;
    }

    if (grounded_) coyote_ = 0.12f;
    else coyote_ = std::max(0.0f, coyote_ - dt);

    if (py_ > 250.0f) miss();
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();

    const uint16_t zenith = gs::rgb4(1, 1, 5);
    const uint16_t glow = gs::rgb4(8, 5, 9);
    const uint16_t nadir = gs::rgb4(1, 1, 3);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = y + camY_;
        float u = (wy - 150.0f) / 120.0f;
        v.lineBackdrop[y] = u < 0 ? lerpC(glow, zenith, std::min(1.0f, -u)) : lerpC(glow, nadir, std::min(1.0f, u));
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
    v.setFogColor(nadir);
    v.A.scroll(int(std::lround(-camX_)), int(std::lround(camY_)));
    v.B.scroll(int(std::lround(-camX_ * 0.32f)), int(std::lround(camY_ * 0.2f)));

    // First sprite sits on top.
    if (mode_ == Title) {
        text("S3 EAVES", 160, 46, 1.7f, PAL_AMBER);
        text("THE FAR LADDER", 160, 74, 1.15f, PAL_HUD);
        text("DON'T FALL", 160, 98, 1.25f, PAL_ALERT);
        if ((int(t_ * 2.0f) & 1) == 0) text("START", 160, 132, 1.15f, PAL_OK);
        hudC(24, "ARROWS MOVE   C JUMP   UP CLIMB", PAL_HUD);
    } else if (mode_ == Pause) {
        text("PAUSED", 160, 96, 1.4f, PAL_HUD);
        hudC(18, "START", PAL_DIM);
    } else if (mode_ == Drop) {
        text("DON'T FALL", 160, 88, 1.2f, PAL_ALERT);
    } else if (mode_ == Over) {
        text("THE ALLEY", 160, 78, 1.3f, PAL_ALERT);
        text("DON'T FALL", 160, 104, 1.05f, PAL_HUD);
        if ((int(t_ * 2.0f) & 1) == 0) text("START", 160, 136, 1.05f, PAL_OK);
    } else if (mode_ == Victory) {
        text("THE FAR LADDER", 160, 64, 1.05f, PAL_AMBER);
        text("IT HOLDS", 160, 92, 1.3f, PAL_OK);
    }

    if (mode_ == Play || mode_ == Pause || mode_ == Victory) {
        char buf[24];
        hud(1, 0, "EAVES", PAL_AMBER);
        std::snprintf(buf, sizeof buf, "SPAN %d", route_ + 1);
        hud(8, 0, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "TRIES %d", std::max(0, tries_));
        hud(31, 0, buf, tries_ <= 2 ? PAL_ALERT : PAL_HUD);
        if (mode_ == Play && route_ == 0) hudC(26, "C JUMP   UP CLIMB", PAL_DIM);
        if (mode_ == Play && grounded_ && onPlat_ >= 0 && kPlats[onPlat_].wind) hudC(3, "WIND", PAL_AMBER);
    }

    if (grounded_ && !onLadder_ && mode_ != Title) world(art_.shadow, px_, py_ - 1.0f, 7, PAL_FX, false, true);
    for (const Puff& p : puffs_) {
        float h = 8.0f + (0.28f - p.life) * 28.0f;
        world(art_.dust, p.x, p.y, h, PAL_FX, false, true);
    }
    world(hero(), px_, py_, 38, PAL_PLAYER, face_ < 0, true);

    const Ladder& goal = kLads[1];
    float pulse = 1.0f + 0.05f * std::sin(t_ * 5.0f);
    world(art_.hatch, goal.x, goal.y0 - 14.0f, 20.0f * pulse, PAL_BRASS, false, false);
    world(art_.lantern, goal.x + 18.0f, goal.y0 - 6.0f, 16.0f * pulse, PAL_BRASS, false, true);
    world(art_.chimney, 1608, float(TOWER_Y), 28, PAL_CITY, false, true);

    for (int i = 0; i < kLadN; i++) {
        const Ladder& L = kLads[i];
        int pal = L.goal ? PAL_BRASS : PAL_IRON;
        for (float y = L.y0; y < L.y1 - 1.0f; y += 32.0f) world(art_.ladder, L.x, y + 16.0f, 32, pal, false, false);
    }
    for (int i = 0; i < kPlatN; i++) {
        const Plat& p = kPlats[i];
        if (p.link < 0 || p.how != HOW_JUMP) continue;
        const Plat& n = kPlats[p.link];
        float x = (p.x + p.w + n.x) * 0.5f;
        float y = std::min(p.y, n.y) - 78.0f;
        world(art_.wire, x, y, 16, PAL_IRON, false, false);
        if (!p.wind) world(art_.lantern, p.x + 18.0f, p.y, 14, PAL_BRASS, false, true);
    }
    world(art_.lantern, kPlats[4].x + 28.0f, kPlats[4].y, 14, PAL_BRASS, false, true);

    const Plat& wind = kPlats[4];
    for (int i = 0; i < 5; i++) {
        float span = wind.w;
        float x = std::fmod(span + i * 46.0f - t_ * 92.0f, span);
        world(art_.streak, wind.x + x, wind.y - 18.0f - (i % 3) * 7.0f, 8, PAL_FX, false, false);
    }

    for (int i = 0; i < 3; i++) {
        float x = std::fmod(160.0f + i * 560.0f + t_ * (20.0f + i * 7.0f), float(WORLD_W));
        float y = 52.0f + float((i * 19) % 30);
        world(art_.bird[int(t_ * 8 + i) & 1], x, y, 14, PAL_BIRD, false, false);
    }
    for (int i = 0; i < 3; i++) {
        float x = std::fmod(80.0f + i * 700.0f - t_ * (6.0f + i), float(WORLD_W));
        world(art_.cloud, x, 78.0f + i * 8.0f, 18, PAL_FAR, false, false);
    }
    world(art_.moon, 150, 62, 32, PAL_MOON, false, false);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    paintCity();
    sys.vdp.setFogColor(gs::rgb4(1, 1, 3));
    sys.apu.setMaster(0.8f);
    sys.apu.setEcho(0.14f, 0.22f, 0.14f);
    resetRun();
    if (!bot_) mode_ = Title;
    t_ = 0;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const float dt = DT;
    t_ += dt;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Title) {
        float u = 0.5f - 0.5f * std::cos(t_ * 0.42f);
        camX_ = u * (kLads[1].x - 200.0f);
        camY_ = 12.0f;
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            resetRun();
            blip(660.0f, 0.05f, 0.05f);
            camX_ = std::clamp(px_ - 150.0f, 0.0f, float(WORLD_W - gs::SCREEN_W));
            camY_ = std::clamp(py_ - 150.0f, 0.0f, float(WORLD_H - gs::SCREEN_H));
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            sys.quit();
        }
    } else if (mode_ == Pause) {
        if (pad.pressed(gs::BTN_START)) {
            mode_ = Play;
            blip(520.0f, 0.04f, 0.04f);
        } else if (pad.pressed(gs::BTN_MODE)) {
            mode_ = Title;
            t_ = 0;
        }
    } else if (mode_ == Over) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            resetRun();
            blip(660.0f, 0.05f, 0.05f);
        }
    } else if (mode_ == Victory) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            resetRun();
            blip(660.0f, 0.05f, 0.05f);
        }
    } else if (mode_ == Drop) {
        dropT_ -= dt;
        if (dropT_ <= 0) respawn();
    } else if (mode_ == Play) {
        bool left = false, right = false, up = false, down = false, jump = false;
        if (bot_) {
            bot(left, right, up, down, jump);
        } else {
            left = pad.down(gs::BTN_LEFT) || pad.axisX <= -0.35f;
            right = pad.down(gs::BTN_RIGHT) || pad.axisX >= 0.35f;
            up = pad.down(gs::BTN_UP);
            down = pad.down(gs::BTN_DOWN);
            jump = pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_TURBO);
            bool near = false;
            for (int i = 0; i < kLadN; i++) {
                const Ladder& L = kLads[i];
                if (std::abs(px_ - L.x) <= 20.0f && py_ >= L.y0 && py_ <= L.y1 + 2.0f) near = true;
            }
            if (!near && pad.pressed(gs::BTN_UP)) jump = true;
            if (pad.pressed(gs::BTN_START)) {
                mode_ = Pause;
                blip(300.0f, 0.04f, 0.04f);
            }
        }
        if (mode_ == Play) play(dt, left, right, up, down, jump);

        float maxX = float(WORLD_W - gs::SCREEN_W);
        float maxY = float(WORLD_H - gs::SCREEN_H);
        float wantX = std::clamp(px_ + face_ * 32.0f - 156.0f, 0.0f, maxX);
        float wantY = std::clamp(py_ - 150.0f, 0.0f, maxY);
        float k = std::min(1.0f, dt * 6.0f);
        camX_ += (wantX - camX_) * k;
        camY_ += (wantY - camY_) * k;
    }

    for (Puff& p : puffs_) p.life -= dt;
    puffs_.erase(std::remove_if(puffs_.begin(), puffs_.end(), [](const Puff& p) { return p.life <= 0; }), puffs_.end());

    float drone = (mode_ == Title) ? 96.0f : (mode_ == Victory) ? 130.0f : 78.0f;
    float dvol = (mode_ == Play || mode_ == Title) ? 0.02f : 0.012f;
    sys.apu.tone(1, drone, dvol);
    if (mode_ == Play && grounded_ && onPlat_ >= 0 && kPlats[onPlat_].wind) sys.apu.noise(0.04f, 1800.0f, false);
    else if (mode_ != Drop) sys.apu.noise(0, 0, false);

    if (fan_ >= 0) {
        static const float notes[] = {523.0f, 659.0f, 784.0f, 1046.0f};
        fanT_ += dt;
        if (fanT_ > 0.13f) {
            if (fan_ < 4) sys.apu.tone(2, notes[fan_], 0.07f);
            else sys.apu.tone(2, 0, 0);
            fan_++;
            fanT_ = 0;
            if (fan_ > 7) fan_ = -1;
        }
    }

    if (mode_ == Play) sys.setLight(190, 120, 40);
    else if (mode_ == Title) sys.setLight(80, 90, 160);

    noteOff();
    draw();
}

}  // namespace eaves

#include "game/ladd.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace lotladd {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr float GRAV = 900.0f;
constexpr float JUMP_V = -372.0f;
constexpr float MAX_RUN = 132.0f;
constexpr float MAX_FALL = 460.0f;
constexpr float CLIMB = 84.0f;
constexpr float FORGIVE = 12.0f;
constexpr float kWatch = 42.0f;
constexpr float kWorld = 1680.0f;
constexpr float kLossY = 236.0f;
constexpr float kFarX = 1328.0f;
constexpr float kPitX = 690.0f;

enum { PLAT_BOOTH = 0, PLAT_COUPE = 1, PLAT_WAGON = 2, PLAT_OFFICE = 3, PLAT_N = 4 };
enum { LAD_PIT = 0, LAD_FAR = 1, LAD_N = 2 };

struct Plat {
    float x, y, w;
};

struct Lad {
    float x, y0, y1;
    bool goal;
};

// Gaps are 40px. A running jump taken just before the lip still lands.
const Plat kPlats[PLAT_N] = {
    {20, 180, 200},     // 20..220 booth
    {260, 168, 230},    // 260..490 coupes, the loose tire
    {530, 164, 250},    // 530..780 wagons
    {820, 152, 560}     // 820..1380 sales office
};

const Lad kLads[LAD_N] = {
    {kPitX, 164, 248, false},
    {kFarX, 48, 152, true}
};

const float kCars[][2] = {
    {70, 180}, {150, 180}, {310, 168}, {410, 168}, {580, 164}, {690, 164}, {900, 152}, {1020, 152}, {1160, 152}};

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

bool Game::tireOn() const {
    float ph = std::fmod(t_, 3.6f);
    if (ph < 0) ph += 3.6f;
    return ph < 2.15f;
}

float Game::tireX() const {
    float ph = std::fmod(t_, 3.6f);
    if (ph < 0) ph += 3.6f;
    return 470.0f - ph * 78.0f;
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_) return 4;
    if (px_ >= kPlats[PLAT_OFFICE].x) return 3;
    if (px_ >= kPlats[PLAT_COUPE].x) return 2;
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
        if (!down && py_ >= L.y0 - 2.0f && py_ <= L.y1 + 6.0f) return i;
        if (down && !L.goal && std::abs(py_ - L.y0) <= 10.0f) return i;
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
    if (py_ < L.y0) py_ = L.y0;
    if (py_ > L.y1) py_ = L.y1;
    blip(390.0f, 0.04f, 0.04f);
}

void Game::leave(bool top) {
    const Lad& L = kLads[lad_];
    onLadder_ = false;
    lock_ = 0.16f;
    vx_ = 0;
    vy_ = 0;
    if (top) {
        py_ = L.y0;
        px_ = L.x + 16.0f;
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
    shake_ = 0.25f;
    sys_->rumble(0.25f, 0.7f, 200);
    sys_->setLight(40, 170, 70);
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
    shake_ = 0.8f;
    sys_->apu.noiseBurst(0.4f, 380.0f, 0.24f);
    sys_->rumble(0.8f, 0.25f, 160);
    sys_->setLight(180, 40, 20);
}

void Game::begin() {
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
    reason_ = "";
    onLadder_ = false;
    lad_ = -1;
    grounded_ = true;
    onPlat_ = PLAT_BOOTH;
    face_ = 1;
    px_ = 80.0f;
    py_ = kPlats[PLAT_BOOTH].y;
    vx_ = vy_ = 0;
    coyote_ = 0.12f;
    jumpBuf_ = 0;
    lock_ = 0.05f;
    step_ = 0;
    foot_ = 8;
    climbSnd_ = 0;
    shake_ = 0;
    fan_ = -1;
    tickSec_ = -1;
    t_ = 0;
    puffs_.clear();
    camX_ = 0;
    camY_ = 16;
    blip(520.0f, 0.05f, 0.05f);
}

void Game::paint() {
    gs::VDP& v = sys_->vdp;
    v.A.resize(256, 64);
    v.B.resize(128, 32);
    v.A.enabled = true;
    v.B.enabled = true;
    v.A.clear();
    v.B.clear();
    auto put = [&](gs::Plane& p, int tx, int ty, int tile, int pal) {
        if (tx < 0 || ty < 0 || tx >= p.w || ty >= p.h || tile <= 0) return;
        p.set(tx, ty, gs::entry(tile, pal));
    };
    for (int ty = 0; ty < 10; ty++) {
        for (int tx = 0; tx < v.B.w; tx++) {
            uint32_t h = uint32_t(tx) * 2246822519u ^ uint32_t(ty) * 3266489917u;
            if ((h % 23u) == 0) put(v.B, tx, ty, art_.star, PAL_NIGHT);
        }
    }
    for (int tx = 0; tx < 200; tx++) {
        for (int ty = 24; ty < 32; ty++) put(v.A, tx, ty, (tx % 7 == 0) ? art_.stall : art_.asphalt, PAL_LOT);
    }
    for (int ty = 8; ty < 20; ty++) {
        for (int tx = 150; tx < 176; tx++) put(v.A, tx, ty, art_.brick, PAL_SIGN);
    }
    for (int tx = 154; tx < 172; tx += 4) put(v.A, tx, 12, art_.win, PAL_NIGHT);
}

void Game::bot(bool& left, bool& right, bool& up, bool& down, bool& jump) const {
    left = right = up = down = jump = false;
    if (onLadder_) {
        if (lad_ == LAD_FAR) up = true;
        else down = py_ < kLads[LAD_PIT].y1 - 4.0f;
        return;
    }
    if (nearLadder(false) == LAD_FAR) {
        up = true;
        return;
    }
    right = true;
    if (onPlat_ == PLAT_COUPE && tireOn()) {
        float tx = tireX();
        if (tx > px_ - 4.0f && tx - px_ < 48.0f) jump = true;
    }
    if (onPlat_ >= 0 && onPlat_ < PLAT_N - 1) {
        const Plat& p = kPlats[onPlat_];
        if (px_ > p.x + p.w - 28.0f) jump = true;
    }
}

void Game::play(float dt, bool left, bool right, bool up, bool down, bool jumpPressed) {
    if (mode_ != Mode::Play) return;
    lock_ = std::max(0.0f, lock_ - dt);
    if (jumpPressed) jumpBuf_ = 0.12f;
    else jumpBuf_ = std::max(0.0f, jumpBuf_ - dt);

    if (onLadder_) {
        const Lad& L = kLads[lad_];
        float dir = (up ? -1.0f : 0.0f) + (down ? 1.0f : 0.0f);
        if (left) face_ = -1;
        if (right) face_ = 1;
        py_ += dir * CLIMB * dt;
        climbSnd_ -= dt;
        if (dir != 0 && climbSnd_ <= 0) {
            blip(210.0f, 0.02f, 0.03f);
            climbSnd_ = 0.16f;
        }
        if (L.goal && py_ <= L.y0 + 1.0f) {
            py_ = L.y0;
            win();
            return;
        }
        if (!L.goal && py_ >= L.y1 - 1.0f) {
            lose("THE PIT LADDER");
            return;
        }
        if (py_ <= L.y0 && (up || jumpPressed)) {
            leave(true);
        } else if (py_ >= L.y1 && down && !L.goal) {
            lose("THE PIT LADDER");
            return;
        } else if (!L.goal && py_ >= L.y0 && (jumpPressed || left || right) && std::abs(py_ - L.y0) < 8.0f) {
            leave(true);
        }
        if (t_ > kWatch) lose("THE WATCH IS OVER");
        return;
    }

    int grab = -1;
    if (up) grab = nearLadder(false);
    if (grab < 0 && down) grab = nearLadder(true);
    if (grab == LAD_FAR || (grab == LAD_PIT && down)) {
        mount(grab);
        return;
    }

    float accel = grounded_ ? 980.0f : 640.0f;
    if (left && !right) {
        face_ = -1;
        vx_ = approach(vx_, -MAX_RUN, accel * dt);
    } else if (right && !left) {
        face_ = 1;
        vx_ = approach(vx_, MAX_RUN, accel * dt);
    } else {
        vx_ = approach(vx_, 0, (grounded_ ? 1100.0f : 280.0f) * dt);
    }

    if (jumpBuf_ > 0 && (grounded_ || coyote_ > 0)) {
        vy_ = JUMP_V;
        grounded_ = false;
        coyote_ = 0;
        jumpBuf_ = 0;
        blip(640.0f, 0.035f, 0.04f);
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
            blip(90.0f, 0.015f, 0.02f);
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
        if (!wasGround && prevY < kPlats[landed].y - 6.0f && puffs_.size() < 12)
            puffs_.push_back({px_, py_, 0.22f});
        py_ = kPlats[landed].y;
        vy_ = 0;
        grounded_ = true;
        onPlat_ = landed;
    }
    if (grounded_) coyote_ = 0.10f;
    else coyote_ = std::max(0.0f, coyote_ - dt);

    if (onPlat_ == PLAT_COUPE && grounded_ && tireOn() && std::abs(px_ - tireX()) < 14.0f) {
        lose("THE LOOSE TIRE");
        return;
    }
    if (py_ > kLossY || px_ < 2.0f || px_ > kWorld - 8.0f) {
        lose("OFF THE LOT");
        return;
    }
    if (t_ > kWatch) lose("THE WATCH IS OVER");
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
    if (shake_ > 0) view += std::sin(t_ * 70.0f) * shake_ * 3.0f;
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
    float pulse = 0.55f + 0.45f * std::sin(t_ * 6.0f);
    v.setColor(PAL_SODIUM * 16 + 6, gs::rgb4(15, 12 + int(3 * pulse), 6));

    const uint16_t zenith = gs::rgb4(1, 1, 3);
    const uint16_t mid = gs::rgb4(2, 2, 5);
    const uint16_t sodium = gs::rgb4(9, 5, 1);
    const uint16_t lot = gs::rgb4(2, 2, 2);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t c;
        if (y < 70) c = lerpC(zenith, mid, y / 70.0f);
        else if (y < 130) c = lerpC(mid, sodium, (y - 70) / 60.0f);
        else c = lerpC(sodium, lot, std::min(1.0f, (y - 130) / 70.0f));
        v.lineBackdrop[y] = c;
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
    v.A.scroll(int(std::lround(-viewX())), int(std::lround(camY_)));
    v.B.scroll(int(std::lround(-viewX() * 0.25f)), int(std::lround(camY_ * 0.1f)));

    if (mode_ == Mode::Title) {
        text("S3 LOT LADD", 160, 10, 1.15f, PAL_GOLD);
        text("AT THE LOT", 160, 34, 0.9f, PAL_HUD);
        text("REACH THE FAR LADDER", 160, 54, 0.85f, PAL_GOLD);
        text("MISS THAT AND THE WATCH IS OVER", 160, 74, 0.62f, PAL_ALERT);
        if ((int(t_ * 2.0f) & 1) == 0) text("START", 160, 98, 1.0f, PAL_OK);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160, 78, 1.3f, PAL_HUD);
    } else if (mode_ == Mode::Won) {
        text("THE FAR LADDER", 160, 146, 1.05f, PAL_GOLD);
        text("THE WATCH HOLDS", 160, 168, 0.9f, PAL_OK);
    } else if (mode_ == Mode::Lost) {
        text("THE WATCH IS OVER", 160, 34, 0.95f, PAL_ALERT);
        text(reason_, 160, 56, 0.85f, PAL_HUD);
    }

    for (const float* c : kCars) world(art_.car, c[0], c[1] + 6.0f, 28, PAL_CAR, false, true);
    world(art_.office, 1288, kPlats[PLAT_OFFICE].y, 64, PAL_SIGN, false, true);

    const float lamps[] = {48, 190, 340, 620, 860, 1100, 1360};
    int fi = int(t_ * 8.0f) & 1;
    for (float lx : lamps) {
        world(art_.lamp, lx, kLossY - 8.0f, fi ? 46 : 44, PAL_SODIUM, false, true);
    }

    if (mode_ != Mode::Won) {
        worldText("PIT", kPitX, 140, 0.65f, PAL_ALERT);
        worldText("FAR", kFarX, 28, 0.85f, PAL_GOLD);
    }

    for (int i = 0; i < LAD_N; i++) {
        const Lad& L = kLads[i];
        int pal = L.goal ? PAL_GOLD : PAL_IRON;
        for (float y = L.y0; y < L.y1 - 1.0f; y += 22.0f)
            world(art_.ladder, L.x, y + 11.0f, 26, pal, false, false);
    }

    float sway = std::sin(t_ * 1.6f) * 3.0f;
    world(art_.flag, 180 + sway, 150, 28, PAL_SIGN, false, false);
    world(art_.flag, 980 - sway, 122, 28, PAL_ALERT, false, false);

    if (tireOn()) world(art_.tire, tireX(), kPlats[PLAT_COUPE].y - 2.0f, 16, PAL_IRON, false, true);

    float hy = py_;
    if (mode_ == Mode::Title) hy += std::sin(t_ * 2.0f) * 1.0f;
    if (grounded_ && !onLadder_ && mode_ != Mode::Title)
        world(art_.shadow, px_, py_ + 2.0f, 6, PAL_LOT, false, true, true);
    world(heroSprite(), px_, hy, 42, PAL_HAND, face_ < 0, true);
    for (const Puff& p : puffs_) world(art_.dust, p.x, p.y, 6.0f + (0.3f - p.life) * 16.0f, PAL_FX, false, true);

    spr(art_.moon, 270 - viewX() * 0.04f, 26, 18, PAL_NIGHT, false, false);

    if (mode_ == Mode::Play || mode_ == Mode::Pause) {
        int left = int(std::ceil(kWatch - t_));
        if (left < 0) left = 0;
        char buf[24];
        std::snprintf(buf, sizeof buf, "WATCH %d", left);
        hud(1, 0, "LOT", PAL_GOLD);
        hud(30, 0, buf, left <= 8 ? PAL_ALERT : PAL_HUD);
        const char* hint = "THE FAR LADDER";
        if (px_ < 230.0f) hint = "C AT THE EDGE";
        else if (onPlat_ == PLAT_COUPE) hint = "JUMP THE TIRE";
        else if (std::abs(px_ - kPitX) < 30.0f) hint = "NOT THE PIT";
        else if (px_ > 1200.0f) hint = "UP THE FAR LADDER";
        hudC(1, hint, PAL_DIM);
        hud(1, 26, "ARROWS  C JUMP  UP CLIMB", PAL_HUD);
    } else if (mode_ == Mode::Title) {
        hudC(26, "START", PAL_HUD);
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
        static const float bad[] = {196.0f, 164.0f, 130.0f, 98.0f};
        fanT_ += DT;
        if (fanT_ > 0.14f) {
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
        sys_->apu.tone(1, px_ > 1100.0f ? 110.0f : 62.0f, 0.015f);
        int sec = int(std::ceil(kWatch - t_));
        if (sec != tickSec_ && sec <= 8 && sec >= 0) blip(880.0f, 0.04f, 0.03f);
        tickSec_ = sec;
    } else if (mode_ == Mode::Title) {
        sys_->apu.tone(1, 55.0f, 0.012f);
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
    grounded_ = true;
    onLadder_ = false;
    onPlat_ = PLAT_BOOTH;
    face_ = 1;
    px_ = 80;
    py_ = kPlats[PLAT_BOOTH].y;
    camX_ = 0;
    camY_ = 16;
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
        camY_ = 12.0f;
        px_ = 80.0f;
        py_ = kPlats[PLAT_BOOTH].y;
        grounded_ = true;
        onLadder_ = false;
        face_ = 1;
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C))) begin();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
    } else if (mode_ == Mode::Won || mode_ == Mode::Lost) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C))) begin();
    } else if (mode_ == Mode::Play) {
        bool left = false, right = false, up = false, down = false, jump = false;
        if (bot_) bot(left, right, up, down, jump);
        else if (pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else {
            left = pad.down(gs::BTN_LEFT) || pad.axisX <= -0.35f;
            right = pad.down(gs::BTN_RIGHT) || pad.axisX >= 0.35f;
            up = pad.down(gs::BTN_UP) || pad.axisY >= 0.45f;
            down = pad.down(gs::BTN_DOWN) || pad.axisY <= -0.45f;
            jump = pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_TURBO);
        }
        if (mode_ == Mode::Play) play(DT, left, right, up, down, jump);
        float maxX = kWorld - gs::SCREEN_W;
        float wantX = std::clamp(px_ - 140.0f, 0.0f, maxX);
        float wantY = std::clamp(py_ - 140.0f, 0.0f, 36.0f);
        float k = std::min(1.0f, DT * 7.0f);
        camX_ += (wantX - camX_) * k;
        camY_ += (wantY - camY_) * k;
    }

    if (shake_ > 0) shake_ = std::max(0.0f, shake_ - DT * 2.0f);
    for (Puff& p : puffs_) p.life -= DT;
    puffs_.erase(std::remove_if(puffs_.begin(), puffs_.end(), [](const Puff& p) { return p.life <= 0; }), puffs_.end());

    if (mode_ == Mode::Play && px_ > 1200.0f) sys.setLight(160, 120, 40);
    else if (mode_ == Mode::Play) sys.setLight(140, 80, 20);
    else if (mode_ == Mode::Title) sys.setLight(90, 50, 20);

    audio();
    draw();
}

}  // namespace lotladd

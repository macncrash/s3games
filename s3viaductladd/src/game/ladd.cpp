#include "game/ladd.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace viaductladd {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr float GRAV = 960.0f;
constexpr float JUMP_V = -348.0f;
constexpr float MAX_RUN = 122.0f;
constexpr float CLIMB = 68.0f;
constexpr float kWorld = 1720.0f;
constexpr float kLossY = 236.0f;

enum { PLAT_ABUT = 0, PLAT_STEP = 1, PLAT_CROWN = 2, PLAT_SPAN = 3, PLAT_FAR = 4, PLAT_N = 5 };
enum { LAD_CROWN = 0, LAD_FAR = 1, LAD_N = 2 };

struct Plat {
    float x, y, w;
};

struct Lad {
    float x, y0, y1;
    bool goal;
};

// The crown sits too high to jump. The far ladder is the only way off the last pier.
const Plat kPlats[PLAT_N] = {
    {16, 190, 270},     // 16..286
    {350, 190, 200},    // 350..550   gap 64
    {490, 108, 320},    // 490..810   reached by the crown ladder
    {874, 146, 210},    // 874..1084  gap 64
    {1148, 174, 480}    // 1148..1628 gap 64
};

const Lad kLads[LAD_N] = {
    {520, 108, 190, false},
    {1488, 58, 174, true}
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

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_) return 4;
    if (px_ >= kPlats[PLAT_FAR].x + 80.0f) return 3;
    if (py_ <= kPlats[PLAT_CROWN].y + 6.0f && px_ >= kPlats[PLAT_CROWN].x) return 2;
    return 1;
}

const gs::Mipped& Game::heroSprite() const {
    if (onLadder_ || mode_ == Mode::Won) return (int(py_ / 7.0f) & 1) ? art_.climbA : art_.climbB;
    if (mode_ == Mode::Play && !grounded_) return art_.jump;
    if (mode_ == Mode::Play && std::abs(vx_) > 16.0f) return (int(step_ / 8.0f) & 1) ? art_.walkA : art_.walkB;
    return art_.stand;
}

int Game::platUnder(float x, float y) const {
    int best = -1;
    float bestDy = 14.0f;
    for (int i = 0; i < PLAT_N; i++) {
        const Plat& p = kPlats[i];
        if (x < p.x - 8.0f || x > p.x + p.w + 6.0f) continue;
        float dy = std::abs(y - p.y);
        if (dy < bestDy) {
            bestDy = dy;
            best = i;
        }
    }
    return best;
}

int Game::nearLadder() const {
    if (lock_ > 0 || onLadder_) return -1;
    for (int i = 0; i < LAD_N; i++) {
        const Lad& L = kLads[i];
        if (std::abs(px_ - L.x) > 16.0f) continue;
        if (std::abs(py_ - L.y1) <= 10.0f) return i;
    }
    return -1;
}

void Game::blip(float freq, float vol, float hold) {
    sys_->apu.tone(0, freq, vol);
    beep_ = hold;
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
    shake_ = 0.2f;
    sys_->rumble(0.2f, 0.65f, 180);
    sys_->setLight(180, 120, 40);
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
    shake_ = 0.65f;
    sys_->apu.noiseBurst(0.35f, 320.0f, 0.2f);
    sys_->rumble(0.7f, 0.2f, 140);
    sys_->setLight(90, 20, 16);
}

void Game::begin() {
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
    reason_ = "";
    onLadder_ = false;
    lad_ = -1;
    grounded_ = true;
    onPlat_ = PLAT_ABUT;
    face_ = 1;
    px_ = 72.0f;
    py_ = kPlats[PLAT_ABUT].y;
    vx_ = vy_ = 0;
    coyote_ = 0.12f;
    jumpBuf_ = 0;
    lock_ = 0.12f;
    step_ = 0;
    foot_ = 8;
    climbSnd_ = 0;
    shake_ = 0;
    fan_ = -1;
    t_ = 0;
    camX_ = 0;
    blip(392.0f, 0.05f, 0.05f);
}

void Game::paint() {
    gs::VDP& v = sys_->vdp;
    v.A.resize(256, 32);
    v.B.resize(128, 32);
    v.A.clear();
    v.B.clear();
    v.A.enabled = true;
    v.B.enabled = true;
    auto put = [&](gs::Plane& p, int tx, int ty, int tile, int pal) {
        if (tx < 0 || ty < 0 || tx >= p.w || ty >= p.h || tile <= 0) return;
        p.set(tx, ty, gs::entry(tile, pal));
    };

    for (int ty = 0; ty < 12; ty++) {
        for (int tx = 0; tx < v.B.w; tx++) {
            uint32_t h = uint32_t(tx) * 1664525u ^ uint32_t(ty) * 1013904223u;
            int tile = (h % 23u) == 0 ? art_.cloud : 0;
            if (tile) put(v.B, tx, ty, tile, PAL_DUSK);
        }
    }
    for (int ty = 24; ty < 28; ty++) {
        for (int tx = 0; tx < v.B.w; tx++) {
            int tile = ((tx + ty) & 3) ? art_.gorge : art_.gorgeB;
            put(v.B, tx, ty, tile, PAL_GORGE);
        }
    }

    auto deck = [&](const Plat& p, bool arches) {
        int x0 = int(p.x) / 8;
        int x1 = int(p.x + p.w) / 8;
        int top = int(p.y) / 8;
        for (int tx = x0; tx <= x1; tx++) {
            bool joint = ((tx - x0) % 6) == 5;
            put(v.A, tx, top, joint ? art_.joint : art_.cap, PAL_BRICK);
            put(v.A, tx, top + 1, ((tx + top) & 1) ? art_.brickB : art_.brick, PAL_BRICK);
            if ((tx & 1) == 0) put(v.A, tx, top - 1, art_.rail, PAL_BRASS);
            if (!arches) continue;
            if (((tx - x0) % 5) == 2) {
                put(v.A, tx, top + 2, art_.voussoir, PAL_ARCH);
                for (int ty = top + 3; ty < 26; ty++) {
                    if ((ty - top) % 4 == 0) put(v.A, tx, ty, art_.arch, PAL_ARCH);
                    else put(v.A, tx, ty, art_.brick, PAL_BRICK);
                }
            }
        }
    };
    deck(kPlats[PLAT_ABUT], true);
    deck(kPlats[PLAT_STEP], false);
    deck(kPlats[PLAT_CROWN], true);
    deck(kPlats[PLAT_SPAN], true);
    deck(kPlats[PLAT_FAR], true);
}

void Game::bot(bool& left, bool& right, bool& up, bool& jump) const {
    left = false;
    right = false;
    up = false;
    jump = false;
    if (onLadder_) {
        up = true;
        return;
    }
    int lad = -1;
    if (lock_ <= 0) {
        for (int i = 0; i < LAD_N; i++) {
            if (std::abs(px_ - kLads[i].x) <= 14.0f && std::abs(py_ - kLads[i].y1) <= 8.0f) lad = i;
        }
    }
    if (lad >= 0) {
        up = true;
        return;
    }
    right = true;
    if (!grounded_) return;
    int id = platUnder(px_, py_);
    if (id < 0) return;
    const Plat& p = kPlats[id];
    float edge = p.x + p.w;
    if (px_ > edge - 18.0f && px_ < edge + 2.0f) {
        float ahead = px_ + 40.0f;
        bool supported = false;
        for (int i = 0; i < PLAT_N; i++) {
            const Plat& q = kPlats[i];
            if (ahead < q.x - 4.0f || ahead > q.x + q.w) continue;
            if (std::abs(q.y - p.y) < 10.0f) supported = true;
        }
        if (!supported) jump = true;
    }
}

void Game::play(float dt, bool left, bool right, bool up, bool jumpPressed) {
    if (lock_ > 0) lock_ -= dt;
    if (jumpPressed) jumpBuf_ = 0.12f;
    else jumpBuf_ = std::max(0.0f, jumpBuf_ - dt);

    if (onLadder_) {
        vx_ = 0;
        vy_ = 0;
        const Lad& L = kLads[lad_];
        px_ = L.x;
        if (up) py_ -= CLIMB * dt;
        if (py_ <= L.y0) {
            py_ = L.y0;
            if (L.goal) {
                win();
                return;
            }
            onLadder_ = false;
            lock_ = 0.2f;
            px_ = L.x + 20.0f;
            grounded_ = true;
            onPlat_ = platUnder(px_, py_);
            if (onPlat_ >= 0) py_ = kPlats[onPlat_].y;
            coyote_ = 0.12f;
            blip(480.0f, 0.04f, 0.04f);
        }
        climbSnd_ -= dt;
        if (climbSnd_ <= 0 && mode_ == Mode::Play) {
            blip(160.0f + std::fmod(py_, 20.0f) * 3.0f, 0.03f, 0.03f);
            climbSnd_ = 0.2f;
        }
        return;
    }

    int grab = (up && lock_ <= 0) ? nearLadder() : -1;
    if (grab >= 0) {
        onLadder_ = true;
        lad_ = grab;
        vx_ = 0;
        vy_ = 0;
        grounded_ = false;
        px_ = kLads[grab].x;
        blip(330.0f, 0.04f, 0.04f);
        return;
    }

    float target = 0;
    if (left) target -= MAX_RUN;
    if (right) target += MAX_RUN;
    if (left && !right) face_ = -1;
    if (right && !left) face_ = 1;
    vx_ = approach(vx_, target, (grounded_ ? 860.0f : 480.0f) * dt);

    if (grounded_) {
        coyote_ = 0.12f;
        vy_ = 0;
        int id = platUnder(px_, py_);
        if (id >= 0) {
            onPlat_ = id;
            py_ = kPlats[id].y;
        } else {
            grounded_ = false;
            coyote_ = 0.08f;
        }
    } else {
        coyote_ = std::max(0.0f, coyote_ - dt);
        vy_ = std::min(MAX_RUN * 3.5f, vy_ + GRAV * dt);
    }

    if (jumpBuf_ > 0 && (grounded_ || coyote_ > 0)) {
        vy_ = JUMP_V;
        grounded_ = false;
        coyote_ = 0;
        jumpBuf_ = 0;
        onPlat_ = -1;
        blip(280.0f, 0.045f, 0.04f);
    }

    float prevY = py_;
    px_ += vx_ * dt;
    py_ += vy_ * dt;
    if (px_ < 14.0f) {
        px_ = 14.0f;
        vx_ = 0;
    }
    if (px_ > kWorld - 28.0f) {
        px_ = kWorld - 28.0f;
        vx_ = 0;
    }

    if (vy_ >= 0) {
        for (int i = 0; i < PLAT_N; i++) {
            const Plat& p = kPlats[i];
            if (px_ < p.x - 6.0f || px_ > p.x + p.w + 4.0f) continue;
            if (prevY <= p.y + 2.0f && py_ >= p.y) {
                py_ = p.y;
                vy_ = 0;
                grounded_ = true;
                onPlat_ = i;
                coyote_ = 0.12f;
                break;
            }
        }
    }

    if (std::abs(vx_) > 20.0f && grounded_) {
        step_ += std::abs(vx_) * dt;
        foot_ -= dt;
        if (foot_ <= 0) {
            blip(84.0f, 0.02f, 0.02f);
            foot_ = 0.3f;
        }
    }

    if (py_ > kLossY) lose("THE GORGE");
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

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::lround(std::clamp(cx - s.w * 0.5f, -2000.0f, 2000.0f)));
    s.y = int16_t(std::lround(std::clamp(feet ? cy - s.h : cy - s.h * 0.5f, -2000.0f, 2000.0f)));
    if (s.x > gs::SCREEN_W + 80 || s.x + s.w < -80 || s.y > gs::SCREEN_H + 80 || s.y + s.h < -80) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

float Game::viewX() const {
    float view = camX_;
    if (shake_ > 0) view += std::sin(t_ * 64.0f) * shake_ * 3.0f;
    return view;
}

void Game::world(const gs::Mipped& m, float wx, float wy, float h, int pal, bool flip, bool feet) {
    spr(m, wx - viewX(), wy, h, pal, flip, feet);
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
        spr(g, x + g.w * scale * 0.5f, y, g.h * scale, pal, false, false);
        x += g.w * scale + scale;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    float pulse = 0.5f + 0.5f * std::sin(t_ * 3.2f);
    v.setColor(PAL_GOLD * 16 + 1, gs::rgb4(15, 11 + int(3 * pulse), 3));
    v.setColor(PAL_LAMP * 16 + 3, gs::rgb4(15, 12 + int(3 * pulse), 4));

    const uint16_t zenith = gs::rgb4(3, 2, 6);
    const uint16_t mid = gs::rgb4(8, 4, 6);
    const uint16_t haze = gs::rgb4(12, 7, 5);
    const uint16_t gorge = gs::rgb4(2, 3, 4);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t c;
        if (y < 80) c = lerpC(zenith, mid, y / 80.0f);
        else if (y < 150) c = lerpC(mid, haze, (y - 80) / 70.0f);
        else c = lerpC(haze, gorge, (y - 150) / 74.0f);
        v.lineBackdrop[y] = c;
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
    v.A.scroll(int(std::lround(-viewX())), 0);
    v.B.scroll(int(std::lround(-viewX() * 0.28f)), 0);

    if (mode_ == Mode::Title) {
        text("S3 VIADUCT LADD", 160, 18, 1.0f, PAL_GOLD);
        text("AT THE VIADUCT", 160, 42, 0.85f, PAL_HUD);
        text("THE FAR LADDER", 160, 62, 1.0f, PAL_OK);
        if ((int(t_ * 2.0f) & 1) == 0) text("START", 160, 88, 1.0f, PAL_GOLD);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160, 78, 1.2f, PAL_HUD);
    } else if (mode_ == Mode::Won) {
        text("THE FAR LADDER", 160, 36, 1.05f, PAL_GOLD);
        text("THE WATCH IS DONE", 160, 58, 0.8f, PAL_OK);
    } else if (mode_ == Mode::Lost) {
        text("THE WATCH IS OVER", 160, 36, 0.9f, PAL_ALERT);
        text(reason_, 160, 58, 0.85f, PAL_HUD);
    }

    for (int i = 0; i < LAD_N; i++) {
        const Lad& L = kLads[i];
        int pal = L.goal ? PAL_GOLD : PAL_BRASS;
        for (float y = L.y0; y < L.y1 - 1.0f; y += 24.0f) world(art_.ladder, L.x, y + 13.0f, 28, pal, false, false);
    }
    world(art_.signal, kLads[LAD_FAR].x + 22.0f, kLads[LAD_FAR].y0 - 2.0f, 34, PAL_GOLD, false, true);

    const float piers[] = {90, 220, 420, 700, 980, 1260, 1580};
    for (float x : piers) world(art_.pier, x, 214, 22, PAL_BRICK, false, true);
    const float lamps[] = {60, 240, 620, 940, 1300, 1560};
    for (float x : lamps) world(art_.lamp, x, 168, 18, PAL_LAMP, false, false);

    for (int i = 0; i < 3; i++) {
        float gx = std::fmod(30.0f + i * 480.0f + t_ * (22.0f + i * 5.0f), kWorld);
        float gy = 36.0f + std::sin(t_ * 1.5f + i * 1.7f) * 8.0f + i * 10.0f;
        world(art_.crow, gx, gy, 12, PAL_CROW, (i & 1) != 0, false);
    }

    float hy = py_;
    if (mode_ == Mode::Title) hy += std::sin(t_ * 2.2f) * 1.0f;
    world(heroSprite(), px_, hy, 38, PAL_COAT, face_ < 0, true);

    if (mode_ == Mode::Play || mode_ == Mode::Pause) {
        const char* hint = "THE FAR LADDER";
        if (px_ < 300.0f) hint = "CROSS THE ARCHES";
        else if (py_ > 160.0f && px_ < 700.0f) hint = "CLIMB TO THE CROWN";
        else if (px_ > 1200.0f) hint = "THE FAR LADDER";
        hud(1, 0, "VIADUCT", PAL_GOLD);
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
        static const float good[] = {349.0f, 440.0f, 523.0f, 698.0f};
        static const float bad[] = {220.0f, 174.0f, 146.0f, 110.0f};
        fanT_ += DT;
        if (fanT_ > 0.15f) {
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
        float drone = (px_ > 1140.0f) ? 73.0f : 49.0f;
        sys_->apu.tone(1, drone, 0.014f);
    } else if (mode_ == Mode::Title) {
        sys_->apu.tone(1, 44.0f, 0.012f);
    } else {
        sys_->apu.tone(1, 0, 0);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    paint();
    sys.vdp.setFogColor(gs::rgb4(4, 2, 4));
    sys.apu.setMaster(0.7f);
    sys.apu.setEcho(0.14f, 0.2f, 0.1f);
    t_ = 0;
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    grounded_ = true;
    onLadder_ = false;
    onPlat_ = PLAT_ABUT;
    face_ = 1;
    px_ = 72;
    py_ = kPlats[PLAT_ABUT].y;
    camX_ = 0;
    if (bot_) begin();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (mode_ != Mode::Pause) t_ += DT;
    if (shake_ > 0) shake_ = std::max(0.0f, shake_ - DT);
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        float span = kWorld - gs::SCREEN_W;
        float pan = 0.5f - 0.5f * std::cos(std::min(t_, 14.0f) * 0.16f);
        camX_ = pan * span;
        px_ = 72.0f;
        py_ = kPlats[PLAT_ABUT].y;
        grounded_ = true;
        onLadder_ = false;
        face_ = 1;
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C))) begin();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Play;
            blip(400.0f, 0.04f, 0.04f);
        }
    } else if (mode_ == Mode::Won || mode_ == Mode::Lost) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C))) begin();
    } else if (mode_ == Mode::Play) {
        bool left = false, right = false, up = false, jump = false;
        if (bot_) {
            bot(left, right, up, jump);
        } else if (pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(220.0f, 0.04f, 0.04f);
        } else {
            left = pad.down(gs::BTN_LEFT) || pad.axisX <= -0.35f;
            right = pad.down(gs::BTN_RIGHT) || pad.axisX >= 0.35f;
            up = pad.down(gs::BTN_UP) || pad.axisY >= 0.45f;
            jump = pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B);
        }
        play(DT, left, right, up, jump);
        float lead = 110.0f;
        float want = px_ - lead;
        if (want < 0) want = 0;
        if (want > kWorld - gs::SCREEN_W) want = kWorld - gs::SCREEN_W;
        camX_ = approach(camX_, want, 260.0f * DT);
    }

    draw();
    audio();
}

}  // namespace viaductladd

#include "game/ladd.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace foundryladd {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr float GRAV = 980.0f;
constexpr float MAX_RUN = 146.0f;
constexpr float MAX_FALL = 420.0f;
constexpr float CLIMB = 96.0f;
constexpr float FORGIVE = 10.0f;
constexpr float kWatch = 32.0f;
constexpr float kWorld = 1120.0f;
constexpr float kLossY = 230.0f;

enum { PLAT_HEARTH = 0, PLAT_MEZZ = 1, PLAT_CUPOLA = 2, PLAT_N = 3 };
enum { LAD_MEZZ = 0, LAD_CUPOLA = 1, LAD_FAR = 2, LAD_N = 3 };

struct Plat {
    float x, y, w;
};
struct Lad {
    float x, y0, y1;
    bool goal;
};

// Hearth, crane mezzanine, cupola walk. The far ladder leaves the cupola.
const Plat kPlats[PLAT_N] = {
    {16, 184, 390},
    {360, 140, 300},
    {610, 96, 430},
};

const Lad kLads[LAD_N] = {
    {392, 140, 184, false},
    {632, 96, 140, false},
    {990, 36, 96, true},
};

const float kFurnace[] = {90, 210, 330};
const float kStacks[] = {48, 700, 860};

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

float Game::ladleX() const { return 168.0f + 96.0f * std::sin(t_ * 0.9f); }

bool Game::pouring() const { return std::sin(t_ * 2.15f) > 0.62f; }

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_) return 4;
    if (onLadder_ && lad_ == LAD_FAR) return 3;
    if (px_ >= kPlats[PLAT_MEZZ].x && py_ <= kPlats[PLAT_HEARTH].y - 8.0f) return 2;
    return 1;
}

const gs::Mipped& Game::heroSprite() const {
    if (onLadder_ || mode_ == Mode::Won) return (int(py_ / 5.0f) & 1) ? art_.climbA : art_.climbB;
    if (mode_ == Mode::Play && !grounded_) return art_.jump;
    if (mode_ == Mode::Play && std::abs(vx_) > 18.0f) return (int(step_ / 8.0f) & 1) ? art_.walkA : art_.walkB;
    return art_.stand;
}

int Game::nearLadder() const {
    if (lock_ > 0) return -1;
    for (int i = 0; i < LAD_N; i++) {
        const Lad& L = kLads[i];
        if (std::abs(px_ - L.x) > 18.0f) continue;
        if (py_ > L.y0 + 6.0f && py_ <= L.y1 + 8.0f) return i;
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
    px_ = L.x;
    blip(340.0f, 0.04f, 0.04f);
}

void Game::leave() {
    const Lad& L = kLads[lad_];
    onLadder_ = false;
    lock_ = 0.18f;
    vx_ = 0;
    vy_ = 0;
    py_ = L.y0;
    px_ = L.x + 18.0f;
    grounded_ = false;
    onPlat_ = -1;
    for (int i = 0; i < PLAT_N; i++) {
        const Plat& p = kPlats[i];
        if (px_ < p.x - 8.0f || px_ > p.x + p.w + 8.0f) continue;
        if (std::abs(py_ - p.y) <= 10.0f) {
            py_ = p.y;
            grounded_ = true;
            onPlat_ = i;
            break;
        }
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    paint();
    mode_ = Mode::Title;
    t_ = 0;
    px_ = 72;
    py_ = kPlats[PLAT_HEARTH].y;
    camX_ = 0;
    camY_ = 8;
}

void Game::begin() {
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
    reason_ = "";
    onLadder_ = false;
    lad_ = -1;
    grounded_ = true;
    onPlat_ = PLAT_HEARTH;
    face_ = 1;
    px_ = 72.0f;
    py_ = kPlats[PLAT_HEARTH].y;
    vx_ = vy_ = 0;
    lock_ = 0.05f;
    step_ = 0;
    foot_ = 8;
    climbSnd_ = 0;
    shake_ = 0;
    fan_ = -1;
    tickSec_ = -1;
    t_ = 0;
    camX_ = 0;
    camY_ = 8;
    blip(480.0f, 0.05f, 0.05f);
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
    sys_->setLight(220, 120, 30);
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
    shake_ = 0.7f;
    sys_->apu.noiseBurst(0.35f, 280.0f, 0.22f);
    sys_->rumble(0.7f, 0.2f, 140);
    sys_->setLight(160, 40, 10);
}

void Game::paint() {
    gs::VDP& v = sys_->vdp;
    v.A.resize(256, 32);
    v.B.resize(64, 32);
    v.A.enabled = true;
    v.B.enabled = true;
    v.A.clear();
    v.B.clear();
    auto put = [&](gs::Plane& p, int tx, int ty, int tile, int pal) {
        if (tx < 0 || ty < 0 || tx >= p.w || ty >= p.h || tile <= 0) return;
        p.set(tx, ty, gs::entry(tile, pal));
    };
    for (int ty = 0; ty < 16; ty++) {
        for (int tx = 0; tx < v.B.w; tx++) {
            uint32_t h = uint32_t(tx) * 2246822519u ^ uint32_t(ty) * 3266489917u;
            if ((h % 11u) == 0) put(v.B, tx, ty, art_.soot, PAL_SMOKE);
        }
    }
    auto deck = [&](const Plat& p, int tile) {
        int x0 = int(p.x) / 8;
        int x1 = int(p.x + p.w) / 8;
        int y = int(p.y) / 8;
        for (int tx = x0; tx <= x1; tx++) {
            put(v.A, tx, y, tile, PAL_BRICK);
            if (y + 1 < v.A.h) put(v.A, tx, y + 1, art_.brick, PAL_BRICK);
        }
    };
    deck(kPlats[PLAT_HEARTH], art_.grate);
    deck(kPlats[PLAT_MEZZ], art_.brick);
    deck(kPlats[PLAT_CUPOLA], art_.brick);
    for (int tx = 18; tx < 28; tx++) put(v.A, tx, 26, art_.ember, PAL_HEAT);
}

void Game::bot(bool& left, bool& right, bool& up, bool& down) const {
    left = right = up = down = false;
    if (onLadder_) {
        up = true;
        return;
    }
    if (nearLadder() >= 0) {
        up = true;
        return;
    }
    if (py_ > 160.0f) {
        float lx = ladleX();
        float phase = std::sin(t_ * 2.15f);
        bool closing = phase > 0.2f;
        if (px_ < lx - 16.0f && px_ > lx - 78.0f && closing) return;
    }
    right = true;
}

void Game::play(float dt, bool left, bool right, bool up, bool down) {
    if (mode_ != Mode::Play) return;
    lock_ = std::max(0.0f, lock_ - dt);

    if (onLadder_) {
        const Lad& L = kLads[lad_];
        float dir = (up ? -1.0f : 0.0f) + (down ? 1.0f : 0.0f);
        if (left) face_ = -1;
        if (right) face_ = 1;
        py_ += dir * CLIMB * dt;
        climbSnd_ -= dt;
        if (dir != 0 && climbSnd_ <= 0) {
            blip(180.0f, 0.02f, 0.03f);
            climbSnd_ = 0.18f;
        }
        if (L.goal && py_ <= L.y0 + 1.0f) {
            py_ = L.y0;
            win();
            return;
        }
        if (py_ <= L.y0) leave();
        if (py_ > L.y1) py_ = L.y1;
        if (t_ > kWatch) lose("THE WATCH IS OVER");
        return;
    }

    if (up) {
        int grab = nearLadder();
        if (grab >= 0) {
            mount(grab);
            return;
        }
    }

    float accel = grounded_ ? 1100.0f : 520.0f;
    if (left && !right) {
        face_ = -1;
        vx_ = approach(vx_, -MAX_RUN, accel * dt);
    } else if (right && !left) {
        face_ = 1;
        vx_ = approach(vx_, MAX_RUN, accel * dt);
    } else {
        vx_ = approach(vx_, 0, (grounded_ ? 1400.0f : 240.0f) * dt);
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
            blip(70.0f, 0.012f, 0.02f);
            foot_ = 14.0f;
        }
    }

    px_ += vx_ * dt;
    float prevY = py_;
    py_ += vy_ * dt;

    grounded_ = false;
    int landed = -1;
    if (vy_ >= 0) {
        float reach = std::max(14.0f, vy_ * dt + 6.0f);
        for (int i = 0; i < PLAT_N; i++) {
            const Plat& p = kPlats[i];
            if (px_ < p.x - FORGIVE || px_ > p.x + p.w + FORGIVE) continue;
            if (prevY <= p.y + 1.0f && py_ >= p.y && py_ - p.y <= reach) {
                if (landed < 0 || p.y < kPlats[landed].y - 0.5f) landed = i;
            }
        }
    }
    if (landed >= 0) {
        py_ = kPlats[landed].y;
        vy_ = 0;
        grounded_ = true;
        onPlat_ = landed;
    }

    if (py_ > 160.0f && pouring() && std::abs(px_ - ladleX()) < 20.0f) {
        lose("THE LADLE");
        return;
    }
    if (py_ > kLossY || px_ < 4.0f || px_ > kWorld - 8.0f) {
        lose("INTO THE SLAG");
        return;
    }
    if (t_ > kWatch) lose("THE WATCH IS OVER");
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    const gs::Pad& pad = sys.pad;
    bool start = pad.pressed(gs::BTN_START);
    if (mode_ == Mode::Title) {
        if (bot_ || start || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) begin();
    } else if (mode_ == Mode::Won || mode_ == Mode::Lost) {
        if (!bot_ && (start || pad.pressed(gs::BTN_A))) {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
        }
    } else if (mode_ == Mode::Pause) {
        if (start) mode_ = Mode::Play;
    } else if (mode_ == Mode::Play && start && !bot_) {
        mode_ = Mode::Pause;
    }

    bool left = false, right = false, up = false, down = false;
    if (mode_ == Mode::Play) {
        if (bot_) bot(left, right, up, down);
        else {
            left = pad.down(gs::BTN_LEFT) || pad.axisX < -0.4f;
            right = pad.down(gs::BTN_RIGHT) || pad.axisX > 0.4f;
            up = pad.down(gs::BTN_UP) || pad.axisY > 0.4f;
            down = pad.down(gs::BTN_DOWN) || pad.axisY < -0.4f;
        }
        play(DT, left, right, up, down);
    }
    float want = std::clamp(px_ - 130.0f, 0.0f, kWorld - float(gs::SCREEN_W));
    camX_ = approach(camX_, want, 280.0f * DT);
    camY_ = 8;
    if (shake_ > 0) shake_ = std::max(0.0f, shake_ - DT);
    draw();
    audio();
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
    if (shake_ > 0) view += std::sin(t_ * 64.0f) * shake_ * 3.0f;
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
    float cx = x - width * 0.5f;
    for (const char* p = s; *p; p++) {
        unsigned char c = static_cast<unsigned char>(*p);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (c == ' ') {
            cx += 8.0f * scale;
            continue;
        }
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, cx + g.w * scale * 0.5f, y, g.h * scale, pal, false, false);
        cx += g.w * scale + scale;
    }
}

void Game::worldText(const char* s, float wx, float wy, float scale, int pal) {
    text(s, wx - viewX(), wy - camY_, scale, pal);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    float pulse = 0.5f + 0.5f * std::sin(t_ * 7.0f);
    v.setColor(PAL_HEAT * 16 + 4, gs::rgb4(15, 8 + int(6 * pulse), 2));

    const uint16_t roof = gs::rgb4(2, 1, 1);
    const uint16_t glow = gs::rgb4(8, 3, 1);
    const uint16_t floor = gs::rgb4(2, 1, 1);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t c;
        if (y < 90) c = lerpC(roof, glow, y / 90.0f);
        else c = lerpC(glow, floor, std::min(1.0f, (y - 90) / 100.0f));
        v.lineBackdrop[y] = c;
        v.lineFog[y] = uint8_t(y > 180 ? (y - 180) / 8 : 0);
        v.road[y].on = false;
    }
    v.A.scroll(int(std::lround(-viewX())), int(std::lround(-camY_)));
    v.B.scroll(int(std::lround(-viewX() * 0.2f)), 0);

    if (mode_ == Mode::Title) {
        text("S3 FOUNDRY LADD", 160, 18, 1.05f, PAL_GOLD);
        text("AT THE FOUNDRY", 160, 42, 0.85f, PAL_HUD);
        text("REACH THE FAR LADDER", 160, 64, 0.8f, PAL_GOLD);
        text("MISS THAT AND THE WATCH IS OVER", 160, 84, 0.58f, PAL_ALERT);
        if ((int(t_ * 2.0f) & 1) == 0) text("START", 160, 112, 1.0f, PAL_OK);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160, 80, 1.2f, PAL_HUD);
    } else if (mode_ == Mode::Won) {
        text("THE FAR LADDER", 160, 148, 1.0f, PAL_GOLD);
        text("THE WATCH HOLDS", 160, 170, 0.85f, PAL_OK);
    } else if (mode_ == Mode::Lost) {
        text("THE WATCH IS OVER", 160, 36, 0.9f, PAL_ALERT);
        text(reason_, 160, 58, 0.8f, PAL_HUD);
    }

    for (float sx : kStacks) world(art_.stack, sx, 70, 70, PAL_SMOKE, false, true);
    for (float fx : kFurnace) world(art_.furnace, fx, kPlats[PLAT_HEARTH].y, 40, PAL_HEAT, false, true);
    world(art_.ingot, 250, kPlats[PLAT_HEARTH].y, 12, PAL_IRON, false, true);
    world(art_.ingot, 470, kPlats[PLAT_MEZZ].y, 12, PAL_GOLD, false, true);
    world(art_.ingot, 800, kPlats[PLAT_CUPOLA].y, 12, PAL_IRON, false, true);

    float lx = ladleX();
    float bob = pouring() ? 10.0f : 0.0f;
    world(art_.ladle, lx, 150 + bob, pouring() ? 28 : 22, PAL_LADLE, false, false);
    if (pouring()) {
        world(art_.spark, lx, 176, 10, PAL_FX, false, true);
        world(art_.spark, lx + 6, 168, 7, PAL_HEAT, false, true);
    }

    if (mode_ != Mode::Won) worldText("FAR", kLads[LAD_FAR].x, 18, 0.8f, PAL_GOLD);

    for (int i = 0; i < LAD_N; i++) {
        const Lad& L = kLads[i];
        int pal = L.goal ? PAL_GOLD : PAL_IRON;
        for (float y = L.y0; y < L.y1 - 1.0f; y += 22.0f)
            world(art_.ladder, L.x, y + 12.0f, 26, pal, false, false);
    }

    float hy = py_;
    if (mode_ == Mode::Title) hy += std::sin(t_ * 2.0f) * 1.2f;
    if (grounded_ && !onLadder_ && mode_ != Mode::Title)
        world(art_.shadow, px_, py_ + 2.0f, 6, PAL_BRICK, false, true, true);
    world(heroSprite(), px_, hy, 40, PAL_HAND, face_ < 0, true);

    if (mode_ == Mode::Play || mode_ == Mode::Pause) {
        int leftT = int(std::ceil(kWatch - t_));
        if (leftT < 0) leftT = 0;
        char buf[24];
        std::snprintf(buf, sizeof buf, "WATCH %d", leftT);
        hud(1, 0, "FOUNDRY", PAL_GOLD);
        hud(30, 0, buf, leftT <= 8 ? PAL_ALERT : PAL_HUD);
        const char* hint = "THE FAR LADDER";
        if (py_ > 160.0f) hint = "UNDER THE LADLE";
        else if (py_ > 120.0f) hint = "UP TO THE CUPOLA";
        else if (px_ > 900.0f) hint = "CLIMB THE FAR LADDER";
        hudC(1, hint, PAL_DIM);
        hud(1, 26, "ARROWS MOVE  UP CLIMB", PAL_HUD);
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
        static const float good[] = {330.0f, 440.0f, 554.0f, 659.0f};
        static const float bad[] = {180.0f, 140.0f, 110.0f, 80.0f};
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
        sys_->apu.tone(1, pouring() ? 55.0f : 42.0f, pouring() ? 0.03f : 0.012f);
        int sec = int(std::ceil(kWatch - t_));
        if (sec != tickSec_ && sec <= 8 && sec >= 0) blip(740.0f, 0.04f, 0.03f);
        tickSec_ = sec;
    } else {
        sys_->apu.tone(1, 0, 0);
    }
}

}  // namespace foundryladd

#include "game/kart.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace buoy {
namespace {

constexpr float PI = 3.14159265f;
constexpr float DT = 1.0f / 60.0f;

// Harbor quay: a concrete ring around a basin, dock hanging off the south side.
constexpr float OX0 = 160, OY0 = 80, OX1 = 1440, OY1 = 980;
constexpr float IX0 = 460, IY0 = 300, IX1 = 1140, IY1 = 760;
constexpr float DX0 = 700, DX1 = 900, DY1 = 1160;

struct Mark {
    float x, y;
    float hx, hy;
    int pal;
};

// East, north, west — each passed to port, buoy on the left hand.
const Mark kMarks[3] = {
    {1290, 530, 0, -1, PAL_RED},
    {800, 190, -1, 0, PAL_GOLD},
    {310, 530, 0, 1, PAL_GREEN},
};

const float kBuoys[3][2] = {{1080, 680}, {1080, 380}, {520, 380}};

struct Wp {
    float x, y;
};
const Wp kWp[] = {
    {800, 900},  {1100, 870}, {1290, 640}, {1290, 400}, {1180, 220}, {800, 190},
    {500, 220},  {310, 400},  {310, 680},  {420, 870},  {700, 900},  {800, 1060},
};
constexpr int NWP = int(sizeof(kWp) / sizeof(kWp[0]));

bool inDock(float x, float y) { return x >= DX0 && x < DX1 && y >= OY1 && y < DY1; }

bool onQuay(float x, float y) {
    bool outer = x >= OX0 && x < OX1 && y >= OY0 && y < OY1;
    bool hole = x >= IX0 && x < IX1 && y >= IY0 && y < IY1;
    return (outer && !hole) || inDock(x, y);
}

float wrap(float a) {
    while (a > PI) a -= 2 * PI;
    while (a < -PI) a += 2 * PI;
    return a;
}

float len(float x, float y) { return std::sqrt(x * x + y * y); }

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Win || mode_ == Mode::Lose) return 2;
    return 1;
}

void Game::blip(float freq) {
    sys_->apu.tone(2, freq, 0.08f);
    beep_ = 0.06f;
}

void Game::fanfare() { fanStep_ = 0; fanT_ = 0; }

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

void Game::spr(const gs::Mipped& m, float sx, float sy, float h, int pal, bool flip, bool shadow) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 400L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 400L));
    s.x = int16_t(std::lround(sx - s.w * 0.5f));
    s.y = int16_t(std::lround(sy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 40 || s.x + s.w < -40 || s.y > gs::SCREEN_H + 40 || s.y + s.h < -40) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal, int align) {
    const float adv = 16.0f * scale;
    float w = float(s.size()) * adv;
    if (align == 0) x -= w * 0.5f;
    else if (align > 0) x -= w;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + i * adv + g.w * scale * 0.5f, y, g.h * scale, pal, false);
    }
}

void Game::startRace() {
    x_ = 800;
    y_ = 1070;
    hdg_ = -PI * 0.5f;
    speed_ = 0;
    time_ = 0;
    marks_ = 0;
    wp_ = 0;
    stuck_ = 0;
    leftDock_ = false;
    won_ = false;
    over_ = false;
    mode_ = Mode::Race;
    t_ = 0;
    if (!engine_) {
        engine_ = true;
        sys_->apu.tone(0, 70, 0.03f);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.B.clear();
    for (int y = 0; y < sys.vdp.B.h; y++)
        for (int x = 0; x < sys.vdp.B.w; x++) sys.vdp.B.set(x, y, gs::entry(art_.water, PAL_WATER));
    sys.vdp.setFogColor(gs::rgb4(4, 8, 12));
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int deep = 2 + (y * 4) / gs::SCREEN_H;
        sys.vdp.lineBackdrop[y] = gs::rgb4(1, 3 + deep / 3, 6 + deep / 2);
    }
    x_ = 800;
    y_ = 1070;
    hdg_ = -PI * 0.5f;
    if (bot_) startRace();
    else mode_ = Mode::Title;
}

void Game::readPad(float& steer, bool& accel, bool& brake) {
    const gs::Pad& p = sys_->pad;
    steer = 0;
    if (std::fabs(p.axisX) > 0.15f) steer = std::clamp(p.axisX, -1.0f, 1.0f);
    else {
        if (p.down(gs::BTN_LEFT)) steer -= 1;
        if (p.down(gs::BTN_RIGHT)) steer += 1;
    }
    accel = p.down(gs::BTN_C) || p.down(gs::BTN_A) || p.down(gs::BTN_UP) || p.accel > 0.2f;
    brake = p.down(gs::BTN_X) || p.down(gs::BTN_B) || p.down(gs::BTN_DOWN) || p.brake > 0.2f;
}

void Game::botDrive(float& steer, bool& accel, bool& brake) {
    if (wp_ < NWP - 1 && len(kWp[wp_].x - x_, kWp[wp_].y - y_) < 70) wp_++;
    float desired = std::atan2(kWp[wp_].y - y_, kWp[wp_].x - x_);
    float err = wrap(desired - hdg_);
    steer = std::clamp(err * 2.1f, -1.0f, 1.0f);
    float ad = std::fabs(err);
    accel = ad < 0.85f && speed_ < 210;
    brake = ad > 1.15f && speed_ > 90;
    if (stuck_ > 40) {
        accel = true;
        brake = false;
        steer = err > 0 ? 1.0f : -1.0f;
    }
}

void Game::updateRace() {
    float steer = 0;
    bool accel = false, brake = false;
    if (bot_) botDrive(steer, accel, brake);
    else readPad(steer, accel, brake);

    bool quay = onQuay(x_, y_);
    float turn = (0.9f + std::min(speed_, 180.0f) * 0.01f) * (quay ? 1.0f : 0.45f);
    hdg_ = wrap(hdg_ + steer * turn * DT);
    if (accel) speed_ += (quay ? 150.0f : 40.0f) * DT;
    if (brake) speed_ -= 240.0f * DT;
    speed_ -= speed_ * (quay ? 0.35f : 1.8f) * DT;
    if (speed_ < 0) speed_ = 0;
    if (speed_ > 230) speed_ = 230;

    float nx = x_ + std::cos(hdg_) * speed_ * DT;
    float ny = y_ + std::sin(hdg_) * speed_ * DT;

    for (int i = 0; i < 3; i++) {
        float dx = nx - kBuoys[i][0], dy = ny - kBuoys[i][1];
        float d = len(dx, dy);
        if (d < 36 && d > 0.01f) {
            nx = kBuoys[i][0] + dx / d * 36;
            ny = kBuoys[i][1] + dy / d * 36;
            speed_ *= 0.45f;
            blip(180);
        }
    }

    // Soft basin: water drags, the outer wall shoves you back onto the apron.
    if (nx < OX0 - 30 || nx > OX1 + 30 || ny < OY0 - 30 || ny > DY1 + 10) {
        nx = std::clamp(nx, OX0 - 20, OX1 + 20);
        ny = std::clamp(ny, OY0 - 20, DY1);
        speed_ *= 0.5f;
    }
    x_ = nx;
    y_ = ny;

    if (!inDock(x_, y_)) leftDock_ = true;
    if (speed_ < 12 && accel) stuck_++;
    else stuck_ = 0;

    if (marks_ < 3) {
        const Mark& m = kMarks[marks_];
        float dx = x_ - m.x, dy = y_ - m.y;
        float vx = std::cos(hdg_), vy = std::sin(hdg_);
        if (onQuay(x_, y_) && dx * dx + dy * dy < 90.0f * 90.0f && vx * m.hx + vy * m.hy > 0.45f) {
            marks_++;
            blip(660.0f + marks_ * 80.0f);
        }
    }

    time_ += DT;
    if (marks_ == 3 && leftDock_ && inDock(x_, y_)) {
        over_ = true;
        won_ = time_ < crew_;
        mode_ = won_ ? Mode::Win : Mode::Lose;
        if (won_) fanfare();
        else blip(140);
        sys_->apu.tone(0, 0, 0);
        return;
    }
    if (time_ > crew_ + 18.0f) {
        over_ = true;
        won_ = false;
        mode_ = Mode::Lose;
        sys_->apu.tone(0, 0, 0);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.clear();

    const float scale = 0.5f;
    camX_ = x_ - (gs::SCREEN_W * 0.5f) / scale;
    camY_ = y_ - (gs::SCREEN_H * 0.5f) / scale;
    int baseX = int(std::floor(camX_ / 16.0f));
    int baseY = int(std::floor(camY_ / 16.0f));
    float fracX = camX_ - baseX * 16.0f;
    float fracY = camY_ - baseY * 16.0f;
    v.A.scroll(int(std::lround(-fracX * 0.5f)), int(std::lround(fracY * 0.5f)));
    int scroll = int(t_ * 12);
    v.B.scroll(scroll, scroll / 2);

    auto worldOf = [&](int cx, int cy, float& wx, float& wy) {
        wx = (baseX + cx) * 16.0f + 8.0f;
        wy = (baseY + cy) * 16.0f + 8.0f;
    };
    for (int cy = 0; cy < 32; cy++) {
        for (int cx = 0; cx < 48; cx++) {
            float wx, wy;
            worldOf(cx, cy, wx, wy);
            if (!onQuay(wx, wy)) continue;
            int tile = art_.quay;
            int pal = PAL_QUAY;
            bool south = wy > IY1 && wy < OY1 && std::fabs(wy - (IY1 + OY1) * 0.5f) < 10 && wx > OX0 + 30 && wx < OX1 - 30;
            bool north = wy > OY0 && wy < IY0 && std::fabs(wy - (OY0 + IY0) * 0.5f) < 10 && wx > OX0 + 30 && wx < OX1 - 30;
            bool east = wx > IX1 && wx < OX1 && std::fabs(wx - (IX1 + OX1) * 0.5f) < 10 && wy > OY0 + 30 && wy < OY1 - 30;
            bool west = wx > OX0 && wx < IX0 && std::fabs(wx - (OX0 + IX0) * 0.5f) < 10 && wy > OY0 + 30 && wy < OY1 - 30;
            if (south || north || east || west) tile = art_.line;
            if (inDock(wx, wy)) tile = art_.dock;
            if (wy > OY1 - 18 && wy < OY1 + 6 && wx > DX0 && wx < DX1) tile = art_.check;
            v.A.set(cx, cy, gs::entry(tile, pal));
        }
    }

    auto toScreen = [&](float wx, float wy, float& sx, float& sy) {
        sx = (wx - camX_) * scale;
        sy = (wy - camY_) * scale;
    };

    const float posts[][2] = {{OX0 + 20, OY0 + 20}, {OX1 - 20, OY0 + 20}, {OX1 - 20, OY1 - 20},
                               {OX0 + 20, OY1 - 20}, {DX0, OY1},           {DX1, OY1},
                               {DX0, DY1 - 16},      {DX1, DY1 - 16}};
    for (auto& p : posts) {
        float sx, sy;
        toScreen(p[0], p[1], sx, sy);
        spr(art_.post, sx, sy, 22, PAL_WOOD);
    }
    for (int i = 0; i < 3; i++) {
        float sx, sy;
        toScreen(kBuoys[i][0], kBuoys[i][1], sx, sy);
        float bob = std::sin(t_ * 2.2f + i) * 2.0f;
        spr(art_.buoy, sx, sy + bob, 36, i == 0 ? PAL_RED : i == 1 ? PAL_GOLD : PAL_GREEN);
    }

    float ksx, ksy;
    toScreen(x_, y_, ksx, ksy);
    spr(art_.shadow, ksx + 4, ksy + 8, 16, PAL_KART, false, true);
    int face = int(std::lround(hdg_ / (PI / 8.0f))) % 16;
    if (face < 0) face += 16;
    spr(art_.kart[face], ksx, ksy, 28, PAL_KART);

    char you[32], crew[32], mk[32];
    std::snprintf(you, sizeof you, "YOU %5.2f", time_);
    std::snprintf(crew, sizeof crew, "CREW %4.2f", crew_);
    std::snprintf(mk, sizeof mk, "MARK %d/3", marks_);
    if (mode_ == Mode::Title) {
        text("KART BUOY", 160, 64, 1.15f, PAL_HUD, 0);
        text("ROUND THE BUOYS", 160, 96, 0.55f, PAL_GOLD, 0);
        text("BEAT THE OTHER CREW HOME", 160, 118, 0.42f, PAL_HUD, 0);
        text("SAME DOCK", 160, 138, 0.42f, PAL_GREEN, 0);
        if (int(t_ * 2) % 2 == 0) text("PRESS START", 160, 176, 0.5f, PAL_GOLD, 0);
        hud(1, 26, "ARROWS STEER  C GO  X BRAKE", PAL_HUD);
    } else if (mode_ == Mode::Race || mode_ == Mode::Pause) {
        hud(1, 1, you, time_ > crew_ ? PAL_RED : PAL_HUD);
        hud(28, 1, crew, PAL_GOLD);
        hud(1, 26, mk, marks_ == 3 ? PAL_GREEN : PAL_HUD);
        hud(24, 26, "DOCK", PAL_HUD);
        if (mode_ == Mode::Pause) text("PAUSE", 160, 100, 1.0f, PAL_GOLD, 0);
    } else if (mode_ == Mode::Win) {
        hud(1, 1, you, PAL_GREEN);
        hud(28, 1, crew, PAL_GOLD);
        text("DOCKED", 160, 78, 1.1f, PAL_GREEN, 0);
        text("CREW BEATEN", 160, 108, 0.55f, PAL_GOLD, 0);
        char line[40];
        std::snprintf(line, sizeof line, "%.2f INSIDE", crew_ - time_);
        text(line, 160, 132, 0.45f, PAL_HUD, 0);
    } else {
        hud(1, 1, you, PAL_RED);
        hud(28, 1, crew, PAL_GOLD);
        text("TOO SLOW", 160, 90, 1.0f, PAL_RED, 0);
        text("THE CREW HOLDS THE DOCK", 160, 120, 0.4f, PAL_HUD, 0);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    if (beep_ > 0) {
        beep_ -= DT;
        if (beep_ <= 0) sys.apu.tone(2, 0, 0);
    }
    if (fanStep_ >= 0) {
        static const float notes[] = {523, 659, 784, 1046};
        fanT_ -= DT;
        if (fanT_ <= 0 && fanStep_ < 4) {
            sys.apu.tone(2, notes[fanStep_], 0.07f);
            beep_ = 0.12f;
            fanT_ = 0.12f;
            fanStep_++;
        }
    }

    const gs::Pad& p = sys.pad;
    if (mode_ == Mode::Title) {
        if (bot_ || p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C)) startRace();
        else if (p.pressed(gs::BTN_MODE) && sys.hasHome()) sys.eject();
    } else if (mode_ == Mode::Race) {
        if (!bot_ && p.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else {
            updateRace();
            if (mode_ == Mode::Race) {
                float hz = 55.0f + speed_ * 0.9f;
                sys.apu.tone(0, hz, speed_ > 8 ? 0.04f : 0.012f);
                sys.apu.tone(1, hz * 1.5f, speed_ > 8 ? 0.02f : 0.0f);
            }
        }
    } else if (mode_ == Mode::Pause) {
        if (p.pressed(gs::BTN_START)) mode_ = Mode::Race;
        else if (p.pressed(gs::BTN_MODE)) mode_ = Mode::Title;
    } else if (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A)) {
        if (!bot_) mode_ = Mode::Title;
    }

    // Keep the kart idling on the title so the dock reads as the start.
    if (mode_ == Mode::Title) {
        x_ = 800;
        y_ = 1070;
        hdg_ = -PI * 0.5f + std::sin(t_ * 0.7f) * 0.04f;
        speed_ = 0;
    }
    draw();
}

}  // namespace buoy

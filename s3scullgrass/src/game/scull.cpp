#include "game/scull.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace scullgrass {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;
constexpr float kCrew = 34.f;
constexpr float kGrassX = 26.f;
constexpr float kGrassY0 = 80.f;
constexpr float kGrassY1 = 124.f;
constexpr float kHoldNeed = 0.55f;
constexpr float kStop = 0.22f;
constexpr float kHullX = 2.05f;
constexpr float kHullY = 8.4f;

struct V2 {
    float x, y;
};

constexpr V2 kMat[] = {{-21.f, 86.f}, {-7.f, 86.f}, {7.f, 86.f},  {21.f, 86.f},  {-21.f, 98.f}, {-7.f, 98.f},
                       {7.f, 98.f},   {21.f, 98.f},  {-21.f, 110.f}, {-7.f, 110.f}, {7.f, 110.f}, {21.f, 110.f},
                       {-21.f, 120.f}, {-7.f, 120.f}, {7.f, 120.f}, {21.f, 120.f}};

constexpr V2 kTuft[] = {{-16.f, 90.f}, {-4.f, 94.f}, {8.f, 91.f},  {18.f, 97.f}, {-12.f, 104.f},
                        {2.f, 108.f},  {14.f, 112.f}, {-18.f, 116.f}, {6.f, 118.f}, {-8.f, 86.f}};

constexpr V2 kReed[] = {{-34.f, 20.f}, {-38.f, 48.f}, {-36.f, 74.f}, {34.f, 22.f}, {38.f, 50.f}, {36.f, 76.f}};

constexpr V2 kTree[] = {{-32.f, 100.f}, {-28.f, 118.f}, {30.f, 96.f}, {34.f, 116.f}, {0.f, 132.f}};

float wrapPi(float a) {
    while (a > kPi) a -= kTau;
    while (a < -kPi) a += kTau;
    return a;
}

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

bool inGrass(float x, float y) { return std::fabs(x) <= kGrassX && y >= kGrassY0 && y <= kGrassY1; }

}  // namespace

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.08f);
    tone_ = 0.07f;
}

void Game::finish(bool win, const char* why) {
    won_ = win;
    over_ = true;
    mode_ = win ? Mode::Win : Mode::Fail;
    std::snprintf(why_, sizeof(why_), "%s", why);
    fanStep_ = 0;
    if (!win) blip(90.f);
}

void Game::begin() {
    x_ = 0.f;
    y_ = 8.f;
    heading_ = 0.f;
    speed_ = 0.f;
    stroke_ = 0.f;
    you_ = 0.f;
    hold_ = 0.f;
    won_ = false;
    over_ = false;
    fanStep_ = -1;
    foamN_ = 0;
    why_[0] = 0;
    camX_ = x_;
    camY_ = y_ + 10.f;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    crew_ = kCrew;
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = true;
    sys.vdp.B.resize(64, 32);
    for (int y = 0; y < sys.vdp.B.h; y++)
        for (int x = 0; x < sys.vdp.B.w; x++) sys.vdp.B.set(x, y, gs::entry(art_.waterTile, PAL_WATER));
    sys.vdp.setFogColor(gs::rgb4(2, 6, 4));
    sys.apu.setMaster(0.7f);
    sys.apu.setEcho(0.08f, 0.16f, 0.06f);
    begin();
    if (bot_) {
        mode_ = Mode::Row;
        zoom_ = 2.4f;
    } else {
        mode_ = Mode::Title;
        zoom_ = 1.65f;
        camX_ = 0.f;
        camY_ = 70.f;
    }
}

void Game::controls(float& row, float& steer) {
    const gs::Pad& p = sys_->pad;
    steer = 0.f;
    row = 0.f;
    if (p.down(gs::BTN_LEFT) || p.axisX < -0.3f) steer -= 1.f;
    if (p.down(gs::BTN_RIGHT) || p.axisX > 0.3f) steer += 1.f;
    if (p.down(gs::BTN_A) || p.down(gs::BTN_C) || p.down(gs::BTN_UP) || p.accel > 0.2f) row += 1.f;
    if (p.down(gs::BTN_B) || p.down(gs::BTN_DOWN) || p.brake > 0.2f) row -= 1.f;
}

void Game::pilot(float& row, float& steer) {
    const float tx = 0.f;
    const float ty = 102.f;
    const float dx = tx - x_;
    const float dy = ty - y_;
    const float dist = std::hypot(dx, dy);
    float err = wrapPi(std::atan2(dx, dy) - heading_);
    if (hullOnGrass()) {
        steer = 0.f;
        if (speed_ > kStop) row = -0.85f;
        else if (speed_ < -0.12f) row = 0.45f;
        else row = 0.f;
        return;
    }
    float desired = 11.5f;
    if (std::fabs(err) > 0.65f) desired = 3.2f;
    else if (bowOnGrass() || y_ > 68.f) desired = clampf(dist * 0.22f + 1.2f, 3.4f, 6.5f);
    steer = clampf(err * 2.3f, -1.f, 1.f);
    float bias = y_ < 70.f ? 0.35f : 0.05f;
    row = clampf((desired - speed_) * 0.85f + bias, -1.f, 1.f);
}

bool Game::hullOnGrass() const {
    const float c = std::cos(heading_);
    const float s = std::sin(heading_);
    const float lx[4] = {-kHullX, kHullX, kHullX, -kHullX};
    const float ly[4] = {-kHullY, -kHullY, kHullY, kHullY};
    for (int i = 0; i < 4; i++) {
        float wx = x_ + lx[i] * c - ly[i] * s;
        float wy = y_ + lx[i] * s + ly[i] * c;
        if (!inGrass(wx, wy)) return false;
    }
    return true;
}

bool Game::bowOnGrass() const {
    const float bx = x_ + std::sin(heading_) * kHullY;
    const float by = y_ + std::cos(heading_) * kHullY;
    return inGrass(bx, by);
}

void Game::physics(float dt, float row, float steer) {
    const bool on = hullOnGrass();
    const bool bow = bowOnGrass();
    float drag = 0.72f;
    float accel = 18.f;
    float cap = 12.2f;
    if (on) {
        drag = 2.4f;
        accel = 5.f;
        cap = 6.f;
    } else if (bow) {
        drag = 0.40f;
        accel = 11.f;
        cap = 11.f;
    }
    speed_ += row * accel * dt;
    speed_ -= speed_ * drag * dt;
    if (on && row > -0.05f && speed_ > 0.f) speed_ = std::max(0.f, speed_ - 1.6f * dt);
    speed_ = clampf(speed_, -5.f, cap);
    const float turn = (on ? 1.1f : 1.85f) + std::fabs(speed_) * 0.04f;
    heading_ = wrapPi(heading_ + steer * turn * dt);
    x_ += std::sin(heading_) * speed_ * dt;
    y_ += std::cos(heading_) * speed_ * dt;
    if (std::fabs(x_) > 60.f) x_ = std::copysign(60.f, x_);
    if (y_ < -8.f) y_ = -8.f;
    if (y_ > 148.f) y_ = 148.f;
    if (row > 0.2f && !on) stroke_ += dt * 2.5f;
    if (std::fabs(speed_) > 1.6f && !on) {
        Foam& f = foam_[foamN_ % 12];
        f.x = x_ - std::sin(heading_) * 7.f;
        f.y = y_ - std::cos(heading_) * 7.f;
        f.life = 1.f;
        foamN_++;
    }
    for (int i = 0; i < 12; i++) foam_[i].life -= dt * 0.85f;
}

void Game::audio(float dt, float row) {
    if (tone_ > 0.f) {
        tone_ -= dt;
        if (tone_ <= 0.f) sys_->apu.tone(0, 0, 0);
    }
    if (mode_ == Mode::Row && row > 0.2f && !hullOnGrass()) {
        float phase = stroke_ - std::floor(stroke_);
        if (phase < dt * 3.f) sys_->apu.tone(1, 150.f + std::fabs(speed_) * 5.f, 0.04f);
        else if (phase > 0.55f && phase < 0.55f + dt * 3.f) sys_->apu.tone(1, 0, 0);
    } else {
        sys_->apu.tone(1, 0, 0);
    }
    if (fanStep_ >= 0) {
        static const float winN[] = {392.f, 523.f, 659.f, 784.f};
        static const float loseN[] = {220.f, 174.f, 130.f};
        fanStep_++;
        if (won_) {
            if (fanStep_ % 9 == 1 && fanStep_ < 36) sys_->apu.tone(0, winN[fanStep_ / 9], 0.1f);
        } else if (fanStep_ % 12 == 1 && fanStep_ < 36) {
            sys_->apu.tone(0, loseN[fanStep_ / 12], 0.1f);
        }
        if (fanStep_ > 52) {
            fanStep_ = -1;
            sys_->apu.tone(0, 0, 0);
        }
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal) {
    if (h < 1.5f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 48 || s.x + s.w < -48 || s.y > gs::SCREEN_H + 48 || s.y + s.h < -48) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::worldToScreen(float wx, float wy, float& sx, float& sy) const {
    sx = (wx - camX_) * zoom_ + 160.f;
    sy = 112.f - (wy - camY_) * zoom_;
}

void Game::place(const gs::Mipped& m, float wx, float wy, float worldH, int pal) {
    float sx, sy;
    worldToScreen(wx, wy, sx, sy);
    spr(m, sx, sy, worldH * zoom_, pal);
}

int Game::shellFrame() const {
    float h = heading_;
    if (h < 0.f) h += kTau;
    int face = int(h / (kTau / 8.f) + 0.5f) & 7;
    int phase = int(stroke_ * 2.f) & 1;
    return face * 2 + phase;
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = 0;
    while (s[n]) n++;
    hud(20 - n / 2, row, s, pal);
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    int scrollX = int(std::lround(camX_ * zoom_));
    int scrollY = int(std::lround(-camY_ * zoom_ + t_ * 4.f));
    vdp.B.scroll(scrollX, scrollY);
    float shoreS = 0, shoreY = 0;
    worldToScreen(0.f, kGrassY0, shoreS, shoreY);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < 28) vdp.lineBackdrop[y] = gs::rgb4(5, 8, 12);
        else if (y < int(shoreY)) vdp.lineBackdrop[y] = gs::rgb4(1, 4, 9);
        else vdp.lineBackdrop[y] = gs::rgb4(2, 7, 2);
        vdp.road[y].on = false;
    }

    for (const V2& p : kMat) place(art_.mat, p.x, p.y, 16.f, PAL_GRASS);
    for (const V2& p : kTree) place(art_.tree, p.x, p.y, 14.f, PAL_STAKE);
    for (const V2& p : kReed) place(art_.reed, p.x, p.y, 9.f, PAL_REED);
    for (const V2& p : kTuft) place(art_.tuft, p.x, p.y, 4.2f, PAL_GRASS);
    place(art_.dock, 0.f, 2.f, 10.f, PAL_DOCK);

    const float stakes[][2] = {{-kGrassX, kGrassY0}, {kGrassX, kGrassY0}, {-kGrassX, kGrassY1},
                               {kGrassX, kGrassY1},  {-kGrassX, 102.f},    {kGrassX, 102.f}};
    for (const auto& s : stakes) place(art_.stake, s[0], s[1], 6.5f, PAL_STAKE);

    for (const Foam& f : foam_) {
        if (f.life <= 0.f) continue;
        place(art_.foam, f.x, f.y, 2.2f + f.life, PAL_FOAM);
    }

    float sx = x_, sy = y_;
    if (mode_ == Mode::Title) {
        sx = 0.f;
        sy = 16.f + std::sin(t_ * 1.4f) * 0.4f;
    }
    place(art_.shell[shellFrame()], sx, sy, 18.f, PAL_SHELL);

    char line[48];
    if (mode_ == Mode::Title) {
        hudC(3, "S3 SCULL GRASS", PAL_HUD);
        hudC(6, "LAND ON THE GRASS", PAL_HUD);
        hudC(8, "COME TO A FULL STOP", PAL_HUD);
        hudC(10, "THE CLOCK IS THE OTHER CREW", PAL_HUD);
        hudC(20, "A ROW    ARROWS STEER", PAL_HUD);
        hudC(22, "START", PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        hudC(12, "PAUSED", PAL_HUD);
        hudC(15, "START", PAL_HUD);
    } else {
        std::snprintf(line, sizeof(line), "YOU  %4.1f", you_);
        hud(1, 1, line, PAL_HUD);
        std::snprintf(line, sizeof(line), "CREW %4.1f", crew_);
        hud(1, 2, line, 2);
        std::snprintf(line, sizeof(line), "SPD %4.1f", std::fabs(speed_));
        hud(28, 1, line, PAL_HUD);
        if (hullOnGrass()) hud(28, 2, "GRASS", 3);
        else if (bowOnGrass()) hud(28, 2, "BANK", 2);
        else hud(28, 2, "WATER", 5);
        if (mode_ == Mode::Win) {
            hudC(10, "FULL STOP", 3);
            hudC(12, "ON THE GRASS", 3);
            hudC(14, "AHEAD OF THE CREW", PAL_HUD);
        } else if (mode_ == Mode::Fail) {
            hudC(10, "LEG LOST", 4);
            hudC(12, why_, 4);
            hudC(16, "START", PAL_HUD);
        } else if (hold_ > 0.05f) {
            hudC(24, "HOLD STILL", 2);
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    const gs::Pad& p = sys.pad;
    float row = 0.f;

    if (mode_ == Mode::Title) {
        camX_ += (0.f - camX_) * 0.04f;
        camY_ += (64.f - camY_) * 0.04f;
        zoom_ += (1.7f - zoom_) * 0.04f;
        stroke_ += kDt * 0.8f;
        if (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A) || bot_) {
            begin();
            mode_ = Mode::Row;
            blip(440.f);
        }
    } else if (mode_ == Mode::Pause) {
        if (p.pressed(gs::BTN_START)) {
            mode_ = Mode::Row;
            blip(330.f);
        }
    } else if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        camX_ += (x_ - camX_) * 0.08f;
        camY_ += (y_ - camY_) * 0.08f;
        if (!bot_ && p.pressed(gs::BTN_START)) {
            begin();
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
        }
    } else {
        if (!bot_ && p.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        float steer = 0.f;
        if (bot_) pilot(row, steer);
        else controls(row, steer);
        physics(kDt, row, steer);
        you_ += kDt;
        if (hullOnGrass() && std::fabs(speed_) <= kStop) hold_ += kDt;
        else hold_ = 0.f;
        if (hold_ >= kHoldNeed && you_ < crew_) finish(true, "full stop");
        else if (you_ >= crew_) finish(false, "the other crew");
        else if (y_ > 140.f) finish(false, "past the grass");
        else if (std::fabs(x_) > 52.f) finish(false, "left the reach");
        float lead = 6.f + y_ * 0.04f;
        camX_ += (x_ - camX_) * 0.08f;
        camY_ += ((y_ + lead) - camY_) * 0.08f;
        float wantZ = hullOnGrass() ? 2.7f : 2.35f;
        zoom_ += (wantZ - zoom_) * 0.05f;
    }
    audio(kDt, mode_ == Mode::Row ? row : 0.f);
    draw();
}

}  // namespace scullgrass

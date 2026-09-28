#include "game/scull.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace scullbox {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;
constexpr float kCrew = 42.f;
constexpr float kBoxX = 0.f;
constexpr float kBoxY = 96.f;
constexpr float kBoxHW = 14.f;
constexpr float kBoxHH = 20.f;
constexpr float kHoldNeed = 0.7f;
constexpr float kStop = 0.32f;

struct V2 {
    float x, y;
};

constexpr V2 kReed[] = {{-36.f, 18.f}, {-40.f, 48.f}, {-38.f, 78.f}, {-34.f, 112.f}, {-28.f, 138.f},
                        {36.f, 16.f},  {40.f, 50.f},  {38.f, 82.f},  {34.f, 116.f}, {30.f, 140.f}};

float wrapPi(float a) {
    while (a > kPi) a -= kTau;
    while (a < -kPi) a += kTau;
    return a;
}

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.08f);
    tone_ = 0.07f;
}

void Game::begin() {
    x_ = 0.f;
    y_ = 10.f;
    heading_ = 0.f;
    speed_ = 0.f;
    stroke_ = 0.f;
    you_ = 0.f;
    hold_ = 0.f;
    won_ = false;
    over_ = false;
    fanStep_ = -1;
    foamN_ = 0;
    camX_ = x_;
    camY_ = y_;
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
    sys.vdp.setFogColor(gs::rgb4(1, 4, 8));
    sys.apu.setMaster(0.7f);
    sys.apu.setEcho(0.1f, 0.18f, 0.08f);
    begin();
    if (bot_) {
        mode_ = Mode::Row;
        zoom_ = 2.5f;
    } else {
        mode_ = Mode::Title;
        zoom_ = 1.7f;
        camX_ = 0.f;
        camY_ = 52.f;
    }
}

void Game::controls(float& row, float& steer) {
    const gs::Pad& p = sys_->pad;
    steer = 0.f;
    row = 0.f;
    if (p.down(gs::BTN_LEFT)) steer -= 1.f;
    if (p.down(gs::BTN_RIGHT)) steer += 1.f;
    if (p.down(gs::BTN_A) || p.down(gs::BTN_C) || p.down(gs::BTN_UP)) row += 1.f;
    if (p.down(gs::BTN_B) || p.down(gs::BTN_DOWN)) row -= 1.f;
}

void Game::pilot(float& row, float& steer) {
    const float dx = kBoxX - x_;
    const float dy = kBoxY - y_;
    const float dist = std::hypot(dx, dy);
    const float fx = std::sin(heading_);
    const float fy = std::cos(heading_);
    const float along = fx * dx + fy * dy;
    const float side = fy * dx - fx * dy;

    if (hullInside() && std::fabs(speed_) < kStop + 0.15f) {
        row = 0.f;
        steer = clampf(wrapPi(0.f - heading_) * 2.f, -1.f, 1.f) * 0.25f;
        if (std::fabs(speed_) < kStop) steer = 0.f;
        return;
    }

    float err = wrapPi(std::atan2(dx, dy) - heading_);
    float desired = std::min(11.f, dist * 0.34f);
    if (dist < 8.f) {
        err = wrapPi(0.f - heading_);
        desired = clampf(along * 0.55f, -3.5f, 3.5f);
        if (std::fabs(side) > 1.2f) desired *= 0.45f;
    } else if (std::fabs(err) > 0.85f) {
        desired = std::min(desired, 2.2f);
    }
    steer = clampf(err * 2.4f + (dist < 12.f ? side * 0.08f : 0.f), -1.f, 1.f);
    const float gap = desired - speed_;
    row = clampf(gap * 1.4f, -1.f, 1.f);
}

void Game::physics(float dt, float row, float steer) {
    speed_ += row * 20.f * dt;
    speed_ -= speed_ * 1.35f * dt;
    speed_ = clampf(speed_, -6.f, 13.f);
    const float turn = 2.05f + std::fabs(speed_) * 0.06f;
    heading_ = wrapPi(heading_ + steer * turn * dt);
    x_ += std::sin(heading_) * speed_ * dt;
    y_ += std::cos(heading_) * speed_ * dt;
    if (row > 0.2f) stroke_ += dt * 2.6f;
    if (std::fabs(speed_) > 1.4f) {
        Foam& f = foam_[foamN_ % 12];
        f.x = x_ - std::sin(heading_) * 6.f;
        f.y = y_ - std::cos(heading_) * 6.f;
        f.life = 1.f;
        foamN_++;
    }
    for (int i = 0; i < 12; i++) foam_[i].life -= dt * 0.8f;
}

bool Game::hullInside() const {
    const float c = std::cos(heading_);
    const float s = std::sin(heading_);
    const float hx = 1.7f, hy = 8.2f;
    const float lx[4] = {-hx, hx, hx, -hx};
    const float ly[4] = {-hy, -hy, hy, hy};
    for (int i = 0; i < 4; i++) {
        float wx = x_ + lx[i] * c - ly[i] * s;
        float wy = y_ - lx[i] * s - ly[i] * c;
        if (wx < kBoxX - kBoxHW || wx > kBoxX + kBoxHW) return false;
        if (wy < kBoxY - kBoxHH || wy > kBoxY + kBoxHH) return false;
    }
    return true;
}

void Game::audio(float dt, float row) {
    if (tone_ > 0.f) {
        tone_ -= dt;
        if (tone_ <= 0.f) sys_->apu.tone(0, 0, 0);
    }
    if (mode_ == Mode::Row && row > 0.2f) {
        float phase = stroke_ - std::floor(stroke_);
        if (phase < dt * 3.f) sys_->apu.tone(1, 160.f + std::fabs(speed_) * 6.f, 0.045f);
        else if (phase > 0.55f && phase < 0.55f + dt * 3.f) sys_->apu.tone(1, 0, 0);
    } else {
        sys_->apu.tone(1, 0, 0);
    }
    if (fanStep_ >= 0) {
        static const float notes[] = {392.f, 523.f, 659.f, 784.f};
        fanStep_++;
        if (fanStep_ % 9 == 1 && fanStep_ < 36) sys_->apu.tone(0, notes[fanStep_ / 9], 0.1f);
        if (fanStep_ > 50) {
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
    if (s.x > gs::SCREEN_W + 40 || s.x + s.w < -40 || s.y > gs::SCREEN_H + 40 || s.y + s.h < -40) return;
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
    int scrollY = int(std::lround(-camY_ * zoom_ + t_ * 5.f));
    vdp.B.scroll(scrollX, scrollY);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int band = 4 + ((y + int(t_ * 14.f)) / 32) % 3;
        vdp.lineBackdrop[y] = gs::rgb4(1, band, 9);
        vdp.road[y].on = false;
    }

    float bob = std::sin(t_ * 1.6f);
    float sx = x_, sy = y_;
    if (mode_ == Mode::Title) {
        sx = 0.f;
        sy = 18.f + bob;
    }
    place(art_.shell[shellFrame()], sx, sy, 18.f, PAL_SHELL);

    const float corners[4][2] = {{kBoxX - kBoxHW, kBoxY - kBoxHH},
                                 {kBoxX + kBoxHW, kBoxY - kBoxHH},
                                 {kBoxX - kBoxHW, kBoxY + kBoxHH},
                                 {kBoxX + kBoxHW, kBoxY + kBoxHH}};
    for (int i = 0; i < 4; i++) place(art_.post, corners[i][0], corners[i][1] + bob * 0.3f, 7.f, PAL_BOX);
    for (int i = 1; i < 5; i++) {
        float u = i / 5.f;
        place(art_.dash, kBoxX - kBoxHW + u * (2.f * kBoxHW), kBoxY - kBoxHH, 1.6f, PAL_BOX);
        place(art_.dash, kBoxX - kBoxHW + u * (2.f * kBoxHW), kBoxY + kBoxHH, 1.6f, PAL_BOX);
    }
    for (int i = 1; i < 6; i++) {
        float u = i / 6.f;
        float yy = kBoxY - kBoxHH + u * (2.f * kBoxHH);
        place(art_.dash, kBoxX - kBoxHW, yy, 1.6f, PAL_BOX);
        place(art_.dash, kBoxX + kBoxHW, yy, 1.6f, PAL_BOX);
    }

    place(art_.dock, 0.f, -2.f, 12.f, PAL_DOCK);
    for (const V2& r : kReed) place(art_.reed, r.x, r.y, 8.f, PAL_REED);
    for (int i = 0; i < 12; i++) {
        if (foam_[i].life > 0.f) place(art_.foam, foam_[i].x, foam_[i].y, 2.4f + foam_[i].life, PAL_FOAM);
    }

    char line[40];
    if (mode_ == Mode::Title) {
        hudC(3, "S3 SCULL BOX", PAL_HUD);
        hudC(6, "STOP INSIDE THE BOX", 2);
        hudC(8, "A FULL STOP. OARS IN.", PAL_HUD);
        hudC(10, "THE CLOCK IS THE OTHER CREW", 3);
        hudC(18, "A ROW   B CHECK   ARROWS STEER", PAL_HUD);
        hudC(21, "START", 2);
    } else if (mode_ == Mode::Pause) {
        hudC(12, "PAUSED", 2);
        hudC(15, "START CONTINUES", PAL_HUD);
    } else {
        int sec = int(you_);
        int cs = int(you_ * 100.f) % 100;
        std::snprintf(line, sizeof line, "YOU %d:%02d.%02d", sec / 60, sec % 60, cs);
        hud(1, 1, line, PAL_HUD);
        int csec = int(crew_);
        std::snprintf(line, sizeof line, "CREW %d:%02d", csec / 60, csec % 60);
        hud(26, 1, line, 5);
        if (mode_ == Mode::Row) {
            if (hullInside()) hudC(25, "INSIDE  HOLD STILL", 3);
            else hudC(25, "THE BOX IS AHEAD", 2);
        } else if (mode_ == Mode::Win) {
            hudC(11, "STOPPED", 3);
            hudC(13, "INSIDE THE BOX", 2);
            if (!bot_) hudC(16, "START ROWS AGAIN", PAL_HUD);
        } else if (mode_ == Mode::Fail) {
            hudC(11, "THE OTHER CREW", 4);
            hudC(13, "TOOK THE BOX", 4);
            if (!bot_) hudC(16, "START ROWS AGAIN", PAL_HUD);
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    const gs::Pad& pad = sys.pad;
    float row = 0.f;

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) {
            begin();
            mode_ = Mode::Row;
            zoom_ = 2.5f;
            blip(620.f);
        }
    } else if (mode_ == Mode::Row) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(280.f);
        } else {
            float steer = 0.f;
            if (bot_) pilot(row, steer);
            else controls(row, steer);
            you_ += kDt;
            physics(kDt, row, steer);
            if (hullInside() && std::fabs(speed_) < kStop) hold_ += kDt;
            else hold_ = 0.f;
            if (hold_ >= kHoldNeed) {
                mode_ = Mode::Win;
                won_ = true;
                over_ = true;
                fanStep_ = 0;
                blip(740.f);
            } else if (you_ >= crew_) {
                mode_ = Mode::Fail;
                won_ = false;
                over_ = true;
                blip(110.f);
            }
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) mode_ = Mode::Row;
    } else if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C))) {
        begin();
        mode_ = Mode::Row;
        zoom_ = 2.5f;
        blip(620.f);
    }

    if (mode_ == Mode::Title) {
        camX_ = std::sin(t_ * 0.2f) * 4.f;
        camY_ = 54.f + std::sin(t_ * 0.13f) * 3.f;
        zoom_ = 1.65f;
    } else {
        float gx = x_ + std::sin(heading_) * 6.f;
        float gy = y_ + std::cos(heading_) * 6.f;
        float k = 1.f - std::exp(-kDt * 4.f);
        camX_ += (gx - camX_) * k;
        camY_ += (gy - camY_) * k;
        zoom_ = 2.5f;
    }
    audio(kDt, row);
    draw();
}

}  // namespace scullbox

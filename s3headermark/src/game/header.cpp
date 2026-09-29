#include "game/header.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace headermark {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;
constexpr float kMarkX = 156.f;
constexpr float kMarkY = 16.f;
constexpr float kOn = 7.2f;
constexpr float kStop = 0.48f;
constexpr float kHoldNeed = 0.55f;
constexpr float kClock = 26.f;
constexpr float kPast = 196.f;
// Wind blows toward +Y. A header on an easterly reach still fills.
constexpr float kWindFrom = -kPi * 0.5f;

float wrapPi(float a) {
    while (a > kPi) a -= kTau;
    while (a < -kPi) a += kTau;
    return a;
}

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

float sailFill(float heading) {
    float into = std::fabs(wrapPi(heading - (kWindFrom + kPi)));
    if (into < 0.48f) return 0.f;
    float beam = std::sin(into);
    float run = into > 2.2f ? 0.58f : 0.f;
    return std::max(beam, run);
}

}  // namespace

void Game::begin() {
    x_ = 22.f;
    y_ = -4.f;
    heading_ = 0.12f;
    speed_ = 0.f;
    hold_ = 0.f;
    still_ = 0.f;
    raceT_ = 0.f;
    clock_ = kClock;
    won_ = false;
    over_ = false;
    why_.clear();
    camX_ = x_;
    camY_ = y_;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = true;
    sys.vdp.B.resize(64, 32);
    for (int y = 0; y < sys.vdp.B.h; y++)
        for (int x = 0; x < sys.vdp.B.w; x++) sys.vdp.B.set(x, y, gs::entry(art_.waterTile, PAL_WATER));
    sys.vdp.setFogColor(gs::rgb4(1, 3, 7));
    sys.apu.setMaster(0.62f);
    sys.apu.setEcho(0.1f, 0.16f, 0.07f);
    begin();
    mode_ = bot_ ? Mode::Sail : Mode::Title;
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Win || mode_ == Mode::Lose) return 4;
    if (hold_ > 0.08f) return 3;
    if (onMark()) return 2;
    return 1;
}

void Game::helm(float& sheet, float& helmIn) {
    const gs::Pad& p = sys_->pad;
    helmIn = 0.f;
    sheet = 0.f;
    if (p.down(gs::BTN_LEFT)) helmIn -= 1.f;
    if (p.down(gs::BTN_RIGHT)) helmIn += 1.f;
    if (p.axisX > 0.2f || p.axisX < -0.2f) helmIn = clampf(helmIn + p.axisX, -1.f, 1.f);
    if (p.down(gs::BTN_UP) || p.down(gs::BTN_A) || p.down(gs::BTN_C)) sheet += 1.f;
    if (p.down(gs::BTN_DOWN) || p.down(gs::BTN_B)) sheet -= 1.f;
}

void Game::pilot(float& sheet, float& helmIn) {
    const float dx = kMarkX - x_;
    const float dy = kMarkY - y_;
    const float dist = std::hypot(dx, dy);
    const float fx = std::cos(heading_);
    const float fy = std::sin(heading_);
    const float along = fx * dx + fy * dy;
    const float side = -fy * dx + fx * dy;

    if (dist < kOn && std::fabs(speed_) < kStop + 0.4f) {
        sheet = 0.f;
        helmIn = 0.f;
        if (std::fabs(speed_) > kStop * 0.6f) sheet = speed_ > 0.f ? -0.7f : 0.4f;
        return;
    }

    float aim = std::atan2(dy, dx);
    float err = wrapPi(aim - heading_);
    float desired = std::min(42.f, 11.f + dist * 0.2f);
    if (dist < 40.f) desired = clampf(dist * 0.42f, 3.5f, 15.f);
    if (std::fabs(side) > 8.f && dist < 50.f) desired = std::min(desired, 9.f);
    if (dist < 14.f) {
        err = wrapPi(aim - heading_) * 0.45f;
        desired = clampf(along * 0.6f, -5.f, 7.f);
    } else if (std::fabs(err) > 0.7f) {
        desired = std::min(desired, 9.f);
    }
    helmIn = clampf(err * 1.9f, -1.f, 1.f);
    float fill = std::max(0.22f, sailFill(heading_));
    sheet = clampf((desired - speed_) / (46.f * fill), -1.f, 1.f);
}

bool Game::onMark() const { return std::hypot(x_ - kMarkX, y_ - kMarkY) <= kOn; }

void Game::update(float dt) {
    float sheet = 0.f, helmIn = 0.f;
    if (bot_) pilot(sheet, helmIn);
    else helm(sheet, helmIn);

    float fill = sailFill(heading_);
    if (sheet > 0.f) speed_ += sheet * fill * 50.f * dt;
    else speed_ += sheet * 30.f * dt;
    float drag = 1.2f + (sheet <= 0.05f ? 1.5f : 0.f) + (fill < 0.05f && sheet > 0.f ? 2.5f : 0.f);
    speed_ -= speed_ * drag * dt;
    speed_ = clampf(speed_, -9.f, 48.f);
    float turn = (1.2f + std::min(std::fabs(speed_), 22.f) * 0.05f) * (std::fabs(speed_) < 1.1f ? 0.4f : 1.f);
    heading_ = wrapPi(heading_ + helmIn * turn * dt);
    x_ += std::cos(heading_) * speed_ * dt;
    y_ += std::sin(heading_) * speed_ * dt;

    raceT_ += dt;
    clock_ -= dt;
    bool on = onMark();
    if (on && std::fabs(speed_) < kStop) hold_ += dt;
    else hold_ = 0.f;
    if (!on && std::fabs(speed_) < kStop * 0.65f) still_ += dt;
    else still_ = 0.f;

    if (hold_ >= kHoldNeed) {
        mode_ = Mode::Win;
        won_ = true;
        over_ = true;
        why_ = "set down on the mark";
        sys_->apu.tone(0, 523.f, 0.12f);
        tone_ = 0.28f;
    } else if (x_ > kPast) {
        mode_ = Mode::Lose;
        won_ = false;
        over_ = true;
        why_ = "past the mark";
    } else if (still_ > 1.25f) {
        mode_ = Mode::Lose;
        won_ = false;
        over_ = true;
        why_ = "set down off the mark";
    } else if (clock_ <= 0.f) {
        mode_ = Mode::Lose;
        won_ = false;
        over_ = true;
        why_ = "the other crew took the mark";
        clock_ = 0.f;
    }

    if (tone_ > 0.f) {
        tone_ -= dt;
        if (tone_ <= 0.f) sys_->apu.tone(0, 0, 0);
    } else if (mode_ == Mode::Sail && sheet > 0.2f && fill > 0.2f) {
        sys_->apu.tone(1, 64.f + std::fabs(speed_) * 2.4f, 0.028f);
    } else {
        sys_->apu.tone(1, 0, 0);
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

void Game::place(const gs::Mipped& m, float wx, float wy, float h, int pal) {
    if (h < 1.2f || m.h < 1) return;
    float sx = (wx - camX_) + gs::SCREEN_W * 0.5f;
    float sy = gs::SCREEN_H * 0.5f - (wy - camY_);
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(sx - s.w * 0.5f));
    s.y = int16_t(std::lround(sy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 48 || s.x + s.w < -48 || s.y > gs::SCREEN_H + 48 || s.y + s.h < -48) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    float lookX = x_ + (kMarkX - x_) * 0.28f;
    float lookY = y_ + (kMarkY - y_) * 0.28f;
    if (mode_ == Mode::Title) {
        lookX = 96.f;
        lookY = 8.f;
    }
    camX_ += (lookX - camX_) * 0.12f;
    camY_ += (lookY - camY_) * 0.12f;

    for (int row = 0; row < gs::SCREEN_H; row++) {
        float wy = camY_ + (gs::SCREEN_H * 0.5f - row);
        int band = int(std::floor(wy / 18.f)) & 1;
        v.lineBackdrop[row] = band ? gs::rgb4(1, 5, 10) : gs::rgb4(1, 6, 12);
        v.road[row].on = false;
        v.lineFog[row] = 0;
    }
    v.B.scroll(int(-camX_), int(camY_));

    place(art_.mark, kMarkX, kMarkY, 52.f, PAL_MARK);
    place(art_.buoy, kMarkX, kMarkY, 18.f, PAL_BUOY);

    if (std::fabs(speed_) > 2.2f) {
        place(art_.wake, x_ - std::cos(heading_) * 18.f, y_ - std::sin(heading_) * 18.f,
              12.f + std::fabs(speed_) * 0.12f, PAL_WAKE);
    }
    int frame = int(std::floor((heading_ / kTau) * 16.f + 16.5f)) & 15;
    place(art_.hull[frame], x_, y_, 40.f, PAL_HULL);

    char line[48];
    if (mode_ == Mode::Title) {
        hudC(6, "S3 HEADERMARK", PAL_HUD);
        hudC(9, "SET DOWN ON THE MARK", PAL_HUD);
        hudC(12, "THE HEADER HAS ONE JOB", PAL_HUD);
        hudC(18, "ENTER TO CAST OFF", PAL_HUD);
        hudC(21, "ARROWS STEER   Z SHEETS IN", PAL_HUD);
    } else if (mode_ == Mode::Sail) {
        std::snprintf(line, sizeof(line), "CLOCK %04.1f", clock_);
        hud(1, 1, line, PAL_HUD);
        hud(1, 2, onMark() ? "ON THE MARK" : "OFF THE MARK", PAL_HUD);
        std::snprintf(line, sizeof(line), "WAY %.0f", speed_);
        hud(30, 1, line, PAL_HUD);
        hudC(26, "COME TO REST ON THE PAINT", PAL_HUD);
    } else if (mode_ == Mode::Win) {
        hudC(10, "SET DOWN", PAL_HUD);
        hudC(12, "ON THE MARK", PAL_HUD);
    } else {
        hudC(10, "LEG FAILED", PAL_HUD);
        hudC(12, why_, PAL_HUD);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    const gs::Pad& p = sys.pad;
    if (mode_ == Mode::Title) {
        if (p.pressed(gs::BTN_MODE) && sys.hasHome()) sys.eject();
        if (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C)) {
            begin();
            mode_ = Mode::Sail;
            sys.apu.tone(0, 330.f, 0.08f);
            tone_ = 0.1f;
        }
    } else if (mode_ == Mode::Sail) {
        update(kDt);
    } else if ((mode_ == Mode::Win || mode_ == Mode::Lose) &&
               (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A))) {
        begin();
        mode_ = Mode::Title;
        over_ = false;
        won_ = false;
    }
    draw();
}

}  // namespace headermark

#include "game/game.h"

#include "game/world.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace cliffboom {
namespace {

constexpr float PI = 3.14159265f;
constexpr float TAU = PI * 2.f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

float wrapAng(float a) {
    while (a > PI) a -= TAU;
    while (a < -PI) a += TAU;
    return a;
}

struct Wp {
    float x, y, arrive;
    bool crawl;
};

// Stay in the middle of the shelf, then crawl up to the gate.
const Wp kRoute[] = {
    {140.f, 184.f, 16.f, false}, {196.f, 160.f, 16.f, false}, {206.f, 96.f, 16.f, false},
    {250.f, 74.f, 16.f, false},  {330.f, 78.f, 16.f, false},  {362.f, 120.f, 16.f, false},
    {368.f, 170.f, 14.f, false}, {396.f, 184.f, 8.f, true},
};
constexpr int kRouteN = int(sizeof(kRoute) / sizeof(kRoute[0]));

}  // namespace

int Game::headingFrame() const {
    float a = heading_;
    while (a < 0) a += TAU;
    while (a >= TAU) a -= TAU;
    int i = int((a + PI / 8.f) / (PI / 4.f));
    return i & 7;
}

bool Game::startPressed() const {
    const gs::Pad& p = sys_->pad;
    return p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C);
}

bool Game::inPocket() const {
    return x_ >= POCKET_X0 && x_ <= POCKET_X1 && y_ >= POCKET_Y0 && y_ <= POCKET_Y1;
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.08f);
    melodyT_ = 0.07f;
    melody_ = -2;
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

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool shadow) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::fail(const char* why) {
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    phase_ = 4;
    why_ = why;
    hold_ = 0;
    sys_->apu.tone(1, 0, 0);
    sys_->apu.noiseBurst(0.5f, 620.f, 0.4f);
    sys_->rumble(0.7f, 0.4f, 180);
}

void Game::succeed() {
    mode_ = Mode::Win;
    over_ = true;
    won_ = true;
    phase_ = 4;
    why_ = "the drive is on the boom";
    melody_ = 0;
    melodyT_ = 0;
    sys_->apu.tone(1, 0, 0);
    sys_->rumble(0.2f, 0.5f, 140);
}

void Game::begin() {
    x_ = START_X;
    y_ = START_Y;
    heading_ = 0;
    speed_ = 0;
    hold_ = 0;
    legT_ = 0;
    wp_ = 0;
    won_ = false;
    over_ = false;
    phase_ = 1;
    melody_ = -1;
    why_ = "the leg ran out";
    mode_ = Mode::Run;
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(1, 0, 0);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = float(y) / float(gs::SCREEN_H - 1);
        int shade = 1 + int((1.f - u) * 3.f);
        sys.vdp.lineBackdrop[y] = gs::rgb4(1, shade, shade + 2);
        sys.vdp.road[y].on = false;
    }
    sys.vdp.setFogColor(gs::rgb4(2, 3, 4));
    sys.vdp.A.enabled = false;
    x_ = START_X;
    y_ = START_Y;
    heading_ = 0;
    if (bot_) begin();
    else {
        mode_ = Mode::Title;
        phase_ = 0;
    }
}

void Game::steerOf(float& steer, float& throttle, float& brake) const {
    const gs::Pad& p = sys_->pad;
    steer = p.axisX;
    if (p.down(gs::BTN_LEFT)) steer -= 1.f;
    if (p.down(gs::BTN_RIGHT)) steer += 1.f;
    steer = clampf(steer, -1.f, 1.f);
    throttle = p.accel;
    if (p.down(gs::BTN_UP) || p.down(gs::BTN_A)) throttle = 1.f;
    if (p.axisY > 0.25f) throttle = std::max(throttle, p.axisY);
    brake = p.brake;
    if (p.down(gs::BTN_DOWN) || p.down(gs::BTN_B)) brake = 1.f;
    if (p.axisY < -0.25f) brake = std::max(brake, -p.axisY);
}

void Game::botDrive(float& steer, float& throttle, float& brake) {
    steer = 0;
    throttle = 0;
    brake = 0;
    if (mode_ != Mode::Run) {
        brake = 1.f;
        return;
    }
    if (inPocket() && speed_ < 10.f) {
        brake = 1.f;
        return;
    }
    if (wp_ >= kRouteN) {
        brake = 1.f;
        return;
    }
    const Wp& w = kRoute[wp_];
    float dx = w.x - x_;
    float dy = w.y - y_;
    float dist = std::sqrt(dx * dx + dy * dy);
    float err = wrapAng(std::atan2(dy, dx) - heading_);
    steer = clampf(err * 2.6f, -1.f, 1.f);
    float ae = std::fabs(err);
    float cap = w.crawl ? (dist > 18.f ? 18.f : 7.f) : (ae > 0.65f ? 22.f : ae > 0.28f ? 36.f : 52.f);
    if (speed_ > cap) brake = 1.f;
    else throttle = (w.crawl && dist < 14.f) ? 0.25f : 1.f;
    if (dist < w.arrive && ae < 0.9f) wp_++;
}

void Game::update(float dt) {
    if (mode_ != Mode::Run) return;
    legT_ += dt;
    if (legT_ > LEG_LIMIT) {
        fail("the leg ran out");
        return;
    }

    float steer = 0, throttle = 0, brake = 0;
    if (bot_) botDrive(steer, throttle, brake);
    else steerOf(steer, throttle, brake);

    heading_ = wrapAng(heading_ + steer * 2.15f * dt * (0.45f + std::min(std::fabs(speed_) / 40.f, 1.f)));
    float drive = throttle * 78.f;
    if (brake > 0.f) {
        if (std::fabs(speed_) < 5.f && throttle < 0.2f) {
            drive = 0.f;
            speed_ *= 0.4f;
        } else {
            drive += (speed_ >= 0.f ? -150.f : 150.f) * brake;
        }
    }
    speed_ += (drive - speed_ * 0.7f) * dt;
    speed_ = clampf(speed_, -16.f, 58.f);
    x_ += std::cos(heading_) * speed_ * dt;
    y_ += std::sin(heading_) * speed_ * dt;

    if (y_ > BOOM_Y0 && y_ < BOOM_Y1 && x_ > BOOM_X - 10.f) {
        if (speed_ > 16.f) {
            fail("broke the boom");
            return;
        }
        x_ = std::min(x_, BOOM_X - 10.f);
        if (speed_ > 0.f) speed_ = 0.f;
    }

    if (!onShelf(x_, y_)) {
        if (x_ > BOOM_X - 24.f && y_ > BOOM_Y0 - 8.f && y_ < BOOM_Y1 + 8.f) fail("missed the end");
        else fail("left the cliff");
        return;
    }

    if (std::fabs(speed_) > 8.f) sys_->apu.tone(1, 52.f + std::fabs(speed_) * 0.9f, 0.045f);
    else sys_->apu.tone(1, 0, 0);

    if (inPocket() && std::fabs(speed_) < 8.f) {
        hold_ += dt;
        if (hold_ > 0.15f && phase_ < 3) {
            phase_ = 3;
            blip(660.f);
        } else if (phase_ < 2) phase_ = 2;
        if (hold_ >= 1.05f) succeed();
    } else {
        if (hold_ > 0.f) hold_ = 0.f;
        phase_ = inPocket() ? 2 : 1;
    }
}

void Game::chime(float dt) {
    static const float notes[] = {392.f, 523.f, 659.f, 784.f};
    if (melody_ < 0 || melody_ >= 4) return;
    if (melodyT_ <= 0.f) sys_->apu.tone(0, notes[melody_], 0.09f);
    melodyT_ += dt;
    if (melodyT_ > 0.16f) {
        melodyT_ = 0;
        melody_++;
        if (melody_ >= 4) sys_->apu.tone(0, 0, 0);
    }
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_) return 4;
    if (phase_ >= 3) return 3;
    if (phase_ == 2) return 2;
    return 1;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const float dt = 1.f / 60.f;
    t_ += dt;
    if (melody_ == -2) {
        melodyT_ -= dt;
        if (melodyT_ <= 0.f) {
            melody_ = -1;
            sys.apu.tone(0, 0, 0);
        }
    }
    if ((mode_ == Mode::Title || mode_ == Mode::Win || mode_ == Mode::Fail) && startPressed()) begin();
    if (mode_ == Mode::Win) chime(dt);
    else if (mode_ == Mode::Run) update(dt);
    draw();
}

void Game::draw() {
    sys_->vdp.clearSprites();
    sys_->vdp.HUD.clear();
    for (int y = 0; y < gs::SCREEN_H; y++) sys_->vdp.road[y].on = false;

    camX_ = clampf(x_ - 160.f, 0.f, 512.f - gs::SCREEN_W);
    camY_ = clampf(y_ - 112.f, 0.f, 256.f - gs::SCREEN_H);
    sys_->vdp.B.scroll(int(std::lround(-camX_)), int(std::lround(camY_)));

    auto toScreen = [&](float wx, float wy, float& sx, float& sy) {
        sx = wx - camX_;
        sy = wy - camY_;
    };
    float cx, cy, bx, by;
    toScreen(x_, y_, cx, cy);
    toScreen(BOOM_X + 1.f, (BOOM_Y0 + BOOM_Y1) * 0.5f, bx, by);

    if (mode_ == Mode::Title) spr(art_.banner, 160.f, 36.f, float(art_.banner.h) * 2.f, PAL_BANNER);

    spr(art_.shadow, cx + 3.f, cy + 12.f, 12.f, PAL_CAR, false, true);
    if (std::fabs(speed_) > 10.f) {
        float dx = cx - std::cos(heading_) * 16.f;
        float dy = cy - std::sin(heading_) * 16.f;
        spr(art_.dust, dx, dy, 8.f, PAL_DUST);
    }
    spr(art_.car[headingFrame()], cx, cy, 30.f, PAL_CAR);
    spr(art_.boom, bx, by, 68.f, PAL_BOOM);

    if (mode_ == Mode::Title) {
        hudC(8, "DELIVER THE DRIVE", PAL_WHITE);
        hudC(10, "TO THE BOOM", PAL_AMBER);
        hudC(12, "THE SHELF IS THE LEG", PAL_WHITE);
        hudC(14, "MISS THE END AND IT FAILS", PAL_RED);
        hudC(16, "STOP UNDER THE GATE", PAL_WHITE);
        hudC(18, "LEFT RIGHT STEERS", PAL_WHITE);
        hudC(19, "UP DRIVES   DOWN BRAKES", PAL_WHITE);
        hudC(22, "PRESS START", PAL_GREEN);
        return;
    }

    hud(1, 0, "S3 CLIFF BOOM", PAL_AMBER);
    int clock = std::max(0, int(std::ceil(LEG_LIMIT - legT_)));
    char line[40];
    std::snprintf(line, sizeof line, "LEG %d", clock);
    hud(30, 0, line, clock < 15 ? PAL_RED : PAL_WHITE);
    if (phase_ >= 2 && mode_ == Mode::Run) hud(1, 1, phase_ >= 3 ? "HOLD" : "POCKET", PAL_GREEN);

    if (mode_ == Mode::Win) {
        hudC(6, "DELIVERED", PAL_GREEN);
        hudC(8, "THE DRIVE IS ON THE BOOM", PAL_WHITE);
        hudC(10, "THE LEG IS MADE", PAL_AMBER);
        hudC(13, "PRESS START", PAL_WHITE);
    } else if (mode_ == Mode::Fail) {
        hudC(6, "LEG FAILED", PAL_RED);
        hudC(8, why_, PAL_WHITE);
        hudC(11, "PRESS START", PAL_AMBER);
    }
}

}  // namespace cliffboom

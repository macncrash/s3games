#include "game/slip.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace slip {

namespace {

constexpr float TAU = 6.28318530718f;
constexpr float DT = 1.f / 60.f;
constexpr float SL = 132.f;
constexpr float SR = 188.f;
constexpr float BACK = 22.f;
constexpr float CLIFF = 86.f;
constexpr float R = 8.f;
constexpr float TIDE0 = 46.f;
constexpr float CX = 160.f;

bool sheltered(float x, float y) { return y < CLIFF && x > SL && x < SR; }

}  // namespace

float Game::wrap(float a) {
    while (a > 3.14159265f) a -= TAU;
    while (a < -3.14159265f) a += TAU;
    return a;
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Win) return 2;
    if (mode_ == Mode::Lose) return 3;
    return 1;
}

bool Game::inSlip() const {
    return x_ > SL + R && x_ < SR - R && y_ > BACK + R && y_ < CLIFF - 18.f;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    t_ = 0;
    tide_ = TIDE0;
    gull_ = 0;
}

void Game::resetRun() {
    x_ = 168.f;
    y_ = 196.f;
    h_ = -1.5707963f;
    vx_ = vy_ = 0;
    t_ = 0;
    tide_ = TIDE0;
    held_ = 0;
    chime_ = 0;
    won_ = false;
    over_ = false;
    mode_ = Mode::Run;
}

void Game::botInput(float& steer, float& thrust) {
    bool outer = y_ > CLIFF - 2.f;
    float tx = CX;
    float ty = outer ? CLIFF - 18.f : 46.f;
    float dx = tx - x_;
    float dy = ty - y_;
    float dist = std::sqrt(dx * dx + dy * dy);
    float want = (!outer && dist < 16.f) ? -1.5707963f : std::atan2(dy, dx);
    float err = wrap(want - h_);
    steer = std::clamp(err * 2.2f, -1.f, 1.f);
    float spd = std::sqrt(vx_ * vx_ + vy_ * vy_);
    thrust = 0;
    if (std::fabs(err) < 0.7f) {
        if (outer && spd < 42.f) thrust = 1.f;
        else if (!outer && dist > 7.f && spd < 24.f) thrust = 0.5f;
    }
    if (!outer && spd > 16.f) thrust = -0.85f;
    if (inSlip() && spd > 10.f) thrust = -1.f;
    if (inSlip() && spd <= 10.f) thrust = 0.f;
}

void Game::physics(float dt, float steer, float thrust) {
    h_ = wrap(h_ + steer * 2.35f * dt);
    float accel = thrust > 0 ? 78.f * thrust : thrust < 0 ? 64.f * thrust : 0.f;
    float c = std::cos(h_), s = std::sin(h_);
    vx_ += c * accel * dt;
    vy_ += s * accel * dt;

    float age = TIDE0 - tide_;
    float expose = 1.f;
    if (sheltered(x_, y_)) {
        float depth = (CLIFF - y_) / (CLIFF - BACK);
        expose = std::clamp(1.f - depth, 0.f, 1.f);
    }
    float pushX = std::sin(t_ * 0.85f) * (12.f + age * 1.4f) * expose;
    float pushY = (5.f + age * 0.85f) * expose;
    vx_ += pushX * dt;
    vy_ += pushY * dt;

    float drag = std::exp(-1.85f * dt);
    vx_ *= drag;
    vy_ *= drag;
    x_ += vx_ * dt;
    y_ += vy_ * dt;

    auto blocked = [&](float x, float y) {
        if (x < R || x > gs::SCREEN_W - R || y > gs::SCREEN_H - R) return true;
        if (y >= CLIFF) return false;
        if (x > SL + R * 0.4f && x < SR - R * 0.4f && y > BACK + R * 0.5f) return false;
        return true;
    };
    if (blocked(x_, y_)) {
        x_ -= vx_ * dt;
        y_ -= vy_ * dt;
        vx_ *= -0.25f;
        vy_ *= -0.25f;
        if (blocked(x_, y_)) {
            if (x_ < SL) x_ = std::max(x_, R);
            if (y_ < CLIFF && x_ <= SL) x_ = std::min(x_, SL - 2.f);
        }
        // Nudge back into open water if a corner ate the step.
        for (int k = 0; k < 8 && blocked(x_, y_); k++) {
            if (y_ < CLIFF && x_ > SL && x_ < SR) y_ += 1.f;
            else if (y_ < CLIFF) y_ = CLIFF + R;
            else x_ = std::clamp(x_, R, float(gs::SCREEN_W) - R);
        }
    }
}

void Game::update(float dt) {
    t_ += dt;
    if (mode_ != Mode::Run) return;
    tide_ -= dt;
    float steer = 0, thrust = 0;
    if (bot_) {
        botInput(steer, thrust);
    } else {
        const gs::Pad& p = sys_->pad;
        steer = (p.down(gs::BTN_RIGHT) ? 1.f : 0.f) - (p.down(gs::BTN_LEFT) ? 1.f : 0.f);
        if (std::fabs(p.axisX) > 0.25f) steer = std::clamp(p.axisX, -1.f, 1.f);
        if (p.down(gs::BTN_UP) || p.down(gs::BTN_A) || p.accel > 0.2f) thrust = 1.f;
        if (p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.brake > 0.2f) thrust = -1.f;
        if (p.axisY > 0.4f && thrust == 0) thrust = 1.f;
        if (p.axisY < -0.4f && thrust == 0) thrust = -1.f;
    }
    physics(dt, steer, thrust);

    float spd = std::sqrt(vx_ * vx_ + vy_ * vy_);
    if (inSlip() && spd < 16.f) held_ += dt;
    else held_ = 0;
    if (held_ > 0.28f) {
        mode_ = Mode::Win;
        won_ = true;
        over_ = true;
        chime_ = 0;
    } else if (tide_ <= 0) {
        tide_ = 0;
        mode_ = Mode::Lose;
        won_ = false;
        over_ = true;
    }

    float hum = std::sqrt(vx_ * vx_ + vy_ * vy_);
    if (thrust > 0.2f) sys_->apu.tone(0, 55.f + hum * 1.4f, 0.05f);
    else sys_->apu.tone(0, 0, 0);
}

void Game::blit(const gs::Image& img, float x, float y, int pal, bool shadow) {
    gs::Sprite s;
    s.img = img;
    s.x = int16_t(x);
    s.y = int16_t(y);
    s.w = img.w;
    s.h = img.h;
    s.pal = uint8_t(pal);
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, int pal) {
    float cx = x;
    for (char ch : s) {
        int i = int(static_cast<unsigned char>(ch)) - 32;
        if (i < 0 || i > 95) i = 0;
        blit(art_.glyph[i], cx, y, pal);
        cx += float(art_.gw[i] > 1 ? art_.gw[i] : 4);
    }
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    float age = (TIDE0 - tide_) / TIDE0;
    uint8_t fog = 0;
    if (mode_ == Mode::Run || mode_ == Mode::Lose) fog = uint8_t(std::clamp(int(age * 10.f), 0, 8));
    if (mode_ == Mode::Lose) fog = 11;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        vdp.lineBackdrop[y] = gs::rgb4(1, 3, 7);
        vdp.lineFog[y] = y > CLIFF ? fog : uint8_t(fog / 2);
        vdp.road[y].on = false;
    }

    int frame = int(std::lround(h_ / TAU * BOAT_FRAMES));
    frame %= BOAT_FRAMES;
    if (frame < 0) frame += BOAT_FRAMES;

    if (mode_ != Mode::Title) {
        float bx = x_ - BOAT_PX * 0.5f;
        float by = y_ - BOAT_PX * 0.5f;
        blit(art_.boat[frame], bx + 2, by + 3, PAL_BOAT, true);
        float c = std::cos(h_), s = std::sin(h_);
        float spd = std::sqrt(vx_ * vx_ + vy_ * vy_);
        if (spd > 8.f) {
            for (int i = 1; i <= 3; i++) {
                float wx = x_ - c * (12.f + i * 7.f) - 5.f;
                float wy = y_ - s * (12.f + i * 7.f) - 3.f;
                blit(art_.wake, wx, wy, PAL_WAKE);
            }
        }
        blit(art_.boat[frame], bx, by, PAL_BOAT);
    }

    float gt = t_ * 28.f + gull_ * 40.f;
    blit(art_.gull, 24.f + std::fmod(gt, 280.f), 100.f + std::sin(t_ * 2.f) * 6.f, PAL_TEXT);
    blit(art_.gull, 80.f + std::fmod(gt * 0.7f, 200.f), 140.f + std::cos(t_ * 1.6f) * 5.f, PAL_DIM);

    char line[64];
    if (mode_ == Mode::Title) {
        text("CLIFFSLIP", 104, 78, PAL_TEXT);
        text("BERTH IN THE SLIP", 86, 96, PAL_DIM);
        text("BEFORE THE TIDE TURNS", 74, 108, PAL_WARN);
        text("ARROWS STEER AND DRIVE", 70, 132, PAL_DIM);
        text("START", 136, 156, (int(t_ * 2.f) % 2) ? PAL_GOOD : PAL_TEXT);
    } else {
        std::snprintf(line, sizeof line, "TIDE %04.1f", std::max(0.f, tide_));
        text(line, 8, 6, tide_ < 12.f ? PAL_WARN : PAL_TEXT);
        float spd = std::sqrt(vx_ * vx_ + vy_ * vy_);
        std::snprintf(line, sizeof line, "WAY %.0f", spd);
        text(line, 248, 6, PAL_DIM);
        if (mode_ == Mode::Pause) text("HELD", 140, 104, PAL_TEXT);
        if (mode_ == Mode::Win) {
            text("BERTHED", 124, 96, PAL_GOOD);
            text("THE SLIP HOLDS", 100, 110, PAL_TEXT);
        }
        if (mode_ == Mode::Lose) {
            text("TIDE TURNED", 108, 96, PAL_WARN);
            text("THE SLIP IS GONE", 96, 110, PAL_TEXT);
        }
        if (mode_ == Mode::Win || mode_ == Mode::Lose) text("START", 136, 132, PAL_DIM);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& p = sys.pad;
    bool start = p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A);
    if (mode_ == Mode::Title) {
        t_ += DT;
        if (bot_ && t_ > 0.35f) resetRun();
        else if (start) resetRun();
    } else if (mode_ == Mode::Pause) {
        if (p.pressed(gs::BTN_START)) mode_ = Mode::Run;
    } else if (mode_ == Mode::Run) {
        if (!bot_ && p.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else update(DT);
    } else if (mode_ == Mode::Win || mode_ == Mode::Lose) {
        t_ += DT;
        chime_ += DT;
        if (mode_ == Mode::Win && chime_ < 0.45f) {
            if (chime_ < DT * 1.5f) {
                gs::FMPatch patch;
                patch.alg = 4;
                patch.vol = 0.2f;
                patch.op[0].mul = 1;
                patch.op[0].level = 1;
                patch.op[0].ar = 0.01f;
                patch.op[0].dr = 0.4f;
                patch.op[0].sl = 0.2f;
                patch.op[0].rr = 0.6f;
                sys.apu.setPatch(0, patch);
                sys.apu.keyOn(0, 523.25f, 0.22f);
            } else if (chime_ > 0.22f && chime_ < 0.22f + DT * 1.5f) {
                sys.apu.keyOn(0, 659.25f, 0.2f);
            }
        }
        if (!bot_ && start && chime_ > 0.4f) {
            mode_ = Mode::Title;
            t_ = 0;
            over_ = false;
        }
    }
    if (mode_ == Mode::Run && bot_) {
        // update already consumed this frame's motion
    } else if (mode_ != Mode::Run) {
        sys.apu.tone(0, 0, 0);
    }
    draw();
}

}  // namespace slip

#include "game/lock.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace buslock {
namespace {

constexpr float LOW = 176.f;
constexpr float HIGH = 116.f;
constexpr float BUS_L = 92.f;
constexpr float BUS_H = 44.f;
constexpr float GATE_W = 16.f;
constexpr float LOWER_X = 268.f;
constexpr float UPPER_X = 668.f;
constexpr float EXIT_X = 860.f;
constexpr float LIFT = 96.f;
constexpr float CREW_SECONDS = 26.f;
constexpr float HOLD_X = 430.f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }
float lerpf(float a, float b, float t) { return a + (b - a) * t; }

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.75f);
    sys.apu.setEcho(0.1f, 0.18f, 0.1f);
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    why_ = "";
    busX_ = 48.f;
    water_ = 0;
    lower_ = 0.2f;
    upper_ = 0;
    crewLeft_ = CREW_SECONDS;
    if (bot_) begin();
}

void Game::begin() {
    mode_ = Mode::Run;
    phase_ = Phase::Approach;
    over_ = false;
    won_ = false;
    why_ = "";
    time_ = 0;
    crewLeft_ = CREW_SECONDS;
    busX_ = 36.f;
    speed_ = 0;
    water_ = 0;
    lower_ = 0.12f;
    upper_ = 0;
    cam_ = 0;
    still_ = 0;
    wheel_ = 0;
    shake_ = 0;
    beep_ = 0;
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(1, 0, 0);
}

float Game::deckAt(float x) const {
    if (x < LOWER_X) return LOW;
    if (x > UPPER_X + GATE_W) return HIGH;
    return lerpf(LOW, HIGH, water_);
}

bool Game::hitGate(float gx, float leafBottom, float deck) const {
    if (busX_ + BUS_L < gx || busX_ > gx + GATE_W) return false;
    float roof = deck - BUS_H;
    return roof < leafBottom - 0.5f;
}

void Game::failRun(const char* why) {
    if (mode_ != Mode::Run) return;
    why_ = why;
    won_ = false;
    over_ = true;
    mode_ = Mode::Fail;
    speed_ = 0;
    shake_ = 14;
    sys_->apu.noiseBurst(0.5f, 640.f, 0.3f);
    sys_->apu.tone(0, 0, 0);
}

void Game::succeed() {
    if (mode_ != Mode::Run) return;
    why_ = "passed";
    won_ = true;
    over_ = true;
    mode_ = Mode::Win;
    speed_ = 0;
    sys_->apu.tone(1, 523.f, 0.08f);
    beep_ = 28;
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (won_ || mode_ == Mode::Win) return 4;
    if (phase_ == Phase::Leave || phase_ == Phase::Open) return 3;
    if (phase_ == Phase::Settle || phase_ == Phase::Shut || phase_ == Phase::Fill) return 2;
    return 1;
}

void Game::blip(float freq, float vol) {
    if (beep_ > 0) return;
    sys_->apu.tone(0, freq, vol);
    beep_ = 5;
}

void Game::pilot() {
    float accel = 0;
    if (bot_) {
        bool pastLower = busX_ > LOWER_X + GATE_W + 24.f;
        bool gateReady = lower_ > 0.72f;
        if (!pastLower) {
            if (!gateReady) {
                float hold = LOWER_X - BUS_L - 18.f;
                if (busX_ > hold) accel = -1.f;
                else accel = speed_ < 0.4f ? 0.2f : 0.f;
            } else {
                accel = speed_ < 2.15f ? 1.f : 0.f;
            }
        } else if (phase_ != Phase::Leave) {
            float err = HOLD_X - busX_;
            if (err > 18.f) accel = speed_ < 1.5f ? 1.f : 0.f;
            else if (err < -10.f) accel = -1.f;
            else accel = -std::copysign(0.6f, speed_);
            if (std::fabs(speed_) < 0.2f && std::fabs(err) < 22.f) accel = 0;
        } else if (upper_ > 0.78f) {
            accel = speed_ < 2.4f ? 1.f : 0.f;
        } else {
            accel = -1.f;
        }
    } else {
        const gs::Pad& p = sys_->pad;
        if (p.down(gs::BTN_RIGHT) || p.down(gs::BTN_A) || p.axisX > 0.35f) accel += 1.f;
        if (p.down(gs::BTN_LEFT) || p.down(gs::BTN_B) || p.axisX < -0.35f) accel -= 1.f;
    }
    speed_ += accel * 0.075f;
    if (accel == 0.f) speed_ *= 0.94f;
    speed_ = clampf(speed_, -1.5f, 2.7f);
    if (std::fabs(speed_) < 0.035f) speed_ = 0;
}

void Game::logic() {
    time_ += 1.f / 60.f;
    crewLeft_ = std::max(0.f, CREW_SECONDS - time_);
    if (crewLeft_ <= 0.f) {
        failRun("the other crew took the lock");
        return;
    }
    pilot();
    busX_ += speed_;
    if (busX_ < 8.f) {
        busX_ = 8.f;
        speed_ = std::max(0.f, speed_);
    }
    wheel_ = (wheel_ + int(std::fabs(speed_) * 3.f)) & 1023;

    float mid = busX_ + BUS_L * 0.5f;
    float deck = deckAt(mid);
    float lowerLeaf = lerpf(LOW - 2.f, LOW - LIFT, lower_);
    float upperDeck = lerpf(LOW, HIGH, water_);
    float upperLeaf = lerpf(upperDeck - 2.f, upperDeck - LIFT, upper_);

    if (phase_ == Phase::Approach) {
        lower_ = std::min(1.f, lower_ + 0.0042f);
        lowerLeaf = lerpf(LOW - 2.f, LOW - LIFT, lower_);
        if (hitGate(LOWER_X, lowerLeaf, deck)) {
            failRun("scraped the lower gate");
            return;
        }
        if (busX_ > LOWER_X + GATE_W + 30.f) phase_ = Phase::Settle;
    } else if (phase_ == Phase::Settle) {
        bool inside = busX_ > LOWER_X + GATE_W + 6.f && busX_ + BUS_L < UPPER_X - 8.f;
        if (inside && std::fabs(speed_) < 0.2f) still_++;
        else still_ = 0;
        if (hitGate(UPPER_X, upperLeaf, deck)) {
            failRun("scraped the upper gate");
            return;
        }
        if (hitGate(LOWER_X, lowerLeaf, deck)) {
            failRun("scraped the lower gate");
            return;
        }
        if (still_ > 28) phase_ = Phase::Shut;
    } else if (phase_ == Phase::Shut) {
        lower_ = std::max(0.f, lower_ - 0.022f);
        lowerLeaf = lerpf(LOW - 2.f, LOW - LIFT, lower_);
        if (hitGate(LOWER_X, lowerLeaf, deck)) {
            failRun("scraped the lower gate");
            return;
        }
        if (hitGate(UPPER_X, upperLeaf, deck)) {
            failRun("scraped the upper gate");
            return;
        }
        if (lower_ <= 0.f) phase_ = Phase::Fill;
    } else if (phase_ == Phase::Fill) {
        water_ = std::min(1.f, water_ + 0.0075f);
        deck = deckAt(mid);
        upperDeck = lerpf(LOW, HIGH, water_);
        upperLeaf = lerpf(upperDeck - 2.f, upperDeck - LIFT, upper_);
        if (hitGate(LOWER_X, lerpf(LOW - 2.f, LOW - LIFT, lower_), deck)) {
            failRun("scraped the lower gate");
            return;
        }
        if (hitGate(UPPER_X, upperLeaf, deck)) {
            failRun("scraped the upper gate");
            return;
        }
        if (water_ >= 1.f) phase_ = Phase::Open;
    } else if (phase_ == Phase::Open) {
        upper_ = std::min(1.f, upper_ + 0.009f);
        deck = deckAt(mid);
        upperLeaf = lerpf(HIGH - 2.f, HIGH - LIFT, upper_);
        if (hitGate(UPPER_X, upperLeaf, deck)) {
            failRun("scraped the upper gate");
            return;
        }
        if (upper_ >= 1.f) phase_ = Phase::Leave;
    } else if (phase_ == Phase::Leave) {
        upper_ = 1.f;
        upperLeaf = HIGH - LIFT;
        if (hitGate(UPPER_X, upperLeaf, deck)) {
            failRun("scraped the upper gate");
            return;
        }
        if (busX_ > EXIT_X) succeed();
    }

    if (mode_ == Mode::Run && std::fabs(speed_) > 0.5f && (wheel_ % 22) < 3) blip(90.f + speed_ * 18.f, 0.035f);
    if (phase_ == Phase::Fill && (int(time_ * 60.f) % 24) == 0) blip(180.f + water_ * 160.f, 0.03f);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool feet) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 48 || s.x + s.w < -48 || s.y > gs::SCREEN_H + 48 || s.y + s.h < -48) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::text(const char* s, float x, float y, float scale, int pal, int align) {
    if (!s) return;
    const float adv = 6.f * scale;
    float w = float(std::strlen(s)) * adv;
    if (align == 0) x -= w * 0.5f;
    else if (align > 0) x -= w;
    for (size_t i = 0; s[i]; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < 32 || c >= 96) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        if (g.h < 1) continue;
        spr(g, x + float(i) * adv + g.w * scale * 0.5f, y, g.h * scale, pal, false);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    float shake = shake_ > 0 ? ((int(shake_) & 1) ? 2.f : -2.f) : 0.f;
    cam_ = busX_ - 70.f;
    if (cam_ < 0) cam_ = 0;
    if (cam_ > EXIT_X - 180.f) cam_ = EXIT_X - 180.f;

    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < 70) v.lineBackdrop[y] = gs::rgb4(4 + y / 28, 6 + y / 22, 11);
        else if (y < 120) v.lineBackdrop[y] = gs::rgb4(5, 8, 5);
        else v.lineBackdrop[y] = gs::rgb4(3, 6, 4);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }

    auto sx = [&](float wx) { return wx - cam_ + shake; };

    const char* label = "WAIT FOR THE LOWER GATE";
    if (phase_ == Phase::Settle) label = "STOP IN THE CHAMBER";
    else if (phase_ == Phase::Shut) label = "LOWER GATE CLOSING";
    else if (phase_ == Phase::Fill) label = "LOCK IS FILLING";
    else if (phase_ == Phase::Open) label = "UPPER GATE OPENING";
    else if (phase_ == Phase::Leave) label = "TAKE THE BUS OUT";

    char crew[48];
    std::snprintf(crew, sizeof(crew), "OTHER CREW  %.1f", crewLeft_);

    if (mode_ == Mode::Title) {
        text("S3 BUS LOCK", 160, 40, 3.f, PAL_HUD, 0);
        text("PASS THE LOCK", 160, 78, 2.f, PAL_HUD, 0);
        text("DO NOT SCRAPE A GATE", 160, 104, 1.7f, PAL_HUD, 0);
        text("THE CLOCK IS THE OTHER CREW", 160, 126, 1.5f, PAL_HUD, 0);
        text("A THROTTLE   B BRAKE", 160, 158, 1.5f, PAL_HUD, 0);
        text("PRESS START", 160, 186, 2.f, PAL_HUD, 0);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160, 96, 3.f, PAL_HUD, 0);
        text(label, 160, 132, 1.5f, PAL_HUD, 0);
    } else if (mode_ == Mode::Fail) {
        text("LOCK LOST", 160, 48, 2.6f, PAL_ALERT, 0);
        text(why_, 160, 86, 1.5f, PAL_HUD, 0);
        text("PRESS START", 160, 130, 2.f, PAL_HUD, 0);
    } else if (mode_ == Mode::Win) {
        text("PASSED THE LOCK", 160, 40, 2.2f, PAL_HUD, 0);
        text("NO GATE WAS SCRAPED", 160, 74, 1.5f, PAL_HUD, 0);
        char buf[64];
        std::snprintf(buf, sizeof(buf), "CREW STILL HAD %.1f S", crewLeft_);
        text(buf, 160, 108, 1.6f, PAL_HUD, 0);
    } else {
        text(label, 8, 10, 1.4f, PAL_HUD, -1);
        text(crew, 312, 10, 1.4f, PAL_CREW, 1);
        text("DO NOT SCRAPE", 8, 206, 1.3f, PAL_HUD, -1);
        float mark = 40.f + (1.f - crewLeft_ / CREW_SECONDS) * 240.f;
        spr(art_.crew, mark, 36, 16, PAL_CREW, false);
    }

    float deck = deckAt(busX_ + BUS_L * 0.5f);
    float feet = deck + 6.f;
    spr(art_.bus, sx(busX_ + BUS_L * 0.5f), feet, 52, PAL_BUS, true);

    auto drawGate = [&](float gx, float leafBottom) {
        float h = std::max(8.f, leafBottom - 28.f);
        spr(art_.gate, sx(gx + GATE_W * 0.5f), 28.f + h * 0.5f, h, mode_ == Mode::Fail ? PAL_ALERT : PAL_GATE, false);
    };
    float lowerLeaf = lerpf(LOW - 2.f, LOW - LIFT, lower_);
    float upperDeck = lerpf(LOW, HIGH, water_);
    if (phase_ == Phase::Open || phase_ == Phase::Leave) upperDeck = HIGH;
    float upperLeaf = lerpf(upperDeck - 2.f, upperDeck - LIFT, upper_);
    drawGate(LOWER_X, lowerLeaf);
    drawGate(UPPER_X, upperLeaf);

    for (float wx = std::floor(cam_ / 36.f) * 36.f; wx < cam_ + 380.f; wx += 36.f) {
        float wy = LOW + 12.f;
        if (wx > LOWER_X && wx < UPPER_X + GATE_W) wy = lerpf(LOW + 12.f, HIGH + 12.f, water_);
        else if (wx >= UPPER_X) wy = HIGH + 12.f;
        spr(art_.water, sx(wx), wy, 18, PAL_WATER, false);
    }
    for (float wx = 200.f; wx < 820.f; wx += 32.f) {
        float top = 150.f;
        if (wx > LOWER_X && wx < UPPER_X) top = lerpf(150.f, 96.f, water_);
        else if (wx >= UPPER_X) top = 96.f;
        spr(art_.stone, sx(wx), top, 24, PAL_STONE, true);
        spr(art_.stone, sx(wx), top + 22, 24, PAL_STONE, true);
    }
    for (int i = 0; i < 5; i++) spr(art_.lamp, sx(80.f + i * 180.f), 108, 40, PAL_BANK, true);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (beep_ > 0 && --beep_ == 0) sys.apu.tone(0, 0, 0);
    if (shake_ > 0) shake_--;

    const gs::Pad& p = sys.pad;
    bool start = p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A);
    if (mode_ == Mode::Title) {
        if (start) begin();
    } else if (mode_ == Mode::Pause) {
        if (p.pressed(gs::BTN_START)) mode_ = Mode::Run;
    } else if (mode_ == Mode::Fail || mode_ == Mode::Win) {
        if (!bot_ && start) begin();
    } else if (mode_ == Mode::Run) {
        if (!bot_ && p.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else logic();
    }
    draw();
}

}  // namespace buslock

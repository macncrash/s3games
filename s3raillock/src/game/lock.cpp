#include "game/lock.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace raillock {
namespace {

constexpr float LOW = 184.f;
constexpr float HIGH = 120.f;
constexpr float CAR_W = 72.f;
constexpr float TALL = 54.f;
constexpr float SHORT = 32.f;
constexpr float GATE_W = 16.f;
constexpr float LOWER_X = 268.f;
constexpr float UPPER_X = 780.f;
constexpr float EXIT_X = 960.f;
constexpr float OPEN_LIP = 86.f;

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
    carX_ = 48.f;
    water_ = 0;
    lower_ = 0.42f;
    upper_ = 0;
    if (bot_) begin();
}

void Game::begin() {
    mode_ = Mode::Run;
    phase_ = Phase::Approach;
    over_ = false;
    won_ = false;
    why_ = "";
    time_ = 0;
    carX_ = 24.f;
    speed_ = 0;
    water_ = 0;
    lower_ = 0.22f;
    upper_ = 0;
    cam_ = 0;
    still_ = 0;
    roll_ = 0;
    panto_ = true;
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

bool Game::hitGate(float gx, float bottom, float deck) const {
    if (carX_ + CAR_W < gx || carX_ > gx + GATE_W) return false;
    float head = deck - (panto_ ? SHORT : TALL);
    return head < bottom - 0.5f;
}

void Game::scrape(const char* why) {
    if (mode_ != Mode::Run) return;
    why_ = why;
    won_ = false;
    over_ = true;
    mode_ = Mode::Fail;
    speed_ = 0;
    shake_ = 14;
    sys_->apu.noiseBurst(0.5f, 520.f, 0.32f);
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

void Game::toneTick(float freq, float vol) {
    if (beep_ > 0) return;
    sys_->apu.tone(0, freq, vol);
    beep_ = 5;
}

void Game::pilot() {
    float accel = 0;
    panto_ = false;
    if (bot_) {
        bool pastLower = carX_ > LOWER_X + GATE_W + 36.f;
        bool leaving = phase_ == Phase::Leave && upper_ > 0.78f;
        if (!pastLower) {
            panto_ = true;
            accel = speed_ < 1.55f ? 1.f : 0.f;
        } else if (!leaving) {
            if (carX_ < 470.f) accel = speed_ < 1.05f ? 1.f : 0.1f;
            else accel = -1.f;
            if (std::fabs(speed_) < 0.16f && carX_ > 440.f && carX_ + CAR_W < UPPER_X - 24.f) accel = 0;
        } else {
            panto_ = carX_ < UPPER_X + GATE_W + 24.f;
            accel = speed_ < 1.85f ? 1.f : 0.f;
        }
    } else {
        const gs::Pad& p = sys_->pad;
        if (p.down(gs::BTN_DOWN) || p.down(gs::BTN_C)) panto_ = true;
        if (p.down(gs::BTN_RIGHT) || p.down(gs::BTN_A) || p.axisX > 0.35f || p.accel > 0.2f) accel += 1.f;
        if (p.down(gs::BTN_LEFT) || p.down(gs::BTN_B) || p.axisX < -0.35f || p.brake > 0.2f) accel -= 1.f;
    }
    speed_ += accel * 0.055f;
    if (accel == 0.f) speed_ *= 0.965f;
    speed_ = clampf(speed_, -1.1f, 2.4f);
    if (std::fabs(speed_) < 0.035f) speed_ = 0;
}

void Game::logic() {
    time_ += 1.f / 60.f;
    pilot();
    carX_ += speed_;
    if (carX_ < 6.f) {
        carX_ = 6.f;
        speed_ = std::max(0.f, speed_);
    }
    roll_ = (roll_ + int(std::fabs(speed_) * 3.f)) & 1023;

    float deck = deckAt(carX_ + CAR_W * 0.5f);
    float lowerBottom = lerpf(LOW - 1.f, OPEN_LIP, lower_);
    float upperDeck = lerpf(LOW, HIGH, water_);
    float upperBottom = lerpf(upperDeck - 1.f, 44.f, upper_);

    if (phase_ == Phase::Approach) {
        lower_ = std::min(1.f, lower_ + 0.0024f);
        if (hitGate(LOWER_X, lowerBottom, deck)) {
            scrape("scraped the lower gate");
            return;
        }
        if (carX_ > LOWER_X + GATE_W + 48.f) phase_ = Phase::Settle;
    } else if (phase_ == Phase::Settle) {
        bool inside = carX_ > LOWER_X + GATE_W + 12.f && carX_ + CAR_W < UPPER_X - 16.f;
        if (inside && std::fabs(speed_) < 0.18f) still_++;
        else still_ = 0;
        if (hitGate(UPPER_X, upperBottom, deck)) {
            scrape("scraped the upper gate");
            return;
        }
        if (still_ > 40) phase_ = Phase::Shut;
    } else if (phase_ == Phase::Shut) {
        lower_ = std::max(0.f, lower_ - 0.018f);
        if (hitGate(LOWER_X, lowerBottom, deck)) {
            scrape("scraped the lower gate");
            return;
        }
        if (hitGate(UPPER_X, upperBottom, deck)) {
            scrape("scraped the upper gate");
            return;
        }
        if (lower_ <= 0.f) phase_ = Phase::Fill;
    } else if (phase_ == Phase::Fill) {
        water_ = std::min(1.f, water_ + 0.007f);
        deck = deckAt(carX_ + CAR_W * 0.5f);
        if (hitGate(LOWER_X, lerpf(LOW - 1.f, OPEN_LIP, lower_), deck)) {
            scrape("scraped the lower gate");
            return;
        }
        upperBottom = lerpf(deckAt(UPPER_X + 4.f) - 1.f, 44.f, upper_);
        if (hitGate(UPPER_X, upperBottom, deck)) {
            scrape("scraped the upper gate");
            return;
        }
        if (water_ >= 1.f) phase_ = Phase::Open;
    } else if (phase_ == Phase::Open) {
        upper_ = std::min(1.f, upper_ + 0.0075f);
        deck = deckAt(carX_ + CAR_W * 0.5f);
        upperBottom = lerpf(HIGH - 1.f, 44.f, upper_);
        if (hitGate(UPPER_X, upperBottom, deck)) {
            scrape("scraped the upper gate");
            return;
        }
        if (upper_ >= 1.f) phase_ = Phase::Leave;
    } else if (phase_ == Phase::Leave) {
        upper_ = 1.f;
        if (hitGate(UPPER_X, 44.f, deck)) {
            scrape("scraped the upper gate");
            return;
        }
        if (carX_ > EXIT_X) succeed();
    }

    if (mode_ == Mode::Run && std::fabs(speed_) > 0.35f && (roll_ % 16) < 2) toneTick(90.f + speed_ * 28.f, 0.045f);
    if (phase_ == Phase::Fill && ((int(time_ * 60.f) % 18) == 0)) toneTick(180.f + water_ * 160.f, 0.03f);
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
    cam_ = carX_ - 64.f;
    if (cam_ < 0) cam_ = 0;

    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < 70) v.lineBackdrop[y] = gs::rgb4(3 + y / 28, 4 + y / 36, 8);
        else if (y < 100) v.lineBackdrop[y] = gs::rgb4(5, 7, 4);
        else v.lineBackdrop[y] = gs::rgb4(3, 5, 3);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }

    auto sx = [&](float wx) { return wx - cam_ + shake; };

    for (int i = 0; i < 7; i++) spr(art_.tree, sx(30.f + i * 160.f), 102, 54, PAL_TREE, true);

    for (float wx = 160.f; wx < 840.f; wx += 34.f) {
        float top = 150.f;
        if (wx > LOWER_X && wx < UPPER_X) top = lerpf(150.f, 98.f, water_);
        else if (wx >= UPPER_X) top = 98.f;
        spr(art_.stone, sx(wx), top, 26, PAL_STONE, true);
        spr(art_.stone, sx(wx), top + 24, 26, PAL_STONE, true);
    }

    for (float wx = cam_ - 10.f; wx < cam_ + 360.f; wx += 38.f) {
        float wy = LOW + 12.f;
        if (wx > LOWER_X && wx < UPPER_X + GATE_W) wy = lerpf(LOW + 12.f, HIGH + 12.f, water_);
        else if (wx >= UPPER_X) wy = HIGH + 12.f;
        spr(art_.water, sx(wx), wy, 18, PAL_WATER, false);
    }

    for (float wx = cam_ - 8.f; wx < cam_ + 350.f; wx += 22.f) {
        spr(art_.sleeper, sx(wx), deckAt(wx) + 6.f, 10, PAL_RAIL, true);
    }

    auto drawGate = [&](float gx, float bottom) {
        float h = bottom - 30.f;
        if (h < 8.f) h = 8.f;
        spr(art_.gate, sx(gx + GATE_W * 0.5f), 30.f + h * 0.5f, h, mode_ == Mode::Fail ? PAL_ALERT : PAL_GATE, false);
    };
    float lowerBottom = lerpf(LOW - 1.f, OPEN_LIP, lower_);
    float upperDeck = (phase_ == Phase::Open || phase_ == Phase::Leave || water_ >= 1.f) ? HIGH : lerpf(LOW, HIGH, water_);
    float upperBottom = lerpf(upperDeck - 1.f, 44.f, upper_);
    drawGate(LOWER_X, lowerBottom);
    drawGate(UPPER_X, upperBottom);

    float feet = deckAt(carX_ + CAR_W * 0.5f) + 4.f;
    const gs::Mipped& body = panto_ ? art_.low : art_.car;
    spr(body, sx(carX_ + CAR_W * 0.5f), feet, panto_ ? 42.f : 56.f, PAL_CAR, true);

    const char* label = "APPROACH THE LOCK";
    if (phase_ == Phase::Settle) label = "STOP ON THE RAIL";
    else if (phase_ == Phase::Shut) label = "LOWER GATE CLOSING";
    else if (phase_ == Phase::Fill) label = "LOCK FILLING";
    else if (phase_ == Phase::Open) label = "UPPER GATE OPENING";
    else if (phase_ == Phase::Leave) label = "ROLL OUT";

    if (mode_ == Mode::Title) {
        text("S3 RAIL LOCK", 160, 42, 3.f, PAL_HUD, 0);
        text("PASS THE LOCK", 160, 80, 2.f, PAL_HUD, 0);
        text("DO NOT SCRAPE A GATE", 160, 104, 2.f, PAL_HUD, 0);
        text("A ROLL  B BRAKE  DOWN PANTOGRAPH", 160, 146, 1.4f, PAL_HUD, 0);
        text("PRESS START", 160, 176, 2.f, PAL_HUD, 0);
    } else if (mode_ == Mode::Pause) {
        text(label, 160, 16, 1.5f, PAL_HUD, 0);
        text("PAUSED", 160, 100, 3.f, PAL_HUD, 0);
    } else if (mode_ == Mode::Fail) {
        text("SCRAPED A GATE", 160, 40, 2.4f, PAL_ALERT, 0);
        text(why_, 160, 72, 1.5f, PAL_HUD, 0);
        text("PRESS START", 160, 120, 2.f, PAL_HUD, 0);
    } else if (mode_ == Mode::Win) {
        text("PASSED THE LOCK", 160, 36, 2.2f, PAL_HUD, 0);
        text("NO GATE WAS SCRAPED", 160, 68, 1.5f, PAL_HUD, 0);
        char buf[64];
        std::snprintf(buf, sizeof(buf), "CLEAN  %.1f S", time_);
        text(buf, 160, 100, 2.f, PAL_HUD, 0);
    } else {
        text(label, 8, 12, 1.4f, PAL_HUD, -1);
        text(panto_ ? "PANTO DOWN" : "PANTO UP", 312, 12, 1.3f, PAL_HUD, 1);
        text("DO NOT SCRAPE", 8, 208, 1.3f, PAL_HUD, -1);
    }
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

}  // namespace raillock

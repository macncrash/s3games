#include "game/lock.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace bikelock {
namespace {

constexpr float LOW = 176.f;
constexpr float HIGH = 112.f;
constexpr float BIKE_W = 36.f;
constexpr float STAND_H = 44.f;
constexpr float DUCK_H = 28.f;
constexpr float GATE_W = 18.f;
constexpr float LOWER_X = 228.f;
constexpr float UPPER_X = 700.f;
constexpr float EXIT_X = 860.f;
constexpr float OPEN_LIP = 92.f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }
float lerpf(float a, float b, float t) { return a + (b - a) * t; }

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.8f);
    sys.apu.setEcho(0.12f, 0.2f, 0.12f);
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    why_ = "";
    bikeX_ = 70.f;
    water_ = 0;
    lower_ = 0.55f;
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
    bikeX_ = 28.f;
    speed_ = 0;
    water_ = 0;
    lower_ = 0.18f;
    upper_ = 0;
    cam_ = 0;
    still_ = 0;
    wheel_ = 0;
    duck_ = false;
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
    if (bikeX_ + BIKE_W < gx || bikeX_ > gx + GATE_W) return false;
    float head = deck - (duck_ ? DUCK_H : STAND_H);
    return head < bottom - 0.5f;
}

void Game::scrape(const char* why) {
    if (mode_ != Mode::Run) return;
    why_ = why;
    won_ = false;
    over_ = true;
    mode_ = Mode::Fail;
    speed_ = 0;
    shake_ = 12;
    sys_->apu.noiseBurst(0.55f, 700.f, 0.28f);
    sys_->apu.tone(0, 0, 0);
}

void Game::succeed() {
    if (mode_ != Mode::Run) return;
    why_ = "passed";
    won_ = true;
    over_ = true;
    mode_ = Mode::Win;
    speed_ = 0;
    sys_->apu.tone(1, 660.f, 0.09f);
    beep_ = 24;
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
    beep_ = 4;
}

void Game::pilot() {
    float accel = 0;
    duck_ = false;
    if (bot_) {
        bool pastLower = bikeX_ > LOWER_X + GATE_W + 28.f;
        bool leaving = phase_ == Phase::Leave && upper_ > 0.76f;
        if (!pastLower) {
            duck_ = true;
            accel = speed_ < 2.0f ? 1.f : 0.f;
        } else if (!leaving) {
            if (bikeX_ < 420.f) accel = speed_ < 1.35f ? 1.f : 0.15f;
            else accel = -1.f;
            if (std::fabs(speed_) < 0.18f && bikeX_ > 390.f) accel = 0;
        } else {
            duck_ = bikeX_ < UPPER_X + GATE_W + 16.f;
            accel = speed_ < 2.25f ? 1.f : 0.f;
        }
    } else {
        const gs::Pad& p = sys_->pad;
        if (p.down(gs::BTN_DOWN) || p.down(gs::BTN_C)) duck_ = true;
        if (p.down(gs::BTN_RIGHT) || p.down(gs::BTN_A) || p.axisX > 0.35f) accel += 1.f;
        if (p.down(gs::BTN_LEFT) || p.down(gs::BTN_B) || p.axisX < -0.35f) accel -= 1.f;
    }
    speed_ += accel * 0.09f;
    if (accel == 0.f) speed_ *= 0.96f;
    speed_ = clampf(speed_, -1.4f, 3.15f);
    if (std::fabs(speed_) < 0.04f) speed_ = 0;
}

void Game::logic() {
    time_ += 1.f / 60.f;
    pilot();
    bikeX_ += speed_;
    if (bikeX_ < 8.f) {
        bikeX_ = 8.f;
        speed_ = std::max(0.f, speed_);
    }
    wheel_ = (wheel_ + int(std::fabs(speed_) * 2.f)) & 1023;

    float deck = deckAt(bikeX_ + BIKE_W * 0.5f);
    float lowerBottom = lerpf(LOW - 1.f, OPEN_LIP, lower_);
    float upperDeck = lerpf(LOW, HIGH, water_);
    float upperBottom = lerpf(upperDeck - 1.f, 46.f, upper_);

    if (phase_ == Phase::Approach) {
        lower_ = std::min(1.f, lower_ + 0.00215f);
        if (hitGate(LOWER_X, lowerBottom, deck)) {
            scrape("scraped the lower gate");
            return;
        }
        if (bikeX_ > LOWER_X + GATE_W + 36.f) phase_ = Phase::Settle;
    } else if (phase_ == Phase::Settle) {
        bool inside = bikeX_ > LOWER_X + GATE_W + 8.f && bikeX_ + BIKE_W < UPPER_X - 8.f;
        if (inside && std::fabs(speed_) < 0.22f) still_++;
        else still_ = 0;
        if (hitGate(UPPER_X, upperBottom, deck)) {
            scrape("scraped the upper gate");
            return;
        }
        if (still_ > 36) phase_ = Phase::Shut;
    } else if (phase_ == Phase::Shut) {
        lower_ = std::max(0.f, lower_ - 0.02f);
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
        water_ = std::min(1.f, water_ + 0.0065f);
        if (hitGate(LOWER_X, lerpf(LOW - 1.f, OPEN_LIP, lower_), deckAt(bikeX_ + BIKE_W * 0.5f))) {
            scrape("scraped the lower gate");
            return;
        }
        if (hitGate(UPPER_X, upperBottom, deck)) {
            scrape("scraped the upper gate");
            return;
        }
        if (water_ >= 1.f) phase_ = Phase::Open;
    } else if (phase_ == Phase::Open) {
        upper_ = std::min(1.f, upper_ + 0.008f);
        deck = deckAt(bikeX_ + BIKE_W * 0.5f);
        upperBottom = lerpf(HIGH - 1.f, 46.f, upper_);
        if (hitGate(UPPER_X, upperBottom, deck)) {
            scrape("scraped the upper gate");
            return;
        }
        if (upper_ >= 1.f) phase_ = Phase::Leave;
    } else if (phase_ == Phase::Leave) {
        upper_ = 1.f;
        if (hitGate(UPPER_X, 46.f, deck)) {
            scrape("scraped the upper gate");
            return;
        }
        if (bikeX_ > EXIT_X) succeed();
    }

    if (mode_ == Mode::Run && std::fabs(speed_) > 0.4f && (int(wheel_) % 18) < 3) toneTick(140.f + speed_ * 30.f, 0.04f);
    if (phase_ == Phase::Fill && ((int(time_ * 60.f) % 20) == 0)) toneTick(220.f + water_ * 180.f, 0.035f);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool feet) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 40 || s.x + s.w < -40 || s.y > gs::SCREEN_H + 40 || s.y + s.h < -40) return;
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
    cam_ = bikeX_ - 78.f;
    if (cam_ < 0) cam_ = 0;

    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < 78) v.lineBackdrop[y] = gs::rgb4(5 + y / 40, 8 + y / 30, 13);
        else if (y < 108) v.lineBackdrop[y] = gs::rgb4(6, 10, 6);
        else v.lineBackdrop[y] = gs::rgb4(4, 7, 5);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }

    auto sx = [&](float wx) { return wx - cam_ + shake; };

    for (int i = 0; i < 6; i++) {
        float tx = 40.f + i * 150.f;
        spr(art_.tree, sx(tx), 108, 58, PAL_TREE, true);
    }

    float deck = deckAt(bikeX_ + BIKE_W * 0.5f);
    // Masonry along the lock, from the coping down into the water.
    for (float wx = 180.f; wx < 760.f; wx += 32.f) {
        float top = (wx > LOWER_X && wx < UPPER_X) ? lerpf(148.f, 96.f, (wx > LOWER_X && wx < UPPER_X) ? water_ : 0.f) : 148.f;
        if (wx >= UPPER_X) top = 96.f;
        if (wx < LOWER_X + 8.f) top = 148.f;
        spr(art_.stone, sx(wx), top, 28, PAL_STONE, true);
        spr(art_.stone, sx(wx), top + 26, 28, PAL_STONE, true);
    }

    float waterY = lerpf(LOW + 8.f, HIGH + 8.f, (bikeX_ > LOWER_X && bikeX_ < UPPER_X) ? water_ : (bikeX_ >= UPPER_X ? 1.f : 0.f));
    for (float wx = cam_; wx < cam_ + 360.f; wx += 36.f) {
        float wy = LOW + 10.f;
        if (wx > LOWER_X && wx < UPPER_X + GATE_W) wy = lerpf(LOW + 10.f, HIGH + 10.f, water_);
        else if (wx >= UPPER_X) wy = HIGH + 10.f;
        spr(art_.water, sx(wx), wy, 22, PAL_WATER, false);
    }
    (void)waterY;

    auto drawGate = [&](float gx, float bottom, float deckY) {
        float h = bottom - 36.f;
        if (h < 6.f) h = 6.f;
        float cy = 36.f + h * 0.5f;
        (void)deckY;
        spr(art_.gate, sx(gx + GATE_W * 0.5f), cy, h, mode_ == Mode::Fail ? PAL_ALERT : PAL_GATE, false);
    };
    float lowerBottom = lerpf(LOW - 1.f, OPEN_LIP, lower_);
    float upperDeck = (phase_ == Phase::Open || phase_ == Phase::Leave || water_ >= 1.f) ? HIGH : lerpf(LOW, HIGH, water_);
    float upperBottom = lerpf(upperDeck - 1.f, 46.f, upper_);
    drawGate(LOWER_X, lowerBottom, LOW);
    drawGate(UPPER_X, upperBottom, upperDeck);

    float feet = deck + 8.f;
    const gs::Mipped& rider = duck_ ? art_.duck : art_.bike;
    float bh = duck_ ? 40.f : 52.f;
    spr(rider, sx(bikeX_ + BIKE_W * 0.5f), feet, bh, PAL_BIKE, true);

    const char* label = "APPROACH";
    if (phase_ == Phase::Settle) label = "SETTLE IN THE LOCK";
    else if (phase_ == Phase::Shut) label = "LOWER GATE CLOSING";
    else if (phase_ == Phase::Fill) label = "LOCK FILLING";
    else if (phase_ == Phase::Open) label = "UPPER GATE OPENING";
    else if (phase_ == Phase::Leave) label = "RIDE OUT";

    if (mode_ == Mode::Title) {
        text("S3 BIKE LOCK", 160, 48, 3.f, PAL_HUD, 0);
        text("PASS THE LOCK", 160, 84, 2.f, PAL_HUD, 0);
        text("DO NOT SCRAPE A GATE", 160, 108, 2.f, PAL_HUD, 0);
        text("A PEDAL  B BRAKE  DOWN DUCK", 160, 150, 1.5f, PAL_HUD, 0);
        text("PRESS START", 160, 178, 2.f, PAL_HUD, 0);
    } else if (mode_ == Mode::Pause) {
        text(label, 160, 16, 1.6f, PAL_HUD, 0);
        text("PAUSED", 160, 100, 3.f, PAL_HUD, 0);
    } else if (mode_ == Mode::Fail) {
        text("SCRAPED A GATE", 160, 40, 2.4f, PAL_ALERT, 0);
        text(why_, 160, 72, 1.6f, PAL_HUD, 0);
        text("PRESS START", 160, 120, 2.f, PAL_HUD, 0);
    } else if (mode_ == Mode::Win) {
        text("PASSED THE LOCK", 160, 36, 2.2f, PAL_HUD, 0);
        text("NO GATE WAS SCRAPED", 160, 68, 1.6f, PAL_HUD, 0);
        char buf[64];
        std::snprintf(buf, sizeof(buf), "CLEAN  %.1f S", time_);
        text(buf, 160, 100, 2.f, PAL_HUD, 0);
    } else {
        text(label, 8, 12, 1.5f, PAL_HUD, -1);
        char buf[48];
        std::snprintf(buf, sizeof(buf), duck_ ? "DUCK" : "UP");
        text(buf, 300, 12, 1.5f, PAL_HUD, 1);
        text("DO NOT SCRAPE", 8, 208, 1.4f, PAL_HUD, -1);
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

}  // namespace bikelock

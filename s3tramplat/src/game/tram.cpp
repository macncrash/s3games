#include "game/tram.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace tramplat {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kMark = 52.f;
constexpr float kStop0 = 40.f;
constexpr float kStop1 = 66.f;
constexpr float kNose = 6.4f;
constexpr float kRear = 6.2f;
constexpr float kDeck = 0.34f;
constexpr float kXTol = 0.42f;
constexpr float kHTol = 0.07f;
constexpr float kStill = 0.16f;
constexpr float kHoldNeed = 0.48f;
constexpr float kCrew = 22.5f;
constexpr float kPpm = 7.2f;
constexpr float kRail = 186.f;
constexpr float kBrakeA = 8.6f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = clampf(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (hold_ > 0.05f) return 3;
    if (door_ + kNose > kStop0) return 2;
    return 1;
}

void Game::showTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    why_ = "";
    chime_ = -1;
    time_ = 0;
    door_ = 22.f;
    floor_ = 0.7f;
    speed_ = 0;
    hold_ = 0;
    dwell_ = 0;
    shake_ = 0;
}

void Game::startRun() {
    door_ = 0.f;
    floor_ = 0.78f;
    speed_ = 2.4f;
    time_ = 0;
    power_ = brake_ = kneel_ = 0;
    hold_ = 0;
    dwell_ = 0;
    shake_ = 0;
    won_ = false;
    over_ = false;
    why_ = "";
    chime_ = -1;
    mode_ = Mode::Run;
    blip(392.f);
}

void Game::pilot(float& power, float& brake, float& kneel) const {
    float err = kMark - door_;
    float want;
    if (err > 0.35f) {
        float coast = std::sqrt(std::max(0.f, 2.f * kBrakeA * err));
        want = err > 16.f ? 9.4f : std::min(8.2f, std::max(0.55f, coast * 0.88f));
    } else if (err < -0.22f) {
        want = -0.9f;
    } else {
        want = 0.f;
    }
    power = 0.f;
    brake = 0.f;
    if (speed_ > want + 0.10f) brake = 1.f;
    else if (speed_ < want - 0.12f && want >= 0.f) power = 1.f;
    else if (want < 0.f && speed_ > want + 0.08f) brake = 0.55f;
    kneel = clampf((kDeck - floor_) * 7.f, -1.f, 1.f);
}

void Game::win() {
    if (mode_ != Mode::Run) return;
    won_ = true;
    over_ = true;
    mode_ = Mode::Win;
    why_ = "level";
    speed_ = 0;
    floor_ = kDeck;
    door_ = kMark;
    chime_ = 0;
    chimeT_ = 0;
    sys_->rumble(0.18f, 0.05f, 110);
    sys_->setLight(30, 160, 70);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    won_ = false;
    over_ = true;
    mode_ = Mode::Fail;
    why_ = why;
    shake_ = 0.7f;
    sys_->rumble(0.45f, 0.22f, 150);
    sys_->setLight(170, 28, 20);
    sys_->apu.noiseBurst(0.38f, 420.f, 0.24f);
}

void Game::physics(float power, float brake, float kneel) {
    power_ = clampf(power, 0.f, 1.f);
    brake_ = clampf(brake, 0.f, 1.f);
    kneel_ = clampf(kneel, -1.f, 1.f);
    time_ += kDt;

    float drive = power_ * 4.6f;
    if (brake_ > 0.25f && speed_ > 0.08f) drive = 0.f;
    float back = 0.f;
    if (brake_ > 0.4f && power_ == 0.f && speed_ <= 0.15f && (kMark - door_) < -0.18f) back = -1.8f;
    float a = drive + back - brake_ * (speed_ > 0.12f ? kBrakeA : 3.2f) - 0.22f * speed_;
    if (power_ == 0.f && brake_ == 0.f) a -= 0.45f * speed_;
    speed_ += a * kDt;
    speed_ = clampf(speed_, -2.4f, 11.f);
    if (std::fabs(speed_) < 0.03f && power_ == 0.f && brake_ < 0.15f) speed_ = 0.f;
    door_ += speed_ * kDt;

    floor_ += kneel_ * 0.72f * kDt;
    floor_ = clampf(floor_, 0.12f, 1.15f);
    shake_ = std::max(0.f, shake_ - kDt);

    if (door_ + kNose > kStop1 + 0.05f) {
        fail("ran past the platform");
        return;
    }
    if (time_ >= kCrew) {
        fail("the other crew");
        return;
    }

    bool atMark = std::fabs(door_ - kMark) <= kXTol;
    bool level = std::fabs(floor_ - kDeck) <= kHTol;
    bool stopped = std::fabs(speed_) <= kStill;
    bool on = (door_ - kRear) >= kStop0 - 0.4f && (door_ + kNose) <= kStop1;
    if (on && atMark && level && stopped) {
        hold_ += kDt;
        dwell_ = 0.f;
        if (hold_ >= kHoldNeed) win();
        return;
    }
    hold_ = 0.f;

    bool sitting = std::fabs(speed_) < 0.035f && power_ < 0.05f && std::fabs(kneel_) < 0.08f;
    if (sitting && door_ > 12.f) dwell_ += kDt;
    else dwell_ = 0.f;
    if (dwell_ > 0.62f) {
        if (!on || door_ < kMark - kXTol) fail("short of the platform");
        else fail("not level");
    }
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.06f);
    beep_ = 0.08f;
}

void Game::sky() {
    uint16_t zen = gs::rgb4(3, 5, 9);
    uint16_t hor = gs::rgb4(10, 11, 12);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float t = float(y) / float(gs::SCREEN_H - 1);
        sys_->vdp.lineBackdrop[y] = lerpC(zen, hor, t * t);
        sys_->vdp.lineFog[y] = 0;
        sys_->vdp.road[y].on = false;
    }
}

void Game::spr(const gs::Mipped& m, float cx, float feet, float ht, int pal, bool flip) {
    if (ht < 2.f) return;
    float sc = ht / float(m.h);
    float w = float(m.w) * sc;
    gs::Sprite s;
    s.img = m.pick(ht);
    s.x = int16_t(std::lround(cx - w * 0.5f));
    s.y = int16_t(std::lround(feet - ht));
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(ht));
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    gs::Plane& h = sys_->vdp.HUD;
    int n = int(std::strlen(s));
    for (int i = 0; i < n; i++) {
        int c = int(static_cast<unsigned char>(s[i]));
        if (c < 32 || c > 127) c = 32;
        h.set(col + i, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::draw() {
    sky();
    sys_->vdp.A.clear();
    sys_->vdp.HUD.clear();
    sys_->vdp.clearSprites();
    float cam = (mode_ == Mode::Title) ? 30.f : door_ + 2.f;
    float jx = std::sin(time_ * 28.f) * shake_ * 4.f;
    auto X = [&](float w) { return 168.f + (w - cam) * kPpm + jx; };

    float deckY = kRail - kDeck * 46.f;
    for (int i = 0; i < 5; i++) spr(art_.pole, X(kStop0 - 6.f + i * 8.5f), kRail - 8.f, 52.f, PAL_TOWN);
    spr(art_.shelter, X(kMark + 1.2f), deckY + 2.f, 36.f, PAL_STOP);
    spr(art_.stop, X((kStop0 + kStop1) * 0.5f), kRail + 4.f, 48.f, PAL_STOP);
    spr(art_.stripe, X(kMark), deckY + 4.f, 22.f, PAL_CLOCK);
    spr(art_.clock, X(kStop0 - 2.4f), deckY - 8.f, 22.f, PAL_CLOCK);

    float other = -18.f + time_ * 2.1f;
    spr(art_.rival, X(other), kRail + 10.f, 18.f, PAL_WIRE, true);

    float body = kRail - floor_ * 46.f;
    spr(art_.bogie, X(door_ - 3.6f), kRail, 14.f, PAL_TRAM);
    spr(art_.bogie, X(door_ + 3.8f), kRail, 14.f, PAL_TRAM);
    spr(art_.tram, X(door_), body + 8.f, 42.f, PAL_TRAM);
    float panY = body - 28.f;
    float sway = std::sin(time_ * 3.f + door_) * 2.f;
    spr(art_.pan, X(door_ - 1.5f) + sway, panY, 16.f, PAL_WIRE);

    char buf[64];
    if (mode_ == Mode::Title) {
        hudC(3, "S3 TRAM PLAT", PAL_CREAM);
        hudC(6, "STOP LEVEL WITH THE PLATFORM", PAL_HUD);
        hudC(8, "DOORS ON THE STRIPE", PAL_GOOD);
        hudC(9, "KNEEL THE FLOOR TO THE DECK", PAL_GOOD);
        hudC(11, "THE CLOCK IS THE OTHER CREW", PAL_CREAM);
        hudC(16, "A POWER    B BRAKE", PAL_HUD);
        hudC(17, "UP DOWN  KNEEL THE FLOOR", PAL_HUD);
        hudC(22, "PRESS START", PAL_CREAM);
    } else if (mode_ == Mode::Pause) {
        hudC(10, "PAUSED", PAL_CREAM);
    } else if (mode_ == Mode::Win) {
        hudC(3, "LEVEL", PAL_GOOD);
        hudC(5, "STOPPED LEVEL WITH THE PLATFORM", PAL_GOOD);
        std::snprintf(buf, sizeof buf, "RUN %.1f S", time_);
        hudC(8, buf, PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        hudC(3, "STAND LOST", PAL_BAD);
        hudC(5, why_, PAL_BAD);
    } else {
        std::snprintf(buf, sizeof buf, "SPD %4.1f", speed_);
        hud(1, 1, buf, PAL_HUD);
        float left = std::max(0.f, kCrew - time_);
        std::snprintf(buf, sizeof buf, "CREW %4.1f", left);
        hud(26, 1, buf, left < 6.f ? PAL_BAD : PAL_CREAM);
        float err = kMark - door_;
        if (door_ + kNose < kStop0) {
            std::snprintf(buf, sizeof buf, "PLATFORM  %3.0f M", kStop0 - door_);
            hudC(2, buf, PAL_CREAM);
        } else if (std::fabs(err) <= kXTol && std::fabs(floor_ - kDeck) <= kHTol) {
            hudC(2, "HOLD  LEVEL", PAL_GOOD);
        } else if (std::fabs(err) > kXTol) {
            hudC(2, err > 0.f ? "SHORT OF THE STRIPE" : "PAST THE STRIPE", PAL_BAD);
        } else if (floor_ > kDeck + kHTol) {
            hudC(2, "FLOOR HIGH", PAL_BAD);
        } else {
            hudC(2, "FLOOR LOW", PAL_BAD);
        }
        hudC(26, "A POWER  B BRAKE  UP DOWN KNEEL", PAL_HUD);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.72f);
    sys.apu.setEcho(0.07f, 0.12f, 0.07f);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    if (bot_) startRun();
    else showTitle();
    draw();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;

    if (beep_ > 0.f) {
        beep_ -= kDt;
        if (beep_ <= 0.f) sys.apu.tone(1, 0.f, 0.f);
    }
    if (chime_ >= 0) {
        static const float notes[] = {349.f, 440.f, 523.f, 698.f};
        chimeT_ += kDt;
        if (chimeT_ > 0.14f) {
            if (chime_ < 4) sys.apu.keyOn(0, notes[chime_], 0.18f);
            else sys.apu.keyOff(0);
            chime_++;
            chimeT_ = 0;
            if (chime_ > 8) chime_ = -1;
        }
    }

    if (mode_ == Mode::Title) {
        time_ += kDt;
        door_ = 18.f + std::sin(time_ * 0.55f) * 1.6f;
        floor_ = 0.62f + std::sin(time_ * 0.9f) * 0.08f;
        draw();
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) startRun();
        else if (pad.pressed(gs::BTN_MODE)) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        }
        return;
    }
    if (mode_ == Mode::Pause) {
        draw();
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Run;
        else if (pad.pressed(gs::BTN_MODE)) showTitle();
        return;
    }
    if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        draw();
        sys.apu.noise(0.f, 400.f, false);
        sys.apu.tone(2, 0.f, 0.f);
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
            if (mode_ == Mode::Fail) startRun();
            else showTitle();
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) showTitle();
        return;
    }

    float power = 0, brake = 0, kneel = 0;
    if (bot_) {
        pilot(power, brake, kneel);
    } else {
        if (pad.down(gs::BTN_RIGHT) || pad.down(gs::BTN_A) || pad.down(gs::BTN_C) || pad.accel > 0.15f) power = 1.f;
        if (pad.down(gs::BTN_LEFT) || pad.down(gs::BTN_B) || pad.brake > 0.15f) brake = 1.f;
        if (pad.axisX > 0.25f) power = std::max(power, pad.axisX);
        if (pad.axisX < -0.25f) brake = std::max(brake, -pad.axisX);
        if (pad.down(gs::BTN_UP)) kneel += 1.f;
        if (pad.down(gs::BTN_DOWN)) kneel -= 1.f;
        if (std::fabs(pad.axisY) > 0.25f) kneel = pad.axisY;
        if (pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(240.f);
            draw();
            return;
        }
    }

    physics(power, brake, kneel);
    if (mode_ == Mode::Run) {
        float hum = 90.f + std::fabs(speed_) * 14.f + power_ * 30.f;
        sys.apu.tone(2, hum, 0.025f + power_ * 0.025f);
        sys.apu.noise(brake_ > 0.4f && std::fabs(speed_) > 1.f ? 0.06f : 0.008f, 480.f, false);
        bool near = std::fabs(door_ - kMark) < 2.2f;
        sys.setLight(near ? 36 : 24, near ? 130 : 60, near ? 70 : 140);
    }
    draw();
}

}  // namespace tramplat

#include "game/bus.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace busplat {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kMark = 58.f;     // door on the stripe
constexpr float kPlat0 = 46.f;
constexpr float kPlat1 = 74.f;    // nose must not pass this
constexpr float kNose = 4.6f;     // door to the front bumper
constexpr float kRear = 7.8f;     // door to the back
constexpr float kDeck = 1.18f;    // platform deck
constexpr float kXTol = 0.48f;
constexpr float kHTol = 0.09f;
constexpr float kStop = 0.20f;
constexpr float kHoldNeed = 0.40f;
constexpr float kRival0 = -12.f;
constexpr float kRivalV = 2.15f;
constexpr float kPpm = 7.2f;
constexpr float kGround = 186.f;

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
    if (s_ + kNose > kPlat0) return 2;
    return 1;
}

void Game::showTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    why_ = "";
    chime_ = -1;
    time_ = 0;
    s_ = 22.f;
    h_ = 0.62f;
    v_ = 0;
    rival_ = 8.f;
    hold_ = 0;
    dwell_ = 0;
    shake_ = 0;
}

void Game::startRun() {
    s_ = 0.f;
    h_ = 0.36f;
    v_ = 2.4f;
    rival_ = kRival0;
    time_ = 0;
    gas_ = brake_ = lift_ = 0;
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

void Game::pilot(float& gas, float& brake, float& lift) const {
    float err = kMark - s_;
    float vWant;
    if (err > 0.36f) {
        float vStop = std::sqrt(std::max(0.f, 2.f * 8.4f * err));
        vWant = err > 16.f ? 7.6f : std::min(6.6f, std::max(0.55f, vStop * 0.88f));
    } else if (err < -0.22f) {
        vWant = -0.9f;
    } else {
        vWant = 0.f;
    }
    gas = 0.f;
    brake = 0.f;
    if (v_ > vWant + 0.10f) brake = 1.f;
    else if (v_ < vWant - 0.12f) {
        if (vWant >= 0.f) gas = 1.f;
        else brake = 0.55f;
    }
    float dh = kDeck - h_;
    if (std::fabs(dh) < 0.04f) lift = 0.f;
    else lift = clampf(dh * 6.f, -1.f, 1.f);
}

void Game::win() {
    if (mode_ != Mode::Run) return;
    won_ = true;
    over_ = true;
    mode_ = Mode::Win;
    why_ = "level";
    v_ = 0;
    h_ = kDeck;
    chime_ = 0;
    chimeT_ = 0;
    sys_->rumble(0.2f, 0.06f, 120);
    sys_->setLight(40, 170, 80);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    won_ = false;
    over_ = true;
    mode_ = Mode::Fail;
    why_ = why;
    shake_ = 0.8f;
    sys_->rumble(0.5f, 0.28f, 160);
    sys_->setLight(180, 30, 24);
    sys_->apu.noiseBurst(0.42f, 480.f, 0.26f);
}

void Game::physics(float gas, float brake, float lift) {
    gas_ = clampf(gas, 0.f, 1.f);
    brake_ = clampf(brake, 0.f, 1.f);
    lift_ = clampf(lift, -1.f, 1.f);
    time_ += kDt;

    float drive = gas_ * 5.2f;
    if (brake_ > 0.2f && v_ > 0.05f) drive = 0.f;
    float rev = 0.f;
    if (brake_ > 0.45f && gas_ == 0.f && v_ <= 0.22f && (kMark - s_) < -0.18f) rev = -1.8f;
    float a = drive + rev - brake_ * (v_ > 0.12f ? 9.6f : 3.4f) - 0.22f * v_;
    if (gas_ == 0.f && brake_ == 0.f) a -= 0.55f * v_;
    v_ += a * kDt;
    v_ = clampf(v_, -2.6f, 9.5f);
    if (std::fabs(v_) < 0.03f && gas_ == 0.f && brake_ < 0.2f) v_ = 0.f;
    s_ += v_ * kDt;

    h_ += lift_ * 1.35f * kDt;
    h_ = clampf(h_, 0.16f, 2.2f);

    rival_ += kRivalV * kDt;
    shake_ = std::max(0.f, shake_ - kDt);

    if (s_ + kNose > kPlat1 + 0.05f) {
        fail("missed the platform");
        return;
    }
    if (rival_ >= kMark) {
        fail("the other crew");
        return;
    }

    bool atMark = std::fabs(s_ - kMark) <= kXTol;
    bool level = std::fabs(h_ - kDeck) <= kHTol;
    bool stopped = std::fabs(v_) <= kStop;
    bool on = (s_ - kRear) >= kPlat0 - 0.4f && (s_ + kNose) <= kPlat1;
    if (on && atMark && level && stopped) {
        hold_ += kDt;
        dwell_ = 0.f;
        if (hold_ >= kHoldNeed) win();
        return;
    }
    hold_ = 0.f;

    bool sitting = std::fabs(v_) < 0.04f && gas_ < 0.05f && std::fabs(lift_) < 0.08f;
    if (sitting && s_ > 12.f) dwell_ += kDt;
    else dwell_ = 0.f;
    if (dwell_ > 0.7f) {
        if (!on || s_ < kMark - kXTol) fail("short of the platform");
        else if (s_ > kMark + kXTol) fail("past the stripe");
        else fail("not level");
    }
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.07f);
    beep_ = 0.08f;
}

void Game::sky() {
    uint16_t zen = gs::rgb4(2, 4, 9);
    uint16_t mid = gs::rgb4(6, 8, 11);
    uint16_t hor = gs::rgb4(13, 11, 7);
    uint16_t street = gs::rgb4(4, 4, 5);
    if (mode_ == Mode::Fail) hor = lerpC(hor, gs::rgb4(12, 4, 3), 0.4f);
    if (mode_ == Mode::Win) hor = lerpC(hor, gs::rgb4(7, 12, 7), 0.35f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float t = y / float(gs::SCREEN_H - 1);
        uint16_t c = t < 0.40f ? lerpC(zen, mid, t / 0.40f) : lerpC(mid, hor, (t - 0.40f) / 0.30f);
        if (y > 158) c = lerpC(street, gs::rgb4(3, 3, 4), (y - 158) / 66.f);
        sys_->vdp.lineBackdrop[y] = c;
        sys_->vdp.lineFog[y] = 0;
        sys_->vdp.road[y].on = false;
    }
    sys_->vdp.A.enabled = false;
    sys_->vdp.B.enabled = false;
    sys_->vdp.hudEnabled = true;
    sys_->vdp.HUD.clear();
    sys_->vdp.setFogColor(gs::rgb4(5, 6, 7));
}

void Game::spr(const gs::Mipped& m, float cx, float feet, float ht, int pal, bool flip) {
    if (ht < 1.2f || m.h < 1) return;
    float w = ht * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::lround(clampf(w, 1.f, 400.f)));
    s.h = int16_t(std::lround(clampf(ht, 1.f, 280.f)));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet - s.h));
    s.img = m.pick(ht);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::draw() {
    sky();
    sys_->vdp.clearSprites();
    float cam = (mode_ == Mode::Title) ? 36.f : s_ + 2.f;
    float jx = std::sin(time_ * 30.f) * shake_ * 5.f;
    auto X = [&](float w) { return 160.f + (w - cam) * kPpm + jx; };

    float deckY = kGround - kDeck * 26.f;
    spr(art_.sign, X(kPlat0 - 1.6f), deckY - 8.f, 26.f, PAL_SIGN);
    spr(art_.shelter, X(kMark + 1.4f), deckY + 2.f, 34.f, PAL_TOWN);
    for (int i = 0; i < 5; i++) {
        float wx = kPlat0 + 2.f + i * 5.2f;
        spr(art_.lamp, X(wx), kGround, 44.f, PAL_TOWN);
    }
    spr(art_.plat, X((kPlat0 + kPlat1) * 0.5f), kGround + 8.f, 70.f, PAL_PLAT);
    spr(art_.stripe, X(kMark), deckY + 2.f, 20.f, PAL_SIGN);

    spr(art_.rival, X(rival_), kGround - 1.f, 28.f, PAL_RIVAL);

    float sill = kGround - h_ * 26.f;
    spr(art_.wheel, X(s_ - 5.4f), kGround, 14.f, PAL_BUS);
    spr(art_.wheel, X(s_ + 3.2f), kGround, 14.f, PAL_BUS);
    spr(art_.bus, X(s_ - 1.4f), sill + 8.f, 38.f, PAL_BUS);

    char buf[64];
    if (mode_ == Mode::Title) {
        hudC(3, "S3 BUS PLAT", PAL_AMBER);
        hudC(6, "STOP LEVEL WITH THE PLATFORM", PAL_HUD);
        hudC(8, "DOOR ON THE STRIPE", PAL_GOOD);
        hudC(9, "SILL FLUSH WITH THE DECK", PAL_GOOD);
        hudC(11, "THE OTHER CREW IS THE CLOCK", PAL_AMBER);
        hudC(16, "A GAS    B BRAKE", PAL_HUD);
        hudC(17, "UP DOWN  RAISE THE SILL", PAL_HUD);
        hudC(22, "PRESS START", PAL_AMBER);
    } else if (mode_ == Mode::Pause) {
        hudC(10, "PAUSED", PAL_AMBER);
    } else if (mode_ == Mode::Win) {
        hudC(3, "LEVEL", PAL_GOOD);
        hudC(5, "STOPPED LEVEL WITH THE PLATFORM", PAL_GOOD);
        std::snprintf(buf, sizeof buf, "RUN %.1f S", time_);
        hudC(8, buf, PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        hudC(3, "STAND LOST", PAL_BAD);
        hudC(5, why_, PAL_BAD);
    } else {
        std::snprintf(buf, sizeof buf, "SPD %4.1f", v_);
        hud(1, 1, buf, PAL_HUD);
        float gap = kMark - rival_;
        std::snprintf(buf, sizeof buf, "CREW %3.0f M", std::max(0.f, gap));
        hud(26, 1, buf, gap < 10.f ? PAL_BAD : PAL_AMBER);
        float err = kMark - s_;
        if (s_ + kNose < kPlat0) {
            std::snprintf(buf, sizeof buf, "PLATFORM  %3.0f M", kPlat0 - (s_ + kNose));
            hudC(2, buf, PAL_AMBER);
        } else if (std::fabs(err) <= kXTol && std::fabs(h_ - kDeck) <= kHTol) {
            hudC(2, "HOLD  LEVEL", PAL_GOOD);
        } else if (std::fabs(err) > kXTol) {
            hudC(2, err > 0.f ? "SHORT OF THE STRIPE" : "PAST THE STRIPE", PAL_BAD);
        } else if (h_ < kDeck - kHTol) {
            hudC(2, "SILL LOW", PAL_BAD);
        } else {
            hudC(2, "SILL HIGH", PAL_BAD);
        }
        hudC(26, "A GAS  B BRAKE  UP DOWN SILL", PAL_HUD);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.75f);
    sys.apu.setEcho(0.08f, 0.14f, 0.08f);
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
        if (chimeT_ > 0.13f) {
            if (chime_ < 4) sys.apu.keyOn(0, notes[chime_], 0.2f);
            else sys.apu.keyOff(0);
            chime_++;
            chimeT_ = 0;
            if (chime_ > 8) chime_ = -1;
        }
    }

    if (mode_ == Mode::Title) {
        time_ += kDt;
        s_ = 20.f + std::sin(time_ * 0.5f) * 1.6f;
        h_ = 0.72f + std::sin(time_ * 0.7f) * 0.12f;
        rival_ = 6.f + time_ * 0.55f;
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

    float gas = 0, brake = 0, lift = 0;
    if (bot_) {
        pilot(gas, brake, lift);
    } else {
        if (pad.down(gs::BTN_A) || pad.down(gs::BTN_C) || pad.accel > 0.15f) gas = 1.f;
        if (pad.down(gs::BTN_B) || pad.brake > 0.15f) brake = 1.f;
        if (pad.down(gs::BTN_UP)) lift += 1.f;
        if (pad.down(gs::BTN_DOWN)) lift -= 1.f;
        if (std::fabs(pad.axisY) > 0.25f) lift = pad.axisY;
        if (pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(240.f);
            draw();
            return;
        }
    }

    physics(gas, brake, lift);
    if (mode_ == Mode::Run) {
        float rpm = 64.f + std::fabs(v_) * 16.f + gas_ * 36.f;
        sys.apu.tone(2, rpm, 0.03f + gas_ * 0.03f);
        sys.apu.noise(brake_ > 0.4f && std::fabs(v_) > 1.f ? 0.07f : 0.01f, 480.f, false);
        bool near = std::fabs(s_ - kMark) < 2.2f;
        sys.setLight(near ? 40 : 30, near ? 140 : 70, near ? 80 : 150);
    }
    draw();
}

}  // namespace busplat

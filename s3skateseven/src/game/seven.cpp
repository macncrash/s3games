#include "game/seven.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace skateseven {
namespace {

constexpr float kSpeed = 2.55f;
constexpr float kGrav = 0.18f;
constexpr float kOllie = 4.75f;
constexpr float kTorque = 4.0f;
constexpr float kLandAng = 18.f;
constexpr float kGround = 168.f;
constexpr float kAnchor = 100.f;
constexpr int kRivalEvery = 108;

struct Deck {
    float x0, x1;
};

// Eight slabs. Landings 1..7 are the race. Gaps sit inside one ollie (~130 px).
const Deck kDecks[8] = {
    {0.f, 190.f},     {280.f, 430.f},   {520.f, 670.f},   {760.f, 910.f},
    {1000.f, 1150.f}, {1240.f, 1390.f}, {1480.f, 1630.f}, {1720.f, 1960.f},
};

float clampf(float v, float a, float b) {
    if (v < a) return a;
    if (v > b) return b;
    return v;
}

}  // namespace

int Game::deckAt(float x) const {
    int found = -1;
    for (int i = 0; i < 8; i++) {
        if (x >= kDecks[i].x0 && x < kDecks[i].x1) found = i;
    }
    return found;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    you_ = 0;
    them_ = 0;
    won_ = false;
    over_ = false;
    mode_ = Mode::Title;
    hold_ = 0;
    tick_ = 0;
    t_ = 0;
    x_ = 48.f;
    y_ = 0;
    vy_ = 0;
    angle_ = 0;
    grounded_ = true;
    say_[0] = 0;
    sayT_ = 0;
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
}

void Game::beginRun() {
    won_ = false;
    over_ = false;
    you_ = 0;
    them_ = 0;
    rivalWait_ = 0;
    x_ = 36.f;
    y_ = 0;
    vy_ = 0;
    angle_ = 0;
    grounded_ = true;
    shake_ = 0;
    mode_ = Mode::Run;
    std::snprintf(say_, sizeof say_, "DROP IN");
    sayT_ = 0.6f;
    blip(320.f, 0.08f, 5);
}

void Game::blip(float freq, float vol, int frames) {
    if (!sys_ || fanT_ > 0) return;
    sys_->apu.tone(0, freq, vol);
    blip_ = frames;
}

void Game::bail() {
    grounded_ = false;
    vy_ = 0;
    shake_ = 8;
    fallT_ = 36;
    mode_ = Mode::Fall;
    std::snprintf(say_, sizeof say_, "FALL");
    sayT_ = 0.8f;
    if (sys_) {
        sys_->apu.noiseBurst(0.26f, 880.f, 0.16f);
        sys_->rumble(0.55f, 0.2f, 120);
        sys_->setLight(180, 30, 20);
    }
}

void Game::land(int deck) {
    if (deck <= 0) return;
    if (deck != you_ + 1) {
        bail();
        return;
    }
    you_ = deck;
    std::snprintf(say_, sizeof say_, "LAND %d", you_);
    sayT_ = 0.55f;
    blip(400.f + float(you_) * 36.f, 0.1f, 6);
    if (sys_) sys_->rumble(0.1f, 0.18f, 50);
    if (you_ >= kGoal) {
        if (them_ >= kGoal) {
            won_ = false;
            mode_ = Mode::Lose;
            hold_ = 0;
            std::snprintf(say_, sizeof say_, "TOO LATE");
            return;
        }
        won_ = true;
        mode_ = Mode::Win;
        hold_ = 0;
        fanT_ = 1;
        std::snprintf(say_, sizeof say_, "FIRST TO SEVEN");
        sayT_ = 2.f;
        if (sys_) {
            sys_->rumble(0.2f, 0.5f, 180);
            sys_->setLight(40, 180, 90);
        }
    }
}

void Game::rivalTick() {
    if (mode_ != Mode::Run && mode_ != Mode::Fall) return;
    if (them_ >= kGoal) return;
    rivalWait_++;
    if (rivalWait_ < kRivalEvery) return;
    rivalWait_ = 0;
    them_++;
    if (them_ >= kGoal && you_ < kGoal && mode_ == Mode::Run) {
        won_ = false;
        mode_ = Mode::Lose;
        hold_ = 0;
        std::snprintf(say_, sizeof say_, "THEY HIT SEVEN");
        sayT_ = 2.f;
        if (sys_) sys_->setLight(160, 20, 30);
    }
}

bool Game::botTap() const {
    if (!grounded_) return false;
    if (you_ < 0 || you_ >= kGoal) return false;
    const Deck& d = kDecks[you_];
    return x_ >= d.x1 - 42.f && x_ < d.x1 - 22.f;
}

float Game::stick() const {
    if (!grounded_) {
        if (bot_) {
            if (angle_ > 1.2f) return -1.f;
            if (angle_ < -1.2f) return 1.f;
            return 0;
        }
    } else if (bot_) {
        return 0;
    }
    if (!sys_) return 0;
    const gs::Pad& pad = sys_->pad;
    if (std::fabs(pad.axisX) > 0.15f) return clampf(pad.axisX, -1.f, 1.f);
    float s = 0;
    if (pad.down(gs::BTN_LEFT)) s -= 1.f;
    if (pad.down(gs::BTN_RIGHT)) s += 1.f;
    return s;
}

void Game::physics() {
    bool tap = false;
    bool steady = false;
    if (bot_) tap = botTap();
    else if (sys_) {
        const gs::Pad& pad = sys_->pad;
        tap = pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_UP);
        steady = pad.down(gs::BTN_B) || pad.down(gs::BTN_X);
    }
    float s = stick();

    if (tap && grounded_) {
        grounded_ = false;
        vy_ = kOllie;
        blip(720.f, 0.08f, 4);
    }

    if (grounded_) angle_ = 0;
    else {
        angle_ += s * kTorque + 0.38f;
        if (steady) angle_ *= 0.86f;
        angle_ = clampf(angle_, -80.f, 80.f);
    }

    if (grounded_) {
        x_ += kSpeed;
        int d = deckAt(x_);
        if (d < 0) {
            grounded_ = false;
            vy_ = 0;
            return;
        }
        y_ = 0;
        vy_ = 0;
        return;
    }

    float prevY = y_;
    vy_ -= kGrav;
    y_ += vy_;
    x_ += kSpeed;
    int d = deckAt(x_);
    if (d >= 0 && prevY > 0.f && y_ <= 0.f && vy_ <= 0) {
        if (std::fabs(angle_) > kLandAng) {
            bail();
            return;
        }
        y_ = 0;
        vy_ = 0;
        grounded_ = true;
        angle_ = 0;
        shake_ = std::max(shake_, 2);
        land(d);
        return;
    }
    if (y_ <= 0.f && d < 0) bail();
    else if (y_ < -48.f) bail();
}

void Game::audio() {
    if (!sys_) return;
    gs::APU& a = sys_->apu;
    if (mode_ == Mode::Run && grounded_) a.noise(0.014f, 460.f, true);
    else a.noise(0.f, 400.f, false);

    if (fanT_ > 0) {
        fanT_++;
        if (fanT_ == 2 || fanT_ == 12 || fanT_ == 22 || fanT_ == 34) {
            static const float n[4] = {392.f, 523.f, 659.f, 784.f};
            int i = fanT_ < 12 ? 0 : fanT_ < 22 ? 1 : fanT_ < 34 ? 2 : 3;
            a.tone(0, n[i], 0.13f);
            a.tone(1, n[i] * 0.5f, 0.05f);
        }
        if (fanT_ > 64) {
            a.tone(0, 0, 0);
            a.tone(1, 0, 0);
            fanT_ = 0;
        }
        return;
    }
    if (blip_ > 0) {
        blip_--;
        if (blip_ == 0) a.tone(0, 0, 0);
    } else a.tone(1, 0, 0);
}

float Game::screenX(float wx) const { return kAnchor + (wx - camX_); }
float Game::screenY(float wy) const { return kGround - wy; }

void Game::image(const gs::Mipped& m, float left, float top, float h, int pal, bool flip, int fog, bool shadow) {
    if (!sys_ || h < 1.5f || h > 420.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    if (left > 340.f || top > 250.f || left + w < -30.f || top + h < -30.f) return;
    gs::Sprite s;
    auto q = [](float v) { return int16_t(std::lround(clampf(v, -400.f, 800.f))); };
    s.x = q(left);
    s.y = q(top);
    s.w = int16_t(std::max(1, int(std::lround(w))));
    s.h = int16_t(std::max(1, int(std::lround(h))));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::spr(const gs::Mipped& m, float cx, float bottom, float h, int pal, bool flip, int fog) {
    if (m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    image(m, cx - w * 0.5f, bottom - h, h, pal, flip, fog, false);
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.road[y].on = false;
        v.lineFog[y] = 0;
        if (y < 96) {
            float u = float(y) / 96.f;
            v.lineBackdrop[y] = gs::rgb4(4 + int((1.f - u) * 2.f), 2 + int(u * 2.f), 12 - int(u * 2.f));
        } else if (y < int(kGround)) {
            float u = float(y - 96) / (kGround - 96.f);
            v.lineBackdrop[y] = gs::rgb4(11 - int(u * 4.f), 6 - int(u * 2.f), 4);
        } else {
            v.lineBackdrop[y] = gs::rgb4(2, 1, 3);
        }
    }
}

void Game::world() {
    auto strip = [&](float x0, float x1) {
        if (art_.deck.h < 1) return;
        float h = 18.f;
        float w = h * float(art_.deck.w) / float(art_.deck.h);
        float step = std::max(8.f, w * 0.92f);
        for (float x = x0; x < x1; x += step) {
            float sx = screenX(x);
            if (sx > 340.f || sx + w < -20.f) continue;
            image(art_.deck, sx, screenY(0), h, PAL_DECK, false, 0, false);
        }
    };
    for (int i = 0; i < 8; i++) {
        strip(kDecks[i].x0, kDecks[i].x1);
        if (i > 0) spr(art_.cone, screenX(kDecks[i].x0 + 8.f), screenY(0), 14.f, PAL_PROP, false, 0);
    }
    static const float farX[] = {40.f, 240.f, 480.f, 760.f, 1040.f, 1320.f, 1600.f, 1880.f};
    static const float farH[] = {64.f, 48.f, 78.f, 56.f, 70.f, 50.f, 74.f, 46.f};
    for (int i = 0; i < 8; i++) {
        float sx = kAnchor + (farX[i] - camX_) * 0.32f;
        spr(art_.building, sx, 104.f, farH[i], PAL_CITY, false, 8);
    }
    float drift = std::fmod(t_ * 10.f, 360.f);
    spr(art_.cloud, drift - 20.f, 30.f, 14.f, PAL_FX, false, 2);
    spr(art_.sun, 270.f, 36.f, 22.f, PAL_FX, false, 0);

    float rx = screenX(x_ - float(them_) * 70.f);
    spr(art_.ride, rx, screenY(0) + 2.f, 36.f, PAL_RIVAL, false, 6);
}

void Game::rider() {
    float wx = (mode_ == Mode::Title) ? 70.f : x_;
    float wy = (mode_ == Mode::Title) ? 0.f : y_;
    if (mode_ == Mode::Title && (tick_ / 36) % 2 == 1) wy = 14.f;
    float sx = screenX(wx);
    if (shake_) sx += (tick_ & 1) ? 2.f : -2.f;
    float feet = screenY(wy) + 2.f;
    image(art_.shadow, sx - 14.f, feet - 3.f, 8.f, PAL_FX, false, 0, true);
    const gs::Mipped* body = &art_.ride;
    float h = 50.f;
    if (mode_ == Mode::Fall) {
        body = &art_.bail;
        h = 26.f;
        feet += 6.f;
    } else if (!grounded_ && mode_ != Mode::Title) {
        body = &art_.air;
        h = 48.f;
    } else if (mode_ == Mode::Title && wy > 4.f) {
        body = &art_.air;
        h = 48.f;
    }
    spr(*body, sx, feet, h, PAL_SKATER, false, 0);
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (!sys_ || row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        int tile = art_.font[c - 32];
        if (!tile) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(tile, pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();
    world();
    rider();

    char buf[48];
    if (mode_ == Mode::Title) {
        spr(art_.logo, 160.f, 34.f, 20.f, PAL_GOLD, false, 0);
        hudC(8, "A SHORT SKATE", PAL_GOLD);
        hudC(10, "FIRST TO SEVEN", PAL_HUD);
        hudC(12, "ARROWS LEVEL   C OLLIES", PAL_HUD);
        hudC(16, "ENTER DROPS IN", PAL_GREEN);
        return;
    }

    hud(1, 0, "S3 SKATE SEVEN", PAL_GOLD);
    std::snprintf(buf, sizeof buf, "YOU %d  THEM %d", you_, them_);
    hud(22, 0, buf, you_ > them_ ? PAL_GREEN : PAL_HUD);

    if (mode_ == Mode::Pause) {
        hudC(4, "PAUSED", PAL_GOLD);
        hudC(6, "ENTER RESUMES", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Win) {
        hudC(12, "FIRST TO SEVEN", PAL_GREEN);
        std::snprintf(buf, sizeof buf, "YOU %d  THEM %d", you_, them_);
        hudC(14, buf, PAL_GOLD);
        return;
    }
    if (mode_ == Mode::Lose) {
        hudC(12, "THEY GOT THERE", PAL_RED);
        std::snprintf(buf, sizeof buf, "YOU %d  THEM %d", you_, them_);
        hudC(14, buf, PAL_HUD);
        return;
    }
    if (mode_ == Mode::Fall) {
        hudC(4, "FALL", PAL_RED);
        hudC(6, "THE LINE RESETS", PAL_GOLD);
        return;
    }
    if (sayT_ > 0 && say_[0]) hudC(3, say_, PAL_GOLD);
    else hudC(3, grounded_ ? "RIDE THE NEXT GAP" : "LEVEL THE BOARD", PAL_HUD);

    if (!grounded_) {
        int tilt = int(std::lround(clampf(angle_ / 3.f, -8.f, 8.f)));
        char meter[18];
        for (int i = 0; i < 17; i++) meter[i] = '-';
        meter[8] = '+';
        meter[std::clamp(8 + tilt, 0, 16)] = 'O';
        meter[17] = 0;
        hudC(5, meter, std::fabs(angle_) > 12.f ? PAL_RED : PAL_GREEN);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += 1.f / 60.f;
    tick_++;
    if (sayT_ > 0) sayT_ = std::max(0.f, sayT_ - 1.f / 60.f);
    if (shake_ > 0) shake_--;

    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        hold_++;
        camX_ = 70.f;
        bool go = bot_ ? hold_ >= 20 : (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C));
        if (!bot_ && pad.pressed(gs::BTN_MODE)) sys.quit();
        if (go) beginRun();
        draw();
        audio();
        return;
    }
    if (mode_ == Mode::Pause) {
        camX_ = x_;
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) mode_ = Mode::Run;
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) sys.quit();
        draw();
        audio();
        return;
    }
    if (mode_ == Mode::Fall) {
        camX_ = x_;
        rivalTick();
        fallT_--;
        if (mode_ == Mode::Lose) {
            draw();
            audio();
            return;
        }
        if (bot_) {
            if (fallT_ <= 0) beginRun();
        } else if (fallT_ <= 0 || pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C)) {
            beginRun();
        }
        draw();
        audio();
        return;
    }
    if (mode_ == Mode::Win || mode_ == Mode::Lose) {
        camX_ = x_;
        hold_++;
        if (hold_ > 70) {
            over_ = true;
            if (!sys.headless) sys.quit();
        }
        draw();
        audio();
        return;
    }

    if (!bot_ && pad.pressed(gs::BTN_START)) {
        mode_ = Mode::Pause;
        draw();
        audio();
        return;
    }
    if (!bot_ && pad.pressed(gs::BTN_MODE)) {
        sys.quit();
        return;
    }

    camX_ = x_;
    rivalTick();
    if (mode_ == Mode::Run) physics();
    if (mode_ == Mode::Run || mode_ == Mode::Win) camX_ = x_;
    draw();
    audio();
}

}  // namespace skateseven

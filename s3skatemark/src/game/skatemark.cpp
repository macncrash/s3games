#include "game/skatemark.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace skatemark {
namespace {

constexpr float kSpeed = 2.5f;
constexpr float kGrav = 0.18f;
constexpr float kOllie = 4.8f;
constexpr float kTorque = 4.2f;
constexpr float kLandAng = 16.f;
constexpr float kGround = 168.f;
constexpr float kAnchor = 108.f;

struct Deck {
    float x0, x1, top;
};

// Three landings. Jump windows are tuned to a 53-frame ollie (~132 px).
const Deck kDecks[] = {
    {0.f, 210.f, 0.f},
    {300.f, 500.f, 0.f},
    {560.f, 760.f, 0.f},
    {840.f, 1120.f, 0.f},
};

const char* kName[Game::kTricks] = {"CURB", "RAIL", "BANK"};

float clampf(float v, float a, float b) {
    if (v < a) return a;
    if (v > b) return b;
    return v;
}

}  // namespace

int Game::line() const {
    int n = 0;
    for (int i = 0; i < kTricks; i++)
        if (did_[i]) n++;
    return n;
}

int Game::deckAt(float x) const {
    int found = -1;
    for (int i = 0; i < 4; i++) {
        if (x >= kDecks[i].x0 && x < kDecks[i].x1) found = i;
    }
    return found;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    falls_ = 0;
    won_ = false;
    over_ = false;
    finished_ = false;
    mode_ = Mode::Title;
    hold_ = 0;
    tick_ = 0;
    t_ = 0;
    x_ = 48.f;
    y_ = 0;
    vy_ = 0;
    angle_ = 0;
    grounded_ = true;
    for (int i = 0; i < kTricks; i++) did_[i] = false;
    say_[0] = 0;
    sayT_ = 0;
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
}

void Game::beginRun(bool keepFalls) {
    if (!keepFalls) falls_ = 0;
    won_ = false;
    over_ = false;
    finished_ = false;
    x_ = 40.f;
    y_ = 0;
    vy_ = 0;
    angle_ = 0;
    grounded_ = true;
    shake_ = 0;
    for (int i = 0; i < kTricks; i++) did_[i] = false;
    mode_ = Mode::Run;
    std::snprintf(say_, sizeof say_, "DROP IN");
    sayT_ = 0.7f;
    blip(330.f, 0.08f, 5);
}

void Game::blip(float freq, float vol, int frames) {
    if (!sys_ || fanT_ > 0) return;
    sys_->apu.tone(0, freq, vol);
    blip_ = frames;
}

void Game::bail(const char* why) {
    (void)why;
    for (int i = 0; i < kTricks; i++) did_[i] = false;
    falls_++;
    grounded_ = false;
    vy_ = 0;
    shake_ = 8;
    fallT_ = 40;
    mode_ = Mode::Fall;
    won_ = false;
    finished_ = false;
    std::snprintf(say_, sizeof say_, "FALL");
    sayT_ = 1.f;
    if (sys_) {
        sys_->apu.noiseBurst(0.28f, 900.f, 0.18f);
        sys_->rumble(0.6f, 0.25f, 140);
        sys_->setLight(180, 30, 20);
    }
}

void Game::scoreTrick(int i) {
    if (i < 0 || i >= kTricks || did_[i]) return;
    did_[i] = true;
    std::snprintf(say_, sizeof say_, "%s", kName[i]);
    sayT_ = 0.8f;
    blip(420.f + float(i) * 70.f, 0.1f, 7);
    if (sys_) sys_->rumble(0.12f, 0.22f, 60);
}

void Game::finishMark() {
    finished_ = true;
    won_ = true;
    mode_ = Mode::Win;
    hold_ = 0;
    fanT_ = 1;
    std::snprintf(say_, sizeof say_, "FINISHED MARK");
    sayT_ = 3.f;
    if (sys_) {
        sys_->rumble(0.25f, 0.55f, 200);
        sys_->setLight(40, 180, 90);
    }
}

bool Game::botTap() const {
    if (!grounded_) return false;
    if (!did_[0] && x_ >= 175.f && x_ < 196.f) return true;
    if (did_[0] && !did_[1] && x_ >= 452.f && x_ < 474.f) return true;
    if (did_[1] && !did_[2] && x_ >= 732.f && x_ < 754.f) return true;
    return false;
}

float Game::stick() const {
    if (!grounded_) {
        if (bot_) {
            if (angle_ > 1.5f) return -1.f;
            if (angle_ < -1.5f) return 1.f;
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

void Game::groundStep() {
    x_ += kSpeed;
    int d = deckAt(x_);
    if (d < 0) {
        grounded_ = false;
        vy_ = 0;
        return;
    }
    y_ = kDecks[d].top;
    vy_ = 0;
}

void Game::airStep() {
    float prevY = y_;
    vy_ -= kGrav;
    y_ += vy_;
    x_ += kSpeed;

    int d = deckAt(x_);
    if (d >= 0 && prevY > kDecks[d].top && y_ <= kDecks[d].top && vy_ <= 0) {
        if (std::fabs(angle_) > kLandAng) {
            bail("CROOKED");
            return;
        }
        y_ = kDecks[d].top;
        vy_ = 0;
        grounded_ = true;
        angle_ = 0;
        shake_ = std::max(shake_, 3);
        if (d == 1) scoreTrick(0);
        else if (d == 2) {
            if (!did_[0]) {
                bail("BROKEN LINE");
                return;
            }
            scoreTrick(1);
        } else if (d == 3) {
            if (!(did_[0] && did_[1])) {
                bail("BROKEN LINE");
                return;
            }
            scoreTrick(2);
            finishMark();
        }
        return;
    }
    if (d < 0 && y_ <= 0.f) bail("PIT");
    else if (y_ < -40.f) bail("PIT");
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
        blip(740.f, 0.09f, 5);
    }

    if (grounded_) angle_ = 0;
    else {
        angle_ += s * kTorque + 0.42f;
        if (steady) angle_ *= 0.86f;
        angle_ = clampf(angle_, -80.f, 80.f);
    }

    if (grounded_) groundStep();
    else airStep();
}

void Game::audio() {
    if (!sys_) return;
    gs::APU& a = sys_->apu;
    if (mode_ == Mode::Run && grounded_) a.noise(0.016f, 480.f, true);
    else a.noise(0.f, 400.f, false);

    if (fanT_ > 0) {
        fanT_++;
        if (fanT_ == 2 || fanT_ == 12 || fanT_ == 22 || fanT_ == 34) {
            static const float n[4] = {392.f, 523.f, 659.f, 784.f};
            int i = fanT_ < 12 ? 0 : fanT_ < 22 ? 1 : fanT_ < 34 ? 2 : 3;
            a.tone(0, n[i], 0.14f);
            a.tone(1, n[i] * 0.5f, 0.05f);
        }
        if (fanT_ > 70) {
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

const char* Game::coach() const {
    if (!grounded_) {
        if (angle_ > 8.f) return "HOLD LEFT";
        if (angle_ < -8.f) return "HOLD RIGHT";
        return "LEVEL TO LAND";
    }
    if (!did_[0] && x_ > 120.f) return "OLLIE THE CURB";
    if (did_[0] && !did_[1] && x_ > 360.f) return "OLLIE THE RAIL GAP";
    if (did_[1] && !did_[2] && x_ > 640.f) return "OLLIE THE BANK";
    return "RIDE TO THE MARK";
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
        if (y < 100) {
            float u = float(y) / 100.f;
            v.lineBackdrop[y] = gs::rgb4(6 + int((1.f - u) * 3.f), 2 + int(u * 3.f), 11 - int(u * 3.f));
        } else if (y < int(kGround)) {
            float u = float(y - 100) / (kGround - 100.f);
            v.lineBackdrop[y] = gs::rgb4(12 - int(u * 4.f), 7 - int(u * 2.f), 5 - int(u));
        } else {
            v.lineBackdrop[y] = gs::rgb4(2, 1, 3);
        }
    }
}

void Game::world() {
    auto strip = [&](const gs::Mipped& m, float x0, float x1, float top, float h, int pal) {
        if (m.h < 1) return;
        float w = h * float(m.w) / float(m.h);
        float step = std::max(8.f, w * 0.9f);
        for (float x = x0; x < x1; x += step) {
            float sx = screenX(x);
            if (sx > 340.f || sx + w < -20.f) continue;
            image(m, sx, screenY(top), h, pal, false, 0, false);
        }
    };

    spr(art_.cone, screenX(198.f), screenY(0), 16.f, PAL_PROP, false, 0);
    spr(art_.cone, screenX(488.f), screenY(0), 16.f, PAL_PROP, false, 0);
    spr(art_.cone, screenX(748.f), screenY(0), 16.f, PAL_PROP, false, 0);

    for (int i = 0; i < 4; i++) strip(art_.deck, kDecks[i].x0, kDecks[i].x1, kDecks[i].top, 20.f, PAL_DECK);

    for (float x = 560.f; x < 700.f; x += 22.f) spr(art_.rail, screenX(x), screenY(0) + 4.f, 26.f, PAL_RAIL, false, 0);
    image(art_.bank, screenX(800.f), screenY(0) - 4.f, 22.f, PAL_DECK, false, 0, false);

    static const float farX[] = {80.f, 260.f, 460.f, 680.f, 900.f, 1080.f};
    static const float farH[] = {70.f, 54.f, 82.f, 60.f, 74.f, 50.f};
    for (int i = 0; i < 6; i++) {
        float sx = kAnchor + (farX[i] - camX_) * 0.35f;
        spr(art_.building, sx, 108.f, farH[i], PAL_CITY, false, 7);
    }

    float drift = std::fmod(t_ * 12.f, 360.f);
    spr(art_.cloud, drift - 20.f, 34.f, 14.f, PAL_FX, false, 2);
    spr(art_.cloud, std::fmod(drift * 0.55f + 140.f, 380.f) - 30.f, 26.f, 18.f, PAL_FX, true, 3);
    spr(art_.sun, 278.f, 40.f, 26.f, PAL_FX, false, 0);
}

void Game::rider() {
    float wx = (mode_ == Mode::Title) ? 70.f : x_;
    float wy = (mode_ == Mode::Title) ? 0.f : y_;
    if (mode_ == Mode::Title && (tick_ / 40) % 2 == 1) wy = 16.f;
    float sx = screenX(wx);
    float jig = shake_ ? ((tick_ & 1) ? 2.f : -2.f) : 0.f;
    sx += jig;
    float feet = screenY(wy) + 2.f;
    image(art_.shadow, sx - 14.f, feet - 3.f, 8.f, PAL_FX, false, 0, true);

    const gs::Mipped* body = &art_.ride;
    float h = 52.f;
    if (mode_ == Mode::Fall) {
        body = &art_.bail;
        h = 28.f;
        feet += 6.f;
    } else if (!grounded_ && mode_ != Mode::Title) {
        body = &art_.air;
        h = 50.f;
    } else if (mode_ == Mode::Title && wy > 4.f) {
        body = &art_.air;
        h = 50.f;
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
    rider();
    world();

    if (mode_ == Mode::Win || (mode_ == Mode::Title && (tick_ / 30) % 2 == 0)) {
        spr(art_.card, 250.f, 78.f, 52.f, PAL_GOLD, false, 0);
        if (mode_ == Mode::Win) spr(art_.stamp, 250.f, 70.f, 18.f, PAL_RED, false, 0);
    }

    char buf[48];
    if (mode_ == Mode::Title) {
        spr(art_.logo, 150.f, 36.f, 22.f, PAL_GOLD, false, 0);
        hudC(8, "SKATE THE MARK", PAL_GOLD);
        hudC(10, "THREE LANDINGS FINISH IT", PAL_HUD);
        hudC(12, "ARROWS LEVEL   C OLLIES", PAL_HUD);
        hudC(16, "ENTER DROPS IN", PAL_GREEN);
        return;
    }

    hud(1, 0, "S3 SKATEMARK", PAL_GOLD);
    char pips[8];
    for (int i = 0; i < kTricks; i++) pips[i] = did_[i] ? '#' : '-';
    pips[kTricks] = 0;
    std::snprintf(buf, sizeof buf, "MARK %s", pips);
    hud(28, 0, buf, line() == kTricks ? PAL_GREEN : PAL_HUD);

    if (mode_ == Mode::Pause) {
        hudC(4, "PAUSED", PAL_GOLD);
        hudC(6, "ENTER RESUMES", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Win) {
        hudC(12, "FINISHED MARK", PAL_GREEN);
        hudC(14, "THE CARD IS CLOSED", PAL_GOLD);
        hudC(16, "LEAVE", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Fall) {
        hudC(4, "FALL", PAL_RED);
        hudC(6, "THE MARK IS OPEN", PAL_GOLD);
        return;
    }
    if (sayT_ > 0 && say_[0]) hudC(3, say_, PAL_GOLD);
    else hudC(3, coach(), PAL_HUD);

    if (!grounded_) {
        int tilt = int(std::lround(clampf(angle_ / 3.f, -8.f, 8.f)));
        char meter[18];
        for (int i = 0; i < 17; i++) meter[i] = '-';
        meter[8] = '+';
        int slot = std::clamp(8 + tilt, 0, 16);
        meter[slot] = 'O';
        meter[17] = 0;
        hudC(5, meter, std::fabs(angle_) > 10.f ? PAL_RED : PAL_GREEN);
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
        bool go = bot_ ? hold_ >= 24 : (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C));
        if (!bot_ && pad.pressed(gs::BTN_MODE)) sys.quit();
        if (go) beginRun(false);
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
        fallT_--;
        if (bot_) {
            if (fallT_ <= 0) beginRun(true);
        } else if (fallT_ <= 0 || pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C)) {
            beginRun(true);
        }
        draw();
        audio();
        return;
    }
    if (mode_ == Mode::Win) {
        camX_ = x_;
        hold_++;
        if (hold_ > 80) {
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
    physics();
    if (mode_ == Mode::Run || mode_ == Mode::Win) camX_ = x_;
    draw();
    audio();
}

}  // namespace skatemark

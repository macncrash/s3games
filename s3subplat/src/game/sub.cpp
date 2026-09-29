#include "sub.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace subplat {
namespace {
constexpr float kDt = 1.f / 60.f;
constexpr float kClock = 52.f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }
}  // namespace

void Game::placeBerth() {
    static const float decks[kBerths] = {96.f, 168.f, 62.f};
    static const float starts[kBerths] = {140.f, 70.f, 176.f};
    deck_ = decks[berth_];
    platX_ = 196.f;
    x_ = 36.f;
    y_ = starts[berth_];
    vx_ = 0;
    vy_ = 0;
    hold_ = 0;
}

void Game::begin() {
    berth_ = 0;
    clock_ = kClock;
    t_ = 0;
    dockT_ = 0;
    over_ = false;
    won_ = false;
    why_ = "";
    placeBerth();
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    begin();
    mode_ = bot_ ? Mode::Play : Mode::Title;
    sys.vdp.setFogColor(gs::rgb4(1, 4, 8));
}

bool Game::levelled() const {
    bool overPlat = (x_ + kHalf > platX_ + 8.f) && (x_ - kHalf < platX_ + kPlatW - 8.f);
    return overPlat && std::fabs(y_ - deck_) < 7.f && std::fabs(vx_) < 26.f && std::fabs(vy_) < 18.f;
}

void Game::pilot(float& thrust, float& ballast) const {
    float aimX = platX_ + kPlatW * 0.48f;
    float dx = aimX - x_;
    float dy = deck_ - y_;
    if (std::fabs(dx) > 34.f) thrust = dx > 0.f ? 1.f : -1.f;
    else thrust = clampf(-vx_ / 28.f, -1.f, 1.f);
    if (std::fabs(dy) > 5.f) ballast = dy > 0.f ? 1.f : -1.f;
    else ballast = clampf(-vy_ / 18.f, -1.f, 1.f);
    if (std::fabs(dx) < 18.f && std::fabs(dy) < 6.f) {
        thrust = clampf(-vx_ / 14.f, -1.f, 1.f);
        ballast = clampf(-vy_ / 12.f, -1.f, 1.f);
    }
}

void Game::step(float dt) {
    float thrust = 0, ballast = 0;
    if (bot_) {
        pilot(thrust, ballast);
    } else {
        const gs::Pad& p = sys_->pad;
        if (p.down(gs::BTN_RIGHT) || p.axisX > 0.3f) thrust += 1.f;
        if (p.down(gs::BTN_LEFT) || p.axisX < -0.3f) thrust -= 1.f;
        if (p.down(gs::BTN_DOWN) || p.axisY < -0.3f) ballast += 1.f;
        if (p.down(gs::BTN_UP) || p.axisY > 0.3f) ballast -= 1.f;
        if (p.down(gs::BTN_A) || p.down(gs::BTN_B)) {
            thrust += clampf(-vx_ / 40.f, -1.f, 1.f);
            ballast += clampf(-vy_ / 30.f, -1.f, 1.f);
        }
    }
    float drift = 8.f * std::sin(t_ * 0.7f + berth_);
    vx_ += thrust * 160.f * dt;
    vy_ += ballast * 130.f * dt + drift * dt;
    vx_ *= std::exp(-2.4f * dt);
    vy_ *= std::exp(-2.8f * dt);
    x_ += vx_ * dt;
    y_ += vy_ * dt;
    x_ = clampf(x_, 16.f, 300.f);
    y_ = clampf(y_, 28.f, 200.f);

    if (levelled()) hold_ += dt;
    else hold_ = std::max(0.f, hold_ - dt * 1.5f);

    if (hold_ >= 0.55f) {
        sys_->apu.tone(0, 660.f, 0.12f);
        tone_ = 0.12f;
        if (berth_ + 1 >= kBerths) {
            won_ = true;
            over_ = true;
            mode_ = Mode::Win;
            why_ = "level";
        } else {
            berth_++;
            placeBerth();
            mode_ = Mode::Dock;
            dockT_ = 0.55f;
        }
    }
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c < 32 || c > 127 || c == ' ') continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { 
    int n = 0;
    if (s)
        while (s[n]) n++;
    hud(20 - n / 2, row, s, pal);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (m.h < 1 || h < 1.f) return;
    float sc = h / float(m.h);
    float w = float(m.w) * sc;
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - w * 0.5f));
    s.y = int16_t(std::lround(cy - h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int g = 2 + y / 28;
        int b = 5 + y / 18;
        v.lineBackdrop[y] = gs::rgb4(0, std::min(g, 8), std::min(b, 12));
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }

    for (int i = 0; i < 5; i++) {
        float kx = 24.f + i * 62.f;
        float sway = 4.f * std::sin(t_ * 1.3f + i);
        spr(art_.kelp, kx + sway, 196.f, 52.f, PAL_KELP);
    }
    float px = platX_ + kPlatW * 0.5f;
    float ph = 48.f;
    spr(art_.plat, px, deck_ + 10.f, ph, PAL_PLAT);
    spr(art_.lamp, platX_ + 16.f, deck_ - 6.f, 10.f, PAL_LIGHT);
    spr(art_.lamp, platX_ + kPlatW - 16.f, deck_ - 6.f, 10.f, PAL_LIGHT);

    float rival = 1.f - clampf(clock_ / kClock, 0.f, 1.f);
    float rx = 300.f - rival * 150.f;
    spr(art_.rival, rx, 22.f + 6.f * std::sin(t_ * 2.f), 12.f, PAL_RIVAL, true);

    for (int i = 0; i < 6; i++) {
        float bx = std::fmod(40.f + i * 48.f + t_ * (12.f + i), 320.f);
        float by = 40.f + std::fmod(i * 37.f + t_ * 18.f, 160.f);
        spr(art_.bub, bx, by, 6.f + (i % 3), PAL_BUB);
    }
    for (int i = 0; i < 3; i++) {
        float fx = std::fmod(300.f - t_ * (22.f + i * 8.f) + i * 90.f, 340.f) - 10.f;
        spr(art_.fish, fx, 50.f + i * 48.f, 10.f, PAL_FISH, true);
    }

    bool flash = levelled() && (int(t_ * 8.f) & 1);
    spr(art_.sub, x_, y_, 26.f, flash ? PAL_WIN : PAL_SUB, vx_ < -4.f);

    char buf[48];
    std::snprintf(buf, sizeof(buf), "BERTH %d/%d", berth_ + 1, kBerths);
    hud(1, 0, "S3 SUBPLAT", PAL_BANNER);
    hud(28, 0, buf, PAL_HUD);
    int sec = std::max(0, int(std::ceil(clock_)));
    std::snprintf(buf, sizeof(buf), "CREW %02d", sec);
    hud(30, 1, buf, clock_ < 12.f ? PAL_ALERT : PAL_RIVAL);
    if (mode_ == Mode::Title) {
        hudC(20, "STOP LEVEL WITH THE PLATFORM", PAL_HUD);
        hudC(22, "ARROWS  DEPTH AND DRIVE", PAL_HUD);
        hudC(24, "A BRAKE    START DIVE", PAL_BANNER);
    } else if (mode_ == Mode::Play) {
        hudC(26, levelled() ? "HOLD HER LEVEL" : "MATCH THE DECK AND STOP", PAL_HUD);
    } else if (mode_ == Mode::Dock) {
        hudC(26, "BERTH TAKEN", PAL_WIN);
    } else if (mode_ == Mode::Win) {
        hudC(24, "STOPPED LEVEL AHEAD OF THE CREW", PAL_WIN);
    } else if (mode_ == Mode::Fail) {
        hudC(24, "THE OTHER CREW TOOK THE PLATFORM", PAL_ALERT);
    } else if (mode_ == Mode::Pause) {
        hudC(24, "PAUSED", PAL_BANNER);
    }

    if (mode_ == Mode::Title) spr(art_.title, 160.f, 36.f, float(art_.title.h) * 2.f, PAL_BANNER);
    else if (mode_ == Mode::Win) spr(art_.level, 160.f, 40.f, float(art_.level.h) * 2.f, PAL_WIN);
    else if (mode_ == Mode::Fail) spr(art_.late, 160.f, 40.f, float(art_.late.h) * 2.f, PAL_ALERT);
    else if (mode_ == Mode::Pause) spr(art_.paused, 160.f, 40.f, float(art_.paused.h) * 2.f, PAL_BANNER);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) {
            begin();
            mode_ = Mode::Play;
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
    } else if ((mode_ == Mode::Win || mode_ == Mode::Fail) && !bot_) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) {
            begin();
            mode_ = Mode::Play;
        }
    }

    if (mode_ == Mode::Play) {
        t_ += kDt;
        clock_ -= kDt;
        if (clock_ <= 0.f) {
            clock_ = 0.f;
            over_ = true;
            won_ = false;
            why_ = "crew";
            mode_ = Mode::Fail;
        } else {
            step(kDt);
        }
    } else if (mode_ == Mode::Dock) {
        t_ += kDt;
        dockT_ -= kDt;
        if (dockT_ <= 0.f) mode_ = Mode::Play;
    } else {
        t_ += kDt;
    }

    if (tone_ > 0.f) {
        tone_ -= kDt;
        if (tone_ <= 0.f) sys.apu.tone(0, 0.f, 0.f);
    }
    draw();
}

}  // namespace subplat

#include "keel.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace keelplat {
namespace {
constexpr float kDt = 1.f / 60.f;
constexpr float kClock = 56.f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }
}  // namespace

void Game::placeBerth() {
    static const float decks[kBerths] = {108.f, 156.f, 78.f};
    static const float starts[kBerths] = {148.f, 86.f, 168.f};
    static const float winds[kBerths] = {10.f, -12.f, 8.f};
    deck_ = decks[berth_];
    platX_ = 188.f;
    wind_ = winds[berth_];
    x_ = 28.f;
    y_ = starts[berth_];
    vx_ = 0;
    vy_ = 0;
    hold_ = 0;
}

void Game::begin() {
    berth_ = 0;
    clock_ = kClock;
    time_ = 0;
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
    sys.vdp.setFogColor(gs::rgb4(5, 8, 12));
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
}

bool Game::levelled() const {
    bool alongside = (x_ + kHalf > platX_ + 10.f) && (x_ - kHalf < platX_ + kPlatW - 10.f);
    return alongside && std::fabs(y_ - deck_) < 6.f && std::fabs(vx_) < 22.f && std::fabs(vy_) < 16.f;
}

void Game::pilot(float& thrust, float& helm) const {
    float aimX = platX_ + kPlatW * 0.50f;
    float dx = aimX - x_;
    float dy = deck_ - y_;
    if (std::fabs(dx) > 30.f) thrust = dx > 0.f ? 1.f : -1.f;
    else thrust = clampf(-vx_ / 24.f, -1.f, 1.f);
    if (std::fabs(dy) > 4.f) helm = dy > 0.f ? 1.f : -1.f;
    else helm = clampf(-vy_ / 16.f, -1.f, 1.f);
    if (std::fabs(dx) < 16.f && std::fabs(dy) < 5.f) {
        thrust = clampf(-vx_ / 12.f, -1.f, 1.f);
        helm = clampf(-vy_ / 10.f, -1.f, 1.f);
    }
}

void Game::fail(const char* why) {
    won_ = false;
    over_ = true;
    why_ = why;
    mode_ = Mode::Fail;
    sys_->apu.tone(0, 140.f, 0.18f);
    tone_ = 0.18f;
}

void Game::step(float dt) {
    float thrust = 0, helm = 0;
    if (bot_) {
        pilot(thrust, helm);
    } else {
        const gs::Pad& p = sys_->pad;
        if (p.down(gs::BTN_RIGHT) || p.axisX > 0.3f) thrust += 1.f;
        if (p.down(gs::BTN_LEFT) || p.axisX < -0.3f) thrust -= 1.f;
        if (p.down(gs::BTN_DOWN) || p.axisY < -0.3f) helm += 1.f;
        if (p.down(gs::BTN_UP) || p.axisY > 0.3f) helm -= 1.f;
        if (p.down(gs::BTN_A) || p.down(gs::BTN_B)) {
            thrust += clampf(-vx_ / 36.f, -1.f, 1.f);
            helm += clampf(-vy_ / 28.f, -1.f, 1.f);
        }
    }
    vx_ += thrust * 150.f * dt;
    vy_ += helm * 120.f * dt + wind_ * std::sin(t_ * 0.85f + berth_) * dt;
    vx_ *= std::exp(-2.2f * dt);
    vy_ *= std::exp(-2.6f * dt);
    x_ += vx_ * dt;
    y_ += vy_ * dt;
    x_ = clampf(x_, 16.f, 304.f);
    y_ = clampf(y_, 36.f, 200.f);

    if (x_ >= 300.f && vx_ > 8.f) {
        fail("past");
        return;
    }
    if (x_ <= 18.f && vx_ < -8.f && t_ > 1.5f) {
        fail("short");
        return;
    }

    if (levelled()) hold_ += dt;
    else hold_ = std::max(0.f, hold_ - dt * 1.5f);

    if (hold_ >= 0.6f) {
        sys_->apu.tone(0, 620.f, 0.12f);
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
            dockT_ = 0.5f;
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
        int sky = y < 70;
        int r = sky ? 4 + (70 - y) / 18 : 1;
        int g = sky ? 7 + (70 - y) / 20 : 6 + (y - 70) / 28;
        int b = sky ? 11 : 8 + (y - 70) / 22;
        v.lineBackdrop[y] = gs::rgb4(std::min(r, 8), std::min(g, 11), std::min(b, 14));
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }

    float px = platX_ + kPlatW * 0.5f;
    spr(art_.quay, px, deck_ - 6.f, 52.f, PAL_QUAY);
    spr(art_.post, platX_ + 14.f, deck_ - 22.f, 22.f, PAL_POST);
    spr(art_.post, platX_ + kPlatW - 14.f, deck_ - 22.f, 22.f, PAL_POST);

    for (int i = 0; i < 7; i++) {
        float wx = std::fmod(i * 48.f + t_ * (14.f + i), 340.f) - 10.f;
        float wy = 188.f + 6.f * std::sin(t_ * 1.4f + i);
        spr(art_.wave, wx, wy, 8.f, PAL_WATER);
    }
    for (int i = 0; i < 3; i++) {
        float gx = std::fmod(40.f + i * 110.f + t_ * (28.f + i * 6.f), 360.f) - 20.f;
        float gy = 28.f + i * 10.f + 4.f * std::sin(t_ * 2.f + i);
        spr(art_.gull, gx, gy, 8.f, PAL_GULL, (i & 1) != 0);
    }

    float rival = 1.f - clampf(clock_ / kClock, 0.f, 1.f);
    float rx = 300.f - rival * 170.f;
    spr(art_.rival, rx, 18.f + 3.f * std::sin(t_ * 2.2f), 12.f, PAL_CREW, true);

    bool flash = levelled() && (int(t_ * 8.f) & 1);
    float heel = clampf(vy_ * 0.15f, -6.f, 6.f);
    spr(art_.hull, x_, y_ + 4.f, 28.f, flash ? PAL_WIN : PAL_HULL, vx_ < -4.f);
    spr(art_.sail, x_ - 6.f, y_ - 16.f + heel, 30.f, PAL_SAIL, vx_ < -4.f);

    char buf[48];
    std::snprintf(buf, sizeof(buf), "BERTH %d/%d", berth_ + 1, kBerths);
    hud(1, 0, "S3 KEELPLAT", PAL_BANNER);
    hud(28, 0, buf, PAL_HUD);
    int sec = std::max(0, int(std::ceil(clock_)));
    std::snprintf(buf, sizeof(buf), "CREW %02d", sec);
    hud(30, 1, buf, clock_ < 12.f ? PAL_ALERT : PAL_CREW);
    if (mode_ == Mode::Title) {
        hudC(18, "STOP LEVEL WITH THE PLATFORM", PAL_HUD);
        hudC(20, "ARROWS  SAIL ALONG THE QUAY", PAL_HUD);
        hudC(22, "A BACKS THE SAIL", PAL_HUD);
        hudC(24, "START TO CAST OFF", PAL_BANNER);
    } else if (mode_ == Mode::Play) {
        hudC(26, levelled() ? "HOLD HER LEVEL" : "MATCH THE QUAY AND STOP", PAL_HUD);
    } else if (mode_ == Mode::Dock) {
        hudC(26, "BERTH TAKEN", PAL_WIN);
    } else if (mode_ == Mode::Win) {
        hudC(24, "STOPPED LEVEL AHEAD OF THE CREW", PAL_WIN);
    } else if (mode_ == Mode::Fail) {
        if (why_ && std::strcmp(why_, "past") == 0) hudC(24, "PAST THE PLATFORM", PAL_ALERT);
        else if (why_ && std::strcmp(why_, "short") == 0) hudC(24, "SHORT OF THE PLATFORM", PAL_ALERT);
        else hudC(24, "THE OTHER CREW TOOK THE PLATFORM", PAL_ALERT);
    } else if (mode_ == Mode::Pause) {
        hudC(24, "PAUSED", PAL_BANNER);
    }

    if (mode_ == Mode::Title) spr(art_.title, 160.f, 40.f, float(art_.title.h) * 2.f, PAL_BANNER);
    else if (mode_ == Mode::Win) spr(art_.level, 160.f, 44.f, float(art_.level.h) * 2.f, PAL_WIN);
    else if (mode_ == Mode::Fail) spr(art_.late, 160.f, 44.f, float(art_.late.h) * 2.f, PAL_ALERT);
    else if (mode_ == Mode::Pause) spr(art_.paused, 160.f, 44.f, float(art_.paused.h) * 2.f, PAL_BANNER);
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
        time_ += kDt;
        clock_ -= kDt;
        if (clock_ <= 0.f) {
            clock_ = 0.f;
            fail("crew");
        } else {
            step(kDt);
        }
    } else if (mode_ == Mode::Dock) {
        t_ += kDt;
        time_ += kDt;
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

}  // namespace keelplat

#include "game/pass.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace subpass {
namespace {
constexpr float GOAL = 4600.f;
constexpr float CLOCK_FRAMES = 60.f * 46.f;
constexpr float CRUISE = 2.05f;
constexpr float SURGE = 2.65f;
}  // namespace

float Game::rnd() {
    rng_ = rng_ * 1664525u + 1013904223u;
    return (rng_ >> 8) * (1.f / 16777216.f);
}

float Game::ceilingAt(float x) const {
    float s = std::sin(x * 0.011f) * 18.f + std::sin(x * 0.0037f + 0.8f) * 14.f;
    return 48.f + s;
}

float Game::floorAt(float x) const {
    float s = std::sin(x * 0.010f + 0.6f) * 16.f + std::sin(x * 0.0031f) * 12.f;
    return 186.f - s;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.HUD.resize(64, 32);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.apu.setMaster(0.8f);
    mines_.clear();
    for (int i = 0; i < 26; i++) {
        float x = 360.f + float(i) * 160.f;
        float mid = (ceilingAt(x) + floorAt(x)) * 0.5f;
        float off = ((i % 5) - 2) * 12.f;
        mines_.push_back({x, mid + off});
    }
    fish_.clear();
    for (int i = 0; i < 8; i++) fish_.push_back({200.f + float(i) * 520.f, 90.f + float(i % 4) * 22.f, 0.6f + float(i % 3) * 0.25f, i});
}

void Game::beginRun() {
    mode_ = Mode::Run;
    px_ = 80;
    py_ = (ceilingAt(80) + floorAt(80)) * 0.5f;
    clock_ = 0;
    hull_ = 4;
    stun_ = 0;
    inv_ = 0;
    won_ = false;
    over_ = false;
    crewPct_ = 0;
}

void Game::hurt() {
    if (inv_ > 0) return;
    hull_--;
    inv_ = 50;
    stun_ = 22;
    sys_->apu.noiseBurst(0.45f, 700.f, 0.2f);
    if (hull_ <= 0) {
        mode_ = Mode::Lose;
        over_ = true;
        won_ = false;
    }
}

void Game::botSteer(float& want) const {
    float look = px_ + 70.f;
    float c = ceilingAt(look);
    float f = floorAt(look);
    want = (c + f) * 0.5f;
    for (const Mine& m : mines_) {
        float dx = m.x - px_;
        if (dx < -20.f || dx > 130.f) continue;
        if (want >= m.y) want = m.y + 28.f;
        else want = m.y - 28.f;
    }
    float c0 = ceilingAt(px_) + 18.f;
    float f0 = floorAt(px_) - 18.f;
    if (want < c0) want = c0;
    if (want > f0) want = f0;
}

void Game::updateRun() {
    const gs::Pad& pad = sys_->pad;
    bool surge = pad.down(gs::BTN_A) || pad.down(gs::BTN_RIGHT);
    bool brake = pad.down(gs::BTN_LEFT);
    float want = py_;
    if (bot_) {
        botSteer(want);
        surge = false;
        brake = false;
    } else {
        if (pad.down(gs::BTN_UP) || pad.axisY > 0.3f) want = py_ - 40.f;
        if (pad.down(gs::BTN_DOWN) || pad.axisY < -0.3f) want = py_ + 40.f;
    }
    float step = bot_ ? 2.4f : 1.85f;
    if (py_ < want) py_ = std::min(want, py_ + step);
    if (py_ > want) py_ = std::max(want, py_ - step);

    float spd = surge ? SURGE : CRUISE;
    if (brake) spd = 1.05f;
    if (stun_ > 0) {
        spd = 0.35f;
        stun_--;
    }
    if (inv_ > 0) inv_--;
    px_ += spd;
    clock_ += 1.f;
    crewPct_ = std::min(100, int(clock_ / CLOCK_FRAMES * 100.f));

    float c = ceilingAt(px_) + 14.f;
    float f = floorAt(px_) - 14.f;
    if (py_ < c) {
        py_ = c;
        hurt();
    }
    if (py_ > f) {
        py_ = f;
        hurt();
    }
    if (mode_ != Mode::Run) return;
    for (const Mine& m : mines_) {
        float dx = m.x - px_;
        float dy = m.y - py_;
        if (dx * dx + dy * dy < 18.f * 18.f) {
            hurt();
            if (mode_ != Mode::Run) return;
            py_ += (py_ < m.y) ? -10.f : 10.f;
        }
    }
    if (clock_ >= CLOCK_FRAMES) {
        mode_ = Mode::Lose;
        over_ = true;
        won_ = false;
        crewPct_ = 100;
        return;
    }
    if (px_ >= GOAL) {
        mode_ = Mode::Win;
        over_ = true;
        won_ = true;
        sys_->apu.tone(1, 523.f, 0.12f);
    }
}

void Game::hud(int col, int row, const std::string& s) {
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || row < 0 || row > 27 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], PAL_HUD));
    }
}

void Game::hudC(int row, const std::string& s) { hud(20 - int(s.size()) / 2, row, s); }

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 40 || s.x + s.w < -40 || s.y > gs::SCREEN_H + 20 || s.y + s.h < -40) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    float storm = clock_ / CLOCK_FRAMES;
    if (mode_ != Mode::Run && mode_ != Mode::Win && mode_ != Mode::Lose) storm = 0.15f;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = float(y) / float(gs::SCREEN_H - 1);
        int g = int((3.f + (1.f - u) * 5.f) * (1.f - storm * 0.55f));
        int b = int((5.f + (1.f - u) * 8.f) * (1.f - storm * 0.45f));
        if (y < 18) {
            g = std::max(0, g - 2);
            b = std::min(15, b + 2);
        }
        vdp.lineBackdrop[y] = gs::rgb4(0, std::clamp(g, 0, 15), std::clamp(b, 0, 15));
        vdp.lineFog[y] = uint8_t(storm > 0.72f && y < 28 ? 6 : 0);
    }

    auto worldX = [&](float x) { return x - px_ + 96.f; };

    if (mode_ == Mode::Run || mode_ == Mode::Win || mode_ == Mode::Lose || mode_ == Mode::Pause) {
        float gateX = worldX(GOAL);
        if (gateX > -40 && gateX < 360) {
            spr(art_.gate, gateX, ceilingAt(GOAL) + 8, 70, PAL_GATE, false, true);
            spr(art_.gate, gateX + 36, ceilingAt(GOAL) + 8, 70, PAL_GATE, true, true);
        }
        for (float x = std::floor((px_ - 120) / 40.f) * 40.f; x < px_ + 280.f; x += 40.f) {
            spr(art_.rock, worldX(x), ceilingAt(x) + 6, 34, PAL_ROCK, false, true);
            spr(art_.rock, worldX(x + 18), floorAt(x) - 4, 32, PAL_ROCK, true);
            if (int(x) % 80 == 0) spr(art_.kelp, worldX(x + 8), floorAt(x) - 2, 36, PAL_KELP, false, true);
        }
        for (const Mine& m : mines_) {
            float sx = worldX(m.x);
            if (sx < -30 || sx > 350) continue;
            spr(art_.mine, sx, m.y, 22, PAL_MINE);
        }
        for (const Fish& f : fish_) {
            float sx = worldX(f.x);
            if (sx < -20 || sx > 340) continue;
            spr(art_.fish, sx, f.y, 12, PAL_FISH, f.kind & 1);
        }
        int bubbles = 4;
        for (int i = 0; i < bubbles; i++) {
            float by = py_ - 8 - std::fmod(t_ * 18.f + i * 11.f, 36.f);
            spr(art_.bubble, 70.f - i * 6.f, by, 6 + (i & 1) * 2, PAL_FX);
        }
        bool blink = inv_ > 0 && (int(t_ * 12) & 1);
        if (!blink) spr(art_.sub, 96, py_, 28, PAL_SUB);
        float crewX = 28.f + (clock_ / CLOCK_FRAMES) * 250.f;
        spr(art_.crew, crewX, 14, 12, PAL_CREW);
    }

    if (mode_ == Mode::Title) {
        spr(art_.banner, 160, 70, 28, PAL_HUD);
        spr(art_.sub, 160, 120, 40, PAL_SUB);
        hudC(16, "TAKE THE SUB");
        hudC(18, "CLEAR THE PASS");
        hudC(20, "BEFORE THE OTHER CREW");
        hudC(23, "UP DOWN DEPTH   A SURGE");
        hudC(25, "PRESS START");
        hud(1, 26, S3_VERSION_STRING);
    } else {
        char buf[48];
        std::snprintf(buf, sizeof buf, "HULL %d", std::max(0, hull_));
        hud(1, 1, buf);
        std::snprintf(buf, sizeof buf, "CREW %d%%", crewPct_);
        hud(28, 1, buf);
        hud(12, 0, "OTHER CREW");
        int dist = std::max(0, int((GOAL - px_) / 40.f));
        std::snprintf(buf, sizeof buf, "PASS %d", dist);
        hud(15, 26, buf);
        if (mode_ == Mode::Win) {
            hudC(12, "PASS CLEAR");
            hudC(14, "AHEAD OF THE CREW");
        } else if (mode_ == Mode::Lose) {
            hudC(12, hull_ <= 0 ? "HULL BREACH" : "STORM CLOSED");
            hudC(14, "THE OTHER CREW");
        } else if (mode_ == Mode::Pause) {
            hudC(13, "PAUSED");
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += 1.f / 60.f;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (bot_ && sys.frame > 20) beginRun();
        else if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) beginRun();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = held_;
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            held_ = Mode::Run;
            mode_ = Mode::Pause;
        } else updateRun();
    } else if ((mode_ == Mode::Win || mode_ == Mode::Lose) && !bot_) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) {
            mode_ = Mode::Title;
            over_ = false;
        }
    }

    if (mode_ == Mode::Run) {
        for (Fish& f : fish_) {
            f.x -= f.v;
            if (f.x < px_ - 400.f) f.x = px_ + 360.f;
        }
        sonar_ -= 1.f / 60.f;
        if (sonar_ <= 0) {
            sys.apu.tone(0, 640.f, 0.05f);
            sonar_ = 0.7f;
        }
    } else if (sonar_ > 0) {
        sonar_ -= 1.f / 60.f;
        if (sonar_ <= 0) sys.apu.tone(0, 0, 0);
    }
    draw();
}

}  // namespace subpass

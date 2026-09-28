#include "game/scull.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace scullslip {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;

constexpr float kTide = 38.f;
constexpr float kMouth = 82.f;
constexpr float kHead = 164.f;
constexpr float kPierIn = 7.2f;
constexpr float kBerthY0 = 118.f;
constexpr float kBerthY1 = 138.f;
constexpr float kBow = 9.4f;
constexpr float kStern = 7.6f;
constexpr float kBeam = 1.25f;
constexpr float kStop = 0.28f;
constexpr float kHoldNeed = 0.48f;
constexpr float kScrape = 3.4f;

constexpr float kPlankY[] = {90.f, 108.f, 126.f, 144.f, 158.f};
constexpr float kPileY[] = {86.f, 112.f, 136.f, 160.f};
constexpr float kReed[][2] = {{-22.f, 168.f}, {-10.f, 176.f}, {8.f, 172.f}, {20.f, 180.f}, {-16.f, 188.f},
                              {14.f, 190.f}};

float wrapPi(float a) {
    while (a > kPi) a -= kTau;
    while (a < -kPi) a += kTau;
    return a;
}

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

float Game::tideLeft() const { return std::max(0.f, kTide - you_); }

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (inEnd_) return 3;
    if (inSlip_) return 2;
    return 1;
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.08f);
    tone_ = 0.07f;
}

void Game::finish(bool win, const char* why) {
    if (over_) return;
    won_ = win;
    over_ = true;
    mode_ = win ? Mode::Win : Mode::Fail;
    std::snprintf(why_, sizeof(why_), "%s", why);
    fanStep_ = 0;
    speed_ = 0.f;
    if (!win) {
        sys_->apu.noiseBurst(0.35f, 90.f, 0.4f);
        blip(86.f);
    } else {
        blip(523.f);
    }
}

void Game::begin() {
    x_ = -12.f;
    y_ = 16.f;
    heading_ = 0.42f;
    speed_ = 0.f;
    stroke_ = 0.f;
    you_ = 0.f;
    hold_ = 0.f;
    won_ = false;
    over_ = false;
    inSlip_ = false;
    inEnd_ = false;
    fanStep_ = -1;
    foamN_ = 0;
    why_[0] = 0;
    camX_ = x_;
    camY_ = y_ + 8.f;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = true;
    sys.vdp.B.resize(64, 32);
    for (int y = 0; y < sys.vdp.B.h; y++)
        for (int x = 0; x < sys.vdp.B.w; x++) sys.vdp.B.set(x, y, gs::entry(art_.waterTile, PAL_WATER));
    sys.vdp.setFogColor(gs::rgb4(2, 5, 8));
    sys.apu.setMaster(0.7f);
    sys.apu.setEcho(0.08f, 0.16f, 0.06f);
    begin();
    if (bot_) {
        mode_ = Mode::Row;
        zoom_ = 2.15f;
    } else {
        mode_ = Mode::Title;
        zoom_ = 1.35f;
        camX_ = 0.f;
        camY_ = 110.f;
    }
}

void Game::controls(float& row, float& steer) {
    const gs::Pad& p = sys_->pad;
    steer = 0.f;
    row = 0.f;
    if (p.down(gs::BTN_LEFT) || p.axisX < -0.28f) steer -= 1.f;
    if (p.down(gs::BTN_RIGHT) || p.axisX > 0.28f) steer += 1.f;
    if (p.down(gs::BTN_A) || p.down(gs::BTN_C) || p.down(gs::BTN_UP) || p.accel > 0.2f) row += 1.f;
    if (p.down(gs::BTN_B) || p.down(gs::BTN_DOWN) || p.brake > 0.2f) row -= 1.f;
}

bool Game::sheltered() const {
    return std::fabs(x_) < kPierIn - 0.4f && y_ > kMouth + 2.f && y_ < kHead - 2.f;
}

void Game::pilot(float& row, float& steer) {
    const float tx = 0.f;
    float ty = 128.f;
    if (y_ < kMouth - 8.f) ty = kMouth - 6.f;
    float dx = tx - x_;
    float dy = ty - y_;
    float err = wrapPi(std::atan2(dx, dy) - heading_);
    if (y_ < kMouth - 4.f && (std::fabs(x_) > 1.6f || std::fabs(heading_) > 0.22f)) {
        err = wrapPi(std::atan2(-x_, std::max(6.f, (kMouth - 10.f) - y_)) - heading_);
    }
    steer = clampf(err * 2.6f, -1.f, 1.f);

    float want = 8.4f;
    if (std::fabs(err) > 0.55f) want = 2.4f;
    if (y_ > 48.f && y_ < kMouth && (std::fabs(x_) > 1.3f || std::fabs(heading_) > 0.18f)) want = 1.6f;
    if (y_ >= kMouth - 2.f && y_ < kBerthY0 - 8.f) want = 3.6f;
    if (y_ >= kBerthY0 - 8.f && y_ < 126.f) want = 1.5f;
    if (y_ >= 126.f) {
        steer = clampf(wrapPi(0.f - heading_) * 3.f - x_ * 0.35f, -1.f, 1.f);
        if (speed_ > kStop) row = -0.9f;
        else if (speed_ < -0.08f) row = 0.45f;
        else row = 0.f;
        return;
    }
    if (std::fabs(x_) > 5.f && y_ > kMouth) want = std::min(want, 2.f);
    row = clampf((want - speed_) * 0.9f, -1.f, 1.f);
}

void Game::physics(float dt, float row, float steer) {
    const bool in = sheltered();
    float drag = in ? 0.95f : 0.62f;
    float accel = 16.5f;
    float cap = in ? 6.2f : 11.5f;
    speed_ += row * accel * dt;
    speed_ -= speed_ * drag * dt;
    speed_ = clampf(speed_, -3.6f, cap);
    float turn = (in ? 1.35f : 1.7f) * (0.35f + std::min(std::fabs(speed_), 8.f) * 0.09f);
    heading_ = wrapPi(heading_ + steer * turn * dt);

    const float c = std::cos(heading_);
    const float s = std::sin(heading_);
    x_ += s * speed_ * dt;
    y_ += c * speed_ * dt;

    float u = clampf(you_ / kTide, 0.f, 1.f);
    if (!in) x_ += (0.35f + 3.1f * u * u) * dt;

    auto sample = [&](float along, float beam, float& px, float& py) {
        px = x_ + s * along + c * beam;
        py = y_ + c * along - s * beam;
    };
    float bowX, bowY, sternX, sternY;
    sample(kBow, 0.f, bowX, bowY);
    sample(-kStern, 0.f, sternX, sternY);

    bool scraped = false;
    float hit = std::fabs(speed_);
    auto wall = [&](float px, float py) {
        if (py < kMouth - 1.f || py > kHead + 2.f) return;
        float left = -kPierIn;
        float right = kPierIn;
        if (px < left && px > left - 6.f) {
            scraped = true;
            x_ += (left - px) + 0.15f;
        } else if (px > right && px < right + 6.f) {
            scraped = true;
            x_ -= (px - right) + 0.15f;
        }
    };
    wall(x_, y_);
    wall(bowX, bowY);
    wall(sternX, sternY);
    wall(x_ + c * kBeam, y_ - s * kBeam);
    wall(x_ - c * kBeam, y_ + s * kBeam);
    if (scraped) {
        if (hit > kScrape) {
            finish(false, "scraped the pier");
            return;
        }
        speed_ *= 0.55f;
    }

    if (y_ > kHead - 0.5f || bowY > kHead) {
        finish(false, "missed the end");
        return;
    }
    if (std::fabs(x_) > 58.f) {
        finish(false, "left the reach");
        return;
    }
    if (y_ < -6.f) y_ = -6.f;

    float errH = std::fabs(heading_);
    if (errH > kPi) errH = kTau - errH;
    bool whole = std::fabs(bowX) < kPierIn - 1.5f && std::fabs(sternX) < kPierIn - 1.5f && std::fabs(x_) < kPierIn - 1.6f;
    inSlip_ = std::fabs(x_) < kPierIn - 0.6f && y_ > kMouth + 1.f && y_ < kHead - 2.f;
    inEnd_ = whole && y_ >= kBerthY0 && y_ <= kBerthY1 && bowY < kHead - 4.f && sternY > kMouth + 4.f && errH < 0.42f;

    if (inEnd_ && std::fabs(speed_) <= kStop) hold_ += dt;
    else hold_ = 0.f;

    if (row > 0.2f) stroke_ += dt * (1.6f + std::fabs(speed_) * 0.12f);
    if (std::fabs(speed_) > 1.4f) {
        Foam& f = foam_[foamN_ % 14];
        f.x = x_ - s * 8.f;
        f.y = y_ - c * 8.f;
        f.life = 1.f;
        foamN_++;
    }
    for (auto& f : foam_) f.life -= dt * 0.9f;

    if (hold_ >= kHoldNeed && you_ < kTide) {
        finish(true, "berthed");
        return;
    }
    if (you_ >= kTide) finish(false, "tide turned");
}

void Game::audio(float dt, float row) {
    if (tone_ > 0.f) {
        tone_ -= dt;
        if (tone_ <= 0.f) sys_->apu.tone(0, 0, 0);
    }
    if (mode_ == Mode::Row && row > 0.2f) {
        float phase = stroke_ - std::floor(stroke_);
        if (phase < dt * 3.f) sys_->apu.tone(1, 140.f + std::fabs(speed_) * 6.f, 0.045f);
        else if (phase > 0.5f && phase < 0.5f + dt * 3.f) sys_->apu.tone(1, 0, 0);
    } else if (mode_ == Mode::Row) {
        sys_->apu.tone(1, 0, 0);
    }
    if (fanStep_ >= 0) {
        static const float winN[] = {392.f, 494.f, 587.f, 784.f};
        static const float loseN[] = {196.f, 164.f, 130.f};
        fanStep_++;
        if (won_) {
            if (fanStep_ % 10 == 1 && fanStep_ < 40) sys_->apu.tone(0, winN[fanStep_ / 10], 0.1f);
        } else if (fanStep_ % 12 == 1 && fanStep_ < 36) {
            sys_->apu.tone(0, loseN[std::min(fanStep_ / 12, 2)], 0.1f);
        }
        if (fanStep_ > 56) {
            fanStep_ = -1;
            sys_->apu.tone(0, 0, 0);
        }
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal) {
    if (h < 1.5f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 64 || s.x + s.w < -64 || s.y > gs::SCREEN_H + 64 || s.y + s.h < -64) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::worldToScreen(float wx, float wy, float& sx, float& sy) const {
    sx = (wx - camX_) * zoom_ + 160.f;
    sy = 112.f - (wy - camY_) * zoom_;
}

void Game::place(const gs::Mipped& m, float wx, float wy, float worldH, int pal) {
    float sx, sy;
    worldToScreen(wx, wy, sx, sy);
    spr(m, sx, sy, worldH * zoom_, pal);
}

int Game::shellFrame() const {
    float h = heading_;
    if (h < 0.f) h += kTau;
    int face = int(h / (kTau / 8.f) + 0.5f) & 7;
    int phase = int(stroke_ * 2.f) & 1;
    return face * 2 + phase;
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = 0;
    while (s[n]) n++;
    hud(20 - n / 2, row, s, pal);
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    int scrollX = int(std::lround(camX_ * zoom_));
    int scrollY = int(std::lround(-camY_ * zoom_ + t_ * 3.f));
    vdp.B.scroll(scrollX, scrollY);
    float u = mode_ == Mode::Title ? 0.15f : clampf(you_ / kTide, 0.f, 1.f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        vdp.lineBackdrop[y] = y < 22 ? gs::rgb4(6, 8, 11) : gs::rgb4(1, 3 + int((1.f - u) * 2), 8);
        vdp.road[y].on = false;
        vdp.lineFog[y] = 0;
    }

    float showX = x_, showY = y_;
    if (mode_ == Mode::Title) {
        showX = 0.f;
        showY = 100.f + std::sin(t_ * 1.2f) * 0.35f;
    }
    place(art_.shell[shellFrame()], showX, showY, 20.f, PAL_SHELL);

    float crewY = 20.f + (kMouth - 20.f) * u;
    place(art_.crew, 22.f + u * 6.f, crewY, 16.f, PAL_CREW);

    for (float py : kPlankY) {
        place(art_.plank, -11.6f, py, 7.2f, PAL_PIER);
        place(art_.plank, 11.6f, py, 7.2f, PAL_PIER);
    }
    for (float py : kPileY) {
        place(art_.pile, -kPierIn, py, 8.f, PAL_PILE);
        place(art_.pile, kPierIn, py, 8.f, PAL_PILE);
    }
    place(art_.mark, 0.f, (kBerthY0 + kBerthY1) * 0.5f, 5.5f, PAL_MARK);
    for (const auto& r : kReed) place(art_.reed, r[0], r[1], 8.f, PAL_REED);

    for (const Foam& f : foam_) {
        if (f.life <= 0.f) continue;
        place(art_.foam, f.x, f.y, 1.8f + f.life * 1.4f, PAL_FOAM);
    }

    char line[48];
    if (mode_ == Mode::Title) {
        hudC(3, "S3 SCULL SLIP", PAL_HUD);
        hudC(6, "BERTH IN THE SLIP", PAL_HUD);
        hudC(8, "BEFORE THE TIDE TURNS", PAL_HUD);
        hudC(10, "THE CLOCK IS THE OTHER CREW", PAL_HUD);
        hudC(18, "A ROW   ARROWS STEER", PAL_HUD);
        hudC(20, "B BACKS WATER", PAL_HUD);
        hudC(23, "START", 2);
    } else if (mode_ == Mode::Pause) {
        hudC(12, "PAUSED", PAL_HUD);
        hudC(15, "START", PAL_HUD);
    } else {
        std::snprintf(line, sizeof(line), "TIDE %4.1f", tideLeft());
        hud(1, 1, line, u > 0.7f ? 4 : PAL_HUD);
        std::snprintf(line, sizeof(line), "SPD %4.1f", std::fabs(speed_));
        hud(28, 1, line, PAL_HUD);
        if (inEnd_) hud(1, 2, "BERTH", 3);
        else if (inSlip_) hud(1, 2, "SLIP", 2);
        else hud(1, 2, "REACH", 5);
        if (mode_ == Mode::Win) {
            hudC(10, "BERTHED", 3);
            hudC(12, "AHEAD OF THE TIDE", PAL_HUD);
        } else if (mode_ == Mode::Fail) {
            hudC(10, "LEG LOST", 4);
            hudC(12, why_, 4);
            hudC(16, "START", PAL_HUD);
        } else if (inEnd_) {
            hudC(24, "HOLD THE BERTH", 2);
        } else if (inSlip_) {
            hudC(24, "THE END IS THE HEAD", PAL_HUD);
        } else {
            hudC(24, "LINE THE BOW ON THE SLIP", PAL_HUD);
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    const gs::Pad& p = sys.pad;
    float row = 0.f;

    if (mode_ == Mode::Title) {
        camX_ += (0.f - camX_) * 0.05f;
        camY_ += (108.f - camY_) * 0.05f;
        zoom_ += (1.45f - zoom_) * 0.04f;
        stroke_ += kDt * 0.7f;
        if (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A) || bot_) {
            begin();
            mode_ = Mode::Row;
            blip(440.f);
        }
    } else if (mode_ == Mode::Pause) {
        if (p.pressed(gs::BTN_START)) {
            mode_ = Mode::Row;
            blip(330.f);
        }
    } else if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        camX_ += (x_ - camX_) * 0.08f;
        camY_ += (y_ - camY_) * 0.08f;
        if (!bot_ && p.pressed(gs::BTN_START)) {
            begin();
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
        }
    } else {
        if (!bot_ && p.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        float steer = 0.f;
        if (bot_) pilot(row, steer);
        else controls(row, steer);
        you_ += kDt;
        if (!over_) physics(kDt, row, steer);
        float lead = inSlip_ ? 2.f : 8.f;
        camX_ += (x_ - camX_) * 0.1f;
        camY_ += ((y_ + lead) - camY_) * 0.1f;
        float wantZ = inEnd_ ? 2.7f : (inSlip_ ? 2.35f : 2.05f);
        zoom_ += (wantZ - zoom_) * 0.06f;
    }
    audio(kDt, mode_ == Mode::Row ? row : 0.f);
    draw();
}

}  // namespace scullslip

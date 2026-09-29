#include "game/van.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace vangrass {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPpm = 4.6f;
constexpr float kStop = 0.18f;
constexpr float kHoldNeed = 0.62f;
constexpr float kShortNeed = 2.2f;
constexpr float kAhead = 9.2f;
constexpr float kAstern = 8.4f;
constexpr float kDrag = 0.85f;
constexpr float kVanY = 132.f;
constexpr float kRoadY = 158.f;

float clampf(float v, float a, float b) { return v < a ? a : v > b ? b : v; }

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (hold_ > 0.06f) return 3;
    if (full_ || x_ + kHalf > kGrassL) return 2;
    return 1;
}

bool Game::hullOnGrass() const {
    float rear = x_ - kHalf;
    float nose = x_ + kHalf;
    return rear >= kGrassL && nose <= kGrassR;
}

void Game::begin() {
    x_ = 28.f;
    vel_ = 0.f;
    hold_ = 0.f;
    shortT_ = 0.f;
    race_ = 0.f;
    phase_ = 0;
    wheel_ = 0.f;
    won_ = false;
    over_ = false;
    full_ = false;
    announced_ = false;
    chimeN_ = chimeStep_ = 0;
    smokeCursor_ = 0;
    tone0_ = chimeT_ = thumpT_ = 0.f;
    smokeT_ = shake_ = 0.f;
    why_[0] = 0;
    for (Puff& p : smoke_) p = {};
    limit_ = 46.f;
}

void Game::showTitle() {
    begin();
    mode_ = Mode::Title;
    camX_ = 92.f;
    x_ = 54.f;
}

void Game::startRun() {
    begin();
    mode_ = Mode::Run;
    camX_ = x_;
    blip(420.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = true;
    sys.vdp.B.resize(64, 32);
    for (int y = 0; y < sys.vdp.B.h; y++)
        for (int x = 0; x < sys.vdp.B.w; x++) sys.vdp.B.set(x, y, gs::entry(art_.skyTile, PAL_SKY));
    sys.vdp.setFogColor(gs::rgb4(5, 7, 4));
    sys.apu.setMaster(0.74f);
    sys.apu.setEcho(0.07f, 0.11f, 0.05f);
    t_ = 0.f;
    if (bot_) startRun();
    else showTitle();
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.05f);
    tone0_ = 0.10f;
}

void Game::chime(int notes) {
    chimeN_ = std::max(1, std::min(notes, 6));
    chimeStep_ = 0;
    chimeT_ = 0.02f;
}

void Game::controls(float& thrust) {
    const gs::Pad& p = sys_->pad;
    thrust = 0.f;
    if (p.down(gs::BTN_RIGHT) || p.down(gs::BTN_UP) || p.down(gs::BTN_A) || p.down(gs::BTN_C)) thrust += 1.f;
    if (p.down(gs::BTN_LEFT) || p.down(gs::BTN_DOWN) || p.down(gs::BTN_B)) thrust -= 1.f;
    if (p.accel > 0.12f) thrust = std::max(thrust, p.accel);
    if (p.brake > 0.12f) thrust = std::min(thrust, -p.brake);
    if (std::fabs(p.axisX) > 0.2f) thrust = clampf(p.axisX, -1.f, 1.f);
    thrust = clampf(thrust, -1.f, 1.f);
}

void Game::pilot(float& thrust) {
    float dx = kGoal - x_;
    float ad = std::fabs(dx);
    float sign = dx >= 0.f ? 1.f : -1.f;
    float want;
    if (!hullOnGrass()) {
        phase_ = x_ + kHalf > kGrassL - 6.f ? 1 : 0;
        if (ad > 36.f) want = 6.2f * sign;
        else if (ad > 16.f) want = 2.6f * sign;
        else want = clampf(dx * 0.28f, -1.1f, 1.1f);
    } else {
        phase_ = 2;
        want = ad < 2.5f ? 0.f : clampf(dx * 0.45f, -1.6f, 1.6f);
    }
    thrust = clampf((want - vel_) * 3.4f, -1.f, 1.f);
    if (hullOnGrass() && ad < 2.5f && std::fabs(vel_) < kStop) thrust = 0.f;
}

void Game::win() {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    vel_ = 0.f;
    std::snprintf(why_, sizeof why_, "full stop");
    chime(4);
    sys_->rumble(0.26f, 0.10f, 140);
    sys_->setLight(40, 170, 60);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    std::snprintf(why_, sizeof why_, "%s", why);
    shake_ = 0.75f;
    sys_->rumble(0.5f, 0.2f, 160);
    sys_->setLight(180, 36, 24);
    sys_->apu.noiseBurst(0.34f, 100.f, 0.30f);
    sys_->apu.tone(0, 70.f, 0.05f);
    tone0_ = 0.34f;
}

void Game::physics(float thrust) {
    float accel = (thrust >= 0.f ? thrust * kAhead : thrust * kAstern) - vel_ * kDrag;
    vel_ += accel * kDt;
    vel_ = clampf(vel_, -4.5f, 7.2f);
    if (std::fabs(vel_) < 0.03f && std::fabs(thrust) < 0.04f) vel_ = 0.f;
    x_ += vel_ * kDt;
    wheel_ += std::fabs(vel_) * kDt;
    if (!std::isfinite(x_) || !std::isfinite(vel_)) {
        fail("lost the street");
        return;
    }
    float rear = x_ - kHalf;
    if (rear < kWest) {
        x_ += kWest - rear;
        vel_ *= -0.12f;
        if (thumpT_ <= 0.f) {
            thumpT_ = 0.24f;
            shake_ = std::max(shake_, 0.4f);
            sys_->apu.noiseBurst(0.16f, 150.f, 0.08f);
        }
    }
    if (std::fabs(vel_) > 2.2f) {
        smokeT_ -= kDt;
        if (smokeT_ <= 0.f) {
            smokeT_ = 0.08f;
            Puff& p = smoke_[smokeCursor_];
            p.x = x_ - 7.f;
            p.life = 0.7f;
            p.rise = 0.f;
            smokeCursor_ = (smokeCursor_ + 1) % 8;
        }
    }
    for (Puff& p : smoke_) {
        if (p.life > 0.f) {
            p.life -= kDt;
            p.rise += kDt * 8.f;
        }
    }
}

void Game::judge() {
    if (mode_ != Mode::Run) return;
    full_ = hullOnGrass();
    float nose = x_ + kHalf;
    if (nose > kGrassR + 0.05f) {
        fail("missed the end");
        return;
    }
    if (full_ && std::fabs(vel_) <= kStop) hold_ += kDt;
    else hold_ = 0.f;
    if (hold_ >= kHoldNeed) {
        win();
        return;
    }
    bool near = nose > kGrassL - 4.f && !full_ && std::fabs(vel_) < 0.35f;
    if (near) shortT_ += kDt;
    else shortT_ = 0.f;
    if (shortT_ >= kShortNeed) {
        fail("short of the grass");
        return;
    }
    if (race_ >= limit_) fail("the leg ran out");
}

void Game::audio() {
    if (tone0_ > 0.f) {
        tone0_ -= kDt;
        if (tone0_ <= 0.f) sys_->apu.tone(0, 0, 0);
    }
    if (thumpT_ > 0.f) thumpT_ -= kDt;
    if (mode_ == Mode::Run && std::fabs(vel_) > 0.4f) {
        float hum = 90.f + std::fabs(vel_) * 18.f;
        sys_->apu.tone(1, hum, 0.03f);
    } else {
        sys_->apu.tone(1, 0, 0);
    }
    if (chimeN_ > 0) {
        chimeT_ -= kDt;
        if (chimeT_ <= 0.f) {
            static const float notes[] = {523.f, 659.f, 784.f, 1046.f};
            int n = chimeStep_;
            if (n < chimeN_ && n < 4) sys_->apu.tone(0, notes[n], 0.07f);
            tone0_ = 0.12f;
            chimeStep_++;
            chimeT_ = 0.14f;
            if (chimeStep_ >= chimeN_) chimeN_ = 0;
        }
    }
}

float Game::sx(float wx) const { return (wx - camX_) * kPpm + 160.f + shake_ * std::sin(t_ * 40.f) * 3.f; }

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal) {
    if (h < 1.5f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 48 || s.x + s.w < -48 || s.y > gs::SCREEN_H + 40 || s.y + s.h < -40) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
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
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = float(y) / float(gs::SCREEN_H);
        vdp.lineBackdrop[y] = u < 0.62f ? gs::rgb4(6, 9, 13) : gs::rgb4(4, 8, 3);
        vdp.road[y].on = false;
        vdp.lineFog[y] = 0;
    }
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 1.6f);

    auto at = [&](const gs::Mipped& m, float wx, float sy, float h, int pal) { spr(m, sx(wx), sy, h, pal); };

    for (float wx = 0.f; wx < 230.f; wx += 7.2f) at(art_.road, wx, kRoadY, 22.f, PAL_ROAD);
    for (float wx = kGrassL + 3.f; wx < kGrassR - 2.f; wx += 6.4f) at(art_.sod, wx, kRoadY - 6.f, 18.f, PAL_GRASS);
    for (int i = 0; i < 7; i++) {
        float wx = kGrassL + 4.f + i * 6.6f;
        at(art_.tuft, wx, kRoadY - 16.f, 12.f, PAL_GRASS);
    }
    for (float wx = 16.f; wx < kGrassL - 8.f; wx += 22.f) at(art_.dash, wx, kRoadY - 2.f, 6.f, PAL_ROAD);
    at(art_.house, kGrassL - 16.f, kRoadY - 28.f, 36.f, PAL_HOUSE);
    at(art_.box, kGrassL + 2.f, kRoadY - 18.f, 20.f, PAL_POST);
    at(art_.lamp, 70.f, kRoadY - 26.f, 32.f, PAL_POST);
    at(art_.endpost, kGrassR, kRoadY - 22.f, 28.f, PAL_END);
    at(art_.endpost, kGrassL, kRoadY - 18.f, 22.f, PAL_END);
    for (float wx = kGrassR + 6.f; wx < kGrassR + 28.f; wx += 6.f) at(art_.drop, wx, kRoadY + 2.f, 16.f, PAL_ROAD);

    float bob = std::sin(t_ * 2.f) * (mode_ == Mode::Title ? 1.2f : 0.f);
    at(art_.van, x_, kVanY + bob, 42.f, PAL_VAN);

    for (Puff& p : smoke_) {
        if (p.life > 0.f) at(art_.smoke, p.x, kVanY + 8.f - p.rise, 6.f + (1.f - p.life) * 6.f, PAL_SMOKE);
    }

    char line[48];
    if (mode_ == Mode::Title) {
        hudC(3, "S3 MAIL VAN GRASS", PAL_HUD);
        hudC(6, "LAND ON THE GRASS", PAL_HUD);
        hudC(8, "COME TO A FULL STOP", PAL_HUD);
        hudC(11, "MISSING THE END FAILS THE LEG", PAL_HUD);
        hudC(18, "A GAS   B BRAKE", PAL_HUD);
        hudC(21, "START", PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        hudC(12, "PAUSED", PAL_HUD);
        hudC(15, "START CONTINUES", PAL_HUD);
    } else {
        int sec = int(race_);
        int cs = int(race_ * 100.f) % 100;
        std::snprintf(line, sizeof line, "LEG %d:%02d.%02d", sec / 60, sec % 60, cs);
        hud(1, 1, line, PAL_HUD);
        int left = int(std::ceil(limit_ - race_));
        if (left < 0) left = 0;
        std::snprintf(line, sizeof line, "LEFT %d", left);
        hud(30, 1, line, PAL_HUD);
        if (mode_ == Mode::Run) {
            if (hold_ > 0.02f) hudC(25, "HOLD THE FULL STOP", PAL_HUD);
            else if (full_) hudC(25, "ON THE GRASS  STOP", PAL_HUD);
            else if (x_ + kHalf > kGrassL) hudC(25, "THE WHOLE VAN ON THE GRASS", PAL_HUD);
            else hudC(25, "THE GRASS IS THE END", PAL_HUD);
        } else if (mode_ == Mode::Win) {
            hudC(11, "STOPPED", PAL_HUD);
            hudC(13, "ON THE GRASS", PAL_HUD);
            if (!bot_) hudC(16, "START RUNS THE LEG AGAIN", PAL_HUD);
        } else if (mode_ == Mode::Fail) {
            hudC(11, why_[0] ? why_ : "FAILED", PAL_HUD);
            hudC(13, "THE LEG IS LOST", PAL_HUD);
            if (!bot_) hudC(16, "START RUNS THE LEG AGAIN", PAL_HUD);
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    const gs::Pad& pad = sys.pad;
    float thrust = 0.f;

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) {
            startRun();
        }
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(220.f);
        } else {
            if (bot_) pilot(thrust);
            else controls(thrust);
            race_ += kDt;
            physics(thrust);
            if (mode_ == Mode::Run) {
                if (full_ && !announced_ && hullOnGrass()) {
                    announced_ = true;
                    blip(680.f);
                }
                judge();
            }
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) mode_ = Mode::Run;
    } else if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C))) {
        startRun();
    }

    if (mode_ == Mode::Title) {
        camX_ = 100.f + std::sin(t_ * 0.18f) * 8.f;
    } else {
        float look = x_ + vel_ * 1.4f;
        float k = 1.f - std::exp(-kDt * 5.f);
        camX_ += (look - camX_) * k;
    }
    audio();
    draw();
}

}  // namespace vangrass

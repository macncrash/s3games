#include "game/metro.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace metroturn {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kBendZ[3] = {210.f, 500.f, 800.f};
constexpr float kBendA[3] = {-0.021f, 0.023f, -0.022f};
constexpr float kBendS[3] = {44.f, 42.f, 46.f};
constexpr float kFinish = 940.f;
constexpr float kBankCancel = 6.15f;
constexpr float kRollScale = 5.5f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

float Game::curveAt(float z) const {
    float k = 0.f;
    for (int i = 0; i < 3; i++) {
        float d = (z - kBendZ[i]) / kBendS[i];
        k += kBendA[i] * std::exp(-d * d);
    }
    return k;
}

void Game::buildTrack() {
    float x = 0.f, h = 0.f;
    for (int i = 0; i < kSamples; i++) {
        x_[i] = x;
        float z = i * kStep;
        h += curveAt(z) * kStep;
        x += std::sin(h) * kStep;
    }
}

float Game::trackX(float z) const {
    if (z < 0.f) z = 0.f;
    float u = z / kStep;
    int i = int(u);
    if (i >= kSamples - 1) return x_[kSamples - 1];
    float f = u - float(i);
    return x_[i] * (1.f - f) + x_[i + 1] * f;
}

const char* Game::tipWhy() const {
    if (turns_ <= 0) return "tipped before the first turn";
    if (turns_ == 1) return "tipped on the second turn";
    if (turns_ == 2) return "tipped on the third turn";
    return "tipped after the turns";
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    if (bot_) startRun();
    else showTitle();
}

void Game::showTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    turns_ = 0;
    made_[0] = made_[1] = made_[2] = false;
    why_ = "";
    race_ = 0;
    t_ = 0;
    z_ = 40.f;
    speed_ = 0;
    bank_ = 0;
    roll_ = 0;
    rollVel_ = 0;
    buildTrack();
    if (sys_) sys_->apu.tone(0, 0, 0);
}

void Game::startRun() {
    mode_ = Mode::Run;
    over_ = false;
    won_ = false;
    turns_ = 0;
    made_[0] = made_[1] = made_[2] = false;
    why_ = "";
    race_ = 0;
    t_ = 0;
    z_ = 16.f;
    speed_ = 8.f;
    bank_ = 0;
    roll_ = 0;
    rollVel_ = 0;
    buildTrack();
}

void Game::succeed() {
    if (mode_ == Mode::Win || mode_ == Mode::Fail) return;
    won_ = true;
    over_ = true;
    why_ = "upright";
    mode_ = Mode::Win;
    speed_ = 0;
    if (sys_) sys_->apu.tone(1, 660.f, 0.16f);
}

void Game::fail(const char* why) {
    if (mode_ == Mode::Win || mode_ == Mode::Fail) return;
    won_ = false;
    over_ = true;
    why_ = why;
    mode_ = Mode::Fail;
    speed_ = 0;
    if (sys_) sys_->apu.tone(1, 80.f, 0.22f);
}

void Game::controls(float& bank, float& throttle, float& brake) {
    bank = 0;
    throttle = 0;
    brake = 0;
    if (bot_ || (sys_ && sys_->headless && !sys_->scripted)) {
        pilot(bank, throttle, brake);
        return;
    }
    const gs::Pad& p = sys_->pad;
    float ax = p.axisX;
    if (p.down(gs::BTN_LEFT)) ax -= 1.f;
    if (p.down(gs::BTN_RIGHT)) ax += 1.f;
    bank = clampf(ax, -1.f, 1.f);
    if (p.down(gs::BTN_C) || p.down(gs::BTN_A) || p.accel > 0.2f) throttle = 1.f;
    if (p.down(gs::BTN_B) || p.down(gs::BTN_X) || p.brake > 0.15f) brake = 1.f;
}

void Game::pilot(float& bank, float& throttle, float& brake) {
    float look = z_ + 26.f + speed_ * 0.9f;
    float ka = std::fabs(curveAt(look));
    float want = 19.5f;
    if (ka > 0.004f) want = 13.2f;
    if (ka > 0.012f) want = 11.4f;
    if (speed_ > want + 0.35f) brake = 1.f;
    else if (speed_ < want - 0.2f) throttle = 1.f;
    float lead = curveAt(z_ + 14.f + speed_ * 0.4f);
    float need = (speed_ * speed_ * lead) / kBankCancel;
    bank = clampf(need * 1.05f, -1.f, 1.f);
}

void Game::markTurns(float zPrev) {
    for (int i = 0; i < 3; i++) {
        if (made_[i]) continue;
        if (zPrev < kBendZ[i] && z_ >= kBendZ[i]) {
            made_[i] = true;
            turns_ = i + 1;
            if (sys_) sys_->apu.tone(2, 520.f + float(i) * 70.f, 0.1f);
        }
    }
}

void Game::physics(float bank, float throttle, float brake, float dt) {
    bank_ = bank;
    if (throttle > 0.f) speed_ += 7.4f * throttle * dt;
    if (brake > 0.f) speed_ = std::max(0.f, speed_ - 11.5f * brake * dt);
    speed_ -= speed_ * 0.08f * dt;
    speed_ = clampf(speed_, 0.f, 24.f);
    float zPrev = z_;
    z_ += speed_ * dt;

    float lat = speed_ * speed_ * curveAt(z_);
    float target = (lat - bank_ * kBankCancel) * kRollScale;
    rollVel_ += (target - roll_) * 11.f * dt;
    rollVel_ *= std::exp(-5.2f * dt);
    roll_ += rollVel_ * dt;

    markTurns(zPrev);
    if (std::fabs(roll_) > kTip) {
        fail(tipWhy());
        return;
    }
    if (turns_ >= 3 && z_ >= kFinish) succeed();
}

void Game::audio() {
    if (!sys_) return;
    if (mode_ != Mode::Run) {
        sys_->apu.tone(0, 0, 0);
        return;
    }
    float squeal = clampf((std::fabs(roll_) - 18.f) / 22.f, 0.f, 1.f);
    sys_->apu.tone(0, 48.f + speed_ * 3.2f, 0.04f + speed_ * 0.004f);
    sys_->apu.tone(3, 180.f + std::fabs(roll_) * 8.f, squeal * 0.08f);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!sys_ || !s) return;
    for (; *s && col < 40; ++s, ++col) {
        unsigned char c = static_cast<unsigned char>(*s);
        if (c < 32 || c > 127) c = ' ';
        sys_->vdp.HUD.set(col, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = 0;
    while (s[n]) n++;
    hud((40 - n) / 2, row, s, pal);
}

void Game::blit(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, bool flip) {
    gs::Sprite s;
    s.img = m.pick(h);
    s.w = int16_t(w);
    s.h = int16_t(h);
    s.x = int16_t(cx - w * 0.5f);
    s.y = int16_t(cy - h * 0.5f);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    vdp.A.clear();
    vdp.B.clear();
    vdp.hudEnabled = true;
    vdp.roadTime = int(t_ * 60.f);

    const int horizon = 78;
    float camX = trackX(z_);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int band = y < horizon ? (y * 2) / horizon : 0;
        vdp.lineBackdrop[y] = y < horizon ? gs::rgb4(1 + band, 1 + band, 2 + band) : gs::rgb4(2, 2, 3);
        vdp.lineFog[y] = y < horizon ? uint8_t(0) : uint8_t(0);
        gs::RoadLine& r = vdp.road[y];
        if (y <= horizon) {
            r.on = false;
            continue;
        }
        float n = float(y - horizon) / float(gs::SCREEN_H - 1 - horizon);
        float dist = 3.6f / std::max(0.04f, n);
        float wz = z_ + dist;
        float dx = trackX(wz) - camX;
        r.on = true;
        r.cx = 160.f - dx * (168.f / dist);
        r.hw = 2.35f * (168.f / dist);
        r.v = wz * 48.f;
        r.pal = PAL_ROAD;
        r.style = 1;
        r.band = (int(wz / 6.f) & 1) ? 1 : 0;
        r.left = gs::GROUND_DROP;
        r.right = gs::GROUND_DROP;
    }

    for (float lz = 20.f; lz < 1100.f; lz += 36.f) {
        float dist = lz - z_;
        if (dist < 4.f || dist > 78.f) continue;
        float f = 168.f / dist;
        float sx = 160.f + (trackX(lz) - camX) * f;
        float sy = float(horizon) + float(gs::SCREEN_H - horizon) * (3.6f / dist);
        float side = 3.4f * f;
        bool leftTurn = curveAt(lz) < -0.004f;
        bool rightTurn = curveAt(lz) > 0.004f;
        blit(art_.lamp, sx - side, sy - 18.f * (f / 40.f), 8.f * f / 8.f, 22.f * f / 10.f, PAL_LAMP, false);
        blit(art_.lamp, sx + side, sy - 18.f * (f / 40.f), 8.f * f / 8.f, 22.f * f / 10.f, PAL_LAMP, true);
        if (leftTurn) blit(art_.chev, sx - side * 0.72f, sy - 28.f, 18.f, 14.f, PAL_SIGN, true);
        if (rightTurn) blit(art_.chev, sx + side * 0.72f, sy - 28.f, 18.f, 14.f, PAL_SIGN, false);
    }

    float lean = roll_ * 1.15f;
    blit(art_.car, 168.f + lean * 0.45f, 158.f, 78.f, 34.f, PAL_TRAIN, false);
    blit(art_.car, 96.f + lean, 168.f, 92.f, 40.f, PAL_TRAIN, false);

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(4, "S3 METRO TURN", PAL_HUD);
        hudC(7, "THREE TURNS  DO NOT TIP", PAL_HUD);
        hudC(10, "LEFT RIGHT BANK THE CARS", PAL_HUD);
        hudC(12, "C POWER    B BRAKE", PAL_HUD);
        hudC(16, "ENTER TO ROLL", PAL_HUD);
        return;
    }

    std::snprintf(buf, sizeof buf, "TURNS %d/3", turns_);
    hud(1, 1, buf, PAL_HUD);
    std::snprintf(buf, sizeof buf, "SPD %02d", int(speed_));
    hud(30, 1, buf, PAL_HUD);
    int sec = int(race_);
    std::snprintf(buf, sizeof buf, "%d:%02d", sec / 60, sec % 60);
    hud(17, 1, buf, PAL_HUD);
    std::snprintf(buf, sizeof buf, "ROLL %+03.0f", roll_);
    hud(1, 26, buf, std::fabs(roll_) > 28.f ? PAL_HUD : PAL_HUD);

    int cells = int(clampf(roll_ / kTip, -1.f, 1.f) * 8.f);
    char bar[20];
    for (int i = 0; i < 17; i++) bar[i] = (i == 8) ? '|' : ((i - 8) == cells || (cells > 0 && i > 8 && i - 8 < cells) ||
                                                            (cells < 0 && i < 8 && i - 8 > cells))
                                                       ? '#'
                                                       : '.';
    bar[17] = 0;
    hud(11, 26, bar, PAL_HUD);

    if (mode_ == Mode::Pause) {
        hudC(10, "PAUSED", PAL_HUD);
        hudC(12, "ENTER RUN", PAL_HUD);
    } else if (mode_ == Mode::Win) {
        hudC(9, "UPRIGHT", PAL_HUD);
        hudC(11, "THREE TURNS MADE", PAL_HUD);
        hudC(14, "ENTER RIDES AGAIN", PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        hudC(9, "TIPPED", PAL_HUD);
        hudC(11, why_ && why_[0] ? why_ : "THE CARS WENT OVER", PAL_HUD);
        hudC(14, "ENTER RIDES AGAIN", PAL_HUD);
    } else if (std::fabs(curveAt(z_ + 30.f)) > 0.008f && std::fabs(curveAt(z_)) < 0.006f) {
        hudC(10, curveAt(z_ + 40.f) < 0.f ? "BEND LEFT" : "BEND RIGHT", PAL_HUD);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    if (bot_ && mode_ == Mode::Title) startRun();

    if (mode_ == Mode::Title) {
        if (sys.pad.pressed(gs::BTN_START)) startRun();
        z_ = 48.f + std::sin(t_ * 0.4f) * 6.f;
        audio();
        draw();
        return;
    }
    if (mode_ == Mode::Pause) {
        if (sys.pad.pressed(gs::BTN_START)) mode_ = Mode::Run;
        audio();
        draw();
        return;
    }
    if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        if (!bot_ && sys.pad.pressed(gs::BTN_START)) startRun();
        audio();
        draw();
        return;
    }
    if (!bot_ && sys.pad.pressed(gs::BTN_START)) {
        mode_ = Mode::Pause;
        audio();
        draw();
        return;
    }

    float bank, throttle, brake;
    controls(bank, throttle, brake);
    physics(bank, throttle, brake, kDt);
    if (mode_ == Mode::Run) race_ += kDt;
    audio();
    draw();
}

}  // namespace metroturn

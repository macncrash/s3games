#include "game/grass.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace grass {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float PI = 3.14159265f;
constexpr float STRIP_LEN = 400.f;
constexpr float STRIP_HALF = 16.f;
constexpr float STALL = 16.5f;
constexpr float ROTATE = 21.f;
constexpr float MAX_PITCH = 0.32f;
constexpr float THRUST = 5.4f;
constexpr float DRAG_Q = 0.0035f;
constexpr float DRAG_L = 0.02f;
constexpr float FOCAL = 210.f;
constexpr float HORIZON = 96.f;
constexpr float CAM_BACK = 12.5f;
constexpr float CAM_EYE = 3.5f;

struct Gate {
    float x, z, h, spd;
};

// Left-hand circuit: climb past the far hedge, crosswind, downwind, base, final.
constexpr Gate GATES[] = {
    {0.f, 170.f, 30.f, 32.f},
    {0.f, 470.f, 54.f, 33.f},
    {-52.f, 490.f, 54.f, 32.f},
    {-96.f, 360.f, 50.f, 30.f},
    {-96.f, 190.f, 34.f, 28.f},
    {-96.f, 24.f, 20.f, 26.f},
    {-10.f, 52.f, 13.f, 25.f},
};
constexpr int NGATES = int(sizeof(GATES) / sizeof(GATES[0]));

float wrap(float a) {
    while (a > PI) a -= 2.f * PI;
    while (a < -PI) a += 2.f * PI;
    return a;
}

float predictVh(float pitch, float speed) {
    if (speed < STALL) return -9.f - (STALL - speed) * 1.1f;
    return pitch * std::max(speed, 12.f) * 1.15f + (speed - 30.f) * 0.22f;
}

float pitchFor(float wantVh, float speed) {
    float s = std::max(speed, 12.f);
    return (wantVh - (speed - 30.f) * 0.22f) / (s * 1.15f);
}

float throttleFor(float want, float speed) {
    float drag = DRAG_Q * want * want + DRAG_L * want;
    return std::clamp(drag / THRUST + (want - speed) * 0.16f, 0.f, 1.f);
}

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

gs::FMPatch enginePatch() {
    gs::FMPatch p;
    p.alg = 4;
    p.vol = 0.16f;
    p.drive = 0.18f;
    p.tone = 900.f;
    p.glide = 0.02f;
    p.op[0] = {1.f, 1.f, 0.02f, 0.18f, 0.85f, 0.2f};
    p.op[1] = {2.f, 0.35f, 0.04f, 0.22f, 0.55f, 0.25f};
    p.op[2] = {0.5f, 0.22f, 0.03f, 0.3f, 0.4f, 0.3f};
    p.op[3] = {1.f, 0.12f, 0.02f, 0.2f, 0.3f, 0.25f};
    return p;
}

gs::FMPatch tonePatch() {
    gs::FMPatch p;
    p.alg = 7;
    p.vol = 0.2f;
    p.tone = 1800.f;
    p.op[0] = {1.f, 1.f, 0.01f, 0.16f, 0.7f, 0.18f};
    p.op[1] = {2.f, 0.12f, 0.02f, 0.2f, 0.3f, 0.2f};
    p.op[2] = {3.f, 0.04f, 0.02f, 0.2f, 0.2f, 0.2f};
    p.op[3] = {1.f, 0.f, 0.02f, 0.2f, 0.2f, 0.2f};
    return p;
}

}  // namespace

int Game::circuit() const {
    if (won_) return 3;
    return std::clamp(circuitsDone_ + 1, 1, 3);
}

void Game::blip(bool high) {
    sys_->apu.tone(1, high ? 820.f : 390.f, 0.055f);
    beep_ = 0.07f;
}

void Game::buildWorld() {
    props_.clear();
    birds_.clear();
    clouds_.clear();
    for (float z = -20.f; z < STRIP_LEN + 140.f; z += 34.f) {
        props_.push_back({34.f, z, ((int)z / 34) & 1 ? Kind::TreeB : Kind::TreeA});
        props_.push_back({-38.f, z + 12.f, ((int)z / 34) & 1 ? Kind::TreeA : Kind::TreeB});
    }
    for (float z = 0.f; z <= STRIP_LEN; z += 48.f) {
        props_.push_back({STRIP_HALF + 1.5f, z, Kind::Post});
        props_.push_back({-STRIP_HALF - 1.5f, z, Kind::Post});
    }
    props_.push_back({0.f, 8.f, Kind::Bar});
    props_.push_back({0.f, STRIP_LEN - 10.f, Kind::Bar});
    props_.push_back({-24.f, 20.f, Kind::Sock});
    props_.push_back({58.f, 230.f, Kind::Hangar});
    props_.push_back({22.f, 155.f, Kind::Cone});
    props_.push_back({-22.f, 155.f, Kind::Cone});
    birds_.push_back({-30.f, 260.f, 18.f, 0.4f});
    birds_.push_back({18.f, 340.f, 26.f, 1.7f});
    birds_.push_back({-70.f, 140.f, 22.f, 2.6f});
    birds_.push_back({40.f, 80.f, 16.f, 0.9f});
    clouds_.push_back({-80.f, 300.f, 120.f});
    clouds_.push_back({60.f, 520.f, 140.f});
    clouds_.push_back({-150.f, 80.f, 110.f});
    clouds_.push_back({30.f, -60.f, 130.f});
    clouds_.push_back({120.f, 200.f, 150.f});
}

void Game::place() {
    x_ = 0.f;
    z_ = 18.f;
    h_ = 0.f;
    v_ = 0.f;
    vh_ = 0.f;
    heading_ = 0.f;
    pitch_ = 0.f;
    bank_ = 0.f;
    throttle_ = 0.f;
    onGround_ = true;
    braking_ = false;
    sawDownwind_ = false;
    sawDepart_ = false;
    maxH_ = 0.f;
    gate_ = 0;
    rollout_ = Roll::None;
    airT_ = 0.f;
    shake_ = 0.f;
    puffT_ = 0.f;
    puffs_.clear();
    banner_[0] = 0;
    bannerT_ = 0.f;
}

void Game::newGame() {
    score_ = 0;
    lives_ = 3;
    circuitsDone_ = 0;
    won_ = false;
    over_ = false;
    result_[0] = 0;
    fanStep_ = -1;
    sampleN_ = 0;
    sampleT_ = 0.f;
    place();
    mode_ = Mode::Fly;
}

void Game::sample() {
    sampleT_ += DT;
    if (sampleT_ < 3.f) return;
    sampleT_ = 0.f;
    Sample s{airT_, x_, z_, h_, v_, gate_};
    if (sampleN_ < 12) samples_[sampleN_++] = s;
    else {
        for (int i = 1; i < 12; i++) samples_[i - 1] = samples_[i];
        samples_[11] = s;
    }
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Fly) return;
    std::snprintf(result_, sizeof result_, "%s", why);
    lives_--;
    shake_ = 1.f;
    sys_->rumble(0.55f, 0.4f, 180);
    sys_->setLight(180, 40, 20);
    sys_->apu.noiseBurst(0.45f, 620.f, 0.28f);
    if (bot_ || lives_ <= 0) {
        mode_ = Mode::Over;
        over_ = true;
        won_ = false;
        for (int i = 0; i < sampleN_; i++) {
            const Sample& s = samples_[i];
            std::fprintf(stderr, "  t %.0f x %.0f z %.0f h %.0f v %.0f g %d\n", s.t, s.x, s.z, s.h, s.v, s.gate);
        }
    } else {
        mode_ = Mode::Dead;
        deadT_ = 0.f;
    }
}

void Game::succeed() {
    if (mode_ != Mode::Fly) return;
    std::snprintf(result_, sizeof result_, "FULL STOP");
    circuitsDone_ = 3;
    score_ += 2500 + int(std::max(0.f, STRIP_LEN - z_));
    won_ = true;
    over_ = true;
    mode_ = Mode::Victory;
    v_ = 0.f;
    throttle_ = 0.f;
    braking_ = true;
    fanStep_ = 0;
    fanT_ = 0.f;
    sys_->rumble(0.2f, 0.08f, 120);
    sys_->setLight(40, 170, 60);
    sys_->apu.noise(0.f, 800.f, false);
}

void Game::note() {
    if (h_ > maxH_) maxH_ = h_;
    if (z_ > STRIP_LEN + 20.f && h_ > 22.f) sawDepart_ = true;
    if (x_ < -58.f && h_ > 28.f && std::cos(heading_) < -0.35f) sawDownwind_ = true;
}

void Game::touchdown(float sink) {
    if (mode_ != Mode::Fly) return;
    const float ang = std::fabs(wrap(heading_));
    const bool strip = std::fabs(x_) <= STRIP_HALF - 0.6f && z_ >= 4.f && z_ <= STRIP_LEN - 12.f;
    if (!strip) {
        fail("MISSED THE STRIP");
        return;
    }
    if (sink < -6.0f) {
        fail("HARD LANDING");
        return;
    }
    if (ang > 0.48f) {
        fail("CROOKED");
        return;
    }
    if (v_ > 38.f) {
        fail("TOO FAST");
        return;
    }
    if (v_ < 18.5f) {
        fail("TOO SLOW");
        return;
    }
    if (!sawDownwind_ || !sawDepart_ || maxH_ < 36.f) {
        fail("NOT A CIRCUIT");
        return;
    }
    sys_->rumble(0.28f, 0.1f, 90);
    sys_->apu.noiseBurst(0.28f, 420.f, 0.16f);
    int soft = int(std::max(0.f, (6.f + sink) * 80.f));
    if (circuitsDone_ >= 2) {
        rollout_ = Roll::Stop;
        score_ += 600 + soft;
        std::snprintf(banner_, sizeof banner_, "FULL STOP");
    } else {
        rollout_ = Roll::Go;
        score_ += 350 + soft;
        std::snprintf(banner_, sizeof banner_, "TOUCH AND GO");
    }
    bannerT_ = 1.6f;
    blip(true);
}

void Game::leaveGround() {
    if (mode_ != Mode::Fly) return;
    if (rollout_ == Roll::Stop) {
        fail("NOT A FULL STOP");
        return;
    }
    onGround_ = false;
    vh_ = std::max(0.6f, predictVh(pitch_, v_));
    if (rollout_ == Roll::Go) {
        circuitsDone_++;
        score_ += 1000;
        sawDownwind_ = false;
        sawDepart_ = false;
        maxH_ = h_;
        gate_ = 0;
        rollout_ = Roll::None;
        std::snprintf(banner_, sizeof banner_, "CIRCUIT %d", circuitsDone_);
        bannerT_ = 1.8f;
        blip(true);
        sys_->setLight(40, 140, 70);
    }
}

void Game::groundChecks() {
    if (mode_ != Mode::Fly || !onGround_) return;
    if (rollout_ == Roll::Stop && v_ < 0.85f) {
        const bool parked = std::fabs(x_) <= STRIP_HALF && z_ >= 4.f && z_ <= STRIP_LEN - 4.f;
        if (parked) succeed();
        else fail(z_ > STRIP_LEN - 4.f ? "OVERRUN" : "OFF THE GRASS");
        return;
    }
    if (rollout_ == Roll::Go && v_ < 1.5f) {
        fail("EARLY FULL STOP");
        return;
    }
    if (v_ > 8.f && (std::fabs(x_) > STRIP_HALF + 6.f || z_ < -10.f || z_ > STRIP_LEN + 12.f)) {
        if (rollout_ == Roll::Stop && z_ > STRIP_LEN) fail("OVERRUN");
        else fail("OFF THE GRASS");
    }
}

void Game::pilot(float& nose, float& steer, float& thr, bool& brake) {
    auto toHeading = [&](float want) {
        float err = wrap(want - heading_);
        steer = std::clamp(err * 2.8f - bank_ * 0.75f, -1.f, 1.f);
    };
    auto toPitch = [&](float rad) { nose = std::clamp(rad / MAX_PITCH, -1.f, 1.f); };
    brake = false;

    if (onGround_) {
        if (rollout_ == Roll::Stop) {
            toPitch(0.f);
            thr = 0.f;
            brake = true;
            toHeading(std::clamp(-x_ * 0.09f, -0.3f, 0.3f));
            return;
        }
        toPitch(v_ > ROTATE - 1.f ? 0.18f : 0.02f);
        thr = 1.f;
        toHeading(std::clamp(-x_ * 0.08f, -0.28f, 0.28f));
        return;
    }

    if (gate_ < NGATES) {
        const Gate& g = GATES[gate_];
        float dx = g.x - x_;
        float dz = g.z - z_;
        float dist = std::sqrt(dx * dx + dz * dz);
        float ahead = dx * std::sin(heading_) + dz * std::cos(heading_);
        if (dist < 34.f || (ahead < -18.f && dist < 110.f)) gate_++;
    }

    const bool finals = (sawDownwind_ && sawDepart_ && std::fabs(wrap(heading_)) < 0.62f && z_ > 8.f &&
                         z_ < STRIP_LEN - 8.f && x_ > -82.f && x_ < 30.f && h_ < 68.f) ||
                        gate_ >= NGATES;
    if (finals) {
        const bool last = circuitsDone_ >= 2;
        const float touchZ = last ? 148.f : 188.f;
        const float slope = 0.15f;
        const float distZ = touchZ - z_;
        float wantH = std::clamp(distZ * slope, 0.55f, 46.f);
        const bool lined = std::fabs(x_) < 8.f && std::fabs(wrap(heading_)) < 0.18f;
        if (!lined && h_ < 14.f) wantH = std::max(wantH, 9.5f);
        float slopeRate = -v_ * std::max(0.f, std::cos(heading_)) * slope;
        if (distZ < 0.f) slopeRate = -1.1f;
        float wantVh = slopeRate + (wantH - h_) * 0.85f;
        const bool flare = h_ < 9.f && z_ > 40.f && (lined || h_ < 4.5f);
        if (flare) wantVh = h_ < 2.3f ? -0.65f : -1.25f;
        wantVh = std::clamp(wantVh, flare ? -2.0f : -4.8f, 4.4f);
        toPitch(std::clamp(pitchFor(wantVh, v_), -MAX_PITCH, MAX_PITCH));
        float vx = std::sin(heading_) * v_;
        toHeading(std::clamp(-x_ * 0.07f - vx * 0.04f, -0.5f, 0.5f));
        float wantSpd = last ? 24.f : 26.8f;
        if (flare && !last) wantSpd = 27.2f;
        thr = throttleFor(wantSpd, v_);
        return;
    }

    const Gate& g = GATES[std::min(gate_, NGATES - 1)];
    float dx = g.x - x_;
    float dz = g.z - z_;
    toHeading(std::atan2(dx, dz));
    float wantVh = std::clamp((g.h - h_) * 0.72f, -4.4f, 4.6f);
    toPitch(std::clamp(pitchFor(wantVh, v_), -MAX_PITCH, MAX_PITCH));
    thr = throttleFor(g.spd, v_);
}

void Game::human(float& nose, float& steer, float& thr, bool& brake) {
    const gs::Pad& pad = sys_->pad;
    if (pad.down(gs::BTN_C) || pad.accel > 0.12f) throttle_ = std::min(1.f, throttle_ + DT * 0.72f);
    if (pad.down(gs::BTN_B) || pad.down(gs::BTN_X)) throttle_ = std::max(0.f, throttle_ - DT * 0.9f);
    thr = throttle_;
    nose = (pad.down(gs::BTN_UP) ? 1.f : 0.f) - (pad.down(gs::BTN_DOWN) ? 1.f : 0.f);
    steer = (pad.down(gs::BTN_RIGHT) ? 1.f : 0.f) - (pad.down(gs::BTN_LEFT) ? 1.f : 0.f);
    if (std::fabs(pad.axisX) > 0.18f) steer = std::clamp(pad.axisX, -1.f, 1.f);
    brake = pad.down(gs::BTN_TURBO) || pad.brake > 0.18f;
}

void Game::fly(float nose, float steer, float thr, bool brake) {
    throttle_ = std::clamp(thr, 0.f, 1.f);
    braking_ = brake;
    pitch_ += (nose * MAX_PITCH - pitch_) * 6.8f * DT;
    pitch_ = std::clamp(pitch_, -MAX_PITCH, MAX_PITCH);

    if (onGround_) {
        bank_ += (0.f - bank_) * 6.f * DT;
        float auth = std::clamp(v_ / 7.f, 0.25f, 1.f);
        heading_ = wrap(heading_ + steer * 1.05f * auth * DT);
    } else {
        bank_ += (steer * 0.92f - bank_) * 6.5f * DT;
        bank_ = std::clamp(bank_, -1.f, 1.f);
        heading_ = wrap(heading_ + bank_ * 1.28f * DT);
    }

    float drag = DRAG_Q * v_ * v_ + DRAG_L * v_;
    float resist = onGround_ ? 0.48f : 0.f;
    float brakes = (onGround_ && braking_) ? 3.6f : 0.f;
    v_ += (throttle_ * THRUST - drag - resist - brakes) * DT;
    if (v_ < 0.f) v_ = 0.f;
    if (v_ > 46.f) v_ = 46.f;

    x_ += std::sin(heading_) * v_ * DT;
    z_ += std::cos(heading_) * v_ * DT;
    travel_ += v_ * DT;

    if (onGround_) {
        h_ = 0.f;
        vh_ = 0.f;
        if (v_ > ROTATE && pitch_ > 0.09f && throttle_ > 0.48f && !braking_) leaveGround();
        if (mode_ == Mode::Fly) groundChecks();
    } else {
        note();
        float target = predictVh(pitch_, v_);
        vh_ += (target - vh_) * 3.5f * DT;
        h_ += vh_ * DT;
        if (h_ <= 0.f) {
            float sink = vh_;
            h_ = 0.f;
            vh_ = 0.f;
            onGround_ = true;
            touchdown(sink);
        } else if (h_ > 190.f || z_ > 1600.f || z_ < -500.f || std::fabs(x_) > 700.f) {
            fail("LOST THE FIELD");
        }
    }

    if (mode_ == Mode::Fly && airT_ > 300.f) fail("LOST THE FIELD");

    if (onGround_ && v_ > 7.f && mode_ == Mode::Fly) {
        puffT_ -= DT;
        if (puffT_ <= 0.f) {
            puffT_ = 0.07f;
            Puff p;
            p.x = x_ - std::sin(heading_) * 3.2f + bank_ * 0.4f;
            p.z = z_ - std::cos(heading_) * 3.2f;
            p.t = 0.38f;
            if (puffs_.size() > 12) puffs_.erase(puffs_.begin());
            puffs_.push_back(p);
        }
    }
    for (Puff& p : puffs_) p.t -= DT;
}

const char* Game::hint() const {
    if (rollout_ == Roll::Stop) return "BRAKES  SPACE  —  FULL STOP";
    if (rollout_ == Roll::Go) return "POWER UP  —  TOUCH AND GO";
    if (onGround_ && v_ < ROTATE) return "C POWER   ROTATE WITH UP";
    if (!sawDepart_) return "CLIMB PAST THE FAR HEDGE";
    if (!sawDownwind_) return "LEFT HAND  —  DOWNWIND";
    if (h_ > 16.f) return circuitsDone_ >= 2 ? "FINAL  —  THEN FULL STOP" : "FINAL  —  THEN POWER UP";
    if (circuitsDone_ >= 2) return "FLARE  —  THIRD LANDING";
    return "FLARE  —  TOUCH AND GO";
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet, bool shadow) {
    if (h < 1.3f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::clamp(long(std::lround(cx - s.w * 0.5f)), -8000L, 8000L));
    float top = feet ? cy - s.h : cy - s.h * 0.5f;
    s.y = int16_t(std::clamp(long(std::lround(top)), -8000L, 8000L));
    if (s.x > gs::SCREEN_W + 48 || s.x + s.w < -48 || s.y > gs::SCREEN_H + 24 || s.y + s.h < -80) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    float width = 0.f;
    for (unsigned char c : s) {
        if (c < 32 || c >= 128) continue;
        if (c == 32) width += 14.f * scale;
        else width += float(art_.glyph[c - 32].w) * scale + scale;
    }
    float pen = x - width * 0.5f;
    for (unsigned char c : s) {
        if (c < 32 || c >= 128) continue;
        if (c == 32) {
            pen += 14.f * scale;
            continue;
        }
        const gs::Mipped& g = art_.glyph[c - 32];
        float w = float(g.w) * scale;
        spr(g, pen + w * 0.5f, y, float(g.h) * scale, pal, false);
        pen += w + scale;
    }
}

bool Game::project(float wx, float wy, float wz, float& sx, float& sy, float& scale, int& fog) const {
    float camX = x_ - std::sin(heading_) * CAM_BACK;
    float camZ = z_ - std::cos(heading_) * CAM_BACK;
    float camH = h_ + CAM_EYE;
    float dx = wx - camX;
    float dz = wz - camZ;
    float S = std::sin(heading_);
    float C = std::cos(heading_);
    float rz = dx * S + dz * C;
    float rx = dx * C - dz * S;
    if (rz < 1.2f) return false;
    scale = FOCAL / rz;
    sx = 160.f + rx * scale + bank_ * 34.f;
    sy = HORIZON - (wy - camH) * scale;
    fog = 0;
    if (rz > 70.f) fog = std::clamp(int((rz - 70.f) / 28.f), 0, 12);
    return true;
}

void Game::drawRoad() {
    gs::VDP& v = sys_->vdp;
    const uint16_t zenith = gs::rgb4(3, 6, 12);
    const uint16_t mid = gs::rgb4(6, 11, 15);
    const uint16_t haze = gs::rgb4(12, 15, 14);
    float camX = x_ - std::sin(heading_) * CAM_BACK;
    float camZ = z_ - std::cos(heading_) * CAM_BACK;
    float camH = std::max(2.2f, h_ + CAM_EYE);
    float S = std::sin(heading_);
    float C = std::cos(heading_);
    const float hor = HORIZON;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (float(y) < hor) {
            float u = float(y) / hor;
            v.lineBackdrop[y] = u < 0.62f ? lerpC(zenith, mid, u / 0.62f) : lerpC(mid, haze, (u - 0.62f) / 0.38f);
            v.lineFog[y] = 0;
            v.road[y].on = false;
            continue;
        }
        float row = std::max(1.f, float(y) - hor);
        float dist = camH * FOCAL / row;
        gs::RoadLine& r = v.road[y];
        r.on = true;
        r.pal = PAL_FIELD;
        r.left = r.right = gs::GROUND_LAND;
        r.v = (travel_ + dist) * 28.f;
        r.band = (int(std::floor((travel_ + dist) / 16.f)) & 1) ? 1 : 0;
        r.style = 0;
        r.cx = -4000.f;
        r.hw = 1.f;
        float lo = -1e8f, hi = 1e8f;
        auto clip = [&](float a, float b, float c0, float c1) -> bool {
            if (std::fabs(a) < 1e-4f) return b >= c0 && b <= c1;
            float u0 = (c0 - b) / a;
            float u1 = (c1 - b) / a;
            if (u0 > u1) std::swap(u0, u1);
            lo = std::max(lo, u0);
            hi = std::min(hi, u1);
            return lo <= hi;
        };
        float px0 = camX + S * dist;
        float pz0 = camZ + C * dist;
        bool hit = clip(C, px0, -STRIP_HALF, STRIP_HALF) && clip(-S, pz0, 0.f, STRIP_LEN) && (hi - lo) > 0.6f;
        if (hit) {
            float midU = 0.5f * (lo + hi);
            float halfU = 0.5f * (hi - lo);
            float scl = FOCAL / std::max(1.f, dist);
            r.cx = 160.f + midU * scl + bank_ * 34.f;
            r.hw = std::min(1800.f, halfU * scl);
            float align = std::fabs(C);
            r.style = align > 0.72f ? 1 : 0;
        }
        int fog = 0;
        if (dist > 50.f) fog = std::clamp(int((dist - 50.f) / 22.f), 0, 13);
        v.lineFog[y] = uint8_t(fog);
        v.lineBackdrop[y] = haze;
    }
}

void Game::drawWorld() {
    struct Item {
        float rz;
        float sx, sy, sh;
        const gs::Mipped* img;
        int pal;
        int fog;
        bool flip;
        bool feet;
    };
    Item items[160];
    int n = 0;
    auto push = [&](float wx, float wy, float wz, const gs::Mipped& img, float worldH, int pal, bool flip, bool feet) {
        if (n >= 160) return;
        float sx, sy, scale;
        int fog;
        if (!project(wx, wy, wz, sx, sy, scale, fog)) return;
        float sh = worldH * scale;
        if (sh < 1.4f) return;
        float dx = wx - x_;
        float dz = wz - z_;
        float rz = dx * std::sin(heading_) + dz * std::cos(heading_);
        items[n++] = {rz, sx, sy, sh, &img, pal, fog, flip, feet};
    };

    for (const Prop& p : props_) {
        switch (p.kind) {
        case Kind::TreeA: push(p.x, 0, p.z, art_.tree[0], 8.2f, PAL_TREE, false, true); break;
        case Kind::TreeB: push(p.x, 0, p.z, art_.tree[1], 7.4f, PAL_TREE, true, true); break;
        case Kind::Post: push(p.x, 0, p.z, art_.post, 1.7f, PAL_MARK, false, true); break;
        case Kind::Sock: {
            int fr = int(t_ * 4.f) % 3;
            if (fr < 0) fr = 0;
            push(p.x, 0, p.z, art_.sock[fr], 4.6f, PAL_CONE, false, true);
            break;
        }
        case Kind::Hangar: push(p.x, 0, p.z, art_.hangar, 7.2f, PAL_BARN, false, true); break;
        case Kind::Cone: push(p.x, 0, p.z, art_.cone, 1.15f, PAL_CONE, false, true); break;
        case Kind::Bar: push(p.x, 0, p.z, art_.bar, 0.55f, PAL_MARK, false, true); break;
        }
    }
    int flap = int(t_ * 7.f) & 1;
    for (const Bird& b : birds_) {
        float bx = b.x + std::sin(t_ * 0.6f + b.ph) * 8.f;
        float by = b.y + std::sin(t_ * 1.4f + b.ph) * 1.2f;
        push(bx, by, b.z, art_.bird[flap], 0.7f, PAL_BIRD, false, false);
    }
    for (const Cloud& c : clouds_) push(c.x, c.y, c.z + t_ * 1.5f, art_.cloud, 16.f, PAL_SKY, false, false);
    for (const Puff& p : puffs_) {
        if (p.t <= 0.f) continue;
        push(p.x, 0.2f, p.z, art_.dust, 0.7f + (0.38f - p.t) * 1.4f, PAL_DUST, false, false);
    }

    std::sort(items, items + n, [](const Item& a, const Item& b) { return a.rz < b.rz; });
    for (int i = 0; i < n; i++) {
        const Item& it = items[i];
        spr(*it.img, it.sx, it.sy, it.sh, it.pal, it.flip, it.fog, it.feet, false);
    }
}

void Game::drawCraft() {
    float bob = std::sin(t_ * 9.f) * (onGround_ ? std::min(v_ * 0.04f, 1.6f) : 0.4f);
    float sx = 160.f + bank_ * 26.f;
    float sy = 158.f - pitch_ * 34.f + bob + (shake_ > 0.f ? std::sin(t_ * 40.f) * shake_ * 3.f : 0.f);
    int fr = propFrame_;
    if (fr < 0) fr = 0;
    if (fr > 2) fr = 2;
    spr(art_.cub[fr], sx, sy, 78.f, PAL_SHIP, false);

    float ssx, ssy, scale;
    int fog;
    if (project(x_, 0.f, z_, ssx, ssy, scale, fog)) {
        float sh = std::clamp(7.f * scale * (1.f + h_ * 0.01f), 3.f, 70.f);
        spr(art_.shadow, ssx, ssy, sh, PAL_SHIP, false, fog, false, true);
    }

    bool showBug = mode_ == Mode::Fly && rollout_ != Roll::Stop;
    if (showBug) {
        float want = 0.f;
        if (!onGround_ && gate_ < NGATES && !(sawDownwind_ && std::fabs(wrap(heading_)) < 0.4f && z_ < STRIP_LEN)) {
            const Gate& g = GATES[gate_];
            want = std::atan2(g.x - x_, g.z - z_);
        } else if (onGround_) {
            want = std::clamp(-x_ * 0.08f, -0.3f, 0.3f);
        }
        float err = wrap(want - heading_);
        if (std::fabs(err) > 0.05f || std::fabs(x_) > 6.f) {
            float cx = 160.f + std::clamp(err, -1.1f, 1.1f) * 78.f;
            spr(art_.chevron, cx, HORIZON - 8.f, 16.f, PAL_AMBER, false);
        }
    }
}

void Game::drawHud() {
    char buf[64];
    if (mode_ == Mode::Title) {
        text("S3 GRASS", 160.f, 24.f, 1.7f, PAL_HUD);
        hudC(8, "THREE CIRCUITS", PAL_AMBER);
        hudC(10, "THE THIRD LANDING IS A FULL STOP", PAL_GOOD);
        hudC(13, "UP DOWN     PITCH", PAL_HUD);
        hudC(14, "LEFT RIGHT  STEER", PAL_HUD);
        hudC(15, "C / X       POWER", PAL_HUD);
        hudC(16, "SPACE       BRAKES", PAL_HUD);
        hudC(18, "LEFT-HAND CIRCUIT OF THE GRASS", PAL_AMBER);
        if ((sys_->frame / 30) % 2 == 0) hudC(22, "PRESS START", PAL_GOOD);
        hud(39 - int(std::strlen(S3_VERSION_STRING)), 27, S3_VERSION_STRING, PAL_HUD);
        return;
    }

    if (mode_ == Mode::Fly || mode_ == Mode::Pause) {
        std::snprintf(buf, sizeof buf, "CIRCUIT %d/3", circuit());
        hud(1, 1, buf, PAL_GOOD);
        std::snprintf(buf, sizeof buf, "SPD %02.0f", v_);
        int sp = PAL_HUD;
        if (v_ < STALL + 2.f && !onGround_) sp = PAL_BAD;
        hud(30, 1, buf, sp);
        std::snprintf(buf, sizeof buf, "ALT %03.0f", std::max(0.f, h_));
        hud(1, 2, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "THR %02.0f", throttle_ * 100.f);
        hud(30, 2, buf, throttle_ > 0.8f ? PAL_AMBER : PAL_HUD);
        if (bannerT_ > 0.f && banner_[0]) hudC(12, banner_, PAL_GOOD);
        else hudC(24, hint(), circuitsDone_ >= 2 ? PAL_AMBER : PAL_HUD);
        std::snprintf(buf, sizeof buf, "LIVES %d", lives_);
        hud(1, 27, buf, lives_ > 1 ? PAL_HUD : PAL_BAD);
        if (mode_ == Mode::Pause) {
            hudC(14, "PAUSE", PAL_AMBER);
            hudC(16, "START  RESUME", PAL_HUD);
            hudC(17, "ESC    TITLE", PAL_HUD);
        }
        return;
    }

    if (mode_ == Mode::Dead) {
        hudC(12, result_, PAL_BAD);
        std::snprintf(buf, sizeof buf, "LIVES %d", lives_);
        hudC(15, buf, PAL_HUD);
        hudC(18, "START", PAL_GOOD);
        return;
    }
    if (mode_ == Mode::Over) {
        hudC(11, result_, PAL_BAD);
        hudC(14, "THE GRASS KEEPS THE STRIP", PAL_HUD);
        std::snprintf(buf, sizeof buf, "SCORE %d", score_);
        hudC(16, buf, PAL_AMBER);
        hudC(20, "START", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Victory) {
        text("FULL STOP", 160.f, 28.f, 1.35f, PAL_GOOD);
        hudC(12, "THREE CIRCUITS", PAL_AMBER);
        hudC(14, "ON THE GRASS", PAL_GOOD);
        std::snprintf(buf, sizeof buf, "SCORE %d", score_);
        hudC(17, buf, PAL_HUD);
        hudC(21, "START", PAL_HUD);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    float spin = throttle_ * 28.f + v_ * 0.35f + (mode_ == Mode::Title ? 4.f : 0.f);
    propFrame_ = int(t_ * spin) % 3;
    if (propFrame_ < 0) propFrame_ = 0;
    drawRoad();
    drawCraft();
    drawWorld();
    for (int i = 0; i < 4; i++) {
        float sx = std::fmod(30.f + i * 160.f - heading_ * 86.f + 900.f, 520.f) - 50.f;
        spr(art_.hill, sx, HORIZON + 2.f, 34.f + float(i & 1) * 8.f, PAL_TREE, false, 9, true);
    }
    float sunX = 274.f - heading_ * 46.f;
    spr(art_.sun, sunX, 34.f, 22.f, PAL_SKY, false, 1);
    drawHud();
}

void Game::ambience() {
    float hz = 62.f + throttle_ * 84.f + v_ * 0.65f;
    if (!engine_) {
        sys_->apu.setPatch(0, enginePatch());
        sys_->apu.keyOn(0, hz, 0.12f);
        engine_ = true;
    }
    sys_->apu.setFreq(0, hz);
    float vol = 0.05f + throttle_ * 0.09f;
    if (mode_ == Mode::Title) vol = 0.04f;
    if (mode_ == Mode::Victory || mode_ == Mode::Over) vol = 0.03f;
    sys_->apu.setVol(0, vol);
    if (mode_ == Mode::Fly && !onGround_) sys_->apu.noise(std::clamp(v_ / 900.f, 0.f, 0.04f), 1600.f + v_ * 20.f, false);
    else if (mode_ == Mode::Fly && v_ > 4.f)
        sys_->apu.noise(std::min(0.03f, v_ / 1400.f), 700.f, false);
    else sys_->apu.noise(0.f, 500.f, false);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    buildWorld();
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.setFogColor(gs::rgb4(10, 13, 14));
    sys.apu.setMaster(0.8f);
    sys.apu.setEcho(0.12f, 0.2f, 0.12f);
    sys.apu.setPatch(0, enginePatch());
    sys.apu.setPatch(1, tonePatch());
    if (bot_) newGame();
    else {
        lives_ = 3;
        score_ = 0;
        circuitsDone_ = 0;
        won_ = false;
        over_ = false;
        result_[0] = 0;
        place();
        mode_ = Mode::Title;
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    if (mode_ != Mode::Pause) t_ += DT;
    if (beep_ > 0.f) {
        beep_ -= DT;
        if (beep_ <= 0.f) sys.apu.tone(1, 0.f, 0.f);
    }
    if (bannerT_ > 0.f) bannerT_ = std::max(0.f, bannerT_ - DT);
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - DT * 0.7f);

    if (fanStep_ >= 0) {
        static const float notes[] = {392.f, 523.f, 659.f, 784.f};
        fanT_ += DT;
        if (fanT_ > 0.16f) {
            if (fanStep_ < 4) sys.apu.keyOn(1, notes[fanStep_], 0.2f);
            else sys.apu.keyOff(1);
            fanStep_++;
            fanT_ = 0.f;
            if (fanStep_ > 8) fanStep_ = -1;
        }
    }

    ambience();

    if (mode_ == Mode::Title) {
        draw();
        if (pad.pressed(gs::BTN_START)) {
            blip(true);
            newGame();
        } else if (pad.pressed(gs::BTN_MODE)) {
            sys.quit();
        }
        return;
    }

    if (mode_ == Mode::Pause) {
        draw();
        if (pad.pressed(gs::BTN_START)) {
            blip(true);
            mode_ = Mode::Fly;
        } else if (pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
        }
        return;
    }

    if (mode_ == Mode::Dead) {
        deadT_ += DT;
        draw();
        if (pad.pressed(gs::BTN_START) || deadT_ > 1.35f) {
            place();
            mode_ = Mode::Fly;
        }
        return;
    }

    if (mode_ == Mode::Over || mode_ == Mode::Victory) {
        draw();
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
        }
        return;
    }

    if (!bot_ && pad.pressed(gs::BTN_START)) {
        mode_ = Mode::Pause;
        blip(false);
        draw();
        return;
    }

    float nose = 0.f, steer = 0.f, thr = 0.f;
    bool brake = false;
    if (bot_) pilot(nose, steer, thr, brake);
    else human(nose, steer, thr, brake);
    airT_ += DT;
    if (bot_) sample();
    fly(nose, steer, thr, brake);
    draw();
}

}  // namespace grass

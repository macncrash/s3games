#include "game/ladd.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace rladd {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kHorizon = 64.f;
constexpr float kZScale = 248.f;
constexpr float kZLine = 2.5f;
constexpr float kRun = 4.20f;
constexpr float kAir = 4.50f;
constexpr float kJump = 0.50f;
constexpr float kSide = 1.70f;
constexpr float kPush = 0.58f;
constexpr float kStartZ = 2.15f;
constexpr float kGrab0 = 30.00f;
constexpr float kGrab1 = 32.10f;
constexpr float kGrabU = 0.34f;
constexpr float kLadderZ = 31.55f;
constexpr float kCliffZ = 32.05f;
constexpr float kVoidZ = 32.30f;

float approach(float v, float goal, float step) {
    float d = goal - v;
    if (std::fabs(d) <= step) return goal;
    return v + std::copysign(step, d);
}

uint16_t mix(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Won || mode_ == Mode::Lost) return 4;
    if (mode_ != Mode::Play && mode_ != Mode::Pause) return 0;
    if (climbing_ || pz_ > 29.4f) return 3;
    if (pz_ >= 7.6f && pz_ <= 10.0f) return 2;
    if (pz_ >= 16.2f && pz_ <= 19.2f) return 2;
    return 1;
}

float Game::windAt(float z) const {
    auto band = [&](float a, float b, float amp) {
        if (z < a || z > b) return 0.f;
        float e = std::min(z - a, b - z);
        return amp * std::clamp(e / 0.55f, 0.f, 1.f);
    };
    float w = band(3.4f, 7.7f, 0.78f) + band(10.0f, 15.8f, -0.92f) + band(19.4f, 23.6f, 0.70f) +
              band(26.2f, 31.4f, -0.36f);
    w += std::sin(t_ * 1.6f + z) * 0.08f;
    return w;
}

float Game::stoneU(int i) const {
    const Stone& s = stones_[i];
    return std::sin(t_ * s.rate + s.phase) * s.amp;
}

int Game::nextStone(float& dz) const {
    int best = -1;
    dz = 99.f;
    for (int i = 0; i < 3; ++i) {
        float d = stones_[i].z - pz_;
        if (d > -0.20f && d < dz && d < 1.85f) {
            dz = d;
            best = i;
        }
    }
    return best;
}

void Game::bandAt(float z, float& u0, float& u1, int& kind) const {
    kind = BAND_ROAD;
    u0 = -1.02f;
    u1 = 1.02f;
    if (z < 0.f) return;
    struct Seg {
        float a, b, u0, u1;
        int k;
    };
    static const Seg segs[] = {
        {8.10f, 9.45f, 0.f, 0.f, BAND_GAP},
        {16.70f, 18.20f, 0.f, 0.f, BAND_GAP},
        {18.20f, 19.10f, -0.92f, 0.42f, BAND_ROAD},
        {24.50f, 25.70f, 0.f, 0.f, BAND_GAP},
    };
    for (const Seg& e : segs) {
        if (z >= e.a && z < e.b) {
            u0 = e.u0;
            u1 = e.u1;
            kind = e.k;
            return;
        }
    }
    if (z >= 29.20f && z < kVoidZ) {
        float t = (z - 29.20f) / (kVoidZ - 29.20f);
        float h = 1.02f - 0.66f * t;
        u0 = -h;
        u1 = h;
        return;
    }
    if (z >= kVoidZ) kind = BAND_VOID;
}

bool Game::feetSafe(const char*& why) const {
    float a = 0, b = 0;
    int kind = BAND_ROAD;
    bandAt(pz_, a, b, kind);
    if (kind == BAND_GAP) {
        why = "THE BREAK";
        return false;
    }
    if (kind == BAND_VOID) {
        why = "PAST THE LADDER";
        return false;
    }
    if (u_ < a - 0.03f || u_ > b + 0.03f) {
        if (pz_ >= 18.15f && pz_ < 19.15f) why = "THE LIP";
        else if (pz_ >= 29.2f) why = "THE LAST LIP";
        else why = "OFF THE RIDGE";
        return false;
    }
    return true;
}

float Game::viewZ(float worldZ) const { return kZLine + (worldZ - pz_); }

float Game::worldAt(float row) const {
    float vz = kZScale / std::max(row, 0.5f);
    return pz_ + (vz - kZLine);
}

float Game::bendAt(float row) const {
    float wz = worldAt(row);
    return std::sin(wz * 0.15f) * (8.f + row * 0.02f);
}

float Game::halfAt(float row) const { return 20.f + row * 0.55f; }

int Game::horizon() const { return std::clamp(int(std::lround(kHorizon + shy_)), 36, 100); }

Game::Spot Game::spot(float u, float vz, float base) const {
    Spot s;
    if (!(vz > 0.85f) || vz > 52.f) return s;
    float row = kZScale / vz;
    float feet = kZScale / kZLine;
    float tn = std::clamp(row / feet, 0.02f, 1.35f);
    s.h = std::max(4.f, base * std::pow(tn, 0.76f));
    float along = std::clamp((vz - kZLine) / 36.f, 0.f, 1.f);
    float lift = along * along * s.h * 0.48f;
    s.x = 160.f + bendAt(row) + u * halfAt(row) + shx_;
    s.y = float(horizon()) + row - lift + shy_;
    s.fog = int(std::clamp(along * 11.f, 0.f, 11.f));
    s.ok = true;
    return s;
}

void Game::layout() {
    props_.clear();
    puffs_.clear();
    stones_[0] = Stone{12.80f, 0.6f, 0.50f, 0.76f};
    stones_[1] = Stone{21.50f, 2.2f, 0.58f, 0.74f};
    stones_[2] = Stone{27.70f, 1.1f, 0.46f, 0.76f};

    auto add = [&](float z, float u, int kind, float h) {
        Prop p;
        p.z = z;
        p.u = u;
        p.kind = kind;
        p.h = h;
        props_.push_back(p);
    };
    for (float z = 1.4f; z < 28.8f; z += 3.35f) {
        float a, b;
        int kind = BAND_ROAD;
        bandAt(z, a, b, kind);
        if (kind == BAND_GAP) continue;
        float side = (int(z * 2.f) & 1) ? 1.f : -1.f;
        add(z, side * 1.16f, PROP_POST, 44.f);
    }
    auto lips = [&](float z) {
        add(z, -0.78f, PROP_TOOTH, 30.f);
        add(z, 0.78f, PROP_TOOTH, 26.f);
    };
    lips(7.95f);
    lips(9.55f);
    lips(16.55f);
    lips(18.28f);
    lips(24.35f);
    lips(25.82f);
    add(kCliffZ, 0.f, PROP_CLIFF, 132.f);
    add(kLadderZ, 0.02f, PROP_LADDER, 118.f);
}

void Game::bootTitle() {
    layout();
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    climbing_ = false;
    jumping_ = false;
    grounded_ = true;
    moving_ = true;
    reason_ = "UNFINISHED";
    face_ = 1;
    fanStep_ = -1;
    holdStone_ = -1;
    holdSide_ = 1;
    hold_ = 0;
    pz_ = 5.6f;
    u_ = 0;
    vu_ = 0;
    stun_ = hurt_ = leave_ = jumpT_ = jumpBuf_ = climb_ = wait_ = 0;
    shake_ = 0;
    t_ = 0;
}

void Game::begin() {
    layout();
    mode_ = Mode::Play;
    won_ = false;
    over_ = false;
    climbing_ = false;
    jumping_ = false;
    grounded_ = true;
    moving_ = false;
    reason_ = "UNFINISHED";
    face_ = 1;
    fanStep_ = -1;
    holdStone_ = -1;
    holdSide_ = 1;
    hold_ = 0;
    pz_ = kStartZ;
    u_ = 0;
    vu_ = 0;
    stun_ = hurt_ = leave_ = jumpT_ = jumpBuf_ = climb_ = wait_ = 0;
    shake_ = 0;
    t_ = 0;
    scroll_ = 0;
}

void Game::blip(float freq) {
    beep_ = 0.09f;
    if (sys_) sys_->apu.tone(0, freq, 0.07f);
}

void Game::fanfare(bool good) {
    fanGood_ = good;
    fanStep_ = 0;
    fanT_ = 0;
}

void Game::puffAt(float z, float u) {
    if (puffs_.size() >= 12) return;
    Puff p;
    p.z = z;
    p.u = u;
    p.t = 0.36f;
    puffs_.push_back(p);
}

void Game::startClimb() {
    if (climbing_ || mode_ != Mode::Play) return;
    climbing_ = true;
    jumping_ = false;
    jumpT_ = 0;
    climb_ = 0;
    stun_ = 0;
    shake_ = 0.25f;
    blip(520.f);
    if (sys_) sys_->rumble(0.2f, 0.45f, 120);
}

bool Game::tryGrab(float axisZ) {
    if (climbing_ || mode_ != Mode::Play) return false;
    if (pz_ < kGrab0 || pz_ > kGrab1) return false;
    if (std::fabs(u_) > kGrabU) return false;
    bool reach = axisZ > 0.10f || std::fabs(u_) < 0.18f || bot_;
    if (!reach) return false;
    startClimb();
    return true;
}

void Game::win() {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Won;
    won_ = true;
    reason_ = "THE FAR LADDER";
    hold_ = 1.25f;
    shake_ = 0.18f;
    fanfare(true);
    if (sys_) sys_->rumble(0.25f, 0.55f, 180);
}

void Game::lose(const char* why) {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Lost;
    won_ = false;
    reason_ = why ? why : "THE RIDGE";
    hold_ = 1.30f;
    shake_ = 1.f;
    jumping_ = false;
    fanfare(false);
    if (sys_) {
        sys_->apu.noiseBurst(0.22f, 180.f, 0.28f);
        sys_->rumble(0.55f, 0.16f, 200);
    }
}

void Game::readPad(float& axisU, float& axisZ, bool& jump) const {
    axisU = 0;
    axisZ = 0;
    jump = false;
    if (!sys_) return;
    const gs::Pad& pad = sys_->pad;
    if (pad.down(gs::BTN_LEFT)) axisU -= 1.f;
    if (pad.down(gs::BTN_RIGHT)) axisU += 1.f;
    if (std::fabs(pad.axisX) > std::fabs(axisU)) axisU = pad.axisX;
    if (pad.down(gs::BTN_UP)) axisZ += 1.f;
    if (pad.down(gs::BTN_DOWN)) axisZ -= 1.f;
    if (std::fabs(pad.axisY) > std::fabs(axisZ)) axisZ = pad.axisY;
    jump = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_X) ||
           pad.pressed(gs::BTN_Y) || pad.pressed(gs::BTN_Z) || pad.pressed(gs::BTN_TURBO);
}

void Game::bot(float& axisU, float& axisZ, bool& jump) {
    axisU = 0;
    axisZ = 1.f;
    jump = false;
    if (climbing_) {
        axisZ = 0;
        return;
    }

    float goal = 0.f;
    if (pz_ > 15.3f && pz_ < 19.35f) goal = -0.08f;

    float dz = 99.f;
    int sid = nextStone(dz);
    if (pz_ > 28.15f) {
        goal = 0.f;
        holdStone_ = -1;
        if (std::fabs(u_) > 0.12f) axisZ = 0.30f;
    } else if (sid >= 0) {
        float su = stoneU(sid);
        if (holdStone_ != sid && std::fabs(su) >= 0.42f) {
            holdStone_ = sid;
            holdSide_ = su > 0.f ? -1 : 1;
        }
        if (holdStone_ == sid && dz < 0.95f && std::fabs(su - holdSide_ * 0.66f) < 0.30f) holdStone_ = -1;
        if (holdStone_ == sid) {
            goal = float(holdSide_) * 0.66f;
            bool aligned = std::fabs(u_ - goal) < 0.20f;
            if (dz < 1.15f && !aligned) axisZ = 0.f;
            else if (!aligned) axisZ = 0.40f;
        } else if (dz < 1.05f) {
            axisZ = 0.f;
        }
        if (axisZ < 0.05f) wait_ += kDt;
        else wait_ = 0.f;
        if (wait_ > 5.f) axisZ = 1.f;
    } else {
        holdStone_ = -1;
        wait_ = 0.f;
    }

    static const float gaps[] = {8.10f, 16.70f, 24.50f};
    if (!jumping_ && grounded_ && stun_ <= 0.05f) {
        for (float g : gaps) {
            float lead = g - pz_;
            if (lead > 0.18f && lead < 0.50f) jump = true;
        }
    }
    if (jump || jumping_) axisZ = 1.f;

    float cancel = windAt(pz_) * (kPush / kSide);
    axisU = std::clamp((goal - u_) * 3.6f - cancel, -1.f, 1.f);
}

void Game::update() {
    float axisU = 0, axisZ = 0;
    bool jumpNow = false;
    if (bot_) bot(axisU, axisZ, jumpNow);
    else readPad(axisU, axisZ, jumpNow);
    axisU = std::clamp(axisU, -1.f, 1.f);
    axisZ = std::clamp(axisZ, -1.f, 1.f);

    if (climbing_) {
        climb_ += kDt;
        u_ = approach(u_, 0.f, 2.8f * kDt);
        if (climb_ >= 1.12f) win();
        return;
    }

    if (stun_ > 0.f) stun_ = std::max(0.f, stun_ - kDt);
    if (hurt_ > 0.f) hurt_ = std::max(0.f, hurt_ - kDt);
    if (jumpBuf_ > 0.f) jumpBuf_ = std::max(0.f, jumpBuf_ - kDt);
    if (jumpNow) jumpBuf_ = 0.12f;

    if (jumpBuf_ > 0.f && grounded_ && !jumping_ && stun_ <= 0.05f) {
        jumping_ = true;
        jumpT_ = 0.f;
        grounded_ = false;
        jumpBuf_ = 0.f;
        leave_ = 0.f;
        blip(740.f);
        puffAt(pz_, u_);
        if (sys_) sys_->rumble(0.18f, 0.32f, 70);
    }

    float wind = windAt(pz_);
    float side = kSide * (jumping_ ? 0.48f : 1.f) * (stun_ > 0.f ? 0.35f : 1.f);
    float push = kPush * (jumping_ ? 0.35f : 1.f);
    float prevU = u_;
    u_ += (axisU * side + wind * push) * kDt;
    if (std::fabs(axisU) < 0.10f && !jumping_ && stun_ <= 0.f) u_ -= u_ * 0.40f * kDt;
    u_ = std::clamp(u_, -1.50f, 1.50f);
    vu_ = (u_ - prevU) / kDt;
    if (vu_ > 0.25f) face_ = 1;
    else if (vu_ < -0.25f) face_ = -1;

    float vz;
    if (jumping_) vz = axisZ > 0.12f ? kAir : (axisZ < -0.12f ? -1.4f : kAir * 0.45f);
    else if (stun_ > 0.f) vz = 0.f;
    else vz = axisZ * kRun;
    pz_ += vz * kDt;
    if (pz_ < 0.50f) pz_ = 0.50f;
    scroll_ += std::fabs(vz) * 86.f * kDt;
    moving_ = std::fabs(vz) > 0.45f && !jumping_;

    bool landed = false;
    if (jumping_) {
        jumpT_ += kDt;
        if (jumpT_ >= kJump - 0.0001f) {
            jumping_ = false;
            jumpT_ = 0.f;
            landed = true;
        }
    }

    if (tryGrab(axisZ)) return;

    if (!jumping_) {
        const char* why = nullptr;
        if (feetSafe(why)) {
            grounded_ = true;
            leave_ = 0.f;
            if (landed) {
                blip(180.f);
                puffAt(pz_, u_);
            }
        } else if (landed || leave_ > 0.09f) {
            lose(why);
            return;
        } else {
            leave_ += kDt;
        }
    } else {
        grounded_ = false;
    }
    if (mode_ != Mode::Play) return;

    float hang = jumping_ ? std::sin(std::min(jumpT_, kJump) / kJump * kPi) : 0.f;
    if (hurt_ <= 0.f && hang < 0.50f) {
        for (int i = 0; i < 3; ++i) {
            float su = stoneU(i);
            if (std::fabs(stones_[i].z - pz_) < 0.32f && std::fabs(su - u_) < 0.17f) {
                float away = (u_ >= su) ? 1.f : -1.f;
                if (std::fabs(u_ - su) < 0.04f) away = (su >= 0.f) ? -1.f : 1.f;
                u_ = std::clamp(u_ + away * 0.14f, -1.5f, 1.5f);
                stun_ = 0.28f;
                hurt_ = 0.70f;
                shake_ = 0.7f;
                blip(150.f);
                puffAt(pz_, u_);
                if (sys_) sys_->rumble(0.4f, 0.12f, 90);
                if (!jumping_) {
                    const char* why = nullptr;
                    if (!feetSafe(why)) {
                        lose("A STONE");
                        return;
                    }
                }
                break;
            }
        }
    }

    if (moving_ && stun_ <= 0.f) {
        stepSnd_ -= kDt;
        if (stepSnd_ <= 0.f) {
            stepSnd_ = 0.28f;
            if (sys_) sys_->apu.tone(2, 92.f, 0.035f);
            if ((int(t_ * 8.f) & 1) && puffs_.size() < 8) puffAt(pz_, u_);
        }
    }

    if (std::fabs(wind) > 0.55f && (int(t_ * 10.f) % 7) == 0) puffAt(pz_ + 0.4f, u_ - wind * 0.2f);
}

void Game::serviceAudio() {
    if (!sys_) return;
    if (beep_ > 0.f) {
        beep_ -= kDt;
        if (beep_ <= 0.f && fanStep_ < 0) sys_->apu.tone(0, 0.f, 0.f);
    }
    if (climbing_ && mode_ == Mode::Play) sys_->apu.tone(0, 240.f + climb_ * 420.f, 0.05f);

    float wind = std::fabs(windAt(mode_ == Mode::Title ? 5.6f : pz_));
    if (wind > 0.28f && mode_ != Mode::Won) sys_->apu.noise(0.035f * wind, 700.f + wind * 900.f, false);
    else sys_->apu.noise(0.f, 400.f, false);

    if (mode_ == Mode::Play && !climbing_ && (moving_ || stun_ > 0.f)) sys_->apu.tone(2, moving_ ? 70.f : 48.f, 0.018f);
    else if (fanStep_ < 0) sys_->apu.tone(2, 0.f, 0.f);

    if (fanStep_ < 0) return;
    fanT_ += kDt;
    if (fanT_ < 0.13f) return;
    static const float good[] = {349.2f, 440.f, 523.3f, 698.5f};
    static const float bad[] = {196.f, 155.6f, 123.5f};
    const float* notes = fanGood_ ? good : bad;
    int n = fanGood_ ? 4 : 3;
    if (fanStep_ < n) sys_->apu.tone(1, notes[fanStep_], 0.08f);
    else sys_->apu.tone(1, 0.f, 0.f);
    ++fanStep_;
    fanT_ = 0.f;
    if (fanStep_ > n + 4) fanStep_ = -1;
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; ++i) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c < 33 || c > 126) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    if (!s) return;
    hud(20 - int(std::strlen(s)) / 2, row, s, pal);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet) {
    if (!(h > 1.5f) || m.h < 1 || m.w < 1) return;
    h = std::min(h, 420.f);
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::clamp(int(std::lround(cx - s.w * 0.5f)), -2000, 2000));
    s.y = int16_t(std::clamp(int(std::lround(feet ? cy - s.h : cy - s.h * 0.5f)), -2000, 2000));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::shadow(float cx, float cy, float w) {
    if (w < 6.f) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 6, 420));
    s.h = int16_t(std::max(4, int(std::lround(w * 0.16f))));
    s.x = int16_t(std::clamp(int(std::lround(cx - s.w * 0.5f)), -2000, 2000));
    s.y = int16_t(std::clamp(int(std::lround(cy - s.h * 0.5f)), -2000, 2000));
    s.img = art_.shadow.pick(float(s.h));
    s.pal = PAL_HUD;
    s.shadow = true;
    sys_->vdp.sprite(s);
}

void Game::text(const char* s, float x, float y, float scale, int pal) {
    if (!s) return;
    float width = 0.f;
    for (unsigned char c : std::string(s)) {
        if (c < 33 || c > 126) width += 10.f * scale;
        else width += float(art_.glyph[c - 32].w) * scale;
    }
    x -= width * 0.5f;
    for (unsigned char c : std::string(s)) {
        if (c < 33 || c > 126) {
            x += 10.f * scale;
            continue;
        }
        const gs::Mipped& g = art_.glyph[c - 32];
        float gw = float(g.w) * scale;
        spr(g, x + gw * 0.5f, y, float(g.h) * scale, pal, false, 0, false);
        x += gw;
    }
}

void Game::layRoad() {
    gs::VDP& v = sys_->vdp;
    uint16_t skyTop = gs::rgb4(2, 3, 8);
    uint16_t skyHor = gs::rgb4(14, 8, 6);
    uint16_t valley = gs::rgb4(2, 3, 5);
    uint16_t chasm = gs::rgb4(1, 0, 3);
    uint16_t rim = gs::rgb4(7, 7, 9);
    if (mode_ == Mode::Lost) skyHor = gs::rgb4(10, 3, 3);
    else if (mode_ == Mode::Won) skyHor = gs::rgb4(14, 11, 6);
    v.setFogColor(skyHor);
    int hor = horizon();
    for (int y = 0; y < gs::SCREEN_H; ++y) {
        gs::RoadLine& r = v.road[y];
        if (y <= hor) {
            float tn = float(y) / float(std::max(hor, 1));
            v.lineBackdrop[y] = mix(skyTop, skyHor, tn * tn);
            v.lineFog[y] = 0;
            r.on = false;
            continue;
        }
        float row = float(y - hor);
        float vz = kZScale / std::max(row, 0.5f);
        float wz = worldAt(row);
        float u0, u1;
        int kind = BAND_ROAD;
        bandAt(wz, u0, u1, kind);
        if (kind == BAND_VOID && row < 18.f) {
            kind = BAND_ROAD;
            u0 = -1.02f;
            u1 = 1.02f;
        }
        bool hole = kind == BAND_GAP || kind == BAND_VOID;
        bool edge = false;
        if (hole) {
            float a2, b2;
            int k0 = BAND_ROAD, k1 = BAND_ROAD;
            bandAt(wz - 0.28f, a2, b2, k0);
            bandAt(wz + 0.28f, a2, b2, k1);
            if (kind == BAND_VOID && row < 18.f) k0 = k1 = BAND_ROAD;
            edge = k0 == BAND_ROAD || k1 == BAND_ROAD;
        }
        v.lineBackdrop[y] = hole ? (edge ? rim : chasm) : valley;
        v.lineFog[y] = hole ? uint8_t(edge ? 2 : 0) : uint8_t(std::clamp(int(11.f - row * 0.07f), 0, 11));
        if (hole) {
            r.on = false;
            continue;
        }
        float half = halfAt(row);
        float mid = (u0 + u1) * 0.5f;
        float halfU = std::max(0.04f, (u1 - u0) * 0.5f);
        r.on = true;
        r.cx = 160.f + bendAt(row) + mid * half + shx_;
        r.hw = std::max(4.f, halfU * half);
        r.v = scroll_ + vz * 8.f;
        r.pal = uint8_t(PAL_FIELD);
        r.style = gs::ROAD_ROCKY;
        r.band = (int(std::floor(wz * 0.5f)) & 1) ? 1 : 0;
        r.left = r.right = gs::GROUND_DROP;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    shx_ = shy_ = 0.f;
    if (shake_ > 0.f) {
        shx_ = std::sin(t_ * 71.f) * 4.f * shake_;
        shy_ = std::cos(t_ * 53.f) * 2.2f * shake_;
    }
    layRoad();

    if (mode_ == Mode::Title) text("S3 RIDGE LADD", 160.f + shx_, 26.f, 0.78f, PAL_AMBER);
    else if (mode_ == Mode::Won) text("IT IS DONE", 160.f + shx_, 28.f, 1.0f, PAL_GOOD);
    else if (mode_ == Mode::Lost)
        text(reason_, 160.f + shx_, 28.f, std::strlen(reason_) > 14 ? 0.72f : 0.92f, PAL_ALERT);
    else if (mode_ == Mode::Pause) text("PAUSED", 160.f + shx_, 30.f, 1.0f, PAL_AMBER);

    struct Item {
        float z;
        int kind;
        int id;
    };
    std::vector<Item> items;
    float heroDrawZ = pz_;
    if (climbing_ || mode_ == Mode::Won) {
        float k = mode_ == Mode::Won ? 1.f : std::min(1.f, climb_ / 0.85f);
        heroDrawZ = pz_ + (kLadderZ - 0.15f - pz_) * k;
    }
    items.push_back({viewZ(heroDrawZ) - 0.04f, 0, 0});
    for (int i = 0; i < int(props_.size()); ++i) {
        float vz = viewZ(props_[size_t(i)].z);
        if (vz > 0.9f) items.push_back({vz, 1, i});
    }
    for (int i = 0; i < 3; ++i) {
        float vz = viewZ(stones_[i].z);
        if (vz > 0.9f) items.push_back({vz, 2, i});
    }
    for (int i = 0; i < int(puffs_.size()); ++i) {
        float vz = viewZ(puffs_[size_t(i)].z);
        if (vz > 0.9f) items.push_back({vz, 3, i});
    }
    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) { return a.z < b.z; });

    float wind = windAt(mode_ == Mode::Play || mode_ == Mode::Pause ? pz_ : 5.6f);
    int fr = int(t_ * 8.f) & 1;
    float hang = jumping_ ? std::sin(std::min(jumpT_, kJump) / kJump * kPi) : 0.f;

    for (const Item& it : items) {
        if (it.kind == 0) {
            float base = (climbing_ || mode_ == Mode::Won) ? 102.f : jumping_ ? 78.f : 90.f;
            Spot you = spot(u_, viewZ(heroDrawZ), base);
            if (!you.ok) continue;
            const gs::Mipped* body = &art_.walk[fr];
            if (mode_ == Mode::Title) body = &art_.walk[int(t_ * 6.f) & 1];
            else if (climbing_ || mode_ == Mode::Won) body = &art_.climb;
            else if (jumping_) body = &art_.jump;
            else if (!moving_) body = &art_.walk[0];
            float hop = hang * you.h * 0.42f;
            if (climbing_ || mode_ == Mode::Won) {
                float k = mode_ == Mode::Won ? 1.f : std::min(1.f, climb_);
                hop += k * you.h * 0.72f;
            }
            spr(*body, you.x, you.y - hop, you.h, PAL_YOU, face_ < 0, 0, true);
            shadow(you.x, you.y, you.h * (0.42f - hang * 0.18f));
            continue;
        }
        if (it.kind == 1) {
            const Prop& p = props_[size_t(it.id)];
            Spot s = spot(p.u, it.z, p.h);
            if (!s.ok) continue;
            if (p.kind == PROP_LADDER) {
                float flutter = std::sin(t_ * 9.f) * s.h * 0.04f;
                spr(art_.rag, s.x + s.h * 0.16f + flutter, s.y - s.h * 0.92f, s.h * 0.16f, PAL_ALERT, wind < 0.f, s.fog,
                    false);
                spr(art_.ladder, s.x, s.y, s.h, PAL_WOOD, false, s.fog, true);
                shadow(s.x, s.y, s.h * 0.22f);
                if (s.h > 36.f && mode_ != Mode::Won && mode_ != Mode::Lost)
                    text("LADDER", s.x, s.y - s.h - 8.f, 0.40f, PAL_AMBER);
                continue;
            }
            if (p.kind == PROP_CLIFF) {
                spr(art_.cliff, s.x, s.y, s.h, PAL_STONE, false, std::min(14, s.fog + 1), true);
                continue;
            }
            if (p.kind == PROP_TOOTH) {
                spr(art_.tooth, s.x, s.y, s.h, PAL_STONE, p.u > 0.f, s.fog, true);
                shadow(s.x, s.y, s.h * 0.55f);
                continue;
            }
            spr(art_.post, s.x, s.y, s.h, PAL_STONE, wind < 0.f, s.fog, true);
            shadow(s.x, s.y, s.h * 0.28f);
            continue;
        }
        if (it.kind == 2) {
            const Stone& st = stones_[it.id];
            float su = stoneU(it.id);
            Spot s = spot(su, it.z, 34.f);
            if (!s.ok) continue;
            int squat = int(t_ * st.rate * 2.f + st.phase) & 1;
            shadow(s.x, s.y, s.h * 0.7f);
            spr(art_.stone[squat], s.x, s.y - std::fabs(std::sin(t_ * st.rate + st.phase)) * 3.f, s.h, PAL_STONE,
                su > 0.f, s.fog, true);
            continue;
        }
        const Puff& p = puffs_[size_t(it.id)];
        Spot s = spot(p.u, it.z, 22.f);
        if (!s.ok) continue;
        float k = std::clamp(p.t / 0.36f, 0.f, 1.f);
        spr(art_.dust, s.x, s.y - (1.f - k) * 8.f, s.h * (0.5f + k), PAL_FX, false, int((1.f - k) * 5.f), false);
    }

    int hor = horizon();
    float drift = std::fmod(t_ * 6.f, 420.f);
    spr(art_.cloud, drift - 60.f, 18.f, 16.f, PAL_FX, false, 5, false);
    spr(art_.cloud, std::fmod(drift + 200.f, 420.f) - 40.f, 30.f, 12.f, PAL_FX, true, 6, false);
    spr(art_.sun, 46.f, 22.f, 22.f, PAL_FX, false, 1, false);
    spr(art_.peak[0], 250.f + shx_ * 0.1f, float(hor) + 2.f, 48.f, PAL_MOUNT, false, 4, true);
    spr(art_.peak[1], 78.f + shx_ * 0.12f, float(hor) + 6.f, 36.f, PAL_MOUNT, false, 5, true);

    char buf[64];
    if (mode_ == Mode::Title) {
        hudC(20, "ONE RIDGE", PAL_HUD);
        hudC(21, "REACH THE FAR LADDER", PAL_AMBER);
        hudC(22, "THEN IT IS DONE", PAL_GOOD);
        hudC(24, "ARROWS RUN THE RIDGE", PAL_HUD);
        hudC(25, "Z OR SPACE JUMPS THE BREAKS", PAL_AMBER);
        if ((int(t_ * 2.f) & 1) == 0) hudC(27, "PRESS START", PAL_AMBER);
        hud(39 - int(std::strlen(S3_VERSION_STRING)), 0, S3_VERSION_STRING, PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        hudC(24, "START RESUMES", PAL_HUD);
        hudC(26, "ESC TITLE", PAL_HUD);
    } else if (mode_ == Mode::Won) {
        hudC(22, "REACHED THE FAR LADDER", PAL_GOOD);
        hudC(23, "THEN IT IS DONE", PAL_AMBER);
        hudC(26, "START", PAL_HUD);
    } else if (mode_ == Mode::Lost) {
        hudC(22, "THE RIDGE IS NOT DONE", PAL_ALERT);
        hudC(23, reason_, PAL_HUD);
        hudC(26, "START RETRIES", PAL_HUD);
    } else {
        int paces = int(std::ceil(std::max(0.f, kGrab0 - pz_)));
        if (climbing_) std::snprintf(buf, sizeof buf, "CLIMB");
        else std::snprintf(buf, sizeof buf, "LADDER %d", paces);
        hud(1, 0, "RIDGE", PAL_HUD);
        hud(39 - int(std::strlen(buf)), 0, buf, climbing_ ? PAL_GOOD : PAL_AMBER);

        const char* hint = "THE LADDER IS AHEAD";
        int pal = PAL_HUD;
        float lead = 99.f;
        static const float gaps[] = {8.10f, 16.70f, 24.50f};
        for (float g : gaps) lead = std::min(lead, g - pz_);
        float sdz = 99.f;
        int sid = nextStone(sdz);
        if (climbing_) {
            hint = "UP THE LADDER";
            pal = PAL_GOOD;
        } else if (pz_ >= kGrab0 - 1.4f) {
            hint = "CENTER ON THE LADDER";
            pal = PAL_GOOD;
        } else if (sid >= 0 && sdz < 1.6f) {
            hint = "LET THE STONE PASS";
            pal = PAL_ALERT;
        } else if (lead > 0.f && lead < 2.4f) {
            hint = "JUMP THE BREAK";
            pal = PAL_AMBER;
        } else if (pz_ > 15.0f && pz_ < 19.2f) {
            hint = "THE RIGHT LIP IS GONE";
            pal = PAL_ALERT;
        } else if (wind > 0.40f) {
            hint = "GUST TO THE RIGHT";
            pal = PAL_AMBER;
        } else if (wind < -0.40f) {
            hint = "GUST TO THE LEFT";
            pal = PAL_AMBER;
        }
        hudC(1, hint, pal);
        hudC(27, "ARROWS RUN    Z JUMPS", PAL_HUD);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.apu.setMaster(0.82f);
    sys.apu.setEcho(0.16f, 0.22f, 0.12f);
    if (bot_) begin();
    else bootTitle();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 0.9f);
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        float prev = u_;
        u_ = std::sin(t_ * 0.7f) * 0.16f;
        vu_ = (u_ - prev) / kDt;
        face_ = vu_ >= 0.f ? 1 : -1;
        moving_ = true;
        scroll_ += 16.f * kDt;
        pz_ = 5.6f + std::sin(t_ * 0.33f) * 0.25f;
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_Z) ||
            pad.pressed(gs::BTN_TURBO))
            begin();
        else if (pad.pressed(gs::BTN_MODE) && !bot_) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        }
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) bootTitle();
        else update();
    } else if (mode_ == Mode::Pause) {
        moving_ = false;
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
        else if (pad.pressed(gs::BTN_MODE)) bootTitle();
    } else {
        hold_ -= kDt;
        if (hold_ <= 0.f) over_ = true;
        if (!bot_ && pad.pressed(gs::BTN_START)) begin();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) bootTitle();
    }

    for (Puff& p : puffs_) p.t -= kDt;
    puffs_.erase(std::remove_if(puffs_.begin(), puffs_.end(), [](const Puff& p) { return p.t <= 0.f; }), puffs_.end());

    serviceAudio();
    draw();
    if (won_) sys.setLight(40, 160, 70);
    else if (mode_ == Mode::Lost) sys.setLight(170, 36, 28);
    else sys.setLight(40, 70, 140);
}

}  // namespace rladd

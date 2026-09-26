#include "game/pouc.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace rpouc {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kWatch = 44.f;
constexpr float kStartZ = 2.6f;
constexpr float kFar = 32.2f;
constexpr float kPlant = 31.45f;
constexpr float kHorizon = 64.f;
constexpr float kZScale = 248.f;
constexpr float kZLine = 2.5f;
constexpr float kEdge = 1.03f;
constexpr float kSlipDie = 0.30f;
constexpr float kCarry = 2.85f;
constexpr float kFree = 4.55f;
constexpr float kSide = 3.05f;
constexpr float kDodge = 0.56f;
constexpr float kThreat = 3.6f;

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
    if (mode_ == Mode::Victory || mode_ == Mode::Fail) return 4;
    if (mode_ != Mode::Play && mode_ != Mode::Pause) return 0;
    if (!carrying_ && !planted_) return 2;
    if (pz_ > 18.f) return 3;
    return 1;
}

bool Game::stolen() const {
    for (const Foe& f : foes_)
        if (f.on && f.hasPouch) return true;
    return false;
}

float Game::windNow() const {
    if (mode_ != Mode::Play) return 0.f;
    auto hump = [&](float center, float amp, float wid) {
        float d = std::fabs(watch_ - center);
        if (d >= wid) return 0.f;
        float x = 1.f - d / wid;
        return amp * x * x;
    };
    return hump(2.8f, 1.05f, 0.7f) + hump(8.15f, -0.55f, 0.30f);
}

float Game::foeU(const Foe& f) const { return f.lane + std::sin(t_ * 1.35f + f.phase) * 0.05f; }

float Game::viewZ(float worldZ) const { return kZLine + (worldZ - pz_) * float(face_); }

float Game::worldAt(float row) const {
    float vz = kZScale / std::max(row, 0.5f);
    return pz_ + (vz - kZLine) * float(face_);
}

float Game::bendAt(float row) const {
    float wz = worldAt(row);
    return std::sin(wz * 0.17f) * (7.f + row * 0.018f);
}

float Game::halfAt(float row) const { return 20.f + row * 0.55f; }

int Game::horizon() const { return std::clamp(int(std::lround(kHorizon + shy_)), 36, 100); }

Game::Spot Game::spot(float u, float vz, float base) const {
    Spot s;
    if (!(vz > 0.92f) || vz > 48.f) return s;
    float row = kZScale / vz;
    float feet = kZScale / kZLine;
    float t = std::clamp(row / feet, 0.03f, 1.4f);
    s.h = std::max(6.f, base * std::pow(t, 0.72f));
    float along = std::clamp((vz - kZLine) / 36.f, 0.f, 1.f);
    float lift = along * along * s.h * 0.62f;
    s.x = 160.f + bendAt(row) + u * halfAt(row) + shx_;
    s.y = float(horizon()) + row - lift + shy_;
    s.fog = int(std::clamp(along * 10.f, 0.f, 10.f));
    s.ok = true;
    return s;
}

void Game::layout() {
    foes_.clear();
    props_.clear();
    puffs_.clear();
    auto foe = [&](int kind, float z, float lane, float speed, float lo, float hi) {
        Foe f;
        f.kind = kind;
        f.z = z;
        f.lane = lane;
        f.home = lane;
        f.speed = speed;
        f.lo = lo;
        f.hi = hi;
        f.dir = 1;
        f.phase = z * 0.37f;
        f.on = true;
        foes_.push_back(f);
    };
    // Three raiders own the spine. Step off, let them pass, step back.
    foe(0, 27.f, -0.06f, 1.65f, 0.f, 0.f);
    foe(0, 34.5f, 0.07f, 1.40f, 0.f, 0.f);
    foe(0, 42.f, -0.03f, 1.50f, 0.f, 0.f);
    // Shoulder hunter. Only dangerous once the pouch is loose.
    foe(1, 22.f, 0.90f, 0.55f, 20.4f, 23.8f);

    auto prop = [&](float z, float u, int kind, float h) {
        Prop p;
        p.z = z;
        p.u = u;
        p.kind = kind;
        p.h = h;
        props_.push_back(p);
    };
    prop(3.4f, -1.16f, 0, 42.f);
    prop(8.6f, 1.18f, 0, 36.f);
    prop(12.4f, -1.18f, 0, 38.f);
    prop(21.2f, 1.16f, 0, 34.f);
    prop(26.6f, -1.16f, 0, 36.f);
    prop(30.2f, 1.14f, 0, 32.f);
    prop(1.15f, 0.12f, 0, 46.f);
    prop(14.2f, -0.90f, 2, 30.f);
    prop(19.8f, 0.90f, 2, 28.f);
    prop(22.8f, -0.90f, 2, 30.f);
    prop(29.5f, 0.90f, 2, 26.f);
    prop(kFar, 0.f, 1, 78.f);
}

void Game::bootTitle() {
    layout();
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    carrying_ = true;
    planted_ = false;
    face_ = 1;
    lastSec_ = 99;
    fanStep_ = -1;
    watch_ = 0;
    hold_ = 0;
    pz_ = 5.2f;
    u_ = 0.f;
    vu_ = 0.f;
    pouchZ_ = pz_;
    pouchU_ = 0.f;
    pouchVz_ = pouchVu_ = 0.f;
    stun_ = 0.f;
    slip_ = 0.f;
    shake_ = 0.f;
    rush_ = 0.f;
    tossCd_ = 0.f;
    reason_ = "THE WATCH IS OVER";
}

void Game::begin() {
    layout();
    mode_ = Mode::Play;
    won_ = false;
    over_ = false;
    carrying_ = true;
    planted_ = false;
    face_ = 1;
    lastSec_ = 99;
    fanStep_ = -1;
    watch_ = 0.f;
    hold_ = 0.f;
    pz_ = kStartZ;
    u_ = 0.f;
    vu_ = 0.f;
    pouchZ_ = pz_;
    pouchU_ = 0.f;
    pouchVz_ = pouchVu_ = 0.f;
    stun_ = 0.f;
    slip_ = 0.f;
    shake_ = 0.f;
    rush_ = 0.f;
    tossCd_ = 0.f;
    t_ = 0.f;
    reason_ = "THE WATCH IS OVER";
    blip(620.f);
}

void Game::blip(float freq) {
    if (!sys_) return;
    sys_->apu.tone(0, freq, 0.07f);
    beep_ = 0.08f;
}

void Game::fanfare(bool good) {
    fanStep_ = 0;
    fanT_ = 0.f;
    fanGood_ = good;
}

void Game::puffAt(float z, float u) {
    puffs_.push_back({z, u, 0.36f});
    if (puffs_.size() > 10) puffs_.erase(puffs_.begin());
}

void Game::dropPouch(float away) {
    carrying_ = false;
    pouchU_ = u_ + away * 0.28f;
    pouchZ_ = pz_;
    pouchVu_ = away * 1.05f;
    pouchVz_ = -0.55f;
}

void Game::grab() {
    carrying_ = true;
    pouchVz_ = pouchVu_ = 0.f;
    shake_ = 0.18f;
    blip(740.f);
    if (sys_) sys_->rumble(0.12f, 0.28f, 60);
}

void Game::plant() {
    if (!carrying_) return;
    carrying_ = false;
    planted_ = true;
    pouchZ_ = kFar;
    pouchU_ = 0.f;
    pouchVz_ = pouchVu_ = 0.f;
    win();
}

void Game::win() {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Victory;
    won_ = true;
    reason_ = "THE POUCH CROSSED";
    hold_ = 1.15f;
    shake_ = 0.22f;
    fanfare(true);
    if (sys_) sys_->rumble(0.22f, 0.5f, 160);
}

void Game::lose(const char* why) {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Fail;
    won_ = false;
    reason_ = why;
    hold_ = 1.35f;
    shake_ = 1.f;
    fanfare(false);
    if (sys_) {
        sys_->apu.noiseBurst(0.22f, 210.f, 0.24f);
        sys_->rumble(0.55f, 0.18f, 200);
    }
}

void Game::tickFoes(float dt) {
    bool bagLoose = loose();
    for (Foe& f : foes_) {
        if (!f.on) continue;
        if (f.stun > 0.f) {
            f.stun -= dt;
            continue;
        }
        if (f.hasPouch) {
            f.z += 3.15f * dt;
            pouchZ_ = f.z;
            pouchU_ = f.lane;
            pouchVz_ = pouchVu_ = 0.f;
            if (f.z > kFar + 2.2f) lose("THEY TOOK THE POUCH");
            continue;
        }
        if (f.kind == 1 && bagLoose) {
            f.lane = approach(f.lane, pouchU_, 1.6f * dt);
            if (pouchZ_ > f.z + 0.05f) f.z += 2.5f * dt;
            else if (pouchZ_ < f.z - 0.05f) f.z -= 2.3f * dt;
            if (std::fabs(f.z - pouchZ_) < 0.55f && std::fabs(f.lane - pouchU_) < 0.30f) {
                f.hasPouch = true;
                pouchZ_ = f.z;
                pouchU_ = f.lane;
                blip(160.f);
            }
            continue;
        }
        if (f.kind == 1) {
            f.lane = approach(f.lane, f.home, 0.7f * dt);
            f.z += float(f.dir) * f.speed * dt;
            if (f.z > f.hi) {
                f.z = f.hi;
                f.dir = -1;
            } else if (f.z < f.lo) {
                f.z = f.lo;
                f.dir = 1;
            }
            continue;
        }
        f.z -= f.speed * dt;
        if (f.z < 0.15f) f.on = false;
    }
}

void Game::bot(float& axisU, float& axisZ, bool& plantNow, bool& toss) const {
    axisU = 0.f;
    axisZ = 0.f;
    plantNow = false;
    toss = false;
    if (stun_ > 0.f) return;

    float goalU = 0.f;
    if (!carrying_) {
        goalU = pouchU_;
        if (pouchZ_ > pz_ + 0.22f) axisZ = 1.f;
        else if (pouchZ_ < pz_ - 0.22f) axisZ = -1.f;
    } else {
        float best = 99.f;
        for (int i = 0; i < int(foes_.size()); ++i) {
            const Foe& f = foes_[size_t(i)];
            if (!f.on || f.stun > 0.f || f.kind != 0) continue;
            float dz = f.z - pz_;
            if (dz < -0.30f || dz > kThreat) continue;
            if (dz < best) {
                best = dz;
                goalU = (i % 2 == 0) ? kDodge : -kDodge;
            }
        }
        for (const Prop& p : props_) {
            if (p.kind != 2) continue;
            if (std::fabs(p.z - pz_) < 1.2f && std::fabs(goalU - p.u) < 0.30f)
                goalU = std::copysign(std::min(std::fabs(goalU), 0.46f), goalU);
        }
        if (std::fabs(u_) > 0.64f && best > 1.5f) goalU = 0.f;
        axisZ = 1.f;
        if (pz_ > kPlant - 1.1f && best > 2.f) goalU = 0.f;
        if (pz_ >= kPlant - 0.15f && std::fabs(u_) > 0.40f) axisZ = 0.35f;
        if (pz_ >= kPlant - 0.85f && std::fabs(u_) < 0.46f) plantNow = true;
    }
    axisU = std::clamp((goalU - u_) * 5.f, -1.f, 1.f);
}

void Game::update() {
    if (stun_ > 0.f) stun_ = std::max(0.f, stun_ - kDt);
    if (tossCd_ > 0.f) tossCd_ = std::max(0.f, tossCd_ - kDt);

    float axisU = 0.f, axisZ = 0.f;
    bool plantNow = false, toss = false;
    if (bot_) {
        bot(axisU, axisZ, plantNow, toss);
    } else if (sys_) {
        const gs::Pad& pad = sys_->pad;
        if (pad.down(gs::BTN_LEFT)) axisU -= 1.f;
        if (pad.down(gs::BTN_RIGHT)) axisU += 1.f;
        if (std::fabs(pad.axisX) > std::fabs(axisU)) axisU = pad.axisX;
        if (pad.down(gs::BTN_UP)) axisZ += 1.f;
        if (pad.down(gs::BTN_DOWN)) axisZ -= 1.f;
        if (std::fabs(pad.axisY) > std::fabs(axisZ)) axisZ = pad.axisY;
        plantNow = pad.down(gs::BTN_A) || pad.down(gs::BTN_C) || pad.down(gs::BTN_Z) || pad.down(gs::BTN_TURBO);
        toss = pad.pressed(gs::BTN_B);
    }
    axisU = std::clamp(axisU, -1.f, 1.f);
    axisZ = std::clamp(axisZ, -1.f, 1.f);
    if (stun_ > 0.f) {
        axisU = 0.f;
        axisZ = 0.f;
        plantNow = false;
        toss = false;
    }
    if (axisZ > 0.2f) face_ = 1;
    else if (axisZ < -0.2f) face_ = -1;

    float wind = windNow();
    float prevU = u_;
    float side = carrying_ ? kSide : kSide + 0.35f;
    if (stun_ <= 0.f) u_ += (axisU * side + wind * 0.8f) * kDt;
    else u_ += wind * 0.35f * kDt;
    u_ = std::clamp(u_, -1.35f, 1.35f);
    vu_ = (u_ - prevU) / kDt;

    float spd = axisZ >= 0.f ? (carrying_ ? kCarry : kFree) : (carrying_ ? kCarry * 0.82f : kFree);
    if (stun_ <= 0.f) pz_ += axisZ * spd * kDt;
    if (std::fabs(axisZ) > 0.35f) scroll_ += std::fabs(axisZ) * 130.f * kDt;
    rush_ = std::fabs(axisZ);

    if (stun_ <= 0.f) {
        for (const Prop& p : props_) {
            if (p.kind != 2) continue;
            if (std::fabs(p.z - pz_) < 0.42f && std::fabs(p.u - u_) < 0.20f) {
                float away = (u_ >= p.u) ? 1.f : -1.f;
                u_ += away * 0.16f;
                stun_ = 0.26f;
                shake_ = 0.55f;
                blip(150.f);
                puffAt(pz_, u_);
                if (sys_) sys_->rumble(0.3f, 0.1f, 80);
                if (carrying_ && std::fabs(u_) > 0.72f) {
                    dropPouch(away);
                    if (std::fabs(pouchU_) > kEdge) {
                        lose("THE POUCH IS GONE");
                        return;
                    }
                }
                break;
            }
        }
    }

    if (std::fabs(u_) > kEdge) slip_ += kDt;
    else slip_ = std::max(0.f, slip_ - kDt * 2.6f);
    if (slip_ > kSlipDie) {
        lose("OFF THE RIDGE");
        return;
    }

    tickFoes(kDt);
    if (mode_ != Mode::Play) return;

    if (stun_ <= 0.f) {
        for (Foe& f : foes_) {
            if (!f.on || f.stun > 0.f) continue;
            float fu = foeU(f);
            if (std::fabs(f.z - pz_) < 0.58f && std::fabs(fu - u_) < 0.30f) {
                f.stun = 0.75f;
                stun_ = 0.40f;
                shake_ = 0.75f;
                blip(180.f);
                if (sys_) sys_->rumble(0.45f, 0.12f, 110);
                puffAt(pz_, u_);
                if (f.hasPouch) {
                    f.hasPouch = false;
                    pouchZ_ = f.z;
                    pouchU_ = fu;
                    pouchVu_ = (u_ >= fu) ? 0.45f : -0.45f;
                    pouchVz_ = -0.35f;
                } else if (carrying_) {
                    float away = (u_ >= fu) ? 1.f : -1.f;
                    dropPouch(away);
                    if (std::fabs(pouchU_) > kEdge) {
                        lose("THE POUCH IS GONE");
                        return;
                    }
                }
                break;
            }
            if (f.kind == 0 && loose() && f.stun <= 0.f && std::fabs(f.z - pouchZ_) < 0.55f &&
                std::fabs(fu - pouchU_) < 0.28f) {
                f.stun = 0.35f;
                float away = (pouchU_ >= fu) ? 1.f : -1.f;
                pouchVu_ = away * 1.45f;
                pouchVz_ = -0.7f;
                blip(210.f);
                puffAt(pouchZ_, pouchU_);
            }
        }
    }
    if (mode_ != Mode::Play) return;

    if (toss && carrying_ && tossCd_ <= 0.f) {
        tossCd_ = 0.35f;
        float dir = float(face_);
        carrying_ = false;
        pouchU_ = u_ + std::clamp(vu_ * 0.04f, -0.12f, 0.12f);
        pouchZ_ = pz_ + 0.28f * dir;
        pouchVu_ = vu_ * 0.2f;
        pouchVz_ = 3.4f * dir;
        blip(480.f);
        if (std::fabs(pouchU_) > kEdge) {
            lose("THE POUCH IS GONE");
            return;
        }
    }

    if (loose()) {
        float pull = pouchVz_ > 0.35f ? 4.5f : 5.4f;
        pouchVz_ = approach(pouchVz_, -2.05f, pull * kDt);
        pouchVu_ = approach(pouchVu_, 0.f, 1.15f * kDt);
        pouchZ_ += pouchVz_ * kDt;
        pouchU_ += pouchVu_ * kDt;
        if (std::fabs(pouchU_) > kEdge) {
            lose("THE POUCH IS GONE");
            return;
        }
        if (pouchZ_ < 0.45f) {
            lose("THE POUCH FELL BACK");
            return;
        }
        if (pouchZ_ > kFar + 1.35f) {
            lose("IT WENT WITHOUT YOU");
            return;
        }
    }

    if (loose() && stun_ <= 0.f && std::fabs(pz_ - pouchZ_) < 0.52f && std::fabs(u_ - pouchU_) < 0.34f) grab();

    if (!carrying_ && !planted_ && pz_ >= kPlant) {
        lose("CROSSED WITHOUT IT");
        return;
    }
    pz_ = std::clamp(pz_, 0.4f, kFar + 0.2f);

    bool inBox = carrying_ && pz_ >= kPlant && std::fabs(u_) < 0.48f;
    bool nearBox = carrying_ && pz_ >= kPlant - 0.85f && std::fabs(u_) < 0.55f;
    if (inBox || (plantNow && nearBox)) {
        plant();
        return;
    }

    if (carrying_) {
        pouchZ_ = pz_;
        pouchU_ = u_;
        pouchVz_ = pouchVu_ = 0.f;
    }

    watch_ += kDt;
    int sec = int(std::ceil(std::max(0.f, kWatch - watch_)));
    if (sec != lastSec_ && sec <= 5) blip(sec <= 2 ? 880.f : 520.f);
    lastSec_ = sec;
    if (watch_ >= kWatch) {
        lose("THE WATCH IS OVER");
        return;
    }

    if ((int(t_ * 9.f) & 1) && rush_ > 0.45f && puffs_.size() < 8) puffAt(pz_, u_);
}

void Game::serviceAudio() {
    if (!sys_) return;
    if (beep_ > 0.f) {
        beep_ -= kDt;
        if (beep_ <= 0.f) sys_->apu.tone(0, 0.f, 0.f);
    }
    float wind = std::fabs(windNow());
    if (wind > 0.25f) sys_->apu.noise(0.04f * wind, 880.f + wind * 700.f, false);
    else if (mode_ == Mode::Play && rush_ > 0.2f) sys_->apu.noise(0.012f, 480.f, true);
    else sys_->apu.noise(0.f, 400.f, false);

    if (mode_ == Mode::Play && (rush_ > 0.35f || stun_ > 0.f)) sys_->apu.tone(2, 58.f, 0.022f);
    else sys_->apu.tone(2, 0.f, 0.f);

    if (fanStep_ < 0) return;
    fanT_ += kDt;
    if (fanT_ < 0.12f) return;
    static const float good[] = {392.f, 523.3f, 659.3f, 784.f};
    static const float bad[] = {196.f, 155.6f, 123.5f};
    const float* notes = fanGood_ ? good : bad;
    int n = fanGood_ ? 4 : 3;
    if (fanStep_ < n) sys_->apu.tone(1, notes[fanStep_], 0.08f);
    else sys_->apu.tone(1, 0.f, 0.f);
    ++fanStep_;
    fanT_ = 0.f;
    if (fanStep_ > n + 3) fanStep_ = -1;
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

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    float width = 0.f;
    for (unsigned char c : s) {
        if (c < 33 || c > 126) width += 10.f * scale;
        else width += float(art_.glyph[c - 32].w) * scale;
    }
    x -= width * 0.5f;
    for (unsigned char c : s) {
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
    uint16_t skyTop = gs::rgb4(1, 2, 6);
    uint16_t skyHor = gs::rgb4(12, 6, 3);
    if (mode_ == Mode::Fail) skyHor = gs::rgb4(9, 2, 2);
    else if (mode_ == Mode::Victory) skyHor = gs::rgb4(13, 10, 5);
    v.setFogColor(skyHor);
    int hor = horizon();
    for (int y = 0; y < gs::SCREEN_H; ++y) {
        gs::RoadLine& r = v.road[y];
        if (y <= hor) {
            float t = float(y) / float(std::max(hor, 1));
            v.lineBackdrop[y] = mix(skyTop, skyHor, t * t * t);
            v.lineFog[y] = 0;
            r.on = false;
            continue;
        }
        float row = float(y - hor);
        float vz = kZScale / std::max(row, 0.5f);
        float wz = worldAt(row);
        r.on = true;
        r.cx = 160.f + bendAt(row) + shx_;
        r.hw = halfAt(row);
        r.v = scroll_ + vz * 7.f;
        r.pal = uint8_t(PAL_FIELD);
        r.style = gs::ROAD_ROCKY;
        r.band = (int(std::floor(wz * 0.45f)) & 1) ? 1 : 0;
        r.left = r.right = gs::GROUND_DROP;
        v.lineFog[y] = uint8_t(std::clamp(int(10.f - row * 0.065f), 0, 10));
        v.lineBackdrop[y] = mix(gs::rgb4(3, 3, 4), gs::rgb4(1, 1, 2), std::clamp(row / 150.f, 0.f, 1.f));
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

    int flutter = int(t_ * 7.f) & 1;
    if (mode_ == Mode::Title) text("S3 RIDGE POUC", 160.f + shx_, 28.f, 0.86f, PAL_AMBER);
    else if (mode_ == Mode::Victory) text("IT CROSSED", 160.f + shx_, 30.f, 1.02f, PAL_GOOD);
    else if (mode_ == Mode::Fail)
        text(reason_, 160.f + shx_, 30.f, std::strlen(reason_) > 16 ? 0.68f : 0.9f, PAL_ALERT);
    else if (mode_ == Mode::Pause) text("PAUSED", 160.f + shx_, 32.f, 1.02f, PAL_AMBER);

    struct Item {
        float z;
        int kind;
        int id;
    };
    std::vector<Item> items;
    items.push_back({kZLine - 0.02f, 0, 0});
    if (!carrying_) {
        float vz = viewZ(pouchZ_);
        if (vz > 0.95f) items.push_back({vz, 1, 0});
    }
    for (int i = 0; i < int(foes_.size()); ++i) {
        if (!foes_[size_t(i)].on) continue;
        float vz = viewZ(foes_[size_t(i)].z);
        if (vz > kZLine - 0.2f) items.push_back({vz, 2, i});
    }
    for (int i = 0; i < int(props_.size()); ++i) {
        float vz = viewZ(props_[size_t(i)].z);
        if (vz > kZLine - 0.15f) items.push_back({vz, 3, i});
    }
    for (int i = 0; i < int(puffs_.size()); ++i) {
        float vz = viewZ(puffs_[size_t(i)].z);
        if (vz > 0.95f) items.push_back({vz, 4, i});
    }
    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) { return a.z < b.z; });

    bool flip = vu_ < -0.18f;
    int fr = 0;
    if ((mode_ == Mode::Title || rush_ > 0.25f || std::fabs(vu_) > 0.45f) && stun_ <= 0.f) fr = int(t_ * 8.f) & 1;

    for (const Item& it : items) {
        if (it.kind == 0) {
            float base = mode_ == Mode::Victory ? 78.f : 94.f;
            Spot you = spot(u_, kZLine, base);
            if (!you.ok) continue;
            const gs::Mipped& body = mode_ == Mode::Victory ? art_.kneel : art_.you[fr];
            if (carrying_ && mode_ != Mode::Victory) {
                float side = flip ? -1.f : 1.f;
                float bob = std::sin(t_ * 8.f) * 2.2f;
                spr(art_.pouch[flutter], you.x + side * you.h * 0.16f, you.y - you.h * 0.48f + bob, you.h * 0.36f,
                    PAL_POUCH, false, 0, false);
            }
            spr(body, you.x, you.y, you.h, PAL_YOU, flip, 0, true);
            shadow(you.x, you.y, you.h * 0.40f);
            continue;
        }
        if (it.kind == 1) {
            Spot s = spot(pouchU_, it.z, 42.f);
            if (!s.ok) continue;
            float hop = std::max(0.f, pouchVz_) * 7.f;
            float rest = planted_ ? s.h * 0.62f : 0.f;
            shadow(s.x, s.y, s.h * 0.45f);
            spr(art_.pouch[flutter], s.x, s.y - hop - rest, s.h, PAL_POUCH, false, s.fog, true);
            if (!planted_ && s.h > 22.f && (mode_ == Mode::Play || mode_ == Mode::Title))
                text("POUCH", s.x, s.y - s.h - hop - 8.f, 0.40f, PAL_AMBER);
            continue;
        }
        if (it.kind == 2) {
            const Foe& f = foes_[size_t(it.id)];
            float fu = foeU(f);
            float tall = f.kind == 1 ? 76.f : 84.f;
            Spot s = spot(fu, it.z, tall);
            if (!s.ok) continue;
            int step = int(t_ * 6.f + f.phase) & 1;
            const gs::Mipped& body = f.kind == 1 ? art_.sneak[step] : art_.foe[step];
            int pal = f.kind == 1 ? PAL_SNEAK : PAL_FOE;
            if (f.hasPouch) {
                spr(art_.pouch[flutter], s.x, s.y - s.h * 0.62f, s.h * 0.32f, PAL_POUCH, false, s.fog, false);
            }
            spr(body, s.x, s.y, s.h, pal, fu > 0.f, s.fog, true);
            shadow(s.x, s.y, s.h * 0.34f);
            continue;
        }
        if (it.kind == 3) {
            const Prop& p = props_[size_t(it.id)];
            Spot s = spot(p.u, it.z, p.h);
            if (!s.ok) continue;
            const gs::Mipped& img = p.kind == 1 ? art_.cairn : p.kind == 2 ? art_.rock : art_.post;
            spr(img, s.x, s.y, s.h, PAL_STONE, p.u > 0.f, s.fog, true);
            shadow(s.x, s.y, s.h * (p.kind == 2 ? 0.7f : 0.36f));
            if (p.kind == 1 && s.h > 34.f && mode_ != Mode::Victory && mode_ != Mode::Fail)
                text("CAIRN", s.x, s.y - s.h - 6.f, 0.42f, carrying_ ? PAL_AMBER : PAL_ALERT);
            continue;
        }
        const Puff& p = puffs_[size_t(it.id)];
        Spot s = spot(p.u, it.z, 24.f);
        if (!s.ok) continue;
        float k = std::clamp(p.t / 0.36f, 0.f, 1.f);
        spr(art_.dust, s.x, s.y - (1.f - k) * 6.f, s.h * (0.55f + k), PAL_FX, false, int((1.f - k) * 4.f), false);
    }

    int hor = horizon();
    spr(art_.cloud, std::fmod(t_ * 7.f, 380.f) - 40.f, 20.f, 15.f, PAL_FX, false, 4, false);
    spr(art_.cloud, std::fmod(t_ * 7.f + 200.f, 380.f) - 30.f, 32.f, 11.f, PAL_FX, true, 5, false);
    spr(art_.sun, 248.f, 18.f, 20.f, PAL_FX, false, 1, false);
    spr(art_.peak[0], 58.f + shx_ * 0.15f, float(hor) + 2.f, 52.f, PAL_MOUNT, false, 3, true);
    spr(art_.peak[1], 268.f + shx_ * 0.15f, float(hor) + 4.f, 40.f, PAL_MOUNT, false, 4, true);

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(21, "CARRY THE POUCH ACROSS", PAL_AMBER);
        hudC(22, "ANYTHING ELSE IS A LOSS", PAL_ALERT);
        hudC(24, "ARROWS RUN THE SPINE", PAL_HUD);
        hudC(25, "C OR Z PLANTS IT ON THE CAIRN", PAL_AMBER);
        hudC(26, "X TOSSES IT   EMPTY CROSSES LOSE", PAL_HUD);
        if ((int(t_ * 2.f) & 1) == 0) hudC(27, "PRESS START", PAL_AMBER);
        hud(39 - int(std::strlen(S3_VERSION_STRING)), 0, S3_VERSION_STRING, PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        hudC(24, "START RESUMES", PAL_HUD);
        hudC(26, "ESC TITLE", PAL_HUD);
    } else if (mode_ == Mode::Victory) {
        hudC(23, "THE POUCH CROSSED THE RIDGE", PAL_GOOD);
        hudC(24, "THAT WAS THE JOB", PAL_AMBER);
        hudC(26, "START", PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        hudC(23, "ANYTHING ELSE IS A LOSS", PAL_ALERT);
        hudC(24, reason_, PAL_HUD);
        hudC(26, "START RETRIES", PAL_HUD);
    } else {
        int left = int(std::ceil(std::max(0.f, kWatch - watch_)));
        std::snprintf(buf, sizeof buf, "WATCH %d", left);
        hud(1, 0, buf, left <= 5 ? PAL_ALERT : PAL_HUD);
        if (carrying_) {
            int paces = int(std::ceil(std::max(0.f, kPlant - pz_)));
            std::snprintf(buf, sizeof buf, "POUCH  LEFT %d", paces);
            hud(39 - int(std::strlen(buf)), 0, buf, PAL_AMBER);
        } else if (stolen()) {
            hud(39 - 12, 0, "THEY HAVE IT", PAL_ALERT);
        } else {
            hud(39 - 5, 0, "LOOSE", PAL_ALERT);
        }
        const char* hint = "CARRY IT ACROSS";
        int pal = PAL_HUD;
        if (slip_ > 0.02f) {
            hint = "THE DROP";
            pal = PAL_ALERT;
        } else if (std::fabs(windNow()) > 0.4f) {
            hint = "GUST";
            pal = PAL_AMBER;
        } else if (!carrying_ && stolen()) {
            hint = "TAKE IT BACK";
            pal = PAL_ALERT;
        } else if (!carrying_) {
            hint = "GET THE POUCH";
            pal = PAL_AMBER;
        } else if (pz_ >= kPlant - 2.4f) {
            hint = "PLANT IT";
            pal = PAL_GOOD;
        } else if (pz_ > 13.f && pz_ < 24.f) {
            hint = "ROCKS ON THE SPINE";
            pal = PAL_HUD;
        }
        hudC(1, hint, pal);
        hudC(27, "ARROWS RUN   C PLANTS   X TOSSES", PAL_HUD);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.apu.setMaster(0.85f);
    sys.apu.setEcho(0.14f, 0.2f, 0.1f);
    if (bot_) begin();
    else bootTitle();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 0.85f);
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        float prev = u_;
        u_ = std::sin(t_ * 0.65f) * 0.22f;
        vu_ = (u_ - prev) / kDt;
        rush_ = 0.45f;
        face_ = 1;
        scroll_ += 18.f * kDt;
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_Z) ||
            pad.pressed(gs::BTN_B))
            begin();
        else if (pad.pressed(gs::BTN_MODE) && !bot_) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        }
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            vu_ = 0.f;
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            bootTitle();
        } else {
            update();
        }
    } else if (mode_ == Mode::Pause) {
        vu_ = 0.f;
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
        else if (pad.pressed(gs::BTN_MODE)) bootTitle();
    } else {
        hold_ -= kDt;
        vu_ *= 0.9f;
        if (hold_ <= 0.f) over_ = true;
        if (!bot_ && pad.pressed(gs::BTN_START)) begin();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) bootTitle();
    }

    for (Puff& p : puffs_) p.t -= kDt;
    puffs_.erase(std::remove_if(puffs_.begin(), puffs_.end(), [](const Puff& p) { return p.t <= 0.f; }), puffs_.end());

    serviceAudio();
    draw();
    if (won_) sys.setLight(40, 150, 70);
    else if (mode_ == Mode::Fail) sys.setLight(170, 36, 28);
    else if (mode_ == Mode::Play && slip_ > 0.02f) sys.setLight(170, 50, 30);
    else if (mode_ == Mode::Play && carrying_) sys.setLight(170, 110, 40);
    else if (mode_ == Mode::Play) sys.setLight(70, 90, 140);
    else if (mode_ == Mode::Title) sys.setLight(40, 48, 96);
    else sys.setLight(150, 100, 48);
}

}  // namespace rpouc

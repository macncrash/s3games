#include "game/sled.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "version.h"

namespace sled {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float TAU = 6.2831853f;
constexpr float PI = 3.14159265f;
constexpr float COURSE = 2460.f;
constexpr float LAMP = 112.f;
constexpr float HORIZON = 96.f;
constexpr float PROJ_K = 230.f;
constexpr float ROAD_P = 300.f;
constexpr float CURVE_K = 0.014f;
constexpr float PULL_K = 0.018f;
constexpr float LEAD_MAX = 0.48f;
constexpr float TOW = 3.2f;
constexpr int BEND_N = 80;
constexpr float BEND_DZ = 4.f;

constexpr float GATE_S[5] = {500.f, 980.f, 1440.f, 1900.f, 2300.f};
constexpr float GATE_X[5] = {0.78f, -0.78f, 0.06f, -0.72f, 0.f};
constexpr float GATE_OPEN[5] = {0.48f, 0.48f, 0.58f, 0.44f, 0.86f};
constexpr bool GATE_FINISH[5] = {false, false, false, false, true};
constexpr const char* GATE_NAME[5] = {"BIRCH", "FORK", "CREEK", "RIDGE", "LANTERN"};
constexpr float GATE_NOTE[4] = {523.f, 659.f, 784.f, 880.f};

struct DogSlot {
    float sOff, xOff, phase;
    int lead;
};
constexpr DogSlot SLOTS[6] = {
    {5.8f, -0.10f, 0.00f, 1}, {5.9f, 0.10f, 0.33f, 1}, {4.5f, -0.16f, 0.16f, 0},
    {4.6f, 0.16f, 0.50f, 0},  {3.25f, -0.22f, 0.22f, 0}, {3.35f, 0.22f, 0.66f, 0},
};

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    const int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    const int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    return gs::rgb4(int(std::lround(ar + (br - ar) * t)), int(std::lround(ag + (bg - ag) * t)),
                     int(std::lround(ab + (bb - ab) * t)));
}

void formatClock(char* dst, int n, float t) {
    if (t < 0.f) t = 0.f;
    const int cs = int(std::lround(t * 100.f));
    std::snprintf(dst, size_t(n), "%d:%02d.%02d", cs / 6000, (cs / 100) % 60, cs % 100);
}

gs::FMPatch windPatch() {
    gs::FMPatch p;
    p.alg = 4;
    p.fb = 0.12f;
    p.vol = 0.16f;
    p.drive = 0.06f;
    p.tone = 900.f;
    p.vibRate = 0.22f;
    p.vibDepth = 0.01f;
    p.echo = 0.28f;
    p.glide = 0.004f;
    p.op[0] = {1.f, 0.4f, 0.08f, 0.5f, 0.75f, 0.4f, 0.f};
    p.op[1] = {1.f, 0.85f, 0.05f, 0.55f, 0.85f, 0.45f, 0.f};
    p.op[2] = {2.f, 0.22f, 0.1f, 0.45f, 0.4f, 0.4f, 1.2f};
    p.op[3] = {1.5f, 0.5f, 0.06f, 0.4f, 0.7f, 0.4f, 0.f};
    return p;
}

gs::FMPatch bellPatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.04f;
    p.vol = 0.22f;
    p.echo = 0.42f;
    p.tone = 3200.f;
    p.op[0] = {1.f, 0.7f, 0.004f, 0.16f, 0.f, 0.28f, 0.f};
    p.op[1] = {2.f, 0.45f, 0.004f, 0.12f, 0.f, 0.22f, 0.f};
    p.op[2] = {3.4f, 0.3f, 0.004f, 0.1f, 0.f, 0.18f, 0.f};
    p.op[3] = {5.1f, 0.18f, 0.004f, 0.08f, 0.f, 0.16f, 0.f};
    return p;
}

}  // namespace

int Game::gatesTaken() const {
    int n = 0;
    for (const Gate& g : gates_)
        if (g.taken) n++;
    return n;
}

int Game::marker() const {
    if (mode_ == Mode::Victory || mode_ == Mode::Fail) return 4;
    if (mode_ != Mode::Run) return 0;
    const int g = nextGate();
    if (g == 2) return 2;
    if (g == 4 && gates_[4].s - s_ < 55.f) return 3;
    return 1;
}

const char* Game::outcome() const {
    if (won_) return "lantern lit";
    if (why_ == Why::Post) return "hit the post";
    if (why_ == Why::Open) return "gate open";
    if (why_ == Why::Lamp) return "lamp out";
    return "unfinished";
}

void Game::formatElapsed(char* dst, int n) const { formatClock(dst, n, elapsed_); }

bool Game::iceAt(float s) const {
    if (s > 600.f && s < 760.f) return true;
    if (s > 1120.f && s < 1280.f) return true;
    if (s > 1560.f && s < 1720.f) return true;
    return false;
}

bool Game::nearGate(float s, float r) const {
    for (const Gate& g : gates_)
        if (std::fabs(g.s - s) < r) return true;
    return false;
}

float Game::curvature(float s) const {
    const float u = s / COURSE;
    float c = 0.55f * std::sin(u * TAU * 1.6f);
    c += 0.32f * std::sin(u * TAU * 3.2f + 1.3f);
    c += 0.18f * std::sin(u * TAU * 5.1f + 2.2f);
    c *= std::sin(std::clamp(u, 0.f, 1.f) * PI);
    if (s < 100.f) c *= std::max(0.f, s / 100.f);
    if (s > COURSE - 180.f) c *= std::max(0.f, (COURSE - s) / 180.f);
    for (const Gate& g : gates_) {
        const float d = std::fabs(s - g.s);
        if (d < 70.f) c *= d / 70.f;
    }
    if (iceAt(s)) c *= 0.22f;
    return std::clamp(c, -1.f, 1.f);
}

int Game::nextGate() const {
    for (int i = 0; i < 5; i++)
        if (!gates_[i].taken && !gates_[i].missed && gates_[i].s >= s_ - 1.f) return i;
    return -1;
}

void Game::setupGates() {
    for (int i = 0; i < 5; i++) {
        gates_[i].s = GATE_S[i];
        gates_[i].x = GATE_X[i];
        gates_[i].open = GATE_OPEN[i];
        gates_[i].finish = GATE_FINISH[i];
        gates_[i].name = GATE_NAME[i];
        gates_[i].taken = false;
        gates_[i].missed = false;
    }
}

void Game::buildScenery() {
    scenery_.clear();
    uint32_t h = 0x4C1DF00Du;
    auto rnd = [&]() {
        h = h * 1664525u + 1013904223u;
        return h >> 8;
    };
    for (float s = 28.f; s < COURSE - 50.f; s += 13.f) {
        if (nearGate(s, 18.f)) continue;
        const float side = (rnd() & 1) ? 1.f : -1.f;
        const float mag = 1.58f + float(rnd() % 100) * 0.0065f;
        scenery_.push_back({s, side * mag, (rnd() % 7 == 0) ? 1 : 0});
        if ((rnd() % 3) == 0) {
            const float mag2 = 1.66f + float(rnd() % 70) * 0.006f;
            scenery_.push_back({s + 5.5f, -side * mag2, 0});
        }
    }
    scenery_.push_back({720.f, 1.92f, 2});
    scenery_.push_back({1680.f, -1.96f, 2});
    scenery_.push_back({2348.f, 1.9f, 3});
}

void Game::enterTitle() {
    mode_ = Mode::Title;
    why_ = Why::None;
    won_ = false;
    over_ = false;
    newBest_ = false;
    spills_ = 0;
    elapsed_ = 0;
    s_ = 24.f;
    x_ = 0;
    vx_ = 0;
    lead_ = 0;
    speed_ = 0;
    breath_ = 1.f;
    hike_ = 0;
    stun_ = 0;
    shake_ = 0;
    fan_ = -1;
    missName_ = "";
    pops_.clear();
    for (Gate& g : gates_) g.taken = g.missed = false;
}

void Game::resetRun() {
    mode_ = Mode::Run;
    why_ = Why::None;
    won_ = false;
    over_ = false;
    newBest_ = false;
    spills_ = 0;
    elapsed_ = 0;
    s_ = 0;
    x_ = 0;
    vx_ = 0;
    lead_ = 0;
    speed_ = 12.f;
    breath_ = 1.f;
    hike_ = 0;
    stun_ = 0;
    shake_ = 0;
    fan_ = -1;
    missName_ = "";
    pops_.clear();
    for (Gate& g : gates_) g.taken = g.missed = false;
    pop("MUSH", PAL_GOLD);
}

void Game::loadBest() {
    if (!sys_ || sys_->headless || bot_) return;
    const std::string s = sys_->loadBlob("s3dogsled.txt");
    if (s.empty()) return;
    const float v = std::strtof(s.c_str(), nullptr);
    if (v > 1.f && v < 600.f) best_ = v;
}

void Game::saveBest() {
    if (!sys_ || bot_ || sys_->headless) return;
    if (best_ > 0.f && elapsed_ + 0.005f >= best_) return;
    newBest_ = true;
    best_ = elapsed_;
    char b[32];
    std::snprintf(b, sizeof b, "%.2f", best_);
    sys_->saveBlob("s3dogsled.txt", b);
}

void Game::chime(float freq) {
    sys_->apu.keyOn(1, freq, 0.22f);
    sys_->apu.tone(2, freq * 0.5f, 0.045f);
    blip_ = 0.08f;
    sys_->rumble(0.15f, 0.35f, 70);
}

void Game::yip() {
    sys_->apu.tone(1, 740.f, 0.05f);
    yipT_ = 0.07f;
}

void Game::fanfareTick() {
    if (fan_ < 0) return;
    fanT_ += DT;
    if (fanT_ < 0.13f) return;
    fanT_ = 0;
    static const float notes[] = {523.f, 659.f, 784.f, 1046.f, 784.f, 1174.f};
    if (fan_ < 6) sys_->apu.keyOn(1, notes[fan_], 0.2f);
    if (++fan_ > 8) fan_ = -1;
}

void Game::pop(const char* s, int pal) {
    if (pops_.size() > 4) pops_.erase(pops_.begin());
    pops_.push_back({74.f, 1.2f, pal, s});
}

void Game::spill() {
    if (stun_ > 0.f || mode_ != Mode::Run) return;
    spills_++;
    stun_ = 0.85f;
    speed_ *= 0.32f;
    shake_ = 1.f;
    x_ *= 0.72f;
    lead_ *= 0.35f;
    vx_ *= 0.15f;
    sys_->apu.noiseBurst(0.42f, 650.f, 0.22f);
    sys_->rumble(0.75f, 0.3f, 180);
    pop("SPILL", PAL_RED);
}

void Game::win() {
    mode_ = Mode::Victory;
    won_ = true;
    why_ = Why::None;
    fan_ = 0;
    fanT_ = 0;
    sys_->apu.keyOn(1, 1046.f, 0.24f);
    sys_->rumble(0.3f, 0.75f, 280);
    saveBest();
    if (bot_) over_ = true;
}

void Game::fail(Why why, const char* name) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    why_ = why;
    won_ = false;
    missName_ = name ? name : "";
    speed_ *= 0.2f;
    shake_ = why == Why::Lamp ? 0.3f : 1.f;
    sys_->apu.tone(2, 120.f, 0.07f);
    blip_ = 0.22f;
    sys_->apu.noiseBurst(0.3f, 180.f, 0.28f);
    sys_->rumble(0.5f, 0.2f, 160);
    if (bot_) over_ = true;
}

Game::Cmd Game::botCmd() const {
    Cmd c;
    c.mush = true;
    const int gi = nextGate();
    float target = 0.f;
    float distG = 999.f;
    if (gi >= 0) {
        distG = gates_[gi].s - s_;
        float blend = 1.f - std::clamp(distG / 180.f, 0.f, 1.f);
        blend = blend * blend * (3.f - 2.f * blend);
        target = gates_[gi].x * blend;
    }
    if (std::fabs(x_) > 0.9f) target = 0.f;
    const float err = target - x_;
    const float pull = curvature(s_) * speed_ * PULL_K;
    const float deep = std::max(0.f, std::fabs(x_) - 1.02f);
    const float grip = iceAt(s_) ? 0.4f : (deep > 0.15f ? 0.6f : 1.f);
    const float desire = std::clamp(err * 2.5f - vx_ * 1.75f, -1.15f, 1.15f);
    const float leadWant = (desire + pull) / TOW;
    c.steer = std::clamp(leadWant / std::max(0.15f, LEAD_MAX * grip), -1.f, 1.f);
    const float look = std::fabs(curvature(s_ + 22.f));
    if (look > 0.72f && speed_ > 27.f) {
        c.mush = false;
        c.whoa = speed_ > 30.f;
    }
    if (distG < 50.f && std::fabs(err) > 0.16f && speed_ > 20.f) {
        c.mush = false;
        c.whoa = speed_ > 22.f;
    }
    if (std::fabs(x_) > 0.92f) {
        c.mush = false;
        c.whoa = speed_ > 16.f;
    }
    if (stun_ > 0.f) {
        c.steer = 0;
        c.mush = false;
        c.whoa = false;
    }
    return c;
}

Game::Cmd Game::command(const gs::Pad& pad) const {
    if (bot_) return botCmd();
    Cmd c;
    c.steer = std::fabs(pad.axisX) > 0.08f ? pad.axisX : float(pad.down(gs::BTN_RIGHT)) - float(pad.down(gs::BTN_LEFT));
    c.mush = pad.down(gs::BTN_UP) || pad.down(gs::BTN_C) || pad.down(gs::BTN_A) || pad.accel > 0.35f;
    c.whoa = pad.down(gs::BTN_DOWN) || pad.down(gs::BTN_B) || pad.brake > 0.35f;
    c.hike = pad.pressed(gs::BTN_TURBO);
    if (c.whoa) c.mush = false;
    c.steer = std::clamp(c.steer, -1.f, 1.f);
    return c;
}

void Game::updateTitle(const gs::Pad& pad) {
    s_ += 15.f * DT;
    if (s_ > 390.f) s_ = 28.f;
    x_ = std::sin(t_ * 0.55f) * 0.2f;
    lead_ = std::sin(t_ * 0.55f + 0.45f) * 0.16f;
    vx_ = std::cos(t_ * 0.55f) * 0.1f;
    paw_ += 14.f * DT * 0.085f;
    if (pad.pressed(gs::BTN_START)) {
        chime(660.f);
        resetRun();
    } else if (pad.pressed(gs::BTN_MODE)) {
        if (sys_->hasHome()) sys_->eject();
        else sys_->quit();
    }
}

void Game::updatePause(const gs::Pad& pad) {
    if (pad.pressed(gs::BTN_START)) {
        mode_ = Mode::Run;
        chime(520.f);
    } else if (pad.pressed(gs::BTN_MODE)) {
        enterTitle();
    }
}

void Game::updateResult(const gs::Pad& pad) {
    speed_ = std::max(0.f, speed_ - 16.f * DT);
    lead_ *= 1.f - std::min(1.f, 2.2f * DT);
    paw_ += std::max(6.f, speed_) * DT * 0.08f;
    s_ += speed_ * DT;
    if (mode_ == Mode::Victory) fanfareTick();
    if (bot_) {
        over_ = true;
        return;
    }
    if (pad.pressed(gs::BTN_START)) resetRun();
    else if (pad.pressed(gs::BTN_MODE)) enterTitle();
}

void Game::updateRun(const gs::Pad& pad) {
    if (!bot_ && pad.pressed(gs::BTN_START)) {
        mode_ = Mode::Pause;
        return;
    }
    Cmd cmd = command(pad);
    if (hike_ > 0.f) hike_ -= DT;
    if (cmd.hike && hike_ <= 0.f && breath_ >= 0.34f && stun_ <= 0.f) {
        hike_ = 0.85f;
        breath_ -= 0.34f;
        yip();
        pop("HIKE", PAL_GOLD);
    }
    breath_ = std::min(1.f, breath_ + DT * (hike_ > 0.f ? 0.04f : 0.15f));

    const float deep = std::max(0.f, std::fabs(x_) - 1.02f);
    const bool ice = iceAt(s_);
    if (stun_ > 0.f) {
        stun_ -= DT;
        cmd.steer = 0;
        cmd.mush = false;
        cmd.whoa = false;
    }
    float top = 18.f;
    float acc = 7.f;
    if (cmd.mush) {
        top = hike_ > 0.f ? 40.f : 32.f;
        acc = hike_ > 0.f ? 20.f : 13.f;
    }
    if (cmd.whoa) {
        top = 0.f;
        acc = 26.f;
    }
    if (stun_ > 0.f) top = std::min(top, 7.f);
    if (speed_ < top) speed_ = std::min(top, speed_ + acc * DT);
    else speed_ = std::max(top, speed_ - acc * DT);
    if (deep > 0.f) speed_ = std::max(6.5f, speed_ - deep * 22.f * DT);

    const float grip = ice ? 0.4f : (deep > 0.15f ? 0.6f : 1.f);
    const float wantLead = cmd.steer * LEAD_MAX * grip;
    lead_ += (wantLead - lead_) * std::min(1.f, (ice ? 2.6f : 6.5f) * DT);
    const float pull = curvature(s_) * speed_ * PULL_K;
    const float targetVx = lead_ * TOW - pull;
    vx_ += (targetVx - vx_) * std::min(1.f, (ice ? 2.4f : 6.f) * DT);
    x_ += vx_ * DT;
    x_ = std::clamp(x_, -2.35f, 2.35f);

    const float s0 = s_;
    s_ += speed_ * DT;
    elapsed_ += DT;
    paw_ += speed_ * DT * 0.085f;

    for (const Prop& p : scenery_) {
        if (p.kind > 1) continue;
        if (s0 < p.s && s_ >= p.s) {
            const float rad = p.kind == 0 ? 0.14f : 0.11f;
            if (std::fabs(x_ - p.x) < rad) spill();
        }
    }
    if (mode_ != Mode::Run) return;

    for (int i = 0; i < 5; i++) {
        Gate& g = gates_[i];
        if (g.taken || g.missed) continue;
        if (!(s0 < g.s && s_ >= g.s)) continue;
        const float halfW = g.open / GATE_OPEN_FRAC;
        const float postAt = halfW * GATE_POST_FRAC;
        const float postHalf = halfW * GATE_POST_HALF_FRAC;
        const float adx = std::fabs(x_ - g.x);
        if (adx <= g.open) {
            g.taken = true;
            if (g.finish) {
                bool all = true;
                for (const Gate& o : gates_)
                    if (!o.taken) all = false;
                if (all) win();
                else fail(Why::Open, g.name);
            } else {
                chime(GATE_NOTE[i]);
                pop(g.name, PAL_GREEN);
            }
        } else if (std::fabs(adx - postAt) <= postHalf + 0.02f) {
            g.missed = true;
            spills_++;
            s_ = g.s - 8.f;
            speed_ = 0;
            fail(Why::Post, g.name);
        } else {
            g.missed = true;
            s_ = g.s - 8.f;
            speed_ = 0;
            fail(Why::Open, g.name);
        }
        break;
    }
    if (mode_ == Mode::Run && elapsed_ >= LAMP) fail(Why::Lamp, nullptr);

    puffAcc_ += DT;
    const float every = (deep > 0.f || cmd.whoa) ? 0.045f : 0.1f;
    if (speed_ > 8.f && puffAcc_ >= every) {
        puffAcc_ = 0;
        for (int k = 0; k < 2; k++) {
            Puff& p = puffs_[puffI_++ & 15];
            const float j = std::sin(t_ * 17.f + float(puffI_) * 1.7f) * 10.f;
            p.x = 160.f + lead_ * 20.f + (k ? 16.f : -16.f) + j * 0.3f;
            p.y = 200.f;
            p.vx = (k ? 18.f : -18.f) + j;
            p.vy = -28.f - speed_ * 0.4f;
            p.t = 0.42f;
        }
    }
}

void Game::updateFlakes() {
    for (Flake& f : flakes_) {
        f.y += f.sp * DT;
        f.x += std::sin(t_ * 1.3f + f.y * 0.02f) * 12.f * DT;
        if (f.y > gs::SCREEN_H + 4) {
            f.y = -4.f;
            f.x = std::fmod(f.x + 80.f, float(gs::SCREEN_W));
            if (f.x < 0) f.x += gs::SCREEN_W;
        }
    }
}

void Game::updatePops() {
    for (Pop& p : pops_) p.t -= DT;
    pops_.erase(std::remove_if(pops_.begin(), pops_.end(), [](const Pop& p) { return p.t <= 0.f; }), pops_.end());
    for (Puff& p : puffs_) {
        if (p.t <= 0.f) continue;
        p.t -= DT;
        p.x += p.vx * DT;
        p.y += p.vy * DT;
    }
}

void Game::lights() {
    int r = 170, g = 100, b = 36;
    if (mode_ == Mode::Victory) {
        r = 40;
        g = 180;
        b = 70;
    } else if (mode_ == Mode::Fail) {
        r = 180;
        g = 28;
        b = 22;
    } else if (mode_ == Mode::Run && iceAt(s_)) {
        r = 40;
        g = 120;
        b = 190;
    } else if (mode_ == Mode::Run && LAMP - elapsed_ < 16.f) {
        r = 190;
        g = 36;
        b = 24;
    }
    if (r == lightR_ && g == lightG_ && b == lightB_) return;
    lightR_ = r;
    lightG_ = g;
    lightB_ = b;
    sys_->setLight(r, g, b);
}

void Game::audio() {
    if (windOn_) {
        const float f = 76.f + speed_ * 2.05f + (hike_ > 0.f ? 26.f : 0.f);
        const float v = (mode_ == Mode::Run || mode_ == Mode::Title) ? 0.06f + std::min(speed_, 36.f) * 0.0032f : 0.035f;
        sys_->apu.setFreq(0, f);
        sys_->apu.setVol(0, v);
    }
    float hiss = 0.f;
    if (mode_ == Mode::Run || mode_ == Mode::Title) hiss = 0.018f + speed_ * 0.0015f;
    if (mode_ == Mode::Run && iceAt(s_)) hiss += 0.018f;
    const bool ice = mode_ == Mode::Run && iceAt(s_);
    sys_->apu.noise(hiss, ice ? 1500.f + speed_ * 28.f : 380.f + speed_ * 50.f, false);
    if (blip_ > 0.f) {
        blip_ -= DT;
        if (blip_ <= 0.f) sys_->apu.tone(2, 0, 0);
    }
    if (yipT_ > 0.f) {
        yipT_ -= DT;
        if (yipT_ <= 0.f) sys_->apu.tone(1, 0, 0);
    }
}

void Game::cacheBend() {
    for (int i = 0; i < BEND_N; i++) {
        const float dist = float(i) * BEND_DZ;
        if (dist <= 0.05f) {
            bendCache_[i] = 0.f;
            continue;
        }
        const int n = 12;
        const float du = dist / float(n);
        float head = 0.f, bend = 0.f;
        for (int k = 0; k < n; k++) {
            const float c = curvature(s_ + (float(k) + 0.5f) * du);
            head += c * CURVE_K * du;
            bend += head * du;
        }
        bendCache_[i] = bend;
    }
}

float Game::bendTo(float dist) const {
    if (dist <= 0.f) return 0.f;
    const float u = dist / BEND_DZ;
    const int i = std::min(BEND_N - 2, std::max(0, int(u)));
    const float f = std::clamp(u - float(i), 0.f, 1.f);
    return bendCache_[i] * (1.f - f) + bendCache_[i + 1] * f;
}

bool Game::project(float wx, float wz, float& sx, float& sy, float& scale, int& fog) const {
    const float z = wz - s_;
    if (z < 0.45f) return false;
    const float hor = HORIZON + camY_;
    sy = hor + PROJ_K / z;
    scale = ROAD_P / z;
    sx = 160.f + (bendTo(z) + wx - x_) * scale + camX_;
    fog = std::clamp(int((z - 16.f) / 7.5f), 0, 13);
    return sy > -30.f && sy < gs::SCREEN_H + 50.f;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet, bool shadow) {
    if (h < 1.5f || m.h <= 0 || m.w <= 0) return;
    const float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 90 || s.x + s.w < -90 || s.y > gs::SCREEN_H + 50 || s.y + s.h < -50) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        const int x = col + int(i);
        const unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    const float adv = 18.f * scale;
    x -= float(s.size()) * adv * 0.5f;
    for (size_t i = 0; i < s.size(); i++) {
        const unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + float(i) * adv + g.w * scale * 0.5f, y, g.h * scale, pal, false, 0, false, false);
    }
}

void Game::drawRoad() {
    gs::VDP& v = sys_->vdp;
    const uint16_t top = gs::rgb4(1, 1, 6);
    const uint16_t mid = gs::rgb4(5, 4, 10);
    const uint16_t horC = gs::rgb4(13, 7, 5);
    v.setFogColor(gs::rgb4(8, 7, 10));
    const float hor = HORIZON + camY_;
    v.B.scroll(int(t_ * 5.f + x_ * 24.f), 0);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (float(y) < hor) {
            const float u = float(y) / std::max(1.f, hor);
            v.lineBackdrop[y] = u < 0.62f ? lerpC(top, mid, u / 0.62f) : lerpC(mid, horC, (u - 0.62f) / 0.38f);
            int hf = 0;
            if (float(y) > hor - 10.f) hf = int((float(y) - (hor - 10.f)) * 0.45f);
            v.lineFog[y] = uint8_t(std::clamp(hf, 0, 6));
            v.road[y].on = false;
            continue;
        }
        const float row = std::max(0.75f, float(y) - hor);
        const float z = PROJ_K / row;
        gs::RoadLine& r = v.road[y];
        const bool ice = iceAt(s_ + z);
        r.on = true;
        r.hw = ROAD_P / z;
        r.cx = 160.f + (bendTo(z) - x_) * r.hw + camX_;
        r.v = (s_ + z) * 3.5f;
        r.pal = uint8_t(ice ? PAL_ICE : PAL_SNOW);
        r.style = uint8_t(ice ? gs::ROAD_ICE : gs::ROAD_SNOW);
        r.band = (int(std::floor((s_ + z) / 24.f)) & 1) ? 1 : 0;
        r.left = r.right = gs::GROUND_SNOWWALL;
        v.lineFog[y] = uint8_t(std::clamp(int((z - 20.f) / 9.f), 0, 11));
        v.lineBackdrop[y] = horC;
    }
}

void Game::drawTeam() {
    const float lean = std::clamp(lead_ * 1.35f + vx_ * 0.4f, -1.f, 1.f);
    const int frame = lean > 0.28f ? 2 : lean < -0.28f ? 0 : 1;
    const float bob = std::sin(paw_ * 2.1f) * (1.6f + speed_ * 0.04f);
    const float sx = 160.f + lead_ * 28.f + camX_;
    const float sy = 212.f + bob + camY_ * 0.3f;
    for (const Puff& p : puffs_) {
        if (p.t <= 0.f) continue;
        const float h = 7.f + (0.42f - p.t) * 16.f;
        spr(art_.puff, p.x + camX_ * 0.2f, p.y, h, PAL_FX, false, 0, false, false);
    }
    spr(art_.sled[frame], sx, sy, 90.f, PAL_SLED, false, 0, true, false);
    spr(art_.shadow, sx + 2.f, sy + 1.f, 16.f, PAL_FX, false, 0, true, true);
}

void Game::drawWorld() {
    struct Item {
        float z;
        int kind;  // 0 prop, 1 gate, 2 dog
        int index;
    };
    std::vector<Item> items;
    items.reserve(48);
    for (int i = 0; i < int(scenery_.size()); i++) {
        const float z = scenery_[size_t(i)].s - s_;
        if (z > 2.4f && z < 100.f) items.push_back({z, 0, i});
    }
    for (int i = 0; i < 5; i++) {
        const float z = gates_[i].s - s_;
        if (z > 0.7f && z < 120.f) items.push_back({z, 1, i});
    }
    for (int i = 0; i < 6; i++) items.push_back({SLOTS[i].sOff, 2, i});
    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) { return a.z < b.z; });

    for (const Item& it : items) {
        if (it.kind == 2) {
            const DogSlot& sl = SLOTS[it.index];
            const float along = std::clamp((sl.sOff - 2.6f) / 3.4f, 0.f, 1.f);
            const float wx = x_ + sl.xOff + lead_ * along;
            const float wz = s_ + sl.sOff;
            float sx, sy, scale;
            int fog;
            if (!project(wx, wz, sx, sy, scale, fog)) continue;
            const float bob = std::sin((paw_ + sl.phase) * TAU) * 1.6f;
            int fr = int(std::floor(paw_ * 3.f + sl.phase * 3.f));
            fr %= 3;
            if (fr < 0) fr += 3;
            spr(art_.dog[fr], sx, sy + bob, 0.5f * scale, sl.lead ? PAL_LEAD : PAL_DOG, sl.xOff > 0, 0, true, false);
            continue;
        }
        if (it.kind == 1) {
            const Gate& g = gates_[it.index];
            float sx, sy, scale;
            int fog;
            if (!project(g.x, g.s, sx, sy, scale, fog)) continue;
            const float halfW = g.open / GATE_OPEN_FRAC;
            const gs::Mipped& img = g.finish ? art_.lantern : art_.gate;
            const float pixW = halfW * 2.f * scale;
            const float pixH = pixW * float(img.h) / float(img.w);
            spr(img, sx, sy, pixH, PAL_GATE, false, fog, true, false);
            if (g.finish) {
                const float pulse = 0.7f + 0.3f * std::sin(t_ * 4.2f);
                spr(art_.puff, sx, sy - pixH * 0.62f, 18.f * pulse * (scale / 40.f + 0.4f), PAL_GOLD, false, fog, false, false);
            }
            if (it.z < 48.f && it.z > 3.f) {
                const char* nm = (g.finish && it.z < 80.f) ? "CHECKPOINT" : g.name;
                text(nm, sx, sy - pixH - 8.f, std::clamp(0.28f + 8.f / it.z, 0.32f, 0.7f), PAL_GOLD);
            }
            continue;
        }
        const Prop& p = scenery_[size_t(it.index)];
        if ((p.kind == 0 || p.kind == 1) && it.z < 3.1f) continue;
        float sx, sy, scale;
        int fog;
        if (!project(p.x, p.s, sx, sy, scale, fog)) continue;
        if (p.kind == 0) spr(art_.spruce, sx, sy, 2.55f * scale, PAL_TREE, p.x < 0, fog, true, false);
        else if (p.kind == 1) spr(art_.rock, sx, sy, 0.55f * scale, PAL_TREE, false, fog, true, false);
        else if (p.kind == 2) spr(art_.hare, sx, sy, 0.42f * scale, PAL_HARE, p.x < 0, fog, true, false);
        else spr(art_.cabin, sx, sy, 2.15f * scale, PAL_CABIN, false, fog, true, false);
    }
}

void Game::drawSkyBits() {
    const float mx = 262.f - x_ * 10.f + camX_ * 0.15f;
    spr(art_.moon, mx, 22.f + camY_ * 0.1f, 22.f, PAL_FX, false, 1, false, false);
    const float a0 = 58.f + std::sin(t_ * 0.25f) * 8.f;
    const float a1 = 196.f + std::sin(t_ * 0.2f + 1.f) * 10.f;
    spr(art_.aurora, a0, 58.f, 64.f, PAL_AURORA, false, 2, false, false);
    spr(art_.aurora, a1, 64.f, 48.f, PAL_AURORA, true, 3, false, false);
    for (const Flake& f : flakes_) spr(art_.flake, f.x, f.y, 2.5f + f.sz, PAL_FX, false, 0, false, false);
}

void Game::drawHud() {
    char buf[48];
    char clk[16];
    if (mode_ == Mode::Title) {
        text("S3 DOGSLED", 160.f, 24.f, 0.92f, PAL_HUD);
        text("THE CHECKPOINT", 160.f, 46.f, 0.62f, PAL_GOLD);
        hudC(7, "THE TEAM TURNS WITH YOU", PAL_GOLD);
        hudC(9, "FIVE GATES  MUSH THE LAMP", PAL_HUD);
        hudC(10, "ARROWS STEER  C MUSH  X WHOA  SPACE HIKE", PAL_HUD);
        if ((int(t_ * 2.f) & 1) == 0) hudC(11, "PRESS START", PAL_GOLD);
        if (best_ > 0.f) {
            formatClock(clk, int(sizeof clk), best_);
            std::snprintf(buf, sizeof buf, "BEST %s", clk);
            hud(1, 0, buf, PAL_GOLD);
        }
        hud(40 - int(std::strlen(S3_VERSION_STRING)), 0, S3_VERSION_STRING, PAL_HUD);
        return;
    }

    if (mode_ == Mode::Run || mode_ == Mode::Pause) {
        const int gi = nextGate();
        if (gi >= 0) {
            const Gate& g = gates_[gi];
            const float dist = std::max(0.f, g.s - s_);
            const char* name = (g.finish && dist < 130.f) ? "CHECKPOINT" : g.name;
            const char* call = "HOLD";
            if (g.x - x_ > 0.1f) call = "GEE";
            else if (x_ - g.x > 0.1f) call = "HAW";
            const int cp = (std::strcmp(call, "HOLD") == 0) ? PAL_HUD : PAL_GOLD;
            std::snprintf(buf, sizeof buf, "%s %s %d", name, call, int(dist));
            hud(1, 0, buf, cp);
        }
        formatClock(clk, int(sizeof clk), std::max(0.f, LAMP - elapsed_));
        const int tp = (LAMP - elapsed_ < 16.f) ? PAL_RED : PAL_HUD;
        hud(40 - int(std::strlen(clk)), 0, clk, tp);

        const int paceN = std::clamp(int(std::lround(std::clamp(speed_ / 32.f, 0.f, 1.f) * 8.f)), 0, 8);
        const int hikeN = std::clamp(int(std::lround(breath_ * 8.f)), 0, 8);
        hud(1, 1, "PACE", hike_ > 0.f ? PAL_GOLD : PAL_HUD);
        for (int i = 0; i < 8; i++) hud(6 + i, 1, i < paceN ? "=" : "-", i < paceN ? PAL_GREEN : PAL_GOLD);
        hud(16, 1, "HIKE", PAL_HUD);
        for (int i = 0; i < 8; i++) hud(21 + i, 1, i < hikeN ? "=" : "-", i < hikeN ? PAL_GOLD : PAL_HUD);
        if (std::fabs(x_) > 1.02f) hud(31, 1, "DEEP", PAL_RED);
        else if (iceAt(s_)) hud(32, 1, "ICE", PAL_GOLD);
        else if (spills_ > 0) {
            std::snprintf(buf, sizeof buf, "X%d", spills_);
            hud(33, 1, buf, PAL_RED);
        }
        if (mode_ == Mode::Pause) {
            text("PAUSE", 160.f, 70.f, 1.f, PAL_HUD);
            hudC(12, "START  RESUME", PAL_HUD);
            hudC(13, "ESC  TITLE", PAL_HUD);
        }
    }

    for (const Pop& p : pops_) {
        const float y = p.y - (1.2f - p.t) * 16.f;
        text(p.text, 160.f, y, 0.58f, p.pal);
    }

    if (mode_ == Mode::Victory) {
        text("THE CHECKPOINT", 160.f, 36.f, 0.7f, PAL_GOLD);
        text("LANTERN LIT", 160.f, 58.f, 0.85f, PAL_GREEN);
        formatElapsed(clk, int(sizeof clk));
        std::snprintf(buf, sizeof buf, "TIME %s", clk);
        hudC(10, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "GATES %d/5   SPILLS %d", gatesTaken(), spills_);
        hudC(12, buf, PAL_HUD);
        if (newBest_) hudC(14, "NEW BEST", PAL_GREEN);
        else if (best_ > 0.f) {
            formatClock(clk, int(sizeof clk), best_);
            std::snprintf(buf, sizeof buf, "BEST %s", clk);
            hudC(14, buf, PAL_GOLD);
        }
        if (!bot_) hudC(16, "START  AGAIN", PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        if (why_ == Why::Lamp) {
            text("LAMP OUT", 160.f, 40.f, 0.9f, PAL_RED);
            hudC(10, "THE LAMP WENT OUT", PAL_HUD);
        } else if (why_ == Why::Post) {
            text("HIT THE POST", 160.f, 40.f, 0.72f, PAL_RED);
            hudC(10, missName_ ? missName_ : "", PAL_GOLD);
        } else {
            text("GATE OPEN", 160.f, 40.f, 0.85f, PAL_RED);
            std::snprintf(buf, sizeof buf, "MISSED %s", missName_ ? missName_ : "");
            hudC(10, buf, PAL_GOLD);
        }
        formatElapsed(clk, int(sizeof clk));
        std::snprintf(buf, sizeof buf, "TIME %s   GATES %d/5", clk, gatesTaken());
        hudC(12, buf, PAL_HUD);
        if (!bot_) {
            hudC(15, "START  RETRY", PAL_HUD);
            hudC(16, "ESC  TITLE", PAL_HUD);
        }
    }
}

void Game::draw() {
    camX_ = camY_ = 0.f;
    if (shake_ > 0.f) {
        camX_ = std::sin(t_ * 71.f) * shake_ * 8.f;
        camY_ = std::sin(t_ * 94.f) * shake_ * 4.f;
    }
    cacheBend();
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    drawRoad();
    // Earlier sprites sit in front. Banners, then the team, then the trail, then the sky.
    drawHud();
    drawTeam();
    drawWorld();
    drawSkyBits();
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    setupGates();
    buildScenery();
    uint32_t h = 0x51A7u;
    for (Flake& f : flakes_) {
        h = h * 1664525u + 1013904223u;
        f.x = float(h % 320);
        h = h * 1664525u + 1013904223u;
        f.y = float(h % 224);
        f.sp = 16.f + float(h % 18);
        f.sz = float(h % 3);
    }
    sys.apu.setMaster(0.82f);
    sys.apu.setEcho(0.2f, 0.3f, 0.2f);
    sys.apu.setPatch(0, windPatch());
    sys.apu.setPatch(1, bellPatch());
    sys.apu.keyOn(0, 90.f, 0.08f);
    windOn_ = true;
    t_ = 0;
    if (bot_) resetRun();
    else {
        enterTitle();
        loadBest();
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    updateFlakes();
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) updateTitle(pad);
    else if (mode_ == Mode::Pause) updatePause(pad);
    else if (mode_ == Mode::Run) updateRun(pad);
    else updateResult(pad);
    if (shake_ > 0.f && mode_ != Mode::Pause) shake_ = std::max(0.f, shake_ - DT * 1.35f);
    updatePops();
    draw();
    audio();
    lights();
}

}  // namespace sled

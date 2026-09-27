#include "game/harbor.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace harborpurs {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr int kHull = 6;
constexpr float kSpeed = 92.f;
constexpr float kShotV = 230.f;
constexpr float kBoltV = 118.f;
constexpr float kFireCd = 0.16f;
constexpr float kLaneCd = 0.1f;
constexpr float kGrace = 0.35f;
constexpr float kWatch = 55.f;
constexpr float kPlayerHalf = 16.f;
constexpr float kAimNeed = 0.62f;
constexpr float kPocketNear = 40.f;
constexpr float kPocketFar = 128.f;

const char* nameOf(int kind) {
    if (kind == 0) return "TUG STOPPED";
    if (kind == 1) return "FERRY STOPPED";
    return "CUTTER STOPPED";
}

int hpOf(int kind) { return kind == 0 ? 3 : 2; }
float speedOf(int kind) { return kind == 0 ? 26.f : kind == 1 ? 34.f : 46.f; }
float halfOf(int kind) { return kind == 0 ? 20.f : kind == 1 ? 26.f : 14.f; }
int palOf(int kind) { return kind == 0 ? PAL_TUG : kind == 1 ? PAL_FERRY : PAL_CUT; }

bool crossed(float a0, float a1, float cx, float r) {
    float lo = std::min(a0, a1);
    float hi = std::max(a0, a1);
    return hi >= cx - r && lo <= cx + r;
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Victory || mode_ == Mode::Fail) return 3;
    if (bannerT_ > 0.f) return 2;
    return 1;
}

int Game::rivalsRunning() const {
    int n = 0;
    for (const Machine& m : machines_)
        if (m.running) n++;
    return n;
}

int Game::stoppedCount() const { return int(machines_.size()) - rivalsRunning(); }

int Game::quarry() const {
    int best = -1;
    float score = 1e9f;
    for (int i = 0; i < int(machines_.size()); i++) {
        const Machine& m = machines_[size_t(i)];
        if (!m.running) continue;
        float s = std::fabs(m.x - pX_) + std::fabs(float(m.lane - pLane_)) * 70.f + float(m.hp) * 8.f;
        if (s < score) {
            score = s;
            best = i;
        }
    }
    return best;
}

bool Game::shotDanger(int lane) const {
    for (const Shot& s : shots_) {
        if (s.from < 0 || s.lane != lane) continue;
        float dx = s.x - pX_;
        if (s.vx > 0.f && dx < 8.f && dx > -90.f) return true;
        if (s.vx < 0.f && dx > -8.f && dx < 90.f) return true;
    }
    return false;
}

bool Game::bodyClose(int lane) const {
    for (const Machine& m : machines_) {
        if (!m.running || m.lane != lane) continue;
        if (std::fabs(m.x - pX_) < halfOf(m.kind) + kPlayerHalf + 10.f) return true;
    }
    return false;
}

int Game::safeLane() const {
    for (int d = 1; d <= 2; d++) {
        for (int s : {1, -1}) {
            int lane = pLane_ + s * d;
            if (lane < 0 || lane > 2) continue;
            if (!shotDanger(lane) && !bodyClose(lane)) return lane;
        }
    }
    for (int lane = 0; lane < 3; lane++)
        if (!shotDanger(lane)) return lane;
    return pLane_;
}

const gs::Mipped& Game::bodyOf(int kind, int frame) const {
    int f = frame & 1;
    if (kind == 0) return art_.tug[f];
    if (kind == 1) return art_.ferry[f];
    return art_.cutter[f];
}

void Game::resetWorld() {
    hull_ = kHull;
    pLane_ = 1;
    pDir_ = 1;
    pX_ = 140.f;
    t_ = 0;
    watch_ = 0;
    fireCd_ = 0;
    laneCd_ = 0;
    hurtT_ = 0;
    shake_ = 0;
    bannerT_ = 0;
    banner_ = "";
    blip_ = 0;
    grace_ = 0;
    tide_ = 0;
    fanStep_ = -1;
    fanT_ = 0;
    over_ = false;
    won_ = false;
    reason_ = "NOT THE LAST";
    shots_.clear();
    wakes_.clear();
    machines_.clear();

    auto add = [&](int kind, int lane, float x, int dir) {
        Machine m;
        m.kind = kind;
        m.lane = lane;
        m.dir = dir;
        m.hp = hpOf(kind);
        m.x = x;
        m.wakeT = 0.1f;
        m.running = true;
        machines_.push_back(m);
    };
    add(0, 0, 250.f, -1);
    add(1, 2, 220.f, 1);
    add(2, 1, 280.f, -1);
}

void Game::bootTitle() {
    resetWorld();
    mode_ = Mode::Title;
    if (sys_) sys_->setLight(40, 90, 140);
}

void Game::begin() {
    resetWorld();
    mode_ = Mode::Play;
    if (sys_) {
        sys_->setLight(30, 110, 160);
        sys_->apu.tone(0, 330.f, 0.05f);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    if (bot_) begin();
    else bootTitle();
}

void Game::launchPlayer() {
    if (fireCd_ > 0.f || !sys_) return;
    int n = 0;
    for (const Shot& s : shots_)
        if (s.from < 0) n++;
    if (n >= 2) return;
    Shot s;
    s.lane = pLane_;
    s.x = s.prev = pX_ + float(pDir_) * (kPlayerHalf + 4.f);
    s.vx = float(pDir_) * kShotV;
    s.from = -1;
    shots_.push_back(s);
    fireCd_ = kFireCd;
    blip_ = 0.04f;
    sys_->apu.tone(2, 740.f, 0.04f);
}

void Game::launchBolt(int index) {
    Machine& m = machines_[size_t(index)];
    for (const Shot& s : shots_)
        if (s.from == index) return;
    Shot s;
    s.lane = m.lane;
    s.x = s.prev = m.x + float(m.dir) * (halfOf(m.kind) + 3.f);
    s.vx = float(m.dir) * kBoltV;
    s.from = index;
    shots_.push_back(s);
    m.aim = 0;
    m.cool = 1.35f + float(m.kind) * 0.2f;
    if (sys_) sys_->apu.tone(2, 180.f, 0.035f);
}

void Game::hurtPlayer() {
    if (hurtT_ > 0.f || hull_ <= 0 || !sys_) return;
    hull_--;
    hurtT_ = 0.45f;
    shake_ = 1.f;
    sys_->rumble(0.6f, 0.8f, 100);
    sys_->apu.noiseBurst(0.4f, 220.f, 0.1f);
    sys_->apu.tone(0, 80.f, 0.05f);
}

void Game::stopMachine(Machine& m) {
    if (!m.running) return;
    m.running = false;
    m.hp = 0;
    m.flash = 0.2f;
    banner_ = nameOf(m.kind);
    bannerT_ = 1.1f;
    shake_ = std::max(shake_, 0.35f);
    if (!sys_) return;
    sys_->apu.noiseBurst(0.36f, 120.f, 0.14f);
    sys_->apu.tone(0, 110.f, 0.05f);
    for (int i = 0; i < 3; i++) {
        Wake w;
        w.lane = m.lane;
        w.x = m.x + float(i - 1) * 8.f;
        w.y = kLaneY[m.lane] - 12.f;
        w.life = 0.45f;
        w.kind = 1;
        wakes_.push_back(w);
    }
}

void Game::winHarbor() {
    if (mode_ == Mode::Victory) return;
    mode_ = Mode::Victory;
    over_ = true;
    won_ = true;
    reason_ = "LAST MACHINE STILL RUNNING";
    fanGood_ = true;
    fanStep_ = 0;
    fanT_ = 0;
    if (!sys_) return;
    sys_->rumble(0.25f, 0.45f, 160);
    sys_->setLight(40, 180, 90);
}

void Game::loseHarbor(const char* why) {
    if (mode_ == Mode::Fail || mode_ == Mode::Victory) return;
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    reason_ = why;
    hull_ = std::max(0, hull_);
    fanGood_ = false;
    fanStep_ = 0;
    fanT_ = 0;
    if (sys_) sys_->setLight(180, 30, 24);
}

Game::Plan Game::botPlan() const {
    Plan plan;
    plan.lane = pLane_;
    plan.face = pDir_;
    plan.move = 0;
    plan.fire = false;
    int qi = quarry();
    auto faceOf = [&](float x) { return x >= pX_ ? 1 : -1; };
    if (shotDanger(pLane_) || bodyClose(pLane_)) {
        int alt = safeLane();
        if (qi >= 0 && machines_[size_t(qi)].lane == pLane_) {
            const Machine& q = machines_[size_t(qi)];
            float adx = std::fabs(q.x - pX_);
            plan.face = faceOf(q.x);
            if (adx >= 34.f && adx <= kPocketFar + 16.f) plan.fire = true;
        }
        if (alt != pLane_) {
            plan.lane = alt;
            return plan;
        }
        plan.move = (qi >= 0 && machines_[size_t(qi)].x >= pX_) ? -1 : 1;
        if (!plan.fire) plan.face = plan.move;
        return plan;
    }
    if (qi < 0) return plan;
    const Machine& q = machines_[size_t(qi)];
    int face = faceOf(q.x);
    plan.face = face;
    if (q.lane != pLane_) {
        int step = q.lane > pLane_ ? pLane_ + 1 : pLane_ - 1;
        if (!shotDanger(step) && !bodyClose(step)) plan.lane = step;
        float hold = std::clamp(q.x - float(face) * 78.f, kMinX, kMaxX);
        if (std::fabs(pX_ - hold) > 8.f) {
            plan.move = pX_ < hold ? 1 : -1;
            plan.face = plan.move;
        }
        return plan;
    }
    float adx = std::fabs(q.x - pX_);
    if (adx > kPocketFar) {
        plan.move = face;
        plan.face = face;
    } else if (adx < kPocketNear) {
        plan.move = -face;
        plan.face = face;
    }
    if (adx >= kPocketNear - 4.f && adx <= kPocketFar + 24.f) plan.fire = true;
    return plan;
}

void Game::update(float dt) {
    t_ += dt;
    tide_ = std::sin(t_ * 0.7f) * 6.f;
    if (fireCd_ > 0.f) fireCd_ = std::max(0.f, fireCd_ - dt);
    if (laneCd_ > 0.f) laneCd_ = std::max(0.f, laneCd_ - dt);
    if (hurtT_ > 0.f) hurtT_ = std::max(0.f, hurtT_ - dt);
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - dt * 1.8f);
    if (bannerT_ > 0.f) bannerT_ = std::max(0.f, bannerT_ - dt);
    if (blip_ > 0.f) {
        blip_ -= dt;
        if (blip_ <= 0.f && fanStep_ < 0 && sys_) sys_->apu.tone(2, 0.f, 0.f);
    }

    const bool live = mode_ == Mode::Play;
    if (mode_ != Mode::Play && mode_ != Mode::Title) {
        serviceAudio();
        return;
    }

    if (live) {
        watch_ += dt;
        Plan plan;
        if (bot_) {
            plan = botPlan();
        } else if (sys_) {
            const gs::Pad& pad = sys_->pad;
            int move = int(pad.down(gs::BTN_RIGHT)) - int(pad.down(gs::BTN_LEFT));
            if (std::fabs(pad.axisX) > 0.35f) move = pad.axisX > 0.f ? 1 : -1;
            plan.move = move;
            plan.face = move != 0 ? move : pDir_;
            plan.lane = pLane_;
            if (pad.pressed(gs::BTN_UP) && !pad.pressed(gs::BTN_DOWN)) plan.lane = pLane_ - 1;
            if (pad.pressed(gs::BTN_DOWN) && !pad.pressed(gs::BTN_UP)) plan.lane = pLane_ + 1;
            plan.fire = pad.down(gs::BTN_C) || pad.down(gs::BTN_A) || pad.down(gs::BTN_B) || pad.accel > 0.45f;
        }
        plan.lane = std::clamp(plan.lane, 0, 2);
        if (plan.face != 0) pDir_ = plan.face;
        if (plan.fire) launchPlayer();
        if (plan.lane != pLane_ && laneCd_ <= 0.f) {
            pLane_ = plan.lane;
            laneCd_ = kLaneCd;
            Wake w;
            w.lane = pLane_;
            w.x = pX_;
            w.y = kLaneY[pLane_] - 2.f;
            w.life = 0.25f;
            w.kind = 1;
            wakes_.push_back(w);
        }
        if (plan.move != 0) {
            pDir_ = plan.move;
            pX_ += float(plan.move) * kSpeed * dt;
        }
        pX_ = std::clamp(pX_, kMinX, kMaxX);
    }

    for (int i = 0; i < int(machines_.size()); i++) {
        Machine& m = machines_[size_t(i)];
        if (m.flash > 0.f) m.flash = std::max(0.f, m.flash - dt);
        if (!m.running) {
            m.wreck += dt;
            continue;
        }
        if (live) {
            if (m.cool > 0.f) m.cool = std::max(0.f, m.cool - dt);
            float dx = pX_ - m.x;
            bool ahead = dx * float(m.dir) > 0.f;
            if (m.lane == pLane_ && !ahead && std::fabs(dx) > 22.f) {
                m.turn += dt;
                if (m.turn > 0.4f) {
                    m.dir = dx >= 0.f ? 1 : -1;
                    m.turn = 0.f;
                    ahead = true;
                }
            } else {
                m.turn = 0.f;
            }
            m.x += float(m.dir) * speedOf(m.kind) * dt;
            if (m.x <= kMinX) {
                m.x = kMinX;
                m.dir = 1;
            } else if (m.x >= kMaxX) {
                m.x = kMaxX;
                m.dir = -1;
            }
            bool lined = m.lane == pLane_ && ahead && std::fabs(dx) > 48.f && std::fabs(dx) < 170.f;
            if (lined) m.aim += dt;
            else m.aim = std::max(0.f, m.aim - dt * 0.5f);
            if (m.aim >= kAimNeed && m.cool <= 0.f) launchBolt(i);
        }
        m.wakeT -= dt;
        if (m.wakeT <= 0.f) {
            m.wakeT = 0.18f;
            Wake w;
            w.lane = m.lane;
            w.x = m.x - float(m.dir) * 10.f;
            w.y = kLaneY[m.lane] + 2.f;
            w.life = 0.4f;
            wakes_.push_back(w);
        }
    }

    for (int i = 0; i < int(machines_.size()); i++) {
        for (int j = i + 1; j < int(machines_.size()); j++) {
            Machine& a = machines_[size_t(i)];
            Machine& b = machines_[size_t(j)];
            if (!a.running || !b.running || a.lane != b.lane) continue;
            float need = halfOf(a.kind) + halfOf(b.kind);
            float dx = b.x - a.x;
            if (std::fabs(dx) >= need || std::fabs(dx) < 0.01f) continue;
            float push = (need - std::fabs(dx)) * 0.5f;
            float s = dx > 0.f ? 1.f : -1.f;
            a.x -= s * push;
            b.x += s * push;
        }
    }

    if (!live) {
        for (Wake& w : wakes_) w.age += dt;
        wakes_.erase(std::remove_if(wakes_.begin(), wakes_.end(), [](const Wake& w) { return w.age >= w.life; }),
                     wakes_.end());
        serviceAudio();
        return;
    }

    std::vector<Shot> keep;
    keep.reserve(shots_.size());
    for (Shot s : shots_) {
        s.prev = s.x;
        s.x += s.vx * dt;
        bool hit = false;
        if (s.from < 0) {
            int which = -1;
            float best = 1e9f;
            for (int i = 0; i < int(machines_.size()); i++) {
                Machine& m = machines_[size_t(i)];
                if (!m.running || m.lane != s.lane) continue;
                if (!crossed(s.prev, s.x, m.x, halfOf(m.kind))) continue;
                float d = std::fabs(m.x - s.prev);
                if (d < best) {
                    best = d;
                    which = i;
                }
            }
            if (which >= 0) {
                Machine& m = machines_[size_t(which)];
                m.hp--;
                m.flash = 0.12f;
                hit = true;
                if (sys_) sys_->apu.noiseBurst(0.14f, 760.f, 0.04f);
                if (m.hp <= 0) stopMachine(m);
            }
        } else if (s.lane == pLane_ && crossed(s.prev, s.x, pX_, kPlayerHalf)) {
            hurtPlayer();
            hit = true;
        }
        if (!hit && s.x > 70.f && s.x < 330.f) keep.push_back(s);
    }
    shots_.swap(keep);

    for (Machine& m : machines_) {
        if (!m.running || m.lane != pLane_) continue;
        float need = halfOf(m.kind) + kPlayerHalf;
        if (std::fabs(m.x - pX_) >= need) continue;
        if (hurtT_ <= 0.f) {
            m.hp--;
            m.flash = 0.1f;
            hurtPlayer();
            if (m.hp <= 0) stopMachine(m);
        }
        float dir = pX_ < m.x ? -1.f : 1.f;
        pX_ = std::clamp(pX_ + dir * 10.f, kMinX, kMaxX);
        m.x = std::clamp(m.x - dir * 10.f, kMinX, kMaxX);
        m.dir = pX_ < m.x ? 1 : -1;
    }

    for (Wake& w : wakes_) w.age += dt;
    if (wakes_.size() > 24) wakes_.erase(wakes_.begin(), wakes_.begin() + int(wakes_.size()) - 24);
    wakes_.erase(std::remove_if(wakes_.begin(), wakes_.end(), [](const Wake& w) { return w.age >= w.life; }),
                 wakes_.end());

    if (hull_ <= 0) loseHarbor("YOUR MACHINE STOPPED");
    else if (rivalsRunning() == 0) {
        grace_ += dt;
        if (grace_ >= kGrace) winHarbor();
    } else if (watch_ >= kWatch) {
        loseHarbor("NOT THE LAST");
    } else if (sys_) {
        if (hull_ <= 2) sys_->setLight(190, 40, 30);
        else if (shotDanger(pLane_)) sys_->setLight(180, 110, 30);
        else sys_->setLight(30, 110, 160);
    }
    serviceAudio();
}

void Game::serviceAudio() {
    if (!sys_) return;
    if (fanStep_ >= 0) {
        fanT_ += kDt;
        if (fanT_ > 0.14f) {
            static const float good[] = {392.f, 494.f, 587.f, 784.f};
            static const float bad[] = {196.f, 155.f, 123.f};
            const float* notes = fanGood_ ? good : bad;
            int n = fanGood_ ? 4 : 3;
            if (fanStep_ < n) sys_->apu.tone(0, notes[fanStep_], 0.07f);
            else sys_->apu.tone(0, 0.f, 0.f);
            fanStep_++;
            fanT_ = 0;
            if (fanStep_ > n + 2) fanStep_ = -1;
        }
        sys_->apu.tone(1, 0.f, 0.f);
        return;
    }
    bool engine = mode_ == Mode::Play || mode_ == Mode::Title;
    float wob = std::sin(t_ * 28.f) * 2.f;
    sys_->apu.tone(1, engine ? 55.f + wob : 0.f, engine ? 0.022f : 0.f);
    if (blip_ > 0.f) return;
    bool warn = false;
    if (mode_ == Mode::Play) {
        for (const Machine& m : machines_)
            if (m.running && m.lane == pLane_ && m.aim > 0.25f) warn = true;
    }
    if (warn) sys_->apu.tone(2, 140.f + std::sin(t_ * 40.f) * 20.f, 0.028f);
    else if (mode_ != Mode::Play) sys_->apu.tone(2, 0.f, 0.f);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const bool start = sys.pad.pressed(gs::BTN_START);
    if (mode_ == Mode::Title) {
        if (!bot_ && (start || sys.pad.pressed(gs::BTN_C))) begin();
        else update(kDt);
    } else if (mode_ == Mode::Pause) {
        if (!bot_ && start) mode_ = Mode::Play;
    } else if (mode_ == Mode::Play) {
        if (!bot_ && start) mode_ = Mode::Pause;
        else update(kDt);
    } else {
        update(kDt);
        if (!bot_ && start) begin();
    }
    draw();
}

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    v.setFogColor(mode_ == Mode::Fail ? gs::rgb4(6, 2, 2) : gs::rgb4(3, 5, 8));
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.road[y].on = false;
        v.lineFog[y] = 0;
        float u = std::clamp(float(y) / 90.f, 0.f, 1.f);
        int r = int(1 + 6 * (1.f - u));
        int g = int(3 + 6 * (1.f - u * 0.4f));
        int b = int(8 + 6 * (1.f - u));
        if (y > 40) {
            r = 2;
            g = 5;
            b = 8;
        }
        if (mode_ == Mode::Fail) {
            r = std::min(15, r + 3);
            b = std::max(0, b - 3);
        }
        if (mode_ == Mode::Victory && y < 40) g = std::min(15, g + 2);
        v.lineBackdrop[y] = gs::rgb4(std::clamp(r, 0, 15), std::clamp(g, 0, 15), std::clamp(b, 0, 15));
    }
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c < 33 || c > 126) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet) {
    if (!(h > 1.2f) || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 40 || s.x + s.w < -40 || s.y > gs::SCREEN_H + 20 || s.y + s.h < -40) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    float width = 0;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < 33 || c > 126) {
            width += 10.f * scale;
            continue;
        }
        width += float(art_.glyph[c - 32].w) * scale;
    }
    x -= width * 0.5f;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < 33 || c > 126) {
            x += 10.f * scale;
            continue;
        }
        const gs::Mipped& g = art_.glyph[c - 32];
        float gw = float(g.w) * scale;
        spr(g, x + gw * 0.5f, y, float(g.h) * scale, pal, false, false);
        x += gw;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    float shx = shake_ > 0.f ? std::sin(t_ * 80.f) * 3.f * std::min(shake_, 1.f) : 0.f;
    v.B.scroll(int(std::lround(shx + tide_ * 0.15f)), 0);
    sky();

    auto boat = [&](int kind, int frame, float x, int lane, int dir, int pal, bool wreck) {
        float h = (kind == 1 ? 28.f : kind == 0 ? 30.f : 22.f);
        if (kind < 0) h = 24.f;
        float bob = std::sin(t_ * 2.2f + x * 0.05f) * 1.4f;
        float y = kLaneY[lane] + bob;
        if (wreck) y += 4.f;
        spr(kind < 0 ? art_.pilot[frame] : bodyOf(kind, frame), x + shx, y, h, pal, dir < 0, true);
    };

    int frame = int(t_ * 6.f) & 1;
    for (int lane = 2; lane >= 0; lane--) {
        for (const Machine& m : machines_) {
            if (m.lane != lane) continue;
            int pal = m.flash > 0.f ? PAL_ALERT : palOf(m.kind);
            boat(m.kind, frame, m.x, m.lane, m.running ? m.dir : 1, pal, !m.running);
        }
        if (pLane_ == lane && hull_ > 0) boat(-1, frame, pX_, pLane_, pDir_, hurtT_ > 0.f ? PAL_ALERT : PAL_YOU, false);
    }
    for (const Shot& s : shots_) spr(art_.bolt, s.x + shx, kLaneY[s.lane] - 10.f, 6.f, s.from < 0 ? PAL_GOLD : PAL_ALERT, s.vx < 0, false);
    for (const Wake& w : wakes_) {
        float k = 1.f - w.age / w.life;
        spr(w.kind ? art_.spark : art_.wake, w.x + shx, w.y, (w.kind ? 10.f : 8.f) * k, w.kind ? PAL_FX : PAL_WATER, false, false);
    }
    spr(art_.buoy, 96.f + tide_ * 0.3f, kLaneY[0] - 2.f, 18.f, PAL_ALERT, false, true);
    spr(art_.buoy, 312.f, kLaneY[2] - 2.f, 18.f, PAL_GOLD, false, true);
    spr(art_.lamp, 300.f, 52.f, 28.f, PAL_LAMP, false, false);
    float gx = 40.f + std::fmod(t_ * 18.f, 360.f);
    spr(art_.gull, gx, 28.f + std::sin(t_ * 3.f) * 4.f, 8.f, PAL_HUD, false, false);

    hud(1, 0, "HULL", PAL_HUD);
    std::string bars;
    for (int i = 0; i < kHull; i++) bars += i < hull_ ? "#" : "-";
    hud(6, 0, bars, hull_ <= 2 ? PAL_ALERT : PAL_GOOD);
    hud(16, 0, "RIVALS " + std::to_string(rivalsRunning()), PAL_HUD);
    if (bannerT_ > 0.f && banner_ && banner_[0]) hudC(2, banner_, PAL_GOLD);

    if (mode_ == Mode::Title) {
        text("HARBOR PURSUIT", 168.f, 22.f, 0.7f, PAL_GOLD);
        hudC(25, "LAST MACHINE STILL RUNNING", PAL_HUD);
        hudC(26, "START  LANES  FIRE", PAL_GOLD);
    } else if (mode_ == Mode::Pause) {
        hudC(3, "HOLDING", PAL_GOLD);
    } else if (mode_ == Mode::Victory) {
        text("HARBOR YOURS", 168.f, 24.f, 0.72f, PAL_GOOD);
        hudC(26, "LAST MACHINE STILL RUNNING", PAL_GOOD);
    } else if (mode_ == Mode::Fail) {
        text(reason_, 168.f, 24.f, 0.6f, PAL_ALERT);
        hudC(26, "THE HARBOR KEPT GOING", PAL_ALERT);
    }
}

}  // namespace harborpurs

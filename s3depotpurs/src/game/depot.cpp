#include "game/depot.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace depotpurs {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPlayerHalf = 14.f;
constexpr float kSpeed = 96.f;
constexpr float kShotV = 250.f;
constexpr float kBoltV = 92.f;
constexpr float kFireCd = 0.15f;
constexpr float kTrackCd = 0.12f;
constexpr float kWatch = 64.f;
constexpr float kPocketNear = 54.f;
constexpr float kPocketFar = 112.f;
constexpr float kDangerShot = 108.f;
constexpr float kAimNeed = 0.46f;
constexpr float kGrace = 0.55f;
constexpr int kHull = 8;

float halfOf(int kind) {
    if (kind == 0) return 16.f;
    if (kind == 1) return 17.f;
    return 22.f;
}

float tallOf(int kind) {
    if (kind == 0) return 28.f;
    if (kind == 1) return 32.f;
    return 30.f;
}

float speedOf(int kind) {
    if (kind == 0) return 44.f;
    if (kind == 1) return 30.f;
    return 48.f;
}

int hpOf(int kind) { return kind == 2 ? 4 : 3; }

int palOf(int kind) {
    if (kind == 0) return PAL_DRAY;
    if (kind == 1) return PAL_CRANE;
    return PAL_LOCO;
}

const char* nameOf(int kind) {
    if (kind == 0) return "DRAY STOPPED";
    if (kind == 1) return "CRANE STOPPED";
    return "LOCO STOPPED";
}

bool crossed(float prev, float now, float center, float half) {
    float a = std::min(prev, now);
    float b = std::max(prev, now);
    return b >= center - half && a <= center + half;
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Victory || mode_ == Mode::Fail) return 3;
    if (bannerT_ > 0.f && (mode_ == Mode::Play || mode_ == Mode::Pause)) return 2;
    if (mode_ == Mode::Play || mode_ == Mode::Pause) return 1;
    return 0;
}

int Game::stoppedCount() const {
    int n = 0;
    for (const Machine& m : machines_)
        if (!m.running) n++;
    return n;
}

int Game::rivalsRunning() const {
    int n = 0;
    for (const Machine& m : machines_)
        if (m.running) n++;
    return n;
}

int Game::onTrack(int track) const {
    int n = 0;
    for (const Machine& m : machines_)
        if (m.running && m.track == track) n++;
    return n;
}

int Game::quarry() const {
    int best = -1;
    float score = 1e9f;
    for (int i = 0; i < int(machines_.size()); i++) {
        const Machine& m = machines_[size_t(i)];
        if (!m.running) continue;
        float s = std::fabs(m.x - pX_) + std::fabs(float(m.track - pTrack_)) * 96.f + float(m.hp) * 16.f;
        if (s < score) {
            score = s;
            best = i;
        }
    }
    return best;
}

bool Game::shotDanger(int track) const {
    for (const Shot& s : shots_) {
        if (s.from < 0 || s.track != track) continue;
        float dx = pX_ - s.x;
        if (dx * s.vx > 0.f && std::fabs(dx) < kDangerShot) return true;
    }
    return false;
}

bool Game::bodyClose(int track) const {
    for (const Machine& m : machines_) {
        if (!m.running || m.track != track) continue;
        if (std::fabs(m.x - pX_) < halfOf(m.kind) + kPlayerHalf + 4.f) return true;
    }
    return false;
}

int Game::safeTrack() const {
    int empty = -1;
    int any = -1;
    for (int d : {1, -1, 2, -2}) {
        int t = pTrack_ + d;
        if (t < 0 || t > 2) continue;
        if (shotDanger(t) || bodyClose(t)) continue;
        if (onTrack(t) == 0 && empty < 0) empty = t;
        else if (any < 0) any = t;
    }
    if (empty >= 0) return empty;
    if (any >= 0) return any;
    return pTrack_;
}

const gs::Mipped& Game::bodyOf(int kind, int frame) const {
    int f = frame & 1;
    if (kind == 0) return art_.dray[f];
    if (kind == 1) return art_.crane[f];
    return art_.loco[f];
}

void Game::resetWorld() {
    hull_ = kHull;
    pTrack_ = 1;
    pDir_ = 1;
    pX_ = 146.f;
    t_ = 0;
    watch_ = 0;
    fireCd_ = 0;
    trackCd_ = 0;
    hurtT_ = 0;
    shake_ = 0;
    bannerT_ = 0;
    banner_ = "";
    blip_ = 0;
    grace_ = 0;
    puffT_ = 0.08f;
    fanStep_ = -1;
    fanT_ = 0;
    over_ = false;
    won_ = false;
    reason_ = "NOT THE LAST";
    shots_.clear();
    puffs_.clear();
    machines_.clear();

    auto add = [&](int kind, int track, float x, int dir, float phase) {
        Machine m;
        m.kind = kind;
        m.track = track;
        m.dir = dir;
        m.hp = m.maxHp = hpOf(kind);
        m.x = x;
        m.phase = phase;
        m.puffT = phase * 0.1f;
        m.running = true;
        machines_.push_back(m);
    };
    add(0, 0, 228.f, -1, 0.4f);
    add(1, 2, 206.f, 1, 1.3f);
    add(2, 1, 262.f, -1, 2.1f);
}

void Game::bootTitle() {
    resetWorld();
    mode_ = Mode::Title;
    if (sys_) sys_->setLight(140, 86, 32);
}

void Game::begin() {
    resetWorld();
    mode_ = Mode::Play;
    if (sys_) {
        sys_->setLight(150, 92, 36);
        sys_->apu.tone(0, 220.f, 0.05f);
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
    if (n >= 3) return;
    Shot s;
    s.track = pTrack_;
    s.x = s.prev = pX_ + float(pDir_) * (kPlayerHalf + 5.f);
    s.vx = float(pDir_) * kShotV;
    s.from = -1;
    shots_.push_back(s);
    fireCd_ = kFireCd;
    blip_ = 0.05f;
    sys_->apu.tone(2, 860.f, 0.045f);
}

void Game::launchBolt(int index) {
    Machine& m = machines_[size_t(index)];
    for (const Shot& s : shots_)
        if (s.from == index) return;
    Shot s;
    s.track = m.track;
    s.x = s.prev = m.x + float(m.dir) * (halfOf(m.kind) + 4.f);
    s.vx = float(m.dir) * kBoltV;
    s.from = index;
    shots_.push_back(s);
    m.aim = 0;
    m.cool = 1.15f + float(m.kind) * 0.18f;
    if (sys_) sys_->apu.tone(2, 160.f, 0.04f);
}

void Game::hurtPlayer() {
    if (hurtT_ > 0.f || hull_ <= 0 || !sys_) return;
    hull_--;
    hurtT_ = 0.48f;
    shake_ = 1.f;
    sys_->rumble(0.65f, 0.85f, 120);
    sys_->apu.noiseBurst(0.42f, 240.f, 0.12f);
    sys_->apu.tone(0, 90.f, 0.06f);
}

void Game::stopMachine(Machine& m) {
    if (!m.running) return;
    m.running = false;
    m.hp = 0;
    m.aim = 0;
    m.flash = 0.18f;
    banner_ = nameOf(m.kind);
    bannerT_ = 1.15f;
    shake_ = std::max(shake_, 0.4f);
    if (!sys_) return;
    sys_->apu.noiseBurst(0.38f, 140.f, 0.16f);
    sys_->apu.tone(0, 130.f, 0.06f);
    for (int i = 0; i < 4; i++) {
        Puff p;
        p.track = m.track;
        p.x = m.x + float(i - 1) * 6.f;
        p.y = kTrackY[m.track] - 16.f;
        p.vy = -10.f;
        p.life = 0.5f;
        p.kind = 1;
        puffs_.push_back(p);
    }
}

void Game::winYard() {
    if (mode_ == Mode::Victory) return;
    mode_ = Mode::Victory;
    over_ = true;
    won_ = true;
    reason_ = "LAST MACHINE STILL RUNNING";
    fanGood_ = true;
    fanStep_ = 0;
    fanT_ = 0;
    shake_ = 0;
    if (!sys_) return;
    sys_->rumble(0.3f, 0.5f, 180);
    sys_->setLight(40, 170, 70);
}

void Game::loseYard(const char* why) {
    if (mode_ == Mode::Fail || mode_ == Mode::Victory) return;
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    reason_ = why;
    hull_ = std::max(0, hull_);
    fanGood_ = false;
    fanStep_ = 0;
    fanT_ = 0;
    if (sys_) sys_->setLight(180, 30, 20);
}

Game::Plan Game::botPlan() const {
    Plan plan;
    plan.track = pTrack_;
    plan.face = pDir_;
    plan.move = 0;
    plan.fire = false;

    int qi = quarry();
    auto faceOf = [&](float x) { return x >= pX_ ? 1 : -1; };
    bool pointBlank = false;
    if (qi >= 0 && machines_[size_t(qi)].track == pTrack_) {
        const Machine& q = machines_[size_t(qi)];
        pointBlank = std::fabs(q.x - pX_) < halfOf(q.kind) + kPlayerHalf + 6.f;
    }
    if (shotDanger(pTrack_) || pointBlank) {
        int alt = safeTrack();
        if (qi >= 0 && machines_[size_t(qi)].track == pTrack_) {
            const Machine& q = machines_[size_t(qi)];
            float adx = std::fabs(q.x - pX_);
            plan.face = faceOf(q.x);
            if (adx >= 36.f && adx <= kPocketFar + 24.f) plan.fire = true;
        }
        if (alt != pTrack_) {
            plan.track = alt;
            return plan;
        }
        plan.move = (qi >= 0 && machines_[size_t(qi)].x >= pX_) ? -1 : 1;
        plan.face = plan.fire ? plan.face : plan.move;
        return plan;
    }
    if (qi < 0) return plan;
    const Machine& q = machines_[size_t(qi)];
    int face = faceOf(q.x);
    plan.face = face;
    if (q.track != pTrack_) {
        int step = q.track > pTrack_ ? pTrack_ + 1 : pTrack_ - 1;
        float standoff = std::clamp(q.x - float(face) * 86.f, kMinX, kMaxX);
        if (std::fabs(pX_ - standoff) > 10.f) {
            plan.move = pX_ < standoff ? 1 : -1;
            plan.face = plan.move;
        }
        if (!shotDanger(step) && !bodyClose(step)) plan.track = step;
        return plan;
    }
    float adx = std::fabs(q.x - pX_);
    if (adx > kPocketFar) {
        plan.move = face;
        plan.face = face;
    }
    if (adx >= kPocketNear - 6.f && adx <= kPocketFar + 20.f) plan.fire = true;
    return plan;
}

void Game::update(float dt) {
    t_ += dt;
    if (fireCd_ > 0.f) fireCd_ = std::max(0.f, fireCd_ - dt);
    if (trackCd_ > 0.f) trackCd_ = std::max(0.f, trackCd_ - dt);
    if (hurtT_ > 0.f) hurtT_ = std::max(0.f, hurtT_ - dt);
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - dt * 1.7f);
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
            plan.track = pTrack_;
            if (pad.pressed(gs::BTN_UP) && !pad.pressed(gs::BTN_DOWN)) plan.track = pTrack_ - 1;
            if (pad.pressed(gs::BTN_DOWN) && !pad.pressed(gs::BTN_UP)) plan.track = pTrack_ + 1;
            plan.fire = pad.down(gs::BTN_C) || pad.down(gs::BTN_A) || pad.down(gs::BTN_B) || pad.down(gs::BTN_TURBO) ||
                        pad.accel > 0.45f;
        }
        plan.track = std::clamp(plan.track, 0, 2);
        if (plan.face != 0) pDir_ = plan.face;
        if (plan.fire) launchPlayer();
        if (plan.track != pTrack_ && trackCd_ <= 0.f) {
            pTrack_ = plan.track;
            trackCd_ = kTrackCd;
            Puff p;
            p.track = pTrack_;
            p.x = pX_;
            p.y = kTrackY[pTrack_] - 4.f;
            p.life = 0.28f;
            p.kind = 1;
            puffs_.push_back(p);
        }
        if (plan.move != 0) {
            pDir_ = plan.move;
            pX_ += float(plan.move) * kSpeed * dt;
        }
        pX_ = std::clamp(pX_, kMinX, kMaxX);
    }

    int hops = 0;
    for (int i = 0; i < int(machines_.size()); i++) {
        Machine& m = machines_[size_t(i)];
        if (m.flash > 0.f) m.flash = std::max(0.f, m.flash - dt);
        if (!m.running) {
            m.wreck += dt;
            continue;
        }
        if (live) {
            if (m.cool > 0.f) m.cool = std::max(0.f, m.cool - dt);
            if (m.hop > 0.f) m.hop = std::max(0.f, m.hop - dt);
            float dx = pX_ - m.x;
            bool ahead = dx * float(m.dir) > 0.f;
            if (m.track == pTrack_ && !ahead && std::fabs(dx) > 18.f) {
                m.turn += dt;
                if (m.turn > 0.32f) {
                    m.dir = dx >= 0.f ? 1 : -1;
                    m.turn = 0.f;
                    ahead = true;
                }
            } else {
                m.turn = 0.f;
            }
            if (m.hop <= 0.f && m.kind != 1 && m.track != pTrack_ && hops < 1 && onTrack(pTrack_) < 1) {
                m.track += pTrack_ > m.track ? 1 : -1;
                m.hop = 0.75f;
                hops++;
            }
            float spd = speedOf(m.kind);
            if (m.kind == 2 && m.track == pTrack_ && ahead && std::fabs(dx) < 150.f) spd *= 1.28f;
            m.x += float(m.dir) * spd * dt;
            if (m.x <= kMinX) {
                m.x = kMinX;
                m.dir = 1;
            } else if (m.x >= kMaxX) {
                m.x = kMaxX;
                m.dir = -1;
            }
            bool lined = m.track == pTrack_ && ahead && std::fabs(dx) > 40.f && std::fabs(dx) < 168.f;
            if (lined) m.aim += dt;
            else m.aim = std::max(0.f, m.aim - dt * 0.55f);
            if (m.aim >= kAimNeed && m.cool <= 0.f) launchBolt(i);
        }
        m.puffT -= dt;
        if (m.puffT <= 0.f) {
            m.puffT = 0.22f;
            Puff p;
            p.track = m.track;
            p.x = m.x - float(m.dir) * 8.f;
            p.y = kTrackY[m.track] - tallOf(m.kind) * 0.72f;
            p.vy = -14.f;
            p.life = 0.4f;
            puffs_.push_back(p);
        }
    }

    for (int i = 0; i < int(machines_.size()); i++) {
        for (int j = i + 1; j < int(machines_.size()); j++) {
            Machine& a = machines_[size_t(i)];
            Machine& b = machines_[size_t(j)];
            if (!a.running || !b.running || a.track != b.track) continue;
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
        for (Puff& p : puffs_) {
            p.age += dt;
            p.y += p.vy * dt;
        }
        puffs_.erase(std::remove_if(puffs_.begin(), puffs_.end(), [](const Puff& p) { return p.age >= p.life; }),
                     puffs_.end());
        serviceAudio();
        return;
    }

    puffT_ -= dt;
    if (puffT_ <= 0.f && hull_ > 0) {
        puffT_ = 0.2f;
        Puff p;
        p.track = pTrack_;
        p.x = pX_ - float(pDir_) * 6.f;
        p.y = kTrackY[pTrack_] - 20.f;
        p.vy = -12.f;
        p.life = 0.35f;
        puffs_.push_back(p);
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
                if (!m.running || m.track != s.track) continue;
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
                Puff p;
                p.track = m.track;
                p.x = m.x;
                p.y = kTrackY[m.track] - 14.f;
                p.life = 0.22f;
                p.kind = 1;
                puffs_.push_back(p);
                if (sys_) sys_->apu.noiseBurst(0.16f, 880.f, 0.04f);
                if (m.hp <= 0) stopMachine(m);
            }
        } else if (s.track == pTrack_ && crossed(s.prev, s.x, pX_, kPlayerHalf)) {
            hurtPlayer();
            hit = true;
        }
        if (!hit && s.x > 92.f && s.x < 316.f) keep.push_back(s);
    }
    shots_.swap(keep);

    for (Machine& m : machines_) {
        if (!m.running || m.track != pTrack_) continue;
        float need = halfOf(m.kind) + kPlayerHalf;
        if (std::fabs(m.x - pX_) >= need) continue;
        if (hurtT_ <= 0.f) {
            m.hp--;
            m.flash = 0.1f;
            hurtPlayer();
            if (m.hp <= 0) stopMachine(m);
        }
        float dir = pX_ < m.x ? -1.f : 1.f;
        pX_ += dir * 8.f;
        m.x -= dir * 8.f;
        m.dir = pX_ < m.x ? 1 : -1;
        pX_ = std::clamp(pX_, kMinX, kMaxX);
        m.x = std::clamp(m.x, kMinX, kMaxX);
    }

    for (Puff& p : puffs_) {
        p.age += dt;
        p.y += p.vy * dt;
    }
    if (puffs_.size() > 28) puffs_.erase(puffs_.begin(), puffs_.begin() + int(puffs_.size()) - 28);
    puffs_.erase(std::remove_if(puffs_.begin(), puffs_.end(), [](const Puff& p) { return p.age >= p.life; }),
                 puffs_.end());

    if (hull_ <= 0) loseYard("YOUR MACHINE STOPPED");
    else if (rivalsRunning() == 0) {
        grace_ += dt;
        if (grace_ >= kGrace) winYard();
    } else if (watch_ >= kWatch) {
        loseYard("NOT THE LAST");
    } else if (sys_) {
        if (hull_ <= 2) sys_->setLight(200, 40, 28);
        else if (shotDanger(pTrack_)) sys_->setLight(200, 120, 30);
        else sys_->setLight(150, 92, 36);
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
    float wob = std::sin(t_ * 34.f) * 3.f;
    sys_->apu.tone(1, engine ? 48.f + wob : 0.f, engine ? 0.028f : 0.f);
    if (blip_ > 0.f) return;
    bool warn = false;
    if (mode_ == Mode::Play) {
        for (const Machine& m : machines_)
            if (m.running && m.track == pTrack_ && m.aim > 0.22f) warn = true;
    }
    if (warn) sys_->apu.tone(2, 150.f + std::sin(t_ * 48.f) * 30.f, 0.03f);
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
    v.setFogColor(mode_ == Mode::Fail ? gs::rgb4(6, 2, 2) : gs::rgb4(3, 2, 4));
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.road[y].on = false;
        v.lineFog[y] = 0;
        float u = std::clamp(float(y) / 72.f, 0.f, 1.f);
        int r = int(2 + (12 - 2) * u * u);
        int g = int(2 + (5 - 2) * u);
        int b = int(7 - 4 * u);
        if (y > 48) {
            r = 3;
            g = 3;
            b = 3;
        }
        if (mode_ == Mode::Fail) {
            r = std::min(15, r + 2);
            b = std::max(0, b - 2);
        }
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

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet, bool shadow) {
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
    s.shadow = shadow;
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
        spr(g, x + gw * 0.5f, y, float(g.h) * scale, pal, false, false, false);
        x += gw;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    float shx = 0.f;
    if (shake_ > 0.f) shx = std::sin(t_ * 86.f) * 4.f * std::min(shake_, 1.f);
    v.B.scroll(int(std::lround(shx)), 0);
    sky();

    if (mode_ == Mode::Title) {
        text("DEPOT PURSUIT", 196.f + shx, 18.f, 0.92f, PAL_GOLD);
        text("BE THE LAST", 196.f + shx, 40.f, 0.7f, PAL_GOOD);
    } else if (mode_ == Mode::Victory) {
        text("LAST MACHINE", 196.f + shx, 16.f, 0.86f, PAL_GOOD);
        text("STILL RUNNING", 196.f + shx, 38.f, 0.86f, PAL_GOOD);
    } else if (mode_ == Mode::Fail) {
        text("WATCH OVER", 196.f + shx, 22.f, 0.95f, PAL_ALERT);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 196.f + shx, 22.f, 1.f, PAL_GOLD);
    } else if (bannerT_ > 0.f && banner_ && banner_[0]) {
        text(banner_, 196.f + shx, 22.f, 0.72f, PAL_GOLD);
    }

    auto machine = [&](int kind, float x, float feet, int dir, int frame, int pal, bool running, int hp, bool flash,
                        bool you, float aim) {
        const gs::Mipped& body = you ? art_.shunter[frame & 1] : bodyOf(kind, frame);
        float h = you ? 28.f : tallOf(kind);
        bool hide = you && hurtT_ > 0.f && (int(t_ * 24.f) & 1);
        spr(art_.shadow, x, feet + 1.f, h * 0.22f, PAL_FX, false, false, true);
        if (!hide) spr(body, x, feet, h, pal, dir < 0, true, false);
        if (running && !hide) {
            int lampPal = (!you && aim > 0.22f && (int(t_ * 18.f) & 1)) ? PAL_ALERT : PAL_FX;
            spr(art_.pip, x + float(dir) * h * 0.46f, feet - h * 0.55f, 5.f, lampPal, false, false, false);
        }
        if (!running) {
            int flick = int(t_ * 12.f + float(kind)) & 1;
            spr(art_.flame[flick], x, feet - h * 0.62f, h * 0.48f, PAL_FX, false, false, false);
        }
        if (flash) spr(art_.spark, x, feet - h * 0.45f, 12.f, PAL_FX, false, false, false);
        if (!you && running) {
            float span = float(hp - 1) * 6.f;
            for (int i = 0; i < hp; i++)
                spr(art_.pip, x - span * 0.5f + float(i) * 6.f, feet - h - 4.f, 5.f, PAL_GOLD, false, false, false);
        }
    };

    // Earlier sprites sit on top. Draw the near stall first.
    for (int tr = 2; tr >= 0; --tr) {
        for (Puff& p : puffs_) {
            if (p.track != tr) continue;
            float k = 1.f - p.age / std::max(0.05f, p.life);
            spr(p.kind ? art_.spark : art_.puff, p.x + shx, p.y, 6.f + k * 8.f, PAL_FX, false, false, false);
        }
        for (const Shot& s : shots_) {
            if (s.track != tr) continue;
            const gs::Mipped& img = s.from < 0 ? art_.shoe : art_.bolt;
            spr(img, s.x + shx, kTrackY[tr] - 12.f, s.from < 0 ? 8.f : 7.f, s.from < 0 ? PAL_GOLD : PAL_FX, s.vx < 0.f,
                false, false);
        }
        if (pTrack_ == tr && mode_ != Mode::Title) {
            int fr = int(std::floor(std::fabs(pX_) * 0.12f)) & 1;
            bool dead = mode_ == Mode::Fail || hull_ <= 0;
            machine(-1, pX_ + shx, kTrackY[tr], pDir_, fr, PAL_YOU, !dead, hull_, hurtT_ > 0.2f, true, 0.f);
        }
        for (const Machine& m : machines_) {
            if (m.track != tr) continue;
            int fr = m.running ? (int(std::floor((m.x + t_ * 8.f) * 0.15f)) & 1) : 0;
            machine(m.kind, m.x + shx, kTrackY[tr], m.dir, fr, palOf(m.kind), m.running, m.hp, m.flash > 0.f, false,
                    m.aim);
        }
    }
    if (mode_ == Mode::Title) {
        int fr = int(t_ * 6.f) & 1;
        machine(-1, pX_ + shx, kTrackY[pTrack_], pDir_, fr, PAL_YOU, true, hull_, false, true, 0.f);
    }

    for (int tr = 0; tr < 3; tr++) {
        spr(art_.buffer, 82.f + shx, kTrackY[tr], 20.f, PAL_PROP, false, true, false);
        spr(art_.buffer, 312.f + shx, kTrackY[tr], 20.f, PAL_PROP, true, true, false);
    }
    spr(art_.post, 96.f + shx, 86.f, 28.f, PAL_PROP, false, true, false);
    spr(art_.post, 300.f + shx, 86.f, 28.f, PAL_PROP, true, true, false);
    spr(art_.barrel, 168.f + shx, 78.f, 16.f, PAL_PROP, false, true, false);
    spr(art_.crate, 236.f + shx, 78.f, 14.f, PAL_PROP, false, true, false);
    spr(art_.sign, 200.f + shx, 66.f, float(art_.sign.h), PAL_HOUSE, false, false, false);
    float cloudX = std::fmod(40.f + t_ * 6.f, 380.f) - 40.f;
    spr(art_.cloud, cloudX + shx, 14.f, 12.f, PAL_NIGHT, false, false, false);
    spr(art_.moon, 292.f + shx, 16.f, 16.f, PAL_NIGHT, false, false, false);
    if ((int(t_ * 3.f) & 1) == 0) spr(art_.pip, 20.f, 18.f, 6.f, PAL_GOLD, false, false, false);

    if (mode_ == Mode::Title) {
        hud(1, 0, "S3 DEPOT PURSUIT", PAL_GOLD);
        const char* go = ((int(t_ * 2.f) & 1) == 0) ? "ENTER" : "START";
        hud(33, 0, go, PAL_HUD);
        hud(1, 26, "ARROWS MOVE", PAL_HUD);
        hud(15, 26, "C BRAKE", PAL_GOLD);
        hud(24, 26, "UP DOWN DODGE", PAL_HUD);
        hud(40 - int(std::strlen(S3_VERSION_STRING)), 27, S3_VERSION_STRING, PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        hudC(0, "PAUSED", PAL_GOLD);
        hudC(26, "ENTER TO ROLL", PAL_HUD);
    } else if (mode_ == Mode::Victory) {
        hud(1, 0, "DEPOT", PAL_GOOD);
        hudC(0, "LAST MACHINE", PAL_GOOD);
        char end[24];
        std::snprintf(end, sizeof end, "HULL %d", hull_);
        hud(32, 0, end, PAL_HUD);
        hudC(26, "THE YARD IS DONE", PAL_GOOD);
        hudC(27, "ENTER FOR ANOTHER SHIFT", PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        hudC(0, reason_, PAL_ALERT);
        hudC(26, "YOUR ENGINE DIED OR THEIRS DID NOT", PAL_HUD);
        hudC(27, "ENTER TO TRY THE YARD AGAIN", PAL_HUD);
    } else {
        char left[20];
        std::snprintf(left, sizeof left, "HULL %d", hull_);
        hud(1, 0, left, hull_ <= 2 ? PAL_ALERT : PAL_HUD);
        char mid[20];
        std::snprintf(mid, sizeof mid, "OTHERS %d", rivalsRunning());
        hudC(0, mid, rivalsRunning() == 1 ? PAL_GOLD : PAL_HUD);
        int sec = std::max(0, int(std::ceil(kWatch - watch_)));
        char clock[16];
        std::snprintf(clock, sizeof clock, "%d:%02d", sec / 60, sec % 60);
        int cpal = (sec <= 10 && (int(t_ * 8.f) & 1)) ? PAL_ALERT : PAL_HUD;
        hud(40 - int(std::strlen(clock)), 0, clock, cpal);
        hud(1, 27, "STOP THE OTHER MACHINES", PAL_HUD);
        hud(26, 27, "STAY RUNNING", PAL_GOLD);
    }
}

}  // namespace depotpurs

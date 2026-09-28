#include "game/quarry.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace quarrypurs {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPlayerV = 92.f;
constexpr float kShotV = 150.f;
constexpr float kBoltV = 78.f;
constexpr float kFireCd = 0.20f;
constexpr float kBenchCd = 0.16f;
constexpr float kPlayerHalf = 16.f;
constexpr float kPocketNear = 58.f;
constexpr float kPocketFar = 118.f;

}  // namespace

float Game::halfOf(int kind) const {
    if (kind == 1) return 22.f;
    if (kind == 2) return 20.f;
    if (kind == 3) return 18.f;
    return 16.f;
}

const char* Game::nameOf(int kind) const {
    if (kind == 1) return "DUMPER STOPPED";
    if (kind == 2) return "HOE STOPPED";
    if (kind == 3) return "CRUSHER STOPPED";
    return "MACHINE STOPPED";
}

const gs::Mipped& Game::bodyOf(int kind, int frame) const {
    int f = frame & 1;
    if (kind == 1) return art_.dumper[f];
    if (kind == 2) return art_.hoe[f];
    if (kind == 3) return art_.crush[f];
    return art_.loader[f];
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

int Game::marker() const {
    if (mode_ == Mode::Victory || mode_ == Mode::Fail) return 3;
    if (mode_ == Mode::Play && bannerT_ > 0.15f && stopFlash_ > 0) return 2;
    if (mode_ == Mode::Play && watch_ > 0.35f) return 1;
    return 0;
}

void Game::resetWorld() {
    machines_.clear();
    shots_.clear();
    puffs_.clear();
    hull_ = 8;
    pBench_ = 0;
    pDir_ = 1;
    pX_ = 64.f;
    t_ = 0;
    watch_ = 0;
    fireCd_ = 0;
    benchCd_ = 0.35f;
    hurtT_ = 0.4f;
    shake_ = 0;
    bannerT_ = 0;
    blip_ = 0;
    banner_ = "";
    stopFlash_ = 0;
    over_ = false;
    won_ = false;
    reason_ = "NOT THE LAST";
    fanStep_ = -1;
    fanT_ = 0;
    auto add = [&](int kind, int bench, float x, int dir, int hp, float cool) {
        Machine m;
        m.kind = kind;
        m.bench = bench;
        m.x = x;
        m.dir = dir;
        m.hp = hp;
        m.maxHp = hp;
        m.cool = cool;
        machines_.push_back(m);
    };
    add(1, 0, 250.f, -1, 2, 1.4f);
    add(2, 1, 86.f, 1, 2, 1.1f);
    add(3, 2, 236.f, -1, 3, 1.6f);
    add(1, 1, 250.f, -1, 2, 1.8f);
}

void Game::bootTitle() {
    resetWorld();
    mode_ = Mode::Title;
    if (sys_) sys_->setLight(160, 110, 40);
}

void Game::begin() {
    resetWorld();
    mode_ = Mode::Play;
    if (sys_) {
        sys_->setLight(170, 120, 48);
        sys_->apu.tone(0, 196.f, 0.05f);
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
    s.bench = pBench_;
    s.x = s.prev = pX_ + float(pDir_) * (kPlayerHalf + 6.f);
    s.vx = float(pDir_) * kShotV;
    s.from = -1;
    shots_.push_back(s);
    fireCd_ = kFireCd;
    blip_ = 0.05f;
    sys_->apu.tone(2, 740.f, 0.04f);
}

void Game::launchBolt(int index) {
    Machine& m = machines_[size_t(index)];
    for (const Shot& s : shots_)
        if (s.from == index) return;
    Shot s;
    s.bench = m.bench;
    s.x = s.prev = m.x + float(m.dir) * (halfOf(m.kind) + 4.f);
    s.vx = float(m.dir) * kBoltV;
    s.from = index;
    shots_.push_back(s);
    m.cool = 1.35f + float(m.kind) * 0.12f;
    if (sys_) sys_->apu.tone(1, 140.f, 0.035f);
}

void Game::hurtPlayer() {
    if (hurtT_ > 0.f || hull_ <= 0 || !sys_) return;
    hull_--;
    hurtT_ = 0.55f;
    shake_ = 1.f;
    sys_->rumble(0.6f, 0.8f, 100);
    sys_->apu.noiseBurst(0.4f, 220.f, 0.1f);
    if (hull_ <= 0) loseQuarry("THE LOADER STOPPED");
}

void Game::stopMachine(Machine& m) {
    if (!m.running) return;
    m.running = false;
    m.hp = 0;
    m.flash = 0.2f;
    banner_ = nameOf(m.kind);
    bannerT_ = 1.2f;
    stopFlash_ = 18;
    shake_ = std::max(shake_, 0.45f);
    if (!sys_) return;
    sys_->apu.noiseBurst(0.36f, 120.f, 0.16f);
    sys_->apu.tone(0, 110.f, 0.06f);
    for (int i = 0; i < 4; i++) {
        Puff p;
        p.bench = m.bench;
        p.x = m.x + float(i - 1) * 7.f;
        p.y = kBenchY[m.bench] - 18.f;
        p.vy = -14.f;
        p.life = 0.55f;
        p.kind = 1;
        puffs_.push_back(p);
    }
}

void Game::winQuarry() {
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
    sys_->setLight(40, 160, 60);
}

void Game::loseQuarry(const char* why) {
    if (mode_ == Mode::Fail || mode_ == Mode::Victory) return;
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    reason_ = why;
    fanGood_ = false;
    fanStep_ = 0;
    fanT_ = 0;
    if (sys_) sys_->setLight(180, 40, 20);
}

bool Game::shotDanger(int bench) const {
    for (const Shot& s : shots_) {
        if (s.from < 0 || s.bench != bench) continue;
        float dx = pX_ - s.x;
        if (s.vx * dx > 0.f && std::fabs(dx) < 86.f) return true;
    }
    return false;
}

bool Game::bodyClose(int bench) const {
    for (const Machine& m : machines_) {
        if (!m.running || m.bench != bench) continue;
        if (std::fabs(m.x - pX_) < halfOf(m.kind) + kPlayerHalf + 8.f) return true;
    }
    return false;
}

int Game::safeBench() const {
    int best = -1;
    int bestScore = -1000;
    for (int b = 0; b < 3; b++) {
        if (shotDanger(b) || bodyClose(b)) continue;
        int score = 10 - std::abs(b - pBench_) * 3;
        for (const Machine& m : machines_)
            if (m.running && m.bench == b) score += 6;
        if (score > bestScore) {
            bestScore = score;
            best = b;
        }
    }
    return best;
}

int Game::quarry() const {
    int best = -1;
    float bestD = 1e9f;
    for (int i = 0; i < int(machines_.size()); i++) {
        const Machine& m = machines_[size_t(i)];
        if (!m.running) continue;
        float d = std::fabs(m.x - pX_) + std::abs(m.bench - pBench_) * 80.f;
        if (d < bestD) {
            bestD = d;
            best = i;
        }
    }
    return best;
}

Game::Plan Game::botPlan() const {
    Plan plan;
    plan.bench = pBench_;
    plan.face = pDir_;
    plan.move = 0;
    plan.fire = false;
    int qi = quarry();
    auto faceOf = [&](float x) { return x >= pX_ ? 1 : -1; };
    if (shotDanger(pBench_) || bodyClose(pBench_)) {
        int alt = safeBench();
        if (alt >= 0 && alt != pBench_) {
            plan.bench = alt;
            return plan;
        }
        plan.move = (qi >= 0 && machines_[size_t(qi)].x >= pX_) ? -1 : 1;
        plan.face = plan.move;
        if (plan.move < 0 && pX_ < kMinX + 8.f) plan.move = 1;
        if (plan.move > 0 && pX_ > kMaxX - 8.f) plan.move = -1;
        return plan;
    }
    if (qi < 0) return plan;
    const Machine& q = machines_[size_t(qi)];
    int face = faceOf(q.x);
    plan.face = face;
    if (q.bench != pBench_) {
        int step = q.bench > pBench_ ? pBench_ + 1 : pBench_ - 1;
        if (!shotDanger(step) && !bodyClose(step)) plan.bench = step;
        float hold = std::clamp(q.x - float(face) * 90.f, kMinX, kMaxX);
        if (std::fabs(pX_ - hold) > 8.f) {
            plan.move = pX_ < hold ? 1 : -1;
            plan.face = plan.move;
        }
        return plan;
    }
    float adx = std::fabs(q.x - pX_);
    if (adx > kPocketFar) plan.move = face;
    else if (adx < kPocketNear) plan.move = -face;
    plan.face = face;
    if (adx >= kPocketNear - 4.f && adx <= kPocketFar + 16.f) plan.fire = true;
    return plan;
}

void Game::update(float dt) {
    t_ += dt;
    if (fireCd_ > 0.f) fireCd_ = std::max(0.f, fireCd_ - dt);
    if (benchCd_ > 0.f) benchCd_ = std::max(0.f, benchCd_ - dt);
    if (hurtT_ > 0.f) hurtT_ = std::max(0.f, hurtT_ - dt);
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - dt * 1.6f);
    if (bannerT_ > 0.f) bannerT_ = std::max(0.f, bannerT_ - dt);
    if (stopFlash_ > 0) stopFlash_--;
    if (blip_ > 0.f) {
        blip_ -= dt;
        if (blip_ <= 0.f && fanStep_ < 0 && sys_) sys_->apu.tone(2, 0.f, 0.f);
    }
    if (mode_ != Mode::Play && mode_ != Mode::Title) {
        serviceAudio();
        return;
    }
    const bool live = mode_ == Mode::Play;
    if (live) {
        watch_ += dt;
        Plan plan;
        if (bot_) plan = botPlan();
        else if (sys_) {
            const gs::Pad& pad = sys_->pad;
            int move = int(pad.down(gs::BTN_RIGHT)) - int(pad.down(gs::BTN_LEFT));
            if (std::fabs(pad.axisX) > 0.35f) move = pad.axisX > 0.f ? 1 : -1;
            plan.move = move;
            plan.face = move != 0 ? move : pDir_;
            plan.bench = pBench_;
            if (pad.pressed(gs::BTN_UP)) plan.bench = pBench_ - 1;
            if (pad.pressed(gs::BTN_DOWN)) plan.bench = pBench_ + 1;
            plan.fire = pad.down(gs::BTN_C) || pad.down(gs::BTN_A) || pad.down(gs::BTN_B) || pad.accel > 0.45f;
        }
        plan.bench = std::clamp(plan.bench, 0, 2);
        if (plan.face != 0) pDir_ = plan.face;
        if (plan.fire) launchPlayer();
        if (plan.bench != pBench_ && benchCd_ <= 0.f) {
            pBench_ = plan.bench;
            benchCd_ = kBenchCd;
            hurtT_ = std::max(hurtT_, 0.12f);
        }
        if (plan.move != 0) pX_ = std::clamp(pX_ + float(plan.move) * kPlayerV * dt, kMinX, kMaxX);

        for (int i = 0; i < int(machines_.size()); i++) {
            Machine& m = machines_[size_t(i)];
            if (!m.running) continue;
            m.phase += dt;
            m.cool = std::max(0.f, m.cool - dt);
            if (m.flash > 0.f) m.flash = std::max(0.f, m.flash - dt);
            float nx = m.x + float(m.dir) * (26.f + float(m.kind) * 2.f) * dt;
            if (nx < kMinX + 10.f || nx > kMaxX - 10.f) m.dir = -m.dir;
            bool blocked = false;
            for (int j = 0; j < int(machines_.size()); j++) {
                if (j == i || !machines_[size_t(j)].running) continue;
                const Machine& o = machines_[size_t(j)];
                if (o.bench != m.bench) continue;
                if (std::fabs(o.x - nx) < halfOf(m.kind) + halfOf(o.kind) + 6.f) blocked = true;
            }
            if (blocked) m.dir = -m.dir;
            else m.x = std::clamp(nx, kMinX, kMaxX);
            if (m.bench == pBench_ && std::fabs(m.x - pX_) < halfOf(m.kind) + kPlayerHalf) {
                m.dir = m.x >= pX_ ? 1 : -1;
                hurtPlayer();
            }
            bool sees = m.bench == pBench_ && std::fabs(m.x - pX_) < 130.f && std::fabs(m.x - pX_) > 36.f;
            if (sees && m.cool <= 0.f && watch_ > 0.7f) {
                m.dir = pX_ >= m.x ? 1 : -1;
                launchBolt(i);
            }
        }

        std::vector<Shot> keep;
        keep.reserve(shots_.size());
        for (Shot& s : shots_) {
            s.prev = s.x;
            s.x += s.vx * dt;
            if (s.x < 8.f || s.x > 312.f) continue;
            bool hit = false;
            if (s.from < 0) {
                for (Machine& m : machines_) {
                    if (!m.running || m.bench != s.bench) continue;
                    float a = std::min(s.prev, s.x), b = std::max(s.prev, s.x);
                    float left = m.x - halfOf(m.kind), right = m.x + halfOf(m.kind);
                    if (b < left || a > right) continue;
                    m.hp--;
                    m.flash = 0.12f;
                    hit = true;
                    if (m.hp <= 0) stopMachine(m);
                    else if (sys_) sys_->apu.tone(0, 320.f, 0.03f);
                    break;
                }
            } else if (s.bench == pBench_) {
                float a = std::min(s.prev, s.x), b = std::max(s.prev, s.x);
                if (!(b < pX_ - kPlayerHalf || a > pX_ + kPlayerHalf)) {
                    hurtPlayer();
                    hit = true;
                }
            }
            if (!hit) keep.push_back(s);
        }
        shots_.swap(keep);

        for (Puff& p : puffs_) {
            p.age += dt;
            p.y += p.vy * dt;
            p.vy += 18.f * dt;
        }
        puffs_.erase(std::remove_if(puffs_.begin(), puffs_.end(), [](const Puff& p) { return p.age >= p.life; }),
                     puffs_.end());

        if (rivalsRunning() == 0 && hull_ > 0) winQuarry();
    } else {
        for (Machine& m : machines_)
            if (m.running) m.phase += dt * 0.5f;
    }
    serviceAudio();
}

void Game::serviceAudio() {
    if (!sys_) return;
    if (fanStep_ >= 0) {
        fanT_ += kDt;
        static const float good[] = {523.f, 659.f, 784.f, 1046.f};
        static const float bad[] = {220.f, 196.f, 174.f, 130.f};
        if (fanT_ > 0.14f) {
            fanT_ = 0;
            const float* seq = fanGood_ ? good : bad;
            if (fanStep_ < 4) sys_->apu.tone(0, seq[fanStep_], 0.07f);
            fanStep_++;
            if (fanStep_ > 6) {
                fanStep_ = -1;
                sys_->apu.tone(0, 0.f, 0.f);
            }
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const bool start = sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_C);
    if (mode_ == Mode::Title) {
        if (!bot_ && start) begin();
        else update(kDt);
    } else if (mode_ == Mode::Pause) {
        if (!bot_ && sys.pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
    } else if (mode_ == Mode::Play) {
        if (!bot_ && sys.pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else update(kDt);
    } else {
        update(kDt);
        if (!bot_ && sys.pad.pressed(gs::BTN_START)) begin();
    }
    if (!bot_ && mode_ == Mode::Title && sys.hasHome() && sys.pad.pressed(gs::BTN_MODE)) sys.eject();
    draw();
}

void Game::face() {
    gs::VDP& v = sys_->vdp;
    v.setFogColor(mode_ == Mode::Fail ? gs::rgb4(6, 2, 1) : gs::rgb4(8, 6, 4));
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.road[y].on = false;
        v.lineFog[y] = 0;
        int r = 6, g = 8, b = 12;
        if (y > 28) {
            float u = std::clamp((y - 28) / 40.f, 0.f, 1.f);
            r = int(6 + 4 * u);
            g = int(8 - 3 * u);
            b = int(12 - 7 * u);
        }
        if (y > 48) {
            r = 7;
            g = 5;
            b = 3;
        }
        if (mode_ == Mode::Fail) {
            r = std::min(15, r + 3);
            b = std::max(0, b - 3);
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
    float pen = x;
    for (unsigned char c : s) {
        if (c < 33 || c > 126) {
            pen += 10.f * scale;
            continue;
        }
        const gs::Mipped& g = art_.glyph[c - 32];
        float h = 8.f * scale;
        spr(g, pen + h * 0.4f, y, h, pal, false, false, false);
        pen += (float(g.w) / float(std::max(1, g.h))) * h + scale;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    face();
    float sh = (shake_ > 0.f) ? std::sin(t_ * 70.f) * shake_ * 2.f : 0.f;

    auto prop = [&](const gs::Mipped& m, float x, int bench, float h, int pal) {
        spr(m, x + sh, kBenchY[bench], h, pal, false, true, false);
    };
    prop(art_.boulder, 28, 0, 18, PAL_PROP);
    prop(art_.cone, 300, 0, 16, PAL_PROP);
    prop(art_.lamp, 18, 1, 22, PAL_PROP);
    prop(art_.boulder, 304, 1, 16, PAL_PROP);
    prop(art_.cone, 24, 2, 16, PAL_PROP);
    prop(art_.lamp, 306, 2, 22, PAL_PROP);

    for (const Puff& p : puffs_) {
        float k = 1.f - p.age / p.life;
        spr(p.kind ? art_.spark : art_.puff, p.x + sh, p.y, 8.f + 10.f * (1.f - k), PAL_FX, false, false, false);
    }

    auto drawBody = [&](int kind, int frame, float x, int bench, int pal, bool flip, bool dead) {
        float h = dead ? 22.f : 30.f + float(kind == 1);
        spr(art_.shadow, x + sh, kBenchY[bench] + 1.f, 8.f, PAL_FX, false, true, true);
        spr(bodyOf(kind, frame), x + sh, kBenchY[bench], h, pal, flip, true, false);
    };

    for (const Machine& m : machines_) {
        int pal = m.kind == 1 ? PAL_DUMP : m.kind == 2 ? PAL_HOE : PAL_CRUSH;
        if (!m.running) pal = PAL_DUST;
        else if (m.flash > 0.f) pal = PAL_ALERT;
        int frame = int(m.phase * 6.f);
        drawBody(m.kind, frame, m.x, m.bench, pal, m.dir < 0, !m.running);
    }

    int pPal = (hurtT_ > 0.f && int(t_ * 20.f) & 1) ? PAL_ALERT : PAL_YOU;
    drawBody(0, int(t_ * 8.f), pX_, pBench_, pPal, pDir_ < 0, mode_ == Mode::Fail);

    for (const Shot& s : shots_) {
        float y = kBenchY[s.bench] - 14.f;
        spr(s.from < 0 ? art_.rock : art_.spark, s.x + sh, y, s.from < 0 ? 8.f : 7.f, PAL_FX, s.vx < 0, false, false);
    }

    hud(1, 1, "QUARRY", PAL_GOLD);
    hud(28, 1, "HULL " + std::to_string(hull_), hull_ > 2 ? PAL_HUD : PAL_ALERT);
    hud(1, 26, "STILL " + std::to_string(rivalsRunning()), PAL_HUD);
    hud(24, 26, "STOPPED " + std::to_string(stoppedCount()), PAL_GOOD);
    if (bannerT_ > 0.f && banner_[0]) hudC(3, banner_, PAL_GOLD);

    if (mode_ == Mode::Title) {
        text("QUARRY", 78, 40, 3.f, PAL_GOLD);
        hudC(12, "LAST MACHINE STILL RUNNING", PAL_HUD);
        hudC(14, "ARROWS MOVE  C FIRES", PAL_HUD);
        hudC(16, "PRESS START", PAL_GOOD);
    } else if (mode_ == Mode::Pause) {
        hudC(12, "HOLD", PAL_GOLD);
    } else if (mode_ == Mode::Victory) {
        hudC(12, "THE FACE IS YOURS", PAL_GOOD);
        hudC(14, "LAST MACHINE STILL RUNNING", PAL_GOLD);
    } else if (mode_ == Mode::Fail) {
        hudC(12, reason_, PAL_ALERT);
        hudC(14, "THE QUARRY OUTLASTED YOU", PAL_HUD);
    }
}

}  // namespace quarrypurs

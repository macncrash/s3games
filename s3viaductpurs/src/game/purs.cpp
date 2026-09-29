#include "game/purs.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace viaductpurs {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr int kHull = 8;
constexpr float kSpeed = 108.f;
constexpr float kShotV = 250.f;
constexpr float kBoltV = 96.f;
constexpr float kFireCd = 0.16f;
constexpr float kLaneCd = 0.12f;
constexpr float kPlayerHalf = 16.f;
constexpr float kLeft = 36.f;
constexpr float kRight = 292.f;

const float kLaneY[3] = {112.f, 146.f, 180.f};

const char* nameOf(int kind) {
    if (kind == 0) return "RAIL STOPPED";
    if (kind == 1) return "CRANE STOPPED";
    return "BUS STOPPED";
}

int hpOf(int kind) { return kind == 1 ? 3 : 2; }
float speedOf(int kind) { return kind == 0 ? 28.f : kind == 1 ? 18.f : 36.f; }
float halfOf(int kind) { return kind == 0 ? 18.f : kind == 1 ? 22.f : 18.f; }
int palOf(int kind) { return kind == 0 ? PAL_RAIL : kind == 1 ? PAL_CRANE : PAL_BUS; }

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

float Game::deckY(int lane) const { return kLaneY[std::clamp(lane, 0, 2)]; }

int Game::quarry() const {
    int best = -1;
    float score = 1e9f;
    for (int i = 0; i < int(machines_.size()); i++) {
        const Machine& m = machines_[size_t(i)];
        if (!m.running) continue;
        float s = std::fabs(m.x - pX_) + std::fabs(float(m.lane - pLane_)) * 80.f + float(m.hp) * 6.f;
        if (s < score) {
            score = s;
            best = i;
        }
    }
    return best;
}

bool Game::boltDanger(int lane) const {
    for (const Bolt& s : bolts_) {
        if (s.from < 0 || s.lane != lane) continue;
        float dx = s.x - pX_;
        if (s.vx > 0.f && dx < 10.f && dx > -110.f) return true;
        if (s.vx < 0.f && dx > -10.f && dx < 110.f) return true;
    }
    return false;
}

bool Game::bodyClose(int lane) const {
    for (const Machine& m : machines_) {
        if (!m.running || m.lane != lane) continue;
        if (std::fabs(m.x - pX_) < halfOf(m.kind) + kPlayerHalf + 14.f) return true;
    }
    return false;
}

int Game::safeLane() const {
    for (int d = 1; d <= 2; d++) {
        for (int s : {1, -1}) {
            int lane = pLane_ + s * d;
            if (lane < 0 || lane > 2) continue;
            if (!boltDanger(lane) && !bodyClose(lane)) return lane;
        }
    }
    for (int lane = 0; lane < 3; lane++)
        if (!boltDanger(lane) && !bodyClose(lane)) return lane;
    return pLane_;
}

const gs::Mipped& Game::bodyOf(int kind, int frame) const {
    int f = frame & 1;
    if (kind == 0) return art_.rail[f];
    if (kind == 1) return art_.crane[f];
    return art_.bus[f];
}

void Game::resetWorld() {
    hull_ = kHull;
    pLane_ = 1;
    pDir_ = 1;
    pX_ = 64.f;
    t_ = 0;
    fireCd_ = 0.2f;
    laneCd_ = 0;
    hurtT_ = 0;
    shake_ = 0;
    bannerT_ = 0;
    banner_ = "";
    fanStep_ = -1;
    fanT_ = 0;
    over_ = false;
    won_ = false;
    reason_ = "NOT THE LAST";
    bolts_.clear();
    puffs_.clear();
    machines_.clear();

    auto add = [&](int kind, int lane, float x, int dir) {
        Machine m;
        m.kind = kind;
        m.lane = lane;
        m.dir = dir;
        m.hp = hpOf(kind);
        m.x = x;
        m.cool = 0.8f + float(kind) * 0.3f;
        m.laneWait = 1.4f;
        m.running = true;
        machines_.push_back(m);
    };
    add(0, 0, 240.f, -1);
    add(1, 2, 200.f, -1);
    add(2, 1, 270.f, -1);
}

void Game::bootTitle() {
    resetWorld();
    mode_ = Mode::Title;
    if (sys_) sys_->setLight(70, 80, 110);
}

void Game::begin() {
    resetWorld();
    mode_ = Mode::Play;
    if (sys_) {
        sys_->setLight(90, 120, 80);
        sys_->apu.tone(0, 294.f, 0.05f);
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
    for (const Bolt& s : bolts_)
        if (s.from < 0) n++;
    if (n >= 2) return;
    Bolt s;
    s.lane = pLane_;
    s.x = s.prev = pX_ + float(pDir_) * (kPlayerHalf + 4.f);
    s.vx = float(pDir_) * kShotV;
    s.from = -1;
    bolts_.push_back(s);
    fireCd_ = kFireCd;
    sys_->apu.tone(2, 680.f, 0.04f);
}

void Game::launchBolt(int index) {
    Machine& m = machines_[size_t(index)];
    for (const Bolt& s : bolts_)
        if (s.from == index) return;
    Bolt s;
    s.lane = m.lane;
    s.x = s.prev = m.x + float(m.dir) * (halfOf(m.kind) + 4.f);
    s.vx = float(m.dir) * kBoltV;
    s.from = index;
    bolts_.push_back(s);
    m.aim = 0;
    m.cool = 1.55f + float(m.kind) * 0.25f;
    if (sys_) sys_->apu.tone(1, 160.f, 0.03f);
}

void Game::hurtPlayer() {
    if (hurtT_ > 0.f || hull_ <= 0 || !sys_) return;
    hull_--;
    hurtT_ = 0.5f;
    shake_ = 1.f;
    sys_->rumble(0.55f, 0.75f, 90);
    sys_->apu.noiseBurst(0.35f, 180.f, 0.1f);
    if (hull_ <= 0) loseSpan("NOT THE LAST");
}

void Game::stopMachine(Machine& m) {
    if (!m.running) return;
    m.running = false;
    m.hp = 0;
    m.flash = 0.25f;
    banner_ = nameOf(m.kind);
    bannerT_ = 0.9f;
    shake_ = std::max(shake_, 0.3f);
    Puff w;
    w.lane = m.lane;
    w.x = m.x;
    w.life = 0.5f;
    puffs_.push_back(w);
    if (!sys_) return;
    sys_->apu.noiseBurst(0.32f, 110.f, 0.12f);
    sys_->apu.tone(0, 98.f, 0.05f);
}

void Game::winSpan() {
    if (mode_ == Mode::Victory) return;
    mode_ = Mode::Victory;
    over_ = true;
    won_ = true;
    reason_ = "LAST MACHINE STILL RUNNING";
    fanStep_ = 0;
    fanT_ = 0;
    if (!sys_) return;
    sys_->rumble(0.2f, 0.4f, 140);
    sys_->setLight(40, 170, 80);
}

void Game::loseSpan(const char* why) {
    if (mode_ == Mode::Fail || mode_ == Mode::Victory) return;
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    reason_ = why;
    hull_ = std::max(0, hull_);
    fanStep_ = 0;
    fanT_ = 0;
    if (sys_) sys_->setLight(170, 30, 24);
}

Game::Plan Game::botPlan() const {
    Plan plan;
    plan.lane = pLane_;
    plan.face = pDir_;
    plan.move = 0;
    plan.fire = false;
    int qi = quarry();
    if (boltDanger(pLane_) || bodyClose(pLane_)) plan.lane = safeLane();
    if (qi < 0) return plan;
    const Machine& q = machines_[size_t(qi)];
    plan.face = q.x >= pX_ ? 1 : -1;
    float adx = std::fabs(q.x - pX_);
    if (q.lane != plan.lane && !boltDanger(q.lane) && !bodyClose(q.lane)) plan.lane = q.lane;
    if (boltDanger(plan.lane) || bodyClose(plan.lane)) plan.lane = safeLane();
    bool lined = plan.lane == q.lane && pLane_ == q.lane;
    if (lined && adx > 40.f && adx < 150.f) plan.fire = true;
    if (adx > 100.f) plan.move = plan.face;
    else if (adx < 48.f) plan.move = -plan.face;
    if (plan.move != 0) {
        float nx = pX_ + float(plan.move) * 8.f;
        for (const Machine& m : machines_) {
            if (!m.running || m.lane != pLane_) continue;
            if (std::fabs(m.x - nx) < halfOf(m.kind) + kPlayerHalf + 6.f) plan.move = 0;
        }
    }
    return plan;
}

void Game::update(float dt) {
    if (mode_ != Mode::Play) return;
    t_ += dt;
    fireCd_ = std::max(0.f, fireCd_ - dt);
    laneCd_ = std::max(0.f, laneCd_ - dt);
    hurtT_ = std::max(0.f, hurtT_ - dt);
    shake_ = std::max(0.f, shake_ - dt);
    bannerT_ = std::max(0.f, bannerT_ - dt);

    int move = 0;
    int face = pDir_;
    int lane = pLane_;
    bool fire = false;
    if (bot_) {
        Plan plan = botPlan();
        move = plan.move;
        face = plan.face;
        lane = plan.lane;
        fire = plan.fire;
    } else if (sys_) {
        const gs::Pad& pad = sys_->pad;
        if (pad.down(gs::BTN_LEFT)) move = -1;
        if (pad.down(gs::BTN_RIGHT)) move = 1;
        if (pad.down(gs::BTN_UP)) lane = std::max(0, pLane_ - 1);
        if (pad.down(gs::BTN_DOWN)) lane = std::min(2, pLane_ + 1);
        fire = pad.down(gs::BTN_A) || pad.down(gs::BTN_B);
        if (move != 0) face = move;
        float ax = pad.axisX;
        if (std::fabs(ax) > 0.35f) {
            move = ax > 0.f ? 1 : -1;
            face = move;
        }
    }
    pDir_ = face;
    if (lane != pLane_ && laneCd_ <= 0.f && !bodyClose(lane)) {
        pLane_ = lane;
        laneCd_ = kLaneCd;
    }
    pX_ = std::clamp(pX_ + float(move) * kSpeed * dt, kLeft, kRight);
    if (fire) launchPlayer();

    for (int i = 0; i < int(machines_.size()); i++) {
        Machine& m = machines_[size_t(i)];
        m.flash = std::max(0.f, m.flash - dt);
        if (!m.running) continue;
        m.cool -= dt;
        m.laneWait -= dt;
        float dx = pX_ - m.x;
        float gap = halfOf(m.kind) + kPlayerHalf;
        bool same = m.lane == pLane_;
        if (same && std::fabs(dx) < 150.f && std::fabs(dx) > gap + 6.f) m.dir = dx > 0.f ? 1 : -1;
        float step = speedOf(m.kind) * dt;
        if (same && std::fabs(dx) < gap + 28.f) {
            m.dir = dx > 0.f ? -1 : 1;
            step *= 1.4f;
        }
        m.x += float(m.dir) * step;
        if (m.x < kLeft + 8.f) {
            m.x = kLeft + 8.f;
            m.dir = 1;
        } else if (m.x > kRight - 8.f) {
            m.x = kRight - 8.f;
            m.dir = -1;
        }
        if (m.laneWait <= 0.f && m.lane != pLane_) {
            int next = m.lane + (pLane_ > m.lane ? 1 : -1);
            if (std::fabs(m.x - pX_) > gap + 36.f) m.lane = next;
            m.laneWait = 2.6f;
        }
        bool facing = (m.dir > 0 && dx > 0.f) || (m.dir < 0 && dx < 0.f);
        if (same && facing && std::fabs(dx) > gap && std::fabs(dx) < 170.f) {
            m.aim += dt;
            if (m.aim > 0.85f && m.cool <= 0.f) launchBolt(i);
        } else {
            m.aim = std::max(0.f, m.aim - dt);
        }
        if (same && std::fabs(m.x - pX_) < gap - 4.f) {
            hurtPlayer();
            m.dir = pX_ < m.x ? 1 : -1;
            m.x = std::clamp(m.x + float(m.dir) * 10.f, kLeft, kRight);
            pX_ = std::clamp(pX_ - float(m.dir) * 16.f, kLeft, kRight);
            if (mode_ != Mode::Play) return;
        }
    }

    std::vector<Bolt> keep;
    keep.reserve(bolts_.size());
    for (Bolt s : bolts_) {
        s.prev = s.x;
        s.x += s.vx * dt;
        if (s.x < -20.f || s.x > 340.f) continue;
        bool hit = false;
        if (s.from < 0) {
            for (Machine& m : machines_) {
                if (!m.running || m.lane != s.lane) continue;
                if (!crossed(s.prev, s.x, m.x, halfOf(m.kind))) continue;
                m.hp--;
                m.flash = 0.12f;
                if (m.hp <= 0) stopMachine(m);
                hit = true;
                break;
            }
        } else if (s.lane == pLane_ && crossed(s.prev, s.x, pX_, kPlayerHalf)) {
            hurtPlayer();
            hit = true;
        }
        if (!hit) keep.push_back(s);
        if (mode_ != Mode::Play) return;
    }
    bolts_.swap(keep);

    for (Puff& w : puffs_) w.age += dt;
    puffs_.erase(std::remove_if(puffs_.begin(), puffs_.end(), [](const Puff& w) { return w.age >= w.life; }),
                 puffs_.end());

    if (mode_ == Mode::Play && hull_ > 0 && rivalsRunning() == 0) winSpan();
}

void Game::spr(const gs::Mipped& m, float cx, float top, float h, int pal, bool flip) {
    if (h < 1.f || m.h < 1 || !sys_) return;
    float w = h * (float(m.w) / float(m.h));
    gs::Sprite s;
    s.x = int16_t(std::lround(cx - w * 0.5f));
    s.y = int16_t(std::lround(top));
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || !sys_ || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        int x = col + i;
        if (x < 0 || x > 39 || c < 32 || c > 126) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = int(std::strlen(s));
    hud(20 - n / 2, row, s, pal);
}

void Game::text(const char* s, float x, float y, float scale, int pal) {
    int n = int(std::strlen(s));
    float adv = 16.f * scale;
    float left = x - n * adv * 0.5f;
    for (int i = 0; i < n; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < 33 || c > 126) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, left + float(i) * adv + adv * 0.5f, y, std::max(8.f, float(g.h) * scale), pal, false);
    }
}

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.road[y].on = false;
        if (y < 78) {
            v.lineBackdrop[y] = gs::rgb4(2 + y / 28, 3 + y / 36, 8 - y / 40);
            v.lineFog[y] = 1;
        } else if (y < 100) {
            v.lineBackdrop[y] = gs::rgb4(5, 5, 5);
            v.lineFog[y] = 2;
        } else {
            int band = ((y / 6) & 1);
            v.lineBackdrop[y] = gs::rgb4(1, 2 + band, 4);
            v.lineFog[y] = 5;
        }
    }
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.hudEnabled = true;
    sky();

    float ox = 0;
    if (shake_ > 0.f) ox = std::sin(t_ * 48.f) * 3.f;

    for (int i = 0; i < 4; i++) {
        float x = 48.f + float(i) * 74.f + ox * 0.2f;
        spr(art_.pier, x, 28.f, 150.f, PAL_STONE, false);
        spr(art_.lamp, x, 22.f, 12.f, (i & 1) ? PAL_BOLT : PAL_HUD, false);
    }

    auto paint = [&](bool wrecks) {
        for (const Machine& m : machines_) {
            if (m.running == wrecks) continue;
            int fr = m.running ? (int(t_ * 6.f) & 1) : 0;
            float top = deckY(m.lane) - (m.kind == 1 ? 30.f : 24.f);
            int pal = m.flash > 0.f ? PAL_BOLT : palOf(m.kind);
            spr(bodyOf(m.kind, fr), m.x + ox, top, m.kind == 1 ? 32.f : 26.f, pal, m.dir < 0);
        }
    };
    paint(false);

    for (const Puff& w : puffs_) {
        float k = 1.f - w.age / w.life;
        spr(art_.puff, w.x, deckY(w.lane) - 20.f, 10.f + k * 12.f, PAL_ALERT, false);
    }
    for (const Bolt& b : bolts_)
        spr(art_.bolt, b.x, deckY(b.lane) - 10.f, 7.f, b.from < 0 ? PAL_BOLT : PAL_ALERT, b.vx < 0.f);

    int fr = int(t_ * 8.f) & 1;
    if (hurtT_ <= 0.f || int(t_ * 20.f) & 1)
        spr(art_.runner[fr], pX_ + ox, deckY(pLane_) - 24.f, 26.f, PAL_YOU, pDir_ < 0);
    paint(true);

    char line[64];
    if (mode_ == Mode::Title) {
        hudC(21, "ONE VIADUCT.", PAL_HUD);
        hudC(22, "BE THE LAST MACHINE STILL RUNNING.", PAL_OK);
        hudC(23, "LEFT RIGHT ALONG THE DECK", PAL_HUD);
        hudC(24, "UP DOWN BETWEEN THE LANES", PAL_HUD);
        hudC(25, "A FIRES. THEN IT IS DONE.", PAL_HUD);
        hudC(26, "ENTER", PAL_HUD);
        int n = int(std::strlen(S3_VERSION_STRING));
        hud(39 - n, 0, S3_VERSION_STRING, PAL_HUD);
        text("VIADUCT PURSUIT", 160.f, 8.f, 0.62f, PAL_HUD);
    } else {
        std::snprintf(line, sizeof line, "HULL %d", hull_);
        hud(1, 0, line, hull_ > 2 ? PAL_HUD : PAL_ALERT);
        std::snprintf(line, sizeof line, "RUN %d", rivalsRunning());
        hud(12, 0, line, PAL_HUD);
        std::snprintf(line, sizeof line, "STOP %d", stoppedCount());
        hud(28, 0, line, PAL_OK);
        if (mode_ == Mode::Pause) hudC(26, "PAUSED", PAL_HUD);
        else if (mode_ == Mode::Victory) hudC(26, "LAST MACHINE STILL RUNNING", PAL_OK);
        else if (mode_ == Mode::Fail) hudC(26, reason_, PAL_ALERT);
        else if (bannerT_ > 0.f && banner_) hudC(26, banner_, PAL_BOLT);
        else hudC(26, "HOLD THE DECK", PAL_HUD);
        if (mode_ == Mode::Victory) text("LAST", 160.f, 6.f, 1.f, PAL_OK);
        else if (mode_ == Mode::Fail) text("LOST", 160.f, 6.f, 1.f, PAL_ALERT);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    bool start = !bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C));
    bool back = !bot_ && pad.pressed(gs::BTN_MODE);
    if (back && mode_ == Mode::Title) {
        if (sys.hasHome()) sys.eject();
        else sys.quit();
    } else if (back && mode_ == Mode::Play) mode_ = Mode::Pause;
    else if (back && mode_ == Mode::Pause) mode_ = Mode::Title;
    else if (start && (mode_ == Mode::Title || mode_ == Mode::Victory || mode_ == Mode::Fail)) begin();
    else if (start && mode_ == Mode::Play) mode_ = Mode::Pause;
    else if (start && mode_ == Mode::Pause) mode_ = Mode::Play;

    if (mode_ == Mode::Play) update(kDt);
    else if ((mode_ == Mode::Victory || mode_ == Mode::Fail) && fanStep_ >= 0) {
        fanT_ += kDt;
        if (fanT_ > 0.16f) {
            fanT_ = 0;
            static const float notes[] = {330.f, 392.f, 494.f, 659.f};
            if (fanStep_ < 4 && mode_ == Mode::Victory) sys.apu.tone(0, notes[fanStep_], 0.06f);
            fanStep_++;
            if (fanStep_ > 6) {
                fanStep_ = -1;
                sys.apu.tone(0, 0, 0);
            }
        }
    } else if (mode_ == Mode::Title) {
        t_ += kDt;
    }
    draw();
}

}  // namespace viaductpurs

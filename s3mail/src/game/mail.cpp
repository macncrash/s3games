#include "game/mail.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "../version.h"

namespace mail {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float TAU = 6.2831853f;
constexpr float PI = 3.14159265f;

// Box, turn, box, turn, box, turn, box, then the depot.
constexpr float kBoxS[4] = {48.f, 124.f, 206.f, 288.f};
constexpr int kBoxSide[4] = {1, -1, 1, -1};
constexpr float kTurnS[3] = {84.f, 166.f, 248.f};
constexpr int kTurnSide[3] = {-1, 1, -1};
constexpr float FINISH = 328.f;
constexpr float TITLE_S = 36.f;

constexpr float CRUISE = 12.6f;
constexpr float FAST = 16.4f;
constexpr float EASY = 8.4f;
constexpr float ACCEL = 8.5f;
constexpr float DRAG = 3.2f;
constexpr float CURB = 2.55f;
constexpr int PAPERS = 6;

// A toss leaves the hand and lands this far down the street.
constexpr float FLIGHT = 0.30f;
constexpr float TOSS = 5.2f;
constexpr float HIT = 3.8f;
constexpr float BOX_LAT = 4.55f;

constexpr float BEND_LEN = 32.f;
constexpr float BEND_AMT = 0.78f;
constexpr float APPROACH = 26.f;
constexpr float HOLD_AT = 8.f;
constexpr float PLACED = 0.9f;
constexpr float HELD = 0.5f;

constexpr float HORIZON = 92.f;
constexpr float CAM_H = 1.15f;
constexpr float FOCAL = 240.f;
constexpr float ROAD_HALF = 3.35f;

constexpr int HOUSE = 0, TREE = 1, LAMP = 2, SIGN = 3, DEPOT = 4;

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    const int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    const int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    return gs::rgb4(int(std::lround(ar + (br - ar) * t)), int(std::lround(ag + (bg - ag) * t)),
                     int(std::lround(ab + (bb - ab) * t)));
}

gs::FMPatch airPatch() {
    gs::FMPatch p;
    p.alg = 4;
    p.fb = 0.08f;
    p.vol = 0.1f;
    p.tone = 900.f;
    p.echo = 0.28f;
    p.op[0] = {1.f, 0.35f, 0.12f, 0.5f, 0.8f, 0.5f, 0.f};
    p.op[1] = {1.f, 0.7f, 0.08f, 0.55f, 0.85f, 0.45f, 0.f};
    p.op[2] = {2.f, 0.16f, 0.15f, 0.4f, 0.4f, 0.4f, 0.f};
    p.op[3] = {0.5f, 0.22f, 0.12f, 0.5f, 0.7f, 0.5f, 0.f};
    return p;
}

gs::FMPatch bellPatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.04f;
    p.vol = 0.22f;
    p.echo = 0.4f;
    p.tone = 3200.f;
    p.op[0] = {1.f, 0.7f, 0.004f, 0.16f, 0.f, 0.26f, 0.f};
    p.op[1] = {2.f, 0.42f, 0.004f, 0.12f, 0.f, 0.2f, 0.f};
    p.op[2] = {3.2f, 0.28f, 0.004f, 0.1f, 0.f, 0.16f, 0.f};
    p.op[3] = {4.6f, 0.16f, 0.004f, 0.08f, 0.f, 0.14f, 0.f};
    return p;
}

}  // namespace

int Game::boxesGot() const {
    int n = 0;
    for (const Box& b : boxes_)
        if (b.got) n++;
    return n;
}

int Game::turnsMade() const {
    int n = 0;
    for (const Turn& t : turns_)
        if (t.made) n++;
    return n;
}

int Game::boxesLeft() const {
    int n = 0;
    for (const Box& b : boxes_)
        if (!b.got) n++;
    return n;
}

int Game::nextBox() const {
    for (int i = 0; i < 4; i++)
        if (!boxes_[i].got) return i;
    return -1;
}

int Game::nextTurn() const {
    for (int i = 0; i < 3; i++)
        if (!turns_[i].resolved) return i;
    return -1;
}

float Game::leadDist() const { return v_ * FLIGHT + TOSS; }

float Game::curvature(float s) const {
    // Cosine bump. The integral across the bend is the heading change.
    float k = 0.f;
    for (int i = 0; i < 3; i++) {
        const float u = (s - (kTurnS[i] - BEND_LEN * 0.5f)) / BEND_LEN;
        if (u <= 0.f || u >= 1.f) continue;
        const float window = 1.f - std::cos(u * TAU);
        k += float(kTurnSide[i]) * (BEND_AMT / BEND_LEN) * window;
    }
    return k;
}

bool Game::throwWindow(int bi) const {
    if (bi < 0 || boxes_[bi].got) return false;
    return std::fabs(s_ + leadDist() - boxes_[bi].s) <= HIT;
}

bool Game::tossHits(int bi) const {
    if (!paper_.live || bi < 0 || boxes_[bi].got) return false;
    if (paper_.side != boxes_[bi].side) return false;
    return std::fabs(paper_.landS - boxes_[bi].s) <= HIT;
}

void Game::layout() {
    props_.clear();
    for (int i = 0; i < 4; i++) {
        boxes_[i].s = kBoxS[i];
        boxes_[i].side = kBoxSide[i];
        boxes_[i].got = false;
        props_.push_back({kBoxS[i] + 1.2f, float(kBoxSide[i]) * 6.9f, HOUSE, i % 3});
    }
    for (int i = 0; i < 3; i++) {
        turns_[i].s = kTurnS[i];
        turns_[i].side = kTurnSide[i];
        turns_[i].made = false;
        turns_[i].resolved = false;
        props_.push_back({kTurnS[i] - 18.f, float(kTurnSide[i]) * 4.35f, SIGN, kTurnSide[i] > 0 ? 1 : 0});
    }
    for (int n = 0; n < 14; n++) {
        const float s = 18.f + float(n) * 22.f;
        const int side = (n & 1) ? -1 : 1;
        bool busy = false;
        for (int i = 0; i < 4; i++)
            if (std::fabs(kBoxS[i] - s) < 9.f && kBoxSide[i] == side) busy = true;
        if (!busy && s < FINISH) props_.push_back({s, float(side) * 7.1f, HOUSE, n % 3});
    }
    for (int n = 0; n < 18; n++) {
        const float s = 12.f + float(n) * 18.f;
        const int side = (n & 1) ? 1 : -1;
        bool busy = false;
        for (int i = 0; i < 4; i++)
            if (std::fabs(kBoxS[i] - s) < 6.f) busy = true;
        if (!busy) props_.push_back({s, float(side) * 8.3f, TREE, 0});
    }
    for (int n = 0; n < 9; n++) {
        const float s = 26.f + float(n) * 34.f;
        const int side = (n & 1) ? -1 : 1;
        bool busy = false;
        for (int i = 0; i < 4; i++)
            if (std::fabs(kBoxS[i] - s) < 5.f) busy = true;
        if (!busy) props_.push_back({s, float(side) * 5.15f, LAMP, 0});
    }
    props_.push_back({FINISH + 12.f, 7.2f, DEPOT, 0});
}

void Game::clearProgress() {
    for (Box& b : boxes_) b.got = false;
    for (Turn& t : turns_) {
        t.made = false;
        t.resolved = false;
    }
    paper_ = {};
    for (Puff& p : puffs_) p.t = 0;
    for (Pop& p : pops_) p.t = 0;
    puffN_ = popN_ = 0;
    x_ = vx_ = steer_ = 0;
    clock_ = shake_ = chain_ = 0;
    papers_ = PAPERS;
    won_ = false;
    over_ = false;
    fail_ = "";
    fanStep_ = -1;
    fanT_ = 0;
    v_ = CRUISE;
}

void Game::resetRun() {
    clearProgress();
    mode_ = Mode::Run;
    s_ = 0.f;
}

void Game::enterTitle() {
    clearProgress();
    mode_ = Mode::Title;
    s_ = TITLE_S;
}

void Game::addPop(float s, float x, const char* text, int pal) {
    Pop& p = pops_[popN_++ % 8];
    p = {s, x, 1.15f, text, pal};
}

void Game::addPuff(float s, float x) {
    Puff& p = puffs_[puffN_++ % 10];
    p = {s, x, 0.42f};
}

void Game::throwPaper() {
    if (mode_ != Mode::Run || paper_.live) return;
    const int bi = nextBox();
    if (bi < 0) return;
    if (papers_ <= 0) {
        sys_->apu.noiseBurst(0.05f, 400.f, 0.05f);
        return;
    }
    papers_--;
    paper_.live = true;
    paper_.t = 0;
    paper_.dur = FLIGHT;
    paper_.side = boxes_[bi].side;
    paper_.landS = s_ + leadDist();
    paper_.landX = float(boxes_[bi].side) * BOX_LAT;
    sys_->apu.noiseBurst(0.16f, 2600.f, 0.07f);
}

void Game::updatePaper(float dt) {
    if (!paper_.live) return;
    paper_.t += dt;
    if (paper_.t < paper_.dur) return;
    int hit = -1;
    for (int i = 0; i < 4; i++) {
        if (boxes_[i].got || paper_.side != boxes_[i].side) continue;
        if (std::fabs(paper_.landS - boxes_[i].s) <= HIT) {
            hit = i;
            break;
        }
    }
    addPuff(paper_.landS, paper_.landX);
    paper_.live = false;
    if (hit >= 0) {
        boxes_[hit].got = true;
        addPop(boxes_[hit].s, float(boxes_[hit].side) * BOX_LAT, "IN", PAL_GREEN);
        sys_->apu.keyOn(1, 880.f, 0.2f);
        sys_->rumble(0.15f, 0.35f, 70);
    }
    if (papers_ < boxesLeft()) fail("SHORT A PAPER");
}

void Game::checkPass() {
    if (mode_ != Mode::Run) return;
    for (int i = 0; i < 4; i++) {
        if (boxes_[i].got) continue;
        if (s_ <= boxes_[i].s + 2.4f) continue;
        if (tossHits(i)) continue;
        fail("THE BOX MISSED THE PAPER");
        return;
    }
}

void Game::checkTurns() {
    if (mode_ != Mode::Run) return;
    for (int i = 0; i < 3; i++) {
        Turn& t = turns_[i];
        if (t.resolved || s_ < t.s) continue;
        t.resolved = true;
        const bool held = steer_ * float(t.side) > HELD;
        const bool placed = x_ * float(t.side) > PLACED;
        if (held && placed) {
            t.made = true;
            addPop(t.s, x_, "MADE", PAL_GOLD);
            sys_->apu.keyOn(1, 660.f, 0.18f);
        } else {
            x_ = -float(t.side) * 4.6f;
            vx_ = 0;
            fail("MISSED THE TURN");
            return;
        }
    }
}

void Game::checkFinish() {
    if (mode_ != Mode::Run || s_ < FINISH) return;
    if (boxesGot() == 4 && turnsMade() == 3) win();
    else fail("SHORT THE ROUTE");
}

void Game::win() {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    fanStep_ = 1;
    fanT_ = 0;
    sys_->apu.keyOn(1, 523.f, 0.22f);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    fail_ = why;
    shake_ = 1.f;
    sys_->apu.keyOn(1, 98.f, 0.26f);
    sys_->rumble(0.7f, 0.45f, 200);
}

float Game::botSteer() const {
    const int ti = nextTurn();
    if (ti >= 0) {
        const float d = turns_[ti].s - s_;
        if (d < APPROACH && d > -4.f) return float(turns_[ti].side);
    }
    return std::clamp((-1.5f * x_ - 0.5f * vx_) / 4.f, -1.f, 1.f);
}

bool Game::botToss() const {
    const int bi = nextBox();
    if (bi < 0 || paper_.live || papers_ <= 0) return false;
    return std::fabs(s_ + leadDist() - boxes_[bi].s) <= 0.4f;
}

void Game::updateRun(float steer, bool fast, bool easy, bool toss) {
    steer_ = std::clamp(steer, -1.f, 1.f);
    if (bot_) v_ = CRUISE;
    else {
        float want = CRUISE;
        if (fast) want = FAST;
        if (easy) want = EASY;
        v_ += (want - v_) * std::min(1.f, 2.8f * DT);
    }
    vx_ += (steer_ * ACCEL - vx_ * DRAG) * DT;
    x_ += vx_ * DT;
    if (std::fabs(x_) > CURB) {
        const float sgn = x_ > 0.f ? 1.f : -1.f;
        x_ = sgn * CURB;
        if (vx_ * sgn > 0.f) vx_ = 0.f;
    }
    s_ += v_ * DT;
    clock_ += DT;
    updatePaper(DT);
    if (toss) throwPaper();
    checkPass();
    checkTurns();
    checkFinish();
}

void Game::audio(float dt) {
    if (mode_ == Mode::Run) {
        sys_->apu.tone(0, 80.f + v_ * 3.5f, 0.028f);
        chain_ += v_ * dt;
        if (chain_ > 2.4f) {
            chain_ = 0;
            sys_->apu.noiseBurst(0.025f, 1400.f, 0.02f);
        }
    } else {
        sys_->apu.tone(0, 0, 0);
    }
    if (mode_ == Mode::Win && fanStep_ >= 1 && fanStep_ < 4) {
        static const float notes[4] = {523.f, 659.f, 784.f, 1046.f};
        fanT_ += dt;
        if (fanT_ >= 0.16f) {
            sys_->apu.keyOn(1, notes[fanStep_], 0.2f);
            fanStep_++;
            fanT_ = 0;
        }
    }
    for (Puff& p : puffs_)
        if (p.t > 0.f) p.t -= dt;
    for (Pop& p : pops_)
        if (p.t > 0.f) p.t -= dt;
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - dt * 1.6f);
}

void Game::cacheAhead() {
    float h = 0, x = 0;
    ahead_[0] = {0, 0};
    for (int i = 1; i <= AHEAD_N; i++) {
        h += curvature(s_ + float(i - 1));
        x += std::sin(h);
        ahead_[i] = {h, x};
    }
}

void Game::roadPoint(float dist, float& h, float& x) const {
    if (dist <= 0.f) {
        h = x = 0.f;
        return;
    }
    if (dist >= float(AHEAD_N)) {
        const Ahead& a = ahead_[AHEAD_N];
        const float extra = dist - float(AHEAD_N);
        h = a.h;
        x = a.x + std::sin(h) * extra;
        return;
    }
    const int i = int(dist);
    const float f = dist - float(i);
    const Ahead& a = ahead_[i];
    const Ahead& b = ahead_[i + 1];
    h = a.h + (b.h - a.h) * f;
    x = a.x + (b.x - a.x) * f;
}

bool Game::project(float wx, float ws, float& sx, float& sy, float& scale, int& fog, float minDist) const {
    const float dist = ws - s_;
    if (dist < minDist || dist > 168.f) return false;
    float h, cx;
    roadPoint(dist, h, cx);
    const float px = cx + wx * std::cos(h);
    const float pz = dist - wx * std::sin(h);
    if (pz < minDist) return false;
    const float dx = px - x_;
    scale = FOCAL / pz;
    sx = 160.f + dx * scale + camX_;
    sy = HORIZON + camY_ + CAM_H * scale;
    fog = pz > 28.f ? std::clamp(int((pz - 28.f) / 9.f), 0, 12) : 0;
    return true;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet, bool shadow) {
    if (m.h <= 0 || h < 1.f || h > 460.f) return;
    gs::Sprite s;
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.w = int16_t(std::clamp(long(std::lround(h * float(m.w) / float(m.h))), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 80 || s.x + s.w < -80 || s.y > gs::SCREEN_H + 40 || s.y + s.h < -40) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* text, int pal) {
    if (!text || row < 0 || row > 27) return;
    for (int i = 0; text[i] != 0; i++) {
        const int x = col + i;
        const unsigned char c = static_cast<unsigned char>(text[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* text, int pal) {
    int n = 0;
    if (text)
        while (text[n]) n++;
    hud(20 - n / 2, row, text, pal);
}

void Game::text(const char* s, float x, float y, float scale, int pal) {
    if (!s || !*s) return;
    float width = 0.f;
    for (const char* p = s; *p; ++p) {
        const unsigned char c = static_cast<unsigned char>(*p);
        if (c < 32 || c >= 128) continue;
        width += float(art_.glyph[c - 32].w) * scale;
    }
    float pen = x - width * 0.5f;
    for (const char* p = s; *p; ++p) {
        const unsigned char c = static_cast<unsigned char>(*p);
        if (c < 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        const float w = float(g.w) * scale;
        if (c > 32) spr(g, pen + w * 0.5f, y, float(g.h) * scale, pal, false, 0, false, false);
        pen += w;
    }
}

void Game::drawRoad() {
    gs::VDP& v = sys_->vdp;
    const uint16_t top = gs::rgb4(4, 7, 12);
    const uint16_t mid = gs::rgb4(8, 12, 15);
    const uint16_t horC = gs::rgb4(15, 12, 8);
    const float hor = HORIZON + camY_;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (float(y) < hor) {
            const float u = float(y) / std::max(1.f, hor);
            v.lineBackdrop[y] = u < 0.55f ? lerpC(top, mid, u / 0.55f) : lerpC(mid, horC, (u - 0.55f) / 0.45f);
            v.lineFog[y] = float(y) > hor - 14.f ? uint8_t((hor - float(y)) < 6.f ? 3 : 1) : 0;
            v.road[y].on = false;
            continue;
        }
        const float row = std::max(1.f, float(y) - hor);
        const float dist = CAM_H * FOCAL / row;
        float h = 0.f, cx = 0.f;
        roadPoint(dist, h, cx);
        (void)h;
        const float dx = cx - x_;
        const float scale = FOCAL / std::max(0.8f, dist);
        gs::RoadLine& r = v.road[y];
        r.on = true;
        r.cx = 160.f + dx * scale + camX_;
        r.hw = ROAD_HALF * scale;
        r.v = (s_ + dist) * 42.f;
        r.pal = PAL_ROAD;
        r.style = 1;
        r.band = (int(std::floor((s_ + dist) / 14.f)) & 1) ? 1 : 0;
        r.left = r.right = gs::GROUND_LAND;
        int fog = 0;
        if (dist > 26.f) fog = std::clamp(int((dist - 26.f) / 10.f), 0, 10);
        v.lineFog[y] = uint8_t(fog);
        v.lineBackdrop[y] = horC;
    }
}

void Game::drawWorld() {
    struct Item {
        float z;
        int kind;
        int index;
        float absx;
    };
    Item items[96];
    int n = 0;
    auto push = [&](float z, int kind, int index, float ax) {
        if (n < 96 && z > 0.6f && z < 160.f) items[n++] = {z, kind, index, ax};
    };
    for (int i = 0; i < int(props_.size()); i++) push(props_[size_t(i)].s - s_, 0, i, std::fabs(props_[size_t(i)].x));
    for (int i = 0; i < 4; i++) push(boxes_[i].s - s_, 1, i, BOX_LAT);
    std::sort(items, items + n, [](const Item& a, const Item& b) {
        if (a.z != b.z) return a.z < b.z;
        return a.absx < b.absx;
    });
    const int bi = nextBox();
    const int ti = nextTurn();
    for (int i = 0; i < n; i++) {
        const Item& it = items[i];
        if (it.kind == 1) {
            const Box& b = boxes_[it.index];
            float sx, sy, scale;
            int fog;
            if (!project(float(b.side) * BOX_LAT, b.s, sx, sy, scale, fog, 1.4f)) continue;
            float h = 1.28f * scale;
            if (it.index == bi && mode_ == Mode::Run) h *= 1.f + 0.07f * std::sin(t_ * 9.f);
            spr(b.got ? art_.boxOpen : art_.boxShut, sx, sy, h, PAL_BOX, b.side > 0, fog, true, false);
            continue;
        }
        const Prop& p = props_[size_t(it.index)];
        float sx, sy, scale;
        int fog;
        const float minD = p.kind == SIGN ? 2.2f : p.kind == LAMP ? 2.8f : 4.2f;
        if (!project(p.x, p.s, sx, sy, scale, fog, minD)) continue;
        if (p.kind == HOUSE) {
            spr(art_.house[p.variant % 3], sx, sy, 4.5f * scale, PAL_HOUSE, p.x > 0, fog, true, false);
        } else if (p.kind == TREE) {
            spr(art_.tree, sx, sy, 4.6f * scale, PAL_TREE, p.x > 0, fog, true, false);
        } else if (p.kind == LAMP) {
            spr(art_.lamp, sx, sy, 2.7f * scale, PAL_BOX, false, fog, true, false);
        } else if (p.kind == SIGN) {
            float h = 1.85f * scale;
            if (ti >= 0 && std::fabs(p.s - (turns_[ti].s - 18.f)) < 0.5f) h *= 1.f + 0.08f * std::sin(t_ * 8.f);
            spr(art_.sign, sx, sy, h, PAL_SIGN, p.variant != 0, fog, true, false);
        } else if (p.kind == DEPOT) {
            spr(art_.depot, sx, sy, 5.1f * scale, PAL_HOUSE, false, fog, true, false);
        }
    }
    for (const Puff& p : puffs_) {
        if (p.t <= 0.f) continue;
        float sx, sy, scale;
        int fog;
        if (!project(p.x, p.s, sx, sy, scale, fog, 1.2f)) continue;
        const float h = (8.f + (0.42f - p.t) * 30.f) * (scale / 20.f);
        spr(art_.puff, sx, sy - 10.f, std::max(6.f, h), PAL_FX, false, fog, false, false);
    }
}

void Game::drawBike() {
    int frame = 1;
    if (steer_ < -0.25f || vx_ < -0.45f) frame = 0;
    else if (steer_ > 0.25f || vx_ > 0.45f) frame = 2;
    const float bob = std::sin((mode_ == Mode::Run ? clock_ : t_) * (mode_ == Mode::Run ? 14.f : 6.f)) *
                      (mode_ == Mode::Run ? 1.8f : 1.2f);
    const float bx = 160.f + steer_ * 8.f + camX_;
    const float by = 218.f + bob + camY_;
    spr(art_.shadow, bx, by - 2.f, 18.f, PAL_FX, false, 0, true, true);
    spr(art_.bike[frame], bx, by, 108.f, PAL_BIKE, false, 0, true, false);
    const int n = std::clamp(papers_, 0, PAPERS);
    for (int i = 0; i < n; i++) {
        const float ox = float(i % 3 - 1) * 7.f;
        const float oy = float(i / 3) * 5.f;
        spr(art_.paper, bx + ox + steer_ * 4.f, by - 64.f - oy, 9.f, PAL_PAPER, (i & 1) != 0, 0, false, false);
    }
}

void Game::drawToss(float u, float landS, float landX) {
    float sx, sy, scale;
    int fog;
    const float bx = 160.f + steer_ * 8.f + camX_;
    const float by = 168.f + camY_;
    if (!project(landX, landS, sx, sy, scale, fog, 0.8f)) {
        sx = bx + (u > 0.5f ? (landX > 0 ? 40.f : -40.f) : 0.f);
        sy = by;
        scale = 18.f;
        fog = 0;
    }
    sy -= 0.75f * scale;
    const float x = bx + (sx - bx) * u;
    const float y = by + (sy - by) * u - std::sin(u * PI) * 34.f;
    const float h = 14.f - u * 4.f;
    spr(art_.paper, x, y, h, PAL_PAPER, u > 0.5f, fog, false, false);
}

void Game::drawCall() {
    if (mode_ == Mode::Title) {
        text("S3 MAIL", 160.f + camX_, 30.f, 1.25f, PAL_GOLD);
        return;
    }
    if (mode_ == Mode::Pause) {
        text("PAUSE", 160.f, 70.f, 1.1f, PAL_HUD);
        return;
    }
    if (mode_ == Mode::Win) {
        text("THE BOX GOT THE PAPER", 160.f, 36.f, 0.52f, PAL_GOLD);
        text("EVERY TURN MADE", 160.f, 62.f, 0.62f, PAL_GREEN);
        return;
    }
    if (mode_ == Mode::Fail) {
        text(fail_ != nullptr ? fail_ : "FAIL", 160.f, 44.f, 0.55f, PAL_RED);
        return;
    }
    const int bi = nextBox();
    const int ti = nextTurn();
    const char* call = nullptr;
    int pal = PAL_HUD;
    float sc = 0.9f;
    if (bi >= 0 && throwWindow(bi)) {
        call = "THROW";
        pal = PAL_HUD;
        sc = 1.2f + 0.06f * std::sin(t_ * 14.f);
    } else if (ti >= 0) {
        const float d = turns_[ti].s - s_;
        if (d < HOLD_AT && d > -1.f) {
            call = turns_[ti].side < 0 ? "HOLD LEFT" : "HOLD RIGHT";
            pal = PAL_RED;
            sc = 0.85f;
        } else if (d < APPROACH) {
            call = turns_[ti].side < 0 ? "LEFT" : "RIGHT";
            pal = PAL_GOLD;
            sc = 1.15f;
        }
    }
    if (call == nullptr && bi >= 0 && boxes_[bi].s - s_ < 22.f && boxes_[bi].s - s_ > -1.f) {
        call = boxes_[bi].side < 0 ? "BOX LEFT" : "BOX RIGHT";
        pal = PAL_GREEN;
        sc = 0.7f;
    }
    if (call) text(call, 160.f + camX_, 74.f, sc, pal);
}

void Game::drawHud() {
    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(8, "THE BOX GETS THE PAPER", PAL_GOLD);
        hudC(10, "DON'T MISS THE TURN", PAL_HUD);
        hudC(13, "4 BOXES    3 TURNS    6 PAPERS", PAL_HUD);
        hudC(15, "STEER INTO THE CORNER", PAL_GOLD);
        hudC(17, "ARROWS STEER     C THROW", PAL_HUD);
        hudC(18, "UP FAST     DOWN EASY", PAL_HUD);
        if ((int(t_ * 2.f) & 1) == 0) hudC(21, "PRESS START", PAL_GOLD);
        hud(40 - int(std::strlen(S3_VERSION_STRING)), 27, S3_VERSION_STRING, PAL_HUD);
        return;
    }
    if (mode_ == Mode::Run || mode_ == Mode::Pause) {
        std::snprintf(buf, sizeof buf, "BOXES %d/4", boxesGot());
        hud(1, 0, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "TURNS %d/3", turnsMade());
        hud(14, 0, buf, PAL_GOLD);
        std::snprintf(buf, sizeof buf, "PAPER %d", papers_);
        hud(28, 0, buf, papers_ < boxesLeft() ? PAL_RED : PAL_HUD);

        const int bi = nextBox();
        const int ti = nextTurn();
        if (bi >= 0 && (ti < 0 || boxes_[bi].s <= turns_[ti].s))
            std::snprintf(buf, sizeof buf, "BOX %d M", int(std::max(0.f, boxes_[bi].s - s_) + 0.5f));
        else if (ti >= 0)
            std::snprintf(buf, sizeof buf, "TURN %d M", int(std::max(0.f, turns_[ti].s - s_) + 0.5f));
        else
            std::snprintf(buf, sizeof buf, "DEPOT %d M", int(std::max(0.f, FINISH - s_) + 0.5f));
        hud(1, 1, buf, PAL_HUD);
        const int cs = int(std::lround(clock_ * 10.f));
        std::snprintf(buf, sizeof buf, "%d.%d", cs / 10, cs % 10);
        hud(34, 1, buf, PAL_HUD);

        char lane[18];
        const int pos = std::clamp(int(std::lround(x_ / CURB * 8.f + 8.f)), 0, 16);
        for (int i = 0; i < 17; i++) lane[i] = (i == pos) ? 'O' : '-';
        lane[17] = 0;
        int lp = PAL_GOLD;
        if (ti >= 0 && turns_[ti].s - s_ < APPROACH) lp = (x_ * float(turns_[ti].side) > PLACED) ? PAL_GREEN : PAL_RED;
        hud(1, 26, "LANE", PAL_HUD);
        hud(6, 26, lane, lp);
        if (mode_ == Mode::Pause) {
            hudC(16, "START  RESUME", PAL_HUD);
            hudC(17, "ESC  TITLE", PAL_HUD);
        }
        return;
    }
    std::snprintf(buf, sizeof buf, "BOXES %d/4   TURNS %d/3", boxesGot(), turnsMade());
    hudC(12, buf, PAL_HUD);
    const int cs = int(std::lround(clock_ * 10.f));
    std::snprintf(buf, sizeof buf, "%d.%d S", cs / 10, cs % 10);
    hudC(14, buf, PAL_GOLD);
    if (!bot_) {
        hudC(17, mode_ == Mode::Win ? "START  AGAIN" : "START  RETRY", PAL_HUD);
        hudC(18, "ESC  TITLE", PAL_HUD);
    }
}

void Game::draw() {
    camX_ = camY_ = 0.f;
    if (shake_ > 0.f) {
        camX_ = std::sin(t_ * 73.f) * shake_ * 7.f;
        camY_ = std::sin(t_ * 91.f) * shake_ * 3.f;
    }
    cacheAhead();
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    drawRoad();
    // Earlier sprites sit in front.
    drawCall();
    for (const Pop& p : pops_) {
        if (p.t <= 0.f) continue;
        float sx, sy, scale;
        int fog;
        if (!project(p.x, p.s, sx, sy, scale, fog, 1.f)) continue;
        text(p.text, sx, sy - (1.15f - p.t) * 22.f, 0.55f, p.pal);
    }
    if (paper_.live) drawToss(std::clamp(paper_.t / paper_.dur, 0.f, 1.f), paper_.landS, paper_.landX);
    else if (mode_ == Mode::Title) {
        const float u = std::fmod(t_ * 0.55f, 1.f);
        drawToss(u, kBoxS[0], float(kBoxSide[0]) * BOX_LAT);
    }
    drawBike();
    drawWorld();
    spr(art_.sun, 262.f - x_ * 4.f + camX_ * 0.2f, 32.f + camY_ * 0.2f, 22.f, PAL_FX, false, 1, false, false);
    spr(art_.cloud, std::fmod(40.f + t_ * 6.f, 380.f) - 30.f, 26.f, 20.f, PAL_FX, false, 2, false, false);
    spr(art_.cloud, std::fmod(180.f + t_ * 4.f, 400.f) - 40.f, 46.f, 16.f, PAL_FX, true, 3, false, false);
    spr(art_.cloud, std::fmod(90.f + t_ * 3.f, 360.f) - 20.f, 18.f, 14.f, PAL_FX, false, 3, false, false);
    drawHud();
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    layout();
    sys.apu.setMaster(0.8f);
    sys.apu.setEcho(0.18f, 0.28f, 0.18f);
    sys.apu.setPatch(0, airPatch());
    sys.apu.setPatch(1, bellPatch());
    sys.apu.keyOn(0, 196.f, 0.06f);
    t_ = 0;
    if (bot_) resetRun();
    else enterTitle();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START)) resetRun();
        else if (pad.pressed(gs::BTN_MODE)) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Run;
        else if (pad.pressed(gs::BTN_MODE)) enterTitle();
    } else if (mode_ == Mode::Run) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_MODE))) {
            mode_ = Mode::Pause;
        } else {
            float steer = 0.f;
            bool fast = false, easy = false, toss = false;
            if (bot_) {
                steer = botSteer();
                toss = botToss();
            } else {
                if (pad.down(gs::BTN_LEFT)) steer -= 1.f;
                if (pad.down(gs::BTN_RIGHT)) steer += 1.f;
                if (std::fabs(pad.axisX) > 0.18f) steer = pad.axisX;
                fast = pad.down(gs::BTN_UP) || pad.accel > 0.25f;
                easy = pad.down(gs::BTN_DOWN) || pad.down(gs::BTN_B) || pad.brake > 0.25f;
                toss = pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_Z) ||
                       pad.pressed(gs::BTN_TURBO);
            }
            updateRun(steer, fast, easy, toss);
        }
    } else if (!bot_) {
        if (pad.pressed(gs::BTN_START)) resetRun();
        else if (pad.pressed(gs::BTN_MODE)) enterTitle();
    }
    audio(DT);
    draw();
}

}  // namespace mail

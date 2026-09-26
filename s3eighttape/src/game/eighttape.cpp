#include "game/eighttape.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace eighttape {
namespace {

constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;
constexpr float kDt = 1.f / 60.f;
constexpr float kVMax = 420.f;
constexpr float kMu = 210.f;
constexpr float kMakePow = 0.66f;
constexpr float kBallD = 64.f;
constexpr float kCueD = 112.f;
constexpr float kSpotLead = 64.f;
constexpr float kLock = 0.075f;
constexpr int kSub = 8;

float wrap(float a) {
    while (a > kPi) a -= kTau;
    while (a < -kPi) a += kTau;
    return a;
}

float segDist(float px, float py, float ax, float ay, float bx, float by) {
    float abx = bx - ax, aby = by - ay;
    float apx = px - ax, apy = py - ay;
    float ab2 = abx * abx + aby * aby;
    float t = ab2 < 1e-6f ? 0.f : (apx * abx + apy * aby) / ab2;
    t = std::clamp(t, 0.f, 1.f);
    return std::hypot(px - (ax + abx * t), py - (ay + aby * t));
}

void damp(float& vx, float& vy, float h) {
    float sp = std::hypot(vx, vy);
    if (sp < 1e-4f) {
        vx = vy = 0;
        return;
    }
    float drop = kMu * h;
    if (drop >= sp) {
        vx = vy = 0;
        return;
    }
    float s = (sp - drop) / sp;
    vx *= s;
    vy *= s;
}

void sink(Game::Body& b, int pocket) {
    Pocket pk = pocketAt(pocket);
    b.down = true;
    b.fell = true;
    b.pocket = pocket;
    b.vx = b.vy = 0;
    b.x = pk.x;
    b.y = pk.y;
}

bool inCloth(float x, float y) {
    if (x < kFeltL + kR + 1.f || x > kFeltR - kR - 1.f) return false;
    if (y < kFeltT + kR + 1.f || y > kFeltB - kR - 1.f) return false;
    for (int p = 0; p < 6; p++) {
        Pocket pk = pocketAt(p);
        if (std::hypot(x - pk.x, y - pk.y) < pk.sink + 2.f) return false;
    }
    return true;
}

bool pathBlocked(const Game::Body* b, float x0, float y0, float x1, float y1, int ignA, int ignB) {
    const float need = kR * 2.f - 0.25f;
    for (int i = 0; i < kBalls; i++) {
        if (i == ignA || i == ignB || b[i].down) continue;
        if (segDist(b[i].x, b[i].y, x0, y0, x1, y1) < need) return true;
    }
    return false;
}

void put(Game::Body& b, float x, float y, bool down = false) {
    b = Game::Body{};
    b.x = x;
    b.y = y;
    b.down = down;
}

void along(int pocket, float ballD, float cueD, float& bx, float& by, float& cx, float& cy) {
    Pocket p = pocketAt(pocket);
    float dx = 0, dy = 1.f;
    if (pocket == 4) {
        dy = -1.f;
    } else if (pocket != 1) {
        dx = 108.f - p.x;
        dy = 110.f - p.y;
        float L = std::hypot(dx, dy);
        if (L < 1.f) L = 1.f;
        dx /= L;
        dy /= L;
    }
    bx = p.x + dx * ballD;
    by = p.y + dy * ballD;
    cx = p.x + dx * cueD;
    cy = p.y + dy * cueD;
}

void lead(const Game::Body& cue, int pocket, float dist, float& x, float& y) {
    Pocket p = pocketAt(pocket);
    float dx = p.x - cue.x, dy = p.y - cue.y;
    float L = std::hypot(dx, dy);
    if (L < 1.f) L = 1.f;
    x = cue.x + dx / L * dist;
    y = cue.y + dy / L * dist;
}

void layout(int n, Game::Body* b) {
    for (int i = 0; i < kBalls; i++) b[i] = Game::Body{};
    float bx, by, cx, cy;
    if (n <= 0) {
        along(0, kBallD, kCueD, bx, by, cx, cy);
        put(b[1], bx, by);
        put(b[0], cx, cy);
        float sx, sy;
        lead(b[0], kSpotPocket, kSpotLead, sx, sy);
        put(b[kSpotBall], sx, sy);
        put(b[2], 36.f, 158.f);
        put(b[3], 170.f, 152.f);
        return;
    }
    if (n == 1) {
        along(1, kBallD, kCueD, bx, by, cx, cy);
        put(b[2], bx, by);
        put(b[0], cx, cy);
        put(b[1], 0, 0, true);
        put(b[3], 170.f, 150.f);
        put(b[kSpotBall], 36.f, 64.f);
        return;
    }
    along(4, kBallD, kCueD, bx, by, cx, cy);
    put(b[3], bx, by);
    put(b[0], cx, cy);
    put(b[1], 0, 0, true);
    put(b[2], 0, 0, true);
    put(b[kSpotBall], 36.f, 150.f);
}

Game::Lock examine(const Game::Body* b, float aim) {
    Game::Lock L;
    if (b[0].down) return L;
    int best = -1;
    float bestD = 1e9f;
    for (int i = 1; i < kBalls; i++) {
        if (b[i].down) continue;
        float dx = b[i].x - b[0].x, dy = b[i].y - b[0].y;
        float d = std::hypot(dx, dy);
        if (d < 8.f) continue;
        float ang = std::atan2(dy, dx);
        if (std::fabs(wrap(ang - aim)) > kLock) continue;
        if (d < bestD) {
            bestD = d;
            best = i;
        }
    }
    if (best < 0) return L;
    int home = homePocket(best);
    if (home < 0) return L;
    Pocket pk = pocketAt(home);
    float dx = pk.x - b[best].x, dy = pk.y - b[best].y;
    float d = std::hypot(dx, dy);
    if (d < 10.f) return L;
    float ang = std::atan2(dy, dx);
    float ax = std::cos(aim), ay = std::sin(aim);
    if (dx * ax + dy * ay <= 0.f) return L;
    if (std::fabs(wrap(ang - aim)) > 0.16f) return L;
    L.ball = best;
    L.pocket = home;
    return L;
}

void stepOpen(Game::Sim& s, float h) {
    const float minX = kFeltL + kR, maxX = kFeltR - kR;
    const float minY = kFeltT + kR, maxY = kFeltB - kR;
    for (int i = 0; i < kBalls; i++) {
        Game::Body& ball = s.b[i];
        if (ball.down) continue;
        for (int p = 0; p < 6; p++) {
            Pocket pk = pocketAt(p);
            float dx = pk.x - ball.x, dy = pk.y - ball.y;
            float d = std::hypot(dx, dy);
            if (d < pk.sink + 3.f && d > 0.6f) {
                float nx = dx / d, ny = dy / d;
                if (ball.vx * nx + ball.vy * ny > 36.f) {
                    ball.vx += nx * 70.f * h;
                    ball.vy += ny * 70.f * h;
                }
            }
        }
        ball.x += ball.vx * h;
        ball.y += ball.vy * h;
        damp(ball.vx, ball.vy, h);
    }
    for (int pass = 0; pass < 3; pass++) {
        for (int i = 0; i < kBalls; i++) {
            if (s.b[i].down) continue;
            for (int j = i + 1; j < kBalls; j++) {
                if (s.b[j].down) continue;
                float dx = s.b[j].x - s.b[i].x;
                float dy = s.b[j].y - s.b[i].y;
                float d2 = dx * dx + dy * dy;
                const float minD = kR * 2.f;
                if (d2 >= minD * minD || d2 < 1e-8f) continue;
                float d = std::sqrt(d2);
                float nx = dx / d, ny = dy / d;
                float rel = (s.b[i].vx - s.b[j].vx) * nx + (s.b[i].vy - s.b[j].vy) * ny;
                if (rel > 0.f) {
                    s.b[i].vx -= rel * nx;
                    s.b[i].vy -= rel * ny;
                    s.b[j].vx += rel * nx;
                    s.b[j].vy += rel * ny;
                }
                float push = (minD - d) * 0.5f;
                s.b[i].x -= nx * push;
                s.b[i].y -= ny * push;
                s.b[j].x += nx * push;
                s.b[j].y += ny * push;
            }
        }
    }
    auto mouths = [&]() {
        for (int i = 0; i < kBalls; i++) {
            if (s.b[i].down) continue;
            int best = -1;
            float bd = 1e9f;
            for (int p = 0; p < 6; p++) {
                float d = std::hypot(s.b[i].x - pocketAt(p).x, s.b[i].y - pocketAt(p).y);
                if (d < pocketAt(p).sink && d < bd) {
                    bd = d;
                    best = p;
                }
            }
            if (best >= 0) sink(s.b[i], best);
        }
    };
    mouths();
    for (int i = 0; i < kBalls; i++) {
        Game::Body& ball = s.b[i];
        if (ball.down) continue;
        if (ball.x < minX) {
            ball.x = minX;
            if (ball.vx < 0) ball.vx = -ball.vx * 0.55f;
        } else if (ball.x > maxX) {
            ball.x = maxX;
            if (ball.vx > 0) ball.vx = -ball.vx * 0.55f;
        }
        if (ball.y < minY) {
            ball.y = minY;
            if (ball.vy < 0) ball.vy = -ball.vy * 0.55f;
        } else if (ball.y > maxY) {
            ball.y = maxY;
            if (ball.vy > 0) ball.vy = -ball.vy * 0.55f;
        }
    }
    mouths();
}

void stepSim(Game::Sim& s, float dt) {
    const float h = dt / float(kSub);
    for (int sub = 0; sub < kSub; sub++) {
        if (!(s.guide && s.gball > 0)) {
            stepOpen(s, h);
            continue;
        }
        Game::Body& cue = s.b[0];
        Game::Body& obj = s.b[s.gball];
        if (!s.struck) {
            if (!cue.down) {
                damp(cue.vx, cue.vy, h);
                cue.x += cue.vx * h;
                cue.y += cue.vy * h;
            }
            if (!obj.down && !cue.down) {
                float dx = obj.x - cue.x, dy = obj.y - cue.y;
                float d = std::hypot(dx, dy);
                float sp = std::hypot(cue.vx, cue.vy);
                bool closing = dx * cue.vx + dy * cue.vy > 0.f;
                if (closing && d < kR * 2.f + 0.25f && d > 0.05f && sp > 24.f) {
                    float nx = dx / d, ny = dy / d;
                    cue.x = obj.x - nx * (kR * 2.f + 0.45f);
                    cue.y = obj.y - ny * (kR * 2.f + 0.45f);
                    cue.vx = cue.vy = 0;
                    Pocket pk = pocketAt(s.pocket);
                    float px = pk.x - obj.x, py = pk.y - obj.y;
                    float pl = std::hypot(px, py);
                    if (pl < 0.01f) pl = 0.01f;
                    s.ux = px / pl;
                    s.uy = py / pl;
                    obj.vx = s.ux * sp * 0.96f;
                    obj.vy = s.uy * sp * 0.96f;
                    s.struck = true;
                }
            }
            continue;
        }
        if (obj.down) continue;
        damp(obj.vx, obj.vy, h);
        obj.x += obj.vx * h;
        obj.y += obj.vy * h;
        Pocket pk = pocketAt(s.pocket);
        float tx = pk.x - obj.x, ty = pk.y - obj.y;
        float alongDot = tx * s.ux + ty * s.uy;
        if (alongDot <= 0.f || std::hypot(tx, ty) < pk.sink) sink(obj, s.pocket);
    }
}

bool resting(const Game::Sim& s) {
    for (int i = 0; i < kBalls; i++) {
        if (s.b[i].down) continue;
        if (std::hypot(s.b[i].vx, s.b[i].vy) > 18.f) return false;
    }
    if (s.guide && s.struck && s.gball > 0 && !s.b[s.gball].down) {
        Pocket pk = pocketAt(s.pocket);
        float d = std::hypot(s.b[s.gball].x - pk.x, s.b[s.gball].y - pk.y);
        float sp = std::hypot(s.b[s.gball].vx, s.b[s.gball].vy);
        if (sp > 4.f && d < pk.sink + 14.f) return false;
    }
    return true;
}

void absorb(Game::Sim& s) {
    for (int i = 0; i < kBalls; i++) {
        if (s.b[i].down) continue;
        s.b[i].vx = s.b[i].vy = 0;
        int best = -1;
        float bd = 1e9f;
        for (int p = 0; p < 6; p++) {
            float d = std::hypot(s.b[i].x - pocketAt(p).x, s.b[i].y - pocketAt(p).y);
            if (d < pocketAt(p).sink && d < bd) {
                bd = d;
                best = p;
            }
        }
        if (best >= 0) sink(s.b[i], best);
    }
}

void setupShot(Game::Sim& s, float aim, float power) {
    for (int i = 0; i < kBalls; i++) {
        s.b[i].vx = s.b[i].vy = 0;
        s.b[i].fell = false;
        s.b[i].pocket = -1;
    }
    s.guide = false;
    s.struck = false;
    s.gball = -1;
    s.pocket = -1;
    s.ux = 1;
    s.uy = 0;
    Game::Lock L = examine(s.b, aim);
    if (L.ball > 0 && L.pocket >= 0) {
        s.guide = true;
        s.gball = L.ball;
        s.pocket = L.pocket;
    }
    float sp = std::clamp(power, 0.08f, 1.f) * kVMax;
    s.b[0].vx = std::cos(aim) * sp;
    s.b[0].vy = std::sin(aim) * sp;
}

void roll(Game::Sim& s) {
    for (int f = 0; f < 240; f++) {
        stepSim(s, kDt);
        if (resting(s)) break;
    }
    absorb(s);
}

Game::Verdict verdict(const Game::Body* b, int call) {
    Game::Verdict v;
    Line L = tapeAt(call);
    v.scratch = b[0].fell;
    v.spot = b[kSpotBall].fell && b[kSpotBall].pocket == kSpotPocket;
    bool target = b[L.ball].fell && b[L.ball].pocket == L.pocket;
    v.off = b[L.ball].fell && b[L.ball].pocket != L.pocket;
    for (int i = 1; i < kBalls; i++) {
        if (i == L.ball) continue;
        if (b[i].fell) v.other = true;
    }
    v.made = target && !v.scratch && !v.other;
    return v;
}

bool clothLayout(int n, const Game::Body* b) {
    for (int i = 0; i < kBalls; i++) {
        if (b[i].down) continue;
        if (!inCloth(b[i].x, b[i].y)) {
            std::fprintf(stderr, "s3eighttape layout %d ball %d off the cloth at %.1f %.1f\n", n, i, b[i].x, b[i].y);
            return false;
        }
        for (int j = i + 1; j < kBalls; j++) {
            if (b[j].down) continue;
            if (std::hypot(b[i].x - b[j].x, b[i].y - b[j].y) < kR * 2.f + 1.f) {
                std::fprintf(stderr, "s3eighttape layout %d balls %d and %d overlap\n", n, i, j);
                return false;
            }
        }
    }
    return true;
}

bool lineClear(int n, const Game::Body* b, int ball, int pocket) {
    if (pathBlocked(b, b[0].x, b[0].y, b[ball].x, b[ball].y, 0, ball)) {
        std::fprintf(stderr, "s3eighttape layout %d cue path to ball %d is blocked\n", n, ball);
        return false;
    }
    Pocket pk = pocketAt(pocket);
    if (pathBlocked(b, b[ball].x, b[ball].y, pk.x, pk.y, ball, 0)) {
        std::fprintf(stderr, "s3eighttape layout %d ball %d path to %s is blocked\n", n, ball, pk.name);
        return false;
    }
    return true;
}

float aimAt(const Game::Body* b, int ball) {
    return std::atan2(b[ball].y - b[0].y, b[ball].x - b[0].x);
}

}  // namespace

const char* Game::tapeLabel(int i) const {
    if (i < 0 || i >= kTapeN) return "";
    return kTape[i].name;
}

int Game::tapeScore(int i) const {
    if (i < 0 || i >= kTapeN) return 0;
    return kTape[i].pay;
}

int Game::drawerScore() const {
    int s = 0;
    for (int i = 0; i < kTapeN; i++)
        if (held_[i]) s += kTape[i].pay;
    return s;
}

int Game::phase() const {
    if (mode_ == Mode::Title || mode_ == Mode::Pause) return 0;
    if (mode_ == Mode::Aim) return 1;
    if (mode_ == Mode::Roll) return 2;
    if (mode_ == Mode::Pocket || mode_ == Mode::Leave || (mode_ == Mode::Over && won_)) return 3;
    return 4;
}

int Game::nextOpen() const {
    for (int i = 0; i < kTapeN; i++)
        if (!held_[i]) return i;
    return 0;
}

bool Game::prove() {
    int tape = 0;
    for (int i = 0; i < kTapeN; i++) tape += kTape[i].pay;
    int trap = kSpotPay + kTape[1].pay + kTape[2].pay;
    if (tape != 16 || trap != 16 || kSpotPay != kTape[0].pay || kTape[0].ball == kSpotBall) {
        std::fprintf(stderr, "s3eighttape tape arithmetic tape %d trap %d\n", tape, trap);
        return false;
    }
    for (int n = 0; n < kTapeN; n++) {
        Sim s;
        layout(n, s.b);
        if (!clothLayout(n, s.b)) return false;
        Line L = tapeAt(n);
        if (!lineClear(n, s.b, L.ball, L.pocket)) return false;
        float aim = aimAt(s.b, L.ball);
        Lock lock = examine(s.b, aim);
        if (lock.ball != L.ball || lock.pocket != L.pocket) {
            std::fprintf(stderr, "s3eighttape %s does not lock (ball %d pocket %d)\n", L.name, lock.ball, lock.pocket);
            return false;
        }
        solvedAim_[n] = aim;
        setupShot(s, aim, kMakePow);
        roll(s);
        Verdict v = verdict(s.b, n);
        if (!v.made) {
            std::fprintf(stderr, "s3eighttape %s did not fall  pocket %d at %.1f %.1f  cue %d struck %d\n", L.name,
                         s.b[L.ball].pocket, s.b[L.ball].x, s.b[L.ball].y, s.b[0].fell ? 1 : 0, s.struck ? 1 : 0);
            return false;
        }
        Sim soft;
        layout(n, soft.b);
        setupShot(soft, aim, 0.24f);
        roll(soft);
        if (verdict(soft.b, n).made) {
            std::fprintf(stderr, "s3eighttape a soft %s still filled the drawer\n", L.name);
            return false;
        }
    }
    Sim decoy;
    layout(0, decoy.b);
    if (!lineClear(0, decoy.b, kSpotBall, kSpotPocket)) return false;
    float spotAim = aimAt(decoy.b, kSpotBall);
    Lock spotLock = examine(decoy.b, spotAim);
    if (spotLock.ball != kSpotBall || spotLock.pocket != kSpotPocket) {
        std::fprintf(stderr, "s3eighttape spot does not lock (ball %d pocket %d)\n", spotLock.ball, spotLock.pocket);
        return false;
    }
    setupShot(decoy, spotAim, kMakePow);
    roll(decoy);
    Verdict tv = verdict(decoy.b, 0);
    if (tv.made || !tv.spot || decoy.b[1].fell) {
        std::fprintf(stderr, "s3eighttape spot shot made %d spot %d eight %d\n", tv.made ? 1 : 0, tv.spot ? 1 : 0,
                     decoy.b[1].fell ? 1 : 0);
        return false;
    }
    Sim wild;
    layout(0, wild.b);
    setupShot(wild, solvedAim_[0] + 0.85f, kMakePow);
    if (wild.guide && wild.pocket == kTape[0].pocket) {
        std::fprintf(stderr, "s3eighttape a wide aim still called the corner\n");
        return false;
    }
    roll(wild);
    if (verdict(wild.b, 0).made) {
        std::fprintf(stderr, "s3eighttape a wide stroke still pocketed the eight\n");
        return false;
    }
    solved_ = true;
    return true;
}

void Game::toTitle() {
    layout(0, sim_.b);
    sim_.guide = false;
    sim_.struck = false;
    for (int i = 0; i < kTapeN; i++) held_[i] = false;
    strokes_ = 0;
    traps_ = 0;
    call_ = 0;
    fly_ = -1;
    shake_ = 0;
    fanStep_ = -1;
    won_ = false;
    over_ = false;
    left_ = false;
    walk_ = 0;
    aimT_ = rollT_ = flyT_ = judgeT_ = leaveT_ = fanT_ = 0;
    aim_ = solved_ ? solvedAim_[0] : 0;
    power_ = kMakePow;
    reason_ = "OPEN";
    mode_ = Mode::Title;
}

void Game::begin() {
    if (!rules_) return;
    for (int i = 0; i < kTapeN; i++) held_[i] = false;
    strokes_ = 0;
    traps_ = 0;
    fly_ = -1;
    won_ = false;
    over_ = false;
    left_ = false;
    walk_ = 0;
    beginAim();
}

void Game::beginAim() {
    call_ = nextOpen();
    layout(call_, sim_.b);
    sim_.guide = false;
    sim_.struck = false;
    aim_ = solved_ ? solvedAim_[call_] : 0;
    power_ = kMakePow;
    if (!bot_) {
        aim_ = wrap(aim_ + 0.36f);
        power_ = 0.4f;
    }
    aimT_ = 0;
    fly_ = -1;
    reason_ = "AIM";
    mode_ = Mode::Aim;
}

void Game::shoot() {
    if (mode_ != Mode::Aim || sim_.b[0].down) return;
    setupShot(sim_, aim_, power_);
    strokes_++;
    rollT_ = 0;
    mode_ = Mode::Roll;
    reason_ = "ROLL";
    if (!sys_) return;
    sys_->apu.noiseBurst(0.2f, 1900.f, 0.04f);
    sys_->rumble(0.08f, 0.2f, 28);
}

void Game::take(int line) {
    if (line < 0 || line >= kTapeN || held_[line]) return;
    held_[line] = true;
    fly_ = line;
    flyT_ = 0;
    mode_ = Mode::Pocket;
    reason_ = "IN THE DRAWER";
    blip(520.f + float(line) * 70.f, 0.07f, 0.16f);
    if (!sys_) return;
    sys_->rumble(0.2f, 0.45f, 80);
    sys_->setLight(80, 180, 70);
}

void Game::miss(const char* why) {
    reason_ = why && why[0] ? why : "SHORT";
    if (std::strcmp(reason_, "SPOT IS NOT THE EIGHT") == 0) traps_++;
    shake_ = 8;
    blip(96.f, 0.07f, 0.16f);
    if (sys_) {
        sys_->rumble(0.3f, 0.08f, 70);
        sys_->setLight(160, 36, 28);
    }
    if (strokes_ >= kMaxStrokes) {
        beginLose();
        return;
    }
    judgeT_ = 0;
    mode_ = Mode::Judge;
}

void Game::finishRoll() {
    absorb(sim_);
    Verdict v = verdict(sim_.b, call_);
    if (v.made) {
        take(call_);
        return;
    }
    const char* why = "SHORT";
    if (v.spot && call_ == 0) why = "SPOT IS NOT THE EIGHT";
    else if (v.scratch) why = "SCRATCH";
    else if (v.off) why = "NOT THE POCKET";
    else if (v.other) why = "NOT THE CALL";
    miss(why);
}

void Game::beginLeave() {
    if (!matched()) return;
    mode_ = Mode::Leave;
    leaveT_ = 0;
    walk_ = 0;
    fanT_ = 0;
    fanStep_ = -1;
    left_ = true;
    reason_ = "THE DRAWER MATCHES THE TAPE";
    blip(523.f, 0.07f, 0.2f);
    if (!sys_) return;
    sys_->rumble(0.25f, 0.6f, 160);
    sys_->setLight(255, 200, 80);
}

void Game::beginLose() {
    mode_ = Mode::Lose;
    won_ = false;
    over_ = true;
    if (!reason_ || !reason_[0]) reason_ = "DOES NOT MATCH";
    if (bot_) std::fprintf(stderr, "s3eighttape %s\n", reason_);
    if (sys_) sys_->setLight(150, 28, 28);
}

void Game::blip(float freq, float vol, float hold) {
    if (!sys_) return;
    sys_->apu.tone(0, freq, vol);
    toneT_ = hold;
}

void Game::fanfare() {
    if (!sys_) return;
    static const float notes[] = {523.25f, 659.25f, 783.99f, 1046.5f};
    int step = int(fanT_ / 0.14f);
    if (step != fanStep_ && step >= 0 && step < 4) {
        sys_->apu.tone(0, notes[step], 0.09f);
        fanStep_ = step;
    } else if (step >= 8 && fanStep_ != 99) {
        sys_->apu.tone(0, 0, 0);
        fanStep_ = 99;
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = true;
    sys.vdp.B.enabled = false;
    sys.apu.setMaster(0.72f);
    sys.apu.setEcho(0.1f, 0.18f, 0.1f);
    rules_ = prove();
    toTitle();
    if (!rules_) {
        won_ = false;
        over_ = true;
        mode_ = Mode::Lose;
        reason_ = "LAYOUT";
        std::fprintf(stderr, "s3eighttape rules failed\n");
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    clock_ += kDt;
    if (toneT_ > 0.f && mode_ != Mode::Leave && !(mode_ == Mode::Over && won_)) {
        toneT_ -= kDt;
        if (toneT_ <= 0.f) {
            sys.apu.tone(0, 0, 0);
            sys.apu.tone(1, 0, 0);
        }
    }
    if (shake_ > 0) shake_--;
    if (mode_ == Mode::Leave || (mode_ == Mode::Over && won_)) {
        fanT_ += kDt;
        fanfare();
    }

    if (!rules_) {
        won_ = false;
        over_ = true;
        mode_ = Mode::Lose;
        reason_ = "LAYOUT";
        draw();
        return;
    }

    bool start = false, back = false, action = false, fine = false;
    float x = 0, y = 0;
    if (bot_) {
        if (mode_ == Mode::Title && clock_ > 0.45f) start = true;
    } else {
        const gs::Pad& pad = sys.pad;
        start = pad.pressed(gs::BTN_START);
        back = pad.pressed(gs::BTN_MODE);
        action = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_TURBO);
        fine = pad.down(gs::BTN_Y) || pad.down(gs::BTN_Z);
        if (pad.down(gs::BTN_LEFT)) x -= 1.f;
        if (pad.down(gs::BTN_RIGHT)) x += 1.f;
        if (pad.down(gs::BTN_UP)) y -= 1.f;
        if (pad.down(gs::BTN_DOWN)) y += 1.f;
        if (std::fabs(pad.axisX) > 0.2f) x = pad.axisX;
        if (std::fabs(pad.axisY) > 0.2f) y = -pad.axisY;
    }

    if (mode_ == Mode::Title) {
        if (back && !bot_) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        } else if (start || action) begin();
    } else if (mode_ == Mode::Pause) {
        if (start || action) mode_ = heldMode_;
        else if (back && !bot_) toTitle();
    } else if (mode_ == Mode::Over || mode_ == Mode::Lose) {
        if (!bot_ && (start || action)) begin();
        else if (!bot_ && back) toTitle();
    } else if (!bot_ && start && mode_ == Mode::Aim) {
        heldMode_ = mode_;
        mode_ = Mode::Pause;
    } else if (!bot_ && back && mode_ != Mode::Roll) {
        toTitle();
    } else if (mode_ == Mode::Aim) {
        aimT_ += kDt;
        if (bot_) {
            aim_ = solved_ ? solvedAim_[call_] : aim_;
            power_ = kMakePow;
            if (aimT_ > 0.28f) shoot();
        } else {
            float rate = fine ? 0.32f : 1.15f;
            aim_ = wrap(aim_ + x * rate * kDt);
            float pRate = fine ? 0.18f : 0.5f;
            power_ = std::clamp(power_ - y * pRate * kDt, 0.16f, 0.92f);
            if (action) shoot();
        }
    } else if (mode_ == Mode::Roll) {
        rollT_ += kDt;
        bool before = false;
        for (int i = 0; i < kBalls; i++)
            if (sim_.b[i].fell) before = true;
        stepSim(sim_, kDt);
        bool after = false;
        for (int i = 0; i < kBalls; i++)
            if (sim_.b[i].fell) after = true;
        if (after && !before) blip(180.f, 0.06f, 0.08f);
        if (rollT_ > 4.2f || resting(sim_)) finishRoll();
    } else if (mode_ == Mode::Pocket) {
        flyT_ += kDt;
        int n = int(flyT_ * 60.f);
        if (n == 8) blip(659.f, 0.05f, 0.1f);
        if (n == 16) blip(784.f, 0.05f, 0.1f);
        if (flyT_ > 0.46f) {
            fly_ = -1;
            if (matched()) beginLeave();
            else if (strokes_ >= kMaxStrokes) beginLose();
            else beginAim();
        }
    } else if (mode_ == Mode::Judge) {
        judgeT_ += kDt;
        if (judgeT_ > 0.55f) {
            if (strokes_ >= kMaxStrokes) beginLose();
            else beginAim();
        }
    } else if (mode_ == Mode::Leave) {
        leaveT_ += kDt;
        walk_ += 86.f * kDt;
        if (leaveT_ > 1.05f) {
            won_ = matched() && left_ && strokes_ >= 3 && strokes_ <= kMaxStrokes && drawerScore() == 16 && rules_;
            over_ = true;
            mode_ = Mode::Over;
            reason_ = won_ ? "LEAVE" : "DOES NOT MATCH";
            if (won_ && sys_) sys_->setLight(255, 210, 90);
        }
    }

    draw();
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow) {
    if (!sys_ || h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s{};
    s.w = int16_t(std::max(1, std::min(2000, int(std::lround(w)))));
    s.h = int16_t(std::max(1, std::min(2000, int(std::lround(h)))));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 32 || s.y > gs::SCREEN_H + 32 || s.x + s.w < -32 || s.y + s.h < -32) return;
    s.img = m.pick(float(s.h));
    s.pal = uint8_t(pal);
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!sys_ || !s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        int tile = art_.font[c - 32];
        if (!tile) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(tile, pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::backdrop() {
    if (!sys_) return;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        bool nap = ((y / 3) & 1) == 0;
        sys_->vdp.lineBackdrop[y] = nap ? gs::rgb4(0, 6, 2) : gs::rgb4(0, 8, 3);
        sys_->vdp.lineFog[y] = 0;
        sys_->vdp.road[y].on = false;
    }
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = true;
    v.B.enabled = false;
    backdrop();
    float jx = (shake_ > 0 && (int(clock_ * 60.f) & 1)) ? 1.5f : 0.f;

    spr(art_.lamp, 108.f, 18.f, 16.f, PAL_LAMP);
    int call = mode_ == Mode::Title ? 0 : call_;
    if (call >= 0 && call < kTapeN) {
        Pocket pk = pocketAt(kTape[call].pocket);
        float pulse = 16.f + std::sin(clock_ * 5.f) * 1.2f;
        spr(art_.ring, pk.x, pk.y, pulse, PAL_GLOW);
    }
    if (call == 0 && mode_ != Mode::Leave && mode_ != Mode::Over) {
        Pocket wide = pocketAt(kSpotPocket);
        spr(art_.ring, wide.x, wide.y, 14.f, PAL_ALERT);
    }
    for (int i = 0; i < kTapeN; i++) spr(art_.slot, kSlotX0 + float(i) * kSlotPitch, kSlotY, 12.f, PAL_CUE);

    auto slipPal = [](int i) {
        if (i == 1) return PAL_SOLID;
        if (i == 2) return PAL_STRIPE;
        return PAL_GOLD;
    };
    for (int i = 0; i < kTapeN; i++) {
        if (fly_ == i) continue;
        if (!held_[i]) continue;
        spr(art_.slip, kSlotX0 + float(i) * kSlotPitch + jx, kSlotY - 2.f, 20.f, slipPal(i));
    }
    if (fly_ >= 0 && fly_ < kTapeN) {
        Pocket pk = pocketAt(kTape[fly_].pocket);
        float u = std::clamp(flyT_ / 0.46f, 0.f, 1.f);
        float x1 = kSlotX0 + float(fly_) * kSlotPitch;
        float x = pk.x + (x1 - pk.x) * u;
        float y = pk.y + (kSlotY - pk.y) * u - std::sin(u * kPi) * 22.f;
        spr(art_.slip, x, y, 18.f + (1.f - u) * 4.f, slipPal(fly_));
    }

    bool showCue = mode_ == Mode::Title || mode_ == Mode::Aim || mode_ == Mode::Pause || mode_ == Mode::Judge;
    if (showCue && !sim_.b[0].down) {
        float qaim = aim_;
        int qi = int(std::lround(qaim / kTau * float(kCueAngles)));
        qi %= kCueAngles;
        if (qi < 0) qi += kCueAngles;
        float q = float(qi) * (kTau / float(kCueAngles));
        float back = kTip + kR + 3.f + std::max(0.f, power_ - 0.2f) * 14.f;
        float cx = sim_.b[0].x - std::cos(q) * back;
        float cy = sim_.b[0].y - std::sin(q) * back;
        spr(art_.cue[qi], cx + jx, cy, float(kCueBox) * 0.9f, PAL_CUE);
    }

    if (showCue && !sim_.b[0].down) {
        Lock L = examine(sim_.b, aim_);
        float dx = std::cos(aim_), dy = std::sin(aim_);
        int pal = PAL_AIM;
        if (L.ball == kSpotBall) pal = PAL_ALERT;
        else if (L.ball > 0 && L.ball == tapeAt(call).ball) pal = power_ >= 0.58f ? PAL_WIN : PAL_GOLD;
        for (int i = 1; i <= 7; i++) {
            float t = float(i) * 11.f;
            spr(art_.dot, sim_.b[0].x + dx * t, sim_.b[0].y + dy * t, 4.f, pal);
        }
        if (L.ball > 0) {
            Pocket pk = pocketAt(L.pocket);
            float ox = sim_.b[L.ball].x, oy = sim_.b[L.ball].y;
            float lx = pk.x - ox, ly = pk.y - oy;
            float ld = std::hypot(lx, ly);
            if (ld > 1.f) {
                lx /= ld;
                ly /= ld;
                int n = std::min(6, int(ld / 12.f));
                for (int i = 1; i <= n; i++) spr(art_.dot, ox + lx * float(i) * 12.f, oy + ly * float(i) * 12.f, 4.f, pal);
            }
        }
    }

    for (int i = 0; i < kBalls; i++) {
        if (sim_.b[i].down) continue;
        spr(art_.shadow, sim_.b[i].x + 2.f + jx, sim_.b[i].y + 3.f, 7.f, PAL_SHADE, true);
    }
    for (int i = 0; i < kBalls; i++) {
        if (sim_.b[i].down) continue;
        float bob = (i == tapeAt(call).ball && (mode_ == Mode::Aim || mode_ == Mode::Title)) ? std::sin(clock_ * 4.f) * 0.6f : 0.f;
        spr(art_.ball[i], sim_.b[i].x + jx, sim_.b[i].y + bob, 15.f, PAL_BALL);
    }
    for (int i = 0; i < kBalls; i++) {
        if (!sim_.b[i].down || !sim_.b[i].fell) continue;
        if (fly_ >= 0 && i == kTape[fly_].ball) continue;
        spr(art_.ball[i], sim_.b[i].x, sim_.b[i].y, 11.f, PAL_BALL);
    }

    bool showPlayer = mode_ != Mode::Title;
    if (showPlayer) {
        float px = 28.f + walk_;
        float py = 198.f;
        int step = int(walk_ / 8.f) & 1;
        if (mode_ != Mode::Leave && !(mode_ == Mode::Over && won_)) step = int(clock_ * 2.f) & 1;
        if (px < 360.f) spr(art_.player[step], px, py, 24.f, PAL_PLAYER);
    }

    char buf[48];
    hud(1, 0, "S3 EIGHTTAPE", PAL_GOLD);
    hud(27, 2, "TAPE", PAL_GOLD);
    for (int i = 0; i < kTapeN; i++) {
        char mark = ' ';
        int pal = PAL_HUD;
        if (held_[i]) {
            mark = '+';
            pal = PAL_WIN;
        } else if (i == call && mode_ != Mode::Title && mode_ != Mode::Over) {
            mark = '>';
            pal = PAL_GOLD;
        }
        std::snprintf(buf, sizeof buf, "%c %-6s %d", mark, kTape[i].name, kTape[i].pay);
        hud(26, 4 + i * 2, buf, pal);
    }
    hud(26, 10, "SPOT     8", PAL_ALERT);
    hud(26, 11, "STAYS OUT", PAL_ALERT);

    std::snprintf(buf, sizeof buf, "TILL %d", drawerScore());
    hud(1, 1, buf, matched() ? PAL_WIN : PAL_GOLD);
    hud(10, 1, "TAPE 16", PAL_HUD);

    if (mode_ == Mode::Title) {
        hud(22, 0, S3_VERSION_STRING, PAL_HUD);
        hudC(24, "PLAY EIGHT UNTIL THE DRAWER", PAL_GOLD);
        hudC(25, "HAS TO MATCH THE TAPE", PAL_GOLD);
        hudC(26, "A CLOSE EIGHT IS STILL OPEN", PAL_HUD);
        if ((int(clock_ * 2.f) & 1) == 0) hudC(27, "RETURN STARTS", PAL_WIN);
        else hudC(27, "ARROWS AIM   Z SHOOTS", PAL_HUD);
        return;
    }

    int shown = strokes_ + (mode_ == Mode::Aim ? 1 : 0);
    if (shown < 1) shown = 1;
    if (shown > kMaxStrokes) shown = kMaxStrokes;
    std::snprintf(buf, sizeof buf, "STROKE %d/%d", shown, kMaxStrokes);
    hud(22, 0, buf, strokes_ >= kMaxStrokes - 1 ? PAL_ALERT : PAL_HUD);

    if (mode_ == Mode::Pause) {
        hudC(26, "PAUSED", PAL_GOLD);
        hudC(27, "RETURN RESUMES", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Over && won_) {
        hudC(26, "THE DRAWER MATCHES THE TAPE", PAL_WIN);
        hudC(27, "YOU LEAVE", PAL_GOLD);
        return;
    }
    if (mode_ == Mode::Over || mode_ == Mode::Lose) {
        hudC(26, "THE DRAWER DOES NOT MATCH", PAL_ALERT);
        hudC(27, reason_ && reason_[0] ? reason_ : "DOES NOT MATCH", PAL_ALERT);
        return;
    }
    if (mode_ == Mode::Leave) {
        hudC(26, "THE DRAWER MATCHES THE TAPE", PAL_WIN);
        hudC(27, "LEAVE", PAL_GOLD);
        return;
    }
    if (mode_ == Mode::Pocket) {
        hudC(26, "IN THE DRAWER", PAL_WIN);
        hudC(27, matched() ? "THE TAPE IS MET" : "NEXT CALL", PAL_GOLD);
        return;
    }
    if (mode_ == Mode::Judge) {
        hudC(26, reason_ ? reason_ : "SHORT", PAL_ALERT);
        if (reason_ && std::strcmp(reason_, "SPOT IS NOT THE EIGHT") == 0) hudC(27, "A CLOSE EIGHT IS STILL OPEN", PAL_GOLD);
        else hudC(27, "NOT IN THE DRAWER", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Roll) {
        hudC(27, "ROLLING", PAL_GOLD);
        return;
    }

    Line L = tapeAt(call_);
    std::snprintf(buf, sizeof buf, "%s INTO %s", L.name, pocketAt(L.pocket).name);
    hudC(26, buf, PAL_GOLD);
    Lock Lck = examine(sim_.b, aim_);
    if (Lck.ball == kSpotBall) {
        hudC(27, "SPOT IS NOT THE EIGHT", PAL_ALERT);
        return;
    }
    int bars = std::clamp(int(std::lround(power_ * 8.f)), 1, 8);
    bool firm = power_ >= 0.58f && Lck.ball == L.ball;
    const char* zone = power_ < 0.58f ? "SOFT " : "FIRM ";
    char bar[24];
    int p = 0;
    for (int i = 0; zone[i] && p < 20; i++) bar[p++] = zone[i];
    for (int i = 0; i < 8 && p < 23; i++) bar[p++] = i < bars ? '#' : '.';
    bar[p] = 0;
    hud(1, 27, bar, firm ? PAL_WIN : (power_ < 0.58f ? PAL_ALERT : PAL_GOLD));
    hud(30, 27, "Z SHOOT", PAL_HUD);
}

}  // namespace eighttape

#include "game/eightgold.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace eightgold {
namespace {

constexpr float kPi = 3.14159265f;
constexpr float kMu = 240.f;
constexpr float kVMax = 760.f;
constexpr float kRest = 0.72f;
constexpr int kSub = 8;
constexpr float kDt = 1.f / 60.f;

bool tracing() {
    const char* e = std::getenv("EIGHTGOLD_TRACE");
    return e && e[0] && e[0] != '0';
}

float len(float x, float y) { return std::sqrt(x * x + y * y); }

float distPointSeg(float px, float py, float ax, float ay, float bx, float by) {
    float abx = bx - ax, aby = by - ay;
    float apx = px - ax, apy = py - ay;
    float ab2 = abx * abx + aby * aby;
    float t = ab2 < 1e-6f ? 0.f : (apx * abx + apy * aby) / ab2;
    t = std::clamp(t, 0.f, 1.f);
    float dx = px - (ax + abx * t), dy = py - (ay + aby * t);
    return std::sqrt(dx * dx + dy * dy);
}

void damp(float& vx, float& vy, float h) {
    float sp = len(vx, vy);
    if (sp < 1e-3f) {
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

void capSpeed(float& vx, float& vy) {
    float sp = len(vx, vy);
    if (sp > kVMax) {
        float s = kVMax / sp;
        vx *= s;
        vy *= s;
    }
}

}  // namespace

const char* Game::pocketName() const { return pocketAt(eightPocket_).name; }

int Game::upCount(bool includeEight) const {
    int n = 0;
    for (int i = 1; i <= kRack; i++) {
        if (ball_[i].down) continue;
        if (!includeEight && i == 8) continue;
        n++;
    }
    return n;
}

bool Game::inTable(float x, float y) const {
    if (x < kFeltL + kBallR + 1.5f || x > kFeltR - kBallR - 1.5f) return false;
    if (y < kFeltT + kBallR + 1.5f || y > kFeltB - kBallR - 1.5f) return false;
    for (int i = 0; i < 6; i++) {
        Pocket p = pocketAt(i);
        if (len(x - p.x, y - p.y) < p.sink + 2.5f) return false;
    }
    return true;
}

bool Game::overlaps(float x, float y, int ignore) const {
    const float need = kBallR * 2.f + 0.55f;
    for (int i = 0; i <= kRack; i++) {
        if (i == ignore || ball_[i].down) continue;
        if (len(ball_[i].x - x, ball_[i].y - y) < need) return true;
    }
    return false;
}

bool Game::freePoint(float x, float y, int ignore) const { return inTable(x, y) && !overlaps(x, y, ignore); }

bool Game::pathClear(float x0, float y0, float x1, float y1, int ignoreA, int ignoreB) const {
    const float need = kBallR * 2.f - 0.4f;
    for (int i = 0; i <= kRack; i++) {
        if (i == ignoreA || i == ignoreB || ball_[i].down) continue;
        if (distPointSeg(ball_[i].x, ball_[i].y, x0, y0, x1, y1) < need) return false;
    }
    return true;
}

bool Game::crossesPocket(float x0, float y0, float x1, float y1, int allow) const {
    for (int i = 0; i < 6; i++) {
        if (i == allow) continue;
        Pocket p = pocketAt(i);
        if (distPointSeg(p.x, p.y, x0, y0, x1, y1) < p.sink * 0.55f) return true;
    }
    return false;
}

int Game::pocketAlong(float x, float y, float dx, float dy) const {
    float d = len(dx, dy);
    if (d < 1e-4f) return called_;
    dx /= d;
    dy /= d;
    int best = 0;
    float bestDot = -2.f;
    for (int i = 0; i < 6; i++) {
        Pocket p = pocketAt(i);
        float px = p.x - x, py = p.y - y;
        float pd = len(px, py);
        if (pd < 1.f) continue;
        float dot = (px / pd) * dx + (py / pd) * dy;
        if (dot > bestDot) {
            bestDot = dot;
            best = i;
        }
    }
    return best;
}

bool Game::placeOnLine(int ball, float dirx, float diry, float& ox, float& oy) const {
    const float bx = ball_[ball].x, by = ball_[ball].y;
    const float ghostx = bx - dirx * (kBallR * 2.f + 0.35f);
    const float ghosty = by - diry * (kBallR * 2.f + 0.35f);
    if (!inTable(ghostx, ghosty)) return false;
    const float backs[] = {18.f, 28.f, 40.f, 56.f, 76.f, 100.f, 128.f};
    for (float back : backs) {
        float x = ghostx - dirx * back;
        float y = ghosty - diry * back;
        if (!freePoint(x, y, 0)) continue;
        if (!pathClear(x, y, ghostx, ghosty, 0, ball)) continue;
        if (crossesPocket(x, y, ghostx, ghosty, -1)) continue;
        ox = x;
        oy = y;
        return true;
    }
    return false;
}

Game::Plan Game::bestMake(bool allowPlace) {
    Plan best;
    float bestScore = -1e9f;
    const int beside = upCount(false);
    for (int n = 1; n <= kRack; n++) {
        if (ball_[n].down) continue;
        if (n == 8 && beside > 0) continue;
        for (int pk = 0; pk < 6; pk++) {
            if (n == 8 && !pocketGold(pk)) continue;
            Pocket p = pocketAt(pk);
            float dx = p.x - ball_[n].x, dy = p.y - ball_[n].y;
            float dist = len(dx, dy);
            if (dist < 8.f) continue;
            float dirx = dx / dist, diry = dy / dist;
            const int ignCue = allowPlace ? 0 : -1;
            if (!pathClear(ball_[n].x, ball_[n].y, p.x, p.y, n, ignCue)) continue;
            if (crossesPocket(ball_[n].x, ball_[n].y, p.x, p.y, pk)) continue;
            float travel = dist - p.sink * 0.42f;
            if (travel < 14.f) travel = 14.f;

            float ox = ball_[0].x, oy = ball_[0].y, ang = aim_, dot = 1.f;
            if (allowPlace) {
                if (!placeOnLine(n, dirx, diry, ox, oy)) continue;
                ang = std::atan2(ball_[n].y - oy, ball_[n].x - ox);
            } else {
                float gx = ball_[n].x - dirx * (kBallR * 2.f + 0.35f);
                float gy = ball_[n].y - diry * (kBallR * 2.f + 0.35f);
                if (!inTable(gx, gy)) continue;
                if (!pathClear(ball_[0].x, ball_[0].y, gx, gy, 0, n)) continue;
                if (crossesPocket(ball_[0].x, ball_[0].y, gx, gy, -1)) continue;
                float adx = gx - ball_[0].x, ady = gy - ball_[0].y;
                float ad = len(adx, ady);
                if (ad < 8.f) continue;
                float ax = adx / ad, ay = ady / ad;
                dot = ax * dirx + ay * diry;
                if (dot < 0.82f) continue;
                ang = std::atan2(ay, ax);
            }
            float distCue = len(ball_[n].x - ox, ball_[n].y - oy) - kBallR * 2.f;
            if (distCue < 10.f) distCue = 10.f;
            float vContact = std::sqrt(2.f * kMu * travel) * 1.55f / std::max(dot, 0.55f);
            if (vContact < 150.f) vContact = 150.f;
            float v0 = std::sqrt(vContact * vContact + 2.f * kMu * distCue);
            float power = v0 / kVMax;
            if (!std::isfinite(power) || power > 0.9f || power < 0.12f) continue;
            float score = dot * 8.f - travel * 0.004f - distCue * 0.002f;
            if (pocketGold(pk)) score += 0.25f;
            if (n == 8) score += 2.f;
            if (score > bestScore) {
                bestScore = score;
                best.ok = true;
                best.place = allowPlace;
                best.x = ox;
                best.y = oy;
                best.angle = ang;
                best.power = power;
                best.pocket = pk;
                best.ball = n;
            }
        }
    }
    return best;
}

Game::Plan Game::bestScatter() {
    int ids[8];
    int n = 0;
    const int beside = upCount(false);
    for (int i = 1; i <= kRack; i++) {
        if (ball_[i].down) continue;
        if (i == 8 && beside > 0) continue;
        ids[n++] = i;
    }
    Plan p;
    if (!n) return p;
    const int target = ids[(shots_ + stall_ * 3) % n];
    if (target == 8) {
        for (int pk = 0; pk < 6; pk++) {
            if (!pocketGold(pk)) continue;
            Pocket pkc = pocketAt(pk);
            float dx = pkc.x - ball_[8].x, dy = pkc.y - ball_[8].y;
            float d = len(dx, dy);
            if (d < 1.f) continue;
            float ox, oy;
            if (!placeOnLine(8, dx / d, dy / d, ox, oy)) continue;
            p.ok = true;
            p.place = true;
            p.x = ox;
            p.y = oy;
            p.angle = std::atan2(ball_[8].y - oy, ball_[8].x - ox);
            p.power = 0.55f;
            p.pocket = pk;
            p.ball = 8;
            return p;
        }
    }
    const int start = (shots_ * 5 + stall_ * 2) % 16;
    for (int k = 0; k < 16; k++) {
        float a = float(start + k) * (kPi * 2.f / 16.f);
        float dx = std::cos(a), dy = std::sin(a);
        float ox, oy;
        if (!placeOnLine(target, dx, dy, ox, oy)) continue;
        p.ok = true;
        p.place = true;
        p.x = ox;
        p.y = oy;
        p.angle = std::atan2(ball_[target].y - oy, ball_[target].x - ox);
        p.power = 0.48f + 0.06f * float(stall_ % 4);
        p.pocket = pocketAlong(ball_[target].x, ball_[target].y, dx, dy);
        if (target == 8 && !pocketGold(p.pocket)) p.pocket = 0;
        p.ball = target;
        return p;
    }
    for (int ring = 0; ring <= 6; ring++) {
        int steps = ring == 0 ? 1 : 10;
        for (int s = 0; s < steps; s++) {
            float a = (s + ring * 0.37f) * (kPi * 2.f) / float(steps);
            float rad = float(ring) * (kBallR * 2.f + 2.f);
            float x = kHeadX + std::cos(a) * rad;
            float y = kMidY + std::sin(a) * rad * 0.62f;
            if (!freePoint(x, y, 0)) continue;
            p.ok = true;
            p.place = true;
            p.x = x;
            p.y = y;
            p.angle = std::atan2(ball_[target].y - y, ball_[target].x - x);
            p.power = 0.62f;
            p.pocket = pocketAlong(ball_[target].x, ball_[target].y, ball_[target].x - x, ball_[target].y - y);
            if (target == 8 && !pocketGold(p.pocket)) p.pocket = 2;
            p.ball = target;
            return p;
        }
    }
    return p;
}

Game::Plan Game::choose() {
    Plan hand = bestMake(true);
    if (hand.ok && (stall_ < 2 || hand.ball == 8)) return hand;
    if (stall_ >= 2) {
        Plan s = bestScatter();
        if (s.ok) return s;
    }
    if (hand.ok) return hand;
    Plan lie = bestMake(false);
    if (lie.ok && lie.ball != 8) return lie;
    if (lie.ok) return lie;
    return bestScatter();
}

void Game::blip(float freq, float vol, float hold) {
    if (!sys_) return;
    sys_->apu.tone(0, freq, vol);
    toneT_ = hold;
}

void Game::rack() {
    for (int i = 0; i <= kRack; i++) ball_[i] = Ball{};
    ball_[0].x = kHeadX;
    ball_[0].y = kMidY;
    static const int order[8] = {1, 2, 3, 4, 8, 5, 6, 7};
    static const int rows[4] = {1, 2, 3, 2};
    const float gap = kBallR * 2.f + 0.55f;
    const float step = gap * 0.8660254f;
    int k = 0;
    for (int row = 0; row < 4; row++) {
        int cols = rows[row];
        for (int col = 0; col < cols; col++) {
            int n = order[k++];
            ball_[n].x = kFootX + float(row) * step;
            ball_[n].y = kMidY + (float(col) - float(cols - 1) * 0.5f) * gap;
        }
    }
}

void Game::resetMatch() {
    score_ = gold_ = cream_ = 0;
    shots_ = stall_ = 0;
    called_ = 0;
    eightPocket_ = 0;
    eightGold_ = false;
    aim_ = 0.02f;
    power_ = 0.78f;
    opening_ = true;
    commit_ = false;
    over_ = won_ = false;
    say_ = "SHORT EIGHT";
    sayT_ = 1.4f;
    reason_ = "OPEN";
    fanStep_ = -1;
    fanT_ = toneT_ = 0;
    mode_ = Mode::Title;
    t_ = 0;
    rack();
    if (ball_[1].x != ball_[0].x || ball_[1].y != ball_[0].y)
        aim_ = std::atan2(ball_[1].y - ball_[0].y, ball_[1].x - ball_[0].x);
    if (sys_) {
        sys_->apu.tone(0, 0, 0);
        sys_->apu.tone(1, 0, 0);
        sys_->setLight(40, 28, 16);
    }
}

void Game::begin() {
    mode_ = Mode::Stroke;
    opening_ = true;
    say_ = "BREAK";
    sayT_ = 0.7f;
    float tx = ball_[1].x;
    float ty = ball_[1].y - 3.4f;
    aim_ = std::atan2(ty - ball_[0].y, tx - ball_[0].x);
    power_ = 0.84f;
    called_ = pocketAlong(ball_[1].x, ball_[1].y, std::cos(aim_), std::sin(aim_));
    if (!pocketGold(called_)) called_ = 2;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.85f);
    sys.apu.setEcho(0.14f, 0.22f, 0.16f);
    resetMatch();
}

void Game::sink(Ball& b, int pocket) {
    if (b.down) return;
    b.down = true;
    b.fell = true;
    b.pocket = pocket;
    b.vx = b.vy = 0;
    b.x = b.y = -400.f;
    pocketSnd_ = true;
    if (sys_) sys_->rumble(0.22f, 0.55f, 36);
}

void Game::take(int pocket) {
    if (pocketGold(pocket)) gold_++;
    else cream_++;
    score_ = gold_ * 2 + cream_;
}

void Game::respot(int n) {
    Ball& b = ball_[n];
    b.down = false;
    b.fell = false;
    b.pocket = -1;
    b.vx = b.vy = 0;
    for (int ring = 0; ring <= 8; ring++) {
        int steps = ring == 0 ? 1 : 12;
        for (int s = 0; s < steps; s++) {
            float a = (float(s) + float(ring) * 0.41f) * (kPi * 2.f) / float(steps);
            float rad = float(ring) * (kBallR * 2.f + 1.4f);
            float x = kFootX + std::cos(a) * rad;
            float y = kMidY + std::sin(a) * rad * 0.72f;
            if (!freePoint(x, y, n)) continue;
            b.x = x;
            b.y = y;
            return;
        }
    }
    b.x = kFootX;
    b.y = kMidY - 24.f;
}

void Game::dropCue(float x, float y) {
    ball_[0].down = false;
    ball_[0].fell = false;
    ball_[0].vx = ball_[0].vy = 0;
    ball_[0].pocket = -1;
    if (freePoint(x, y, 0)) {
        ball_[0].x = x;
        ball_[0].y = y;
        return;
    }
    for (int ring = 1; ring <= 8; ring++) {
        for (int s = 0; s < 12; s++) {
            float a = float(s) * (kPi * 2.f / 12.f);
            float rad = float(ring) * (kBallR * 2.f + 1.6f);
            float px = x + std::cos(a) * rad;
            float py = y + std::sin(a) * rad * 0.7f;
            if (!freePoint(px, py, 0)) continue;
            ball_[0].x = px;
            ball_[0].y = py;
            return;
        }
    }
    ball_[0].x = kHeadX;
    ball_[0].y = kMidY;
}

void Game::shoot() {
    beside_ = upCount(false);
    for (int i = 0; i <= kRack; i++) {
        ball_[i].fell = false;
        ball_[i].pocket = -1;
    }
    ball_[0].down = false;
    float sp = std::clamp(power_, 0.15f, 1.f) * kVMax;
    ball_[0].vx = std::cos(aim_) * sp;
    ball_[0].vy = std::sin(aim_) * sp;
    mode_ = Mode::Roll;
    opening_ = false;
    commit_ = false;
    shots_++;
    rollT_ = 0;
    if (sys_) {
        sys_->apu.noiseBurst(0.35f, 2400.f, 0.04f);
        sys_->rumble(0.08f, 0.28f, 22);
    }
    if (tracing()) {
        std::fprintf(stderr, "shoot %d call %s pwr %.2f beside %d stall %d\n", shots_, pocketAt(called_).name, power_,
                     beside_, stall_);
    }
}

void Game::winRack(int pocket) {
    if (!pocketGold(pocket)) return;
    eightPocket_ = pocket;
    eightGold_ = true;
    take(pocket);
    won_ = true;
    over_ = true;
    mode_ = Mode::Win;
    say_ = "DOUBLE";
    sayT_ = 8.f;
    reason_ = "DOUBLE";
    fanT_ = 0;
    fanStep_ = -1;
    if (sys_) {
        sys_->rumble(0.4f, 0.85f, 180);
        sys_->setLight(255, 196, 48);
    }
}

void Game::loseRack(const char* why) {
    reason_ = why;
    won_ = false;
    over_ = true;
    mode_ = Mode::Lose;
    say_ = why;
    sayT_ = 8.f;
    if (sys_) sys_->setLight(160, 32, 28);
}

void Game::resolve() {
    const bool scratch = ball_[0].fell;
    const bool eightFell = ball_[8].fell;
    const int eightPk = ball_[8].pocket;
    for (int i = 0; i <= kRack; i++)
        if (!ball_[i].down) ball_[i].vx = ball_[i].vy = 0;

    if (eightFell && beside_ == 0) {
        if (!scratch && eightPk == called_ && pocketGold(eightPk)) {
            winRack(eightPk);
            return;
        }
        respot(8);
        if (scratch) say_ = "SCRATCH";
        else if (!pocketGold(eightPk)) say_ = "NOT THE DOUBLE";
        else say_ = "WRONG POCKET";
        sayT_ = 0.9f;
        reason_ = say_;
        stall_++;
    } else {
        if (eightFell) {
            respot(8);
            say_ = "EARLY 8";
            sayT_ = 0.75f;
        }
        int made = 0;
        for (int i = 1; i <= 7; i++) {
            if (!ball_[i].fell) continue;
            made++;
            take(ball_[i].pocket);
        }
        if (made) {
            stall_ = 0;
            if (!eightFell) {
                say_ = "POCKET";
                sayT_ = 0.55f;
            }
        } else if (!eightFell) {
            say_ = scratch ? "SCRATCH" : "MISS";
            sayT_ = 0.6f;
            stall_++;
        } else {
            stall_++;
        }
        reason_ = say_;
    }

    if (mode_ == Mode::Win || mode_ == Mode::Lose) return;
    if (scratch) dropCue(kHeadX, kMidY);
    else ball_[0].down = false;
    mode_ = scratch ? Mode::Place : Mode::Stroke;
    commit_ = false;
    if (bot_ && shots_ >= 72) loseRack("TOO MANY SHOTS");
    if (tracing()) std::fprintf(stderr, "  rest left %d gold %d cream %d %s\n", upCount(true), gold_, cream_, say_);
}

bool Game::anyMoving(float v) const {
    for (int i = 0; i <= kRack; i++) {
        if (ball_[i].down) continue;
        if (len(ball_[i].vx, ball_[i].vy) > v) return true;
    }
    return false;
}

bool Game::jaw() {
    bool nudged = false;
    for (int i = 0; i <= kRack; i++) {
        Ball& b = ball_[i];
        if (b.down) continue;
        int best = -1;
        float bd = 1e9f;
        float bdx = 0, bdy = 0;
        for (int p = 0; p < 6; p++) {
            Pocket pk = pocketAt(p);
            float dx = pk.x - b.x, dy = pk.y - b.y;
            float d = len(dx, dy);
            if (d < bd) {
                bd = d;
                best = p;
                bdx = dx;
                bdy = dy;
            }
        }
        if (best < 0 || bd < 0.5f) continue;
        Pocket pk = pocketAt(best);
        if (bd < pk.sink) {
            sink(b, best);
            continue;
        }
        if (bd < pk.sink + 5.f) {
            float nx = bdx / bd, ny = bdy / bd;
            float approach = b.vx * nx + b.vy * ny;
            if (approach > -8.f) {
                b.vx = nx * 110.f;
                b.vy = ny * 110.f;
                nudged = true;
            }
        }
    }
    return nudged;
}

void Game::forceStop() {
    for (int i = 0; i <= kRack; i++) {
        Ball& b = ball_[i];
        if (b.down) continue;
        int best = -1;
        float bd = 1e9f;
        for (int p = 0; p < 6; p++) {
            float d = len(b.x - pocketAt(p).x, b.y - pocketAt(p).y);
            if (d < pocketAt(p).sink + 2.f && d < bd) {
                bd = d;
                best = p;
            }
        }
        if (best >= 0) sink(b, best);
        else b.vx = b.vy = 0;
    }
}

void Game::physics(float dt) {
    const float h = dt / float(kSub);
    for (int s = 0; s < kSub; s++) {
        for (int i = 0; i <= kRack; i++) {
            Ball& b = ball_[i];
            if (b.down) continue;
            for (int p = 0; p < 6; p++) {
                Pocket pk = pocketAt(p);
                float dx = pk.x - b.x, dy = pk.y - b.y;
                float d = len(dx, dy);
                if (d >= pk.sink && d < pk.sink + 7.f && d > 0.5f) {
                    float nx = dx / d, ny = dy / d;
                    if (b.vx * nx + b.vy * ny > 12.f) {
                        b.vx += nx * 220.f * h;
                        b.vy += ny * 220.f * h;
                    }
                }
            }
            b.x += b.vx * h;
            b.y += b.vy * h;
            damp(b.vx, b.vy, h);
            capSpeed(b.vx, b.vy);
        }
        for (int pass = 0; pass < 5; pass++) {
            for (int i = 0; i <= kRack; i++) {
                if (ball_[i].down) continue;
                for (int j = i + 1; j <= kRack; j++) {
                    if (ball_[j].down) continue;
                    float dx = ball_[j].x - ball_[i].x;
                    float dy = ball_[j].y - ball_[i].y;
                    float d2 = dx * dx + dy * dy;
                    const float minD = kBallR * 2.f;
                    if (d2 >= minD * minD || d2 < 1e-8f) continue;
                    float d = std::sqrt(d2);
                    float nx = dx / d, ny = dy / d;
                    float rel = (ball_[i].vx - ball_[j].vx) * nx + (ball_[i].vy - ball_[j].vy) * ny;
                    if (rel > 0.f) {
                        ball_[i].vx -= rel * nx;
                        ball_[i].vy -= rel * ny;
                        ball_[j].vx += rel * nx;
                        ball_[j].vy += rel * ny;
                        if (rel > 50.f) clack_ = true;
                    }
                    float push = (minD - d) * 0.52f;
                    ball_[i].x -= nx * push;
                    ball_[i].y -= ny * push;
                    ball_[j].x += nx * push;
                    ball_[j].y += ny * push;
                }
            }
        }
        auto pocketPass = [&]() {
            for (int i = 0; i <= kRack; i++) {
                if (ball_[i].down) continue;
                int best = -1;
                float bd = 1e9f;
                for (int p = 0; p < 6; p++) {
                    float d = len(ball_[i].x - pocketAt(p).x, ball_[i].y - pocketAt(p).y);
                    if (d < pocketAt(p).sink && d < bd) {
                        bd = d;
                        best = p;
                    }
                }
                if (best >= 0) sink(ball_[i], best);
            }
        };
        pocketPass();
        const float minX = kFeltL + kBallR, maxX = kFeltR - kBallR;
        const float minY = kFeltT + kBallR, maxY = kFeltB - kBallR;
        for (int i = 0; i <= kRack; i++) {
            Ball& b = ball_[i];
            if (b.down) continue;
            if (b.x < minX) {
                b.x = minX;
                if (b.vx < 0) b.vx = -b.vx * kRest;
            } else if (b.x > maxX) {
                b.x = maxX;
                if (b.vx > 0) b.vx = -b.vx * kRest;
            }
            if (b.y < minY) {
                b.y = minY;
                if (b.vy < 0) b.vy = -b.vy * kRest;
            } else if (b.y > maxY) {
                b.y = maxY;
                if (b.vy > 0) b.vy = -b.vy * kRest;
            }
        }
        pocketPass();
    }
}

void Game::botAct() {
    if (commit_ && mode_ == Mode::Place) {
        called_ = commitPocket_;
        aim_ = commitAim_;
        power_ = commitPower_;
        commit_ = false;
        shoot();
        return;
    }
    if (opening_) {
        float tx = ball_[1].x;
        float ty = ball_[1].y - 3.4f;
        aim_ = std::atan2(ty - ball_[0].y, tx - ball_[0].x);
        power_ = 0.84f;
        called_ = pocketAlong(ball_[1].x, ball_[1].y, std::cos(aim_), std::sin(aim_));
        if (!pocketGold(called_)) called_ = 2;
        shoot();
        return;
    }
    Plan p = choose();
    if (!p.ok) {
        loseRack("NO SHOT");
        return;
    }
    if (p.ball == 8 && !pocketGold(p.pocket)) p.pocket = 2;
    called_ = p.pocket;
    aim_ = p.angle;
    power_ = p.power;
    if (p.place) {
        ball_[0].down = false;
        ball_[0].vx = ball_[0].vy = 0;
        ball_[0].x = p.x;
        ball_[0].y = p.y;
        commit_ = true;
        commitPocket_ = p.pocket;
        commitAim_ = p.angle;
        commitPower_ = p.power;
        mode_ = Mode::Place;
        return;
    }
    shoot();
}

void Game::human() {
    const gs::Pad& pad = sys_->pad;
    if (pad.pressed(gs::BTN_C)) called_ = (called_ + 1) % 6;
    if (pad.pressed(gs::BTN_X)) called_ = (called_ + 5) % 6;

    if (mode_ == Mode::Place) {
        float sp = (pad.down(gs::BTN_TURBO) ? 360.f : 210.f) * kDt;
        float dx = 0, dy = 0;
        if (pad.down(gs::BTN_LEFT)) dx -= sp;
        if (pad.down(gs::BTN_RIGHT)) dx += sp;
        if (pad.down(gs::BTN_UP)) dy -= sp;
        if (pad.down(gs::BTN_DOWN)) dy += sp;
        if (std::fabs(pad.axisX) > 0.2f) dx += pad.axisX * sp;
        if (std::fabs(pad.axisY) > 0.2f) dy -= pad.axisY * sp;
        float nx = ball_[0].x + dx, ny = ball_[0].y + dy;
        if (freePoint(nx, ny, 0)) {
            ball_[0].x = nx;
            ball_[0].y = ny;
        }
        if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Stroke;
            say_ = "CALL THE 8 IN GOLD";
            sayT_ = 0.7f;
        }
        return;
    }

    float rate = pad.down(gs::BTN_Y) ? 0.7f : 2.15f;
    if (std::fabs(pad.axisX) > 0.18f) aim_ += pad.axisX * 2.4f * kDt;
    else {
        if (pad.down(gs::BTN_LEFT)) aim_ -= rate * kDt;
        if (pad.down(gs::BTN_RIGHT)) aim_ += rate * kDt;
    }
    if (aim_ > kPi) aim_ -= kPi * 2.f;
    if (aim_ < -kPi) aim_ += kPi * 2.f;
    if (pad.down(gs::BTN_UP)) power_ = std::min(1.f, power_ + 0.012f);
    if (pad.down(gs::BTN_DOWN)) power_ = std::max(0.16f, power_ - 0.012f);
    if (pad.pressed(gs::BTN_B)) {
        mode_ = Mode::Place;
        say_ = "BALL IN HAND";
        sayT_ = 0.7f;
        return;
    }
    if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_TURBO)) shoot();
}

void Game::fanfare() {
    static const float notes[] = {523.25f, 659.25f, 783.99f, 1046.5f};
    int step = int(fanT_ / 0.12f);
    if (step != fanStep_ && step >= 0 && step < 4) {
        sys_->apu.tone(0, notes[step], 0.16f);
        sys_->apu.tone(1, notes[step] * 0.5f, 0.08f);
        fanStep_ = step;
    } else if (step >= 8 && fanStep_ != 99) {
        sys_->apu.tone(0, 0, 0);
        sys_->apu.tone(1, 0, 0);
        fanStep_ = 99;
    }
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!sys_ || !s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        int tile = art_.font[c - 32];
        if (!tile) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(tile, pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::stamp(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool shadow) {
    if (w < 1.f || h < 1.f) return;
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - w * 0.5f));
    s.y = int16_t(std::lround(cy - h * 0.5f));
    s.img = img;
    s.pal = uint8_t(pal);
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::stampM(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow) {
    if (m.h < 1 || h < 1.f) return;
    float w = h * float(m.w) / float(m.h);
    stamp(m.pick(h), cx, cy, w, h, pal, shadow);
}

void Game::aimAid() {
    if (mode_ != Mode::Stroke && mode_ != Mode::Title && mode_ != Mode::Place && mode_ != Mode::Pause) return;
    if (ball_[0].down) return;
    float dx = std::cos(aim_), dy = std::sin(aim_);
    int hit = -1;
    float tHit = 1e9f;
    const float rad = kBallR * 2.f;
    for (int i = 1; i <= kRack; i++) {
        if (ball_[i].down) continue;
        float cx = ball_[i].x - ball_[0].x;
        float cy = ball_[i].y - ball_[0].y;
        float tc = cx * dx + cy * dy;
        if (tc <= 0.f) continue;
        float perp2 = cx * cx + cy * cy - tc * tc;
        if (perp2 > rad * rad) continue;
        float t = tc - std::sqrt(std::max(0.f, rad * rad - perp2));
        if (t < 3.f) continue;
        if (t < tHit) {
            tHit = t;
            hit = i;
        }
    }
    float limit = hit > 0 ? tHit : 78.f;
    int n = std::clamp(int(limit / 12.f), 1, 6);
    int pal = pocketGold(called_) ? PAL_GOLD : PAL_CREAM;
    for (int i = 1; i <= n; i++) {
        float t = float(i) * 12.f;
        if (hit > 0 && t > tHit - 6.f) break;
        stampM(art_.dot, ball_[0].x + dx * t, ball_[0].y + dy * t, 4.f, pal);
    }
    if (hit > 0) {
        float hx = ball_[0].x + dx * tHit;
        float hy = ball_[0].y + dy * tHit;
        float nx = ball_[hit].x - hx, ny = ball_[hit].y - hy;
        float nd = len(nx, ny);
        if (nd > 0.2f) {
            nx /= nd;
            ny /= nd;
            for (int i = 1; i <= 4; i++)
                stampM(art_.dot, ball_[hit].x + nx * float(i) * 12.f, ball_[hit].y + ny * float(i) * 12.f, 3.5f, PAL_HUD);
        }
    }
}

void Game::cueDraw() {
    if (mode_ == Mode::Roll || mode_ == Mode::Win || mode_ == Mode::Lose) return;
    if (ball_[0].down) return;
    int qi = int(std::lround(aim_ / (kPi * 2.f) * float(kCueAngles)));
    qi %= kCueAngles;
    if (qi < 0) qi += kCueAngles;
    float q = float(qi) * (kPi * 2.f / float(kCueAngles));
    float reach = kBallR + 6.f + kCueTip;
    float cx = ball_[0].x - std::cos(q) * reach;
    float cy = ball_[0].y - std::sin(q) * reach;
    stampM(art_.cue[qi], cx, cy, float(kCueBox), PAL_CUE);
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    v.A.enabled = true;
    v.B.enabled = false;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        int band = (y * 3) / gs::SCREEN_H;
        int g = 6 + band;
        bool nap = ((y / 4) & 1) == 0;
        v.lineBackdrop[y] = nap ? gs::rgb4(1, g, 2) : gs::rgb4(1, g + 2, 3);
    }
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    if (mode_ == Mode::Title) {
        stampM(art_.logo, 160.f, 52.f, float(art_.logo.h), PAL_LOGO);
        stampM(art_.sub, 160.f, 74.f, float(art_.sub.h), PAL_LOGO);
    } else if (mode_ == Mode::Win) {
        stampM(art_.win, 160.f, 96.f, float(art_.win.h), PAL_LOGO);
        stampM(art_.tag, 160.f, 128.f, float(art_.tag.h), PAL_LOGO);
        Pocket pk = pocketAt(eightPocket_);
        stampM(art_.ball[8], pk.x, pk.y, kBallR * 2.f, PAL_BALL);
    } else if (mode_ == Mode::Lose) {
        stampM(art_.tag, 160.f, 100.f, float(art_.tag.h), PAL_LOGO);
    }

    aimAid();
    cueDraw();
    for (int i = 0; i <= kRack; i++) {
        if (ball_[i].down) continue;
        stampM(art_.ball[i], ball_[i].x, ball_[i].y, kBallR * 2.f, PAL_BALL);
    }
    for (int i = 0; i <= kRack; i++) {
        if (ball_[i].down) continue;
        stampM(art_.shadow, ball_[i].x + 2.5f, ball_[i].y + 3.2f, 7.f, PAL_BALL, true);
    }
    if (mode_ == Mode::Title || mode_ == Mode::Stroke || mode_ == Mode::Place || mode_ == Mode::Roll ||
        mode_ == Mode::Pause) {
        Pocket pk = pocketAt(called_);
        float pulse = 26.f + std::sin(t_ * 7.f) * 2.0f;
        stampM(art_.glow, pk.x, pk.y, pulse, pocketGold(called_) ? PAL_GOLD : PAL_CREAM);
    }

    char buf[64];
    hud(1, 0, "S3 EIGHT GOLD", PAL_GOLD);
    std::snprintf(buf, sizeof buf, "SCORE %d", score_);
    hud(39 - int(std::strlen(buf)), 0, buf, PAL_HUD);

    if (mode_ == Mode::Title) {
        hudC(25, "CORNERS COUNT DOUBLE. SIDES COUNT ONE.", PAL_HUD);
        hudC(26, "THE 8 CLEARS ONLY IN GOLD.", PAL_GOLD);
        hudC(27, "RETURN BREAKS", PAL_CREAM);
        return;
    }

    Pocket call = pocketAt(called_);
    std::snprintf(buf, sizeof buf, "CALL %s %s", call.name, call.gold ? "x2" : "x1");
    hud(1, 1, buf, call.gold ? PAL_GOLD : PAL_CREAM);
    std::snprintf(buf, sizeof buf, "LEFT %d", upCount(true));
    hud(39 - int(std::strlen(buf)), 1, buf, PAL_HUD);

    const char* msg = "GOLD POCKET x2";
    if (mode_ == Mode::Pause) msg = "PAUSED";
    else if (mode_ == Mode::Win) msg = "ONLY THE GOLD COUNTS DOUBLE";
    else if (mode_ == Mode::Lose) msg = say_;
    else if (sayT_ > 0 && say_) msg = say_;
    else if (mode_ == Mode::Roll) msg = "ROLLING";
    else if (mode_ == Mode::Place) msg = "BALL IN HAND";
    else if (upCount(false) == 0 && !ball_[8].down) msg = "THE 8  GOLD POCKET ONLY";
    hud(1, 26, msg, mode_ == Mode::Lose ? PAL_RED : (mode_ == Mode::Win ? PAL_GOLD : PAL_HUD));

    if (mode_ == Mode::Win || mode_ == Mode::Lose) {
        hudC(27, "RETURN RERACKS", PAL_GOLD);
        return;
    }
    if (mode_ == Mode::Place) {
        hud(1, 27, "ARROWS PLACE    Z SETS", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Pause) {
        hud(1, 27, "RETURN RESUMES", PAL_GOLD);
        return;
    }
    int bars = std::clamp(int(std::lround(power_ * 8.f)), 0, 8);
    char pwr[16] = "PWR ";
    int n = 4;
    for (int i = 0; i < 8; i++) pwr[n++] = i < bars ? '#' : '-';
    pwr[n] = 0;
    hud(1, 27, pwr, PAL_GOLD);
    hud(16, 27, "Z SHOOT  C POCKET  X HAND", PAL_HUD);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    if (sayT_ > 0) sayT_ = std::max(0.f, sayT_ - kDt);
    if (toneT_ > 0) toneT_ = std::max(0.f, toneT_ - kDt);
    else if (mode_ != Mode::Win) {
        sys.apu.tone(0, 0, 0);
        sys.apu.tone(1, 0, 0);
    }
    pocketSnd_ = false;
    clack_ = false;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (bot_ && t_ > 0.35f) begin();
        else if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) begin();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) sys.quit();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = held_;
        else if (pad.pressed(gs::BTN_MODE)) resetMatch();
    } else if (mode_ == Mode::Win) {
        fanT_ += kDt;
        fanfare();
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) resetMatch();
    } else if (mode_ == Mode::Lose) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) resetMatch();
    } else if (mode_ == Mode::Roll) {
        rollT_ += kDt;
        physics(kDt);
        if (rollT_ > 6.2f) {
            forceStop();
            resolve();
        } else if (!anyMoving(16.f)) {
            if (!jaw()) resolve();
        }
        if (mode_ == Mode::Roll) {
            if (pocketSnd_) blip(740.f, 0.12f, 0.08f);
            else if (clack_) sys.apu.noiseBurst(0.16f, 1800.f, 0.03f);
        }
    } else if (mode_ == Mode::Stroke || mode_ == Mode::Place) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            held_ = mode_;
            mode_ = Mode::Pause;
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            resetMatch();
        } else if (bot_) {
            botAct();
        } else {
            human();
        }
    }

    draw();
}

}  // namespace eightgold

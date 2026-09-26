#include "game/eightchime.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace eightchime {
namespace {

constexpr int kCue = 0;
constexpr int kEight = 1;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;
constexpr float kDt = 1.f / 60.f;
constexpr float kMu = 165.f;
constexpr float kVMax = 640.f;
constexpr float kRest = 0.7f;
constexpr float kStop = 20.f;
constexpr int kSub = 8;
constexpr int kFpc = 6;
constexpr int kGraceSec = 24;
constexpr int kHourSec = 12 * 3600;
constexpr int kAhead = 60;
constexpr int kStartSec = kHourSec - kAhead;
constexpr int kInto = 4;
constexpr int kMaxStrokes = 3;
constexpr float kGap = 40.f;
constexpr float kTip = 20.f;

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
    float sp = std::hypot(vx, vy);
    if (sp > kVMax) {
        float s = kVMax / sp;
        vx *= s;
        vy *= s;
    }
}

float strokeSpeed(float power) { return std::clamp(power, 0.08f, 1.f) * kVMax; }

gs::FMPatch bellPatch() {
    gs::FMPatch p;
    p.alg = 6;
    p.fb = 0.18f;
    p.op[0] = {1.0f, 1.0f, 0.004f, 0.52f, 0.14f, 0.46f};
    p.op[1] = {2.6f, 0.28f, 0.003f, 0.34f, 0.0f, 0.26f};
    p.op[2] = {4.2f, 0.14f, 0.003f, 0.18f, 0.0f, 0.16f};
    p.op[3] = {1.5f, 0.22f, 0.005f, 0.58f, 0.06f, 0.34f};
    p.vol = 0.28f;
    p.echo = 0.30f;
    p.tone = 1600.f;
    return p;
}

}  // namespace

int Game::secFrom(int play) const {
    int t = kStartSec + play / kFpc;
    return t < 0 ? 0 : t;
}

int Game::untilFrom(int play) const {
    int sec = secFrom(play);
    int sub = play % kFpc;
    if (sub < 0) sub = 0;
    if (sec > kHourSec) return -((sec - kHourSec) * kFpc + sub);
    if (sec == kHourSec) return -sub;
    int secLeft = kHourSec - sec;
    return (secLeft - 1) * kFpc + (kFpc - sub);
}

bool Game::onHourPlay(int play) const {
    int sec = secFrom(play);
    return sec >= kHourSec && sec < kHourSec + kGraceSec;
}

bool Game::pastHourPlay(int play) const { return secFrom(play) >= kHourSec + kGraceSec; }

bool Game::shouldShoot(int play, int travel) const {
    if (travel < kInto + 8) return false;
    int until = untilFrom(play);
    int want = travel - kInto;
    int land = until - travel;
    bool inWindow = land <= 0 && land > -(kGraceSec * kFpc);
    return until == want || (until < want && inWindow);
}

bool Game::clockHits(int travel) const {
    if (travel < kInto + 8 || travel > 160) return false;
    int play = 0;
    const int cap = (kAhead + kGraceSec + 2) * kFpc;
    while (play < cap) {
        play++;
        if (!shouldShoot(play, travel)) continue;
        for (int s = 0; s < travel; s++) play++;
        return onHourPlay(play);
    }
    return false;
}

void Game::split(int& h, int& m, int& s) const {
    int t = secFrom(playFrames_);
    h = t / 3600;
    m = (t / 60) % 60;
    s = t % 60;
}

int Game::hour() const {
    int h, m, s;
    split(h, m, s);
    return h;
}

int Game::minute() const {
    int h, m, s;
    split(h, m, s);
    return m;
}

int Game::second() const {
    int h, m, s;
    split(h, m, s);
    return s;
}

void Game::faceTime(int& h, int& m, int& s) const { split(h, m, s); }

void Game::step(Body* b, bool* clack) const {
    auto sink = [](Body& ball, int pocket) {
        if (ball.down) return;
        ball.down = true;
        ball.fell = true;
        ball.pocket = pocket;
        ball.vx = ball.vy = 0;
    };
    const float h = kDt / float(kSub);
    const float minX = kFeltL + kR, maxX = kFeltR - kR;
    const float minY = kFeltT + kR, maxY = kFeltB - kR;
    for (int s = 0; s < kSub; s++) {
        for (int i = 0; i < kBalls; i++) {
            Body& ball = b[i];
            if (ball.down) continue;
            for (int p = 0; p < 6; p++) {
                Pocket pk = pocketAt(p);
                float dx = pk.x - ball.x, dy = pk.y - ball.y;
                float d = std::hypot(dx, dy);
                if (d < pk.sink + 6.f && d > 0.6f) {
                    float nx = dx / d, ny = dy / d;
                    if (ball.vx * nx + ball.vy * ny > 36.f) {
                        ball.vx += nx * 240.f * h;
                        ball.vy += ny * 240.f * h;
                    }
                }
            }
            ball.x += ball.vx * h;
            ball.y += ball.vy * h;
            damp(ball.vx, ball.vy, h);
            capSpeed(ball.vx, ball.vy);
        }
        for (int pass = 0; pass < 3; pass++) {
            for (int i = 0; i < kBalls; i++) {
                if (b[i].down) continue;
                for (int j = i + 1; j < kBalls; j++) {
                    if (b[j].down) continue;
                    float dx = b[j].x - b[i].x;
                    float dy = b[j].y - b[i].y;
                    float d2 = dx * dx + dy * dy;
                    const float minD = kR * 2.f;
                    if (d2 >= minD * minD || d2 < 1e-8f) continue;
                    float d = std::sqrt(d2);
                    float nx = dx / d, ny = dy / d;
                    float rel = (b[i].vx - b[j].vx) * nx + (b[i].vy - b[j].vy) * ny;
                    if (rel > 0.f) {
                        b[i].vx -= rel * nx;
                        b[i].vy -= rel * ny;
                        b[j].vx += rel * nx;
                        b[j].vy += rel * ny;
                        if (clack && rel > 48.f) *clack = true;
                    }
                    float push = (minD - d) * 0.52f;
                    b[i].x -= nx * push;
                    b[i].y -= ny * push;
                    b[j].x += nx * push;
                    b[j].y += ny * push;
                }
            }
        }
        auto mouths = [&]() {
            for (int i = 0; i < kBalls; i++) {
                if (b[i].down) continue;
                int best = -1;
                float bd = 1e9f;
                for (int p = 0; p < 6; p++) {
                    float d = std::hypot(b[i].x - pocketAt(p).x, b[i].y - pocketAt(p).y);
                    if (d < pocketAt(p).sink && d < bd) {
                        bd = d;
                        best = p;
                    }
                }
                if (best >= 0) sink(b[i], best);
            }
        };
        mouths();
        for (int i = 0; i < kBalls; i++) {
            Body& ball = b[i];
            if (ball.down) continue;
            if (ball.x < minX) {
                ball.x = minX;
                if (ball.vx < 0) ball.vx = -ball.vx * kRest;
            } else if (ball.x > maxX) {
                ball.x = maxX;
                if (ball.vx > 0) ball.vx = -ball.vx * kRest;
            }
            if (ball.y < minY) {
                ball.y = minY;
                if (ball.vy < 0) ball.vy = -ball.vy * kRest;
            } else if (ball.y > maxY) {
                ball.y = maxY;
                if (ball.vy > 0) ball.vy = -ball.vy * kRest;
            }
            capSpeed(ball.vx, ball.vy);
        }
        mouths();
    }
}

bool Game::moving(const Body* b) const {
    for (int i = 0; i < kBalls; i++) {
        if (b[i].down) continue;
        if (std::hypot(b[i].vx, b[i].vy) > kStop) return true;
    }
    return false;
}

bool Game::inCloth(float x, float y) const {
    if (x < kFeltL + kR + 1.f || x > kFeltR - kR - 1.f) return false;
    if (y < kFeltT + kR + 1.f || y > kFeltB - kR - 1.f) return false;
    for (int i = 0; i < 6; i++) {
        Pocket p = pocketAt(i);
        if (std::hypot(x - p.x, y - p.y) < p.sink + 1.5f) return false;
    }
    return true;
}

Game::Lie Game::roll(Body* b, float aim, float power, int* travel) const {
    for (int i = 0; i < kBalls; i++) {
        b[i].vx = b[i].vy = 0;
        b[i].fell = false;
        b[i].pocket = -1;
        b[i].down = false;
    }
    float sp = strokeSpeed(power);
    b[kCue].vx = std::cos(aim) * sp;
    b[kCue].vy = std::sin(aim) * sp;
    int n = 0;
    Lie lie = Lie::Miss;
    while (n < 260) {
        n++;
        step(b, nullptr);
        if (b[kCue].fell) {
            lie = Lie::Scratch;
            break;
        }
        if (b[kEight].fell) {
            lie = b[kEight].pocket == kHourPocket ? Lie::Hour : Lie::Wide;
            break;
        }
        if (!moving(b)) {
            lie = Lie::Miss;
            break;
        }
    }
    if (travel) *travel = n;
    return lie;
}

float Game::lineAim() const {
    Pocket pk = pocketAt(kHourPocket);
    float dx = pk.x - ball_[kEight].x;
    float dy = pk.y - ball_[kEight].y;
    float dist = std::hypot(dx, dy);
    if (dist < 1.f) dist = 1.f;
    float ux = dx / dist, uy = dy / dist;
    float gx = ball_[kEight].x - ux * (kR * 2.f + 0.4f);
    float gy = ball_[kEight].y - uy * (kR * 2.f + 0.4f);
    return std::atan2(gy - ball_[kCue].y, gx - ball_[kCue].x);
}

void Game::spot() {
    for (int i = 0; i < kBalls; i++) {
        Home h = homeAt(i);
        ball_[i] = Body{};
        ball_[i].x = h.x;
        ball_[i].y = h.y;
    }
    Pocket pk = pocketAt(kHourPocket);
    float dx = pk.x - ball_[kEight].x;
    float dy = pk.y - ball_[kEight].y;
    float dist = std::hypot(dx, dy);
    if (dist < 1.f) dist = 1.f;
    float ux = dx / dist, uy = dy / dist;
    ball_[kCue].x = ball_[kEight].x - ux * (kR * 2.f + kGap);
    ball_[kCue].y = ball_[kEight].y - uy * (kR * 2.f + kGap);
}

bool Game::layout() const {
    for (int i = 0; i < kBalls; i++) {
        if (!inCloth(ball_[i].x, ball_[i].y)) {
            std::fprintf(stderr, "s3eightchime ball %d off the cloth at %.1f %.1f\n", i, ball_[i].x, ball_[i].y);
            return false;
        }
        for (int j = i + 1; j < kBalls; j++) {
            if (std::hypot(ball_[i].x - ball_[j].x, ball_[i].y - ball_[j].y) < kR * 2.f + 0.8f) {
                std::fprintf(stderr, "s3eightchime balls %d and %d overlap\n", i, j);
                return false;
            }
        }
    }
    Pocket pk = pocketAt(kHourPocket);
    if (segDist(ball_[2].x, ball_[2].y, ball_[kCue].x, ball_[kCue].y, ball_[kEight].x, ball_[kEight].y) < kR * 2.f - 0.2f ||
        segDist(ball_[3].x, ball_[3].y, ball_[kCue].x, ball_[kCue].y, ball_[kEight].x, ball_[kEight].y) < kR * 2.f - 0.2f ||
        segDist(ball_[2].x, ball_[2].y, ball_[kEight].x, ball_[kEight].y, pk.x, pk.y) < kR * 2.f - 0.2f ||
        segDist(ball_[3].x, ball_[3].y, ball_[kEight].x, ball_[kEight].y, pk.x, pk.y) < kR * 2.f - 0.2f) {
        std::fprintf(stderr, "s3eightchime the hour line is blocked\n");
        return false;
    }
    return true;
}

bool Game::prove() {
    spot();
    if (!layout()) return false;
    float base = lineAim();
    const float powers[] = {0.42f, 0.48f, 0.54f, 0.36f, 0.60f, 0.30f, 0.66f, 0.72f};
    const float offsets[] = {0.f, -0.012f, 0.012f, -0.028f, 0.028f, -0.05f, 0.05f, -0.006f, 0.006f};
    float best = 1e9f;
    bool found = false;
    float bestAim = base, bestPower = 0.48f;
    int bestTravel = 0;
    Body sim[kBalls];
    for (float off : offsets) {
        for (float power : powers) {
            for (int i = 0; i < kBalls; i++) sim[i] = ball_[i];
            int tr = 0;
            Lie lie = roll(sim, base + off, power, &tr);
            if (lie != Lie::Hour || tr < 22 || tr > 140) continue;
            if (!clockHits(tr)) continue;
            float score = std::fabs(float(tr) - 46.f) + std::fabs(off) * 140.f + std::fabs(power - 0.5f) * 8.f;
            if (score < best) {
                best = score;
                found = true;
                bestAim = base + off;
                bestPower = power;
                bestTravel = tr;
            }
        }
    }
    if (!found) {
        for (int i = 0; i < kBalls; i++) sim[i] = ball_[i];
        int tr = 0;
        Lie natural = roll(sim, base, 0.48f, &tr);
        std::fprintf(stderr, "s3eightchime no stroke  natural %d  travel %d  eight %d at %.1f %.1f  cue %d\n",
                     int(natural), tr, sim[kEight].pocket, sim[kEight].x, sim[kEight].y, sim[kCue].fell ? 1 : 0);
        return false;
    }
    for (int i = 0; i < kBalls; i++) sim[i] = ball_[i];
    int again = 0;
    if (roll(sim, bestAim, bestPower, &again) != Lie::Hour || again != bestTravel) {
        std::fprintf(stderr, "s3eightchime stroke did not repeat\n");
        return false;
    }
    for (int i = 0; i < kBalls; i++) sim[i] = ball_[i];
    int softN = 0;
    if (roll(sim, bestAim, 0.14f, &softN) == Lie::Hour) {
        std::fprintf(stderr, "s3eightchime a soft stroke still fell\n");
        return false;
    }
    for (int i = 0; i < kBalls; i++) sim[i] = ball_[i];
    int wideN = 0;
    if (roll(sim, bestAim + 0.9f, bestPower, &wideN) == Lie::Hour) {
        std::fprintf(stderr, "s3eightchime a wide stroke still fell\n");
        return false;
    }
    solvedAim_ = bestAim;
    solvedPower_ = bestPower;
    travel_ = bestTravel;
    solved_ = true;
    spot();
    return true;
}

void Game::toTitle() {
    mode_ = Mode::Title;
    titleFrames_ = 0;
    playFrames_ = 0;
    over_ = false;
    won_ = false;
    eightIn_ = false;
    strokes_ = 0;
    strikes_ = 0;
    chimeFrames_ = 0;
    failFrames_ = 0;
    rollFrames_ = 0;
    lastSec_ = -1;
    bellAmp_ = 0.14f;
    reason_ = "";
    eightPocket_ = -1;
    spot();
    if (solved_) {
        aim_ = solvedAim_;
        power_ = solvedPower_;
        if (!bot_) aim_ = wrap(solvedAim_ + 0.14f);
    } else {
        aim_ = lineAim();
        power_ = 0.48f;
    }
    silenceTicks();
    if (sys_) sys_->setLight(40, 60, 110);
}

void Game::beginAim(bool keepAim) {
    float keptA = aim_, keptP = power_;
    spot();
    mode_ = Mode::Aim;
    rollFrames_ = 0;
    earlyFrames_ = 0;
    missFrames_ = 0;
    if (bot_ && solved_) {
        aim_ = solvedAim_;
        power_ = solvedPower_;
    } else if (keepAim) {
        aim_ = keptA;
        power_ = keptP;
    }
}

void Game::newGame() {
    over_ = false;
    won_ = false;
    eightIn_ = false;
    strokes_ = 0;
    playFrames_ = 0;
    chimeFrames_ = 0;
    failFrames_ = 0;
    strikes_ = 0;
    lastSec_ = -1;
    reason_ = "";
    eightPocket_ = -1;
    beginAim(false);
    if (!bot_) {
        if (solved_) {
            aim_ = wrap(solvedAim_ + 0.14f);
            power_ = solvedPower_;
        } else {
            aim_ = lineAim();
            power_ = 0.48f;
        }
    }
    if (sys_) sys_->setLight(40, 70, 120);
}

void Game::shoot() {
    if (mode_ != Mode::Aim) return;
    for (int i = 0; i < kBalls; i++) {
        ball_[i].vx = ball_[i].vy = 0;
        ball_[i].fell = false;
        ball_[i].pocket = -1;
        ball_[i].down = false;
    }
    float sp = strokeSpeed(power_);
    ball_[kCue].vx = std::cos(aim_) * sp;
    ball_[kCue].vy = std::sin(aim_) * sp;
    strokes_++;
    rollFrames_ = 0;
    mode_ = Mode::Roll;
    if (!sys_) return;
    sys_->apu.noiseBurst(0.22f, 1900.f, 0.04f);
    blip(210.f, 0.05f);
    sys_->rumble(0.12f, 0.24f, 36);
}

void Game::beginChime() {
    if (won_) return;
    won_ = true;
    eightIn_ = true;
    eightPocket_ = ball_[kEight].pocket;
    reason_ = "CHIME";
    mode_ = Mode::Chime;
    chimeFrames_ = 0;
    strikes_ = 0;
    bellAmp_ = 1.f;
    for (int i = 0; i < kBalls; i++) ball_[i].vx = ball_[i].vy = 0;
    if (!sys_) return;
    sys_->setLight(255, 196, 48);
    sys_->rumble(0.45f, 0.8f, 200);
    strikeBell(0);
    strikes_ = 1;
}

void Game::beginEarly() {
    eightIn_ = false;
    ball_[kEight].vx = ball_[kEight].vy = 0;
    if (strokes_ >= kMaxStrokes || pastHour()) {
        beginFail(pastHour() ? "LATE" : "SHORT");
        return;
    }
    reason_ = "SHORT";
    mode_ = Mode::Early;
    earlyFrames_ = 0;
    blip(130.f, 0.06f);
    if (sys_) sys_->setLight(160, 80, 30);
}

void Game::beginMiss(const char* why) {
    eightIn_ = false;
    if (strokes_ >= kMaxStrokes || pastHour()) {
        beginFail(pastHour() ? "LATE" : (why && why[0] ? why : "OPEN"));
        return;
    }
    reason_ = why && why[0] ? why : "OPEN";
    mode_ = Mode::Miss;
    missFrames_ = 0;
    blip(96.f, 0.05f);
    if (sys_) sys_->setLight(140, 50, 30);
}

void Game::beginFail(const char* why) {
    reason_ = why && why[0] ? why : "OPEN";
    won_ = false;
    eightIn_ = false;
    mode_ = Mode::Fail;
    failFrames_ = 0;
    blip(78.f, 0.07f);
    if (sys_) sys_->setLight(140, 24, 24);
}

void Game::resolve() {
    for (int i = 0; i < kBalls; i++) ball_[i].vx = ball_[i].vy = 0;
    if (ball_[kCue].fell) {
        beginMiss("SCRATCH");
        return;
    }
    if (ball_[kEight].fell && ball_[kEight].pocket == kHourPocket) {
        if (onHour()) beginChime();
        else if (pastHour()) beginFail("LATE");
        else beginEarly();
        return;
    }
    if (ball_[kEight].fell) beginMiss("WIDE");
    else if (pastHour()) beginFail("LATE");
    else beginMiss("OPEN");
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = true;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(1, 1, 2));
    sys.apu.setMaster(0.72f);
    sys.apu.setEcho(0.16f, 0.26f, 0.14f);
    sys.apu.setPatch(0, bellPatch());
    sys.apu.setPan(0, 0.04f);
    solved_ = prove();
    if (!solved_) std::fprintf(stderr, "s3eightchime rules failed\n");
    toTitle();
}

void Game::blip(float freq, float vol) {
    if (!sys_) return;
    sys_->apu.tone(0, freq, vol);
    tickLeft_ = 6;
}

void Game::strikeBell(int n) {
    if (!sys_) return;
    static const float peal[] = {523.25f, 659.25f, 783.99f, 1046.5f};
    sys_->apu.keyOn(0, peal[n % 4]);
    sys_->apu.tone(1, peal[n % 4] * 0.5f, 0.045f);
    tickLeft_ = 8;
    bellAmp_ = 1.f;
}

void Game::silenceTicks() {
    if (!sys_) return;
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(1, 0, 0);
    sys_->apu.tone(2, 0, 0);
}

void Game::tickClock() {
    int sec = secFrom(playFrames_);
    if (mode_ == Mode::Title) sec = kStartSec;
    if (sec == lastSec_) return;
    int prev = lastSec_;
    lastSec_ = sec;
    if (prev < 0) return;
    if (mode_ == Mode::Chime || mode_ == Mode::Over || mode_ == Mode::Fail) return;
    int until = untilFrom(playFrames_);
    if (mode_ != Mode::Title && until > 0 && until <= kFpc * 12) blip((sec & 1) ? 880.f : 660.f, 0.045f);
    else blip(196.f, 0.03f);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool shadow) {
    if (!sys_ || h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::max(1, std::min(2000, (int)std::lround(w))));
    s.h = int16_t(std::max(1, std::min(2000, (int)std::lround(h))));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 48 || s.x + s.w < -48 || s.y > gs::SCREEN_H + 48 || s.y + s.h < -48) return;
    s.img = m.pick(float(s.h));
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::image(const gs::Image& img, float cx, float cy, float h, int pal) {
    if (!sys_ || h < 1.f || img.h == 0) return;
    float w = h * float(img.w) / float(img.h);
    gs::Sprite s;
    s.w = int16_t(std::max(1, std::min(2000, (int)std::lround(w))));
    s.h = int16_t(std::max(1, std::min(2000, (int)std::lround(h))));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = img;
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!sys_ || !s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        if (x < 0 || x > 39) continue;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (c < 32 || c > 127) c = ' ';
        int tile = art_.font[c - 32];
        if (!tile) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(tile, pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    if (!s) return;
    int n = int(std::strlen(s));
    hud((40 - n) / 2, row, s, pal);
}

void Game::backdrop() {
    if (!sys_) return;
    bool noon = mode_ == Mode::Chime || (mode_ == Mode::Over && won_);
    bool dead = mode_ == Mode::Fail || (mode_ == Mode::Over && !won_ && reason_[0]);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        sys_->vdp.road[y].on = false;
        sys_->vdp.lineFog[y] = 0;
        float u = std::clamp(y / 56.f, 0.f, 1.f);
        if (noon) sys_->vdp.lineBackdrop[y] = u < 0.5f ? gs::rgb4(6, 4, 2) : gs::rgb4(10, 7, 3);
        else if (dead) sys_->vdp.lineBackdrop[y] = u < 0.5f ? gs::rgb4(3, 1, 2) : gs::rgb4(5, 2, 2);
        else sys_->vdp.lineBackdrop[y] = u < 0.45f ? gs::rgb4(1, 1, 3) : gs::rgb4(3, 2, 5);
    }
}

void Game::aimAid() {
    bool show = mode_ == Mode::Title || mode_ == Mode::Aim || (mode_ == Mode::Pause && held_ == Mode::Aim);
    if (!show || ball_[kCue].down) return;
    float dx = std::cos(aim_), dy = std::sin(aim_);
    int hit = -1;
    float tHit = 1e9f;
    const float rad = kR * 2.f;
    for (int i = 1; i < kBalls; i++) {
        if (ball_[i].down) continue;
        float cx = ball_[i].x - ball_[kCue].x;
        float cy = ball_[i].y - ball_[kCue].y;
        float tc = cx * dx + cy * dy;
        if (tc <= 0.f) continue;
        float perp2 = cx * cx + cy * cy - tc * tc;
        if (perp2 > rad * rad) continue;
        float th = tc - std::sqrt(std::max(0.f, rad * rad - perp2));
        if (th < 4.f || th >= tHit) continue;
        tHit = th;
        hit = i;
    }
    float limit = hit > 0 ? tHit : 64.f;
    for (int i = 1; i <= 6; i++) {
        float t = float(i) * 11.f;
        if (t > limit - 4.f) break;
        spr(art_.dot, ball_[kCue].x + dx * t, ball_[kCue].y + dy * t, 4.f, PAL_GOLD);
    }
    if (hit != kEight) return;
    float hx = ball_[kCue].x + dx * tHit;
    float hy = ball_[kCue].y + dy * tHit;
    float nx = ball_[kEight].x - hx, ny = ball_[kEight].y - hy;
    float nd = std::hypot(nx, ny);
    if (nd < 0.2f) return;
    nx /= nd;
    ny /= nd;
    for (int i = 1; i <= 5; i++)
        spr(art_.dot, ball_[kEight].x + nx * float(i) * 12.f, ball_[kEight].y + ny * float(i) * 12.f, 4.f, PAL_WIN);
}

void Game::cueDraw() {
    if (mode_ == Mode::Roll || mode_ == Mode::Chime || mode_ == Mode::Fail || mode_ == Mode::Over) return;
    if (ball_[kCue].down) return;
    int qi = int(std::lround(aim_ / kTau * float(kCueAngles)));
    qi %= kCueAngles;
    if (qi < 0) qi += kCueAngles;
    float q = float(qi) * (kTau / float(kCueAngles));
    float back = kTip + kR + 6.f + std::max(0.f, power_ - 0.2f) * 14.f;
    float cx = ball_[kCue].x - std::cos(q) * back;
    float cy = ball_[kCue].y - std::sin(q) * back;
    spr(art_.cue[qi], cx, cy, float(kCueBox) * 0.82f, PAL_CUE);
}

void Game::clockAt(float cx, float cy, float size) {
    int h, m, s;
    faceTime(h, m, s);
    int hi = ((h % 12) * 5 + m / 12) % 60;
    bool noon = mode_ == Mode::Chime || (mode_ == Mode::Over && won_) || (mode_ != Mode::Title && onHour());
    float swing = std::sin(bellPh_) * 5.f * bellAmp_;
    spr(art_.bell, cx - 30.f + swing, cy + 1.f, 16.f, PAL_BELL);
    spr(art_.bell, cx + 30.f - swing, cy + 1.f, 16.f, PAL_BELL, true);
    image(art_.cap, cx, cy, size * 0.18f, PAL_HAND);
    image(art_.hand[2][s % 60], cx, cy, size, PAL_HAND);
    image(art_.hand[1][m % 60], cx, cy, size, PAL_HAND);
    image(art_.hand[0][hi], cx, cy, size, PAL_HAND);
    if (noon) image(art_.bezel, cx, cy, size * 1.06f, PAL_GOLD);
    else image(art_.bezel, cx, cy, size * 1.04f, PAL_FACE);
    image(art_.face, cx, cy, size, PAL_FACE);
    spr(art_.plaque, cx, cy + 2.f, size * 1.32f, PAL_BELL);
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = true;
    v.B.enabled = false;
    v.hudEnabled = true;
    backdrop();

    bool showEightIn = ball_[kEight].down && ball_[kEight].fell;
    cueDraw();
    aimAid();
    for (int i = 0; i < kBalls; i++) {
        if (ball_[i].down) continue;
        spr(art_.ball[i], ball_[i].x, ball_[i].y, kR * 2.f, PAL_BALL);
    }
    for (int i = 0; i < kBalls; i++) {
        if (ball_[i].down) continue;
        spr(art_.shadow, ball_[i].x + 2.2f, ball_[i].y + 3.f, 7.f, PAL_BALL, false, true);
    }
    if (showEightIn) {
        Pocket pk = pocketAt(ball_[kEight].pocket >= 0 ? ball_[kEight].pocket : kHourPocket);
        spr(art_.ball[kEight], pk.x, pk.y + 1.f, 11.f, PAL_BALL);
    }

    Pocket hour = pocketAt(kHourPocket);
    int until = untilFrom(playFrames_);
    bool hot = mode_ != Mode::Title && (onHour() || (until > 0 && until <= kFpc * 10));
    float pulse = hot ? 18.f + std::sin(bellPh_ * 3.f) * 1.6f : 16.f;
    spr(art_.ring, hour.x, hour.y, pulse, (mode_ == Mode::Fail) ? PAL_ALERT : PAL_GLOW);

    if (mode_ == Mode::Chime || (mode_ == Mode::Over && won_)) {
        for (int i = 0; i < 6; i++) {
            float a = bellPh_ * 1.4f + float(i) * 1.047f;
            float rad = 18.f + (1.f - std::min(bellAmp_, 1.f)) * 10.f;
            spr(art_.dot, 160.f + std::cos(a) * rad, 28.f + std::sin(a) * rad * 0.45f, 4.f, PAL_GOLD);
        }
    }
    clockAt(160.f, 26.f, float(kDial));

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(22, "S3 EIGHTCHIME", PAL_GOLD);
        hudC(24, "POCKET THE 8 AS THE HOUR CHIMES", PAL_HUD);
        hudC(25, "EARLY DOES NOT COUNT", PAL_GOLD);
        if ((titleFrames_ / 30) & 1) hudC(27, "PRESS START", PAL_GOLD);
        else hudC(27, "ARROWS AIM    Z SHOOT", PAL_HUD);
        return;
    }

    int h, m, s;
    split(h, m, s);
    std::snprintf(buf, sizeof buf, "%d:%02d:%02d", h, m, s);
    bool chiming = mode_ == Mode::Chime || (mode_ == Mode::Over && won_);
    hud(1, 0, buf, (hot || chiming) ? PAL_GOLD : PAL_HUD);
    int showStroke = std::min(std::max(strokes_, mode_ == Mode::Aim ? strokes_ + 1 : strokes_), kMaxStrokes);
    if (chiming) showStroke = strokes_;
    std::snprintf(buf, sizeof buf, "STROKE %d/%d", showStroke, kMaxStrokes);
    hud(28, 0, buf, strokes_ >= kMaxStrokes - 1 && !chiming ? PAL_ALERT : PAL_GOLD);

    if (mode_ == Mode::Pause) {
        hudC(24, "PAUSED", PAL_GOLD);
        hudC(26, "THE CLOCK STILL RUNS", PAL_HUD);
        hudC(27, "START RESUMES   ESC TITLE", PAL_GOLD);
        return;
    }
    if (chiming) {
        hudC(12, "THE HOUR CHIMES", PAL_GOLD);
        std::snprintf(buf, sizeof buf, "STROKE %d", strokes_);
        hudC(14, buf, PAL_WIN);
        if (mode_ == Mode::Over && !bot_) hudC(27, "START AGAIN", PAL_GOLD);
        return;
    }
    if (mode_ == Mode::Fail || (mode_ == Mode::Over && !won_)) {
        bool late = pastHour() || std::strcmp(reason_, "LATE") == 0;
        hudC(12, late ? "THE HOUR PASSED" : "NO CHIME", PAL_ALERT);
        hudC(14, reason_[0] ? reason_ : "OPEN", PAL_HUD);
        if (!bot_) hudC(27, "START AGAIN", PAL_GOLD);
        return;
    }
    if (mode_ == Mode::Early) {
        hudC(12, "SHORT OF THE HOUR", PAL_ALERT);
        hudC(14, "THE POCKET GIVES IT BACK", PAL_GOLD);
        return;
    }
    if (mode_ == Mode::Miss) {
        hudC(12, reason_[0] ? reason_ : "OPEN", PAL_ALERT);
        hudC(14, strokes_ >= 2 ? "ONE STROKE LEFT" : "STILL SHORT OF THE HOUR", PAL_GOLD);
        return;
    }

    if (until > 0) {
        int show = (until + kFpc - 1) / kFpc;
        std::snprintf(buf, sizeof buf, "HOUR IN %d", show);
        hud(1, 1, buf, hot ? PAL_GOLD : PAL_HUD);
    } else if (onHour()) {
        hud(1, 1, "ON THE HOUR", PAL_GOLD);
    } else {
        hud(1, 1, "TOO LATE", PAL_ALERT);
    }
    hud(30, 1, "HOUR", PAL_GOLD);

    if (mode_ == Mode::Roll) {
        hud(1, 26, "ROLLING", PAL_GOLD);
        return;
    }
    int bars = std::clamp(int(std::lround(power_ * 8.f)), 1, 8);
    const char* zone = power_ < 0.34f ? "SOFT" : power_ > 0.72f ? "HARD" : "FIRM";
    int zpal = (power_ < 0.34f || power_ > 0.72f) ? PAL_ALERT : PAL_GOLD;
    std::snprintf(buf, sizeof buf, "%s ", zone);
    int n = int(std::strlen(buf));
    for (int i = 0; i < 8; i++) buf[n++] = i < bars ? '#' : '-';
    buf[n] = 0;
    hud(1, 26, buf, zpal);
    hud(28, 26, "Z SHOOT", PAL_HUD);
    hudC(27, "LINE THE 8 ON THE GOLD POCKET", PAL_HUD);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    bellPh_ += kDt * ((mode_ == Mode::Chime || (mode_ == Mode::Over && won_)) ? 11.f : 1.7f);
    if (mode_ == Mode::Chime || (mode_ == Mode::Over && won_)) bellAmp_ = std::max(0.4f, bellAmp_ - kDt * 0.28f);
    else if (mode_ != Mode::Chime) bellAmp_ = std::max(0.12f, bellAmp_ - kDt * 0.4f);

    const gs::Pad& pad = sys.pad;
    bool action = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_TURBO);
    bool start = pad.pressed(gs::BTN_START);
    bool back = pad.pressed(gs::BTN_MODE);
    if (bot_) {
        start = false;
        back = false;
        action = false;
    }

    if (mode_ == Mode::Title) {
        titleFrames_++;
        if (bot_) {
            if (titleFrames_ >= 36) newGame();
        } else if (back) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        } else if (start || action) {
            newGame();
        }
    } else if (mode_ == Mode::Pause) {
        playFrames_++;
        if (start) mode_ = held_;
        else if (back) toTitle();
    } else if (mode_ == Mode::Chime) {
        chimeFrames_++;
        if (strikes_ < 12 && (chimeFrames_ % 5) == 0) {
            strikeBell(strikes_);
            strikes_++;
        }
        if (chimeFrames_ >= 12 * 5 + 30) {
            mode_ = Mode::Over;
            over_ = true;
            won_ = true;
            sys.apu.keyOff(0);
            silenceTicks();
        }
    } else if (mode_ == Mode::Fail) {
        if (++failFrames_ >= 72) {
            mode_ = Mode::Over;
            over_ = true;
        }
    } else if (mode_ == Mode::Over) {
        if (!bot_ && (start || action)) newGame();
        else if (!bot_ && back) toTitle();
    } else {
        playFrames_++;
        if (!bot_ && back && mode_ != Mode::Roll) {
            toTitle();
        } else if (!bot_ && start && mode_ != Mode::Early && mode_ != Mode::Miss) {
            held_ = mode_;
            mode_ = Mode::Pause;
        } else if (mode_ == Mode::Aim) {
            if (pastHour()) {
                beginFail("LATE");
            } else if (bot_) {
                if (solved_) {
                    aim_ = solvedAim_;
                    power_ = solvedPower_;
                }
                if (solved_ && shouldShoot(playFrames_, travel_)) shoot();
            } else {
                float rate = (pad.down(gs::BTN_Y) || pad.down(gs::BTN_Z)) ? 0.38f : 1.25f;
                float x = 0.f, y = 0.f;
                if (pad.down(gs::BTN_LEFT)) x -= 1.f;
                if (pad.down(gs::BTN_RIGHT)) x += 1.f;
                if (pad.down(gs::BTN_UP)) y -= 1.f;
                if (pad.down(gs::BTN_DOWN)) y += 1.f;
                if (std::fabs(pad.axisX) > 0.22f) x = pad.axisX;
                if (std::fabs(pad.axisY) > 0.22f) y = -pad.axisY;
                aim_ = wrap(aim_ + x * rate * kDt);
                power_ = std::clamp(power_ - y * 0.42f * kDt, 0.16f, 0.92f);
                if (action) shoot();
            }
        } else if (mode_ == Mode::Roll) {
            rollFrames_++;
            bool clack = false;
            step(ball_, &clack);
            if (ball_[kEight].fell || ball_[kCue].fell) {
                sys.apu.noise(0, 1000);
                blip(170.f, 0.06f);
                sys.rumble(0.16f, 0.32f, 40);
                resolve();
            } else if (clack) {
                sys.apu.noiseBurst(0.1f, 1400.f, 0.03f);
            } else if (!moving(ball_) || rollFrames_ > 260) {
                sys.apu.noise(0, 1000);
                resolve();
            } else {
                sys.apu.noise(0.02f, 900.f, true);
            }
        } else if (mode_ == Mode::Early) {
            if (++earlyFrames_ > 42) {
                if (pastHour() || strokes_ >= kMaxStrokes) beginFail(pastHour() ? "LATE" : "SHORT");
                else beginAim(true);
            }
        } else if (mode_ == Mode::Miss) {
            if (++missFrames_ > 42) {
                if (pastHour() || strokes_ >= kMaxStrokes) beginFail(pastHour() ? "LATE" : "OPEN");
                else beginAim(true);
            }
        }
    }

    if (mode_ != Mode::Roll && sys_) sys_->apu.noise(0, 1000);
    if (tickLeft_ > 0 && mode_ != Mode::Chime) {
        if (--tickLeft_ == 0) silenceTicks();
    }
    tickClock();

    if (mode_ == Mode::Aim || mode_ == Mode::Roll || mode_ == Mode::Pause) {
        int until = untilFrom(playFrames_);
        if (onHour()) sys.setLight(240, 190, 50);
        else if (until > 0 && until < kFpc * 12) sys.setLight(180, 130, 40);
        else sys.setLight(36, 70, 120);
    }
    draw();
}

}  // namespace eightchime

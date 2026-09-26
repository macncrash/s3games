#include "game/eightmark.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace eightmark {
namespace {

constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;
constexpr float kDt = 1.f / 60.f;
constexpr float kMu = 190.f;
constexpr float kVMax = 620.f;
constexpr float kRest = 0.72f;
constexpr int kSub = 8;
constexpr float kStop = 14.f;
constexpr float kMarkR = 11.f;
constexpr float kLiftR = 14.f;
constexpr float kCoinV = 165.f;
constexpr float kHandV = 155.f;
constexpr float kPlaceV = 170.f;
constexpr int kMaxShots = 6;

float wrap(float a) {
    while (a > kPi) a -= kTau;
    while (a < -kPi) a += kTau;
    return a;
}

void cloth(float& x, float& y) {
    x = std::clamp(x, kFeltL + 8.f, kFeltR - 8.f);
    y = std::clamp(y, kFeltT + 8.f, kFeltB - 8.f);
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

}  // namespace

const char* Game::pocketName() const { return pocketAt(eightPocket_).name; }

void Game::stepBodies(Body* b, float dt, bool* clack) {
    auto sink = [](Body& ball, int pocket) {
        if (ball.down) return;
        ball.down = true;
        ball.fell = true;
        ball.pocket = pocket;
        ball.vx = ball.vy = 0;
    };
    const float h = dt / float(kSub);
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
                if (d < pk.sink + 5.f && d > 0.8f) {
                    float nx = dx / d, ny = dy / d;
                    if (ball.vx * nx + ball.vy * ny > 24.f) {
                        ball.vx += nx * 140.f * h;
                        ball.vy += ny * 140.f * h;
                    }
                }
            }
            ball.x += ball.vx * h;
            ball.y += ball.vy * h;
            damp(ball.vx, ball.vy, h);
            capSpeed(ball.vx, ball.vy);
        }
        for (int pass = 0; pass < 4; pass++) {
            for (int i = 0; i < kBalls; i++) {
                if (b[i].down) continue;
                for (int j = i + 1; j < kBalls; j++) {
                    if (b[j].down) continue;
                    float dx = b[j].x - b[i].x;
                    float dy = b[j].y - b[i].y;
                    float d2 = dx * dx + dy * dy;
                    const float minD = kR * 2.f;
                    if (d2 >= minD * minD) continue;
                    float d = std::sqrt(std::max(d2, 1e-8f));
                    float nx = dx / d, ny = dy / d;
                    float rel = (b[i].vx - b[j].vx) * nx + (b[i].vy - b[j].vy) * ny;
                    if (rel > 0.f) {
                        b[i].vx -= rel * nx;
                        b[i].vy -= rel * ny;
                        b[j].vx += rel * nx;
                        b[j].vy += rel * ny;
                        if (clack && rel > 48.f) *clack = true;
                    }
                    float push = (minD - d) * 0.5f;
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

void Game::absorb(Body* b) {
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
        if (best >= 0) {
            b[i].down = true;
            b[i].fell = true;
            b[i].pocket = best;
            b[i].vx = b[i].vy = 0;
        } else {
            b[i].vx = b[i].vy = 0;
        }
    }
}

bool Game::moving(const Body* b, float v) {
    for (int i = 0; i < kBalls; i++) {
        if (b[i].down) continue;
        if (std::hypot(b[i].vx, b[i].vy) > v) return true;
    }
    return false;
}

bool Game::inCloth(float x, float y) const {
    if (x < kFeltL + kR + 1.f || x > kFeltR - kR - 1.f) return false;
    if (y < kFeltT + kR + 1.f || y > kFeltB - kR - 1.f) return false;
    for (int i = 0; i < 6; i++) {
        Pocket p = pocketAt(i);
        if (std::hypot(x - p.x, y - p.y) < p.sink + 1.f) return false;
    }
    return true;
}

bool Game::freePoint(float x, float y, int ignore) const {
    if (!inCloth(x, y)) return false;
    const float need = kR * 2.f + 0.6f;
    for (int i = 0; i < kBalls; i++) {
        if (i == ignore || ball_[i].down) continue;
        if (std::hypot(ball_[i].x - x, ball_[i].y - y) < need) return false;
    }
    return true;
}

bool Game::pathClear(float x0, float y0, float x1, float y1, int ignoreA, int ignoreB) const {
    const float need = kR * 2.f - 0.4f;
    for (int i = 0; i < kBalls; i++) {
        if (i == ignoreA || i == ignoreB || ball_[i].down) continue;
        if (segDist(ball_[i].x, ball_[i].y, x0, y0, x1, y1) < need) return false;
    }
    return true;
}

bool Game::playOut(Body* b, float aim, float power, int want) const {
    for (int i = 0; i < kBalls; i++) {
        b[i].vx = b[i].vy = 0;
        b[i].fell = false;
        b[i].pocket = -1;
    }
    float sp = std::clamp(power, 0.18f, 1.f) * kVMax;
    b[0].down = false;
    b[0].vx = std::cos(aim) * sp;
    b[0].vy = std::sin(aim) * sp;
    for (int f = 0; f < 280; f++) {
        stepBodies(b, kDt, nullptr);
        if (b[1].fell && b[0].fell) break;
        if (!moving(b, kStop)) break;
    }
    absorb(b);
    return b[1].fell && b[1].pocket == want && !b[0].fell;
}

Game::Shot Game::solve() const {
    Shot best;
    if (ball_[0].down || ball_[1].down) return best;
    const float powers[5] = {0.42f, 0.50f, 0.58f, 0.36f, 0.68f};
    const int pockets[6] = {1, 2, 5, 4, 0, 3};
    const int offsets[9] = {0, -1, 1, -2, 2, -3, 3, -4, 4};
    const float ex = ball_[1].x, ey = ball_[1].y;
    const float cx = ball_[0].x, cy = ball_[0].y;
    for (int p : pockets) {
        Pocket pk = pocketAt(p);
        float dx = pk.x - ex, dy = pk.y - ey;
        float dist = std::hypot(dx, dy);
        if (dist < 12.f) continue;
        float dirx = dx / dist, diry = dy / dist;
        float gx = ex - dirx * (kR * 2.f + 0.35f);
        float gy = ey - diry * (kR * 2.f + 0.35f);
        if (!pathClear(cx, cy, gx, gy, 0, 1)) continue;
        if (!pathClear(ex, ey, pk.x, pk.y, 1, 0)) continue;
        float base = std::atan2(gy - cy, gx - cx);
        for (int ia : offsets) {
            float aim = base + float(ia) * 0.02f;
            for (float power : powers) {
                Body sim[kBalls];
                for (int i = 0; i < kBalls; i++) sim[i] = ball_[i];
                if (!playOut(sim, aim, power, p)) continue;
                best.ok = true;
                best.aim = aim;
                best.power = power;
                best.pocket = p;
                return best;
            }
        }
    }
    return best;
}

void Game::place() {
    for (int i = 0; i < kBalls; i++) {
        Home h = homeAt(i);
        ball_[i] = Body{};
        ball_[i].x = h.x;
        ball_[i].y = h.y;
    }
    coinX_ = kCoinX;
    coinY_ = kCoinY;
    handX_ = kHandX;
    handY_ = kHandY;
    aim_ = std::atan2(kEightY - kCueY, kEightX - kCueX);
    power_ = 0.42f;
    called_ = 1;
    eightPocket_ = 1;
    shots_ = 0;
    sayT_ = rollT_ = openT_ = fanT_ = 0;
    fanStep_ = -1;
    say_ = "";
    reason_ = "OPEN";
    marked_ = opened_ = lifted_ = onBall_ = solved_ = pocketSnd_ = false;
    over_ = won_ = false;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.72f);
    sys.apu.setEcho(0.14f, 0.24f, 0.15f);
    place();
    mode_ = Mode::Title;
    t_ = 0;
}

void Game::begin() {
    place();
    mode_ = Mode::Mark;
    say("SET THE COIN", 1.1f);
}

void Game::toTitle() {
    place();
    mode_ = Mode::Title;
    if (sys_) {
        sys_->apu.tone(0, 0, 0);
        sys_->apu.tone(1, 0, 0);
        sys_->apu.tone(2, 0, 0);
    }
}

void Game::say(const char* s, float time) {
    say_ = s;
    sayT_ = time;
}

void Game::blip(float freq) {
    if (!sys_) return;
    sys_->apu.tone(0, freq, 0.07f);
    beep_ = std::max(beep_, 0.08f);
}

void Game::chord(float a, float b, float c, float hold) {
    if (!sys_) return;
    sys_->apu.tone(0, a, 0.09f);
    sys_->apu.tone(1, b, 0.07f);
    sys_->apu.tone(2, c, 0.06f);
    beep_ = hold;
}

void Game::unmark() {
    marked_ = false;
    onBall_ = false;
    solved_ = false;
    coinX_ = kCoinX;
    coinY_ = kCoinY;
}

void Game::dropCue(float x, float y) {
    ball_[0].down = false;
    ball_[0].fell = false;
    ball_[0].pocket = -1;
    ball_[0].vx = ball_[0].vy = 0;
    if (freePoint(x, y, 0)) {
        ball_[0].x = x;
        ball_[0].y = y;
        return;
    }
    for (int ring = 1; ring <= 8; ring++) {
        for (int s = 0; s < 10; s++) {
            float a = float(s) * (kTau / 10.f);
            float rad = float(ring) * (kR * 2.f + 1.6f);
            float px = x + std::cos(a) * rad;
            float py = y + std::sin(a) * rad;
            if (!freePoint(px, py, 0)) continue;
            ball_[0].x = px;
            ball_[0].y = py;
            return;
        }
    }
    ball_[0].x = kCueX;
    ball_[0].y = kCueY;
}

void Game::respotEight() {
    Body& b = ball_[1];
    b.down = false;
    b.fell = false;
    b.pocket = -1;
    b.vx = b.vy = 0;
    if (freePoint(kEightX, kEightY, 1)) {
        b.x = kEightX;
        b.y = kEightY;
        return;
    }
    for (int ring = 1; ring <= 8; ring++) {
        for (int s = 0; s < 10; s++) {
            float a = float(s) * (kTau / 10.f) + float(ring) * 0.2f;
            float rad = float(ring) * (kR * 2.f + 1.5f);
            float x = kEightX + std::cos(a) * rad;
            float y = kEightY + std::sin(a) * rad;
            if (!freePoint(x, y, 1)) continue;
            b.x = x;
            b.y = y;
            return;
        }
    }
    b.x = kEightX;
    b.y = kEightY;
}

void Game::tryMark() {
    float d = std::hypot(coinX_ - ball_[1].x, coinY_ - ball_[1].y);
    if (ball_[1].down || d > kMarkR) {
        say("NOT ON THE EIGHT", 0.6f);
        blip(110.f);
        return;
    }
    coinX_ = ball_[1].x;
    coinY_ = ball_[1].y;
    marked_ = true;
    solved_ = false;
    onBall_ = true;
    mode_ = Mode::Aim;
    blip(523.f);
    say("MARK SET", 0.7f);
}

void Game::shoot() {
    if (ball_[0].down || !marked_) return;
    for (int i = 0; i < kBalls; i++) {
        ball_[i].fell = false;
        ball_[i].pocket = -1;
    }
    float sp = std::clamp(power_, 0.18f, 1.f) * kVMax;
    ball_[0].down = false;
    ball_[0].vx = std::cos(aim_) * sp;
    ball_[0].vy = std::sin(aim_) * sp;
    shots_++;
    rollT_ = 0;
    pocketSnd_ = false;
    mode_ = Mode::Roll;
    if (sys_) {
        sys_->apu.noiseBurst(0.28f, 2200.f, 0.04f);
        sys_->rumble(0.12f, 0.28f, 30);
    }
}

void Game::coinOnLip(int pocket) {
    Pocket p = pocketAt(pocket);
    float dx = kCx - p.x, dy = kCy - p.y;
    float d = std::hypot(dx, dy);
    if (d < 1.f) d = 1.f;
    coinX_ = p.x + dx / d * 22.f;
    coinY_ = p.y + dy / d * 22.f;
    cloth(coinX_, coinY_);
}

void Game::openMark(int pocket) {
    opened_ = true;
    eightPocket_ = pocket;
    coinOnLip(pocket);
    mode_ = Mode::Open;
    openT_ = 0.5f;
    chord(392.f, 523.25f, 659.25f, 0.45f);
    if (sys_) {
        sys_->rumble(0.28f, 0.5f, 120);
        sys_->setLight(80, 170, 70);
    }
    say("MARK OPEN", 0.5f);
}

void Game::beginLift() {
    handX_ = kHandX;
    handY_ = kHandY;
    mode_ = Mode::Lift;
    say("LIFT THE COIN", 0.8f);
}

void Game::tryLift() {
    if (std::hypot(handX_ - coinX_, handY_ - coinY_) > kLiftR) {
        say("WALK TO THE COIN", 0.45f);
        blip(120.f);
        return;
    }
    handX_ = coinX_;
    handY_ = coinY_;
    finish();
}

void Game::finish() {
    if (!opened_) return;
    lifted_ = true;
    won_ = true;
    over_ = true;
    mode_ = Mode::Win;
    fanT_ = 0;
    fanStep_ = -1;
    reason_ = "FINISHED MARK";
    say("FINISHED MARK", 4.f);
    if (sys_) {
        sys_->rumble(0.4f, 0.8f, 180);
        sys_->setLight(255, 200, 60);
    }
}

void Game::fail(const char* why) {
    won_ = false;
    lifted_ = false;
    over_ = true;
    mode_ = Mode::Lose;
    reason_ = why;
    say(why, 4.f);
    if (sys_) {
        sys_->apu.tone(0, 90.f, 0.1f);
        sys_->setLight(140, 30, 24);
    }
    beep_ = 0.4f;
}

void Game::resolve() {
    const bool scratch = ball_[0].fell;
    const bool eightFell = ball_[1].fell;
    const int pk = ball_[1].pocket;
    if (eightFell && pk == called_ && !scratch && marked_) {
        openMark(pk);
        return;
    }
    if (eightFell) {
        say(scratch && pk == called_ ? "SCRATCH ON THE 8" : "WRONG POCKET", 1.f);
        respotEight();
        dropCue(kCueX, kCueY);
        unmark();
        mode_ = Mode::Mark;
        blip(90.f);
    } else if (scratch) {
        dropCue(kCueX, kCueY);
        solved_ = false;
        mode_ = Mode::Place;
        say("SCRATCH", 0.8f);
        blip(98.f);
    } else {
        solved_ = false;
        mode_ = Mode::Aim;
        say("MISS", 0.6f);
    }
    if (mode_ != Mode::Open && shots_ >= kMaxShots) fail("NO MARK");
}

void Game::follow() {
    if (marked_ && !opened_ && !ball_[1].down) {
        coinX_ = ball_[1].x;
        coinY_ = ball_[1].y;
    }
}

Game::Input Game::readPad(const gs::Pad& pad) const {
    Input in;
    in.action = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_TURBO);
    in.start = pad.pressed(gs::BTN_START);
    in.back = pad.pressed(gs::BTN_MODE);
    in.call = pad.pressed(gs::BTN_X);
    if (pad.down(gs::BTN_LEFT)) in.x -= 1.f;
    if (pad.down(gs::BTN_RIGHT)) in.x += 1.f;
    if (pad.down(gs::BTN_UP)) in.y -= 1.f;
    if (pad.down(gs::BTN_DOWN)) in.y += 1.f;
    if (std::fabs(pad.axisX) > 0.2f) in.x = pad.axisX;
    if (std::fabs(pad.axisY) > 0.2f) in.y = -pad.axisY;
    float len = std::hypot(in.x, in.y);
    if (len > 1.f) {
        in.x /= len;
        in.y /= len;
    }
    return in;
}

Game::Input Game::botInput() const {
    Input in;
    if (mode_ == Mode::Title && t_ > 0.45f) {
        in.start = true;
        return in;
    }
    if (mode_ == Mode::Mark) {
        float dx = ball_[1].x - coinX_;
        float dy = ball_[1].y - coinY_;
        float d = std::hypot(dx, dy);
        if (d < 6.f) in.action = true;
        else if (d > 1e-3f) {
            in.x = dx / d;
            in.y = dy / d;
        }
        return in;
    }
    if (mode_ == Mode::Lift) {
        float dx = coinX_ - handX_;
        float dy = coinY_ - handY_;
        float d = std::hypot(dx, dy);
        if (d < 8.f) in.action = true;
        else if (d > 1e-3f) {
            in.x = dx / d;
            in.y = dy / d;
        }
    }
    return in;
}

void Game::fanfare() {
    if (!sys_) return;
    static const float notes[] = {523.25f, 659.25f, 783.99f, 1046.5f};
    int step = int(fanT_ / 0.13f);
    if (step != fanStep_ && step >= 0 && step < 4) {
        sys_->apu.tone(0, notes[step], 0.12f);
        sys_->apu.tone(1, notes[step] * 0.5f, 0.06f);
        fanStep_ = step;
    } else if (step >= 8 && fanStep_ != 99) {
        sys_->apu.tone(0, 0, 0);
        sys_->apu.tone(1, 0, 0);
        fanStep_ = 99;
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    if (sayT_ > 0) sayT_ = std::max(0.f, sayT_ - kDt);
    if (beep_ > 0 && mode_ != Mode::Win) {
        beep_ -= kDt;
        if (beep_ <= 0) {
            sys.apu.tone(0, 0, 0);
            sys.apu.tone(1, 0, 0);
            sys.apu.tone(2, 0, 0);
        }
    }

    Input in = bot_ ? botInput() : readPad(sys.pad);
    const bool fine = sys.pad.down(gs::BTN_Y) || sys.pad.down(gs::BTN_Z);

    if (mode_ == Mode::Title) {
        if (!bot_ && in.back) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        } else if (in.start || in.action) begin();
    } else if (mode_ == Mode::Pause) {
        if (in.start) mode_ = held_;
        else if (in.back) toTitle();
    } else if (mode_ == Mode::Win || mode_ == Mode::Lose) {
        if (mode_ == Mode::Win) {
            fanT_ += kDt;
            fanfare();
        }
        if (!bot_ && (in.start || in.action)) begin();
        else if (!bot_ && in.back) toTitle();
    } else if (!bot_ && in.start) {
        held_ = mode_;
        mode_ = Mode::Pause;
    } else if (!bot_ && in.back) {
        toTitle();
    } else {
        if (!bot_ && in.call && (mode_ == Mode::Mark || mode_ == Mode::Aim || mode_ == Mode::Place)) {
            called_ = (called_ + 1) % 6;
            eightPocket_ = called_;
            blip(640.f);
            say(pocketAt(called_).name, 0.6f);
        }
        if (mode_ == Mode::Mark) {
            coinX_ += in.x * kCoinV * kDt;
            coinY_ += in.y * kCoinV * kDt;
            cloth(coinX_, coinY_);
            bool on = !ball_[1].down && std::hypot(coinX_ - ball_[1].x, coinY_ - ball_[1].y) <= kMarkR;
            if (on && !onBall_) blip(740.f);
            onBall_ = on;
            if (in.action) tryMark();
        } else if (mode_ == Mode::Place) {
            if (bot_) {
                if (!freePoint(ball_[0].x, ball_[0].y, 0)) dropCue(kCueX, kCueY);
                solved_ = false;
                mode_ = Mode::Aim;
            } else {
                float nx = ball_[0].x + in.x * kPlaceV * kDt;
                float ny = ball_[0].y + in.y * kPlaceV * kDt;
                if (freePoint(nx, ny, 0)) {
                    ball_[0].x = nx;
                    ball_[0].y = ny;
                }
                if (in.action) {
                    solved_ = false;
                    mode_ = Mode::Aim;
                    say("CALL THE POCKET", 0.6f);
                }
            }
        } else if (mode_ == Mode::Aim) {
            if (bot_) {
                if (!solved_) {
                    Shot s = solve();
                    if (s.ok) {
                        aim_ = s.aim;
                        power_ = s.power;
                        called_ = s.pocket;
                        eightPocket_ = s.pocket;
                    } else {
                        called_ = 1;
                        eightPocket_ = 1;
                        aim_ = std::atan2(ball_[1].y - ball_[0].y, ball_[1].x - ball_[0].x);
                        power_ = 0.42f;
                    }
                    solved_ = true;
                }
                shoot();
            } else {
                float rate = fine ? 0.7f : 2.1f;
                aim_ = wrap(aim_ + in.x * rate * kDt);
                if (in.y < -0.2f) power_ = std::min(1.f, power_ + 0.5f * kDt);
                if (in.y > 0.2f) power_ = std::max(0.18f, power_ - 0.5f * kDt);
                if (in.action) shoot();
            }
        } else if (mode_ == Mode::Roll) {
            bool fellBefore[kBalls];
            for (int i = 0; i < kBalls; i++) fellBefore[i] = ball_[i].fell;
            bool clack = false;
            stepBodies(ball_, kDt, &clack);
            rollT_ += kDt;
            bool stopped = !moving(ball_, kStop);
            if (rollT_ > 4.8f || stopped) {
                absorb(ball_);
                resolve();
            } else {
                bool sunk = false;
                for (int i = 0; i < kBalls; i++)
                    if (ball_[i].fell && !fellBefore[i]) sunk = true;
                if (sunk) {
                    sys.apu.tone(0, 698.f, 0.09f);
                    sys.apu.tone(1, 880.f, 0.05f);
                    beep_ = 0.12f;
                    sys.rumble(0.2f, 0.45f, 40);
                } else if (clack) {
                    sys.apu.noiseBurst(0.14f, 1700.f, 0.03f);
                }
            }
        } else if (mode_ == Mode::Open) {
            openT_ -= kDt;
            if (openT_ <= 0) beginLift();
        } else if (mode_ == Mode::Lift) {
            handX_ += in.x * kHandV * kDt;
            handY_ += in.y * kHandV * kDt;
            cloth(handX_, handY_);
            if (in.action) tryLift();
        }
    }

    follow();
    draw();
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow) {
    if (!sys_ || h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 40 || s.x + s.w < -40 || s.y > gs::SCREEN_H + 40 || s.y + s.h < -40) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (!sys_ || row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        int tile = art_.font[c - 32];
        if (!tile) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(tile, pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    v.A.enabled = true;
    v.B.enabled = false;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        bool nap = ((y / 3) & 1) == 0;
        v.lineBackdrop[y] = nap ? gs::rgb4(1, 7, 3) : gs::rgb4(1, 10, 3);
    }
}

void Game::aimAid() {
    if (mode_ != Mode::Aim && mode_ != Mode::Place) return;
    if (ball_[0].down) return;
    float dx = std::cos(aim_), dy = std::sin(aim_);
    int hit = -1;
    float tHit = 1e9f;
    const float rad = kR * 2.f;
    for (int i = 1; i < kBalls; i++) {
        if (ball_[i].down) continue;
        float cx = ball_[i].x - ball_[0].x;
        float cy = ball_[i].y - ball_[0].y;
        float tc = cx * dx + cy * dy;
        if (tc <= 0.f) continue;
        float perp2 = cx * cx + cy * cy - tc * tc;
        if (perp2 > rad * rad) continue;
        float th = tc - std::sqrt(std::max(0.f, rad * rad - perp2));
        if (th < 4.f) continue;
        if (th < tHit) {
            tHit = th;
            hit = i;
        }
    }
    float limit = hit > 0 ? tHit : 78.f;
    int n = std::clamp(int(limit / 11.f), 1, 6);
    for (int i = 1; i <= n; i++) {
        float t = float(i) * 11.f;
        if (hit > 0 && t > tHit - 5.f) break;
        spr(art_.dot, ball_[0].x + dx * t, ball_[0].y + dy * t, 4.f, PAL_GOLD);
    }
    if (hit > 0) {
        float hx = ball_[0].x + dx * tHit;
        float hy = ball_[0].y + dy * tHit;
        float nx = ball_[hit].x - hx, ny = ball_[hit].y - hy;
        float nd = std::hypot(nx, ny);
        if (nd > 0.2f) {
            nx /= nd;
            ny /= nd;
            int dots = hit == 1 ? 5 : 3;
            for (int i = 1; i <= dots; i++)
                spr(art_.dot, ball_[hit].x + nx * float(i) * 12.f, ball_[hit].y + ny * float(i) * 12.f, 4.f,
                    hit == 1 ? PAL_HUD : PAL_GOLD);
        }
    }
}

void Game::cueDraw() {
    if (mode_ == Mode::Roll || mode_ == Mode::Open || mode_ == Mode::Lift || mode_ == Mode::Win || mode_ == Mode::Lose)
        return;
    if (ball_[0].down) return;
    int qi = int(std::lround(aim_ / kTau * float(kCueAngles)));
    qi %= kCueAngles;
    if (qi < 0) qi += kCueAngles;
    float q = float(qi) * (kTau / float(kCueAngles));
    const float tip = 15.f;
    const float gap = kR + 3.f;
    float cx = ball_[0].x - std::cos(q) * (tip + gap);
    float cy = ball_[0].y - std::sin(q) * (tip + gap);
    spr(art_.cue[qi], cx, cy, float(kCueBox), PAL_CUE);
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    if (mode_ == Mode::Title) {
        spr(art_.logo, 160.f, 52.f, float(art_.logo.h), PAL_LOGO);
        spr(art_.sub, 160.f, 74.f, float(art_.sub.h), PAL_LOGO);
    } else if (mode_ == Mode::Win) {
        spr(art_.win, 160.f, 108.f, float(art_.win.h), PAL_LOGO);
    } else if (mode_ == Mode::Lose) {
        spr(art_.lose, 160.f, 108.f, float(art_.lose.h), PAL_LOGO);
    }

    int showPk = opened_ ? eightPocket_ : called_;
    if (mode_ != Mode::Win && mode_ != Mode::Lose) {
        Pocket pk = pocketAt(showPk);
        float pulse = 22.f + std::sin(t_ * 6.f) * 1.8f;
        spr(art_.glow, pk.x, pk.y, pulse, PAL_GLOW);
    }

    if (mode_ == Mode::Win && lifted_) spr(art_.coin, handX_, handY_ - 8.f, 10.f, PAL_COIN);
    if (mode_ == Mode::Lift || (mode_ == Mode::Win && lifted_)) spr(art_.glove, handX_, handY_, 16.f, PAL_HAND);
    if (!lifted_) {
        float coinDrawY = coinY_;
        if (marked_ && !opened_ && !ball_[1].down) coinDrawY -= 6.f;
        spr(art_.coin, coinX_, coinDrawY, 10.f, PAL_COIN);
    }
    cueDraw();
    for (int i = 0; i < kBalls; i++) {
        if (ball_[i].down) continue;
        spr(art_.ball[i], ball_[i].x, ball_[i].y, kR * 2.f, PAL_BALL);
    }
    aimAid();
    for (int i = 0; i < kBalls; i++) {
        if (ball_[i].down) continue;
        spr(art_.shadow, ball_[i].x + 2.5f, ball_[i].y + 3.2f, 8.f, PAL_BALL, true);
    }
    if (!lifted_) spr(art_.shadow, coinX_ + 2.f, coinY_ + 3.f, 6.f, PAL_COIN, true);

    hud(1, 0, "S3 EIGHTMARK", PAL_GOLD);
    const char* st = "NO COIN";
    if (lifted_) st = "LIFTED";
    else if (opened_) st = "OPEN";
    else if (marked_) st = "MARK SET";
    hud(39 - int(std::strlen(st)), 0, st, PAL_GOLD);

    if (mode_ == Mode::Title) {
        hudC(1, "THE REST IS NOT THE JOB", PAL_HUD);
        hudC(26, "A FINISHED MARK ENDS IT", PAL_GOLD);
        hudC(27, "RETURN STARTS", PAL_HUD);
        return;
    }

    char buf[48];
    std::snprintf(buf, sizeof buf, "CALL %s", pocketAt(showPk).name);
    hud(1, 1, buf, PAL_GOLD);
    std::snprintf(buf, sizeof buf, "SHOTS %d", shots_);
    hud(39 - int(std::strlen(buf)), 1, buf, PAL_HUD);

    const char* msg = sayT_ > 0 && say_ ? say_ : "";
    int msgPal = mode_ == Mode::Lose ? PAL_RED : PAL_HUD;
    if (mode_ == Mode::Pause) msg = "PAUSED";
    else if (mode_ == Mode::Win) msg = "FINISHED MARK";
    else if (mode_ == Mode::Lose) msg = say_ ? say_ : "NO MARK";
    else if (mode_ == Mode::Roll) msg = "ROLLING";
    else if (mode_ == Mode::Open) msg = "MARK OPEN";
    else if (!(sayT_ > 0 && say_)) {
        if (mode_ == Mode::Mark) msg = onBall_ ? "ON THE EIGHT" : "SET THE COIN";
        else if (mode_ == Mode::Aim) msg = "CALL THE POCKET";
        else if (mode_ == Mode::Place) msg = "BALL IN HAND";
        else if (mode_ == Mode::Lift) msg = "LIFT THE COIN";
    }
    hudC(26, msg, msgPal);

    if (mode_ == Mode::Pause) {
        hudC(27, "RETURN RESUMES  ESC TITLE", PAL_GOLD);
        return;
    }
    if (mode_ == Mode::Win || mode_ == Mode::Lose) {
        hudC(27, mode_ == Mode::Win ? "RETURN RERACKS" : "RETURN TRIES AGAIN", PAL_GOLD);
        return;
    }
    if (mode_ == Mode::Mark) {
        hudC(27, "ARROWS MOVE   Z OR C SETS", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Place) {
        hudC(27, "ARROWS PLACE   Z DROPS", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Lift) {
        hudC(27, "ARROWS WALK   Z OR C LIFTS", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Aim) {
        int bars = std::clamp(int(std::lround(power_ * 8.f)), 1, 8);
        std::string pwr = "PWR ";
        for (int i = 0; i < 8; i++) pwr += i < bars ? '#' : '-';
        hud(1, 27, pwr, PAL_GOLD);
        hud(14, 27, "Q CALL  Z SHOOT", PAL_HUD);
    }
}

}  // namespace eightmark

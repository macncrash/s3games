#include "game/eightbell.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace eightbell {
namespace {

constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;
constexpr float kDt = 1.f / 60.f;
constexpr float kMu = 200.f;
constexpr float kVMax = 680.f;
constexpr float kRest = 0.74f;
constexpr int kSub = 8;
constexpr float kStop = 16.f;

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

}  // namespace

const char* Game::fateName(Fate f) {
    switch (f) {
    case Fate::Ring: return "BELL";
    case Fate::Scratch: return "SCRATCH";
    case Fate::Wide: return "WIDE";
    case Fate::Jaw: return "JAW";
    case Fate::Short: return "SHORT";
    case Fate::Miss: return "MISS";
    }
    return "MISS";
}

void Game::step(Body* b, float dt, bool* clack) {
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
                if (d < pk.sink + 6.f && d > 0.8f) {
                    float nx = dx / d, ny = dy / d;
                    if (ball.vx * nx + ball.vy * ny > 20.f) {
                        ball.vx += nx * 160.f * h;
                        ball.vy += ny * 160.f * h;
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
                        if (clack && rel > 40.f) *clack = true;
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
        }
        b[i].vx = b[i].vy = 0;
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
        if (std::hypot(x - p.x, y - p.y) < p.sink + 1.5f) return false;
    }
    return true;
}

bool Game::pathClear(float x0, float y0, float x1, float y1, int ignoreA, int ignoreB) const {
    const float need = kR * 2.f - 0.35f;
    for (int i = 0; i < kBalls; i++) {
        if (i == ignoreA || i == ignoreB || ball_[i].down) continue;
        if (segDist(ball_[i].x, ball_[i].y, x0, y0, x1, y1) < need) return false;
    }
    return true;
}

Game::Fate Game::fateOf(const Body* b) const {
    if (b[1].fell && b[1].pocket == kBell && !b[0].fell) return Fate::Ring;
    if (b[0].fell) return Fate::Scratch;
    if (b[1].fell) return Fate::Wide;
    Pocket bell = pocketAt(kBell);
    float near = std::hypot(b[1].x - bell.x, b[1].y - bell.y);
    if (near < 34.f) return Fate::Jaw;
    Home h = homeAt(1);
    if (std::hypot(b[1].x - h.x, b[1].y - h.y) < 26.f) return Fate::Short;
    return Fate::Miss;
}

Game::Fate Game::rollCopy(Body* b, float aim, float power) const {
    for (int i = 0; i < kBalls; i++) {
        b[i].vx = b[i].vy = 0;
        b[i].fell = false;
        b[i].pocket = -1;
        b[i].down = false;
    }
    float sp = strokeSpeed(power);
    b[0].vx = std::cos(aim) * sp;
    b[0].vy = std::sin(aim) * sp;
    for (int f = 0; f < 360; f++) {
        step(b, kDt, nullptr);
        if (b[1].fell && b[0].fell) break;
        if (!moving(b, kStop)) break;
    }
    absorb(b);
    return fateOf(b);
}

Game::Stroke Game::solve() const {
    Stroke best;
    if (ball_[0].down || ball_[1].down) return best;
    const float powers[] = {0.42f, 0.48f, 0.54f, 0.38f, 0.60f, 0.46f, 0.50f, 0.36f, 0.66f};
    const float offsets[] = {0.f, -0.015f, 0.015f, -0.03f, 0.03f, -0.05f, 0.05f, -0.008f, 0.008f, -0.07f, 0.07f};
    Pocket pk = pocketAt(kBell);
    const float ex = ball_[1].x, ey = ball_[1].y;
    const float cx = ball_[0].x, cy = ball_[0].y;
    float dx = pk.x - ex, dy = pk.y - ey;
    float dist = std::hypot(dx, dy);
    if (dist < 16.f) return best;
    float dirx = dx / dist, diry = dy / dist;
    float gx = ex - dirx * (kR * 2.f + 0.45f);
    float gy = ey - diry * (kR * 2.f + 0.45f);
    if (!inCloth(gx, gy)) return best;
    if (!pathClear(cx, cy, gx, gy, 0, 1)) return best;
    if (!pathClear(ex, ey, pk.x, pk.y, 1, 0)) return best;
    float base = std::atan2(gy - cy, gx - cx);
    for (float off : offsets) {
        float aim = base + off;
        for (float power : powers) {
            Body sim[kBalls];
            for (int i = 0; i < kBalls; i++) sim[i] = ball_[i];
            if (rollCopy(sim, aim, power) != Fate::Ring) continue;
            best.ok = true;
            best.aim = aim;
            best.power = power;
            return best;
        }
    }
    return best;
}

bool Game::layout() const {
    for (int i = 0; i < kBalls; i++) {
        if (!inCloth(ball_[i].x, ball_[i].y)) {
            std::fprintf(stderr, "s3eightbell ball %d off the cloth\n", i);
            return false;
        }
        for (int j = i + 1; j < kBalls; j++) {
            if (std::hypot(ball_[i].x - ball_[j].x, ball_[i].y - ball_[j].y) < kR * 2.f + 0.8f) {
                std::fprintf(stderr, "s3eightbell balls %d and %d overlap\n", i, j);
                return false;
            }
        }
    }
    return true;
}

bool Game::prove() {
    spot();
    if (!layout()) return false;
    Stroke s = solve();
    if (!s.ok) {
        Body sim[kBalls];
        for (int i = 0; i < kBalls; i++) sim[i] = ball_[i];
        float base = std::atan2(homeAt(1).y - homeAt(0).y, homeAt(1).x - homeAt(0).x);
        Fate natural = rollCopy(sim, base, 0.48f);
        std::fprintf(stderr, "s3eightbell no stroke  natural %s  eight %d at %.1f %.1f  cue %d\n", fateName(natural),
                     sim[1].pocket, sim[1].x, sim[1].y, sim[0].fell ? 1 : 0);
        return false;
    }
    Body sim[kBalls];
    for (int i = 0; i < kBalls; i++) sim[i] = ball_[i];
    if (rollCopy(sim, s.aim, 0.16f) == Fate::Ring) {
        std::fprintf(stderr, "s3eightbell a short stroke still rang\n");
        return false;
    }
    for (int i = 0; i < kBalls; i++) sim[i] = ball_[i];
    if (rollCopy(sim, s.aim + 1.15f, s.power) == Fate::Ring) {
        std::fprintf(stderr, "s3eightbell a wide stroke still rang\n");
        return false;
    }
    solvedAim_ = s.aim;
    solvedPower_ = s.power;
    solved_ = true;
    return true;
}

void Game::spot() {
    for (int i = 0; i < kBalls; i++) {
        Home h = homeAt(i);
        ball_[i] = Body{};
        ball_[i].x = h.x;
        ball_[i].y = h.y;
    }
}

void Game::fresh() {
    dead_ = 0;
    tryNo_ = 0;
    shots_ = 0;
    sunkSnd_ = 0;
    fanStep_ = -1;
    rung_ = won_ = over_ = false;
    whyFate_ = Fate::Miss;
    why_ = "OPEN";
    bellAmp_ = 0.16f;
    bellPh_ = 0;
    bellTick_ = chimeHold_ = toneT_ = fanT_ = 0;
    ready_ = rollT_ = deadT_ = ringT_ = leaveT_ = 0;
    t_ = 0;
    spot();
    if (solved_) {
        aim_ = solvedAim_;
        power_ = solvedPower_;
        if (!bot_) {
            aim_ = wrap(solvedAim_ + 0.26f);
            power_ = 0.28f;
        }
    } else {
        aim_ = std::atan2(homeAt(1).y - homeAt(0).y, homeAt(1).x - homeAt(0).x);
        power_ = 0.46f;
    }
}

void Game::toTitle() {
    fresh();
    mode_ = Mode::Title;
    if (sys_) sys_->setLight(30, 70, 40);
}

void Game::begin() {
    fresh();
    ready_ = 0;
    mode_ = Mode::Aim;
    why_ = "AIM";
    blip(523.f, 0.06f, 0.08f);
    if (sys_) sys_->setLight(24, 90, 40);
}

void Game::shoot() {
    if (mode_ != Mode::Aim || ball_[0].down) return;
    for (int i = 0; i < kBalls; i++) {
        ball_[i].fell = false;
        ball_[i].pocket = -1;
        ball_[i].vx = ball_[i].vy = 0;
    }
    float sp = strokeSpeed(power_);
    ball_[0].vx = std::cos(aim_) * sp;
    ball_[0].vy = std::sin(aim_) * sp;
    shots_++;
    rollT_ = 0;
    sunkSnd_ = 0;
    mode_ = Mode::Roll;
    why_ = "ROLL";
    if (sys_) {
        sys_->apu.noiseBurst(0.22f, 2100.f, 0.04f);
        sys_->rumble(0.1f, 0.22f, 28);
    }
}

void Game::ring() {
    if (rung_ || mode_ != Mode::Roll) return;
    rung_ = true;
    won_ = true;
    tryNo_ = dead_ + 1;
    whyFate_ = Fate::Ring;
    why_ = "BELL";
    bellAmp_ = 1.f;
    bellTick_ = 0.02f;
    ringT_ = 1.05f;
    mode_ = Mode::Ring;
    if (sys_) {
        sys_->rumble(0.4f, 0.75f, 180);
        sys_->setLight(255, 190, 50);
    }
}

void Game::dieTry(Fate why) {
    if (rung_ || mode_ != Mode::Roll) return;
    dead_++;
    whyFate_ = why;
    why_ = fateName(why);
    blip(96.f, 0.08f, 0.16f);
    if (sys_) {
        sys_->rumble(0.18f, 0.08f, 70);
        sys_->setLight(150, 28, 22);
    }
    if (dead_ >= 3) {
        won_ = false;
        over_ = true;
        mode_ = Mode::Over;
        why_ = "THIRD";
        return;
    }
    deadT_ = 0.9f;
    mode_ = Mode::Dead;
}

void Game::resolve() {
    absorb(ball_);
    Fate f = fateOf(ball_);
    if (f == Fate::Ring) ring();
    else dieTry(f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.7f);
    sys.apu.setEcho(0.12f, 0.22f, 0.14f);
    rules_ = prove();
    if (!rules_) std::fprintf(stderr, "s3eightbell rules failed\n");
    toTitle();
}

Game::Input Game::readPad(const gs::Pad& pad) const {
    Input in;
    in.action = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_TURBO);
    in.start = pad.pressed(gs::BTN_START);
    in.back = pad.pressed(gs::BTN_MODE);
    in.fine = pad.down(gs::BTN_Y) || pad.down(gs::BTN_Z);
    if (pad.down(gs::BTN_LEFT)) in.x -= 1.f;
    if (pad.down(gs::BTN_RIGHT)) in.x += 1.f;
    if (pad.down(gs::BTN_UP)) in.y -= 1.f;
    if (pad.down(gs::BTN_DOWN)) in.y += 1.f;
    if (std::fabs(pad.axisX) > 0.22f) in.x = pad.axisX;
    if (std::fabs(pad.axisY) > 0.22f) in.y = -pad.axisY;
    return in;
}

Game::Input Game::botInput() const {
    Input in;
    if (mode_ == Mode::Title && t_ > 0.4f) in.start = true;
    return in;
}

void Game::steer(float x, float y, bool fine) {
    float rate = fine ? 0.4f : 1.2f;
    aim_ = wrap(aim_ + x * rate * kDt);
    float pRate = fine ? 0.16f : 0.4f;
    power_ = std::clamp(power_ - y * pRate * kDt, 0.12f, 0.92f);
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
        sys_->apu.tone(0, notes[step], 0.1f);
        fanStep_ = step;
    } else if (step >= 8 && fanStep_ != 99) {
        sys_->apu.tone(0, 0, 0);
        fanStep_ = 99;
    }
}

void Game::tickAudio(float dt) {
    if (!sys_) return;
    bool chiming = rung_ && (mode_ == Mode::Ring || mode_ == Mode::Leave || (mode_ == Mode::Over && won_));
    bellPh_ += dt * (chiming ? 13.5f : 2.1f);
    if (chiming) bellAmp_ = std::max(0.32f, bellAmp_ - dt * 0.26f);
    else bellAmp_ = 0.16f;
    if (chiming) {
        bellTick_ -= dt;
        if (bellTick_ <= 0.f && bellAmp_ > 0.4f) {
            float vol = 0.05f + 0.08f * bellAmp_;
            sys_->apu.tone(1, 880.f, vol);
            sys_->apu.tone(2, 1320.f, vol * 0.65f);
            bellTick_ = 0.22f;
            chimeHold_ = 0.11f;
        }
        if (chimeHold_ > 0.f) {
            chimeHold_ -= dt;
            if (chimeHold_ <= 0.f) {
                sys_->apu.tone(1, 0, 0);
                sys_->apu.tone(2, 0, 0);
            }
        }
    } else if (chimeHold_ > 0.f) {
        chimeHold_ = 0;
        sys_->apu.tone(1, 0, 0);
        sys_->apu.tone(2, 0, 0);
    }
    bool fan = mode_ == Mode::Leave || (mode_ == Mode::Over && won_);
    if (!fan && toneT_ > 0.f) {
        toneT_ -= dt;
        if (toneT_ <= 0.f) sys_->apu.tone(0, 0, 0);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    tickAudio(kDt);
    Input in = bot_ ? botInput() : readPad(sys.pad);

    if (mode_ == Mode::Title) {
        if (!bot_ && in.back) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        } else if (in.start || in.action) begin();
    } else if (mode_ == Mode::Pause) {
        if (in.start) mode_ = held_;
        else if (in.back) toTitle();
    } else if (mode_ == Mode::Over) {
        if (won_) {
            fanT_ += kDt;
            fanfare();
        }
        if (!bot_ && (in.start || in.action)) begin();
        else if (!bot_ && in.back) toTitle();
    } else if (!bot_ && in.start) {
        held_ = mode_;
        mode_ = Mode::Pause;
    } else if (!bot_ && in.back && mode_ != Mode::Roll) {
        toTitle();
    } else if (mode_ == Mode::Aim) {
        ready_ += kDt;
        if (bot_) {
            if (solved_) {
                aim_ = solvedAim_;
                power_ = solvedPower_;
            }
            if (ready_ > 0.45f) shoot();
        } else {
            steer(in.x, in.y, in.fine);
            if (in.action) shoot();
        }
    } else if (mode_ == Mode::Roll) {
        rollT_ += kDt;
        bool clack = false;
        step(ball_, kDt, &clack);
        int sunk = 0;
        for (int i = 0; i < kBalls; i++)
            if (ball_[i].fell) sunk++;
        if (sunk > sunkSnd_) {
            blip(180.f, 0.07f, 0.06f);
            sunkSnd_ = sunk;
            sys.rumble(0.16f, 0.3f, 36);
        } else if (clack) {
            sys.apu.noiseBurst(0.12f, 1600.f, 0.03f);
        }
        bool both = ball_[0].fell && ball_[1].fell;
        if (both || rollT_ > 5.5f || !moving(ball_, kStop)) resolve();
    } else if (mode_ == Mode::Dead) {
        deadT_ -= kDt;
        if (deadT_ <= 0.f) {
            spot();
            ready_ = 0;
            mode_ = Mode::Aim;
            why_ = "AIM";
            if (sys_) sys_->setLight(24, 90, 40);
        }
    } else if (mode_ == Mode::Ring) {
        ringT_ -= kDt;
        if (ringT_ <= 0.f) {
            mode_ = Mode::Leave;
            leaveT_ = 1.15f;
            fanT_ = 0;
            fanStep_ = -1;
        }
    } else if (mode_ == Mode::Leave) {
        leaveT_ -= kDt;
        fanT_ += kDt;
        fanfare();
        if (leaveT_ <= 0.f) {
            mode_ = Mode::Over;
            over_ = true;
            won_ = true;
        }
    }

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

void Game::hud(int col, int row, const char* s, int pal) {
    if (!sys_ || !s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        if (x < 0 || x > 39) continue;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < 32 || c > 127) c = ' ';
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    if (!s) return;
    int n = int(std::strlen(s));
    hud((40 - n) / 2, row, s, pal);
}

void Game::backdrop() {
    if (!sys_) return;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        bool nap = ((y / 3) & 1) == 0;
        sys_->vdp.lineBackdrop[y] = nap ? gs::rgb4(0, 6, 3) : gs::rgb4(0, 8, 4);
        sys_->vdp.lineFog[y] = 0;
    }
}

void Game::aimAid() {
    if (mode_ != Mode::Aim && mode_ != Mode::Pause && mode_ != Mode::Title) return;
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
        if (th < 4.f || th >= tHit) continue;
        tHit = th;
        hit = i;
    }
    float limit = hit > 0 ? tHit : 70.f;
    for (int i = 1; i <= 6; i++) {
        float t = float(i) * 12.f;
        if (t > limit - 4.f) break;
        spr(art_.dot, ball_[0].x + dx * t, ball_[0].y + dy * t, 4.f, PAL_GOLD);
    }
    if (hit != 1) return;
    float hx = ball_[0].x + dx * tHit;
    float hy = ball_[0].y + dy * tHit;
    float nx = ball_[1].x - hx, ny = ball_[1].y - hy;
    float nd = std::hypot(nx, ny);
    if (nd < 0.2f) return;
    nx /= nd;
    ny /= nd;
    for (int i = 1; i <= 4; i++) spr(art_.dot, ball_[1].x + nx * float(i) * 12.f, ball_[1].y + ny * float(i) * 12.f, 4.f, PAL_WIN);
}

void Game::cueDraw() {
    if (mode_ == Mode::Roll || mode_ == Mode::Ring || mode_ == Mode::Leave || mode_ == Mode::Over) return;
    if (ball_[0].down) return;
    int qi = int(std::lround(aim_ / kTau * float(kCueAngles)));
    qi %= kCueAngles;
    if (qi < 0) qi += kCueAngles;
    float q = float(qi) * (kTau / float(kCueAngles));
    float back = kTip + kR + 4.f + std::max(0.f, power_ - 0.2f) * 16.f;
    float cx = ball_[0].x - std::cos(q) * back;
    float cy = ball_[0].y - std::sin(q) * back;
    spr(art_.cue[qi], cx, cy, float(kCueBox) * 0.86f, PAL_CUE);
}

void Game::lamps() {
    const float xs = 8.f;
    const float ys[3] = {76.f, 112.f, 148.f};
    bool lost = mode_ == Mode::Over && !won_;
    for (int i = 0; i < 3; i++) {
        spr(art_.lamp, xs, ys[i], 16.f, PAL_BELL);
        bool lit = !lost && i >= dead_;
        if (!lit) continue;
        float flick = 0.f;
        if (i == dead_ && !rung_) flick = std::sin(t_ * 18.f) * 0.6f;
        spr(art_.flame, xs + flick, ys[i] - 10.f, 8.f, PAL_FLAME);
    }
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    float amp = rung_ ? bellAmp_ : 0.16f;
    float swing = std::sin(bellPh_) * 8.f * amp;
    float bx = 160.f + swing;
    float by = 34.f;
    bool words = mode_ == Mode::Title || mode_ == Mode::Ring || mode_ == Mode::Leave || mode_ == Mode::Over;
    if (words && mode_ == Mode::Title) {
        spr(art_.eight, 160.f, 74.f, float(art_.eight.h), PAL_LOGO);
        spr(art_.bellWord, 160.f, 100.f, float(art_.bellWord.h), PAL_LOGO);
    } else if (mode_ == Mode::Ring) {
        spr(art_.rung, 160.f, 96.f, float(art_.rung.h), PAL_LOGO);
    } else if (mode_ == Mode::Leave || (mode_ == Mode::Over && won_)) {
        spr(art_.left, 160.f, 96.f, float(art_.left.h), PAL_WIN);
    } else if (mode_ == Mode::Over && !won_) {
        spr(art_.dead, 160.f, 96.f, float(art_.dead.h), PAL_ALERT);
    }

    if (rung_ && bellAmp_ > 0.45f) {
        for (int i = 0; i < 6; i++) {
            float a = bellPh_ * 1.3f + float(i) * 1.047f;
            float rad = 14.f + (1.f - bellAmp_) * 16.f;
            spr(art_.dot, bx + std::cos(a) * rad, by + 4.f + std::sin(a) * rad * 0.55f, 4.f, PAL_GOLD);
        }
    }
    spr(art_.bell, bx, by, 30.f, PAL_BELL);
    float clap = std::sin(bellPh_ + 0.7f) * 6.f * amp;
    spr(art_.clapper, bx + clap, by + 6.f, 10.f, PAL_BELL);
    spr(art_.yoke, 160.f, 16.f, 8.f, PAL_BELL);

    lamps();
    float pulse = 16.f + std::sin(t_ * 5.f) * 1.4f;
    Pocket bell = pocketAt(kBell);
    spr(art_.ring, bell.x, bell.y + 2.f, pulse, (mode_ == Mode::Over && !won_) ? PAL_ALERT : PAL_GLOW);

    cueDraw();
    aimAid();
    for (int i = 0; i < kBalls; i++) {
        if (ball_[i].down) continue;
        spr(art_.ball[i], ball_[i].x, ball_[i].y, kR * 2.f, PAL_BALL);
    }
    for (int i = 0; i < kBalls; i++) {
        if (ball_[i].down) continue;
        spr(art_.shadow, ball_[i].x + 2.4f, ball_[i].y + 3.1f, 7.f, PAL_BALL, true);
    }

    hud(1, 0, "S3 EIGHTBELL", PAL_GOLD);
    char buf[40];
    if (mode_ == Mode::Title) {
        hud(32, 0, "3 TRIES", PAL_GOLD);
        hudC(26, "THE BELL RINGS OR THE TRY DIES", PAL_HUD);
        hudC(27, "RETURN STARTS", PAL_GOLD);
        return;
    }

    int showTry = rung_ ? tryNo_ : std::min(dead_ + 1, 3);
    std::snprintf(buf, sizeof buf, "TRY %d OF 3", showTry);
    hud(28, 0, buf, dead_ == 2 && !rung_ ? PAL_ALERT : PAL_GOLD);

    const char* msg = "AIM THE EIGHT";
    int msgPal = PAL_HUD;
    if (mode_ == Mode::Pause) {
        msg = "PAUSED";
        msgPal = PAL_GOLD;
    } else if (mode_ == Mode::Roll) {
        msg = "ROLLING";
    } else if (mode_ == Mode::Dead) {
        msg = why_;
        msgPal = PAL_ALERT;
    } else if (mode_ == Mode::Ring) {
        msg = "THE BELL";
        msgPal = PAL_WIN;
    } else if (mode_ == Mode::Leave || (mode_ == Mode::Over && won_)) {
        msg = "BEFORE THE THIRD TRY DIED";
        msgPal = PAL_WIN;
    } else if (mode_ == Mode::Over) {
        msg = "THE THIRD TRY DIED";
        msgPal = PAL_ALERT;
    }
    hudC(26, msg, msgPal);

    if (mode_ == Mode::Pause) {
        hudC(27, "RETURN RESUMES  ESC TITLE", PAL_GOLD);
        return;
    }
    if (mode_ == Mode::Over) {
        hudC(27, won_ ? "RETURN PLAYS AGAIN" : "RETURN TRIES AGAIN", PAL_GOLD);
        return;
    }
    if (mode_ == Mode::Dead) {
        hudC(27, dead_ == 1 ? "TWO LEFT" : "ONE LEFT", PAL_ALERT);
        return;
    }
    if (mode_ == Mode::Aim) {
        int bars = std::clamp(int(std::lround(power_ * 8.f)), 1, 8);
        const char* zone = power_ < 0.34f ? "SHORT" : power_ > 0.72f ? "HOT" : "FIRM";
        int zpal = power_ < 0.34f || power_ > 0.72f ? PAL_ALERT : PAL_GOLD;
        std::snprintf(buf, sizeof buf, "%s ", zone);
        int n = int(std::strlen(buf));
        for (int i = 0; i < 8; i++) buf[n++] = i < bars ? '#' : '-';
        buf[n] = 0;
        hud(1, 27, buf, zpal);
        hud(30, 27, "Z SHOOT", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Ring) hudC(27, "IT RANG", PAL_WIN);
    else if (mode_ == Mode::Leave) hudC(27, "LEAVE", PAL_WIN);
}

}  // namespace eightbell

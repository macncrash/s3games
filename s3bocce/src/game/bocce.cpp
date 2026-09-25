#include "game/bocce.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <sstream>

namespace bocce {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr int SUB = 4;
constexpr float MU = 140.f;
constexpr float STOP = 2.f;
constexpr float WALL_E = 0.38f;
constexpr float BALL_E = 0.70f;
constexpr float X0 = 40.f;
constexpr float X1 = 280.f;
constexpr float Y0 = 48.f;
constexpr float Y1 = 176.f;
constexpr float R = 5.5f;
constexpr float RJ = 3.25f;
constexpr float MID = 160.f;
constexpr int YOU = 0;
constexpr int THEM = 1;
constexpr int JACK = 2;
constexpr float JACK_X = 214.f;
constexpr float JACK_Y = 112.f;

struct Off {
    float dx, dy;
};
constexpr Off kYou[4] = {{-24.f, -18.f}, {-24.f, 18.f}, {-24.f, -36.f}, {-24.f, 36.f}};
constexpr Off kCpu[4] = {{-78.f, 0.f}, {-78.f, -52.f}, {-78.f, 52.f}, {-120.f, 18.f}};

float radius(int side) { return side == JACK ? RJ : R; }

float startX(int side) { return X0 + radius(side) + 1.5f; }

// Move, then shave a fixed speed. speedForRange integrates this same step,
// so a straight toss with no contact stops on the mark the autopilot picked.
void freeStep(float& x, float& y, float& vx, float& vy, float h) {
    float sp = std::hypot(vx, vy);
    if (sp < STOP) {
        vx = vy = 0;
        return;
    }
    x += vx * h;
    y += vy * h;
    float drop = MU * h;
    if (sp <= drop) {
        vx = vy = 0;
        return;
    }
    float k = (sp - drop) / sp;
    vx *= k;
    vy *= k;
    if (std::hypot(vx, vy) < STOP) vx = vy = 0;
}

float rangeOf(float speed) {
    float x = 0, y = 0, vx = speed, vy = 0;
    const float h = DT / float(SUB);
    const int n = 60 * SUB * 8;
    for (int i = 0; i < n; i++) {
        if (std::hypot(vx, vy) < STOP) break;
        freeStep(x, y, vx, vy, h);
    }
    return x;
}

float speedForRange(float dist) {
    if (dist < 1.f) return 0;
    float lo = 0.f, hi = 40.f;
    while (rangeOf(hi) < dist && hi < 4000.f) hi *= 2.f;
    for (int i = 0; i < 24; i++) {
        float mid = 0.5f * (lo + hi);
        if (rangeOf(mid) < dist) lo = mid;
        else hi = mid;
    }
    return hi;
}

void nudgeWalls(Game::Ball& b) {
    float r = radius(b.side);
    if (b.y < Y0 + r) {
        b.y = Y0 + r;
        if (b.vy < 0) b.vy = -b.vy * WALL_E;
    } else if (b.y > Y1 - r) {
        b.y = Y1 - r;
        if (b.vy > 0) b.vy = -b.vy * WALL_E;
    }
    if (b.x < X0 + r) {
        b.x = X0 + r;
        if (b.vx < 0) b.vx = -b.vx * WALL_E;
    } else if (b.x > X1 - r) {
        b.x = X1 - r;
        if (b.vx > 0) b.vx = -b.vx * WALL_E;
    }
}

bool collide(Game::Ball& a, Game::Ball& b) {
    if (!a.live || !b.live) return false;
    float dx = b.x - a.x;
    float dy = b.y - a.y;
    float minD = radius(a.side) + radius(b.side);
    float d2 = dx * dx + dy * dy;
    if (d2 < 1e-6f) {
        b.x += minD;
        return false;
    }
    if (d2 >= minD * minD) return false;
    float d = std::sqrt(d2);
    float nx = dx / d, ny = dy / d;
    float overlap = minD - d;
    a.x -= nx * overlap * 0.5f;
    a.y -= ny * overlap * 0.5f;
    b.x += nx * overlap * 0.5f;
    b.y += ny * overlap * 0.5f;
    float rv = (b.vx - a.vx) * nx + (b.vy - a.vy) * ny;
    if (rv >= 0) return false;
    float impulse = -(1.f + BALL_E) * rv * 0.5f;
    a.vx -= impulse * nx;
    a.vy -= impulse * ny;
    b.vx += impulse * nx;
    b.vy += impulse * ny;
    a.rest = b.rest = false;
    return impulse > 16.f;
}

void cpuSpot(int i, float jx, float jy, float& tx, float& ty) {
    i = std::clamp(i, 0, 3);
    tx = jx + kCpu[i].dx;
    ty = jy + kCpu[i].dy;
    const float minX = X0 + R + 28.f;
    const float maxX = X1 - R - 4.f;
    const float minY = Y0 + R + 2.f;
    const float maxY = Y1 - R - 2.f;
    tx = std::clamp(tx, minX, maxX);
    ty = std::clamp(ty, minY, maxY);
    if (std::hypot(tx - jx, ty - jy) < 60.f) {
        tx = minX + float(i) * 8.f;
        ty = (i & 1) ? maxY : minY;
    }
}

}  // namespace

void Game::beginEnd() {
    balls_.clear();
    phase_ = Phase::Jack;
    next_ = YOU;
    thrown_[0] = thrown_[1] = 0;
    aimY_ = (Y0 + Y1) * 0.5f;
    aimAng_ = 0;
    meter_ = 0;
    meterDir_ = 1.f;
    charging_ = false;
    aimT_ = 0;
    rollT_ = 0;
    mode_ = Mode::Aim;
    say_ = "TOSS THE LITTLE BALL";
}

void Game::layCourt() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.A.clear();
    v.A.enabled = true;
    v.B.enabled = false;
    v.A.scroll(0, 0);
    for (int cy = 6; cy < 22; cy++) {
        for (int cx = 5; cx < 35; cx++) {
            int tile = (cx == 20) ? art_.chalk : art_.gravel;
            v.A.set(cx, cy, gs::entry(tile, PAL_GRAVEL));
        }
    }
    for (int cx = 4; cx <= 35; cx++) {
        v.A.set(cx, 5, gs::entry(art_.wood, PAL_WOOD));
        v.A.set(cx, 22, gs::entry(art_.wood, PAL_WOOD));
    }
    for (int cy = 6; cy < 22; cy++) {
        v.A.set(4, cy, gs::entry(art_.wood, PAL_WOOD));
        v.A.set(35, cy, gs::entry(art_.wood, PAL_WOOD));
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    layCourt();
    sys.vdp.setFogColor(gs::rgb4(6, 5, 3));
    for (int y = 0; y < gs::SCREEN_H; y++) {
        sys.vdp.lineFog[y] = 0;
        sys.vdp.road[y].on = false;
    }
    sys.apu.setMaster(0.7f);
    balls_.reserve(16);
    you_ = them_ = 0;
    ends_ = 0;
    endNo_ = 1;
    over_ = won_ = false;
    if (bot_) beginEnd();
    else mode_ = Mode::Title;
}

bool Game::yourTurn() const { return phase_ == Phase::Jack || next_ == YOU; }

Game::Ball* Game::findJack() {
    for (auto& b : balls_)
        if (b.live && b.side == JACK) return &b;
    return nullptr;
}

const Game::Ball* Game::findJack() const {
    for (const auto& b : balls_)
        if (b.live && b.side == JACK) return &b;
    return nullptr;
}

void Game::spawn(int side, float x, float y, float vx, float vy, float wantX, float wantY) {
    Ball b;
    b.side = side;
    b.x = x;
    b.y = y;
    b.vx = vx;
    b.vy = vy;
    b.wantX = wantX;
    b.wantY = wantY;
    b.live = true;
    b.rest = false;
    balls_.push_back(b);
    if (!sys_) return;
    sys_->apu.noiseBurst(side == JACK ? 0.16f : 0.26f, side == JACK ? 880.f : 1500.f, 0.05f);
    sys_->rumble(0.12f, 0.32f, 28);
}

void Game::rollTo(int side, float tx, float ty) {
    float r = radius(side);
    float x = startX(side);
    float y = std::clamp(ty, Y0 + r + 1.f, Y1 - r - 1.f);
    float dist = tx - x;
    float maxDist = (X1 - r - 2.f) - x;
    if (dist < 8.f) dist = 8.f;
    if (dist > maxDist) dist = maxDist;
    float spd = speedForRange(dist);
    spawn(side, x, y, spd, 0.f, x + dist, y);
    mode_ = Mode::Roll;
    rollT_ = 0;
    charging_ = false;
}

void Game::botLaunch() {
    if (phase_ == Phase::Jack) {
        rollTo(JACK, JACK_X, JACK_Y);
        return;
    }
    Ball* j = findJack();
    if (!j) return;
    int i = thrown_[YOU];
    if (i < 0 || i > 3) return;
    float jx = j->x, jy = j->y;
    rollTo(YOU, jx + kYou[i].dx, jy + kYou[i].dy);
    thrown_[YOU]++;
}

void Game::cpuLaunch() {
    Ball* j = findJack();
    if (!j) return;
    int i = thrown_[THEM];
    if (i < 0 || i > 3) return;
    float tx, ty;
    cpuSpot(i, j->x, j->y, tx, ty);
    rollTo(THEM, tx, ty);
    thrown_[THEM]++;
}

void Game::humanAim(const gs::Pad& pad) {
    const float rate = 86.f;
    if (pad.down(gs::BTN_LEFT)) aimY_ -= rate * DT;
    if (pad.down(gs::BTN_RIGHT)) aimY_ += rate * DT;
    if (std::fabs(pad.axisX) > 0.15f) aimY_ += pad.axisX * 80.f * DT;
    if (pad.down(gs::BTN_UP)) aimAng_ -= 0.85f * DT;
    if (pad.down(gs::BTN_DOWN)) aimAng_ += 0.85f * DT;
    const int side = phase_ == Phase::Jack ? JACK : YOU;
    const float r = radius(side);
    aimY_ = std::clamp(aimY_, Y0 + r + 2.f, Y1 - r - 2.f);
    aimAng_ = std::clamp(aimAng_, -0.65f, 0.65f);

    const bool held = pad.down(gs::BTN_A) || pad.down(gs::BTN_B) || pad.down(gs::BTN_C) || pad.down(gs::BTN_TURBO);
    const bool tapped =
        pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_TURBO);
    if (pad.accel > 0.18f) {
        charging_ = true;
        meter_ = std::clamp((pad.accel - 0.18f) / 0.82f, 0.f, 1.f);
    } else if (tapped) {
        charging_ = true;
        meter_ = 0.f;
        meterDir_ = 1.f;
    }
    if (charging_ && (held || pad.accel > 0.18f)) {
        if (pad.accel <= 0.18f) {
            meter_ += meterDir_ * 0.72f * DT;
            if (meter_ >= 1.f) {
                meter_ = 1.f;
                meterDir_ = -1.f;
            } else if (meter_ <= 0.f) {
                meter_ = 0.f;
                meterDir_ = 1.f;
            }
        }
        return;
    }
    if (!charging_) return;
    float rmin = phase_ == Phase::Jack ? 130.f : 28.f;
    float rmax = (X1 - r - 3.f) - startX(side);
    if (rmax < rmin + 10.f) rmax = rmin + 10.f;
    float dist = rmin + meter_ * (rmax - rmin);
    float spd = speedForRange(dist);
    float ang = aimAng_;
    spawn(side, startX(side), aimY_, std::cos(ang) * spd, std::sin(ang) * spd, -1.f, -1.f);
    if (side != JACK) thrown_[YOU]++;
    charging_ = false;
    mode_ = Mode::Roll;
    rollT_ = 0;
}

void Game::stepSim() {
    const float h = DT / float(SUB);
    bool hit = false;
    for (int s = 0; s < SUB; s++) {
        for (auto& b : balls_) {
            if (!b.live) continue;
            if (b.vx == 0.f && b.vy == 0.f) {
                b.rest = true;
                continue;
            }
            freeStep(b.x, b.y, b.vx, b.vy, h);
            nudgeWalls(b);
            if (!std::isfinite(b.x) || !std::isfinite(b.y)) {
                b.live = false;
                b.rest = true;
                b.vx = b.vy = 0;
            }
        }
        for (int pass = 0; pass < 2; pass++) {
            for (size_t i = 0; i < balls_.size(); i++)
                for (size_t j = i + 1; j < balls_.size(); j++)
                    if (collide(balls_[i], balls_[j])) hit = true;
        }
        for (auto& b : balls_) {
            if (!b.live) continue;
            nudgeWalls(b);
            float sp = std::hypot(b.vx, b.vy);
            if (sp > 700.f) {
                b.vx *= 700.f / sp;
                b.vy *= 700.f / sp;
                sp = 700.f;
            }
            if (sp < STOP) {
                b.vx = b.vy = 0;
                b.rest = true;
            } else {
                b.rest = false;
            }
        }
    }
    if (hit && hitCd_ <= 0.f && sys_) {
        sys_->apu.noiseBurst(0.12f, 700.f, 0.04f);
        hitCd_ = 0.06f;
    }
}

void Game::forceRest() {
    for (auto& b : balls_) {
        b.vx = b.vy = 0;
        b.rest = true;
    }
}

bool Game::allRest() const {
    if (balls_.empty()) return false;
    for (const auto& b : balls_)
        if (b.live && !b.rest) return false;
    return true;
}

void Game::removeJack() {
    balls_.erase(std::remove_if(balls_.begin(), balls_.end(), [](const Ball& b) { return b.side == JACK; }),
                 balls_.end());
}

void Game::onRest() {
    rollT_ = 0;
    if (phase_ == Phase::Jack) {
        Ball* j = findJack();
        if (!j || j->x < MID + 8.f) {
            say_ = "PAST THE MIDDLE";
            sayT_ = bot_ ? 0.05f : 1.05f;
            mode_ = Mode::Reject;
            return;
        }
        phase_ = Phase::Bowl;
        next_ = YOU;
        thrown_[0] = thrown_[1] = 0;
        mode_ = Mode::Aim;
        aimT_ = 0;
        charging_ = false;
        meter_ = 0;
        meterDir_ = 1.f;
        aimAng_ = 0;
        aimY_ = j->y;
        say_ = "YOUR BALL";
        return;
    }
    if (thrown_[YOU] >= 4 && thrown_[THEM] >= 4) {
        applyScore();
        return;
    }
    int other = 1 - next_;
    if (thrown_[other] < 4) next_ = other;
    mode_ = Mode::Aim;
    aimT_ = 0;
    charging_ = false;
    if (next_ == YOU) {
        meter_ = 0;
        meterDir_ = 1.f;
        aimAng_ = 0;
        if (Ball* j = findJack()) aimY_ = j->y;
    }
}

void Game::applyScore() {
    const Ball* jack = findJack();
    int gain[2] = {};
    if (jack) {
        float best[2] = {1e9f, 1e9f};
        int n[2] = {};
        float dist[16];
        int sides[16];
        int nb = 0;
        for (const auto& b : balls_) {
            if (!b.live || b.side == JACK || nb >= 16) continue;
            float d = std::hypot(b.x - jack->x, b.y - jack->y);
            dist[nb] = d;
            sides[nb] = b.side;
            if (d < best[b.side]) best[b.side] = d;
            n[b.side]++;
            nb++;
        }
        if (n[YOU] && n[THEM]) {
            if (best[YOU] + 0.5f < best[THEM]) {
                for (int i = 0; i < nb; i++)
                    if (sides[i] == YOU && dist[i] < best[THEM] - 0.5f) gain[YOU]++;
            } else if (best[THEM] + 0.5f < best[YOU]) {
                for (int i = 0; i < nb; i++)
                    if (sides[i] == THEM && dist[i] < best[YOU] - 0.5f) gain[THEM]++;
            }
        } else if (n[YOU] && !n[THEM]) {
            gain[YOU] = n[YOU];
        } else if (n[THEM] && !n[YOU]) {
            gain[THEM] = n[THEM];
        }
    }
    you_ += gain[YOU];
    them_ += gain[THEM];
    gainYou_ = gain[YOU];
    gainThem_ = gain[THEM];
    ends_++;
    aimT_ = 0;
    if (you_ >= 7 && you_ > them_) {
        won_ = true;
        over_ = true;
        mode_ = Mode::Win;
        chime(2);
    } else if (them_ >= 7 && them_ > you_) {
        won_ = false;
        over_ = true;
        mode_ = Mode::Lose;
        chime(0);
    } else {
        mode_ = Mode::Score;
        chime(gainYou_ > 0 ? 1 : 0);
    }
}

void Game::chime(int kind) {
    if (!sys_) return;
    if (kind >= 2) {
        sys_->apu.tone(0, 523.f, 0.09f);
        sys_->apu.tone(1, 659.f, 0.08f);
        sys_->apu.tone(2, 784.f, 0.07f);
        toneT_ = 0.55f;
        sys_->rumble(0.45f, 0.75f, 160);
    } else if (kind == 1) {
        sys_->apu.tone(0, 440.f, 0.07f);
        sys_->apu.tone(1, 554.f, 0.06f);
        toneT_ = 0.28f;
    } else {
        sys_->apu.tone(0, 220.f, 0.05f);
        toneT_ = 0.18f;
    }
}

void Game::quiet() {
    if (!sys_ || toneT_ <= 0.f) return;
    toneT_ -= DT;
    if (toneT_ <= 0.f) {
        sys_->apu.tone(0, 0, 0);
        sys_->apu.tone(1, 0, 0);
        sys_->apu.tone(2, 0, 0);
    }
}

void Game::ghost(float& gx, float& gy) const {
    const int side = phase_ == Phase::Jack ? JACK : YOU;
    const float r = radius(side);
    const float x = startX(side);
    float rmin = phase_ == Phase::Jack ? 130.f : 28.f;
    float rmax = (X1 - r - 3.f) - x;
    if (rmax < rmin + 10.f) rmax = rmin + 10.f;
    float dist = rmin + meter_ * (rmax - rmin);
    gx = x + std::cos(aimAng_) * dist;
    gy = aimY_ + std::sin(aimAng_) * dist;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool shadow) {
    if (!sys_ || h < 1.f || m.h <= 0) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - w * 0.5f));
    s.y = int16_t(std::lround(cy - h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
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
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        if (y < 40) {
            float u = y / 39.f;
            v.lineBackdrop[y] = gs::rgb4(4 + int(u * 6), 8 + int(u * 4), 14 - int(u * 2));
        } else if (y < 184) {
            v.lineBackdrop[y] = gs::rgb4(2, 7, 3);
        } else {
            v.lineBackdrop[y] = gs::rgb4(2, 5, 2);
        }
    }
}

void Game::drawBall(int side, float x, float y) {
    float h = side == JACK ? 9.f : 13.f;
    const gs::Mipped& m = side == JACK ? art_.jack : art_.ball;
    int pal = side == JACK ? PAL_JACK : side == YOU ? PAL_YOU : PAL_THEM;
    spr(m, x, y, h, pal, false, false);
    spr(art_.shadow, x + 2.f, y + 3.f, h * 0.45f, pal, false, true);
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    const bool showPlay = mode_ != Mode::Title;
    if (mode_ == Mode::Title) spr(art_.logo, 160, 16, float(art_.logo.h), PAL_LOGO);
    if (mode_ == Mode::Win) spr(art_.win, 160, 18, float(art_.win.h), PAL_LOGO);

    if (showPlay && mode_ == Mode::Aim && yourTurn()) {
        float gx, gy;
        ghost(gx, gy);
        const int side = phase_ == Phase::Jack ? JACK : YOU;
        float x = startX(side);
        float dx = gx - x, dy = gy - aimY_;
        float len = std::hypot(dx, dy);
        int n = std::min(8, int(len / 16.f));
        for (int i = 1; i <= n; i++) {
            float u = float(i) / float(n + 1);
            spr(art_.dot, x + dx * u, aimY_ + dy * u, 4.5f, PAL_AIM);
        }
        spr(art_.dot, gx, gy, 8.f, PAL_JACK);
    } else if (showPlay && mode_ == Mode::Aim && phase_ == Phase::Bowl) {
        if (const Ball* j = findJack()) {
            float tx, ty;
            cpuSpot(std::min(thrown_[THEM], 3), j->x, j->y, tx, ty);
            spr(art_.dot, tx, ty, 7.f, PAL_THEM);
        }
    }

    if (mode_ == Mode::Title) {
        drawBall(JACK, JACK_X, JACK_Y);
        for (const Off& o : kYou) drawBall(YOU, JACK_X + o.dx, JACK_Y + o.dy);
        for (const Off& o : kCpu) drawBall(THEM, JACK_X + o.dx, JACK_Y + o.dy);
    } else {
        for (const auto& b : balls_)
            if (b.live && b.side == JACK) drawBall(b.side, b.x, b.y);
        for (const auto& b : balls_)
            if (b.live && b.side != JACK) drawBall(b.side, b.x, b.y);
    }

    spr(art_.sun, 230, 14, 16, PAL_SUN);
    const float trees[4] = {24.f, 72.f, 250.f, 304.f};
    for (int i = 0; i < 4; i++) {
        float th = 32.f;
        spr(art_.tree, trees[i], 46.f - th * 0.5f, th, PAL_TREE, i & 1);
    }
    spr(art_.awning, 262, 33, 16, PAL_AWN);
    spr(art_.pot, 16, 158, 20, PAL_POT);
    spr(art_.pot, 306, 158, 20, PAL_POT, true);
    const float tx[6] = {10, 20, 302, 312, 14, 308};
    const float ty[6] = {68, 112, 74, 124, 146, 98};
    for (int i = 0; i < 6; i++) spr(art_.tuft, tx[i], ty[i], 12, PAL_TUFT, i & 1);

    const bool blink = (int(t_ * 2.2f) & 1) == 0;
    if (mode_ == Mode::Title) {
        hudC(23, "FIRST TO SEVEN", PAL_GOLD);
        hudC(24, "CLOSEST TO THE LITTLE BALL", PAL_INK);
        hud(2, 25, "GREEN YOU", PAL_YOU);
        hud(14, 25, "RED THEM", PAL_THEM);
        hud(26, 25, "CREAM BALL", PAL_JACK);
        hudC(26, "ARROWS AIM    HOLD Z", PAL_INK);
        if (blink) hudC(27, "PRESS START", PAL_GOLD);
    } else if (mode_ == Mode::Pause) {
        hudC(24, "PAUSED", PAL_GOLD);
        hudC(27, "START RESUME    ESC TITLE", PAL_INK);
    } else if (mode_ == Mode::Win || mode_ == Mode::Lose) {
        char buf[64];
        std::snprintf(buf, sizeof buf, "YOU %d", you_);
        hud(1, 24, buf, PAL_YOU);
        std::snprintf(buf, sizeof buf, "THEM %d", them_);
        hud(32, 24, buf, PAL_THEM);
        hudC(25, mode_ == Mode::Win ? "FIRST TO SEVEN" : "THEY REACHED SEVEN", mode_ == Mode::Win ? PAL_GOLD : PAL_THEM);
        if (gainYou_ > 0) {
            std::snprintf(buf, sizeof buf, "YOU SCORE %d", gainYou_);
            hudC(26, buf, PAL_YOU);
        } else if (gainThem_ > 0) {
            std::snprintf(buf, sizeof buf, "THEY SCORE %d", gainThem_);
            hudC(26, buf, PAL_THEM);
        }
        if (!bot_) hudC(27, "START AGAIN", PAL_GOLD);
    } else {
        char buf[64];
        std::snprintf(buf, sizeof buf, "YOU %d", you_);
        hud(1, 24, buf, PAL_YOU);
        std::snprintf(buf, sizeof buf, "END %d  TO 7", endNo_);
        hudC(24, buf, PAL_GOLD);
        std::snprintf(buf, sizeof buf, "THEM %d", them_);
        hud(32, 24, buf, PAL_THEM);

        std::string line;
        if (mode_ == Mode::Reject) line = "PAST THE MIDDLE";
        else if (mode_ == Mode::Score) {
            if (gainYou_ > 0) line = "YOU SCORE " + std::to_string(gainYou_);
            else if (gainThem_ > 0) line = "THEY SCORE " + std::to_string(gainThem_);
            else line = "END TIED";
        } else if (mode_ == Mode::Roll) {
            const Ball* j = findJack();
            float best[2] = {1e9f, 1e9f};
            int n[2] = {};
            if (j) {
                for (const auto& b : balls_) {
                    if (!b.live || b.side == JACK) continue;
                    float d = std::hypot(b.x - j->x, b.y - j->y);
                    if (d < best[b.side]) best[b.side] = d;
                    n[b.side]++;
                }
            }
            if (n[YOU] && (!n[THEM] || best[YOU] + 0.5f < best[THEM])) line = "YOU ARE CLOSER";
            else if (n[THEM] && (!n[YOU] || best[THEM] + 0.5f < best[YOU])) line = "THEY ARE CLOSER";
            else line = "ROLLING";
        } else if (phase_ == Phase::Jack) {
            float gx, gy;
            ghost(gx, gy);
            line = gx < MID + 8.f ? "TOO SHORT - PASS THE LINE" : "TOSS THE LITTLE BALL";
        } else if (next_ == YOU) {
            line = "YOUR BALL " + std::to_string(thrown_[YOU] + 1) + " OF 4";
        } else {
            line = "THEIR BALL " + std::to_string(std::min(thrown_[THEM] + 1, 4)) + " OF 4";
        }
        hudC(25, line, mode_ == Mode::Score && gainYou_ > 0 ? PAL_YOU : PAL_INK);

        if (mode_ == Mode::Aim && yourTurn() && charging_) {
            int n = int(std::lround(meter_ * 16.f));
            if (n < 0) n = 0;
            if (n > 16) n = 16;
            std::string bar = "POWER ";
            for (int i = 0; i < 16; i++) bar += (i < n) ? '=' : '.';
            hud(8, 26, bar, PAL_GOLD);
        } else if (mode_ == Mode::Aim && yourTurn() && phase_ == Phase::Jack) {
            hudC(26, "HOLD Z, LET GO TO TOSS", PAL_INK);
        } else if (mode_ == Mode::Aim && yourTurn()) {
            hudC(26, "HOLD Z, LET GO TO BOWL", PAL_INK);
        } else if (mode_ == Mode::Roll) {
            hudC(26, "ROLLING", PAL_GOLD);
        } else {
            hudC(26, "CLOSER THAN THEIRS SCORES", PAL_INK);
        }
        hudC(27, "ARROWS AIM   HOLD Z   START PAUSE", PAL_INK);
    }
}

std::string Game::dump() const {
    std::ostringstream o;
    o.setf(std::ios::fixed);
    o.precision(1);
    o << "mode " << int(mode_) << " end " << endNo_;
    for (const auto& b : balls_) {
        const char* n = b.side == JACK ? "jack" : b.side == YOU ? "you" : "them";
        o << " " << n << "=" << b.x << "," << b.y;
        if (b.wantX > 0) o << "(want " << b.wantX << "," << b.wantY << ")";
    }
    return o.str();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    if (hitCd_ > 0) hitCd_ -= DT;
    quiet();
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
            you_ = them_ = 0;
            ends_ = 0;
            endNo_ = 1;
            over_ = won_ = false;
            gainYou_ = gainThem_ = 0;
            beginEnd();
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            sys.quit();
        }
        draw();
        return;
    }
    if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = held_;
        else if (pad.pressed(gs::BTN_MODE)) mode_ = Mode::Title;
        draw();
        return;
    }
    if (mode_ == Mode::Win || mode_ == Mode::Lose) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
            you_ = them_ = 0;
            ends_ = 0;
            endNo_ = 1;
            over_ = won_ = false;
            gainYou_ = gainThem_ = 0;
            beginEnd();
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
        }
        draw();
        return;
    }
    if (!bot_ && pad.pressed(gs::BTN_MODE)) {
        mode_ = Mode::Title;
        charging_ = false;
        draw();
        return;
    }
    if (!bot_ && (mode_ == Mode::Aim || mode_ == Mode::Roll) && pad.pressed(gs::BTN_START)) {
        held_ = mode_;
        mode_ = Mode::Pause;
        draw();
        return;
    }
    if (mode_ == Mode::Reject) {
        sayT_ -= DT;
        if (sayT_ <= 0.f || (!bot_ && (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_START)))) {
            removeJack();
            mode_ = Mode::Aim;
            aimT_ = 0;
            charging_ = false;
            meter_ = 0;
            meterDir_ = 1.f;
            say_ = "TOSS THE LITTLE BALL";
        }
        draw();
        return;
    }
    if (mode_ == Mode::Score) {
        aimT_ += DT;
        if (aimT_ > (bot_ ? 0.12f : 1.35f) || (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)))) {
            endNo_++;
            beginEnd();
        }
        draw();
        return;
    }

    if (mode_ == Mode::Aim) {
        if (yourTurn()) {
            if (bot_) botLaunch();
            else humanAim(pad);
        } else {
            aimT_ += DT;
            float wait = bot_ ? 0.f : 0.42f;
            if (aimT_ >= wait) cpuLaunch();
        }
    }
    if (mode_ == Mode::Roll) {
        if (balls_.empty()) {
            mode_ = Mode::Aim;
        } else {
            rollT_ += DT;
            stepSim();
            if (rollT_ > 8.f) forceRest();
            if (allRest()) onRest();
        }
    }

    if (mode_ == Mode::Aim && yourTurn()) sys.setLight(30, 160, 50);
    else if (mode_ == Mode::Aim) sys.setLight(170, 30, 40);
    else if (mode_ == Mode::Win) sys.setLight(40, 180, 70);

    draw();
}

}  // namespace bocce

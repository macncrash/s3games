#include "game/eightseven.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace eightseven {
namespace {

constexpr float kPi = 3.14159265f;
constexpr float kMu = 210.f;
constexpr float kVMax = 680.f;
constexpr float kRest = 0.62f;
constexpr int kSub = 6;
constexpr float kDt = 1.f / 60.f;
constexpr float kPower = 0.56f;

bool tracing() {
    const char* e = std::getenv("EIGHTSEVEN_TRACE");
    return e && e[0] && e[0] != '0';
}

float len(float x, float y) { return std::sqrt(x * x + y * y); }

float wrap(float a) {
    while (a > kPi) a -= kPi * 2.f;
    while (a < -kPi) a += kPi * 2.f;
    return a;
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

void Game::park(int n) {
    Ball& b = ball_[n];
    b.down = false;
    b.fell = false;
    b.pocket = -1;
    b.vx = b.vy = 0;
    b.x = b.homeX;
    b.y = b.homeY;
}

void Game::sink(Ball& b, int pocket) {
    if (b.down) return;
    b.down = true;
    b.fell = true;
    b.pocket = pocket;
    b.vx = b.vy = 0;
    b.x = b.y = -400.f;
    pocketSnd_ = true;
}

bool Game::inTable(float x, float y) const {
    if (x < kFeltL + kBallR + 1.5f || x > kFeltR - kBallR - 1.5f) return false;
    if (y < kFeltT + kBallR + 1.5f || y > kFeltB - kBallR - 1.5f) return false;
    for (int i = 0; i < 6; i++) {
        const Pocket& p = pocketAt(i);
        if (len(x - p.x, y - p.y) < p.sink + 2.5f) return false;
    }
    return true;
}

bool Game::overlaps(float x, float y, int ignore) const {
    const float need = kBallR * 2.f + 0.6f;
    for (int i = 0; i < 16; i++) {
        if (i == ignore || ball_[i].down) continue;
        if (len(ball_[i].x - x, ball_[i].y - y) < need) return true;
    }
    return false;
}

bool Game::freePoint(float x, float y, int ignore) const { return inTable(x, y) && !overlaps(x, y, ignore); }

int Game::sunk(int lo, int hi) const {
    int n = 0;
    for (int i = lo; i <= hi; i++)
        if (ball_[i].down) n++;
    return n;
}

bool Game::audit() const {
    if (ball_[8].down) return false;
    if (sunk(1, 7) != you_) return false;
    if (sunk(9, 15) != them_) return false;
    if (you_ < kRace || them_ >= kRace || you_ <= them_) return false;
    return true;
}

void Game::rack() {
    for (int i = 0; i < 16; i++) ball_[i] = Ball{};
    ball_[0].homeX = kHeadX;
    ball_[0].homeY = kMidY;
    for (int n = 1; n <= 7; n++) {
        ball_[n].homeX = kRailL;
        ball_[n].homeY = kParkY0 + float(n - 1) * kParkStep;
    }
    ball_[8].homeX = kFootX;
    ball_[8].homeY = kMidY;
    for (int n = 9; n <= 15; n++) {
        ball_[n].homeX = kRailR;
        ball_[n].homeY = kParkY0 + float(n - 9) * kParkStep;
    }
    for (int i = 0; i < 16; i++) park(i);
    if (tracing()) {
        for (int i = 0; i < 16; i++) {
            if (!freePoint(ball_[i].x, ball_[i].y, i))
                std::fprintf(stderr, "s3eightseven crowd %d\n", i);
        }
    }
}

void Game::begin() {
    you_ = 0;
    them_ = 0;
    shots_ = 0;
    turn_ = 0;
    retries_ = 0;
    retry_ = false;
    yours_ = true;
    won_ = false;
    over_ = false;
    rules_ = false;
    guided_ = false;
    fanStep_ = -1;
    fanT_ = 0;
    say_ = "BREAK";
    rack();
    setupShot();
    mode_ = Mode::Aim;
    think_ = 0;
    if (sys_) sys_->setLight(40, 130, 60);
}

void Game::setupShot() {
    int id = yours_ ? 1 + you_ : 9 + them_;
    if (id < 1 || id > 15 || id == 8 || ball_[id].down) {
        std::fprintf(stderr, "s3eightseven no ball id %d you %d them %d\n", id, you_, them_);
        fail("NO BALL");
        return;
    }
    live_ = id;
    upShot_ = (turn_ & 1) == 0;
    solvedPocket_ = upShot_ ? 1 : 4;
    called_ = solvedPocket_;
    const float objY = upShot_ ? 80.f : 148.f;
    const float cueY = upShot_ ? 148.f : 80.f;
    solvedAim_ = upShot_ ? -kPi * 0.5f : kPi * 0.5f;
    aim_ = solvedAim_;
    power_ = kPower;
    for (int i = 0; i < 16; i++) {
        ball_[i].fell = false;
        ball_[i].pocket = -1;
        ball_[i].vx = ball_[i].vy = 0;
    }
    park(id);
    ball_[id].x = kShotX;
    ball_[id].y = objY;
    ball_[0].down = false;
    ball_[0].fell = false;
    ball_[0].pocket = -1;
    ball_[0].vx = ball_[0].vy = 0;
    ball_[0].x = kShotX;
    ball_[0].y = cueY;
    if (tracing() && !freePoint(ball_[id].x, ball_[id].y, id))
        std::fprintf(stderr, "s3eightseven blocked object %d\n", id);
    if (tracing() && !freePoint(ball_[0].x, ball_[0].y, 0))
        std::fprintf(stderr, "s3eightseven blocked cue\n");
}

void Game::shoot() {
    if (mode_ != Mode::Aim || live_ < 1) return;
    float sp = std::clamp(power_, 0.15f, 1.f) * kVMax;
    float err = std::fabs(wrap(aim_ - solvedAim_));
    bool straight = err < 0.08f && std::fabs(ball_[0].x - kShotX) < 0.5f && std::fabs(ball_[live_].x - kShotX) < 0.5f;
    guided_ = straight && (bot_ || !yours_ || err < 0.02f);
    if (bot_ || !yours_) {
        called_ = solvedPocket_;
        aim_ = solvedAim_;
        guided_ = true;
        ball_[0].x = kShotX;
        ball_[live_].x = kShotX;
    }
    if (guided_) {
        ball_[0].vx = 0;
        ball_[0].vy = upShot_ ? -sp : sp;
    } else {
        ball_[0].vx = std::cos(aim_) * sp;
        ball_[0].vy = std::sin(aim_) * sp;
    }
    for (int i = 0; i < 16; i++) {
        ball_[i].fell = false;
        ball_[i].pocket = -1;
    }
    ball_[0].down = false;
    mode_ = Mode::Roll;
    rollT_ = 0;
    shots_++;
    if (sys_) {
        sys_->apu.noiseBurst(0.42f, 2400.f, 0.04f);
        sys_->rumble(0.12f, 0.3f, 30);
    }
}

void Game::physics(float dt) {
    const float h = dt / float(kSub);
    for (int s = 0; s < kSub; s++) {
        auto lock = [&]() {
            if (!guided_) return;
            if (!ball_[0].down) {
                ball_[0].x = kShotX;
                ball_[0].vx = 0;
            }
            if (live_ > 0 && !ball_[live_].down) {
                ball_[live_].x = kShotX;
                ball_[live_].vx = 0;
            }
        };
        lock();
        for (int i = 0; i < 16; i++) {
            Ball& b = ball_[i];
            if (b.down) continue;
            for (int p = 0; p < 6; p++) {
                const Pocket& pk = pocketAt(p);
                float dx = pk.x - b.x, dy = pk.y - b.y;
                float d = len(dx, dy);
                if (d >= pk.sink && d < pk.sink + 4.f && d > 0.5f) {
                    float nx = dx / d, ny = dy / d;
                    if (b.vx * nx + b.vy * ny > 30.f) {
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
        lock();
        for (int pass = 0; pass < 4; pass++) {
            for (int i = 0; i < 16; i++) {
                if (ball_[i].down) continue;
                for (int j = i + 1; j < 16; j++) {
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
                    float push = (minD - d) * 0.5f;
                    ball_[i].x -= nx * push;
                    ball_[i].y -= ny * push;
                    ball_[j].x += nx * push;
                    ball_[j].y += ny * push;
                }
            }
        }
        lock();
        auto pocketPass = [&]() {
            for (int i = 0; i < 16; i++) {
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
        for (int i = 0; i < 16; i++) {
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
        lock();
    }
}

bool Game::anyMoving(float v) const {
    for (int i = 0; i < 16; i++) {
        if (ball_[i].down) continue;
        if (len(ball_[i].vx, ball_[i].vy) > v) return true;
    }
    return false;
}

void Game::forceStop() {
    for (int i = 0; i < 16; i++) {
        Ball& b = ball_[i];
        if (b.down) continue;
        int best = -1;
        float bd = 1e9f;
        for (int p = 0; p < 6; p++) {
            float d = len(b.x - pocketAt(p).x, b.y - pocketAt(p).y);
            if (d < pocketAt(p).sink && d < bd) {
                bd = d;
                best = p;
            }
        }
        if (best >= 0) sink(b, best);
        else b.vx = b.vy = 0;
    }
}

void Game::finish() {
    rules_ = audit();
    won_ = rules_;
    over_ = true;
    mode_ = rules_ ? Mode::Leave : Mode::Lose;
    say_ = rules_ ? "FIRST TO SEVEN" : "SHORT";
    fanStep_ = rules_ ? 0 : -1;
    fanT_ = 0;
    if (!rules_) std::fprintf(stderr, "s3eightseven audit you %d them %d solids %d stripes %d eight %d\n", you_, them_,
                              sunk(1, 7), sunk(9, 15), eightUp() ? 1 : 0);
    if (!sys_) return;
    if (rules_) {
        sys_->setLight(255, 210, 48);
        sys_->rumble(0.45f, 0.9f, 180);
    } else {
        sys_->setLight(160, 40, 30);
    }
}

void Game::fail(const char* why) {
    say_ = why;
    won_ = false;
    rules_ = false;
    over_ = true;
    mode_ = Mode::Lose;
    if (sys_) sys_->setLight(160, 36, 32);
}

void Game::resolve() {
    guided_ = false;
    const bool scratch = ball_[0].fell;
    const bool eightFell = ball_[8].fell || ball_[8].down;
    const bool made = live_ > 0 && ball_[live_].fell && ball_[live_].pocket == called_ && !scratch && !eightFell &&
                      ((yours_ && live_ >= 1 && live_ <= 7) || (!yours_ && live_ >= 9 && live_ <= 15));
    if (eightFell) park(8);
    for (int i = 1; i <= 15; i++) {
        if (i == live_) continue;
        if (ball_[i].fell || (ball_[i].down && i != 8)) {
            if (!(ball_[i].down && ((i <= 7 && i <= you_) || (i >= 9 && (i - 9) < them_)))) park(i);
        }
    }
    // Scored balls stay down. Anything else that fell this stroke, including a wrong pocket, comes back.
    if (!made && live_ > 0) park(live_);
    if (ball_[0].down || ball_[0].fell) park(0);

    if (made) {
        if (yours_) you_++;
        else them_++;
        say_ = "CALLED";
        retries_ = 0;
        retry_ = false;
        if (sys_) sys_->rumble(0.28f, 0.55f, 50);
        if (you_ >= kRace && you_ > them_) {
            finish();
            return;
        }
        if (them_ >= kRace && them_ >= you_) {
            fail("THEY REACHED SEVEN");
            return;
        }
        yours_ = !yours_;
        turn_++;
    } else {
        if (eightFell) say_ = "EIGHT STAYS";
        else if (scratch) say_ = "SCRATCH";
        else if (live_ > 0 && ball_[live_].fell) say_ = "WRONG POCKET";
        else say_ = "MISS";
        // The live ball was parked above, which cleared fell. The say already captured why.
        if ((bot_ || !yours_) && retries_ < 4) {
            retries_++;
            retry_ = true;
            if (bot_) std::fprintf(stderr, "s3eightseven miss shot %d retry %d\n", shots_, retries_);
        } else {
            retries_ = 0;
            retry_ = false;
            yours_ = !yours_;
            turn_++;
        }
    }
    if (tracing()) std::fprintf(stderr, "rest you %d them %d %s turn %d\n", you_, them_, say_, turn_);
    mode_ = Mode::Hold;
    holdT_ = 0;
}

void Game::blip(float freq, float vol) {
    if (!sys_) return;
    sys_->apu.tone(0, freq, vol);
    toneT_ = 0.12f;
}

void Game::silence() {
    if (!sys_ || toneT_ <= 0.f) return;
    toneT_ -= kDt;
    if (toneT_ <= 0.f) {
        sys_->apu.tone(0, 0, 0);
        sys_->apu.tone(1, 0, 0);
        sys_->apu.tone(2, 0, 0);
    }
}

void Game::fanfare() {
    if (!sys_ || fanStep_ < 0) return;
    static const float notes[] = {523.25f, 659.25f, 783.99f, 1046.5f};
    int step = int(fanT_ / 0.12f);
    if (step != fanStep_ && step >= 0 && step < 4) {
        sys_->apu.tone(0, notes[step], 0.12f);
        sys_->apu.tone(1, notes[step] * 0.5f, 0.07f);
        fanStep_ = step;
        toneT_ = 0.2f;
    } else if (step >= 6 && fanStep_ != 99) {
        sys_->apu.tone(0, 0, 0);
        sys_->apu.tone(1, 0, 0);
        fanStep_ = 99;
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow) {
    if (!sys_ || h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - w * 0.5f));
    s.y = int16_t(std::lround(cy - h * 0.5f));
    s.img = m.pick(h);
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
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        int tile = art_.font[c - 32];
        if (!tile) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(tile, pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = s ? int(std::strlen(s)) : 0;
    hud(20 - n / 2, row, s, pal);
}

void Game::pips(char* out, int n) const {
    for (int i = 0; i < kRace; i++) out[i] = i < n ? '#' : '-';
    out[kRace] = 0;
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    v.A.enabled = true;
    v.B.enabled = false;
    v.hudEnabled = true;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        bool nap = ((y / 3) & 1) == 0;
        v.lineBackdrop[y] = nap ? gs::rgb4(1, 9, 3) : gs::rgb4(2, 12, 5);
    }
}

void Game::aimAid() {
    if (mode_ != Mode::Aim && mode_ != Mode::Title && mode_ != Mode::Pause) return;
    if (ball_[0].down) return;
    float dx = std::cos(aim_), dy = std::sin(aim_);
    int n = 6;
    for (int i = 1; i <= n; i++) spr(art_.dot, ball_[0].x + dx * float(i) * 12.f, ball_[0].y + dy * float(i) * 12.f, 4.f, PAL_GOLD);
}

void Game::cueDraw() {
    if (mode_ == Mode::Roll || mode_ == Mode::Leave || mode_ == Mode::Lose) return;
    if (ball_[0].down) return;
    int qi = int(std::lround(aim_ / (kPi * 2.f) * float(kCueAngles)));
    qi %= kCueAngles;
    if (qi < 0) qi += kCueAngles;
    float q = float(qi) * (kPi * 2.f / float(kCueAngles));
    float reach = kBallR + 6.f + kCueTip;
    float cx = ball_[0].x - std::cos(q) * reach;
    float cy = ball_[0].y - std::sin(q) * reach;
    spr(art_.cue[qi], cx, cy, float(kCueBox), PAL_CUE);
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    if (mode_ == Mode::Title) {
        spr(art_.logo, 160.f, 46.f, float(art_.logo.h), PAL_LOGO);
        spr(art_.sub, 160.f, 68.f, float(art_.sub.h), PAL_LOGO);
    } else if (mode_ == Mode::Leave) {
        spr(art_.leave, 160.f, 58.f, float(art_.leave.h), PAL_WIN);
    } else if (mode_ == Mode::Lose) {
        spr(art_.lost, 160.f, 58.f, float(art_.lost.h), PAL_RED);
    }

    if (mode_ != Mode::Lose) {
        const Pocket& pk = pocketAt(called_);
        float pulse = 22.f + std::sin(t_ * 7.f) * 2.f;
        spr(art_.glow, pk.x, pk.y, pulse, PAL_GLOW);
    }

    for (int i = 0; i < 16; i++) {
        if (ball_[i].down) continue;
        spr(art_.ball[i], ball_[i].x, ball_[i].y, kBallR * 2.f, PAL_BALL);
    }
    aimAid();
    cueDraw();
    for (int i = 0; i < 16; i++) {
        if (ball_[i].down) continue;
        spr(art_.shadow, ball_[i].x + 3.f, ball_[i].y + 3.2f, 6.5f, PAL_BALL, true);
    }

    char yp[8], tp[8], buf[48];
    pips(yp, you_);
    pips(tp, them_);
    hud(1, 0, "S3 EIGHT SEVEN", PAL_GOLD);
    std::snprintf(buf, sizeof buf, "YOU %d %s  THEM %d %s", you_, yp, them_, tp);
    hud(1, 1, buf, PAL_HUD);

    if (mode_ == Mode::Title) {
        hudC(26, "SOLIDS YOU   STRIPES THEM   8 STAYS", PAL_HUD);
        hudC(27, "RETURN STARTS    ESC LEAVES", PAL_GOLD);
        return;
    }
    if (mode_ == Mode::Leave) {
        std::snprintf(buf, sizeof buf, "YOU %d   THEM %d", you_, them_);
        hudC(26, "FIRST TO SEVEN", PAL_WIN);
        hudC(27, "ESC LEAVES", PAL_GOLD);
        return;
    }
    if (mode_ == Mode::Lose) {
        hudC(26, say_ ? say_ : "SHORT", PAL_RED);
        hudC(27, "RETURN RERACKS   ESC LEAVES", PAL_GOLD);
        return;
    }
    if (mode_ == Mode::Pause) {
        hudC(26, "PAUSED", PAL_GOLD);
        hudC(27, "RETURN RESUMES   ESC TITLE", PAL_HUD);
        return;
    }

    const char* msg = yours_ ? "YOUR SHOT" : "THEIR SHOT";
    if (mode_ == Mode::Roll) msg = "ROLLING";
    else if (mode_ == Mode::Hold && say_) msg = say_;
    std::snprintf(buf, sizeof buf, "%s  %s", msg, pocketAt(called_).name);
    hudC(26, buf, mode_ == Mode::Hold && say_ && std::strcmp(say_, "CALLED") != 0 ? PAL_RED : PAL_HUD);

    if (mode_ == Mode::Aim && yours_ && !bot_) {
        int bars = std::clamp(int(std::lround(power_ * 8.f)), 0, 8);
        char pwr[16];
        int p = std::snprintf(pwr, sizeof pwr, "PWR ");
        for (int i = 0; i < 8 && p + 1 < int(sizeof pwr); i++) pwr[p++] = i < bars ? '#' : '-';
        pwr[p] = 0;
        hud(1, 27, pwr, PAL_GOLD);
        hud(16, 27, "Z SHOOT  C CALL", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Aim && !yours_) {
        hudC(27, "THEIR BALL", PAL_RED);
        return;
    }
    hudC(27, yours_ ? "SOLIDS" : "STRIPES", yours_ ? PAL_GOLD : PAL_RED);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.8f);
    sys.apu.setEcho(0.12f, 0.22f, 0.14f);
    rack();
    called_ = 1;
    aim_ = 0.f;
    mode_ = Mode::Title;
    t_ = 0;
    say_ = "FIRST TO SEVEN";
    sys.setLight(170, 120, 40);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    silence();
    pocketSnd_ = false;
    clack_ = false;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        ball_[0].x = kHeadX;
        ball_[0].y = kMidY;
        aim_ = std::sin(t_ * 0.7f) * 0.35f;
        called_ = 1;
        if (bot_ && t_ > 0.35f) begin();
        else if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) begin();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = held_;
        else if (pad.pressed(gs::BTN_MODE)) {
            rack();
            you_ = them_ = shots_ = turn_ = 0;
            won_ = over_ = rules_ = false;
            yours_ = true;
            mode_ = Mode::Title;
            t_ = 0;
        }
    } else if (mode_ == Mode::Leave) {
        fanT_ += kDt;
        fanfare();
        if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        } else if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
            begin();
        }
    } else if (mode_ == Mode::Lose) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) begin();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        }
    } else if (mode_ == Mode::Hold) {
        holdT_ += kDt;
        float need = bot_ ? 0.05f : 0.35f;
        if (holdT_ >= need) {
            if (mode_ == Mode::Hold) setupShot();
            if (mode_ != Mode::Lose) {
                if (retry_) power_ = std::min(0.92f, kPower + 0.12f * float(retries_));
                mode_ = Mode::Aim;
                think_ = 0;
                if (sys_) sys_->setLight(yours_ ? 40 : 170, yours_ ? 130 : 46, yours_ ? 60 : 36);
            }
        }
    } else if (mode_ == Mode::Roll) {
        rollT_ += kDt;
        physics(kDt);
        if (rollT_ > 3.2f) {
            forceStop();
            resolve();
        } else if (!anyMoving(16.f)) {
            resolve();
        }
        if (mode_ == Mode::Roll) {
            if (pocketSnd_) blip(740.f, 0.1f);
            else if (clack_) sys.apu.noiseBurst(0.16f, 1800.f, 0.03f);
        } else if (pocketSnd_) {
            blip(698.f, 0.11f);
            if (sys_) sys_->apu.tone(1, 880.f, 0.06f);
        }
    } else if (mode_ == Mode::Aim) {
        bool cpu = bot_ || !yours_;
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            held_ = Mode::Aim;
            mode_ = Mode::Pause;
        } else if (!bot_ && yours_ && pad.pressed(gs::BTN_MODE)) {
            rack();
            you_ = them_ = shots_ = turn_ = 0;
            won_ = over_ = rules_ = false;
            mode_ = Mode::Title;
            t_ = 0;
        } else if (cpu) {
            aim_ = solvedAim_;
            called_ = solvedPocket_;
            power_ = retry_ ? std::min(0.92f, kPower + 0.12f * float(retries_)) : kPower;
            think_ += kDt;
            float wait = bot_ ? 0.03f : 0.45f;
            if (think_ >= wait) shoot();
        } else {
            if (pad.pressed(gs::BTN_C)) called_ = (called_ + 1) % 6;
            if (pad.pressed(gs::BTN_X)) called_ = (called_ + 5) % 6;
            float rate = (pad.down(gs::BTN_Y) || pad.down(gs::BTN_Z)) ? 0.7f : 2.1f;
            if (std::fabs(pad.axisX) > 0.18f) aim_ += pad.axisX * 2.4f * kDt;
            else {
                if (pad.down(gs::BTN_LEFT)) aim_ -= rate * kDt;
                if (pad.down(gs::BTN_RIGHT)) aim_ += rate * kDt;
            }
            aim_ = wrap(aim_);
            if (pad.down(gs::BTN_UP)) power_ = std::min(1.f, power_ + 0.012f);
            if (pad.down(gs::BTN_DOWN)) power_ = std::max(0.16f, power_ - 0.012f);
            if (std::fabs(pad.axisY) > 0.25f) power_ = std::clamp(power_ + pad.axisY * 0.5f * kDt, 0.16f, 1.f);
            if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_TURBO)) shoot();
        }
    }

    draw();
}

}  // namespace eightseven

#include "game/putttape.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace putttape {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kOx = 8.f;
constexpr float kOy = 30.f;
constexpr float kGw = 196.f;
constexpr float kGh = 136.f;
constexpr float kMu = 64.f;
constexpr float kAx = 26.f;
constexpr float kAy = 5.f;
constexpr float kStop = 7.f;
constexpr float kBounce = 0.38f;
constexpr float kTeeX = 100.f;
constexpr float kTeeY = 116.f;
constexpr float kBallR = 4.f;
constexpr int kMaxPutts = 5;
constexpr int kTapeN = 3;
constexpr int kCupN = 4;
constexpr float kPi = 3.14159265f;

struct Cup {
    const char* name;
    int cents;
    bool tape;
    float x, y;
    float hit;
    float sink;
    int pal;
};

// Token sits off the tape and is worth the same .25 as the quarter.
const Cup kCup[kCupN] = {
    {"QUARTER", 25, true, 50.f, 30.f, 12.f, 220.f, PAL_GOLD},
    {"DIME", 10, true, 152.f, 36.f, 12.f, 220.f, PAL_SILVER},
    {"NICKEL", 5, true, 104.f, 66.f, 12.f, 210.f, PAL_COPPER},
    {"TOKEN", 25, false, 164.f, 98.f, 12.f, 210.f, PAL_TOKEN},
};

enum class Halt { Fly, Rest, Hole };

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

void launch(Game::Lie& b, float ang, float spd) {
    b.x = kTeeX;
    b.y = kTeeY;
    b.vx = std::cos(ang) * spd;
    b.vy = std::sin(ang) * spd;
    b.rest = false;
}

Halt advance(Game::Lie& b, int& hole) {
    hole = -1;
    if (b.rest) return Halt::Rest;
    b.vx += kAx * kDt;
    b.vy += kAy * kDt;
    float sp = std::hypot(b.vx, b.vy);
    float drop = kMu * kDt;
    if (sp <= drop) {
        b.vx = b.vy = 0;
    } else {
        float s = (sp - drop) / sp;
        b.vx *= s;
        b.vy *= s;
    }
    b.x += b.vx * kDt;
    b.y += b.vy * kDt;

    int nearest = -1;
    float best = 1e9f;
    for (int i = 0; i < kCupN; i++) {
        float d = std::hypot(b.x - kCup[i].x, b.y - kCup[i].y);
        if (d < kCup[i].hit && d < best) {
            best = d;
            nearest = i;
        }
    }
    if (nearest >= 0) {
        const Cup& c = kCup[nearest];
        sp = std::hypot(b.vx, b.vy);
        if (sp <= c.sink) {
            b.x = c.x;
            b.y = c.y;
            b.vx = b.vy = 0;
            b.rest = true;
            hole = nearest;
            return Halt::Hole;
        }
        float dx = b.x - c.x;
        float dy = b.y - c.y;
        float d = best > 0.001f ? best : 0.001f;
        if (best <= 0.001f) {
            float sp2 = sp > 0.001f ? sp : 1.f;
            dx = b.vx / sp2;
            dy = b.vy / sp2;
        }
        float nx = dx / d;
        float ny = dy / d;
        b.x = c.x + nx * (c.hit + 0.75f);
        b.y = c.y + ny * (c.hit + 0.75f);
        float vn = b.vx * nx + b.vy * ny;
        if (vn < 0) {
            b.vx -= 1.55f * vn * nx;
            b.vy -= 1.55f * vn * ny;
        }
    }

    if (b.x < kBallR) {
        b.x = kBallR;
        if (b.vx < 0) b.vx = -b.vx * kBounce;
    }
    if (b.x > kGw - kBallR) {
        b.x = kGw - kBallR;
        if (b.vx > 0) b.vx = -b.vx * kBounce;
    }
    if (b.y < kBallR) {
        b.y = kBallR;
        if (b.vy < 0) b.vy = -b.vy * kBounce;
    }
    if (b.y > kGh - kBallR) {
        b.y = kGh - kBallR;
        if (b.vy > 0) b.vy = -b.vy * kBounce;
    }

    sp = std::hypot(b.vx, b.vy);
    if (sp < kStop) {
        b.vx = b.vy = 0;
        b.rest = true;
        return Halt::Rest;
    }
    return Halt::Fly;
}

struct Trace {
    int n = 0;
    int hole = -1;
    float x[14] = {};
    float y[14] = {};
};

Trace traceShot(float ang, float spd) {
    Trace t;
    Game::Lie b;
    launch(b, ang, spd);
    for (int f = 0; f < 360; f++) {
        int hole = -1;
        Halt h = advance(b, hole);
        if ((f % 8) == 7 && t.n < 14) {
            t.x[t.n] = b.x;
            t.y[t.n] = b.y;
            t.n++;
        }
        if (h == Halt::Hole) {
            t.hole = hole;
            break;
        }
        if (h == Halt::Rest) break;
    }
    return t;
}

float slotX(int i) { return 36.f + float(i) * 48.f; }

}  // namespace

const char* Game::tapeLabel(int i) const {
    if (i < 0 || i >= kTapeN) return "";
    return kCup[i].name;
}

int Game::drawerCents() const {
    int s = 0;
    for (int i = 0; i < kTapeN; i++)
        if (have_[i]) s += kCup[i].cents;
    return s;
}

int Game::nextCup() const {
    for (int i = 0; i < kTapeN; i++)
        if (!have_[i]) return i;
    return 0;
}

int Game::rollCup(float ang, float spd) const {
    Lie b;
    launch(b, ang, spd);
    for (int i = 0; i < 420; i++) {
        int hole = -1;
        Halt h = advance(b, hole);
        if (h == Halt::Hole) return hole;
        if (h == Halt::Rest) return -1;
    }
    return -1;
}

bool Game::solveCup(int cup, float& ang, float& spd) const {
    float base = std::atan2(kCup[cup].y - kTeeY, kCup[cup].x - kTeeX);
    bool any = false;
    float best = 1e9f;
    ang = base;
    spd = 160.f;
    for (float da = -1.05f; da <= 1.05f; da += 0.03f) {
        float a = base + da;
        for (float v = 90.f; v <= 230.f; v += 5.f) {
            if (rollCup(a, v) != cup) continue;
            bool wideV = rollCup(a, v - 12.f) == cup && rollCup(a, v + 12.f) == cup;
            bool wideA = rollCup(a - 0.05f, v) == cup && rollCup(a + 0.05f, v) == cup;
            float score = std::fabs(da) * 3.f + std::fabs(v - 160.f) * 0.03f;
            if (!wideV) score += 5.f;
            if (!wideA) score += 5.f;
            if (score < best) {
                best = score;
                ang = a;
                spd = v;
                any = true;
            }
        }
    }
    return any;
}

bool Game::audit() {
    bool ok = true;
    int tape = 0;
    for (int i = 0; i < kTapeN; i++) tape += kCup[i].cents;
    int trap = kCup[3].cents + kCup[1].cents + kCup[2].cents;
    if (tape != 40 || trap != 40 || kCup[3].tape || kCup[3].cents != kCup[0].cents) {
        std::fprintf(stderr, "s3putttape tape arithmetic tape %d trap %d\n", tape, trap);
        ok = false;
    }
    for (int i = 0; i < kCupN; i++) {
        if (solveCup(i, shotAng_[i], shotSpd_[i]) && rollCup(shotAng_[i], shotSpd_[i]) == i) continue;
        std::fprintf(stderr, "s3putttape: %s unreachable\n", kCup[i].name);
        float base = std::atan2(kCup[i].y - kTeeY, kCup[i].x - kTeeX);
        for (float v = 80.f; v <= 220.f; v += 20.f) {
            std::fprintf(stderr, "  straight v %.0f -> %d\n", v, rollCup(base, v));
        }
        ok = false;
    }
    return ok;
}

void Game::blip(float freq, float vol) {
    if (!sys_) return;
    sys_->apu.tone(0, freq, vol);
    beep_ = std::max(beep_, 0.09f);
}

void Game::toTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    swinging_ = false;
    putts_ = 0;
    flying_ = -1;
    lastHole_ = -1;
    shake_ = 0;
    walk_ = 0;
    meter_ = 0;
    meterDir_ = 1.f;
    aimT_ = rollT_ = flyT_ = judgeT_ = leaveT_ = loseT_ = 0;
    reason_ = "";
    for (int i = 0; i < kTapeN; i++) have_[i] = false;
    ball_ = Lie{};
    ball_.x = kTeeX;
    ball_.y = kTeeY;
    ball_.rest = true;
    aim_ = layoutOk_ ? shotAng_[0] : std::atan2(kCup[0].y - kTeeY, kCup[0].x - kTeeX);
    botSpd_ = shotSpd_[0] > 1.f ? shotSpd_[0] : 160.f;
}

void Game::newGame() {
    won_ = false;
    over_ = false;
    putts_ = 0;
    reason_ = "";
    for (int i = 0; i < kTapeN; i++) have_[i] = false;
    beginAim();
}

void Game::beginAim() {
    ball_ = Lie{};
    ball_.x = kTeeX;
    ball_.y = kTeeY;
    ball_.rest = true;
    flying_ = -1;
    swinging_ = false;
    meter_ = 0;
    meterDir_ = 1.f;
    aimT_ = 0;
    rollT_ = 0;
    int c = nextCup();
    float base = std::atan2(kCup[c].y - kTeeY, kCup[c].x - kTeeX);
    aim_ = (bot_ && layoutOk_) ? shotAng_[c] : base;
    botSpd_ = shotSpd_[c] > 1.f ? shotSpd_[c] : 160.f;
    mode_ = Mode::Aim;
}

void Game::putt(float ang, float spd) {
    aim_ = ang;
    launch(ball_, ang, spd);
    swinging_ = false;
    rollT_ = 0;
    putts_++;
    mode_ = Mode::Roll;
    if (!sys_) return;
    sys_->apu.noiseBurst(0.18f, 1600.f, 0.05f);
    blip(196.f, 0.05f);
}

void Game::settle(int hole) {
    ball_.vx = ball_.vy = 0;
    ball_.rest = true;
    lastHole_ = hole;
    if (hole >= 0 && hole < kTapeN && !have_[hole]) {
        have_[hole] = true;
        flying_ = hole;
        flyT_ = 0;
        mode_ = Mode::Pocket;
        reason_ = "IN THE DRAWER";
        if (sys_) {
            sys_->apu.tone(1, 523.f + float(hole) * 80.f, 0.07f);
            beep_ = std::max(beep_, 0.22f);
            sys_->rumble(0.2f, 0.45f, 80);
            sys_->setLight(80, 170, 70);
        }
        return;
    }
    if (hole == 3 && !have_[0]) reason_ = "CLOSE COUNT";
    else if (hole == 3) reason_ = "NOT THE TAPE";
    else if (hole >= 0 && hole < kTapeN) reason_ = "ALREADY IN";
    else if (hole >= 0) reason_ = "NOT THE TAPE";
    else reason_ = "SHORT";
    judgeT_ = 0;
    shake_ = 8;
    mode_ = Mode::Judge;
    if (!sys_) return;
    sys_->apu.tone(1, reason_[0] == 'C' ? 110.f : 82.f, 0.08f);
    sys_->apu.noiseBurst(0.3f, 240.f, 0.12f);
    beep_ = std::max(beep_, 0.28f);
    sys_->rumble(0.45f, 0.1f, 90);
    sys_->setLight(170, 36, 28);
}

void Game::beginLeave() {
    if (!matched()) return;
    mode_ = Mode::Leave;
    leaveT_ = 0;
    walk_ = 0;
    reason_ = "THE DRAWER MATCHES THE TAPE";
    if (!sys_) return;
    sys_->apu.tone(1, 523.f, 0.06f);
    beep_ = std::max(beep_, 0.5f);
    sys_->rumble(0.25f, 0.6f, 160);
    sys_->setLight(255, 200, 80);
}

void Game::beginLose() {
    mode_ = Mode::Lose;
    loseT_ = 0;
    won_ = false;
    if (!reason_ || !reason_[0]) reason_ = "DOES NOT MATCH";
    if (sys_) sys_->setLight(150, 28, 28);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.setFogColor(gs::rgb4(3, 6, 8));
    sys.apu.setMaster(0.74f);
    sys.apu.setEcho(0.08f, 0.16f, 0.08f);
    layoutOk_ = audit();
    toTitle();
    if (!layoutOk_) {
        won_ = false;
        over_ = true;
        mode_ = Mode::Lose;
        reason_ = "LAYOUT";
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_++;
    clock_ += kDt;
    if (beep_ > 0) {
        beep_ -= kDt;
        if (beep_ <= 0) {
            sys.apu.tone(0, 0, 0);
            sys.apu.tone(1, 0, 0);
            sys.apu.tone(2, 0, 0);
        }
    }
    if (shake_ > 0) shake_--;

    if (!layoutOk_) {
        won_ = false;
        over_ = true;
        mode_ = Mode::Lose;
        reason_ = "LAYOUT";
        draw();
        return;
    }

    bool start = false, back = false, hold = false;
    float slide = 0;
    if (bot_) {
        if (mode_ == Mode::Title && clock_ > 0.4f) start = true;
    } else {
        const gs::Pad& pad = sys.pad;
        start = pad.pressed(gs::BTN_START);
        back = pad.pressed(gs::BTN_MODE);
        hold = pad.down(gs::BTN_A) || pad.down(gs::BTN_C) || pad.down(gs::BTN_TURBO);
        if (pad.down(gs::BTN_LEFT)) slide -= 1.f;
        if (pad.down(gs::BTN_RIGHT)) slide += 1.f;
        if (std::fabs(pad.axisX) > 0.18f) slide = pad.axisX;
    }

    if (mode_ == Mode::Title) {
        if (back && !bot_) sys.quit();
        else if (start) newGame();
    } else if (mode_ == Mode::Pause) {
        if (start) mode_ = heldMode_;
        else if (back && !bot_) toTitle();
    } else if (mode_ == Mode::Over) {
        if (!bot_ && start) newGame();
        else if (!bot_ && back) toTitle();
    } else if (mode_ == Mode::Lose) {
        loseT_ += kDt;
        if (loseT_ > 0.45f) over_ = true;
        if (!bot_ && start) newGame();
        else if (!bot_ && back) toTitle();
    } else if (back && !bot_) {
        toTitle();
    } else if (start && !bot_ && mode_ == Mode::Aim) {
        heldMode_ = mode_;
        mode_ = Mode::Pause;
    } else if (mode_ == Mode::Aim) {
        aimT_ += kDt;
        if (bot_) {
            if (aimT_ > 0.28f) putt(aim_, botSpd_);
        } else {
            aim_ = clampf(aim_ + slide * 1.7f * kDt, -2.9f, -0.05f);
            if (hold && !swinging_) {
                swinging_ = true;
                meter_ = 0;
                meterDir_ = 1.f;
                blip(440.f, 0.04f);
            }
            if (swinging_ && hold) {
                float prev = meter_;
                meter_ += meterDir_ * kDt / 0.85f;
                if (meter_ >= 1.f) {
                    meter_ = 1.f;
                    meterDir_ = -1.f;
                }
                if (meter_ <= 0.f) {
                    meter_ = 0.f;
                    meterDir_ = 1.f;
                }
                if (prev < 0.5f && meter_ >= 0.5f) blip(720.f, 0.035f);
            }
            if (swinging_ && !hold) putt(aim_, 80.f + meter_ * 160.f);
        }
    } else if (mode_ == Mode::Roll) {
        rollT_ += kDt;
        int hole = -1;
        Halt h = advance(ball_, hole);
        if (h == Halt::Hole) settle(hole);
        else if (h == Halt::Rest || rollT_ > 6.f) settle(-1);
    } else if (mode_ == Mode::Pocket) {
        flyT_ += kDt;
        int n = int(flyT_ * 60.f);
        if (n == 8) sys.apu.tone(2, 659.f, 0.05f);
        if (n == 16) sys.apu.tone(2, 784.f, 0.05f);
        if (flyT_ > 0.46f) {
            flying_ = -1;
            if (matched()) beginLeave();
            else if (putts_ >= kMaxPutts) beginLose();
            else beginAim();
        }
    } else if (mode_ == Mode::Judge) {
        judgeT_ += kDt;
        if (judgeT_ > 0.55f) {
            if (putts_ >= kMaxPutts) beginLose();
            else beginAim();
        }
    } else if (mode_ == Mode::Leave) {
        leaveT_ += kDt;
        walk_ += 78.f * kDt;
        int n = int(leaveT_ * 60.f);
        if (n == 10) sys.apu.tone(1, 659.f, 0.06f);
        if (n == 20) sys.apu.tone(1, 784.f, 0.06f);
        if (n == 32) sys.apu.tone(1, 1046.f, 0.07f);
        if (leaveT_ > 1.05f) {
            won_ = matched() && putts_ >= 3 && putts_ <= kMaxPutts && drawerCents() == 40;
            over_ = true;
            mode_ = Mode::Over;
            reason_ = won_ ? "LEAVE" : "DOES NOT MATCH";
            if (won_) sys.setLight(255, 210, 90);
        }
    }

    if (mode_ == Mode::Roll) sys.apu.noise(0.028f, 2200.f, true);
    else sys.apu.noise(0, 900.f);

    draw();
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool shadow) {
    if (!sys_ || h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s{};
    s.w = int16_t(std::max(1, std::min(2000, int(std::lround(w)))));
    s.h = int16_t(std::max(1, std::min(2000, int(std::lround(h)))));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(float(s.h));
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::stamp(const gs::Mipped& m, float x, float y, float w, float h, int pal) {
    if (!sys_ || w < 1.f || h < 1.f || m.h < 1) return;
    gs::Sprite s{};
    s.w = int16_t(std::max(1, std::min(2000, int(std::lround(w)))));
    s.h = int16_t(std::max(1, std::min(2000, int(std::lround(h)))));
    s.x = int16_t(std::lround(x));
    s.y = int16_t(std::lround(y));
    s.img = m.pick(float(s.h));
    s.pal = uint8_t(pal);
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

void Game::hudC(int row, const char* s, int pal) {
    if (!s) return;
    hud(20 - int(std::strlen(s)) / 2, row, s, pal);
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    float jx = (shake_ > 0 && (t_ & 1)) ? 1.5f : 0.f;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        if (y < 28) {
            float u = y / 27.f;
            v.lineBackdrop[y] = gs::rgb4(4 + int(u * 5), 7 + int(u * 3), 13 - int(u * 3));
        } else if (y < 172) {
            int band = ((y / 4) & 1);
            int g = 7 + int(((y - 28) / 144.f) * 4);
            v.lineBackdrop[y] = gs::rgb4(1 + band, g - band, 2);
        } else {
            int band = (y / 3) & 1;
            v.lineBackdrop[y] = gs::rgb4(6 + band, 3, 1);
        }
    }

    stamp(art_.paper, 208, 16, 106, 150, PAL_PAPER);
    stamp(art_.wood, kOx - 6, kOy - 8, kGw + 12, 8, PAL_WOOD);
    stamp(art_.wood, kOx - 6, kOy + kGh, kGw + 12, 8, PAL_WOOD);
    stamp(art_.wood, kOx - 6, kOy, 8, kGh, PAL_WOOD);
    stamp(art_.wood, kOx + kGw - 2, kOy, 8, kGh, PAL_WOOD);
    stamp(art_.wood, 0, 170, 320, 8, PAL_WOOD);

    spr(art_.sun, 78, 12, 14, PAL_GOLD);
    spr(art_.cloud, 36 + std::sin(clock_ * 0.3f) * 3.f, 14, 12, PAL_TEXT);
    spr(art_.cloud, 132, 11, 10, PAL_TEXT);
    spr(art_.tree, 24, 26, 34, PAL_TREE);
    spr(art_.tree, 186, 24, 30, PAL_TREE, true);

    const float tuftX[5] = {28, 78, 132, 40, 120};
    const float tuftY[5] = {48, 52, 58, 100, 108};
    for (int i = 0; i < 5; i++) {
        bool blocked = false;
        for (int c = 0; c < kCupN; c++) {
            if (std::hypot(tuftX[i] - kCup[c].x, tuftY[i] - kCup[c].y) < 18.f) blocked = true;
        }
        if (std::hypot(tuftX[i] - ball_.x, tuftY[i] - ball_.y) < 12.f) blocked = true;
        if (blocked) continue;
        float sway = std::sin(clock_ * 1.8f + float(i)) * 1.1f;
        spr(art_.tuft, kOx + tuftX[i] + sway, kOy + tuftY[i], 11, PAL_GREEN);
    }

    for (int i = 0; i < kCupN; i++) {
        float cx = kOx + kCup[i].x + jx;
        float cy = kOy + kCup[i].y;
        spr(art_.slot, cx, cy + 2.f, 8, PAL_SHADE, false, true);
        spr(art_.cup, cx, cy, 13, kCup[i].pal);
        if (i < kTapeN) {
            float wave = std::sin(clock_ * 2.2f + float(i)) * 1.4f;
            spr(art_.flag, cx + wave, cy - 14.f, 16, kCup[i].pal);
        }
    }

    for (int i = 0; i < kTapeN; i++) spr(art_.slot, slotX(i), 200, 10, PAL_WOOD);

    auto coinAt = [&](int i, float x, float y, float h) {
        if (i == 3) spr(art_.token, x, y, h, PAL_TOKEN);
        else spr(art_.coin, x, y, h, kCup[i].pal);
    };

    for (int i = 0; i < kCupN; i++) {
        bool flight = mode_ == Mode::Pocket && flying_ == i;
        bool inDrawer = i < kTapeN && have_[i] && !flight;
        if (inDrawer) coinAt(i, slotX(i), 196, 16);
        else if (!flight) coinAt(i, kOx + kCup[i].x - 10.f, kOy + kCup[i].y - 16.f, 11);
    }
    if (mode_ == Mode::Pocket && flying_ >= 0 && flying_ < kTapeN) {
        float u = clampf(flyT_ / 0.46f, 0.f, 1.f);
        float x0 = kOx + kCup[flying_].x;
        float y0 = kOy + kCup[flying_].y;
        float x1 = slotX(flying_);
        float y1 = 196.f;
        float x = x0 + (x1 - x0) * u;
        float y = y0 + (y1 - y0) * u - std::sin(u * kPi) * 26.f;
        coinAt(flying_, x, y, 12.f + (1.f - u) * 2.f);
    }

    bool sunk = false;
    if (lastHole_ >= 0 && (mode_ == Mode::Pocket || mode_ == Mode::Judge || mode_ == Mode::Leave || mode_ == Mode::Over ||
                           mode_ == Mode::Lose)) {
        sunk = mode_ != Mode::Judge || lastHole_ >= 0;
        if (mode_ == Mode::Aim || mode_ == Mode::Roll || mode_ == Mode::Title || mode_ == Mode::Pause) sunk = false;
    }
    bool hideBall = mode_ == Mode::Leave || mode_ == Mode::Over || (mode_ == Mode::Pocket && flyT_ > 0.2f);
    if (!hideBall) {
        float bx = kOx + ball_.x + jx;
        float by = kOy + ball_.y;
        spr(art_.shadow, bx + 2.f, by + 4.f, 6, PAL_SHADE, false, true);
        spr(art_.ball, bx, by - (sunk ? 0.f : 1.f), sunk ? 8.f : 11.f, PAL_BALL);
    } else if (lastHole_ >= 0 && lastHole_ < kCupN) {
        spr(art_.ball, kOx + kCup[lastHole_].x, kOy + kCup[lastHole_].y, 7, PAL_BALL);
    }

    int pose = (mode_ == Mode::Roll && rollT_ < 0.22f) || (mode_ == Mode::Aim && swinging_ && meter_ > 0.72f) ? 1 : 0;
    float feetX = kOx + kTeeX - 6.f + jx;
    float feetY = kOy + kTeeY + 16.f;
    if (mode_ == Mode::Leave || (mode_ == Mode::Over && won_)) feetX += walk_;
    if (feetX < 360.f) {
        float bob = (mode_ == Mode::Title || mode_ == Mode::Aim) ? std::sin(clock_ * 3.f) * 0.8f : 0.f;
        spr(art_.shadow, feetX, feetY + 2.f, 7, PAL_SHADE, false, true);
        spr(art_.golfer[pose], feetX, feetY - 12.f + bob, 30, PAL_PLAYER);
    }

    bool showArc = mode_ == Mode::Title || mode_ == Mode::Aim || mode_ == Mode::Pause;
    if (showArc) {
        float spd = bot_ ? botSpd_ : (swinging_ ? 80.f + meter_ * 160.f : 160.f);
        if (mode_ == Mode::Title) spd = botSpd_;
        Trace tr = traceShot(aim_, spd);
        int pal = PAL_AIM;
        if (tr.hole >= 0 && tr.hole < kTapeN && !have_[tr.hole]) pal = PAL_GREEN;
        else if (tr.hole == 3 || (tr.hole >= 0 && tr.hole < kTapeN && have_[tr.hole])) pal = PAL_RED;
        for (int i = 0; i < tr.n; i++) spr(art_.dot, kOx + tr.x[i], kOy + tr.y[i], 4.5f, pal);
    }

    char buf[48];
    char need[32] = "NEED";
    int np = 4;
    need[np] = 0;
    if (!have_[0]) {
        std::memcpy(need + np, " QTR", 4);
        np += 4;
        need[np] = 0;
    }
    if (!have_[1]) {
        std::memcpy(need + np, " DIME", 5);
        np += 5;
        need[np] = 0;
    }
    if (!have_[2]) {
        std::memcpy(need + np, " NKL", 4);
        np += 4;
        need[np] = 0;
    }
    if (np == 4) {
        std::memcpy(need, "DRAWER FULL", 12);
    }

    hud(1, 2, "TAPE", PAL_GOLD);
    for (int i = 0; i < kTapeN; i++) {
        char mark = ' ';
        int pal = PAL_TEXT;
        if (have_[i]) {
            mark = '+';
            pal = PAL_GREEN;
        } else if (i == nextCup() && mode_ != Mode::Title && mode_ != Mode::Over) {
            mark = '>';
            pal = PAL_GOLD;
        }
        std::snprintf(buf, sizeof buf, "%c %-7s .%02d", mark, kCup[i].name, kCup[i].cents);
        hud(26, 4 + i * 2, buf, pal);
    }
    hud(26, 10, "TOKEN    .25", PAL_TOKEN);
    hud(26, 11, "NOT THE QTR", PAL_RED);

    std::snprintf(buf, sizeof buf, "TILL .%02d", drawerCents());
    hud(1, 22, buf, matched() ? PAL_GREEN : PAL_GOLD);
    hud(12, 22, "TAPE .40", PAL_TEXT);
    hud(3, 23, "QTR", have_[0] ? PAL_GREEN : PAL_TEXT);
    hud(9, 23, "DIME", have_[1] ? PAL_GREEN : PAL_TEXT);
    hud(15, 23, "NKL", have_[2] ? PAL_GREEN : PAL_TEXT);

    if (mode_ == Mode::Title) {
        hud(1, 0, "S3 PUTTTAPE", PAL_GOLD);
        hud(1, 1, S3_VERSION_STRING, PAL_TEXT);
        hudC(24, "PLAY PUTT UNTIL THE DRAWER", PAL_GOLD);
        hudC(25, "HAS TO MATCH THE TAPE", PAL_GOLD);
        hudC(26, "A CLOSE COUNT IS NOT THE TAPE", PAL_TEXT);
        if ((t_ / 30) % 2 == 0) hudC(27, "PRESS START", PAL_GREEN);
        else hudC(27, "ARROWS AIM   HOLD Z", PAL_TEXT);
    } else if (mode_ == Mode::Pause) {
        hud(1, 0, "S3 PUTTTAPE", PAL_GOLD);
        hudC(26, "PAUSE", PAL_GOLD);
        hudC(27, "START RESUMES", PAL_TEXT);
    } else if (mode_ == Mode::Over && won_) {
        hud(1, 0, "S3 PUTTTAPE", PAL_GOLD);
        hudC(25, "THE DRAWER MATCHES THE TAPE", PAL_GREEN);
        hudC(26, "YOU LEAVE", PAL_GOLD);
        if (!bot_) hudC(27, "START PUTTS AGAIN", PAL_TEXT);
    } else if (mode_ == Mode::Over || mode_ == Mode::Lose) {
        hud(1, 0, "S3 PUTTTAPE", PAL_GOLD);
        hudC(25, "THE DRAWER DOES NOT MATCH", PAL_RED);
        hudC(26, reason_ && reason_[0] ? reason_ : "DOES NOT MATCH", PAL_RED);
        if (!bot_) hudC(27, "START PUTTS AGAIN", PAL_TEXT);
    } else {
        hud(1, 0, "S3 PUTTTAPE", PAL_GOLD);
        int shown = putts_ + (mode_ == Mode::Aim ? 1 : 0);
        if (shown < 1) shown = 1;
        if (shown > kMaxPutts) shown = kMaxPutts;
        std::snprintf(buf, sizeof buf, "STROKE %d/%d", shown, kMaxPutts);
        hud(1, 1, buf, putts_ >= kMaxPutts - 1 ? PAL_RED : PAL_TEXT);

        if (mode_ == Mode::Leave) {
            hudC(26, "THE DRAWER MATCHES THE TAPE", PAL_GREEN);
            hudC(27, "LEAVE", PAL_GOLD);
        } else if (mode_ == Mode::Pocket) {
            hudC(26, "IN THE DRAWER", PAL_GREEN);
            hudC(27, matched() ? "THE TAPE IS MET" : need, PAL_GOLD);
        } else if (mode_ == Mode::Judge) {
            int pal = (reason_ && reason_[0] == 'C') ? PAL_RED : PAL_RED;
            hudC(26, reason_ ? reason_ : "SHORT", pal);
            if (reason_ && std::strcmp(reason_, "CLOSE COUNT") == 0) hudC(27, "TOKEN IS NOT THE QUARTER", PAL_TOKEN);
            else if (putts_ >= kMaxPutts) hudC(27, "NO STROKES LEFT", PAL_RED);
            else hudC(27, "NOT IN THE DRAWER", PAL_TEXT);
        } else if (mode_ == Mode::Roll) {
            hudC(26, "ROLLING", PAL_GOLD);
            hudC(27, need, PAL_TEXT);
        } else if (swinging_) {
            int n = int(std::lround(clampf(meter_, 0.f, 1.f) * 16.f));
            char bar[24];
            int p = 0;
            const char* pre = "POWER ";
            for (int i = 0; pre[i] && p < 22; i++) bar[p++] = pre[i];
            for (int i = 0; i < 16 && p < 23; i++) bar[p++] = (i == 8) ? '|' : (i < n ? '#' : '.');
            bar[p] = 0;
            hud(1, 26, bar, PAL_GOLD);
            hudC(27, "LET GO INTO A TAPE CUP", PAL_TEXT);
        } else {
            hudC(25, "BREAKS RIGHT", PAL_GOLD);
            hudC(26, need, PAL_GREEN);
            hudC(27, "ARROWS AIM   HOLD Z", PAL_TEXT);
        }
    }
}

}  // namespace putttape

#include "game/hooptape.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace hooptape {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kBotSpeed = 7.5f;
constexpr float kAimSpeed = 3.4f;
constexpr float kMeterRate = 0.75f;
constexpr float kTitleWait = 0.55f;
constexpr float kPocketWait = 0.42f;
constexpr float kJudgeWait = 0.50f;
constexpr float kLeaveWait = 1.05f;
constexpr int kFlightN = 32;
constexpr float kRimScrX = 272.f;
constexpr float kPx = 24.f;
constexpr float kFloorY = 164.f;
constexpr float kZpx = 9.f;
constexpr float kRimY = 3.05f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

float ease(float u) {
    u = clampf(u, 0.f, 1.f);
    return u * u * (3.f - 2.f * u);
}

float apexFor(Make m) {
    switch (m) {
    case Make::Arc: return 2.15f;
    case Make::Bank: return 1.85f;
    case Make::Swish: return 1.65f;
    case Make::Free: return 1.15f;
    case Make::Iron: return 1.50f;
    case Make::Rim: return 1.30f;
    }
    return 1.5f;
}

Game::Pt3 lerp3(Game::Pt3 a, Game::Pt3 b, float u) {
    return {a.x + (b.x - a.x) * u, a.y + (b.y - a.y) * u, a.z + (b.z - a.z) * u};
}

Game::Pt3 arcTo(Game::Pt3 a, Game::Pt3 b, float u, float apex) {
    Game::Pt3 p = lerp3(a, b, u);
    p.y += std::sin(clampf(u, 0.f, 1.f) * kPi) * apex;
    return p;
}

int markPal(const Mark& m, bool held) {
    if (m.line < 0) return PAL_MARKR;
    if (held) return PAL_MARKG;
    if (m.make == Make::Bank) return PAL_MARKB;
    if (m.make == Make::Free) return PAL_MARKG;
    return PAL_MARKW;
}

}  // namespace

int Game::drawerScore() const {
    int s = 0;
    for (int i = 0; i < kTapeN; i++)
        if (held_[i]) s += tapePay(i);
    return s;
}

const char* Game::tapeLabel(int i) const {
    if (i < 0 || i >= kTapeN) return "";
    return tapeName(i);
}

int Game::tapeScore(int i) const {
    if (i < 0 || i >= kTapeN) return 0;
    return tapePay(i);
}

const char* Game::modeName() const {
    switch (mode_) {
    case Mode::Title: return "TITLE";
    case Mode::Aim: return "AIM";
    case Mode::Flight: return "FLIGHT";
    case Mode::Pocket: return "POCKET";
    case Mode::Judge: return "JUDGE";
    case Mode::Leave: return "LEAVE";
    case Mode::Lose: return "LOSE";
    case Mode::Pause: return "PAUSE";
    case Mode::Over: return "OVER";
    }
    return "?";
}

int Game::phase() const {
    if (!rules_ || mode_ == Mode::Lose) return 4;
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Flight) return 2;
    if (mode_ == Mode::Leave || mode_ == Mode::Over || matched()) return 3;
    return 1;
}

bool Game::audit() const {
    bool ok = true;
    auto bad = [&](const char* msg) {
        std::fprintf(stderr, "s3hooptape %s\n", msg);
        ok = false;
    };
    const char* wantName[kTapeN] = {"SWISH", "BANK", "FREE"};
    const int wantPay[kTapeN] = {2, 3, 1};
    const char* wantTwin[kTapeN] = {"IRON", "ARC", "RIM"};
    const Make wantMake[kTapeN] = {Make::Swish, Make::Bank, Make::Free};
    const Make twinMake[kTapeN] = {Make::Iron, Make::Arc, Make::Rim};
    int seen[kTapeN] = {-1, -1, -1};
    int payCount[kTapeN] = {};
    if (markAt(kHomeX, kHomeZ) != -1) bad("home stands on a mark");
    for (int i = 0; i < kMarkN; i++) {
        const Mark& m = kMark[i];
        if (markAt(m.x, m.z) != i) bad("center missed its mark");
        if (markAt(m.x + kHit, m.z) != i || markAt(m.x - kHit, m.z) != i) bad("depth rim missed its mark");
        if (markAt(m.x, m.z + kHit) != i || markAt(m.x, m.z - kHit) != i) bad("line rim missed its mark");
        float in = kHit * 0.7f;
        if (markAt(m.x + in, m.z + in) != i) bad("inner rim missed its mark");
        if (markAt(m.x + kHit + 0.08f, m.z) == i || markAt(m.x - (kHit + 0.08f), m.z) == i) bad("outside depth still hit");
        if (markAt(m.x, m.z + kHit + 0.08f) == i || markAt(m.x, m.z - (kHit + 0.08f)) == i) bad("outside line still hit");
        if (m.x - kHit < kMinX || m.x + kHit > kMaxX || m.z - kHit < kMinZ || m.z + kHit > kMaxZ) bad("mark leaves the floor");
        if (m.line >= kTapeN) {
            bad("line out of range");
            continue;
        }
        if (m.line >= 0) {
            if (seen[m.line] >= 0) bad("tape line repeated");
            seen[m.line] = i;
        }
        int which = -1;
        for (int t = 0; t < kTapeN; t++)
            if (m.pay == wantPay[t]) which = t;
        if (which < 0) bad("pay is not on the tape");
        else payCount[which]++;
    }
    for (int i = 0; i < kMarkN; i++) {
        for (int j = i + 1; j < kMarkN; j++) {
            float d = std::hypot(kMark[i].x - kMark[j].x, kMark[i].z - kMark[j].z);
            if (d <= kHit * 2.f) bad("marks overlap");
            float mx = (kMark[i].x + kMark[j].x) * 0.5f;
            float mz = (kMark[i].z + kMark[j].z) * 0.5f;
            int mid = markAt(mx, mz);
            if (mid == i || mid == j) bad("gap counted as a mark");
        }
    }
    if (markAt(-20.f, 0.f) != -1 || markAt(2.f, 0.f) != -1) bad("off the court still hit");
    if (!firmMeter(0.5f) || !firmMeter(0.5f - kSweet) || !firmMeter(0.5f + kSweet)) bad("sweet window slipped");
    if (firmMeter(0.5f - kSweet - 0.02f) || firmMeter(0.5f + kSweet + 0.02f)) bad("sweet window grew");
    for (int t = 0; t < kTapeN; t++) {
        if (seen[t] < 0) bad("tape line missing");
        if (payCount[t] != 2) bad("a pay does not have one twin");
        int s = markOfLine(t);
        int twin = twinMark(t);
        if (s < 0 || twin < 0 || s == twin) {
            bad("tape and twin did not pair");
            continue;
        }
        if (std::strcmp(kMark[s].name, wantName[t]) != 0 || kMark[s].pay != wantPay[t]) bad("tape name drifted");
        if (kMark[s].make != wantMake[t]) bad("tape make drifted");
        if (std::strcmp(kMark[twin].name, wantTwin[t]) != 0 || kMark[twin].pay != wantPay[t]) bad("twin drifted");
        if (kMark[twin].line >= 0 || kMark[twin].make != twinMake[t]) bad("twin counted on the tape");
        if (takenLine(true, kMark[s].x, kMark[s].z) != t) bad("firm shot missed the tape");
        if (takenLine(false, kMark[s].x, kMark[s].z) != -1) bad("a soft shot filled the tape");
        if (takenLine(true, kMark[twin].x, kMark[twin].z) != -1 || !isTwin(true, kMark[twin].x, kMark[twin].z))
            bad("twin filled the tape");
        if (isTwin(false, kMark[twin].x, kMark[twin].z)) bad("a soft twin counted");
        if (std::strcmp(tapeName(t), wantName[t]) != 0 || tapePay(t) != wantPay[t]) bad("tape label drifted");
    }
    if (tapeSum() != 6) bad("tape does not sum to 6");
    return ok;
}

Game::Arc Game::plan(float x, float z, float meter, Make& make) const {
    make = Make::Swish;
    if (!firmMeter(meter)) return meter < 0.5f ? Arc::Short : Arc::Hot;
    int m = markAt(x, z);
    if (m < 0) return Arc::Brick;
    make = kMark[m].make;
    if (make == Make::Bank) return Arc::Bank;
    if (make == Make::Iron || make == Make::Rim) return Arc::Rattle;
    return Arc::Clean;
}

Game::Pt3 Game::handAt(float x, float z) const { return {x + 0.34f, 1.72f, z * 0.12f}; }

Game::Pt3 Game::flightAt(float u) const {
    u = clampf(u, 0.f, 1.f);
    Pt3 from = from_;
    if (arc_ == Arc::Short) {
        Pt3 b{from.x + (0.15f - from.x) * 0.62f, 0.08f, from.z * 0.4f};
        return arcTo(from, b, u, 1.05f);
    }
    if (arc_ == Arc::Hot) {
        Pt3 b{0.95f, 3.7f, from.z * 0.25f};
        return arcTo(from, b, u, 1.35f);
    }
    if (arc_ == Arc::Brick) {
        float side = std::fabs(from.z) > 0.25f ? from.z * 0.45f : 0.32f;
        Pt3 iron{-0.02f, kRimY, clampf(from.z, -0.35f, 0.35f) + side};
        if (u < 0.70f) return arcTo(from, iron, u / 0.70f, 1.45f);
        Pt3 back{iron.x - 0.75f, 1.35f, iron.z};
        return lerp3(iron, back, (u - 0.70f) / 0.30f);
    }
    if (arc_ == Arc::Bank) {
        Pt3 glass{0.42f, 3.42f, 0.38f};
        Pt3 drop{-0.02f, 2.12f, 0.02f};
        if (u < 0.60f) return arcTo(from, glass, u / 0.60f, 1.65f);
        return lerp3(glass, drop, (u - 0.60f) / 0.40f);
    }
    if (arc_ == Arc::Rattle) {
        Pt3 iron{0.14f, kRimY, 0.20f};
        Pt3 drop{0.0f, 2.18f, 0.02f};
        if (u < 0.68f) return arcTo(from, iron, u / 0.68f, 1.50f);
        Pt3 p = lerp3(iron, drop, (u - 0.68f) / 0.32f);
        p.x += std::sin((u - 0.68f) / 0.32f * kPi) * 0.05f;
        return p;
    }
    Pt3 rim{0.0f, kRimY, 0.0f};
    Pt3 drop{-0.02f, 2.02f, 0.0f};
    if (u < 0.76f) return arcTo(from, rim, u / 0.76f, apex_);
    return lerp3(rim, drop, (u - 0.76f) / 0.24f);
}

int Game::openLine() const {
    for (int i = 0; i < kTapeN; i++)
        if (!held_[i]) return i;
    return 0;
}

void Game::blip(int ch, float freq, float vol, float hold) {
    if (!sys_) return;
    sys_->apu.tone(ch, freq, vol);
    toneT_ = std::max(toneT_, hold);
}

void Game::chord(float a, float b, float c, float hold) {
    if (!sys_) return;
    sys_->apu.tone(0, a, 0.07f);
    sys_->apu.tone(1, b, 0.05f);
    sys_->apu.tone(2, c, 0.04f);
    toneT_ = std::max(toneT_, hold);
}

void Game::toTitle() {
    won_ = false;
    over_ = false;
    left_ = false;
    shots_ = 0;
    traps_ = 0;
    board_ = 0;
    shake_ = 0;
    flightT_ = 0;
    fly_ = -1;
    mark_ = -1;
    wasSweet_ = false;
    hitSnd_ = false;
    feetX_ = kHomeX;
    feetZ_ = kHomeZ;
    meter_ = 0.f;
    meterDir_ = 1.f;
    pocketT_ = judgeT_ = leaveT_ = 0.f;
    reason_[0] = 0;
    for (int i = 0; i < kTapeN; i++) held_[i] = false;
    mode_ = Mode::Title;
    if (sys_) sys_->apu.silence();
}

void Game::beginAim() {
    meterDir_ = meter_ < 0.5f ? 1.f : -1.f;
    wasSweet_ = false;
    hitSnd_ = false;
    fly_ = -1;
    pocketT_ = 0.f;
    judgeT_ = 0.f;
    reason_[0] = 0;
    mode_ = Mode::Aim;
}

void Game::newGame() {
    won_ = false;
    over_ = false;
    left_ = false;
    shots_ = 0;
    traps_ = 0;
    board_ = 0;
    shake_ = 0;
    flightT_ = 0;
    feetX_ = kHomeX;
    feetZ_ = kHomeZ;
    meter_ = 0.f;
    meterDir_ = 1.f;
    for (int i = 0; i < kTapeN; i++) held_[i] = false;
    beginAim();
    blip(0, 392.f, 0.05f, 0.08f);
}

void Game::launch() {
    if (mode_ != Mode::Aim || !rules_ || shots_ >= kMaxShots) return;
    shots_++;
    from_ = handAt(feetX_, feetZ_);
    arc_ = plan(feetX_, feetZ_, meter_, made_);
    apex_ = apexFor(made_);
    mark_ = markAt(feetX_, feetZ_);
    flightT_ = 0;
    hitSnd_ = false;
    mode_ = Mode::Flight;
    if (sys_) sys_->apu.noiseBurst(0.16f, 1700.f, 0.05f);
}

void Game::stick() {
    bool firm = arc_ == Arc::Clean || arc_ == Arc::Bank || arc_ == Arc::Rattle;
    int line = firm ? takenLine(true, feetX_, feetZ_) : -1;
    bool twin = firm && isTwin(true, feetX_, feetZ_);
    if (arc_ == Arc::Short) std::snprintf(reason_, sizeof reason_, "SHORT");
    else if (arc_ == Arc::Hot) std::snprintf(reason_, sizeof reason_, "HOT");
    else if (arc_ == Arc::Brick) std::snprintf(reason_, sizeof reason_, "NOT SET");
    else if (line >= 0) {
        if (held_[line]) {
            std::snprintf(reason_, sizeof reason_, "ALREADY IN");
            mode_ = Mode::Judge;
            judgeT_ = 0.f;
            shake_ = 4;
            blip(1, 160.f, 0.05f, 0.14f);
            return;
        }
        held_[line] = true;
        board_ += tapePay(line);
        fly_ = line;
        slipFromX_ = sx(0.f);
        slipFromY_ = sy(kRimY, 0.f);
        pocketT_ = 0.f;
        mode_ = Mode::Pocket;
        std::snprintf(reason_, sizeof reason_, "IN THE DRAWER");
        blip(1, 523.f + float(line) * 70.f, 0.06f, 0.18f);
        if (sys_) {
            sys_->rumble(0.2f, 0.45f, 80);
            sys_->setLight(80, 170, 70);
        }
        return;
    } else if (twin && mark_ >= 0) {
        traps_++;
        board_ += kMark[mark_].pay;
        std::snprintf(reason_, sizeof reason_, "%s STAYS OUT", kMark[mark_].name);
        mode_ = Mode::Judge;
        judgeT_ = 0.f;
        shake_ = 8;
        blip(1, 110.f, 0.07f, 0.2f);
        if (sys_) {
            sys_->apu.noiseBurst(0.2f, 280.f, 0.08f);
            sys_->rumble(0.35f, 0.1f, 70);
            sys_->setLight(170, 40, 30);
        }
        return;
    } else std::snprintf(reason_, sizeof reason_, "MISS");
    mode_ = Mode::Judge;
    judgeT_ = 0.f;
    shake_ = 5;
    blip(1, 90.f, 0.06f, 0.16f);
    if (sys_) sys_->rumble(0.2f, 0.05f, 40);
}

void Game::beginLeave() {
    if (!matched() || mode_ == Mode::Leave || mode_ == Mode::Over) return;
    mode_ = Mode::Leave;
    leaveT_ = 0.f;
    left_ = true;
    fly_ = -1;
    std::snprintf(reason_, sizeof reason_, "THE DRAWER MATCHES THE TAPE");
    chord(392.f, 523.f, 659.f, 0.7f);
    if (sys_) {
        sys_->rumble(0.25f, 0.55f, 140);
        sys_->setLight(255, 180, 60);
    }
}

void Game::beginLose(const char* why) {
    mode_ = Mode::Lose;
    won_ = false;
    over_ = true;
    left_ = false;
    if (why && why[0]) std::snprintf(reason_, sizeof reason_, "%s", why);
    else if (!reason_[0]) std::snprintf(reason_, sizeof reason_, "DOES NOT MATCH");
    blip(0, 82.f, 0.07f, 0.28f);
    if (sys_) sys_->setLight(150, 30, 28);
}

void Game::finishLeave() {
    won_ = rules_ && matched() && left_ && drawerScore() == tapeSum() && shots_ >= kTapeN && shots_ <= kMaxShots;
    over_ = true;
    mode_ = Mode::Over;
    if (!won_) std::snprintf(reason_, sizeof reason_, "DOES NOT MATCH");
}

void Game::continuePlay() {
    if (matched()) beginLeave();
    else if (shots_ >= kMaxShots) beginLose("DOES NOT MATCH");
    else beginAim();
}

void Game::botAim() {
    int line = openLine();
    int s = markOfLine(line);
    if (s < 0) return;
    float tx = kMark[s].x;
    float tz = kMark[s].z;
    float dx = tx - feetX_;
    float dz = tz - feetZ_;
    float dist = std::hypot(dx, dz);
    if (dist > 0.03f) {
        float step = std::min(kBotSpeed * kDt, dist);
        feetX_ += dx / dist * step;
        feetZ_ += dz / dist * step;
        feetX_ = clampf(feetX_, kMinX, kMaxX);
        feetZ_ = clampf(feetZ_, kMinZ, kMaxZ);
        return;
    }
    feetX_ = tx;
    feetZ_ = tz;
    if (sweet() && takenLine(true, feetX_, feetZ_) == line) launch();
}

void Game::humanAim(const gs::Pad& pad, bool fire) {
    float mx = 0.f, mz = 0.f;
    if (pad.down(gs::BTN_LEFT)) mx -= 1.f;
    if (pad.down(gs::BTN_RIGHT)) mx += 1.f;
    if (pad.down(gs::BTN_UP)) mz -= 1.f;
    if (pad.down(gs::BTN_DOWN)) mz += 1.f;
    if (std::fabs(pad.axisX) > 0.22f) mx = pad.axisX;
    if (std::fabs(pad.axisY) > 0.22f) mz = -pad.axisY;
    feetX_ = clampf(feetX_ + mx * kAimSpeed * kDt, kMinX, kMaxX);
    feetZ_ = clampf(feetZ_ + mz * kAimSpeed * kDt, kMinZ, kMaxZ);
    bool sw = sweet();
    int hot = markAt(feetX_, feetZ_);
    if (sw && !wasSweet_) {
        bool tape = hot >= 0 && kMark[hot].line >= 0 && !held_[kMark[hot].line];
        blip(2, tape ? 880.f : 220.f, 0.03f, 0.04f);
    }
    wasSweet_ = sw;
    if (fire) launch();
}

void Game::dribble() {
    float s = std::sin(clock_ * 7.6f);
    ballX_ = feetX_ + 0.40f;
    ballY_ = 0.20f + std::fabs(s) * 0.78f;
    ballZ_ = feetZ_ * 0.08f;
    if (s > 0.f && dribS_ <= 0.f && sys_) sys_->apu.noiseBurst(0.07f, 260.f, 0.03f);
    dribS_ = s;
}

void Game::holdBall() {
    Pt3 h = handAt(feetX_, feetZ_);
    ballX_ = h.x;
    ballY_ = h.y;
    ballZ_ = h.z;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    rules_ = audit();
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(2, 2, 3));
    sys.apu.setMaster(0.7f);
    sys.apu.setEcho(0.08f, 0.16f, 0.08f);
    toTitle();
    if (!rules_) {
        over_ = true;
        won_ = false;
        mode_ = Mode::Lose;
        std::snprintf(reason_, sizeof reason_, "RULES");
        std::fprintf(stderr, "s3hooptape rules failed\n");
    }
}

void Game::tickAudio(float dt) {
    if (!sys_) return;
    if (toneT_ > 0.f) {
        toneT_ -= dt;
        if (toneT_ <= 0.f) {
            sys_->apu.tone(0, 0, 0);
            sys_->apu.tone(1, 0, 0);
            sys_->apu.tone(2, 0, 0);
        }
    }
    if (mode_ == Mode::Flight) sys_->apu.noise(0.02f, 1400.f, false);
    else sys_->apu.noise(0.f, 1000.f, false);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (mode_ != Mode::Pause) clock_ += kDt;
    if (shake_ > 0) shake_--;
    if (mode_ == Mode::Title || mode_ == Mode::Aim) {
        meter_ += meterDir_ * kMeterRate * kDt;
        if (meter_ >= 1.f) {
            meter_ = 1.f;
            meterDir_ = -1.f;
        } else if (meter_ <= 0.f) {
            meter_ = 0.f;
            meterDir_ = 1.f;
        }
    }

    const gs::Pad& pad = sys.pad;
    bool start = pad.pressed(gs::BTN_START);
    bool back = pad.pressed(gs::BTN_MODE);
    bool fire = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_TURBO);
    if (bot_) start = back = fire = false;

    Mode before = mode_;
    if (mode_ == Mode::Pause) {
        if (start || fire) mode_ = heldMode_;
        else if (back) toTitle();
    } else if (mode_ == Mode::Title) {
        if (bot_ && clock_ > kTitleWait) newGame();
        else if (back) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        } else if (start || fire) newGame();
    } else if (mode_ == Mode::Over || mode_ == Mode::Lose) {
        if (!bot_ && (start || fire)) newGame();
        else if (!bot_ && back) toTitle();
    } else if (!bot_ && (start || back)) {
        heldMode_ = mode_;
        mode_ = Mode::Pause;
    } else if (mode_ == Mode::Aim) {
        if (bot_) botAim();
        else humanAim(pad, fire);
    }

    if (mode_ == Mode::Flight && before == Mode::Flight) {
        flightT_++;
        float u = float(flightT_) / float(kFlightN);
        if (!hitSnd_ && u > 0.62f && (arc_ == Arc::Bank || arc_ == Arc::Rattle || arc_ == Arc::Brick)) {
            hitSnd_ = true;
            if (sys_) sys_->apu.noiseBurst(arc_ == Arc::Bank ? 0.22f : 0.28f, arc_ == Arc::Bank ? 520.f : 1900.f, 0.06f);
        }
        if (!hitSnd_ && u > 0.74f && arc_ == Arc::Clean) {
            hitSnd_ = true;
            if (sys_) sys_->apu.noiseBurst(0.08f, 2400.f, 0.04f);
        }
        if (flightT_ >= kFlightN) stick();
    } else if (mode_ == Mode::Pocket && before == Mode::Pocket) {
        pocketT_ += kDt;
        if (pocketT_ > kPocketWait) continuePlay();
    } else if (mode_ == Mode::Judge && before == Mode::Judge) {
        judgeT_ += kDt;
        if (judgeT_ > kJudgeWait) continuePlay();
    } else if (mode_ == Mode::Leave && before == Mode::Leave) {
        leaveT_ += kDt;
        feetX_ -= 2.5f * kDt;
        if (leaveT_ > kLeaveWait) finishLeave();
    }

    if (mode_ == Mode::Title) dribble();
    else if (mode_ == Mode::Aim) holdBall();
    tickAudio(kDt);
    draw();
}

float Game::sx(float x) const { return kRimScrX + x * kPx; }
float Game::sy(float y, float z) const { return kFloorY - y * kPx + z * kZpx; }

void Game::spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool flip, bool shadow) {
    if (!sys_ || w < 1.f || h < 1.f || img.w == 0 || img.h == 0) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = img;
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::sprM(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (m.h <= 0 || h < 1.f) return;
    float ih = std::max(4.f, h);
    float iw = ih * float(m.w) / float(m.h);
    spr(m.pick(ih), cx, cy, iw, ih, pal, flip, false);
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

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        uint16_t c;
        if (y < 12) c = gs::rgb4(1, 1, 2);
        else if (y < 46) c = gs::rgb4(3, 4, 6);
        else if (y < 92) {
            int band = (y / 6) & 1;
            c = gs::rgb4(4, 5 + band, 8);
        } else if (y < 128) c = gs::rgb4(3, 4, 6);
        else if (y < 178) {
            int band = (y / 3) & 1;
            c = gs::rgb4(9 - band, 5, 2);
        } else if (y < 192) c = gs::rgb4(6, 4, 2);
        else {
            int lip = y < 198 ? 1 : 0;
            c = gs::rgb4(4 + lip, 2, 1);
        }
        v.lineBackdrop[y] = c;
    }
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    backdrop();

    const Mode m = view();
    const bool title = m == Mode::Title;
    const bool aiming = m == Mode::Aim || title;
    const bool leaving = m == Mode::Leave || (m == Mode::Over && won_);
    const bool failed = m == Mode::Lose || (m == Mode::Over && !won_);
    const float jx = shake_ > 0 ? std::sin(float(shake_) * 1.7f) * 2.f : 0.f;
    const int hot = aiming ? markAt(feetX_, feetZ_) : -1;

    if (title) spr(art_.logo, 168.f, 18.f, float(art_.logo.w), float(art_.logo.h), PAL_GOLD);
    if (leaving) spr(art_.matchW, 168.f, 20.f, float(art_.matchW.w), float(art_.matchW.h), PAL_GOOD);
    if (failed) spr(art_.openW, 168.f, 20.f, float(art_.openW.w), float(art_.openW.h), PAL_BAD);

    if (aiming && m != Mode::Title) {
        for (int i = 0; i < 11; i++) {
            float pos = float(i) / 10.f;
            float x = 118.f + pos * 100.f;
            bool mid = i == 5;
            bool on = std::fabs(meter_ - pos) < 0.06f;
            int pal = mid ? PAL_GOLD : PAL_INK;
            if (on && sweet()) pal = PAL_GOOD;
            else if (on) pal = PAL_BAD;
            spr(art_.blot, x, 30.f, mid ? 6.f : 4.f, mid ? 10.f : 7.f, pal);
        }
    }

    Pt3 ball{ballX_, ballY_, ballZ_};
    bool showBall = aiming || m == Mode::Flight || m == Mode::Judge || m == Mode::Pocket || leaving;
    if (m == Mode::Flight) ball = flightAt(float(flightT_) / float(kFlightN));
    else if (m == Mode::Judge && (arc_ == Arc::Short || arc_ == Arc::Hot || arc_ == Arc::Brick)) ball = flightAt(1.f);
    else if (leaving || m == Mode::Pocket) ball = flightAt(1.f);

    if (aiming && m == Mode::Aim) {
        Make previewMake = Make::Swish;
        Arc preview = plan(feetX_, feetZ_, meter_, previewMake);
        Arc saved = arc_;
        float savedApex = apex_;
        Pt3 savedFrom = from_;
        arc_ = preview;
        apex_ = apexFor(previewMake);
        from_ = handAt(feetX_, feetZ_);
        int pal = sweet() && hot >= 0 && kMark[hot].line >= 0 ? PAL_GOOD : PAL_GOLD;
        for (int i = 1; i <= 10; i++) {
            Pt3 p = flightAt(float(i) / 11.f);
            if (p.y < 0.15f) break;
            spr(art_.blot, sx(p.x), sy(p.y, p.z), 3.f, 3.f, pal);
        }
        arc_ = saved;
        apex_ = savedApex;
        from_ = savedFrom;
    }

    int spin = (int(clock_ * 8.f) + flightT_) & 1;
    if (showBall) sprM(art_.ball[spin], sx(ball.x) + jx, sy(ball.y, ball.z), 16.f, PAL_BALL);

    bool flip = leaving;
    float ph = 52.f;
    float py = sy(0.f, feetZ_) - ph * 0.5f;
    if (m == Mode::Leave) py += std::sin(leaveT_ * 12.f) * 1.1f;
    sprM(art_.player, sx(feetX_), py, ph, PAL_YOU, flip);

    float rimX = sx(0.f);
    float rimY = sy(kRimY, 0.f);
    int netFrame = (m == Mode::Flight && flightT_ > 20) ? 1 : (int(clock_ * 2.f) & 1);
    spr(art_.net[netFrame], rimX - 2.f, rimY + 16.f, 30.f, 28.f, PAL_NET);
    spr(art_.rim, rimX - 2.f, rimY, 42.f, 13.f, PAL_IRON);
    spr(art_.arm, rimX + 16.f, rimY + 1.f, 24.f, 5.f, PAL_IRON);

    float boardX = sx(0.22f);
    float boardY = rimY - 8.f;
    if (aiming || m == Mode::Flight) {
        float px = boardX + feetZ_ * 12.f;
        px = clampf(px, boardX - 16.f, boardX + 18.f);
        int pal = PAL_GOLD;
        if (hot >= 0 && kMark[hot].make == Make::Bank) pal = PAL_MARKB;
        else if (hot >= 0 && kMark[hot].line >= 0) pal = PAL_GOOD;
        else if (hot >= 0) pal = PAL_BAD;
        spr(art_.blot, px, boardY + 2.f, 5.f, 5.f, pal);
    }
    spr(art_.board, boardX, boardY, 50.f, 36.f, PAL_BOARD);
    float poleTop = rimY + 10.f;
    float poleH = std::max(8.f, kFloorY - poleTop);
    spr(art_.pole, sx(0.28f), (poleTop + kFloorY) * 0.5f, 8.f, poleH, PAL_IRON);
    spr(art_.base, sx(0.28f), kFloorY - 2.f, 18.f, 8.f, PAL_IRON);

    spr(art_.window, 130.f, 100.f, 30.f, 18.f, PAL_WIN);
    spr(art_.window, 196.f, 96.f, 30.f, 18.f, PAL_WIN);

    float keyL = sx(-4.05f);
    float keyR = sx(0.05f);
    spr(art_.key, (keyL + keyR) * 0.5f, kFloorY + 2.f, keyR - keyL, 12.f, PAL_MARKB);
    spr(art_.blot, sx(-4.00f), kFloorY + 2.f, 3.f, 16.f, PAL_MARKW);
    spr(art_.blot, sx(-6.75f), kFloorY + 1.f, 3.f, 12.f, PAL_MARKW);
    spr(art_.blot, sx(0.05f), kFloorY, 3.f, 18.f, PAL_MARKW);
    spr(art_.blot, 160.f, kFloorY + 6.f, 300.f, 2.f, PAL_WOOD);

    for (int i = 0; i < kMarkN; i++) {
        const Mark& mk = kMark[i];
        float pulse = (hot == i) ? 15.f + std::sin(clock_ * 8.f) * 1.4f : 12.f;
        int pal = markPal(mk, mk.line >= 0 && held_[mk.line]);
        const gs::Image& img = mk.make == Make::Bank ? art_.box : art_.pip;
        spr(img, sx(mk.x), sy(0.f, mk.z) + 2.f, pulse, pulse, pal);
    }

    spr(art_.shadow, sx(ball.x), sy(0.f, ball.z) + 4.f, 14.f, 5.f, PAL_WOOD, false, true);
    spr(art_.shadow, sx(feetX_), sy(0.f, feetZ_) + 5.f, 22.f, 6.f, PAL_WOOD, false, true);

    spr(art_.tape, 50.f, 200.f, float(art_.tape.w), float(art_.tape.h), PAL_PAPER);
    spr(art_.drawer, 210.f, 208.f, float(art_.drawer.w), float(art_.drawer.h), PAL_WOOD);
    for (int i = 0; i < kTapeN; i++) spr(art_.slot, kSlotX[i], kSlotY, 42.f, 14.f, PAL_WOOD);

    for (int i = 0; i < kTapeN; i++) {
        if (!held_[i]) continue;
        float x = kSlotX[i];
        float y = kSlotY;
        if (m == Mode::Pocket && fly_ == i) {
            float e = ease(pocketT_ / kPocketWait);
            x = slipFromX_ + (kSlotX[i] - slipFromX_) * e;
            y = slipFromY_ + (kSlotY - slipFromY_) * e;
        }
        const gs::Image& slip = art_.slip[i];
        spr(slip, x + jx, y, float(slip.w), float(slip.h), PAL_PAPER);
    }

    char buf[48];
    int showShot = std::min(shots_ + (m == Mode::Aim || title ? 1 : 0), kMaxShots);
    std::snprintf(buf, sizeof buf, "SHOT %d", showShot);
    hud(1, 0, buf, PAL_GOLD);
    std::snprintf(buf, sizeof buf, "BOARD %d", board_);
    hud(31, 0, buf, board_ == drawerScore() ? PAL_INK : PAL_BAD);
    hud(12, 22, "SWISH 2", held_[0] ? PAL_GOOD : PAL_GOLD);
    hud(20, 22, "BANK  3", held_[1] ? PAL_GOOD : PAL_GOLD);
    hud(28, 22, "FREE  1", held_[2] ? PAL_GOOD : PAL_GOLD);

    if (title) {
        hudC(4, "A SHORT HOOP", PAL_INK);
        hudC(5, "MATCH THE TAPE", PAL_GOLD);
        hudC(7, "TWINS PAY AND STAY OUT", PAL_BAD);
        hudC(9, "ARROWS MOVE   Z SHOOTS", PAL_INK);
    } else if (mode_ == Mode::Pause) {
        hudC(4, "PAUSED", PAL_INK);
        hudC(6, "ENTER RESUMES", PAL_GOLD);
    } else if (m == Mode::Aim) {
        if (hot >= 0 && kMark[hot].line >= 0) {
            std::snprintf(buf, sizeof buf, held_[kMark[hot].line] ? "%s IN" : "%s  %d", kMark[hot].name, kMark[hot].pay);
            hudC(2, buf, sweet() && !held_[kMark[hot].line] ? PAL_GOOD : PAL_GOLD);
        } else if (hot >= 0) {
            std::snprintf(buf, sizeof buf, "%s STAYS OUT", kMark[hot].name);
            hudC(2, buf, PAL_BAD);
        } else hudC(2, sweet() ? "NOT SET" : "FIND A MARK", PAL_INK);
        if (!sweet()) hudC(3, meter_ < 0.5f ? "SHORT" : "HOT", PAL_BAD);
        hud(1, 26, "L-R DEPTH  U-D LINE  Z SHOOT", PAL_INK);
    } else if (m == Mode::Judge || m == Mode::Pocket) {
        int pal = (arc_ == Arc::Clean || arc_ == Arc::Bank || arc_ == Arc::Rattle) && fly_ >= 0 ? PAL_GOOD : PAL_BAD;
        hudC(2, reason_, pal);
    } else if (leaving) {
        hudC(3, "THE DRAWER MATCHES", PAL_GOOD);
        if (m == Mode::Over && !bot_) hudC(5, "ENTER PLAYS AGAIN", PAL_INK);
    } else if (failed) {
        hudC(3, "STILL OPEN", PAL_BAD);
        hudC(5, reason_, PAL_INK);
        if (!bot_) hudC(7, "ENTER PLAYS AGAIN", PAL_INK);
    }
}

}  // namespace hooptape

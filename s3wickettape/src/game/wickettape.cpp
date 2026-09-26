#include "game/wickettape.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace wickettape {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kMeterRate = 0.70f;
constexpr float kTitleWait = 0.55f;
constexpr float kPocketWait = 0.42f;
constexpr float kJudgeWait = 0.48f;
constexpr float kLeaveWait = 1.10f;
constexpr int kFlightN = 32;
constexpr float kContact = 0.30f;
constexpr float kBatX = 78.f;
constexpr float kFeetY = 162.f;
constexpr float kBowlHome = 208.f;
constexpr float kNearX = 54.f;
constexpr float kFarX = 184.f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

float ease(float u) {
    u = clampf(u, 0.f, 1.f);
    return u * u * (3.f - 2.f * u);
}

int poseFor(Stroke s) {
    switch (s) {
    case Stroke::Glance:
    case Stroke::Nudge: return 1;
    case Stroke::Drive:
    case Stroke::Push: return 2;
    case Stroke::Six:
    case Stroke::Loft: return 3;
    }
    return 0;
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
    if (mode_ == Mode::Leave || mode_ == Mode::Over) return 3;
    if (mode_ == Mode::Pause && heldMode_ == Mode::Leave) return 3;
    return 1;
}

bool Game::audit() {
    bool ok = true;
    const char* why = "RULES";
    auto bad = [&](const char* msg) {
        if (ok) why = msg;
        std::fprintf(stderr, "s3wickettape %s\n", msg);
        ok = false;
    };
    const char* wantName[kTapeN] = {"GLANCE", "DRIVE", "SIX"};
    const int wantPay[kTapeN] = {1, 3, 6};
    const char* wantTwin[kTapeN] = {"NUDGE", "PUSH", "LOFT"};
    const Stroke wantStroke[kTapeN] = {Stroke::Glance, Stroke::Drive, Stroke::Six};
    const Stroke twinStroke[kTapeN] = {Stroke::Nudge, Stroke::Push, Stroke::Loft};
    if (kMaxBalls != 6 || kTapeN != 3 || kGapN != 6) bad("the over is the wrong length");
    int seen[kTapeN] = {-1, -1, -1};
    int payCount[kTapeN] = {};
    int strokeCount[kGapN] = {};
    for (int i = 0; i < kGapN; i++) {
        const Gap& g = kGap[i];
        int st = int(g.stroke);
        if (st < 0 || st >= kGapN) bad("stroke out of range");
        else strokeCount[st]++;
        if (g.line >= kTapeN) {
            bad("line out of range");
            continue;
        }
        if (g.line >= 0) {
            if (seen[g.line] >= 0) bad("tape line repeated");
            seen[g.line] = i;
        }
        int which = -1;
        for (int t = 0; t < kTapeN; t++)
            if (g.pay == wantPay[t]) which = t;
        if (which < 0) bad("pay is not on the tape");
        else payCount[which]++;
        if (g.fx < 16.f || g.fx > 300.f || g.fy < 96.f || g.fy > 168.f) bad("fielder leaves the ground");
        if (g.apex <= 0.f) bad("a stroke has no flight");
    }
    for (int i = 0; i < kGapN; i++)
        if (strokeCount[i] != 1) bad("a stroke is missing");
    for (int i = 0; i < kGapN; i++) {
        for (int j = i + 1; j < kGapN; j++) {
            float d = std::hypot(kGap[i].fx - kGap[j].fx, kGap[i].fy - kGap[j].fy);
            if (d < 36.f) bad("fielders stand on each other");
            if (std::strcmp(kGap[i].name, kGap[j].name) == 0) bad("two strokes share a name");
        }
    }
    if (!sweetMeter(0.5f) || !sweetMeter(0.5f - kSweet) || !sweetMeter(0.5f + kSweet)) bad("sweet window slipped");
    if (sweetMeter(0.5f - kSweet - 0.03f) || sweetMeter(0.5f + kSweet + 0.03f)) bad("sweet window grew");
    int trap = 0;
    for (int t = 0; t < kTapeN; t++) {
        if (seen[t] < 0) bad("tape line missing");
        if (payCount[t] != 2) bad("a pay does not have one twin");
        int s = gapOfLine(t);
        int twin = twinOf(t);
        if (s < 0 || twin < 0 || s == twin) {
            bad("tape and twin did not pair");
            continue;
        }
        if (std::strcmp(kGap[s].name, wantName[t]) != 0 || kGap[s].pay != wantPay[t]) bad("tape name drifted");
        if (kGap[s].stroke != wantStroke[t]) bad("tape stroke drifted");
        if (std::strcmp(kGap[twin].name, wantTwin[t]) != 0 || kGap[twin].pay != wantPay[t]) bad("twin drifted");
        if (kGap[twin].line >= 0 || kGap[twin].stroke != twinStroke[t]) bad("twin counted on the tape");
        if (takenLine(true, s) != t) bad("firm stroke missed the tape");
        if (takenLine(false, s) != -1) bad("a mistimed stroke filled the tape");
        if (takenLine(true, twin) != -1 || !isTwin(true, twin)) bad("twin filled the tape");
        if (isTwin(false, twin)) bad("a mistimed twin counted");
        if (std::strcmp(tapeName(t), wantName[t]) != 0 || tapePay(t) != wantPay[t]) bad("tape label drifted");
        trap += kGap[twin].pay;
    }
    if (tapeSum() != 10 || trap != 10) bad("a close till is not still the same total");
    int six = gapOfLine(2);
    int loft = twinOf(2);
    int glance = gapOfLine(0);
    if (six < 0 || loft < 0 || glance < 0) bad("the straight shots are missing");
    else {
        if (kGap[six].apex <= kGap[loft].apex) bad("the six does not climb over the loft");
        if (kGap[loft].apex <= kGap[glance].apex) bad("the loft does not climb");
        if (kGap[six].tx < 290.f || kGap[six].ty > 70.f) bad("the six does not clear the rope");
        if (kGap[loft].tx > 240.f || kGap[loft].ty > 70.f) bad("the loft is not a high catch");
        if (kGap[glance].ty < 120.f) bad("the glance leaves the ground");
    }
    std::snprintf(reason_, sizeof reason_, "%s", ok ? "OPEN" : why);
    return ok;
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
    sys_->apu.tone(0, a, 0.06f);
    sys_->apu.tone(1, b, 0.045f);
    sys_->apu.tone(2, c, 0.035f);
    toneT_ = std::max(toneT_, hold);
}

void Game::toTitle() {
    won_ = false;
    over_ = false;
    left_ = false;
    balls_ = 0;
    traps_ = 0;
    board_ = 0;
    shake_ = 0;
    flightT_ = 0;
    fly_ = -1;
    shot_ = gapOfLine(0);
    pick_ = shot_ < 0 ? 0 : shot_;
    sweetWas_ = false;
    meter_ = 0.f;
    meterDir_ = 1.f;
    pocketT_ = judgeT_ = leaveT_ = 0.f;
    feetX_ = kBatX;
    releaseX_ = kBowlHome;
    std::snprintf(reason_, sizeof reason_, "OPEN");
    for (int i = 0; i < kTapeN; i++) held_[i] = false;
    mode_ = Mode::Title;
    if (sys_) sys_->apu.silence();
}

void Game::beginAim() {
    meterDir_ = meter_ < 0.5f ? 1.f : -1.f;
    sweetWas_ = sweetMeter(meter_);
    fly_ = -1;
    pocketT_ = 0.f;
    judgeT_ = 0.f;
    std::snprintf(reason_, sizeof reason_, "OPEN");
    mode_ = Mode::Aim;
}

void Game::newGame() {
    won_ = false;
    over_ = false;
    left_ = false;
    balls_ = 0;
    traps_ = 0;
    board_ = 0;
    shake_ = 0;
    flightT_ = 0;
    meter_ = 0.f;
    meterDir_ = 1.f;
    feetX_ = kBatX;
    releaseX_ = kBowlHome;
    pick_ = gapOfLine(0);
    if (pick_ < 0) pick_ = 0;
    for (int i = 0; i < kTapeN; i++) held_[i] = false;
    beginAim();
    blip(0, 392.f, 0.05f, 0.08f);
}

void Game::play() {
    if (mode_ != Mode::Aim || !rules_ || balls_ >= kMaxBalls) return;
    if (pick_ < 0 || pick_ >= kGapN) return;
    balls_++;
    shot_ = pick_;
    bool firm = sweetMeter(meter_);
    call_ = firm ? Call::Play : (meter_ < 0.5f ? Call::Soon : Call::Late);
    releaseX_ = kBowlHome;
    handX_ = releaseX_ - 8.f;
    handY_ = kFeetY - 44.f;
    batX_ = kBatX + 14.f;
    batY_ = kFeetY - 34.f;
    flightT_ = 0;
    mode_ = Mode::Flight;
    if (sys_) sys_->apu.noiseBurst(0.12f, 980.f, 0.04f);
    blip(0, 280.f, 0.04f, 0.05f);
}

void Game::settle() {
    bool firm = call_ == Call::Play;
    int line = takenLine(firm, shot_);
    bool twin = isTwin(firm, shot_);
    if (call_ == Call::Soon) std::snprintf(reason_, sizeof reason_, "LEADING EDGE");
    else if (call_ == Call::Late) std::snprintf(reason_, sizeof reason_, "TOO LATE");
    else if (line >= 0) {
        if (held_[line]) {
            std::snprintf(reason_, sizeof reason_, "ALREADY IN");
            mode_ = Mode::Judge;
            judgeT_ = 0.f;
            shake_ = 4;
            blip(1, 160.f, 0.05f, 0.12f);
            return;
        }
        held_[line] = true;
        board_ += tapePay(line);
        fly_ = line;
        slipFromX_ = kGap[shot_].tx;
        slipFromY_ = kGap[shot_].ty;
        pocketT_ = 0.f;
        mode_ = Mode::Pocket;
        std::snprintf(reason_, sizeof reason_, "IN THE DRAWER");
        if (line == 2) chord(392.f, 523.f, 659.f, 0.28f);
        else blip(1, 494.f + float(line) * 60.f, 0.06f, 0.16f);
        if (sys_) {
            sys_->apu.noiseBurst(0.16f, line == 2 ? 420.f : 1400.f, 0.05f);
            sys_->rumble(0.2f, line == 2 ? 0.55f : 0.35f, 90);
            sys_->setLight(80, 170, 70);
        }
        return;
    } else if (twin) {
        traps_++;
        board_ += kGap[shot_].pay;
        std::snprintf(reason_, sizeof reason_, "%s STAYS OUT", kGap[shot_].name);
        mode_ = Mode::Judge;
        judgeT_ = 0.f;
        shake_ = 8;
        blip(1, 110.f, 0.07f, 0.18f);
        if (sys_) {
            sys_->apu.noiseBurst(0.18f, 240.f, 0.07f);
            sys_->rumble(0.3f, 0.08f, 70);
            sys_->setLight(170, 40, 30);
        }
        return;
    } else std::snprintf(reason_, sizeof reason_, "MISSED");
    mode_ = Mode::Judge;
    judgeT_ = 0.f;
    shake_ = 5;
    blip(1, 90.f, 0.06f, 0.14f);
    if (sys_) sys_->rumble(0.15f, 0.04f, 40);
}

void Game::beginLeave() {
    if (!matched() || mode_ == Mode::Leave || mode_ == Mode::Over) return;
    mode_ = Mode::Leave;
    leaveT_ = 0.f;
    left_ = true;
    feetX_ = kBatX;
    fly_ = -1;
    std::snprintf(reason_, sizeof reason_, "THE DRAWER MATCHES THE TAPE");
    chord(392.f, 523.f, 659.f, 0.7f);
    if (sys_) {
        sys_->rumble(0.25f, 0.5f, 140);
        sys_->setLight(255, 190, 70);
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
    won_ = rules_ && matched() && left_ && drawerScore() == tapeSum() && balls_ >= kTapeN && balls_ <= kMaxBalls;
    over_ = true;
    mode_ = Mode::Over;
    if (!won_) std::snprintf(reason_, sizeof reason_, "DOES NOT MATCH");
}

void Game::continuePlay() {
    if (matched()) beginLeave();
    else if (balls_ >= kMaxBalls) beginLose("DOES NOT MATCH");
    else beginAim();
}

void Game::nudge(int dir) {
    pick_ = (pick_ + dir + kGapN) % kGapN;
    if (!bot_) blip(2, 480.f + float(pick_) * 28.f, 0.03f, 0.04f);
}

void Game::snapTape() {
    if (pick_ < 0 || pick_ >= kGapN) return;
    int line = kGap[pick_].line;
    int next = -1;
    if (line < 0) {
        for (int t = 0; t < kTapeN; t++)
            if (tapePay(t) == kGap[pick_].pay) next = gapOfLine(t);
    } else next = gapOfLine((line + 1) % kTapeN);
    if (next >= 0) pick_ = next;
    if (!bot_) blip(2, 620.f, 0.03f, 0.04f);
}

void Game::snapTwin() {
    if (pick_ < 0 || pick_ >= kGapN) return;
    int line = kGap[pick_].line;
    int next = -1;
    if (line >= 0) next = twinOf(line);
    else {
        int idx = 0;
        for (int t = 0; t < kTapeN; t++)
            if (tapePay(t) == kGap[pick_].pay) idx = t;
        next = twinOf((idx + 1) % kTapeN);
    }
    if (next >= 0) pick_ = next;
    if (!bot_) blip(2, 220.f, 0.03f, 0.04f);
}

void Game::botAim() {
    int want = gapOfLine(openLine());
    if (want < 0) return;
    if (pick_ != want) {
        int cw = (want - pick_ + kGapN) % kGapN;
        int ccw = (pick_ - want + kGapN) % kGapN;
        pick_ = cw <= ccw ? (pick_ + 1) % kGapN : (pick_ + kGapN - 1) % kGapN;
        return;
    }
    if (sweetMeter(meter_)) play();
}

void Game::humanAim(const gs::Pad& pad, bool fire) {
    if (pad.pressed(gs::BTN_LEFT)) nudge(-1);
    if (pad.pressed(gs::BTN_RIGHT)) nudge(1);
    if (pad.pressed(gs::BTN_UP)) snapTape();
    if (pad.pressed(gs::BTN_DOWN)) snapTwin();
    float ax = pad.axisX;
    float ay = pad.axisY;
    if (ax > 0.5f && axLatch_ <= 0.5f) nudge(1);
    if (ax < -0.5f && axLatch_ >= -0.5f) nudge(-1);
    if (ay > 0.5f && ayLatch_ <= 0.5f) snapTape();
    if (ay < -0.5f && ayLatch_ >= -0.5f) snapTwin();
    bool sw = sweetMeter(meter_);
    int hotLine = (pick_ >= 0 && pick_ < kGapN) ? kGap[pick_].line : -1;
    if (sw && !sweetWas_) {
        bool tape = hotLine >= 0 && !held_[hotLine];
        blip(2, tape ? 880.f : 200.f, 0.03f, 0.04f);
    }
    sweetWas_ = sw;
    if (fire) play();
}

float Game::bowlerX() const {
    if (mode_ == Mode::Flight || mode_ == Mode::Pocket || mode_ == Mode::Judge || mode_ == Mode::Leave ||
        mode_ == Mode::Over)
        return releaseX_;
    return kBowlHome + std::sin(clock_ * 8.f) * 4.f;
}

int Game::bowlPose() const {
    if ((mode_ == Mode::Flight || mode_ == Mode::Pocket) && flightT_ < 14) return 2;
    return int(clock_ * 8.f) & 1;
}

int Game::batPose() const {
    if (mode_ == Mode::Leave || (mode_ == Mode::Over && won_)) return 4;
    int gap = pick_;
    if (mode_ == Mode::Flight || mode_ == Mode::Pocket || mode_ == Mode::Judge) gap = shot_;
    if (gap >= 0 && gap < kGapN) return poseFor(kGap[gap].stroke);
    return 0;
}

void Game::flightPoint(float u, float& x, float& y, float& gx, float& gy) const {
    u = clampf(u, 0.f, 1.f);
    float tx, ty, apex, groundY;
    if (call_ == Call::Soon) {
        tx = 36.f;
        ty = 134.f;
        apex = 22.f;
        groundY = 152.f;
    } else if (call_ == Call::Late) {
        tx = kBowlHome - 10.f;
        ty = 152.f;
        apex = 4.f;
        groundY = 156.f;
    } else {
        const Gap& g = kGap[shot_ >= 0 && shot_ < kGapN ? shot_ : 0];
        tx = g.tx;
        ty = g.ty;
        apex = g.apex;
        groundY = g.fy + 6.f;
    }
    if (u < kContact) {
        float w = u / kContact;
        x = handX_ + (batX_ - handX_) * w;
        y = handY_ + (batY_ - handY_) * w - std::sin(w * kPi) * 5.f;
        gx = x;
        gy = y + 8.f;
        return;
    }
    float w = (u - kContact) / (1.f - kContact);
    x = batX_ + (tx - batX_) * w;
    gx = x;
    gy = batY_ + 8.f + (groundY - (batY_ + 8.f)) * w;
    y = batY_ + (ty - batY_) * w - std::sin(w * kPi) * apex;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    rules_ = audit();
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(6, 10, 6));
    sys.apu.setMaster(0.7f);
    sys.apu.setEcho(0.09f, 0.18f, 0.07f);
    if (!rules_) {
        over_ = true;
        won_ = false;
        left_ = false;
        mode_ = Mode::Lose;
        if (!reason_[0]) std::snprintf(reason_, sizeof reason_, "RULES");
        std::fprintf(stderr, "s3wickettape rules failed\n");
        return;
    }
    toTitle();
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
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    float dt = mode_ == Mode::Pause ? 0.f : kDt;
    if (dt > 0.f) clock_ += dt;
    if (shake_ > 0) shake_--;
    if (mode_ == Mode::Title || mode_ == Mode::Aim) {
        meter_ += meterDir_ * kMeterRate * dt;
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

    axLatch_ = pad.axisX;
    ayLatch_ = pad.axisY;

    if (mode_ == Mode::Flight && before == Mode::Flight) {
        flightT_++;
        if (flightT_ >= kFlightN) settle();
    } else if (mode_ == Mode::Pocket && before == Mode::Pocket) {
        pocketT_ += dt;
        if (pocketT_ > kPocketWait) continuePlay();
    } else if (mode_ == Mode::Judge && before == Mode::Judge) {
        judgeT_ += dt;
        if (judgeT_ > kJudgeWait) continuePlay();
    } else if (mode_ == Mode::Leave && before == Mode::Leave) {
        leaveT_ += dt;
        feetX_ -= 78.f * dt;
        if (leaveT_ > kLeaveWait) finishLeave();
    }

    tickAudio(dt > 0.f ? dt : kDt);
    draw();
}

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
        if (y < 18) c = gs::rgb4(6, 10, 14);
        else if (y < 48) c = gs::rgb4(8, 12, 15);
        else if (y < 78) c = gs::rgb4(11, 13, 12);
        else if (y < 176) {
            int mow = (y / 5) & 1;
            c = mow ? gs::rgb4(3, 10, 3) : gs::rgb4(2, 8, 2);
        } else if (y < 192) c = gs::rgb4(7, 4, 2);
        else c = gs::rgb4(4, 2, 1);
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

    const Mode m = mode_ == Mode::Pause ? heldMode_ : mode_;
    const bool title = m == Mode::Title;
    const bool aiming = m == Mode::Aim || title;
    const bool leaving = m == Mode::Leave || (m == Mode::Over && won_);
    const bool failed = m == Mode::Lose || (m == Mode::Over && !won_);
    const float jx = shake_ > 0 ? std::sin(float(shake_) * 1.7f) * 2.f : 0.f;
    const int hot = (pick_ >= 0 && pick_ < kGapN) ? pick_ : 0;

    if (title) spr(art_.logo, 160.f, 18.f, float(art_.logo.w), float(art_.logo.h), PAL_GOLD);
    if (leaving) spr(art_.matchW, 160.f, 20.f, float(art_.matchW.w), float(art_.matchW.h), PAL_GOOD);
    if (failed) spr(art_.openW, 160.f, 22.f, float(art_.openW.w), float(art_.openW.h), PAL_BAD);

    if (m == Mode::Aim) {
        for (int i = 0; i < 11; i++) {
            float pos = float(i) / 10.f;
            float x = 108.f + pos * 104.f;
            bool mid = i >= 4 && i <= 6;
            bool on = std::fabs(meter_ - pos) < 0.06f;
            int pal = mid ? PAL_GOLD : PAL_INK;
            if (on && sweetMeter(meter_)) pal = PAL_GOOD;
            else if (on) pal = PAL_BAD;
            spr(art_.blot, x, 34.f, mid ? 7.f : 4.f, mid ? 11.f : 7.f, pal);
        }
    }

    for (int i = 0; i < kTapeN; i++) {
        if (!held_[i]) continue;
        float x = kSlotX[i];
        float y = kSlotY;
        if (m == Mode::Pocket && fly_ == i) {
            float e = ease(pocketT_ / std::max(0.01f, kPocketWait));
            x = slipFromX_ + (kSlotX[i] - slipFromX_) * e;
            y = slipFromY_ + (kSlotY - slipFromY_) * e;
        }
        const gs::Image& slip = art_.slip[i];
        spr(slip, x + jx, y, float(slip.w), float(slip.h), PAL_PAPER);
    }
    for (int i = 0; i < kTapeN; i++) spr(art_.slot, kSlotX[i], kSlotY, 44.f, 12.f, PAL_WOOD);
    spr(art_.drawer, 180.f, 214.f, float(art_.drawer.w), float(art_.drawer.h), PAL_WOOD);
    spr(art_.tape, 160.f, 190.f, float(art_.tape.w), float(art_.tape.h), PAL_PAPER);

    float ballX = bowlerX() - 8.f;
    float ballY = kFeetY - 44.f + std::sin(clock_ * 6.f) * 2.f;
    float shadeX = ballX;
    float shadeY = ballY + 10.f;
    bool showBall = true;
    if (m == Mode::Flight || m == Mode::Pocket || m == Mode::Judge) {
        float u = m == Mode::Flight ? float(flightT_) / float(kFlightN) : 1.f;
        flightPoint(u, ballX, ballY, shadeX, shadeY);
    } else if (leaving || failed) showBall = false;

    int spin = (int(clock_ * 10.f) + flightT_) & 1;
    if (showBall) {
        spr(art_.ball[spin], ballX + jx, ballY, 12.f, 12.f, PAL_BALL);
        spr(art_.shadow, shadeX, shadeY, 12.f, 5.f, PAL_WOOD, false, true);
    }

    float batFeet = leaving ? feetX_ : kBatX;
    if (m == Mode::Leave) batFeet += std::sin(leaveT_ * 14.f) * 0.f;
    int pose = batPose();
    float batH = 58.f;
    spr(art_.batsman[pose], batFeet, kFeetY - batH * 0.5f, 36.f, batH, PAL_KIT);
    spr(art_.shadow, batFeet, kFeetY + 2.f, 22.f, 6.f, PAL_WOOD, false, true);

    float bx = bowlerX();
    float bowlH = 52.f;
    spr(art_.bowler[bowlPose()], bx, kFeetY - bowlH * 0.5f + 2.f, 34.f, bowlH, PAL_BOWL, true);
    spr(art_.shadow, bx, kFeetY + 2.f, 18.f, 6.f, PAL_WOOD, false, true);

    spr(art_.keeper, 34.f, kFeetY - 16.f, 22.f, 32.f, PAL_BOWL);
    spr(art_.stump, kNearX, kFeetY - 20.f, 22.f, 42.f, PAL_WOOD);
    spr(art_.stump, kFarX, kFeetY - 16.f, 18.f, 36.f, PAL_WOOD);

    for (int i = 0; i < kGapN; i++) {
        const Gap& g = kGap[i];
        bool on = aiming && i == hot;
        float pulse = on ? 20.f + std::sin(clock_ * 8.f) * 1.6f : 16.f;
        int pal = g.line < 0 ? PAL_TWIN : PAL_FIELD;
        if (g.line >= 0 && held_[g.line]) pal = PAL_FIELD;
        spr(art_.fielder, g.fx, g.fy - pulse * 0.15f, pulse * 0.72f, pulse, pal);
        if (on) spr(art_.blot, g.fx, g.fy + 8.f, 10.f, 4.f, g.line < 0 ? PAL_BAD : PAL_GOOD);
    }

    float ropeY = 118.f + std::sin(clock_ * 2.2f) * 1.4f;
    spr(art_.rope, 304.f, ropeY, 28.f, 96.f, PAL_ROPE);
    spr(art_.pitch, 140.f, 146.f, 210.f, 48.f, PAL_WOOD);
    spr(art_.screen, 232.f, 96.f, 34.f, 26.f, PAL_HOUSE);
    spr(art_.house, 28.f, 92.f, 64.f, 36.f, PAL_HOUSE);
    spr(art_.crowd, 92.f, 100.f, 96.f, 20.f, PAL_CROWD);
    spr(art_.crowd, 210.f, 98.f, 90.f, 18.f, PAL_CROWD);
    const float trees[] = {124.f, 168.f, 292.f};
    const float treeY[] = {86.f, 90.f, 94.f};
    for (int i = 0; i < 3; i++) spr(art_.tree, trees[i], treeY[i], 36.f, 40.f, PAL_GRASS);
    spr(art_.cloud, 70.f, 28.f, 36.f, 14.f, PAL_SKY);
    spr(art_.cloud, 150.f, 18.f, 30.f, 12.f, PAL_SKY);
    spr(art_.sun, 286.f, 22.f, 16.f, 16.f, PAL_SKY);

    char buf[48];
    int showBallN = std::min(balls_ + (aiming ? 1 : 0), kMaxBalls);
    if (showBallN < 1) showBallN = 1;
    hud(1, 0, "WICKET", PAL_GOLD);
    std::snprintf(buf, sizeof buf, "BALL %d/%d", showBallN, kMaxBalls);
    hud(40 - int(std::strlen(buf)) - 1, 0, buf, PAL_INK);
    std::snprintf(buf, sizeof buf, "TILL %d", drawerScore());
    hud(1, 1, buf, matched() ? PAL_GOOD : PAL_INK);
    std::snprintf(buf, sizeof buf, "BOARD %d", board_);
    hud(40 - int(std::strlen(buf)) - 1, 1, buf, board_ == drawerScore() ? PAL_INK : PAL_BAD);

    for (int i = 0; i < kTapeN; i++) {
        std::snprintf(buf, sizeof buf, "%s %d", tapeName(i), tapePay(i));
        int col = 8 + i * 10;
        hud(col, 22, buf, held_[i] ? PAL_GOOD : PAL_GOLD);
    }

    if (title) {
        hudC(4, "PLAY WICKET", PAL_INK);
        hudC(5, "MATCH THE TAPE", PAL_GOLD);
        hudC(7, "TWINS PAY AND STAY OUT", PAL_BAD);
        hudC(9, "L-R STROKE   Z PLAYS", PAL_INK);
        hudC(10, "ENTER STARTS", PAL_GOOD);
    } else if (mode_ == Mode::Pause) {
        hudC(4, "PAUSED", PAL_INK);
        hudC(6, "ENTER RESUMES", PAL_GOLD);
    } else if (m == Mode::Aim) {
        const Gap& g = kGap[hot];
        if (g.line >= 0) {
            std::snprintf(buf, sizeof buf, held_[g.line] ? "%s IN" : "%s  %d", g.name, g.pay);
            hudC(3, buf, sweetMeter(meter_) && !held_[g.line] ? PAL_GOOD : PAL_GOLD);
        } else {
            std::snprintf(buf, sizeof buf, "%s STAYS OUT", g.name);
            hudC(3, buf, PAL_BAD);
        }
        if (!sweetMeter(meter_)) hudC(4, meter_ < 0.5f ? "EARLY" : "LATE", PAL_BAD);
        else hudC(4, "SWEET", PAL_GOOD);
        hudC(16, "L-R STROKE  U TAPE  D TWIN  Z PLAYS", PAL_INK);
    } else if (m == Mode::Judge || m == Mode::Pocket) {
        int pal = m == Mode::Pocket ? PAL_GOOD : PAL_BAD;
        hudC(3, reason_, pal);
    } else if (leaving) {
        hudC(4, "THE DRAWER MATCHES", PAL_GOOD);
        hudC(5, "YOU CAN LEAVE", PAL_GOLD);
        if (m == Mode::Over && !bot_) hudC(7, "ENTER PLAYS AGAIN", PAL_INK);
    } else if (failed) {
        hudC(4, "STILL OPEN", PAL_BAD);
        hudC(6, reason_[0] ? reason_ : "DOES NOT MATCH", PAL_INK);
        if (!bot_) hudC(8, "ENTER PLAYS AGAIN", PAL_INK);
    }
}

}  // namespace wickettape

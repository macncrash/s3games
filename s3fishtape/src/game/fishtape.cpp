#include "game/fishtape.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace fishtape {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kMeterRate = 0.70f;
constexpr float kTitleWait = 0.55f;
constexpr float kPocketWait = 0.42f;
constexpr float kJudgeWait = 0.48f;
constexpr float kLeaveWait = 1.10f;
constexpr int kFlightN = 32;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

float ease(float u) {
    u = clampf(u, 0.f, 1.f);
    return u * u * (3.f - 2.f * u);
}

const gs::Image& bodyOf(const Art& art, int fish) {
    int shape = (fish >= 0 && fish < kFishN) ? shapeOf(kFish[fish].kind) : 3;
    if (shape == 0) return art.dab;
    if (shape == 1) return art.bass;
    if (shape == 2) return art.pike;
    return art.twin;
}

int palOf(int fish) {
    if (fish < 0 || fish >= kFishN) return PAL_TWIN;
    if (kFish[fish].line < 0) return PAL_TWIN;
    if (kFish[fish].line == 0) return PAL_DAB;
    if (kFish[fish].line == 1) return PAL_BASS;
    return PAL_PIKE;
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
        std::fprintf(stderr, "s3fishtape %s\n", msg);
        ok = false;
    };
    const char* wantName[kTapeN] = {"DAB", "BASS", "PIKE"};
    const int wantPay[kTapeN] = {1, 3, 6};
    const char* wantTwin[kTapeN] = {"GOBY", "PERCH", "CARP"};
    const Kind wantKind[kTapeN] = {Kind::Dab, Kind::Bass, Kind::Pike};
    const Kind twinKind[kTapeN] = {Kind::Goby, Kind::Perch, Kind::Carp};
    if (kMaxCasts != 6 || kTapeN != 3 || kFishN != 6) bad("the session is the wrong length");
    int seen[kTapeN] = {-1, -1, -1};
    int payCount[kTapeN] = {};
    int kindCount[kFishN] = {};
    for (int i = 0; i < kFishN; i++) {
        const Fish& f = kFish[i];
        int kd = int(f.kind);
        if (kd < 0 || kd >= kFishN) bad("kind out of range");
        else kindCount[kd]++;
        if (f.line >= kTapeN) {
            bad("line out of range");
            continue;
        }
        if (f.line >= 0) {
            if (seen[f.line] >= 0) bad("tape line repeated");
            seen[f.line] = i;
        }
        int which = -1;
        for (int t = 0; t < kTapeN; t++)
            if (f.pay == wantPay[t]) which = t;
        if (which < 0) bad("pay is not on the tape");
        else payCount[which]++;
        if (f.x < 40.f || f.x > 312.f || f.y < 110.f || f.y > 200.f) bad("a fish leaves the water");
        if (f.sink <= 0.f) bad("a cast has no sink");
    }
    for (int i = 0; i < kFishN; i++)
        if (kindCount[i] != 1) bad("a kind is missing");
    for (int i = 0; i < kFishN; i++) {
        for (int j = i + 1; j < kFishN; j++) {
            float d = std::hypot(kFish[i].x - kFish[j].x, kFish[i].y - kFish[j].y);
            if (d < 28.f) bad("fish lie on each other");
            if (std::strcmp(kFish[i].name, kFish[j].name) == 0) bad("two fish share a name");
        }
    }
    if (!sweetMeter(0.5f) || !sweetMeter(0.5f - kSweet) || !sweetMeter(0.5f + kSweet)) bad("sweet window slipped");
    if (sweetMeter(0.5f - kSweet - 0.03f) || sweetMeter(0.5f + kSweet + 0.03f)) bad("sweet window grew");
    int trap = 0;
    for (int t = 0; t < kTapeN; t++) {
        if (seen[t] < 0) bad("tape line missing");
        if (payCount[t] != 2) bad("a pay does not have one twin");
        int s = fishOfLine(t);
        int twin = twinOf(t);
        if (s < 0 || twin < 0 || s == twin) {
            bad("tape and twin did not pair");
            continue;
        }
        if (std::strcmp(kFish[s].name, wantName[t]) != 0 || kFish[s].pay != wantPay[t]) bad("tape name drifted");
        if (kFish[s].kind != wantKind[t]) bad("tape kind drifted");
        if (std::strcmp(kFish[twin].name, wantTwin[t]) != 0 || kFish[twin].pay != wantPay[t]) bad("twin drifted");
        if (kFish[twin].line >= 0 || kFish[twin].kind != twinKind[t]) bad("twin counted on the tape");
        if (takenLine(true, s) != t) bad("firm cast missed the tape");
        if (takenLine(false, s) != -1) bad("a mistimed cast filled the tape");
        if (takenLine(true, twin) != -1 || !isTwin(true, twin)) bad("twin filled the tape");
        if (isTwin(false, twin)) bad("a mistimed twin counted");
        if (std::strcmp(tapeName(t), wantName[t]) != 0 || tapePay(t) != wantPay[t]) bad("tape label drifted");
        trap += kFish[twin].pay;
    }
    if (tapeSum() != 10 || trap != 10) bad("a close drawer is not still the same total");
    int pike = fishOfLine(2);
    int carp = twinOf(2);
    int dab = fishOfLine(0);
    if (pike < 0 || carp < 0 || dab < 0) bad("the deep fish are missing");
    else {
        if (kFish[pike].sink <= kFish[carp].sink) bad("the pike does not run under the carp");
        if (kFish[carp].sink <= kFish[dab].sink) bad("the carp does not sink");
        if (kFish[pike].y > 140.f) bad("the pike is not high in the run");
        if (kFish[dab].y < 140.f) bad("the dab leaves the shallows");
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
    casts_ = 0;
    traps_ = 0;
    board_ = 0;
    shake_ = 0;
    flightT_ = 0;
    fly_ = -1;
    shot_ = fishOfLine(0);
    pick_ = shot_ < 0 ? 0 : shot_;
    sweetWas_ = false;
    meter_ = 0.f;
    meterDir_ = 1.f;
    pocketT_ = judgeT_ = leaveT_ = 0.f;
    feetX_ = kRodX;
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
    casts_ = 0;
    traps_ = 0;
    board_ = 0;
    shake_ = 0;
    flightT_ = 0;
    meter_ = 0.f;
    meterDir_ = 1.f;
    feetX_ = kRodX;
    pick_ = fishOfLine(0);
    if (pick_ < 0) pick_ = 0;
    for (int i = 0; i < kTapeN; i++) held_[i] = false;
    beginAim();
    blip(0, 392.f, 0.05f, 0.08f);
}

void Game::play() {
    if (mode_ != Mode::Aim || !rules_ || casts_ >= kMaxCasts) return;
    if (pick_ < 0 || pick_ >= kFishN) return;
    casts_++;
    shot_ = pick_;
    bool firm = sweetMeter(meter_);
    call_ = firm ? Call::Play : (meter_ < 0.5f ? Call::Soon : Call::Late);
    tipX_ = feetX_ + 18.f;
    tipY_ = kFeetY - 48.f;
    flightT_ = 0;
    mode_ = Mode::Flight;
    if (sys_) sys_->apu.noiseBurst(0.08f, 1400.f, 0.04f);
    blip(0, 320.f, 0.04f, 0.05f);
}

void Game::settle() {
    bool firm = call_ == Call::Play;
    int line = takenLine(firm, shot_);
    bool twin = isTwin(firm, shot_);
    if (call_ == Call::Soon) std::snprintf(reason_, sizeof reason_, "SHORT OF THE FISH");
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
        slipFromX_ = kFish[shot_].x;
        slipFromY_ = kFish[shot_].y;
        pocketT_ = 0.f;
        mode_ = Mode::Pocket;
        std::snprintf(reason_, sizeof reason_, "IN THE DRAWER");
        if (line == 2) chord(330.f, 440.f, 554.f, 0.28f);
        else blip(1, 440.f + float(line) * 70.f, 0.06f, 0.16f);
        if (sys_) {
            sys_->apu.noiseBurst(0.12f, line == 2 ? 380.f : 900.f, 0.05f);
            sys_->rumble(0.2f, line == 2 ? 0.55f : 0.3f, 90);
            sys_->setLight(40, 140, 160);
        }
        return;
    } else if (twin) {
        traps_++;
        board_ += kFish[shot_].pay;
        std::snprintf(reason_, sizeof reason_, "%s STAYS OUT", kFish[shot_].name);
        mode_ = Mode::Judge;
        judgeT_ = 0.f;
        shake_ = 8;
        blip(1, 110.f, 0.07f, 0.18f);
        if (sys_) {
            sys_->apu.noiseBurst(0.16f, 220.f, 0.07f);
            sys_->rumble(0.25f, 0.06f, 70);
            sys_->setLight(140, 50, 30);
        }
        return;
    } else std::snprintf(reason_, sizeof reason_, "MISSED");
    mode_ = Mode::Judge;
    judgeT_ = 0.f;
    shake_ = 5;
    blip(1, 90.f, 0.06f, 0.14f);
    if (sys_) sys_->rumble(0.12f, 0.04f, 40);
}

void Game::beginLeave() {
    if (!matched() || traps_ != 0 || board_ != drawerScore() || mode_ == Mode::Leave || mode_ == Mode::Over) return;
    mode_ = Mode::Leave;
    leaveT_ = 0.f;
    left_ = true;
    feetX_ = kRodX;
    fly_ = -1;
    std::snprintf(reason_, sizeof reason_, "THE DRAWER MATCHES THE TAPE");
    chord(330.f, 415.f, 494.f, 0.7f);
    if (sys_) {
        sys_->rumble(0.2f, 0.45f, 140);
        sys_->setLight(80, 180, 200);
    }
}

void Game::beginLose(const char* why) {
    mode_ = Mode::Lose;
    won_ = false;
    over_ = true;
    left_ = false;
    if (why && why[0]) std::snprintf(reason_, sizeof reason_, "%s", why);
    else if (!reason_[0]) std::snprintf(reason_, sizeof reason_, "DOES NOT MATCH");
    blip(0, 78.f, 0.07f, 0.28f);
    if (sys_) sys_->setLight(140, 30, 28);
}

void Game::finishLeave() {
    won_ = rules_ && matched() && left_ && traps_ == 0 && drawerScore() == tapeSum() && board_ == drawerScore() &&
           casts_ >= kTapeN && casts_ <= kMaxCasts;
    over_ = true;
    mode_ = Mode::Over;
    if (!won_) std::snprintf(reason_, sizeof reason_, "DOES NOT MATCH");
}

void Game::continuePlay() {
    if (matched() && traps_ == 0 && board_ == drawerScore()) beginLeave();
    else if (casts_ >= kMaxCasts) beginLose(traps_ ? "TWIN STILL OUT" : "DOES NOT MATCH");
    else beginAim();
}

void Game::nudge(int dir) {
    pick_ = (pick_ + dir + kFishN) % kFishN;
    if (!bot_) blip(2, 480.f + float(pick_) * 28.f, 0.03f, 0.04f);
}

void Game::snapTape() {
    if (pick_ < 0 || pick_ >= kFishN) return;
    int line = kFish[pick_].line;
    int next = -1;
    if (line < 0) {
        for (int t = 0; t < kTapeN; t++)
            if (tapePay(t) == kFish[pick_].pay) next = fishOfLine(t);
    } else next = fishOfLine((line + 1) % kTapeN);
    if (next >= 0) pick_ = next;
    if (!bot_) blip(2, 620.f, 0.03f, 0.04f);
}

void Game::snapTwin() {
    if (pick_ < 0 || pick_ >= kFishN) return;
    int line = kFish[pick_].line;
    int next = -1;
    if (line >= 0) next = twinOf(line);
    else {
        int idx = 0;
        for (int t = 0; t < kTapeN; t++)
            if (tapePay(t) == kFish[pick_].pay) idx = t;
        next = twinOf((idx + 1) % kTapeN);
    }
    if (next >= 0) pick_ = next;
    if (!bot_) blip(2, 220.f, 0.03f, 0.04f);
}

void Game::botAim() {
    int want = fishOfLine(openLine());
    if (want < 0) return;
    if (pick_ != want) {
        int cw = (want - pick_ + kFishN) % kFishN;
        int ccw = (pick_ - want + kFishN) % kFishN;
        pick_ = cw <= ccw ? (pick_ + 1) % kFishN : (pick_ + kFishN - 1) % kFishN;
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
    int hotLine = (pick_ >= 0 && pick_ < kFishN) ? kFish[pick_].line : -1;
    if (sw && !sweetWas_) {
        bool tape = hotLine >= 0 && !held_[hotLine];
        blip(2, tape ? 880.f : 200.f, 0.03f, 0.04f);
    }
    sweetWas_ = sw;
    if (fire) play();
}

void Game::flightPoint(float u, float& x, float& y) const {
    u = clampf(u, 0.f, 1.f);
    float tx, ty, sink;
    if (call_ == Call::Soon) {
        tx = tipX_ + 28.f;
        ty = 132.f;
        sink = 6.f;
    } else if (call_ == Call::Late) {
        tx = tipX_ + 8.f;
        ty = kFeetY - 8.f;
        sink = 2.f;
    } else {
        const Fish& f = kFish[shot_ >= 0 && shot_ < kFishN ? shot_ : 0];
        tx = f.x;
        ty = f.y;
        sink = f.sink;
    }
    x = tipX_ + (tx - tipX_) * u;
    y = tipY_ + (ty - tipY_) * u - std::sin(u * kPi) * (sink * 0.35f);
}

int Game::anglerPose() const {
    if (mode_ == Mode::Leave || (mode_ == Mode::Over && won_)) return 2;
    if ((mode_ == Mode::Flight || mode_ == Mode::Pocket) && flightT_ < 16) return 1;
    return 0;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    rules_ = audit();
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(4, 8, 12));
    sys.apu.setMaster(0.7f);
    sys.apu.setEcho(0.08f, 0.16f, 0.06f);
    if (!rules_) {
        over_ = true;
        won_ = false;
        left_ = false;
        mode_ = Mode::Lose;
        if (!reason_[0]) std::snprintf(reason_, sizeof reason_, "RULES");
        std::fprintf(stderr, "s3fishtape rules failed\n");
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
        feetX_ -= 70.f * dt;
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
        if (y < 36) c = gs::rgb4(6, 10, 14);
        else if (y < 70) c = gs::rgb4(8, 12, 15);
        else if (y < 108) c = gs::rgb4(10, 12, 9);
        else if (y < 132) c = gs::rgb4(3, 8, 6);
        else {
            int band = ((y / 4) + int(clock_ * 6.f)) & 1;
            c = band ? gs::rgb4(1, 5, 10) : gs::rgb4(2, 6, 12);
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

    const Mode m = mode_ == Mode::Pause ? heldMode_ : mode_;
    const bool title = m == Mode::Title;
    const bool aiming = m == Mode::Aim || title;
    const bool leaving = m == Mode::Leave || (m == Mode::Over && won_);
    const bool failed = m == Mode::Lose || (m == Mode::Over && !won_);
    const float jx = shake_ > 0 ? std::sin(float(shake_) * 1.7f) * 2.f : 0.f;
    const int hot = (pick_ >= 0 && pick_ < kFishN) ? pick_ : 0;

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
    for (int i = 0; i < kTapeN; i++) spr(art_.blot, kSlotX[i], kSlotY, 18.f, 4.f, PAL_WOOD);
    spr(art_.creel, 180.f, 214.f, float(art_.creel.w), float(art_.creel.h), PAL_WOOD);
    spr(art_.tape, 160.f, 190.f, float(art_.tape.w), float(art_.tape.h), PAL_PAPER);

    float lureX = tipX_;
    float lureY = tipY_;
    bool showLure = m == Mode::Flight || m == Mode::Pocket || m == Mode::Judge;
    if (showLure) {
        float u = m == Mode::Flight ? float(flightT_) / float(kFlightN) : 1.f;
        flightPoint(u, lureX, lureY);
        spr(art_.lure, lureX + jx, lureY, 8.f, 10.f, PAL_LURE);
    }

    for (int i = 0; i < kFishN; i++) {
        const Fish& f = kFish[i];
        if (f.line >= 0 && held_[f.line] && !(m == Mode::Pocket && fly_ == f.line && pocketT_ < 0.08f)) continue;
        bool on = aiming && i == hot;
        float bob = std::sin(clock_ * 2.4f + float(i)) * 2.f;
        float h = on ? 16.f : 13.f;
        if (f.kind == Kind::Pike || f.kind == Kind::Carp) h += 1.f;
        const gs::Image& img = bodyOf(art_, i);
        float w = h * (float(img.w) / std::max(1, int(img.h)));
        spr(img, f.x, f.y + bob, w, h, palOf(i), (int(clock_ * 1.5f) + i) & 2);
        if (on) spr(art_.blot, f.x, f.y + 10.f, 8.f, 4.f, f.line < 0 ? PAL_BAD : PAL_GOOD);
    }

    float ax = leaving ? feetX_ : kRodX;
    int pose = anglerPose();
    spr(art_.angler[pose], ax, kFeetY - 20.f, 30.f, 40.f, PAL_KIT);
    spr(art_.jetty, 56.f, 128.f, 96.f, 22.f, PAL_WOOD);
    spr(art_.reed, 18.f, 112.f, 16.f, 32.f, PAL_REED);
    spr(art_.reed, 104.f, 116.f, 14.f, 28.f, PAL_REED);
    spr(art_.reed, 300.f, 108.f, 16.f, 34.f, PAL_REED);
    spr(art_.cloud, 70.f, 28.f, 36.f, 14.f, PAL_SKY);
    spr(art_.cloud, 180.f, 18.f, 30.f, 12.f, PAL_SKY);
    spr(art_.sun, 286.f, 22.f, 16.f, 16.f, PAL_SUN);

    char buf[48];
    int showN = std::min(casts_ + (aiming ? 1 : 0), kMaxCasts);
    if (showN < 1) showN = 1;
    hud(1, 0, "FISH", PAL_GOLD);
    std::snprintf(buf, sizeof buf, "CAST %d/%d", showN, kMaxCasts);
    hud(40 - int(std::strlen(buf)) - 1, 0, buf, PAL_INK);
    std::snprintf(buf, sizeof buf, "TILL %d", drawerScore());
    hud(1, 1, buf, matched() ? PAL_GOOD : PAL_INK);
    std::snprintf(buf, sizeof buf, "BOARD %d", board_);
    hud(40 - int(std::strlen(buf)) - 1, 1, buf, board_ == drawerScore() ? PAL_INK : PAL_BAD);

    for (int i = 0; i < kTapeN; i++) {
        std::snprintf(buf, sizeof buf, "%s %d", tapeName(i), tapePay(i));
        hud(8 + i * 10, 22, buf, held_[i] ? PAL_GOOD : PAL_GOLD);
    }

    if (title) {
        hudC(4, "PLAY FISH", PAL_INK);
        hudC(5, "MATCH THE TAPE", PAL_GOLD);
        hudC(7, "TWINS PAY AND STAY OUT", PAL_BAD);
        hudC(9, "L-R FISH   Z CASTS", PAL_INK);
        hudC(10, "ENTER STARTS", PAL_GOOD);
    } else if (mode_ == Mode::Pause) {
        hudC(4, "PAUSED", PAL_INK);
        hudC(6, "ENTER RESUMES", PAL_GOLD);
    } else if (m == Mode::Aim) {
        const Fish& f = kFish[hot];
        if (f.line >= 0) {
            std::snprintf(buf, sizeof buf, held_[f.line] ? "%s IN" : "%s  %d", f.name, f.pay);
            hudC(3, buf, sweetMeter(meter_) && !held_[f.line] ? PAL_GOOD : PAL_GOLD);
        } else {
            std::snprintf(buf, sizeof buf, "%s STAYS OUT", f.name);
            hudC(3, buf, PAL_BAD);
        }
        if (!sweetMeter(meter_)) hudC(4, meter_ < 0.5f ? "EARLY" : "LATE", PAL_BAD);
        else hudC(4, "SWEET", PAL_GOOD);
    } else if (m == Mode::Flight) {
        hudC(3, "LINE OUT", PAL_INK);
    } else if (m == Mode::Pocket || m == Mode::Judge || leaving || failed) {
        hudC(3, reason_, leaving ? PAL_GOOD : (failed ? PAL_BAD : PAL_GOLD));
    }
    if (leaving) hudC(5, "LEAVE THE BANK", PAL_GOOD);
    if (failed && reason_[0]) hudC(5, "THE DRAWER IS OPEN", PAL_BAD);
}

}  // namespace fishtape

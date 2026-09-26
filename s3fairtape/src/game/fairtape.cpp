#include "game/fairtape.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace fairtape {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kBaseY = 136.f;
constexpr float kJarH = 40.f;
constexpr float kNeckY = 108.f;
constexpr float kHangY = 68.f;
constexpr float kRailY = 56.f;
constexpr float kNameY = 82.f;
constexpr float kTopY = 98.f;
constexpr float kShelfY = 140.f;
constexpr float kTapeY = 156.f;
constexpr float kDrawerY = 180.f;
constexpr float kSlotY = 180.f;
constexpr float kFrontY = 150.f;
constexpr float kHotY = 58.f;
constexpr float kMeterY = 24.f;
constexpr float kKidX = 22.f;
constexpr float kKidFeet = 136.f;
constexpr float kAimHome = 160.f;
constexpr float kBotStep = 4.f;
constexpr float kAimSpeed = 2.5f;
constexpr float kSweet = 0.12f;
constexpr float kMeterRate = 1.5f;
constexpr int kFlightN = 16;
constexpr int kMaxRings = 8;
constexpr float kPocketWait = 0.48f;
constexpr float kJudgeWait = 0.60f;
constexpr float kLeaveWait = 1.05f;
constexpr float kTitleWait = 0.40f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

int jarPal(Shape shape) {
    switch (shape) {
        case Shape::Rose: return PAL_ROSE;
        case Shape::Bud: return PAL_CREAM;
        case Shape::Cane: return PAL_CANE;
        case Shape::Stick: return PAL_WOOD;
        case Shape::Bear: return PAL_BEAR;
        case Shape::Cub: return PAL_BEAR;
    }
    return PAL_WOOD;
}

const gs::Mipped* topper(const Art& art, Shape shape) {
    switch (shape) {
        case Shape::Rose: return &art.rose;
        case Shape::Bud: return &art.bud;
        case Shape::Cane: return &art.cane;
        case Shape::Stick: return &art.stick;
        case Shape::Bear: return &art.bear;
        case Shape::Cub: return &art.cub;
    }
    return &art.rose;
}

float ease(float u) {
    u = clampf(u, 0.f, 1.f);
    return u * u * (3.f - 2.f * u);
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
        std::fprintf(stderr, "s3fairtape %s\n", msg);
        ok = false;
    };
    if (kHit * 2.f >= kShelfGap) bad("hit windows overlap");
    const char* wantName[kTapeN] = {"ROSE", "CANE", "BEAR"};
    const int wantPay[kTapeN] = {15, 24, 40};
    const char* wantTwin[kTapeN] = {"BUD", "STICK", "CUB"};
    const Shape wantShape[kTapeN] = {Shape::Rose, Shape::Cane, Shape::Bear};
    const Shape twinShape[kTapeN] = {Shape::Bud, Shape::Stick, Shape::Cub};
    int seenLine[kTapeN] = {-1, -1, -1};
    int payCount[kTapeN] = {};
    for (int i = 0; i < kShelfN; i++) {
        if (shelfAt(shelfX(i)) != i) bad("center missed its bottle");
        if (shelfAt(shelfX(i) + kHit) != i || shelfAt(shelfX(i) - kHit) != i) bad("rim missed its bottle");
        if (shelfAt(shelfX(i) + kHit + 2.f) != -1 || shelfAt(shelfX(i) - kHit - 2.f) != -1)
            bad("outside the rim still hit");
        if (i + 1 < kShelfN && shelfAt((shelfX(i) + shelfX(i + 1)) * 0.5f) != -1) bad("gap counted as a bottle");
        int line = kShelf[i].line;
        if (line >= kTapeN) {
            bad("line out of range");
            continue;
        }
        if (line >= 0) {
            if (seenLine[line] >= 0) bad("tape line repeated");
            seenLine[line] = i;
        }
        int which = -1;
        for (int t = 0; t < kTapeN; t++)
            if (kShelf[i].pay == wantPay[t]) which = t;
        if (which < 0) bad("pay is not on the tape");
        else payCount[which]++;
    }
    if (shelfAt(kAimHome) != -1) bad("home aim sits on a bottle");
    for (int t = 0; t < kTapeN; t++) {
        if (seenLine[t] < 0) bad("tape line missing");
        if (payCount[t] != 2) bad("a pay does not have one twin");
        int s = shelfOfLine(t);
        int twin = twinShelf(t);
        if (s < 0 || twin < 0 || s == twin) {
            bad("tape and twin did not pair");
            continue;
        }
        if (std::strcmp(kShelf[s].name, wantName[t]) != 0 || kShelf[s].pay != wantPay[t]) bad("tape name drifted");
        if (kShelf[s].shape != wantShape[t]) bad("tape shape drifted");
        if (std::strcmp(kShelf[twin].name, wantTwin[t]) != 0 || kShelf[twin].pay != wantPay[t]) bad("twin drifted");
        if (kShelf[twin].line >= 0 || kShelf[twin].shape != twinShape[t]) bad("twin counted on the tape");
        if (takenLine(Fate::Firm, shelfX(s)) != t) bad("firm ring missed the tape");
        if (takenLine(Fate::Short, shelfX(s)) != -1 || takenLine(Fate::Hot, shelfX(s)) != -1)
            bad("a soft ring filled the tape");
        if (takenLine(Fate::Firm, shelfX(twin)) != -1 || !isTwin(Fate::Firm, shelfX(twin)))
            bad("twin filled the tape");
        if (isTwin(Fate::Short, shelfX(twin)) || isTwin(Fate::Hot, shelfX(twin))) bad("a soft twin counted");
        if (std::strcmp(tapeName(t), wantName[t]) != 0 || tapePay(t) != wantPay[t]) bad("tape label drifted");
    }
    if (tapeSum() != 79) bad("tape does not sum to 79");
    if (shelfAt(-20.f) != -1 || shelfAt(340.f) != -1) bad("off the booth still hit");
    return ok;
}

bool Game::sweet() const { return std::fabs(meter_ - 0.5f) <= kSweet; }

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
    rings_ = 0;
    traps_ = 0;
    shake_ = 0;
    flightT_ = 0;
    fly_ = -1;
    wasSweet_ = false;
    aimX_ = kAimHome;
    targetX_ = kAimHome;
    meter_ = 0.f;
    meterDir_ = 1.f;
    pocketT_ = judgeT_ = leaveT_ = 0.f;
    reason_[0] = 0;
    for (int i = 0; i < kTapeN; i++) held_[i] = false;
    for (int i = 0; i < kShelfN; i++) seated_[i] = false;
    mode_ = Mode::Title;
    if (sys_) sys_->apu.silence();
}

void Game::beginAim() {
    int line = openLine();
    int s = shelfOfLine(line);
    targetX_ = s >= 0 ? shelfX(s) : kAimHome;
    meter_ = 0.f;
    meterDir_ = 1.f;
    wasSweet_ = false;
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
    rings_ = 0;
    traps_ = 0;
    shake_ = 0;
    flightT_ = 0;
    aimX_ = kAimHome;
    for (int i = 0; i < kTapeN; i++) held_[i] = false;
    for (int i = 0; i < kShelfN; i++) seated_[i] = false;
    beginAim();
    blip(0, 392.f, 0.05f, 0.08f);
}

void Game::launch() {
    if (mode_ != Mode::Aim || !rules_ || rings_ >= kMaxRings) return;
    rings_++;
    fromX_ = aimX_;
    fromY_ = kHangY;
    landX_ = aimX_;
    if (sweet()) {
        fate_ = Fate::Firm;
        landY_ = kNeckY;
    } else if (meter_ < 0.5f) {
        fate_ = Fate::Short;
        landY_ = kFrontY;
    } else {
        fate_ = Fate::Hot;
        landY_ = kHotY;
    }
    flightT_ = 0;
    mode_ = Mode::Flight;
    if (sys_) sys_->apu.noiseBurst(0.14f, 1800.f, 0.04f);
}

void Game::stick() {
    int line = takenLine(fate_, landX_);
    bool twin = isTwin(fate_, landX_);
    int shelf = (fate_ == Fate::Firm) ? shelfAt(landX_) : -1;
    if (fate_ == Fate::Short) std::snprintf(reason_, sizeof reason_, "SHORT");
    else if (fate_ == Fate::Hot) std::snprintf(reason_, sizeof reason_, "HOT");
    else if (line >= 0) {
        if (shelf >= 0) seated_[shelf] = true;
        if (held_[line]) {
            std::snprintf(reason_, sizeof reason_, "ALREADY IN");
            mode_ = Mode::Judge;
            judgeT_ = 0.f;
            shake_ = 4;
            blip(1, 160.f, 0.05f, 0.14f);
            return;
        }
        held_[line] = true;
        fly_ = line;
        slipFromX_ = shelfX(shelf);
        slipFromY_ = kNeckY;
        pocketT_ = 0.f;
        mode_ = Mode::Pocket;
        std::snprintf(reason_, sizeof reason_, "IN THE DRAWER");
        blip(1, 523.f + float(line) * 80.f, 0.06f, 0.2f);
        if (sys_) {
            sys_->rumble(0.15f, 0.4f, 70);
            sys_->setLight(80, 170, 70);
        }
        return;
    } else if (twin && shelf >= 0) {
        seated_[shelf] = true;
        traps_++;
        std::snprintf(reason_, sizeof reason_, "%s STAYS OUT", kShelf[shelf].name);
        mode_ = Mode::Judge;
        judgeT_ = 0.f;
        shake_ = 8;
        blip(1, 110.f, 0.07f, 0.22f);
        if (sys_) {
            sys_->apu.noiseBurst(0.22f, 240.f, 0.08f);
            sys_->rumble(0.4f, 0.1f, 80);
            sys_->setLight(170, 40, 30);
        }
        return;
    } else {
        std::snprintf(reason_, sizeof reason_, "MISS");
    }
    mode_ = Mode::Judge;
    judgeT_ = 0.f;
    shake_ = 5;
    blip(1, fate_ == Fate::Firm ? 90.f : 80.f, 0.06f, 0.16f);
    if (sys_) sys_->rumble(0.25f, 0.05f, 50);
}

void Game::beginLeave() {
    if (!matched()) return;
    mode_ = Mode::Leave;
    leaveT_ = 0.f;
    left_ = true;
    fly_ = -1;
    std::snprintf(reason_, sizeof reason_, "THE DRAWER MATCHES THE TAPE");
    chord(392.f, 523.f, 784.f, 0.9f);
    if (sys_) {
        sys_->rumble(0.3f, 0.65f, 160);
        sys_->setLight(255, 200, 80);
    }
}

void Game::beginLose(const char* why) {
    mode_ = Mode::Lose;
    won_ = false;
    over_ = true;
    left_ = false;
    if (why && why[0]) std::snprintf(reason_, sizeof reason_, "%s", why);
    else if (!reason_[0]) std::snprintf(reason_, sizeof reason_, "DOES NOT MATCH");
    blip(0, 82.f, 0.07f, 0.3f);
    if (sys_) sys_->setLight(150, 30, 28);
}

void Game::continuePlay() {
    if (matched()) beginLeave();
    else if (rings_ >= kMaxRings) beginLose("DOES NOT MATCH");
    else beginAim();
}

void Game::botAim() {
    int line = openLine();
    int s = shelfOfLine(line);
    if (s < 0) return;
    targetX_ = shelfX(s);
    float dx = targetX_ - aimX_;
    float ad = std::fabs(dx);
    if (ad > 0.4f) {
        float step = std::min(kBotStep, ad);
        aimX_ += dx > 0.f ? step : -step;
        return;
    }
    aimX_ = targetX_;
    if (sweet() && takenLine(Fate::Firm, aimX_) == line) launch();
}

void Game::humanAim(const gs::Pad& pad, bool fire) {
    float vx = 0.f;
    if (pad.down(gs::BTN_LEFT)) vx -= 1.f;
    if (pad.down(gs::BTN_RIGHT)) vx += 1.f;
    if (std::fabs(pad.axisX) > 0.2f) vx = pad.axisX;
    aimX_ = clampf(aimX_ + vx * kAimSpeed, shelfX(0), shelfX(kShelfN - 1));
    bool sw = sweet();
    if (sw && !wasSweet_) {
        int hot = shelfAt(aimX_);
        bool tape = hot >= 0 && kShelf[hot].line >= 0 && !held_[kShelf[hot].line];
        blip(2, tape ? 880.f : 240.f, 0.03f, 0.04f);
    }
    wasSweet_ = sw;
    if (fire) launch();
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    rules_ = audit();
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(2, 0, 4));
    sys.apu.setMaster(0.7f);
    sys.apu.setEcho(0.12f, 0.18f, 0.1f);
    toTitle();
    if (!rules_) {
        over_ = true;
        won_ = false;
        mode_ = Mode::Lose;
        std::snprintf(reason_, sizeof reason_, "RULES");
        std::fprintf(stderr, "s3fairtape rules failed\n");
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
    if (mode_ == Mode::Flight) sys_->apu.noise(0.025f, 1500.f, false);
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

    Mode before = mode_;
    const gs::Pad& pad = sys.pad;
    bool start = pad.pressed(gs::BTN_START);
    bool back = pad.pressed(gs::BTN_MODE);
    bool fire = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_TURBO);
    if (bot_) start = back = fire = false;

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
        if (flightT_ >= kFlightN) stick();
    } else if (mode_ == Mode::Pocket && before == Mode::Pocket) {
        pocketT_ += kDt;
        if (pocketT_ > kPocketWait) continuePlay();
    } else if (mode_ == Mode::Judge && before == Mode::Judge) {
        judgeT_ += kDt;
        if (judgeT_ > kJudgeWait) continuePlay();
    } else if (mode_ == Mode::Leave && before == Mode::Leave) {
        leaveT_ += kDt;
        if (leaveT_ > kLeaveWait) {
            won_ = rules_ && matched() && left_ && drawerScore() == tapeSum() && rings_ >= kTapeN &&
                   rings_ <= kMaxRings;
            over_ = true;
            mode_ = Mode::Over;
            if (!won_) std::snprintf(reason_, sizeof reason_, "DOES NOT MATCH");
        }
    }

    tickAudio(kDt);
    draw();
}

void Game::spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, int fog) {
    if (!sys_ || w < 1.f || h < 1.f || img.w == 0 || img.h == 0) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = img;
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::sprM(const gs::Mipped& m, float cx, float cy, float h, int pal, int fog) {
    if (m.h <= 0 || h < 1.f) return;
    float ih = std::max(4.f, h);
    float iw = ih * float(m.w) / float(m.h);
    spr(m.pick(ih), cx, cy, iw, ih, pal, fog);
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
        if (y < 48) {
            float u = y / 47.f;
            c = gs::rgb4(1, 0, 3 + int((1.f - u) * 3.f));
        } else if (y < 96) {
            float u = (y - 48) / 48.f;
            c = gs::rgb4(3 + int(u * 4.f), 1 + int(u * 2.f), 6 - int(u * 3.f));
        } else if (y < 158) {
            c = gs::rgb4(6, 2, 2);
        } else {
            int g = 2 + ((y / 4) & 1);
            c = gs::rgb4(g + 1, g, 1);
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
    const bool aiming = m == Mode::Aim || m == Mode::Title;
    const bool leaving = m == Mode::Leave || (m == Mode::Over && won_);
    const bool failed = m == Mode::Lose || (m == Mode::Over && !won_);
    const float jx = shake_ > 0 ? std::sin(float(shake_) * 1.7f) * 2.f : 0.f;
    const float showX = title ? shelfX(shelfOfLine(0)) : aimX_;
    const int hot = (m == Mode::Aim) ? shelfAt(aimX_) : (title ? shelfOfLine(0) : -1);

    if (title) spr(art_.logo, 160.f, 18.f, float(art_.logo.w), float(art_.logo.h), PAL_GOLD);
    if (leaving) spr(art_.leave, 160.f, 30.f, float(art_.leave.w), float(art_.leave.h), PAL_GOOD);
    if (failed) spr(art_.stay, 160.f, 30.f, float(art_.stay.w), float(art_.stay.h), PAL_BAD);
    spr(art_.sign, 162.f, 42.f, float(art_.sign.w), float(art_.sign.h), PAL_GOLD);

    if (leaving) {
        for (int i = 0; i < 8; i++) {
            float a = clock_ * 2.2f + float(i) * kPi / 4.f;
            float rad = 28.f + float(i & 1) * 10.f;
            sprM(art_.dot, 160.f + std::cos(a) * rad, 30.f + std::sin(a) * 8.f, 4.f, (i & 1) ? PAL_GOLD : PAL_GOOD);
        }
    }

    if (aiming) {
        for (int i = 0; i < 9; i++) {
            float x = 160.f + float(i - 4) * 12.f;
            float pos = float(i) / 8.f;
            bool on = std::fabs(meter_ - pos) <= 0.07f;
            bool mid = i == 4;
            int pal = mid ? PAL_GOLD : PAL_INK;
            if (on && sweet() && m == Mode::Aim) pal = PAL_GOOD;
            else if (on && m == Mode::Aim) pal = PAL_BAD;
            sprM(art_.dot, x, kMeterY, mid ? 8.f : 5.f, pal);
        }
    }

    auto ringAt = [&](float x, float y, float h, int pal) { sprM(art_.ring, x + jx, y, h, pal); };

    if (m == Mode::Flight) {
        float u = float(flightT_) / float(kFlightN);
        float e = ease(u);
        float x = fromX_ + (landX_ - fromX_) * e;
        float y = fromY_ + (landY_ - fromY_) * e - std::sin(clampf(u, 0.f, 1.f) * kPi) * 16.f;
        ringAt(x, y, 16.f + (1.f - e) * 6.f, PAL_RING);
    } else if (m == Mode::Judge && fate_ != Fate::Firm) {
        ringAt(landX_, landY_, 14.f, PAL_BAD);
    } else if (aiming) {
        int pal = PAL_RING;
        if (hot >= 0 && kShelf[hot].line >= 0) pal = (m == Mode::Aim && sweet()) ? PAL_GOOD : PAL_GOLD;
        else if (hot >= 0) pal = PAL_BAD;
        float bob = (hot >= 0 ? 1.4f : 0.6f) * std::sin(clock_ * 6.f);
        ringAt(showX, kHangY + bob, hot >= 0 ? 18.f : 15.f, pal);
        for (int i = 1; i <= 4; i++) {
            float t = float(i) / 5.f;
            sprM(art_.dot, showX, kRailY + (kHangY - kRailY) * t, 3.f, PAL_RING);
        }
    }

    for (int i = 0; i < kTapeN; i++) {
        if (!held_[i]) continue;
        float x = slotX(i);
        float y = kSlotY;
        if (m == Mode::Pocket && fly_ == i) {
            float e = ease(pocketT_ / kPocketWait);
            x = slipFromX_ + (slotX(i) - slipFromX_) * e;
            y = slipFromY_ + (kSlotY - slipFromY_) * e;
        }
        const gs::Image& slip = art_.slip[i];
        spr(slip, x + jx, y, float(slip.w), float(slip.h), PAL_PAPER);
    }

    for (int i = 0; i < kShelfN; i++) {
        int pal = PAL_INK;
        if (kShelf[i].line >= 0) pal = held_[kShelf[i].line] ? PAL_GOOD : PAL_GOLD;
        else pal = PAL_BAD;
        if (hot == i && m == Mode::Aim) pal = sweet() && kShelf[i].line >= 0 ? PAL_GOOD : pal;
        const gs::Image& tag = art_.tag[i];
        spr(tag, shelfX(i) + jx, kNameY, float(tag.w), float(tag.h), pal);
    }

    for (int i = 0; i < kShelfN; i++) {
        if (!seated_[i]) continue;
        ringAt(shelfX(i), kNeckY, 16.f, PAL_RING);
    }

    for (int i = 0; i < kShelfN; i++) {
        Shape shape = kShelf[i].shape;
        int pal = jarPal(shape);
        const gs::Mipped* top = topper(art_, shape);
        float th = (shape == Shape::Cane || shape == Shape::Stick) ? 18.f : (shape == Shape::Bear ? 16.f : 14.f);
        if (shape == Shape::Cub) th = 12.f;
        sprM(*top, shelfX(i) + jx, kTopY - (th - 14.f) * 0.35f, th, pal);
    }

    for (int i = 0; i < kShelfN; i++) {
        float cy = kBaseY - kJarH * 0.5f;
        sprM(art_.jar, shelfX(i) + jx, cy, kJarH, jarPal(kShelf[i].shape));
        sprM(art_.shade, shelfX(i), kBaseY + 2.f, 6.f, PAL_WOOD);
    }

    float walk = 0.f;
    if (m == Mode::Leave) walk = std::min(leaveT_ / kLeaveWait, 1.f);
    else if (leaving) walk = 1.f;
    float kidX = kKidX - walk * 36.f;
    int step = int((m == Mode::Leave ? leaveT_ * 8.f : clock_ * 2.f)) & 1;
    if (kidX > -8.f) {
        sprM(art_.shade, kidX, kKidFeet + 1.f, 7.f, PAL_WOOD);
        sprM(art_.kid[step], kidX, kKidFeet - 18.f, 36.f, PAL_KID);
    }

    spr(art_.rail, 162.f, kRailY, float(art_.rail.w), float(art_.rail.h), PAL_WOOD);
    spr(art_.shelf.pick(12.f), 162.f, kShelfY, 230.f, 12.f, PAL_WOOD);
    spr(art_.tape, (slotX(0) + slotX(2)) * 0.5f, kTapeY, float(art_.tape.w), float(art_.tape.h), PAL_PAPER);
    spr(art_.drawer, (slotX(0) + slotX(2)) * 0.5f, kDrawerY, float(art_.drawer.w), float(art_.drawer.h), PAL_WOOD);

    spr(art_.cloth.pick(44.f), 162.f, 112.f, 210.f, 44.f, PAL_ROSE);
    spr(art_.awning.pick(28.f), 168.f, 40.f, 250.f, 28.f, PAL_AWN);
    spr(art_.post.pick(110.f), 14.f, 100.f, 14.f, 120.f, PAL_WOOD);
    spr(art_.post.pick(110.f), 306.f, 100.f, 14.f, 120.f, PAL_WOOD);

    for (int i = 0; i < 9; i++) {
        float x = 78.f + float(i) * 22.f;
        bool on = ((i + int(clock_ * 6.f)) % 4) != 0;
        sprM(art_.bulb, x, 64.f + std::sin(clock_ * 2.f + float(i)) * 1.2f, on ? 9.f : 6.f, PAL_BULB);
    }
    const int bunt[3] = {PAL_ROSE, PAL_GOLD, PAL_BLUE};
    for (int i = 0; i < 12; i++) {
        float x = 16.f + float(i) * 26.f;
        float y = 10.f + ((i & 1) ? 2.f : 0.f) + std::sin(clock_ * 1.5f + float(i)) * 1.f;
        sprM(art_.pennant, x, y, 12.f, bunt[i % 3]);
    }
    const int balloons[3] = {PAL_ROSE, PAL_GOLD, PAL_BLUE};
    for (int i = 0; i < 3; i++) {
        float y = 78.f + float(i) * 18.f + std::sin(clock_ * 1.7f + float(i)) * 2.f;
        sprM(art_.balloon, 300.f, y, 16.f, balloons[i]);
    }

    sprM(art_.wheel, 42.f, 28.f, 36.f, PAL_NIGHT, 6);
    for (int i = 0; i < 6; i++) {
        float a = clock_ * 0.5f + float(i) * kPi / 3.f;
        sprM(art_.gondola, 42.f + std::cos(a) * 14.f, 28.f + std::sin(a) * 14.f, 6.f, PAL_NIGHT, 5);
    }
    sprM(art_.moon, 292.f, 16.f, 12.f, PAL_BULB, 2);
    const float stars[5][2] = {{70, 12}, {120, 8}, {200, 14}, {250, 8}, {270, 18}};
    for (int i = 0; i < 5; i++) {
        if ((int(clock_ * 2.f + float(i)) & 3) == 0) continue;
        sprM(art_.star, stars[i][0], stars[i][1], 7.f, PAL_BULB, 2);
    }

    char buf[64];
    if (title) {
        hud(1, 0, "V" S3_VERSION, PAL_INK);
        hudC(24, "THE DRAWER HAS TO MATCH THE TAPE", PAL_GOLD);
        hudC(25, "ROSE  CANE  BEAR DROP THE SLIP", PAL_INK);
        hudC(26, "BUD STICK AND CUB STAY OUT", PAL_BAD);
        if ((int(clock_ * 2.f) & 1) == 0) hudC(27, "PRESS START", PAL_GOLD);
        else hudC(27, "ARROWS AIM    Z OR C TOSSES", PAL_INK);
    } else if (mode_ == Mode::Pause) {
        hudC(12, "PAUSED", PAL_GOLD);
        hudC(13, "START RESUMES", PAL_INK);
        hudC(14, "ESC TO THE TITLE", PAL_INK);
    } else {
        std::snprintf(buf, sizeof buf, "TILL %d", drawerScore());
        hud(1, 0, buf, matched() ? PAL_GOOD : PAL_GOLD);
        std::snprintf(buf, sizeof buf, "RINGS %d", rings_);
        hud(31, 0, buf, PAL_INK);
        hud(1, 1, "ROSE", held_[0] ? PAL_GOOD : PAL_INK);
        hud(8, 1, "CANE", held_[1] ? PAL_GOOD : PAL_INK);
        hud(15, 1, "BEAR", held_[2] ? PAL_GOOD : PAL_INK);

        for (int i = 0; i < kTapeN; i++) {
            std::snprintf(buf, sizeof buf, "%s %d", tapeName(i), tapePay(i));
            int n = int(std::strlen(buf));
            int col = int(slotX(i) / 8.f) - n / 2;
            hud(col, 19, buf, held_[i] ? PAL_GOOD : PAL_GOLD);
        }
        hud(1, 18, "TAPE", PAL_INK);

        if (leaving) {
            hudC(25, "THE DRAWER MATCHES THE TAPE", PAL_GOOD);
            std::snprintf(buf, sizeof buf, "ROSE %d  CANE %d  BEAR %d", tapePay(0), tapePay(1), tapePay(2));
            hudC(26, buf, PAL_GOLD);
            hudC(27, "LEAVE", PAL_GOOD);
        } else if (failed) {
            hudC(25, "THE DRAWER DOES NOT MATCH", PAL_BAD);
            hudC(26, reason_[0] ? reason_ : "STAY", PAL_BAD);
            if (!bot_) hudC(27, "START TRIES AGAIN", PAL_INK);
        } else if (m == Mode::Pocket || m == Mode::Judge) {
            int pal = (m == Mode::Pocket) ? PAL_GOOD : PAL_BAD;
            hudC(26, reason_, pal);
        } else if (m == Mode::Flight) {
            hudC(26, "RING AWAY", PAL_GOLD);
        } else if (m == Mode::Aim) {
            if (hot >= 0 && kShelf[hot].line >= 0) {
                std::snprintf(buf, sizeof buf, "%s %d ON THE TAPE", kShelf[hot].name, kShelf[hot].pay);
                hudC(26, buf, sweet() ? PAL_GOOD : PAL_GOLD);
            } else if (hot >= 0) {
                std::snprintf(buf, sizeof buf, "%s %d STAYS OUT", kShelf[hot].name, kShelf[hot].pay);
                hudC(26, buf, PAL_BAD);
            } else {
                hudC(26, "AIM A FIRM RING", PAL_INK);
            }
            hudC(27, "ARROWS AIM    Z OR C TOSSES", PAL_INK);
        }
    }
}

}  // namespace fairtape

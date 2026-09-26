#include "game/fairchime.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace fairchime {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kSweet = 0.09f;
constexpr float kMeterRate = 1.f;
constexpr int kFlight = 16;
constexpr float kGoldX = 160.f;
constexpr float kSpread = 34.f;
constexpr float kCatch = 10.f;
constexpr float kBottleCy = 138.f;
constexpr float kBottleH = 54.f;
constexpr float kNeckY = kBottleCy - kBottleH * 0.5f + (12.f / 48.f) * kBottleH;
constexpr float kFeet = 188.f;
constexpr float kKidHome = 58.f;

const int kStars[][2] = {{18, 18}, {48, 30}, {78, 14}, {236, 16}, {274, 28}, {304, 12}, {204, 22}, {112, 12}};

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

float bottleX(int i) { return kGoldX + float(i - 2) * kSpread; }

// Float steps, not double. The first green window then lands on 12:00:00.
void stepMeter(float& m, float& dir) {
    m += dir * kMeterRate * kDt;
    if (m >= 1.f) {
        m = 1.f;
        dir = -1.f;
    } else if (m <= 0.f) {
        m = 0.f;
        dir = 1.f;
    }
}

uint16_t mix(uint16_t a, uint16_t b, float t) {
    t = clampf(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

gs::FMPatch bellPatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.18f;
    p.op[0] = {1.f, 1.f, 0.004f, 0.55f, 0.2f, 0.48f};
    p.op[1] = {2.f, 0.26f, 0.005f, 0.34f, 0.f, 0.26f};
    p.op[2] = {3.01f, 0.12f, 0.004f, 0.2f, 0.f, 0.16f};
    p.op[3] = {0.5f, 0.15f, 0.008f, 0.6f, 0.08f, 0.32f};
    p.vol = 0.28f;
    p.echo = 0.28f;
    p.tone = 1600.f;
    return p;
}

}  // namespace

bool Game::sweet() const { return std::fabs(meter_ - 0.5f) <= kSweet; }

int Game::secAt(int frames) const {
    if (frames < 0) frames = 0;
    return kStartSec + frames / kFpc;
}

int Game::clockSec() const { return secAt(playFrames_); }

int Game::framesUntilHour() const {
    int sec = clockSec();
    int sub = playFrames_ % kFpc;
    if (playFrames_ < 0) return 120 * kFpc;
    if (sec > kHourSec) return -((sec - kHourSec) * kFpc + sub);
    if (sec == kHourSec) return -sub;
    int secLeft = kHourSec - sec;
    return (secLeft - 1) * kFpc + (kFpc - sub);
}

bool Game::onHourAt(int frames) const {
    int sec = secAt(frames);
    return sec >= kHourSec && sec < kHourSec + kGraceSec;
}

bool Game::onHour() const { return onHourAt(playFrames_); }

bool Game::pastHour() const { return clockSec() >= kHourSec + kGraceSec; }

void Game::split(int& h, int& m, int& s) const {
    int t = clockSec();
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

Game::Bed Game::classify(float x, float y) const {
    int best = -1;
    float bestD = 1e9f;
    for (int i = 0; i < 5; i++) {
        float d = std::hypot(x - bottleX(i), y - kNeckY);
        if (d < bestD) {
            bestD = d;
            best = i;
        }
    }
    if (best < 0 || bestD > kCatch) return Bed::Miss;
    return best == 2 ? Bed::Gold : Bed::Cream;
}

void Game::spot(float meter, float aim, float& x, float& y, Fate& fate) const {
    float off = std::fabs(meter - 0.5f);
    if (off <= kSweet) {
        fate = Fate::Firm;
        x = aim;
        y = kNeckY;
        return;
    }
    if (meter < 0.5f) {
        fate = Fate::Short;
        x = aim;
        y = kNeckY + 36.f;
        return;
    }
    fate = Fate::Hot;
    float u = clampf((meter - 0.5f - kSweet) / (0.5f - kSweet), 0.f, 1.f);
    x = aim + 22.f + u * 24.f;
    y = kNeckY - 18.f - u * 8.f;
}

bool Game::audit() const {
    auto bad = [](const char* w) {
        std::fprintf(stderr, "s3fairchime %s\n", w);
        return false;
    };
    if (secAt(0) != kStartSec) return bad("clock does not open at 11:58");
    if (secAt(120 * kFpc) != kHourSec) return bad("frame 720 is not noon");
    if (onHourAt(120 * kFpc - 1)) return bad("11:59 counted as the hour");
    if (!onHourAt(120 * kFpc)) return bad("noon missed");
    if (!onHourAt(120 * kFpc + kGraceSec * kFpc - 1)) return bad("grace ended early");
    if (onHourAt(120 * kFpc + kGraceSec * kFpc)) return bad("grace ran long");

    float x, y;
    Fate f;
    spot(0.5f, kGoldX, x, y, f);
    if (f != Fate::Firm || classify(x, y) != Bed::Gold) return bad("gold neck missed");
    spot(0.f, kGoldX, x, y, f);
    if (f != Fate::Short || classify(x, y) != Bed::Miss) return bad("short ring scored");
    spot(1.f, kGoldX, x, y, f);
    if (f != Fate::Hot || classify(x, y) != Bed::Miss) return bad("hot ring scored");
    for (int i = 0; i < 5; i++) {
        spot(0.5f, bottleX(i), x, y, f);
        Bed b = classify(x, y);
        if (i == 2) {
            if (b != Bed::Gold) return bad("centre bottle is not gold");
        } else if (b != Bed::Cream) {
            return bad("cream bottle counted as the hour");
        }
    }
    float gap = (bottleX(1) + bottleX(2)) * 0.5f;
    spot(0.5f, gap, x, y, f);
    if (classify(x, y) != Bed::Miss) return bad("gap between bottles scored");
    if (classify(kGoldX, kNeckY - 30.f) != Bed::Miss) return bad("air above the neck scored");

    float m = 0.f, d = 1.f;
    for (int play = 1; play <= 1200; play++) {
        stepMeter(m, d);
        int stick = play + kFlight;
        if (std::fabs(m - 0.5f) <= kSweet && onHourAt(stick)) {
            if (secAt(stick) != kHourSec) return bad("first chime is not twelve sharp");
            return true;
        }
    }
    return bad("no chime on the hour");
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    rules_ = audit();
    if (!rules_) std::fprintf(stderr, "s3fairchime rules failed\n");
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(2, 1, 3));
    sys.apu.setMaster(0.7f);
    sys.apu.setEcho(0.16f, 0.22f, 0.12f);
    sys.apu.setPatch(0, bellPatch());
    sys.apu.setPan(0, 0.f);
    toTitle();
}

void Game::toTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    clockOn_ = false;
    thrown_ = 0;
    ringN_ = 0;
    playFrames_ = 0;
    titleFrames_ = 0;
    chimeFrames_ = 0;
    leaveT_ = 0;
    failFrames_ = 0;
    strikes_ = 0;
    showT_ = 0;
    lastSec_ = -1;
    reason_ = "";
    bed_[0] = 0;
    last_[0] = 0;
    wasSweet_ = false;
    meter_ = 0.f;
    meterDir_ = 1.f;
    bellAmp_ = 0.22f;
    kidX_ = kKidHome;
    aimX_ = kGoldX;
    for (Ring& r : ring_) r.on = false;
    if (!sys_) return;
    sys_->apu.keyOff(0);
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(1, 0, 0);
    sys_->apu.tone(2, 0, 0);
}

void Game::newGame() {
    won_ = false;
    over_ = false;
    thrown_ = 0;
    ringN_ = 0;
    playFrames_ = 0;
    chimeFrames_ = 0;
    leaveT_ = 0;
    failFrames_ = 0;
    strikes_ = 0;
    showT_ = 0;
    lastSec_ = -1;
    reason_ = "";
    bed_[0] = 0;
    last_[0] = 0;
    wasSweet_ = false;
    meter_ = 0.f;
    meterDir_ = 1.f;
    bellAmp_ = 0.22f;
    clockOn_ = true;
    kidX_ = kKidHome;
    aimX_ = kGoldX;
    for (Ring& r : ring_) r.on = false;
    if (!rules_) {
        beginFail("RULES");
        return;
    }
    beginAim();
    if (sys_) sys_->setLight(90, 48, 28);
}

void Game::beginAim() {
    int w = 0;
    for (int i = 0; i < ringN_; i++) {
        if (ring_[i].on && ring_[i].held) ring_[w++] = ring_[i];
    }
    ringN_ = w;
    showT_ = 0;
    wasSweet_ = false;
    mode_ = Mode::Aim;
}

void Game::remember(float x, float y, bool held) {
    if (ringN_ >= 3) {
        for (int i = 1; i < 3; i++) ring_[i - 1] = ring_[i];
        ringN_ = 2;
    }
    ring_[ringN_].x = x;
    ring_[ringN_].y = y;
    ring_[ringN_].on = true;
    ring_[ringN_].held = held;
    ringN_++;
}

void Game::launch() {
    if (mode_ != Mode::Aim || !rules_ || !clockOn_ || thrown_ >= 3) return;
    spot(meter_, aimX_, landX_, landY_, fate_);
    fromX_ = kidX_ + 16.f;
    fromY_ = kFeet - 42.f;
    flightT_ = 0;
    thrown_++;
    mode_ = Mode::Flight;
    if (sys_) {
        sys_->apu.noiseBurst(0.16f, 1700.f, 0.04f);
        sys_->rumble(0.1f, 0.18f, 30);
    }
}

void Game::beginChime() {
    if (won_) return;
    won_ = true;
    reason_ = "CHIME";
    std::snprintf(bed_, sizeof bed_, "GOLD");
    clockOn_ = false;
    mode_ = Mode::Chime;
    chimeFrames_ = 0;
    strikes_ = 0;
    bellAmp_ = 1.f;
    if (!sys_) return;
    sys_->setLight(255, 196, 64);
    sys_->rumble(0.4f, 0.8f, 180);
    strikeBell();
    strikes_ = 1;
}

void Game::beginEarly() {
    std::snprintf(last_, sizeof last_, "EARLY");
    mode_ = Mode::Early;
    showT_ = 0;
    blip(0, 150.f, 0.06f, 0.16f);
    if (sys_) sys_->setLight(160, 80, 28);
}

void Game::beginDead() {
    mode_ = Mode::Dead;
    showT_ = 0;
    blip(0, 110.f, 0.06f, 0.14f);
    if (sys_) sys_->setLight(120, 40, 28);
}

void Game::beginFail(const char* why) {
    reason_ = why;
    won_ = false;
    clockOn_ = false;
    mode_ = Mode::Fail;
    failFrames_ = 0;
    blip(0, 82.f, 0.08f, 0.26f);
    if (sys_) {
        sys_->apu.tone(1, 55.f, 0.05f);
        tickT_ = 0.28f;
        sys_->setLight(140, 24, 24);
    }
}

void Game::stick() {
    if (mode_ != Mode::Flight) return;
    Bed bed = Bed::Miss;
    if (fate_ == Fate::Firm) bed = classify(landX_, landY_);
    if (fate_ == Fate::Short) std::snprintf(last_, sizeof last_, "SHORT");
    else if (fate_ == Fate::Hot) std::snprintf(last_, sizeof last_, "HOT");
    else if (bed == Bed::Gold) std::snprintf(last_, sizeof last_, "GOLD");
    else if (bed == Bed::Cream) std::snprintf(last_, sizeof last_, "CREAM");
    else std::snprintf(last_, sizeof last_, "WIDE");

    bool gold = fate_ == Fate::Firm && rules_ && bed == Bed::Gold;
    if (gold && onHour()) {
        remember(landX_, landY_, true);
        if (sys_) sys_->apu.noiseBurst(0.2f, 260.f, 0.08f);
        beginChime();
        return;
    }
    if (gold && !pastHour()) {
        remember(landX_, landY_, false);
        if (sys_) sys_->apu.noiseBurst(0.12f, 180.f, 0.05f);
        if (thrown_ >= 3) beginFail("EARLY");
        else beginEarly();
        return;
    }
    remember(landX_, landY_, bed == Bed::Cream);
    if (sys_) sys_->apu.noiseBurst(0.08f, 150.f, 0.04f);
    if (pastHour()) beginFail("LATE");
    else if (thrown_ >= 3) beginFail("SPENT");
    else beginDead();
}

void Game::botAim() {
    float dx = kGoldX - aimX_;
    if (std::fabs(dx) > 0.4f) {
        aimX_ += std::copysign(std::min(6.f, std::fabs(dx)), dx);
        return;
    }
    aimX_ = kGoldX;
    if (classify(aimX_, kNeckY) != Bed::Gold) return;
    if (pastHour()) {
        beginFail("LATE");
        return;
    }
    float m = meter_;
    float d = meterDir_;
    for (int f = 0; f < 960; f++) {
        int stickAt = playFrames_ + f + kFlight;
        if (std::fabs(m - 0.5f) <= kSweet && onHourAt(stickAt)) {
            if (f == 0) launch();
            return;
        }
        if (secAt(stickAt) >= kHourSec + kGraceSec) {
            if (f == 0) beginFail("LATE");
            return;
        }
        stepMeter(m, d);
    }
}

void Game::humanAim(const gs::Pad& p, bool fire) {
    float mx = 0.f;
    if (p.down(gs::BTN_LEFT)) mx -= 1.f;
    if (p.down(gs::BTN_RIGHT)) mx += 1.f;
    if (std::fabs(p.axisX) > 0.2f) mx = p.axisX;
    aimX_ = clampf(aimX_ + mx * 2.8f, 78.f, 242.f);
    bool sw = sweet();
    if (sw && !wasSweet_) blip(2, 988.f, 0.04f, 0.04f);
    wasSweet_ = sw;
    if (fire) launch();
}

void Game::blip(int ch, float freq, float vol, float hold) {
    if (!sys_) return;
    sys_->apu.tone(ch, freq, vol);
    if (ch == 0) toneT_ = hold;
    else tickT_ = hold;
}

void Game::strikeBell() {
    if (!sys_) return;
    sys_->apu.keyOn(0, 392.f, 0.32f);
    sys_->apu.tone(1, 784.f, 0.03f);
    toneT_ = 0.12f;
    bellAmp_ = 1.f;
}

void Game::tickClock() {
    if (!clockOn_) return;
    if (mode_ != Mode::Aim && mode_ != Mode::Flight && mode_ != Mode::Early && mode_ != Mode::Dead) return;
    int sec = clockSec();
    if (sec == lastSec_) return;
    int prev = lastSec_;
    lastSec_ = sec;
    if (prev < 0) return;
    int until = framesUntilHour();
    if (until > 0 && until <= kFpc * 12) blip(0, (sec & 1) ? 880.f : 698.f, 0.04f, 0.04f);
    else if ((sec % 10) == 0) blip(0, 196.f, 0.03f, 0.04f);
}

void Game::tickAudio(float dt) {
    if (!sys_) return;
    if (toneT_ > 0.f) {
        toneT_ -= dt;
        if (toneT_ <= 0.f) {
            sys_->apu.tone(0, 0, 0);
            sys_->apu.tone(1, 0, 0);
            sys_->apu.keyOff(0);
        }
    }
    if (tickT_ > 0.f) {
        tickT_ -= dt;
        if (tickT_ <= 0.f) sys_->apu.tone(2, 0, 0);
    }
    if (mode_ == Mode::Flight) sys_->apu.noise(0.03f, 1400.f, true);
    else sys_->apu.noise(0, 1000);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    anim_ += kDt;
    bool chiming = mode_ == Mode::Chime || mode_ == Mode::Leave || (mode_ == Mode::Over && won_);
    bellPh_ += kDt * (chiming ? 14.f : 2.2f);
    bellAmp_ = chiming ? 1.f : 0.22f;

    const gs::Pad& p = sys.pad;
    bool start = p.pressed(gs::BTN_START);
    bool back = p.pressed(gs::BTN_MODE);
    bool fire = p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C) || p.pressed(gs::BTN_TURBO);
    if (bot_) {
        start = false;
        back = false;
        fire = false;
    }

    if (mode_ == Mode::Pause) {
        if (start || fire) mode_ = held_;
        else if (back) toTitle();
    } else if (mode_ == Mode::Title) {
        titleFrames_++;
        if (bot_ && titleFrames_ >= 30) newGame();
        else if (back) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        } else if (start || fire) newGame();
    } else if (mode_ == Mode::Aim) {
        if (!bot_ && start) {
            held_ = Mode::Aim;
            mode_ = Mode::Pause;
        } else if (!bot_ && back) toTitle();
        else {
            if (clockOn_) playFrames_++;
            stepMeter(meter_, meterDir_);
            if (pastHour()) beginFail("LATE");
            else if (bot_) botAim();
            else humanAim(p, fire);
        }
    } else if (mode_ == Mode::Flight) {
        if (clockOn_) playFrames_++;
        flightT_++;
        if (flightT_ >= kFlight) stick();
    } else if (mode_ == Mode::Early || mode_ == Mode::Dead) {
        if (clockOn_) playFrames_++;
        showT_++;
        if (showT_ > 36) {
            if (pastHour()) beginFail("LATE");
            else if (thrown_ >= 3) beginFail(mode_ == Mode::Early ? "EARLY" : "SPENT");
            else beginAim();
        }
    } else if (mode_ == Mode::Chime) {
        chimeFrames_++;
        if (chimeFrames_ > 0 && (chimeFrames_ % 6) == 0 && strikes_ < 12) {
            strikeBell();
            strikes_++;
        }
        if (strikes_ >= 12 && chimeFrames_ > 12 * 6 + 28) {
            mode_ = Mode::Leave;
            leaveT_ = 0;
            reason_ = "CHIME";
        }
    } else if (mode_ == Mode::Leave) {
        leaveT_++;
        kidX_ = std::min(250.f, kKidHome + leaveT_ * 3.9f);
        if (leaveT_ > 48) {
            mode_ = Mode::Over;
            won_ = true;
            over_ = true;
            sys.apu.keyOff(0);
        }
    } else if (mode_ == Mode::Fail) {
        if (++failFrames_ > 70) over_ = true;
        else if (!bot_ && (start || fire)) newGame();
    } else if (mode_ == Mode::Over) {
        if (!bot_ && back) toTitle();
        else if (!bot_ && (start || fire)) newGame();
    }

    tickClock();
    tickAudio(kDt);
    int until = framesUntilHour();
    if (mode_ == Mode::Aim || mode_ == Mode::Flight) {
        if (onHour()) sys.setLight(240, 180, 48);
        else if (until > 0 && until < kFpc * 12) sys.setLight(170, 120, 36);
        else sys.setLight(70, 40, 28);
    }
    draw();
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet, int fog, bool shadow) {
    if (!sys_ || h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    if (s.w < 1 || s.h < 1) return;
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::sprI(const gs::Image& img, float cx, float cy, float w, float h, int pal) {
    if (!sys_ || img.w < 1 || img.h < 1 || w < 1.f || h < 1.f) return;
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    if (s.w < 1 || s.h < 1) return;
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
    bool win = mode_ == Mode::Chime || mode_ == Mode::Leave || (mode_ == Mode::Over && won_);
    bool dead = mode_ == Mode::Fail || (mode_ == Mode::Over && !won_ && reason_[0]);
    uint16_t top = win ? gs::rgb4(6, 3, 2) : dead ? gs::rgb4(3, 1, 2) : gs::rgb4(1, 1, 4);
    uint16_t mid = win ? gs::rgb4(10, 6, 3) : dead ? gs::rgb4(6, 2, 3) : gs::rgb4(4, 2, 6);
    uint16_t horizon = win ? gs::rgb4(14, 9, 3) : dead ? gs::rgb4(8, 3, 2) : gs::rgb4(10, 5, 3);
    uint16_t walk = win ? gs::rgb4(8, 5, 2) : gs::rgb4(6, 3, 2);
    uint16_t dirt = gs::rgb4(3, 2, 1);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.road[y].on = false;
        v.lineFog[y] = 0;
        float u = y / float(gs::SCREEN_H - 1);
        uint16_t c;
        if (u < 0.28f) c = mix(top, mid, u / 0.28f);
        else if (u < 0.62f) c = mix(mid, horizon, (u - 0.28f) / 0.34f);
        else if (u < 0.78f) c = mix(horizon, walk, (u - 0.62f) / 0.16f);
        else c = mix(walk, dirt, (u - 0.78f) / 0.22f);
        if (y >= 176 && ((y / 3) & 1)) c = mix(c, gs::rgb4(2, 1, 1), 0.28f);
        v.lineBackdrop[y] = c;
    }
}

void Game::clockAt(float cx, float cy, float size) {
    int hs, ms, ss;
    if (mode_ == Mode::Title || (mode_ == Mode::Pause && held_ == Mode::Title)) {
        hs = 55;
        ms = 58;
        ss = (titleFrames_ / 8) % 60;
    } else {
        int h, m, s;
        split(h, m, s);
        hs = ((h % 12) * 5 + m / 12) % 60;
        ms = m % 60;
        ss = s % 60;
    }
    bool noon = onHour() || mode_ == Mode::Chime || mode_ == Mode::Leave || (mode_ == Mode::Over && won_);
    sprI(art_.hand[2][ss], cx, cy, size, size, PAL_HAND);
    sprI(art_.hand[0][hs], cx, cy, size, size, PAL_HAND);
    sprI(art_.hand[1][ms], cx, cy, size, size, PAL_HAND);
    sprI(art_.cap, cx, cy, 8, 8, PAL_HAND);
    sprI(art_.face, cx, cy, size, size, PAL_CLOCK);
    if (noon) sprI(art_.halo, cx, cy, size + 14.f, size + 14.f, PAL_BRASS);
}

void Game::meterRow(float y) {
    float m = meter_;
    if (mode_ == Mode::Title || (mode_ == Mode::Pause && held_ == Mode::Title)) {
        float u = std::fmod(titleFrames_ * kDt, 2.f);
        m = u < 1.f ? u : 2.f - u;
    }
    bool green = std::fabs(m - 0.5f) <= kSweet;
    for (int i = 0; i < 9; i++) {
        float x = kGoldX + float(i - 4) * 12.f;
        bool mid = i == 4;
        bool on = std::fabs(m * 8.f - float(i)) <= 0.55f;
        int pal = mid ? PAL_WORD : PAL_INK;
        if (on) pal = (mid && green) ? PAL_GREEN : PAL_ALERT;
        sprI(art_.pip, x, y, mid ? 9.f : 7.f, mid ? 9.f : 7.f, pal);
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

    bool chiming = mode_ == Mode::Chime || mode_ == Mode::Leave || (mode_ == Mode::Over && won_);
    bool showAim = mode_ == Mode::Title || mode_ == Mode::Aim || (mode_ == Mode::Pause && held_ == Mode::Aim);
    bool aiming = showAim;
    auto stamp = [&](const gs::Mipped& m, float cx, float cy, float w, float h, int pal, int fog = 0, bool shadow = false) {
        if (w < 1.f || h < 1.f || m.h < 1) return;
        gs::Sprite s;
        s.w = int16_t(std::lround(w));
        s.h = int16_t(std::lround(h));
        s.x = int16_t(std::lround(cx - s.w * 0.5f));
        s.y = int16_t(std::lround(cy - s.h * 0.5f));
        s.img = m.pick(std::max(w, h));
        s.pal = uint8_t(pal);
        s.fog = uint8_t(std::clamp(fog, 0, 16));
        s.shadow = shadow;
        v.sprite(s);
    };

    if (chiming) {
        for (int i = 0; i < 8; i++) {
            float a = bellPh_ * 1.3f + float(i) * 0.8f;
            float rad = 18.f + float(i % 4) * 7.f;
            spr(art_.burst, kGoldX + std::cos(a) * rad, 62.f + std::sin(a) * rad * 0.55f, 8.f + float(i % 3), PAL_BULB,
                false, false, 0);
        }
    }

    if (mode_ == Mode::Flight) {
        float u = clampf(float(flightT_) / float(kFlight), 0.f, 1.f);
        float e = u * u * (3.f - 2.f * u);
        float x = fromX_ + (landX_ - fromX_) * e;
        float y = fromY_ + (landY_ - fromY_) * e;
        float lift = std::sin(u * kPi) * (fate_ == Fate::Short ? 28.f : 16.f);
        spr(art_.ring, x, y - lift, 16.f - 6.f * e, PAL_BRASS);
    }
    for (int i = ringN_ - 1; i >= 0; --i) {
        if (!ring_[i].on) continue;
        float y = ring_[i].y;
        if (!ring_[i].held && mode_ == Mode::Early) y += float(showT_) * 1.6f;
        spr(art_.ring, ring_[i].x, y, ring_[i].held ? 14.f : 15.f, PAL_BRASS);
    }

    if (showAim) {
        Bed live = classify(aimX_, kNeckY);
        int pal = PAL_INK;
        if (live == Bed::Gold && sweet() && mode_ != Mode::Title) pal = PAL_GREEN;
        else if (live == Bed::Gold) pal = PAL_WORD;
        else pal = PAL_ALERT;
        float bob = mode_ == Mode::Title ? std::sin(anim_ * 3.f) : 0.f;
        sprI(art_.chevron, aimX_, kNeckY - 16.f + bob, 11, 8, pal);
    }

    int peg = 3 - thrown_;
    if (peg < 0) peg = 0;
    if (aiming && peg > 0) {
        spr(art_.ring, kidX_ + 16.f, kFeet - 40.f, 15.f, PAL_BRASS);
        peg--;
    }
    for (int i = 0; i < peg; i++) spr(art_.ring, 16.f, 156.f + float(i) * 12.f, 12.f, PAL_BRASS);

    float pulse = 8.f + std::sin(anim_ * 5.f) * ((onHour() || chiming) ? 2.2f : 0.7f);
    spr(art_.bulb, kGoldX, kNeckY - 30.f, pulse, (onHour() || chiming) ? PAL_BULB : PAL_WORD);
    sprI(art_.twelve, kGoldX, kNeckY - 20.f, float(art_.twelve.w), float(art_.twelve.h), PAL_WORD);

    bool stepping = mode_ == Mode::Title || mode_ == Mode::Leave || chiming;
    int pose = stepping && (int(anim_ * 8.f) & 1) ? 1 : 0;
    float kidBob = mode_ == Mode::Title ? std::sin(anim_ * 3.f) * 1.1f : 0.f;
    spr(art_.kid[pose], kidX_, kFeet + kidBob, 70.f, PAL_KID, false, true);

    spr(art_.bottle, kGoldX, kBottleCy, kBottleH, PAL_BRASS, false, false, 0);
    for (int i = 0; i < 5; i++) {
        if (i == 2) continue;
        spr(art_.bottle, bottleX(i), kBottleCy, kBottleH, PAL_CREAM, false, false, 1);
    }
    stamp(art_.counter, kGoldX, 172.f, 210.f, 16.f, PAL_WOOD);

    meterRow(188.f);

    float swing = std::sin(bellPh_) * 8.f * bellAmp_;
    spr(art_.bell, kGoldX - 38.f + swing, 50.f, 18.f, PAL_BRASS);
    spr(art_.bell, kGoldX + 38.f - swing, 50.f, 18.f, PAL_BRASS, true);
    clockAt(kGoldX, 64.f, 54.f);

    stamp(art_.awning, kGoldX, 100.f, 230.f, 20.f, PAL_AWN);
    stamp(art_.board, kGoldX, 136.f, 214.f, 78.f, PAL_WOOD, 1);

    if (mode_ == Mode::Title) sprI(art_.title, kGoldX, 16.f, float(art_.title.w), float(art_.title.h), PAL_WORD);

    for (int i = 0; i < 6; i++) {
        float x = (i < 3) ? 18.f + float(i) * 22.f : 236.f + float(i - 3) * 22.f;
        int blink = (int(anim_ * 7.f) + i) & 1;
        spr(art_.bulb, x, 30.f, blink ? 8.f : 6.f, blink ? PAL_BULB : PAL_WORD);
    }
    const int pennantPal[] = {PAL_RED, PAL_AWN, PAL_BLUE, PAL_WORD, PAL_PINK, PAL_RED};
    for (int i = 0; i < 6; i++) {
        float x = (i < 3) ? 10.f + float(i) * 16.f : 262.f + float(i - 3) * 16.f;
        spr(art_.pennant, x, 42.f, 16.f, pennantPal[i]);
    }
    spr(art_.balloon, 24.f, 58.f + std::sin(anim_ * 2.f) * 2.f, 26.f, PAL_RED, false, false, 1);
    spr(art_.balloon, 300.f, 64.f + std::sin(anim_ * 2.2f + 1.f) * 2.f, 22.f, PAL_BLUE, false, false, 2);
    spr(art_.balloon, 46.f, 40.f + std::sin(anim_ * 1.7f + 2.f) * 1.5f, 18.f, PAL_PINK, false, false, 2);

    bool open = chiming;
    spr(art_.gate, 296.f, kFeet, 96.f, PAL_WOOD, false, true, open ? 0 : 3);
    spr(art_.bulb, 296.f, 118.f, open ? 12.f : 7.f, open ? PAL_BULB : PAL_WORD, false, false, open ? 0 : 2);

    for (int i = 0; i < 6; i++) {
        float a = anim_ * 0.65f + float(i) * (kPi / 3.f);
        float cx = 34.f + std::cos(a) * 28.f;
        float cy = 92.f + std::sin(a) * 18.f;
        spr(art_.car, cx, cy, 11.f, (i & 1) ? PAL_RED : PAL_BLUE, false, false, 6);
    }
    spr(art_.hub, 34.f, 92.f, 18.f, PAL_BRASS, false, false, 5);
    spr(art_.stand, 34.f, 176.f, 78.f, PAL_NIGHT, false, true, 7);

    spr(art_.moon, 286.f, 18.f, 16.f, PAL_CREAM, false, false, 1);
    for (const auto& st : kStars) spr(art_.star, float(st[0]), float(st[1]), 6.f, PAL_BULB, false, false, 2);

    stamp(art_.shadow, kidX_, kFeet + 2.f, 36.f, 8.f, PAL_INK, 0, true);
    stamp(art_.shadow, kGoldX, 176.f, 180.f, 10.f, PAL_INK, 0, true);

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(24, "RING THE GOLD BOTTLE", PAL_WORD);
        hudC(25, "AS THE HOUR CHIMES", PAL_INK);
        hudC(26, "THEN LEAVE", PAL_GREEN);
        if ((titleFrames_ / 30) & 1) hudC(27, "PRESS START", PAL_WORD);
        else hudC(27, "ARROWS AIM   Z ON GREEN", PAL_GREEN);
        return;
    }
    if (mode_ == Mode::Pause) {
        hudC(24, "PAUSED", PAL_WORD);
        hudC(27, "START RESUME   ESC TITLE", PAL_INK);
        return;
    }

    int h, m, s;
    split(h, m, s);
    std::snprintf(buf, sizeof buf, "%d:%02d:%02d", h, m, s);
    bool hot = onHour() || chiming;
    hud(1, 0, buf, hot ? PAL_WORD : PAL_INK);
    int shown = thrown_;
    if (mode_ == Mode::Aim) shown = thrown_ + 1;
    if (shown < 1) shown = 1;
    if (shown > 3) shown = 3;
    std::snprintf(buf, sizeof buf, "RING %d/3", shown);
    hud(31, 0, buf, thrown_ >= 2 ? PAL_ALERT : PAL_WORD);

    if (chiming || (mode_ == Mode::Over && won_)) {
        hud(1, 1, "ON THE HOUR", PAL_WORD);
        hudC(24, mode_ == Mode::Leave || mode_ == Mode::Over ? "LEAVE" : "THE HOUR CHIMES", PAL_WORD);
        hudC(25, "GOLD BOTTLE", PAL_INK);
        if (mode_ == Mode::Over && !bot_) hudC(27, "START AGAIN", PAL_WORD);
        return;
    }
    if (mode_ == Mode::Fail || (mode_ == Mode::Over && !won_)) {
        bool late = std::strcmp(reason_, "LATE") == 0 || pastHour();
        hud(1, 1, "TOO LATE", PAL_ALERT);
        hudC(24, late ? "THE HOUR PASSED" : "NO CHIME", PAL_ALERT);
        hudC(25, reason_[0] ? reason_ : "OPEN", PAL_INK);
        if (!bot_) hudC(27, "START AGAIN", PAL_WORD);
        return;
    }
    if (mode_ == Mode::Early) {
        hudC(24, "EARLY", PAL_ALERT);
        hudC(25, "THE HOUR HAS NOT CHIMED", PAL_WORD);
        hudC(26, thrown_ >= 2 ? "ONE RING LEFT" : "RING FALLS OFF", PAL_INK);
        return;
    }
    if (mode_ == Mode::Dead) {
        hudC(24, last_[0] ? last_ : "WIDE", PAL_ALERT);
        hudC(25, "NOT THE HOUR", PAL_INK);
        hudC(26, thrown_ >= 2 ? "ONE RING LEFT" : "STILL SHORT OF TWELVE", PAL_WORD);
        return;
    }

    int until = framesUntilHour();
    if (until > 0) {
        int show = (until + kFpc - 1) / kFpc;
        std::snprintf(buf, sizeof buf, "HOUR IN %d", show);
        hud(1, 1, buf, until <= kFpc * 12 ? PAL_WORD : PAL_INK);
    } else if (onHour()) hud(1, 1, "ON THE HOUR", PAL_WORD);
    else hud(1, 1, "TOO LATE", PAL_ALERT);

    if (mode_ == Mode::Flight) {
        hudC(26, "IN THE AIR", PAL_WORD);
        return;
    }

    Bed live = classify(aimX_, kNeckY);
    if (live == Bed::Gold) hudC(26, "GOLD", sweet() ? PAL_GREEN : PAL_WORD);
    else if (live == Bed::Cream) hudC(26, "CREAM", PAL_ALERT);
    else hudC(26, "WIDE", PAL_ALERT);

    if (live == Bed::Gold && onHour() && sweet()) hudC(27, "THROW", PAL_GREEN);
    else if (onHour()) hudC(27, sweet() ? "NOT THE GOLD" : "WAIT FOR GREEN", sweet() ? PAL_ALERT : PAL_WORD);
    else if (live == Bed::Gold) hudC(27, sweet() ? "HOLD FOR THE HOUR" : "WAIT FOR GREEN", PAL_WORD);
    else hudC(27, "FIND THE GOLD", PAL_INK);
}

}  // namespace fairchime

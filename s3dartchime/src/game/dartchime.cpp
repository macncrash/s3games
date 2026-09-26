#include "game/dartchime.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace dartchime {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kSweet = 0.09f;
constexpr float kMeterRate = 1.f;
constexpr float kAim = 2.4f;
constexpr int kFlight = 16;
constexpr int kHourFrame = 120 * kFpc;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

uint16_t mix(uint16_t a, uint16_t b, float t) {
    t = clampf(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

gs::FMPatch bellPatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.22f;
    p.op[0] = {1.0f, 1.0f, 0.003f, 0.62f, 0.18f, 0.55f};
    p.op[1] = {2.01f, 0.34f, 0.004f, 0.4f, 0.0f, 0.32f};
    p.op[2] = {3.0f, 0.16f, 0.003f, 0.22f, 0.0f, 0.2f};
    p.op[3] = {0.5f, 0.2f, 0.006f, 0.7f, 0.1f, 0.4f};
    p.vol = 0.3f;
    p.echo = 0.34f;
    p.tone = 1400.f;
    return p;
}

}  // namespace

bool Game::sweet() const { return std::fabs(meter_ - 0.5f) <= kSweet; }

void Game::stepMeter(float& m, float& dir) const {
    m += dir * kMeterRate * kDt;
    if (m >= 1.f) {
        m = 1.f;
        dir = -1.f;
    } else if (m <= 0.f) {
        m = 0.f;
        dir = 1.f;
    }
}

int Game::secAt(int frames) const {
    if (frames < 0) frames = 0;
    return kStartSec + frames / kFpc;
}

int Game::clockSec() const { return secAt(playFrames_); }

int Game::framesUntilHour() const {
    int sec = clockSec();
    int sub = playFrames_ % kFpc;
    if (playFrames_ < 0) return kHourFrame;
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

bool Game::audit() {
    auto check = [&](int seg, float rad, Bed bed, int mul, const char* name) {
        float x, y;
        bedPoint(seg, rad, x, y);
        Mark m = classify(x, y);
        char got[12];
        markName(m, got, int(sizeof got));
        if (m.bed != bed || m.mul != mul || std::strcmp(got, name) != 0) {
            std::fprintf(stderr, "s3dartchime bed %s scored %s +%d at %.2f %.2f\n", name, got, m.mul, x, y);
            return false;
        }
        return true;
    };
    if (classify(kCx, kCy).bed != Bed::Bull) {
        std::fprintf(stderr, "s3dartchime centre is not the bull\n");
        return false;
    }
    float ox, oy;
    bedPoint(0, (kBullIn + kBullOut) * 0.5f, ox, oy);
    if (classify(ox, oy).bed != Bed::Outer) {
        std::fprintf(stderr, "s3dartchime outer bull missed\n");
        return false;
    }
    for (int s = 0; s < 20; s++) {
        int n = kSeg[s];
        char nm[8];
        std::snprintf(nm, sizeof nm, "S%d", n);
        if (!check(s, (kBullOut + kTripIn) * 0.5f, Bed::Single, 1, nm)) return false;
        if (!check(s, (kTripOut + kDoubIn) * 0.5f, Bed::Single, 1, nm)) return false;
        std::snprintf(nm, sizeof nm, "T%d", n);
        if (!check(s, (kTripIn + kTripOut) * 0.5f, Bed::Triple, 3, nm)) return false;
        std::snprintf(nm, sizeof nm, "D%d", n);
        if (!check(s, (kDoubIn + kDoubOut) * 0.5f, Bed::Double, 2, nm)) return false;
    }
    float hx, hy;
    bedPoint(kHourSeg, kHourRad, hx, hy);
    Mark hour = classify(hx, hy);
    if (!hourBed(hour)) {
        std::fprintf(stderr, "s3dartchime hour point is not D12\n");
        return false;
    }
    Mark trip = classify(hx, hy);
    bedPoint(kHourSeg, (kTripIn + kTripOut) * 0.5f, hx, hy);
    trip = classify(hx, hy);
    if (hourBed(trip) || trip.bed != Bed::Triple || trip.number != 12) {
        std::fprintf(stderr, "s3dartchime treble twelve counted as the hour\n");
        return false;
    }
    if (classify(kCx, kCy - (kDoubOut + 3.f)).bed != Bed::Miss) {
        std::fprintf(stderr, "s3dartchime wood scored\n");
        return false;
    }
    if (classify(kCx, kCy - (kRim + 6.f)).bed != Bed::Miss) return false;
    return true;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    rules_ = audit();
    if (!rules_) std::fprintf(stderr, "s3dartchime rules failed\n");
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(2, 1, 2));
    sys.apu.setMaster(0.72f);
    sys.apu.setEcho(0.18f, 0.24f, 0.14f);
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
    pinN_ = 0;
    playFrames_ = 0;
    titleFrames_ = 0;
    chimeFrames_ = 0;
    strikes_ = 0;
    lastSec_ = -1;
    reason_ = "";
    bed_[0] = 0;
    last_[0] = 0;
    wasSweet_ = false;
    meter_ = 0;
    meterDir_ = 1.f;
    bellAmp_ = 0.22f;
    for (Pin& p : pin_) p.on = false;
    bedPoint(kHourSeg, kHourRad, aimX_, aimY_);
    sys_->apu.keyOff(0);
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(1, 0, 0);
    sys_->apu.tone(2, 0, 0);
}

void Game::newGame() {
    won_ = false;
    over_ = false;
    thrown_ = 0;
    pinN_ = 0;
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
    for (Pin& p : pin_) p.on = false;
    bedPoint(kHourSeg, kHourRad, aimX_, aimY_);
    beginAim();
    if (sys_) sys_->setLight(80, 48, 28);
}

void Game::beginAim() {
    int w = 0;
    for (int i = 0; i < pinN_; i++) {
        if (pin_[i].on && pin_[i].held) pin_[w++] = pin_[i];
    }
    pinN_ = w;
    showT_ = 0;
    wasSweet_ = false;
    mode_ = Mode::Aim;
}

void Game::remember(float x, float y, bool held) {
    if (pinN_ >= 3) {
        for (int i = 1; i < 3; i++) pin_[i - 1] = pin_[i];
        pinN_ = 2;
    }
    pin_[pinN_].x = x;
    pin_[pinN_].y = y;
    pin_[pinN_].on = true;
    pin_[pinN_].held = held;
    pinN_++;
}

void Game::moveAim(float mx, float my) {
    float m = std::hypot(mx, my);
    if (m > 0.f) {
        float scale = kAim * (m > 1.f ? 1.f / m : 1.f);
        aimX_ += mx * scale;
        aimY_ += my * scale;
    }
    float lim = kRim + 10.f;
    aimX_ = clampf(aimX_, kCx - lim, kCx + lim);
    aimY_ = clampf(aimY_, kCy - lim, kCy + lim);
}

void Game::launch() {
    if (mode_ != Mode::Aim || !rules_ || !clockOn_) return;
    float off = std::fabs(meter_ - 0.5f);
    fromX_ = 168.f;
    fromY_ = float(gs::SCREEN_H) + 16.f;
    if (off <= kSweet) {
        fate_ = Fate::True;
        landX_ = aimX_;
        landY_ = aimY_;
    } else if (meter_ < 0.5f) {
        fate_ = Fate::Short;
        float u = clampf((0.5f - kSweet - meter_) / (0.5f - kSweet), 0.f, 1.f);
        landX_ = kCx + (aimX_ - kCx) * 0.22f;
        landY_ = kCy + kRim + 12.f + u * 6.f;
    } else {
        fate_ = Fate::Hot;
        float u = clampf((meter_ - 0.5f - kSweet) / (0.5f - kSweet), 0.f, 1.f);
        float dx = aimX_ - kCx;
        float dy = aimY_ - kCy;
        float d = std::hypot(dx, dy);
        float dirx = 0.f, diry = -1.f;
        if (d > 0.5f) {
            dirx = dx / d;
            diry = dy / d;
        }
        float push = 9.f + u * u * 26.f;
        landX_ = aimX_ + dirx * push;
        landY_ = aimY_ + diry * push;
    }
    flightT_ = 0;
    expectStick_ = playFrames_ + kFlight;
    thrown_++;
    mode_ = Mode::Flight;
    if (sys_) {
        sys_->apu.noiseBurst(0.18f, 1900.f, 0.045f);
        sys_->rumble(0.12f, 0.2f, 30);
    }
}

void Game::beginChime() {
    if (won_) return;
    won_ = true;
    reason_ = "CHIME";
    std::snprintf(bed_, sizeof bed_, "D12");
    clockOn_ = false;
    mode_ = Mode::Chime;
    chimeFrames_ = 0;
    strikes_ = 0;
    bellAmp_ = 1.f;
    if (!sys_) return;
    sys_->setLight(255, 196, 48);
    sys_->rumble(0.45f, 0.85f, 200);
    strikeBell();
    strikes_ = 1;
}

void Game::beginEarly() {
    reason_ = "EARLY";
    std::snprintf(last_, sizeof last_, "EARLY");
    mode_ = Mode::Early;
    showT_ = 0;
    blip(0, 140.f, 0.07f, 0.18f);
    if (sys_) sys_->setLight(150, 70, 28);
}

void Game::beginDead() {
    reason_ = "WIDE";
    mode_ = Mode::Dead;
    showT_ = 0;
    blip(0, 110.f, 0.06f, 0.16f);
    if (sys_) sys_->setLight(120, 40, 28);
}

void Game::beginFail(const char* why) {
    reason_ = why;
    won_ = false;
    clockOn_ = false;
    mode_ = Mode::Fail;
    failFrames_ = 0;
    blip(0, 82.f, 0.08f, 0.28f);
    if (sys_) {
        sys_->apu.tone(1, 55.f, 0.05f);
        tickT_ = 0.3f;
        sys_->setLight(140, 24, 24);
    }
}

void Game::stick() {
    if (mode_ != Mode::Flight) return;
    float r = std::hypot(landX_ - kCx, landY_ - kCy);
    bool onBoard = fate_ != Fate::Short && r <= kRim + 0.5f;
    Mark m = classify(landX_, landY_);
    if (fate_ == Fate::Short) std::snprintf(last_, sizeof last_, "SHORT");
    else if (fate_ == Fate::Hot) std::snprintf(last_, sizeof last_, "HOT");
    else markName(m, last_, int(sizeof last_));

    bool gold = fate_ == Fate::True && rules_ && hourBed(m);
    if (gold && onHour()) {
        remember(landX_, landY_, true);
        if (sys_) sys_->apu.noiseBurst(0.22f, 240.f, 0.08f);
        beginChime();
        return;
    }
    if (gold && !pastHour()) {
        remember(landX_, landY_, false);
        if (thrown_ >= 3) beginFail("EARLY");
        else beginEarly();
        return;
    }
    if (onBoard) remember(landX_, landY_, true);
    else remember(landX_, landY_, true);
    if (sys_) sys_->apu.noiseBurst(0.1f, 160.f, 0.05f);
    if (pastHour()) beginFail("LATE");
    else if (thrown_ >= 3) beginFail(gold ? "LATE" : "SPENT");
    else beginDead();
}

void Game::botAim() {
    float x, y;
    bedPoint(kHourSeg, kHourRad, x, y);
    float dx = x - aimX_;
    float dy = y - aimY_;
    float dist = std::hypot(dx, dy);
    if (dist > 0.6f) {
        float step = std::min(7.f, dist);
        aimX_ += dx / dist * step;
        aimY_ += dy / dist * step;
        return;
    }
    aimX_ = x;
    aimY_ = y;
    if (!hourBed(classify(aimX_, aimY_))) return;
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
    float mx = 0, my = 0;
    if (p.down(gs::BTN_LEFT)) mx -= 1.f;
    if (p.down(gs::BTN_RIGHT)) mx += 1.f;
    if (p.down(gs::BTN_UP)) my -= 1.f;
    if (p.down(gs::BTN_DOWN)) my += 1.f;
    if (std::fabs(p.axisX) > 0.2f) mx = p.axisX;
    if (std::fabs(p.axisY) > 0.2f) my = -p.axisY;
    moveAim(mx, my);
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
    sys_->apu.keyOn(0, 349.23f, 0.34f);
    sys_->apu.tone(1, 698.46f, 0.04f);
    toneT_ = 0.16f;
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
    if (until > 0 && until <= kFpc * 12) blip(0, (sec & 1) ? 880.f : 698.f, 0.045f, 0.04f);
    else if ((sec % 10) == 0) blip(0, 196.f, 0.03f, 0.04f);
}

void Game::tickAudio(float dt) {
    if (!sys_) return;
    if (toneT_ > 0.f) {
        toneT_ -= dt;
        if (toneT_ <= 0.f) {
            sys_->apu.tone(0, 0, 0);
            sys_->apu.tone(1, 0, 0);
        }
    }
    if (tickT_ > 0.f) {
        tickT_ -= dt;
        if (tickT_ <= 0.f) sys_->apu.tone(2, 0, 0);
    }
    if (mode_ == Mode::Flight) sys_->apu.noise(0.035f, 1500.f, true);
    else sys_->apu.noise(0, 1000);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    anim_ += kDt;
    bool chiming = mode_ == Mode::Chime || mode_ == Mode::Leave || (mode_ == Mode::Over && won_);
    bellPh_ += kDt * (chiming ? 13.f : 2.1f);
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
        stepMeter(meter_, meterDir_);
        if (bot_ && titleFrames_ >= 30) newGame();
        else if (back) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        } else if (start || fire) newGame();
    } else if (mode_ == Mode::Aim) {
        if (clockOn_) playFrames_++;
        stepMeter(meter_, meterDir_);
        if (pastHour()) beginFail("LATE");
        else if (!bot_ && start) {
            held_ = Mode::Aim;
            mode_ = Mode::Pause;
        } else if (!bot_ && back) toTitle();
        else if (bot_) botAim();
        else humanAim(p, fire);
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
        if (leaveT_ > 48) {
            mode_ = Mode::Over;
            won_ = true;
            over_ = true;
            sys.apu.keyOff(0);
        }
    } else if (mode_ == Mode::Fail) {
        if (++failFrames_ > 48) over_ = true;
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
        else sys.setLight(70, 42, 28);
    }
    draw();
}

void Game::spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool shadow, bool flip) {
    if (!sys_ || w < 1.f || h < 1.f || img.w == 0) return;
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = img;
    s.pal = uint8_t(pal);
    s.shadow = shadow;
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::dartAt(float x, float y, float h) {
    float ih = std::max(8.f, h);
    const gs::Image& img = art_.dart.pick(ih);
    float iw = ih * float(art_.dart.w) / float(std::max(1, int(art_.dart.h)));
    spr(img, x, y - ih * 0.15f, iw, ih, PAL_DART);
    spr(art_.shadow, x + 1.f, y + 3.f, iw * 0.7f, 4.f, PAL_BOARD, true);
}

void Game::clockAt() {
    int h, m, s;
    split(h, m, s);
    int hi = ((h % 12) * 5 + m / 12) % 60;
    bool noon = onHour() || mode_ == Mode::Chime || mode_ == Mode::Leave || (mode_ == Mode::Over && won_);
    if (noon) spr(art_.ring, kClockX, kClockY, kClockS * 1.06f, kClockS * 1.06f, PAL_GOLD);
    spr(art_.cap, kClockX, kClockY, 10, 10, PAL_HAND);
    spr(art_.hand[2][s % 60], kClockX, kClockY, kClockS, kClockS, PAL_HAND);
    spr(art_.hand[0][hi], kClockX, kClockY, kClockS, kClockS, PAL_HAND);
    spr(art_.hand[1][m % 60], kClockX, kClockY, kClockS, kClockS, PAL_HAND);
    spr(art_.face, kClockX, kClockY, kClockS, kClockS, PAL_CLOCK);
}

void Game::meterRow() {
    for (int i = 0; i < 9; i++) {
        float x = kCx + float(i - 4) * 11.f;
        bool mid = i == 4;
        bool on = std::fabs(meter_ * 8.f - float(i)) <= 0.55f;
        int pal = mid ? PAL_GOLD : PAL_WOOD;
        if (on) pal = sweet() ? PAL_GREEN : PAL_ALERT;
        float s = mid ? 9.f : 7.f;
        spr(art_.pip, x, kMeterY, s, s, pal);
    }
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
    uint16_t top = win ? gs::rgb4(6, 4, 2) : dead ? gs::rgb4(3, 1, 2) : gs::rgb4(2, 1, 3);
    uint16_t mid = win ? gs::rgb4(10, 7, 3) : dead ? gs::rgb4(6, 2, 2) : gs::rgb4(5, 3, 4);
    uint16_t wall = win ? gs::rgb4(12, 9, 4) : dead ? gs::rgb4(5, 2, 2) : gs::rgb4(6, 4, 5);
    uint16_t wood = win ? gs::rgb4(8, 5, 2) : dead ? gs::rgb4(4, 1, 1) : gs::rgb4(4, 2, 1);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.road[y].on = false;
        v.lineFog[y] = 0;
        float u = y / float(gs::SCREEN_H - 1);
        uint16_t c;
        if (u < 0.18f) c = mix(top, mid, u / 0.18f);
        else if (u < 0.72f) c = mix(mid, wall, (u - 0.18f) / 0.54f);
        else c = mix(wall, wood, (u - 0.72f) / 0.28f);
        if (y >= 176 && ((y / 4) & 1)) c = mix(c, gs::rgb4(3, 1, 1), 0.35f);
        v.lineBackdrop[y] = c;
    }
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    bool chiming = mode_ == Mode::Chime || mode_ == Mode::Leave || (mode_ == Mode::Over && won_);
    bool showAim = mode_ == Mode::Title || mode_ == Mode::Aim || mode_ == Mode::Pause;
    bool showMeter = mode_ == Mode::Title || mode_ == Mode::Aim || (mode_ == Mode::Pause && held_ == Mode::Aim);

    if (mode_ == Mode::Title) spr(art_.title, kCx, 16.f, float(art_.title.w), float(art_.title.h), PAL_TITLE);

    if (showAim) {
        Mark live = classify(aimX_, aimY_);
        int pal = PAL_INK;
        if (hourBed(live) && sweet()) pal = PAL_GREEN;
        else if (hourBed(live)) pal = PAL_GOLD;
        else if (sweet()) pal = PAL_ALERT;
        float bob = mode_ == Mode::Title ? std::sin(anim_ * 3.f) * 1.1f : 0.f;
        spr(art_.cross, aimX_, aimY_ + bob, float(art_.cross.w), float(art_.cross.h), pal);
    }
    if (showMeter) meterRow();

    if (mode_ == Mode::Flight) {
        float u = clampf(float(flightT_) / float(kFlight), 0.f, 1.f);
        float e = u * u * (3.f - 2.f * u);
        float x = fromX_ + (landX_ - fromX_) * e;
        float y = fromY_ + (landY_ - fromY_) * e;
        float lift = std::sin(u * kPi) * (fate_ == Fate::Short ? 36.f : 18.f);
        float h = 30.f + (13.f - 30.f) * e;
        dartAt(x, y - lift, h);
    }
    for (int i = pinN_ - 1; i >= 0; --i) {
        if (!pin_[i].on) continue;
        float r = std::hypot(pin_[i].x - kCx, pin_[i].y - kCy);
        dartAt(pin_[i].x, pin_[i].y, r > kRim ? 20.f : 13.f);
    }
    for (int i = 0; i < 3; i++) {
        if (i < thrown_) continue;
        dartAt(16.f, 78.f + float(i) * 28.f, 26.f);
    }

    float hx, hy;
    bedPoint(kHourSeg, kRim + 7.f, hx, hy);
    float pulse = 4.f + std::sin(anim_ * 5.f) * (onHour() || chiming ? 1.6f : 0.6f);
    spr(art_.dot, hx, hy, pulse, pulse, (onHour() || chiming) ? PAL_GREEN : PAL_GOLD);

    if (chiming) {
        for (int i = 0; i < 6; i++) {
            float a = bellPh_ * 1.4f + float(i) * 1.047f;
            float rad = 30.f + float(i % 3) * 5.f;
            spr(art_.dot, kClockX + std::cos(a) * rad, kClockY + std::sin(a) * rad * 0.62f, 3.5f, 3.5f, PAL_GOLD);
        }
    }

    float swing = std::sin(bellPh_) * 7.f * bellAmp_;
    spr(art_.bell, kClockX - 34.f + swing, kClockY - 42.f, 20, 18, PAL_BELL);
    spr(art_.bell, kClockX + 34.f - swing, kClockY - 42.f, 20, 18, PAL_BELL, false, true);
    spr(art_.yoke, kClockX, kClockY - 52.f, float(art_.yoke.w), float(art_.yoke.h), PAL_WOOD);
    clockAt();

    spr(art_.flame, 18.f, 168.f, 10, 14, PAL_FLAME);
    spr(art_.flame, 304.f, 176.f, 8, 12, PAL_FLAME);
    spr(art_.oche, kCx, 200.f, float(art_.oche.w), float(art_.oche.h), PAL_CHALK);

    gs::Sprite board;
    board.img = art_.board;
    board.x = int16_t(kBoardX);
    board.y = int16_t(kBoardY);
    board.w = int16_t(art_.board.w);
    board.h = int16_t(art_.board.h);
    board.pal = PAL_BOARD;
    v.sprite(board);

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(24, "STICK DOUBLE TWELVE", PAL_GOLD);
        hudC(25, "AS THE HOUR CHIMES", PAL_INK);
        hudC(26, "THEN LEAVE", PAL_GREEN);
        if ((titleFrames_ / 30) & 1) hudC(27, "PRESS START", PAL_GOLD);
        else hudC(27, "ARROWS AIM   Z ON GREEN", PAL_GREEN);
        return;
    }
    if (mode_ == Mode::Pause) {
        hudC(24, "PAUSED", PAL_GOLD);
        hudC(27, "START RESUME   ESC TITLE", PAL_INK);
        return;
    }

    int h, m, s;
    split(h, m, s);
    std::snprintf(buf, sizeof buf, "%d:%02d:%02d", h, m, s);
    bool hot = onHour() || chiming;
    hud(1, 0, buf, hot ? PAL_GOLD : PAL_INK);
    int shown = thrown_ < 3 ? thrown_ + (mode_ == Mode::Aim || mode_ == Mode::Pause ? 1 : 0) : thrown_;
    if (shown < 1) shown = 1;
    std::snprintf(buf, sizeof buf, "DART %d/3", shown);
    hud(30, 0, buf, thrown_ >= 2 ? PAL_ALERT : PAL_GOLD);

    if (chiming || (mode_ == Mode::Over && won_)) {
        hud(1, 1, "ON THE HOUR", PAL_GOLD);
        hudC(24, mode_ == Mode::Leave || mode_ == Mode::Over ? "LEAVE" : "THE HOUR CHIMES", PAL_GOLD);
        hudC(25, "DOUBLE TWELVE", PAL_INK);
        if (mode_ == Mode::Over && !bot_) hudC(27, "START AGAIN", PAL_GOLD);
        return;
    }
    if (mode_ == Mode::Fail || (mode_ == Mode::Over && !won_)) {
        hud(1, 1, "TOO LATE", PAL_ALERT);
        bool late = std::strcmp(reason_, "LATE") == 0 || pastHour();
        hudC(24, late ? "THE HOUR PASSED" : "NO CHIME", PAL_ALERT);
        hudC(25, reason_[0] ? reason_ : "OPEN", PAL_INK);
        if (!bot_) hudC(27, "START AGAIN", PAL_GOLD);
        return;
    }
    if (mode_ == Mode::Early) {
        hudC(24, "EARLY", PAL_ALERT);
        hudC(25, "THE HOUR HAS NOT CHIMED", PAL_GOLD);
        hudC(26, thrown_ >= 2 ? "ONE DART LEFT" : "DART FALLS OUT", PAL_INK);
        return;
    }
    if (mode_ == Mode::Dead) {
        hudC(24, last_[0] ? last_ : "WIDE", PAL_ALERT);
        hudC(25, "NOT THE HOUR", PAL_INK);
        hudC(26, thrown_ >= 2 ? "ONE DART LEFT" : "STILL SHORT OF TWELVE", PAL_GOLD);
        return;
    }

    int until = framesUntilHour();
    if (until > 0) {
        int show = (until + kFpc - 1) / kFpc;
        std::snprintf(buf, sizeof buf, "HOUR IN %d", show);
        hud(1, 1, buf, until <= kFpc * 12 ? PAL_GOLD : PAL_INK);
    } else if (onHour()) hud(1, 1, "ON THE HOUR", PAL_GOLD);
    else hud(1, 1, "TOO LATE", PAL_ALERT);

    if (mode_ == Mode::Flight) {
        hudC(26, "IN THE AIR", PAL_GOLD);
        return;
    }
    Mark live = classify(aimX_, aimY_);
    char name[12];
    markName(live, name, int(sizeof name));
    int pal = PAL_INK;
    if (hourBed(live)) pal = sweet() ? PAL_GREEN : PAL_GOLD;
    else if (live.bed == Bed::Miss) pal = PAL_ALERT;
    hudC(26, name, pal);
    if (hourBed(live) && onHour() && sweet()) hudC(27, "THROW", PAL_GREEN);
    else if (onHour()) hudC(27, sweet() ? "NOT THE HOUR" : "WAIT FOR GREEN", sweet() ? PAL_ALERT : PAL_GOLD);
    else if (hourBed(live)) hudC(27, sweet() ? "HOLD FOR THE HOUR" : "WAIT FOR GREEN", PAL_AIM);
    else hudC(27, "FIND THE GOLD TWELVE", PAL_INK);
}

}  // namespace dartchime

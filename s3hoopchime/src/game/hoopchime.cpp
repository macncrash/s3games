#include "game/hoopchime.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace hoopchime {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr int kSub = 8;
constexpr float kH = kDt / float(kSub);
constexpr float kG = 9.81f;
constexpr float kPi = 3.14159265f;
constexpr float kAngle = 51.f * kPi / 180.f;
constexpr float kRimX = 0.f;
constexpr float kRimY = 3.05f;
constexpr float kBallR = 0.12f;
constexpr float kTubeR = 0.02f;
constexpr float kRimR = 0.40f;
constexpr float kCountR = 0.14f;
constexpr float kBoardX = 0.68f;
constexpr float kReleaseY = 2.05f;
constexpr float kHand = 0.30f;
constexpr float kBotX = -6.05f;
constexpr float kMinX = -7.20f;
constexpr float kMaxX = -4.50f;
constexpr float kMaxZ = 0.85f;
constexpr float kSweetLo = 0.42f;
constexpr float kSweetHi = 0.58f;
constexpr float kMeterRate = 0.62f;
constexpr float kFeetRate = 2.4f;
constexpr float kAimRate = 0.50f;
constexpr float kPx = 22.f;
constexpr float kRimScr = 246.f;
constexpr float kFloor = 198.f;
constexpr float kClockX = 50.f;
constexpr float kClockY = 62.f;
constexpr float kClockS = 52.f;
constexpr float kGlassX = 164.f;
constexpr float kGlassY = 78.f;
constexpr float kGlassS = 44.f;
constexpr float kMeterX = 176.f;
constexpr float kMeterW = 120.f;
constexpr float kMeterY = 20.f;
constexpr int kHourSec = 12 * 3600;
constexpr int kStartSec = kHourSec - 120;
constexpr int kCap = 60 * 5;

float scrX(float x) { return kRimScr + x * kPx; }
float scrY(float y) { return kFloor - y * kPx; }
float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }
int rgbClamp(int v) { return std::max(0, std::min(15, v)); }

float speedScale(float m) {
    if (m >= kSweetLo && m <= kSweetHi) return 1.f;
    if (m > kSweetHi) return 1.12f + (m - kSweetHi) * 0.9f;
    return 0.88f - (kSweetLo - m) * 0.9f;
}

bool solveArc(float x0, float lat, float& vx, float& vy, float& vz) {
    float dx = kRimX - x0;
    float dz = lat;
    float dist = std::hypot(dx, dz);
    float cosA = std::cos(kAngle);
    float sinA = std::sin(kAngle);
    float rise = std::tan(kAngle) * dist - (kRimY - kReleaseY);
    if (rise < 0.25f || dist < 1.f) return false;
    float s2 = (0.5f * kG * dist * dist) / (cosA * cosA * rise);
    if (s2 <= 0.f) return false;
    float s = std::sqrt(s2);
    float vh = s * cosA;
    vx = vh * dx / dist;
    vz = vh * dz / dist;
    vy = s * sinA;
    return true;
}

bool launchBall(Ball& b, Trace& t, float feet, float lat, float meter) {
    float vx, vy, vz;
    if (!solveArc(feet + kHand, lat, vx, vy, vz)) return false;
    float sc = speedScale(meter);
    t = Trace{};
    b.x = feet + kHand;
    b.y = kReleaseY;
    b.z = 0.f;
    b.vx = vx * sc;
    b.vy = vy * sc;
    b.vz = vz * sc;
    return true;
}

void stepBall(Ball& b, Trace& t, float dt) {
    float px = b.x, py = b.y, pz = b.z;
    b.y += b.vy * dt - 0.5f * kG * dt * dt;
    b.vy -= kG * dt;
    b.x += b.vx * dt;
    b.z += b.vz * dt;
    float ix = b.x, iy = b.y, iz = b.z;

    if (b.y <= kBallR) {
        b.y = kBallR;
        if (b.vy < 0.f) b.vy = -b.vy * 0.42f;
        if (std::fabs(b.vy) < 0.8f) b.vy = 0.f;
        b.vx *= 0.90f;
        b.vz *= 0.90f;
    }

    if (t.clean) {
        b.vx += (kRimX - b.x) * 6.f * dt;
        b.vz += (0.f - b.z) * 6.f * dt;
        b.vx *= 0.82f;
        b.vz *= 0.82f;
        return;
    }

    float dx = b.x - kRimX;
    float dz = b.z;
    float d = std::hypot(dx, dz);
    float hole = kRimR - kBallR;
    if (d >= hole) {
        float ax = d < 1e-4f ? kRimR : kRimR * dx / d;
        float az = d < 1e-4f ? 0.f : kRimR * dz / d;
        float ox = b.x - (kRimX + ax);
        float oy = b.y - kRimY;
        float oz = b.z - az;
        float od = std::sqrt(ox * ox + oy * oy + oz * oz);
        float minD = kBallR + kTubeR;
        if (od < minD && od > 1e-5f) {
            float nx = ox / od, ny = oy / od, nz = oz / od;
            float pen = minD - od;
            b.x += nx * pen;
            b.y += ny * pen;
            b.z += nz * pen;
            float vn = b.vx * nx + b.vy * ny + b.vz * nz;
            if (vn < 0.f) {
                b.vx -= 1.55f * vn * nx;
                b.vy -= 1.55f * vn * ny;
                b.vz -= 1.55f * vn * nz;
            }
            t.rim = true;
        }
    }

    if (b.x + kBallR > kBoardX && b.vx > 0.f && std::fabs(b.z) < 0.85f && b.y > kRimY - 0.2f && b.y < kRimY + 1.2f) {
        b.x = kBoardX - kBallR;
        b.vx = -std::fabs(b.vx) * 0.55f;
        b.vy *= 0.92f;
        t.bank = true;
    }

    if (!t.crossed && py >= kRimY && iy < kRimY) {
        float den = py - iy;
        float u = den > 1e-8f ? (py - kRimY) / den : 1.f;
        float cx = px + (ix - px) * u;
        float cz = pz + (iz - pz) * u;
        t.crossed = true;
        t.crossX = cx;
        t.crossZ = cz;
        t.rad = std::hypot(cx - kRimX, cz);
        if (t.rad <= kCountR && !t.rim && !t.bank) {
            t.clean = true;
            t.scoreAt = t.flight;
        }
    }
}

bool settled(const Ball& b, const Trace& t) {
    if (t.clean && t.flight > t.scoreAt + 0.36f) return true;
    bool rest = b.y <= kBallR + 0.05f && std::fabs(b.vy) < 0.55f && std::hypot(b.vx, b.vz) < 0.65f && t.flight > 0.45f;
    bool gone = b.x < -12.f || b.x > 4.f || std::fabs(b.z) > 5.f;
    return rest || gone || t.flight > 4.2f;
}

Kind classify(const Trace& t) {
    if (t.clean) return Kind::Swish;
    if (t.rim || t.bank) return Kind::Iron;
    if (!t.crossed || t.crossX < kRimX - 0.20f) return Kind::Short;
    if (t.crossX > kRimX + 0.22f) return Kind::Hot;
    if (t.rad > kCountR) return Kind::Wide;
    return Kind::Miss;
}

const char* kindName(Kind k) {
    switch (k) {
    case Kind::Swish: return "SWISH";
    case Kind::Iron: return "IRON";
    case Kind::Short: return "SHORT";
    case Kind::Hot: return "HOT";
    case Kind::Wide: return "WIDE";
    case Kind::Miss: return "MISS";
    }
    return "MISS";
}

struct Forecast {
    Kind kind = Kind::Miss;
    int score = 0;
    float rad = 99.f;
};

Forecast forecast(float feet, float lat, float meter) {
    Forecast r;
    Ball b;
    Trace t;
    if (!launchBall(b, t, feet, lat, meter)) return r;
    for (int frame = 1; frame <= kCap; frame++) {
        bool stop = false;
        for (int i = 0; i < kSub; i++) {
            t.flight += kH;
            stepBall(b, t, kH);
            if (t.clean && r.score == 0) r.score = frame;
            if (settled(b, t)) {
                stop = true;
                break;
            }
        }
        if (stop) break;
    }
    r.kind = classify(t);
    r.rad = t.rad;
    return r;
}

bool inWindow(int score, int until) {
    if (score < 1) return false;
    const int slack = kGraceSec * kFpc - 1;
    return until <= score && until >= score - slack;
}

gs::FMPatch bellPatch() {
    gs::FMPatch p;
    p.alg = 6;
    p.fb = 0.16f;
    p.op[0] = {1.0f, 1.0f, 0.002f, 0.10f, 0.0f, 0.08f};
    p.op[1] = {2.7f, 0.32f, 0.002f, 0.14f, 0.0f, 0.10f};
    p.op[2] = {4.1f, 0.16f, 0.003f, 0.12f, 0.0f, 0.08f};
    p.op[3] = {1.5f, 0.20f, 0.004f, 0.22f, 0.05f, 0.12f};
    p.vol = 0.30f;
    p.echo = 0.36f;
    p.tone = 1800.f;
    return p;
}

}  // namespace

const char* Game::phase() const {
    switch (mode_) {
    case Mode::Title: return "title";
    case Mode::Aim: return "aim";
    case Mode::Flight: return "flight";
    case Mode::Call: return "call";
    case Mode::Chime: return "chime";
    case Mode::Fail: return "fail";
    case Mode::Over: return "over";
    case Mode::Pause: return "pause";
    }
    return "?";
}

int Game::clockSec() const { return kStartSec + playFrames_ / kFpc; }

int Game::framesUntilHour() const {
    int sec = clockSec();
    int sub = playFrames_ % kFpc;
    if (sec > kHourSec) return -((sec - kHourSec) * kFpc + sub);
    if (sec == kHourSec) return -sub;
    int secLeft = kHourSec - sec;
    return (secLeft - 1) * kFpc + (kFpc - sub);
}

bool Game::onHour() const {
    int sec = clockSec();
    return sec >= kHourSec && sec < kHourSec + kGraceSec;
}

bool Game::pastHour() const { return clockSec() >= kHourSec + kGraceSec; }

void Game::split(int& h, int& m, int& s) const {
    int t = clockSec();
    if (t < 0) t = 0;
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

void Game::faceTime(int& h, int& m, int& s) const {
    if (mode_ == Mode::Title || (mode_ == Mode::Pause && held_ == Mode::Title)) {
        h = 11;
        m = 58;
        s = (titleFrames_ / 4) % 60;
        return;
    }
    split(h, m, s);
}

void Game::solve() {
    struct T {
        float x, z, m;
        bool swish;
        const char* name;
    };
    const T tests[] = {
        {kBotX, 0.f, 0.50f, true, "TRUE"},   {kBotX, 0.f, 0.44f, true, "LO"},
        {kBotX, 0.f, 0.56f, true, "HI"},     {kMinX, 0.f, 0.50f, true, "FAR"},
        {kMaxX, 0.f, 0.50f, true, "CLOSE"},  {kBotX, 0.f, 0.22f, false, "SHORT"},
        {kBotX, 0.f, 0.84f, false, "HOT"},   {kBotX, 0.55f, 0.50f, false, "WIDE"},
    };
    bool ok = true;
    Forecast best;
    for (const T& t : tests) {
        Forecast f = forecast(t.x, t.z, t.m);
        bool sw = f.kind == Kind::Swish;
        if (sw != t.swish) {
            std::fprintf(stderr, "s3hoopchime %s -> %s rad %.3f score %d\n", t.name, kindName(f.kind), f.rad, f.score);
            ok = false;
        }
        if (std::strcmp(t.name, "TRUE") == 0) best = f;
    }
    scoreFrames_ = best.score;
    rules_ = ok && best.kind == Kind::Swish && best.score >= 24 && best.score <= 110;
    if (!rules_) std::fprintf(stderr, "s3hoopchime rules failed  score %d %s\n", best.score, kindName(best.kind));
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    solve();
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(3, 2, 4));
    sys.apu.setMaster(0.72f);
    sys.apu.setEcho(0.18f, 0.28f, 0.16f);
    sys.apu.setPatch(0, bellPatch());
    sys.apu.setPan(0, -0.15f);
    showTitle();
    draw();
}

void Game::showTitle() {
    mode_ = Mode::Title;
    titleFrames_ = 0;
    playFrames_ = 0;
    over_ = false;
    won_ = false;
    clean_ = false;
    shots_ = 0;
    reason_ = "";
    hint_ = Kind::Miss;
    last_ = Kind::Miss;
    feet_ = kBotX;
    aimZ_ = 0.f;
    meter_ = 0.f;
    meterDir_ = 1.f;
    wasSweet_ = false;
    wasWindow_ = false;
    sawClean_ = false;
    lastSec_ = -1;
    strikes_ = 0;
    flash_ = 0;
    bellAmp_ = 0.2f;
    bellPh_ = 0.f;
    ball_ = Ball{};
    tr_ = Trace{};
    if (!sys_) return;
    sys_->setLight(70, 80, 140);
    sys_->apu.keyOff(0);
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(1, 0, 0);
    sys_->apu.tone(2, 0, 0);
    sys_->apu.noise(0.f, 900.f, false);
}

void Game::newGame() {
    over_ = false;
    won_ = false;
    clean_ = false;
    shots_ = 0;
    playFrames_ = 0;
    flightFrames_ = 0;
    splash_ = 0;
    chimeFrames_ = 0;
    failFrames_ = 0;
    strikes_ = 0;
    lastSec_ = -1;
    reason_ = "";
    wasWindow_ = false;
    wasSweet_ = false;
    sawClean_ = false;
    flash_ = 0;
    bellAmp_ = 0.2f;
    feet_ = kBotX;
    aimZ_ = 0.f;
    if (sys_) {
        sys_->setLight(90, 70, 120);
        sys_->apu.keyOff(0);
    }
    beginAim();
}

void Game::beginAim() {
    mode_ = Mode::Aim;
    meter_ = bot_ ? 0.50f : 0.f;
    meterDir_ = 1.f;
    tr_ = Trace{};
    sawClean_ = false;
    wasSweet_ = false;
    wasWindow_ = false;
    hint_ = bot_ ? Kind::Swish : Kind::Miss;
    if (bot_) {
        feet_ = kBotX;
        aimZ_ = 0.f;
    }
    feet_ = clampf(feet_, kMinX, kMaxX);
    aimZ_ = clampf(aimZ_, -kMaxZ, kMaxZ);
}

bool Game::launch(float meter) {
    if (mode_ != Mode::Aim) return false;
    if (!launchBall(ball_, tr_, feet_, aimZ_, meter)) return false;
    shots_++;
    flightFrames_ = 0;
    sawClean_ = false;
    mode_ = Mode::Flight;
    if (sys_) {
        sys_->apu.noiseBurst(0.16f, 1800.f, 0.05f);
        blip(520.f);
        sys_->rumble(0.15f, 0.28f, 60);
    }
    return true;
}

void Game::fly() {
    flightFrames_++;
    for (int i = 0; i < kSub; i++) {
        tr_.flight += kH;
        stepBall(ball_, tr_, kH);
        if (tr_.clean && !sawClean_) {
            sawClean_ = true;
            if (onHour()) {
                beginChime();
                return;
            }
            finishShot();
            return;
        }
        if (settled(ball_, tr_)) {
            finishShot();
            return;
        }
    }
}

void Game::finishShot() {
    if (mode_ != Mode::Flight) return;
    last_ = classify(tr_);
    if (last_ == Kind::Swish && onHour() && sawClean_) {
        beginChime();
        return;
    }
    const char* why = last_ == Kind::Swish ? (pastHour() ? "LATE" : "EARLY") : kindName(last_);
    if (pastHour() || shots_ >= kShots) beginFail(why);
    else beginCall(why);
}

void Game::beginCall(const char* why) {
    reason_ = why;
    mode_ = Mode::Call;
    splash_ = 0;
    ball_.vx = ball_.vy = ball_.vz = 0;
    if (!sys_) return;
    blip(std::strcmp(why, "EARLY") == 0 ? 180.f : 120.f);
    sys_->setLight(140, 50, 40);
}

void Game::beginChime() {
    if (won_) return;
    won_ = true;
    clean_ = true;
    reason_ = "CHIME";
    mode_ = Mode::Chime;
    chimeFrames_ = 0;
    strikes_ = 0;
    bellAmp_ = 1.f;
    flash_ = 20;
    ball_.x = kRimX;
    ball_.y = kRimY - 0.28f;
    ball_.z = 0.f;
    ball_.vx = ball_.vy = ball_.vz = 0.f;
    if (!sys_) return;
    sys_->apu.noise(0.f, 900.f, false);
    sys_->setLight(255, 196, 64);
    sys_->rumble(0.45f, 0.85f, 220);
    blip(880.f);
}

void Game::beginFail(const char* why) {
    reason_ = why;
    won_ = false;
    clean_ = false;
    mode_ = Mode::Fail;
    failFrames_ = 0;
    if (!sys_) return;
    blip(90.f);
    sys_->setLight(140, 24, 24);
    sys_->apu.keyOff(0);
}

void Game::humanAim(const gs::Pad& pad, bool fire) {
    float mx = 0.f, my = 0.f;
    if (pad.down(gs::BTN_LEFT)) mx -= 1.f;
    if (pad.down(gs::BTN_RIGHT)) mx += 1.f;
    if (pad.down(gs::BTN_UP)) my -= 1.f;
    if (pad.down(gs::BTN_DOWN)) my += 1.f;
    if (std::fabs(pad.axisX) > 0.25f) mx = pad.axisX;
    if (std::fabs(pad.axisY) > 0.25f) my = -pad.axisY;
    feet_ = clampf(feet_ + mx * kFeetRate * kDt, kMinX, kMaxX);
    aimZ_ = clampf(aimZ_ + my * kAimRate * kDt, -kMaxZ, kMaxZ);
    meter_ += meterDir_ * kMeterRate * kDt;
    if (meter_ >= 1.f) {
        meter_ = 1.f;
        meterDir_ = -1.f;
    } else if (meter_ <= 0.f) {
        meter_ = 0.f;
        meterDir_ = 1.f;
    }
    Forecast f = forecast(feet_, aimZ_, meter_);
    hint_ = f.kind;
    bool sweet = hint_ == Kind::Swish;
    bool window = sweet && inWindow(f.score, framesUntilHour());
    if (sweet && !wasSweet_) blip(740.f);
    if (window && !wasWindow_) blip(988.f);
    wasSweet_ = sweet;
    wasWindow_ = window;
    if (fire) launch(meter_);
}

void Game::dribble() {
    float s = std::sin(anim_ * 0.22f);
    ball_.x = feet_ + 0.55f;
    ball_.y = 0.18f + std::fabs(s) * 0.85f;
    ball_.z = 0.f;
    ball_.vx = ball_.vy = ball_.vz = 0.f;
    if (s > 0.f && dribS_ <= 0.f && mode_ != Mode::Pause && sys_) sys_->apu.noiseBurst(0.07f, 260.f, 0.03f);
    dribS_ = s;
}

void Game::blip(float freq) {
    if (!sys_) return;
    sys_->apu.tone(2, freq, 0.05f);
    beep_ = 5;
}

void Game::strikeBell() {
    if (!sys_) return;
    sys_->apu.keyOn(0, 523.25f, 0.36f);
    sys_->apu.tone(1, 261.6f, 0.04f);
    strikes_++;
    beep_ = 6;
    bellAmp_ = 1.f;
}

void Game::decayAudio() {
    if (!sys_) return;
    if (beep_ > 0 && --beep_ == 0 && mode_ != Mode::Chime && !(mode_ == Mode::Over && won_)) {
        sys_->apu.tone(1, 0, 0);
        sys_->apu.tone(2, 0, 0);
    }
    if (mode_ == Mode::Flight) sys_->apu.noise(0.018f, 1500.f, true);
    else sys_->apu.noise(0.f, 900.f, false);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    anim_ += 1.f;
    bool chiming = mode_ == Mode::Chime || (mode_ == Mode::Over && won_);
    bellPh_ += chiming ? 0.46f : 0.05f;
    if (bellAmp_ > 0.18f) bellAmp_ = std::max(0.18f, bellAmp_ - 0.02f);
    if (flash_ > 0) flash_--;
    decayAudio();

    const gs::Pad& pad = sys.pad;
    bool start = pad.pressed(gs::BTN_START);
    bool back = pad.pressed(gs::BTN_MODE);
    bool fire = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_TURBO);
    if (bot_) {
        start = false;
        back = false;
        fire = false;
    }

    if (mode_ == Mode::Pause) {
        if (start || fire) mode_ = held_;
        else if (back) showTitle();
        draw();
        return;
    }

    if (mode_ == Mode::Title) titleFrames_++;
    if (mode_ == Mode::Aim || mode_ == Mode::Flight) {
        playFrames_++;
        int sec = clockSec();
        if (sec != lastSec_) {
            int prev = lastSec_;
            lastSec_ = sec;
            if (prev >= 0) {
                int until = framesUntilHour();
                if (until > 0 && until <= kFpc * 18) blip(until <= kFpc * 6 ? 880.f : 620.f);
                else if (sec % 10 == 0) blip(196.f);
            }
        }
    }

    if (back && mode_ == Mode::Title) {
        if (sys.hasHome()) sys.eject();
        else if (!bot_) sys.quit();
        draw();
        return;
    }
    if (back && !bot_ && mode_ != Mode::Chime && mode_ != Mode::Over && mode_ != Mode::Fail) {
        showTitle();
        draw();
        return;
    }
    if (start && !bot_ && (mode_ == Mode::Aim || mode_ == Mode::Flight)) {
        held_ = mode_;
        mode_ = Mode::Pause;
        draw();
        return;
    }

    if (mode_ == Mode::Title) {
        dribble();
        if (bot_) {
            if (titleFrames_ >= 40) newGame();
        } else if (start || fire) newGame();
    } else if (mode_ == Mode::Aim) {
        if (pastHour()) beginFail("LATE");
        else if (bot_) {
            feet_ = kBotX;
            aimZ_ = 0.f;
            meter_ = 0.50f;
            hint_ = Kind::Swish;
            wasWindow_ = inWindow(scoreFrames_, framesUntilHour());
            if (rules_ && wasWindow_) launch(0.50f);
        } else humanAim(pad, fire);
        if (mode_ == Mode::Aim) dribble();
    } else if (mode_ == Mode::Flight) {
        fly();
    } else if (mode_ == Mode::Call) {
        if (++splash_ >= 36 || (!bot_ && (start || fire))) {
            if (pastHour() || shots_ >= kShots) beginFail(reason_);
            else beginAim();
        }
    } else if (mode_ == Mode::Chime) {
        if (chimeFrames_ % 6 == 0 && strikes_ < 12) strikeBell();
        if (++chimeFrames_ >= 12 * 6 + 24) {
            mode_ = Mode::Over;
            over_ = true;
            sys.apu.keyOff(0);
        }
    } else if (mode_ == Mode::Fail) {
        if (++failFrames_ >= 48) {
            mode_ = Mode::Over;
            over_ = true;
        }
    } else if (mode_ == Mode::Over) {
        if (!bot_ && (start || fire)) newGame();
    }

    draw();
}

void Game::spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool flip, bool shadow) {
    if (!sys_ || w < 1.f || h < 1.f || img.w == 0) return;
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 8 || s.y > gs::SCREEN_H + 8 || s.x + s.w < -8 || s.y + s.h < -8) return;
    s.img = img;
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::sprM(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (m.h < 1 || h < 1.f) return;
    float w = h * float(m.w) / float(m.h);
    spr(m.pick(h), cx, cy, w, h, pal, flip, false);
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

const gs::Image* Game::wordFor(const char* why) const {
    if (!why) return &art_.missW;
    if (!std::strcmp(why, "EARLY")) return &art_.earlyW;
    if (!std::strcmp(why, "LATE")) return &art_.lateW;
    if (!std::strcmp(why, "SHORT")) return &art_.shortW;
    if (!std::strcmp(why, "HOT")) return &art_.hotW;
    if (!std::strcmp(why, "WIDE")) return &art_.wideW;
    if (!std::strcmp(why, "IRON")) return &art_.ironW;
    return &art_.missW;
}

void Game::clockAt(float cx, float cy, float size, bool live) {
    int h, m, s;
    faceTime(h, m, s);
    int hi = ((h % 12) * 5 + m / 12) % 60;
    bool hot = mode_ == Mode::Chime || (mode_ == Mode::Over && won_) || (live && onHour());
    int until = framesUntilHour();
    if (live && until > 0 && until <= kFpc * 12) hot = true;
    float swing = std::sin(bellPh_) * (hot ? 8.f * bellAmp_ : 1.6f);
    spr(art_.hand[2][s % 60], cx, cy, size, size, PAL_HAND);
    spr(art_.hand[1][m % 60], cx, cy, size, size, PAL_HAND);
    spr(art_.hand[0][hi], cx, cy, size, size, PAL_HAND);
    spr(art_.cap, cx, cy, size * 0.16f, size * 0.16f, PAL_HAND);
    spr(art_.face, cx, cy, size, size, PAL_FACE);
    if (hot) spr(art_.ring, cx, cy, size * 1.22f, size * 1.22f, PAL_GOLD);
    float by = cy + size * 0.62f;
    spr(art_.clapper, cx - 18.f + swing, by + 5.f, 6.f, 9.f, PAL_BELL);
    spr(art_.bell, cx - 18.f + swing * 0.55f, by, 18.f, 22.f, PAL_BELL);
    spr(art_.clapper, cx + 18.f - swing, by + 5.f, 6.f, 9.f, PAL_BELL, true);
    spr(art_.bell, cx + 18.f - swing * 0.55f, by, 18.f, 22.f, PAL_BELL, true);
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        int boost = flash_ > 0 && y < 110 ? 2 : 0;
        if (y < int(kFloor) - 8) {
            float u = y / float(kFloor);
            int r = rgbClamp(int(2 + u * 8) + boost);
            int g = rgbClamp(int(2 + u * 3));
            int b = rgbClamp(int(6 - u * 3) + (flash_ > 0 ? 1 : 0));
            if (mode_ == Mode::Chime || (mode_ == Mode::Over && won_)) {
                r = rgbClamp(r + 2);
                g = rgbClamp(g + 1);
            }
            v.lineBackdrop[y] = gs::rgb4(r, g, b);
        } else if (y < int(kFloor)) {
            v.lineBackdrop[y] = gs::rgb4(3, 2, 3);
        } else {
            int w = ((y / 4) & 1) ? 5 : 4;
            v.lineBackdrop[y] = gs::rgb4(w, w - 1, 3);
        }
    }
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.hudEnabled = true;
    backdrop();

    const Mode view = mode_ == Mode::Pause ? held_ : mode_;
    const bool victory = view == Mode::Chime || (view == Mode::Over && won_);
    const bool aiming = view == Mode::Title || view == Mode::Aim;

    auto word = [&](const gs::Image& img, float x, float y, int pal) {
        spr(img, x, y, float(img.w), float(img.h), pal);
    };

    if (view == Mode::Title) {
        word(art_.hoopW, 196.f, 40.f, PAL_GOLD);
        word(art_.chimeW, 200.f, 72.f, PAL_WIN);
    } else if (victory) {
        word(art_.chimeW, 196.f, 40.f, PAL_WIN);
        word(art_.twelveW, 196.f, 68.f, PAL_GOLD);
    } else if (view == Mode::Call || view == Mode::Fail || (view == Mode::Over && !won_)) {
        const gs::Image* img = wordFor(reason_);
        word(*img, 196.f, 42.f, PAL_ALERT);
    }

    if (aiming && (view == Mode::Aim)) {
        float vx, vy, vz;
        if (solveArc(feet_ + kHand, aimZ_, vx, vy, vz)) {
            float sc = speedScale(meter_);
            vx *= sc;
            vy *= sc;
            vz *= sc;
            float x = feet_ + kHand;
            float y = kReleaseY;
            float z = 0.f;
            int pal = hint_ == Kind::Swish ? PAL_GREEN : PAL_GOLD;
            for (int i = 0; i < 14; i++) {
                float h = 0.06f;
                y += vy * h - 0.5f * kG * h * h;
                vy -= kG * h;
                x += vx * h;
                z += vz * h;
                if (y < 0.4f || x > kBoardX) break;
                spr(art_.pip, scrX(x), scrY(y) + z * 8.f, 4.f, 4.f, pal);
            }
        }
        float nx = kMeterX + clampf(meter_, 0.f, 1.f) * kMeterW;
        int npal = hint_ == Kind::Swish ? PAL_GREEN : PAL_GOLD;
        spr(art_.blot, nx, kMeterY, 4.f, 12.f, npal);
        float sweetW = (kSweetHi - kSweetLo) * kMeterW;
        float sweetX = kMeterX + (kSweetLo + kSweetHi) * 0.5f * kMeterW;
        spr(art_.blot, sweetX, kMeterY, sweetW, 8.f, PAL_GREEN);
        spr(art_.blot, kMeterX + kMeterW * 0.5f, kMeterY, kMeterW, 5.f, PAL_METER);
    }

    int spin = (int(tr_.flight * 14.f) + int(anim_ * 0.15f)) & 1;
    float ballH = 16.f;
    sprM(art_.ball[spin], scrX(ball_.x), scrY(ball_.y) + ball_.z * 8.f, ballH, PAL_BALL);

    float rimSy = scrY(kRimY);
    int netFrame = (victory ? int(bellPh_ * 2.f) : int(anim_ * 0.08f)) & 1;
    spr(art_.net[netFrame], scrX(kRimX) - 1.f, rimSy + 16.f, 32.f, 28.f, PAL_IRON);
    spr(art_.rim, scrX(kRimX) - 2.f, rimSy, 46.f, 15.f, PAL_IRON);

    bool showGlass = aiming || view == Mode::Flight || view == Mode::Chime;
    if (showGlass) {
        if (aiming) {
            float ax = clampf(kGlassX + aimZ_ * 36.f, kGlassX - 16.f, kGlassX + 16.f);
            int pal = PAL_GOLD;
            if (view == Mode::Aim) {
                if (hint_ == Kind::Swish) pal = PAL_GREEN;
                else if (hint_ == Kind::Short || hint_ == Kind::Hot || hint_ == Kind::Wide) pal = PAL_ALERT;
            }
            spr(art_.cross, ax, kGlassY, 13.f, 13.f, pal);
        }
        spr(art_.glass, kGlassX, kGlassY, kGlassS, kGlassS, PAL_GLASS);
    }

    float ph = 60.f;
    float py = kFloor - ph * 0.5f;
    if (view == Mode::Flight && flightFrames_ < 18) py -= std::sin(flightFrames_ / 18.f * kPi) * 8.f;
    if (victory) py -= std::fabs(std::sin(anim_ * 0.18f)) * 5.f;
    sprM(art_.player, scrX(feet_), py, ph, PAL_YOU, false);

    spr(art_.board, scrX(kBoardX) + 8.f, rimSy - 6.f, 50.f, 36.f, PAL_BOARD);
    spr(art_.arm, scrX(kRimX) + 18.f, rimSy + 1.f, 26.f, 5.f, PAL_IRON);
    float poleTop = rimSy + 6.f;
    float poleH = std::max(8.f, kFloor - poleTop);
    spr(art_.pole, scrX(kBoardX) + 10.f, (poleTop + kFloor) * 0.5f, 10.f, poleH, PAL_IRON);

    clockAt(kClockX, kClockY, kClockS, view != Mode::Title);
    spr(art_.school, kClockX + 8.f, kClockY + 46.f, 78.f, 108.f, PAL_TOWER);
    spr(art_.moon, 300.f, 36.f, 18.f, 18.f, PAL_COURT);

    static const float stars[][2] = {{18.f, 14.f}, {96.f, 18.f}, {210.f, 12.f}, {274.f, 22.f}, {138.f, 16.f}};
    for (const auto& s : stars) spr(art_.pip, s[0], s[1], 3.f, 3.f, PAL_COURT);

    for (int i = 0; i < kShots; i++) {
        bool live = i >= shots_;
        if (view == Mode::Flight && i == shots_ - 1) live = true;
        float px = 286.f + float(i) * 12.f;
        if (live) sprM(art_.ball[0], px, 14.f, 10.f, PAL_BALL);
        else spr(art_.pip, px, 14.f, 6.f, 6.f, PAL_COURT);
    }

    spr(art_.blot, scrX((kRimX + kBotX) * 0.5f), kFloor, 2.f, 10.f, PAL_COURT);
    spr(art_.blot, scrX(kRimX) - 18.f, kFloor, 36.f, 3.f, PAL_COURT);
    spr(art_.shadow, scrX(ball_.x), kFloor + 3.f, clampf(16.f - ball_.y * 2.2f, 6.f, 16.f), 5.f, PAL_COURT, false, true);
    spr(art_.shadow, scrX(feet_), kFloor + 4.f, 22.f, 6.f, PAL_COURT, false, true);

    char buf[48];
    if (view == Mode::Title) {
        hudC(0, "S3 HOOPCHIME", PAL_GOLD);
        hudC(1, "THE HOUR HAS TO CHIME", PAL_INK);
        hudC(25, "EARLY IS WAVED OFF", PAL_ALERT);
        hudC(26, "L-R RANGE   U-D AIM   Z SHOOT", PAL_INK);
        hudC(27, "ENTER STARTS", PAL_GREEN);
        return;
    }

    int h, m, s;
    split(h, m, s);
    std::snprintf(buf, sizeof buf, "%d:%02d:%02d", h, m, s);
    hud(32, 0, buf, onHour() || victory ? PAL_WIN : PAL_GOLD);
    hud(1, 0, "S3 HOOPCHIME", PAL_GOLD);

    if (victory) {
        hudC(24, "THE HOUR CHIMES", PAL_WIN);
        std::snprintf(buf, sizeof buf, "SHOT %d", std::max(1, std::min(shots_, kShots)));
        hudC(25, buf, PAL_GOLD);
        hudC(26, "TWELVE BELLS", PAL_GOLD);
        if (view == Mode::Over && !bot_) hudC(27, "ENTER PLAYS AGAIN", PAL_INK);
        return;
    }
    if (view == Mode::Fail || (view == Mode::Over && !won_)) {
        hudC(24, "THE HOUR DID NOT CHIME", PAL_ALERT);
        hudC(25, reason_[0] ? reason_ : "LATE", PAL_ALERT);
        if (!bot_) hudC(27, "ENTER PLAYS AGAIN", PAL_INK);
        return;
    }
    if (mode_ == Mode::Pause) {
        hudC(24, "PAUSED", PAL_GOLD);
        hudC(27, "ENTER RESUMES    ESC TITLE", PAL_INK);
        return;
    }

    int show = shots_;
    if (view == Mode::Aim) show = std::min(kShots, shots_ + 1);
    if (show < 1) show = 1;
    std::snprintf(buf, sizeof buf, "SHOT %d", show);
    hud(1, 2, buf, PAL_INK);

    const char* call = "LINE";
    int cpal = PAL_INK;
    if (view == Mode::Flight) {
        call = "UP";
        cpal = PAL_GOLD;
    } else if (view == Mode::Call) {
        call = reason_[0] ? reason_ : "MISS";
        cpal = PAL_ALERT;
    } else if (view == Mode::Aim) {
        bool window = false;
        if (bot_) window = wasWindow_;
        else {
            Forecast f = forecast(feet_, aimZ_, meter_);
            window = f.kind == Kind::Swish && inWindow(f.score, framesUntilHour());
        }
        if (window && hint_ == Kind::Swish) {
            call = "NOW";
            cpal = PAL_WIN;
        } else if (hint_ == Kind::Swish) {
            call = "WAIT";
            cpal = PAL_GOLD;
        } else {
            call = kindName(hint_);
            cpal = PAL_ALERT;
        }
    }
    hud(8, 2, call, cpal);
    if (view == Mode::Call && !std::strcmp(reason_, "EARLY")) hudC(26, "WAVED OFF", PAL_ALERT);
    else hudC(26, "L-R RANGE  U-D AIM  Z SHOOT", PAL_INK);
    hudC(27, "CLEAN ON TWELVE", onHour() ? PAL_WIN : PAL_INK);
}

}  // namespace hoopchime

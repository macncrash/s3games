#include "game/wicketchime.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace wicketchime {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;

constexpr float kStumpX = 198.f;
constexpr float kStumpY = 96.f;
constexpr float kStumpW = 58.f;
constexpr float kStumpH = 74.f;
constexpr float kBowlX = 196.f;
constexpr float kBowlY = 168.f;
constexpr float kBowlW = 52.f;
constexpr float kBowlH = 74.f;
constexpr float kBatX = 236.f;
constexpr float kBatY = 104.f;
constexpr float kKeepX = 152.f;
constexpr float kKeepY = 108.f;
constexpr float kClockX = 50.f;
constexpr float kClockY = 62.f;
constexpr float kClockS = 52.f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

uint16_t mix(uint16_t a, uint16_t b, float t) {
    t = clampf(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

gs::FMPatch bellPatch() {
    gs::FMPatch p;
    p.alg = 6;
    p.fb = 0.16f;
    p.op[0] = {1.0f, 1.0f, 0.006f, 0.48f, 0.18f, 0.42f};
    p.op[1] = {2.4f, 0.24f, 0.004f, 0.30f, 0.0f, 0.22f};
    p.op[2] = {3.8f, 0.12f, 0.004f, 0.16f, 0.0f, 0.14f};
    p.op[3] = {1.5f, 0.20f, 0.008f, 0.50f, 0.08f, 0.36f};
    p.vol = 0.30f;
    p.echo = 0.24f;
    p.tone = 1700.f;
    return p;
}

}  // namespace

Game::Call Game::judge(float aim, float meter) {
    if (std::fabs(aim) > kWide) return Call::Wide;
    if (meter < kSweetLo) return Call::Short;
    if (meter > kSweetHi) return Call::Full;
    if (std::fabs(aim) > kStump) return Call::Edge;
    return Call::Timber;
}

const char* Game::callName(Call c) {
    switch (c) {
    case Call::Timber: return "TIMBER";
    case Call::Wide: return "WIDE";
    case Call::Edge: return "EDGE";
    case Call::Short: return "SHORT";
    case Call::Full: return "FULL";
    }
    return "OPEN";
}

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

int Game::secAt(int frames) const {
    if (frames < 0) frames = 0;
    int t = kStartSec + frames / kFpc;
    return t < 0 ? 0 : t;
}

int Game::untilAt(int frames) const {
    if (frames < 0) frames = 0;
    int sec = secAt(frames);
    int sub = frames % kFpc;
    if (sec > kHourSec) return -((sec - kHourSec) * kFpc + sub);
    if (sec == kHourSec) return -sub;
    int secLeft = kHourSec - sec;
    return (secLeft - 1) * kFpc + (kFpc - sub);
}

int Game::clockSec() const { return secAt(playFrames_); }
int Game::framesUntilHour() const { return untilAt(playFrames_); }
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

void Game::faceTime(int& h, int& m, int& s) const {
    if (mode_ == Mode::Title || (mode_ == Mode::Pause && held_ == Mode::Title)) {
        h = 11;
        m = 58;
        s = (titleFrames_ / kFpc) % 60;
        return;
    }
    split(h, m, s);
}

bool Game::audit() {
    auto bad = [](const char* why) {
        std::fprintf(stderr, "s3wicketchime rules %s\n", why);
        return false;
    };
    float mid = (kSweetLo + kSweetHi) * 0.5f;
    if (judge(0.f, mid) != Call::Timber) return bad("timber");
    if (judge(kStump, mid) != Call::Timber) return bad("lip");
    if (judge(kStump + 0.2f, mid) != Call::Edge) return bad("edge");
    if (judge(kWide, mid) != Call::Edge) return bad("wide lip");
    if (judge(kWide + 0.2f, mid) != Call::Wide) return bad("wide");
    if (judge(0.f, kSweetLo) != Call::Timber) return bad("sweet lo");
    if (judge(0.f, kSweetLo - 0.02f) != Call::Short) return bad("short");
    if (judge(0.f, kSweetHi) != Call::Timber) return bad("sweet hi");
    if (judge(0.f, kSweetHi + 0.02f) != Call::Full) return bad("full");
    int span = (kHourSec - kStartSec) * kFpc;
    if (untilAt(0) != span) return bad("span");
    if (onHourAt(0)) return bad("opened early");
    int release = span - kFlight;
    if (untilAt(release) != kFlight) return bad("release");
    if (onHourAt(release)) return bad("release hour");
    int arrive = release + kFlight;
    if (!onHourAt(arrive) || secAt(arrive) != kHourSec) return bad("noon");
    if ((secAt(arrive) / 60) % 60 != 0 || secAt(arrive) % 60 != 0) return bad("hands");
    if (onHourAt(release - 30 * kFpc + kFlight)) return bad("early ball");
    int last = (kHourSec + kGraceSec - 1 - kStartSec) * kFpc;
    if (!onHourAt(last) || !onHourAt(last + kFpc - 1)) return bad("grace end");
    if (onHourAt(last + kFpc)) return bad("grace long");
    if (kBalls != 3 || kFlight < 12) return bad("shape");
    rules_ = true;
    return true;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = true;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(8, 11, 14));
    sys.apu.setMaster(0.72f);
    sys.apu.setEcho(0.16f, 0.26f, 0.12f);
    sys.apu.setPatch(0, bellPatch());
    sys.apu.setPan(0, -0.15f);
    showTitle();
    if (!audit()) std::fprintf(stderr, "s3wicketchime rules failed\n");
}

void Game::showTitle() {
    mode_ = Mode::Title;
    titleFrames_ = 0;
    playFrames_ = 0;
    over_ = false;
    won_ = false;
    wicket_ = false;
    balls_ = 0;
    reason_ = "";
    aim_ = 0.f;
    meter_ = (kSweetLo + kSweetHi) * 0.5f;
    meterDir_ = 1.f;
    lastSec_ = -1;
    strikes_ = 0;
    flightFrames_ = 0;
    callFrames_ = 0;
    chimeFrames_ = 0;
    failFrames_ = 0;
    bellAmp_ = 0.16f;
    fate_ = Call::Timber;
    if (sys_) sys_->apu.keyOff(0);
}

void Game::newGame() {
    over_ = false;
    won_ = false;
    wicket_ = false;
    balls_ = 0;
    playFrames_ = 0;
    strikes_ = 0;
    chimeFrames_ = 0;
    failFrames_ = 0;
    lastSec_ = -1;
    reason_ = "";
    bellAmp_ = 0.14f;
    if (sys_) {
        sys_->apu.keyOff(0);
        sys_->setLight(40, 120, 55);
    }
    beginAim();
}

void Game::beginAim() {
    mode_ = Mode::Aim;
    flightFrames_ = 0;
    callFrames_ = 0;
    if (bot_) {
        aim_ = 0.f;
        meter_ = (kSweetLo + kSweetHi) * 0.5f;
    }
}

void Game::handPos(int pose, float& x, float& y) const {
    float bx = 40.f, by = 40.f;
    if (pose == 1) {
        bx = 34.f;
        by = 12.f;
    } else if (pose == 2) {
        bx = 44.f;
        by = 20.f;
    }
    x = kBowlX + (bx - 24.f) * (kBowlW / 48.f);
    y = kBowlY + (by - 36.f) * (kBowlH / 72.f);
}

void Game::spot(Call c, float aim, float meter, float& x, float& y) const {
    x = kStumpX + aim;
    y = (kStumpY + 42.f) - meter * 50.f;
    if (c == Call::Timber) {
        x = kStumpX;
        y = kStumpY + 6.f;
    } else if (c == Call::Edge) {
        x = kStumpX + (aim >= 0.f ? 34.f : -34.f);
    } else if (c == Call::Wide) {
        x = kStumpX + (aim >= 0.f ? 62.f : -62.f);
    } else if (c == Call::Full) {
        y = kStumpY - 4.f;
    } else if (c == Call::Short) {
        y = kStumpY + 34.f;
    }
}

void Game::ballAt(float u, float& x, float& y) const {
    u = clampf(u, 0.f, 1.f);
    float lob = 13.f;
    if (fate_ == Call::Short) lob = 24.f;
    else if (fate_ == Call::Full) lob = 6.f;
    else if (fate_ == Call::Timber) lob = 10.f;
    x = fromX_ + (toX_ - fromX_) * u;
    y = fromY_ + (toY_ - fromY_) * u - std::sin(u * kPi) * lob;
}

int Game::bowlPose(Mode view) const {
    if (view == Mode::Flight) {
        if (flightFrames_ < 5) return releasePose_;
        if (flightFrames_ < 18) return 2;
        return 0;
    }
    if (view == Mode::Aim || view == Mode::Title) return meter_ > 0.78f ? 1 : 0;
    if (view == Mode::Call || view == Mode::Chime || view == Mode::Over) return 0;
    return 0;
}

void Game::release() {
    if (mode_ != Mode::Aim || balls_ >= kBalls) return;
    fate_ = judge(aim_, meter_);
    releasePose_ = meter_ > 0.78f ? 1 : 0;
    handPos(releasePose_, fromX_, fromY_);
    spot(fate_, aim_, meter_, toX_, toY_);
    balls_++;
    flightFrames_ = 0;
    mode_ = Mode::Flight;
    if (!sys_) return;
    sys_->apu.noiseBurst(0.18f, 1700.f, 0.045f);
    blip(1, 210.f, 0.05f, 6);
    sys_->rumble(0.1f, 0.26f, 45);
}

void Game::beginChime() {
    if (won_) return;
    won_ = true;
    wicket_ = true;
    reason_ = "CHIME";
    mode_ = Mode::Chime;
    chimeFrames_ = 0;
    strikes_ = 0;
    bellAmp_ = 1.f;
    if (!sys_) return;
    sys_->apu.noiseBurst(0.42f, 640.f, 0.09f);
    blip(1, 120.f, 0.07f, 12);
    sys_->setLight(255, 196, 48);
    sys_->rumble(0.45f, 0.85f, 200);
}

void Game::beginCall() {
    mode_ = Mode::Call;
    callFrames_ = 0;
    if (fate_ == Call::Timber) {
        reason_ = "EARLY";
        if (sys_) {
            sys_->apu.noiseBurst(0.32f, 520.f, 0.07f);
            blip(1, 98.f, 0.06f, 10);
            sys_->setLight(170, 90, 30);
        }
    } else {
        reason_ = callName(fate_);
        blip(1, 80.f, 0.05f, 8);
        if (sys_) sys_->setLight(120, 50, 30);
    }
}

void Game::beginFail(const char* why) {
    reason_ = why ? why : "OPEN";
    won_ = false;
    wicket_ = false;
    mode_ = Mode::Fail;
    failFrames_ = 0;
    blip(1, 70.f, 0.06f, 14);
    if (sys_) sys_->setLight(140, 28, 24);
}

void Game::resolve() {
    if (sys_) sys_->apu.noise(0, 1000);
    if (fate_ == Call::Timber && onHour()) beginChime();
    else beginCall();
}

void Game::steer(const gs::Pad& pad) {
    float rate = 86.f;
    if (std::fabs(pad.axisX) > 0.22f) aim_ += pad.axisX * rate * kDt;
    else {
        if (pad.down(gs::BTN_LEFT)) aim_ -= rate * kDt;
        if (pad.down(gs::BTN_RIGHT)) aim_ += rate * kDt;
    }
    aim_ = clampf(aim_, -78.f, 78.f);
}

void Game::oscillate() {
    meter_ += meterDir_ * kDt / 1.15f;
    if (meter_ >= 1.f) {
        meter_ = 1.f;
        meterDir_ = -1.f;
    }
    if (meter_ <= 0.f) {
        meter_ = 0.f;
        meterDir_ = 1.f;
    }
}

void Game::blip(int ch, float freq, float vol, int hold) {
    if (!sys_ || ch < 0 || ch > 2) return;
    sys_->apu.tone(ch, freq, vol);
    toneUntil_[ch] = hold;
}

void Game::pumpAudio() {
    if (!sys_) return;
    for (int c = 0; c < 3; c++) {
        if (toneUntil_[c] > 0 && --toneUntil_[c] == 0) sys_->apu.tone(c, 0, 0);
    }
}

void Game::strikeBell(int n) {
    if (!sys_) return;
    static const float peal[] = {523.25f, 659.25f, 783.99f, 1046.5f, 783.99f, 659.25f,
                                 523.25f, 392.00f, 523.25f, 659.25f, 783.99f, 1046.5f};
    sys_->apu.keyOn(0, peal[n % 12]);
    bellAmp_ = 1.f;
}

void Game::tickClock() {
    int sec = (mode_ == Mode::Title) ? titleFrames_ / kFpc : clockSec();
    if (sec == lastSec_) return;
    int prev = lastSec_;
    lastSec_ = sec;
    if (prev < 0) return;
    if (mode_ == Mode::Chime || mode_ == Mode::Over || mode_ == Mode::Fail || mode_ == Mode::Pause) return;
    int until = framesUntilHour();
    bool urgent = mode_ != Mode::Title && until > 0 && until <= kFpc * 18;
    bool window = mode_ != Mode::Title && onHour();
    if (urgent || window) blip(0, (sec & 1) ? 880.f : 660.f, 0.045f, 3);
    else if ((sec % 10) == 0) blip(0, 196.f, 0.03f, 4);
}

void Game::spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool flip) {
    if (!sys_ || img.w == 0 || w < 1.f || h < 1.f) return;
    gs::Sprite s;
    s.w = int16_t(std::max(1, std::min(2000, (int)std::lround(w))));
    s.h = int16_t(std::max(1, std::min(2000, (int)std::lround(h))));
    s.x = int16_t(std::lround(cx - w * 0.5f));
    s.y = int16_t(std::lround(cy - h * 0.5f));
    s.img = img;
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::sprM(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, bool flip) {
    if (!sys_ || m.h < 1 || w < 1.f || h < 1.f) return;
    gs::Sprite s;
    s.w = int16_t(std::max(1, std::min(2000, (int)std::lround(w))));
    s.h = int16_t(std::max(1, std::min(2000, (int)std::lround(h))));
    s.x = int16_t(std::lround(cx - w * 0.5f));
    s.y = int16_t(std::lround(cy - h * 0.5f));
    s.img = m.pick(float(s.h));
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!sys_ || !s || row < 0 || row > 31) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = (unsigned char)s[i];
        if (c >= 'a' && c <= 'z') c = (unsigned char)(c - 32);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        int tile = art_.font[c - 32];
        if (!tile) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(tile, pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = s ? (int)std::strlen(s) : 0;
    hud(20 - n / 2, row, s, pal);
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    bool win = mode_ == Mode::Chime || (mode_ == Mode::Over && won_);
    bool dead = mode_ == Mode::Fail || (mode_ == Mode::Over && !won_ && reason_[0]);
    uint16_t zenith = win ? gs::rgb4(9, 7, 3) : dead ? gs::rgb4(3, 3, 5) : gs::rgb4(5, 9, 14);
    uint16_t sky = win ? gs::rgb4(14, 11, 5) : dead ? gs::rgb4(6, 4, 5) : gs::rgb4(10, 14, 15);
    uint16_t farG = win ? gs::rgb4(8, 10, 3) : dead ? gs::rgb4(3, 5, 3) : gs::rgb4(3, 8, 3);
    uint16_t nearG = win ? gs::rgb4(11, 12, 4) : dead ? gs::rgb4(4, 6, 3) : gs::rgb4(6, 12, 4);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.road[y].on = false;
        v.lineFog[y] = 0;
        if (y < 86) {
            v.lineBackdrop[y] = mix(zenith, sky, y / 86.f);
        } else {
            float u = (y - 86) / float(gs::SCREEN_H - 86);
            uint16_t g = mix(farG, nearG, u);
            if (((y / 6) & 1) == 0) g = mix(g, gs::rgb4(2, 6, 2), 0.22f);
            v.lineBackdrop[y] = g;
        }
    }
}

void Game::clockAt(float cx, float cy, float size) {
    int h, m, s;
    faceTime(h, m, s);
    int hi = ((h % 12) * 5 + m / 12) % 60;
    bool noon = mode_ == Mode::Chime || (mode_ == Mode::Over && won_) ||
                (mode_ != Mode::Title && !(mode_ == Mode::Pause && held_ == Mode::Title) && onHour());
    spr(art_.cap, cx, cy, size * 0.2f, size * 0.2f, PAL_HAND);
    spr(art_.hand[2][s % 60], cx, cy, size, size, PAL_HAND);
    spr(art_.hand[1][m % 60], cx, cy, size, size, PAL_HAND);
    spr(art_.hand[0][hi], cx, cy, size, size, PAL_HAND);
    if (noon) spr(art_.ring, cx, cy, size * 1.08f, size * 1.08f, PAL_GOLD);
    spr(art_.face, cx, cy, size, size, PAL_FACE);
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = true;
    v.hudEnabled = true;
    backdrop();

    Mode view = mode_ == Mode::Pause ? held_ : mode_;
    bool chiming = mode_ == Mode::Chime || (mode_ == Mode::Over && won_);
    int batPose = 0;
    if (view == Mode::Flight && flightFrames_ > kFlight / 2) {
        if (fate_ == Call::Full || fate_ == Call::Edge) batPose = 1;
        else if (fate_ == Call::Timber) batPose = 2;
    } else if (view == Mode::Call || chiming) {
        if (fate_ == Call::Timber || chiming) batPose = 2;
        else if (fate_ == Call::Full || fate_ == Call::Edge) batPose = 1;
    }
    int bowl = bowlPose(view);

    float bailK = 0.f;
    if (chiming) bailK = 1.f;
    else if (view == Mode::Call && fate_ == Call::Timber) {
        float u = clampf(callFrames_ / 42.f, 0.f, 1.f);
        bailK = u < 0.5f ? u * 2.f : std::max(0.f, 1.f - (u - 0.5f) * 2.f);
    }
    float rise = bailK * 22.f;
    float spread = bailK * 16.f;
    float shake = 0.f;
    if (mode_ == Mode::Chime) shake = std::sin(anim_ * 48.f) * std::max(0.f, 2.8f - chimeFrames_ * 0.07f);
    else if (view == Mode::Call && fate_ == Call::Timber) shake = std::sin(anim_ * 36.f) * (1.f - bailK) * 1.4f;

    bool showMeter = view == Mode::Aim;
    if (showMeter) {
        const float x0 = 86.f, x1 = 276.f, yb = 214.f;
        spr(art_.blot, (x0 + x1) * 0.5f, yb, x1 - x0, 5.f, PAL_SHADE);
        float sw = (kSweetHi - kSweetLo) * (x1 - x0);
        float sx = x0 + (kSweetLo + kSweetHi) * 0.5f * (x1 - x0);
        spr(art_.blot, sx, yb, sw, 10.f, PAL_GREEN);
        float nu = clampf(meter_, 0.f, 1.f);
        spr(art_.blot, x0 + nu * (x1 - x0), yb, 4.f, 16.f, PAL_GOLD);
    }

    if (view == Mode::Title) {
        float tw = float(art_.title.w);
        float th = float(art_.title.h);
        spr(art_.title, 206.f, 16.f, tw, th, PAL_GOLD);
    }

    Call live = (view == Mode::Flight || view == Mode::Call || view == Mode::Chime || (view == Mode::Over && balls_ > 0))
                    ? fate_
                    : judge(aim_, meter_);
    if (view == Mode::Title || view == Mode::Aim || view == Mode::Flight) {
        float dx, dy;
        float useM = (view == Mode::Flight) ? meter_ : meter_;
        float useA = aim_;
        spot(live, useA, useM, dx, dy);
        int dotPal = live == Call::Timber ? PAL_GREEN : (live == Call::Edge ? PAL_GOLD : PAL_ALERT);
        spr(art_.blot, dx, dy + 6.f, live == Call::Timber ? 12.f : 8.f, 4.f, dotPal);
    }

    float bx = 0.f, by = 0.f, bh = 14.f;
    bool showBall = true;
    if (view == Mode::Aim || view == Mode::Title) {
        handPos(bowl, bx, by);
        if (view == Mode::Title) by += std::sin(anim_ * 3.f) * 1.6f;
        bh = 14.f;
    } else if (view == Mode::Flight) {
        ballAt(flightFrames_ / float(kFlight), bx, by);
        float u = clampf(flightFrames_ / float(kFlight), 0.f, 1.f);
        bh = 15.f - u * 6.f;
    } else if (view == Mode::Call || chiming || (view == Mode::Over && balls_ > 0)) {
        bx = chiming ? kStumpX : toX_;
        by = chiming ? kStumpY + 12.f : toY_;
        bh = chiming ? 9.f : 11.f;
    } else {
        showBall = false;
    }
    if (showBall) {
        int seam = int(anim_ * 14.f) & 1;
        spr(art_.shadow, bx + 2.f, by + 8.f, bh * 0.9f, bh * 0.4f, PAL_SHADE);
        spr(art_.ball[seam], bx, by, bh, bh, PAL_BALL);
    }

    float top = kStumpY - kStumpH * 0.5f;
    spr(art_.bail, kStumpX - 12.f - spread + shake, top + 8.f - rise, 22.f, 8.f, PAL_WOOD);
    spr(art_.bail, kStumpX + 12.f + spread + shake, top + 8.f - rise, 22.f, 8.f, PAL_WOOD, true);
    sprM(art_.batsman[batPose], kBatX, kBatY, 48.f, 70.f, PAL_BAT);
    sprM(art_.bowler[bowl], kBowlX, kBowlY, kBowlW, kBowlH, PAL_BOWL);
    sprM(art_.keeper, kKeepX, kKeepY, 36.f, 48.f, PAL_BAT);
    sprM(art_.stumps, kStumpX + shake, kStumpY, kStumpW, kStumpH, PAL_WOOD);

    spr(art_.shadow, kBowlX, kBowlY + kBowlH * 0.42f, 36.f, 10.f, PAL_SHADE);
    spr(art_.shadow, kBatX, kBatY + 30.f, 30.f, 8.f, PAL_SHADE);
    spr(art_.shadow, kStumpX, kStumpY + kStumpH * 0.42f, 40.f, 8.f, PAL_SHADE);

    sprM(art_.pitch, 200.f, 156.f, 220.f, 120.f, PAL_PITCH);
    sprM(art_.rope, 108.f, 150.f, 16.f, 48.f, PAL_SKY);
    sprM(art_.rope, 292.f, 146.f, 16.f, 48.f, PAL_SKY, true);
    sprM(art_.screen, kStumpX, 78.f, 132.f, 42.f, PAL_SKY);
    sprM(art_.crowd, 150.f, 70.f, 96.f, 20.f, PAL_CROWD);
    sprM(art_.crowd, 268.f, 72.f, 88.f, 18.f, PAL_CROWD);
    sprM(art_.house, 292.f, 96.f, 78.f, 52.f, PAL_CROWD);
    sprM(art_.tree, 118.f, 62.f, 40.f, 36.f, PAL_TREE);
    sprM(art_.tree, 304.f, 58.f, 44.f, 40.f, PAL_TREE, true);

    float wag = std::sin(bellPh_) * 7.f * bellAmp_;
    clockAt(kClockX, kClockY, kClockS);
    spr(art_.bell, kClockX - 16.f + wag, kClockY + 30.f, 14.f, 16.f, PAL_TOWER);
    spr(art_.bell, kClockX + 16.f - wag * 0.8f, kClockY + 30.f, 14.f, 16.f, PAL_TOWER, true);
    sprM(art_.tower, kClockX, 108.f, 80.f, 128.f, PAL_TOWER);
    sprM(art_.cloud, 150.f + std::sin(cloud_) * 8.f, 18.f, 40.f, 16.f, PAL_SKY);
    sprM(art_.cloud, 250.f + std::sin(cloud_ * 0.8f) * 5.f, 28.f, 32.f, 13.f, PAL_SKY);
    sprM(art_.sun, 300.f, 20.f, 18.f, 18.f, PAL_SKY);

    char buf[48];
    int h, m, s;
    faceTime(h, m, s);
    if (mode_ == Mode::Title) {
        hudC(26, "THE HOUR HAS TO CHIME", PAL_GOLD);
        hudC(27, "LEFT RIGHT AIM   Z BOWLS   START", PAL_INK);
    } else if (mode_ == Mode::Pause) {
        hudC(12, "PAUSED", PAL_GOLD);
        hudC(27, "START RESUMES", PAL_INK);
    } else if (chiming || (mode_ == Mode::Over && won_)) {
        std::snprintf(buf, sizeof buf, "%d:%02d:%02d", h, m, s);
        hud(1, 0, "S3 WICKETCHIME", PAL_GOLD);
        hud(40 - (int)std::strlen(buf), 0, buf, PAL_GOLD);
        std::snprintf(buf, sizeof buf, "BALL %d", balls_);
        hud(1, 1, buf, PAL_INK);
        hudC(26, "THE HOUR CHIMES", PAL_GOLD);
        if (mode_ == Mode::Over && !bot_) hudC(27, "START AGAIN", PAL_GOLD);
    } else if (mode_ == Mode::Fail || (mode_ == Mode::Over && !won_)) {
        hud(1, 0, "S3 WICKETCHIME", PAL_ALERT);
        hud(1, 1, reason_[0] ? reason_ : "OPEN", PAL_ALERT);
        bool late = pastHour() || std::strcmp(reason_, "LATE") == 0;
        hudC(26, late ? "THE HOUR PASSED" : "NO CHIME", PAL_ALERT);
        if (mode_ == Mode::Over && !bot_) hudC(27, "START AGAIN", PAL_GOLD);
    } else {
        std::snprintf(buf, sizeof buf, "%d:%02d:%02d", h, m, s);
        int until = framesUntilHour();
        int arrival = until - kFlight;
        bool window = arrival <= 0 && arrival > -(kGraceSec * kFpc);
        bool late = arrival <= -(kGraceSec * kFpc);
        bool hot = window || (until > 0 && until <= kFpc * 12);
        hud(1, 0, "S3 WICKETCHIME", PAL_GOLD);
        hud(40 - (int)std::strlen(buf), 0, buf, hot ? PAL_GOLD : PAL_INK);
        int shown = balls_;
        if (view == Mode::Aim) shown = balls_ + 1;
        if (shown < 1) shown = 1;
        if (shown > kBalls) shown = kBalls;
        std::snprintf(buf, sizeof buf, "BALL %d/%d", shown, kBalls);
        hud(1, 1, buf, balls_ >= 2 ? PAL_ALERT : PAL_INK);
        if (until > 0 && !window) {
            int show = (until + kFpc - 1) / kFpc;
            std::snprintf(buf, sizeof buf, "HOUR IN %d", show);
            hud(28, 1, buf, hot ? PAL_GOLD : PAL_INK);
        } else if (window) {
            hud(28, 1, "ON THE HOUR", PAL_GOLD);
        } else {
            hud(31, 1, "TOO LATE", PAL_ALERT);
        }
        int namePal = live == Call::Timber ? PAL_GREEN : PAL_ALERT;
        hud(1, 2, callName(live), namePal);
        if (view == Mode::Aim) {
            if (window) hud(30, 2, "BOWL NOW", PAL_GOLD);
            else if (late) hud(30, 2, "TOO LATE", PAL_ALERT);
            else hud(34, 2, "WAIT", PAL_INK);
        } else if (view == Mode::Call && fate_ == Call::Timber) {
            hudC(26, "SHORT OF THE HOUR", PAL_ALERT);
            hudC(27, "BAILS PUT BACK", PAL_GOLD);
        } else if (view == Mode::Call) {
            hudC(26, callName(fate_), PAL_ALERT);
            hudC(27, balls_ >= 2 ? "ONE BALL LEFT" : "STILL SHORT OF THE HOUR", PAL_INK);
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    anim_ += kDt;
    cloud_ += kDt * 0.35f;
    bool chiming = mode_ == Mode::Chime || (mode_ == Mode::Over && won_);
    bellPh_ += kDt * (chiming ? 13.f : 2.2f);
    if (chiming) bellAmp_ = std::max(0.5f, bellAmp_ - kDt * 0.22f);
    else if (mode_ != Mode::Title && onHour()) bellAmp_ = 0.42f;
    else bellAmp_ = 0.14f;

    pumpAudio();
    bool live = mode_ == Mode::Aim || mode_ == Mode::Flight || mode_ == Mode::Call;
    if (live) playFrames_++;
    if (mode_ == Mode::Title) titleFrames_++;
    tickClock();

    const gs::Pad& pad = sys.pad;
    bool bowl = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_TURBO);
    bool start = pad.pressed(gs::BTN_START);
    bool back = pad.pressed(gs::BTN_MODE);
    if (bot_) {
        bowl = false;
        start = false;
        back = false;
    }

    if (mode_ == Mode::Pause) {
        if (start) mode_ = held_;
        else if (back) showTitle();
        draw();
        return;
    }
    if (back && mode_ == Mode::Title) {
        sys.quit();
        draw();
        return;
    }
    if (back && mode_ != Mode::Chime && mode_ != Mode::Over && mode_ != Mode::Fail) {
        showTitle();
        draw();
        return;
    }
    if (start && (mode_ == Mode::Aim || mode_ == Mode::Flight || mode_ == Mode::Call)) {
        held_ = mode_;
        mode_ = Mode::Pause;
        draw();
        return;
    }

    if (mode_ == Mode::Title) {
        oscillate();
        if (bot_) {
            if (titleFrames_ >= 40) newGame();
        } else if (start || bowl) {
            newGame();
        }
    } else if (mode_ == Mode::Aim) {
        if (pastHour() || balls_ >= kBalls) {
            beginFail(pastHour() ? "LATE" : "OPEN");
        } else if (bot_) {
            aim_ = 0.f;
            meter_ = (kSweetLo + kSweetHi) * 0.5f;
            int until = framesUntilHour();
            int arrival = until - kFlight;
            if (arrival <= 0 && arrival > -(kGraceSec * kFpc)) release();
        } else {
            steer(pad);
            oscillate();
            if (bowl) release();
        }
    } else if (mode_ == Mode::Flight) {
        flightFrames_++;
        if (flightFrames_ >= kFlight) resolve();
    } else if (mode_ == Mode::Call) {
        if (++callFrames_ > 42) {
            if (pastHour() || balls_ >= kBalls) {
                const char* why = pastHour() ? "LATE" : (fate_ == Call::Timber ? "EARLY" : callName(fate_));
                beginFail(why);
            } else {
                beginAim();
            }
        }
    } else if (mode_ == Mode::Chime) {
        chimeFrames_++;
        if (strikes_ < 12 && ((chimeFrames_ - 1) % 6) == 0) {
            strikeBell(strikes_);
            strikes_++;
        }
        if (chimeFrames_ >= 12 * 6 + 18) {
            mode_ = Mode::Over;
            over_ = true;
            sys.apu.keyOff(0);
            sys.apu.tone(0, 0, 0);
            sys.apu.tone(1, 0, 0);
        }
    } else if (mode_ == Mode::Fail) {
        if (++failFrames_ >= 64) {
            mode_ = Mode::Over;
            over_ = true;
        }
    } else if (mode_ == Mode::Over) {
        if (!bot_ && (start || bowl)) newGame();
    }

    if (mode_ == Mode::Flight) sys.apu.noise(0.018f, 1500.f, true);
    else sys.apu.noise(0, 1000);

    int until = framesUntilHour();
    if (mode_ == Mode::Chime || (mode_ == Mode::Over && won_)) sys.setLight(255, 190, 40);
    else if (mode_ == Mode::Fail || (mode_ == Mode::Over && !won_)) sys.setLight(140, 28, 24);
    else if (mode_ == Mode::Aim || mode_ == Mode::Flight) {
        int arrival = until - kFlight;
        bool window = arrival <= 0 && arrival > -(kGraceSec * kFpc);
        if (window || onHour()) sys.setLight(240, 180, 40);
        else if (until > 0 && until < kFpc * 18) sys.setLight(180, 130, 36);
        else sys.setLight(36, 120, 52);
    }
    draw();
}

}  // namespace wicketchime

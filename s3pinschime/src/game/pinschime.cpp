#include "game/pinschime.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace pinschime {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPocket = 0.12f;
constexpr float kDrive = 0.07f;
constexpr float kMiss = 0.88f;
constexpr float kZ0 = 3.85f;
constexpr float kReleaseZ = 1.21f;
constexpr float kStepZ = 0.08f;
constexpr float kPinX = 0.40f;
constexpr float kPinZ = 0.36f;
constexpr int kHorizon = 70;
constexpr float kFocal = 210.f;
constexpr float kLaneK = 0.60f;
constexpr float kClockX = 160.f;
constexpr float kClockY = 52.f;
constexpr float kClockS = 58.f;

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
    p.fb = 0.2f;
    p.op[0] = {1.0f, 1.0f, 0.003f, 0.48f, 0.12f, 0.42f};
    p.op[1] = {2.8f, 0.32f, 0.002f, 0.36f, 0.0f, 0.28f};
    p.op[2] = {4.6f, 0.16f, 0.002f, 0.2f, 0.0f, 0.18f};
    p.op[3] = {1.4f, 0.24f, 0.004f, 0.62f, 0.08f, 0.36f};
    p.vol = 0.3f;
    p.echo = 0.3f;
    p.tone = 1700.f;
    return p;
}

}  // namespace

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

int Game::pinsDown() const {
    int n = 0;
    for (const Pin& p : pin_)
        if (p.down) n++;
    return n;
}

float Game::meter() const {
    if (swingFrames_ <= 0) return 0.f;
    int p = swingFrames_ % kPeriod;
    if (p == 0) return 0.f;
    if (p <= kPeriod / 2) return p / float(kPeriod / 2);
    return (kPeriod - p) / float(kPeriod / 2);
}

float Game::aimX() const {
    Mode m = mode_ == Mode::Pause ? held_ : mode_;
    if (m == Mode::Swing) return clampf(stance_ + (meter() - 0.5f) * kMiss, -1.2f, 1.2f);
    if (m == Mode::Roll) return ballX_;
    return stance_;
}

bool Game::onPocket() const { return std::fabs(std::fabs(aimX()) - kPocket) <= kDrive; }

int Game::pose() const {
    Mode m = mode_ == Mode::Pause ? held_ : mode_;
    if (m == Mode::Swing) return 1;
    if (m == Mode::Roll && rollFrames_ < 8) return 2;
    if (m == Mode::Roll && rollFrames_ < 28) return 3;
    if ((m == Mode::Chime || m == Mode::Over) && won_) return 3;
    return 0;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(1, 1, 3));
    sys.apu.setMaster(0.8f);
    sys.apu.setEcho(0.2f, 0.32f, 0.16f);
    sys.apu.setPatch(0, bellPatch());
    sys.apu.setPan(0, 0.05f);
    showTitle();
    silence();
}

void Game::showTitle() {
    mode_ = Mode::Title;
    titleFrames_ = 0;
    playFrames_ = 0;
    over_ = false;
    won_ = false;
    balls_ = 3;
    reason_ = "";
    stance_ = 0;
    ballLive_ = false;
    gutter_ = false;
    resolved_ = false;
    wasPocket_ = false;
    wasStar_ = false;
    lastSec_ = -1;
    resetRack();
    silence();
    sys_->apu.keyOff(0);
}

void Game::newGame() {
    over_ = false;
    won_ = false;
    balls_ = 3;
    reason_ = "";
    stance_ = 0;
    playFrames_ = 0;
    swingFrames_ = 0;
    rollFrames_ = 0;
    resetFrames_ = 0;
    chimeFrames_ = 0;
    failFrames_ = 0;
    beep_ = 0;
    wasPocket_ = false;
    wasStar_ = false;
    gutter_ = false;
    resolved_ = false;
    ballLive_ = false;
    resetRack();
    lastSec_ = clockSec();
    mode_ = Mode::Approach;
    sys_->setLight(180, 120, 40);
}

void Game::resetRack() {
    int n = 0;
    for (int row = 0; row < 4; row++) {
        for (int i = 0; i <= row; i++, n++) {
            Pin& p = pin_[n];
            p.homeX = (float(i) - row * 0.5f) * kPinX;
            p.homeZ = kZ0 + row * kPinZ;
            p.x = p.homeX;
            p.z = p.homeZ;
            p.vx = p.vz = 0;
            p.side = 1;
            p.hour = n == 0;
            p.down = false;
            p.fall = 0;
        }
    }
}

void Game::beginSwing() {
    mode_ = Mode::Swing;
    swingFrames_ = 0;
    wasStar_ = false;
}

void Game::release() {
    float m = meter();
    ballX_ = clampf(stance_ + (m - 0.5f) * kMiss, -1.25f, 1.25f);
    ballZ_ = kReleaseZ;
    ballLive_ = true;
    gutter_ = std::fabs(ballX_) > 1.0f;
    resolved_ = false;
    rollFrames_ = 0;
    if (balls_ > 0) balls_--;
    mode_ = Mode::Roll;
    blip(196.f);
    sys_->apu.noiseBurst(0.16f, 1900.f, 0.05f);
    sys_->rumble(0.15f, 0.25f, 40);
}

void Game::topple(Pin& p, float vx, float vz) {
    if (p.down) return;
    p.down = true;
    p.vx = vx;
    p.vz = vz;
    p.side = vx < 0.f ? -1 : 1;
    if (p.fall < 0.08f) p.fall = 0.08f;
}

void Game::resolveHit() {
    bool pocket = !gutter_ && std::fabs(std::fabs(ballX_) - kPocket) <= kDrive;
    bool hit[10] = {};
    for (int i = 0; i < 10; i++) {
        float dx = pin_[i].homeX - ballX_;
        float dz = pin_[i].homeZ - kZ0;
        float reach = pin_[i].hour ? 0.30f : 0.24f;
        hit[i] = pocket || (!gutter_ && std::hypot(dx, dz) <= reach);
    }
    bool extra[10] = {};
    for (int i = 0; i < 10; i++) {
        if (!hit[i]) continue;
        for (int j = 0; j < 10; j++) {
            if (hit[j]) continue;
            float dx = pin_[i].homeX - pin_[j].homeX;
            float dz = pin_[i].homeZ - pin_[j].homeZ;
            if (std::hypot(dx, dz) < 0.42f) extra[j] = true;
        }
    }
    int fell = 0;
    for (int i = 0; i < 10; i++) {
        if (!hit[i] && !extra[i]) continue;
        float side = pin_[i].homeX >= ballX_ ? 1.f : -1.f;
        topple(pin_[i], side * (1.5f + 0.08f * i), 0.7f + 0.4f * (pin_[i].homeZ - kZ0));
        fell++;
    }
    if (fell) {
        sys_->apu.noiseBurst(std::min(0.7f, 0.22f + fell * 0.04f), 700.f, 0.12f);
        sys_->rumble(0.3f, 0.7f, 90);
    } else {
        blip(80.f);
        sys_->apu.noiseBurst(0.28f, 220.f, 0.14f);
    }
}

void Game::finishBall(const char* why) {
    ballLive_ = false;
    if (pin_[0].down && onHour()) {
        beginChime();
        return;
    }
    if (balls_ <= 0 || pastHour()) beginFail(why);
    else beginReset(why);
}

void Game::beginChime() {
    if (won_) return;
    won_ = true;
    reason_ = "CHIME";
    mode_ = Mode::Chime;
    chimeFrames_ = 0;
    ballLive_ = false;
    sys_->setLight(255, 190, 50);
    sys_->rumble(0.45f, 0.85f, 200);
}

void Game::beginReset(const char* why) {
    reason_ = why;
    mode_ = Mode::Reset;
    resetFrames_ = 0;
    ballLive_ = false;
    sys_->setLight(160, 40, 30);
}

void Game::beginFail(const char* why) {
    reason_ = why;
    won_ = false;
    mode_ = Mode::Fail;
    failFrames_ = 0;
    ballLive_ = false;
    blip(90.f);
    sys_->setLight(140, 20, 20);
}

void Game::settle(float dt) {
    for (Pin& p : pin_) {
        if (!p.down) continue;
        p.x += p.vx * dt;
        p.z += p.vz * dt;
        p.vx *= 0.9f;
        p.vz *= 0.9f;
        p.fall = std::min(1.f, p.fall + dt * 3.1f);
        p.x = clampf(p.x, -1.9f, 1.9f);
    }
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.06f);
    beep_ = std::max(beep_, 6);
}

void Game::strikeBell(int n) {
    static const float peal[] = {523.25f, 659.25f, 783.99f, 1046.5f};
    sys_->apu.keyOn(0, peal[n % 4]);
    sys_->apu.tone(1, peal[n % 4] * 0.5f, 0.04f);
    beep_ = 10;
}

void Game::silence() {
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(1, 0, 0);
    sys_->apu.tone(2, 0, 0);
}

void Game::tickSound() {
    int sec = clockSec();
    if (sec == lastSec_) return;
    int prev = lastSec_;
    lastSec_ = sec;
    if (prev < 0) return;
    int until = framesUntilHour();
    if (until > 0 && until <= kFpc * 15) blip((sec & 1) ? 880.f : 660.f);
    else if (sec % 60 == 0) blip(220.f);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    bool running = mode_ == Mode::Approach || mode_ == Mode::Swing || mode_ == Mode::Roll || mode_ == Mode::Reset;
    if (running) playFrames_++;
    if (mode_ == Mode::Title) titleFrames_++;
    if (running) tickSound();
    if (beep_ > 0 && mode_ != Mode::Chime) {
        if (--beep_ == 0) silence();
    }
    if (mode_ != Mode::Title) settle(kDt);

    const gs::Pad& pad = sys.pad;
    bool action = pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_TURBO);
    bool start = pad.pressed(gs::BTN_START);
    bool back = pad.pressed(gs::BTN_MODE);
    float slide = 0.f;
    if (!bot_) {
        if (pad.down(gs::BTN_LEFT)) slide -= 1.f;
        if (pad.down(gs::BTN_RIGHT)) slide += 1.f;
        if (std::fabs(pad.axisX) > 0.25f) slide = pad.axisX;
    }

    if (mode_ == Mode::Pause) {
        if (start) mode_ = held_;
        else if (back) showTitle();
        draw();
        return;
    }
    if (back && mode_ == Mode::Title) {
        if (!bot_) sys.quit();
        draw();
        return;
    }
    if (back && !bot_ && mode_ != Mode::Chime && mode_ != Mode::Over && mode_ != Mode::Fail) {
        showTitle();
        draw();
        return;
    }
    if (start && !bot_ && mode_ != Mode::Title && mode_ != Mode::Over && mode_ != Mode::Fail && mode_ != Mode::Chime &&
        mode_ != Mode::Reset) {
        held_ = mode_;
        mode_ = Mode::Pause;
        draw();
        return;
    }

    if ((mode_ == Mode::Approach || mode_ == Mode::Swing) && pastHour()) beginFail("LATE");

    if (mode_ == Mode::Title) {
        if (bot_) {
            if (titleFrames_ >= 30) newGame();
        } else if (start || action) {
            newGame();
        }
    } else if (mode_ == Mode::Over || mode_ == Mode::Fail) {
        if (mode_ == Mode::Fail && !over_ && ++failFrames_ >= 48) over_ = true;
        if (!bot_ && (start || action)) newGame();
    } else if (mode_ == Mode::Approach) {
        bool go = false;
        if (bot_) {
            float d = kPocket - stance_;
            if (std::fabs(d) > 0.008f) slide = clampf(d * 8.f, -1.f, 1.f);
            else if (framesUntilHour() == kLead) go = true;
        } else {
            go = action;
        }
        stance_ = clampf(stance_ + slide * 1.35f * kDt, -0.78f, 0.78f);
        if (bot_ && std::fabs(stance_ - kPocket) <= 0.02f) stance_ = kPocket;
        bool lined = std::fabs(std::fabs(stance_) - kPocket) <= kDrive;
        if (lined && !wasPocket_) blip(820.f);
        wasPocket_ = lined;
        sys.setLight(lined ? 40 : 170, lined ? 170 : 110, lined ? 60 : 40);
        if (go) beginSwing();
    } else if (mode_ == Mode::Swing) {
        swingFrames_++;
        bool go = action;
        if (bot_ && swingFrames_ == kStar && framesUntilHour() == kTravel) go = true;
        bool lined = onPocket();
        bool star = std::fabs(meter() - 0.5f) <= 0.08f;
        if (lined && star && !wasStar_) blip(980.f);
        wasStar_ = lined && star;
        if (lined && star) sys.setLight(80, 210, 90);
        if (go) release();
    } else if (mode_ == Mode::Roll) {
        rollFrames_++;
        ballZ_ = kReleaseZ + rollFrames_ * kStepZ;
        if (!bot_ && !gutter_ && rollFrames_ < kTravel) ballX_ = clampf(ballX_ + slide * 0.55f * kDt, -1.25f, 1.25f);
        if (std::fabs(ballX_) > 1.0f) gutter_ = true;
        if (rollFrames_ == kTravel && !resolved_) {
            resolved_ = true;
            resolveHit();
            const char* why = gutter_ ? "GUTTER" : "STANDING";
            if (pin_[0].down) why = pastHour() ? "LATE" : "EARLY";
            finishBall(why);
        }
        if (gutter_) ballX_ = std::copysign(1.12f, ballX_);
    } else if (mode_ == Mode::Reset) {
        if (++resetFrames_ >= 36) {
            if (pastHour() || balls_ <= 0) beginFail(pastHour() ? "LATE" : reason_);
            else {
                resetRack();
                ballLive_ = false;
                gutter_ = false;
                resolved_ = false;
                wasPocket_ = false;
                wasStar_ = false;
                mode_ = Mode::Approach;
            }
        }
    } else if (mode_ == Mode::Chime) {
        chimeFrames_++;
        if (chimeFrames_ <= 60 && (chimeFrames_ % 5) == 1) strikeBell(chimeFrames_ / 5);
        if (chimeFrames_ >= 96) {
            mode_ = Mode::Over;
            over_ = true;
            sys.apu.keyOff(0);
            silence();
        }
    }

    draw();
}

void Game::project(float x, float z, float& sx, float& sy, float& ppm) const {
    z = std::max(0.35f, z);
    float row = kFocal / z;
    sy = float(kHorizon) + row;
    ppm = row * kLaneK;
    sx = 160.f + x * ppm;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet) {
    if (h < 1.1f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::max(1, std::min(2000, (int)std::lround(w))));
    s.h = int16_t(std::max(1, std::min(2000, (int)std::lround(h))));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 80 || s.x + s.w < -80 || s.y > gs::SCREEN_H + 40 || s.y + s.h < -40) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, int fog, bool shadow) {
    if (w < 1.f || h < 1.f || m.h < 1) return;
    gs::Sprite s;
    s.w = int16_t(std::max(1, std::min(2000, (int)std::lround(w))));
    s.h = int16_t(std::max(1, std::min(2000, (int)std::lround(h))));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(std::max(w, h));
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::image(const gs::Image& img, float cx, float cy, float h, int pal) {
    if (h < 1.f || img.h == 0) return;
    float w = h * float(img.w) / float(img.h);
    gs::Sprite s;
    s.w = int16_t(std::max(1, std::min(2000, (int)std::lround(w))));
    s.h = int16_t(std::max(1, std::min(2000, (int)std::lround(h))));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = img;
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::text(const char* s, float x, float y, float scale, int pal) {
    if (!s || !s[0]) return;
    int n = (int)std::strlen(s);
    const float adv = 18.f * scale;
    x -= n * adv * 0.5f;
    for (int i = 0; i < n; i++) {
        unsigned char c = (unsigned char)s[i];
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + i * adv + adv * 0.5f, y, g.h * scale, pal, false);
    }
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = (unsigned char)s[i];
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = s ? (int)std::strlen(s) : 0;
    hud(20 - n / 2, row, s, pal);
}

void Game::clockAt(float cx, float cy, float size, bool live) {
    int h, m, s;
    split(h, m, s);
    if (!live) s = (titleFrames_ / 4) % 60;
    int hi = ((h % 12) * 5 + m / 12) % 60;
    image(art_.cap, cx, cy, size * 0.18f, PAL_HAND);
    image(art_.hand[2][s % 60], cx, cy, size, PAL_HAND);
    image(art_.hand[1][m % 60], cx, cy, size, PAL_HAND);
    image(art_.hand[0][hi], cx, cy, size, PAL_HAND);
    bool hot = mode_ == Mode::Chime || (mode_ == Mode::Over && won_) || (live && onHour());
    int until = framesUntilHour();
    if (live && until > 0 && until <= kFpc * 12) hot = true;
    if (hot) image(art_.ring, cx, cy, size * 1.12f, PAL_GOLD);
    image(art_.face, cx, cy, size, PAL_FACE);
}

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    bool win = mode_ == Mode::Chime || (mode_ == Mode::Over && won_);
    bool dead = mode_ == Mode::Fail || (mode_ == Mode::Over && !won_ && reason_[0]);
    uint16_t top = win ? gs::rgb4(6, 3, 1) : dead ? gs::rgb4(3, 0, 1) : gs::rgb4(1, 1, 5);
    uint16_t mid = win ? gs::rgb4(12, 7, 2) : dead ? gs::rgb4(6, 1, 1) : gs::rgb4(3, 2, 6);
    uint16_t hor = win ? gs::rgb4(14, 9, 3) : dead ? gs::rgb4(4, 1, 1) : gs::rgb4(8, 5, 2);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < kHorizon) {
            float u = y / float(kHorizon);
            v.lineBackdrop[y] = u < 0.55f ? mix(top, mid, u / 0.55f) : mix(mid, hor, (u - 0.55f) / 0.45f);
            v.lineFog[y] = 0;
            v.road[y].on = false;
            continue;
        }
        float row = float(y - kHorizon);
        float z = kFocal / std::max(8.f, row);
        gs::RoadLine& rd = v.road[y];
        rd.on = true;
        rd.cx = 160.f;
        rd.hw = row * kLaneK;
        rd.v = z * 34.f;
        rd.pal = PAL_LANE;
        rd.band = (int(z * 3.f) & 1) ? 1 : 0;
        rd.style = 1;
        rd.left = rd.right = 0;
        v.lineFog[y] = uint8_t(std::clamp(int((70.f - row) / 22.f), 0, 4));
        v.lineBackdrop[y] = gs::rgb4(2, 1, 2);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    sky();

    bool live = mode_ != Mode::Title;
    clockAt(kClockX, kClockY, kClockS, live);

    float swing = 0.f;
    if (mode_ == Mode::Chime || (mode_ == Mode::Over && won_)) swing = std::sin(chimeFrames_ * 0.5f) * 8.f;
    spr(art_.bell, kClockX - 30.f + swing, kClockY + kClockS * 0.55f, 22.f, PAL_BELL, false, 0, true);
    spr(art_.bell, kClockX + 30.f - swing, kClockY + kClockS * 0.55f, 22.f, PAL_BELL, true, 0, true);

    auto place = [&](float x, float z, float& sx, float& sy, float& ppm, int& fog) {
        project(x, z, sx, sy, ppm);
        fog = std::clamp(int((z - 3.2f) * 1.5f), 0, 4);
    };

    float feetRow = 136.f;
    float feetX = 160.f + stance_ * feetRow * kLaneK;
    float feetY = float(kHorizon) + feetRow;
    int bp = pose();
    bool hand = !ballLive_ && mode_ != Mode::Roll && mode_ != Mode::Reset && mode_ != Mode::Chime && mode_ != Mode::Fail &&
                mode_ != Mode::Over;
    if (hand) {
        float bx = feetX + 16.f, by = feetY - 36.f;
        if (bp == 1) {
            bx = feetX + 22.f;
            by = feetY - 18.f;
        } else if (bp == 2) {
            bx = feetX + 14.f;
            by = feetY - 58.f;
        }
        int spin = (mode_ == Mode::Title ? titleFrames_ : playFrames_) & 3;
        spr(art_.ball[spin], bx, by, 14.f, PAL_BALL, false);
    }

    float bsx = 0, bsy = 0, bppm = 0;
    int bfog = 0;
    bool showBall = ballLive_ || (mode_ == Mode::Chime && chimeFrames_ < 20);
    if (showBall) {
        float z = ballLive_ ? std::max(1.15f, ballZ_) : kZ0;
        place(ballX_, z, bsx, bsy, bppm, bfog);
        float dia = std::max(8.f, bppm * 0.36f);
        spr(art_.ball[(rollFrames_ / 2) & 3], bsx, bsy, dia, PAL_BALL, false, bfog, true);
    }
    spr(art_.bowler[bp], feetX, feetY, 68.f, PAL_BOWLER, false, 0, true);

    bool guide = mode_ == Mode::Title || mode_ == Mode::Approach || mode_ == Mode::Swing ||
                 (mode_ == Mode::Pause && (held_ == Mode::Approach || held_ == Mode::Swing));
    bool hot = onPocket() && mode_ != Mode::Title;
    float csx, csy, cppm;
    int cfog;
    if (guide) {
        for (int i = 0; i < 7; i++) {
            place(aimX(), 1.7f + i * 0.24f, csx, csy, cppm, cfog);
            spr(art_.dot, csx, csy, hot ? 7.f : 5.f, hot ? PAL_GREEN : PAL_GOLD, false, cfog);
        }
    }
    float pulse = 1.f + 0.1f * std::sin(playFrames_ * 0.2f);
    for (float side : {-1.f, 1.f}) {
        bool sideHot = hot && ((side > 0.f) == (aimX() >= 0.f));
        for (int i = 0; i < 3; i++) {
            place(side * kPocket, 2.15f + i * 0.16f, csx, csy, cppm, cfog);
            float ah = std::max(8.f, cppm * 0.22f) * (sideHot ? pulse : 1.f);
            spr(art_.arrow, csx, csy, ah, sideHot ? PAL_GREEN : PAL_AMBER, false, cfog, true);
        }
    }

    int order[10];
    for (int i = 0; i < 10; i++) order[i] = i;
    std::sort(order, order + 10, [&](int a, int b) { return pin_[a].z < pin_[b].z; });
    for (int k = 0; k < 10; k++) {
        const Pin& p = pin_[order[k]];
        float sx, sy, ppm;
        int fog;
        place(p.x, p.z, sx, sy, ppm, fog);
        float ph = std::max(12.f, ppm * 1.15f);
        int pal = p.hour ? PAL_BELL : PAL_CREAM;
        const gs::Mipped& up = p.hour ? art_.bellPin : art_.pin;
        const gs::Mipped& flat = p.hour ? art_.bellFlat : art_.pinFlat;
        if (p.fall <= 0.48f)
            spr(up, sx + p.side * p.fall * 4.f, sy, ph * (1.f - 0.18f * p.fall), pal, false, fog, true);
        else
            spr(flat, sx + p.side * 5.f, sy, ph * 0.42f, pal, p.side < 0, fog, true);
    }

    place(0.f, kZ0 + 1.15f, csx, csy, cppm, cfog);
    spr(art_.tower, csx, csy + 8.f, std::max(70.f, cppm * 3.1f), PAL_TOWER, false, cfog, true);
    spr(art_.lamp, 18.f, 96.f, 32.f, PAL_AMBER, false, 0, true);
    spr(art_.lamp, 302.f, 96.f, 32.f, PAL_AMBER, true, 0, true);

    if (showBall) stamp(art_.shadow, bsx, bsy, std::max(8.f, bppm * 0.3f), 4.f, PAL_INK, bfog, true);
    stamp(art_.shadow, feetX, feetY, 32.f, 8.f, PAL_INK, 0, true);
    for (int k = 0; k < 10; k++) {
        const Pin& p = pin_[order[k]];
        float sx, sy, ppm;
        int fog;
        place(p.x, p.z, sx, sy, ppm, fog);
        float ph = std::max(12.f, ppm * 1.15f);
        stamp(art_.shadow, sx, sy, ph * 0.5f, ph * 0.14f, PAL_INK, fog, true);
    }
    place(0.f, 1.55f, csx, csy, cppm, cfog);
    stamp(art_.foul, csx, csy, cppm * 2.05f, 3.f, PAL_RED, 0, false);

    char line[48];
    if (mode_ == Mode::Title) {
        hudC(0, "S3 PINSCHIME", PAL_GOLD);
        hudC(1, "THE HOUR HAS TO CHIME", PAL_INK);
        hudC(26, "HEAD PIN FALLS ON 12:00", PAL_GOLD);
        hudC(27, "ARROWS AIM   C SWING   START", PAL_INK);
    } else if (mode_ == Mode::Chime || (mode_ == Mode::Over && won_)) {
        hudC(0, "THE HOUR CHIMES", PAL_GOLD);
        hudC(1, "12:00:00", PAL_GOLD);
        hudC(26, "TWELVE BELLS", PAL_GOLD);
        if (mode_ == Mode::Over) hudC(27, "START", PAL_INK);
    } else if (mode_ == Mode::Fail || (mode_ == Mode::Over && !won_)) {
        hudC(0, "THE HOUR DID NOT CHIME", PAL_RED);
        std::snprintf(line, sizeof line, "%s", reason_);
        hudC(1, line, PAL_RED);
        int h, m, s;
        split(h, m, s);
        std::snprintf(line, sizeof line, "%d:%02d:%02d", h, m, s);
        hudC(26, line, PAL_AMBER);
        if (over_ || mode_ == Mode::Over) hudC(27, "START RETRY", PAL_INK);
    } else if (mode_ == Mode::Pause) {
        hudC(0, "PAUSED", PAL_GOLD);
        hudC(27, "START RESUME    ESC TITLE", PAL_INK);
    } else {
        hud(1, 0, "S3 PINSCHIME", PAL_GOLD);
        int shownBall = (mode_ == Mode::Roll || mode_ == Mode::Reset) ? 3 - balls_ : 4 - balls_;
        std::snprintf(line, sizeof line, "BALL %d", shownBall);
        hud(33, 0, line, PAL_AMBER);
        int h, m, s;
        split(h, m, s);
        std::snprintf(line, sizeof line, "%d:%02d:%02d", h, m, s);
        int tpal = onHour() ? PAL_GREEN : PAL_GOLD;
        hudC(1, line, tpal);
        const char* hint = "LINE THE POCKET";
        int hpal = PAL_INK;
        int until = framesUntilHour();
        if (mode_ == Mode::Reset) {
            hint = reason_;
            hpal = PAL_RED;
        } else if (mode_ == Mode::Roll) {
            hint = gutter_ ? "GUTTER" : (pin_[0].down ? "PINS" : "ROLLING");
            hpal = gutter_ ? PAL_RED : PAL_GOLD;
        } else if (mode_ == Mode::Swing) {
            if (onPocket() && std::fabs(meter() - 0.5f) <= 0.08f) {
                hint = until <= kTravel + 8 ? "RELEASE" : "HOLD FOR THE HOUR";
                hpal = PAL_GREEN;
            } else if (!onPocket()) {
                hint = "OFF THE POCKET";
                hpal = PAL_RED;
            } else {
                hint = "WAIT FOR THE STAR";
                hpal = PAL_AMBER;
            }
        } else if (onPocket()) {
            hint = until < kFpc * 20 ? "POCKET  WATCH THE HOUR" : "POCKET  C SWINGS";
            hpal = PAL_GREEN;
        } else if (until < kFpc * 20) {
            hint = "GET ON THE POCKET";
            hpal = PAL_AMBER;
        }
        hudC(26, hint, hpal);
        int cells = 16;
        int filled = std::clamp(int(std::lround(meter() * cells)), 0, cells);
        char bar[20];
        bar[0] = '[';
        for (int i = 0; i < cells; i++) bar[i + 1] = i < filled ? '#' : '-';
        bar[cells + 1] = ']';
        bar[cells + 2] = 0;
        if (mode_ == Mode::Swing) hudC(27, bar, std::fabs(meter() - 0.5f) <= 0.08f ? PAL_GREEN : PAL_AMBER);
        else hudC(27, "C SWING   ARROWS AIM", PAL_INK);
    }
}

}  // namespace pinschime

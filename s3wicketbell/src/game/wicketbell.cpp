#include "game/wicketbell.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace wicketbell {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kMeterRate = 0.50f;
constexpr float kSweetLo = 0.40f;
constexpr float kSweetHi = 0.60f;
constexpr float kBotLo = 0.46f;
constexpr float kBotHi = 0.54f;
constexpr float kAimSpeed = 40.f;
constexpr float kAimLim = 70.f;
constexpr float kFlightT = 0.68f;
constexpr float kRingT = 1.10f;
constexpr float kLeaveT = 0.80f;
constexpr float kDeadT = 0.72f;
constexpr float kTitleT = 0.36f;
constexpr float kPi = 3.14159265f;

constexpr float kBowlerX = 96.f;
constexpr float kBowlerFeet = 208.f;
constexpr float kBowlerW = 56.f;
constexpr float kBowlerH = 80.f;
constexpr float kBowlerBmpW = 48.f;
constexpr float kBowlerBmpH = 68.f;

float clampf(float v, float a, float b) { return v < a ? a : (v > b ? v : b); }

bool geometry() {
    const float step = kMeterRate / 60.f;
    if (kMouthR < 4.f) return false;
    if (kMouthR + 3.f >= kLipR) return false;
    if (kLipR + 4.f >= kStumpOuter) return false;
    if (mouthPx() + 2.f >= woodPx()) return false;
    if (!(lipPx() > mouthPx() && lipPx() < woodPx())) return false;
    if (!(kSweetLo >= 0.05f && kSweetHi <= 0.95f && kSweetHi - kSweetLo >= 0.12f)) return false;
    if (kSweetHi - kSweetLo < step * 6.f) return false;
    if (!(kBotLo > kSweetLo && kBotHi < kSweetHi && kBotHi - kBotLo > step * 3.f)) return false;
    if (kAimLim < woodPx() + 16.f) return false;
    if (kDrawW != float(kBmpW) || kDrawH != float(kBmpH)) return false;
    return true;
}

}  // namespace

Game::Fate Game::judge(float x, float meter) {
    if (meter < kSweetLo) return Fate::Short;
    if (meter > kSweetHi) return Fate::Hot;
    float ax = std::fabs(x);
    if (ax <= mouthPx()) return Fate::Bell;
    if (ax <= woodPx()) return Fate::Wood;
    return Fate::Wide;
}

const char* Game::fateName(Fate f) {
    switch (f) {
        case Fate::Bell: return "BELL";
        case Fate::Wood: return "WOOD";
        case Fate::Wide: return "WIDE";
        case Fate::Short: return "SHORT";
        case Fate::Hot: return "HOT";
    }
    return "OPEN";
}

bool Game::picture() const {
    if (!sys_ || art_.bell.w != kBmpW || art_.bell.h != kBmpH || art_.stumps.w != kBmpW) return false;
    auto pix = [&](const gs::Image& im, int x, int y) -> int {
        if (x < 0 || y < 0 || x >= im.w || y >= im.h) return -1;
        return sys_->vdp.rom()[im.off + size_t(y) * im.w + size_t(x)];
    };
    const int mx = int(kMidX);
    const int my = int(kBellY);
    const int mouth = pix(art_.bell, mx, my);
    const int lip = pix(art_.bell, int(kMidX + kMouthR + 3.f), my);
    const int stump = pix(art_.stumps, int(kMidX - kStumpOuter + 2.f), 70);
    const int gap = pix(art_.stumps, mx, 70);
    if (mouth != 0 || lip == 0 || stump == 0 || gap != 0) {
        std::fprintf(stderr, "s3wicketbell picture mouth %d lip %d stump %d gap %d\n", mouth, lip, stump, gap);
        return false;
    }
    return true;
}

bool Game::rules() const {
    if (!geometry()) {
        std::fprintf(stderr, "s3wicketbell geometry\n");
        return false;
    }
    const float mid = (kSweetLo + kSweetHi) * 0.5f;
    if (judge(0.f, mid) != Fate::Bell) return false;
    if (judge(mouthPx(), mid) != Fate::Bell) return false;
    if (judge(-mouthPx(), mid) != Fate::Bell) return false;
    if (judge(mouthPx() + 0.6f, mid) != Fate::Wood) return false;
    if (judge(lipPx(), mid) != Fate::Wood) return false;
    if (judge(woodPx(), mid) != Fate::Wood) return false;
    if (judge(woodPx() + 0.6f, mid) != Fate::Wide) return false;
    if (judge(0.f, kSweetLo) != Fate::Bell || judge(0.f, kSweetHi) != Fate::Bell) return false;
    if (judge(0.f, kSweetLo - 0.01f) != Fate::Short) return false;
    if (judge(0.f, kSweetHi + 0.01f) != Fate::Hot) return false;
    if (judge(0.f, 0.f) != Fate::Short || judge(0.f, 1.f) != Fate::Hot) return false;
    if (!picture()) return false;
    return true;
}

const char* Game::phase() const {
    switch (mode_) {
        case Mode::Title: return "title";
        case Mode::Aim: return "aim";
        case Mode::Flight: return "flight";
        case Mode::Dead: return "dead";
        case Mode::Ring: return "ring";
        case Mode::Leave: return "leave";
        case Mode::Pause: return "pause";
        case Mode::Over: return won_ ? "leave" : "over";
    }
    return "over";
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.setFogColor(gs::rgb4(6, 8, 10));
    sys.apu.setMaster(0.7f);
    if (!rules()) std::fprintf(stderr, "s3wicketbell rules failed\n");
    toTitle();
}

void Game::toTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    rung_ = false;
    armed_ = false;
    dead_ = 0;
    tryNo_ = 0;
    aim_ = 0.f;
    meter_ = 0.f;
    meterDir_ = 1.f;
    titleT_ = 0.f;
    flight_ = 0.f;
    shake_ = 0.f;
    bellAmp_ = 0.2f;
    why_ = "OPEN";
    fate_ = Fate::Short;
}

void Game::newGame() {
    if (!rules()) return;
    dead_ = 0;
    tryNo_ = 0;
    won_ = false;
    over_ = false;
    rung_ = false;
    aim_ = 0.f;
    shake_ = 0.f;
    bellAmp_ = 0.2f;
    why_ = "OPEN";
    beginAim();
}

void Game::beginAim() {
    meter_ = 0.f;
    meterDir_ = 1.f;
    armed_ = false;
    flight_ = 0.f;
    deadT_ = 0.f;
    mode_ = Mode::Aim;
}

void Game::bowlerHand(int which, float& x, float& y) const {
    float bx = 40.f, by = 30.f;
    if (which == 1) {
        bx = 34.f;
        by = 16.f;
    } else if (which == 2) {
        bx = 42.f;
        by = 10.f;
    }
    x = kBowlerX + (bx - kBowlerBmpW * 0.5f) * (kBowlerW / kBowlerBmpW);
    y = (kBowlerFeet - kBowlerH * 0.5f) + (by - kBowlerBmpH * 0.5f) * (kBowlerH / kBowlerBmpH);
}

void Game::release() {
    if (mode_ != Mode::Aim || !rules()) return;
    fate_ = judge(aim_, meter_);
    tryNo_ = dead_ + 1;
    why_ = fateName(fate_);
    int handPose = 2;
    bowlerHand(handPose, fromX_, fromY_);
    float mx, my;
    bmpToScreen(kMidX, kBellY, mx, my);
    toX_ = mx + aim_;
    toY_ = my;
    lob_ = 16.f;
    if (fate_ == Fate::Wood) {
        toY_ = my + 8.f;
        lob_ = 12.f;
    } else if (fate_ == Fate::Wide) {
        toY_ = my + 10.f;
        lob_ = 10.f;
    } else if (fate_ == Fate::Short) {
        toX_ = fromX_ + (mx + aim_ - fromX_) * 0.56f;
        toY_ = fromY_ + (my - fromY_) * 0.40f;
        lob_ = 8.f;
    } else if (fate_ == Fate::Hot) {
        toX_ = mx + aim_ * 0.35f;
        toY_ = my - 28.f;
        lob_ = 24.f;
    }
    flight_ = 0.f;
    mode_ = Mode::Flight;
    if (sys_) sys_->apu.noiseBurst(0.18f, 1800.f, 0.05f);
}

void Game::ring() {
    if (mode_ != Mode::Flight || rung_) return;
    rung_ = true;
    won_ = true;
    why_ = "BELL";
    bellAmp_ = 1.f;
    ringT_ = 0.f;
    mode_ = Mode::Ring;
    if (!sys_) return;
    sys_->rumble(0.45f, 0.9f, 200);
    sys_->setLight(255, 196, 48);
    sys_->apu.noiseBurst(0.22f, 900.f, 0.08f);
}

void Game::dieTry() {
    if (mode_ != Mode::Flight || rung_) return;
    dead_++;
    why_ = fateName(fate_);
    if (fate_ == Fate::Wood) shake_ = 1.f;
    deadT_ = 0.f;
    mode_ = Mode::Dead;
    if (!sys_) return;
    sys_->apu.noise(0, 1000);
    if (fate_ == Fate::Wood) sys_->apu.noiseBurst(0.22f, 420.f, 0.07f);
    else if (fate_ == Fate::Hot) sys_->apu.noiseBurst(0.12f, 1600.f, 0.05f);
    else sys_->apu.noiseBurst(0.1f, 240.f, 0.06f);
    blip(0, fate_ == Fate::Wood ? 180.f : 110.f, 0.08f, 0.22f);
    sys_->setLight(fate_ == Fate::Wood ? 160 : 120, 48, 28);
}

void Game::resolve() {
    if (mode_ != Mode::Flight) return;
    if (fate_ == Fate::Bell && judge(aim_, meter_) == Fate::Bell) ring();
    else dieTry();
}

void Game::advanceMeter(float dt) {
    meter_ += meterDir_ * kMeterRate * dt;
    if (meter_ >= 1.f) {
        meter_ = 1.f;
        meterDir_ = -1.f;
    } else if (meter_ <= 0.f) {
        meter_ = 0.f;
        meterDir_ = 1.f;
    }
}

void Game::tickTitle(float dt) {
    titleT_ += dt;
    if (sys_ && sys_->pad.pressed(gs::BTN_MODE)) sys_->eject();
    if (!rules()) {
        if (bot_) {
            won_ = false;
            over_ = true;
            mode_ = Mode::Over;
        }
        return;
    }
    if (bot_) {
        if (titleT_ > kTitleT) newGame();
        return;
    }
    if (startPressed() || bowlPressed()) newGame();
}

void Game::tickAim(float dt) {
    advanceMeter(dt);
    const bool sweetBell = judge(aim_, meter_) == Fate::Bell;
    if (sweetBell && !armed_) {
        blip(2, 1320.f, 0.045f, 0.04f);
        armed_ = true;
    }
    if (!sweetBell) armed_ = false;
    if (bot_) {
        if (meter_ >= kBotLo && meter_ <= kBotHi && std::fabs(aim_) <= mouthPx()) release();
        return;
    }
    float ax = sys_->pad.axisX;
    if (sys_->pad.down(gs::BTN_LEFT)) ax -= 1.f;
    if (sys_->pad.down(gs::BTN_RIGHT)) ax += 1.f;
    ax = clampf(ax, -1.f, 1.f);
    aim_ = clampf(aim_ + ax * kAimSpeed * dt, -kAimLim, kAimLim);
    if (bowlPressed()) release();
}

void Game::tickFlight(float dt) {
    flight_ += dt / kFlightT;
    if (flight_ >= 1.f) {
        flight_ = 1.f;
        resolve();
    }
}

void Game::tickDead(float dt) {
    deadT_ += dt;
    if (deadT_ < kDeadT) return;
    if (dead_ >= 3) {
        won_ = false;
        over_ = true;
        mode_ = Mode::Over;
        if (sys_) sys_->setLight(140, 28, 28);
        return;
    }
    beginAim();
}

void Game::tickRing(float dt) {
    ringT_ += dt;
    if (ringT_ > kRingT) {
        mode_ = Mode::Leave;
        leaveT_ = 0.f;
    }
}

void Game::tickLeave(float dt) {
    leaveT_ += dt;
    if (leaveT_ > kLeaveT) {
        over_ = true;
        mode_ = Mode::Over;
    }
}

void Game::tickOver() {
    if (bot_) return;
    if (startPressed() || bowlPressed()) newGame();
}

void Game::tickBell(float dt) {
    float rate = (mode_ == Mode::Ring || mode_ == Mode::Leave) ? 13.f : 2.5f;
    bellPh_ += dt * rate;
    if (mode_ == Mode::Ring || mode_ == Mode::Leave || (mode_ == Mode::Over && won_))
        bellAmp_ = std::max(0.22f, bellAmp_ - dt * 0.38f);
    else if (mode_ != Mode::Pause)
        bellAmp_ = 0.2f;
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - dt);
}

int Game::pose() const {
    if (mode_ == Mode::Flight && flight_ < 0.42f) return 2;
    if (mode_ == Mode::Aim && meter_ > 0.18f) return 1;
    if (mode_ == Mode::Title) return (int(clock_ * 2.f) & 1) ? 0 : 1;
    return 0;
}

void Game::ballAt(float u, float& x, float& y) const {
    float e = u * u * (3.f - 2.f * u);
    x = fromX_ + (toX_ - fromX_) * e;
    y = fromY_ + (toY_ - fromY_) * e - std::sin(u * kPi) * lob_;
}

void Game::blip(int ch, float freq, float vol, float hold) {
    if (!sys_ || ch < 0 || ch > 2) return;
    sys_->apu.tone(ch, freq, vol);
    hold_[ch] = hold;
}

void Game::audio(float dt) {
    if (!sys_) return;
    for (int ch = 0; ch < 3; ch++) {
        if (hold_[ch] > 0.f) {
            hold_[ch] -= dt;
            if (hold_[ch] <= 0.f) sys_->apu.tone(ch, 0, 0);
        }
    }
    if (rung_ && (mode_ == Mode::Ring || mode_ == Mode::Leave || (mode_ == Mode::Over && won_))) {
        float v = std::max(0.f, bellAmp_ - 0.16f) * 0.22f;
        float wob = 740.f + 16.f * std::sin(bellPh_ * 2.f);
        sys_->apu.tone(0, wob, v);
        sys_->apu.tone(1, 988.f, v * 0.38f);
        hold_[0] = 0.06f;
        hold_[1] = 0.06f;
    }
}

bool Game::bowlPressed() const {
    if (!sys_) return false;
    const gs::Pad& p = sys_->pad;
    return p.pressed(gs::BTN_A) || p.pressed(gs::BTN_B) || p.pressed(gs::BTN_C) || p.pressed(gs::BTN_X) ||
           p.pressed(gs::BTN_Z) || p.pressed(gs::BTN_TURBO);
}

bool Game::startPressed() const { return sys_ && sys_->pad.pressed(gs::BTN_START); }

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    clock_ += kDt;
    if (mode_ == Mode::Pause) {
        if (startPressed()) mode_ = held_;
        tickBell(kDt);
        audio(kDt);
        draw();
        return;
    }
    if (mode_ == Mode::Over) {
        tickOver();
        tickBell(kDt);
        audio(kDt);
        draw();
        return;
    }
    if (mode_ != Mode::Title && startPressed()) {
        held_ = mode_;
        mode_ = Mode::Pause;
        draw();
        return;
    }
    switch (mode_) {
        case Mode::Title: tickTitle(kDt); break;
        case Mode::Aim: tickAim(kDt); break;
        case Mode::Flight: tickFlight(kDt); break;
        case Mode::Dead: tickDead(kDt); break;
        case Mode::Ring: tickRing(kDt); break;
        case Mode::Leave: tickLeave(kDt); break;
        default: break;
    }
    tickBell(kDt);
    audio(kDt);
    draw();
}

void Game::spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool flip, bool shadow) {
    if (!sys_ || img.w == 0 || w < 1.f || h < 1.f) return;
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - w * 0.5f));
    s.y = int16_t(std::lround(cy - h * 0.5f));
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
        if (y < 64) {
            int r = 6 + y * 4 / 64;
            int g = 9 + y * 4 / 64;
            int b = 13 + y * 2 / 64;
            v.lineBackdrop[y] = gs::rgb4(r, g, b);
        } else if (y < 84) {
            v.lineBackdrop[y] = gs::rgb4(3, 7, 3);
        } else {
            int s = (y / 6) & 1;
            v.lineBackdrop[y] = gs::rgb4(2, 8 + s, 2);
        }
    }
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    float mx, my;
    bmpToScreen(kMidX, kBellY, mx, my);
    const float feet = kWicketY + kDrawH * 0.5f;
    const float wag = std::sin(bellPh_) * bellAmp_ * 10.f;
    const float stumpShake = std::sin(clock_ * 46.f) * shake_ * 3.f;
    const bool showBall = mode_ != Mode::Over || !won_;
    const int which = pose();

    auto banner = [&](const gs::Image& img, float y, int pal) {
        spr(img, 160.f, y, float(img.w), float(img.h), pal);
    };
    if (mode_ == Mode::Title) {
        banner(art_.title, 18.f, PAL_TITLE);
        banner(art_.three, 40.f, PAL_INK);
    } else if (won_ && (mode_ == Mode::Ring || mode_ == Mode::Leave || mode_ == Mode::Over)) {
        banner(art_.theBell, 20.f, PAL_GOLD);
        if (mode_ != Mode::Ring) banner(art_.leave, 42.f, PAL_TITLE);
    } else if (!won_ && dead_ >= 3 && (mode_ == Mode::Dead || mode_ == Mode::Over)) {
        banner(art_.tryDied, 22.f, PAL_RED);
    }

    float ballX = 0.f, ballY = 0.f;
    bool ball = false;
    if (mode_ == Mode::Title || mode_ == Mode::Aim || (mode_ == Mode::Pause && held_ == Mode::Aim)) {
        bowlerHand(which, ballX, ballY);
        if (mode_ == Mode::Title) ballY += std::sin(clock_ * 2.f) * 1.5f;
        ball = true;
    } else if (mode_ == Mode::Flight || mode_ == Mode::Dead ||
               (mode_ == Mode::Pause && (held_ == Mode::Flight || held_ == Mode::Dead))) {
        float u = mode_ == Mode::Flight || held_ == Mode::Flight ? clampf(flight_, 0.f, 1.f) : 1.f;
        if (mode_ == Mode::Dead) u = 1.f;
        ballAt(u, ballX, ballY);
        ball = showBall && !(won_ && ringT_ > 0.28f);
        float su = clampf(u, 0.f, 1.f);
        float shx = fromX_ + (toX_ - fromX_) * su;
        float shy = kBowlerFeet - 4.f + (feet - 6.f - (kBowlerFeet - 4.f)) * su;
        spr(art_.shadow, shx, shy, 16.f + (1.f - su) * 8.f, 6.f, PAL_WHITE, false, true);
    }

    if (ball) {
        int seam = int(clock_ * 14.f) & 1;
        float u = (mode_ == Mode::Flight) ? clampf(flight_, 0.f, 1.f) : (mode_ == Mode::Aim ? 0.f : 0.35f);
        float bh = 15.f - u * 6.f;
        spr(art_.ball[seam], ballX, ballY, bh, bh, PAL_BALL);
    }

    float rise = 0.f, spread = 0.f;
    if (rung_) {
        float k = 1.f;
        if (mode_ == Mode::Ring) k = std::min(1.f, ringT_ / 0.35f);
        rise = k * 22.f;
        spread = k * 14.f;
    }
    float bx, by;
    bmpToScreen(kMidX, 36.f, bx, by);
    spr(art_.bail, bx - 12.f - spread, by - rise, 18, 6, PAL_WOOD);
    spr(art_.bail, bx + 12.f + spread, by - rise, 18, 6, PAL_WOOD, true);

    spr(art_.bowler[which], kBowlerX, kBowlerFeet - kBowlerH * 0.5f, kBowlerW, kBowlerH, PAL_KIT);
    spr(art_.shadow, kBowlerX, kBowlerFeet + 1.f, 28, 8, PAL_WHITE, false, true);

    spr(art_.bell, mx + wag, kWicketY, kDrawW, kDrawH, PAL_BRASS);
    spr(art_.stumps, mx + stumpShake, kWicketY, kDrawW, kDrawH, PAL_WOOD);
    spr(art_.keeper, mx + 46.f, kWicketY + 18.f, 34, 46, PAL_KEEP);

    float markX = mx + aim_;
    Fate line = judge(aim_, (kSweetLo + kSweetHi) * 0.5f);
    int markPal = line == Fate::Bell ? PAL_GREEN : (line == Fate::Wide ? PAL_RED : PAL_GOLD);
    if (mode_ == Mode::Aim || mode_ == Mode::Flight || mode_ == Mode::Title)
        spr(art_.blot, markX, feet - 2.f, line == Fate::Bell ? 14.f : 8.f, 4.f, markPal);

    if (mode_ == Mode::Aim || (mode_ == Mode::Pause && held_ == Mode::Aim)) {
        const float x0 = 150.f, x1 = 308.f, yb = 198.f;
        spr(art_.blot, (x0 + x1) * 0.5f, yb, x1 - x0, 5.f, PAL_INK);
        float u0 = kSweetLo;
        float u1 = kSweetHi;
        float sw = (u1 - u0) * (x1 - x0);
        float sx = x0 + (u0 + u1) * 0.5f * (x1 - x0);
        spr(art_.blot, sx, yb, sw, 9.f, PAL_GREEN);
        float nu = clampf(meter_, 0.f, 1.f);
        spr(art_.blot, x0 + nu * (x1 - x0), yb, 4.f, 16.f, PAL_GOLD);
    }

    spr(art_.pitch, mx, 196.f, 230, 112, PAL_PITCH);
    spr(art_.screen, mx, my - 2.f, 120, 84, PAL_WHITE);
    spr(art_.rope, 28.f, 150.f, 28, 56, PAL_WHITE);
    spr(art_.rope, 300.f, 146.f, 28, 56, PAL_WHITE);
    spr(art_.crowd, 48.f, 96.f, 96, 20, PAL_CROWD);
    spr(art_.crowd, 250.f, 94.f, 90, 18, PAL_CROWD);
    spr(art_.house, 286.f, 86.f, 72, 48, PAL_HOUSE);
    spr(art_.tree, 22.f, 78.f, 44, 40, PAL_TREE);
    spr(art_.tree, 304.f, 82.f, 40, 36, PAL_TREE);
    spr(art_.cloud, 52.f, 16.f, 40, 16, PAL_SKY);
    spr(art_.cloud, 300.f, 26.f, 34, 14, PAL_SKY);
    spr(art_.sun, 20.f, 18.f, 16, 16, PAL_SKY);

    char buf[48];
    hud(1, 0, "S3 WICKETBELL", PAL_TITLE);
    if (mode_ != Mode::Title) {
        int shown = tryNo_ > 0 ? tryNo_ : dead_ + 1;
        if (shown > 3) shown = 3;
        std::snprintf(buf, sizeof buf, "TRY %d", shown);
        hud(33, 0, buf, PAL_INK);
        char pips[8];
        for (int i = 0; i < 3; i++) pips[i] = i < dead_ ? 'X' : (i == dead_ && !won_ ? '>' : '.');
        if (won_) {
            for (int i = 0; i < 3; i++) pips[i] = i < dead_ ? 'X' : (i == tryNo_ - 1 ? '*' : '.');
        }
        pips[3] = 0;
        std::snprintf(buf, sizeof buf, "DEAD %d/3  %s", dead_, pips);
        hud(1, 1, buf, dead_ ? PAL_RED : PAL_GOLD);
    }

    if (mode_ == Mode::Title) {
        hudC(24, "RING THE MOUTH BEFORE THE THIRD TRY DIES", PAL_GOLD);
        hudC(26, "LEFT RIGHT AIM   Z BOWLS   ENTER STARTS", PAL_INK);
    } else if (mode_ == Mode::Pause) {
        hudC(12, "PAUSED", PAL_GOLD);
        hudC(14, "ENTER RESUMES", PAL_INK);
    } else if (mode_ == Mode::Aim || (mode_ == Mode::Flight && !won_)) {
        Fate live = mode_ == Mode::Aim ? judge(aim_, meter_) : fate_;
        hudC(22, fateName(live), live == Fate::Bell ? PAL_GREEN : PAL_RED);
        hud(30, 26, "Z BOWLS", PAL_INK);
    } else if (won_ && mode_ == Mode::Over) {
        std::snprintf(buf, sizeof buf, "TRY %d", tryNo_);
        hudC(24, buf, PAL_GOLD);
        if (!bot_) hudC(26, "ENTER BOWLS AGAIN", PAL_INK);
    } else if (!won_ && mode_ == Mode::Over) {
        hudC(24, "BELL SILENT", PAL_RED);
        if (!bot_) hudC(26, "ENTER BOWLS AGAIN", PAL_INK);
    } else if (mode_ == Mode::Dead) {
        hudC(22, why_, PAL_RED);
    }
}

}  // namespace wicketbell

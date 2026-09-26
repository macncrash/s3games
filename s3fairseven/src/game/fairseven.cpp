#include "game/fairseven.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace fairseven {
namespace {

constexpr int kBottles = 5;
constexpr float kBottleX[kBottles] = {48.f, 104.f, 160.f, 216.f, 272.f};
constexpr int kBottlePts[kBottles] = {1, 2, 3, 2, 1};
constexpr float kHit = 15.f;
constexpr float kBotHit = 10.f;
constexpr int kRace = 7;
constexpr int kLogMax = 32;
constexpr float kCx = 160.f;
constexpr float kAmp = 124.f;
constexpr float kOmega = 2.2f;
constexpr float kNudgeMax = 40.f;
constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kFlightT = 0.30f;
constexpr float kShowT = 0.38f;
constexpr float kShowSix = 0.62f;

constexpr float kPipY = 20.f;
constexpr float kPennantY = 34.f;
constexpr float kAwnY = 58.f;
constexpr float kBulbY = 76.f;
constexpr float kNumY = 90.f;
constexpr float kBellY = 104.f;
constexpr float kPivotY = 116.f;
constexpr float kHangY = 130.f;
constexpr float kNeckY = 152.f;
constexpr float kBottleY = 172.f;
constexpr float kShelfY = 196.f;
constexpr float kMissY = 204.f;
constexpr float kHeadY = 164.f;

int bottleAt(float x, float hit) {
    int best = -1;
    float bestD = hit;
    for (int i = 0; i < kBottles; i++) {
        float d = std::fabs(x - kBottleX[i]);
        if (d <= bestD) {
            bestD = d;
            best = i;
        }
    }
    return best;
}

int pointsAt(float x, float hit) {
    int i = bottleAt(x, hit);
    return i < 0 ? 0 : kBottlePts[i];
}

bool reached(int n) { return n >= kRace; }

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

const char* Game::phase() const {
    switch (mode_) {
        case Mode::Title: return "title";
        case Mode::Aim: return "aim";
        case Mode::Flight: return "flight";
        case Mode::Show: return "show";
        case Mode::Win: return "win";
        case Mode::Lose: return "lose";
        case Mode::Pause: return "pause";
    }
    return "?";
}

Game::Mode Game::view() const { return mode_ == Mode::Pause ? held_ : mode_; }

int Game::palFor(int pts) const {
    if (pts >= 3) return PAL_BELL;
    if (pts == 2) return PAL_GOLD;
    return PAL_CREAM;
}

const gs::Image& Game::digit(int pts) const {
    if (pts >= 3) return art_.three;
    if (pts == 2) return art_.two;
    return art_.one;
}

float Game::ringX() const {
    float x = kCx + kAmp * std::sin(swingT_ * kOmega) + nudge_;
    return clampf(x, 8.f, 312.f);
}

int Game::aimBottle() const {
    if (!yours_) return 0;
    if (you_ >= 6) return 0;
    if (you_ >= 4) return 1;
    return 2;
}

bool Game::ready() const {
    float d = std::fabs(ringX() - kBottleX[aimBottle()]);
    if (d <= kBotHit) return true;
    return aimT_ > 3.2f && d <= kHit;
}

bool Game::audit() {
    auto bad = [&](const char* why) {
        std::fprintf(stderr, "s3fairseven rules: %s\n", why);
        return false;
    };
    if (kRace != 7) return bad("race");
    if (reached(6) || !reached(7) || !reached(8)) return bad("tally");
    int bells = 0;
    for (int i = 0; i < kBottles; i++) {
        if (kBottlePts[i] < 1 || kBottlePts[i] > 3) return bad("points");
        if (kBottlePts[i] == 3) bells++;
        if (pointsAt(kBottleX[i], kHit) != kBottlePts[i]) return bad("center");
        if (pointsAt(kBottleX[i], kBotHit) != kBottlePts[i]) return bad("bot window");
        if (kBottleX[i] < kCx - kAmp + 8.f || kBottleX[i] > kCx + kAmp - 8.f) return bad("reach");
        for (int j = i + 1; j < kBottles; j++) {
            if (std::fabs(kBottleX[i] - kBottleX[j]) <= kHit * 2.f) return bad("overlap");
        }
    }
    if (bells != 1 || kBottlePts[2] != 3) return bad("bell");
    for (int i = 0; i < kBottles - 1; i++) {
        float mid = (kBottleX[i] + kBottleX[i + 1]) * 0.5f;
        if (pointsAt(mid, kHit) != 0) return bad("gap");
    }
    if (pointsAt(4.f, kHit) != 0 || pointsAt(316.f, kHit) != 0) return bad("miss");
    const float start = kCx - kAmp;
    if (std::fabs(start - kBottleX[0]) <= kBotHit) return bad("snap");
    if (pointsAt(start, kBotHit) != 0) return bad("idle");
    return true;
}

bool Game::ledgerOk() const {
    if (tossN_ <= 0 || tossN_ > kLogMax) return false;
    int y = 0;
    int t = 0;
    for (int i = 0; i < tossN_; i++) {
        int p = log_[i].pts;
        if (p < 0 || p > 3) return false;
        if (log_[i].yours != (i % 2 == 0)) return false;
        if (log_[i].yours) y += p;
        else t += p;
        bool last = i + 1 == tossN_;
        if ((y >= kRace || t >= kRace) && !last) return false;
    }
    return y == you_ && t == them_ && y >= kRace && t < kRace && log_[tossN_ - 1].yours;
}

void Game::resetMatch() {
    you_ = them_ = gain_ = tossN_ = 0;
    shortSix_ = won_ = over_ = false;
    yours_ = true;
    hot_ = -1;
    fanStep_ = -1;
    say_ = "";
    landX_ = fromX_ = kCx;
    for (Toss& t : log_) t = {};
}

void Game::armSwing() {
    swingT_ = -kPi * 0.5f / kOmega;
    nudge_ = 0.f;
    aimT_ = 0.f;
    hot_ = -1;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    rules_ = audit();
    if (!rules_) std::fprintf(stderr, "s3fairseven rules failed\n");
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(2, 1, 4));
    sys.apu.setMaster(0.7f);
    sys.apu.setEcho(0.16f, 0.22f, 0.12f);
    resetMatch();
    mode_ = Mode::Title;
    clock_ = 0.f;
}

void Game::begin() {
    resetMatch();
    armSwing();
    mode_ = Mode::Aim;
    blip(392.f);
}

void Game::toTitle() {
    resetMatch();
    hush();
    mode_ = Mode::Title;
}

void Game::release() {
    if (mode_ != Mode::Aim || !rules_) return;
    fromX_ = landX_ = ringX();
    flight_ = kFlightT;
    mode_ = Mode::Flight;
    blip(yours_ ? 520.f : 340.f);
    if (sys_) sys_->apu.noiseBurst(0.08f, 1800.f, 0.03f);
}

void Game::win() {
    won_ = true;
    over_ = true;
    mode_ = Mode::Win;
    say_ = "FIRST TO SEVEN";
    fanStep_ = 0;
    fanT_ = 0.f;
    chord(523.f, 659.f, 784.f);
    if (sys_) {
        sys_->rumble(0.35f, 0.7f, 180);
        sys_->setLight(255, 196, 48);
    }
}

void Game::lose() {
    won_ = false;
    over_ = true;
    mode_ = Mode::Lose;
    say_ = "THEY HIT SEVEN";
    blip(110.f);
    if (sys_) sys_->setLight(140, 32, 28);
}

void Game::resolve() {
    const int pts = pointsAt(landX_, kHit);
    if (tossN_ >= kLogMax) {
        rules_ = false;
        lose();
        say_ = "RULES";
        return;
    }
    log_[tossN_].pts = pts;
    log_[tossN_].yours = yours_;
    tossN_++;
    gain_ = pts;
    if (yours_ && you_ == 6) shortSix_ = true;
    if (yours_) you_ += pts;
    else them_ += pts;
    if (yours_ && you_ == 6) shortSix_ = true;

    if (yours_ && you_ >= kRace) {
        if (rules_ && ledgerOk()) win();
        else {
            rules_ = false;
            std::fprintf(stderr, "s3fairseven ledger you %d them %d tosses %d\n", you_, them_, tossN_);
            lose();
            say_ = "RULES";
        }
        return;
    }
    if (!yours_ && them_ >= kRace && you_ < kRace) {
        lose();
        return;
    }

    if (pts == 0) say_ = "MISS";
    else if (yours_ && you_ == 6) say_ = "SIX IS SHORT";
    else if (!yours_ && them_ == 6) say_ = "THEIR SIX IS SHORT";
    else if (pts == 3) say_ = "BELL COUNTS 3";
    else if (pts == 2) say_ = "GOLD COUNTS 2";
    else say_ = "CREAM COUNTS 1";

    if (pts == 0) {
        blip(120.f);
        if (sys_) sys_->apu.noiseBurst(0.1f, 500.f, 0.05f);
    } else if (yours_ && you_ == 6) {
        blip(196.f);
    } else if (pts == 3) {
        chord(523.f, 659.f, 784.f);
    } else {
        blip(pts == 2 ? 494.f : 370.f);
    }
    showT_ = (yours_ && you_ == 6) ? kShowSix : kShowT;
    mode_ = Mode::Show;
}

void Game::afterShow() {
    yours_ = !yours_;
    armSwing();
    mode_ = Mode::Aim;
}

void Game::blip(float freq) {
    if (!sys_) return;
    sys_->apu.tone(0, freq, 0.07f);
    beep_ = std::max(beep_, 0.08f);
}

void Game::chord(float a, float b, float c) {
    if (!sys_) return;
    sys_->apu.tone(0, a, 0.08f);
    sys_->apu.tone(2, b, 0.06f);
    beep_ = std::max(beep_, 0.16f);
    (void)c;
}

void Game::hush() {
    if (!sys_) return;
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(2, 0, 0);
}

void Game::tickAudio(float dt) {
    if (!sys_) return;
    if (beep_ > 0.f) {
        beep_ -= dt;
        if (beep_ <= 0.f) hush();
    }
    if (fanStep_ < 0) return;
    if (fanStep_ > 0) {
        fanT_ -= dt;
        if (fanT_ > 0.f) return;
    }
    const float notes[4] = {523.25f, 659.25f, 783.99f, 1046.5f};
    if (fanStep_ < 4) {
        sys_->apu.tone(1, notes[fanStep_], 0.1f);
        fanStep_++;
        fanT_ = 0.11f;
    } else {
        sys_->apu.tone(1, 0, 0);
        fanStep_ = -1;
    }
}

Game::Input Game::readPad(const gs::Pad& pad) const {
    Input in;
    in.action = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_TURBO);
    in.start = pad.pressed(gs::BTN_START);
    in.back = pad.pressed(gs::BTN_MODE);
    if (pad.down(gs::BTN_LEFT)) in.x -= 1.f;
    if (pad.down(gs::BTN_RIGHT)) in.x += 1.f;
    if (std::fabs(pad.axisX) > 0.25f) in.x = pad.axisX;
    in.x = clampf(in.x, -1.f, 1.f);
    return in;
}

Game::Input Game::botInput() const {
    Input in;
    if (mode_ == Mode::Title && clock_ > 0.45f) in.start = true;
    return in;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (mode_ != Mode::Pause) clock_ += kDt;
    tickAudio(kDt);

    const Mode before = mode_;
    if (before == Mode::Aim) {
        swingT_ += kDt;
        aimT_ += kDt;
        float want = (!bot_ && yours_) ? readPad(sys.pad).x * kNudgeMax : 0.f;
        nudge_ += (want - nudge_) * 0.45f;
    }

    const Input in = bot_ ? botInput() : readPad(sys.pad);
    if (before == Mode::Title) {
        if (in.back && !bot_) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        } else if (in.start || in.action) begin();
    } else if (before == Mode::Pause) {
        if (in.start || in.action) mode_ = held_;
        else if (in.back) toTitle();
    } else if (before == Mode::Win || before == Mode::Lose) {
        if (!bot_ && (in.start || in.action)) begin();
        else if (!bot_ && in.back) toTitle();
    } else if (!bot_ && (in.start || in.back)) {
        held_ = before;
        mode_ = Mode::Pause;
    } else if (before == Mode::Aim) {
        int hot = yours_ ? bottleAt(ringX(), kHit) : -1;
        if (hot >= 0 && hot != hot_) blip(520.f + float(kBottlePts[hot]) * 90.f);
        hot_ = hot;
        if (bot_ || !yours_) {
            if (ready()) release();
        } else if (in.action) release();
    } else if (before == Mode::Flight) {
        flight_ -= kDt;
        if (flight_ <= 0.f) resolve();
    } else if (before == Mode::Show) {
        showT_ -= kDt;
        if (showT_ <= 0.f || (!bot_ && in.action)) afterShow();
    }

    draw();
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog) {
    if (!sys_ || h < 1.f || m.h < 1) return;
    const float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, int fog) {
    if (!sys_ || w < 1.f || h < 1.f || m.h < 1) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(std::max(w, h));
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::word(const gs::Image& img, float cx, float cy, int pal, float mul) {
    if (!sys_ || img.w < 1) return;
    gs::Sprite s;
    s.w = int16_t(std::max(1, int(std::lround(img.w * mul))));
    s.h = int16_t(std::max(1, int(std::lround(img.h * mul))));
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
    const int n = s ? int(std::strlen(s)) : 0;
    hud(20 - n / 2, row, s, pal);
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        if (y < 48) {
            float u = y / 47.f;
            v.lineBackdrop[y] = gs::rgb4(1 + int(u * 2), 1, 6 + int((1.f - u) * 2));
        } else if (y < 120) {
            float u = (y - 48) / 72.f;
            v.lineBackdrop[y] = gs::rgb4(3 + int(u * 5), 1 + int(u), 6 - int(u * 3));
        } else if (y < 188) {
            v.lineBackdrop[y] = gs::rgb4(6, 2, 3);
        } else {
            int g = 3 + ((y / 4) & 1);
            v.lineBackdrop[y] = gs::rgb4(g + 1, g - 1, 1);
        }
    }
}

void Game::drawBooth() {
    const Mode m = view();
    const bool hang = m == Mode::Title || m == Mode::Aim;
    const float titleX = kCx + kAmp * std::sin(clock_ * kOmega);
    const float hx = m == Mode::Title ? clampf(titleX, 8.f, 312.f) : ringX();
    const int over = hang ? bottleAt(hx, kHit) : -1;
    const bool showRing = m == Mode::Show || m == Mode::Win || m == Mode::Lose;
    const int landed = showRing ? bottleAt(landX_, kHit) : -1;

    if (m == Mode::Title) word(art_.fair, kCx, 42.f, PAL_TITLE);
    if (m == Mode::Win) {
        float mul = 1.f + 0.04f * std::sin(clock_ * 8.f);
        word(art_.seven, kCx, 118.f, PAL_TITLE, mul);
    }
    if (m == Mode::Lose) word(art_.late, kCx, 118.f, PAL_BAD);

    if (m == Mode::Flight) {
        float u = 1.f - clampf(flight_ / kFlightT, 0.f, 1.f);
        float e = u * u * (3.f - 2.f * u);
        int pred = bottleAt(landX_, kHit);
        float destY = pred >= 0 ? kNeckY : kMissY;
        float x = fromX_ + (landX_ - fromX_) * e;
        float y = kHangY + (destY - kHangY) * e - std::sin(u * kPi) * 28.f;
        int pal = yours_ ? PAL_YOU : PAL_THEM;
        if (pred >= 0) pal = palFor(kBottlePts[pred]);
        spr(art_.ring, x, y, 16.f + (1.f - e) * 6.f, pal);
    } else if (showRing && landed >= 0) {
        float pulse = m == Mode::Show ? 1.f + std::sin(showT_ * 18.f) * 0.06f : 1.f;
        spr(art_.ring, landX_, kNeckY, 18.f * pulse, yours_ ? PAL_YOU : PAL_THEM);
    } else if (showRing) {
        spr(art_.ring, landX_, kMissY, 14.f, yours_ ? PAL_YOU : PAL_THEM);
    } else if (hang) {
        int pal = yours_ || m == Mode::Title ? PAL_YOU : PAL_THEM;
        if (over >= 0) pal = palFor(kBottlePts[over]);
        float bob = over >= 0 ? std::sin(clock_ * 16.f) * 1.4f : 0.f;
        spr(art_.ring, hx, kHangY + bob, over >= 0 ? 20.f : 16.f, pal);
        float x0 = kCx;
        float y0 = kPivotY;
        float x1 = hx;
        float y1 = kHangY;
        for (int i = 1; i <= 5; i++) {
            float u = i / 6.f;
            spr(art_.dot, x0 + (x1 - x0) * u, y0 + (y1 - y0) * u, 3.f, PAL_TITLE);
        }
    }

    for (int i = 0; i < kRace; i++) {
        float yx = 12.f + float(i) * 11.f;
        float tx = 308.f - 11.f * float(kRace - 1 - i);
        int yp = i < you_ ? PAL_PIPY : PAL_PIPD;
        int tp = i < them_ ? PAL_PIPT : PAL_PIPD;
        float s = (i == kRace - 1) ? 11.f : 8.f;
        if (i == kRace - 1 && you_ >= kRace) s = 13.f;
        spr(art_.pip, yx, kPipY, s, yp);
        spr(art_.pip, tx, kPipY, i == kRace - 1 ? 11.f : 8.f, tp);
    }

    word(art_.sign, kCx, kAwnY - 2.f, PAL_PAPER);
    spr(art_.bell, kBottleX[2], kBellY, 16.f, PAL_BELL);

    float yb = yours_ ? std::sin(clock_ * 6.f) * 1.2f : 0.f;
    float tb = !yours_ && m != Mode::Title ? std::sin(clock_ * 6.f) * 1.2f : 0.f;
    spr(art_.head, 24.f, kHeadY + yb, 36.f, PAL_HEADY, false);
    spr(art_.head, 296.f, kHeadY + tb, 36.f, PAL_HEADT, true);

    for (int i = 0; i < kBottles; i++) {
        int pts = kBottlePts[i];
        int pal = palFor(pts);
        bool hot = hang && over == i;
        float bob = (pts == 3) ? std::sin(clock_ * 3.f) * 1.2f : 0.f;
        word(digit(pts), kBottleX[i], kNumY + bob, pal);
        spr(art_.shade, kBottleX[i], kShelfY - 4.f, 10.f, PAL_BOOTH);
        float h = (pts == 3 ? 54.f : 46.f) * (hot ? 1.05f : 1.f);
        spr(art_.bottle, kBottleX[i], kBottleY - (pts == 3 ? 4.f : 0.f), h, pal);
    }

    for (int i = 0; i < 11; i++) {
        float x = 48.f + float(i) * 22.f;
        bool on = ((i + int(clock_ * 6.f)) % 4) != 0;
        spr(art_.bulb, x, kBulbY + std::sin(clock_ * 2.f + float(i)) * 1.2f, on ? 9.f : 6.f, PAL_BOOTH);
    }

    stamp(art_.shelf, kCx, kShelfY, 236.f, 12.f, PAL_BOOTH);
    stamp(art_.cloth, kCx, 132.f, 220.f, 86.f, PAL_BOOTH);
    stamp(art_.awning, kCx, kAwnY, 268.f, 26.f, PAL_BOOTH);
    stamp(art_.post, 30.f, 120.f, 12.f, 120.f, PAL_BOOTH);
    stamp(art_.post, 290.f, 120.f, 12.f, 120.f, PAL_BOOTH);

    const int buntPal[3] = {PAL_BAD, PAL_TITLE, PAL_THEM};
    for (int i = 0; i < 4; i++) {
        float y = kPennantY + ((i & 1) ? 2.f : 0.f);
        spr(art_.pennant, 16.f + float(i) * 18.f, y, 12.f, buntPal[i % 3]);
        spr(art_.pennant, 304.f - float(i) * 18.f, y, 12.f, buntPal[(i + 1) % 3]);
    }

    spr(art_.wheel, 58.f, 46.f, 52.f, PAL_BOOTH, false, 8);
    for (int i = 0; i < 6; i++) {
        float a = clock_ * 0.55f + float(i) * kPi / 3.f;
        spr(art_.gondola, 58.f + std::cos(a) * 18.f, 46.f + std::sin(a) * 18.f, 7.f, PAL_BOOTH, false, 7);
    }
    spr(art_.moon, 286.f, 28.f, 14.f, PAL_BOOTH, false, 1);
    const float stars[][2] = {{96, 12}, {140, 18}, {188, 10}, {230, 16}, {256, 8}};
    for (int i = 0; i < 5; i++) {
        if ((int(clock_ * 2.f) + i) % 4 == 0) continue;
        spr(art_.dot, stars[i][0], stars[i][1], 3.f, PAL_INK);
    }

    if (m == Mode::Win) {
        for (int i = 0; i < 8; i++) {
            float a = clock_ * 2.2f + float(i) * kPi / 4.f;
            float rad = 28.f + float(i & 1) * 14.f;
            spr(art_.dot, kCx + std::cos(a) * rad, 118.f + std::sin(a) * 10.f, 4.f, buntPal[i % 3]);
        }
    }
}

void Game::drawHud() {
    const Mode m = view();
    char buf[64];
    if (m == Mode::Title) {
        hud(1, 0, "V" S3_VERSION, PAL_INK);
        hudC(22, "FIRST TO SEVEN", PAL_TITLE);
        hudC(23, "ONE RING APIECE", PAL_INK);
        hudC(24, "CREAM 1   GOLD 2   BELL 3", PAL_GOLD);
        hudC(25, "A SIX IS STILL SHORT", PAL_BAD);
        hudC(26, "ARROWS NUDGE   Z OR C THROWS", PAL_INK);
        if ((int(clock_ * 2.f) & 1) == 0) hudC(27, "PRESS START", PAL_TITLE);
        return;
    }
    if (mode_ == Mode::Pause) {
        hudC(12, "PAUSED", PAL_TITLE);
        hudC(13, "START RESUMES", PAL_INK);
        hudC(14, "ESC TO THE TITLE", PAL_INK);
    }

    std::snprintf(buf, sizeof buf, "YOU %d", you_);
    hud(1, 0, buf, you_ >= kRace ? PAL_GOOD : (you_ == 6 ? PAL_BAD : PAL_YOU));
    std::snprintf(buf, sizeof buf, "THEM %d", them_);
    hud(40 - int(std::strlen(buf)), 0, buf, them_ >= kRace ? PAL_BAD : PAL_THEM);
    if (m == Mode::Aim || m == Mode::Flight || m == Mode::Show) {
        hudC(1, yours_ ? "YOUR RING" : "BARKER RING", yours_ ? PAL_YOU : PAL_THEM);
    }

    if (m == Mode::Win) {
        hudC(23, "FIRST TO SEVEN", PAL_GOOD);
        std::snprintf(buf, sizeof buf, "YOU %d   THEM %d", you_, them_);
        hudC(24, buf, PAL_TITLE);
        hudC(25, "A SIX WAS STILL SHORT", PAL_INK);
        if (!bot_) hudC(27, "START PLAYS AGAIN", PAL_INK);
    } else if (m == Mode::Lose) {
        hudC(23, "THEY HIT SEVEN", PAL_BAD);
        std::snprintf(buf, sizeof buf, "YOU %d   THEM %d", you_, them_);
        hudC(24, buf, PAL_INK);
        hudC(25, "UNDER SEVEN IS NOT THE GAME", PAL_BAD);
        if (!bot_) hudC(27, "START TRIES AGAIN", PAL_INK);
    } else if (m == Mode::Show) {
        int pal = PAL_INK;
        if (you_ == 6 && yours_) pal = PAL_BAD;
        else if (gain_ == 3) pal = PAL_BELL;
        else if (gain_ == 2) pal = PAL_GOLD;
        else if (gain_ == 1) pal = PAL_CREAM;
        else pal = PAL_BAD;
        hudC(25, say_, pal);
    } else if (m == Mode::Flight) {
        hudC(25, yours_ ? "RING AWAY" : "BARKER TOSSES", yours_ ? PAL_YOU : PAL_THEM);
    } else if (m == Mode::Aim) {
        int hot = bottleAt(ringX(), kHit);
        if (you_ == 6) hudC(24, "A SIX IS STILL SHORT", PAL_BAD);
        else if (them_ == 6) hudC(24, "THEIR SIX IS STILL SHORT", PAL_BAD);
        if (!yours_) hudC(25, "BARKER TOSSES", PAL_THEM);
        else if (hot >= 0 && kBottlePts[hot] == 3) hudC(25, "OVER THE BELL", PAL_BELL);
        else if (hot >= 0 && kBottlePts[hot] == 2) hudC(25, "OVER GOLD", PAL_GOLD);
        else if (hot >= 0) hudC(25, "OVER CREAM", PAL_CREAM);
        else hudC(25, "LET IT COME ACROSS", PAL_INK);
        if (!bot_ && yours_ && tossN_ == 0) hudC(27, "Z OR C THROWS   ARROWS NUDGE", PAL_INK);
    }
}

void Game::draw() {
    if (!sys_) return;
    sys_->vdp.clearSprites();
    sys_->vdp.HUD.clear();
    backdrop();
    drawBooth();
    drawHud();
}

}  // namespace fairseven

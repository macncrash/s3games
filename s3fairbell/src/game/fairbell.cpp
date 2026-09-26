#include "game/fairbell.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace fairbell {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kMeterRate = 0.50f;
constexpr float kStrikeT = 0.28f;
constexpr float kFallT = 0.42f;
constexpr float kDeadT = 0.65f;
constexpr float kRingT = 1.10f;
constexpr float kLeaveT = 1.15f;
constexpr float kKidHome = 96.f;
constexpr float PI = 3.14159265f;

const int kStars[][2] = {{18, 14}, {46, 28}, {78, 12}, {250, 16}, {286, 30}, {304, 12}, {150, 10}, {210, 8}};

int clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

}  // namespace

bool Game::rules() const {
    if (!(kCapTop < kSlotCap && kSlotCap < kCapBot && kCapBot < kGoldTop && kGoldTop < kGoldBot &&
          kGoldBot < kSlotBase))
        return false;
    const float span = kSlotBase - kSlotCap;
    if (span < 40.f) return false;
    const float lo = imgToMeter(float(kGoldBot));
    const float hi = imgToMeter(float(kGoldTop));
    const float step = kMeterRate * kDt;
    if (!(step > 0.f && hi > lo && hi - lo > step * 3.f)) return false;
    if (classify((lo + hi) * 0.5f) != Fate::Bell) return false;
    if (classify(imgToMeter(float(kGoldBot) + 2.f)) != Fate::Short) return false;
    if (classify(imgToMeter(float(kGoldTop) - 2.f)) != Fate::Hot) return false;
    if (classify(0.f) != Fate::Short || classify(1.f) != Fate::Hot) return false;
    return true;
}

const char* Game::phase() const {
    switch (mode_) {
        case Mode::Title: return "title";
        case Mode::Aim: return "aim";
        case Mode::Strike: return "strike";
        case Mode::Fall: return "fall";
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
    sys.vdp.B.enabled = true;
    sys.vdp.setFogColor(gs::rgb4(2, 1, 4));
    clock_ = 0.f;
    meter_ = 0.f;
    meterDir_ = 1;
    bellPh_ = 0.f;
    bellAmp_ = 0.12f;
    kidX_ = kKidHome;
    toTitle();
}

void Game::toTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    rung_ = false;
    dead_ = 0;
    tryNo_ = 0;
    why_ = "OPEN";
    fate_ = Fate::Short;
    wasFate_ = Fate::Short;
    wasSweet_ = false;
    bellAmp_ = 0.12f;
    kidX_ = kKidHome;
    burstT_ = 0.f;
}

void Game::newGame() {
    dead_ = 0;
    tryNo_ = 0;
    won_ = false;
    over_ = false;
    rung_ = false;
    why_ = "OPEN";
    power_ = 0.f;
    bellAmp_ = 0.12f;
    burstT_ = 0.f;
    kidX_ = kKidHome;
    hush();
    beginAim();
}

void Game::beginAim() {
    meter_ = 0.f;
    meterDir_ = 1;
    wasSweet_ = false;
    wasFate_ = Fate::Short;
    mode_ = Mode::Aim;
}

void Game::advanceMeter(float dt) {
    meter_ += float(meterDir_) * kMeterRate * dt;
    if (meter_ >= 1.f) {
        meter_ = 1.f;
        meterDir_ = -1;
    } else if (meter_ <= 0.f) {
        meter_ = 0.f;
        meterDir_ = 1;
    }
}

float Game::meterToImg(float m) const {
    m = std::clamp(m, 0.f, 1.f);
    return kSlotBase + (kSlotCap - kSlotBase) * m;
}

float Game::imgToMeter(float img) const { return (kSlotBase - img) / (kSlotBase - kSlotCap); }

Game::Fate Game::classify(float m) const {
    const float img = meterToImg(m);
    if (img >= float(kGoldTop) && img <= float(kGoldBot)) return Fate::Bell;
    if (img < float(kGoldTop)) return Fate::Hot;
    return Fate::Short;
}

float Game::towerTop() const { return kTowerFeet - kTowerDrawH; }

float Game::trackY(float imgY) const { return towerTop() + imgY * (kTowerDrawH / float(kTowerImgH)); }

float Game::puckY(float m) const { return trackY(meterToImg(m)); }

float Game::shownPuckY() const {
    if (mode_ == Mode::Fall) {
        float u = std::clamp(fallT_ / kFallT, 0.f, 1.f);
        u = u * u;
        return puckY(power_) + (puckY(0.f) - puckY(power_)) * u;
    }
    if (mode_ == Mode::Strike || mode_ == Mode::Ring || mode_ == Mode::Leave || (mode_ == Mode::Over && won_))
        return puckY(power_);
    if (mode_ == Mode::Dead || (mode_ == Mode::Over && !won_)) return puckY(0.f);
    return puckY(meter_);
}

bool Game::hammerUp() const {
    if (mode_ == Mode::Strike || mode_ == Mode::Fall || mode_ == Mode::Dead) return false;
    if (mode_ == Mode::Ring || mode_ == Mode::Leave || (mode_ == Mode::Over && won_)) return false;
    if (mode_ == Mode::Over) return false;
    return meter_ > 0.42f;
}

void Game::strike() {
    if (mode_ != Mode::Aim) return;
    power_ = meter_;
    fate_ = classify(power_);
    mode_ = Mode::Strike;
    strikeT_ = 0.f;
    why_ = fate_ == Fate::Bell ? "BELL" : (fate_ == Fate::Hot ? "HOT" : "SHORT");
    if (sys_) {
        sys_->apu.noiseBurst(0.18f, 240.f, 0.05f);
        sys_->rumble(0.25f, 0.45f, 40);
    }
}

void Game::startFall() {
    if (mode_ != Mode::Strike) return;
    mode_ = Mode::Fall;
    fallT_ = 0.f;
}

void Game::ring() {
    if (rung_ || mode_ != Mode::Strike) return;
    rung_ = true;
    won_ = true;
    tryNo_ = dead_ + 1;
    why_ = "BELL";
    fate_ = Fate::Bell;
    bellAmp_ = 1.f;
    bellTick_ = 0.02f;
    ringT_ = 0.f;
    burstT_ = 0.6f;
    mode_ = Mode::Ring;
    chord(523.f, 659.f, 784.f, 0.42f);
    if (!sys_) return;
    sys_->rumble(0.5f, 0.9f, 180);
    sys_->setLight(255, 210, 70);
}

void Game::dieTry() {
    if (rung_ || mode_ != Mode::Fall) return;
    dead_++;
    why_ = fate_ == Fate::Hot ? "HOT" : "SHORT";
    blip(fate_ == Fate::Hot ? 150.f : 90.f, 0.1f, 0.28f);
    if (sys_) {
        sys_->rumble(0.3f, 0.08f, 80);
        sys_->setLight(160, 36, 28);
        sys_->apu.noiseBurst(0.16f, 140.f, 0.08f);
    }
    if (dead_ >= 3) {
        why_ = "THIRD";
        won_ = false;
        over_ = true;
        mode_ = Mode::Over;
        return;
    }
    mode_ = Mode::Dead;
    deadT_ = 0.f;
}

void Game::blip(float freq, float vol, float hold) {
    if (!sys_) return;
    sys_->apu.tone(2, freq, vol);
    tickT_ = hold;
}

void Game::chord(float a, float b, float c, float hold) {
    if (!sys_) return;
    sys_->apu.tone(0, a, 0.10f);
    sys_->apu.tone(1, b, 0.08f);
    sys_->apu.tone(2, c, 0.07f);
    beep_ = hold;
}

void Game::hush() {
    beep_ = 0.f;
    tickT_ = 0.f;
    if (!sys_) return;
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(1, 0, 0);
    sys_->apu.tone(2, 0, 0);
}

void Game::tickAudio(float dt) {
    if (!sys_) return;
    const bool chiming = rung_ && (mode_ == Mode::Ring || mode_ == Mode::Leave || (mode_ == Mode::Over && won_));
    if (chiming && bellAmp_ > 0.35f) {
        bellTick_ -= dt;
        if (bellTick_ <= 0.f) {
            chord(784.f, 1175.f, 1568.f, 0.16f);
            bellTick_ = 0.28f;
        }
    }
    if (beep_ > 0.f) {
        beep_ -= dt;
        if (beep_ <= 0.f) {
            sys_->apu.tone(0, 0, 0);
            sys_->apu.tone(1, 0, 0);
            if (tickT_ <= 0.f) sys_->apu.tone(2, 0, 0);
        }
    }
    if (tickT_ > 0.f) {
        tickT_ -= dt;
        if (tickT_ <= 0.f && beep_ <= 0.f) sys_->apu.tone(2, 0, 0);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    clock_ += kDt;
    if (burstT_ > 0.f) burstT_ = std::max(0.f, burstT_ - kDt);
    bellPh_ += kDt * (rung_ ? 14.f : 2.1f);
    if (rung_) bellAmp_ = std::max(0.14f, bellAmp_ - kDt * 0.38f);
    tickAudio(kDt);

    if (mode_ == Mode::Title || mode_ == Mode::Aim) advanceMeter(kDt);

    const gs::Pad& pad = sys.pad;
    bool start = pad.pressed(gs::BTN_START);
    bool back = pad.pressed(gs::BTN_MODE);
    bool fire = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_TURBO);
    if (bot_) {
        start = false;
        back = false;
        fire = false;
    }

    const Mode before = mode_;
    if (mode_ == Mode::Pause && before == Mode::Pause) {
        if (start || fire) mode_ = held_;
        else if (back) toTitle();
    } else if (mode_ == Mode::Title && before == Mode::Title) {
        if (bot_ && clock_ > 0.40f) newGame();
        else if (back) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        } else if (start || fire) newGame();
    } else if (mode_ == Mode::Over && before == Mode::Over) {
        if (!bot_ && (start || fire)) newGame();
        else if (!bot_ && back) toTitle();
    } else if (mode_ == Mode::Aim && before == Mode::Aim && !bot_ && (start || back)) {
        held_ = Mode::Aim;
        mode_ = Mode::Pause;
    } else if (mode_ == Mode::Aim && before == Mode::Aim) {
        const Fate zone = classify(meter_);
        if (zone == Fate::Bell && !wasSweet_) blip(988.f, 0.05f, 0.05f);
        if (zone == Fate::Hot && wasFate_ != Fate::Hot) blip(170.f, 0.04f, 0.04f);
        wasSweet_ = zone == Fate::Bell;
        wasFate_ = zone;
        if (bot_ && zone == Fate::Bell && meterDir_ > 0) fire = true;
        if (fire) strike();
    }

    if (mode_ == before) {
        if (mode_ == Mode::Strike) {
            strikeT_ += kDt;
            if (strikeT_ >= kStrikeT) {
                if (fate_ == Fate::Bell) ring();
                else startFall();
            }
        } else if (mode_ == Mode::Fall) {
            fallT_ += kDt;
            if (fallT_ >= kFallT) dieTry();
        } else if (mode_ == Mode::Dead) {
            deadT_ += kDt;
            if (deadT_ >= kDeadT) beginAim();
        } else if (mode_ == Mode::Ring) {
            ringT_ += kDt;
            if (ringT_ >= kRingT) {
                mode_ = Mode::Leave;
                leaveT_ = 0.f;
                chord(392.f, 523.f, 659.f, 0.5f);
            }
        } else if (mode_ == Mode::Leave) {
            leaveT_ += kDt;
            kidX_ = std::min(132.f, kidX_ + 32.f * kDt);
            if (leaveT_ >= kLeaveT) {
                mode_ = Mode::Over;
                over_ = true;
                won_ = true;
            }
        }
    }
    draw();
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet, int fog, bool shadow) {
    if (!sys_ || h < 1.f || m.h < 1) return;
    const float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 40 || s.x + s.w < -40 || s.y > gs::SCREEN_H + 40 || s.y + s.h < -40) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(clampi(fog, 0, 16));
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, int fog, bool shadow) {
    if (!sys_ || w < 1.f || h < 1.f || m.h < 1) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(std::max(w, h));
    s.pal = uint8_t(pal);
    s.fog = uint8_t(clampi(fog, 0, 16));
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::word(const gs::Image& img, float cx, float cy, int pal) {
    if (!sys_ || img.w < 1 || img.h < 1) return;
    gs::Sprite s;
    s.w = int16_t(img.w);
    s.h = int16_t(img.h);
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
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
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
    const bool lit = mode_ == Mode::Leave || (mode_ == Mode::Over && won_) || mode_ == Mode::Ring;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.road[y].on = false;
        v.lineFog[y] = 0;
        if (y < 90) {
            float u = y / 89.f;
            int r = 1 + int(u * 5) + (lit ? 1 : 0);
            int g = 1 + int(u * 1);
            int b = 7 - int(u * 3);
            v.lineBackdrop[y] = gs::rgb4(clampi(r, 0, 15), clampi(g, 0, 15), clampi(b, 0, 15));
        } else if (y < 168) {
            float u = (y - 90) / 78.f;
            int r = 5 + int(u * 3);
            int g = 1 + int(u * 2);
            int b = 4 - int(u * 2);
            v.lineBackdrop[y] = gs::rgb4(clampi(r, 0, 15), clampi(g, 0, 15), clampi(b, 0, 15));
        } else if (y < 192) {
            v.lineBackdrop[y] = gs::rgb4(5, 2, 2);
        } else {
            v.lineBackdrop[y] = gs::rgb4(3, 2, 1);
        }
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = true;
    v.B.scroll(0, 0);
    backdrop();

    const float halfW = kTowerDrawH * float(kTowerImgW) / float(kTowerImgH) * 0.5f;
    const float plateY = trackY(0.5f * float(kGoldTop + kGoldBot));
    const float bellX = kTowerX + halfW + 18.f;
    const float bellSwing = std::sin(bellPh_) * (3.f + 12.f * bellAmp_);
    const bool inPlate = (mode_ == Mode::Aim || mode_ == Mode::Title || mode_ == Mode::Pause) &&
                         classify(meter_) == Fate::Bell;
    const bool showBell = inPlate || (mode_ == Mode::Strike && fate_ == Fate::Bell) || rung_;
    const int bellPal = showBell ? PAL_GOLD : PAL_BRASS;
    const float bellH = showBell ? 40.f : 34.f;

    const bool walking = mode_ == Mode::Leave || mode_ == Mode::Title;
    const int pose = walking && (int(clock_ * 6.f) & 1) ? 1 : 0;
    const float bob = mode_ == Mode::Title ? std::sin(clock_ * 3.f) * 1.1f : 0.f;
    const bool up = hammerUp();

    if (burstT_ > 0.f) {
        float u = 1.f - burstT_ / 0.6f;
        spr(art_.burst, bellX + bellSwing, plateY, 18.f + u * 28.f, PAL_GOLD, false, false, int(u * 12.f));
    }
    word(mode_ == Mode::Title ? art_.logo : gs::Image{}, mode_ == Mode::Title ? kTowerX : 0.f, 18.f, PAL_LOGO);

    spr(art_.puck, kTowerX, shownPuckY(), 15.f, PAL_RED);
    float lampH = inPlate ? 14.f : 8.f;
    spr(art_.bulb, kTowerX - halfW - 8.f, plateY, lampH, inPlate ? PAL_GOLD : PAL_BULB, false, false, inPlate ? 0 : 6);

    int showTry = rung_ ? tryNo_ : std::min(dead_ + 1, 3);
    for (int i = 0; i < 3; i++) {
        int state = 2;
        if (rung_) state = (i == tryNo_ - 1) ? 3 : (i < tryNo_ - 1 ? 0 : 2);
        else if (i < dead_) state = 0;
        else if (i == std::min(dead_, 2) && !rung_) state = 1;
        int pal = state == 0 ? PAL_RED : (state == 2 ? PAL_CREAM : PAL_GOLD);
        int fog = state == 0 ? 8 : (state == 2 ? 5 : 0);
        float h = (state == 1 || state == 3) ? 12.f : 8.f;
        if (mode_ == Mode::Title) {
            pal = (int(clock_ * 5.f) + i) & 1 ? PAL_GOLD : PAL_BULB;
            fog = 0;
            h = 9.f;
        }
        spr(art_.bulb, kTowerX - halfW - 10.f, 108.f + float(i) * 16.f, h, pal, false, false, fog);
    }

    spr(art_.hammer[up ? 1 : 0], up ? kidX_ + 28.f : kidX_ + 34.f, up ? 148.f : 168.f, up ? 40.f : 34.f, PAL_BRASS);
    spr(art_.kid[pose], kidX_, kTowerFeet + bob, 68.f, PAL_KID, false, true);
    spr(art_.bell, bellX + bellSwing, plateY, bellH, bellPal);
    spr(art_.tower, kTowerX, kTowerFeet, kTowerDrawH, PAL_WOOD, false, true);
    stamp(art_.awning, kTowerX, 38.f, 168.f, 22.f, PAL_AWN);

    const bool gateLit = mode_ == Mode::Leave || (mode_ == Mode::Over && won_);
    spr(art_.gate, 292.f, kTowerFeet, 86.f, PAL_GATE, false, true);
    spr(art_.bulb, 292.f, 118.f, gateLit ? 12.f : 7.f, gateLit ? PAL_GOLD : PAL_BULB, false, false, gateLit ? 0 : 4);

    spr(art_.balloon, 270.f, 62.f, 28.f, PAL_PINK, false, false, 2);
    spr(art_.balloon, 304.f, 78.f, 22.f, PAL_BLUE, false, false, 3);
    spr(art_.balloon, 24.f, 52.f, 24.f, PAL_RED, false, false, 4);

    const float fx = 48.f, fy = 108.f, rad = 36.f;
    spr(art_.hub, fx, fy, 28.f, PAL_BRASS, false, false, 5);
    for (int i = 0; i < 6; i++) {
        float a = clock_ * 0.55f + i * (PI * 2.f / 6.f);
        float cx = fx + std::cos(a) * rad;
        float cy = fy + std::sin(a) * rad * 0.72f;
        spr(art_.car, cx, cy, 14.f, (i & 1) ? PAL_RED : PAL_BLUE, false, false, 6);
    }
    spr(art_.stand, fx, 176.f, 78.f, PAL_NIGHT, false, true, 7);

    for (int i = 0; i < 5; i++) {
        float x = 12.f + float(i) * 16.f;
        int pal = (i % 3 == 0) ? PAL_RED : ((i % 3 == 1) ? PAL_GOLD : PAL_BLUE);
        spr(art_.pennant, x, 36.f, 14.f, pal);
    }
    spr(art_.pennant, 304.f, 40.f, 14.f, PAL_GOLD);
    for (int i = 0; i < 6; i++) {
        float x = 16.f + float(i) * 14.f;
        int blink = (int(clock_ * 6.f) + i) & 1;
        spr(art_.bulb, x, 26.f, blink ? 7.f : 5.f, blink ? PAL_BULB : PAL_GOLD, false, false, blink ? 0 : 8);
    }

    spr(art_.moon, 304.f, 20.f, 22.f, PAL_GOLD, false, false, 1);
    for (const auto& st : kStars) spr(art_.star, float(st[0]), float(st[1]), 6.f, PAL_HUD, false, false, 2);

    stamp(art_.shadow, kidX_, kTowerFeet + 2.f, 36.f, 10.f, PAL_HUD, 0, true);
    stamp(art_.shadow, kTowerX, kTowerFeet + 1.f, 70.f, 12.f, PAL_HUD, 0, true);

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(24, "RING BEFORE THE THIRD TRY DIES", PAL_GOLD);
        hudC(25, "GOLD RINGS  SHORT AND HOT DIE", PAL_HUD);
        hudC(26, "ENTER START", PAL_HUD);
        hudC(27, "Z STRIKE", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Pause) {
        hudC(3, "PAUSED", PAL_GOLD);
        hudC(26, "ENTER RESUME", PAL_HUD);
        hudC(27, "ESC TITLE", PAL_HUD);
    }

    hud(1, 0, "S3 FAIRBELL", PAL_GOLD);
    std::snprintf(buf, sizeof buf, "TRY %d", showTry);
    hud(33, 0, buf, PAL_HUD);
    std::snprintf(buf, sizeof buf, "DEAD %d", std::min(dead_, 3));
    hud(1, 1, buf, dead_ > 0 ? PAL_BAD : PAL_HUD);

    const char* call = nullptr;
    int callPal = PAL_GOLD;
    if (mode_ == Mode::Aim && inPlate) call = "NOW";
    else if (mode_ == Mode::Strike && fate_ == Fate::Bell) call = "BELL";
    else if (mode_ == Mode::Fall || mode_ == Mode::Dead) {
        call = fate_ == Fate::Hot ? "HOT" : "SHORT";
        callPal = PAL_BAD;
    } else if (mode_ == Mode::Ring) call = "BELL";
    else if (mode_ == Mode::Leave || (mode_ == Mode::Over && won_)) call = "YOU LEAVE";
    else if (mode_ == Mode::Over) call = "BELL SILENT";
    if (call && mode_ != Mode::Pause) hudC(3, call, callPal);

    if (mode_ == Mode::Over && !won_) {
        hudC(25, "THE THIRD TRY DIED", PAL_BAD);
        hudC(26, "BELL SILENT", PAL_HUD);
        hudC(27, "ENTER PLAYS AGAIN", PAL_HUD);
    } else if (mode_ == Mode::Leave || (mode_ == Mode::Over && won_)) {
        std::snprintf(buf, sizeof buf, "BELL ON TRY %d", tryNo_);
        hudC(25, buf, PAL_GOLD);
        hudC(26, "YOU LEAVE THE FAIR", PAL_HUD);
        hudC(27, "ENTER PLAYS AGAIN", PAL_HUD);
    } else if (mode_ != Mode::Pause) {
        hudC(26, "STOP THE PUCK ON THE GOLD", PAL_HUD);
        hudC(27, "Z STRIKE", PAL_GOLD);
    }
}

}  // namespace fairbell

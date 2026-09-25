#include "game/arch.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace arch {
namespace {

constexpr int kFullDraw = 24;
constexpr int kWobbleAt = 18;
constexpr int kFlight = 11;

// A full draw that stays on the pin, and never walks into the wind, scores 298.
// The line is 300, so the dot has to reach the gold.
constexpr int kWind[kArrows] = {
    0,  2, -3,  4, -1,  3, -4,  1, -2,  4,  0, -3,  2, -4,  3, -1, 1, -4,  2,  3,  0, -2,
    4, -3,  3, -1,  4, -2,  1, -3,  2,  4, -4,  0,  3, -2,
};

static_assert(sizeof(kWind) / sizeof(kWind[0]) == kArrows, "one wind per arrow");

float clampf(float v, float a, float b) {
    if (v < a) return a;
    if (v > b) return b;
    return v;
}

void nudge(int n, float& ox, float& oy) {
    if (n <= 0) {
        ox = oy = 0.f;
        return;
    }
    float rad = 6.3f * std::sqrt(float(n) / float(kArrows - 1));
    float a = float(n) * 2.3999632f;
    ox = std::cos(a) * rad;
    oy = std::sin(a) * rad;
    float r = std::sqrt(ox * ox + oy * oy);
    if (r > 6.f) {
        ox *= 6.f / r;
        oy *= 6.f / r;
    }
}

}  // namespace

int Game::marker() const {
    if (phase_ == Phase::Title) return 0;
    if (phase_ == Phase::Aim || phase_ == Phase::Nock) return 1;
    if (phase_ == Phase::Win) return 3;
    if (phase_ == Phase::Lose) return 4;
    return 2;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.apu.setMaster(0.75f);
    phase_ = Phase::Title;
    over_ = false;
    won_ = false;
    score_ = 0;
    golds_ = 0;
    shot_ = 0;
    t_ = 0;
    marks_.clear();
    marks_.reserve(size_t(kArrows));
}

void Game::queueTune(const float* hz, const int* frames, int n) {
    tuneHz_.clear();
    tuneN_.clear();
    tuneLeft_ = 0;
    for (int i = 0; i < n; i++) {
        tuneHz_.push_back(hz[i]);
        tuneN_.push_back(frames[i]);
    }
}

bool Game::drawHeld() const {
    if (!sys_) return false;
    if (sys_->pad.down(gs::BTN_A) || sys_->pad.down(gs::BTN_C)) return true;
    return sys_->pad.accel > 0.45f;
}

void Game::steer() {
    if (!sys_) return;
    float x = sys_->pad.axisX;
    float y = 0.f;
    if (sys_->pad.down(gs::BTN_LEFT)) x = -1.f;
    if (sys_->pad.down(gs::BTN_RIGHT)) x = 1.f;
    if (sys_->pad.down(gs::BTN_UP)) y = -1.f;
    if (sys_->pad.down(gs::BTN_DOWN)) y = 1.f;
    float mag = std::sqrt(x * x + y * y);
    if (mag > 1.f) {
        x /= mag;
        y /= mag;
    }
    float sp = sys_->pad.down(gs::BTN_TURBO) ? 4.6f : 2.8f;
    aimX_ = clampf(aimX_ + x * sp, kCx - 100.f, kCx + 100.f);
    aimY_ = clampf(aimY_ + y * sp, kCy - 88.f, kCy + 88.f);
    if (sys_->pad.pressed(gs::BTN_B)) {
        aimX_ = kCx;
        aimY_ = kCy;
    }
}

Game::Shot Game::predict() const {
    float power = 1.f;
    int steady = 0;
    if (phase_ == Phase::Nock) {
        power = std::min(1.f, float(drawTick_) / float(kFullDraw));
        steady = steady_;
    }
    float ox = 0.f;
    float oy = 0.f;
    if (steady > kWobbleAt) {
        float mag = std::min(22.f, float(steady - kWobbleAt) * 0.65f);
        float w = std::sin(float(steady) * 0.41f) * mag;
        ox = w * 0.22f;
        oy = w;
    }
    Shot s;
    s.x = aimX_ + float(wind_) * kWindPx + ox;
    s.y = aimY_ + (1.f - power) * kMissDrop + oy;
    float dx = s.x - kCx;
    float dy = s.y - kCy;
    s.ring = ringAt(std::sqrt(dx * dx + dy * dy));
    return s;
}

void Game::beginArrow() {
    if (shot_ < 0 || shot_ >= kArrows) return;
    wind_ = kWind[shot_];
    aimX_ = kCx;
    aimY_ = kCy;
    drawTick_ = 0;
    steady_ = 0;
    phase_ = Phase::Aim;
}

void Game::beginRound() {
    score_ = 0;
    golds_ = 0;
    shot_ = 0;
    marks_.clear();
    won_ = false;
    over_ = false;
    last_ = {};
    pending_ = {};
    beginArrow();
    skipHold_ = !bot_;
    if (!sys_) return;
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(2, 0, 0);
}

void Game::enterNock() {
    phase_ = Phase::Nock;
    drawTick_ = 0;
    steady_ = 0;
    if (!sys_) return;
    sys_->apu.tone(0, 160.f, 0.03f);
    blipLeft_ = 4;
}

void Game::startBotNock() {
    float ox = 0.f;
    float oy = 0.f;
    nudge(shot_, ox, oy);
    aimX_ = kCx - float(wind_) * kWindPx + ox;
    aimY_ = kCy + oy;
    enterNock();
    nockTick();
}

void Game::humanAim() {
    steer();
    if (skipHold_) {
        if (!drawHeld()) skipHold_ = false;
        return;
    }
    if (!drawHeld()) return;
    enterNock();
    nockTick();
}

void Game::nockTick() {
    bool release = bot_ ? (drawTick_ >= kFullDraw && steady_ == 0) : !drawHeld();
    if (release) {
        loose();
        return;
    }
    if (drawTick_ < kFullDraw) {
        drawTick_++;
        if (drawTick_ == kFullDraw && sys_) {
            sys_->apu.tone(0, 880.f, 0.045f);
            blipLeft_ = 5;
        }
    } else {
        steady_++;
    }
}

void Game::loose() {
    pending_ = predict();
    phase_ = Phase::Flight;
    flightT_ = 0;
    if (!sys_) return;
    sys_->apu.tone(2, 0, 0);
    sys_->apu.tone(0, 220.f, 0.05f);
    blipLeft_ = 7;
    sys_->rumble(0.12f, 0.22f, 28);
}

void Game::flightTick() {
    if (++flightT_ > kFlight) arrive();
}

void Game::arrive() {
    last_ = pending_;
    marks_.push_back(pending_);
    score_ += pending_.ring.points;
    if (pending_.ring.gold) golds_++;
    shot_++;
    phase_ = Phase::Call;
    callT_ = 0;
    if (!sys_) return;
    bool gold = pending_.ring.gold;
    sys_->apu.noiseBurst(gold ? 0.1f : 0.18f, gold ? 2400.f : 760.f, 0.07f);
    if (gold) {
        const float hz[] = {659.25f, 783.99f, 987.77f};
        const int fr[] = {5, 5, 10};
        queueTune(hz, fr, 3);
        sys_->rumble(0.25f, 0.7f, 48);
        sys_->setLight(255, 190, 30);
    } else if (pending_.ring.points > 0) {
        const float hz[] = {280.f + float(pending_.ring.points) * 32.f};
        const int fr[] = {7};
        queueTune(hz, fr, 1);
    } else {
        const float hz[] = {130.f};
        const int fr[] = {8};
        queueTune(hz, fr, 1);
    }
}

void Game::finish() {
    won_ = score_ >= kLine;
    over_ = true;
    phase_ = won_ ? Phase::Win : Phase::Lose;
    if (!sys_) return;
    if (won_) {
        const float hz[] = {523.25f, 659.25f, 783.99f, 1046.5f};
        const int fr[] = {7, 7, 7, 16};
        queueTune(hz, fr, 4);
        sys_->setLight(255, 200, 40);
    } else {
        const float hz[] = {392.f, 329.63f, 261.63f};
        const int fr[] = {8, 8, 16};
        queueTune(hz, fr, 3);
        sys_->setLight(140, 30, 20);
    }
}

void Game::callTick() {
    int limit = bot_ ? 5 : 34;
    if (shot_ > 0 && shot_ % 6 == 0) limit = bot_ ? 10 : 64;
    if (++callT_ < limit) return;
    if (shot_ >= kArrows) {
        finish();
        return;
    }
    beginArrow();
}

void Game::endTick() {
    if (bot_ || !sys_) return;
    if (sys_->pad.pressed(gs::BTN_START) || sys_->pad.pressed(gs::BTN_A) || sys_->pad.pressed(gs::BTN_C)) beginRound();
}

void Game::pumpAudio() {
    if (!sys_) return;
    if (blipLeft_ > 0 && --blipLeft_ == 0) sys_->apu.tone(0, 0, 0);
    if (phase_ == Phase::Nock) {
        float p = std::min(1.f, float(drawTick_) / float(kFullDraw));
        sys_->apu.tone(2, 90.f + p * 150.f, 0.018f);
    } else if (phase_ != Phase::Flight) {
        sys_->apu.tone(2, 0, 0);
    }
    if (tuneHz_.empty()) return;
    if (tuneLeft_ == 0) {
        float hz = tuneHz_.front();
        int frames = tuneN_.front();
        tuneHz_.erase(tuneHz_.begin());
        tuneN_.erase(tuneN_.begin());
        sys_->apu.tone(1, hz, hz > 0.f ? 0.055f : 0.f);
        tuneLeft_ = frames;
    }
    if (--tuneLeft_ <= 0) {
        tuneLeft_ = 0;
        if (tuneHz_.empty()) sys_->apu.tone(1, 0, 0);
    }
}

const gs::Image& Game::dotFor(const Ring& ring) const {
    if (ring.face >= 9) return art_.dot[0];
    if (ring.face == 7) return art_.dot[1];
    if (ring.face == 5) return art_.dot[2];
    if (ring.face == 3) return art_.dot[3];
    if (ring.face == 1) return art_.dot[4];
    return art_.dot[5];
}

void Game::stamp(const gs::Image& img, float x, float y, float w, float h, int pal, bool flip, bool shadow) {
    if (!sys_ || img.w == 0 || w < 1.f || h < 1.f) return;
    gs::Sprite s;
    s.x = int16_t(std::lround(x));
    s.y = int16_t(std::lround(y));
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.img = img;
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::at(const gs::Image& img, float cx, float cy, int pal) {
    stamp(img, cx - float(img.w) * 0.5f, cy - float(img.h) * 0.5f, float(img.w), float(img.h), pal);
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
    int n = 0;
    if (s)
        while (s[n]) n++;
    int col = (40 - n) / 2;
    if (col < 0) col = 0;
    hud(col, row, s, pal);
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        if (y < 132) {
            float t = float(y) / 132.f;
            int r = 3 + int(t * 9.f);
            int g = 5 + int(t * 6.f);
            int b = 12 - int(t * 4.f);
            v.lineBackdrop[y] = gs::rgb4(r, g, b);
        } else if (y < 176) {
            v.lineBackdrop[y] = gs::rgb4(3, 8, 3);
        } else {
            v.lineBackdrop[y] = (y & 1) ? gs::rgb4(2, 7, 3) : gs::rgb4(3, 9, 4);
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
    backdrop();

    Shot pred{};
    bool aiming = phase_ == Phase::Aim || phase_ == Phase::Nock;
    if (aiming) pred = predict();

    if (phase_ == Phase::Title) stamp(art_.wordArch, 54.f, 30.f, float(art_.wordArch.w), float(art_.wordArch.h), PAL_WORD);
    if (phase_ == Phase::Win)
        stamp(art_.wordWin, (gs::SCREEN_W - art_.wordWin.w) * 0.5f, 16.f, float(art_.wordWin.w), float(art_.wordWin.h), PAL_WORD);
    if (phase_ == Phase::Lose)
        stamp(art_.wordShort, (gs::SCREEN_W - art_.wordShort.w) * 0.5f, 16.f, float(art_.wordShort.w), float(art_.wordShort.h),
              PAL_SHORT);

    stamp(art_.badge, 8.f, 22.f, float(art_.badge.w), float(art_.badge.h), PAL_BADGE);

    if (aiming) {
        float power = phase_ == Phase::Nock ? std::min(1.f, float(drawTick_) / float(kFullDraw)) : 0.f;
        if (power > 0.02f) {
            uint16_t col = gs::rgb4(15, 14, 10);
            if (pred.ring.gold) col = gs::rgb4(15, 12, 2);
            else if (steady_ > kWobbleAt) col = gs::rgb4(14, 4, 3);
            v.setColor(PAL_FILL * 16 + 1, col);
            stamp(art_.px, 8.f, 190.f, 112.f * power, 7.f, PAL_FILL);
        }
        stamp(art_.px, 8.f, 190.f, 112.f, 7.f, PAL_TRACK);

        uint16_t ghost = gs::rgb4(8, 4, 3);
        if (pred.ring.gold) ghost = gs::rgb4(15, 13, 2);
        else if (pred.ring.face == 7) ghost = gs::rgb4(15, 4, 3);
        else if (pred.ring.face == 5) ghost = gs::rgb4(6, 8, 15);
        else if (pred.ring.face == 3) ghost = gs::rgb4(9, 9, 11);
        else if (pred.ring.face == 1) ghost = gs::rgb4(15, 15, 15);
        v.setColor(PAL_GHOST * 16 + 1, ghost);
        bool hot = phase_ == Phase::Nock && pred.ring.gold;
        v.setColor(PAL_SIGHT * 16 + 1, hot ? gs::rgb4(15, 13, 3) : gs::rgb4(15, 15, 15));
        at(art_.sight, aimX_, aimY_, PAL_SIGHT);
        at(art_.ghost, pred.x, pred.y, PAL_GHOST);
    }

    if (phase_ == Phase::Flight) {
        float u = std::min(1.f, float(flightT_) / float(kFlight));
        float x = kLooseX + (pending_.x - kLooseX) * u;
        float y = kLooseY + (pending_.y - kLooseY) * u;
        y -= std::sin(u * 3.14159265f) * 8.f;
        stamp(art_.arrow, x - float(art_.arrow.w), y - float(art_.arrow.h) * 0.5f, float(art_.arrow.w), float(art_.arrow.h),
              PAL_ARROW);
    }

    for (const Shot& m : marks_) at(dotFor(m.ring), m.x, m.y, PAL_MARK);

    int pose = 0;
    if (phase_ == Phase::Nock) pose = 1;
    else if (phase_ == Phase::Flight) pose = 2;
    stamp(art_.archer[pose], kArchX, kArchY, float(kArchW), float(kArchH), PAL_ARCH);
    stamp(art_.face, kCx - kFaceMid, kCy - kFaceMid, float(kFace), float(kFace), PAL_FACE);
    stamp(art_.stand, kCx - 32.f, kCy + 28.f, float(art_.stand.w), float(art_.stand.h), PAL_WORLD);

    if (phase_ != Phase::Title) {
        int mag = wind_ < 0 ? -wind_ : wind_;
        float bob = std::sin(float(t_) * 0.22f) * float(mag) * 0.55f;
        const gs::Image& flag = wind_ == 0 ? art_.limp : art_.flag;
        stamp(flag, 102.f, 34.f + bob, float(flag.w), float(flag.h), PAL_WORLD, wind_ < 0);
    }

    stamp(art_.bush, 116.f, 166.f, float(art_.bush.w), float(art_.bush.h), PAL_WORLD);
    stamp(art_.hut, 0.f, 122.f, float(art_.hut.w), float(art_.hut.h), PAL_WORLD);
    stamp(art_.tree, -2.f, 96.f, float(art_.tree.w), float(art_.tree.h), PAL_WORLD);
    stamp(art_.tree, 278.f, 98.f, float(art_.tree.w), float(art_.tree.h), PAL_WORLD);
    stamp(art_.cloud, 150.f, 4.f, float(art_.cloud.w), float(art_.cloud.h), PAL_WORLD);
    stamp(art_.cloud, 236.f, 8.f, float(art_.cloud.w) * 0.8f, float(art_.cloud.h) * 0.8f, PAL_WORLD);
    stamp(art_.sun, 292.f, 18.f, float(art_.sun.w), float(art_.sun.h), PAL_WORLD);
    stamp(art_.hill, 0.f, 126.f, float(art_.hill.w), float(art_.hill.h), PAL_WORLD);
    stamp(art_.shadow, 24.f, 176.f, 52.f, 14.f, PAL_WORLD, false, true);
    stamp(art_.shadow, kCx - 30.f, 180.f, 60.f, 14.f, PAL_WORLD, false, true);

    char line[48];
    if (phase_ == Phase::Title) {
        hud(1, 0, "THIRTY-SIX ARROWS", PAL_HUD);
        hud(1, 1, "GOLD COUNTS DOUBLE", PAL_HUD_GOLD);
        hudC(25, "LINE 300   WIND MOVES THE DOT", PAL_HUD);
        hudC(26, "PUT THE DOT IN THE GOLD", PAL_HUD_GOLD);
        hudC(27, "Z DRAWS  LET GO LOOSES  START", PAL_HUD);
        return;
    }
    if (phase_ == Phase::Win) {
        hud(1, 0, "S3 ARCH", PAL_HUD_GOLD);
        hud(1, 1, "GOLD COUNTS DOUBLE", PAL_HUD_GOLD);
        hudC(25, "MADE THE LINE", PAL_HUD_GOLD);
        std::snprintf(line, sizeof line, "SCORE %d   GOLDS %d", score_, golds_);
        hudC(26, line, PAL_HUD);
        hudC(27, "START SHOOTS AGAIN", PAL_HUD);
        return;
    }
    if (phase_ == Phase::Lose) {
        int gap = kLine - score_;
        if (gap < 0) gap = 0;
        hud(1, 0, "S3 ARCH", PAL_HUD);
        hud(1, 1, "GOLD COUNTS DOUBLE", PAL_HUD_GOLD);
        std::snprintf(line, sizeof line, "SHORT BY %d", gap);
        hudC(25, line, PAL_HUD_ALERT);
        std::snprintf(line, sizeof line, "SCORE %d   LINE %d", score_, kLine);
        hudC(26, line, PAL_HUD);
        hudC(27, "START TRIES AGAIN", PAL_HUD);
        return;
    }

    int shown = shot_ + 1;
    if (phase_ == Phase::Call) shown = shot_;
    if (shown < 1) shown = 1;
    if (shown > kArrows) shown = kArrows;
    std::snprintf(line, sizeof line, "ARROW %02d/%d   SCORE %d", shown, kArrows, score_);
    hud(1, 0, line, PAL_HUD);
    if (wind_ == 0) std::snprintf(line, sizeof line, "WIND CALM   LINE %d", kLine);
    else if (wind_ > 0) std::snprintf(line, sizeof line, "WIND +%d RIGHT   LINE %d", wind_, kLine);
    else std::snprintf(line, sizeof line, "WIND %d LEFT   LINE %d", wind_, kLine);
    hud(1, 1, line, PAL_HUD);

    if (phase_ == Phase::Aim && pred.ring.gold) hudC(25, "HOLD Z TO DRAW", PAL_HUD_GOLD);
    else if (phase_ == Phase::Aim) hudC(25, "MOVE THE DOT ONTO THE GOLD", PAL_HUD);
    else if (phase_ == Phase::Nock && pred.ring.gold) hudC(25, "LOOSE", PAL_HUD_GOLD);
    else if (phase_ == Phase::Nock && steady_ > kWobbleAt) hudC(25, "WAIT FOR THE GOLD", PAL_HUD_ALERT);
    else if (phase_ == Phase::Nock) hudC(25, "HOLD THE DRAW", PAL_HUD);
    else if (phase_ == Phase::Flight) hudC(25, "LOOSED", PAL_HUD);
    else if (phase_ == Phase::Call && last_.ring.name) {
        if (last_.ring.gold) std::snprintf(line, sizeof line, "GOLD %d x2 = %d", last_.ring.face, last_.ring.points);
        else if (last_.ring.points > 0) std::snprintf(line, sizeof line, "%s %d", last_.ring.name, last_.ring.points);
        else std::snprintf(line, sizeof line, "MISS");
        hudC(25, line, last_.ring.gold ? PAL_HUD_GOLD : (last_.ring.points ? PAL_HUD : PAL_HUD_ALERT));
    }

    if (phase_ == Phase::Call && shot_ % 6 == 0 && shot_ < kArrows) {
        std::snprintf(line, sizeof line, "END %d OF 6   SCORE %d", shot_ / 6, score_);
        hudC(26, line, PAL_HUD_GOLD);
    } else {
        std::snprintf(line, sizeof line, "GOLDS %d", golds_);
        hudC(26, line, PAL_HUD_GOLD);
    }
    hudC(27, "Z DRAWS  LET GO LOOSES  B CENTERS", PAL_HUD);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_++;

    if (!bot_ && sys.pad.pressed(gs::BTN_MODE)) {
        if (phase_ == Phase::Title) {
            if (sys.hasHome()) sys.eject();
        } else {
            phase_ = Phase::Title;
            over_ = false;
            won_ = false;
            sys.apu.tone(0, 0, 0);
            sys.apu.tone(1, 0, 0);
            sys.apu.tone(2, 0, 0);
            tuneHz_.clear();
            tuneN_.clear();
            tuneLeft_ = 0;
        }
    }

    bool started = false;
    if (phase_ == Phase::Title) {
        bool go = bot_ ? (t_ >= 24) : (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A) || sys.pad.pressed(gs::BTN_C));
        if (go) {
            beginRound();
            started = true;
        }
    }

    if (phase_ == Phase::Aim && !started) {
        if (bot_) startBotNock();
        else humanAim();
    } else if (phase_ == Phase::Nock) {
        if (!bot_) steer();
        nockTick();
    } else if (phase_ == Phase::Flight) {
        flightTick();
    } else if (phase_ == Phase::Call) {
        callTick();
        if (bot_ && phase_ == Phase::Aim) startBotNock();
    } else if (phase_ == Phase::Win || phase_ == Phase::Lose) {
        endTick();
    }

    if (started && bot_) startBotNock();

    pumpAudio();
    draw();
}

}  // namespace arch

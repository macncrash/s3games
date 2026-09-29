#include "game/fishbell.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace fishbell {
namespace {

constexpr float kDt = 1.f / 60.f;

float fishAt(float swimT) {
    if (swimT < kFishDelay) return kFishX0;
    return kFishX0 + (swimT - kFishDelay) * kFishSpd;
}

float weedAt(float swimT) { return kWeedX0 + swimT * kWeedSpd; }

}  // namespace

const char* Game::fateName(Fate f) {
    switch (f) {
    case Fate::Ring: return "BELL";
    case Fate::Short: return "SHORT";
    case Fate::Snag: return "SNAG";
    case Fate::Miss: return "MISS";
    }
    return "MISS";
}

bool Game::prove() {
    float tWeed = (kHookHome - kWeedX0) / kWeedSpd;
    float tFish = kFishDelay + (kHookHome - kFishX0) / kFishSpd;
    if (!(tFish > tWeed + 0.45f)) {
        std::fprintf(stderr, "s3fishbell weed still on the hook when the fish arrives\n");
        return false;
    }
    if (!(tFish < 8.f && tFish > 1.f)) {
        std::fprintf(stderr, "s3fishbell the keeper never crosses\n");
        return false;
    }
    float fish = fishAt(tFish);
    float weed = weedAt(tFish);
    if (std::fabs(fish - kHookHome) > 0.6f) {
        std::fprintf(stderr, "s3fishbell fish misses the home hook\n");
        return false;
    }
    if (std::fabs(weed - kHookHome) < kSnag + 8.f) {
        std::fprintf(stderr, "s3fishbell a clean strike still snags\n");
        return false;
    }
    // A strike before the fish arrives, on the weed, is not a ring.
    float early = tWeed;
    if (std::fabs(fishAt(early) - kHookHome) < kStrike && std::fabs(weedAt(early) - kHookHome) >= kSnag) {
        std::fprintf(stderr, "s3fishbell an early strike still meets the fish\n");
        return false;
    }
    rules_ = true;
    return true;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    loadArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    for (int y = 0; y < gs::SCREEN_H; y++) sys.vdp.road[y].on = false;
    if (!prove()) {
        rules_ = false;
        why_ = "RULES";
    }
    toTitle();
    if (bot_ && rules_) begin();
}

Game::Input Game::readPad(const gs::Pad& pad) const {
    Input in;
    in.strike = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_Z) || pad.pressed(gs::BTN_C);
    in.start = pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A);
    float x = 0;
    if (pad.down(gs::BTN_LEFT)) x -= 1.f;
    if (pad.down(gs::BTN_RIGHT)) x += 1.f;
    if (std::fabs(pad.axisX) > 0.25f) x = pad.axisX;
    in.x = std::clamp(x, -1.f, 1.f);
    return in;
}

Game::Input Game::botInput() const {
    Input in;
    if (mode_ == Mode::Title) {
        in.start = true;
        return in;
    }
    if (mode_ != Mode::Swim || struck_) return in;
    float fish = fishX_;
    float weed = weedX_;
    bool weedNear = std::fabs(weed - hookX_) < kSnag + 6.f && weed > -8.f && weed < 330.f;
    bool fishNear = std::fabs(fish - hookX_) < kStrike - 1.5f && fish > 24.f && fish < 300.f;
    if (fishNear && !weedNear) in.strike = true;
    return in;
}

void Game::fresh() {
    dead_ = 0;
    tryNo_ = 0;
    fanStep_ = -1;
    rung_ = won_ = over_ = struck_ = false;
    whyFate_ = Fate::Miss;
    why_ = "OPEN";
    bellAmp_ = 0.16f;
    bellPh_ = 0;
    bellTick_ = toneT_ = fanT_ = 0;
    swimT_ = deadT_ = ringT_ = leaveT_ = 0;
    t_ = 0;
    hookX_ = kHookHome;
    fishX_ = kFishX0;
    weedX_ = kWeedX0;
}

void Game::toTitle() {
    fresh();
    mode_ = Mode::Title;
    if (sys_) sys_->setLight(20, 50, 80);
}

void Game::cast() {
    struck_ = false;
    swimT_ = 0;
    hookX_ = kHookHome;
    fishX_ = kFishX0;
    weedX_ = kWeedX0;
    mode_ = Mode::Swim;
    why_ = "SWIM";
    blip(392.f, 0.05f, 0.06f);
}

void Game::begin() {
    fresh();
    cast();
    if (sys_) sys_->setLight(30, 80, 120);
}

void Game::ring() {
    if (rung_ || mode_ != Mode::Swim) return;
    rung_ = true;
    won_ = true;
    tryNo_ = dead_ + 1;
    whyFate_ = Fate::Ring;
    why_ = "BELL";
    bellAmp_ = 1.f;
    bellTick_ = 0.02f;
    ringT_ = 1.05f;
    fanT_ = 0;
    fanStep_ = -1;
    mode_ = Mode::Ring;
    if (sys_) {
        sys_->rumble(0.35f, 0.7f, 160);
        sys_->setLight(255, 190, 60);
    }
}

void Game::dieTry(Fate why) {
    if (rung_ || mode_ != Mode::Swim) return;
    dead_++;
    whyFate_ = why;
    why_ = fateName(why);
    blip(110.f, 0.08f, 0.18f);
    if (dead_ >= 3) {
        over_ = true;
        won_ = false;
        mode_ = Mode::Over;
        why_ = "DEAD";
        if (sys_) sys_->setLight(80, 10, 10);
        return;
    }
    deadT_ = 0.85f;
    mode_ = Mode::Dead;
}

void Game::blip(float freq, float vol, float hold) {
    if (!sys_) return;
    sys_->apu.tone(0, freq, vol);
    toneT_ = hold;
}

void Game::tickAudio(float dt) {
    if (!sys_) return;
    bool chiming = rung_ && (mode_ == Mode::Ring || mode_ == Mode::Leave || (mode_ == Mode::Over && won_));
    bellPh_ += dt * (chiming ? 14.f : 1.8f);
    if (chiming) bellAmp_ = std::max(0.34f, bellAmp_ - dt * 0.22f);
    else bellAmp_ = 0.14f;
    if (chiming) {
        bellTick_ -= dt;
        if (bellTick_ <= 0.f && bellAmp_ > 0.4f) {
            sys_->apu.tone(1, 880.f, 0.07f + 0.06f * bellAmp_);
            bellTick_ = 0.22f;
        }
        fanT_ += dt;
        static const float notes[] = {523.25f, 659.25f, 783.99f, 1046.5f};
        int step = int(fanT_ / 0.16f);
        if (step != fanStep_ && step >= 0 && step < 4) {
            sys_->apu.tone(0, notes[step], 0.09f);
            fanStep_ = step;
        }
    } else if (toneT_ > 0.f) {
        toneT_ -= dt;
        if (toneT_ <= 0.f) sys_->apu.tone(0, 0, 0);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    Input in = bot_ ? botInput() : readPad(sys.pad);
    t_ += kDt;
    tickAudio(kDt);

    if (mode_ == Mode::Title) {
        if (in.start) begin();
    } else if (mode_ == Mode::Swim) {
        hookX_ = std::clamp(hookX_ + in.x * 78.f * kDt, 48.f, 292.f);
        swimT_ += kDt;
        fishX_ = fishAt(swimT_);
        weedX_ = weedAt(swimT_);
        if (!struck_ && in.strike) {
            struck_ = true;
            bool onFish = std::fabs(fishX_ - hookX_) <= kStrike && fishX_ > 16.f;
            bool onWeed = std::fabs(weedX_ - hookX_) <= kSnag && weedX_ > -4.f && weedX_ < 324.f;
            if (onWeed && !onFish) dieTry(Fate::Snag);
            else if (onFish && !onWeed) ring();
            else if (onFish && onWeed) dieTry(Fate::Snag);
            else if (fishX_ < hookX_ - kStrike) dieTry(Fate::Short);
            else dieTry(Fate::Miss);
        } else if (!struck_ && fishX_ > 328.f) {
            dieTry(Fate::Miss);
        }
    } else if (mode_ == Mode::Dead) {
        deadT_ -= kDt;
        if (deadT_ <= 0.f) cast();
    } else if (mode_ == Mode::Ring) {
        ringT_ -= kDt;
        if (ringT_ <= 0.f) {
            mode_ = Mode::Leave;
            leaveT_ = 0.55f;
        }
    } else if (mode_ == Mode::Leave) {
        leaveT_ -= kDt;
        if (leaveT_ <= 0.f) {
            over_ = true;
            mode_ = Mode::Over;
        }
    } else if (mode_ == Mode::Over) {
        if (!bot_ && (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A))) begin();
    }

    draw();
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal) {
    if (!sys_ || h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 40 || s.x + s.w < -40 || s.y > gs::SCREEN_H + 40 || s.y + s.h < -40) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!sys_ || !s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        if (x < 0 || x > 39) continue;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < 32 || c > 127) c = ' ';
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    if (!s) return;
    int n = int(std::strlen(s));
    hud((40 - n) / 2, row, s, pal);
}

void Game::backdrop() {
    if (!sys_) return;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t c;
        if (y < 78) {
            int g = 6 + y / 18;
            c = gs::rgb4(1, 2 + y / 40, std::min(12, g));
        } else if (y < 108) {
            c = ((y / 2) & 1) ? gs::rgb4(2, 8, 11) : gs::rgb4(1, 6, 10);
        } else {
            int d = (y - 108) / 16;
            c = gs::rgb4(0, std::max(2, 6 - d), std::max(4, 9 - d));
        }
        sys_->vdp.lineBackdrop[y] = c;
        sys_->vdp.lineFog[y] = 0;
        sys_->vdp.road[y].on = false;
    }
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    float amp = rung_ ? bellAmp_ : 0.14f;
    float swing = std::sin(bellPh_) * 7.f * amp;
    float bx = 160.f + swing;
    float by = 36.f;

    // Foreground first: earlier sprites sit on top.
    spr(art_.hook, hookX_, 158.f, 16.f, PAL_ROD);
    spr(art_.bobber, hookX_, 118.f, 14.f, PAL_ALERT);
    spr(art_.rod, 36.f, 86.f, 52.f, PAL_ROD);
    if (weedX_ > -30.f && weedX_ < 350.f) spr(art_.weed, weedX_, kWeedY, 26.f, PAL_WEED);
    if (fishX_ > -40.f && fishX_ < 360.f) spr(art_.fish, fishX_, kFishY, 22.f, PAL_FISH);

    for (int i = 0; i < 5; i++) {
        float px = 24.f + float(i) * 68.f;
        spr(art_.post, px, 100.f, 34.f, PAL_ROD);
    }
    float ripple = 8.f + std::sin(t_ * 3.f + hookX_ * 0.05f) * 1.f;
    spr(art_.ripple, hookX_, 112.f, ripple, PAL_WATER);

    spr(art_.yoke, 160.f, 16.f, 8.f, PAL_BELL);
    spr(art_.bell, bx, by, 28.f, PAL_BELL);
    float clap = std::sin(bellPh_ + 0.6f) * 5.f * amp;
    spr(art_.clapper, bx + clap, by + 6.f, 10.f, PAL_BELL);

    hud(1, 0, "S3 FISHBELL", PAL_GOLD);
    if (mode_ == Mode::Title) {
        hud(31, 0, "3 TRIES", PAL_GOLD);
        hudC(24, "HOOK THE FISH NOT THE WEED", PAL_HUD);
        hudC(26, "THE BELL RINGS OR THE TRY DIES", PAL_HUD);
        hudC(27, "RETURN STARTS", PAL_GOLD);
        return;
    }

    char buf[48];
    int showTry = rung_ ? tryNo_ : std::min(dead_ + 1, 3);
    std::snprintf(buf, sizeof buf, "TRY %d OF 3", showTry);
    hud(28, 0, buf, dead_ == 2 && !rung_ ? PAL_ALERT : PAL_GOLD);

    const char* msg = "SET THE HOOK";
    int msgPal = PAL_HUD;
    if (mode_ == Mode::Swim) {
        msg = "STRIKE WHEN THE FISH MEETS THE HOOK";
    } else if (mode_ == Mode::Dead) {
        msg = why_;
        msgPal = PAL_ALERT;
    } else if (mode_ == Mode::Ring) {
        msg = "THE BELL";
        msgPal = PAL_WIN;
    } else if (mode_ == Mode::Leave || (mode_ == Mode::Over && won_)) {
        msg = "BEFORE THE THIRD TRY DIED";
        msgPal = PAL_WIN;
    } else if (mode_ == Mode::Over) {
        msg = "THE THIRD TRY DIED";
        msgPal = PAL_ALERT;
    }
    hudC(26, msg, msgPal);
    if (mode_ == Mode::Dead) hudC(27, dead_ == 1 ? "TWO LEFT" : "ONE LEFT", PAL_ALERT);
    else if (mode_ == Mode::Over) hudC(27, won_ ? "RETURN PLAYS AGAIN" : "RETURN TRIES AGAIN", PAL_GOLD);
    else if (mode_ == Mode::Swim) hudC(27, "ARROWS MOVE  A STRIKES", PAL_GOLD);
}

}  // namespace fishbell

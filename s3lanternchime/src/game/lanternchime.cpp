#include "game/lanternchime.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace lanternchime {
namespace {

constexpr float kSpeed = 3.6f;
constexpr float kReach = 22.f;
constexpr int kKindle = 12;
constexpr int kDeadline = 60 * 22;

float dist(float a, float b) { return a > b ? a - b : b - a; }

}  // namespace

int Game::lamps() const {
    int n = 0;
    for (int i = 0; i < kLamps; i++)
        if (lit_[i]) n++;
    return n;
}

bool Game::allLit() const { return lamps() == kLamps; }

bool Game::inWindow() const {
    int c = playFrames_ % kCycle;
    return c < kWindow || c > kCycle - 18;
}

int Game::nearestDark() const {
    int best = -1;
    float bd = 1e9f;
    for (int i = 0; i < kLamps; i++) {
        if (lit_[i]) continue;
        float d = dist(x_, kPosts[i]);
        if (d < bd) {
            bd = d;
            best = i;
        }
    }
    return best;
}

void Game::note(int ch, float freq, float vol) { sys_->apu.tone(ch, freq, vol); }

void Game::toTitle() {
    mode_ = Mode::Title;
    reason_ = "";
    won_ = false;
    over_ = false;
    strikes_ = 0;
    playFrames_ = 0;
    timer_ = 0;
    charge_ = -1;
    chargeT_ = 0;
    tolls_ = 0;
    ropeTap_ = false;
    x_ = kPosts[0];
    faceLeft_ = false;
    hour_ = 11;
    minute_ = 59;
    second_ = 40;
    for (int i = 0; i < kLamps; i++) lit_[i] = false;
    sys_->apu.silence();
}

void Game::beginPlay() {
    mode_ = Mode::Play;
    playFrames_ = 0;
    timer_ = 0;
    strikes_ = 0;
    for (int i = 0; i < kLamps; i++) lit_[i] = false;
    x_ = kPosts[0];
    note(0, 220.f, 0.08f);
}

void Game::beginChime() {
    mode_ = Mode::Chime;
    timer_ = 0;
    tolls_ = 0;
    hour_ = 12;
    minute_ = 0;
    second_ = 0;
    reason_ = "CHIME";
    won_ = true;
    note(1, 196.f, 0.22f);
}

void Game::beginFail(const char* why) {
    mode_ = Mode::Fail;
    reason_ = why;
    timer_ = 0;
    won_ = false;
    note(0, 90.f, 0.12f);
    note(1, 0.f, 0.f);
}

void Game::finish() {
    mode_ = Mode::Over;
    over_ = true;
    note(0, 0.f, 0.f);
    note(1, 0.f, 0.f);
    note(2, 0.f, 0.f);
}

void Game::readClock() {
    if (mode_ == Mode::Chime || mode_ == Mode::Over || won_) {
        hour_ = 12;
        minute_ = 0;
        second_ = 0;
        return;
    }
    int c = playFrames_ % kCycle;
    second_ = (c * 60) / kCycle;
    hour_ = 11;
    minute_ = 59;
}

void Game::botIntent() {
    wantL_ = wantR_ = wantA_ = false;
    if (mode_ == Mode::Title) {
        if (timer_ > 10) wantA_ = true;
        return;
    }
    if (mode_ != Mode::Play) return;
    int dark = nearestDark();
    if (dark >= 0) {
        float tx = kPosts[dark];
        if (x_ < tx - 2.f) wantR_ = true;
        else if (x_ > tx + 2.f) wantL_ = true;
        else wantA_ = true;
        ropeTap_ = false;
        return;
    }
    if (x_ < kRopeX - 2.f) wantR_ = true;
    else if (x_ > kRopeX + 2.f) wantL_ = true;
    else if (inWindow() && !ropeTap_) {
        wantA_ = true;
        ropeTap_ = true;
    }
    if (!inWindow()) ropeTap_ = false;
}

void Game::tickPlay() {
    if (wantL_) {
        x_ -= kSpeed;
        faceLeft_ = true;
    }
    if (wantR_) {
        x_ += kSpeed;
        faceLeft_ = false;
    }
    if (x_ < 16.f) x_ = 16.f;
    if (x_ > 304.f) x_ = 304.f;

    bool held = wantA_;
    int near = -1;
    float nd = kReach;
    for (int i = 0; i < kLamps; i++) {
        float d = dist(x_, kPosts[i]);
        if (d < nd) {
            nd = d;
            near = i;
        }
    }
    if (held && near >= 0 && !lit_[near]) {
        if (charge_ != near) {
            charge_ = near;
            chargeT_ = 0;
        }
        chargeT_++;
        if (chargeT_ >= kKindle) {
            lit_[near] = true;
            charge_ = -1;
            chargeT_ = 0;
            note(2, 330.f + float(near) * 40.f, 0.1f);
        }
    } else {
        charge_ = -1;
        chargeT_ = 0;
    }

    bool edge = wantA_ && !prevA_;
    if (edge && dist(x_, kRopeX) < kReach) {
        if (allLit() && inWindow()) {
            beginChime();
            return;
        }
        strikes_++;
        if (!allLit()) {
            for (int i = 0; i < kLamps; i++) lit_[i] = false;
        } else {
            lit_[playFrames_ % kLamps] = false;
        }
        note(0, 70.f, 0.14f);
        if (strikes_ >= kStrikes) {
            beginFail("SPENT");
            return;
        }
    }

    if (playFrames_ > 0 && playFrames_ % 180 == 0) {
        int i = (playFrames_ / 180) % kLamps;
        if (lit_[i] && dist(x_, kPosts[i]) > 30.f) {
            lit_[i] = false;
            note(2, 140.f, 0.08f);
        }
    }

    playFrames_++;
    if (playFrames_ > kDeadline) beginFail("LATE");
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    toTitle();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (bot_) botIntent();
    else {
        const gs::Pad& p = sys.pad;
        float ax = p.axisX;
        wantL_ = p.down(gs::BTN_LEFT) || ax < -0.3f;
        wantR_ = p.down(gs::BTN_RIGHT) || ax > 0.3f;
        wantA_ = p.down(gs::BTN_A) || p.down(gs::BTN_B) || p.down(gs::BTN_C);
        if (mode_ == Mode::Title && (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A))) wantA_ = true;
    }

    if (mode_ == Mode::Title) {
        timer_++;
        if (wantA_ && !prevA_) beginPlay();
    } else if (mode_ == Mode::Play) {
        tickPlay();
        readClock();
    } else if (mode_ == Mode::Chime) {
        timer_++;
        if (timer_ % 16 == 1 && tolls_ < 4) {
            tolls_++;
            float f = tolls_ % 2 ? 196.f : 262.f;
            note(1, f, 0.2f);
        }
        if (timer_ > 16 && timer_ % 16 == 8) note(1, 0.f, 0.f);
        if (timer_ > 78) finish();
        readClock();
    } else if (mode_ == Mode::Fail) {
        timer_++;
        if (timer_ == 20) note(0, 0.f, 0.f);
        if (timer_ > 50) finish();
    }

    prevA_ = wantA_;
    draw();
}

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
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

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool shadow) {
    if (h < 1.f || m.h < 1 || m.w < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::lround(std::max(1.f, w)));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.clear();
    sky();

    int flicker = (playFrames_ / 7) & 1;
    float bob = (mode_ == Mode::Chime) ? std::sin(timer_ * 0.4f) * 3.f : 0.f;

    spr(art_.bearer, x_, 176.f, 52.f, PAL_BODY, faceLeft_);
    spr(art_.pole, x_ + (faceLeft_ ? -10.f : 10.f), 162.f, 30.f, PAL_FIRE);
    spr(art_.bell, 160.f, 78.f + bob, 26.f, PAL_IRON);
    for (int i = 0; i < kLamps; i++) {
        if (lit_[i]) spr(art_.flame[flicker], kPosts[i], 128.f, 14.f, PAL_FIRE);
        spr(art_.cage, kPosts[i], 140.f, 40.f, PAL_IRON);
        if (lit_[i]) spr(art_.glow, kPosts[i], 146.f, 46.f, PAL_GLOW);
    }
    spr(art_.rope, kRopeX, 132.f, 70.f, PAL_BODY);
    int hi = ((playFrames_ % kCycle) * kHands) / kCycle;
    if (hi < 0) hi = 0;
    if (hi >= kHands) hi = 0;
    if (won_) hi = 0;
    spr(art_.hand[hi], 160.f, 58.f, 28.f, PAL_CLOCK);
    spr(art_.hourHand, 160.f, 58.f, 22.f, PAL_CLOCK);
    spr(art_.face, 160.f, 58.f, 58.f, PAL_CLOCK);
    spr(art_.tower, 160.f, 118.f, 120.f, PAL_CLOCK);
    spr(art_.moon, 276.f, 36.f, 26.f, PAL_HUD);

    char line[48];
    if (mode_ == Mode::Title) {
        hudC(3, "LANTERN", PAL_HUD);
        hudC(5, "THE HOUR HAS TO CHIME", PAL_HUD);
        hudC(18, "KINDLE EACH LAMP", PAL_HUD);
        hudC(20, "RING THE ROPE ON TWELVE", PAL_HUD);
        if ((timer_ / 30) % 2 == 0) hudC(24, "A  START", PAL_HUD);
    } else {
        std::snprintf(line, sizeof line, "LAMPS %d/%d", lamps(), kLamps);
        hud(1, 1, line, PAL_HUD);
        std::snprintf(line, sizeof line, "%d:%02d:%02d", hour_, minute_, second_);
        hud(30, 1, line, PAL_HUD);
        std::snprintf(line, sizeof line, "STRIKE %d", strikes_);
        hud(1, 26, line, PAL_HUD);
        if (mode_ == Mode::Play && allLit() && inWindow()) hudC(24, "CHIME", PAL_HUD);
        else if (mode_ == Mode::Play && !allLit()) hudC(24, "THE LAMPS ARE DARK", PAL_HUD);
        if (mode_ == Mode::Chime) hudC(16, "THE HOUR CHIMES", PAL_HUD);
        if (mode_ == Mode::Fail) {
            hudC(15, "THE HOUR IS GONE", PAL_HUD);
            hudC(17, reason_, PAL_HUD);
        }
        if (mode_ == Mode::Over && won_) hudC(16, "THE HOUR CHIMES", PAL_HUD);
    }
}

}  // namespace lanternchime

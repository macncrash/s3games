#include "game/skate.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace skatebell {
namespace {

constexpr float kGrav = 0.20f;
constexpr float kJump = -5.35f;
constexpr float kAccel = 0.09f;
constexpr float kMaxV = 3.25f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

bool Game::audit() const {
    if (kTries != 3) return false;
    if (!groundAt(kGap0 - 12.f) || groundAt((kGap0 + kGap1) * 0.5f) || !groundAt(kGap1 + 12.f)) return false;
    if (!(kBellX > kGap1 + 40.f && kBellX + 48.f < kLeaveX && kLeaveX < kWorld)) return false;
    if (!(kBellY + 20.f < kGroundY)) return false;
    return true;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    rules_ = audit();
    if (!rules_) std::fprintf(stderr, "s3skatebell rules failed\n");
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.apu.setMaster(0.7f);
    toTitle();
}

void Game::toTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    rung_ = false;
    dead_ = 0;
    tryNo_ = 0;
    last_[0] = 0;
    bellAmp_ = 0.12f;
    bellPh_ = 0.f;
    t_ = 0;
    px_ = 72.f;
    py_ = kGroundY;
    vx_ = 0.f;
    vy_ = 0.f;
    onGround_ = true;
}

void Game::newGame() {
    dead_ = 0;
    tryNo_ = 0;
    rung_ = false;
    won_ = false;
    over_ = false;
    last_[0] = 0;
    bellAmp_ = 0.12f;
    beginTry();
}

void Game::beginTry() {
    mode_ = Mode::Ride;
    px_ = 48.f;
    py_ = kGroundY;
    vx_ = 0.f;
    vy_ = 0.f;
    onGround_ = true;
    gapJumped_ = false;
    bellJumped_ = false;
    tryNo_ = dead_ + 1;
    wait_ = 0;
}

void Game::ring() {
    if (rung_ || mode_ != Mode::Ride) return;
    rung_ = true;
    tryNo_ = dead_ + 1;
    bellAmp_ = 1.f;
    bellTick_ = 1;
    std::snprintf(last_, sizeof last_, "BELL");
    if (!sys_) return;
    sys_->rumble(0.35f, 0.75f, 140);
    sys_->setLight(255, 196, 48);
}

void Game::dieTry(const char* why) {
    if (rung_ || mode_ != Mode::Ride) return;
    dead_++;
    std::snprintf(last_, sizeof last_, "%s", why);
    if (dead_ >= kTries) {
        mode_ = Mode::Over;
        won_ = false;
        over_ = true;
        if (sys_) {
            blip(0, 90.f, 0.1f, 22);
            sys_->setLight(150, 28, 28);
        }
    } else {
        mode_ = Mode::Dead;
        wait_ = 48;
        if (sys_) {
            blip(0, 140.f, 0.08f, 12);
            sys_->apu.noiseBurst(0.16f, 400.f, 0.08f);
            sys_->setLight(120, 48, 28);
        }
    }
}

void Game::ride() {
    if (!sys_) return;
    const gs::Pad& p = sys_->pad;
    bool right = p.down(gs::BTN_RIGHT) || bot_;
    bool left = p.down(gs::BTN_LEFT) && !bot_;
    bool ollie = p.pressed(gs::BTN_A) || p.pressed(gs::BTN_B) || p.pressed(gs::BTN_C);
    if (bot_) {
        ollie = false;
        if (onGround_) {
            if (!gapJumped_ && px_ >= kGap0 - 40.f && px_ < kGap0 - 6.f) {
                ollie = true;
                gapJumped_ = true;
            } else if (!bellJumped_ && px_ >= kBellX - 88.f && px_ < kBellX - 6.f) {
                ollie = true;
                bellJumped_ = true;
            }
        }
    }

    if (onGround_) {
        if (right) vx_ = std::min(kMaxV, vx_ + kAccel);
        else if (left) vx_ = std::max(-1.6f, vx_ - kAccel);
        else vx_ *= 0.9f;
        if (std::fabs(vx_) < 0.04f) vx_ = 0.f;
        if (ollie) {
            vy_ = kJump;
            onGround_ = false;
            blip(1, 520.f, 0.06f, 4);
            sys_->apu.noiseBurst(0.08f, 900.f, 0.03f);
        }
    } else {
        if (right) vx_ = std::min(kMaxV, vx_ + kAccel * 0.35f);
        else if (left) vx_ = std::max(-1.6f, vx_ - kAccel * 0.35f);
        vy_ += kGrav;
    }

    px_ += vx_;
    py_ += vy_;
    px_ = clampf(px_, 20.f, kWorld - 10.f);

    if (py_ >= kGroundY && groundAt(px_) && vy_ >= 0.f) {
        py_ = kGroundY;
        vy_ = 0.f;
        onGround_ = true;
    } else if (!groundAt(px_) || py_ < kGroundY - 0.5f) {
        onGround_ = false;
    }

    float swing = std::sin(bellPh_) * bellAmp_ * 6.f;
    float bx = kBellX + swing;
    if (!rung_ && !onGround_) {
        float best = 1e9f;
        for (float hy = py_ - 28.f; hy <= py_; hy += 7.f)
            best = std::min(best, std::hypot(px_ - bx, hy - kBellY));
        if (best < 14.f) ring();
    }

    if (!rung_ && onGround_ && px_ > kBellX + 28.f) dieTry("MISS");
    else if (!rung_ && py_ > kGroundY + 36.f) dieTry("BAIL");
    else if (rung_ && px_ >= kLeaveX) {
        mode_ = Mode::Leave;
        won_ = true;
        wait_ = 36;
    }
}

void Game::blip(int ch, float freq, float vol, int frames) {
    if (!sys_) return;
    sys_->apu.tone(ch, freq, vol);
    if (ch == 0) blipLeft_ = frames;
}

void Game::pumpAudio() {
    if (!sys_) return;
    bool chiming = rung_ && (mode_ == Mode::Ride || mode_ == Mode::Leave || (mode_ == Mode::Over && won_));
    if (chiming) {
        if (--bellTick_ <= 0 && bellAmp_ > 0.35f) {
            float vol = 0.05f + 0.08f * bellAmp_;
            sys_->apu.tone(0, 784.f, vol);
            sys_->apu.tone(1, 1174.f, vol * 0.6f);
            blipLeft_ = 8;
            bellTick_ = 12;
        }
    }
    if (blipLeft_ > 0 && --blipLeft_ == 0) {
        sys_->apu.tone(0, 0, 0);
        if (!chiming) sys_->apu.tone(1, 0, 0);
    }
    if (mode_ == Mode::Ride && onGround_ && std::fabs(vx_) > 0.4f)
        sys_->apu.tone(2, 70.f + std::fabs(vx_) * 18.f, 0.018f);
    else if (mode_ != Mode::Ride) sys_->apu.tone(2, 0, 0);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_++;
    bellPh_ += rung_ ? 0.45f : 0.05f;
    if (rung_) bellAmp_ = std::max(0.18f, bellAmp_ - 0.01f);

    const gs::Pad& p = sys.pad;
    bool start = p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C);

    if (mode_ == Mode::Title) {
        if (bot_ && t_ > 16) newGame();
        else if (!bot_ && p.pressed(gs::BTN_MODE)) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        } else if (!bot_ && start) newGame();
    } else if (mode_ == Mode::Over) {
        if (!bot_ && start) newGame();
    } else if (mode_ == Mode::Ride) {
        ride();
    } else if (mode_ == Mode::Dead) {
        if (--wait_ <= 0) beginTry();
    } else if (mode_ == Mode::Leave) {
        px_ = std::min(kWorld - 12.f, px_ + 2.4f);
        if (--wait_ <= 0) {
            mode_ = Mode::Over;
            won_ = true;
            over_ = true;
        }
    }

    pumpAudio();
    draw();
}

void Game::stamp(const gs::Image& img, float x, float y, int pal, bool shadow) {
    if (!sys_ || img.w == 0) return;
    gs::Sprite s;
    s.x = int16_t(std::lround(x));
    s.y = int16_t(std::lround(y));
    s.w = int16_t(img.w);
    s.h = int16_t(img.h);
    s.img = img;
    s.pal = uint8_t(pal);
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
        if (y < 150) {
            float u = float(y) / 150.f;
            v.lineBackdrop[y] = gs::rgb4(4 + int(u * 5.f), 7 + int(u * 4.f), 13 - int(u * 2.f));
        } else {
            v.lineBackdrop[y] = (y & 4) ? gs::rgb4(3, 3, 4) : gs::rgb4(4, 4, 5);
        }
    }
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    float cam = clampf(px_ - 96.f, 0.f, kWorld - float(gs::SCREEN_W));
    auto sx = [&](float wx) { return wx - cam; };

    if (mode_ == Mode::Title) {
        stamp(art_.word, 78.f, 22.f, PAL_WORD);
        hudC(8, "SKATE THE BELL", PAL_INK);
        hudC(20, "RIGHT SKATE   A OLLIE", PAL_GOLD);
        hudC(22, "THREE TRIES   THEN LEAVE", PAL_INK);
    }

    float swing = std::sin(bellPh_) * bellAmp_ * 6.f;
    float feet = py_;
    float rx = sx(px_) - 11.f;
    float ry = feet - 32.f - (onGround_ ? 6.f : 8.f);
    stamp(art_.rider, rx, ry, PAL_RIDER);
    stamp(art_.deck, sx(px_) - 15.f, feet - 6.f, PAL_DECK);

    stamp(art_.pole, sx(kBellX) - 3.f, kBellY - 70.f, PAL_TOWN);
    stamp(art_.bell, sx(kBellX + swing) - 10.f, kBellY - 12.f, PAL_BELL);

    const float cones[] = {150.f, 260.f, 560.f, 700.f, 960.f};
    for (float cx : cones) stamp(art_.cone, sx(cx) - 6.f, kGroundY - 16.f, PAL_TOWN);

    for (float bx = 0.f; bx < kWorld; bx += 32.f) {
        if (!groundAt(bx + 8.f)) continue;
        float x = sx(bx);
        if (x < -32.f || x > gs::SCREEN_W) continue;
        stamp(art_.brick, x, kGroundY, PAL_STREET);
    }

    const float blocks[] = {40.f, 180.f, 620.f, 900.f, 1100.f};
    const float bh[] = {0.f, 16.f, 8.f, 24.f, 4.f};
    for (int i = 0; i < 5; i++) {
        float x = sx(blocks[i]);
        if (x < -40.f || x > gs::SCREEN_W) continue;
        stamp(art_.block, x, kGroundY - 48.f - bh[i], PAL_TOWN);
    }

    if (mode_ != Mode::Title) {
        int shown = tryNo_;
        if (shown < 1) shown = 1;
        if (shown > 3) shown = 3;
        char line[40];
        std::snprintf(line, sizeof line, "TRY %d/3", shown);
        hud(1, 1, line, PAL_INK);
        if (last_[0]) hud(30, 1, last_, rung_ ? PAL_GOLD : PAL_ALERT);
    }
    if (rung_ && mode_ != Mode::Title) hudC(24, "THE BELL RINGS", PAL_GOLD);
    if (mode_ == Mode::Leave || (mode_ == Mode::Over && won_)) hudC(25, "LEAVE", PAL_GOLD);
    if (mode_ == Mode::Over && !won_) hudC(24, "THIRD TRY DIED", PAL_ALERT);
}

}  // namespace skatebell

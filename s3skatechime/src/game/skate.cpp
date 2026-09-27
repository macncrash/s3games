#include "game/skate.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace skatechime {
namespace {

constexpr float kGrav = 0.22f;
constexpr float kJump = -4.8f;
constexpr float kAccel = 0.16f;
constexpr float kMaxV = 2.6f;
constexpr float kJumpAt = 500.f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

bool Game::audit() const {
    if (kTries != 3 || kGraceSec < 2 || kFpc < 1 || kLeadSec < 1) return false;
    if (!(kGold0 + 40.f < kGold1 && kGold1 + 80.f < kLeaveX && kLeaveX < kWorld)) return false;
    if (!(kTowerX > kGold0 && kTowerX < kGold1)) return false;
    if (onGold(kGold0 - 2.f) || !onGold((kGold0 + kGold1) * 0.5f) || onGold(kGold1)) return false;
    int before = kHourSec - kLeadSec;
    int at = kHourSec - kLeadSec + (kLeadSec * kFpc) / kFpc;
    int late = kHourSec - kLeadSec + ((kLeadSec + kGraceSec) * kFpc) / kFpc;
    if (before >= kHourSec || at != kHourSec || late != kHourSec + kGraceSec) return false;
    return true;
}

int Game::clockSec() const { return kHourSec - kLeadSec + playFrames_ / kFpc; }

bool Game::onHour() const {
    int sec = clockSec();
    return sec >= kHourSec && sec < kHourSec + kGraceSec;
}

bool Game::pastHour() const { return clockSec() >= kHourSec + kGraceSec; }

void Game::face(int& h, int& m, int& s) const {
    int t = clockSec();
    if (t < 0) t = 0;
    s = t % 60;
    m = (t / 60) % 60;
    h = (t / 3600) % 12;
    if (h == 0) h = 12;
}

int Game::hour() const {
    int h, m, s;
    face(h, m, s);
    return h;
}

int Game::minute() const {
    int h, m, s;
    face(h, m, s);
    return m;
}

int Game::second() const {
    int h, m, s;
    face(h, m, s);
    return s;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    rules_ = audit();
    if (!rules_) std::fprintf(stderr, "s3skatechime rules failed\n");
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
    chimed_ = false;
    reason_ = "";
    dead_ = 0;
    tryNo_ = 0;
    playFrames_ = 0;
    t_ = 0;
    px_ = 72.f;
    py_ = kGroundY;
    vx_ = 0.f;
    vy_ = 0.f;
    onGround_ = true;
    jumped_ = false;
    bellAmp_ = 0.1f;
    bellPh_ = 0.f;
    strikes_ = 0;
}

void Game::newGame() {
    dead_ = 0;
    chimed_ = false;
    won_ = false;
    over_ = false;
    reason_ = "";
    playFrames_ = 0;
    bellAmp_ = 0.1f;
    strikes_ = 0;
    beginTry();
}

void Game::beginTry() {
    mode_ = Mode::Ride;
    px_ = 48.f;
    py_ = kGroundY;
    vx_ = 0.f;
    vy_ = 0.f;
    onGround_ = true;
    jumped_ = false;
    tryNo_ = dead_ + 1;
    wait_ = 0;
}

void Game::beginChime() {
    chimed_ = true;
    won_ = false;
    reason_ = "CHIME";
    bellAmp_ = 1.f;
    bellTick_ = 1;
    strikes_ = 0;
    mode_ = Mode::Chime;
    if (!sys_) return;
    sys_->rumble(0.3f, 0.7f, 120);
    sys_->setLight(255, 210, 64);
    blip(0, 523.f, 0.1f, 10);
    sys_->apu.tone(1, 784.f, 0.07f);
}

void Game::beginEarly() {
    reason_ = "EARLY";
    dead_++;
    jumped_ = false;
    if (dead_ >= kTries || pastHour()) {
        beginFail("EARLY");
        return;
    }
    mode_ = Mode::Early;
    wait_ = 28;
    if (sys_) {
        blip(0, 180.f, 0.06f, 10);
        sys_->setLight(180, 120, 40);
    }
}

void Game::beginFail(const char* why) {
    if (!chimed_) reason_ = why;
    won_ = false;
    mode_ = Mode::Fail;
    wait_ = 36;
    if (sys_) {
        blip(0, 90.f, 0.09f, 18);
        sys_->setLight(150, 28, 28);
    }
}

void Game::blip(int ch, float freq, float vol, int frames) {
    if (!sys_) return;
    sys_->apu.tone(ch, freq, vol);
    if (ch == 0) blipLeft_ = frames;
}

void Game::pumpAudio() {
    if (!sys_) return;
    bool ringing = chimed_ && (mode_ == Mode::Chime || mode_ == Mode::Leave || (mode_ == Mode::Over && won_));
    if (ringing && strikes_ < 8) {
        if (--bellTick_ <= 0) {
            float vol = 0.05f + 0.07f * bellAmp_;
            sys_->apu.tone(0, 523.f, vol);
            sys_->apu.tone(1, 784.f, vol * 0.55f);
            blipLeft_ = 8;
            bellTick_ = 14;
            strikes_++;
        }
    }
    if (blipLeft_ > 0 && --blipLeft_ == 0) {
        sys_->apu.tone(0, 0, 0);
        if (!ringing) sys_->apu.tone(1, 0, 0);
    }
    if (mode_ == Mode::Ride && onGround_ && std::fabs(vx_) > 0.4f)
        sys_->apu.tone(2, 68.f + std::fabs(vx_) * 16.f, 0.016f);
    else if (mode_ != Mode::Ride) sys_->apu.tone(2, 0, 0);
}

void Game::ride() {
    if (!sys_) return;
    const gs::Pad& p = sys_->pad;
    bool right = p.down(gs::BTN_RIGHT) || bot_;
    bool left = p.down(gs::BTN_LEFT) && !bot_;
    bool ollie = p.pressed(gs::BTN_A) || p.pressed(gs::BTN_B) || p.pressed(gs::BTN_C);
    if (bot_) {
        ollie = false;
        if (onGround_ && !jumped_ && px_ >= kJumpAt && px_ < kGold0) ollie = true;
    }

    if (onGround_) {
        if (right) vx_ = std::min(kMaxV, vx_ + kAccel);
        else if (left) vx_ = std::max(-1.5f, vx_ - kAccel);
        else vx_ *= 0.88f;
        if (std::fabs(vx_) < 0.04f) vx_ = 0.f;
        if (ollie) {
            vy_ = kJump;
            onGround_ = false;
            jumped_ = true;
            blip(1, 480.f, 0.05f, 4);
            sys_->apu.noiseBurst(0.07f, 800.f, 0.03f);
        }
    } else {
        if (right) vx_ = std::min(kMaxV, vx_ + kAccel * 0.35f);
        else if (left) vx_ = std::max(-1.5f, vx_ - kAccel * 0.35f);
        vy_ += kGrav;
    }

    px_ += vx_;
    py_ += vy_;
    px_ = clampf(px_, 20.f, kWorld - 12.f);

    if (py_ >= kGroundY && vy_ >= 0.f) {
        py_ = kGroundY;
        vy_ = 0.f;
        onGround_ = true;
    } else if (py_ < kGroundY - 0.5f) {
        onGround_ = false;
    }

    if (!chimed_ && !onGround_ && onGold(px_)) {
        if (onHour()) beginChime();
        else if (!pastHour()) beginEarly();
        else beginFail("LATE");
        return;
    }
    if (!chimed_ && onGround_ && px_ >= kGold1) beginFail("ROLLED");
    else if (!chimed_ && pastHour()) beginFail("LATE");
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_++;
    bellPh_ += chimed_ ? 0.42f : 0.04f;
    if (chimed_) bellAmp_ = std::max(0.2f, bellAmp_ - 0.008f);

    const gs::Pad& p = sys.pad;
    bool start = p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C);

    if (mode_ == Mode::Title) {
        if (bot_ && t_ > 18) newGame();
        else if (!bot_ && p.pressed(gs::BTN_MODE)) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        } else if (!bot_ && start) newGame();
    } else if (mode_ == Mode::Ride) {
        playFrames_++;
        ride();
    } else if (mode_ == Mode::Early) {
        if (--wait_ <= 0) beginTry();
    } else if (mode_ == Mode::Chime) {
        px_ = std::min(kWorld - 12.f, px_ + 2.4f);
        py_ = kGroundY;
        onGround_ = true;
        if (px_ >= kLeaveX) {
            mode_ = Mode::Leave;
            wait_ = 30;
        }
    } else if (mode_ == Mode::Leave) {
        px_ = std::min(kWorld - 12.f, px_ + 2.2f);
        if (--wait_ <= 0) {
            mode_ = Mode::Over;
            won_ = true;
            over_ = true;
        }
    } else if (mode_ == Mode::Fail) {
        if (--wait_ <= 0) {
            mode_ = Mode::Over;
            won_ = false;
            over_ = true;
        }
    } else if (mode_ == Mode::Over) {
        if (!bot_ && start) newGame();
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
        if (y < 148) {
            float u = float(y) / 148.f;
            v.lineBackdrop[y] = gs::rgb4(3 + int(u * 4.f), 6 + int(u * 3.f), 12 - int(u * 2.f));
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

    float cam = clampf(px_ - 110.f, 0.f, kWorld - float(gs::SCREEN_W));
    auto sx = [&](float wx) { return wx - cam; };

    if (mode_ == Mode::Title) {
        stamp(art_.word, 78.f, 18.f, PAL_WORD);
        hudC(8, "THE HOUR HAS TO CHIME", PAL_INK);
        hudC(20, "RIGHT SKATE   A OLLIE", PAL_HOUR);
        hudC(22, "GOLD ON TWELVE", PAL_INK);
    }

    float swing = std::sin(bellPh_) * bellAmp_ * 8.f;
    float feet = py_;
    stamp(art_.rider, sx(px_) - 11.f, feet - 30.f - (onGround_ ? 6.f : 10.f), PAL_RIDER);
    stamp(art_.deck, sx(px_) - 16.f, feet - 6.f, PAL_DECK);

    stamp(art_.tower, sx(kTowerX) - 18.f, kGroundY - 96.f, PAL_TOWER);
    stamp(art_.face, sx(kTowerX) - 7.f, kGroundY - 64.f, PAL_GOLD);
    stamp(art_.bell, sx(kTowerX) + swing - 7.f, kGroundY - 108.f, PAL_GOLD);

    const float cones[] = {180.f, 320.f, 460.f, 760.f, 980.f};
    for (float cx : cones) stamp(art_.cone, sx(cx) - 6.f, kGroundY - 16.f, PAL_TOWER);

    for (float bx = 0.f; bx < kWorld; bx += 32.f) {
        float x = sx(bx);
        if (x < -32.f || x > gs::SCREEN_W) continue;
        bool gold = onGold(bx + 8.f);
        stamp(gold ? art_.gold : art_.brick, x, kGroundY, gold ? PAL_GOLD : PAL_STREET);
    }

    if (mode_ != Mode::Title) {
        int h, m, s;
        face(h, m, s);
        char clock[24];
        std::snprintf(clock, sizeof clock, "%d:%02d:%02d", h, m, s);
        hud(1, 1, clock, onHour() ? PAL_HOUR : PAL_INK);
        int shown = tryNo_;
        if (shown < 1) shown = 1;
        if (shown > kTries) shown = kTries;
        char tr[16];
        std::snprintf(tr, sizeof tr, "TRY %d/%d", shown, kTries);
        hud(30, 1, tr, PAL_INK);
    }
    if (chimed_ && mode_ != Mode::Title) hudC(24, "THE HOUR CHIMES", PAL_HOUR);
    if (mode_ == Mode::Early) hudC(25, "EARLY", PAL_ALERT);
    if (mode_ == Mode::Leave || (mode_ == Mode::Over && won_)) hudC(25, "LEAVE", PAL_HOUR);
    if (mode_ == Mode::Fail || (mode_ == Mode::Over && !won_)) hudC(24, reason_[0] ? reason_ : "LATE", PAL_ALERT);
}

}  // namespace skatechime

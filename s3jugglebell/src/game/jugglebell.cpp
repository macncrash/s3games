#include "game/jugglebell.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace jugglebell {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kG = 520.f;
constexpr float kHandX = 214.f;
constexpr float kHandY = 158.f;
constexpr float kCatchX = 114.f;
constexpr float kCatchY = 158.f;
constexpr float kBellX = 160.f;
constexpr float kBellY = 54.f;
constexpr float kBellR = 13.f;
constexpr float kVx0 = -78.f;
constexpr float kSweetLo = 0.53f;
constexpr float kSweetHi = 0.61f;
constexpr float kMeterRate = 0.72f;
constexpr float kCatchR = 20.f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

float vyFor(float meter) { return -180.f - meter * 260.f; }

float vxFor(float aim) { return kVx0 + aim * 36.f; }

enum class End { Fly, Bell, Floor, Out };

struct Ball {
    float x, y, vx, vy;
};

End stepBall(Ball& b) {
    b.x += b.vx * kDt;
    b.y += b.vy * kDt;
    b.vy += kG * kDt;
    float dx = b.x - kBellX;
    float dy = b.y - kBellY;
    if (dx * dx + dy * dy <= kBellR * kBellR) return End::Bell;
    if (b.y > 208.f) return End::Floor;
    if (b.x < 4.f || b.x > 316.f || b.y < 2.f) return End::Out;
    return End::Fly;
}

}  // namespace

bool Game::audit() const {
    Ball sweet{kHandX, kHandY, vxFor(0.f), vyFor(0.57f)};
    End e = End::Fly;
    for (int i = 0; i < 180 && e == End::Fly; i++) e = stepBall(sweet);
    if (e != End::Bell) {
        std::fprintf(stderr, "s3jugglebell sweet loft missed the bell\n");
        return false;
    }
    Ball weak{kHandX, kHandY, vxFor(0.f), vyFor(0.12f)};
    e = End::Fly;
    for (int i = 0; i < 180 && e == End::Fly; i++) e = stepBall(weak);
    if (e != End::Floor) {
        std::fprintf(stderr, "s3jugglebell a short loft did not die\n");
        return false;
    }
    Ball hot{kHandX, kHandY, vxFor(0.f), vyFor(0.96f)};
    e = End::Fly;
    for (int i = 0; i < 180 && e == End::Fly; i++) e = stepBall(hot);
    if (e != End::Out) {
        std::fprintf(stderr, "s3jugglebell a hot loft stayed in\n");
        return false;
    }
    return true;
}

bool Game::sweet() const { return meter_ >= kSweetLo && meter_ <= kSweetHi; }

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    rules_ = audit();
    if (!rules_) std::fprintf(stderr, "s3jugglebell rules failed\n");
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.setFogColor(gs::rgb4(2, 1, 3));
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
    why_ = Death::None;
    wasSweet_ = false;
    bellAmp_ = 0.2f;
    bellPh_ = 0;
    clock_ = 0;
    aim_ = 0;
    meter_ = 0;
    meterDir_ = 1.f;
    ballX_ = kHandX;
    ballY_ = kHandY;
}

void Game::newGame() {
    dead_ = 0;
    tryNo_ = 0;
    won_ = false;
    over_ = false;
    rung_ = false;
    why_ = Death::None;
    bellAmp_ = 0.2f;
    aim_ = 0;
    beginAim();
}

void Game::beginAim() {
    meter_ = 0;
    meterDir_ = 1.f;
    wasSweet_ = false;
    deadT_ = 0;
    flightT_ = 0;
    ballX_ = kHandX;
    ballY_ = kHandY;
    ballVx_ = 0;
    ballVy_ = 0;
    mode_ = Mode::Aim;
}

void Game::launch() {
    if (mode_ != Mode::Aim || !rules_) return;
    tryNo_++;
    ballX_ = kHandX;
    ballY_ = kHandY;
    ballVx_ = vxFor(aim_);
    ballVy_ = vyFor(meter_);
    flightT_ = 0;
    mode_ = Mode::Flight;
    blip(2, 520.f, 0.05f, 0.05f);
    if (sys_) sys_->apu.noiseBurst(0.08f, 1800.f, 0.04f);
}

void Game::ring() {
    if (rung_ || mode_ != Mode::Flight) return;
    rung_ = true;
    won_ = dead_ < 3;
    bellAmp_ = 1.f;
    bellTick_ = 0.02f;
    ringT_ = 0;
    mode_ = Mode::Ring;
    if (!sys_) return;
    sys_->apu.noise(0, 1000);
    sys_->rumble(0.35f, 0.8f, 160);
    sys_->setLight(255, 196, 64);
}

void Game::dieTry(Death why) {
    if (mode_ != Mode::Flight) return;
    why_ = why;
    dead_++;
    deadT_ = 0;
    mode_ = Mode::Dead;
    blip(0, why == Death::Hot ? 180.f : 140.f, 0.06f, 0.18f);
    if (sys_) sys_->rumble(0.5f, 0.2f, 90);
}

void Game::caught() {
    if (mode_ != Mode::Flight) return;
    blip(2, 660.f, 0.04f, 0.05f);
    beginAim();
}

void Game::botAim() {
    aim_ = 0;
    if (sweet()) launch();
}

void Game::humanAim(const gs::Pad& p, bool fire) {
    float n = 0;
    if (p.down(gs::BTN_LEFT)) n -= 1.f;
    if (p.down(gs::BTN_RIGHT)) n += 1.f;
    if (std::fabs(p.axisX) > 0.25f) n = p.axisX;
    aim_ = clampf(aim_ + n * 1.4f * kDt, -1.f, 1.f);
    bool sw = sweet();
    if (sw && !wasSweet_) blip(2, 880.f, 0.035f, 0.04f);
    wasSweet_ = sw;
    if (fire) launch();
}

void Game::blip(int ch, float freq, float vol, float hold) {
    if (!sys_) return;
    sys_->apu.tone(ch, freq, vol);
    if (ch == 0) toneT_ = hold;
    else tickT_ = hold;
}

void Game::tickAudio(float dt) {
    if (!sys_) return;
    bool chiming = rung_ && (mode_ == Mode::Ring || mode_ == Mode::Leave || (mode_ == Mode::Over && won_));
    if (chiming) {
        bellTick_ -= dt;
        if (bellTick_ <= 0.f && bellAmp_ > 0.45f) {
            float vol = 0.05f + 0.08f * bellAmp_;
            sys_->apu.tone(0, 698.f, vol);
            sys_->apu.tone(1, 1046.f, vol * 0.65f);
            toneT_ = 0.14f;
            bellTick_ = 0.28f;
        }
    }
    if (toneT_ > 0.f) {
        toneT_ -= dt;
        if (toneT_ <= 0.f) {
            sys_->apu.tone(0, 0, 0);
            sys_->apu.tone(1, 0, 0);
        }
    }
    if (tickT_ > 0.f) {
        tickT_ -= dt;
        if (tickT_ <= 0.f) sys_->apu.tone(2, 0, 0);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    clock_ += kDt;
    bellPh_ += kDt * (rung_ ? 14.f : 2.1f);
    if (rung_) bellAmp_ = std::max(0.16f, bellAmp_ - kDt * 0.4f);

    const gs::Pad& pad = sys.pad;
    bool fire = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_B);
    bool start = pad.pressed(gs::BTN_START);

    if (mode_ == Mode::Title || mode_ == Mode::Aim) {
        meter_ += meterDir_ * kMeterRate * kDt;
        if (meter_ >= 1.f) {
            meter_ = 1.f;
            meterDir_ = -1.f;
        } else if (meter_ <= 0.f) {
            meter_ = 0.f;
            meterDir_ = 1.f;
        }
    }

    if (mode_ == Mode::Title) {
        if ((start || fire) && rules_) newGame();
        else if (bot_ && clock_ > 0.35f && rules_) newGame();
    } else if (mode_ == Mode::Aim) {
        if (start) {
            held_ = mode_;
            mode_ = Mode::Pause;
        } else if (bot_) botAim();
        else humanAim(pad, fire);
    } else if (mode_ == Mode::Flight) {
        flightT_++;
        Ball b{ballX_, ballY_, ballVx_, ballVy_};
        End e = stepBall(b);
        ballX_ = b.x;
        ballY_ = b.y;
        ballVx_ = b.vx;
        ballVy_ = b.vy;
        if (e == End::Bell) ring();
        else if (e == End::Floor) dieTry(Death::Drop);
        else if (e == End::Out) dieTry(Death::Hot);
        else if (!bot_) {
            float dx = ballX_ - kCatchX;
            float dy = ballY_ - kCatchY;
            bool near = dx * dx + dy * dy <= kCatchR * kCatchR && ballVy_ > 40.f;
            if (near && (pad.down(gs::BTN_LEFT) || pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_A))) caught();
        }
    } else if (mode_ == Mode::Dead) {
        deadT_ += kDt;
        ballY_ = std::min(ballY_ + 90.f * kDt, 214.f);
        if (deadT_ > 0.85f) {
            if (dead_ >= 3) {
                won_ = false;
                leaveT_ = 0;
                mode_ = Mode::Leave;
            } else beginAim();
        }
    } else if (mode_ == Mode::Ring) {
        ringT_ += kDt;
        if (ringT_ > 1.15f) {
            leaveT_ = 0;
            mode_ = Mode::Leave;
        }
    } else if (mode_ == Mode::Leave) {
        leaveT_ += kDt;
        if (leaveT_ > 1.15f) {
            over_ = true;
            mode_ = Mode::Over;
        }
    } else if (mode_ == Mode::Pause) {
        if (start || fire) mode_ = held_;
    } else if (mode_ == Mode::Over) {
        if (!bot_ && start) toTitle();
    }

    tickAudio(kDt);
    draw();
}

void Game::spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool shadow) {
    if (!sys_ || img.w == 0) return;
    gs::Sprite s;
    s.img = img;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - w * 0.5f));
    s.y = int16_t(std::lround(cy - h * 0.5f));
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
    int n = s ? int(std::strlen(s)) : 0;
    hud(20 - n / 2, row, s, pal);
}

void Game::pattern(int i, float& x, float& y) const {
    constexpr float period = 0.95f;
    float t = clock_ + float(i) * period * 0.5f;
    float u = std::fmod(t, period) / period;
    bool rtl = (int(t / period) & 1) == 0;
    float x0 = rtl ? 206.f : 114.f;
    float x1 = rtl ? 114.f : 206.f;
    x = x0 + (x1 - x0) * u;
    y = 156.f - std::sin(u * kPi) * 58.f;
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        uint16_t c;
        if (y < 168) {
            int stripe = ((y / 8) & 1);
            int lift = y / 40;
            c = stripe ? gs::rgb4(6, 1, 4) : gs::rgb4(4 + (lift & 1), 1, 3);
        } else if (y < 188) {
            c = gs::rgb4(5, 2, 2);
        } else {
            int band = (y / 4) & 1;
            c = gs::rgb4(3 + band, 2, 1);
        }
        v.lineBackdrop[y] = c;
    }
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    float swing = std::sin(bellPh_) * 8.f * (rung_ ? bellAmp_ : 0.12f);
    float bx = kBellX + swing;
    float by = kBellY;

    spr(art_.post, 28.f, 120.f, float(art_.post.w), float(art_.post.h), PAL_WOOD);
    spr(art_.post, 292.f, 120.f, float(art_.post.w), float(art_.post.h), PAL_WOOD);
    spr(art_.juggler, 160.f, 156.f, float(art_.juggler.w), float(art_.juggler.h), PAL_BODY);
    spr(art_.rope, kBellX, 22.f, float(art_.rope.w), float(art_.rope.h), PAL_WOOD);

    float bh = float(art_.bell.h);
    float bw = bh * float(art_.bell.w) / float(std::max(1, int(art_.bell.h)));
    spr(art_.bell.pick(bh), bx, by, bw, bh, PAL_BELL);
    spr(art_.clapper, bx + swing * 0.35f, by + 6.f, float(art_.clapper.w), float(art_.clapper.h), PAL_BELL);

    if (rung_ && bellAmp_ > 0.35f) {
        for (int i = 0; i < 3; i++) {
            float a = bellPh_ * 1.4f + float(i) * 2.094f;
            float rad = 18.f + (1.f - bellAmp_) * 16.f;
            spr(art_.pip, bx + std::cos(a) * rad, by + std::sin(a) * rad * 0.45f, 5.f, 8.f, PAL_GOLD);
        }
    }

    int pals[2] = {PAL_GOLD_BALL, PAL_BLUE};
    for (int i = 0; i < 2; i++) {
        float x, y;
        pattern(i, x, y);
        spr(art_.ball, x + 2.f, y + 6.f, 10.f, 5.f, PAL_INK, true);
        spr(art_.ball, x, y, 14.f, 14.f, pals[i]);
    }

    float lx = ballX_, ly = ballY_;
    if (mode_ == Mode::Aim || mode_ == Mode::Title || mode_ == Mode::Pause) {
        lx = kHandX;
        ly = kHandY + std::sin(clock_ * 6.f) * 1.5f;
    }
    spr(art_.ball, lx + 2.f, ly + 7.f, 12.f, 5.f, PAL_INK, true);
    float bs = (mode_ == Mode::Flight) ? 15.f : 16.f;
    spr(art_.ball, lx, ly, bs, bs, PAL_BALL);
    spr(art_.hand, kHandX, kHandY + 6.f, float(art_.hand.w), float(art_.hand.h), PAL_CLOTH);
    spr(art_.hand, kCatchX, kCatchY + 6.f, float(art_.hand.w), float(art_.hand.h), PAL_CLOTH);

    if (mode_ == Mode::Aim || mode_ == Mode::Title) {
        int n = 16;
        float x0 = 160.f - float(n) * 4.f;
        for (int i = 0; i < n; i++) {
            float u = (float(i) + 0.5f) / float(n);
            bool on = u <= meter_;
            bool in = u >= kSweetLo && u <= kSweetHi;
            int pal = in ? (on ? PAL_GREEN : PAL_GOLD) : (on ? PAL_ALERT : PAL_INK);
            spr(art_.meter, x0 + float(i) * 8.f, 196.f, 6.f, 8.f, pal);
        }
    }

    for (int i = 0; i < 3; i++) {
        bool gone = i < dead_;
        spr(art_.pip, 16.f, 28.f + float(i) * 12.f, 5.f, 8.f, gone ? PAL_ALERT : PAL_GREEN);
    }

    if (mode_ == Mode::Title) {
        hudC(3, "S3 JUGGLEBELL", PAL_TITLE);
        hudC(6, "A SHORT JUGGLE", PAL_GOLD);
        hudC(9, "LOFT THE RED BALL", PAL_INK);
        hudC(10, "INTO THE BELL", PAL_INK);
        hudC(13, "THREE TRIES", PAL_ALERT);
        hudC(22, "A THROWS", PAL_GOLD);
        hudC(24, "LEFT CATCHES A SHORT ONE", PAL_INK);
    } else if (mode_ == Mode::Aim) {
        hudC(3, sweet() ? "LOFT" : "WAIT", sweet() ? PAL_GREEN : PAL_GOLD);
        hudC(22, "LEFT RIGHT AIM", PAL_INK);
        char buf[24];
        std::snprintf(buf, sizeof buf, "TRY %d", std::min(tryNo_ + 1, 3));
        hud(1, 1, buf, dead_ == 2 ? PAL_ALERT : PAL_GOLD);
    } else if (mode_ == Mode::Flight) {
        hudC(3, "UP", PAL_INK);
    } else if (mode_ == Mode::Dead) {
        hudC(3, why_ == Death::Hot ? "SAILED OFF" : "DROPPED", PAL_ALERT);
    } else if (mode_ == Mode::Ring || (mode_ == Mode::Leave && won_) || (mode_ == Mode::Over && won_)) {
        hudC(3, "BELL", PAL_GOLD);
        hudC(5, "LEAVE", PAL_GREEN);
    } else if (mode_ == Mode::Leave || mode_ == Mode::Over) {
        hudC(3, "THIRD TRY DIED", PAL_ALERT);
        hudC(5, "BELL SILENT", PAL_INK);
    } else if (mode_ == Mode::Pause) {
        hudC(3, "PAUSED", PAL_GOLD);
    }
    hud(30, 1, "S3 JUGGLEBELL", PAL_GOLD);
}

}  // namespace jugglebell

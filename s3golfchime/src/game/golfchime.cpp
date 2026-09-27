#include "game/golfchime.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace golfchime {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr int kHourSec = 12 * 3600;
constexpr int kStartSec = kHourSec - 90;
constexpr float kBreak = 34.f;
constexpr float kDrag = 46.f;
constexpr float kHoleSpd = 86.f;

}  // namespace

int Game::clockSec() const { return kStartSec + playFrames_ / kFpc; }

int Game::framesUntilHour() const {
    int sec = clockSec();
    int sub = playFrames_ % kFpc;
    if (sec > kHourSec) return -((sec - kHourSec) * kFpc + sub);
    if (sec == kHourSec) return -sub;
    return (kHourSec - sec) * kFpc - sub;
}

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

bool Game::stepBall(Ball& b) const {
    if (!b.live || b.holed) return b.holed;
    b.vx += kBreak * kDt;
    b.x += b.vx * kDt;
    b.y += b.vy * kDt;
    float sp = std::hypot(b.vx, b.vy);
    float ns = std::max(0.f, sp - kDrag * kDt);
    if (sp > 0.0001f) {
        b.vx *= ns / sp;
        b.vy *= ns / sp;
    }
    sp = ns;
    if (b.x < 24.f || b.x > 304.f || b.y < 44.f || b.y > 206.f) {
        b.live = false;
        b.vx = b.vy = 0;
        return false;
    }
    float dx = b.x - kCupX;
    float dy = b.y - kCupY;
    if (dx * dx + dy * dy <= kCupR * kCupR) {
        if (sp <= kHoleSpd) {
            b.holed = true;
            b.live = false;
            b.x = kCupX;
            b.y = kCupY;
            b.vx = b.vy = 0;
            return true;
        }
        b.vx *= 0.8f;
        b.vy *= 0.8f;
    }
    if (sp < 7.f) {
        b.live = false;
        b.vx = b.vy = 0;
    }
    return false;
}

int Game::rollUntil(float ang, float spd, Ball& out) const {
    Ball b;
    b.x = kTeeX;
    b.y = kTeeY;
    float a = ang * (kPi / 180.f);
    b.vx = std::cos(a) * spd;
    b.vy = -std::sin(a) * spd;
    b.live = true;
    for (int i = 1; i <= 60 * 8; i++) {
        if (stepBall(b)) {
            out = b;
            return i;
        }
        if (!b.live) {
            out = b;
            return 0;
        }
    }
    out = b;
    return 0;
}

void Game::solve() {
    float bestScore = 1e9f;
    Ball junk;
    for (int ai = 8; ai <= 78; ai += 2) {
        for (int si = 90; si <= 280; si += 8) {
            Ball out;
            int n = rollUntil(float(ai), float(si), out);
            if (n < 48 || n > 150 || !out.holed) continue;
            float score = std::fabs(float(n) - 96.f) + std::fabs(float(ai) - 36.f) * 0.02f;
            if (score < bestScore) {
                bestScore = score;
                solAng_ = float(ai);
                solSpd_ = float(si);
                solTravel_ = n;
                solved_ = true;
            }
        }
    }
    if (!solved_) reason_ = "RULES";
    (void)junk;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.HUD.enabled = true;
    sys.vdp.setFogColor(gs::rgb4(6, 8, 10));
    sys.apu.setMaster(0.4f);
    solve();
    showTitle();
}

void Game::showTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    used_ = 0;
    playFrames_ = 0;
    hold_ = 0;
    charging_ = false;
    reason_ = solved_ ? "" : "RULES";
    ball_ = {};
    ball_.x = kTeeX;
    ball_.y = kTeeY;
    aim_ = solved_ ? solAng_ : 32.f;
    meter_ = 0.4f;
    meterDir_ = 1.f;
}

void Game::newGame() { showTitle(); }

void Game::launch(float ang, float spd) {
    if (used_ >= kBalls || pastHour()) {
        beginFail(pastHour() ? "HOUR" : "BALLS");
        return;
    }
    float a = ang * (kPi / 180.f);
    ball_.x = kTeeX;
    ball_.y = kTeeY;
    ball_.vx = std::cos(a) * spd;
    ball_.vy = -std::sin(a) * spd;
    ball_.live = true;
    ball_.holed = false;
    used_++;
    charging_ = false;
    mode_ = Mode::Roll;
    if (sys_) sys_->apu.noiseBurst(0.1f, 700.f, 0.04f);
}

void Game::beginLift(const char* why) {
    reason_ = why;
    ball_.live = false;
    hold_ = 0;
    if (used_ >= kBalls || pastHour()) {
        beginFail(pastHour() ? "HOUR" : why);
        return;
    }
    mode_ = Mode::Lift;
    if (sys_) sys_->apu.tone(1, 180.f, 0.05f);
}

void Game::beginChime() {
    won_ = true;
    reason_ = "CHIME";
    hold_ = 0;
    mode_ = Mode::Chime;
    if (!sys_) return;
    sys_->apu.tone(0, 523.f, 0.1f);
    sys_->apu.tone(1, 784.f, 0.06f);
}

void Game::beginFail(const char* why) {
    if (reason_[0] == 0 || std::strcmp(why, "HOUR") == 0 || std::strcmp(why, "RULES") == 0) reason_ = why;
    else if (!won_) reason_ = why;
    won_ = false;
    hold_ = 0;
    mode_ = Mode::Fail;
    if (sys_) sys_->apu.tone(0, 90.f, 0.06f);
}

void Game::settle() {
    if (ball_.holed) {
        if (onHour()) beginChime();
        else if (pastHour()) beginFail("HOUR");
        else beginLift("EARLY");
        return;
    }
    const char* why = "MISS";
    float dx = ball_.x - kCupX;
    if (dx < -36.f) why = "SHORT";
    else if (std::fabs(ball_.y - kCupY) < 18.f && std::fabs(dx) < 22.f) why = "LIP";
    else if (dx > 10.f) why = "HOT";
    beginLift(why);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    anim_ += kDt;
    const gs::Pad& pad = sys.pad;
    bool clockOn = mode_ == Mode::Aim || mode_ == Mode::Roll || mode_ == Mode::Lift;
    if (clockOn) playFrames_++;

    if (mode_ == Mode::Title) {
        bool go = pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A);
        if (bot_) go = hold_++ > 18;
        if (go) {
            if (!solved_) beginFail("RULES");
            else {
                ball_.x = kTeeX;
                ball_.y = kTeeY;
                aim_ = solAng_;
                mode_ = Mode::Aim;
                hold_ = 0;
            }
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        }
    } else if (mode_ == Mode::Aim) {
        if (pastHour()) {
            beginFail("HOUR");
        } else if (bot_) {
            aim_ = solAng_;
            meter_ = std::clamp((solSpd_ - 80.f) / 200.f, 0.f, 1.f);
            if (framesUntilHour() == solTravel_) launch(solAng_, solSpd_);
        } else {
            if (pad.down(gs::BTN_LEFT)) aim_ += 48.f * kDt;
            if (pad.down(gs::BTN_RIGHT)) aim_ -= 48.f * kDt;
            if (std::fabs(pad.axisX) > 0.2f) aim_ -= pad.axisX * 54.f * kDt;
            aim_ = std::clamp(aim_, 4.f, 88.f);
            if (pad.pressed(gs::BTN_A)) charging_ = true;
            if (charging_) {
                meter_ += meterDir_ * 0.85f * kDt;
                if (meter_ >= 1.f) {
                    meter_ = 1.f;
                    meterDir_ = -1.f;
                }
                if (meter_ <= 0.08f) {
                    meter_ = 0.08f;
                    meterDir_ = 1.f;
                }
                bool release = pad.pressed(gs::BTN_B) || (pad.pressed(gs::BTN_A) == false && !pad.down(gs::BTN_A));
                if (release && meter_ > 0.05f) launch(aim_, 80.f + meter_ * 200.f);
            }
        }
    } else if (mode_ == Mode::Roll) {
        if (stepBall(ball_) || !ball_.live) settle();
    } else if (mode_ == Mode::Lift) {
        if (pastHour()) beginFail("HOUR");
        else if (++hold_ > 28) {
            ball_ = {};
            ball_.x = kTeeX;
            ball_.y = kTeeY;
            charging_ = false;
            mode_ = Mode::Aim;
            hold_ = 0;
        }
    } else if (mode_ == Mode::Chime) {
        if (hold_ == 20 && sys_) sys_->apu.tone(0, 659.f, 0.09f);
        if (hold_ == 40 && sys_) sys_->apu.tone(0, 784.f, 0.09f);
        if (hold_ == 64 && sys_) sys_->apu.tone(0, 1046.f, 0.08f);
        if (++hold_ > 90) {
            over_ = true;
            mode_ = Mode::Over;
            if (!sys.headless) sys.quit();
        }
    } else if (mode_ == Mode::Fail) {
        if (++hold_ > 70) {
            over_ = true;
            mode_ = Mode::Over;
            if (!sys.headless && !bot_) sys.quit();
        }
    } else if (mode_ == Mode::Over) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
            solve();
            showTitle();
        }
    }
    draw();
}

void Game::spr(const gs::Image& img, float cx, float cy, float w, float h, int pal) {
    if (!sys_ || img.w == 0) return;
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = img;
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::handAt(float cx, float cy, float ang, float len) {
    int n = std::max(2, int(len / 4.f));
    for (int i = 1; i <= n; i++) {
        float t = len * (float(i) / float(n));
        float x = cx + std::sin(ang) * t;
        float y = cy - std::cos(ang) * t;
        spr(art_.pip, x, y, 3.f, 3.f, PAL_HAND);
    }
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
        uint16_t c = gs::rgb4(3, 5, 10);
        if (y > 40) c = gs::rgb4(5, 8, 12);
        if (y > 90) c = gs::rgb4(7, 10, 12);
        if (y > 150) c = gs::rgb4(4, 8, 6);
        v.lineBackdrop[y] = c;
    }
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    spr(art_.green, 160.f, 115.f, float(art_.green.w), float(art_.green.h), PAL_GREEN);
    spr(art_.clock, 160.f, 36.f, float(art_.clock.w), float(art_.clock.h), PAL_CLOCK);

    int h, m, s;
    face(h, m, s);
    float secA = (s / 60.f) * 2.f * kPi;
    float minA = ((m + s / 60.f) / 60.f) * 2.f * kPi;
    float hourA = ((float((h % 12)) + m / 60.f) / 12.f) * 2.f * kPi;
    handAt(160.f, 40.f, hourA, 9.f);
    handAt(160.f, 40.f, minA, 13.f);
    handAt(160.f, 40.f, secA, 15.f);

    spr(art_.flag, kCupX + 2.f, kCupY - 16.f, float(art_.flag.w), float(art_.flag.h), PAL_FLAG);
    spr(art_.golfer, kTeeX - 8.f, kTeeY - 6.f, float(art_.golfer.w), float(art_.golfer.h), PAL_GOLFER);

    if (mode_ == Mode::Aim || mode_ == Mode::Title) {
        float a = aim_ * (kPi / 180.f);
        float reach = 18.f + meter_ * 36.f;
        for (int i = 1; i <= 5; i++) {
            float t = reach * (float(i) / 5.f);
            spr(art_.pip, kTeeX + std::cos(a) * t, kTeeY - std::sin(a) * t, 3.f, 3.f, PAL_AIM);
        }
    }

    float bx = ball_.holed ? kCupX : ball_.x;
    float by = ball_.holed ? kCupY + 2.f : ball_.y;
    if (mode_ != Mode::Lift) spr(art_.ball, bx, by, 9.f, 9.f, PAL_BALL);

    char buf[48];
    std::snprintf(buf, sizeof(buf), "%d:%02d:%02d", h, m, s);
    bool hot = onHour() || (mode_ == Mode::Chime) || (mode_ == Mode::Over && won_);
    hudC(1, buf, hot ? PAL_WIN : PAL_GOLD);
    std::snprintf(buf, sizeof(buf), "BALL %d/%d", std::min(used_ + (mode_ == Mode::Aim || mode_ == Mode::Title ? 1 : 0), kBalls),
                  kBalls);
    hud(1, 26, buf, PAL_INK);

    if (mode_ == Mode::Title) {
        spr(art_.title, 160.f, 78.f, float(art_.title.w), float(art_.title.h), PAL_TITLE);
        hudC(16, "CUP ON TWELVE", PAL_GOLD);
        hudC(18, "A AIM  RELEASE PUTT", PAL_INK);
    } else if (mode_ == Mode::Aim) {
        int until = framesUntilHour();
        if (onHour()) hudC(24, "THE HOUR", PAL_WIN);
        else if (until > 0) {
            std::snprintf(buf, sizeof(buf), "HOUR IN %d", (until + kFpc - 1) / kFpc);
            hudC(24, buf, PAL_AIM);
        }
        hudC(25, charging_ ? "RELEASE" : "HOLD A", PAL_INK);
    } else if (mode_ == Mode::Roll) {
        hudC(24, "ROLLING", PAL_AIM);
    } else if (mode_ == Mode::Lift) {
        hudC(23, reason_, PAL_ALERT);
        hudC(24, "LIFTED", PAL_INK);
    } else if (mode_ == Mode::Chime || (mode_ == Mode::Over && won_)) {
        hudC(22, "THE HOUR CHIMES", PAL_WIN);
        hudC(24, "CUP", PAL_GOLD);
    } else if (mode_ == Mode::Fail || (mode_ == Mode::Over && !won_)) {
        hudC(22, "HOUR GONE", PAL_ALERT);
        hudC(24, reason_[0] ? reason_ : "OPEN", PAL_INK);
    }
}

}  // namespace golfchime

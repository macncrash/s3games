#include "game/boccechime.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace boccechime {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kDrag = 72.f;
constexpr float kStop = 1.6f;
constexpr float kL = 42.f;
constexpr float kR = 278.f;
constexpr float kBack = 40.f;
constexpr float kFoul = 156.f;
constexpr float kFoot = 204.f;
constexpr float kBowlR = 7.f;
constexpr float kPallX = 160.f;
constexpr float kPallY = 80.f;
constexpr float kRing = 16.f;
constexpr float kThrowY = 192.f;
constexpr float kCourtX = 160.f;
constexpr float kCourtY = 118.f;

float span(float x, float y) { return std::sqrt(x * x + y * y); }

}  // namespace

int Game::secAt(int frames) const {
    if (frames < 0) frames = 0;
    return kStartSec + frames / kFpc;
}

int Game::clockSec() const { return secAt(playFrames_); }

bool Game::onHour() const {
    int sec = clockSec();
    return sec >= kHourSec && sec < kHourSec + kGraceSec;
}

bool Game::pastHour() const { return clockSec() >= kHourSec + kGraceSec; }

void Game::split(int& h, int& m, int& s) const {
    int t = clockSec();
    if (t < 0) t = 0;
    h = t / 3600;
    m = (t / 60) % 60;
    s = t % 60;
}

int Game::hour() const {
    int h, m, s;
    split(h, m, s);
    return h;
}

int Game::minute() const {
    int h, m, s;
    split(h, m, s);
    return m;
}

int Game::second() const {
    int h, m, s;
    split(h, m, s);
    return s;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.HUD.enabled = true;
    sys.apu.setMaster(0.34f);
    toTitle();
}

void Game::toTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    clockOn_ = false;
    used_ = 0;
    dead_ = 0;
    hold_ = 0;
    playFrames_ = 0;
    strikes_ = 0;
    chimeFrames_ = 0;
    why_ = "";
    aim_ = 0.f;
    power_ = 0.5f;
    powerDir_ = 1.f;
    for (Bowl& b : bowl_) b = Bowl{};
}

void Game::begin() {
    used_ = 0;
    dead_ = 0;
    hold_ = 0;
    playFrames_ = 0;
    strikes_ = 0;
    chimeFrames_ = 0;
    won_ = false;
    over_ = false;
    why_ = "";
    aim_ = 0.f;
    power_ = 0.55f;
    powerDir_ = 1.f;
    clockOn_ = true;
    for (Bowl& b : bowl_) b = Bowl{};
    mode_ = Mode::Aim;
}

void Game::launch() {
    if (used_ >= 3 || mode_ != Mode::Aim) return;
    Bowl& b = bowl_[used_++];
    b.live = true;
    b.moving = true;
    b.burned = false;
    b.y = kThrowY;
    b.x = 160.f;

    float tx = kPallX;
    float ty = kPallY;
    if (!bot_) {
        float reach = 36.f + power_ * 160.f;
        float ang = std::clamp(aim_, -0.9f, 0.9f);
        tx = b.x + std::sin(ang) * reach;
        ty = b.y - std::cos(ang) * reach;
    }
    float dx = tx - b.x;
    float dy = ty - b.y;
    float d = span(dx, dy);
    if (d < 1.f) {
        b.x = tx;
        b.y = ty;
        b.vx = b.vy = 0.f;
        b.moving = false;
    } else {
        float v = std::sqrt(2.f * kDrag * d);
        b.vx = dx / d * v;
        b.vy = dy / d * v;
    }
    if (sys_) sys_->apu.noiseBurst(0.16f, 480.f, 0.05f);
    mode_ = Mode::Roll;
    hold_ = 0;
}

void Game::coast(float dt) {
    for (int i = 0; i < used_; i++) {
        Bowl& b = bowl_[i];
        if (!b.moving) continue;
        float sp = span(b.vx, b.vy);
        if (sp < kStop) {
            b.vx = b.vy = 0.f;
            b.moving = false;
            continue;
        }
        float stopIn = sp / kDrag;
        float use = dt;
        bool halt = false;
        if (use >= stopIn) {
            use = stopIn;
            halt = true;
        }
        float dist = sp * use - 0.5f * kDrag * use * use;
        b.x += b.vx / sp * dist;
        b.y += b.vy / sp * dist;
        if (halt) {
            b.vx = b.vy = 0.f;
            b.moving = false;
        } else {
            float ns = sp - kDrag * use;
            b.vx = b.vx / sp * ns;
            b.vy = b.vy / sp * ns;
        }
        if (b.x < kL + kBowlR) {
            b.x = kL + kBowlR;
            b.vx = std::fabs(b.vx) * 0.4f;
            b.burned = true;
        }
        if (b.x > kR - kBowlR) {
            b.x = kR - kBowlR;
            b.vx = -std::fabs(b.vx) * 0.4f;
            b.burned = true;
        }
        if (b.y < kBack + kBowlR) {
            b.y = kBack + kBowlR;
            b.vy = std::fabs(b.vy) * 0.3f;
            b.burned = true;
        }
        if (b.y > kFoot - kBowlR) {
            b.y = kFoot - kBowlR;
            b.vy = -std::fabs(b.vy) * 0.2f;
        }
    }
}

void Game::beginChime() {
    won_ = true;
    why_ = "CHIME";
    clockOn_ = false;
    mode_ = Mode::Chime;
    hold_ = 0;
    strikes_ = 0;
    chimeFrames_ = 0;
    if (sys_) {
        sys_->apu.setEcho(0.28f, 0.35f, 0.25f);
        sys_->apu.tone(0, 523.f, 0.16f);
        if (!sys_->headless) sys_->rumble(0.2f, 0.5f, 160);
    }
}

void Game::miss(const char* why) {
    why_ = why;
    dead_++;
    if (std::strcmp(why, "EARLY") == 0) {
        if (used_ > 0) bowl_[used_ - 1].live = false;
        mode_ = Mode::Early;
    } else {
        mode_ = Mode::Dead;
    }
    hold_ = 0;
    if (sys_) sys_->apu.tone(0, 140.f, 0.07f);
}

void Game::fail(const char* why) {
    why_ = why;
    won_ = false;
    clockOn_ = false;
    mode_ = Mode::Fail;
    hold_ = 0;
}

void Game::settle() {
    if (used_ <= 0) return;
    Bowl& b = bowl_[used_ - 1];
    if (b.burned && b.y <= kBack + kBowlR + 1.f) {
        miss("LONG");
        return;
    }
    if (b.burned) {
        miss("WIDE");
        return;
    }
    if (b.y > kFoul) {
        miss("SHORT");
        return;
    }
    float d = span(b.x - kPallX, b.y - kPallY);
    if (d > kRing) {
        miss("OUT");
        return;
    }
    if (pastHour()) {
        miss("LATE");
        return;
    }
    if (!onHour()) {
        miss("EARLY");
        return;
    }
    beginChime();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    anim_ += kDt;
    if (clockOn_) playFrames_++;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (bot_ || pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) begin();
    } else if (mode_ == Mode::Aim) {
        if (pastHour()) {
            fail("LATE");
        } else if (bot_) {
            if (onHour()) launch();
        } else {
            if (pad.down(gs::BTN_LEFT)) aim_ -= 1.15f * kDt;
            if (pad.down(gs::BTN_RIGHT)) aim_ += 1.15f * kDt;
            aim_ = std::clamp(aim_, -0.9f, 0.9f);
            power_ += powerDir_ * kDt * 0.65f;
            if (power_ >= 1.f) {
                power_ = 1.f;
                powerDir_ = -1.f;
            }
            if (power_ <= 0.12f) {
                power_ = 0.12f;
                powerDir_ = 1.f;
            }
            if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B)) launch();
        }
    } else if (mode_ == Mode::Roll) {
        coast(kDt);
        bool moving = false;
        for (int i = 0; i < used_; i++)
            if (bowl_[i].moving) moving = true;
        if (!moving) {
            hold_++;
            if (hold_ > (bot_ ? 3 : 14)) settle();
        } else {
            hold_ = 0;
        }
    } else if (mode_ == Mode::Early || mode_ == Mode::Dead) {
        hold_++;
        int wait = bot_ ? 10 : 42;
        if (hold_ > wait) {
            if (dead_ >= 3 || pastHour()) fail(pastHour() ? "LATE" : "SPENT");
            else {
                mode_ = Mode::Aim;
                hold_ = 0;
            }
        }
    } else if (mode_ == Mode::Chime) {
        chimeFrames_++;
        if (chimeFrames_ > 0 && (chimeFrames_ % 6) == 0 && strikes_ < 12) {
            strikes_++;
            float f = 392.f + float(strikes_ % 3) * 40.f;
            sys.apu.tone(0, f, 0.12f);
        }
        if (strikes_ >= 12 && chimeFrames_ > 12 * 6 + 20) {
            mode_ = Mode::Leave;
            hold_ = 0;
        }
    } else if (mode_ == Mode::Leave) {
        hold_++;
        if (hold_ > (bot_ ? 6 : 36)) {
            over_ = true;
            mode_ = Mode::Over;
            if (!sys.headless) sys.quit();
        }
    } else if (mode_ == Mode::Fail) {
        hold_++;
        if (hold_ > (bot_ ? 8 : 30)) {
            over_ = true;
            mode_ = Mode::Over;
        }
    } else if (mode_ == Mode::Over) {
        if (!won_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) toTitle();
    }
    draw();
}

void Game::spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool shadow) {
    if (!sys_ || img.w == 0) return;
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
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

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(s ? std::strlen(s) : 0) / 2, row, s, pal); }

void Game::clockLine() {
    char buf[24];
    std::snprintf(buf, sizeof(buf), "%d:%02d:%02d", hour(), minute(), second());
    hudC(1, buf, onHour() ? PAL_GOLD : PAL_INK);
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        int sky = y < 26 ? 2 : (y < 46 ? 3 : 4);
        v.lineBackdrop[y] = gs::rgb4(sky, sky + 1, sky + 4);
    }

    spr(art_.court, kCourtX, kCourtY, float(art_.court.w), float(art_.court.h), PAL_COURT);
    spr(art_.clock, kPallX, kPallY - 28.f, float(art_.clock.w), float(art_.clock.h), PAL_CLOCK);
    spr(art_.pallino, kPallX, kPallY, 8.f, 8.f, PAL_PALL);
    for (int i = 0; i < used_; i++) {
        if (!bowl_[i].live) continue;
        spr(art_.bowl, bowl_[i].x + 2.f, bowl_[i].y + 3.f, 14.f, 7.f, PAL_BOWL, true);
        spr(art_.bowl, bowl_[i].x, bowl_[i].y, 16.f, 16.f, PAL_BOWL);
    }

    bool showBanner = mode_ == Mode::Title || mode_ == Mode::Chime || mode_ == Mode::Leave || (mode_ == Mode::Over && won_);
    if (showBanner)
        spr(art_.banner, 160.f, 18.f, float(art_.banner.w), float(art_.banner.h),
            mode_ == Mode::Title ? PAL_TITLE : PAL_WIN);

    if (mode_ == Mode::Title) {
        hudC(18, "THE HOUR HAS TO CHIME", PAL_INK);
        hudC(19, "INSIDE THE RING ON TWELVE", PAL_HINT);
        hudC(22, "START", PAL_TITLE);
    } else if (mode_ == Mode::Aim || mode_ == Mode::Roll) {
        clockLine();
        char buf[32];
        std::snprintf(buf, sizeof(buf), "BOWL %d   DEAD %d", used_ + (mode_ == Mode::Aim ? 1 : 0), dead_);
        hud(2, 3, buf, PAL_HINT);
        if (mode_ == Mode::Aim && !bot_) {
            int bars = int(power_ * 10.f + 0.5f);
            if (bars > 10) bars = 10;
            char meter[24];
            std::snprintf(meter, sizeof(meter), "POWER %.*s%.*s", bars, "##########", 10 - bars, "..........");
            hudC(26, meter, PAL_TITLE);
            hudC(25, "LEFT RIGHT   A BOWL", PAL_HINT);
        } else if (mode_ == Mode::Roll) {
            hudC(26, "ROLLING", PAL_INK);
        } else if (onHour()) {
            hudC(26, "THE HOUR", PAL_GOLD);
        } else {
            hudC(26, "WAIT FOR TWELVE", PAL_HINT);
        }
    } else if (mode_ == Mode::Early) {
        clockLine();
        hudC(24, "EARLY", PAL_DEAD);
        hudC(25, "THE HOUR HAS NOT CHIMED", PAL_HINT);
    } else if (mode_ == Mode::Dead) {
        clockLine();
        hudC(24, why_, PAL_DEAD);
        hudC(25, "THAT BOWL DIED", PAL_DEAD);
    } else if (mode_ == Mode::Chime || mode_ == Mode::Leave || (mode_ == Mode::Over && won_)) {
        clockLine();
        hudC(24, "THE HOUR CHIMES", PAL_WIN);
        char buf[24];
        std::snprintf(buf, sizeof(buf), "STRIKE %d", strikes_ > 12 ? 12 : strikes_);
        hudC(25, buf, PAL_GOLD);
    } else if (mode_ == Mode::Fail || (mode_ == Mode::Over && !won_)) {
        clockLine();
        hudC(24, why_[0] ? why_ : "GONE", PAL_DEAD);
        hudC(25, "THE HOUR IS GONE", PAL_DEAD);
    }
}

}  // namespace boccechime

#include "game/tablechime.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace tablechime {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kSweet = 0.045f;
constexpr float kMeterRate = 1.15f;
constexpr int kFlight = 28;

}  // namespace

bool Game::audit() const {
    if (spotAt(kCupX, kCupY) != Spot::Cup) return false;
    if (spotAt(kCupX + kCupR - 0.5f, kCupY) != Spot::Cup) return false;
    if (spotAt(kServeX, kNetY) != Spot::Net) return false;
    if (spotAt(kServeX, kServeY) != Spot::Short) return false;
    if (spotAt(kCupX, (kT + kNetY) * 0.5f) != Spot::Wide && spotAt(kCupX + 36.f, kCupY) != Spot::Wide) return false;
    if (spotAt(6.f, 6.f) != Spot::Hot) return false;
    if (kCupY >= kNetY - kNetH) return false;
    if (kStartSec >= kHourSec || kGraceSec < 8 || kFpc < 1) return false;
    return true;
}

bool Game::sweet() const { return std::fabs(meter_ - 0.5f) <= kSweet; }

void Game::stepMeter(float& m, float& dir) const {
    m += dir * kMeterRate * kDt;
    if (m >= 1.f) {
        m = 1.f;
        dir = -1.f;
    } else if (m <= 0.f) {
        m = 0.f;
        dir = 1.f;
    }
}

int Game::secAt(int frames) const {
    if (frames < 0) frames = 0;
    return kStartSec + frames / kFpc;
}

int Game::clockSec() const { return secAt(playFrames_); }

int Game::framesUntilHour() const {
    int sec = clockSec();
    int sub = playFrames_ % kFpc;
    if (playFrames_ < 0) return (kHourSec - kStartSec) * kFpc;
    if (sec > kHourSec) return -((sec - kHourSec) * kFpc + sub);
    if (sec == kHourSec) return -sub;
    int secLeft = kHourSec - sec;
    return (secLeft - 1) * kFpc + (kFpc - sub);
}

bool Game::onHourAt(int frames) const {
    int sec = secAt(frames);
    return sec >= kHourSec && sec < kHourSec + kGraceSec;
}

bool Game::onHour() const { return onHourAt(playFrames_); }

bool Game::pastHour() const { return clockSec() >= kHourSec + kGraceSec; }

void Game::face(int& h, int& m, int& s) const {
    int t = clockSec();
    if (t < 0) t = 0;
    s = t % 60;
    m = (t / 60) % 60;
    h = t / 3600;
    if (h > 12) h = 12;
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

void Game::toTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    tries_ = 0;
    playFrames_ = 0;
    titleFrames_ = 0;
    reason_ = "";
    bed_ = "";
    miss_ = "";
    firm_ = false;
    clockOn_ = false;
    aimX_ = kServeX;
    aimY_ = 148.f;
    ballX_ = kServeX;
    ballY_ = kServeY;
    hop_ = 0;
    meter_ = 0.15f;
    meterDir_ = 1.f;
    bellAmp_ = 0.2f;
    lastSec_ = -1;
}

void Game::newGame() {
    tries_ = 0;
    playFrames_ = 0;
    won_ = false;
    over_ = false;
    reason_ = "";
    bed_ = "";
    miss_ = "";
    clockOn_ = true;
    lastSec_ = -1;
    bellAmp_ = 0.2f;
    beginAim();
}

void Game::beginAim() {
    mode_ = Mode::Aim;
    firm_ = false;
    flightT_ = 0;
    hop_ = 0;
    showT_ = 0;
    ballX_ = kServeX;
    ballY_ = kServeY;
    aimX_ = kServeX;
    aimY_ = 148.f;
    if (bot_) {
        meter_ = 0.08f;
        meterDir_ = 1.f;
    }
}

void Game::launch(bool firm) {
    if (tries_ >= kTries) {
        beginFail("SPENT");
        return;
    }
    firm_ = firm;
    tries_++;
    flightT_ = 0;
    mode_ = Mode::Flight;
    float x = aimX_;
    float y = aimY_;
    if (!firm) {
        float dx = x - kCupX;
        float dy = y - kCupY;
        float d = std::hypot(dx, dy);
        if (d < 1.f) {
            dx = 1.f;
            dy = 0.4f;
            d = 1.f;
        }
        x += dx / d * 28.f;
        y += dy / d * 18.f;
    } else if (spotAt(x, y) == Spot::Cup) {
        x = kCupX;
        y = kCupY;
    }
    landX_ = x;
    landY_ = y;
    blip(2, 220.f, 0.07f, 0.05f);
}

void Game::beginChime() {
    won_ = true;
    reason_ = "CHIME";
    bed_ = "CUP";
    mode_ = Mode::Chime;
    chimeFrames_ = 0;
    strikes_ = 0;
    clockOn_ = false;
    bellAmp_ = 1.f;
    bellTick_ = 0.02f;
    ballX_ = kCupX;
    ballY_ = kCupY;
    hop_ = 0;
}

void Game::beginEarly() {
    reason_ = "EARLY";
    bed_ = "";
    mode_ = Mode::Early;
    showT_ = 0;
    ballX_ = kCupX;
    ballY_ = kCupY - 10.f;
    hop_ = 0;
    blip(1, 240.f, 0.05f, 0.1f);
}

void Game::beginDead(const char* why) {
    miss_ = why;
    reason_ = why;
    mode_ = Mode::Dead;
    showT_ = 0;
    hop_ = 0;
    ballX_ = landX_;
    ballY_ = landY_;
    blip(2, 96.f, 0.06f, 0.1f);
}

void Game::beginFail(const char* why) {
    if (!won_) reason_ = why;
    won_ = false;
    bed_ = "";
    mode_ = Mode::Fail;
    failFrames_ = 0;
    clockOn_ = false;
    blip(0, 90.f, 0.06f, 0.16f);
}

void Game::resolve() {
    Spot s = spotAt(landX_, landY_);
    bool hour = onHour();
    if (s == Spot::Cup && firm_) {
        if (hour) beginChime();
        else if (pastHour()) beginFail("LATE");
        else beginEarly();
        return;
    }
    const char* why = "HOT";
    if (s == Spot::Short) why = "SHORT";
    else if (s == Spot::Net) why = "NET";
    else if (s == Spot::Wide || s == Spot::Cup) why = "LIP";
    beginDead(why);
}

void Game::botAim() {
    float dx = kCupX - aimX_;
    float dy = kCupY - aimY_;
    float dist = std::hypot(dx, dy);
    if (dist > 0.7f) {
        float step = std::min(6.f, dist);
        aimX_ += dx / dist * step;
        aimY_ += dy / dist * step;
        return;
    }
    aimX_ = kCupX;
    aimY_ = kCupY;
    if (spotAt(aimX_, aimY_) != Spot::Cup) return;
    if (pastHour()) {
        beginFail("LATE");
        return;
    }
    float m = meter_;
    float d = meterDir_;
    for (int f = 0; f < 1400; f++) {
        int landAt = playFrames_ + f + kFlight;
        bool sw = std::fabs(m - 0.5f) <= kSweet;
        if (sw && onHourAt(landAt)) {
            if (f == 0) launch(true);
            return;
        }
        if (secAt(landAt) >= kHourSec + kGraceSec) {
            if (f == 0) beginFail("LATE");
            return;
        }
        stepMeter(m, d);
    }
}

void Game::humanAim(const gs::Pad& p, bool fire) {
    float mx = 0, my = 0;
    if (p.down(gs::BTN_LEFT)) mx -= 1.f;
    if (p.down(gs::BTN_RIGHT)) mx += 1.f;
    if (p.down(gs::BTN_UP)) my -= 1.f;
    if (p.down(gs::BTN_DOWN)) my += 1.f;
    mx += p.axisX;
    my -= p.axisY;
    aimX_ = std::clamp(aimX_ + mx * 2.2f, kL + 6.f, kR - 6.f);
    aimY_ = std::clamp(aimY_ + my * 2.2f, kT + 6.f, kB - 6.f);
    if (fire) launch(sweet());
}

void Game::blip(int ch, float freq, float vol, float hold) {
    if (!sys_) return;
    sys_->apu.tone(ch, freq, vol);
    if (ch == 2) tickT_ = hold;
    else toneT_ = hold;
}

void Game::tickAudio(float dt) {
    if (!sys_) return;
    bool chiming = mode_ == Mode::Chime || mode_ == Mode::Leave || (mode_ == Mode::Over && won_);
    if (chiming) {
        bellTick_ -= dt;
        if (bellTick_ <= 0.f) {
            sys_->apu.tone(0, 523.f, 0.08f);
            sys_->apu.tone(1, 784.f, 0.05f);
            toneT_ = 0.14f;
            bellTick_ = 0.22f;
        }
    }
    if (toneT_ > 0.f) {
        toneT_ -= dt;
        if (toneT_ <= 0.f) {
            sys_->apu.tone(0, 0, 0);
            if (!chiming) sys_->apu.tone(1, 0, 0);
        }
    }
    if (tickT_ > 0.f) {
        tickT_ -= dt;
        if (tickT_ <= 0.f) sys_->apu.tone(2, 0, 0);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    art_.load(sys.vdp);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.HUD.scroll(0, 0);
    rules_ = audit();
    if (!rules_) std::fprintf(stderr, "s3tablechime rules failed\n");
    toTitle();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    bool chiming = mode_ == Mode::Chime || mode_ == Mode::Leave || (mode_ == Mode::Over && won_);
    bellPh_ += kDt * (chiming ? 14.f : 2.2f);
    if (chiming) bellAmp_ = 1.f;

    const gs::Pad& p = sys.pad;
    bool start = p.pressed(gs::BTN_START);
    bool fire = p.pressed(gs::BTN_A) || p.pressed(gs::BTN_B) || p.pressed(gs::BTN_Z);
    if (bot_) {
        start = false;
        fire = false;
    }

    if (mode_ == Mode::Title) {
        titleFrames_++;
        stepMeter(meter_, meterDir_);
        if (bot_ && titleFrames_ >= 24) newGame();
        else if (start || p.anyPressed()) newGame();
    } else if (mode_ == Mode::Pause) {
        if (start || fire) mode_ = held_;
    } else if (mode_ == Mode::Aim) {
        if (clockOn_) playFrames_++;
        stepMeter(meter_, meterDir_);
        if (pastHour()) beginFail("LATE");
        else if (!bot_ && start) {
            held_ = Mode::Aim;
            mode_ = Mode::Pause;
        } else if (bot_) botAim();
        else humanAim(p, fire);
    } else if (mode_ == Mode::Flight) {
        if (clockOn_) playFrames_++;
        flightT_++;
        float u = std::clamp(float(flightT_) / float(kFlight), 0.f, 1.f);
        ballX_ = kServeX + (landX_ - kServeX) * u;
        ballY_ = kServeY + (landY_ - kServeY) * u;
        hop_ = std::sin(u * 3.14159265f) * 42.f;
        if (flightT_ >= kFlight) resolve();
    } else if (mode_ == Mode::Early || mode_ == Mode::Dead) {
        if (clockOn_) playFrames_++;
        showT_++;
        if (showT_ > 40) {
            if (pastHour()) beginFail("LATE");
            else if (tries_ >= kTries) beginFail(mode_ == Mode::Early ? "EARLY" : "SPENT");
            else beginAim();
        }
    } else if (mode_ == Mode::Chime) {
        chimeFrames_++;
        if (chimeFrames_ > 0 && (chimeFrames_ % 6) == 0 && strikes_ < 12) {
            blip(0, 523.25f, 0.1f, 0.12f);
            if (sys_) sys_->apu.tone(1, 784.f, 0.06f);
            strikes_++;
            bellAmp_ = 1.f;
        }
        if (strikes_ >= 12 && chimeFrames_ > 12 * 6 + 24) {
            mode_ = Mode::Leave;
            leaveT_ = 0;
            reason_ = "CHIME";
            bed_ = "CUP";
        }
    } else if (mode_ == Mode::Leave) {
        leaveT_++;
        if (leaveT_ > 36) {
            mode_ = Mode::Over;
            won_ = true;
            over_ = true;
        }
    } else if (mode_ == Mode::Fail) {
        if (++failFrames_ > 40) over_ = true;
    } else if (mode_ == Mode::Over) {
        if (!bot_ && start) newGame();
    }

    if (clockOn_ && (mode_ == Mode::Aim || mode_ == Mode::Flight)) {
        int sec = clockSec();
        if (sec != lastSec_ && lastSec_ >= 0) {
            int until = framesUntilHour();
            if (until > 0 && until <= kFpc * 10) blip(0, (sec & 1) ? 880.f : 660.f, 0.04f, 0.04f);
        }
        lastSec_ = sec;
    }
    tickAudio(kDt);
    draw();
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
    bool win = mode_ == Mode::Chime || mode_ == Mode::Leave || (mode_ == Mode::Over && won_);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        uint16_t c;
        if (y < 40) c = win ? gs::rgb4(6, 4, 2) : gs::rgb4(2, 2, 4);
        else if (y < 196) c = win ? gs::rgb4(8, 6, 3) : gs::rgb4(3, 3, 5);
        else c = gs::rgb4(4, 3, 2);
        v.lineBackdrop[y] = c;
    }
}

void Game::spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool shadow) {
    if (!sys_ || img.w == 0) return;
    gs::Sprite s;
    s.img = img;
    s.w = int16_t(std::max(1.f, w));
    s.h = int16_t(std::max(1.f, h));
    s.x = int16_t(std::lround(cx - w * 0.5f));
    s.y = int16_t(std::lround(cy - h * 0.5f));
    s.pal = uint8_t(pal);
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    float cx = (kL + kR) * 0.5f;
    float cy = (kT + kB) * 0.5f;
    spr(art_.cloth, cx, cy, float(art_.cloth.w), float(art_.cloth.h), PAL_CLOTH);
    spr(art_.net, cx, kNetY, float(art_.net.w), float(art_.net.h), PAL_NET);

    float swing = std::sin(bellPh_) * 4.f * bellAmp_;
    spr(art_.cup, kCupX + swing, kCupY, float(art_.cup.w), float(art_.cup.h), PAL_CUP);
    spr(art_.clock, kClockX, kClockY, float(art_.clock.w), float(art_.clock.h), PAL_CLOCK);

    bool aiming = mode_ == Mode::Aim || mode_ == Mode::Title || mode_ == Mode::Pause;
    if (aiming) {
        spr(art_.cross, aimX_, aimY_, float(art_.cross.w), float(art_.cross.h), PAL_AIM);
        spr(art_.bat, kServeX - 20.f, kServeY + 6.f, float(art_.bat.w), float(art_.bat.h), PAL_BAT);
        spr(art_.ball, kServeX, kServeY, float(art_.ball.w), float(art_.ball.h), PAL_BALL);
    } else {
        spr(art_.bat, kServeX - 20.f, kServeY + 6.f, float(art_.bat.w), float(art_.bat.h), PAL_BAT);
        float by = ballY_ - hop_;
        if (hop_ > 6.f) spr(art_.ball, ballX_, ballY_ + 3.f, float(art_.ball.w), 4.f, PAL_BALL, true);
        spr(art_.ball, ballX_, by, float(art_.ball.w), float(art_.ball.h), PAL_BALL);
    }

    if (mode_ == Mode::Aim || mode_ == Mode::Flight || mode_ == Mode::Title) {
        int filled = int(std::clamp(meter_, 0.f, 1.f) * 12.f);
        for (int i = 0; i < 12; i++) {
            bool mid = i >= 5 && i <= 6;
            const char* ch = i < filled ? "I" : "-";
            int pal = PAL_INK;
            if (i < filled && mid && sweet()) pal = PAL_GREEN;
            else if (i < filled) pal = mid ? PAL_GOLD : PAL_INK;
            hud(2 + i, 26, ch, pal);
        }
    }

    char clk[16];
    int hh, mm, ss;
    face(hh, mm, ss);
    std::snprintf(clk, sizeof clk, "%d:%02d:%02d", hh, mm, ss);

    if (mode_ == Mode::Title) {
        hudC(1, "S3 TABLECHIME", PAL_GOLD);
        hudC(3, clk, PAL_INK);
        hudC(21, "THE HOUR HAS TO CHIME", PAL_GOLD);
        hudC(22, "FIRM IN THE FAR CUP", PAL_GREEN);
        hudC(23, "EARLY IS LIFTED", PAL_INK);
        if ((titleFrames_ & 16) == 0) hudC(25, "PRESS START", PAL_GOLD);
        else hudC(25, "ARROWS AIM  A IN GREEN", PAL_GREEN);
        return;
    }
    if (mode_ == Mode::Pause) {
        hudC(1, "PAUSED", PAL_GOLD);
        hudC(24, "START RESUME", PAL_INK);
        return;
    }
    if (mode_ == Mode::Over && won_) {
        hudC(1, "CHIME", PAL_GOLD);
        hudC(3, clk, PAL_GOLD);
        hudC(22, "THE HOUR CHIMED", PAL_INK);
        hudC(23, "CUP", PAL_GREEN);
        if (!bot_) hudC(25, "START AGAIN", PAL_GOLD);
        return;
    }
    if (mode_ == Mode::Over || mode_ == Mode::Fail) {
        hudC(1, reason_[0] ? reason_ : "LATE", PAL_ALERT);
        hudC(3, clk, PAL_INK);
        hudC(22, "THE HOUR IS GONE", PAL_INK);
        if (!bot_ && mode_ == Mode::Over) hudC(25, "START AGAIN", PAL_GOLD);
        return;
    }
    if (mode_ == Mode::Chime || mode_ == Mode::Leave) {
        hudC(1, "CHIME", PAL_GOLD);
        hudC(3, clk, PAL_GOLD);
        hudC(22, mode_ == Mode::Leave ? "LEAVE" : "TWELVE STRIKES", PAL_GREEN);
        return;
    }
    if (mode_ == Mode::Early) {
        hudC(1, "LIFTED", PAL_GOLD);
        hudC(22, "TOO EARLY FOR THE HOUR", PAL_INK);
        return;
    }
    if (mode_ == Mode::Dead) {
        hudC(1, "TRY SPENT", PAL_ALERT);
        if (std::strcmp(miss_, "SHORT") == 0) hudC(22, "DIED SHORT", PAL_ALERT);
        else if (std::strcmp(miss_, "NET") == 0) hudC(22, "DIED IN THE NET", PAL_ALERT);
        else if (std::strcmp(miss_, "HOT") == 0) hudC(22, "DIED OFF THE WOOD", PAL_ALERT);
        else hudC(22, "DIED ON THE LIP", PAL_INK);
        return;
    }

    hud(1, 0, clk, onHour() ? PAL_GOLD : PAL_INK);
    char buf[32];
    std::snprintf(buf, sizeof buf, "TRY %d", tries_ + (mode_ == Mode::Flight ? 0 : 1));
    hud(32, 0, buf, PAL_INK);
    hud(1, 1, "S3 TABLECHIME", PAL_GOLD);
    if (mode_ == Mode::Flight) {
        hudC(24, "IN THE AIR", PAL_GOLD);
        return;
    }
    Spot live = spotAt(aimX_, aimY_);
    const char* name = "OFF THE WOOD";
    int pal = PAL_ALERT;
    if (live == Spot::Cup) {
        name = sweet() ? "CUP  STRIKE" : "CUP  WAIT";
        pal = sweet() ? PAL_GREEN : PAL_GOLD;
    } else if (live == Spot::Short) {
        name = "YOUR HALF";
    } else if (live == Spot::Net) {
        name = "ON THE NET";
    } else if (live == Spot::Wide) {
        name = "FAR CLOTH";
        pal = PAL_INK;
    }
    hudC(24, name, pal);
    int until = framesUntilHour();
    if (onHour()) hudC(25, "ON THE HOUR", PAL_GOLD);
    else if (until > 0 && until < kFpc * 12) hudC(25, "HOUR COMING", PAL_ALERT);
    else hudC(25, sweet() ? "GREEN  A" : "WAIT FOR GREEN", sweet() ? PAL_GREEN : PAL_AIM);
}

}  // namespace tablechime

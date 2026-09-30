#include "game/cuechime.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace cuechime {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPeriod = 0.8f;
constexpr float kWin = 0.5f;
constexpr int kFpc = 6;
constexpr int kGraceSec = 24;
constexpr int kHourSec = 12 * 3600;
constexpr int kStartSec = kHourSec - 90;
constexpr float kCueHome = 118.f;
constexpr float kObjHome = 176.f;
constexpr float kPocket = 252.f;

bool strokeDown(const gs::Pad& p) {
    return p.pressed(gs::BTN_A) || p.pressed(gs::BTN_B) || p.pressed(gs::BTN_C);
}

}  // namespace

float Game::meter() const {
    float t = std::fmod(phase_, kPeriod) / kPeriod;
    return t < 0.5f ? t * 2.f : (1.f - t) * 2.f;
}

bool Game::inWindow() const { return meter() >= kWin; }

float Game::cueTip() const { return 28.f + meter() * 78.f; }

int Game::clockSec() const {
    int t = kStartSec + playFrames_ / kFpc;
    return t < 0 ? 0 : t;
}

int Game::framesUntilHour() const {
    int sec = clockSec();
    int sub = playFrames_ % kFpc;
    if (sec > kHourSec) return -((sec - kHourSec) * kFpc + sub);
    if (sec == kHourSec) return -sub;
    int secLeft = kHourSec - sec;
    return (secLeft - 1) * kFpc + (kFpc - sub);
}

bool Game::onHour() const {
    int sec = clockSec();
    return sec >= kHourSec && sec < kHourSec + kGraceSec;
}

bool Game::pastHour() const { return clockSec() >= kHourSec + kGraceSec; }

void Game::split(int& h, int& m, int& s) const {
    int t = (mode_ == Mode::Title) ? (kStartSec + titleFrames_ / kFpc) : clockSec();
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

void Game::hush() {
    if (toneT_ <= 0) return;
    toneT_ = std::max(0.f, toneT_ - kDt);
    if (toneT_ <= 0) {
        sys_->apu.tone(0, 0, 0);
        sys_->apu.tone(1, 0, 0);
    }
}

void Game::showTitle() {
    mode_ = Mode::Title;
    titleFrames_ = 0;
    playFrames_ = 0;
    over_ = false;
    won_ = false;
    potted_ = false;
    strokes_ = 0;
    reason_ = "";
    ballT_ = 0;
    botHeld_ = false;
    lastSec_ = -1;
    strikes_ = 0;
    phase_ = 0.2f;
    sys_->apu.silence();
}

void Game::begin() {
    playFrames_ = 0;
    strokes_ = 0;
    won_ = false;
    over_ = false;
    potted_ = false;
    botHeld_ = false;
    reason_ = "";
    ballT_ = 0;
    strikes_ = 0;
    lastSec_ = -1;
    phase_ = 0.05f;
    mode_ = Mode::Aim;
    modeFrames_ = 0;
    sys_->apu.silence();
    if (!bot_) sys_->setLight(30, 70, 40);
}

void Game::stroke() {
    if (mode_ != Mode::Aim) return;
    strokes_++;
    if (!inWindow()) {
        reason_ = "thin";
        mode_ = Mode::Thin;
        modeFrames_ = 0;
        ballT_ = 0;
        sys_->apu.tone(0, 90.f, 0.1f);
        toneT_ = 0.2f;
        return;
    }
    mode_ = Mode::Travel;
    travel_ = 0;
    ballT_ = 0;
    sys_->apu.tone(0, 320.f, 0.08f);
    sys_->apu.noiseBurst(0.05f, 800.f, 0.04f);
    toneT_ = 0.12f;
    if (!bot_) sys_->rumble(0.2f, 0.3f, 20);
}

void Game::resolve() {
    ballT_ = 1.f;
    if (onHour()) {
        potted_ = true;
        won_ = true;
        reason_ = "CHIME";
        mode_ = Mode::Chime;
        modeFrames_ = 0;
        strikes_ = 0;
        swing_ = 1.f;
        sys_->apu.tone(0, 523.f, 0.16f);
        sys_->apu.tone(1, 784.f, 0.08f);
        toneT_ = 0.4f;
        if (!bot_) {
            sys_->setLight(180, 140, 40);
            sys_->rumble(0.2f, 0.5f, 90);
        }
        return;
    }
    if (pastHour()) {
        won_ = false;
        potted_ = false;
        reason_ = "late";
        mode_ = Mode::Fail;
        modeFrames_ = 0;
        return;
    }
    reason_ = "early";
    mode_ = Mode::Early;
    modeFrames_ = 0;
    sys_->apu.tone(0, 180.f, 0.08f);
    toneT_ = 0.2f;
}

void Game::botPlay() {
    if (mode_ == Mode::Title) {
        if (titleFrames_ > 12) begin();
        return;
    }
    if (mode_ != Mode::Aim || botHeld_) return;
    if (framesUntilHour() == kTravel) {
        phase_ = kPeriod * 0.25f;
        stroke();
        botHeld_ = true;
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(0, 1, 1));
    for (int y = 0; y < gs::SCREEN_H; y++) sys.vdp.road[y].on = false;
    showTitle();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    bool clock = mode_ == Mode::Aim || mode_ == Mode::Travel || mode_ == Mode::Early || mode_ == Mode::Thin;
    if (mode_ == Mode::Title) titleFrames_++;
    if (clock) playFrames_++;

    if (mode_ == Mode::Title) {
        if (bot_) botPlay();
        else if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) begin();
    } else if (mode_ == Mode::Aim) {
        phase_ += kDt;
        if (bot_) botPlay();
        else if (strokeDown(pad)) stroke();
        if (mode_ == Mode::Aim && pastHour()) {
            won_ = false;
            reason_ = "passed";
            mode_ = Mode::Fail;
            modeFrames_ = 0;
        }
    } else if (mode_ == Mode::Travel) {
        travel_++;
        ballT_ = std::min(1.f, float(travel_) / float(kTravel));
        if (travel_ >= kTravel) resolve();
    } else if (mode_ == Mode::Early || mode_ == Mode::Thin) {
        modeFrames_++;
        if (modeFrames_ > 28) {
            ballT_ = 0;
            phase_ = 0.08f;
            botHeld_ = false;
            if (pastHour()) {
                won_ = false;
                reason_ = "passed";
                mode_ = Mode::Fail;
                modeFrames_ = 0;
            } else {
                mode_ = Mode::Aim;
                modeFrames_ = 0;
            }
        }
    } else if (mode_ == Mode::Chime) {
        modeFrames_++;
        swing_ = 0.65f + 0.35f * std::sin(modeFrames_ * 0.7f);
        if (strikes_ < 12 && (modeFrames_ % 6) == 0) {
            strikes_++;
            sys_->apu.tone(0, 494.f, 0.14f);
            sys_->apu.tone(1, 740.f, 0.06f);
            toneT_ = 0.22f;
        }
        if (modeFrames_ > 12 * 6 + 24) over_ = true;
    } else if (mode_ == Mode::Fail) {
        modeFrames_++;
        if (modeFrames_ > 36) over_ = true;
    }

    if (clock) {
        int sec = clockSec();
        if (sec != lastSec_) {
            lastSec_ = sec;
            if (mode_ == Mode::Aim) {
                sys_->apu.tone(1, sec >= kHourSec ? 880.f : 660.f, 0.04f);
                toneT_ = std::max(toneT_, 0.05f);
            }
        }
    }

    hush();
    draw();
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = 0;
    while (s[n]) n++;
    hud(20 - n / 2, row, s, pal);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool hflip) {
    if (h < 1.f || m.h < 1) return;
    float sc = h / float(m.h);
    gs::Sprite s;
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.w = int16_t(std::clamp(int(std::lround(m.w * sc)), 1, 2000));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = hflip;
    sys_->vdp.sprite(s);
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int g = y < 70 ? 1 : (y < 168 ? 2 : 1);
        v.lineBackdrop[y] = gs::rgb4(1, g, y < 36 ? 2 : 1);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::clockAt(float cx, float cy) {
    spr(art_.clock, cx, cy, 52, PAL_CLOCK);
    int h, m, s;
    split(h, m, s);
    float sub = 0;
    if (mode_ != Mode::Title) sub = float(playFrames_ % kFpc) / float(kFpc);
    float secA = (float(s) + sub) / 60.f * 6.2831853f - 1.5707963f;
    float minA = (float(m) + float(s) / 60.f) / 60.f * 6.2831853f - 1.5707963f;
    float hrA = ((h % 12) + float(m) / 60.f) / 12.f * 6.2831853f - 1.5707963f;
    auto hand = [&](float ang, float len, int n) {
        for (int i = 1; i <= n; i++) {
            float t = len * float(i) / float(n);
            spr(art_.hand, cx + std::cos(ang) * t, cy + std::sin(ang) * t, 4, PAL_CLOCK);
        }
    };
    hand(hrA, 10.f, 3);
    hand(minA, 16.f, 4);
    hand(secA, 18.f, 5);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    clockAt(160.f, 36.f);
    spr(art_.table, 168, 156, 86, PAL_CLOTH);
    spr(art_.player, 34, 128, 62, PAL_PLAYER);
    spr(art_.pocket, kPocket, 150, 16, PAL_POCKET);

    float cueX = kCueHome + ballT_ * 48.f;
    float objX = kObjHome + ballT_ * (kPocket - 6.f - kObjHome);
    if (mode_ == Mode::Aim || mode_ == Mode::Title || mode_ == Mode::Thin || mode_ == Mode::Early || mode_ == Mode::Fail)
        spr(art_.cue, cueTip(), 156, 8, PAL_CUE);
    spr(art_.cueBall, cueX, 156, 14, PAL_BALL);
    float objH = (mode_ == Mode::Chime) ? 8.f : 14.f;
    spr(art_.object, objX, mode_ == Mode::Chime ? 158.f : 156.f, objH, PAL_BALL);

    if (art_.title.w > 0 && (mode_ == Mode::Title || mode_ == Mode::Chime)) {
        gs::Sprite s;
        s.img = art_.title;
        s.w = int16_t(art_.title.w);
        s.h = int16_t(art_.title.h);
        s.x = int16_t(160 - art_.title.w / 2);
        s.y = 4;
        s.pal = PAL_CLOCK;
        v.sprite(s);
    }

    char line[48];
    int h, m, s;
    split(h, m, s);
    std::snprintf(line, sizeof(line), "%d:%02d:%02d", h, m, s);

    if (mode_ == Mode::Title) {
        hudC(8, "A SHORT CUE", PAL_HUD);
        hudC(9, "THE HOUR HAS TO CHIME", PAL_HUD);
        hudC(12, "STROKE WHEN THE TIP", PAL_HUD);
        hudC(13, "MEETS THE BALL", PAL_HUD);
        hudC(15, "ONLY THE HOUR COUNTS", PAL_HUD);
        hudC(25, "A TO BEGIN", PAL_HUD);
    } else if (mode_ == Mode::Aim) {
        hudC(8, line, onHour() ? PAL_CLOCK : PAL_HUD);
        std::snprintf(line, sizeof(line), "CUE %d", strokes_ + 1);
        hudC(9, line, PAL_HUD);
        hudC(25, inWindow() ? "STROKE" : "HOLD", inWindow() ? PAL_CLOCK : PAL_HUD);
    } else if (mode_ == Mode::Travel) {
        hudC(8, line, PAL_HUD);
        hudC(10, "THE CUE IS AWAY", PAL_HUD);
    } else if (mode_ == Mode::Early) {
        hudC(8, "TOO SOON", PAL_POCKET);
        hudC(10, "THE HOUR HAS NOT COME", PAL_HUD);
    } else if (mode_ == Mode::Thin) {
        hudC(8, "THE TIP MISSED", PAL_POCKET);
        hudC(10, line, PAL_HUD);
    } else if (mode_ == Mode::Chime) {
        hudC(8, "THE HOUR CHIMES", PAL_CLOCK);
        std::snprintf(line, sizeof(line), "%d:%02d:%02d", h, m, s);
        hudC(10, line, PAL_HUD);
        std::snprintf(line, sizeof(line), "CUE %d", strokes_);
        hudC(11, line, PAL_HUD);
    } else {
        hudC(8, "THE HOUR PASSED", PAL_POCKET);
        hudC(10, "THE CLOTH STAYS QUIET", PAL_HUD);
    }
}

}  // namespace cuechime

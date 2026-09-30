#include "game/bellchime.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace bellchime {

static_assert(kShortH + 12 < kLongH, "the bell is short");
static_assert(kShortH + 8 < kDeepH, "the short bell is shorter than the deep one");
static_assert(kShort == 1, "the middle rope is the short bell");

int Game::clockSec() const { return kStartSec + playFrames_ / kFpc; }

bool Game::onHour() const {
    int sec = clockSec();
    return sec >= kHourSec && sec < kHourSec + kGraceSec;
}

bool Game::pastHour() const { return clockSec() >= kHourSec + kGraceSec; }

void Game::faceTime(int& h, int& m, int& s) const {
    int t = clockSec();
    if (t < 0) t = 0;
    s = t % 60;
    m = (t / 60) % 60;
    h = (t / 3600) % 12;
    if (h == 0) h = 12;
}

int Game::hour() const {
    int h, m, s;
    faceTime(h, m, s);
    return h;
}

int Game::minute() const {
    int h, m, s;
    faceTime(h, m, s);
    return m;
}

int Game::second() const {
    int h, m, s;
    faceTime(h, m, s);
    return s;
}

void Game::toTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    reason_ = "";
    bell_ = "";
    pulls_ = 0;
    sel_ = 0;
    playFrames_ = 0;
    hold_ = 0;
    strikes_ = 0;
    titleWait_ = 0;
    yank_ = 0;
}

void Game::begin() {
    won_ = false;
    over_ = false;
    reason_ = "";
    bell_ = "";
    pulls_ = 0;
    sel_ = 0;
    playFrames_ = 0;
    hold_ = 0;
    strikes_ = 0;
    yank_ = 0;
    mode_ = Mode::Wait;
    sys_->apu.silence();
}

void Game::beginChime() {
    won_ = true;
    reason_ = "CHIME";
    bell_ = "SHORT";
    hold_ = 0;
    strikes_ = 0;
    mode_ = Mode::Chime;
    sys_->apu.keyOn(0, 392.f, 0.32f);
    sys_->apu.keyOn(1, 784.f, 0.18f);
    sys_->apu.noiseBurst(0.08f, 90.f, 0.05f);
    sys_->rumble(0.2f, 0.35f, 80);
}

void Game::beginGap(const char* why) {
    reason_ = why;
    hold_ = 0;
    mode_ = Mode::Gap;
    sys_->apu.tone(1, 140.f, 0.05f);
}

void Game::beginFail(const char* why) {
    if (!won_) reason_ = why;
    won_ = false;
    hold_ = 0;
    mode_ = Mode::Fail;
    sys_->apu.tone(0, 70.f, 0.06f);
}

void Game::pull() {
    pulls_++;
    yank_ = 10.f;
    bell_ = bellName(sel_);
    const bool shortBell = sel_ == kShort;
    const bool hour = onHour();
    if (shortBell && hour) {
        beginChime();
        return;
    }
    if (pastHour() || pulls_ >= kPulls) beginFail(pastHour() ? "HOUR" : (shortBell ? "EARLY" : bell_));
    else if (shortBell) beginGap("EARLY");
    else beginGap(bell_);
}

void Game::spr(const gs::Image& img, float cx, float cy, int pal, int dw, int dh) {
    if (!sys_ || img.w == 0) return;
    gs::Sprite s;
    s.img = img;
    s.w = int16_t(dw > 0 ? dw : img.w);
    s.h = int16_t(dh > 0 ? dh : img.h);
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::hand(float cx, float cy, float ang, float len, int pal) {
    int n = std::max(2, int(len / 3.f));
    for (int i = 1; i <= n; i++) {
        float t = len * (float(i) / float(n));
        spr(art_.pip, cx + std::sin(ang) * t, cy - std::cos(ang) * t, pal);
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

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    uint16_t top = gs::rgb4(1, 1, 4);
    uint16_t mid = gs::rgb4(2, 3, 6);
    uint16_t bot = gs::rgb4(2, 2, 3);
    if (mode_ == Mode::Chime || (mode_ == Mode::Over && won_)) mid = gs::rgb4(8, 6, 2);
    if (mode_ == Mode::Fail || (mode_ == Mode::Over && !won_)) mid = gs::rgb4(4, 1, 2);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H - 1);
        auto mix = [](uint16_t a, uint16_t b, float t) {
            int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
            int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
            auto L = [&](int p, int q) { return int(p + (q - p) * t + 0.5f); };
            return gs::rgb4(L(ar, br), L(ag, bg), L(ab, bb));
        };
        v.lineBackdrop[y] = u < 0.5f ? mix(top, mid, u / 0.5f) : mix(mid, bot, (u - 0.5f) / 0.5f);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    sky();

    spr(art_.beam, 160.f, 28.f, PAL_WOOD);
    bool chiming = mode_ == Mode::Chime || (mode_ == Mode::Over && won_);
    float swing = chiming ? std::sin(hold_ * 0.42f) * 8.f : 0.f;

    for (int i = 0; i < kBells; i++) {
        float x = bellX(i);
        float drop = (i == sel_ && yank_ > 0.f) ? yank_ : 0.f;
        if (chiming && i == kShort) {
            x += swing;
            drop = 4.f;
        }
        int pal = (i == kShort) ? PAL_SHORT : PAL_BELL;
        float by = 36.f + art_.bell[i].h * 0.5f + drop;
        spr(art_.bell[i], x, by, pal);
        float ropeTop = 34.f;
        float ropeBot = by - art_.bell[i].h * 0.5f;
        if (ropeBot > ropeTop + 4.f) {
            spr(art_.rope, x, (ropeTop + ropeBot) * 0.5f, PAL_ROPE, 4, int(ropeBot - ropeTop));
        }
        if (i == sel_ && mode_ != Mode::Title) spr(art_.pip, x, 18.f, PAL_GOLD);
    }

    spr(art_.clock, 28.f, 52.f, PAL_NIGHT);
    int h, m, s;
    faceTime(h, m, s);
    if (mode_ == Mode::Title) {
        h = 11;
        m = 59;
        s = 40;
    }
    float hourAng = ((float(h % 12) + float(m) / 60.f) / 12.f) * 2.f * kPi;
    float minAng = (float(m) / 60.f) * 2.f * kPi;
    float secAng = (float(s) / 60.f) * 2.f * kPi;
    hand(28.f, 52.f, hourAng, 7.f, PAL_GOLD);
    hand(28.f, 52.f, minAng, 10.f, PAL_HUD);
    hand(28.f, 52.f, secAng, 11.f, PAL_BAD);

    for (int i = 0; i < kPulls; i++) {
        int pal = i < pulls_ ? PAL_BAD : PAL_DIM;
        if (won_ && i == pulls_ - 1) pal = PAL_GOLD;
        spr(art_.lamp, 250.f + i * 14.f, 14.f, pal);
    }

    char buf[80];
    int f = int(sys_->frame);
    if (mode_ == Mode::Title) {
        hudC(3, "S3 BELLCHIME", PAL_GOLD);
        hudC(18, "A SHORT BELL", PAL_HUD);
        hudC(20, "THE HOUR HAS TO CHIME", PAL_GOLD);
        hudC(22, "LONG AND DEEP STAY QUIET", PAL_DIM);
        if ((f & 16) == 0) hudC(26, "PRESS START", PAL_GOLD);
        const char* ver = S3_VERSION_STRING;
        hud(40 - int(std::strlen(ver)), 0, ver, PAL_DIM);
    } else if (mode_ == Mode::Over && won_) {
        hudC(2, "THE HOUR CHIMES", PAL_GOLD);
        std::snprintf(buf, sizeof buf, "%d:%02d:%02d  SHORT", hour(), minute(), second());
        hudC(20, buf, PAL_GOLD);
        std::snprintf(buf, sizeof buf, "PULL %d", pulls_);
        hudC(22, buf, PAL_HUD);
    } else if (mode_ == Mode::Fail || (mode_ == Mode::Over && !won_)) {
        hudC(2, "THE HOUR IS GONE", PAL_BAD);
        hudC(22, reason_ && reason_[0] ? reason_ : "HOUR", PAL_HUD);
        if ((f & 16) == 0) hudC(26, "START", PAL_GOLD);
    } else if (mode_ == Mode::Chime) {
        hudC(2, "THE HOUR CHIMES", PAL_GOLD);
        hudC(24, "SHORT BELL", PAL_GOLD);
    } else {
        std::snprintf(buf, sizeof buf, "%d:%02d:%02d", hour(), minute(), second());
        hud(1, 1, buf, onHour() ? PAL_GOLD : PAL_HUD);
        std::snprintf(buf, sizeof buf, "%s  %d/%d", bellName(sel_), pulls_ + (mode_ == Mode::Wait ? 1 : 0), kPulls);
        hud(24, 1, buf, sel_ == kShort ? PAL_GOLD : PAL_DIM);
        if (mode_ == Mode::Gap && std::strcmp(reason_, "EARLY") == 0) hudC(25, "TOO SOON", PAL_BAD);
        else if (mode_ == Mode::Gap) hudC(25, "WRONG BELL", PAL_BAD);
        else if (onHour()) hudC(25, "THE HOUR IS HERE", PAL_GOLD);
        else hudC(25, "LEFT RIGHT  C ON SHORT", PAL_DIM);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.apu.setMaster(0.8f);
    toTitle();
    draw();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    bool clockOn = mode_ == Mode::Wait || mode_ == Mode::Gap;
    if (clockOn) playFrames_++;
    if (yank_ > 0.f) yank_ -= 1.2f;

    if (mode_ == Mode::Title) {
        bool go = pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C);
        if (bot_) go = ++titleWait_ > 8;
        if (go) begin();
    } else if (mode_ == Mode::Wait) {
        if (pastHour()) beginFail("HOUR");
        else if (bot_) {
            if (sel_ < kShort) sel_++;
            else if (sel_ > kShort) sel_--;
            else if (onHour()) pull();
        } else {
            if (pad.pressed(gs::BTN_LEFT) && sel_ > 0) sel_--;
            if (pad.pressed(gs::BTN_RIGHT) && sel_ < kBells - 1) sel_++;
            if (pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A)) pull();
        }
    } else if (mode_ == Mode::Gap) {
        if (pastHour()) beginFail("HOUR");
        else if (++hold_ > 18) {
            hold_ = 0;
            mode_ = Mode::Wait;
        }
    } else if (mode_ == Mode::Chime) {
        if (hold_ % 7 == 0 && strikes_ < 8) {
            float hz = (strikes_ % 2) ? 494.f : 392.f;
            sys.apu.tone(0, hz, 0.09f);
            strikes_++;
        }
        if (++hold_ > 40) {
            over_ = true;
            mode_ = Mode::Over;
        }
    } else if (mode_ == Mode::Fail) {
        if (++hold_ > 28) {
            over_ = true;
            mode_ = Mode::Over;
        }
    } else if (mode_ == Mode::Over) {
        if (!bot_ && !won_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C))) toTitle();
    }

    draw();
}

}  // namespace bellchime

#include "game/presschime.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace presschime {

static_assert(kShortHi >= kShortLo, "short");
static_assert(kShortHi - kShortLo < 8, "the press is short");
static_assert(kShortHi < kStroke, "short sits on the stroke");
static_assert((kHourSec - kStartSec) * kFpc % kStroke >= kShortLo, "hour meets the short span");
static_assert((kHourSec - kStartSec) * kFpc % kStroke <= kShortHi, "hour meets the short span");

int Game::clockSec() const { return kStartSec + playFrames_ / kFpc; }

int Game::phase() const {
    int p = playFrames_ % kStroke;
    if (p < 0) p += kStroke;
    return p;
}

bool Game::onHour() const {
    int sec = clockSec();
    return sec >= kHourSec && sec < kHourSec + kGraceSec;
}

bool Game::pastHour() const { return clockSec() >= kHourSec + kGraceSec; }

bool Game::onShort() const {
    int p = phase();
    return p >= kShortLo && p <= kShortHi;
}

float Game::drop() const {
    int p = phase();
    int mid = (kShortLo + kShortHi) / 2;
    int span = mid > 0 ? mid : 1;
    if (p <= mid) return float(p) / float(span);
    int back = kStroke - mid;
    if (back < 1) back = 1;
    return float(kStroke - p) / float(back);
}

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
    face_ = "";
    pulls_ = 0;
    playFrames_ = 0;
    hold_ = 0;
    strikes_ = 0;
    titleWait_ = 0;
}

void Game::begin() {
    won_ = false;
    over_ = false;
    reason_ = "";
    face_ = "";
    pulls_ = 0;
    playFrames_ = 0;
    hold_ = 0;
    strikes_ = 0;
    mode_ = Mode::Wait;
    sys_->apu.silence();
}

void Game::beginChime() {
    won_ = true;
    reason_ = "CHIME";
    face_ = "SHORT";
    hold_ = 0;
    strikes_ = 0;
    mode_ = Mode::Chime;
    sys_->apu.keyOn(0, 523.f, 0.35f);
    sys_->apu.keyOn(1, 784.f, 0.22f);
    sys_->apu.noiseBurst(0.12f, 180.f, 0.06f);
    sys_->rumble(0.2f, 0.4f, 80);
}

void Game::beginFail(const char* why) {
    if (!won_) reason_ = why;
    won_ = false;
    hold_ = 0;
    mode_ = Mode::Fail;
    sys_->apu.tone(0, 90.f, 0.06f);
}

void Game::pull() {
    pulls_++;
    const bool shortSpan = onShort();
    const bool hour = onHour();
    if (shortSpan) face_ = "SHORT";
    else if (phase() < kShortLo) face_ = "HIGH";
    else face_ = "LONG";
    if (shortSpan && hour) {
        beginChime();
        return;
    }
    if (pastHour()) beginFail("HOUR");
    else if (shortSpan) beginFail("EARLY");
    else beginFail(face_);
}

void Game::spr(const gs::Image& img, float cx, float cy, int pal) {
    if (!sys_ || img.w == 0) return;
    gs::Sprite s;
    s.img = img;
    s.w = img.w;
    s.h = img.h;
    s.x = int16_t(std::lround(cx - img.w * 0.5f));
    s.y = int16_t(std::lround(cy - img.h * 0.5f));
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

void Game::shop() {
    gs::VDP& v = sys_->vdp;
    uint16_t top = gs::rgb4(2, 1, 1);
    uint16_t mid = gs::rgb4(5, 3, 2);
    uint16_t bot = gs::rgb4(2, 2, 2);
    if (mode_ == Mode::Chime || (mode_ == Mode::Over && won_)) mid = gs::rgb4(8, 6, 2);
    if (mode_ == Mode::Fail || (mode_ == Mode::Over && !won_)) mid = gs::rgb4(5, 1, 1);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H - 1);
        auto mix = [](uint16_t a, uint16_t b, float t) {
            int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
            int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
            auto L = [&](int p, int q) { return int(p + (q - p) * t + 0.5f); };
            return gs::rgb4(L(ar, br), L(ag, bg), L(ab, bb));
        };
        v.lineBackdrop[y] = u < 0.55f ? mix(top, mid, u / 0.55f) : mix(mid, bot, (u - 0.55f) / 0.45f);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    shop();

    const float bed = kBedY;
    float d = drop();
    if (mode_ == Mode::Title) d = 0.15f;
    spr(art_.frame, kPressX, 118.f, PAL_WOOD);
    spr(art_.sheet, kPressX, bed - 18.f, PAL_PAPER);
    spr(art_.roller, kPressX - 52.f, bed - 6.f, PAL_INK);
    spr(art_.roller, kPressX + 52.f, bed - 4.f, PAL_INK);
    float fall = 62.f * d;
    spr(art_.screw, kPressX, 78.f + fall * 0.35f, PAL_IRON);
    int platenPal = onShort() && mode_ == Mode::Wait ? PAL_GOLD : PAL_IRON;
    spr(art_.platen, kPressX, 92.f + fall, platenPal);

    const float cx = 58.f, cy = 78.f;
    spr(art_.clock, cx, cy, PAL_FACE);
    int h, m, s;
    faceTime(h, m, s);
    float ha = (float(h % 12) + m / 60.f) * (kPi * 2.f / 12.f);
    float ma = m * (kPi * 2.f / 60.f);
    hand(cx, cy, ha, 12.f, PAL_WOOD);
    hand(cx, cy, ma, 18.f, PAL_IRON);
    if (onHour() || mode_ == Mode::Chime || (mode_ == Mode::Over && won_)) spr(art_.pip, cx, cy - 22.f, PAL_GOLD);

    char buf[80];
    int f = int(sys_->frame);
    if (mode_ == Mode::Title) {
        hudC(2, "S3 PRESSCHIME", PAL_GOLD);
        hudC(20, "A SHORT PRESS", PAL_HUD);
        hudC(21, "THE HOUR HAS TO CHIME", PAL_GOLD);
        if ((f & 16) == 0) hudC(26, "PRESS START", PAL_GOLD);
        const char* ver = S3_VERSION_STRING;
        hud(40 - int(std::strlen(ver)), 0, ver, PAL_DIM);
    } else if (mode_ == Mode::Over && won_) {
        hudC(2, "THE HOUR CHIMES", PAL_GOLD);
        std::snprintf(buf, sizeof buf, "%d:%02d:%02d  SHORT", hour(), minute(), second());
        hudC(22, buf, PAL_GOLD);
    } else if (mode_ == Mode::Over || mode_ == Mode::Fail) {
        hudC(2, "THE HOUR IS SILENT", PAL_BAD);
        hudC(22, reason_, PAL_HUD);
        if (mode_ == Mode::Over && (f & 16) == 0) hudC(26, "START", PAL_GOLD);
    } else {
        std::snprintf(buf, sizeof buf, "%d:%02d:%02d", hour(), minute(), second());
        hud(1, 1, buf, onHour() ? PAL_GOLD : PAL_HUD);
        hud(30, 1, onShort() ? "SHORT" : "LONG", onShort() ? PAL_GOLD : PAL_DIM);
        int mark = phase() * 20 / kStroke;
        if (mark > 19) mark = 19;
        char bar[24];
        for (int i = 0; i < 20; i++) {
            int at = i * kStroke / 20;
            bool sweet = at >= kShortLo && at <= kShortHi;
            bar[i] = (i == mark) ? '|' : (sweet ? '=' : '-');
        }
        bar[20] = 0;
        hudC(24, bar, PAL_GOLD);
        if (onHour() && onShort()) hudC(26, "THE HOUR IS HERE", PAL_GOLD);
        else if (onHour()) hudC(26, "WAIT FOR THE SHORT", PAL_HUD);
        else hudC(26, "C ON THE SHORT PRESS", PAL_DIM);
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
    bool clockOn = mode_ == Mode::Wait;
    if (clockOn) playFrames_++;

    if (mode_ == Mode::Title) {
        bool go = pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C);
        if (bot_) go = ++titleWait_ > 8;
        if (go) begin();
    } else if (mode_ == Mode::Wait) {
        if (pastHour()) beginFail("HOUR");
        else {
            bool tap = false;
            if (bot_) tap = onHour() && onShort();
            else tap = pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A);
            if (tap) pull();
        }
    } else if (mode_ == Mode::Chime) {
        if (hold_ % 6 == 0 && strikes_ < 8) {
            float hz = (strikes_ % 2) ? 659.f : 523.f;
            sys.apu.tone(0, hz, 0.08f);
            strikes_++;
        }
        if (++hold_ > 36) {
            over_ = true;
            mode_ = Mode::Over;
        }
    } else if (mode_ == Mode::Fail) {
        if (++hold_ > 24) {
            over_ = true;
            mode_ = Mode::Over;
        }
    } else if (mode_ == Mode::Over) {
        if (!bot_ && !won_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C))) toTitle();
    }

    draw();
}

}  // namespace presschime

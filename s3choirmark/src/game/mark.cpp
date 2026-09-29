#include "mark.h"

#include "../version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace choirmark {
namespace {

constexpr int LAND = 160;
constexpr int TRAVEL[3] = {84, 118, 52};
constexpr int SLOP = 8;
constexpr int LIM = SLOP * 2;
constexpr int LANE[3] = {52, 68, 84};
constexpr int X0 = 36;
constexpr int X_CUE = 176;
constexpr int X_LAND = 284;
constexpr int STALL[3] = {58, 160, 262};
constexpr float PITCH[3] = {392.00f, 329.63f, 261.63f};
constexpr float ROOT = 130.81f;
constexpr int PAL_VOICE[3] = {PAL_TREBLE, PAL_ALTO, PAL_BASS};

int ideal(int i) { return LAND - TRAVEL[i]; }

gs::FMPatch voicePatch() {
    gs::FMPatch p;
    p.alg = 7;
    p.fb = 0.1f;
    p.op[0] = {1, 0.85f, 0.04f, 0.3f, 0.7f, 0.2f};
    p.op[1] = {2, 0.2f, 0.06f, 0.35f, 0.5f, 0.25f};
    p.op[2] = {3, 0.08f, 0.08f, 0.4f, 0.35f, 0.3f};
    p.op[3] = {4, 0.04f, 0.1f, 0.45f, 0.25f, 0.35f};
    p.vol = 0.13f;
    p.drive = 0.06f;
    p.tone = 1600;
    p.vibRate = 4.4f;
    p.vibDepth = 0.006f;
    p.echo = 0.35f;
    return p;
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Victory) return 4;
    if (mode_ == Mode::Stop) return 3;
    if (mode_ == Mode::Hold) return 2;
    if (mode_ == Mode::Phrase) return 1;
    return 0;
}

int Game::gap() const {
    int lo = arr_[0], hi = arr_[0];
    for (int i = 1; i < 3; i++) {
        lo = std::min(lo, arr_[i]);
        hi = std::max(hi, arr_[i]);
    }
    return hi - lo;
}

void Game::hush() {
    for (int i = 0; i < 4; i++) sys_->apu.keyOff(i);
    for (int i = 0; i < 3; i++) sys_->apu.tone(i, 0, 0);
    breath_ = 0;
}

void Game::enterTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    finished_ = false;
    spread_ = 0;
    tick_ = 0;
    hold_ = 0;
    for (int i = 0; i < 3; i++) cued_[i] = false;
    hush();
    sys_->apu.keyOn(2, ROOT, 0.04f);
    sys_->setLight(40, 28, 80);
}

void Game::begin() {
    tick_ = 0;
    hold_ = 0;
    spread_ = 0;
    finished_ = false;
    won_ = false;
    over_ = false;
    for (int i = 0; i < 3; i++) cued_[i] = false;
    hush();
    mode_ = Mode::Phrase;
}

void Game::cue(int i) {
    if (i < 0 || i > 2 || cued_[i] || mode_ != Mode::Phrase) return;
    cued_[i] = true;
    cueAt_[i] = tick_;
    arr_[i] = tick_ + TRAVEL[i];
    sys_->apu.keyOn(i, PITCH[i], 0.14f);
}

bool Game::stillOpen() const {
    int lo = 0, hi = 0, n = 0;
    for (int i = 0; i < 3; i++) {
        if (!cued_[i]) continue;
        if (n == 0) lo = hi = arr_[i];
        else {
            lo = std::min(lo, arr_[i]);
            hi = std::max(hi, arr_[i]);
        }
        n++;
    }
    if (n == 0) return tick_ <= LAND + 48;
    if (hi - lo > LIM) return false;
    for (int i = 0; i < 3; i++) {
        if (cued_[i]) continue;
        if (tick_ + TRAVEL[i] > lo + LIM) return false;
    }
    return true;
}

bool Game::arrived() const {
    if (!cued_[0] || !cued_[1] || !cued_[2]) return false;
    int hi = std::max(arr_[0], std::max(arr_[1], arr_[2]));
    return tick_ >= hi;
}

float Game::along(int i) const {
    if (!cued_[i]) return 0;
    return std::clamp(float(tick_ - cueAt_[i]) / float(TRAVEL[i]), 0.f, 1.f);
}

bool Game::window(int i) const {
    if (mode_ != Mode::Phrase || cued_[i]) return false;
    int d = tick_ - ideal(i);
    if (d < 0) d = -d;
    return d <= SLOP;
}

void Game::chordOn() {
    for (int i = 0; i < 3; i++) sys_->apu.setVol(i, 0.2f);
    sys_->apu.keyOn(3, ROOT, 0.1f);
    sys_->rumble(0.4f, 0.55f, 140);
    sys_->setLight(255, 200, 80);
}

void Game::phraseStep() {
    const gs::Pad& pad = sys_->pad;
    if (!bot_ && pad.pressed(gs::BTN_MODE)) {
        enterTitle();
        return;
    }
    for (int i = 0; i < 3; i++) {
        if (!cued_[i] && tick_ == ideal(i) - SLOP) sys_->apu.tone(i, PITCH[i], 0.035f), breath_ = 6;
    }
    if (bot_) {
        for (int i = 0; i < 3; i++)
            if (tick_ == ideal(i)) cue(i);
    } else {
        if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_Z)) cue(0);
        if (pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_X)) cue(1);
        if (pad.pressed(gs::BTN_C)) cue(2);
    }
    if (!stillOpen()) {
        hush();
        sys_->apu.noiseBurst(0.18f, 700.f, 0.12f);
        sys_->setLight(160, 20, 20);
        mode_ = Mode::Stop;
        hold_ = 0;
        return;
    }
    if (arrived()) {
        spread_ = gap();
        chordOn();
        mode_ = Mode::Hold;
        hold_ = 0;
        return;
    }
    tick_++;
}

void Game::ink(int col, int row, const char* s, int pal) {
    if (row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::inkC(int row, const char* s, int pal) {
    int n = 0;
    while (s[n]) n++;
    ink(20 - n / 2, row, s, pal);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, int pal, bool feet, int fog) {
    if (m.w < 1 || m.h < 1) return;
    gs::Sprite s;
    s.w = int16_t(m.w);
    s.h = int16_t(m.h);
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    s.img = m.pick(float(s.h));
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::bar(int x, int y, int w, int h, int pal) {
    if (w <= 0 || h <= 0) return;
    gs::Sprite s;
    s.img = art_.bar;
    s.x = int16_t(x);
    s.y = int16_t(y);
    s.w = int16_t(w);
    s.h = int16_t(h);
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float t = y / float(gs::SCREEN_H - 1);
        int r = int(1 + 4 * t);
        int g = int(1 + 2 * t);
        int b = int(4 + 3 * (1.f - t));
        v.lineBackdrop[y] = gs::rgb4(r, g, b);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::staff() {
    bool hot = window(0) || window(1) || window(2);
    int top = LANE[0] - 10;
    int bot = LANE[2] + 10;
    for (int i = 0; i < 3; i++) {
        bar(X0, LANE[i], X_LAND - X0, 1, PAL_GOLD);
        float x;
        if (!cued_[i]) {
            int at = std::max(ideal(i), 1);
            x = X0 + (X_CUE - X0) * std::min(1.f, float(tick_) / float(at));
            spr(art_.note, x, float(LANE[i]), PAL_VOICE[i], false, 8);
        } else {
            x = X0 + (X_LAND - X0) * along(i);
            spr(art_.note, x, float(LANE[i]), PAL_VOICE[i], false, mode_ == Mode::Stop ? 8 : 0);
        }
        if (window(i)) bar(X_CUE - 6, LANE[i] - 3, 14, 6, PAL_INK);
    }
    spr(art_.mark, float(X_LAND), float(LANE[0] - 16), PAL_GOLD, false, 0);
    bar(X_CUE, top, 2, bot - top, hot ? PAL_INK : PAL_GOLD);
    bar(X_LAND, top, 3, bot - top, PAL_GOLD);
}

void Game::choir() {
    int gloom = mode_ == Mode::Stop ? 8 : 0;
    for (int i = 2; i >= 0; i--) {
        float p = mode_ == Mode::Title ? 0 : along(i);
        bool sing = mode_ == Mode::Hold || mode_ == Mode::Victory || p > 0.05f;
        if (mode_ == Mode::Title) sing = std::sin(anim_ * 0.07f + i) > 0.2f;
        int bob = int(std::lround(std::sin((anim_ + i * 11) * 0.05f)));
        float foot = 198.f + bob;
        spr(art_.singer[i][sing ? 1 : 0], float(STALL[i]), foot, PAL_VOICE[i], true, gloom);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.clear();
    v.B.clear();
    sky();
    if (mode_ == Mode::Title) spr(art_.title, 160, 22, PAL_INK, false, 0);
    if (mode_ == Mode::Victory) spr(art_.done, 160, 118, PAL_GOLD, false, 0);
    if (mode_ != Mode::Title) staff();
    choir();
    if (mode_ == Mode::Title) {
        inkC(5, "A SHORT CHOIR", PAL_INK);
        inkC(7, "THREE VOICES ON THE MARK", PAL_GOLD);
        inkC(9, "CUE A B C WHEN THE BAR GLOWS", PAL_INK);
        ink(2, 26, "START", PAL_INK);
        ink(24, 26, S3_VERSION_STRING, PAL_INK);
    } else {
        ink(1, 0, "S3 CHOIRMARK", PAL_INK);
        inkC(0, "THE MARK", PAL_GOLD);
    }
    if (mode_ == Mode::Hold) inkC(22, "HOLD THE MARK", PAL_GOLD);
    if (mode_ == Mode::Stop) {
        inkC(22, "THE PIECE STOPS", PAL_ALERT);
        inkC(24, "START", PAL_INK);
    }
    if (mode_ == Mode::Victory) inkC(24, "THE MARK IS CLOSED", PAL_GOLD);
    inkC(27, "A TREBLE   B ALTO   C BASS", PAL_INK);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    gs::FMPatch voice = voicePatch();
    for (int i = 0; i < 4; i++) sys.apu.setPatch(i, voice);
    sys.apu.setPan(0, -0.5f);
    sys.apu.setPan(1, 0.f);
    sys.apu.setPan(2, 0.5f);
    sys.apu.setMaster(0.8f);
    if (bot_) begin();
    else enterTitle();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    anim_++;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START)) begin();
        else if (pad.pressed(gs::BTN_MODE) && sys.hasHome()) sys.eject();
    } else if (mode_ == Mode::Phrase) {
        phraseStep();
    } else if (mode_ == Mode::Hold) {
        hold_++;
        bool close = bot_ || pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A);
        if (hold_ > 40 || (close && hold_ > 8)) {
            finished_ = true;
            won_ = true;
            mode_ = Mode::Victory;
            hold_ = 0;
            sys.setLight(255, 220, 120);
        }
    } else if (mode_ == Mode::Victory) {
        if (++hold_ > 36) over_ = true;
        if (!bot_ && pad.pressed(gs::BTN_START)) enterTitle();
    } else if (mode_ == Mode::Stop) {
        if (pad.pressed(gs::BTN_START)) begin();
        else if (pad.pressed(gs::BTN_MODE)) enterTitle();
    }
    if (breath_ > 0 && --breath_ == 0) {
        for (int i = 0; i < 3; i++)
            if (!cued_[i]) sys.apu.tone(i, 0, 0);
    }
    draw();
}

}  // namespace choirmark

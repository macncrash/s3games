#include "game/choirgold.h"

#include "../version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace choirgold {
namespace {

constexpr int PHRASES = 5;
constexpr int STALL_X[3] = {56, 160, 264};
constexpr int LANE_Y[3] = {36, 52, 68};
constexpr int X0 = 48;
constexpr int X_CUE = 188;
constexpr int X_LAND = 292;

struct Phrase {
    const char* name;
    int travel[3];
    int gold[3];
    int landAt;
    int slop;
    float pitch[3];
};

// Nine gold entries. Each gold is worth two. Cream is face value and is left.
constexpr Phrase PHRASE[PHRASES] = {
    {"ENTRY", {60, 90, 120}, {1, 0, 1}, 200, 14, {392.00f, 329.63f, 261.63f}},
    {"VERSE", {100, 50, 130}, {0, 1, 0}, 210, 11, {440.00f, 349.23f, 261.63f}},
    {"CANON", {45, 110, 70}, {1, 1, 0}, 190, 9, {329.63f, 261.63f, 220.00f}},
    {"REFRAIN", {80, 80, 48}, {0, 1, 1}, 180, 10, {523.25f, 392.00f, 329.63f}},
    {"CODA", {96, 64, 140}, {1, 0, 1}, 230, 7, {523.25f, 329.63f, 261.63f}},
};

int idealAt(const Phrase& p, int i) { return p.landAt - p.travel[i]; }

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Victory) return 4;
    if (mode_ == Mode::Stop) return 3;
    if (mode_ == Mode::Resolve) return 2;
    if (mode_ == Mode::Phrase || mode_ == Mode::Pause) return 1;
    return 0;
}

void Game::enterTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    goldOut_ = false;
    phrase_ = 0;
    tick_ = 0;
    hold_ = 0;
    score_ = bare_ = golds_ = cream_ = 0;
    reason_[0] = 0;
    for (int i = 0; i < 3; i++) cued_[i] = takenCream_[i] = false;
    sys_->apu.tone(0, 130.81f, 0.05f);
    sys_->setLight(48, 36, 90);
}

void Game::beginPhrase(int p) {
    phrase_ = p;
    tick_ = 0;
    hold_ = 0;
    for (int i = 0; i < 3; i++) {
        cued_[i] = false;
        takenCream_[i] = false;
        arr_[i] = 0;
    }
    mode_ = Mode::Phrase;
}

void Game::startAnthem() {
    score_ = bare_ = golds_ = cream_ = 0;
    goldOut_ = false;
    won_ = false;
    over_ = false;
    reason_[0] = 0;
    beginPhrase(0);
}

void Game::cueVoice(int i) {
    if (i < 0 || i > 2 || cued_[i] || mode_ != Mode::Phrase) return;
    const Phrase& ph = PHRASE[phrase_];
    int at = idealAt(ph, i);
    int d = tick_ - at;
    if (d < 0) d = -d;
    if (!ph.gold[i]) {
        // Cream taken on the bar scores its face. It does not double.
        cued_[i] = true;
        takenCream_[i] = true;
        arr_[i] = tick_ + ph.travel[i];
        cream_++;
        score_ += 1;
        bare_ += 1;
        sys_->apu.tone(i, ph.pitch[i], 0.05f);
        return;
    }
    if (d > ph.slop) {
        fail(tick_ < at ? "EARLY GOLD" : "LATE GOLD");
        return;
    }
    cued_[i] = true;
    arr_[i] = tick_ + ph.travel[i];
    score_ += 2;
    bare_ += 1;
    golds_++;
    goldOut_ = true;
    sys_->apu.tone(i, ph.pitch[i], 0.12f);
}

void Game::succeed() {
    sys_->apu.tone(3, 196.f, 0.08f);
    sys_->setLight(255, 196, 70);
    if (!sys_->headless) sys_->rumble(0.2f, 0.35f, 80);
    mode_ = Mode::Resolve;
    hold_ = 0;
}

void Game::fail(const char* why) {
    std::snprintf(reason_, sizeof reason_, "%s", why);
    sys_->apu.noiseBurst(0.18f, 720.f, 0.1f);
    sys_->setLight(170, 24, 24);
    mode_ = Mode::Stop;
    won_ = false;
    over_ = true;
}

void Game::judge() {
    // Only the gold counts double. Cream face points cannot buy the line.
    if (goldOut_ && golds_ > 0 && cream_ == 0 && score_ >= kLine && bare_ < kLine) {
        won_ = true;
        mode_ = Mode::Victory;
        hold_ = 0;
        sys_->apu.tone(0, 523.25f, 0.1f);
        sys_->setLight(255, 220, 130);
    } else {
        fail(cream_ > 0 ? "CREAM SANG" : "SHORT");
    }
}

void Game::phraseFrame() {
    const gs::Pad& pad = sys_->pad;
    if (!bot_ && pad.pressed(gs::BTN_START)) {
        mode_ = Mode::Pause;
        return;
    }
    if (!bot_ && pad.pressed(gs::BTN_MODE)) {
        enterTitle();
        return;
    }
    const Phrase& ph = PHRASE[phrase_];
    if (bot_) {
        for (int i = 0; i < 3; i++) {
            if (cued_[i]) continue;
            if (tick_ != idealAt(ph, i)) continue;
            if (ph.gold[i]) cueVoice(i);
            // Cream is left. Leaving is how only the gold counts double.
        }
    } else {
        if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_Z)) cueVoice(0);
        if (pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_X)) cueVoice(1);
        if (pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_Y)) cueVoice(2);
    }
    if (mode_ != Mode::Phrase) return;

    int hi = 0;
    int n = 0;
    for (int i = 0; i < 3; i++) {
        if (!ph.gold[i]) continue;
        int at = idealAt(ph, i);
        if (!cued_[i] && tick_ > at + ph.slop) {
            fail("MISSED GOLD");
            return;
        }
        if (cued_[i]) {
            hi = n == 0 ? arr_[i] : std::max(hi, arr_[i]);
            n++;
        }
    }
    int need = 0;
    for (int i = 0; i < 3; i++)
        if (ph.gold[i]) need++;
    if (n == need && tick_ >= hi) {
        int lo = hi;
        for (int i = 0; i < 3; i++)
            if (ph.gold[i]) lo = std::min(lo, arr_[i]);
        if (hi - lo > ph.slop * 2) {
            fail("SPREAD");
            return;
        }
        succeed();
        return;
    }
    tick_++;
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
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

void Game::blit(const gs::Image& img, float cx, float cy, float w, float h, int pal) {
    if (img.w == 0 || w <= 0 || h <= 0) return;
    gs::Sprite s;
    s.img = img;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        float t = y / float(gs::SCREEN_H - 1);
        int r = 1 + int(t * 3);
        int g = 1 + int(t * 2);
        int b = 3 + int(t * 4);
        v.lineBackdrop[y] = gs::rgb4(r, g, b);
    }
    blit(art_.nave, 160, 168, 320, 100, PAL_NAVE);

    const Phrase& ph = PHRASE[phrase_];
    bool show = mode_ != Mode::Title;
    if (show) {
        int top = LANE_Y[0] - 8;
        int bot = LANE_Y[2] + 10;
        blit(art_.bar, float(X_CUE), float((top + bot) / 2), 2, float(bot - top), PAL_GOLD);
        blit(art_.bar, float(X_LAND), float((top + bot) / 2), 2, float(bot - top), PAL_GOLD);
        for (int i = 0; i < 3; i++) {
            blit(art_.staff, float((X0 + X_LAND) / 2), float(LANE_Y[i]), float(X_LAND - X0), 2,
                 ph.gold[i] ? PAL_NOTE : PAL_WAX);
            float x = float(X0);
            int pal = ph.gold[i] ? PAL_NOTE : PAL_WAX;
            if (!cued_[i]) {
                int at = std::max(idealAt(ph, i), 1);
                float u = std::clamp(float(tick_) / float(at), 0.f, 1.f);
                x = float(X0) + (float(X_CUE) - float(X0)) * u;
                blit(art_.diamond, x, float(LANE_Y[i]), 11, 11, pal);
            } else {
                float u = 0.f;
                int tr = ph.travel[i];
                if (tr > 0) u = std::clamp(float(tick_ - (arr_[i] - tr)) / float(tr), 0.f, 1.f);
                x = float(X0) + (float(X_LAND) - float(X0)) * u;
                blit(art_.note, x, float(LANE_Y[i]), 14, 16, pal);
            }
        }
    }
    for (int i = 2; i >= 0; i--) {
        int bob = int(std::lround(std::sin((anim_ + i * 13) * 0.07f) * 2.f));
        bool sing = cued_[i] || mode_ == Mode::Victory || mode_ == Mode::Resolve;
        float y = 176.f + float(bob) - (sing ? 4.f : 0.f);
        blit(art_.singer[i], float(STALL_X[i]), y, 36, 52, PAL_ROBE);
    }

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(3, "S3 CHOIR GOLD", PAL_GOLD);
        hudC(5, "ONLY THE GOLD COUNTS DOUBLE", PAL_INK);
        hudC(7, "LEAVE THE CREAM", PAL_CREAM);
        hudC(9, "CUE A B C ON THE BAR", PAL_GOLD);
        hudC(11, "START", PAL_INK);
        hud(1, 26, S3_VERSION_STRING, PAL_INK);
    } else {
        hud(1, 0, "S3 CHOIR GOLD", PAL_INK);
        std::snprintf(buf, sizeof buf, "%s %d/%d", ph.name, phrase_ + 1, PHRASES);
        hudC(0, buf, PAL_GOLD);
        std::snprintf(buf, sizeof buf, "%d", score_);
        hud(40 - int(std::strlen(buf)), 0, buf, PAL_INK);
        std::snprintf(buf, sizeof buf, "GOLD %d  CREAM %d  LINE %d", golds_, cream_, kLine);
        hudC(25, buf, cream_ ? PAL_ALERT : PAL_HINT);
    }
    if (mode_ == Mode::Pause) hudC(8, "PAUSED", PAL_GOLD);
    if (mode_ == Mode::Resolve) hudC(8, "TOGETHER", PAL_GOLD);
    if (mode_ == Mode::Stop) {
        hudC(8, "THE PIECE STOPS", PAL_ALERT);
        hudC(10, reason_, PAL_ALERT);
    }
    if (mode_ == Mode::Victory) {
        hudC(8, "ONLY THE GOLD COUNTS DOUBLE", PAL_GOLD);
        std::snprintf(buf, sizeof buf, "SCORE %d  BARE %d", score_, bare_);
        hudC(10, buf, PAL_INK);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.apu.setMaster(0.75f);
    if (bot_) startAnthem();
    else enterTitle();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    anim_++;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) startAnthem();
    } else if (mode_ == Mode::Phrase) {
        phraseFrame();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Phrase;
        else if (pad.pressed(gs::BTN_MODE)) enterTitle();
    } else if (mode_ == Mode::Resolve) {
        hold_++;
        bool skip = !bot_ && pad.pressed(gs::BTN_START);
        if (hold_ > 36 || skip) {
            if (phrase_ + 1 >= PHRASES) judge();
            else beginPhrase(phrase_ + 1);
        }
    } else if (mode_ == Mode::Victory) {
        if (hold_ < 1000) hold_++;
        if (hold_ > 40) over_ = true;
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_MODE))) enterTitle();
    } else if (mode_ == Mode::Stop) {
        if (!bot_ && pad.pressed(gs::BTN_START)) startAnthem();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) enterTitle();
    }
    draw();
}

}  // namespace choirgold

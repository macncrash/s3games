#include "choir.h"

#include "../version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace choir {
namespace {

constexpr int PHRASES = 5;
constexpr int STALL_X[3] = {56, 160, 264};
constexpr int FOOT0 = 180;
constexpr int FOOT1 = 200;
constexpr int LANE_Y[3] = {40, 55, 70};
constexpr int X0 = 52;
constexpr int X_CUE = 196;
constexpr int X_LAND = 286;
constexpr int PAL_VOICE[3] = {PAL_TREBLE, PAL_ALTO, PAL_BASS};

struct Phrase {
    const char* name;
    int travel[3];
    int landAt;
    int slop;
    float pitch[3];
    float root;
};

// Cue frame is landAt - travel. Landings must sit inside two slop-widths.
constexpr Phrase PHRASE[PHRASES] = {
    {"ENTRY", {60, 90, 120}, 200, 14, {392.00f, 329.63f, 261.63f}, 130.81f},
    {"VERSE", {100, 50, 130}, 210, 11, {440.00f, 349.23f, 261.63f}, 174.61f},
    {"CANON", {45, 110, 70}, 190, 9, {329.63f, 261.63f, 220.00f}, 110.00f},
    {"REFRAIN", {80, 80, 48}, 180, 10, {523.25f, 392.00f, 329.63f}, 130.81f},
    {"CODA", {96, 64, 140}, 230, 7, {523.25f, 329.63f, 261.63f}, 130.81f},
};

constexpr bool cuesOk(const Phrase& p) {
    for (int i = 0; i < 3; i++) {
        int ideal = p.landAt - p.travel[i];
        if (ideal <= p.slop + 8) return false;
        if (p.travel[i] < 30) return false;
    }
    return true;
}
static_assert(cuesOk(PHRASE[0]) && cuesOk(PHRASE[1]) && cuesOk(PHRASE[2]) && cuesOk(PHRASE[3]) && cuesOk(PHRASE[4]),
              "phrase cues");

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

gs::FMPatch voicePatch() {
    gs::FMPatch p;
    p.alg = 7;
    p.fb = 0.12f;
    p.op[0] = {1, 0.9f, 0.05f, 0.28f, 0.78f, 0.22f};
    p.op[1] = {2, 0.22f, 0.07f, 0.36f, 0.55f, 0.28f};
    p.op[2] = {3, 0.1f, 0.09f, 0.4f, 0.4f, 0.32f};
    p.op[3] = {4, 0.05f, 0.1f, 0.45f, 0.28f, 0.36f};
    p.vol = 0.14f;
    p.drive = 0.08f;
    p.tone = 1800;
    p.vibRate = 4.6f;
    p.vibDepth = 0.007f;
    p.vibDelay = 0.12f;
    p.echo = 0.42f;
    return p;
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Victory) return 4;
    if (mode_ == Mode::Stop) return 3;
    if (mode_ == Mode::Resolve) return 2;
    if (mode_ == Mode::Phrase || mode_ == Mode::Pause) return 1;
    return 0;
}

void Game::silence() {
    for (int i = 0; i < 4; i++) sys_->apu.keyOff(i);
    for (int i = 0; i < 3; i++) sys_->apu.tone(i, 0, 0);
    breath_ = 0;
}

void Game::enterTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    phrase_ = 0;
    tick_ = 0;
    hold_ = 0;
    for (int i = 0; i < 3; i++) cued_[i] = false;
    silence();
    sys_->apu.keyOn(2, 130.81f, 0.045f);
    sys_->setLight(48, 36, 90);
}

void Game::beginPhrase(int p) {
    phrase_ = p;
    tick_ = 0;
    hold_ = 0;
    for (int i = 0; i < 3; i++) cued_[i] = false;
    silence();
    mode_ = Mode::Phrase;
}

void Game::startAnthem() {
    score_ = 0;
    lastSpread_ = 0;
    reason_[0] = 0;
    won_ = false;
    over_ = false;
    beginPhrase(0);
}

void Game::cueVoice(int i) {
    if (i < 0 || i > 2 || cued_[i] || mode_ != Mode::Phrase) return;
    cued_[i] = true;
    cueAt_[i] = tick_;
    arr_[i] = tick_ + PHRASE[phrase_].travel[i];
    sys_->apu.keyOn(i, PHRASE[phrase_].pitch[i], 0.14f);
}

void Game::breath(int i) {
    sys_->apu.tone(i, PHRASE[phrase_].pitch[i], 0.04f);
    breath_ = 6;
}

int Game::spread() const {
    int lo = arr_[0], hi = arr_[0];
    for (int i = 1; i < 3; i++) {
        lo = std::min(lo, arr_[i]);
        hi = std::max(hi, arr_[i]);
    }
    return hi - lo;
}

bool Game::possible() const {
    const Phrase& ph = PHRASE[phrase_];
    const int lim = ph.slop * 2;
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
    if (n == 0) return tick_ <= ph.landAt + 160;
    if (hi - lo > lim) return false;
    for (int i = 0; i < 3; i++) {
        if (cued_[i]) continue;
        if (tick_ + ph.travel[i] > lo + lim) return false;
    }
    return true;
}

bool Game::landed() const {
    if (!cued_[0] || !cued_[1] || !cued_[2]) return false;
    int hi = std::max(arr_[0], std::max(arr_[1], arr_[2]));
    return tick_ >= hi;
}

float Game::progress(int i) const {
    if (!cued_[i]) return 0;
    int tr = PHRASE[phrase_].travel[i];
    if (tr <= 0) return 1;
    return std::clamp(float(tick_ - cueAt_[i]) / float(tr), 0.f, 1.f);
}

bool Game::hot(int i) const {
    if (mode_ != Mode::Phrase || cued_[i]) return false;
    const Phrase& ph = PHRASE[phrase_];
    int at = ph.landAt - ph.travel[i];
    int d = tick_ - at;
    if (d < 0) d = -d;
    return d <= ph.slop;
}

void Game::swell() {
    for (int i = 0; i < 3; i++) sys_->apu.setVol(i, 0.2f);
    sys_->apu.keyOn(3, PHRASE[phrase_].root, 0.1f);
    bool last = phrase_ == PHRASES - 1;
    sys_->rumble(last ? 0.5f : 0.22f, last ? 0.7f : 0.4f, last ? 180 : 90);
    sys_->setLight(255, 196, 70);
}

void Game::choke() {
    silence();
    sys_->apu.noiseBurst(0.2f, 840.f, 0.12f);
    sys_->rumble(0.6f, 0.15f, 130);
    sys_->setLight(170, 24, 24);
}

void Game::succeed() {
    lastSpread_ = spread();
    score_ += 250 - lastSpread_;
    swell();
    mode_ = Mode::Resolve;
    hold_ = 0;
}

void Game::fail(const char* why) {
    std::snprintf(reason_, sizeof reason_, "%s", why);
    choke();
    mode_ = Mode::Stop;
    won_ = false;
    over_ = true;
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
    for (int i = 0; i < 3; i++) {
        int at = ph.landAt - ph.travel[i];
        if (!cued_[i] && tick_ == at - ph.slop) breath(i);
    }
    if (bot_) {
        for (int i = 0; i < 3; i++)
            if (tick_ == ph.landAt - ph.travel[i]) cueVoice(i);
    } else {
        if (pad.pressed(gs::BTN_A)) cueVoice(0);
        if (pad.pressed(gs::BTN_B)) cueVoice(1);
        if (pad.pressed(gs::BTN_C)) cueVoice(2);
    }
    if (!possible()) {
        if (cued_[0] && cued_[1] && cued_[2]) {
            char buf[32];
            std::snprintf(buf, sizeof buf, "SPREAD %d", spread());
            fail(buf);
        } else {
            fail("MISSED ENTRY");
        }
        return;
    }
    if (landed()) {
        succeed();
        return;
    }
    tick_++;
}

void Game::audioTail() {
    if (breath_ > 0 && --breath_ == 0) {
        for (int i = 0; i < 3; i++) sys_->apu.tone(i, 0, 0);
    }
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::hudR(int row, const std::string& s, int pal) { hud(40 - int(s.size()), row, s, pal); }

void Game::spr(const gs::Mipped& m, float cx, float cy, int pal, bool feet, int fog) {
    if (m.h < 1 || m.w < 1) return;
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

void Game::ruleAt(const gs::Image& img, int x, int y, int w, int h, int pal) {
    if (w <= 0 || h <= 0 || img.w == 0) return;
    gs::Sprite s;
    s.img = img;
    s.x = int16_t(x);
    s.y = int16_t(y);
    s.w = int16_t(w);
    s.h = int16_t(h);
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float t = y / float(gs::SCREEN_H - 1);
        v.lineBackdrop[y] = lerpC(gs::rgb4(1, 1, 4), gs::rgb4(5, 3, 6), t);
        v.lineFog[y] = 0;
    }
    bool hi = ((anim_ / 5) & 1) != 0;
    v.setColor(PAL_NAVE * 16 + 13, gs::rgb4(15, hi ? 14 : 10, hi ? 6 : 2));
}

void Game::board() {
    const Phrase& ph = PHRASE[phrase_];
    int top = LANE_Y[0] - 8;
    int bot = LANE_Y[2] + 8;
    bool anyHot = hot(0) || hot(1) || hot(2);
    for (int i = 0; i < 3; i++) {
        if (!cued_[i]) {
            int at = ph.landAt - ph.travel[i];
            float x = X0 + (X_CUE - X0) * (float(tick_) / float(std::max(at, 1)));
            if (x > X_CUE + 16) x = float(X_CUE + 16);
            spr(art_.diamond, x, float(LANE_Y[i]), PAL_VOICE[i], false, 0);
        } else {
            float x = X0 + (X_LAND - X0) * progress(i);
            spr(art_.note, x, float(LANE_Y[i]), PAL_VOICE[i], false, mode_ == Mode::Stop ? 6 : 0);
        }
        if (hot(i)) ruleAt(art_.rule, X_CUE - 7, LANE_Y[i] - 3, 16, 7, PAL_HUD);
    }
    spr(art_.fermata, float(X_LAND), float(LANE_Y[0] - 14), PAL_GOLD, false, 0);
    ruleAt(art_.rule, X_CUE, top, 2, bot - top, anyHot ? PAL_HUD : PAL_GOLD);
    ruleAt(art_.rule, X_LAND, top, 2, bot - top, PAL_GOLD);
    for (int i = 0; i < 3; i++) ruleAt(art_.staff, X0, LANE_Y[i], X_LAND - X0, 1, PAL_GOLD);
}

void Game::people() {
    bool showStep = mode_ != Mode::Title;
    int gloom = mode_ == Mode::Stop ? 7 : 0;
    for (int i = 2; i >= 0; i--) {
        float p = showStep ? progress(i) : 0;
        bool sing = false;
        if (mode_ == Mode::Title) sing = std::sin(anim_ * 0.08f + i * 2.1f) > 0.25f;
        else if (mode_ == Mode::Resolve || mode_ == Mode::Victory) sing = true;
        else sing = p > 0.04f;
        int bob = 0;
        if (!(showStep && p > 0.f && p < 1.f)) bob = int(std::lround(std::sin((anim_ + i * 17) * 0.06f)));
        float foot = float(FOOT0 + (FOOT1 - FOOT0) * p + bob);
        int letterPal = hot(i) ? PAL_GOLD : PAL_HUD;
        int letterFog = (showStep && cued_[i]) ? 8 : 0;
        spr(art_.letter[i], float(STALL_X[i]), 108, letterPal, false, letterFog);
        spr(art_.singer[i][sing ? 1 : 0], float(STALL_X[i]), foot, PAL_VOICE[i], true, gloom);
        if (hot(i)) spr(art_.halo, float(STALL_X[i]), foot - 48, PAL_FX, false, 0);
    }
    for (int i = 0; i < 4; i++) {
        float x = 24.f + float((i * 78 + anim_ / 2) % 270);
        float y = 118.f + std::sin(anim_ * 0.04f + i) * 6.f;
        spr(art_.mote, x, y, PAL_FX, false, 4);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();
    if (mode_ == Mode::Title) spr(art_.title, 160, 14, PAL_HUD, false, 0);
    if (mode_ == Mode::Resolve) spr(art_.together, 160, 116, PAL_GOLD, false, 0);
    if (mode_ == Mode::Stop) spr(art_.stopped, 160, 116, PAL_ALERT, false, 0);
    if (mode_ == Mode::Victory) spr(art_.anthem, 160, 116, PAL_GOLD, false, 0);
    if (mode_ != Mode::Title) board();
    people();

    char buf[40];
    if (mode_ == Mode::Title) {
        hudC(4, "THREE ENTRIES", PAL_HUD);
        hudC(6, "THEY LAND TOGETHER", PAL_GOLD);
        hudC(8, "OR THE PIECE STOPS", PAL_HUD);
        hudC(10, "CUE A B C ON THE BAR", PAL_GOLD);
        hud(2, 27, "START", PAL_HUD);
        hudR(27, S3_VERSION_STRING, PAL_HUD);
    } else {
        std::snprintf(buf, sizeof buf, "S3 CHOIR");
        hud(1, 0, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "%s %d/%d", PHRASE[phrase_].name, phrase_ + 1, PHRASES);
        hudC(0, buf, PAL_GOLD);
        std::snprintf(buf, sizeof buf, "%d", score_);
        hudR(0, buf, PAL_HUD);
    }
    if (mode_ == Mode::Pause) {
        hudC(5, "PAUSED", PAL_GOLD);
        hudC(7, "START RESUMES", PAL_HUD);
    }
    if (mode_ == Mode::Resolve) {
        std::snprintf(buf, sizeof buf, "SPREAD %d", lastSpread_);
        hudC(20, buf, PAL_GOLD);
    }
    if (mode_ == Mode::Stop) {
        hudC(20, reason_, PAL_ALERT);
        hudC(24, "START", PAL_HUD);
    }
    if (mode_ == Mode::Victory) {
        std::snprintf(buf, sizeof buf, "SCORE %d", score_);
        hudC(20, buf, PAL_GOLD);
        hudC(24, "START", PAL_HUD);
    }
    hudC(26, "A/Z TREBLE  B/X ALTO  C BASS", PAL_HUD);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    gs::FMPatch voice = voicePatch();
    for (int i = 0; i < 4; i++) sys.apu.setPatch(i, voice);
    sys.apu.setPan(0, -0.55f);
    sys.apu.setPan(1, 0.f);
    sys.apu.setPan(2, 0.55f);
    sys.apu.setPan(3, 0.f);
    sys.apu.setMaster(0.8f);
    sys.apu.setEcho(0.28f, 0.4f, 0.22f);
    if (bot_) startAnthem();
    else enterTitle();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    anim_++;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START)) startAnthem();
        else if (pad.pressed(gs::BTN_MODE)) sys.quit();
    } else if (mode_ == Mode::Phrase) {
        phraseFrame();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Phrase;
        else if (pad.pressed(gs::BTN_MODE)) enterTitle();
    } else if (mode_ == Mode::Resolve) {
        hold_++;
        bool skip = !bot_ && pad.pressed(gs::BTN_START);
        if (hold_ > 68 || skip) {
            if (phrase_ + 1 >= PHRASES) {
                mode_ = Mode::Victory;
                won_ = true;
                hold_ = 0;
                sys.setLight(255, 220, 130);
            } else {
                beginPhrase(phrase_ + 1);
            }
        }
    } else if (mode_ == Mode::Victory) {
        if (hold_ < 1000) hold_++;
        if (hold_ > 48) over_ = true;
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_MODE))) enterTitle();
    } else if (mode_ == Mode::Stop) {
        if (pad.pressed(gs::BTN_START)) startAnthem();
        else if (pad.pressed(gs::BTN_MODE)) enterTitle();
    }
    draw();
    audioTail();
}

}  // namespace choir

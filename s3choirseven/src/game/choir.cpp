#include "choir.h"

#include "../version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace choirseven {
namespace {

constexpr int GOAL = 7;
constexpr int STALL_X[3] = {64, 160, 256};
constexpr int LANE_Y[3] = {46, 62, 78};
constexpr int X0 = 40;
constexpr int X_CUE = 188;
constexpr int X_LAND = 292;
constexpr int PAL_VOICE[3] = {PAL_TREBLE, PAL_ALTO, PAL_BASS};

struct Phrase {
    const char* name;
    int travel[3];
    int landAt;
    int slop;
    float pitch[3];
    float root;
};

constexpr Phrase PHRASE[] = {
    {"ENTRY", {36, 52, 68}, 96, 8, {392.00f, 329.63f, 261.63f}, 130.81f},
    {"VERSE", {48, 32, 70}, 100, 8, {440.00f, 349.23f, 261.63f}, 174.61f},
    {"CANON", {28, 60, 44}, 92, 7, {329.63f, 261.63f, 220.00f}, 110.00f},
    {"LIFT", {40, 40, 56}, 94, 7, {523.25f, 392.00f, 329.63f}, 130.81f},
};
constexpr int NPHRASE = int(sizeof(PHRASE) / sizeof(PHRASE[0]));

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

gs::FMPatch voicePatch() {
    gs::FMPatch p;
    p.alg = 7;
    p.vol = 0.16f;
    p.drive = 0.06f;
    p.tone = 1600;
    p.echo = 0.35f;
    p.op[0] = {1, 0.85f, 0.04f, 0.25f, 0.7f, 0.2f};
    p.op[1] = {2, 0.2f, 0.06f, 0.3f, 0.5f, 0.25f};
    p.op[2] = {3, 0.08f, 0.08f, 0.35f, 0.35f, 0.3f};
    p.op[3] = {4, 0.04f, 0.1f, 0.4f, 0.25f, 0.35f};
    return p;
}

}  // namespace

void Game::enterTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    you_ = 0;
    them_ = 0;
    tick_ = 0;
    for (int i = 0; i < 3; i++) cued_[i] = false;
    for (int i = 0; i < 4; i++) sys_->apu.keyOff(i);
    sys_->apu.keyOn(2, 130.81f, 0.04f);
    sys_->setLight(40, 28, 80);
}

void Game::beginPhrase() {
    tick_ = 0;
    hold_ = 0;
    for (int i = 0; i < 3; i++) cued_[i] = false;
    for (int i = 0; i < 4; i++) sys_->apu.keyOff(i);
    mode_ = Mode::Phrase;
    line_[0] = 0;
}

void Game::beginMatch() {
    you_ = 0;
    them_ = 0;
    phrase_ = 0;
    won_ = false;
    over_ = false;
    beginPhrase();
}

void Game::cueVoice(int i) {
    if (i < 0 || i > 2 || cued_[i] || mode_ != Mode::Phrase) return;
    const Phrase& ph = PHRASE[phrase_ % NPHRASE];
    cued_[i] = true;
    cueAt_[i] = tick_;
    arr_[i] = tick_ + ph.travel[i];
    sys_->apu.keyOn(i, ph.pitch[i], 0.14f);
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
    const Phrase& ph = PHRASE[phrase_ % NPHRASE];
    const int lim = ph.slop * 2;
    int lo = 0, hi = 0, n = 0;
    for (int i = 0; i < 3; i++) {
        if (!cued_[i]) continue;
        if (n++ == 0) lo = hi = arr_[i];
        else {
            lo = std::min(lo, arr_[i]);
            hi = std::max(hi, arr_[i]);
        }
    }
    if (n == 0) return tick_ <= ph.landAt + 40;
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
    int tr = PHRASE[phrase_ % NPHRASE].travel[i];
    if (tr <= 0) return 1;
    return std::clamp(float(tick_ - cueAt_[i]) / float(tr), 0.f, 1.f);
}

bool Game::hot(int i) const {
    if (mode_ != Mode::Phrase || cued_[i]) return false;
    const Phrase& ph = PHRASE[phrase_ % NPHRASE];
    int at = ph.landAt - ph.travel[i];
    int d = tick_ - at;
    if (d < 0) d = -d;
    return d <= ph.slop;
}

void Game::awardYou() {
    you_++;
    yours_ = true;
    const Phrase& ph = PHRASE[phrase_ % NPHRASE];
    sys_->apu.keyOn(3, ph.root, 0.12f);
    sys_->rumble(0.25f, 0.45f, 80);
    sys_->setLight(255, 190, 60);
    std::snprintf(line_, sizeof line_, "TOGETHER  %d", you_);
    mode_ = (you_ >= GOAL && you_ > them_) ? Mode::Win : Mode::Hold;
    if (mode_ == Mode::Win) {
        won_ = true;
        over_ = true;
        std::snprintf(line_, sizeof line_, "FIRST TO SEVEN");
    }
    hold_ = 0;
}

void Game::awardThem() {
    them_++;
    yours_ = false;
    for (int i = 0; i < 4; i++) sys_->apu.keyOff(i);
    sys_->apu.noiseBurst(0.18f, 700.f, 0.1f);
    sys_->rumble(0.45f, 0.1f, 90);
    sys_->setLight(160, 30, 40);
    std::snprintf(line_, sizeof line_, "THEIR POINT  %d", them_);
    mode_ = (them_ >= GOAL && them_ > you_) ? Mode::Lose : Mode::Hold;
    if (mode_ == Mode::Lose) {
        won_ = false;
        over_ = true;
        std::snprintf(line_, sizeof line_, "THEY REACHED SEVEN");
    }
    hold_ = 0;
}

void Game::phraseFrame() {
    const gs::Pad& pad = sys_->pad;
    const Phrase& ph = PHRASE[phrase_ % NPHRASE];
    if (bot_) {
        for (int i = 0; i < 3; i++)
            if (!cued_[i] && tick_ == ph.landAt - ph.travel[i]) cueVoice(i);
    } else {
        if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_Z)) cueVoice(0);
        if (pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_X)) cueVoice(1);
        if (pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_Y)) cueVoice(2);
        if (pad.pressed(gs::BTN_MODE)) {
            enterTitle();
            return;
        }
    }
    if (!possible()) {
        awardThem();
        return;
    }
    if (landed()) {
        if (spread() <= ph.slop * 2) awardYou();
        else awardThem();
        return;
    }
    tick_++;
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

void Game::spr(const gs::Mipped& m, float cx, float cy, int pal, bool feet) {
    if (m.h < 1 || m.w < 1) return;
    gs::Sprite s;
    s.w = int16_t(m.w);
    s.h = int16_t(m.h);
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    s.img = m.pick(float(s.h));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float t = y / float(gs::SCREEN_H - 1);
        v.lineBackdrop[y] = lerpC(gs::rgb4(1, 1, 3), gs::rgb4(6, 3, 7), t * 0.85f);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::board() {
    const Phrase& ph = PHRASE[phrase_ % NPHRASE];
    for (int i = 0; i < 3; i++) {
        float y = float(LANE_Y[i]);
        spr(art_.bar, float(X0), y, PAL_GOLD, false);
        for (int x = X0; x < X_LAND; x += 10) spr(art_.bar, float(x), y, PAL_NAVE, false);
        if (!cued_[i]) {
            int at = std::max(ph.landAt - ph.travel[i], 1);
            float x = X0 + (X_CUE - X0) * std::min(1.f, float(tick_) / float(at));
            spr(art_.pip, x, y, hot(i) ? PAL_GOLD : PAL_VOICE[i], false);
        } else {
            float x = X0 + (X_LAND - X0) * progress(i);
            spr(art_.note, x, y, PAL_VOICE[i], false);
        }
    }
    spr(art_.bar, float(X_CUE), float(LANE_Y[1]), PAL_GOLD, false);
    spr(art_.bar, float(X_LAND), float(LANE_Y[1] - 16), PAL_GOLD, false);
}

void Game::people() {
    for (int i = 2; i >= 0; i--) {
        float bob = std::sin((anim_ + i * 19) * 0.07f) * 2.f;
        bool sing = cued_[i] || mode_ == Mode::Win || mode_ == Mode::Title;
        float foot = 196.f + bob + (sing ? -3.f : 0.f);
        spr(art_.singer[i], float(STALL_X[i]), foot, PAL_VOICE[i], true);
    }
}

void Game::scorePips() {
    for (int i = 0; i < GOAL; i++) {
        spr(art_.pip, 28.f + i * 12.f, 18.f, i < you_ ? PAL_GOLD : PAL_NAVE, false);
        spr(art_.pip, 208.f + i * 12.f, 18.f, i < them_ ? PAL_THEM : PAL_NAVE, false);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.clear();
    v.B.clear();
    backdrop();
    if (mode_ == Mode::Title) {
        spr(art_.wordChoir, 160, 48, PAL_HUD, false);
        spr(art_.wordSeven, 160, 86, PAL_GOLD, false);
    } else {
        board();
        scorePips();
    }
    people();

    if (mode_ == Mode::Title) {
        hudC(16, "CUE A B C ON THE BAR", PAL_HUD);
        hudC(18, "LAND TOGETHER FOR A POINT", PAL_GOLD);
        hudC(20, "FIRST CHOIR TO SEVEN", PAL_HUD);
        hud(2, 26, "START", PAL_HUD);
        hud(28, 26, S3_VERSION_STRING, PAL_HUD);
    } else {
        hud(1, 0, "YOU", PAL_GOLD);
        hud(22, 0, "THEM", PAL_THEM);
        const Phrase& ph = PHRASE[phrase_ % NPHRASE];
        char buf[40];
        std::snprintf(buf, sizeof buf, "%s", ph.name);
        hudC(3, buf, PAL_HUD);
        if (line_[0]) hudC(22, line_, yours_ ? PAL_GOLD : PAL_ALERT);
        if (mode_ == Mode::Win) hudC(24, "YOU LEAVE AT SEVEN", PAL_GOLD);
        if (mode_ == Mode::Lose) hudC(24, "MATCH OVER", PAL_ALERT);
        hudC(26, "A TREBLE   B ALTO   C BASS", PAL_HUD);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    gs::FMPatch voice = voicePatch();
    for (int i = 0; i < 4; i++) sys.apu.setPatch(i, voice);
    sys.apu.setPan(0, -0.5f);
    sys.apu.setPan(1, 0.f);
    sys.apu.setPan(2, 0.5f);
    sys.apu.setMaster(0.75f);
    sys.apu.setEcho(0.22f, 0.35f, 0.18f);
    if (bot_) beginMatch();
    else enterTitle();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    anim_++;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START)) beginMatch();
        else if (pad.pressed(gs::BTN_MODE)) sys.quit();
    } else if (mode_ == Mode::Phrase) {
        phraseFrame();
    } else if (mode_ == Mode::Hold) {
        hold_++;
        bool skip = !bot_ && pad.pressed(gs::BTN_START);
        if (hold_ > 36 || skip) {
            phrase_++;
            beginPhrase();
        }
    } else if (mode_ == Mode::Win || mode_ == Mode::Lose) {
        if (!bot_ && pad.pressed(gs::BTN_START)) enterTitle();
    }
    draw();
}

}  // namespace choirseven

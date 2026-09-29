#include "game/choirchime.h"

#include "../version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace choirchime {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr int kStallX[3] = {64, 160, 256};
constexpr int kLaneY[3] = {78, 96, 114};
constexpr int kX0 = 28;
constexpr int kXLand = 292;
constexpr int kFoot = 196;
constexpr float kPitch[3] = {392.00f, 329.63f, 261.63f};
constexpr int kPalVoice[3] = {PAL_TREBLE, PAL_ALTO, PAL_BASS};

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

gs::FMPatch voicePatch() {
    gs::FMPatch p;
    p.alg = 7;
    p.fb = 0.1f;
    p.op[0] = {1, 0.85f, 0.05f, 0.28f, 0.75f, 0.22f};
    p.op[1] = {2, 0.2f, 0.07f, 0.34f, 0.5f, 0.28f};
    p.op[2] = {3, 0.08f, 0.09f, 0.4f, 0.35f, 0.3f};
    p.op[3] = {4, 0.04f, 0.1f, 0.45f, 0.25f, 0.34f};
    p.vol = 0.13f;
    p.drive = 0.06f;
    p.tone = 1800;
    p.vibRate = 4.4f;
    p.vibDepth = 0.006f;
    p.vibDelay = 0.1f;
    p.echo = 0.38f;
    return p;
}

gs::FMPatch bellPatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.16f;
    p.op[0] = {1.f, 1.f, 0.004f, 0.5f, 0.18f, 0.45f};
    p.op[1] = {2.f, 0.24f, 0.005f, 0.32f, 0.f, 0.24f};
    p.op[2] = {3.01f, 0.1f, 0.004f, 0.18f, 0.f, 0.14f};
    p.op[3] = {0.5f, 0.12f, 0.008f, 0.55f, 0.06f, 0.3f};
    p.vol = 0.26f;
    p.echo = 0.3f;
    p.tone = 1600.f;
    return p;
}

}  // namespace

int Game::secAt(int frames) const {
    if (frames < 0) frames = 0;
    return kStartSec + frames / kFpc;
}

int Game::clockSec() const { return secAt(playFrames_); }

bool Game::onHourAt(int frames) const {
    int sec = secAt(frames);
    return sec >= kHourSec && sec < kHourSec + kGraceSec;
}

bool Game::onHour() const { return onHourAt(playFrames_); }

bool Game::pastHour() const { return clockSec() >= kHourSec + kGraceSec; }

void Game::split(int& h, int& m, int& s) const {
    int t = clockSec();
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

int Game::spread() const {
    int lo = arr_[0], hi = arr_[0];
    for (int i = 1; i < 3; i++) {
        lo = std::min(lo, arr_[i]);
        hi = std::max(hi, arr_[i]);
    }
    return hi - lo;
}

bool Game::possible() const {
    const int lim = kSlop * 2;
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
    if (n >= 2 && hi - lo > lim) return false;
    if (n == 0) return !pastHour();
    for (int i = 0; i < 3; i++) {
        if (cued_[i]) continue;
        if (playFrames_ + kTravel[i] > lo + lim) return false;
    }
    return true;
}

bool Game::arrived() const {
    for (int i = 0; i < 3; i++) {
        if (!cued_[i] || playFrames_ < arr_[i]) return false;
    }
    return true;
}

float Game::progress(int i) const {
    if (!cued_[i]) return 0.f;
    int tr = kTravel[i];
    if (tr <= 0) return 1.f;
    return std::clamp(float(playFrames_ - cueAt_[i]) / float(tr), 0.f, 1.f);
}

bool Game::hot(int i) const {
    if (mode_ != Mode::Phrase || cued_[i]) return false;
    int at = kLandFrame - kTravel[i];
    int d = playFrames_ - at;
    if (d < 0) d = -d;
    return d <= kSlop;
}

bool Game::audit() const {
    auto bad = [](const char* w) {
        std::fprintf(stderr, "s3choirchime %s\n", w);
        return false;
    };
    if (secAt(0) != kStartSec) return bad("clock does not open at 11:58:30");
    if (secAt(kLandFrame) != kHourSec) return bad("the land frame is not noon");
    if (onHourAt(kLandFrame - 1)) return bad("11:59 counted as the hour");
    if (!onHourAt(kLandFrame)) return bad("noon missed");
    if (!onHourAt(kLandFrame + kGraceSec * kFpc - 1)) return bad("grace ended early");
    if (onHourAt(kLandFrame + kGraceSec * kFpc)) return bad("grace ran long");
    int arr[3];
    for (int i = 0; i < 3; i++) arr[i] = (kLandFrame - kTravel[i]) + kTravel[i];
    int lo = arr[0], hi = arr[0];
    for (int i = 1; i < 3; i++) {
        lo = std::min(lo, arr[i]);
        hi = std::max(hi, arr[i]);
    }
    if (hi - lo != 0 || hi != kLandFrame) return bad("entries do not meet on the hour");
    if (kTravel[0] >= kTravel[1] || kTravel[1] >= kTravel[2]) return bad("voices share a travel");
    if (kSlop < 1 || kSlop * 2 >= kTravel[0]) return bad("slop swallows an entry");
    return true;
}

void Game::silenceVoices() {
    if (!sys_) return;
    for (int i = 0; i < 3; i++) sys_->apu.keyOff(i);
}

void Game::toTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    clockOn_ = false;
    phrases_ = 0;
    playFrames_ = 0;
    titleFrames_ = 0;
    for (int i = 0; i < 3; i++) cued_[i] = false;
    silenceVoices();
    reason_ = "";
    if (sys_) {
        sys_->apu.keyOn(2, 130.81f, 0.04f);
        sys_->setLight(48, 36, 90);
    }
}

void Game::beginPhrase() {
    for (int i = 0; i < 3; i++) cued_[i] = false;
    showT_ = 0;
    mode_ = Mode::Phrase;
    silenceVoices();
}

void Game::newGame() {
    won_ = false;
    over_ = false;
    phrases_ = 0;
    playFrames_ = 0;
    chimeFrames_ = 0;
    leaveT_ = 0;
    failFrames_ = 0;
    strikes_ = 0;
    lastSpread_ = 0;
    reason_ = "";
    bellAmp_ = 0.2f;
    clockOn_ = true;
    if (!rules_) {
        beginFail("RULES");
        return;
    }
    beginPhrase();
    if (sys_) sys_->setLight(70, 48, 90);
}

void Game::cueVoice(int i) {
    if (i < 0 || i > 2 || cued_[i] || mode_ != Mode::Phrase) return;
    cued_[i] = true;
    cueAt_[i] = playFrames_;
    arr_[i] = playFrames_ + kTravel[i];
    if (sys_) sys_->apu.keyOn(i, kPitch[i], 0.14f);
}

void Game::beginChime() {
    if (won_) return;
    phrases_++;
    won_ = true;
    reason_ = "CHIME";
    clockOn_ = false;
    mode_ = Mode::Chime;
    chimeFrames_ = 0;
    strikes_ = 0;
    bellAmp_ = 1.f;
    lastSpread_ = spread();
    if (!sys_) return;
    sys_->apu.setPatch(3, bellPatch());
    sys_->setLight(255, 196, 64);
    sys_->rumble(0.45f, 0.8f, 180);
    strikeBell();
    strikes_ = 1;
}

void Game::beginEarly() {
    phrases_++;
    lastSpread_ = spread();
    mode_ = Mode::Early;
    showT_ = 0;
    silenceVoices();
    blip(3, 196.f, 0.05f, 0.16f);
    if (sys_) sys_->setLight(120, 80, 40);
}

void Game::beginFail(const char* why) {
    reason_ = why;
    won_ = false;
    clockOn_ = false;
    mode_ = Mode::Fail;
    failFrames_ = 0;
    silenceVoices();
    if (sys_) {
        sys_->apu.noiseBurst(0.18f, 700.f, 0.12f);
        sys_->setLight(160, 24, 24);
        sys_->rumble(0.5f, 0.12f, 120);
    }
}

void Game::resolve() {
    if (!arrived()) return;
    int sp = spread();
    if (sp > kSlop * 2) {
        beginFail("SPREAD");
        return;
    }
    if (onHourAt(arr_[0]) && onHourAt(arr_[1]) && onHourAt(arr_[2])) {
        beginChime();
        return;
    }
    if (pastHour() || secAt(arr_[2]) >= kHourSec + kGraceSec) {
        beginFail("LATE");
        return;
    }
    beginEarly();
}

void Game::botCue() {
    if (pastHour()) {
        beginFail("LATE");
        return;
    }
    for (int i = 0; i < 3; i++) {
        if (!cued_[i] && playFrames_ == kLandFrame - kTravel[i]) cueVoice(i);
    }
}

void Game::blip(int ch, float freq, float vol, float hold) {
    if (!sys_) return;
    sys_->apu.tone(ch, freq, vol);
    toneT_ = hold;
}

void Game::strikeBell() {
    if (!sys_) return;
    sys_->apu.keyOn(3, 392.f, 0.32f);
    sys_->apu.tone(1, 784.f, 0.03f);
    toneT_ = 0.14f;
    bellAmp_ = 1.f;
}

void Game::tickAudio(float dt) {
    if (!sys_) return;
    if (toneT_ > 0.f) {
        toneT_ -= dt;
        if (toneT_ <= 0.f) {
            sys_->apu.tone(1, 0, 0);
            sys_->apu.tone(3, 0, 0);
            if (mode_ != Mode::Chime && mode_ != Mode::Leave) sys_->apu.keyOff(3);
        }
    }
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
    rules_ = audit();
    toTitle();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    anim_++;
    bool chiming = mode_ == Mode::Chime || mode_ == Mode::Leave || (mode_ == Mode::Over && won_);
    bellAmp_ = chiming ? 1.f : std::max(0.15f, bellAmp_ * 0.96f);

    const gs::Pad& p = sys.pad;
    bool start = p.pressed(gs::BTN_START);
    bool back = p.pressed(gs::BTN_MODE);
    if (bot_) {
        start = false;
        back = false;
    }

    if (mode_ == Mode::Pause) {
        if (start) mode_ = held_;
        else if (back) toTitle();
    } else if (mode_ == Mode::Title) {
        titleFrames_++;
        if (bot_ && titleFrames_ >= 30) newGame();
        else if (back) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        } else if (start) newGame();
    } else if (mode_ == Mode::Phrase) {
        if (!bot_ && start) {
            held_ = Mode::Phrase;
            mode_ = Mode::Pause;
        } else if (!bot_ && back) toTitle();
        else {
            if (clockOn_) playFrames_++;
            if (bot_) botCue();
            else {
                if (p.pressed(gs::BTN_A) || p.pressed(gs::BTN_Z)) cueVoice(0);
                if (p.pressed(gs::BTN_B) || p.pressed(gs::BTN_X)) cueVoice(1);
                if (p.pressed(gs::BTN_C) || p.pressed(gs::BTN_Y)) cueVoice(2);
            }
            if (mode_ == Mode::Phrase) {
                if (!possible()) beginFail(cued_[0] && cued_[1] && cued_[2] ? "SPREAD" : "MISSED");
                else if (arrived()) resolve();
                else if (pastHour()) beginFail("LATE");
            }
        }
    } else if (mode_ == Mode::Early) {
        if (clockOn_) playFrames_++;
        showT_++;
        if (pastHour()) beginFail("LATE");
        else if (showT_ > 28) beginPhrase();
    } else if (mode_ == Mode::Chime) {
        chimeFrames_++;
        if (chimeFrames_ > 0 && (chimeFrames_ % 8) == 0 && strikes_ < 12) {
            strikeBell();
            strikes_++;
        }
        if (strikes_ >= 12 && chimeFrames_ > 12 * 8 + 18) {
            mode_ = Mode::Leave;
            leaveT_ = 0;
            reason_ = "CHIME";
            silenceVoices();
        }
    } else if (mode_ == Mode::Leave) {
        leaveT_++;
        if (leaveT_ > 46) {
            mode_ = Mode::Over;
            won_ = true;
            over_ = true;
            if (sys_) sys_->apu.keyOff(3);
        }
    } else if (mode_ == Mode::Fail) {
        if (++failFrames_ > 70) {
            over_ = true;
        } else if (!bot_ && start) newGame();
    } else if (mode_ == Mode::Over) {
        if (!bot_ && back) toTitle();
        else if (!bot_ && start) newGame();
    }

    tickAudio(kDt);
    draw();
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::hudR(int row, const char* s, int pal) { hud(40 - int(std::strlen(s)), row, s, pal); }

void Game::spr(const gs::Mipped& m, float cx, float cy, int pal, bool feet, int fog) {
    if (!sys_ || m.h < 1 || m.w < 1) return;
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
    if (!sys_ || w <= 0 || h <= 0 || img.w == 0) return;
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
        v.lineBackdrop[y] = lerpC(gs::rgb4(1, 1, 3), gs::rgb4(4, 3, 6), t);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::board() {
    for (int i = 0; i < 3; i++) {
        ruleAt(art_.staff, kX0, kLaneY[i], kXLand - kX0, 2, PAL_GOLD);
        if (!cued_[i]) {
            int at = std::max(1, kLandFrame - kTravel[i]);
            float u = std::clamp(float(playFrames_) / float(at), 0.f, 1.f);
            float x = kX0 + (160.f - kX0) * u;
            spr(art_.diamond, x, float(kLaneY[i]), hot(i) ? PAL_FX : kPalVoice[i], false, 0);
        } else {
            float x = kX0 + (kXLand - kX0) * progress(i);
            spr(art_.note, x, float(kLaneY[i] - 6), kPalVoice[i], false, mode_ == Mode::Fail ? 8 : 0);
        }
        if (hot(i)) spr(art_.halo, float(kStallX[i]), 128.f, PAL_FX, false, 0);
    }
    spr(art_.fermata, float(kXLand - 6), float(kLaneY[0] - 16), PAL_GOLD, false, 0);
    ruleAt(art_.rule, kXLand - 2, kLaneY[0] - 8, 2, kLaneY[2] - kLaneY[0] + 12, PAL_GOLD);
}

void Game::people() {
    float slide = mode_ == Mode::Leave || (mode_ == Mode::Over && won_) ? float(leaveT_) * 6.f : 0.f;
    for (int i = 2; i >= 0; i--) {
        bool sing = false;
        if (mode_ == Mode::Title) sing = std::sin(anim_ * 0.08f + i) > 0.2f;
        else if (mode_ == Mode::Chime || mode_ == Mode::Early) sing = true;
        else sing = cued_[i];
        int bob = int(std::lround(std::sin((anim_ + i * 19) * 0.07f)));
        float x = float(kStallX[i]) + slide;
        if (x > 360.f) continue;
        spr(art_.singer[i][sing ? 1 : 0], x, float(kFoot + bob), kPalVoice[i], true, mode_ == Mode::Fail ? 8 : 0);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.clear();
    backdrop();

    float swing = std::sin(anim_ * (mode_ == Mode::Chime || mode_ == Mode::Leave ? 0.45f : 0.05f)) * (8.f + bellAmp_ * 10.f);
    spr(art_.bell, 160.f + swing, 36.f, PAL_GOLD, false, 0);

    gs::Sprite face;
    face.img = art_.face;
    face.w = art_.face.w;
    face.h = art_.face.h;
    face.x = 276;
    face.y = 8;
    face.pal = PAL_CLOCK;
    v.sprite(face);

    if (mode_ == Mode::Title) spr(art_.title, 160, 58, PAL_HUD, false, 0);
    if (mode_ != Mode::Title) board();
    people();

    char buf[48];
    std::snprintf(buf, sizeof buf, "%d:%02d:%02d", hour(), minute(), second());
    if (mode_ == Mode::Title) {
        hudC(12, "PLAY UNTIL THE HOUR", PAL_HUD);
        hudC(13, "HAS TO CHIME", PAL_GOLD);
        hudC(15, "THEN LEAVE", PAL_HUD);
        hudC(17, "CUE A B C TO LAND", PAL_GOLD);
        hudC(18, "TOGETHER ON TWELVE", PAL_HUD);
        hud(2, 26, "START", PAL_HUD);
        hudR(26, S3_VERSION_STRING, PAL_HUD);
        hudR(1, buf, PAL_GOLD);
    } else {
        hud(1, 0, "S3 CHOIRCHIME", PAL_HUD);
        hudR(0, buf, onHour() || mode_ == Mode::Chime || mode_ == Mode::Leave ? PAL_GOLD : PAL_HUD);
        std::snprintf(buf, sizeof buf, "PHRASE %d", std::max(phrases_, mode_ == Mode::Phrase ? 1 : phrases_));
        hud(1, 1, buf, PAL_GOLD);
    }
    if (mode_ == Mode::Pause) {
        hudC(16, "PAUSED", PAL_GOLD);
        hudC(18, "START RESUMES", PAL_HUD);
    }
    if (mode_ == Mode::Early) hudC(20, "EARLY", PAL_ALERT);
    if (mode_ == Mode::Chime) hudC(20, "THE HOUR CHIMES", PAL_GOLD);
    if (mode_ == Mode::Leave || (mode_ == Mode::Over && won_)) hudC(20, "LEAVE", PAL_GOLD);
    if (mode_ == Mode::Fail || (mode_ == Mode::Over && !won_)) {
        hudC(20, reason_[0] ? reason_ : "STOP", PAL_ALERT);
        if (!bot_) hudC(22, "START", PAL_HUD);
    }
    if (mode_ == Mode::Phrase || mode_ == Mode::Title) hudC(27, "A TREBLE   B ALTO   C BASS", PAL_HUD);
}

}  // namespace choirchime

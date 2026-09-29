#include "game/anvilchime.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace anvilchime {

static_assert(kFaceHi >= kFaceLo, "face");
static_assert(kFaceHi - kFaceLo < 8, "the anvil face is short");
static_assert(kFaceHi < kSweep, "face sits on the rail");

int Game::clockSec() const { return kStartSec + playFrames_ / kFpc; }

int Game::phase() const {
    int p = playFrames_ % kSweep;
    if (p < 0) p += kSweep;
    return p;
}

bool Game::onHour() const {
    int sec = clockSec();
    return sec >= kHourSec && sec < kHourSec + kGraceSec;
}

bool Game::pastHour() const { return clockSec() >= kHourSec + kGraceSec; }

bool Game::onFace() const {
    int p = phase();
    return p >= kFaceLo && p <= kFaceHi;
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
    blows_ = 0;
    playFrames_ = 0;
    hold_ = 0;
    strikes_ = 0;
    titleWait_ = 0;
    hammerX_ = railX(0);
    hammerY_ = kAnvilY - 46.f;
}

void Game::begin() {
    won_ = false;
    over_ = false;
    reason_ = "";
    face_ = "";
    blows_ = 0;
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
    sys_->apu.noiseBurst(0.12f, 160.f, 0.06f);
    sys_->rumble(0.25f, 0.45f, 90);
}

void Game::beginGap(const char* why) {
    reason_ = why;
    hold_ = 0;
    mode_ = Mode::Gap;
    sys_->apu.noiseBurst(0.18f, 80.f, 0.12f);
}

void Game::beginFail(const char* why) {
    if (!won_) reason_ = why;
    won_ = false;
    hold_ = 0;
    mode_ = Mode::Fail;
    sys_->apu.tone(0, 90.f, 0.06f);
}

void Game::strike() {
    blows_++;
    hammerX_ = railX(phase());
    const bool face = onFace();
    const bool hour = onHour();
    if (face && hour) {
        beginChime();
        return;
    }
    if (face) face_ = "SHORT";
    else if (phase() < kFaceLo) face_ = "HORN";
    else face_ = "WAIST";
    if (pastHour() || blows_ >= kBlows) beginFail(pastHour() ? "HOUR" : (face ? "EARLY" : face_));
    else if (face) beginGap("EARLY");
    else beginGap(face_);
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

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    uint16_t top = gs::rgb4(1, 1, 4);
    uint16_t mid = gs::rgb4(3, 3, 6);
    uint16_t bot = gs::rgb4(2, 2, 2);
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
        v.lineBackdrop[y] = u < 0.55f ? mix(top, mid, u / 0.55f) : mix(mid, bot, (u - 0.55f) / 0.45f);
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

    const float tx = 78.f;
    float swing = 0.f;
    bool chiming = mode_ == Mode::Chime || (mode_ == Mode::Over && won_);
    if (chiming) swing = std::sin(hold_ * 0.45f) * 7.f;
    spr(art_.tower, tx, 112.f, PAL_WOOD);
    spr(art_.bell, tx + swing, 36.f, PAL_BELL);

    const float ax = 196.f;
    spr(art_.anvil, ax, kAnvilY, PAL_IRON);
    float faceCx = (railX(kFaceLo) + railX(kFaceHi)) * 0.5f;
    spr(art_.face, faceCx, kAnvilY - 16.f, onFace() ? PAL_FACE : PAL_GOLD);

    if (mode_ == Mode::Wait || mode_ == Mode::Title) {
        hammerX_ = railX(mode_ == Mode::Title ? (titleWait_ % kSweep) : phase());
        hammerY_ = kAnvilY - 48.f;
    } else if (mode_ == Mode::Chime) {
        hammerY_ = kAnvilY - 28.f;
    } else if (mode_ == Mode::Gap || mode_ == Mode::Fail) {
        hammerY_ = kAnvilY - 34.f;
    }
    spr(art_.hammer, hammerX_, hammerY_, PAL_IRON);

    spr(art_.clock, 36.f, 40.f, PAL_NIGHT);
    int h, m, s;
    faceTime(h, m, s);
    if (mode_ == Mode::Title) {
        h = 11;
        m = 59;
        s = 32;
    }
    float hourAng = ((float(h % 12) + float(m) / 60.f) / 12.f) * 2.f * kPi;
    float minAng = (float(m) / 60.f) * 2.f * kPi;
    float secAng = (float(s) / 60.f) * 2.f * kPi;
    hand(36.f, 40.f, hourAng, 8.f, PAL_GOLD);
    hand(36.f, 40.f, minAng, 11.f, PAL_HUD);
    hand(36.f, 40.f, secAng, 12.f, PAL_BAD);

    for (int i = 0; i < kBlows; i++) {
        int pal = i < blows_ ? PAL_BAD : PAL_DIM;
        if (won_ && i == blows_ - 1) pal = PAL_GOLD;
        spr(art_.lamp, 250.f + i * 16.f, 22.f, pal);
    }

    char buf[80];
    int f = int(sys_->frame);
    if (mode_ == Mode::Title) {
        hudC(2, "S3 ANVILCHIME", PAL_GOLD);
        hudC(18, "A SHORT ANVIL", PAL_HUD);
        hudC(20, "THE HOUR HAS TO CHIME", PAL_GOLD);
        hudC(22, "EARLY IRON IS PULLED", PAL_DIM);
        if ((f & 16) == 0) hudC(26, "PRESS START", PAL_GOLD);
        const char* ver = S3_VERSION_STRING;
        hud(40 - int(std::strlen(ver)), 0, ver, PAL_DIM);
    } else if (mode_ == Mode::Over && won_) {
        hudC(2, "THE HOUR CHIMES", PAL_GOLD);
        std::snprintf(buf, sizeof buf, "%d:%02d:%02d  SHORT", hour(), minute(), second());
        hudC(20, buf, PAL_GOLD);
        std::snprintf(buf, sizeof buf, "BLOW %d", blows_);
        hudC(22, buf, PAL_HUD);
    } else if (mode_ == Mode::Fail || (mode_ == Mode::Over && !won_)) {
        hudC(2, "THE HOUR IS GONE", PAL_BAD);
        hudC(22, reason_ && reason_[0] ? reason_ : "HOUR", PAL_HUD);
        if ((f & 16) == 0) hudC(26, "START", PAL_GOLD);
    } else if (mode_ == Mode::Chime) {
        hudC(2, "THE HOUR CHIMES", PAL_GOLD);
        hudC(24, "SHORT FACE", PAL_GOLD);
    } else {
        std::snprintf(buf, sizeof buf, "%d:%02d:%02d", hour(), minute(), second());
        hud(1, 1, buf, onHour() ? PAL_GOLD : PAL_HUD);
        std::snprintf(buf, sizeof buf, "BLOW %d/%d", blows_ + (mode_ == Mode::Wait ? 1 : 0), kBlows);
        hud(28, 1, buf, PAL_DIM);
        int mark = phase() * 20 / kSweep;
        if (mark > 19) mark = 19;
        char bar[24];
        for (int i = 0; i < 20; i++) {
            int p = i * kSweep / 20;
            bool sweet = p >= kFaceLo && p <= kFaceHi;
            bar[i] = (i == mark) ? '|' : (sweet ? '=' : '-');
        }
        bar[20] = 0;
        hudC(23, bar, onFace() ? PAL_GOLD : PAL_DIM);
        if (mode_ == Mode::Gap && std::strcmp(reason_, "EARLY") == 0) hudC(25, "TOO SOON", PAL_BAD);
        else if (mode_ == Mode::Gap) hudC(25, "OFF THE SHORT FACE", PAL_BAD);
        else if (onHour()) hudC(25, "THE HOUR IS HERE", PAL_GOLD);
        else hudC(25, "C ON THE SHORT FACE", PAL_DIM);
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

    if (mode_ == Mode::Title) {
        bool go = pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C);
        if (bot_) go = ++titleWait_ > 10;
        if (go) begin();
    } else if (mode_ == Mode::Wait) {
        if (pastHour()) beginFail("HOUR");
        else {
            bool tap = false;
            if (bot_) tap = onHour() && onFace();
            else tap = pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A);
            if (tap) strike();
        }
    } else if (mode_ == Mode::Gap) {
        if (pastHour()) beginFail("HOUR");
        else if (++hold_ > 16) {
            hold_ = 0;
            mode_ = Mode::Wait;
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
        if (++hold_ > 28) {
            over_ = true;
            mode_ = Mode::Over;
        }
    } else if (mode_ == Mode::Over) {
        if (!bot_ && !won_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C))) toTitle();
    }

    draw();
}

}  // namespace anvilchime

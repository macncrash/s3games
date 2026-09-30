#include "game/cueseven.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace cueseven {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kRow = 132.f;
constexpr float kCueHome = 86.f;
constexpr float kObjHome = 176.f;
constexpr float kPocket = 268.f;
constexpr float kSweet0 = 0.44f;
constexpr float kSweet1 = 0.58f;
constexpr float kMeterRate = 0.85f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

// House misses every third visit, so a clean run reaches seven first.
bool housePots(int shot) { return (shot % 3) != 2; }

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(0, 2, 1));
    sys.apu.setMaster(0.65f);
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    you_ = 0;
    them_ = 0;
    clock_ = 0;
    meter_ = 0.15f;
    meterDir_ = 1.f;
    cueX_ = kCueHome;
    whiteX_ = kCueHome;
    objX_ = kObjHome;
    say_ = "FIRST TO SEVEN";
}

void Game::beginMatch() {
    you_ = 0;
    them_ = 0;
    visits_ = 0;
    themShots_ = 0;
    yours_ = true;
    over_ = false;
    won_ = false;
    beginVisit();
}

void Game::beginVisit() {
    mode_ = Mode::Aim;
    willPot_ = false;
    potted_ = false;
    meter_ = yours_ ? 0.08f : 0.2f;
    meterDir_ = 1.f;
    cueX_ = kCueHome;
    whiteX_ = kCueHome;
    objX_ = kObjHome;
    rollT_ = 0;
    say_ = yours_ ? "YOUR CUE" : "THEIR CUE";
    clock_ = 0;
}

void Game::stroke() {
    willPot_ = meter_ >= kSweet0 && meter_ <= kSweet1;
    if (!yours_) willPot_ = housePots(themShots_);
    mode_ = Mode::Roll;
    rollT_ = 0;
    potted_ = false;
    if (sys_) sys_->apu.tone(0, willPot_ ? 220.f : 90.f, 0.06f);
}

void Game::stepRoll() {
    rollT_ += kDt;
    float u = clampf(rollT_ / 0.55f, 0.f, 1.f);
    u = u * u * (3.f - 2.f * u);
    whiteX_ = kCueHome + (kObjHome - 18.f - kCueHome) * u;
    cueX_ = kCueHome - 10.f;
    if (willPot_) {
        objX_ = kObjHome + (kPocket - kObjHome) * u;
        if (u > 0.92f) potted_ = true;
    } else {
        objX_ = kObjHome + 22.f * std::sin(u * 3.14159265f);
    }
    if (rollT_ > 0.7f) settle();
}

void Game::settle() {
    if (willPot_) {
        if (yours_) you_++;
        else them_++;
        say_ = "POT";
        if (sys_) sys_->apu.tone(1, 440.f, 0.07f);
    } else {
        say_ = "MISS";
        if (sys_) sys_->apu.tone(1, 70.f, 0.05f);
    }
    if (!yours_) themShots_++;
    visits_++;
    mode_ = Mode::Call;
    callT_ = 0.45f;
    checkLine();
}

void Game::checkLine() {
    if (you_ >= kLine && them_ < kLine) {
        won_ = true;
        over_ = true;
        mode_ = Mode::Win;
        say_ = "FIRST TO SEVEN";
        if (sys_) {
            sys_->apu.tone(0, 523.f, 0.08f);
            sys_->apu.tone(1, 659.f, 0.06f);
            sys_->apu.tone(2, 784.f, 0.05f);
        }
    } else if (them_ >= kLine && you_ < kLine) {
        won_ = false;
        over_ = true;
        mode_ = Mode::Lose;
        say_ = "THEY REACHED SEVEN";
    }
}

Game::Input Game::readPad(const gs::Pad& pad) const {
    Input in;
    in.action = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_C);
    in.start = pad.pressed(gs::BTN_START);
    return in;
}

Game::Input Game::botInput() const {
    Input in;
    if (mode_ == Mode::Title && clock_ > 0.4f) in.start = true;
    if (mode_ == Mode::Aim && yours_ && meter_ >= kSweet0 && meter_ <= kSweet1 && clock_ > 0.15f) in.action = true;
    return in;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    clock_ += kDt;

    const Input in = bot_ ? botInput() : readPad(sys.pad);

    if (mode_ == Mode::Title) {
        meter_ += meterDir_ * kMeterRate * kDt;
        if (meter_ >= 1.f) {
            meter_ = 1.f;
            meterDir_ = -1.f;
        } else if (meter_ <= 0.f) {
            meter_ = 0.f;
            meterDir_ = 1.f;
        }
        cueX_ = kCueHome - meter_ * 16.f;
        whiteX_ = kCueHome;
        objX_ = kObjHome;
        if (in.start || in.action) beginMatch();
    } else if (mode_ == Mode::Win || mode_ == Mode::Lose) {
        if (!bot_ && (in.start || in.action)) beginMatch();
    } else if (mode_ == Mode::Call) {
        callT_ -= kDt;
        if (callT_ <= 0.f && !over_) {
            yours_ = !yours_;
            beginVisit();
        }
    } else if (mode_ == Mode::Aim) {
        if (!yours_) {
            // The house strokes on its own, on a fixed beat.
            meter_ = housePots(themShots_) ? 0.50f : 0.12f;
            if (clock_ > 0.35f) stroke();
        } else {
            meter_ += meterDir_ * kMeterRate * kDt;
            if (meter_ >= 1.f) {
                meter_ = 1.f;
                meterDir_ = -1.f;
            } else if (meter_ <= 0.f) {
                meter_ = 0.f;
                meterDir_ = 1.f;
            }
            cueX_ = kCueHome - meter_ * 22.f;
            if (in.action || in.start) stroke();
        }
    } else if (mode_ == Mode::Roll) {
        stepRoll();
    }

    draw();
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal) {
    if (!sys_ || h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(std::max(1, m.h));
    gs::Sprite s;
    s.w = int16_t(std::max(1, std::min(2000, (int)std::lround(w))));
    s.h = int16_t(std::max(1, std::min(2000, (int)std::lround(h))));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::blob(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool shadow) {
    if (!sys_ || w < 1.f || h < 1.f || img.w == 0) return;
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = img;
    s.pal = uint8_t(pal);
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!sys_ || !s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = (unsigned char)s[i];
        if (c >= 'a' && c <= 'z') c = (unsigned char)(c - 32);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        int tile = art_.font[c - 32];
        if (!tile) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(tile, pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - (s ? (int)std::strlen(s) / 2 : 0), row, s, pal); }

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        int band = (y / 14) & 1;
        int g = mode_ == Mode::Win && y < 36 ? 7 : 2 + band;
        v.lineBackdrop[y] = gs::rgb4(1, g, 1);
    }

    blob(art_.blot, 160.f, kRow, 268.f, 86.f, PAL_RAIL);
    blob(art_.blot, 160.f, kRow, 246.f, 66.f, PAL_FELT);
    blob(art_.blot, 48.f, kRow, 20.f, 22.f, PAL_POCKET);
    blob(art_.blot, 272.f, kRow, 22.f, 24.f, PAL_POCKET);
    blob(art_.blot, 160.f, 104.f, 12.f, 8.f, PAL_POCKET);
    blob(art_.blot, 160.f, 160.f, 12.f, 8.f, PAL_POCKET);
    for (int i = 0; i < 5; i++) blob(art_.blot, 88.f + i * 28.f, 108.f, 3.f, 3.f, PAL_RAIL);

    spr(art_.cue, cueX_ - 34.f, kRow, 8.f, PAL_CUE);

    if (!potted_) {
        blob(art_.shadow, objX_, kRow + 10.f, 18.f, 6.f, PAL_SHADOW, true);
        spr(art_.ball, objX_, kRow, 18.f, PAL_OBJECT);
    }
    blob(art_.shadow, whiteX_, kRow + 10.f, 18.f, 6.f, PAL_SHADOW, true);
    spr(art_.ball, whiteX_, kRow, 18.f, PAL_WHITE);

    if (mode_ == Mode::Aim || mode_ == Mode::Title) {
        blob(art_.blot, 160.f, 198.f, 180.f, 10.f, PAL_METER);
        float span = 176.f;
        float x0 = 160.f - span * 0.5f;
        float sw = span * (kSweet1 - kSweet0);
        float sx = x0 + span * kSweet0 + sw * 0.5f;
        blob(art_.blot, sx, 198.f, sw, 10.f, PAL_GOOD);
        float nx = x0 + span * clampf(meter_, 0.f, 1.f);
        blob(art_.blot, nx, 198.f, 3.f, 16.f, PAL_WORD);
    }

    char buf[64];
    hudC(1, "S3 CUE SEVEN", PAL_WORD);
    if (mode_ == Mode::Title) {
        hudC(3, "PLAY CUE UNTIL FIRST TO SEVEN", PAL_INK);
        hudC(5, "POT THE CORNER  LEAVE AT SEVEN", PAL_GOOD);
        hudC(24, "A STROKE    START", PAL_INK);
    } else if (mode_ == Mode::Win) {
        std::snprintf(buf, sizeof buf, "YOU %d   THEM %d", you_, them_);
        hudC(3, buf, PAL_GOOD);
        hudC(5, "FIRST TO SEVEN", PAL_WORD);
    } else if (mode_ == Mode::Lose) {
        std::snprintf(buf, sizeof buf, "YOU %d   THEM %d", you_, them_);
        hudC(3, buf, PAL_BAD);
        hudC(5, "THEY GOT THERE FIRST", PAL_BAD);
    } else {
        std::snprintf(buf, sizeof buf, "YOU %d   THEM %d   LINE %d", you_, them_, kLine);
        hudC(3, buf, yours_ ? PAL_YOU : PAL_OBJECT);
        hudC(5, say_, willPot_ && mode_ != Mode::Aim ? PAL_GOOD : PAL_INK);
        if (mode_ == Mode::Aim && yours_) hudC(24, "A  STROKE IN THE GREEN", PAL_INK);
        else if (mode_ == Mode::Aim) hudC(24, "HOUSE AT THE TABLE", PAL_OBJECT);
    }

    for (int i = 0; i < kLine; i++) {
        hud(12 + i * 2, 22, i < you_ ? "Y" : ".", i < you_ ? PAL_YOU : PAL_INK);
        hud(12 + i * 2, 23, i < them_ ? "T" : ".", i < them_ ? PAL_OBJECT : PAL_INK);
    }
}

}  // namespace cueseven

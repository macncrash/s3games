#include "game/scorechime.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace scorechime {
namespace {
constexpr int kWindowLo = 12;
constexpr int kWindowHi = 18;
constexpr int kMissAt = 28;
constexpr int kTap = (kWindowLo + kWindowHi) / 2;
}  // namespace

int Game::clockSec() const { return kStartSec + playFrames_ / 60; }

int Game::hour() const {
    int h = (clockSec() / 3600) % 12;
    return h == 0 ? 12 : h;
}

int Game::minute() const { return (clockSec() / 60) % 60; }

int Game::second() const { return clockSec() % 60; }

bool Game::onHour() const {
    const int s = clockSec();
    return s >= kHourSec && s < kHourSec + kGraceSec;
}

bool Game::pastHour() const { return clockSec() >= kHourSec + kGraceSec; }

bool Game::ready() const { return !spoiled_ && misses_ == 0 && marks_ == kMarks; }

void Game::note(float freq) { sys_->apu.keyOn(0, freq, 0.28f); }

void Game::begin() {
    marks_ = misses_ = wind_ = anim_ = playFrames_ = 0;
    chimeAge_ = -1;
    over_ = won_ = spoiled_ = false;
    why_ = "";
    mode_ = Mode::Roll;
    sys_->apu.silence();
}

void Game::mark(bool hit) {
    if (hit) {
        note(330.f);
        sys_->apu.noiseBurst(0.12f, 240.f, 0.05f);
    } else {
        spoiled_ = true;
        misses_++;
        why_ = "the mark missed the line";
        sys_->apu.noiseBurst(0.28f, 70.f, 0.2f);
    }
    marks_++;
    anim_ = 0;
    mode_ = Mode::Ink;
}

void Game::fail(const char* why) {
    won_ = false;
    over_ = true;
    why_ = why;
    mode_ = Mode::Over;
    sys_->apu.noiseBurst(0.3f, 70.f, 0.22f);
}

void Game::leave() {
    over_ = true;
    mode_ = Mode::Over;
    if (ready() && onHour()) {
        won_ = true;
        why_ = "CHIME";
        sys_->apu.keyOn(0, 392.f, 0.32f);
        sys_->apu.keyOn(1, 494.f, 0.24f);
        sys_->apu.keyOn(2, 587.f, 0.18f);
    } else {
        won_ = false;
        if (spoiled_ || misses_ > 0) why_ = "the mark missed the line";
        else if (marks_ < kMarks) why_ = "the score is not full";
        else if (pastHour()) why_ = "the chime went unheard";
        else why_ = "the hour has not chimed";
        sys_->apu.noiseBurst(0.3f, 80.f, 0.22f);
    }
}

void Game::spr(const gs::Image& img, float cx, float cy, int pal) {
    if (img.w == 0) return;
    gs::Sprite s;
    s.img = img;
    s.w = img.w;
    s.h = img.h;
    s.x = int16_t(std::lround(cx - img.w * 0.5f));
    s.y = int16_t(std::lround(cy - img.h * 0.5f));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    uint16_t top = gs::rgb4(1, 3, 6);
    uint16_t mid = gs::rgb4(2, 7, 3);
    uint16_t bot = gs::rgb4(1, 4, 2);
    if (onHour() && mode_ != Mode::Title) mid = gs::rgb4(10, 9, 3);
    if (mode_ == Mode::Over && won_) mid = gs::rgb4(12, 11, 4);
    if (mode_ == Mode::Over && !won_) mid = gs::rgb4(6, 2, 2);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H - 1);
        auto mix = [](uint16_t a, uint16_t b, float t) {
            int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
            int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
            auto L = [&](int p, int q) { return int(p + (q - p) * t + 0.5f); };
            return gs::rgb4(L(ar, br), L(ag, bg), L(ab, bb));
        };
        v.lineBackdrop[y] = u < 0.45f ? mix(top, mid, u / 0.45f) : mix(mid, bot, (u - 0.45f) / 0.55f);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    sky();

    const float lineX = 196.f;
    const float ground = 168.f;
    int shown = marks_;
    if (mode_ == Mode::Ink && anim_ < 6) shown = marks_ - 1;
    if (shown < 0) shown = 0;
    if (mode_ == Mode::Title) shown = int(sys_->frame / 28) % (kMarks + 1);

    float travel = 0.2f;
    if (mode_ == Mode::Roll) {
        if (wind_ < kWindowLo) travel = wind_ / float(kWindowLo);
        else if (wind_ <= kWindowHi) travel = 1.f;
        else travel = 1.f - 0.35f * ((wind_ - kWindowHi) / float(kMissAt - kWindowHi));
    } else if (mode_ == Mode::Ink || mode_ == Mode::Hold) {
        travel = 1.f;
    } else if (mode_ == Mode::Title) {
        travel = 0.45f + 0.1f * std::sin(sys_->frame * 0.06f);
    }

    spr(art_.board, 250.f, 108.f, PAL_BOARD);
    spr(art_.line, lineX, ground - 20.f, PAL_PITCH);

    float bellX = 42.f + (onHour() ? std::sin(sys_->frame * 0.5f) * 5.f : 0.f);
    spr(art_.bell, bellX, 48.f, PAL_BELL);

    for (int i = 0; i < shown && i < kMarks; i++) {
        float tx = 214.f + (i % 2) * 28.f;
        float ty = 78.f + (i / 2) * 30.f;
        spr(art_.tick, tx, ty, PAL_CHALK);
    }

    const bool hideBall = mode_ == Mode::Hold || mode_ == Mode::Over || (mode_ == Mode::Ink && anim_ > 8);
    if (!hideBall) {
        float bx = 70.f + travel * (lineX - 78.f);
        if (mode_ == Mode::Ink) bx = lineX - 2.f;
        float by = ground - 10.f - (mode_ == Mode::Roll && travel > 0.6f ? 6.f : 0.f);
        spr(art_.ball, bx, by, PAL_BALL);
    }

    char buf[72];
    int f = int(sys_->frame);
    if (mode_ == Mode::Title) {
        hudC(1, "S3 SCORE CHIME", PAL_BELL);
        hudC(20, "A SHORT SCORE", PAL_HUD);
        hudC(21, "THE HOUR HAS TO CHIME", PAL_BELL);
        hudC(22, "FOUR MARKS THEN WAIT FOR TWELVE", PAL_HUD);
        hudC(23, "LEAVE EARLY AND THE BOARD STAYS OPEN", PAL_DIM);
        if ((f & 16) == 0) hudC(26, "PRESS START", PAL_BELL);
        const char* ver = S3_VERSION_STRING;
        hud(40 - int(std::strlen(ver)), 0, ver, PAL_DIM);
    } else if (mode_ == Mode::Over && won_) {
        hudC(1, "THE HOUR CHIMED", PAL_BELL);
        std::snprintf(buf, sizeof buf, "%d:%02d:%02d", hour(), minute(), second());
        hudC(21, buf, PAL_BELL);
        hudC(22, "THE SHORT SCORE IS DONE", PAL_HUD);
        std::snprintf(buf, sizeof buf, "MARKS %d", marks_);
        hudC(23, buf, PAL_HUD);
    } else if (mode_ == Mode::Over) {
        hudC(1, "STILL ON THE BOARD", PAL_BAD);
        std::snprintf(buf, sizeof buf, "%d:%02d:%02d", hour(), minute(), second());
        hudC(21, buf, PAL_BAD);
        hudC(22, why_, PAL_HUD);
        if ((f & 16) == 0) hudC(26, "START", PAL_BELL);
    } else {
        std::snprintf(buf, sizeof buf, "%d:%02d:%02d", hour(), minute(), second());
        hudC(1, buf, onHour() ? PAL_BELL : PAL_HUD);
        int n = marks_;
        if (mode_ == Mode::Ink && anim_ < 6 && n > 0) n--;
        std::snprintf(buf, sizeof buf, "MARK %d/%d", n, kMarks);
        if (mode_ == Mode::Hold) std::snprintf(buf, sizeof buf, "MARK %d/%d", kMarks, kMarks);
        hudC(20, buf, PAL_CHALK);
        if (mode_ == Mode::Pause) hudC(22, "PAUSED", PAL_BELL);
        else if (onHour() && ready()) hudC(22, "THE HOUR IS CHIMING  LEAVE", PAL_BELL);
        else if (mode_ == Mode::Hold) hudC(22, "WAIT  THE HOUR HAS TO CHIME", PAL_HUD);
        else if (wind_ >= kWindowLo && wind_ <= kWindowHi && mode_ == Mode::Roll)
            hudC(22, "THE BALL IS ON THE LINE", PAL_BALL);
        else hudC(22, "CHALK THE MARK ON THE LINE", PAL_HUD);
        hudC(25, "C MARK   A LEAVE ON THE CHIME", PAL_DIM);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.85f);
    mode_ = Mode::Title;
    if (bot_) begin();
    draw();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;

    if (mode_ != Mode::Title && mode_ != Mode::Pause && mode_ != Mode::Over) playFrames_++;

    if (chimeAge_ < 0 && onHour() && mode_ != Mode::Title && mode_ != Mode::Over) {
        chimeAge_ = 0;
        sys_->apu.keyOn(1, 523.f, 0.34f);
        sys_->apu.keyOn(2, 784.f, 0.16f);
    } else if (chimeAge_ >= 0 && chimeAge_ < 40) {
        chimeAge_++;
        if (chimeAge_ == 12) sys_->apu.keyOn(1, 659.f, 0.28f);
        if (chimeAge_ == 24) sys_->apu.keyOn(1, 784.f, 0.24f);
    }

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C)) begin();
    } else if (mode_ == Mode::Roll) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            held_ = Mode::Roll;
            mode_ = Mode::Pause;
        } else if (marks_ < kMarks) {
            const bool tap = bot_ ? (wind_ == kTap) : pad.pressed(gs::BTN_C);
            if (tap) mark(wind_ >= kWindowLo && wind_ <= kWindowHi);
            else if (++wind_ >= kMissAt) mark(false);
        }
    } else if (mode_ == Mode::Ink) {
        anim_++;
        if (anim_ >= 12) {
            wind_ = anim_ = 0;
            mode_ = Mode::Gap;
        }
    } else if (mode_ == Mode::Gap) {
        anim_++;
        if (anim_ >= 10) {
            anim_ = 0;
            mode_ = (marks_ >= kMarks) ? Mode::Hold : Mode::Roll;
        }
    } else if (mode_ == Mode::Hold) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            held_ = Mode::Hold;
            mode_ = Mode::Pause;
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = held_;
    } else if (mode_ == Mode::Over) {
        if (!bot_ && !won_ && pad.pressed(gs::BTN_START)) begin();
    }

    if (mode_ == Mode::Roll || mode_ == Mode::Ink || mode_ == Mode::Gap || mode_ == Mode::Hold) {
        if (pastHour()) fail("the chime went unheard");
        else if (bot_ && mode_ == Mode::Hold && onHour() && ready()) leave();
        else if (!bot_ && pad.pressed(gs::BTN_A)) leave();
    }

    draw();
}

}  // namespace scorechime

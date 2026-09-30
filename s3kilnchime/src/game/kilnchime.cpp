#include "game/kilnchime.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace kilnchime {
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

bool Game::ready() const { return !spoiled_ && misses_ == 0 && fires_ == kFires; }

void Game::note(float freq) { sys_->apu.keyOn(0, freq, 0.28f); }

void Game::begin() {
    fires_ = misses_ = wind_ = anim_ = sparkN_ = playFrames_ = 0;
    chimeAge_ = -1;
    over_ = won_ = spoiled_ = false;
    why_ = "";
    mode_ = Mode::Slide;
    sys_->apu.silence();
}

void Game::fire(bool hit) {
    if (hit) {
        note(220.f);
        sys_->apu.noiseBurst(0.16f, 200.f, 0.06f);
    } else {
        spoiled_ = true;
        misses_++;
        why_ = "the pot missed the mouth";
        sys_->apu.noiseBurst(0.28f, 70.f, 0.2f);
    }
    fires_++;
    sparkN_ = hit ? 4 : 0;
    anim_ = 0;
    mode_ = Mode::Glow;
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
        if (spoiled_ || misses_ > 0) why_ = "the pot missed the mouth";
        else if (!onHour() && !pastHour()) why_ = "the hour has not chimed";
        else if (fires_ < kFires) why_ = "the kiln is not full";
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
    uint16_t top = gs::rgb4(2, 1, 3);
    uint16_t mid = gs::rgb4(7, 3, 2);
    uint16_t bot = gs::rgb4(2, 2, 2);
    if (onHour() && mode_ != Mode::Title) mid = gs::rgb4(12, 8, 2);
    if (mode_ == Mode::Over && won_) mid = gs::rgb4(13, 9, 2);
    if (mode_ == Mode::Over && !won_) mid = gs::rgb4(5, 1, 1);
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
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    sky();

    const float mouthX = 220.f;
    const float shelfY = 170.f;
    int show = fires_ < kFires ? fires_ : kFires - 1;
    if (mode_ == Mode::Title) show = int(sys_->frame / 30) % kFires;

    float travel = 0.35f;
    if (mode_ == Mode::Slide) {
        if (wind_ < kWindowLo) travel = wind_ / float(kWindowLo);
        else if (wind_ <= kWindowHi) travel = 1.f;
        else travel = 1.f - 0.3f * ((wind_ - kWindowHi) / float(kMissAt - kWindowHi));
    } else if (mode_ == Mode::Glow || mode_ == Mode::Hold) {
        travel = 1.f;
    } else if (mode_ == Mode::Title) {
        travel = 0.5f + 0.12f * std::sin(sys_->frame * 0.07f);
    }

    spr(art_.shelf, 146.f, shelfY, PAL_ASH);
    spr(art_.kiln, mouthX, 124.f, PAL_BRICK);

    float bellX = 46.f + (onHour() ? std::sin(sys_->frame * 0.45f) * 6.f : 0.f);
    spr(art_.bell, bellX, 52.f, PAL_BELL);

    const bool hidePot = (mode_ == Mode::Glow && anim_ > 6) || mode_ == Mode::Hold || mode_ == Mode::Over;
    if (!hidePot) {
        float px = 52.f + travel * 120.f;
        if (mode_ == Mode::Glow) px = mouthX - 10.f;
        spr(art_.pot, px, shelfY - 26.f, PAL_CLAY);
    }
    if (sparkN_ > 0 && (mode_ == Mode::Glow || mode_ == Mode::Title || onHour())) {
        int n = mode_ == Mode::Title ? 3 : sparkN_;
        for (int i = 0; i < n; i++) {
            float fx = mouthX - 16.f + (i % 3) * 11.f;
            float fy = 112.f - (int(sys_->frame) % 7) * 2.f - i * 4.f;
            spr(art_.flame, fx, fy, PAL_FIRE);
        }
    } else if (mode_ == Mode::Slide && wind_ >= kWindowLo && wind_ <= kWindowHi) {
        spr(art_.flame, mouthX - 8.f, 114.f, PAL_FIRE);
    }

    char buf[72];
    int f = int(sys_->frame);
    if (mode_ == Mode::Title) {
        hudC(1, "S3 KILN CHIME", PAL_BELL);
        hudC(21, "A SHORT KILN", PAL_HUD);
        hudC(22, "THE HOUR HAS TO CHIME", PAL_BELL);
        hudC(23, "FOUR POTS THEN WAIT FOR TWELVE", PAL_HUD);
        hudC(24, "LEAVE EARLY AND THE KILN STAYS COLD", PAL_DIM);
        if ((f & 16) == 0) hudC(27, "PRESS START", PAL_BELL);
        const char* ver = S3_VERSION_STRING;
        hud(40 - int(std::strlen(ver)), 0, ver, PAL_DIM);
    } else if (mode_ == Mode::Over && won_) {
        hudC(1, "THE HOUR CHIMED", PAL_BELL);
        std::snprintf(buf, sizeof buf, "%d:%02d:%02d", hour(), minute(), second());
        hudC(22, buf, PAL_BELL);
        hudC(23, "THE SHORT KILN IS DONE", PAL_HUD);
        std::snprintf(buf, sizeof buf, "FIRES %d", fires_);
        hudC(24, buf, PAL_HUD);
    } else if (mode_ == Mode::Over) {
        hudC(1, "STILL AT THE KILN", PAL_BAD);
        std::snprintf(buf, sizeof buf, "%d:%02d:%02d", hour(), minute(), second());
        hudC(22, buf, PAL_BAD);
        hudC(23, why_, PAL_HUD);
        if ((f & 16) == 0) hudC(27, "START", PAL_BELL);
    } else {
        std::snprintf(buf, sizeof buf, "%d:%02d:%02d", hour(), minute(), second());
        hudC(1, buf, onHour() ? PAL_BELL : PAL_HUD);
        std::snprintf(buf, sizeof buf, "FIRE %d/%d", show + (mode_ == Mode::Hold ? 1 : 1), kFires);
        if (mode_ == Mode::Hold) std::snprintf(buf, sizeof buf, "FIRE %d/%d", kFires, kFires);
        hudC(21, buf, PAL_CLAY);
        if (mode_ == Mode::Pause) hudC(23, "PAUSED", PAL_BELL);
        else if (onHour() && ready()) hudC(23, "THE HOUR IS CHIMING  LEAVE", PAL_BELL);
        else if (mode_ == Mode::Hold) hudC(23, "WAIT  THE HOUR HAS TO CHIME", PAL_HUD);
        else if (wind_ >= kWindowLo && wind_ <= kWindowHi && mode_ == Mode::Slide)
            hudC(23, "THE MOUTH IS OPEN", PAL_FIRE);
        else hudC(23, "WALK THE POT TO THE MOUTH", PAL_HUD);
        hudC(26, "C FIRE   A LEAVE ON THE CHIME", PAL_DIM);
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
    } else if (mode_ == Mode::Slide) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            held_ = Mode::Slide;
            mode_ = Mode::Pause;
        } else if (fires_ < kFires) {
            const bool tap = bot_ ? (wind_ == kTap) : pad.pressed(gs::BTN_C);
            if (tap) fire(wind_ >= kWindowLo && wind_ <= kWindowHi);
            else if (++wind_ >= kMissAt) fire(false);
        }
    } else if (mode_ == Mode::Glow) {
        anim_++;
        if (anim_ >= 12) {
            wind_ = anim_ = 0;
            mode_ = Mode::Gap;
        }
    } else if (mode_ == Mode::Gap) {
        anim_++;
        if (anim_ >= 10) {
            anim_ = 0;
            mode_ = (fires_ >= kFires) ? Mode::Hold : Mode::Slide;
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

    if (mode_ == Mode::Slide || mode_ == Mode::Glow || mode_ == Mode::Gap || mode_ == Mode::Hold) {
        if (pastHour()) fail("the chime went unheard");
        else if (bot_ && mode_ == Mode::Hold && onHour() && ready()) leave();
        else if (!bot_ && pad.pressed(gs::BTN_A)) leave();
    }

    draw();
}

}  // namespace kilnchime

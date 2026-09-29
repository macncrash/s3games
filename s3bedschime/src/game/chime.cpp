#include "game/chime.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace bedschime {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr int kHour = 12 * 3600;

float bedX(int i) { return 54.f + float(i % 3) * 100.f; }
float bedY(int i) { return i < 3 ? 108.f : 176.f; }

}  // namespace

void Game::readClock() {
    int t = kOpenSec + playFrames_ / 60;
    if (t < 0) t = 0;
    if (chimed_ || (mode_ == Mode::Fail && hour_ == 12)) t = kHour;
    second_ = t % 60;
    minute_ = (t / 60) % 60;
    int h = (t / 3600) % 12;
    hour_ = h == 0 ? 12 : h;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    buildArt(sys.vdp, art_);
    toTitle();
    if (bot_) begin();
    draw();
}

void Game::toTitle() {
    mode_ = Mode::Title;
    over_ = won_ = chimed_ = pouring_ = false;
    wet_ = 0;
    cursor_ = 0;
    playFrames_ = hold_ = chimeStep_ = 0;
    why_ = "hour silent";
    vis_ = 0;
    for (int i = 0; i < kBeds; i++) bed_[i] = {};
    readClock();
    if (sys_) sys_->apu.silence();
}

void Game::begin() {
    toTitle();
    mode_ = Mode::Pour;
}

void Game::lockBed() {
    Bed& b = bed_[cursor_];
    if (b.held) return;
    if (b.fill < kGoldLo || b.fill >= kGoldHi) return;
    b.held = true;
    wet_++;
    pouring_ = false;
    sys_->apu.tone(0, 440.f + float(wet_) * 40.f, 0.08f);
    toneT_ = 0.1f;
}

void Game::flood() {
    pouring_ = false;
    miss("bed flooded");
}

void Game::miss(const char* why) {
    if (mode_ != Mode::Pour) return;
    why_ = why;
    won_ = chimed_ = false;
    mode_ = Mode::Fail;
    hold_ = 0;
    pouring_ = false;
    readClock();
    sys_->apu.tone(1, 70.f, 0.16f);
    toneT_ = 0.3f;
}

void Game::strike() {
    chimed_ = true;
    won_ = true;
    why_ = "the hour chimes";
    hour_ = 12;
    minute_ = 0;
    second_ = 0;
    mode_ = Mode::Chime;
    hold_ = 0;
    chimeStep_ = 0;
    pouring_ = false;
}

void Game::passHour(const char* why) {
    why_ = why;
    hour_ = 12;
    minute_ = 0;
    second_ = 0;
    won_ = chimed_ = false;
    mode_ = Mode::Fail;
    hold_ = 0;
    pouring_ = false;
    sys_->apu.tone(1, 64.f, 0.14f);
    toneT_ = 0.3f;
}

void Game::steer() {
    const gs::Pad& p = sys_->pad;
    if (p.pressed(gs::BTN_LEFT)) cursor_ = (cursor_ + kBeds - 1) % kBeds;
    if (p.pressed(gs::BTN_RIGHT)) cursor_ = (cursor_ + 1) % kBeds;
    if (p.pressed(gs::BTN_UP) && cursor_ >= 3) cursor_ -= 3;
    if (p.pressed(gs::BTN_DOWN) && cursor_ < 3) cursor_ += 3;
    bool want = p.down(gs::BTN_A) || p.down(gs::BTN_B);
    Bed& b = bed_[cursor_];
    if (b.held) {
        pouring_ = false;
        return;
    }
    if (want) {
        pouring_ = true;
        b.fill += kPour * kDt;
        if (b.fill >= 1.f) {
            b.fill = 1.f;
            flood();
        }
    } else if (pouring_) {
        pouring_ = false;
        lockBed();
    }
}

void Game::botAct() {
    if (wet_ >= kBeds) {
        pouring_ = false;
        return;
    }
    int next = 0;
    while (next < kBeds && bed_[next].held) next++;
    cursor_ = next;
    Bed& b = bed_[cursor_];
    if (b.fill < 0.66f) {
        pouring_ = true;
        b.fill += kPour * kDt;
        if (b.fill >= 1.f) {
            b.fill = 1.f;
            flood();
        }
        return;
    }
    if (pouring_) {
        pouring_ = false;
        lockBed();
    }
}

void Game::audio(float dt) {
    if (toneT_ > 0) {
        toneT_ -= dt;
        if (toneT_ <= 0) {
            sys_->apu.tone(0, 0, 0);
            sys_->apu.tone(1, 0, 0);
            sys_->apu.tone(2, 0, 0);
        }
    }
    if (mode_ != Mode::Chime) return;
    static const float notes[] = {392.f, 494.f, 587.f, 784.f};
    int step = hold_ / 8;
    if (step != chimeStep_ && step >= 0 && step < 4) {
        chimeStep_ = step;
        sys_->apu.tone(0, notes[step], 0.16f);
        sys_->apu.tone(1, notes[step] * 0.5f, 0.08f);
        toneT_ = 0.2f;
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    vis_ += kDt;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) begin();
    } else if (mode_ == Mode::Pour) {
        int prevSec = second_;
        playFrames_++;
        readClock();
        if (second_ != prevSec && second_ != 0) {
            sys.apu.tone(2, 880.f, 0.03f);
            toneT_ = 0.04f;
        }
        if (bot_) botAct();
        else steer();
        if (mode_ == Mode::Pour) {
            int t = kOpenSec + playFrames_ / 60;
            if (t >= kHour) {
                if (wet_ == kBeds && !pouring_) strike();
                else passHour(pouring_ ? "still pouring" : "beds short");
            }
        }
    } else if (mode_ == Mode::Chime) {
        hold_++;
        if (hold_ > 56) {
            over_ = true;
            mode_ = Mode::Over;
            if (!sys.headless) sys.quit();
        }
    } else if (mode_ == Mode::Fail) {
        hold_++;
        if (hold_ > (bot_ ? 10 : 50)) {
            over_ = true;
            mode_ = Mode::Over;
        }
    } else if (mode_ == Mode::Over) {
        if (!won_ && !bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) toTitle();
    }
    audio(kDt);
    draw();
}

void Game::spr(const gs::Image& img, float cx, float cy, float w, float h, int pal) {
    if (!sys_ || img.w == 0 || w < 1.f || h < 1.f) return;
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = img;
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
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

void Game::hudC(int row, const char* s, int pal) {
    int n = s ? int(std::strlen(s)) : 0;
    hud(20 - n / 2, row, s, pal);
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        int g = y < 70 ? 2 : 4;
        int b = y < 70 ? 6 : 2;
        v.lineBackdrop[y] = gs::rgb4(g, g + 3, b);
    }

    const float ccx = 160.f;
    const float ccy = 28.f;
    int shown = chimed_ || mode_ == Mode::Chime ? 0 : second_;
    float ang = float(shown) * 0.10472f - 1.5708f;
    spr(art_.hand, ccx + std::cos(ang) * 11.f, ccy + std::sin(ang) * 11.f, 3.f, 10.f, PAL_CLOCK);
    float mang = (minute_ == 0 && hour_ == 12 ? 0.f : 59.f) * 0.10472f - 1.5708f;
    spr(art_.hand, ccx + std::cos(mang) * 6.f, ccy + std::sin(mang) * 6.f, 4.f, 7.f, PAL_DEAD);
    spr(art_.clock, ccx, ccy, 42.f, 42.f, PAL_CLOCK);

    for (int i = 0; i < kBeds; i++) {
        float x = bedX(i);
        float y = bedY(i);
        spr(art_.bed, x, y, 72.f, 40.f, PAL_BED);
        int soilPal = bed_[i].fill >= 1.f ? PAL_DEAD : (bed_[i].fill > 0.2f ? PAL_WET : PAL_BED);
        spr(art_.soil, x, y + 2.f, 58.f, 14.f, soilPal);
        float grow = 8.f + bed_[i].fill * 16.f;
        if (bed_[i].held) grow = 22.f;
        spr(art_.plant, x, y - 10.f, grow * 0.8f, grow, bed_[i].held ? PAL_PLANT : PAL_YARD);
        if (pouring_ && i == cursor_) {
            float bob = std::sin(vis_ * 14.f) * 3.f;
            spr(art_.drop, x + 10.f, y - 16.f + bob, 6.f, 8.f, PAL_DROP);
        }
    }
    if (mode_ == Mode::Pour || mode_ == Mode::Title) {
        spr(art_.can, bedX(cursor_) + 22.f, bedY(cursor_) - 22.f, 16.f, 22.f, PAL_CAN);
    }

    char clk[16];
    std::snprintf(clk, sizeof clk, "%d:%02d:%02d", hour_, minute_, second_);

    if (mode_ == Mode::Title) {
        hudC(1, "BEDS CHIME", PAL_TITLE);
        hudC(4, clk, PAL_INK);
        hudC(16, "WATER SIX BEDS", PAL_INK);
        hudC(17, "THE HOUR HAS TO CHIME", PAL_HINT);
        hudC(25, "START", PAL_TITLE);
    } else if (mode_ == Mode::Pour) {
        hud(1, 1, clk, second_ >= 54 ? PAL_TITLE : PAL_INK);
        char buf[24];
        std::snprintf(buf, sizeof buf, "WET %d/%d", wet_, kBeds);
        hud(30, 1, buf, PAL_HINT);
        const Bed& b = bed_[cursor_];
        int bars = int(std::max(0.f, std::min(1.f, b.fill)) * 8.f + 0.5f);
        if (bars > 8) bars = 8;
        char meter[20];
        std::snprintf(meter, sizeof meter, "%.*s%.*s", bars, "########", 8 - bars, "........");
        int pal = PAL_INK;
        if (b.held) pal = PAL_WIN;
        else if (b.fill >= kGoldLo && b.fill < kGoldHi) pal = PAL_TITLE;
        else if (b.fill >= kGoldHi) pal = PAL_DEAD;
        hud(1, 26, meter, pal);
        hudC(24, b.held ? "BED HOLDS" : "POUR TO GOLD", PAL_HINT);
    } else if (mode_ == Mode::Chime || (mode_ == Mode::Over && won_)) {
        hudC(1, "12:00:00", PAL_WIN);
        hudC(3, "THE HOUR CHIMES", PAL_WIN);
        hudC(25, "THE BEDS HELD", PAL_HINT);
    } else if (mode_ == Mode::Fail || mode_ == Mode::Over) {
        hudC(1, clk, PAL_DEAD);
        hudC(3, why_, PAL_DEAD);
        hudC(25, "THE HOUR STAYS QUIET", PAL_DEAD);
        if (!bot_ && mode_ == Mode::Over) hudC(26, "START", PAL_HINT);
    }
}

}  // namespace bedschime

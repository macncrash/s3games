#include "game/chime.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace boardchime {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr int kX0 = 18;
constexpr int kX1 = 262;
constexpr float kLife = 3.4f;
constexpr int kHour = 12 * 3600;

const char* kLine[JACKS] = {"DOCK", "MILL", "YARD", "GATE", "BARN", "LOFT"};

int jackY(int i) { return 86 + i * 20; }

}  // namespace

int Game::irnd(int n) {
    rng_ = rng_ * 1664525u + 1013904223u;
    if (n <= 1) return 0;
    return int((rng_ >> 8) % uint32_t(n));
}

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
    over_ = won_ = chimed_ = held_ = false;
    call_ = {};
    made_ = 0;
    bank_ = row_ = 0;
    playFrames_ = gap_ = hold_ = chimeStep_ = 0;
    why_ = "hour silent";
    clockVis_ = 0;
    readClock();
    if (sys_) sys_->apu.silence();
}

void Game::begin() {
    toTitle();
    mode_ = Mode::Patch;
    rng_ = bot_ ? 0xC41Cu : (0xC41Cu ^ uint32_t(sys_->frame) * 0x9E3779B9u);
    spawn();
}

void Game::spawn() {
    if (made_ >= kNeed) {
        call_.on = false;
        return;
    }
    call_.on = true;
    call_.trunk = irnd(JACKS);
    call_.line = irnd(JACKS);
    call_.life = call_.lifeMax = kLife;
    held_ = false;
}

void Game::lift() {
    if (!call_.on || held_ || row_ != call_.trunk) return;
    held_ = true;
    holdTrunk_ = call_.trunk;
    holdLine_ = call_.line;
    bank_ = 1;
    sys_->apu.noiseBurst(0.18f, 1800.f, 0.03f);
}

void Game::seat() {
    if (!held_) return;
    if (row_ != holdLine_) {
        sys_->apu.tone(2, 90.f, 0.08f);
        toneT_ = 0.12f;
        return;
    }
    held_ = false;
    call_.on = false;
    made_++;
    sys_->apu.tone(0, 520.f, 0.08f);
    toneT_ = 0.08f;
    if (made_ < kNeed) gap_ = 16;
}

void Game::miss(const char* why) {
    if (mode_ != Mode::Patch) return;
    why_ = why;
    won_ = chimed_ = false;
    mode_ = Mode::Fail;
    hold_ = 0;
    held_ = false;
    call_.on = false;
    readClock();
    sys_->apu.noiseBurst(0.2f, 400.f, 0.12f);
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
    call_.on = false;
    held_ = false;
}

void Game::passHour(const char* why) {
    why_ = why;
    hour_ = 12;
    minute_ = 0;
    second_ = 0;
    won_ = chimed_ = false;
    mode_ = Mode::Fail;
    hold_ = 0;
    held_ = false;
    call_.on = false;
    sys_->apu.tone(1, 64.f, 0.14f);
    toneT_ = 0.3f;
}

void Game::steer() {
    const gs::Pad& p = sys_->pad;
    if (p.pressed(gs::BTN_UP)) row_ = (row_ + JACKS - 1) % JACKS;
    if (p.pressed(gs::BTN_DOWN)) row_ = (row_ + 1) % JACKS;
    if (p.pressed(gs::BTN_LEFT)) bank_ = 0;
    if (p.pressed(gs::BTN_RIGHT)) bank_ = 1;
    if (p.pressed(gs::BTN_A) || p.pressed(gs::BTN_B)) {
        if (!held_) {
            if (bank_ == 0) lift();
        } else if (bank_ == 1) {
            seat();
        }
    }
}

void Game::botAct() {
    if (!call_.on) return;
    if (!held_) {
        bank_ = 0;
        if (row_ != call_.trunk) row_ += call_.trunk > row_ ? 1 : -1;
        else lift();
        return;
    }
    bank_ = 1;
    if (row_ != holdLine_) row_ += holdLine_ > row_ ? 1 : -1;
    else seat();
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
    static const float notes[] = {523.f, 659.f, 784.f, 1046.f};
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
    clockVis_ += kDt;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) begin();
    } else if (mode_ == Mode::Patch) {
        int prevSec = second_;
        playFrames_++;
        readClock();
        if (second_ != prevSec && second_ != 0) {
            sys.apu.tone(2, 880.f, 0.03f);
            toneT_ = 0.04f;
        }
        if (bot_) botAct();
        else steer();
        if (mode_ == Mode::Patch && call_.on) {
            call_.life -= kDt;
            if (call_.life <= 0) miss("lamp died");
        }
        if (mode_ == Mode::Patch && !call_.on && made_ < kNeed) {
            if (gap_ > 0) gap_--;
            else spawn();
        }
        if (mode_ == Mode::Patch) {
            int t = kOpenSec + playFrames_ / 60;
            if (t >= kHour) {
                if (made_ == kNeed && !call_.on) strike();
                else passHour(call_.on ? "line open" : "board short");
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
        int sky = y < 52 ? 2 : 1;
        v.lineBackdrop[y] = gs::rgb4(sky, sky, sky + 2);
    }

    const float ccx = 160.f;
    const float ccy = 30.f;
    int shown = chimed_ || mode_ == Mode::Chime ? 0 : second_;
    float ang = float(shown) * 0.10472f - 1.5708f;
    float hx = ccx + std::cos(ang) * 12.f;
    float hy = ccy + std::sin(ang) * 12.f;
    spr(art_.hand, hx, hy, 3.f, 10.f, PAL_CLOCK);
    float mang = (minute_ == 0 && hour_ == 12 ? 0.f : 59.f) * 0.10472f - 1.5708f;
    spr(art_.hand, ccx + std::cos(mang) * 7.f, ccy + std::sin(mang) * 7.f, 4.f, 8.f, PAL_DEAD);

    float pulse = call_.on ? (0.72f + 0.28f * std::sin(clockVis_ * 11.f)) : 0.55f;
    for (int i = 0; i < JACKS; i++) {
        float y = float(jackY(i));
        bool hot = call_.on && i == call_.trunk;
        if (hot) spr(art_.lamp, float(kX0 + 42), y, 16.f * pulse, 16.f * pulse, PAL_LAMP);
        else spr(art_.lamp, float(kX0 + 42), y, 9.f, 9.f, PAL_DESK);
        if (held_ && i == holdTrunk_) {
            float x0 = float(kX0 + 62);
            float x1 = float(kX1 - 28);
            float y1 = float(jackY(holdLine_));
            for (int b = 1; b <= 5; b++) {
                float u = float(b) / 6.f;
                float sag = std::sin(u * 3.1416f) * 10.f;
                spr(art_.bead, x0 + (x1 - x0) * u, y + (y1 - y) * u + sag, 4.f, 4.f, PAL_CORD);
            }
            spr(art_.plug, x1, y1, 12.f, 16.f, PAL_PLUG);
        }
    }

    if (mode_ == Mode::Patch) {
        float cy = float(jackY(row_));
        float cx = bank_ == 0 ? float(kX0 + 22) : float(kX1 - 8);
        if (!(held_ && bank_ == 1)) spr(art_.plug, cx, cy, 12.f, 18.f, PAL_CORD);
    }

    spr(art_.clock, ccx, ccy, 46.f, 46.f, PAL_CLOCK);
    spr(art_.desk, 160.f, 142.f, float(art_.desk.w), float(art_.desk.h), PAL_DESK);

    char clk[16];
    std::snprintf(clk, sizeof clk, "%d:%02d:%02d", hour_, minute_, second_);

    if (mode_ == Mode::Title) {
        hudC(1, "BOARD CHIME", PAL_TITLE);
        hudC(3, clk, PAL_INK);
        hudC(16, "SEAT EACH LAMP", PAL_INK);
        hudC(17, "THE HOUR HAS TO CHIME", PAL_HINT);
        hudC(25, "START", PAL_TITLE);
    } else if (mode_ == Mode::Patch) {
        hud(1, 1, clk, second_ >= 56 || second_ < 2 ? PAL_TITLE : PAL_INK);
        char buf[24];
        std::snprintf(buf, sizeof buf, "PATCH %d/%d", made_, kNeed);
        hud(30, 1, buf, PAL_HINT);
        for (int i = 0; i < JACKS; i++) {
            int pal = (call_.on && i == call_.line) ? PAL_TITLE : PAL_INK;
            hud(33, 9 + i * 2, kLine[i], pal);
        }
        if (call_.on) {
            int bars = int(std::max(0.f, call_.life / call_.lifeMax) * 8.f + 0.5f);
            if (bars > 8) bars = 8;
            char meter[20];
            std::snprintf(meter, sizeof meter, "%.*s%.*s", bars, "########", 8 - bars, "........");
            hud(1, 26, meter, call_.life < 1.2f ? PAL_DEAD : PAL_TITLE);
        }
        hudC(24, held_ ? "SEAT THE LINE" : "LIFT THE LAMP", PAL_HINT);
    } else if (mode_ == Mode::Chime || (mode_ == Mode::Over && won_)) {
        hudC(1, "12:00:00", PAL_WIN);
        hudC(3, "THE HOUR CHIMES", PAL_WIN);
        hudC(25, "THE BOARD HELD", PAL_HINT);
    } else if (mode_ == Mode::Fail || mode_ == Mode::Over) {
        hudC(1, clk, PAL_DEAD);
        hudC(3, why_, PAL_DEAD);
        hudC(25, won_ ? "THE HOUR CHIMES" : "THE HOUR STAYS QUIET", PAL_DEAD);
        if (!bot_ && mode_ == Mode::Over) hudC(26, "START", PAL_HINT);
    }
}

}  // namespace boardchime

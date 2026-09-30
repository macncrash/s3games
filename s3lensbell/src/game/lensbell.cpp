#include "game/lensbell.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace lensbell {

namespace {
constexpr float kFocus = 0.72f;
constexpr float kFocusWin = 0.055f;
constexpr float kFrameWin = 15.f;
constexpr float kAmp = 30.f;
constexpr float kRate = 0.04f;
}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    rung_ = false;
    dead_ = 0;
    tryNo_ = 1;
    t_ = 0;
    focus_ = 0.22f;
    pan_ = -46.f;
    why_ = "";
}

void Game::begin() {
    dead_ = 0;
    tryNo_ = 1;
    over_ = false;
    won_ = false;
    rung_ = false;
    why_ = "";
    nextTry();
}

void Game::nextTry() {
    mode_ = Mode::Aim;
    t_ = 0;
    settle_ = 0;
    focus_ = 0.22f;
    pan_ = -46.f;
    sharp_ = false;
    framed_ = false;
    why_ = "";
}

float Game::bellX() const { return kAmp * std::sin(t_ * kRate); }

void Game::trip() {
    sharp_ = std::fabs(focus_ - kFocus) < kFocusWin;
    framed_ = std::fabs(pan_ - bellX()) < kFrameWin;
    mode_ = Mode::Flash;
    flashT_ = 0;
    sys_->apu.noiseBurst(0.18f, 2400.f, 0.05f);
}

void Game::dieTry() {
    dead_++;
    if (!sharp_ && !framed_) why_ = "SOFT AND WIDE";
    else if (!sharp_) why_ = "SOFT PLATE";
    else why_ = "OFF THE GLASS";
    mode_ = Mode::Dead;
    holdT_ = 0;
    sys_->apu.tone(0, 140.f, 0.1f);
    beep_ = 18;
}

void Game::ring() {
    rung_ = true;
    won_ = true;
    why_ = "THE BELL RINGS";
    mode_ = Mode::Ring;
    holdT_ = 0;
    sys_->apu.tone(0, 880.f, 0.14f);
    beep_ = 8;
}

void Game::judge() {
    if (sharp_ && framed_) ring();
    else dieTry();
}

void Game::botAim() {
    float df = kFocus - focus_;
    focus_ = std::clamp(focus_ + std::clamp(df, -0.02f, 0.02f), 0.f, 1.f);
    float dx = bellX() - pan_;
    pan_ = std::clamp(pan_ + std::clamp(dx, -2.4f, 2.4f), -90.f, 90.f);
    bool ready = std::fabs(focus_ - kFocus) < kFocusWin * 0.6f && std::fabs(pan_ - bellX()) < kFrameWin * 0.45f;
    if (ready) {
        if (++settle_ > 10) trip();
    } else {
        settle_ = 0;
    }
}

void Game::humanAim() {
    const gs::Pad& p = sys_->pad;
    if (p.down(gs::BTN_LEFT)) pan_ -= 2.1f;
    if (p.down(gs::BTN_RIGHT)) pan_ += 2.1f;
    if (p.down(gs::BTN_DOWN)) focus_ -= 0.008f;
    if (p.down(gs::BTN_UP)) focus_ += 0.008f;
    if (std::fabs(p.axisX) > 0.2f) pan_ += p.axisX * 2.4f;
    if (std::fabs(p.axisY) > 0.2f) focus_ += p.axisY * 0.01f;
    pan_ = std::clamp(pan_, -90.f, 90.f);
    focus_ = std::clamp(focus_, 0.f, 1.f);
    if (p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C)) trip();
}

void Game::pumpAudio() {
    if (beep_ > 0 && --beep_ == 0) sys_->apu.tone(0, 0, 0);
    if (mode_ == Mode::Ring) {
        if (holdT_ == 10) sys_->apu.tone(1, 1174.f, 0.1f);
        if (holdT_ == 26) {
            sys_->apu.tone(1, 0, 0);
            sys_->apu.tone(0, 880.f, 0.09f);
            beep_ = 30;
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& p = sys.pad;
    if (mode_ == Mode::Title) {
        t_++;
        if (bot_) {
            if (t_ > 24) begin();
        } else if (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A)) {
            begin();
        }
    } else if (mode_ == Mode::Aim) {
        t_++;
        if (bot_) botAim();
        else humanAim();
    } else if (mode_ == Mode::Flash) {
        if (++flashT_ > 8) judge();
    } else if (mode_ == Mode::Dead) {
        if (++holdT_ > 70) {
            if (dead_ >= 3) {
                mode_ = Mode::Over;
                over_ = true;
                won_ = false;
                why_ = "THIRD TRY DIED";
            } else {
                tryNo_++;
                nextTry();
            }
        }
    } else if (mode_ == Mode::Ring) {
        holdT_++;
        if (holdT_ > 110) {
            mode_ = Mode::Over;
            over_ = true;
            won_ = true;
        }
    } else if (mode_ == Mode::Over) {
        if (!bot_ && (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A))) begin();
    }
    pumpAudio();
    draw();
}

void Game::backdrop() {
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int sky = y < 150 ? y : 150;
        int r = 2 + sky / 50;
        int g = 3 + sky / 36;
        int b = 8 + (150 - sky) / 22;
        if (y >= 150) {
            r = 4;
            g = 4;
            b = 5;
        }
        sys_->vdp.lineBackdrop[y] = gs::rgb4(std::clamp(r, 0, 15), std::clamp(g, 0, 15), std::clamp(b, 0, 15));
        sys_->vdp.lineFog[y] = 0;
        sys_->vdp.road[y].on = false;
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 40 || s.x + s.w < -40 || s.y > gs::SCREEN_H + 40 || s.y + s.h < -40) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = 0;
    while (s[n]) n++;
    hud(20 - n / 2, row, s, pal);
}

void Game::draw() {
    sys_->vdp.clearSprites();
    sys_->vdp.HUD.clear();
    backdrop();

    float swing = 0;
    if (mode_ == Mode::Ring) swing = std::sin(holdT_ * 0.32f) * 16.f * std::exp(-holdT_ * 0.025f);
    float bx = 160.f + (bellX() - pan_) + swing;
    float by = 78.f;
    float ferr = std::fabs(focus_ - kFocus);
    int fog = mode_ == Mode::Ring || mode_ == Mode::Flash ? 0 : int(std::clamp(ferr * 90.f, 0.f, 14.f));

    spr(art_.beam, 160, 28, 16, PAL_STONE);
    spr(art_.rope, bx, 40, 22, PAL_BELL);
    spr(art_.bell, bx, by, 70, PAL_BELL, false, fog);
    spr(art_.clapper, bx + swing * 0.15f, by + 16, 14, PAL_BELL, false, fog);
    if (fog > 3) spr(art_.haze, bx, by, 36 + fog, PAL_GLASS, false, std::min(fog, 8));

    const float fx0 = 118, fy0 = 52, fx1 = 202, fy1 = 118;
    spr(art_.corner, fx0, fy0, 14, PAL_GLASS);
    spr(art_.corner, fx1, fy0, 14, PAL_GLASS, true);
    spr(art_.corner, fx0, fy1, 14, PAL_GLASS);
    spr(art_.corner, fx1, fy1, 14, PAL_GLASS, true);

    float split = (focus_ - kFocus) * 70.f;
    spr(art_.caret, 160 - 10 - split, 40, 12, PAL_OK);
    spr(art_.caret, 160 + 10 + split, 40, 12, PAL_ALERT);

    spr(art_.body, 168, 196, 48, PAL_BRASS);
    spr(art_.barrel, 214, 188, 30, PAL_BRASS);
    spr(art_.glass, 214, 188, 16, PAL_GLASS, false, fog / 2);

    float scaleX = 36.f + focus_ * 70.f;
    spr(art_.notch, 36.f + kFocus * 70.f, 168, 16, PAL_OK);
    spr(art_.caret, scaleX, 168, 14, PAL_HUD);

    for (int i = 0; i < 3; i++) {
        int pal = i < dead_ ? PAL_ALERT : PAL_PAPER;
        if (rung_ && i == tryNo_ - 1) pal = PAL_OK;
        spr(art_.plate, 250 + i * 18, 188, 14, pal);
    }

    if (mode_ == Mode::Flash) {
        for (int y = 40; y < 130; y += 8) sys_->vdp.lineBackdrop[y] = gs::rgb4(15, 15, 14);
    }

    hud(1, 1, "S3 LENSBELL", PAL_HUD);
    char line[48];
    std::snprintf(line, sizeof(line), "TRY %d", tryNo_);
    hud(30, 1, line, PAL_HUD);
    std::snprintf(line, sizeof(line), "LOST %d", dead_);
    hud(30, 2, line, dead_ ? PAL_ALERT : PAL_HUD);

    if (mode_ == Mode::Title) {
        hudC(10, "A SHORT LENS", PAL_HUD);
        hudC(12, "RING THE BELL", PAL_BRASS);
        hudC(14, "BEFORE THE THIRD TRY DIES", PAL_HUD);
        hudC(18, "A  START", PAL_OK);
        hudC(24, "ARROWS FRAME AND FOCUS", PAL_HUD);
        hudC(25, "A  TRIPS THE SHUTTER", PAL_HUD);
    } else if (mode_ == Mode::Aim) {
        bool sharp = std::fabs(focus_ - kFocus) < kFocusWin;
        bool framed = std::fabs(pan_ - bellX()) < kFrameWin;
        hud(1, 24, sharp ? "SHARP" : "SOFT", sharp ? PAL_OK : PAL_ALERT);
        hud(10, 24, framed ? "IN GLASS" : "WIDE", framed ? PAL_OK : PAL_ALERT);
        hudC(26, "UP DOWN FOCUS   LEFT RIGHT FRAME   A TRIP", PAL_HUD);
    } else if (mode_ == Mode::Flash) {
        hudC(24, "EXPOSE", PAL_HUD);
    } else if (mode_ == Mode::Dead) {
        hudC(16, why_, PAL_ALERT);
        if (dead_ >= 3) hudC(18, "THE BELL STAYS QUIET", PAL_HUD);
    } else if (mode_ == Mode::Ring) {
        hudC(16, "THE BELL RINGS", PAL_OK);
        hudC(18, "LEAVE", PAL_HUD);
    } else if (mode_ == Mode::Over) {
        if (won_) {
            hudC(16, "THE BELL RANG", PAL_OK);
            std::snprintf(line, sizeof(line), "LEFT ON TRY %d", tryNo_);
            hudC(18, line, PAL_HUD);
        } else {
            hudC(16, "THIRD TRY DIED", PAL_ALERT);
            hudC(18, "BELL SILENT", PAL_HUD);
        }
        if (!bot_) hudC(22, "A  AGAIN", PAL_HUD);
    }
}

}  // namespace lensbell

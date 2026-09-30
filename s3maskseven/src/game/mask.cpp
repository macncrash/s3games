#include "game/mask.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace maskseven {

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Leave || mode_ == Mode::Over) return 2;
    return 1;
}

void Game::begin() {
    mode_ = Mode::Title;
    reason_ = "";
    you_ = 0;
    rival_ = 0;
    clock_ = 0;
    titleWait_ = 0;
    show_ = 0;
    flash_ = 0;
    barHit_ = false;
    over_ = false;
    won_ = false;
    released_ = true;
    beep_ = 0;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.HUD.enabled = true;
    sys.vdp.setFogColor(gs::rgb4(2, 1, 2));
    begin();
}

void Game::startGild() {
    mode_ = Mode::Gild;
    you_ = 0;
    rival_ = 0;
    clock_ = 0;
    barHit_ = false;
    flash_ = 0;
    show_ = 0;
    reason_ = "";
    won_ = false;
    over_ = false;
    released_ = true;
}

bool Game::inSweet() const {
    int t = clock_ % kPeriod;
    return t >= kSweet0 && t <= kSweet1;
}

float Game::needle() const {
    int t = clock_ % kPeriod;
    float u = t / float(kPeriod);
    return u < 0.5f ? u * 2.f : (1.f - u) * 2.f;
}

void Game::toLead() {
    mode_ = Mode::Lead;
    show_ = 0;
    reason_ = "";
    if (sys_) {
        sys_->apu.tone(0, 523.f, 0.28f);
        sys_->apu.tone(1, 659.f, 0.16f);
        beep_ = 0.22f;
    }
}

void Game::toLeave() {
    if (mode_ != Mode::Lead) return;
    mode_ = Mode::Leave;
    show_ = 0;
    reason_ = "LEAVE";
    if (sys_) {
        sys_->apu.tone(0, 392.f, 0.3f);
        sys_->apu.tone(1, 523.f, 0.18f);
        beep_ = 0.35f;
    }
}

void Game::toLost(const char* why) {
    if (mode_ == Mode::Lost || mode_ == Mode::Over || mode_ == Mode::Leave || mode_ == Mode::Lead) return;
    mode_ = Mode::Lost;
    show_ = 0;
    reason_ = why;
    won_ = false;
    if (sys_) {
        sys_->apu.tone(0, 90.f, 0.26f);
        beep_ = 0.2f;
    }
}

void Game::finishLeave() {
    won_ = you_ >= kGoal && you_ > rival_;
    over_ = true;
    mode_ = Mode::Over;
}

void Game::lay(bool sweet) {
    if (mode_ != Mode::Gild || barHit_) return;
    barHit_ = true;
    if (sweet) {
        you_++;
        flash_ = 8;
        if (sys_) {
            sys_->apu.tone(0, 330.f + you_ * 28.f, 0.3f);
            beep_ = 0.12f;
        }
        if (you_ >= kGoal && you_ > rival_) toLead();
    } else {
        rival_++;
        flash_ = 10;
        if (sys_) {
            sys_->apu.tone(0, 140.f, 0.22f);
            beep_ = 0.1f;
        }
        if (rival_ >= kGoal && rival_ >= you_) toLost("LATE");
    }
}

void Game::missBar() {
    if (mode_ != Mode::Gild) return;
    if (!barHit_) {
        rival_++;
        if (sys_) {
            sys_->apu.tone(0, 110.f, 0.16f);
            beep_ = 0.08f;
        }
        if (rival_ >= kGoal && rival_ >= you_) toLost("BEHIND");
    }
    barHit_ = false;
    clock_ = 0;
}

Game::Input Game::readPad(const gs::Pad& pad) const {
    Input in;
    in.lay = pad.pressed(gs::BTN_A);
    in.spoil = pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_C);
    in.start = pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A);
    return in;
}

Game::Input Game::botInput() const {
    Input in;
    if (mode_ == Mode::Title && titleWait_ > 24) in.start = true;
    else if (mode_ == Mode::Gild && !barHit_ && (clock_ % kPeriod) == 18) in.lay = true;
    else if (mode_ == Mode::Lead && show_ > 16) in.start = true;
    else if (mode_ == Mode::Lost && show_ > 20) in.start = true;
    return in;
}

void Game::stepPlay(const Input& in) {
    if (mode_ == Mode::Title) {
        titleWait_++;
        if (in.start && titleWait_ > 8) startGild();
        return;
    }
    if (mode_ == Mode::Gild) {
        if (in.spoil) {
            toLost("SPOIL");
            return;
        }
        if (in.lay) lay(inSweet());
        if (mode_ == Mode::Gild) {
            clock_++;
            if (clock_ >= kPeriod) missBar();
        }
        return;
    }
    if (mode_ == Mode::Lead) {
        show_++;
        if (in.start && show_ > 8) toLeave();
        return;
    }
    if (mode_ == Mode::Leave) {
        show_++;
        if (show_ > 48) finishLeave();
        return;
    }
    if (mode_ == Mode::Lost) {
        show_++;
        if (show_ > 70) {
            over_ = true;
            won_ = false;
            mode_ = Mode::Over;
        } else if (in.start && show_ > 16 && !bot_) {
            begin();
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    Input in = bot_ ? botInput() : readPad(sys.pad);
    stepPlay(in);
    if (beep_ > 0.f) {
        beep_ -= 1.f / 60.f;
        if (beep_ <= 0.f) {
            sys.apu.tone(0, 0, 0);
            sys.apu.tone(1, 0, 0);
        }
    }
    if (flash_ > 0) flash_--;
    if (mode_ == Mode::Lead || mode_ == Mode::Leave || (mode_ == Mode::Over && won_)) sys.setLight(220, 170, 50);
    else if (mode_ == Mode::Lost || (mode_ == Mode::Over && !won_ && reason_[0])) sys.setLight(90, 24, 24);
    else if (mode_ == Mode::Gild) sys.setLight(160, 110, 40);
    else sys.setLight(36, 22, 32);
    draw();
}

void Game::spr(const gs::Image& img, float cx, float cy, float w, float h, int pal) {
    if (!sys_ || img.w == 0 || w < 1.f || h < 1.f) return;
    gs::Sprite s;
    s.img = img;
    s.x = int16_t(std::lround(cx - w * 0.5f));
    s.y = int16_t(std::lround(cy - h * 0.5f));
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
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
    if (!s) return;
    int n = int(std::strlen(s));
    hud(20 - n / 2, row, s, pal);
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    bool win = mode_ == Mode::Leave || mode_ == Mode::Lead || (mode_ == Mode::Over && won_);
    bool dead = mode_ == Mode::Lost || (mode_ == Mode::Over && !won_ && reason_[0]);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        if (y < 28) v.lineBackdrop[y] = win ? gs::rgb4(6, 3, 2) : dead ? gs::rgb4(3, 1, 2) : gs::rgb4(2, 1, 3);
        else if (y < 150) {
            int g = 2 + y / 40;
            v.lineBackdrop[y] = win ? gs::rgb4(8, 5, 2) : dead ? gs::rgb4(4, 2, 2) : gs::rgb4(4, g, 3);
        } else if (y < 168) {
            v.lineBackdrop[y] = gs::rgb4(7, 4, 2);
        } else {
            v.lineBackdrop[y] = gs::rgb4(5 + ((y / 4) & 1), 3, 1);
        }
    }
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    spr(art_.hook, 108, 28, 14, 16, PAL_HOOK);
    spr(art_.hook, 236, 28, 14, 16, PAL_HOOK);

    static const float kYouX[7] = {84, 100, 116, 132, 92, 108, 124};
    static const float kYouY[7] = {88, 82, 88, 96, 108, 114, 108};
    static const float kThemX[7] = {208, 220, 232, 244, 214, 226, 238};
    static const float kThemY[7] = {102, 96, 102, 110, 118, 122, 118};

    const bool hanging = mode_ == Mode::Leave || (mode_ == Mode::Over && won_);
    if (!hanging) {
        int yourPal = flash_ && mode_ == Mode::Gild ? PAL_GOLD : PAL_CLAY;
        spr(art_.yours, 108, 108, 92, 110, yourPal);
        for (int i = 0; i < you_ && i < kGoal; i++) spr(art_.foil, kYouX[i], kYouY[i], 14, 14, PAL_FOIL);
    }
    spr(art_.theirs, 228, 118, 70, 84, PAL_RIVAL);
    for (int i = 0; i < rival_ && i < kGoal; i++) spr(art_.foil, kThemX[i], kThemY[i], 11, 11, PAL_RIVAL);

    if (mode_ == Mode::Gild) {
        float n = needle();
        float x = 62.f + n * 92.f;
        int pal = inSweet() ? PAL_GOLD : PAL_BRUSH;
        spr(art_.bar, 108, 168, 100, 12, inSweet() ? PAL_MARK : PAL_WOOD);
        spr(art_.brush, x, 154, 16, 28, pal);
    } else if (hanging) {
        float lift = mode_ == Mode::Leave ? float(show_) * 1.4f : 70.f;
        if (lift > 78.f) lift = 78.f;
        spr(art_.yours, 108, 108 - lift, 92, 110, PAL_GOLD);
        for (int i = 0; i < kGoal; i++) spr(art_.foil, kYouX[i], kYouY[i] - lift, 14, 14, PAL_FOIL);
    }

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(2, "S3 MASKSEVEN", PAL_TITLE);
        hudC(5, "FIRST TO SEVEN", PAL_INK);
        hudC(18, "GILD YOUR MASK BEFORE THEIRS", PAL_GOLD);
        hudC(21, "A IN THE GOLD   START", PAL_DIM);
        hud(30, 26, S3_VERSION_STRING, PAL_DIM);
    } else if (mode_ == Mode::Gild) {
        hudC(1, inSweet() ? "GOLD" : "WAIT", inSweet() ? PAL_GOLD : PAL_DIM);
        std::snprintf(buf, sizeof buf, "YOU %d", you_);
        hud(1, 25, buf, PAL_YOU);
        std::snprintf(buf, sizeof buf, "RIVAL %d", rival_);
        hud(12, 25, buf, PAL_RIVAL);
        hud(24, 25, "SEVEN", PAL_INK);
    } else if (mode_ == Mode::Lead) {
        hudC(2, "FIRST TO SEVEN", PAL_GOLD);
        hudC(20, "HANG THE MASK", PAL_INK);
        hudC(23, "A TO LEAVE", PAL_TITLE);
    } else if (mode_ == Mode::Leave || (mode_ == Mode::Over && won_)) {
        hudC(2, "FIRST TO SEVEN", PAL_GOLD);
        std::snprintf(buf, sizeof buf, "YOU %d  RIVAL %d", you_, rival_);
        hudC(23, buf, PAL_INK);
        hudC(25, "LEAVE", PAL_TITLE);
    } else if (mode_ == Mode::Lost || (mode_ == Mode::Over && !won_)) {
        hudC(2, "THE OTHER MASK", PAL_BAD);
        hudC(5, "REACHED SEVEN", PAL_INK);
        std::snprintf(buf, sizeof buf, "YOU %d  RIVAL %d", you_, rival_);
        hudC(23, buf, PAL_DIM);
        if (!bot_) hudC(25, "START TO TRY AGAIN", PAL_DIM);
    }
}

}  // namespace maskseven

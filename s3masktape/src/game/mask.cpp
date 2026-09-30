#include "game/mask.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace masktape {

namespace {

const char* kNames[6] = {"WASH", "CROWN", "RINSE", "LIP", "DUST", "CHEEK"};

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Leave || mode_ == Mode::Over) return 2;
    return 1;
}

const char* Game::gateName(int g) const {
    if (g < 0 || g > 5) return "";
    return kNames[g];
}

const char* Game::tapeName(int i) const {
    if (i < 0 || i >= kSlots) return "";
    return kNames[kTapeGate[i]];
}

const char* Game::slotName(int i) const {
    if (i < 0 || i >= kSlots || held_[i] < 0) return "----";
    return gateName(held_[i]);
}

bool Game::tapeGate(int g) const {
    for (int i = 0; i < kSlots; i++)
        if (kTapeGate[i] == g) return true;
    return false;
}

bool Game::matched() const {
    for (int i = 0; i < kSlots; i++)
        if (held_[i] != kTapeGate[i]) return false;
    return true;
}

int Game::gate() const {
    int t = clock_ % kPeriod;
    int g = t / kSpan;
    if (g < 0) g = 0;
    if (g > 5) g = 5;
    return g;
}

bool Game::inBed() const {
    int t = clock_ % kPeriod;
    return (t % kSpan) < kWide;
}

int Game::nextOpen() const { return filled_ < kSlots ? filled_ : -1; }

void Game::begin() {
    mode_ = Mode::Title;
    reason_ = "";
    clock_ = 0;
    titleWait_ = 0;
    show_ = 0;
    flash_ = 0;
    filled_ = 0;
    faults_ = 0;
    swept_ = false;
    over_ = false;
    won_ = false;
    beep_ = 0;
    for (int i = 0; i < kSlots; i++) held_[i] = -1;
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
    clock_ = 0;
    swept_ = false;
    flash_ = 0;
    show_ = 0;
    filled_ = 0;
    faults_ = 0;
    reason_ = "";
    won_ = false;
    over_ = false;
    for (int i = 0; i < kSlots; i++) held_[i] = -1;
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
    if (mode_ != Mode::Lead || !matched()) return;
    mode_ = Mode::Leave;
    show_ = 0;
    reason_ = "MATCH";
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
    won_ = matched();
    over_ = true;
    mode_ = Mode::Over;
    if (!won_) reason_ = "OPEN";
}

void Game::lay() {
    if (mode_ != Mode::Gild || swept_) return;
    swept_ = true;
    int g = gate();
    int need = nextOpen();
    bool ok = inBed() && need >= 0 && g == kTapeGate[need];
    if (ok) {
        held_[need] = g;
        filled_++;
        flash_ = 8;
        if (sys_) {
            sys_->apu.tone(0, 330.f + filled_ * 40.f, 0.28f);
            beep_ = 0.12f;
        }
        if (matched()) toLead();
        return;
    }
    if (need >= 0 && inBed()) {
        held_[need] = g;
        filled_++;
    }
    flash_ = 10;
    toLost(inBed() ? "OFF TAPE" : "GAP");
}

void Game::endSweep() {
    if (mode_ != Mode::Gild) return;
    if (!swept_) {
        faults_++;
        if (sys_) {
            sys_->apu.tone(0, 110.f, 0.16f);
            beep_ = 0.08f;
        }
        if (faults_ >= kFaults) {
            toLost("MISSED");
            return;
        }
    }
    swept_ = false;
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
    else if (mode_ == Mode::Gild && !swept_) {
        int need = nextOpen();
        if (need >= 0) {
            int centre = kTapeGate[need] * kSpan + (kWide / 2);
            if ((clock_ % kPeriod) == centre) in.lay = true;
        }
    } else if (mode_ == Mode::Lead && show_ > 16) in.start = true;
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
        if (in.lay) lay();
        if (mode_ == Mode::Gild) {
            clock_++;
            if (clock_ >= kPeriod) endSweep();
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
        if (y < 22) v.lineBackdrop[y] = win ? gs::rgb4(6, 4, 2) : dead ? gs::rgb4(3, 1, 2) : gs::rgb4(2, 1, 3);
        else if (y < 148) {
            int g = 2 + y / 48;
            v.lineBackdrop[y] = win ? gs::rgb4(7, 5, 2) : dead ? gs::rgb4(4, 2, 2) : gs::rgb4(4, g, 3);
        } else if (y < 176) {
            v.lineBackdrop[y] = gs::rgb4(6, 4, 2);
        } else {
            v.lineBackdrop[y] = gs::rgb4(4 + ((y / 4) & 1), 2, 1);
        }
    }
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    const bool hanging = mode_ == Mode::Leave || (mode_ == Mode::Over && won_);
    float lift = 0.f;
    if (hanging) {
        lift = mode_ == Mode::Leave ? float(show_) * 1.2f : 64.f;
        if (lift > 64.f) lift = 64.f;
    }

    spr(art_.hook, 118, 26, 14, 18, PAL_HOOK);
    spr(art_.paper, 36, 78, 52, 78, PAL_PAPER);
    int maskPal = (flash_ && mode_ == Mode::Gild) ? PAL_GOLD : hanging ? PAL_GOLD : PAL_CLAY;
    spr(art_.mask, 168, 96 - lift, 100, 108, maskPal);

    static const float kFx[3] = {148.f, 168.f, 188.f};
    static const float kFy[3] = {72.f, 96.f, 78.f};
    for (int i = 0; i < filled_ && i < kSlots; i++) {
        bool good = held_[i] == kTapeGate[i];
        spr(art_.slip, kFx[i], kFy[i] - lift, 20, 12, good ? PAL_SLIP : PAL_DECOY);
    }

    spr(art_.drawer, 168, 196, 176, 30, PAL_DRAWER);
    for (int i = 0; i < kSlots; i++) {
        if (held_[i] < 0) continue;
        bool good = held_[i] == kTapeGate[i];
        spr(art_.slip, 108.f + float(i) * 56.f, 194.f, 26, 14, good ? PAL_SLIP : PAL_WAX);
    }

    if (mode_ == Mode::Gild) {
        float u = (float(clock_ % kPeriod) + 0.5f) / float(kPeriod);
        float x = 78.f + u * 180.f;
        int g = gate();
        bool hot = inBed() && nextOpen() >= 0 && g == kTapeGate[nextOpen()];
        spr(art_.brush, x, 150, 16, 26, hot ? PAL_GOLD : PAL_BRUSH);
        spr(art_.rail, 168, 168, 188, 14, hot ? PAL_MARK : inBed() && tapeGate(g) ? PAL_WOOD : PAL_DECOY);
    }

    char buf[64];
    if (mode_ == Mode::Title) {
        hudC(1, "S3 MASKTAPE", PAL_TITLE);
        hudC(4, "MATCH THE TAPE", PAL_INK);
        hudC(15, "DRAWER WANTS CROWN LIP CHEEK", PAL_GOLD);
        hudC(18, "WASH RINSE DUST STAY OUT", PAL_DIM);
        hudC(22, "A ON THE NEXT BED", PAL_INK);
        hud(30, 26, S3_VERSION_STRING, PAL_DIM);
    } else if (mode_ == Mode::Gild) {
        int g = gate();
        int need = nextOpen();
        bool hot = inBed() && need >= 0 && g == kTapeGate[need];
        hudC(1, hot ? "LAY" : gateName(g), hot ? PAL_GOLD : inBed() ? PAL_INK : PAL_DIM);
        std::snprintf(buf, sizeof buf, "TAPE %s %s %s", tapeName(0), tapeName(1), tapeName(2));
        hud(1, 24, buf, PAL_PAPER);
        std::snprintf(buf, sizeof buf, "TILL %s %s %s", slotName(0), slotName(1), slotName(2));
        hud(1, 26, buf, matched() ? PAL_GOLD : PAL_INK);
    } else if (mode_ == Mode::Lead) {
        hudC(2, "DRAWER MATCHES", PAL_GOLD);
        hudC(4, "CROWN LIP CHEEK", PAL_INK);
        hudC(21, "HANG THE MASK", PAL_TITLE);
        hudC(23, "A TO LEAVE", PAL_DIM);
    } else if (mode_ == Mode::Leave || (mode_ == Mode::Over && won_)) {
        hudC(2, "DRAWER MATCHES THE TAPE", PAL_GOLD);
        hudC(4, "CROWN  LIP  CHEEK", PAL_INK);
        hudC(25, "LEAVE", PAL_TITLE);
    } else if (mode_ == Mode::Lost || (mode_ == Mode::Over && !won_)) {
        hudC(2, "THE DRAWER DOES NOT", PAL_BAD);
        hudC(4, reason_[0] ? reason_ : "OPEN", PAL_INK);
        std::snprintf(buf, sizeof buf, "TILL %s %s %s", slotName(0), slotName(1), slotName(2));
        hudC(23, buf, PAL_DIM);
        if (!bot_) hudC(25, "START TO TRY AGAIN", PAL_DIM);
    }
}

}  // namespace masktape

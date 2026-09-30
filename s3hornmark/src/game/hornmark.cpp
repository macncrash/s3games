#include "game/hornmark.h"

#include <cstring>

namespace hornmark {
namespace {

constexpr float kFreq[3] = {196.f, 262.f, 330.f};
constexpr int kNeed = 28;
constexpr int kSour = 36;

}  // namespace

const char* Game::callName() const { return closed_ ? "closed" : "open"; }

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.apu.setMaster(0.5f);
    sys.apu.silence();
    mode_ = Mode::Title;
    pitch_ = 0;
    locked_ = 0;
    hold_ = 0;
    sourHold_ = 0;
    titleWait_ = 0;
    doneWait_ = 0;
    over_ = false;
    won_ = false;
    finished_ = false;
    closed_ = false;
    blowing_ = false;
    released_ = true;
    clock_ = 0;
}

void Game::begin() {
    mode_ = Mode::Call;
    locked_ = 0;
    hold_ = 0;
    sourHold_ = 0;
    pitch_ = phrase_[0];
    blowing_ = false;
    released_ = true;
    finished_ = false;
    closed_ = false;
    won_ = false;
}

void Game::lockNote() {
    locked_++;
    hold_ = 0;
    sourHold_ = 0;
    if (sys_) sys_->apu.tone(1, kFreq[phrase_[locked_ - 1]] * 2.f, 0.12f);
    if (locked_ >= kNotes) {
        finished_ = true;
        mode_ = Mode::Done;
        blowing_ = false;
        if (sys_) sys_->apu.tone(0, 0, 0);
    } else {
        pitch_ = phrase_[locked_];
    }
}

void Game::sour() {
    locked_ = 0;
    hold_ = 0;
    sourHold_ = 0;
    pitch_ = phrase_[0];
    finished_ = false;
    closed_ = false;
    if (sys_) {
        sys_->apu.tone(0, 90.f, 0.08f);
        sys_->apu.noiseBurst(0.15f, 1800.f, 0.08f);
    }
}

void Game::finish() {
    if (closed_) return;
    closed_ = true;
    won_ = true;
    fanfare();
}

void Game::fanfare() {
    if (!sys_) return;
    sys_->apu.tone(0, 262.f, 0.16f);
    sys_->apu.tone(1, 330.f, 0.12f);
    sys_->apu.tone(2, 392.f, 0.1f);
}

void Game::toneAt(int pitch, bool on) {
    if (!sys_) return;
    if (!on) {
        sys_->apu.tone(0, 0, 0);
        return;
    }
    int p = pitch < 0 ? 0 : (pitch > 2 ? 2 : pitch);
    sys_->apu.tone(0, kFreq[p], 0.18f);
}

Game::Input Game::readPad(const gs::Pad& pad) const {
    Input in;
    in.blow = pad.down(gs::BTN_A) || pad.down(gs::BTN_C) || pad.accel > 0.4f;
    in.start = pad.pressed(gs::BTN_START);
    if (pad.pressed(gs::BTN_UP)) in.pitch = pitch_ < 2 ? pitch_ + 1 : 2;
    else if (pad.pressed(gs::BTN_DOWN)) in.pitch = pitch_ > 0 ? pitch_ - 1 : 0;
    else if (pad.axisY > 0.55f && !pad.prev[gs::BTN_UP]) in.pitch = pitch_ < 2 ? pitch_ + 1 : 2;
    else if (pad.axisY < -0.55f) in.pitch = pitch_ > 0 ? pitch_ - 1 : 0;
    return in;
}

Game::Input Game::botInput() const {
    Input in;
    if (mode_ == Mode::Title) {
        if (titleWait_ > 24) in.start = true;
        return in;
    }
    if (mode_ == Mode::Done) return in;
    int want = phrase_[locked_ < kNotes ? locked_ : kNotes - 1];
    in.pitch = want;
    in.blow = hold_ < kNeed + 2;
    return in;
}

void Game::stepCall(const Input& in) {
    if (in.pitch >= 0) pitch_ = in.pitch;
    blowing_ = in.blow;
    int want = phrase_[locked_];
    if (!in.blow) {
        released_ = true;
        hold_ = 0;
        sourHold_ = 0;
        toneAt(pitch_, false);
        return;
    }
    released_ = false;
    toneAt(pitch_, true);
    if (pitch_ == want) {
        sourHold_ = 0;
        hold_++;
        if (hold_ >= kNeed) lockNote();
    } else {
        hold_ = 0;
        sourHold_++;
        if (sourHold_ >= kSour) sour();
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    clock_ += 1.f / 60.f;
    Input in = bot_ ? botInput() : readPad(sys.pad);
    if (mode_ == Mode::Title) {
        titleWait_++;
        if (in.start) begin();
    } else if (mode_ == Mode::Call) {
        stepCall(in);
    } else if (mode_ == Mode::Done) {
        blowing_ = false;
        toneAt(0, false);
        if (released_ || bot_) {
            if (!closed_ && (bot_ || !in.blow)) finish();
        }
        if (!bot_ && in.blow) {
            // still the open breath; the mark closes when the horn is let go
        } else if (!closed_) {
            released_ = true;
            finish();
        }
        doneWait_++;
        if (closed_ && doneWait_ > 36) over_ = true;
    }
    draw();
}

int Game::staffY(int pitch) const {
    if (pitch <= 0) return 92;
    if (pitch == 1) return 76;
    return 60;
}

void Game::spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool flip) {
    if (!sys_ || img.w == 0) return;
    gs::Sprite s;
    s.img = img;
    s.w = int16_t(w);
    s.h = int16_t(h);
    s.x = int16_t(cx - w * 0.5f);
    s.y = int16_t(cy - h * 0.5f);
    s.pal = uint8_t(pal);
    s.hflip = flip;
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
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        if (y < 150) {
            float u = y / 149.f;
            int r = 2 + int(u * 4);
            int g = 2 + int(u * 3);
            int b = 8 - int(u * 3);
            v.lineBackdrop[y] = gs::rgb4(r, g, b < 2 ? 2 : b);
        } else if (y < 178) {
            v.lineBackdrop[y] = gs::rgb4(3, 5, 2);
        } else {
            v.lineBackdrop[y] = gs::rgb4(2, 4, 2);
        }
    }
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();
    spr(art_.moon, 268, 36, 28, 28, PAL_MOON);
    spr(art_.pine, 36, 150, 36, 60, PAL_PINE);
    spr(art_.pine, 292, 156, 30, 50, PAL_PINE, true);
    spr(art_.rock, 70, 196, 40, 18, PAL_HILL);
    spr(art_.rock, 250, 200, 48, 16, PAL_HILL);
    spr(art_.player, 118, 168, 40, 64, PAL_COAT);
    float hx = blowing_ ? 168.f : 164.f;
    spr(art_.horn, hx, 158, 70, 40, PAL_BRASS);
    if (blowing_) spr(art_.breath, hx + 48, 150, 28, 16, PAL_BREATH);

    for (int i = 0; i < 5; i++) spr(art_.staff, 196, 52.f + i * 10.f, 150, 6, PAL_STAFF);
    for (int n = 0; n < kNotes; n++) {
        float x = 140.f + n * 36.f;
        float y = float(staffY(phrase_[n]));
        int pal = PAL_DIM;
        if (n < locked_) pal = PAL_GOLD;
        else if (mode_ != Mode::Title && n == locked_) pal = (pitch_ == phrase_[n] && blowing_) ? PAL_GOLD : PAL_NOTE;
        spr(art_.note, x, y, 16, 16, pal);
    }
    if (mode_ == Mode::Done) spr(art_.stamp, 196, 118, 40, 24, PAL_GOLD);

    hudC(1, "S3 HORNMARK", PAL_TITLE);
    if (mode_ == Mode::Title) {
        hudC(12, "A FINISHED MARK ENDS IT", PAL_INK);
        hudC(16, "UP DOWN  PITCH", PAL_GOLD);
        hudC(17, "HOLD A   BLOW THE CALL", PAL_GOLD);
        hudC(24, "START", PAL_TITLE);
    } else if (mode_ == Mode::Call) {
        const char* name = pitch_ == 0 ? "LOW" : (pitch_ == 1 ? "MID" : "HIGH");
        hud(2, 24, "PITCH", PAL_INK);
        hud(8, 24, name, pitch_ == phrase_[locked_] ? PAL_GOLD : PAL_BAD);
        hud(16, 24, blowing_ ? "BLOW" : "REST", PAL_INK);
        hudC(26, "FOUR NOTES  THEN LET GO", PAL_INK);
    } else {
        hudC(22, closed_ ? "FINISHED MARK" : "LET THE HORN GO", PAL_GOLD);
        hudC(24, "CALL CLOSED", closed_ ? PAL_GOLD : PAL_INK);
    }
}

}  // namespace hornmark

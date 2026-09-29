#include "game/memorybell.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace memorybell {
namespace {
constexpr float kY = 108.f;
constexpr float kFreq[NB] = {392.f, 494.f, 587.f, 698.f};
}  // namespace

float Game::bellX(int i) const { return 48.f + float(i) * 74.f; }

void Game::tone(int ch, float freq, float vol, int frames) {
    sys_->apu.tone(ch, freq, vol);
    if (ch == 0) tone0_ = frames;
    else tone1_ = frames;
}

void Game::toTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    rung_ = false;
    dead_ = 0;
    tryNo_ = 0;
    got_ = 0;
    cursor_ = 0;
    lit_ = -1;
    t_ = 0;
    swing_ = 0;
    lock_ = 0;
}

void Game::newGame() {
    over_ = false;
    won_ = false;
    rung_ = false;
    dead_ = 0;
    tryNo_ = 1;
    got_ = 0;
    cursor_ = 0;
    beginShow();
}

void Game::beginShow() {
    mode_ = Mode::Show;
    got_ = 0;
    lit_ = -1;
    t_ = 0;
    cursor_ = seq_[0];
    lock_ = 0;
}

void Game::showTick() {
    constexpr int kHold = 16;
    constexpr int kGap = 10;
    constexpr int kStep = kHold + kGap;
    int i = t_ / kStep;
    int ph = t_ % kStep;
    if (i >= SEQ) {
        mode_ = Mode::Recall;
        lit_ = -1;
        t_ = 0;
        cursor_ = 0;
        lock_ = 8;
        return;
    }
    if (ph == 0) {
        lit_ = seq_[i];
        tone(0, kFreq[lit_], 0.16f, kHold);
    }
    if (ph == kHold) lit_ = -1;
    t_++;
}

void Game::ring() {
    mode_ = Mode::Ring;
    won_ = true;
    rung_ = true;
    got_ = SEQ;
    lit_ = -1;
    t_ = 0;
    swing_ = 1.f;
    tone(0, 196.f, 0.22f, 50);
    tone(1, 392.f, 0.12f, 40);
}

void Game::dieTry() {
    dead_++;
    mode_ = Mode::Dead;
    lit_ = cursor_;
    t_ = 0;
    tone(0, 110.f, 0.14f, 18);
    if (dead_ >= TRIES) {
        won_ = false;
        rung_ = false;
    }
}

void Game::strike(int which) {
    if (mode_ != Mode::Recall || lock_ > 0) return;
    if (which < 0 || which >= NB) return;
    cursor_ = which;
    lit_ = which;
    if (which != seq_[got_]) {
        dieTry();
        return;
    }
    tone(0, kFreq[which], 0.18f, 10);
    got_++;
    lock_ = 8;
    if (got_ >= SEQ) ring();
}

bool Game::audit() const {
    bool seen[NB] = {};
    int n = 0;
    for (int i = 0; i < SEQ; i++) {
        if (seq_[i] < 0 || seq_[i] >= NB) return false;
        if (!seen[seq_[i]]) {
            seen[seq_[i]] = true;
            n++;
        }
    }
    return n == NB && SEQ == 4 && TRIES == 3;
}

void Game::botAct() {
    if (mode_ == Mode::Title && t_ > 10) newGame();
    else if (mode_ == Mode::Recall && lock_ == 0 && got_ < SEQ) strike(seq_[got_]);
}

void Game::readPad() {
    const gs::Pad& p = sys_->pad;
    bool start = p.pressed(gs::BTN_START);
    bool a = p.pressed(gs::BTN_A);
    if (mode_ == Mode::Title) {
        if (start || a) newGame();
        return;
    }
    if (mode_ == Mode::Over) {
        if (start || a) toTitle();
        return;
    }
    if (mode_ != Mode::Recall) return;
    if (p.pressed(gs::BTN_LEFT)) cursor_ = (cursor_ + NB - 1) % NB;
    if (p.pressed(gs::BTN_RIGHT)) cursor_ = (cursor_ + 1) % NB;
    if (a) strike(cursor_);
}

void Game::pumpAudio() {
    if (tone0_ > 0 && --tone0_ == 0) sys_->apu.tone(0, 0, 0);
    if (tone1_ > 0 && --tone1_ == 0) sys_->apu.tone(1, 0, 0);
    if (mode_ == Mode::Ring || (mode_ == Mode::Leave && won_)) {
        swing_ *= 0.985f;
        if ((t_ % 18) == 0 && t_ < 70) tone(1, 196.f + 40.f * swing_, 0.08f + 0.08f * swing_, 12);
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal) {
    if (m.h < 1 || h < 1.f) return;
    float w = h * (float(m.w) / float(m.h));
    gs::Sprite s;
    s.h = int16_t(std::lround(h));
    s.w = int16_t(std::max(1, int(std::lround(w))));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal & 15);
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = int(std::strlen(s));
    hud(20 - n / 2, row, s, pal);
}

void Game::backdrop() {
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int b = 6 - y / 40;
        if (b < 1) b = 1;
        int r = y > 170 ? 2 : 1;
        sys_->vdp.lineBackdrop[y] = gs::rgb4(r, 1 + y / 90, b);
        sys_->vdp.lineFog[y] = 0;
        sys_->vdp.road[y].on = false;
    }
}

void Game::draw() {
    sys_->vdp.clearSprites();
    sys_->vdp.HUD.clear();
    sys_->vdp.A.clear();
    backdrop();
    spr(art_.beam, 160, 58, 14, PAL_BEAM);

    float amp = (mode_ == Mode::Ring || (mode_ == Mode::Leave && won_)) ? 10.f * swing_ : 0.f;
    for (int i = NB - 1; i >= 0; i--) {
        float sway = (i == lit_ || (mode_ == Mode::Ring)) ? std::sin(t_ * 0.45f + i) * (mode_ == Mode::Ring ? amp : 2.f) : 0.f;
        float h = (i == lit_) ? 62.f : 52.f;
        if (mode_ == Mode::Ring) h = 58.f;
        spr(art_.bell, bellX(i) + sway, kY, h, PAL_B0 + i);
    }
    if (mode_ == Mode::Recall || mode_ == Mode::Dead) {
        spr(art_.mark, bellX(cursor_), kY + 40.f, 12, mode_ == Mode::Dead ? PAL_ALERT : PAL_GOLD);
    }
    for (int i = 0; i < SEQ; i++) {
        int pal = i < got_ ? PAL_OK : PAL_DIM;
        if (mode_ == Mode::Dead && i == got_) pal = PAL_ALERT;
        spr(art_.pip, 132.f + float(i) * 16.f, 176, 8, pal);
    }

    hudC(1, "S3 MEMORYBELL", PAL_GOLD);
    if (mode_ == Mode::Title) {
        hudC(8, "A SHORT MEMORY", PAL_INK);
        hudC(10, "FOUR BELLS", PAL_INK);
        hudC(12, "THEN THE SAME ORDER", PAL_INK);
        hudC(18, "START", PAL_GOLD);
        hudC(24, "THIRD DEAD TRY ENDS IT", PAL_DIM);
    } else if (mode_ == Mode::Show) {
        hudC(22, "WATCH", PAL_GOLD);
    } else if (mode_ == Mode::Recall) {
        char line[32];
        std::snprintf(line, sizeof(line), "TRY %d   DEAD %d", tryNo_, dead_);
        hudC(22, line, PAL_INK);
        hudC(24, "LEFT RIGHT   A STRIKE", PAL_DIM);
    } else if (mode_ == Mode::Dead) {
        hudC(22, dead_ >= TRIES ? "THIRD TRY DIED" : "TRY DIED", PAL_ALERT);
    } else if (mode_ == Mode::Ring || (mode_ == Mode::Leave && won_) || (mode_ == Mode::Over && won_)) {
        hudC(22, "THE BELL RINGS", PAL_OK);
    } else if (mode_ == Mode::Over) {
        hudC(22, "BELL SILENT", PAL_ALERT);
        hudC(24, "START", PAL_DIM);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    sys.vdp.A.clear();
    sys.vdp.B.clear();
    sys.vdp.HUD.clear();
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(1, 1, 3));
    buildArt(sys.vdp, art_);
    rules_ = audit();
    toTitle();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (lock_ > 0) lock_--;
    if (bot_) botAct();
    else readPad();

    if (mode_ == Mode::Title) t_++;
    else if (mode_ == Mode::Show) showTick();
    else if (mode_ == Mode::Recall) {
        if (lit_ >= 0 && lock_ == 0) lit_ = -1;
        t_++;
    } else if (mode_ == Mode::Dead) {
        t_++;
        if (t_ > 50) {
            if (dead_ >= TRIES) {
                mode_ = Mode::Over;
                over_ = true;
                won_ = false;
                t_ = 0;
            } else {
                tryNo_++;
                beginShow();
            }
        }
    } else if (mode_ == Mode::Ring) {
        t_++;
        if (t_ > 90) {
            mode_ = Mode::Leave;
            t_ = 0;
        }
    } else if (mode_ == Mode::Leave) {
        t_++;
        if (t_ > 24) {
            mode_ = Mode::Over;
            over_ = true;
        }
    }

    pumpAudio();
    draw();
}

}  // namespace memorybell

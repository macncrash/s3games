#include "game/solitairebell.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace solitairebell {

const char* Game::phase() const {
    switch (mode_) {
    case Mode::Title: return "title";
    case Mode::Play: return "play";
    case Mode::Ring: return "ring";
    case Mode::Lose: return "lose";
    }
    return "?";
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.apu.setMaster(0.4f);
    // One face-up row. Build 1 through 8. A wrong card kills the try.
    const int layout[kCards] = {4, 1, 6, 2, 8, 3, 7, 5};
    bool seen[kCards + 1] = {};
    rules_ = true;
    for (int i = 0; i < kCards; i++) {
        ranks_[i] = layout[i];
        if (layout[i] < 1 || layout[i] > kCards || seen[layout[i]]) rules_ = false;
        seen[layout[i]] = true;
    }
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    rung_ = false;
    dead_ = 0;
    tryNo_ = 0;
    why_ = "";
}

void Game::resetTry() {
    for (int i = 0; i < kCards; i++) live_[i] = true;
    next_ = 1;
    built_ = 0;
    cursor_ = indexOf(1);
    if (cursor_ < 0) cursor_ = 0;
    life_ = 0;
    wait_ = 0;
}

void Game::begin() {
    rung_ = false;
    won_ = false;
    over_ = false;
    dead_ = 0;
    tryNo_ = 1;
    why_ = "PLAY";
    bellAmp_ = 0.12f;
    bellPh_ = 0;
    resetTry();
    mode_ = Mode::Play;
    sys_->apu.tone(0, 392.f, 0.06f);
}

int Game::indexOf(int rank) const {
    for (int i = 0; i < kCards; i++)
        if (live_[i] && ranks_[i] == rank) return i;
    return -1;
}

void Game::ring() {
    if (rung_ || dead_ >= kMaxDead || built_ != kCards) return;
    rung_ = true;
    won_ = true;
    tryNo_ = dead_ + 1;
    why_ = "BELL";
    bellAmp_ = 1.f;
    mode_ = Mode::Ring;
    wait_ = 0;
    sys_->apu.tone(0, 523.f, 0.12f);
    sys_->apu.tone(1, 659.f, 0.1f);
    sys_->apu.tone(2, 784.f, 0.08f);
    if (!sys_->headless) {
        sys_->rumble(0.35f, 0.7f, 160);
        sys_->setLight(255, 196, 64);
    }
}

void Game::dieTry() {
    if (rung_ || mode_ != Mode::Play) return;
    dead_++;
    why_ = "BACK";
    sys_->apu.tone(0, 140.f, 0.1f);
    sys_->apu.noiseBurst(0.14f, 700.f, 0.06f);
    if (dead_ >= kMaxDead) {
        mode_ = Mode::Lose;
        won_ = false;
        why_ = "THIRD";
        bellAmp_ = 0.02f;
        wait_ = 0;
        if (!sys_->headless) sys_->setLight(150, 28, 28);
        return;
    }
    tryNo_ = dead_ + 1;
    resetTry();
    if (!sys_->headless) sys_->setLight(120, 48, 28);
}

void Game::play(int i) {
    if (mode_ != Mode::Play || i < 0 || i >= kCards || !live_[i] || rung_) return;
    if (ranks_[i] != next_) {
        dieTry();
        return;
    }
    live_[i] = false;
    built_++;
    next_++;
    sys_->apu.tone(0, 520.f + float(built_) * 28.f, 0.06f);
    if (!sys_->headless) sys_->rumble(0.12f, 0.3f, 30);
    if (built_ == kCards) ring();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    bellPh_ += rung_ ? 0.55f : 0.08f;
    if (bellAmp_ > 0.08f) bellAmp_ *= rung_ ? 0.985f : 0.992f;

    if (mode_ == Mode::Title) {
        if (bot_ || pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) begin();
    } else if (mode_ == Mode::Play) {
        life_++;
        if (bot_) {
            if (life_ % 6 == 0) {
                int g = indexOf(next_);
                if (g >= 0) play(g);
                else dieTry();
            }
        } else {
            if (pad.pressed(gs::BTN_LEFT)) cursor_ = (cursor_ + kCards - 1) % kCards;
            if (pad.pressed(gs::BTN_RIGHT)) cursor_ = (cursor_ + 1) % kCards;
            if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_X)) play(cursor_);
        }
    } else if (mode_ == Mode::Ring) {
        wait_++;
        if (wait_ == 16) sys.apu.tone(0, 659.f, 0.08f);
        if (wait_ == 32) sys.apu.tone(0, 784.f, 0.12f);
        if (wait_ > 70) {
            over_ = true;
            if (!sys.headless) sys.quit();
        }
    } else if (mode_ == Mode::Lose) {
        wait_++;
        if (bot_) {
            if (wait_ > 20) over_ = true;
        } else if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) {
            mode_ = Mode::Title;
            dead_ = 0;
            tryNo_ = 0;
            why_ = "";
        }
    }
    paint();
}

void Game::blit(const gs::Image& img, float cx, float cy, float w, float h, int pal) {
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = img;
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::lineAt(int col, int row, const char* s, int pal) {
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

void Game::lineC(int row, const char* s, int pal) {
    int n = s ? int(std::strlen(s)) : 0;
    lineAt(20 - n / 2, row, s, pal);
}

void Game::paint() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        int shade = (y * 2) / gs::SCREEN_H;
        v.lineBackdrop[y] = gs::rgb4(0, shade + 1, shade);
    }

    blit(art_.felt, 160.f, 118.f, float(art_.felt.w), float(art_.felt.h), PAL_FELT);

    float swing = std::sin(bellPh_) * bellAmp_ * 12.f;
    float bellY = 46.f + (rung_ ? std::sin(bellPh_ * 2.f) * 1.5f : 0.f);
    blit(art_.bell, 160.f + swing, bellY, float(art_.bell.w), float(art_.bell.h), PAL_BELL);
    blit(art_.clapper, 160.f + swing * 1.4f, bellY + 10.f, float(art_.clapper.w), float(art_.clapper.h), PAL_BELL);
    blit(art_.pile, 160.f, 92.f, float(art_.pile.w), float(art_.pile.h), PAL_CARD);

    if (mode_ != Mode::Title) {
        for (int i = 0; i < kCards; i++) {
            float x = 28.f + float(i) * 36.f;
            float y = 156.f;
            bool up = live_[i];
            if (!up) {
                x = 160.f;
                y = 90.f - float(ranks_[i]) * 0.4f;
            }
            bool hot = mode_ == Mode::Play && i == cursor_ && up && !bot_;
            float cy = hot ? y - 8.f : y;
            blit(art_.card, x, cy, float(art_.card.w), float(art_.card.h), PAL_CARD);
            blit(art_.pip, x, cy + 6.f, 10.f, 10.f, PAL_PIP);
            char rank[2] = {char('0' + ranks_[i]), 0};
            int col = int(std::lround(x)) / 8 - 0;
            int row = int(std::lround(cy - 16.f)) / 8;
            if (up || mode_ != Mode::Play) lineAt(col, row < 0 ? 0 : row, rank, up ? PAL_TITLE : PAL_WIN);
        }
    }

    lineC(0, "S3 SOLITAIRE BELL", PAL_TITLE);
    if (mode_ == Mode::Title) {
        lineC(4, "RING THE BELL", PAL_INK);
        lineC(6, "BEFORE THE THIRD TRY DIES", PAL_INK);
        lineC(12, "BUILD THE RUN 1 TO 8", PAL_HINT);
        lineC(14, "A WRONG CARD KILLS THE TRY", PAL_BAD);
        lineC(16, "THREE DEAD TRIES AND IT IS OVER", PAL_BAD);
        lineC(20, "A  PLAY THE CARD", PAL_HINT);
        lineC(25, "START", PAL_TITLE);
    } else if (mode_ == Mode::Play) {
        char buf[48];
        std::snprintf(buf, sizeof buf, "TRY %d/3   BUILT %d   NEXT %d", tryNo_ > 3 ? 3 : tryNo_, built_,
                      next_ > kCards ? kCards : next_);
        lineC(2, buf, PAL_INK);
        if (why_ && std::strcmp(why_, "BACK") == 0 && life_ < 18) lineC(4, "TRY DIED", PAL_BAD);
        if (!bot_) lineC(26, "LEFT RIGHT   A PLAY", PAL_HINT);
    } else if (mode_ == Mode::Ring) {
        char buf[48];
        std::snprintf(buf, sizeof buf, "BELL ON TRY %d", tryNo_);
        lineC(3, buf, PAL_WIN);
        lineC(5, "BEFORE THE THIRD TRY DIED", PAL_WIN);
    } else if (mode_ == Mode::Lose) {
        lineC(3, "THIRD TRY DIED", PAL_BAD);
        lineC(5, "BELL SILENT", PAL_BAD);
        if (!bot_) lineC(25, "START", PAL_HINT);
    }
}

}  // namespace solitairebell

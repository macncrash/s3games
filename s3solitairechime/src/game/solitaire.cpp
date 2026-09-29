#include "game/solitaire.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace solitairechime {

const char* Game::phase() const {
    switch (mode_) {
    case Mode::Title: return "title";
    case Mode::Play: return "play";
    case Mode::Wait: return "wait";
    case Mode::Chime: return "chime";
    case Mode::Fail: return "fail";
    }
    return "?";
}

void Game::splitLive(int& h, int& m, int& s) const {
    int t = kOpenSec + playFrames_ / 60;
    if (t < 0) t = 0;
    s = t % 60;
    m = (t / 60) % 60;
    h = (t / 3600) % 12;
    if (h == 0) h = 12;
}

int Game::liveHour() const {
    int h, m, s;
    splitLive(h, m, s);
    return h;
}
int Game::liveMinute() const {
    int h, m, s;
    splitLive(h, m, s);
    return m;
}
int Game::liveSecond() const {
    int h, m, s;
    splitLive(h, m, s);
    return s;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.apu.setMaster(0.4f);
    const int layout[kCards] = {3, 1, 5, 8, 2, 7, 4, 6};
    bool seen[kCards + 1] = {};
    rules_ = true;
    for (int i = 0; i < kCards; i++) {
        ranks_[i] = layout[i];
        if (layout[i] < 1 || layout[i] > kCards || seen[layout[i]]) rules_ = false;
        seen[layout[i]] = true;
    }
    mode_ = Mode::Title;
    over_ = won_ = chimed_ = spoiled_ = frozen_ = false;
    built_ = playFrames_ = 0;
    why_ = "hour silent";
    if (bot_) begin();
    paint();
}

void Game::begin() {
    for (int i = 0; i < kCards; i++) live_[i] = true;
    next_ = 1;
    built_ = 0;
    cursor_ = indexOf(1);
    if (cursor_ < 0) cursor_ = 0;
    life_ = playFrames_ = hold_ = 0;
    over_ = won_ = chimed_ = spoiled_ = frozen_ = false;
    bellAmp_ = 0.1f;
    bellPh_ = 0;
    why_ = "PLAY";
    mode_ = Mode::Play;
    sys_->apu.tone(0, 392.f, 0.06f);
}

int Game::indexOf(int rank) const {
    for (int i = 0; i < kCards; i++)
        if (live_[i] && ranks_[i] == rank) return i;
    return -1;
}

void Game::strikeHour() {
    if (chimed_ || spoiled_ || built_ != kCards || mode_ == Mode::Play) {
        passHour(spoiled_ ? "wrong card" : "run open");
        return;
    }
    chimed_ = true;
    won_ = true;
    frozen_ = true;
    fh_ = 12;
    fm_ = 0;
    fs_ = 0;
    why_ = "CHIME";
    mode_ = Mode::Chime;
    hold_ = 48;
    bellAmp_ = 1.f;
    sys_->apu.tone(0, 523.f, 0.14f);
    sys_->apu.tone(1, 659.f, 0.1f);
    sys_->apu.tone(2, 784.f, 0.08f);
    if (!sys_->headless) {
        sys_->rumble(0.35f, 0.7f, 180);
        sys_->setLight(255, 210, 80);
    }
}

void Game::passHour(const char* why) {
    frozen_ = true;
    int h, m, s;
    splitLive(h, m, s);
    fh_ = h;
    fm_ = m;
    fs_ = s;
    won_ = false;
    chimed_ = false;
    why_ = why;
    mode_ = Mode::Fail;
    hold_ = 36;
    bellAmp_ = 0.02f;
    sys_->apu.tone(0, 110.f, 0.12f);
    if (!sys_->headless) sys_->setLight(140, 28, 28);
}

void Game::play(int i) {
    if (mode_ != Mode::Play || i < 0 || i >= kCards || !live_[i] || spoiled_) return;
    if (ranks_[i] != next_) {
        spoiled_ = true;
        why_ = "wrong card";
        sys_->apu.tone(0, 140.f, 0.1f);
        sys_->apu.noiseBurst(0.14f, 700.f, 0.06f);
        return;
    }
    live_[i] = false;
    built_++;
    next_++;
    sys_->apu.tone(0, 480.f + float(built_) * 30.f, 0.06f);
    if (!sys_->headless) sys_->rumble(0.1f, 0.25f, 28);
    if (built_ == kCards) {
        why_ = "WAIT";
        mode_ = Mode::Wait;
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    bellPh_ += chimed_ ? 0.55f : 0.08f;
    if (bellAmp_ > 0.08f && !chimed_) bellAmp_ *= 0.992f;

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) begin();
    } else if (mode_ == Mode::Play || mode_ == Mode::Wait) {
        playFrames_++;
        life_++;
        if (mode_ == Mode::Play && bot_ && !spoiled_ && (life_ % 8) == 0) {
            int g = indexOf(next_);
            if (g >= 0) play(g);
            else spoiled_ = true;
        } else if (mode_ == Mode::Play && !bot_ && !spoiled_) {
            if (pad.pressed(gs::BTN_LEFT)) cursor_ = (cursor_ + kCards - 1) % kCards;
            if (pad.pressed(gs::BTN_RIGHT)) cursor_ = (cursor_ + 1) % kCards;
            if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_X)) play(cursor_);
        }
        if (playFrames_ >= kHourFrame) strikeHour();
    } else if (mode_ == Mode::Chime || mode_ == Mode::Fail) {
        if (--hold_ <= 0) {
            over_ = true;
            if (!sys.headless && mode_ == Mode::Chime) sys.quit();
        }
        if (mode_ == Mode::Chime) {
            if (hold_ == 36) sys.apu.tone(0, 659.f, 0.1f);
            if (hold_ == 24) sys.apu.tone(0, 784.f, 0.12f);
            if (hold_ == 12) sys.apu.tone(1, 1046.f, 0.1f);
        }
        if (mode_ == Mode::Fail && !bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
            mode_ = Mode::Title;
            over_ = false;
            why_ = "hour silent";
            frozen_ = false;
            playFrames_ = 0;
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
        v.lineBackdrop[y] = gs::rgb4(0, shade + 1, shade + 1);
    }

    blit(art_.felt, 160.f, 120.f, float(art_.felt.w), float(art_.felt.h), PAL_FELT);
    blit(art_.clock, 160.f, 52.f, float(art_.clock.w), float(art_.clock.h), PAL_CLOCK);

    int sec = second();
    int minu = minute();
    float angM = (float(minu) / 60.f) * 6.28318f;
    float angS = (float(sec) / 60.f) * 6.28318f;
    blit(art_.hand, 160.f + std::sin(angM) * 6.f, 50.f - std::cos(angM) * 6.f, 4.f, 12.f, PAL_CLOCK);
    blit(art_.hand, 160.f + std::sin(angS) * 10.f, 52.f - std::cos(angS) * 8.f, 3.f, 10.f, PAL_BELL);

    float swing = std::sin(bellPh_) * bellAmp_ * 10.f;
    blit(art_.bell, 248.f + swing, 48.f, float(art_.bell.w), float(art_.bell.h), PAL_BELL);
    blit(art_.clapper, 248.f + swing * 1.3f, 56.f, float(art_.clapper.w), float(art_.clapper.h), PAL_BELL);

    if (mode_ != Mode::Title) {
        for (int i = 0; i < kCards; i++) {
            float x = 28.f + float(i) * 36.f;
            float y = 158.f;
            bool up = live_[i];
            if (!up) {
                x = 72.f + float(ranks_[i] - 1) * 22.f;
                y = 108.f;
            }
            bool hot = mode_ == Mode::Play && i == cursor_ && up && !bot_ && !spoiled_;
            float cy = hot ? y - 8.f : y;
            blit(art_.card, x, cy, float(art_.card.w), float(art_.card.h), PAL_CARD);
            blit(art_.pip, x, cy + 6.f, 10.f, 10.f, PAL_PIP);
            char rank[2] = {char('0' + ranks_[i]), 0};
            int col = int(std::lround(x)) / 8;
            int row = int(std::lround(cy - 16.f)) / 8;
            if (row < 0) row = 0;
            lineAt(col, row, rank, up ? PAL_TITLE : PAL_WIN);
        }
    }

    lineC(0, "S3 SOLITAIRE CHIME", PAL_TITLE);
    char clock[16];
    std::snprintf(clock, sizeof clock, "%d:%02d:%02d", hour(), minute(), second());
    lineC(1, clock, chimed_ ? PAL_WIN : PAL_INK);

    if (mode_ == Mode::Title) {
        lineC(4, "THE HOUR HAS TO CHIME", PAL_INK);
        lineC(6, "BUILD 1 TO 8 THEN WAIT", PAL_HINT);
        lineC(8, "A RUN BEFORE TWELVE IS SHORT", PAL_BAD);
        lineC(10, "A WRONG CARD STAYS SILENT", PAL_BAD);
        lineC(20, "A  PLAY THE CARD", PAL_HINT);
        lineC(25, "START", PAL_TITLE);
    } else if (mode_ == Mode::Play || mode_ == Mode::Wait) {
        char buf[48];
        std::snprintf(buf, sizeof buf, "BUILT %d  NEXT %d", built_, next_ > kCards ? kCards : next_);
        lineC(3, buf, PAL_INK);
        if (mode_ == Mode::Wait) lineC(5, "WAIT FOR THE HOUR", PAL_HINT);
        if (spoiled_) lineC(5, "WRONG CARD", PAL_BAD);
        if (!bot_ && mode_ == Mode::Play && !spoiled_) lineC(26, "LEFT RIGHT   A PLAY", PAL_HINT);
    } else if (mode_ == Mode::Chime) {
        lineC(3, "THE HOUR CHIMES", PAL_WIN);
        lineC(5, "12:00:00", PAL_WIN);
    } else if (mode_ == Mode::Fail) {
        lineC(3, "HOUR SILENT", PAL_BAD);
        lineC(5, why_ ? why_ : "", PAL_BAD);
        if (!bot_) lineC(25, "START", PAL_HINT);
    }
}

}  // namespace solitairechime

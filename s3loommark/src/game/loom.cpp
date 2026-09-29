#include "game/loom.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace loommark {

namespace {
constexpr float kDt = 1.f / 60.f;
}

void Game::toTitle() {
    mode_ = Mode::Title;
    over_ = won_ = finished_ = markWoven_ = pendingWin_ = caught_ = false;
    picks_ = misses_ = row_ = col_ = 0;
    dir_ = 1;
    dwell_ = beat_ = shake_ = 0;
    clock_ = 0;
    for (int c = 0; c < kCols; c++) attempt_[c] = 0;
    for (int r = 0; r < kRows; r++)
        for (int c = 0; c < kCols; c++) cloth_[r][c] = 0;
}

void Game::begin() {
    toTitle();
    mode_ = Mode::Weave;
    dwell_ = dwellFor();
    blip(440.f, 0.05f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.apu.setMaster(0.8f);
    sys.apu.silence();
    toTitle();
}

bool Game::rowMatches() const {
    for (int c = 0; c < kCols; c++)
        if (attempt_[c] != markAt(row_, c)) return false;
    return true;
}

void Game::lockRow() {
    for (int c = 0; c < kCols; c++) cloth_[row_][c] = attempt_[c];
    row_++;
    picks_ = row_;
    for (int c = 0; c < kCols; c++) attempt_[c] = 0;
    col_ = 0;
    caught_ = false;
    beat_ = bot_ ? 8 : 18;
    mode_ = Mode::Beat;
    pendingWin_ = row_ >= kRows;
    if (pendingWin_) markWoven_ = true;
    sys_->apu.noiseBurst(0.18f, 900.f, 18.f);
    blip(pendingWin_ ? 660.f : 220.f, 0.07f);
}

void Game::snarl() {
    misses_++;
    for (int c = 0; c < kCols; c++) attempt_[c] = 0;
    col_ = 0;
    caught_ = false;
    shake_ = 12;
    sys_->apu.noiseBurst(0.12f, 2400.f, 22.f);
    blip(140.f, 0.06f);
    if (misses_ >= 4) lose();
    else dwell_ = dwellFor();
}

void Game::win() {
    mode_ = Mode::Win;
    won_ = finished_ = over_ = true;
    markWoven_ = true;
    picks_ = kRows;
    chord();
}

void Game::lose() {
    mode_ = Mode::Lose;
    over_ = true;
    won_ = finished_ = false;
}

void Game::commitEnd() {
    int warp = dir_ > 0 ? col_ : (kCols - 1 - col_);
    attempt_[warp] = caught_ ? 1 : 0;
    caught_ = false;
    col_++;
    blip(520.f + float(warp) * 18.f, 0.03f);
    if (col_ >= kCols) {
        if (rowMatches()) lockRow();
        else snarl();
    } else dwell_ = dwellFor();
}

void Game::blip(float freq, float vol) {
    if (!sys_) return;
    sys_->apu.tone(0, freq, vol);
    toneT_ = 0.07f;
}

void Game::chord() {
    if (!sys_) return;
    sys_->apu.tone(0, 523.f, 0.1f);
    sys_->apu.tone(1, 659.f, 0.08f);
    sys_->apu.tone(2, 784.f, 0.07f);
    toneT_ = 0.45f;
}

void Game::decayAudio() {
    if (!sys_ || toneT_ <= 0.f) return;
    toneT_ -= kDt;
    if (toneT_ <= 0.f) {
        sys_->apu.tone(0, 0, 0);
        sys_->apu.tone(1, 0, 0);
        sys_->apu.tone(2, 0, 0);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    clock_ += kDt;
    if (shake_ > 0) shake_--;
    decayAudio();
    const gs::Pad& p = sys.pad;
    const bool action = p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C) || p.pressed(gs::BTN_TURBO);
    const bool held = p.down(gs::BTN_A) || p.down(gs::BTN_C) || p.down(gs::BTN_TURBO);
    const bool start = p.pressed(gs::BTN_START);
    const bool back = p.pressed(gs::BTN_MODE);

    if (mode_ == Mode::Title) {
        if (bot_) {
            if (clock_ > 0.35f) begin();
        } else if (back) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        } else if (start || action) begin();
    } else if (mode_ == Mode::Pause) {
        if (start || action) mode_ = held_;
        else if (back) toTitle();
    } else if (mode_ == Mode::Win || mode_ == Mode::Lose) {
        if (!bot_ && (start || action)) begin();
        else if (!bot_ && back) toTitle();
    } else if (!bot_ && (start || back)) {
        held_ = mode_;
        mode_ = Mode::Pause;
    } else if (mode_ == Mode::Weave) {
        int warp = dir_ > 0 ? col_ : (kCols - 1 - col_);
        if (bot_) {
            if (markAt(row_, warp)) caught_ = true;
        } else if (held) caught_ = true;
        if (--dwell_ <= 0) commitEnd();
    } else if (mode_ == Mode::Beat) {
        if (--beat_ <= 0) {
            if (pendingWin_) win();
            else {
                dir_ = -dir_;
                mode_ = Mode::Weave;
                dwell_ = dwellFor();
            }
        }
    }
    draw();
}

void Game::spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool flip) {
    if (!sys_ || w < 1.f || h < 1.f || img.w == 0) return;
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = img;
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
    int n = s ? int(std::strlen(s)) : 0;
    hud(20 - n / 2, row, s, pal);
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.hudEnabled = true;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        if (y < 168) v.lineBackdrop[y] = gs::rgb4(3 + (y > 80), 2, 2);
        else v.lineBackdrop[y] = gs::rgb4(5, 3, 2);
    }
    v.setFogColor(gs::rgb4(3, 2, 2));

    const Mode view = mode_ == Mode::Pause ? held_ : mode_;
    const int ox = shake_ ? ((shake_ & 2) ? 2 : -2) : 0;
    const bool live = view == Mode::Weave || view == Mode::Beat;
    const int shownRow = (view == Mode::Win || view == Mode::Lose) ? kRows : (live ? row_ : 0);

    auto clothRows = [&](int rows) {
        for (int r = 0; r < rows && r < kRows; r++) {
            float y = float(pickY(r));
            spr(art_.cream, float(warpX(4) + ox), y, float((kCols - 1) * kWarpGap + 14), 7.f, PAL_CLOTH);
            for (int c = 0; c < kCols; c++)
                if (cloth_[r][c]) spr(art_.mark, float(warpX(c) + ox), y, 12.f, 8.f, PAL_CLOTH);
        }
    };

    if (view == Mode::Title) spr(art_.logo, 160.f, 96.f, float(art_.logo.w), float(art_.logo.h), PAL_TITLE);
    if (view == Mode::Win) spr(art_.fin, 160.f, 92.f, float(art_.fin.w), float(art_.fin.h), PAL_WIN);

    int warpNow = dir_ > 0 ? col_ : (kCols - 1 - col_);
    if (view == Mode::Weave && row_ < kRows) {
        float y = float(pickY(row_));
        spr(art_.cream, float(warpX(4) + ox), y, float((kCols - 1) * kWarpGap + 14), 7.f, PAL_CLOTH);
        for (int c = 0; c < kCols; c++) {
            bool on = attempt_[c] || (c == warpNow && caught_);
            if (on) spr(art_.mark, float(warpX(c) + ox), y, 12.f, 8.f, PAL_CLOTH);
        }
        float t = dwellFor() > 1 ? 1.f - float(dwell_) / float(dwellFor()) : 1.f;
        if (t < 0.f) t = 0.f;
        int prev = dir_ > 0 ? warpNow - 1 : warpNow + 1;
        float x0 = prev < 0 || prev >= kCols ? float(warpX(warpNow) - dir_ * 28) : float(warpX(prev));
        float sx = x0 + (float(warpX(warpNow)) - x0) * t;
        spr(art_.shuttle, sx + float(ox), y - 10.f, float(art_.shuttle.w), float(art_.shuttle.h), PAL_SHUTTLE, dir_ < 0);
    }

    float reedY = 78.f;
    if (view == Mode::Beat && beat_ > 0) {
        float u = float(beat_) / 18.f;
        float target = float(pickY(row_ > 0 ? row_ - 1 : 0)) - 8.f;
        reedY = 78.f + (target - 78.f) * (1.f - u);
    }
    if (view != Mode::Title) spr(art_.reed, float(warpX(4) + ox), reedY, 132.f, 10.f, PAL_WOOD);

    spr(art_.skein, 30.f, 78.f, 18.f, 16.f, PAL_YARN);
    spr(art_.skein, 30.f, 112.f, 16.f, 14.f, PAL_CLOTH);

    clothRows(shownRow == kRows ? kRows : shownRow);

    if (view == Mode::Weave && row_ < kRows) {
        for (int c = 0; c < kCols; c++)
            if (!(attempt_[c] || (c == warpNow && caught_))) {
                // kept so the inlay sits on the cream already drawn above
            }
    }

    float hy = 64.f + std::sin(clock_ * 2.f) * 1.5f;
    for (int c = 0; c < kCols; c++)
        spr(art_.heddle, float(warpX(c) + ox), hy + float((c & 1) ? 4 : -4), 8.f, 14.f, PAL_WOOD);
    for (int c = 0; c < kCols; c++)
        spr(art_.warp, float(warpX(c) + ox), 108.f, 3.f, 124.f, PAL_CLOTH);

    spr(art_.frame, float(kFrameX + art_.frame.w / 2 + ox), float(kFrameY + art_.frame.h / 2), float(art_.frame.w),
        float(art_.frame.h), PAL_WOOD);

    for (int r = 0; r < kRows; r++) {
        for (int c = 0; c < kCols; c++) {
            bool on = markAt(r, c) != 0;
            bool hot = live && r == row_;
            int pal = on ? (hot ? PAL_ALERT : PAL_GOLD) : PAL_DIM;
            spr(art_.dot, 286.f + float(c) * 5.f, 36.f + float(r) * 5.f, on ? 4.f : 3.f, on ? 4.f : 3.f, pal);
        }
    }
    if (live && row_ < kRows) {
        spr(art_.dot, 280.f, 36.f + float(row_) * 5.f, 3.f, 3.f, PAL_WIN);
    }

    char buf[40];
    hud(1, 1, "S3 LOOMMARK", PAL_TITLE);
    if (view == Mode::Title) {
        hudC(18, "A SHORT LOOM", PAL_INK);
        hudC(20, "WEAVE THE MARK", PAL_GOLD);
        hudC(23, "START", PAL_INK);
        hudC(25, "A CATCHES THE RED", PAL_DIM);
    } else if (view == Mode::Win) {
        hudC(16, "FINISHED MARK", PAL_WIN);
        hudC(24, "THE LOOM IS DONE", PAL_INK);
    } else if (view == Mode::Lose) {
        hudC(16, "CLOTH CUT", PAL_ALERT);
        hudC(24, "START RETRIES", PAL_INK);
    } else {
        std::snprintf(buf, sizeof buf, "PICK %d/%d", std::min(picks_ + (view == Mode::Weave ? 1 : 0), kRows), kRows);
        hud(1, 24, buf, PAL_INK);
        std::snprintf(buf, sizeof buf, "MISS %d", misses_);
        hud(1, 26, buf, misses_ ? PAL_ALERT : PAL_DIM);
        if (view == Mode::Weave) hud(24, 26, "A CATCH", PAL_GOLD);
        if (mode_ == Mode::Pause) hudC(16, "PAUSED", PAL_GOLD);
        else if (view == Mode::Beat && pendingWin_) hudC(16, "BEAT IN", PAL_WIN);
    }
    hud(30, 1, "MARK", PAL_DIM);
}

}  // namespace loommark

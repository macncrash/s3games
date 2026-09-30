#include "game/scoregold.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace scoregold {
namespace {

constexpr int SHEET_N = 6;
constexpr int FAULTS = 3;
constexpr int DUE[SHEET_N] = {3, 5, 4, 2, 6, 4};

const char* KIND_NAME[] = {"BARE", "CREAM", "GOLD"};

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    toTitle();
}

void Game::clearSheet() {
    idx_ = 0;
    lines_ = 0;
    faults_ = 0;
    score_ = 0;
    bare_ = 0;
    golds_ = 0;
    cream_ = 0;
    dueSum_ = 0;
    for (int i = 0; i < SHEET_N; i++) dueSum_ += DUE[i];
    markN_ = 0;
    cursor_ = 0;
    wait_ = 0;
    repL_ = 0;
    repR_ = 0;
    flashT_ = 0;
    flashK_ = 0;
    holdN_ = 0;
    filed_ = false;
    finisherGold_ = false;
    over_ = false;
    won_ = false;
    why_ = "";
    melStep_ = -1;
}

void Game::toTitle() {
    clearSheet();
    mode_ = Mode::Title;
    age_ = 0;
}

void Game::startSheet() {
    clearSheet();
    mode_ = Mode::Play;
    wait_ = 8;
}

void Game::tally(int& face, int& tot) const {
    face = 0;
    tot = 0;
    for (int i = 0; i < markN_; i++) {
        face += 1;
        tot += (marks_[i] == Gold) ? 2 : 1;
    }
}

int Game::dueOf() const {
    if (idx_ < 0 || idx_ >= SHEET_N) return 0;
    return DUE[idx_];
}

void Game::blip(float freq, int frames) {
    beepF_ = freq;
    beepV_ = 0.08f;
    beepN_ = frames;
}

void Game::stamp() {
    if (mode_ != Mode::Play) return;
    if (markN_ >= 8) return;
    marks_[markN_++] = cursor_;
    blip(cursor_ == Gold ? 660.f : 420.f, 4);
}

void Game::undo() {
    if (mode_ != Mode::Play || markN_ <= 0) return;
    markN_--;
    blip(180.f, 4);
}

void Game::post() {
    if (mode_ != Mode::Play) return;
    int face = 0, tot = 0;
    tally(face, tot);
    int due = dueOf();
    bool goldLast = markN_ > 0 && marks_[markN_ - 1] == Gold;
    if (tot == due && due > 0 && face < due && goldLast) {
        lines_++;
        int g = 0, c = 0;
        for (int i = 0; i < markN_; i++) {
            if (marks_[i] == Gold) g++;
            else if (marks_[i] == Cream) c++;
        }
        golds_ += g;
        cream_ += c;
        score_ += tot;
        bare_ += face;
        finisherGold_ = true;
        markN_ = 0;
        flashK_ = 1;
        flashT_ = 22;
        blip(880.f, 8);
        sys_->apu.tone(1, 1320.f, 0.05f);
        if (lines_ >= SHEET_N) offerFile();
        else {
            idx_ = lines_;
            mode_ = Mode::Hold;
            holdN_ = 0;
        }
        return;
    }
    faults_++;
    flashK_ = 3;
    flashT_ = 28;
    markN_ = 0;
    blip(110.f, 10);
    sys_->apu.noiseBurst(0.12f, 480.f, 0.2f);
    sys_->rumble(0.4f, 0.2f, 70);
    if (faults_ >= FAULTS) loseSheet("cream or a short line stopped the sheet");
}

void Game::offerFile() {
    mode_ = Mode::File;
    why_ = "only the gold counts double";
}

void Game::fileSheet() {
    if (!(lines_ >= SHEET_N && finisherGold_ && golds_ > 0 && score_ == dueSum_ && bare_ < score_)) {
        loseSheet("filed before the gold double");
        return;
    }
    filed_ = true;
    winSheet();
}

void Game::winSheet() {
    mode_ = Mode::Win;
    over_ = true;
    won_ = true;
    why_ = "only the gold counts double";
    melStep_ = 0;
    melWait_ = 0;
}

void Game::loseSheet(const char* why) {
    if (mode_ == Mode::Lose || mode_ == Mode::Win) return;
    mode_ = Mode::Lose;
    over_ = true;
    won_ = false;
    filed_ = false;
    why_ = why;
    sys_->rumble(0.6f, 0.3f, 140);
}

void Game::readInput() {
    gs::Pad& pad = sys_->pad;
    if (pad.pressed(gs::BTN_MODE)) {
        if (mode_ == Mode::Title) {
            if (!bot_) sys_->quit();
        } else if (mode_ == Mode::Pause) toTitle();
        else if (mode_ == Mode::Win || mode_ == Mode::Lose) toTitle();
        else {
            held_ = mode_;
            mode_ = Mode::Pause;
        }
        return;
    }
    if (pad.pressed(gs::BTN_START)) {
        if (mode_ == Mode::Title || mode_ == Mode::Win || mode_ == Mode::Lose) startSheet();
        else if (mode_ == Mode::File) fileSheet();
        else if (mode_ == Mode::Pause) mode_ = held_;
        else {
            held_ = mode_;
            mode_ = Mode::Pause;
        }
        return;
    }
    if (mode_ != Mode::Play) return;
    if (pad.pressed(gs::BTN_LEFT)) cursor_ = (cursor_ + 2) % 3;
    else if (pad.axisX < -0.4f && repL_ == 0) {
        cursor_ = (cursor_ + 2) % 3;
        repL_ = 8;
    }
    if (pad.pressed(gs::BTN_RIGHT)) cursor_ = (cursor_ + 1) % 3;
    else if (pad.axisX > 0.4f && repR_ == 0) {
        cursor_ = (cursor_ + 1) % 3;
        repR_ = 8;
    }
    if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) stamp();
    if (pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_X)) post();
    if (pad.pressed(gs::BTN_Y) || pad.pressed(gs::BTN_Z)) undo();
}

void Game::driveBot() {
    for (int i = 0; i < gs::BTN_COUNT; i++) sys_->pad.keys[i] = false;
    if (wait_ > 0 || over_) return;
    auto tap = [&](gs::Button b) { sys_->pad.keys[b] = true; };
    if (mode_ == Mode::Title) {
        if (age_ > 20) tap(gs::BTN_START);
        return;
    }
    if (mode_ == Mode::Hold || mode_ == Mode::Pause) return;
    if (mode_ == Mode::File) {
        tap(gs::BTN_START);
        return;
    }
    if (mode_ != Mode::Play) return;
    int due = dueOf();
    int needBare = std::max(0, due - 2);
    if (markN_ < needBare) {
        if (cursor_ != Bare) tap(gs::BTN_LEFT);
        else tap(gs::BTN_A);
    } else if (markN_ == needBare) {
        if (cursor_ != Gold) tap(gs::BTN_RIGHT);
        else tap(gs::BTN_A);
    } else {
        tap(gs::BTN_B);
    }
    wait_ = 2;
}

void Game::logic() {
    if (wait_ > 0) wait_--;
    if (repL_ > 0) repL_--;
    if (repR_ > 0) repR_--;
    if (flashT_ > 0) flashT_--;
    if (mode_ == Mode::Hold) {
        if (++holdN_ > 18) {
            mode_ = Mode::Play;
            wait_ = 6;
        }
    }
}

void Game::audio() {
    if (beepN_ > 0) {
        sys_->apu.tone(0, beepF_, beepV_);
        beepN_--;
        if (beepN_ == 0) sys_->apu.tone(0, 0, 0);
    }
    if (melStep_ >= 0) {
        if (melWait_ > 0) {
            melWait_--;
        } else {
            static const float mel[] = {523.f, 659.f, 784.f, 1046.f};
            if (melStep_ < 4) {
                sys_->apu.tone(2, mel[melStep_], 0.07f);
                melWait_ = 8;
                melStep_++;
            } else {
                sys_->apu.tone(2, 0, 0);
                melStep_ = -1;
            }
        }
    }
}

void Game::lights() {
    if (mode_ == Mode::Win) sys_->setLight(220, 180, 40);
    else if (mode_ == Mode::Lose) sys_->setLight(180, 30, 20);
    else if (cursor_ == Gold) sys_->setLight(180, 140, 20);
    else sys_->setLight(40, 80, 30);
}

void Game::hud(int col, int row, const char* s) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], PAL_HUD));
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 2.f || m.h < 1 || m.w < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::sprI(const gs::Image& img, float cx, float cy, float h, int pal) {
    if (h < 1.f || img.h < 1 || img.w < 1) return;
    float w = h * float(img.w) / float(img.h);
    gs::Sprite s;
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = img;
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::solid(float x, float y, float w, float h, int pal) {
    if (w < 1.f || h < 1.f) return;
    gs::Sprite s;
    s.img = art_.solid;
    s.x = int16_t(std::lround(x));
    s.y = int16_t(std::lround(y));
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.clear();
    v.B.clear();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.road[y].on = false;
        int g = 2 + (y * 3) / gs::SCREEN_H;
        v.lineBackdrop[y] = gs::rgb4(1, g, 1);
        v.lineFog[y] = 0;
    }
    spr(art_.board, 168.f, 118.f, 150.f, PAL_WOOD);
    spr(art_.ledge, 160.f, 206.f, 26.f, PAL_WOOD);

    if (mode_ == Mode::Title) {
        sprI(art_.title, 160.f, 36.f, float(art_.title.h), PAL_HUD);
        sprI(art_.rule, 160.f, 64.f, float(art_.rule.h) * 0.7f, PAL_GOLD);
        hud(6, 14, "A STAMP   B POST   START");
        hud(4, 16, "ONLY THE GOLD COUNTS DOUBLE");
        spr(art_.gold, 70.f, 140.f, 28.f, PAL_GOLD);
        spr(art_.cream, 120.f, 142.f, 22.f, PAL_CREAM);
        spr(art_.bare, 160.f, 142.f, 26.f, PAL_CHALK);
        spr(art_.pen, 220.f, 150.f, 14.f, PAL_INK);
    } else if (mode_ == Mode::Win || mode_ == Mode::File) {
        sprI(art_.filed, 160.f, 40.f, float(art_.filed.h), PAL_OK);
        char buf[64];
        std::snprintf(buf, sizeof(buf), "SCORE %d  BARE %d  GOLD %d", score_, bare_, golds_);
        hud(3, 10, buf);
        hud(4, 12, "ONLY THE GOLD COUNTS DOUBLE");
        if (mode_ == Mode::File) hud(8, 16, "START FILES THE SHEET");
        for (int i = 0; i < golds_ && i < 12; i++)
            spr(art_.gold, 40.f + float(i % 6) * 48.f, 140.f + float(i / 6) * 28.f, 22.f, PAL_GOLD);
    } else if (mode_ == Mode::Lose) {
        sprI(art_.shorted, 160.f, 48.f, float(art_.shorted.h), PAL_BAD);
        hud(4, 12, why_);
        hud(8, 16, "START TRIES THE SHEET");
    } else {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "LINE %d/%d  DUE %d", idx_ + 1, SHEET_N, dueOf());
        hud(2, 1, buf);
        int face = 0, tot = 0;
        tally(face, tot);
        std::snprintf(buf, sizeof(buf), "COUNT %d  FACE %d  %s", tot, face, KIND_NAME[cursor_]);
        hud(2, 3, buf);
        std::snprintf(buf, sizeof(buf), "SCORE %d  FAULT %d", score_, faults_);
        hud(2, 25, buf);
        hud(2, 26, "L R INK  A STAMP  B POST");
        float y = 78.f;
        for (int i = 0; i < markN_; i++) {
            float x = 90.f + float(i) * 22.f;
            if (marks_[i] == Gold) spr(art_.gold, x, y, 20.f, PAL_GOLD);
            else if (marks_[i] == Cream) spr(art_.cream, x, y, 18.f, PAL_CREAM);
            else spr(art_.bare, x, y, 22.f, PAL_CHALK);
        }
        const gs::Mipped* ink = &art_.bare;
        int pal = PAL_CHALK;
        if (cursor_ == Cream) {
            ink = &art_.cream;
            pal = PAL_CREAM;
        } else if (cursor_ == Gold) {
            ink = &art_.gold;
            pal = PAL_GOLD;
        }
        float bob = (age_ / 8 % 2) ? 2.f : 0.f;
        spr(*ink, 48.f, 150.f + bob, 32.f, pal);
        spr(art_.pen, 48.f, 176.f, 12.f, PAL_INK);
        if (flashT_ > 0 && (flashT_ / 4) % 2 == 0) {
            int palF = flashK_ == 1 ? PAL_OK : PAL_BAD;
            solid(70.f, 60.f, 180.f, 4.f, palF);
        }
        if (mode_ == Mode::Pause) hud(14, 14, "PAUSED");
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    age_++;
    readInput();
    if (bot_) driveBot();
    logic();
    audio();
    lights();
    draw();
}

}  // namespace scoregold

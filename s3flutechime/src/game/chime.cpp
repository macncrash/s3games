#include "game/chime.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace flutechime {
namespace {

constexpr float kLeft = 36.f;
constexpr float kSpeed = 2.2f;
constexpr float kWindow = 9.f;
constexpr float kMarkX[kNotes] = {64.f, 104.f, 144.f, 184.f, 224.f, 264.f};
constexpr int kPhrase[kNotes] = {0, 2, 1, 3, 0, 2};
constexpr float kHz[4] = {261.6f, 293.7f, 329.6f, 392.0f};
constexpr char kName[4][2] = {{'C', 0}, {'D', 0}, {'E', 0}, {'G', 0}};
constexpr int kHour = 12 * 3600;

float clampf(float v, float a, float b) {
    if (v < a) return a;
    if (v > b) return b;
    return v;
}

}  // namespace

bool Game::phraseDone() const { return clean_ && played_ == kNotes; }

float Game::headX() const { return kLeft + phase_; }

void Game::tone(float freq, float vol, int frames) {
    if (!sys_) return;
    sys_->apu.tone(0, freq, vol);
    sys_->apu.tone(1, freq * 2.f, vol * 0.28f);
    tone_ = frames;
}

void Game::readClock() {
    int t = kOpenSec + playFrames_ / 60;
    if (t < 0) t = 0;
    if (chimed_ || (mode_ == Mode::Fail && hour_ == 12 && minute_ == 0)) t = kHour;
    second_ = t % 60;
    minute_ = (t / 60) % 60;
    int h = (t / 3600) % 12;
    hour_ = h == 0 ? 12 : h;
}

void Game::begin() {
    played_ = 0;
    finger_ = kPhrase[0];
    phase_ = 0;
    hold_ = 0;
    breath_ = 0;
    shake_ = 0;
    playFrames_ = 0;
    chimeStep_ = 0;
    over_ = won_ = chimed_ = false;
    clean_ = true;
    why_ = "hour silent";
    hour_ = 11;
    minute_ = 59;
    second_ = 50;
    for (int i = 0; i < kNotes; i++) did_[i] = false;
    mode_ = Mode::Play;
    readClock();
    tone(220.f, 0.05f, 4);
}

void Game::blow(bool hit) {
    if (played_ >= kNotes) return;
    breath_ = 12;
    shake_ = 4;
    if (!hit) {
        clean_ = false;
        why_ = "the breath missed the note";
        mode_ = Mode::Miss;
        hold_ = 36;
        won_ = chimed_ = false;
        if (sys_) sys_->apu.noiseBurst(0.2f, 90.f, 0.16f);
        return;
    }
    tone(kHz[kPhrase[played_]], 0.12f, 12);
    did_[played_] = true;
    played_++;
    if (sys_) sys_->rumble(0.06f, 0.12f, 36);
    if (played_ >= kNotes) {
        mode_ = Mode::Wait;
        hold_ = 0;
        why_ = "phrase held";
    } else {
        mode_ = Mode::Hold;
        hold_ = 8;
        finger_ = kPhrase[played_];
    }
}

void Game::early() {
    if (chimed_ || mode_ == Mode::Chime || mode_ == Mode::Over) return;
    won_ = chimed_ = false;
    clean_ = false;
    why_ = "left before the hour";
    mode_ = Mode::Fail;
    hold_ = 0;
    if (sys_) sys_->apu.noiseBurst(0.18f, 70.f, 0.16f);
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
    chimeStep_ = -1;
}

void Game::passHour(const char* why) {
    why_ = why;
    hour_ = 12;
    minute_ = 0;
    second_ = 0;
    won_ = chimed_ = false;
    mode_ = Mode::Fail;
    hold_ = 0;
    if (sys_) sys_->apu.tone(1, 64.f, 0.14f);
}

void Game::stepPlay() {
    if (played_ >= kNotes) {
        mode_ = Mode::Wait;
        return;
    }
    if (bot_) finger_ = kPhrase[played_];
    else if (sys_) {
        const gs::Pad& pad = sys_->pad;
        if (pad.pressed(gs::BTN_UP) || pad.pressed(gs::BTN_RIGHT)) finger_ = (finger_ + 1) & 3;
        if (pad.pressed(gs::BTN_DOWN) || pad.pressed(gs::BTN_LEFT)) finger_ = (finger_ + 3) & 3;
    }

    const float mark = kMarkX[played_];
    const bool inWin = std::fabs(headX() - mark) <= kWindow;
    bool tap = false;
    if (bot_) tap = inWin && finger_ == kPhrase[played_];
    else if (sys_) {
        const gs::Pad& pad = sys_->pad;
        tap = pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_B);
    }
    if (tap) {
        blow(inWin && finger_ == kPhrase[played_]);
        return;
    }
    phase_ += kSpeed;
    if (headX() > mark + kWindow) blow(false);
}

void Game::image(const gs::Mipped& m, float left, float top, float h, int pal, bool flip) {
    if (!sys_ || h < 1.5f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    auto q = [](float v) { return int16_t(std::lround(clampf(v, -400.f, 800.f))); };
    s.x = q(left);
    s.y = q(top);
    s.w = int16_t(std::max(1, int(std::lround(w))));
    s.h = int16_t(std::max(1, int(std::lround(h))));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::spr(const gs::Mipped& m, float cx, float bottom, float h, int pal, bool flip) {
    if (m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    image(m, cx - w * 0.5f, bottom - h, h, pal, flip);
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.road[y].on = false;
        v.lineFog[y] = 0;
        float u = y / float(gs::SCREEN_H - 1);
        int r = 1 + int(u * 2.f);
        int g = 1 + int((1.f - u) * 2.f);
        int b = 5 + int((1.f - u) * 6.f);
        if (mode_ == Mode::Chime || (mode_ == Mode::Over && won_)) {
            r = 6 + int(u * 4.f);
            g = 5;
            b = 2;
        }
        v.lineBackdrop[y] = gs::rgb4(r, g, b);
    }
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!sys_ || row < 0 || row > 27) return;
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

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    const float ccx = 286.f;
    const float ccy = 36.f;
    int shown = chimed_ || mode_ == Mode::Chime ? 0 : second_;
    float ang = float(shown) * 0.10472f - 1.5708f;
    spr(art_.hand, ccx + std::cos(ang) * 10.f, ccy + std::sin(ang) * 10.f + 8.f, 10.f, PAL_CLOCK);
    float mang = (minute_ == 0 && hour_ == 12 ? 0.f : 59.f) * 0.10472f - 1.5708f;
    spr(art_.hand, ccx + std::cos(mang) * 6.f, ccy + std::sin(mang) * 6.f + 6.f, 7.f, PAL_GOLD);
    spr(art_.clock, ccx, ccy + 20.f, 40.f, PAL_CLOCK);

    float swing = 0.f;
    if (mode_ == Mode::Chime) swing = std::sin(float(hold_) * 0.45f) * 6.f;
    spr(art_.bell, 28.f + swing, 78.f, 28.f, mode_ == Mode::Chime || chimed_ ? PAL_GOLD : PAL_WOOD);

    spr(art_.lamp, 48.f, 132.f, 36.f, PAL_LAMP);
    spr(art_.stand, 78.f, 160.f, 40.f, PAL_WOOD);
    float jig = shake_ ? ((sys_->frame & 1) ? 1.f : -1.f) : 0.f;
    spr(art_.player, 150.f + jig, 168.f, 60.f, PAL_PLAYER);
    image(art_.flute, 128.f, 108.f + (breath_ > 0 ? -2.f : 0.f), 12.f, PAL_WOOD);
    image(art_.staff, 28.f, 186.f, 24.f, PAL_HUD);

    int show = played_ < kNotes ? played_ : kNotes - 1;
    if (mode_ == Mode::Title) show = int(sys_->frame / 20) % kNotes;
    for (int i = 0; i < kNotes; i++) {
        const gs::Mipped& img = did_[i] ? art_.noteOn : art_.note;
        float ny = 210.f - float(kPhrase[i]) * 3.f;
        spr(img, kMarkX[i], ny, did_[i] ? 18.f : 15.f, did_[i] ? PAL_GOLD : PAL_CREAM);
    }
    if (mode_ == Mode::Play || mode_ == Mode::Hold || mode_ == Mode::Title || mode_ == Mode::Wait) {
        float bx = mode_ == Mode::Title ? (kLeft + std::fmod(float(sys_->frame) * 1.4f, 240.f)) : headX();
        if (mode_ == Mode::Wait) bx = kMarkX[kNotes - 1];
        spr(art_.breath, bx, 196.f, 14.f, PAL_GOLD);
    }

    char buf[72];
    char clk[16];
    std::snprintf(clk, sizeof clk, "%d:%02d:%02d", hour_, minute_, second_);
    int f = int(sys_->frame);
    if (mode_ == Mode::Title) {
        hudC(1, "S3 FLUTE CHIME", PAL_GOLD);
        hudC(3, clk, PAL_HUD);
        hudC(21, "A SHORT FLUTE", PAL_HUD);
        hudC(22, "THE HOUR HAS TO CHIME", PAL_GOLD);
        hudC(23, "PLAY THE PHRASE AND WAIT", PAL_CREAM);
        hudC(24, "LEAVING EARLY STAYS QUIET", PAL_HUD);
        if ((f & 16) == 0) hudC(26, "PRESS START", PAL_GOLD);
        const char* ver = S3_VERSION_STRING;
        hud(40 - int(std::strlen(ver)), 0, ver, PAL_DIM);
        return;
    }
    if (mode_ == Mode::Chime || (mode_ == Mode::Over && won_)) {
        hudC(1, "12:00:00", PAL_GOLD);
        hudC(3, "THE HOUR CHIMES", PAL_GOLD);
        std::snprintf(buf, sizeof buf, "NOTES %d/%d", played_, kNotes);
        hudC(23, buf, PAL_HUD);
        hudC(24, "THE PHRASE HELD", PAL_GOLD);
        return;
    }
    if (mode_ == Mode::Fail || mode_ == Mode::Miss || (mode_ == Mode::Over && !won_)) {
        hudC(1, clk, PAL_BAD);
        hudC(3, "THE HOUR STAYS QUIET", PAL_BAD);
        hudC(23, why_[0] ? why_ : "HOUR SILENT", PAL_HUD);
        if ((f & 16) == 0 && !bot_) hudC(26, "START", PAL_GOLD);
        return;
    }

    hud(1, 1, clk, second_ >= 54 ? PAL_GOLD : PAL_HUD);
    std::snprintf(buf, sizeof buf, "NOTE %d/%d  FINGER %s", show + 1, kNotes, kName[finger_ & 3]);
    hudC(21, buf, PAL_CREAM);
    if (mode_ == Mode::Pause) hudC(23, "PAUSED", PAL_GOLD);
    else if (mode_ == Mode::Wait) hudC(23, "PHRASE HELD  WAIT FOR THE HOUR", PAL_GOLD);
    else hudC(23, "BLOW ON THE MARK", PAL_HUD);
    hudC(25, "ARROWS FINGER  C BLOW  A IS EARLY", PAL_DIM);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(1, 1, 3));
    sys.apu.setMaster(0.85f);
    mode_ = Mode::Title;
    over_ = won_ = chimed_ = false;
    played_ = 0;
    clean_ = true;
    why_ = "hour silent";
    readClock();
    if (bot_) begin();
    draw();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (breath_ > 0) breath_--;
    if (shake_ > 0) shake_--;
    if (tone_ > 0 && --tone_ == 0) {
        sys.apu.tone(0, 0, 0);
        sys.apu.tone(1, 0, 0);
        sys.apu.tone(2, 0, 0);
    }

    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C)) begin();
        if (!bot_ && pad.pressed(gs::BTN_MODE)) sys.quit();
    } else if (mode_ == Mode::Play || mode_ == Mode::Hold || mode_ == Mode::Wait) {
        playFrames_++;
        readClock();
        if (mode_ == Mode::Play) {
            if (!bot_ && pad.pressed(gs::BTN_START)) {
                held_ = Mode::Play;
                mode_ = Mode::Pause;
            } else stepPlay();
        } else if (mode_ == Mode::Hold) {
            if (--hold_ <= 0) mode_ = played_ >= kNotes ? Mode::Wait : Mode::Play;
        }
        if (mode_ == Mode::Play || mode_ == Mode::Hold || mode_ == Mode::Wait) {
            int t = kOpenSec + playFrames_ / 60;
            if (t >= kHour) {
                if (phraseDone()) strike();
                else passHour(played_ < kNotes ? "phrase short" : "phrase broken");
            }
        }
        if (!bot_ && (mode_ == Mode::Play || mode_ == Mode::Hold || mode_ == Mode::Wait) && pad.pressed(gs::BTN_A))
            early();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = held_;
    } else if (mode_ == Mode::Chime) {
        hold_++;
        int step = hold_ / 6;
        if (step != chimeStep_ && step >= 0 && step < 4) {
            chimeStep_ = step;
            static const float notes[] = {392.f, 494.f, 587.f, 784.f};
            sys.apu.tone(0, notes[step], 0.16f);
            sys.apu.tone(1, notes[step] * 0.5f, 0.08f);
            tone_ = 10;
        }
        if (hold_ > 4 * 6 + 24) {
            over_ = true;
            mode_ = Mode::Over;
        }
    } else if (mode_ == Mode::Miss || mode_ == Mode::Fail) {
        hold_++;
        if (hold_ > (bot_ ? 8 : 40)) {
            over_ = true;
            mode_ = Mode::Over;
        }
    } else if (mode_ == Mode::Over) {
        if (!bot_ && !won_ && pad.pressed(gs::BTN_START)) begin();
    }

    if (!bot_ && pad.pressed(gs::BTN_MODE) && mode_ != Mode::Title) sys.quit();
    draw();
}

}  // namespace flutechime

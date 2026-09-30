#include "game/flutegold.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace flutegold {
namespace {

constexpr float kLeft = 36.f;
constexpr float kSpeed = 1.7f;
constexpr float kWindow = 9.f;
constexpr float kMarkX[kNotes] = {64.f, 104.f, 144.f, 184.f, 224.f, 264.f};
// C E D G C E. Finger 0..3 selects the pitch.
constexpr int kPhrase[kNotes] = {0, 2, 1, 3, 0, 2};
constexpr float kHz[4] = {261.6f, 293.7f, 329.6f, 392.0f};
constexpr char kName[4][2] = {{'C', 0}, {'D', 0}, {'E', 0}, {'G', 0}};

static_assert(kGolds * kGoldFace * 2 + kCreams * kCreamFace >= kLine, "doubled gold clears the line");
static_assert(kGolds * kGoldFace + kCreams * kCreamFace < kLine, "bare faces stay short");
static_assert(kGolds * kGoldFace * 2 < kLine, "gold alone does not buy the line");
static_assert((kGolds - 1) * kGoldFace * 2 + kCreams * kCreamFace < kLine, "leave on the last gold");
static_assert(kNotes == kGolds + kCreams, "the short flute is six notes");

float clampf(float v, float a, float b) {
    if (v < a) return a;
    if (v > b) return b;
    return v;
}

}  // namespace

bool Game::open() const {
    const int face = golds_ * kGoldFace * 2 + cream_ * kCreamFace;
    return !spoiled_ && finisherGold_ && played_ == kNotes && golds_ == kGolds && cream_ == kCreams && score_ == face &&
           bare_ < kLine && score_ >= kLine;
}

float Game::headX() const { return kLeft + phase_; }

void Game::tone(float freq, float vol, int frames) {
    if (!sys_) return;
    sys_->apu.tone(0, freq, vol);
    sys_->apu.tone(1, freq * 2.f, vol * 0.28f);
    tone_ = frames;
}

void Game::begin() {
    score_ = bare_ = golds_ = cream_ = played_ = 0;
    finger_ = kPhrase[0];
    phase_ = 0;
    hold_ = 0;
    breath_ = 0;
    shake_ = 0;
    over_ = won_ = finisherGold_ = spoiled_ = false;
    why_ = "";
    for (int i = 0; i < kNotes; i++) did_[i] = false;
    mode_ = Mode::Play;
    tone(220.f, 0.05f, 4);
}

void Game::blow(bool hit) {
    if (played_ >= kNotes) return;
    const bool gold = noteGold(played_);
    breath_ = 12;
    shake_ = 4;
    if (!hit) {
        spoiled_ = true;
        finisherGold_ = false;
        why_ = "the breath missed the note";
        mode_ = Mode::Miss;
        hold_ = 40;
        if (sys_) sys_->apu.noiseBurst(0.2f, 90.f, 0.16f);
        return;
    }
    const int add = gold ? kGoldFace * 2 : kCreamFace;
    if (!gold && score_ + add >= kLine) {
        spoiled_ = true;
        finisherGold_ = false;
        cream_++;
        played_++;
        did_[played_ - 1] = true;
        why_ = "cream cannot buy the line";
        mode_ = Mode::Miss;
        hold_ = 40;
        if (sys_) sys_->apu.noiseBurst(0.16f, 140.f, 0.14f);
        return;
    }
    bare_ += gold ? kGoldFace : kCreamFace;
    score_ += add;
    if (gold) {
        golds_++;
        finisherGold_ = true;
        tone(kHz[kPhrase[played_]], 0.12f, 12);
    } else {
        cream_++;
        finisherGold_ = false;
        tone(kHz[kPhrase[played_]] * 0.5f, 0.08f, 10);
    }
    did_[played_] = true;
    played_++;
    if (sys_) sys_->rumble(gold ? 0.08f : 0.03f, gold ? 0.16f : 0.05f, 40);
    if (played_ >= kNotes) {
        mode_ = Mode::Ready;
        hold_ = 0;
    } else {
        mode_ = Mode::Hold;
        hold_ = 10;
        finger_ = kPhrase[played_];
    }
}

void Game::leave() {
    won_ = open();
    over_ = true;
    mode_ = Mode::Over;
    if (!won_ && why_[0] == 0) why_ = "the double was not the leave";
    if (won_) {
        why_ = "only the gold counts double";
        tone(523.f, 0.14f, 20);
        if (sys_) {
            sys_->apu.tone(2, 659.f, 0.08f);
            sys_->setLight(220, 170, 40);
        }
    } else if (sys_) {
        sys_->apu.noiseBurst(0.22f, 70.f, 0.18f);
    }
}

void Game::stepPlay() {
    if (played_ >= kNotes) {
        mode_ = Mode::Ready;
        return;
    }
    if (bot_) finger_ = kPhrase[played_];
    else if (sys_) {
        const gs::Pad& pad = sys_->pad;
        if (pad.pressed(gs::BTN_UP) || pad.pressed(gs::BTN_RIGHT)) finger_ = (finger_ + 1) & 3;
        if (pad.pressed(gs::BTN_DOWN) || pad.pressed(gs::BTN_LEFT)) finger_ = (finger_ + 3) & 3;
    }

    const float x = headX();
    const float mark = kMarkX[played_];
    const bool inWin = std::fabs(x - mark) <= kWindow;
    bool tap = false;
    if (bot_) tap = inWin && finger_ == kPhrase[played_];
    else if (sys_) {
        const gs::Pad& pad = sys_->pad;
        tap = pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_B);
    }
    if (tap) {
        const bool ok = inWin && finger_ == kPhrase[played_];
        blow(ok);
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
        int r = 1 + int(u * 3.f);
        int g = 1 + int((1.f - u) * 2.f);
        int b = 6 + int((1.f - u) * 5.f);
        if (mode_ == Mode::Over && won_) {
            r = 8 + int(u * 4.f);
            g = 4;
            b = 1;
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

    spr(art_.lamp, 22.f, 150.f, 40.f, PAL_LAMP);
    spr(art_.lamp, 300.f, 150.f, 40.f, PAL_LAMP);
    spr(art_.stand, 78.f, 168.f, 44.f, PAL_WOOD);
    float jig = shake_ ? ((sys_->frame & 1) ? 1.f : -1.f) : 0.f;
    spr(art_.player, 150.f + jig, 172.f, 64.f, PAL_PLAYER);
    image(art_.flute, 128.f, 112.f + (breath_ > 0 ? -2.f : 0.f), 12.f, PAL_WOOD);
    image(art_.staff, 28.f, 178.f, 26.f, PAL_HUD);

    int show = played_ < kNotes ? played_ : kNotes - 1;
    if (mode_ == Mode::Title) show = int(sys_->frame / 20) % kNotes;
    for (int i = 0; i < kNotes; i++) {
        const bool gold = noteGold(i);
        int pal = did_[i] ? (gold ? PAL_GOLD : PAL_CREAM) : (gold ? PAL_GOLD : PAL_CREAM);
        const gs::Mipped& img = did_[i] ? art_.noteOn : art_.note;
        float ny = 204.f - float(kPhrase[i]) * 3.f;
        spr(img, kMarkX[i], ny, did_[i] ? 18.f : 15.f, pal);
    }
    if (mode_ == Mode::Play || mode_ == Mode::Hold || mode_ == Mode::Title || mode_ == Mode::Ready) {
        float bx = mode_ == Mode::Title ? (kLeft + std::fmod(float(sys_->frame) * 1.4f, 240.f)) : headX();
        if (mode_ == Mode::Ready) bx = kMarkX[kNotes - 1];
        spr(art_.breath, bx, 190.f, 14.f, noteGold(show) ? PAL_GOLD : PAL_CREAM);
    }

    char buf[72];
    int f = int(sys_->frame);
    if (mode_ == Mode::Title) {
        hudC(1, "S3 FLUTE GOLD", PAL_GOLD);
        hudC(21, "A SHORT FLUTE", PAL_HUD);
        hudC(22, "ONLY THE GOLD COUNTS DOUBLE", PAL_GOLD);
        hudC(23, "CREAM KEEPS ITS FACE", PAL_CREAM);
        hudC(24, "LEAVE WHEN THAT IS TRUE", PAL_HUD);
        if ((f & 16) == 0) hudC(26, "PRESS START", PAL_GOLD);
        const char* ver = S3_VERSION_STRING;
        hud(40 - int(std::strlen(ver)), 0, ver, PAL_DIM);
        return;
    }
    if (mode_ == Mode::Over && won_) {
        hudC(1, "LEFT ON THE DOUBLE", PAL_GOLD);
        std::snprintf(buf, sizeof buf, "SCORE %d  BARE %d", score_, bare_);
        hudC(22, buf, PAL_GOLD);
        hudC(23, "ONLY THE GOLD COUNTED DOUBLE", PAL_HUD);
        hudC(24, "THE SHORT FLUTE IS DONE", PAL_GOLD);
        return;
    }
    if (mode_ == Mode::Over || mode_ == Mode::Miss) {
        hudC(1, "STILL AT THE FLUTE", PAL_BAD);
        std::snprintf(buf, sizeof buf, "SCORE %d  BARE %d", score_, bare_);
        hudC(22, buf, PAL_BAD);
        hudC(23, why_[0] ? why_ : "THE LINE IS OPEN", PAL_HUD);
        if ((f & 16) == 0) hudC(26, "START", PAL_GOLD);
        return;
    }

    std::snprintf(buf, sizeof buf, "SCORE %d", score_);
    hud(1, 1, buf, PAL_GOLD);
    std::snprintf(buf, sizeof buf, "BARE %d", bare_);
    hud(12, 1, buf, PAL_HUD);
    std::snprintf(buf, sizeof buf, "LINE %d", kLine);
    hud(28, 1, buf, PAL_DIM);
    std::snprintf(buf, sizeof buf, "NOTE %d/%d  FINGER %s", show + 1, kNotes, kName[finger_ & 3]);
    hudC(21, buf, noteGold(show) ? PAL_GOLD : PAL_CREAM);
    std::snprintf(buf, sizeof buf, "GOLDS %d  CREAM %d", golds_, cream_);
    hudC(22, buf, PAL_HUD);
    if (mode_ == Mode::Pause) hudC(24, "PAUSED", PAL_GOLD);
    else if (open() || mode_ == Mode::Ready) hudC(24, "LEAVE  THE GOLD IS DOUBLE", PAL_GOLD);
    else if (noteGold(show)) hudC(24, "BLOW THE GOLD", PAL_GOLD);
    else hudC(24, "CREAM IS NOT A DOUBLE", PAL_CREAM);
    hudC(26, "ARROWS FINGER  C BLOW  A LEAVE", PAL_DIM);
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
    }

    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C)) begin();
        if (!bot_ && pad.pressed(gs::BTN_MODE)) sys.quit();
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            held_ = Mode::Play;
            mode_ = Mode::Pause;
        } else stepPlay();
    } else if (mode_ == Mode::Hold) {
        if (--hold_ <= 0) mode_ = Mode::Play;
    } else if (mode_ == Mode::Ready) {
        if (bot_ || pad.pressed(gs::BTN_A)) leave();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = held_;
    } else if (mode_ == Mode::Miss) {
        if (--hold_ <= 0 || (!bot_ && pad.pressed(gs::BTN_START))) begin();
    } else if (mode_ == Mode::Over) {
        if (!bot_ && !won_ && pad.pressed(gs::BTN_START)) begin();
    }

    if (!bot_ && mode_ != Mode::Title && mode_ != Mode::Over && mode_ != Mode::Miss && pad.pressed(gs::BTN_A) &&
        mode_ != Mode::Pause) {
        leave();
    }
    if (!bot_ && pad.pressed(gs::BTN_MODE) && mode_ != Mode::Title) sys.quit();

    draw();
}

}  // namespace flutegold

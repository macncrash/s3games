#include "game/memory.h"

#include <cstdio>
#include <cstring>

namespace memorytape {
namespace {

constexpr int PITCH = 28;
constexpr int OX = (gs::SCREEN_W - LEN * PITCH) / 2;
constexpr int TAPE_Y = 52;
constexpr int DRAWER_Y = 112;
constexpr int BANK_Y = 176;
constexpr int BANK_OX = (gs::SCREEN_W - KINDS * PITCH) / 2;
constexpr int STUDY_FRAMES = 70;

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.7f);
    phase_ = Phase::Title;
    t_ = 0;
    filled_ = 0;
    cursor_ = 0;
    tries_ = 0;
    won_ = false;
    over_ = false;
    paused_ = false;
    showTape_ = true;
    for (int i = 0; i < LEN; i++) drawer_[i] = -1;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (beep_ > 0 && --beep_ == 0) sys.apu.tone(0, 0, 0);
    if (paused_) {
        if (sys.pad.pressed(gs::BTN_START)) paused_ = false;
        draw();
        return;
    }
    update();
    draw();
}

int Game::marker() const {
    if (phase_ == Phase::Title) return 0;
    if (phase_ == Phase::Study) return 1;
    if (phase_ == Phase::Recall) return 2;
    if (phase_ == Phase::Seal) return 3;
    return 4;
}

bool Game::matched() const {
    if (filled_ < LEN) return false;
    for (int i = 0; i < LEN; i++)
        if (drawer_[i] != art_.tapeSeq[i]) return false;
    return true;
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.18f);
    beep_ = 5;
}

void Game::place() {
    if (filled_ >= LEN) return;
    drawer_[filled_++] = cursor_;
    blip(180.f + float(cursor_) * 40.f);
}

void Game::check() {
    tries_++;
    if (matched()) {
        phase_ = Phase::Seal;
        t_ = 0;
        showTape_ = true;
        return;
    }
    filled_ = 0;
    for (int i = 0; i < LEN; i++) drawer_[i] = -1;
    showTape_ = true;
    phase_ = Phase::Study;
    t_ = 0;
    blip(90.f);
}

void Game::update() {
    const gs::Pad& p = sys_->pad;
    t_++;
    if (phase_ == Phase::Title) {
        showTape_ = true;
        if (bot_ ? t_ > 18 : (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C))) {
            phase_ = Phase::Study;
            t_ = 0;
            filled_ = 0;
            for (int i = 0; i < LEN; i++) drawer_[i] = -1;
            showTape_ = true;
        }
        return;
    }
    if (phase_ == Phase::Study) {
        showTape_ = true;
        int need = bot_ ? 24 : STUDY_FRAMES;
        if (t_ > need) {
            phase_ = Phase::Recall;
            t_ = 0;
            showTape_ = false;
        }
        return;
    }
    if (phase_ == Phase::Seal) {
        showTape_ = true;
        if (t_ > (bot_ ? 12 : 40)) {
            phase_ = Phase::Victory;
            t_ = 0;
            won_ = true;
            blip(523.f);
        }
        return;
    }
    if (phase_ == Phase::Victory) {
        if (t_ > (bot_ ? 8 : 90)) over_ = true;
        return;
    }

    if (!bot_ && p.pressed(gs::BTN_START)) {
        paused_ = true;
        return;
    }

    if (bot_) {
        int want = art_.tapeSeq[filled_];
        if (cursor_ < want) cursor_++;
        else if (cursor_ > want) cursor_--;
        else place();
        if (filled_ >= LEN) check();
        return;
    }

    if (p.pressed(gs::BTN_LEFT) && cursor_ > 0) cursor_--;
    if (p.pressed(gs::BTN_RIGHT) && cursor_ < KINDS - 1) cursor_++;
    if (p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C)) place();
    if (p.pressed(gs::BTN_B) && filled_ > 0) {
        drawer_[--filled_] = -1;
        blip(140.f);
    }
    if (filled_ >= LEN) check();
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::spr(const gs::Image& img, int x, int y, int w, int h, int pal) {
    gs::Sprite s;
    s.img = img;
    s.x = int16_t(x);
    s.y = int16_t(y);
    s.w = int16_t(w);
    s.h = int16_t(h);
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::backdrop() {
    for (int y = 0; y < gs::SCREEN_H; y++) {
        sys_->vdp.lineBackdrop[y] = gs::rgb4(1 + y / 80, 2, 4 + y / 90);
        sys_->vdp.lineFog[y] = 0;
        sys_->vdp.road[y].on = false;
    }
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.A.clear();
    vdp.B.clear();
    vdp.HUD.clear();
    backdrop();

    spr(art_.tape, OX - 8, TAPE_Y - 6, LEN * PITCH + 16, 36, PAL_GLYPH);
    spr(art_.drawer, OX - 10, DRAWER_Y - 8, LEN * PITCH + 20, 40, PAL_GLYPH);

    for (int i = 0; i < LEN; i++) {
        int x = OX + i * PITCH;
        if (showTape_ || phase_ == Phase::Seal || phase_ == Phase::Victory)
            spr(art_.glyph[art_.tapeSeq[i]], x + 2, TAPE_Y, 22, 22, PAL_GLYPH);
        else
            spr(art_.slot, x + 1, TAPE_Y - 1, 24, 24, PAL_GLYPH);
        if (drawer_[i] >= 0)
            spr(art_.glyph[drawer_[i]], x + 2, DRAWER_Y, 22, 22, PAL_GLYPH);
        else
            spr(art_.slot, x + 1, DRAWER_Y - 1, 24, 24, PAL_GLYPH);
    }

    if (phase_ == Phase::Recall || phase_ == Phase::Title) {
        for (int i = 0; i < KINDS; i++)
            spr(art_.glyph[i], BANK_OX + i * PITCH + 2, BANK_Y, 22, 22, PAL_GLYPH);
        if (phase_ == Phase::Recall)
            spr(art_.cursor, BANK_OX + cursor_ * PITCH, BANK_Y + 24, 26, 6, PAL_GLYPH);
    }

    if (phase_ == Phase::Title) {
        hudC(2, "S3 MEMORY TAPE", PAL_GOLD);
        hudC(4, "THE DRAWER HAS TO MATCH THE TAPE", PAL_CREAM);
        hudC(24, "START", PAL_LEAF);
    } else if (phase_ == Phase::Study) {
        hudC(2, "STUDY THE TAPE", PAL_GOLD);
    } else if (phase_ == Phase::Recall) {
        hudC(2, "FILL THE DRAWER", PAL_CREAM);
        if (paused_) hudC(8, "PAUSED", PAL_GOLD);
    } else if (phase_ == Phase::Seal || phase_ == Phase::Victory) {
        hudC(2, "THE DRAWER MATCHES THE TAPE", PAL_LEAF);
    }
}

}  // namespace memorytape

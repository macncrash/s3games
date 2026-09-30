#include "game/press.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace pressmark {
namespace {

constexpr float kGoldLo = 0.70f;
constexpr float kGoldHi = 0.82f;
constexpr float kSheetOk = 4.5f;

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    buildSound();
}

void Game::buildSound() {
    sys_->apu.setMaster(0.45f);
    gs::FMPatch tone;
    tone.alg = 0;
    tone.vol = 0.28f;
    tone.op[0] = {1, 1, 0.01f, 0.18f, 0.45f, 0.25f, 0};
    tone.op[1] = {2, 0.35f, 0.01f, 0.22f, 0.2f, 0.3f, 0};
    tone.op[2] = {1, 0.2f, 0.02f, 0.3f, 0.25f, 0.35f, 0};
    tone.op[3] = {1, 0.45f, 0.01f, 0.2f, 0.4f, 0.2f, 0};
    sys_->apu.setPatch(0, tone);
    sys_->apu.setPatch(1, tone);
}

void Game::blip(float freq) { sys_->apu.keyOn(0, freq, 0.2f); }

void Game::thunk() {
    sys_->apu.noiseBurst(0.35f, 900.f, 0.08f);
    sys_->apu.keyOn(1, 90.f, 0.3f);
}

void Game::fanfare() {
    sys_->apu.keyOn(0, 392.f, 0.26f);
    sys_->apu.keyOn(1, 523.f, 0.22f);
}

bool Game::press(gs::Button b) const { return !bot_ && sys_->pad.pressed(b); }

bool Game::held(gs::Button b) const { return !bot_ && sys_->pad.down(b); }

float Game::steer() const {
    if (bot_) return sheet_ > 0.8f ? -1.f : (sheet_ < -0.8f ? 1.f : 0.f);
    float x = sys_->pad.axisX;
    if (held(gs::BTN_LEFT)) x -= 1.f;
    if (held(gs::BTN_RIGHT)) x += 1.f;
    return std::clamp(x, -1.f, 1.f);
}

float Game::sweep() const {
    float u = std::fmod(t_, 1.7f) / 1.7f;
    return u < 0.5f ? u * 2.f : (1.f - u) * 2.f;
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
    int n = 0;
    while (s[n]) n++;
    hud(std::max(0, (40 - n) / 2), row, s, pal);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::backdrop() {
    gs::VDP& vdp = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H - 1);
        int r = 4 + int((1.f - u) * 3);
        int g = 3 + int(u * 2);
        int b = 3 + int((1.f - u) * 2);
        if (finished_) {
            r = std::min(15, r + 2);
            g = std::min(15, g + 2);
        }
        vdp.lineBackdrop[y] = gs::rgb4(r, g, b);
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const float dt = 1.f / 60.f;
    t_ += dt;
    if (msgT_ > 0) msgT_ -= dt;
    sys.vdp.clearSprites();
    sys.vdp.HUD.clear();

    if (mode_ == Mode::Title) {
        bool go = bot_ ? t_ > 0.45f : (press(gs::BTN_START) || press(gs::BTN_A));
        if (go) {
            mode_ = Mode::Set;
            t_ = 0;
            msg_ = "SET THE SHEET";
            msgT_ = 2.f;
            blip(520.f);
        }
    } else if (mode_ == Mode::Set) {
        if (!locked_) {
            sheet_ = std::clamp(sheet_ + steer() * 42.f * dt, -40.f, 40.f);
            bool on = std::fabs(sheet_) <= kSheetOk;
            bool act = bot_ ? (on && t_ > 0.15f) : press(gs::BTN_A);
            if (act) {
                if (on) {
                    locked_ = true;
                    sheet_ = 0;
                    mode_ = Mode::Ink;
                    roll_ = 0;
                    t_ = 0;
                    msg_ = "INK THE FORM";
                    msgT_ = 2.f;
                    blip(640.f);
                } else {
                    msg_ = "OFF THE MARK";
                    msgT_ = 1.f;
                    blip(180.f);
                }
            }
        }
    } else if (mode_ == Mode::Ink) {
        roll_ += dt * 1.35f;
        float s = std::sin(roll_);
        bool window = std::fabs(s) < 0.22f;
        bool act = bot_ ? (window && roll_ > 2.6f) : press(gs::BTN_A);
        if (act) {
            if (window) {
                inked_ = true;
                mode_ = Mode::Pull;
                t_ = 0.15f;
                msg_ = "PULL ON THE GOLD";
                msgT_ = 2.f;
                blip(700.f);
            } else {
                msg_ = "THIN INK";
                msgT_ = 0.8f;
                blip(160.f);
            }
        }
    } else if (mode_ == Mode::Pull) {
        float g = sweep();
        bool in = g >= kGoldLo && g <= kGoldHi;
        bool act = false;
        if (bot_) {
            act = in && t_ > 0.4f;
        } else {
            act = press(gs::BTN_A);
        }
        if (act) {
            pulls_++;
            if (in && inked_ && locked_) {
                finished_ = true;
                mode_ = Mode::Slam;
                slam_ = 0;
                msg_ = "MARK CLOSED";
                msgT_ = 2.f;
                thunk();
            } else {
                tries_--;
                mode_ = Mode::Slam;
                slam_ = 0;
                finished_ = false;
                msg_ = g < kGoldLo ? "SHORT  OPEN MARK" : "HARD  OPEN MARK";
                msgT_ = 1.4f;
                thunk();
            }
        }
    } else if (mode_ == Mode::Slam) {
        slam_ += dt;
        if (slam_ > 0.55f) {
            if (finished_) {
                mode_ = Mode::Win;
                won_ = true;
                over_ = true;
                fanfare();
                msg_ = "FINISHED MARK";
                msgT_ = 6.f;
            } else if (tries_ <= 0) {
                mode_ = Mode::Lose;
                won_ = false;
                over_ = true;
                msg_ = "MARK STILL OPEN";
                msgT_ = 4.f;
            } else {
                mode_ = Mode::Pull;
                t_ = 0.1f;
            }
        }
    } else if (mode_ == Mode::Win || mode_ == Mode::Lose) {
        if (!bot_ && press(gs::BTN_START)) {
            mode_ = Mode::Set;
            over_ = false;
            won_ = false;
            finished_ = false;
            inked_ = false;
            locked_ = false;
            tries_ = 3;
            pulls_ = 0;
            sheet_ = -36.f;
            t_ = 0;
            msg_ = "SET THE SHEET";
            msgT_ = 2.f;
        }
    }

    backdrop();
    draw();
}

void Game::draw() {
    float platenY = 108.f;
    if (mode_ == Mode::Slam) {
        float u = std::min(1.f, slam_ / 0.22f);
        float down = u < 0.55f ? u / 0.55f : 1.f - (u - 0.55f) / 0.45f;
        platenY = 108.f + down * 28.f;
    } else if (mode_ == Mode::Win) {
        platenY = 96.f;
    }

    float rollX = 168.f + std::sin(roll_) * 28.f;
    bool showRoll = mode_ == Mode::Ink || mode_ == Mode::Title;

    if (finished_ && (mode_ == Mode::Win || mode_ == Mode::Slam)) spr(art_.mark, 168.f, 132.f, 26.f, PAL_WIN);

    if (showRoll) spr(art_.roller, rollX, 124.f, 16.f, PAL_INK);
    spr(art_.platen, 168.f, platenY, 18.f, PAL_IRON);

    int sheetPal = finished_ && mode_ == Mode::Win ? PAL_WIN : PAL_PAPER;
    if (mode_ != Mode::Title) spr(art_.sheet, 168.f + sheet_, 148.f, 36.f, sheetPal);
    spr(art_.type, 168.f, 156.f, 22.f, PAL_INK);
    spr(art_.bed, 168.f, 164.f, 34.f, PAL_WOOD);
    spr(art_.frame, 168.f, 118.f, 150.f, PAL_WOOD);
    float figX = 48.f;
    if (mode_ == Mode::Pull || mode_ == Mode::Slam) figX = 58.f;
    spr(art_.figure, figX, 158.f, 64.f, PAL_MAN, mode_ == Mode::Slam);

    if (mode_ == Mode::Pull || mode_ == Mode::Slam) {
        spr(art_.gauge, 286.f, 112.f, 90.f, PAL_BRASS);
        float g = sweep();
        float ny = 148.f - g * 72.f;
        spr(art_.needle, 286.f, ny, 7.f, PAL_WIN);
    }

    hudC(1, "S3 PRESSMARK", PAL_HUD);
    if (mode_ == Mode::Title) {
        hudC(3, "ONE IMPRESSION", PAL_HUD);
        hudC(24, "A START", PAL_HUD);
        hudC(25, "LEFT RIGHT SET  A INK AND PULL", PAL_HUD);
    } else if (mode_ == Mode::Set) {
        hudC(3, std::fabs(sheet_) <= kSheetOk ? "ON THE MARK" : "SLIDE TO THE CROSS", PAL_HUD);
    } else if (mode_ == Mode::Ink) {
        hudC(3, "A WHEN THE ROLLER COVERS", PAL_HUD);
    } else if (mode_ == Mode::Pull) {
        hudC(3, "A ON THE GOLD LINE", PAL_HUD);
    } else if (mode_ == Mode::Win) {
        hudC(3, "THE MARK IS FINISHED", PAL_HUD);
    } else if (mode_ == Mode::Lose) {
        hudC(3, "STILL OPEN", PAL_HUD);
    }

    char line[40];
    std::snprintf(line, sizeof(line), "TRIES %d", tries_);
    hud(1, 26, line, PAL_HUD);
    if (msgT_ > 0 && msg_) hudC(22, msg_, finished_ ? PAL_HUD : PAL_HUD);
}

}  // namespace pressmark

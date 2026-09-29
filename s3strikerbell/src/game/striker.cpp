#include "game/striker.h"

#include <cmath>

namespace striker {

static constexpr float kBase = 178.f;
static constexpr float kBell = 28.f;
static constexpr float kTrack = kBase - kBell;
static constexpr int kTowerX = 142;
static constexpr int kSweet = 56;  // triangle phase that clears the bell

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.5f);
    sys.apu.setEcho(0.18f, 0.35f, 0.2f);
    begin();
}

void Game::begin() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    tries_ = 0;
    age_ = 0;
    phase_ = 0;
    meter_ = 0;
    rising_ = true;
    power_ = 0;
    puck_ = 0;
    puckV_ = 0;
    ding_ = false;
    bellFlash_ = 0;
}

void Game::chime() {
    gs::FMPatch p;
    p.alg = 5;
    p.vol = 0.45f;
    p.echo = 0.6f;
    p.op[0].mul = 1.f;
    p.op[0].level = 1.f;
    p.op[0].ar = 0.01f;
    p.op[0].dr = 0.9f;
    p.op[0].sl = 0.f;
    p.op[0].rr = 1.4f;
    p.op[1].mul = 2.76f;
    p.op[1].level = 0.35f;
    p.op[1].ar = 0.01f;
    p.op[1].dr = 0.5f;
    p.op[1].sl = 0.f;
    p.op[1].rr = 0.8f;
    for (int i = 2; i < 4; i++) p.op[i] = p.op[1];
    sys_->apu.setPatch(0, p);
    sys_->apu.keyOn(0, 784.f, 0.5f);
    sys_->apu.keyOn(1, 1176.f, 0.22f);
}

void Game::thud() {
    sys_->apu.noiseBurst(0.35f, 1800.f, 0.12f);
    sys_->apu.tone(0, 90.f, 0.12f);
}

void Game::tickAudio() {
    if (mode_ != Mode::Flight) {
        if (mode_ != Mode::Win) sys_->apu.tone(0, 0, 0);
        return;
    }
    float f = 140.f + puck_ * 640.f;
    sys_->apu.tone(0, f, 0.08f);
}

void Game::swing() {
    if (tries_ >= 3) return;
    tries_++;
    power_ = meter_;
    puck_ = 0;
    puckV_ = 0.045f + power_ * 0.09f;
    ding_ = false;
    mode_ = Mode::Flight;
    age_ = 0;
    sys_->apu.noiseBurst(0.2f, 900.f, 0.05f);
}

void Game::tick(float dt) {
    (void)dt;
    age_++;
    const gs::Pad& pad = sys_->pad;
    bool tap = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_START);

    if (mode_ == Mode::Title) {
        if (bot_ && age_ > 20) tap = true;
        if (tap) {
            tries_ = 0;
            puck_ = 0;
            mode_ = Mode::Ready;
            age_ = 0;
            phase_ = 0;
        }
        return;
    }
    if (mode_ == Mode::Win || mode_ == Mode::Lose) {
        if (bellFlash_ > 0) bellFlash_--;
        if (bot_ && age_ > 40) {
            over_ = true;
            won_ = mode_ == Mode::Win;
        }
        if (!bot_ && tap && age_ > 30) begin();
        return;
    }

    if (mode_ == Mode::Ready || mode_ == Mode::Meter) {
        phase_ = (phase_ + 1) % 120;
        rising_ = phase_ < 60;
        meter_ = rising_ ? phase_ / 60.f : (120 - phase_) / 60.f;
        mode_ = Mode::Meter;
        bool go = tap;
        if (bot_ && rising_ && phase_ >= kSweet) go = true;
        if (go) swing();
        return;
    }

    if (mode_ == Mode::Flight) {
        puckV_ -= 0.0045f;
        puck_ += puckV_;
        if (puck_ < 0.f) puck_ = 0.f;
        if (puck_ >= 1.f) {
            puck_ = 1.f;
            puckV_ = 0;
            ding_ = true;
            bellFlash_ = 36;
            chime();
            mode_ = Mode::Win;
            age_ = 0;
            return;
        }
        if (puckV_ <= 0.f && puck_ < 1.f) {
            mode_ = Mode::Mark;
            age_ = 0;
            thud();
        }
        return;
    }

    if (mode_ == Mode::Mark) {
        if (puck_ > 0.f) puck_ -= 0.02f;
        if (puck_ < 0.f) puck_ = 0.f;
        if (age_ > 36) {
            if (tries_ >= 3) {
                mode_ = Mode::Lose;
                age_ = 0;
            } else {
                mode_ = Mode::Ready;
                age_ = 0;
                phase_ = 0;
            }
        }
    }
}

void Game::spr(const gs::Image& img, float x, float y, int w, int h, int pal) {
    gs::Sprite s;
    s.img = img;
    s.x = int16_t(x);
    s.y = int16_t(y);
    s.w = int16_t(w);
    s.h = int16_t(h);
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::label(const gs::Image& img, float x, float y) { spr(img, x, y, img.w, img.h, PAL_INK); }

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int r = 2 + (y < 140 ? 0 : (y - 140) / 28);
        int g = 1;
        int b = 5 - y / 50;
        if (b < 2) b = 2;
        vdp.lineBackdrop[y] = gs::rgb4(r, g, b);
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }
    vdp.B.scroll(0, 0);
    vdp.clearSprites();

    if (mode_ == Mode::Title || mode_ == Mode::Win || mode_ == Mode::Lose) {
        float x = 160.f - art_.title.w * 0.5f;
        label(art_.title, x, mode_ == Mode::Title ? 28.f : 18.f);
    }
    if (mode_ == Mode::Title) {
        label(art_.sub, 160.f - art_.sub.w * 0.5f, 64.f);
        label(art_.hint, 160.f - art_.hint.w * 0.5f, 80.f);
        if ((age_ / 20) & 1) label(art_.swing, 160.f - art_.swing.w * 0.5f, 108.f);
    } else if (mode_ == Mode::Win) {
        int pal = (bellFlash_ / 4) & 1 ? PAL_WIN : PAL_GOLD;
        spr(art_.ding, 160.f - art_.ding.w * 0.5f, 58.f, art_.ding.w, art_.ding.h, pal);
    } else if (mode_ == Mode::Lose) {
        label(art_.dead, 160.f - art_.dead.w * 0.5f, 56.f);
    } else if (mode_ == Mode::Mark && ((age_ / 6) & 1)) {
        label(art_.miss, 36.f, 150.f);
    }

    if (tries_ >= 1 && tries_ <= 3) spr(art_.digit[tries_], 18.f, 16.f, art_.digit[tries_].w, art_.digit[tries_].h, PAL_GOLD);

    float py = kBase - puck_ * kTrack;
    bool raised = mode_ == Mode::Flight || (mode_ == Mode::Meter && meter_ > 0.72f);
    spr(raised ? art_.malletUp : art_.mallet, 196.f, raised ? 150.f : 164.f, 40, 40, PAL_MALLET);
    int bp = bellFlash_ > 0 && ((bellFlash_ / 3) & 1) ? PAL_WIN : PAL_BELL;
    float by = (mode_ == Mode::Win && bellFlash_ > 0) ? 16.f + float(bellFlash_ & 1) : 18.f;
    spr(art_.bell, float(kTowerX - 4), by, 44, 36, bp);
    spr(art_.puck, float(kTowerX + 9), py - 6.f, 18, 12, PAL_PUCK);
    spr(art_.tower, float(kTowerX), 36.f, 36, 168, PAL_TOWER);
    spr(art_.base, float(kTowerX - 22), 192.f, 80, 16, PAL_TOWER);

    if (mode_ == Mode::Meter || mode_ == Mode::Ready) {
        for (int i = 0; i < 12; i++) {
            float need = (i + 1) / 12.f;
            int pal = PAL_METER;
            if (meter_ + 0.001f >= need) pal = i >= 10 ? PAL_GOLD : (i >= 7 ? PAL_WIN : PAL_METER);
            spr(art_.lamp, 48.f, 168.f - i * 12.f, 10, 10, pal);
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    tick(1.f / 60.f);
    tickAudio();
    draw();
}

}  // namespace striker

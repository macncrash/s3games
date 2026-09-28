#include "game/jugglemark.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace jugglemark {
namespace {

constexpr int FLIGHT = 40;
constexpr int EVERY = 20;
constexpr int NEED = 3;
constexpr float LX = 112.f;
constexpr float RX = 208.f;
constexpr float HY = 150.f;
constexpr float PI = 3.14159265f;

float handX(int h) { return h == 0 ? LX : RX; }

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    mode_ = Mode::Title;
    over_ = won_ = rules_ = finished_ = false;
    marks_ = catches_ = 0;
    titleWait_ = 0;
    beep_ = 0;
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
}

void Game::begin() {
    rules_ = true;
    won_ = over_ = finished_ = false;
    marks_ = catches_ = 0;
    pattern();
    blip(392.f);
}

void Game::pattern() {
    mode_ = Mode::Play;
    tick_ = 0;
    wait_ = 0;
    latch_[0] = latch_[1] = 0;
    hn_[0] = 2;
    hold_[0][0] = 0;
    hold_[0][1] = 2;
    hn_[1] = 1;
    hold_[1][0] = 1;
    hold_[1][1] = 0;
    for (auto& a : air_) a = {};
}

void Game::finishMark() {
    finished_ = true;
    won_ = true;
    over_ = true;
    mode_ = Mode::Over;
    chord(523.25f, 659.25f, 1046.5f);
    if (sys_) {
        sys_->rumble(0.3f, 0.7f, 140);
        sys_->setLight(255, 210, 70);
    }
}

void Game::miss() {
    marks_ = 0;
    mode_ = Mode::Drop;
    wait_ = 36;
    for (auto& a : air_) a.on = false;
    blip(98.f);
    if (sys_) sys_->setLight(140, 30, 30);
}

void Game::blip(float freq) {
    if (!sys_) return;
    sys_->apu.tone(0, freq, 0.08f);
    beep_ = 0.09f;
}

void Game::chord(float a, float b, float c) {
    if (!sys_) return;
    sys_->apu.tone(0, a, 0.08f);
    sys_->apu.tone(1, b, 0.06f);
    sys_->apu.tone(2, c, 0.05f);
    beep_ = 0.32f;
}

void Game::ageTone() {
    if (!sys_) return;
    if (beep_ > 0.f) {
        beep_ -= 1.f / 60.f;
        if (beep_ <= 0.f) {
            sys_->apu.tone(0, 0, 0);
            sys_->apu.tone(1, 0, 0);
            sys_->apu.tone(2, 0, 0);
        }
    }
}

void Game::stepPlay() {
    if (bot_) {
        for (const auto& a : air_) {
            if (!a.on) continue;
            if (a.age >= FLIGHT - 8) latch_[a.to] = 12;
        }
    } else if (sys_) {
        const gs::Pad& p = sys_->pad;
        if (p.pressed(gs::BTN_LEFT) || p.pressed(gs::BTN_A) || p.pressed(gs::BTN_X)) latch_[0] = 14;
        if (p.pressed(gs::BTN_RIGHT) || p.pressed(gs::BTN_B) || p.pressed(gs::BTN_Y)) latch_[1] = 14;
    }

    if (tick_ % EVERY == 0) {
        int h = (tick_ / EVERY) & 1;
        if (hn_[h] <= 0) {
            miss();
            return;
        }
        int b = hold_[h][0];
        hold_[h][0] = hold_[h][1];
        hn_[h]--;
        for (auto& a : air_) {
            if (a.on) continue;
            a.on = true;
            a.ball = b;
            a.to = 1 - h;
            a.age = 0;
            break;
        }
    }

    for (auto& a : air_) {
        if (!a.on || a.age != FLIGHT) continue;
        int h = a.to;
        a.on = false;
        if (latch_[h] <= 0 || hn_[h] >= 2) {
            miss();
            return;
        }
        hold_[h][hn_[h]++] = a.ball;
        latch_[h] = 0;
        catches_++;
        blip(a.ball == 1 ? 660.f : 440.f);
        if (a.ball == 1) {
            marks_++;
            if (marks_ >= NEED) {
                finishMark();
                return;
            }
        }
    }

    for (auto& a : air_)
        if (a.on && a.age < FLIGHT) a.age++;
    for (int i = 0; i < 2; i++)
        if (latch_[i] > 0) latch_[i]--;
    tick_++;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    ageTone();
    if (mode_ == Mode::Title) {
        titleWait_++;
        bool go = sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A);
        if (bot_ && titleWait_ > 24) go = true;
        if (go) begin();
    } else if (mode_ == Mode::Play) {
        if (!bot_ && sys.pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Title;
            titleWait_ = 0;
            rules_ = false;
        } else {
            stepPlay();
        }
    } else if (mode_ == Mode::Drop) {
        if (--wait_ <= 0) pattern();
    } else if (mode_ == Mode::Over) {
        if (!bot_ && (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A))) begin();
    }
    draw();
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        uint16_t c = gs::rgb4(2, 1, 5);
        if (y > 40) c = gs::rgb4(4, 2, 8);
        if (y > 100) c = gs::rgb4(6, 2, 8);
        if (y > 150) c = gs::rgb4(3, 2, 4);
        if (y > 186) c = gs::rgb4(2, 1, 2);
        v.lineBackdrop[y] = c;
    }
}

void Game::spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool shadow) {
    if (!sys_ || img.w == 0) return;
    gs::Sprite s;
    s.img = img;
    s.x = int16_t(std::lround(cx - w * 0.5f));
    s.y = int16_t(std::lround(cy - h * 0.5f));
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.pal = uint8_t(pal);
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::ballAt(int ball, float x, float y, float scale) {
    const gs::Image& img = ball == 1 ? art_.gold : art_.ball;
    int pal = ball == 1 ? PAL_GOLD : PAL_RED;
    float s = 16.f * scale;
    spr(img, x, y + 10.f, s, s * 0.45f, pal, true);
    spr(img, x, y, s, s, pal, false);
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
    backdrop();

    spr(art_.stage, 160.f, 188.f, float(art_.stage.w), float(art_.stage.h), PAL_STAGE);
    spr(art_.lamp, 28.f, 78.f, float(art_.lamp.w), float(art_.lamp.h), PAL_LAMP);
    spr(art_.lamp, 292.f, 78.f, float(art_.lamp.w), float(art_.lamp.h), PAL_LAMP);
    spr(art_.juggler, 160.f, 132.f, float(art_.juggler.w), float(art_.juggler.h), PAL_BODY);
    spr(art_.chalk, 160.f, 168.f, float(art_.chalk.w), float(art_.chalk.h), PAL_GOLD);

    auto glove = [&](int h, float x) {
        bool hot = latch_[h] > 0 && mode_ == Mode::Play;
        spr(art_.glove, x, HY, hot ? 22.f : 18.f, hot ? 16.f : 14.f, PAL_GLOVE);
    };

    if (mode_ == Mode::Play || mode_ == Mode::Drop || mode_ == Mode::Over) {
        for (int h = 0; h < 2; h++) {
            for (int i = 0; i < hn_[h]; i++) {
                float y = HY - 8.f - i * 10.f;
                ballAt(hold_[h][i], handX(h), y, hold_[h][i] == 1 ? 1.15f : 1.f);
            }
        }
        for (const auto& a : air_) {
            if (!a.on) continue;
            float u = float(a.age) / float(FLIGHT);
            if (u < 0.f) u = 0.f;
            if (u > 1.f) u = 1.f;
            float x = handX(1 - a.to) + (handX(a.to) - handX(1 - a.to)) * u;
            float y = HY - 8.f - std::sin(u * PI) * 86.f;
            ballAt(a.ball, x, y, a.ball == 1 ? 1.15f : 1.f);
        }
    } else {
        ballAt(0, LX, HY - 18.f, 1.f);
        ballAt(1, 160.f, 78.f, 1.2f);
        ballAt(2, RX, HY - 18.f, 1.f);
    }
    glove(0, LX);
    glove(1, RX);

    if (mode_ == Mode::Title) spr(art_.title, 160.f, 28.f, float(art_.title.w), float(art_.title.h), PAL_TITLE);
    if (mode_ == Mode::Over && won_) spr(art_.win, 160.f, 28.f, float(art_.win.w), float(art_.win.h), PAL_WIN);

    hud(1, 1, "S3 JUGGLEMARK", PAL_INK);
    char buf[40];
    std::snprintf(buf, sizeof buf, "MARK %d/%d", marks_, NEED);
    hud(29, 1, buf, PAL_MARK);
    if (mode_ == Mode::Title) {
        hudC(23, "GOLD BALL IS THE MARK", PAL_MARK);
        hudC(25, "START  CATCH IT THREE TIMES", PAL_INK);
    } else if (mode_ == Mode::Play) {
        hudC(25, "LEFT AND RIGHT TO CATCH", PAL_INK);
    } else if (mode_ == Mode::Drop) {
        hudC(25, "DROP  MARK CLEARED", PAL_ALERT);
    } else if (mode_ == Mode::Over) {
        hudC(25, won_ ? "FINISHED MARK" : "STILL OPEN", won_ ? PAL_WIN : PAL_ALERT);
    }
}

}  // namespace jugglemark

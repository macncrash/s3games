#include "game/archmark.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace archmark {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float CX = 214.f;
constexpr float CY = 100.f;
constexpr float GOLD = 14.f;
constexpr float SET_R = 8.f;
constexpr float LIFT_R = 14.f;
constexpr int ARROWS = 3;
constexpr float AMP_X = 26.f;
constexpr float AMP_Y = 10.f;
constexpr float OX = 1.15f;
constexpr float OY = 0.83f;
constexpr int DRAW_N = 18;
constexpr float DROP = 78.f;
constexpr float COIN_V = 96.f;
constexpr float HAND_V = 130.f;
constexpr float AIM_V = 72.f;
constexpr float LOOSE_X = 52.f;
constexpr float LOOSE_Y = 108.f;
constexpr int FLIGHT_N = 12;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

float Game::swayX(float t) const { return AMP_X * std::sin(t * OX); }
float Game::swayY(float t) const { return AMP_Y * std::sin(t * OY); }

void Game::reset() {
    arrows_ = 0;
    pinN_ = 0;
    drawTick_ = 0;
    steady_ = 0;
    flight_ = 0;
    show_ = 0;
    drawing_ = false;
    marked_ = lifted_ = finished_ = onGold_ = false;
    aimX_ = CX;
    aimY_ = CY;
    coinX_ = CX;
    coinY_ = 186.f;
    handX_ = 36.f;
    handY_ = 176.f;
    hitX_ = CX;
    hitY_ = CY;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.setFogColor(gs::rgb4(8, 11, 14));
    sys.apu.setMaster(0.7f);
    reset();
    won_ = over_ = rules_ = false;
    mode_ = Mode::Title;
    clock_ = 0;
    beep_ = 0;
}

void Game::begin() {
    reset();
    won_ = over_ = false;
    rules_ = true;
    mode_ = Mode::Place;
    blip(392.f);
}

void Game::toTitle() {
    reset();
    won_ = over_ = rules_ = false;
    mode_ = Mode::Title;
}

void Game::blip(float freq) {
    if (!sys_) return;
    sys_->apu.tone(0, freq, 0.07f);
    beep_ = std::max(beep_, 0.09f);
}

void Game::chord(float a, float b, float c) {
    if (!sys_) return;
    sys_->apu.tone(0, a, 0.08f);
    sys_->apu.tone(1, b, 0.06f);
    sys_->apu.tone(2, c, 0.05f);
    beep_ = 0.28f;
}

void Game::loose() {
    float power = clampf(float(drawTick_) / float(DRAW_N), 0.f, 1.f);
    float ox = swayX(clock_);
    float oy = swayY(clock_);
    if (steady_ > 0) {
        float mag = std::min(24.f, float(steady_) * 0.7f);
        oy += std::sin(float(steady_) * 0.47f) * mag;
        ox += std::cos(float(steady_) * 0.31f) * mag * 0.35f;
    }
    hitX_ = aimX_ + ox;
    hitY_ = aimY_ + oy + (1.f - power) * DROP;
    drawing_ = false;
    flight_ = 0;
    arrows_++;
    mode_ = Mode::Flight;
    blip(220.f);
    if (sys_) sys_->rumble(0.1f, 0.2f, 24);
}

void Game::stick() {
    float dx = hitX_ - CX;
    float dy = hitY_ - CY;
    bool gold = marked_ && std::sqrt(dx * dx + dy * dy) <= GOLD;
    if (pinN_ < ARROWS) {
        pins_[pinN_].x = hitX_;
        pins_[pinN_].y = hitY_;
        pins_[pinN_].gold = gold;
        pinN_++;
    }
    show_ = 0;
    mode_ = Mode::Show;
    if (gold) {
        onGold_ = true;
        chord(659.25f, 783.99f, 987.77f);
        if (sys_) {
            sys_->rumble(0.3f, 0.7f, 50);
            sys_->setLight(255, 190, 40);
        }
    } else {
        blip(gold ? 880.f : 140.f);
    }
}

void Game::finishMark() {
    lifted_ = true;
    finished_ = true;
    won_ = true;
    over_ = true;
    mode_ = Mode::Over;
    chord(523.25f, 659.25f, 1046.5f);
    if (sys_) {
        sys_->rumble(0.35f, 0.8f, 160);
        sys_->setLight(255, 210, 80);
    }
}

void Game::fail() {
    won_ = false;
    finished_ = false;
    lifted_ = false;
    over_ = true;
    mode_ = Mode::Over;
    blip(96.f);
    if (sys_) sys_->setLight(120, 30, 20);
}

Game::Input Game::readPad(const gs::Pad& pad) const {
    Input in;
    in.action = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_Z);
    in.held = pad.down(gs::BTN_A) || pad.down(gs::BTN_C) || pad.down(gs::BTN_Z) || pad.accel > 0.45f;
    in.start = pad.pressed(gs::BTN_START);
    in.back = pad.pressed(gs::BTN_MODE);
    if (pad.down(gs::BTN_LEFT)) in.x -= 1.f;
    if (pad.down(gs::BTN_RIGHT)) in.x += 1.f;
    if (pad.down(gs::BTN_UP)) in.y -= 1.f;
    if (pad.down(gs::BTN_DOWN)) in.y += 1.f;
    if (std::fabs(pad.axisX) > 0.2f) in.x = pad.axisX;
    if (std::fabs(pad.axisY) > 0.2f) in.y = -pad.axisY;
    float len = std::hypot(in.x, in.y);
    if (len > 1.f) {
        in.x /= len;
        in.y /= len;
    }
    return in;
}

Game::Input Game::botInput() const {
    Input in;
    if (mode_ == Mode::Title && clock_ > 0.35f) {
        in.start = true;
        return in;
    }
    if (mode_ == Mode::Place) {
        float dx = CX - coinX_;
        float dy = CY - coinY_;
        float d = std::hypot(dx, dy);
        if (d <= SET_R - 1.f) in.action = true;
        else if (d > 1e-3f) {
            in.x = dx / d;
            in.y = dy / d;
        }
        return in;
    }
    if (mode_ == Mode::Aim && !drawing_) {
        float future = clock_ + float(DRAW_N) * DT;
        float sx = swayX(future);
        float sy = swayY(future);
        if (std::hypot(sx, sy) < 2.2f && std::hypot(aimX_ - CX, aimY_ - CY) < 1.f) in.held = true;
        return in;
    }
    if (mode_ == Mode::Aim && drawing_) {
        in.held = drawTick_ < DRAW_N;
        return in;
    }
    if (mode_ == Mode::Lift && !lifted_) {
        float dx = coinX_ - handX_;
        float dy = coinY_ - handY_;
        float d = std::hypot(dx, dy);
        if (d <= LIFT_R - 2.f) in.action = true;
        else if (d > 1e-3f) {
            in.x = dx / d;
            in.y = dy / d;
        }
    }
    return in;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    clock_ += DT;
    if (beep_ > 0) {
        beep_ -= DT;
        if (beep_ <= 0) {
            sys.apu.tone(0, 0, 0);
            sys.apu.tone(1, 0, 0);
            sys.apu.tone(2, 0, 0);
        }
    }

    Input in = bot_ ? botInput() : readPad(sys.pad);

    if (mode_ == Mode::Title) {
        if (in.back && !bot_) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        } else if (in.start || in.action) begin();
    } else if (mode_ == Mode::Pause) {
        if (in.start) mode_ = held_;
        else if (in.back) toTitle();
    } else if (mode_ == Mode::Over) {
        if (!bot_ && (in.start || in.action)) begin();
        else if (!bot_ && in.back) toTitle();
    } else if ((in.start || in.back) && !bot_) {
        held_ = mode_;
        mode_ = Mode::Pause;
    } else if (mode_ == Mode::Place) {
        coinX_ = clampf(coinX_ + in.x * COIN_V * DT, 40.f, 300.f);
        coinY_ = clampf(coinY_ + in.y * COIN_V * DT, 40.f, 200.f);
        if (in.action && std::hypot(coinX_ - CX, coinY_ - CY) <= SET_R) {
            coinX_ = CX;
            coinY_ = CY;
            marked_ = true;
            mode_ = Mode::Aim;
            aimX_ = CX;
            aimY_ = CY;
            blip(520.f);
        }
    } else if (mode_ == Mode::Aim) {
        if (!drawing_) {
            aimX_ = clampf(aimX_ + in.x * AIM_V * DT, CX - 70.f, CX + 70.f);
            aimY_ = clampf(aimY_ + in.y * AIM_V * DT, CY - 50.f, CY + 50.f);
            if (in.held) {
                drawing_ = true;
                drawTick_ = 1;
                steady_ = 0;
                blip(160.f);
            }
        } else if (!in.held) {
            loose();
        } else if (drawTick_ < DRAW_N) {
            drawTick_++;
            if (drawTick_ == DRAW_N) blip(880.f);
        } else {
            steady_++;
        }
    } else if (mode_ == Mode::Flight) {
        if (++flight_ >= FLIGHT_N) stick();
    } else if (mode_ == Mode::Show) {
        int wait = bot_ ? 6 : 28;
        if (++show_ >= wait) {
            if (onGold_) mode_ = Mode::Lift;
            else if (arrows_ >= ARROWS) fail();
            else mode_ = Mode::Aim;
        }
    } else if (mode_ == Mode::Lift) {
        handX_ = clampf(handX_ + in.x * HAND_V * DT, 8.f, 310.f);
        handY_ = clampf(handY_ + in.y * HAND_V * DT, 16.f, 210.f);
        if (in.action && onGold_ && marked_ && std::hypot(handX_ - coinX_, handY_ - coinY_) <= LIFT_R) finishMark();
    }

    draw();
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        uint16_t c = gs::rgb4(7, 11, 15);
        if (y > 70) c = gs::rgb4(9, 13, 15);
        if (y > 150) c = gs::rgb4(4, 9, 4);
        if (y > 190) c = gs::rgb4(3, 7, 3);
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

    spr(art_.tree, 28.f, 128.f, float(art_.tree.w), float(art_.tree.h), PAL_WORLD);
    spr(art_.tree, 300.f, 122.f, float(art_.tree.w) * 0.8f, float(art_.tree.h) * 0.85f, PAL_WORLD);
    spr(art_.bale, CX - 46.f, 168.f, float(art_.bale.w), float(art_.bale.h), PAL_WORLD);
    spr(art_.bale, CX, 174.f, float(art_.bale.w), float(art_.bale.h), PAL_WORLD);
    spr(art_.bale, CX + 46.f, 168.f, float(art_.bale.w), float(art_.bale.h), PAL_WORLD);
    spr(art_.face, CX, CY, float(art_.face.w), float(art_.face.h), PAL_FACE);
    spr(art_.archer, 28.f, 132.f, float(art_.archer.w), float(art_.archer.h), PAL_ARCH);

    for (int i = 0; i < pinN_; i++)
        spr(art_.dot, pins_[i].x, pins_[i].y, pins_[i].gold ? 5.f : 3.f, pins_[i].gold ? 5.f : 3.f,
            pins_[i].gold ? PAL_COIN : PAL_ARROW);

    if (mode_ == Mode::Flight) {
        float u = clampf(float(flight_) / float(FLIGHT_N), 0.f, 1.f);
        float x = LOOSE_X + (hitX_ - LOOSE_X) * u;
        float y = LOOSE_Y + (hitY_ - LOOSE_Y) * u - std::sin(u * 3.14159f) * 18.f;
        spr(art_.arrow, x, y, float(art_.arrow.w), float(art_.arrow.h), PAL_ARROW);
    } else if (mode_ == Mode::Aim) {
        float sx = aimX_ + swayX(clock_);
        float sy = aimY_ + swayY(clock_);
        if (drawing_ && drawTick_ >= DRAW_N && steady_ > 0) {
            float mag = std::min(24.f, float(steady_) * 0.7f);
            sy += std::sin(float(steady_) * 0.47f) * mag;
            sx += std::cos(float(steady_) * 0.31f) * mag * 0.35f;
        }
        spr(art_.sight, sx, sy, float(art_.sight.w), float(art_.sight.h), PAL_SIGHT);
        float pull = drawing_ ? clampf(float(drawTick_) / float(DRAW_N), 0.f, 1.f) : 0.15f;
        spr(art_.arrow, LOOSE_X - (1.f - pull) * 10.f, LOOSE_Y, float(art_.arrow.w), float(art_.arrow.h), PAL_ARROW);
    }

    if (!lifted_) spr(art_.coin, coinX_, coinY_, float(art_.coin.w), float(art_.coin.h), PAL_COIN);
    if (mode_ == Mode::Lift) spr(art_.hand, handX_, handY_, float(art_.hand.w), float(art_.hand.h), PAL_HAND);

    if (mode_ == Mode::Title) spr(art_.title, 160.f, 36.f, float(art_.title.w), float(art_.title.h), PAL_TITLE);
    if (mode_ == Mode::Over && won_) spr(art_.win, 160.f, 36.f, float(art_.win.w), float(art_.win.h), PAL_WIN);

    hud(1, 1, "S3 ARCHMARK", PAL_INK);
    char buf[48];
    std::snprintf(buf, sizeof buf, "ARROWS %d/%d", arrows_, ARROWS);
    hud(28, 1, buf, PAL_GOLD);
    if (mode_ == Mode::Title) {
        hudC(22, "ONE END", PAL_GOLD);
        hudC(24, "START  SET THE MARK", PAL_INK);
    } else if (mode_ == Mode::Place) {
        hudC(24, "SET THE COIN ON THE GOLD", PAL_GOLD);
    } else if (mode_ == Mode::Aim) {
        hudC(24, drawing_ ? "LOOSE ON THE PIN" : "HOLD A  FULL DRAW", PAL_INK);
    } else if (mode_ == Mode::Flight) {
        hudC(24, "IN THE AIR", PAL_INK);
    } else if (mode_ == Mode::Show) {
        hudC(24, onGold_ ? "GOLD  LIFT THE MARK" : "WIDE", onGold_ ? PAL_GOLD : PAL_ALERT);
    } else if (mode_ == Mode::Lift) {
        hudC(24, "LIFT THE MARK", PAL_GOLD);
    } else if (mode_ == Mode::Pause) {
        hudC(23, "PAUSED", PAL_ALERT);
    } else if (mode_ == Mode::Over) {
        hudC(23, won_ ? "FINISHED MARK" : "STILL OPEN", won_ ? PAL_WIN : PAL_ALERT);
    }
}

}  // namespace archmark

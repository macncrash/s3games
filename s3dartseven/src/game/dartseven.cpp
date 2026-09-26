#include "game/dartseven.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace dartseven {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kHuman = 2.45f;
constexpr float kAi = 5.6f;
constexpr float kMeterRate = 1.05f;
constexpr float kSweet = 0.08f;
constexpr int kFlight = 14;
constexpr float kTau = 6.2831853f;

}  // namespace

void Game::bedPoint(int seg, float rad, float& x, float& y) const {
    float ang = float(seg) * (3.14159265f / 10.f);
    x = kCx + std::sin(ang) * rad;
    y = kCy - std::cos(ang) * rad;
}

Game::Hit Game::scoreAt(float x, float y) const {
    Hit h;
    std::snprintf(h.name, sizeof h.name, "MISS");
    float dx = x - kCx;
    float dy = y - kCy;
    float r = std::hypot(dx, dy);
    if (r > kDoubOut) return h;
    if (r <= kBullIn) {
        h.mul = 2;
        std::snprintf(h.name, sizeof h.name, "BULL");
        return h;
    }
    if (r <= kBullOut) {
        h.mul = 1;
        std::snprintf(h.name, sizeof h.name, "25");
        return h;
    }
    int n = kSeg[sectorAt(dx, dy)];
    int mul = 1;
    char kind = 'S';
    if (r > kTripIn && r <= kTripOut) {
        mul = 3;
        kind = 'T';
    } else if (r > kDoubIn) {
        mul = 2;
        kind = 'D';
    }
    h.mul = mul;
    std::snprintf(h.name, sizeof h.name, "%c%d", kind, n);
    return h;
}

bool Game::audit() {
    auto check = [&](int seg, float rad, int mul, const char* name) {
        float x, y;
        bedPoint(seg, rad, x, y);
        Hit h = scoreAt(x, y);
        if (h.mul != mul || std::strcmp(h.name, name) != 0) {
            std::fprintf(stderr, "s3dartseven bed %s scored %s +%d at %.2f %.2f\n", name, h.name, h.mul, x, y);
            return false;
        }
        return true;
    };
    for (int s = 0; s < 20; s++) {
        int n = kSeg[s];
        char nm[8];
        std::snprintf(nm, sizeof nm, "S%d", n);
        if (!check(s, kRInner, 1, nm)) return false;
        if (!check(s, kRSing, 1, nm)) return false;
        std::snprintf(nm, sizeof nm, "T%d", n);
        if (!check(s, kRTrip, 3, nm)) return false;
        std::snprintf(nm, sizeof nm, "D%d", n);
        if (!check(s, kRDoub, 2, nm)) return false;
    }
    if (!check(0, 0.f, 2, "BULL")) return false;
    float ox, oy;
    bedPoint(0, kR25, ox, oy);
    Hit outer = scoreAt(ox, oy);
    if (outer.mul != 1 || std::strcmp(outer.name, "25") != 0) {
        std::fprintf(stderr, "s3dartseven 25 scored %s +%d\n", outer.name, outer.mul);
        return false;
    }
    Hit miss = scoreAt(kCx, kCy - (kDoubOut + 6.f));
    if (miss.mul != 0) return false;
    return true;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    rules_ = audit();
    if (!rules_) std::fprintf(stderr, "s3dartseven rules failed\n");
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.setFogColor(gs::rgb4(3, 1, 2));
    sys.apu.setMaster(0.7f);
    aimX_ = kCx;
    aimY_ = kCy;
    last_[0] = 0;
    mode_ = Mode::Title;
}

void Game::begin() {
    you_ = 0;
    them_ = 0;
    gain_ = 0;
    thrown_ = 0;
    pinN_ = 0;
    yours_ = true;
    won_ = false;
    over_ = false;
    wasSweet_ = false;
    fanStep_ = -1;
    last_[0] = 0;
    for (Pin& p : pin_) p.on = false;
    aimX_ = kCx;
    aimY_ = kCy;
    meter_ = 0;
    meterDir_ = 1.f;
    mode_ = Mode::Aim;
}

bool Game::sweet() const { return std::fabs(meter_ - 0.5f) <= kSweet; }

bool Game::wantsThrow() const {
    const gs::Pad& p = sys_->pad;
    return p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C) || p.pressed(gs::BTN_TURBO);
}

uint32_t Game::rnd() {
    rng_ = rng_ * 1664525u + 1013904223u;
    return rng_;
}

void Game::moveAim(float mx, float my) {
    float m = std::hypot(mx, my);
    if (m > 0.f) {
        float scale = kHuman * (m > 1.f ? 1.f / m : 1.f);
        aimX_ += mx * scale;
        aimY_ += my * scale;
    }
    float lim = kRim + 14.f;
    aimX_ = std::clamp(aimX_, kCx - lim, kCx + lim);
    aimY_ = std::clamp(aimY_, kCy - lim, kCy + lim);
}

void Game::launch() {
    if (mode_ != Mode::Aim || !rules_) return;
    float off = std::fabs(meter_ - 0.5f);
    float err = 0.f;
    if (off > kSweet) {
        float u = (off - kSweet) / (0.5f - kSweet);
        err = u * u * 34.f;
    }
    float dir = float(rnd() & 1023) * (kTau / 1024.f);
    landX_ = aimX_ + std::cos(dir) * err;
    landY_ = aimY_ + std::sin(dir) * err;
    fromX_ = kCx;
    fromY_ = float(gs::SCREEN_H) + 24.f;
    destX_ = landX_;
    destY_ = landY_;
    flightT_ = 0;
    mode_ = Mode::Flight;
    if (sys_) sys_->apu.noiseBurst(0.14f, 2400.f, 0.04f);
}

void Game::stick() {
    Hit h = scoreAt(landX_, landY_);
    if (pinN_ >= 8) {
        for (int i = 1; i < 8; i++) pin_[i - 1] = pin_[i];
        pinN_ = 7;
    }
    pin_[pinN_].x = landX_;
    pin_[pinN_].y = landY_;
    pin_[pinN_].yours = yours_;
    pin_[pinN_].on = true;
    pinN_++;
    thrown_++;
    gain_ = h.mul;
    std::snprintf(last_, sizeof last_, "%s", h.name);
    if (yours_) you_ += h.mul;
    else them_ += h.mul;

    if (yours_ && you_ >= kRace) {
        won_ = true;
        over_ = true;
        mode_ = Mode::Win;
        fanStep_ = 0;
        fanT_ = 0;
        blip(0, 880.f, 0.1f, 0.12f);
        if (sys_ && !sys_->headless) sys_->rumble(0.35f, 0.7f, 160);
        return;
    }
    if (!yours_ && them_ >= kRace && you_ < kRace) {
        won_ = false;
        over_ = true;
        mode_ = Mode::Lose;
        blip(0, 110.f, 0.1f, 0.28f);
        if (sys_) sys_->apu.noiseBurst(0.12f, 80.f, 0.1f);
        return;
    }
    if (h.mul == 0) {
        if (sys_) sys_->apu.noiseBurst(0.1f, 160.f, 0.06f);
    } else {
        float freq = h.mul >= 3 ? 740.f : (h.mul == 2 ? 587.f : 392.f);
        blip(0, freq, 0.08f, 0.1f);
    }
    showT_ = 0.28f;
    mode_ = Mode::Show;
}

void Game::afterShow() {
    yours_ = !yours_;
    meter_ = 0;
    meterDir_ = 1.f;
    wasSweet_ = false;
    mode_ = Mode::Aim;
}

void Game::autoAim() {
    float x, y;
    bedPoint(kFastSeg, yours_ ? kRTrip : kRSing, x, y);
    float dx = x - aimX_;
    float dy = y - aimY_;
    float d = std::hypot(dx, dy);
    if (d > 0.35f) {
        float step = std::min(kAi, d);
        aimX_ += dx / d * step;
        aimY_ += dy / d * step;
        return;
    }
    aimX_ = x;
    aimY_ = y;
    if (sweet()) launch();
}

void Game::blip(int ch, float freq, float vol, float hold) {
    if (!sys_) return;
    sys_->apu.tone(ch, freq, vol);
    if (ch == 0) toneT_ = hold;
    else tickT_ = hold;
}

void Game::tickAudio(float dt) {
    if (!sys_) return;
    if (toneT_ > 0.f) {
        toneT_ -= dt;
        if (toneT_ <= 0.f) sys_->apu.tone(0, 0, 0);
    }
    if (tickT_ > 0.f) {
        tickT_ -= dt;
        if (tickT_ <= 0.f) sys_->apu.tone(2, 0, 0);
    }
    if (fanStep_ < 0) return;
    if (fanStep_ > 0) {
        fanT_ -= dt;
        if (fanT_ > 0.f) return;
    }
    const float notes[4] = {523.25f, 659.25f, 783.99f, 1046.5f};
    if (fanStep_ < 4) {
        sys_->apu.tone(1, notes[fanStep_], 0.11f);
        fanStep_++;
        fanT_ = 0.11f;
    } else {
        sys_->apu.tone(1, 0, 0);
        fanStep_ = -1;
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    clock_ += kDt;
    const gs::Pad& p = sys.pad;
    Mode before = mode_;

    if (mode_ == Mode::Title || mode_ == Mode::Aim) {
        meter_ += meterDir_ * kMeterRate * kDt;
        if (meter_ >= 1.f) {
            meter_ = 1.f;
            meterDir_ = -1.f;
        } else if (meter_ <= 0.f) {
            meter_ = 0.f;
            meterDir_ = 1.f;
        }
    }

    if (mode_ == Mode::Title) {
        if (bot_) {
            if (clock_ > 0.4f) begin();
        } else if (p.pressed(gs::BTN_MODE)) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        } else if (p.pressed(gs::BTN_START) || wantsThrow()) {
            begin();
        }
    } else if (mode_ == Mode::Pause) {
        if (p.pressed(gs::BTN_START) || wantsThrow()) mode_ = held_;
        else if (p.pressed(gs::BTN_MODE)) mode_ = Mode::Title;
    } else if (mode_ == Mode::Win || mode_ == Mode::Lose) {
        if (!bot_ && (p.pressed(gs::BTN_START) || wantsThrow())) begin();
        else if (!bot_ && p.pressed(gs::BTN_MODE)) mode_ = Mode::Title;
    } else if (mode_ == Mode::Aim) {
        bool sw = sweet();
        if (sw && !wasSweet_) blip(2, 988.f, 0.04f, 0.045f);
        wasSweet_ = sw;
        if (!bot_ && (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_MODE))) {
            held_ = Mode::Aim;
            mode_ = Mode::Pause;
        } else if (bot_ || !yours_) {
            autoAim();
        } else {
            float mx = 0, my = 0;
            if (p.down(gs::BTN_LEFT)) mx -= 1.f;
            if (p.down(gs::BTN_RIGHT)) mx += 1.f;
            if (p.down(gs::BTN_UP)) my -= 1.f;
            if (p.down(gs::BTN_DOWN)) my += 1.f;
            if (std::fabs(p.axisX) > 0.2f) mx = p.axisX;
            if (std::fabs(p.axisY) > 0.2f) my = -p.axisY;
            moveAim(mx, my);
            if (wantsThrow()) launch();
        }
    }

    if (mode_ == Mode::Flight && before == Mode::Flight) {
        flightT_++;
        if (flightT_ >= kFlight) stick();
    } else if (mode_ == Mode::Show && before == Mode::Show) {
        showT_ -= kDt;
        bool skip = !bot_ && (wantsThrow() || p.pressed(gs::BTN_START));
        if (showT_ <= 0.f || skip) afterShow();
    }

    tickAudio(kDt);
    draw();
}

void Game::spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool shadow) {
    if (!sys_ || w < 1.f || h < 1.f || img.w == 0) return;
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = img;
    s.pal = uint8_t(pal);
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::dartAt(float x, float y, float h, bool yours) {
    int ih = std::max(1, int(std::lround(h)));
    int iw = std::max(1, int(std::lround(h * float(art_.dart.w) / float(std::max(1, int(art_.dart.h))))));
    spr(art_.dart.pick(float(ih)), x, y, float(iw), float(ih), yours ? PAL_YOU : PAL_HOUSE);
    spr(art_.shadow, x + 1.f, y + h * 0.28f, float(iw) * 0.7f, 4.f, PAL_BOARD, true);
}

void Game::meterRow() {
    for (int i = 0; i < kRace; i++) {
        float x = kCx + float(i - 3) * 12.f;
        bool mid = i == 3;
        bool on = std::fabs(meter_ * 6.f - float(i)) <= 0.48f;
        int pal = PAL_PIPE;
        if (on && sweet()) pal = PAL_PIPY;
        else if (on) pal = PAL_PIPH;
        else if (mid) pal = PAL_METER;
        float s = (mid ? 9.f : 7.f);
        spr(art_.pip, x, kMeterY, s, s, pal);
    }
}

void Game::lamps() {
    int yl = std::min(you_, kRace);
    int tl = std::min(them_, kRace);
    for (int i = 0; i < kRace; i++) {
        float y = kLampY0 + float(i) * kLampStep;
        spr(art_.pip, kLampYou, y, 9.f, 9.f, i < yl ? PAL_PIPY : PAL_PIPE);
        spr(art_.pip, kLampHouse, y, 9.f, 9.f, i < tl ? PAL_PIPH : PAL_PIPE);
    }
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

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        uint16_t c = gs::rgb4(4, 1, 2);
        if (y < 32) c = gs::rgb4(6, 2, 3);
        else if (y >= 200) c = gs::rgb4(2, 1, 1);
        v.lineBackdrop[y] = c;
    }
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    if (mode_ == Mode::Title) spr(art_.title, kCx, 22.f, float(art_.title.w), float(art_.title.h), PAL_TITLE);
    if (mode_ == Mode::Win) spr(art_.win, kCx, kCy - 8.f, float(art_.win.w), float(art_.win.h), PAL_WIN);
    if (mode_ == Mode::Lose) spr(art_.lose, kCx, kCy - 8.f, float(art_.lose.w), float(art_.lose.h), PAL_LOSE);

    if (mode_ == Mode::Aim || mode_ == Mode::Pause) {
        meterRow();
        int pal = sweet() ? PAL_GREEN : (yours_ ? PAL_AIM : PAL_RED);
        spr(art_.cross, aimX_, aimY_, float(art_.cross.w), float(art_.cross.h), pal);
    } else if (mode_ == Mode::Title) {
        float tx, ty;
        bedPoint(kFastSeg, kRTrip, tx, ty);
        float bob = std::sin(clock_ * 3.f) * 1.5f;
        spr(art_.cross, tx, ty + bob, float(art_.cross.w), float(art_.cross.h), PAL_GOLD);
    }

    if (mode_ == Mode::Flight) {
        float u = std::clamp(float(flightT_) / float(kFlight), 0.f, 1.f);
        float e = u * u * (3.f - 2.f * u);
        float x = fromX_ + (destX_ - fromX_) * e;
        float y = fromY_ + (destY_ - fromY_) * e;
        float h = 30.f + (12.f - 30.f) * e;
        dartAt(x, y, h, yours_);
    }
    for (int i = pinN_ - 1; i >= 0; --i) {
        if (!pin_[i].on) continue;
        dartAt(pin_[i].x, pin_[i].y, 13.f, pin_[i].yours);
    }

    float tx, ty;
    bedPoint(kFastSeg, kRTrip, tx, ty);
    spr(art_.dot, tx, ty, float(art_.dot.w), float(art_.dot.h), PAL_GOLD);

    lamps();
    float railY = kLampY0 + 3.f * kLampStep;
    spr(art_.rail, kLampYou, railY, float(art_.rail.w), float(art_.rail.h), PAL_BOARD);
    spr(art_.rail, kLampHouse, railY, float(art_.rail.w), float(art_.rail.h), PAL_BOARD);

    gs::Sprite board;
    board.img = art_.board;
    board.x = int16_t(kBoardX);
    board.y = int16_t(kBoardY);
    board.w = int16_t(art_.board.w);
    board.h = int16_t(art_.board.h);
    board.pal = PAL_BOARD;
    v.sprite(board);

    char buf[40];
    std::snprintf(buf, sizeof buf, "YOU %d", you_);
    hud(1, 0, buf, PAL_GREEN);
    std::snprintf(buf, sizeof buf, "HOUSE %d", them_);
    hud(40 - int(std::strlen(buf)) - 1, 0, buf, PAL_GOLD);

    if (mode_ == Mode::Title) {
        hudC(25, "RACE TO 7    HOUSE THROWS S20", PAL_GOLD);
        hudC(26, "T3  D2  S1   25 IS 1   BULL 2", PAL_INK);
        hudC(27, "ARROWS AIM   Z IN GREEN   START", PAL_GREEN);
        return;
    }

    if (mode_ == Mode::Aim || mode_ == Mode::Pause || mode_ == Mode::Flight) {
        hudC(0, yours_ ? "YOUR DART" : "HOUSE DART", yours_ ? PAL_GREEN : PAL_RED);
    }

    if (mode_ == Mode::Aim || mode_ == Mode::Pause) {
        Hit live = scoreAt(aimX_, aimY_);
        int pal = live.mul >= 3 ? PAL_GREEN : (live.mul == 2 ? PAL_GOLD : (live.mul == 1 ? PAL_INK : PAL_RED));
        std::snprintf(buf, sizeof buf, "%s  +%d", live.name, live.mul);
        hudC(25, buf, pal);
        hudC(26, sweet() ? "THROW" : "WAIT", sweet() ? PAL_GREEN : PAL_AIM);
    } else if (last_[0]) {
        std::snprintf(buf, sizeof buf, "LAST %s  +%d", last_, gain_);
        hudC(25, buf, gain_ > 0 ? PAL_GOLD : PAL_RED);
    }

    int need = std::max(0, kRace - you_);
    if (mode_ == Mode::Win) hudC(26, "FIRST TO SEVEN", PAL_GREEN);
    else if (mode_ == Mode::Lose) hudC(26, "HOUSE GOT THERE", PAL_RED);
    else if (mode_ != Mode::Aim && mode_ != Mode::Pause) {
        std::snprintf(buf, sizeof buf, "NEED %d", need);
        hudC(26, buf, need <= 2 ? PAL_GOLD : PAL_INK);
    }
    if (mode_ == Mode::Pause) hudC(27, "PAUSED", PAL_GOLD);
    else if (mode_ == Mode::Win) hudC(27, "START", PAL_GREEN);
    else if (yours_ && mode_ == Mode::Aim && thrown_ == 0) hudC(27, "T20 IS THE FAST WAY", PAL_GOLD);
}

}  // namespace dartseven

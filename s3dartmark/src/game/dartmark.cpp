#include "game/dartmark.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace dartmark {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kSpeed = 3.35f;
constexpr float kMeterRate = 1.25f;
constexpr float kSweet = 0.06f;
constexpr int kFlight = 16;
constexpr int kMaxDarts = 3;
constexpr float kTRad = (kTripIn + kTripOut) * 0.5f;
constexpr float kTau = 6.2831853f;

void cluster(int slot, float& ox, float& oy) {
    static const float oxs[3] = {-4.f, 4.2f, 0.f};
    static const float oys[3] = {3.2f, 2.6f, -4.f};
    int i = slot < 0 ? 0 : (slot > 2 ? 2 : slot);
    ox = oxs[i];
    oy = oys[i];
}

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
        h.number = 50;
        h.mul = 2;
        std::snprintf(h.name, sizeof h.name, "BULL");
        return h;
    }
    if (r <= kBullOut) {
        h.number = 25;
        h.mul = 1;
        std::snprintf(h.name, sizeof h.name, "25");
        return h;
    }
    int s = sectorAt(dx, dy);
    int n = kSeg[s];
    int mul = 1;
    char kind = 'S';
    if (r > kTripIn && r <= kTripOut) {
        mul = 3;
        kind = 'T';
    } else if (r > kDoubIn) {
        mul = 2;
        kind = 'D';
    }
    h.number = n;
    h.mul = mul;
    std::snprintf(h.name, sizeof h.name, "%c%d", kind, n);
    return h;
}

bool Game::audit() {
    auto check = [&](int seg, float rad, int number, int mul, const char* name) {
        float x, y;
        bedPoint(seg, rad, x, y);
        Hit h = scoreAt(x, y);
        if (h.number != number || h.mul != mul || std::strcmp(h.name, name) != 0) {
            std::fprintf(stderr, "s3dartmark bed %s scored %s (%d x%d) at %.2f %.2f\n", name, h.name, h.number, h.mul, x,
                         y);
            return false;
        }
        return true;
    };
    for (int s = 0; s < 20; s++) {
        int n = kSeg[s];
        char nm[8];
        std::snprintf(nm, sizeof nm, "S%d", n);
        if (!check(s, (kBullOut + kTripIn) * 0.5f, n, 1, nm)) return false;
        if (!check(s, (kTripOut + kDoubIn) * 0.5f, n, 1, nm)) return false;
        std::snprintf(nm, sizeof nm, "T%d", n);
        if (!check(s, kTRad, n, 3, nm)) return false;
        std::snprintf(nm, sizeof nm, "D%d", n);
        if (!check(s, (kDoubIn + kDoubOut) * 0.5f, n, 2, nm)) return false;
    }
    Hit bull = scoreAt(kCx, kCy);
    if (std::strcmp(bull.name, "BULL") != 0) return false;
    Hit outer = scoreAt(kCx, kCy - (kBullIn + kBullOut) * 0.5f);
    if (std::strcmp(outer.name, "25") != 0) {
        std::fprintf(stderr, "s3dartmark 25 scored %s\n", outer.name);
        return false;
    }
    Hit miss = scoreAt(kCx, kCy - (kDoubOut + 8.f));
    if (miss.number != 0) return false;
    return true;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    rules_ = audit();
    if (!rules_) std::fprintf(stderr, "s3dartmark rules failed\n");
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.setFogColor(gs::rgb4(2, 1, 2));
    sys.apu.setMaster(0.7f);
    aimX_ = kCx;
    aimY_ = kCy;
    std::snprintf(out_, sizeof out_, "----");
    last_[0] = 0;
    mode_ = Mode::Title;
}

void Game::begin() {
    marks_ = 0;
    thrown_ = 0;
    visitN_ = 0;
    finished_ = false;
    won_ = false;
    over_ = false;
    wasSweet_ = false;
    fanStep_ = -1;
    std::snprintf(out_, sizeof out_, "----");
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
        float scale = kSpeed * (m > 1.f ? 1.f / m : 1.f);
        aimX_ += mx * scale;
        aimY_ += my * scale;
    }
    float lim = kRim + 10.f;
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
    float ox, oy;
    cluster(visitN_, ox, oy);
    fromX_ = kCx;
    fromY_ = float(gs::SCREEN_H) + 22.f;
    destX_ = landX_ + ox;
    destY_ = landY_ + oy;
    flightT_ = 0;
    mode_ = Mode::Flight;
    if (sys_) sys_->apu.noiseBurst(0.16f, 2100.f, 0.04f);
}

void Game::finishMark() {
    finished_ = true;
    won_ = true;
    over_ = true;
    mode_ = Mode::Win;
    fanStep_ = 0;
    fanT_ = 0;
    if (sys_ && !sys_->headless) sys_->rumble(0.3f, 0.7f, 160);
}

void Game::fail() {
    finished_ = false;
    won_ = false;
    over_ = true;
    mode_ = Mode::Lose;
    if (sys_) {
        sys_->apu.tone(0, 90.f, 0.1f);
        toneT_ = 0.28f;
        sys_->apu.noiseBurst(0.16f, 70.f, 0.12f);
    }
}

void Game::stick() {
    Hit h = scoreAt(landX_, landY_);
    float r = std::hypot(landX_ - kCx, landY_ - kCy);
    if (r <= kRim && visitN_ < 3) {
        pin_[visitN_].x = landX_;
        pin_[visitN_].y = landY_;
        pin_[visitN_].on = true;
    }
    if (visitN_ < 3) visitN_++;
    thrown_++;
    std::snprintf(last_, sizeof last_, "%s", h.name);

    int add = (h.number == kMarkNum && h.mul > 0) ? h.mul : 0;
    if (add > 0) {
        marks_ += add;
        if (marks_ > 3) marks_ = 3;
    }
    if (marks_ >= 3) {
        std::snprintf(out_, sizeof out_, "%s", h.name);
        float freq = add >= 3 ? 784.f : (add == 2 ? 659.f : 523.f);
        blip(0, freq, 0.1f, 0.12f);
        finishMark();
        return;
    }
    if (add == 0) {
        if (h.number == 0) {
            if (sys_) sys_->apu.noiseBurst(0.1f, 140.f, 0.07f);
        } else {
            blip(0, 196.f, 0.06f, 0.08f);
        }
    } else {
        float freq = add >= 2 ? 587.f : 392.f;
        blip(0, freq, 0.08f, 0.1f);
    }
    showT_ = 0.42f;
    mode_ = Mode::Show;
}

void Game::afterShow() {
    if (marks_ >= 3) return;
    if (thrown_ >= kMaxDarts) {
        fail();
        return;
    }
    meter_ = 0;
    meterDir_ = 1.f;
    wasSweet_ = false;
    mode_ = Mode::Aim;
}

void Game::botPlay() {
    float x, y;
    bedPoint(kMarkSeg, kTRad, x, y);
    float dx = x - aimX_;
    float dy = y - aimY_;
    float d = std::hypot(dx, dy);
    if (d > 0.7f) {
        float step = std::min(kSpeed, d);
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
        fanT_ = 0.12f;
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

    if (bot_) {
        if (mode_ == Mode::Title && clock_ > 0.35f) begin();
    } else if (mode_ == Mode::Title) {
        if (p.pressed(gs::BTN_MODE)) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        } else if (p.pressed(gs::BTN_START) || wantsThrow()) {
            begin();
        }
    } else if (mode_ == Mode::Pause) {
        if (p.pressed(gs::BTN_START) || wantsThrow()) mode_ = held_;
        else if (p.pressed(gs::BTN_MODE)) mode_ = Mode::Title;
    } else if (mode_ == Mode::Win || mode_ == Mode::Lose) {
        if (p.pressed(gs::BTN_START) || wantsThrow()) begin();
        else if (p.pressed(gs::BTN_MODE)) mode_ = Mode::Title;
    }

    if (mode_ == Mode::Aim) {
        meter_ += meterDir_ * kMeterRate * kDt;
        if (meter_ >= 1.f) {
            meter_ = 1.f;
            meterDir_ = -1.f;
        } else if (meter_ <= 0.f) {
            meter_ = 0.f;
            meterDir_ = 1.f;
        }
        bool sw = sweet();
        if (sw && !wasSweet_) blip(2, 880.f, 0.045f, 0.05f);
        wasSweet_ = sw;

        if (bot_) {
            botPlay();
        } else if (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_MODE)) {
            held_ = Mode::Aim;
            mode_ = Mode::Pause;
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

void Game::dartSpr(float x, float y, float h, int slot) {
    int ih = std::max(1, int(std::lround(h)));
    int iw = std::max(1, int(std::lround(h * float(art_.dart.w) / float(std::max(1, int(art_.dart.h))))));
    spr(art_.dart.pick(float(ih)), x, y, float(iw), float(ih), PAL_DART0 + (slot % 3));
    spr(art_.shadow, x + 1.f, y + h * 0.32f, float(iw) * 0.8f, 5.f, PAL_BOARD);
}

void Game::ringAt(float x, float y, float meter, bool cross) {
    float rad = 5.f + meter * 22.f;
    bool sw = std::fabs(meter - 0.5f) <= kSweet;
    int pal = sw ? PAL_GREEN : PAL_AIM;
    int n = sw ? 14 : 8;
    for (int i = 0; i < n; i++) {
        float a = clock_ * 1.4f + float(i) * (kTau / float(n));
        spr(art_.dot, x + std::cos(a) * rad, y + std::sin(a) * rad, float(art_.dot.w), float(art_.dot.h), pal);
    }
    if (cross) spr(art_.cross, x, y, float(art_.cross.w), float(art_.cross.h), pal);
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
        uint16_t c = gs::rgb4(2, 1, 2);
        if (y < 36) c = gs::rgb4(4, 2, 3);
        else if (y >= 200) c = gs::rgb4(1, 1, 1);
        v.lineBackdrop[y] = c;
    }
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    float tx, ty;
    bedPoint(kMarkSeg, kTRad, tx, ty);

    if (mode_ == Mode::Title) spr(art_.title, 196.f, 20.f, float(art_.title.w), float(art_.title.h), PAL_TITLE);
    if (mode_ == Mode::Win) spr(art_.win, kCx, kCy + 8.f, float(art_.win.w), float(art_.win.h), PAL_WIN);
    if (mode_ == Mode::Lose) spr(art_.lose, kCx, kCy + 8.f, float(art_.lose.w), float(art_.lose.h), PAL_LOSE);

    if (mode_ == Mode::Aim || mode_ == Mode::Pause) ringAt(aimX_, aimY_, meter_, true);
    if (mode_ == Mode::Title) {
        float u = std::fmod(clock_ * 0.55f, 2.f);
        if (u > 1.f) u = 2.f - u;
        ringAt(tx, ty, u, true);
    }

    if (mode_ == Mode::Flight) {
        float u = std::clamp(float(flightT_) / float(kFlight), 0.f, 1.f);
        float e = u * u * (3.f - 2.f * u);
        float x = fromX_ + (destX_ - fromX_) * e;
        float y = fromY_ + (destY_ - fromY_) * e;
        float h = 26.f + (13.f - 26.f) * e;
        dartSpr(x, y, h, visitN_);
    }
    for (int i = 0; i < 3; i++) {
        if (!pin_[i].on) continue;
        float ox, oy;
        cluster(i, ox, oy);
        dartSpr(pin_[i].x + ox, pin_[i].y + oy, 20.f, i);
    }

    spr(art_.pip, tx, ty, float(art_.pip.w), float(art_.pip.h), PAL_PIP);

    for (int i = 0; i < marks_ && i < 3; i++) {
        float sx = kSlateX + 19.f + float(i) * 16.f;
        spr(art_.slash, sx, kSlateY + 75.f, float(art_.slash.w), float(art_.slash.h), PAL_CHALK);
    }
    spr(art_.slate, kSlateX + 35.f, kSlateY + 54.f, float(art_.slate.w), float(art_.slate.h), PAL_SLATE);

    gs::Sprite board;
    board.img = art_.board;
    board.x = int16_t(kBoardX);
    board.y = int16_t(kBoardY);
    board.w = int16_t(art_.board.w);
    board.h = int16_t(art_.board.h);
    board.pal = PAL_BOARD;
    v.sprite(board);

    hud(0, 0, "S3 DARTMARK", PAL_INK);
    char buf[16];
    if (mode_ == Mode::Title) {
        hud(33, 0, "SHORT", PAL_GOLD);
        hud(2, 10, "MARK", PAL_GOLD);
        hud(3, 12, "20", PAL_INK);
        hud(2, 14, "0/3", PAL_INK);
        hud(2, 21, "CHALK", PAL_INK);
        hud(31, 8, "ARROWS", PAL_INK);
        hud(31, 10, "AIM", PAL_GOLD);
        hud(31, 13, "THROW", PAL_INK);
        hud(31, 15, "IN GREEN", PAL_GREEN);
        hud(31, 18, "Z C", PAL_GOLD);
        hud(31, 20, "SPACE", PAL_INK);
        hudC(26, "THREE MARKS CLOSE 20", PAL_GOLD);
        hud(33, 27, "START", PAL_GREEN);
        return;
    }

    std::snprintf(buf, sizeof buf, "%d/%d", std::min(thrown_, kMaxDarts), kMaxDarts);
    hud(35, 0, buf, PAL_GOLD);
    hud(2, 10, "MARK", PAL_GOLD);
    hud(3, 12, "20", PAL_INK);
    std::snprintf(buf, sizeof buf, "%d/3", marks_);
    hud(3, 14, buf, marks_ > 0 ? PAL_GREEN : PAL_INK);
    if (finished_) hud(1, 21, "CLOSED", PAL_GREEN);
    else if (marks_ > 0) hud(2, 21, "OPEN", PAL_GOLD);
    else hud(2, 21, "CHALK", PAL_INK);

    if (mode_ == Mode::Aim || mode_ == Mode::Pause) {
        Hit live = scoreAt(aimX_, aimY_);
        int pal = PAL_INK;
        if (live.number == kMarkNum && live.mul == 3) pal = PAL_GREEN;
        else if (live.number == kMarkNum) pal = PAL_GOLD;
        else if (live.number == 0) pal = PAL_RED;
        hud(31, 6, live.name, pal);
        hud(31, 8, sweet() ? "NOW" : "WAIT", sweet() ? PAL_GREEN : PAL_AIM);
    }
    hud(31, 11, "LAST", PAL_INK);
    hud(31, 13, last_[0] ? last_ : "----", PAL_GOLD);
    hud(31, 16, finished_ ? "DART" : "LEFT", PAL_INK);
    std::snprintf(buf, sizeof buf, "%d", finished_ ? thrown_ : std::max(0, kMaxDarts - thrown_));
    hud(33, 18, buf, finished_ ? PAL_GOLD : PAL_INK);

    if (mode_ == Mode::Pause) hudC(25, "PAUSED", PAL_GOLD);
    if (mode_ == Mode::Win) hudC(26, "MARK CLOSED", PAL_GREEN);
    if (mode_ == Mode::Lose) hudC(26, "MARK STILL OPEN", PAL_RED);
    if (thrown_ == 0 && mode_ == Mode::Aim) hudC(26, "T20 FINISHES IT", PAL_GOLD);
}

}  // namespace dartmark

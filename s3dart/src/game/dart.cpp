#include "game/dart.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace dart {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kSpeed = 2.65f;
constexpr float kGreen = 0.22f;
constexpr int kFlight = 16;
constexpr float kTau = 6.2831853f;

void cluster(int slot, float& ox, float& oy) {
    static const float oxs[3] = {-3.5f, 3.5f, 0.f};
    static const float oys[3] = {2.5f, 2.5f, -3.5f};
    int i = slot < 0 ? 0 : (slot > 2 ? 2 : slot);
    ox = oxs[i];
    oy = oys[i];
}

}  // namespace

void Game::addBed(int score, bool dbl, int sector, float rad, char kind) {
    Bed b;
    b.score = score;
    b.dbl = dbl;
    b.sector = sector;
    b.rad = rad;
    std::snprintf(b.name, sizeof b.name, "%c%d", kind, kSeg[sector]);
    beds_.push_back(b);
}

void Game::buildBeds() {
    beds_.clear();
    const float rS = (kTripOut + kDoubIn) * 0.5f;
    const float rT = (kTripIn + kTripOut) * 0.5f;
    const float rD = (kDoubIn + kDoubOut) * 0.5f;
    for (int s = 0; s < 20; s++) {
        int n = kSeg[s];
        addBed(n, false, s, rS, 'S');
        addBed(n * 2, true, s, rD, 'D');
        addBed(n * 3, false, s, rT, 'T');
    }
    Bed bull;
    bull.score = 50;
    bull.dbl = true;
    bull.sector = -1;
    bull.rad = 0;
    std::snprintf(bull.name, sizeof bull.name, "BULL");
    beds_.push_back(bull);
    Bed outer;
    outer.score = 25;
    outer.dbl = false;
    outer.sector = -2;
    outer.rad = (kBullIn + kBullOut) * 0.5f;
    std::snprintf(outer.name, sizeof outer.name, "25");
    beds_.push_back(outer);
    std::sort(beds_.begin(), beds_.end(), [](const Bed& a, const Bed& b) {
        if (a.score != b.score) return a.score > b.score;
        if (a.dbl != b.dbl) return a.dbl > b.dbl;
        return a.sector < b.sector;
    });
}

void Game::place(const Bed& b, float& x, float& y) const {
    if (b.rad <= 0.f) {
        x = kCx;
        y = kCy;
        return;
    }
    float sector = b.sector < 0 ? 0.f : float(b.sector);
    float ang = sector * (3.14159265f / 10.f);
    x = kCx + std::sin(ang) * b.rad;
    y = kCy - std::cos(ang) * b.rad;
}

Game::Hit Game::scoreAt(float x, float y) const {
    Hit h;
    std::snprintf(h.name, sizeof h.name, "MISS");
    float dx = x - kCx;
    float dy = y - kCy;
    float r = std::hypot(dx, dy);
    if (r > kDoubOut) return h;
    if (r <= kBullIn) {
        h.score = 50;
        h.dbl = true;
        std::snprintf(h.name, sizeof h.name, "BULL");
        return h;
    }
    if (r <= kBullOut) {
        h.score = 25;
        std::snprintf(h.name, sizeof h.name, "25");
        return h;
    }
    int s = sectorAt(dx, dy);
    int n = kSeg[s];
    char kind = 'S';
    int mul = 1;
    if (r > kTripIn && r <= kTripOut) {
        kind = 'T';
        mul = 3;
    } else if (r > kDoubIn) {
        kind = 'D';
        mul = 2;
        h.dbl = true;
    }
    h.score = n * mul;
    std::snprintf(h.name, sizeof h.name, "%c%d", kind, n);
    return h;
}

bool Game::audit() {
    for (const Bed& b : beds_) {
        float x, y;
        place(b, x, y);
        Hit h = scoreAt(x, y);
        if (h.score != b.score || h.dbl != b.dbl || std::strcmp(h.name, b.name) != 0) {
            std::fprintf(stderr, "s3dart bed %s scored %s (%d) at %.2f %.2f\n", b.name, h.name, h.score, x, y);
            return false;
        }
    }
    return !beds_.empty();
}

bool Game::finish(int remain, int darts, Bed* plan, int& n) const {
    n = 0;
    if (darts <= 0 || remain < 2) return false;
    // Last dart is a double, so the ceiling is trebles plus the bull.
    if (remain > 60 * (darts - 1) + 50) return false;
    for (const Bed& b : beds_) {
        int left = remain - b.score;
        if (left == 0) {
            if (!b.dbl) continue;
            plan[0] = b;
            n = 1;
            return true;
        }
        if (left < 2 || darts == 1) continue;
        int sub = 0;
        if (finish(left, darts - 1, plan + 1, sub)) {
            plan[0] = b;
            n = sub + 1;
            return true;
        }
    }
    return false;
}

Game::Bed Game::bestSetup(int remain) const {
    for (const Bed& b : beds_)
        if (remain - b.score >= 2) return b;
    return beds_.back();
}

const Game::Bed& Game::target() const { return have_ ? plan_[0] : setup_; }

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    buildBeds();
    rules_ = audit();
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.setFogColor(gs::rgb4(1, 1, 2));
    sys.apu.setMaster(0.72f);
    if (bot_) begin();
    else mode_ = Mode::Title;
}

void Game::clearPins() {
    visitN_ = 0;
    for (int i = 0; i < 3; i++) pin_[i].on = false;
}

void Game::begin() {
    remain_ = 501;
    visitStart_ = 501;
    thrown_ = 0;
    visitDart_ = 0;
    won_ = false;
    over_ = false;
    busting_ = false;
    closeVisit_ = false;
    last_[0] = 0;
    out_[0] = 0;
    aimX_ = kCx;
    aimY_ = kCy;
    fanStep_ = -1;
    clearPins();
    enterAim();
}

void Game::enterAim() {
    int left = 3 - visitDart_;
    if (left < 1) left = 1;
    have_ = finish(remain_, left, plan_, planN_);
    if (!have_) setup_ = bestSetup(remain_);
    mode_ = Mode::Aim;
}

uint32_t Game::rnd() {
    rng_ = rng_ * 1664525u + 1013904223u;
    return rng_;
}

float Game::pulse() const { return 0.5f * (1.f + std::sin(t_ * 6.1f)); }

bool Game::wantsThrow() const {
    const gs::Pad& p = sys_->pad;
    return p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C) || p.pressed(gs::BTN_TURBO);
}

void Game::moveAim() {
    float mx = 0, my = 0;
    const gs::Pad& p = sys_->pad;
    if (p.down(gs::BTN_LEFT)) mx -= 1.f;
    if (p.down(gs::BTN_RIGHT)) mx += 1.f;
    if (p.down(gs::BTN_UP)) my -= 1.f;
    if (p.down(gs::BTN_DOWN)) my += 1.f;
    float m = std::hypot(mx, my);
    if (m > 0.f) {
        aimX_ += mx / m * kSpeed;
        aimY_ += my / m * kSpeed;
    }
    float lim = kDoubOut + 16.f;
    aimX_ = std::clamp(aimX_, kCx - lim, kCx + lim);
    aimY_ = std::clamp(aimY_, kCy - lim, kCy + lim);
}

void Game::launch() {
    if (mode_ != Mode::Aim) return;
    float p = pulse();
    float err = 0.f;
    if (p > kGreen) {
        float u = (p - kGreen) / (1.f - kGreen);
        err = u * u * 42.f;
    }
    float dir = float(rnd() & 1023) * (kTau / 1024.f);
    landX_ = aimX_ + std::cos(dir) * err;
    landY_ = aimY_ + std::sin(dir) * err;
    float ox, oy;
    cluster(visitN_, ox, oy);
    fromX_ = kCx;
    fromY_ = float(gs::SCREEN_H) + 18.f;
    destX_ = landX_ + ox;
    destY_ = landY_ + oy;
    flightT_ = 0;
    mode_ = Mode::Flight;
    if (sys_) sys_->apu.noiseBurst(0.15f, 2300.f, 0.04f);
}

void Game::stick() {
    Hit h = scoreAt(landX_, landY_);
    int slot = visitN_;
    if (slot < 3) {
        pin_[slot].x = landX_;
        pin_[slot].y = landY_;
        pin_[slot].on = true;
        std::snprintf(visit_[slot], sizeof visit_[slot], "%s", h.name);
        visitN_++;
    }
    thrown_++;
    visitDart_++;
    std::snprintf(last_, sizeof last_, "%s", h.name);

    int next = remain_ - h.score;
    // A non-double that lands on 0, a leave of 1, or a negative is a visit bust.
    bool game = h.score > 0 && next == 0 && h.dbl;
    bool bust = h.score > 0 && !game && next < 2;

    if (h.score == 0) {
        if (sys_) sys_->apu.noiseBurst(0.1f, 160.f, 0.07f);
        showT_ = 0.4f;
        mode_ = Mode::Show;
        if (visitDart_ >= 3) closeVisit_ = true;
        return;
    }
    if (game) {
        remain_ = 0;
        won_ = true;
        over_ = true;
        std::snprintf(out_, sizeof out_, "%s", h.name);
        mode_ = Mode::Win;
        fanStep_ = 0;
        fanT_ = 0;
        if (sys_ && !sys_->headless) sys_->rumble(0.35f, 0.75f, 180);
        return;
    }
    if (bust) {
        remain_ = visitStart_;
        busting_ = true;
        showT_ = 0.75f;
        mode_ = Mode::Bust;
        if (sys_) {
            sys_->apu.tone(0, 98.f, 0.1f);
            toneT_ = 0.3f;
            sys_->apu.noiseBurst(0.2f, 80.f, 0.14f);
        }
        return;
    }
    remain_ -= h.score;
    float freq = 392.f;
    if (h.name[0] == 'T') freq = 698.f;
    else if (h.dbl) freq = 587.f;
    else if (h.score >= 25) freq = 784.f;
    if (sys_) {
        sys_->apu.tone(0, freq, 0.07f);
        toneT_ = 0.08f;
    }
    if (visitDart_ >= 3) closeVisit_ = true;
    showT_ = 0.38f;
    mode_ = Mode::Show;
}

void Game::afterMark() {
    if (busting_ || closeVisit_ || visitDart_ >= 3) {
        busting_ = false;
        closeVisit_ = false;
        clearPins();
        visitDart_ = 0;
        visitStart_ = remain_;
    }
    enterAim();
}

void Game::botAim() {
    float x, y;
    place(target(), x, y);
    float dx = x - aimX_;
    float dy = y - aimY_;
    float d = std::hypot(dx, dy);
    if (d <= kSpeed) {
        aimX_ = x;
        aimY_ = y;
        if (pulse() < 0.08f) launch();
    } else {
        aimX_ += dx / d * kSpeed;
        aimY_ += dy / d * kSpeed;
    }
}

void Game::tickAudio(float dt) {
    if (!sys_) return;
    if (toneT_ > 0.f) {
        toneT_ -= dt;
        if (toneT_ <= 0.f) sys_->apu.tone(0, 0, 0);
    }
    if (fanStep_ < 0) return;
    fanT_ -= dt;
    if (fanT_ > 0.f) return;
    const float notes[4] = {523.25f, 659.25f, 783.99f, 1046.5f};
    if (fanStep_ < 4) {
        sys_->apu.tone(1, notes[fanStep_], 0.1f);
        fanStep_++;
        fanT_ = 0.11f;
    } else {
        sys_->apu.tone(1, 0, 0);
        fanStep_ = -1;
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    const gs::Pad& p = sys.pad;
    if (mode_ == Mode::Title) {
        if (p.pressed(gs::BTN_START) || wantsThrow()) begin();
    } else if (mode_ == Mode::Aim) {
        if (!bot_) {
            if (p.pressed(gs::BTN_START)) mode_ = Mode::Pause;
            else {
                moveAim();
                if (wantsThrow()) launch();
            }
        } else {
            botAim();
        }
    } else if (mode_ == Mode::Flight) {
        flightT_++;
        if (flightT_ >= kFlight) stick();
    } else if (mode_ == Mode::Show || mode_ == Mode::Bust) {
        showT_ -= kDt;
        if (showT_ <= 0.f || (!bot_ && (wantsThrow() || p.pressed(gs::BTN_START)))) afterMark();
    } else if (mode_ == Mode::Win) {
        if (p.pressed(gs::BTN_START) || wantsThrow()) begin();
    } else if (mode_ == Mode::Pause) {
        if (p.pressed(gs::BTN_START) || wantsThrow()) mode_ = Mode::Aim;
    }
    tickAudio(kDt);
    draw();
}

void Game::blit(const gs::Image& img, float cx, float cy, int w, int h, int pal) {
    if (!sys_ || w < 1 || h < 1 || img.w == 0) return;
    gs::Sprite s;
    s.w = int16_t(w);
    s.h = int16_t(h);
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = img;
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::boardSprite() {
    gs::Sprite s;
    s.img = art_.board;
    s.x = int16_t(kBoardX);
    s.y = int16_t(kBoardY);
    s.w = int16_t(art_.board.w);
    s.h = int16_t(art_.board.h);
    s.pal = PAL_BOARD;
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (!sys_ || row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        int tile = art_.font[c - 32];
        if (!tile) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(tile, pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        v.lineBackdrop[y] = y < kBoardY ? gs::rgb4(1, 1, 2) : gs::rgb4(2, 1, 1);
    }
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    if (mode_ == Mode::Title) blit(art_.title, kCx, 58.f, art_.title.w, art_.title.h, PAL_TITLE);
    if (mode_ == Mode::Bust) blit(art_.bust, kCx, kCy - 8.f, art_.bust.w, art_.bust.h, PAL_BUST);
    if (mode_ == Mode::Win) blit(art_.win, kCx, kCy - 6.f, art_.win.w, art_.win.h, PAL_WIN);

    char num[8];
    std::snprintf(num, sizeof num, "%d", remain_);
    int len = int(std::strlen(num));
    int gap = 1;
    float span = float(len * art_.digitW + (len - 1) * gap);
    float left = 160.f - span * 0.5f;
    for (int i = 0; i < len; i++) {
        int d = num[i] - '0';
        if (d < 0 || d > 9) continue;
        blit(art_.digit[d], left + art_.digitW * 0.5f, 13.f, art_.digit[d].w, art_.digit[d].h, PAL_SCORE);
        left += float(art_.digitW + gap);
    }

    if (mode_ == Mode::Aim || mode_ == Mode::Pause) {
        float rad = 5.f + pulse() * 24.f;
        int pal = pulse() <= kGreen ? PAL_SWEET : PAL_AIM;
        for (int i = 0; i < 12; i++) {
            float a = t_ * 1.7f + float(i) * (kTau / 12.f);
            blit(art_.dot, aimX_ + std::cos(a) * rad, aimY_ + std::sin(a) * rad, art_.dot.w, art_.dot.h, pal);
        }
        blit(art_.cross, aimX_, aimY_, art_.cross.w, art_.cross.h, pal);
    }

    auto dartAt = [&](float x, float y, float h, int slot) {
        gs::Sprite s;
        int ih = std::max(1, int(std::lround(h)));
        int iw = std::max(1, int(std::lround(h * float(art_.dart.w) / float(std::max(1, int(art_.dart.h))))));
        s.w = int16_t(iw);
        s.h = int16_t(ih);
        s.x = int16_t(std::lround(x - s.w * 0.5f));
        s.y = int16_t(std::lround(y - s.h * 0.5f));
        s.img = art_.dart.pick(float(ih));
        s.pal = uint8_t(PAL_DART0 + (slot % 3));
        sys_->vdp.sprite(s);
    };

    if (mode_ == Mode::Flight) {
        float u = std::clamp(float(flightT_) / float(kFlight), 0.f, 1.f);
        float e = u * u * (3.f - 2.f * u);
        float x = fromX_ + (destX_ - fromX_) * e;
        float y = fromY_ + (destY_ - fromY_) * e;
        float h = 22.f + (12.f - 22.f) * e;
        dartAt(x, y, h, visitN_);
    }
    for (int i = 0; i < 3; i++) {
        if (!pin_[i].on) continue;
        float ox, oy;
        cluster(i, ox, oy);
        dartAt(pin_[i].x + ox, pin_[i].y + oy, 12.f, i);
    }
    if (mode_ == Mode::Title) dartAt(kCx, kCy, 12.f, 0);

    boardSprite();

    hud(0, 0, "S3 DART", PAL_INK);
    hud(33, 0, "DBL OUT", PAL_GOLD);
    if (mode_ == Mode::Title) {
        hud(0, 8, "ARROWS", PAL_INK);
        hud(0, 9, "AIM", PAL_GOLD);
        hud(0, 12, "THROW", PAL_INK);
        hud(0, 13, "GREEN", PAL_GREEN);
        hud(33, 8, "Z C", PAL_GOLD);
        hud(33, 9, "SPACE", PAL_INK);
        hud(33, 14, "START", PAL_GREEN);
        return;
    }
    if (mode_ == Mode::Pause) hud(0, 16, "PAUSED", PAL_GOLD);
    if (mode_ == Mode::Win) hud(0, 16, "START", PAL_GREEN);

    if (have_) {
        hud(0, 4, "OUT", PAL_GREEN);
        for (int i = 0; i < planN_ && i < 3; i++) hud(0, 6 + i, plan_[i].name, PAL_GOLD);
    } else if (!beds_.empty()) {
        hud(0, 4, "SET", PAL_GOLD);
        hud(0, 6, setup_.name, PAL_INK);
    }
    hud(0, 11, "DARTS", PAL_INK);
    char buf[8];
    std::snprintf(buf, sizeof buf, "%d", thrown_);
    hud(0, 12, buf, PAL_GOLD);

    hud(33, 4, "VISIT", PAL_INK);
    for (int i = 0; i < 3; i++) {
        const char* s = i < visitN_ ? visit_[i] : "---";
        int pal = PAL_INK;
        if (i < visitN_) {
            if (s[0] == 'T') pal = PAL_RED;
            else if (s[0] == 'D' || std::strcmp(s, "BULL") == 0) pal = PAL_GREEN;
        }
        hud(33, 6 + i, s, pal);
    }
    hud(33, 11, "LAST", PAL_INK);
    hud(33, 12, last_[0] ? last_ : "----", PAL_GOLD);
    if (thrown_ == 0 && mode_ == Mode::Aim) {
        hud(0, 18, "GREEN", PAL_GREEN);
        hud(0, 19, "THROW", PAL_INK);
    }
}

}  // namespace dart

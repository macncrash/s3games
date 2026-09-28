#include "game/chime.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace paradechime {
namespace {

constexpr float kStartX = 160.f;
constexpr float kStartY = 206.f;
constexpr float kTowerX = 160.f;
constexpr float kSquareY = 40.f;
constexpr float kClimb = 1.7f;
constexpr float kHalf = 20.f;
constexpr float kBody = 6.f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

const char* Game::phase() const {
    switch (mode_) {
    case Mode::Title: return "title";
    case Mode::March: return "march";
    case Mode::Hurt: return "hurt";
    case Mode::Chime: return "chime";
    case Mode::Leave: return "leave";
    case Mode::Fail: return "fail";
    case Mode::Over: return "over";
    case Mode::Pause: return "pause";
    }
    return "march";
}

int Game::clockSec() const {
    if (chimed_ || mode_ == Mode::Chime || mode_ == Mode::Leave) return kHourSec;
    int s = kStartSec + playFrames_ / kFpc;
    if (s > kHourSec + 4) s = kHourSec + 4;
    return s;
}

int Game::hour() const { return clockSec() / 3600; }
int Game::minute() const { return (clockSec() / 60) % 60; }
int Game::second() const { return clockSec() % 60; }

bool Game::inSquare() const { return py_ <= 50.f && px_ > 70.f && px_ < 250.f; }

void Game::toTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    chimed_ = false;
    lives_ = 3;
    tries_ = 0;
    age_ = 0;
    playFrames_ = 0;
    hold_ = 0;
    strikes_ = 0;
    chimeFrames_ = 0;
    flash_ = 0;
    time_ = 0.f;
    px_ = kStartX;
    py_ = kStartY;
    bellSwing_ = 0.f;
    faceLeft_ = false;
    reason_ = "hour silent";
}

void Game::beginMarch() {
    toTitle();
    tries_ = 1;
    mode_ = Mode::March;
    sys_->setLight(40, 60, 130);
}

void Game::respawn() {
    px_ = kStartX;
    py_ = kStartY;
    faceLeft_ = false;
    tries_++;
    mode_ = Mode::March;
    hold_ = 0;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(4, 5, 8));
    lanes_[0] = {162.f, 0.72f, 150.f, 18.f, 0};
    lanes_[1] = {118.f, -0.88f, 164.f, 70.f, 1};
    lanes_[2] = {76.f, 0.64f, 142.f, 30.f, 2};
    toTitle();
}

int Game::laneAt(float y) const {
    for (int i = 0; i < 3; i++) {
        if (std::fabs(y - lanes_[i].y) <= 13.f) return i;
    }
    return -1;
}

int Game::nextLane(float y) const {
    int best = -1;
    float bestY = -1.f;
    for (int i = 0; i < 3; i++) {
        if (lanes_[i].y < y - 14.f && lanes_[i].y > bestY) {
            bestY = lanes_[i].y;
            best = i;
        }
    }
    return best;
}

float Game::gapX(const Lane& lane, float x) const {
    float base = lane.phase + lane.speed * time_;
    float n = std::round((x - base) / lane.spacing - 0.5f);
    float g = base + (n + 0.5f) * lane.spacing;
    float best = g;
    float bestD = 1e9f;
    for (int k = -4; k <= 4; k++) {
        float c = g + float(k) * lane.spacing;
        while (c < -20.f) c += lane.spacing;
        while (c > 340.f) c -= lane.spacing;
        if (c < 24.f || c > 296.f) continue;
        float d = std::fabs(c - x);
        if (d < bestD) {
            bestD = d;
            best = c;
        }
    }
    if (bestD > 1e8f) best = clampf(g, 24.f, 296.f);
    return best;
}

bool Game::struck(float x, float y) const {
    for (const Lane& lane : lanes_) {
        if (std::fabs(y - lane.y) > 12.f) continue;
        float base = lane.phase + lane.speed * time_;
        float u = std::fmod(x - base, lane.spacing);
        if (u < 0.f) u += lane.spacing;
        float dist = std::min(u, lane.spacing - u);
        if (dist < kHalf + kBody) return true;
    }
    return false;
}

void Game::reachHour() {
    chimed_ = true;
    won_ = true;
    mode_ = Mode::Chime;
    hold_ = 0;
    strikes_ = 0;
    chimeFrames_ = 0;
    flash_ = 8;
    reason_ = "CHIME";
    sys_->setLight(220, 180, 40);
    sys_->rumble(0.25f, 0.55f, 200);
}

void Game::missHour() {
    won_ = false;
    chimed_ = false;
    over_ = true;
    mode_ = Mode::Fail;
    reason_ = "hour passed";
    hold_ = 0;
    sys_->setLight(90, 20, 20);
}

void Game::dieTry() {
    lives_--;
    flash_ = 10;
    sys_->rumble(0.7f, 0.15f, 120);
    sys_->apu.noiseBurst(0.32f, 0.45f, 0.18f);
    if (lives_ <= 0) {
        mode_ = Mode::Over;
        over_ = true;
        won_ = false;
        chimed_ = false;
        reason_ = "tries spent";
        hold_ = 0;
        sys_->setLight(80, 10, 10);
        return;
    }
    mode_ = Mode::Hurt;
    hold_ = 0;
}

void Game::botMove() {
    if (inSquare() || py_ < 58.f) {
        float dx = clampf(kTowerX - px_, -1.2f, 1.2f);
        px_ += dx;
        if (py_ > kSquareY) py_ -= kClimb * 0.6f;
        faceLeft_ = dx < -0.15f;
        return;
    }
    int inside = laneAt(py_);
    if (inside >= 0) {
        const Lane& lane = lanes_[inside];
        float g = gapX(lane, px_);
        float vx = lane.speed * 0.35f + clampf(g - px_, -1.2f, 1.2f);
        px_ = clampf(px_ + vx, 16.f, 304.f);
        py_ -= kClimb;
        faceLeft_ = vx < 0.f;
        return;
    }
    int nxt = nextLane(py_);
    if (nxt >= 0 && py_ < lanes_[nxt].y + 36.f) {
        float g = gapX(lanes_[nxt], px_);
        float dx = clampf(g - px_, -2.1f, 2.1f);
        px_ = clampf(px_ + dx, 16.f, 304.f);
        faceLeft_ = dx < -0.2f;
        if (std::fabs(g - px_) < 5.f) py_ -= kClimb;
        return;
    }
    float dx = clampf(kTowerX - px_, -1.4f, 1.4f);
    px_ += dx;
    py_ -= kClimb;
    faceLeft_ = dx < -0.2f;
}

void Game::humanMove() {
    const gs::Pad& pad = sys_->pad;
    float dx = 0.f, dy = 0.f;
    if (pad.down(gs::BTN_LEFT)) dx -= 1.f;
    if (pad.down(gs::BTN_RIGHT)) dx += 1.f;
    if (pad.down(gs::BTN_UP)) dy -= 1.f;
    if (pad.down(gs::BTN_DOWN)) dy += 1.f;
    if (dx == 0.f && std::fabs(pad.axisX) > 0.28f) dx = pad.axisX;
    if (dy == 0.f && std::fabs(pad.axisY) > 0.28f) dy = -pad.axisY;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len > 1.f) {
        dx /= len;
        dy /= len;
    }
    px_ = clampf(px_ + dx * 1.85f, 12.f, 308.f);
    py_ = clampf(py_ + dy * 1.85f, 28.f, 214.f);
    if (dx < -0.2f) faceLeft_ = true;
    if (dx > 0.2f) faceLeft_ = false;
}

void Game::logic() {
    time_ += 1.f;
    if (bot_) botMove();
    else humanMove();
    py_ = clampf(py_, 28.f, 214.f);
    if (struck(px_, py_)) {
        dieTry();
        return;
    }
    playFrames_++;
    if (clockSec() >= kHourSec) {
        if (inSquare()) reachHour();
        else missHour();
    }
}

void Game::audio() {
    if (mode_ == Mode::March && (age_ % 30) == 0) {
        sys_->apu.tone(0, 180.f, 0.04f);
    } else if (mode_ == Mode::March && (age_ % 30) == 4) {
        sys_->apu.tone(0, 0.f, 0.f);
    }
    if (mode_ == Mode::Chime) {
        chimeFrames_++;
        bellSwing_ += 0.22f;
        if ((chimeFrames_ % 8) == 1 && strikes_ < 12) {
            strikes_++;
            float f = 520.f + float(strikes_ % 3) * 30.f;
            sys_->apu.keyOn(0, f, 0.35f);
            sys_->apu.tone(1, f * 0.5f, 0.06f);
        }
        if ((chimeFrames_ % 8) == 6) {
            sys_->apu.keyOff(0);
            sys_->apu.tone(1, 0.f, 0.f);
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    age_++;
    if (flash_ > 0) flash_--;
    const gs::Pad& pad = sys.pad;
    bool start = pad.pressed(gs::BTN_START);
    bool fire = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_C);

    if (mode_ == Mode::Title) {
        time_ += 1.f;
        if ((bot_ && age_ > 18) || (!bot_ && (start || fire))) beginMarch();
    } else if (mode_ == Mode::March) {
        if (!bot_ && start) {
            held_ = Mode::March;
            mode_ = Mode::Pause;
        } else {
            logic();
        }
    } else if (mode_ == Mode::Pause) {
        if (start || fire) mode_ = held_;
    } else if (mode_ == Mode::Hurt) {
        time_ += 1.f;
        playFrames_++;
        hold_++;
        if (clockSec() > kHourSec + 1) missHour();
        else if (hold_ > 28) respawn();
    } else if (mode_ == Mode::Chime) {
        hold_++;
        if (strikes_ >= 12 && chimeFrames_ > 12 * 8 + 16) {
            mode_ = Mode::Leave;
            over_ = true;
        }
    } else if ((mode_ == Mode::Leave || mode_ == Mode::Fail || mode_ == Mode::Over) && !bot_ && (start || fire)) {
        toTitle();
    }
    audio();
    draw();
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (!sys_ || h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    if (s.x >= gs::SCREEN_W || s.y >= gs::SCREEN_H || s.x + s.w <= 0 || s.y + s.h <= 0) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal) {
    if (!sys_ || w < 1.f || h < 1.f) return;
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(std::max(h, 2.f));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::word(const gs::Image& img, float cx, float cy, int pal) {
    gs::Sprite s;
    s.w = int16_t(img.w);
    s.h = int16_t(img.h);
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = img;
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!sys_ || !s) return;
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
    int n = 0;
    while (s[n]) n++;
    hud(std::max(0, (40 - n) / 2), row, s, pal);
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t c;
        if (y < 22) c = gs::rgb4(5, 7, 12);
        else if (y < 64) c = gs::rgb4(10, 9, 6);
        else if ((y / 8) % 2 == 0) c = gs::rgb4(4, 4, 5);
        else c = gs::rgb4(5, 5, 6);
        if (flash_ > 4 && (y & 3) == 0) c = gs::rgb4(15, 13, 6);
        vdp.lineBackdrop[y] = c;
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }

    word(art_.title, 160.f, mode_ == Mode::Title ? 86.f : -40.f, PAL_INK);
    if (mode_ != Mode::Title) {
        bool blink = mode_ == Mode::Hurt && ((hold_ / 3) & 1);
        if (!blink) spr(art_.major, px_, py_, 28.f, PAL_MAJOR, faceLeft_);
    } else {
        spr(art_.major, 160.f, 150.f, 42.f, PAL_MAJOR, false);
    }
    float swing = std::sin(bellSwing_) * (mode_ == Mode::Chime || mode_ == Mode::Leave ? 8.f : 1.2f);
    spr(art_.bell, kTowerX + swing, 22.f, 16.f, PAL_BELL, swing < 0.f);
    spr(art_.tower, kTowerX, 34.f, 52.f, PAL_TOWER, false);

    for (int i = 0; i < 3; i++) stamp(art_.stripe, 160.f, lanes_[i].y, 320.f, 3.f, (i & 1) ? PAL_GOLD : PAL_CREAM);

    for (const Lane& lane : lanes_) {
        float base = lane.phase + lane.speed * time_;
        for (int n = -2; n < 6; n++) {
            float x = base + float(n) * lane.spacing;
            x = std::fmod(x, lane.spacing * 6.f);
            if (x < -40.f) x += lane.spacing * 6.f;
            if (x < -48.f || x > 368.f) continue;
            bool flip = lane.speed < 0.f;
            if (lane.kind == 1) spr(art_.horse, x, lane.y - 2.f, 20.f, PAL_HORSE, flip);
            else if (lane.kind == 2) spr(art_.drum, x, lane.y - 1.f, 18.f, PAL_DRUM, flip);
            else spr(art_.wagon, x, lane.y, 22.f, PAL_WAGON, flip);
        }
    }
    for (int i = 0; i < 10; i++) {
        float x = 12.f + float((i * 47) % 296);
        float y = 6.f + float(i % 2) * 8.f;
        spr(art_.pennant, x, y + std::sin(time_ * 0.07f + i) * 1.4f, 14.f, (i % 3) ? PAL_FLAG : PAL_GOLD, (i & 1) != 0);
    }
    for (int side = 0; side < 2; side++) {
        float x = side ? 306.f : 14.f;
        for (int i = 0; i < 5; i++) {
            float y = 78.f + float(i) * 26.f;
            spr(art_.person, x, y, 15.f, (i + side) & 1 ? PAL_CROWD : PAL_FLAG, side == 0);
        }
    }
    for (int i = 0; i < 8; i++) {
        float x = std::fmod(16.f + i * 41.f + time_ * 0.35f, 340.f) - 8.f;
        float y = std::fmod(40.f + i * 23.f + time_ * 0.2f, 190.f);
        spr(art_.confetti, x, y, 4.f, PAL_CONF, false);
    }

    if (mode_ == Mode::Title) {
        hudC(20, "WAIT FOR THE HOUR", PAL_GOLD);
        hudC(22, bot_ ? "MARCHING" : "A  MARCH", PAL_HUD);
        hudC(24, "STAND IN THE SQUARE", PAL_CREAM);
    } else if (mode_ == Mode::Pause) {
        hudC(12, "PAUSED", PAL_GOLD);
    } else {
        char line[48];
        std::snprintf(line, sizeof(line), "%d:%02d:%02d", hour(), minute(), second());
        hud(1, 26, line, chimed_ ? PAL_GOLD : PAL_HUD);
        std::snprintf(line, sizeof(line), "TRY %d", std::max(1, tries_));
        hud(16, 26, line, PAL_HUD);
        std::snprintf(line, sizeof(line), "LEFT %d", std::max(0, lives_));
        hud(28, 26, line, lives_ < 3 ? PAL_FLAG : PAL_HUD);
        if (mode_ == Mode::Chime || mode_ == Mode::Leave) hudC(3, "THE HOUR CHIMES", PAL_GOLD);
        else if (mode_ == Mode::Fail) hudC(3, "THE HOUR PASSED", PAL_FLAG);
        else if (mode_ == Mode::Over) hudC(3, "TRIES SPENT", PAL_FLAG);
        else if (mode_ == Mode::Hurt) hudC(3, "HIT", PAL_FLAG);
        else if (inSquare()) hudC(3, "HOLD FOR NOON", PAL_CREAM);
        else hudC(3, "REACH THE SQUARE", PAL_CREAM);
    }
}

}  // namespace paradechime

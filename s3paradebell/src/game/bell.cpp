#include "game/bell.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace paradebell {
namespace {

constexpr float kStartX = 160.f;
constexpr float kStartY = 204.f;
constexpr float kBellX = 160.f;
constexpr float kBellY = 30.f;
constexpr float kClimb = 1.55f;
constexpr float kHalf = 22.f;
constexpr float kBody = 6.f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

const char* Game::phase() const {
    switch (mode_) {
    case Mode::Title: return "title";
    case Mode::March: return "march";
    case Mode::Dead: return "dead";
    case Mode::Ring: return "ring";
    case Mode::Leave: return "leave";
    case Mode::Over: return "over";
    case Mode::Pause: return "pause";
    }
    return "march";
}

void Game::toTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    rung_ = false;
    dead_ = 0;
    tryNo_ = 0;
    age_ = 0;
    clock_ = 0.f;
    time_ = 0.f;
    px_ = kStartX;
    py_ = kStartY;
    bellSwing_ = 0.f;
    bellAmp_ = 0.12f;
    hold_ = 0.f;
    flash_ = 0.f;
    faceLeft_ = false;
}

void Game::beginMarch() {
    toTitle();
    tryNo_ = 1;
    mode_ = Mode::March;
    sys_->setLight(40, 70, 140);
}

void Game::respawn() {
    px_ = kStartX;
    py_ = kStartY;
    faceLeft_ = false;
    tryNo_++;
    mode_ = Mode::March;
    hold_ = 0.f;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(5, 6, 8));
    lanes_[0] = {158.f, 0.85f, 168.f, 20.f, 0};
    lanes_[1] = {112.f, -1.05f, 176.f, 90.f, 1};
    lanes_[2] = {68.f, 0.95f, 160.f, 40.f, 0};
    toTitle();
}

int Game::laneAt(float y) const {
    for (int i = 0; i < 3; i++) {
        if (std::fabs(y - lanes_[i].y) <= 12.f) return i;
    }
    return -1;
}

int Game::nextLane(float y) const {
    int best = -1;
    float bestY = -1.f;
    for (int i = 0; i < 3; i++) {
        if (lanes_[i].y < y - 12.f && lanes_[i].y > bestY) {
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
    for (int k = -3; k <= 3; k++) {
        float c = g + float(k) * lane.spacing;
        if (c < 28.f || c > 292.f) continue;
        float d = std::fabs(c - x);
        if (d < bestD) {
            bestD = d;
            best = c;
        }
    }
    if (bestD > 1e8f) best = clampf(g, 28.f, 292.f);
    return best;
}

bool Game::struck(float x, float y) const {
    for (const Lane& lane : lanes_) {
        if (std::fabs(y - lane.y) > 11.f) continue;
        float base = lane.phase + lane.speed * time_;
        float u = std::fmod(x - base, lane.spacing);
        if (u < 0.f) u += lane.spacing;
        float dist = std::min(u, lane.spacing - u);
        if (dist < kHalf + kBody) return true;
    }
    return false;
}

void Game::reachBell() {
    rung_ = true;
    won_ = true;
    mode_ = Mode::Ring;
    hold_ = 0.f;
    bellAmp_ = 1.f;
    flash_ = 1.f;
    sys_->setLight(220, 180, 40);
    sys_->rumble(0.2f, 0.6f, 180);
}

void Game::dieTry() {
    dead_++;
    flash_ = 1.f;
    sys_->rumble(0.7f, 0.2f, 140);
    sys_->apu.noiseBurst(0.35f, 0.4f, 0.2f);
    if (dead_ >= 3) {
        mode_ = Mode::Over;
        over_ = true;
        won_ = false;
        rung_ = false;
        hold_ = 0.f;
        sys_->setLight(80, 10, 10);
        return;
    }
    mode_ = Mode::Dead;
    hold_ = 0.f;
}

void Game::botMove() {
    int inside = laneAt(py_);
    if (inside >= 0) {
        const Lane& lane = lanes_[inside];
        float g = gapX(lane, px_);
        float vx = lane.speed + clampf(g - px_, -1.3f, 1.3f);
        px_ = clampf(px_ + vx, 18.f, 302.f);
        py_ -= kClimb;
        faceLeft_ = vx < 0.f;
        return;
    }
    int nxt = nextLane(py_);
    if (nxt >= 0 && py_ < lanes_[nxt].y + 32.f) {
        float g = gapX(lanes_[nxt], px_);
        float dx = clampf(g - px_, -2.f, 2.f);
        px_ = clampf(px_ + dx, 18.f, 302.f);
        faceLeft_ = dx < -0.2f;
        if (std::fabs(g - px_) < 4.f) py_ -= kClimb;
        return;
    }
    float dx = clampf(kBellX - px_, -1.4f, 1.4f);
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
    if (dx == 0.f && std::fabs(pad.axisX) > 0.3f) dx = pad.axisX;
    if (dy == 0.f && std::fabs(pad.axisY) > 0.3f) dy = -pad.axisY;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len > 1.f) {
        dx /= len;
        dy /= len;
    }
    px_ = clampf(px_ + dx * 1.7f, 16.f, 304.f);
    py_ = clampf(py_ + dy * 1.7f, 16.f, 210.f);
    if (dx < -0.2f) faceLeft_ = true;
    if (dx > 0.2f) faceLeft_ = false;
}

void Game::logic(float dt) {
    time_ += dt * 60.f;
    if (bot_) botMove();
    else humanMove();
    if (struck(px_, py_)) {
        dieTry();
        return;
    }
    if (py_ < 44.f && std::fabs(px_ - kBellX) < 18.f) reachBell();
}

void Game::audio() {
    gs::APU& apu = sys_->apu;
    if (mode_ == Mode::Ring || mode_ == Mode::Leave) {
        bellSwing_ += 0.22f;
        bellAmp_ *= 0.992f;
        float f = 494.f + std::sin(bellSwing_) * 18.f;
        apu.tone(0, f, 0.18f * bellAmp_);
        apu.tone(1, f * 2.f, 0.06f * bellAmp_);
        return;
    }
    if (mode_ == Mode::March) {
        int beat = int(time_) / 18;
        float vol = (int(time_) % 18) < 3 ? 0.08f : 0.f;
        apu.tone(2, (beat & 1) ? 196.f : 146.f, vol);
    } else {
        apu.tone(2, 0.f, 0.f);
    }
    if (mode_ != Mode::Ring) {
        apu.tone(0, 0.f, 0.f);
        apu.tone(1, 0.f, 0.f);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    age_++;
    clock_ += 1.f / 60.f;
    if (flash_ > 0.f) flash_ = std::max(0.f, flash_ - 0.04f);
    const gs::Pad& pad = sys.pad;
    bool start = pad.pressed(gs::BTN_START);
    bool fire = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C);
    if (mode_ == Mode::Title) {
        if ((bot_ && clock_ > 0.35f) || (!bot_ && (start || fire))) beginMarch();
    } else if (mode_ == Mode::March) {
        if (!bot_ && start) {
            held_ = Mode::March;
            mode_ = Mode::Pause;
        } else {
            logic(1.f / 60.f);
        }
    } else if (mode_ == Mode::Pause) {
        if (start || fire) mode_ = held_;
    } else if (mode_ == Mode::Dead) {
        hold_ += 1.f / 60.f;
        if (hold_ > 0.55f) respawn();
    } else if (mode_ == Mode::Ring) {
        hold_ += 1.f / 60.f;
        bellSwing_ += 0.28f;
        if (hold_ > 1.1f) {
            mode_ = Mode::Leave;
            over_ = true;
        }
    } else if ((mode_ == Mode::Leave || mode_ == Mode::Over) && !bot_ && (start || fire)) {
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
    s.img = m.pick(h);
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
        if (y < 18) c = gs::rgb4(4, 6, 10);
        else if (y < 52) c = gs::rgb4(9, 8, 4);
        else if ((y / 6) % 2 == 0) c = gs::rgb4(4, 4, 5);
        else c = gs::rgb4(5, 5, 6);
        if (flash_ > 0.4f && (y & 3) == 0) c = gs::rgb4(15, 14, 8);
        vdp.lineBackdrop[y] = c;
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }
    for (int i = 0; i < 3; i++) {
        float y = lanes_[i].y;
        stamp(art_.block, 160.f, y, 320.f, 4.f, (i & 1) ? PAL_GOLD : PAL_CREAM);
    }
    for (int i = 0; i < 12; i++) {
        float x = 14.f + float((i * 53) % 300);
        float y = 8.f + float((i % 3) * 6);
        bool flip = (i & 1) != 0;
        spr(art_.pennant, x, y + std::sin(time_ * 0.08f + i) * 1.5f, 16.f, (i % 3 == 0) ? PAL_FLAG : PAL_RED, flip);
    }
    for (int side = 0; side < 2; side++) {
        float x = side ? 304.f : 16.f;
        for (int i = 0; i < 6; i++) {
            float y = 64.f + float(i) * 24.f;
            int pal = (i + side) & 1 ? PAL_CROWD : PAL_FLAG;
            spr(art_.person, x, y, 16.f, pal, side == 0);
        }
    }
    for (const Lane& lane : lanes_) {
        float base = lane.phase + lane.speed * time_;
        float span = lane.spacing * 4.f;
        float c = base - std::ceil((base + 60.f) / lane.spacing) * lane.spacing;
        for (int n = 0; n < 8; n++) {
            float x = c + float(n) * lane.spacing;
            if (x < -50.f || x > 370.f) continue;
            bool flip = lane.speed < 0.f;
            if (lane.kind) spr(art_.horse, x, lane.y - 2.f, 22.f, PAL_HORSE, flip);
            else spr(art_.wagon, x, lane.y - 1.f, 24.f, PAL_FLOAT, flip);
        }
        (void)span;
    }
    for (int i = 0; i < 10; i++) {
        float x = std::fmod(20.f + i * 37.f + time_ * (0.4f + (i & 1)), 340.f) - 10.f;
        float y = std::fmod(30.f + i * 19.f + time_ * 0.25f, 200.f);
        spr(art_.confetti, x, y, 4.f, PAL_CONF, false);
    }
    float swing = std::sin(bellSwing_) * (mode_ == Mode::Ring || mode_ == Mode::Leave ? 10.f : 1.5f);
    spr(art_.shadow, kBellX, kBellY + 16.f, 8.f, PAL_SHADE, false);
    spr(art_.bell, kBellX + swing, kBellY, 28.f, PAL_BELL, swing < 0.f);
    if (mode_ != Mode::Title) {
        spr(art_.shadow, px_, py_ + 12.f, 8.f, PAL_SHADE, false);
        bool blink = mode_ == Mode::Dead && (int(hold_ * 12.f) & 1);
        if (!blink) spr(art_.major, px_, py_, 30.f, PAL_MAJOR, faceLeft_);
    }
    if (mode_ == Mode::Title) {
        word(art_.title, 160.f, 78.f, PAL_INK);
        spr(art_.major, 160.f, 150.f, 40.f, PAL_MAJOR, false);
        hudC(20, "THREE TRIES", PAL_GOLD);
        hudC(22, bot_ ? "MARCHING" : "A  MARCH", PAL_HUD);
        hudC(24, "RING THE BELL", PAL_CREAM);
    } else if (mode_ == Mode::Pause) {
        hudC(12, "PAUSED", PAL_GOLD);
    } else {
        char line[40];
        std::snprintf(line, sizeof(line), "TRY %d", std::max(1, tryNo_));
        hud(1, 26, line, PAL_HUD);
        std::snprintf(line, sizeof(line), "DEAD %d", dead_);
        hud(30, 26, line, dead_ ? PAL_RED : PAL_HUD);
        if (mode_ == Mode::Ring || mode_ == Mode::Leave) hudC(4, "THE BELL RINGS", PAL_GOLD);
        else if (mode_ == Mode::Over) hudC(4, "THIRD TRY DIED", PAL_RED);
        else if (mode_ == Mode::Dead) hudC(4, "TRY DIED", PAL_RED);
        else hudC(4, "REACH THE BELL", PAL_CREAM);
    }
}

}  // namespace paradebell

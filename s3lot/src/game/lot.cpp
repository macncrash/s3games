#include "game/lot.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <functional>
#include <string>

#include "version.h"

namespace lot {
namespace {

constexpr int kClock0 = 40 * 60;
constexpr float kSpeed = 2.1f;
constexpr float kBallSp = 5.8f;
constexpr float kPlayerR = 6.f;
constexpr float kBoardR = 14.f;
constexpr float kBallR = 3.5f;
constexpr float kSway = 7.f;
constexpr float kStand = 44.f;
constexpr float kLane = 196.f;
constexpr float kLeft = 30.f;
constexpr float kRight = 290.f;
constexpr float kBottom = 204.f;
constexpr float kTopShut = 78.f;
constexpr float kTopOpen = 20.f;
constexpr float kGateL = 136.f;
constexpr float kGateR = 184.f;
constexpr float kWinY = 50.f;

constexpr float kHomeX[3] = {82.f, 242.f, 160.f};
constexpr float kHomeY[3] = {104.f, 98.f, 132.f};
constexpr float kPhase0[3] = {0.6f, 2.4f, 4.2f};
constexpr int kPalT[3] = {PAL_T1, PAL_T2, PAL_T3};

struct Circ {
    float x, y, r;
};

constexpr Circ kJunk[] = {
    {128.f, 168.f, 12.f},
    {200.f, 118.f, 12.f},
    {50.f, 154.f, 10.f},
    {268.f, 178.f, 8.f},
    {42.f, 108.f, 4.f},
};

float swayAt(float t, float phase) { return std::sin(t * 0.046f + phase) * kSway; }

}  // namespace

void Game::setFace(float x, float y) {
    float d = std::hypot(x, y);
    if (d < 0.01f) return;
    faceX_ = x / d;
    faceY_ = y / d;
}

float Game::targetX(int i) const {
    if (down_[i]) return fellX_[i];
    return homeX_[i] + swayAt(t_, phase_[i]);
}

float Game::targetY(int i) const { return homeY_[i]; }

bool Game::solid() const {
    if (px_ < kLeft || px_ > kRight || py_ > kBottom) return true;
    bool gap = gateOpen() && px_ > kGateL && px_ < kGateR;
    if (py_ < (gap ? kTopOpen : kTopShut)) return true;
    for (const Circ& c : kJunk)
        if (std::hypot(px_ - c.x, py_ - c.y) < c.r + kPlayerR) return true;
    for (int i = 0; i < 3; i++) {
        if (down_[i]) continue;
        float tx = targetX(i), ty = targetY(i);
        if (std::hypot(px_ - tx, py_ - ty) < 11.f + kPlayerR) return true;
        if (std::hypot(px_ - tx, py_ - (ty + 18.f)) < 6.f + kPlayerR) return true;
    }
    return false;
}

void Game::tryMove(float dx, float dy) {
    px_ += dx;
    if (solid()) px_ -= dx;
    py_ += dy;
    if (solid()) py_ -= dy;
}

bool Game::stepTo(float x, float y) {
    float dx = x - px_, dy = y - py_;
    float d = std::hypot(dx, dy);
    if (d <= 1.3f) {
        float ox = px_, oy = py_;
        px_ = x;
        py_ = y;
        if (solid()) {
            px_ = ox;
            py_ = oy;
            return false;
        }
        return true;
    }
    float s = std::min(kSpeed, d);
    tryMove(dx / d * s, dy / d * s);
    return false;
}

void Game::blip(float freq, float vol, int frames) {
    sys_->apu.tone(0, freq, vol);
    toneLeft_ = frames;
}

void Game::chord(float a, float b, float c, int frames) {
    sys_->apu.tone(0, a, 0.06f);
    sys_->apu.tone(1, b, 0.05f);
    sys_->apu.tone(2, c, 0.045f);
    chordLeft_ = frames;
    toneLeft_ = 0;
}

void Game::tickAudio() {
    if (chordLeft_ > 0) {
        if (--chordLeft_ == 0) {
            sys_->apu.tone(0, 0, 0);
            sys_->apu.tone(1, 0, 0);
            sys_->apu.tone(2, 0, 0);
        }
        return;
    }
    if (toneLeft_ > 0 && --toneLeft_ == 0) sys_->apu.tone(0, 0, 0);
}

void Game::toss() {
    if (ballOn_ || cool_ > 0 || mode_ != Mode::Play) return;
    ballOn_ = true;
    bx_ = px_ + faceX_ * 11.f;
    by_ = py_ + faceY_ * 11.f;
    bvx_ = faceX_ * kBallSp;
    bvy_ = faceY_ * kBallSp;
    ballLife_ = 70;
    cool_ = 8;
    blip(720.f, 0.05f, 4);
    sys_->apu.noiseBurst(0.06f, 2800.f, 0.04f);
}

void Game::begin() {
    px_ = 160.f;
    py_ = kLane;
    faceX_ = 0;
    faceY_ = -1;
    hits_ = 0;
    clock_ = kClock0;
    cool_ = 0;
    ballOn_ = false;
    gateLift_ = 0;
    over_ = false;
    won_ = false;
    botTarget_ = 0;
    botPhase_ = 0;
    puffs_.clear();
    for (int i = 0; i < 3; i++) {
        down_[i] = false;
        fellX_[i] = homeX_[i];
        homeX_[i] = kHomeX[i];
        homeY_[i] = kHomeY[i];
        phase_[i] = kPhase0[i];
    }
    mode_ = Mode::Play;
    if (sys_) sys_->apu.silence();
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    for (int i = 0; i < 3; i++) {
        homeX_[i] = kHomeX[i];
        homeY_[i] = kHomeY[i];
        phase_[i] = kPhase0[i];
    }
    if (bot_) begin();
    else mode_ = Mode::Title;
}

void Game::driveHuman() {
    const gs::Pad& pad = sys_->pad;
    float x = 0, y = 0;
    if (pad.down(gs::BTN_LEFT)) x -= 1;
    if (pad.down(gs::BTN_RIGHT)) x += 1;
    if (pad.down(gs::BTN_UP)) y -= 1;
    if (pad.down(gs::BTN_DOWN)) y += 1;
    if (std::fabs(pad.axisX) > 0.3f) x = pad.axisX;
    if (x != 0 || y != 0) {
        setFace(x, y);
        float d = std::hypot(x, y);
        tryMove(faceX_ * kSpeed * std::min(d, 1.f), faceY_ * kSpeed * std::min(d, 1.f));
    }
    if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_TURBO)) toss();
}

void Game::driveBot() {
    if (botTarget_ >= 3) {
        setFace(0, -1);
        stepTo(160.f, 28.f);
        return;
    }
    int i = botTarget_;
    if (down_[i]) {
        botTarget_++;
        botPhase_ = 0;
        return;
    }
    float tx = targetX(i);
    float ty = targetY(i);
    if (botPhase_ == 0) {
        setFace(0, 1);
        if (stepTo(px_, kLane)) botPhase_ = 1;
    } else if (botPhase_ == 1) {
        setFace(homeX_[i] - px_, 0);
        if (stepTo(homeX_[i], kLane)) botPhase_ = 2;
    } else {
        setFace(0, -1);
        float gy = ty + kStand;
        stepTo(tx, gy);
        if (!ballOn_ && cool_ == 0 && std::fabs(px_ - tx) < 1.8f && std::fabs(py_ - gy) < 3.2f) {
            float ox = px_, oy = py_;
            px_ = tx;
            py_ = gy;
            if (solid()) {
                px_ = ox;
                py_ = oy;
            } else {
                toss();
            }
        }
    }
}

void Game::moveBall() {
    if (cool_ > 0) cool_--;
    for (auto& p : puffs_) p.t += 1.f;
    puffs_.erase(std::remove_if(puffs_.begin(), puffs_.end(), [](const Puff& p) { return p.t > 22.f; }), puffs_.end());
    if (!ballOn_) return;
    bx_ += bvx_;
    by_ += bvy_;
    auto burst = [&](float x, float y) {
        for (int n = 0; n < 4; n++) puffs_.push_back({x + (n - 1.5f) * 3.f, y + (n % 2) * 2.f, float(n)});
    };
    auto stop = [&]() { ballOn_ = false; };
    bool inGap = bx_ > kGateL && bx_ < kGateR && gateOpen();
    if (--ballLife_ <= 0 || bx_ < 16.f || bx_ > 306.f || by_ > 222.f || by_ < 8.f || (by_ < kTopShut && !inGap)) {
        burst(bx_, by_);
        stop();
        return;
    }
    for (const Circ& c : kJunk) {
        if (std::hypot(bx_ - c.x, by_ - c.y) < c.r + kBallR) {
            burst(bx_, by_);
            sys_->apu.noiseBurst(0.12f, 900.f, 0.08f);
            stop();
            return;
        }
    }
    for (int i = 0; i < 3; i++) {
        if (down_[i]) continue;
        float tx = targetX(i), ty = targetY(i);
        if (std::hypot(bx_ - tx, by_ - ty) > kBoardR + kBallR) continue;
        fellX_[i] = tx;
        down_[i] = true;
        hits_++;
        burst(tx, ty);
        blip(180.f + hits_ * 90.f, 0.07f, 8);
        sys_->apu.noiseBurst(0.18f, 1400.f, 0.1f);
        sys_->rumble(0.35f, 0.15f, 70);
        if (hits_ == 3) chord(392.f, 523.f, 659.f, 28);
        stop();
        return;
    }
}

void Game::celebrate() {
    if (!gateOpen() || px_ <= kGateL || px_ >= kGateR || py_ >= kWinY) return;
    mode_ = Mode::Win;
    over_ = true;
    won_ = true;
    ballOn_ = false;
    chord(523.f, 659.f, 784.f, 40);
    sys_->rumble(0.5f, 0.8f, 160);
}

void Game::timeUp() {
    mode_ = Mode::Lose;
    over_ = true;
    won_ = false;
    ballOn_ = false;
    blip(90.f, 0.08f, 24);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    if (mode_ != Mode::Pause) t_ += 1.f;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_TURBO))
            begin();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else {
            if (bot_) driveBot();
            else driveHuman();
            moveBall();
            if (gateOpen()) gateLift_ = std::min(1.f, gateLift_ + 0.06f);
            celebrate();
            if (mode_ == Mode::Play) {
                if (clock_ > 0) clock_--;
                if (clock_ == 0) timeUp();
                else if (clock_ <= 10 * 60 && clock_ % 60 == 0) blip(480.f, 0.04f, 4);
            }
        }
    } else if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C))) {
        begin();
    }
    tickAudio();
    draw();
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool shadow, int fog) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(std::max(m.h, 1));
    gs::Sprite s;
    s.w = int16_t(std::lround(std::clamp(w, 1.f, 480.f)));
    s.h = int16_t(std::lround(std::clamp(h, 1.f, 480.f)));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::text(const std::string& s, float x, float y, float h, int pal) {
    if (s.empty()) return;
    const gs::Mipped& sample = art_.glyph['A' - 32];
    if (sample.h < 1) return;
    float adv = h * float(sample.w) / float(sample.h);
    float left = x - adv * float(s.size()) * 0.5f;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < 32 || c > 126) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, left + (float(i) + 0.5f) * adv, y, h, pal, false);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = std::clamp(y / 46.f, 0.f, 1.f);
        int r = int(4 + u * 10);
        int g = int(2 + u * 6);
        int b = int(9 - u * 5);
        v.lineBackdrop[y] = gs::rgb4(r, g, b);
        int fog = 0;
        if (y > 56 && y < 200) fog = (200 - y) / 40;
        v.lineFog[y] = uint8_t(std::clamp(fog, 0, 4));
    }

    if (mode_ == Mode::Title) {
        text("S3 LOT", 160, 14, 20, PAL_GOLD);
        text("THREE TARGETS, THEN THE GATE", 160, 30, 11, PAL_HUD);
        if ((int(t_) / 30) % 2 == 0) text("START    ARROWS    C THROW", 160, 42, 10, PAL_GOLD);
    } else if (mode_ == Mode::Win) {
        text("THROUGH THE GATE", 160, 108, 18, PAL_OK);
        char buf[32];
        std::snprintf(buf, sizeof buf, "%.0f SECONDS LEFT", std::ceil(clockLeft()));
        text(buf, 160, 132, 12, PAL_HUD);
    } else if (mode_ == Mode::Lose) {
        text("OUT OF TIME", 160, 108, 20, PAL_ALERT);
        text("START TO TRY AGAIN", 160, 132, 12, PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160, 108, 20, PAL_GOLD);
    }

    spr(art_.sun, 292, 26, 20, PAL_SKY, false);
    const float stars[][2] = {{18, 24}, {46, 16}, {74, 30}, {108, 20}, {250, 18}, {270, 34}};
    for (auto& s : stars) spr(art_.star, s[0], s[1], 7, PAL_SKY, false);

    struct Item {
        float y;
        std::function<void()> fn;
    };
    std::vector<Item> items;
    auto kid = art_.kidFront;
    bool flip = false;
    if (std::fabs(faceX_) > std::fabs(faceY_) + 0.05f) {
        kid = art_.kidSide;
        flip = faceX_ < 0;
    } else if (faceY_ < 0) {
        kid = art_.kidBack;
    }
    float bob = (mode_ == Mode::Play) ? std::sin(t_ * 0.35f) * 0.8f : 0;
    items.push_back({py_, [&, kid, flip, bob] {
                         spr(art_.shadow, px_, py_ + 14, 9, 0, false, true);
                         spr(kid, px_, py_ + bob, 46, PAL_KID, flip);
                     }});
    if (mode_ == Mode::Play) {
        items.push_back({-10.f, [&] {
                             for (int n = 1; n <= 3; n++)
                                 spr(art_.dot, px_ + faceX_ * (12.f + n * 8.f), py_ + faceY_ * (12.f + n * 8.f), 5, PAL_GOLD, false);
                         }});
    }
    for (int i = 0; i < 3; i++) {
        items.push_back({targetY(i) + 24.f, [this, i] {
                             float x = targetX(i), y = targetY(i);
                             spr(art_.shadow, x, y + 28, 8, 0, false, true);
                             const gs::Mipped& pic = down_[i] ? art_.targetDown : art_.targetUp;
                             spr(pic, x, y + (down_[i] ? 8.f : 0.f), 58, kPalT[i], false);
                         }});
    }
    items.push_back({168.f, [&] { spr(art_.drum, 128, 168, 36, PAL_JUNK, false); }});
    items.push_back({118.f, [&] { spr(art_.drum, 200, 118, 34, PAL_JUNK, false); }});
    items.push_back({154.f, [&] { spr(art_.crate, 50, 150, 32, PAL_JUNK, false); }});
    items.push_back({178.f, [&] { spr(art_.tire, 268, 176, 26, PAL_JUNK, false); }});
    items.push_back({108.f, [&] { spr(art_.lamp, 42, 96, 56, PAL_JUNK, false); }});
    const float weeds[][2] = {{96, 186}, {214, 188}, {150, 120}};
    for (auto& w : weeds)
        items.push_back({w[1], [&, w] { spr(art_.weed, w[0], w[1], 20, PAL_JUNK, false); }});
    items.push_back({78.f, [&] { spr(art_.post, 124, 74, 64, PAL_GATE, false); }});
    items.push_back({78.f, [&] { spr(art_.post, 196, 74, 64, PAL_GATE, false); }});
    for (int i = 0; i < 4; i++) {
        float y = 62.f + i * 9.f - gateLift_ * 50.f;
        items.push_back({y, [&, y] { spr(art_.bar, 160, y, 12, PAL_GATE, false); }});
    }
    if (ballOn_) items.push_back({by_ + 20.f, [&] { spr(art_.ball, bx_, by_, 12, PAL_BALL, false); }});
    for (size_t pi = 0; pi < puffs_.size(); pi++) {
        items.push_back({puffs_[pi].y, [this, pi] {
                             float k = puffs_[pi].t / 22.f;
                             spr(art_.puff, puffs_[pi].x, puffs_[pi].y - k * 8.f, 10.f + k * 10.f, PAL_FX, false, false, int(k * 10));
                         }});
    }

    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) { return a.y > b.y; });
    for (auto& it : items) it.fn();

    if (mode_ == Mode::Title) return;
    hud(0, 0, "S3 LOT", PAL_GOLD);
    char buf[24];
    std::snprintf(buf, sizeof buf, "TARGETS %d/3", hits_);
    hud(12, 0, buf, PAL_HUD);
    int sec = (clock_ + 59) / 60;
    std::snprintf(buf, sizeof buf, "%02ds", sec);
    hud(35, 0, buf, (clock_ <= 10 * 60 && mode_ == Mode::Play) ? PAL_ALERT : PAL_HUD);
    if (gateOpen()) hud(0, 1, "GATE IS OPEN  RUN", PAL_OK);
    else if (mode_ == Mode::Play) hud(0, 1, "ARROWS MOVE    C THROWS", PAL_DIM);
    else if (mode_ == Mode::Pause) hud(0, 1, "START CONTINUES", PAL_HUD);
    hud(28, 27, S3_VERSION_STRING, PAL_DIM);
}

}  // namespace lot

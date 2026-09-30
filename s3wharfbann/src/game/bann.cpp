#include "game/bann.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace wharfbann {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr float kWorld = 2480.0f;
constexpr float kFloor = 186.0f;
constexpr float kHome = 168.0f;
constexpr float kSpawn = 250.0f;
constexpr float kBanner0 = 2140.0f;
constexpr float kMinX = 90.0f;
constexpr float kMaxX = 2320.0f;
constexpr float kRun = 188.0f;
constexpr float kCarry = 150.0f;
constexpr float kAccel = 2400.0f;
constexpr float kGrav = 1100.0f;
constexpr float kHop = -340.0f;
constexpr float kBody = 20.0f;
constexpr float kGrab = 30.0f;
constexpr float kWatch = 78.0f;

float approach(float v, float target, float delta) {
    if (v < target) return std::min(target, v + delta);
    return std::max(target, v - delta);
}

}  // namespace

const gs::Mipped& Game::hero() const {
    if (py_ < kFloor - 2.0f) return art_.leap;
    if (std::abs(vx_) > 18.0f) return (int(step_ / 7.0f) & 1) ? art_.walkA : art_.walkB;
    return art_.stand;
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_) return 4;
    if (!has_) return 1;
    if (px_ > 1200.0f) return 2;
    return 3;
}

void Game::resetRun() {
    auto set = [&](int i, float a, float b, float sp) {
        Barrel& w = barrel_[i];
        w.minX = a;
        w.maxX = b;
        w.x = a + (b - a) * (0.25f + 0.2f * i);
        w.speed = sp;
        w.dir = (i & 1) ? -1.0f : 1.0f;
    };
    set(0, 460.0f, 860.0f, 62.0f);
    set(1, 980.0f, 1420.0f, 74.0f);
    set(2, 1540.0f, 2040.0f, 68.0f);
    px_ = kSpawn;
    py_ = kFloor;
    vx_ = vy_ = 0;
    face_ = 1;
    has_ = false;
    bannerX_ = kBanner0;
    lives_ = 4;
    watch_ = kWatch;
    step_ = hop_ = inv_ = dropLock_ = shake_ = 0;
    playT_ = 0;
}

void Game::begin() {
    resetRun();
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
    cam_ = std::clamp(px_ - 140.0f, 0.0f, kWorld - gs::SCREEN_W);
    blip(440.0f, 0.05f, 0.07f);
}

void Game::blip(float freq, float vol, float hold) {
    sys_->apu.tone(0, freq, vol);
    beep_ = hold;
}

void Game::win() {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Victory;
    won_ = true;
    over_ = true;
    vx_ = vy_ = 0;
    shake_ = 0.35f;
    sys_->rumble(0.25f, 0.6f, 180);
    sys_->setLight(30, 120, 160);
    blip(640.0f, 0.08f, 0.2f);
}

void Game::lose() {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Over;
    won_ = false;
    over_ = true;
    vx_ = vy_ = 0;
    sys_->rumble(0.7f, 0.2f, 160);
    sys_->setLight(20, 40, 90);
    sys_->apu.noiseBurst(0.35f, 120.0f, 0.25f);
}

void Game::dunk(float fromX) {
    if (inv_ > 0 || mode_ != Mode::Play) return;
    lives_--;
    inv_ = 1.1f;
    hop_ = 0.2f;
    float away = px_ < fromX ? -1.0f : 1.0f;
    vx_ = away * 160.0f;
    vy_ = -160.0f;
    px_ = std::clamp(px_ + away * 10.0f, kMinX, kMaxX);
    shake_ = 0.8f;
    sys_->apu.noiseBurst(0.4f, 220.0f, 0.16f);
    sys_->rumble(0.5f, 0.3f, 100);
    if (has_) {
        has_ = false;
        bannerX_ = std::clamp(px_ + away * 36.0f, 400.0f, 2060.0f);
        dropLock_ = 0.5f;
    }
    if (lives_ <= 0) lose();
}

void Game::bot(bool& left, bool& right, bool& jump) {
    left = right = jump = false;
    const float goal = has_ ? (kHome - 12.0f) : bannerX_;
    int travel = 0;
    if (goal > px_ + 6.0f) travel = 1;
    else if (goal < px_ - 6.0f) travel = -1;

    bool danger = false;
    for (const Barrel& w : barrel_) {
        float ahead = travel == 0 ? std::abs(w.x - px_) : (w.x - px_) * float(travel);
        float closing = travel == 0 ? w.speed : (w.dir * float(travel) < 0 ? w.speed + kCarry : std::max(0.0f, w.speed - 40.0f));
        float reach = 34.0f + closing * 0.22f;
        if (ahead > 6.0f && ahead < reach) danger = true;
        if (std::abs(w.x - px_) < 26.0f && py_ > kFloor - 6.0f) danger = true;
    }
    const bool grounded = py_ >= kFloor - 1.0f && vy_ >= 0.0f;
    if (danger && grounded && hop_ <= 0.0f) jump = true;

    if (travel > 0) {
        right = true;
        face_ = 1;
    } else if (travel < 0) {
        left = true;
        face_ = -1;
    }
}

void Game::stepPlay(bool left, bool right, bool jump) {
    playT_ += DT;
    watch_ -= DT;
    if (watch_ <= 0.0f) {
        watch_ = 0;
        lose();
        return;
    }
    if (inv_ > 0) inv_ -= DT;
    if (dropLock_ > 0) dropLock_ -= DT;
    if (hop_ > 0) hop_ -= DT;

    for (Barrel& w : barrel_) {
        w.x += w.dir * w.speed * DT;
        if (w.x >= w.maxX) {
            w.x = w.maxX;
            w.dir = -1.0f;
        } else if (w.x <= w.minX) {
            w.x = w.minX;
            w.dir = 1.0f;
        }
    }

    const bool grounded = py_ >= kFloor - 0.6f && vy_ >= 0.0f;
    if (jump && grounded) {
        vy_ = kHop;
        hop_ = 0.22f;
        blip(520.0f, 0.04f, 0.04f);
    }

    float target = 0;
    if (right && !left) target = has_ ? kCarry : kRun;
    if (left && !right) target = has_ ? -kCarry : -kRun;
    if ((right && !left) || (left && !right)) face_ = right ? 1 : -1;
    vx_ = approach(vx_, target, kAccel * DT);
    px_ += vx_ * DT;
    px_ = std::clamp(px_, kMinX, kMaxX);
    vy_ += kGrav * DT;
    py_ += vy_ * DT;
    if (py_ >= kFloor) {
        py_ = kFloor;
        vy_ = 0;
    }
    if (std::abs(vx_) > 24.0f && py_ >= kFloor - 1.0f) step_ += std::abs(vx_) * DT * 0.18f;

    if (has_ && px_ <= kHome && py_ >= kFloor - 2.0f) {
        win();
        return;
    }

    if (inv_ <= 0 && hop_ <= 0 && py_ > kFloor - 22.0f) {
        for (const Barrel& w : barrel_) {
            if (std::abs(w.x - px_) < kBody) {
                dunk(w.x);
                break;
            }
        }
    }

    if (!has_ && dropLock_ <= 0 && py_ > kFloor - 18.0f && std::abs(px_ - bannerX_) < kGrab) {
        has_ = true;
        blip(720.0f, 0.07f, 0.1f);
        sys_->rumble(0.2f, 0.45f, 80);
        sys_->setLight(160, 70, 20);
    }
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 40 || s.y > gs::SCREEN_H + 40 || s.x + s.w < -40 || s.y + s.h < -40) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    float view = cam_;
    if (shake_ > 0.02f) view += std::sin(t_ * 48.0f) * shake_ * 3.0f;
    vdp.B.scroll(int(view * 0.22f), 0);
    vdp.A.scroll(int(view * 0.55f), int(std::sin(t_ * 1.4f) * 2.0f));

    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < 96) vdp.lineBackdrop[y] = gs::rgb4(4 + y / 40, 7 + y / 50, 11);
        else if (y < 150) vdp.lineBackdrop[y] = gs::rgb4(3, 7, 11);
        else vdp.lineBackdrop[y] = gs::rgb4(1, 4 + (y - 150) / 30, 8);
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }

    auto world = [&](const gs::Mipped& m, float wx, float foot, float h, int pal, bool flip) {
        spr(m, wx - view, foot, h, pal, flip, true);
    };

    for (float x = 48.0f; x < kWorld - 40.0f; x += 46.0f) {
        world(art_.plank, x, kFloor + 6.0f, 16, PAL_WOOD, false);
        if (int(x) % 184 < 46) world(art_.rope, x + 10.0f, kFloor - 2.0f, 12, PAL_ROPE, false);
    }
    const float piles[] = {180, 520, 900, 1280, 1660, 2040};
    for (float x : piles) {
        world(art_.bollard, x, kFloor + 2.0f, 30, PAL_WOOD, false);
        world(art_.buoy, x + 28.0f, kFloor + 22.0f + std::sin(t_ * 2.0f + x) * 2.0f, 16, PAL_WATER, false);
    }
    world(art_.crate, 700.0f, kFloor, 18, PAL_BARREL, false);
    world(art_.crate, 1500.0f, kFloor, 18, PAL_BARREL, false);
    for (int i = 0; i < 3; i++) {
        float gx = 200.0f + i * 700.0f + std::sin(t_ * 0.7f + i) * 40.0f;
        float gy = 48.0f + std::sin(t_ * 1.6f + i * 2.0f) * 6.0f;
        spr(art_.gull, gx - view * 0.3f, gy, 10, PAL_HARBOR, (int(t_ + i) & 1), false);
    }

    for (const Barrel& w : barrel_) {
        float roll = std::sin(playT_ * w.speed * 0.08f) * 2.0f;
        world(art_.barrel, w.x, kFloor + roll, 22, PAL_BARREL, w.dir < 0);
    }

    if ((inv_ <= 0) || (int(t_ * 18.0f) & 1)) {
        world(hero(), px_, py_, 60, PAL_SAILOR, face_ < 0);
        if (has_) world(art_.banner, px_ + face_ * 14.0f, py_ - 28.0f, 32, PAL_BANNER, face_ < 0);
    }
    if (!has_) {
        world(art_.staff, bannerX_, kFloor, 78, PAL_WOOD, false);
        float wave = std::sin(t_ * 3.0f) * 2.0f;
        world(art_.banner, bannerX_ + wave, kFloor - 8.0f, 40, PAL_BANNER, false);
    }
    world(art_.staff, 120.0f, kFloor, 86, PAL_WOOD, false);
    world(art_.banner, 128.0f + std::sin(t_ * 2.2f), kFloor - 18.0f, 22, PAL_BANNER, false);

    std::string hearts;
    for (int i = 0; i < lives_; i++) hearts += "O ";
    hud(1, 1, hearts, PAL_HUD);
    int sec = int(watch_ + 0.999f);
    if (sec < 0) sec = 0;
    char clock[16];
    std::snprintf(clock, sizeof(clock), "WATCH %02d", sec);
    hud(16, 1, clock, watch_ < 12.0f ? PAL_BANNER : PAL_HUD);
    if (has_) hud(30, 1, "BANNER", PAL_BANNER);
    else hud(31, 1, "PIER", PAL_HUD);

    auto text = [&](const std::string& s, float x, float y, float scale, int pal) {
        float width = 0;
        for (unsigned char c : s) {
            if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
            if (c == ' ') width += 8.0f * scale;
            else if (c > 32 && c < 128) width += art_.glyph[c - 32].w * scale + scale;
        }
        x -= width * 0.5f;
        for (unsigned char c : s) {
            if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
            if (c == ' ') {
                x += 8.0f * scale;
                continue;
            }
            if (c <= 32 || c >= 128) continue;
            const gs::Mipped& g = art_.glyph[c - 32];
            spr(g, x + g.w * scale * 0.5f, y, float(g.h) * scale, pal, false, false);
            x += g.w * scale + scale;
        }
    };

    if (mode_ == Mode::Title) {
        text("WHARF BANN", 160, 58, 1.15f, PAL_HUD);
        text("BRING THE BANNER BACK", 160, 90, 0.55f, PAL_BANNER);
        text("MISS IT AND THE WATCH IS OVER", 160, 110, 0.42f, PAL_WATER);
        if ((int(t_ * 2.0f) & 1) == 0) text("PRESS START", 160, 142, 0.7f, PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160, 90, 1.0f, PAL_HUD);
    } else if (mode_ == Mode::Victory) {
        text("THE BANNER IS BACK", 160, 72, 0.68f, PAL_BANNER);
        text("AT THE WHARF", 160, 98, 0.62f, PAL_HUD);
    } else if (mode_ == Mode::Over) {
        text("THE WATCH IS OVER", 160, 78, 0.7f, PAL_HUD);
        text("START TO TRY AGAIN", 160, 108, 0.5f, PAL_WATER);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    resetRun();
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    titleHold_ = 0;
    t_ = 0;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    if (shake_ > 0) shake_ = std::max(0.0f, shake_ - DT);
    if (beep_ > 0) {
        beep_ -= DT;
        if (beep_ <= 0) sys.apu.tone(0, 0, 0);
    }

    const gs::Pad& pad = sys.pad;
    bool left = pad.down(gs::BTN_LEFT) || pad.axisX < -0.35f;
    bool right = pad.down(gs::BTN_RIGHT) || pad.axisX > 0.35f;
    bool jump = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B);
    bool start = pad.pressed(gs::BTN_START);

    if (mode_ == Mode::Title) {
        titleHold_++;
        if (bot_ && titleHold_ > 8) begin();
        else if (start || pad.pressed(gs::BTN_A)) begin();
    } else if (mode_ == Mode::Pause) {
        if (start) mode_ = Mode::Play;
    } else if (mode_ == Mode::Play) {
        if (bot_) bot(left, right, jump);
        else if (start) {
            mode_ = Mode::Pause;
            vx_ = 0;
        }
        if (mode_ == Mode::Play) stepPlay(left, right, jump);
    } else if (mode_ == Mode::Over || mode_ == Mode::Victory) {
        if (!bot_ && start) {
            mode_ = Mode::Title;
            titleHold_ = 0;
            over_ = false;
            won_ = false;
            resetRun();
        }
    }

    float want = std::clamp(px_ - 150.0f, 0.0f, kWorld - float(gs::SCREEN_W));
    cam_ = approach(cam_, want, 520.0f * DT);
    draw();
}

}  // namespace wharfbann

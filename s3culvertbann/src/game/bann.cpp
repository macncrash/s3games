#include "game/bann.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace culvertbann {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr float kWorld = 2480.0f;
constexpr float kFloor = 186.0f;
constexpr float kHome = 128.0f;
constexpr float kSpawn = 210.0f;
constexpr float kBanner0 = 2260.0f;
constexpr float kMinX = 90.0f;
constexpr float kMaxX = 2340.0f;
constexpr float kRun = 168.0f;
constexpr float kAccel = 1600.0f;
constexpr float kJump = -470.0f;
constexpr float kGrav = 900.0f;
constexpr float kGrab = 28.0f;
constexpr int kLives = 6;

struct Pit {
    float l, r;
};
const Pit kPits[] = {{430, 510}, {920, 1000}, {1420, 1500}, {1880, 1960}};

float approach(float v, float target, float delta) {
    if (v < target) return std::min(target, v + delta);
    return std::max(target, v - delta);
}

}  // namespace

const gs::Mipped& Game::hero() const {
    if (!grounded_) return art_.walkA;
    if (std::abs(vx_) > 18.0f) return (int(step_ / 8.0f) & 1) ? art_.walkA : art_.walkB;
    return art_.stand;
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_) return 4;
    if (!has_) return 1;
    if (px_ > 1200.0f) return 2;
    return 3;
}

bool Game::inPit(float x) const {
    for (const Pit& p : kPits)
        if (x > p.l && x < p.r) return true;
    return false;
}

bool Game::dryAt(float x, float& left, float& right) const {
    left = kMinX;
    right = kMaxX;
    if (inPit(x)) return false;
    for (const Pit& p : kPits) {
        if (p.r <= x && p.r > left) left = p.r;
        if (p.l >= x && p.l < right) right = p.l;
    }
    return true;
}

float Game::dryX(float x) const {
    if (!inPit(x)) return std::clamp(x, kMinX, kMaxX);
    for (const Pit& p : kPits) {
        if (x > p.l && x < p.r) {
            float mid = (p.l + p.r) * 0.5f;
            return x < mid ? p.l - 16.0f : p.r + 16.0f;
        }
    }
    return lastSafe_;
}

void Game::resetRun() {
    auto set = [&](int i, float a, float b, float sp) {
        Rat& r = rats_[i];
        r.minX = a;
        r.maxX = b;
        r.x = (a + b) * 0.5f;
        r.speed = sp;
        r.dir = (i & 1) ? -1.0f : 1.0f;
        r.stun = 0;
    };
    set(0, 640.0f, 800.0f, 46.0f);
    set(1, 1140.0f, 1300.0f, 52.0f);
    set(2, 1600.0f, 1760.0f, 48.0f);
    set(3, 2040.0f, 2160.0f, 44.0f);
    px_ = kSpawn;
    py_ = kFloor;
    vx_ = vy_ = 0;
    face_ = 1;
    has_ = false;
    grounded_ = true;
    bannerX_ = kBanner0;
    lastSafe_ = kSpawn;
    lives_ = kLives;
    step_ = inv_ = stun_ = dropLock_ = shake_ = 0;
    playT_ = 0;
}

void Game::begin() {
    resetRun();
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
    cam_ = std::clamp(px_ - 140.0f, 0.0f, kWorld - gs::SCREEN_W);
    blip(420.0f, 0.05f, 0.08f);
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
    shake_ = 0.3f;
    sys_->rumble(0.2f, 0.55f, 180);
    sys_->setLight(40, 140, 80);
    blip(640.0f, 0.08f, 0.22f);
}

void Game::lose() {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Over;
    won_ = false;
    over_ = true;
    vx_ = vy_ = 0;
    sys_->rumble(0.65f, 0.15f, 160);
    sys_->setLight(140, 30, 20);
    sys_->apu.noiseBurst(0.3f, 120.0f, 0.22f);
}

void Game::hurt(float fromX) {
    if (inv_ > 0 || stun_ > 0 || mode_ != Mode::Play) return;
    lives_--;
    inv_ = 1.15f;
    stun_ = 0.22f;
    float away = px_ < fromX ? -1.0f : 1.0f;
    vx_ = away * 140.0f;
    float L = kMinX, R = kMaxX;
    if (dryAt(px_, L, R)) px_ = std::clamp(px_ + away * 18.0f, L + 10.0f, R - 10.0f);
    shake_ = 0.7f;
    sys_->apu.noiseBurst(0.32f, 380.0f, 0.1f);
    sys_->rumble(0.5f, 0.2f, 80);
    if (has_) {
        has_ = false;
        bannerX_ = dryX(px_);
        dropLock_ = 0.4f;
    }
    if (lives_ <= 0) lose();
}

void Game::drown() {
    if (mode_ != Mode::Play) return;
    lives_--;
    inv_ = 1.0f;
    stun_ = 0.15f;
    shake_ = 0.9f;
    sys_->apu.noiseBurst(0.4f, 180.0f, 0.16f);
    sys_->rumble(0.35f, 0.45f, 100);
    if (has_) {
        has_ = false;
        bannerX_ = dryX(lastSafe_);
        dropLock_ = 0.45f;
    }
    px_ = lastSafe_;
    py_ = kFloor;
    vx_ = vy_ = 0;
    grounded_ = true;
    if (lives_ <= 0) lose();
}

void Game::bot(bool& left, bool& right, bool& jump) {
    left = right = jump = false;
    const float goal = has_ ? (kHome - 20.0f) : bannerX_;
    int travel = 0;
    if (goal > px_ + 6.0f) travel = 1;
    else if (goal < px_ - 6.0f) travel = -1;
    if (travel > 0) right = true;
    else if (travel < 0) left = true;
    if (!grounded_) return;
    const float dir = travel >= 0 ? 1.0f : -1.0f;
    if (travel == 0) return;
    for (const Pit& p : kPits) {
        float edge = dir > 0 ? p.l : p.r;
        float dist = (edge - px_) * dir;
        if (dist > 8.0f && dist < 30.0f) jump = true;
    }
    for (const Rat& r : rats_) {
        if (r.stun > 0) continue;
        float dist = (r.x - px_) * dir;
        if (dist > 18.0f && dist < 62.0f) jump = true;
    }
}

void Game::stepPlay(bool left, bool right, bool jump) {
    playT_ += DT;
    if (stun_ > 0) stun_ -= DT;
    if (inv_ > 0) inv_ -= DT;
    if (dropLock_ > 0) dropLock_ -= DT;

    for (Rat& r : rats_) {
        if (r.stun > 0) {
            r.stun -= DT;
            continue;
        }
        r.x += r.dir * r.speed * DT;
        if (r.x >= r.maxX) {
            r.x = r.maxX;
            r.dir = -1.0f;
        } else if (r.x <= r.minX) {
            r.x = r.minX;
            r.dir = 1.0f;
        }
    }

    if (stun_ > 0) {
        vx_ = approach(vx_, 0.0f, 500.0f * DT);
    } else {
        float target = 0;
        if (right && !left) target = kRun;
        if (left && !right) target = -kRun;
        if ((right && !left) || (left && !right)) face_ = right ? 1 : -1;
        vx_ = approach(vx_, target, kAccel * DT);
    }

    vy_ += kGrav * DT;
    px_ += vx_ * DT;
    py_ += vy_ * DT;
    px_ = std::clamp(px_, kMinX, kMaxX);
    if (std::abs(vx_) > 24.0f && grounded_) step_ += std::abs(vx_) * DT * 0.16f;

    if (!inPit(px_) && py_ >= kFloor) {
        py_ = kFloor;
        vy_ = 0;
        grounded_ = true;
        float L, R;
        if (dryAt(px_, L, R) && px_ > L + 22.0f && px_ < R - 22.0f) lastSafe_ = px_;
    } else if (py_ > kFloor + 30.0f) {
        drown();
        return;
    } else {
        grounded_ = false;
    }

    if (jump && grounded_ && stun_ <= 0) {
        vy_ = kJump;
        grounded_ = false;
        blip(300.0f, 0.04f, 0.04f);
    }

    if (has_ && grounded_ && px_ <= kHome) {
        win();
        return;
    }

    if (inv_ <= 0) {
        for (Rat& r : rats_) {
            if (std::abs(r.x - px_) > 18.0f) continue;
            if (r.stun > 0) continue;
            if (vy_ > 30.0f && py_ < kFloor - 10.0f && py_ > kFloor - 90.0f) {
                r.stun = 1.4f;
                vy_ = -200.0f;
                blip(220.0f, 0.05f, 0.05f);
                continue;
            }
            if (py_ > kFloor - 24.0f) hurt(r.x);
        }
    }

    if (!has_ && dropLock_ <= 0 && stun_ <= 0 && grounded_ && std::abs(px_ - bannerX_) < kGrab) {
        has_ = true;
        blip(700.0f, 0.07f, 0.1f);
        sys_->rumble(0.15f, 0.4f, 70);
        sys_->setLight(160, 60, 30);
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

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 48 || s.y > gs::SCREEN_H + 48 || s.x + s.w < -48 || s.y + s.h < -48) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
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
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int shade = 1 + (y < 70 ? 0 : (y < 150 ? 1 : 2));
        vdp.lineBackdrop[y] = gs::rgb4(shade, shade + 1, shade + 2);
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }
    float view = cam_;
    if (shake_ > 0.02f) view += std::sin(t_ * 46.0f) * shake_ * 3.0f;

    auto world = [&](const gs::Mipped& m, float wx, float foot, float h, int pal, bool flip, bool feet) {
        spr(m, wx - view, foot, h, pal, flip, feet);
    };

    if ((inv_ <= 0) || (int(t_ * 16.0f) & 1)) {
        world(hero(), px_, py_, 58, PAL_COAT, face_ < 0, true);
        world(art_.lamp, px_ + face_ * 10.0f, py_ - 46.0f, 10, PAL_COAT, false, false);
        if (has_) world(art_.banner, px_ + face_ * 14.0f, py_ - 30.0f, 32, PAL_BANNER, face_ < 0, false);
    }
    for (const Rat& r : rats_) {
        bool step = int(playT_ * 7.0f + r.x * 0.02f) & 1;
        float bob = r.stun > 0 ? 4.0f : (step ? 1.0f : 0.0f);
        world(art_.rat, r.x, kFloor - bob, 18, PAL_RAT, r.dir < 0, true);
    }
    if (!has_) {
        float wave = std::sin(t_ * 3.2f) * 2.0f;
        world(art_.banner, bannerX_ + wave, kFloor - 36.0f, 40, PAL_BANNER, false, false);
        world(art_.grate, bannerX_ + 28.0f, kFloor, 92, PAL_IRON, false, true);
    }
    world(art_.grate, 70.0f, kFloor, 100, PAL_IRON, false, true);

    for (const Pit& p : kPits) {
        float wob = std::sin(t_ * 4.0f + p.l) * 1.5f;
        for (float x = p.l + 10.0f; x < p.r; x += 18.0f)
            world(art_.water, x, kFloor + 6.0f + wob, 16, PAL_WATER, false, true);
    }
    for (float x = 40.0f; x < kWorld; x += 56.0f) {
        world(art_.rib, x, 78.0f, 52, PAL_STONE, false, false);
        if (!inPit(x)) world(art_.slab, x, kFloor + 8.0f, 14, PAL_STONE, false, true);
        if (int(x) % 112 == 0) {
            float drip = 24.0f + std::fmod(t_ * 36.0f + x, 70.0f);
            world(art_.drip, x + 8.0f, drip, 12, PAL_WATER, false, false);
        }
    }

    std::string hearts;
    for (int i = 0; i < lives_; i++) hearts += "O ";
    hud(1, 1, hearts, PAL_HUD);
    if (has_) hud(28, 1, "BANNER", PAL_BANNER);
    else hud(31, 1, "PIPE", PAL_HUD);

    if (mode_ == Mode::Title) {
        text("CULVERT BANN", 160, 58, 1.05f, PAL_HUD);
        text("BRING THE BANNER BACK", 160, 88, 0.5f, PAL_BANNER);
        if ((int(t_ * 2.0f) & 1) == 0) text("PRESS START", 160, 128, 0.65f, PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160, 80, 1.0f, PAL_HUD);
    } else if (mode_ == Mode::Victory) {
        text("THE BANNER IS BACK", 160, 70, 0.65f, PAL_BANNER);
    } else if (mode_ == Mode::Over) {
        text("THE CULVERT KEPT IT", 160, 70, 0.6f, PAL_HUD);
        text("START TO TRY AGAIN", 160, 104, 0.48f, PAL_STONE);
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
    bool jump = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_UP);
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
            left = right = jump = false;
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

    float want = std::clamp(px_ - 140.0f, 0.0f, kWorld - float(gs::SCREEN_W));
    cam_ = approach(cam_, want, 480.0f * DT);
    draw();
}

}  // namespace culvertbann

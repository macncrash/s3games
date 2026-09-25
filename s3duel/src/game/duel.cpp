#include "game/duel.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace duel {
namespace {

constexpr int HORIZON = 120;
constexpr float FEET = 208.f;
constexpr float MAN_H = 102.f;
constexpr float YOU0 = 112.f;
constexpr float RIV0 = 208.f;
constexpr float STEP = 20.f;
constexpr int PACE_SPAN = 42;
constexpr int FACE_LEN = 64;
constexpr int TURN_LEN = 16;
constexpr int RESOLVE_LEN = 70;

const char* PACE_WORD[] = {"ONE", "TWO", "THREE"};

float smooth(float u) {
    u = std::clamp(u, 0.f, 1.f);
    return u * u * (3.f - 2.f * u);
}

}  // namespace

int Game::phase() const {
    switch (mode_) {
        case Mode::Title: return 0;
        case Mode::Face: return 1;
        case Mode::Pace: return 2;
        case Mode::Turn: return 3;
        case Mode::Holster: return 4;
        case Mode::Draw: return 5;
        case Mode::Resolve: return 6;
        case Mode::Over: return 7;
    }
    return 0;
}

uint32_t Game::rnd() {
    rng_ = rng_ * 1664525u + 1013904223u;
    return rng_;
}

void Game::tone(int ch, float freq, float vol, float hold) {
    sys_->apu.tone(ch, freq, vol);
    hold_[ch] = hold;
}

void Game::audioTick() {
    const float dt = 1.f / 60.f;
    for (int ch = 0; ch < 3; ch++) {
        if (hold_[ch] > 0) {
            hold_[ch] -= dt;
            if (hold_[ch] <= 0) sys_->apu.tone(ch, 0, 0);
        }
    }
}

bool Game::fireEdge() const {
    if (bot_) return false;
    const gs::Pad& p = sys_->pad;
    return p.pressed(gs::BTN_A) || p.pressed(gs::BTN_B) || p.pressed(gs::BTN_C) || p.pressed(gs::BTN_TURBO);
}

bool Game::startEdge() const {
    if (bot_) return false;
    return sys_->pad.pressed(gs::BTN_START);
}

void Game::layStreet() {
    int row0 = HORIZON / 8;
    for (int cy = row0; cy < 32; cy++) {
        for (int cx = 0; cx < 64; cx++) {
            int v = (cx * 3 + cy * 5) & 3;
            sys_->vdp.B.set(cx, cy, gs::entry(art_.ground[v], PAL_DUST));
        }
    }
}

void Game::openDuel() {
    mode_ = Mode::Face;
    t_ = 0;
    pace_ = 0;
    foot_ = 0;
    early_ = false;
    settled_ = false;
    playerWon_ = false;
    won_ = false;
    over_ = false;
    holsterWait_ = 28 + int(rnd() % 36);
    rivalDraw_ = 30 + int(rnd() % 18);
    reason_ = "unfinished";
    you_.x = YOU0;
    cole_.x = RIV0;
    you_.pose = POSE_FACE;
    cole_.pose = POSE_FACE;
    you_.flip = false;
    cole_.flip = true;
    shake_ = 0;
    flash_ = 0;
    puff_ = 0;
    tone(0, 523.f, 0.12f, 0.18f);
}

void Game::beginPaces() {
    mode_ = Mode::Pace;
    t_ = 0;
    pace_ = 0;
    you_.flip = true;
    cole_.flip = false;
    you_.pose = POSE_BACK;
    cole_.pose = POSE_BACK;
}

void Game::foul() {
    if (settled_) return;
    settled_ = true;
    early_ = true;
    playerWon_ = false;
    won_ = false;
    reason_ = "fired early";
    mode_ = Mode::Resolve;
    t_ = 0;
    you_.pose = POSE_HURT;
    you_.flip = false;
    cole_.pose = POSE_FACE;
    cole_.flip = true;
    shake_ = 0.35f;
    tone(0, 82.f, 0.2f, 0.4f);
    sys_->apu.noiseBurst(0.16f, 2800.f, 0.03f);
}

void Game::cleanShot() {
    if (settled_) return;
    settled_ = true;
    early_ = false;
    playerWon_ = true;
    reason_ = "clean draw";
    mode_ = Mode::Resolve;
    t_ = 0;
    you_.pose = POSE_FIRE;
    you_.flip = false;
    cole_.pose = POSE_HURT;
    cole_.flip = true;
    shake_ = 1.f;
    flash_ = 6;
    puff_ = 18;
    sys_->apu.noiseBurst(0.9f, 1500.f, 0.12f);
    sys_->rumble(0.6f, 0.9f, 90);
}

void Game::slowShot() {
    if (settled_) return;
    settled_ = true;
    early_ = false;
    playerWon_ = false;
    won_ = false;
    reason_ = "too slow";
    mode_ = Mode::Resolve;
    t_ = 0;
    cole_.pose = POSE_FIRE;
    cole_.flip = true;
    you_.pose = POSE_HURT;
    you_.flip = false;
    shake_ = 1.f;
    flash_ = 6;
    puff_ = 18;
    sys_->apu.noiseBurst(0.9f, 1300.f, 0.12f);
    sys_->rumble(0.9f, 0.4f, 120);
}

void Game::stepPace() {
    if (t_ >= PACE_SPAN * 3) {
        mode_ = Mode::Turn;
        t_ = 0;
        you_.x = YOU0 - 3.f * STEP;
        cole_.x = RIV0 + 3.f * STEP;
        return;
    }
    int f = t_ % PACE_SPAN;
    pace_ = t_ / PACE_SPAN;
    float u = 0;
    if (f >= 8 && f <= 26) u = (f - 8) / 18.f;
    else if (f > 26) u = 1.f;
    float s = smooth(u);
    you_.x = YOU0 - (pace_ + s) * STEP;
    cole_.x = RIV0 + (pace_ + s) * STEP;
    bool stepping = f >= 8 && f <= 26;
    int pose = stepping ? ((f / 6) & 1 ? POSE_STEP : POSE_BACK) : POSE_BACK;
    you_.pose = pose;
    cole_.pose = pose;
    you_.flip = true;
    cole_.flip = false;
    if (f == 0) tone(0, 294.f + pace_ * 60.f, 0.14f, 0.12f);
    if (f == 8) {
        tone(2, 120.f + pace_ * 24.f, 0.1f, 0.06f);
        sys_->apu.noiseBurst(0.22f, 420.f + pace_ * 70.f, 0.045f);
    }
    if (f == 26) foot_ = pace_ + 1;
    t_++;
}

void Game::stepTurn() {
    t_++;
    you_.x = YOU0 - 3.f * STEP;
    cole_.x = RIV0 + 3.f * STEP;
    if (t_ < TURN_LEN / 2) {
        you_.pose = POSE_BACK;
        cole_.pose = POSE_BACK;
        you_.flip = true;
        cole_.flip = false;
    } else {
        you_.pose = POSE_FACE;
        cole_.pose = POSE_FACE;
        you_.flip = false;
        cole_.flip = true;
    }
    if (t_ == TURN_LEN / 2) tone(0, 392.f, 0.1f, 0.1f);
    if (t_ >= TURN_LEN) {
        mode_ = Mode::Holster;
        t_ = 0;
        you_.pose = POSE_FACE;
        cole_.pose = POSE_FACE;
        you_.flip = false;
        cole_.flip = true;
    }
}

void Game::stepResolve() {
    t_++;
    if (playerWon_) {
        you_.pose = t_ < 12 ? POSE_FIRE : POSE_AIM;
        you_.flip = false;
        cole_.pose = t_ < 16 ? POSE_HURT : POSE_DOWN;
        cole_.flip = true;
        if (t_ == 8) tone(0, 523.f, 0.16f, 0.12f);
        if (t_ == 18) tone(0, 659.f, 0.16f, 0.12f);
        if (t_ == 28) tone(0, 784.f, 0.18f, 0.22f);
    } else if (early_) {
        you_.pose = t_ < 28 ? POSE_HURT : POSE_FACE;
        you_.flip = false;
        cole_.pose = POSE_FACE;
        cole_.flip = true;
    } else {
        cole_.pose = t_ < 12 ? POSE_FIRE : POSE_AIM;
        cole_.flip = true;
        you_.pose = t_ < 16 ? POSE_HURT : POSE_DOWN;
        you_.flip = false;
        if (t_ == 10) tone(0, 196.f, 0.16f, 0.25f);
    }
    if (t_ >= RESOLVE_LEN) {
        mode_ = Mode::Over;
        over_ = true;
        won_ = playerWon_;
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    layStreet();
    you_.x = YOU0;
    cole_.x = RIV0;
    you_.pose = POSE_FACE;
    cole_.pose = POSE_FACE;
    you_.flip = false;
    cole_.flip = true;
    mode_ = Mode::Title;
    draw();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    bool fire = fireEdge();
    bool start = startEdge();
    if (flash_ > 0) flash_--;
    if (puff_ > 0) puff_--;
    if (shake_ > 0) {
        shake_ *= 0.86f;
        if (shake_ < 0.03f) shake_ = 0;
    }

    switch (mode_) {
        case Mode::Title:
            if (start || fire || (bot_ && titleT_ > 24)) openDuel();
            else titleT_++;
            break;
        case Mode::Face:
            if (fire) foul();
            else if (++t_ >= FACE_LEN) beginPaces();
            break;
        case Mode::Pace:
            if (fire) foul();
            else stepPace();
            break;
        case Mode::Turn:
            if (fire) foul();
            else stepTurn();
            break;
        case Mode::Holster:
            if (fire) foul();
            else if (++t_ >= holsterWait_) {
                mode_ = Mode::Draw;
                t_ = 0;
                tone(0, 880.f, 0.2f, 0.14f);
                tone(1, 1318.f, 0.12f, 0.16f);
                flash_ = 5;
            } else if (t_ % 22 == 0) {
                tone(1, 98.f, 0.07f, 0.05f);
            }
            break;
        case Mode::Draw:
            if (!settled_) {
                if (bot_ || fire) cleanShot();
                else if (t_ >= rivalDraw_) slowShot();
                else t_++;
            }
            break;
        case Mode::Resolve: stepResolve(); break;
        case Mode::Over:
            if (!bot_ && (start || fire)) openDuel();
            break;
    }
    audioTick();
    draw();
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet, bool shadow) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 80 || s.x + s.w < -80 || s.y > gs::SCREEN_H + 40 || s.y + s.h < -40) return;
    s.img = m.pick(std::max(w, h));
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    float total = 0;
    for (unsigned char c : s) {
        if (c <= 32 || c >= 128) total += 8.f * scale;
        else total += (art_.glyph[c - 32].w + 1) * scale;
    }
    float pen = x - total * 0.5f;
    for (unsigned char c : s) {
        if (c <= 32 || c >= 128) {
            pen += 8.f * scale;
            continue;
        }
        const gs::Mipped& g = art_.glyph[c - 32];
        float gw = g.w * scale;
        spr(g, pen + gw * 0.5f, y, g.h * scale, pal, false);
        pen += (g.w + 1) * scale;
    }
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

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();

    float ox = 0, oy = 0;
    if (shake_ > 0) {
        ox = std::sin(sys_->frame * 1.7f) * 5.f * shake_;
        oy = std::cos(sys_->frame * 2.1f) * 3.f * shake_;
    }

    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        if (y < HORIZON) {
            float u = y / float(HORIZON - 1);
            int r = std::clamp(int(5 + u * 9), 0, 15);
            int g = std::clamp(int(7 + u * 4), 0, 15);
            int b = std::clamp(int(12 - u * 7), 0, 15);
            if (flash_ > 0) {
                int k = flash_ > 3 ? 6 : 3;
                r = std::min(15, r + k);
                g = std::min(15, g + k);
                b = std::min(15, b + k / 2);
            }
            v.lineBackdrop[y] = gs::rgb4(r, g, b);
        } else {
            float u = (y - HORIZON) / float(gs::SCREEN_H - HORIZON);
            int r = std::clamp(int(11 - u * 4), 0, 15);
            int g = std::clamp(int(8 - u * 3), 0, 15);
            int b = std::clamp(int(4 - u * 2), 0, 15);
            v.lineBackdrop[y] = gs::rgb4(r, g, b);
        }
    }

    const char* word = nullptr;
    const char* sub = nullptr;
    int wordPal = PAL_HUD;
    if (mode_ == Mode::Title) {
        text("S3 DUEL", 160 + ox, 30 + oy, 1.7f, PAL_HUD);
        text("THREE PACES", 160 + ox, 56 + oy, 1.05f, PAL_SUN);
    } else if (mode_ == Mode::Face) {
        word = "STAND";
    } else if (mode_ == Mode::Pace) {
        word = PACE_WORD[std::clamp(pace_, 0, 2)];
        wordPal = PAL_HUD;
    } else if (mode_ == Mode::Turn) {
        word = "TURN";
    } else if (mode_ == Mode::Holster) {
        word = "HOLD";
        wordPal = PAL_ALERT;
    } else if (mode_ == Mode::Draw) {
        word = "DRAW";
        wordPal = PAL_SUN;
    } else if (early_) {
        word = "EARLY";
        sub = "YOU LOSE";
        wordPal = PAL_ALERT;
    } else if (playerWon_) {
        word = "CLEAN";
        sub = "DRAW";
        wordPal = PAL_SUN;
    } else if (mode_ == Mode::Resolve || mode_ == Mode::Over) {
        word = "SLOW";
        sub = "HE FIRES";
        wordPal = PAL_ALERT;
    }
    if (word) {
        float pulse = (mode_ == Mode::Draw || mode_ == Mode::Holster) ? 1.f + 0.06f * std::sin(sys_->frame * 0.35f) : 1.f;
        text(word, 160 + ox, 34 + oy, 2.f * pulse, wordPal);
    }
    if (sub) text(sub, 160 + ox, 64 + oy, 1.15f, PAL_HUD);

    auto barrel = [&](const Fighter& f) {
        if (f.pose != POSE_FIRE) return;
        const gs::Mipped& m = art_.pose[f.pose];
        float h = MAN_H;
        float w = h * float(m.w) / float(m.h);
        float left = f.x + ox - w * 0.5f;
        float top = FEET + oy - h;
        float fx = 0.90f;
        if (f.flip) fx = 1.f - fx;
        spr(art_.flash, left + fx * w, top + 0.50f * h, 22, PAL_SUN, false);
    };

    if (mode_ != Mode::Resolve && mode_ != Mode::Over) {
        float bob = std::sin(sys_->frame * 0.14f) * 3.f;
        float top = FEET - MAN_H;
        spr(art_.arrow, you_.x + ox, top - 8 + bob + oy, 14, PAL_SUN, false);
    }

    barrel(you_);
    barrel(cole_);

    float weedX = std::fmod(sys_->frame * 1.15f, 420.f) - 50.f;
    spr(art_.weed, weedX + ox, 198 + oy, 20, PAL_DUST, false, true);

    auto man = [&](const Fighter& f, int pal) {
        float h = f.pose == POSE_DOWN ? 36.f : MAN_H;
        spr(art_.pose[f.pose], f.x + ox, FEET + oy, h, pal, f.flip, true);
        spr(art_.shadow, f.x + ox, FEET + 2 + oy, 12, PAL_HUD, false, true, true);
    };
    man(you_, PAL_YOU);
    man(cole_, PAL_RIVAL);

    if (puff_ > 0) {
        float px = playerWon_ ? cole_.x : you_.x;
        spr(art_.dust, px + ox, FEET - 6 + oy, 16 + (18 - puff_), PAL_DUST, false, true);
    }

    for (int i = 0; i < foot_ && i < 3; i++) {
        spr(art_.print, YOU0 - (i + 1) * STEP + ox, FEET - 1 + oy, 11, PAL_DUST, false, true);
        spr(art_.print, RIV0 + (i + 1) * STEP + ox, FEET - 1 + oy, 11, PAL_DUST, true, true);
    }

    spr(art_.barrel, 128 + ox, 186 + oy, 22, PAL_WOOD, false, true);
    spr(art_.wheel, 148 + ox, 184 + oy, 22, PAL_WOOD, false, true);
    spr(art_.cactus, 22 + ox, 196 + oy, 52, PAL_PLANT, false, true);
    spr(art_.cactus, 304 + ox, 190 + oy, 40, PAL_PLANT, true, true);

    spr(art_.horse, 36 + ox, 168 + oy, 48, PAL_HORSE, false, true);
    spr(art_.fence, 168 + ox, 162 + oy, 28, PAL_WOOD, false, true);
    spr(art_.saloon, 78 + ox, 172 + oy, 90, PAL_WOOD, false, true);
    spr(art_.store, 236 + ox, 166 + oy, 72, PAL_WOOD, false, true);
    spr(art_.tower, 292 + ox, 168 + oy, 108, PAL_WOOD, false, true);

    spr(art_.hill, 160 + ox * 0.3f, HORIZON - 23.f, 46, PAL_LAND, false);

    float bird = std::fmod(sys_->frame * 0.65f, 400.f) - 30.f;
    spr(art_.bird, bird, 42 + std::sin(sys_->frame * 0.1f) * 4.f, 8, PAL_RIVAL, false);
    spr(art_.bird, std::fmod(bird + 180.f, 400.f) - 30.f, 58, 6, PAL_RIVAL, true);

    float cloud = std::fmod(40.f + sys_->frame * 0.12f, 440.f) - 60.f;
    spr(art_.cloud, cloud, 36, 26, PAL_CLOUD, false);
    spr(art_.cloud, std::fmod(cloud + 200.f, 440.f) - 60.f, 22, 18, PAL_CLOUD, false);
    spr(art_.sun, 262, 34, 42, PAL_SUN, false);

    hud(1, 0, "S3 DUEL", PAL_HUD);
    if (mode_ == Mode::Title) {
        hudC(1, "FIRE EARLY AND YOU LOSE", PAL_ALERT);
        hudC(2, "ENTER TO STAND    TAP C ON DRAW", PAL_HUD);
    } else if (mode_ == Mode::Face) {
        hudC(1, "HOLSTER STAYS SHUT", PAL_HUD);
        hud(33, 0, "YOU", PAL_SUN);
    } else if (mode_ == Mode::Pace) {
        std::string mark = std::to_string(pace_ + 1) + "/3";
        hud(36, 0, mark, PAL_SUN);
        hudC(1, "THREE PACES", PAL_HUD);
    } else if (mode_ == Mode::Turn) {
        hudC(1, "TURN AND WAIT", PAL_HUD);
    } else if (mode_ == Mode::Holster) {
        hudC(1, "NOT YET", PAL_ALERT);
    } else if (mode_ == Mode::Draw) {
        hudC(1, "TAP C", PAL_SUN);
    } else if (early_) {
        hudC(1, "FIRED BEFORE THE DRAW", PAL_ALERT);
        if (mode_ == Mode::Over) hudC(2, "ENTER REMATCHES", PAL_HUD);
    } else if (playerWon_) {
        hudC(1, "THREE PACES, CLEAN DRAW", PAL_SUN);
        if (mode_ == Mode::Over) hudC(2, "ENTER REMATCHES", PAL_HUD);
    } else if (mode_ == Mode::Resolve || mode_ == Mode::Over) {
        hudC(1, "COLE WAS FASTER", PAL_ALERT);
        if (mode_ == Mode::Over) hudC(2, "ENTER REMATCHES", PAL_HUD);
    }
}

}  // namespace duel

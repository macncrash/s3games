#include "game/rock.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace rock {
namespace {

constexpr int MINUTE = 60 * 60;
constexpr float BOAT_Y = 168.f;
constexpr float BOAT_R = 12.f;
constexpr float SPEED = 1.75f;
constexpr float ACCEL = 0.16f;
constexpr float BANK = 24.f;  // cliff tiles: three columns
constexpr float SLACK = 4.f;
constexpr int ROCK_PALS[] = {PAL_ROCK, PAL_WARM, PAL_MOSS};

float radiusOf(int kind) { return kind <= 0 ? 7.f : kind == 1 ? 11.f : 15.f; }
int damageOf(int kind) { return kind <= 0 ? 5 : kind == 1 ? 12 : 24; }

// The clear water. Half-width is how far the hull center may wander and still
// miss every rock placed for that frame. It narrows across the minute.
float gapHalf(int f) {
    float t = f / 60.f;
    float u = std::clamp(f / float(MINUTE), 0.f, 1.f);
    float breathe = 3.2f * std::sin(t * 2.05f);
    return std::clamp(40.f - 22.f * u + breathe, 16.f, 46.f);
}

float gapCenter(int f) {
    float t = f / 60.f;
    float u = std::clamp(f / float(MINUTE), 0.f, 1.f);
    float x = 160.f + 66.f * std::sin(t * 0.30f) + 18.f * std::sin(t * 0.74f + 1.1f) +
              12.f * u * u * std::sin(t * 1.25f + 0.4f);
    float half = gapHalf(f);
    float lo = BANK + BOAT_R + half + 8.f;
    float hi = float(gs::SCREEN_W) - BANK - BOAT_R - half - 8.f;
    if (lo > hi) return 160.f;
    return std::clamp(x, lo, hi);
}

float paceAt(int f) {
    float u = std::clamp(f / float(MINUTE), 0.f, 1.f);
    return 1.55f + u * 1.15f;
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title || mode_ == Mode::Pause) return mode_ == Mode::Title ? 0 : 1;
    if (mode_ == Mode::Run) return 1;
    return won_ ? 2 : 3;
}

int Game::report(int frames) const {
    if (won_) {
        std::printf("S3 ROCK  HELD  hull %d  the hull is the score  (%.1f s)\n", hull_, frames / 60.0);
        return 0;
    }
    std::printf("S3 ROCK  OPEN  hull %d  rocks %d  hit %d  (%.1f s)\n", hull_, spawned_, hitFrame_, frames / 60.0);
    return 1;
}

float Game::rnd() {
    rng_ = rng_ * 1664525u + 1013904223u;
    return (rng_ >> 8) * (1.0f / 16777216.0f);
}

void Game::quiet() {
    if (!sys_) return;
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(1, 0, 0);
    sys_->apu.tone(2, 0, 0);
    sys_->apu.noise(0, 0, false);
}

void Game::layoutDecor() {
    static const Decor seed[] = {
        {34, 20, 0.85f, 2, 0, 0, false},  {52, 90, 0.7f, 1, 1, 1, true},
        {28, 160, 0.95f, 0, 2, 2, false}, {292, 36, 0.8f, 2, 2, 1, true},
        {270, 110, 0.65f, 1, 0, 0, false}, {300, 176, 0.9f, 0, 1, 2, true},
        {46, 210, 0.75f, 1, 2, 1, false}, {278, 200, 0.7f, 2, 1, 0, true},
    };
    for (int i = 0; i < 8; i++) decor_[i] = seed[i];
}

void Game::castOff() {
    rng_ = 0xA11CE5u;
    rocks_.clear();
    motes_.clear();
    pops_.clear();
    hull_ = 100;
    playFrame_ = 0;
    lived_ = 0;
    spawned_ = 0;
    inv_ = 0;
    shake_ = 0;
    scrape_ = 0;
    thud_ = 0;
    spawnSide_ = 0;
    hitFrame_ = -1;
    hitDx_ = hitDy_ = 0;
    fan_ = 0;
    fanFrame_ = 0;
    vel_ = 0;
    boatX_ = gapCenter(0);
    mode_ = Mode::Run;
    won_ = false;
    over_ = false;
    if (!sys_) return;
    quiet();
    sys_->apu.tone(2, 330, 0.04f);
    thud_ = 6;
    sys_->setLight(40, 180, 220);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    layoutDecor();
    boatX_ = gapCenter(0);
    hull_ = 100;
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    quiet();
    if (bot_) castOff();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (bot_ && mode_ == Mode::Title) castOff();
    update();
    draw();
}

void Game::steer() {
    float input = 0;
    if (bot_) {
        // A few frames ahead, so the heavy hull sits in the channel rather than chasing it.
        float err = gapCenter(playFrame_ + 5) - boatX_;
        input = std::clamp(err / 10.f, -1.f, 1.f);
    } else {
        if (sys_->pad.down(gs::BTN_LEFT)) input -= 1.f;
        if (sys_->pad.down(gs::BTN_RIGHT)) input += 1.f;
        if (std::fabs(sys_->pad.axisX) > 0.12f) input = sys_->pad.axisX;
        input = std::clamp(input, -1.f, 1.f);
    }
    float wish = input * SPEED;
    vel_ += std::clamp(wish - vel_, -ACCEL, ACCEL);
    boatX_ += vel_;
    if (boatX_ < 8.f) boatX_ = 8.f;
    if (boatX_ > float(gs::SCREEN_W) - 8.f) boatX_ = float(gs::SCREEN_W) - 8.f;
}

bool Game::trySpawn(int kind) {
    const float r = radiusOf(kind);
    float vy = paceAt(playFrame_) + (rnd() - 0.5f) * 0.22f;
    if (vy < 1.35f) vy = 1.35f;
    const float y0 = -r - 8.f;
    float gMin = 1e9f, gMax = -1e9f;
    bool any = false;
    const int maxK = int((gs::SCREEN_H + 48.f + r - y0) / vy) + 2;
    for (int k = 0; k <= maxK; k++) {
        float y = y0 + float(k + 1) * vy;
        if (y > gs::SCREEN_H + r + 6.f) break;
        // Same test the collide loop uses: a circle misses when the centers
        // are at least BOAT_R + r apart. Pad the window so a fractional step
        // cannot skip the overlap.
        if (std::fabs(y - BOAT_Y) > BOAT_R + r + 2.5f) continue;
        int f = playFrame_ + k;
        float g = gapCenter(f);
        float h = gapHalf(f);
        gMin = std::min(gMin, g - h);
        gMax = std::max(gMax, g + h);
        any = true;
    }
    if (!any) return false;
    const float forbidL = gMin - BOAT_R - r - SLACK;
    const float forbidR = gMax + BOAT_R + r + SLACK;
    const float left0 = 6.f;
    const float right1 = float(gs::SCREEN_W) - 6.f;
    float pockets[2][2] = {{left0, forbidL - 2.f}, {forbidR + 2.f, right1}};
    int side = spawnSide_ & 1;
    auto room = [&](int s) { return pockets[s][1] - pockets[s][0]; };
    if (room(side) < 8.f) side ^= 1;
    if (room(side) < 8.f) return false;
    Rock rk;
    rk.x = pockets[side][0] + rnd() * room(side);
    rk.y = y0;
    rk.vy = vy;
    rk.r = r;
    rk.kind = kind;
    rk.var = int(rnd() * 3.f) % 3;
    rk.pal = int(rnd() * 3.f) % 3;
    rk.flip = rnd() >= 0.5f;
    rk.live = true;
    rocks_.push_back(rk);
    spawned_++;
    spawnSide_++;
    return true;
}

void Game::spawnRock() {
    if (int(rocks_.size()) >= 24) return;
    float roll = rnd();
    int first = roll < 0.16f ? 2 : roll < 0.48f ? 1 : 0;
    for (int k = first; k >= 0; --k)
        if (trySpawn(k)) return;
}

void Game::chip(int dmg, float x, float y, int kind) {
    if (dmg < 1) dmg = 1;
    hull_ -= dmg;
    if (hull_ < 0) hull_ = 0;
    shake_ = kind >= 2 ? 8 : 5;
    inv_ = 32;
    thud_ = 10;
    if (hitFrame_ < 0) {
        hitFrame_ = playFrame_;
        hitDx_ = x - boatX_;
        hitDy_ = y - BOAT_Y;
    }
    if (pops_.size() < 6) pops_.push_back(Pop{x, y - 8.f, 42.f, dmg});
    int n = kind < 0 ? 5 : 8 + kind * 2;
    for (int i = 0; i < n && motes_.size() < 36; i++) {
        float a = rnd() * 6.2831853f;
        float s = 0.35f + rnd() * (kind < 0 ? 0.8f : 1.5f);
        Mote m;
        m.x = x;
        m.y = y;
        m.vx = std::cos(a) * s;
        m.vy = std::sin(a) * s * 0.6f + 0.25f;
        m.max = 16.f + rnd() * 14.f;
        m.life = m.max;
        motes_.push_back(m);
    }
    if (!sys_) return;
    float boom = kind >= 2 ? 140.f : kind == 1 ? 90.f : kind < 0 ? 220.f : 70.f;
    sys_->apu.tone(2, boom, kind >= 2 ? 0.07f : 0.05f);
    sys_->apu.noiseBurst(kind >= 2 ? 0.32f : 0.18f, kind < 0 ? 3200.f : 1800.f, 0.07f);
    sys_->rumble(kind >= 2 ? 0.85f : 0.45f, 0.25f, kind >= 2 ? 110 : 70);
}

void Game::collide() {
    if (inv_ > 0) {
        inv_--;
        if (scrape_ > 0) scrape_--;
        return;
    }
    for (auto& rk : rocks_) {
        if (!rk.live) continue;
        float dx = rk.x - boatX_;
        float dy = rk.y - BOAT_Y;
        float rr = rk.r + BOAT_R;
        if (dx * dx + dy * dy < rr * rr) {
            rk.live = false;
            chip(damageOf(rk.kind), rk.x, rk.y, rk.kind);
            if (scrape_ > 0) scrape_--;
            return;
        }
    }
    const bool bank = boatX_ < BANK + BOAT_R || boatX_ > float(gs::SCREEN_W) - BANK - BOAT_R;
    if (bank && scrape_ == 0) {
        chip(4, boatX_, BOAT_Y, -1);
        scrape_ = 16;
    }
    if (scrape_ > 0) scrape_--;
}

void Game::tickFx() {
    for (auto& m : motes_) {
        m.life -= 1.f;
        m.x += m.vx;
        m.y += m.vy;
        m.vy += 0.03f;
    }
    motes_.erase(std::remove_if(motes_.begin(), motes_.end(), [](const Mote& m) { return m.life <= 0; }), motes_.end());
    for (auto& p : pops_) {
        p.life -= 1.f;
        p.y -= 0.45f;
    }
    pops_.erase(std::remove_if(pops_.begin(), pops_.end(), [](const Pop& p) { return p.life <= 0; }), pops_.end());
    if (shake_ > 0) shake_--;
    if (thud_ > 0) thud_--;
}

void Game::finish(bool win) {
    if (mode_ == Mode::End) return;
    mode_ = Mode::End;
    won_ = win && hull_ > 0;
    over_ = true;
    lived_ = playFrame_;
    if (won_ && hull_ > best_) best_ = hull_;
    fan_ = won_ ? 1 : -1;
    fanFrame_ = 0;
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(1, 0, 0);
    sys_->apu.noise(0, 0, false);
    if (won_) {
        sys_->setLight(30, 220, 80);
        sys_->rumble(0.15f, 0.35f, 180);
    } else {
        sys_->setLight(220, 24, 16);
        sys_->rumble(0.9f, 0.2f, 240);
        sys_->apu.tone(2, 70, 0.06f);
        thud_ = 18;
    }
}

void Game::audio() {
    if (mode_ == Mode::Title) {
        sys_->apu.noise(0.012f, 420.f, false);
        sys_->apu.tone(0, 0, 0);
        sys_->apu.tone(1, 0, 0);
        return;
    }
    if (mode_ == Mode::Pause) return;
    if (mode_ == Mode::End) {
        if (fan_ == 1) {
            static const float notes[] = {392.f, 523.f, 659.f, 784.f};
            int step = fanFrame_ / 8;
            if (fanFrame_ % 8 == 0 && step < 4) sys_->apu.tone(2, notes[step], 0.055f);
            if (fanFrame_ == 36) sys_->apu.tone(2, 0, 0);
            fanFrame_++;
        } else if (fan_ < 0) {
            if (thud_ == 0) sys_->apu.tone(2, 0, 0);
        }
        return;
    }
    float wob = 1.f + 0.012f * std::sin(playFrame_ * 0.045f);
    sys_->apu.tone(0, 49.f * wob, 0.018f);
    sys_->apu.tone(1, 73.5f, 0.009f);
    sys_->apu.noise(0.014f, 520.f + 30.f * std::sin(scroll_ * 0.015f), false);
    int left = MINUTE - playFrame_;
    if (thud_ == 0 && left > 0 && left <= 600 && left % 60 == 0) {
        sys_->apu.tone(2, left <= 180 ? 880.f : 620.f, 0.045f);
        thud_ = 7;
    }
    float u = std::clamp(hull_ / 100.f, 0.f, 1.f);
    sys_->setLight(int((1.f - u) * 255), int(u * 210), 36);
}

void Game::update() {
    const gs::Pad& pad = sys_->pad;
    if (mode_ == Mode::Title) {
        for (auto& d : decor_) {
            d.y += d.vy;
            if (d.y > gs::SCREEN_H + 28) d.y = -36.f;
        }
        scroll_ += 1.1f;
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) castOff();
        audio();
        return;
    }
    if (pad.pressed(gs::BTN_MODE) && !bot_) {
        quiet();
        mode_ = Mode::Title;
        layoutDecor();
        return;
    }
    if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Run;
        return;
    }
    if (mode_ == Mode::End) {
        for (auto& rk : rocks_) rk.y += rk.vy * 0.35f;
        rocks_.erase(std::remove_if(rocks_.begin(), rocks_.end(),
                                     [](const Rock& r) { return r.y - r.r > gs::SCREEN_H + 10.f; }),
                     rocks_.end());
        tickFx();
        scroll_ += 0.6f;
        audio();
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C))) castOff();
        return;
    }

    if (!bot_ && pad.pressed(gs::BTN_START)) {
        mode_ = Mode::Pause;
        quiet();
        return;
    }

    steer();
    if (playFrame_ % 2 == 0) spawnRock();
    for (auto& rk : rocks_) rk.y += rk.vy;
    collide();
    rocks_.erase(std::remove_if(rocks_.begin(), rocks_.end(),
                                 [](const Rock& r) { return !r.live || r.y - r.r > gs::SCREEN_H + 8.f; }),
                 rocks_.end());
    tickFx();
    scroll_ += 1.25f + paceAt(playFrame_) * 0.35f;
    playFrame_++;
    if (hull_ <= 0) finish(false);
    else if (playFrame_ >= MINUTE) finish(true);
    audio();
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool shadow) {
    if (h < 1.f || m.h <= 0 || m.w <= 0) return;
    float w = h * (float(m.w) / float(m.h));
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal & 15);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal, int align) {
    if (s.empty() || art_.glyph[0].h <= 0) return;
    const float adv = float(art_.glyph[0].w) * scale;
    const float gh = float(art_.glyph[0].h) * scale;
    const float width = adv * float(s.size());
    float x0 = x;
    if (align == 1) x0 = x - width * 0.5f;
    else if (align == 2) x0 = x - width;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < 32 || c > 126) c = '?';
        if (c != ' ') spr(art_.glyph[c - 32], x0 + adv * 0.5f, y, gh, pal, false);
        x0 += adv;
    }
}

void Game::scenery() {
    gs::VDP& v = sys_->vdp;
    const int lift = shake_ > 3 ? 1 : 0;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / 223.f;
        int r = 1 + lift;
        int g = 5 + int(u * 3.f) + lift;
        int b = 10 + int(u * 3.f);
        v.lineBackdrop[y] = gs::rgb4(std::min(15, r), std::min(15, g), std::min(15, b));
        int fog = 0;
        if (y < 90) fog = (90 - y) / 18;
        v.lineFog[y] = uint8_t(std::clamp(fog, 0, 5));
        float depth = 0.55f + 0.7f * u;
        int vs = int(std::floor(scroll_ * depth));
        float sway = std::sin(y * 0.045f + scroll_ * 0.012f) * 5.5f;
        v.B.vscroll[y] = int16_t(vs);
        v.A.vscroll[y] = int16_t(int(vs * 0.82f));
        v.B.hscroll[y] = int16_t(std::lround(sway));
        v.A.hscroll[y] = 0;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    scenery();
    const float bob = std::sin(float(sys_->frame) * 0.16f) * 1.1f;
    const int sx = shake_ ? ((shake_ & 1) ? 1 : -1) : 0;
    const int sy = shake_ > 2 ? ((shake_ & 2) ? 1 : -1) : 0;

    auto worldRock = [&](int kind, int var, int pal, float x, float y, float r, bool flip) {
        int fog = 0;
        if (y < 36) fog = 8;
        else if (y < 80) fog = 4;
        else if (y < 120) fog = 2;
        float depth = 0.90f + 0.10f * std::clamp(y / BOAT_Y, 0.f, 1.f);
        float h = (r / ROCK_FILL) * depth;
        spr(art_.rock[kind][var], x + sx, y + sy, h, ROCK_PALS[pal], flip, fog);
    };

    // Earlier sprites are drawn on top. HUD and banners first, then the hull, then rocks.
    if (mode_ == Mode::Title) {
        text("S3 ROCK", 160, 30, 2.f, PAL_HUD, 1);
        text("A MINUTE OF ROCKS", 160, 62, 1.f, PAL_GOLD, 1);
        text("THE HULL IS THE SCORE", 160, 80, 1.f, PAL_HUD, 1);
        text("ONE RAPID EVERY TIME", 160, 98, 1.f, PAL_HUD, 1);
        text("ARROWS STEER", 160, 178, 1.f, PAL_HUD, 1);
        if ((sys_->frame / 36) % 2 == 0) text("ENTER CASTS OFF", 160, 198, 1.f, PAL_GOLD, 1);
        if (best_ > 0) {
            char best[24];
            std::snprintf(best, sizeof best, "BEST HULL %d", best_);
            text(best, 160, 216, 1.f, PAL_HUD, 1);
        }
        for (const auto& d : decor_) worldRock(d.kind, d.var, d.pal, d.x, d.y, radiusOf(d.kind), d.flip);
        spr(art_.shade, 160, 148 + bob, 12, 0, false, 0, true);
        spr(art_.boat[1], 160, 136 + bob, 34, PAL_BOAT, false);
        return;
    }

    if (mode_ == Mode::Pause) text("PAUSED", 160, 108, 2.f, PAL_GOLD, 1);

    if (mode_ == Mode::End) {
        if (won_) {
            text("MINUTE HELD", 160, 78, 1.f, PAL_GOLD, 1);
            char big[8];
            std::snprintf(big, sizeof big, "%d", hull_);
            int pal = hull_ == 100 ? PAL_HUD : hull_ >= 60 ? PAL_GOLD : PAL_BAD;
            text(big, 160, 112, 3.f, pal, 1);
            text(hull_ == 100 ? "NOT A SCRATCH" : "THE HULL IS THE SCORE", 160, 148, 1.f, PAL_HUD, 1);
        } else {
            text("HULL OPEN", 160, 78, 2.f, PAL_BAD, 1);
            int sec = std::max(0, lived_) / 60;
            char lasted[24];
            std::snprintf(lasted, sizeof lasted, "LASTED %d:%02d", sec / 60, sec % 60);
            text(lasted, 160, 114, 1.f, PAL_HUD, 1);
            text("THE MINUTE WAS NOT YOURS", 160, 136, 1.f, PAL_HUD, 1);
        }
        if ((sys_->frame / 36) % 2 == 0) text("ENTER RUNS IT AGAIN", 160, 176, 1.f, PAL_GOLD, 1);
    }

    {
        int pal = hull_ >= 60 ? PAL_HUD : hull_ >= 30 ? PAL_GOLD : PAL_BAD;
        char buf[24];
        std::snprintf(buf, sizeof buf, "HULL %d", hull_);
        text(buf, 8, 12, 1.f, pal, 0);
        int left = mode_ == Mode::Run ? std::max(0, MINUTE - playFrame_) : 0;
        int show = mode_ == Mode::End && won_ ? 0 : (left + 59) / 60;
        if (mode_ == Mode::End && !won_) {
            int sec = std::max(0, lived_) / 60;
            std::snprintf(buf, sizeof buf, "%d:%02d", sec / 60, sec % 60);
        } else {
            std::snprintf(buf, sizeof buf, "%d:%02d", show / 60, show % 60);
        }
        text(buf, float(gs::SCREEN_W) - 8.f, 12, 1.f, left <= 600 && mode_ == Mode::Run ? PAL_GOLD : PAL_HUD, 2);
        int full = hull_ / 10;
        int rem = hull_ % 10;
        for (int i = 0; i < 10; i++) {
            float rh = 0;
            if (i < full) rh = 12.f;
            else if (i == full && rem) rh = 12.f * (rem / 10.f);
            if (rh < 2.f) continue;
            spr(art_.rib, 14.f + i * 10.f, 28.f, rh, PAL_BOAT, false);
        }
    }

    for (const auto& p : pops_) {
        char buf[8];
        std::snprintf(buf, sizeof buf, "-%d", p.dmg);
        text(buf, p.x + sx, p.y + sy, 1.f, PAL_BAD, 1);
    }
    for (const auto& m : motes_) {
        int frame = m.life < m.max * 0.45f ? 1 : 0;
        float h = 8.f + 10.f * (m.life / m.max);
        spr(art_.splash[frame], m.x + sx, m.y + sy, h, PAL_FX, false, 0);
    }

    const bool blink = inv_ > 0 && ((inv_ / 3) & 1) == 0;
    if (!blink) {
        int lean = 1;
        if (vel_ < -0.28f) lean = 0;
        else if (vel_ > 0.28f) lean = 2;
        spr(art_.shade, boatX_ + sx, BOAT_Y + 12.f + bob + sy, 11, 0, false, 0, true);
        if (hull_ <= 60) spr(art_.crack, boatX_ + sx - 2, BOAT_Y + bob + sy - 2, 22, PAL_BOAT, false);
        if (hull_ <= 25) spr(art_.crack, boatX_ + sx + 3, BOAT_Y + bob + sy + 4, 18, PAL_BOAT, true);
        spr(art_.boat[lean], boatX_ + sx, BOAT_Y + bob + sy, 32, PAL_BOAT, false);
        spr(art_.foam, boatX_ + sx, BOAT_Y + 18.f + bob + sy, 12, PAL_FX, false, 4);
        spr(art_.foam, boatX_ + sx, BOAT_Y + 28.f + bob + sy, 8, PAL_FX, false, 8);
    }

    // Rocks under the hull. Nearer ones first so they cover the far shore.
    std::vector<int> order(rocks_.size());
    for (int i = 0; i < int(rocks_.size()); i++) order[i] = i;
    std::sort(order.begin(), order.end(), [&](int a, int b) { return rocks_[a].y > rocks_[b].y; });
    for (int i : order) {
        const Rock& rk = rocks_[i];
        worldRock(rk.kind, rk.var, rk.pal, rk.x, rk.y, rk.r, rk.flip);
    }

    float pace = paceAt(mode_ == Mode::Run ? playFrame_ : lived_);
    for (int i = 0; i < 5; i++) {
        float y = std::fmod(scroll_ * 1.15f + i * 52.f, 260.f) - 24.f;
        int ahead = int(std::max(0.f, (BOAT_Y - y) / pace));
        int f = (mode_ == Mode::Run ? playFrame_ : std::max(0, lived_ - 1)) + ahead;
        spr(art_.foam, gapCenter(f) + sx, y + sy, 14.f, PAL_FX, false, 9);
    }
}

}  // namespace rock

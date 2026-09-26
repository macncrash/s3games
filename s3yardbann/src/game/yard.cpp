#include "game/yard.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace yard {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float kWorld = 2280.f;
constexpr float kFloor = 188.f;
constexpr float kWin = 210.f;
constexpr float kSpawn = 236.f;
constexpr float kBanner0 = 2010.f;
constexpr float kMinX = 78.f;
constexpr float kMaxX = 2160.f;
constexpr float kRun = 168.f;
constexpr float kCarry = 148.f;
constexpr float kAccel = 2600.f;
constexpr float kBody = 16.f;
constexpr float kStrike = 58.f;
constexpr float kGrab = 34.f;
constexpr float kShove = 124.f;
constexpr float kChase = 96.f;

// Bot boxes are wider than the hit, so a crossing that looks safe stays safe.
constexpr float kMagX[2] = {900.f, 1540.f};
constexpr float kMagPeriod[2] = {4.4f, 4.0f};
constexpr float kMagPhase[2] = {0.6f, 2.4f};
constexpr float kMagHalf = 40.f;
constexpr float kMagHit = 26.f;
constexpr float kCrX = 1210.f;
constexpr float kCrPeriod = 4.8f;
constexpr float kCrPhase = 1.3f;
constexpr float kCrHalf = 38.f;

float approach(float v, float target, float delta) {
    if (v < target) return std::min(target, v + delta);
    return std::max(target, v - delta);
}

float wrapPhase(float t, float period, float phase0) {
    float u = std::fmod(t + phase0, period);
    if (u < 0.f) u += period;
    return u / period;
}

float magDrop(float p) {
    if (p < 0.48f) return 0.f;
    if (p < 0.62f) return (p - 0.48f) / 0.14f;
    if (p < 0.76f) return 1.f;
    if (p < 0.92f) return 1.f - (p - 0.76f) / 0.16f;
    return 0.f;
}

float jawOpen(float p) {
    if (p < 0.50f) return 1.f;
    if (p < 0.64f) return 1.f - (p - 0.50f) / 0.14f;
    if (p < 0.78f) return 0.f;
    if (p < 0.94f) return (p - 0.78f) / 0.16f;
    return 1.f;
}

bool magDanger(float p) { return magDrop(p) > 0.58f; }
bool magHit(float p) { return magDrop(p) > 0.80f; }
bool jawDanger(float p) { return jawOpen(p) < 0.58f; }
bool jawWall(float p) { return jawOpen(p) < 0.38f; }
bool jawCrush(float p) { return jawOpen(p) < 0.16f; }

bool spanCold(float t, float dur, float period, float phase0, bool (*hot)(float)) {
    for (int i = 0; i <= 16; i++) {
        float p = wrapPhase(t + dur * float(i) / 16.f, period, phase0);
        if (hot(p)) return false;
    }
    return true;
}

}  // namespace

const gs::Mipped& Game::hero() const {
    if (swing_ > 0.f) return art_.pry;
    if (std::fabs(vx_) > 22.f) return (int(step_ / 6.f) & 1) ? art_.runA : art_.runB;
    return art_.stand;
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_) return 4;
    if (!has_) return 1;
    if (px_ > 1200.f) return 2;
    return 3;
}

void Game::layout() {
    dogs_[0].minX = 460.f;
    dogs_[0].maxX = 720.f;
    dogs_[0].x = 560.f;
    dogs_[0].speed = 42.f;
    dogs_[0].dir = 1.f;
    dogs_[0].stun = 0.f;
    dogs_[1].minX = 1700.f;
    dogs_[1].maxX = 1880.f;
    dogs_[1].x = 1780.f;
    dogs_[1].speed = 46.f;
    dogs_[1].dir = -1.f;
    dogs_[1].stun = 0.f;
}

void Game::resetRun() {
    layout();
    px_ = kSpawn;
    vx_ = 0.f;
    face_ = 1;
    has_ = false;
    bannerX_ = kBanner0;
    lives_ = 4;
    step_ = 0.f;
    strikeCd_ = swing_ = inv_ = stun_ = dropLock_ = 0.f;
    shake_ = 0.f;
    playT_ = 0.f;
    magLow_[0] = magLow_[1] = false;
    jawShut_ = false;
    fan_ = -1;
    waitT_ = 0.f;
    waitX_ = px_;
}

void Game::begin() {
    resetRun();
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
    cam_ = std::clamp(px_ - 150.f, 0.f, kWorld - float(gs::SCREEN_W));
    blip(480.f, 0.04f, 0.05f);
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
    vx_ = 0.f;
    fan_ = 0;
    fanT_ = 0.f;
    shake_ = 0.35f;
    sys_->rumble(0.25f, 0.7f, 220);
    sys_->setLight(40, 170, 70);
}

void Game::lose() {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Over;
    won_ = false;
    over_ = true;
    vx_ = 0.f;
    fan_ = 0;
    fanT_ = 0.f;
    sys_->rumble(0.8f, 0.2f, 180);
    sys_->setLight(180, 30, 24);
    sys_->apu.noiseBurst(0.4f, 140.f, 0.3f);
}

void Game::clearDrop() {
    auto shove = [&](float c, float half) {
        if (bannerX_ > c - half - 8.f && bannerX_ < c + half + 8.f)
            bannerX_ = px_ < c ? c - half - 46.f : c + half + 46.f;
    };
    shove(kMagX[0], kMagHalf);
    shove(kMagX[1], kMagHalf);
    shove(kCrX, kCrHalf);
    bannerX_ = std::clamp(bannerX_, 360.f, 1960.f);
}

void Game::hurt(float fromX) {
    if (inv_ > 0.f || mode_ != Mode::Play) return;
    lives_--;
    inv_ = 1.35f;
    stun_ = 0.2f;
    float away = px_ < fromX ? -1.f : 1.f;
    if (std::fabs(px_ - fromX) < 0.5f) away = -float(face_);
    vx_ = away * 150.f;
    px_ = std::clamp(px_ + away * 10.f, kMinX, kMaxX);
    shake_ = 1.f;
    sys_->apu.noiseBurst(0.45f, 520.f, 0.12f);
    sys_->rumble(0.7f, 0.3f, 110);
    if (has_) {
        has_ = false;
        bannerX_ = px_;
        clearDrop();
        dropLock_ = 0.65f;
    }
    if (lives_ <= 0) lose();
}

void Game::bot(bool& left, bool& right, bool& strike) {
    left = right = false;
    strike = false;
    const float goal = has_ ? (kWin - 16.f) : bannerX_;
    int travel = 0;
    if (goal > px_ + 3.f) travel = 1;
    else if (goal < px_ - 3.f) travel = -1;

    for (const Dog& d : dogs_) {
        if (d.stun > 0.f) continue;
        float rel = d.x - px_;
        float ad = std::fabs(rel);
        bool ahead = false;
        if (travel > 0) ahead = rel > -10.f && rel < kStrike - 6.f;
        else if (travel < 0) ahead = rel < 10.f && rel > -(kStrike - 6.f);
        else ahead = ad < kStrike - 6.f;
        if (ad < 32.f) ahead = true;
        if (!ahead) continue;
        if (strikeCd_ <= 0.f) {
            face_ = rel >= 0.f ? 1 : -1;
            strike = true;
            return;
        }
        if (ad < 26.f) {
            if (rel >= 0.f) left = true;
            else right = true;
            return;
        }
    }

    struct Zone {
        float x, half, period, phase0;
        bool (*hot)(float);
    };
    const Zone zones[3] = {
        {kMagX[0], kMagHalf, kMagPeriod[0], kMagPhase[0], magDanger},
        {kCrX, kCrHalf, kCrPeriod, kCrPhase, jawDanger},
        {kMagX[1], kMagHalf, kMagPeriod[1], kMagPhase[1], magDanger},
    };
    bool blocked = false;
    if (travel != 0) {
        float spd = (has_ ? kCarry : kRun) * 0.80f;
        for (const Zone& z : zones) {
            float lo = z.x - z.half;
            float hi = z.x + z.half;
            float distEnter = travel > 0 ? lo - px_ : px_ - hi;
            float distExit = travel > 0 ? hi - px_ : px_ - lo;
            if (distExit <= 0.f || distEnter <= 0.f || distEnter > 104.f) continue;
            float tNeed = distExit / spd + 0.32f;
            if (!spanCold(playT_, tNeed, z.period, z.phase0, z.hot)) blocked = true;
        }
    }
    if (blocked) {
        if (std::fabs(px_ - waitX_) > 12.f) {
            waitX_ = px_;
            waitT_ = 0.f;
        } else {
            waitT_ += DT;
        }
        // A missed window must not pin the run. One dash, then the lives cover a mistake.
        if (waitT_ < 5.5f) return;
    } else {
        waitT_ = 0.f;
    }
    if (travel > 0) {
        right = true;
        face_ = 1;
    } else if (travel < 0) {
        left = true;
        face_ = -1;
    }
}

void Game::step(bool left, bool right, bool strike) {
    if (stun_ > 0.f) stun_ -= DT;
    if (inv_ > 0.f) inv_ -= DT;
    if (dropLock_ > 0.f) dropLock_ -= DT;
    if (strikeCd_ > 0.f) strikeCd_ -= DT;
    if (swing_ > 0.f) swing_ -= DT;

    for (Dog& d : dogs_) {
        if (d.stun > 0.f) {
            d.stun -= DT;
            continue;
        }
        if (has_) {
            d.dir = px_ >= d.x ? 1.f : -1.f;
            d.x += d.dir * kChase * DT;
            d.x = std::clamp(d.x, 200.f, 2100.f);
        } else {
            if (d.x > d.maxX) d.dir = -1.f;
            else if (d.x < d.minX) d.dir = 1.f;
            d.x += d.dir * d.speed * DT;
            if (d.x >= d.maxX && d.dir > 0.f) {
                d.x = d.maxX;
                d.dir = -1.f;
            } else if (d.x <= d.minX && d.dir < 0.f) {
                d.x = d.minX;
                d.dir = 1.f;
            }
        }
    }

    float prev = px_;
    if (stun_ > 0.f) vx_ = approach(vx_, 0.f, 700.f * DT);
    else {
        float target = 0.f;
        if (right && !left) target = has_ ? kCarry : kRun;
        if (left && !right) target = has_ ? -kCarry : -kRun;
        if ((right && !left) || (left && !right)) face_ = right ? 1 : -1;
        vx_ = approach(vx_, target, kAccel * DT);
    }
    px_ += vx_ * DT;

    float cp = wrapPhase(playT_, kCrPeriod, kCrPhase);
    float open = jawOpen(cp);
    if (jawWall(cp)) {
        float a = prev - kCrX;
        float b = px_ - kCrX;
        if (a * b <= 0.f || std::fabs(b) < 10.f) {
            float side = std::fabs(a) >= 1.f ? (a > 0.f ? 1.f : -1.f) : (b >= 0.f ? 1.f : -1.f);
            bool deep = std::fabs(a) < 16.f && jawCrush(cp);
            px_ = kCrX + side * 28.f;
            vx_ = 0.f;
            if (deep) hurt(kCrX);
            if (mode_ != Mode::Play) return;
        }
    }
    px_ = std::clamp(px_, kMinX, kMaxX);

    if (std::fabs(vx_) > 24.f) {
        step_ += std::fabs(vx_) * DT * 0.18f;
        if (step_ > 1000.f) step_ -= 1000.f;
    }

    if (has_ && px_ <= kWin) {
        win();
        return;
    }

    if (strike && strikeCd_ <= 0.f && stun_ <= 0.f && mode_ == Mode::Play) {
        strikeCd_ = 0.34f;
        swing_ = 0.16f;
        bool hit = false;
        for (Dog& d : dogs_) {
            if (d.stun > 0.f) continue;
            float rel = d.x - px_;
            bool in = std::fabs(rel) < kStrike;
            if (!has_) {
                if (face_ > 0) in = rel > -8.f && rel < kStrike;
                else in = rel < 8.f && rel > -kStrike;
            }
            if (!in) continue;
            d.stun = 2.5f;
            float away = std::fabs(rel) < 1.f ? float(face_) : (rel >= 0.f ? 1.f : -1.f);
            d.x += away * kShove;
            d.dir = away;
            hit = true;
        }
        if (hit) {
            blip(160.f, 0.05f, 0.04f);
            sys_->apu.noiseBurst(0.22f, 700.f, 0.06f);
            sys_->rumble(0.2f, 0.4f, 40);
        } else {
            blip(280.f, 0.025f, 0.03f);
        }
    }

    if (inv_ <= 0.f && mode_ == Mode::Play) {
        for (int i = 0; i < 2; i++) {
            float p = wrapPhase(playT_, kMagPeriod[i], kMagPhase[i]);
            if (magHit(p) && std::fabs(px_ - kMagX[i]) < kMagHit) {
                hurt(kMagX[i]);
                break;
            }
        }
    }
    if (mode_ == Mode::Play && inv_ <= 0.f && jawCrush(cp) && std::fabs(px_ - kCrX) < 14.f) hurt(kCrX);
    if (mode_ == Mode::Play && inv_ <= 0.f && stun_ <= 0.f) {
        for (const Dog& d : dogs_) {
            if (d.stun > 0.f) continue;
            if (std::fabs(d.x - px_) < kBody) {
                hurt(d.x);
                break;
            }
        }
    }
    if (mode_ != Mode::Play) return;

    if (!has_ && dropLock_ <= 0.f && stun_ <= 0.f && std::fabs(px_ - bannerX_) < kGrab) {
        has_ = true;
        blip(680.f, 0.07f, 0.08f);
        sys_->apu.noiseBurst(0.08f, 1600.f, 0.04f);
        sys_->rumble(0.3f, 0.55f, 90);
    }

    for (int i = 0; i < 2; i++) {
        float d = magDrop(wrapPhase(playT_, kMagPeriod[i], kMagPhase[i]));
        bool low = d > 0.9f;
        if (low && !magLow_[i]) sys_->apu.noiseBurst(0.1f, 180.f, 0.07f);
        magLow_[i] = low;
    }
    bool shut = open < 0.08f;
    if (shut && !jawShut_) sys_->apu.noiseBurst(0.18f, 70.f, 0.1f);
    jawShut_ = shut;
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

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet, bool shadow) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::clamp(long(std::lround(cx - s.w * 0.5f)), -2000L, 2000L));
    s.y = int16_t(std::clamp(long(std::lround(feet ? cy - s.h : cy - s.h * 0.5f)), -2000L, 2000L));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal, int align) {
    float width = 0.f;
    for (unsigned char c : s) {
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (c == ' ') width += 8.f * scale;
        else if (c > 32 && c < 128) width += art_.glyph[c - 32].w * scale + scale;
    }
    if (align == 0) x -= width * 0.5f;
    else if (align > 0) x -= width;
    for (unsigned char c : s) {
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (c == ' ') {
            x += 8.f * scale;
            continue;
        }
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + g.w * scale * 0.5f, y, g.h * scale, pal, false, false, false);
        x += g.w * scale + scale;
    }
}

void Game::drawWorld(float view) {
    auto world = [&](const gs::Mipped& m, float wx, float foot, float h, int pal, bool flip, bool feet = true) {
        spr(m, wx - view, foot, h, pal, flip, feet, false);
    };
    int flutter = int(t_ * 8.f) & 1;
    bool homed = !has_ && std::fabs(bannerX_ - kBanner0) < 12.f;
    bool show = inv_ <= 0.f || (int(inv_ * 14.f) & 1) == 0;

    text("OFFICE", 124.f - view, kFloor - 86.f, 0.85f, PAL_HUD, 0);
    text("HOIST", kBanner0 - view, kFloor - 108.f, 0.85f, PAL_HUD, 0);

    if (has_) {
        float lift = mode_ == Mode::Victory ? 18.f : 0.f;
        world(art_.banner[flutter], px_ + face_ * 16.f, kFloor - 22.f - lift, 58.f, PAL_BANNER, face_ < 0);
    }
    for (int i = 0; i < 2; i++) {
        float p = wrapPhase(playT_, kMagPeriod[i], kMagPhase[i]);
        float drop = magDrop(p);
        float bottom = kFloor - (76.f - drop * 54.f);
        float top = bottom - 32.f;
        float beamBottom = kFloor - 108.f;
        if (top - beamBottom > 2.f) world(art_.cable, kMagX[i], top, top - beamBottom, PAL_STEEL, false, true);
        world(art_.magnet, kMagX[i], bottom, 32.f, PAL_STEEL, false, true);
        if (drop > 0.55f) world(art_.spark, kMagX[i], bottom + 4.f, 12.f, PAL_FX, flutter, true);
        spr(art_.sun, kMagX[i] - view, kFloor - 116.f, 8.f, magDanger(p) ? PAL_ALERT : PAL_FX, false, false, false);
    }
    if (show) {
        spr(hero(), px_ - view, kFloor, 52.f, PAL_HAND, face_ < 0, true, false);
        spr(art_.shadow, px_ - view, kFloor + 2.f, 8.f, PAL_FX, false, true, true);
    }
    for (const Dog& d : dogs_) {
        bool down = d.stun > 0.4f;
        world(art_.dog[down ? 2 : (int(t_ * 8.f + d.x) & 1)], d.x, kFloor, down ? 22.f : 30.f, PAL_DOG, d.dir < 0);
        spr(art_.shadow, d.x - view, kFloor + 2.f, 7.f, PAL_FX, false, true, true);
    }
    if (!has_) {
        if (homed) {
            world(art_.cable, kBanner0, kFloor - 70.f, 28.f, PAL_STEEL, false, true);
            world(art_.banner[flutter], kBanner0 + 8.f, kFloor - 12.f, 64.f, PAL_BANNER, false);
        } else {
            world(art_.banner[flutter], bannerX_, kFloor + 2.f, 40.f, PAL_BANNER, false);
        }
    }

    float cp = wrapPhase(playT_, kCrPeriod, kCrPhase);
    float gap = 10.f + jawOpen(cp) * 52.f;
    float jh = 46.f;
    float jw = jh * float(art_.jaw.w) / float(std::max(1, int(art_.jaw.h)));
    world(art_.jaw, kCrX - gap - jw * 0.5f, kFloor, jh, PAL_STEEL, false);
    world(art_.jaw, kCrX + gap + jw * 0.5f, kFloor, jh, PAL_STEEL, true);

    world(art_.hoist, kBanner0, kFloor, 100.f, PAL_STEEL, false);
    for (int i = 0; i < 2; i++) {
        world(art_.beam, kMagX[i], kFloor - 108.f, 12.f, PAL_STEEL, false);
        world(art_.post, kMagX[i] - 40.f, kFloor, 124.f, PAL_STEEL, false);
        world(art_.post, kMagX[i] + 40.f, kFloor, 124.f, PAL_STEEL, true);
        world(art_.plate, kMagX[i], kFloor + 2.f, 10.f, PAL_STEEL, false);
    }
    if (has_) {
        const float marks[] = {480.f, 780.f, 1100.f, 1420.f, 1720.f};
        for (int i = 0; i < 5; i++) {
            float bob = (int(t_ * 3.f + i) & 1) ? 0.f : -2.f;
            world(art_.chevron, marks[i], kFloor - 8.f + bob, 14.f, PAL_BANNER, false);
        }
    }
    const float piles[] = {390.f, 640.f, 1040.f, 1360.f, 1760.f, 2140.f};
    for (int i = 0; i < 6; i++) world(art_.car[i & 1], piles[i], kFloor, i & 1 ? 46.f : 54.f, PAL_SCRAP, i == 3);
    world(art_.shack, 124.f, kFloor, 78.f, PAL_SCRAP, false);
    world(art_.mat, 140.f, kFloor + 3.f, has_ ? 12.f : 10.f, PAL_BANNER, false);
    world(art_.drum, 520.f, kFloor, 28.f, PAL_STEEL, false);
    world(art_.drum, 1320.f, kFloor, 28.f, PAL_STEEL, false);
    world(art_.tire, 740.f, kFloor, 18.f, PAL_SCRAP, false);
    world(art_.tire, 1660.f, kFloor, 18.f, PAL_SCRAP, false);
    world(art_.lamp, 300.f, kFloor, 58.f, PAL_SCRAP, false);
    world(art_.lamp, 1680.f, kFloor, 58.f, PAL_SCRAP, false);
    spr(art_.sun, 250.f - view * 0.06f, 30.f, 18.f, PAL_FX, false, false, false);
}

void Game::drawPoster() {
    int flutter = int(t_ * 7.f) & 1;
    text("S3 YARD BANN", 160.f, 16.f, 1.15f, PAL_HUD, 0);
    text("ONE YARD", 160.f, 40.f, 1.f, PAL_FX, 0);
    text("BRING THE BANNER BACK", 160.f, 60.f, 0.9f, PAL_HUD, 0);
    if (int(t_ * 2.f) & 1) text("START", 160.f, 86.f, 1.f, PAL_HUD, 0);
    spr(art_.shack, 48.f, kFloor, 70.f, PAL_SCRAP, false, true, false);
    spr(art_.stand, 108.f, kFloor, 50.f, PAL_HAND, false, true, false);
    spr(art_.shadow, 108.f, kFloor + 2.f, 8.f, PAL_FX, false, true, true);
    spr(art_.dog[int(t_ * 6.f) & 1], 162.f, kFloor, 28.f, PAL_DOG, false, true, false);
    spr(art_.shadow, 162.f, kFloor + 2.f, 7.f, PAL_FX, false, true, true);
    spr(art_.beam, 214.f, 118.f, 10.f, PAL_STEEL, false, true, false);
    spr(art_.cable, 214.f, 150.f, 22.f, PAL_STEEL, false, true, false);
    spr(art_.magnet, 214.f, 168.f, 30.f, PAL_STEEL, false, true, false);
    spr(art_.hoist, 278.f, kFloor, 74.f, PAL_STEEL, false, true, false);
    spr(art_.banner[flutter], 286.f, kFloor - 8.f, 52.f, PAL_BANNER, false, true, false);
    spr(art_.sun, 286.f, 28.f, 14.f, PAL_FX, false, false, false);
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    float view = 0.f;
    if (mode_ != Mode::Title) {
        view = cam_;
        if (shake_ > 0.f) view += std::sin(t_ * 90.f) * shake_ * 3.f;
        view = std::clamp(view, 0.f, kWorld - float(gs::SCREEN_W));
    }
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t c;
        if (y < 70) {
            float u = y / 70.f;
            c = gs::rgb4(3 + int(2 * u), 3 + int(u), 6 - int(u));
        } else if (y < 130) {
            float u = (y - 70) / 60.f;
            c = gs::rgb4(5 + int(6 * u), 4 + int(2 * u), 5 - int(2 * u));
        } else if (y < 188) {
            float u = (y - 130) / 58.f;
            c = gs::rgb4(10 - int(4 * u), 5 - int(2 * u), 3);
        } else {
            c = gs::rgb4(3, 2, 1);
        }
        vdp.lineBackdrop[y] = c;
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
        vdp.A.hscroll[y] = int16_t(std::lround(-view * 0.32f));
        vdp.B.hscroll[y] = int16_t(std::lround(-view));
        vdp.A.vscroll[y] = 0;
        vdp.B.vscroll[y] = 0;
    }

    if (mode_ == Mode::Title) {
        drawPoster();
        hudC(25, "TIME THE HOIST. PRY THE DOGS.", PAL_HUD);
        hudC(26, "ARROWS MOVE   Z PRY   START", PAL_HUD);
        return;
    }

    if (mode_ == Mode::Victory) {
        text("THE BANNER IS BACK", 160.f, 28.f, 1.05f, PAL_HUD, 0);
        text("THE YARD IS DONE", 160.f, 50.f, 1.f, PAL_FX, 0);
    } else if (mode_ == Mode::Over) {
        text("THE YARD KEEPS IT", 160.f, 36.f, 1.05f, PAL_ALERT, 0);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160.f, 40.f, 1.3f, PAL_HUD, 0);
    }
    drawWorld(view);

    if (mode_ == Mode::Play || mode_ == Mode::Pause) {
        char lives[16];
        std::snprintf(lives, sizeof lives, "LIVES %d", lives_);
        hud(1, 0, lives, lives_ > 1 ? PAL_HUD : PAL_ALERT);
        hud(31, 0, has_ ? "BANNER" : "YARD", has_ ? PAL_ALERT : PAL_HUD);
        if (!has_) hudC(1, "TAKE THE BANNER", PAL_HUD);
        else hudC(1, px_ < 420.f ? "THE OFFICE" : "BRING IT BACK", PAL_HUD);
        hud(1, 26, "ARROWS MOVE   Z PRY", PAL_HUD);
    } else if (!bot_) {
        hudC(26, "START", PAL_HUD);
    }
}

void Game::serviceAudio() {
    if (beep_ > 0.f) {
        beep_ -= DT;
        if (beep_ <= 0.f) sys_->apu.tone(0, 0.f, 0.f);
    }
    if (fan_ >= 0) {
        static const float good[] = {392.f, 494.f, 587.f, 784.f};
        static const float bad[] = {220.f, 174.f, 146.f, 110.f};
        fanT_ += DT;
        if (fanT_ > 0.18f) {
            const float* notes = won_ ? good : bad;
            if (fan_ < 4) sys_->apu.tone(2, notes[fan_], won_ ? 0.07f : 0.05f);
            else sys_->apu.tone(2, 0.f, 0.f);
            fan_++;
            fanT_ = 0.f;
            if (fan_ > 8) fan_ = -1;
        }
        sys_->apu.tone(1, 0.f, 0.f);
        return;
    }
    if (mode_ == Mode::Play) sys_->apu.tone(1, has_ ? 62.f : 46.f, 0.018f);
    else if (mode_ == Mode::Title) sys_->apu.tone(1, 52.f, 0.014f);
    else sys_->apu.tone(1, 0.f, 0.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.55f);
    resetRun();
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    t_ = 0.f;
    cam_ = 0.f;
    if (bot_) begin();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C))) begin();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) sys.quit();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Play;
            blip(440.f, 0.04f, 0.04f);
        } else if (pad.pressed(gs::BTN_MODE)) {
            resetRun();
            mode_ = Mode::Title;
            t_ = 0.f;
        }
    } else if (mode_ == Mode::Over || mode_ == Mode::Victory) {
        if (!bot_ && pad.pressed(gs::BTN_START)) begin();
    } else if (mode_ == Mode::Play) {
        playT_ += DT;
        bool left = false, right = false, strike = false;
        if (bot_) bot(left, right, strike);
        else {
            left = pad.down(gs::BTN_LEFT) || pad.axisX <= -0.35f;
            right = pad.down(gs::BTN_RIGHT) || pad.axisX >= 0.35f;
            strike = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_TURBO);
            if (pad.pressed(gs::BTN_START)) {
                mode_ = Mode::Pause;
                blip(260.f, 0.04f, 0.04f);
            }
        }
        if (mode_ == Mode::Play) step(left, right, strike);
        float lead = face_ * 24.f;
        float want = std::clamp(px_ + lead - 150.f, 0.f, kWorld - float(gs::SCREEN_W));
        cam_ += (want - cam_) * std::min(1.f, DT * 7.f);
        if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - DT * 2.2f);
    }

    if (mode_ == Mode::Play && inv_ > 0.f) sys.setLight(180, 36, 24);
    else if (mode_ == Mode::Play && has_) sys.setLight(40, 150, 64);
    else if (mode_ == Mode::Play) sys.setLight(170, 96, 30);
    else if (mode_ == Mode::Title) sys.setLight(120, 72, 24);
    else if (mode_ == Mode::Victory) sys.setLight(40, 170, 70);

    serviceAudio();
    draw();
}

}  // namespace yard

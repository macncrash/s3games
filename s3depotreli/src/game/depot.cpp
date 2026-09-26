#include "game/depot.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace depot {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float kBellAt = 35.6f;
constexpr float kRing = 2.2f;
constexpr float kLampV = 420.f;
constexpr float kLampCool = 0.24f;
constexpr float kBrakeCool = 0.16f;
constexpr float kMoveX = 118.f;
constexpr float kSlide = 320.f;

enum { Runner = 0, Dray = 1, Wagon = 2 };

struct Row {
    float arrive;
    int kind;
    int track;
};

// Arrival is the moment the nose would touch the buffer if nobody stopped it.
const Row kRows[] = {
    {4.6f, Runner, 0}, {7.2f, Runner, 2}, {10.0f, Dray, 1},  {12.8f, Runner, 0},
    {15.6f, Runner, 2}, {18.8f, Wagon, 1}, {21.8f, Runner, 0}, {24.6f, Dray, 2},
    {27.4f, Runner, 1}, {30.4f, Wagon, 0}, {33.2f, Runner, 2},
};

float speedOf(int kind) { return kind == Wagon ? 34.f : kind == Dray ? 50.f : 76.f; }
float halfOf(int kind) { return kind == Wagon ? 32.f : kind == Dray ? 18.f : 11.f; }
float tallOf(int kind) { return kind == Wagon ? 38.f : kind == Dray ? 42.f : 44.f; }
int ptsOf(int kind) { return kind == Wagon ? 250 : kind == Dray ? 200 : 100; }
int palOf(int kind) { return kind == Wagon ? PAL_WAGON : kind == Dray ? PAL_DRAY : PAL_LOOT; }

float frontOf(float x, int kind) { return x - halfOf(kind); }

uint16_t mix(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

gs::FMPatch dronePatch() {
    gs::FMPatch p;
    p.alg = 4;
    p.fb = 0.22f;
    p.op[0] = {1.f, 0.55f, 0.4f, 1.2f, 0.8f, 0.5f};
    p.op[1] = {0.5f, 0.3f, 0.35f, 0.9f, 0.6f, 0.4f};
    p.op[2] = {2.f, 0.12f, 0.2f, 0.5f, 0.3f, 0.25f};
    p.op[3] = {3.4f, 0.08f, 0.15f, 0.4f, 0.2f, 0.2f};
    p.vol = 0.07f;
    p.tone = 280.f;
    p.drive = 0.05f;
    return p;
}

gs::FMPatch bellPatch() {
    gs::FMPatch p;
    p.alg = 7;
    p.fb = 0.1f;
    p.op[0] = {1.f, 1.f, 0.004f, 0.5f, 0.15f, 0.85f};
    p.op[1] = {2.71f, 0.42f, 0.005f, 0.36f, 0.08f, 0.7f};
    p.op[2] = {5.15f, 0.18f, 0.007f, 0.28f, 0.04f, 0.55f};
    p.op[3] = {7.8f, 0.08f, 0.01f, 0.2f, 0.02f, 0.4f};
    p.vol = 0.22f;
    p.echo = 0.38f;
    return p;
}

float etaOf(float front, int kind) { return (front - kBreachX) / speedOf(kind); }

}  // namespace

int Game::marker() const {
    if (over_ || mode_ == Mode::Victory || mode_ == Mode::Over) return 3;
    if (bell_ && (mode_ == Mode::Watch || mode_ == Mode::Pause)) return 2;
    if (mode_ == Mode::Watch || mode_ == Mode::Pause) return 1;
    return 0;
}

int Game::cover() const {
    int best = 0;
    float d = 1.0e9f;
    for (int i = 0; i < 3; i++) {
        float dd = std::fabs(y_ - float(kTrackY[i]));
        if (dd < d) {
            d = dd;
            best = i;
        }
    }
    return best;
}

int Game::soonest(int track) const {
    int best = -1;
    float eta = 1.0e9f;
    for (int i = 0; i < int(foes_.size()); i++) {
        const Foe& f = foes_[size_t(i)];
        if (!f.on) continue;
        if (track >= 0 && f.track != track) continue;
        float e = etaOf(frontOf(f.x, f.kind), f.kind);
        if (e < eta) {
            eta = e;
            best = i;
        }
    }
    return best;
}

const gs::Mipped& Game::foeImg(int kind, int frame) const {
    if (kind == Wagon) return art_.wagon[frame & 1];
    if (kind == Dray) return art_.dray[frame & 1];
    return art_.runner[frame & 1];
}

void Game::beginWatch() {
    mode_ = Mode::Watch;
    over_ = false;
    won_ = false;
    bell_ = false;
    reason_ = "THE WATCH RAN OUT";
    score_ = 0;
    stopped_ = 0;
    bays_ = 3;
    stack_[0] = stack_[1] = stack_[2] = true;
    track_ = 1;
    spawnAt_ = 0;
    fanStep_ = -1;
    x_ = kStandX;
    y_ = float(kTrackY[1]);
    watch_ = 0;
    modeT_ = 0;
    lampCd_ = brakeCd_ = trackCd_ = shake_ = flash_ = beep_ = bellTick_ = fanT_ = moving_ = 0;
    foes_.clear();
    lamps_.clear();
    puffs_.clear();
    pops_.clear();
    script_.clear();
    for (const Row& r : kRows) {
        float dist = kSpawnX - (kBreachX + halfOf(r.kind));
        Spawn s;
        s.t = r.arrive - dist / speedOf(r.kind);
        s.kind = r.kind;
        s.track = r.track;
        script_.push_back(s);
    }
    std::sort(script_.begin(), script_.end(), [](const Spawn& a, const Spawn& b) { return a.t < b.t; });
    if (!sys_) return;
    sys_->apu.setPatch(0, dronePatch());
    sys_->apu.setPatch(1, bellPatch());
    sys_->apu.setPatch(2, bellPatch());
    sys_->apu.keyOn(0, 55.f, 0.05f);
    sys_->setLight(70, 48, 22);
}

void Game::kill(Foe& f) {
    if (!f.on) return;
    f.on = false;
    score_ += f.points;
    stopped_++;
    puffs_.push_back({f.x, float(kTrackY[f.track]) - 12.f, 0.35f, 22.f});
    pops_.push_back({f.x, float(kTrackY[f.track]) - tallOf(f.kind) - 4.f, 0.7f, f.points});
    if (puffs_.size() > 10) puffs_.erase(puffs_.begin());
    if (pops_.size() > 4) pops_.erase(pops_.begin());
    sys_->apu.tone(1, 680.f, 0.05f);
    sys_->apu.noiseBurst(0.14f, 1100.f, 0.05f);
    beep_ = 0.05f;
}

void Game::breach(int track) {
    if (won_ || mode_ == Mode::Over) return;
    if (track >= 0 && track < 3 && stack_[track]) stack_[track] = false;
    else {
        for (int i = 0; i < 3; i++)
            if (stack_[i]) {
                stack_[i] = false;
                break;
            }
    }
    bays_ = int(stack_[0]) + int(stack_[1]) + int(stack_[2]);
    shake_ = 0.5f;
    puffs_.push_back({kBreachX + 8.f, float(kTrackY[std::clamp(track, 0, 2)]) - 8.f, 0.45f, 28.f});
    sys_->apu.noiseBurst(0.4f, 180.f, 0.22f);
    sys_->apu.tone(0, 90.f, 0.08f);
    beep_ = 0.12f;
    sys_->rumble(0.7f, 0.3f, 160);
    sys_->setLight(150, 30, 18);
    if (bays_ <= 0) loseWatch();
}

void Game::throwLamp() {
    if (lampCd_ > 0 || lamps_.size() >= 6 || won_ || bell_) return;
    lampCd_ = kLampCool;
    flash_ = 0.06f;
    Lamp L;
    L.track = cover();
    L.x = L.prev = x_ + 14.f;
    L.on = true;
    lamps_.push_back(L);
    sys_->apu.tone(0, 880.f, 0.04f);
    beep_ = std::max(beep_, 0.04f);
}

void Game::brake() {
    if (brakeCd_ > 0 || won_ || bell_) return;
    int here = cover();
    int hit = -1;
    float nearest = 1.0e9f;
    for (int i = 0; i < int(foes_.size()); i++) {
        Foe& f = foes_[size_t(i)];
        if (!f.on || f.track != here) continue;
        float front = frontOf(f.x, f.kind);
        if (front > x_ + 40.f || front + halfOf(f.kind) < x_ - 18.f) continue;
        if (front < nearest) {
            nearest = front;
            hit = i;
        }
    }
    if (hit < 0) return;
    brakeCd_ = kBrakeCool;
    flash_ = 0.05f;
    sys_->apu.noiseBurst(0.2f, 520.f, 0.05f);
    sys_->apu.tone(2, 160.f, 0.05f);
    beep_ = std::max(beep_, 0.05f);
    kill(foes_[size_t(hit)]);
}

void Game::winWatch() {
    if (won_ || mode_ == Mode::Over) return;
    won_ = true;
    over_ = true;
    mode_ = Mode::Victory;
    reason_ = "THE DEPOT HELD UNTIL THE RELIEF BELL";
    score_ += 600 + bays_ * 300;
    fanStep_ = 0;
    fanT_ = 0;
    sys_->apu.setVol(0, 0.03f);
    sys_->apu.keyOn(2, 523.f, 0.2f);
    sys_->rumble(0.25f, 0.45f, 180);
    sys_->setLight(40, 140, 60);
}

void Game::loseWatch() {
    if (won_ || mode_ == Mode::Over) return;
    reason_ = "THE DEPOT FELL";
    won_ = false;
    over_ = true;
    bell_ = false;
    mode_ = Mode::Over;
    shake_ = 0.7f;
    sys_->apu.keyOff(0);
    sys_->apu.keyOff(1);
    sys_->apu.noiseBurst(0.45f, 120.f, 0.3f);
    sys_->apu.tone(0, 70.f, 0.1f);
    beep_ = 0.2f;
    sys_->rumble(0.85f, 0.4f, 240);
    sys_->setLight(160, 24, 16);
}

void Game::update(float dt) {
    if (mode_ != Mode::Watch) return;
    watch_ += dt;

    if (!bell_) {
        while (spawnAt_ < int(script_.size()) && script_[size_t(spawnAt_)].t <= watch_) {
            const Spawn& s = script_[size_t(spawnAt_)];
            spawnAt_++;
            Foe f;
            f.kind = s.kind;
            f.track = s.track;
            f.hp = 1;
            f.points = ptsOf(s.kind);
            f.x = kSpawnX;
            f.age = 0;
            f.on = true;
            foes_.push_back(f);
            sys_->apu.tone(2, 140.f, 0.03f);
            beep_ = std::max(beep_, 0.03f);
        }

        int best = soonest(-1);
        bool wantLamp = false;
        bool wantBrake = false;
        if (bot_) {
            if (best >= 0) track_ = foes_[size_t(best)].track;
            float dx = kStandX - x_;
            float step = 140.f * dt;
            if (std::fabs(dx) <= step) x_ = kStandX;
            else x_ += std::copysign(step, dx);
            int here = cover();
            int local = soonest(here);
            if (local >= 0 && here == track_) {
                float front = frontOf(foes_[size_t(local)].x, foes_[size_t(local)].kind);
                if (front < x_ + 40.f && front + halfOf(foes_[size_t(local)].kind) > x_ - 18.f) wantBrake = true;
                else wantLamp = true;
            }
        } else {
            const gs::Pad& pad = sys_->pad;
            int want = 0;
            if (pad.down(gs::BTN_UP) || pad.axisY > 0.55f) want -= 1;
            if (pad.down(gs::BTN_DOWN) || pad.axisY < -0.55f) want += 1;
            if (want && trackCd_ <= 0.f) {
                int n = std::clamp(track_ + want, 0, 2);
                if (n != track_) {
                    track_ = n;
                    trackCd_ = 0.16f;
                }
            }
            float dir = 0;
            if (std::fabs(pad.axisX) > 0.18f) dir = std::clamp(pad.axisX, -1.f, 1.f);
            else dir = float(pad.down(gs::BTN_RIGHT)) - float(pad.down(gs::BTN_LEFT));
            moving_ = dir;
            x_ = std::clamp(x_ + dir * kMoveX * dt, 104.f, 270.f);
            wantLamp = pad.down(gs::BTN_A) || pad.down(gs::BTN_C) || pad.down(gs::BTN_TURBO) || pad.accel > 0.45f;
            wantBrake = pad.down(gs::BTN_B) || pad.down(gs::BTN_X);
        }

        float goal = float(kTrackY[track_]);
        float dy = goal - y_;
        float step = kSlide * dt;
        if (std::fabs(dy) <= step) y_ = goal;
        else y_ += std::copysign(step, dy);

        if (wantBrake) brake();
        if (wantLamp) throwLamp();

        for (Foe& f : foes_) {
            if (!f.on) continue;
            f.age += dt;
            f.x -= speedOf(f.kind) * dt;
        }

        for (Lamp& L : lamps_) {
            if (!L.on) continue;
            L.prev = L.x;
            L.x += kLampV * dt;
            int hit = -1;
            float bestFront = 1.0e9f;
            for (int i = 0; i < int(foes_.size()); i++) {
                Foe& f = foes_[size_t(i)];
                if (!f.on || f.track != L.track) continue;
                float front = frontOf(f.x, f.kind);
                float back = f.x + halfOf(f.kind) * 0.35f;
                if (L.x >= front && L.prev <= back && front < bestFront) {
                    bestFront = front;
                    hit = i;
                }
            }
            if (hit >= 0) {
                L.on = false;
                kill(foes_[size_t(hit)]);
            } else if (L.x > 360.f) {
                L.on = false;
            }
        }

        if (!won_ && mode_ == Mode::Watch) {
            for (Foe& f : foes_) {
                if (!f.on) continue;
                if (frontOf(f.x, f.kind) > kBreachX) continue;
                f.on = false;
                breach(f.track);
                if (over_) break;
            }
        }

        if (!over_ && watch_ >= kBellAt && bays_ > 0) {
            bell_ = true;
            bellTick_ = 0.7f;
            lamps_.clear();
            sys_->apu.keyOn(1, 392.f, 0.26f);
            sys_->rumble(0.3f, 0.55f, 140);
            sys_->setLight(170, 120, 36);
        }
    } else {
        float goal = float(kTrackY[track_]);
        float dy = goal - y_;
        float step = kSlide * dt;
        if (std::fabs(dy) <= step) y_ = goal;
        else y_ += std::copysign(step, dy);
        for (Foe& f : foes_) {
            if (!f.on) continue;
            f.age += dt;
            if (f.kind == Wagon) continue;
            f.x += 96.f * dt;
            if (f.x > 400.f) f.on = false;
        }
        if (watch_ >= kBellAt + kRing) winWatch();
    }

    foes_.erase(std::remove_if(foes_.begin(), foes_.end(), [](const Foe& f) { return !f.on; }), foes_.end());
    lamps_.erase(std::remove_if(lamps_.begin(), lamps_.end(), [](const Lamp& L) { return !L.on; }), lamps_.end());
    for (Puff& p : puffs_) p.t -= dt;
    puffs_.erase(std::remove_if(puffs_.begin(), puffs_.end(), [](const Puff& p) { return p.t <= 0; }), puffs_.end());
    for (Pop& p : pops_) {
        p.t -= dt;
        p.y -= 16.f * dt;
    }
    pops_.erase(std::remove_if(pops_.begin(), pops_.end(), [](const Pop& p) { return p.t <= 0; }), pops_.end());
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet) {
    if (h < 2.f || m.h < 1 || m.w < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 40 || s.y > gs::SCREEN_H + 40 || s.x + s.w < -40 || s.y + s.h < -40) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::shadow(float cx, float cy, float w) {
    if (w < 6.f) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 6L, 420L));
    s.h = int16_t(std::max(4L, std::lround(double(w) * 0.18)));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.45f));
    s.img = art_.shadow.pick(float(s.h));
    s.pal = 0;
    s.shadow = true;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    const float adv = 16.f * scale;
    float left = x - float(s.size()) * adv * 0.5f;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, left + float(i) * adv + adv * 0.5f, y, float(g.h) * scale, pal, false, false);
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
    float shx = 0, shy = 0;
    if (shake_ > 0) {
        shx = std::sin(modeT_ * 70.f) * 4.f * shake_;
        shy = std::cos(modeT_ * 50.f) * 2.f * shake_;
    }

    float warm = 0.15f;
    if (mode_ == Mode::Watch || mode_ == Mode::Pause || mode_ == Mode::Victory || mode_ == Mode::Over)
        warm = 0.15f + 0.55f * std::clamp(watch_ / kBellAt, 0.f, 1.f);
    if (bell_ || mode_ == Mode::Victory) warm = 1.f;
    uint16_t skyTop = mix(gs::rgb4(1, 1, 4), gs::rgb4(3, 2, 6), warm * 0.35f);
    uint16_t skyHor = mix(gs::rgb4(6, 3, 4), gs::rgb4(14, 8, 3), warm);
    v.setFogColor(gs::rgb4(2, 2, 4));
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.road[y].on = false;
        v.lineFog[y] = 0;
        float t = std::clamp(float(y) / 36.f, 0.f, 1.f);
        v.lineBackdrop[y] = y < 96 ? mix(skyTop, skyHor, t * t) : gs::rgb4(2, 2, 2);
    }

    auto body = [&](int kind, int frame, float x, float feet, bool flip) {
        float h = tallOf(kind);
        shadow(x + shx, feet + shy, kind == Wagon ? 54.f : 28.f);
        spr(foeImg(kind, frame), x + shx, feet + shy, h, palOf(kind), flip, true);
    };

    if (mode_ == Mode::Title) text("S3 DEPOT RELIEF", 188, 16, 0.62f, PAL_AMBER);
    else if (bell_ && mode_ == Mode::Watch) text("RELIEF", 188, 16, 0.9f, PAL_AMBER);
    else if (mode_ == Mode::Victory) text("HELD", 188, 16, 1.05f, PAL_GREEN);
    else if (mode_ == Mode::Over) text("FELL", 188, 16, 1.05f, PAL_RED);
    else if (mode_ == Mode::Pause) text("PAUSE", 188, 16, 1.0f, PAL_HUD);
    for (const Pop& p : pops_) text("+" + std::to_string(p.pts), p.x + shx, p.y, 0.5f, PAL_AMBER);

    float px = x_;
    float py = y_;
    if (mode_ == Mode::Title) {
        px = 156.f + std::sin(modeT_ * 0.7f) * 34.f;
        py = float(kTrackY[1]);
    }
    int fr = int(modeT_ * ((std::fabs(moving_) > 0.1f || mode_ == Mode::Title) ? 8.f : 3.f)) & 1;
    spr(art_.clerk[fr], px + shx, py + shy, 46.f, PAL_CLERK, false, true);
    shadow(px + shx, py + shy, 26.f);
    if (flash_ > 0) spr(art_.glint, px + 18.f + shx, py - 28.f, 12.f, PAL_FX, false, false);

    for (const Lamp& L : lamps_) {
        spr(art_.lamp, L.x + shx, float(kTrackY[L.track]) - 18.f, 12.f, PAL_FX, false, false);
    }
    for (const Puff& p : puffs_) {
        float k = std::clamp(p.t / 0.4f, 0.f, 1.f);
        spr(art_.dust, p.x + shx, p.y, p.h * (1.15f - k), PAL_FX, false, false);
    }

    for (int t = 2; t >= 0; --t) {
        for (const Foe& f : foes_) {
            if (!f.on || f.track != t) continue;
            int frame = int(f.age * (f.kind == Wagon ? 6.f : 8.f)) & 1;
            bool flip = bell_ && f.kind != Wagon;
            body(f.kind, frame, f.x, float(kTrackY[t]), flip);
        }
    }

    if (mode_ == Mode::Title) body(Wagon, int(modeT_ * 2.f) & 1, 214.f, float(kTrackY[0]), false);

    if (bell_ || mode_ == Mode::Victory) {
        float u = std::clamp(watch_ - kBellAt, 0.f, kRing);
        body(Runner, int(modeT_ * 9.f) & 1, 150.f + u * 86.f, float(kTrackY[0]), true);
        body(Runner, int(modeT_ * 9.f + 1.f) & 1, 188.f + u * 96.f, float(kTrackY[2]), true);
        float lx = 348.f - u * (118.f / kRing);
        shadow(lx + shx, float(kTrackY[1]) + shy, 60.f);
        spr(art_.loco, lx + shx, float(kTrackY[1]) + shy, 36.f, PAL_LOCO, false, true);
        spr(art_.cloud, lx + 8.f, float(kTrackY[1]) - 40.f, 10.f, PAL_NIGHT, false, false);
    }

    for (int t = 0; t < 3; t++) {
        float feet = float(kTrackY[t]);
        spr(art_.buffer, 84.f + shx, feet + shy, 16.f, PAL_NIGHT, false, true);
        spr(art_.signal, 292.f, feet, 26.f, PAL_NIGHT, false, true);
        if (stack_[t]) {
            spr(art_.crate, 100.f, feet - 12.f, 13.f, PAL_WOOD, true, true);
            spr(art_.crate, 98.f, feet, 13.f, PAL_WOOD, false, true);
        }
        spr(art_.lantern, 70.f, feet - 30.f, 14.f, PAL_BELL, false, false);
    }

    float swing = std::sin(modeT_ * (bell_ ? 9.f : 2.1f)) * (bell_ ? 4.f : 1.2f);
    spr(art_.bell, 20.f + swing, 20.f, bell_ ? 24.f : 20.f, PAL_BELL, false, false);
    spr(art_.rope, 20.f + swing * 0.4f, 40.f, 28.f, PAL_BELL, false, false);
    spr(art_.sign, 176.f, 54.f, 14.f, PAL_HUD, false, false);
    spr(art_.lantern, 168.f, kPlatformY, 22.f, PAL_BELL, false, true);
    spr(art_.lantern, 236.f, kPlatformY, 22.f, PAL_BELL, false, true);
    spr(art_.barrel, 206.f, kPlatformY, 16.f, PAL_WOOD, false, true);
    spr(art_.crate, 128.f, kPlatformY, 14.f, PAL_WOOD, false, true);

    float smoke = std::fmod(modeT_ * 10.f, 28.f);
    spr(art_.cloud, 62.f + smoke * 0.4f, 22.f - smoke * 0.25f, 10.f, PAL_NIGHT, false, false);

    const float sx[6] = {48, 96, 140, 210, 260, 304};
    const float sy[6] = {6, 12, 4, 10, 6, 14};
    if (!bell_ && mode_ != Mode::Victory) {
        for (int i = 0; i < 6; i++) spr(art_.star, sx[i], sy[i], 4.f, PAL_NIGHT, false, false);
    }
    spr(art_.moon, 248.f, 10.f, bell_ ? 11.f : 14.f, bell_ ? PAL_AMBER : PAL_NIGHT, false, false);

    char buf[48];
    if (mode_ == Mode::Watch || mode_ == Mode::Pause) {
        if (bell_) std::snprintf(buf, sizeof buf, "BELL");
        else std::snprintf(buf, sizeof buf, "BELL %d", int(std::ceil(std::max(0.f, kBellAt - watch_))));
        hud(6, 1, buf, bell_ ? PAL_AMBER : PAL_HUD);
        std::snprintf(buf, sizeof buf, "%d", score_);
        hud(39 - int(std::strlen(buf)), 1, buf, PAL_AMBER);
        std::snprintf(buf, sizeof buf, "TRACK %d", cover() + 1);
        hud(1, 25, buf, PAL_HUD);
        std::string bays = "BAYS ";
        for (int i = 0; i < 3; i++) bays += stack_[i] ? '#' : '.';
        hud(1, 26, bays, bays_ == 1 ? PAL_RED : PAL_HUD);
        hud(24, 26, bell_ ? "THEN IT IS DONE" : "HOLD THE DEPOT", bell_ ? PAL_AMBER : PAL_HUD);
        if (!bell_) {
            int s = soonest(-1);
            if (s >= 0 && foes_[size_t(s)].track != cover() &&
                etaOf(frontOf(foes_[size_t(s)].x, foes_[size_t(s)].kind), foes_[size_t(s)].kind) < 2.4f)
                hudC(3, "SWITCH TRACK", (int(modeT_ * 6.f) & 1) ? PAL_RED : PAL_AMBER);
            else if (watch_ < 3.2f)
                hudC(3, "HOLD THE DEPOT", PAL_AMBER);
        }
    }

    if (mode_ == Mode::Title) {
        hud(39 - int(std::strlen(S3_VERSION_STRING)), 0, S3_VERSION_STRING, PAL_HUD);
        hudC(23, "UP DOWN TRACK    Z LAMP    X BRAKE", PAL_HUD);
        hudC(24, "HOLD UNTIL THE RELIEF BELL", PAL_AMBER);
        hudC(25, "THEN IT IS DONE", PAL_HUD);
        if ((int(modeT_ * 2.f) & 1) == 0) hudC(26, "PRESS START", PAL_AMBER);
    } else if (mode_ == Mode::Pause) {
        hudC(24, "START  RESUME", PAL_HUD);
        hudC(25, "ESC    TITLE", PAL_HUD);
    } else if (mode_ == Mode::Victory) {
        hudC(23, "THE DEPOT HELD", PAL_GREEN);
        hudC(24, "UNTIL THE RELIEF BELL", PAL_AMBER);
        std::snprintf(buf, sizeof buf, "STOPPED %d   BAYS %d", stopped_, bays_);
        hudC(25, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "SCORE %d    START", score_);
        hudC(26, buf, PAL_AMBER);
    } else if (mode_ == Mode::Over) {
        hudC(24, "THE DEPOT FELL", PAL_RED);
        std::snprintf(buf, sizeof buf, "STOPPED %d", stopped_);
        hudC(25, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "SCORE %d    START", score_);
        hudC(26, buf, PAL_AMBER);
    }

    if (mode_ == Mode::Title) sys_->setLight(64, 42, 28);
    else if (mode_ == Mode::Victory) sys_->setLight(40, 140, 60);
    else if (mode_ == Mode::Over) sys_->setLight(160, 24, 16);
    else if (bell_) sys_->setLight(170, 120, 36);
}

void Game::tickAudio() {
    if (beep_ > 0) {
        beep_ -= DT;
        if (beep_ <= 0) {
            sys_->apu.tone(0, 0, 0);
            sys_->apu.tone(1, 0, 0);
            sys_->apu.tone(2, 0, 0);
        }
    }
    if ((mode_ == Mode::Watch || mode_ == Mode::Victory) && bell_) {
        bellTick_ += DT;
        if (bellTick_ >= 0.72f) {
            bellTick_ = 0;
            float note = (int(watch_ * 2.f) & 1) ? 494.f : 392.f;
            sys_->apu.keyOn(1, note, 0.22f);
        }
    }
    if (mode_ == Mode::Victory && fanStep_ >= 0 && fanStep_ < 4) {
        fanT_ += DT;
        const float notes[4] = {523.f, 659.f, 784.f, 1046.f};
        const float when[4] = {0.f, 0.12f, 0.24f, 0.4f};
        if (fanT_ >= when[fanStep_]) {
            sys_->apu.keyOn(2, notes[fanStep_], 0.16f);
            fanStep_++;
        }
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.85f);
    sys.apu.setEcho(0.16f, 0.32f, 0.18f);
    sys.apu.setPatch(0, dronePatch());
    sys.apu.setPatch(1, bellPatch());
    sys.apu.setPatch(2, bellPatch());
    x_ = kStandX;
    y_ = float(kTrackY[1]);
    if (bot_) beginWatch();
    else {
        mode_ = Mode::Title;
        sys.setLight(64, 42, 28);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (bot_ && mode_ == Mode::Title) beginWatch();
    modeT_ += DT;
    if (lampCd_ > 0) lampCd_ -= DT;
    if (brakeCd_ > 0) brakeCd_ -= DT;
    if (trackCd_ > 0) trackCd_ -= DT;
    if (flash_ > 0) flash_ -= DT;
    if (shake_ > 0) shake_ = std::max(0.f, shake_ - DT);

    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        moving_ = std::cos(modeT_ * 0.7f) >= 0 ? 1.f : -1.f;
        if (!bot_ && pad.pressed(gs::BTN_START)) beginWatch();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) sys.quit();
    } else if (mode_ == Mode::Watch) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
        } else {
            update(DT);
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Watch;
        else if (pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            bell_ = false;
            foes_.clear();
            lamps_.clear();
            sys.apu.silence();
            sys.apu.setPatch(0, dronePatch());
            sys.apu.setPatch(1, bellPatch());
            sys.apu.setPatch(2, bellPatch());
        }
    } else if (mode_ == Mode::Victory || mode_ == Mode::Over) {
        if (!bot_ && pad.pressed(gs::BTN_START)) beginWatch();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
            bell_ = false;
            foes_.clear();
            lamps_.clear();
            pops_.clear();
            sys.apu.silence();
            sys.apu.setPatch(0, dronePatch());
            sys.apu.setPatch(1, bellPatch());
            sys.apu.setPatch(2, bellPatch());
        }
    }

    tickAudio();
    draw();
}

}  // namespace depot

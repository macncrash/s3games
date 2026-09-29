#include "cistern.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace cistern {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float BELL_AT = 20.f;
constexpr float BELL_END = 32.f;
constexpr float ROPE_NEED = 0.8f;
constexpr float CX = 160.f;
constexpr float CY = 120.f;
constexpr int BEARS = 8;

float speedOf(int kind) {
    if (kind == 1) return 0.22f;
    if (kind == 2) return 0.13f;
    return 0.16f;
}
int hpOf(int kind) { return kind == 2 ? 2 : 1; }
int ptsOf(int kind) { return kind == 2 ? 250 : kind == 1 ? 150 : 100; }

struct Arr {
    float t;
    int kind;
    int bear;
};
const Arr kArr[] = {
    {1.1f, 0, 0},  {3.4f, 0, 3},  {5.8f, 1, 6},  {8.2f, 0, 1},  {10.6f, 2, 4},
    {13.4f, 1, 7}, {16.0f, 0, 2}, {19.2f, 0, 5}, {22.4f, 1, 0}, {25.5f, 0, 3},
};

float ang(int bear) { return bear * 6.2831853f / float(BEARS) - 1.5707963f; }

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Victory || mode_ == Mode::Over) return 3;
    if (bell_) return 2;
    return 1;
}

float Game::eta(const Foe& f) const { return f.alive ? f.r / f.speed : 1.0e9f; }

void Game::place(int bear, float rad, float& x, float& y) const {
    float a = ang(bear);
    float d = 30.f + rad * 78.f;
    x = CX + std::cos(a) * d;
    y = CY + std::sin(a) * d * 0.62f;
}

void Game::winWatch() {
    if (won_ || mode_ == Mode::Over) return;
    won_ = true;
    over_ = true;
    mode_ = Mode::Victory;
    modeT_ = 0;
    reason_ = "THE WATCH HELD UNTIL THE RELIEF BELL";
    sys_->apu.keyOff(1);
    sys_->apu.noiseBurst(0.12f, 1600.f, 0.06f);
    fanStep_ = 0;
    fanT_ = 0;
    sys_->setLight(40, 160, 80);
}

void Game::loseWatch(const char* why) {
    if (won_ || mode_ == Mode::Over || mode_ == Mode::Victory) return;
    reason_ = why;
    won_ = false;
    over_ = true;
    mode_ = Mode::Over;
    modeT_ = 0;
    sys_->apu.keyOff(1);
    sys_->apu.noiseBurst(0.4f, 160.f, 0.3f);
    sys_->setLight(170, 20, 30);
    shake_ = 0.5f;
}

void Game::kill(Foe& f) {
    f.alive = false;
    score_ += f.points;
    sys_->apu.noiseBurst(0.14f, 800.f, 0.05f);
    sys_->apu.tone(0, 540.f, 0.04f);
}

void Game::shoot() {
    if (cool_ > 0) return;
    cool_ = 0.18f;
    flash_ = 4;
    sys_->apu.noiseBurst(0.2f, 1300.f, 0.04f);
    int best = -1;
    float bestR = 1.0e9f;
    for (int i = 0; i < int(foes_.size()); i++) {
        if (!foes_[i].alive || foes_[i].bear != face_) continue;
        if (foes_[i].r < bestR) {
            bestR = foes_[i].r;
            best = i;
        }
    }
    if (best < 0 || bestR > 1.05f) return;
    Foe& f = foes_[best];
    f.hp -= 1;
    if (f.hp <= 0) kill(f);
    else sys_->apu.tone(2, 220.f, 0.05f);
}

void Game::beginWatch() {
    mode_ = Mode::Watch;
    over_ = false;
    won_ = false;
    bell_ = false;
    reason_ = "THE WATCH FAILED";
    score_ = 0;
    face_ = 0;
    spawnAt_ = 0;
    flash_ = 0;
    fanStep_ = -1;
    watch_ = 0;
    rope_ = 0;
    cool_ = 0;
    shake_ = 0;
    bellTick_ = 0;
    modeT_ = 0;
    foes_.clear();
    script_.clear();
    for (const Arr& a : kArr) {
        Spawn s;
        s.t = a.t;
        s.kind = a.kind;
        s.bear = a.bear;
        script_.push_back(s);
    }
    if (sys_) sys_->setLight(20, 40, 50);
}

void Game::update(float dt) {
    while (spawnAt_ < int(script_.size()) && script_[spawnAt_].t <= watch_) {
        const Spawn& s = script_[spawnAt_++];
        Foe f;
        f.kind = s.kind;
        f.bear = s.bear;
        f.r = 1.f;
        f.speed = speedOf(s.kind);
        f.hp = hpOf(s.kind);
        f.points = ptsOf(s.kind);
        f.alive = true;
        foes_.push_back(f);
    }

    bool wasBell = bell_;
    bell_ = watch_ >= BELL_AT && mode_ == Mode::Watch;
    if (bell_ && !wasBell) {
        bellTick_ = 0;
        sys_->apu.keyOn(1, 440.f, 0.22f);
        sys_->rumble(0.25f, 0.4f, 140);
    } else if (bell_ && !won_) {
        bellTick_ += dt;
        if (bellTick_ >= 0.95f) {
            bellTick_ = 0;
            sys_->apu.keyOn(1, 440.f, 0.18f);
        }
    }

    int best = -1;
    float mind = 1.0e9f;
    for (int i = 0; i < int(foes_.size()); i++) {
        if (!foes_[i].alive) continue;
        float e = eta(foes_[i]);
        if (e < mind) {
            mind = e;
            best = i;
        }
    }

    bool pull = false;
    if (bot_) {
        bool safe = mind > 1.15f;
        if (bell_ && safe && watch_ < BELL_END) pull = true;
        else if (best >= 0) face_ = foes_[best].bear;
    } else {
        const gs::Pad& pad = sys_->pad;
        if (pad.pressed(gs::BTN_LEFT)) face_ = (face_ + BEARS - 1) % BEARS;
        if (pad.pressed(gs::BTN_RIGHT)) face_ = (face_ + 1) % BEARS;
        bool fire = pad.down(gs::BTN_A) || pad.down(gs::BTN_C) || pad.down(gs::BTN_Z) || pad.accel > 0.45f;
        bool up = pad.down(gs::BTN_UP) || pad.down(gs::BTN_B);
        if (bell_ && up && !fire) pull = true;
        else if (fire) shoot();
    }

    if (bot_ && !pull && best >= 0 && foes_[best].bear == face_ && eta(foes_[best]) < 5.5f) shoot();

    if (pull) {
        rope_ += dt;
        if (rope_ >= ROPE_NEED && bell_) winWatch();
    } else if (rope_ > 0) {
        rope_ = std::max(0.f, rope_ - dt * 0.55f);
    }

    if (mode_ == Mode::Watch) {
        for (Foe& f : foes_) {
            if (!f.alive) continue;
            f.age++;
            f.r -= f.speed * dt;
            if (f.r <= 0.02f) {
                loseWatch("THEY REACH THE CISTERN");
                return;
            }
        }
        if (bell_ && watch_ >= BELL_END && !won_) loseWatch("THE BELL WENT UNANSWERED");
    }
    watch_ += dt;
    if (cool_ > 0) cool_ -= dt;
    if (flash_ > 0) flash_--;
    if (shake_ > 0) shake_ = std::max(0.f, shake_ - dt);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::lround(cx - s.w * 0.5f + shx_));
    s.y = int16_t(std::lround(cy - s.h * 0.5f + shy_));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        int x = col + i;
        if (x < 0 || x > 39 || c < 32 || c > 126) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = int(std::strlen(s));
    hud(20 - n / 2, row, s, pal);
}

void Game::text(const char* s, float x, float y, float scale, int pal) {
    int n = int(std::strlen(s));
    float adv = 16.f * scale;
    float left = x - n * adv * 0.5f;
    for (int i = 0; i < n; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < 33 || c > 126) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, left + float(i) * adv + adv * 0.5f, y, std::max(8.f, float(g.h) * scale), pal, false, 0);
    }
}

void Game::draw() {
    shx_ = shy_ = 0;
    if (shake_ > 0.05f) {
        shx_ = std::sin(watch_ * 40.f) * 3.f;
        shy_ = std::cos(watch_ * 33.f) * 1.5f;
    }
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.hudEnabled = true;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int sky = y < 70 ? 1 : 2;
        v.lineBackdrop[y] = gs::rgb4(sky, sky + 1, sky + 3);
        v.lineFog[y] = uint8_t(y < 40 ? 2 : 0);
        v.road[y].on = false;
    }

    spr(art_.basin, CX, CY + 4.f, 78.f, PAL_WATER, false, 0);
    spr(art_.ring, CX, CY, 112.f, PAL_STONE, false, 0);
    spr(art_.reed, 96.f, 168.f, 22.f, PAL_WATER, false, 0);
    spr(art_.reed, 224.f, 166.f, 18.f, PAL_WATER, true, 0);

    float swing = std::sin((modeT_ + watch_) * (bell_ ? 8.f : 1.6f)) * (bell_ ? 6.f : 1.4f);
    float lift = std::clamp(rope_ / ROPE_NEED, 0.f, 1.f) * 18.f;
    spr(art_.bell, CX + swing, 18.f, bell_ ? 24.f : 18.f, PAL_BELL, false, 0);
    spr(art_.rope, CX + swing * 0.25f, 40.f - lift * 0.2f, 36.f - lift, PAL_BELL, false, 0);
    spr(art_.bucket, CX + swing * 0.15f, 62.f - lift, 14.f, PAL_BELL, false, 0);

    if (mode_ == Mode::Title) text("S3 CISTERN RELIEF", 160, 16, 0.7f, PAL_HUD);
    else if (mode_ == Mode::Victory) text("RELIEF", 160, 16, 1.0f, PAL_OK);
    else if (mode_ == Mode::Over) text("WATCH OVER", 160, 16, 0.8f, PAL_ALERT);
    else if (bell_) text("THE BELL", 160, 16, 0.75f, PAL_BELL);

    auto drawFoe = [&](const Foe& f) {
        float x, y;
        place(f.bear, f.r, x, y);
        float h = 12.f + (1.f - f.r) * 22.f;
        int fog = int(std::clamp(f.r * 11.f, 0.f, 12.f));
        const gs::Mipped* img = &art_.raider[(f.age / 8) & 1];
        int pal = PAL_FOE;
        if (f.kind == RUNNER) img = &art_.runner[(f.age / 5) & 1];
        if (f.kind == PLATE) {
            img = &art_.plate[(f.age / 8) & 1];
            pal = PAL_PLATE;
        }
        bool flip = std::cos(ang(f.bear)) < 0;
        spr(*img, x, y, h, pal, flip, fog);
    };

    if (mode_ == Mode::Title) {
        Foe demo;
        demo.bear = 1;
        demo.r = 0.72f;
        demo.kind = RAIDER;
        demo.age = int(modeT_ * 8.f);
        drawFoe(demo);
        demo.bear = 5;
        demo.r = 0.9f;
        demo.kind = RUNNER;
        drawFoe(demo);
    } else {
        std::vector<int> order;
        for (int i = 0; i < int(foes_.size()); i++)
            if (foes_[i].alive) order.push_back(i);
        std::sort(order.begin(), order.end(), [&](int a, int b) { return foes_[a].r > foes_[b].r; });
        for (int i : order) drawFoe(foes_[i]);
    }

    float px, py;
    place(face_, 0.05f, px, py);
    if (flash_ > 0) {
        float fx, fy;
        place(face_, 0.28f, fx, fy);
        spr(art_.flash, fx, fy, 12.f, PAL_FX, false, 0);
    }
    spr(art_.sentry[int(watch_ * 6.f) & 1], px, py, 28.f, PAL_YOU, std::cos(ang(face_)) < 0, 0);

    char line[48];
    if (mode_ == Mode::Title) {
        hudC(23, "AT THE CISTERN.", PAL_HUD);
        hudC(24, "HOLD UNTIL THE RELIEF BELL.", PAL_OK);
        hudC(25, "LEFT RIGHT THE RIM    A FIRE", PAL_HUD);
        hudC(26, "UP HAULS THE ROPE WHEN IT RINGS", PAL_BELL);
        hudC(27, "ENTER", PAL_HUD);
        int n = int(std::strlen(S3_VERSION_STRING));
        hud(39 - n, 0, S3_VERSION_STRING, PAL_HUD);
    } else {
        int left = std::max(0, int(std::ceil(BELL_AT - watch_)));
        if (!bell_) std::snprintf(line, sizeof line, "BELL %02d", left);
        else std::snprintf(line, sizeof line, "ROPE %d", int(std::clamp(rope_ / ROPE_NEED, 0.f, 1.f) * 10.f));
        hud(1, 0, line, bell_ ? PAL_OK : PAL_HUD);
        std::snprintf(line, sizeof line, "SCORE %d", score_);
        hud(1, 27, line, PAL_HUD);
        if (mode_ == Mode::Pause) hudC(26, "PAUSED", PAL_HUD);
        else if (mode_ == Mode::Victory) hudC(26, "THE WATCH HELD", PAL_OK);
        else if (mode_ == Mode::Over) hudC(26, reason_, PAL_ALERT);
        else if (bell_) hudC(26, "ANSWER THE BELL", PAL_BELL);
        else hudC(26, "HOLD THE RIM", PAL_HUD);
    }
    if (mode_ == Mode::Victory) sys_->setLight(40, 160, 80);
    else if (mode_ == Mode::Over) sys_->setLight(170, 20, 30);
    else if (bell_) sys_->setLight(150, 120, 30);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    mode_ = Mode::Title;
    modeT_ = 0;
    if (bot_) beginWatch();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const float dt = DT;
    modeT_ += dt;
    const gs::Pad& pad = sys.pad;
    bool start = !bot_ && pad.pressed(gs::BTN_START);
    bool back = !bot_ && pad.pressed(gs::BTN_MODE);
    if (back && mode_ == Mode::Title) {
        if (sys.hasHome()) sys.eject();
        else sys.quit();
    } else if (back && mode_ == Mode::Watch) mode_ = Mode::Pause;
    else if (back && mode_ == Mode::Pause) mode_ = Mode::Title;
    else if (start && (mode_ == Mode::Title || mode_ == Mode::Victory || mode_ == Mode::Over)) beginWatch();
    else if (start && mode_ == Mode::Watch) mode_ = Mode::Pause;
    else if (start && mode_ == Mode::Pause) mode_ = Mode::Watch;

    if (mode_ == Mode::Watch) update(dt);
    else if (mode_ == Mode::Victory && fanStep_ >= 0) {
        fanT_ += dt;
        if (fanT_ > 0.16f) {
            fanT_ = 0;
            static const float notes[] = {392.f, 494.f, 587.f, 784.f};
            if (fanStep_ < 4) sys.apu.tone(0, notes[fanStep_], 0.06f);
            fanStep_++;
            if (fanStep_ > 6) {
                fanStep_ = -1;
                sys.apu.tone(0, 0, 0);
            }
        }
    }
    draw();
}

}  // namespace cistern

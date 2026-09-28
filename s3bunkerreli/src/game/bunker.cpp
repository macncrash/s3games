#include "bunker.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace bunker {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float Z0 = 1.25f;
constexpr float BELL_AT = 22.f;
constexpr float BELL_END = 34.f;
constexpr float ROPE_NEED = 0.85f;
constexpr float SLIT_L = 92.f;
constexpr float SLIT_R = 228.f;
constexpr float SLIT_T = 70.f;
constexpr float SLIT_B = 122.f;

float speedOf(int kind) {
    if (kind == 1) return 0.42f;
    if (kind == 2) return 0.22f;
    return 0.28f;
}
int hpOf(int kind) { return kind == 2 ? 2 : 1; }
int ptsOf(int kind) { return kind == 2 ? 250 : kind == 1 ? 150 : 100; }

struct Arr {
    float t;
    int kind;
    int lane;
};
const Arr kArr[] = {
    {1.0f, 0, 0},  {2.6f, 0, -1}, {4.0f, 0, 1},   {5.6f, 1, 0},  {7.2f, 0, -1}, {8.8f, 0, 1},
    {10.2f, 2, 0}, {12.4f, 1, 1}, {13.8f, 0, -1}, {15.4f, 0, 0}, {17.0f, 1, -1}, {18.6f, 0, 1},
    {20.2f, 0, 0}, {23.2f, 0, 1}, {25.6f, 1, -1}, {28.4f, 0, 0},
};

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Victory || mode_ == Mode::Over) return 3;
    if (bell_) return 2;
    return 1;
}

float Game::eta(const Foe& f) const { return f.alive ? f.z / f.speed : 1.0e9f; }

void Game::place(const Foe& f, float& x, float& y, float& h) const {
    float near = 1.f - std::clamp(f.z / Z0, 0.f, 1.f);
    x = 160.f + float(f.lane) * (16.f + near * 42.f);
    y = 86.f + near * 22.f;
    h = 12.f + near * (f.kind == PLATE ? 40.f : 32.f);
}

void Game::winWatch() {
    if (won_ || mode_ == Mode::Over) return;
    won_ = true;
    over_ = true;
    mode_ = Mode::Victory;
    modeT_ = 0;
    reason_ = "THE WATCH HELD UNTIL THE RELIEF BELL";
    sys_->apu.keyOff(1);
    sys_->apu.noiseBurst(0.12f, 1800.f, 0.06f);
    fanStep_ = 0;
    fanT_ = 0;
    sys_->setLight(40, 180, 90);
}

void Game::loseWatch(const char* why) {
    if (won_ || mode_ == Mode::Over || mode_ == Mode::Victory) return;
    reason_ = why;
    won_ = false;
    over_ = true;
    mode_ = Mode::Over;
    modeT_ = 0;
    sys_->apu.keyOff(1);
    sys_->apu.noiseBurst(0.4f, 180.f, 0.3f);
    sys_->setLight(180, 20, 30);
    shake_ = 0.5f;
}

void Game::kill(Foe& f) {
    f.alive = false;
    score_ += f.points;
    sys_->apu.noiseBurst(0.16f, 900.f, 0.05f);
    sys_->apu.tone(0, 620.f, 0.05f);
}

void Game::shoot() {
    if (cool_ > 0) return;
    cool_ = 0.2f;
    flash_ = 4;
    sys_->apu.noiseBurst(0.22f, 1400.f, 0.04f);
    int best = -1;
    float bestZ = 1.0e9f;
    for (int i = 0; i < int(foes_.size()); i++) {
        if (!foes_[i].alive || foes_[i].lane != lane_) continue;
        if (foes_[i].z < bestZ) {
            bestZ = foes_[i].z;
            best = i;
        }
    }
    if (best < 0 || bestZ > 1.15f) return;
    Foe& f = foes_[best];
    f.hp -= 1;
    if (f.hp <= 0) kill(f);
    else sys_->apu.tone(2, 240.f, 0.05f);
}

void Game::beginWatch() {
    mode_ = Mode::Watch;
    over_ = false;
    won_ = false;
    bell_ = false;
    reason_ = "THE WATCH FAILED";
    score_ = 0;
    lane_ = 0;
    aim_ = 0;
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
        s.lane = a.lane;
        script_.push_back(s);
    }
    if (sys_) sys_->setLight(40, 30, 20);
}

void Game::update(float dt) {
    while (spawnAt_ < int(script_.size()) && script_[spawnAt_].t <= watch_) {
        const Spawn& s = script_[spawnAt_++];
        Foe f;
        f.kind = s.kind;
        f.lane = s.lane;
        f.z = Z0;
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
        sys_->apu.keyOn(1, 520.f, 0.22f);
        sys_->rumble(0.2f, 0.35f, 120);
    } else if (bell_ && !won_) {
        bellTick_ += dt;
        if (bellTick_ >= 0.9f) {
            bellTick_ = 0;
            sys_->apu.keyOn(1, 520.f, 0.18f);
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
    int want = lane_;
    if (bot_) {
        bool closing = bell_ && (BELL_END - watch_) < ROPE_NEED + 1.2f;
        bool safe = mind > (closing ? 0.7f : 1.35f);
        if (bell_ && safe && watch_ < BELL_END) pull = true;
        else if (best >= 0) want = foes_[best].lane;
    } else {
        const gs::Pad& pad = sys_->pad;
        if (pad.pressed(gs::BTN_LEFT)) want = std::max(-1, lane_ - 1);
        if (pad.pressed(gs::BTN_RIGHT)) want = std::min(1, lane_ + 1);
        float ax = std::fabs(pad.axisX) > 0.4f ? pad.axisX : 0;
        if (ax < -0.4f && cool_ < 0.12f) want = std::max(-1, lane_ - 1);
        if (ax > 0.4f && cool_ < 0.12f) want = std::min(1, lane_ + 1);
        bool fire = pad.down(gs::BTN_A) || pad.down(gs::BTN_C) || pad.down(gs::BTN_Z) || pad.accel > 0.45f;
        bool up = pad.down(gs::BTN_UP) || pad.down(gs::BTN_B);
        if (bell_ && up && !fire) pull = true;
        else if (fire) shoot();
    }
    if (!pull) lane_ = want;
    aim_ += (float(lane_) - aim_) * std::min(1.f, dt * 10.f);

    if (bot_ && !pull && best >= 0 && foes_[best].lane == lane_ && eta(foes_[best]) < 4.2f) shoot();

    if (pull) {
        rope_ += dt;
        if (rope_ >= ROPE_NEED && bell_) winWatch();
    } else if (rope_ > 0) {
        rope_ = std::max(0.f, rope_ - dt * 0.8f);
    }

    for (Foe& f : foes_) {
        if (!f.alive) continue;
        f.age++;
        f.z -= f.speed * dt;
        if (f.z <= 0.02f) {
            loseWatch("THE DOOR OPENS");
            return;
        }
    }
    if (bell_ && watch_ >= BELL_END && !won_) loseWatch("THE BELL WENT UNANSWERED");
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

void Game::stamp(const gs::Mipped& m, float x, float y, float w, float h, int pal) {
    if (w < 1.f || h < 1.f || m.h < 1) return;
    gs::Sprite s;
    s.x = int16_t(std::lround(x + shx_));
    s.y = int16_t(std::lround(y + shy_));
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
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
        int g = 1 + (y > 150 ? 1 : 0);
        v.lineBackdrop[y] = gs::rgb4(g, g, g + 1);
        v.road[y].on = false;
    }

    float swing = std::sin((modeT_ + watch_) * (bell_ ? 9.f : 2.f)) * (bell_ ? 5.f : 1.2f);
    spr(art_.bell, 28.f + swing, 28.f, bell_ ? 26.f : 20.f, PAL_BELL, false, 0);
    spr(art_.rope, 28.f + swing * 0.3f, 58.f, 36.f, PAL_BELL, false, 0);

    float ax = 160.f + aim_ * 48.f;
    ax = std::clamp(ax, SLIT_L + 8.f, SLIT_R - 8.f);
    float ay = (SLIT_T + SLIT_B) * 0.5f;
    if (flash_ > 0) spr(art_.flash, ax, ay, 16.f, PAL_FX, false, 0);
    spr(art_.sight, ax, ay, 16.f, PAL_FX, false, 0);
    spr(art_.rifle, 168.f + aim_ * 10.f, 196.f, 48.f, PAL_YOU, false, 0);
    spr(art_.helm, 150.f, 188.f, 18.f, PAL_YOU, false, 0);

    if (mode_ == Mode::Title) text("S3 BUNKER RELIEF", 160, 18, 0.85f, PAL_HUD);
    else if (mode_ == Mode::Victory) text("RELIEF", 160, 18, 1.1f, PAL_OK);
    else if (mode_ == Mode::Over) text("WATCH OVER", 160, 18, 0.9f, PAL_ALERT);
    else if (bell_) text("THE BELL", 160, 18, 0.85f, PAL_BELL);

    stamp(art_.door, 70, 28, 180, 168, PAL_DOOR);

    if (mode_ == Mode::Title) {
        int fr = int(modeT_ * 6.f) & 1;
        spr(art_.walker[fr], 168, 96, 28, PAL_FOE, false, 3);
        spr(art_.sprinter[fr ^ 1], 120, 90, 18, PAL_FOE, true, 8);
    } else {
        std::vector<int> order;
        for (int i = 0; i < int(foes_.size()); i++)
            if (foes_[i].alive) order.push_back(i);
        std::sort(order.begin(), order.end(), [&](int a, int b) { return foes_[a].z < foes_[b].z; });
        for (int i : order) {
            const Foe& f = foes_[i];
            float x, y, h;
            place(f, x, y, h);
            int fog = int(std::clamp(f.z * 9.f, 0.f, 12.f));
            const gs::Mipped* img = &art_.walker[(f.age / 8) & 1];
            int pal = PAL_FOE;
            if (f.kind == SPRINTER) img = &art_.sprinter[(f.age / 6) & 1];
            if (f.kind == PLATE) {
                img = &art_.plate[(f.age / 8) & 1];
                pal = PAL_PLATE;
            }
            spr(*img, x, y, h, pal, f.lane < 0, fog);
        }
    }

    char line[48];
    if (mode_ == Mode::Title) {
        hudC(24, "YOU HAVE THE BUNKER.", PAL_HUD);
        hudC(25, "HOLD UNTIL THE RELIEF BELL.", PAL_OK);
        hudC(26, "LEFT RIGHT AIM   A FIRE   UP THE ROPE", PAL_HUD);
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
        else hudC(26, "HOLD THE SLIT", PAL_HUD);
    }
    if (mode_ == Mode::Victory) sys_->setLight(40, 170, 80);
    else if (mode_ == Mode::Over) sys_->setLight(170, 20, 30);
    else if (bell_) sys_->setLight(160, 130, 40);
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
            static const float notes[] = {523.f, 659.f, 784.f, 1046.f};
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

}  // namespace bunker

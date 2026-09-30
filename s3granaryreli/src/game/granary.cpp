#include "granary.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace granary {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float Z0 = 1.45f;
constexpr float BELL_AT = 18.f;
constexpr float BELL_END = 28.f;
constexpr float ROPE_NEED = 0.75f;

float speedOf(int kind) {
    if (kind == 1) return 0.38f;
    if (kind == 2) return 0.20f;
    return 0.26f;
}
int hpOf(int kind) { return kind == 2 ? 2 : 1; }
int ptsOf(int kind) { return kind == 2 ? 250 : kind == 1 ? 150 : 100; }

struct Arr {
    float t;
    int kind;
    int lane;
};
const Arr kArr[] = {
    {1.2f, 0, 0},  {3.4f, 0, -1}, {5.4f, 1, 1},  {7.6f, 2, 0},  {10.0f, 0, -1},
    {12.2f, 1, 1}, {14.2f, 0, 0}, {16.2f, 0, -1},
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
    x = 168.f + float(f.lane) * (18.f + near * 48.f);
    y = 118.f + near * 62.f;
    h = 14.f + near * (f.kind == WAGON ? 36.f : 40.f);
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
    sys_->setLight(90, 160, 40);
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
    sys_->setLight(170, 30, 20);
    shake_ = 0.45f;
}

void Game::kill(Foe& f) {
    f.alive = false;
    score_ += f.points;
    sys_->apu.noiseBurst(0.14f, 700.f, 0.05f);
    sys_->apu.tone(0, 480.f, 0.05f);
}

void Game::shoot() {
    if (cool_ > 0) return;
    cool_ = 0.18f;
    flash_ = 4;
    sys_->apu.noiseBurst(0.2f, 1100.f, 0.04f);
    int best = -1;
    float bestZ = 1.0e9f;
    for (int i = 0; i < int(foes_.size()); i++) {
        if (!foes_[i].alive || foes_[i].lane != lane_) continue;
        if (foes_[i].z < bestZ) {
            bestZ = foes_[i].z;
            best = i;
        }
    }
    if (best < 0 || bestZ > 1.25f) return;
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
    if (sys_) sys_->setLight(90, 50, 20);
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
        sys_->apu.keyOn(1, 440.f, 0.22f);
        sys_->rumble(0.15f, 0.3f, 100);
    } else if (bell_ && !won_) {
        bellTick_ += dt;
        if (bellTick_ >= 1.0f) {
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
    int want = lane_;
    if (bot_) {
        bool quiet = mind > 1.6f;
        if (bell_ && quiet && watch_ + ROPE_NEED < BELL_END - 0.2f) pull = true;
        else if (best >= 0) want = foes_[best].lane;
    } else {
        const gs::Pad& pad = sys_->pad;
        if (pad.pressed(gs::BTN_LEFT)) want = std::max(-1, lane_ - 1);
        if (pad.pressed(gs::BTN_RIGHT)) want = std::min(1, lane_ + 1);
        float ax = std::fabs(pad.axisX) > 0.4f ? pad.axisX : 0;
        if (ax < -0.4f) want = std::max(-1, lane_ - 1);
        if (ax > 0.4f) want = std::min(1, lane_ + 1);
        bool fire = pad.down(gs::BTN_A) || pad.down(gs::BTN_C) || pad.down(gs::BTN_Z) || pad.accel > 0.45f;
        bool up = pad.down(gs::BTN_UP) || pad.down(gs::BTN_B);
        if (bell_ && up && !fire) pull = true;
        else if (fire) shoot();
    }
    if (!pull) lane_ = want;
    aim_ += (float(lane_) - aim_) * std::min(1.f, dt * 12.f);

    if (bot_ && !pull && best >= 0 && foes_[best].lane == lane_ && eta(foes_[best]) < 5.5f) shoot();

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
            f.z -= f.speed * dt;
            if (f.z <= 0.03f) {
                loseWatch("A RAIDER REACHED THE DOOR");
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
    float adv = 14.f * scale;
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
        shx_ = std::sin(watch_ * 38.f) * 3.f;
        shy_ = std::cos(watch_ * 29.f) * 1.4f;
    }
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.hudEnabled = true;
    v.roadTime = int(watch_ * 60.f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        gs::RoadLine& r = v.road[y];
        r.on = false;
        if (y < 96) {
            int dusk = 4 + y / 28;
            v.lineBackdrop[y] = gs::rgb4(dusk + 2, dusk, 6);
            v.lineFog[y] = 0;
        } else {
            float n = float(y - 96) / float(gs::SCREEN_H - 96);
            v.lineBackdrop[y] = gs::rgb4(5, 6, 2);
            v.lineFog[y] = uint8_t(std::clamp(int((1.f - n) * 6.f), 0, 8));
            r.on = true;
            r.cx = 168.f;
            r.hw = 18.f + n * 130.f;
            r.v = (96.f - float(y)) * 2.2f + watch_ * 30.f;
            r.pal = PAL_ROAD;
            r.band = (int(r.v / 40.f) & 1) ? 1 : 0;
            r.style = 0;
            r.left = r.right = gs::GROUND_LAND;
        }
    }

    float ax = 168.f + aim_ * 52.f;
    if (flash_ > 0) spr(art_.puff, ax + 18.f, 176.f, 14.f, PAL_FX, false, 0);
    spr(art_.sight, ax, 168.f, 14.f, PAL_FX, false, 0);
    spr(art_.keeper, ax, 198.f, 46.f, PAL_YOU, aim_ < 0, 0);
    spr(art_.fork, ax + 16.f, 188.f, 16.f, PAL_YOU, false, 0);

    float swing = std::sin((modeT_ + watch_) * (bell_ ? 8.f : 1.6f)) * (bell_ ? 6.f : 1.f);
    spr(art_.bell, 250.f + swing, 42.f, bell_ ? 28.f : 20.f, PAL_BELL, false, 0);
    spr(art_.rope, 250.f + swing * 0.25f, 68.f, 34.f, PAL_BELL, false, 0);

    if (mode_ == Mode::Title) text("S3 GRANARY RELIEF", 160, 16, 0.72f, PAL_HUD);
    else if (mode_ == Mode::Victory) text("RELIEF", 160, 16, 1.05f, PAL_OK);
    else if (mode_ == Mode::Over) text("WATCH OVER", 160, 16, 0.85f, PAL_ALERT);
    else if (bell_) text("THE BELL", 160, 16, 0.8f, PAL_BELL);

    if (mode_ == Mode::Title) {
        int fr = int(modeT_ * 5.f) & 1;
        spr(art_.thief[fr], 150, 150, 28, PAL_FOE, false, 4);
        spr(art_.runner[fr ^ 1], 196, 142, 20, PAL_FOE, true, 8);
        spr(art_.wagon[fr], 120, 156, 22, PAL_CART, false, 6);
    } else {
        std::vector<int> order;
        for (int i = 0; i < int(foes_.size()); i++)
            if (foes_[i].alive) order.push_back(i);
        std::sort(order.begin(), order.end(), [&](int a, int b) { return foes_[a].z > foes_[b].z; });
        for (int i : order) {
            const Foe& f = foes_[i];
            float x, y, h;
            place(f, x, y, h);
            int fog = int(std::clamp(f.z * 8.f, 0.f, 12.f));
            const gs::Mipped* img = &art_.thief[(f.age / 8) & 1];
            int pal = PAL_FOE;
            if (f.kind == RUNNER) img = &art_.runner[(f.age / 5) & 1];
            if (f.kind == WAGON) {
                img = &art_.wagon[(f.age / 8) & 1];
                pal = PAL_CART;
            }
            spr(*img, x, y, h, pal, f.lane > 0, fog);
        }
    }

    stamp(art_.barn, 48, 28, 200, 110, PAL_BARN);
    stamp(art_.silo, 8, 18, 40, 100, PAL_BARN);
    for (int i = 0; i < 3; i++) spr(art_.sack, 78.f + float(i) * 46.f, 128.f, 16.f, PAL_BARN, false, 0);

    char line[56];
    if (mode_ == Mode::Title) {
        hudC(23, "AT THE GRANARY.", PAL_HUD);
        hudC(24, "HOLD UNTIL THE RELIEF BELL.", PAL_OK);
        hudC(25, "MISS THAT AND THE WATCH IS OVER.", PAL_ALERT);
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
        else hudC(26, "HOLD THE GRANARY", PAL_HUD);
    }
    if (mode_ == Mode::Victory) sys_->setLight(80, 160, 40);
    else if (mode_ == Mode::Over) sys_->setLight(160, 30, 20);
    else if (bell_) sys_->setLight(180, 140, 40);
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

}  // namespace granary

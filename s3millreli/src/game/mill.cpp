#include "mill.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace mill {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float Z0 = 1.35f;
constexpr float BELL_AT = 24.f;
constexpr float BELL_END = 36.f;
constexpr float ROPE_NEED = 0.95f;
constexpr float STRIKE_Z = 0.78f;
constexpr float DECAY = 0.045f;

float speedOf(int kind) {
    if (kind == 1) return 0.40f;
    if (kind == 2) return 0.24f;
    return 0.28f;
}
int ptsOf(int kind) { return kind == 2 ? 200 : kind == 1 ? 140 : 100; }

struct Arr {
    float t;
    int kind;
    int lane;
};
const Arr kArr[] = {
    {1.1f, 0, 0},  {4.4f, 0, -1}, {7.6f, 1, 1},  {10.6f, 0, 0}, {13.4f, 2, -1},
    {16.6f, 1, 1}, {19.4f, 0, 0}, {22.2f, 0, -1}, {25.0f, 1, 1},
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
    x = 160.f + float(f.lane) * (22.f + near * 48.f);
    y = 118.f + near * 48.f;
    h = 10.f + near * (f.kind == WAIN ? 36.f : 30.f);
}

void Game::winWatch() {
    if (won_ || mode_ == Mode::Over) return;
    won_ = true;
    over_ = true;
    mode_ = Mode::Victory;
    modeT_ = 0;
    reason_ = "THE MILL HELD UNTIL THE RELIEF BELL";
    sys_->apu.keyOff(1);
    sys_->apu.noiseBurst(0.1f, 1600.f, 0.05f);
    fanStep_ = 0;
    fanT_ = 0;
    sys_->setLight(40, 170, 80);
}

void Game::loseWatch(const char* why) {
    if (won_ || mode_ == Mode::Over || mode_ == Mode::Victory) return;
    reason_ = why;
    won_ = false;
    over_ = true;
    mode_ = Mode::Over;
    modeT_ = 0;
    sys_->apu.keyOff(1);
    sys_->apu.noiseBurst(0.35f, 140.f, 0.28f);
    sys_->setLight(170, 30, 20);
    shake_ = 0.45f;
}

void Game::kill(Foe& f) {
    f.alive = false;
    score_ += f.points;
    sys_->apu.noiseBurst(0.14f, 700.f, 0.04f);
    sys_->apu.tone(0, 440.f, 0.04f);
}

void Game::openSluice() {
    if (sluiceCd_ > 0) return;
    sluiceCd_ = 4.2f;
    sluiceT_ = 0.45f;
    speed_ = std::min(1.f, speed_ + 0.42f);
    sys_->apu.noise(0.08f, 400.f, false);
    sys_->apu.tone(2, 180.f, 0.05f);
}

void Game::sweepHit() {
    for (Foe& f : foes_) {
        if (!f.alive || f.lane != bay_ || f.z > STRIKE_Z) continue;
        f.hp -= 1;
        if (f.hp <= 0) kill(f);
        else sys_->apu.tone(2, 220.f, 0.04f);
    }
}

void Game::beginWatch() {
    mode_ = Mode::Watch;
    over_ = false;
    won_ = false;
    bell_ = false;
    reason_ = "THE MILL WAS LOST";
    score_ = 0;
    bay_ = 0;
    aim_ = 0;
    spawnAt_ = 0;
    fanStep_ = -1;
    watch_ = 0;
    rope_ = 0;
    speed_ = 0.78f;
    pass_ = 0.25f;
    sluiceCd_ = 1.2f;
    sluiceT_ = 0;
    shake_ = 0;
    bellTick_ = 0;
    sail_ = 0;
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
    if (sys_) sys_->setLight(70, 50, 30);
}

void Game::update(float dt) {
    while (spawnAt_ < int(script_.size()) && script_[spawnAt_].t <= watch_) {
        const Spawn& s = script_[spawnAt_++];
        Foe f;
        f.kind = s.kind;
        f.lane = s.lane;
        f.z = Z0;
        f.speed = speedOf(s.kind);
        f.hp = 1;
        f.points = ptsOf(s.kind);
        f.alive = true;
        foes_.push_back(f);
    }

    bool wasBell = bell_;
    bell_ = watch_ >= BELL_AT && mode_ == Mode::Watch;
    if (bell_ && !wasBell) {
        bellTick_ = 0;
        sys_->apu.keyOn(1, 494.f, 0.2f);
        sys_->rumble(0.15f, 0.3f, 100);
    } else if (bell_ && !won_) {
        bellTick_ += dt;
        if (bellTick_ >= 1.05f) {
            bellTick_ = 0;
            sys_->apu.keyOn(1, 494.f, 0.16f);
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
    bool feed = false;
    int want = bay_;
    if (bot_) {
        bool threat = best >= 0 && mind < (bell_ ? 1.15f : 1.7f);
        if (bell_ && !threat && speed_ > 0.2f && watch_ < BELL_END) pull = true;
        else if (best >= 0) want = foes_[best].lane;
        if (!pull && speed_ < 0.5f && sluiceCd_ <= 0) feed = true;
    } else {
        const gs::Pad& pad = sys_->pad;
        if (pad.pressed(gs::BTN_LEFT)) want = std::max(-1, bay_ - 1);
        if (pad.pressed(gs::BTN_RIGHT)) want = std::min(1, bay_ + 1);
        float ax = std::fabs(pad.axisX) > 0.45f ? pad.axisX : 0;
        if (ax < -0.45f) want = std::max(-1, bay_ - 1);
        if (ax > 0.45f) want = std::min(1, bay_ + 1);
        bool up = pad.down(gs::BTN_UP) || pad.down(gs::BTN_A);
        bool down = pad.pressed(gs::BTN_DOWN) || pad.pressed(gs::BTN_B) || pad.brake > 0.5f;
        if (bell_ && up) pull = true;
        else if (down) feed = true;
    }
    if (!pull) bay_ = want;
    aim_ += (float(bay_) - aim_) * std::min(1.f, dt * 8.f);

    if (feed) openSluice();

    speed_ = std::max(0.f, speed_ - DECAY * dt);
    if (speed_ <= 0.02f) {
        loseWatch("THE STONES STOPPED");
        return;
    }
    sail_ += dt * (0.6f + speed_ * 2.4f);
    if (sluiceT_ > 0) sluiceT_ -= dt;
    if (sluiceCd_ > 0) sluiceCd_ -= dt;

    pass_ -= dt * std::max(0.25f, speed_);
    if (pass_ <= 0.f) {
        pass_ = 0.48f;
        sweepHit();
    }

    if (pull && bell_) {
        rope_ += dt;
        if (rope_ >= ROPE_NEED) winWatch();
    } else if (rope_ > 0) {
        rope_ = std::max(0.f, rope_ - dt * 0.7f);
    }

    if (won_) return;

    for (Foe& f : foes_) {
        if (!f.alive) continue;
        f.age++;
        f.z -= f.speed * dt;
        if (f.z <= 0.04f) {
            loseWatch("THEY HAVE THE MILL");
            return;
        }
    }
    if (bell_ && watch_ >= BELL_END && !won_) {
        loseWatch("THE RELIEF PASSED THE MILL");
        return;
    }
    watch_ += dt;
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
        shx_ = std::sin(watch_ * 38.f) * 3.f;
        shy_ = std::cos(watch_ * 29.f) * 1.4f;
    }
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.hudEnabled = true;
    v.roadTime = int(watch_ * 30.f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int sky = y < 96 ? 4 + (96 - y) / 28 : 3;
        v.lineBackdrop[y] = gs::rgb4(sky + 2, sky + 1, sky + 3);
        v.lineFog[y] = uint8_t(y < 70 ? (70 - y) / 12 : 0);
        gs::RoadLine& r = v.road[y];
        r.on = y >= 108;
        if (!r.on) continue;
        float t = float(y - 108) / float(gs::SCREEN_H - 108);
        r.cx = 160.f + aim_ * 6.f * t;
        r.hw = 18.f + t * 78.f;
        r.v = watch_ * (1.2f + speed_ * 4.f) + t * 40.f;
        r.pal = 12;
        r.band = (y / 8) & 1;
        r.style = 0;
        r.left = 0;
        r.right = y > 160 ? 1 : 0;
    }
    if (v.color(12 * 16 + 1) == 0) {
        v.setColor(12 * 16 + 1, gs::rgb4(8, 6, 3));
        v.setColor(12 * 16 + 2, gs::rgb4(6, 5, 2));
        v.setColor(12 * 16 + 3, gs::rgb4(4, 7, 3));
        v.setColor(12 * 16 + 4, gs::rgb4(3, 6, 8));
        v.setColor(12 * 16 + 5, gs::rgb4(10, 9, 6));
    }

    const gs::Mipped& sails = (int(sail_) & 1) ? art_.sailCross : art_.sailPlus;
    spr(sails, 160.f, 78.f, 86.f, PAL_SAIL, false, 1);
    stamp(art_.mill, 112, 48, 96, 120, PAL_MILL);

    float swing = std::sin((modeT_ + watch_) * (bell_ ? 8.f : 1.6f)) * (bell_ ? 4.f : 1.f);
    spr(art_.bell, 196.f + swing, 58.f, bell_ ? 22.f : 16.f, PAL_BELL, false, 0);
    float ropeH = 28.f + rope_ * 18.f;
    spr(art_.rope, 196.f, 78.f, ropeH, PAL_BELL, false, 0);

    if (mode_ == Mode::Title) text("S3 MILL RELIEF", 160, 16, 0.8f, PAL_HUD);
    else if (mode_ == Mode::Victory) text("RELIEF", 160, 16, 1.05f, PAL_OK);
    else if (mode_ == Mode::Over) text("MILL LOST", 160, 16, 0.9f, PAL_ALERT);
    else if (bell_) text("THE BELL", 160, 16, 0.8f, PAL_BELL);

    if (mode_ == Mode::Title) {
        int fr = int(modeT_ * 5.f) & 1;
        spr(art_.reaper[fr], 168, 150, 26, PAL_FOE, false, 2);
        spr(art_.wain[fr], 118, 156, 22, PAL_WAIN, false, 4);
        spr(art_.miller, 160, 132, 28, PAL_YOU, false, 0);
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
            if (f.kind == WAIN) {
                spr(art_.wain[(f.age / 8) & 1], x, y, h, PAL_WAIN, f.lane < 0, fog);
            } else if (f.kind == RUNNER) {
                spr(art_.runner[(f.age / 5) & 1], x, y, h, PAL_FOE, f.lane < 0, fog);
            } else {
                spr(art_.reaper[(f.age / 8) & 1], x, y, h, PAL_FOE, f.lane < 0, fog);
            }
        }
        float px = 160.f + aim_ * 36.f;
        bool flip = aim_ < -0.05f;
        spr(art_.sweep, px + (flip ? -8.f : 8.f), 168.f, 10.f, PAL_FX, flip, 0);
        spr(art_.miller, px, 176.f, 32.f, PAL_YOU, flip, 0);
        spr(art_.sack, px - 14.f, 182.f, 14.f, PAL_YOU, false, 0);
        if (sluiceT_ > 0) spr(art_.sluice, 78.f, 150.f, 26.f, PAL_WATER, false, 0);
    }

    char line[48];
    if (mode_ == Mode::Title) {
        hudC(24, "YOU HAVE THE MILL.", PAL_HUD);
        hudC(25, "HOLD UNTIL THE RELIEF BELL.", PAL_OK);
        hudC(26, "LEFT RIGHT SWEEP   DOWN SLUICE   UP ROPE", PAL_HUD);
        hudC(27, "ENTER", PAL_HUD);
        int n = int(std::strlen(S3_VERSION_STRING));
        hud(39 - n, 0, S3_VERSION_STRING, PAL_HUD);
    } else {
        int left = std::max(0, int(std::ceil(BELL_AT - watch_)));
        if (!bell_) std::snprintf(line, sizeof line, "BELL %02d", left);
        else std::snprintf(line, sizeof line, "ROPE %d", int(std::clamp(rope_ / ROPE_NEED, 0.f, 1.f) * 10.f));
        hud(1, 0, line, bell_ ? PAL_OK : PAL_HUD);
        std::snprintf(line, sizeof line, "STONES %d", int(std::clamp(speed_, 0.f, 1.f) * 10.f));
        hud(28, 0, line, speed_ < 0.35f ? PAL_ALERT : PAL_HUD);
        std::snprintf(line, sizeof line, "SCORE %d", score_);
        hud(1, 27, line, PAL_HUD);
        if (mode_ == Mode::Pause) hudC(26, "PAUSED", PAL_HUD);
        else if (mode_ == Mode::Victory) hudC(26, "THE MILL HELD", PAL_OK);
        else if (mode_ == Mode::Over) hudC(26, reason_, PAL_ALERT);
        else if (bell_) hudC(26, "HAUL THE BELL ROPE", PAL_BELL);
        else hudC(26, "KEEP THE STONES TURNING", PAL_HUD);
    }
    if (mode_ == Mode::Victory) sys_->setLight(40, 160, 70);
    else if (mode_ == Mode::Over) sys_->setLight(160, 24, 20);
    else if (bell_) sys_->setLight(150, 120, 40);
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

}  // namespace mill

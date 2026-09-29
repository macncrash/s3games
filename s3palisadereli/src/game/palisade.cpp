#include "palisade.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace palisade {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr int POSTS = 5;
constexpr float BELL_AT = 16.f;
constexpr float BELL_END = 26.f;
constexpr float ROPE_NEED = 0.8f;
constexpr float WALK = 6.f;

float speedOf(int kind) {
    if (kind == 1) return 0.22f;
    if (kind == 2) return 0.11f;
    return 0.14f;
}
int hpOf(int kind) { return kind == 2 ? 2 : 1; }
int ptsOf(int kind) { return kind == 2 ? 250 : kind == 1 ? 150 : 100; }

struct Arr {
    float t;
    int post;
    int kind;
};
const Arr kArr[] = {
    {1.1f, 2, 0}, {2.8f, 4, 0}, {4.6f, 1, 1}, {6.6f, 3, 0},  {8.6f, 0, 2},
    {10.8f, 4, 1}, {13.0f, 2, 0}, {15.6f, 3, 0}, {18.2f, 1, 1}, {20.8f, 4, 0},
};

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Victory || mode_ == Mode::Over) return 3;
    if (bell_) return 2;
    return 1;
}

float Game::postX(float post) const { return 52.f + post * 54.f; }

void Game::winWatch() {
    if (won_ || mode_ == Mode::Over) return;
    won_ = true;
    over_ = true;
    mode_ = Mode::Victory;
    modeT_ = 0;
    reason_ = "THE WATCH HELD UNTIL THE RELIEF BELL";
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
    sys_->apu.noiseBurst(0.4f, 160.f, 0.28f);
    sys_->setLight(170, 24, 28);
    shake_ = 0.45f;
}

void Game::fell(Foe& f) {
    f.alive = false;
    score_ += f.points;
    sys_->apu.noiseBurst(0.14f, 700.f, 0.05f);
    sys_->apu.tone(0, 480.f, 0.05f);
}

void Game::thrust() {
    if (cool_ > 0) return;
    cool_ = 0.26f;
    flash_ = 5;
    sys_->apu.noiseBurst(0.18f, 1100.f, 0.04f);
    int best = -1;
    float bestC = -1.f;
    for (int i = 0; i < int(foes_.size()); i++) {
        Foe& f = foes_[i];
        if (!f.alive) continue;
        if (std::fabs(px_ - float(f.post)) > 0.42f) continue;
        if (f.climb > bestC) {
            bestC = f.climb;
            best = i;
        }
    }
    if (best < 0 || bestC < 0.08f) return;
    Foe& f = foes_[best];
    f.hp -= 1;
    f.climb = std::max(0.f, f.climb - 0.28f);
    if (f.hp <= 0) fell(f);
    else sys_->apu.tone(2, 220.f, 0.05f);
}

void Game::beginWatch() {
    mode_ = Mode::Watch;
    over_ = false;
    won_ = false;
    bell_ = false;
    reason_ = "THE WALL FAILED";
    score_ = 0;
    px_ = 2.f;
    want_ = 2;
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
        s.post = a.post;
        s.kind = a.kind;
        script_.push_back(s);
    }
    if (sys_) sys_->setLight(30, 40, 24);
}

void Game::update(float dt) {
    while (spawnAt_ < int(script_.size()) && script_[spawnAt_].t <= watch_) {
        const Spawn& s = script_[spawnAt_++];
        Foe f;
        f.kind = s.kind;
        f.post = s.post;
        f.climb = 0;
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
        sys_->apu.keyOn(1, 494.f, 0.22f);
        sys_->rumble(0.25f, 0.4f, 140);
    } else if (bell_ && !won_) {
        bellTick_ += dt;
        if (bellTick_ >= 0.85f) {
            bellTick_ = 0;
            sys_->apu.keyOn(1, 494.f, 0.18f);
        }
    }

    int threat = -1;
    float high = -1.f;
    for (int i = 0; i < int(foes_.size()); i++) {
        if (!foes_[i].alive) continue;
        if (foes_[i].climb > high) {
            high = foes_[i].climb;
            threat = i;
        }
    }

    bool pull = false;
    int want = want_;
    if (bot_) {
        float travel = px_ / WALK;
        bool closing = bell_ && (BELL_END - watch_) < travel + ROPE_NEED + 0.55f;
        bool clear = high < 0.55f;
        if (bell_ && closing && clear && watch_ < BELL_END) {
            want = 0;
            if (px_ < 0.38f) pull = true;
        } else if (threat >= 0) {
            want = foes_[threat].post;
        }
    } else {
        const gs::Pad& pad = sys_->pad;
        if (std::fabs(px_ - float(want_)) < 0.08f) {
            if (pad.down(gs::BTN_LEFT) || pad.axisX < -0.45f) want = std::max(0, want_ - 1);
            if (pad.down(gs::BTN_RIGHT) || pad.axisX > 0.45f) want = std::min(POSTS - 1, want_ + 1);
        }
        bool fire = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_Z) || pad.accel > 0.55f;
        bool up = pad.down(gs::BTN_UP) || pad.down(gs::BTN_B);
        if (bell_ && up && px_ < 0.45f && !fire) pull = true;
        else if (fire) thrust();
    }
    want_ = want;
    float dir = float(want_) - px_;
    float step = WALK * dt;
    if (std::fabs(dir) <= step) px_ = float(want_);
    else px_ += (dir > 0 ? step : -step);

    if (bot_ && !pull && threat >= 0 && std::fabs(px_ - float(foes_[threat].post)) < 0.4f &&
        foes_[threat].climb > 0.12f)
        thrust();

    if (pull && px_ < 0.45f) {
        rope_ += dt;
        if (rope_ >= ROPE_NEED && bell_) winWatch();
    } else if (rope_ > 0) {
        rope_ = std::max(0.f, rope_ - dt * 0.7f);
    }

    for (Foe& f : foes_) {
        if (!f.alive) continue;
        f.age++;
        f.climb += f.speed * dt;
        if (f.climb >= 1.f) {
            loseWatch("THE WALL IS BREACHED");
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
        shx_ = std::sin(watch_ * 42.f) * 3.f;
        shy_ = std::cos(watch_ * 31.f) * 1.4f;
    }
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.hudEnabled = true;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int sky = y < 90 ? 3 : y < 140 ? 2 : 1;
        v.lineBackdrop[y] = gs::rgb4(sky + 1, sky + 2, sky + 4);
        v.road[y].on = false;
        v.lineFog[y] = 0;
    }

    float swing = std::sin((modeT_ + watch_) * (bell_ ? 8.f : 1.6f)) * (bell_ ? 6.f : 1.f);
    if (flash_ > 0) spr(art_.spark, postX(px_) + 6.f, 118.f, 14.f, PAL_FX);

    float py = 86.f;
    int step = int(watch_ * 8.f) & 1;
    spr(art_.you[std::fabs(float(want_) - px_) > 0.05f ? step : 0], postX(px_), py, 40.f, PAL_YOU);
    spr(art_.spear, postX(px_) + (flash_ ? 10.f : 6.f), py + 8.f, flash_ ? 58.f : 50.f, PAL_YOU);
    spr(art_.bell, 28.f + swing, 46.f, bell_ ? 28.f : 22.f, PAL_BELL);
    float ropeH = 34.f + rope_ * 18.f;
    spr(art_.rope, 28.f + swing * 0.25f, 70.f + rope_ * 6.f, ropeH, PAL_BELL);

    if (mode_ == Mode::Title) {
        spr(art_.climber[step], postX(2), 150.f, 32.f, PAL_FOE, false, 2);
        spr(art_.runner[step ^ 1], postX(3.2f), 162.f, 28.f, PAL_FOE, false, 4);
        spr(art_.shield[step], postX(1), 168.f, 34.f, PAL_SHIELD, false, 3);
    } else {
        std::vector<int> order;
        for (int i = 0; i < int(foes_.size()); i++)
            if (foes_[i].alive) order.push_back(i);
        std::sort(order.begin(), order.end(), [&](int a, int b) { return foes_[a].climb < foes_[b].climb; });
        for (int i : order) {
            const Foe& f = foes_[i];
            float y = 196.f - f.climb * 118.f;
            float h = 22.f + f.climb * 16.f;
            const gs::Mipped* img = &art_.climber[(f.age / 8) & 1];
            int pal = PAL_FOE;
            if (f.kind == RUNNER) img = &art_.runner[(f.age / 5) & 1];
            if (f.kind == SHIELD) {
                img = &art_.shield[(f.age / 8) & 1];
                pal = PAL_SHIELD;
            }
            spr(*img, postX(float(f.post)) + 2.f, y, h, pal, false, int((1.f - f.climb) * 6.f));
        }
    }

    for (int i = 0; i < POSTS; i++) spr(art_.stake, postX(float(i)), 150.f, 108.f, PAL_WOOD);
    spr(art_.gate, 28.f, 132.f, 100.f, PAL_WOOD);
    for (int i = 0; i < 6; i++) spr(art_.hill, 32.f + float(i) * 52.f, 206.f, 26.f, PAL_FIELD);

    if (mode_ == Mode::Title) text("PALISADE", 176, 28, 1.05f, PAL_HUD);
    else if (mode_ == Mode::Victory) text("RELIEF", 176, 22, 1.15f, PAL_OK);
    else if (mode_ == Mode::Over) text("WALL LOST", 176, 22, 0.95f, PAL_ALERT);
    else if (bell_) text("THE BELL", 176, 22, 0.9f, PAL_BELL);

    char line[64];
    if (mode_ == Mode::Title) {
        hudC(23, "ONE PALISADE.", PAL_HUD);
        hudC(24, "HOLD UNTIL THE RELIEF BELL.", PAL_OK);
        hudC(25, "LEFT RIGHT THE WALK    A THE SPEAR", PAL_HUD);
        hudC(26, "AT THE GATE, UP ANSWERS THE BELL", PAL_HUD);
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
        else if (bell_) hudC(26, "ANSWER AT THE GATE", PAL_BELL);
        else hudC(26, "HOLD THE STAKES", PAL_HUD);
    }
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

}  // namespace palisade

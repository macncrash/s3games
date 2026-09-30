#include "beacon.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace beacon {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float Z0 = 1.25f;
constexpr float BELL_AT = 24.f;
constexpr float BELL_END = 36.f;
constexpr float HAUL_NEED = 0.9f;
constexpr float DRAIN = 2.45f;
constexpr float PUMP = 16.f;

float speedOf(int kind) { return kind == 1 ? 0.26f : 0.36f; }
int hpOf(int kind) { return kind == 1 ? 2 : 1; }
int ptsOf(int kind) { return kind == 1 ? 180 : 100; }

struct Arr {
    float t;
    int kind;
    int face;
};
const Arr kArr[] = {
    {1.1f, 0, -1}, {4.8f, 0, 1},  {8.6f, 1, 0},  {13.4f, 0, -1}, {17.0f, 0, 1},
    {20.6f, 1, -1}, {25.0f, 0, 1}, {28.6f, 0, 0},
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
    float lane = f.face == 0 ? 0.f : float(f.face);
    x = 168.f + lane * (36.f + near * 78.f);
    y = 86.f + near * 96.f;
    h = 12.f + near * (f.kind == 1 ? 40.f : 34.f);
}

void Game::winWatch() {
    if (won_ || mode_ == Mode::Over) return;
    won_ = true;
    over_ = true;
    mode_ = Mode::Victory;
    modeT_ = 0;
    reason_ = "THE BEACON HELD UNTIL THE RELIEF BELL";
    sys_->apu.keyOff(1);
    sys_->apu.noiseBurst(0.1f, 1400.f, 0.05f);
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
    sys_->apu.noiseBurst(0.4f, 140.f, 0.28f);
    sys_->setLight(160, 20, 20);
    shake_ = 0.45f;
}

void Game::fell(Foe& f) {
    f.alive = false;
    score_ += f.points;
    sys_->apu.noiseBurst(0.12f, 640.f, 0.05f);
    sys_->apu.tone(0, 520.f, 0.05f);
}

void Game::flare() {
    if (cool_ > 0) return;
    cool_ = 0.2f;
    flash_ = 5;
    sys_->apu.noiseBurst(0.16f, 1100.f, 0.04f);
    int best = -1;
    float bestZ = 1.0e9f;
    for (int i = 0; i < int(foes_.size()); i++) {
        if (!foes_[i].alive || foes_[i].face != face_) continue;
        if (foes_[i].z < bestZ) {
            bestZ = foes_[i].z;
            best = i;
        }
    }
    if (best < 0 || bestZ > 1.15f) return;
    Foe& f = foes_[best];
    f.hp -= 1;
    if (f.hp <= 0) fell(f);
    else sys_->apu.tone(2, 180.f, 0.05f);
}

void Game::beginWatch() {
    mode_ = Mode::Watch;
    over_ = false;
    won_ = false;
    bell_ = false;
    reason_ = "THE BEACON FAILED";
    score_ = 0;
    face_ = 0;
    lean_ = 0;
    oil_ = 72.f;
    spawnAt_ = 0;
    flash_ = 0;
    fanStep_ = -1;
    watch_ = 0;
    haul_ = 0;
    cool_ = 0;
    shake_ = 0;
    bellTick_ = 0;
    sweep_ = 0;
    foes_.clear();
    script_.clear();
    for (const Arr& a : kArr) {
        Spawn s;
        s.t = a.t;
        s.kind = a.kind;
        s.face = a.face;
        script_.push_back(s);
    }
    if (sys_) sys_->setLight(20, 28, 60);
}

void Game::update(float dt) {
    while (spawnAt_ < int(script_.size()) && script_[spawnAt_].t <= watch_) {
        const Spawn& s = script_[spawnAt_++];
        Foe f;
        f.kind = s.kind;
        f.face = s.face;
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
        sys_->apu.keyOn(1, 392.f, 0.2f);
        sys_->rumble(0.2f, 0.35f, 120);
    } else if (bell_ && !won_) {
        bellTick_ += dt;
        if (bellTick_ >= 0.9f) {
            bellTick_ = 0;
            sys_->apu.keyOn(1, 392.f, 0.16f);
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

    bool pump = false;
    bool pull = false;
    int want = face_;
    bool urgent = best >= 0 && mind < 1.35f;
    if (bot_) {
        bool closing = bell_ && (BELL_END - watch_) < HAUL_NEED + 2.2f;
        if (bell_ && oil_ > 28.f && !urgent && (closing || mind > 2.2f)) {
            pull = true;
            want = 0;
        } else if (oil_ < 64.f && !urgent) {
            pump = true;
            want = 0;
        } else if (best >= 0) {
            want = foes_[best].face;
        }
    } else {
        const gs::Pad& pad = sys_->pad;
        if (pad.pressed(gs::BTN_LEFT)) want = std::max(-1, face_ - 1);
        if (pad.pressed(gs::BTN_RIGHT)) want = std::min(1, face_ + 1);
        float ax = std::fabs(pad.axisX) > 0.4f ? pad.axisX : 0;
        if (ax < -0.4f && cool_ < 0.1f) want = std::max(-1, face_ - 1);
        if (ax > 0.4f && cool_ < 0.1f) want = std::min(1, face_ + 1);
        bool fire = pad.down(gs::BTN_A) || pad.down(gs::BTN_C) || pad.down(gs::BTN_Z) || pad.accel > 0.45f;
        bool up = pad.down(gs::BTN_UP) || pad.down(gs::BTN_Y);
        bool feed = pad.down(gs::BTN_B) || pad.down(gs::BTN_DOWN) || pad.brake > 0.45f;
        if (bell_ && up && !fire) {
            pull = true;
            want = 0;
        } else {
            if (feed) pump = true;
            if (fire) flare();
        }
    }
    if (!pull) face_ = want;
    else face_ = 0;
    lean_ += (float(face_) - lean_) * std::min(1.f, dt * 10.f);

    if (bot_ && !pull && !pump && best >= 0 && foes_[best].face == face_ && eta(foes_[best]) < 4.2f) flare();

    if (pump && face_ == 0) {
        oil_ = std::min(100.f, oil_ + PUMP * dt);
        if (int(watch_ * 8.f) % 4 == 0) sys_->apu.tone(2, 90.f, 0.02f);
    }
    if (pull) {
        haul_ += dt;
        if (haul_ >= HAUL_NEED && bell_ && oil_ > 0.f) winWatch();
    } else if (haul_ > 0) {
        haul_ = std::max(0.f, haul_ - dt * 0.6f);
    }

    oil_ -= DRAIN * dt;
    if (oil_ <= 0.f && !won_) {
        oil_ = 0;
        loseWatch("THE BEACON WENT DARK");
        return;
    }

    for (Foe& f : foes_) {
        if (!f.alive) continue;
        f.age++;
        f.z -= f.speed * dt;
        if (f.z <= 0.02f) {
            loseWatch("THE BEACON WAS TAKEN");
            return;
        }
    }
    if (bell_ && watch_ >= BELL_END && !won_) loseWatch("THE RELIEF BELL WENT UNANSWERED");
    watch_ += dt;
    sweep_ += dt * (2.2f + oil_ * 0.01f);
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
        int sea = y > 150 ? (y - 150) / 10 : 0;
        int skyR = 1 + (y < 90 ? 0 : (y - 90) / 50);
        int skyB = 6 - y / 50;
        v.lineBackdrop[y] = gs::rgb4(skyR, 2 + sea, std::clamp(skyB + sea, 1, 8));
        v.lineFog[y] = uint8_t(y > 190 ? (y - 190) / 2 : 0);
        v.road[y].on = false;
    }

    if (mode_ == Mode::Victory) text("RELIEF", 160, 18, 1.05f, PAL_OK);
    else if (mode_ == Mode::Over) text("BEACON LOST", 160, 18, 0.8f, PAL_ALERT);
    else if (bell_ && mode_ != Mode::Title) text("THE BELL", 160, 16, 0.78f, PAL_BELL);
    else if (mode_ == Mode::Title) text("S3 BEACON RELIEF", 160, 16, 0.66f, PAL_HUD);

    float kx = 168.f + lean_ * 16.f;
    spr(art_.keeper, kx, 78.f, 36.f, PAL_KEEP, lean_ < -0.2f, 0);
    float flameH = 10.f + std::clamp(oil_, 0.f, 100.f) * 0.12f;
    if (oil_ > 1.f || mode_ == Mode::Title) {
        float bob = std::sin(sweep_ * 9.f) * 1.4f;
        spr(art_.flame, 160.f, 58.f + bob, flameH, PAL_FX, false, 0);
        float bx = 160.f + std::sin(sweep_) * 70.f;
        spr(art_.beam, bx, 64.f, 6.f, PAL_FX, std::sin(sweep_) < 0, 6);
    }
    spr(art_.glass, 160.f, 58.f, 22.f, PAL_LAMP, false, 0);
    if (flash_ > 0) {
        float fx = 168.f + float(face_) * 36.f;
        spr(art_.flare, fx, 96.f, 16.f, PAL_FX, false, 0);
    }

    float swing = std::sin((modeT_ + watch_) * (bell_ ? 9.f : 1.4f)) * (bell_ ? 7.f : 1.2f);
    spr(art_.bell, 228.f + swing, 58.f, bell_ ? 18.f : 14.f, PAL_BELL, false, 0);

    if (mode_ == Mode::Title) {
        int fr = int(modeT_ * 6.f) & 1;
        spr(art_.wreck[fr], 78, 168, 28, PAL_WRECK, false, 3);
        spr(art_.douse[fr ^ 1], 250, 176, 30, PAL_DOUSE, true, 2);
    } else {
        std::vector<int> order;
        for (int i = 0; i < int(foes_.size()); i++)
            if (foes_[i].alive) order.push_back(i);
        std::sort(order.begin(), order.end(), [&](int a, int b) { return foes_[a].z < foes_[b].z; });
        for (int i : order) {
            const Foe& f = foes_[i];
            float x, y, h;
            place(f, x, y, h);
            int fog = int(std::clamp(f.z * 5.f, 0.f, 8.f));
            const gs::Mipped* img = &art_.wreck[(f.age / 8) & 1];
            int pal = PAL_WRECK;
            if (f.kind == 1) {
                img = &art_.douse[(f.age / 7) & 1];
                pal = PAL_DOUSE;
            }
            spr(*img, x, y, h, pal, f.face > 0, fog);
        }
    }

    stamp(art_.house, 124, 36, 72, 108, PAL_LAMP);
    stamp(art_.post, 222, 62, 8, 52, PAL_ROCK);
    stamp(art_.rock, 20, 176, 70, 36, PAL_ROCK);
    stamp(art_.rock, 118, 188, 90, 30, PAL_ROCK);
    stamp(art_.rock, 230, 180, 76, 34, PAL_ROCK);

    char line[48];
    if (mode_ == Mode::Title) {
        hudC(23, "YOU HAVE THE BEACON.", PAL_HUD);
        hudC(24, "HOLD UNTIL THE RELIEF BELL.", PAL_OK);
        hudC(25, "LEFT RIGHT FACE   A FLARE", PAL_HUD);
        hudC(26, "B PUMP THE LAMP   UP THE BELL", PAL_HUD);
        hudC(27, "ENTER", PAL_HUD);
        int n = int(std::strlen(S3_VERSION_STRING));
        hud(39 - n, 0, S3_VERSION_STRING, PAL_HUD);
    } else {
        int left = std::max(0, int(std::ceil(BELL_AT - watch_)));
        if (!bell_) std::snprintf(line, sizeof line, "BELL %02d", left);
        else std::snprintf(line, sizeof line, "BELL %d", int(std::clamp(haul_ / HAUL_NEED, 0.f, 1.f) * 10.f));
        hud(1, 0, line, bell_ ? PAL_BELL : PAL_HUD);
        std::snprintf(line, sizeof line, "OIL %02d", int(std::clamp(oil_, 0.f, 99.f)));
        hud(32, 0, line, oil_ < 30.f ? PAL_ALERT : PAL_OK);
        std::snprintf(line, sizeof line, "SCORE %d", score_);
        hud(1, 27, line, PAL_HUD);
        const char* face = face_ < 0 ? "WEST PATH" : face_ > 0 ? "EAST JETTY" : "THE LAMP";
        hud(28, 27, face, PAL_HUD);
        if (mode_ == Mode::Pause) hudC(26, "PAUSED", PAL_HUD);
        else if (mode_ == Mode::Victory) hudC(26, "THE BEACON HELD", PAL_OK);
        else if (mode_ == Mode::Over) hudC(26, reason_, PAL_ALERT);
        else if (bell_) hudC(26, "ANSWER THE BELL", PAL_BELL);
        else hudC(26, "HOLD THE BEACON", PAL_HUD);
    }
    if (mode_ == Mode::Victory) sys_->setLight(40, 170, 80);
    else if (mode_ == Mode::Over) sys_->setLight(160, 24, 24);
    else if (bell_) sys_->setLight(170, 120, 30);
    else sys_->setLight(int(20 + oil_ * 0.4f), int(20 + oil_ * 0.15f), 40);
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
            static const float notes[] = {349.f, 440.f, 523.f, 698.f};
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

}  // namespace beacon

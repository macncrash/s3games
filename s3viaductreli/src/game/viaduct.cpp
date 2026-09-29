#include "viaduct.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace viaduct {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr int ARCHES = 5;
constexpr float BELL_AT = 18.f;
constexpr float RELIEF_AT = 26.f;
constexpr float WALK = 150.f;
constexpr float REACH = 26.f;
constexpr float CLIMB_TIME = 4.4f;
constexpr float FUSE_TIME = 3.8f;
constexpr float SNUFF_NEED = 0.32f;
constexpr float DECK_Y = 78.f;

struct Arr {
    float t;
    int kind;
    int arch;
};

const Arr kArr[] = {
    {1.1f, 0, 2}, {4.6f, 1, 0}, {8.6f, 0, 4}, {12.6f, 1, 1},
    {16.2f, 0, 3}, {19.8f, 0, 0}, {23.2f, 1, 4},
};

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Victory || mode_ == Mode::Over) return 3;
    if (bell_) return 2;
    return 1;
}

float Game::archX(int i) const { return 36.f + float(i) * 62.f; }

int Game::urgent() const {
    int best = -1;
    float left = 1.0e9f;
    for (int i = 0; i < int(threats_.size()); i++) {
        if (!threats_[i].alive) continue;
        if (threats_[i].life < left) {
            left = threats_[i].life;
            best = i;
        }
    }
    return best;
}

void Game::clearThreat(Threat& t, int pts) {
    t.alive = false;
    score_ += pts;
    sys_->apu.noiseBurst(0.12f, 900.f, 0.04f);
    sys_->apu.tone(0, t.kind == FUSE ? 360.f : 520.f, 0.04f);
}

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
    sys_->apu.noiseBurst(0.4f, 140.f, 0.3f);
    sys_->setLight(180, 20, 30);
    shake_ = 0.45f;
}

void Game::beginWatch() {
    mode_ = Mode::Watch;
    over_ = false;
    won_ = false;
    bell_ = false;
    reason_ = "THE SPAN WAS LOST";
    score_ = 0;
    px_ = archX(2);
    face_ = 1;
    spawnAt_ = 0;
    flash_ = 0;
    fanStep_ = -1;
    watch_ = 0;
    cool_ = 0;
    shake_ = 0;
    bellTick_ = 0;
    modeT_ = 0;
    threats_.clear();
    script_.clear();
    for (const Arr& a : kArr) {
        Spawn s;
        s.t = a.t;
        s.kind = a.kind;
        s.arch = a.arch;
        script_.push_back(s);
    }
    if (sys_) sys_->setLight(40, 30, 50);
}

void Game::update(float dt) {
    watch_ += dt;
    if (cool_ > 0) cool_ -= dt;
    if (flash_ > 0) flash_--;
    if (shake_ > 0) shake_ -= dt;

    while (spawnAt_ < int(script_.size()) && script_[spawnAt_].t <= watch_) {
        const Spawn& s = script_[spawnAt_++];
        Threat t;
        t.kind = s.kind;
        t.arch = s.arch;
        t.life = s.kind == CLIMB ? CLIMB_TIME : FUSE_TIME;
        t.alive = true;
        threats_.push_back(t);
    }

    bool wasBell = bell_;
    bell_ = watch_ >= BELL_AT && mode_ == Mode::Watch;
    if (bell_ && !wasBell) {
        bellTick_ = 0;
        sys_->apu.keyOn(1, 392.f, 0.22f);
        sys_->rumble(0.2f, 0.45f, 160);
    } else if (bell_ && !won_) {
        bellTick_ += dt;
        if (bellTick_ >= 0.9f) {
            bellTick_ = 0;
            sys_->apu.keyOn(1, 392.f, 0.18f);
        }
    }

    int focus = urgent();
    const gs::Pad& pad = sys_->pad;
    float aim = px_;
    bool fire = false;
    bool hold = false;
    if (bot_) {
        if (focus >= 0) aim = archX(threats_[focus].arch);
        float d = std::fabs(px_ - aim);
        if (focus >= 0 && d <= REACH) {
            if (threats_[focus].kind == CLIMB) fire = true;
            else hold = true;
        }
    } else {
        float ax = pad.axisX;
        if (pad.down(gs::BTN_LEFT) || ax < -0.3f) aim = px_ - 1.f;
        else if (pad.down(gs::BTN_RIGHT) || ax > 0.3f) aim = px_ + 1.f;
        else aim = px_;
        fire = pad.down(gs::BTN_A) || pad.down(gs::BTN_B) || pad.down(gs::BTN_C) || pad.accel > 0.4f;
        hold = fire;
    }

    if (bot_) {
        float d = aim - px_;
        float step = WALK * dt;
        if (std::fabs(d) <= step) px_ = aim;
        else px_ += (d > 0 ? step : -step);
        if (std::fabs(d) > 1.f) face_ = d > 0 ? 1.f : -1.f;
    } else if (aim != px_) {
        float dir = aim > px_ ? 1.f : -1.f;
        px_ = std::clamp(px_ + dir * WALK * dt, 16.f, 304.f);
        face_ = dir;
    }
    px_ = std::clamp(px_, 16.f, 304.f);

    for (Threat& t : threats_) {
        if (!t.alive) continue;
        t.age++;
        float near = std::fabs(px_ - archX(t.arch)) <= REACH;
        if (t.kind == CLIMB) {
            bool can = fire && near && cool_ <= 0 && t.life < CLIMB_TIME - 0.2f;
            if (can) {
                cool_ = 0.28f;
                flash_ = 4;
                sys_->apu.noiseBurst(0.16f, 1400.f, 0.03f);
                clearThreat(t, 120);
                continue;
            }
        } else if (hold && near) {
            t.snuff += dt;
            if (t.snuff >= SNUFF_NEED) {
                clearThreat(t, 160);
                continue;
            }
        } else {
            t.snuff = 0;
        }
        t.life -= dt;
        if (t.life <= 0) {
            loseWatch(t.kind == CLIMB ? "A CLIMBER TOOK THE DECK" : "A CHARGE BROKE THE ARCH");
            return;
        }
    }

    if (watch_ >= RELIEF_AT) winWatch();
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog) {
    if (h < 1.f || m.h < 1) return;
    float w = h * (float(m.w) / float(m.h));
    gs::Sprite s;
    s.x = int16_t(std::lround(cx - w * 0.5f));
    s.y = int16_t(std::lround(cy));
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
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

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.road[y].on = false;
        float u = float(y) / float(gs::SCREEN_H);
        if (y < 96) {
            v.lineBackdrop[y] = gs::rgb4(3 + int(u * 8), 3 + int(u * 4), 8 - int(u * 4));
            v.lineFog[y] = 2;
        } else if (y < 150) {
            v.lineBackdrop[y] = gs::rgb4(2, 3, 4);
            v.lineFog[y] = 4;
        } else {
            int w = 1 + int((y & 7) == 0);
            v.lineBackdrop[y] = gs::rgb4(1, 2 + w, 4 + ((y / 3) & 1));
            v.lineFog[y] = 6;
        }
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.hudEnabled = true;
    sky();

    float ox = 0;
    if (shake_ > 0) ox = std::sin(watch_ * 40.f) * 3.f;

    for (int i = 0; i < ARCHES; i++)
        spr(art_.arch, archX(i) + ox, 70.f, 140.f, PAL_STONE, false, 0);

    for (int i = 0; i < ARCHES; i++)
        spr(art_.lamp, archX(i) + ox, 62.f, 14.f, (i & 1) ? PAL_BELL : PAL_HUD, false, 0);

    float swing = std::sin((modeT_ + watch_) * (bell_ ? 11.f : 2.f)) * (bell_ ? 7.f : 1.2f);
    spr(art_.bell, 300.f + swing * 0.3f, 18.f + swing * 0.15f, bell_ ? 30.f : 22.f, PAL_BELL, false, 0);

    if (mode_ == Mode::Title) {
        int fr = int(modeT_ * 5.f) & 1;
        spr(art_.climber[fr], archX(1), 150.f, 28.f, PAL_FOE, false, 1);
        spr(art_.spark, archX(3), DECK_Y - 2.f, 16.f, PAL_FUSE, false, 0);
        spr(art_.sentry[fr], archX(2), DECK_Y - 36.f, 48.f, PAL_YOU, false, 0);
    } else {
        for (const Threat& t : threats_) {
            if (!t.alive) continue;
            float x = archX(t.arch) + ox;
            if (t.kind == CLIMB) {
                float p = 1.f - std::clamp(t.life / CLIMB_TIME, 0.f, 1.f);
                float y = 176.f - p * 100.f;
                spr(art_.climber[(t.age / 6) & 1], x, y, 26.f + p * 8.f, PAL_FOE, t.arch < 2, int((1.f - p) * 6));
            } else {
                float bob = std::sin(watch_ * 14.f + t.arch) * 2.f;
                spr(art_.spark, x, DECK_Y - 6.f + bob, 16.f, PAL_FUSE, false, 0);
            }
        }
        int fr = int(watch_ * 8.f) & 1;
        if (std::fabs(px_ - archX(urgent() < 0 ? 2 : threats_[std::max(0, urgent())].arch)) < 2.f) fr = 0;
        if (flash_ > 0) spr(art_.spark, px_ + face_ * 16.f, DECK_Y - 22.f, 12.f, PAL_FUSE, false, 0);
        spr(art_.sentry[fr], px_ + ox, DECK_Y - 40.f, 46.f, PAL_YOU, face_ < 0, 0);
    }

    char line[64];
    if (mode_ == Mode::Title) {
        hudC(22, "ONE VIADUCT.", PAL_HUD);
        hudC(23, "HOLD UNTIL THE RELIEF BELL.", PAL_OK);
        hudC(24, "LEFT RIGHT ALONG THE DECK", PAL_HUD);
        hudC(25, "A FIRES OR STAMPS A FUSE", PAL_HUD);
        hudC(26, "THEN IT IS DONE.", PAL_OK);
        hudC(27, "ENTER", PAL_HUD);
        int n = int(std::strlen(S3_VERSION_STRING));
        hud(39 - n, 0, S3_VERSION_STRING, PAL_HUD);
    } else {
        int left = std::max(0, int(std::ceil(RELIEF_AT - watch_)));
        if (!bell_) std::snprintf(line, sizeof line, "BELL %02d", std::max(0, int(std::ceil(BELL_AT - watch_))));
        else std::snprintf(line, sizeof line, "RELIEF %02d", left);
        hud(1, 0, line, bell_ ? PAL_OK : PAL_HUD);
        std::snprintf(line, sizeof line, "SCORE %d", score_);
        hud(28, 0, line, PAL_HUD);
        if (mode_ == Mode::Pause) hudC(26, "PAUSED", PAL_HUD);
        else if (mode_ == Mode::Victory) hudC(26, "THE WATCH HELD", PAL_OK);
        else if (mode_ == Mode::Over) hudC(26, reason_, PAL_ALERT);
        else if (bell_) hudC(26, "THE BELL. HOLD THE SPAN.", PAL_BELL);
        else hudC(26, "HOLD THE VIADUCT", PAL_HUD);
    }

    if (mode_ == Mode::Title) text("S3 VIADUCT", 160, 14, 0.75f, PAL_HUD);
    else if (mode_ == Mode::Victory) text("RELIEF", 160, 28, 1.1f, PAL_OK);
    else if (mode_ == Mode::Over) text("LOST", 160, 28, 1.1f, PAL_ALERT);
    else if (bell_) text("THE BELL", 160, 30, 0.8f, PAL_BELL);
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
    modeT_ += DT;
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

    if (mode_ == Mode::Watch) update(DT);
    else if (mode_ == Mode::Victory && fanStep_ >= 0) {
        fanT_ += DT;
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

}  // namespace viaduct

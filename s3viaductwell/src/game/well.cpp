#include "well.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace well {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float WELL_X = 160.f;
constexpr float DECK = 158.f;
constexpr float REACH = 20.f;
constexpr float BOLT_V = 360.f;
constexpr int STONES = 8;

int waveCount(int wave) { return 6 + wave * 2; }

float waveGap(int wave) { return 1.15f - float(wave) * 0.18f; }

float waveSpeed(int wave) { return 34.f + float(wave) * 10.f; }

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Victory || mode_ == Mode::Over) return 2;
    return 1;
}

int Game::nearest() const {
    int best = -1;
    float left = 1.0e9f;
    for (int i = 0; i < int(foes_.size()); i++) {
        if (!foes_[i].alive) continue;
        float d = std::fabs(foes_[i].x - WELL_X);
        if (d < left) {
            left = d;
            best = i;
        }
    }
    return best;
}

void Game::winWatch() {
    if (won_ || mode_ == Mode::Over) return;
    won_ = true;
    over_ = true;
    mode_ = Mode::Victory;
    modeT_ = 0;
    reason_ = "THE WELL STOOD THROUGH THREE WAVES";
    score_ += 500;
    sys_->apu.noiseBurst(0.12f, 1400.f, 0.06f);
    sys_->setLight(40, 170, 90);
}

void Game::loseWatch(const char* why) {
    if (won_ || mode_ == Mode::Victory || mode_ == Mode::Over) return;
    reason_ = why;
    won_ = false;
    over_ = true;
    mode_ = Mode::Over;
    modeT_ = 0;
    stones_ = 0;
    sys_->apu.noiseBurst(0.45f, 120.f, 0.35f);
    sys_->setLight(180, 30, 20);
    shake_ = 0.5f;
}

void Game::beginWatch() {
    mode_ = Mode::Watch;
    over_ = false;
    won_ = false;
    reason_ = "THE WELL FELL";
    score_ = 0;
    wave_ = 0;
    stones_ = STONES;
    spawned_ = 0;
    toSpawn_ = waveCount(0);
    px_ = WELL_X;
    clock_ = 0;
    cool_ = 0.3f;
    lull_ = 0;
    modeT_ = 0;
    shake_ = 0;
    between_ = false;
    face_ = 1;
    step_ = 0;
    foes_.clear();
    bolts_[0].on = false;
    bolts_[1].on = false;
    sys_->setLight(40, 70, 140);
}

void Game::fireAt(float tx) {
    if (cool_ > 0) return;
    int slot = -1;
    for (int i = 0; i < 2; i++)
        if (!bolts_[i].on) slot = i;
    if (slot < 0) return;
    float dir = tx >= px_ ? 1.f : -1.f;
    face_ = dir > 0 ? 1 : -1;
    bolts_[slot].on = true;
    bolts_[slot].x = px_ + dir * 10.f;
    bolts_[slot].vx = dir * BOLT_V;
    cool_ = 0.16f;
    sys_->apu.noiseBurst(0.08f, 1800.f, 0.03f);
    sys_->apu.tone(0, 680.f, 0.03f);
}

void Game::spawnDue() {
    if (between_ || spawned_ >= toSpawn_) return;
    float gap = waveGap(wave_);
    float due = 0.7f + float(spawned_) * gap;
    if (clock_ < due) return;
    Foe f;
    int side = (spawned_ + wave_) & 1;
    f.dir = side ? -1 : 1;
    f.x = side ? 312.f : 8.f;
    f.spd = waveSpeed(wave_);
    bool ram = wave_ == 2 && (spawned_ % 4) == 3;
    f.kind = ram ? 1 : 0;
    f.hp = ram ? 2 : 1;
    f.alive = true;
    foes_.push_back(f);
    spawned_++;
}

void Game::update(float dt) {
    const gs::Pad& pad = sys_->pad;
    if (shake_ > 0) shake_ -= dt;
    step_ = int(clock_ * 6.f) & 1;

    if (bot_) {
        int n = nearest();
        if (n >= 0) {
            float tx = foes_[n].x;
            float aim = WELL_X + std::clamp(tx - WELL_X, -28.f, 28.f);
            px_ += std::clamp(aim - px_, -90.f * dt, 90.f * dt);
            fireAt(tx);
        }
    } else {
        float mv = 0;
        if (pad.down(gs::BTN_LEFT)) mv -= 1;
        if (pad.down(gs::BTN_RIGHT)) mv += 1;
        if (std::fabs(pad.axisX) > 0.2f) mv = pad.axisX;
        if (mv != 0) face_ = mv > 0 ? 1 : -1;
        px_ += mv * 130.f * dt;
        if (pad.down(gs::BTN_A) || pad.down(gs::BTN_B)) {
            float tx = px_ + float(face_) * 80.f;
            int n = nearest();
            if (n >= 0 && std::fabs(foes_[n].x - px_) < 120.f) tx = foes_[n].x;
            fireAt(tx);
        }
    }
    px_ = std::clamp(px_, 28.f, 292.f);
    if (cool_ > 0) cool_ -= dt;

    for (int i = 0; i < 2; i++) {
        if (!bolts_[i].on) continue;
        bolts_[i].x += bolts_[i].vx * dt;
        if (bolts_[i].x < -8.f || bolts_[i].x > 328.f) bolts_[i].on = false;
    }

    spawnDue();

    for (Foe& f : foes_) {
        if (!f.alive) continue;
        f.age++;
        f.x += float(f.dir) * f.spd * dt;
        for (int i = 0; i < 2; i++) {
            if (!bolts_[i].on || !f.alive) continue;
            if (std::fabs(bolts_[i].x - f.x) < 12.f) {
                bolts_[i].on = false;
                f.hp--;
                if (f.hp <= 0) {
                    f.alive = false;
                    score_ += f.kind ? 180 : 100;
                    sys_->apu.noiseBurst(0.1f, 700.f, 0.04f);
                }
            }
        }
        if (!f.alive) continue;
        if (std::fabs(f.x - WELL_X) < REACH) {
            f.alive = false;
            int hit = f.kind ? 2 : 1;
            stones_ -= hit;
            shake_ = 0.28f;
            sys_->apu.noiseBurst(0.28f, 180.f, 0.12f);
            if (stones_ <= 0) {
                loseWatch("THE WELL FELL");
                return;
            }
        }
    }

    if (between_) {
        lull_ -= dt;
        if (lull_ <= 0) {
            between_ = false;
            wave_++;
            clock_ = 0;
            spawned_ = 0;
            toSpawn_ = waveCount(wave_);
            score_ += 200;
        }
        return;
    }

    bool any = false;
    for (const Foe& f : foes_)
        if (f.alive) any = true;
    if (spawned_ >= toSpawn_ && !any) {
        if (wave_ >= 2) {
            winWatch();
            return;
        }
        between_ = true;
        lull_ = 1.4f;
        foes_.clear();
    }
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

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.road[y].on = false;
        if (y < 118) {
            float u = float(y) / 118.f;
            v.lineBackdrop[y] = gs::rgb4(2 + int(u * 3), 2 + int(u * 2), 8 - int(u * 3));
            v.lineFog[y] = 1;
        } else if (y < 168) {
            v.lineBackdrop[y] = gs::rgb4(5, 5, 5);
            v.lineFog[y] = 2;
        } else {
            int ripple = ((y / 2 + int(clock_ * 8)) & 3) == 0 ? 1 : 0;
            v.lineBackdrop[y] = gs::rgb4(1, 3 + ripple, 6 + ripple);
            v.lineFog[y] = 5;
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
    if (shake_ > 0) ox = std::sin(clock_ * 48.f) * 3.f;

    for (int i = 0; i < 5; i++) {
        float x = 32.f + float(i) * 64.f + ox * 0.3f;
        spr(art_.pier, x, 108.f, 78.f, PAL_STONE, false, 3);
    }

    int cracks = STONES - std::max(stones_, 0);
    spr(art_.well, WELL_X + ox, 104.f, 64.f, PAL_WELL, false, 0);
    for (int i = 0; i < cracks && i < 4; i++) {
        spr(art_.crack, WELL_X - 10.f + float(i) * 7.f + ox, 124.f + float(i & 1) * 8.f, 14.f, PAL_BAD, i & 1, 0);
    }

    for (const Foe& f : foes_) {
        if (!f.alive) continue;
        const gs::Mipped& img = f.kind ? art_.rammer[step_] : art_.raider[(f.age / 8) & 1];
        float h = f.kind ? 36.f : 30.f;
        int pal = f.kind ? PAL_RAM : PAL_FOE;
        spr(img, f.x + ox, DECK - h, h, pal, f.dir < 0, 0);
    }

    for (int i = 0; i < 2; i++) {
        if (!bolts_[i].on) continue;
        spr(art_.bolt, bolts_[i].x + ox, DECK - 24.f, 4.f, PAL_SHOT, bolts_[i].vx < 0, 0);
    }

    float ph = 38.f;
    spr(art_.sentry[step_], px_ + ox, DECK - ph, ph, PAL_YOU, face_ < 0, 0);

    char line[48];
    std::snprintf(line, sizeof(line), "WAVE %d/3", wave_ + 1);
    hud(1, 1, line, PAL_HUD);
    std::snprintf(line, sizeof(line), "SCORE %d", score_);
    hud(28, 1, line, PAL_HUD);
    int shown = std::max(stones_, 0);
    std::snprintf(line, sizeof(line), "WELL ");
    for (int i = 0; i < STONES; i++) line[5 + i] = i < shown ? '#' : '-';
    line[5 + STONES] = 0;
    hud(1, 2, line, shown > 2 ? PAL_OK : PAL_BAD);

    if (mode_ == Mode::Title) {
        hudC(8, "S3 VIADUCT WELL", PAL_HUD);
        hudC(11, "KEEP THE WELL STANDING", PAL_HUD);
        hudC(13, "THROUGH THREE WAVES", PAL_HUD);
        hudC(16, "ARROWS MOVE    A FIRES", PAL_HUD);
        hudC(20, "PRESS START", (int(modeT_ * 2.f) & 1) ? PAL_HUD : PAL_OK);
    } else if (between_) {
        hudC(24, "THE SPAN IS QUIET", PAL_HUD);
    } else if (mode_ == Mode::Victory) {
        hudC(22, "THE WELL STOOD", PAL_OK);
        hudC(24, "THREE WAVES HELD", PAL_HUD);
    } else if (mode_ == Mode::Over) {
        hudC(22, "THE WATCH IS OVER", PAL_BAD);
        hudC(24, reason_, PAL_HUD);
    } else {
        hudC(26, "HOLD THE WELL", PAL_HUD);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    mode_ = Mode::Title;
    modeT_ = 0;
    over_ = false;
    won_ = false;
    sys.vdp.hudEnabled = true;
    sys.apu.setMaster(0.8f);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    modeT_ += DT;
    clock_ += DT;

    if (mode_ == Mode::Title) {
        bool go = pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A);
        if (bot_ && modeT_ > 0.35f) go = true;
        if (go) beginWatch();
    } else if (mode_ == Mode::Watch) {
        update(DT);
    } else if (pad.pressed(gs::BTN_START) && !bot_) {
        mode_ = Mode::Title;
        modeT_ = 0;
        over_ = false;
        won_ = false;
    }

    draw();
}

}  // namespace well

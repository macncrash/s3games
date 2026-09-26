#include "game/well.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace well {
namespace {

constexpr float kDt = 1.f / 60.f;
// Spawn x -16, pin x 118: 134px. Wave gaps assume that run at the speeds below.
constexpr float kSpawnX = -16.f;
constexpr float kStopX = 118.f;
constexpr float kBiteLo = 108.f;
constexpr float kBiteHi = 164.f;
constexpr float kGateX = 142.f;
constexpr float kWellX = 242.f;
constexpr float kWellY = 210.f;
constexpr float kWellDrawX = 274.f;
constexpr float kLaneV = 8.f;
constexpr float kCover = 0.34f;
constexpr float kSlamCd = 0.28f;
constexpr int kStones = 4;

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    auto ch = [&](int c0, int c1) { return int(c0 + (c1 - c0) * t + 0.5f); };
    return gs::rgb4(ch(ar, br), ch(ag, bg), ch(ab, bb));
}

gs::FMPatch dronePatch() {
    gs::FMPatch p;
    p.alg = 4;
    p.fb = 0.28f;
    p.op[0] = {1.f, 0.7f, 0.4f, 1.1f, 0.8f, 0.5f};
    p.op[1] = {2.f, 0.28f, 0.3f, 0.8f, 0.5f, 0.4f};
    p.op[2] = {0.5f, 0.45f, 0.4f, 1.2f, 0.7f, 0.5f};
    p.op[3] = {3.f, 0.14f, 0.2f, 0.6f, 0.3f, 0.35f};
    p.vol = 0.12f;
    p.tone = 540.f;
    p.drive = 0.05f;
    return p;
}

gs::FMPatch hitPatch() {
    gs::FMPatch p;
    p.alg = 7;
    p.fb = 0.18f;
    p.op[0] = {1.f, 1.f, 0.004f, 0.09f, 0.f, 0.06f};
    p.op[1] = {2.4f, 0.45f, 0.004f, 0.08f, 0.f, 0.06f};
    p.op[2] = {0.5f, 0.3f, 0.006f, 0.12f, 0.f, 0.08f};
    p.op[3] = {4.f, 0.2f, 0.005f, 0.07f, 0.f, 0.05f};
    p.vol = 0.2f;
    p.tone = 1800.f;
    return p;
}

gs::FMPatch hornPatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.2f;
    p.op[0] = {1.f, 1.f, 0.02f, 0.18f, 0.55f, 0.14f};
    p.op[1] = {2.f, 0.35f, 0.02f, 0.2f, 0.35f, 0.14f};
    p.op[2] = {3.f, 0.18f, 0.03f, 0.22f, 0.25f, 0.16f};
    p.op[3] = {1.f, 0.28f, 0.02f, 0.2f, 0.4f, 0.14f};
    p.vol = 0.18f;
    return p;
}

float bashLimit(int kind) {
    if (kind == 2) return 2.05f;
    if (kind == 1) return 1.05f;
    return 0.95f;
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Victory) return 3;
    if (mode_ == Mode::Over) return 4;
    if (mode_ == Mode::Play || mode_ == Mode::Banner || mode_ == Mode::Pause) return 1;
    return 0;
}

const char* Game::waveName(int w) const {
    if (w <= 0) return "BUCKETS";
    if (w == 1) return "CLUBS";
    return "THE RAM";
}

void Game::blip(float freq, float vol) { sys_->apu.keyOn(1, freq, vol); }

void Game::buildScript(int wave) {
    script_.clear();
    auto add = [&](float t, int lane, int kind, float speed) {
        Spawn s;
        s.t = t;
        s.lane = lane;
        s.kind = kind;
        s.speed = speed;
        script_.push_back(s);
    };
    if (wave <= 0) {
        const int lanes[] = {1, 0, 2, 1, 0, 2};
        for (int i = 0; i < 6; ++i) add(0.55f + i * 1.42f, lanes[i], 0, 46.f);
    } else if (wave == 1) {
        const int lanes[] = {0, 2, 1, 2, 0, 1, 0};
        for (int i = 0; i < 7; ++i) add(0.50f + i * 1.32f, lanes[i], 1, 42.f);
    } else {
        add(0.50f, 1, 0, 62.f);
        add(1.60f, 0, 0, 62.f);
        add(2.70f, 2, 0, 62.f);
        add(3.80f, 1, 0, 62.f);
        add(4.75f, 0, 2, 26.f);
        add(8.90f, 2, 0, 66.f);
        add(10.05f, 1, 0, 66.f);
        add(11.20f, 0, 1, 50.f);
        add(12.60f, 2, 0, 66.f);
        add(13.75f, 1, 0, 66.f);
    }
}

void Game::startWave() {
    foes_.clear();
    pops_.clear();
    puffs_.clear();
    spawnAt_ = 0;
    tWave_ = 0;
    slamCd_ = 0;
    buildScript(wave_);
}

void Game::beginRun() {
    wave_ = 0;
    nextWave_ = 0;
    stones_ = kStones;
    through_ = 0;
    score_ = 0;
    gateLane_ = 1.f;
    gateTarget_ = 1;
    over_ = false;
    won_ = false;
    reason_ = "UNFINISHED";
    fanStep_ = -1;
    fanT_ = 0;
    endT_ = 0;
    shake_ = 0;
    slamT_ = 0;
    mode_ = Mode::Play;
    startWave();
    sys_->setLight(70, 48, 22);
}

void Game::lose() {
    if (over_) return;
    stones_ = 0;
    won_ = false;
    over_ = true;
    mode_ = Mode::Over;
    reason_ = "THE WELL FELL";
    shake_ = 0.85f;
    sys_->rumble(0.9f, 1.f, 240);
    sys_->apu.noiseBurst(0.6f, 280.f, 0.22f);
    blip(64.f, 0.16f);
    sys_->setLight(90, 16, 12);
}

void Game::win() {
    if (over_ || stones_ <= 0) return;
    won_ = true;
    over_ = true;
    mode_ = Mode::Victory;
    reason_ = "THE WELL STANDS";
    score_ += stones_ * 250;
    fanStep_ = 0;
    fanT_ = 0;
    shake_ = 0.15f;
    sys_->setLight(40, 80, 30);
    blip(523.f, 0.16f);
}

void Game::kill(Foe& f) {
    int pts = f.kind == 2 ? 500 : (f.kind == 1 ? 150 : 100);
    score_ += pts;
    f.alive = false;
    Pop p;
    p.x = f.x;
    p.y = laneFoot(f.lane) - 40.f;
    p.t = 0.7f;
    p.pts = pts;
    pops_.push_back(p);
    Puff u;
    u.x = (f.x + kGateX) * 0.5f;
    u.y = laneFoot(f.lane) - 12.f;
    u.t = 0.28f;
    puffs_.push_back(u);
}

void Game::strike(int lane) {
    if (slamCd_ > 0.f) return;
    bool any = false;
    bool killed = false;
    for (Foe& f : foes_) {
        if (!f.alive || f.leaked || f.lane != lane) continue;
        if (f.x < kBiteLo || f.x > kBiteHi) continue;
        any = true;
        f.hp -= 1;
        f.flash = 0.1f;
        if (f.hp <= 0) {
            killed = true;
            kill(f);
        }
    }
    if (!any) return;
    slamCd_ = kSlamCd;
    slamT_ = 0.12f;
    shake_ = std::max(shake_, 0.14f);
    sys_->rumble(0.25f, 0.5f, 50);
    sys_->apu.noiseBurst(0.34f, killed ? 1700.f : 900.f, 0.05f);
    blip(killed ? 480.f : 170.f, 0.14f);
}

int Game::bestFoe() const {
    int best = -1;
    float bestU = 1e9f;
    for (int i = 0; i < int(foes_.size()); ++i) {
        const Foe& f = foes_[i];
        if (!f.alive || f.leaked || f.x > kBiteHi) continue;
        float u;
        if (f.x >= kStopX) u = -1000.f - f.bash * 12.f + std::fabs(float(f.lane) - gateLane_) * 0.02f;
        else u = (kStopX - f.x) / std::max(1.f, f.speed);
        if (u < bestU) {
            bestU = u;
            best = i;
        }
    }
    return best;
}

void Game::aim(float dt) {
    bool brace = false;
    if (bot_) {
        int dest = gateTarget_;
        int tgt = bestFoe();
        if (tgt >= 0) dest = foes_[tgt].lane;
        else if (spawnAt_ < int(script_.size())) dest = script_[spawnAt_].lane;
        gateTarget_ = std::clamp(dest, 0, 2);
        brace = true;
    } else if (mode_ == Mode::Play || mode_ == Mode::Banner) {
        const gs::Pad& p = sys_->pad;
        if (p.pressed(gs::BTN_UP)) gateTarget_ = std::max(0, gateTarget_ - 1);
        if (p.pressed(gs::BTN_DOWN)) gateTarget_ = std::min(2, gateTarget_ + 1);
        brace = p.down(gs::BTN_A) || p.down(gs::BTN_B) || p.down(gs::BTN_C) || p.down(gs::BTN_Z) || p.accel > 0.45f;
    }
    float d = float(gateTarget_) - gateLane_;
    float step = kLaneV * dt;
    if (std::fabs(d) <= step) gateLane_ = float(gateTarget_);
    else gateLane_ += std::copysign(step, d);
    bracing_ = brace;
}

void Game::fadeFx(float dt) {
    for (Pop& p : pops_) {
        p.t -= dt;
        p.y -= 16.f * dt;
    }
    pops_.erase(std::remove_if(pops_.begin(), pops_.end(), [](const Pop& p) { return p.t <= 0.f; }), pops_.end());
    for (Puff& u : puffs_) u.t -= dt;
    puffs_.erase(std::remove_if(puffs_.begin(), puffs_.end(), [](const Puff& u) { return u.t <= 0.f; }), puffs_.end());
    if (slamCd_ > 0.f) slamCd_ = std::max(0.f, slamCd_ - dt);
    if (slamT_ > 0.f) slamT_ = std::max(0.f, slamT_ - dt);
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - dt * 1.5f);
}

void Game::updatePlay(float dt) {
    tWave_ += dt;
    while (spawnAt_ < int(script_.size()) && script_[spawnAt_].t <= tWave_) {
        const Spawn& s = script_[spawnAt_];
        Foe f;
        f.lane = s.lane;
        f.kind = s.kind;
        f.hp = s.kind == 2 ? 2 : 1;
        f.x = kSpawnX;
        f.speed = s.speed;
        f.alive = true;
        foes_.push_back(f);
        ++spawnAt_;
    }
    aim(dt);
    for (Foe& f : foes_) {
        if (!f.alive) continue;
        if (f.flash > 0.f) f.flash -= dt;
        if (f.leaked) {
            f.x += f.speed * dt;
            f.anim += dt * 8.f;
            if (f.x >= kWellX) {
                f.alive = false;
                ++through_;
                --stones_;
                shake_ = std::max(shake_, 0.55f);
                sys_->rumble(0.8f, 1.f, 160);
                sys_->apu.noiseBurst(0.5f, 360.f, 0.16f);
                blip(78.f, 0.14f);
                if (stones_ <= 0) {
                    stones_ = 0;
                    lose();
                    break;
                }
            }
            continue;
        }
        bool cover = std::fabs(gateLane_ - float(f.lane)) <= kCover;
        if (cover && f.x >= kStopX && f.x <= kBiteHi) {
            f.x = kStopX;
            f.bash += dt;
            f.anim += dt * 10.f;
            if (f.bash >= bashLimit(f.kind)) {
                f.leaked = true;
                f.x = kBiteHi + 8.f;
                f.bash = 0.f;
                shake_ = std::max(shake_, 0.4f);
                sys_->apu.noiseBurst(0.4f, 500.f, 0.1f);
                blip(110.f, 0.1f);
            }
        } else {
            f.x += f.speed * dt;
            f.anim += dt * 7.f;
            if (f.x > kBiteHi) f.leaked = true;
        }
    }
    if (over_) return;
    if (bracing_) {
        int lane = int(std::lround(gateLane_));
        if (std::fabs(gateLane_ - float(lane)) <= kCover) strike(lane);
    }
    if (over_) return;
    std::vector<Foe> keep;
    keep.reserve(foes_.size());
    for (const Foe& f : foes_)
        if (f.alive) keep.push_back(f);
    foes_.swap(keep);
    if (spawnAt_ >= int(script_.size())) {
        bool any = false;
        for (const Foe& f : foes_)
            if (f.alive) any = true;
        if (!any) {
            if (stones_ <= 0) lose();
            else if (wave_ >= 2) win();
            else {
                nextWave_ = wave_ + 1;
                mode_ = Mode::Banner;
                bannerT_ = 1.45f;
                buildScript(nextWave_);
                spawnAt_ = 0;
                foes_.clear();
                blip(660.f, 0.12f);
            }
        }
    }
}

void Game::updateBanner(float dt) {
    aim(dt);
    bannerT_ -= dt;
    if (bannerT_ <= 0.f) {
        wave_ = nextWave_;
        startWave();
        mode_ = Mode::Play;
        blip(392.f, 0.1f);
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    if (feet) cy -= h * 0.5f;
    stamp(m, cx, cy, w, h, pal, flip);
}

void Game::stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, bool flip) {
    if (w < 1.f || h < 1.f || m.h < 1) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx + camX_ - s.w * 0.5f));
    s.y = int16_t(std::lround(cy + camY_ - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal & 15);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::shadowAt(float x, float y, float w) {
    if (art_.shadow.h < 1 || w < 2.f) return;
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = 7;
    s.x = int16_t(std::lround(x + camX_ - s.w * 0.5f));
    s.y = int16_t(std::lround(y + camY_ - 3.f));
    s.img = art_.shadow.pick(7);
    s.shadow = true;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    const float adv = 18.f * scale;
    float left = x - float(s.size()) * adv * 0.5f;
    for (size_t i = 0; i < s.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, left + float(i) * adv + g.w * scale * 0.5f, y, g.h * scale, pal, false, false);
    }
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); ++i) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::backdrop() {
    uint16_t top = gs::rgb4(1, 2, 6);
    uint16_t hor = gs::rgb4(12, 7, 4);
    if (mode_ == Mode::Victory) {
        top = gs::rgb4(3, 6, 11);
        hor = gs::rgb4(14, 10, 5);
    } else if (mode_ == Mode::Over) {
        top = gs::rgb4(4, 1, 2);
        hor = gs::rgb4(8, 3, 2);
    }
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; ++y) {
        float u = y < 52 ? y / 52.f : 1.f;
        v.lineBackdrop[y] = lerpC(top, hor, u);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
    int wob = 6 + int(2.f * std::sin(t_ * 3.f));
    v.setColor(PAL_STONE * 16 + 6, gs::rgb4(2, wob, 10));
    int lamp = 10 + int(2.f * std::sin(t_ * 7.f));
    v.setColor(PAL_YARD * 16 + 13, gs::rgb4(14, lamp, 3));
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();
    camX_ = 0;
    camY_ = 0;
    if (shake_ > 0.f) {
        camX_ = std::sin(t_ * 47.f) * shake_ * 6.f;
        camY_ = std::cos(t_ * 39.f) * shake_ * 3.f;
    }

    if (mode_ == Mode::Title) {
        text("S3 GATE WELL", 160, 15, 0.9f, PAL_GOLD);
        text("THE WELL MUST STAND", 160, 36, 0.48f, PAL_TEXT);
    } else if (mode_ == Mode::Victory) {
        text("THE WELL STANDS", 160, 16, 0.72f, PAL_GOLD);
        text("THREE WAVES", 160, 36, 0.5f, PAL_GOOD);
    } else if (mode_ == Mode::Over) {
        text("THE WELL FELL", 160, 16, 0.8f, PAL_ALERT);
        text("THE WATCH IS LOST", 160, 36, 0.46f, PAL_TEXT);
    } else if (mode_ == Mode::Banner) {
        char line[24];
        std::snprintf(line, sizeof line, "WAVE %d", nextWave_ + 1);
        text(line, 160, 16, 0.7f, PAL_GOLD);
        text(waveName(nextWave_), 160, 36, 0.5f, PAL_TEXT);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160, 18, 0.8f, PAL_GOLD);
    }

    for (const Pop& p : pops_) {
        char buf[12];
        std::snprintf(buf, sizeof buf, "%d", p.pts);
        text(buf, p.x, p.y, 0.4f, PAL_GOLD);
    }

    float gy = laneFoot(0) + gateLane_ * (laneFoot(1) - laneFoot(0));
    float lunge = slamT_ > 0.f ? (slamT_ / 0.12f) * 12.f : 0.f;
    float gx = kGateX - lunge;
    int kframe = (bracing_ || slamT_ > 0.f) ? 1 : (int(t_ * 3.f) & 1);

    spr(art_.keeper[kframe], gx + 22.f, gy - 20.f, 34.f, PAL_KEEPER, false, false);
    spr(art_.gate, gx, gy - 18.f, 48.f, PAL_WOOD, false, false);
    if (slamT_ > 0.04f) spr(art_.shock, gx - 16.f, gy - 16.f, 18.f, PAL_FX, false, false);

    auto drawFoe = [&](int kind, int frame, int lane, float x, float flash) {
        const gs::Mipped* m = art_.runner;
        int pal = PAL_RAIDER;
        float h = 34.f;
        if (kind == 1) {
            m = art_.club;
            h = 38.f;
        } else if (kind == 2) {
            m = art_.ram;
            pal = PAL_RAM;
            h = 30.f;
        }
        float y = laneFoot(lane) - h * 0.45f;
        if (flash > 0.f) pal = PAL_FX;
        spr(m[frame & 1], x, y, h, pal, false, false);
    };

    if (mode_ == Mode::Title) {
        float s = std::sin(t_ * 0.85f);
        float ax = 86.f + s * 34.f;
        bool back = std::cos(t_ * 0.85f) < 0.f;
        int fr = int(t_ * 6.f) & 1;
        spr(art_.runner[fr], ax, laneFoot(1) - 16.f, 34.f, PAL_RAIDER, back, false);
        spr(art_.club[fr], 46.f, laneFoot(2) - 18.f, 36.f, PAL_RAIDER, false, false);
    } else {
        for (const Foe& f : foes_) {
            if (!f.alive) continue;
            int fr = int(f.anim) & 1;
            if (f.kind == 2 && f.hp < 2) fr = 1;
            drawFoe(f.kind, fr, f.lane, f.x, f.flash);
            if (f.bash > 0.12f && !f.leaked) {
                float w = 28.f * std::clamp(f.bash / bashLimit(f.kind), 0.f, 1.f);
                stamp(art_.bar, f.x, laneFoot(f.lane) - 40.f, std::max(4.f, w), 4.f, PAL_ALERT, false);
            }
        }
    }

    for (const Puff& u : puffs_) {
        float k = 1.f - u.t / 0.28f;
        spr(art_.puff, u.x, u.y, 8.f + k * 18.f, PAL_FX, false, false);
    }

    if (stones_ <= 0 || mode_ == Mode::Over) {
        spr(art_.rubble, kWellDrawX, kWellY - 20.f, 48.f, PAL_STONE, false, false);
    } else {
        spr(art_.well, kWellDrawX, kWellY - 75.f, 150.f, PAL_STONE, false, false);
        int cracks = kStones - stones_;
        for (int i = 0; i < cracks; ++i) {
            float ox = (i == 1 ? 14.f : (i == 2 ? -10.f : 2.f));
            float oy = (i == 0 ? -40.f : (i == 1 ? -18.f : -64.f));
            spr(art_.crack, kWellDrawX + ox, kWellY + oy, 36.f + i * 4.f, PAL_STONE, i == 2, false);
        }
        if (stones_ > 1) {
            float swing = std::sin(t_ * 1.7f) * 6.f;
            spr(art_.bucket, kWellDrawX + swing, kWellY - 86.f, 14.f, PAL_STONE, false, false);
        } else {
            spr(art_.bucket, kWellDrawX + 28.f, kWellY - 10.f, 14.f, PAL_STONE, false, false);
        }
    }

    float barY = gy + 6.f;
    stamp(art_.bar, 168.f, barY, 120.f, 5.f, bracing_ ? PAL_GOLD : PAL_TEXT, false);

    spr(art_.winch, kGateX, 58.f, 16.f, PAL_WOOD, false, false);
    float chainTop = 66.f;
    float chainBot = gy - 40.f;
    if (chainBot > chainTop) {
        for (float y = chainTop; y < chainBot; y += 7.f) spr(art_.chain, kGateX, y, 8.f, PAL_WOOD, false, false);
    }

    if ((int(t_ * 2.f) & 3) != 0) spr(art_.star, 250, 14, 6, PAL_SKY, false, false);
    if ((int(t_ * 2.f + 1.f) & 3) != 0) spr(art_.star, 300, 22, 5, PAL_SKY, false, false);
    spr(art_.star, 24, 18, 5, PAL_SKY, false, false);
    spr(art_.moon, 286, 18, 20, PAL_SKY, false, false);

    shadowAt(gx, gy + 8.f, 36.f);
    shadowAt(kWellDrawX, kWellY - 4.f, 60.f);
    if (mode_ != Mode::Title) {
        for (const Foe& f : foes_)
            if (f.alive) shadowAt(f.x, laneFoot(f.lane) + 2.f, f.kind == 2 ? 40.f : 22.f);
    }

    char line[40];
    if (mode_ == Mode::Title) {
        hud(1, 24, "UP DOWN MOVES THE GATE", PAL_TEXT);
        hud(1, 25, "Z HOLDS THE SLAM", PAL_GOLD);
        hud(1, 26, "ENTER TAKES THE WATCH", PAL_GOOD);
        hud(28, 27, S3_VERSION_STRING, PAL_TEXT);
    } else if (mode_ == Mode::Play || mode_ == Mode::Pause || mode_ == Mode::Banner) {
        std::snprintf(line, sizeof line, "WAVE %d/3", std::min(3, wave_ + 1));
        hud(1, 1, line, PAL_GOLD);
        std::string pips = "WELL ";
        int show = std::max(0, stones_);
        for (int i = 0; i < kStones; ++i) pips.push_back(i < show ? '#' : '-');
        hud(27, 1, pips, show > 1 ? PAL_GOOD : PAL_ALERT);
        std::snprintf(line, sizeof line, "SCORE %d", score_);
        hud(1, 26, line, PAL_TEXT);
        if (mode_ == Mode::Play && tWave_ < 1.6f) hud(28, 26, waveName(wave_), PAL_GOLD);
    } else if (mode_ == Mode::Victory) {
        hud(1, 1, "THE WELL STANDS", PAL_GOOD);
        std::snprintf(line, sizeof line, "SCORE %d", score_);
        hud(1, 26, line, PAL_GOLD);
        hud(22, 26, "ENTER", PAL_TEXT);
    } else if (mode_ == Mode::Over) {
        hud(1, 1, "THE WELL FELL", PAL_ALERT);
        std::snprintf(line, sizeof line, "WAVE %d", wave_ + 1);
        hud(30, 1, line, PAL_TEXT);
        hud(1, 26, "ENTER TRIES AGAIN", PAL_GOLD);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.84f);
    sys.apu.setEcho(0.18f, 0.28f, 0.16f);
    sys.apu.setPatch(0, dronePatch());
    sys.apu.setPatch(1, hitPatch());
    sys.apu.setPatch(2, hornPatch());
    sys.apu.keyOn(0, 78.f, 0.04f);
    droneOn_ = true;
    droneWatch_ = false;
    if (bot_) beginRun();
    else {
        mode_ = Mode::Title;
        sys.setLight(30, 36, 70);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    const gs::Pad& pad = sys.pad;
    bool watch = mode_ == Mode::Play || mode_ == Mode::Banner;
    if (droneOn_ && watch != droneWatch_) {
        droneWatch_ = watch;
        sys.apu.keyOn(0, watch ? 52.f : 78.f, watch ? 0.05f : 0.035f);
    }

    if (mode_ == Mode::Title) {
        gateLane_ = 1.f + std::sin(t_ * 0.75f);
        gateTarget_ = int(std::lround(std::clamp(gateLane_, 0.f, 2.f)));
        bracing_ = std::sin(t_ * 0.75f) > 0.2f;
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            blip(660.f, 0.08f);
            beginRun();
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
        else if (pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
            foes_.clear();
        }
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else {
            updatePlay(kDt);
            fadeFx(kDt);
        }
    } else if (mode_ == Mode::Banner) {
        updateBanner(kDt);
        fadeFx(kDt);
    } else {
        endT_ += kDt;
        fadeFx(kDt);
        if (mode_ == Mode::Victory) {
            fanT_ += kDt;
            if (fanStep_ >= 0 && fanT_ > 0.16f) {
                static const float notes[] = {262.f, 330.f, 392.f, 523.f, 392.f, 523.f};
                if (fanStep_ < 6) sys.apu.keyOn(2, notes[fanStep_], 0.16f);
                else sys.apu.keyOff(2);
                ++fanStep_;
                fanT_ = 0;
                if (fanStep_ > 8) fanStep_ = -1;
            }
        }
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            if (mode_ == Mode::Over) beginRun();
            else {
                mode_ = Mode::Title;
                over_ = false;
                won_ = false;
                foes_.clear();
            }
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
            foes_.clear();
        }
    }
    draw();
}

}  // namespace well

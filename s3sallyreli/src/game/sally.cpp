#include "sally.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace sally {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float kBellAt = 33.f;
constexpr float kRing = 1.8f;
constexpr float kGate = 46.f;
constexpr float kSpawn = 312.f;
constexpr float kHomeX = 118.f;
constexpr float kRetreat = 78.f;
constexpr int kLaneY[3] = {96, 136, 176};

struct Row {
    float arrive;
    int kind;
    int lane;
};

const Row kRows[] = {
    {5.0f, 1, 0},  {7.6f, 0, 1},  {10.2f, 1, 2}, {13.2f, 2, 1}, {16.0f, 1, 0},
    {18.6f, 0, 2}, {21.6f, 2, 0}, {24.2f, 1, 1}, {26.8f, 1, 2}, {29.4f, 0, 0},
    {31.6f, 2, 2},
};

float speedOf(int kind) { return kind == 2 ? 36.f : kind == 1 ? 74.f : 52.f; }
int hpOf(int kind) { return kind == 2 ? 2 : 1; }
int ptsOf(int kind) { return kind == 2 ? 250 : kind == 1 ? 100 : 150; }

gs::FMPatch dronePatch() {
    gs::FMPatch p;
    p.alg = 4;
    p.fb = 0.18f;
    p.op[0] = {1.f, 0.5f, 0.4f, 1.2f, 0.8f, 0.5f};
    p.op[1] = {0.5f, 0.28f, 0.35f, 0.9f, 0.6f, 0.4f};
    p.op[2] = {2.f, 0.1f, 0.2f, 0.5f, 0.3f, 0.25f};
    p.op[3] = {3.2f, 0.06f, 0.15f, 0.4f, 0.2f, 0.2f};
    p.vol = 0.06f;
    p.tone = 240.f;
    p.drive = 0.04f;
    return p;
}

gs::FMPatch bellPatch() {
    gs::FMPatch p;
    p.alg = 7;
    p.fb = 0.08f;
    p.op[0] = {1.f, 1.f, 0.004f, 0.5f, 0.15f, 0.85f};
    p.op[1] = {2.71f, 0.4f, 0.005f, 0.36f, 0.08f, 0.7f};
    p.op[2] = {5.15f, 0.16f, 0.007f, 0.28f, 0.04f, 0.55f};
    p.op[3] = {7.8f, 0.07f, 0.01f, 0.2f, 0.02f, 0.4f};
    p.vol = 0.2f;
    p.echo = 0.34f;
    return p;
}

}  // namespace

int Game::marker() const {
    if (over_ || mode_ == Mode::Victory || mode_ == Mode::Over) return 3;
    if (bell_ && (mode_ == Mode::Sally || mode_ == Mode::Pause)) return 2;
    if (mode_ == Mode::Sally || mode_ == Mode::Pause) return 1;
    return 0;
}

int Game::soonest(int lane) const {
    int best = -1;
    float eta = 1.0e9f;
    for (int i = 0; i < int(foes_.size()); i++) {
        const Foe& f = foes_[size_t(i)];
        if (!f.on) continue;
        if (lane >= 0 && f.lane != lane) continue;
        float e = (f.x - kGate) / speedOf(f.kind);
        bool nearer = e < eta - 0.02f;
        bool tieHere = std::fabs(e - eta) <= 0.02f && f.lane == lane_;
        if (nearer || tieHere || best < 0) {
            if (nearer || best < 0 || tieHere) {
                eta = e;
                best = i;
            }
        }
    }
    return best;
}

void Game::beginSally() {
    mode_ = Mode::Sally;
    over_ = false;
    won_ = false;
    bell_ = false;
    reason_ = "THE SALLY FAILED";
    score_ = 0;
    hp_ = 4;
    lane_ = 1;
    spawnAt_ = 0;
    fanStep_ = -1;
    flash_ = 0;
    x_ = kHomeX;
    watch_ = 0;
    pikeCd_ = shotCd_ = laneCd_ = shake_ = bellTick_ = modeT_ = fanT_ = 0;
    foes_.clear();
    puffs_.clear();
    pops_.clear();
    script_.clear();
    for (const Row& r : kRows) {
        Spawn s;
        s.t = r.arrive - (kSpawn - kGate) / speedOf(r.kind);
        s.kind = r.kind;
        s.lane = r.lane;
        script_.push_back(s);
    }
    if (!sys_) return;
    sys_->apu.setPatch(0, dronePatch());
    sys_->apu.setPatch(1, bellPatch());
    sys_->apu.keyOn(0, 49.f, 0.05f);
    sys_->setLight(70, 60, 40);
}

void Game::fell(Foe& f) {
    if (!f.on) return;
    f.on = false;
    score_ += f.points;
    float y = float(kLaneY[f.lane]);
    puffs_.push_back({f.x, y - 10.f, 0.35f, 16.f});
    pops_.push_back({f.x, y - 36.f, 0.7f, f.points});
    if (puffs_.size() > 8) puffs_.erase(puffs_.begin());
    if (pops_.size() > 4) pops_.erase(pops_.begin());
    sys_->apu.tone(1, 620.f, 0.04f);
    sys_->apu.noiseBurst(0.12f, 900.f, 0.05f);
}

void Game::loseSally(const char* why) {
    if (won_ || mode_ == Mode::Over || mode_ == Mode::Victory) return;
    won_ = false;
    over_ = true;
    mode_ = Mode::Over;
    reason_ = why;
    sys_->apu.keyOn(0, 70.f, 0.08f);
    sys_->apu.noiseBurst(0.35f, 140.f, 0.25f);
    sys_->rumble(0.6f, 0.2f, 180);
    sys_->setLight(150, 30, 20);
}

void Game::winSally() {
    if (won_ || mode_ == Mode::Over) return;
    won_ = true;
    over_ = true;
    mode_ = Mode::Victory;
    reason_ = "THE SALLY HELD UNTIL THE RELIEF BELL";
    score_ += 700 + hp_ * 150;
    fanStep_ = 0;
    fanT_ = 0;
    sys_->apu.setVol(0, 0.03f);
    sys_->apu.keyOn(1, 523.f, 0.2f);
    sys_->rumble(0.2f, 0.4f, 160);
    sys_->setLight(40, 140, 60);
}

void Game::strike() {
    if (pikeCd_ > 0 || bell_ && watch_ > kBellAt + kRing) return;
    if (won_ || mode_ != Mode::Sally) return;
    pikeCd_ = 0.18f;
    flash_ = 4;
    sys_->apu.noiseBurst(0.16f, 480.f, 0.04f);
    int hit = -1;
    float nearest = 1.0e9f;
    for (int i = 0; i < int(foes_.size()); i++) {
        Foe& f = foes_[size_t(i)];
        if (!f.on || f.lane != lane_) continue;
        if (f.x > x_ + 52.f || f.x < x_ - 16.f) continue;
        if (f.x < nearest) {
            nearest = f.x;
            hit = i;
        }
    }
    if (hit < 0) return;
    Foe& f = foes_[size_t(hit)];
    f.hp--;
    f.x += 10.f;
    if (f.hp <= 0) fell(f);
    else sys_->apu.tone(1, 180.f, 0.04f);
}

void Game::shoot() {
    if (shotCd_ > 0 || won_ || mode_ != Mode::Sally) return;
    shotCd_ = 0.42f;
    flash_ = 5;
    sys_->apu.tone(1, 140.f, 0.05f);
    sys_->apu.noiseBurst(0.22f, 1800.f, 0.04f);
    int hit = -1;
    float nearest = 1.0e9f;
    for (int i = 0; i < int(foes_.size()); i++) {
        Foe& f = foes_[size_t(i)];
        if (!f.on || f.lane != lane_) continue;
        if (f.x < x_ + 8.f || f.x > x_ + 210.f) continue;
        if (f.x < nearest) {
            nearest = f.x;
            hit = i;
        }
    }
    if (hit < 0) return;
    Foe& f = foes_[size_t(hit)];
    f.hp--;
    if (f.hp <= 0) fell(f);
}

void Game::bot(bool& up, bool& down, bool& pike, bool& shot) {
    up = down = pike = shot = false;
    int threat = soonest(-1);
    if (threat >= 0) {
        int want = foes_[size_t(threat)].lane;
        if (want < lane_) up = true;
        else if (want > lane_) down = true;
    }
    int here = soonest(lane_);
    if (here < 0) return;
    float fx = foes_[size_t(here)].x;
    if (fx < x_ + 50.f) pike = true;
    else if (fx < x_ + 200.f) shot = true;
}

void Game::update(float dt) {
    const gs::Pad& pad = sys_->pad;
    bool up = false, down = false, left = false, right = false, pike = false, shot = false;
    if (bot_) bot(up, down, pike, shot);
    else {
        up = pad.down(gs::BTN_UP);
        down = pad.down(gs::BTN_DOWN);
        left = pad.down(gs::BTN_LEFT);
        right = pad.down(gs::BTN_RIGHT);
        pike = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C);
        shot = pad.pressed(gs::BTN_B);
    }
    if (laneCd_ > 0) laneCd_ -= dt;
    if (laneCd_ <= 0) {
        if (up && lane_ > 0) {
            lane_--;
            laneCd_ = 0.12f;
        } else if (down && lane_ < 2) {
            lane_++;
            laneCd_ = 0.12f;
        }
    }
    float vx = 0;
    if (left) vx -= 70.f;
    if (right) vx += 70.f;
    if (!left && !right) x_ += (kHomeX - x_) * std::min(1.f, dt * 6.f);
    x_ += vx * dt;
    x_ = std::clamp(x_, 60.f, 168.f);
    if (!bell_ && x_ < kRetreat) {
        loseSally("THE SALLY TURNED BACK");
        return;
    }

    if (pikeCd_ > 0) pikeCd_ -= dt;
    if (shotCd_ > 0) shotCd_ -= dt;
    if (pike) strike();
    if (shot) shoot();

    while (spawnAt_ < int(script_.size()) && script_[size_t(spawnAt_)].t <= watch_) {
        const Spawn& s = script_[size_t(spawnAt_++)];
        Foe f;
        f.kind = s.kind;
        f.lane = s.lane;
        f.hp = hpOf(s.kind);
        f.points = ptsOf(s.kind);
        f.x = kSpawn;
        f.on = true;
        foes_.push_back(f);
    }

    for (Foe& f : foes_) {
        if (!f.on) continue;
        f.x -= speedOf(f.kind) * dt;
        if (f.lane == lane_ && f.x <= x_ - 4.f) {
            f.on = false;
            hp_--;
            shake_ = 0.35f;
            sys_->rumble(0.45f, 0.2f, 90);
            sys_->apu.noiseBurst(0.28f, 220.f, 0.08f);
            puffs_.push_back({x_, float(kLaneY[lane_]) - 8.f, 0.3f, 14.f});
            if (hp_ <= 0) {
                loseSally("THE SALLY WAS CUT DOWN");
                return;
            }
        } else if (f.x <= kGate) {
            loseSally("THE GATE WAS TAKEN");
            return;
        }
    }

    if (shake_ > 0) shake_ -= dt;
    if (flash_ > 0) flash_--;
    for (Puff& p : puffs_) p.t -= dt;
    for (Pop& p : pops_) {
        p.t -= dt;
        p.y -= 18.f * dt;
    }
    puffs_.erase(std::remove_if(puffs_.begin(), puffs_.end(), [](const Puff& p) { return p.t <= 0; }), puffs_.end());
    pops_.erase(std::remove_if(pops_.begin(), pops_.end(), [](const Pop& p) { return p.t <= 0; }), pops_.end());

    watch_ += dt;
    if (!bell_ && watch_ >= kBellAt) {
        bell_ = true;
        bellTick_ = 0;
        sys_->setLight(160, 130, 40);
    }
    if (bell_) {
        bellTick_ -= dt;
        if (bellTick_ <= 0) {
            bellTick_ = 0.55f;
            sys_->apu.keyOn(1, 392.f, 0.16f);
        }
        if (watch_ >= kBellAt + kRing) winSally();
    }
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
        shx_ = std::sin(watch_ * 46.f) * 3.f;
        shy_ = std::cos(watch_ * 31.f) * 1.4f;
    }
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.hudEnabled = true;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.road[y].on = false;
        if (y < 78) v.lineBackdrop[y] = gs::rgb4(4, 5, 8);
        else if (y < 96) v.lineBackdrop[y] = gs::rgb4(5, 6, 4);
        else v.lineBackdrop[y] = gs::rgb4(3 + ((y / 8) & 1), 5, 2);
        v.lineFog[y] = 0;
    }

    stamp(art_.wall, 0, 28, 78, 176, PAL_WALL);
    stamp(art_.arch, 18, 118, 52, 70, PAL_WALL);
    float swing = std::sin(watch_ * (bell_ ? 10.f : 2.2f)) * (bell_ ? 6.f : 1.4f);
    spr(art_.bell, 36.f + swing, 40.f, bell_ ? 28.f : 20.f, PAL_BELL, false, 0);
    spr(art_.banner, 70.f, 58.f + std::sin(watch_ * 3.f) * 2.f, 26.f, PAL_ALERT, false, 0);

    auto imgOf = [&](const Foe& f) -> const gs::Mipped& {
        int fr = int(watch_ * (f.kind == 1 ? 10.f : 6.f)) & 1;
        if (f.kind == 2) return art_.shield[fr];
        if (f.kind == 1) return art_.runner[fr];
        return art_.raider[fr];
    };
    for (const Foe& f : foes_) {
        if (!f.on) continue;
        float y = float(kLaneY[f.lane]);
        int fog = int(std::clamp((f.x - 80.f) / 22.f, 0.f, 10.f));
        int pal = f.kind == 2 ? PAL_SHIELD : PAL_RAID;
        float h = f.kind == 2 ? 40.f : f.kind == 1 ? 34.f : 36.f;
        spr(imgOf(f), f.x, y, h, pal, false, fog);
    }

    float py = float(kLaneY[lane_]);
    int fr = int(watch_ * 6.f) & 1;
    bool thrusting = pikeCd_ > 0.08f;
    spr(art_.pike, x_ + (thrusting ? 28.f : 18.f), py - 6.f, thrusting ? 12.f : 9.f, PAL_YOU, false, 0);
    spr(art_.you[fr], x_, py, 46.f, PAL_YOU, false, 0);
    spr(art_.pistol, x_ + 8.f, py - 2.f, 12.f, PAL_YOU, false, 0);
    if (flash_ > 0) spr(art_.flash, x_ + (pikeCd_ > shotCd_ ? 40.f : 22.f), py - 8.f, 14.f, PAL_FX, false, 0);

    for (const Puff& p : puffs_) spr(art_.puff, p.x, p.y, p.r * (p.t / 0.35f + 0.4f), PAL_FX, false, 0);
    for (const Pop& p : pops_) {
        char buf[16];
        std::snprintf(buf, sizeof buf, "%d", p.pts);
        text(buf, p.x, p.y, 0.45f, PAL_HUD);
    }

    if (mode_ == Mode::Title) text("S3 SALLY RELIEF", 188, 28, 0.72f, PAL_HUD);
    else if (mode_ == Mode::Victory) text("RELIEF", 200, 24, 1.05f, PAL_OK);
    else if (mode_ == Mode::Over) text("SALLY OVER", 200, 24, 0.8f, PAL_ALERT);
    else if (bell_) text("THE BELL", 200, 22, 0.8f, PAL_BELL);

    char line[48];
    if (mode_ == Mode::Title) {
        hudC(22, "ONE SALLY.", PAL_HUD);
        hudC(23, "HOLD UNTIL THE RELIEF BELL.", PAL_OK);
        hudC(25, "ARROWS STEP   A PIKE   B PISTOL", PAL_HUD);
        hudC(26, "DO NOT TURN BACK BEFORE THE BELL", PAL_ALERT);
        hudC(27, "START", PAL_HUD);
        int n = int(std::strlen(S3_VERSION_STRING));
        hud(39 - n, 0, S3_VERSION_STRING, PAL_HUD);
    } else {
        int left = std::max(0, int(std::ceil(kBellAt - watch_)));
        if (!bell_) std::snprintf(line, sizeof line, "BELL %02d", left);
        else std::snprintf(line, sizeof line, "BELL");
        hud(1, 0, line, bell_ ? PAL_BELL : PAL_HUD);
        std::snprintf(line, sizeof line, "HP %d", hp_);
        hud(12, 0, line, hp_ > 1 ? PAL_HUD : PAL_ALERT);
        std::snprintf(line, sizeof line, "SCORE %d", score_);
        hud(1, 27, line, PAL_HUD);
        if (mode_ == Mode::Pause) hudC(26, "PAUSED", PAL_HUD);
        else if (mode_ == Mode::Victory) hudC(26, "THE SALLY HELD", PAL_OK);
        else if (mode_ == Mode::Over) hudC(26, reason_, PAL_ALERT);
        else if (bell_) hudC(26, "HOLD THE SALLY", PAL_BELL);
        else hudC(26, "ONE SALLY  HOLD THE FIELD", PAL_HUD);
    }
    if (mode_ == Mode::Victory) sys_->setLight(40, 170, 80);
    else if (mode_ == Mode::Over) sys_->setLight(170, 24, 28);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    mode_ = Mode::Title;
    modeT_ = 0;
    if (bot_) beginSally();
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
    } else if (back && mode_ == Mode::Sally) mode_ = Mode::Pause;
    else if (back && mode_ == Mode::Pause) mode_ = Mode::Title;
    else if (start && (mode_ == Mode::Title || mode_ == Mode::Victory || mode_ == Mode::Over)) beginSally();
    else if (start && mode_ == Mode::Sally) mode_ = Mode::Pause;
    else if (start && mode_ == Mode::Pause) mode_ = Mode::Sally;

    if (mode_ == Mode::Sally) update(DT);
    else if (mode_ == Mode::Victory && fanStep_ >= 0) {
        fanT_ += DT;
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

}  // namespace sally

#include "game/culvert.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace culvert {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr float BELL_AT = 32.0f;
constexpr float BELL_HOLD = 6.0f;
constexpr int MAX_HP = 6;

struct Spawn {
    float t;
    int kind;
    int lane;
};

const Spawn kScript[] = {
    {0.7f, 1, 0},  {2.4f, 0, 2},  {4.2f, 1, 1},  {6.2f, 0, 0},  {8.0f, 0, 2},
    {9.6f, 1, 2},  {11.4f, 2, 1}, {13.2f, 0, 0}, {15.0f, 1, 1}, {16.6f, 0, 2},
    {18.2f, 2, 0}, {20.0f, 1, 2}, {21.6f, 0, 1}, {23.2f, 1, 0}, {24.8f, 0, 2},
    {26.4f, 2, 1}, {28.0f, 1, 1}, {29.6f, 0, 0}, {31.2f, 1, 2}, {33.0f, 0, 1},
};

float foeSpeed(int kind) {
    if (kind == 1) return 0.30f;
    if (kind == 2) return 0.16f;
    return 0.13f;
}

}  // namespace

int Game::marker() const {
    if (over_ || mode_ == Mode::Victory || mode_ == Mode::Over) return 3;
    if (mode_ == Mode::Bell) return 2;
    if (mode_ == Mode::Watch) return 1;
    return 0;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    mode_ = Mode::Title;
    t_ = 0;
}

void Game::beginWatch() {
    mode_ = Mode::Watch;
    t_ = 0;
    watch_ = 0;
    hp_ = MAX_HP;
    score_ = 0;
    lane_ = 1;
    duck_ = 0;
    fireCd_ = 0;
    laneCd_ = 0;
    spawnAt_ = 0;
    over_ = false;
    won_ = false;
    reason_ = "WATCH OVER";
    foes_.clear();
    bolts_.clear();
    shots_.clear();
    nades_.clear();
    if (!drone_) {
        gs::FMPatch p;
        p.alg = 4;
        p.fb = 0.2f;
        p.op[0] = {1, 0.7f, 0.02f, 0.4f, 0.8f, 0.3f};
        p.op[1] = {2, 0.35f, 0.04f, 0.5f, 0.4f, 0.3f};
        p.op[2] = {3, 0.2f, 0.05f, 0.4f, 0.3f, 0.25f};
        p.op[3] = {1, 0.25f, 0.02f, 0.3f, 0.5f, 0.2f};
        p.vol = 0.12f;
        p.tone = 900;
        sys_->apu.setPatch(0, p);
        sys_->apu.keyOn(0, 55, 0.1f);
        drone_ = true;
    }
}

void Game::lose(const char* why) {
    if (mode_ == Mode::Over || mode_ == Mode::Victory) return;
    reason_ = why;
    mode_ = Mode::Over;
    endT_ = 0;
    won_ = false;
    sys_->apu.noiseBurst(0.5f, 400.0f, 0.35f);
    sys_->apu.keyOff(0);
    drone_ = false;
}

void Game::answer() {
    if (mode_ != Mode::Bell) return;
    if (lane_ != 1) return;
    mode_ = Mode::Victory;
    won_ = true;
    endT_ = 0;
    score_ += 200;
    reason_ = "THE WATCH HELD UNTIL THE RELIEF BELL";
    sys_->apu.tone(1, 523.0f, 0.12f);
    sys_->apu.tone(2, 659.0f, 0.1f);
}

void Game::botThink() {
    if (mode_ == Mode::Bell) {
        if (lane_ != 1 && laneCd_ <= 0) {
            lane_ += lane_ < 1 ? 1 : -1;
            laneCd_ = 0.08f;
        }
        bool urgent = false;
        for (const Foe& f : foes_)
            if (f.alive && f.z < 0.22f) urgent = true;
        if (!urgent && lane_ == 1 && watch_ > BELL_AT + 0.35f) answer();
    }
    int want = lane_;
    float nearZ = 2;
    int nearLane = lane_;
    for (const Foe& f : foes_) {
        if (!f.alive) continue;
        if (f.z < nearZ) {
            nearZ = f.z;
            nearLane = f.lane;
        }
    }
    bool dodge = false;
    for (const Shot& s : shots_)
        if (s.lane == lane_ && s.z < 0.16f) dodge = true;
    for (const Nade& n : nades_)
        if (n.lane == lane_ && n.fuse < 0.28f) dodge = true;
    if (dodge) {
        duck_ = 0.16f;
        if (mode_ != Mode::Bell) {
            for (const Nade& n : nades_) {
                if (n.lane == lane_ && n.fuse < 0.4f) want = lane_ == 0 ? 1 : 0;
            }
        }
    } else if (nearZ < 1.5f) {
        want = nearLane;
    }
    if (mode_ == Mode::Bell && nearZ > 0.28f) want = 1;
    if (want != lane_ && laneCd_ <= 0) {
        lane_ += want > lane_ ? 1 : -1;
        laneCd_ = 0.1f;
    }
    if (fireCd_ <= 0) {
        for (const Foe& f : foes_) {
            if (f.alive && f.lane == lane_ && f.z < 0.95f) {
                bolts_.push_back({lane_, 0.02f});
                fireCd_ = 0.26f;
                sys_->apu.tone(1, 740.0f, 0.05f);
                break;
            }
        }
    }
}

void Game::update(float dt) {
    watch_ += dt;
    scroll_ += dt * 90.0f;
    if (fireCd_ > 0) fireCd_ -= dt;
    if (laneCd_ > 0) laneCd_ -= dt;
    if (duck_ > 0) duck_ -= dt;
    if (shake_ > 0) shake_ -= dt;

    while (spawnAt_ < int(sizeof(kScript) / sizeof(kScript[0])) && watch_ >= kScript[spawnAt_].t) {
        const Spawn& s = kScript[spawnAt_++];
        Foe f;
        f.kind = s.kind;
        f.lane = s.lane;
        f.z = 1.05f;
        f.speed = foeSpeed(s.kind);
        f.cool = 0.45f;
        f.hp = 1;
        foes_.push_back(f);
    }

    if (mode_ == Mode::Watch && watch_ >= BELL_AT) {
        mode_ = Mode::Bell;
        bellSwing_ = 0;
        sys_->apu.tone(2, 392.0f, 0.14f);
    }
    if (mode_ == Mode::Bell) {
        bellSwing_ += dt;
        if (std::fmod(bellSwing_, 0.55f) < dt) sys_->apu.tone(2, 392.0f, 0.12f);
        if (watch_ > BELL_AT + BELL_HOLD) lose("MISSED THE BELL");
    }

    for (Foe& f : foes_) {
        if (!f.alive) continue;
        f.z -= f.speed * dt;
        f.cool -= dt;
        if (f.kind == 0 && f.z < 0.62f && f.z > 0.12f && f.cool <= 0) {
            shots_.push_back({f.lane, f.z});
            f.cool = 1.35f;
        }
        if (f.kind == 2 && f.z < 0.5f && f.cool <= 0) {
            nades_.push_back({lane_, 0.85f, f.z});
            f.cool = 8.0f;
        }
        if (f.z <= 0.02f) {
            f.alive = false;
            if (f.kind == 1) {
                hp_ -= 2;
                shake_ = 0.25f;
                sys_->apu.noiseBurst(0.4f, 600.0f, 0.2f);
            } else {
                hp_ -= 1;
                shake_ = 0.15f;
            }
        }
    }
    for (Bolt& b : bolts_) b.z += 1.85f * dt;
    for (Shot& s : shots_) s.z -= 0.85f * dt;
    for (Nade& n : nades_) {
        n.fuse -= dt;
        n.z -= 0.35f * dt;
    }

    for (Bolt& b : bolts_) {
        if (b.z > 1.3f) continue;
        for (Foe& f : foes_) {
            if (!f.alive || f.lane != b.lane) continue;
            if (std::fabs(f.z - b.z) < 0.06f) {
                f.hp -= 1;
                b.z = 9;
                if (f.hp <= 0) {
                    f.alive = false;
                    score_ += f.kind == 1 ? 50 : f.kind == 2 ? 80 : 40;
                    sys_->apu.noiseBurst(0.22f, 1200.0f, 0.08f);
                }
                break;
            }
        }
    }
    for (Shot& s : shots_) {
        if (s.z > 0.02f) continue;
        s.z = -1;
        if (s.lane == lane_ && duck_ <= 0) {
            hp_ -= 1;
            shake_ = 0.18f;
            sys_->apu.noiseBurst(0.3f, 800.0f, 0.12f);
        }
    }
    for (Nade& n : nades_) {
        if (n.fuse > 0) continue;
        n.fuse = -1;
        if (n.lane == lane_ && duck_ <= 0) {
            hp_ -= 2;
            shake_ = 0.28f;
            sys_->apu.noiseBurst(0.55f, 500.0f, 0.25f);
        }
    }

    foes_.erase(std::remove_if(foes_.begin(), foes_.end(), [](const Foe& f) { return !f.alive; }), foes_.end());
    bolts_.erase(std::remove_if(bolts_.begin(), bolts_.end(), [](const Bolt& b) { return b.z > 1.25f; }), bolts_.end());
    shots_.erase(std::remove_if(shots_.begin(), shots_.end(), [](const Shot& s) { return s.z < 0; }), shots_.end());
    nades_.erase(std::remove_if(nades_.begin(), nades_.end(), [](const Nade& n) { return n.fuse < 0; }), nades_.end());

    if (hp_ <= 0) lose("THE MOUTH FELL");
}

void Game::project(int lane, float z, float& sx, float& sy, float& s) const {
    float u = std::clamp(z, 0.0f, 1.15f);
    float near = 1.0f - u;
    sx = 160.0f + float(lane - 1) * (28.0f + near * 78.0f);
    sy = 92.0f + near * 108.0f;
    s = 16.0f + near * 64.0f;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal, int align) {
    const float adv = 16.0f * scale;
    float w = float(s.size()) * adv;
    if (align == 0) x -= w * 0.5f;
    else if (align > 0) x -= w;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + i * adv + g.w * scale * 0.5f, y, g.h * scale, pal, false);
    }
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.A.clear();
    v.B.clear();
    v.HUD.clear();
    float sh = shake_ > 0 ? std::sin(shake_ * 80.0f) * 3.0f : 0;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float k = float(y) / float(gs::SCREEN_H);
        int r = int(2 + (6 - 2) * k);
        int g = int(3 + (6 - 3) * k);
        int b = int(6 + (5 - 6) * k);
        v.lineBackdrop[y] = gs::rgb4(r, g, b);
        v.lineFog[y] = y < 88 ? uint8_t(8 - y / 12) : 0;
        gs::RoadLine& rd = v.road[y];
        rd = gs::RoadLine{};
        if (y >= 96) {
            float t = float(y - 96) / 128.0f;
            rd.on = true;
            rd.cx = 160.0f + sh;
            rd.hw = 16.0f + t * t * 150.0f;
            rd.v = scroll_ + (1.0f - t) * 2200.0f;
            rd.pal = PAL_ROAD;
            rd.style = gs::ROAD_MUD;
            rd.band = (int(rd.v / 80.0f) & 1) ? 1 : 0;
            rd.left = gs::GROUND_WATER;
            rd.right = gs::GROUND_LAND;
        }
    }
    v.roadTime = int(watch_ * 60.0f);

    for (const Foe& f : foes_) {
        float sx, sy, sc;
        project(f.lane, f.z, sx, sy, sc);
        int fog = int(std::clamp(f.z, 0.0f, 1.0f) * 10);
        const gs::Mipped& img = f.kind == 1 ? art_.runner : f.kind == 2 ? art_.grenadier : art_.raider;
        spr(img, sx + sh, sy, sc, PAL_RAID, f.lane < 1, fog);
    }
    for (const Shot& s : shots_) {
        float sx, sy, sc;
        project(s.lane, s.z, sx, sy, sc);
        spr(art_.shot, sx, sy - sc * 0.15f, 10.0f + (1.0f - s.z) * 8.0f, PAL_BOLT);
    }
    for (const Nade& n : nades_) {
        float sx, sy, sc;
        float zz = std::max(n.z, 0.05f);
        project(n.lane, zz, sx, sy, sc);
        float hop = std::sin(std::clamp(1.0f - n.fuse / 0.85f, 0.0f, 1.0f) * 3.14159f) * 36.0f;
        spr(art_.grenade, sx, sy - hop, 14, PAL_RAID);
    }
    for (const Bolt& b : bolts_) {
        float sx, sy, sc;
        project(b.lane, b.z, sx, sy, sc);
        spr(art_.bolt, sx, sy - 8, 12, PAL_BOLT, false);
    }

    spr(art_.archL, 36, 130, 200, PAL_STONE);
    spr(art_.archR, 284, 130, 200, PAL_STONE);
    spr(art_.lintel, 160, 18, 36, PAL_STONE);
    spr(art_.lip, 160, 206, 18, PAL_STONE);

    float px = 70.0f + float(lane_) * 90.0f;
    bool ducked = duck_ > 0;
    spr(ducked ? art_.sentryDuck : art_.sentry, px, ducked ? 188 : 176, ducked ? 40 : 62, PAL_SENTRY);

    if (mode_ == Mode::Bell || mode_ == Mode::Victory) {
        float swing = std::sin(bellSwing_ * 14.0f) * 8.0f;
        spr(art_.bell, 160 + swing, 46, 28, PAL_BELL);
    }

    if (mode_ == Mode::Title) {
        text("S3 CULVERT RELIEF", 160, 78, 0.72f, PAL_HUD);
        text("HOLD UNTIL THE RELIEF BELL", 160, 108, 0.42f, PAL_BELL);
        text("ARROWS POST   A FIRE   DOWN DUCK", 160, 136, 0.36f, PAL_HUD);
        text("ANSWER THE BELL AT THE MOUTH", 160, 156, 0.36f, PAL_LAMP);
        if (int(t_ * 2) % 2 == 0) text("START", 160, 184, 0.5f, PAL_BELL);
    } else if (mode_ == Mode::Victory) {
        text("RELIEF", 160, 86, 1.0f, PAL_BELL);
        text("THE WATCH HELD", 160, 118, 0.5f, PAL_HUD);
    } else if (mode_ == Mode::Over) {
        text("WATCH OVER", 160, 86, 0.8f, PAL_RAID);
        text(reason_, 160, 118, 0.4f, PAL_HUD);
    }

    char line[64];
    std::snprintf(line, sizeof line, "HP %d", std::max(hp_, 0));
    hud(1, 1, line, PAL_HUD);
    std::snprintf(line, sizeof line, "SCORE %d", score_);
    hud(28, 1, line, PAL_HUD);
    if (mode_ == Mode::Watch) {
        int left = int(std::ceil(std::max(0.0f, BELL_AT - watch_)));
        std::snprintf(line, sizeof line, "RELIEF %02d", left);
        hudC(26, line, PAL_HUD);
        hudC(25, "HOLD THE MOUTH", PAL_LAMP);
    } else if (mode_ == Mode::Bell) {
        hudC(25, "THE RELIEF BELL", PAL_BELL);
        hudC(26, lane_ == 1 ? "B ANSWERS" : "BACK TO THE MOUTH", PAL_HUD);
    } else if (mode_ == Mode::Victory) {
        hudC(26, "THE WATCH HELD UNTIL THE RELIEF BELL", PAL_BELL);
    } else if (mode_ == Mode::Over) {
        hudC(26, reason_, PAL_RAID);
    }
    const char* posts[3] = {"LEFT BANK", "THE MOUTH", "RIGHT BANK"};
    if (mode_ == Mode::Watch || mode_ == Mode::Bell) hudC(3, posts[lane_], ducked ? PAL_LAMP : PAL_HUD);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    gs::Pad& pad = sys.pad;
    t_ += DT;

    if (mode_ == Mode::Title) {
        if (bot_ && t_ > 0.35f) beginWatch();
        else if (pad.pressed(gs::BTN_START)) beginWatch();
    } else if (mode_ == Mode::Watch || mode_ == Mode::Bell) {
        if (bot_) botThink();
        else {
            if (laneCd_ <= 0) {
                if (pad.pressed(gs::BTN_LEFT) && lane_ > 0) {
                    lane_--;
                    laneCd_ = 0.12f;
                } else if (pad.pressed(gs::BTN_RIGHT) && lane_ < 2) {
                    lane_++;
                    laneCd_ = 0.12f;
                }
            }
            if (pad.down(gs::BTN_DOWN)) duck_ = 0.12f;
            if (pad.pressed(gs::BTN_A) && fireCd_ <= 0) {
                bolts_.push_back({lane_, 0.02f});
                fireCd_ = 0.32f;
                sys.apu.tone(1, 680.0f, 0.05f);
            }
            if (mode_ == Mode::Bell && pad.pressed(gs::BTN_B)) answer();
        }
        if (mode_ == Mode::Watch || mode_ == Mode::Bell) update(DT);
    } else if (mode_ == Mode::Victory || mode_ == Mode::Over) {
        endT_ += DT;
        if (endT_ > 1.2f) over_ = true;
        if (!bot_ && over_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Title;
            t_ = 0;
            over_ = false;
        }
    }

    // Silence the one-shot square after a short tick. Channel 2 is the bell.
    if (mode_ != Mode::Bell && mode_ != Mode::Victory) sys.apu.tone(2, 0, 0);
    if (fireCd_ <= 0.2f && mode_ != Mode::Victory) sys.apu.tone(1, 0, 0);

    draw();
}

}  // namespace culvert

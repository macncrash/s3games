#include "game/well.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace tww {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr int WAVES = 3;
constexpr int WELL_MAX = 8;
constexpr float WELL_X = 108.0f;
constexpr float LAUNCH_X = 78.0f;
constexpr float BOLT_V = 310.0f;
constexpr int LANE_Y[3] = {128, 158, 190};

struct WaveDef {
    int count;
    float gap;
    float speed;
    float bruteSpeed;
    int bruteEvery;
};

const WaveDef WAVES_DEF[WAVES] = {
    {6, 1.35f, 34.0f, 28.0f, 0},
    {8, 1.05f, 46.0f, 32.0f, 4},
    {10, 0.82f, 54.0f, 36.0f, 3},
};

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

}  // namespace

int Game::marker() const {
    if (won_) return 2;
    if (over_) return 3;
    if (mode_ == Mode::Fight || mode_ == Mode::Brief || mode_ == Mode::Gap || mode_ == Mode::Pause) return 1;
    return 0;
}

float Game::rnd() {
    rng_ = rng_ * 1664525u + 1013904223u;
    return (rng_ >> 8) * (1.0f / 16777216.0f);
}

float Game::laneY(int lane) const { return float(LANE_Y[std::clamp(lane, 0, 2)]); }

float Game::foeH(int lane) const {
    if (lane <= 0) return 28.0f;
    if (lane == 1) return 36.0f;
    return 46.0f;
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.08f);
    toneT_ = 0.07f;
}

void Game::serviceAudio() {
    if (toneT_ > 0) {
        toneT_ -= DT;
        if (toneT_ <= 0) sys_->apu.tone(0, 0, 0);
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

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    const float adv = 16.0f * scale;
    x -= float(s.size()) * adv * 0.5f;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + i * adv + g.w * scale * 0.5f, y, g.h * scale, pal, false, 0);
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 40 || s.x + s.w < -40 || s.y > gs::SCREEN_H + 40 || s.y + s.h < -40) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.hudEnabled = true;
    mode_ = Mode::Title;
    if (bot_) begin();
}

void Game::begin() {
    wave_ = 0;
    well_ = WELL_MAX;
    over_ = false;
    won_ = false;
    reason_ = "THE WELL IS STANDING";
    aim_ = 1;
    foes_.clear();
    bolts_.clear();
    puffs_.clear();
    openWave();
}

void Game::openWave() {
    const WaveDef& w = WAVES_DEF[std::clamp(wave_, 0, WAVES - 1)];
    quota_ = w.count;
    spawned_ = 0;
    spawnT_ = 0.45f;
    cool_ = 0.2f;
    brief_ = 1.1f;
    foes_.clear();
    bolts_.clear();
    mode_ = Mode::Brief;
    t_ = 0;
}

void Game::win() {
    mode_ = Mode::Victory;
    over_ = true;
    won_ = true;
    reason_ = "THE WELL STANDS";
    wave_ = WAVES;
    blip(660);
}

void Game::lose(const char* why) {
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    reason_ = why;
    well_ = 0;
    sys_->apu.noiseBurst(0.5f, 240.0f, 0.4f);
}

void Game::fire() {
    if (cool_ > 0 || mode_ != Mode::Fight) return;
    Bolt b;
    b.lane = aim_;
    b.x = LAUNCH_X;
    b.live = true;
    bolts_.push_back(b);
    cool_ = 0.22f;
    blip(740);
}

void Game::bot() {
    if (mode_ != Mode::Fight || cool_ > 0) return;
    const Foe* best = nullptr;
    for (const Foe& f : foes_) {
        if (!f.alive) continue;
        bool covered = false;
        for (const Bolt& b : bolts_) {
            if (b.live && b.lane == f.lane && b.x < f.x - 6.0f) covered = true;
        }
        if (covered) continue;
        if (!best || f.x < best->x) best = &f;
    }
    if (!best) return;
    aim_ = best->lane;
    fire();
}

void Game::update() {
    const WaveDef& def = WAVES_DEF[std::clamp(wave_, 0, WAVES - 1)];
    if (mode_ == Mode::Brief) {
        brief_ -= DT;
        if (brief_ <= 0) mode_ = Mode::Fight;
        return;
    }
    if (mode_ == Mode::Gap) {
        brief_ -= DT;
        if (brief_ <= 0) {
            if (wave_ >= WAVES) win();
            else openWave();
        }
        return;
    }
    if (mode_ != Mode::Fight) return;

    cool_ = std::max(0.0f, cool_ - DT);
    if (bot_) bot();

    if (spawned_ < quota_) {
        spawnT_ -= DT;
        if (spawnT_ <= 0) {
            Foe f;
            f.lane = spawned_ % 3;
            if (wave_ == 2) f.lane = (spawned_ * 2) % 3;
            bool brute = def.bruteEvery > 0 && ((spawned_ + 1) % def.bruteEvery) == 0;
            f.kind = brute ? 1 : 0;
            f.hp = brute ? 2 : 1;
            f.speed = brute ? def.bruteSpeed : def.speed;
            f.x = 332.0f + rnd() * 8.0f;
            f.alive = true;
            foes_.push_back(f);
            spawned_++;
            spawnT_ = def.gap;
        }
    }

    for (Bolt& b : bolts_) {
        if (!b.live) continue;
        b.x += BOLT_V * DT;
        if (b.x > 340) b.live = false;
    }

    for (Foe& f : foes_) {
        if (!f.alive) continue;
        f.x -= f.speed * DT;
        for (Bolt& b : bolts_) {
            if (!b.live || b.lane != f.lane) continue;
            if (b.x > f.x - 8.0f && b.x < f.x + 14.0f) {
                b.live = false;
                f.hp--;
                blip(420);
                if (f.hp <= 0) {
                    f.alive = false;
                    puffs_.push_back({f.lane, f.x, 0});
                    sys_->apu.noiseBurst(0.22f, 1400.0f, 0.12f);
                }
            }
        }
        if (f.alive && f.x <= WELL_X) {
            f.alive = false;
            well_--;
            shake_ = 1.0f;
            sys_->apu.noiseBurst(0.4f, 180.0f, 0.25f);
            if (well_ <= 0) {
                lose("THE WELL FALLS");
                return;
            }
        }
    }

    bool any = false;
    for (const Foe& f : foes_)
        if (f.alive) any = true;
    if (spawned_ >= quota_ && !any && !over_) {
        wave_++;
        bolts_.clear();
        if (wave_ >= WAVES) win();
        else {
            mode_ = Mode::Gap;
            brief_ = 0.9f;
        }
    }

    for (auto it = puffs_.begin(); it != puffs_.end();) {
        it->age += DT;
        if (it->age > 0.35f) it = puffs_.erase(it);
        else ++it;
    }
}

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    const uint16_t top = gs::rgb4(3, 5, 10);
    const uint16_t hor = gs::rgb4(12, 8, 5);
    const uint16_t grass = gs::rgb4(3, 6, 2);
    const uint16_t grassFar = gs::rgb4(5, 8, 3);
    const int horizon = 118;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.road[y].on = false;
        v.lineFog[y] = 0;
        if (y < horizon) v.lineBackdrop[y] = lerpC(top, hor, y / float(horizon));
        else v.lineBackdrop[y] = lerpC(grassFar, grass, (y - horizon) / float(gs::SCREEN_H - horizon));
    }
    v.setFogColor(gs::rgb4(6, 5, 4));
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    sky();
    float sh = 0;
    if (shake_ > 0) {
        sh = (rnd() - 0.5f) * 6.0f * shake_;
        shake_ = std::max(0.0f, shake_ - DT * 2.2f);
    }

    auto groundY = [&](int lane) { return laneY(lane) + sh; };

    for (int i = 0; i < 5; i++) {
        float x = 150.0f + i * 36.0f;
        spr(art_.tree, x, 108 + sh, 40 + (i % 2) * 8, PAL_FIELD, i & 1, 6);
    }
    spr(art_.reed, 130, 176 + sh, 28, PAL_FIELD, false, 0);
    spr(art_.reed, 250, 200 + sh, 22, PAL_FIELD, true, 0);

    spr(art_.tower, 48 + sh * 0.2f, 118 + sh, 150, PAL_STONE);
    spr(art_.banner, 70, 52 + std::sin(t_ * 3.0f) * 2.0f + sh, 26, PAL_SKY);
    for (int lane = 0; lane < 3; lane++) {
        float y = 78 + lane * 16.0f;
        spr(art_.slit, 62, y, lane == aim_ ? 12.0f : 8.0f, lane == aim_ ? PAL_FX : PAL_WOOD);
    }

    const gs::Mipped& wellSpr = well_ <= WELL_MAX / 2 ? art_.wellHurt : art_.well;
    spr(wellSpr, 96 + sh, 176 + sh, 70, PAL_WELL);
    bucket_ += DT;
    spr(art_.bucket, 96, 154 + std::sin(bucket_) * 4.0f + sh, 14, PAL_WOOD);

    for (int lane = 2; lane >= 0; --lane) {
        for (const Foe& f : foes_) {
            if (!f.alive || f.lane != lane) continue;
            float bob = std::sin(t_ * 8.0f + f.x * 0.05f) * 1.5f;
            bool step = std::sin(t_ * 10.0f + f.x) > 0;
            const gs::Mipped& body = f.kind ? art_.brute : art_.raider;
            spr(body, f.x, groundY(lane) + bob, foeH(lane) * (f.kind ? 1.15f : 1.0f), f.kind ? PAL_BRUTE : PAL_RAIDER,
                step, lane == 0 ? 3 : 0);
        }
        for (const Bolt& b : bolts_) {
            if (!b.live || b.lane != lane) continue;
            spr(art_.bolt, b.x, groundY(lane) - foeH(lane) * 0.35f, 8, PAL_FX);
        }
        for (const Puff& p : puffs_) {
            if (p.lane != lane) continue;
            spr(art_.puff, p.x, groundY(lane) - 8, 16 + p.age * 30, PAL_FX);
        }
    }

    std::string marks;
    for (int i = 0; i < WELL_MAX; i++) marks.push_back(i < well_ ? '#' : '.');
    hud(1, 1, "WELL", PAL_HUD);
    hud(6, 1, marks, well_ > 3 ? PAL_HUD : PAL_HUD);
    hud(28, 1, "WAVE " + std::to_string(std::min(wave_ + 1, WAVES)) + "/3", PAL_HUD);

    if (mode_ == Mode::Title) {
        text("TOWER WELL", 160, 70, 1.15f, PAL_HUD);
        text("KEEP THE WELL STANDING", 160, 108, 0.55f, PAL_HUD);
        text("THREE WAVES", 160, 132, 0.55f, PAL_HUD);
        text("UP DOWN AIM   A FIRE", 160, 168, 0.48f, PAL_HUD);
        hudC(26, "PRESS START", PAL_HUD);
    } else if (mode_ == Mode::Brief) {
        text(std::string("WAVE ") + char('1' + std::clamp(wave_, 0, 2)), 160, 48, 1.0f, PAL_HUD);
    } else if (mode_ == Mode::Gap) {
        text("WAVE HELD", 160, 48, 0.8f, PAL_HUD);
    } else if (mode_ == Mode::Victory) {
        text("THE WELL STANDS", 160, 46, 0.72f, PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        text("THE WELL FALLS", 160, 46, 0.72f, PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        text("HOLD", 160, 48, 1.0f, PAL_HUD);
    } else if (mode_ == Mode::Fight) {
        hud(1, 26, "UP DOWN AIM", PAL_HUD);
        hud(26, 26, "A FIRE", PAL_HUD);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    serviceAudio();
    const gs::Pad& p = sys.pad;
    if (mode_ == Mode::Title) {
        if (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A) || bot_) begin();
    } else if (mode_ == Mode::Pause) {
        if (p.pressed(gs::BTN_START)) mode_ = pausedFrom_;
    } else if (!over_) {
        if (!bot_ && p.pressed(gs::BTN_START) && (mode_ == Mode::Fight || mode_ == Mode::Brief)) {
            pausedFrom_ = mode_;
            mode_ = Mode::Pause;
        } else {
            if (p.pressed(gs::BTN_UP)) aim_ = std::max(0, aim_ - 1);
            if (p.pressed(gs::BTN_DOWN)) aim_ = std::min(2, aim_ + 1);
            if (p.pressed(gs::BTN_A) || p.pressed(gs::BTN_B) || p.pressed(gs::BTN_C)) fire();
            update();
        }
    } else if (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A)) {
        if (!bot_) {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
        }
    }
    draw();
}

}  // namespace tww

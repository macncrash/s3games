#include "game/well.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace granary {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr float STRIKE_Y = 168.0f;
constexpr float WELL_L = 118.0f;
constexpr float WELL_R = 202.0f;
constexpr int WELL_MAX = 4;

}  // namespace

float Game::rnd() {
    rng_ = rng_ * 1664525u + 1013904223u;
    return (rng_ >> 8) * (1.0f / 16777216.0f);
}

void Game::blip(int ch, float freq, float vol) { sys_->apu.tone(ch, freq, vol); }

int Game::marker() const {
    if (mode_ == Mode::Watch) return 1;
    if (mode_ == Mode::Banner) return 2;
    if (mode_ == Mode::Victory || mode_ == Mode::Defeat) return 3;
    return 0;
}

void Game::bootWatch() {
    wave_ = 0;
    wellHp_ = WELL_MAX;
    stopped_ = 0;
    through_ = 0;
    score_ = 0;
    over_ = false;
    won_ = false;
    reason_ = "THE WELL FELL";
    px_ = 160;
    swing_ = 0;
    swingCd_ = 0;
    stones_.clear();
    puffs_.clear();
    prepareWave();
    mode_ = Mode::Watch;
    t_ = 0;
}

void Game::prepareWave() {
    spawns_.clear();
    spawnI_ = 0;
    stones_.clear();
    const int n = 7 + wave_ * 3;
    float t = 0.45f;
    float prevX = 160.0f;
    for (int i = 0; i < n; i++) {
        Spawn s;
        s.t = t;
        bool pair = wave_ > 0 && (i % 3) == 1;
        if (pair) {
            s.x = std::clamp(prevX + (prevX < 160 ? 26.0f : -26.0f), 40.0f, 280.0f);
            t += 0.40f;
        } else {
            float side = (i % 2) ? -1.0f : 1.0f;
            float spread = 46.0f + float(wave_) * 16.0f + float((i / 2) % 3) * 12.0f;
            s.x = std::clamp(160.0f + side * spread, 36.0f, 284.0f);
            t += 1.28f - float(wave_) * 0.12f;
        }
        prevX = s.x;
        spawns_.push_back(s);
    }
}

void Game::update(float dt) {
    bool left = false, right = false, fire = false;
    if (bot_) {
        const Stone* threat = nullptr;
        for (const Stone& s : stones_) {
            if (!s.live) continue;
            if (!threat || s.y > threat->y) threat = &s;
        }
        if (!threat && spawnI_ < int(spawns_.size())) {
            float nx = spawns_[size_t(spawnI_)].x;
            if (px_ < nx - 6) right = true;
            else if (px_ > nx + 6) left = true;
        } else if (threat) {
            if (px_ < threat->x - 5) right = true;
            else if (px_ > threat->x + 5) left = true;
            if (std::fabs(px_ - threat->x) < 18.0f && threat->y > 118.0f) fire = true;
        }
    } else if (sys_) {
        const gs::Pad& pad = sys_->pad;
        left = pad.down(gs::BTN_LEFT) || pad.axisX < -0.3f;
        right = pad.down(gs::BTN_RIGHT) || pad.axisX > 0.3f;
        fire = pad.down(gs::BTN_A) || pad.down(gs::BTN_B) || pad.down(gs::BTN_C);
    }

    float speed = 340.0f;
    if (left) px_ -= speed * dt;
    if (right) px_ += speed * dt;
    px_ = std::clamp(px_, 28.0f, 292.0f);

    if (swing_ > 0) swing_ -= dt;
    if (swingCd_ > 0) swingCd_ -= dt;
    if (fire && swing_ <= 0 && swingCd_ <= 0) {
        swing_ = 0.12f;
        swingCd_ = 0.20f;
        blip(0, 180, 0.08f);
    }

    const float vy = 50.0f + float(wave_) * 12.0f;
    while (spawnI_ < int(spawns_.size()) && spawns_[size_t(spawnI_)].t <= t_) {
        Stone s;
        s.x = spawns_[size_t(spawnI_)].x;
        s.y = -6.0f;
        s.vx = (160.0f - s.x) * 0.12f;
        s.vy = vy;
        s.live = true;
        stones_.push_back(s);
        spawnI_++;
    }

    for (Stone& s : stones_) {
        if (!s.live) continue;
        s.x += s.vx * dt;
        s.y += s.vy * dt;
        s.vx += (160.0f - s.x) * 0.35f * dt;
        if (swing_ > 0 && s.y > 108.0f && s.y < STRIKE_Y && std::fabs(s.x - px_) < 26.0f) {
            s.live = false;
            stopped_++;
            score_ += 10 + wave_ * 5;
            puffs_.push_back({s.x, s.y, 0.35f});
            blip(1, 320, 0.1f);
            continue;
        }
        if (s.y >= STRIKE_Y) {
            s.live = false;
            if (s.x >= WELL_L && s.x <= WELL_R) {
                through_++;
                wellHp_--;
                puffs_.push_back({s.x, STRIKE_Y - 8, 0.45f});
                blip(2, 70, 0.16f);
                if (wellHp_ <= 0) {
                    wellHp_ = 0;
                    mode_ = Mode::Defeat;
                    over_ = true;
                    won_ = false;
                    reason_ = "THE WELL FELL";
                    t_ = 0;
                    return;
                }
            }
        }
    }

    for (Puff& p : puffs_) p.t -= dt;
    puffs_.erase(std::remove_if(puffs_.begin(), puffs_.end(), [](const Puff& p) { return p.t <= 0; }), puffs_.end());

    bool any = false;
    for (const Stone& s : stones_)
        if (s.live) any = true;
    if (!any && spawnI_ >= int(spawns_.size())) {
        if (wave_ >= 2) {
            mode_ = Mode::Victory;
            over_ = true;
            won_ = wellHp_ > 0;
            reason_ = won_ ? "THE WELL STANDS THROUGH THREE WAVES" : "THE WELL FELL";
            t_ = 0;
        } else {
            mode_ = Mode::Banner;
            bannerT_ = 0;
            t_ = 0;
        }
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

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
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
    sys_->vdp.sprite(s);
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();

    const uint16_t top = gs::rgb4(3, 4, 8);
    const uint16_t hor = gs::rgb4(12, 8, 4);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H - 1);
        auto mix = [&](int a, int b) { return int(std::lround(a + (b - a) * std::min(1.0f, u * 1.15f))); };
        int r = mix((top >> 8) & 15, (hor >> 8) & 15);
        int g = mix((top >> 4) & 15, (hor >> 4) & 15);
        int b = mix(top & 15, hor & 15);
        vdp.lineBackdrop[y] = gs::rgb4(r, g, b);
        vdp.lineFog[y] = 0;
        gs::RoadLine& rd = vdp.road[y];
        rd = {};
        if (y >= 150) {
            rd.on = true;
            rd.cx = 160;
            rd.hw = 420;
            rd.v = float(y) * 0.35f + t_ * 2.0f;
            rd.pal = PAL_YARD;
            rd.style = 0;
            rd.band = (y / 8) & 1;
            rd.left = 0;
            rd.right = 0;
        }
    }
    vdp.setFogColor(gs::rgb4(8, 6, 4));

    bool fallen = mode_ == Mode::Defeat;
    spr(art_.granary, 78, 78, 86, PAL_GRANARY);
    spr(fallen ? art_.wellFall : art_.well, 168, fallen ? 132 : 108, fallen ? 64 : 100, PAL_WELL);
    if (!fallen) spr(art_.bucket, 168, 118 + std::sin(t_ * 1.3f) * 3.0f, 14, PAL_WELL);

    for (const Stone& s : stones_)
        if (s.live) spr(art_.stone, s.x, s.y, 16, PAL_STONE);
    for (const Puff& p : puffs_) spr(art_.puff, p.x, p.y, 18 + (0.35f - p.t) * 20.0f, PAL_FX);

    if (mode_ == Mode::Watch || mode_ == Mode::Banner || mode_ == Mode::Title) {
        float gy = 158;
        spr(art_.guard, px_, gy, 48, PAL_GUARD, px_ > 180);
        if (swing_ > 0) spr(art_.stave, px_, gy - 22, 18, PAL_FX);
    }

    char line[64];
    if (mode_ == Mode::Title) {
        hudC(4, "GRANARY WELL", 3);
        hudC(8, "KEEP THE WELL STANDING", 1);
        hudC(10, "THREE WAVES", 1);
        hudC(16, "LEFT RIGHT MOVE", 2);
        hudC(18, "A SWING THE STAVE", 2);
        if (int(t_ * 2) % 2 == 0) hudC(23, "START", 3);
    } else if (mode_ == Mode::Watch || mode_ == Mode::Banner) {
        std::snprintf(line, sizeof line, "WAVE %d/3", wave_ + 1);
        hud(1, 1, line, 1);
        std::snprintf(line, sizeof line, "WELL %d", wellHp_);
        hud(30, 1, line, wellHp_ > 1 ? 5 : 4);
        if (mode_ == Mode::Banner) hudC(12, "THE WELL HOLDS", 3);
    } else if (mode_ == Mode::Victory) {
        hudC(8, "THE WELL STANDS", 3);
        hudC(10, "THREE WAVES", 5);
        std::snprintf(line, sizeof line, "STONES %d", stopped_);
        hudC(14, line, 1);
    } else if (mode_ == Mode::Defeat) {
        hudC(8, "THE WELL FELL", 4);
        hudC(11, "THE WATCH IS LOST", 1);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.8f);
    sys.apu.noise(0, 0);
    if (bot_) bootWatch();
    else {
        mode_ = Mode::Title;
        t_ = 0;
        px_ = 150;
        wellHp_ = WELL_MAX;
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        px_ = 150.0f + std::sin(t_ * 0.7f) * 36.0f;
        if (pad.pressed(gs::BTN_START) || (bot_ && t_ > 0.2f)) {
            bootWatch();
        }
    } else if (mode_ == Mode::Watch) {
        update(DT);
    } else if (mode_ == Mode::Banner) {
        bannerT_ += DT;
        bool go = bot_ ? bannerT_ > 0.55f : (pad.pressed(gs::BTN_START) || bannerT_ > 1.4f);
        if (go) {
            wave_++;
            prepareWave();
            mode_ = Mode::Watch;
            t_ = 0;
        }
    } else if (mode_ == Mode::Victory || mode_ == Mode::Defeat) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Title;
            t_ = 0;
            over_ = false;
        }
    }

    if (t_ > 0.05f) {
        sys.apu.tone(0, 0, 0);
        sys.apu.tone(1, 0, 0);
        sys.apu.tone(2, 0, 0);
    }
    draw();
}

}  // namespace granary

#include "game/mill.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace mill {
namespace {
constexpr float DT = 1.0f / 60.0f;
constexpr int WELL_X = 160;
constexpr float HIT_X = 22;
}

float Game::laneY(int lane) { return 128.0f + lane * 28.0f; }

float Game::rnd() {
    rng_ = rng_ * 1664525u + 1013904223u;
    return (rng_ >> 8) * (1.0f / 16777216.0f);
}

const char* Game::waveName() const {
    if (wave_ <= 0) return "CROWS ON THE RIDGE";
    if (wave_ == 1) return "BOARS IN THE FURROW";
    return "NIGHT CREW";
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Fight) return 1;
    if (mode_ == Mode::Banner) return 2;
    return 3;
}

void Game::tone(float f, float v, float hold) {
    sys_->apu.tone(0, f, v);
    beep_ = hold;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.apu.setMaster(0.45f);
    mode_ = Mode::Title;
    t_ = 0;
}

void Game::begin() {
    wave_ = 0;
    score_ = 0;
    wellHp_ = wellMax_;
    over_ = false;
    won_ = false;
    px_ = 108;
    lane_ = 1;
    face_ = 1;
    swingT_ = 0;
    cool_ = 0;
    throwC_ = 0;
    foes_.clear();
    sacks_.clear();
    puffs_.clear();
    armWave();
    mode_ = Mode::Banner;
    clock_ = 1.3f;
    t_ = 0;
}

void Game::armWave() {
    foes_.clear();
    sacks_.clear();
    spawns_.clear();
    spawned_ = 0;
    auto add = [&](float when, Kind k, int lane, bool left) { spawns_.push_back({when, k, lane, left}); };
    if (wave_ == 0) {
        for (int i = 0; i < 8; i++) add(0.4f + i * 1.85f, Kind::Crow, i % 3, (i % 2) == 0);
    } else if (wave_ == 1) {
        for (int i = 0; i < 7; i++) add(0.35f + i * 2.35f, Kind::Boar, (i * 2) % 3, (i % 2) == 0);
    } else {
        for (int i = 0; i < 8; i++) add(0.3f + i * 2.15f, Kind::Raider, (i + 1) % 3, (i % 2) != 0);
    }
}

void Game::swing() {
    if (cool_ > 0) return;
    cool_ = 0.32f;
    swingT_ = 0.2f;
    tone(180, 0.08f, 0.05f);
    for (auto& f : foes_) {
        if (f.lane != lane_ || f.hp <= 0) continue;
        float dx = f.x - px_;
        if (face_ < 0 && dx > 4) continue;
        if (face_ > 0 && dx < -4) continue;
        if (std::fabs(dx) > 46) continue;
        f.hp -= 1;
        f.flash = 0.12f;
        f.x += face_ * 10.0f;
        puffs_.push_back({f.x, laneY(f.lane) - 10, 0.25f});
        if (f.hp <= 0) {
            score_ += f.kind == Kind::Crow ? 50 : f.kind == Kind::Boar ? 100 : 150;
            tone(420, 0.1f, 0.07f);
        }
    }
}

void Game::botThink() {
    wantSwing_ = false;
    wantThrow_ = false;
    wantLane_ = 0;
    const Foe* best = nullptr;
    float bestD = 1e9f;
    for (const auto& f : foes_) {
        if (f.hp <= 0) continue;
        float d = std::fabs(f.x - WELL_X);
        if (d < bestD) {
            bestD = d;
            best = &f;
        }
    }
    if (!best) {
        if (px_ < WELL_X - 8) face_ = 1;
        else if (px_ > WELL_X + 8) face_ = -1;
        return;
    }
    int dl = best->lane - lane_;
    wantLane_ = dl > 0 ? 1 : dl < 0 ? -1 : 0;
    if (best->x < px_ - 6) face_ = -1;
    else if (best->x > px_ + 6) face_ = 1;
    if (best->lane == lane_ && std::fabs(best->x - px_) < 40) wantSwing_ = true;
    else if (best->lane == lane_ && std::fabs(best->x - px_) < 120 && throwC_ <= 0) wantThrow_ = true;
}

void Game::update(float dt) {
    t_ += dt;
    if (beep_ > 0) {
        beep_ -= dt;
        if (beep_ <= 0) sys_->apu.tone(0, 0, 0);
    }
    if (fan_) {
        static const float notes[] = {392, 494, 587, 784};
        noteT_ -= dt;
        if (noteT_ <= 0 && noteI_ < 4) {
            sys_->apu.tone(1, notes[noteI_], 0.12f);
            noteI_++;
            noteT_ = 0.16f;
        }
        if (noteI_ >= 4 && noteT_ <= 0) {
            sys_->apu.tone(1, 0, 0);
            fan_ = false;
        }
    }

    if (mode_ == Mode::Title) return;

    if (mode_ == Mode::Banner) {
        clock_ -= dt;
        if (clock_ <= 0) {
            mode_ = Mode::Fight;
            clock_ = 0;
        }
        return;
    }
    if (mode_ == Mode::Victory || mode_ == Mode::Defeat) return;

    if (cool_ > 0) cool_ -= dt;
    if (throwC_ > 0) throwC_ -= dt;
    if (swingT_ > 0) swingT_ -= dt;

    float axis = 0;
    if (bot_) {
        botThink();
        if (wantLane_ < 0 && lane_ > 0) lane_--;
        if (wantLane_ > 0 && lane_ < 2) lane_++;
        if (wantSwing_) swing();
        const Foe* best = nullptr;
        float bestD = 1e9f;
        for (const auto& f : foes_) {
            if (f.hp <= 0) continue;
            float d = std::fabs(f.x - float(WELL_X));
            if (d < bestD) {
                bestD = d;
                best = &f;
            }
        }
        if (best && best->lane == lane_) axis = best->x < px_ ? -1 : 1;
        if (wantThrow_ && throwC_ <= 0 && swingT_ <= 0) {
            throwC_ = 0.45f;
            sacks_.push_back({px_ + face_ * 12, face_ * 210.0f, lane_});
            tone(260, 0.06f, 0.04f);
        }
    } else {
        const gs::Pad& p = sys_->pad;
        if (p.down(gs::BTN_LEFT) || p.axisX < -0.3f) {
            axis = -1;
            face_ = -1;
        }
        if (p.down(gs::BTN_RIGHT) || p.axisX > 0.3f) {
            axis = 1;
            face_ = 1;
        }
        if (p.pressed(gs::BTN_UP) && lane_ > 0) lane_--;
        if (p.pressed(gs::BTN_DOWN) && lane_ < 2) lane_++;
        if (p.down(gs::BTN_A) || p.pressed(gs::BTN_A)) swing();
        if (p.pressed(gs::BTN_B) && throwC_ <= 0) {
            throwC_ = 0.45f;
            sacks_.push_back({px_ + face_ * 12, face_ * 210.0f, lane_});
            tone(260, 0.06f, 0.04f);
        }
    }
    px_ = std::clamp(px_ + axis * 170.0f * dt, 28.0f, 292.0f);

    for (auto& s : spawns_) {
        if (s.t < 0) continue;
        s.t -= dt;
        if (s.t > 0) continue;
        s.t = -1;
        Foe f;
        f.kind = s.kind;
        f.lane = s.lane;
        f.left = s.left;
        f.x = s.left ? -12.0f : 332.0f;
        f.hp = s.kind == Kind::Crow ? 1 : 2;
        f.flash = 0;
        foes_.push_back(f);
        spawned_++;
    }

    auto speedOf = [](Kind k) {
        if (k == Kind::Crow) return 34.0f;
        if (k == Kind::Boar) return 46.0f;
        return 40.0f;
    };
    for (auto& f : foes_) {
        if (f.hp <= 0) continue;
        if (f.flash > 0) f.flash -= dt;
        float dir = f.x < WELL_X ? 1.0f : -1.0f;
        f.x += dir * speedOf(f.kind) * dt;
        if (std::fabs(f.x - WELL_X) < HIT_X) {
            f.hp = 0;
            wellHp_ -= 1;
            puffs_.push_back({float(WELL_X), laneY(f.lane) - 16, 0.35f});
            tone(90, 0.16f, 0.12f);
            if (wellHp_ <= 0) {
                wellHp_ = 0;
                mode_ = Mode::Defeat;
                over_ = true;
                won_ = false;
                tone(70, 0.18f, 0.4f);
            }
        }
    }
    foes_.erase(std::remove_if(foes_.begin(), foes_.end(), [](const Foe& f) { return f.hp <= 0; }), foes_.end());

    for (auto& s : sacks_) {
        s.x += s.vx * dt;
        for (auto& f : foes_) {
            if (f.hp <= 0 || f.lane != s.lane) continue;
            if (std::fabs(f.x - s.x) < 16) {
                f.hp -= 1;
                f.flash = 0.1f;
                s.x = -999;
                puffs_.push_back({f.x, laneY(f.lane) - 8, 0.2f});
                if (f.hp <= 0) score_ += 40;
                break;
            }
        }
    }
    sacks_.erase(std::remove_if(sacks_.begin(), sacks_.end(), [](const Sack& s) { return s.x < -20 || s.x > 340; }), sacks_.end());
    foes_.erase(std::remove_if(foes_.begin(), foes_.end(), [](const Foe& f) { return f.hp <= 0; }), foes_.end());

    for (auto& p : puffs_) p.t -= dt;
    puffs_.erase(std::remove_if(puffs_.begin(), puffs_.end(), [](const Puff& p) { return p.t <= 0; }), puffs_.end());

    if (spawned_ == int(spawns_.size()) && foes_.empty() && wellHp_ > 0) {
        if (wave_ >= 2) {
            mode_ = Mode::Victory;
            over_ = true;
            won_ = true;
            fan_ = true;
            noteI_ = 0;
            noteT_ = 0;
        } else {
            wave_++;
            armWave();
            mode_ = Mode::Banner;
            clock_ = 1.5f;
            tone(523, 0.1f, 0.15f);
        }
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    const float adv = 16.0f * scale;
    float left = x - float(s.size()) * adv * 0.5f;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, left + i * adv + g.w * scale * 0.5f, y, g.h * scale, pal, false);
    }
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || row < 0 || row > 27 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.roadTime = int(t_ * 60);

    const int horizon = 78;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < horizon) {
            float u = y / float(horizon);
            int r = int(4 + u * 8);
            int g = int(7 + u * 5);
            int b = int(12 - u * 4);
            v.lineBackdrop[y] = gs::rgb4(r, g, b);
            v.lineFog[y] = 0;
            v.road[y].on = false;
            continue;
        }
        float row = float(y - horizon) + 1.0f;
        gs::RoadLine& rd = v.road[y];
        rd.on = true;
        rd.cx = 160;
        rd.hw = 28.0f + row * 0.85f;
        rd.v = 200.0f + row * 3.0f;
        rd.pal = PAL_YARD;
        rd.band = (int(std::floor(rd.v / 40.0f)) & 1) ? 1 : 0;
        rd.style = 0;
        rd.left = rd.right = 0;
        v.lineFog[y] = 0;
        v.lineBackdrop[y] = gs::rgb4(5, 8, 3);
    }

    // Near sprites first so they cover the mill and the well.
    bool swinging = swingT_ > 0;
    if (mode_ != Mode::Title) {
        spr(art_.man[swinging ? 1 : 0], px_, laneY(lane_) - 8, 44, PAL_MAN, face_ < 0);
        for (const auto& f : foes_) {
            const gs::Mipped* m = &art_.crow;
            int pal = PAL_CROW;
            float h = 26;
            if (f.kind == Kind::Boar) {
                m = &art_.boar;
                pal = PAL_BOAR;
                h = 30;
            } else if (f.kind == Kind::Raider) {
                m = &art_.raider;
                pal = PAL_RAIDER;
                h = 42;
            }
            bool flip = f.x > WELL_X;
            spr(*m, f.x, laneY(f.lane) - 6, h, pal, flip);
        }
        for (const auto& s : sacks_) spr(art_.sack, s.x, laneY(s.lane) - 16, 14, PAL_FX, s.vx < 0);
        for (const auto& p : puffs_) spr(art_.puff, p.x, p.y, 18 + (0.3f - p.t) * 20, PAL_FX, false);
    }
    float wellH = mode_ == Mode::Defeat ? 36 : 78;
    spr(art_.well, WELL_X, 150, wellH, PAL_WELL, false);
    int sail = int(t_ * 6) & 1;
    spr(art_.mill[sail], 62, 108, 130, PAL_MILL, false);

    if (mode_ == Mode::Title) {
        text("S3 MILLWELL", 160, 48, 1.15f, PAL_HUD);
        text("KEEP THE WELL", 160, 168, 0.7f, PAL_WELL);
        text("START", 160, 196, 0.6f, PAL_MAN);
    } else if (mode_ == Mode::Banner) {
        char buf[40];
        std::snprintf(buf, sizeof buf, "WAVE %d", wave_ + 1);
        text(buf, 160, 40, 1.0f, PAL_HUD);
        text(waveName(), 160, 64, 0.55f, PAL_MAN);
    } else if (mode_ == Mode::Victory) {
        text("THE WELL STANDS", 160, 36, 0.7f, PAL_HUD);
        text("THREE WAVES", 160, 58, 0.55f, PAL_MAN);
    } else if (mode_ == Mode::Defeat) {
        text("THE WELL FALLS", 160, 36, 0.75f, PAL_HUD);
    }

    if (mode_ != Mode::Title) {
        char buf[48];
        std::snprintf(buf, sizeof buf, "WELL %d", wellHp_);
        hud(1, 1, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "WAVE %d", wave_ + 1);
        hudC(1, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "%d", score_);
        hud(36, 1, buf, PAL_HUD);
        hud(1, 26, "A SWING  B SACK", PAL_HUD);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (mode_ == Mode::Title) {
        bool go = bot_ && t_ > 0.35f;
        if (!bot_ && (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A))) go = true;
        if (go) begin();
    } else if (!bot_ && mode_ == Mode::Fight && sys.pad.pressed(gs::BTN_START)) {
        // A short rest. The next start resumes. Kept as a banner so the pad still works.
    }
    update(DT);
    draw();
}

}  // namespace mill

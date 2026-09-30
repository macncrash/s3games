#include "game/well.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace culvertwell {

namespace {
constexpr float kZ0 = 8.0f;
constexpr float kStrike = 2.55f;
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Banner) return 2;
    if (mode_ == Mode::Fell) return 3;
    if (mode_ == Mode::Stood) return 4;
    return 1;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.hudEnabled = true;
    mode_ = Mode::Title;
    t_ = 0;
}

void Game::begin() {
    wave_ = 0;
    hp_ = 5;
    lane_ = 1;
    over_ = false;
    won_ = false;
    t_ = 0;
    mode_ = Mode::Play;
    armWave();
}

void Game::armWave() {
    spawns_.clear();
    threats_.clear();
    spawnAt_ = 0;
    waveT_ = 0;
    cool_ = 0.2f;
    swing_ = 0;
    auto add = [&](float t, int lane, int hp, Kind k) { spawns_.push_back({t, k, lane, hp}); };
    if (wave_ == 0) {
        add(0.5f, 1, 1, Kind::Sapper);
        add(2.8f, 0, 1, Kind::Sapper);
        add(5.1f, 2, 1, Kind::Barrel);
        add(7.5f, 1, 1, Kind::Sapper);
        add(9.9f, 0, 1, Kind::Barrel);
        add(12.3f, 2, 1, Kind::Sapper);
    } else if (wave_ == 1) {
        add(0.45f, 0, 1, Kind::Sapper);
        add(2.6f, 2, 2, Kind::Ram);
        add(5.2f, 1, 1, Kind::Barrel);
        add(7.5f, 0, 1, Kind::Sapper);
        add(9.8f, 2, 1, Kind::Barrel);
        add(12.2f, 1, 2, Kind::Ram);
        add(14.8f, 0, 1, Kind::Sapper);
    } else {
        add(0.4f, 1, 1, Kind::Sapper);
        add(2.5f, 0, 1, Kind::Barrel);
        add(4.8f, 2, 2, Kind::Ram);
        add(7.3f, 1, 1, Kind::Sapper);
        add(9.6f, 0, 2, Kind::Ram);
        add(12.1f, 2, 1, Kind::Barrel);
        add(14.4f, 1, 1, Kind::Sapper);
        add(16.7f, 0, 1, Kind::Barrel);
    }
}

void Game::botAct(int& lane, bool& swing) {
    if (mode_ == Mode::Title) return;
    if (mode_ != Mode::Play) return;
    int best = -1;
    float bz = 99.f;
    for (int i = 0; i < int(threats_.size()); i++) {
        const Threat& f = threats_[i];
        if (f.gone) continue;
        if (f.z < bz) {
            bz = f.z;
            best = i;
        }
    }
    if (best < 0) return;
    lane = threats_[best].lane;
    if (lane_ == lane && bz < kStrike && bz > 0.25f) swing = true;
}

void Game::update(float dt) {
    t_ += dt;
    scroll_ += dt * (40.f + wave_ * 18.f);
    if (shake_ > 0) shake_ -= dt;
    if (cool_ > 0) cool_ -= dt;
    if (swing_ > 0) swing_ -= dt;

    if (mode_ == Mode::Title) {
        bool go = bot_ && t_ > 0.4f;
        if (!bot_ && (sys_->pad.pressed(gs::BTN_START) || sys_->pad.pressed(gs::BTN_A))) go = true;
        if (go) begin();
        return;
    }
    if (mode_ == Mode::Fell || mode_ == Mode::Stood) return;
    if (mode_ == Mode::Banner) {
        banner_ -= dt;
        if (banner_ <= 0) {
            if (wave_ >= 2) {
                mode_ = Mode::Stood;
                won_ = true;
                over_ = true;
                sys_->apu.tone(0, 523.f, 0.12f);
                sys_->apu.tone(1, 659.f, 0.1f);
            } else {
                wave_++;
                mode_ = Mode::Play;
                armWave();
            }
        }
        return;
    }

    int want = lane_;
    bool swing = false;
    if (bot_) {
        botAct(want, swing);
    } else {
        if (sys_->pad.pressed(gs::BTN_LEFT)) want = std::max(0, lane_ - 1);
        if (sys_->pad.pressed(gs::BTN_RIGHT)) want = std::min(2, lane_ + 1);
        swing = sys_->pad.pressed(gs::BTN_A) || sys_->pad.pressed(gs::BTN_B);
    }
    lane_ = want;

    float speed = 1.05f + wave_ * 0.28f;
    while (spawnAt_ < int(spawns_.size()) && spawns_[spawnAt_].t <= waveT_) {
        const Spawn& s = spawns_[spawnAt_++];
        threats_.push_back({s.kind, s.lane, s.hp, kZ0, false});
    }
    waveT_ += dt;

    if (swing && cool_ <= 0) {
        cool_ = 0.34f;
        swing_ = 0.18f;
        int hit = -1;
        float hz = 99.f;
        for (int i = 0; i < int(threats_.size()); i++) {
            Threat& f = threats_[i];
            if (f.gone || f.lane != lane_) continue;
            if (f.z < kStrike && f.z > 0.2f && f.z < hz) {
                hz = f.z;
                hit = i;
            }
        }
        if (hit >= 0) {
            threats_[hit].hp--;
            sys_->apu.noiseBurst(0.35f, 900.f, 0.08f);
            if (threats_[hit].hp <= 0) threats_[hit].gone = true;
        }
    }

    for (Threat& f : threats_) {
        if (f.gone) continue;
        f.z -= speed * dt;
        if (f.z <= 0.12f) {
            f.gone = true;
            int dmg = f.kind == Kind::Ram ? 2 : 1;
            hp_ -= dmg;
            shake_ = 0.3f;
            sys_->apu.noiseBurst(0.55f, 220.f, 0.25f);
            if (hp_ <= 0) {
                hp_ = 0;
                mode_ = Mode::Fell;
                over_ = true;
                won_ = false;
                return;
            }
        }
    }
    threats_.erase(std::remove_if(threats_.begin(), threats_.end(), [](const Threat& f) { return f.gone; }),
                   threats_.end());

    if (spawnAt_ >= int(spawns_.size()) && threats_.empty()) {
        mode_ = Mode::Banner;
        banner_ = 1.35f;
        sys_->apu.tone(0, 392.f, 0.1f);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    update(1.f / 60.f);
    draw();
}

void Game::project(int lane, float z, float& sx, float& sy, float& scale) const {
    float u = std::clamp(z / kZ0, 0.f, 1.f);
    float near = 1.f - u;
    sx = 160.f + float(lane - 1) * (24.f + near * 72.f);
    sy = 86.f + near * 96.f;
    scale = 16.f + near * 50.f;
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

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    const float adv = 16.f * scale;
    float w = float(s.size()) * adv;
    x -= w * 0.5f;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + i * adv + g.w * scale * 0.5f, y, g.h * scale, pal, false, 0);
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
    v.roadTime = int(t_ * 60.f);
    float sh = shake_ > 0 ? std::sin(shake_ * 90.f) * 4.f : 0;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float k = float(y) / float(gs::SCREEN_H);
        v.lineBackdrop[y] = gs::rgb4(int(2 + 3 * k), int(3 + 2 * k), int(5 + k));
        v.lineFog[y] = y < 80 ? uint8_t(10 - y / 10) : 0;
        gs::RoadLine& rd = v.road[y];
        rd = gs::RoadLine{};
        if (y >= 92) {
            float t = float(y - 92) / 132.f;
            rd.on = true;
            rd.cx = 160.f + sh;
            rd.hw = 18.f + t * t * 148.f;
            rd.v = scroll_ + (1.f - t) * 1800.f;
            rd.pal = PAL_ROAD;
            rd.style = gs::ROAD_MUD;
            rd.band = (int(rd.v / 70.f) & 1) ? 1 : 0;
            rd.left = gs::GROUND_LAND;
            rd.right = gs::GROUND_LAND;
        }
    }

    spr(art_.archL, 28 + sh * 0.2f, 128, 200, PAL_STONE, false, 0);
    spr(art_.archR, 292 + sh * 0.2f, 128, 200, PAL_STONE, false, 0);
    spr(art_.lintel, 160, 16, 28, PAL_STONE, false, 0);

    std::vector<int> order(threats_.size());
    for (int i = 0; i < int(order.size()); i++) order[i] = i;
    std::sort(order.begin(), order.end(), [&](int a, int b) { return threats_[a].z > threats_[b].z; });
    for (int i : order) {
        const Threat& f = threats_[i];
        float sx, sy, sc;
        project(f.lane, f.z, sx, sy, sc);
        int fog = int(std::clamp(f.z / kZ0, 0.f, 1.f) * 10.f);
        if (f.kind == Kind::Barrel) {
            spr(art_.barrel, sx + sh, sy, sc * 0.7f, PAL_BARREL, false, fog);
        } else if (f.kind == Kind::Ram) {
            spr(art_.ram, sx + sh, sy - sc * 0.1f, sc * 0.85f, PAL_RAM, f.lane < 1, fog);
        } else {
            spr(art_.sapper, sx + sh, sy - sc * 0.15f, sc, PAL_FOE, f.lane < 1, fog);
        }
    }

    float px, py, ps;
    project(lane_, 1.15f, px, py, ps);
    const gs::Mipped& body = swing_ > 0 ? art_.swing : art_.keeper;
    spr(body, px + sh, py - 8, 62, PAL_KEEPER, false, 0);

    int cracks = hp_ >= 4 ? 0 : hp_ >= 2 ? 1 : 2;
    spr(art_.well[cracks], 160 + sh, 196, 78, PAL_WELL, false, 0);

    if (mode_ == Mode::Title) {
        text("S3 CULVERTWELL", 160, 70, 0.62f, PAL_HUD);
        text("KEEP THE WELL STANDING", 160, 100, 0.40f, PAL_WELL);
        text("ARROWS POST    A SWING", 160, 128, 0.36f, PAL_HUD);
        text("THREE WAVES  THEN IT STANDS", 160, 150, 0.34f, PAL_STONE);
        if (int(t_ * 2) % 2 == 0) text("START", 160, 180, 0.5f, PAL_WELL);
    } else if (mode_ == Mode::Banner) {
        text(wave_ >= 2 ? "THE WELL STANDS" : "WAVE HELD", 160, 78, 0.7f, PAL_WELL);
    } else if (mode_ == Mode::Fell) {
        text("THE WELL FELL", 160, 78, 0.7f, PAL_FOE);
    } else if (mode_ == Mode::Stood) {
        text("THE WELL STANDS", 160, 78, 0.62f, PAL_WELL);
        text("THREE WAVES HELD", 160, 108, 0.4f, PAL_HUD);
    }

    char line[40];
    std::snprintf(line, sizeof line, "WAVE %d", wave_ + 1);
    hud(1, 1, line, PAL_HUD);
    std::snprintf(line, sizeof line, "WELL %d", hp_);
    hud(32, 1, line, PAL_HUD);
    const char* posts[] = {"LEFT WALL", "THE WELL", "RIGHT WALL"};
    if (mode_ == Mode::Play) hudC(26, posts[lane_], swing_ > 0 ? PAL_WELL : PAL_HUD);
    else if (mode_ == Mode::Title) hudC(26, "THE CULVERT IS YOURS", PAL_HUD);
}

}  // namespace culvertwell

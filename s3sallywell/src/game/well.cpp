#include "game/well.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace sally {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr int WAVES = 3;
constexpr float HORIZON = 70.0f;
constexpr float SPAN = 150.0f;

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

}  // namespace

void Game::project(float wx, float z, float& sx, float& sy, float& sc) const {
    float zz = std::clamp(z, 0.0f, 1.0f);
    float near = std::pow(1.0f - zz, 0.78f);
    sy = HORIZON + near * SPAN;
    float t = std::clamp((sy - HORIZON) / SPAN, 0.0f, 1.0f);
    float hw = 18.0f + t * t * 148.0f;
    sx = 160.0f + wx * hw * 0.86f;
    sc = 12.0f + near * 58.0f;
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

void Game::spr(const gs::Mipped& m, float cx, float feet, float h, int pal, bool flip, int fog, bool shadow) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet - s.h));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    const float adv = 16.0f * scale;
    float w = float(s.size()) * adv;
    x -= w * 0.5f;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + i * adv + g.w * scale * 0.5f, y + g.h * scale, g.h * scale, pal, false, 0, false);
    }
}

void Game::startWatch() {
    wave_ = 0;
    score_ = 0;
    well_ = 8;
    over_ = false;
    won_ = false;
    px_ = 0;
    strikeCd_ = tossCd_ = swing_ = shake_ = 0;
    tosses_.clear();
    sparks_.clear();
    beginWave();
    mode_ = Mode::Watch;
}

void Game::beginWave() {
    spawns_.clear();
    raiders_.clear();
    spawned_ = 0;
    clock_ = 0;
    const int n = 8 + wave_ * 4;
    const float gap = 1.28f - wave_ * 0.16f;
    const float spd = 0.15f + wave_ * 0.045f;
    const float lane[3] = {-0.46f, 0.46f, 0.0f};
    for (int i = 0; i < n; i++) {
        Spawn s;
        s.t = 0.7f + i * gap;
        s.x = lane[(i + wave_) % 3];
        if (wave_ == 2 && (i % 5) == 4) s.x = (i & 1) ? 0.62f : -0.62f;
        s.spd = spd + ((i % 3) == 2 ? 0.03f : 0.0f);
        spawns_.push_back(s);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(3, 3, 6));
    sys.vdp.hudEnabled = true;
    if (bot_) startWatch();
    else mode_ = Mode::Title;
}

void Game::layCourt() {
    gs::VDP& vdp = sys_->vdp;
    const uint16_t top = gs::rgb4(1, 2, 5);
    const uint16_t hor = gs::rgb4(8, 5, 4);
    const float jx = (shake_ > 0 ? std::sin(shake_ * 80.0f) * 3.0f : 0.0f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H - 1);
        vdp.lineBackdrop[y] = lerpC(top, hor, std::min(1.0f, u * 1.35f));
        vdp.lineFog[y] = y < 78 ? uint8_t(6 - y / 16) : 0;
        gs::RoadLine& r = vdp.road[y];
        if (y < int(HORIZON)) {
            r.on = false;
            continue;
        }
        float t = (y - HORIZON) / SPAN;
        t = std::clamp(t, 0.0f, 1.0f);
        r.on = true;
        r.cx = 160.0f + jx;
        r.hw = 18.0f + t * t * 148.0f;
        r.v = (1.0f - t) * 820.0f + march_;
        r.pal = PAL_ROAD;
        r.band = (int(std::floor(r.v / 48.0f)) & 1) ? 1 : 0;
        r.style = 0;
        r.left = r.right = 0;
    }
}

void Game::killAt(float x, float z, float rx, float rz, int pts) {
    (void)rz;
    for (Raider& r : raiders_) {
        if (!r.alive) continue;
        if (std::fabs(r.x - x) <= rx && r.z <= z && r.z > 0.03f) {
            r.alive = false;
            r.flash = 0.2f;
            score_ += pts;
            float sx, sy, sc;
            project(r.x, r.z, sx, sy, sc);
            sparks_.push_back({sx, sy - sc * 0.6f, 0.28f});
            sys_->apu.noiseBurst(0.35f, 1400.0f, 0.12f);
            return;
        }
    }
}

void Game::botIntent(bool& left, bool& right, bool& strike, bool& toss) {
    const Raider* best = nullptr;
    for (const Raider& r : raiders_) {
        if (!r.alive) continue;
        if (!best || r.z < best->z) best = &r;
    }
    if (!best) return;
    float dx = best->x - px_;
    if (dx < -0.03f) left = true;
    else if (dx > 0.03f) right = true;
    if (std::fabs(dx) < 0.2f && best->z < 0.42f) strike = true;
    if (std::fabs(dx) < 0.22f && best->z > 0.38f && best->z < 0.9f) toss = true;
}

void Game::update() {
    bool left = false, right = false, strike = false, toss = false;
    if (bot_) {
        botIntent(left, right, strike, toss);
    } else {
        const gs::Pad& p = sys_->pad;
        left = p.down(gs::BTN_LEFT) || p.axisX < -0.3f;
        right = p.down(gs::BTN_RIGHT) || p.axisX > 0.3f;
        strike = p.down(gs::BTN_A) || p.down(gs::BTN_C);
        toss = p.pressed(gs::BTN_B) || p.pressed(gs::BTN_X);
    }
    float dir = (right ? 1.0f : 0.0f) - (left ? 1.0f : 0.0f);
    px_ = std::clamp(px_ + dir * 2.6f * DT, -0.92f, 0.92f);
    if (strikeCd_ > 0) strikeCd_ -= DT;
    if (tossCd_ > 0) tossCd_ -= DT;
    if (swing_ > 0) swing_ -= DT;
    if (shake_ > 0) shake_ -= DT;

    if (strike && strikeCd_ <= 0) {
        strikeCd_ = 0.22f;
        swing_ = 0.12f;
        sys_->apu.tone(0, 220.0f, 0.06f);
        killAt(px_, 0.5f, 0.28f, 0.5f, 100);
    }
    if (toss && tossCd_ <= 0) {
        tossCd_ = 0.42f;
        tosses_.push_back({px_, 0.16f});
        sys_->apu.tone(1, 440.0f, 0.04f);
    }

    clock_ += DT;
    march_ += 18.0f * DT;
    while (spawned_ < int(spawns_.size()) && clock_ >= spawns_[spawned_].t) {
        const Spawn& s = spawns_[spawned_++];
        raiders_.push_back({s.x, 1.02f, s.spd, 0.0f, true});
    }

    bool any = false;
    for (Raider& r : raiders_) {
        if (r.flash > 0) r.flash -= DT;
        if (!r.alive) continue;
        any = true;
        r.z -= r.spd * DT;
        if (r.z <= 0.045f) {
            r.alive = false;
            well_ -= 1;
            shake_ = 0.35f;
            score_ = std::max(0, score_ - 25);
            sys_->apu.noiseBurst(0.6f, 180.0f, 0.28f);
            if (well_ <= 0) {
                well_ = 0;
                mode_ = Mode::Defeat;
                over_ = true;
                won_ = false;
                banner_ = 0;
                return;
            }
        }
    }
    for (auto it = tosses_.begin(); it != tosses_.end();) {
        it->z += 0.95f * DT;
        bool hit = false;
        for (Raider& r : raiders_) {
            if (!r.alive) continue;
            if (std::fabs(r.x - it->x) < 0.16f && std::fabs(r.z - it->z) < 0.07f) {
                r.alive = false;
                score_ += 100;
                float sx, sy, sc;
                project(r.x, r.z, sx, sy, sc);
                sparks_.push_back({sx, sy - 20, 0.25f});
                sys_->apu.noiseBurst(0.3f, 900.0f, 0.1f);
                hit = true;
                break;
            }
        }
        if (hit || it->z > 1.15f) it = tosses_.erase(it);
        else ++it;
    }
    for (auto it = sparks_.begin(); it != sparks_.end();) {
        it->t -= DT;
        it->y -= 18.0f * DT;
        if (it->t <= 0) it = sparks_.erase(it);
        else ++it;
    }

    if (!any && spawned_ >= int(spawns_.size())) {
        score_ += 200;
        if (wave_ + 1 >= WAVES) {
            mode_ = Mode::Victory;
            over_ = true;
            won_ = true;
            banner_ = 0;
            sys_->apu.tone(0, 523.0f, 0.1f);
            sys_->apu.tone(1, 659.0f, 0.1f);
        } else {
            mode_ = Mode::Banner;
            banner_ = 1.3f;
            sys_->apu.tone(0, 392.0f, 0.08f);
        }
    }
}

void Game::draw() {
    layCourt();
    sys_->vdp.clearSprites();
    sys_->vdp.HUD.clear();

    float gx, gy, gs;
    project(px_, 0.1f, gx, gy, gs);
    if (swing_ > 0) spr(art_.spark, gx + 22, gy - 36, 22, PAL_FX, false, 0, false);
    for (const Spark& s : sparks_) spr(art_.spark, s.x, s.y, 16 + s.t * 20, PAL_FX, false, 0, false);
    spr(art_.guard, gx + (swing_ > 0 ? 4.0f : 0.0f), gy, 64, PAL_GUARD, false, 0, false);
    spr(art_.shadow, gx, gy, 16, PAL_FX, false, 0, true);

    std::vector<int> order;
    order.reserve(raiders_.size());
    for (int i = 0; i < int(raiders_.size()); i++)
        if (raiders_[i].alive || raiders_[i].flash > 0) order.push_back(i);
    std::sort(order.begin(), order.end(), [&](int a, int b) { return raiders_[a].z < raiders_[b].z; });
    for (int i : order) {
        const Raider& r = raiders_[i];
        float sx, sy, sc;
        project(r.x, std::max(r.z, 0.02f), sx, sy, sc);
        int fog = int(std::clamp(r.z * 10.0f, 0.0f, 8.0f));
        if (r.alive) {
            spr(art_.shadow, sx, sy, sc * 0.22f, PAL_FX, false, 0, true);
            spr(art_.raider, sx, sy, sc * 1.15f, PAL_RAID, r.x < 0, fog, false);
        } else {
            spr(art_.spark, sx, sy - sc * 0.4f, 18 + r.flash * 40, PAL_FX, false, 0, false);
        }
    }
    for (const Toss& t : tosses_) {
        float sx, sy, sc;
        project(t.x, t.z, sx, sy, sc);
        spr(art_.stone, sx, sy - sc * 0.45f, 10, PAL_STONE, false, 0, false);
    }

    float wx, wy, ws;
    project(0.0f, 0.02f, wx, wy, ws);
    spr(art_.well, wx, wy + 6, 92, PAL_STONE, false, 0, false);
    spr(art_.shadow, wx, wy + 4, 36, PAL_FX, false, 0, true);

    float ax, ay, as;
    project(0, 0.96f, ax, ay, as);
    for (int i = 0; i < 2; i++) {
        float tx, ty, ts;
        project(i ? 0.95f : -0.95f, 0.72f, tx, ty, ts);
        spr(art_.torch, tx, ty, 28, PAL_FX, false, 2, false);
    }
    spr(art_.arch, ax, ay + 8, 78, PAL_STONE, false, 6, false);

    hud(1, 1, "SALLY WELL", PAL_HUD);
    hud(28, 1, "WAVE " + std::to_string(wave_ + 1) + "/3", PAL_HUD);
    std::string stones(std::max(well_, 0), '#');
    hud(1, 26, "WELL " + stones, PAL_HUD);
    hud(26, 26, std::to_string(score_), PAL_HUD);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    switch (mode_) {
    case Mode::Title:
        draw();
        text("SALLY WELL", 160, 78, 1.15f, PAL_HUD);
        text("KEEP THE WELL", 160, 108, 0.7f, PAL_HUD);
        text("THREE WAVES", 160, 132, 0.6f, PAL_STONE);
        hudC(22, "ARROWS MOVE   A STRIKE   B STONE", PAL_HUD);
        hudC(24, "PRESS START", PAL_HUD);
        if (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A)) {
            mode_ = Mode::Brief;
            banner_ = 1.4f;
            well_ = 8;
            wave_ = 0;
            score_ = 0;
        }
        break;
    case Mode::Brief:
        draw();
        text("HOLD THE SALLY", 160, 96, 0.85f, PAL_HUD);
        hudC(18, "DO NOT LET THEM REACH THE WELL", PAL_HUD);
        banner_ -= DT;
        if (banner_ <= 0) startWatch();
        break;
    case Mode::Watch:
        update();
        draw();
        if (mode_ == Mode::Watch) hudC(3, "WAVE " + std::to_string(wave_ + 1), PAL_HUD);
        break;
    case Mode::Banner:
        banner_ -= DT;
        draw();
        text("THE LINE HOLDS", 160, 100, 0.8f, PAL_HUD);
        if (banner_ <= 0) {
            wave_ += 1;
            beginWave();
            mode_ = Mode::Watch;
        }
        break;
    case Mode::Victory:
        draw();
        text("THE WELL STANDS", 160, 92, 0.85f, PAL_HUD);
        hudC(18, "THREE WAVES HELD", PAL_HUD);
        hudC(20, "SCORE " + std::to_string(score_), PAL_HUD);
        break;
    case Mode::Defeat:
        draw();
        text("THE WATCH IS OVER", 160, 96, 0.75f, PAL_HUD);
        hudC(18, "THE WELL HAS FALLEN", PAL_HUD);
        if (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A)) {
            over_ = false;
            mode_ = Mode::Title;
        }
        break;
    }
}

}  // namespace sally

#include "game/culvert.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace cpurs {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float Z_SPAWN = 16.f;
constexpr float Z_HIT = 1.25f;
constexpr float Z_SPIKE = 3.7f;
constexpr float MOVE = 4.2f;
constexpr float SPIKE_CD = 0.26f;
constexpr int HORIZON = 72;

int hpOf(int kind) { return kind == 2 ? 2 : 1; }
int ptsOf(int kind) { return kind == 2 ? 250 : kind == 1 ? 140 : 100; }
float speedOf(int kind) { return kind == 1 ? 1.72f : kind == 2 ? 1.02f : 1.32f; }

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Victory || mode_ == Mode::Over) return 3;
    if (mode_ != Mode::Watch) return 0;
    int left = 0;
    for (const Mach& m : machs_)
        if (m.alive) ++left;
    left += fleet_ - spawnAt_;
    if (stalled_ > 0 && left == 1) return 2;
    return 1;
}

int Game::aliveCount() const {
    int n = 0;
    for (const Mach& m : machs_)
        if (m.alive) ++n;
    return n;
}

const gs::Mipped& Game::bodyOf(int kind) const {
    if (kind == 1) return art_.roller;
    if (kind == 2) return art_.drainer;
    return art_.pump;
}

int Game::palOf(int kind) const {
    if (kind == 1) return PAL_ROLL;
    if (kind == 2) return PAL_DRAIN;
    return PAL_PUMP;
}

void Game::project(float worldX, float z, float& sx, float& sy, float& s) const {
    float depth = std::max(0.35f, z);
    s = 168.f / (depth + 1.4f);
    sx = 160.f + worldX * s * 0.95f;
    sy = float(HORIZON) + s * 0.72f;
}

void Game::winWatch() {
    if (won_ || mode_ == Mode::Over) return;
    won_ = true;
    over_ = true;
    mode_ = Mode::Victory;
    reason_ = "THE LAST MACHINE STILL RUNNING";
    score_ += 500;
    shake_ = 0.12f;
    sys_->apu.noiseBurst(0.12f, 900.f, 0.06f);
    sys_->apu.keyOn(2, 392.f, 0.16f);
    sys_->rumble(0.2f, 0.45f, 140);
    sys_->setLight(30, 140, 70);
}

void Game::loseWatch(const char* why) {
    if (won_ || mode_ == Mode::Over || mode_ == Mode::Victory) return;
    reason_ = why;
    won_ = false;
    over_ = true;
    mode_ = Mode::Over;
    shake_ = 0.6f;
    sys_->apu.keyOff(0);
    sys_->apu.noiseBurst(0.4f, 90.f, 0.28f);
    sys_->rumble(0.75f, 0.3f, 200);
    sys_->setLight(140, 24, 18);
}

void Game::beginWatch() {
    mode_ = Mode::Watch;
    over_ = false;
    won_ = false;
    reason_ = "THE WATCH IS OVER";
    score_ = 0;
    stalled_ = 0;
    spawnAt_ = 0;
    hull_ = 3;
    px_ = 0;
    watch_ = 0;
    endT_ = 0;
    lunge_ = spikeCd_ = shake_ = scrape_ = 0;
    machs_.clear();
    puffs_.clear();
    static const Row kRows[] = {
        {0.5f, 0, 0}, {3.0f, 1, -1}, {5.6f, 0, 1}, {8.2f, 2, 0}, {12.0f, 1, 1}, {14.6f, 0, -1},
    };
    script_.assign(std::begin(kRows), std::end(kRows));
    fleet_ = int(script_.size());
    if (!sys_) return;
    sys_->apu.keyOn(0, 42.f, 0.05f);
    sys_->setLight(70, 90, 80);
}

void Game::hurt(Mach& m) {
    m.hp -= 1;
    m.flash = 0.12f;
    shake_ = std::max(shake_, 0.18f);
    sys_->apu.noiseBurst(0.18f, 700.f, 0.05f);
    sys_->rumble(0.25f, 0.4f, 50);
    if (m.hp > 0) {
        sys_->apu.keyOn(1, 180.f, 0.12f);
        return;
    }
    m.alive = false;
    stalled_ += 1;
    score_ += m.points;
    Puff p;
    p.x = float(m.lane);
    p.z = m.z;
    p.t = 0.4f;
    p.kind = 0;
    puffs_.push_back(p);
    int n = std::clamp(stalled_ - 1, 0, 4);
    static const float notes[] = {220.f, 277.f, 330.f, 392.f, 494.f};
    sys_->apu.keyOn(1, notes[n], 0.16f);
}

void Game::spike() {
    if (spikeCd_ > 0.f || mode_ != Mode::Watch || won_) return;
    lunge_ = 0.14f;
    spikeCd_ = SPIKE_CD;
    int lane = int(std::lround(px_));
    if (std::fabs(px_ - float(lane)) > 0.35f) lane = 99;
    int hit = -1;
    float best = 1.0e9f;
    if (lane >= -1 && lane <= 1) {
        for (int i = 0; i < int(machs_.size()); ++i) {
            const Mach& m = machs_[size_t(i)];
            if (!m.alive || m.lane != lane || m.z <= Z_HIT || m.z > Z_SPIKE) continue;
            if (m.z < best) {
                best = m.z;
                hit = i;
            }
        }
    }
    if (hit < 0) {
        hull_ -= 1;
        shake_ = std::max(shake_, 0.28f);
        sys_->apu.noiseBurst(0.08f, 200.f, 0.04f);
        if (hull_ <= 0) loseWatch("YOU STALLED YOURSELF");
        return;
    }
    hurt(machs_[size_t(hit)]);
}

void Game::update(float dt) {
    watch_ += dt;
    while (spawnAt_ < int(script_.size()) && script_[size_t(spawnAt_)].t <= watch_) {
        const Row& s = script_[size_t(spawnAt_)];
        ++spawnAt_;
        Mach m;
        m.kind = s.kind;
        m.lane = s.lane;
        m.hp = hpOf(s.kind);
        m.points = ptsOf(s.kind);
        m.z = Z_SPAWN;
        m.alive = true;
        machs_.push_back(m);
    }

    int best = -1;
    float bestZ = 1.0e9f;
    for (int i = 0; i < int(machs_.size()); ++i) {
        const Mach& m = machs_[size_t(i)];
        if (!m.alive) continue;
        if (m.z < bestZ) {
            bestZ = m.z;
            best = i;
        }
    }

    bool want = false;
    float steer = 0.f;
    if (bot_) {
        if (best >= 0) {
            float dest = float(machs_[size_t(best)].lane);
            float step = MOVE * dt;
            if (std::fabs(px_ - dest) <= step) px_ = dest;
            else px_ += (dest > px_ ? step : -step);
        }
    } else {
        const gs::Pad& pad = sys_->pad;
        float digital = float(pad.down(gs::BTN_RIGHT)) - float(pad.down(gs::BTN_LEFT));
        float axis = std::fabs(pad.axisX) > 0.18f ? pad.axisX : digital;
        steer = std::clamp(axis, -1.f, 1.f);
        px_ += steer * MOVE * dt;
        want = pad.pressed(gs::BTN_UP) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) ||
               pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_X) || pad.pressed(gs::BTN_Z);
    }
    float before = px_;
    px_ = std::clamp(px_, -1.08f, 1.08f);
    bool scraped = std::fabs(before) > 1.08f && std::fabs(steer) > 0.45f;
    if (scraped) scrape_ += dt;
    else scrape_ = std::max(0.f, scrape_ - dt * 0.5f);
    if (scrape_ > 0.5f && mode_ == Mode::Watch) {
        hull_ -= 1;
        scrape_ = 0;
        shake_ = 0.4f;
        sys_->apu.noiseBurst(0.1f, 140.f, 0.05f);
        if (hull_ <= 0) {
            loseWatch("YOU STALLED YOURSELF");
            return;
        }
    }

    if (bot_ && best >= 0) {
        const Mach& m = machs_[size_t(best)];
        if (std::fabs(px_ - float(m.lane)) <= 0.2f && m.z <= Z_SPIKE && m.z > Z_HIT + 0.35f) want = true;
    }
    if (want && mode_ == Mode::Watch) spike();
    if (mode_ != Mode::Watch) return;

    for (Mach& m : machs_) {
        if (!m.alive) continue;
        if (m.flash > 0.f) m.flash -= dt;
        m.z -= speedOf(m.kind) * dt;
        if (m.z > Z_HIT) continue;
        m.alive = false;
        Puff puff;
        puff.x = float(m.lane);
        puff.z = Z_HIT;
        puff.t = 0.45f;
        puff.kind = 1;
        puffs_.push_back(puff);
        loseWatch("A MACHINE GOT THROUGH");
        break;
    }
    machs_.erase(std::remove_if(machs_.begin(), machs_.end(), [](const Mach& m) { return !m.alive; }), machs_.end());

    if (mode_ == Mode::Watch && spawnAt_ == int(script_.size()) && aliveCount() == 0 && stalled_ >= fleet_ &&
        fleet_ > 0 && hull_ > 0)
        winWatch();
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog) {
    if (!(h > 1.5f) || m.h < 1 || m.w < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::clamp(int(std::lround(cx - s.w * 0.5f)), -2000, 2000));
    s.y = int16_t(std::clamp(int(std::lround(cy - s.h)), -2000, 2000));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::shadow(float cx, float cy, float w) {
    if (w < 4.f) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 4, 400));
    s.h = int16_t(std::max(3, int(std::lround(double(w) * 0.22))));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = art_.shadow.pick(float(s.h));
    s.pal = 0;
    s.shadow = true;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    float width = 0.f;
    for (unsigned char c : s) {
        if (c <= 32 || c >= 128) width += 12.f * scale;
        else width += float(art_.glyph[c - 32].w) * scale;
    }
    if (width > 304.f) scale *= 304.f / width;
    width = 0.f;
    for (unsigned char c : s) {
        if (c <= 32 || c >= 128) width += 12.f * scale;
        else width += float(art_.glyph[c - 32].w) * scale;
    }
    x -= width * 0.5f;
    for (unsigned char c : s) {
        if (c <= 32 || c >= 128) {
            x += 12.f * scale;
            continue;
        }
        const gs::Mipped& g = art_.glyph[c - 32];
        float gw = float(g.w) * scale;
        spr(g, x + gw * 0.5f, y + float(g.h) * scale, float(g.h) * scale, pal, false);
        x += gw;
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

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::layCulvert(float shx) {
    gs::VDP& v = sys_->vdp;
    uint16_t ceil = mode_ == Mode::Over ? gs::rgb4(4, 1, 1) : gs::rgb4(1, 2, 2);
    uint16_t mouth = mode_ == Mode::Victory ? gs::rgb4(3, 6, 4) : gs::rgb4(2, 3, 3);
    v.setFogColor(mode_ == Mode::Over ? gs::rgb4(5, 1, 1) : gs::rgb4(1, 2, 2));
    for (int y = 0; y < gs::SCREEN_H; ++y) {
        gs::RoadLine& rd = v.road[y];
        if (y < HORIZON) {
            float u = float(y) / float(HORIZON);
            int r = int((1.f - u) * ((ceil >> 8) & 15) + u * ((mouth >> 8) & 15));
            int g = int((1.f - u) * ((ceil >> 4) & 15) + u * ((mouth >> 4) & 15));
            int b = int((1.f - u) * (ceil & 15) + u * (mouth & 15));
            v.lineBackdrop[y] = gs::rgb4(r, g, b);
            v.lineFog[y] = uint8_t(u * 6.f);
            rd.on = false;
            continue;
        }
        float row = float(y - HORIZON) + 1.f;
        rd.on = true;
        rd.cx = 160.f + shx * 0.4f;
        rd.hw = 18.f + row * 0.72f;
        rd.v = 640.f / row + t_ * 6.f;
        rd.pal = uint8_t(PAL_ROAD);
        rd.style = gs::ROAD_RUTS;
        rd.band = (int(std::floor(rd.v / 28.f)) & 1) ? 1 : 0;
        rd.left = gs::GROUND_WATER;
        rd.right = gs::GROUND_WATER;
        float fog = std::clamp(1.f - row / 90.f, 0.f, 1.f);
        v.lineFog[y] = uint8_t(fog * (mode_ == Mode::Over ? 8.f : 11.f));
        v.lineBackdrop[y] = gs::rgb4(1, 2, 2);
    }
    v.roadTime = int(t_ * 60.f);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.hudEnabled = true;

    float shx = shake_ > 0.f ? std::sin(t_ * 90.f) * 5.f * shake_ : 0.f;
    layCulvert(shx);

    struct Item {
        float z;
        int kind;
        int i;
    };
    std::vector<Item> items;
    static const float ribs[] = {5.f, 8.f, 11.f, 14.f};
    for (int i = 0; i < 4; ++i) items.push_back({ribs[i], 1, i});
    items.push_back({6.5f, 2, 0});
    for (int i = 0; i < int(machs_.size()); ++i)
        if (machs_[size_t(i)].alive) items.push_back({machs_[size_t(i)].z, 0, i});
    for (int i = 0; i < int(puffs_.size()); ++i) items.push_back({puffs_[size_t(i)].z, 3, i});
    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) { return a.z > b.z; });

    for (const Item& it : items) {
        if (it.kind == 1) {
            float sx, sy, s;
            project(0.f, it.z, sx, sy, s);
            int fog = std::clamp(int(it.z * 0.7f), 0, 12);
            spr(art_.rib, sx + shx, sy + 8.f, s * 1.35f, PAL_STONE, false, fog);
        } else if (it.kind == 2) {
            float sx, sy, s;
            project(-1.15f, it.z, sx, sy, s);
            spr(art_.grate, sx + shx, sy, s * 0.35f, PAL_STONE, false, 6);
            project(1.15f, it.z, sx, sy, s);
            spr(art_.grate, sx + shx, sy, s * 0.35f, PAL_STONE, true, 6);
        } else if (it.kind == 0) {
            const Mach& m = machs_[size_t(it.i)];
            float sx, sy, s;
            project(float(m.lane), m.z, sx, sy, s);
            float h = s * (m.kind == 2 ? 0.85f : 0.62f);
            int fog = std::clamp(int((m.z - 2.f) * 0.8f), 0, 14);
            if (m.flash > 0.f) fog = 0;
            shadow(sx + shx, sy + 2.f, h * 0.9f);
            spr(bodyOf(m.kind), sx + shx, sy, h, m.flash > 0.f ? PAL_SPARK : palOf(m.kind), m.lane < 0, fog);
        } else {
            const Puff& p = puffs_[size_t(it.i)];
            float sx, sy, s;
            project(p.x, p.z, sx, sy, s);
            spr(p.kind ? art_.spark : art_.puff, sx + shx, sy - s * 0.2f, s * 0.4f, PAL_SPARK, false, 0);
        }
    }

    float psx, psy, ps;
    project(px_, 0.85f, psx, psy, ps);
    float ph = ps * 0.7f * (lunge_ > 0.f ? 1.08f : 1.f);
    shadow(psx + shx, psy + 4.f, ph);
    spr(art_.you, psx + shx, psy - (lunge_ > 0.f ? 6.f : 0.f), ph, PAL_YOU, false, 0);

    if (mode_ == Mode::Title) {
        text("CULVERT PURSE", 160.f, 36.f, 0.85f, PAL_YOU);
        hudC(12, "BE THE LAST MACHINE", PAL_HUD);
        hudC(14, "STILL RUNNING", PAL_WATER);
        hudC(18, "LEFT RIGHT   A SPIKE", PAL_HUD);
        hudC(22, "START", PAL_YOU);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160.f, 70.f, 1.f, PAL_HUD);
        hudC(16, "START RESUMES", PAL_HUD);
    } else if (mode_ == Mode::Victory) {
        text("LAST", 160.f, 34.f, 1.2f, PAL_YOU);
        hudC(16, "THE LAST MACHINE STILL RUNNING", PAL_YOU);
        char buf[64];
        std::snprintf(buf, sizeof buf, "STALLED %d   SCORE %d", stalled_, score_);
        hudC(18, buf, PAL_HUD);
    } else if (mode_ == Mode::Over) {
        text("OVER", 160.f, 34.f, 1.1f, PAL_PUMP);
        hudC(16, "THE WATCH IS OVER", PAL_PUMP);
        hudC(18, reason_, PAL_HUD);
        hudC(22, "START TRIES AGAIN", PAL_HUD);
    } else {
        char buf[48];
        std::snprintf(buf, sizeof buf, "HULL %d", hull_);
        hud(1, 1, buf, hull_ == 1 ? PAL_PUMP : PAL_HUD);
        std::snprintf(buf, sizeof buf, "RUN %d/%d", stalled_, fleet_);
        hud(1, 26, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "%d", score_);
        hud(39 - int(std::strlen(buf)), 26, buf, PAL_HUD);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.85f);
    sys.apu.setEcho(0.18f, 0.32f, 0.2f);
    if (bot_) beginWatch();
    else {
        mode_ = Mode::Title;
        sys.apu.keyOn(0, 34.f, 0.03f);
        sys.setLight(40, 70, 60);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        px_ = std::sin(t_ * 0.7f) * 0.6f;
        if (!bot_ && pad.pressed(gs::BTN_START)) beginWatch();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        }
    } else if (mode_ == Mode::Watch) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else update(DT);
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Watch;
        else if (pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
            machs_.clear();
        }
    } else {
        endT_ += DT;
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            if (mode_ == Mode::Over) beginWatch();
            else {
                mode_ = Mode::Title;
                over_ = false;
                won_ = false;
            }
        }
    }

    if (lunge_ > 0.f) lunge_ = std::max(0.f, lunge_ - DT);
    if (spikeCd_ > 0.f) spikeCd_ = std::max(0.f, spikeCd_ - DT);
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - DT * 1.6f);
    for (Puff& p : puffs_) p.t -= DT;
    puffs_.erase(std::remove_if(puffs_.begin(), puffs_.end(), [](const Puff& p) { return p.t <= 0.f; }), puffs_.end());
    if (mode_ == Mode::Watch) {
        float pitch = 40.f + std::min(watch_, 18.f) * 0.35f + (lunge_ > 0.f ? 12.f : 0.f);
        sys.apu.setFreq(0, pitch);
    }
    draw();
}

}  // namespace cpurs
